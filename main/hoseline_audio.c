#include "hoseline_audio.h"
#include "bm_qso_store.h"
#include "bm_dsp.h"
#include "bm_identity.h"
#include "bsp_audio.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_websocket_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *TAG = "bm_audio";
#define AUDIO_QUEUE_DEPTH 3
#define METADATA_START_FREE (48 * 1024)
#define METADATA_START_BLOCK (24 * 1024)
#define METADATA_RESERVE (12 * 1024)

/* Queue compressed 8-bit samples; expand only in the playback task. */
typedef struct {
    uint32_t generation;
    uint32_t source_id;
    uint32_t call_sequence;
    uint16_t count;
    uint8_t samples[BM_AUDIO_PACKET_MAX];
} audio_packet_t;

static esp_websocket_client_handle_t s_client, s_metadata_client;
static QueueHandle_t s_audio_queue;
static TaskHandle_t s_play_task;
static bool s_audio_started, s_metadata_started;
static volatile bool s_metadata_ready, s_subscribed;
static volatile bool s_metadata_refresh_requested;
static volatile bm_audio_state_t s_state = BM_AUDIO_IDLE;
static volatile int s_talkgroup;
static volatile uint32_t s_stream_source, s_play_source;
static bm_identity_cache_t s_identities;
static volatile uint32_t s_stream_call_sequence;
static uint32_t s_play_call_sequence;
static int64_t s_play_call_started_us, s_play_call_wall_us;
static uint32_t s_last_lookup_id;
static int64_t s_last_lookup_us;
static bm_speech_level_t s_speech_level;
/* PCM queued through the codec, including data still in DMA after a socket
 * queue becomes empty. Only the playback task accesses these fields. */
static int64_t s_pcm_until_us;
static unsigned s_resume_fade;
/* Only the supervisor replaces a session. Buttons invalidate it immediately;
 * packets from that session can no longer reach the speaker. */
static volatile uint32_t s_selection_generation = 1, s_stream_generation;
static int s_stream_talkgroup;
static int64_t s_last_audio_us, s_audio_connected_us, s_metadata_retry_us;
static int64_t s_last_query_us, s_last_diagnostic_us;
static volatile unsigned s_audio_packets, s_audio_dropped;
static unsigned s_audio_underruns, s_audio_write_errors;
static const char *s_metadata_status = "WAITING";

static portMUX_TYPE s_metadata_mux = portMUX_INITIALIZER_UNLOCKED;
static bm_qso_store_t s_qso_store;
/* Receive assembly belongs to each socket, never its 6 KB task stack. */
static uint8_t s_audio_message[1024], s_metadata_message[4096];
static bm_frame_state_t s_audio_frame, s_metadata_frame;

static void log_memory(const char *where)
{
    ESP_LOGI(TAG, "%s heap=%u largest=%u audio_packets=%u dropped=%u underruns=%u write_errors=%u", where,
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
        s_audio_packets, s_audio_dropped,s_audio_underruns,s_audio_write_errors);
}

static int metadata_send(const char *text)
{
    if (!s_metadata_client) return -1;
    return esp_websocket_client_send_text(s_metadata_client, text, (int)strlen(text), pdMS_TO_TICKS(1000));
}

static void metadata_query(void)
{
    if (!s_metadata_ready || !s_metadata_client ||
        !esp_websocket_client_is_connected(s_metadata_client)) return;
    char query[320];
    snprintf(query, sizeof(query),
        "42[\"searchHouse\",{\"query\":{\"condition\":\"AND\",\"rules\":[{\"id\":\"DestinationID\",\"field\":\"DestinationID\",\"type\":\"integer\",\"input\":\"text\",\"operator\":\"equal\",\"value\":%d}]},\"amount\":%u}]",
        (int)s_talkgroup,(unsigned)BM_QSO_HISTORY_MAX);
    if (metadata_send(query) == (int)strlen(query)) {
        s_last_query_us = esp_timer_get_time();
        s_metadata_status = "LOADING HISTORY";
    }
}

