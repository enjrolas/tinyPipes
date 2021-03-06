#include <Streaming.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include "delay.h"
#include "samples.h"
#include "floatToString.h"
#include "stringHandling.h"
#include "charging.h"
#include "blink.h"
#include "pipe.h"


//#define DEBUG 0

#define PANEL_EN 5
#define PANEL_CURRENT 0
#define PANEL_VOLTAGE 1
#define BATTERY_VOLTAGE 2
#define GSM_POWER 4
#define CHARGING 8 
#define ENABLED 9
#define SIM_STATUS 6
#define LOAD_EN 8


float panelCurrent, panelVoltage, batteryVoltage;
float wattSeconds, averageBatteryVoltage, averagePanelVoltage;
int index=0;
int samples=0;
int cycles=0;  //number of charge cycles
int m=0;

SoftwareSerial GSM(3,2);

struct Sample currentSample;
struct Sample data;
long start, cycleTime;



void setup()
{
  Serial.begin(9600);
  GSM.begin(9600);
  pinMode(PANEL_EN, OUTPUT);
  pinMode(CHARGING, OUTPUT);
  pinMode(ENABLED, OUTPUT);
  pinMode(LOAD_EN, OUTPUT);
  pinMode(GSM_POWER, OUTPUT);
  pinMode(SIM_STATUS, INPUT);
  Serial.println("TinyPipes is booting up...");
  startBlinking();  //flash the ENABLED LED until the GSM module is ready to receive messages
  bootGSMModule();
  checkSIM();
  wattSeconds=0;
  samples=0;
  initPipe();
  setupWatchdog();
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
    watchdogDelay(2000);
    digitalWrite(GSM_POWER, LOW);
    watchdogDelay(2000);
    booted=true;
  }
  if(booted)
  {
#ifdef DEBUG
    Serial<<"letting the module finish booting\n";
#endif
    watchdogDelay(7000);
    flushBuffer();
  }
#ifdef DEBUG
  Serial.println("GSM module is powered up");
#endif
}

