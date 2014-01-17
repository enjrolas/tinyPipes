#define R 5
#define G 6 
#define B 7
#define nfet 18
#define pfet 17

unsigned char battData=0;

void pillowTalk(char data);
void clock(void);
void setup()
{
  pinMode(R,OUTPUT); //analogWrite(R,d)
  pinMode(G,OUTPUT); //analogWrite(G,d)
  pinMode(B,OUTPUT); //can't analogWrite(B,d)
  pinMode(nfet,OUTPUT);
  pinMode(pfet,OUTPUT);
  digitalWrite(nfet,LOW);
  digitalWrite(pfet,HIGH);
}

void loop()
{
  for(unsigned char y=0;y<255;y++)
  {
   pillowTalk(y);
   delay(30);
  } 
}

void pillowTalk(unsigned char data)
{
  clock();
  for(unsigned char x=0;x<data;x++)
  {
    delayMicroseconds(74);
  }
  clock();
}

void clock(void)
{
  digitalWrite(pfet,LOW);
  delayMicroseconds(7);
  digitalWrite(nfet,HIGH);
  delayMicroseconds(20);
  digitalWrite(nfet,LOW);
  delayMicroseconds(7);
  digitalWrite(pfet,HIGH);
  delayMicroseconds(20); 
}

