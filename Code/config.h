#ifndef config_h
#define config_h

#include <ESP8266WiFi.h>
#include "MyTypes.h"
#include "myUtils.h"

char tmp[256]="";
char buf[256]="";
// prototypes
int resetOutputconfig(int);
int resetConfig(void);
//----------------------------------------------------------------------------------------------
// Outputkonfig als JSON in Datei schreiben
//--------------------------------------------------------------------------------------------
int saveOutputConfig(int idx) {
  // Save configuration from RAM to File
  char fname[32], tmp[3];
  DynamicJsonDocument doc(2048);

  strcpy(fname, "output");
  itoa((idx+1), tmp, 10);
  strcat(fname, tmp);
  strcat(fname, "cfg.json");  //z.B. output1cfg.json
/*  
  Serial.print("Freier Speicher: ");
  Serial.println(ESP.getMaxFreeBlockSize());
  */
  doc["name"] = output[idx].strOutputname;
  doc["typ"] = String(output[idx].typ);
  char ip[20];
  IP4_Bytes_to_String(ip,output[idx].ip, 10);
  doc["ip"] = String(ip);
  doc["pin"] = String(output[idx].pin);
  doc["pos"] = String(output[idx].pos);
  doc["modus"] = String(output[idx].eMode);
  doc["ref"] = String(output[idx].ref);
  doc["pwron"] = String(output[idx].iLimitOn);
  doc["pwroff"] = String(output[idx].iLimitOff);
  doc["minbatt"] = String(output[idx].uiMinBattState);
  doc["prio"] = String(output[idx].uiPrio);
  doc["mintime"] = String(output[idx].uiMinOnTime);
  doc["offtime"] = String(output[idx].uiOffTimeDelay);
#ifdef DEBUGLEVEL  
  if(cfg.bDebug){
    Serial.println("Ausgangskonfiguration wird gespeichert:");
    serializeJsonPretty(doc, Serial);
  }
#endif  
  File configfile = LittleFS.open(fname, "w");   
  if (!configfile) {
    Serial.println("Fehler beim Öffnen des Konfigurationsfiles");
    return MY_ERROR;
  }
  serializeJsonPretty(doc, configfile);
  configfile.close();
  return OK;
}

//----------------------------------------------------------------------------------------------
// Allgemeine Konfig als JSON in Datei schreiben
//--------------------------------------------------------------------------------------------
int saveGeneralConfig() {
  // Save configuration from RAM to File
  DynamicJsonDocument doc(2048);
  doc["BOXNAME"] = String(cfg.boxname);
  cfg.anzoutputs = min(cfg.anzoutputs, (uint8_t) MAX_OUTPUT_ANZ);
  doc["ANZOUTPUTS"] = String(cfg.anzoutputs);
  doc["TYP"]=String(cfg.typ);
  char ip[20];
  IP4_Bytes_to_String(ip, cfg.battip, 10);
  doc["BATTIP"] = String(ip);
  doc["PORT"] = String(cfg.uibattport);
  doc["CMD"] = String(cfg.cmd);
  cfg.fTime = min(cfg.fTime, (uint8_t) MAX_FILTER_TIME);
  doc["FILTERTIME"] = String(cfg.fTime);
  doc["DEBUG"] = String(cfg.bDebug);

#ifdef DEBUGLEVEL
  Serial.println("Folgende Konfiguration wird gespeichert:");
  serializeJsonPretty(doc, Serial);
#endif
  File configfile = LittleFS.open("/config.json", "w");
  if (!configfile) {
    Serial.println("Fehler beim Öffnen des Wlan Konfigurationsfiles");
    return MY_ERROR;
  }
  serializeJsonPretty(doc, configfile);
  configfile.close();
  return OK;
}


#ifdef DEBUGLEVEL
void showCfgSerial() {
  Serial.printf("Konfiguration:\n NAME: %s AUSGAENGE: %i ", cfg.boxname, cfg.anzoutputs);
  sprintf(tmp, "BATTIP: %u.%u.%u.%u  PORT: %u  Typ: %u", cfg.battip[0], cfg.battip[1], cfg.battip[2], cfg.battip[3], cfg.uibattport, cfg.typ);
  Serial.println(tmp);
  sprintf(tmp, "CMD: %s  FILTERTIME: %i ", cfg.cmd, cfg.fTime);
  Serial.println(tmp);
}

