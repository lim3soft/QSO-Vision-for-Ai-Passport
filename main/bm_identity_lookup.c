#include "bm_identity.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include <stdio.h>

bool bm_identity_lookup(uint32_t id,bm_identity_t *out)
{
    char url[96];
    snprintf(url,sizeof(url),"https://radioid.net/api/dmr/user/?id=%lu",(unsigned long)id);
    esp_http_client_config_t config={.url=url,.timeout_ms=3000,.buffer_size=512,
        .buffer_size_tx=512,.crt_bundle_attach=esp_crt_bundle_attach,
        .user_agent="QSO-VISION/2.3.3",.disable_auto_redirect=false};
    esp_http_client_handle_t client=esp_http_client_init(&config);
    if(!client)return false;
    bool ok=false;
    /* Bounded body in supervisor stack, never in a socket callback. */
    char body[2048];int used=0;
    if(esp_http_client_open(client,0)!=ESP_OK)goto done;
    if(esp_http_client_fetch_headers(client)<0 || esp_http_client_get_status_code(client)!=200)goto done;
    while(used<(int)sizeof(body)-1) {
        int n=esp_http_client_read(client,body+used,sizeof(body)-1-used);
        if(n<0)goto done;
        if(!n)break;
        used+=n;
    }
    if(!esp_http_client_is_complete_data_received(client))goto done;
    body[used]=0;ok=bm_identity_decode(body,id,out);
done:
    esp_http_client_close(client);esp_http_client_cleanup(client);return ok;
}
