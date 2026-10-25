#include "wifi_monitor.h"
#include "wifi_info_reader.h"
#include "wifi_retry.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "bm_wifi";
#define WIFI_NVS_NAMESPACE "monitor"
#define WIFI_NVS_SSID "wifi_ssid"
#define WIFI_NVS_PASSWORD "wifi_password"
#define WIFI_NVS_LAST_SSID "wifi_last_ssid"
#define WIFI_PROFILE_MAX 8
#define WIFI_CONNECTED_BIT BIT0

static bm_wifi_state_t s_state = BM_WIFI_IDLE;
static esp_netif_t *s_netif;
static EventGroupHandle_t s_events;
static bool s_initialized;
static bool s_started;
static bool s_handler_registered;
static esp_event_handler_instance_t s_wifi_handler;
static esp_event_handler_instance_t s_ip_handler;
static char s_connected_ssid[33];
static bool s_link_connected;
static bool s_scan_ready;
static bool s_reconfiguring;
static bool s_sntp_initialized;
static portMUX_TYPE s_retry_mux = portMUX_INITIALIZER_UNLOCKED;
static wifi_retry_t s_retry;
static char s_last_error[48], s_failed_ssid[33];
static char s_pending_ssid[33], s_pending_password[65];
static bool s_save_pending, s_commit_pending;
static bool s_scan_requested, s_scan_active;
static int64_t s_scan_deadline;

static void set_state(bm_wifi_state_t state)
{
    s_state = state;
}

static void connection_failed(const char *message)
{
    snprintf(s_last_error,sizeof(s_last_error),"%s",message);
    snprintf(s_failed_ssid,sizeof(s_failed_ssid),"%s",s_connected_ssid);
    taskENTER_CRITICAL(&s_retry_mux);
    s_save_pending=s_commit_pending=false;
    memset(s_pending_password,0,sizeof(s_pending_password));
    taskEXIT_CRITICAL(&s_retry_mux);
    set_state(BM_WIFI_FAILED);
}

