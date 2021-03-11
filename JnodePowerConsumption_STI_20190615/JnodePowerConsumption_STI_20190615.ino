/*
Test the current consumption with JeeNode SMD v1 with different logics and regimes

  15.06.2019 First version
  by I.Steiner
*/

int i;
#include <JeeLib.h>
void setup() {
  pinMode(9, OUTPUT);   // build-in LED on PB1 = Arduino pin 9
  Serial.begin(57600);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(9, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);              // wait for a second: 1 = 1ms

  digitalWrite(9, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);              // wait for some seconds

  i++;
  Serial.flush();
  delay(100); // make sure tx buf is empty before going back to sleep  
  Serial.println(i);  
//  Sleepy::loseSomeTime(10000);
//  digitalWrite(9, LOW);    // turn the LED off by making the voltage LOW
//  Sleepy::loseSomeTime(10000);
}
