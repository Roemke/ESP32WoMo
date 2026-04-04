#include "ui_details.h"
#include "sensorpoll.h"
#include "appconfig.h"

// ----------------------------------------------------------------
// Private State
// ----------------------------------------------------------------
static lv_obj_t  *s_table    = nullptr;
//static lv_obj_t  *s_roller   = nullptr;
static lv_obj_t  *s_dropdown = nullptr;
static uint32_t   s_hours    = 12;

// Spalten-Indizes
#define COL_NAME    0
#define COL_ACTUAL  1
#define COL_MIN     2
#define COL_MAX     3
#define COL_AVG     4

// Zeilen-Indizes
#define ROW_HEADER  0
#define ROW_TEMP    1
#define ROW_HUM     2
#define ROW_PRESS   3
#define ROW_CO2     4
#define ROW_VOLT    5
#define ROW_CURR    6
#define ROW_SOC     7
#define ROW_POWER   8
#define ROW_VS      9


#define ROW_MPPT1_V   10
#define ROW_MPPT1_PV  11
#define ROW_MPPT2_V   12
#define ROW_MPPT2_PV  13
#define ROW_CHARGER_V   14
#define ROW_CHARGER_I   15
#define ROW_COUNT       16  

// ----------------------------------------------------------------
// Hilfsfunktion: Zelle setzen
// ----------------------------------------------------------------
static void setCell(int row, int col, const char *text)
{
    lv_table_set_cell_value(s_table, row, col, text);
}

static void setCellF(int row, int col, float val, int dec, const char *unit)
{
    char buf[24];
    snprintf(buf, sizeof(buf), "%.*f %s", dec, val, unit);
    setCell(row, col, buf);
}

static void setCellI(int row, int col, int val, const char *unit)
{
    char buf[24];
    snprintf(buf, sizeof(buf), "%d %s", val, unit);
    setCell(row, col, buf);
}

