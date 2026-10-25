#include "wifi_info_reader.h"
#include "esp_wifi.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static wifi_ap_record_t ap;
static esp_netif_ip_info_t ip;
static esp_netif_dns_info_t dns[3];
static bool ap_ok=true, ip_ok=true, drop, roam;
static unsigned ap_reads, ip_reads, dns_reads;
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *out)
{
    ++ap_reads;if(!ap_ok || (drop && ap_reads%2==0))return ESP_FAIL;
    *out=ap;if(roam && ap_reads%2==0)out->bssid[0]^=1;return ESP_OK;
}
esp_err_t esp_netif_get_ip_info(esp_netif_t *netif,esp_netif_ip_info_t *out)
{ (void)netif;++ip_reads;if(!ip_ok)return ESP_FAIL;*out=ip;return ESP_OK; }
esp_err_t esp_netif_get_dns_info(esp_netif_t *netif,esp_netif_dns_type_t type,esp_netif_dns_info_t *out)
{ (void)netif;++dns_reads;*out=dns[type];return ESP_OK; }

int main(void)
{
    esp_netif_t netif={0};bm_wifi_info_t info;
    memset(ap.ssid,'W',32);ap.rssi=-57;ap.bssid[0]=1;
    inet_pton(AF_INET,"192.0.2.27",&ip.ip.addr);
    inet_pton(AF_INET,"255.255.255.0",&ip.netmask.addr);
    inet_pton(AF_INET,"192.0.2.1",&ip.gw.addr);
    inet_pton(AF_INET,"192.0.2.1",&dns[0].ip.u_addr.ip4.addr);
    dns[1].ip.type=ESP_IPADDR_TYPE_V6;
    inet_pton(AF_INET6,"2001:db8::53",dns[1].ip.u_addr.ip6.addr);
    bm_wifi_read_info(&netif,true,&info);
    assert(info.connected && info.signal_valid && info.ipv4_valid);
    assert(strlen(info.ssid)==32 && info.rssi==-57);
    assert(!strcmp(info.ip,"192.0.2.27") && !strcmp(info.subnet,"255.255.255.0"));
    assert(!strcmp(info.gateway,"192.0.2.1") && !strcmp(info.dns[0],"192.0.2.1"));
    assert(!strcmp(info.dns[1],"2001:0db8:0000:0000:0000:0000:0000:0053") && !info.dns[2][0]);
    unsigned reads=ap_reads+ip_reads+dns_reads;
    bm_wifi_read_info(&netif,false,&info);
    assert(!info.connected && !info.ssid[0] && !info.ip[0] && !info.dns[0][0]);
    assert(reads==ap_reads+ip_reads+dns_reads); /* Offline must not expose cached lease/DNS. */
    bm_wifi_read_info(NULL,true,&info);assert(!info.connected);
    ap_ok=false;bm_wifi_read_info(&netif,true,&info);assert(!info.connected);
    ap_ok=true;ip_ok=false;bm_wifi_read_info(&netif,true,&info);
    assert(info.connected && info.signal_valid && !info.ipv4_valid && !info.ip[0]);
    ip_ok=true;drop=true;ap_reads=0;bm_wifi_read_info(&netif,true,&info);
    assert(!info.connected && !info.ip[0]);
    drop=false;roam=true;ap_reads=0;bm_wifi_read_info(&netif,true,&info);
    assert(!info.connected && !info.ssid[0]);
    bm_wifi_read_info(&netif,true,NULL);
    puts("Wi-Fi diagnostics: PASS (live fields, long SSID, IPv4/IPv6 DNS, offline, errors, association changes)");
    return 0;
}
