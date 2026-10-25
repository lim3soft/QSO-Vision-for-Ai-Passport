#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
typedef int esp_err_t;
typedef int esp_event_base_t;
typedef int esp_event_handler_instance_t;
typedef int esp_netif_t;
typedef int esp_netif_config_t;
typedef int wifi_init_config_t;
typedef int nvs_handle_t;
typedef int EventGroupHandle_t;
typedef int portMUX_TYPE;
typedef struct { bool start; } esp_sntp_config_t;
typedef struct { uint8_t ssid[33]; int rssi, authmode; } wifi_ap_record_t;
typedef struct { int reason; } wifi_event_sta_disconnected_t;
typedef struct { struct { uint8_t ssid[32], password[64]; } sta; } wifi_config_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_STATE 1
#define ESP_ERR_INVALID_ARG 2
#define ESP_ERR_NO_MEM 3
#define ESP_ERR_NOT_FOUND 4
#define ESP_ERR_NVS_NOT_FOUND 4
#define NVS_READONLY 0
#define NVS_READWRITE 1
#define WIFI_EVENT 1
#define IP_EVENT 2
#define WIFI_EVENT_SCAN_DONE 1
#define WIFI_EVENT_STA_START 2
#define WIFI_EVENT_STA_DISCONNECTED 3
#define IP_EVENT_STA_GOT_IP 4
#define ESP_EVENT_ANY_ID -1
#define WIFI_REASON_AUTH_FAIL 202
#define WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT 15
#define WIFI_REASON_HANDSHAKE_TIMEOUT 204
#define WIFI_STORAGE_RAM 0
#define WIFI_MODE_STA 1
#define WIFI_IF_STA 0
#define BIT0 1
#define portMUX_INITIALIZER_UNLOCKED 0
#define taskENTER_CRITICAL(x) ((void)(x))
#define taskEXIT_CRITICAL(x) ((void)(x))
#define ESP_NETIF_SNTP_DEFAULT_CONFIG(x) ((esp_sntp_config_t){0})
#define ESP_NETIF_DEFAULT_WIFI_STA() 0
#define WIFI_INIT_CONFIG_DEFAULT() 0
#define pdMS_TO_TICKS(x) (x)
#define ESP_LOGI(tag,...) do{(void)tag;if(false)printf(__VA_ARGS__);}while(0)
#define ESP_LOGW ESP_LOGI
#define ESP_LOGE ESP_LOGI
int64_t esp_timer_get_time(void);
void vTaskDelay(int ticks);
const char *esp_err_to_name(esp_err_t err);
int esp_wifi_connect(void);
int esp_wifi_disconnect(void);
int esp_wifi_scan_start(const void *config,bool block);
int esp_wifi_scan_stop(void);
int esp_wifi_scan_get_ap_num(uint16_t *count);
int esp_wifi_scan_get_ap_records(uint16_t *count,wifi_ap_record_t *records);
int esp_wifi_set_config(int interface,const wifi_config_t *config);
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *out);
int nvs_open(const char *name,int mode,nvs_handle_t *handle);
int nvs_get_str(nvs_handle_t h,const char *key,char *value,size_t *length);
int nvs_set_str(nvs_handle_t h,const char *key,const char *value);
int nvs_commit(nvs_handle_t h);
int nvs_erase_key(nvs_handle_t h,const char *key);
void nvs_close(nvs_handle_t h);
static inline int esp_netif_init(void){return 0;}
static inline int esp_event_loop_create_default(void){return 0;}
static inline int esp_netif_sntp_init(const esp_sntp_config_t *c){(void)c;return 0;}
static inline int esp_netif_sntp_start(void){return 0;}
static inline int xEventGroupCreate(void){return 1;}
static inline int xEventGroupSetBits(int e,int b){(void)e;return b;}
static inline int xEventGroupClearBits(int e,int b){(void)e;return b;}
static inline esp_netif_t *esp_netif_new(const esp_netif_config_t *c){static int n;(void)c;return &n;}
static inline int esp_netif_attach_wifi_station(esp_netif_t *n){(void)n;return 0;}
static inline void esp_netif_destroy_default_wifi(esp_netif_t *n){(void)n;}
static inline int esp_wifi_set_default_wifi_sta_handlers(void){return 0;}
static inline int esp_wifi_init(const wifi_init_config_t *c){(void)c;return 0;}
static inline int esp_wifi_set_storage(int mode){(void)mode;return 0;}
static inline int esp_wifi_set_mode(int mode){(void)mode;return 0;}
static inline int esp_wifi_start(void){return 0;}
static inline int esp_wifi_stop(void){return 0;}
static inline int esp_wifi_deinit(void){return 0;}
static inline int esp_event_handler_instance_register(int base,int id,void(*fn)(void*,int,int32_t,void*),void *arg,int *out)
{(void)base;(void)id;(void)fn;(void)arg;*out=1;return 0;}
static inline int esp_event_handler_instance_unregister(int base,int id,int instance)
{(void)base;(void)id;(void)instance;return 0;}
