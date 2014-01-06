#include <SoftwareSerial.h>

SoftwareSerial GSM(3,2);

void setup()
{
  Serial.begin(9600);
  GSM.begin(9600);
}

void loop()
{
  if(Serial.available()>0)
    GSM.write(Serial.read());
  if(GSM.available()>0)  
    Serial.write(GSM.read());    
}
