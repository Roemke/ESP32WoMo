#include <esp32_smartdisplay.h>
#include "ui_main.h"
#include "ui_details.h"
#include "ui_charger.h"
//#include "ui_history.h"
#include "wifi.h"
#include "ui_wled.h"
//user interface
static lv_obj_t *ui_tabview = nullptr;

void uiMainSetup()
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Tabview über den ganzen Screen
    ui_tabview = lv_tabview_create(scr);
    lv_obj_t *content = lv_tabview_get_content(ui_tabview);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_tabview_set_tab_bar_position(ui_tabview, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(ui_tabview, 40);
    lv_obj_set_size(ui_tabview, 800, 440);
    lv_obj_set_pos(ui_tabview, 0, 0);
    lv_obj_set_style_bg_color(ui_tabview, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(ui_tabview, LV_OPA_COVER, 0);

    // Tab-Leiste dunkler
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(ui_tabview);
    lv_obj_set_style_bg_color(tab_bar, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_bg_opa(tab_bar, LV_OPA_COVER, 0);


    // Tabs anlegen
    lv_obj_t *tab_sensoren = lv_tabview_add_tab(ui_tabview, "Sensoren");
    lv_obj_t *tab_charger  = lv_tabview_add_tab(ui_tabview, "Charger");
    lv_obj_t *tab_stat     = lv_tabview_add_tab(ui_tabview, "Details");
    lv_obj_t *tab_wled    = lv_tabview_add_tab(ui_tabview, "Beleuchtung");
    //lv_obj_t *tab_bat_hist   = lv_tabview_add_tab(ui_tabview, "Bat-Verlauf");
    //lv_obj_t *tab_klima_hist = lv_tabview_add_tab(ui_tabview, "Klima-Verlauf");

    

    
    // Durch alle Buttons im Tab-Bar iterieren, Farben etwas anpassen
    uint32_t tab_count = lv_tabview_get_tab_count(ui_tabview);
    for (uint32_t i = 0; i < tab_count; i++)
    {
        lv_obj_t *btn = lv_obj_get_child(tab_bar, i);
        // Inaktiv
        lv_obj_set_style_text_color(btn, lv_color_hex(0x8888BB), LV_STATE_DEFAULT);
        // Aktiv
        lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x2233AA), LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_CHECKED);
    }

    // Tab Sensoren – ui_sensoren.cpp
    uiSensorenSetup(tab_sensoren);
    uiSensorenSetIP(wifiGetIP());

    // Tab Charger – ui_charger.cpp
    uiChargerSetup(tab_charger);

    // Details / Statistik
    uiDetailsSetup(tab_stat);
 
    //beleuchtung
    uiWledSetup(tab_wled);
    // Klima und Batterie setup
    //uiHistoryKlimaSetup(tab_klima_hist);
    //uiHistoryBatSetup(tab_bat_hist);

    // ---- Helligkeits-Leiste (unterhalb Tabview, global) ----------
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_set_size(bar, 800, 40);
    lv_obj_set_pos(bar, 0, 440);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 4, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    // Datum + Zeit (links)
    static lv_obj_t *s_datetime = lv_label_create(bar);
    lv_label_set_text(s_datetime, "--.--.---- --:--:--");
    lv_obj_set_style_text_color(s_datetime, lv_color_hex(0x8888BB), 0);
    lv_obj_set_style_text_font(s_datetime, &lv_font_montserrat_18, 0);
    lv_obj_align(s_datetime, LV_ALIGN_LEFT_MID, 8, 0);

    // Display-Off Button (mittig)
    lv_obj_t *btn = lv_button_create(bar);
    lv_obj_set_size(btn, 120, 32);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2255CC), 0);
    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, LV_SYMBOL_POWER " Display");
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(btn_lbl);
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
        static bool on = true;
        on = !on;
        smartdisplay_lcd_set_backlight(on ? 1.0f : 0.0f);
    }, LV_EVENT_CLICKED, nullptr);

    // Helligkeits-Slider (rechts)
    lv_obj_t *slider = lv_slider_create(bar);
    lv_obj_set_size(slider, 280, 10);
    lv_obj_align(slider, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 100, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, [](lv_event_t *e) {
        lv_obj_t *s = (lv_obj_t *)lv_event_get_target(e);
        int val = lv_slider_get_value(s);
        smartdisplay_lcd_set_backlight(val / 100.0f);
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    // Timer: Uhrzeit jede Sekunde aktualisieren
    lv_timer_create([](lv_timer_t *t) {
        lv_label_set_text(s_datetime, wifiGetTime().c_str());
    }, 1000, nullptr);

    Serial.println("setupUI OK");
}
