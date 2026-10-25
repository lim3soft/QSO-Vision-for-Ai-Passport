#pragma once
#include "qso_screen.h"
typedef enum { QSO_CONTROL_NONE, QSO_HISTORY_NEWER, QSO_HISTORY_OLDER,
    QSO_VOLUME_UP, QSO_VOLUME_DOWN, QSO_MUTE_TOGGLE } qso_control_action_t;
typedef struct { int held; int64_t repeat_at_us; } qso_controls_t;
qso_control_action_t qso_controls_key(qso_controls_t *s, unsigned key,
    qso_key_event_t event, int64_t now, bool monitor);
qso_control_action_t qso_controls_tick(qso_controls_t *s, int64_t now, bool monitor);
