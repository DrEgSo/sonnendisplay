#ifndef go_e_charger_h
#define go_e_charger_h

#include "myWifi.h"
void webDelay(unsigned long); //prototyp aus myWifi
void handleServer(void);    //prototyp aus myWifi
#include "myUtils.h"

#define GOE_MAXLENGTH 2048
#define GOE_RESPONSE_DELAY 3000 
#define GOE_CONTROL_DELAY 20000   //Zeitverzögerung bevor die Stromregelung einsetzt 

#define GOE_PWR_RESERVE 100
#define GOE_PWR_MIN 1380
#define GOE_PWR_MAX 22080
#define GOE_PWR_INKREMENT 40
#define GOE_MAX_AMPS 32

enum eGOE_CMD {goeWRITEOUTPUT,goeWRITEPOWER,goeREADSTATUS};

int ExtractGoeInfo( char* json , bool *x, int *pwr, int anzphasen) {
  // Allocate JsonBuffer
  #define GOE_JSONSIZE 2800
  DynamicJsonDocument jsondoc(GOE_JSONSIZE);

//  Serial.println(json);
  // Parse JSON object
  DeserializationError  error = deserializeJson(jsondoc, json);
//  Serial.print("Speicherverbrauch für das JSON Objekt: ");
//  Serial.println(jsondoc.memoryUsage()); 
  jsondoc.shrinkToFit(); 
  // Get a reference to the root object
  JsonObject root = jsondoc.as<JsonObject>();

  if (error) {
    Serial.print(F("Parsing Go-e Info failed! : "));
    Serial.println(error.f_str());
    return MY_ERROR;
  }
  else  // json auslesen
  {
    JsonVariant on_state = root["alw"].as<JsonVariant>();      
    if(!on_state.isNull()){
      Serial.printf("ALW Zustand: %i\n",on_state.as<int>());
      *x = on_state.as<int>();
    }
    JsonVariant totpwr = root["nrg"][11].as<JsonVariant>();                   
    if(!totpwr.isNull()){
      Serial.printf("Gesamtleistung: %i\n",totpwr.as<int>());
      *pwr = totpwr.as<int>()*10;
    }
  }
  return MY_OK;
}

