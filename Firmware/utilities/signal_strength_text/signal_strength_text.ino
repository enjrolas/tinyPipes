#include <Streaming.h>
#include <SoftwareSerial.h>
#include "delay.h"

#define DEBUG 1
#define GSM_POWER 4

int cycles=0;
SoftwareSerial GSM(3,2);

void setup()
{
  Serial.begin(9600);
  GSM.begin(9600);
  pinMode(GSM_POWER, OUTPUT);
  digitalWrite(GSM_POWER, HIGH);
  delay(3000);
  digitalWrite(GSM_POWER, LOW);
  Serial.println("TinyPipes is booting up...");
  delay(10000);
  checkSIM();
  //sendSMS("+639293143222","entering into the realm of the living");  //alex number
 // sendSMS("+639156568684","entering into the realm of the living");  //hannah number
 // sendSMS("+639209011401","entering into the realm of the living");  //tonito number
  //sendSMS("+639471782972","entering into the realm of the living");
  setupWatchdog();
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
  watchdogDelay(2000);
  //turn it on
  digitalWrite(GSM_POWER, HIGH);
  watchdogDelay(3000);
  digitalWrite(GSM_POWER, LOW);

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


int checkSIM()
{
  flush();
  GSM.println("AT+CPIN?");
  watchdogDelay(500);
  readLine();  //the first line is blank
  readLine();  //the second line is blank, too
  String response=readLine();
  Serial.println(response);
  if((response.indexOf("ERROR")>=0) || (response.indexOf("NOT")>=0))
  {
    #ifdef DEBUG  
    Serial.println("looks like there's some problem with the SIM.  Rebooting the cell module");
    #endif
    bootGSMModule();
    return -1;  //some problemo with the SIM-O
  }
  else
    return 0;  //all's well

}

void flush()
{
  Serial.println("flushing...");
  while(GSM.available()>0)
    Serial.write(GSM.read());
}

void loop()
{
  if(cycles%5==0)
  {
    textSignalStrength("+8613632651167");
    checkSIM();
  }
  watchdogDelay(1000);
  cycles++;
}

void textSignalStrength(String number)
{
  flush();
  GSM.println("AT+CSQ");
  int i;
  watchdogDelay(500);
  for(i=0;i<2;i++)
    Serial<<"line "<<i<<": "<<readLine()<<"\r\n";
  String line=readLine();
  Serial.println(line);
  String signalStrength=line.substring(line.indexOf(":")+1, line.indexOf(",")-1);
  signalStrength.trim();
  Serial.println(signalStrength);
  char index[10];
  signalStrength.toCharArray(index, 10);
  int db=map(atoi(index),2,3,-110, -54);
  Serial.println(db);
 // sendSMS(number, line);
}

void sendSMS(String number, String message)
{

  GSM.print("AT+CMGF=1\r");    //Because we want to send the SMS in text mode
  watchdogDelay(500);
  GSM.print("AT+CMGS=\"");
  GSM.print(number);
  GSM.println("\"");//send sms message, be careful need to add a country code before the cellphone number
  watchdogDelay(500);
  GSM.println(message);//the content of the message
  watchdogDelay(500);
  GSM.println((char)26);//the ASCII code of the ctrl+z is 26
  watchdogDelay(500);
  GSM.println();
  watchdogDelay(4000);
  GSM.print("AT+CMGDA=\"DEL SENT\"\r\n");  //delete all sent messages
  watchdogDelay(1000);
  readLine();
  readLine();

}