static void metadata_publish(const bm_qso_info_t *item, uint32_t generation)
{
    int64_t now = esp_timer_get_time();
    time_t wall_now = time(NULL);
    taskENTER_CRITICAL(&s_metadata_mux);
    if (generation != s_selection_generation) {
        taskEXIT_CRITICAL(&s_metadata_mux);
        return;
    }
    bm_identity_t person={.id=item->source_id};
    snprintf(person.callsign,sizeof(person.callsign),"%s",item->callsign);
    snprintf(person.name,sizeof(person.name),"%s",item->name);
    snprintf(person.country,sizeof(person.country),"%s",item->country);
    bm_identity_put(&s_identities,&person);
    bm_qso_info_t enriched=*item;
    bm_identity_enrich(&s_identities,&enriched);
    bm_qso_store_apply(&s_qso_store, &enriched, now, (int64_t)wall_now);
    taskEXIT_CRITICAL(&s_metadata_mux);
    ESP_LOGI(TAG, "QSO metadata received TG=%d %s", (int)s_talkgroup, item->active ? "start" : "stop");
}

static void metadata_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base;
    if (id == WEBSOCKET_EVENT_CONNECTED) {
        memset(&s_metadata_frame, 0, sizeof(s_metadata_frame));
        s_metadata_status = "HANDSHAKE";
        return;
    }
    if (id == WEBSOCKET_EVENT_DISCONNECTED || id == WEBSOCKET_EVENT_ERROR) {
        s_metadata_ready = false;
        s_metadata_status = "RECONNECTING";
        return;
    }
    esp_websocket_event_data_t *event = data;
    if (id != WEBSOCKET_EVENT_DATA || !event || event->data_len < 0 ||
        event->payload_len < 0 || event->payload_offset < 0) return;
    size_t length;
    if (!bm_frame_append(&s_metadata_frame, s_metadata_message, sizeof(s_metadata_message),
                        event->op_code, event->fin, (size_t)event->payload_len,
                        (size_t)event->payload_offset, event->data_ptr, (size_t)event->data_len, &length) ||
        s_metadata_frame.opcode != 1 || !length) return;
    const char *text = (const char *)s_metadata_message;
    if (text[0] == '0') {
        (void)metadata_send("40");
    } else if (!strncmp(text, "40", 2)) {
        s_metadata_ready = true;
        s_metadata_status = "CONNECTED";
        (void)metadata_send("42[\"join\",\"everything\"]");
        (void)metadata_send("42[\"sendCurrentSessions\"]");
        metadata_query();
        log_memory("Metadata ready");
    } else if (!strcmp(text, "2")) {
        (void)metadata_send("3");
    } else if (!strncmp(text, "42", 2)) {
        if (!strcmp(text,"42[\"searchHouseComplete\"]")) {
            s_metadata_status="CONNECTED";
            ESP_LOGI(TAG,"BM history loaded: %u completed QSOs",(unsigned)s_qso_store.count);
            return;
        }
        bm_qso_info_t item;
        taskENTER_CRITICAL(&s_metadata_mux);
        int tg = s_talkgroup;
        uint32_t generation = s_selection_generation;
        taskEXIT_CRITICAL(&s_metadata_mux);
        if (bm_metadata_may_match(text,tg) && bm_decode_metadata(text, tg, &item) && generation == s_selection_generation)
            metadata_publish(&item, generation);
    }
}

