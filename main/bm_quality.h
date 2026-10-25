#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Hundredths of a percent, independently valid (zero is a real report).
 * Values are from BM's per-session ingress report, not Passport Wi-Fi loss. */
#define BM_QUALITY_BER 1u
#define BM_QUALITY_LOSS 2u
typedef struct {
    uint32_t sample_count;
    uint16_t ber_centi_pct, loss_centi_pct;
    uint8_t valid;
    /* Distinguish an explicitly unavailable value from an omitted field.
     * A provisional zero with no samples is neither a measurement nor a clear. */
    uint8_t reported;
} bm_qso_quality_t;

void bm_quality_merge(bm_qso_quality_t *target, const bm_qso_quality_t *update);
void bm_quality_format(char *out, size_t size, const bm_qso_quality_t *quality);

