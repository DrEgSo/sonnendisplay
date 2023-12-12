#ifndef myWifi_h
#define myWifi_h

/*WiFi und Server Initialisierung*/
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266HTTPUpdateServer.h>
  #include <WiFiManager.h>    // 2.0.15 https://github.com/tzapu/WiFiManager
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WiFiClient.h>
  #include <WebServer.h>
#endif

#include <SimpleFTPServer.h>

#include "globvar.h"
#include "switchlogic.h"
#include "config.h"
#include "myUtils.h"
#include "mydisplay.h"
#include "DisplayRGB.h"

// prototypes
void searchNextError(void);
/* Variablen */
/* Variablen für den AccessPoint */
const char* ssid = "SoSmart";         /* Netzwerk-ID  */
const char* password = "geheim0815";        /* Passwort  */

WiFiClient client;
/* Instanz des Webservers -> 80 Standard für HTTP Anfragen eines Internet Browsers (kann jedoch frei gewählt werden) */
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
FtpServer ftpSrv;   //set #define FTP_SERVER_DEBUG in FtpServerkeys.h to see ftp verbose on serial


/* Festlegung der Parameter für den Access-Point */
IPAddress ip(192, 168, 4, 1);             /* IP-Adresse des Access-Points */
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 0, 0);


void NotFound()                                             /* Handling nicht definierter/verwendeter URL's */
{
  server.send(404, F("text/plain"), F("Not Found - or Error") );
  Serial.println("Nicht vorhandene Seite angewählt");
}

void IndexNotFound()                                         /* Antwort bei fehlender index Seite */
{
  server.send(404, F("text/plain"), F("Startseite nicht gefunden - Filesystem mit FTP pruefen !") );
  Serial.println("Index.html fehlt");
}

/* Wifi Funktionen */
void handleRoot() {                                          /* Senden der Hauptseite */
  char fname[32];
  if (cfg.anzoutputs !=0){
    strcpy(fname,"/index.html");
  }
  else
  {
    strcpy(fname,"/index0.html");
  }
  Serial.printf("Hauptseite %s angewählt",fname);

  File file = LittleFS.open(fname, "r");
  if (!file) {
    IndexNotFound();
  }
  else
  {
    server.streamFile(file, F("text/html"));
  }
  file.close();
}

void handleConfig() {                                        /* Senden der Konfigurationsseite */
  Serial.println("Konfigseite angewählt");
  File file = LittleFS.open("/config.html", "r");
  if (!file) {
    NotFound();
    Serial.println("config.html: File not found");
  }
  else
  {
    server.streamFile(file, F("text/html"));
  }
  file.close();
}

void handleOutputConfig() {                                        /* Senden der Konfigurationsseite */
  if(cfg.anzoutputs==0){
    server.send(200, F("text/plain"), F("Keine Ausgänge konfiguriert. Eintrag bei <Konfig> ist Null.") );
    return;
  }
  File file = LittleFS.open("/outconfig.html", "r");
  if (!file) {
    NotFound();
    Serial.println("outconfig.html: File not found");
  }
  else
  {
    server.streamFile(file, F("text/html"));
  }
  Serial.println("Output-Konfigseite angewählt");
  file.close();
}


void sendIcon()
{
  Serial.println("Favicon angefordert");
  File file = LittleFS.open("/favicon.ico", "r");
  if (!file) {
    NotFound();
    Serial.println("Icon: File not found");
  }
  else
  {
    server.streamFile(file, F("image/avif"));
    Serial.println("Icon gesendet");
  }
}

