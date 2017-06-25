// Capacitive sensor - proximity switch for JeeNode & Lina diploma work
// Igor Steiner
// Date 02.10.2016

/// @dir proximity_demo
/// Demo of the Proximity Plug, based on ProximityPlug class in Ports library.
// 2010-03-19 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <JeeLib.h>
#include <RF12.h>
// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2

// value for payload if capacitive proximity is set
#define CAPSET 100

//PortI2C myBus (4, PortI2C::KHZ100); // max 100 KHz!
PortI2C myBus (4, 15); // max 100 KHz!
ProximityPlug sensor (myBus);

struct {
    byte capSensePin1;     // sensor 1
    byte capSensePin2;     // sensor 2
    byte capSensePin3;     // sensor 3
    byte counter;          // counter
} payload;

/* MPR084 Registers   see: http://www.nxp.com/files/sensors/doc/data_sheet/MPR084.pdf

FIFO      FIFO Register                     0x00
FAULT     Fault Register                    0x01
TPSTATUS  Touch Pad Status Register         0x02
TPCONFIG  Touch Pad Configuration Register  0x03
STR1      Sensitivity Threshold Registers 1 0x04
STR2      Sensitivity Threshold Registers 2 0x05
STR3      Sensitivity Threshold Registers 3 0x06
STR4      Sensitivity Threshold Registers 4 0x07
STR5      Sensitivity Threshold Registers 5 0x08
STR6      Sensitivity Threshold Registers 6 0x09
STR7      Sensitivity Threshold Registers 7 0x0A
STR8      Sensitivity Threshold Registers 8 0x0B
ECEMR     Electrode Channel Enable Mask Register 0x0C
MNTPR     Maximum Number of Touched Positions Register 0x0D
MTPR      Master Tick Period Register       0x0E
TASPR     Touch Acquisition Sample Period Register 0x0F
SCR       Sounder Configuration Register    0x10
LPCR      Low Power Configuration Register  0x11
SKTR      Stuck Key Timeout Register        0x12
CONFIG    Configuration Register            0x13
SINFO     Sensor Information Register       0x14

TPCONFIG Touch Pad Configuration Register Descriptions:
7 TPSE Touch Pad Sounder Enable – The Touch Pad Sounder Enable bit controls if data
is sent to the sounder.
0 Disable – Click Feedback Off
1 Enable – Click Feedback On

5 BKA Best Key Algorithm - The Best Key Algorithm, when enabled the Maximum Number
of Touches register is ignored and the algorithm reports the single best key based
upon the BKA algorithm and all electrodes that are decoding a touch.
0 BKA Disabled
1 BKA Enabled

4 ACE Auto Calibration Enable – The Auto Calibration Enable bit enables or disables the
auto calibration function.
0 Disable
1 Enable

3 TPRBE Touch Pad Release Buffer Enable – The Touch Pad Release Buffer Enable bit
determines whether or not data is logged in the FIFO when the touch pad
transitions from a touched to untouched state.
0 Disable – No Release Data Logged
1 Enable – Release Data Logged

2 TPTBE Touch Pad Touch Buffer Enable – The Touch Pad Touch Buffer Enable bit
determines whether or not data is logged in the FIFO any time a button is pressed.
0 Disable – Touches are not logged
1 Enable – Touches are logged

0 TPE Touch Pad Enable – The Touch Pad Enable bit enables or disables the touch
sensor. When disabled, no touches are detected.
0 Disable – Touches not detected
1 Enable – Touches detected
*/

// Pin for the LEDs
int LED1 = 4;
int LED2 = 5;
int LED3 = 6;

int sendNow = 0;
    
