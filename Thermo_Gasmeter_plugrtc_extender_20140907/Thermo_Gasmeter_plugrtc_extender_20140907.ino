// Date and time functions using a DS1307 RTC connected via the Ports library
// 2010-02-04 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <JeeLib.h>
#include <Wire.h> // needed to avoid a linker error :(
#include <RTClib.h>

// keyboard inputs 
#define SET   B00000001
#define PLUS  B00000010
#define MINUS B00000100
#define ESC   B00001000

//gas counter states
#define HIGH_STATE     10    // define high state 
#define LOW_STATE      20    // define low state
#define ACK_LOW_STATE  30    // define ack low state

// Port 1, IO expander
PortI2C myport (1 /*, PortI2C::KHZ400 */);
DeviceI2C expander (myport, 0x20); // also works with output plug if 0x26/0x27

// Port 2, magnetic compass, used for gas meter counting
PortI2C myBus (2);
CompassBoard compass (myBus);

// Port 4, thermocouple sensor plug
Port tplug (4);

unsigned char lowSignal = 0;
unsigned char state = HIGH_STATE;
int tick = 0;

// Payload data structure:

struct {
    int temp;   // temperature: (tenths)
    unsigned long gasCounter;    // gas counter: (tenths) m3 
    byte count;  // counter
} payload;

enum {
  MCP_IODIR, MCP_IPOL, MCP_GPINTEN, MCP_DEFVAL, MCP_INTCON, MCP_IOCON,
  MCP_GPPU, MCP_INTF, MCP_INTCAP, MCP_GPIO, MCP_OLAT
};

// setup IO expander
static void exp_setup () {
    expander.send();
    expander.write(MCP_IODIR);
//    expander.write(0); // all outputs    
    expander.write(0x1F); // 5 inputs
    expander.stop();
}

static void exp_write (byte value) {
    expander.send();
    expander.write(MCP_GPIO);
    expander.write(value);
    expander.stop();
}

// example code for reading:
static byte exp_read () {
     expander.send();
     expander.write(MCP_GPIO);
     expander.receive();
     byte result = expander.read(1);
     expander.stop();
     return result;
}

// wait untill no key is pressed
void nokey()
{
      byte keyboard=1;
      while (keyboard)
      {
        keyboard = exp_read();
        delay(100);
      }
}

void editDT(int &value, int lolim, int hilim)
{
      nokey();
      byte keyboard = 0; 
      while (!(keyboard & SET))
      {
        Serial.println(value, DEC);
        keyboard = exp_read();
        if (keyboard & PLUS)
          value++;
        else if (keyboard & MINUS)  
          value--;      
        if (value > hilim)
           value = lolim;
        if (value < lolim)
           value = hilim;   
        delay(500);  
      }
}
      
// RTC based on the DS1307 chip connected via the Ports library
class RTC_Plug : public DeviceI2C {
    // shorthand
    static uint8_t bcd2bin (uint8_t val) { return RTC_DS1307::bcd2bin(val); }
    static uint8_t bin2bcd (uint8_t val) { return RTC_DS1307::bin2bcd(val); }
public:
    RTC_Plug (const PortI2C& port) : DeviceI2C (port, 0x68) {}

    void begin() {}
    
    void adjust(const DateTime& dt) {
        send();
        write(0);
        write(bin2bcd(dt.second()));
        write(bin2bcd(dt.minute()));
        write(bin2bcd(dt.hour()));
        write(bin2bcd(0));
        write(bin2bcd(dt.day()));
        write(bin2bcd(dt.month()));
        write(bin2bcd(dt.year() - 2000));
        write(0);
        stop();
    }

    DateTime now() {
      	send();
      	write(0);	
        stop();

        receive();
        uint8_t ss = bcd2bin(read(0));
        uint8_t mm = bcd2bin(read(0));
        uint8_t hh = bcd2bin(read(0));
        read(0);
        uint8_t d = bcd2bin(read(0));
        uint8_t m = bcd2bin(read(0));
        uint16_t y = bcd2bin(read(1)) + 2000;
    
        return DateTime (y, m, d, hh, mm, ss);
    }
};

