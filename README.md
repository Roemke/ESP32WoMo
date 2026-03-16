# ESP32 WoMo

Wohnmobil-Steuerung auf Basis eines **Sunton ESP32-S3 4,3" 800×480 IPS Display-Boards**.

## Hardware

| Komponente | Details |
|---|---|
| Board | Sunton ESP32-8048S043C (ESP32-S3, 8MB PSRAM, 16MB Flash) |
| Display | 4,3" IPS RGB 800×480, kapazitiver Touch (GT911) |
| Batteriemonitor | Victron BMV712 via VE.Direct (UART, 19200 Baud) |
| Klimasensor | BME280 (I²C) |
| Beleuchtung | 2× WLED-ESP (innen + außen) |
| Speicher | SD-Karte (SPI, onboard Slot) |

## Projektstruktur

```
ESP32WoMo/
├── boards/                        ← Git-Submodul: Sunton Board-Definitionen
├── include/
│   ├── config.h                   ← Zentrale Konfiguration (Pins, Intervalle)
│   ├── logging.h
│   ├── wifi.h
│   ├── vedirect.h
│   ├── bme280sensor.h
│   ├── sdcard.h
│   ├── wled.h
│   └── lv_conf.h                  ← LVGL-Konfiguration (aus Template erzeugt)
├── src/
│   ├── main.cpp                   ← Aktives Hauptprogramm
│   ├── main.cpp-mini              ← Minimales Testprogramm (nur Display)
│   ├── main.cpp-DemoLVGL          ← LVGL Demo (3-Spalten UI)
│   ├── logging.cpp
│   ├── wifi.cpp
│   ├── vedirect.cpp
│   ├── bme280sensor.cpp
│   ├── sdcard.cpp
│   └── wled.cpp
└── platformio.ini
```

## Einrichtung

### 1. Board-Definitionen (einmalig)

```bash
git init
git submodule add https://github.com/rzeldent/platformio-espressif32-sunton.git boards
```

Bei vorhandenem Repo klonen:
```bash
git clone --recurse-submodules <repo-url>
```

### 2. LVGL Konfiguration

Nach dem ersten `pio pkg install` das Template kopieren und aktivieren:

```bash
cp .pio/libdeps/esp32-8048S043C/lvgl/lv_conf_template.h include/lv_conf.h
sed -i 's/#if 0/#if 1/' include/lv_conf.h
```

Dann in `include/lv_conf.h` sicherstellen:
```c
#define LV_COLOR_DEPTH 16
// Alle benötigten Montserrat-Fonts auf 1 setzen:
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
```

### 3. `platformio.ini` prüfen

```ini
[platformio]
default_envs = esp32-8048S043C
boards_dir   = boards

build_flags =
    -D LV_CONF_INCLUDE_SIMPLE
    -I include
```

### 4. Bauen & Flashen

In VSCode mit PlatformIO-Extension: **Build** dann **Upload**.

> Falls Upload hängt: BOOT-Taste gedrückt halten beim Upload-Start.

## Konfiguration zur Laufzeit

Alle Einstellungen werden in LittleFS gespeichert und überleben einen Neustart.

### WiFi konfigurieren

```bash
curl -X POST http://<ip>/api/config/wifi \
  -H "Content-Type: application/json" \
  -d '{"ssid":"MeinWLAN","password":"MeinPasswort"}'
```

Der ESP startet nach dem Speichern automatisch neu.
Ohne gespeicherte Credentials startet er als Access Point **ESP32WoMo**.

### WLED IPs konfigurieren

```bash
curl -X POST http://<ip>/api/config/wled \
  -H "Content-Type: application/json" \
  -d '{"innen":{"ip":"192.168.1.10","port":80},"aussen":{"ip":"192.168.1.11","port":80}}'
```

Gespeichert in `/wled.json` auf LittleFS.

## REST-API Übersicht

