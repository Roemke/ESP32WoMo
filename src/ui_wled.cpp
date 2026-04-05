#include "ui_wled.h"
#include "appconfig.h"
#include "wled.h"

// ----------------------------------------------------------------
// Vordefinierte Farben
// ----------------------------------------------------------------
static const uint32_t WLED_COLORS[] = {
    0xFF0000,  // Rot
    0xFF8800,  // Orange
    0xFFC800,  // Gelb
    0xFFD0A0,  // Warm-Weiß
    0xFFFFFF,  // Weiß
    0x8000FF,  // Violett
    0x0000FF,  // Blau
    0x00FFAA,  // Türkis
    0x00FF00,  // Grün
};
#define WLED_COLOR_COUNT 9

// ----------------------------------------------------------------
// State pro WLED-Instanz
// ----------------------------------------------------------------
struct WledBox {
    WledInstance *inst; 
    bool        on; //state
    bool        online; // die wled sind häufiger mal stromlos, das ist normal
    uint8_t     bri;
    uint8_t     r, g, b;

    lv_obj_t   *btnPower;
    lv_obj_t   *lblPower;
    lv_obj_t   *sliderBri;
    lv_obj_t   *sliderR;
    lv_obj_t   *sliderG;
    lv_obj_t   *sliderB;
    lv_obj_t   *preview;
};

static WledBox s_boxes[2];


static void updatePreview(WledBox &box) {
    // Farbe mit Helligkeit skalieren
    uint8_t r = (uint8_t)((uint32_t)box.r * box.bri / 255);
    uint8_t g = (uint8_t)((uint32_t)box.g * box.bri / 255);
    uint8_t b = (uint8_t)((uint32_t)box.b * box.bri / 255);
    lv_obj_set_style_bg_color(box.preview, lv_color_make(r, g, b), 0);
}

// ----------------------------------------------------------------
// Status anwenden
// ----------------------------------------------------------------
static void wledApplyState(WledBox &box) {
    if (!box.online) {
        lv_obj_set_style_bg_color(box.btnPower, lv_color_hex(0xCC2222), 0);
        lv_label_set_text(box.lblPower, LV_SYMBOL_WARNING " Offline");
        return;
    }
    lv_obj_set_style_bg_color(box.btnPower,
        box.on ? lv_color_hex(0x2e7d32) : lv_color_hex(0x444466), 0);
    lv_label_set_text(box.lblPower, box.on ? LV_SYMBOL_POWER " AN" : LV_SYMBOL_POWER " AUS");
    lv_slider_set_value(box.sliderBri, box.bri, LV_ANIM_OFF);
    lv_slider_set_value(box.sliderR,   box.r,   LV_ANIM_OFF);
    lv_slider_set_value(box.sliderG,   box.g,   LV_ANIM_OFF);
    lv_slider_set_value(box.sliderB,   box.b,   LV_ANIM_OFF);
    updatePreview(box);
}

// ----------------------------------------------------------------
// Panel erstellen
// ----------------------------------------------------------------
static lv_obj_t *makePanel(lv_obj_t *parent, int x, int y, int w, int h) {
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_set_size(p, w, h);
    lv_obj_set_pos(p, x, y);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(p, lv_color_hex(0x3A3A8E), 0);
    lv_obj_set_style_border_width(p, 1, 0);
    lv_obj_set_style_radius(p, 10, 0);
    lv_obj_set_style_pad_all(p, 5, 0);
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    return p;
}

// ----------------------------------------------------------------
// Slider mit Label erstellen
// ----------------------------------------------------------------
static lv_obj_t* makeSlider(lv_obj_t *parent, const char *label, int y, uint32_t color) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(lbl, 0, y);

    lv_obj_t *sl = lv_slider_create(parent);
    lv_obj_set_size(sl, 330, 10);
    lv_obj_set_pos(sl, 24, y);
    lv_slider_set_range(sl, 0, 255);
    lv_obj_set_style_bg_color(sl, lv_color_hex(color), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sl, lv_color_hex(color), LV_PART_KNOB);
    return sl;
}

