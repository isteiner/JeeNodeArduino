// Date and time functions using a DS1307 RTC connected via the Ports library
// 2010-02-04 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// Port configuration:
// Port 1:  
//          IO expander http://jeelabs.net/projects/hardware/wiki/Expander_Plug
//          RTC plug http://jeelabs.net/projects/hardware/wiki/RTC_Plug
//          LCD plug http://jeelabs.net/projects/hardware/wiki/LCD_Plug
// Port 2:
//          Compass Board http://jeelabs.org/2012/04/02/meet-the-heading-board-replacement/
// Port 3:
//          Analog input AIO3 (Arduino = Analog 2)
// Port 4:
//          Thermocouple plug http://jeelabs.net/projects/hardware/wiki/Thermo_Plug

#include <JeeLib.h>
#include <Wire.h> // needed to avoid a linker error :(
#include <RTClib.h>
#include <PortsLCD.h>

// keyboard inputs 
#define SET   B00000001
#define PLUS  B00000010
#define MINUS B00000100
#define ESC   B00001000

// IO extender - 5 digital inputs (0-4)
#define IO_INPUTS   0x1F  

// X position for LCD characters
#define LCD_HOUR      0
#define LCD_MIN       3
#define LCD_SEC       6
#define LCD_DAY       9
#define LCD_MONTH    12
#define LCD_YEAR     15

// backlight time in milliseconds
#define MAX_BACKLIGHT_TIME  30000

// charging battery time in milliseconds: 10 hours = 36000000 ms
#define MAX_CHARGING_TIME_MILLIS 36000000

//gas counter states
#define HIGH_STATE     10    // define high state 
#define LOW_STATE      20    // define low state
#define ACK_LOW_STATE  30    // define ack low state

//LCD screen size
#define screen_width 20
#define screen_height 4

// Analog input pin to meassure battery voltage
#define AIO_BATT_PIN 2    //0 for AIO3 on Port3

// Port 1, IO expander
PortI2C myport (1 /*, PortI2C::KHZ400 */);
DeviceI2C expander (myport, 0x20); // also works with output plug if 0x26/0x27

// Port 1, LCD 4x20
PortI2C myI2C (1);
LiquidCrystalI2C lcd (myI2C);

// Port 2, magnetic compass, used for gas meter counting
PortI2C myBus (2);
CompassBoard compass (myBus);

// Port 4, thermocouple sensor plug
Port tplug (4);

unsigned long prevBacklightTime = 0;
unsigned long prevChargingTime = 0;
unsigned char lowSignal = 0;
unsigned char charging = 0;
unsigned char state = HIGH_STATE;
int tick = 0;
int cntCharging = 0;

// Payload data structure:

