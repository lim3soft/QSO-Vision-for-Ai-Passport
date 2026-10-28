#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "talkgroup_model.h"
#include "wifi_monitor.h"
#include "hoseline_audio.h"
#include "qso_ui.h"
#include "qso_screen.h"
#include "qso_dimmer.h"
#include "qso_controls.h"
#include "lvgl.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "esp_timer.h"
#include <time.h>

static const char *TAG = "bm_monitor";

/* Do not reallocate/repaint static monitor labels every 500 ms. */
static void monitor_text(lv_obj_t *object, const char *text)
{
    if(strcmp(lv_label_get_text(object),text))lv_label_set_text(object,text);
}
static void monitor_text_fmt(lv_obj_t *object, const char *format, ...)
{
    char text[160];va_list args;va_start(args,format);
    vsnprintf(text,sizeof(text),format,args);va_end(args);
    monitor_text(object,text);
}

static lv_obj_t *s_screen;
static lv_obj_t *s_status, *s_tg, *s_selection, *s_directory, *s_help;
static lv_obj_t *s_onair, *s_qso_info;
static qso_ui_t s_ui;
static qso_screen_t s_screen_control;
static qso_dimmer_t s_dimmer;
static qso_controls_t s_monitor_controls;
static size_t s_history_offset, s_history_total;
static char s_history_anchor[64];
static int s_screen_settings, s_timeout_selected;
static unsigned s_pending_brightness;
static bool s_display_save_failed;
static bool s_volume_dirty;
static const qso_dim_mode_t TIMEOUT_MODES[]={QSO_DIM_1_MIN,QSO_DIM_5_MIN,
    QSO_DIM_10_MIN,QSO_DIM_SMART,QSO_DIM_NEVER};
static const char *TIMEOUT_LABELS[]={"After 1 minute","After 5 minutes",
    "After 10 minutes","Smart: idle 5 seconds","Never"};
_Static_assert((int)BSP_BTN_OK==QSO_KEY_OK &&
    (int)BSP_BTN_PRESS==(int)QSO_KEY_PRESS && (int)BSP_BTN_RELEASE==(int)QSO_KEY_RELEASE &&
    (int)BSP_BTN_CLICK==(int)QSO_KEY_CLICK && (int)BSP_BTN_DOUBLE==(int)QSO_KEY_DOUBLE &&
    (int)BSP_BTN_LONG==(int)QSO_KEY_LONG,"BSP key mapping changed");
static QueueHandle_t s_keys;
static int s_selected, s_talkgroup = 91, s_volume = 85;
static bool s_audio_ready, s_buttons_ready;
static bool s_wifi_screen, s_wifi_loaded, s_wifi_password_prompt;
static bool s_options_screen, s_tg_menu, s_about_screen;
static bm_wifi_info_t s_about_wifi;
static bool s_about_info_valid;
static int s_option_selected;
static bool s_muted;
static int s_pre_mute_volume = 85;
static bool s_custom_tg_prompt;
static char s_custom_tg[9];
static size_t s_custom_tg_length;
static int s_custom_digit;
static int s_wifi_selected;
static uint16_t s_wifi_total;
static size_t s_wifi_count;
#define WIFI_SCAN_CAPACITY 32
#define WIFI_VISIBLE_ROWS 5
static wifi_ap_record_t s_wifi_records[WIFI_SCAN_CAPACITY];
static char s_wifi_ssid[33];
#define WIFI_PASSWORD_MAX 25
static char s_wifi_password[WIFI_PASSWORD_MAX + 1];
static size_t s_wifi_password_length;
static size_t s_wifi_char_index;
static bool s_wifi_connect_requested;
static bm_wifi_state_t s_last_wifi_state = (bm_wifi_state_t)-1;
static bm_audio_state_t s_last_audio_state = (bm_audio_state_t)-1;
static bool s_last_wifi_screen;
static int s_battery_soc = -1;
static int64_t s_qso_started_us;
static volatile bool s_password_key_held;
static volatile bsp_btn_t s_password_held_button = BSP_BTN_OK;

static const char *WIFI_CHARSET =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    " !@#$%^&*()-_=+[]{}:;,.?/\\|";

static void refresh(void);

static void request_wifi_connection(void)
{
    if (s_wifi_connect_requested) return;
    ESP_LOGI(TAG, "Wi-Fi password confirmed for SSID %.32s", s_wifi_ssid);
    esp_err_t err = bm_wifi_set_credentials(s_wifi_ssid, s_wifi_password);
    if (err == ESP_OK) {
        s_wifi_connect_requested = true;
        if (bsp_lvgl_lock(250)) {
            lv_label_set_text(s_status, "CONNECTING...");
            bsp_lvgl_unlock();
        }
        ESP_LOGI(TAG, "Wi-Fi connect requested for SSID %.32s", s_wifi_ssid);
    } else if (bsp_lvgl_lock(250)) {
        ESP_LOGE(TAG, "Wi-Fi connection request failed for SSID %.32s: %s",
                 s_wifi_ssid, esp_err_to_name(err));
        lv_label_set_text_fmt(s_status, "Wi-Fi error: %s", esp_err_to_name(err));
        bsp_lvgl_unlock();
    }
}

static bool saved_ssid(const char *ssid)
{
    char password[65];
    return bm_wifi_get_saved_credentials(ssid, password, sizeof(password));
}

