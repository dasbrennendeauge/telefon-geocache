#include <avr/sleep.h>
#include <LiquidCrystal_I2C.h>
#include <DFMiniMp3.h>
#include <SoftwareSerial.h>

#define TIME_TO_LIVE 300

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
    Serial.print("Track beendet");
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
static DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(mySoftwareSerial);

LiquidCrystal_I2C lcd(0x27, 20, 4); //Hier wird das Display benannt. In unserem //Fall „lcd“. Die I²C Adresse (Erläuterung und I²C Adressen Scanner in folgender Anleitung: Link zur Anleitung „2 I²C Displays gleichzeitig“) 0x27 wird auch angegeben.

unsigned long startMillis;

void setup() {
  Serial.begin(115200); // Es gibt ein paar Debug Ausgaben über die serielle Schnittstelle
  // -> Arduino start -> GSM start
  lcd.begin();
  lcd.backlight();//Beleuchtung des Displays einschalten
  startMillis = millis();
}

// -> Displayanzeige "Hallo, willkommen zu meinem Cache. Du hast 5 Minuten Zeit und nur einen Versuch."
// -> Sound abspielen "Hallo, willkommen zu meinem Cache. Du hast 5 Minuten Zeit und nur einen Versuch."
// -> Display zeigt 5-Minuten-Zähler an

void loop() {
  showTextAndPlayMp3(
    "Willkommen!         ",
    "Hoerer ans Ohr!     ",
    "                    ", 1);
  decreaseTimer();
  delay(200);
}

void showTextAndPlayMp3(char *line1, char *line2, char *line3, uint16_t track) {
  lcd.setCursor(0,0); //Text soll beim ersten Zeichen in der ersten Reihe beginnen..
  lcd.print(line1); //In der ersten Zeile soll der Text „Test Zeile 1“ angezeigt werden
  lcd.setCursor(0,1); //Genauso geht es bei den weiteren drei Zeilen weiter
  lcd.print(line2);
  lcd.setCursor(0,2);
  lcd.print(line3);
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

