/// @dir Gas Meter with Compass
/// Demo sketch for the Modern Device Compass Board.
/// @see http://jeelabs.org/2012/04/02/meet-the-heading-board-replacement/
// 2012-03-29 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
// Programmed by Igor Steiner
// Date:  01 November, 2012

#include <JeeLib.h>
#define HIGH_STATE     10    // define high state 
#define LOW_STATE      20    // define low state
#define ACK_LOW_STATE  30    // define ack low state

PortI2C myBus (1);
CompassBoard compass (myBus);

unsigned long gasCounter = 0;
unsigned char lowSignal = 0;
unsigned char state = HIGH_STATE;

void setup() {
    Serial.begin(57600);
    Serial.println("\n[Gas Meter with Compass]");
    state = ACK_LOW_STATE;
}

void loop() {
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
          if (lowSignal > 3)  {
            gasCounter++;
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
    
    Serial.print("compassValue gasCounter state ");
    Serial.print(compassValue);
    Serial.print(' ');
    Serial.print(gasCounter);
    Serial.print(' ');
    Serial.print(state);
    Serial.println();    
    
    delay(5000);
}
