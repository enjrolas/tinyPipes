#include <SoftwareSerial.h>
#include <Streaming.h>

SoftwareSerial GSM(3,2);
String line;

void setup()
{
  Serial.begin(19200);
  GSM.begin(19200);
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);
  delay(1000);
  digitalWrite(4, LOW);
}

void loop()
{
  if(Serial.available()>0)
  {
    char a=Serial.read();
    GSM.write(a);
    if(a==13)
    {
      unsigned char i;
      for(i=0;i<4;i++)
        Serial<<i<<" "<<readLine()<<endl;
    }
  }
  if(GSM.available()>0)
    {
      checkMessages();
      /*
      line=readLine();
      if(line.indexOf("CMTI")>=0)
        {
          unsigned char firstComma=line.indexOf(",");
          String num=line.substring(firstComma+1, line.length());
//          readMessage(num);
          
        } */
    }
    
}


String readLine()
{
  String line="";
  char a=' ';
  long timeout=1000;
  long start=millis();
  do
  {
    if(GSM.available()>0)
    {
      a=GSM.read();
      line+=a;
    }
  }
  while((a!=13)&&((millis()-start)<timeout));
  if((millis()-start)>timeout)
    Serial<<"string timed out.  here's what we've got so far:  "<<line<<endl;
  line.trim();
  return line;
}

void readMessage(int index)
{
  GSM.write("AT+CMGR=");
  GSM.write(index);
  GSM.write("\r\n");
  unsigned char i;
      for(i=0;i<4;i++)
        Serial<<i<<" "<<readLine()<<endl;
}

//deletes an SMS message from memory
void deleteMessage(int index)
{
  GSM.print("AT+CMGD=");
  GSM.println(index);  

}

void checkMessages()
{
  GSM.write("AT+CMGL=\"ALL\"\r\n");
  delay(1000);
  readLine();  //the first line is an echo
  boolean message=false;
  String line;
  int num=0;
  while((GSM.available()>0)&&(!message))
  {
    line=readLine();
    Serial.print(num, DEC);
    num++;
    Serial.print(": ");
    Serial.println(line);    
    if(line.indexOf("CMGL")>=0)  //if it's a message line
    {
      message=true;
      String index=line.substring(line.indexOf(":")+1, line.indexOf(","));
      char messageIndex[5];
      index.toCharArray(messageIndex, 5);
      Serial.println(atoi(messageIndex), DEC);
      readMessage(atoi(messageIndex));
      deleteMessage(atoi(messageIndex));
    }
  }
}

