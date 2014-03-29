#include <EEPROM.h>

#include <Streaming.h>
#include <SoftwareSerial.h>
SoftwareSerial GSM(3,2);

#include "delay.h"
#include "EEPROM_functions.h"
#include "stringHandling.h"
#include<stdlib.h>

// #define DEBUG 1
#define GSM_POWER 4
#define PANEL_EN 5
#define SIM_STATUS 6

boolean tcpSetup=false;
boolean httpSetup=false;
boolean panelConnected=false;

int cycles=0;

void setup()
{
  Serial.begin(9600);
  GSM.begin(9600);
  pinMode(PANEL_EN, OUTPUT);
  pinMode(GSM_POWER, OUTPUT);
  pinMode(SIM_STATUS, INPUT);
  Serial.println("TinyPipes is booting up...");
  bootGSMModule();
  checkSIM();
  setupWatchdog();
}

//boots or reboots the GSM module 
void bootGSMModule()
{
  boolean booted=false;
  //if the module is on, turn it off
  if(digitalRead(SIM_STATUS)==0)
  {
    digitalWrite(GSM_POWER, HIGH);
    watchdogDelay(2000);
    digitalWrite(GSM_POWER, LOW);
    watchdogDelay(3000);
  }
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
    watchdogDelay(5000);
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
#ifdef DEBUG  
  if((millis()-start)>timeout)
    Serial<<"string timed out:"<<line<<endl;
#endif
  line.trim();
  return line;
}


