// rf95_transmiter_STI_20170625.pde

#include <SPI.h>
#include <RH_RF95.h>

// Transmitter address
#define TX_ADDR 2

// Singleton instance of the radio driver
RH_RF95 rf95;
//RH_RF95 rf95(5, 2); // Rocket Scream Mini Ultra Pro with the RFM95W
//RH_RF95 rf95(8, 3); // Adafruit Feather M0 with RFM95 

int led = 9;

struct typePayload {
    int temp;         // temperature (tenths)
    uint8_t count;  // counter
} payload;

//payload pl;

union utypepayload{
    typePayload payload;
    uint8_t sendBuf[RH_RF95_MAX_MESSAGE_LEN]; 
}upayload;

void setup() 
{
  pinMode(led, OUTPUT);     
  Serial.begin(57600);
  while (!Serial) ; // Wait for serial port to be available
  if (!rf95.init())
    Serial.println("init failed");  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  //  driver.setTxPower(23, false);
  // If you are using Modtronix inAir4 or inAir9,or any other module which uses the
  // transmitter RFO pins and not the PA_BOOST pins
  // then you can configure the power transmitter power for -1 to 14 dBm and with useRFO true. 
  // Failure to do that will result in extremely low transmit powers.
  //  driver.setTxPower(14, true);

  // Set transmitter address which is Header From
  rf95.setHeaderFrom(TX_ADDR);
}

void loop()
{
      digitalWrite(led, HIGH);
      upayload.payload.temp = 100 + upayload.payload.temp;
//      RH_RF95::printBuffer("request: ", buf, len);
      Serial.println(upayload.payload.temp, DEC);      
      Serial.println(upayload.payload.count++, DEC);
      //Serial.println(sizeof(payload), DEC);      
                   
      // Transmit a payload
      rf95.send(upayload.sendBuf, sizeof(payload));      
      rf95.waitPacketSent();
      Serial.println("Transmit");
      digitalWrite(led, LOW);
      delay(1000);
}