static void status_task(void *arg) {
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));
        bm_wifi_set_setup_active(s_wifi_screen);
        bm_wifi_service();
        bm_wifi_state_t state = bm_wifi_state();
        bm_audio_state_t audio_state = bm_audio_state();
        bm_network_service(state == BM_WIFI_CONNECTED);
        int battery_soc = bsp_battery_soc();
        s_battery_soc = battery_soc;
        if (s_about_screen) {
            bm_wifi_info_t info;
            bm_wifi_get_info(&info);
            if (bsp_lvgl_lock(250)) {
                s_about_wifi = info;
                s_about_info_valid = true;
                bsp_lvgl_unlock();
            }
        }
        if(s_wifi_connect_requested && state==BM_WIFI_FAILED) {
            s_wifi_connect_requested=false;
            s_wifi_password_prompt=false;
            s_wifi_loaded=s_wifi_count>0;
            if(s_wifi_screen && !s_wifi_loaded)(void)bm_wifi_scan();
        }
        if (s_wifi_screen && s_wifi_password_prompt && s_wifi_connect_requested &&
            state == BM_WIFI_CONNECTED) {
            /* Saved profiles connect without another password prompt. */
            s_wifi_screen = false;
            s_wifi_password_prompt = false;
            s_wifi_connect_requested = false;
            s_wifi_loaded = false;
            s_wifi_count = 0;
            s_wifi_total = 0;
        }
        bool needs_scan_render = s_wifi_screen && !s_wifi_loaded && bm_wifi_scan_ready();
        if (state != s_last_wifi_state || audio_state != s_last_audio_state ||
            s_wifi_screen != s_last_wifi_screen || needs_scan_render || true) {
            s_last_wifi_state = state;
            s_last_audio_state = audio_state;
            s_last_wifi_screen = s_wifi_screen;
            refresh();
        }
    }
}

