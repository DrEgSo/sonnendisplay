#ifndef oleddisplay_h
#define oleddisplay_h

#include "SSD1306.h"  // OLed SSD1306     4.3.0    for ESP by ThingPulse



// Version 4 - weiße Platine (Sascha)
//SSD1306  display(0x3c, 13, 12);
// Version 5 -rot - D1,D2
SSD1306  display(0x3c, 5, 4);
enum eDisplayType {STD,ERR1,ERR2};

uint8_t uiPixOffset = 0;

void oled_loeschen(){
  display.clear();
  display.display();
}

//__________________________________________________
void initoled() {
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
}

//-------------------------------------------------------------
//                                Oled-Display
//-------------------------------------------------------------
void drawProgressBar(uint8_t uiOffset, int percent) {
  // draw the progress bar
  display.drawProgressBar(0, 53, 120, 10, percent);
  // draw the percentage as String
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(uiOffset + 64, 37, String(percent) + "%");
  display.display();
}


//-------------
void displayinfo(eDisplayType dt){
  switch(dt){
    case STD:
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      char buf[120];
      sprintf(buf, "U: %i F: %.2f B: %i", uiNetzSpng,fNetzFreq,g_iBatterieLeistung); 
      display.drawString(uiPixOffset, 5, buf);
      // zweite Zeile     
      display.setFont(ArialMT_Plain_16);
      sprintf(buf, "V: %i  P: %i", g_uiVerbrauch,g_uiErzeugung);
      display.drawString(uiPixOffset, 20, buf);
 
      drawProgressBar(uiPixOffset, g_uiBatterieLadezustand);
      display.display();
      uiPixOffset++;
      uiPixOffset = uiPixOffset % 8;

    break;
    case ERR1:
    break;
    default:
    break;
  }
}
//----------------------------------
void oleddisplay(const char* c1,const char* c2,const char* c3, bool clear){
      if(clear)
          display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(4, 10, c1);
      display.drawString(4, 30, c2);
      display.drawString(4, 50, c3);
      display.display();
}
void oleddisplay(const char* c1,const char* c2,const char* c3){
  oleddisplay(c1,c2,c3,true);
}
void oleddisplay(const char* c1,const char* c2){
  oleddisplay(c1,c2,"",false);
}

//----------------------------------
void oleddisplay(const char* c1){
      display.clear();
      display.setFont(ArialMT_Plain_16);
      display.drawString(4, 10, c1);
      display.display();
}

#endif
