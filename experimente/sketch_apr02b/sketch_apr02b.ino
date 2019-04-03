#define TINY_GSM_MODEM_SIM900
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(10, 11); // RX, TX

#include <TinyGsmClient.h>

TinyGsm modem(SerialAT);


void setup() {
  Serial.begin(19200);
  Serial.println("Start");
  delay(100);
  SIM900power();
  Serial.println("Start2");
  TinyGsmAutoBaud(SerialAT);
   Serial.println("Start3");
  String modemInfo = modem.getModemInfo();
  Serial.println(modemInfo);

   Serial.println("Start4");
    Serial.println("Waiting for network...");
  if (!modem.waitForNetwork()) {
    delay(10000);
    return;
  }

   Serial.println("Start5");
  if (modem.isNetworkConnected()) {
    Serial.println("Network connected");
  }

  modem.sendAT("AT");
  modem.waitResponse();

  bool res = modem.callNumber("+4919876543");
  Serial.println(res ? "Call OK" : "Call fail");

  delay(1000);

  res = modem.callHangup();
  Serial.println(res ? "Hangup OK" : "Hanup fail");
}

void loop() {
  
}

void SIM900power()
{
  pinMode(9, OUTPUT); 
  digitalWrite(9,LOW);
  delay(1000);
  digitalWrite(9,HIGH);
  delay(2000);
}
