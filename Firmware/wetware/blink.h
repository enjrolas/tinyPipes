boolean blink =false; 
#define BLINK_PIN 8 

void startBlinking()
{
cli();//stop interrupts

//set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 2000;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

sei();//allow interrupts
}

void stopBlinking()
{
  TIMSK1 &= (0xFF ^ (1 << OCIE1A));  
  digitalWrite(BLINK_PIN, LOW);
}

ISR(TIMER1_COMPA_vect){//timer1 interrupt 1Hz toggles pin 13 (LED)
  blink=!blink;
  digitalWrite(BLINK_PIN, blink);
}
  
