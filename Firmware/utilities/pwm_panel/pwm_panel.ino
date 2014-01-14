#include <Streaming.h>
#include <SoftwareSerial.h>
/*
#include "delay.h"
#include "samples.h"
#include "floatToString.h"
#include "stringHandling.h"
#include "charging.h"
*/
#define DEBUG 0

#define PANEL_EN 5
#define PANEL_CURRENT 0
#define PANEL_VOLTAGE 1
#define BATTERY_VOLTAGE 2
#define GSM_POWER 4
#define CHARGING 8 
#define ENABLED 9
#define LOAD_EN 8
//make a couple defs for charging/discharging status
#define CHARGING 0
#define DISCHARGING 1 


float panelCurrent, panelVoltage, batteryVoltage;
float wattSeconds, averageBatteryVoltage, averagePanelVoltage;
int index=0;
int samples=0;
int cycles=0;  //number of charge cycles
int m=0;

SoftwareSerial GSM(3,2);





void setup()
{
  Serial.begin(9600);
  GSM.begin(9600);
  pinMode(PANEL_EN, OUTPUT);
  pinMode(CHARGING, OUTPUT);
  pinMode(ENABLED, OUTPUT);
  pinMode(LOAD_EN, OUTPUT);
  pinMode(GSM_POWER, OUTPUT);
  analogWrite(PANEL_EN, 128);
  }

void loop(){
//analogWrite(PANEL_EN, 128);


}
