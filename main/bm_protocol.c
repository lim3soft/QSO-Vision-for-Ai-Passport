#include "bm_protocol.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

/* BM often omits country. These common prefixes identify the callsign's
 * allocating administration, not the operator's current physical location.
 * Source: https://www.itu.int/gladapp/Allocation/CallSigns (2026-10-24).
 * Unknown and portable calls are deliberately left unresolved. */
static void normalize_country(char *out)
{
    /* A server placeholder is still missing data; allow the prefix fallback.
     * Keep actual country names/codes authoritative. */
    size_t first=0,last=strlen(out);
    while(first<last && isspace((unsigned char)out[first]))++first;
    while(last>first && isspace((unsigned char)out[last-1]))--last;
    memmove(out,out+first,last-first);out[last-first]=0;
    static const char *const missing[]={"unknown","unknown country","country unavailable","unavailable","unk","n/a","--","-","null","none"};
    for(size_t i=0;i<sizeof(missing)/sizeof(*missing);++i) {
        size_t j=0;
        while(out[j] && missing[i][j] && tolower((unsigned char)out[j])==missing[i][j])++j;
        if(!out[j] && !missing[i][j]){out[0]=0;break;}
    }
}

void bm_country_label(char *out, size_t size, const char *callsign)
{
    normalize_country(out);
    static const struct { const char *iso, *name, *prefixes; } countries[] = {
        {"MY","Malaysia","9M,9W"}, {"HK","Hong Kong","VR"},
        {"CN","China","B"}, {"SG","Singapore","9V"},
        {"TW","Taiwan","BV,BW,BX"}, {"JP","Japan","JA,JB,JC,JD,JE,JF,JG,JH,JI,JJ,JK,JL,JM,JN,JO,JP,JQ,JR,JS,7J,7K,7L,7M,7N,8J,8K,8L,8M,8N"},
        {"ID","Indonesia","YB,YC,YD,YE,YF,YG,YH,7A,7B,7C,7D,7E,7F,7G,7H,7I,8A,8B,8C,8D,8E,8F,8G,8H,8I"},
        {"TH","Thailand","HS,E2"}, {"PH","Philippines","DU,DV,DW,DX,DY,DZ,4D,4E,4F,4G,4H,4I"},
        {"AU","Australia","VK,VI,VJ,VH,VL,VM,VN,VZ"}, {"NZ","New Zealand","ZL,ZM"},
        {"KR","South Korea","HL,DS,DT,D7,D8,D9,6K,6L,6M,6N"},
        {"US","United States","K,N,W,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL"},
        {"CA","Canada","VA,VB,VC,VD,VE,VF,VG,VO,VX,VY"},
        {"GB","United Kingdom","G,M,2E,2D,2I,2J,2M,2U,2W"},
        {"RU","Russia","R,UA,UB,UC,UD,UE,UF,UG,UH,UI"},
        {"DE","Germany","DA,DB,DC,DD,DE,DF,DG,DH,DI,DJ,DK,DL,DM,DN,DO,DP,DQ,DR"},
        {"FR","France","F,TM,TO,TP,TQ"}, {"IT","Italy","I"},
        {"ES","Spain","EA,EB,EC,ED,EE,EF,EG,EH"}, {"PT","Portugal","CT,CS,CQ,CR"},
        {"NL","Netherlands","PA,PB,PC,PD,PE,PF,PG,PH,PI"}, {"BE","Belgium","ON,OO,OP,OQ,OR,OS,OT"},
        {"CH","Switzerland","HB,HE"}, {"AT","Austria","OE"},
        {"PL","Poland","SP,SQ,SO,SN,SR,3Z"}, {"CZ","Czechia","OK,OL"},
        {"ZA","South Africa","ZR,ZS,ZT,ZU"}, {"IN","India","VU,VT,VV,VW"},
        {"BR","Brazil","PP,PQ,PR,PS,PT,PU,PV,PW,PX,PY,ZV,ZW,ZX,ZY,ZZ"},
        {"CO","Colombia","HJ,HK,5J,5K"}, {"UA","Ukraine","UR,US,UT,UU,UV,UW,UX,UY,UZ,EM,EN,EO"},
        {"FI","Finland","OF,OG,OH,OI,OJ"}, {"SE","Sweden","SA,SB,SC,SD,SE,SF,SG,SH,SI,SJ,SK,SL,SM,7S,8S"},
        {"NO","Norway","LA,LB,LC,LD,LE,LF,LG,LH,LI,LJ,LK,LL,LM,LN,JW,JX,3Y"},
        {"DK","Denmark","OU,OV,OW,5P,5Q"}, {"RO","Romania","YO,YP,YQ,YR"},
        {"GR","Greece","SV,SW,SX,SY,SZ,J4"}, {"HU","Hungary","HA,HG"},
        {"SK","Slovakia","OM"}, {"SI","Slovenia","S5"}, {"HR","Croatia","9A"},
        {"LT","Lithuania","LY"}, {"LV","Latvia","YL"}, {"EE","Estonia","ES"},
        {"BG","Bulgaria","LZ"}, {"RS","Serbia","YT,YU"}, {"BA","Bosnia-Herzegovina","E7"},
        {"ME","Montenegro","4O"}, {"MK","North Macedonia","Z3"}, {"IE","Ireland","EI,EJ"},
        {"IS","Iceland","TF"}, {"LU","Luxembourg","LX"}, {"MT","Malta","9H"},
        {"MD","Moldova","ER"}, {"BY","Belarus","EU,EV,EW"}, {"TR","Turkey","TA,TB,TC,YM"},
        {"AR","Argentina","LU,LV,LW,AY,AZ,L2,L3,L4,L5,L6,L7,L8,L9"},
        {"MX","Mexico","XE,XF,4A,4B,4C,6D,6E,6F,6G,6H,6I,6J"},
        {"CL","Chile","CA,CB,CC,CD,CE,XQ,XR,3G"}, {"PE","Peru","OA,OB,OC,4T"},
        {"EC","Ecuador","HC,HD"}, {"UY","Uruguay","CV,CW,CX"}, {"PY","Paraguay","ZP"},
        {"BO","Bolivia","CP"}, {"VE","Venezuela","YV,YW,4M"},
        {"CR","Costa Rica","TE,TI"}, {"PA","Panama","HO,HP,3E,3F"}, {"CU","Cuba","CL,CM,CO,T4"},
        {"MA","Morocco","CN,5C,5D,5E,5F,5G"}, {"DZ","Algeria","7R,7T,7U,7V,7W,7X,7Y"},
        {"TN","Tunisia","3V,TS"}, {"EG","Egypt","SU,6A,6B"}, {"IL","Israel","4X,4Z"},
        {"AE","United Arab Emirates","A6"}, {"SA","Saudi Arabia","HZ,7Z,8Z"},
        {"KW","Kuwait","9K"}, {"QA","Qatar","A7"}, {"OM","Oman","A4"}, {"BH","Bahrain","A9"},
        {"VN","Vietnam","XV,3W"}, {"KH","Cambodia","XU"}, {"LK","Sri Lanka","4P,4Q,4R,4S"},
        {"BD","Bangladesh","S2,S3"}, {"PK","Pakistan","AP,AQ,AR,AS,6P,6Q,6R,6S"},
        {"NP","Nepal","9N"}, {"KE","Kenya","5Y,5Z"}, {"NG","Nigeria","5N,5O"}, {"GH","Ghana","9G"},
    };
    if (out[0]) {
        for (size_t i=0;i<sizeof(countries)/sizeof(*countries);++i)
            if (strlen(out)==2 && toupper((unsigned char)out[0])==countries[i].iso[0] &&
                toupper((unsigned char)out[1])==countries[i].iso[1]) {
                snprintf(out,size,"%s",countries[i].name);
                break;
            }
        return;
    }
    if (!callsign[0] || strchr(callsign,'/') || !strncmp(callsign,"ID ",3)) return;
    bool digit=false;
    for (const char *p=callsign;*p;++p) {
        if (!isalnum((unsigned char)*p)) return;
        if (isdigit((unsigned char)*p)) digit=true;
    }
    if (!digit) return;
    /* Most specific first, so BV does not resolve to the generic B range. */
    for (size_t length=2;length>0;--length) {
        for (size_t i=0;i<sizeof(countries)/sizeof(*countries);++i) {
            const char *p=countries[i].prefixes;
            while (*p) {
                const char *end=strchr(p,',');
                size_t n=end?(size_t)(end-p):strlen(p);
                if (n==length && strlen(callsign)>n) {
                    size_t j=0;
                    while(j<n && toupper((unsigned char)callsign[j])==p[j]) ++j;
                    if(j==n) {snprintf(out,size,"%s",countries[i].name);return;}
                }
                if(!end) break;
                p=end+1;
            }
        }
    }
}

