/// @dir uart_demo
/// Demo for the SC16IS740 UART, connected via I2C.
// 2009-10-01 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <JeeLib.h>

PortI2C i2cBus (1);
UartPlug uart (i2cBus, 0x4D);

// Payload data structure:
struct {
    float uartRead;  // UART read value   
    byte count = 0;  // counter     
} payload;

char InputString[20];
byte i = 0;

void setup () {
  Serial.begin(57600);
  Serial.println("\n[uart_send_value]");
  uart.begin(57600);
  // Initialize RFM12B as an 868Mhz module and Node 15 + Group 212:
  rf12_initialize(15, RF12_868MHZ, 212);   
}

void loop () {
    int u = uart.read();
    if (u >= 0)
    {
      //Serial.print((char) u);
      InputString[i] = (char) u;
      i++;
      if (u == 0xD) //CR
      {
        payload.uartRead = atof(InputString);
        //Serial.println(); 
        Serial.print(payload.uartRead);  
        Serial.println();   
        i = 0;     
        
        rf12_sendNow(0, &payload, sizeof payload);
        //rf12_sendWait(2);       
        payload.count++;  
        delay(100);           
      }       
    }
}
