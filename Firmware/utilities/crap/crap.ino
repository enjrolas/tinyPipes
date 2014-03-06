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
  watchdogDelay(1000);
  String num=getPhoneNumber();
  Serial<<"phone number is: "<<num<<"\n";
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


String getPhoneNumber()
{
  flushBuffer();
  //the AT command to return phone number
  //returns it in this format:
  //
  //+CNUM: "","13714281494",129,7,4

  //this function pulls the meat of the number from that line and returns the String
  //13714281494
  
  GSM.println("AT+CNUM");
  delay(500);
  readLine();
  readLine();  //skip the first blank line
  String line=readLine();
  unsigned char firstComma=line.indexOf(',');
  unsigned char secondComma=line.indexOf(',', firstComma+1);
  line=line.substring(firstComma+2, secondComma-1);  //chop the number out from between the first and second commas, and cut the quotation marks out, while we're at it
  return line;
}


