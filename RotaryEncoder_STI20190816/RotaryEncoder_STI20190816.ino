/* read a rotary encoder with interrupts
   Encoder hooked up with common to GROUND,
   encoder0PinA to pin 3, encoder0PinB to pin 6 
   it doesn't matter which encoder pin you use for A or B

   uses Arduino pull-ups on A & B channel outputs
   turning on the pull-ups saves having to hook up resistors
   to the A & B channel outputs

*/

#define encoder0PinA   3
#define encoder0PinB   6
#define encoderPush    5

volatile unsigned int encoder0Pos = 0;
volatile unsigned int encoder0PosPrev = 0;

void setup() {
  pinMode(encoder0PinA, INPUT);
  //digitalWrite(encoder0PinA, HIGH);       // turn on pull-up resistor
  pinMode(encoder0PinB, INPUT);
  //digitalWrite(encoder0PinB, HIGH);       // turn on pull-up resistor
  pinMode(encoderPush, INPUT);

  attachInterrupt(1, doEncoder, FALLING);  // encoder pin on interrupt 1 - pin 3
  // attachInterrupt(1, doEncoder, CHANGE);  // encoder pin on interrupt 1 - pin 3
  Serial.begin (57600);
  Serial.println("start");                // a personal quirk
}

void loop() {
  // do some stuff here - the joy of interrupts is that they take care of themselves
  if (!digitalRead(encoderPush)) encoder0Pos = 0;   // encoder click to reset position

  if (encoder0Pos != encoder0PosPrev)  {
    Serial.println (encoder0Pos, DEC);
    encoder0PosPrev = encoder0Pos;
  }
}

void doEncoder() {
  /* If pinA and pinB are both high or both low, it is spinning
     forward. If they're different, it's going backward.

     For more information on speeding up this process, see
     [Reference/PortManipulation], specifically the PIND register.
  */
  if (digitalRead(encoder0PinA) == digitalRead(encoder0PinB)) {
    encoder0Pos++;
  } else {
    encoder0Pos--;
  }
}
