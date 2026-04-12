#include "appconfig.h"

#define USE_NVS_FOR_CONFIG 1
#if USE_NVS_FOR_CONFIG
    #include <Preferences.h>
#else
    #include <LittleFS.h>
#endif

//extern esp_lcd_panel_handle_t lcd_handle; // aus smartdisplay
//flackern verhindern, ging nicht
/*
    flackern anscheinend durch littlefs und display.
    Lösung: verwende preferences, die sind schneller und verursachen kein flackern.
    ich hatte preferences tatsächlich vergessen :-( ist eigentlich die bessere Lösung 
    es geht um key-value pairs, da ist preferences besser geeignet als eine json datei.
*/


AppConfig appConfig;

#if USE_NVS_FOR_CONFIG
static Preferences prefs;
static const char* NVS_NAMESPACE = "appconfig";

void appConfigLoad()
{
    prefs.begin(NVS_NAMESPACE, true); // read-only
    strlcpy(appConfig.sensor_esp_ip,
            prefs.getString("sensor_ip",   SENSOR_ESP_IP_DEFAULT).c_str(),
            sizeof(appConfig.sensor_esp_ip));
    strlcpy(appConfig.wled_innen_ip,
            prefs.getString("wled_innen",  WLED_DEFAULT_INNEN_IP).c_str(),
            sizeof(appConfig.wled_innen_ip));
    strlcpy(appConfig.wled_aussen_ip,
            prefs.getString("wled_aussen", WLED_DEFAULT_AUSSEN_IP).c_str(),
            sizeof(appConfig.wled_aussen_ip));
    appConfig.sensor_poll_interval_ms =
            prefs.getUInt("poll_ms",     SENSOR_POLL_INTERVAL_MS);
    appConfig.display_brightness =
            prefs.getUChar("brightness", DISPLAY_BRIGHTNESS_DEFAULT);
    appConfig.indicator_opacity =
            prefs.getUChar("opacity",    INDICATOR_OPACITY_DEFAULT);
    prefs.end();
}

void appConfigSave()
{
    prefs.begin(NVS_NAMESPACE, false); // read-write
    prefs.putString("sensor_ip",   appConfig.sensor_esp_ip);
    prefs.putString("wled_innen",  appConfig.wled_innen_ip);
    prefs.putString("wled_aussen", appConfig.wled_aussen_ip);
    prefs.putUInt("poll_ms",       appConfig.sensor_poll_interval_ms);
    prefs.putUChar("brightness",   appConfig.display_brightness);
    prefs.putUChar("opacity",      appConfig.indicator_opacity);
    prefs.end();
}
#else

#define APP_CONFIG_PATH "/appconfig.json"
static bool saveInProgress = false; 

void appConfigLoad()
{
    File f = LittleFS.open(APP_CONFIG_PATH, "r");
    // in appConfigLoad() Defaults:
    if (!f)
    {
        // Defaults
        strlcpy(appConfig.sensor_esp_ip,  SENSOR_ESP_IP_DEFAULT,    sizeof(appConfig.sensor_esp_ip));
        strlcpy(appConfig.wled_innen_ip,  WLED_DEFAULT_INNEN_IP,    sizeof(appConfig.wled_innen_ip));
        strlcpy(appConfig.wled_aussen_ip, WLED_DEFAULT_AUSSEN_IP,   sizeof(appConfig.wled_aussen_ip));
        appConfig.sensor_poll_interval_ms = SENSOR_POLL_INTERVAL_MS; 
        appConfigSave();
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, f);
    f.close();
    strlcpy(appConfig.sensor_esp_ip,  doc["sensor_esp_ip"]  | SENSOR_ESP_IP_DEFAULT,  sizeof(appConfig.sensor_esp_ip));
    strlcpy(appConfig.wled_innen_ip,  doc["wled_innen_ip"]  | WLED_DEFAULT_INNEN_IP,  sizeof(appConfig.wled_innen_ip));
    strlcpy(appConfig.wled_aussen_ip, doc["wled_aussen_ip"] | WLED_DEFAULT_AUSSEN_IP, sizeof(appConfig.wled_aussen_ip));
    appConfig.sensor_poll_interval_ms = doc["sensor_poll_interval_ms"] | SENSOR_POLL_INTERVAL_MS;
    appConfig.display_brightness = doc["display_brightness"] | DISPLAY_BRIGHTNESS_DEFAULT;
    appConfig.indicator_opacity = doc["indicator_opacity"] | INDICATOR_OPACITY_DEFAULT;

}

void appConfigSave()
{    
    if (saveInProgress) return; //wenn schon ein save läuft, nicht nochmal starten
    saveInProgress = true;
    //esp_lcd_rgb_panel_set_pclk(lcd_handle, 6000000); // 6 MHz, herunter setzen um flackern 
                                                     //des displays zu vermeiden ging nicht
    File f = LittleFS.open(APP_CONFIG_PATH, "w");
    if (!f) return;
    JsonDocument doc;
    doc["sensor_esp_ip"]  = appConfig.sensor_esp_ip;
    doc["wled_innen_ip"]  = appConfig.wled_innen_ip;
    doc["wled_aussen_ip"] = appConfig.wled_aussen_ip;
    doc["sensor_poll_interval_ms"] = appConfig.sensor_poll_interval_ms;
    doc["display_brightness"] = appConfig.display_brightness;
    doc["indicator_opacity"] = appConfig.indicator_opacity;


    serializeJson(doc, f);
    f.close();
    // PCLK wieder erhöhen
    //esp_lcd_rgb_panel_set_pclk(lcd_handle, 12500000); // zurück auf 12.5 MHz
    saveInProgress = false;
}
#endif



String appConfigToJson()
{
    JsonDocument doc;
    doc["sensor_esp_ip"]  = appConfig.sensor_esp_ip;
    doc["wled_innen_ip"]  = appConfig.wled_innen_ip;
    doc["wled_aussen_ip"] = appConfig.wled_aussen_ip;
    doc["sensor_poll_interval_ms"] = appConfig.sensor_poll_interval_ms;
    doc["display_brightness"] = appConfig.display_brightness;
    doc["indicator_opacity"] = appConfig.indicator_opacity;

    String out;
    serializeJson(doc, out);
    return out;
}


