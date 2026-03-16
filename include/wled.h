#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"
#include "logging.h"

// ----------------------------------------------------------------
// WLED – An/Aus per JSON-API
//
// POST /json/state  {"on":true}  → einschalten
// POST /json/state  {"on":false} → ausschalten
//
// Zwei Instanzen: innen + außen
// Konfiguration wird in LittleFS als /wled.json gespeichert
// ----------------------------------------------------------------

struct WledInstance
{
    char ip[32];
    int  port;
    bool lastState;     // letzter bekannter Zustand
};

struct WledConfig
{
    WledInstance innen;
    WledInstance aussen;
};

extern WledConfig    wledConfig;
extern volatile bool wledInnenState;   // aktueller Zustand (vom Poll-Task aktualisiert)
extern volatile bool wledAussenState;

void    wledSetup();                               // Konfiguration laden
bool    wledSetState(WledInstance &inst, bool on); // An/Aus senden
bool    wledGetState(WledInstance &inst);          // Zustand abfragen (aktualisiert lastState)
void    wledLoop();                                // in loop() aufrufen, pollt in Intervallen
bool    wledSaveConfig();                          // Konfiguration speichern
bool    wledLoadConfig();                          // Konfiguration laden
String  wledToJson();                              // Status als JSON für REST-API

// Polling-Intervall: alle 10 Sekunden tatsächlichen Zustand abfragen
#define WLED_POLL_INTERVAL_MS 10000

// Convenience-Wrapper
inline bool wledInnenOn()   { return wledSetState(wledConfig.innen,  true);  }
inline bool wledInnenOff()  { return wledSetState(wledConfig.innen,  false); }
inline bool wledAussenOn()  { return wledSetState(wledConfig.aussen, true);  }
inline bool wledAussenOff() { return wledSetState(wledConfig.aussen, false); }
