#include "usart.h"
#include "commands.h"
#include "adc.h"

#define baud0 1  //500k baud rate

volatile unsigned char com = 0;
volatile unsigned char data[12]; // max 12 bytes of data per command
volatile char frame = 0;
volatile unsigned int sampCount = 0;

void setup(){
  DDRC=0x00;
  adchan=0;           //adc channel selection 
  adc_init(6);      //channel 2, div 64 clock prescaler
  adc_select(adchan);
  adc_start();

  usart0_init(baud0);
  
  sei();            // start interrupts

}

void loop() {

}

// ******************************
// * INTERRUPT SERVICE ROUTINES *
// ******************************

ISR(ADC_vect){               //ADC complete interrupt handler
  //analog[adcmap[adchan]]= ADCH;
  //fil[adchan] *= 15;
  //fil[adchan] += ((uint32_t) ADCH)<<20;  
  //fil[adchan] >>= 4;
  samples[adchan][sampCount] = ADCH;
  adchan++;
  if(adchan>=chanNum){
    adchan=0;
    sampCount++;
  }
  if(sampCount>sampNum)sampCount=0;
  adc_select(adchan);
}

ISR(USART_RX_vect){         //USART receive interrupt handler
  if (!frame){               //if incoming command
    com = UDR0;              //write rx buffer to command variable
    frame = commands[com];   //number of expected data bytes to follow
  } else                     //if incoming data
    data[--frame] = UDR0;    //write rx buffer to data array
  if(!frame)                 //if command is complete
    (*responses[com])(data); //run responder
}

