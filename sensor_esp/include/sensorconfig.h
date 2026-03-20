#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
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
    int     vedirect_rx;
    int     vedirect_baud;
};

extern SensorConfig sensorConfig;

bool sensorConfigLoad();
bool sensorConfigSave();
String sensorConfigToJson();
