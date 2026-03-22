#pragma once
#include <Arduino.h>

// Batterie-Daten vom BMV712
struct BmvData {
    bool    valid;
    float   voltage;
    float   current;
    float   power;
    float   soc;
    int     ttg;           // Restlaufzeit in Minuten
    float   voltage_starter;
    float   consumed_ah;
    uint32_t lastUpdateMs;
};

// Solar-Daten vom MPPT
struct MpptData {
    bool    valid;
    float   battery_voltage;
    float   battery_current;
    float   panel_power;
    uint16_t yield_today;  // Wh
    uint8_t charge_state;
    uint32_t lastUpdateMs;
};

extern BmvData  bmvData;
extern MpptData mppt1Data;
extern MpptData mppt2Data;

void victronBleSetup();
void victronBleLoop();
String bmvToJson();
String mppt1ToJson();
String mppt2ToJson();
