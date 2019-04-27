/// @dir analog_demo
/// Demo readout of the 4-channel 18-bit MCP3424 on the Analog Plug v2.
/// @see http://jeelabs.org/2012/04/13/analog-plug-readout/
// 2009-09-28 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// 2019-04-06 <isteiner> corrected for negative values 

#include <JeeLib.h>

PortI2C myI2C (1);
AnalogPlug adc (myI2C, 0x68);

// Payload data structure:
struct {
    int AD_Channel[4];           
} payload;

static void AP2init (DeviceI2C& dev, byte mode =0x1C) {
    // mode =0x1C  is channel 1, continuous, 18-bit, gain x1
    // mode =0x10  is channel 1, continuous, 12-bit, gain x1
    dev.send();
    dev.write(mode);
    dev.stop();
}

static long AP2read (DeviceI2C& dev, byte channel) {
    adc.select(channel);
    delay(300);
    dev.receive();  
    long raw = (long) dev.read(0) << 16;
    raw |= (word) dev.read(0) << 8;     
    raw |= dev.read(0);
    if (raw > 0x01ffff) raw = -((~raw & 0x1ffff)+1);  // calculate negative value for 18 bit values
 
    byte status = adc.read(1);
    return raw;
}

void setup () {
    Serial.begin(57600);
    Serial.print("\n[analog_demo]");
    if (adc.isPresent())
      Serial.println("Analog Plug found");    
    // Initialize RFM12B as an 868Mhz module and Node 14 + Group 212:
    rf12_initialize(14, RF12_868MHZ, 212); 
    AP2init(adc);
}

void loop () {
    for (byte i=1; i <= 4; ++i) {
      long val = AP2read(adc, i);
      val = val/64;
      payload.AD_Channel[i-1] = (int)val; 

      Serial.print(val);  
      Serial.print(' ');
      //delay(270); // 270 ms = 3.75 readings/sec max for 18-bit resolution
    }
    
    delay(2000);    
    rf12_sendStart(1, &payload, sizeof payload);
    rf12_sendWait(2);
    Serial.println();
}
