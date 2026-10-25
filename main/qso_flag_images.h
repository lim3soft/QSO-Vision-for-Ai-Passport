#pragma once
#include "lvgl.h"
#include "qso_flags.h"
/* Stable read-only descriptors and RGB565 pixels are memory-mapped Flash. */
const lv_image_dsc_t *qso_flag_image(const char *country);
