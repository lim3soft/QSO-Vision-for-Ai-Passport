#include "qso_dimmer.h"
#include "qso_controls.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    qso_dimmer_t s;qso_dimmer_init(&s,0);
    assert(s.mode==QSO_DIM_SMART && s.brightness_percent==100);
    const unsigned levels[]={1,37,100};
    for(unsigned n=0;n<3;++n) {
        qso_dimmer_init(&s,0);s.brightness_percent=(uint8_t)levels[n];
        unsigned awake=levels[n];
        assert(qso_dimmer_awake_level(&s)==awake);
        assert(qso_dimmer_level(&s,4999999,false,true)==awake);
        assert(qso_dimmer_level(&s,5000000,false,true)==1);
        assert(qso_dimmer_level(&s,6000000,true,true)==awake);
        assert(qso_dimmer_level(&s,10999999,false,true)==awake);
        assert(qso_dimmer_level(&s,11000000,false,true)==1);
        qso_dimmer_input(&s,12000000);
        assert(qso_dimmer_level(&s,12000000,false,true)==awake);
        assert(qso_dimmer_level(&s,17000000,false,true)==1);
        assert(qso_dimmer_level(&s,18000000,false,false)==awake);
        assert(qso_dimmer_level(&s,22999999,false,true)==awake);
        assert(qso_dimmer_level(&s,23000000,false,true)==1);
        const unsigned mins[]={1,5,10};
        for(unsigned i=0;i<3;++i) {
            s.mode=(qso_dim_mode_t)(QSO_DIM_1_MIN+i);
            qso_dimmer_input(&s,0);int64_t deadline=(int64_t)mins[i]*60000000;
            assert(qso_dimmer_level(&s,deadline-1,true,true)==awake);
            assert(qso_dimmer_level(&s,deadline,true,true)==1);
            qso_dimmer_input(&s,deadline);
            assert(qso_dimmer_level(&s,deadline,false,true)==awake);
        }
        s.mode=QSO_DIM_NEVER;assert(qso_dimmer_level(&s,INT64_MAX,false,true)==awake);
    }
    s.brightness_percent=0;assert(qso_dimmer_awake_level(&s)==1);
    s.brightness_percent=255;assert(qso_dimmer_awake_level(&s)==100);
    qso_controls_t c={0};
    assert(qso_controls_key(&c,0,QSO_KEY_CLICK,0,true)==QSO_HISTORY_NEWER);
    assert(qso_controls_key(&c,1,QSO_KEY_CLICK,0,true)==QSO_HISTORY_OLDER);
    assert(qso_controls_key(&c,1,QSO_KEY_DOUBLE,0,true)==QSO_MUTE_TOGGLE);
    assert(qso_controls_key(&c,0,QSO_KEY_PRESS,0,true)==QSO_CONTROL_NONE);
    assert(qso_controls_key(&c,0,QSO_KEY_LONG,500000,true)==QSO_VOLUME_UP);
    assert(qso_controls_tick(&c,699999,true)==QSO_CONTROL_NONE);
    assert(qso_controls_tick(&c,700000,true)==QSO_VOLUME_UP);
    assert(qso_controls_key(&c,0,QSO_KEY_RELEASE,750000,true)==QSO_CONTROL_NONE);
    assert(qso_controls_tick(&c,950000,true)==QSO_CONTROL_NONE);
    assert(qso_controls_key(&c,1,QSO_KEY_LONG,1000000,true)==QSO_VOLUME_DOWN);
    assert(qso_controls_tick(&c,1200000,true)==QSO_VOLUME_DOWN);
    assert(qso_controls_tick(&c,1400000,false)==QSO_CONTROL_NONE);
    assert(qso_controls_tick(&c,1600000,true)==QSO_CONTROL_NONE);
    assert(qso_controls_key(&c,1,QSO_KEY_DOUBLE,2000000,false)==QSO_CONTROL_NONE);
    puts("Fixed 1% dim, saved awake levels 1/37/100%, deadlines, brightness bounds, repeat-volume and history/mute mapping: PASS");
}
