#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BM_AUDIO_PACKET_MAX 960
#define BM_QSO_HISTORY_MAX 15
#define BM_QSO_VISIBLE_ROWS 5

typedef struct {
    bool active;
    bool historical;
    uint32_t source_id;
    char session[64];
    char callsign[24];
    char name[40];
    char country[24];
    int64_t started_us;
    int64_t ended_us;
    unsigned elapsed_seconds;
} bm_qso_info_t;

/* Assemble both driver chunks and RFC 6455 continuation frames into a bounded
 * caller-owned buffer. Control frames do not disturb an unfinished message. */
typedef struct {
    size_t used;
    size_t frame_offset;
    uint8_t opcode;
    bool assembling;
} bm_frame_state_t;

bool bm_frame_append(bm_frame_state_t *state, uint8_t *buffer, size_t capacity,
                     uint8_t opcode, bool fin, size_t payload_length,
                     size_t offset, const void *data, size_t length,
                     size_t *message_length);
size_t bm_decode_audio(const uint8_t *message, size_t length,
                        uint8_t *samples, size_t capacity);
bool bm_decode_caller(const uint8_t *message, size_t length, uint32_t *source, uint32_t *talkgroup);
bool bm_decode_metadata(const char *message, int talkgroup, bm_qso_info_t *out);
bool bm_metadata_may_match(const char *message, int talkgroup);
void bm_country_label(char *out, size_t size, const char *callsign);
