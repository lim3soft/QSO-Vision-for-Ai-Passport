#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum { WIFI_RETRY_NONE, WIFI_RETRY_CONNECT, WIFI_RETRY_TIMEOUT } wifi_retry_action_t;
typedef struct {
    bool active, manual;
    unsigned failures;
    int64_t deadline, retry_at;
} wifi_retry_t;
void wifi_retry_begin(wifi_retry_t *r, int64_t now, bool manual);
void wifi_retry_pause(wifi_retry_t *r);
void wifi_retry_connected(wifi_retry_t *r);
/* True means this attempt has failed and must be exposed to the user. */
bool wifi_retry_disconnected(wifi_retry_t *r, int64_t now, bool authentication_failed);
wifi_retry_action_t wifi_retry_poll(wifi_retry_t *r, int64_t now);
