#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"

#define APP_CONFIG_PATH "/appconfig.json"

struct AppConfig
{
    char sensor_esp_ip[16];
    char wled_innen_ip[16];
    char wled_aussen_ip[16];
    char bmv_mac[18];      // "AA:BB:CC:DD:EE:FF"
    char bmv_bindkey[33];  // 32 hex Zeichen AES Key
};

extern AppConfig appConfig;

void appConfigLoad();
void appConfigSave();
String appConfigToJson();
