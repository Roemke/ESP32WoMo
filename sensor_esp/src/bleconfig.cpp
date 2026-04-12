#include "bleconfig.h"
#include "logging.h"
#include <Preferences.h>

static const char* BLE_NVS_NAMESPACE = "bleconfig";

BleConfig bleConfig;

bool bleConfigLoad()
{
    Preferences prefs;
    prefs.begin(BLE_NVS_NAMESPACE, true);

    if (!prefs.isKey("version")) {
        prefs.end();        
        logPrintln("BleConfig: Keine Konfiguration, nutze Defaults");
        return bleConfigSave();
    }

    uint8_t ver = prefs.getUChar("version", 0);
    if (ver != BLE_CONFIG_VERSION) {
        prefs.end();
        logPrintln("BleConfig: Version veraltet, schreibe Defaults");
        return bleConfigSave();
    }

    prefs.getString("bmv_mac",         bleConfig.bmv_mac,         sizeof(bleConfig.bmv_mac));
    prefs.getString("bmv_bindkey",     bleConfig.bmv_bindkey,     sizeof(bleConfig.bmv_bindkey));
    prefs.getString("mppt1_mac",       bleConfig.mppt1_mac,       sizeof(bleConfig.mppt1_mac));
    prefs.getString("mppt1_bindkey",   bleConfig.mppt1_bindkey,   sizeof(bleConfig.mppt1_bindkey));
    prefs.getString("mppt2_mac",       bleConfig.mppt2_mac,       sizeof(bleConfig.mppt2_mac));
    prefs.getString("mppt2_bindkey",   bleConfig.mppt2_bindkey,   sizeof(bleConfig.mppt2_bindkey));
    prefs.getString("charger_mac",     bleConfig.charger_mac,     sizeof(bleConfig.charger_mac));
    prefs.getString("charger_bindkey", bleConfig.charger_bindkey, sizeof(bleConfig.charger_bindkey));
    bleConfig.version = ver;
    prefs.end();

    logPrintf("BleConfig: geladen – BMV=%s MPPT1=%s MPPT2=%s Charger=%s\n",
              bleConfig.bmv_mac, bleConfig.mppt1_mac, bleConfig.mppt2_mac, bleConfig.charger_mac);
    return true;
}

bool bleConfigSave()
{
    Preferences prefs;
    prefs.begin(BLE_NVS_NAMESPACE, false);
    prefs.putUChar("version",        bleConfig.version);
    prefs.putString("bmv_mac",         bleConfig.bmv_mac);
    prefs.putString("bmv_bindkey",     bleConfig.bmv_bindkey);
    prefs.putString("mppt1_mac",       bleConfig.mppt1_mac);
    prefs.putString("mppt1_bindkey",   bleConfig.mppt1_bindkey);
    prefs.putString("mppt2_mac",       bleConfig.mppt2_mac);
    prefs.putString("mppt2_bindkey",   bleConfig.mppt2_bindkey);
    prefs.putString("charger_mac",     bleConfig.charger_mac);
    prefs.putString("charger_bindkey", bleConfig.charger_bindkey);
    prefs.end();
    logPrintln("BleConfig: gespeichert");
    return true;
}

String bleConfigToJson()
{
    JsonDocument doc;
    doc["bmv_mac"]         = bleConfig.bmv_mac;
    doc["bmv_bindkey"]     = bleConfig.bmv_bindkey;
    doc["mppt1_mac"]       = bleConfig.mppt1_mac;
    doc["mppt1_bindkey"]   = bleConfig.mppt1_bindkey;
    doc["mppt2_mac"]       = bleConfig.mppt2_mac;
    doc["mppt2_bindkey"]   = bleConfig.mppt2_bindkey;
    doc["charger_mac"]     = bleConfig.charger_mac;
    doc["charger_bindkey"] = bleConfig.charger_bindkey;
    String out;
    serializeJson(doc, out);
    return out;
}
