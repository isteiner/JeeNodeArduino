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

#include <Ports.h>
#include <RF12.h>

// Use the watchdog to wake the processor from sleep:
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

Port tplug (1);

// Send a single unsigned long:

struct {
    int temp;   // temperature: (tenths)
    byte count;  // counter
} payload;

void setup()
{
  // Serial output at 57600 baud:
  Serial.begin(57600);
 
  // Initialize RFM12B as an 868Mhz module and Node 22 + Group 212:
  rf12_initialize(22, RF12_868MHZ, 212); 
}


void loop()
{

  Serial.println("Going to sleep...");

  // Need to flush the serial before we put the ATMega to sleep, otherwise it
  // will get shutdown before it's finished sending:
  Serial.flush();
  delay(5);
    
  // Power down radio:
  rf12_sleep(RF12_SLEEP);
  
  // Sleep for 15s:
  Sleepy::loseSomeTime(15000);
  
  // Power back up radio:
  rf12_sleep(RF12_WAKEUP);
   
  Serial.println("Woke up...");
  Serial.flush();
  delay(5);
  
  // Wait until we can send:
  while(!rf12_canSend())                     
    rf12_recvDone();
  
  long t = 0;
  for (int i = 0; i < 100; i++)
    t = t + tplug.anaRead();
  t = t/100;
  Serial.println(t,DEC);
  
  payload.temp = map((int)t, 0, 1024, 0, 3300); // 10 mv/C 

  payload.count++;
  
   // Send:
  rf12_sendStart(1, &payload, sizeof payload);
  rf12_sendWait(2);
  
  Serial.print("Sent ");
  Serial.println(payload.temp,DEC);
}
