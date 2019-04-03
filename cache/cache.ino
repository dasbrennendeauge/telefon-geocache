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

SoftwareSerial SIM900(10, 11); 

unsigned long startMillis;
int step = 1;
bool refreshDisplay = false;
char number[20] = "";
int numberCounter = 0;

void setup() {
  Serial.begin(115200); // Es gibt ein paar Debug Ausgaben über die serielle Schnittstelle
  SIM900.begin(19200);
  Serial.println("Arduino start");
  // -> Arduino start -> GSM start
  lcd.begin();
  lcd.backlight();//Beleuchtung des Displays einschalten
  startMillis = millis();
  refreshDisplay = true;
  pinMode(53, INPUT);
}

// -> Displayanzeige "Hallo, willkommen zu meinem Cache. Du hast 5 Minuten Zeit und nur einen Versuch."
// -> Sound abspielen "Hallo, willkommen zu meinem Cache. Du hast 5 Minuten Zeit und nur einen Versuch."
// -> Display zeigt 5-Minuten-Zähler an

void loop() {
  if(step == 1) {
    step1();
  } else if(step == 2) {
    step2();
  } else if(step == 3) {
    step3();
  } else if(step == 4) {
    step4();
  }
  decreaseTimer();
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
    int secondsElapsed = (currentMillis - startMillis) / 1000;
    if(secondsElapsed > 1) {
      step = 2;
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

  int val = digitalRead(53);
  if(val == 1) {
    step = 4;
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
  // TODO: GSM anruf starten
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
