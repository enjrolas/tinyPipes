#include <avr/wdt.h>

#define WDT_TIME 4000
#define SIM_CHECK 15

void watchdogDelay(long Delay)
{
  while(Delay>WDT_TIME)
  {
    delay(WDT_TIME);
//    Serial.println("patting the dog");
    wdt_reset();
    Delay-=WDT_TIME;
  }
  delay(Delay);

//  Serial.println("patting the dog");
  wdt_reset();
}

void setupWatchdog()
{
  wdt_enable(WDTO_8S);
}
