/* Exercise the firmware supervisor and callbacks with fake sockets/queues.
 * No actual RF, Wi-Fi, TLS, FreeRTOS scheduling or speaker is simulated. */
#include "hoseline_test_stubs.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#ifndef QSO_AUDIO_IMPLEMENTATION
#define QSO_AUDIO_IMPLEMENTATION "../main/hoseline_audio.c"
#endif
#include QSO_AUDIO_IMPLEMENTATION

struct test_socket { bool started, connected; test_callback_t callback; unsigned tg, starts, stops; };
struct test_queue { unsigned count,limit,head; size_t size; unsigned char last[2048],ring[6][2048]; };
static int64_t clock_us=1000000;
static bool fail_stop, fail_start;
static unsigned writes;
static bool switch_during_write;
static bool replay_mode,replay_jitter;
static unsigned replay_next,replay_zero_blocks,replay_samples;
static int64_t replay_base,replay_dma_until;
static jmp_buf replay_done;
static void replay_advance(int64_t target);
void *esp_crt_bundle_attach;
void vTaskDelay(TickType_t ticks) {
    if(replay_mode)replay_advance(clock_us+(int64_t)ticks*1000);
    else clock_us+=(int64_t)ticks*1000;
}
static unsigned directory_requests;
bool bm_identity_lookup(uint32_t id,bm_identity_t *out) {
    assert(!s_metadata_client);++directory_requests;
    if(id==6300001){memset(out,0,sizeof(*out));out->id=id;strcpy(out->callsign,"ZC4AA");strcpy(out->country,"Cyprus");return true;}
    if(id==3139516){memset(out,0,sizeof(*out));out->id=id;strcpy(out->callsign,"KM4SAB");strcpy(out->country,"United States");return true;}
    if(id!=5020879)return false;
    memset(out,0,sizeof(*out));out->id=id;strcpy(out->callsign,"9M2FRD");
    strcpy(out->name,"Firdaus");strcpy(out->country,"Malaysia");return true;
}
const char *esp_err_to_name(esp_err_t err) { return err==ESP_OK?"OK":"FAIL"; }
int64_t esp_timer_get_time(void) { return clock_us; }
size_t heap_caps_get_free_size(int caps) { (void)caps; return 100000; }
size_t heap_caps_get_largest_free_block(int caps) { (void)caps; return 60000; }
esp_err_t bsp_audio_write(const void *data,size_t size) {
    (void)data;assert(size<=320);++writes;
    if(replay_mode) {
        int64_t duration=(int64_t)(size/2)*125;
        if(replay_dma_until+duration>clock_us+180000)
            replay_advance(replay_dma_until+duration-180000);
        if(replay_dma_until<clock_us)replay_dma_until=clock_us;
        replay_dma_until+=duration;replay_samples+=(unsigned)(size/2);
        const int16_t *pcm=data;bool zero=true;
        for(size_t i=0;i<size/2;++i)if(pcm[i])zero=false;
        if(zero)++replay_zero_blocks;
    }
    if(switch_during_write){switch_during_write=false;bm_audio_select(45401);}
    return ESP_OK;
}
QueueHandle_t xQueueCreate(unsigned count,size_t size) { assert(count<=6);QueueHandle_t q=calloc(1,sizeof(*q));q->size=size;q->limit=count;return q; }
void vQueueDelete(QueueHandle_t q) { free(q); }
int xQueueSend(QueueHandle_t q,const void *data,TickType_t wait) { (void)wait;assert(q->size<=sizeof(q->last));if(q->count==q->limit)return 0;memcpy(q->last,data,q->size);memcpy(q->ring[(q->head+q->count)%q->limit],data,q->size);++q->count;return pdTRUE; }
int xQueueReceive(QueueHandle_t q,void *data,TickType_t wait) {
    if(replay_mode && wait && !q->count) {
        if(replay_next==20)longjmp(replay_done,1);
        int64_t next=replay_base+(int64_t)replay_next*120000;
        if(replay_jitter && (replay_next==7 || replay_next==8))next=replay_base+1006000;
        int64_t deadline=clock_us+(int64_t)wait*1000;
        replay_advance(next<deadline?next:deadline);
    }
    if(!q->count)return 0;
    memcpy(data,q->ring[q->head],q->size);q->head=(q->head+1)%q->limit;--q->count;return pdTRUE;
}
int xQueueReset(QueueHandle_t q) { q->count=q->head=0;return pdTRUE; }
int xTaskCreate(void (*fn)(void *),const char *name,unsigned stack,void *arg,int priority,TaskHandle_t *task)
{ (void)fn;(void)name;(void)stack;(void)arg;(void)priority;*task=(void*)1;return pdPASS; }
esp_websocket_client_handle_t esp_websocket_client_init(const esp_websocket_client_config_t *config)
{ assert(config->crt_bundle_attach==esp_crt_bundle_attach);return calloc(1,sizeof(struct test_socket)); }
esp_err_t esp_websocket_client_destroy(esp_websocket_client_handle_t c) { assert(!c->started);free(c);return ESP_OK; }
esp_err_t esp_websocket_register_events(esp_websocket_client_handle_t c,int event,test_callback_t callback,void *arg)
{ (void)event;(void)arg;c->callback=callback;return ESP_OK; }
esp_err_t esp_websocket_client_start(esp_websocket_client_handle_t c)
{ assert(!c->started);if(fail_start)return ESP_FAIL;c->started=true;++c->starts;return ESP_OK; }
esp_err_t esp_websocket_client_stop(esp_websocket_client_handle_t c)
{
    assert(c->started);
    if(fail_stop && c==s_client)return ESP_FAIL;
    /* Releasing metadata must precede audio TLS replacement. */
    if(c==s_client)assert(!s_metadata_client);
    c->started=c->connected=false;++c->stops;
    c->callback(NULL,NULL,WEBSOCKET_EVENT_DISCONNECTED,NULL);
    return ESP_OK;
}
bool esp_websocket_client_is_connected(esp_websocket_client_handle_t c) { return c->connected; }
int esp_websocket_client_send_bin(esp_websocket_client_handle_t c,const char *data,int size,TickType_t wait)
{
    (void)wait;assert(c->connected && size==8);
    const uint8_t *p=(const uint8_t*)data;
    assert(p[0]==0x92 && p[1]==1 && p[2]==0x91 && p[3]==0xce);
    /* A session must never subscribe to a second talkgroup. */
    assert(c->tg==0);c->tg=(uint32_t)p[4]<<24|(uint32_t)p[5]<<16|(uint32_t)p[6]<<8|p[7];
    return size;
}
int esp_websocket_client_send_text(esp_websocket_client_handle_t c,const char *data,int size,TickType_t wait)
{
    (void)c;(void)wait;assert(size>=0);
    if(strstr(data,"searchHouse"))assert(strstr(data,"\"amount\":15"));
    return size;
}
static void connected(void)
{
    s_client->connected=true;s_client->tg=0;
    s_client->callback(NULL,NULL,WEBSOCKET_EVENT_CONNECTED,NULL);
}
static void audio(void)
{
    const uint8_t msg[]={0x92,20,0xc4,2,0x7f,0xff};
    esp_websocket_event_data_t ev={.data_len=sizeof(msg),.payload_len=sizeof(msg),.fin=true,.op_code=2,.data_ptr=(const char *)msg};
    websocket_event(NULL,NULL,WEBSOCKET_EVENT_DATA,&ev);
}

