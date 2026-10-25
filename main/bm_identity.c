#include "bm_identity.h"
#include "cJSON.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

bool bm_callsign_known(const char *text)
{
    if (!text || !text[0] || !strncmp(text,"ID ",3)) return false;
    bool digit=false, letter=false;
    size_t n=0;
    for (; text[n]; ++n) {
        unsigned char c=(unsigned char)text[n];
        if (c>127 || (!isalnum(c) && c!='/' && c!='-')) return false;
        digit |= isdigit(c)!=0; letter |= isalpha(c)!=0;
    }
    return digit && letter && n>=3 && n<24;
}

bool bm_identity_get(const bm_identity_cache_t *cache, uint32_t id, bm_identity_t *out)
{
    if (!cache || !id || !out) return false;
    for (size_t i=0;i<BM_IDENTITY_CACHE_SIZE;++i)
        if (cache->entries[i].id==id && bm_callsign_known(cache->entries[i].callsign)) {
            *out=cache->entries[i]; return true;
        }
    return false;
}

void bm_identity_put(bm_identity_cache_t *cache, const bm_identity_t *item)
{
    if (!cache || !item || !item->id || !bm_callsign_known(item->callsign)) return;
    bm_identity_t normalized=*item;
    bm_country_label(normalized.country,sizeof(normalized.country),normalized.callsign);
    item=&normalized;
    size_t slot=BM_IDENTITY_CACHE_SIZE;
    for(size_t i=0;i<BM_IDENTITY_CACHE_SIZE;++i)if(cache->entries[i].id==item->id){slot=i;break;}
    if(slot==BM_IDENTITY_CACHE_SIZE) {
        slot=cache->next++%BM_IDENTITY_CACHE_SIZE;
        memset(&cache->entries[slot],0,sizeof(cache->entries[slot]));
    }
    bm_identity_t *old=&cache->entries[slot];
    old->id=item->id;
    snprintf(old->callsign,sizeof(old->callsign),"%.23s",item->callsign);
    if(item->name[0])snprintf(old->name,sizeof(old->name),"%.39s",item->name);
    if(item->country[0])snprintf(old->country,sizeof(old->country),"%.23s",item->country);
}

void bm_identity_enrich(const bm_identity_cache_t *cache, bm_qso_info_t *qso)
{
    bm_identity_t person;
    if(!qso)return;
    bm_country_label(qso->country,sizeof(qso->country),qso->callsign);
    if(bm_identity_get(cache,qso->source_id,&person)) {
        if(!bm_callsign_known(qso->callsign))snprintf(qso->callsign,sizeof(qso->callsign),"%s",person.callsign);
        if(!qso->name[0])snprintf(qso->name,sizeof(qso->name),"%s",person.name);
        if(!qso->country[0])snprintf(qso->country,sizeof(qso->country),"%s",person.country);
    }
    bm_country_label(qso->country,sizeof(qso->country),qso->callsign);
}

static void field(char *out,size_t capacity,const cJSON *o,const char *key)
{
    const cJSON *v=cJSON_GetObjectItemCaseSensitive(o,key);
    snprintf(out,capacity,"%s",cJSON_IsString(v)?v->valuestring:"");
}

bool bm_identity_decode(const char *json,uint32_t expected_id,bm_identity_t *out)
{
    if(!json || !expected_id || !out)return false;
    bool ok=false;
    cJSON *root=cJSON_Parse(json);
    const cJSON *results=cJSON_GetObjectItemCaseSensitive(root,"results");
    const cJSON *row=cJSON_GetArrayItem(results,0);
    const cJSON *id=cJSON_GetObjectItemCaseSensitive(row,"id");
    if(!cJSON_IsNumber(id) || id->valuedouble!=expected_id)goto done;
    memset(out,0,sizeof(*out));out->id=expected_id;
    field(out->callsign,sizeof(out->callsign),row,"callsign");
    field(out->name,sizeof(out->name),row,"fname");
    if(!out->name[0])field(out->name,sizeof(out->name),row,"name");
    field(out->country,sizeof(out->country),row,"country");
    ok=bm_callsign_known(out->callsign);
done:
    cJSON_Delete(root);return ok;
}

bool bm_identity_alias(const uint8_t *message,size_t length,uint32_t id,bm_identity_t *out)
{
    if(!message || !out || !id || length<4 || message[0]!=0x92 || message[1]!=21)return false;
    size_t header=3,n=0;
    if((message[2]&0xe0)==0xa0)n=message[2]&31;
    else if(message[2]==0xd9 && length>=4)n=message[header++];
    else if(message[2]==0xda && length>=5){n=(size_t)message[3]<<8|message[4];header=5;}
    if(!n || n!=length-header || n>128)return false;
    char alias[129];memcpy(alias,message+header,n);alias[n]=0;
    char *space=strchr(alias,' ');if(space)*space++=0;
    if(!bm_callsign_known(alias))return false;
    memset(out,0,sizeof(*out));out->id=id;
    snprintf(out->callsign,sizeof(out->callsign),"%.23s",alias);
    if(space)snprintf(out->name,sizeof(out->name),"%.39s",space);
    bm_country_label(out->country,sizeof(out->country),out->callsign);
    return true;
}
