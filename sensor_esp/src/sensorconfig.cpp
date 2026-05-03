#include "sensorconfig.h"
#include "logging.h"
#include <Preferences.h>

static const char* SENSOR_NVS_NAMESPACE = "sensorconfig";

// Defaults aus config.h
SensorConfig sensorConfig = {
    SENSOR_CONFIG_VERSION,
    BME280_SDA_PIN_DEFAULT,
    BME280_SCL_PIN_DEFAULT,
    BME280_I2C_ADDR_DEFAULT,
    BME280_INTERVAL_MS_DEFAULT,
    GAS_RAW_MIN_DEFAULT,
    GAS_RAW_MAX_DEFAULT,
    GAS_ENABLED_DEFAULT
};

bool sensorConfigLoad()
{
    Preferences prefs;
    prefs.begin(SENSOR_NVS_NAMESPACE, true);

    if (!prefs.isKey("version")) {
        prefs.end();
        logPrintln("Config: Keine Konfiguration gefunden, nutze Defaults");
        return sensorConfigSave();
    }

    uint8_t ver = prefs.getUChar("version", 0);
    if (ver != SENSOR_CONFIG_VERSION) {
        prefs.end();
        logPrintln("Config: Version veraltet, schreibe Defaults");
        return sensorConfigSave();
    }

    sensorConfig.bme_sda         = prefs.getUChar("bme_sda",      BME280_SDA_PIN_DEFAULT);
    sensorConfig.bme_scl         = prefs.getUChar("bme_scl",      BME280_SCL_PIN_DEFAULT);
    sensorConfig.bme_addr        = prefs.getUChar("bme_addr",     BME280_I2C_ADDR_DEFAULT);
    sensorConfig.bme_interval_ms = prefs.getUInt("bme_interval",  BME280_INTERVAL_MS_DEFAULT);
    
    sensorConfig.gas_raw_min = prefs.getInt("gas_raw_min", GAS_RAW_MIN_DEFAULT);
    sensorConfig.gas_raw_max = prefs.getInt("gas_raw_max", GAS_RAW_MAX_DEFAULT);
    sensorConfig.gas_enabled = prefs.getBool("gas_enabled", GAS_ENABLED_DEFAULT);

    prefs.end();

    logPrintf("Config: geladen – BME SDA=%d SCL=%d \n",
              sensorConfig.bme_sda, sensorConfig.bme_scl);
    return true;
}


bool sensorConfigSave()
{
    Preferences prefs;
    prefs.begin(SENSOR_NVS_NAMESPACE, false);
    prefs.putUChar("version",      sensorConfig.version);
    prefs.putUChar("bme_sda",      sensorConfig.bme_sda);
    prefs.putUChar("bme_scl",      sensorConfig.bme_scl);
    prefs.putUChar("bme_addr",     sensorConfig.bme_addr);
    prefs.putUInt("bme_interval",  sensorConfig.bme_interval_ms);
    prefs.putInt("gas_raw_min", sensorConfig.gas_raw_min);
    prefs.putInt("gas_raw_max", sensorConfig.gas_raw_max);
    prefs.putBool("gas_enabled", sensorConfig.gas_enabled);
    prefs.end();
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
    doc["gas_raw_min"] = sensorConfig.gas_raw_min;
    doc["gas_raw_max"] = sensorConfig.gas_raw_max;
    doc["gas_enabled"] = sensorConfig.gas_enabled;
    String out;
    serializeJson(doc, out);
    return out;
}