// ----------------------------------------------------------------
// Box aufbauen
// ----------------------------------------------------------------
static void buildBox(WledBox &box, lv_obj_t *panel, const char *title) {
    // Titel
    lv_obj_t *lbl = lv_label_create(panel);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xAAAAFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 0);

    // Power Button
    box.btnPower = lv_button_create(panel);
    lv_obj_set_size(box.btnPower, 120, 36);
    lv_obj_set_pos(box.btnPower, 0, 28);
    lv_obj_set_style_bg_color(box.btnPower, lv_color_hex(0x444466), 0);
    lv_obj_set_style_radius(box.btnPower, 6, 0);
    box.lblPower = lv_label_create(box.btnPower);
    lv_label_set_text(box.lblPower, LV_SYMBOL_POWER " AUS");
    lv_obj_center(box.lblPower);    
    lv_obj_add_event_cb(box.btnPower, [](lv_event_t *e) {
        WledBox *b = (WledBox*)lv_event_get_user_data(e);
        wledSetState(*b->inst, !b->inst->lastState);
        wledApplyState(*b);
    }, LV_EVENT_CLICKED, &box);
    // Vorschau-Kreis
    box.preview = lv_obj_create(panel);
    lv_obj_set_size(box.preview, 36, 36);
    lv_obj_set_pos(box.preview, 330, 28);
    //lv_obj_set_style_radius(box.preview, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(box.preview, lv_color_make(255, 255, 255), 0);
    lv_obj_set_style_bg_opa(box.preview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(box.preview, lv_color_hex(0x888888), 0);
    lv_obj_set_style_border_width(box.preview, 1, 0);
    lv_obj_clear_flag(box.preview, LV_OBJ_FLAG_SCROLLABLE);

    // Brightness Slider
    lv_obj_t *lblBri = lv_label_create(panel);
    lv_label_set_text(lblBri, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(lblBri, lv_color_hex(0x8888BB), 0);
    lv_obj_set_pos(lblBri, 0, 90);

    box.sliderBri = lv_slider_create(panel);
    lv_obj_set_size(box.sliderBri, 330, 10);
    lv_obj_set_pos(box.sliderBri, 24, 90);
    lv_slider_set_range(box.sliderBri, 1, 255);
    lv_slider_set_value(box.sliderBri, 128, LV_ANIM_OFF);
    lv_obj_add_event_cb(box.sliderBri, [](lv_event_t *e) {
        WledBox *b = (WledBox*)lv_event_get_user_data(e);
        b->bri = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
        updatePreview(*b);
    }, LV_EVENT_VALUE_CHANGED, &box);
    lv_obj_add_event_cb(box.sliderBri, [](lv_event_t *e) {
        WledBox *b = (WledBox*)lv_event_get_user_data(e);
        b->bri = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
        wledSendBri(*b->inst, b->bri);
    }, LV_EVENT_RELEASED, &box);

    // RGB Slider
    box.sliderR = makeSlider(panel, "R", 150, 0xFF2222);
    lv_obj_add_event_cb(box.sliderR, [](lv_event_t *e) {
        WledBox *b = (WledBox*)lv_event_get_user_data(e);
        b->r = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
        updatePreview(*b);
    }, LV_EVENT_VALUE_CHANGED, &box);
    lv_obj_add_event_cb(box.sliderR, [](lv_event_t *e) {
        WledBox *b = (WledBox*)lv_event_get_user_data(e);
        b->r = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
        wledSendColor(*b->inst, b->r, b->g, b->b);
    }, LV_EVENT_RELEASED, &box);

    box.sliderG = makeSlider(panel, "G", 190, 0x22CC22);
    lv_obj_add_event_cb(box.sliderG, [](lv_event_t *e) {
        WledBox *b = (WledBox*)lv_event_get_user_data(e);
        b->g = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
        updatePreview(*b);
    }, LV_EVENT_VALUE_CHANGED, &box);
    lv_obj_add_event_cb(box.sliderG, [](lv_event_t *e) {
        WledBox *b = (WledBox*)lv_event_get_user_data(e);
        b->g = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
        wledSendColor(*b->inst, b->r, b->g, b->b);
    }, LV_EVENT_RELEASED, &box);

    box.sliderB = makeSlider(panel, "B", 230, 0x2222FF);
    lv_obj_add_event_cb(box.sliderB, [](lv_event_t *e) {
        WledBox *b = (WledBox*)lv_event_get_user_data(e);
        b->b = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
        updatePreview(*b);
    }, LV_EVENT_VALUE_CHANGED, &box);
    lv_obj_add_event_cb(box.sliderB, [](lv_event_t *e) {
        WledBox *b = (WledBox*)lv_event_get_user_data(e);
        b->b = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
        wledSendColor(*b->inst, b->r, b->g, b->b);
    }, LV_EVENT_RELEASED, &box);
    // Farbbuttons
    const int btnSize = 34;
    const int btnGap  = 9;
    const int startX  = 0;
    const int startY  = 305;

    for (int i = 0; i < WLED_COLOR_COUNT; i++) {
        int col = i ; // % 5;
        int row = 0; //i / 5; nehme nur eine Zeile, müsste ohne Random passen
        lv_obj_t *btn = lv_button_create(panel);
        lv_obj_set_size(btn, btnSize, btnSize);
        lv_obj_set_pos(btn, startX + col * (btnSize + btnGap), startY + row * (btnSize + btnGap));
        lv_obj_set_style_bg_color(btn, lv_color_hex(WLED_COLORS[i]), 0);
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x666688), 0);
        lv_obj_set_style_border_width(btn, 1, 0);

        struct ColorData { WledBox *box; uint32_t color; };
        ColorData *cd = new ColorData{&box, WLED_COLORS[i]};
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            ColorData *cd = (ColorData*)lv_event_get_user_data(e);
            uint32_t col = cd->color;
            cd->box->r = (col >> 16) & 0xFF;
            cd->box->g = (col >> 8)  & 0xFF;
            cd->box->b =  col        & 0xFF;
            lv_slider_set_value(cd->box->sliderR, cd->box->r, LV_ANIM_OFF);
            lv_slider_set_value(cd->box->sliderG, cd->box->g, LV_ANIM_OFF);
            lv_slider_set_value(cd->box->sliderB, cd->box->b, LV_ANIM_OFF);
            updatePreview(*cd->box);
            wledSendColor(*cd->box->inst, cd->box->r, cd->box->g, cd->box->b);
        }, LV_EVENT_CLICKED, cd);
    }    

    // Random Button
    /*
    lv_obj_t *btnRnd = lv_button_create(panel);
    lv_obj_set_size(btnRnd, btnSize, btnSize);
    lv_obj_set_pos(btnRnd, startX + 5 * (btnSize + btnGap), startY);
    lv_obj_set_style_bg_color(btnRnd, lv_color_hex(0x444466), 0);
    lv_obj_set_style_radius(btnRnd, LV_RADIUS_CIRCLE, 0);
    lv_obj_t *lblRnd = lv_label_create(btnRnd);
    lv_label_set_text(lblRnd, "R");
    lv_obj_center(lblRnd);
    lv_obj_add_event_cb(btnRnd, [](lv_event_t *e) {
        WledBox *b = (WledBox*)lv_event_get_user_data(e);
        b->r = random(256); b->g = random(256); b->b = random(256);
        lv_slider_set_value(b->sliderR, b->r, LV_ANIM_OFF);
        lv_slider_set_value(b->sliderG, b->g, LV_ANIM_OFF);
        lv_slider_set_value(b->sliderB, b->b, LV_ANIM_OFF);
        updatePreview(*b);
        wledSendColor(*b);
    }, LV_EVENT_CLICKED, &box);
    */
}

