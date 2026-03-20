#include "appconfig.h"

AppConfig appConfig;

void appConfigLoad()
{
    File f = LittleFS.open(APP_CONFIG_PATH, "r");
    if (!f)
    {
        // Defaults
        strlcpy(appConfig.sensor_esp_ip,  SENSOR_ESP_IP_DEFAULT,    sizeof(appConfig.sensor_esp_ip));
        strlcpy(appConfig.wled_innen_ip,  WLED_DEFAULT_INNEN_IP,    sizeof(appConfig.wled_innen_ip));
        strlcpy(appConfig.wled_aussen_ip, WLED_DEFAULT_AUSSEN_IP,   sizeof(appConfig.wled_aussen_ip));
        appConfigSave();
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, f);
    f.close();
    strlcpy(appConfig.sensor_esp_ip,  doc["sensor_esp_ip"]  | SENSOR_ESP_IP_DEFAULT,  sizeof(appConfig.sensor_esp_ip));
    strlcpy(appConfig.wled_innen_ip,  doc["wled_innen_ip"]  | WLED_DEFAULT_INNEN_IP,  sizeof(appConfig.wled_innen_ip));
    strlcpy(appConfig.wled_aussen_ip, doc["wled_aussen_ip"] | WLED_DEFAULT_AUSSEN_IP, sizeof(appConfig.wled_aussen_ip));
}

void appConfigSave()
{
    File f = LittleFS.open(APP_CONFIG_PATH, "w");
    if (!f) return;
    JsonDocument doc;
    doc["sensor_esp_ip"]  = appConfig.sensor_esp_ip;
    doc["wled_innen_ip"]  = appConfig.wled_innen_ip;
    doc["wled_aussen_ip"] = appConfig.wled_aussen_ip;
    serializeJson(doc, f);
    f.close();
}

String appConfigToJson()
{
    JsonDocument doc;
    doc["sensor_esp_ip"]  = appConfig.sensor_esp_ip;
    doc["wled_innen_ip"]  = appConfig.wled_innen_ip;
    doc["wled_aussen_ip"] = appConfig.wled_aussen_ip;
    String out;
    serializeJson(doc, out);
    return out;
}