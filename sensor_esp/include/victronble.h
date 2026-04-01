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

// Ladegerät-Daten vom Blue Smart IP22 (Record-Type 0x08 = AC_CHARGER)
struct ChargerData {
    bool    valid;
    float   battery_voltage;  // Ausgangsspannung in V
    float   battery_current;  // Ausgangsstrom in A
    uint8_t charge_state;
    uint8_t error_code;
    uint32_t lastUpdateMs;
};

extern BmvData     bmvData;
extern MpptData    mppt1Data;
extern MpptData    mppt2Data;
extern ChargerData chargerData;

void victronBleSetup();
void victronBleLoop();
String bmvToJson();
String mppt1ToJson();
String mppt2ToJson();
String chargerToJson();
