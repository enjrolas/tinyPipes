#include <SoftwareSerial.h>
#define SIM_POWER 4
#define SIM_STATUS 6
SoftwareSerial GSM(3,2);

void setup()
{
  Serial.begin(38400);
  GSM.begin(38400);
  pinMode(SIM_POWER, OUTPUT);
  pinMode(SIM_STATUS, INPUT);
  while(digitalRead(SIM_STATUS)==0)
  {
//    Serial.println("it seems that the module is off.  Booting it");
    digitalWrite(SIM_POWER, HIGH);
    delay(2000);
    digitalWrite(SIM_POWER, LOW);
    delay(1000);
  }
//  Serial.println("ok the module is booted");
 // at();
}

void at()
{
  int i;
  for(i=0;i<100;i++)
  {
    GSM.print("AT\r\n");
    Serial.print("AT\r\n");
    delay(500);
  while(GSM.available()>0)  
    Serial.write(GSM.read());    
  }
}

void loop()
{
  if(Serial.available()>0)
    GSM.write(Serial.read());
  if(GSM.available()>0)  
    Serial.write(GSM.read());    
}
