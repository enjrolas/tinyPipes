#include <SoftwareSerial.h>
#include <Streaming.h>
#include "watchdog.h"

#define GSM_POWER 4
#define DEBUG 1

SoftwareSerial GSM(3,2);
String line;

void setup()
{
  pinMode(GSM_POWER, OUTPUT);
  Serial.begin(9600);
  GSM.begin(9600);
  Serial.println("waking up");
  setupWatchdog();
  bootGSMModule();
  diagnostics();
}

void diagnostics()
{
  command("AT+CGMI", "Manufacturer");
  command("AT+CGMM", "Module");
  command("AT+CGMR", "Revision");
  command("AT+CNUM", "Phone Number");
  command("AT+CSQ",  "Signal Strength");
}

void command(String commandString, String label)
{
  Serial<<label<<":\r\n";
  GSM.println(commandString);
  int i;
  for(i=0;i<4;i++)
    readLine();
  watchdogDelay(1000);
}

void loop()
{
  wdt_reset();
  if(millis()%30000==0)
    diagnostics();
  if(Serial.available()>0)
    GSM.write(Serial.read());
  if(GSM.available()>0)
    Serial.write(GSM.read());    
}

//boots or reboots the GSM module 
void bootGSMModule()
{
  GSM.write("AT\r\n");
  readLine();
  readLine();
  String line=readLine();
  if(line.equals("OK"))  //module is up, turn it off
  {
#ifdef DEBUG  
    Serial.println("module is on, turning it off");
#endif
    GSM.print("AT+CPOWD=1\r\n");
  }

  delay(1000);

  Serial.println("booting the module");
  //turn it on
  digitalWrite(GSM_POWER, HIGH);
  watchdogDelay(3000);
  digitalWrite(GSM_POWER, LOW);
  watchdogDelay(3000);
  Serial.println("module is booted");
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
  Serial.println(line);
  return line;
}


