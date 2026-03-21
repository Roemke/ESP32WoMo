#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "sensorpoll.h"
#include "wifi.h"
#include "sdcard.h"


//#include "vedirect.h"
//#include "bme280sensor.h"

// ----------------------------------------------------------------
// Interner State
// ----------------------------------------------------------------
//c++ lokale variante namespace statt static
namespace 
{
    bool     ready      = false;
    uint32_t lastLogMs  = 0;

    static char *s_historyBuffer = nullptr;  //speicher auf psram 

    struct CsvRow
    {
        time_t  ts;
        float   V, I, VS, SOC, P_W, T_C, H_pct, P_hPa;
        int     TTG, CO2;
        bool    valid;
    };



    // ----------------------------------------------------------------
    // Dateiname aus Datum bauen: /data/YYYY/YYYY-MM-DD.csv
    // ----------------------------------------------------------------
   String getFilename()
    {
        struct tm t;
        getLocalTime(&t, 0);
        char buf[40];
        strftime(buf, sizeof(buf), "/%Y/%Y-%m-%d.csv", &t);
        return String(buf);
    }

    // ----------------------------------------------------------------
    // CSV-Header schreiben wenn Datei neu angelegt wird
    // ----------------------------------------------------------------
    void writeHeader(File &f)
    {
        f.println("Datum,Zeit,V,I,VS,SOC,TTG_min,P_W,T_C,H_pct,P_hPa,CO2_ppm");
    }
    // ----------------------------------------------------------------
    // Eine Zeile mit aktuellen Messwerten schreiben
    // ----------------------------------------------------------------
    void writeDataLine()
    {
        String filename = getFilename();
        struct tm t;
        char yearDir[8] = "";
        if (!wifiTimeValid())
        {
           logPrintln("SD: NTP nicht sync, überspringe");
            return;
        }

        if (getLocalTime(&t, 0))
            strftime(yearDir, sizeof(yearDir), "/%Y", &t);

        if (strlen(yearDir) > 0 && !SD.exists(yearDir))
            SD.mkdir(yearDir);


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
        char datum[16] = "";
        char zeit[12]  = "";
        if (getLocalTime(&t, 0))
        {
            strftime(datum, sizeof(datum), "%Y-%m-%d", &t);
            strftime(zeit,  sizeof(zeit),  "%H:%M:%S", &t);
        }

        char line[160];
        snprintf(line, sizeof(line),
            "%s,%s,%.3f,%.3f,%.3f,%.1f,%d,%.1f,%.1f,%.1f,%.1f,%d",
            datum, zeit,
            sensorData.vedirect_valid ? sensorData.voltage         : -1.0f,
            sensorData.vedirect_valid ? sensorData.current         : 0.0f,
            sensorData.vedirect_valid ? sensorData.voltage_starter : -1.0f,
            sensorData.vedirect_valid ? sensorData.soc             : -1.0f,
            sensorData.vedirect_valid ? sensorData.ttg             : -1,
            sensorData.vedirect_valid ? sensorData.power           : 0.0f,
            sensorData.bme_valid      ? sensorData.temperature     : -99.0f,
            sensorData.bme_valid      ? sensorData.humidity        : -1.0f,
            sensorData.bme_valid      ? sensorData.pressure        : -1.0f,
            sensorData.co2_valid      ? sensorData.co2_ppm         : -1);

        f.println(line);
        f.close();
        logPrintf("SD: Zeile geschrieben in %s\n", filename.c_str());                
    }

    //history, nicht öffentlich
    time_t parseDateTime(const String &s)
    {
        // Format: 2025-06-10T08:00
        struct tm t = {};
        sscanf(s.c_str(), "%d-%d-%dT%d:%d",
            &t.tm_year, &t.tm_mon, &t.tm_mday,
            &t.tm_hour, &t.tm_min);
        t.tm_year -= 1900;
        t.tm_mon  -= 1;
        t.tm_isdst = -1;
        return mktime(&t);
    }

    CsvRow parseLine(const char *line)
    {
        CsvRow row = {};
        // Format: Datum,Zeit,V,I,VS,SOC,TTG_min,P_W,T_C,H_pct,P_hPa,CO2_ppm
        char datum[16], zeit[12];
        int n = sscanf(line, "%10[^,],%8[^,],%f,%f,%f,%f,%d,%f,%f,%f,%f,%d",
                    datum, zeit,
                    &row.V, &row.I, &row.VS, &row.SOC,
                    &row.TTG, &row.P_W, &row.T_C, &row.H_pct, &row.P_hPa, &row.CO2);
        if (n < 12) return row;

        // Timestamp zusammenbauen
        char dt[20];
        snprintf(dt, sizeof(dt), "%sT%.5s", datum, zeit);
        row.ts = parseDateTime(dt);        
        row.valid = true;
        return row;
    }

