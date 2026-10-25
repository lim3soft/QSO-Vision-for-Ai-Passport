#include "bm_identity.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    bm_identity_cache_t cache={0};bm_identity_t person;
    const char *body="{\"count\":1,\"results\":[{\"id\":5020879,\"callsign\":\"9M2FRD\",\"fname\":\"Firdaus\",\"country\":\"Malaysia\"}]}";
    assert(bm_identity_decode(body,5020879,&person));
    assert(!strcmp(person.callsign,"9M2FRD"));bm_identity_put(&cache,&person);
    assert(!bm_identity_decode(body,123456,&person));
    assert(!bm_identity_decode("{\"results\":[]}",5020879,&person));
    assert(!bm_identity_decode("broken",5020879,&person));
    assert(!bm_callsign_known("Unknown") && !bm_callsign_known("ID 5020879"));
    assert(bm_callsign_known("9M2FRD") && bm_callsign_known("UB3XCM"));
    static const struct {const char *call,*country;} countries[]={
        {"HK1E","Colombia"},{"VR2ABC","Hong Kong"},{"UR5FEL","Ukraine"},
        {"OH7UDH","Finland"},{"SM7ABC","Sweden"},{"LZ2ABC","Bulgaria"},
        {"9H1AA","Malta"},{"EC3GBT","Spain"},{"CT2FPN","Portugal"}};
    for(size_t i=0;i<sizeof(countries)/sizeof(*countries);++i) {
        bm_qso_info_t row={0};strcpy(row.callsign,countries[i].call);strcpy(row.country,"Unknown");
        bm_identity_enrich(&cache,&row);assert(!strcmp(row.country,countries[i].country));
    }
    bm_identity_t hk={.id=7300001};strcpy(hk.callsign,"HK1E");strcpy(hk.country,"Unknown");
    bm_identity_put(&cache,&hk);assert(bm_identity_get(&cache,hk.id,&person));
    assert(!strcmp(person.country,"Colombia"));
    /* Reproduce the video: A resolved -> B -> sparse ID-only A. */
    bm_identity_t other={.id=2355555};strcpy(other.callsign,"M7HYH");bm_identity_put(&cache,&other);
    bm_qso_info_t shown={.source_id=5020879};strcpy(shown.callsign,"ID 5020879");
    bm_identity_enrich(&cache,&shown);
    assert(!strcmp(shown.callsign,"9M2FRD") && !strcmp(shown.country,"Malaysia"));
    bm_identity_t partial={.id=5020879};strcpy(partial.callsign,"9M2FRD");bm_identity_put(&cache,&partial);
    assert(bm_identity_get(&cache,5020879,&person) && !strcmp(person.name,"Firdaus"));
    const uint8_t alias[]={0x92,21,0xb3,'O','H','7','U','D','H',' ','S','a','v','o','n',' ','V','o','i','m','a','x'};
    /* Exact live alias format with strict payload boundaries. */
    uint8_t actual[sizeof(alias)-1];memcpy(actual,alias,sizeof(actual));actual[2]=18|0xa0;
    assert(bm_identity_alias(actual,sizeof(actual),2441318,&person));
    assert(!strcmp(person.callsign,"OH7UDH") && !strcmp(person.name,"Savon Voima") && !strcmp(person.country,"Finland"));
    assert(!bm_identity_alias(actual,sizeof(actual)-1,2441318,&person));
    assert(!bm_identity_alias(actual,sizeof(actual),0,&person));
    assert(bm_metadata_may_match("42[\"mqtt\",{\"DestinationID\":91}]",91));
    assert(!bm_metadata_may_match("42[\"mqtt\",{\"DestinationID\":910}]",91));
    assert(!bm_metadata_may_match("42[\"mqtt\",{\"payload\":\"{\\\"DestinationID\\\":46001}\"}]",91));
    assert(bm_metadata_may_match("42[\"mqtt\",{\"payload\":\"{\\\"DestinationID\\\":91}\"}]",91));
    assert(bm_metadata_may_match("42[\"searchHouseComplete\"]",91));
    puts("Identity cache/directory/alias and pre-allocation TG filter: PASS");return 0;
}
