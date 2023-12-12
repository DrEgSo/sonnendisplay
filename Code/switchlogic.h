#ifndef switchlogic_h
#define switchlogic_h
#include "myTypes.h"
#include "tasmota.h"
#include "go_e_charger.h"

#define CHANGE_INHIBIT_TIME 10000      //nur alle 10 Sekunden eine Änderung zulassen

static unsigned long lastOutputChange;


//----------------Zustand für Webbrowser und Display zusammenstellen
void updateExState(){
  // Ausgangszustand für (externe) Anzeige aufbereiten
  g_bAllOutputsOffline_Inactive=true;
  g_bOneOutputOn=false;
  for (int i = 0; i < cfg.anzoutputs; i++) {
    output[i].exState = (OutputxState) output[i].x;
    if ( output[i].bForced) {
      output[i].exState = (OutputxState) ((unsigned int)output[i].exState | (unsigned int)XFRC);
    }
    if (output[i].bLinkLost) {
      output[i].exState = (OutputxState) ((unsigned int)output[i].exState | (unsigned int)XLL);
    }
    else
    {
      if(output[i].eMode != INAKTIV ){
          g_bAllOutputsOffline_Inactive=false;
      }
      if(output[i].x){
          g_bOneOutputOn=true;
      }
    }
  }
}

// --- check if output can be accessed
bool outputAccessible(int idx) {
    bool retval = MY_OK;
    switch (output[idx].typ){
      case GPIOPIN:
      break;
      case TASMOTA:
      case TASMOTA_POWER:
      case GO_E_CHARGER:
        if (millis() > output[idx].nextAccessTime){
          retval = MY_OK;
        }
        else {
          retval = MY_NOT_OK;
        }
      break;
      default:
      break;
    }
    return retval;
}

//-----------------Ausgang schreiben
void outputWrite(int idx, bool x){
//  if(outputAccessible(idx) == MY_OK) {
    // Steigende Flanke
    if(x && !output[idx].x){
      output[idx].startTime=millis();
    }
    switch (output[idx].typ){
      case GPIOPIN:
        digitalWrite(output[idx].pin,x);
        output[idx].x=x;      // Zustand merken
      break;
      case TASMOTA:
      case TASMOTA_POWER:
        tasmotaAccess(idx,tsmWRITE,x);
        if(output[idx].uiLiLoCnt ==0){   // nur wenn Zugriff ohne Fehler
          output[idx].x=x;      // Zustand merken
        }
      break;
      case GO_E_CHARGER:
        Serial.printf("Neuer Wallbox_Zustand: %i\n",x);
        goeAccess(idx,goeWRITEOUTPUT,x);
        if(output[idx].uiLiLoCnt ==0){   // nur wenn Zugriff ohne Fehler
          output[idx].x=x;      // Zustand merken
        }
      break;
      default:
      break;
    }
//  }
}
//---------------Ausgang lesen
bool outputRead(int idx){
  bool retval=false;
  switch (output[idx].typ){
    case GPIOPIN:
      retval= digitalRead(output[idx].pin);
    break;
    case TASMOTA:
    case TASMOTA_POWER:
      retval= tsmReadOutput(idx);
    break;
    case GO_E_CHARGER:
      retval= goeReadOutput(idx);
    break;

    default:
    break;
  }
  return retval;
}

//------------------------------------------------------------------------------------------------
void resetOffTimer(int idx){
  int iLimRef;
  if (output[idx].ref == UEBERSCHUSS){
    iLimRef = g_iUeberschuss;
  }
  if (output[idx].ref == NETZBEZUG){
    iLimRef = g_iNetzEinspeisung;
  }
  
  if (iLimRef > output[idx].iLimitOff && g_uiBatterieLadezustand > output[idx].uiMinBattState) {
    output[idx].stopTime=0;
  }
}


//--------------------------------------------------------------------------------
void switchupdate() {
  unsigned long akttime;
  unsigned long change_inhibit;
  int iLimitReferenz;

  // Reset Stoptimer
  for (int i = 0; i < MAX_OUTPUT_ANZ; i++) {
    resetOffTimer(i);
  }

  change_inhibit=CHANGE_INHIBIT_TIME;
  if(cfg.fTime >0){
    change_inhibit=change_inhibit+(cfg.fTime*FILTER_TIME_MS);
  }
  akttime = millis();
  if (akttime < (lastOutputChange + change_inhibit))
    return;   // erst ein paar Sekunden warten bevor wieder was an den Ausgängen geändert wird
 
  // Ausgänge anschalten - mit max prio (=0) anfangen
  for (int prio = 0; prio <= MIN_PRIO; prio++) {
    for (int i = 0; i < cfg.anzoutputs; i++) {
      if (output[i].ref == UEBERSCHUSS){
        iLimitReferenz = g_iUeberschuss;
      }
      if (output[i].ref == NETZBEZUG){
        iLimitReferenz = g_iNetzEinspeisung;
      }
      if (output[i].uiPrio == prio &&  iLimitReferenz > output[i].iLimitOn && g_uiBatterieLadezustand >= output[i].uiMinBattState  && output[i].xa == OFF && output[i].eMode == AUTOMATIK) {
        //einschalten
        if(cfg.bDebug)
          Serial.printf("Ausgang %i einschalten",i+1);
        output[i].stopTime =0;  
        output[i].startTime = akttime;
        output[i].xa = ON;
        if(!output[i].bForced)
          output[i].x=ON;
        lastOutputChange = akttime;
        outputWrite(i,output[i].x);
        return;                       // nur eine Änderung pro Durchlauf
      }
    }
  }
  // Ausgänge abschalten prüfen - mit min prio anfangen
  for (int prio = MIN_PRIO; prio >= 0; prio--) {
    for (int i = 0; i < cfg.anzoutputs; i++) {
      if (output[i].ref == UEBERSCHUSS){
        iLimitReferenz = g_iUeberschuss;
      }
      if (output[i].ref == NETZBEZUG){
        iLimitReferenz = g_iNetzEinspeisung;
      }
      // Abschalten
      if (output[i].uiPrio == prio && output[i].xa == ON && (iLimitReferenz < output[i].iLimitOff || g_uiBatterieLadezustand < output[i].uiMinBattState) && output[i].eMode == AUTOMATIK ) {
        //Mindestlaufzeit
        if ((akttime - output[i].startTime) > (unsigned long)output[i].uiMinOnTime*60000){
          // Ausschaltverzögerung
          if (output[i].uiOffTimeDelay !=0){
            if(output[i].stopTime==0){
              output[i].stopTime=akttime+(unsigned long)output[i].uiOffTimeDelay*60000;   // Ausschalten vormerken
              Serial.printf("Ausgang %i ausschalten - vormerken für %i ms\n",i+1,output[i].stopTime);
              if(output[i].stopTime==0){
                  output[i].stopTime++;     // Null reservieren für nicht aktiv
              }
            }
            if(akttime > output[i].stopTime){
              output[i].xa = OFF;
              lastOutputChange = akttime;
              if(!output[i].bForced)
                output[i].x=OFF;
              outputWrite(i,output[i].x);
              if(cfg.bDebug)
                Serial.printf("Ausgang %i ausschalten",i+1);
              return;
            }
          }
          else {
            output[i].xa = OFF;
            lastOutputChange = akttime;
            if(!output[i].bForced)
              output[i].x=OFF;
            outputWrite(i,output[i].x);
            if(cfg.bDebug)
              Serial.printf("Ausgang %i ausschalten",i+1);
            return;
          }
        }
      }
    }
  }
}

#endif switchlogic_h
