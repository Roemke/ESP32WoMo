#include "bleconfig.h"
#include "logging.h"

BleConfig bleConfig;

bool bleConfigLoad()
{
    File f = LittleFS.open(BLE_CONFIG_PATH, "r");
    if (!f)
    {
        logPrintln("BleConfig: Keine Konfiguration, nutze Defaults");
        return bleConfigSave();
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err)
    {
        logPrintln("BleConfig: Fehler beim Lesen, nutze Defaults");
        return false;
    }

    uint8_t ver = doc["version"] | 0;
    if (ver != BLE_CONFIG_VERSION)
    {
        logPrintln("BleConfig: Version veraltet, schreibe Defaults");
        return bleConfigSave();
    }

    bleConfig.version = BLE_CONFIG_VERSION;
    strlcpy(bleConfig.bmv_mac,        doc["bmv_mac"]        | BLE_BMV_MAC_DEFAULT,       sizeof(bleConfig.bmv_mac));
    strlcpy(bleConfig.bmv_bindkey,    doc["bmv_bindkey"]    | BLE_BMV_BINDKEY_DEFAULT,   sizeof(bleConfig.bmv_bindkey));
    strlcpy(bleConfig.mppt1_mac,      doc["mppt1_mac"]      | BLE_MPPT1_MAC_DEFAULT,     sizeof(bleConfig.mppt1_mac));
    strlcpy(bleConfig.mppt1_bindkey,  doc["mppt1_bindkey"]  | BLE_MPPT1_BINDKEY_DEFAULT, sizeof(bleConfig.mppt1_bindkey));
    strlcpy(bleConfig.mppt2_mac,      doc["mppt2_mac"]      | BLE_MPPT2_MAC_DEFAULT,     sizeof(bleConfig.mppt2_mac));
    strlcpy(bleConfig.mppt2_bindkey,  doc["mppt2_bindkey"]  | BLE_MPPT2_BINDKEY_DEFAULT, sizeof(bleConfig.mppt2_bindkey));

    logPrintf("BleConfig: geladen – BMV=%s MPPT1=%s MPPT2=%s\n",
        bleConfig.bmv_mac, bleConfig.mppt1_mac, bleConfig.mppt2_mac);
    return true;
}

bool bleConfigSave()
{
    File f = LittleFS.open(BLE_CONFIG_PATH, "w");
    if (!f)
    {
        logPrintln("BleConfig: Fehler beim Speichern");
        return false;
    }
    JsonDocument doc;
    doc["version"]      = BLE_CONFIG_VERSION;
    doc["bmv_mac"]      = bleConfig.bmv_mac;
    doc["bmv_bindkey"]  = bleConfig.bmv_bindkey;
    doc["mppt1_mac"]    = bleConfig.mppt1_mac;
    doc["mppt1_bindkey"]= bleConfig.mppt1_bindkey;
    doc["mppt2_mac"]    = bleConfig.mppt2_mac;
    doc["mppt2_bindkey"]= bleConfig.mppt2_bindkey;
    serializeJson(doc, f);
    f.close();
    logPrintln("BleConfig: gespeichert");
    return true;
}

String bleConfigToJson()
{
    JsonDocument doc;
    doc["bmv_mac"]      = bleConfig.bmv_mac;
    doc["bmv_bindkey"]  = bleConfig.bmv_bindkey;
    doc["mppt1_mac"]    = bleConfig.mppt1_mac;
    doc["mppt1_bindkey"]= bleConfig.mppt1_bindkey;
    doc["mppt2_mac"]    = bleConfig.mppt2_mac;
    doc["mppt2_bindkey"]= bleConfig.mppt2_bindkey;
    String out;
    serializeJson(doc, out);
    return out;
}