#include <avr/sleep.h>
#include <LiquidCrystal_I2C.h>
#include <DFMiniMp3.h>
#include <Keypad.h>

// GSM Calls: https://www.activexperts.com/serial-port-component/tutorials/gsmdial/

// ---------------------------------------------------------------------------
// Konfiguration
// ---------------------------------------------------------------------------
#define RELAIS_PIN 10
#define TINY_GSM_DEBUG Serial

#define GSM_PIN "6348"        // PIN der SIM-Karte

// Zeiten
#define TIME_TO_LIVE       300   // Gesamt-Timeout einer Session in Sekunden
#define STEP_HOLD_SECONDS    6   // Anzeige-/Ansagedauer (Begruessung, "geschlossen")
#define ERROR_HOLD_SECONDS   30  // Anzeigedauer bei Netzwerk-/Modemfehler
#define CALL_RING_MS      5000   // Klingeldauer beim Rueckruf
#define RETRY_DELAY_MS   10000   // Wartezeit nach Netz-/Verbindungsfehler
#define RELAY_PULSE_MS    1000  // Relais-Impuls zum Oeffnen der Klappe
#define MP3_VOLUME          15

// Oeffnungszeiten (Stunde, einschliesslich)
#define OPEN_FROM_HOUR  8
#define OPEN_TO_HOUR    22

// MP3-Tracks auf der SD-Karte (Ordner 1)
#define TRACK_WELCOME       1   // Begruessung
#define TRACK_ENTER_NUMBER  2   // Handynummer eingeben
#define TRACK_INSERT_COIN   3   // Muenze einwerfen
#define TRACK_THANKS        4   // Danke / Rueckruf laeuft
#define TRACK_ENTER_PIN     5   // PIN eingeben
#define TRACK_SUCCESS       6   // geschafft
#define TRACK_CLOSED        7   // ausserhalb der Oeffnungszeiten
#define TRACK_WRONG_PIN     8   // PIN falsch
#define TRACK_HOLD          9   // Wartemusik waehrend Modem/Anruf-Phase

// Schritte der Zustandsmaschine
enum Step {
  STEP_GREETING,      // Hoerer ans Ohr, Taste 5
  STEP_WELCOME,       // Begruessung, Netz & Uhrzeit pruefen
  STEP_ENTER_NUMBER,  // Handynummer eingeben
  STEP_INSERT_COIN,   // Muenze einwerfen
  STEP_CALL,          // Server abfragen + zurueckrufen
  STEP_ENTER_PIN,     // letzte 4 Ziffern eingeben
  STEP_RESULT,        // Ergebnis (Klappe auf / falsch)
  STEP_CLOSED,        // ausserhalb der Oeffnungszeiten
  STEP_ERROR          // Netzwerk-/Modemfehler -> Owner informieren
};

// ---------------------------------------------------------------------------
// Tastenfeld (3x4-Matrix)
// ---------------------------------------------------------------------------
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'#', '0', '*'}
};
byte rowPins[ROWS] = { 34, 35, 36, 37 };  // ROW0..ROW3
byte colPins[COLS] = { 31, 32, 33 };      // COL0..COL2
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

const int analogInPin = A13;  // Muenzeinwurf (Lichtschranke)

// ---------------------------------------------------------------------------
// DFPlayer Mini (MP3) ueber Serial3
// ---------------------------------------------------------------------------
class Mp3Notify {
  public:
    static void OnError(uint16_t errorCode) {
      // siehe DfMp3_Error fuer die Bedeutung
      Serial.println();
      Serial.print("Com Error ");
      Serial.println(errorCode);
    }
    static void OnPlayFinished(uint16_t track) {
      Serial.print("Track beendet ");
      Serial.println(track);
      delay(100);
    }
    static void OnCardOnline(uint16_t code)   { Serial.println(F("SD Karte online ")); }
    static void OnCardInserted(uint16_t code) { Serial.println(F("SD Karte bereit ")); }
    static void OnCardRemoved(uint16_t code)  { Serial.println(F("SD Karte entfernt ")); }
    static void OnUsbOnline(uint16_t code)    { Serial.println(F("USB online ")); }
    static void OnUsbInserted(uint16_t code)  { Serial.println(F("USB bereit ")); }
    static void OnUsbRemoved(uint16_t code)   { Serial.println(F("USB entfernt ")); }
};
static DFMiniMp3<HardwareSerial, Mp3Notify> mp3(Serial3);

