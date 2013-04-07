#include <JeeLib.h>

byte value=0;

void setup () {
  // this is node 15 in net group 212 on the 868 MHz band
  rf12_initialize(15, RF12_868MHZ, 212);
  Serial.begin(57600);
  Serial.println("\n[RF_Counter]");  
}

void loop () {
  // increment counter
  value++;

  // actual packet send: broadcast to all, current counter, 1 byte long
  rf12_sendNow(0, &value, 1);
  
  Serial.println(value);
  
  // let one second pass before sending out another packet
  delay(1000);
}
