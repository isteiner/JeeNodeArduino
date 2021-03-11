/*
 * Geiger counter Kit could get on:  https://www.aliexpress.com            search: geiger counter kit
* --------------------------------------------------------------------------------------
* WHAT IS CPM?
* CPM (or counts per minute) is events quantity from Geiger Tube you get during one minute. Usually it used to 
* calculate a radiation level. Different GM Tubes has different quantity of CPM for background. Some tubes can produce
* about 10-50 CPM for normal background, other GM Tube models produce 50-100 CPM or 0-5 CPM for same radiation level.
* Please refer your GM Tube datasheet for more information. Just for reference here, J305 and SBM-20 can generate 
* about 10-50 CPM for normal background. 
* --------------------------------------------------------------------------------------
* HOW TO CONNECT GEIGER KIT?
* The kit 3 wires that should be connected to Arduino UNO board: 5V, GND and INT. PullUp resistor is included on
* kit PCB. Connect INT wire to Digital Pin#2 (INT0), 5V to 5V, GND to GND. Then connect the Arduino with
* USB cable to the computer and upload this sketch. 
* 
 * Author:JiangJie Zhang * If you have any questions, please connect cajoetech@qq.com
 * 
 * License: MIT License
 * 
 * Please use freely with attribution. Thank you!
*/

#include <JeeLib.h>
#include <SPI.h>
#define LOG_PERIOD 60000  //Logging period in milliseconds, recommended value 15000-60000.
#define MAX_PERIOD 60000  //Maximum logging period without modifying this sketch = 1 minute

// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2

volatile unsigned long counts;     //variable for GM Tube events
//unsigned long cpm;        //variable for CPM
unsigned int multiplier;  //variable for calculation CPM in this sketch
unsigned long previousMillis;  //variable for time measurement

// Payload data structure:
struct {
    int cpm = 0;          // Geiger-Muller counter - counts per minute  
    byte counter = 0;     // test counter             
} payload;


void tube_impulse(){       //interrupt subprocedure for capturing events from Geiger counter
  counts++;
}

void setup(){             //setup subprocedure
  counts = 0;
  multiplier = MAX_PERIOD / LOG_PERIOD;      //calculating multiplier, depend on your log period
  Serial.begin(57600);
  attachInterrupt(1, tube_impulse, FALLING); //define external interrupts IRQ on JeeNode = INT1 (it's Arduino pin 3)

  // Initialize RFM12B as an 868Mhz module and Node 17 + Group 212:
  rf12_initialize(17, RF12_868MHZ, 212); 
}

void loop(){                                 //main cycle
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > LOG_PERIOD){
    previousMillis = currentMillis;
    payload.cpm = counts * multiplier;
    counts = 0;
    payload.counter++;    
      
    rf12_sendNow(0, &payload, sizeof payload);
    rf12_sendWait(RADIO_SYNC_MODE);
    Serial.println(payload.cpm);   
  }
  
}