// ---------------------------------------------------------------------------
// LCD (20x4, I2C-Adresse 0x27)
// ---------------------------------------------------------------------------
#define LCD_COLS 20
LiquidCrystal_I2C lcd(0x27, LCD_COLS, 4);

// ---------------------------------------------------------------------------
// GSM / Netz (SIM800 ueber Serial1)
// ---------------------------------------------------------------------------
#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 512   // ganze Antwort puffern
#define SerialAT Serial1         // Hardware-Serial des Mega: RX1 = D19, TX1 = D18

// GPRS-Zugangsdaten (leer lassen, falls nicht noetig)
const char apn[]      = "sipgate";
const char gprsUser[] = "sipgate";
const char gprsPass[] = "sipgate";

// Server
const char server[] = "dev.rotmanov.de";
const int  port     = 9992;
String url = String("/?number=");

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
HttpClient http(client, server, port);

// ---------------------------------------------------------------------------
// Zustand
// ---------------------------------------------------------------------------
unsigned long startMillis;       // Anker fuer den Session-Countdown
unsigned long stepStartMillis;   // Anker fuer schritt-lokale Wartezeiten
Step step = STEP_GREETING;
bool refreshDisplay = false;
bool outOfOrder = false;

String phoneNumber = String("");
String expected = String("");    // vom Server geliefert (erwarteter PIN)
String pin = String("");

// Vorab deklariert, damit die Schritt-Funktionen sie nutzen koennen.
// Kooperative Wartefunktionen: ruft mp3.loop() und decreaseTimer() auf,
// damit MP3-Callbacks und Countdown auch waehrend langer Modem-Wartezeiten
// weiterlaufen.
void coopDelay(unsigned long ms);
bool waitForNetworkCoop(uint32_t timeout_ms = 60000L);
void coopYield();

void setup() {
  Serial.begin(115200); // ein paar Debug-Ausgaben ueber die serielle Schnittstelle
  Serial.println("Arduino start");

  pinMode(RELAIS_PIN, OUTPUT);
  digitalWrite(RELAIS_PIN, HIGH); // Relais aus (Klappe zu)

  mp3.begin();
  mp3.setVolume(MP3_VOLUME);

  lcd.begin();
  lcd.backlight();
  startMillis = millis();
  refreshDisplay = true;
}

void loop() {
  mp3.loop();
  switch (step) {
    case STEP_GREETING:     stepGreeting();    break;
    case STEP_WELCOME:      stepWelcome();     break;
    case STEP_ENTER_NUMBER: stepEnterNumber(); break;
    case STEP_INSERT_COIN:  stepInsertCoin();  break;
    case STEP_CALL:         stepCall();        break;
    case STEP_ENTER_PIN:    stepEnterPin();    break;
    case STEP_RESULT:       stepResult();      break;
    case STEP_CLOSED:       stepClosed();      break;
    case STEP_ERROR:        stepError();       break;
  }
  decreaseTimer();
}

// Hoerer ans Ohr -> GSM initialisieren, auf Taste warten
bool modemReady = false;  // true sobald modem.init() durchgelaufen ist