static bool read_value(const char *key, char *out, size_t capacity, bool require_nonempty)
{
    if (!out || capacity == 0) return false;
    out[0] = '\0';
    nvs_handle_t handle;
    if (nvs_open(WIFI_NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return false;
    size_t length = capacity;
    esp_err_t err = nvs_get_str(handle, key, out, &length);
    nvs_close(handle);
    return err == ESP_OK && (!require_nonempty || out[0] != '\0');
}

static bool read_credential(const char *key, char *out, size_t capacity)
{
    return read_value(key, out, capacity, strcmp(key, WIFI_NVS_SSID) == 0);
}

static bool read_profile(size_t index, char *ssid, size_t ssid_capacity,
                         char *password, size_t password_capacity)
{
    if (index >= WIFI_PROFILE_MAX || !ssid || !password) return false;
    char ssid_key[24], password_key[32];
    /* NVS keys are limited to 15 characters. Keep profile keys deliberately
     * short: p0_s / p0_p, etc. */
    snprintf(ssid_key, sizeof(ssid_key), "p%u_s", (unsigned)index);
    snprintf(password_key, sizeof(password_key), "p%u_p", (unsigned)index);
    return read_value(ssid_key, ssid, ssid_capacity, true) &&
           read_value(password_key, password, password_capacity, false);
}

static esp_err_t write_profile(nvs_handle_t handle, size_t index,
                               const char *ssid, const char *password)
{
    char ssid_key[24], password_key[32];
    snprintf(ssid_key, sizeof(ssid_key), "p%u_s", (unsigned)index);
    snprintf(password_key, sizeof(password_key), "p%u_p", (unsigned)index);
    esp_err_t err = nvs_set_str(handle, ssid_key, ssid);
    if (err == ESP_OK) err = nvs_set_str(handle, password_key, password);
    return err;
}

static int find_profile(const char *ssid, char *password, size_t password_capacity)
{
    char saved_ssid[33], saved_password[65];
    for (size_t i = 0; i < WIFI_PROFILE_MAX; ++i) {
        if (read_profile(i, saved_ssid, sizeof(saved_ssid), saved_password, sizeof(saved_password)) &&
            strcmp(saved_ssid, ssid) == 0) {
            if (password && password_capacity) {
                strncpy(password, saved_password, password_capacity - 1);
                password[password_capacity - 1] = '\0';
            }
            return (int)i;
        }
    }
    return -1;
}

static esp_err_t migrate_legacy_profile(void)
{
    char ssid[33] = {0}, password[65] = {0};
    if (read_credential(WIFI_NVS_SSID, ssid, sizeof(ssid)) == false ||
        read_credential(WIFI_NVS_PASSWORD, password, sizeof(password)) == false)
        return ESP_OK;
    nvs_handle_t handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = write_profile(handle, 0, ssid, password);
        if (err == ESP_OK) err = nvs_set_str(handle, WIFI_NVS_LAST_SSID, ssid);
        if (err == ESP_OK) err = nvs_commit(handle);
        nvs_close(handle);
    }
    return err;
}

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)data;
    if (base != WIFI_EVENT) return;
    if (id == WIFI_EVENT_SCAN_DONE) {
        if(!s_scan_active)return; /* Ignore a cancelled scan's late event. */
        s_scan_active=false;s_scan_deadline=0;
        s_scan_ready = true;
        /* Scanning must not make an existing STA connection look lost. */
        set_state(s_link_connected ? BM_WIFI_CONNECTED : BM_WIFI_SCAN_READY);
        if(!s_link_connected) {
            /* A real link loss during a scan resumes automatic recovery once
             * the scan releases the radio. A user-paused attempt stays paused. */
            taskENTER_CRITICAL(&s_retry_mux);
            if(s_retry.active && !s_retry.deadline)
                (void)wifi_retry_disconnected(&s_retry,esp_timer_get_time(),false);
            taskEXIT_CRITICAL(&s_retry_mux);
        }
    } else if (id == WIFI_EVENT_STA_START) {
        /* A saved profile is connected by the worker; no-profile boots scan. */
    } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
        s_link_connected = false;
        xEventGroupClearBits(s_events, WIFI_CONNECTED_BIT);
        const wifi_event_sta_disconnected_t *disconnected =
            (const wifi_event_sta_disconnected_t *)data;
        ESP_LOGW(TAG, "Wi-Fi disconnected reason=%d", disconnected ? disconnected->reason : -1);
        if(s_reconfiguring || s_scan_requested || s_scan_active)return;
        int reason=disconnected?disconnected->reason:0;
        bool auth=reason==WIFI_REASON_AUTH_FAIL || reason==WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ||
                  reason==WIFI_REASON_HANDSHAKE_TIMEOUT;
        taskENTER_CRITICAL(&s_retry_mux);
        bool active=s_retry.active;
        bool failed=wifi_retry_disconnected(&s_retry,esp_timer_get_time(),auth);
        taskEXIT_CRITICAL(&s_retry_mux);
        if(failed)connection_failed(auth?"Authentication failed - edit password":"Connection failed - select network");
        else if(active)set_state(BM_WIFI_CONNECTING);
    }
}

static void ip_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)data;
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        wifi_ap_record_t ap;
        if(esp_wifi_sta_get_ap_info(&ap)!=ESP_OK)return;
        taskENTER_CRITICAL(&s_retry_mux);
        bool active=s_retry.active && !s_reconfiguring &&
                    !strncmp((const char*)ap.ssid,s_connected_ssid,32);
        if(active) {
            wifi_retry_connected(&s_retry);
            s_link_connected=true;
            set_state(BM_WIFI_CONNECTED);
            s_last_error[0]=s_failed_ssid[0]=0;
            s_commit_pending=s_save_pending;
        }
        taskEXIT_CRITICAL(&s_retry_mux);
        if(!active)return;
        xEventGroupSetBits(s_events, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Wi-Fi connected; DHCP/IP acquired");
        if (s_sntp_initialized) (void)esp_netif_sntp_start();
    }
}

