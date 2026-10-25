#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#define QSO_EQ_BARS 10
#define QSO_EQ_SEGMENTS 12
/* Activity animation, deliberately independent of the audio/DSP path. */
bool qso_eq_step(uint8_t levels[QSO_EQ_BARS], unsigned phase, bool active);
void qso_clock_format(char *out, size_t capacity, time_t wall_time);
uint32_t qso_battery_color(int percentage);
