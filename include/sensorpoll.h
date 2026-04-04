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
    char    mppt1_stateStr[24];
    // MPPT2
    bool    mppt2_valid;
    float   mppt2_voltage;
    float   mppt2_current;
    float   mppt2_pv_power;
    uint16_t mppt2_yield_today;
    char    mppt2_stateStr[24];
    // Charger (Blue Smart IP22)
    bool    charger_valid;
    float   charger_voltage;
    float   charger_current;
    uint8_t charger_state;
    char    charger_stateStr[24];
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
    // MPPT1
    float mppt1_v_min,  mppt1_v_max,  mppt1_v_avg;
    float mppt1_i_min,  mppt1_i_max,  mppt1_i_avg;
    float mppt1_pv_min, mppt1_pv_max, mppt1_pv_avg;
    // MPPT2
    float mppt2_v_min,  mppt2_v_max,  mppt2_v_avg;
    float mppt2_i_min,  mppt2_i_max,  mppt2_i_avg;
    float mppt2_pv_min, mppt2_pv_max, mppt2_pv_avg;
    // Charger
    float charger_v_min, charger_v_max, charger_v_avg;
    float charger_i_min, charger_i_max, charger_i_avg;
    uint32_t hours;
    bool valid; //überhaupt daten
    uint8_t valid_sensors;  //flags für die sensorarten, damit ui weiß, was es anzeigen kann
};

#define RING_MAX_ENTRIES    75600  // 42h × 1800 Einträge/h, alle 2 sek

struct RingEntry {
    float T, H, P;
    int   CO2;
    float V, I, SOC, PW, VS;
    // MPPT
    float    mppt1_V, mppt1_I, mppt1_PV;
    float    mppt2_V, mppt2_I, mppt2_PV;
    uint16_t mppt1_yield, mppt2_yield;
    float    charger_V, charger_I;
    uint8_t valid_flags; //flags
};

#define VALID_BME     (1<<0) 
/* bitfeld spart platz 
1 << 0 = 00000001 = 1
1 << 1 = 00000010 = 2
*/
#define VALID_VE      (1<<1)
#define VALID_MPPT1   (1<<2)
#define VALID_MPPT2   (1<<3)
#define VALID_CHARGER (1<<4)
#define VALID_CO2     (1<<5)

extern bool sensorDataUpdated; //bei update ui aktualisieren
extern RingEntry *ringBuffer;
extern RingStats ringStats;
extern uint32_t   ringHead;   // nächste Schreibposition
extern uint32_t   ringCount;  // belegte Einträge

extern SensorData sensorData;

void sensorPollSetup();
void sensorPollLoop();
void calcRingStats(uint32_t hours);
