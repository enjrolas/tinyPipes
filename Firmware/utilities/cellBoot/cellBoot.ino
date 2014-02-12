#include <Streaming.h>
#include <I2C16.h>           // Don't miss this line!
#include <EEPROM_24XX1025.h>
#include <SoftwareSerial.h>
#include "delay.h"
#include "samples.h"
#include<stdlib.h>

#define SAMPLES 100
#define DEBUG 1
#define PANEL_CURRENT 0
#define PANEL_VOLTAGE 1
#define BATTERY_VOLTAGE 2
#define GSM_POWER 4
#define PANEL_EN 5
#define SIM_STATUS 6

#define INDEX 20  //the data will be stored starting at this location


int m=0;
boolean GPRSSetup=false;
boolean panelConnected=false;

int cycles=0;
SoftwareSerial GSM(3,2);
// Initialize the EEPROM with address bits A0 = 0, A1 = 0 (in that order)
// This is set by connecting the physical pins 1 and 2
// to either ground (for 0) or VDD (for 1).
EEPROM_24XX1025 eeprom (0, 0);
#define WP 9

void setup()
{
  Serial.begin(38400);
  GSM.begin(38400);
  pinMode(WP, OUTPUT);
  pinMode(PANEL_EN, OUTPUT);
  pinMode(GSM_POWER, OUTPUT);
  pinMode(SIM_STATUS, INPUT);
  Serial.println("TinyPipes is booting up...");
  checkSIM();
  bootGSMModule();
  showHelpMenu();
}

//boots or reboots the GSM module 
void bootGSMModule()
{
  while(digitalRead(SIM_STATUS)==0)
  {
    Serial.println("it seems that the module is off.  Booting it");
    digitalWrite(GSM_POWER, HIGH);
    delay(2000);
    digitalWrite(GSM_POWER, LOW);
    delay(2000);
  }
  Serial.println("GSM module is powered up");
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
  if(Serial.available()>0)
  {
    char command=(char)Serial.read();
    switch(command){
      case('g'): //get request
        saveTCPComm();
//        saveHttpRequest("http://valve.tinypipes.net/firmware/", INDEX);
        Serial.println("firmware download complete");
          //readEEPROM();
        break;
      case('r'):  //read the saved data
        readEEPROM();
        break;

    }
  }      
}

void readEEPROM()
{
    Serial.println("reading EEPROM contents");
        eeprom.setPosition(INDEX);
        unsigned long endPosition=eeprom.readUInt();
        Serial<<endPosition-INDEX<<" bytes\n";
        for(int i=0;i<endPosition-INDEX;i++)
          Serial.print((char)eeprom.readByte());
}

void showHelpMenu()
{
  Serial.println("enter one of the following commands to control the firmware update tester");
  Serial.println("'g' to load the firmware from http://valve.tinypipes.net/firmware and save it to EEPROM");
  Serial.println("'r' to read the firmware from EEPROM and print it to the screen");       
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

void setupGPRS()
{
  GSM.println("AT+CGATT?");
  watchdogDelay(500);
 
  showSerialData();
 
  GSM.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");//setting the SAPBR, the connection type is using gprs
  watchdogDelay(1000);
 
  showSerialData();
 
  GSM.println("AT+SAPBR=3,1,\"APN\",\"CMNET\"");//setting the APN, the second need you fill in your local apn server
//  GSM.println("AT+SAPBR=3,1,\"APN\",\"hkcsl\"");//setting the APN, the second need you fill in your local apn server
  watchdogDelay(4000);
 
  showSerialData();
 
  GSM.println("AT+SAPBR=1,1");//setting the SAPBR, for detail you can refer to the AT command mamual
  watchdogDelay(2000);
 
  showSerialData(); 
}

//saveTCPcomm
void saveTCPComm()
{
  if(!GPRSSetup)
  {
    setupGPRS();
    GPRSSetup=true;
  }

  GSM.println("AT+CIPSTART=\"tcp\",\"valve.tinypipes.net\",\"80\"");//start up the connection
 delay(500);
 GSM.println("AT+CIPSEND");//begin send data to remote server
 delay(500);
 GSM.println("GET /firmware/ HTTP/1.1");
 GSM.println("host: valve.tinypipes.net");
 GSM.println((char)26);//sending
 while(GSM.available()>0)
    Serial<<(char)GSM.read();

 GSM.println("AT+CIPCLOSE");//close the connection
 }


//saveHttpRequest
//this submits a GET request to the url parameter and saves all data returned from the server to the EEPROM, starting at <index>
void saveHttpRequest(String url, unsigned long index)
{
  if(!GPRSSetup)
  {
    setupGPRS();
    GPRSSetup=true;
  }
  GSM.println("AT+HTTPINIT"); //init the HTTP request
 
  watchdogDelay(500); 
  showSerialData();
 
  GSM.print("AT+HTTPPARA=\"URL\",\"");
  GSM.print(url);
  GSM.println("\"");// setting the httppara, the second parameter is the website you want to access
  watchdogDelay(500);
 
  showSerialData();
 
  GSM.println("AT+HTTPACTION=0");//submit the request 
  watchdogDelay(5000);//the delay is very important, the delay time is base on the return from the website, if the return datas are very large, the time required longer.
  //while(!GSM.available());
 
  showSerialData();
 
  GSM.println("AT+HTTPREAD");// read the data from the website you access
  delay(100);
//  unsigned char i;
//  for(i=0;i<3;i++)
//    readLine();
  // Write something out.
  digitalWrite(WP, LOW);
  eeprom.setPosition(index+4);  //the first four bytes will hold the end position
  while(GSM.available()>0)
  {
    byte a=GSM.read();
    Serial<<GSM.available()<<" "<<(char)a<<"\n";
    eeprom.writeByte(a);
    if(GSM.available()==0)    
      delay(100);
  }
  GSM.println("AT+HTTPTERM");
  unsigned long finalPosition=eeprom.getPosition();
  eeprom.setPosition(index);
  eeprom.writeUInt(finalPosition);
}

void showSerialData()
{
  while(GSM.available()>0)
  {
    Serial.write(GSM.read());
    watchdogDelay(1);
  }
}

