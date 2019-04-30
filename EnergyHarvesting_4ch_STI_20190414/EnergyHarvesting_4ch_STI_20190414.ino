/// @dir analog_demo
/// Demo readout of the 4-channel 18-bit MCP3424 on the Analog Plug v2.
/// @see http://jeelabs.org/2012/04/13/analog-plug-readout/
// 2009-09-28 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// 2019-04-06 <isteiner> corrected for negative values 
// 2019-04-14 <isteiner> EnergyHarvesting_4ch_STI_20190414, monitoring LTC3331 energy harvesting IC
// 2019-04-21 <isteiner> connect PGVOUT and EHOUT 
// 2019-04-27 <isteiner> sleep and SERIAL

#include <JeeLib.h>
#include "Ports.h"

#define SERIAL  0   // set to 1 to also report readings on the serial port

PortI2C myI2C (1);
AnalogPlug adc (myI2C, 0x68);

Port two (2);
Port three (3);
Port four (4);

// this must be added since we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// Payload data structure:
struct {
    int AD_Channel[4];  // mVolts    
    byte DIO;           // digital inputs     
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
    delay(300);   // 270 ms = 3.75 readings/sec max for 18-bit resolution
    dev.receive();  
    long raw = (long) dev.read(0) << 16;
    raw |= (word) dev.read(0) << 8;     
    raw |= dev.read(0);
    if (raw > 0x01ffff) raw = -((~raw & 0x1ffff)+1);  // calculate negative value for 18 bit values
 
    byte status = adc.read(1);
    return raw;
}

static void serialFlush () {
    #if ARDUINO >= 100
        Serial.flush();
    #endif  
    delay(10); // make sure tx buf is empty before going back to sleep
}

void setup () {
/*
#if SERIAL
    Serial.begin(57600);
    Serial.print("\n[EnergyHarvesting_4ch_STI_20190414]");  
    if (adc.isPresent()) 
      Serial.println("Analog Plug found");    
    serialFlush();  
#endif */
      
    cli();                    //cli clears the global interrupt flag in SREG so prevent any form of interrupt occurring. 
    // sets prescaler
    CLKPR = bit(CLKPCE);
#if defined(__AVR_ATtiny84__)
    CLKPR = 0; // div 1, i.e. speed up to 8 MHz
#else
    CLKPR = 1; // div 2, i.e. slow down to 8 MHz
    #if SERIAL
    Serial.begin(115200);   // actually is 57600 because of prescaler (div 2)
    Serial.print("\n[EnergyHarvesting_4ch_STI_20190414]");  
    if (adc.isPresent()) 
      Serial.println("Analog Plug found");    
    serialFlush();      
    #endif
#endif
    sei();                    //sei sets the bit and switches interrupts on

#if defined(__AVR_ATtiny84__)
    // power up the radio on JMv3
    bitSet(DDRB, 0);
    bitClear(PORTB, 0);
#endif  

    // Initialize RFM12B as an 868Mhz module and Node 14 + Group 212:
    rf12_initialize(14, RF12_868MHZ, 212); 
    rf12_control(0xC040); // set low-battery level to 2.2V i.s.o. 3.1V
    AP2init(adc);
    // initialize the PGVOUT and EHOUT pins as an input:
    two.mode(INPUT);    // PGVOUT on port 2
    three.mode(INPUT);  // EHOUT on port 3
    four.mode(INPUT);   // spare on port 4
}

void loop () {
    for (byte i=1; i <= 4; ++i) {
      long val = AP2read(adc, i);
      // calibrating values for CH1..CH4
      if (i == 1) val = val/13; // 0x1ffff/10020 = 13  CH1 = +- 10020 mV
      if (i == 2) val = val/24; // 0x1ffff/5660  = 23  CH2 = +- 5660 mV
      if (i == 3) val = val/24; // 0x1ffff/5660  = 23  CH3 = +- 5660 mV    
      if (i == 4) val = val/24; // 0x1ffff/5660  = 23  CH4 = +- 5660 mV      
      payload.AD_Channel[i-1] = (int)val; 
      #if SERIAL
        Serial.print(val);  
        Serial.print(' ');
      #endif      
    }

    uint8_t PGVOUT = two.digiRead();
    uint8_t EHOUT = three.digiRead();
    #if SERIAL    
      Serial.print(PGVOUT); 
      Serial.print(' '); 
      Serial.print(EHOUT);  
      Serial.print(' '); 
    #endif  
    
    //set - reset bit b0 = PGVOUT and b1 = EHOUT
    if (PGVOUT) payload.DIO |= 0x01;
    else payload.DIO &= 0xFE; 
    if (EHOUT) payload.DIO |= 0x02;
    else payload.DIO &= 0xFD; 
    #if SERIAL
      Serial.print(payload.DIO, BIN); 
      Serial.println();
      serialFlush();
    #endif  

    rf12_sendNow(0, &payload, sizeof payload);
    // set the sync mode to 2 if the fuses are still the Arduino default
    // mode 3 (full powerdown) can only be used with 258 CK startup fuses
    rf12_sendWait(2);
    
    rf12_sleep(RF12_SLEEP);
    Sleepy::loseSomeTime(60000);
    rf12_sleep(RF12_WAKEUP); 
}
