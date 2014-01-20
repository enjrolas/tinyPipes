#include <avr/interrupt.h>

#define R 5
#define G 6
#define B 7
#define rcv 2

unsigned long stime=0;
int etime=0;
int count=0;
char state=0;
char nstate=0;
volatile unsigned long ttime=0;
volatile unsigned long ltrig=0;
volatile char wait=0;
volatile int countr=0;
int x=0;

ISR(INT0_vect)
{
  if(wait==0)
  {
    ttime=micros()-ltrig;
    wait=2;
  }
  if(wait==1)
  {
    wait=0;
  }
  ltrig=micros();
}

void setup()
{
  EICRA|=0x03; //INT0 falling edge interrupt
  EIMSK|=0x01; //Enable INTO interrupts
  Serial.begin(9600);
  pinMode(rcv, INPUT);
  stime=millis();
  ltrig=micros();
  sei();
}

void loop()
{
 /* nstate=digitalRead(rcv);
  if(state!=nstate)
  {
    state=nstate;
    if(wait==1)
    {
      stime=millis();
      wait=0;
    }
    else
    {
      if(state==0)count++;
    } 
  }
  */
  if(wait==1)stime=millis();
  if(((millis()-stime)>60)||(millis()<stime))
  {
   wait=1;
   int data=(ttime+100)/200;
   if(data==1)
   {
     x=1;  
   }
   if(data==x)
   {
     Serial.println(data, DEC);
   } else {
     Serial.print(data, DEC);
     Serial.println(" Nope");
   }
   if(x==256)x=0;
   x++;
  // Serial.println((ttime+60)/100, DEC); //(ttime+50)/100
   countr=0; 
  }
}