esp_err_t bm_metadata_start(void)
{
    if (s_metadata_started) return ESP_OK;
    if (heap_caps_get_free_size(MALLOC_CAP_8BIT) < METADATA_START_FREE ||
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < METADATA_START_BLOCK) {
        s_metadata_status = "LOW MEMORY";
        return ESP_ERR_NO_MEM;
    }
    esp_websocket_client_config_t config = {
        .uri = "wss://api.brandmeister.network/lh/?EIO=4&transport=websocket&version=2",
        .task_name = "bm_metadata",
        .buffer_size = 1024,
        .task_stack = 6144,
        .network_timeout_ms = 5000,
        .reconnect_timeout_ms = 5000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    log_memory("Starting metadata");
    s_metadata_client = esp_websocket_client_init(&config);
    if (!s_metadata_client) return ESP_ERR_NO_MEM;
    esp_err_t err = esp_websocket_register_events(s_metadata_client, WEBSOCKET_EVENT_ANY, metadata_event, NULL);
    if (err == ESP_OK) err = esp_websocket_client_start(s_metadata_client);
    if (err != ESP_OK) {
        esp_websocket_client_destroy(s_metadata_client);
        s_metadata_client = NULL;
        return err;
    }
    s_metadata_started = true;
    s_metadata_status = "CONNECTING";
    return ESP_OK;
}

static int16_t mulaw_to_pcm(uint8_t value)
{
    uint8_t u = (uint8_t)~value;
    int32_t sample = ((u & 15) << 3) + 0x84;
    sample <<= (u & 0x70) >> 4;
    return (int16_t)((u & 0x80) ? 0x84 - sample : sample - 0x84);
}

static void send_subscription(void)
{
    uint32_t tg = (uint32_t)s_stream_talkgroup;
    uint8_t packet[] = {0x92, 1, 0x91, 0xce, tg >> 24, tg >> 16, tg >> 8, tg};
    if (!s_client || !esp_websocket_client_is_connected(s_client)) return;
    int sent = esp_websocket_client_send_bin(s_client, (const char *)packet, sizeof(packet), pdMS_TO_TICKS(1000));
    s_subscribed = sent == (int)sizeof(packet) && s_stream_generation == s_selection_generation;
    ESP_LOGI(TAG, "Hoseline TG %d subscription: %s", s_stream_talkgroup, s_subscribed ? "sent" : "FAILED");
}

static void play_packet(const audio_packet_t *packet, int16_t *pcm)
{
    if (packet->generation != s_selection_generation || !s_subscribed) return;
    if(!s_play_call_started_us || s_play_call_sequence!=packet->call_sequence || s_play_source!=packet->source_id) {
        taskENTER_CRITICAL(&s_metadata_mux);
        if(packet->generation!=s_selection_generation){taskEXIT_CRITICAL(&s_metadata_mux);return;}
        s_play_call_sequence=packet->call_sequence;
        s_play_source=packet->source_id;
        s_play_call_started_us=esp_timer_get_time();
        s_play_call_wall_us=(int64_t)time(NULL)*1000000;
        taskEXIT_CRITICAL(&s_metadata_mux);
        bm_speech_level_reset(&s_speech_level);
        s_resume_fade=40;
    }
    for(size_t offset=0;offset<packet->count;offset+=160) {
        size_t n=packet->count-offset;if(n>160)n=160;
        for(size_t i=0;i<n;++i)pcm[i]=mulaw_to_pcm(packet->samples[offset+i]);
        bm_speech_level(&s_speech_level,pcm,n);
        for(size_t i=0;i<n && s_resume_fade;++i,--s_resume_fade)
            pcm[i]=(int16_t)((int32_t)pcm[i]*(40-(int)s_resume_fade)/40);
        if(packet->generation!=s_selection_generation || !s_subscribed)return;
        int64_t started=esp_timer_get_time();
        if(bsp_audio_write(pcm,n*sizeof(*pcm))!=ESP_OK){++s_audio_write_errors;return;}
        if(s_pcm_until_us<started)s_pcm_until_us=started;
        s_pcm_until_us+=(int64_t)n*125; /* 8 kHz */
        int64_t dma_bound=esp_timer_get_time()+180000;
        if(s_pcm_until_us>dma_bound)s_pcm_until_us=dma_bound;
        taskENTER_CRITICAL(&s_metadata_mux);
        if(packet->generation==s_selection_generation)s_last_audio_us=esp_timer_get_time();
        taskEXIT_CRITICAL(&s_metadata_mux);
    }
}

static void playback_task(void *arg)
{
    (void)arg;
    audio_packet_t packet;
    int16_t pcm[160];
    bool primed=false,starved=false;
    for (;;) {
        if (xQueueReceive(s_audio_queue,&packet,pdMS_TO_TICKS(20))!=pdTRUE) {
            if(bm_audio_is_active() && esp_timer_get_time()>=s_pcm_until_us) {
                if(!starved)++s_audio_underruns;
                starved=true;
                /* DMA auto-clears a genuine gap. Never insert silence merely
                 * because network packets have not reached the socket queue. */
                s_resume_fade=40;
            } else if(!bm_audio_is_active()){primed=false;vTaskDelay(pdMS_TO_TICKS(10));}
            continue;
        }
        /* 180 ms playout delay plus each 120 ms packet spans the observed
         * 286 ms gap. DMA capacity is not an additional independent reserve. */
        if(!primed){vTaskDelay(pdMS_TO_TICKS(180));primed=true;s_pcm_until_us=0;}
        starved=false;
        play_packet(&packet, pcm);
    }
}

static void websocket_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base;
    if (id == WEBSOCKET_EVENT_CONNECTED) {
        if (s_stream_generation != s_selection_generation) return;
        s_state = BM_AUDIO_CONNECTED;
        s_audio_connected_us = esp_timer_get_time();
        s_stream_source=s_play_source=0;
        s_play_call_started_us=0;
        memset(&s_audio_frame, 0, sizeof(s_audio_frame));
        send_subscription();
        log_memory("Hoseline connected");
        return;
    }
    if (id == WEBSOCKET_EVENT_DISCONNECTED || id == WEBSOCKET_EVENT_ERROR) {
        s_state = BM_AUDIO_CONNECTING;
        s_subscribed = false;
        s_audio_connected_us = 0;
        return;
    }
    esp_websocket_event_data_t *event = data;
    if (!s_subscribed || s_stream_generation != s_selection_generation) return;
    if (id != WEBSOCKET_EVENT_DATA || !event || event->data_len < 0 ||
        event->payload_len < 0 || event->payload_offset < 0) return;
    size_t length;
    if (!bm_frame_append(&s_audio_frame, s_audio_message, sizeof(s_audio_message),
                        event->op_code, event->fin, (size_t)event->payload_len,
                        (size_t)event->payload_offset, event->data_ptr, (size_t)event->data_len, &length) ||
        s_audio_frame.opcode != 2) return;
    uint32_t source,tg;
    if(bm_decode_caller(s_audio_message,length,&source,&tg)) {
        if(tg==(uint32_t)s_stream_talkgroup){s_stream_source=source;++s_stream_call_sequence;}
        return;
    }
    bm_identity_t alias;
    if(bm_identity_alias(s_audio_message,length,s_stream_source,&alias)) {
        taskENTER_CRITICAL(&s_metadata_mux);bm_identity_put(&s_identities,&alias);taskEXIT_CRITICAL(&s_metadata_mux);
        return;
    }
    audio_packet_t packet;
    packet.generation = s_stream_generation;
    packet.source_id=s_stream_source;
    packet.call_sequence=s_stream_call_sequence;
    packet.count = (uint16_t)bm_decode_audio(s_audio_message, length, packet.samples, sizeof(packet.samples));
    if (!packet.count) return;
    if (s_audio_packets++ == 0) ESP_LOGI(TAG, "First Hoseline audio packet: %u samples", packet.count);
    if(xQueueSend(s_audio_queue,&packet,0)!=pdTRUE) {
        /* Retain fresh audio on congestion rather than accumulating delay. */
        audio_packet_t stale;
        if(xQueueReceive(s_audio_queue,&stale,0)==pdTRUE)++s_audio_dropped;
        if(xQueueSend(s_audio_queue,&packet,0)!=pdTRUE)++s_audio_dropped;
    }
}

