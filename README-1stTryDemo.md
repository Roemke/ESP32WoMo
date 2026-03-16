# ESP32-S3 4,3" LVGL Demo – PlatformIO

Board: **Sunton ESP32-8048S043** (800×480 IPS, ESP32-S3, 8MB PSRAM, 16MB Flash)

## Projektstruktur

```
esp32_lvgl_demo/
├── platformio.ini
├── boards/                  ← Git-Submodul (siehe Schritt 1!)
├── include/
│   └── lv_conf.h
└── src/
    └── main.cpp
```

## Einrichtung (einmalig, gemäß rzeldent README)

**Schritt 1 – Board-Definitionen als Git-Submodul einbinden**

Die Board-JSON-Dateien müssen im `boards/`-Unterordner liegen:

```bash
git init
git submodule add https://github.com/rzeldent/platformio-espressif32-sunton.git boards
```

Beim Klonen eines bestehenden Repos:
```bash
git clone --recurse-submodules <repo-url>
```

**Schritt 2 – Touch-Variante wählen** in `platformio.ini`:

| Env              | Touch-Typ                        |
|------------------|----------------------------------|
| esp32-8048S043C  | Kapazitiv GT911 (Glas-Touch)     |
| esp32-8048S043R  | Resistiv XPT2046 (Stift)         |
| esp32-8048S043N  | Kein Touch                       |

**Schritt 3 – Bauen & Flashen** über PlatformIO in VSCode.

> Falls Upload bei 0% hängt: BOOT-Taste gedrückt halten beim Upload-Start.

## Wichtig: Backlight

`smartdisplay_lcd_set_backlight(1.0f)` nach `smartdisplay_init()` ist
notwendig – ohne es bleibt das Display auf manchen Boards dunkel
(rzeldent Issue #131).

## LVGL-Loop (gemäß README)

```cpp
static auto lv_last_tick = millis();

void loop() {
    auto const now = millis();
    lv_tick_inc(now - lv_last_tick);
    lv_last_tick = now;
    lv_timer_handler();
    // eigene Logik hier
}
```
