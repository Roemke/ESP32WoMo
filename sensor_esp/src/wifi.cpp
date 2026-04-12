#include "wifi.h"
#include <Preferences.h> //auch umgestellt

static const char* WIFI_NVS_NAMESPACE = "wificonfig";

WifiData wifiData;
String   wifiMode;
String   wifiMacAp;
String   wifiMacSta;

void wifiSaveData()
{
    Preferences prefs;
    prefs.begin(WIFI_NVS_NAMESPACE, false);
    prefs.putString("ssid",      wifiData.ssid);
    prefs.putString("password",  wifiData.password);
    prefs.putBool("use_static",  wifiData.use_static_ip);
    prefs.putString("static_ip", wifiData.static_ip);
    prefs.putString("subnet",    wifiData.subnet);
    prefs.putUInt("magic",       wifiData.magic);
    prefs.end();
    logPrintln("WiFi: Credentials gespeichert");
}

static bool loadWifiData()
{
    Preferences prefs;
    prefs.begin(WIFI_NVS_NAMESPACE, true);

    if (!prefs.isKey("magic")) {
        prefs.end();
        logPrintln("WiFi: Keine gespeicherten Credentials");
        return false;
    }

    uint32_t magic = prefs.getUInt("magic", 0);
    if (magic != 0x43) {
        prefs.end();
        logPrintln("WiFi: Credentials ungültig");
        return false;
    }

    prefs.getString("ssid",      wifiData.ssid,      sizeof(wifiData.ssid));
    prefs.getString("password",  wifiData.password,  sizeof(wifiData.password));
    wifiData.use_static_ip = prefs.getBool("use_static", false);
    prefs.getString("static_ip", wifiData.static_ip, sizeof(wifiData.static_ip));
    prefs.getString("subnet",    wifiData.subnet,     sizeof(wifiData.subnet));
    wifiData.magic = magic;
    prefs.end();

    if (wifiData.use_static_ip)
        logPrintln("WiFi: Statische IP aktiviert");

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
    WiFi.setSleep(true);
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
    WiFi.setSleep(true);

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
/*
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
    */
void wifiSetup()
{
    // Direkt starten ohne Reset-Zyklus (BLE hält bereits den RF-Controller)
    if (loadWifiData() && connectSTA())
    {
        wifiMacAp  = WiFi.softAPmacAddress();
        wifiMacSta = WiFi.macAddress();
        logPrintf("WiFi: MAC STA=%s  AP=%s\n", wifiMacSta.c_str(), wifiMacAp.c_str());
        return;
    }

    startAP();
    wifiMacAp  = WiFi.softAPmacAddress();
    wifiMacSta = WiFi.macAddress();
    logPrintf("WiFi: MAC STA=%s  AP=%s\n", wifiMacSta.c_str(), wifiMacAp.c_str());
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
