/* 
 *  Model: QYEG0213RWS800
 *  Resolution: 250x 122
 *  Refresh time: 14S
 *  Color: black, white, red
 *  Interface: SPI
 *  Refresh power consumption: 9.0mW
 *  
#include <DEPG0150BxS810FxX_BW.h>
#include <DEPG0213Bx800FxX_BW.h>
#include <DEPG0290BxS75AFxX_BW.h>
#include <DEPG0290BxS800FxX_BW.h> */
#include <EpdBase.h>
#include <epdif.h> 
#include <e_ink.h> 
#include <e_ink_display.h>
#include <fonts.h>
#include <imagedata.h> 
//#include <lut.h>  
#include <picture.h> 
#include <QYEG0213RWS800_BWR.h>
#include <select.h>

/**
  *This example is the latest 2.13 black and white red ink screen code.
  *Compared with the previous 2.13 black and white red Tri Color ink screen, the biggest change is that the pixel point is changed from 212x104 to 250x122.
  *When selecting this program, please pay attention to whether the ink screen in your hand is the latest 2.13 black and white red ink screen.
  */

#include "QYEG0213RWS800_BWR.h"
#include "picture.h"
#include "select.h"

#define UNDEFINED -1

#define RST_PIN         16  // not used
#define DC_PIN          22  // data/command
#define CS_PIN          5   // SS/CS  (SPI)
#define BUSY_PIN        21  // 
#define CLK_PIN         18  // SCLK (SPI)
// SDI = MOSI (SPI) pin 23

QYEG0213RWS800_BWR epd213bwr(RST_PIN, DC_PIN, CS_PIN, BUSY_PIN, CLK_PIN); //reset_pin, dc_pin, cs_pin, busy_pin, clk_pin
    
void setup() {
    Serial.begin(115200);
      /*
    epd213bwr.EPD_HW_Init(); //Electronic paper initialization
    epd213bwr.EPD_ALL_image(gImage_213black,gImage_213red);	//Refresh the picture in full screen
    epd213bwr.EPD_DeepSleep();  //Enter deep sleep	


    //delay(15000);
    epd213bwr.EPD_HW_Init();
    Serial.println("WhiteScreen_Red");
    epd213bwr.EPD_WhiteScreen_Red();
   
    //delay(15000);
    epd213bwr.EPD_HW_Init();
    Serial.println("WhiteScreen_Black");
    epd213bwr.EPD_WhiteScreen_Black();
  */ 
    //delay(15000);
    epd213bwr.EPD_HW_Init();
    Serial.println("WhiteScreen_White");
    epd213bwr.EPD_WhiteScreen_White(); 
    
    //epd213bwr.EPD_DeepSleep(); 
}
void loop() {

}
