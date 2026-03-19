#include "sensorconfig.h"
#include "logging.h"

// Defaults aus config.h
SensorConfig sensorConfig = {
    BME280_SDA_PIN_DEFAULT,
    BME280_SCL_PIN_DEFAULT,
    BME280_I2C_ADDR_DEFAULT,
    VEDIRECT_RX_PIN_DEFAULT,
    VEDIRECT_BAUD
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
    sensorConfig.bme_sda      = doc["bme_sda"]      | BME280_SDA_PIN_DEFAULT;
    sensorConfig.bme_scl      = doc["bme_scl"]      | BME280_SCL_PIN_DEFAULT;
    sensorConfig.bme_addr     = doc["bme_addr"]      | BME280_I2C_ADDR_DEFAULT;
    sensorConfig.bme_interval_ms = doc["bme_interval_ms"] | BME280_INTERVAL_MS_DEFAULT;
    sensorConfig.vedirect_rx  = doc["vedirect_rx"]   | VEDIRECT_RX_PIN_DEFAULT;
    sensorConfig.vedirect_baud= doc["vedirect_baud"] | VEDIRECT_BAUD;

    logPrintf("Config: geladen – BME SDA=%d SCL=%d  VE.Direct RX=%d\n",
              sensorConfig.bme_sda, sensorConfig.bme_scl,
              sensorConfig.vedirect_rx);
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
    doc["bme_sda"]       = sensorConfig.bme_sda;
    doc["bme_scl"]       = sensorConfig.bme_scl;
    doc["bme_addr"]      = sensorConfig.bme_addr;
    doc["bme_interval_ms"] = sensorConfig.bme_interval_ms;
    doc["vedirect_rx"]   = sensorConfig.vedirect_rx;
    doc["vedirect_baud"] = sensorConfig.vedirect_baud;
    String out;
    serializeJson(doc, out);
    return out;
}
