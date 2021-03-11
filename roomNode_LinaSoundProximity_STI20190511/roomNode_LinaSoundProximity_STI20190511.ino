/// @roomNode_LinaSoundProximity_STI20190511
// Based on RoomNode from jc@wippler.nl
// The complexity in the code below comes from the fact that newly detected PIR
// motion needs to be reported as soon as possible, but only once, while all the
// other sensor values are being collected and averaged in a more regular cycle.

#include <JeeLib.h>
#include "Ports.h"
#include <avr/sleep.h>
#include <util/atomic.h>

#define SERIAL  1   // set to 1 to also report readings on the serial port
#define DEBUG   1   // set to 1 to display each loop() run and PIR trigger
#define SWITCH_LOAD 1 // set to 1 to use LED for indicator

#define PIR_PORT    4   // defined if PIR is connected to a port's DIO pin

#define RETRY_PERIOD    10  // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     5   // maximum number of times to retry
#define ACK_TIME        10  // number of milliseconds to wait for an ack

// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2

// Other variables used in various places in the code:

static byte reportCount;    // count up until next report, i.e. packet send
static byte myNodeID;       // node ID used for this unit

// This defines the structure of the packets which get sent out by wireless:

struct {
    byte moved;   // motion detector: 0..1
    byte count = 0;   
} payload;

// PIR
#define PIR_HOLD_TIME   1   // hold PIR value this many seconds after change
#define PIR_PULLUP      1   // set to one to pull-up the PIR input pin
#define PIR_INVERTED    1   // 0 or 1, to match PIR reporting high or low

/// Interface to a Passive Infrared motion sensor.
class PIR : public Port {
    volatile byte value, changed;
    volatile uint32_t lastOn;
public:
    PIR (byte portnum)
        : Port (portnum), value (0), changed (0), lastOn (0) {}

    // this code is called from the pin-change interrupt handler
    void poll() {
        // see http://talk.jeelabs.net/topic/811#post-4734 for PIR_INVERTED
        byte pin = digiRead() ^ PIR_INVERTED;
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
                    f = 1;
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

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// send packet and wait for ack when there is a motion trigger
static void doTrigger() {
    #if DEBUG
        Serial.print("PIR ");
        Serial.print((int) payload.moved);
        serialFlush();
    #endif

    for (byte i = 0; i < RETRY_LIMIT; ++i) {
        rf12_sleep(RF12_WAKEUP);
        rf12_sendNow(RF12_HDR_ACK, &payload, sizeof payload);
        rf12_sendWait(RADIO_SYNC_MODE);
        byte acked = waitForAck();
        rf12_sleep(RF12_SLEEP);

        if (acked) {
            #if DEBUG
                Serial.print(" ack ");
                Serial.println((int) i);
                serialFlush();
            #endif
            // reset scheduling to start a fresh measurement cycle
            //scheduler.timer(MEASURE, MEASURE_PERIOD);
            return;
        }
        
        delay(RETRY_PERIOD * 100);
    }
    //scheduler.timer(MEASURE, MEASURE_PERIOD);
    #if DEBUG
        Serial.println(" no ack!");
        serialFlush();
    #endif
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

static void serialFlush () {
    #if ARDUINO >= 100
        Serial.flush();
    #endif  
    delay(2); // make sure tx buf is empty before going back to sleep
}

void setup () {
    #if SERIAL || DEBUG
        Serial.begin(57600);
        Serial.print("\n[roomNode_LinaSoundProximity_STI20190511]");
        myNodeID = rf12_config();
        serialFlush();
    #else
        myNodeID = rf12_config(0); // don't report info on the serial port
    #endif
    
    // Initialize RFM12B as an 868Mhz module and Node 1 + Group 210:
    //rf12_initialize(1, RF12_868MHZ, 210);     
    rf12_sleep(RF12_SLEEP); // power down
    
    pir.digiWrite(PIR_PULLUP);
    bitSet(PCMSK2, PIR_PORT + 3);
    bitSet(PCICR, PCIE2);

    #if SWITCH_LOAD   //if LED indicator is selected
        pinMode(9, OUTPUT);   // build-in LED on PB1 = Arduino pin 9
        digitalWrite(9, HIGH);  // switch LED off (invert/pullup)                     
    #endif     
}

void loop () {

    if (pir.triggered()) {
        payload.count++;
        doTrigger();
        #if SWITCH_LOAD   //LED
          digitalWrite(9, LOW);  // switch LED on        
        #endif                           
        payload.moved = pir.state();

        #if SWITCH_LOAD   //LED
          delay(100);
          digitalWrite(9, HIGH);  // switch LED off        
        #endif                     
    }
}
