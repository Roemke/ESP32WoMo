#include "wifi.h"

#define WIFI_DATA_PATH "/wifiData.json"

WifiData wifiData;
String   wifiMode;
String   wifiMacAp;
String   wifiMacSta;

// ----------------------------------------------------------------
// Credentials in LittleFS speichern
// ----------------------------------------------------------------
void wifiSaveData()
{
    File f = LittleFS.open(WIFI_DATA_PATH, "w");
    if (!f)
    {
        logPrintln("WiFi: Fehler beim Speichern der Credentials");
        return;
    }
    JsonDocument doc;
    doc["ssid"]     = wifiData.ssid;
    doc["password"] = wifiData.password;
    doc["use_static_ip"] = wifiData.use_static_ip;
    doc["static_ip"]     = wifiData.static_ip;
    doc["subnet"]        = wifiData.subnet;
    doc["magic"]    = wifiData.magic;
    
    serializeJson(doc, f);
    f.close();

    logPrintln("WiFi: Credentials gespeichert");
}

// ----------------------------------------------------------------
// Credentials aus LittleFS laden
// ----------------------------------------------------------------
static bool loadWifiData()
{
    File f = LittleFS.open(WIFI_DATA_PATH, "r");
    if (!f)
    {
        logPrintln("WiFi: Keine gespeicherten Credentials");
        return false;
    }
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err)
    {
        logPrintln("WiFi: Fehler beim Lesen der Credentials");
        return false;
    }

    strncpy(wifiData.ssid,     doc["ssid"]     | "", sizeof(wifiData.ssid)     - 1);
    strncpy(wifiData.password, doc["password"] | "", sizeof(wifiData.password) - 1);
    wifiData.magic = doc["magic"] | 0;

    wifiData.use_static_ip = doc["use_static_ip"] | false;
    strncpy(wifiData.static_ip, doc["static_ip"] | WIFI_STATIC_IP_DEFAULT, sizeof(wifiData.static_ip) - 1);
    strncpy(wifiData.subnet,    doc["subnet"]     | WIFI_SUBNET_DEFAULT,    sizeof(wifiData.subnet)    - 1);
    
    if (wifiData.magic != 0x43)
    {
        logPrintln("WiFi: Credentials ungültig");
        return false;
    }

    logPrintf("WiFi: Credentials geladen, SSID=%s\n", wifiData.ssid);
    return true;
}

// ----------------------------------------------------------------
// Credentials setzen und speichern (von außen aufrufbar, z.B. REST)
// ----------------------------------------------------------------
void wifiSetCredentials(const char *ssid, const char *password)
{
    strncpy(wifiData.ssid,     ssid,     sizeof(wifiData.ssid)     - 1);
    strncpy(wifiData.password, password, sizeof(wifiData.password) - 1);
    wifiData.ssid[sizeof(wifiData.ssid)         - 1] = 0;
    wifiData.password[sizeof(wifiData.password) - 1] = 0;
    wifiData.magic = 0x43;
    wifiSaveData();
}

// ----------------------------------------------------------------
// STA-Modus: Verbindung herstellen
// ----------------------------------------------------------------
static bool connectSTA()
{
    logPrintf("WiFi: Verbinde mit '%s' ...\n", wifiData.ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiData.ssid, wifiData.password);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    if (wifiData.use_static_ip)
    {
        IPAddress ip, sn;
        ip.fromString(wifiData.static_ip);
        sn.fromString(wifiData.subnet);
        WiFi.config(ip, IPAddress(0,0,0,0), sn);
    }

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < WIFI_CONNECT_TIMEOUT_MS)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        logPrintf("WiFi: Verbunden, IP=%s\n", WiFi.localIP().toString().c_str());
        wifiMode = "STA, IP " + WiFi.localIP().toString();
        return true;
    }

    logPrintln("WiFi: Verbindung fehlgeschlagen");
    return false;
}

// ----------------------------------------------------------------
// AP-Modus starten
// ----------------------------------------------------------------
static void startAP()
{
    logPrintln("WiFi: Starte Access Point...");
    WiFi.mode(WIFI_AP);
    delay(500);
    WiFi.setSleep(false);

    bool ok;
    if (strlen(WIFI_AP_PASSWORD) > 0)
        ok = WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    else
        ok = WiFi.softAP(WIFI_AP_SSID); // offenes Netz

    delay(1000);
    logPrintf("WiFi: AP '%s' gestartet: %s, IP=%s\n",
              WIFI_AP_SSID,
              ok ? "ok" : "FEHLER",
              WiFi.softAPIP().toString().c_str());

    wifiMode = "AP '" + String(WIFI_AP_SSID) + "', IP " +
               WiFi.softAPIP().toString();
}

// ----------------------------------------------------------------
// Öffentlicher Setup-Einstiegspunkt
// ----------------------------------------------------------------
void wifiSetup()
{
    // MACs auslesen bevor WiFi zurückgesetzt wird
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    wifiMacAp  = WiFi.softAPmacAddress();
    wifiMacSta = WiFi.macAddress();

    // Kompletter Reset
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(500);

    logPrintf("WiFi: MAC STA=%s  AP=%s\n",
              wifiMacSta.c_str(), wifiMacAp.c_str());
    logPrintf("WiFi: Heap nach Reset: %lu\n",
              (unsigned long)ESP.getFreeHeap());

    // Credentials laden und STA versuchen, sonst AP
    if (loadWifiData() && connectSTA())
        return;

    startAP();
}

// ----------------------------------------------------------------
// Hilfsfunktionen
// ----------------------------------------------------------------
bool wifiIsConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

String wifiGetIP()
{
    return wifiIsConnected()
               ? WiFi.localIP().toString()
               : WiFi.softAPIP().toString();
}
