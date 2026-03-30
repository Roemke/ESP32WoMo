#include "scd41sensor.h"
#include "logging.h"
#include <SensirionI2cScd4x.h>
#include <Wire.h>
#include <ArduinoJson.h>

static SensirionI2cScd4x scd4x;

struct Scd41Data {
    bool    valid;
    int     co2;
    float   temperature;
    float   humidity;
    uint32_t lastUpdateMs;
};

static Scd41Data s_data = {};

bool scd41Setup()
{
    scd4x.begin(Wire, SCD41_I2C_ADDR_62);
    delay(30);

    int16_t err = scd4x.wakeUp();
    logPrintf("SCD41: wakeUp err=%d\n", err);
    
    err = scd4x.stopPeriodicMeasurement();
    logPrintf("SCD41: stopPeriodic err=%d\n", err);
    delay(500);

    err = scd4x.reinit();
    logPrintf("SCD41: reinit err=%d\n", err);
    delay(30);

    uint64_t serialNumber = 0;
    err = scd4x.getSerialNumber(serialNumber);
    logPrintf("SCD41: serialNumber err=%d\n", err);
    if (err) return false;

    err = scd4x.startPeriodicMeasurement();
    logPrintf("SCD41: startPeriodic err=%d\n", err);
    if (err) return false;

    logPrintln("SCD41: gestartet");
    return true;
}
void scd41Loop()
{
    static uint32_t lastMs = 0;
    static uint32_t startMs = 0;
    if (startMs == 0) startMs = millis();
    if (millis() - startMs < 10000) return; // 10s warten nach Start
    if (millis() - lastMs < 5000) return;
    lastMs = millis();

   uint16_t dataReadyRaw = 0;
    int16_t err = scd4x.getDataReadyStatusRaw(dataReadyRaw);
    logPrintf("SCD41: err=%d raw=0x%04X ready=%d\n", err, dataReadyRaw, (dataReadyRaw & 0x07FF) != 0);
    bool ready = (dataReadyRaw & 0x07FF) != 0;
    if (!ready) return;
    

    uint16_t co2 = 0;
    float temp = 0, hum = 0;
    err = scd4x.readMeasurement(co2, temp, hum);
    if (err) { logPrintf("SCD41: readMeasurement Fehler %d\n", err); return; }
    if (co2 == 0) { logPrintln("SCD41: CO2=0"); return; }

    s_data.valid        = true;
    s_data.co2          = co2;
    s_data.temperature  = temp;
    s_data.humidity     = hum;
    s_data.lastUpdateMs = millis();
    logPrintf("SCD41: CO2=%d ppm T=%.1f H=%.1f\n", co2, temp, hum);
}
bool scd41IsValid()
{
    return s_data.valid && (millis() - s_data.lastUpdateMs < 30000);
}

int   scd41GetCO2()         { return s_data.co2; }
float scd41GetTemperature() { return s_data.temperature; }
float scd41GetHumidity()    { return s_data.humidity; }

String scd41ToJson()
{
    JsonDocument doc;
    doc["valid"] = scd41IsValid();
    if (scd41IsValid())
        doc["co2"] = s_data.co2;
    String out;
    serializeJson(doc, out);
    return out;
}

String scd41ToJsonFull()
{
    JsonDocument doc;
    doc["valid"] = scd41IsValid();
    if (scd41IsValid())
    {
        doc["co2"] = s_data.co2;
        doc["T"]   = serialized(String(s_data.temperature, 1));
        doc["H"]   = serialized(String(s_data.humidity,    1));
    }
    String out;
    serializeJson(doc, out);
    return out;
}