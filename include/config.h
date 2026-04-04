#pragma once


// ---- SD-Karte (SPI) ----------------------------------------
// Pins kommen aus der Board-JSON (esp32-8048S043C):
// TF_CS=10, TF_SPI_MOSI=11, TF_SPI_SCLK=12, TF_SPI_MISO=13
// → keine Neudefinition nötig, zur Dokumentation hier:
#define SD_CS_PIN           10
#define SD_MOSI_PIN         11
#define SD_SCLK_PIN         12
#define SD_MISO_PIN         13
#define SD_LOG_INTERVAL_MS  60000   // alle 60 Sekunden auf SD schreiben

// ---- WLED --------------------------------------------------
// Konfiguration wird in LittleFS als /wled.json gespeichert.
// Diese Defaults gelten nur beim allerersten Start (Datei nicht vorhanden).
// Struktur der wled.json:
// {
//   "innen":  { "ip": "192.168.x.x", "port": 80 },
//   "aussen": { "ip": "192.168.x.x", "port": 80 }
// }
// WLED JSON-API: POST /json/state  mit {"on":true} oder {"on":false}
#define WLED_CONFIG_PATH        "/wled.json"
#define WLED_DEFAULT_INNEN_IP   "192.168.4.6"
#define WLED_DEFAULT_AUSSEN_IP  "192.168.4.7"
#define WLED_DEFAULT_PORT       80

// ---- WiFi --------------------------------------------------
// Credentials werden in LittleFS gespeichert (wifiData.json)
// AP-Name falls kein WLAN bekannt
#define WIFI_AP_SSID        "ESP32WoMo"
#define WIFI_AP_PASSWORD    ""          // leer = offenes Netz
#define WIFI_CONNECT_TIMEOUT_MS 10000
#define WIFI_USE_STATIC_IP_DEFAULT  false
#define WIFI_STATIC_IP_DEFAULT      "192.168.42.4"
#define WIFI_SUBNET_DEFAULT         "255.255.255.0"
#define WIFI_GATEWAY_DEFAULT    "192.168.42.1"
#define WIFI_DNS_DEFAULT        "192.168.42.1"

// ---- Webserver ---------------------------------------------
#define HTTP_PORT           80

// ---- Sensor-ESP --------------------------------------------
#define SENSOR_ESP_IP_DEFAULT   "192.168.42.3"
#define SENSOR_POLL_INTERVAL_MS  2000 //kleiner nicht sinnvoll, sensor liest bei 1000 in der voreinstellung
// ---- Logging -----------------------------------------------
#define LOG_BUFFER_SIZE     50
#define LOG_LINE_LENGTH     128

#define BMV_MAC_DEFAULT     ""
#define BMV_BINDKEY_DEFAULT ""