#include "ui_history.h"
#include "sdcard.h"
#include "wifi.h"
#include <ArduinoJson.h>

// ----------------------------------------------------------------
// Private State – Klima, Batterie
// ----------------------------------------------------------------
// Chart-Objekte – lv_obj_t*
static lv_obj_t *s_klima_chart = nullptr;
static lv_obj_t *s_bat_chart   = nullptr;

// Series – lv_chart_series_t*
static lv_chart_series_t *s_klima_ser_t   = nullptr;
static lv_chart_series_t *s_klima_ser_h   = nullptr;
static lv_chart_series_t *s_klima_ser_p   = nullptr;
static lv_chart_series_t *s_klima_ser_co2 = nullptr;
static lv_chart_series_t *s_bat_ser_v     = nullptr;
static lv_chart_series_t *s_bat_ser_i     = nullptr;
static lv_chart_series_t *s_bat_ser_soc   = nullptr;
static lv_chart_series_t *s_bat_ser_pw    = nullptr;
static lv_chart_series_t *s_bat_ser_vs    = nullptr;

// Datepicker State – Von/Bis als tm struct
static struct tm s_klima_from = {};
static struct tm s_klima_to   = {};
static struct tm s_bat_from   = {};
static struct tm s_bat_to     = {};

//fuer den status
static bool      s_load_pending = false;
static bool      s_load_isKlima = false;
static struct tm s_load_from    = {};
static struct tm s_load_to      = {};

// ----------------------------------------------------------------
// Hilfsfunktion: Panel erstellen (analog ui_sensoren)
// ----------------------------------------------------------------
static lv_obj_t *makePanel(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_set_size(p, w, h);
    lv_obj_set_pos(p, x, y);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(p, lv_color_hex(0x3A3A8E), 0);
    lv_obj_set_style_border_width(p, 1, 0);
    lv_obj_set_style_radius(p, 8, 0);
    lv_obj_set_style_pad_all(p, 4, 0);
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    return p;
}

// ----------------------------------------------------------------
// Hilfsfunktion: Datepicker – Von/Bis Zeile
// Gibt Label zurück das Datum/Zeit anzeigt
// ----------------------------------------------------------------
static lv_obj_t *makeDateLabel(lv_obj_t *parent, const char *prefix, int y, struct tm *dt)
{
    
    
    lv_obj_t *lbl_prefix = lv_label_create(parent);
    lv_label_set_text(lbl_prefix, prefix);
    lv_obj_set_style_text_color(lbl_prefix, lv_color_hex(0x8888BB), 0);
    lv_obj_set_style_text_font(lbl_prefix, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_prefix, 8, y);

    lv_obj_t *lbl_val = lv_label_create(parent);
    char buf[32];
    if (dt->tm_year > 0)
        strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M", dt);
    else
        strncpy(buf, "--.--.---- --:--", sizeof(buf));
    lv_label_set_text(lbl_val, buf);
    
    lv_label_set_text(lbl_val, buf);
    lv_obj_set_style_text_color(lbl_val, lv_color_hex(0xEEEEFF), 0);
    lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_val, 50, y);
    return lbl_val;
}

