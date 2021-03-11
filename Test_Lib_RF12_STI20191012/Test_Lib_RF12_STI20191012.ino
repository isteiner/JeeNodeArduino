// Test RF12 library for Hope module RFM12B 

#include <JeeLib.h>
byte payload=0;

void setup () {
    Serial.begin(57600);
    Serial.println("\n[Lib-RF12-Test]");
    rf12_initialize(5, RF12_868MHZ, 210); //id=5, group=210
}

void loop () {
    rf12_sendNow(0, &payload, sizeof payload);
    //Serial.println(payload);
    payload++;
    
    delay(2000);
}
