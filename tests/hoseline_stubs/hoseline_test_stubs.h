#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef int esp_err_t;
typedef void *esp_event_base_t;
typedef int portMUX_TYPE;
typedef void *TaskHandle_t;
typedef unsigned TickType_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_NO_MEM 0x101
#define ESP_ERR_INVALID_ARG 0x102
#define MALLOC_CAP_8BIT 1
#define portMUX_INITIALIZER_UNLOCKED 0
#define portMAX_DELAY 0xffffffffu
#define pdTRUE 1
#define pdPASS 1
#define pdMS_TO_TICKS(x) (x)
#define taskENTER_CRITICAL(x) ((void)(x))
#define taskEXIT_CRITICAL(x) ((void)(x))
#define ESP_LOGI(tag,...) do { (void)(tag); if (false) printf(__VA_ARGS__); } while (0)
#define ESP_LOGW ESP_LOGI

typedef struct test_queue *QueueHandle_t;
typedef struct test_socket *esp_websocket_client_handle_t;
enum { WEBSOCKET_EVENT_ANY, WEBSOCKET_EVENT_CONNECTED, WEBSOCKET_EVENT_DISCONNECTED,
       WEBSOCKET_EVENT_ERROR, WEBSOCKET_EVENT_DATA };
typedef struct {
    int data_len, payload_len, payload_offset;
    uint8_t op_code;
    bool fin;
    const char *data_ptr;
} esp_websocket_event_data_t;
typedef struct {
    const char *uri, *subprotocol, *task_name;
    int buffer_size, task_stack, task_prio, network_timeout_ms, reconnect_timeout_ms;
    void *crt_bundle_attach;
} esp_websocket_client_config_t;
typedef void (*test_callback_t)(void *,esp_event_base_t,int32_t,void *);

const char *esp_err_to_name(esp_err_t err);
int64_t esp_timer_get_time(void);
size_t heap_caps_get_free_size(int caps);
size_t heap_caps_get_largest_free_block(int caps);
extern void *esp_crt_bundle_attach;
esp_err_t bsp_audio_write(const void *data,size_t size);
QueueHandle_t xQueueCreate(unsigned count,size_t size);
void vQueueDelete(QueueHandle_t queue);
int xQueueSend(QueueHandle_t queue,const void *data,TickType_t wait);
int xQueueReceive(QueueHandle_t queue,void *data,TickType_t wait);
int xQueueReset(QueueHandle_t queue);
void vTaskDelay(TickType_t ticks);
int xTaskCreate(void (*fn)(void *),const char *name,unsigned stack,void *arg,int priority,TaskHandle_t *task);
esp_websocket_client_handle_t esp_websocket_client_init(const esp_websocket_client_config_t *config);
esp_err_t esp_websocket_client_destroy(esp_websocket_client_handle_t client);
esp_err_t esp_websocket_register_events(esp_websocket_client_handle_t client,int event,test_callback_t callback,void *arg);
esp_err_t esp_websocket_client_start(esp_websocket_client_handle_t client);
esp_err_t esp_websocket_client_stop(esp_websocket_client_handle_t client);
bool esp_websocket_client_is_connected(esp_websocket_client_handle_t client);
int esp_websocket_client_send_bin(esp_websocket_client_handle_t client,const char *data,int size,TickType_t wait);
int esp_websocket_client_send_text(esp_websocket_client_handle_t client,const char *data,int size,TickType_t wait);
