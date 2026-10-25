#include "bm_protocol.h"
#include "bm_qso_store.h"
#include "qso_ui_math.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_russian_history(void)
{
    const char *calls[]={"UB3XCM","R2ADU"};
    const char *country_values[]={"","Unknown"," UNKNOWN ","unknown","Unknown country","RU","ru","--","N/A"};
    for(size_t c=0;c<2;++c)for(size_t v=0;v<sizeof(country_values)/sizeof(*country_values);++v) {
        char json[512];bm_qso_info_t item;bm_qso_store_t store={0};
        snprintf(json,sizeof(json),"42[\"mqtt\",{\"payload\":{\"DestinationID\":91,\"SessionID\":\"country-live\",\"SourceCall\":\"%s\",\"SourceCountry\":\"%s\",\"Start\":1700000000}}]",calls[c],country_values[v]);
        assert(bm_decode_metadata(json,91,&item) && !strcmp(item.country,"Russia"));
        bm_qso_store_apply(&store,&item,1000000,1700000001);
        assert(!strcmp(store.current.country,"Russia"));
        /* Sparse Session-Stop must retain the live caller's resolved country. */
        assert(bm_decode_metadata("42[\"mqtt\",{\"payload\":{\"DestinationID\":91,\"SessionID\":\"country-live\",\"Start\":1700000000,\"Stop\":1700000010}}]",91,&item));
        bm_qso_store_apply(&store,&item,11000000,1700000010);
        assert(store.count==1 && !strcmp(store.history[0].country,"Russia"));
        assert(!strcmp(store.history[0].callsign,calls[c]));
        /* BM startup history resolves without first receiving a live call. */
        snprintf(json,sizeof(json),"42[\"mqtt\",{\"topic\":\"LH-Startup\",\"payload\":{\"DestinationID\":91,\"SessionID\":\"country-history\",\"SourceCall\":\"%s\",\"SourceCountry\":\"%s\",\"Start\":\"1700000020\",\"Stop\":\"1700000030\"}}]",calls[c],country_values[v]);
        assert(bm_decode_metadata(json,91,&item) && item.historical);
        bm_qso_store_apply(&store,&item,31000000,1700000030);
        assert(store.count==2 && !strcmp(store.history[0].country,"Russia"));
    }
    /* An explicit supplied country remains authoritative, including portable
     * operators whose present location differs from callsign allocation. */
    bm_qso_info_t item;
    assert(bm_decode_metadata("42[\"mqtt\",{\"payload\":{\"DestinationID\":91,\"SessionID\":\"provided\",\"SourceCall\":\"UB3XCM\",\"SourceCountry\":\"MY\",\"Start\":1700000000}}]",91,&item));
    assert(!strcmp(item.country,"Malaysia"));
    assert(bm_decode_metadata("42[\"mqtt\",{\"payload\":{\"DestinationID\":91,\"SessionID\":\"fallback-provided\",\"SourceCall\":\"UB3XCM\",\"SourceCountry\":\"Unknown\",\"Country\":\"MY\",\"Start\":1700000000}}]",91,&item));
    assert(!strcmp(item.country,"Malaysia"));
    puts("Russian caller/history country tests: PASS");
}

