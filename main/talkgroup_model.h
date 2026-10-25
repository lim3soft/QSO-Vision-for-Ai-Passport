#pragma once
#include <stdbool.h>
#include <stddef.h>
typedef struct { int id; const char *label; } bm_talkgroup_t;
extern const bm_talkgroup_t bm_presets[];
extern const size_t bm_preset_count;
bool bm_talkgroup_valid(int id);
int bm_parse_spoken_number(const char *text);
