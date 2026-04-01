#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"

#define BLE_CONFIG_PATH "/bleconfig.json"
#define BLE_CONFIG_VERSION 1

struct BleConfig
{
    uint8_t version;
    char    bmv_mac[18];       // "AA:BB:CC:DD:EE:FF"
    char    bmv_bindkey[33];   // 32 hex Zeichen AES Key
    char    mppt1_mac[18];     // optional, leer = nicht genutzt
    char    mppt1_bindkey[33];
    char    mppt2_mac[18];
    char    mppt2_bindkey[33];
    char    charger_mac[18];       // Blue Smart IP22 o.ä.
    char    charger_bindkey[33];
};

extern BleConfig bleConfig;

bool   bleConfigLoad();
bool   bleConfigSave();
String bleConfigToJson();
