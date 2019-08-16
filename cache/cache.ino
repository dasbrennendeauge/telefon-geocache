#include <avr/sleep.h>
#include <LiquidCrystal_I2C.h>
#include <DFMiniMp3.h>
#include <SoftwareSerial.h>
#include <Keypad.h>

// GSM Calls: https://www.activexperts.com/serial-port-component/tutorials/gsmdial/

#define TIME_TO_LIVE 300

const byte ROWS = 4; // Four rows
const byte COLS = 3; // Three columns
// Define the Keymap
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'#','0','*'}
};
// Connect keypad ROW0, ROW1, ROW2 and ROW3 to these Arduino pins.
byte rowPins[ROWS] = { 34, 35, 36, 37 };
// Connect keypad COL0, COL1 and COL2 to these Arduino pins.
byte colPins[COLS] = { 31, 32, 33 }; 
// Create the Keypad
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

const int analogInPin = A8;  // Data Muenzeinwurf

// DFPlayer Mini
SoftwareSerial mySoftwareSerial(2, 3); // RX, TX
// implement a notification class,
// its member methods will get called
//
class Mp3Notify {
public:
  static void OnError(uint16_t errorCode) {
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    Serial.println(errorCode);
  }
  static void OnPlayFinished(uint16_t track) {
    Serial.print("Track beendet ");
    Serial.println(track);
    delay(100);
    // TODO: nächsten Schritt aufrufen
  }
  static void OnCardOnline(uint16_t code) {
    Serial.println(F("SD Karte online "));
  }
  static void OnCardInserted(uint16_t code) {
    Serial.println(F("SD Karte bereit "));
  }
  static void OnCardRemoved(uint16_t code) {
    Serial.println(F("SD Karte entfernt "));
  }
};
static DFMiniMp3<HardwareSerial, Mp3Notify> mp3(Serial3);

LiquidCrystal_I2C lcd(0x27, 20, 4); //Hier wird das Display benannt. In unserem //Fall „lcd“. Die I²C Adresse (Erläuterung und I²C Adressen Scanner in folgender Anleitung: Link zur Anleitung „2 I²C Displays gleichzeitig“) 0x27 wird auch angegeben.

// Select your modem:
#define TINY_GSM_MODEM_SIM800

// Increase RX buffer to capture the entire response
// Chips without internal buffering (A6/A7, ESP8266, M590)
// need enough space in the buffer for the entire response
// else data will be lost (and the http library will fail).
#define TINY_GSM_RX_BUFFER 512

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega
// RX118 & TX119
#define SerialAT Serial1

// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "sipgate";
const char gprsUser[] = "sipgate";
const char gprsPass[] = "sipgate";

// Server details
const char server[] = "dev.rotmanov.de";
const char resource[] = "/?number=491787777948";
const int  port = 9992;

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
HttpClient http(client, server, port);

unsigned long startMillis;
unsigned long stepStartMillis;
int step = 0;
bool refreshDisplay = false;
char number[20] = "";
int numberCounter = 0;

String expected = String("4711"); // TODO: ersetze mit angerufener Nummer
String pin = String("");

void setup() {
  Serial.begin(115200); // Es gibt ein paar Debug Ausgaben über die serielle Schnittstelle
  Serial.println("Arduino start");

  // DFPlayer Mini initialisieren
  mp3.begin();
  mp3.setVolume(15);
  
  // -> Arduino start -> GSM start
 
  // Set GSM module baud rate
  SerialAT.begin(19200);
  Serial.println("GSM start");

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  // modem.restart();
  modem.init();

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);
  
  lcd.begin();
  lcd.backlight();//Beleuchtung des Displays einschalten
  startMillis = millis();
  refreshDisplay = true;
}

// -> Displayanzeige "Hallo, willkommen zu meinem Cache. Du hast 5 Minuten Zeit und nur einen Versuch."
// -> Sound abspielen "Hallo, willkommen zu meinem Cache. Du hast 5 Minuten Zeit und nur einen Versuch."
// -> Display zeigt 5-Minuten-Zähler an

void loop() {
  mp3.loop();
  if(step == 0) {
    step0();
  } else if(step == 1) {
    step1();
  } else if(step == 2) {
    step2();
  } else if(step == 3) {
    step3();
  } else if(step == 4) {
    step4();
  } else if(step == 5) {
    step5();
  } else if(step == 6) {
    step6();
  }
  decreaseTimer();
}

void step0() {
  if(refreshDisplay == true) {
    showTextAndPlayMp3(
      "                    ",
      "Hoerer ans Ohr!     ",
      "                    ", 1);
    refreshDisplay = false;
  }
  unsigned long currentMillis = millis();
  int secondsElapsed = (currentMillis - startMillis) / 1000;
  if(secondsElapsed > 2) {
    step = 1;
    mp3.playMp3FolderTrack(1);
    stepStartMillis = millis();
    refreshDisplay = true;
  }
}

