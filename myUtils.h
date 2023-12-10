#ifndef myUtils_h
#define myUtils_h

#include "globvar.h"
//
//__________________________
void DebugPrint(const char* c){
//  if(!cfg.bDebug)
//    return;
  Serial.print(c); 
  Serial.println(); 
}
// -- Funktionen ----------------------------
void searchNextError(){
  for(int i=0;i<=ERRANZAHL;i++){
    uiActErrIdx++;
    uiActErrIdx=uiActErrIdx % ERRANZAHL;
    if(gc_Error[uiActErrIdx]!= "")
      return;
  }
}

//----------------- IP Adresse umwandeln
void IP4_String_to_Bytes(const char* str, char sep, byte* bytes, int base) {
  for (int i = 0; i < 4; i++) {
    bytes[i] = strtoul(str, NULL, base);  // Convert byte
    str = strchr(str, sep);               // Find next separator
    if (str == NULL || *str == '\0') {
      break;                            // No more separators, exit
    }
    str++;                                // Point to next character after separator
  }
}
//----------------- 
void IP4_Bytes_to_String(char* str, byte* bytes,int base) {
  int i;
  char tmp[5];
  *str='\0';           // clear string
  for ( i = 0; i < 3; i++) {
    itoa(bytes[i],tmp,base);
    strcat(str,tmp);               // add byte 
    strcat(str,".");
  }
  itoa(bytes[i],tmp,base); // add last byte
  strcat(str,tmp);  
}

//-----------------------------


#endif
