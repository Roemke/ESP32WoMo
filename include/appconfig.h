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
    uint32_t sensor_poll_interval_ms;
};

extern AppConfig appConfig;

void appConfigLoad();
void appConfigSave();
String appConfigToJson();
