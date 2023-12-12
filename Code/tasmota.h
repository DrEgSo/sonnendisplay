#ifndef tasmota_h
#define tasmota_h

#include "myWifi.h"
void webDelay(unsigned long); //prototyp aus myWifi
void handleServer(void);    //prototyp aus myWifi
#include "myUtils.h"
#include "myTypes.h"

#define TASMOTAINFO_MAXLENGTH 512

#define TSM_RESPONSE_DELAY 1300  

enum eCMD {tsmREADPOWER,tsmWRITE, tsmREADOUTPUT};

int ExtractTasmotaInfo( char* json , eCMD cmd, bool *x, int *pwr, int pin_nr) {
  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  #define TSMJSONSIZE 512
  DynamicJsonDocument jsondoc(TSMJSONSIZE);

//  Serial.println(json);
  // Parse JSON object
  DeserializationError  error = deserializeJson(jsondoc, json);
//  Serial.print("Speicherverbrauch für das JSON Objekt: ");
//  Serial.println(jsondoc.memoryUsage()); 
  jsondoc.shrinkToFit(); 
  // Get a reference to the root object
  JsonObject root = jsondoc.as<JsonObject>();

  if (error) {
    Serial.print(F("Parsing Tasmotainfo failed! : "));
    Serial.println(error.f_str());
    return MY_ERROR;
  }
  else  // json auslesen
  {
    if(cmd == tsmREADPOWER){
      int pwrnr = pin_nr-1;
      JsonVariant power = root["StatusSNS"]["ENERGY"]["Power"][pwrnr].as<JsonVariant>();      //bei mehrkanaligen Schaltern
      if(!power.isNull()){
        *pwr = power.as<int>();
      }
      else {
        power = root["StatusSNS"]["ENERGY"]["Power"].as<JsonVariant>();                   // bei einkanaligen Schaltern
        if(!power.isNull()){
          *pwr = power.as<int>();
        }
        else {
          Serial.println("Leistung nicht enthalten");
          return MY_ERROR;
        }
      }
    }
    if(cmd ==tsmREADOUTPUT){
      char pwrname[12]="POWER";
      char pwrnr[4]="";
      JsonVariant relais = root[pwrname].as<JsonVariant>();
      if(!relais.isNull()){
        if(strcmp(jsondoc[pwrname],"ON")==0)
        {  // eingeschaltet
          *x=true;
        }
        else
        {
          *x=false;
        }
      }
      else {
        itoa(pin_nr,pwrnr,10);
        strcat(pwrname,pwrnr);      // z.B. "POWER2"
        relais = root[pwrname].as<JsonVariant>();
        if(!relais.isNull()){
          if(strcmp(jsondoc[pwrname],"ON")==0)
          {  // eingeschaltet
            *x=true;
          }
          else
          {
            *x=false;
          }
        }
        else {
          DebugPrint("Relaiszustand nicht enthalten");
          return MY_ERROR;
        }
      }
    }
  }
  return MY_OK;
}

//----------------- Zugriff auf Tasmota Gerät - liefert Ausgangszustand bei Lesezugriff zurück
int tasmotaAccess (int idx, eCMD cmd, uint8_t x) {
  static unsigned long stopTime;
  unsigned int charcnt;
  char tmp[128];
  char ip[20];
  char JsonResponse[TASMOTAINFO_MAXLENGTH];
  
  outputData_t* pX = &output[idx];
  WiFiClient client;
  ESP.wdtDisable();
  IP4_Bytes_to_String(ip, pX->ip, 10);  // IP Adresse als ASCII erzeugen

  if (!client.connect(pX->ip, 80)) {
    Serial.printf("Verbindung zu %s einmal nicht möglich\n",pX->strOutputname);
    delay(1);
    ESP.wdtEnable(2000);
    pX->uiLiLoCnt++;
    pX->nextAccessTime=millis()+ (8*IP_UPDATE_RATE_MS) + (idx*6000);

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
  ESP.wdtEnable(2000);
  // -- nächsten Abfragezeitpunkt festlegen
  if(pX->eMode == INAKTIV){
    pX->nextAccessTime=millis()+IP_UPDATE_INAKTIV;
  }
  else {
    pX->nextAccessTime=millis()+IP_UPDATE_RATE_MS;          
  }

  
  // Kommando anlegen
  char nr[3];
  switch(cmd){
     case tsmREADPOWER:
        strcpy(tmp,"GET /cm?cmnd=status%2010");      // Status 10 entspricht Leistungsabfrage
        break;
     case tsmWRITE:
        strcpy(tmp,"GET /cm?cmnd=power");
        itoa(pX->pin,nr,10);
        strcat(tmp,nr);
        strcat(tmp,"%20");
        itoa(x,nr,10);
        strcat(tmp,nr);
        DebugPrint("Kommando an TSM: ");
        DebugPrint(tmp);
        // nach Schreibzugriff -> lesen wieder früher zulassen
        pX->nextAccessTime=millis()+(IP_UPDATE_RATE_MS/2);          
        break;
     case tsmREADOUTPUT:
     default:
        strcpy(tmp,"GET /cm?cmnd=power");
        itoa(pX->pin,nr,10);
        strcat(tmp,nr);
        break;
  }
  strcat(tmp," HTTP/1.1");
  client.println(tmp);    // Kommando Anfrage schicken
  Serial.println(tmp); //@@@
  // hostinfo aufbereiten
  sprintf(tmp, "Host: %u.%u.%u.%u", pX->ip[0],pX->ip[1],pX->ip[2],pX->ip[3]);
  client.println(tmp);   // ip nr
  client.println(F("Accept: application/json"));
  client.println(F("User-Agent: SoSmartBox"));
  client.println(F("Connection: close"));
  client.println();
  // Antwort lesen
  // Check HTTP status
  char buf[128] = {0};
  client.readBytesUntil('\r', buf, sizeof(buf));
  if (cfg.bDebug)
  {
    Serial.printf("\nTasmota Antwort von %s : ",pX->strOutputname);
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
  stopTime=millis()+TSM_RESPONSE_DELAY;
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
  if(cmd==tsmREADPOWER || cmd==tsmREADOUTPUT)
  {
    access_err=ExtractTasmotaInfo(JsonResponse, cmd, &act_x, &act_pwr, pX->pin );    // Daten auslesen  - Achtung String wird dabei zerstört
  }
  if (access_err== MY_OK){
    if( cmd==tsmREADPOWER){ // nur dabei ist ein gültiger pwr wert geholt worden
        pX->power=act_pwr;
    }
  }
  
  if(access_err== MY_OK  && cmd==tsmREADOUTPUT){   //Zugriff war erfolgreich
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
//_______________________________________ReadOutput of Tasmota Station
int tsmReadOutput(int idx){
    int retval;
    // Ausgangszustand lesen
    retval= tasmotaAccess(idx, tsmREADOUTPUT, output[idx].x);
    // bei eingeschaltetem Ausggang auch noch die Leistung lesen
    if(output[idx].x == true && output[idx].uiLiLoCnt ==0 && output[idx].typ == TASMOTA_POWER){
      retval= tasmotaAccess(idx, tsmREADPOWER, output[idx].x);
    }
    else {
      output[idx].power=0;
    }
    return retval;
}

#endif //tasmota_h