char checkSIM()
{
  flushBuffer();
  GSM.println("AT+CPIN?");
  watchdogDelay(500);
  readLine();  //the first line is blank
  readLine();  //the second line is blank, too
  char response[50];
  horribleReadline(response, sizeof(response));
  Serial.println(response);
  if((strstr(response, "ERROR")!=NULL) || (strstr(response, "NOT")!=NULL))
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

void flushBuffer()
{
  Serial.println("flushing...");
  while(GSM.available()>0)
  {
    Serial.write(GSM.read());
    wdt_reset();  //if I don't pet the dog, I can actually get stuck reading for more than 8 seconds,
    //and the dog bites and resets the system
  }
}


//the AT command to return phone number
//returns it in this format:
//
//+CNUM: "","13714281494",129,7,4

//this function pulls the meat of the number from that line and returns the String
//13714281494  
String getPhoneNumber()
{
  flushBuffer();

  GSM.println("AT+CNUM");
  watchdogDelay(500);
  readLine();
  readLine();  //skip the first blank line
  String line=readLine();
  unsigned char firstComma=line.indexOf(',');
  unsigned char secondComma=line.indexOf(',', firstComma+1);
  line=line.substring(firstComma+2, secondComma-1);  //chop the number out from between the first and second commas, and cut the quotation marks out, while we're at it
  return line;
}


void getSpray()
{
  String url="http://valve.tinypipes.net/getSpray/"+getPhoneNumber()+"/";
  Serial.println(url);
  submitHttpRequest(url);
  char data[150];
  getEEPROMdata(data, sizeof(data));
  trim(data, strlen(data));
  Serial<<data<<endl;
  if(strcmp(data, "None")!=0)  // only do this if there is a spray to send out
  {
    //parse out the number and content
    unsigned char commaIndex=getIndex(data, ',');
    char to[22];
    substring(data, to, getIndex(data, ':')+1, commaIndex);
    substring(data, data, getIndex(data, ':', commaIndex)+1, strlen(data));
    trim(to, strlen(to));
    trim(data, strlen(data));
    Serial<<"to:  "<<to<<"\n";
    Serial<<"content:  "<<data<<"\n";
    //send the SMS
    sendSMS(to, data);  
  }
}

void sendSMS(char* number, char* message)
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

void setupHttp()
{
  GSM.println("AT+CGATT?");
  watchdogDelay(500);

  showSerialData();

  GSM.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");//setting the SAPBR, the connection type is using gprs
  watchdogDelay(1000);

  showSerialData();

  GSM.println("AT+SAPBR=3,1,\"APN\",\"CMNET\"");//setting the APN, the second need you fill in your local apn server
  //GSM.println("AT+SAPBR=3,1,\"APN\",\"hkcsl\"");//setting the APN, the second need you fill in your local apn server
  //GSM.println("AT+SAPBR=3,1,\"APN\",\"pccw\"");//setting the APN, the second need you fill in your local apn server
  watchdogDelay(4000);

  showSerialData();

  GSM.println("AT+SAPBR=1,1");//setting the SAPBR, for detail you can refer to the AT command mamual
  watchdogDelay(2000);

  showSerialData(); 
  GSM.println("AT+HTTPINIT"); //init the HTTP request

  watchdogDelay(500); 
  showSerialData();
}

///this function is submit a http GET request
///attention:the time of delay is very important, it must be set enough 
int submitHttpRequest(String url)
{
  if(!httpSetup)
  {
    setupHttp();
    httpSetup=true;
  }

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
  delay(2000);
  unsigned char i;
  for(i=0;i<3;i++)
    Serial<<"Line "<<i<<" :  "<<readLine()<<"\n";
  int dataLength=saveModuleOutput();  
  Serial<<dataLength<<endl;
  flushBuffer();
  GSM.println("");
  delay(500);
  return dataLength;
}


void setupTcp()
{
  GSM.println("AT+CSTT=\"CMNET\"");//start task and setting the APN,
  watchdogDelay(1000);

  showSerialData();

  GSM.println("AT+CIICR");//bring up wireless connection
  watchdogDelay(3000);

  showSerialData();

  GSM.println("AT+CIFSR");//get local IP adress
  watchdogDelay(2000);

  showSerialData();
}

void tcpPOSTRequest(char * host, char * url, int length)
{

#ifdef DEBUG
  Serial<<"initializing POST request\n";
#endif

  if(!tcpSetup)
  {
    setupTcp();
    tcpSetup=true;
  }

  GSM.println("AT+CIFSR");  //get local IP address
  watchdogDelay(500);
  showSerialData();

  sendATCommand("AT+CIPSPRT=1", "OK", 3000);

  char temp[255];
  sprintf(temp,"AT+CIPSTART=\"tcp\",\"%s\",\"80\"",host);
  GSM.println(temp);//start up the connection
  watchdogDelay(4000);

  showSerialData();

  sendATCommand("AT+CIPSEND", ">", 3000);
  sprintf(temp,"POST %s HTTP/1.1",url);
  GSM.println(temp);
  sprintf(temp,"Host:  %s" ,host);
  GSM.println(temp);
  GSM.println("User-Agent: bigBooty/1.0");
  GSM.println("Content-Type: application/x-www-form-urlencoded");
  GSM.print("Content-Length: ");
  GSM.println(length);
  GSM.println();
  sendEEPROMdata();

  GSM.println((char)26);//sending
  watchdogDelay(5000);//waitting for reply, important! the time is base on the condition of internet 
  GSM.println();

  showSerialData();

  sendATCommand("AT+CIPCLOSE", "CLOSE OK", 1000);
}


//deletes an SMS message from memory
void deleteMessage(int index)
{
  //  Serial<<"deleting message "<<index<<endl;
  GSM.print("AT+CMGD=");
  GSM.println(index);  

}

uint16_t getLength(char * buffer)
{
  uint16_t maxLength=200;
  uint16_t pointer=0;
  while((*(buffer+pointer)!=0)&&(pointer<maxLength))
    pointer++;
  return pointer;
}



//puts together the parameter string for a POST request and saves it to EEPROM (it was too unweildly to pass around as a parameter)
//returns the length of the string
uint16_t constructPostString(int index)
{
  flushBuffer();
  GSM.print("AT+CMGR=");
  GSM.print(index, DEC);
  GSM.print("\r\n");
  readLine();
  readLine();
  //  char messageContent[160];
  char temp[200];
  char from[18];
  char metadata[22];
  //  String temp=readLine();
  horribleReadline(temp,200);
  strcpy(metadata,strtok(temp, ","));  //throw out everything before the first line
  strcpy(from, strtok(NULL, ","));  //the second phone number is in between the commas.
  removeChar(from, '\"');
  strtok(NULL, ",");
  /*
  date=strtok(NULL, ",");
   time=strtok(NULL, ",");
   */
  if(strstr(metadata, "READ")!=NULL)  //check the metadata to see if this is a message or some weird message.  Like, ew!
  {
    boolean phoneNumber=false, fromNumber=false, message=false;
    uint16_t before;
    free(metadata);
    for(unsigned char a=0;a<sizeof(temp);a++)
      temp[a]=0;
    char scratch[22];
    uint16_t address=STRING_ADDRESS;
    address=saveString("Body=", address);
    before=address;
    horribleReadline(temp+getLength(temp),160);
    address=saveString(temp, address);
    if(address-before>1)
      message=true;
    address=saveString("&From=", address);
    getPhoneNumber().toCharArray(scratch,15);
    before=address;
    address=saveString(scratch, address);
    if(address-before>1)
      phoneNumber=true;
    address=saveString("&Origin=", address);
    before=address;
    address=saveString(from, address);
    if(address-before>1)
      fromNumber=true;
    if(phoneNumber && fromNumber && message)
      return address;
    else
      return false;
  }
  else
    return false;
}

void saveIndex(unsigned char index)
{
  EEPROM.write(MESSAGE_INDEX, index);
}

unsigned char readIndex()
{
  return EEPROM.read(MESSAGE_INDEX);
}
//forward the message up to the server in a POST request, then delete it
void postMessage(unsigned char index)
{

  //I'm gonna save the index to EEPROM, because dealing with this massive string handling is pretty much guaranteed to mess with RAM.
  saveIndex(index);
  Serial<<index<<endl;
  int postStringLength=constructPostString(index);
  if(postStringLength>0)
  {
    printEEPROMdata();
    tcpPOSTRequest("valve.tinypipes.net", "/addSquirt/", postStringLength);

    //pull the index value back out of EEPROM
    index=readIndex();
    deleteMessage(index);
  }
}



void checkMessages()
{
  flushBuffer();
#ifdef DEBUG
  Serial<<"checking messages...\n";
#endif

  GSM.println("AT+CMGL=\"ALL\"");
  watchdogDelay(500);
  boolean validMessage=false;
  char line[100];
  int lineNum=0;
  while((!validMessage)&&(GSM.available()>0))
  {
    horribleReadline(line, sizeof(line));
    Serial<<line<<"\n";
    if(strstr(line, "CMGL:")!=NULL)  //there's a new unread message in there
      validMessage=true;
  }
  if(validMessage)
  {
    //get the message index
    substring(line, line, getIndex(line, ':')+2, getIndex(line, ','));
    int index=atoi(line);
    postMessage(index);
  }
}


void showSerialData()
{
  while(GSM.available()>0)
  {
    Serial.write(GSM.read());
    wdt_reset();
  }
}

//thanks to Libelium for the function
int8_t sendATCommand(char* ATcommand, char* expected_answer1, unsigned int timeout){


  uint8_t x=0,  answer=0;
  char response[100];
  unsigned long previous;

  memset(response, '\0', 100);    // Initialize the string

  watchdogDelay(100);

  while( GSM.available() > 0) GSM.read();    // Clean the input buffer

  GSM.println(ATcommand);    // Send the AT command 


    x = 0;
  previous = millis();
  Serial<<ATcommand<<endl;
  // this loop waits for the answer
  do{
    wdt_reset();
    if(GSM.available() != 0){    
      response[x] = GSM.read();
      x++;
      // check if the desired answer is in the response of the module
      if (strstr(response, expected_answer1) != NULL)    
      {
        answer = 1;
      }
    }
    // Waits for the asnwer with time out
  }
  while((answer == 0) && ((millis() - previous) < timeout));    

  Serial<<response<<endl;
  return answer;
}

void loop()
{
  if(cycles%2==0)
  {
    checkSIM();
    getSpray();
    checkMessages();

  }
  watchdogDelay(1000);
  cycles++;
}



