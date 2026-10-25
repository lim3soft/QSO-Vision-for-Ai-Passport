#include "wifi_retry.h"
#include <string.h>
void wifi_retry_begin(wifi_retry_t *r, int64_t now, bool manual)
{ *r=(wifi_retry_t){.active=true,.manual=manual,.deadline=now+30000000}; }
void wifi_retry_pause(wifi_retry_t *r) { memset(r,0,sizeof(*r)); }
void wifi_retry_connected(wifi_retry_t *r)
{ *r=(wifi_retry_t){.active=true}; } /* Established link, automatic recovery. */
static void failed(wifi_retry_t *r, int64_t now)
{ r->active=false;r->deadline=0;r->retry_at=r->manual?0:now+60000000; }
bool wifi_retry_disconnected(wifi_retry_t *r, int64_t now, bool authentication_failed)
{
    if(!r->active)return false;
    if(!r->deadline)r->deadline=now+30000000;
    if(authentication_failed || ++r->failures>=3 || now>=r->deadline) {
        failed(r,now);return true;
    }
    r->retry_at=now+1000000;return false;
}
wifi_retry_action_t wifi_retry_poll(wifi_retry_t *r, int64_t now)
{
    if(r->active && r->deadline && now>=r->deadline) {
        failed(r,now);return WIFI_RETRY_TIMEOUT;
    }
    if(r->retry_at && now>=r->retry_at) {
        if(!r->active)wifi_retry_begin(r,now,false);
        r->retry_at=0;return WIFI_RETRY_CONNECT;
    }
    return WIFI_RETRY_NONE;
}
