#pragma once
#include <Arduino.h>
#include "config.h"
#include "logging.h"

// ----------------------------------------------------------------
// SD-Karte – Messwerte in festen Intervallen als CSV speichern
//
// Dateistruktur:
//   /data/YYYY-MM-DD.csv  – eine Datei pro Tag
//
// CSV-Format:
//   timestamp,V,I,VS,SOC,TTG,P,T,H,P_hPa
// ----------------------------------------------------------------

bool    sdSetup();      // false wenn keine SD-Karte gefunden
void    sdLoop();       // in loop() aufrufen, prüft Intervall selbst
bool    sdIsReady();    // true wenn SD-Karte initialisiert
String  sdGetStatus();  // freier Speicher etc. als JSON

//history kram 
void        sdGetHistoryStream(const String &from, const String &to, int points, Print &output);
const char* sdGetHistoryBuffer(const String &from, const String &to, int points);

//beim starten prüfen, ob ich etwas in den ringbuffer schieben kann. 
void sdFillRingBuffer(uint32_t hours);