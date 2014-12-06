// Gas meter with LCD, RTC, Thermocouple nad Compass (to detect magnet field in counter)
// Igor Steiner & JeeLib libraries http://jeelabs.net/pub/docs/jeelib/
// 2014-11-29

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

// Time constants
#define MINUTES_PER_DAY  1440
#define MINUTES_PER_YEAR  525600

unsigned int minutesPerMonth[13]={0, 44640, 40320, 44640, 43200, 44640, 43200, 44640, 44640, 43200, 44640, 43200, 44640};
unsigned long powerOfTen[8] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000};

// X position for LCD characters
#define LCD_HOUR      0
#define LCD_MIN       3
#define LCD_SEC       6
#define LCD_DAY       9
#define LCD_MONTH    12
#define LCD_YEAR     15

// LCD screen numbers
#define LCD_MAIN            0
#define LCD_CUMULATIVES     1
#define LCD_COST            2
#define LCD_PRICE           3
#define LCD_PROGNOSTIC      4
#define LCD_DIAGNOSTIC      5
#define LCD_SETUPGAS        6
#define LCD_SETUPDATE       7
#define LCD_LOCKING         8

// backlight time in milliseconds
#define MAX_BACKLIGHT_TIME  30000

// time to jump on next/previous screen after key is pressed in milliseconds
#define MAX_KEY_TIME 2000

// time to lock settings in milliseconds: 10 minutes = 600000
#define MAX_UNLOCK_TIME 600000

// charging battery time in milliseconds: 10 hours = 36000000 ms
#define MAX_CHARGING_TIME_MILLIS 36000000

//gas counter states (to detect gas counter increment)
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

byte lcdScreen = 0;
unsigned long prevKeyTime = 0;
unsigned long prevBacklightTime = 0;
unsigned long prevChargingTime = 0;
unsigned long prevLockedTime = 0;
unsigned char lowSignal = 0;
unsigned char charging = 0;
unsigned char state = HIGH_STATE;
int tick = 0;
int cntCharging = 0;
byte locked = 0;
int tickLock = 0;

// Payload data structure:
struct {
    int temp;   // temperature: (tenths)
    unsigned long gasCounter;    // gas counter: (tenths) m3 
    byte count;  // counter
    byte battery;  //battery voltage (tenths)
    unsigned int gasPrevDay;      // yesterday consumption: (tenths) m3
    unsigned int gasPrevMonth;   // previous month consumption: (tenths) m3
} payload;

// gas counter  and calculated values
struct {
  unsigned long counter;         // gas counter: (tenths) m3
  byte cumulThisHour;   // this hour consumption: (tenths) m3  
  byte cumulPrevHour;   // previous hour consumption: (tenths) m3   
  unsigned int cumulThisDay;     // today consumption: (tenths) m3
  unsigned int cumulPrevDay;     // yesterday consumption: (tenths) m3  
  unsigned int cumulThisMonth;   // this month consumption: (tenths) m3  
  unsigned int cumulPrevMonth;   // previous month consumption: (tenths) m3    
  unsigned int cumulThisYear;   // this year consumption: (tenths) m3    
  unsigned int cumulPrevYear;   // previous year consumption: (tenths) m3    
  unsigned int costThisMonth;   // this month cost: EUR
  unsigned int costPrevMonth;   // previous month cost: EUR  
  unsigned int costThisYear;   // this year cost: EUR  
  unsigned int costPrevYear;   // previous year cost: EUR  
  float priceVarM3;           // gas price per m3: EUR  
  float priceFixMonth;         // fix price per month: EUR  
  unsigned int prognoseToday;  // prognose for today consumption: (tenths) m3  
  unsigned int prognoseThisMonth;  // prognose for this month consumption: (tenths) m3    
  unsigned int prognoseThisYear;  // prognose for this year consumption: (tenths) m3     
} gas;

// previous RTC values
struct {
  byte hour;
  byte day;
  byte month;
  int year;
} oldRTC;
  
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

// Extension writing
static void exp_write (byte value) {
    expander.send();
    expander.write(MCP_GPIO);
    expander.write(value);
    expander.stop();
}

// Extension reading
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


// Calculate number of digits of unsigned long integer 
byte DigitsOfInteger(unsigned long value)
{
  byte digits = 1;
  for (byte i=8; i>0; i--)
    if (value < powerOfTen[i]) digits = i;
  return digits;
}

