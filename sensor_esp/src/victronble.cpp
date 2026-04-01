#include "victronble.h"
#include "bleconfig.h"
#include "logging.h"

// BLE nur wenn nicht S2 Mini (kein BLE)
#ifndef CONFIG_IDF_TARGET_ESP32S2

#include <VictronBLE.h>

static VictronBLE victron;
static bool s_initialized = false;

BmvData    bmvData    = {};
MpptData   mppt1Data  = {};
MpptData   mppt2Data  = {};
ChargerData chargerData = {};

// MAC normalisieren: "AA:BB:CC:DD:EE:FF" → "aabbccddeeff"
static void normalizeMac(const char* input, char* output) {
    int j = 0;
    for (int i = 0; input[i] && j < 12; i++) {
        if (input[i] == ':' || input[i] == '-') continue;
        output[j++] = tolower(input[i]);
    }
    output[j] = '\0';
}

static void onVictronData(const VictronDevice *dev)
{
    if (!dev->dataValid) return;
    logPrintf("BLE cb: mac=%s type=0x%02X\n", dev->mac, dev->deviceType);

    if (dev->deviceType == DEVICE_TYPE_BATTERY_MONITOR)
    {
        const VictronBatteryData &b = dev->battery;
        bmvData.valid           = true;
        bmvData.voltage         = b.voltage;
        bmvData.current         = b.current;
        bmvData.power           = b.voltage * b.current;
        bmvData.soc             = b.soc;
        bmvData.ttg             = b.remainingMinutes;
        bmvData.voltage_starter = b.auxVoltage;
        bmvData.consumed_ah     = b.consumedAh;
        bmvData.lastUpdateMs    = millis();
        //logPrintf("BMV712: %.2fV %.2fA %.1f%% TTG=%dmin\n",
          //  b.voltage, b.current, b.soc, b.remainingMinutes);
    }
    else if (dev->deviceType == DEVICE_TYPE_SOLAR_CHARGER)
    {
        const VictronSolarData &s = dev->solar;
        char macNorm[13] = {};

        // Charger-MAC prüfen
        if (strlen(bleConfig.charger_mac) > 0) {
            normalizeMac(bleConfig.charger_mac, macNorm);
            if (strcmp(dev->mac, macNorm) == 0) {
                chargerData.valid           = true;
                chargerData.battery_voltage = s.batteryVoltage;
                chargerData.battery_current = s.batteryCurrent;
                chargerData.input_power     = s.panelPower;
                chargerData.charged_today   = s.yieldToday;
                chargerData.charge_state    = s.chargeState;
                chargerData.lastUpdateMs    = millis();
                logPrintf("Charger: %.2fV %.2fA %dW geladen=%dWh\n",
                    s.batteryVoltage, s.batteryCurrent,
                    (int)s.panelPower, s.yieldToday);
                return;
            }
        }

        // anhand MAC unterscheiden welcher MPPT
        normalizeMac(bleConfig.mppt1_mac, macNorm);
        bool isMppt1 = (strlen(bleConfig.mppt1_mac) > 0 &&
                strcmp(dev->mac, macNorm) == 0);
        MpptData &mppt = isMppt1 ? mppt1Data : mppt2Data;
        mppt.valid           = true;
        mppt.battery_voltage = s.batteryVoltage;
        mppt.battery_current = s.batteryCurrent;
        mppt.panel_power     = s.panelPower;
        mppt.yield_today     = s.yieldToday;
        mppt.charge_state    = s.chargeState;
        mppt.lastUpdateMs    = millis();
        logPrintf("MPPT%d: %.2fV %.2fA %dW Ertrag=%dWh\n",
            isMppt1 ? 1 : 2,
            s.batteryVoltage, s.batteryCurrent,
            (int)s.panelPower, s.yieldToday);
    }
}

void victronBleSetup()
{
    if (strlen(bleConfig.bmv_mac) == 0 &&
        strlen(bleConfig.mppt1_mac) == 0 &&
        strlen(bleConfig.mppt2_mac) == 0 &&
        strlen(bleConfig.charger_mac) == 0)
    {
        logPrintln("VictronBLE: Keine Geräte konfiguriert");
        return;
    }
    victron.setDebug(true);
    if (!victron.begin(5))
    {
        logPrintln("VictronBLE: BLE init FEHLER");
        return;
    }

    victron.setCallback(onVictronData);
    victron.setMinInterval(2000);

    if (strlen(bleConfig.bmv_mac) > 0)
        victron.addDevice("BMV712", bleConfig.bmv_mac, bleConfig.bmv_bindkey,
                          DEVICE_TYPE_BATTERY_MONITOR);

    if (strlen(bleConfig.mppt1_mac) > 0)
        victron.addDevice("MPPT1", bleConfig.mppt1_mac, bleConfig.mppt1_bindkey,
                          DEVICE_TYPE_SOLAR_CHARGER);

    if (strlen(bleConfig.mppt2_mac) > 0)
        victron.addDevice("MPPT2", bleConfig.mppt2_mac, bleConfig.mppt2_bindkey,
                          DEVICE_TYPE_SOLAR_CHARGER);

    if (strlen(bleConfig.charger_mac) > 0)
        victron.addDevice("Charger", bleConfig.charger_mac, bleConfig.charger_bindkey,
                          DEVICE_TYPE_SOLAR_CHARGER);

    s_initialized = true;
    logPrintln("VictronBLE: gestartet");
}

