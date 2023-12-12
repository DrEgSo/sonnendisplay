//-------------------------NeoSonnePlus mit Ansteuerung von Tasmota Komponenten
//
//2023-04-14/Eso:   V2.05 Inaktive Ausgänge werden eingeschaltet auch an den LEDs angezeigt
//2023-04-14/Eso:   V2.04 Ausschaltverzögerung hinzugefügt
//2023-04-14/Eso:   V2.01 Go-e Charger hinzugefügt
//2023-03-16/ESo:   V1.03 Tasmotazugriffe  geändert (Leistungsabfrage nur wenn ein) und für Mehrkanal Tasmota (Shelly2) angepasst
//2023-03-06/ESo:   V1.02 Statusabfrage überarbeitet, Status 10 für Powermonitoring, Forcen (nur bei Automatik) aufheben, wenn Zustand gleich, kein Reset bei Wlanverlust
//2023-03-03/ESo:   V1.01 Referenz fürs Schalten wählbar,  Mittelung für Einspeisewert gleich wie Verbrauch/Erzeugung
//2023-02-24/ESo:   erste Version
//
// Board:  esp8266  3.1.1         Generic8266  = normaler ESP8266 (kein E Modell !) hat nur 1MB Speicher
// Bibliotheken
// IDE              1.8.19
// ArduinoJson      6.20.1
// OLed SSD1306     4.3.0   
// Neopixel  (GFX)  1.11.0
// SimpleFTP        2.1.6

#define SW_VERSION 2.05

/**********Initialisierung und Variablen*********************************************************************************************************************************************************/
//

#include <Wire.h>
#include <Adafruit_GFX.h>    
#include <ArduinoJson.h>    
#include <LittleFS.h>

#include "myTypes.h"
#include "globvar.h"
#include "config.h"
#include "sonnendata.h"
#include "switchlogic.h"
#include "myWifi.h"
#include "myUtils.h"
#include "mydisplay.h"
#include "DisplayRGB.h"

#define ARRAYSIZE 10
#define REFRESHRATE_DISPLAY 1000  //ms


/*Variablen der Ausgabespalte*/
String msgWindowStat[ARRAYSIZE] = {" ", " ", " ", " ", " ", " ", " ", " ", " ", " "};


/**********Funktionen****************************************************************************************************************************************************************************/


/**********void setup()***************************************************************************************************************************************************************************/

void setup() {
  unsigned long lastAction, firstTryTime;
  unsigned long dauer = 0;
  unsigned int counter = 0;
  rst_info *resetInfo;          // Zeiger auf die Reset Information

  Serial.begin(115200);                                            /* Start/Öffnen der seriellen Schnittstelle */
  while (!Serial);

  resetInfo = ESP.getResetInfoPtr();
  Serial.println(resetInfo->reason);

  WiFi.mode(WIFI_STA);  // set station mode
  WiFiManager wm;       //  wm instance local

  pixels.begin(); // This initializes the NeoPixel library.
  mydisplay_init();     // display initialisieren
  String tmp=ESP.getResetReason();
  char buf[128];
  tmp.toCharArray(buf,128);
  mydisplay_show("Neustart wegen",buf,"");
  delay(2000);
  //______________________Filesystem öffnen 
  if (LittleFS.begin()) {
    if (cfg.bDebug)
      Serial.println("Filesystem geöffnet");
  }
  if(resetInfo->reason != REASON_SOFT_RESTART){     // nach reset keine Meldung
      char actssid[64]="";
      WiFi.SSID().toCharArray(actssid,sizeof(actssid));
      mydisplay_clear();
      mydisplay_show("Wlanzugriff",actssid);
  }
  else {
      mydisplay_clear();
  }
  
  wm.setConfigPortalTimeout(10);   //Sekunden
  wm.setConnectTimeout(8);
  bool res = wm.autoConnect(ssid,password); // in myWifi.h
  if(!res){
    // nochmal mit grosser Zeit für Konfigportal probieren
    wm.setConfigPortalTimeout(300);   //Sekunden
    mydisplay_clear();
    mydisplay_show("WLAN SoSmart","erzeugt");
    res = wm.autoConnect(ssid,password); //nochmal probieren  
  }
  else {
    mydisplay_show(WiFi.localIP().toString().c_str());
    delay(1000);
  }
  delay(1);
  if(!res) {
    Serial.println("Failed to connect or hit timeout");
    mydisplay_show("Keine WLAN","Verbindung","-> Neustart");
    unsigned long nextAction=millis()+5000;
    while (millis()<nextAction) {
      SequenzFlash(10, 0, 0);        //rotes Lauflicht
    }
    mydisplay_clear();
    WiFi.mode(WIFI_OFF);
    delay(600000);     //600s 
    ESP.restart();
  }
  /*-----------------------------------------------------------------config lesen ------------------------*/
  int err=loadConfig();
  if (err == MY_ERROR || err == MY_ERROR_NO_HTML ) { //Fehler beim lesen der Konfiguration
    StartFTPServer();
    StartWebServer();
    mydisplay_show("Webseiten-Dateien","fehlen !","-> FTP Server aktiv !",WiFi.localIP().toString().c_str());
    while (1) {
      SequenzFlash(0, 0, 25, true);        //blaues Lauflicht
      delay(1);
      SequenzFlash(25, 0, 0, true);        //rotes Lauflicht
      delay(20);
    }
  }

  StartWebServer();
  httpUpdater.setup(&server,"master",password);   
  StartFTPServer();  
  delay(10);
  // Alle Ausgänge auf OUTPUT und PowerOnState setzen
  for ( int i=0; i < cfg.anzoutputs; i++) {
    switch(output[i].typ){
      case GPIOPIN:
            pinMode(output[i].pin, OUTPUT);
            outputWrite(i,output[i].pos);      // dann vorbelegen
      break;
      case TASMOTA_POWER:
      case TASMOTA:
           if(output[i].pos != DC){              // nicht dont care/change
               output[i].xa=output[i].pos;      // auch den automatik merker
               output[i].x=output[i].pos;      // auch den automatik merker
               outputWrite(i,output[i].pos);      // dann vorbelegen
               output[i].bForced= false;
            }
            else {
               output[i].x = outputRead(i);     // einlesen und übernehmen
               output[i].xa = output[i].x;      // auch den automatik merker
               output[i].bForced= false;
            }
      break;
      case XTYP_UNDEFINED:
      default:
      break;
    }
  }
  nextIpSwitchUpdate=millis() + IP_UPDATE_RATE_MS;
  nextBattUpdate=millis() + BATT_UPDATE_RATE_MS;
  nextLoop=millis()+LOOP_DELAY;
  bBatterieConnected=true;
}

