#include "qso_dimmer.h"

void qso_dimmer_init(qso_dimmer_t *s, int64_t now)
{
    *s=(qso_dimmer_t){.mode=QSO_DIM_SMART,.brightness_percent=100,
        .last_input_us=now,.last_qso_us=now};
}
void qso_dimmer_input(qso_dimmer_t *s, int64_t now) { s->last_input_us=now; }
unsigned qso_dimmer_level(qso_dimmer_t *s, int64_t now, bool qso, bool monitor)
{
    unsigned awake=qso_dimmer_awake_level(s);
    if(qso || !monitor)s->last_qso_us=now;
    if(s->mode==QSO_DIM_SMART) {
        int64_t last=s->last_input_us>s->last_qso_us?s->last_input_us:s->last_qso_us;
        return now-last>=5000000 ? 1 : awake;
    }
    static const unsigned minutes[QSO_DIM_MODE_COUNT]={0,1,5,10,0};
    if(s->mode<=QSO_DIM_SMART || s->mode>=QSO_DIM_NEVER)return awake;
    return now-s->last_input_us>=(int64_t)minutes[s->mode]*60000000 ? 1 : awake;
}

unsigned qso_dimmer_awake_level(const qso_dimmer_t *s)
{
    unsigned level=s->brightness_percent;
    if(level<1)level=1;
    if(level>100)level=100;
    return level;
}