void victronBleLoop()
{
    if (!s_initialized) return;

    // Daten als ungültig markieren wenn älter als 30s
    if (bmvData.valid && millis() - bmvData.lastUpdateMs > 30000)
    {
        bmvData.valid = false;
        logPrintln("BMV712: Timeout");
    }
    if (mppt1Data.valid && millis() - mppt1Data.lastUpdateMs > 30000)
        mppt1Data.valid = false;
    if (mppt2Data.valid && millis() - mppt2Data.lastUpdateMs > 30000)
        mppt2Data.valid = false;
    if (chargerData.valid && millis() - chargerData.lastUpdateMs > 30000)
        chargerData.valid = false;

    victron.loop();
}

#else
// S2 Mini – kein BLE
BmvData     bmvData     = {};
MpptData    mppt1Data   = {};
MpptData    mppt2Data   = {};
ChargerData chargerData = {};
void victronBleSetup() { logPrintln("VictronBLE: S2 Mini hat kein BLE"); }
void victronBleLoop()  {}
String chargerToJson() { JsonDocument d; d["valid"]=false; String o; serializeJson(d,o); return o; }
#endif

// ---- JSON Ausgabe ----------------------------------------

String bmvToJson()
{
    JsonDocument doc;
    doc["valid"] = bmvData.valid;
    if (bmvData.valid)
    {
        doc["V"]   = serialized(String(bmvData.voltage,         2));
        doc["I"]   = serialized(String(bmvData.current,         2));
        doc["P"]   = serialized(String(bmvData.power,           1));
        doc["SOC"] = serialized(String(bmvData.soc,             1));
        doc["TTG"] = bmvData.ttg;
        doc["VS"]  = serialized(String(bmvData.voltage_starter, 2));
        doc["AH"]  = serialized(String(bmvData.consumed_ah,     1));
    }
    String out;
    serializeJson(doc, out);
    return out;
}

String mpptToJson(const MpptData &m)
{
    JsonDocument doc;
    doc["valid"] = m.valid;
    if (m.valid)
    {
        doc["V"]     = serialized(String(m.battery_voltage, 2));
        doc["I"]     = serialized(String(m.battery_current, 2));
        doc["PV"]    = serialized(String(m.panel_power,     1));
        doc["yield"] = m.yield_today;
        doc["state"] = m.charge_state;
         // Zusätzlich als Text:
        const char* stateStr = "Unbekannt";
        switch (m.charge_state) {
            case 0:   stateStr = "Aus"; break;
            case 1:   stateStr = "Niedrige Leistung"; break;
            case 2:   stateStr = "Fehler"; break;
            case 3:   stateStr = "Bulk"; break;
            case 4:   stateStr = "Absorption"; break;
            case 5:   stateStr = "Float"; break;
            case 6:   stateStr = "Speicher"; break;
            case 7:   stateStr = "Ausgleich"; break;
            case 9:   stateStr = "Invertierung"; break;
            case 11:  stateStr = "Netzteil"; break;
            case 252: stateStr = "Externe Regelung"; break;
        }
        doc["stateStr"] = stateStr;
    }
    String out;
    serializeJson(doc, out);
    return out;
}

String mppt1ToJson() { return mpptToJson(mppt1Data); }
String mppt2ToJson() { return mpptToJson(mppt2Data); }

String chargerToJson()
{
    JsonDocument doc;
    doc["valid"] = chargerData.valid;
    if (chargerData.valid)
    {
        doc["V"]     = serialized(String(chargerData.battery_voltage, 2));
        doc["I"]     = serialized(String(chargerData.battery_current, 2));
        doc["P"]     = serialized(String(chargerData.input_power,     1));
        doc["Wh"]    = chargerData.charged_today;
        doc["state"] = chargerData.charge_state;
        const char* stateStr = "Unbekannt";
        switch (chargerData.charge_state) {
            case 0:   stateStr = "Aus"; break;
            case 2:   stateStr = "Fehler"; break;
            case 3:   stateStr = "Bulk"; break;
            case 4:   stateStr = "Absorption"; break;
            case 5:   stateStr = "Float"; break;
            case 6:   stateStr = "Speicher"; break;
            case 7:   stateStr = "Ausgleich"; break;
            case 11:  stateStr = "Netzteil"; break;
            case 252: stateStr = "Externe Regelung"; break;
        }
        doc["stateStr"] = stateStr;
    }
    String out;
    serializeJson(doc, out);
    return out;
}