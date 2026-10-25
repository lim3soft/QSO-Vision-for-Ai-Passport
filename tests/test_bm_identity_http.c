#include "bm_identity.h"
#include "esp_http_client.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct http_fake { size_t offset; } client;
void *esp_crt_bundle_attach;
static const char *body;
static int status=200;
static bool fail_open,fail_read,incomplete;
static unsigned closed,cleaned;

esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config)
{
    assert(!strcmp(config->url,"https://radioid.net/api/dmr/user/?id=5020879"));
    assert(config->crt_bundle_attach==esp_crt_bundle_attach && config->timeout_ms==3000);
    client.offset=0;return &client;
}
esp_err_t esp_http_client_open(esp_http_client_handle_t c,int n){(void)c;(void)n;return fail_open?ESP_FAIL:ESP_OK;}
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t c){(void)c;return (int64_t)strlen(body);}
int esp_http_client_get_status_code(esp_http_client_handle_t c){(void)c;return status;}
int esp_http_client_read(esp_http_client_handle_t c,char *buffer,int capacity)
{
    if(fail_read)return -1;
    size_t n=strlen(body)-c->offset;if(n>17)n=17;if(n>(size_t)capacity)n=(size_t)capacity;
    memcpy(buffer,body+c->offset,n);c->offset+=n;return (int)n;
}
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t c)
{return !incomplete && c->offset==strlen(body);}
esp_err_t esp_http_client_close(esp_http_client_handle_t c){(void)c;++closed;return ESP_OK;}
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c){(void)c;++cleaned;return ESP_OK;}

int main(void)
{
    body="{\"results\":[{\"id\":5020879,\"callsign\":\"9M2FRD\",\"fname\":\"Firdaus\",\"country\":\"Malaysia\"}]}";
    bm_identity_t person;
    assert(bm_identity_lookup(5020879,&person) && !strcmp(person.callsign,"9M2FRD"));
    fail_open=true;assert(!bm_identity_lookup(5020879,&person));fail_open=false;
    fail_read=true;assert(!bm_identity_lookup(5020879,&person));fail_read=false;
    status=429;assert(!bm_identity_lookup(5020879,&person));status=200;
    incomplete=true;assert(!bm_identity_lookup(5020879,&person));incomplete=false;
    char huge[3000];memset(huge,'x',sizeof(huge)-1);huge[sizeof(huge)-1]=0;body=huge;
    assert(!bm_identity_lookup(5020879,&person));
    assert(closed==6 && cleaned==6);
    puts("Directory HTTPS worker: PASS (chunked body, timeout/error, status, incomplete/oversize, cleanup, verification configured)");
    return 0;
}
