#pragma once
#include <stdbool.h>

/* Display-only snapshot. No passwords or saved-profile contents. */
typedef struct {
    bool connected, signal_valid, ipv4_valid;
    char ssid[33];
    int rssi;
    char ip[16], subnet[16], gateway[16];
    char dns[3][40];
} bm_wifi_info_t;
