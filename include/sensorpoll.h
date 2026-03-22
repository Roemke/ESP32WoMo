#pragma once
#include <Arduino.h>

// Sensordaten die vom Sensor-ESP geholt werden
struct SensorData
{
    // BME280
    bool    bme_valid;
    float   temperature;
    float   humidity;
    float   pressure;
    // VE.Direct / BMV712
    bool    vedirect_valid;
    float   voltage;
    float   current;
    float   power;
    float   soc;
    int     ttg;
    float   voltage_starter;
    // CO2
    bool    co2_valid;
    int     co2_ppm;
    // MPPT1
    bool    mppt1_valid;
    float   mppt1_voltage;
    float   mppt1_current;
    float   mppt1_pv_power;
    uint16_t mppt1_yield_today;
    // MPPT2
    bool    mppt2_valid;
    float   mppt2_voltage;
    float   mppt2_current;
    float   mppt2_pv_power;
    uint16_t mppt2_yield_today;
};

struct RingStats {
    float t_min,  t_max,  t_avg;
    float h_min,  h_max,  h_avg;
    float p_min,  p_max,  p_avg;
    int   co2_min,co2_max,co2_avg;
    float v_min,  v_max,  v_avg;
    float i_min,  i_max,  i_avg;
    float soc_min,soc_max,soc_avg;
    float pw_min, pw_max, pw_avg;
    float vs_min, vs_max, vs_avg;
    uint32_t hours; // für welchen Zeitraum
    bool valid;
};


#define RING_INTERVAL_MS    2000
#define RING_MAX_ENTRIES    86400  // 48h × 1800 Einträge/h

struct RingEntry {
    float T, H, P;
    int   CO2;
    float V, I, SOC, PW, VS;
    // MPPT
    float mppt1_V, mppt1_I, mppt1_PV;
    float mppt2_V, mppt2_I, mppt2_PV;
    uint16_t mppt1_yield, mppt2_yield;
};

extern RingEntry *ringBuffer;
extern RingStats ringStats;
extern uint32_t   ringHead;   // nächste Schreibposition
extern uint32_t   ringCount;  // belegte Einträge

extern SensorData sensorData;

void sensorPollSetup();
void sensorPollLoop();
void calcRingStats(uint32_t hours);