static void replay_advance(int64_t target)
{
    while(replay_next<20) {
        int64_t when=replay_base+(int64_t)replay_next*120000;
        if(replay_jitter && (replay_next==7 || replay_next==8))when=replay_base+1006000;
        if(when>target)break;
        clock_us=when;
        uint8_t msg[965]={0x92,20,0xc5,3,0xc0};
        for(size_t i=5;i<sizeof(msg);++i)msg[i]=(i&1)?0xb0:0x30;
        esp_websocket_event_data_t event={.data_len=sizeof(msg),.payload_len=sizeof(msg),.fin=true,.op_code=2,.data_ptr=(const char*)msg};
        ++replay_next;websocket_event(NULL,NULL,WEBSOCKET_EVENT_DATA,&event);
    }
    clock_us=target;
}

static void test_playback_timing(bool jitter)
{
    bm_network_service(true);connected();xQueueReset(s_audio_queue);
    replay_next=replay_zero_blocks=replay_samples=0;replay_dma_until=0;
    replay_base=clock_us;replay_jitter=jitter;
    s_last_audio_us=s_play_call_started_us=0;s_stream_source=5020879;++s_stream_call_sequence;
    unsigned drops=s_audio_dropped,underruns=s_audio_underruns;
    replay_mode=true;
    if(setjmp(replay_done)==0)playback_task(NULL);
    replay_mode=false;
    printf("Virtual DMA replay (%s): samples=%u artificial_silence_blocks=%u underruns=%u\n",
           jitter?"286ms gap":"steady 120ms",replay_samples,replay_zero_blocks,s_audio_underruns-underruns);
    fflush(stdout);
    assert(replay_zero_blocks==0 && replay_samples==20*960);
    assert(s_audio_dropped==drops && s_audio_underruns==underruns);
}

