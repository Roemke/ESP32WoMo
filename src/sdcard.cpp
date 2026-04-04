#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "sensorpoll.h"
#include "wifi.h"
#include "sdcard.h"
#include "appconfig.h"


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

   struct CsvRow {
        time_t  ts;
        float   V, I, VS, SOC, P_W, T_C, H_pct, P_hPa;
        int     TTG, CO2;
        float   mppt1_V, mppt1_I, mppt1_PV;
        float   mppt2_V, mppt2_I, mppt2_PV;
        float   charger_V, charger_I;
        uint8_t valid_flags;
        int     fields;  // Anzahl erfolgreich gelesener Felder
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
        //f.println("Datum,Zeit,V,I,VS,SOC,TTG_min,P_W,T_C,H_pct,P_hPa,CO2_ppm");
        f.println("Datum,Zeit,V,I,VS,SOC,TTG_min,P_W,T_C,H_pct,P_hPa,CO2_ppm,MPPT1_V,MPPT1_I,MPPT1_PV,MPPT2_V,MPPT2_I,MPPT2_PV,Charger_V,Charger_I,valid_flags");
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

        uint8_t flags = 0;
        if (sensorData.bme_valid)      flags |= VALID_BME;
        if (sensorData.vedirect_valid) flags |= VALID_VE;
        if (sensorData.co2_valid)      flags |= VALID_CO2;
        if (sensorData.mppt1_valid)    flags |= VALID_MPPT1;
        if (sensorData.mppt2_valid)    flags |= VALID_MPPT2;
        if (sensorData.charger_valid)  flags |= VALID_CHARGER;

        char line[240];
        snprintf(line, sizeof(line),
            "%s,%s,%.3f,%.3f,%.3f,%.1f,%d,%.1f,%.1f,%.1f,%.1f,%d,%.2f,%.2f,%.1f,%.2f,%.2f,%.1f,%.2f,%.2f,%d",
            datum, zeit,
            sensorData.vedirect_valid ? sensorData.voltage         : 0.0f,
            sensorData.vedirect_valid ? sensorData.current         : 0.0f,
            sensorData.vedirect_valid ? sensorData.voltage_starter : 0.0f,
            sensorData.vedirect_valid ? sensorData.soc             : 0.0f,
            sensorData.vedirect_valid ? sensorData.ttg             : -1,
            sensorData.vedirect_valid ? sensorData.power           : 0.0f,
            sensorData.bme_valid      ? sensorData.temperature     : 0.0f,
            sensorData.bme_valid      ? sensorData.humidity        : 0.0f,
            sensorData.bme_valid      ? sensorData.pressure        : 0.0f,
            sensorData.co2_valid      ? sensorData.co2_ppm         : 0,
            sensorData.mppt1_valid    ? sensorData.mppt1_voltage   : 0.0f,
            sensorData.mppt1_valid    ? sensorData.mppt1_current   : 0.0f,
            sensorData.mppt1_valid    ? sensorData.mppt1_pv_power  : 0.0f,
            sensorData.mppt2_valid    ? sensorData.mppt2_voltage   : 0.0f,
            sensorData.mppt2_valid    ? sensorData.mppt2_current   : 0.0f,
            sensorData.mppt2_valid    ? sensorData.mppt2_pv_power  : 0.0f,
            sensorData.charger_valid  ? sensorData.charger_voltage : 0.0f,
            sensorData.charger_valid  ? sensorData.charger_current : 0.0f,
            flags);


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

    CsvRow parseLine(const char *line) {
        CsvRow row = {};
        char datum[16], zeit[12];

        // Neues Format versuchen (21 Felder)
        row.fields = sscanf(line,
            "%10[^,],%8[^,],%f,%f,%f,%f,%d,%f,%f,%f,%f,%d,%f,%f,%f,%f,%f,%f,%f,%f,%hhu",
            datum, zeit,
            &row.V, &row.I, &row.VS, &row.SOC,
            &row.TTG, &row.P_W, &row.T_C, &row.H_pct, &row.P_hPa, &row.CO2,
            &row.mppt1_V, &row.mppt1_I, &row.mppt1_PV,
            &row.mppt2_V, &row.mppt2_I, &row.mppt2_PV,
            &row.charger_V, &row.charger_I,
            &row.valid_flags);

        if (row.fields < 12) return row;  // ungültig

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
        float mppt1_V=0,mppt1_I=0,mppt1_PV=0;
        float mppt2_V=0,mppt2_I=0,mppt2_PV=0;
        float charger_V=0,charger_I=0;
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
            float  mppt1_V, mppt1_I, mppt1_PV;
            float  mppt2_V, mppt2_I, mppt2_PV;
            float  charger_V, charger_I;
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
            b.mppt1_V  = roundf(mppt1_V/cnt  * 100) / 100.0f;
            b.mppt1_I  = roundf(mppt1_I/cnt  * 100) / 100.0f;
            b.mppt1_PV = roundf(mppt1_PV/cnt * 10)  / 10.0f;
            b.mppt2_V  = roundf(mppt2_V/cnt  * 100) / 100.0f;
            b.mppt2_I  = roundf(mppt2_I/cnt  * 100) / 100.0f;
            b.mppt2_PV = roundf(mppt2_PV/cnt * 10)  / 10.0f;
            b.charger_V = roundf(charger_V/cnt * 100) / 100.0f;
            b.charger_I = roundf(charger_I/cnt * 100) / 100.0f;
            // Reset
            T=0;H=0;Pr=0;V=0;I=0;SOC=0;PW=0;CO2=0;VS=0;cnt=0;
            mppt1_V=0;mppt1_I=0;mppt1_PV=0;
            mppt2_V=0;mppt2_I=0;mppt2_PV=0;
            charger_V=0;charger_I=0;
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
                    char line[256];
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
                        if (row.fields == 21) {
                            mppt1_V  += row.mppt1_V;
                            mppt1_I  += row.mppt1_I;
                            mppt1_PV += row.mppt1_PV;
                            mppt2_V  += row.mppt2_V;
                            mppt2_I  += row.mppt2_I;
                            mppt2_PV += row.mppt2_PV;
                            charger_V += row.charger_V;
                            charger_I += row.charger_I;
                        }
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
        output.print("],\"MPPT1_V\":[");
        for (int i=0;i<bucketCount;i++){if(i)output.print(',');output.print(buckets[i].mppt1_V,2);}
        output.print("],\"MPPT1_I\":[");
        for (int i=0;i<bucketCount;i++){if(i)output.print(',');output.print(buckets[i].mppt1_I,2);}
        output.print("],\"MPPT1_PV\":[");
        for (int i=0;i<bucketCount;i++){if(i)output.print(',');output.print(buckets[i].mppt1_PV,1);}
        output.print("],\"MPPT2_V\":[");
        for (int i=0;i<bucketCount;i++){if(i)output.print(',');output.print(buckets[i].mppt2_V,2);}
        output.print("],\"MPPT2_I\":[");
        for (int i=0;i<bucketCount;i++){if(i)output.print(',');output.print(buckets[i].mppt2_I,2);}
        output.print("],\"MPPT2_PV\":[");
        for (int i=0;i<bucketCount;i++){if(i)output.print(',');output.print(buckets[i].mppt2_PV,1);}
        output.print("],\"CHARGER_V\":[");
        for (int i=0;i<bucketCount;i++){if(i)output.print(',');output.print(buckets[i].charger_V,2);}
        output.print("],\"CHARGER_I\":[");
        for (int i=0;i<bucketCount;i++){if(i)output.print(',');output.print(buckets[i].charger_I,2);}

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
    s_historyBuffer = (char*)ps_malloc(512 * 1024);
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

// Für Display – in PSRAM Buffer //werden wir nicht mehr brauchen, lasse charts im displsay
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

void sdFillRingBuffer(uint32_t hours)
{
    if (!ready || !ringBuffer) return;

    time_t tsTo   = time(nullptr);
    time_t tsFrom = tsTo - hours * 3600;
    //int reps = SD_LOG_INTERVAL_MS / RING_INTERVAL_MS; // = 30
    int reps = SD_LOG_INTERVAL_MS / appConfig.sensor_poll_interval_ms;
    logPrintf("sdFillRingBuffer: lade letzte %lu Stunden\n", hours);

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
                char tmp[32];
                f.readBytesUntil('\n', tmp, sizeof(tmp)); // Header
                char line[256];
                while (f.available())
                {
                    size_t len = f.readBytesUntil('\n', line, sizeof(line)-1);
                    if (len == 0) continue;
                    line[len] = 0;
                    if (line[len-1] == '\r') line[--len] = 0;
                    if (len == 0) continue;

                    CsvRow row = parseLine(line);
                    if (!row.valid || row.ts < tsFrom || row.ts > tsTo) continue;

                    for (int rep = 0; rep < reps; rep++) //reps ist bei standarddefines auf 30 
                    {
                        RingEntry &e = ringBuffer[ringHead];
                        e.valid_flags = 0;
                        if (row.fields == 21) {
                            // neues Format: flags direkt übernehmen
                            e.valid_flags = row.valid_flags;
                        } else {
                            // altes Format: aus Sentinel-Werten ableiten
                            if (row.T_C    > -99.0f) e.valid_flags |= VALID_BME;
                            if (row.V      > -1.0f)  e.valid_flags |= VALID_VE;
                            if (row.CO2    > -1)     e.valid_flags |= VALID_CO2;
                        }

                        e.T   = (e.valid_flags & VALID_BME) ? row.T_C   : 0.0f;
                        e.H   = (e.valid_flags & VALID_BME) ? row.H_pct : 0.0f;
                        e.P   = (e.valid_flags & VALID_BME) ? row.P_hPa : 0.0f;
                        e.CO2 = (e.valid_flags & VALID_CO2) ? row.CO2   : 0;
                        e.V   = (e.valid_flags & VALID_VE)  ? row.V     : 0.0f;
                        e.I   = (e.valid_flags & VALID_VE)  ? row.I     : 0.0f;
                        e.SOC = (e.valid_flags & VALID_VE)  ? row.SOC   : 0.0f;
                        e.PW  = (e.valid_flags & VALID_VE)  ? row.P_W   : 0.0f;
                        e.VS  = (e.valid_flags & VALID_VE)  ? row.VS    : 0.0f;
                        if (row.fields == 21) {
                            e.mppt1_V   = row.mppt1_V;
                            e.mppt1_I   = row.mppt1_I;
                            e.mppt1_PV  = row.mppt1_PV;
                            e.mppt2_V   = row.mppt2_V;
                            e.mppt2_I   = row.mppt2_I;
                            e.mppt2_PV  = row.mppt2_PV;
                            e.charger_V = row.charger_V;
                            e.charger_I = row.charger_I;
                        }

                        ringHead = (ringHead + 1) % RING_MAX_ENTRIES;
                        if (ringCount < RING_MAX_ENTRIES) ringCount++;
                    }
                    
                }
                f.close();
                esp_task_wdt_reset();
            }
        }
        day += 86400;
    }
    logPrintf("sdFillRingBuffer: %lu Einträge geladen\n", ringCount);
}