void stepGreeting() {
  if (refreshDisplay && !modemReady) {
    showText(
      "",
      "Bitte warten...",
      "");
    refreshDisplay = false;

    SerialAT.begin(19200);
    Serial.println("GSM start");

    // restart() dauert lange; init() ueberspringt den Neustart
    Serial.println("Initializing modem...");
    modem.init();

    String modemInfo = modem.getModemInfo();
    Serial.print("Modem: ");
    Serial.println(modemInfo);

    if (GSM_PIN && modem.getSimStatus() != 3) {
      modem.simUnlock(GSM_PIN);
    }

    modemReady = true;
    refreshDisplay = true;
  }

  if (refreshDisplay && modemReady) {
    showText(
      "H\xEFrer ans Ohr!",
      "",
      "Dr""\xF5""cke dann Taste 5");
    refreshDisplay = false;
  }

  if (modemReady && kpd.getKey()) {
    step = STEP_WELCOME;
    mp3.playMp3FolderTrack(TRACK_WELCOME);
    stepStartMillis = millis();
    refreshDisplay = true;
  }
}

// Begruessung, Netz suchen und Uhrzeit gegen die Oeffnungszeiten pruefen
void stepWelcome() {
  if (refreshDisplay) {
    showText(
      "Willkommen!",
      "Deine Zeit l\xE1uft...",
      "");
    refreshDisplay = false;

    unsigned long netStart = millis();
    Serial.println("Waiting for network...");

    bool netOk = waitForNetworkCoop();
    if (netOk) {
      int secondsElapsed = (millis() - netStart) / 1000;
      Serial.print("Network connected");
      Serial.println(secondsElapsed);
    } else {
      Serial.println("Network timeout");
    }

    coopDelay(500);

    if (netOk) {
      String gsmTime = modem.getGSMDateTime(DATE_TIME);
      Serial.print("GSM Time:");
      Serial.println(gsmTime);
      int hourI = gsmTime.substring(0, 2).toInt();
      Serial.println(hourI);
      if (hourI > OPEN_TO_HOUR || hourI < OPEN_FROM_HOUR) {
        outOfOrder = true;
      }
    } else {
      // Kein Netz -> Owner informieren statt Oeffnungszeiten zu zeigen
      outOfOrder = false;
    }
  }

  int secondsElapsed = (millis() - stepStartMillis) / 1000;
  if (secondsElapsed > STEP_HOLD_SECONDS) {
    if (!modem.isNetworkConnected()) {
      stepStartMillis = millis();
      step = STEP_ERROR;
    } else if (outOfOrder) {
      stepStartMillis = millis();
      step = STEP_CLOSED;
    } else {
      step = STEP_ENTER_NUMBER;
      mp3.playMp3FolderTrack(TRACK_ENTER_NUMBER);
    }
    refreshDisplay = true;
  }
}

// Handynummer eingeben, Abschluss mit '#'
void stepEnterNumber() {
  if (refreshDisplay) {
    lcd.clear();
    showText(
      "Deine Handynummer:",
      phoneNumber.c_str(),
      "Ende mit Leertaste");
    refreshDisplay = false;
  }

  char key = kpd.getKey();
  if (key) {
    if (key == '#') {
      step = STEP_INSERT_COIN;
      url.concat(phoneNumber);
      mp3.playMp3FolderTrack(TRACK_INSERT_COIN);
      refreshDisplay = true;
      return;
    }
    phoneNumber.concat(key);
    refreshDisplay = true;
  }
}

// Auf den Muenzeinwurf (Lichtschranke) warten
void stepInsertCoin() {
  if (refreshDisplay) {
    showText(
      "M\xF5nze einwerfen",
      "",
      "(Keine R""\xF5""ckgabe)");
    refreshDisplay = false;
  }

  int sensorValue = analogRead(analogInPin);
  if (sensorValue != 0) {
    step = STEP_CALL;
    mp3.playMp3FolderTrack(TRACK_THANKS);
    stepStartMillis = millis();
    refreshDisplay = true;
    Serial.println("Pling!");
  }
}

