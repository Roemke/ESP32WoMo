#include "bme280sensor.h"
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>

// ----------------------------------------------------------------
// Globale Daten
// ----------------------------------------------------------------
Bme280Data bme280Data = {0, 0, 0, false, 0};

// ----------------------------------------------------------------
// Interner State
// ----------------------------------------------------------------
namespace
{
    Adafruit_BME280 bme;
    bool            initialized = false;
    uint32_t        lastPollMs  = 0;
}

// ----------------------------------------------------------------
// Setup – Wire auf konfigurierten Pins starten, Sensor suchen
// ----------------------------------------------------------------
bool bme280Setup()
{
    Wire1.begin(BME280_SDA_PIN, BME280_SCL_PIN);

    if (!bme.begin(BME280_I2C_ADDR, &Wire1))
    {
        // Zweite Adresse versuchen (0x76 ↔ 0x77)
        uint8_t altAddr = (BME280_I2C_ADDR == 0x76) ? 0x77 : 0x76;
        logPrintf("BME280: Nicht auf 0x%02X gefunden, versuche 0x%02X\n",
                  BME280_I2C_ADDR, altAddr);

        if (!bme.begin(altAddr, &Wire1))
        {
            logPrintln("BME280: Sensor nicht gefunden – Pins und Adresse prüfen");
            initialized = false;
            return false;
        }
        logPrintf("BME280: Sensor gefunden auf 0x%02X\n", altAddr);
    }
    else
    {
        logPrintf("BME280: Sensor gefunden auf 0x%02X\n", BME280_I2C_ADDR);
    }

    // Empfohlene Einstellungen für Innenraum-Monitoring
    // (geringe Auflösung, dafür rauscharmer)
    bme.setSampling(
        Adafruit_BME280::MODE_NORMAL,
        Adafruit_BME280::SAMPLING_X2,   // Temperatur
        Adafruit_BME280::SAMPLING_X16,  // Luftdruck
        Adafruit_BME280::SAMPLING_X1,   // Luftfeuchtigkeit
        Adafruit_BME280::FILTER_X16,
        Adafruit_BME280::STANDBY_MS_500
    );

    initialized = true;
    logPrintf("BME280: Initialisiert, SDA=GPIO%d, SCL=GPIO%d, Intervall=%dms\n",
              BME280_SDA_PIN, BME280_SCL_PIN, BME280_INTERVAL_MS);
    return true;
}

// ----------------------------------------------------------------
// Loop – Messung nur wenn Intervall abgelaufen
// ----------------------------------------------------------------
void bme280Loop()
{
    if (!initialized) return;
    if (millis() - lastPollMs < BME280_INTERVAL_MS) return;
    lastPollMs = millis();

    float t = bme.readTemperature();
    float h = bme.readHumidity();
    float p = bme.readPressure() / 100.0f; // Pa → hPa

    // Plausibilitätsprüfung
    if (isnan(t) || isnan(h) || isnan(p) || t < -40 || t > 85)
    {
        logPrintln("BME280: Messfehler – ungültige Werte");
        return;
    }

    bme280Data.temperature  = t;
    bme280Data.humidity     = h;
    bme280Data.pressure     = p;
    bme280Data.valid        = true;
    bme280Data.lastUpdateMs = millis();

    logPrintf("BME280: T=%.1f°C  H=%.1f%%  P=%.1fhPa\n", t, h, p);
}

// ----------------------------------------------------------------
// Öffentliche Hilfsfunktionen
// ----------------------------------------------------------------
bool bme280IsValid()
{
    if (!bme280Data.valid) return false;
    return (millis() - bme280Data.lastUpdateMs) < (BME280_INTERVAL_MS * 3);
}

String bme280ToJson()
{
    JsonDocument doc;
    if (bme280IsValid())
    {
        doc["valid"] = true;
        doc["T"]     = serialized(String(bme280Data.temperature, 1));
        doc["H"]     = serialized(String(bme280Data.humidity,    1));
        doc["P"]     = serialized(String(bme280Data.pressure,    1));
        doc["age_ms"] = (millis() - bme280Data.lastUpdateMs);
    }
    else
    {
        doc["valid"] = false;
    }
    String out;
    serializeJson(doc, out);
    return out;
}