//----------------- Zugriff auf Go-e Wallbox
// Rückgabewert ist die Ladefreigabe als Ausgangszustand
int goeAccess (int idx, eGOE_CMD cmd, uint8_t x) {
  static unsigned long stopTime;
  unsigned int charcnt;
  char tmp[128];
  char ip[20];
  char JsonResponse[GOE_MAXLENGTH];
  
  outputData_t* pX = &output[idx];
  WiFiClient client;
  IP4_Bytes_to_String(ip, pX->ip, 10);  // IP Adresse als ASCII erzeugen

  if (!client.connect(pX->ip, 80)) {
    Serial.printf("Verbindung zu %s einmal nicht möglich\n",pX->strOutputname);
    delay(1);
    pX->uiLiLoCnt++;
    pX->nextAccessTime=millis()+(8*IP_UPDATE_RATE_MS) + (idx*6000);

    if(pX->uiLiLoCnt >= IP_LINK_LOSS_CNT_LIMIT){
      memset(tmp, '\0', sizeof(tmp));
      strcpy(tmp,"Verbindung zu(r) ");
      strcat(tmp,pX->strOutputname);
      strcat(tmp," fehlt! IP: ");
      strcat(tmp,ip);
      if (cfg.bDebug) {
        Serial.println(tmp);
      }
      //-------------------Fehler nur anzeigen, wenn Ausgang aktiv
      if(pX->eMode != INAKTIV){
        strncpy(gc_Error[idx+1],tmp,ERRLENGTH);
      }
      pX->bLinkLost=true;
      pX->nextAccessTime= millis()+ IP_RETRY_LOST_LINK + (idx*10000);   // Anfragen bei mehreren fehlenden Stationen entzerren */
    }
    client.stop();
    handleServer();
    return MY_NOT_OK;
  }
  else {
    strcpy(gc_Error[idx+1]," ");
    pX->uiLiLoCnt=0;
//    pX->bLinkLost=false;  // der alte Zustand wird noch weiter unten gebraucht !!
  }
  // -- nächsten Abfragezeitpunkt festlegen
  if(pX->eMode == INAKTIV){
    pX->nextAccessTime=millis()+IP_UPDATE_INAKTIV;
  }
  else {
    if(pX->x == 0){
      pX->nextAccessTime=millis()+ IP_UPDATE_RATE_MS;  //@@@ evtl größer weil Wallbox langsam antwortet  
    }
    else {    // bei Ladung an Filterzeit anpassen
      pX->nextAccessTime=millis()+ cfg.fTime*FILTER_TIME_MS;
    }
  }

  
  // Kommando anlegen
  char nr[3];
  uint16_t iPwrA=0;
  int extrapwr;
  int len;
  String strcmd;
  switch(cmd){
     case goeWRITEOUTPUT:
          strcmd = "GET /mqtt?payload=alw=";
          strcmd+=String(x);
        break;
     case goeWRITEPOWER:   
        // Leistung setzen
        if(pX->ref == UEBERSCHUSS){
          extrapwr=g_uiErzeugung - g_uiVerbrauch - GOE_PWR_RESERVE;
        }
        else
        {
          extrapwr =g_iNetzEinspeisung - GOE_PWR_RESERVE;
        }
        //
        if(millis()> pX->startTime + GOE_CONTROL_DELAY){
          if (extrapwr > 0){
            extrapwr= extrapwr/6;         // Überschuss zu einem Sechstel
          }  
          pX->setpower+=extrapwr;         // Leistung anpassen
           
          if(pX->setpower > GOE_PWR_MAX){
            pX->setpower = GOE_PWR_MAX;
          }
          if(pX->setpower < GOE_PWR_MIN){
            pX->setpower = GOE_PWR_MIN;
          }
        }
        // ohne Ladefreigabe -> Strom auf Minimum
        if(pX->x ==0){
          pX->setpower=GOE_PWR_MIN;
        }
        //
        iPwrA= pX->setpower/230.0/pX->pin;   // auch durch anzahl Phasen geteilt
        if(iPwrA > GOE_MAX_AMPS){
          iPwrA= GOE_MAX_AMPS;
        }
        strcmd = "GET /mqtt?payload=amx=";   //
        strcmd+=String(iPwrA);
        break;
     default:    
        String strcmd = "GET /status";
        break;
  }
  len= strcmd.length()+1;
  strcmd.toCharArray(tmp,len);   

  strcat(tmp," HTTP/1.1");
  client.println(tmp);    // Kommando Anfrage schicken
  Serial.print("Kommando an Go-E Wallbox: ");
  Serial.println(tmp); //@@@
  // hostinfo aufbereiten
  sprintf(tmp, "Host: %u.%u.%u.%u", pX->ip[0],pX->ip[1],pX->ip[2],pX->ip[3]);
  client.println(tmp);   // ip nr
  client.println(F("Accept: application/json"));
  client.println(F("User-Agent: SoSmartBox"));
  client.println(F("Connection: keep-alive"));
  client.println();
  // Antwort lesen
  // Check HTTP status
  char buf[128] = {0};
  client.readBytesUntil('\r', buf, sizeof(buf));
  if (cfg.bDebug)
  {
    Serial.printf("\n Antwort von %s : ",pX->strOutputname);
    Serial.print(buf);
  }
  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";   // leere Zeile nach dem Kopf
  if (!client.find(endOfHeaders)) {
    if (cfg.bDebug) {
      Serial.println(F("Invalid response"));
    }
    client.stop();
    return 0;
  }
  //
  charcnt=0;
  stopTime=millis()+GOE_RESPONSE_DELAY;
  // Längeninfo lesen
  int digits = client.readBytesUntil('\r', buf, sizeof(buf));
  buf[digits]='\0';
  int bodylength;
  bodylength= strtol(buf,NULL,16)+1;
  Serial.printf("  Bodylänge: %i \n",bodylength); 
  
  while (millis()<stopTime &&  charcnt <= bodylength) {
    if(client.available()){
      JsonResponse[charcnt] = (char)client.read();
      charcnt++;
    } 
    else
    {
      delay(2);
    }
  }
  JsonResponse[charcnt] = 0x0; //String abschliessen
  DebugPrint(JsonResponse);
  client.stop();
  //----------------------------------Antwort auswerten
  bool act_x;
  int act_pwr; 
  int access_err=MY_OK; 
  access_err=ExtractGoeInfo(JsonResponse, &act_x, &act_pwr, pX->pin );    // Daten auslesen  - Achtung String wird dabei zerstört
  if (access_err== MY_OK){
      pX->power=act_pwr;
  }
  
  if(access_err== MY_OK  && cmd==goeWRITEOUTPUT){   //Zugriff war erfolgreich
      //------------------ forcen automatisch aufheben, wenn externer Zustand und interner Automatik-Zustand gleich
      if(pX->xa == act_x && pX->bForced){
        pX->bForced = false;
      }
      if(pX->x != act_x  && pX->eMode != INAKTIV){     // Gelesener Zustand ist ungleich intern 
          if (cmd != tsmWRITE)
          {
            if(!pX->bLinkLost && pX->eMode == AUTOMATIK){     //link war vorher auch da und Automatik
                pX->bForced = true;
            }
          }
          if(act_x){
            pX->startTime=millis();
          }
      }
      //-------------
      if (pX->bLinkLost) {    // Verbindung war vorher unterbrochen
          pX->bLinkLost=false;
          pX->bForced=false;
          pX->xa = act_x;     // Automatikzustand synchronisieren 
          // Fehler löschen
      }
      pX->x = act_x;
      return act_x;
  }
//  Serial.printf("\n Zugriff auf %s lieferte keine auswertbaren Daten\n",pX->strOutputname);
  return MY_OK;   //@@@ besser -1 fehler
}
//_______________________________________Read Status of Wallbox
// Gleichzeitig wird die Ladeleistung angepasst.
int goeReadOutput(int idx){
    int retval;
    // Statt Status lesen -> Power setzen
    retval= goeAccess(idx, goeWRITEPOWER, output[idx].x);
    return retval;
}

#endif //go_e_charger_h
