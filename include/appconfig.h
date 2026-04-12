#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

struct AppConfig
{
    char sensor_esp_ip[16];
    char wled_innen_ip[16];
    char wled_aussen_ip[16];
    uint32_t sensor_poll_interval_ms;
    uint8_t display_brightness;   // 0-100, Default DISPLAY_BRIGHTNESS_DEFAULT 
    uint8_t indicator_opacity;    // 0-100, Default INDICATOR_OPACITY_DEFAULT 
};

extern AppConfig appConfig;

void appConfigLoad();
void appConfigSave();

String appConfigToJson();