/**********void loop()****************************************************************************************************************************************************************************/
void loop()
{
  while(millis() < nextLoop){
    // ----- Server + FTP bearbeiten
    handleServer();  
    delay(10);  
  }
  nextLoop=millis()+LOOP_DELAY;
  // ----- Server + FTP bearbeiten
  handleServer();
  //-----------------------------------------------------------------------------------
  Serial.print(F("-"));

  //--------------------Check Wlan
  if(WiFi.status() != WL_CONNECTED){
    if(millis() < (lastWlanAcessTime + LOSTWLAN_DISPTIME)){
        Serial.println("Wlanverbindung verloren");
        mydisplay_show("Kein Wlan");
        rgbleds_aus();
        for(int i=0; i< 5; i++){
              SequenzFlash(10, 0, 0);        //rotes Lauflicht
        }      
        delay(3000);
    }
    else {
      rgbleds_aus();
      mydisplay_clear();
    }
    return;
  }
  else {
     lastWlanAcessTime=millis();
  }
  //-----------------------------------------------
  if (millis() > nextBattUpdate)
  { 
    if(bBatterieConnected)   
      nextBattUpdate = millis()+BATT_UPDATE_RATE_MS;
    else
      nextBattUpdate = millis()+10*BATT_UPDATE_RATE_MS;
    //---------Anfrage der Daten
    switch(cfg.typ){
      case SONNEN:
        getSonnenInfo();
      break;
      case SENEC:
        strcpy(gc_Error[0],"Senec wird noch nicht unterstützt !");
      break;
      default:
      break;
    }
    //-----auf dem Display anzeigen
    mydisplayinfo(STD);
  }
  //-------------------------------- IP Schalter aktualisieren
  uint32_t now=millis();
  if(now > nextIpSwitchUpdate &&  (cfg.anzoutputs >0))
  {
    int i=0;
    while(output[actOutputIdx].typ == GPIOPIN  && i < cfg.anzoutputs){   // GPIOs überspringen
      actOutputIdx = ++actOutputIdx%cfg.anzoutputs;
      i++;
    }
    //---------------------------------------
    if(now > output[actOutputIdx].nextAccessTime) {    // readtime reached
        outputRead(actOutputIdx);
        nextIpSwitchUpdate+=IP_UPDATE_RATE_MS;            // nur weiterstellen wenn auch ein Zugriff erfolgt
    } 
    actOutputIdx = ++actOutputIdx%cfg.anzoutputs;
  }  
  // Überschuss berechnen
  g_iUeberschuss = (int)g_uiErzeugung - (int)g_uiVerbrauch;
  // Ausgangszustände aktualisieren - wenn Batteriezugriff möglich
  if(cfg.anzoutputs >0 && bBatterieConnected ){
    switchupdate();
  }
  ShowRGB();
  updateExState(); // damit auch bei inaktiven Ausgängen -der Zustand aktualisiert wird
}