// ----------------------------------------------------------------
// Datepicker Dialog – Roller für Tag/Monat/Jahr/Stunde/Minute
// ----------------------------------------------------------------
static void showDatepicker(struct tm *dt, lv_obj_t *lbl_update, lv_obj_t *parent)
{
    // Modal Box
    lv_obj_t *box = lv_obj_create(lv_screen_active());
    lv_obj_set_size(box, 500, 260);
    lv_obj_center(box);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(0x3A3A8E), 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_radius(box, 8, 0);

    // Roller-Optionen
    // Tag 01-31
    char days[128] = "";
    for (int i=1; i<=31; i++) { char tmp[8]; snprintf(tmp, sizeof(tmp), i<31?"%02d\n":"%02d", i); strcat(days, tmp); }
    // Monat 01-12
    char months[64] = "";
    for (int i=1; i<=12; i++) { char tmp[8]; snprintf(tmp, sizeof(tmp), i<12?"%02d\n":"%02d", i); strcat(months, tmp); }
    // Jahr 2025-2030
    char years[64] = "2025\n2026\n2027\n2028\n2029\n2030";
    // Stunde 00-23
    char hours[128] = "";
    for (int i=0; i<=23; i++) { char tmp[8]; snprintf(tmp, sizeof(tmp), i<23?"%02d\n":"%02d", i); strcat(hours, tmp); }
    // Minute 00-59
    char mins[256] = "";
    for (int i=0; i<=59; i++) { char tmp[8]; snprintf(tmp, sizeof(tmp), i<59?"%02d\n":"%02d", i); strcat(mins, tmp); }

    // Roller erstellen
    const char *opts[]   = { days, months, years, hours, mins };
    const int   widths[] = { 60, 60, 80, 60, 60 };
    const char *labels[] = { "Tag", "Mon", "Jahr", "Std", "Min" };
    lv_obj_t *rollers[5];

    int x = 20;
    for (int i=0; i<5; i++)
    {
        lv_obj_t *lbl = lv_label_create(box);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x8888BB), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_pos(lbl, x + widths[i]/2 - 10, 8);

        rollers[i] = lv_roller_create(box);
        lv_roller_set_options(rollers[i], opts[i], LV_ROLLER_MODE_NORMAL);
        lv_roller_set_visible_row_count(rollers[i], 3);
        lv_obj_set_width(rollers[i], widths[i]);
        lv_obj_set_pos(rollers[i], x, 28);
        lv_obj_set_style_bg_color(rollers[i], lv_color_hex(0x16213E), 0);
        lv_obj_set_style_text_color(rollers[i], lv_color_hex(0xEEEEFF), 0);
        lv_obj_set_style_text_font(rollers[i], &lv_font_montserrat_14, 0);

        // Aktuellen Wert setzen
        int sel = 0;
        if (i==0) sel = dt->tm_mday - 1;
        if (i==1) sel = dt->tm_mon;
        if (i==2) sel = dt->tm_year - 125; // 2025 = index 0
        if (i==3) sel = dt->tm_hour;
        if (i==4) sel = dt->tm_min;
        lv_roller_set_selected(rollers[i], sel, LV_ANIM_OFF);

        x += widths[i] + 8;
    }

    // OK Button
    lv_obj_t *btn = lv_button_create(box);
    lv_obj_set_size(btn, 100, 36);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2255CC), 0);
    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "OK");
    lv_obj_center(btn_lbl);

    // Struct für Callback
    struct DpData {
        lv_obj_t *rollers[5];
        lv_obj_t *box;
        lv_obj_t *lbl_update;
        struct tm *dt;
    };
    auto *dpd = new DpData;
    for (int i=0; i<5; i++) dpd->rollers[i] = rollers[i];
    dpd->box        = box;
    dpd->lbl_update = lbl_update;
    dpd->dt         = dt;

    lv_obj_add_event_cb(btn, [](lv_event_t *e)
    {
        auto *dpd = (DpData*)lv_event_get_user_data(e);
        dpd->dt->tm_mday  = lv_roller_get_selected(dpd->rollers[0]) + 1;
        dpd->dt->tm_mon   = lv_roller_get_selected(dpd->rollers[1]);
        dpd->dt->tm_year  = lv_roller_get_selected(dpd->rollers[2]) + 125;
        dpd->dt->tm_hour  = lv_roller_get_selected(dpd->rollers[3]);
        dpd->dt->tm_min   = lv_roller_get_selected(dpd->rollers[4]);
        dpd->dt->tm_sec   = 0;
        mktime(dpd->dt); // Wochentag etc. korrigieren
        char buf[32];
        strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M", dpd->dt);
        lv_label_set_text(dpd->lbl_update, buf);
        lv_obj_del(dpd->box);
        delete dpd;
    }, LV_EVENT_CLICKED, dpd);
}

