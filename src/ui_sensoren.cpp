#include "ui_sensoren.h"
#include "sensorpoll.h"
#include "SD.h"

// ----------------------------------------------------------------
// Private UI-Handles – nur in dieser Datei sichtbar
// ----------------------------------------------------------------
static lv_obj_t *s_volt    = nullptr;
static lv_obj_t *s_current = nullptr;
static lv_obj_t *s_power   = nullptr;
static lv_obj_t *s_soc     = nullptr;
static lv_obj_t *s_ttg     = nullptr;
static lv_obj_t *s_vs      = nullptr;

static lv_obj_t *s_temp    = nullptr;
static lv_obj_t *s_hum     = nullptr;
static lv_obj_t *s_press   = nullptr;
static lv_obj_t *s_co2     = nullptr;
static lv_obj_t *s_ip      = nullptr;


//hilfsfunktion zum laden von bildern in den psram, damit lvgl die bilder direkt von dort laden kann. spart ram und ist schneller als sd lesen.

static uint8_t  *s_imgLeft      = nullptr;
static uint8_t  *s_imgRight     = nullptr;
static uint32_t  s_imgLeftSize  = 0;
static uint32_t  s_imgRightSize = 0;

LV_IMAGE_DECLARE(earthSmall);
LV_IMAGE_DECLARE(tardisSmall);

// ----------------------------------------------------------------
// Hilfsfunktion: Panel erstellen
// ----------------------------------------------------------------
static lv_obj_t *makePanel(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_set_size(p, w, h);
    lv_obj_set_pos(p, x, y);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x0A0A1A), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(p, lv_color_hex(0x3A3A8E), 0);
    lv_obj_set_style_border_width(p, 1, 0);
    lv_obj_set_style_radius(p, 10, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    return p;
}

// ----------------------------------------------------------------
// Hilfsfunktion: Titel im Panel
// ----------------------------------------------------------------
static void makeTitle(lv_obj_t *parent, const char *text)
{
    lv_obj_t *t = lv_label_create(parent);
    lv_label_set_text(t, text);
    lv_obj_set_style_text_color(t, lv_color_hex(0xAAAAFF), 0);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_18, 0);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 8);
}


// ----------------------------------------------------------------
// Hilfsfunktion: Wert-Zeile – gibt Value-Label zurück
// ----------------------------------------------------------------
static lv_obj_t *makeRow(lv_obj_t *parent, const char *key, int y)
{
    lv_obj_t *k = lv_label_create(parent);
    lv_label_set_text(k, key);
    lv_obj_set_style_text_color(k, lv_color_hex(0x8888BB), 0);
    lv_obj_set_style_text_font(k, &lv_font_montserrat_18, 0);
    lv_obj_align(k, LV_ALIGN_TOP_LEFT, 12, y);

    lv_obj_t *v = lv_label_create(parent);
    lv_label_set_text(v, "---");
    lv_obj_set_style_text_color(v, lv_color_hex(0xEEEEFF), 0);
    lv_obj_set_style_text_font(v, &lv_font_montserrat_18, 0);
    lv_obj_align(v, LV_ALIGN_TOP_RIGHT, -12, y - 2);
    return v;
}

