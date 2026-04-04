#include "sensorpoll.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <climits>
#include "appconfig.h"
#include "logging.h"
#include "wifi.h"

SensorData sensorData = {};
RingEntry *ringBuffer = nullptr;
uint32_t   ringHead   = 0; //erster freier eintrag, bzw. der nächste zu setzende
uint32_t   ringCount  = 0;
// Global am Anfang:
bool sensorDataUpdated = false;


RingStats ringStats = {};
void calcRingStats(uint32_t hours)
{
    ringStats.valid = false;
    if (!ringBuffer || ringCount == 0) return;  //kein eintrag zurück

    uint32_t maxEntries = (hours * 3600 * 1000) / appConfig.sensor_poll_interval_ms;
    uint32_t entries    = min(ringCount, maxEntries);

    uint32_t bme_cnt = 0, ve_cnt = 0, mppt1_cnt = 0, mppt2_cnt = 0, charger_cnt = 0,scd_cnt=0;

    // Startwerte
    uint32_t first = (ringHead + RING_MAX_ENTRIES - 1) % RING_MAX_ENTRIES; //letzter gesetzter eintrag
    RingEntry &f = ringBuffer[first];

    //setzte die Min/Max- Werte auf den letzten Verfügbaren eintrag
    ringStats.t_min = ringStats.t_max = f.T;
    ringStats.h_min = ringStats.h_max = f.H;
    ringStats.p_min = ringStats.p_max = f.P;
    ringStats.co2_min = ringStats.co2_max = f.CO2;
    ringStats.v_min = ringStats.v_max = f.V;
    ringStats.i_min = ringStats.i_max = f.I;
    ringStats.soc_min = ringStats.soc_max = f.SOC;
    ringStats.pw_min = ringStats.pw_max = f.PW;
    ringStats.vs_min = ringStats.vs_max = f.VS;
    ringStats.mppt1_v_min  = ringStats.mppt1_v_max  = f.mppt1_V;
    ringStats.mppt1_i_min  = ringStats.mppt1_i_max  = f.mppt1_I;
    ringStats.mppt1_pv_min = ringStats.mppt1_pv_max = f.mppt1_PV;
    ringStats.mppt2_v_min  = ringStats.mppt2_v_max  = f.mppt2_V;
    ringStats.mppt2_i_min  = ringStats.mppt2_i_max  = f.mppt2_I;
    ringStats.mppt2_pv_min = ringStats.mppt2_pv_max = f.mppt2_PV;
    ringStats.charger_v_min = ringStats.charger_v_max = f.charger_V;
    ringStats.charger_i_min = ringStats.charger_i_max = f.charger_I;

    // Avgs zurücksetzen
    ringStats.t_avg = ringStats.h_avg = ringStats.p_avg = ringStats.co2_avg = 0;
    ringStats.v_avg = ringStats.i_avg = ringStats.soc_avg = ringStats.pw_avg = ringStats.vs_avg = 0;
    ringStats.mppt1_v_avg = ringStats.mppt1_i_avg = ringStats.mppt1_pv_avg = 0;
    ringStats.mppt2_v_avg = ringStats.mppt2_i_avg = ringStats.mppt2_pv_avg = 0;
    ringStats.charger_v_avg = ringStats.charger_i_avg = 0;

    for (uint32_t i = 0; i < entries; i++) //entries passens zum zeitintervall
    {
        //laufe von hinten durch den ringbuffer starte eins vor ringHead
        uint32_t idx = (ringHead + RING_MAX_ENTRIES - 1 - i) % RING_MAX_ENTRIES;
        RingEntry &e = ringBuffer[idx];

        // BME:
        if (e.valid_flags & VALID_BME)
        {
            bme_cnt++;
            ringStats.t_min = min(ringStats.t_min, e.T);
            ringStats.t_max = max(ringStats.t_max, e.T);
            ringStats.t_avg += e.T;
            ringStats.h_min = min(ringStats.h_min, e.H);
            ringStats.h_max = max(ringStats.h_max, e.H);
            ringStats.h_avg += e.H;
            ringStats.p_min = min(ringStats.p_min, e.P);
            ringStats.p_max = max(ringStats.p_max, e.P);
            ringStats.p_avg += e.P;
        }
        if (e.valid_flags & VALID_CO2)
        {
            scd_cnt++;
            ringStats.co2_min = min(ringStats.co2_min, e.CO2);
            ringStats.co2_max = max(ringStats.co2_max, e.CO2);
            ringStats.co2_avg += e.CO2;
        }

        // VE.Direct:
        if (e.valid_flags & VALID_VE)
        {
            ve_cnt++;
            ringStats.v_min = min(ringStats.v_min, e.V);
            ringStats.v_max = max(ringStats.v_max, e.V);
            ringStats.v_avg += e.V;
            ringStats.i_min = min(ringStats.i_min, e.I);
            ringStats.i_max = max(ringStats.i_max, e.I);
            ringStats.i_avg += e.I;
            ringStats.soc_min = min(ringStats.soc_min, e.SOC);
            ringStats.soc_max = max(ringStats.soc_max, e.SOC);
            ringStats.soc_avg += e.SOC;
            ringStats.pw_min = min(ringStats.pw_min, e.PW);
            ringStats.pw_max = max(ringStats.pw_max, e.PW);
            ringStats.pw_avg += e.PW;
            ringStats.vs_min = min(ringStats.vs_min, e.VS);
            ringStats.vs_max = max(ringStats.vs_max, e.VS);
            ringStats.vs_avg += e.VS;
        }

        // MPPT1: 
        if (e.valid_flags & VALID_MPPT1)
        {
            mppt1_cnt++;
            ringStats.mppt1_v_min  = min(ringStats.mppt1_v_min,  e.mppt1_V);
            ringStats.mppt1_v_max  = max(ringStats.mppt1_v_max,  e.mppt1_V);
            ringStats.mppt1_v_avg  += e.mppt1_V;
            ringStats.mppt1_i_min  = min(ringStats.mppt1_i_min,  e.mppt1_I);
            ringStats.mppt1_i_max  = max(ringStats.mppt1_i_max,  e.mppt1_I);
            ringStats.mppt1_i_avg  += e.mppt1_I;
            ringStats.mppt1_pv_min = min(ringStats.mppt1_pv_min, e.mppt1_PV);
            ringStats.mppt1_pv_max = max(ringStats.mppt1_pv_max, e.mppt1_PV);
            ringStats.mppt1_pv_avg += e.mppt1_PV;
        }

        // MPPT2: 
        if (e.valid_flags & VALID_MPPT2)
        {
            mppt2_cnt++;
            ringStats.mppt2_v_min  = min(ringStats.mppt2_v_min,  e.mppt2_V);
            ringStats.mppt2_v_max  = max(ringStats.mppt2_v_max,  e.mppt2_V);
            ringStats.mppt2_v_avg  += e.mppt2_V;
            ringStats.mppt2_i_min  = min(ringStats.mppt2_i_min,  e.mppt2_I);
            ringStats.mppt2_i_max  = max(ringStats.mppt2_i_max,  e.mppt2_I);
            ringStats.mppt2_i_avg  += e.mppt2_I;
            ringStats.mppt2_pv_min = min(ringStats.mppt2_pv_min, e.mppt2_PV);
            ringStats.mppt2_pv_max = max(ringStats.mppt2_pv_max, e.mppt2_PV);
            ringStats.mppt2_pv_avg += e.mppt2_PV;
        }

        // Charger: 
        if (e.valid_flags & VALID_CHARGER)
        {
            charger_cnt++;
            ringStats.charger_v_min = min(ringStats.charger_v_min, e.charger_V);
            ringStats.charger_v_max = max(ringStats.charger_v_max, e.charger_V);
            ringStats.charger_v_avg += e.charger_V;
            ringStats.charger_i_min = min(ringStats.charger_i_min, e.charger_I);
            ringStats.charger_i_max = max(ringStats.charger_i_max, e.charger_I);
            ringStats.charger_i_avg += e.charger_I;
        }
    } //ende der Schleife

    // Durchschnitte berechnen, noch die division durch die anzahl gültiger werte, wenn > 0, sonst 0
    if (bme_cnt > 0) {
        ringStats.t_avg   /= bme_cnt;
        ringStats.h_avg   /= bme_cnt;
        ringStats.p_avg   /= bme_cnt;
    } else {
        ringStats.t_avg = ringStats.h_avg = ringStats.p_avg = ringStats.co2_avg = 0;
    }

    if (scd_cnt > 0) {
        ringStats.co2_avg /= scd_cnt;
    } else {
        ringStats.co2_avg = 0;
    }   

    if (ve_cnt > 0) {
        ringStats.v_avg   /= ve_cnt;
        ringStats.i_avg   /= ve_cnt;
        ringStats.soc_avg /= ve_cnt;
        ringStats.pw_avg  /= ve_cnt;
        ringStats.vs_avg  /= ve_cnt;
    } else {
        ringStats.v_avg = ringStats.i_avg = ringStats.soc_avg = ringStats.pw_avg = ringStats.vs_avg = 0;
    }

    if (mppt1_cnt > 0) {
        ringStats.mppt1_v_avg  /= mppt1_cnt;
        ringStats.mppt1_i_avg  /= mppt1_cnt;
        ringStats.mppt1_pv_avg /= mppt1_cnt;
    }

    if (mppt2_cnt > 0) {
        ringStats.mppt2_v_avg  /= mppt2_cnt;
        ringStats.mppt2_i_avg  /= mppt2_cnt;
        ringStats.mppt2_pv_avg /= mppt2_cnt;
    }

    if (charger_cnt > 0) {
        ringStats.charger_v_avg /= charger_cnt;
        ringStats.charger_i_avg /= charger_cnt;
    } else {
        ringStats.charger_v_avg = ringStats.charger_i_avg = 0;
    }

    ringStats.hours = hours;
    ringStats.valid = true;
    ringStats.valid_sensors = 0;
    if (bme_cnt > 0)     ringStats.valid_sensors |= VALID_BME;
    if (ve_cnt > 0)      ringStats.valid_sensors |= VALID_VE;
    if (mppt1_cnt > 0)   ringStats.valid_sensors |= VALID_MPPT1;
    if (mppt2_cnt > 0)   ringStats.valid_sensors |= VALID_MPPT2;
    if (charger_cnt > 0) ringStats.valid_sensors |= VALID_CHARGER;
    if (scd_cnt > 0)     ringStats.valid_sensors |= VALID_CO2;
}