int year, month, day, hour, minute, second;

// Port 1, RTC
PortI2C i2cBus (1);
RTC_Plug RTC (i2cBus);

void ReadWriteDT()
{
    byte keyboard = exp_read();
    DateTime now = RTC.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    Serial.println(keyboard, BIN);      
    Serial.println();

    if (keyboard & SET)  //setup date and time with keys: SET, PLUS, MINUS
    {
      Serial.println("Set Year");   
      year = now.year();
      editDT(year, 2014, 2100);
   
      Serial.println("Set Month");      
      month = now.month();
      editDT(month, 1, 12);
     
      Serial.println("Set Day");      
      day = now.day();
      editDT(day, 1, 31);
    
      Serial.println("Set Hour");      
      hour = now.hour();
      editDT(hour, 0, 23);      
       
      Serial.println("Set Minutes");      
      minute = now.minute();
      editDT(minute, 0, 60);
       
      Serial.println("Set Seconds");      
      second = now.second();
      editDT(second, 0, 60);   
      RTC.adjust(DateTime(year,month,day,hour,minute,second));  
    }
}

// Use the watchdog to wake the processor from sleep:
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

void setup () {
    Serial.begin(57600);
    //exp_setup();    
    //RTC.begin();   
    // following line sets the RTC to the date & time this sketch was compiled
    //RTC.adjust(DateTime(__DATE__, __TIME__));
    
    // Initialize RFM12B as an 868Mhz module and Node 22 + Group 212:
    rf12_initialize(22, RF12_868MHZ, 212); 
  
    // Initialize gas counter
    payload.gasCounter = 134864;
  
    Serial.println("\n[Gas Meter with Compass, Thermo couple, RTC]");
    state = ACK_LOW_STATE;      
}

void loop () 
{
  ReadWriteDT();

  Serial.println("Going to sleep...");

  // Need to flush the serial before we put the ATMega to sleep, otherwise it
  // will get shutdown before it's finished sending:
  Serial.flush();
  delay(5);
    
  // Power down radio:
  rf12_sleep(RF12_SLEEP);
  
  // Sleep for 5s:
  Sleepy::loseSomeTime(5000);
  
  // Power back up radio:
  rf12_sleep(RF12_WAKEUP);
   
//  Serial.println("Woke up...");
  Serial.flush();
  delay(5);
  
  // Wait until we can send:
  while(!rf12_canSend())                     
    rf12_recvDone();
  
  long t = 0;
  for (int i = 0; i < 100; i++)
    t = t + tplug.anaRead();
  t = t/100;
//  Serial.println(t,DEC);
  
  payload.temp = map((int)t, 0, 1024, 0, 3300); // 10 mv/C 

  payload.count++;
//  Serial.println(payload.count);

    // gas counter
    int compassValue = compass.heading();
        
    switch (state) {
      case HIGH_STATE: {
        if (compassValue < -160) {
          lowSignal = 0;
          state = LOW_STATE;
        }
        break;
      }    
      
      case LOW_STATE: {
        if (compassValue > 0) state = HIGH_STATE;
        else if (compassValue < -160) {
          lowSignal++;
          if (lowSignal > 2)  {
            payload.gasCounter++;
            state = ACK_LOW_STATE;
          }
        }
        break;    
      }
    
      case ACK_LOW_STATE: {
        if (compassValue > 0) state = HIGH_STATE;
        break;
      } 

      default:  {
        state = HIGH_STATE;
        break;
      }
              
    }
    
    Serial.print("compassValue gasCounter state ");
    Serial.print(compassValue);
    Serial.print(' ');
    Serial.print(payload.gasCounter);
    Serial.print(' ');
    Serial.print(state);
    Serial.println();    
    
   // Send every 5th tick
   tick++;
   if (tick >= 5) {
     rf12_sendStart(1, &payload, sizeof payload);
     rf12_sendWait(2);
     tick = 0;
   }
  
  Serial.print("Sent ");
  Serial.println(payload.temp,DEC);    
    
//    delay(3000);
}
