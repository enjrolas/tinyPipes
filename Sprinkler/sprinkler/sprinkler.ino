#include <Streaming.h>
#include <SoftwareSerial.h>
#include "delay.h"
#include<stdlib.h>

#define PANEL_CURRENT 0
#define PANEL_VOLTAGE 1
#define BATTERY_VOLTAGE 2
#define GSM_POWER 4
#define PANEL_EN 5
#define SIM_STATUS 6

boolean httpSetup=false;
boolean tcpSetup=false;

int cycles=0;
SoftwareSerial GSM(3,2);

String names[3];
String values[3];

void setup()
{
  Serial.begin(9600);
  GSM.begin(9600);
  pinMode(PANEL_EN, OUTPUT);
  pinMode(GSM_POWER, OUTPUT);
  pinMode(SIM_STATUS, INPUT);
  Serial.println("TinyPipes is booting up...");
  setupWatchdog();
  checkSIM();
  bootGSMModule();
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
    watchdogDelay(5000);
  }
  GSM.println("AT+SAPBR =0,1");  //release the GPRS context -- this is probably only an issue in testing, but sometimes we're rebooting the chip without rebooting the module, and then we're unable to open the
  //GPRS context because it's already open
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
  flushBuffer();
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

void flushBuffer()
{
  Serial.println("flushing...");
  while(GSM.available()>0)
    Serial.write(GSM.read());
}

void loop()
{
  wdt_reset();
  Serial<<"cycles: "<<cycles<<"\n";
  if(cycles%60==0)
  {
    checkSIM();
    //heartbeat();
    getSpray();
  }
  watchdogDelay(1000);
  cycles++;
}

void readMessage(int index)
{
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
  Serial<<"Message metadata: "<<message[0]<<endl;
  Serial<<"Message content: "<<message[1]<<endl;
  Serial<<"Time Stamp: "<<timeStamp<<endl;
  Serial<<"From Number: "<<from<<endl;

  message[1].trim();

  char **words;
  size_t len=0;
  char temp[160];
  message[1].toCharArray(temp, 160);
  words = split(temp, " ", &len);
  
  Serial.print("length: ");
  Serial.println(len);

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
      charging=true;
    if(message[1].equalsIgnoreCase("disconnect"))
      charging=false;
    if(message[1].equalsIgnoreCase("status"))
    {
      Serial.println("generating status");
      sendSMS(from, generateStatusMessage());
    }
    if(message[1].equalsIgnoreCase("instantaneous"))
      sendSMS(from, instantaneousMeasurement());
    if(message[1].indexOf("load")>=0)
      load(message[1].substring(4, message[1].length()));  //it comes in the format "load1510XXXXXXXXXXXXXX", so slice out the "load" part and sent it on
  }

  free4split(words);
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

void setupHttp()
{
  GSM.println("AT+CGATT?");
  watchdogDelay(500);

  showSerialData();

  GSM.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");//setting the SAPBR, the connection type is using gprs
  watchdogDelay(1000);

  showSerialData();

  GSM.println("AT+SAPBR=3,1,\"APN\",\"CMNET\"");//setting the APN, the second need you fill in your local apn server

  //GSM.println("AT+SAPBR=3,1,\"APN\",\"smartbro\"");//smartbro APN
  //  GSM.println("AT+SAPBR=3,1,\"APN\",\"hkcsl\"");//setting the APN, the second need you fill in your local apn server
  watchdogDelay(4000);

  showSerialData();

  while (sendATCommand("AT+SAPBR=1,1", "OK", 20000) == 0)
    watchdogDelay(5000);
  GSM.println("AT+HTTPINIT"); //init the HTTP request
 
  watchdogDelay(500); 
  showSerialData();

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
    String data=submitHttpRequest(url);
    if(!data.equals("None"))  // only do this if there is a spray to send out
    {
      //parse out the number and content
      unsigned char commaIndex=data.indexOf(',');
      String to=data.substring(data.indexOf(':')+1, commaIndex);
      String content=data.substring(data.indexOf(':', commaIndex)+1);
      Serial<<"to:  "<<to<<"\n";
      Serial<<"content:  "<<content<<"\n";
      //send the SMS
      sendSMS(to, content);  
    }
}


void heartbeat()
{
    String content="phoneNumber="+getPhoneNumber();
    tcpPOSTRequest("valve.tinypipes.net", "/heartbeat/", content);
}


void tcpPOSTRequest(char * host, char * url, String content)
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
  Serial<<content<<"\n";

  sendATCommand("AT+CIPSEND", ">", 3000);
  sprintf(temp,"POST %s HTTP/1.1",url);
  GSM.println(temp);
  sprintf(temp,"Host:  %s" ,host);
  GSM.println(temp);
  GSM.println("User-Agent: bigBooty/1.0");
  GSM.println("Content-Type: application/x-www-form-urlencoded");
  GSM.print("Content-Length: ");
  GSM.println(content.length());
  GSM.println();
  GSM.println(content);

  GSM.println((char)26);//sending
  watchdogDelay(5000);//waitting for reply, important! the time is base on the condition of internet 
  GSM.println();

  showSerialData();

  sendATCommand("AT+CIPCLOSE", "CLOSE OK", 1000);
}



void showSerialData()
{
  while(GSM.available()>0)
  {
    Serial.write(GSM.read());
    watchdogDelay(1);

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
  Serial<<"command: "<<ATcommand<<"\n";
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

  Serial<<"response: "<<response<<"\n";
  return answer;
}

///SubmitHttpRequest()
///this function is submit a http GET request
///attention:the time of delay is very important, it must be set enough 
String submitHttpRequest(String url)
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
  watchdogDelay(3000);//the delay is very important, the delay time is base on the return from the website, if the return datas are very large, the time required longer.
  //while(!GSM.available());
 
  showSerialData();
 
  GSM.println("AT+HTTPREAD");// read the data from the website you access
  delay(500);
  unsigned char i;
  for(i=0;i<3;i++)
    readLine();
  String data=readLine();
  data.trim();
  flushBuffer();
  GSM.println("");
  delay(500);
  return data;
}


