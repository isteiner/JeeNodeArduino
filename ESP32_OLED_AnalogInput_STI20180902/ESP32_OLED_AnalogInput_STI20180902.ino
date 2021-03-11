/**
 * Igor Steiner
 * Date: 2.9.2018
 */

// Include the correct display library
// For a connection via I2C using Wire include
// #include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, 5, 4); //5,4  21,22

int counter = 0;
/*
GPIO ADC Channels:
* GPIO 0 ==> ADC2_CH1
* GPIO 2 ==> ADC2_CH2
* GPIO 4 ==> ADC2_CH0
* GPIO 12 => ADC2_CH5
* GPIO 13 => ADC2_CH4
* GPIO 14 => ADC2_CH6
* GPIO 15 => ADC2_CH3
* GPIO 25 => ADC2_CH8
* GPIO 26 => ADC2_CH9
* GPIO 27 => ADC2_CH7
* GPIO 32 => ADC1_CH4
* GPIO 33 => ADC1_CH5
* GPIO 34 => ADC1_CH6
* GPIO 35 => ADC1_CH7
* GPIO 36 => ADC1_CH0
* GPIO 37 => ADC1_CH1
* GPIO 38 => ADC1_CH2
* GPIO 39 => ADC1_CH3 */
#define ANALOG_PIN_0 36
int analogVal0 = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("[ESP32_OLED_AnalogInput.001]");
  
  // Initialising the UI will init the display too.
  display.init();
  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
}

void loop() {
  // clear the display
  display.clear();
  //display.drawString(0, 0, "Counter");
  //display.drawString(90, 0, String(counter));

  // Analog input
  analogVal0 = analogRead(ANALOG_PIN_0); // read the value from GPIO 36 => ADC1_CH0
  
  Serial.println("analogVal0"); // print the value to the serial port 
  Serial.println(analogVal0);
  display.drawString(0, 20, "ADC1CH0");  
  display.drawString(0, 40, "mV");  
  display.drawString(90, 20, String(analogVal0));     
  display.drawString(90, 40, String(map(analogVal0,0,4095,0,3300)));
  display.display();
  dacWrite(25, analogVal0/16); //Analog write on GPIO 25 
    
  counter++;
  delay(500);
}
