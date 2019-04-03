///./////////Arduino 1.0 code//////////////////

#include <SoftwareSerial.h>     //please put less than and greater than sign respectively in place of  =

SoftwareSerial SIM900(10, 11); // (RX, TX). please zoom image above to see exactly what is going where.

char incoming_char = 0;
String data;

void setup()
{
  SIM900power();
  SIM900.begin(19200);               // the SIM900 baud rate   
  Serial.begin(19200);
  delay(100);
  //SIM900.println("ATD+4917812345678;");
  
  data = "";
  SIM900.println("AT+CLCC");
}

void SIM900power()
{
  pinMode(9, OUTPUT); 
  digitalWrite(9,LOW);
  delay(1000);
  digitalWrite(9,HIGH);
  delay(2000);
}

void loop()
{
  if(SIM900.available() > 0) {
    //Get the character from the cellular serial port
    incoming_char=SIM900.read();
    if((int)incoming_char > 11) {
      data += incoming_char;
      if(incoming_char == 13) {
        Serial.print("Erhalten: ");
        Serial.println(data);
        data = "";
      }
      //Print the incoming character to the terminal
//      Serial.print((int)incoming_char);
//      Serial.print(incoming_char);       
    }
  }
  /*
  if (SIM900.available())              // if date is comming from softwareserial port ==> data is comming from gprs shield
  Serial.write(SIM900.read());
  if (Serial.available())                // if data is available on hardwareserial port ==> data is comming from PC or notebook 
  SIM900.write(Serial.read()); 
  */
}