int main(void)
{
    uint32_t source,tg;
    const uint8_t caller[]={0x95,11,0,0xce,0,0x4d,0x15,0x2f,91,0};
    assert(bm_decode_caller(caller,sizeof(caller),&source,&tg) && source==5051695 && tg==91);
    for(size_t n=0;n<sizeof(caller);++n)assert(!bm_decode_caller(caller,n,&source,&tg));
    assert(!bm_decode_caller(NULL,0,&source,&tg));
    test_russian_history();
    const struct {const char *call, *country;} country_cases[]={
        {"9W3SPY","Malaysia"},{"VR2TEST","Hong Kong"},{"bd7test","China"},
        {"BV2TEST","Taiwan"},{"JA1TEST","Japan"},{"K1TEST","United States"},
        {"G0TEST","United Kingdom"},{"ID 123456",""},{"9W3SPY/P",""},
        {"D6TEST",""},{"NOCALL",""},{"UNKNOWN1",""},
        {"UB3XCM","Russia"},{"R2ADU","Russia"},{"ub3xcm","Russia"},
        {"UA1TEST","Russia"},{"UI9TEST","Russia"},{"RA3TEST","Russia"},
        {"UJ8TEST",""},{"UK8TEST",""},{"UN7TEST",""},{"UR5TEST","Ukraine"},
        {"UB3XCM/P",""}
    };
    for(size_t i=0;i<sizeof(country_cases)/sizeof(*country_cases);++i) {
        char json[256];bm_qso_info_t item;
        snprintf(json,sizeof(json),"42[\"mqtt\",{\"payload\":{\"DestinationID\":91,\"SessionID\":\"country-test\",\"SourceCall\":\"%s\",\"Start\":1700000000}}]",country_cases[i].call);
        assert(bm_decode_metadata(json,91,&item));
        assert(!strcmp(item.country,country_cases[i].country));
    }
    uint8_t frame[1024], buffer[1024], samples[960];
    bm_frame_state_t state = {0};
    size_t length;
    frame[0] = 0x92; frame[1] = 20; frame[2] = 0xc5;
    frame[3] = 3; frame[4] = 0xc0;
    memset(frame + 5, 0xff, 960);
    assert(!bm_frame_append(&state, buffer, sizeof(buffer), 2, true, 965, 0, frame, 111, &length));
    assert(bm_frame_append(&state, buffer, sizeof(buffer), 2, true, 965, 111, frame + 111, 854, &length));
    assert(length == 965 && bm_decode_audio(buffer, length, samples, sizeof(samples)) == 960);
    assert(samples[0] == 0xff && samples[959] == 0xff);
    assert(bm_decode_audio(buffer, 964, samples, sizeof(samples)) == 0);
    assert(bm_decode_audio(buffer, 965, samples, 959) == 0);
    // RFC continuation frames with an intervening control frame.
    assert(!bm_frame_append(&state, buffer, sizeof(buffer), 2, false, 100, 0, frame, 100, &length));
    assert(!bm_frame_append(&state, buffer, sizeof(buffer), 9, true, 0, 0, NULL, 0, &length));
    assert(bm_frame_append(&state, buffer, sizeof(buffer), 0, true, 865, 0, frame + 100, 865, &length));
    assert(bm_decode_audio(buffer, length, samples, sizeof(samples)) == 960);
    // Overflow and out-of-order offsets reset assembly without a partial parse.
    assert(!bm_frame_append(&state, buffer, 8, 2, true, 965, 0, frame, 111, &length));
    assert(!bm_frame_append(&state, buffer, sizeof(buffer), 2, true, 965, 7, frame, 111, &length));
    const uint8_t bin8[] = {0x92,20,0xc4,2,0x7f,0xff};
    assert(bm_decode_audio(bin8, sizeof(bin8), samples, sizeof(samples)) == 2);
    const uint8_t str8[] = {0x92,20,0xd9,2,0x7f,0xff};
    assert(bm_decode_audio(str8, sizeof(str8), samples, sizeof(samples)) == 2);
    const uint8_t telemetry[] = {0x92,22,0xca,0,0,0,0};
    assert(bm_decode_audio(telemetry, sizeof(telemetry), samples, sizeof(samples)) == 0);
    bm_qso_info_t qso;
    const char *start = "42[\"mqtt\",{\"topic\":\"LH\",\"payload\":\"{\\\"DestinationID\\\":91,\\\"SessionID\\\":\\\"test-session\\\",\\\"SourceCall\\\":\\\"TEST1\\\",\\\"SourceName\\\":\\\"Test operator\\\",\\\"SourceCountry\\\":\\\"MY\\\",\\\"Start\\\":1700000000,\\\"Stop\\\":0,\\\"Event\\\":\\\"Session-Start\\\"}\"}]";
    assert(bm_decode_metadata(start, 91, &qso));
    assert(qso.active && !strcmp(qso.callsign, "TEST1"));
    assert(!strcmp(qso.name, "Test operator") && !strcmp(qso.country, "Malaysia"));
    assert(qso.started_us == 1700000000000000LL);
    assert(!bm_decode_metadata(start, 46020, &qso));
    const char *stop = "42[\"mqtt\",{\"payload\":{\"DestinationID\":91,\"SessionID\":\"test-session\",\"SourceID\":1234,\"Start\":1700000000,\"Stop\":1700000012,\"Event\":\"Session-Stop\"}}]";
    assert(bm_decode_metadata(stop, 91, &qso));
    assert(!qso.active && !strcmp(qso.callsign, "ID 1234"));
    assert(qso.ended_us - qso.started_us == 12000000);
    assert(!bm_decode_metadata("42[\"mqtt\",{\"payload\":\"invalid JSON\"}]",91,&qso));
    assert(!bm_decode_metadata("42[\"mqtt\",{}]",91,&qso));
    assert(!bm_decode_metadata("40{}",91,&qso));
    // Real BM LH-Startup schema: historical timestamps are quoted strings.
    const char *startup = "42[\"mqtt\",{\"topic\":\"LH-Startup\",\"payload\":{\"DestinationID\":\"91\",\"SessionID\":\"archive-1\",\"SourceCall\":\"OLD1\",\"Start\":\"1700000000\",\"Stop\":\"1700000020\"}}]";
    assert(bm_decode_metadata(startup,91,&qso));
    assert(qso.historical && !qso.active && qso.ended_us-qso.started_us==20000000);
    bm_qso_store_t store={0};
    bm_qso_store_apply(&store,&qso,1000000,1700000030);
    assert(store.count==1 && !store.current.active);
    // Starting a live caller does not add a premature history row.
    assert(bm_decode_metadata(start,91,&qso));
    bm_qso_store_apply(&store,&qso,2000000,1700000002);
    assert(store.current.active && store.count==1);
    bm_qso_info_t partial=qso;partial.callsign[0]=partial.name[0]=partial.country[0]=0;
    bm_qso_store_apply(&store,&partial,2500000,1700000002);
    assert(!strcmp(store.current.callsign,"TEST1") && !strcmp(store.current.country,"Malaysia"));
    assert(!strcmp(store.current.name,"Test operator"));
    // A second startup record must not overwrite the current on-air caller.
    bm_qso_info_t historical;size_t offset=0,total=0;
    assert(bm_qso_store_page(&store,&historical,1,&offset,&total,NULL)==1);
    bm_qso_store_apply(&store,&historical,3000000,1700000003);
    assert(store.current.active && !strcmp(store.current.callsign,"TEST1"));
    assert(bm_decode_metadata(stop,91,&qso));
    // SourceCall is missing in this real-shaped stop; keep the live callsign
    // instead of replacing it with the parser's SourceID fallback.
    bm_qso_store_apply(&store,&qso,14000000,1700000012);
    assert(!store.current.active && store.count==2);
    bool kept_callsign=false;
    for(size_t i=0;i<store.count;++i) if(!strcmp(store.history[i].session,"test-session"))
        kept_callsign=!strcmp(store.history[i].callsign,"TEST1");
    assert(kept_callsign);
    size_t completed=store.count;
    bm_qso_store_apply(&store,&qso,15000000,1700000013);
    assert(store.count==completed);
    assert(bm_decode_metadata(start,91,&qso));
    bm_qso_store_apply(&store,&qso,16000000,1700000014);
    assert(!store.current.active); // Late start must not resurrect a closed QSO.
    for(unsigned i=1;i<=60;++i) {
        historical.started_us=1700000040000000LL+i*1000000LL;
        historical.ended_us=historical.started_us+3000000;
        snprintf(historical.session,sizeof(historical.session),"archive-%u",i+1);
        bm_qso_store_apply(&store,&historical,0,1700000100);
    }
    assert(store.count==15);
    bm_qso_info_t page[5];offset=0;
    assert(bm_qso_store_page(&store,page,5,&offset,&total,NULL)==5 && total==15);
    assert(!strcmp(page[0].session,"archive-61"));
    offset=1;
    assert(bm_qso_store_page(&store,page,5,&offset,&total,NULL)==5);
    assert(!strcmp(page[0].session,"archive-60"));
    char anchor[64];snprintf(anchor,sizeof(anchor),"%s",page[0].session);
    historical.started_us+=1000000;historical.ended_us+=1000000;
    strcpy(historical.session,"new-completed");
    bm_qso_store_apply(&store,&historical,0,1700000200);
    assert(store.count==15);
    assert(bm_qso_store_page(&store,page,5,&offset,&total,anchor)==5 && offset==2);
    assert(!strcmp(page[0].session,anchor));
    offset=999;
    assert(bm_qso_store_page(&store,page,5,&offset,&total,NULL)==5 && offset==10);
    assert(!strcmp(page[4].session,"archive-48"));
    size_t duplicate_count=store.count;
    bm_qso_store_apply(&store,&historical,0,1700000200);
    assert(store.count==duplicate_count);
    /* A late startup from outside the retained window must not displace rows. */
    historical.started_us=1600000000000000LL;strcpy(historical.session,"too-old");
    bm_qso_store_apply(&store,&historical,0,1700000200);
    offset=0;bm_qso_store_page(&store,page,5,&offset,&total,NULL);
    assert(!strcmp(page[0].session,"new-completed"));
    printf("History: 15 unique rows, one-entry paging, anchored inserts, bounds PASS; store=%zu bytes\n",sizeof(store));
    for(size_t i=1;i<store.count;++i) assert(store.history[i-1].started_us>=store.history[i].started_us);
    puts("History tests: PASS (BM startup, live-to-history, duplicate/late events, newest 15 / five visible)");
    puts("BrandMeister protocol tests: PASS (chunks, continuations, bounds, audio formats, nested metadata, TG filter)");
    return 0;
}
