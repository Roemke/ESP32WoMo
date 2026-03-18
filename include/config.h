#pragma once

// ============================================================
// config.h  –  zentrale Konfiguration ESP32WoMo
// Alle Pins, Intervalle und Einstellungen an einem Ort.
// ============================================================

// ---- VE.Direct (Victron BMV712) ----------------------------
// Nur RX wird benötigt (wir lesen nur)
// ACHTUNG: Spannung am VE.Direct TX messen!
//   3.3V → direkt anschließen
//   5V   → Spannungsteiler oder Level-Shifter nötig
#define VEDIRECT_RX_PIN     17      // ESP32-S3 GPIO → VE.Direct TX
#define VEDIRECT_TX_PIN     -1      // nicht angeschlossen (nur lesen)
#define VEDIRECT_BAUD       19200
#define VEDIRECT_UART       Serial1 // Serial1 des ESP32-S3

// ---- BME280 (I²C) ------------------------------------------
// Der ESP32-S3 hat I²C auf verschiedenen Pins möglich.
// GT911 Touch nutzt bereits I2C_NUM_0 auf GPIO 19/20 (laut Board-JSON)
// → BME280 auf I2C_NUM_1 oder andere Pins legen, nein, andere adresse, also auf 19,20
// aber bme erst nach dem display initialisieren, nein muss auf anderen Controlle, Wire1
// mal sehen, ob dann 19 und 20 als pins gehen 

#define BME280_SDA_PIN      19       // freier I²C Bus
#define BME280_SCL_PIN      20
#define BME280_I2C_ADDR     0x76    // 0x76 oder 0x77 je nach Modul

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
#define WLED_DEFAULT_INNEN_IP   "192.168.4.2"
#define WLED_DEFAULT_AUSSEN_IP  "192.168.4.3"
#define WLED_DEFAULT_PORT       80

// ---- WiFi --------------------------------------------------
// Credentials werden in LittleFS gespeichert (wifiData.json)
// AP-Name falls kein WLAN bekannt
#define WIFI_AP_SSID        "ESP32WoMo"
#define WIFI_AP_PASSWORD    ""          // leer = offenes Netz
#define WIFI_CONNECT_TIMEOUT_MS 10000

// ---- Webserver ---------------------------------------------
#define HTTP_PORT           80

// ---- Sensor-Abfrageintervalle ------------------------------
#define BME280_INTERVAL_MS  10000   // alle 10 Sekunden
#define VEDIRECT_TIMEOUT_MS 5000    // Timeout wenn keine Daten kommen

// ---- Logging -----------------------------------------------
#define LOG_BUFFER_SIZE     50
#define LOG_LINE_LENGTH     128
