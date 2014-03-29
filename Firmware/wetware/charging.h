#define FLOAT_VOLTAGE 14.4
#define CURRENT_HYSTERESIS .1
#define VOLTAGE_HYSTERESIS .1
#define SAMPLES 100

float averagePanelCurrent;

unsigned char chargeMode;
#define CONSTANT_VOLTAGE 0
#define CONSTANT_CURRENT 1

#define INCREMENT 0.1

#define SET_POINTS 8
const float voltagePoints[]={17, 16.75, 16.5, 16.25, 16, 15.5, 15, 14.5};
const float currentPoints[]={4,  3.5,  3,  2.5,  2,    1.5,  1,    0.5};

float panelPWM=50;
float increment;
unsigned char chargePoint=SET_POINTS-1;


unsigned char CV_cycles=0;
unsigned char CC_cycles=0;

