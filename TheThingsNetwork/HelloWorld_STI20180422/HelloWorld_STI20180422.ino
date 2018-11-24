// MIT License
// https://github.com/gonzalocasas/arduino-uno-dragino-lorawan/blob/master/LICENSE
// Based on examples from https://github.com/matthijskooijman/arduino-lmic
// Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman

// Adaptions: Andreas Spiess
// changes serial, DiitalWrite, keys definition      I.Steiner 22.4.2018

#include <lmic.h>
#include <hal/hal.h>
//#include <credentials.h>

#ifdef CREDENTIALS
static const u1_t NWKSKEY[16] = NWKSKEY1;
static const u1_t APPSKEY[16] = APPSKEY1;
static const u4_t DEVADDR = DEVADDR1;
#else
static const PROGMEM u1_t  NWKSKEY[16] = {0xA2, 0xA5, 0xB6, 0xB1, 0xA5, 0x1B, 0xC8, 0xCA, 0x59, 0x88, 0x24, 0xCA, 0x1F, 0xF7, 0x6F, 0xBC};   //{0x12, 0x34, 0x56, 0x78, 0x91, 0x23, 0x45, 0x67};
static const u1_t PROGMEM  APPSKEY[16] = {0x27, 0x26, 0xEB, 0x2C, 0xA6, 0xE7, 0x06, 0xBF, 0xFE, 0xDF, 0xB3, 0x66, 0xC8, 0x31, 0x0F, 0x93};   //{0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x00, 0xBE, 0x86};
static const u4_t DEVADDR = 0x260114AE;
#endif

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 20;

// Pin mapping Dragino Shield
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 6, 7},
};
void onEvent (ev_t ev) {
    if (ev == EV_TXCOMPLETE) {
        Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
        // Schedule next transmission
        os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
    }
}

void do_send(osjob_t* j){
    // Payload to send (uplink)
    static uint8_t message[] = "hi";

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, message, sizeof(message)-1, 0);
        Serial.println(F("Sending uplink packet..."));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    Serial.begin(57600);
    Serial.println(F("Starting..."));

    // LMIC init
    os_init();

    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters.
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);

/*
  // Set static session parameters. Instead of dynamically establishing a session
  // by joining the network, precomputed session parameters are be provided.
#ifdef PROGMEM
  // On AVR, these values are stored in flash and only copied to RAM
  // once. Copy them to a temporary buffer here, LMIC_setSession will
  // copy them into a buffer of its own again.
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
  Serial.println(F("PROGMEN"));
#else
  // If not running an AVR with PROGMEM, just use the arrays directly
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
  Serial.println(F("NO-PROGMEN")); 
#endif
  */  

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF12,14);

    // Start job
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