esp_err_t bm_audio_start(int talkgroup)
{
    if (talkgroup <= 0) return ESP_ERR_INVALID_ARG;
    if (s_client) return bm_audio_select(talkgroup);
    s_talkgroup = talkgroup;
    s_audio_queue = xQueueCreate(AUDIO_QUEUE_DEPTH, sizeof(audio_packet_t));
    if (!s_audio_queue) return ESP_ERR_NO_MEM;
    esp_websocket_client_config_t config = {
        .uri = "wss://hose.brandmeister.network/spotter/?token=test:test",
        .subprotocol = "spotter",
        .task_name = "bm_hoseline",
        .buffer_size = 1024,
        .task_stack = 6144,
        .network_timeout_ms = 5000,
        .reconnect_timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    s_client = esp_websocket_client_init(&config);
    if (!s_client) goto failed;
    if (esp_websocket_register_events(s_client, WEBSOCKET_EVENT_ANY, websocket_event, NULL) != ESP_OK) goto failed;
    if (xTaskCreate(playback_task, "bm_audio_play", 4096, NULL, 6, &s_play_task) != pdPASS) goto failed;
    /* The network supervisor starts TLS only after DHCP. */
    s_state = BM_AUDIO_IDLE;
    return ESP_OK;
failed:
    if (s_client) esp_websocket_client_destroy(s_client);
    s_client = NULL;
    vQueueDelete(s_audio_queue);
    s_audio_queue = NULL;
    return ESP_ERR_NO_MEM;
}

static bool stop_metadata(void)
{
    if(!s_metadata_started)return true;
    esp_err_t err=esp_websocket_client_stop(s_metadata_client);
    if(err!=ESP_OK){ESP_LOGW(TAG,"Metadata stop deferred: %s",esp_err_to_name(err));return false;}
    esp_websocket_client_destroy(s_metadata_client);
    s_metadata_client=NULL;s_metadata_started=s_metadata_ready=false;
    return true;
}

static void resolve_current_id(int64_t now)
{
    uint32_t id=0;
    bm_identity_t person;
    taskENTER_CRITICAL(&s_metadata_mux);
    uint32_t current=bm_audio_is_active()?s_play_source:0;
    bool complete=bm_identity_get(&s_identities,current,&person) && person.country[0];
    if(current && !complete)id=current;
    /* Resolve incomplete history too, including callers whose audio was not
     * selected by Hoseline. A known callsign alone is not a complete country. */
    if(!id)for(size_t i=0;i<s_qso_store.count;++i) {
        uint32_t candidate=s_qso_store.history[i].source_id;
        if(!candidate || (candidate==s_last_lookup_id && now-s_last_lookup_us<60000000))continue;
        if(bm_callsign_known(s_qso_store.history[i].callsign) && s_qso_store.history[i].country[0])continue;
        if(!bm_identity_get(&s_identities,candidate,&person) || !person.country[0]){id=candidate;break;}
    }
    taskEXIT_CRITICAL(&s_metadata_mux);
    if(!id || (s_last_lookup_us && now-s_last_lookup_us<15000000) ||
       (id==s_last_lookup_id && now-s_last_lookup_us<60000000))return;
    /* Only two TLS sessions at once: release LH before a bounded HTTPS
     * directory request. Audio is never stopped, and LH restarts immediately. */
    if(!stop_metadata())return;
    s_metadata_status="RESOLVING CALLER";
    s_last_lookup_id=id;s_last_lookup_us=now;
    if(heap_caps_get_free_size(MALLOC_CAP_8BIT)>=40*1024 &&
       heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)>=24*1024 &&
       bm_identity_lookup(id,&person)) {
        taskENTER_CRITICAL(&s_metadata_mux);
        bm_identity_put(&s_identities,&person);
        /* Preserve a resolved history identity even after the small live cache
         * rotates. Only fill absent fields; provider data remains authoritative. */
        for(size_t i=0;i<s_qso_store.count;++i) {
            bm_qso_history_row_t *row=&s_qso_store.history[i];
            if(row->source_id!=person.id)continue;
            if(!bm_callsign_known(row->callsign))snprintf(row->callsign,sizeof(row->callsign),"%s",person.callsign);
            if(!row->country[0])snprintf(row->country,sizeof(row->country),"%s",person.country);
        }
        taskEXIT_CRITICAL(&s_metadata_mux);
        ESP_LOGI(TAG,"Caller directory resolved");
    } else ESP_LOGW(TAG,"Caller directory deferred/unavailable; retaining numeric ID");
    s_metadata_retry_us=esp_timer_get_time();
}

