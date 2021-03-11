// CO2 Senseair S8 LP using Software Serial
#include <SoftwareSerial.h>
#include <JeeLib.h>
/*
 Basic Arduino example for CO2 Senseair S8 LP sensor
 Created by Jason Berger
 Co2meter.com
 Modified by I. Steiner 2021-02-21
*/
#include "SoftwareSerial.h"
SoftwareSerial S8_Serial(3,4); //RX, TX Sets up a virtual serial port

// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2

// Payload data structure:
struct {
    int valCO2 = 0;       // CO2 value  
    byte counter = 0;     // test counter             
} payload;

byte readCO2[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25}; //Command packet to read Co2 (see app note)
byte response[] = {0,0,0,0,0,0,0}; //create an array to store the response
//multiplier for value. default is 1. set to 3 for K-30 3% and 10 for K-33 ICB
int valMultiplier = 1;

void sendRequest(byte packet[])
{
 while(!S8_Serial.available()) //keep sending request until we start to get a response
 {
    S8_Serial.write(readCO2,7);
    delay(50); 
 }
 int timeout=0; //set a timeout counter
 while(S8_Serial.available() < 7 ) //Wait to get a 7 byte response
 {
   timeout++;
   if(timeout > 10) //if it takes too long there was probably an error
   {
    while(S8_Serial.available()) //flush whatever we have
    S8_Serial.read();
    break; //exit and try again
   }
   delay(50);
 }
 for (int i=0; i < 7; i++)
 {
    response[i] = S8_Serial.read();
 }
}

unsigned long getValue(byte packet[])
{
  int high = packet[3]; //high byte for value is 4th byte in packet in the packet
  int low = packet[4]; //low byte for value is 5th byte in the packet
  unsigned long val = high*256 + low; //Combine high byte and low byte with this formula to get value
  return val* valMultiplier;
} 

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(57600); //Opens the main serial port to communicate with the computer
  S8_Serial.begin(9600); //Opens the virtual serial port with a baud of 9600
  Serial.println("CO2 Senseair S8 LP Software Serial");
  // Initialize RFM12B as an 868Mhz module and Node 20 + Group 212:
  rf12_initialize(20, RF12_868MHZ, 212);   
}
void loop()
{
  sendRequest(readCO2);
  unsigned long valCO2 = getValue(response);
  payload.valCO2 = (int)valCO2;
  payload.counter++;
  rf12_sendNow(0, &payload, sizeof payload);
  rf12_sendWait(RADIO_SYNC_MODE);  
  
  Serial.print("Co2 ppm = ");
  Serial.println(valCO2);  
  //delay(10000);
  rf12_sleep(RF12_SLEEP);
  //for (byte i=0; i < 10; ++i) Sleepy::loseSomeTime(60000);  // sleep for 60 sec * n
  delay(50000);
  rf12_sleep(RF12_WAKEUP);   
}