static void caller_audio(uint32_t source)
{
    uint32_t tg=(uint32_t)s_talkgroup;
    const uint8_t header[]={0x95,11,0,0xce,source>>24,source>>16,source>>8,source,
        0xce,tg>>24,tg>>16,tg>>8,tg,0};
    esp_websocket_event_data_t ev={.data_len=sizeof(header),.payload_len=sizeof(header),.fin=true,.op_code=2,.data_ptr=(const char*)header};
    websocket_event(NULL,NULL,WEBSOCKET_EVENT_DATA,&ev);audio();
    audio_packet_t packet;int16_t pcm[BM_AUDIO_PACKET_MAX];
    memcpy(&packet,s_audio_queue->last,sizeof(packet));play_packet(&packet,pcm);
}

static void test_caller_persistence(void)
{
    bm_qso_info_t item={.active=true,.source_id=1234,.started_us=1700000000000000LL};
    strcpy(item.session,"caller-a");strcpy(item.callsign,"CALL1");strcpy(item.name,"Operator");strcpy(item.country,"Russia");
    caller_audio(1234);s_metadata_ready=true;metadata_publish(&item,s_selection_generation);
    bm_qso_info_t shown;
    assert(bm_qso_get_current(&shown) && shown.active && !strcmp(shown.callsign,"CALL1"));
    bm_qso_info_t partial=item;partial.callsign[0]=partial.name[0]=partial.country[0]=0;partial.source_id=0;
    metadata_publish(&partial,s_selection_generation);
    clock_us+=5000000;caller_audio(1234);
    assert(bm_qso_get_current(&shown) && shown.active && !strcmp(shown.callsign,"CALL1"));
    metadata_event(NULL,NULL,WEBSOCKET_EVENT_DISCONNECTED,NULL);
    assert(bm_qso_get_current(&shown) && shown.active && !strcmp(shown.callsign,"CALL1"));
    partial.active=false;partial.ended_us=partial.started_us+10000000;
    metadata_publish(&partial,s_selection_generation);
    clock_us+=301000000;caller_audio(1234);
    assert(bm_qso_get_current(&shown) && shown.active && !strcmp(shown.callsign,"CALL1"));
    caller_audio(5678);
    assert(bm_qso_get_current(&shown) && shown.active && !strcmp(shown.callsign,"ID 5678"));
    item.source_id=5678;item.started_us+=20000000;strcpy(item.session,"caller-b");strcpy(item.callsign,"CALL2");
    metadata_publish(&item,s_selection_generation);
    assert(bm_qso_get_current(&shown) && shown.active && !strcmp(shown.callsign,"CALL2"));
    s_metadata_ready=true;clock_us+=2000000;
    assert(bm_qso_get_current(&shown) && !shown.active);
    caller_audio(5678);
    assert(bm_qso_get_current(&shown) && shown.active && !strcmp(shown.callsign,"CALL2"));
    item.source_id=9999;item.started_us+=20000000;strcpy(item.session,"caller-c");strcpy(item.callsign,"CALL3");
    metadata_publish(&item,s_selection_generation);
    assert(bm_qso_get_current(&shown) && shown.active && !strcmp(shown.callsign,"CALL2"));
    clock_us+=2000000;
    assert(bm_qso_get_current(&shown) && !shown.active);
    puts("Caller persistence: PASS (sparse updates, metadata outage/early stop, long calls, caller changes, idle expiry)");
}

static void test_video_regression(void)
{
    caller_audio(5020879);
    assert(bm_metadata_start()==ESP_OK);
    s_metadata_client->connected=true;s_metadata_ready=true;
    metadata_query(); /* Check the production request asks for only 15 rows. */
    s_metadata_ready=false;
    bm_network_service(true);
    bm_qso_info_t shown;
    assert(directory_requests==1);
    assert(bm_qso_get_current(&shown) && shown.active && !strcmp(shown.callsign,"9M2FRD"));
    /* Other callers and unrelated late reports must not erase the learned ID. */
    caller_audio(5678);caller_audio(5020879);
    metadata_event(NULL,NULL,WEBSOCKET_EVENT_DISCONNECTED,NULL);
    assert(bm_qso_get_current(&shown) && shown.active && !strcmp(shown.callsign,"9M2FRD"));
    bm_network_service(true);assert(directory_requests==1);
    /* Complete loss of metadata still retains the current numeric source. */
    caller_audio(7654321);
    assert(bm_qso_get_current(&shown) && shown.active && !strcmp(shown.callsign,"ID 7654321"));
    xQueueReset(s_audio_queue);
    for(unsigned i=0;i<12;++i){s_stream_source=100+i;audio();}
    assert(s_audio_queue->count==AUDIO_QUEUE_DEPTH);
    audio_packet_t oldest;assert(xQueueReceive(s_audio_queue,&oldest,0)==pdTRUE);
    assert(oldest.source_id==109 && s_audio_dropped>=9);
    audio_packet_t frame={.generation=s_selection_generation,.source_id=5020879,.count=960};
    memset(frame.samples,0xff,sizeof(frame.samples));
    int16_t pcm[160];unsigned before=writes;play_packet(&frame,pcm);assert(writes==before+6);
    before=writes;switch_during_write=true;play_packet(&frame,pcm);assert(writes==before+1);
    puts("Video regression: PASS (directory lookup, cross-caller cache, missing metadata ID, bounded fresh-audio queue)");
}

