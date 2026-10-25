#include "bm_protocol.h"
#include "bm_qso_store.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bm_qso_info_t decode(const char *fields, bool historical, bool active)
{
    char message[1024];bm_qso_info_t row;
    snprintf(message,sizeof(message),
        "42[\"mqtt\",{\"topic\":\"%s\",\"payload\":{\"DestinationID\":91,"
        "\"SessionID\":\"quality-session\",\"SourceCall\":\"TEST1\","
        "\"Start\":\"1700000000\",\"Stop\":\"%u\"%s}}]",
        historical?"LH-Startup":"LH",active?0:1700000012u,fields);
    assert(bm_decode_metadata(message,91,&row));
    return row;
}

int main(void)
{
    /* Exact shapes/units observed on the public LH-Startup feed; no caller PII.
     * BER conversion matches the official dashboard; loss matches heard.js. */
    const struct {const char *fields;unsigned flags,ber,loss;} cases[]={
        {",\"BER\":0.007092198356986046,\"LossCount\":1,\"TotalCount\":79",3,71,126},
        {",\"BER\":\"0.01234\",\"LossCount\":\"2\",\"TotalCount\":\"100\"",3,123,200},
        {",\"BER\":0,\"LossCount\":0,\"TotalCount\":6",3,0,0},
        {",\"BER\":0,\"LossCount\":0,\"TotalCount\":0",0,0,0},
        {",\"BER\":0.007092198356986046,\"LossCount\":0,\"TotalCount\":0",1,71,0},
        {",\"BER\":null,\"LossCount\":0,\"TotalCount\":22",2,0,0},
        {",\"BER\":-1,\"LossCount\":0,\"TotalCount\":0",0,0,0},
        {",\"BER\":1,\"LossCount\":100,\"TotalCount\":100",3,10000,10000},
        {",\"BER\":1.1,\"LossCount\":11,\"TotalCount\":10",0,0,0},
        {",\"BER\":\"NaN\",\"LossCount\":-1,\"TotalCount\":10",0,0,0},
        {",\"BER\":\"inf\",\"LossCount\":1,\"TotalCount\":null",0,0,0},
        {",\"BER\":true,\"LossCount\":1,\"TotalCount\":0",0,0,0},
        {",\"BER\":\" \",\"LossCount\":\"\",\"TotalCount\":9",0,0,0},
        {",\"BER\":\"0.01oops\",\"LossCount\":1.5,\"TotalCount\":10",0,0,0},
        {",\"LossCount\":1,\"TotalCount\":10.5",0,0,0},
        {",\"LossCount\":1,\"TotalCount\":90000000000000",0,0,0},
        {"",0,0,0},
    };
    for(size_t i=0;i<sizeof(cases)/sizeof(*cases);++i)for(unsigned startup=0;startup<2;++startup) {
        bm_qso_info_t row=decode(cases[i].fields,startup,false);
        assert(row.quality.valid==cases[i].flags);
        assert(row.quality.ber_centi_pct==cases[i].ber);
        assert(row.quality.loss_centi_pct==cases[i].loss);
    }
    bm_qso_store_t store={0};
    bm_qso_info_t live=decode(",\"BER\":0.02,\"LossCount\":1,\"TotalCount\":20",false,true);
    bm_qso_store_apply(&store,&live,1000000,1700000001);
    bm_qso_info_t sparse=decode("",false,true);
    bm_qso_store_apply(&store,&sparse,2000000,1700000002);
    assert(store.current.quality.valid==3 && store.current.quality.ber_centi_pct==200);
    bm_qso_info_t stop=decode(",\"BER\":0,\"LossCount\":2,\"TotalCount\":100",false,false);
    bm_qso_store_apply(&store,&stop,12000000,1700000012);
    assert(store.count==1 && !store.current.active);
    assert(store.history[0].quality.valid==3 && store.history[0].quality.ber_centi_pct==0);
    assert(store.history[0].quality.loss_centi_pct==200);
    sparse.active=false;sparse.ended_us=stop.ended_us;
    bm_qso_store_apply(&store,&sparse,13000000,1700000013);
    assert(store.count==1 && store.history[0].quality.loss_centi_pct==200);
    /* A shorter duplicate report must not replace final session rates. */
    live.active=false;live.ended_us=stop.ended_us-1000000;
    bm_qso_store_apply(&store,&live,14000000,1700000014);
    assert(store.history[0].quality.ber_centi_pct==0 && store.history[0].quality.loss_centi_pct==200);
    size_t offset=0,total=0;bm_qso_info_t page[5];
    assert(bm_qso_store_page(&store,page,5,&offset,&total,NULL)==1 && total==1);
    assert(page[0].quality.valid==3 && page[0].quality.loss_centi_pct==200);
    char text[48];bm_quality_format(text,sizeof(text),&page[0].quality);
    assert(!strcmp(text,"BER 0.00%   LOSS 2.00%"));
    memset(&store,0,sizeof(store));
    stop.historical=true;
    bm_qso_store_apply(&store,&stop,0,1700000013);
    assert(store.count==1 && !store.current.active && store.history[0].quality.valid==3);
    /* Sparse startup followed by final enriched stop fills in rates. */
    memset(&store,0,sizeof(store));sparse.historical=true;
    bm_qso_store_apply(&store,&sparse,0,1700000013);
    bm_qso_store_apply(&store,&stop,0,1700000013);
    assert(store.count==1 && store.history[0].quality.valid==3);
    bm_qso_quality_t missing={0};bm_quality_format(text,sizeof(text),&missing);
    assert(!strcmp(text,"BER --   LOSS --"));
    missing.valid=BM_QUALITY_BER;missing.ber_centi_pct=10000;
    bm_quality_format(text,sizeof(text),&missing);assert(!strcmp(text,"BER 100.00%   LOSS --"));
    /* Values are session-specific, never an attribute of the callsign cache. */
    live=decode("",false,true);strcpy(live.session,"next-session");
    live.started_us+=20000000;
    bm_qso_store_apply(&store,&live,20000000,1700000020);
    assert(store.current.quality.valid==0);
    live.quality=(bm_qso_quality_t){.valid=BM_QUALITY_BER,.ber_centi_pct=25};
    bm_qso_store_apply(&store,&live,21000000,1700000021);
    live.quality=(bm_qso_quality_t){.valid=BM_QUALITY_LOSS,.loss_centi_pct=300};
    bm_qso_store_apply(&store,&live,22000000,1700000022);
    assert(store.current.quality.valid==3 && store.current.quality.ber_centi_pct==25);
    assert(store.current.quality.loss_centi_pct==300);
    /* Observed BM delivery order: start/update; zero-counter stop; final
     * measured stop can arrive later but have an earlier Stop timestamp. */
    memset(&store,0,sizeof(store));
    live=decode(",\"BER\":0,\"LossCount\":0,\"TotalCount\":0",false,true);
    assert(!live.quality.valid);
    bm_qso_store_apply(&store,&live,1000000,1700000001);
    bm_quality_format(text,sizeof(text),&store.current.quality);
    assert(!strcmp(text,"BER --   LOSS --"));
    live=decode(",\"BER\":0.007092198356986046,\"LossCount\":0,\"TotalCount\":0",false,true);
    bm_qso_store_apply(&store,&live,2000000,1700000002);
    assert(store.current.quality.ber_centi_pct==71);
    stop=decode(",\"BER\":0,\"LossCount\":0,\"TotalCount\":0",false,false);
    bm_qso_store_apply(&store,&stop,12000000,1700000012);
    assert(store.count==1 && store.history[0].quality.ber_centi_pct==71);
    assert(!(store.history[0].quality.valid & BM_QUALITY_LOSS));
    bm_qso_info_t measured=decode(",\"BER\":0.0070921984,\"LossCount\":2,\"TotalCount\":100",false,false);
    measured.ended_us-=1000000;
    bm_qso_store_apply(&store,&measured,13000000,1700000013);
    assert(store.count==1 && store.history[0].ended_us==stop.ended_us);
    assert(store.history[0].quality.valid==3 && store.history[0].quality.loss_centi_pct==200);
    assert(store.history[0].quality.sample_count==100);
    /* A later placeholder must not overwrite finalized nonzero values. */
    stop.ended_us+=1000000;
    bm_qso_store_apply(&store,&stop,14000000,1700000014);
    assert(store.history[0].quality.ber_centi_pct==71 && store.history[0].quality.loss_centi_pct==200);
    /* Canonical history explicitly reports null BER: show unavailable,
     * rather than retaining a provisional BER measurement/placeholder. */
    measured=decode(",\"BER\":null,\"LossCount\":2,\"TotalCount\":100",true,false);
    measured.ended_us=stop.ended_us;
    bm_qso_store_apply(&store,&measured,15000000,1700000015);
    assert(store.history[0].quality.valid==BM_QUALITY_LOSS);
    bm_qso_store_page(&store,page,5,&offset,&total,NULL);
    bm_quality_format(text,sizeof(text),&page[0].quality);
    assert(!strcmp(text,"BER --   LOSS 2.00%"));
    /* Completed positive-sample zero is genuine and must remain zero. */
    measured=decode(",\"BER\":0,\"LossCount\":0,\"TotalCount\":101",true,false);
    measured.ended_us=stop.ended_us-1000000;
    bm_qso_store_apply(&store,&measured,16000000,1700000016);
    assert(store.history[0].quality.valid==3 && !store.history[0].quality.ber_centi_pct);
    assert(!store.history[0].quality.loss_centi_pct);
    /* Counterless/null duplicates cannot erase those sampled measurements. */
    measured=decode(",\"BER\":null,\"LossCount\":null,\"TotalCount\":null",true,false);
    measured.ended_us=stop.ended_us+1000000;
    bm_qso_store_apply(&store,&measured,17000000,1700000017);
    assert(store.history[0].quality.valid==3 && store.history[0].quality.sample_count==101);
    char tiny[4];bm_quality_format(tiny,sizeof(tiny),&missing);assert(!strcmp(tiny,"BER"));
    bm_quality_format(NULL,0,&missing);
    printf("BER/loss: startup/live fields, units, invalid/absent/zero, session completion, duplicate merging, paging and format PASS; store=%zu bytes\n",sizeof(store));
    return 0;
}
