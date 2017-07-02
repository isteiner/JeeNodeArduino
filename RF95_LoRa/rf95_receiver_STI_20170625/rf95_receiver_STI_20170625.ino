// rf95_receiver_STI_20170625.pde

#include <SPI.h>
#include <RH_RF95.h>

#define FREQ_CENTRE 868   // Centre frequency in MHz

// Singleton instance of the radio driver
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

static void showNibble (byte nibble) {
    char c = '0' + (nibble & 0x0F);
    if (c > '9')
        c += 7;
    Serial.print(c);
}

static void showByte (byte value) {
    //if (config.hex_output) {
    //    showNibble(value >> 4);
    //    showNibble(value);
    //} else
        Serial.print((word) value);
}

static void printOneChar (char c) {
    Serial.print(c);
}

static void showString (PGM_P s) {
    for (;;) {
        char c = pgm_read_byte(s++);
        if (c == 0)
            break;
        if (c == '\n')
            printOneChar('\r');
        printOneChar(c);
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
  // If you are using Modtronix inAir4 or inAir9,or any other module which uses the
  // transmitter RFO pins and not the PA_BOOST pins
  // then you can configure the power transmitter power for -1 to 14 dBm and with useRFO true. 
  // Failure to do that will result in extremely low transmit powers.
//  driver.setTxPower(14, true);
  rf95.setFrequency(FREQ_CENTRE); // Sets the transmitter and receiver centre frequency
  Serial.println("Registers: "); 
  //Serial.println(rf95.mode(), HEX);
  rf95.printRegisters();
  Serial.println("=================================================");   
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
 /*   Serial.println(len); 
      digitalWrite(led, LOW);    
      Serial.print("OK ");
      Serial.print(from);      
      Serial.print(" ");            
      Serial.print(upayload.sendBuf[0]);
      Serial.print(" ");      
      Serial.print(upayload.sendBuf[1]);
      Serial.print(" ");
      Serial.println(upayload.sendBuf[2]);  */
      
      // take it from RF12demo JeeLab
      showString(PSTR("OK"));
      printOneChar(' ');
      showByte(from);
      for (byte i = 0; i < len; ++i) {
         printOneChar(' ');
         showByte(upayload.sendBuf[i]);
      }
      // display RSSI value after packet data
      showString(PSTR(" ("));
      //Serial.print(-(RF69::rssi>>1));
      Serial.print(rf95.lastRssi());        
      showString(PSTR(") "));
      Serial.println();      
      
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