void sensorPollSetup()
{
    ringBuffer = (RingEntry*)ps_malloc(RING_MAX_ENTRIES * sizeof(RingEntry));
    memset(ringBuffer, 0, RING_MAX_ENTRIES * sizeof(RingEntry)); 
    if (!ringBuffer)
        logPrintln("SensorPoll: Ringpuffer FEHLER");
    else
        logPrintf("SensorPoll: Ringpuffer OK, %lu KB\n",
            (unsigned long)(RING_MAX_ENTRIES * sizeof(RingEntry)) / 1024);
}

void sensorPollLoop()
{
    if (!wifiIsConnected()) return;

    static uint32_t lastPollMs = 0;
    if (millis() - lastPollMs < appConfig.sensor_poll_interval_ms) return;
    
    lastPollMs = millis();

    HTTPClient http;
    String url = "http://" + String(appConfig.sensor_esp_ip) + "/api/data";
    http.begin(url);
    http.setTimeout(2000);
    http.setConnectTimeout(2000);
    int code = http.GET();
    if (code != 200)
    {
        logPrintf("SensorPoll: HTTP Fehler %d\n", code);
        http.end();
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();

    if (err)
    {
        logPrintln("SensorPoll: JSON Fehler");
        return;
    }

    // BME280
    sensorData.bme_valid = doc["bme"]["valid"] | false;
    if (sensorData.bme_valid)
    {
        sensorData.temperature = doc["bme"]["T"] | 0.0f;
        sensorData.humidity    = doc["bme"]["H"] | 0.0f;
        sensorData.pressure    = doc["bme"]["P"] | 0.0f;
    }

    // VE.Direct - nicht mehr, aber auf dieser schnittstelle, wobei der sensor_esp per ble lesen wird, wenns klappt
    sensorData.vedirect_valid = doc["vedirect"]["valid"] | false;
    if (sensorData.vedirect_valid)
    {
        sensorData.voltage         = doc["vedirect"]["V"]   | 0.0f;
        sensorData.current         = doc["vedirect"]["I"]   | 0.0f;
        sensorData.power           = doc["vedirect"]["P"]   | 0.0f;
        sensorData.soc             = doc["vedirect"]["SOC"] | 0.0f;
        sensorData.ttg             = doc["vedirect"]["TTG"] | -1;
        sensorData.voltage_starter = doc["vedirect"]["VS"]  | 0.0f;
    }
    // MPPT1
    sensorData.mppt1_valid = doc["mppt1"]["valid"] | false;
    if (sensorData.mppt1_valid)
    {
        sensorData.mppt1_voltage     = doc["mppt1"]["V"]     | 0.0f;
        sensorData.mppt1_current     = doc["mppt1"]["I"]     | 0.0f;
        sensorData.mppt1_pv_power    = doc["mppt1"]["PV"]    | 0.0f;
        sensorData.mppt1_yield_today = doc["mppt1"]["yield"] | 0;
        strlcpy(sensorData.mppt1_stateStr, doc["mppt1"]["stateStr"] | "---", sizeof(sensorData.mppt1_stateStr));
    }

    // MPPT2
    sensorData.mppt2_valid = doc["mppt2"]["valid"] | false;
    if (sensorData.mppt2_valid)
    {
        sensorData.mppt2_voltage     = doc["mppt2"]["V"]     | 0.0f;
        sensorData.mppt2_current     = doc["mppt2"]["I"]     | 0.0f;
        sensorData.mppt2_pv_power    = doc["mppt2"]["PV"]    | 0.0f;
        sensorData.mppt2_yield_today = doc["mppt2"]["yield"] | 0;
        strlcpy(sensorData.mppt2_stateStr, doc["mppt2"]["stateStr"] | "---", sizeof(sensorData.mppt2_stateStr));
    }

    // Charger (Blue Smart IP22)
    sensorData.charger_valid = doc["charger"]["valid"] | false;
    if (sensorData.charger_valid)
    {
        sensorData.charger_voltage = doc["charger"]["V"]        | 0.0f;
        sensorData.charger_current = doc["charger"]["I"]        | 0.0f;
        sensorData.charger_state   = doc["charger"]["state"]    | 0;
        strlcpy(sensorData.charger_stateStr, doc["charger"]["stateStr"] | "---", sizeof(sensorData.charger_stateStr));
    }

    // CO2
    sensorData.co2_valid = doc["co2"]["valid"] | false;
    if (sensorData.co2_valid)
        sensorData.co2_ppm = doc["co2"]["ppm"] | 0;
    
    sensorDataUpdated = true; //neue daten da, kann ui aktualisieren
    
    //static uint32_t lastRingMs = 0;
    if (ringBuffer)// (millis() - lastRingMs >= RING_INTERVAL_MS && ringBuffer)
    {
        //lastRingMs = millis();
        RingEntry &e = ringBuffer[ringHead];
        e.valid_flags = 0;
        if (sensorData.bme_valid)      e.valid_flags |= VALID_BME;
        if (sensorData.vedirect_valid) e.valid_flags |= VALID_VE;
        if (sensorData.mppt1_valid)    e.valid_flags |= VALID_MPPT1;
        if (sensorData.mppt2_valid)    e.valid_flags |= VALID_MPPT2;
        if (sensorData.charger_valid)  e.valid_flags |= VALID_CHARGER;
        if (sensorData.co2_valid) e.valid_flags |= VALID_CO2;

        e.T   = sensorData.temperature;
        e.H   = sensorData.humidity;
        e.P   = sensorData.pressure;
        e.CO2 = sensorData.co2_ppm;
        e.V   = sensorData.voltage;
        e.I   = sensorData.current;
        e.SOC = sensorData.soc;
        e.PW  = sensorData.power;
        e.VS  = sensorData.voltage_starter;
        e.mppt1_V     = sensorData.mppt1_voltage;
        e.mppt1_I     = sensorData.mppt1_current;
        e.mppt1_PV    = sensorData.mppt1_pv_power;
        e.mppt1_yield = sensorData.mppt1_yield_today;
        e.mppt2_V     = sensorData.mppt2_voltage;
        e.mppt2_I     = sensorData.mppt2_current;
        e.mppt2_PV    = sensorData.mppt2_pv_power;
        e.mppt2_yield = sensorData.mppt2_yield_today;
        e.charger_V   = sensorData.charger_voltage;
        e.charger_I   = sensorData.charger_current;
        ringHead = (ringHead + 1) % RING_MAX_ENTRIES;
        if (ringCount < RING_MAX_ENTRIES) ringCount++;
    }    

    
    calcRingStats(ringStats.hours > 0 ? ringStats.hours : 12);
    
    //logPrintln("SensorPoll: OK");
}