// ----------------------------------------------------------------
// Daten laden und Chart befüllen
// ----------------------------------------------------------------
static void loadChartData()
{
    char from_s[20], to_s[20];
    strftime(from_s, sizeof(from_s), "%Y-%m-%dT%H:%M", &s_load_from);
    strftime(to_s,   sizeof(to_s),   "%Y-%m-%dT%H:%M", &s_load_to);

    lv_obj_t *chart = s_load_isKlima ? s_klima_chart : s_bat_chart;
    int points = lv_obj_get_width(chart);

    const char *json = sdGetHistoryBuffer(from_s, to_s, points);
    if (!json || strstr(json, "\"error\"")) return;

    const char *lp = strstr(json, "\"labels\":[");
    if (!lp) return;
    lp += 10;
    int count = 0;
    while (*lp && *lp != ']') { if (*lp == '"') count++; lp++; }
    count /= 2;
 
    if(count == 0) return;
    lv_chart_set_point_count(chart, count);

    // Alle Series zurücksetzen
    if (s_load_isKlima)
    {
        lv_chart_set_all_value(chart, s_klima_ser_t,   LV_CHART_POINT_NONE);
        lv_chart_set_all_value(chart, s_klima_ser_h,   LV_CHART_POINT_NONE);
        lv_chart_set_all_value(chart, s_klima_ser_p,   LV_CHART_POINT_NONE);
        lv_chart_set_all_value(chart, s_klima_ser_co2, LV_CHART_POINT_NONE);
    }
    else
    {
        lv_chart_set_all_value(chart, s_bat_ser_v,   LV_CHART_POINT_NONE);
        lv_chart_set_all_value(chart, s_bat_ser_i,   LV_CHART_POINT_NONE);
        lv_chart_set_all_value(chart, s_bat_ser_soc, LV_CHART_POINT_NONE);
        lv_chart_set_all_value(chart, s_bat_ser_pw,  LV_CHART_POINT_NONE);
        lv_chart_set_all_value(chart, s_bat_ser_vs,  LV_CHART_POINT_NONE);
    }
    

    auto parseArray = [](const char *json, const char *key,
                     lv_obj_t *chart, lv_chart_series_t *ser,
                     float scale, int count)
    {
        char search[16];
        snprintf(search, sizeof(search), "\"%s\":[", key);
        const char *p = strstr(json, search);
        if (!p) return;
        p += strlen(search);
        int idx = 0;
        while (*p && *p != ']' && idx < count)
        {
            if (*p == ',' || *p == ' ') { p++; continue; }
            char *end;
            float val = strtof(p, &end);
            if (end == p) break;
            lv_chart_set_series_value_by_id(chart, ser, idx, (int)(val * scale));
            idx++;
            p = end;
        }
    };

    if (s_load_isKlima)
    {
        parseArray(json, "T",   chart, s_klima_ser_t,    10.0f, count);
        parseArray(json, "H",   chart, s_klima_ser_h,    10.0f, count);
        parseArray(json, "P",   chart, s_klima_ser_p,     1.0f, count);
        parseArray(json, "CO2", chart, s_klima_ser_co2,   1.0f, count);
    }
    else
    {
        parseArray(json, "V",   chart, s_bat_ser_v,     100.0f, count);
        parseArray(json, "I",   chart, s_bat_ser_i,     100.0f, count);
        parseArray(json, "SOC", chart, s_bat_ser_soc,    10.0f, count);
        parseArray(json, "PW",  chart, s_bat_ser_pw,      1.0f, count);
        parseArray(json, "VS",  chart, s_bat_ser_vs,    100.0f, count);
    }
    lv_chart_refresh(chart);
}

void uiHistoryLoop()
{
    if (!s_load_pending) return;
    s_load_pending = false;
    loadChartData();
}

