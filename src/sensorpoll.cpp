#include "sensorpoll.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <climits>
#include "appconfig.h"
#include "logging.h"
#include "wifi.h"

SensorData sensorData = {};
RingEntry *ringBuffer = nullptr;
uint32_t   ringHead   = 0;
uint32_t   ringCount  = 0;

RingStats ringStats = {};

void calcRingStats(uint32_t hours)
{
    ringStats.valid = false;
    if (!ringBuffer || ringCount == 0) return;

    // Maximale Anzahl Einträge für den Zeitraum
    uint32_t maxEntries = (hours * 3600 * 1000) / RING_INTERVAL_MS;
    uint32_t entries    = min(ringCount, maxEntries);
    uint32_t bme_cnt = 0, ve_cnt = 0;
    
    // Startwerte
    uint32_t first = (ringHead + RING_MAX_ENTRIES - 1) % RING_MAX_ENTRIES;
    RingEntry &f = ringBuffer[first];

    ringStats.t_min = ringStats.t_max = f.T;
    ringStats.h_min = ringStats.h_max = f.H;
    ringStats.p_min = ringStats.p_max = f.P;
    ringStats.co2_min = ringStats.co2_max = f.CO2;
    ringStats.v_min = ringStats.v_max = f.V;
    ringStats.i_min = ringStats.i_max = f.I;
    ringStats.soc_min = ringStats.soc_max = f.SOC;
    ringStats.pw_min = ringStats.pw_max = f.PW;
    ringStats.vs_min = ringStats.vs_max = f.VS;
    
    // Rückwärts durch den Ringpuffer – neueste Einträge zuerst
    ringStats.t_avg = ringStats.h_avg = ringStats.p_avg = 0;
    ringStats.co2_avg = 0;
    ringStats.v_avg = ringStats.i_avg = ringStats.soc_avg = ringStats.pw_avg = ringStats.vs_avg = 0;
    
    for (uint32_t i = 0; i < entries; i++)
    {
        uint32_t idx = (ringHead + RING_MAX_ENTRIES - 1 - i) % RING_MAX_ENTRIES;
        RingEntry &e = ringBuffer[idx];

        // Nur gültige Werte berücksichtigen
        if (sensorData.bme_valid)
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

            ringStats.co2_min = min(ringStats.co2_min, e.CO2);
            ringStats.co2_max = max(ringStats.co2_max, e.CO2);
            ringStats.co2_avg += e.CO2;
        }

        if (sensorData.vedirect_valid)
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
    }

    // Durchschnitt berechnen
    if (bme_cnt > 0)
    {
        ringStats.t_avg   /= bme_cnt;
        ringStats.h_avg   /= bme_cnt;
        ringStats.p_avg   /= bme_cnt;
        ringStats.co2_avg /= bme_cnt;
    }
    else{
        ringStats.t_avg = ringStats.h_avg = ringStats.p_avg = ringStats.co2_avg = 0;
    }
    if(ve_cnt > 0)
    {
        ringStats.v_avg   /= ve_cnt;
        ringStats.i_avg   /= ve_cnt;
        ringStats.soc_avg /= ve_cnt;
        ringStats.pw_avg  /= ve_cnt;
        ringStats.vs_avg  /= ve_cnt;
    }
    else 
        ringStats.v_avg = ringStats.i_avg = ringStats.soc_avg = ringStats.pw_avg = ringStats.vs_avg = 0;


    ringStats.hours = hours;
    ringStats.valid = true;
    //logPrintf("calcRingStats: %lu Einträge, %lu Stunden\n", entries, hours);
}

void sensorPollSetup()
{
    ringBuffer = (RingEntry*)ps_malloc(RING_MAX_ENTRIES * sizeof(RingEntry));
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
    if (millis() - lastPollMs < SENSOR_POLL_INTERVAL_MS) return;
    lastPollMs = millis();

    HTTPClient http;
    String url = "http://" + String(appConfig.sensor_esp_ip) + "/api/data";
    http.begin(url);
    http.setTimeout(3000);

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

    // VE.Direct
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

    // CO2
    sensorData.co2_valid = doc["co2"]["valid"] | false;
    if (sensorData.co2_valid)
        sensorData.co2_ppm = doc["co2"]["ppm"] | 0;
    
    static uint32_t lastRingMs = 0;
    if (millis() - lastRingMs >= RING_INTERVAL_MS && ringBuffer)
    {
        lastRingMs = millis();
        RingEntry &e = ringBuffer[ringHead];
        e.T   = sensorData.temperature;
        e.H   = sensorData.humidity;
        e.P   = sensorData.pressure;
        e.CO2 = sensorData.co2_ppm;
        e.V   = sensorData.voltage;
        e.I   = sensorData.current;
        e.SOC = sensorData.soc;
        e.PW  = sensorData.power;
        e.VS  = sensorData.voltage_starter;
        ringHead = (ringHead + 1) % RING_MAX_ENTRIES;
        if (ringCount < RING_MAX_ENTRIES) ringCount++;
    }    

    //denn ringbuffer für statistik berechnen, sollte hier passen und nicht stören, nicht on demand berechnen    
    static uint32_t lastStatsMs = 0;
    if (millis() - lastStatsMs > 1000)
    {
        lastStatsMs = millis();
        calcRingStats(ringStats.hours > 0 ? ringStats.hours : 12);
    }
    //logPrintln("SensorPoll: OK");
}