void sendBattData()                                     /*  Daten der Batterie + Version */
{
  if(cfg.bDebug)
    Serial.println(F("Send BattData to Browser "));
    DynamicJsonDocument doc(1024);
    
    doc["SOC"] = String(g_uiBatterieLadezustand);
    doc["PV"] = String(g_uiErzeugung);
    doc["VB"] = String(g_uiVerbrauch);
    doc["BPWR"] = String(g_iBatterieLeistung);
    doc["GRID"] = String(g_iNetzEinspeisung);
    doc["FAC"] = String(fNetzFreq);
    doc["VAC"] = String(uiNetzSpng);
    doc["ERR"] = String(gc_Error[uiActErrIdx]);       // aktuellen Fehler anzeigen
    doc["VERSION"] = String(SW_VERSION);
    //
    searchNextError();    //uiActErrIdx auf nächsten Fehler weiterstellen
    char outputjson[512];
    serializeJson(doc,outputjson);
    server.send(200, F("application/json"), outputjson ); 
//    Serial.println(outputjson);
}
//-------------------------------------------
void sendOutputConfig() {                               /* Senden der Konfiguration des Ausgangs */
  Serial.print(F("sendOutputConfig to Browser: "));
  //  Konfiginfo lesen
  String tmp=server.arg("XNR");
  uiAktOutputNr=tmp.toInt();
  String fname = String("output" + String(uiAktOutputNr) + "cfg.json");
  File file = LittleFS.open(fname, "r");
  Serial.print(F("Filename:"));
  Serial.println(fname);
  if (!file) {
    Serial.println("Fehler beim Öffnen des Konfigurationsfiles der Ausgänge");
    return;
  }
  int len = file.available();
  char buf[1024];
  int i;
  for (i = 0; i < len; i++) {
    buf[i] = file.read();
  }
  buf[i] = 0; // terminieren
  file.close();
  server.send(200, F("application/json"), buf);
}

void sendConfig() {                               /* Senden der allgemeinen Konfiguration */
  Serial.println(F("sendConfig to Browser: "));
  //  Konfiginfo lesen
  File file = LittleFS.open("config.json", "r");
  if (!file) {
    Serial.println("Fehler beim Öffnen des allgemeinen Konfigurationsfiles");
    return;
  }
  int len = file.available();
  char buf[1024];
  int i;
  for (i = 0; i < len; i++) {
    buf[i] = file.read();
  }
  buf[i] = 0; // terminieren
  file.close();
  server.send(200, F("application/json"), buf);
}

