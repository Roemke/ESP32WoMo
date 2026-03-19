#include <Arduino.h>
#include <LittleFS.h>
#include "logging.h"
#include "wifi.h"
#include "sensorconfig.h"
#include "bme280sensor.h"

void setup() {
    Serial.begin(115200);
    delay(5000);
    Serial.println("=== Sensor ESP startet ===");
    
    if (!LittleFS.begin(true))
        Serial.println("LittleFS FEHLER");
    else
        Serial.println("LittleFS OK");

    sensorConfigLoad();
    Serial.println("Config OK");

    wifiSetup();
    Serial.println("WiFi OK");

    
    if (!bme280Setup())
        Serial.println("BME280 FEHLER");
    else
        Serial.println("BME280 OK");
    
}

void loop() {
    Serial.println("loop");
    bme280Loop();
    delay(1000);
}