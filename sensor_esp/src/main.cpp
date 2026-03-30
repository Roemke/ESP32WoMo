#include <Arduino.h>
#include <LittleFS.h>
#include "esp_log.h"
#include "logging.h"
#include "wifi.h"
#include "sensorconfig.h"
#include "bme280sensor.h"
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include "indexHtmlJS.h"
#include "logging.h"
#include "victronble.h"
#include "bleconfig.h"
#include "scd41sensor.h"

AsyncWebServer server(HTTP_PORT);


//processor - Variablenersetzung beim Ausliefern der Website
String processor(const String& var)
{
    if (var == "WIFI_MAC_AP")   return wifiMacAp;
    if (var == "WIFI_MAC_STA")  return wifiMacSta;
    if (var == "WIFI_MODE")     return wifiMode;
    if (var == "BME_SDA")       return String(sensorConfig.bme_sda);
    if (var == "BME_SCL")       return String(sensorConfig.bme_scl);
    if (var == "BME_ADDR")      return String(sensorConfig.bme_addr);
    if (var == "BME_INTERVAL")  return String(sensorConfig.bme_interval_ms);    
    if (var == "BMV_MAC")        return String(bleConfig.bmv_mac);
    if (var == "BMV_BINDKEY")    return String(bleConfig.bmv_bindkey);
    if (var == "MPPT1_MAC")      return String(bleConfig.mppt1_mac);
    if (var == "MPPT1_BINDKEY")  return String(bleConfig.mppt1_bindkey);
    if (var == "MPPT2_MAC")      return String(bleConfig.mppt2_mac);
    if (var == "MPPT2_BINDKEY")  return String(bleConfig.mppt2_bindkey);
    return "";
}


//funktionen für die api-calls
String buildDataJson()
{
    JsonDocument doc;
    doc["bme"]     = serialized(bme280ToJson());    
    doc["co2"]   = serialized(scd41ToJson());     // nur CO2 für Display
    doc["scd41"]    = serialized(scd41ToJsonFull()); //alle für webinterface
    doc["vedirect"] = serialized(bmvToJson());      // BMV712 via BLE    
    doc["mppt1"]   = serialized(mppt1ToJson());     // MPPT1 via BLE
    doc["mppt2"]   = serialized(mppt2ToJson());     // MPPT2 via BLE
    doc["wifi"]    = wifiGetIP();


    String out;
    serializeJson(doc, out);
    return out;
}
String buildLogJson(uint32_t from)
{
    JsonDocument doc;
    uint32_t maxLogs = 10;
    uint32_t start = (logCount > from + maxLogs) ? logCount - maxLogs : from;
    JsonArray logs = doc["log"].to<JsonArray>();
    for (uint32_t i = start; i < logCount; i++)
    {
        uint32_t idx = (logIndex + LOG_BUFFER_SIZE - logCount + i) % LOG_BUFFER_SIZE;
        logs.add(logBuffer[idx]);
    }
    doc["logCount"] = logCount;
    String out;
    serializeJson(doc, out);
    return out;
}