// ----------------------------------------------------------------
// Setup
// ----------------------------------------------------------------
void uiDetailsSetup(lv_obj_t *tab)
{
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(tab, 4, 0);
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

    // ---- Stunden-Auswahl oben ----
    lv_obj_t *ctrl = lv_obj_create(tab);
    lv_obj_set_size(ctrl, 792, 60);
    lv_obj_set_pos(ctrl, 0, 0);
    lv_obj_set_style_bg_color(ctrl, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_bg_opa(ctrl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ctrl, lv_color_hex(0x3A3A8E), 0);
    lv_obj_set_style_border_width(ctrl, 1, 0);
    lv_obj_set_style_radius(ctrl, 8, 0);
    lv_obj_set_style_pad_all(ctrl, 4, 0);
    lv_obj_clear_flag(ctrl, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(ctrl);
    lv_label_set_text(lbl, "Statistik letzte:");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x8888BB), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl, 8, 14);

    s_dropdown = lv_dropdown_create(ctrl);
        // Maximale Stunden berechnen
    uint32_t maxHours = (uint32_t)((uint64_t)RING_MAX_ENTRIES * 
                        appConfig.sensor_poll_interval_ms / 1000 / 3600);

    char opts[64];
    snprintf(opts, sizeof(opts), "1h\n4h\n8h\n12h\n16h\n24h\nMax(%luh)", maxHours);
    lv_dropdown_set_options(s_dropdown, opts);
  
    lv_dropdown_set_selected(s_dropdown, 3); // 12h = index 3
    lv_obj_set_width(s_dropdown, 100);
    lv_obj_set_pos(s_dropdown, 200, 10);
    lv_obj_set_style_bg_color(s_dropdown, lv_color_hex(0x2255CC), 0);
    lv_obj_set_style_text_color(s_dropdown, lv_color_hex(0xEEEEFF), 0);
    lv_obj_set_style_text_font(s_dropdown, &lv_font_montserrat_14, 0);

    lv_obj_add_event_cb(s_dropdown, [](lv_event_t *e)
    { //im event muss ich maxhours neu berechnen
        uint16_t sel = lv_dropdown_get_selected(s_dropdown);
        uint32_t maxHours = (uint32_t)((uint64_t)RING_MAX_ENTRIES * 
                        appConfig.sensor_poll_interval_ms / 1000 / 3600);
        const uint32_t hours[] = {1, 4, 8, 12, 16, 24, maxHours};
        s_hours = hours[sel];
        calcRingStats(s_hours);
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    // ---- Tabelle ----
    s_table = lv_table_create(tab);
    lv_obj_set_pos(s_table, 0, 64);
    lv_obj_set_size(s_table, 792, 326);

    // Spaltenbreiten
    lv_table_set_column_width(s_table, COL_NAME,   180);
    lv_table_set_column_width(s_table, COL_ACTUAL, 150);
    lv_table_set_column_width(s_table, COL_MIN,    150);
    lv_table_set_column_width(s_table, COL_MAX,    150);
    lv_table_set_column_width(s_table, COL_AVG,    150);

    // Hintergrund
    lv_obj_set_style_bg_color(s_table, lv_color_hex(0x16213E), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_table, lv_color_hex(0x1A1A2E), LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_table, lv_color_hex(0xEEEEFF), LV_PART_ITEMS);
    lv_obj_set_style_border_color(s_table, lv_color_hex(0x3A3A8E), LV_PART_ITEMS);

    // Header
    setCell(ROW_HEADER, COL_NAME,   "Kanal");
    setCell(ROW_HEADER, COL_ACTUAL, "Aktuell");
    setCell(ROW_HEADER, COL_MIN,    "Min");
    setCell(ROW_HEADER, COL_MAX,    "Max");
    setCell(ROW_HEADER, COL_AVG,    "Avg");

    // Zeilenbeschriftungen
    setCell(ROW_TEMP,  COL_NAME, "Temperatur");
    setCell(ROW_HUM,   COL_NAME, "Feuchte");
    setCell(ROW_PRESS, COL_NAME, "Luftdruck");
    setCell(ROW_CO2,   COL_NAME, "CO2");
    setCell(ROW_VOLT,  COL_NAME, "Spannung");
    setCell(ROW_CURR,  COL_NAME, "Strom");
    setCell(ROW_SOC,   COL_NAME, "SoC");
    setCell(ROW_POWER, COL_NAME, "Leistung");
    setCell(ROW_VS,    COL_NAME, "Starter");
    setCell(ROW_MPPT1_V,  COL_NAME, "MPPT1 Span.");
    setCell(ROW_MPPT1_PV, COL_NAME, "MPPT1 PV");
    setCell(ROW_MPPT2_V,  COL_NAME, "MPPT2 Span.");
    setCell(ROW_MPPT2_PV, COL_NAME, "MPPT2 PV");
    setCell(ROW_CHARGER_V, COL_NAME, "Charger V");
    setCell(ROW_CHARGER_I, COL_NAME, "Charger I");

    // Header-Zeile stylen
    for (int col = 0; col < 5; col++)
    {
        lv_table_set_cell_user_data(s_table, ROW_HEADER, col, (void*)1);
    }

    // Scroll erlauben
    lv_obj_add_flag(s_table, LV_OBJ_FLAG_SCROLLABLE);
}

// ----------------------------------------------------------------
// Update – aus loop() aufrufen
// ----------------------------------------------------------------
void uiDetailsUpdate(bool force)
{
    static uint32_t lastMs = 0;
    if (!force && millis() - lastMs < 2000) return;
    lastMs = millis();

    if (!s_table) return;

    // ---- Aktuelle Werte ----
    if (sensorData.bme_valid) {
        setCellF(ROW_TEMP,  COL_ACTUAL, sensorData.temperature, 1, "C");
        setCellF(ROW_HUM,   COL_ACTUAL, sensorData.humidity,    1, "%");
        setCellF(ROW_PRESS, COL_ACTUAL, sensorData.pressure,    1, "hPa");
    }
    if (sensorData.co2_valid)
        setCellI(ROW_CO2, COL_ACTUAL, sensorData.co2_ppm, "ppm");

    if (sensorData.vedirect_valid) {
        setCellF(ROW_VOLT,  COL_ACTUAL, sensorData.voltage,         2, "V");
        setCellF(ROW_CURR,  COL_ACTUAL, sensorData.current,         2, "A");
        setCellF(ROW_SOC,   COL_ACTUAL, sensorData.soc,             1, "%");
        setCellF(ROW_POWER, COL_ACTUAL, sensorData.power,           1, "W");
        setCellF(ROW_VS,    COL_ACTUAL, sensorData.voltage_starter, 2, "V");
    }
    if (sensorData.mppt1_valid) {
        setCellF(ROW_MPPT1_V,  COL_ACTUAL, sensorData.mppt1_voltage,  2, "V");
        setCellF(ROW_MPPT1_PV, COL_ACTUAL, sensorData.mppt1_pv_power, 1, "W");
    }
    if (sensorData.mppt2_valid) {
        setCellF(ROW_MPPT2_V,  COL_ACTUAL, sensorData.mppt2_voltage,  2, "V");
        setCellF(ROW_MPPT2_PV, COL_ACTUAL, sensorData.mppt2_pv_power, 1, "W");
    }
    if (sensorData.charger_valid) {
        setCellF(ROW_CHARGER_V, COL_ACTUAL, sensorData.charger_voltage, 2, "V");
        setCellF(ROW_CHARGER_I, COL_ACTUAL, sensorData.charger_current, 2, "A");
    }

    // ---- Stats ----
    if (!ringStats.valid) return;

    // Hilfsmakro: Zelle setzen oder --- wenn Sensor keine Daten hat
    auto statsRow = [&](int row, uint8_t flag, float mn, float mx, float avg, int dec, const char *unit) {
        if (ringStats.valid_sensors & flag) {
            setCellF(row, COL_MIN, mn,  dec, unit);
            setCellF(row, COL_MAX, mx,  dec, unit);
            setCellF(row, COL_AVG, avg, dec, unit);
        } else {
            setCell(row, COL_MIN, "---");
            setCell(row, COL_MAX, "---");
            setCell(row, COL_AVG, "---");
        }
    };
    auto statsRowI = [&](int row, uint8_t flag, int mn, int mx, int avg, const char *unit) {
        if (ringStats.valid_sensors & flag) {
            setCellI(row, COL_MIN, mn,  unit);
            setCellI(row, COL_MAX, mx,  unit);
            setCellI(row, COL_AVG, avg, unit);
        } else {
            setCell(row, COL_MIN, "---");
            setCell(row, COL_MAX, "---");
            setCell(row, COL_AVG, "---");
        }
    };

    statsRow(ROW_TEMP,  VALID_BME,     ringStats.t_min,   ringStats.t_max,   ringStats.t_avg,   1, "C");
    statsRow(ROW_HUM,   VALID_BME,     ringStats.h_min,   ringStats.h_max,   ringStats.h_avg,   1, "%");
    statsRow(ROW_PRESS, VALID_BME,     ringStats.p_min,   ringStats.p_max,   ringStats.p_avg,   1, "hPa");
    statsRowI(ROW_CO2,  VALID_CO2,     ringStats.co2_min, ringStats.co2_max, ringStats.co2_avg, "ppm");

    statsRow(ROW_VOLT,  VALID_VE,      ringStats.v_min,   ringStats.v_max,   ringStats.v_avg,   2, "V");
    statsRow(ROW_CURR,  VALID_VE,      ringStats.i_min,   ringStats.i_max,   ringStats.i_avg,   2, "A");
    statsRow(ROW_SOC,   VALID_VE,      ringStats.soc_min, ringStats.soc_max, ringStats.soc_avg, 1, "%");
    statsRow(ROW_POWER, VALID_VE,      ringStats.pw_min,  ringStats.pw_max,  ringStats.pw_avg,  1, "W");
    statsRow(ROW_VS,    VALID_VE,      ringStats.vs_min,  ringStats.vs_max,  ringStats.vs_avg,  2, "V");

    statsRow(ROW_MPPT1_V,  VALID_MPPT1, ringStats.mppt1_v_min,  ringStats.mppt1_v_max,  ringStats.mppt1_v_avg,  2, "V");
    statsRow(ROW_MPPT1_PV, VALID_MPPT1, ringStats.mppt1_pv_min, ringStats.mppt1_pv_max, ringStats.mppt1_pv_avg, 1, "W");
    statsRow(ROW_MPPT2_V,  VALID_MPPT2, ringStats.mppt2_v_min,  ringStats.mppt2_v_max,  ringStats.mppt2_v_avg,  2, "V");
    statsRow(ROW_MPPT2_PV, VALID_MPPT2, ringStats.mppt2_pv_min, ringStats.mppt2_pv_max, ringStats.mppt2_pv_avg, 1, "W");

    statsRow(ROW_CHARGER_V, VALID_CHARGER, ringStats.charger_v_min, ringStats.charger_v_max, ringStats.charger_v_avg, 2, "V");
    statsRow(ROW_CHARGER_I, VALID_CHARGER, ringStats.charger_i_min, ringStats.charger_i_max, ringStats.charger_i_avg, 2, "A");
}