bool bm_frame_append(bm_frame_state_t *state, uint8_t *buffer, size_t capacity,
                     uint8_t opcode, bool fin, size_t payload_length,
                     size_t offset, const void *data, size_t length,
                     size_t *message_length)
{
    if (!state || !buffer || !message_length || (!data && length)) return false;
    *message_length = 0;
    if (opcode >= 8) return false;
    if (offset == 0) {
        state->frame_offset = 0;
        if (opcode == 1 || opcode == 2) {
            state->used = 0;
            state->opcode = opcode;
            state->assembling = true;
        } else if (opcode != 0 || !state->assembling) return false;
    }
    if (!state->assembling || offset != state->frame_offset ||
        offset > payload_length || length > payload_length - offset ||
        state->used >= capacity || length >= capacity - state->used) {
        state->assembling = false;
        state->used = 0;
        return false;
    }
    if (length) memcpy(buffer + state->used, data, length);
    state->used += length;
    state->frame_offset += length;
    if (state->frame_offset != payload_length || !fin) return false;
    buffer[state->used] = 0;
    *message_length = state->used;
    state->assembling = false;
    return true;
}

size_t bm_decode_audio(const uint8_t *message, size_t length,
                        uint8_t *samples, size_t capacity)
{
    if (!message || !samples || length < 3 || message[0] != 0x92 || message[1] != 20)
        return 0;
    size_t header = 3, count = 0;
    uint8_t tag = message[2];
    if ((tag & 0xe0) == 0xa0) count = tag & 0x1f;
    else {
        size_t bytes = tag == 0xc4 || tag == 0xd9 ? 1 :
                       tag == 0xc5 || tag == 0xda ? 2 :
                       tag == 0xc6 || tag == 0xdb ? 4 : 0;
        if (!bytes || length < header + bytes) return 0;
        for (size_t i = 0; i < bytes; ++i) count = (count << 8) | message[header++];
    }
    if (!count || count > capacity || count != length - header) return 0;
    memcpy(samples, message + header, count);
    return count;
}