esp_err_t bm_wifi_start(void)
{
    if (s_started) return ESP_OK;
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    if (!s_sntp_initialized) {
        esp_sntp_config_t time_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
        time_config.start = false;
        s_sntp_initialized = esp_netif_sntp_init(&time_config) == ESP_OK;
    }
    if (!s_events) s_events = xEventGroupCreate();
    if (!s_events) return ESP_ERR_NO_MEM;

    if (!s_netif) {
        esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_WIFI_STA();
        s_netif = esp_netif_new(&netif_cfg);
        if (!s_netif) return ESP_ERR_NO_MEM;
        err = esp_netif_attach_wifi_station(s_netif);
        if (err != ESP_OK) return err;
        err = esp_wifi_set_default_wifi_sta_handlers();
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    }

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&config);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    s_initialized = true;
    if (!s_handler_registered) {
        err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                   wifi_event, NULL, &s_wifi_handler);
        if (err != ESP_OK) return err;
        err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                   ip_event, NULL, &s_ip_handler);
        if (err != ESP_OK) return err;
        s_handler_registered = true;
    }
    /* Only our confirmed profile store persists credentials. Failed trial
     * passwords must not overwrite either a profile or the SDK's flash config. */
    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK) return err;
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) return err;

    (void)migrate_legacy_profile();
    char ssid[33] = {0};
    char password[65] = {0};
    char last_ssid[33] = {0};
    bool have_profile = false;
    if (read_value(WIFI_NVS_LAST_SSID, last_ssid, sizeof(last_ssid), true))
        have_profile = find_profile(last_ssid, password, sizeof(password)) >= 0;
    if (!have_profile) {
        for (size_t i = 0; i < WIFI_PROFILE_MAX && !have_profile; ++i)
            have_profile = read_profile(i, ssid, sizeof(ssid), password, sizeof(password));
    } else {
        strncpy(ssid, last_ssid, sizeof(ssid) - 1);
    }
    if (!have_profile) {
        set_state(BM_WIFI_IDLE);
        err = esp_wifi_start();
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
        s_started = true;
        return bm_wifi_scan();
    }

    wifi_config_t wifi = {0};
    memcpy(wifi.sta.ssid, ssid, strlen(ssid));
    strncpy((char *)wifi.sta.password, password, sizeof(wifi.sta.password) - 1);
    strncpy(s_connected_ssid, ssid, sizeof(s_connected_ssid) - 1);
    s_connected_ssid[sizeof(s_connected_ssid) - 1] = '\0';
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi);
    if (err != ESP_OK) return err;
    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    s_started = true;
    taskENTER_CRITICAL(&s_retry_mux);
    wifi_retry_begin(&s_retry,esp_timer_get_time(),false);
    s_retry.retry_at=esp_timer_get_time();
    taskEXIT_CRITICAL(&s_retry_mux);
    set_state(BM_WIFI_CONNECTING);
    return ESP_OK;
}

esp_err_t bm_wifi_scan(void)
{
    if (!s_started) return ESP_ERR_INVALID_STATE;
    bool was_connected = s_link_connected;
    s_scan_ready = false;
    s_scan_requested=true;s_scan_active=false;
    s_scan_deadline=esp_timer_get_time()+15000000;
    if(!was_connected) {
        taskENTER_CRITICAL(&s_retry_mux);wifi_retry_pause(&s_retry);taskEXIT_CRITICAL(&s_retry_mux);
        taskENTER_CRITICAL(&s_retry_mux);
        s_save_pending=s_commit_pending=false;
        memset(s_pending_password,0,sizeof(s_pending_password));
        taskEXIT_CRITICAL(&s_retry_mux);
        (void)esp_wifi_disconnect();
    }
    set_state(was_connected?BM_WIFI_CONNECTED:BM_WIFI_SCANNING);
    return ESP_OK; /* Worker retries while the driver finishes disconnecting. */
}

