#pragma once
#include <Arduino.h>
#include <lvgl.h>

// Initialisiert Klima-History Tab
void uiHistoryKlimaSetup(lv_obj_t *tab);

// Initialisiert Batterie-History Tab  
void uiHistoryBatSetup(lv_obj_t *tab);

void uiHistoryLoop();