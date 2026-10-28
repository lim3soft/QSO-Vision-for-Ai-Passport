#pragma once

#include "esp_err.h"
#include "esp_wifi_types.h"
#include "wifi_info.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    BM_WIFI_IDLE = 0,
    BM_WIFI_SCANNING,
    BM_WIFI_SCAN_READY,
    BM_WIFI_CONNECTING,
    BM_WIFI_CONNECTED,
    BM_WIFI_FAILED,
} bm_wifi_state_t;

esp_err_t bm_wifi_start(void);
esp_err_t bm_wifi_stop(void);
esp_err_t bm_wifi_scan(void);
esp_err_t bm_wifi_set_credentials(const char *ssid, const char *password);
bm_wifi_state_t bm_wifi_state(void);
const char *bm_wifi_state_text(void);
const char *bm_wifi_connected_ssid(void);
void bm_wifi_get_info(bm_wifi_info_t *out);
void bm_wifi_service(void);
/* Suspend saved-network selection while the Wi-Fi list/editor owns the radio.
 * Leaving setup resumes automatic recovery if there is no active connection. */
void bm_wifi_set_setup_active(bool active);
const char *bm_wifi_last_error(void);
const char *bm_wifi_failed_ssid(void);
bool bm_wifi_has_credentials(void);
bool bm_wifi_get_saved_credentials(const char *ssid, char *password, size_t capacity);
size_t bm_wifi_saved_profile_count(void);
bool bm_wifi_get_saved_profile(size_t index, char *ssid, size_t ssid_capacity);
esp_err_t bm_wifi_delete_profile(const char *ssid);
bool bm_wifi_scan_ready(void);
size_t bm_wifi_get_scan_records(wifi_ap_record_t *records, size_t capacity,
                                uint16_t *total);
