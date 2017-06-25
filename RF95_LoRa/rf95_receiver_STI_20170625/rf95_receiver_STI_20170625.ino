// rf95_receiver_STI_20170625.pde

#include <SPI.h>
#include <RH_RF95.h>
// Singleton instance of the radio driver
RH_RF95 rf95;
//RH_RF95 rf95(5, 2); // Rocket Scream Mini Ultra Pro with the RFM95W
//RH_RF95 rf95(8, 3); // Adafruit Feather M0 with RFM95 
// Need this on Arduino Zero with SerialUSB port (eg RocketScream Mini Ultra Pro)
//#define Serial SerialUSB
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
}

void loop()
{
  // Wait to receive a message
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  uint8_t from;
  
  if (rf95.waitAvailableTimeout(3000))
  { 
    // Should be a message for us now   
    if (rf95.recv(upayload.sendBuf, &len))
    {
      from = rf95.headerFrom();
      digitalWrite(led, LOW);    
      Serial.print("OK ");
      Serial.print(from);      
      Serial.print(" ");            
      Serial.print(upayload.sendBuf[0]);
      Serial.print(" ");      
      Serial.print(upayload.sendBuf[1]);
      Serial.print(" ");
      Serial.println(upayload.sendBuf[2]);   

      /*
      Serial.print("got message: ");
      Serial.print(upayload.payload.temp, DEC);
      Serial.print("   ");      
      Serial.println(upayload.payload.count, DEC);  
      Serial.println(from, DEC);        
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);   */
      digitalWrite(led, HIGH);       
    }
    else
    {
      Serial.println("recv failed");
    }
  }
}

