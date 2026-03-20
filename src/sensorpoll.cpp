#include "sensorpoll.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "appconfig.h"
#include "logging.h"
#include "wifi.h"

SensorData sensorData = {};


void sensorPollLoop()
{
    if (!wifiIsConnected()) return;

    static uint32_t lastPollMs = 0;
    if (millis() - lastPollMs < SENSOR_POLL_INTERVAL_MS) return;
    lastPollMs = millis();

    HTTPClient http;
    String url = "http://" + String(appConfig.sensor_esp_ip) + "/api/data";
    http.begin(url);
    http.setTimeout(3000);

    int code = http.GET();
    if (code != 200)
    {
        logPrintf("SensorPoll: HTTP Fehler %d\n", code);
        http.end();
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();

    if (err)
    {
        logPrintln("SensorPoll: JSON Fehler");
        return;
    }

    // BME280
    sensorData.bme_valid = doc["bme"]["valid"] | false;
    if (sensorData.bme_valid)
    {
        sensorData.temperature = doc["bme"]["T"] | 0.0f;
        sensorData.humidity    = doc["bme"]["H"] | 0.0f;
        sensorData.pressure    = doc["bme"]["P"] | 0.0f;
    }

    // VE.Direct
    sensorData.vedirect_valid = doc["vedirect"]["valid"] | false;
    if (sensorData.vedirect_valid)
    {
        sensorData.voltage         = doc["vedirect"]["V"]   | 0.0f;
        sensorData.current         = doc["vedirect"]["I"]   | 0.0f;
        sensorData.power           = doc["vedirect"]["P"]   | 0.0f;
        sensorData.soc             = doc["vedirect"]["SOC"] | 0.0f;
        sensorData.ttg             = doc["vedirect"]["TTG"] | -1;
        sensorData.voltage_starter = doc["vedirect"]["VS"]  | 0.0f;
    }

    // CO2
    sensorData.co2_valid = doc["co2"]["valid"] | false;
    if (sensorData.co2_valid)
        sensorData.co2_ppm = doc["co2"]["ppm"] | 0;

    logPrintln("SensorPoll: OK");
}