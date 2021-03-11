/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the Uno and
  Leonardo, it is attached to digital pin 13. If you're unsure what
  pin the on-board LED is connected to on your Arduino model, check
  the documentation at http://www.arduino.cc

  This example code is in the public domain.

  modified 8 May 2014
  by Scott Fitzgerald
 */

int i;
// the setup function runs once when you press reset or power the board
void setup() {
  //pinMode(9, OUTPUT);   // build-in LED on PB1 = Arduino pin 9
  Serial.begin(57600);
}

// the loop function runs over and over again forever
void loop() {
  //digitalWrite(9, HIGH);   // turn the LED on (HIGH is the voltage level)
  //delay(1000);              // wait for a second: 1 = 1ms
  //digitalWrite(9, LOW);    // turn the LED off by making the voltage LOW
  //delay(1000);              // wait for a second
  i++;
  Serial.println(i);  
}
