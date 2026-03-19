#pragma once
#include <Arduino.h>
#include "config.h"
#include "logging.h"

// ----------------------------------------------------------------
// VE.Direct Text-Mode Parser für Victron BMV712
//
// Der BMV sendet alle ~1 Sekunde einen Block mit Key-Value-Paaren:
//   <Label>\t<Value>\r\n
//   ...
//   Checksum\t<byte>\r\n
//
// Baud: 19200, 8N1, nur RX nötig (wir lesen nur)
// ----------------------------------------------------------------

struct BmvData
{
    float   voltage;        // V   – Batteriespannung in V  (BMV: mV / 1000)
    float   current;        // I   – Strom in A             (BMV: mA / 1000)
    float   voltageStarter; // VS  – Starterbatterie in V   (BMV: mV / 1000)
    float   soc;            // SOC – State of Charge in %   (BMV: promille / 10)
    int32_t timeToGo;       // TTG – Restlaufzeit in Min    (-1 = unbekannt)
    float   power;          // berechnet: V * I in W

    bool    valid;          // true sobald ein kompletter Block empfangen wurde
    uint32_t lastUpdateMs;  // millis() des letzten gültigen Blocks
};

extern BmvData bmvData;

void    vedirectSetup();
void    vedirectLoop();         // in loop() aufrufen
bool    vedirectIsValid();      // true wenn Daten vorhanden und nicht zu alt
String  vedirectToJson();       // JSON-String für REST-API
