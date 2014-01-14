#include <Arduino.h>

//volatile unsigned char adcmap[5] = {2,5,4,3,9};
//volatile unsigned char analog[15];
volatile unsigned char adchan=0;
//volatile uint32_t fil[5];

void adc_init(char prescaler){
  ADMUX |= (1<<REFS0)|(1<<ADLAR); //select ADC channel, rvcc as ref, and left-shift the data
  ADCSRA |= (prescaler&0x07); //set prescaler 3-bits -> (1,2,4,8,16,32,64,128)
  //ADCSRA |=(1<<ADATE); //AUTO TRIGGER ENABLE
  ADCSRA |=(1<<ADEN); //ENABLE ADC
  ADCSRA |=(1<<ADIE); //INTERUPT ENABLE
  DIDR0 = 0x3F; //disable all digital input
}

inline void adc_select(char channel){
  ADMUX &= 0xE0;
  ADMUX |= channel&0x07;
  ADCSRB &= 0xF7;
  ADCSRA |=(1<<ADSC);
}

unsigned char adc_channel(void){
  return ((ADMUX&0x07)|(ADCSRB&0x08));
}

inline void adc_start(void){
  ADCSRA |=(1<<ADSC); //start conversions
}

