#pragma once
#include <stdint.h>
#include <arpa/inet.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
typedef struct esp_netif_obj { int unused; } esp_netif_t;
typedef struct { uint32_t addr; } esp_ip4_addr_t;
typedef struct { uint32_t addr[4]; } esp_ip6_addr_t;
typedef struct { esp_ip4_addr_t ip, netmask, gw; } esp_netif_ip_info_t;
typedef enum { ESP_NETIF_DNS_MAIN, ESP_NETIF_DNS_BACKUP, ESP_NETIF_DNS_FALLBACK } esp_netif_dns_type_t;
typedef struct {
    struct { union { esp_ip4_addr_t ip4; esp_ip6_addr_t ip6; } u_addr; uint8_t type; } ip;
} esp_netif_dns_info_t;
#define ESP_IPADDR_TYPE_V4 0
#define ESP_IPADDR_TYPE_V6 6
#define IPSTR "%d.%d.%d.%d"
#define IP2STR(ip) ((uint8_t*)&(ip)->addr)[0], ((uint8_t*)&(ip)->addr)[1], ((uint8_t*)&(ip)->addr)[2], ((uint8_t*)&(ip)->addr)[3]
#define IPV6STR "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x"
#define BLOCK(ip,n) ((unsigned)(ntohl((ip).addr[(n)/2]) >> ((n)%2?0:16)) & 65535)
#define IPV62STR(ip) BLOCK(ip,0),BLOCK(ip,1),BLOCK(ip,2),BLOCK(ip,3),BLOCK(ip,4),BLOCK(ip,5),BLOCK(ip,6),BLOCK(ip,7)
esp_err_t esp_netif_get_ip_info(esp_netif_t *netif, esp_netif_ip_info_t *out);
esp_err_t esp_netif_get_dns_info(esp_netif_t *netif, esp_netif_dns_type_t type, esp_netif_dns_info_t *out);