/* Hoseline's observed [11,slot,SourceID,TG,flags] caller header. No allocation. */
static bool msgpack_uint(const uint8_t **p, const uint8_t *end, uint32_t *value)
{
    if(*p>=end)return false;
    uint8_t tag=*(*p)++;
    if(tag<128){*value=tag;return true;}
    unsigned n=tag==0xcc?1:tag==0xcd?2:tag==0xce?4:0;
    if(!n || (size_t)(end-*p)<n)return false;
    *value=0;
    while(n--)*value=(*value<<8)|*(*p)++;
    return true;
}

bool bm_decode_caller(const uint8_t *message, size_t length, uint32_t *source, uint32_t *talkgroup)
{
    if(!message || !source || !talkgroup || length<6 || message[0]!=0x95 || message[1]!=11)return false;
    const uint8_t *p=message+2,*end=message+length;
    uint32_t slot,flags,id,tg;
    if(!msgpack_uint(&p,end,&slot) || !msgpack_uint(&p,end,&id) ||
       !msgpack_uint(&p,end,&tg) || !msgpack_uint(&p,end,&flags) || p!=end || !id || !tg)return false;
    *source=id;*talkgroup=tg;return true;
}

static void text_field(char *target, size_t size, const cJSON *object, const char *key)
{
    const cJSON *field = cJSON_GetObjectItemCaseSensitive(object, key);
    snprintf(target, size, "%s", cJSON_IsString(field) && field->valuestring ? field->valuestring : "");
}

/* LH live records use JSON numbers; LH-Startup history uses quoted epochs. */
static double number_field(const cJSON *object, const char *key)
{
    const cJSON *field = cJSON_GetObjectItemCaseSensitive(object, key);
    double value = -1;
    if (cJSON_IsNumber(field)) value = field->valuedouble;
    else if (cJSON_IsString(field) && field->valuestring[0]) {
        char *end;
        value = strtod(field->valuestring, &end);
        if (end == field->valuestring || *end) return -1;
    }
    return isfinite(value) && value >= 0 && value <= 9000000000000.0 ? value : -1;
}

static bm_qso_quality_t decode_quality(const cJSON *payload)
{
    bm_qso_quality_t quality={0};
    /* BM live starts/stops can contain BER=0, LossCount=TotalCount=0
     * before the final report. Do not label that placeholder as measured BER.
     * Nonzero BER updates are useful even before packet counters arrive. */
    double ber=number_field(payload,"BER");
    double loss=number_field(payload,"LossCount");
    double total=number_field(payload,"TotalCount");
    if(total>0 && total<=UINT32_MAX && (double)(uint32_t)total==total)
        quality.sample_count=(uint32_t)total;
    if (ber>=0 && ber<=1 && (ber>0 || quality.sample_count>0)) {
        quality.ber_centi_pct=(uint16_t)(ber*10000.0+0.5);
        quality.valid |= BM_QUALITY_BER;
        quality.reported |= BM_QUALITY_BER;
    } else if(cJSON_GetObjectItemCaseSensitive(payload,"BER") && ber!=0) {
        /* Explicit null/-1 must clear a provisional value; omission must not. */
        quality.reported |= BM_QUALITY_BER;
    }
    if (quality.sample_count>0 && loss>=0 && loss<=total && (double)(uint64_t)loss==loss) {
        quality.loss_centi_pct=(uint16_t)(loss/total*10000.0);
        quality.valid |= BM_QUALITY_LOSS;
        quality.reported |= BM_QUALITY_LOSS;
    } else if(cJSON_GetObjectItemCaseSensitive(payload,"LossCount") &&
              cJSON_GetObjectItemCaseSensitive(payload,"TotalCount") &&
              !(loss==0 && total==0)) {
        quality.reported |= BM_QUALITY_LOSS;
    }
    return quality;
}