void step1() {
  if(refreshDisplay == true) {
    showTextAndPlayMp3(
      "Willkommen!         ",
      "Hoerer ans Ohr!     ",
      "                    ", 1);
    refreshDisplay = false;
  }

    unsigned long currentMillis = millis();
    int secondsElapsed = (currentMillis - stepStartMillis) / 1000;
    if(secondsElapsed > 6) {
      step = 2;
      mp3.playMp3FolderTrack(2);
      refreshDisplay = true;
    }
}

void step2() {
  if(refreshDisplay == true) {
    lcd.clear();
    showTextAndPlayMp3(
      "Deine Handynummer:  ",
      number,
      "Ende mit Leertaste  ", 2);  
    refreshDisplay = false;
  }

  char key = kpd.getKey();
  if(key) {
    if(key == '#') {
      step = 3;
      mp3.playMp3FolderTrack(3);
      refreshDisplay = true;
      return;
    }
    number[numberCounter] = key;
    numberCounter++;
    Serial.println(key);
    refreshDisplay = true;
  }
}

void step3() {
  if(refreshDisplay == true) {
    showTextAndPlayMp3(
      "Münze einwerfen     ",
      "                    ",
      "(Keine Rueckgabe)   ", 3);  
    refreshDisplay = false;
  }

  // read the analog in value:
  int sensorValue = analogRead(analogInPin);

  if (sensorValue != 0) {
    step = 4;
    mp3.playMp3FolderTrack(4);
    stepStartMillis = millis();
    refreshDisplay = true;    
    Serial.println("Pling!");
  }
}

void step4() {
  if(refreshDisplay == true) {
    showTextAndPlayMp3(
      "Danke.              ",
      "Ich rufe dich an.   ",
      "NICHT ABHEBEN!      ", 4);  
    refreshDisplay = false;
  }

  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" OK");

  if (modem.isNetworkConnected()) {
    Serial.println("Network connected");
  }

    Serial.print(F("Connecting to "));
    Serial.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      Serial.println(" fail");
      delay(10000);
      return;
    }
    Serial.println(" OK");

  Serial.print(F("Performing HTTP GET request... "));
  int err = http.get(resource);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    delay(10000);
    return;
  }

  int status = http.responseStatusCode();
  Serial.print(F("Response status code: "));
  Serial.println(status);
  if (!status) {
    delay(10000);
    return;
  }

  String body = http.responseBody();
  Serial.println(F("Response:"));
  Serial.println(body);

  Serial.print(F("Body length is: "));
  Serial.println(body.length());

  expected = body;
  
  // Shutdown

  http.stop();
  Serial.println(F("Server disconnected"));

  modem.gprsDisconnect();
  Serial.println(F("GPRS disconnected"));

    unsigned long currentMillis = millis();
    int secondsElapsed = (currentMillis - stepStartMillis) / 1000;
    if(secondsElapsed > 5) {
      step = 5;
      mp3.playMp3FolderTrack(5);
      refreshDisplay = true;
    }
}

void step5() {
  if(refreshDisplay == true) {
    lcd.clear();
    showTextAndPlayMp3(
      "Wer rief dich an?   ",
      "Gib letzte 4 Zahlen:",
      pin.c_str(), 5);  
    refreshDisplay = false;
  }

  char key = kpd.getKey();
  if(key) {
    pin.concat(key);
    Serial.println(key);
    if(pin.length() == 4) {
      step = 6;
      mp3.playMp3FolderTrack(6);      
      refreshDisplay = true;
      return;
    }
    refreshDisplay = true;
  }
}

void step6() {
  if(refreshDisplay == true) {
    lcd.clear();
    if(pin == expected) {
      // TODO: Klappe oeffnen
      showTextAndPlayMp3(
        "Geschafft.          ",
        "Trag dich ein, dann ",
        "Klappe zu.          ", 6);
    } else {
      showTextAndPlayMp3(
        "Computer sagt:      ",
        "NEIN                ",
        "                    ", 6);
    }
    refreshDisplay = false;
  }
}

void showTextAndPlayMp3(char *line1, char *line2, char *line3, uint16_t track) {
  lcd.setCursor(0,0); //Text soll beim ersten Zeichen in der ersten Reihe beginnen..
  lcd.print(line1); //In der ersten Zeile soll der Text „Test Zeile 1“ angezeigt werden
  lcd.setCursor(0,1); //Genauso geht es bei den weiteren drei Zeilen weiter
  lcd.print(line2);
  lcd.setCursor(0,2);
  lcd.print(line3);
  Serial.println(line1);
  Serial.println(line2);
  Serial.println(line3);
}

void decreaseTimer() {
  unsigned long currentMillis = millis();
  int secondsRemaining = TIME_TO_LIVE - ((currentMillis - startMillis) / 1000);
  if(secondsRemaining <= 0) {
    enterSleep();
  }
  char line[20] = "";
  sprintf(line, "%3d Sekunden   ", secondsRemaining);
  lcd.setCursor(0,3);
  lcd.print(line);
}

void enterSleep() {
  lcd.clear();
  lcd.noBacklight();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
}
