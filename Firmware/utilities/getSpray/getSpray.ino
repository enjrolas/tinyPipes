#include <Streaming.h>
#include <SoftwareSerial.h>
#include "delay.h"
#include "string.h"
#include<stdlib.h>

//#define DEBUG 1
#define GSM_POWER 4
#define PANEL_EN 5
#define SIM_STATUS 6

boolean tcpSetup=false;
boolean httpSetup=false;
boolean panelConnected=false;

int cycles=0;
SoftwareSerial GSM(3,2);

void setup()
{
  Serial.begin(9600);
  GSM.begin(9600);
  pinMode(PANEL_EN, OUTPUT);
  pinMode(GSM_POWER, OUTPUT);
  pinMode(SIM_STATUS, INPUT);
  Serial.println("TinyPipes is booting up...");
  checkSIM();
  bootGSMModule();
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
  if(cycles%10==0)
  {
    checkSIM();
//    getSpray();
    checkMessages();
  }
  watchdogDelay(1000);
  cycles++;
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
//  GSM.println("AT+SAPBR=3,1,\"APN\",\"hkcsl\"");//setting the APN, the second need you fill in your local apn server
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
  delay(2000);
  unsigned char i;
  for(i=0;i<3;i++)
    Serial<<"Line "<<i<<" :  "<<readLine()<<"\n";
  String data=readLine();
  data.trim();
  Serial<<"your data:  "<<data<<"\n";
  flushBuffer();
  GSM.println("");
  delay(500);
  return data;
}

String httpPost(String url, String parameters)
{
  GSM.println("AT+CGATT?");
  watchdogDelay(500);
 
  showSerialData();
 
  GSM.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");//setting the SAPBR, the connection type is using gprs
  watchdogDelay(1000);
 
  showSerialData();
 
  GSM.println("AT+SAPBR=3,1,\"APN\",\"CMNET\"");//setting the APN, the second need you fill in your local apn server
  watchdogDelay(4000);
 
  showSerialData();
 
  GSM.println("AT+SAPBR=1,1");//setting the SAPBR, for detail you can refer to the AT command mamual
  watchdogDelay(2000);
 
  showSerialData();
 
  GSM.println("AT+HTTPINIT"); //init the HTTP request
 
  watchdogDelay(2000); 
  showSerialData();
 
  GSM.print("AT+HTTPPARA=\"URL\",\"");
  GSM.print(url);
  GSM.println("\"");// setting the httppara, the second parameter is the website you want to access
  watchdogDelay(1000);
 
  showSerialData();
 
  GSM.println("AT+HTTPACTION=0");//submit the request 
  watchdogDelay(10000);//the delay is very important, the delay time is base on the return from the website, if the return datas are very large, the time required longer.
  //while(!GSM.available());
 
  showSerialData();
 
  GSM.println("AT+HTTPREAD");// read the data from the website you access
  delay(500);
  
  showSerialData();
  
  GSM.println("");
  delay(500);
  
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

void tcpPOSTRequest(char * host, char * url, char * content)
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
  GSM.println(strlen(content));
  GSM.println();
  GSM.println(content);

  GSM.println((char)26);//sending
  watchdogDelay(5000);//waitting for reply, important! the time is base on the condition of internet 
  GSM.println();

  showSerialData();

  sendATCommand("AT+CIPCLOSE", "CLOSE OK", 1000);
}


//deletes an SMS message from memory
void deleteMessage(int index)
{
  GSM.print("AT+CMGD=");
  GSM.println(index);  

}

//forward the message up to the server in a POST request, then delete it
void postMessage(int index)
{
  flushBuffer();
  GSM.print("AT+CMGR=");
  GSM.print(index, DEC);
  GSM.print("\r\n");
  String from;
   String date, time;
  readLine();
  readLine();
  char messageContent[160];
  char firstLine[160];
  horribleReadline(firstLine, sizeof(firstLine));
  horribleReadline(messageContent, sizeof(messageContent));

  Serial<<endl<<strlen(firstLine)<<endl<<firstLine<<endl;
  strtok(firstLine, ",");  //throw out everything before the first line
  Serial<<firstLine<<endl;
  from=strtok(NULL, ",");  //the second phone number is in between the commas.
  /*
  strtok(NULL, ",");
  date=strtok(NULL, ",");
  time=strtok(NULL, ",");
  */
//  Serial<<firstLine<<endl;
  Serial<<from<<endl;
  #ifdef DEBUG
  Serial<<"Message metadata: "<<firstLine<<endl;
  Serial<<"Message content: "<<messageContent<<endl;
  Serial<<"Time Stamp: "<<time<<" "<<date<<endl;
  Serial<<"From Number: "<<from<<endl;
  #endif
  Serial<<"got this far"<<endl;
  if(strstr(firstLine, "READ")!=NULL)  //check the metadata to see if this is a message or some weird message.  Like, ew!
  {
    char * content;  //160 chars for the text, 20 chars for both phone numbers, 20 chars for the variable names
    sprintf(content, "From=%s&Origin=+%s&Body=%s", getPhoneNumber(), from, messageContent);
    Serial<<"content string:  "<<content<<endl;
    
//    if((strcmp(from, "")!=0)&&(strcmp(messageContent, "")!=0))  //only post the content and delete the message if we parsed it correctly.
//    {
      tcpPOSTRequest("valve.tinypipes.net", "/addSquirt/", content);
      deleteMessage(index);
//    }
  }
}

uint16_t horribleReadline (char *buffer, uint16_t limit)
{
	char c;
	uint16_t ptr = 0;

	while (1) {

		if (GSM.available()) {

			c =GSM.read();

			if (c == 0x0D) { // cr == end of line
				*(buffer + ptr +1) = 0; // mark end of line
				break; // return char count
			}

				if (ptr < (limit - 1)) { // if not at the end of the line

					Serial.print ( (char) c); // echo char to user
					*(buffer + ptr++) = c; // put char into buffer

				} else { // at end of line, can't add any more
					Serial.print ( (char) 0x07); // ascii bel (beep) if terminal supports it
				}
		}
	}

	return ptr;
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
  String line;
  int lineNum=0;
  while((!validMessage)&&(GSM.available()>0))
  {
    line=readLine();
    Serial<<line<<"\n";
    if(line.indexOf("CMGL:")>=0)  //there's a new unread message in there
      validMessage=true;
  }
  if(validMessage)
  {
    char temp[10];
    //get the message index
    line=line.substring(line.indexOf(':')+2, line.indexOf(','));
    line.toCharArray(temp, 10);
    int index=atoi(temp);
    postMessage(index);
  }
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
