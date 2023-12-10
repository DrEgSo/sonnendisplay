#ifndef DisplayRGB_h
#define DisplayRGB_h

#include "Adafruit_NeoPixel.h"

void handleServer(void);          //prototyp kann weg wenn idle func für flash übergeben wird @@@@
// Which pin on the Arduino is connected to the NeoPixels?
// D8(GPIO15) for leds
#define PIN   15
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 8

#define UPDATE_MS 100
#define MAX_CYCLE_CNT 14
// #define ANZLICHTSTUFEN 8   // Anzahl Helligkeitsstufen der LEDS
#define LICHTBASIS 3        // kleinste sichtbare  Stufe ist  3
#define LICHTMAX_TAG 9
#define LICHTMAX_NACHT 5
#define MAXPOWER 2000
#define MINPOWERDIFF_FORDISPLAY 50

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

unsigned long rgbChgTime;
bool bDisableOutputShow;

void rgbleds_aus(void){
  pixels.clear();
  pixels.show();
}

void ShowRelaisState() {
  pixels.clear();
  for ( int i = 0; i < cfg.anzoutputs; i++) {
    if (output[i].x == ON)
      if(output[i].bForced){
        pixels.setPixelColor(i, pixels.Color(0, 0, LICHTMAX_NACHT));
      }else
      {
        if(output[i].eMode == MANUELL){
          pixels.setPixelColor(i, pixels.Color(LICHTMAX_NACHT, 0, 0));
        }
        if(output[i].eMode == AUTOMATIK || output[i].eMode == INAKTIV){
          pixels.setPixelColor(i, pixels.Color(0, LICHTMAX_NACHT, 0));
        }
      }
    else {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
  }
  pixels.show();
}

void ShowBattRGB(uint8_t uipercent,uint16_t pv, uint16_t vb, int bat, bool bDischg, bool bChg, uint8_t uianzleds) {
  static long lastAction=0;
  static uint16_t uiCycleCnt=0;
  
  static uint32_t uiOldColor1, uiOldColor2;
  uint8_t  uiAktivePixel, uiOnCycles;	
  uint16_t  uiLumLastPix;
  int i,irest, pwrdiff;

  char puffer[80];
  // letzter Aufruf noch nicht lange genug her, dann raus
  if((millis()-lastAction)<UPDATE_MS)
    return;
  // Durchlaufzeit merken  
  lastAction=millis();
  uiCycleCnt++;
  uiCycleCnt=uiCycleCnt%MAX_CYCLE_CNT;
    
  // berechne Anzahl leuchtender Leds und Helligkeit der letzten
  if(uipercent==0) {
    uiAktivePixel=0;
    uiLumLastPix=0;
  }
  else {
    uiAktivePixel=((uint16_t)uianzleds*(uint16_t)uipercent)/100;
    irest=(((int32_t)uipercent*10)-(int32_t)uiAktivePixel*100*10/(int32_t)uianzleds); // rest in  Prozent
    uiLumLastPix=irest*LICHTMAX_TAG/100;
  }
  // zeige ladezustand
  pixels.clear();
  for(i=0;i<uiAktivePixel;i++){
    pixels.setPixelColor(i, pixels.Color(LICHTMAX_TAG,LICHTMAX_TAG,LICHTMAX_TAG));
  }
  pixels.setPixelColor(uiAktivePixel, pixels.Color(uiLumLastPix,uiLumLastPix,uiLumLastPix));  // letztes pixel des Ladezustands

  // Überschuss berechnen
  pwrdiff=pv-vb;
  uiOnCycles=(abs(pwrdiff)*MAX_CYCLE_CNT)/MAXPOWER;
/*/
  sprintf(puffer,"cnt:%d diff %d oncycl %d",uiCycleCnt, pwrdiff,uiOnCycles);
  Serial.println(puffer);
*/
  // erst ab Mindestdifferenz Ladung/Entladung anzeigen
  if (abs(pwrdiff)>MINPOWERDIFF_FORDISPLAY )
  {
    uiOldColor1=pixels.getPixelColor(0); //Helligkeit merken
    uiOldColor2=pixels.getPixelColor(uianzleds-1); //Helligkeit merken
    
    //------Ladung? letzte LED grün
    if(pwrdiff>0 )
    {
      if(uiCycleCnt<=uiOnCycles)  //anteilig anschalten
      {
        if(bChg || uipercent==100) //Batterie wird geladen
        {
          pixels.setPixelColor(uianzleds-1, pixels.Color(0,3*LICHTMAX_TAG,0));
        }
        else  // Überschuss geht ins Netz
        {
          pixels.setPixelColor(uianzleds-1, pixels.Color(0,0,3*LICHTMAX_TAG));
        }
      }
      else
      {
        pixels.setPixelColor(uianzleds-1, uiOldColor2);
      }
    }
    if((pwrdiff<0)&&(uipercent>0)) // Verbrauchen
    {
      if(uiCycleCnt<=uiOnCycles)  //anteilig anschalten
      {
        if(bDischg) //Batterie wird entladen
        {
           pixels.setPixelColor(0, pixels.Color(2*LICHTMAX_TAG,0,0));
        }
        else // Verbrauch kommt aus dem Netz = blau
        {
           pixels.setPixelColor(0, pixels.Color(0,0,2*LICHTMAX_TAG));
        }
      }
      else
      {
        pixels.setPixelColor(0,uiOldColor1);
      }
    }
  }
  pixels.show();
}
//-----------------------------------Anzeige von Ausgängen oder Batterie
void ShowRGB(){
  unsigned long now=millis();
  if(cfg.anzoutputs ==0 || !g_bOneOutputOn){
    bDisableOutputShow=true;
  }
  else {
    bDisableOutputShow=false;
  }
  if(g_bDispBattinfo || bDisableOutputShow){
     ShowBattRGB(g_uiBatterieLadezustand, g_uiErzeugung, g_uiVerbrauch, g_iBatterieLeistung, bBatteryDischarging, bBatteryCharging, NUMPIXELS);
  }
  else {
     ShowRelaisState();
  }
}






//-----------------------------------------------------------------------------------
void SequenzFlash(uint8_t red, uint8_t green, uint8_t blue, bool srvactive){
  uint32_t uiOldColor;
  for(int j=0;j<NUMPIXELS;j++){
    uiOldColor=pixels.getPixelColor(j%NUMPIXELS);
    pixels.setPixelColor(j%NUMPIXELS,pixels.Color(red,green,blue));
    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(1);
    pixels.setPixelColor(j%NUMPIXELS,uiOldColor);
    pixels.show();
    unsigned long endTime=millis()+20;
    while (millis()<endTime){
      if(srvactive){
        handleServer();
      }
      delay(1);  
    }
 }
}

void SequenzFlash(uint8_t red, uint8_t green, uint8_t blue){
  SequenzFlash( red, green, blue, false);
}
#endif
