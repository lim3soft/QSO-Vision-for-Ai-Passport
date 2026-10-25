<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# QSO Vision fonts

`qso_bold_12.c`, `qso_bold_14.c`, and `qso_bold_28.c` contain 4-bit,
uncompressed LVGL glyphs for ASCII U+0020–U+007E. These are true bold
DejaVu Sans Condensed glyphs, used by history callsigns (12 px), the
QSO VISION wordmark (14 px), and the current callsign/idle TG (28 px).
They do not provide Chinese glyph coverage.

Source: [DejaVu Fonts](https://dejavu-fonts.github.io/), version 2.37,
Debian package `fonts-dejavu-core`. Original file:
`/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf`, SHA-256
`21f02ebcea75529d3ed9d26e089cd1b9c59a4b784119ba3a96e9bc83cf755ec7`.
Redistribution terms are included in `DejaVu-LICENSE.txt`.

Generated C is compiled into firmware Flash; no runtime font loader,
FreeType, download, or additional framebuffer is used. Firmware and native
preview compile the same fonts.

Regenerate from the project root (Node/npm required):

```sh
for size in 12 14 28; do
  npm exec --yes --package=lv_font_conv@1.5.3 -- lv_font_conv \
    --font /usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf \
    --size "$size" --bpp 4 --format lvgl --range 32-126 --no-compress \
    --lv-include lvgl.h --lv-font-name "qso_bold_$size" \
    -o "assets/fonts/qso_bold_$size.c"
done
```

## Readable regular fonts

`qso_text_11.c`, `qso_text_12.c`, `qso_text_14.c`, and `qso_text_16.c` use regular DejaVu Sans
Condensed for separated history columns (11 px), status text (12 px), About and input text (14 px), and
drawn menu entries (16 px). They include ASCII, Latin-1 printable characters,
bullet, en dash, and em dash; they do not add Chinese glyph coverage. The
same `DejaVu-LICENSE.txt` applies. Source font SHA-256:
`6de96c09bf752e3774583eea6bc3fb0b60f1733c595f22a926c89219ba8ade43`.

```sh
for size in 11 12 14 16; do
  npm exec --yes --package=lv_font_conv@1.5.3 -- lv_font_conv \
    --font /usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf \
    --size "$size" --bpp 4 --format lvgl \
    --range 32-126,160-255,0x2022,0x2013,0x2014 --no-compress \
    --lv-include lvgl.h --lv-font-name "qso_text_$size" \
    -o "assets/fonts/qso_text_$size.c"
done
```

`qso_bold_11.c` is a compact digit/percent/dash subset of the same bold source
font used above. It makes `100%` readable inside the battery without clipping.

```sh
npm exec --yes --package=lv_font_conv@1.5.3 -- lv_font_conv \
  --font /usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf \
  --size 11 --bpp 4 --format lvgl --symbols '0123456789%-' --no-compress \
  --lv-include lvgl.h --lv-font-name qso_bold_11 -o assets/fonts/qso_bold_11.c
```
