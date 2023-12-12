#ifndef mydisplay_h
#define mydisplay_h


//#define NO_DISPLAY      // wenn kein Display vorhanden

#ifdef NO_DISPLAY         // leere Aufrufe
void mydisplay_clear(){
  return;
}
void drawProgressBar(uint8_t uiOffset, int percent) {
  return;
}
void drawOutputs(uint8_t uiOffset) {
  return;
}
void mydisplay_init() {
  return;
}
void mydisplayinfo(eDisplayType dt){
  return;
}
void mydisplay_show(const char* c1,const char* c2,const char* c3, const char* c4){
  return;
}
void mydisplay_show(const char* c1,const char* c2,const char* c3, bool clear){
  return;
}
void mydisplay_show(const char* c1,const char* c2,const char* c3){
  return;
}
void mydisplay_show(const char* c1,const char* c2){
  return;
}
void mydisplay_show(const char* c1,const char* c2, bool cflag){
  return;
}
void mydisplay_show(const char* c1){
  return;
}
//--------------------------ende von kein Display
#else
//------------------------------------------------------------------------------------------

#include "oleddisplay.h"
#define DISP_CHANGE_CNT 3

void drawProgressBar(uint8_t , int ); 

unsigned int uiDispMode =0;
bool bChangeMode=false;
unsigned int uiDispCnt=0;


//--------------------------------------------------------------
void mydisplay_clear(){
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.display();
}
//-------------------------------------------------------------
//
void drawOutputs(uint8_t uiOffset) {
  // draw circles
  #define RADIUS  8
  #define ABSTAND 2
  #define YPOS 52

  int x_Offset=RADIUS+ABSTAND;
  
  for (int i=0;i<cfg.anzoutputs;i++){
    if(output[i].bLinkLost){ 
      display.setFont(ArialMT_Plain_16);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(x_Offset+uiOffset,(YPOS-8),"X");
    }
    else {
      int radius = RADIUS;
      if(output[i].eMode == INAKTIV){
        radius=radius/3;
      }
      display.drawCircle(x_Offset+uiOffset,YPOS, radius);
      if(output[i].x ){
        display.fillCircle(x_Offset+uiOffset, YPOS, radius);
      }
    }
    x_Offset+=2*RADIUS+ABSTAND;
  }
  display.display();
}

//__________________________________________________
void mydisplay_init() {
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
}

//-------------
void mydisplayinfo(eDisplayType dt){
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
      uiDispCnt++;
      if(uiDispCnt >= DISP_CHANGE_CNT){
        uiDispCnt=0;
        bChangeMode=true;
      }
      switch(uiDispMode){
        case 0:
          if(bChangeMode){
            bChangeMode=false;
            if(cfg.anzoutputs >0 && g_bOneOutputOn){
                uiDispMode++;
            }
          }
          drawProgressBar(uiPixOffset, g_uiBatterieLadezustand);
          g_bDispBattinfo=true;
        break;
        case 1:
          if(bChangeMode){
            bChangeMode=false;
            uiDispMode=0;
          }
          drawOutputs(uiPixOffset);
          // nur anzeigen wenn mindestens ein Ausgang ein
          if(g_bOneOutputOn){
            g_bDispBattinfo=false;
          }
        break;
        default:
        break;
      }
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
//----------------------------------
void mydisplay_show(const char* c1,const char* c2,const char* c3, const char* c4)
{
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(4, 10, c1);
      display.drawString(4, 24, c2);
      display.drawString(4, 36, c3);
      display.drawString(4, 48, c4);
      display.display();
}

void mydisplay_show(const char* c1,const char* c2,const char* c3, bool clear){
      if(clear)
          display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(4, 10, c1);
      display.drawString(4, 30, c2);
      display.drawString(4, 50, c3);
      display.display();
}
void mydisplay_show(const char* c1,const char* c2,const char* c3){
  mydisplay_show(c1,c2,c3,true);
}
void mydisplay_show(const char* c1,const char* c2){
  mydisplay_show(c1,c2,"",false);
}
void mydisplay_show(const char* c1,const char* c2, bool cflag){
  mydisplay_show(c1,c2,"",cflag);
}

//----------------------------------
void mydisplay_show(const char* c1){
      display.clear();
      display.setFont(ArialMT_Plain_16);
      display.drawString(4, 10, c1);
      display.display();
}
#endif
#endif
