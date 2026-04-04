#include "ui_charger.h"
#include "sensorpoll.h"

// ---- Widget-Handles ----
static lv_obj_t *s_ch_v     = nullptr;
static lv_obj_t *s_ch_i     = nullptr;
static lv_obj_t *s_ch_state = nullptr;

static lv_obj_t *s_m1_v     = nullptr;
static lv_obj_t *s_m1_i     = nullptr;
static lv_obj_t *s_m1_pv    = nullptr;
static lv_obj_t *s_m1_state = nullptr;
static lv_obj_t *s_m1_y     = nullptr;

static lv_obj_t *s_m2_v     = nullptr;
static lv_obj_t *s_m2_i     = nullptr;
static lv_obj_t *s_m2_pv    = nullptr;
static lv_obj_t *s_m2_state = nullptr;
static lv_obj_t *s_m2_y     = nullptr;

// ---- Hilfsfunktionen (gleiche Logik wie ui_sensoren) ----
static lv_obj_t *makePanel(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_set_size(p, w, h);
    lv_obj_set_pos(p, x, y);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(p, lv_color_hex(0x3A3A8E), 0);
    lv_obj_set_style_border_width(p, 1, 0);
    lv_obj_set_style_radius(p, 10, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    return p;
}

static void makeTitle(lv_obj_t *parent, const char *text)
{
    lv_obj_t *t = lv_label_create(parent);
    lv_label_set_text(t, text);
    lv_obj_set_style_text_color(t, lv_color_hex(0xAAAAFF), 0);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_18, 0);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 8);
}

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
void uiChargerSetup(lv_obj_t *tab)
{
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(tab, 4, 0);
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

    // Drei Panels nebeneinander: je 256px breit, 8px Abstand
    // 8 + 256 + 8 + 256 + 8 + 256 + 8 = 800px
    const int pw         = 256;
    const int ph         = 392;
    const int gap        = 8;
    const int row_start  = 34;
    const int row_step   = 62;

    // ---- Charger (links) ----
    lv_obj_t *p_ch = makePanel(tab, gap, 4, pw, ph);
    makeTitle(p_ch, "Landstrom (IP22)");
    s_ch_v     = makeRow(p_ch, "Spannung:", row_start);
    s_ch_i     = makeRow(p_ch, "Strom:",    row_start + row_step);
    s_ch_state = makeRow(p_ch, "Status:",   row_start + row_step * 2);

    // ---- MPPT1 (mitte) ----
    lv_obj_t *p_m1 = makePanel(tab, gap + pw + gap, 4, pw, ph);
    makeTitle(p_m1, "Solar MPPT1");
    s_m1_v     = makeRow(p_m1, "Spannung:",     row_start);
    s_m1_i     = makeRow(p_m1, "Strom:",        row_start + row_step);
    s_m1_pv    = makeRow(p_m1, "PV Leistung:",  row_start + row_step * 2);
    s_m1_state = makeRow(p_m1, "Status:",       row_start + row_step * 3);
    s_m1_y     = makeRow(p_m1, "Ertrag heute:", row_start + row_step * 4);

    // ---- MPPT2 (rechts) ----
    lv_obj_t *p_m2 = makePanel(tab, gap + pw + gap + pw + gap, 4, pw, ph);
    makeTitle(p_m2, "Solar MPPT2");
    s_m2_v     = makeRow(p_m2, "Spannung:",     row_start);
    s_m2_i     = makeRow(p_m2, "Strom:",        row_start + row_step);
    s_m2_pv    = makeRow(p_m2, "PV Leistung:",  row_start + row_step * 2);
    s_m2_state = makeRow(p_m2, "Status:",       row_start + row_step * 3);
    s_m2_y     = makeRow(p_m2, "Ertrag heute:", row_start + row_step * 4);
}

// ================================================================
// Update
// ================================================================
void uiChargerUpdate(bool force) {
    static uint32_t lastUpdate = 0;
    if (!force && millis() - lastUpdate < 2000) return;
    lastUpdate = millis();
    char buf[32];

    // ---- Charger ----
    if (sensorData.charger_valid)
    {
        snprintf(buf, sizeof(buf), "%.2f V", sensorData.charger_voltage);
        lv_label_set_text(s_ch_v, buf);
        snprintf(buf, sizeof(buf), "%.2f A", sensorData.charger_current);
        lv_label_set_text(s_ch_i, buf);
        lv_label_set_text(s_ch_state, sensorData.charger_stateStr);
    }
    else
    {
        lv_label_set_text(s_ch_v,     "---");
        lv_label_set_text(s_ch_i,     "---");
        lv_label_set_text(s_ch_state, "---");
    }

    // ---- MPPT1 ----
    if (sensorData.mppt1_valid)
    {
        snprintf(buf, sizeof(buf), "%.2f V", sensorData.mppt1_voltage);
        lv_label_set_text(s_m1_v, buf);
        snprintf(buf, sizeof(buf), "%.2f A", sensorData.mppt1_current);
        lv_label_set_text(s_m1_i, buf);
        snprintf(buf, sizeof(buf), "%.1f W", sensorData.mppt1_pv_power);
        lv_label_set_text(s_m1_pv, buf);
        lv_label_set_text(s_m1_state, sensorData.mppt1_stateStr);
        snprintf(buf, sizeof(buf), "%d Wh", sensorData.mppt1_yield_today);
        lv_label_set_text(s_m1_y, buf);
    }
    else
    {
        lv_label_set_text(s_m1_v,     "---");
        lv_label_set_text(s_m1_i,     "---");
        lv_label_set_text(s_m1_pv,    "---");
        lv_label_set_text(s_m1_state, "---");
        lv_label_set_text(s_m1_y,     "---");
    }

    // ---- MPPT2 ----
    if (sensorData.mppt2_valid)
    {
        snprintf(buf, sizeof(buf), "%.2f V", sensorData.mppt2_voltage);
        lv_label_set_text(s_m2_v, buf);
        snprintf(buf, sizeof(buf), "%.2f A", sensorData.mppt2_current);
        lv_label_set_text(s_m2_i, buf);
        snprintf(buf, sizeof(buf), "%.1f W", sensorData.mppt2_pv_power);
        lv_label_set_text(s_m2_pv, buf);
        lv_label_set_text(s_m2_state, sensorData.mppt2_stateStr);
        snprintf(buf, sizeof(buf), "%d Wh", sensorData.mppt2_yield_today);
        lv_label_set_text(s_m2_y, buf);
    }
    else
    {
        lv_label_set_text(s_m2_v,     "---");
        lv_label_set_text(s_m2_i,     "---");
        lv_label_set_text(s_m2_pv,    "---");
        lv_label_set_text(s_m2_state, "---");
        lv_label_set_text(s_m2_y,     "---");
    }
}
