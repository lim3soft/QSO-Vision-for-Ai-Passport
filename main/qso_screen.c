#include "qso_screen.h"

qso_screen_action_t qso_screen_key(qso_screen_t *s, unsigned key,
                                  qso_key_event_t event, int64_t now, bool monitor)
{
    if(s->off) {
        /* The release of the long OK which blanked the display is not a wake. */
        if(s->wait_off_release) {
            if(event==QSO_KEY_RELEASE && key==QSO_KEY_OK)s->wait_off_release=false;
            else if(now>=s->guard_until && event==QSO_KEY_PRESS)s->wait_off_release=false;
            else return QSO_SCREEN_IGNORE;
            if(event==QSO_KEY_RELEASE)return QSO_SCREEN_IGNORE;
        }
        if(event!=QSO_KEY_LONG || key!=QSO_KEY_OK)return QSO_SCREEN_IGNORE;
        s->off=false;s->consuming_wake=true;s->wake_released=false;
        s->wake_key=key;s->guard_until=now+3000000;
        return QSO_SCREEN_WAKE;
    }
    if(s->consuming_wake) {
        /* Also consume delayed click/double/long events after the wake release.
         * A fresh press after the quiet interval starts a new user gesture. */
        if(event==QSO_KEY_RELEASE && key==s->wake_key) {
            s->wake_released=true;s->guard_until=now+500000;
        }
        if(event==QSO_KEY_PRESS && now>=s->guard_until) {
            s->consuming_wake=false;
            return QSO_SCREEN_PASS;
        }
        return QSO_SCREEN_IGNORE;
    }
    if(monitor && key==QSO_KEY_OK && event==QSO_KEY_LONG) {
        s->off=true;s->wait_off_release=true;s->guard_until=now+3000000;
        return QSO_SCREEN_OFF;
    }
    return QSO_SCREEN_PASS;
}