// Server abfragen (liefert PIN, setzt Absender-Rufnummer) und zurueckrufen
void stepCall() {
  if (refreshDisplay) {
    showText(
      "Danke.",
      "Ich rufe dich an.",
      "NICHT ABHEBEN!");

    // Warteansage abspielen, solange das Modem arbeitet (Netz, GPRS,
    // HTTP, Anruf). Wird gestoppt, sobald der Anruf klingelt.
    mp3.playMp3FolderTrack(TRACK_HOLD);

    Serial.print("Waiting for network...");
    if (!waitForNetworkCoop()) {
      Serial.println(" fail");
      coopDelay(RETRY_DELAY_MS);
      return;
    }
    Serial.println(" OK");

    if (modem.isNetworkConnected()) {
      Serial.println("Network connected");
    }
    coopYield();

    Serial.print(F("Connecting to "));
    Serial.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      Serial.println(" fail");
      coopDelay(RETRY_DELAY_MS);
      return;
    }
    Serial.println(" OK");
    coopYield();

    Serial.print(F("Performing HTTP GET request... "));
    Serial.print(url);
    int err = http.get(url);
    if (err != 0) {
      Serial.println(F("failed to connect"));
      coopDelay(RETRY_DELAY_MS);
      return;
    }

    int status = http.responseStatusCode();
    Serial.print(F("Response status code: "));
    Serial.println(status);
    if (!status) {
      coopDelay(RETRY_DELAY_MS);
      return;
    }
    coopYield();

    // responseBody() kooperativ nachgebaut: liest Byte fuer Byte mit
    // Timeout, dabei wird coopYield() aufgerufen, damit mp3.loop() und
    // decreaseTimer() weiterlaufen. endOfBodyReached() wird wie im Original
    // ausgewertet (nutzt Content-Length).
    String body = "";
    int bodyLength = http.contentLength();
    if (bodyLength > 0) {
      body.reserve(bodyLength);
    }
    unsigned long readStart = millis();
    while (!http.endOfBodyReached()) {
      if (http.available()) {
        int c = http.read();
        if (c >= 0) {
          body.concat((char)c);
        }
      }
      coopYield();
      if (millis() - readStart > 10000) {
        Serial.println(F("Body read timeout"));
        break;
      }
    }
    body.trim();
    Serial.println(F("Response:"));
    Serial.println(body);
    Serial.print(F("Body length is: "));
    Serial.println(body.length());

    expected = body;

    http.stop();
    Serial.println(F("Server disconnected"));
    coopYield();

    modem.gprsDisconnect();
    Serial.println(F("GPRS disconnected"));
    coopYield();

    // Nutzer anrufen, damit sein Telefon mit der vom Server gesetzten
    // Absender-Rufnummer (+491579999<code>) klingelt. NICHT abheben.
    Serial.print(F("Calling "));
    Serial.println(phoneNumber);
    bool ringing = modem.callNumber(phoneNumber);
    Serial.println(ringing ? F("Ringing") : F("Call failed"));
    if (ringing) {
      // Warteansage stoppen, sobald der Anruf klingelt, damit der Spieler
      // das Klingeln am Hoerer hoert.
      mp3.stop();
      coopDelay(CALL_RING_MS);
      modem.callHangup();
      Serial.println(F("Call hung up"));
    }

    step = STEP_ENTER_PIN;
    mp3.playMp3FolderTrack(TRACK_ENTER_PIN);
    refreshDisplay = true;
  }
}

// Letzte 4 Ziffern der angezeigten Rufnummer eingeben
void stepEnterPin() {
  if (refreshDisplay) {
    lcd.clear();
    showText(
      "Wer rief dich an?",
      "Gib letzte 4 Zahlen:",
      pin.c_str());
    refreshDisplay = false;
  }

  char key = kpd.getKey();
  if (key) {
    pin.concat(key);
    if (pin.length() == 4) {
      step = STEP_RESULT;
      refreshDisplay = true;
      return;
    }
    refreshDisplay = true;
  }
}

// Ergebnis: PIN korrekt -> Klappe oeffnen, sonst Hinweis
void stepResult() {
  if (refreshDisplay) {
    lcd.clear();
    if (pin == expected) {
      digitalWrite(RELAIS_PIN, LOW);   // Relais an -> Klappe auf
      coopDelay(RELAY_PULSE_MS);
      digitalWrite(RELAIS_PIN, HIGH);  // Relais wieder aus

      mp3.playMp3FolderTrack(TRACK_SUCCESS);
      showText(
        "Geschafft.",
        "Trag dich ein, dann",
        "Klappe zu.");
    } else {
      mp3.playMp3FolderTrack(TRACK_WRONG_PIN);
      showText(
        "PIN nicht korrekt :(",
        "Auflegen &",
        "neu versuchen");
    }
    refreshDisplay = false;
  }
}

