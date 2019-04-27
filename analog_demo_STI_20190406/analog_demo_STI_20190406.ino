/// @dir analog_demo
/// Demo readout of the 4-channel 18-bit MCP3424 on the Analog Plug v2.
/// @see http://jeelabs.org/2012/04/13/analog-plug-readout/
// 2009-09-28 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// 2019-04-06 <isteiner> corrected for negative values 

#include <JeeLib.h>

PortI2C myI2C (1);
DeviceI2C adc (myI2C, 0x68);

static void AP2init (DeviceI2C& dev, byte mode =0x1C) {
    // mode =0x1C  is channel 1, continuous, 18-bit, gain x1 (2048 mV)
    // mode =0x10  is channel 1, continuous, 12-bit, gain x1 (2048 mV)
    // mode =0x1f  is channel 1, continuous, 18-bit, gain x8 (256 mV)
    dev.send();
    dev.write(mode);
    dev.stop();
}

static long AP2read (DeviceI2C& dev) {
    dev.receive();  
    long raw = (long) dev.read(0) << 16;
    raw |= (word) dev.read(0) << 8;     
    raw |= dev.read(0);
    if (raw > 0x01ffff) {             // calculate negative value for 18 bit values
      raw =  -((~raw & 0x1ffff)+1);
      //Serial.println(raw, HEX);  
      //Serial.println(-raw, DEC);  
      //Serial.println(" ");                     
     }
    //else {                           // else positive value
    //  Serial.println(raw, HEX);  
    //  Serial.println(raw, DEC);  
    //  Serial.println(" ");        
    //}
    byte status = adc.read(1);
    return raw / 64;
}

void setup () {
    Serial.begin(57600);
    Serial.println("\n[analog_demo]");
    if (adc.isPresent())
      Serial.println("Analog Plug found");        
    rf12_config();
    rf12_easyInit(0);
    AP2init(adc);
}

void loop () {
    long val = AP2read(adc);
    
    rf12_easyPoll();
    rf12_easySend(&val, sizeof val);

    Serial.print("mV ");
    Serial.println(val);  

    delay(1000);
}
