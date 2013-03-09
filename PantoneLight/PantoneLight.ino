#include <RGBConverter.h>
#include <TimerOne.h>
#define _useTimer1
#include <ShiftPWM.h>

/* Color Sensor Pins */
#define S0     6
#define S1     4
#define S2     7
#define S3     5
#define OUT    2

/* LED Pins  */
#define LEDROUGE   5
#define LEDVERT    6
#define LEDBLEU    3

int   g_count = 0;    // Count the frequecy
int   g_array[3];     // Store the RGB value
int   g_flag = 0;     // Filter of RGB queue
float g_SF[3];        // Save the RGB Scale factor

/* ShiftPWM Config */
const int ShiftPWM_latchPin=8;
const bool ShiftPWM_invertOutputs = false; 
const bool ShiftPWM_balanceLoad = false;
unsigned char maxBrightness = 255;
unsigned char pwmFrequency = 75;
int numRegisters = 2;
int numRGBleds = numRegisters*8/3;

// Init TSC230 and setting Frequency.
void TSC_Init()
{
  // Set output pin
  pinMode (LEDVERT,OUTPUT); 
  pinMode (LEDROUGE,OUTPUT);
  pinMode (LEDBLEU,OUTPUT); 
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(OUT, INPUT);
  
  // Output frequency scaling 2%
  digitalWrite(S0, LOW);  
  digitalWrite(S1, HIGH); 
}
 
// Select the filter color 
void TSC_FilterColor(int Level01, int Level02)
{
  if(Level01 != 0)
    Level01 = HIGH;
 
  if(Level02 != 0)
    Level02 = HIGH;
 
  digitalWrite(S2, Level01); 
  digitalWrite(S3, Level02); 
}
 
void TSC_Count()
{
  g_count ++ ;
}
 
void TSC_Callback()
{
  switch(g_flag)
  {
    case 0: 
         Serial.println("->WB Start");
         TSC_WB(LOW, LOW); //Filter without Red
         break;
    case 1:
         Serial.print("->Frequency R=");
         Serial.println(g_count);
         g_array[0] = g_count;
         TSC_WB(HIGH, HIGH); //Filter without Green
         break;
    case 2:
         Serial.print("->Frequency G=");
         Serial.println(g_count);
         g_array[1] = g_count;
         TSC_WB(LOW, HIGH); //Filter without Blue
         break;
 
    case 3:
         Serial.print("->Frequency B=");
         Serial.println(g_count);
         Serial.println("->WB End");
         g_array[2] = g_count;
         TSC_WB(HIGH, LOW); //Clear(no filter)   
         break;
   default:
         g_count = 0;
         break;
  }
}
 
void TSC_WB(int Level0, int Level1)      //White Balance
{
  g_count = 0;
  g_flag ++;
  TSC_FilterColor(Level0, Level1);
  Timer1.setPeriod(1000000);             // Set 1s period
}
 
void setup()
{
  TSC_Init();
  Serial.begin(9600);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(TSC_Callback);  
  attachInterrupt(0, TSC_Count, RISING);
  delay(5000);
  g_SF[0] = 255.0/ g_array[0];     //R Scale factor
  g_SF[1] = 255.0/ g_array[1];    //G Scale factor
  g_SF[2] = 255.0/ g_array[2];    //B Scale factor
  
  ShiftPWM.SetAmountOfRegisters(numRegisters);
  ShiftPWM.SetPinGrouping(1);
  ShiftPWM.Start(pwmFrequency,maxBrightness);
}
 
void loop()
{
   ShiftPWM.SetAll(0);
   g_flag = 0;
   int valR = int(g_array[0] * g_SF[0]);
   int valG = int(g_array[1] * g_SF[1]);
   int valB = int(g_array[2] * g_SF[2]);
   ledRVBpwm(valR,valG,valB);
   delay(5000);
}


void ledRVBpwm(int pwmRed, int pwmGreen, int pwmBlue) {
 if(pwmRed > 255)
   pwmRed = 255;
 if(pwmGreen > 255)
   pwmGreen = 255;
 if(pwmBlue > 255)
   pwmBlue = 255;
   
   // RGB to HSV conversion
   byte r = pwmRed;
   byte g = pwmGreen;
   byte b = pwmBlue;
   double hsv[3];
   byte rgb[3];
   RGBConverter color;
   color.rgbToHsv(r,g,b,hsv);
   
   // Saturate the color thanks to HSV
   color.hsvToRgb(hsv[0],1,1,rgb);
   ShiftPWM.SetAllRGB(rgb[0],rgb[1],rgb[2]);
}
