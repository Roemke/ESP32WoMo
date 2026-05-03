#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

// ----------------------------------------------------------------
// Laufzeit-Konfiguration – wird aus LittleFS geladen
// Defaults kommen aus config.h
// ----------------------------------------------------------------
struct SensorConfig
{
    uint8_t version;        
    int     bme_sda;
    int     bme_scl;
    uint8_t bme_addr;
    int     bme_interval_ms;    
    int     gas_raw_min; //um gaskalibrierung einfacher zu machen
    int     gas_raw_max;
    bool    gas_enabled;
};

extern SensorConfig sensorConfig;

bool sensorConfigLoad();
bool sensorConfigSave();
String sensorConfigToJson();
