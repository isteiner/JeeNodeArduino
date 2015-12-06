#include <JeeLib.h>
#include <Ports.h>
#include <RF12.h>
#include <RF12sio.h>

#include <avr/sleep.h>

/// @dir compass_demo
/// Demo sketch for the Modern Device Compass Board.
/// @see http://jeelabs.org/2012/04/02/meet-the-heading-board-replacement/
// 2012-03-29 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// joined projects: compass_ demo & bmp085demo
// STI, 2015 nov. 21



PortI2C myBus (1);
CompassBoard compass (myBus);
MilliTimer timer;

EMPTY_INTERRUPT(WDT_vect); // just wakes us up to resume

void watchdogInterrupts (char mode) {
    MCUSR &= ~(1<<WDRF); // only generate interrupts, no reset
    cli();
    WDTCSR |= (1<<WDCE) | (1<<WDE); // start timed sequence
    WDTCSR = mode >= 0 ? bit(WDIE) | mode : 0;
    sei();
}

void lowPower (byte mode) {
    // prepare to go into power down mode
    set_sleep_mode(mode);
    // disable the ADC
    byte prrSave = PRR, adcsraSave = ADCSRA;
    ADCSRA &= ~ bit(ADEN);
    PRR |=  bit(PRADC);
    // zzzzz...
    sleep_mode();
    // re-enable the ADC
    PRR = prrSave;
    ADCSRA = adcsraSave;
}

byte loseSomeTime (word msecs) {
    // only slow down for periods longer than the watchdog granularity
    if (msecs >= 16) {
        // watchdog needs to be running to regularly wake us from sleep mode
        watchdogInterrupts(0); // 16ms
        for (word ticks = msecs / 16; ticks > 0; --ticks) {
            lowPower(SLEEP_MODE_PWR_DOWN); // now completely power down
            // adjust the milli ticks, since we will have missed several
            extern volatile unsigned long timer0_millis;
            timer0_millis += 16;
        }
        watchdogInterrupts(-1); // off
        return 1;
    }
    return 0;
}

// End of new power-saving code.

struct { int compassTx; byte counter; } payload;
    
void setup() {
    Serial.begin(57600);
    Serial.println("\n[compass_TX]");
    rf12_initialize(9, RF12_868MHZ, 212); // 868 Mhz, net group 212, node 9    
}

void loop() {
    // spend most of the waiting time in a low-power sleep mode
    // note: the node's sense of time is no longer 100% accurate after sleeping
    rf12_sleep(RF12_SLEEP);          // turn the radio off
    //loseSomeTime(timer.remaining()); // go into a (controlled) comatose state
    
    while (!timer.poll(5000))
        lowPower(SLEEP_MODE_IDLE);   // still not running at full power

    payload.compassTx = compass.heading();
    Serial.println(payload.compassTx);
    payload.counter++;
    
    rf12_sleep(RF12_WAKEUP);         // turn radio back on at the last moment
    
    MilliTimer wait;                 // radio needs some time to power up, why?
    while (!wait.poll(5)) {
        rf12_recvDone();
        lowPower(SLEEP_MODE_IDLE);
    }
        
    while (!rf12_canSend())
        rf12_recvDone();
    
    rf12_sendStart(1, &payload, sizeof payload); 
    delay(500);
}

