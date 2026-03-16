/**
 * main.cpp
 * LVGL Demo für ESP32-S3 4,3" 800x480 RGB-Display
 * Bibliothek: rzeldent/esp32-smartdisplay 2.1.1 + LVGL 9.x
 */

#include <Arduino.h>
#include <esp32_smartdisplay.h>

// ----------------------------------------------------------------
// Forward-Deklarationen
// ----------------------------------------------------------------
static void create_demo_ui(void);
static void btn_event_cb(lv_event_t *e);
static void slider_event_cb(lv_event_t *e);

// ----------------------------------------------------------------
// Globale UI-Handles
// ----------------------------------------------------------------
static lv_obj_t *label_counter = nullptr;
static lv_obj_t *arc           = nullptr;
static lv_obj_t *label_arc     = nullptr;

// ----------------------------------------------------------------
// Setup
// ----------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    Serial.println("\n=== ESP32-S3 LVGL Demo ===");

    smartdisplay_init();
    smartdisplay_lcd_set_backlight(1.0f);

    auto display = lv_display_get_default();
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);

    create_demo_ui();
}

// ----------------------------------------------------------------
// Loop
// ----------------------------------------------------------------
static auto lv_last_tick = millis();

void loop()
{
    auto const now = millis();
    lv_tick_inc(now - lv_last_tick);
    lv_last_tick = now;
    lv_timer_handler();

    static uint32_t last_second = 0;
    static uint32_t counter     = 0;

    if (now - last_second >= 1000)
    {
        last_second = now;
        counter++;

        char buf[32];
        snprintf(buf, sizeof(buf), "Laufzeit: %lu s", counter);
        lv_label_set_text(label_counter, buf);

        lv_arc_set_value(arc, (int)(counter % 100));
        snprintf(buf, sizeof(buf), "%lu%%", counter % 100);
        lv_label_set_text(label_arc, buf);
    }
}

