// GxEPD2_HelloWorld.ino by Jean-Marc Zingg

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold18pt7b.h>

#define ESP32

// select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"

// copy the constructor from GxEPD2_display_selection.h or GxEPD2_display_selection_added.h to here
//GxEPD2_BW<GxEPD2_213_B72, GxEPD2_213_B72::HEIGHT> display(GxEPD2_213_B72(/*CS=5*/ SS, /*DC=*/ 22, /*RST=*/ 16, /*BUSY=*/ 21)); // GDEH0213B72 - deluje slabo
//GxEPD2_3C<GxEPD2_213c, GxEPD2_213c::HEIGHT> display(GxEPD2_213c(/*CS=5*/ SS, /*DC=*/ 22, /*RST=*/ 16, /*BUSY=*/ 21)); // GDEW0213Z16 - NE deluje
//GxEPD2_3C<GxEPD2_290c, GxEPD2_290c::HEIGHT> display(GxEPD2_290c(/*CS=5*/ SS, /*DC=*/ 22, /*RST=*/ 16, /*BUSY=*/ 21)); // GDEW029Z10 - NE deluje

void setup()
{
  Serial.begin(115200);
  display.init();
  //delay(10000);
  helloWorld();
  display.hibernate();
}

const char HelloWorld[] = "Hello World!";

void helloWorld()
{
  display.setRotation(3);
  display.setFont(&FreeMonoBold18pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  Serial.println("display.setFullWindow()");
  display.firstPage();
  Serial.println("display.firstPage");
 
  do
  {
    display.fillScreen(GxEPD_WHITE);
    Serial.println("display.fillScreen(GxEPD_WHITE)");
    display.setCursor(x, y);
    display.print(HelloWorld);
  }
  while (display.nextPage());
  Serial.println("--- OUT ---");
}

void loop() {};