void deleteAllMessages()
{
  unsigned char i;
  for(i=0;i<50;i++)
  {
#ifdef DEBUG  
    Serial<<"deleting ..."<<i<<endl;
#endif
    deleteMessage(i);
    watchdogDelay(1000);
  }
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

void updateSignalStrength()
{
  flushBuffer();
  GSM.println("AT+CSQ");
  unsigned char i;
  watchdogDelay(500);
  for(i=0;i<2;i++)
    readLine();
  String line=readLine();
  Serial.println(line);
  String signalStrength=line.substring(line.indexOf(":")+1, line.indexOf(","));
  signalStrength.trim();
  Serial.println(signalStrength);
  char index[10];
  signalStrength.toCharArray(index, 10);
  signal=map(atoi(index),2,30,-110, -54);
  Serial<<signalStrength<<" "<<signal<<endl;
}




int checkSIM()
{
  updateSignalStrength();
  GSM.println("AT+CPIN?");
  readLine();  //the first line is blank
  readLine();  //the second line is blank, too
  String response=readLine();
  Serial.println(response);
  if((response.indexOf("ERROR")>=0) || (response.indexOf("NOT")>=0) || (response.equals("")))
  {
#ifdef DEBUG  
    Serial.println("looks like there's some problem with the SIM.  Rebooting the cell module");
#endif
    bootGSMModule();
    return -1;  //some problemo with the SIM-O
  }
  else
  {
    stopBlinking();
    digitalWrite(ENABLED, HIGH);  //turn on the ENABLED LED to indicate that the SIM module is now ready for business
    return 0;  //all's well
  }
}

void flushBuffer()
{
  while(GSM.available()>0)
    GSM.read();
}

void readMessage(int index)
{
  GSM.print("AT+CMGF=1\r");    //enter text mode for messages
  watchdogDelay(500);
  flushBuffer();
  GSM.print("AT+CMGR=");
  GSM.print(index, DEC);
  GSM.print("\r\n");

  String from, date, time, timeStamp;


  int i;
  String message[2];
  readLine();
  readLine();
  for(i=0;i<2;i++)
    message[i]=readLine();


  int firstComma=message[0].indexOf(",");
  int secondComma=message[0].indexOf(",", firstComma+1);
  from=message[0].substring(firstComma+1, secondComma);
  timeStamp=message[0].substring(message[0].lastIndexOf("\"",message[0].length()-2));
  from.replace("\"","");
  timeStamp.replace("\"","");
  /*
  Serial<<"Message metadata: "<<message[0]<<endl;
  Serial<<"Message content: "<<message[1]<<endl;
  Serial<<"Time Stamp: "<<timeStamp<<endl;
  Serial<<"From Number: "<<from<<endl;
  */
  Serial<<message[0]<<endl<<message[1]<<endl<<timeStamp<<endl<<from<<endl;
  message[1].trim();

  char **words;
  size_t len=0;
  char temp[160];
  message[1].toCharArray(temp, 160);
  words = split(temp, " ", &len);

  if(len>=2)
  {
    strcpy(temp, words[0]);
    Serial.println(temp);
    if(strcmp(temp,"text")==0)
    {
#ifdef DEBUG
      Serial.println("got a command to send out a text");
#endif
      String textString="";
      unsigned char i, count;
      for(i=2;i<len;i++)
      {
        strcpy(temp, words[i]);
        for(count=0;((count<sizeof(temp))&&(temp[count]!='\n'));count++)
          textString+=temp[count];
#ifdef DEBUG  
        Serial.println("adding a new word to our message content");
        Serial.println(temp);
        Serial.println(sizeof(temp));
#endif
        if(i<(len-1))
          textString+=" ";
#ifdef DEBUG  
        Serial.print("message so far: ");
        Serial.println(textString);
#endif
      }
      strcpy(temp, words[1]);
      Serial.println(temp);
      Serial.println(String(temp));
      sendSMS(String(temp), textString);
    }
  }

  else
  {
    if(message[1].equalsIgnoreCase("connect"))  
    {
      setCharging();
      if(verbose)
        sendSMS(from, "disconnected");
    }
    if(message[1].equalsIgnoreCase("disconnect"))
    {
      setDisconnected();
      if(verbose)
        sendSMS(from, "charging");
    }
    if(message[1].equalsIgnoreCase("status"))
      statusResponse(from);
//      sendSMS(from, generateStatusMessage());

    if(message[1].equalsIgnoreCase("version"))  //return the pipe's firmware version
      sendSMS(from, pipeVersion);

    if(message[1].equalsIgnoreCase("verbose"))  //return the pipe's firmware version
    {
      setVerboseMode();
      sendSMS(from, "verbose mode");
    }

    if(message[1].equalsIgnoreCase("quiet"))  //return the pipe's firmware version
    {
      setQuietMode();
      sendSMS(from, "quiet mode");
    }


    if(message[1].equalsIgnoreCase("test"))
    {
      startBlinking();
      testResponse(from);
      //      sendSMS(from, generateTestMessage());
      stopBlinking();
    }

//    if(message[1].indexOf("load")>=0)
//      load(message[1].substring(4, message[1].length()));  //it comes in the format "load1510XXXXXXXXXXXXXX", so slice out the "load" part and sent it on
  }

  free4split(words);
}


/*
String generateTestMessage()
 {
 unsigned char i;
 String statusString="";
 boolean fail=false;
 #ifdef DEBUG  
 Serial.println("generating status message");
 #endif
 disconnectPanel();  //disconnect the panel from the battery, so we can get good measurements of panel and battery voltage
 float disconnectedPanelVoltage=analogRead(PANEL_VOLTAGE)*5*4.15/1023;
 float disconnectedBatteryVoltage=analogRead(BATTERY_VOLTAGE)*5*4.15/1023;
 connectPanel();
 watchdogDelay(1000);    
 panelCurrent=analogRead(PANEL_CURRENT)*4.15/1023/100/.003;
 float connectedPanelVoltage=analogRead(PANEL_VOLTAGE)*5*4.15/1023;
 float connectedBatteryVoltage=analogRead(BATTERY_VOLTAGE)*5*4.15/1023;
 
 char reason[25];
 if(disconnectedBatteryVoltage<10)
 {
 fail=true;
 strcpy(reason,"low battery voltage");
 }
 
 if(disconnectedPanelVoltage<10)  //this is ONLY a daytime check, but we're checking to make sure there's a minimum VOC on the panel
 {
 fail=true;
 strcpy(reason,"low panel voltage");
 }
 
 if(panelCurrent==0)  // this is ONLY a daytime check -- at night, the panel current will definitely be zero.
 {
 fail=true;
 strcpy(reason,"battery not charging");
 }
 
 if(disconnectedBatteryVoltage>disconnectedPanelVoltage)  //this is ONLY a daytime check --- we check that the VOC of the panel is greater than the VOC of the battery, indicating that the panel _can_ charge the battery
 {
 fail=true;
 strcpy(reason,"panel VOC too low");
 }
 
 if(fail)
 {
 statusString="test failed,";
 statusString+=reason;
 statusString+=",";
 }
 else
 statusString="test passed,_,";
 if(charging)
 statusString+="charging,";
 else
 statusString+="disconnected,";
 statusString+=pipeVersion;  
 statusString+=',';
 char temp[10];
 floatToString(temp,disconnectedPanelVoltage,2);
 statusString+=temp;
 statusString+=",";
 floatToString(temp,disconnectedBatteryVoltage,2);
 statusString+=temp;
 statusString+=",";
 floatToString(temp,panelCurrent,2);
 statusString+=temp;
 statusString+=",";
 floatToString(temp,connectedPanelVoltage,2);
 statusString+=temp;
 statusString+=",";
 floatToString(temp,connectedBatteryVoltage,2);
 statusString+=temp;
 Serial<<statusString<<endl;
 return statusString;
 }
 */

void printTestMessage()
{
  unsigned char i;
  //  String statusString="";
  boolean fail=false;
  disconnectPanel();  //disconnect the panel from the battery, so we can get good measurements of panel and battery voltage
  float disconnectedPanelVoltage=analogRead(PANEL_VOLTAGE)*5*4.15/1023;
  float disconnectedBatteryVoltage=analogRead(BATTERY_VOLTAGE)*5*4.15/1023;
  connectPanel();
  watchdogDelay(1000);    
  panelCurrent=analogRead(PANEL_CURRENT)*4.15/1023/100/.003;
  float connectedPanelVoltage=analogRead(PANEL_VOLTAGE)*5*4.15/1023;
  float connectedBatteryVoltage=analogRead(BATTERY_VOLTAGE)*5*4.15/1023;

  char reason[25];
  if(disconnectedBatteryVoltage<10)
  {
    fail=true;
    strcpy(reason,"low battery voltage");
  }

  if(disconnectedPanelVoltage<10)  //this is ONLY a daytime check, but we're checking to make sure there's a minimum VOC on the panel
  {
    fail=true;
    strcpy(reason,"low panel voltage");
  }

  if(panelCurrent==0)  // this is ONLY a daytime check -- at night, the panel current will definitely be zero.
  {
    fail=true;
    strcpy(reason,"battery not charging");
  }

  if(disconnectedBatteryVoltage>disconnectedPanelVoltage)  //this is ONLY a daytime check --- we check that the VOC of the panel is greater than the VOC of the battery, indicating that the panel _can_ charge the battery
  {
    fail=true;
    strcpy(reason,"panel VOC too low");
  }

  if(fail)
    GSM<<"test failed,"<<reason<<",";
  else
    GSM<<"test passed,_,";
  if(charging)
    GSM<<"charging,";
  else
    GSM<<"disconnected,";
  GSM<<pipeVersion<<",";
  char temp[10];
  floatToString(temp,disconnectedPanelVoltage,2);
  GSM<<temp<<",";
  floatToString(temp,disconnectedBatteryVoltage,2);
  GSM<<temp<<",";
  floatToString(temp,panelCurrent,2);
  GSM<<temp<<",";
  floatToString(temp,connectedPanelVoltage,2);
  GSM<<temp<<",";
  floatToString(temp,connectedBatteryVoltage,2);
  GSM<<temp<<","<<signal<<"dBm"<<endl;
  
}

/*
String generateStatusMessage()
{
  unsigned char i;
  String statusString="";
  char temp[10];
#ifdef DEBUG  
  Serial.println("generating status message");
#endif
  for(i=1;i<=8;i++)
  {
    readSample(data, samplePeriods-i);
    floatToString(temp,data.panelVoltage,2);
    statusString+=temp;
    statusString+=",";
    floatToString(temp,data.batteryVoltage,2);
    statusString+=temp;
    statusString+=",";
    floatToString(temp,data.panelEnergy,2);
    statusString+=temp;
    statusString+=",";
  }
  Serial.println(statusString);
  return statusString;
}
*/
void statusResponse(String number)
{
  GSM.print("AT+CMGF=1\r");    //Because we want to send the SMS in text mode
  watchdogDelay(500);
  GSM.print("AT+CMGS=\"");
  GSM.print(number);
  GSM.println("\"");//send sms message, be careful need to add a country code before the cellphone number
  watchdogDelay(500);
  printStatusMessage();
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

void printStatusMessage()
{
  unsigned char i;
  char temp[10];
#ifdef DEBUG  
  Serial.println("generating status message");
#endif
  for(i=1;i<=8;i++)
  {
    readSample(data, samplePeriods-i);
    GSM<<data.panelVoltage<<","<<data.batteryVoltage<<","<<data.panelEnergy<<",";
    Serial<<data.panelVoltage<<","<<data.batteryVoltage<<","<<data.panelEnergy<<",";
  }
  GSM<<endl;
  Serial<<endl;
}

//deletes an SMS message from memory
void deleteMessage(int index)
{
  GSM.print("AT+CMGD=");
  GSM.println(index);  

}

void sendSMS(String number, String message)
{
  Serial<<message<<endl;

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

void testResponse(String number)
{
  updateSignalStrength();
  GSM.print("AT+CMGF=1\r");    //Because we want to send the SMS in text mode
  watchdogDelay(500);
  GSM.print("AT+CMGS=\"");
  GSM.print(number);
  GSM.println("\"");//send sms message, be careful need to add a country code before the cellphone number
  watchdogDelay(500);
  printTestMessage();
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

void loop()
{
  checkForMessages();
  watchdogDelay(PERIOD*1000);
  readValues();
  printValues();
  if(charging)
    chargeBattery();
  else
    disconnectPanel();
}

void checkAllMessages()
{
#ifdef DEBUG  
  Serial.println("checking messages");
#endif
  GSM.println("AT+CMGL=\"ALL\"\r\n");
  watchdogDelay(5000);
  while(GSM.available()>0)
    Serial.print((char)GSM.read());
}

void checkForMessages()
{
  if(GSM.available()>0)
  {
    String line=readLine();
    Serial.println(line);
    if(line.indexOf("CMTI")>=0)
    {
      unsigned char firstComma=line.indexOf(",");
      String num=line.substring(firstComma+1, line.length());
      char index[10];
      num.toCharArray(index, 10);
      Serial.println(num);
      Serial.println(atoi(index),DEC);
      readMessage(atoi(index));
      deleteMessage(atoi(index));         
    } 
  }
}

void writeCycleTime()
{
  cycleTime=millis()-start;
  index+=sizeof(cycleTime);
  start=millis();
}

void readValues()
{

  panelCurrent=0;
  panelVoltage=0;
  batteryVoltage=0;
  int i;
  for(i=0;i<SAMPLES;i++)
  {
    panelCurrent+=analogRead(PANEL_CURRENT)*4.15/1023/50/.003;
    panelVoltage+=analogRead(PANEL_VOLTAGE)*5*4.15/1023;
    batteryVoltage+=analogRead(BATTERY_VOLTAGE)*5*4.15/1023;
  }
  panelCurrent/=SAMPLES;
  panelVoltage/=SAMPLES;
  batteryVoltage/=SAMPLES;
  averagePanelCurrent=panelCurrent;
  currentSample.panelEnergy+=PERIOD*panelCurrent*panelVoltage/3600;  //keep track of the watt-seconds the panel's generated this hour.
  currentSample.panelVoltage+=panelVoltage;
  currentSample.batteryVoltage+=batteryVoltage;
  currentSample.batteryEnergy++;  //dummy value for now
  samples++;
  if(samples>(SAMPLE_PERIOD/PERIOD))  // if we've taken samples for an hour, save em!
  {
    saveValues();
    samples=0;    
  }
  if(samples%SIM_CHECK==0)
    if(GSM.available()==0)  //if there's data from the module, we know it's working.  No need to check
      checkSIM();
}

void saveValues()
{
  currentSample.batteryVoltage/=(SAMPLE_PERIOD/PERIOD);
  currentSample.panelVoltage/=(SAMPLE_PERIOD/PERIOD);
  currentSample.panelEnergy/=(SAMPLE_PERIOD/PERIOD);
  saveSample(currentSample);

  currentSample.panelEnergy=0;
  currentSample.batteryVoltage=0;
  currentSample.panelVoltage=0;
  currentSample.batteryEnergy=0;  
}

void printValues()
{
#ifdef DEBUG  
  Serial<<"sample "<<samples<<" ";
  Serial<<"Status: ";
  if(charging)
    Serial<<"charging ";
  else
    Serial<<"disconnected ";
  cycleTime=millis()-start;
  cycleTime/=1000;
  int minutes=cycleTime/60;
  int seconds=cycleTime%60;
  Serial<<"Panel Current: "<<panelCurrent<<" Panel Voltage: "<<panelVoltage<<" panel PWM:  "<<panelPWM<<" Battery Voltage: "<<batteryVoltage<<" cycle time:  "<<minutes<<":"<<seconds<<"\n";
#endif
}

void connectLoad()
{
  digitalWrite(LOAD_EN, HIGH);
}

void disconnectLoad()
{
  digitalWrite(LOAD_EN, LOW);
}

void connectPanel()
{
  digitalWrite(PANEL_EN, HIGH);
  digitalWrite(CHARGING, HIGH);
}

void chargeBattery(){
  digitalWrite(CHARGING, HIGH);
  Serial.println(batteryVoltage);
  Serial.println(panelCurrent);
  if(chargeMode==CONSTANT_VOLTAGE)
  {
    if(((14.4-batteryVoltage)/14.4)>0.05)
      increment=1;
    else
      increment=0.2;
#ifdef DEBUG  
    Serial.println("constant voltage charging");
#endif
    if(batteryVoltage<(FLOAT_VOLTAGE-VOLTAGE_HYSTERESIS))
      panelPWM+=increment;
    else if(batteryVoltage>(FLOAT_VOLTAGE+VOLTAGE_HYSTERESIS))
      panelPWM-=increment;

    CV_cycles++;
    if(CV_cycles>30)
    {
      if(panelPWM<30)  //low panelPWM indicates that we have a high panel voltage and a low current
      {
        if(CV_cycles%300==0)  //check every five minutes to see if we really should be in CV mode
        {  //if we're at low PWM and high voltage (>13V), we're in the right place, and we're topping off the battery
          //low PWM and low voltage is a no-no.  We should be back to CC mode.
          disconnectPanel();
          watchdogDelay(10000);  //wait ten seconds for the battery voltage to settle
          readValues();
          if(batteryVoltage<13)
          {
            CC_cycles=0;
            chargeMode=CONSTANT_CURRENT;
          }
        }
      }
      if(panelPWM>100)  //if we're opening it up full throttle to charge, we should be in CC mode
      {
        panelPWM=85;
        CC_cycles=0;
        chargeMode=CONSTANT_CURRENT;
      }
    }
    if(panelPWM>255)
      panelPWM=255;
    if(panelPWM<0)
      panelPWM=0;
    analogWrite(PANEL_EN, panelPWM);
  }

  else if(chargeMode==CONSTANT_CURRENT)
  {
#ifdef DEBUG  
    Serial.println("constant current charging");
#endif
    digitalWrite(PANEL_EN, HIGH);
    unsigned char i;
    CC_cycles++;
    for(i=0;i<SET_POINTS-1;i++)
    {
      if((panelCurrent<currentPoints[i])&&(panelCurrent>currentPoints[i+1]))  //check what bin the current is in
      {
        Serial.print(currentPoints[i+1]);
        Serial.print(" ");
        Serial.println(voltagePoints[i+1]);
        if(CC_cycles>10)
          if(batteryVoltage>voltagePoints[i+1])  //our voltage is too high for this current.  Switch to constant voltage mode
          {
            chargeMode=CONSTANT_VOLTAGE;
            CV_cycles=0;          
          }
      }
    }
  }


  if(panelVoltage<batteryVoltage)  //the panel voltage is lower than the battery voltage.  Might as well disconnect the battery
  {
#ifdef DEBUG  
    Serial.println("guess it's nighttime.  Disconnecting the panel");
#endif
    disconnectPanel();
  }
}


void disconnectPanel()
{
  digitalWrite(PANEL_EN, LOW);
  digitalWrite(CHARGING, LOW);
}






