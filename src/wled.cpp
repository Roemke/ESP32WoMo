#include "wled.h"
#include "appconfig.h"
#include <HTTPClient.h>
#include <WiFi.h>

// ----------------------------------------------------------------
// Globale Konfiguration
// ----------------------------------------------------------------
WledConfig wledConfig = {};

// ----------------------------------------------------------------
// Interner HTTP POST Helper
// ----------------------------------------------------------------
static bool wledPost(WledInstance &inst, const char *json)
{
    if (!WiFi.isConnected() || strlen(inst.ip) == 0) return false;
    char url[64];
    snprintf(url, sizeof(url), "http://%s:%d/json/state", inst.ip, inst.port);
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(2000);
    int code = http.POST((uint8_t*)json, strlen(json));
    http.end();
    return code == 200;
}

// ----------------------------------------------------------------
// State von WLED lesen
// ----------------------------------------------------------------
static void fetchState(WledInstance &inst)
{
    if (!WiFi.isConnected() || strlen(inst.ip) == 0) return;
    char url[64];
    snprintf(url, sizeof(url), "http://%s:%d/json/state", inst.ip, inst.port);
    HTTPClient http;
    http.begin(url);
    http.setTimeout(2000);
    int code = http.GET();
    if (code == 200) {
        inst.online = true;
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, http.getString());
        http.end();
        if (!err) {
            inst.lastState = doc["on"] | false;
            inst.bri       = doc["bri"] | 128;
            if (doc["seg"][0]["col"][0].is<JsonArray>()) {
                inst.r = doc["seg"][0]["col"][0][0] | 255;
                inst.g = doc["seg"][0]["col"][0][1] | 255;
                inst.b = doc["seg"][0]["col"][0][2] | 255;
            }
        }
    } else {
        inst.online = false;
        http.end();
    }
}

// ----------------------------------------------------------------
// Öffentliche API
// ----------------------------------------------------------------
bool wledSetState(WledInstance &inst, bool on)
{
    char json[32];
    snprintf(json, sizeof(json), "{\"on\":%s}", on ? "true" : "false");
    bool ok = wledPost(inst, json);
    if (ok) inst.lastState = on;
    return ok;
}

bool wledSendColor(WledInstance &inst, uint8_t r, uint8_t g, uint8_t b)
{
    char json[64];
    snprintf(json, sizeof(json), "{\"seg\":[{\"col\":[[%d,%d,%d]]}]}", r, g, b);
    bool ok = wledPost(inst, json);
    if (ok) { inst.r = r; inst.g = g; inst.b = b; }
    return ok;
}

bool wledSendBri(WledInstance &inst, uint8_t bri)
{
    char json[32];
    snprintf(json, sizeof(json), "{\"bri\":%d}", bri);
    bool ok = wledPost(inst, json);
    if (ok) inst.bri = bri;
    return ok;
}

// ----------------------------------------------------------------
// FreeRTOS-Task auf Core 0
// ----------------------------------------------------------------
static void wledPollTask(void *pvParam)
{
    vTaskDelay(pdMS_TO_TICKS(5000));
    for (;;) {
        fetchState(wledConfig.innen);
        fetchState(wledConfig.aussen);
        vTaskDelay(pdMS_TO_TICKS(WLED_POLL_INTERVAL_MS));
    }
}

// ----------------------------------------------------------------
// Setup
// ----------------------------------------------------------------
void wledSetup()
{
    // IPs einmalig aus appConfig übernehmen
    strlcpy(wledConfig.innen.ip,  appConfig.wled_innen_ip,  sizeof(wledConfig.innen.ip));
    strlcpy(wledConfig.aussen.ip, appConfig.wled_aussen_ip, sizeof(wledConfig.aussen.ip));
    wledConfig.innen.port  = WLED_DEFAULT_PORT;
    wledConfig.aussen.port = WLED_DEFAULT_PORT;

    xTaskCreatePinnedToCore(
        wledPollTask, "wledPoll", 4096, nullptr, 1, nullptr, 0
    );
    logPrintf("WLED: Poll-Task gestartet auf Core 0, mit ips %s / %s\n", wledConfig.innen.ip, wledConfig.aussen.ip);
}

// ----------------------------------------------------------------
// Loop – leer, Polling läuft im Task
// ----------------------------------------------------------------
void wledLoop()
{
}