// ================================================================
// Setup
// ================================================================
void uiWledSetup(lv_obj_t *tab) {
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(tab, 4, 0);
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

    s_boxes[0].inst = &wledConfig.innen;
    s_boxes[1].inst = &wledConfig.aussen;
// s_boxes[0].ip und s_boxes[1].ip entfernen
    
    lv_obj_t *p0 = makePanel(tab,   0, 4, 388, 392);
    lv_obj_t *p1 = makePanel(tab, 404, 4, 388, 392);
    
    buildBox(s_boxes[0], p0, "WLED Innen");
    buildBox(s_boxes[1], p1, "WLED Aussen");
    wledApplyState(s_boxes[0]);
    wledApplyState(s_boxes[1]);

}

// ================================================================
// Update – einfach nur daten übernehmen 
// ================================================================
void uiWledUpdate() {
    s_boxes[0].on  = wledConfig.innen.lastState;
    s_boxes[0].online = wledConfig.innen.online;
    s_boxes[0].bri = wledConfig.innen.bri;
    s_boxes[0].r   = wledConfig.innen.r;
    s_boxes[0].g   = wledConfig.innen.g;
    s_boxes[0].b   = wledConfig.innen.b;

    s_boxes[1].on  = wledConfig.aussen.lastState;
    s_boxes[1].online = wledConfig.aussen.online;
    s_boxes[1].bri = wledConfig.aussen.bri;
    s_boxes[1].r   = wledConfig.aussen.r;
    s_boxes[1].g   = wledConfig.aussen.g;
    s_boxes[1].b   = wledConfig.aussen.b;

    wledApplyState(s_boxes[0]);
    wledApplyState(s_boxes[1]);
}