    // Interne Logik – schreibt in beliebiges Print-Ziel
    static void sdGetHistoryImpl(const String &from, const String &to, int points, Print &output)
    {
        time_t tsFrom = parseDateTime(from);
        time_t tsTo   = parseDateTime(to);

        // ---- Durchlauf 1: Zählen ----
        int total = 0;
        time_t day = tsFrom;
        while (day <= tsTo)
        {
            struct tm t;
            localtime_r(&day, &t);
            char path[32];
            strftime(path, sizeof(path), "/%Y/%Y-%m-%d.csv", &t);
            if (SD.exists(path))
            {
                File f = SD.open(path, FILE_READ);
                if (f)
                {
                    f.readStringUntil('\n');
                    while (f.available())
                    {
                        char line[160];
                        size_t len = f.readBytesUntil('\n', line, sizeof(line) - 1);
                        if (len == 0) continue;
                        line[len] = 0;
                        if (len > 0 && line[len-1] == '\r') line[--len] = 0;
                        if (len == 0) continue;
                        CsvRow row = parseLine(line);
                        if (row.valid && row.ts >= tsFrom && row.ts <= tsTo)
                            total++;
                    }
                    f.close();
                    esp_task_wdt_reset();
                }
            }
            esp_task_wdt_reset();
            day += 86400;
        }

        if (total == 0)
        {
            output.print("{\"error\":\"keine Daten\",\"labels\":[]}");
            return;
        }

        int bucketSize = max(1, total / points);

        // ---- Durchlauf 2: Downsampling ----
        logPrintf("vor neuer nutzung: Heap=%lu PSRAM=%lu\n",
            (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getFreePsram());


        // ---- Durchlauf 2: Downsampling direkt als JSON ----
        float T=0,H=0,Pr=0,V=0,I=0,SOC=0,PW=0,VS=0;
        int   CO2=0, cnt=0, rowIdx=0;
        time_t firstTs = 0;
        char tmp[32];

        // JSON Arrays – alle parallel aufbauen
        // Wir sammeln Labels und Werte in separaten Print-Puffern
        // geht nicht parallel ohne Speicher – also einmal komplett durch
        // und direkt ausgeben

        // Struktur: {"labels":[...],"T":[...],...}
        // Wir brauchen alle Buckets zuerst – also doch Buffer nötig
        // Aber: wir schreiben direkt in output, Arrays nacheinander
        // Das bedeutet: drei Durchläufe (einmal pro Kanal-Gruppe)
        // ODER: wir bauen ein struct pro Bucket im PSRAM

        // Pragmatische Lösung: Buckets als struct-Array im PSRAM
        struct Bucket {
            char   lbl[20];
            float  T, H, P, V, I, SOC, PW, VS;
            int    CO2;
        };

        int maxBuckets = points + 2;
        Bucket *buckets = (Bucket*)ps_malloc(maxBuckets * sizeof(Bucket));
        if (!buckets)
        {
            output.print("{\"error\":\"PSRAM voll\"}");
            return;
        }
        int bucketCount = 0;

        auto flushBucket = [&]()
        {
            if (cnt == 0 || bucketCount >= maxBuckets) return;
            Bucket &b = buckets[bucketCount++];
            struct tm lt;
            localtime_r(&firstTs, &lt);
            strftime(b.lbl, sizeof(b.lbl), "%d.%m %H:%M", &lt);
            b.T   = roundf(T/cnt  * 10)  / 10.0f;
            b.H   = roundf(H/cnt  * 10)  / 10.0f;
            b.P   = roundf(Pr/cnt * 10)  / 10.0f;
            b.CO2 = CO2/cnt;
            b.V   = roundf(V/cnt  * 100) / 100.0f;
            b.I   = roundf(I/cnt  * 100) / 100.0f;
            b.SOC = roundf(SOC/cnt* 10)  / 10.0f;
            b.PW  = roundf(PW/cnt * 10)  / 10.0f;
            b.VS  = roundf(VS/cnt * 10)  / 10.0f;
            T=0;H=0;Pr=0;V=0;I=0;SOC=0;PW=0;CO2=0;VS=0;cnt=0;
        };

        day = tsFrom;
        while (day <= tsTo)
        {
            struct tm t;
            localtime_r(&day, &t);
            char path[32];
            strftime(path, sizeof(path), "/%Y/%Y-%m-%d.csv", &t);
            if (SD.exists(path))
            {
                File f = SD.open(path, FILE_READ);
                if (f)
                {
                    f.readBytesUntil('\n', tmp, sizeof(tmp)); // Header
                    char line[160];
                    while (f.available())
                    {
                        size_t len = f.readBytesUntil('\n', line, sizeof(line)-1);
                        if (len == 0) continue;
                        line[len] = 0;
                        if (line[len-1] == '\r') line[--len] = 0;
                        if (len == 0) continue;
                        CsvRow row = parseLine(line);
                        if (!row.valid || row.ts < tsFrom || row.ts > tsTo) continue;
                        if (cnt == 0) firstTs = row.ts;
                        T+=row.T_C; H+=row.H_pct; Pr+=row.P_hPa;
                        CO2+=row.CO2; V+=row.V; I+=row.I;
                        SOC+=row.SOC; PW+=row.P_W; VS+=row.VS;
                        cnt++; rowIdx++;
                        if (rowIdx % bucketSize == 0) flushBucket();
                    }
                    f.close();
                    esp_task_wdt_reset();
                }
            }
            esp_task_wdt_reset();
            day += 86400;
        }
        flushBucket();

        // ---- JSON direkt schreiben ----
        output.print("{\"labels\":[");
        for (int i=0; i<bucketCount; i++)
        {
            if (i) output.print(',');
            output.print('"'); output.print(buckets[i].lbl); output.print('"');
        }
        output.print("],\"T\":[");
        for (int i=0; i<bucketCount; i++) { if (i) output.print(','); output.print(buckets[i].T,1); }
        output.print("],\"H\":[");
        for (int i=0; i<bucketCount; i++) { if (i) output.print(','); output.print(buckets[i].H,1); }
        output.print("],\"P\":[");
        for (int i=0; i<bucketCount; i++) { if (i) output.print(','); output.print(buckets[i].P,1); }
        output.print("],\"CO2\":[");
        for (int i=0; i<bucketCount; i++) { if (i) output.print(','); output.print(buckets[i].CO2); }
        output.print("],\"V\":[");
        for (int i=0; i<bucketCount; i++) { if (i) output.print(','); output.print(buckets[i].V,2); }
        output.print("],\"I\":[");
        for (int i=0; i<bucketCount; i++) { if (i) output.print(','); output.print(buckets[i].I,2); }
        output.print("],\"SOC\":[");
        for (int i=0; i<bucketCount; i++) { if (i) output.print(','); output.print(buckets[i].SOC,1); }
        output.print("],\"PW\":[");
        for (int i=0; i<bucketCount; i++) { if (i) output.print(','); output.print(buckets[i].PW,1); }
        output.print("],\"VS\":[");
        for (int i=0; i<bucketCount; i++) { if (i) output.print(','); output.print(buckets[i].VS,2); }

        snprintf(tmp, sizeof(tmp), "],\"total\":%d,\"points\":%d}", total, bucketCount);
        output.print(tmp);

        free(buckets);
    }
}
// ----------------------------------------------------------------
// Öffentliche API
// ----------------------------------------------------------------
bool sdSetup()
{
    //Datenspeicher
    s_historyBuffer = (char*)ps_malloc(2 * 1024 * 1024);
    if (!s_historyBuffer)
        logPrintln("SD: PSRAM Buffer FEHLER");
    else
    logPrintf("SD: PSRAM Buffer OK, %lu bytes\n", (unsigned long)2 * 1024 * 1024);
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

//für die history
// Für HTTP – direkt in Stream
void sdGetHistoryStream(const String &from, const String &to, int points, Print &output)
{
    if (!ready) { output.print("{\"error\":\"SD nicht bereit\"}"); return; }
    logPrintf("sdGetHistory: Heap=%lu PSRAM=%lu\n",
        (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getFreePsram());
    sdGetHistoryImpl(from, to, points, output);
    logPrintf("sdGetHistory: Heap=%lu PSRAM=%lu\n",
        (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getFreePsram());
}

// Für Display – in PSRAM Buffer
const char* sdGetHistoryBuffer(const String &from, const String &to, int points)
{
    if (!ready || !s_historyBuffer) return "{\"error\":\"SD nicht bereit\"}";
    // Buffer als Print-Wrapper
    class BufferPrint : public Print {
    public:
        char *buf; size_t pos, size;
        BufferPrint(char *b, size_t s) : buf(b), pos(0), size(s) {}
        size_t write(uint8_t c) override {
            if (pos < size-1) { buf[pos++] = c; buf[pos] = 0; } return 1;
        }
        size_t write(const uint8_t *b, size_t len) override {
            for (size_t i=0; i<len; i++) write(b[i]); return len;
        }
    };
    BufferPrint bp(s_historyBuffer, 2*1024*1024);
    sdGetHistoryImpl(from, to, points, bp);
    return s_historyBuffer;
}