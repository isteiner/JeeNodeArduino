// This "rooms" code is for room sensing, i.e. temp/humid/light/motion.
// 2009-03-18 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id: rooms.pde 6100 2010-10-27 13:40:12Z jcw $

// Needs a plug on ports 1 and 4 wired to an SHT11, an LDR, and a PIR sensor.
// The PIR sensor can be either a simple digital I/O pin sensor or a ZDOT SBC.
// Wireless node configuration is obtained from EEPROM, as set by "RF12demo".
// Ports 2 and 3 are unused and could perhaps be used for proximity switches.
// This code was derived from the older "pulse" project and is a bit simpler.

#define EPIR 0      // 1 for ZDOTS EPIR, 0 for ELV or Parallax PIR sensor
#define SHT1X 1     // 1 for SHT11/SHT15, 0 for 1-Wire DS18B20

// The orientation of the room board can be a bit confusing:
//
// If EPIR is used, then it connects to port 4 and the LDR uses the EPIR.
// In that case, the SHT11 or DS18B20 connects to port 1.
// The orientation is with the "JeeLabs.org/rb1" text towards the radio.
//
// If EPIR is not used, the the boards is rotated so the PIR is in the middle.
// In that case PIR is on 1, LDR is on 1, and SHT11 or DS18B20 is on port 4.
// The orientation is with the "JeeLabs.org/rb1" text away from the radio.
//
// Feel free to adjust to other configurations or ports, as needed.

#include <Ports.h>
#include <RF12.h>
#include <avr/sleep.h>

#if EPIR
#include <NewSoftSerial.h>
#endif

#if SHT1X
#include <PortsSHT11.h>
#else
#include <OneWire.h>
#endif

struct {
    byte light;     // light sensor
    byte moved :1;  // motion detector
    byte humi  :7;  // humidity
    int temp   :10; // temperature
    byte lobat :1;  // supply voltage dropped under 3.1V
} payload;

#if EPIR

NewSoftSerial pir_ldr (17, 7);

#if SHT1X
SHT11 sht11 (1);
#else
OneWire ds18b20 (4); // 1-wire temperature sensors, uses DIO port 1
#endif

#else

Port pir_ldr (1);

#if SHT1X
SHT11 sht11 (4);
#else
OneWire ds18b20 (7); // 1-wire temperature sensors, uses DIO port 4
#endif

#endif

uint8_t oneID[8];

static void lowPower (byte mode) {
    // prepare to go into power down mode
    set_sleep_mode(mode);
    // disable the ADC
    byte prrSave = PRR, adcsraSave = ADCSRA;
    ADCSRA &= ~ bit(ADEN);
    PRR |= bit(PRADC);
    // zzzzz...
    sleep_mode();
    // re-enable the ADC
    PRR = prrSave;
    ADCSRA = adcsraSave;
}

EMPTY_INTERRUPT(WDT_vect); // just wakes us up to resume

static void watchdogInterrupts (uint8_t mode) {
    MCUSR &= ~(1<<WDRF); // only generate interrupts, no reset
    cli();
    WDTCSR |= (1<<WDCE) | (1<<WDE); // start timed sequence
    WDTCSR = bit(WDIE) | mode; // mode is a slightly quirky bit-pattern
    sei();
}

static byte loseSomeTime (word msecs) {
    // only slow down for periods longer than twice the watchdog granularity
    if (msecs >= 32) {
        for (word ticks = msecs / 16; ticks > 0; --ticks) {
            lowPower(SLEEP_MODE_PWR_DOWN); // now completely power down
            // adjust the milli ticks, since we will have missed several
            extern volatile unsigned long timer0_millis;
            timer0_millis += 16;
        }
        return 1;
    }
    return 0;
}

static MilliTimer sleepTimer;   // poll the sensor once every so often
static MilliTimer aliveTimer;   // without change, force sending every 60 s
static byte radioIsOn = 1;      // track whether the RFM12B is on

static byte periodicSleep (word msecs) {
    // switch to idle mode while waiting for the next event
    lowPower(SLEEP_MODE_IDLE);
    // keep the easy tranmission mechanism going
    if (radioIsOn && rf12_easyPoll() == 0) {
        rf12_sleep(0); // turn the radio off
        radioIsOn = 0;
    }
    // if we will wait for quite some time, go into total power down mode
    if (!radioIsOn) {
        // see http://news.jeelabs.org/2009/12/18/battery-life-estimation/
        if (loseSomeTime(sleepTimer.remaining()))
            sleepTimer.set(1); // really did a power down, trigger right now
    }
    // return true if the time has come to do something meaningful
    return sleepTimer.poll(msecs);
}

static void wakeupToSend (const void* ptr, byte len) {
    char sending = rf12_easySend(ptr, len);
    if (sending) // clear timer if we are about to send a packet anyway
        aliveTimer.set(0);
    // otherwise force a "sign of life" packet out every 60 seconds
    if (aliveTimer.poll(60000))
        sending = rf12_easySend(0, 0); // always returns 1
    if (sending) {
        // make sure the radio is on again
        if (!radioIsOn)
            rf12_sleep(-1); // turn the radio back on
        radioIsOn = 1;
    }
}