void sendOutputNames() {  /* Senden der Bezeichnungen der Ausgänge */
  String msgOutputsJson = "{\"O1N\":\"" + String(output[0].strOutputname)+"\""+
                            ",\"O2N\":\"" + String(output[1].strOutputname) + "\""+ 
                            ",\"O3N\":\"" + String(output[2].strOutputname) + "\""+ 
                            ",\"O4N\":\"" + String(output[3].strOutputname) + "\""+ 
                            ",\"O5N\":\"" + String(output[4].strOutputname) + "\""+ 
                            ",\"O6N\":\"" + String(output[5].strOutputname) + "\"}"; 
  server.send(200, F("application/json"), msgOutputsJson);
  DebugPrint("Namen der Ausgänge gesendet !");
}
//--------------------------------------------------------------------------------------
void sendOutputData() {                                    /* Senden der Zustände der Ausgänge */
  updateExState();
  String msgOutputsJson = "{\"ANZ\":\"" + String(cfg.anzoutputs)+"\""+
                            ",\"A1Z\":\"" + String(output[0].exState) + "\""+ 
                            ",\"A1M\":\"" + String(output[0].eMode) + "\""+ 
                            ",\"A1P\":\"" + String(output[0].power) + "\""+ 
                            ",\"A2Z\":\"" + String(output[1].exState) + "\""+
                            ",\"A2M\":\"" + String(output[1].eMode) + "\""+ 
                            ",\"A2P\":\"" + String(output[1].power) + "\""+ 
                            ",\"A3Z\":\"" + String(output[2].exState) + "\""+
                            ",\"A3M\":\"" + String(output[2].eMode) + "\""+ 
                            ",\"A3P\":\"" + String(output[2].power) + "\""+ 
                            ",\"A4Z\":\"" + String(output[3].exState) + "\""+
                            ",\"A4M\":\"" + String(output[3].eMode) + "\""+ 
                            ",\"A4P\":\"" + String(output[3].power) + "\""+ 
                            ",\"A5Z\":\"" + String(output[4].exState) + "\""+
                            ",\"A5M\":\"" + String(output[4].eMode) + "\""+ 
                            ",\"A5P\":\"" + String(output[4].power) + "\""+ 
                            ",\"A6Z\":\"" + String(output[5].exState) + "\""+
                            ",\"A6M\":\"" + String(output[5].eMode) + "\""+
                            ",\"A6P\":\"" + String(output[5].power) + "\"}";      //@@@ iterativ machen bis act anzahl
  server.send(200, F("application/json"), msgOutputsJson);
  if(cfg.bDebug){
      Serial.println(F("sendOutputData to Browser: "));
      //Serial.println(msgOutputsJson);
    }  
}
//-------------------------------------------------------------------------------------
// Ausgang umschalten
//-------------------------------------------------------------------------------------
void outputChange() {
  int idx;
  DebugPrint("Ausgangsänderung\n");
  if (server.hasArg("XNR")){
    String tmp=server.arg("XNR");
    idx=tmp.toInt()-1;              // index eins kleiner wie Ausgangsnummer
    tmp=server.arg("X");
    
    if(output[idx].bForced == true){
      output[idx].bForced=false;    // forcen aufheben ohne Wertänderung
      output[idx].xa=output[idx].x; // aktuellen Zustand auch als internen Übernehmen
    }
    else {
      output[idx].x =tmp.toInt();
      if(output[idx].x == ON){
        output[idx].startTime=millis(); //startzeit merken
        output[idx].stopTime = 0;
      }
      if(output[idx].eMode==AUTOMATIK){
         output[idx].bForced=true;       // von Hand eingegriffen
      }
      outputWrite(idx,output[idx].x);   // an hardware oder wlan ausgeben
#ifdef DEBUGLEVEL      
      Serial.print(" Ausgang: ");
      Serial.print(idx+1);
      Serial.print("  Zustand neu: ");
      Serial.println(output[idx].x);
#endif      
    }
    server.send(200,F("text/html"),"");
  }
  else {
    return;
  }
}
//-------------------------------------------------------------------------------------------------
// Inhalt aus der POST Nachricht auslesen 
void outConfigModify() {
    int idx=uiAktOutputNr-1;
    char tmp1[32]="";
    if(cfg.bDebug)
      Serial.printf("got OutputConfig %i from Browser ", uiAktOutputNr);
#ifdef DEBUGLEVEL
    char message[255]="";
    strcpy(message,"Ausgang Info: \n");
   
    for (uint8_t i = 0; i < server.args(); i++) {
      strcat(message," ");
      server.argName(i).toCharArray(tmp1,server.argName(i).length()+1);
      strcat(message,tmp1);
      strcat(message,":");
      server.arg(i).toCharArray(tmp1, server.arg(i).length()+1);
      strcat(message,tmp1);
      strcat(message,"\n");
    }
    if(cfg.bDebug)
      Serial.println(message);
#endif
 // 
  String tmp;
  int str_len;
  if(!server.hasArg("name")){
    Serial.println("Fehler bei der Konfigübergabe vom Browser");
    return;
  }
  tmp = server.arg("name");
  tmp.toCharArray(output[idx].strOutputname, STRINGLEN);

  tmp =server.arg("typ");
  output[idx].typ = (OutputTyp)tmp.toInt();
  tmp =server.arg("ip");
  str_len = tmp.length() + 1;
  tmp.toCharArray(tmp1,str_len);
  IP4_String_to_Bytes(tmp1,'.',output[idx].ip,10);
  tmp =server.arg("pin");
  output[idx].pin = tmp.toInt();
  tmp =server.arg("pos");
  output[idx].pos = tmp.toInt();

  tmp =server.arg("ba");
  output[idx].eMode = (OutputMode)tmp.toInt();
  tmp =server.arg("ref");
  output[idx].ref = (OutputReferenz)tmp.toInt();
  tmp = server.arg("pwr_on");
  output[idx].iLimitOn = tmp.toInt();
  tmp = server.arg("pwr_off");
  output[idx].iLimitOff = tmp.toInt();
  tmp = server.arg("ladung");
  output[idx].uiMinBattState = tmp.toInt();
  tmp = server.arg("prio");
  output[idx].uiPrio = tmp.toInt();
  tmp = server.arg("mintime");
  output[idx].uiMinOnTime = tmp.toInt();
  tmp = server.arg("offtime");
  output[idx].uiOffTimeDelay = tmp.toInt();
  

  // Bestätigung schicken
  File file = LittleFS.open("/confirm.html", "r");
  if (!file) {
    NotFound();
    Serial.println("confirm.html: File not found");
  }
  else
  {
    server.streamFile(file, F("text/html"));    // Bestätigungsseite an browser ausgeben
  }
  file.close();
  saveOutputConfig(idx); // in Datei schreiben
}

