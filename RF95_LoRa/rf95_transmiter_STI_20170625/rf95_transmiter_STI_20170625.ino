// rf95_transmiter_STI_20170625.pde

#include <SPI.h>
#include <RH_RF95.h>

#define TX_ADDR 2         // Transmitter address
#define FREQ_CENTRE 868   // Centre frequency in MHz

// Singleton instance of the radio driver (default setup for Arduino)
RH_RF95 rf95;

int led = 9;

struct typePayload {
    int temp;         // temperature (tenths)
    uint8_t count;  // counter
//    int test;    
} payload;

union utypepayload{
    typePayload payload;
    uint8_t sendBuf[RH_RF95_MAX_MESSAGE_LEN]; 
}upayload;

// sets a new spreding factor
boolean setSpreadingFactor(byte SF) { // abort and return FALSE if new spreading factor not in acceptable range; otherwise, set spreading factor and return TRUE
  uint8_t currentSF, newLowerNibble, newUpperNibble, current_register_1E_value, new_register_1E_value;
  if ((SF >= 6) && (SF <= 12)) {// only set if the spreading factor is in the proper range
    current_register_1E_value = rf95.spiRead(0x1E);
    Serial.print(F("Current value of RF95 register 0x1E = "));
    Serial.println(current_register_1E_value, HEX);
    currentSF = current_register_1E_value >> 4;  //upper nibble of register 0x1E
    Serial.print(F("Current spreading factor = "));
    Serial.println(currentSF, HEX);
    newLowerNibble = (current_register_1E_value & 0x0F); //same as old lower nibble
    newUpperNibble = (SF << 4); // upper nibble equals new spreading factor
    new_register_1E_value = (newUpperNibble + newLowerNibble);
    rf95.spiWrite(0x1E, new_register_1E_value);
    Serial.print(F("New spreading factor = "));
    Serial.println(SF);
    Serial.print(F("New value of register 0x1E = "));
    Serial.println(new_register_1E_value, HEX);
    return true;
  }
  else {
    return false;
  }
}
  
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
  
  rf95.setHeaderFrom(TX_ADDR);    // Sets the transmitter address which is Header From
  rf95.setFrequency(FREQ_CENTRE); // Sets the transmitter and receiver centre frequency
  rf95.setTxPower(20, false);     // Sets the transmitter power output level 

  //rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512);  //set for pre-configured long range
  //setSpreadingFactor(10);
/*
  // Set new value in register 0x09 RegPaConfig for maximum power
  uint8_t register_09_value = rf95.spiRead(0x09); 
  Serial.print(F("Current value of RF95 register 0x09 = "));
  Serial.println(register_09_value, HEX);
  register_09_value = 0xFF;
  rf95.spiWrite(0x09, register_09_value);
  register_09_value = rf95.spiRead(0x09); 
  Serial.print(F("Current value of RF95 register 0x09 = "));
  Serial.println(register_09_value, HEX); */
        
  //Serial.println(rf95.mode(), HEX);
  Serial.println("Registers: "); 
  rf95.printRegisters();
  Serial.println("=================================================");   
}

void loop()
{
      digitalWrite(led, HIGH);
      upayload.payload.temp = 100 + upayload.payload.temp;
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

