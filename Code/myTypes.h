#ifndef myTypes_h
#define myTypes_h
// function prototypes



//#define DEBUGLEVEL 1

#define LOOP_DELAY 80         //ms
#define STRINGLEN 32
#define CMDLEN 62

#define MAX_FILTER_TIME 30    // maximale Mittelungszeit für Leistungswerte in 10sec
#define FILTER_TIME_MS 10000  // Filterzeit in ms

#define IP_UPDATE_RATE_MS 5000   // alle 5s neue Werte lesen
#define IP_RETRY_LOST_LINK 180000    //180s
#define IP_UPDATE_INAKTIV 300000     //300s
#define IP_LINK_LOSS_CNT_LIMIT 3       


#define MY_OK      1
#define MY_NOT_OK  0
#define MY_ERROR   -1
#define MY_ERROR_NO_HTML -2
#define TRUE      1
#define FALSE     0
#define DC        2        // dont care fuer Vorbelegung von Ausgängen

#define MAX_OUTPUT_ANZ 6

#define MIN_PRIO 3
#define ON 1
#define OFF 0


enum OutputxState {XOFF,XON,XFRC=4,XLL=128};
enum OutputMode {INAKTIV,MANUELL,AUTOMATIK,MODE_UNDEFINED};
enum OutputReferenz {UEBERSCHUSS,NETZBEZUG};
enum OutputTyp {XTYP_UNDEFINED,GPIOPIN, TASMOTA, TASMOTA_POWER, GO_E_CHARGER};
enum BattTyp {UNBEKANNT,SONNEN,SENEC,BTYP_UNDEFINED};

typedef struct {
  char strOutputname[STRINGLEN];
  uint8_t typ;
  uint8_t ip[4]; 
  uint8_t pin;
  enum OutputMode eMode;
  uint8_t pos;              // power on state
  enum OutputReferenz ref;
  int16_t iLimitOn;
  int16_t iLimitOff;
  uint8_t uiMinBattState;
  uint8_t uiPrio;
  uint16_t uiMinOnTime;
  uint16_t uiOffTimeDelay;
// internal use
  uint8_t x;    // Istzustand des Ausgangs
  uint8_t xa;   // Automatikzustand
  uint8_t bForced;        // Ausgang über Web oder extern geändert
  uint8_t bLinkLost;      // bei Wlan Ausgängen
  unsigned int uiLiLoCnt;       // Zähler für erfolglose Kontaktversuche direkt hintereinander
  enum OutputxState exState;   // kombi fuer Visu/Led
  uint16_t power;
  uint16_t setpower;    // angeforderte Leistung, z.B. für Wallboxen
  uint32_t startTime;   // systime for on  
  uint32_t stopTime;    // systime when off requested  
  uint32_t nextAccessTime;  // systemtime for next access - only valid for IP-based outputs
} outputData_t;

typedef struct {
  char boxname[STRINGLEN];     // Name der Schachtel
  uint8_t anzoutputs;          // Anzahl der Ausgänge
  enum BattTyp typ;            // Batterietyp
  byte battip[4];              // Ip address of battery 
  uint16_t uibattport;         // which port to use
  char cmd[CMDLEN];            // kommando zum Datenlesen
  uint8_t fTime;
  uint8_t bDebug;
} configData_t;

#endif