// ----------------------------------------------------------------
// Demo-UI
// ----------------------------------------------------------------
static void create_demo_ui(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Titel
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "ESP32-S3  LVGL Demo");
    lv_obj_set_style_text_color(title, lv_color_hex(0xE0E0FF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Laufzeit
    label_counter = lv_label_create(scr);
    lv_label_set_text(label_counter, "Laufzeit: 0 s");
    lv_obj_set_style_text_color(label_counter, lv_color_hex(0xAAFFAA), 0);
    lv_obj_set_style_text_font(label_counter, &lv_font_montserrat_16, 0);
    lv_obj_align(label_counter, LV_ALIGN_TOP_MID, 0, 50);

    // ---- Linke Spalte: Buttons ----------------------------------
    lv_obj_t *panel_left = lv_obj_create(scr);
    lv_obj_set_size(panel_left, 240, 340);
    lv_obj_align(panel_left, LV_ALIGN_LEFT_MID, 20, 30);
    lv_obj_set_style_bg_color(panel_left, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_border_color(panel_left, lv_color_hex(0x3A3A8E), 0);
    lv_obj_set_style_border_width(panel_left, 1, 0);
    lv_obj_set_style_radius(panel_left, 12, 0);

    lv_obj_t *lbl_btn = lv_label_create(panel_left);
    lv_label_set_text(lbl_btn, "Buttons");
    lv_obj_set_style_text_color(lbl_btn, lv_color_hex(0xCCCCFF), 0);
    lv_obj_align(lbl_btn, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t *status_lbl = lv_label_create(panel_left);
    lv_label_set_text(status_lbl, "---");
    lv_obj_set_style_text_color(status_lbl, lv_color_hex(0xFFFF88), 0);
    lv_obj_align(status_lbl, LV_ALIGN_BOTTOM_MID, 0, -10);

    const char    *btn_labels[] = {"Start", "Pause", "Reset"};
    const uint32_t btn_colors[] = {0x00AA44, 0xCC8800, 0xAA2222};

    for (int i = 0; i < 3; i++)
    {
        lv_obj_t *btn = lv_btn_create(panel_left);
        lv_obj_set_size(btn, 180, 55);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 40 + i * 75);
        lv_obj_set_style_bg_color(btn, lv_color_hex(btn_colors[i]), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(btn_colors[i]), LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_user_data(btn, (void *)(intptr_t)i);
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, status_lbl);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_center(lbl);
    }

    // ---- Mittlere Spalte: Arc + Slider --------------------------
    lv_obj_t *panel_mid = lv_obj_create(scr);
    lv_obj_set_size(panel_mid, 240, 340);
    lv_obj_align(panel_mid, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(panel_mid, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_border_color(panel_mid, lv_color_hex(0x3A3A8E), 0);
    lv_obj_set_style_border_width(panel_mid, 1, 0);
    lv_obj_set_style_radius(panel_mid, 12, 0);

    lv_obj_t *lbl_arc_title = lv_label_create(panel_mid);
    lv_label_set_text(lbl_arc_title, "Fortschritt");
    lv_obj_set_style_text_color(lbl_arc_title, lv_color_hex(0xCCCCFF), 0);
    lv_obj_align(lbl_arc_title, LV_ALIGN_TOP_MID, 0, 5);

    arc = lv_arc_create(panel_mid);
    lv_obj_set_size(arc, 150, 150);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_value(arc, 0);
    lv_arc_set_mode(arc, LV_ARC_MODE_NORMAL);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x2255CC), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x44AAFF), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_align(arc, LV_ALIGN_TOP_MID, 0, 45);

    label_arc = lv_label_create(panel_mid);
    lv_label_set_text(label_arc, "0%");
    lv_obj_set_style_text_color(label_arc, lv_color_hex(0x44CCFF), 0);
    lv_obj_set_style_text_font(label_arc, &lv_font_montserrat_20, 0);
    lv_obj_align_to(label_arc, arc, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *lbl_slider = lv_label_create(panel_mid);
    lv_label_set_text(lbl_slider, "Helligkeit");
    lv_obj_set_style_text_color(lbl_slider, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(lbl_slider, LV_ALIGN_BOTTOM_MID, 0, -70);

    lv_obj_t *slider = lv_slider_create(panel_mid);
    lv_obj_set_size(slider, 180, 20);
    lv_slider_set_value(slider, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x2255CC), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x44AAFF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -35);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    // ---- Rechte Spalte: System-Info -----------------------------
    lv_obj_t *panel_right = lv_obj_create(scr);
    lv_obj_set_size(panel_right, 240, 340);
    lv_obj_align(panel_right, LV_ALIGN_RIGHT_MID, -20, 30);
    lv_obj_set_style_bg_color(panel_right, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_border_color(panel_right, lv_color_hex(0x3A3A8E), 0);
    lv_obj_set_style_border_width(panel_right, 1, 0);
    lv_obj_set_style_radius(panel_right, 12, 0);

    lv_obj_t *lbl_info = lv_label_create(panel_right);
    lv_label_set_text(lbl_info, "System-Info");
    lv_obj_set_style_text_color(lbl_info, lv_color_hex(0xCCCCFF), 0);
    lv_obj_align(lbl_info, LV_ALIGN_TOP_MID, 0, 5);

    const char *info_keys[]   = {"MCU:",     "Takt:",    "Flash:",  "PSRAM:",    "Display:",    "Touch:"};
    const char *info_values[] = {"ESP32-S3", "240 MHz",  "16 MB",   "8 MB OPI",  "800x480 RGB", "GT911 Cap"};

    for (int i = 0; i < 6; i++)
    {
        lv_obj_t *k = lv_label_create(panel_right);
        lv_label_set_text(k, info_keys[i]);
        lv_obj_set_style_text_color(k, lv_color_hex(0x8888BB), 0);
        lv_obj_set_style_text_font(k, &lv_font_montserrat_14, 0);
        lv_obj_align(k, LV_ALIGN_TOP_LEFT, 10, 40 + i * 44);

        lv_obj_t *v = lv_label_create(panel_right);
        lv_label_set_text(v, info_values[i]);
        lv_obj_set_style_text_color(v, lv_color_hex(0xEEEEFF), 0);
        lv_obj_set_style_text_font(v, &lv_font_montserrat_14, 0);
        lv_obj_align(v, LV_ALIGN_TOP_RIGHT, -10, 40 + i * 44);
    }
}

// ----------------------------------------------------------------
// Events
// ----------------------------------------------------------------
static void btn_event_cb(lv_event_t *e)
{
    lv_obj_t *status_lbl = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_t *btn        = (lv_obj_t *)lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn);
    const char *msgs[] = {"Gestartet!", "Pausiert.", "Zurueckgesetzt."};
    if (idx >= 0 && idx < 3)
        lv_label_set_text(status_lbl, msgs[idx]);
}

static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    smartdisplay_lcd_set_backlight((float)val / 100.0f);
}