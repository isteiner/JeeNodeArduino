/// @dir roomNode
/// New version of the Room Node (derived from rooms.pde).
// 2010-10-19 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// see http://jeelabs.org/2010/10/20/new-roomnode-code/
// and http://jeelabs.org/2010/10/21/reporting-motion/

// The complexity in the code below comes from the fact that newly detected PIR
// motion needs to be reported as soon as possible, but only once, while all the
// other sensor values are being collected and averaged in a more regular cycle.

// Add support for HYT131 and pressure sensor BMP085
// Igor Steiner, 2012-11-18

#include <JeeLib.h>
#include <PortsSHT11.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#include <PortsBMP085.h>

#define SERIAL  1   // set to 1 to also report readings on the serial port
#define DEBUG   1   // set to 1 to display each loop() run and PIR trigger

#define SHT11_PORT  0   // defined if SHT11 is connected to a port  -- ORIG 1
#define HYT131_PORT 1   // defined if HYT131 is connected to a port
#define LDR_PORT    4   // defined if LDR is connected to a port's AIO pin
#define PIR_PORT    4   // defined if PIR is connected to a port's DIO pin
#define BPM085_PORT 2   // defined if pressure BMP085 is connected to a port

#define MEASURE_PERIOD  660 // how often to measure, in tenths of seconds  -- ORIG 600
#define RETRY_PERIOD    10  // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     5   // maximum number of times to retry
#define ACK_TIME        10  // number of milliseconds to wait for an ack
#define REPORT_EVERY    5   // report every N measurement cycles  -- ORIG 5
#define SMOOTH          3   // smoothing factor used for running averages

// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2

// The scheduler makes it easy to perform various tasks at various times:

enum { MEASURE, REPORT, TASK_END };

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// Other variables used in various places in the code:

static byte reportCount;    // count up until next report, i.e. packet send
static byte myNodeID;       // node ID used for this unit

// This defines the structure of the packets which get sent out by wireless:

struct {
    byte light;       // light sensor: 0..255
    byte moved   :1;  // motion detector: 0..1
    byte humi    :7;  // humidity: 0..100
    int temp     :16; // temperature: -500..+500 (tenths)
    int pressure :15; // pressure: hPa    
    byte lobat   :1;  // supply voltage dropped under 3.1V: 0..1
} payload;

// Conditional code, depending on which sensors are connected and how:

#if SHT11_PORT
    SHT11 sht11 (SHT11_PORT);
#endif

#if HYT131_PORT
    PortI2C myBus (HYT131_PORT);
    DeviceI2C hyt131 (myBus, 0x28);
#endif

#if LDR_PORT
    Port ldr (LDR_PORT);
#endif

#if BPM085_PORT
    PortI2C two (BPM085_PORT);
    BMP085 psensor (two, 3); // ultra high resolution    
#endif

#if PIR_PORT
    #define PIR_HOLD_TIME   30  // hold PIR value this many seconds after change
    #define PIR_PULLUP      1   // set to one to pull-up the PIR input pin
    #define PIR_FLIP        0   // 0 or 1, to match PIR reporting high or low
    
    /// Interface to a Passive Infrared motion sensor.
    class PIR : public Port {
        volatile byte value, changed;
        volatile uint32_t lastOn;
    public:
        PIR (byte portnum)
            : Port (portnum), value (0), changed (0), lastOn (0) {}

        // this code is called from the pin-change interrupt handler
        void poll() {
            // see http://talk.jeelabs.net/topic/811#post-4734 for PIR_FLIP
            byte pin = digiRead() ^ PIR_FLIP;
            // if the pin just went on, then set the changed flag to report it
            if (pin) {
                if (!state())
                    changed = 1;
                lastOn = millis();
            }
            value = pin;
        }

        // state is true if curr value is still on or if it was on recently
        byte state() const {
            byte f = value;
            if (lastOn > 0)
                ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                    if (millis() - lastOn < 1000 * PIR_HOLD_TIME)
                        f = 1 ^ PIR_FLIP;
                }
            return f;
        }

        // return true if there is new motion to report
        byte triggered() {
            byte f = changed;
            changed = 0;
            return f;
        }
    };

    PIR pir (PIR_PORT);

    // the PIR signal comes in via a pin-change interrupt
    ISR(PCINT2_vect) { pir.poll(); }
