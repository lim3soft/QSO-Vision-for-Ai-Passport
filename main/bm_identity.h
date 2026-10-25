#pragma once
#include "bm_protocol.h"

#define BM_IDENTITY_CACHE_SIZE 16
typedef struct {
    uint32_t id;
    char callsign[24], name[40], country[24];
} bm_identity_t;
typedef struct { bm_identity_t entries[BM_IDENTITY_CACHE_SIZE]; unsigned next; } bm_identity_cache_t;

bool bm_callsign_known(const char *text);
bool bm_identity_get(const bm_identity_cache_t *cache, uint32_t id, bm_identity_t *out);
void bm_identity_put(bm_identity_cache_t *cache, const bm_identity_t *item);
void bm_identity_enrich(const bm_identity_cache_t *cache, bm_qso_info_t *qso);
bool bm_identity_decode(const char *json, uint32_t expected_id, bm_identity_t *out);
bool bm_identity_alias(const uint8_t *message, size_t length, uint32_t id, bm_identity_t *out);
/* HTTPS lookup, called only by the network supervisor with TLS heap reserved. */
bool bm_identity_lookup(uint32_t id, bm_identity_t *out);
