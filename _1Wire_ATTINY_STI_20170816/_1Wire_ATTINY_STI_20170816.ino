/// @dir roomNode
/// New version of the Room Node (derived from rooms.pde).
// 2010-10-19 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// see http://jeelabs.org/2010/10/20/new-roomnode-code/
// and http://jeelabs.org/2010/10/21/reporting-motion/

// The complexity in the code below comes from the fact that newly detected PIR
// motion needs to be reported as soon as possible, but only once, while all the
// other sensor values are being collected and averaged in a more regular cycle.

// 2017-08-12 change by I.Steiner, test for temperature, humidity and counter

#include <JeeLib.h>
#include <PortsSHT11.h>
#include <avr/sleep.h>
#include <util/atomic.h>

#define SERIAL  0   // set to 1 to also report readings on the serial port
#define DEBUG   0   // set to 1 to display each loop() run and PIR trigger

#define MEASURE_PERIOD  50 // how often to measure, in tenths of seconds 
#define REPORT_EVERY    1   // report every N measurement cycles
#define SMOOTH          1   // smoothing factor used for running averages

// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2

// The scheduler makes it easy to perform various tasks at various times:

// 1Wire libraries
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 10 on the Arduino -> PORT 1 on JNu
#define ONE_WIRE_BUS 10

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

enum { MEASURE, REPORT, TASK_END };

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// Other variables used in various places in the code:

static byte reportCount;    // count up until next report, i.e. packet send
static byte myNodeID;       // node ID used for this unit

// This defines the structure of the packets which get sent out by wireless:

struct {
    byte counter;     // test counter 
    byte humi;        // humidity: 0..100
    int temp;         // temperature: -500..+500 (tenths)
 } payload;


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

// readout all the sensors and other values
static void doMeasure() {
 
    // 1Wire
    sensors.requestTemperatures(); // Send the command to get temperatures
    payload.temp = (sensors.getTempCByIndex(0))*10;
    
    //payload.humi = smoothedAverage(payload.humi, humi/10, firstTime);
    //payload.temp = smoothedAverage(payload.temp, temp, firstTime);    
}


// periodic report, i.e. send out a packet and optionally report on serial port
static void doReport() {
    payload.counter++;
    rf12_sleep(RF12_WAKEUP);
    rf12_sendNow(0, &payload, sizeof payload);
    rf12_sendWait(RADIO_SYNC_MODE);
    rf12_sleep(RF12_SLEEP);
}

void setup () {
      // get the pre-scaler into a known state
      cli();
      CLKPR = bit(CLKPCE);
    #if defined(__AVR_ATtiny84__)
      CLKPR = 0; // div 1, i.e. speed up to 8 MHz
    #else
      CLKPR = 1; // div 2, i.e. slow down to 8 MHz
    #endif
      sei();
    
    #if defined(__AVR_ATtiny84__)
        // power up the radio on JMv3
        bitSet(DDRB, 0);
        bitClear(PORTB, 0);
    #endif

  rf12_initialize(16, RF12_868MHZ, 210); //id = 16; group = 210
  // see http://tools.jeelabs.org/rfm12b
  rf12_control(0xC040); // set low-battery level to 2.2V i.s.o. 3.1V
  //rf12_sleep(RF12_SLEEP);
 
    rf12_sleep(RF12_SLEEP); // power down
    // Start up the 1Wire library
    sensors.begin();
    
    //reportCount = REPORT_EVERY;     // report right away for easy debugging
    scheduler.timer(MEASURE, 0);    // start the measurement loop going
    Sleepy::loseSomeTime(5000); //don't start too fast  
}

void loop () {
    #if DEBUG
        Serial.print('.');
        serialFlush();
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
