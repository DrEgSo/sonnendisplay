#ifndef sonnendata_h
#define sonnendata_h

#include "globvar.h"
#include "myWifi.h"
#include "mydisplay.h"

#define BATTINFO_MAXLENGTH 1536
#define SONNENINFO_MINLENGTH 600
#define BATT_LOST_LINK_LIMIT 25
#define BATT_UPDATE_RATE_MS 3000   // alle 3s neue Werte lesen
#define BATT_RESPONSE_DELAY 100     // Wartezeit beim Empfang

void clearBattData(){
    g_iBatterieLeistung = 0;
    g_uiBatterieLadezustand = 0;
    g_uiErzeugung = 0;
    g_uiVerbrauch = 0;
    g_iNetzEinspeisung = 0;
    bBatteryDischarging = false;
    bBatteryCharging = false;
    uiNetzSpng = 0;
    fNetzFreq = 0;
    bBatteryFirstAccess=true;
}

void ExtractSonnenInfo(char* json) {
  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  #define BATTJSONSIZE 1024
  unsigned int uiPwrP, uiPwrV,iGridIn;
  int32_t anzwerte, runddelta;
  DynamicJsonDocument jsondoc(BATTJSONSIZE);

  // Parse JSON object
  DeserializationError  error = deserializeJson(jsondoc, json);
  // Get a reference to the root object
  JsonObject obj = jsondoc.as<JsonObject>();

  if (error) {
    Serial.print(F("Parsing Sonneninfo failed!"));
    Serial.println(error.c_str());
  }
  else  // json parsen
  {
    g_iBatterieLeistung = jsondoc["Pac_total_W"].as<int>();
    g_uiBatterieLadezustand = jsondoc["USOC"].as<unsigned int>();
    uiPwrP = jsondoc["Production_W"].as<unsigned int>();
    uiPwrV = jsondoc["Consumption_W"].as<unsigned int>();
    iGridIn = jsondoc["GridFeedIn_W"].as<int>();
    bBatteryDischarging = jsondoc["BatteryDischarging"].as<bool>();
    bBatteryCharging = jsondoc["BatteryCharging"].as<bool>();
    uiNetzSpng = jsondoc["Uac"].as<unsigned int>();
    fNetzFreq = jsondoc["Fac"].as<float>();
  }
  if (cfg.fTime != 0  && !bBatteryFirstAccess) {
    anzwerte = ((unsigned long)cfg.fTime * FILTER_TIME_MS) / BATT_UPDATE_RATE_MS;
    runddelta = anzwerte / 2;
    if (uiPwrP < g_uiErzeugung) {
      runddelta -= runddelta;
    }
    g_uiErzeugung = ( (int32_t)g_uiErzeugung * (anzwerte - 1) + (int32_t)uiPwrP + runddelta ) / anzwerte;
    runddelta = anzwerte / 2;
    if (uiPwrV < g_uiVerbrauch) {
      runddelta -= runddelta;
    }
    g_uiVerbrauch = ((int32_t)g_uiVerbrauch * (anzwerte - 1) + (int32_t)uiPwrV + runddelta) / anzwerte;
    // Netzeinspeisung mitteln, weil als Schaltentscheidung sonst zu "lebhaft" 
    if (iGridIn < g_iNetzEinspeisung) {
      runddelta -= runddelta;
    }
    g_iNetzEinspeisung = ((int32_t)g_iNetzEinspeisung * (anzwerte - 1) + (int32_t)iGridIn + runddelta) / anzwerte;
  }
  else
  {
    g_uiErzeugung = uiPwrP;     // aktueller Wert
    g_uiVerbrauch = uiPwrV;     // aktueller Wert
    g_iNetzEinspeisung = iGridIn;
    bBatteryFirstAccess = false;
  }
}

void getSonnenInfo() {
  unsigned int charcnt=0;
  char BattJsonResponse[BATTINFO_MAXLENGTH];
  if (!client.connect(cfg.battip, cfg.uibattport)) {
    delay(1);
    if (cfg.bDebug) {
      Serial.print("Verbindung zur Sonnenbatterie fehlgeschlagen ! IP: ");
      IP4_Bytes_to_String(tmp, cfg.battip, 10);
      Serial.print(tmp);
      Serial.print("  Port: ");
      Serial.println(cfg.uibattport);
    }
    strcpy(gc_Error[0], "Keine Verbindung zur Batterie !");
    bBatterieConnected = false;
    lostlinkcnt++;
    if (lostlinkcnt > BATT_LOST_LINK_LIMIT) {
      clearBattData();    // globale Daten löschen
    }
    return;
  }
  else {
    lostlinkcnt = 0;
    bBatterieConnected = true;
    strcpy(gc_Error[0], "");
  }
  ESP.wdtEnable(2000);
  client.println(cfg.cmd);    // Anfrage schicken
  // hostinfo vorbeireiten
  char battinfo[32]; // für Anfrage
  sprintf(battinfo, "Host: %u.%u.%u.%u", cfg.battip[0],cfg.battip[1],cfg.battip[2],cfg.battip[3]);
  client.println(battinfo);   // ip nr
  client.println("User-Agent: SoSmartBox");
  client.println("Connection: keep-alive");
  client.println();
  // Antwort lesen
  // Check HTTP status
  char status[128] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (cfg.bDebug)
  {
    Serial.println();
    Serial.print("SonnenAntwort: ");
    Serial.println(status);
  }
  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";   // leere Zeile nach dem Kopf
  if (!client.find(endOfHeaders)) {
    if (cfg.bDebug) {
      Serial.println(F("Invalid response"));
    }
    return;
  }
  delay(BATT_RESPONSE_DELAY);    //
  while (client.available() && charcnt < BATTINFO_MAXLENGTH) {
    BattJsonResponse[charcnt] = (char)client.read();
    charcnt++;
  }
  BattJsonResponse[charcnt] = 0x0; //String abschliessen
  if (charcnt < SONNENINFO_MINLENGTH) {
    if (cfg.bDebug) {
      Serial.print(F("Battery Info to short/incomplete - length: "));
      Serial.println(charcnt);
      Serial.println(BattJsonResponse);
    }
    nextBattUpdate = millis()+1000;    // nach 1s nochmal probieren
  }
  else {
    ExtractSonnenInfo(BattJsonResponse);    // Daten auslesen und in globale Variablen ablegen - Achtung wird dabei zerstört
  }
  client.stop();
  delay(1);
}
#endif
