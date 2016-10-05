// Capacitive sensor - proximity switch for JeeNode & Lina diploma work
// Igor Steiner
// Date 02.10.2016

/// @dir proximity_demo
/// Demo of the Proximity Plug, based on ProximityPlug class in Ports library.
// 2010-03-19 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <JeeLib.h>

//PortI2C myBus (4, PortI2C::KHZ100); // max 100 KHz!
PortI2C myBus (4, 15); // max 100 KHz!
ProximityPlug sensor (myBus);

void setup () {
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
    }
    sensor.read(1);
    sensor.stop();
    Serial.println();

    //setup new configuration for proximity switches
    delay(100);
    sensor.setReg(ProximityPlug::CONFIG, 0x04);   // reset, STOP1
    delay(100);
    sensor.setReg(ProximityPlug::STR1,01);   // Sensitivity: 1-High, 63-Low 
    sensor.setReg(ProximityPlug::STR2,01);   // Sensitivity      
    sensor.setReg(ProximityPlug::STR3,01);   // Sensitivity      
    sensor.setReg(ProximityPlug::TPCONFIG,0x21);   // BKA-Best Key Alg., TPE-Touch Pad Enable   
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
    //if (v != 0 && v != 0xFF) {
        Serial.print("PROX ");
        Serial.print(v, HEX);
        Serial.print(" FAULT ");
        Serial.print(fault, HEX);
        Serial.print(" TPCONFIG ");
        Serial.println(tpconfig, HEX);
    //}
    delay (100);
}