void showOutCfgSerial(int idx) {
    char strIp[16]="";
    IP4_Bytes_to_String(strIp, output[idx].ip , 10);
    sprintf(tmp, "\nAusgang: %i  Name: %s Typ: %u IP: %s Pin: %u ", idx+1, output[idx].strOutputname, output[idx].typ, strIp , output[idx].pin);
    Serial.println(tmp);
    sprintf(tmp, "PowerOn: %i Modus: %i Referenz: %i LimitOn: %u ", output[idx].pos, output[idx].ref, output[idx].eMode, output[idx].iLimitOn);
    Serial.print(tmp);
    sprintf(tmp, " LimitOff: %i MinBatt: %i Prio: %i MinTime: %i", output[idx].iLimitOff, output[idx].uiMinBattState, output[idx].uiPrio, output[idx].uiMinOnTime);
    Serial.println(tmp);
    Serial.println();
}
#endif
//--------------------------------------------------------------------------------------------------------------------------
// Lesefunktion für Ausgang Nr. x
// -1 = Fehler
// 1 = OK
//---------------------------------------------------------------------------------------------------------------------------
int8_t loadOutConfig(int nr) {
  unsigned int len;
  unsigned int idx;          // beginnt bei Null, die Nummer bei eins
  char fname[32], tmp[3];
  idx = nr - 1;             // index beginnt bei Null

  strcpy(fname, "output");
  itoa(nr, tmp, 10);
  strcat(fname, tmp);
  strcat(fname, "cfg.json");  //z.B. output1cfg.json
  DebugPrint(fname);
  File outputcfgfile = LittleFS.open(fname, "r");
  if (!outputcfgfile) {
    Serial.print("Fehler beim Öffnen des Konfigurationsfiles von Ausgang: ");
    Serial.println(nr);
    int8_t res=resetOutputconfig(idx);
    if (res != MY_ERROR){
      return OK;
    }
    return res;
  }
  len = outputcfgfile.available();
#ifdef DEBUGLEVEL
  Serial.print("Länge des ");
  Serial.print(fname);
  Serial.print(" KonfigFiles: ");
  Serial.println(len);
#endif
  int i;
  for (i = 0; i < len; i++) {
    buf[i] = outputcfgfile.read();
  }
  buf[i] = 0; // terminieren
  //-----------------------------------ausgeben
  outputcfgfile.close();
#ifdef DEBUGLEVEL
  Serial.print("Output: ");
  Serial.print(nr);
  Serial.println(" Konfigdatei Inhalt:");
  for (int i = 0; i < len; i++) {
    Serial.print(buf[i]);
  }
  Serial.println();
#endif  
  DynamicJsonDocument jsondoc(2048);
  jsondoc.clear();      //Dokument zurücksetzen
  DeserializationError  error = deserializeJson(jsondoc, buf);  //fileinhalt einlesen
  if (error) {
    Serial.print(F("Parsing output config file failed!  -> "));
    Serial.println(error.f_str());
    return MY_ERROR;
  }
  if (jsondoc.containsKey("name"))
    strcpy(output[idx].strOutputname, jsondoc["name"].as<const char*>());
  else
    return MY_ERROR;
  output[idx].typ = jsondoc["typ"];
  IP4_String_to_Bytes(jsondoc["ip"].as<const char*>(),'.',output[idx].ip,10);
  output[idx].pin = jsondoc["pin"];
  output[idx].pos = jsondoc["pos"];
  output[idx].eMode = (OutputMode)jsondoc["modus"];
  output[idx].ref = jsondoc["ref"];
  output[idx].iLimitOn = jsondoc["pwron"];
  output[idx].iLimitOff = jsondoc["pwroff"];
  output[idx].uiMinBattState = jsondoc["minbatt"];
  output[idx].uiPrio = jsondoc["prio"];
  output[idx].uiMinOnTime = jsondoc["mintime"];
  output[idx].uiOffTimeDelay = jsondoc["offtime"];
#ifdef DEBUGLEVEL
  if (cfg.bDebug){
    showOutCfgSerial(idx);
  }
#endif  
  return OK;
}

