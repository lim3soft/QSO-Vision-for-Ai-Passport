#define _POSIX_C_SOURCE 200809L
#include "qso_ui_math.h"
#include <stdio.h>

uint32_t qso_battery_color(int percentage)
{
    if(percentage<0)return 0x93B8C8;
    if(percentage<=10)return 0xFF4D60;
    if(percentage<=20)return 0xFFD44D;
    return 0x56EBAC;
}

bool qso_eq_step(uint8_t levels[QSO_EQ_BARS], unsigned phase, bool active)
{
    bool changed=false;
    for(unsigned i=0;i<QSO_EQ_BARS;++i) {
        unsigned old=levels[i];
        unsigned value=old>QSO_EQ_SEGMENTS?QSO_EQ_SEGMENTS:old;
        if(active) {
            uint32_t noise=(phase/3+1)*1103515245u+(i+1)*2654435761u;
            noise^=noise>>11;
            unsigned target=2+noise%(QSO_EQ_SEGMENTS-1);
            if(value<target)value=target-value>3?value+3:target;
            else if(value>target)--value;
        } else value=value>2?value-2:0;
        levels[i]=(uint8_t)value;
        if(value!=old)changed=true;
    }
    return changed;
}

void qso_clock_format(char *out, size_t capacity, time_t wall_time)
{
    struct tm local;
    static const char *days[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
    if(!out || !capacity)return;
    /* Do not show a plausible-looking 1970 clock before SNTP succeeds. */
    if(wall_time<1577836800 || !localtime_r(&wall_time,&local) ||
        local.tm_wday<0 || local.tm_wday>6) {
        snprintf(out,capacity,"--:--:-- --- --/--/--");
        return;
    }
    snprintf(out,capacity,"%02u:%02u:%02u %s %02u/%02u/%02u",
        (unsigned)local.tm_hour,(unsigned)local.tm_min,(unsigned)local.tm_sec,
        days[local.tm_wday],(unsigned)local.tm_mday,(unsigned)(local.tm_mon+1),
        (unsigned)(local.tm_year+1900)%100);
}