void handleConfigPost(AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t)
{
    JsonDocument doc;
    if (deserializeJson(doc, data, len))
    {
        req->send(400, "application/json", "{\"error\":\"JSON ungueltig\"}");
        return;
    }
    if (doc["bme_sda"].is<int>())         sensorConfig.bme_sda         = doc["bme_sda"];
    if (doc["bme_scl"].is<int>())         sensorConfig.bme_scl         = doc["bme_scl"];
    if (doc["bme_addr"].is<int>())        sensorConfig.bme_addr        = doc["bme_addr"];
    if (doc["bme_interval_ms"].is<int>()) sensorConfig.bme_interval_ms = doc["bme_interval_ms"];
    sensorConfigSave();
    req->send(200, "application/json", "{\"ok\":true}");
    delay(500);
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
    // erst static IP setzen, dann wifiSetCredentials – das speichert alles in einem Rutsch
    wifiData.use_static_ip = doc["use_static_ip"] | false;
    strlcpy(wifiData.static_ip, doc["static_ip"] | WIFI_STATIC_IP_DEFAULT, sizeof(wifiData.static_ip));
    strlcpy(wifiData.subnet,    doc["subnet"]    | WIFI_SUBNET_DEFAULT,    sizeof(wifiData.subnet));
    wifiSetCredentials(ssid, doc["password"] | ""); //speichert mit, daher am Ende
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

void addRoutes()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(200, "text/html", index_html, processor);
    });

    //api
    
    // BLE Konfiguration lesen
    server.on("/api/config/ble", HTTP_GET, [](AsyncWebServerRequest *req)
        { req->send(200, "application/json", bleConfigToJson()); });

    // BLE Konfiguration schreiben
    server.on("/api/config/ble", HTTP_POST,
        [](AsyncWebServerRequest *req) {}, nullptr,
        [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t)
        {
            JsonDocument doc;
            if (deserializeJson(doc, data, len))
            {
                req->send(400, "application/json", "{\"error\":\"JSON ungueltig\"}");
                return;
            }
            if (doc["bmv_mac"].is<const char*>())
                strlcpy(bleConfig.bmv_mac,       doc["bmv_mac"],       sizeof(bleConfig.bmv_mac));
            if (doc["bmv_bindkey"].is<const char*>())
                strlcpy(bleConfig.bmv_bindkey,   doc["bmv_bindkey"],   sizeof(bleConfig.bmv_bindkey));
            if (doc["mppt1_mac"].is<const char*>())
                strlcpy(bleConfig.mppt1_mac,     doc["mppt1_mac"],     sizeof(bleConfig.mppt1_mac));
            if (doc["mppt1_bindkey"].is<const char*>())
                strlcpy(bleConfig.mppt1_bindkey, doc["mppt1_bindkey"], sizeof(bleConfig.mppt1_bindkey));
            if (doc["mppt2_mac"].is<const char*>())
                strlcpy(bleConfig.mppt2_mac,     doc["mppt2_mac"],     sizeof(bleConfig.mppt2_mac));
            if (doc["mppt2_bindkey"].is<const char*>())
                strlcpy(bleConfig.mppt2_bindkey, doc["mppt2_bindkey"], sizeof(bleConfig.mppt2_bindkey));
            bleConfigSave();
            req->send(200, "application/json", "{\"ok\":true}");
            delay(500);
            ESP.restart();
        }
    );    
    server.on("/api/log", HTTP_GET, [](AsyncWebServerRequest *req)
    {
        uint32_t from = 0;
        if (req->hasParam("logFrom"))
            from = req->getParam("logFrom")->value().toInt();
        req->send(200, "application/json", buildLogJson(from));
    });
    // Alle Sensordaten
    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *req)
    {
        req->send(200, "application/json", buildDataJson());
    });

    server.on("/api/config", HTTP_GET,  [](AsyncWebServerRequest *req)
        { req->send(200, "application/json", sensorConfigToJson()); });

    /*
    Signatur server.on(uri, method, onRequest, onUpload, onBody);
    uri – Pfad: "/api/config/..."
    method – HTTP_POST
    onRequest – wird aufgerufen wenn der Request ankommt aber ohne Body – hier leer [](){} weil wir nichts damit machen
    onUpload – für Datei-Uploads, hier nullptr weil nicht benötigt
    onBody – wird aufgerufen wenn der Body-Chunk ankommt – hier handle...
    */        
    server.on("/api/config/wifi", HTTP_POST,
        [](AsyncWebServerRequest *req) {}, nullptr, handleWifiPost);

    server.on("/api/config", HTTP_POST,
        [](AsyncWebServerRequest *req) {}, nullptr, handleConfigPost);


    server.on("/api/reboot", HTTP_POST, handleReboot);

    server.onNotFound([](AsyncWebServerRequest *req)
        { req->send(404, "application/json", "{\"error\":\"nicht gefunden\"}"); });

}
void setup() {
    Serial.begin(115200);
    delay(5000);
    ESP_LOGI("MAIN","start setup");
    if (!LittleFS.begin(true))
        ESP_LOGI("MAIN","LittleFS FEHLER");
    else
        ESP_LOGI("MAIN","LittleFS OK");

    sensorConfigLoad();
    ESP_LOGI("MAIN","Config OK");

    bleConfigLoad();
    ESP_LOGI("MAIN","BleConfig OK");
    

    wifiSetup();
    ESP_LOGI("MAIN","WiFi OK");

    
    if (!bme280Setup())
        ESP_LOGI("MAIN","BME280 FEHLER");
    else
        ESP_LOGI("MAIN","BME280 OK");

    if (!scd41Setup())
        ESP_LOGI("MAIN","SCD41 FEHLER");
    else
        ESP_LOGI("MAIN","SCD41 OK");

    ESP_LOGI("main","Heap vor BLE: %lu\n", ESP.getFreeHeap());
    victronBleSetup();
    ESP_LOGI("main","Heap nach BLE: %lu\n", ESP.getFreeHeap());
    ESP_LOGI("MAIN","VictronBLE OK");    
    
    // server routen setzen 
    addRoutes();

    // OTA
    ElegantOTA.begin(&server);
    ElegantOTA.onStart([]() { logPrintln("OTA: Start"); });
    ElegantOTA.onEnd([](bool ok) {
        logPrintf("OTA: %s\n", ok ? "Erfolgreich" : "Fehler");
        if (ok) ESP.restart();
    });

    server.begin();
    ESP_LOGI("MAIN","Server OK");    
}

void loop() {
    bme280Loop();
    scd41Loop();

    victronBleLoop();
    //Serial.println(bme280ToJson());
    //delay(1000);
}