#include <Arduino.h>
#include <LittleFS.h>
#include <esp32_smartdisplay.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <esp_task_wdt.h>

#include "wifi.h"
#include "wled.h"
//#include "vedirect.h" //raus macht ein anderer esp
//#include "bme280sensor.h"
#include "sdcard.h"
#include "ui_main.h"
#include "ui_sensoren.h"
#include "indexHtmlJS.h"
#include "config.h"  // für Defaults
#include "appconfig.h" //für konfigurierbare geschichten 
#include "sensorpoll.h"
#include "ui_history.h"
#include <SD.h> //für dateioperationen per webserver, hat logisch nichts mit sdcard.h zu tun.


AsyncWebServer server(80);
static auto lv_last_tick = millis();



//fuer die indexHTMLJS
String processor(const String& var)
{
    if (var == "WIFI_MAC_AP")     return wifiMacAp;
    if (var == "WIFI_MAC_STA")    return wifiMacSta;
    if (var == "WIFI_MODE")       return wifiMode;
    if (var == "WIFI_USE_STATIC") return wifiData.use_static_ip ? "checked" : "";
    if (var == "WIFI_STATIC_IP")  return String(wifiData.static_ip);
    if (var == "WIFI_SUBNET")     return String(wifiData.subnet);
    if (var == "SENSOR_ESP_IP")   return String(appConfig.sensor_esp_ip);
    if (var == "WLED_INNEN_IP")   return String(appConfig.wled_innen_ip);
    if (var == "WLED_AUSSEN_IP")  return String(appConfig.wled_aussen_ip);
    return "";
}

//routen der api handlen


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

    JsonObject co2 = doc["co2"].to<JsonObject>();
    co2["valid"] = sensorData.co2_valid;
    if (sensorData.co2_valid)
        co2["ppm"] = sensorData.co2_ppm;

    doc["wifi"] = wifiGetIP();

    String out;
    serializeJson(doc, out);
    return out;
}

void handleAppConfigPost(AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t)
{
    JsonDocument doc;
    if (deserializeJson(doc, data, len))
    {
        req->send(400, "application/json", "{\"error\":\"JSON ungueltig\"}");
        return;
    }
    if (doc["sensor_esp_ip"].is<const char*>())
        strlcpy(appConfig.sensor_esp_ip,  doc["sensor_esp_ip"],  sizeof(appConfig.sensor_esp_ip));
    if (doc["wled_innen_ip"].is<const char*>())
        strlcpy(appConfig.wled_innen_ip,  doc["wled_innen_ip"],  sizeof(appConfig.wled_innen_ip));
    if (doc["wled_aussen_ip"].is<const char*>())
        strlcpy(appConfig.wled_aussen_ip, doc["wled_aussen_ip"], sizeof(appConfig.wled_aussen_ip));
    appConfigSave();
    req->send(200, "application/json", "{\"ok\":true}");
    // kein Neustart nötig – IPs werden beim nächsten Poll aktiv
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
    wifiData.use_static_ip = doc["use_static_ip"] | false;
    strlcpy(wifiData.static_ip, doc["static_ip"] | WIFI_STATIC_IP_DEFAULT, sizeof(wifiData.static_ip));
    strlcpy(wifiData.subnet,    doc["subnet"]    | WIFI_SUBNET_DEFAULT,    sizeof(wifiData.subnet));
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

//anzeigen verzeichnisinhalt
static void handleSDList(AsyncWebServerRequest *req)
{
    String dir = "/";
    if (req->hasParam("dir"))
        dir = req->getParam("dir")->value();

    File root = SD.open(dir);
    if (!root || !root.isDirectory())
    {
        req->send(404, "application/json", "{\"error\":\"Verzeichnis nicht gefunden\"}");
        return;
    }

    JsonDocument doc;
    JsonArray files = doc["files"].to<JsonArray>();
    File f = root.openNextFile();
    while (f)
    {
        JsonObject entry = files.add<JsonObject>();
        entry["name"] = String(f.name());
        entry["size"] = f.size();
        entry["dir"] = f.isDirectory();
        f = root.openNextFile();
    }
    doc["dir"] = dir;
    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);    
}


//download eines Files
static void handleSDDownload(AsyncWebServerRequest *req)
{
    if (!req->hasParam("file"))
    {
        req->send(400, "application/json", "{\"error\":\"file fehlt\"}");
        return;
    }
    String path = req->getParam("file")->value();
    if (!SD.exists(path))
    {
        req->send(404, "application/json", "{\"error\":\"nicht gefunden\"}");
        return;
    }
    req->send(SD, path, "text/csv", true); // true = als Download
};