struct {
    int temp;   // temperature: (tenths)
    unsigned long gasCounter;    // gas counter: (tenths) m3 
    byte count;  // counter
    byte battery;  //battery voltage (tenths)
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
    expander.write(IO_INPUTS); // 5 inputs
//    expander.write(0x2F); // all inputs
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
        keyboard = exp_read() & IO_INPUTS;
        delay(100);
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

// Port 1, RTC
PortI2C i2cBus (1);
RTC_Plug RTC (i2cBus);

DateTime now;

void ReadWriteDT(byte line)
{
    int year, month, day, hour, minute, second;
    byte setDT = false;
    byte keyboard = exp_read() & IO_INPUTS;
    DateTime now = RTC.now();
    if (keyboard & SET) setDT = true;   //setup date and time with keys: SET, PLUS, MINUS
    if (setDT)
    {
      lcd.setCursor(LCD_YEAR, line);
      lcd.blink();     
      nokey();
      keyboard = 0;       
      Serial.println("Set Year");   
      year = now.year();
      while (!(keyboard & SET))
      {
        editValue(year, 2014, 2100);
        DisplayDateTime(year, now.month(), now.day(), now.hour(), now.minute(), now.second(), line);
        lcd.setCursor(LCD_YEAR, line);
        keyboard = exp_read() & IO_INPUTS;
      }
        
      nokey();
      keyboard = 0;    
      Serial.println("Set Month");      
      month = now.month();
      while (!(keyboard & SET))
      {
        editValue(month, 1, 12);
        DisplayDateTime(year, month, now.day(), now.hour(), now.minute(), now.second(), line);
        lcd.setCursor(LCD_MONTH, line);
        keyboard = exp_read() & IO_INPUTS;
      }

      nokey();
      keyboard = 0;          
      Serial.println("Set Day");      
      day = now.day();
      while (!(keyboard & SET))
      {
        editValue(day, 1, 31);
        DisplayDateTime(year, month, day, now.hour(), now.minute(), now.second(), line);
        lcd.setCursor(LCD_DAY, line);
        keyboard = exp_read() & IO_INPUTS;
      } 

      nokey();
      keyboard = 0;                
      Serial.println("Set Hour");      
      hour = now.hour(); 
      while (!(keyboard & SET))
      {
        editValue(hour, 0, 23);     
        DisplayDateTime(year, month, day, hour, now.minute(), now.second(), line);
        lcd.setCursor(LCD_HOUR, line);
        keyboard = exp_read() & IO_INPUTS;
      } 

      nokey();
      keyboard = 0;         
      Serial.println("Set Minutes");      
      minute = now.minute();
      while (!(keyboard & SET))
      {
        editValue(minute, 0, 59);     
        DisplayDateTime(year, month, day, hour, minute, now.second(), line);
        lcd.setCursor(LCD_MIN, line);
        keyboard = exp_read() & IO_INPUTS;
      } 

      nokey();
      keyboard = 0;         
      Serial.println("Set Seconds");      
      second = now.second();   
      while (!(keyboard & SET))
      {
        editValue(second, 0, 59);     
        DisplayDateTime(year, month, day, hour, minute, second, line);
        lcd.setCursor(LCD_SEC, line);
        keyboard = exp_read() & IO_INPUTS;
      } 
      
      RTC.adjust(DateTime(year,month,day,hour,minute,second));  
      lcd.noBlink();
      prevBacklightTime = millis();
      setDT = false;
    }
    else                                 // only display date & time
      DisplayDateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(), line);
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
}

void editValue(int &value, int lolim, int hilim)
{
        byte keyboard = 0; 
        Serial.println(value, DEC);
        keyboard = exp_read() & IO_INPUTS;
        if (keyboard & PLUS)
          value++;
        else if (keyboard & MINUS)  
          value--;      
        if (value > hilim)
           value = lolim;
        if (value < lolim)
           value = hilim;   
        delay(300);  
}

void DisplayDateTime(int year, int month, int day, int hour, int minute, int second, byte line)
{
  if (hour<10)
  {
    lcd.setCursor(LCD_HOUR, line);
    lcd.print(' ');
    lcdPrintFix(hour, LCD_HOUR+1, line);
  }
  else
    lcdPrintFix(hour, LCD_HOUR, line); 
  lcd.print(':');

  if (minute<10)
  {
    lcdPrintFix(0, LCD_MIN, line); 
    lcdPrintFix(minute, LCD_MIN+1, line); 
  }
  else
    lcdPrintFix(minute, LCD_MIN, line);   
  lcd.print(':');

  if (second<10)
  {
    lcdPrintFix(0, LCD_SEC, line); 
    lcdPrintFix(second, LCD_SEC+1, line); 
  }
  else
    lcdPrintFix(second, LCD_SEC, line);     

  // print the current date
  if (day<10)
  {
    lcd.setCursor(LCD_DAY, line);
    lcd.print(' ');    
    lcdPrintFix(day, LCD_DAY+1, line);  
  }
  else
    lcdPrintFix(day, LCD_DAY, line);   
  lcd.print('.');
  
  if (month<10)
  {
    lcdPrintFix(0, LCD_MONTH, line);
    lcdPrintFix(month, LCD_MONTH+1, line);
  }
  else
    lcdPrintFix(month, LCD_MONTH, line);    
  lcd.print('.');
  lcdPrintFix(year, LCD_YEAR, line); 
}  
  
  
void lcdPrintFix(int value, byte cursor_x, byte cursor_y)
{
  lcd.setCursor(cursor_x, cursor_y);
  lcd.print(value);
}

// Use the watchdog to wake the processor from sleep:
ISR(WDT_vect) { Sleepy::watchdogEvent(); }