//--------------------------------------------------------------------------------------------------------------------------
// Lesefunktion
// -1 = Fehler
// 1 = OK
//---------------------------------------------------------------------------------------------------------------------------
int8_t loadConfig() {
  unsigned int len;
  int i;
  // check if webpage file is present
  File configfile = LittleFS.open("index.html", "r");
  if (!configfile) {
    Serial.println("Fehler kein index.HTML File gefunden");
    return MY_ERROR_NO_HTML;
  }
  // Loads configuration from file into RAM
  configfile = LittleFS.open("config.json", "r");
  if (!configfile) {
    Serial.println("Fehler beim Öffnen des Konfigurationsfiles");
    resetConfig();
    return MY_ERROR;
  }
  len = configfile.available();
  //  Serial.print("Länge des Files: ");
  //  Serial.println(len);
  for (i = 0; i < len; i++) {
    buf[i] = configfile.read();
  }
  buf[i] = 0; // terminieren

  configfile.close();
/*
  Serial.println("Konfigdatei Inhalt:");
  for (int i = 0; i < len; i++) {
    Serial.print(buf[i]);
  }
  Serial.println();*/
  DynamicJsonDocument jsondoc(1024);
  // Parse JSON object
  DeserializationError  error = deserializeJson(jsondoc, buf);
  if (error) {
    Serial.print(F("Parsing config file failed!  -> "));
    Serial.println(error.c_str());
    return MY_ERROR;
  }

  if (jsondoc.containsKey("BOXNAME"))
    strcpy(cfg.boxname, jsondoc["BOXNAME"].as<const char*>());
  else
    return MY_ERROR;

  if (jsondoc.containsKey("ANZOUTPUTS"))
    cfg.anzoutputs = jsondoc["ANZOUTPUTS"];
  else
    return MY_ERROR;
    
  if (jsondoc.containsKey("TYP"))
    cfg.typ = jsondoc["TYP"];
  else
    return MY_ERROR;

  if (jsondoc.containsKey("BATTIP"))
    IP4_String_to_Bytes(jsondoc["BATTIP"].as<const char*>(), '.', cfg.battip, 10);
  else
    return MY_ERROR;
  if (jsondoc.containsKey("PORT"))
    cfg.uibattport = jsondoc["PORT"];
  else
    return MY_ERROR;

  if (jsondoc.containsKey("CMD"))
    strcpy(cfg.cmd, jsondoc["CMD"].as<const char*>());
  else
    return MY_ERROR;
  if (jsondoc.containsKey("FILTERTIME"))
    cfg.fTime = jsondoc["FILTERTIME"];
  else
    cfg.fTime = 0;

  if (jsondoc.containsKey("DEBUG"))
    cfg.bDebug = jsondoc["DEBUG"];
  else
    cfg.bDebug = false;
#ifdef DEBUGLEVEL
  if (cfg.bDebug)
    showCfgSerial();
  cfg.bDebug = TRUE;
  Serial.println("Generelle Konfiguration gelesen");
#endif
  //----------------------------------------------Ausgangsconfig lesen
  for (int i = 1; i <= cfg.anzoutputs; i++) {
    loadOutConfig(i);
  }
  return OK;
}
//----------------------------------------------------------
int resetOutputconfig(int idx){
  sprintf(output[idx].strOutputname,"Ausgang%i",idx+1);
  output[idx].typ = XTYP_UNDEFINED;
  output[idx].ip[0]=0;
  output[idx].ip[1]=0;
  output[idx].ip[2]=0;
  output[idx].ip[3]=0;
  output[idx].pin = 1;
  output[idx].pos = 3;
  output[idx].eMode = INAKTIV;
  output[idx].ref = UEBERSCHUSS;
  output[idx].iLimitOn = 0;
  output[idx].iLimitOff = 0;
  output[idx].uiMinBattState = 100;
  output[idx].uiPrio = 3;
  output[idx].uiMinOnTime = 0;
  output[idx].uiOffTimeDelay = 0;

  int res=  saveOutputConfig(idx);
  return res;
}
//----------------------------------------------------------
int resetConfig(){
  strcpy(cfg.boxname,"Smarte Schalt-Box");
  cfg.anzoutputs=6;
  cfg.typ=UNBEKANNT;
  cfg.battip[0]=0;
  cfg.battip[1]=0;
  cfg.battip[2]=0;
  cfg.battip[3]=0;
  cfg.uibattport=80;
  strcpy(cfg.cmd,"GET /api/v2/status HTTP/1.1");
  cfg.fTime=1;
  cfg.bDebug=1;

  int res=saveGeneralConfig();
  return res;
}

#endif
