/// @dir analog_demo
/// Demo readout of the 4-channel 18-bit MCP3424 on the Analog Plug v2.
/// @see http://jeelabs.org/2012/04/13/analog-plug-readout/
// 2009-09-28 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// 2019-04-06 <isteiner> corrected for negative values 
// 2019-04-14 <isteiner> EnergyHarvesting_4ch_STI_20190414, monitoring LTC3331 energy harvesting IC
// 2019-04-21 <isteiner> connect PGVOUT and EHOUT 
// 2019-04-27 <isteiner> sleep and SERIAL
// 2019-05-04 <isteiner> discharge battery, temperature of DS18B20 and payload changed 

#include <JeeLib.h>
#include "Ports.h"
#include <OneWire.h>
#include <DallasTemperature.h>
// Data wire is plugged into  port 7 on the Arduino -> PORT 4 on JeeNode
#define ONE_WIRE_BUS 7

#define SERIAL  0     // set to 1 to also report readings on the serial port
#define SWITCH_LOAD 0 // set to 1 to manage discharging of battery
 
PortI2C myI2C (1);
AnalogPlug adc (myI2C, 0x68);

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

Port two (2);
Port three (3);
//Port four (4);

// this must be added since we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// Payload data structure:
struct {
    int AD_Channel[4];  // mVolts   
    int temperature;    // internal tempearture  
    //byte DIO;           // digital inputs    
    byte counter = 0; 
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
    //four.mode(INPUT);   // spare on port 4 
    
    // Start up the OneWire library
    sensors.begin();
    
    #if SWITCH_LOAD   //discarge battery if this option is selected
        pinMode(9, OUTPUT);   // build-in LED on PB1 = Arduino pin 9
        digitalWrite(9, HIGH);  // switch LED off (invert/pullup)                     
    #endif     
}

void loop () {
    for (byte i=1; i <= 4; ++i) {
      long val = AP2read(adc, i);
      // calibrating values for CH1..CH4
      if (i == 1) val = (float)val/12.630; // 58100/4600 = 12.630 calib.at 4.6V  CH1 = +- 10000 mV  AC1
      if (i == 2) val = (float)val/12.500; // 50000/4000 = 12.500 calib.at 4.0V  CH2 = +- 10000 mV  VBAT
      if (i == 3) val = (float)val/12.303; // 40600/3300 = 12.303 calib.at 3.3V  CH3 = +- 10000 mV  VOUT    
      if (i == 4) val = (float)val/64.400; // 64400/1000 = 64.400 calib.at 1.0V  CH4 = +- 2048  mV  ICHARGE   
      payload.AD_Channel[i-1] = (int)val; 
      #if SERIAL
        Serial.print(val);  
        Serial.print(' ');
      #endif      
    }

    sensors.requestTemperatures(); // Send the command to get temperatures
    // We use the function ByIndex, and as an example get the temperature from the first sensor only.
    payload.temperature = (int)(sensors.getTempCByIndex(0)*10); 
    #if SERIAL
        Serial.print(payload.temperature);  
        Serial.print(' ');
    #endif   
    
    #if SWITCH_LOAD   //discarge battery if Vbat is high enough
      if (payload.AD_Channel[1] > 4000) 
      {
        payload.DIO |= 0x04;
        digitalWrite(9, LOW);  // switch LED on if Vbat > 4000 mV (invert/pullup)      
      }
      if (payload.AD_Channel[1] < 3800) 
      {
        payload.DIO &= 0xFB;
        digitalWrite(9, HIGH);  // switch LED off if Vbat < 3700 mV (invert/pullup)              
      }       
    #endif 
/*        
    uint8_t PGVOUT = two.digiRead();
    uint8_t EHOUT = three.digiRead();

    //set - reset bit b0 = PGVOUT and b1 = EHOUT
    if (PGVOUT) payload.DIO |= 0x01;
    else payload.DIO &= 0xFE; 
    if (EHOUT) payload.DIO |= 0x02;
    else payload.DIO &= 0xFD; 

    #if SERIAL
      Serial.print(payload.DIO, BIN); // b0=PGVOUT, b1=EHOUT, b2=LED discharge 
      Serial.println();
      serialFlush();
    #endif  
*/

    if (payload.AD_Channel[1] > 3700) // transmit if Vbat > 3.700 V
    {
      rf12_sendNow(0, &payload, sizeof payload);
      // set the sync mode to 2 if the fuses are still the Arduino default
      // mode 3 (full powerdown) can only be used with 258 CK startup fuses
      rf12_sendWait(2);      
    }
    
    rf12_sleep(RF12_SLEEP);
    for (byte i=0; i < 10; ++i) Sleepy::loseSomeTime(60000);  // sleep for 60 sec * n
    rf12_sleep(RF12_WAKEUP); 
    payload.counter++;
}
