#include "qso_flags.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    assert(qso_country_flag_count==256);
    assert(strstr(qso_flag_license(),"Permission is hereby granted"));
    assert(strstr(qso_flag_license(),"Panayiotis Lipiridis"));
    for(size_t i=0;i<qso_country_flag_count;++i) {
        const qso_flag_t *flag=&qso_country_flags[i];
        assert(flag->pixels && flag->code[0] && flag->country[0]);
        assert(qso_flag_find(flag->code)==flag);
        assert(qso_flag_find(flag->country)==flag);
    }
    const struct {const char *name,*code;} cases[]={
        {"Malaysia","my"},{"MALAYSIA","my"},{"MY","my"},
        {"Hong Kong","hk"},{"HK","hk"},{"Hongkong","hk"},
        {"Russia","ru"},{"Russian Federation","ru"},{"RU","ru"},
        {"China","cn"},{"United States","us"},{"USA","us"},
        {"United Kingdom","gb"},{"UK","gb"},{"England","gb-eng"},
        {"Scotland","gb-sct"},{"Northern Ireland","gb-nir"},
        {"Czechia","cz"},{"Turkey","tr"},{"Türkiye","tr"},
        {"Bosnia-Herzegovina","ba"},{"North Macedonia","mk"},
        {"South Korea","kr"},{"Republic of Korea","kr"},{"Japan","jp"},
        {"United States of Americ","us"},{"Ivory Coast","ci"},
        {"Namibia","na"},{"NA","na"},
    };
    for(size_t i=0;i<sizeof(cases)/sizeof(*cases);++i) {
        const qso_flag_t *flag=qso_flag_find(cases[i].name);
        assert(flag && !strcmp(flag->code,cases[i].code));
    }
    const char *missing[]={"","Unknown","UNKNOWN COUNTRY","Country unavailable","--","N/A","none","ID 123456","XX","9W3SPY/P"};
    for(size_t i=0;i<sizeof(missing)/sizeof(*missing);++i)assert(!qso_flag_find(missing[i]));
    assert(!qso_flag_find(NULL));
    char long_name[256];memset(long_name,'a',sizeof(long_name)-1);long_name[sizeof(long_name)-1]=0;
    assert(!qso_flag_find(long_name));
    puts("Offline country flags: 256 assets, full names/ISO codes/aliases/case, bounded labels, unavailable-country fallback PASS");
}
