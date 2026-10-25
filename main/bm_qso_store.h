#pragma once
#include "bm_protocol.h"

/* Completed rows omit the unused operator name and live-only state. Keep the
 * complete SessionID for exact de-duplication, not a collision-prone hash. */
typedef struct {
    int64_t started_us, ended_us;
    uint32_t source_id;
    char session[64], callsign[24], country[24];
} bm_qso_history_row_t;

typedef struct {
    bm_qso_info_t current;
    bm_qso_history_row_t history[BM_QSO_HISTORY_MAX];
    size_t count;
    int64_t current_received_us;
    unsigned initial_seconds;
} bm_qso_store_t;

void bm_qso_store_apply(bm_qso_store_t *store, const bm_qso_info_t *item,
                        int64_t monotonic_us, int64_t wall_seconds);

/* A page is expanded into caller-owned storage; never copy the entire history to a task
 * stack. An optional SessionID anchor keeps an older view stable on inserts. */
size_t bm_qso_store_page(const bm_qso_store_t *store, bm_qso_info_t *out,
    size_t capacity, size_t *offset, size_t *total, const char *anchor);
