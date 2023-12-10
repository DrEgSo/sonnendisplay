#ifndef globvar_h
#define globvar_h

#include "myTypes.h"

// Globale Variablen



unsigned int uiAktOutputNr=1;
unsigned int actOutputIdx=0;  // welcher Ausgang gerade abgefragt wird         
bool    g_bAllOutputsOffline_Inactive=false;
bool    g_bOneOutputOn=false;
bool    g_bDispBattinfo;       // es wird gerade der Batterieladezustand angezeigt

// Parameter die ausgelesen werden
bool bBatterieConnected=false;
uint16_t g_uiVerbrauch;
uint16_t g_uiErzeugung;
int16_t g_iUeberschuss;
int16_t g_iNetzEinspeisung;       // negativ bedeutet Bezug
int16_t g_iBatterieLeistung;      // Leistung aus/in die Batterie - negativer Wert bedeutet Entladung
uint8_t g_uiBatterieLadezustand;
uint16_t uiNetzSpng;
float   fNetzFreq;
bool bBatteryDischarging;
bool bBatteryCharging;
bool bBatteryFirstAccess=true;

static unsigned long nextBattUpdate = 0;  // wann soll was neues angezeigt werden [ms]
static unsigned long nextIpSwitchUpdate = 0;  // wann soll übers Internet was neues abgefragt werden [ms]
unsigned long lastWlanAcessTime=0;
#define LOSTWLAN_DISPTIME 20000 
volatile unsigned long nextLoop =0;     //  Durchlauf durch Hauptschleife
//-----------------------------------------------------------------
#define ERRANZAHL 7
#define ERRLENGTH 128
char gc_Error[ERRANZAHL][ERRLENGTH]={'\0','\0','\0','\0','\0','\0'};
uint8_t uiActErrIdx=0;
 
uint16_t lostlinkcnt = 0;
// Kofigurationsinfos
configData_t cfg;
outputData_t output[MAX_OUTPUT_ANZ];


#endif
