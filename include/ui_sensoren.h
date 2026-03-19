#pragma once
#include <Arduino.h>
#include <lvgl.h>
//#include "vedirect.h"
//#include "bme280sensor.h"

// ----------------------------------------------------------------
// ui_sensoren – Tab 1: Batterie (BMV712) + Klima (BME280/SCD41)
// ----------------------------------------------------------------

// Initialisiert alle Widgets im übergebenen Tab-Objekt
void uiSensorenSetup(lv_obj_t *tab);

// Aktualisiert alle Werte – aus loop() aufrufen
void uiSensorenUpdate();

// IP-Adresse im Klima-Panel setzen
void uiSensorenSetIP(const String &ip);
