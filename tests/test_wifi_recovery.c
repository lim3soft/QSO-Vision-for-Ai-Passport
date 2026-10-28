#include "../main/wifi_monitor.c"
#include <assert.h>

static int64_t now=1000000;
static unsigned connects,disconnects,writes,scan_busy;
static unsigned scans,clears,scan_cursor;
static wifi_ap_record_t visible[48];
static uint16_t visible_count=1;
static wifi_config_t configured;
static bool old_ap;
static bool interrupt_setup;
static int connect_error;
static struct { char key[16],value[65]; } entries[20];
static unsigned entry_count;
int64_t esp_timer_get_time(void){return now;}
void vTaskDelay(int ticks){now+=(int64_t)ticks*1000;if(interrupt_setup){interrupt_setup=false;bm_wifi_set_setup_active(true);}}
const char *esp_err_to_name(int err){return err?"FAIL":"OK";}
int esp_wifi_connect(void){++connects;int err=connect_error;connect_error=0;return err;}
int esp_wifi_disconnect(void){++disconnects;wifi_event_sta_disconnected_t d={.reason=8};wifi_event(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,&d);return 0;}
int esp_wifi_scan_start(const void *cfg,bool block){(void)cfg;(void)block;if(scan_busy){--scan_busy;return ESP_ERR_INVALID_STATE;}++scans;scan_cursor=0;return 0;}
int esp_wifi_scan_stop(void){return 0;}
int esp_wifi_scan_get_ap_num(uint16_t *count){*count=visible_count;return 0;}
int esp_wifi_scan_get_ap_records(uint16_t *count,wifi_ap_record_t *records)
{if(*count>visible_count)*count=visible_count;memcpy(records,visible,*count*sizeof(*records));return 0;}
int esp_wifi_scan_get_ap_record(wifi_ap_record_t *record)
{if(scan_cursor>=visible_count)return ESP_FAIL;*record=visible[scan_cursor++];return ESP_OK;}
int esp_wifi_clear_ap_list(void){++clears;scan_cursor=visible_count;return ESP_OK;}
int esp_wifi_set_config(int iface,const wifi_config_t *cfg){(void)iface;configured=*cfg;return 0;}
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *out)
{memset(out,0,sizeof(*out));if(old_ap)strcpy((char*)out->ssid,"Old AP");else memcpy(out->ssid,configured.sta.ssid,32);return 0;}
int nvs_open(const char *name,int mode,nvs_handle_t *h){(void)name;(void)mode;*h=1;return 0;}
int nvs_get_str(int h,const char *key,char *out,size_t *length)
{(void)h;for(unsigned i=0;i<entry_count;++i)if(!strcmp(entries[i].key,key)){size_t n=strlen(entries[i].value)+1;if(n>*length)return ESP_FAIL;memcpy(out,entries[i].value,n);*length=n;return 0;}return ESP_ERR_NOT_FOUND;}
int nvs_set_str(int h,const char *key,const char *value)
{(void)h;++writes;unsigned i;for(i=0;i<entry_count;++i)if(!strcmp(entries[i].key,key))break;if(i==entry_count)++entry_count;assert(i<20);snprintf(entries[i].key,16,"%s",key);snprintf(entries[i].value,65,"%s",value);return 0;}
int nvs_commit(int h){(void)h;return 0;}
int nvs_erase_key(int h,const char *key)
{(void)h;for(unsigned i=0;i<entry_count;++i)if(!strcmp(entries[i].key,key)){entries[i]=entries[--entry_count];return 0;}return ESP_ERR_NOT_FOUND;}
void nvs_close(int h){(void)h;}
void bm_wifi_read_info(esp_netif_t *netif,bool connected,bm_wifi_info_t *out)
{(void)netif;(void)connected;memset(out,0,sizeof(*out));}
static void rejected(int reason)
{wifi_event_sta_disconnected_t d={.reason=reason};wifi_event(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,&d);}
static void scan_done(void){wifi_event(NULL,WIFI_EVENT,WIFI_EVENT_SCAN_DONE,NULL);}
static void ap(unsigned index,const char *ssid,int rssi)
{memset(&visible[index],0,sizeof(visible[index]));snprintf((char*)visible[index].ssid,33,"%s",ssid);visible[index].rssi=rssi;}
static void reset_profiles(void)
{
    bm_wifi_stop();entry_count=0;now=1000000;
    s_connected_ssid[0]=s_last_error[0]=s_failed_ssid[0]=0;
    old_ap=interrupt_setup=false;connect_error=0;scan_busy=0;connects=disconnects=writes=scans=clears=0;
    visible_count=0;
    nvs_set_str(1,"p0_s","Home");nvs_set_str(1,"p0_p","home-password");
    nvs_set_str(1,"p1_s","Office");nvs_set_str(1,"p1_p","office-password");
    nvs_set_str(1,"p2_s","Cafe");nvs_set_str(1,"p2_p","");
    nvs_set_str(1,"wifi_last_ssid","Home");writes=0;
    assert(bm_wifi_start()==ESP_OK);bm_wifi_service();
    assert(!strcmp((char*)configured.sta.ssid,"Home") && connects==1);
}
static void missing_current(void)
{
    for(unsigned i=0;i<3;++i){rejected(201);now+=1000001;bm_wifi_service();}
    assert(s_scan_automatic && s_scan_active && !s_setup_active);
}
static void got_ip(void)
{ip_event(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);bm_wifi_service();assert(bm_wifi_state()==BM_WIFI_CONNECTED);}
static void saved_network_roaming(void)
{
    char password[65],last[33];
    /* Relocation: last-used Home is missing, Office is after 35 unrelated APs.
     * No truncated AP buffer, password prompt, profile rewrite or deletion. */
    reset_profiles();visible_count=36;
    for(unsigned i=0;i<35;++i){char name[33];snprintf(name,sizeof(name),"Unknown-%u",i);ap(i,name,-20);}
    ap(35,"Office",-88);missing_current();scan_done();bm_wifi_service();
    assert(!strcmp((char*)configured.sta.ssid,"Office"));
    assert(!strcmp((char*)configured.sta.password,"office-password") && writes==0 && clears);
    got_ip();assert(writes==1);
    assert(read_value("wifi_last_ssid",last,sizeof(last),true) && !strcmp(last,"Office"));
    assert(bm_wifi_saved_profile_count()==3 && bm_wifi_get_saved_credentials("Home",password,sizeof(password)) && !strcmp(password,"home-password"));
    unsigned before=scans,attempts=connects;visible_count=1;ap(0,"Home",-10);
    now+=90000000;bm_wifi_service();assert(scans==before && connects==attempts && !strcmp(bm_wifi_connected_ssid(),"Office"));
    /* Move again after a healthy connection: fail over to the next location. */
    missing_current();scan_done();bm_wifi_service();assert(!strcmp((char*)configured.sta.ssid,"Home"));got_ip();
    /* Prefer the last-used saved SSID when it is visible, even if weaker. */
    bm_wifi_set_setup_active(true);wifi_retry_pause(&s_retry);s_link_connected=false;
    visible_count=2;ap(0,"Office",-25);ap(1,"Home",-80);
    bm_wifi_set_setup_active(false);bm_wifi_service();scan_done();bm_wifi_service();
    assert(!strcmp((char*)configured.sta.ssid,"Home"));
    /* Try the strongest alternative; a bad saved password must not strand
     * recovery or overwrite it. Open saved networks also work. */
    reset_profiles();visible_count=3;ap(0,"Office",-45);ap(1,"Cafe",-75);ap(2,"Office",-65);
    missing_current();scan_done();bm_wifi_service();assert(!strcmp((char*)configured.sta.ssid,"Office"));
    rejected(WIFI_REASON_AUTH_FAIL);bm_wifi_service();
    assert(!strcmp((char*)configured.sta.ssid,"Cafe") && !configured.sta.password[0]);got_ip();
    assert(bm_wifi_get_saved_credentials("Office",password,sizeof(password)) && !strcmp(password,"office-password"));
    assert(bm_wifi_saved_profile_count()==3);
    /* None available: bounded rescan, then join a saved network that appears. */
    reset_profiles();visible_count=1;ap(0,"Unknown",-15);missing_current();scan_done();bm_wifi_service();
    before=scans;attempts=connects;now+=29000000;bm_wifi_service();assert(scans==before && connects==attempts);
    visible_count=1;ap(0,"Cafe",-50);now+=1000001;bm_wifi_service();assert(scans==before+1);
    scan_done();bm_wifi_service();assert(!strcmp((char*)configured.sta.ssid,"Cafe"));got_ip();
    /* Wi-Fi setup cancels an automatic scan and owns its AP results/editor.
     * Automatic selection resumes only after leaving setup. */
    reset_profiles();visible_count=1;ap(0,"Office",-40);missing_current();
    bm_wifi_scan();scan_done(); /* late completion from cancelled auto scan */
    assert(!s_auto_scan_ready);bm_wifi_service();scan_done();
    assert(bm_wifi_scan_ready() && !s_scan_automatic && s_setup_active);
    attempts=connects;now+=90000000;bm_wifi_service();assert(connects==attempts);
    bm_wifi_set_setup_active(false);bm_wifi_service();scan_done();bm_wifi_service();
    assert(!strcmp((char*)configured.sta.ssid,"Office"));
    /* A DHCP timeout tries the next candidate; an automatic scan timeout
     * clears driver records and backs off instead of allocating repeatedly. */
    reset_profiles();visible_count=2;ap(0,"Office",-35);ap(1,"Cafe",-65);
    missing_current();scan_done();bm_wifi_service();now+=30000001;bm_wifi_service();bm_wifi_service();
    assert(!strcmp((char*)configured.sta.ssid,"Cafe"));
    reset_profiles();missing_current();scan_busy=100;now+=16000000;bm_wifi_service();
    assert(!s_scan_active && !s_scan_automatic && s_auto_scan_at>now && clears);
    /* Driver connect errors also advance, rather than stopping recovery. */
    reset_profiles();visible_count=2;ap(0,"Office",-35);ap(1,"Cafe",-65);missing_current();
    connect_error=ESP_FAIL;scan_done();bm_wifi_service();bm_wifi_service();
    assert(!strcmp((char*)configured.sta.ssid,"Cafe"));
    /* Setup opened during the yielding handoff wins over auto-selection. */
    reset_profiles();visible_count=1;ap(0,"Office",-35);missing_current();
    unsigned attempts_before=connects;interrupt_setup=true;scan_done();bm_wifi_service();
    assert(s_setup_active && connects==attempts_before);
    /* Old single-profile compatibility keys cannot reset the new last-used
     * network or clobber the newer password during each boot. */
    reset_profiles();nvs_set_str(1,"wifi_ssid","Home");nvs_set_str(1,"wifi_password","obsolete");
    nvs_set_str(1,"wifi_last_ssid","Office");writes=0;
    assert(migrate_legacy_profile()==ESP_OK && writes==0);
    assert(read_value("wifi_last_ssid",last,sizeof(last),true) && !strcmp(last,"Office"));
    assert(bm_wifi_get_saved_credentials("Home",password,sizeof(password)) && !strcmp(password,"home-password"));
    nvs_set_str(1,"wifi_ssid","Legacy");assert(migrate_legacy_profile()==ESP_OK);
    assert(bm_wifi_get_saved_credentials("Home",password,sizeof(password)) && !strcmp(password,"home-password"));
    assert(bm_wifi_get_saved_credentials("Legacy",password,sizeof(password)) && !strcmp(password,"obsolete"));
    /* Deleted profiles are no longer candidates, and an empty profile store
     * never joins an unknown visible network or starts a scan loop. */
    reset_profiles();bm_wifi_scan();bm_wifi_service();scan_done();
    assert(bm_wifi_delete_profile("Office")==ESP_OK);
    visible_count=1;ap(0,"Office",-25);attempts_before=connects;
    bm_wifi_set_setup_active(false);bm_wifi_service();scan_done();bm_wifi_service();
    assert(connects==attempts_before && !bm_wifi_get_saved_credentials("Office",password,sizeof(password)));
    bm_wifi_stop();entry_count=0;assert(bm_wifi_start()==ESP_OK);
    bm_wifi_set_setup_active(false);bm_wifi_service();scan_done();bm_wifi_service();
    before=scans;attempts_before=connects;now+=120000000;bm_wifi_service();
    assert(scans==before && connects==attempts_before);
    puts("Saved Wi-Fi roaming: PASS (relocation, all APs, saved/open credentials, preference, RSSI, failure/DHCP fallback, no flapping, retry backoff, setup priority)");
}
int main(void)
{
    ap(0,"Test AP",-40);
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
    assert(connects==attempts+1); /* Setup still owns this legacy retry path. */
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
    saved_network_roaming();
    return 0;
}