static void test_country_completion(void)
{
    bm_network_service(true);connected();
    bm_identity_t partial={.id=6300001};strcpy(partial.callsign,"ZC4AA");bm_identity_put(&s_identities,&partial);
    clock_us+=20000000;caller_audio(partial.id);
    bm_network_service(true);bm_qso_info_t shown;
    assert(bm_qso_get_current(&shown) && !strcmp(shown.country,"Cyprus"));
    assert(directory_requests==2); /* Known callsign still needs missing country. */
    bm_qso_info_t history={.started_us=1800000000000000LL,.ended_us=1800000001000000LL};
    strcpy(history.session,"country-history");strcpy(history.callsign,"ID 3139516");
    metadata_publish(&history,s_selection_generation);
    history.source_id=3139516;metadata_publish(&history,s_selection_generation);
    clock_us+=20000000;bm_network_service(true);
    bm_qso_info_t rows[5];size_t count=bm_qso_get_history(rows,5);
    assert(count && rows[0].source_id==3139516 && !strcmp(rows[0].country,"United States"));
    assert(!strcmp(rows[0].callsign,"KM4SAB") && directory_requests==3);
    memset(&s_identities,0,sizeof(s_identities));
    count=bm_qso_get_history(rows,5);
    assert(count && !strcmp(rows[0].callsign,"KM4SAB") && !strcmp(rows[0].country,"United States"));
    clock_us+=20000000;bm_network_service(true);
    assert(directory_requests==3); // Retained history no longer depends on the 16-entry live cache.
    puts("Country completion: PASS (missing country, duplicate ID backfill, retained identity after cache eviction)");
}
int main(void)
{
    assert(bm_audio_start(91)==ESP_OK);
    bm_network_service(false);assert(!s_client->started);
    bm_network_service(true);connected();assert(s_client->tg==91);
    audio();assert(s_audio_queue->count==1);
    audio_packet_t previous;memcpy(&previous,s_audio_queue->last,sizeof(previous));
    int16_t pcm[BM_AUDIO_PACKET_MAX];
    clock_us+=16000000;bm_network_service(true);assert(s_metadata_started);
    assert(bm_audio_select(45401)==ESP_OK);
    assert(previous.generation!=s_selection_generation && !s_subscribed && s_audio_queue->count==0);
    play_packet(&previous,pcm);assert(!writes);
    audio();assert(s_audio_queue->count==0); /* Old socket is still running. */
    fail_stop=true;bm_network_service(true);assert(s_client->starts==1 && !s_subscribed);
    fail_stop=false;bm_network_service(true);assert(s_client->stops==1 && s_client->starts==2);
    connected();assert(s_client->tg==45401);audio();assert(s_audio_queue->count==1);
    audio_packet_t fresh;memcpy(&fresh,s_audio_queue->last,sizeof(fresh));
    assert(fresh.generation==s_selection_generation);
    play_packet(&previous,pcm);assert(!writes);
    play_packet(&fresh,pcm);assert(writes==1 && bm_audio_is_active());
    /* Rapid changes coalesce to the most recent custom selection. */
    bm_audio_select(46001);bm_audio_select(50210);bm_audio_select(123456);
    bm_network_service(true);connected();assert(s_client->tg==123456 && s_client->starts==3);
    bm_audio_select(123456);bm_network_service(true);assert(s_client->starts==3);
    bm_audio_select(91);bm_audio_select(123456);
    bm_network_service(true);connected();assert(s_client->starts==4 && s_client->tg==123456);
    bm_network_service(false);assert(!s_audio_started && bm_audio_state()==BM_AUDIO_IDLE);
    bm_audio_select(50210);bm_network_service(false);assert(!s_audio_started);
    fail_start=true;bm_network_service(true);assert(!s_audio_started);
    fail_start=false;bm_network_service(true);connected();assert(s_client->tg==50210);
    bm_qso_info_t stale={.active=true,.started_us=1000000};
    metadata_publish(&stale,s_selection_generation-1);assert(!s_qso_store.current.active);
    assert(writes==1); /* Only the selected session reached the fake output. */
    test_caller_persistence();
    test_video_regression();
    test_country_completion();
    test_playback_timing(false);test_playback_timing(true);
    bm_network_service(false);esp_websocket_client_destroy(s_client);vQueueDelete(s_audio_queue);
    puts("Hoseline switching: PASS (preset/custom, stale packets, rapid changes, reconnect, stop/start failures)");
    return 0;
}