void bm_network_service(bool wifi_connected)
{
    int64_t now = esp_timer_get_time();
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    bool audio_unstable = s_audio_connected_us == 0;
    bool switching = s_stream_generation != s_selection_generation;
    if (s_metadata_started && (!wifi_connected || free_heap < METADATA_RESERVE || audio_unstable || switching)) {
        /* Never stop/destroy from the socket callback. Release metadata first
         * so audio can reconnect with the available TLS handshake memory. */
        if(!stop_metadata())return;
        s_metadata_status = "DEFERRED";
        s_metadata_retry_us = now + (switching ? 2000000 : 5000000);
    }
    if ((!wifi_connected || switching) && s_audio_started) {
        s_subscribed = false;
        s_audio_connected_us = s_last_audio_us = 0;
        /* A new socket session has exactly one subscription. Re-sending [1,[TG]]
         * on the old session can retain its previous talkgroup subscription. */
        esp_err_t err = esp_websocket_client_stop(s_client);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Hoseline session stop deferred: %s", esp_err_to_name(err));
            return;
        }
        s_audio_started = false;
        if (s_audio_queue) xQueueReset(s_audio_queue);
        memset(&s_audio_frame, 0, sizeof(s_audio_frame));
    }
    if (!wifi_connected) {
        s_state = BM_AUDIO_IDLE;
        s_audio_connected_us = 0;
        return;
    }
    if (s_client && !s_audio_started) {
        taskENTER_CRITICAL(&s_metadata_mux);
        s_stream_talkgroup = s_talkgroup;
        s_stream_generation = s_selection_generation;
        taskEXIT_CRITICAL(&s_metadata_mux);
        log_memory("Starting Hoseline");
        if (esp_websocket_client_start(s_client) == ESP_OK) {
            s_audio_started = true;
            s_state = BM_AUDIO_CONNECTING;
        }
    }
    if(s_audio_connected_us>0 && !switching)resolve_current_id(now);
    now=esp_timer_get_time();
    if (!s_metadata_started && s_audio_connected_us > 0 &&
        now - s_audio_connected_us >= 2000000 && now >= s_metadata_retry_us) {
        s_metadata_retry_us = now + 5000000;
        esp_err_t err = bm_metadata_start();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Metadata deferred: %s", esp_err_to_name(err));
            log_memory("Metadata budget");
        }
    }
    if (s_metadata_ready && (s_metadata_refresh_requested || now - s_last_query_us > 60000000)) {
        s_metadata_refresh_requested = false;
        metadata_query();
    }
    if (now - s_last_diagnostic_us > 30000000) {
        s_last_diagnostic_us = now;
        log_memory("Monitor");
    }
}

