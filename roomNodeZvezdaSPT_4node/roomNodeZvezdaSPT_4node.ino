// New version of the Room Node, derived from rooms.pde 
// 2010-10-19 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id: roomNode.pde 7503 2011-04-07 10:41:06Z jcw $

// see http://jeelabs.org/2010/10/20/new-roomnode-code/
// and http://jeelabs.org/2010/10/21/reporting-motion/

// The complexity in the code below comes from the fact that newly detected PIR
// motion needs to be reported as soon as possible, but only once, while all the
// other sensor values are being collected and averaged in a more regular cycle.

// Modified for measurement with 4 roomNode sensors and without LDR and PIR
// Igor Steiner  2012-12-26

#include <Ports.h>
#include <PortsSHT11.h>
#include <RF12.h>
#include <avr/sleep.h>
#include <util/atomic.h>

#define SERIAL  1   // set to 1 to also report readings on the serial port
#define DEBUG   1   // set to 1 to display each loop() run and PIR trigger -----ORIG 0

#define SHT11_PORT1  1   // defined  SHT11 ports
#define SHT11_PORT2  2   // defined  SHT11 ports
#define SHT11_PORT3  3   // defined  SHT11 ports
#define SHT11_PORT4  4   // defined  SHT11 ports

#define MEASURE_PERIOD  20 // how often to measure, in tenths of seconds -----ORIG 600
#define REPORT_EVERY    1   // report every N measurement cycles  -----ORIG 5

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
    int temp[4];   // temperature: -500..+500 (tenths)
    byte humi[4];  // humidity: 0..100   
    byte count;    // counter for diagnostic   
    byte lobat :1; // supply voltage dropped under 3.1V: 0..1
} payload;

SHT11 sht11_P1 (SHT11_PORT1);
SHT11 sht11_P2 (SHT11_PORT2);
SHT11 sht11_P3 (SHT11_PORT3);
SHT11 sht11_P4 (SHT11_PORT4);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// spend a little time in power down mode while the SHT11 does a measurement
static void shtDelay () {
    Sleepy::loseSomeTime(32); // must wait at least 20 ms
}

// readout all the sensors and other values
static void doMeasure() {
    // special case to init running avg
    byte firstTime = payload.humi[0] = payload.humi[1] = payload.humi[2] = payload.humi[3]== 0; 
    
    payload.lobat = rf12_lowbat();
    float h, t;
      
    // meassurement on P1 port
    sht11_P1.measure(SHT11::HUMI, shtDelay);        
    sht11_P1.measure(SHT11::TEMP, shtDelay);
    sht11_P1.calculate(h, t);
    payload.humi[0] = h + 0.5;
    payload.temp[0] = 10 * t + 0.5;
 
    // meassurement on P1 port
    sht11_P2.measure(SHT11::HUMI, shtDelay);        
    sht11_P2.measure(SHT11::TEMP, shtDelay);
    sht11_P2.calculate(h, t);
    payload.humi[1] = h + 0.5;
    payload.temp[1] = 10 * t + 0.5;
    
    // meassurement on P3 port
    sht11_P3.measure(SHT11::HUMI, shtDelay);        
    sht11_P3.measure(SHT11::TEMP, shtDelay);
    sht11_P3.calculate(h, t);
    payload.humi[2] = h + 0.5;
    payload.temp[2] = 10 * t + 0.5;  
   
    // meassurement on P4 port
    sht11_P4.measure(SHT11::HUMI, shtDelay);        
    sht11_P4.measure(SHT11::TEMP, shtDelay);
    sht11_P4.calculate(h, t);
    payload.humi[3] = h + 0.5;
    payload.temp[3] = 10 * t + 0.5;
}

// periodic report, i.e. send out a packet and optionally report on serial port
static void doReport() {
    payload.count++;
    rf12_sleep(RF12_WAKEUP);
    while (!rf12_canSend())
        rf12_recvDone();
    rf12_sendStart(0, &payload, sizeof payload, RADIO_SYNC_MODE);
    rf12_sleep(RF12_SLEEP);

    #if SERIAL
        Serial.print("T1=");
        Serial.print(payload.temp[0]);
        Serial.print(" H1=");
        Serial.print(payload.humi[0]);
        Serial.print(" T2=");
        Serial.print(payload.temp[1]);
        Serial.print(" H2=");
        Serial.print(payload.humi[1]);
        Serial.print(" T3=");
        Serial.print(payload.temp[2]);
        Serial.print(" H3=");
        Serial.print(payload.humi[2]); 
        Serial.print(" T4=");
        Serial.print(payload.temp[3]);
        Serial.print(" H4=");
        Serial.print(payload.humi[3]);       
        Serial.print(' ');
        // Serial.print((int) payload.lobat);
        Serial.println();
        delay(10); // make sure tx buf is empty before going back to sleep, delay for 10ms
    #endif
}


void setup () {
    #if SERIAL || DEBUG
        Serial.begin(57600);
        Serial.print("\n[roomNode.4]");
        myNodeID = rf12_config();
    #else
        myNodeID = rf12_config(0); // don't report info on the serial port
    #endif
    
    rf12_sleep(RF12_SLEEP); // power down
    
    reportCount = REPORT_EVERY;     // report right away for easy debugging
    scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

void loop () {
    #if DEBUG
        Serial.print('.');
        delay(2);
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
