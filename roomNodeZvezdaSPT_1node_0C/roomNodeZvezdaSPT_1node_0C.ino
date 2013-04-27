// Inteligentno zaščitno pregrinjalo IZP - Zvezda SPT
// Programmed by Igor Steiner, Matej Lenarčič (INEA d.o.o.)

// Modified for measurement with 4 roomNode sensors, based on JeeLabs RoomNode 

#include <Ports.h>
#include <PortsSHT11.h>
#include <RF12.h>
#include <avr/sleep.h>
#include <util/atomic.h>

#define ALARM_TEMP  0  // alarm temperature is under 0.0 deg.C

#define SERIAL  1   // set to 1 to also report readings on the serial port
#define DEBUG   1   // set to 1 to display each loop() run and PIR trigger -----ORIG 0

#define SHT11_PORT1  1   // defined  SHT11 ports
#define SHT11_PORT2  2   // defined  SHT11 ports
#define SHT11_PORT3  3   // defined  SHT11 ports
#define SHT11_PORT4  4   // defined  SHT11 ports

#define MEASURE_PERIOD  5 // how often to measure, in tenths of seconds -----ORIG 600
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

// set pin numbers:
const int ledPin =  8;      // the number of the LED pin
const int vibePin = 9;      // the number of the vibration pin

int ledState = LOW;             // ledState used to set the LED
int vibeState = LOW;
long previousMillis = 0;        // will store last time LED was updated
long interval = 1000;           // interval at which to blink (milliseconds)

// This defines the structure of the packets which get sent out by wireless:

struct {
    int temp[4];   // temperature: -500..+500 (tenths)
    byte humi[4];  // humidity: 0..100   
    byte count;    // counter for diagnostic   
    byte lobat :1; // supply voltage dropped under 3.1V: 0..1
} payload;

SHT11 sht11_P1 (SHT11_PORT1);
//SHT11 sht11_P2 (SHT11_PORT2);
//SHT11 sht11_P3 (SHT11_PORT3);
//SHT11 sht11_P4 (SHT11_PORT4);

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
    byte alarm = LOW;
    
    payload.lobat = rf12_lowbat();
    float h, t;
      
    // meassurement on P1 port
    sht11_P1.measure(SHT11::HUMI, shtDelay);        
    sht11_P1.measure(SHT11::TEMP, shtDelay);
    sht11_P1.calculate(h, t);
    payload.humi[0] = payload.humi[1] = payload.humi[2] = payload.humi[3] = h + 0.5;
    payload.temp[0] = payload.temp[1] = payload.temp[2] = payload.temp[3] = 10 * t + 0.5;

/* 
    // meassurement on P2 port
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
*/    
//    for (byte i=0; i<4; i++) {
      if (payload.temp[0] < ALARM_TEMP)  alarm = HIGH;
//    }  
    checkAlarm(alarm);    
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

void blink (byte pin) {
    for (byte i = 0; i < 6; ++i) {
        delay(100);
        digitalWrite(pin, !digitalRead(pin));
    }
}

void checkAlarm (byte alarmSet) {
  unsigned long currentMillis = millis();
   
  if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   

    // if the LED is off turn it on and vice-versa:
    if ((ledState == LOW)&& alarmSet) {
      ledState = vibeState = HIGH;
      Serial.println("ALARM");
    }  
    else
      ledState = vibeState = LOW;
  }
  // set the LED with the ledState of the variable:
  digitalWrite(ledPin, ledState);
  digitalWrite(vibePin, vibeState);         
}

void setup () {
    #if SERIAL || DEBUG
        Serial.begin(57600);
        Serial.print("\n[roomNode.4]");
        myNodeID = rf12_config();
    #else
        myNodeID = rf12_config(0); // don't report info on the serial port
    #endif
    
    // LEM - dodal
    rf12_control(0xC623);
    
    rf12_sleep(RF12_SLEEP); // power down
    
    reportCount = REPORT_EVERY;     // report right away for easy debugging
    scheduler.timer(MEASURE, 0);    // start the measurement loop going
    
    // set the Alarm digital pin as output:
    pinMode(ledPin, OUTPUT);      
    pinMode(vibePin, OUTPUT);    
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
