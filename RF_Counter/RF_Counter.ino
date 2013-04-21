#include <JeeLib.h>

byte value=0;

void setup () {
  // this is node 15 in net group 212 on the 868 MHz band
  rf12_initialize(15, RF12_868MHZ, 100);
  Serial.begin(57600);
  Serial.println("\n[RF_Counter]");  
  // set output pin for TXENAB 
  pinMode(9, OUTPUT);  
}

void loop () {
  // increment counter
  value++;
  
  // Switch TXENAB to high
  digitalWrite(9, HIGH);
  delay(10);
  // actual packet send: broadcast to all, current counter, 1 byte long
  rf12_sendNow(0, &value, 1);
  delay(10);
  digitalWrite(9, LOW);
  
  Serial.println(value);
  
  // let one second pass before sending out another packet
  delay(1000);
}