// Ausserhalb der Oeffnungszeiten
void stepClosed() {
  if (refreshDisplay) {
    lcd.clear();
    mp3.playMp3FolderTrack(TRACK_CLOSED);
    showText(
      "Cache-Zeit von",
      "8 Uhr bis 22 Uhr",
      "Sorry. :'-(");
    refreshDisplay = false;
  }

  int secondsElapsed = (millis() - stepStartMillis) / 1000;
  if (secondsElapsed > STEP_HOLD_SECONDS) {
    enterSleep();
  }
}

// Netzwerk- oder Modemfehler -> Besucher soll den Owner informieren
void stepError() {
  if (refreshDisplay) {
    lcd.clear();
    showText(
      "Netzwerk-Fehler!",
      "Bitte Owner",
      "informieren.");
    refreshDisplay = false;
  }

  int secondsElapsed = (millis() - stepStartMillis) / 1000;
  if (secondsElapsed > ERROR_HOLD_SECONDS) {
    enterSleep();
  }
}

// ---------------------------------------------------------------------------
// Hilfsfunktionen
// ---------------------------------------------------------------------------
void showText(const char *line1, const char *line2, const char *line3) {
  printLcdLine(0, line1);
  printLcdLine(1, line2);
  printLcdLine(2, line3);
  Serial.println(line1);
  Serial.println(line2);
  Serial.println(line3);
}

// Schreibt eine Zeile linksbuendig in die angegebene Zeile und fuellt mit
// Leerzeichen auf LCD_COLS auf bzw. kuerzt zu lange Texte.
void printLcdLine(uint8_t row, const char *text) {
  lcd.setCursor(0, row);
  uint8_t i = 0;
  while (i < LCD_COLS && text[i] != '\0') {
    lcd.write(text[i]);
    i++;
  }
  while (i < LCD_COLS) {
    lcd.write(' ');
    i++;
  }
}

void decreaseTimer() {
  int secondsRemaining = TIME_TO_LIVE - ((millis() - startMillis) / 1000);
  if (secondsRemaining <= 0) {
    enterSleep();
  }
  char line[20] = "";
  sprintf(line, "%3d Sekunden   ", secondsRemaining);
  lcd.setCursor(0, 3);
  lcd.print(line);
}

void enterSleep() {
  lcd.clear();
  lcd.noBacklight();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
}

// Kooperative Wartezeit: haelt mp3.loop() (fuer Callbacks wie
// OnPlayFinished) und den Session-Countdown am Laufen, waehrend gewartet
// wird. So ruckelt der Timer nicht, wenn laenger auf das Modem oder einen
// HTTP-Response gewartet wird. Darf nicht aus einem mp3-Callback heraus
// aufgerufen werden (Rekursion ueber mp3.loop()).
void coopDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    mp3.loop();
    decreaseTimer();
  }
}

// Ein einzelner Durchlauf durch mp3.loop() + decreaseTimer().
// Zum Einstreuen zwischen blockierenden Aufrufen (gprsConnect, http.get,
// callNumber, ...), damit der Timer zwischen den AT-Kommandos weiterlaeuft.
void coopYield() {
  mp3.loop();
  decreaseTimer();
}

// Non-blocking Variante von modem.waitForNetwork(): pollt isNetworkConnected()
// in einer Schleife und haelt dabei mp3.loop() und decreaseTimer() am Leben.
// Gibt true zurueck, wenn das Netz innerhalb des Timeouts verfuegbar ist.
bool waitForNetworkCoop(uint32_t timeout_ms) {
  for (uint32_t start = millis(); millis() - start < timeout_ms;) {
    if (modem.isNetworkConnected()) {
      return true;
    }
    coopDelay(250);
  }
  return false;
}
