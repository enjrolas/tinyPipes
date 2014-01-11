#include <SoftwareSerial.h>

#define DEBUG 1
#define GSM_POWER 4

SoftwareSerial GSM(3,2);

void setup()
{
  Serial.begin(9600);
  GSM.begin(9600);
  pinMode(GSM_POWER, OUTPUT);

//  digitalWrite(GSM_POWER, HIGH);
//  delay(3000);
//  digitalWrite(GSM_POWER, LOW);
  Serial.println("TinyPipes is booting up...");
  delay(10000);
  String url="https://raw2.github.com/enjrolas/bootloadedHelloWorld/master/hello__world.cpp.hex";
  submitHttpRequest(url);
}

void loop()
{
}


///SubmitHttpRequest()
///this function is submit a http GET request
///attention:the time of delay is very important, it must be set enough 
void submitHttpRequest(String url)
{
  GSM.println("AT+CGATT?");
  delay(500);
 
  showSerialData();
 
  GSM.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");//setting the SAPBR, the connection type is using gprs
  delay(1000);
 
  showSerialData();
 
  GSM.println("AT+SAPBR=3,1,\"APN\",\"CMNET\"");//setting the APN, the second need you fill in your local apn server
  delay(4000);
 
  showSerialData();
 
  GSM.println("AT+SAPBR=1,1");//setting the SAPBR, for detail you can refer to the AT command mamual
  delay(2000);
 
  showSerialData();
 
  GSM.println("AT+HTTPINIT"); //init the HTTP request
 
  delay(2000); 
  showSerialData();
 
  GSM.print("AT+HTTPPARA=\"URL\",\"");
  GSM.print(url);
  GSM.println("\"");// setting the httppara, the second parameter is the website you want to access
  delay(1000);
 
  showSerialData();
 
  GSM.println("AT+HTTPACTION=0");//submit the request 
  delay(10000);//the delay is very important, the delay time is base on the return from the website, if the return datas are very large, the time required longer.
  //while(!GSM.available());
 
  showSerialData();
 
  GSM.println("AT+HTTPREAD");// read the data from the website you access
  delay(500);
 
  showSerialData();
 
  GSM.println("");
  delay(500);
}


void showSerialData()
{
  while(GSM.available()>0)
  {
    Serial.write(GSM.read());
    delay(1);
    
  }
}

