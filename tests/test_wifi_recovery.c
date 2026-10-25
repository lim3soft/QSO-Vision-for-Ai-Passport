#include "../main/wifi_monitor.c"
#include <assert.h>

static int64_t now=1000000;
static unsigned connects,disconnects,writes,scan_busy;
static wifi_config_t configured;
static bool old_ap;
static struct { char key[16],value[65]; } entries[20];
static unsigned entry_count;
int64_t esp_timer_get_time(void){return now;}
void vTaskDelay(int ticks){now+=(int64_t)ticks*1000;}
const char *esp_err_to_name(int err){return err?"FAIL":"OK";}
int esp_wifi_connect(void){++connects;return 0;}
int esp_wifi_disconnect(void){++disconnects;wifi_event_sta_disconnected_t d={.reason=8};wifi_event(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,&d);return 0;}
int esp_wifi_scan_start(const void *cfg,bool block){(void)cfg;(void)block;if(scan_busy){--scan_busy;return ESP_ERR_INVALID_STATE;}return 0;}
int esp_wifi_scan_stop(void){return 0;}
int esp_wifi_scan_get_ap_num(uint16_t *count){*count=1;return 0;}
int esp_wifi_scan_get_ap_records(uint16_t *count,wifi_ap_record_t *records)
{*count=1;memset(records,0,sizeof(*records));strcpy((char*)records->ssid,"Test AP");return 0;}
int esp_wifi_set_config(int iface,const wifi_config_t *cfg){(void)iface;configured=*cfg;return 0;}
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *out)
{memset(out,0,sizeof(*out));if(old_ap)strcpy((char*)out->ssid,"Old AP");else memcpy(out->ssid,configured.sta.ssid,32);return 0;}
int nvs_open(const char *name,int mode,nvs_handle_t *h){(void)name;(void)mode;*h=1;return 0;}
int nvs_get_str(int h,const char *key,char *out,size_t *length)
{(void)h;for(unsigned i=0;i<entry_count;++i)if(!strcmp(entries[i].key,key)){size_t n=strlen(entries[i].value)+1;if(n>*length)return ESP_FAIL;memcpy(out,entries[i].value,n);*length=n;return 0;}return ESP_ERR_NOT_FOUND;}
int nvs_set_str(int h,const char *key,const char *value)
{(void)h;++writes;unsigned i;for(i=0;i<entry_count;++i)if(!strcmp(entries[i].key,key))break;if(i==entry_count)++entry_count;assert(i<20);snprintf(entries[i].key,16,"%s",key);snprintf(entries[i].value,65,"%s",value);return 0;}
int nvs_commit(int h){(void)h;return 0;}
int nvs_erase_key(int h,const char *key){(void)h;(void)key;return 0;}
void nvs_close(int h){(void)h;}
void bm_wifi_read_info(esp_netif_t *netif,bool connected,bm_wifi_info_t *out)
{(void)netif;(void)connected;memset(out,0,sizeof(*out));}
static void rejected(int reason)
{wifi_event_sta_disconnected_t d={.reason=reason};wifi_event(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,&d);}
static void scan_done(void){wifi_event(NULL,WIFI_EVENT,WIFI_EVENT_SCAN_DONE,NULL);}
int main(void)
{
    s_started=true;s_events=1;
    nvs_set_str(1,"p0_s","Test AP");nvs_set_str(1,"p0_p","known-good");writes=0;
    assert(bm_wifi_set_credentials("Test AP","wrong-password")==ESP_OK);
    assert(writes==0 && bm_wifi_state()==BM_WIFI_CONNECTING);
    rejected(WIFI_REASON_AUTH_FAIL);assert(bm_wifi_state()==BM_WIFI_FAILED);
    ip_event(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);assert(bm_wifi_state()==BM_WIFI_FAILED);
    unsigned attempts=connects;
    for(unsigned i=0;i<40;++i){now+=1000000;bm_wifi_service();}
    assert(connects==attempts && writes==0 && !strcmp(bm_wifi_failed_ssid(),"Test AP"));
    char password[65];assert(bm_wifi_get_saved_credentials("Test AP",password,sizeof(password)));
    assert(!strcmp(password,"known-good"));
    /* Scanning is retried while the radio finishes disconnecting. */
    scan_busy=2;assert(bm_wifi_scan()==ESP_OK);
    for(unsigned i=0;i<3;++i){now+=1000000;bm_wifi_service();}
    scan_done();assert(bm_wifi_scan_ready() && bm_wifi_state()==BM_WIFI_SCAN_READY);
    wifi_ap_record_t ap;uint16_t count;
    assert(bm_wifi_get_scan_records(&ap,1,&count)==1 && count==1);
    /* A corrected password saves only after DHCP. */
    assert(bm_wifi_set_credentials("Test AP","corrected-password")==ESP_OK && writes==0);
    old_ap=true;ip_event(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);
    assert(bm_wifi_state()==BM_WIFI_CONNECTING && writes==0);old_ap=false;
    ip_event(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);bm_wifi_service();
    assert(bm_wifi_state()==BM_WIFI_CONNECTED && writes==3 && !bm_wifi_failed_ssid()[0]);
    assert(bm_wifi_get_saved_credentials("Test AP",password,sizeof(password)) && !strcmp(password,"corrected-password"));
    unsigned before=disconnects;bm_wifi_scan();bm_wifi_service();scan_done();
    assert(disconnects==before && bm_wifi_state()==BM_WIFI_CONNECTED);
    bm_wifi_scan();bm_wifi_service();rejected(201);scan_done();
    attempts=connects;now+=1000001;bm_wifi_service();assert(connects==attempts+1);
    ip_event(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);
    /* A new bad profile is never persisted. */
    writes=0;bm_wifi_set_credentials("Other AP","bad-password");rejected(WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT);
    bm_wifi_service();assert(writes==0 && !bm_wifi_get_saved_credentials("Other AP",password,sizeof(password)));
    /* Neither an unresponsive AP/DHCP server nor a failed scan waits forever. */
    bm_wifi_set_credentials("Other AP","trial-password");now+=30000001;bm_wifi_service();
    assert(bm_wifi_state()==BM_WIFI_FAILED && !s_retry.active);
    scan_busy=100;bm_wifi_scan();now+=16000000;bm_wifi_service();
    assert(bm_wifi_scan_ready() && bm_wifi_state()==BM_WIFI_SCAN_READY);
    bm_wifi_set_credentials("Other AP","trial-password");rejected(201);now+=1000001;bm_wifi_service();
    rejected(201);now+=1000001;bm_wifi_service();rejected(201);
    assert(bm_wifi_state()==BM_WIFI_FAILED && writes==0);
    /* A normal link loss still reconnects; a menu scan can pause recovery. */
    wifi_retry_connected(&s_retry);rejected(201);now+=1000001;bm_wifi_service();
    assert(bm_wifi_state()==BM_WIFI_CONNECTING);
    rejected(201);rejected(201);attempts=connects;now+=60000001;bm_wifi_service();
    assert(connects==attempts+1);
    bm_wifi_scan();attempts=connects;now+=60000001;bm_wifi_service();assert(connects==attempts);
    /* Emulate cold-boot volatile state while retaining the fake NVS store.
     * Only the previously confirmed profile may be restored, not the failed trial. */
    s_started=s_link_connected=s_scan_requested=s_scan_active=false;
    s_save_pending=s_commit_pending=false;s_scan_deadline=0;wifi_retry_pause(&s_retry);
    assert(bm_wifi_start()==ESP_OK);
    assert(!strcmp((char*)configured.sta.ssid,"Test AP"));
    assert(!strcmp((char*)configured.sta.password,"corrected-password"));
    attempts=connects;bm_wifi_service();assert(connects==attempts+1);
    puts("Wi-Fi recovery: PASS (bad password, profile protection, corrected retry, rescan, deadlines, automatic recovery)");
    return 0;
}
