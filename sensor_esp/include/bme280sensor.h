#pragma once
#include <Arduino.h>
#include "config.h"
#include "logging.h"

// ----------------------------------------------------------------
// BME280 – Temperatur, Luftdruck, Luftfeuchtigkeit
// Kommunikation: I²C
// Bibliothek:    Adafruit BME280
// ----------------------------------------------------------------

struct Bme280Data
{
    float    temperature;   // °C
    float    humidity;      // %
    float    pressure;      // hPa

    bool     valid;         // true sobald erste Messung erfolgreich
    uint32_t lastUpdateMs;  // millis() der letzten Messung
};

extern Bme280Data bme280Data;

bool    bme280Setup();      // false wenn Sensor nicht gefunden
void    bme280Loop();       // in loop() aufrufen, prüft Intervall selbst
bool    bme280IsValid();    // true wenn Daten vorhanden und nicht zu alt
String  bme280ToJson();     // JSON-String für REST-API
