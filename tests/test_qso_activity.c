#define _POSIX_C_SOURCE 200809L
#include "qso_ui_math.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static time_t date(int y,int m,int d,int h,int min,int sec)
{
    struct tm local={.tm_year=y-1900,.tm_mon=m-1,.tm_mday=d,.tm_hour=h,
        .tm_min=min,.tm_sec=sec,.tm_isdst=-1};
    return mktime(&local);
}
int main(void)
{
    setenv("TZ","MYT-8",1);tzset();
    char text[40];
    time_t day=date(2026,10,25,23,59,59);
    qso_clock_format(text,sizeof(text),day);assert(!strcmp(text,"23:59:59 SUN 25/10/26"));
    qso_clock_format(text,sizeof(text),day+1);assert(!strcmp(text,"00:00:00 MON 26/10/26"));
    qso_clock_format(text,sizeof(text),date(2026,12,31,23,59,59)+1);
    assert(!strcmp(text,"00:00:00 FRI 01/01/27"));
    qso_clock_format(text,sizeof(text),date(2028,2,28,23,59,59)+1);
    assert(!strcmp(text,"00:00:00 TUE 29/02/28"));
    qso_clock_format(text,sizeof(text),0);assert(!strcmp(text,"--:--:-- --- --/--/--"));
    qso_clock_format(text,sizeof(text),(time_t)-1);assert(!strcmp(text,"--:--:-- --- --/--/--"));
    qso_clock_format(text,0,day);assert(!strcmp(text,"--:--:-- --- --/--/--"));
    qso_clock_format(NULL,40,day);
    char tiny[4];qso_clock_format(tiny,sizeof(tiny),day);assert(!strcmp(tiny,"23:"));
    uint8_t bars[QSO_EQ_BARS]={0};
    assert(!qso_eq_step(bars,0,false));
    bool changed=false;
    for(unsigned phase=0;phase<2000;++phase) {
        changed|=qso_eq_step(bars,phase,true);
        for(unsigned i=0;i<QSO_EQ_BARS;++i)assert(bars[i]>=1 && bars[i]<=QSO_EQ_SEGMENTS);
    }
    assert(changed);
    memset(bars,QSO_EQ_SEGMENTS,sizeof(bars));
    for(unsigned tick=0;tick<6;++tick) {
        uint8_t before[QSO_EQ_BARS];memcpy(before,bars,sizeof(bars));
        assert(qso_eq_step(bars,tick,false));
        for(unsigned i=0;i<QSO_EQ_BARS;++i)assert(bars[i]<=before[i]);
    }
    for(unsigned i=0;i<QSO_EQ_BARS;++i)assert(!bars[i]);
    assert(!qso_eq_step(bars,7,false));
    assert(qso_eq_step(bars,0xffffffffu,true));
    puts("Clock format/timezone/weekday/rollovers/unsynced/bounds and animated EQ attack/decay/idle: PASS");
}