// Set gas counter value
void SetGasCounter(unsigned long &gasCounter, byte line)
{
  byte row = 8;  
  byte shiftLeft = 0;
  lcd.setCursor(row, line);
  lcd.print(((float)gas.counter)/10,1);
  lcd.setCursor(17, line);  
  lcd.print(" m3");

  if (exp_read() & SET & !locked)   //setup gas counter value with keys: SET, PLUS, MINUS
  {
    lcd.blink();       
    for (int i=0; i<8; i++)
    {
      nokey();
      if (i>0) shiftLeft = 1;  // shift cursor additional 1 place for decimal point
      lcd.setCursor(row+DigitsOfInteger(gasCounter)-i-shiftLeft, line);
      while (!(exp_read() & SET))
      {
        if (exp_read() & PLUS) 
        {
          gasCounter += powerOfTen[i];
          lcd.setCursor(row, line);
          lcd.print(((float)gasCounter)/10,1);
          lcd.setCursor(row+DigitsOfInteger(gasCounter)-i-shiftLeft, line);
          delay(1000);
        }
        if (exp_read() & MINUS) 
        {
          gasCounter -= powerOfTen[i];
          lcd.setCursor(row, line);
          lcd.print(((float)gasCounter)/10,1); 
          lcd.setCursor(row+DigitsOfInteger(gasCounter)-i-shiftLeft, line);          
          delay(1000);
        }  
      }
      nokey();
     } 
     lcd.noBlink();
  }
}   
  
