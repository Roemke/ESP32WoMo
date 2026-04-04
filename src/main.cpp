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
//#include "ui_history.h" ist raus 
#include <SD.h> //für dateioperationen per webserver, hat logisch nichts mit sdcard.h zu tun.
#include "ui_details.h"
#include "ui_charger.h"
#include "ui_wled.h"


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
    if (var == "WIFI_GATEWAY") return String(wifiData.gateway);
    if (var == "WIFI_DNS")     return String(wifiData.dns);
    if (var == "SENSOR_ESP_IP")   return String(appConfig.sensor_esp_ip);
    if (var == "WLED_INNEN_IP")   return String(appConfig.wled_innen_ip);
    if (var == "WLED_AUSSEN_IP")  return String(appConfig.wled_aussen_ip);
    if (var == "SENSOR_POLL_INTERVAL") return String(appConfig.sensor_poll_interval_ms);
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

    // VE.Direct / BMV712
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

    // MPPT1
    JsonObject m1 = doc["mppt1"].to<JsonObject>();
    m1["valid"] = sensorData.mppt1_valid;
    if (sensorData.mppt1_valid)
    {
        m1["V"]     = sensorData.mppt1_voltage;
        m1["I"]     = sensorData.mppt1_current;
        m1["PV"]    = sensorData.mppt1_pv_power;
        m1["stateStr"] = sensorData.mppt1_stateStr;
        m1["yield"] = sensorData.mppt1_yield_today;
    }

    // MPPT2
    JsonObject m2 = doc["mppt2"].to<JsonObject>();
    m2["valid"] = sensorData.mppt2_valid;
    if (sensorData.mppt2_valid)
    {
        m2["V"]     = sensorData.mppt2_voltage;
        m2["I"]     = sensorData.mppt2_current;
        m2["PV"]    = sensorData.mppt2_pv_power;
        m2["stateStr"] = sensorData.mppt2_stateStr;
        m2["yield"] = sensorData.mppt2_yield_today;
    }

    // Charger
    JsonObject ch = doc["charger"].to<JsonObject>();
    ch["valid"] = sensorData.charger_valid;
    if (sensorData.charger_valid)
    {
        ch["V"]        = sensorData.charger_voltage;
        ch["I"]        = sensorData.charger_current;
        ch["state"]    = sensorData.charger_state;
        ch["stateStr"] = sensorData.charger_stateStr;
    }

    // CO2
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
    
    wifiData.use_static_ip = doc["use_static_ip"] | false;
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

//stats abfrage behandeln 
void handleStats(AsyncWebServerRequest *req)
{
    if (req->hasParam("hours"))
    {
        uint32_t hours = req->getParam("hours")->value().toInt();
        if (hours != ringStats.hours)
            calcRingStats(hours); // sofort neu bei Stunden-Änderung
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
    addStats("MPPT2_V",    ringStats.mppt2_v_min,    ringStats.mppt2_v_max,    ringStats.mppt2_v_avg);
    addStats("MPPT2_I",    ringStats.mppt2_i_min,    ringStats.mppt2_i_max,    ringStats.mppt2_i_avg);
    addStats("MPPT2_PV",   ringStats.mppt2_pv_min,   ringStats.mppt2_pv_max,   ringStats.mppt2_pv_avg);
    addStats("CHARGER_V",  ringStats.charger_v_min,  ringStats.charger_v_max,  ringStats.charger_v_avg);
    addStats("CHARGER_I",  ringStats.charger_i_min,  ringStats.charger_i_max,  ringStats.charger_i_avg);

    

    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
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
//log abfrage behandeln
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


//alle routen für den server, aus setup rufen 
void addRoutes()
{
    server.on("/api/stats", HTTP_GET, [](AsyncWebServerRequest *req)
        {    handleStats(req);  });

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
    
    server.on("/api/log", HTTP_GET, [](AsyncWebServerRequest *req) {
        handleLog(req); });

    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *req)
        { req->send(200, "application/json", buildDataJson()); });

    //max zeit im ringbuffer    
    server.on("/api/capacity", HTTP_GET, [](AsyncWebServerRequest *req) {
        uint32_t maxHours = (uint32_t)((uint64_t)RING_MAX_ENTRIES * 
                             appConfig.sensor_poll_interval_ms / 1000 / 3600);
        String out = "{\"maxHours\":" + String(maxHours) + "}";
        req->send(200, "application/json", out);
    });
    
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
}

//normal setup
// Stack-Größe für loopTask überschreiben
SET_LOOP_TASK_STACK_SIZE(16384);
void setup() {
    Serial.begin(115200);
    
    if (!LittleFS.begin(true))
        Serial.println("LittleFS FEHLER");
    else
        Serial.println("LittleFS OK");
    
    Serial.printf("Heap am Anfang: %lu\n", ESP.getFreeHeap());
    appConfigLoad();
    Serial.println("AppConfig OK");
    
    
        
    wifiSetup();
    Serial.printf("Heap nach wifi: %lu\n", ESP.getFreeHeap());
    Serial.println("WiFi OK");
    // Auf NTP warten – max 15 Sekunden
    uint32_t ntpStart = millis();
    while (!wifiTimeValid() && millis() - ntpStart < 15000)
        delay(100);
    Serial.printf("NTP: %s\n", wifiTimeValid() ? "sync" : "kein sync");
  
    sdSetup();
    Serial.println("SD OK");

    //wledSetup();
    Serial.println("WLED noch zu tun ");   

    ElegantOTA.begin(&server);


    esp_task_wdt_init(30, true); //etwas mehr zeit 
    //routen für die api
    addRoutes();
    
    server.begin(); //server starten 
    Serial.println("Server OK");

    sensorPollSetup(); //fuer die stats nötig
    Serial.printf("Heap nach Pollsetup: %lu\n", ESP.getFreeHeap());

 

    Serial.printf("Heap vor Display: %lu \n", (unsigned long)ESP.getFreeHeap());
    Serial.printf("PSRAM vor Display: %lu \n", (unsigned long)ESP.getFreePsram());
    
    smartdisplay_init();

    
    smartdisplay_lcd_set_backlight(1.0f);

    auto display = lv_display_get_default();
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);
    
    uiMainSetup();
    Serial.printf("Heap nach uimain: %lu\n", ESP.getFreeHeap());

    sdFillRingBuffer(RING_MAX_ENTRIES/1800); //ringbuffer füllen, falls aktuelle zeiten da. 
    Serial.printf("Heap nach ringbuffer: %lu\n", ESP.getFreeHeap());
    logPrintf("PSRAM: %lu bytes\n", (unsigned long)ESP.getPsramSize());
    logPrintln("Setup ist durch");    
}

void loop() {
    auto const now = millis();
   
    static uint32_t lastHeapLog = 0;
    // In loop(), einmalig nach NTP suchen:
    static bool ntpSynced = false;
    if (!ntpSynced && wifiTimeValid()) {
        ntpSynced = true;
        ESP_LOGI("NTP", "sync nachträglich");
        // sdFillRingBuffer nochmal aufrufen falls vorher kein Sync
    }
    if (millis() - lastHeapLog > 30000) // alle 30 Sekunden
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
    
   
    sensorPollLoop();
    sdLoop();

    if (sensorDataUpdated) {
        sensorDataUpdated = false;
        uiSensorenUpdate(true);
        uiChargerUpdate(true);
        uiDetailsUpdate(true);
    } else {
        uiSensorenUpdate();
        uiChargerUpdate();
        uiDetailsUpdate();
    }
    uiWledUpdate();
}
