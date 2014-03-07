#include <EEPROM.h>

#define STRING_ADDRESS 1
#define MESSAGE_INDEX 0

void sendEEPROMdata()
{
  uint16_t address=STRING_ADDRESS;
  char data;
  Serial.println("sending EEPROM string");
  do{
    data=EEPROM.read(address);
    GSM.print(data);
    Serial.print(data);
    address++;
  }
  while(data!=0);
  GSM.println();
}

void getEEPROMdata(char * buffer, uint16_t maxLength)
{
  uint16_t pointer=0;
  char data;
  do{
    data=EEPROM.read(STRING_ADDRESS+pointer);
    *(buffer+pointer)=data;
     pointer++;
  }
  while((data!=0)&&(pointer<maxLength));
  if(pointer==maxLength)
    *(buffer+pointer-1)=0;  //check to null-terminate the string    
}

void printEEPROMdata()
{
  uint16_t address=STRING_ADDRESS;
  char data;
  do{
    data=EEPROM.read(address);
    Serial.print(data);
    address++;
  }
  while(data!=0);
}

uint16_t saveString(char * buffer, uint16_t address)
{
  uint16_t pointer=0;
  while(*(buffer+pointer)!=0)
  {
    EEPROM.write(address+pointer, *(buffer+pointer));
    pointer++;
  }
  EEPROM.write(address+pointer, 0); //null terminate that string!
  return address+pointer;
}

//saves a line from the GSM module to EEPROM.  Also trims leading whitespace
uint16_t saveModuleOutput()
{
  uint16_t address=STRING_ADDRESS;
  long timeout=2000;
  long start=millis();
  char data;
  do
  {
    if(GSM.available()>0)
    {
        data=GSM.read();
       EEPROM.write(address, data);
        address++;
      }
      wdt_reset();
  }
  while((data!='\n')&&((millis()-start)<timeout));
  EEPROM.write(address,0);  //be sure to null-terminate it

  return address-STRING_ADDRESS;  //return the length of the string
}