//
//-----------------Änderung der allgemeinen Daten
void configmodify() {
  String tmp;
  int str_len;
  char tmp1[32];
  tmp = server.arg("BOXNAME");
  str_len = tmp.length() + 1;
  tmp.toCharArray(cfg.boxname,str_len);
  tmp = server.arg("ANZOUTPUTS");
  cfg.anzoutputs= tmp.toInt();
  tmp = server.arg("TYP");
  cfg.typ= (BattTyp)tmp.toInt();
  tmp = server.arg("BATTIP");
  str_len = tmp.length() + 1;
  tmp.toCharArray(tmp1,str_len);
  IP4_String_to_Bytes(tmp1,'.', cfg.battip,10);
  tmp = server.arg("PORT");
  cfg.uibattport = tmp.toInt();
  tmp = server.arg("CMD");
  str_len = tmp.length() + 1;
  tmp.toCharArray(cfg.cmd, str_len);
  tmp = server.arg("FTT");
  cfg.fTime = tmp.toInt();
  // hier noch abspeichern 
  saveGeneralConfig();
  // Inhalt
  File file = LittleFS.open("/confirm.html", "r");
  if (!file) {
    NotFound();
    Serial.println("confirm.html: File not found");
  }
  else
  {
    server.streamFile(file, F("text/html"));    // Bestätigungsseite an browser ausgeben
  }
  file.close();
  delay(100);
  ESP.restart();
}
//-----------------Ausführen von Kommandos die von clients kommen
void checkCommand() {
  String tmp;
  if(server.hasArg("reset")){
    tmp = server.arg("reset");
    int res = tmp.toInt();
    if(res==0){
        server.send(200,F("text/html"),F("<body><h2>Neustart wird gemacht <br> bitte warten !</h2></body>"));
        Serial.println("Resetanforderung vom Browser");
        mydisplay_clear();
        mydisplay_show("Reset angefordert","",true);
        rgbleds_aus();
        delay(2000); // damit Nachricht noch raus geht
        mydisplay_clear();
        ESP.restart();
    }
    if(res==1){
        server.send(200,F("text/html"),F("<body><h2>Reset wird gemacht <br> Wlan wieder neu einstellen !<br> IP: 192.168.4.1 </h2></body>"));
        Serial.println("Werksreset vom Browser");
        mydisplay_show("Totalreset angefordert","",true);
        rgbleds_aus();
        delay(4000); // damit Nachricht noch raus geht
        mydisplay_clear();
        for(int i=0; i < MAX_OUTPUT_ANZ; i++){
          resetOutputconfig(i);
        }
        resetConfig();
        WiFi.disconnect(true);
        ESP.eraseConfig();
        ESP.restart();
    }
  }
  if(server.hasArg("getoutip")){
    tmp = server.arg("getoutip");
    int idx = tmp.toInt()-1;
    if(idx >=0 ){
        char reply[64]="{\"OUTIP\":\"";
        char ipstr[32];
        IP4_Bytes_to_String(ipstr,output[idx].ip,10);
        strcat(reply,ipstr);
        strcat(reply,"\"}\r\n");
        if(output[idx].typ != GPIOPIN){
          server.send(200,F("application/json"),reply);
        }
        else {
          server.send(204,F("application/json"),"");    // no content
        }
    }
  }
}

//-------------------------------Serverstart
void StartWebServer(){
   /* Einstellen des Servers */
  server.on("/", handleRoot);                 /* Verlinkung zur Funktion welche die Webserver-Seite verwaltet*/
  server.on("/index.html", handleRoot);
  server.on("/config.html", handleConfig);
  server.on("/outconfig.html", handleOutputConfig);
  server.on("/favicon.ico", sendIcon);
  server.onNotFound(NotFound);

  server.on("/battdata", sendBattData);
  server.on("/outputdata", sendOutputData);
  server.on("/outputconfig", sendOutputConfig);
  server.on("/outputnames", sendOutputNames);
  server.on("/getconfig", sendConfig);
  
  server.on("/outconfigmodify", outConfigModify);
  server.on("/configmodify", configmodify);
  server.on("/outputchange", outputChange);
  
  server.on("/cmd", checkCommand);
  
  server.begin();                             /* Starten des Servers */
}

void StartFTPServer(){
   /* Einstellen des Servers */
  ftpSrv.begin("master",password);   //username, password for ftp.
}

//
//------------------------- Server bearbeiten
void handleServer(){
  ftpSrv.handleFTP();
  server.handleClient();
}
//--------------------special wartefunktion
void webDelay(unsigned long wt){
  unsigned long et;
  et=millis()+wt;
  while ( millis()< et){
    handleServer();
    delay(2);
  }
}

#endif
