#pragma once
#include <Arduino.h>
#include <lvgl.h>

// ----------------------------------------------------------------
// ui_charger – Tab 2: Charger (IP22) + MPPT1 + MPPT2
// ----------------------------------------------------------------

void uiChargerSetup(lv_obj_t *tab);
void uiChargerUpdate();
