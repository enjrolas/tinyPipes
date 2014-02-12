#include <Wire.h>         // Para a EEPROM 24LC256, assim chamamos as bibliotecas i2c
#define eeprom 0x50    // endereço da eeprom já shiftado
#define WP 9

void setup(void){
  pinMode(WP, OUTPUT);
  Wire.begin();           // Iniciar ligações i2c
  Serial.begin(9600); // Assim podemos ver qualquer coisa na consola serial

  unsigned int address = 0;  //endereçamento a 2 bytes
  Serial.println("A escrever o melhor numero de sempre nos 10 primeiros bytes!");
  digitalWrite(WP, LOW);
  for(address = 0; address<10; address++) writeEEPROM(eeprom, address, '3');   // Va pub a mim! Encher os 10 primeiros bytes com o número 3
  Serial.println("Vamos ler se esta tudo ok, deves ver 33, 33, 33, 33... Vá 10x 33");
  digitalWrite(WP, LOW);

  for(address = 0; address<10; address++) {
     Serial.print(readEEPROM(eeprom, address), HEX); 
     Serial.print(", ");
  }
}

void loop(){
}

//-------Rotinas para EEPROMS i2c por Daniel Gonçalves a.k.a. Tr3s------
// Podem usar estas rotinas à vontade para projectos particulares. 
// Para fins comerciais entrar em contacto com we_real_cool@hotmail.com
// Partilhem com apenas com o meu concentimento. 
// Se virem este código noutro sitio sem ser [url=http://www.lusorobotica.com]www.lusorobotica.com[/url] avisem de imediato para we_real_cool@hotmail.com!

void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data ) {
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();
}

byte readEEPROM(int deviceaddress, unsigned int eeaddress ) {
  byte rdata = 0xFF;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress,1);
  if (Wire.available()) rdata = Wire.read();
  return rdata;
}

// Por Daniel Gonçalves a.k.a. (t.c.p.) Tr3s, para [url=http://www.lusorobotica.com]www.lusorobotica.com[/url]