void setup () {
    Serial.begin(57600);
    exp_setup();    
    //RTC.begin();   
    // following line sets the RTC to the date & time this sketch was compiled
    //RTC.adjust(DateTime(__DATE__, __TIME__));
    
    // Initialize RFM12B as an 868Mhz module and Node 22 + Group 212:
    rf12_initialize(22, RF12_868MHZ, 212); 
  
    // Initialize gas counter
    payload.gasCounter = 248268;
  
    Serial.println("\n[Gas Meter with Compass, Thermo couple, RTC]");
    state = ACK_LOW_STATE;   
 
    // set up the LCD's number of rows and columns: 
    lcd.begin(screen_width, screen_height);
    // Print a message to the LCD.
    lcd.backlight();
    lcd.print("Gas Meter 20141123");   
    delay(1000);
    lcd.clear();
    
    analogReference(DEFAULT);  // analog input reference = 3.6 Volts (AIO = 1023 @ 3.3V)
}

void loop () 
{
  now = RTC.now();

  Serial.println("Going to sleep...");

  // Need to flush the serial before we put the ATMega to sleep, otherwise it
  // will get shutdown before it's finished sending:
  Serial.flush();
  delay(5);
    
  // Power down radio:
  rf12_sleep(RF12_SLEEP);
  
  // Sleep for 5s:
  Sleepy::loseSomeTime(1000);
  
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
  
  // LCD
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
/*  lcd.clear();
  for (int num=0;num<10;num++) {
    for (int y=0;y<screen_height;y++) {
      lcd.setCursor(0,y);
      lcd.print(y);
      lcd.print(')');
      for (int x=2;x<screen_width;x++) {
        lcd.setCursor(x,y);
        lcd.print(num);
      }
    }
    delay(200);
  }
  lcd.clear(); 
  for (int ascii=0;ascii<=screen_width*screen_height;ascii++) {
    lcd.setCursor(ascii % screen_width,ascii / screen_width);
    lcd.print(char(48+ascii)); // type cast the value as a char to force print to display the char, not the value
  }
  lcd.setCursor(0, 0);
  lcd.blink();
  delay(5000);
  lcd.clear(); */
  
  lcd.setCursor(0, 0);
  // print the number of seconds since reset:
  lcd.print("Up time: ");
  lcd.print(millis()/1000);
  lcd.print('s');

  lcd.setCursor(0, 1);
  // print the temperature

//  lcd.print("Temperature: ");
//  lcdPrintFix(payload.temp/10, 13, 1);
//  lcd.print(" C");
  lcd.print(((float)payload.gasCounter)/10,1);
  lcd.setCursor(14, 1);
  lcd.print("K:");
  lcdPrintFix(compassValue, 16, 1);
//  lcd.print(compassValue);
  
  
  // backlight on/off
  if ((millis() - prevBacklightTime) > MAX_BACKLIGHT_TIME) 
  {
    prevBacklightTime = millis();
    lcd.noBacklight();
  }
  if (exp_read() & IO_INPUTS)    // if any key is pressed then switch on backlight
  {
    lcd.backlight();
    prevBacklightTime = millis();
  }
  
  lcd.setCursor(0,2);
  int value = analogRead(AIO_BATT_PIN);
  float batt = map(value, 0, 1023, 0, 660);  // map from 0-1023 to 0-660 (2 * 3.30 = 6.60 Volts with voltage divider)
  payload.battery = batt/100;
  payload.temp = batt/10;  // only temporary for testing battery
  // print battery voltage
  lcd.print("Batt: ");  
  lcd.print((float) batt/100, 2);  
  lcd.print(" V");
  
  // check if battery charging is needed
  if ((batt < 400) && !charging)   
  {
    prevChargingTime = millis();
    charging = true;
    cntCharging++;
    exp_write (0x20);  // switch on battery charging on extension port, GPIO 5
  }
  if (charging && (millis() - prevChargingTime) > MAX_CHARGING_TIME_MILLIS) 
  {
    charging = false;
    exp_write (0x0);  // switch off battery charging on extension port, GPIO 5
  }
  lcd.setCursor(14,2); 
  lcd.print("C:");
  lcd.print(cntCharging);
  
  // print the current date&time in fourth line
  ReadWriteDT(3); 
 
    // Send every 5th tick
   tick++;
   if (tick >= 5) {
     rf12_sendStart(1, &payload, sizeof payload);  
     rf12_sendWait(2);
     tick = 0;
   }
  
  Serial.print("Sent ");
  Serial.println(payload.temp,DEC);   
}
