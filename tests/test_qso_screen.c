#include "qso_screen.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    qso_screen_t s={0};
    assert(qso_screen_key(&s,QSO_KEY_OK,QSO_KEY_CLICK,10,true)==QSO_SCREEN_PASS);
    assert(!s.off); /* Main handles the short click as menu access. */
    assert(qso_screen_key(&s,QSO_KEY_OK,QSO_KEY_LONG,20,false)==QSO_SCREEN_PASS);
    assert(!s.off); /* Menu long OK is still Back. */
    assert(qso_screen_key(&s,QSO_KEY_OK,QSO_KEY_LONG,100,true)==QSO_SCREEN_OFF);
    assert(qso_screen_key(&s,QSO_KEY_OK,QSO_KEY_RELEASE,200,true)==QSO_SCREEN_IGNORE && s.off);
    for(unsigned key=0;key<3;++key) {
        assert(qso_screen_key(&s,key,QSO_KEY_PRESS,300,true)==QSO_SCREEN_IGNORE);
        assert(qso_screen_key(&s,key,QSO_KEY_CLICK,400,true)==QSO_SCREEN_IGNORE);
        assert(qso_screen_key(&s,key,QSO_KEY_RELEASE,500,true)==QSO_SCREEN_IGNORE);
    }
    assert(qso_screen_key(&s,0,QSO_KEY_LONG,600,true)==QSO_SCREEN_IGNORE && s.off);
    assert(qso_screen_key(&s,1,QSO_KEY_LONG,700,true)==QSO_SCREEN_IGNORE && s.off);
    assert(qso_screen_key(&s,2,QSO_KEY_LONG,800,true)==QSO_SCREEN_WAKE && !s.off);
    assert(qso_screen_key(&s,2,QSO_KEY_RELEASE,900,true)==QSO_SCREEN_IGNORE);
    assert(qso_screen_key(&s,2,QSO_KEY_CLICK,200900,true)==QSO_SCREEN_IGNORE);
    assert(qso_screen_key(&s,2,QSO_KEY_DOUBLE,600900,true)==QSO_SCREEN_IGNORE);
    assert(qso_screen_key(&s,0,QSO_KEY_PRESS,700900,true)==QSO_SCREEN_PASS);
    assert(qso_screen_key(&s,0,QSO_KEY_CLICK,800900,true)==QSO_SCREEN_PASS);
    /* Holding OK to wake must not immediately turn the display off again. */
    assert(qso_screen_key(&s,2,QSO_KEY_LONG,1000000,true)==QSO_SCREEN_OFF);
    qso_screen_key(&s,2,QSO_KEY_RELEASE,1100000,true);
    assert(qso_screen_key(&s,2,QSO_KEY_PRESS,1200000,true)==QSO_SCREEN_IGNORE && s.off);
    assert(qso_screen_key(&s,2,QSO_KEY_LONG,1800000,true)==QSO_SCREEN_WAKE && !s.off);
    assert(qso_screen_key(&s,2,QSO_KEY_LONG,1850000,true)==QSO_SCREEN_IGNORE && !s.off);
    qso_screen_key(&s,2,QSO_KEY_RELEASE,1900000,true);
    assert(qso_screen_key(&s,2,QSO_KEY_PRESS,2500000,true)==QSO_SCREEN_PASS);
    assert(qso_screen_key(&s,2,QSO_KEY_LONG,3100000,true)==QSO_SCREEN_OFF);
    /* Missing release events have bounded recovery on a new physical press. */
    assert(qso_screen_key(&s,2,QSO_KEY_PRESS,6200000,true)==QSO_SCREEN_IGNORE && s.off);
    assert(qso_screen_key(&s,1,QSO_KEY_LONG,6800000,true)==QSO_SCREEN_IGNORE && s.off);
    assert(qso_screen_key(&s,2,QSO_KEY_LONG,6900000,true)==QSO_SCREEN_WAKE);
    assert(qso_screen_key(&s,1,QSO_KEY_PRESS,10000000,true)==QSO_SCREEN_PASS);
    puts("Display control: PASS (long-OK-only wake, short/other keys ignored, delayed gestures, missing releases)");
    return 0;
}
