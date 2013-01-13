/***************************************************************************
 * Script to test wireless communication with the RFM12B tranceiver module
 * with an Arduino or Nanode board.
 *
 * Transmitter - Sends an incrementing number and flashes the LED every second.
 * Puts the ATMega and RFM12B to sleep between sends in case it's running on
 * battery.
 *
 * Ian Chilton <ian@chilton.me.uk>
 * December 2011
 *
 * Requires Arduino version 0022. v1.0 was just released a few days ago so
 * i'll need to update this to work with 1.0.
 *
 * Requires the Ports and RF12 libraries from Jeelabs in your libraries directory:
 *
 * http://jeelabs.org/pub/snapshots/Ports.zip
 * http://jeelabs.org/pub/snapshots/RF12.zip
 *
 * Information on the RF12 library - http://jeelabs.net/projects/11/wiki/RF12
 *
 ***************************************************************************/
// Programmed by Igor Steiner
// Date:  01 November, 2012

#include <JeeLib.h>
#define HIGH_STATE     10    // define high state 
#define LOW_STATE      20    // define low state
#define ACK_LOW_STATE  30    // define ack low state

PortI2C myBus (2);
CompassBoard compass (myBus);

unsigned char lowSignal = 0;
unsigned char state = HIGH_STATE;
int tick = 0;

// Use the watchdog to wake the processor from sleep:
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

Port tplug (1);

// Payload data structure:

struct {
    int temp;   // temperature: (tenths)
    unsigned long gasCounter;    // gas counter: (tenths) m3 
    byte count;  // counter
} payload;

void setup()
{
  // Serial output at 57600 baud:
  Serial.begin(57600);
 
  // Initialize RFM12B as an 868Mhz module and Node 22 + Group 212:
  rf12_initialize(22, RF12_868MHZ, 212); 
  
  // Initialize gas counter
  payload.gasCounter = 134864;
  
  Serial.println("\n[Gas Meter with Compass and Thermo couple]");
  state = ACK_LOW_STATE;  
}

void loop()
{

//  Serial.println("Going to sleep...");

  // Need to flush the serial before we put the ATMega to sleep, otherwise it
  // will get shutdown before it's finished sending:
  Serial.flush();
  delay(5);
    
  // Power down radio:
  rf12_sleep(RF12_SLEEP);
  
  // Sleep for 5s:
  Sleepy::loseSomeTime(5000);
  
  // Power back up radio:
  rf12_sleep(RF12_WAKEUP);
   
//  Serial.println("Woke up...");
  Serial.flush();
  delay(5);
  
  // Wait until we can send:
  while(!rf12_canSend())                     
    rf12_recvDone();
  
  long t = 0;
  for (int i = 0; i < 100; i++)
    t = t + tplug.anaRead();
  t = t/100;
//  Serial.println(t,DEC);
  
  payload.temp = map((int)t, 0, 1024, 0, 3300); // 10 mv/C 

  payload.count++;
//  Serial.println(payload.count);

    // gas counter
    int compassValue = compass.heading();
        
    switch (state) {
      case HIGH_STATE: {
        if (compassValue < -160) {
          lowSignal = 0;
          state = LOW_STATE;
        }
        break;
      }    
      
      case LOW_STATE: {
        if (compassValue > 0) state = HIGH_STATE;
        else if (compassValue < -160) {
          lowSignal++;
          if (lowSignal > 2)  {
            payload.gasCounter++;
            state = ACK_LOW_STATE;
          }
        }
        break;    
      }
    
      case ACK_LOW_STATE: {
        if (compassValue > 0) state = HIGH_STATE;
        break;
      } 

      default:  {
        state = HIGH_STATE;
        break;
      }
              
    }
    
//    Serial.print("compassValue gasCounter state ");
//    Serial.print(compassValue);
//    Serial.print(' ');
//    Serial.print(payload.gasCounter);
//    Serial.print(' ');
//    Serial.print(state);
//    Serial.println();    
    
   // Send every 5th tick
   tick++;
   if (tick >= 5) {
     rf12_sendStart(1, &payload, sizeof payload);
     rf12_sendWait(2);
     tick = 0;
   }
  
//  Serial.print("Sent ");
//  Serial.println(payload.temp,DEC);
}
