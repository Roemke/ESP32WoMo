#include <Arduino.h>
#include <LittleFS.h>
#include <esp32_smartdisplay.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include "wifi.h"
#include "wled.h"
//#include "vedirect.h" //raus macht ein anderer esp
//#include "bme280sensor.h"
#include "sdcard.h"
#include "ui_sensoren.h"
#include "indexHtmlJS.h"
#include "config.h"  // für Defaults
#include "appconfig.h" //für konfigurierbare geschichten 
#include "sensorpoll.h"


AsyncWebServer server(80);
static auto lv_last_tick = millis();


//user interface
static lv_obj_t *ui_tabview = nullptr;

static void setupUI()
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Tabview über den ganzen Screen
    ui_tabview = lv_tabview_create(scr);
    lv_tabview_set_tab_bar_position(ui_tabview, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(ui_tabview, 40);
    lv_obj_set_size(ui_tabview, 800, 480);
    lv_obj_set_pos(ui_tabview, 0, 0);
    lv_obj_set_style_bg_color(ui_tabview, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(ui_tabview, LV_OPA_COVER, 0);

    // Tab-Leiste dunkler
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(ui_tabview);
    lv_obj_set_style_bg_color(tab_bar, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_bg_opa(tab_bar, LV_OPA_COVER, 0);


    // Tabs anlegen
    lv_obj_t *tab_sensoren   = lv_tabview_add_tab(ui_tabview, "Sensoren");
    lv_obj_t *tab_licht      = lv_tabview_add_tab(ui_tabview, "Beleuchtung");
    lv_obj_t *tab_bat_hist   = lv_tabview_add_tab(ui_tabview, "Bat-Verlauf");
    lv_obj_t *tab_klima_hist = lv_tabview_add_tab(ui_tabview, "Klima-Verlauf");

    // Platzhalter für History-Tabs
    for (auto tab : {tab_bat_hist, tab_klima_hist})
    {
        lv_obj_set_style_bg_color(tab, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0);
        lv_obj_t *lbl = lv_label_create(tab);
        lv_label_set_text(lbl, "kommt noch...");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
        lv_obj_center(lbl);
    }

    // Tab Beleuchtung – Platzhalter
    lv_obj_set_style_bg_color(tab_licht, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(tab_licht, LV_OPA_COVER, 0);
    lv_obj_t *lbl_licht = lv_label_create(tab_licht);
    lv_label_set_text(lbl_licht, "kommt noch...");
    lv_obj_set_style_text_color(lbl_licht, lv_color_hex(0x888888), 0);
    lv_obj_center(lbl_licht);

    // Durch alle Buttons im Tab-Bar iterieren, Farben etwas anpassen
    uint32_t tab_count = lv_tabview_get_tab_count(ui_tabview);
    for (uint32_t i = 0; i < tab_count; i++)
    {
        lv_obj_t *btn = lv_obj_get_child(tab_bar, i);
        // Inaktiv
        lv_obj_set_style_text_color(btn, lv_color_hex(0x8888BB), LV_STATE_DEFAULT);
        // Aktiv
        lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x2233AA), LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_CHECKED);
    }

    // Tab Sensoren – ui_sensoren.cpp
    uiSensorenSetup(tab_sensoren);

    Serial.println("setupUI OK");
}

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

    uiSensorenSetIP(wifiGetIP());

    //wledSetup();
    Serial.println("WLED OK");
/* war mal hier gedacht, ausgelagert
    if (!bme280Setup())
        Serial.println("BME280 nicht gefunden");
    else
        Serial.println("BME280 OK");


    vedirectSetup();
    Serial.println("VEDirect OK");
*/
   

    ElegantOTA.begin(&server);
    //routen für die api

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



    server.begin();
    Serial.println("Server OK");

    sdSetup();
    Serial.println("SD OK");


    
    smartdisplay_init();

    
    smartdisplay_lcd_set_backlight(1.0f);

    auto display = lv_display_get_default();
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);
    
    setupUI();

    Serial.println("Setup ist durch");
}

void loop() {
    auto const now = millis();
    /*
    lv_tick_inc(now - lv_last_tick);
    lv_last_tick = now;
    lv_timer_handler();
    */
   
    sdLoop();
    sensorPollLoop();
    //wledLoop();

    // UI alle 2 Sekunden aktualisieren
    /*
    static uint32_t lastUpdate = 0;
    if (now - lastUpdate >= 2000)
    {
        lastUpdate = now;
        uiSensorenUpdate();
        uiSensorenSetIP(wifiGetIP());
    }
    */
}