| Methode | Endpunkt | Beschreibung |
|---|---|---|
| GET | `/api/data` | Alle Sensordaten als JSON |
| POST | `/api/wled/innen/on` | Innenbeleuchtung einschalten |
| POST | `/api/wled/innen/off` | Innenbeleuchtung ausschalten |
| POST | `/api/wled/aussen/on` | Außenbeleuchtung einschalten |
| POST | `/api/wled/aussen/off` | Außenbeleuchtung ausschalten |
| POST | `/api/config/wled` | WLED IPs konfigurieren |
| POST | `/api/config/wifi` | WiFi-Credentials setzen |
| POST | `/api/reboot` | ESP32 neu starten |
| GET/POST | `/update` | OTA-Update (ElegantOTA) |

### Beispiel `/api/data` Antwort

```json
{
  "batterie": {
    "valid": true,
    "V": 12.845,
    "I": -2.310,
    "VS": 12.701,
    "SOC": 87.3,
    "TTG": 1240,
    "P": -29.7
  },
  "klima": {
    "valid": true,
    "T": 22.4,
    "H": 58.1,
    "P": 1013.2
  },
  "wled": {
    "innen":  {"ip":"192.168.1.10","port":80,"state":true},
    "aussen": {"ip":"192.168.1.11","port":80,"state":false}
  },
  "sd":   {"ready":true,"size_mb":32,"used_mb":1,"free_mb":31},
  "wifi": "192.168.1.42"
}
```

## Hardware-Anschluss

### VE.Direct (BMV712)

> ⚠️ **Vor dem Anschluss Spannung messen!**
> Zwischen GND und TX am VE.Direct-Stecker messen.
> Bei 5V einen Spannungsteiler (10kΩ / 20kΩ) verwenden –
> der ESP32-S3 verträgt nur 3,3V an den GPIO-Pins.

| VE.Direct | ESP32-S3 |
|---|---|
| GND | GND |
| TX | GPIO 17 (RX) |
| RX | nicht angeschlossen |
| VCC | nicht verwenden (max. 10mA) |

Stecker: JST-PH 2,0mm 4-polig

### BME280 (I²C)

| BME280 | ESP32-S3 |
|---|---|
| GND | GND |
| VCC | 3,3V |
| SDA | GPIO 8 |
| SCL | GPIO 9 |

> Adresse: 0x76 (Standard) oder 0x77 (SDO an VCC).
> Der Code probiert automatisch beide Adressen.

### SD-Karte

Onboard-Slot, keine externe Verkabelung nötig.
Pins laut Board-Definition: CS=10, MOSI=11, SCLK=12, MISO=13.

### WLED

Verbindung über WLAN, keine direkte Verkabelung.
IPs über `/api/config/wled` konfigurierbar.

## SD-Karte Datenformat

Messwerte werden alle 60 Sekunden als CSV gespeichert:

```
/data/day_00001.csv
uptime_ms,V,I,VS,SOC,TTG,P_W,T_C,H_pct,P_hPa
60123,12.845,-2.310,12.701,87.3,1240,-29.7,22.4,58.1,1013.2
```

Eine neue Datei pro Laufzeittag. Sobald NTP verfügbar ist,
kann der Dateiname auf ein echtes Datum umgestellt werden.

## Bekannte Einschränkungen / TODO

- **Kein RTC / NTP**: SD-Dateinamen basieren auf Laufzeit, nicht auf Uhrzeit
- **VE.Direct Spannung**: Vor dem Anschluss messen (3,3V oder 5V)
- **WLED Polling blockiert bei Timeout**: Läuft in eigenem FreeRTOS-Task
  auf Core 0, beeinflusst Display auf Core 1 nicht
- **WiFi-Credentials im AP-Modus**: Über curl oder eigene Web-UI setzen

## Bibliotheken

| Bibliothek | Zweck |
|---|---|
| rzeldent/esp32-smartdisplay | Display + Touch Treiber |
| lvgl/lvgl | UI Framework |
| ESP32Async/ESPAsyncWebServer | Async Webserver |
| ESP32Async/AsyncTCP | TCP für Async Webserver |
| ayushsharma82/ElegantOTA | OTA Updates |
| bblanchon/ArduinoJson | JSON |
| adafruit/Adafruit BME280 | BME280 Sensor |
