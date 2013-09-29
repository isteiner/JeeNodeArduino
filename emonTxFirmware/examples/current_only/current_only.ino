// EmonLibrary examples openenergymonitor.org, Licence GNU GPL V3

#include "EmonLib.h"                   // Include Emon Library
EnergyMonitor emon1, emon2, emon3;                   // Create an instance

void setup()
{  
  Serial.begin(57600);
  
  emon1.currentTX(1, 111.1);             // Current: input pin, calibration.
  emon2.currentTX(2, 111.1);             // Current: input pin, calibration.
  emon3.currentTX(3, 111.1);             // Current: input pin, calibration.
}

void loop()
{
  double Irms1 = emon1.calcIrms(1480);  // Calculate Irms only
  double Irms2 = emon2.calcIrms(1480);  // Calculate Irms only
  double Irms3 = emon3.calcIrms(1480);  // Calculate Irms only
  
  Serial.print(" Power 1 = ");  
  Serial.print(Irms1*230.0);	       // Apparent power
  Serial.print(" Power 2 = ");  
  Serial.print(Irms2*230.0);	       // Apparent power
  Serial.print(" Power 3 = ");    
  Serial.print(Irms3*230.0);	       // Apparent power
  
  Serial.println(" ");
  Serial.print("Irms 1 = ");    
  Serial.print(Irms1);		       // Irms
  Serial.print(" Irms 2 = ");    
  Serial.print(Irms2);		       // Irms
  Serial.print(" Irms 3 = ");    
  Serial.print(Irms3);		       // Irms  
}
