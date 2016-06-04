/*
  For NANODE 5, I.Steiner, 15.5.2016
  Read 4 digital inputs on DIG3, DIG4, DIG5 and DIG7
  and blink LED on DIG6 - in build led
  and send digital input values to serial line for VVVV.
 */
// define digital inputs: DIG3..DIG7
#define DIG3 3
#define DIG4 4
#define DIG5 5
#define DIG7 7

// define digital output in-build led: LED6
#define LED6 6

void setup() {                
  // initialize the digital pin as an output and inputs.
  pinMode(LED6, OUTPUT);          // Pin 6 has an LED connected on most Arduino boards

  pinMode(DIG3, INPUT);           // set pin to input
  digitalWrite(DIG3, HIGH);       // turn on pullup resistors
  pinMode(DIG4, INPUT);           // set pin to input
  digitalWrite(DIG4, HIGH);       // turn on pullup resistors
  pinMode(DIG5, INPUT);           // set pin to input
  digitalWrite(DIG5, HIGH);       // turn on pullup resistors
  pinMode(DIG7, INPUT);           // set pin to input
  digitalWrite(DIG7, HIGH);       // turn on pullup resistors  
  // initialize serial communications at 57600 bps:
  Serial.begin(57600); 
}

int dig3val, dig4val, dig5val, dig7val;
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 500;           // interval at which to blink (milliseconds)
int ledState = LOW;               // ledState used to set the LED

void loop() {
  dig3val=digitalRead(DIG3);
  dig4val=digitalRead(DIG4);
  dig5val=digitalRead(DIG5);
  dig7val=digitalRead(DIG7);  
//  Serial.print(dig3val);
//  Serial.print(dig4val);
//  Serial.print(dig5val);
//  Serial.println(dig7val);
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    // set the LED with the ledState of the variable:
    digitalWrite(LED6, ledState); 
    // write values to serial output for VVVV
    Serial.print("<input1>");     //first button
    Serial.print(!dig3val);
    Serial.println("</input1>");  
    Serial.print("<input2>");     //second button
    Serial.print(!dig4val);
    Serial.println("</input2>");         
    Serial.print("<input3>");     //third button
    Serial.print(!dig5val);
    Serial.println("</input3>");   
    Serial.print("<input4>");     //fourth button
    Serial.print(!dig7val);
    Serial.println("</input4>");           
  }
}
