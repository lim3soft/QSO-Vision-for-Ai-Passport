#pragma once
#include "lvgl.h"
#include "bm_protocol.h"
#include "qso_ui_math.h"
#include "wifi_info.h"

LV_FONT_DECLARE(qso_bold_12);
LV_FONT_DECLARE(qso_bold_11);
LV_FONT_DECLARE(qso_bold_14);
LV_FONT_DECLARE(qso_bold_28);
LV_FONT_DECLARE(qso_text_12);
LV_FONT_DECLARE(qso_text_14);
LV_FONT_DECLARE(qso_text_16);
LV_FONT_DECLARE(qso_text_11);

typedef enum {
    QSO_PAGE_MONITOR, QSO_PAGE_OPTIONS, QSO_PAGE_TALKGROUP,
    QSO_PAGE_WIFI, QSO_PAGE_PASSWORD, QSO_PAGE_CUSTOM, QSO_PAGE_ABOUT,
    QSO_PAGE_SCREEN_TIMEOUT, QSO_PAGE_BRIGHTNESS
} qso_page_t;

typedef struct {
    lv_obj_t *screen, *body, *title, *status, *selection, *onair, *info, *directory, *help;
    lv_obj_t *audio_icon, *wifi_icon, *battery_icon, *battery_text, *eq, *clock;
    lv_obj_t *history_heading, *data_panel;
    lv_timer_t *timer;
    uint8_t eq_levels[QSO_EQ_BARS];
    time_t clock_second;
    time_t (*clock_now)(void);
    qso_page_t page;
    bool live, wifi_connected, audio_connected, muted, charging, display_off;
    int battery_percent, menu_selected;
    int cursor_y;
    bm_qso_info_t history[5];
    size_t history_count;
    char history_empty[64];
    unsigned phase;
    int brightness, brightness_target;
    void (*backlight)(uint8_t percent);
} qso_ui_t;

bool qso_ui_create(qso_ui_t *ui);
void qso_ui_page(qso_ui_t *ui, qso_page_t page);
void qso_ui_status(qso_ui_t *ui, bool wifi, bool audio, bool muted, int battery, bool charging);
void qso_ui_live(qso_ui_t *ui, bool active);
void qso_ui_history(qso_ui_t *ui, const bm_qso_info_t *history, size_t count, const char *state);
void qso_ui_menu(qso_ui_t *ui);
void qso_ui_about(qso_ui_t *ui, const bm_wifi_info_t *wifi, int battery);
void qso_ui_about_scroll(qso_ui_t *ui, int direction);
void qso_ui_display_enabled(qso_ui_t *ui, bool enabled, unsigned awake_percent);
void qso_ui_set_backlight(qso_ui_t *ui, void (*callback)(uint8_t percent));

void qso_ui_brightness(qso_ui_t *ui, unsigned percent);
void qso_ui_history_position(qso_ui_t *ui, size_t offset, size_t total);
