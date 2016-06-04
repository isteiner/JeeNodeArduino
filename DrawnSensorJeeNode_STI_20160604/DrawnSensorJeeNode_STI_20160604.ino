// Capacitive sensor for JeeNode
// Igor Steiner
// Date 4.6.2016

// Pin for the LED
int LEDPin = 7;
// Pin to connect to your drawing
int capSensePin1 = 4;
int capSensePin2 = 5;
int capSensePin3 = 6;
// This is how high the sensor needs to read in order
//  to trigger a touch.  You'll find this number
//  by trial and error, or you could take readings at 
//  the start of the program to dynamically calculate this.
int touchedCutoff = 30;

#include <RF12.h>
// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2

struct {
    byte capSensePin1;     // sensor 1
    byte capSensePin2;     // sensor 2
    byte capSensePin3;     // sensor 3
    byte counter;          // counter
} payload;

void setup(){
  Serial.begin(9600);
  // Set up the LED
  pinMode(LEDPin, OUTPUT);
  digitalWrite(LEDPin, LOW);
  rf12_initialize(6, RF12_868MHZ, 212); // 868 Mhz, net group 212, node 6  
}

void loop(){
  int CapValue1=0;
  int CapValue2=0;  
  int CapValue3=0;  
  
  for(int i = 0; i < 10; i++){
    CapValue1 = readCapacitivePin(capSensePin1) + CapValue1;   
    CapValue2 = readCapacitivePin(capSensePin2) + CapValue2;     
    CapValue3 = readCapacitivePin(capSensePin3) + CapValue3;       
  }   
  CapValue1=payload.capSensePin1=CapValue1/10;
  CapValue2=payload.capSensePin2=CapValue2/10;  
  CapValue3=payload.capSensePin3=CapValue3/10;    
  
  // If the capacitive sensor reads above a certain threshold,
  //  turn on the LED
  if (CapValue1 > touchedCutoff || CapValue2 > touchedCutoff || CapValue3 > touchedCutoff) {
    digitalWrite(LEDPin, HIGH);
  }
  else {
    digitalWrite(LEDPin, LOW);
  } 
  delay(100);
  Serial.print("Capacitive Sensor on Pin reads: ");
  Serial.print(CapValue1);
  Serial.print("  ");
  Serial.print(CapValue2);    
  Serial.print("  ");
  Serial.println(CapValue3);  
  payload.counter++;
      
  while (!rf12_canSend())
    rf12_recvDone();
  rf12_sendStart(0, &payload, sizeof payload);  //, RADIO_SYNC_MODE);        
}

// readCapacitivePin
//  Input: Arduino pin number
//  Output: A number, from 0 to 17 expressing
//          how much capacitance is on the pin
//  When you touch the pin, or whatever you have
//  attached to it, the number will get higher
//  In order for this to work now,
// The pin should have a 1+Megaohm resistor pulling
//  it up to +5v.
uint8_t readCapacitivePin(int pinToMeasure){
  // This is how you declare a variable which
  //  will hold the PORT, PIN, and DDR registers
  //  on an AVR
  volatile uint8_t* port;
  volatile uint8_t* ddr;
  volatile uint8_t* pin;
  // Here we translate the input pin number from
  //  Arduino pin number to the AVR PORT, PIN, DDR,
  //  and which bit of those registers we care about.
  byte bitmask;
  if ((pinToMeasure >= 0) && (pinToMeasure <= 7)){
    port = &PORTD;
    ddr = &DDRD;
    bitmask = 1 << pinToMeasure;
    pin = &PIND;
  }
  if ((pinToMeasure > 7) && (pinToMeasure <= 13)){
    port = &PORTB;
    ddr = &DDRB;
    bitmask = 1 << (pinToMeasure - 8);
    pin = &PINB;
  }
  if ((pinToMeasure > 13) && (pinToMeasure <= 19)){
    port = &PORTC;
    ddr = &DDRC;
    bitmask = 1 << (pinToMeasure - 13);
    pin = &PINC;
  }
  // Discharge the pin first by setting it low and output
  *port &= ~(bitmask);
  *ddr  |= bitmask;
  delay(1);
  // Make the pin an input WITHOUT the internal pull-up on
  *ddr &= ~(bitmask);
  // Now see how long the pin to get pulled up
  int cycles = 16000;
  for(int i = 0; i < cycles; i++){
    if (*pin & bitmask){
      cycles = i;
      break;
    }
  }
  // Discharge the pin again by setting it low and output
  //  It's important to leave the pins low if you want to 
  //  be able to touch more than 1 sensor at a time - if
  //  the sensor is left pulled high, when you touch
  //  two sensors, your body will transfer the charge between
  //  sensors.
  *port &= ~(bitmask);
  *ddr  |= bitmask;
  
  return cycles;
}
