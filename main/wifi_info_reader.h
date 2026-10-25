#pragma once
#include "wifi_info.h"
#include "esp_netif.h"

/* Read-only SDK queries; call from an application worker, outside LVGL lock. */
void bm_wifi_read_info(esp_netif_t *netif, bool connected, bm_wifi_info_t *out);