// SD Upload
static void handleSDUpload(AsyncWebServerRequest* req, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    String path = "/";
    if (req->hasParam("dir"))
        path = req->getParam("dir")->value();
    if (!path.endsWith("/")) path += "/";
    path += filename;

    static File uploadFile;
    if (index == 0)
    {
        logPrintf("SD Upload: %s\n", path.c_str());
        // Verzeichnis anlegen falls nötig
        String dir = path.substring(0, path.lastIndexOf('/'));
        if (dir.length() > 0 && !SD.exists(dir))
            SD.mkdir(dir);
        uploadFile = SD.open(path, FILE_WRITE);
    }
    if (uploadFile)
        uploadFile.write(data, len);
    if (final && uploadFile)
    {
        uploadFile.close();
        logPrintf("SD Upload: %s fertig (%u bytes)\n", path.c_str(), index + len);
    }
}



//normal setup

void setup() {
    Serial.begin(115200);
    
    if (!LittleFS.begin(true))
        Serial.println("LittleFS FEHLER");
    else
        Serial.println("LittleFS OK");
    
    
    appConfigLoad();
    Serial.println("AppConfig OK");
        
    wifiSetup();
    Serial.println("WiFi OK");
    // Auf NTP warten – max 5 Sekunden
    uint32_t ntpStart = millis();
    while (!wifiTimeValid() && millis() - ntpStart < 5000)
        delay(100);
    Serial.printf("NTP: %s\n", wifiTimeValid() ? "sync" : "kein sync");

    //wledSetup();
    Serial.println("WLED noch zu tun ");   

    ElegantOTA.begin(&server);
    //routen für die api

    //history darstellen 
    
    server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *req)
    {
        String from   = req->hasParam("from")   ? req->getParam("from")->value()   : "";
        String to     = req->hasParam("to")     ? req->getParam("to")->value()     : "";
        int    points = req->hasParam("points") ? req->getParam("points")->value().toInt() : 400;

        if (from.isEmpty() || to.isEmpty())
        {
            req->send(400, "application/json", "{\"error\":\"from/to fehlt\"}");
            return;
        }

        const char *buf = sdGetHistoryBuffer(from, to, points);
        size_t total    = strlen(buf);

        AsyncWebServerResponse *response = req->beginChunkedResponse("application/json",
            [buf, total](uint8_t *chunk, size_t maxLen, size_t index) -> size_t
            {
                if (index >= total) return 0; // fertig
                size_t len = min(maxLen, total - index);
                memcpy(chunk, buf + index, len);
                return len;
            });
        req->send(response);
    });
    // SD Download
    server.on("/api/sd/download", HTTP_GET, [](AsyncWebServerRequest *req)
        { handleSDDownload(req); });

        // SD Verzeichnis auflisten
    server.on("/api/sd/list", HTTP_GET, [](AsyncWebServerRequest *req)
        {   handleSDList(req); });

    server.on("/api/sd/upload", HTTP_POST,     
    [](AsyncWebServerRequest *req)
    {
        req->send(200, "application/json", "{\"ok\":true}");
    },
    [](AsyncWebServerRequest *req, String filename, size_t index, uint8_t *data, size_t len, bool final)
    {
        handleSDUpload(req, filename, index, data, len, final);
    });

    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *req)
        { req->send(200, "application/json", buildDataJson()); });


    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
        { req->send(200, "text/html", index_html, processor); });


    // spezifischere Route zuerst
    server.on("/api/config/wifi", HTTP_POST,
        [](AsyncWebServerRequest *req) {}, nullptr, handleWifiPost);

    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *req)
        { req->send(200, "application/json", appConfigToJson()); });

    server.on("/api/config", HTTP_POST,
        [](AsyncWebServerRequest *req) {}, nullptr, handleAppConfigPost);

    server.on("/api/reboot", HTTP_POST, handleReboot);

    server.onNotFound([](AsyncWebServerRequest *req)
        { req->send(404, "application/json", "{\"error\":\"nicht gefunden\"}"); });


    esp_task_wdt_init(30, true); //etwas mehr zeit 

    server.begin();
    Serial.println("Server OK");

    sdSetup();
    Serial.println("SD OK");


    
    smartdisplay_init();

    
    smartdisplay_lcd_set_backlight(1.0f);

    auto display = lv_display_get_default();
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);
    
    uiMainSetup();
    
    logPrintf("PSRAM: %lu bytes\n", (unsigned long)ESP.getPsramSize());
    logPrintln("Setup ist durch");    
}

void loop() {
    auto const now = millis();
   
    static uint32_t lastHeapLog = 0;
    if (millis() - lastHeapLog > 5000) // alle 30 Sekunden
    {
        lastHeapLog = millis();
        logPrintf("Heap: %lu frei, min: %lu | PSRAM: %lu frei\n",
            (unsigned long)ESP.getFreeHeap(),
            (unsigned long)ESP.getMinFreeHeap(),
            (unsigned long)ESP.getFreePsram());
    }
    
    lv_tick_inc(now - lv_last_tick);
    lv_last_tick = now;
    lv_timer_handler();
    
   
    sdLoop();
    sensorPollLoop();
    uiHistoryLoop();
    //wledLoop();

    // UI alle 2 Sekunden aktualisieren            
    uiSensorenUpdate();        
    
}