void setup () {
    // Set up the LEDs
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);        
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);    
    digitalWrite(LED3, LOW);
      
    byte v;
    Serial.begin(57600);
    Serial.println("\n[proximity_demo]");
    sensor.begin();
 
    sensor.send();
    sensor.write(ProximityPlug::SINFO);
    sensor.receive();
    for (;;) {
        char c = sensor.read(0);
        if (c == 0)
            break;
        if (' ' <= c && c <= '~')
            Serial.print(c);
        else {
            Serial.print('<');
            Serial.print(c, HEX);
            Serial.print('>');
        }

    // node 1, 868 Mhz, net group 1 
    // node 1, 868 Mhz, net group 2
    // node 1, 868 Mhz, net group 3

    rf12_initialize(1, RF12_868MHZ, 2); // node=1, freq=868 Mhz, net group=2  
    }
    sensor.read(1);
    sensor.stop();
    Serial.println();

    //setup new configuration for proximity switches
    delay(100);
    sensor.setReg(ProximityPlug::CONFIG, 0x04);   // reset, STOP1
    delay(100);
    sensor.setReg(ProximityPlug::STR1,15);   // Sensitivity: 1-High, 63-Low 
    sensor.setReg(ProximityPlug::STR2,15);   // Sensitivity      
    sensor.setReg(ProximityPlug::STR3,15);   // Sensitivity      
    sensor.setReg(ProximityPlug::TPCONFIG,0x21);   //  BKA-Best Key Alg., TPE-Touch Pad Enable   
    sensor.setReg(ProximityPlug::ECEMR,0x07);   // Enable electrodes: 1,2,3       
    sensor.setReg(ProximityPlug::CONFIG, 0x15);   // RUN1 
    delay(100);
    
    v = sensor.getReg(ProximityPlug::CONFIG);
    Serial.print( "CONFIG ");
    Serial.println( v, BIN );
    v = sensor.getReg(ProximityPlug::LPCR);
    Serial.print( "Low Power Config ");
    Serial.println( v, BIN );
    v = sensor.getReg(ProximityPlug::SCR);
    Serial.print( "Sounder Configuration ");
    Serial.println( v, BIN );
    v = sensor.getReg(ProximityPlug::MNTPR);
    Serial.print( "Maximum Number of Touches ");
    Serial.println( v, BIN );
    v = sensor.getReg(ProximityPlug::MTPR);
    Serial.print( "Master Tick Period ");
    Serial.println( v, BIN );
    v = sensor.getReg(ProximityPlug::TPSTATUS);
    Serial.print( "TPSTATUS ");
    Serial.println( v, BIN );
    v = sensor.getReg(ProximityPlug::TPCONFIG);
    Serial.print( "TPCONFIG ");
    Serial.println( v, BIN );    
    v = sensor.getReg(ProximityPlug::STR1);
    Serial.print( "STR1 ");
    Serial.println( v, BIN );        
    v = sensor.getReg(ProximityPlug::STR2);
    Serial.print( "STR2 ");
    Serial.println( v, BIN );   
    v = sensor.getReg(ProximityPlug::STR3);
    Serial.print( "STR3 ");
    Serial.println( v, BIN );  
    v = sensor.getReg(ProximityPlug::STR4);
    Serial.print( "STR4 ");
    Serial.println( v, BIN );          
}

void loop() {
    byte v = sensor.getReg(ProximityPlug::TPSTATUS);
    byte fault = sensor.getReg(ProximityPlug::FAULT);
    byte tpconfig = sensor.getReg(ProximityPlug::TPCONFIG);
    /*
    //if (v != 0 && v != 0xFF) {
        Serial.print("PROX ");
        Serial.print(v, HEX);
        Serial.print(" FAULT ");
        Serial.print(fault, HEX);
        Serial.print(" TPCONFIG ");
        Serial.println(tpconfig, HEX);
    //} */
    payload.counter++;

    // switch LED if prox is active
    if (v == 1) {
      digitalWrite(LED1, HIGH);
      payload.capSensePin1=CAPSET;
    }
    else {
      digitalWrite(LED1, LOW);
      payload.capSensePin1=0;
    }      
    if (v == 2) {
      digitalWrite(LED2, HIGH);
      payload.capSensePin2=CAPSET;
    }      
    else {
      digitalWrite(LED2, LOW);
      payload.capSensePin2=0;
    }        
    if (v == 4) {
      digitalWrite(LED3, HIGH);
      payload.capSensePin3=CAPSET;
    }      
    else {
      digitalWrite(LED3, LOW);
      payload.capSensePin3=0;
    }    
    // If the capacitive sensor reads value > 0
    if (v) sendNow = 5;
    else sendNow--;
    if (sendNow < 0)  sendNow = 0;
          
    while (!rf12_canSend())
      rf12_recvDone();
    if (sendNow)  
      rf12_sendStart(0, &payload, sizeof payload);  //, RADIO_SYNC_MODE);        
    delay(100);  
}