// ----------------------------------------------------------------
// Setup – gemeinsame Logik für beide Tabs
// ----------------------------------------------------------------
static void setupHistoryTab(lv_obj_t *tab, bool isKlima)
{
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(tab, 4, 0);
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

    struct tm *pfrom = isKlima ? &s_klima_from : &s_bat_from;
    struct tm *pto   = isKlima ? &s_klima_to   : &s_bat_to;

    // Standardwerte: letzte 24h
   // Datum initialisieren – nur wenn NTP sync
    if (wifiGetTime().length() > 0)
    {
        getLocalTime(pto, 0);
        *pfrom = *pto;
        pfrom->tm_hour -= 24;
        mktime(pfrom);
    }
    else
    {
        memset(pfrom, 0, sizeof(struct tm));
        memset(pto,   0, sizeof(struct tm));
    }

    // ---- Kontroll-Panel oben (50px) ----
    lv_obj_t *ctrl = makePanel(tab, 0, 0, 792, 50);

    // Von: Prefix + Wert + Button
    lv_obj_t *lbl_von = lv_label_create(ctrl);
    lv_label_set_text(lbl_von, "Von:");
    lv_obj_set_style_text_color(lbl_von, lv_color_hex(0x8888BB), 0);
    lv_obj_set_style_text_font(lbl_von, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_von, 8, 16);

    lv_obj_t *lbl_from = lv_label_create(ctrl);
    lv_obj_set_style_text_color(lbl_from, lv_color_hex(0xEEEEFF), 0);
    lv_obj_set_style_text_font(lbl_from, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_from, 52, 16);
    {
        char buf[32];
        if (pfrom->tm_year > 0) strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M", pfrom);
        else strncpy(buf, "--.--.---- --:--", sizeof(buf));
        lv_label_set_text(lbl_from, buf);
    }

    lv_obj_t *btn_from = lv_button_create(ctrl);
    lv_obj_set_size(btn_from, 36, 28);
    lv_obj_set_pos(btn_from, 240, 11);
    lv_obj_set_style_bg_color(btn_from, lv_color_hex(0x2255CC), 0);
    lv_obj_t *bl1 = lv_label_create(btn_from);
    lv_label_set_text(bl1, LV_SYMBOL_EDIT);
    lv_obj_center(bl1);

    // Bis: Prefix + Wert + Button
    lv_obj_t *lbl_bis = lv_label_create(ctrl);
    lv_label_set_text(lbl_bis, "Bis:");
    lv_obj_set_style_text_color(lbl_bis, lv_color_hex(0x8888BB), 0);
    lv_obj_set_style_text_font(lbl_bis, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_bis, 300, 16);

    lv_obj_t *lbl_to = lv_label_create(ctrl);
    lv_obj_set_style_text_color(lbl_to, lv_color_hex(0xEEEEFF), 0);
    lv_obj_set_style_text_font(lbl_to, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_to, 344, 16);
    {
        char buf[32];
        if (pto->tm_year > 0) strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M", pto);
        else strncpy(buf, "--.--.---- --:--", sizeof(buf));
        lv_label_set_text(lbl_to, buf);
    }

    lv_obj_t *btn_to = lv_button_create(ctrl);
    lv_obj_set_size(btn_to, 36, 28);
    lv_obj_set_pos(btn_to, 532, 11);
    lv_obj_set_style_bg_color(btn_to, lv_color_hex(0x2255CC), 0);
    lv_obj_t *bl2 = lv_label_create(btn_to);
    lv_label_set_text(bl2, LV_SYMBOL_EDIT);
    lv_obj_center(bl2);

    // Laden Button rechts
    lv_obj_t *btn_load = lv_button_create(ctrl);
    lv_obj_set_size(btn_load, 80, 28);
    lv_obj_set_pos(btn_load, 700, 11);
    lv_obj_set_style_bg_color(btn_load, lv_color_hex(0x2255CC), 0);
    lv_obj_t *bl3 = lv_label_create(btn_load);
    lv_label_set_text(bl3, "Laden");
    lv_obj_center(bl3);

    // ---- Chart Panel ----
    lv_obj_t *p_chart = makePanel(tab, 0, 54, 792, 334);

    lv_obj_t *chart = lv_chart_create(p_chart);
    lv_obj_set_size(chart, 780, 320);
    lv_obj_set_pos(chart, 6, 8);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 400);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x2A2A4E), LV_PART_MAIN);
    lv_chart_set_div_line_count(chart, 5, 5);


    if (isKlima)
    {
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -100, 1000);   // T×10: -10..100°C, H×10: 0..100%
        lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, 0, 2000);    // P: hPa, CO2: ppm
        s_klima_chart   = chart;
        s_klima_ser_t   = lv_chart_add_series(chart, lv_color_hex(0xFF7043), LV_CHART_AXIS_PRIMARY_Y);
        s_klima_ser_h   = lv_chart_add_series(chart, lv_color_hex(0x42A5F5), LV_CHART_AXIS_PRIMARY_Y);
        s_klima_ser_p   = lv_chart_add_series(chart, lv_color_hex(0xAB47BC), LV_CHART_AXIS_SECONDARY_Y);
        s_klima_ser_co2 = lv_chart_add_series(chart, lv_color_hex(0x66BB6A), LV_CHART_AXIS_SECONDARY_Y);
    }
    else
    {
        s_bat_chart   = chart;
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -2000, 2000);  // V×100, I×100, PW
        lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, 0, 1000);    // SOC×10: 0..100%
        s_bat_ser_v   = lv_chart_add_series(chart, lv_color_hex(0x66BB6A), LV_CHART_AXIS_PRIMARY_Y);
        s_bat_ser_i   = lv_chart_add_series(chart, lv_color_hex(0xAB47BC), LV_CHART_AXIS_PRIMARY_Y);
        s_bat_ser_soc = lv_chart_add_series(chart, lv_color_hex(0xFFA726), LV_CHART_AXIS_SECONDARY_Y);
        s_bat_ser_pw  = lv_chart_add_series(chart, lv_color_hex(0xEF5350), LV_CHART_AXIS_PRIMARY_Y);
        s_bat_ser_vs = lv_chart_add_series(chart, lv_color_hex(0x26C6DA), LV_CHART_AXIS_PRIMARY_Y);
        
    }

    // Button Callbacks
    struct BtnData {
        bool      isKlima;
        lv_obj_t *lbl;
        struct tm *dt;
        struct tm *pfrom;
        struct tm *pto;
        lv_obj_t *parent;
    };

   auto *bd_from = new BtnData{isKlima, lbl_from, pfrom, pfrom, pto, ctrl};
    lv_obj_add_event_cb(btn_from, [](lv_event_t *e) {
        auto *bd = (BtnData*)lv_event_get_user_data(e);
        showDatepicker(bd->dt, bd->lbl, bd->parent);
    }, LV_EVENT_CLICKED, bd_from);

    auto *bd_to = new BtnData{isKlima, lbl_to, pto, pfrom, pto, ctrl};
    lv_obj_add_event_cb(btn_to, [](lv_event_t *e) {
        auto *bd = (BtnData*)lv_event_get_user_data(e);
        showDatepicker(bd->dt, bd->lbl, bd->parent);
    }, LV_EVENT_CLICKED, bd_to);

    // bd_load mit korrekten pfrom/pto
    auto *bd_load = new BtnData{isKlima, nullptr, nullptr, pfrom, pto, nullptr};
    lv_obj_add_event_cb(btn_load, [](lv_event_t *e) {
        auto *bd = (BtnData*)lv_event_get_user_data(e);
        if (bd->pfrom->tm_year == 0 || bd->pto->tm_year == 0) return;
        //nicht laden, flags setzen 
        s_load_pending = true;
        s_load_isKlima = bd->isKlima;
        s_load_from    = *bd->pfrom;
        s_load_to      = *bd->pto;
    }, LV_EVENT_CLICKED, bd_load);
}

// ================================================================
// Öffentliche Funktionen
// ================================================================
void uiHistoryKlimaSetup(lv_obj_t *tab)
{
    setupHistoryTab(tab, true);
}

void uiHistoryBatSetup(lv_obj_t *tab)
{
    setupHistoryTab(tab, false);
}