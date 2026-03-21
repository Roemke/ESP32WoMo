#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "logging.h"

// Gespeicherte WiFi-Credentials
struct WifiData
{
    char    ssid[32];
    char    password[64];
    bool    use_static_ip;
    char    static_ip[16];
    char    subnet[16];
    uint8_t magic;
};

extern WifiData wifiData;

extern String wifiMode;
extern String wifiMacAp;
extern String wifiMacSta;

// Öffentliche API
void     wifiSetup();
bool     wifiIsConnected();
String   wifiGetIP();
void     wifiSetCredentials(const char *ssid, const char *password);
bool     wifiTimeValid();
String   wifiGetTime();