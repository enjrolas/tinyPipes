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

ISR(INT0_vect)
{
  ttime=micros();
  if(wait==0)
  {
    ttime=ttime-ltrig;
  }
  ltrig=micros();
  wait=0;
}

void setup()
{
  EICRA|=0x02; //INT0 falling edge interrupt
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
  if(((millis()-stime)>20)||(millis()<stime))
  {
   wait=1;
   Serial.println(ttime/74, DEC);
   countr=0; 
  }
}
