#pragma once
#include <stddef.h>
#include <stdint.h>
#define QSO_FLAG_WIDTH 16
#define QSO_FLAG_HEIGHT 12

typedef struct {
    const char *code, *country;
    const uint16_t *pixels;
} qso_flag_t;
extern const qso_flag_t qso_country_flags[];
extern const size_t qso_country_flag_count;
/* Country labels/codes only: do not guess an unresolved caller's location. */
const qso_flag_t *qso_flag_find(const char *country);

/* Full redistribution notice is retained in the firmware image. */
const char *qso_flag_license(void);