static esp_err_t save_confirmed_profile(const char *ssid, const char *password)
{
    if (!ssid || !password || ssid[0] == '\0' || strlen(ssid) > 32 || strlen(password) > 64)
        return ESP_ERR_INVALID_ARG;
    /* Resolve the profile slot before opening the write handle. This avoids
     * nested NVS handles during the confirmation path. */
    int slot = find_profile(ssid, NULL, 0);
    if (slot < 0) {
        char existing_ssid[33], existing_password[65];
        for (size_t i = 0; i < WIFI_PROFILE_MAX; ++i) {
            if (!read_profile(i, existing_ssid, sizeof(existing_ssid),
                              existing_password, sizeof(existing_password))) {
                slot = (int)i;
                break;
            }
        }
    }
    if (slot < 0) return ESP_ERR_NO_MEM;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = write_profile(handle, (size_t)slot, ssid, password);
    if (err == ESP_OK) err = nvs_set_str(handle, WIFI_NVS_LAST_SSID, ssid);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t bm_wifi_set_credentials(const char *ssid, const char *password)
{
    if(!ssid || !password || !ssid[0] || strlen(ssid)>32 || strlen(password)>64)
        return ESP_ERR_INVALID_ARG;
    esp_err_t err=ESP_OK;
    if(!s_started) {
        err=bm_wifi_start();
        if(err!=ESP_OK)return err;
    }
    wifi_config_t wifi = {0};
    memcpy(wifi.sta.ssid, ssid, strlen(ssid));
    strncpy((char *)wifi.sta.password, password, sizeof(wifi.sta.password) - 1);
    strncpy(s_connected_ssid, ssid, sizeof(s_connected_ssid) - 1);
    s_connected_ssid[sizeof(s_connected_ssid) - 1] = '\0';
    /* Switching profiles while already connected requires a clean handoff.
     * Suppress the disconnect handler's automatic reconnect until the new
     * configuration has been installed. */
    s_reconfiguring = true;
    s_scan_requested=s_scan_active=false;s_scan_deadline=0;
    taskENTER_CRITICAL(&s_retry_mux);wifi_retry_pause(&s_retry);taskEXIT_CRITICAL(&s_retry_mux);
    /* A completed scan can still leave the driver in scan state on some
     * firmware revisions. Stop it before changing STA credentials. */
    (void)esp_wifi_scan_stop();
    (void)esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(100));
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi);
    if (err == ESP_OK) {
        s_link_connected = false;
        taskENTER_CRITICAL(&s_retry_mux);
        snprintf(s_pending_ssid,sizeof(s_pending_ssid),"%s",ssid);
        snprintf(s_pending_password,sizeof(s_pending_password),"%s",password);
        s_save_pending=true;s_commit_pending=false;s_last_error[0]=0;
        wifi_retry_begin(&s_retry,esp_timer_get_time(),true);
        taskEXIT_CRITICAL(&s_retry_mux);
        set_state(BM_WIFI_CONNECTING);
        err = esp_wifi_connect();
    }
    s_reconfiguring = false;
    if (err != ESP_OK) {
        taskENTER_CRITICAL(&s_retry_mux);wifi_retry_pause(&s_retry);taskEXIT_CRITICAL(&s_retry_mux);
        connection_failed("Connection could not start - retry");
        ESP_LOGE(TAG, "Wi-Fi connect start failed for %.32s: %s",
                 ssid, esp_err_to_name(err));
    }
    return err;
}

void bm_wifi_service(void)
{
    if(!s_started)return;
    int64_t now=esp_timer_get_time();
    if(s_scan_deadline && now>=s_scan_deadline) {
        s_scan_requested=s_scan_active=false;s_scan_deadline=0;
        (void)esp_wifi_scan_stop();
        s_scan_ready=true;
        snprintf(s_last_error,sizeof(s_last_error),"Scan timed out - OK to retry");
        set_state(s_link_connected?BM_WIFI_CONNECTED:BM_WIFI_SCAN_READY);
    }
    if(s_scan_requested) {
        s_scan_active=true;
        esp_err_t err=esp_wifi_scan_start(NULL,false);
        if(err==ESP_OK)s_scan_requested=false;
        else s_scan_active=false; /* Retry next tick; bounded by scan deadline. */
        return;
    }
    taskENTER_CRITICAL(&s_retry_mux);
    wifi_retry_action_t action=wifi_retry_poll(&s_retry,now);
    taskEXIT_CRITICAL(&s_retry_mux);
    if(action==WIFI_RETRY_TIMEOUT) {
        connection_failed("Connection timed out - select network");
        (void)esp_wifi_disconnect();
    } else if(action==WIFI_RETRY_CONNECT) {
        set_state(BM_WIFI_CONNECTING);
        esp_err_t err=esp_wifi_connect();
        if(err!=ESP_OK) {
            taskENTER_CRITICAL(&s_retry_mux);
            bool failed=wifi_retry_disconnected(&s_retry,now,false);
            taskEXIT_CRITICAL(&s_retry_mux);
            if(failed)connection_failed("Connection could not start - retry");
        }
    }
    char ssid[33], password[65];
    taskENTER_CRITICAL(&s_retry_mux);
    bool commit=s_commit_pending && s_save_pending && s_link_connected;
    if(commit) {
        memcpy(ssid,s_pending_ssid,sizeof(ssid));
        memcpy(password,s_pending_password,sizeof(password));
        s_commit_pending=s_save_pending=false;
        memset(s_pending_password,0,sizeof(s_pending_password));
    }
    taskEXIT_CRITICAL(&s_retry_mux);
    if(commit) {
        esp_err_t err=save_confirmed_profile(ssid,password);
        memset(password,0,sizeof(password));
        if(err!=ESP_OK)ESP_LOGE(TAG,"Connected profile save failed: %s",esp_err_to_name(err));
    }
}