// ================================================================
// Setup
// ================================================================
void uiSensorenSetup(lv_obj_t *tab)
{
    // Tab-Hintergrund
    //bilder in buffer 
    
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(tab, 4, 0);
    //lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    // Nutzbare Fläche: 800 × 440px (480px - 40px Tabs)
    // Links: Batterie 388×432, Rechts: Klima 388×432, Abstand 8px
    // Abstaende Zeilen
    const int row_start = 34;
    const int row_step  = 62;
    // ---- Klima-Panel (links) ------------------------------------
    lv_obj_t *p_klima = makePanel(tab, 0, 0, 388, 392);

    // Bild direkt als Objekt ins Panel, soll im psram liegen
    lv_obj_t *imgLeft = lv_image_create(p_klima);
    //lv_image_set_src(imgLeft,  "S:/earthSmall.bin");
    lv_image_set_src(imgLeft,  &earthSmall);
    lv_obj_set_pos(imgLeft, 50, 190);
    lv_obj_move_to_index(imgLeft, 0);  // ganz nach unten in der Z-Order

    makeTitle(p_klima, "Klima");

    s_temp  = makeRow(p_klima, "Temperatur:", row_start);
    s_hum   = makeRow(p_klima, "Feuchte:",    row_start + row_step);
    s_press = makeRow(p_klima, "Luftdruck:",  row_start + row_step * 2);
    s_co2   = makeRow(p_klima, "CO2:",        row_start + row_step * 3);

    // IP-Adresse unten rechts
    s_ip = lv_label_create(p_klima);
    lv_label_set_text(s_ip, "---");
    lv_obj_set_style_text_color(s_ip, lv_color_hex(0x666688), 0);
    lv_obj_set_style_text_font(s_ip, &lv_font_montserrat_16, 0);
    lv_obj_align(s_ip, LV_ALIGN_BOTTOM_RIGHT, -10, -8);

    // ---- Batterie-Panel (rechts) ----------------------------------
    lv_obj_t *p_bat = makePanel(tab, 404, 0, 388, 392);
    makeTitle(p_bat, "Batterie (BMV712)");
    // Bild direkt als Objekt ins Panel
    lv_obj_t *imgRight = lv_image_create(p_bat);
    //lv_image_set_src(imgRight,  "S:/tardisSmall.bin");
    lv_image_set_src(imgRight,  &tardisSmall);
    lv_obj_set_pos(imgRight, 150, 100);
    lv_obj_move_to_index(imgRight, 0);  // ganz nach unten in der Z-Order

    

    s_volt    = makeRow(p_bat, "Spannung:",  row_start);
    s_current = makeRow(p_bat, "Strom:",     row_start + row_step);
    s_power   = makeRow(p_bat, "Leistung:",  row_start + row_step * 2);
    s_soc     = makeRow(p_bat, "SoC:",       row_start + row_step * 3);
    s_ttg     = makeRow(p_bat, "Restlauf:",  row_start + row_step * 4);
    s_vs      = makeRow(p_bat, "Starter:",   row_start + row_step * 5);

}

// ================================================================
// Update – aus loop() aufrufen
// ================================================================
void uiSensorenUpdate()
{
    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate < 2000) return;
    lastUpdate = millis();
    char buf[32];

    // ---- Batterie -----------------------------------------------
    if (sensorData.vedirect_valid)
    {
        snprintf(buf, sizeof(buf), "%.2f V", sensorData.voltage);
        lv_label_set_text(s_volt, buf);

        snprintf(buf, sizeof(buf), "%.2f A", sensorData.current);
        lv_label_set_text(s_current, buf);

        snprintf(buf, sizeof(buf), "%.1f W", sensorData.power);
        lv_label_set_text(s_power, buf);

        snprintf(buf, sizeof(buf), "%.1f %%", sensorData.soc);
        lv_label_set_text(s_soc, buf);

        if (sensorData.ttg < 0)
            lv_label_set_text(s_ttg, "---");
        else
        {
            snprintf(buf, sizeof(buf), "%dh %dm",
                     sensorData.ttg / 60, sensorData.ttg % 60);
            lv_label_set_text(s_ttg, buf);
        }

        snprintf(buf, sizeof(buf), "%.2f V", sensorData.voltage_starter);
        lv_label_set_text(s_vs, buf);
    }
    else
    {
        lv_label_set_text(s_volt,    "---");
        lv_label_set_text(s_current, "---");
        lv_label_set_text(s_power,   "---");
        lv_label_set_text(s_soc,     "---");
        lv_label_set_text(s_ttg,     "---");
        lv_label_set_text(s_vs,      "---");
    }

    // ---- Klima --------------------------------------------------
    if (sensorData.bme_valid)
    {
        snprintf(buf, sizeof(buf), "%.1f C", sensorData.temperature);
        lv_label_set_text(s_temp, buf);

        snprintf(buf, sizeof(buf), "%.1f %%", sensorData.humidity);
        lv_label_set_text(s_hum, buf);

        snprintf(buf, sizeof(buf), "%.1f hPa", sensorData.pressure);
        lv_label_set_text(s_press, buf);
    }
    else
    {
        lv_label_set_text(s_temp,  "---");
        lv_label_set_text(s_hum,   "---");
        lv_label_set_text(s_press, "---");
    }

    // ---- CO2 ----------------------------------------------------
    if (sensorData.co2_valid)
    {
        snprintf(buf, sizeof(buf), "%d ppm", sensorData.co2_ppm);
        lv_label_set_text(s_co2, buf);
    }
    else
        lv_label_set_text(s_co2, "---");

}

// ----------------------------------------------------------------
// IP-Adresse setzen – aus main.cpp aufrufen wenn WiFi bereit
// ----------------------------------------------------------------
void uiSensorenSetIP(const String &ip)
{
    if (s_ip) lv_label_set_text(s_ip, ip.c_str());
}