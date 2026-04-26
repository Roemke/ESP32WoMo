#include <Arduino.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <USB.h>
#include <USBCDC.h>

#include "wifi.h"
#include "indexHtmlJS.h"
#include "config.h"
#include "appconfig.h"
#include "sensorpoll.h"
#include "logging.h"

USBCDC USBSerial;
#define Serial USBSerial

AsyncWebServer server(80);

// ----------------------------------------------------------------
// Processor – Variablenersetzung beim Ausliefern der Website
// ----------------------------------------------------------------
String processor(const String& var)
{
    if (var == "WIFI_MAC_AP")     return wifiMacAp;
    if (var == "WIFI_MAC_STA")    return wifiMacSta;
    if (var == "WIFI_MODE")       return wifiMode;
    if (var == "WIFI_USE_STATIC") return wifiData.use_static_ip ? "checked" : "";
    if (var == "WIFI_STATIC_IP")  return String(wifiData.static_ip);
    if (var == "WIFI_SUBNET")     return String(wifiData.subnet);
    if (var == "WIFI_GATEWAY")    return String(wifiData.gateway);
    if (var == "WIFI_DNS")        return String(wifiData.dns);
    if (var == "SENSOR_ESP_IP")   return String(appConfig.sensor_esp_ip);
    if (var == "SENSOR_POLL_INTERVAL") return String(appConfig.sensor_poll_interval_ms);
    return "";
}

// ----------------------------------------------------------------
// API: /api/data
// ----------------------------------------------------------------
String buildDataJson()
{
    JsonDocument doc;

    JsonObject bme = doc["bme"].to<JsonObject>();
    bme["valid"] = sensorData.bme_valid;
    if (sensorData.bme_valid)
    {
        bme["T"] = sensorData.temperature;
        bme["H"] = sensorData.humidity;
        bme["P"] = sensorData.pressure;
    }

    JsonObject ve = doc["vedirect"].to<JsonObject>();
    ve["valid"] = sensorData.vedirect_valid;
    if (sensorData.vedirect_valid)
    {
        ve["V"]   = sensorData.voltage;
        ve["I"]   = sensorData.current;
        ve["P"]   = sensorData.power;
        ve["SOC"] = sensorData.soc;
        ve["TTG"] = sensorData.ttg;
        ve["VS"]  = sensorData.voltage_starter;
    }

    JsonObject m1 = doc["mppt1"].to<JsonObject>();
    m1["valid"] = sensorData.mppt1_valid;
    if (sensorData.mppt1_valid)
    {
        m1["V"]        = sensorData.mppt1_voltage;
        m1["I"]        = sensorData.mppt1_current;
        m1["PV"]       = sensorData.mppt1_pv_power;
        m1["stateStr"] = sensorData.mppt1_stateStr;
        m1["yield"]    = sensorData.mppt1_yield_today;
    }

    JsonObject m2 = doc["mppt2"].to<JsonObject>();
    m2["valid"] = sensorData.mppt2_valid;
    if (sensorData.mppt2_valid)
    {
        m2["V"]        = sensorData.mppt2_voltage;
        m2["I"]        = sensorData.mppt2_current;
        m2["PV"]       = sensorData.mppt2_pv_power;
        m2["stateStr"] = sensorData.mppt2_stateStr;
        m2["yield"]    = sensorData.mppt2_yield_today;
    }

    JsonObject ch = doc["charger"].to<JsonObject>();
    ch["valid"] = sensorData.charger_valid;
    if (sensorData.charger_valid)
    {
        ch["V"]        = sensorData.charger_voltage;
        ch["I"]        = sensorData.charger_current;
        ch["state"]    = sensorData.charger_state;
        ch["stateStr"] = sensorData.charger_stateStr;
    }

    JsonObject co2 = doc["co2"].to<JsonObject>();
    co2["valid"] = sensorData.co2_valid;
    if (sensorData.co2_valid)
        co2["co2"] = sensorData.co2_ppm;

    JsonObject gas = doc["gas"].to<JsonObject>();
    gas["valid"] = sensorData.gas_valid;
    if (sensorData.gas_valid)
        gas["percent"] = sensorData.gas_percent;

    doc["wifi"] = wifiGetIP();

    String out;
    serializeJson(doc, out);
    return out;
}