bool bm_decode_metadata(const char *message, int talkgroup, bm_qso_info_t *out)
{
    if (!message || !out || strncmp(message, "42", 2)) return false;
    bool result = false;
    cJSON *envelope = cJSON_Parse(message + 2);
    cJSON *decoded = NULL;
    const cJSON *kind = cJSON_GetArrayItem(envelope, 0);
    const cJSON *payload = cJSON_GetArrayItem(envelope, 1);
    if (!cJSON_IsString(kind) || strcmp(kind->valuestring, "mqtt")) goto done;
    const cJSON *topic = cJSON_GetObjectItemCaseSensitive(payload, "topic");
    bool historical = cJSON_IsString(topic) && !strcmp(topic->valuestring, "LH-Startup");
    /* Dashboard sends mqtt -> {topic:"LH", payload: JSON string/object}. */
    const cJSON *nested = cJSON_GetObjectItemCaseSensitive(payload, "payload");
    if (nested) payload = nested;
    if (cJSON_IsString(payload)) {
        decoded = cJSON_Parse(payload->valuestring);
        payload = decoded;
    }
    double tg = number_field(payload, "DestinationID");
    double start = number_field(payload, "Start");
    double stop = number_field(payload, "Stop");
    const cJSON *event = cJSON_GetObjectItemCaseSensitive(payload, "Event");
    if (tg != talkgroup || start <= 0 || (stop > 0 && stop < start)) goto done;
    memset(out, 0, sizeof(*out));
    double source=number_field(payload,"SourceID");
    if(source>0 && source<=UINT32_MAX && (double)(uint32_t)source==source)out->source_id=(uint32_t)source;
    text_field(out->session, sizeof(out->session), payload, "SessionID");
    if (!out->session[0]) goto done;
    text_field(out->callsign, sizeof(out->callsign), payload, "SourceCall");
    text_field(out->name, sizeof(out->name), payload, "SourceName");
    text_field(out->country, sizeof(out->country), payload, "SourceCountry");
    normalize_country(out->country);
    if (!out->country[0]) text_field(out->country, sizeof(out->country), payload, "Country");
    if (!out->callsign[0]) {
        const cJSON *id = cJSON_GetObjectItemCaseSensitive(payload, "SourceID");
        if (cJSON_IsNumber(id)) snprintf(out->callsign, sizeof(out->callsign), "ID %d", id->valueint);
    }
    bm_country_label(out->country, sizeof(out->country), out->callsign);
    out->quality = decode_quality(payload);
    out->historical = historical;
    out->started_us = (int64_t)(start * 1000000.0);
    out->ended_us = stop > 0 ? (int64_t)(stop * 1000000.0) : 0;
    out->active = out->ended_us == 0 && !(cJSON_IsString(event) && !strcmp(event->valuestring, "Session-Stop"));
    result = true;
done:
    cJSON_Delete(decoded);
    cJSON_Delete(envelope);
    return result;
}

/* Cheap rejection before cJSON allocation on the worldwide LH feed. Unknown
 * envelopes still go through the validating JSON parser; never guess a TG. */
bool bm_metadata_may_match(const char *message,int talkgroup)
{
    if(!message)return false;
    const char *p=strstr(message,"DestinationID");
    if(!p)return true;
    p+=strlen("DestinationID");
    while(*p=='\\' || *p=='\"' || isspace((unsigned char)*p))++p;
    if(*p++!=':')return true;
    while(*p=='\\' || *p=='\"' || isspace((unsigned char)*p))++p;
    if(!isdigit((unsigned char)*p))return true;
    uint32_t tg=0;
    while(isdigit((unsigned char)*p)) {
        if(tg>16777215)return true;
        tg=tg*10+(unsigned)(*p++-'0');
    }
    return tg==(uint32_t)talkgroup;
}