const char *bm_wifi_last_error(void) { return s_last_error; }
const char *bm_wifi_failed_ssid(void) { return s_failed_ssid; }

esp_err_t bm_wifi_stop(void)
{
    taskENTER_CRITICAL(&s_retry_mux);wifi_retry_pause(&s_retry);taskEXIT_CRITICAL(&s_retry_mux);
    s_scan_requested=s_scan_active=false;s_scan_deadline=0;
    if (s_started) {
        (void)esp_wifi_disconnect();
        (void)esp_wifi_stop();
        s_started = false;
    }
    if (s_handler_registered) {
        (void)esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, s_wifi_handler);
        (void)esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, s_ip_handler);
        s_handler_registered = false;
    }
    if (s_initialized) {
        (void)esp_wifi_deinit();
        s_initialized = false;
    }
    if (s_netif) {
        esp_netif_destroy_default_wifi(s_netif);
        s_netif = NULL;
    }
    set_state(BM_WIFI_IDLE);
    return ESP_OK;
}

bm_wifi_state_t bm_wifi_state(void) { return s_state; }

bool bm_wifi_scan_ready(void) { return s_scan_ready; }

const char *bm_wifi_state_text(void)
{
    switch (s_state) {
    case BM_WIFI_SCANNING: return "SCANNING";
    case BM_WIFI_SCAN_READY: return "READY";
    case BM_WIFI_CONNECTING: return "CONNECTING";
    case BM_WIFI_CONNECTED: return "CONNECTED";
    case BM_WIFI_FAILED: return "FAILED";
    default: return "SETUP";
    }
}

const char *bm_wifi_connected_ssid(void) { return s_connected_ssid; }

void bm_wifi_get_info(bm_wifi_info_t *out)
{
    bm_wifi_read_info(s_netif, s_state == BM_WIFI_CONNECTED, out);
}

bool bm_wifi_has_credentials(void)
{
    char ssid[33] = {0};
    char password[65] = {0};
    return read_credential(WIFI_NVS_SSID, ssid, sizeof(ssid)) &&
           read_credential(WIFI_NVS_PASSWORD, password, sizeof(password));
}

bool bm_wifi_get_saved_credentials(const char *ssid, char *password, size_t capacity)
{
    return ssid && password && capacity > 0 && find_profile(ssid, password, capacity) >= 0;
}

size_t bm_wifi_saved_profile_count(void)
{
    size_t count = 0;
    char ssid[33], password[65];
    for (size_t i = 0; i < WIFI_PROFILE_MAX; ++i)
        if (read_profile(i, ssid, sizeof(ssid), password, sizeof(password))) count++;
    return count;
}

bool bm_wifi_get_saved_profile(size_t index, char *ssid, size_t ssid_capacity)
{
    char password[65];
    size_t seen = 0;
    for (size_t i = 0; i < WIFI_PROFILE_MAX; ++i) {
        if (read_profile(i, ssid, ssid_capacity, password, sizeof(password))) {
            if (seen++ == index) return true;
        }
    }
    if (ssid && ssid_capacity) ssid[0] = '\0';
    return false;
}

esp_err_t bm_wifi_delete_profile(const char *ssid)
{
    int slot = find_profile(ssid, NULL, 0);
    if (slot < 0) return ESP_ERR_NOT_FOUND;
    nvs_handle_t handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    char ssid_key[24], password_key[32];
    snprintf(ssid_key, sizeof(ssid_key), "p%u_s", (unsigned)slot);
    snprintf(password_key, sizeof(password_key), "p%u_p", (unsigned)slot);
    err = nvs_erase_key(handle, ssid_key);
    if (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) err = nvs_erase_key(handle, password_key);
    if (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) err = nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_ERR_NVS_NOT_FOUND ? ESP_OK : err;
}

size_t bm_wifi_get_scan_records(wifi_ap_record_t *records, size_t capacity, uint16_t *total)
{
    if (!records || capacity == 0 || !s_started) return 0;
    uint16_t count = (uint16_t)capacity;
    if (esp_wifi_scan_get_ap_num(total) != ESP_OK ||
        esp_wifi_scan_get_ap_records(&count, records) != ESP_OK) return 0;
    return count;
}
