/*******************************************************************************
 * Size: 11 px
 * Bpp: 4
 * Opts: --font /usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf --size 11 --bpp 4 --format lvgl --symbols 0123456789%- --no-compress --lv-include lvgl.h --lv-font-name qso_bold_11 -o assets/fonts/qso_bold_11.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef QSO_BOLD_11
#define QSO_BOLD_11 1
#endif

#if QSO_BOLD_11

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0025 "%" */
    0x2d, 0xe8, 0x0, 0x97, 0x0, 0x9a, 0x2f, 0x12,
    0xd0, 0x0, 0x9a, 0x2f, 0x1b, 0x50, 0x0, 0x2c,
    0xd7, 0x4c, 0x0, 0x0, 0x0, 0x0, 0xd3, 0x8d,
    0xb1, 0x0, 0x6, 0xa2, 0xf1, 0xc8, 0x0, 0xe,
    0x12, 0xf1, 0xb8, 0x0, 0x88, 0x0, 0x9e, 0xc1,

    /* U+002D "-" */
    0x38, 0x84, 0x7f, 0xf9,

    /* U+0030 "0" */
    0x4, 0xdf, 0xc2, 0x0, 0xee, 0x7e, 0xd0, 0x5f,
    0x70, 0x9f, 0x37, 0xf5, 0x7, 0xf5, 0x7f, 0x50,
    0x7f, 0x55, 0xf7, 0x9, 0xf3, 0xe, 0xe7, 0xed,
    0x0, 0x4d, 0xfc, 0x20,

    /* U+0031 "1" */
    0x9e, 0xf9, 0x0, 0x99, 0xf9, 0x0, 0x3, 0xf9,
    0x0, 0x3, 0xf9, 0x0, 0x3, 0xf9, 0x0, 0x3,
    0xf9, 0x0, 0x58, 0xfb, 0x61, 0xdf, 0xff, 0xf3,

    /* U+0032 "2" */
    0x1b, 0xef, 0xc2, 0x3, 0xc7, 0x9f, 0xc0, 0x0,
    0x0, 0xff, 0x0, 0x0, 0x3f, 0xa0, 0x0, 0x3e,
    0xd1, 0x0, 0x3f, 0xd1, 0x0, 0x2f, 0xf8, 0x77,
    0x3, 0xff, 0xff, 0xf0,

    /* U+0033 "3" */
    0xc, 0xff, 0xc3, 0x0, 0x96, 0x8f, 0xd0, 0x0,
    0x3, 0xfd, 0x0, 0x1f, 0xfe, 0x30, 0x0, 0x36,
    0xfb, 0x0, 0x0, 0xd, 0xf1, 0x59, 0x68, 0xfe,
    0x3, 0xdf, 0xeb, 0x30,

    /* U+0034 "4" */
    0x0, 0x1e, 0xf6, 0x0, 0x9, 0xff, 0x60, 0x3,
    0xf7, 0xf6, 0x0, 0xd7, 0x5f, 0x60, 0x6e, 0x27,
    0xf7, 0x8, 0xff, 0xff, 0xf7, 0x25, 0x59, 0xf9,
    0x20, 0x0, 0x5f, 0x60,

    /* U+0035 "5" */
    0xf, 0xff, 0xfa, 0x0, 0xfc, 0x77, 0x50, 0xf,
    0x80, 0x0, 0x0, 0xff, 0xfe, 0x50, 0x8, 0x56,
    0xff, 0x0, 0x0, 0xb, 0xf2, 0x3b, 0x68, 0xfe,
    0x1, 0xbe, 0xfc, 0x30,

    /* U+0036 "6" */
    0x1, 0xaf, 0xfa, 0x0, 0xbf, 0x86, 0x90, 0x2f,
    0x90, 0x0, 0x5, 0xfd, 0xff, 0x70, 0x5f, 0xd4,
    0xcf, 0x23, 0xfa, 0x7, 0xf4, 0xd, 0xe5, 0xcf,
    0x10, 0x2c, 0xfd, 0x40,

    /* U+0037 "7" */
    0x5f, 0xff, 0xff, 0x12, 0x77, 0x7f, 0xe0, 0x0,
    0x4, 0xf8, 0x0, 0x0, 0xbf, 0x20, 0x0, 0x1f,
    0xb0, 0x0, 0x8, 0xf4, 0x0, 0x0, 0xee, 0x0,
    0x0, 0x5f, 0x70, 0x0,

    /* U+0038 "8" */
    0x7, 0xef, 0xd6, 0x2, 0xfd, 0x5e, 0xf0, 0x2f,
    0xb1, 0xcf, 0x0, 0x7f, 0xff, 0x50, 0x1e, 0xb4,
    0xde, 0x5, 0xf6, 0x8, 0xf3, 0x3f, 0xc5, 0xdf,
    0x10, 0x7e, 0xfd, 0x50,

    /* U+0039 "9" */
    0x5, 0xdf, 0xb1, 0x3, 0xfb, 0x5f, 0xb0, 0x6f,
    0x50, 0xcf, 0x15, 0xf9, 0x2e, 0xf3, 0xb, 0xff,
    0xff, 0x20, 0x2, 0x2b, 0xf0, 0x9, 0x59, 0xf8,
    0x0, 0xdf, 0xe9, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 159, .box_w = 10, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 40, .adv_w = 66, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 44, .adv_w = 110, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 72, .adv_w = 110, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 110, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 124, .adv_w = 110, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 152, .adv_w = 110, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 180, .adv_w = 110, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 110, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 236, .adv_w = 110, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 264, .adv_w = 110, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 292, .adv_w = 110, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x8
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 37, .range_length = 9, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    },
    {
        .range_start = 48, .range_length = 10, .glyph_id_start = 3,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t qso_bold_11 = {
#else
lv_font_t qso_bold_11 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 8,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 0,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if QSO_BOLD_11*/

