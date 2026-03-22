#include "sensorconfig.h"
#include "logging.h"

// Defaults aus config.h
SensorConfig sensorConfig = {
    SENSOR_CONFIG_VERSION,
    BME280_SDA_PIN_DEFAULT,
    BME280_SCL_PIN_DEFAULT,
    BME280_I2C_ADDR_DEFAULT,
    BME280_INTERVAL_MS_DEFAULT
};

bool sensorConfigLoad()
{
    File f = LittleFS.open(CONFIG_PATH, "r");
    if (!f)
    {
        logPrintln("Config: Keine Konfiguration gefunden, nutze Defaults");
        return sensorConfigSave(); // Defaults speichern
    }
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();    
    if (err)
    {
        logPrintln("Config: Fehler beim Lesen, nutze Defaults");
        return false;
    }
    uint8_t ver = doc["version"] | 0;
    if (ver != SENSOR_CONFIG_VERSION)
    {
        logPrintln("Config: Version veraltet, schreibe Defaults");
        return sensorConfigSave();
    }

    sensorConfig.bme_sda      = doc["bme_sda"]      | BME280_SDA_PIN_DEFAULT;
    sensorConfig.bme_scl      = doc["bme_scl"]      | BME280_SCL_PIN_DEFAULT;
    sensorConfig.bme_addr     = doc["bme_addr"]      | BME280_I2C_ADDR_DEFAULT;
    sensorConfig.bme_interval_ms = doc["bme_interval_ms"] | BME280_INTERVAL_MS_DEFAULT;

    logPrintf("Config: geladen – BME SDA=%d SCL=%d \n",
              sensorConfig.bme_sda, sensorConfig.bme_scl
              );
    return true;
}

bool sensorConfigSave()
{
    File f = LittleFS.open(CONFIG_PATH, "w");
    if (!f)
    {
        logPrintln("Config: Fehler beim Speichern");
        return false;
    }
    f.print(sensorConfigToJson());
    f.close();
    logPrintln("Config: gespeichert");
    return true;
}

String sensorConfigToJson()
{
    JsonDocument doc;
    doc["version"] = SENSOR_CONFIG_VERSION;
    doc["bme_sda"]       = sensorConfig.bme_sda;
    doc["bme_scl"]       = sensorConfig.bme_scl;
    doc["bme_addr"]      = sensorConfig.bme_addr;
    doc["bme_interval_ms"] = sensorConfig.bme_interval_ms;
    String out;
    serializeJson(doc, out);
    return out;
}
