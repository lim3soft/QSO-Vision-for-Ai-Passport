#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum { QSO_DIM_SMART, QSO_DIM_1_MIN, QSO_DIM_5_MIN, QSO_DIM_10_MIN,
               QSO_DIM_NEVER, QSO_DIM_MODE_COUNT } qso_dim_mode_t;
typedef struct {
    qso_dim_mode_t mode;
    uint8_t brightness_percent;
    int64_t last_input_us, last_qso_us;
} qso_dimmer_t;
void qso_dimmer_init(qso_dimmer_t *s, int64_t now);
void qso_dimmer_input(qso_dimmer_t *s, int64_t now);
unsigned qso_dimmer_level(qso_dimmer_t *s, int64_t now, bool qso, bool monitor);

/* Normal brightness is the ceiling for menus, new-QSO restore and manual wake. */
unsigned qso_dimmer_awake_level(const qso_dimmer_t *s);
