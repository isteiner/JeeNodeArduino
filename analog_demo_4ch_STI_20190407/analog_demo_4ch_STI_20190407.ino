/// @dir analog_demo
/// Demo readout of the 4-channel 18-bit MCP3424 on the Analog Plug v2.
/// @see http://jeelabs.org/2012/04/13/analog-plug-readout/
// 2009-09-28 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
// |Resolution Setting | Data Rate | Minimum Code  | Maximum Code
// |     12            |  240 SPS  | -2048         |  2047
// |     14            |  60 SPS   | -8192         |  8191
// |     16            |  15 SPS   | -32768        |  32767
// |     18            |  3.75 SPS | -131072       |  131071

// 2019-04-06 <isteiner> corrected for negative values 
// 2019-07-21 include avrage calculation
// 2020-11-15 include linearization calculation to improve precission of ADC

#include <JeeLib.h>

#define N  50   // set number of measurements to avearage value

PortI2C myI2C (1);
AnalogPlug adc (myI2C, 0x68);

// Payload data structure:
struct {
    int AD_Channel[4];           
} payload;

static void AP2init (DeviceI2C& dev, byte mode =0x1C) {
    // mode =0x10  is channel 1, continuous, 12-bit, gain x1
    // mode =0x14  is channel 1, continuous, 14-bit, gain x1
    // mode =0x18  is channel 1, continuous, 16-bit, gain x1
    // mode =0x1C  is channel 1, continuous, 18-bit, gain x1    
    // 'mode' = MCP3424 8-bit configuration register
    // see sec. 5.2 Configuration Register, p. 18 of datasheet
    // b7  b6  b5  b4    b3  b2  b1  b0  ----- (bit number)
    // RDY C1  C0  O/C   S1  S0  G1  G0  ----- (bit name)
    // --------------------------------
    // RDY = 0 when ready (or in one-shot, write '1' to start conversion)
    // C1/C0: input channel # 00 = 0, 01 = 1, 10 = 2, 11 = 3
    // O/C: 1 = continuous conversion mode, 0 =  single-shot conversion
    // S1/S0: rate: 00 = 240 Hz (12 b), 01 = 60 Hz (14 b) 10 = 15 Hz (16 b) 11 = 3.75 Hz (18 b)
    // G1/G0: gain: 00 = x1, 01 = x2, 10 = x4, 11 = x8    FullScale: 2.048 V, 1.024, 0.512, 0.256        
    dev.send();
    dev.write(mode);
    dev.stop();
}

static long AP2read (DeviceI2C& dev, byte channel) {
    adc.select(channel);
    //adc.begin(0x14);
    delay(300); // 270 ms = 3.75 readings/sec max for 18-bit resolution
                //   5 ms = 240  readings/sec max for 12-bit resolution
    dev.receive();  
    long raw = (long) dev.read(0) << 16;
    raw |= (word) dev.read(0) << 8;     
    raw |= dev.read(0);
    if (raw > 0x01ffff) raw = -((~raw & 0x1ffff)+1);  // calculate negative value for 18 bit values
 
    byte status = adc.read(1);
    return raw;
}

float Linearization(float adc_value, float a, float b, float c) {
    // linearization with quadraric polynomial
    return a*a*adc_value+b*adc_value+c;
}

long LinearizationInt(long adc_value, long a, long b, long c) {
    // linearization with quadraric polynomial: y a*x*x + b*x + c
    // using long integer arithmetic (32 bits)
    byte neg_value = false;
    if (adc_value < 0) {
      neg_value = true;
      adc_value = -adc_value;
    }
    long temp = adc_value/100;
    temp = temp*temp*a + b*adc_value + c;
    if (neg_value) return -temp/10000;
    else return temp/10000;
}

void setup () {
    Serial.begin(57600);
    Serial.println("\n[analog_demo]");
    if (adc.isPresent())
      Serial.println("Analog Plug found");    
    // Initialize RFM12B as an 868Mhz module and Node 19 + Group 212:
    rf12_initialize(19, RF12_868MHZ, 212); 
    //AP2init(adc);
}

void loop () {
    for (byte i=1; i <= 4; ++i) {
      long val = 0;
      long lin;
      for (byte n=0; n < N; ++n) {
        // average values in N steps
        long ADread = AP2read(adc, i);           
        val = val + ADread; 
      }
      val = val/N;
      // see attached excel with polynomial regression to linearize ADC
      //float lin = Linearization(val, -0.00000003, 0.0802, 65.961);  // AVR processor has only float (and not DOUBLE!!!) precision which is insufficient in this case!!!
      //therefore long integer arithmetic is used for ADC linearization for every channel separately
      if (i == 1) lin = LinearizationInt(val, -3L, 802L, 659610L); // CH1 = +- 10100 mV AC1 
      if (i == 2) lin = LinearizationInt(val, -5L, 487L, 464760L); // CH2 = +- 5003 mV  VBAT
      if (i == 3) lin = LinearizationInt(val, -5L, 494L, 485460L); // CH2 = +- 5061 mV  VOUT    
      if (i == 4) lin = LinearizationInt(val,  0L, 157L, -19289L); // CH4 = +- 2050 mV  ICHARGE          
      payload.AD_Channel[i-1] = (int)lin; 

      //Serial.print(val);  
      //Serial.print(' ');
      Serial.print(lin);  
      Serial.print(' ');
    }
    
    delay(1);    
    rf12_sendStart(1, &payload, sizeof payload);
    rf12_sendWait(2);
    Serial.println();
    delay(1);
}
