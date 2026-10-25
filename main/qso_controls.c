#include "qso_controls.h"
qso_control_action_t qso_controls_key(qso_controls_t *s, unsigned key,
    qso_key_event_t event, int64_t now, bool monitor)
{
    if(event==QSO_KEY_RELEASE && s->held==(int)key+1)s->held=0;
    if(!monitor){s->held=0;return QSO_CONTROL_NONE;}
    if(event==QSO_KEY_LONG && key<2) {
        s->held=(int)key+1;s->repeat_at_us=now+200000;
        return key==0?QSO_VOLUME_UP:QSO_VOLUME_DOWN;
    }
    if(event==QSO_KEY_CLICK && key<2)return key==0?QSO_HISTORY_NEWER:QSO_HISTORY_OLDER;
    if(event==QSO_KEY_DOUBLE && key==1)return QSO_MUTE_TOGGLE;
    return QSO_CONTROL_NONE;
}
qso_control_action_t qso_controls_tick(qso_controls_t *s, int64_t now, bool monitor)
{
    if(!monitor){s->held=0;return QSO_CONTROL_NONE;}
    if(!s->held || now<s->repeat_at_us)return QSO_CONTROL_NONE;
    s->repeat_at_us=now+200000;
    return s->held==1?QSO_VOLUME_UP:QSO_VOLUME_DOWN;
}