// ----------------------------------------------------------------
// API: /api/stats
// ----------------------------------------------------------------
void handleStats(AsyncWebServerRequest *req)
{
    if (req->hasParam("hours"))
    {
        uint32_t hours = req->getParam("hours")->value().toInt();
        if (hours != ringStats.hours)
            calcRingStats(hours);
    }

    if (!ringStats.valid)
    {
        req->send(200, "application/json", "{\"valid\":false}");
        return;
    }

    JsonDocument doc;
    doc["valid"] = true;
    doc["hours"] = ringStats.hours;
    doc["valid_sensors"] = ringStats.valid_sensors;

    auto addStats = [&](const char *key, float mn, float mx, float avg)
    {
        doc[key]["min"] = mn;
        doc[key]["max"] = mx;
        doc[key]["avg"] = avg;
    };

    addStats("T",   ringStats.t_min,   ringStats.t_max,   ringStats.t_avg);
    addStats("H",   ringStats.h_min,   ringStats.h_max,   ringStats.h_avg);
    addStats("P",   ringStats.p_min,   ringStats.p_max,   ringStats.p_avg);
    addStats("V",   ringStats.v_min,   ringStats.v_max,   ringStats.v_avg);
    addStats("I",   ringStats.i_min,   ringStats.i_max,   ringStats.i_avg);
    addStats("SOC", ringStats.soc_min, ringStats.soc_max, ringStats.soc_avg);
    addStats("PW",  ringStats.pw_min,  ringStats.pw_max,  ringStats.pw_avg);
    addStats("VS",  ringStats.vs_min,  ringStats.vs_max,  ringStats.vs_avg);
    addStats("CO2", ringStats.co2_min, ringStats.co2_max, ringStats.co2_avg);
    addStats("MPPT1_V",  ringStats.mppt1_v_min,  ringStats.mppt1_v_max,  ringStats.mppt1_v_avg);
    addStats("MPPT1_I",  ringStats.mppt1_i_min,  ringStats.mppt1_i_max,  ringStats.mppt1_i_avg);
    addStats("MPPT1_PV", ringStats.mppt1_pv_min, ringStats.mppt1_pv_max, ringStats.mppt1_pv_avg);
    addStats("MPPT2_V",  ringStats.mppt2_v_min,  ringStats.mppt2_v_max,  ringStats.mppt2_v_avg);
    addStats("MPPT2_I",  ringStats.mppt2_i_min,  ringStats.mppt2_i_max,  ringStats.mppt2_i_avg);
    addStats("MPPT2_PV", ringStats.mppt2_pv_min, ringStats.mppt2_pv_max, ringStats.mppt2_pv_avg);
    addStats("CHARGER_V", ringStats.charger_v_min, ringStats.charger_v_max, ringStats.charger_v_avg);
    addStats("CHARGER_I", ringStats.charger_i_min, ringStats.charger_i_max, ringStats.charger_i_avg);

    // Gas – int statt float, daher manuell
    doc["GAS"]["min"] = ringStats.gas_min;
    doc["GAS"]["max"] = ringStats.gas_max;
    doc["GAS"]["avg"] = ringStats.gas_avg;

    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
}

// ----------------------------------------------------------------
// POST Handler
// ----------------------------------------------------------------
void handleAppConfigPost(AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t)
{
    JsonDocument doc;
    if (deserializeJson(doc, data, len))
    {
        req->send(400, "application/json", "{\"error\":\"JSON ungueltig\"}");
        return;
    }
    if (doc["sensor_esp_ip"].is<const char*>())
        strlcpy(appConfig.sensor_esp_ip, doc["sensor_esp_ip"], sizeof(appConfig.sensor_esp_ip));
    if (doc["sensor_poll_interval_ms"].is<int>())
        appConfig.sensor_poll_interval_ms = (int)doc["sensor_poll_interval_ms"];
    appConfigSave();
    req->send(200, "application/json", "{\"ok\":true}");
    delay(100);
    ESP.restart();
}