esp_err_t bm_audio_select(int talkgroup)
{
    if (talkgroup <= 0) return ESP_ERR_INVALID_ARG;
    taskENTER_CRITICAL(&s_metadata_mux);
    if (talkgroup == s_talkgroup) {
        taskEXIT_CRITICAL(&s_metadata_mux);
        return ESP_OK;
    }
    s_talkgroup = talkgroup;
    ++s_selection_generation;
    s_subscribed = false;
    s_state = BM_AUDIO_CONNECTING;
    s_audio_connected_us = s_last_audio_us = 0;
    memset(&s_qso_store, 0, sizeof(s_qso_store));
    s_play_call_started_us=0;
    s_stream_source=s_play_source=0;
    taskEXIT_CRITICAL(&s_metadata_mux);
    if (s_audio_queue) xQueueReset(s_audio_queue);
    ESP_LOGI(TAG, "Talkgroup changed to %d; replacing Hoseline session", talkgroup);
    s_metadata_refresh_requested = true;
    return ESP_OK;
}

bm_audio_state_t bm_audio_state(void)
{
    if (s_state == BM_AUDIO_CONNECTED && bm_audio_is_active()) return BM_AUDIO_PLAYING;
    return s_state;
}

bool bm_audio_is_active(void)
{
    return s_state == BM_AUDIO_CONNECTED && s_subscribed && s_last_audio_us > 0 &&
           esp_timer_get_time() - s_last_audio_us < 1500000;
}

bool bm_qso_get_current(bm_qso_info_t *out)
{
    if (!out) return false;
    taskENTER_CRITICAL(&s_metadata_mux);
    bool audio=bm_audio_is_active();
    uint32_t source=s_play_source;
    *out=s_qso_store.current;
    if(audio) {
        if(!source || out->source_id!=source)memset(out,0,sizeof(*out));
        out->source_id=source;
        bm_identity_enrich(&s_identities,out);
        if(!bm_callsign_known(out->callsign)) {
            if(source)snprintf(out->callsign,sizeof(out->callsign),"ID %lu",(unsigned long)source);
            else snprintf(out->callsign,sizeof(out->callsign),"RECEIVING");
        }
        out->started_us=s_play_call_wall_us;
        int64_t elapsed=(esp_timer_get_time()-s_play_call_started_us)/1000000;
        out->elapsed_seconds=elapsed>0?(unsigned)elapsed:0;
    }
    out->active=audio;
    taskEXIT_CRITICAL(&s_metadata_mux);
    return audio || out->started_us>0;
}

size_t bm_qso_get_history_page(bm_qso_info_t *out, size_t capacity,
    size_t *offset, size_t *total, const char *anchor)
{
    if(!out || !capacity || !offset || !total)return 0;
    taskENTER_CRITICAL(&s_metadata_mux);
    size_t count=bm_qso_store_page(&s_qso_store,out,capacity,offset,total,anchor);
    for(size_t i=0;i<count;++i)bm_identity_enrich(&s_identities,&out[i]);
    taskEXIT_CRITICAL(&s_metadata_mux);
    return count;
}
size_t bm_qso_get_history(bm_qso_info_t *out, size_t capacity)
{
    size_t offset=0,total=0;
    return bm_qso_get_history_page(out,capacity,&offset,&total,NULL);
}

const char *bm_metadata_state_text(void) { return s_metadata_status; }

const char *bm_audio_state_text(void)
{
    switch (bm_audio_state()) {
    case BM_AUDIO_CONNECTING: return "CONNECTING";
    case BM_AUDIO_CONNECTED: return s_subscribed ? "MONITORING" : "SUBSCRIBING";
    case BM_AUDIO_PLAYING: return "ACTIVE";
    case BM_AUDIO_ERROR: return "ERROR";
    default: return "OFF";
    }
}
