/*
 * MPP test
 This example shows how to measure MPP point of PV cell by using Hill Climbing MPPT techniques

 The circuit:
 Analog output on pin 3 to control variable load
 Analog input on pin A0 to measure current through variable load
 Analog input on pin A1 to measure voltage of PV
 
 Date: 17.8.2018
 By: Igor Steiner
 
 */
#include <JeeLib.h>
#define RADIO_SYNC_MODE 2
#define AOUT 3          // Analog Output pin for variable load
#define STEP 1          // step size for variable load
#define MINSTEP 60      // minimum step value
#define MAXSTEP 75      // max step value
#define BIAS 2          // bias value to automatic adapt variation cycle of detection MPP

// transmit payload
struct {
    int MPP_V;          // voltage in MPP point in hundreths of V
    int MPP_mA;         // current in MPP in mA  
    int MPP_mW;         // power in MPP in mW     
 } payload;
 
int fadeValue = MINSTEP;
int lowStep = MINSTEP;
int highStep = MAXSTEP;
byte increment = true;  
float MPP_V;        
float MPP_mW = 0;  
int MPP_step;
   
void setup() {
    // Initialize RFM12B as an 868Mhz module and Node 13 + Group 212:
    rf12_initialize(13, RF12_868MHZ, 212);     
    Serial.begin(57600);
    Serial.println("\n[MPP TEST l.0]"); 
    Serial.println("\nStep    mA    V    mW");    
}

void loop() {
  // fade in from min to max in increments of 5 points:
  // sets the value (range from 0 to 255):
  analogWrite(AOUT, fadeValue);
  Serial.print(fadeValue);    
  // wait for 2000 milliseconds to stabilize values
  delay(2000);  
  Serial.print("   ");  
  // Read analog input values (current, voltage) and calculate average value, result is mA and V
  float currentValue = 0; 
  float voltValue = 0;   
  float powerValue = 0;          
  
  for (int i=0; i<100; i++) 
  {
    currentValue = currentValue + analogRead(A0);
    voltValue = voltValue + analogRead(A1);
  }
  currentValue = currentValue/100;  
  voltValue = voltValue/7555;             //conversion to actual voltage value
  powerValue = currentValue * voltValue;  //power in mW 

  //check for MPP values
  if (powerValue > MPP_mW) 
  {
    MPP_step = fadeValue;
    MPP_mW = powerValue;
    MPP_V = voltValue;
    payload.MPP_V = MPP_V*100;
    payload.MPP_mW = MPP_mW;
    payload.MPP_mA = currentValue;
  }
    
  Serial.print(currentValue);  
  Serial.print("   ");  
  Serial.print(voltValue); 
  Serial.print("   ");  
  Serial.println(powerValue);   
     
  if (increment)fadeValue += STEP;
  else fadeValue -= STEP;

  if (fadeValue >= highStep || fadeValue >= MAXSTEP) 
  {
    Serial.print(" ****************************** MPP:  ");      
    Serial.print(MPP_V);  
    Serial.print(" V    ");  
    Serial.print(MPP_mW); 
    Serial.println(" mW  "); 
    rf12_sendNow(0, &payload, sizeof payload);   
    lowStep = MPP_step - BIAS;             
    MPP_V = 0;        
    MPP_mW = 0;      
    increment = false; 
  }
  if (fadeValue <= lowStep || fadeValue <= MINSTEP)
  {
    Serial.print(" ****************************** MPP:  ");      
    Serial.print(MPP_V);  
    Serial.print(" V    ");  
    Serial.print(MPP_mW); 
    Serial.println(" mW  "); 
    rf12_sendNow(0, &payload, sizeof payload);  
    highStep = MPP_step + BIAS;  
    MPP_V = 0;        
    MPP_mW = 0;          
    increment = true;    
  }
}
