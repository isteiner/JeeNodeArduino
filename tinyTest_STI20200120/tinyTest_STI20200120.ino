/// @dir tinyTest
/// New version of the Room Node for the ATtiny85 (derived from rooms.pde).
// 2010-10-19 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// see http://jeelabs.org/2010/10/20/new-roomnode-code/
// and http://jeelabs.org/2010/10/21/reporting-motion/

// The complexity in the code below comes from the fact that newly detected PIR
// motion needs to be reported as soon as possible, but only once, while all the
// other sensor values are being collected and averaged in a more regular cycle.

#include <JeeLib.h>
#include <PortsSHT11.h>
#include <avr/sleep.h>
#include <util/atomic.h>

#define RADIO_SYNC_MODE 2

static MilliTimer reportTimer;

struct {
    byte light;     // light sensor: 0..255
    byte moved :1;  // motion detector: 0..1
    byte humi  :7;  // humidity: 0..100
    int temp   :10; // temperature: -500..+500 (tenths)
    byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} payload;

static void blink (byte pin, byte n = 3) {
    for (byte i = 0; i < 2 * n; ++i) {
        delay(100);
        digitalWrite(pin, !digitalRead(pin));
    }
}

void setup () {
    pinMode(0, OUTPUT);
    pinMode(1, OUTPUT);
    pinMode(8, OUTPUT);
    pinMode(10, OUTPUT);  //connect LED on DIO0 - PA0
    digitalWrite(8, 1);
    digitalWrite(9, 1);
    delay(1000);
    rf12_initialize(1, RF12_868MHZ, 210);
    blink(0);
    rf12_recvDone();
    rf12_recvDone();
    rf12_recvDone();
    MilliTimer t;
    while (!t.poll(1000))
        ;
    blink(10,2);

    payload.humi = 55;
    payload.temp = 22;
}

void loop () {
    if (rf12_recvDone())
        blink(0, 1);
    
    if (reportTimer.poll(3000)) {
        blink(10, 4); 
        payload.light++;       
        rf12_sendNow(RF12_HDR_ACK, &payload, sizeof payload);      
    }
 }
