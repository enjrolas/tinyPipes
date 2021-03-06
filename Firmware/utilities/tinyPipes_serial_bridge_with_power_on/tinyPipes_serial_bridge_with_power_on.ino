#include <SoftwareSerial.h>
#include <Streaming.h>
#include "watchdog.h"

#define GSM_POWER 4
#define SIM_STATUS 6
#define DEBUG 1

SoftwareSerial GSM(3,2);
String line;

void setup()
{
  pinMode(GSM_POWER, OUTPUT);
  pinMode(SIM_STATUS, INPUT);
  Serial.begin(9600);
  GSM.begin(9600);
  setupWatchdog();
  bootGSMModule();
}

void loop()
{
    wdt_reset();
  if(Serial.available()>0)
    GSM.write(Serial.read());
  if(GSM.available()>0)
    Serial.write(GSM.read());    
}

//boots or reboots the GSM module 
void bootGSMModule()
{
  boolean booted=false;
  while(digitalRead(SIM_STATUS)==0)
  {
    #ifdef DEBUG
    Serial.println("it seems that the module is off.  Booting it");
    #endif
    digitalWrite(GSM_POWER, HIGH);
    delay(2000);
    digitalWrite(GSM_POWER, LOW);
    delay(2000);
    booted=true;
  }
  if(booted)
  {
    #ifdef DEBUG
    Serial<<"letting the module finish booting\n";
    #endif
    delay(5000);
  }
  Serial.println("GSM module is powered up");
}

void flushBuffer()
{
  Serial.println("flushing...");
  while(GSM.available()>0)
    Serial.write(GSM.read());
}
String readLine()
{
  String line="";
  char a=' ';
  long timeout=2000;
  long start=millis();
  do
  {
    if(GSM.available()>0)
    {
      a=GSM.read();
      line+=a;
      wdt_reset();
    }
  }
  while((a!='\n')&&((millis()-start)<timeout));
  if((millis()-start)>timeout)
#ifdef DEBUG  
    Serial<<"string timed out:"<<line<<endl;
#endif
  line.trim();
  return line;
}