#if EPIR
static uint8_t epirCmd(char c) {
    pir_ldr.print(c);
    uint8_t start = millis();
    while ((uint8_t) (millis() - start) < 10) // careful with overflow
        if (pir_ldr.available())
            return pir_ldr.read();
    return 0;
}
#endif

#if !SHT1X
static uint8_t initOneWire () {
    if (ds18b20.search(oneID)) {
        Serial.print(" 1-wire");
        for (uint8_t i = 0; i < 8; ++i) {
            Serial.print(' ');
            Serial.print(oneID[i], HEX);
        }
        Serial.println();
    }
    ds18b20.reset();
}

static int readout1wire () {
    ds18b20.reset();
    ds18b20.skip();
    ds18b20.write(0x4E); // write to scratchpad
    ds18b20.write(0);
    ds18b20.write(0);
    ds18b20.write(0x1F); // 9-bits is enough, measurement takes 94 msec
    ds18b20.reset();
    ds18b20.skip();
    ds18b20.write(0x44, 0); // start conversion
    delay(100);
    ds18b20.reset();
    // ds18b20.select(oneID);
    ds18b20.skip();
    ds18b20.write(0xBE); // read scratchpad
    uint8_t data[9];
    for (uint8_t j = 0; j < 9; ++j)
        data[j] = ds18b20.read();
    ds18b20.reset();
    if (OneWire::crc8(data, 8) != data[8]) {
        Serial.println(" crc? ");
        return 0;
    }
    return ((data[1] << 8) + data[0]) * 10 >> 4; // degrees * 10
}
#endif

static byte getMotion() {
#if EPIR
    return epirCmd('a') == 'Y';
#else
    return pir_ldr.digiRead();
#endif
}

static byte getLight() {
    static byte avgLight;
#if EPIR
    byte light = epirCmd('b');
#else
    pir_ldr.digiWrite2(1);  // pull-up AIO
    byte light = pir_ldr.anaRead() >> 2;
    pir_ldr.digiWrite2(0);  // pull-down to reduce power draw in bright light
#endif
    // keep track of a running average for light levels
    avgLight = (4 * avgLight + light) / 5;
    return avgLight;
}

static MilliTimer reportTimer;  // don't report too often, unless moved
static byte prevMotion;         // track motion detector state

static void shtDelay () {
    loseSomeTime(32);
}

// this code is called once per second, but not all calls will be reported
static void newReadings() {
    byte motion = getMotion();  // always read out, to detect changes quickly
    byte light = getLight();    // always read out, to keep running avg going

    payload.moved = motion != prevMotion;    
    prevMotion = motion;

    if (reportTimer.poll(30000) || payload.moved) {
        payload.light = light;
#if SHT1X
        float humi, temp;
        sht11.measure(SHT11::HUMI, shtDelay);        
        sht11.measure(SHT11::TEMP, shtDelay);
        sht11.calculate(humi, temp);
        // only accept values if the sensor is present
        if (humi > 1) {
            payload.humi = humi + 0.5;
            payload.temp = 10 * temp + 0.5;
        }
#else
        payload.humi = 0;
        payload.temp = readout1wire();
#endif
        payload.lobat = rf12_lowbat();
    }
}

void setup() {
    Serial.begin(57600);
    Serial.print("\n[rooms]");
    rf12_config();
    rf12_easyInit(5); // throttle packet sending to at least 5 seconds apart

#if EPIR
    pir_ldr.begin(9600);
    epirCmd('E'); // extended range
    epirCmd('Y'); // ... on
    epirCmd('F'); // frequency response
    epirCmd('L'); // ... low freq enabled
    epirCmd('S'); // sensitivity
    epirCmd(25);   // ... very high, but not at max limit
    delay(2000);  // wait for ePIR to init, otherwise LDR value is bogus
#else
    pir_ldr.mode(INPUT);
    pir_ldr.digiWrite(1);   // pull-up DIO
    pir_ldr.mode2(INPUT);
    pir_ldr.digiWrite2(1);  // pull-up AIO
#endif

#if !SHT1X
    initOneWire();
#endif

    watchdogInterrupts(0); // 16ms
}

void loop() {
    if (periodicSleep(1000)) {
        newReadings();

        // Serial.print("ROOM ");
        // Serial.print((int) payload.light);
        // Serial.print(' ');
        // Serial.print((int) payload.moved);
        // Serial.print(' ');
        // Serial.print((int) payload.humi);
        // Serial.print(' ');
        // Serial.print((int) payload.temp);
        // Serial.print(' ');
        // Serial.print((int) payload.lobat);
        // Serial.println();
        // delay(3); // make sure tx buf is empty before going back to sleep

        wakeupToSend(&payload, sizeof payload);
    }
}
