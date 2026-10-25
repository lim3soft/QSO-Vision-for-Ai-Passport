#include "talkgroup_model.h"
const bm_talkgroup_t bm_presets[] = {
    {91, "TG 91 Worldwide"}, {45401, "TG 45401"}, {46001, "TG 46001"},
    {46020, "TG 46020"}, {50210, "TG 50210"}, {0, "Other / custom"},
};
const size_t bm_preset_count = sizeof(bm_presets) / sizeof(bm_presets[0]);
bool bm_talkgroup_valid(int id) { return id >= 91 && id <= 16777215; }
