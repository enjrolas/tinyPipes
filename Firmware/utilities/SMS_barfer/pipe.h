#include<EEPROM.h>

char * pipeVersion="v2.8";  //hardware/firmware version
boolean charging=false;
boolean firstBoot=false;
boolean verbose=false;
int signal=0;   //cellular signal strength (in dBm)

//EEPROM addresses
#define PERIOD 1        //the seconds in-between samples
#define CHARGE_MODE 2  //the EEPROM address where we keep track of what charge mode we're in
#define VERBOSE 3
#define VERSION 4      //where we store our 4-bit version string


void initPipe()
{
  readIndex();
  int a;
  for(int i=0;i<1000;i++)
    a+=EEPROM.read(i);
  if(a==0) //it's the first time we've booted up
    firstBoot=true;
  if(firstBoot)
    EEPROM.write(CHARGE_MODE,charging);
  else
    charging=EEPROM.read(CHARGE_MODE);
}

void setVersion()
{
  for(int i=0;i<3;i++)
    EEPROM.write(VERSION+i,pipeVersion[i]);
}

void setVerboseMode()
{
  verbose=true;
  EEPROM.write(VERBOSE, verbose);
}

void setQuietMode()
{
  verbose=false;
  EEPROM.write(VERBOSE, verbose);
}

void setCharging()
{
  charging=true;
  EEPROM.write(CHARGE_MODE,charging);  
}


void setDisconnected()
{
  charging=false;
  EEPROM.write(CHARGE_MODE,charging);  
}