static bool save_settings_values(qso_dim_mode_t mode, uint8_t brightness) {
    nvs_handle_t h;
    if(nvs_open("monitor",NVS_READWRITE,&h)!=ESP_OK)return false;
    bool ok=nvs_set_i32(h,"talkgroup",s_talkgroup)==ESP_OK &&
        nvs_set_i32(h,"volume",s_volume)==ESP_OK &&
        nvs_set_i32(h,"dim_mode",mode)==ESP_OK &&
        nvs_set_i32(h,"brightness",brightness)==ESP_OK &&
        nvs_commit(h)==ESP_OK;
    nvs_close(h);
    if(!ok)ESP_LOGW(TAG,"Settings could not be saved");
    return ok;
}
static bool save_settings(void) {
    return save_settings_values(s_dimmer.mode,s_dimmer.brightness_percent);
}
static void load_settings(void) {
    nvs_handle_t h; if (nvs_open("monitor", NVS_READONLY, &h) != ESP_OK) return;
    int32_t v; if (nvs_get_i32(h, "talkgroup", &v) == ESP_OK && bm_talkgroup_valid(v)) s_talkgroup = v;
    if (nvs_get_i32(h, "volume", &v) == ESP_OK && v >= 0 && v <= 100) {
        s_volume = v;
    }
    if(nvs_get_i32(h,"dim_mode",&v)==ESP_OK && v>=0 && v<QSO_DIM_MODE_COUNT)s_dimmer.mode=(qso_dim_mode_t)v;
    /* Legacy dim_level is intentionally ignored: all timeout modes now use 1%. */
    if(nvs_get_i32(h,"brightness",&v)==ESP_OK && v>=1 && v<=100)s_dimmer.brightness_percent=(uint8_t)v;
    nvs_close(h);
    bool preset_found = false;
    for (size_t i = 0; i < bm_preset_count; ++i) {
        if (bm_presets[i].id == s_talkgroup) {
            s_selected = (int)i;
            preset_found = true;
            break;
        }
    }
    if (!preset_found) s_selected = (int)bm_preset_count - 1;
}
static void render_content_locked(void) {
    if (!s_screen || !s_directory) return;
    if (s_wifi_screen) {
        lv_label_set_text(s_tg, s_wifi_password_prompt ? "Wi-Fi password" : "Wi-Fi networks");
        if (s_wifi_password_prompt) {
            lv_label_set_text_fmt(s_selection, "SSID: %.24s", s_wifi_records[s_wifi_selected].ssid);
            if (s_wifi_connect_requested && bm_wifi_state() == BM_WIFI_CONNECTED) {
                lv_label_set_text(s_directory, "WIFI CONNECTED");
                lv_label_set_text(s_help, "Long OK back to monitor");
                lv_label_set_text(s_status, "Connected successfully");
                return;
            }
            char password_line[100];
            snprintf(password_line, sizeof(password_line), "PASS: %s\nCHAR: [%c]",
                     s_wifi_password, WIFI_CHARSET[s_wifi_char_index]);
            lv_label_set_text(s_directory, password_line);
            lv_label_set_text_fmt(s_status, "Wi-Fi %s  %u chars",
                                  bm_wifi_state_text(), (unsigned)s_wifi_password_length);
            lv_label_set_text_fmt(s_help, "UP/DOWN char  OK add (%u/25)\n2xUP delete  2xOK connect",
                                  (unsigned)s_wifi_password_length);
            return;
        }
        if (bm_wifi_scan_ready() && !s_wifi_loaded) {
            s_wifi_count = bm_wifi_get_scan_records(s_wifi_records, WIFI_SCAN_CAPACITY,
                                                    &s_wifi_total);
            s_wifi_loaded = true;
            s_wifi_selected = 0;
            const char *saved = bm_wifi_connected_ssid();
            if (saved[0] != '\0') {
                size_t saved_index = s_wifi_count;
                for (size_t i = 0; i < s_wifi_count; ++i) {
                    if (!strcmp((const char *)s_wifi_records[i].ssid, saved)) {
                        saved_index = i;
                        break;
                    }
                }
                if (saved_index == s_wifi_count && s_wifi_count < WIFI_SCAN_CAPACITY) {
                    strncpy((char *)s_wifi_records[s_wifi_count].ssid, saved,
                            sizeof(s_wifi_records[s_wifi_count].ssid) - 1);
                    s_wifi_records[s_wifi_count].rssi = -127;
                    s_wifi_count++;
                }
                if (saved_index < s_wifi_count) s_wifi_selected = (int)saved_index;
            }
            /* Keep saved profiles visible even when they are not currently in range. */
            size_t saved_count = bm_wifi_saved_profile_count();
            for (size_t p = 0; p < saved_count && s_wifi_count < WIFI_SCAN_CAPACITY; ++p) {
                char saved_profile[33] = {0};
                if (!bm_wifi_get_saved_profile(p, saved_profile, sizeof(saved_profile))) continue;
                bool present = false;
                for (size_t i = 0; i < s_wifi_count; ++i)
                    if (!strcmp((const char *)s_wifi_records[i].ssid, saved_profile)) present = true;
                if (!present) {
                    strncpy((char *)s_wifi_records[s_wifi_count].ssid, saved_profile,
                            sizeof(s_wifi_records[s_wifi_count].ssid) - 1);
                    s_wifi_records[s_wifi_count].rssi = -127;
                    s_wifi_records[s_wifi_count].authmode = WIFI_AUTH_OPEN;
                    s_wifi_count++;
                }
            }
        }
        if (!s_wifi_loaded) {
            lv_label_set_text_fmt(s_selection, "Wi-Fi: %s", bm_wifi_state_text());
            lv_label_set_text(s_directory, "Please wait...");
        } else if (s_wifi_count == 0) {
            lv_label_set_text(s_selection, "No networks found");
            lv_label_set_text(s_directory, "OK: RESCAN");
        } else {
            lv_label_set_text_fmt(s_selection, "SELECT %d/%d: %s",
                                  s_wifi_selected + 1, (int)s_wifi_count,
                                  s_wifi_records[s_wifi_selected].ssid);
            char networks[256] = {0};
            size_t used = 0;
            size_t first = 0;
            if (s_wifi_selected >= WIFI_VISIBLE_ROWS) first = (size_t)s_wifi_selected - WIFI_VISIBLE_ROWS + 1;
            if (first + WIFI_VISIBLE_ROWS > s_wifi_count && s_wifi_count > WIFI_VISIBLE_ROWS)
                first = s_wifi_count - WIFI_VISIBLE_ROWS;
            size_t last = first + WIFI_VISIBLE_ROWS;
            if (last > s_wifi_count) last = s_wifi_count;
            for (size_t i = first; i < last; ++i) {
                bool saved = saved_ssid((const char *)s_wifi_records[i].ssid);
                int written = snprintf(networks + used, sizeof(networks) - used,
                                        "%c %.15s %d%s\n",
                                        i == (size_t)s_wifi_selected ? '>' : ' ',
                                        s_wifi_records[i].ssid, s_wifi_records[i].rssi,
                                        saved ? (s_wifi_records[i].rssi == -127 ?
                                                 " [SAVED/OFFLINE]" : " [SAVED]") : "");
                if (written < 0 || (size_t)written >= sizeof(networks) - used) break;
                used += (size_t)written;
            }
            lv_label_set_text(s_directory, networks);
        }
        if(bm_wifi_last_error()[0] && bm_wifi_state()!=BM_WIFI_CONNECTED)
            lv_label_set_text(s_status,bm_wifi_last_error());
        else lv_label_set_text_fmt(s_status, "Wi-Fi %s  %u APs", bm_wifi_state_text(), s_wifi_total);
        bool retry=s_wifi_count>0 && !strcmp((const char*)s_wifi_records[s_wifi_selected].ssid,
                                            bm_wifi_failed_ssid());
        lv_label_set_text_fmt(s_help,"UP/DOWN select  OK %s\n2xUP delete  2xDOWN edit",
                              retry?"edit password":"connect");
        return;
    }
    if (s_custom_tg_prompt) {
        lv_label_set_text(s_tg, "Custom talkgroup");
        lv_label_set_text(s_selection, "ENTER CUSTOM TG ID");
        lv_label_set_text_fmt(s_directory, "TG: %s\nDIGIT: %d", s_custom_tg,
                              s_custom_digit);
        lv_label_set_text(s_status, "Talkgroup range: 91-16777215");
        lv_label_set_text(s_help, "UP/DOWN digit  OK add\n2xUP delete  2xOK save");
        return;
    }
    if(s_screen_settings) {
        lv_label_set_text(s_tg,s_screen_settings==1?"Screen timeout":"Brightness");
        lv_label_set_text(s_status,s_display_save_failed?"Save failed. OK to retry":"AUDIO KEEPS PLAYING");
        if(s_screen_settings==1) {
            lv_label_set_text(s_selection,"Choose dimming rule");
            lv_label_set_text_fmt(s_directory,"%c %s\n%c %s\n%c %s\n%c %s\n%c %s",
                s_timeout_selected==0?'>':' ',TIMEOUT_LABELS[0],
                s_timeout_selected==1?'>':' ',TIMEOUT_LABELS[1],
                s_timeout_selected==2?'>':' ',TIMEOUT_LABELS[2],
                s_timeout_selected==3?'>':' ',TIMEOUT_LABELS[3],
                s_timeout_selected==4?'>':' ',TIMEOUT_LABELS[4]);
            lv_label_set_text(s_help,"UP/DOWN select  OK apply\nDIM TO 1%  HOLD OK back");
        } else {
            lv_label_set_text_fmt(s_selection,"BRIGHTNESS: %u%%",s_pending_brightness);
            lv_label_set_text(s_directory,"Awake brightness: 1-100%\n\nUP/DOWN adjust 1%\nHOLD UP/DOWN faster\nOK save permanently");
            lv_label_set_text(s_help,"Timeout dims to 1%, wakes to this\nHOLD OK cancel unsaved changes");
        }
        return;
    }
    if (s_about_screen) {
        qso_ui_about(&s_ui,s_about_info_valid?&s_about_wifi:NULL,s_battery_soc);
        return;
    }
    if (s_options_screen) {
        lv_label_set_text(s_tg, "Options");
        lv_label_set_text(s_selection, "Choose a setting");
        lv_label_set_text_fmt(s_directory,
            "%c Change talkgroup\n%c Wi-Fi connection\n%c Screen brightness\n%c Screen timeout\n%c About",
            s_option_selected == 0 ? '>' : ' ',
            s_option_selected == 1 ? '>' : ' ',
            s_option_selected == 2 ? '>' : ' ',
            s_option_selected == 3 ? '>' : ' ',
            s_option_selected == 4 ? '>' : ' ');
        lv_label_set_text(s_status, "QSO VISION SETTINGS");
        lv_label_set_text(s_help, "UP/DOWN select  OK open\nLONG OK back");
        return;
    }
    if (s_tg_menu) {
        lv_label_set_text(s_tg, "Talkgroups");
        lv_label_set_text_fmt(s_selection, "CURRENT: TG %d", s_talkgroup);
        char list[256] = {0};
        size_t used = 0;
        for (size_t i = 0; i < bm_preset_count; ++i) {
            int written = snprintf(list + used, sizeof(list) - used, "%c %s\n",
                                    i == (size_t)s_selected ? '>' : ' ', bm_presets[i].label);
            if (written < 0 || (size_t)written >= sizeof(list) - used) break;
            used += (size_t)written;
        }
        lv_label_set_text(s_directory, list);
        lv_label_set_text(s_status, "PRESET DIRECTORY");
        lv_label_set_text(s_help, "UP/DOWN select  OK apply\nLONG OK back");
        return;
    }
    const bm_talkgroup_t *candidate = &bm_presets[s_selected];
    (void)candidate;
    bool on_air = bm_audio_is_active();
    bm_qso_info_t current = {0};
    bm_qso_info_t history[BM_QSO_VISIBLE_ROWS] = {0};
    size_t history_count=bm_qso_get_history_page(history,BM_QSO_VISIBLE_ROWS,
        &s_history_offset,&s_history_total,s_history_anchor);
    if(s_history_offset && history_count)snprintf(s_history_anchor,sizeof(s_history_anchor),"%s",history[0].session);
    else s_history_anchor[0]=0;
    bool metadata_on_air = bm_qso_get_current(&current) && current.active;
    if (metadata_on_air) on_air = true;
    if (on_air && s_qso_started_us == 0) s_qso_started_us = esp_timer_get_time();
    if (!on_air && s_qso_started_us != 0 &&
        esp_timer_get_time() - s_qso_started_us > 2000000) s_qso_started_us = 0;
    monitor_text_fmt(s_tg, "MONITORING TALK GROUP %d", s_talkgroup);
    if (on_air) monitor_text(s_selection, metadata_on_air ? current.callsign : "RECEIVING");
    else monitor_text_fmt(s_selection, "TG %d", s_talkgroup);
    monitor_text(s_onair, on_air ? "CURRENTLY ON AIR" : "WAITING FOR QSO");
    if (on_air && metadata_on_air) {
        unsigned long seconds = current.elapsed_seconds;
        monitor_text_fmt(s_qso_info, "%.30s\n%s . %02lu:%02lu",
            current.name[0] ? current.name : "Name unavailable",
            current.country[0] ? current.country : "Country unavailable", seconds/60, seconds%60);
    } else if (on_air && s_qso_started_us != 0)
        monitor_text_fmt(s_qso_info, "Caller metadata pending  %lus",
                              (unsigned long)((esp_timer_get_time() - s_qso_started_us) / 1000000));
    else
        monitor_text(s_qso_info, "No active transmission");
    qso_ui_live(&s_ui,on_air);
    qso_ui_current_country(&s_ui,on_air && metadata_on_air?current.country:NULL);
    qso_ui_history(&s_ui,history,history_count,bm_metadata_state_text());
    qso_ui_history_position(&s_ui,s_history_offset,s_history_total);
    monitor_text(s_status, "");
    if(s_muted)monitor_text(s_help,"MUTED");
    else monitor_text_fmt(s_help,"VOL-%d%%",s_volume);
}
static void refresh_locked(void)
{
    if (!s_screen || !s_directory) return;
    bool monitor=!s_wifi_screen && !s_custom_tg_prompt && !s_options_screen &&
        !s_tg_menu && !s_about_screen && !s_screen_settings;
    unsigned level=qso_dimmer_level(&s_dimmer,esp_timer_get_time(),bm_audio_is_active(),monitor);
    qso_ui_brightness(&s_ui,s_screen_settings==2?s_pending_brightness:level);
    if(s_ui.display_off)return;
    qso_page_t page=s_wifi_screen ? (s_wifi_password_prompt?QSO_PAGE_PASSWORD:QSO_PAGE_WIFI) :
        s_custom_tg_prompt?QSO_PAGE_CUSTOM:s_options_screen?QSO_PAGE_OPTIONS:
        s_tg_menu?QSO_PAGE_TALKGROUP:s_about_screen?QSO_PAGE_ABOUT:
        s_screen_settings==1?QSO_PAGE_SCREEN_TIMEOUT:s_screen_settings==2?QSO_PAGE_BRIGHTNESS:QSO_PAGE_MONITOR;
    qso_ui_page(&s_ui,page);
    render_content_locked();
    qso_ui_menu(&s_ui);
    bm_audio_state_t state=bm_audio_state();
    qso_ui_status(&s_ui,bm_wifi_state()==BM_WIFI_CONNECTED,
        state==BM_AUDIO_CONNECTED||state==BM_AUDIO_PLAYING,s_muted||s_volume==0,s_battery_soc,false);
}
static void refresh(void) {
    if (!bsp_lvgl_lock(250)) return;
    refresh_locked();
    bsp_lvgl_unlock();
}
static void build_ui(void) {
    if (!qso_ui_create(&s_ui)) {
        ESP_LOGE(TAG,"UI allocation failed");
        return;
    }
    s_screen=s_ui.screen; s_tg=s_ui.title; s_status=s_ui.status;
    s_ui.brightness=s_ui.brightness_target=(int)qso_dimmer_awake_level(&s_dimmer);
    qso_ui_set_backlight(&s_ui,bsp_display_backlight);
    s_selection=s_ui.selection; s_onair=s_ui.onair; s_qso_info=s_ui.info;
    s_directory=s_ui.directory; s_help=s_ui.help;
    refresh_locked();
}
static void on_button(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    (void)user;
    if (!s_keys) return;
    if (ev == BSP_BTN_PRESS || ev == BSP_BTN_CLICK || ev == BSP_BTN_DOUBLE ||
        ev == BSP_BTN_LONG || ev == BSP_BTN_RELEASE) {
        uint8_t c = (uint8_t)btn | ((uint8_t)ev << 4);
        (void)xQueueSend(s_keys, &c, 0);
    }
}
static void password_repeat_task(void *arg) {
    (void)arg;
    int ticks = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(80));
        if (!s_password_key_held || !s_wifi_screen || !s_wifi_password_prompt ||
            (s_password_held_button != BSP_BTN_UP && s_password_held_button != BSP_BTN_DOWN)) {
            ticks = 0;
            continue;
        }
        if (++ticks < 4) continue;
        size_t count = strlen(WIFI_CHARSET);
        if (s_password_held_button == BSP_BTN_UP) {
            s_wifi_char_index = (s_wifi_char_index + count - 1) % count;
        } else {
            s_wifi_char_index = (s_wifi_char_index + 1) % count;
        }
        refresh();
    }
}
static bool is_monitor(void)
{
    return !s_options_screen && !s_tg_menu && !s_about_screen && !s_wifi_screen &&
        !s_custom_tg_prompt && !s_screen_settings;
}
static void monitor_control(qso_control_action_t action)
{
    if(action==QSO_HISTORY_NEWER || action==QSO_HISTORY_OLDER) {
        if(bsp_lvgl_lock(250)) {
            if(action==QSO_HISTORY_NEWER && s_history_offset)s_history_offset--;
            if(action==QSO_HISTORY_OLDER && s_history_total>BM_QSO_VISIBLE_ROWS &&
                s_history_offset<s_history_total-BM_QSO_VISIBLE_ROWS)s_history_offset++;
            s_history_anchor[0]=0;
            bsp_lvgl_unlock();
        }
    } else if(action==QSO_MUTE_TOGGLE) {
        if(!s_muted) {s_pre_mute_volume=s_volume>0?s_volume:85;s_volume=0;s_muted=true;}
        else {s_volume=s_pre_mute_volume>0?s_pre_mute_volume:85;s_muted=false;}
        if(s_audio_ready)bsp_audio_set_volume((uint8_t)s_volume);
        save_settings();
    } else if(action==QSO_VOLUME_UP || action==QSO_VOLUME_DOWN) {
        int volume=s_volume+(action==QSO_VOLUME_UP?5:-5);
        if(volume<0)volume=0;
        if(volume>100)volume=100;
        if(volume!=s_volume) {
            s_volume=volume;s_muted=volume==0;s_volume_dirty=true;
            if(s_audio_ready)bsp_audio_set_volume((uint8_t)s_volume);
        }
    }
}
static void brightness_adjust(int delta)
{
    if(!bsp_lvgl_lock(250))return;
    int value=(int)s_pending_brightness+delta;
    if(value<1)value=1;
    if(value>100)value=100;
    s_pending_brightness=(unsigned)value;
    s_display_save_failed=false;
    qso_dimmer_input(&s_dimmer,esp_timer_get_time());
    refresh_locked();
    bsp_lvgl_unlock();
}
static bool save_display_settings(void)
{
    if(!bsp_lvgl_lock(250))return false;
    qso_dim_mode_t mode=s_screen_settings==1?TIMEOUT_MODES[s_timeout_selected]:s_dimmer.mode;
    uint8_t brightness=s_screen_settings==2?(uint8_t)s_pending_brightness:s_dimmer.brightness_percent;
    bsp_lvgl_unlock();
    /* Persist only on explicit OK, outside the graphics lock. A failed write
     * leaves the saved rule unchanged and the editor available for retry. */
    bool ok=save_settings_values(mode,brightness);
    if(!bsp_lvgl_lock(1000))return false;
    if(ok) {
        s_dimmer.mode=mode;s_dimmer.brightness_percent=brightness;
        qso_dimmer_input(&s_dimmer,esp_timer_get_time());
    }
    s_display_save_failed=!ok;
    bsp_lvgl_unlock();
    return ok;
}
static void key_task(void *arg) {
    (void)arg; uint8_t c;
    for (;;) {
        if(!xQueueReceive(s_keys,&c,pdMS_TO_TICKS(50))) {
            qso_control_action_t repeat=qso_controls_tick(&s_monitor_controls,esp_timer_get_time(),
                (is_monitor() || s_screen_settings==2) && !s_screen_control.off && !s_screen_control.consuming_wake);
            if(repeat!=QSO_CONTROL_NONE) {
                if(s_screen_settings==2)brightness_adjust(repeat==QSO_VOLUME_UP?5:-5);
                else monitor_control(repeat);
                if(bsp_lvgl_lock(250)) {qso_dimmer_input(&s_dimmer,esp_timer_get_time());refresh_locked();bsp_lvgl_unlock();}
            }
            continue;
        }
        bsp_btn_t b=c&0xf; bsp_btn_ev_t e=c>>4;
        if(!s_screen)continue;
        bool monitor=is_monitor();
        if(e==BSP_BTN_RELEASE && s_volume_dirty) {save_settings();s_volume_dirty=false;}
        if(e==BSP_BTN_RELEASE)s_monitor_controls.held=0;
        qso_screen_t prior=s_screen_control;
        qso_screen_action_t action=qso_screen_key(&s_screen_control,(unsigned)b,
                                                 (qso_key_event_t)e,esp_timer_get_time(),monitor);
        if(action==QSO_SCREEN_IGNORE)continue;
        if(bsp_lvgl_lock(250)) {
            qso_dimmer_input(&s_dimmer,esp_timer_get_time());
            if(action!=QSO_SCREEN_OFF)qso_ui_brightness(&s_ui,
                s_screen_settings==2?s_pending_brightness:qso_dimmer_awake_level(&s_dimmer));
            bsp_lvgl_unlock();
        }
        if(action==QSO_SCREEN_OFF || action==QSO_SCREEN_WAKE) {
            s_monitor_controls.held=0;
            if(bsp_lvgl_lock(1000)) {
                bool enabled=action==QSO_SCREEN_WAKE;
                qso_ui_display_enabled(&s_ui,enabled,qso_dimmer_awake_level(&s_dimmer));
                /* Backlight only: no CPU sleep, codec shutdown or Wi-Fi change. */
                if(enabled)refresh_locked();
                bsp_lvgl_unlock();
            } else s_screen_control=prior;
            continue;
        }
        qso_control_action_t control=qso_controls_key(&s_monitor_controls,(unsigned)b,
            (qso_key_event_t)e,esp_timer_get_time(),monitor || s_screen_settings==2);
        if(monitor && e==BSP_BTN_CLICK && b==BSP_BTN_OK) {
            s_options_screen=true;
        } else if (e == BSP_BTN_LONG && b == BSP_BTN_OK) {
            if(s_screen_settings) {
                s_screen_settings=0;s_options_screen=true;s_display_save_failed=false;
            } else if (s_custom_tg_prompt) {
                s_custom_tg_prompt = false;
                s_custom_tg_length = 0;
                s_custom_tg[0] = '\0';
            } else if (s_wifi_screen && s_wifi_password_prompt) {
                if (s_wifi_connect_requested && bm_wifi_state() == BM_WIFI_CONNECTED) {
                    // The worker saved the confirmed profile; leave
                    // the connected profile active and return to the monitor screen.
                    s_wifi_screen = false;
                    s_wifi_password_prompt = false;
                } else {
                    s_wifi_password_prompt = false;
                }
            } else if (s_wifi_screen) {
                s_wifi_screen = false;
                s_options_screen = true;
                s_wifi_loaded = false;
                s_wifi_count = 0;
                s_wifi_total = 0;
            } else if (s_about_screen) {
                s_about_screen = false;
                s_options_screen = true;
            } else if (s_tg_menu) {
                s_tg_menu = false;
                s_options_screen = true;
            } else if (s_options_screen) {
                s_options_screen = false;
            } else {
                s_options_screen = true;
            }
            s_wifi_loaded = false;
            s_wifi_password_length = 0;
            s_wifi_password[0] = '\0';
            s_wifi_connect_requested = false;
            s_wifi_count = 0;
            s_wifi_total = 0;
            if (s_wifi_screen && !s_wifi_password_prompt) (void)bm_wifi_scan();
        } else if (e == BSP_BTN_PRESS && s_wifi_screen && s_wifi_password_prompt &&
                   (b == BSP_BTN_UP || b == BSP_BTN_DOWN)) {
            s_password_held_button = b;
            s_password_key_held = true;
        } else if (e == BSP_BTN_RELEASE && s_password_key_held && b == s_password_held_button) {
            s_password_key_held = false;
        } else if (s_wifi_screen && s_wifi_password_prompt && b == BSP_BTN_OK && e == BSP_BTN_LONG) {
            // Long OK is reserved for Back; handled above.
        } else if (s_wifi_screen && s_wifi_password_prompt && b == BSP_BTN_UP && e == BSP_BTN_CLICK) {
            size_t count = strlen(WIFI_CHARSET);
            s_wifi_char_index = (s_wifi_char_index + count - 1) % count;
        } else if (s_wifi_screen && s_wifi_password_prompt && b == BSP_BTN_DOWN && e == BSP_BTN_CLICK) {
            s_wifi_char_index = (s_wifi_char_index + 1) % strlen(WIFI_CHARSET);
        } else if (s_wifi_screen && s_wifi_password_prompt && b == BSP_BTN_UP && e == BSP_BTN_DOUBLE) {
            if (s_wifi_password_length > 0) {
                s_wifi_password[--s_wifi_password_length] = '\0';
            }
        } else if (s_wifi_screen && s_wifi_password_prompt && b == BSP_BTN_OK && e == BSP_BTN_CLICK) {
            if (!s_wifi_connect_requested && s_wifi_password_length < WIFI_PASSWORD_MAX) {
                s_wifi_password[s_wifi_password_length++] = WIFI_CHARSET[s_wifi_char_index];
                s_wifi_password[s_wifi_password_length] = '\0';
            }
        } else if (s_wifi_screen && s_wifi_password_prompt && b == BSP_BTN_OK && e == BSP_BTN_DOUBLE) {
            request_wifi_connection();
        } else if (s_wifi_screen && b == BSP_BTN_UP && e == BSP_BTN_CLICK && s_wifi_count > 0) {
            s_wifi_selected = (s_wifi_selected + (int)s_wifi_count - 1) % (int)s_wifi_count;
        } else if (s_wifi_screen && b == BSP_BTN_DOWN && e == BSP_BTN_CLICK && s_wifi_count > 0) {
            s_wifi_selected = (s_wifi_selected + 1) % (int)s_wifi_count;
        } else if (s_wifi_screen && !s_wifi_password_prompt && b == BSP_BTN_UP && e == BSP_BTN_DOUBLE) {
            if (s_wifi_count > 0 && saved_ssid((const char *)s_wifi_records[s_wifi_selected].ssid)) {
                esp_err_t err = bm_wifi_delete_profile((const char *)s_wifi_records[s_wifi_selected].ssid);
                if (err == ESP_OK) {
                    s_wifi_loaded = false;
                    (void)bm_wifi_scan();
                }
            }
        } else if (s_wifi_screen && !s_wifi_password_prompt && b == BSP_BTN_DOWN && e == BSP_BTN_DOUBLE) {
            if (s_wifi_count > 0 && saved_ssid((const char *)s_wifi_records[s_wifi_selected].ssid) &&
                bsp_lvgl_lock(250)) {
                strncpy(s_wifi_ssid, (const char *)s_wifi_records[s_wifi_selected].ssid,
                        sizeof(s_wifi_ssid) - 1);
                s_wifi_ssid[sizeof(s_wifi_ssid) - 1] = '\0';
                s_wifi_password_length = 0;
                s_wifi_password[0] = '\0';
                (void)bm_wifi_get_saved_credentials(s_wifi_ssid, s_wifi_password,
                                                     sizeof(s_wifi_password));
                s_wifi_password_length = strlen(s_wifi_password);
                s_wifi_char_index = 0;
                s_wifi_password_prompt = true;
                s_wifi_connect_requested = false;
                bsp_lvgl_unlock();
            }
        } else if (s_wifi_screen && b == BSP_BTN_OK && e == BSP_BTN_CLICK) {
            if (s_wifi_count > 0 && bsp_lvgl_lock(250)) {
                strncpy(s_wifi_ssid, (const char *)s_wifi_records[s_wifi_selected].ssid,
                        sizeof(s_wifi_ssid) - 1);
                s_wifi_ssid[sizeof(s_wifi_ssid) - 1] = '\0';
                s_wifi_password_length = 0;
                s_wifi_password[0] = '\0';
                s_wifi_char_index = 0;
                s_wifi_password_prompt = true;
                bool failed=!strcmp(s_wifi_ssid,bm_wifi_failed_ssid());
                if (!failed && bm_wifi_get_saved_credentials(s_wifi_ssid, s_wifi_password,
                                                  sizeof(s_wifi_password))) {
                    s_wifi_password_length = strlen(s_wifi_password);
                    esp_err_t err = bm_wifi_set_credentials(s_wifi_ssid, s_wifi_password);
                    if (err == ESP_OK) {
                        s_wifi_connect_requested = true;
                        ESP_LOGI(TAG, "Reconnecting to saved SSID %.32s", s_wifi_ssid);
                    } else {
                        s_wifi_password_prompt = false;
                        lv_label_set_text_fmt(s_status, "Wi-Fi error: %s", esp_err_to_name(err));
                    }
                } else if (!failed && s_wifi_records[s_wifi_selected].authmode == WIFI_AUTH_OPEN) {
                    /* Open AP: connect immediately without waiting for password entry. */
                    esp_err_t err = bm_wifi_set_credentials(s_wifi_ssid, "");
                    if (err == ESP_OK) {
                        s_wifi_connect_requested = true;
                    } else {
                        s_wifi_password_prompt = false;
                        lv_label_set_text_fmt(s_status, "Wi-Fi error: %s", esp_err_to_name(err));
                    }
                }
                refresh_locked();
                bsp_lvgl_unlock();
            } else if (s_wifi_count == 0) {
                s_wifi_loaded = false;
                (void)bm_wifi_scan();
            }
        } else if (s_wifi_screen) {
            // Ignore monitor controls while the Wi-Fi list is active.
        } else if (s_about_screen) {
            if ((b == BSP_BTN_UP || b == BSP_BTN_DOWN) && e == BSP_BTN_CLICK &&
                bsp_lvgl_lock(250)) {
                qso_ui_about_scroll(&s_ui,b==BSP_BTN_DOWN?1:-1);
                bsp_lvgl_unlock();
            }
        } else if(s_screen_settings) {
            if((b==BSP_BTN_UP || b==BSP_BTN_DOWN) && e==BSP_BTN_CLICK) {
                if(s_screen_settings==1) {
                    int dir=b==BSP_BTN_UP?-1:1;
                    s_timeout_selected=(s_timeout_selected+5+dir)%5;
                    s_display_save_failed=false;
                } else brightness_adjust(b==BSP_BTN_UP?1:-1);
            } else if(s_screen_settings==2 && (control==QSO_VOLUME_UP || control==QSO_VOLUME_DOWN)) {
                brightness_adjust(control==QSO_VOLUME_UP?5:-5);
            } else if(b==BSP_BTN_OK && e==BSP_BTN_CLICK && save_display_settings()) {
                s_screen_settings=0;s_options_screen=true;
            }
        } else if (s_options_screen && b == BSP_BTN_UP && e == BSP_BTN_CLICK) {
            s_option_selected = (s_option_selected + 4) % 5;
        } else if (s_options_screen && b == BSP_BTN_DOWN && e == BSP_BTN_CLICK) {
            s_option_selected = (s_option_selected + 1) % 5;
        } else if (s_options_screen && b == BSP_BTN_OK && e == BSP_BTN_CLICK) {
            if (s_option_selected == 0) {
                s_tg_menu = true;
                s_options_screen = false;
            } else if (s_option_selected == 1) {
                s_wifi_screen = true;
                s_options_screen = false;
                s_wifi_loaded = false;
                s_wifi_password_prompt = false;
                (void)bm_wifi_scan();
            } else if(s_option_selected==2 || s_option_selected==3) {
                s_screen_settings=s_option_selected==2?2:1;s_options_screen=false;
                s_pending_brightness=qso_dimmer_awake_level(&s_dimmer);s_display_save_failed=false;
                for(int i=0;i<5;++i)if(TIMEOUT_MODES[i]==s_dimmer.mode)s_timeout_selected=i;
            } else {
                s_about_screen = true;
                s_about_info_valid = false;
                s_options_screen = false;
            }
        } else if (s_tg_menu && b == BSP_BTN_UP && e == BSP_BTN_CLICK) {
            s_selected = (s_selected + (int)bm_preset_count - 1) % (int)bm_preset_count;
        } else if (s_tg_menu && b == BSP_BTN_DOWN && e == BSP_BTN_CLICK) {
            s_selected = (s_selected + 1) % (int)bm_preset_count;
        } else if (s_tg_menu && b == BSP_BTN_OK && e == BSP_BTN_CLICK) {
            if (bm_presets[s_selected].id) {
                s_talkgroup = bm_presets[s_selected].id;
                s_history_offset=0;s_history_anchor[0]=0;
                save_settings();
                s_tg_menu = false;
                if (s_audio_ready) (void)bm_audio_select(s_talkgroup);
            } else {
                s_tg_menu = false;
                s_custom_tg_prompt = true;
                s_custom_tg_length = 0;
                s_custom_tg[0] = '\0';
                s_custom_digit = 0;
            }
        } else if (s_custom_tg_prompt && b == BSP_BTN_UP && e == BSP_BTN_CLICK) {
            s_custom_digit = (s_custom_digit + 9) % 10;
        } else if (s_custom_tg_prompt && b == BSP_BTN_DOWN && e == BSP_BTN_CLICK) {
            s_custom_digit = (s_custom_digit + 1) % 10;
        } else if (s_custom_tg_prompt && b == BSP_BTN_UP && e == BSP_BTN_DOUBLE) {
            if (s_custom_tg_length > 0)
                s_custom_tg[--s_custom_tg_length] = '\0';
        } else if (s_custom_tg_prompt && b == BSP_BTN_OK && e == BSP_BTN_CLICK) {
            if (s_custom_tg_length < sizeof(s_custom_tg) - 1) {
                s_custom_tg[s_custom_tg_length++] = (char)('0' + s_custom_digit);
                s_custom_tg[s_custom_tg_length] = '\0';
            }
        } else if (s_custom_tg_prompt && b == BSP_BTN_OK && e == BSP_BTN_DOUBLE) {
            int custom_id = atoi(s_custom_tg);
            if (s_custom_tg_length == 0 || !bm_talkgroup_valid(custom_id)) {
                if (bsp_lvgl_lock(250)) {
                    lv_label_set_text(s_status, "Invalid TG: minimum is 91");
                    bsp_lvgl_unlock();
                }
            } else {
                s_talkgroup = custom_id;
                s_history_offset=0;s_history_anchor[0]=0;
                save_settings();
                s_custom_tg_prompt = false;
                s_selected = (int)bm_preset_count - 1;
                if (s_audio_ready) (void)bm_audio_select(s_talkgroup);
            }
        } else if(monitor) {
            monitor_control(control);
        }
        refresh();
    }
}
void app_main(void) {
    /* Project default: Malaysia (UTC+08:00). Wire timestamps remain UTC. */
    setenv("TZ", "MYT-8", 1);
    tzset();
    ESP_LOGI(TAG,"BrandMeister monitor starting; Hoseline adapter requires ESP-IDF/device validation");
    qso_dimmer_init(&s_dimmer,esp_timer_get_time());
    nvs_flash_init(); load_settings(); bsp_i2c_init();
    esp_err_t wifi_err = bm_wifi_start();
    if (wifi_err != ESP_OK) ESP_LOGE(TAG, "Wi-Fi start failed: %s", esp_err_to_name(wifi_err));
    if (bsp_display_init()!=ESP_OK || !bsp_lvgl_init()) return;
    bsp_display_backlight((uint8_t)qso_dimmer_awake_level(&s_dimmer));
    esp_err_t battery_err = bsp_battery_init();
    if (battery_err != ESP_OK) ESP_LOGW(TAG, "Battery gauge unavailable: %s", esp_err_to_name(battery_err));
    s_audio_ready = bsp_audio_init()==ESP_OK;
    if (s_audio_ready) { bsp_audio_set_format(8000, 16, 1); bsp_audio_set_volume((uint8_t)s_volume); }
    s_keys=xQueueCreate(32,sizeof(uint8_t));
    xTaskCreate(key_task,"bm_keys",8192,NULL,5,NULL);
    xTaskCreate(password_repeat_task, "bm_pw_repeat", 4096, NULL, 3, NULL);
    s_buttons_ready = bsp_button_init(on_button,NULL) == ESP_OK;
    if (!s_buttons_ready) ESP_LOGE(TAG, "button initialization failed");
    if (bsp_lvgl_lock(1000)) { build_ui(); bsp_lvgl_unlock(); }
    // refresh_locked() formats the directory and calls several LVGL routines;
    // keep this task separate from the smaller button-event worker.
    xTaskCreate(status_task, "bm_status", 8192, NULL, 3, NULL);
    if (s_audio_ready) {
        esp_err_t audio_err = bm_audio_start(s_talkgroup);
        if (audio_err != ESP_OK) ESP_LOGE(TAG, "Hoseline audio start failed: %s",
                                          esp_err_to_name(audio_err));
    }
    ESP_LOGI(TAG,"presets=%u audio=%d",(unsigned)bm_preset_count,s_audio_ready);
}
