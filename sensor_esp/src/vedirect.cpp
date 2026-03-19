#include "vedirect.h"
#include "sensorconfig.h"

#include <ArduinoJson.h>

// ----------------------------------------------------------------
// Globale Daten
// ----------------------------------------------------------------
BmvData bmvData = {0, 0, 0, 0, -1, 0, false, 0};

// ----------------------------------------------------------------
// Interner Parser-State
// ----------------------------------------------------------------
namespace
{
    // Zustände des Zeilenparsers
    enum ParseState { LABEL, VALUE, CHECKSUM };

    ParseState  state       = LABEL;
    char        label[32]   = {};
    char        value[32]   = {};
    uint8_t     labelIdx    = 0;
    uint8_t     valueIdx    = 0;
    uint8_t     checksum    = 0;    // laufende Summe über alle Bytes des Blocks
    bool        blockStart  = false;

    // Zwischenpuffer für einen Block
    // (werden erst bei gültigem Checksum nach bmvData übertragen)
    float   _voltage        = 0;
    float   _current        = 0;
    float   _voltageStarter = 0;
    float   _soc            = 0;
    int32_t _timeToGo       = -1;
    bool    _hasV           = false;
    bool    _hasI           = false;
    bool    _hasVS          = false;
    bool    _hasSOC         = false;
    bool    _hasTTG         = false;

    // ----------------------------------------------------------------
    // Ein vollständiges Key-Value-Paar verarbeiten
    // ----------------------------------------------------------------
    void processField(const char *lbl, const char *val)
    {
        if (strcmp(lbl, "V") == 0)
        {
            _voltage = atol(val) / 1000.0f;   // mV → V
            _hasV = true;
        }
        else if (strcmp(lbl, "I") == 0)
        {
            _current = atol(val) / 1000.0f;   // mA → A
            _hasI = true;
        }
        else if (strcmp(lbl, "VS") == 0)
        {
            _voltageStarter = atol(val) / 1000.0f; // mV → V
            _hasVS = true;
        }
        else if (strcmp(lbl, "SOC") == 0)
        {
            _soc = atol(val) / 10.0f;          // promille → %
            _hasSOC = true;
        }
        else if (strcmp(lbl, "TTG") == 0)
        {
            _timeToGo = atol(val);             // Minuten, -1 = unbekannt
            _hasTTG = true;
        }
        // weitere Felder des BMV712 können hier ergänzt werden:
        // P (Leistung, W), CE (verbrauchte Ah), H1-H20 (Historiedaten) etc.
    }

    // ----------------------------------------------------------------
    // Block-Puffer zurücksetzen (nach gültigem oder ungültigem Block)
    // ----------------------------------------------------------------
    void resetBlock()
    {
        _hasV = _hasI = _hasVS = _hasSOC = _hasTTG = false;
        _voltage = _current = _voltageStarter = _soc = 0;
        _timeToGo = -1;
        checksum = 0;
    }

    // ----------------------------------------------------------------
    // Ein Byte des seriellen Datenstroms verarbeiten
    // ----------------------------------------------------------------
    void processByte(uint8_t c)
    {
        // Checksum über alle Bytes des Blocks bilden (außer dem \n nach Checksum)
        checksum += c;

        switch (state)
        {
        case LABEL:
            if (c == '\t')
            {
                label[labelIdx] = '\0';
                labelIdx = 0;
                // Sonderfall: "Checksum"-Feld → nächstes Byte ist der Prüfwert
                if (strcmp(label, "Checksum") == 0)
                    state = CHECKSUM;
                else
                    state = VALUE;
            }
            else if (c == '\n')
            {
                // Leerzeile → neuer Block beginnt
                labelIdx = 0;
                resetBlock();
            }
            else if (c != '\r')
            {
                if (labelIdx < sizeof(label) - 1)
                    label[labelIdx++] = (char)c;
            }
            break;

        case VALUE:
            if (c == '\r')
            {
                // ignorieren
            }
            else if (c == '\n')
            {
                value[valueIdx] = '\0';
                valueIdx = 0;
                processField(label, value);
                labelIdx = 0;
                state = LABEL;
            }
            else
            {
                if (valueIdx < sizeof(value) - 1)
                    value[valueIdx++] = (char)c;
            }
            break;

        case CHECKSUM:
            // checksum enthält jetzt die Summe inkl. des Checksum-Bytes selbst.
            // Wenn alles korrekt ist, muss (checksum & 0xFF) == 0 sein.
            if ((checksum & 0xFF) == 0)
            {
                // Gültiger Block → Daten übernehmen
                if (_hasV)   bmvData.voltage        = _voltage;
                if (_hasI)   bmvData.current        = _current;
                if (_hasVS)  bmvData.voltageStarter = _voltageStarter;
                if (_hasSOC) bmvData.soc            = _soc;
                if (_hasTTG) bmvData.timeToGo       = _timeToGo;

                bmvData.power       = bmvData.voltage * bmvData.current;
                bmvData.valid       = true;
                bmvData.lastUpdateMs = millis();
            }
            else
            {
                logPrintln("VE.Direct: Checksum-Fehler, Block verworfen");
            }
            resetBlock();
            labelIdx = 0;
            state = LABEL;
            break;
        }
    }

} // namespace

// ----------------------------------------------------------------
// Öffentliche API
// ----------------------------------------------------------------
void vedirectSetup()
{
    VEDIRECT_UART.begin(sensorConfig.vedirect_baud, SERIAL_8N1,
                        sensorConfig.vedirect_rx, VEDIRECT_TX_PIN);
    logPrintf("VE.Direct: UART gestartet, RX=GPIO%d, Baud=%d\n",
              sensorConfig.vedirect_rx, sensorConfig.vedirect_baud);
}

void vedirectLoop()
{
    while (VEDIRECT_UART.available())
        processByte((uint8_t)VEDIRECT_UART.read());
}

bool vedirectIsValid()
{
    if (!bmvData.valid) return false;
    // Daten gelten als veraltet wenn länger als VEDIRECT_TIMEOUT_MS kein Update
    return (millis() - bmvData.lastUpdateMs) < VEDIRECT_TIMEOUT_MS;
}

String vedirectToJson()
{
    JsonDocument doc;
    if (vedirectIsValid())
    {
        doc["valid"]   = true;
        doc["V"]       = serialized(String(bmvData.voltage,        3));
        doc["I"]       = serialized(String(bmvData.current,        3));
        doc["VS"]      = serialized(String(bmvData.voltageStarter, 3));
        doc["SOC"]     = serialized(String(bmvData.soc,            1));
        doc["TTG"]     = bmvData.timeToGo;
        doc["P"]       = serialized(String(bmvData.power,          1));
        doc["age_ms"]  = (millis() - bmvData.lastUpdateMs);
    }
    else
    {
        doc["valid"] = false;
    }
    String out;
    serializeJson(doc, out);
    return out;
}
