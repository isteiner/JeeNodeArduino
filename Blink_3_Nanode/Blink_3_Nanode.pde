/*
  Blink
  Turns on an LED on for 50 mili seconds, then off for 50o mili seconds, repeatedly.
 */

  // Pin 6 has an LED connected on Nanode boards
  // Pin 2 is external DIG2 - Red LED
  // Pin 3 is external DIG3 - Yellow LED
  // Pin 4 is external DIG4 - Green LED 
  
#define RED_LED    2
#define YELLOW_LED 3
#define GREEN_LED  4

void setup() {                
  // initialize the digital pin as an output.
  pinMode(RED_LED,    OUTPUT);     
  pinMode(YELLOW_LED, OUTPUT);   
  pinMode(GREEN_LED,  OUTPUT);     
  
  digitalWrite(RED_LED, HIGH);   // set the LED off (sink)
  digitalWrite(YELLOW_LED, HIGH);   // set the LED off (sink)
  digitalWrite(GREEN_LED, HIGH);   // set the LED off (sink) 
}

void loop() {
  digitalWrite(RED_LED, LOW);    // set the LED on (sink)
  delay(500);                    // wait for 500 mili seconds
  digitalWrite(RED_LED, HIGH);   // set the LED off (sink)
   
  digitalWrite(YELLOW_LED, LOW);   
  delay(500);                    
  digitalWrite(YELLOW_LED, HIGH);    
   
  digitalWrite(GREEN_LED, LOW);   
  delay(500);                    
  digitalWrite(GREEN_LED, HIGH);    
}
