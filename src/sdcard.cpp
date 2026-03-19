#include "sdcard.h"
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
//#include "vedirect.h"
//#include "bme280sensor.h"

// ----------------------------------------------------------------
// Interner State
// ----------------------------------------------------------------
namespace
{
    bool     ready      = false;
    uint32_t lastLogMs  = 0;

    // ----------------------------------------------------------------
    // Dateiname aus Datum bauen: /data/YYYY-MM-DD.csv
    // Ohne RTC nutzen wir millis()-basierte Laufzeit als Timestamp.
    // Sobald du eine RTC oder NTP-Zeit hast, hier ersetzen.
    // ----------------------------------------------------------------
    String getFilename()
    {
        // Laufzeit in Tage/Stunden/Minuten umrechnen
        uint32_t sec   = millis() / 1000;
        uint32_t days  = sec / 86400;
        // Dateiname: /data/day_XXXXX.csv
        char buf[32];
        snprintf(buf, sizeof(buf), "/data/day_%05lu.csv", (unsigned long)days);
        return String(buf);
    }

    // ----------------------------------------------------------------
    // CSV-Header schreiben wenn Datei neu angelegt wird
    // ----------------------------------------------------------------
    void writeHeader(File &f)
    {
        f.println("uptime_ms,V,I,VS,SOC,TTG,P_W,T_C,H_pct,P_hPa");
    }

    // ----------------------------------------------------------------
    // Eine Zeile mit aktuellen Messwerten schreiben
    // ----------------------------------------------------------------
    void writeDataLine()
    {
        String filename = getFilename();

        // Verzeichnis anlegen falls nicht vorhanden
        if (!SD.exists("/data"))
            SD.mkdir("/data");

        // Datei öffnen (append) oder neu anlegen
        bool isNew = !SD.exists(filename);
        File f = SD.open(filename, FILE_APPEND);
        if (!f)
        {
            logPrintf("SD: Fehler beim Öffnen von %s\n", filename.c_str());
            return;
        }

        if (isNew)
        {
            writeHeader(f);
            logPrintf("SD: Neue Datei angelegt: %s\n", filename.c_str());
        }

        // Zeile zusammenbauen
        char line[128];
        /*
        snprintf(line, sizeof(line),
                 "%lu,%.3f,%.3f,%.3f,%.1f,%ld,%.1f,%.1f,%.1f,%.1f",
                 (unsigned long)millis(),
                 bmvData.valid ? bmvData.voltage        : -1.0f,
                 bmvData.valid ? bmvData.current        : 0.0f,
                 bmvData.valid ? bmvData.voltageStarter : -1.0f,
                 bmvData.valid ? bmvData.soc            : -1.0f,
                 bmvData.valid ? bmvData.timeToGo       : -1L,
                 bmvData.valid ? bmvData.power          : 0.0f,
                 bme280Data.valid ? bme280Data.temperature : -99.0f,
                 bme280Data.valid ? bme280Data.humidity    : -1.0f,
                 bme280Data.valid ? bme280Data.pressure    : -1.0f);

        f.println(line);*/
        f.close();

        logPrintf("SD: Zeile geschrieben in %s - noe noch nicht\n", filename.c_str());
    }
}

// ----------------------------------------------------------------
// Öffentliche API
// ----------------------------------------------------------------
bool sdSetup()
{
    // SPI-Bus mit den Board-definierten Pins initialisieren
    SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    if (!SD.begin(SD_CS_PIN))
    {
        logPrintln("SD: Keine Karte gefunden oder Initialisierung fehlgeschlagen");
        ready = false;
        return false;
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    logPrintf("SD: Karte initialisiert, Größe: %llu MB\n", cardSize);
    ready = true;
    return true;
}

void sdLoop()
{
    if (!ready) return;
    if (millis() - lastLogMs < SD_LOG_INTERVAL_MS) return;
    lastLogMs = millis();
    writeDataLine();
}

bool sdIsReady()
{
    return ready;
}

String sdGetStatus()
{
    JsonDocument doc;
    doc["ready"] = ready;
    if (ready)
    {
        doc["size_mb"]  = (uint32_t)(SD.cardSize()  / (1024 * 1024));
        doc["used_mb"]  = (uint32_t)(SD.usedBytes() / (1024 * 1024));
        doc["free_mb"]  = (uint32_t)((SD.cardSize() - SD.usedBytes()) / (1024 * 1024));
        doc["interval_s"] = SD_LOG_INTERVAL_MS / 1000;
    }
    String out;
    serializeJson(doc, out);
    return out;
}
