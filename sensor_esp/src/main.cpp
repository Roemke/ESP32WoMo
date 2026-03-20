#include <Arduino.h>
#include <LittleFS.h>
#include "logging.h"
#include "wifi.h"
#include "sensorconfig.h"
#include "bme280sensor.h"
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include "indexHtmlJS.h"
#include "logging.h"
#include "vedirect.h"

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
    if (var == "VE_RX")         return String(sensorConfig.vedirect_rx);
    if (var == "VE_BAUD")       return String(sensorConfig.vedirect_baud);
    return "";
}


//funktionen für die api-calls
String buildDataJson(uint8_t from)
{
    JsonDocument doc;
    doc["bme"]      = serialized(bme280ToJson());
    doc["vedirect"] = serialized(vedirectToJson());
    doc["wifi"]     = wifiGetIP();

    // Log-Zeilen ab 'from' senden
    JsonArray logs = doc["log"].to<JsonArray>();
    for (uint8_t i = from; i < logCount; i++)
    {
        uint8_t idx = (logIndex + LOG_BUFFER_SIZE - logCount + i) % LOG_BUFFER_SIZE;
        logs.add(logBuffer[idx]);
    }
    doc["logCount"] = logCount; // Client weiß wo er beim nächsten Poll anfängt

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
    if (doc["vedirect_rx"].is<int>())     sensorConfig.vedirect_rx     = doc["vedirect_rx"];
    if (doc["vedirect_baud"].is<int>())   sensorConfig.vedirect_baud   = doc["vedirect_baud"];
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

void setup() {
    Serial.begin(115200);
    delay(5000);
    Serial.println("=== Sensor ESP startet ===");
    
    if (!LittleFS.begin(true))
        Serial.println("LittleFS FEHLER");
    else
        Serial.println("LittleFS OK");

    sensorConfigLoad();
    Serial.println("Config OK");

    wifiSetup();
    Serial.println("WiFi OK");

    
    if (!bme280Setup())
        Serial.println("BME280 FEHLER");
    else
        Serial.println("BME280 OK");
    
    // Hauptseite

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(200, "text/html", index_html, processor);
    });

    //api
    // Alle Sensordaten
    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *req)
    {
        uint8_t from = 0;
        if (req->hasParam("logFrom"))
            from = (uint8_t)req->getParam("logFrom")->value().toInt();
        req->send(200, "application/json", buildDataJson(from));
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

    // OTA
    ElegantOTA.begin(&server);
    ElegantOTA.onStart([]() { logPrintln("OTA: Start"); });
    ElegantOTA.onEnd([](bool ok) {
        logPrintf("OTA: %s\n", ok ? "Erfolgreich" : "Fehler");
        if (ok) ESP.restart();
    });

    server.begin();
    Serial.println("Server OK");    
}

void loop() {
    bme280Loop();
    //Serial.println(bme280ToJson());
    //delay(1000);
}