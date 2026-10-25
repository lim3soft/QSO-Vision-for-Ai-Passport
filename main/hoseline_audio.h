#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bm_protocol.h"

typedef enum {
    BM_AUDIO_IDLE = 0,
    BM_AUDIO_CONNECTING,
    BM_AUDIO_CONNECTED,
    BM_AUDIO_PLAYING,
    BM_AUDIO_ERROR,
} bm_audio_state_t;

esp_err_t bm_audio_start(int talkgroup);
esp_err_t bm_audio_select(int talkgroup);
bm_audio_state_t bm_audio_state(void);
const char *bm_audio_state_text(void);
bool bm_audio_is_active(void);
esp_err_t bm_metadata_start(void);
void bm_network_service(bool wifi_connected);
const char *bm_metadata_state_text(void);
bool bm_qso_get_current(bm_qso_info_t *out);
size_t bm_qso_get_history(bm_qso_info_t *out, size_t capacity);

size_t bm_qso_get_history_page(bm_qso_info_t *out, size_t capacity,
    size_t *offset, size_t *total, const char *anchor);