void handleWifiPost(AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t)
{
    JsonDocument doc;
    if (deserializeJson(doc, data, len))
    {
        req->send(400, "application/json", "{\"error\":\"JSON ungueltig\"}");
        return;
    }
    const char *ssid = doc["ssid"] | "";
    if (strlen(ssid) == 0)
    {
        req->send(400, "application/json", "{\"error\":\"SSID fehlt\"}");
        return;
    }
    if (doc["use_static_ip"].is<bool>())
        wifiData.use_static_ip = doc["use_static_ip"].as<bool>();
    else
        wifiData.use_static_ip = false;

    strlcpy(wifiData.static_ip, doc["static_ip"] | WIFI_STATIC_IP_DEFAULT, sizeof(wifiData.static_ip));
    strlcpy(wifiData.subnet,    doc["subnet"]    | WIFI_SUBNET_DEFAULT,    sizeof(wifiData.subnet));
    strlcpy(wifiData.gateway,   doc["gateway"]   | WIFI_GATEWAY_DEFAULT,   sizeof(wifiData.gateway));
    strlcpy(wifiData.dns,       doc["dns"]       | WIFI_DNS_DEFAULT,       sizeof(wifiData.dns));
    wifiSetCredentials(ssid, doc["password"] | "");
    req->send(200, "application/json", "{\"ok\":true}");
    delay(500);
    ESP.restart();
}

void handleReboot(AsyncWebServerRequest *req)
{
    req->send(200, "application/json", "{\"ok\":true}");
    delay(500);
    ESP.restart();
}

static void handleLog(AsyncWebServerRequest *req)
{
    uint32_t from = 0;
    if (req->hasParam("from"))
        from = req->getParam("from")->value().toInt();

    JsonDocument doc;
    JsonArray arr = doc["log"].to<JsonArray>();
    for (uint32_t i = from; i < logCount; i++) {
        uint8_t idx = (logIndex + LOG_BUFFER_SIZE - logCount + i) % LOG_BUFFER_SIZE;
        arr.add(logBuffer[idx]);
    }
    doc["count"] = logCount;
    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
}

// ----------------------------------------------------------------
// Routen
// ----------------------------------------------------------------
void addRoutes()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
        { req->send(200, "text/html", index_html, processor); });

    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *req)
        { req->send(200, "application/json", buildDataJson()); });

    server.on("/api/stats", HTTP_GET, [](AsyncWebServerRequest *req)
        { handleStats(req); });

    server.on("/api/capacity", HTTP_GET, [](AsyncWebServerRequest *req) {
        uint32_t maxHours = (uint32_t)((uint64_t)RING_MAX_ENTRIES *
                             appConfig.sensor_poll_interval_ms / 1000 / 3600);
        String out = "{\"maxHours\":" + String(maxHours) + "}";
        req->send(200, "application/json", out);
    });

    server.on("/api/log", HTTP_GET, [](AsyncWebServerRequest *req)
        { handleLog(req); });

    server.on("/api/config/wifi", HTTP_POST,
        [](AsyncWebServerRequest *req) {}, nullptr, handleWifiPost);

    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *req)
        { req->send(200, "application/json", appConfigToJson()); });

    server.on("/api/config", HTTP_POST,
        [](AsyncWebServerRequest *req) {}, nullptr, handleAppConfigPost);

    server.on("/api/reboot", HTTP_POST, handleReboot);

    server.onNotFound([](AsyncWebServerRequest *req)
        { req->send(404, "application/json", "{\"error\":\"nicht gefunden\"}"); });
}

// ----------------------------------------------------------------
// Setup & Loop
// ----------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    delay(5000);
    delay(3000);
    Serial.println("=== WoMo S2 Mini Start ===");

    if (!LittleFS.begin(true))
        Serial.println("LittleFS FEHLER");
    else
        Serial.println("LittleFS OK");

    appConfigLoad();
    Serial.println("AppConfig OK");

    wifiSetup();
    Serial.println("WiFi OK");

    ElegantOTA.begin(&server);
    addRoutes();
    server.begin();
    Serial.println("Server OK");

    sensorPollSetup();
    Serial.printf("Heap nach Pollsetup: %lu\n", ESP.getFreeHeap());
    Serial.printf("PSRAM: %lu bytes\n", (unsigned long)ESP.getPsramSize());
    Serial.println("Setup fertig");
}

void loop()
{
    static uint32_t lastHeapLog = 0;
    if (millis() - lastHeapLog > 30000)
    {
        lastHeapLog = millis();
        logPrintf("Heap: %lu frei | PSRAM: %lu frei\n",
            (unsigned long)ESP.getFreeHeap(),
            (unsigned long)ESP.getFreePsram());
    }

    sensorPollLoop();
}