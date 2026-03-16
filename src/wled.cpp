#include "wled.h"
#include <HTTPClient.h>
#include <WiFi.h>

// ----------------------------------------------------------------
// Globale Konfiguration
// ----------------------------------------------------------------
WledConfig wledConfig = {
    { WLED_DEFAULT_INNEN_IP,  WLED_DEFAULT_PORT, false },
    { WLED_DEFAULT_AUSSEN_IP, WLED_DEFAULT_PORT, false }
};

// Zustandsvariablen – volatile weil vom Poll-Task (Core 0)
// geschrieben und vom Display-Code (Core 1) gelesen.
// Ein einzelnes bool ist auf ESP32 Xtensa atomar – kein Mutex nötig.
volatile bool wledInnenState  = false;
volatile bool wledAussenState = false;

// ----------------------------------------------------------------
// Konfiguration laden
// ----------------------------------------------------------------
bool wledLoadConfig()
{
    File f = LittleFS.open(WLED_CONFIG_PATH, "r");
    if (!f)
    {
        logPrintln("WLED: Keine Konfiguration gefunden, nutze Defaults");
        return false;
    }
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err)
    {
        logPrintln("WLED: Fehler beim Lesen der Konfiguration");
        return false;
    }
    strlcpy(wledConfig.innen.ip,
            doc["innen"]["ip"] | WLED_DEFAULT_INNEN_IP,
            sizeof(wledConfig.innen.ip));
    wledConfig.innen.port = doc["innen"]["port"] | WLED_DEFAULT_PORT;

    strlcpy(wledConfig.aussen.ip,
            doc["aussen"]["ip"] | WLED_DEFAULT_AUSSEN_IP,
            sizeof(wledConfig.aussen.ip));
    wledConfig.aussen.port = doc["aussen"]["port"] | WLED_DEFAULT_PORT;

    logPrintf("WLED: Konfiguration geladen – innen=%s:%d  aussen=%s:%d\n",
              wledConfig.innen.ip,  wledConfig.innen.port,
              wledConfig.aussen.ip, wledConfig.aussen.port);
    return true;
}

// ----------------------------------------------------------------
// Konfiguration speichern
// ----------------------------------------------------------------
bool wledSaveConfig()
{
    File f = LittleFS.open(WLED_CONFIG_PATH, "w");
    if (!f)
    {
        logPrintln("WLED: Fehler beim Speichern der Konfiguration");
        return false;
    }
    JsonDocument doc;
    doc["innen"]["ip"]    = wledConfig.innen.ip;
    doc["innen"]["port"]  = wledConfig.innen.port;
    doc["aussen"]["ip"]   = wledConfig.aussen.ip;
    doc["aussen"]["port"] = wledConfig.aussen.port;
    serializeJson(doc, f);
    f.close();
    logPrintln("WLED: Konfiguration gespeichert");
    return true;
}

// ----------------------------------------------------------------
// An/Aus senden
// ----------------------------------------------------------------
bool wledSetState(WledInstance &inst, bool on)
{
    if (!WiFi.isConnected())
    {
        logPrintln("WLED: Kein WLAN – Befehl nicht gesendet");
        return false;
    }
    char url[64];
    snprintf(url, sizeof(url), "http://%s:%d/json/state",
             inst.ip, inst.port);

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(2000);

    int code = http.POST(on ? "{\"on\":true}" : "{\"on\":false}");
    http.end();

    if (code == 200)
    {
        inst.lastState = on;
        logPrintf("WLED: %s -> %s\n", url, on ? "AN" : "AUS");
        return true;
    }
    logPrintf("WLED: Fehler %s (HTTP %d)\n", url, code);
    return false;
}

// ----------------------------------------------------------------
// Tatsaechlichen Zustand von WLED abfragen (nur intern im Task)
// ----------------------------------------------------------------
static bool fetchState(WledInstance &inst, volatile bool &stateVar)
{
    char url[64];
    snprintf(url, sizeof(url), "http://%s:%d/json/state",
             inst.ip, inst.port);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(2000);

    int code = http.GET();
    if (code == 200)
    {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, http.getString());
        http.end();
        if (!err)
        {
            stateVar       = doc["on"] | false;
            inst.lastState = stateVar;
            return true;
        }
    }
    http.end();
    // Kein Log – WLED ist oft stromlos, das ist normal
    return false;
}

// ----------------------------------------------------------------
// FreeRTOS-Task auf Core 0
// Blockiert bis zu 2s pro Instanz wenn WLED nicht erreichbar –
// unkritisch weil separater Task, Loop und LVGL laufen weiter
// ----------------------------------------------------------------
static void wledPollTask(void *pvParam)
{
    (void)pvParam;
    vTaskDelay(pdMS_TO_TICKS(5000)); // kurz warten bis WiFi steht

    for (;;)
    {
        if (WiFi.isConnected())
        {
            fetchState(wledConfig.innen,  wledInnenState);
            fetchState(wledConfig.aussen, wledAussenState);
        }
        vTaskDelay(pdMS_TO_TICKS(WLED_POLL_INTERVAL_MS));
    }
}

// ----------------------------------------------------------------
// Setup
// ----------------------------------------------------------------
void wledSetup()
{
    wledLoadConfig();

    // Poll-Task auf Core 0 starten (LVGL laeuft auf Core 1)
    xTaskCreatePinnedToCore(
        wledPollTask,   // Task-Funktion
        "wledPoll",     // Name (nur fuer Debugging)
        4096,           // Stack in Bytes
        nullptr,        // Parameter
        1,              // Prioritaet (niedrig)
        nullptr,        // Task-Handle (nicht benoetigt)
        0               // Core 0
    );
    logPrintln("WLED: Poll-Task gestartet auf Core 0");
}

// ----------------------------------------------------------------
// Loop – leer, Polling laeuft im Task
// ----------------------------------------------------------------
void wledLoop()
{
    // nichts – wledPollTask uebernimmt das Polling
}

// ----------------------------------------------------------------
// Status als JSON fuer REST-API
// ----------------------------------------------------------------
String wledToJson()
{
    JsonDocument doc;

    JsonObject innen = doc["innen"].to<JsonObject>();
    innen["ip"]    = wledConfig.innen.ip;
    innen["port"]  = wledConfig.innen.port;
    innen["state"] = (bool)wledInnenState;

    JsonObject aussen = doc["aussen"].to<JsonObject>();
    aussen["ip"]    = wledConfig.aussen.ip;
    aussen["port"]  = wledConfig.aussen.port;
    aussen["state"] = (bool)wledAussenState;

    String out;
    serializeJson(doc, out);
    return out;
}