#endif

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
    if (firstTime)
        return next;
    return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

// spend a little time in power down mode while the SHT11 does a measurement
static void shtDelay () {
    Sleepy::loseSomeTime(32); // must wait at least 20 ms
}

// wait a few milliseconds for proper ACK to me, return true if indeed received
static byte waitForAck() {
    MilliTimer ackTimer;
    while (!ackTimer.poll(ACK_TIME)) {
        if (rf12_recvDone() && rf12_crc == 0 &&
                // see http://talk.jeelabs.net/topic/811#post-4712
                rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | myNodeID))
            return 1;
        set_sleep_mode(SLEEP_MODE_IDLE);
        sleep_mode();
    }
    return 0;
}

static void watchdogInterrupts (char mode) {
    MCUSR &= ~(1<<WDRF); // only generate interrupts, no reset
    cli();
    WDTCSR |= (1<<WDCE) | (1<<WDE); // start timed sequence
    WDTCSR = mode >= 0 ? bit(WDIE) | mode : 0;
    sei();
}

static void lowPower (byte mode) {
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

static byte loseSomeTime (word msecs) {
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

// readout all the sensors and other values
static void doMeasure() {
    byte firstTime = payload.humi == 0; // special case to init running avg
    
    payload.lobat = rf12_lowbat();

    #if SHT11_PORT
#ifndef __AVR_ATtiny84__
        sht11.measure(SHT11::HUMI, shtDelay);        
        sht11.measure(SHT11::TEMP, shtDelay);
        float h, t;
        sht11.calculate(h, t);
        int humi = h + 0.5, temp = 10 * t + 0.5;
#else
        //XXX TINY!
        int humi = 50, temp = 25;
#endif
        payload.humi = smoothedAverage(payload.humi, humi, firstTime);
        payload.temp = smoothedAverage(payload.temp, temp, firstTime);
    #endif
    
    #if HYT131_PORT
        // start the measurement
        hyt131.send();
        hyt131.stop();
        delay(100);  
        
        // extaract the readings
        hyt131.receive();
        word h = (hyt131.read(0) & 0x3F) << 8;
        h |= hyt131.read(0);
        word t = hyt131.read(0) << 6;
        t |= hyt131.read(1) >> 2;
        hyt131.stop();
        
        // convert 0..16383 to 0..100.0 % RH
        int humi = (h * 1000L >> 14);
        payload.humi = humi * 0.1;
        // convert 0..16383 to -40.0..125.0 deg.C
        int temp = (t * 1650L >> 14) -400;
        payload.temp = temp;
        
        // report results
        //Serial.print(payload.temp * 0.1, 1);
        //Serial.print (" C, ");
        //Serial.print(payload.humi);
        //Serial.println(" %");
    #endif     
    
    
    #if LDR_PORT
        ldr.digiWrite2(1);  // enable AIO pull-up
        byte light = ~ ldr.anaRead() >> 2;
        ldr.digiWrite2(0);  // disable pull-up to reduce current draw
        payload.light = smoothedAverage(payload.light, light, firstTime);
    #endif
    #if PIR_PORT
        payload.moved = pir.state();
    #endif
    
    #if BPM085_PORT
        // sensor readout takes some time, so go into power down while waiting
        // int32_t traw = psensor.measure(BMP085::TEMP);
        // int32_t praw = psensor.measure(BMP085::PRES);        
        
        psensor.startMeas(BMP085::TEMP);
        loseSomeTime(16);
        int32_t traw = psensor.getResult(BMP085::TEMP);

        psensor.startMeas(BMP085::PRES);
        loseSomeTime(32);
        int32_t praw = psensor.getResult(BMP085::PRES);
    
        struct { int16_t temp; int32_t pres; } measure_psensor;
        psensor.calculate(measure_psensor.temp, measure_psensor.pres);   
        payload.pressure = measure_psensor.pres/10;
        
        // report results
        //Serial.print((int32_t) measure_psensor.pres);
        //Serial.print (" raw ");
        //Serial.print(payload.pressure);
        //Serial.println(" hPa");        
    #endif
}

static void serialFlush () {
    #if ARDUINO >= 100
        Serial.flush();
    #endif  
    delay(2); // make sure tx buf is empty before going back to sleep
}

// periodic report, i.e. send out a packet and optionally report on serial port
static void doReport() {
    rf12_sleep(RF12_WAKEUP);
    while (!rf12_canSend())
        rf12_recvDone();
    rf12_sendStart(0, &payload, sizeof payload, RADIO_SYNC_MODE);
    rf12_sleep(RF12_SLEEP);

    #if SERIAL
        Serial.print("ROOM ");
        Serial.print((int) payload.light);
        Serial.print(' ');
        Serial.print((int) payload.moved);
        Serial.print(' ');
        Serial.print((int) payload.humi);
        Serial.print(' ');
        Serial.print((int) payload.temp);
        Serial.print(' ');
        Serial.print((int) payload.pressure);
        Serial.print(' ');        
        Serial.print((int) payload.lobat);
        Serial.println();
        serialFlush();
    #endif
}

// send packet and wait for ack when there is a motion trigger
static void doTrigger() {
    #if DEBUG
        Serial.print("PIR ");
        Serial.print((int) payload.moved);
        serialFlush();
    #endif

    for (byte i = 0; i < RETRY_LIMIT; ++i) {
        rf12_sleep(RF12_WAKEUP);
        while (!rf12_canSend())
            rf12_recvDone();
        rf12_sendStart(RF12_HDR_ACK, &payload, sizeof payload, RADIO_SYNC_MODE);
        byte acked = waitForAck();
        rf12_sleep(RF12_SLEEP);

        if (acked) {
            #if DEBUG
                Serial.print(" ack ");
                Serial.println((int) i);
                serialFlush();
            #endif
            // reset scheduling to start a fresh measurement cycle
            scheduler.timer(MEASURE, MEASURE_PERIOD);
            return;
        }
        
        delay(RETRY_PERIOD * 100);
    }
    scheduler.timer(MEASURE, MEASURE_PERIOD);
    #if DEBUG
        Serial.println(" no ack!");
        serialFlush();
    #endif
}

void blink (byte pin) {
    for (byte i = 0; i < 6; ++i) {
        delay(100);
        digitalWrite(pin, !digitalRead(pin));
    }
}

void setup () {
    #if SERIAL || DEBUG
        Serial.begin(57600);
        Serial.print("\n[roomNode.3a]");
        myNodeID = rf12_config();
        serialFlush();
    #else
        myNodeID = rf12_config(0); // don't report info on the serial port
    #endif
    
    #if HYT131_PORT
        if (hyt131.isPresent())
          Serial.println("HYT131 found!");
    #endif  
    
    rf12_sleep(RF12_SLEEP); // power down
    
    #if PIR_PORT
        pir.digiWrite(PIR_PULLUP);
      #ifdef PCMSK2
        bitSet(PCMSK2, PIR_PORT + 3);
        bitSet(PCICR, PCIE2);
      #else
        //XXX TINY!
      #endif
    #endif

    #if BPM085_PORT    
      psensor.getCalibData();
    #endif
    
    reportCount = REPORT_EVERY;     // report right away for easy debugging
    scheduler.timer(MEASURE, 0);    // start the measurement loop going   
}

void loop () {
    #if DEBUG
        Serial.print('.');
        serialFlush();
    #endif

    #if PIR_PORT
        if (pir.triggered()) {
            payload.moved = pir.state();
            doTrigger();
        }
    #endif
    
    switch (scheduler.pollWaiting()) {

        case MEASURE:
            // reschedule these measurements periodically
            scheduler.timer(MEASURE, MEASURE_PERIOD);
    
            doMeasure();

            // every so often, a report needs to be sent out
            if (++reportCount >= REPORT_EVERY) {
                reportCount = 0;
                scheduler.timer(REPORT, 0);
            }
            break;
            
        case REPORT:
            doReport();
            break;
    }
}
