#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Event values are checked against the BSP at the application boundary. */
typedef enum { QSO_KEY_PRESS, QSO_KEY_CLICK, QSO_KEY_DOUBLE, QSO_KEY_LONG, QSO_KEY_RELEASE } qso_key_event_t;
enum { QSO_KEY_OK=2 };
typedef enum { QSO_SCREEN_PASS, QSO_SCREEN_IGNORE, QSO_SCREEN_OFF, QSO_SCREEN_WAKE } qso_screen_action_t;
typedef struct {
    bool off, wait_off_release, consuming_wake, wake_released;
    unsigned wake_key;
    int64_t guard_until;
} qso_screen_t;
qso_screen_action_t qso_screen_key(qso_screen_t *state, unsigned key,
                                  qso_key_event_t event, int64_t now, bool monitor);
