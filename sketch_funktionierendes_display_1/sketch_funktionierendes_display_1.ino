#include <Wire.h> //Wire Bibliothek einbinden

#include <LiquidCrystal_I2C.h>

#include <SoftwareSerial.h>     //please put less than and greater than sign respectively in place of  =

SoftwareSerial SIM900(10, 11); // (RX, TX). please zoom image above to see exactly what is going where.

LiquidCrystal_I2C lcd(0x27, 20, 4); //Hier wird das Display benannt. In unserem //Fall „lcd“. Die I²C Adresse (Erläuterung und I²C Adressen Scanner in folgender Anleitung: Link zur Anleitung „2 I²C Displays gleichzeitig“) 0x27 wird auch angegeben.

void setup()
{
lcd.begin();
lcd.backlight();//Beleuchtung des Displays einschalten
   SIM900.begin(9600);               // the SIM900 baud rate   
  Serial.begin(9600);
  Serial.println("An");
}

void loop()
{
if(SIM900.available()) {
  Serial.write(SIM900.read());
}

if(Serial.available()) {
  SIM900.write(Serial.read());
}

lcd.setCursor(0,0); //Text soll beim ersten Zeichen in der ersten Reihe beginnen..

lcd.print("Hallo"); //In der ersten Zeile soll der Text „Test Zeile 1“ angezeigt werden

lcd.setCursor(0,1); //Genauso geht es bei den weiteren drei Zeilen weiter

lcd.print("wirf Geld ein");

lcd.setCursor(0,2);

lcd.print("Ruf dich an");

lcd.setCursor(0,3);

lcd.print("8");

}
