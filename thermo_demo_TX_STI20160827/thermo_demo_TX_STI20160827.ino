/// @dir thermo_demo
/// Demo sketch for the Thermo Plug.
// 2009-09-17 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <JeeLib.h>
#define BLIP_NODE 22  // wireless node ID to use for sending blips
#define BLIP_GRP  3   // wireless net group to use for sending blips
#define BLIP_ID   1   // set this to a unique ID to disambiguate multiple nodes
#define SEND_MODE 2   // set to 3 if fuses are e=06/h=DE/l=CE, else set to 2

Port tplug (1);

struct {
  int temperature;
  int ping;       // 16-bit counter
 } payload;
 
void setup () {
    //Serial.begin(57600);
    //Serial.println("\n[thermo_demo]");
    rf12_initialize(BLIP_NODE, RF12_868MHZ, BLIP_GRP);
    tplug.mode(OUTPUT);
    tplug.digiWrite(1);
    delay(1000);
    tplug.digiWrite(0);
}

void loop () {
    ++payload.ping;
    //payload.temperature = tplug.anaRead();
    //payload.temperature = analogRead(1);    // read the input pin 
    //Serial.print(t);
    //Serial.print(' ');
    //Serial.println(map(t, 0, 1023, 0, 330)); // 10 mv/C
    //temperature = map(temperature, 0, 1023, 0, 330); // 10 mv/C
    rf12_sendNow(0, &payload, sizeof payload);
    rf12_sendWait(SEND_MODE);
    delay(1000);
}