void ReadWriteDT(byte line)
{
    int year, month, day, hour, minute, second;
    byte setDT = false;
    byte keyboard = exp_read() & IO_INPUTS;
    DateTime now = RTC.now();
    if (keyboard & SET & !locked) setDT = true;   //setup date and time with keys: SET, PLUS, MINUS
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
        editValue(year, 2000, 2100);
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
   
    Serial.println("\n[Gas Meter Station]");
    state = ACK_LOW_STATE;   
    analogReference(DEFAULT);  // analog input reference = 3.6 Volts (AIO = 1023 @ 3.3V)
    // set up the LCD's number of rows and columns: 
    lcd.begin(screen_width, screen_height);
    // Print a message to the LCD.
    lcd.backlight();
    lcd.print("Gas Meter 20141207");   
    delay(2000);

    // Initialize gas counter per 30.11.2014
    gas.counter = 250326;
 
    gas.cumulThisHour = 0;
    gas.cumulPrevHour = 0;    
    gas.cumulThisDay = 0;
    gas.cumulPrevDay = 150;
    gas.cumulThisMonth = 944;
    gas.cumulPrevMonth = 3864;    
    gas.cumulThisYear = 24508;   
    gas.cumulPrevYear = 36100; 

    now = RTC.now();
    oldRTC.hour = now.hour();
    oldRTC.month = now.month();    
    oldRTC.year = now.year();   
    
    gas.priceVarM3 = 0.6691456;    //per 30.11.2014 Petrol: EUR/m3
    gas.priceFixMonth = 12.44;     //per 30.11.2014 Petrol: EUR/month
  
    oldRTC.hour = now.hour();
    oldRTC.day = now.day();
    oldRTC.month = now. month();
    oldRTC.year = now.year();
    
    //Setup time and date
    lcd.clear(); 
    while (!(exp_read() & PLUS))
    {   
      lcd.setCursor(0, 0);    
      lcd.print("Ura in datum: ");  
      // print and set the current date&time in second line
      ReadWriteDT(1);    
      lcd.setCursor(0, 3);    
      lcd.print("Pritisni + za naprej");  
    }
    nokey();
    //Setup gas counter
    lcd.clear(); 
    while (!(exp_read() & PLUS))
    {   
      lcd.setCursor(0, 0);    
      lcd.print("Stevec plina: ");  
      lcd.setCursor(0, 3);    
      lcd.print("Pritisni + za naprej");  
      // print and set the current gas counter value in second line
      SetGasCounter(gas.counter,1);          
    }
    locked = 1;  
    prevBacklightTime = millis();  
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
  
  // Sleep for 1s:
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
  for (int i = 0; i < 500; i++)
    t = t + tplug.anaRead();
  t = t/500;
//  Serial.println(t,DEC);
  
  payload.temp = map((int)t, 0, 1024, 0, 3300); // 10 mv/C 

  payload.count++;
  //  Serial.println(payload.count);
  
  // Check battery voltage
  int value = analogRead(AIO_BATT_PIN);
  float batt = map(value, 0, 1023, 0, 660);  // map from 0-1023 to 0-660 (2 * 3.30 = 6.60 Volts with voltage divider)
  payload.battery = batt/100;
   
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
          if (lowSignal > 2)  {        // this is new tenth of m3 gas
             gas.counter++;
             gas.cumulThisHour++;
             gas.cumulThisDay++;
             gas.cumulThisMonth++;
             gas.cumulThisYear++;
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
    
    Serial.print("compassValue gas.counter state ");
    Serial.print(compassValue);
    Serial.print(' ');
    Serial.print(gas.counter);
    Serial.print(' ');
    Serial.print(state);
    Serial.println();    
 
 // check change of time and date and calculate new cummulative values
 if (now.hour() != oldRTC.hour) {
   oldRTC.hour = now.hour();
   gas.cumulPrevHour = gas.cumulThisHour;
   gas.cumulThisHour = 0;
  }
 
  if (now.day() != oldRTC.day) {
   oldRTC.day = now.day();
   gas.cumulPrevDay = gas.cumulThisDay;
   gas.cumulThisDay = 0;
  }  

  if (now.month() != oldRTC.month) {
   oldRTC.month = now.month();
   gas.cumulPrevMonth = gas.cumulThisMonth;
   gas.cumulThisMonth = 0;
  }  

  if (now.year() != oldRTC.year) {
   oldRTC.year = now.year();
   gas.cumulPrevYear = gas.cumulThisYear;
   gas.cumulThisYear = 0;
  }    
 
     // show on LCD 
     switch (lcdScreen) {
      case LCD_MAIN: {      
 /*       lcd.setCursor(0, 0);
        lcd.print("Stevec: ");
        lcd.print(((float)gas.counter)/10,1);
        lcd.print(' m3'); */
      
        lcd.setCursor(0, 0);    
        lcd.print("To uro: ");
        lcd.print(((float)gas.cumulThisHour)/10,1);
        lcd.print(" m3");        
        Serial.print("cumulThisHour ");
        Serial.print(gas.cumulThisHour);     
        Serial.println();
        
        lcd.setCursor(0, 1);    
        lcd.print("Prej.uro: ");
        lcd.print(((float)gas.cumulPrevHour)/10,1);
        lcd.print(" m3");         
        
        lcd.setCursor(0, 2);    
        lcd.print("Danes: ");
        lcd.print(((float)gas.cumulThisDay)/10,1);
        lcd.print(" m3");        
        
        lcd.setCursor(0, 3);    
        lcd.print("Vceraj: ");
        lcd.print(((float)gas.cumulPrevDay)/10,1);
        lcd.print(" m3");          
        }
        break;
      
      case LCD_CUMULATIVES: {
        lcd.setCursor(0, 0);    
        lcd.print("Ta mesec: ");
        lcd.print(((float)gas.cumulThisMonth)/10,1);
        lcd.print(" m3");        
        
        lcd.setCursor(0, 1);    
        lcd.print("Prej.mesec: ");
        lcd.print(((float)gas.cumulPrevMonth)/10,1);
        lcd.print(" m3");         
        
        lcd.setCursor(0, 2);    
        lcd.print("Letos: ");
        lcd.print(((float)gas.cumulThisYear)/10,1);
        lcd.print(" m3");        
        
        lcd.setCursor(0, 3);    
        lcd.print("Prej.leto: ");
        lcd.print(((float)gas.cumulPrevYear)/10,1);
        lcd.print(" m3");          
        }
        break;        
        
       case LCD_COST: {
        lcd.setCursor(0, 0);    
        lcd.print("Cena ta mes: ");
        lcd.print(((float)gas.cumulThisMonth)*gas.priceVarM3/10+gas.priceFixMonth,0);
        lcd.print(" EUR");        
        
        lcd.setCursor(0, 1);    
        lcd.print("Cena prej.me: ");
        lcd.print(((float)gas.cumulPrevMonth)*gas.priceVarM3/10+gas.priceFixMonth,0);
        lcd.print(" EUR");         
        
        lcd.setCursor(0, 2);    
        lcd.print("Cena letos: ");
        lcd.print(((float)gas.cumulThisYear)*gas.priceVarM3/10+gas.priceFixMonth*now.month(),0);
        lcd.print(" EUR");        
        
        lcd.setCursor(0, 3);    
        lcd.print("Prej.leto: ");
        lcd.print(((float)gas.cumulPrevYear)*gas.priceVarM3/10+gas.priceFixMonth*12,0);
        lcd.print(" EUR");          
        }
        break;  
   
      case LCD_PRICE: {
        lcd.setCursor(0, 0);    
        lcd.print("Cena m3: ");
        lcd.print((float)gas.priceVarM3,4);
        lcd.print(" EUR"); 
        
        lcd.setCursor(0, 1);    
        lcd.print("Fiksni st: ");
        lcd.print((float)gas.priceFixMonth,2);
        lcd.print(" EUR");         
        }
        break;   

      case LCD_PROGNOSTIC: {
        unsigned int todayActualMinutes = now.hour()*60+now.minute()+1;
        unsigned long thisMonthActualMinutes = (now.day()-1)*MINUTES_PER_DAY+todayActualMinutes;
        unsigned long thisYearActualMinutes = 0;
        for (int i = 1; i < now.month(); i++)
          thisYearActualMinutes += minutesPerMonth[i];
        thisYearActualMinutes += thisMonthActualMinutes;   
          
        lcd.setCursor(0, 0);    
        lcd.print("Prog.danes: ");
        lcd.print(((float)gas.cumulThisDay)*MINUTES_PER_DAY/todayActualMinutes/10,0);
        lcd.print(" m3");        
        
        lcd.setCursor(0, 1);    
        lcd.print("Prog.mesec: ");
        lcd.print(((float)gas.cumulThisMonth)*minutesPerMonth[now.month()]/thisMonthActualMinutes/10,0);
        lcd.print(" m3");         
        
        lcd.setCursor(0, 2);    
        lcd.print("Prog.leto: ");
        lcd.print(((float)gas.cumulThisYear)*MINUTES_PER_YEAR/thisYearActualMinutes/10,0);
        lcd.print(" m3"); 

        lcd.setCursor(0, 3);    
        lcd.print("Prog.mesec: ");
        lcd.print((((float)gas.cumulThisMonth)*minutesPerMonth[now.month()]/thisMonthActualMinutes/10)*gas.priceVarM3+gas.priceFixMonth,0);
        lcd.print(" EUR");                 
        }
        break;
 
      case LCD_DIAGNOSTIC: {
        lcd.setCursor(0, 0);
        // print the number of seconds since reset:
        lcd.print("Deluje: ");
        lcd.print(millis()/1000);
        lcd.print(" s");
      
        lcd.setCursor(0, 1);
        lcd.print(((float)gas.counter)/10,1);
        lcd.setCursor(14, 1);
        lcd.print("K:");
        lcdPrintFix(compassValue, 16, 1);
        // print battery voltage
        lcd.setCursor(0,2);
        lcd.print("Batt: ");  
        lcd.print((float) batt/100, 2);  
        lcd.print(" V");
      
        lcd.setCursor(14,2); 
        lcd.print("C:");
        lcd.print(cntCharging);
              
        lcd.setCursor(0, 3);    
        lcd.print("Temperatura: ");
        lcd.print((float)payload.temp/10,1);
        lcd.print(" C");                          
        }
        break;   
    
      case LCD_SETUPGAS: {
        lcd.setCursor(0, 0);    
        lcd.print("Stevec plina: ");  
        // print and set the current gas counter in third line
        SetGasCounter(gas.counter,2);        
        }
        break;    
    
      case LCD_SETUPDATE: {
        lcd.setCursor(0, 0);    
        lcd.print("Ura in datum: ");  
        // print and set the current date&time in third line
        ReadWriteDT(2);             
        }
        break;        
  
      case LCD_LOCKING: {
        lcd.setCursor(0, 0);    
        lcd.print("Odklepanje");      
        lcd.setCursor(0, 1);    
        lcd.print("Drzi SET tipko 5 sek");         
        lcd.setCursor(0, 3);         
        lcd.print("Trenutno: "); 
        if (locked) lcd.print("zaklenjeno");
        else lcd.print("odklenjeno");
        // if SET key is press 5 seconds then lock/unlock settings
        if (exp_read() & SET) {
          tickLock++;
          if (tickLock > 5) {
            tickLock = 0;
            if (locked) {
              locked = 0;
              lcd.clear();
              lcd.setCursor(6, 2);    
              lcd.print("ODKLENJENO");
              prevLockedTime = millis();
            }
            else {
              locked = 1;
              lcd.clear();
              lcd.setCursor(6, 2);    
              lcd.print("ZAKLENJENO");        
            }
          }
         }               
        }
        break;       
        
      default:  {
        lcdScreen = LCD_MAIN;
        break;        
      } 
     }
  
  // if + or - keey is press 2 seconds go to next/previous screen
  if (exp_read() & (PLUS | MINUS)) {
    if ((millis() - prevKeyTime) > MAX_KEY_TIME) {
      prevKeyTime = millis();
      if (exp_read() & PLUS)  lcdScreen++;
      if (exp_read() & MINUS) lcdScreen--;
      lcd.clear();
    }
  }
 
  // lock settings after unlocked and time is expired
  if (((millis() - prevLockedTime)> MAX_UNLOCK_TIME) & !locked) {
      prevLockedTime = millis();
      locked = 1;
      lcd.clear();
      lcd.setCursor(6, 2);    
      lcd.print("ZAKLENJENO"); 
  }
            
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
  
    // Send every 10th tick
   tick++;
   if (tick >= 10) {
     payload.gasCounter = gas.counter;
     //payload.temp = batt/10;  // only temporary to test battery   
     payload.battery = batt/10;  //battery voltage (tenths)
     payload.gasPrevDay = gas.cumulPrevDay;      // yesterday consumption: (tenths) m3
     payload.gasPrevMonth = gas.cumulPrevMonth;   // previous month consumption: (tenths) m3
     
     rf12_sendStart(1, &payload, sizeof payload);  
     rf12_sendWait(2);                             
     tick = 0;
   }
  
  Serial.print("Sent ");
  Serial.println(payload.temp,DEC);   
}
