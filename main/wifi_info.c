#include "wifi_info_reader.h"
#include "esp_wifi.h"
#include <stdio.h>
#include <string.h>

void bm_wifi_read_info(esp_netif_t *netif, bool connected, bm_wifi_info_t *out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!connected || !netif) return;
    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) return;
    out->connected = out->signal_valid = true;
    memcpy(out->ssid, ap.ssid, 32);
    out->ssid[32] = 0;
    out->rssi = ap.rssi;
    esp_netif_ip_info_t ip;
    if (esp_netif_get_ip_info(netif, &ip) == ESP_OK && ip.ip.addr) {
        out->ipv4_valid = true;
        snprintf(out->ip, sizeof(out->ip), IPSTR, IP2STR(&ip.ip));
        snprintf(out->subnet, sizeof(out->subnet), IPSTR, IP2STR(&ip.netmask));
        snprintf(out->gateway, sizeof(out->gateway), IPSTR, IP2STR(&ip.gw));
    }
    for (unsigned i=0; i<3; ++i) {
        esp_netif_dns_info_t dns = {0};
        if (esp_netif_get_dns_info(netif, (esp_netif_dns_type_t)i, &dns) != ESP_OK) continue;
        if (dns.ip.type == ESP_IPADDR_TYPE_V4 && dns.ip.u_addr.ip4.addr)
            snprintf(out->dns[i], sizeof(out->dns[i]), IPSTR, IP2STR(&dns.ip.u_addr.ip4));
        else if (dns.ip.type == ESP_IPADDR_TYPE_V6 &&
            (dns.ip.u_addr.ip6.addr[0] || dns.ip.u_addr.ip6.addr[1] ||
             dns.ip.u_addr.ip6.addr[2] || dns.ip.u_addr.ip6.addr[3]))
            snprintf(out->dns[i], sizeof(out->dns[i]), IPV6STR, IPV62STR(dns.ip.u_addr.ip6));
    }
    /* A lost association during the snapshot must not display an old lease. */
    wifi_ap_record_t check;
    if (esp_wifi_sta_get_ap_info(&check) != ESP_OK || memcmp(ap.bssid, check.bssid, 6))
        memset(out, 0, sizeof(*out));
}
