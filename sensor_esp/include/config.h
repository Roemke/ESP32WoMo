#pragma once

// ============================================================
// config.h – Sensor-ESP (ESP32-S2 Mini)
// Defaults – werden beim ersten Start in LittleFS gespeichert
// und können danach per REST-API geändert werden.
// ============================================================

// ---- BME280 (I²C) ------------------------------------------
#ifdef ARDUINO_LOLIN_S2_MINI
    #define BME280_SDA_PIN_DEFAULT  33
    #define BME280_SCL_PIN_DEFAULT  35
#else
    // DevKitC und andere ESP32-S3
    #define BME280_SDA_PIN_DEFAULT  8
    #define BME280_SCL_PIN_DEFAULT  9
#endif
#define BME280_I2C_ADDR_DEFAULT     0x76
#define BME280_INTERVAL_MS_DEFAULT  10000

// ---- VE.Direct (Victron BMV712) ----------------------------
#define VEDIRECT_RX_PIN_DEFAULT  37 //wird uninteressant sein, da püber blootooth mal sehen 
#define VEDIRECT_TX_PIN          -1
#define VEDIRECT_BAUD            19200
#define VEDIRECT_UART            Serial1
#define VEDIRECT_TIMEOUT_MS      5000
#define VEDIRECT_TX_PIN  -1

// ---- WiFi --------------------------------------------------
#define WIFI_AP_SSID             "ESP32Sensor"
#define WIFI_AP_PASSWORD         ""
#define WIFI_CONNECT_TIMEOUT_MS  10000

#define WIFI_USE_STATIC_IP_DEFAULT  false
#define WIFI_STATIC_IP_DEFAULT      "192.168.1.100"
#define WIFI_SUBNET_DEFAULT         "255.255.255.0"

// ---- HTTP Server -------------------------------------------
#define HTTP_PORT                80

// ---- LittleFS Pfade ----------------------------------------
#define CONFIG_PATH              "/config.json"
#define WIFI_DATA_PATH           "/wifiData.json"
#define SENSOR_CONFIG_VERSION 2 //ändern bei strukturänderung

// ---- Logging -----------------------------------------------
#define LOG_BUFFER_SIZE          50
#define LOG_LINE_LENGTH          128

#define BLE_BMV_MAC_DEFAULT      ""
#define BLE_BMV_BINDKEY_DEFAULT  ""
#define BLE_MPPT1_MAC_DEFAULT    ""
#define BLE_MPPT1_BINDKEY_DEFAULT ""
#define BLE_MPPT2_MAC_DEFAULT    ""
#define BLE_MPPT2_BINDKEY_DEFAULT ""
#define BLE_CHARGER_MAC_DEFAULT      ""
#define BLE_CHARGER_BINDKEY_DEFAULT  ""