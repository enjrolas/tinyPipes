#include <RSA.h>

int led = 13;
int publicKey[2] = {14351, 11};

void setup()
{
  Serial.begin(115200);  
  pinMode(led, OUTPUT);
}
void loop()
{
  char cipher[CIPHERTEXT_SIZE];
  char plain[PLAINTEXT_SIZE];
  
  memset(cipher, 0, CIPHERTEXT_SIZE);
  memset(plain, 0, PLAINTEXT_SIZE);
  unsigned char index=0;
  
  while(!Serial.available());
  delay(50);
  while(Serial.available() > 0)
  {    
    cipher[index]=Serial.read();
    Serial.print(cipher[index]);
    index++;
  }
  
  unsigned char i=0;
  for(i=0;i<index;i++)
    Serial.print(cipher[i]+" ");
  
  rsa.decrypt(plain, cipher, publicKey);
  Serial.println(plain);
  
  if(strstr(plain, "enable") != NULL)
    digitalWrite(led, HIGH);
  else if(strstr(plain, "disable") != NULL)
    digitalWrite(led, LOW);
}
