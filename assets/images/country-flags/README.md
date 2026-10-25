<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Offline country flag pictures

256 designs: 249 ISO countries/territories plus England, Northern Ireland,
Scotland, Wales, Kosovo, Ascension Island and Tristan da Cunha.

Source: [lipis/flag-icons](https://github.com/lipis/flag-icons), package 7.6.0.
The original artwork is MIT-licensed, copyright 2013 Panayiotis Lipiridis;
the complete [LICENSE](LICENSE) is retained and must accompany redistributions.
The downloaded archive and every vendored SVG are identified in
[provenance.json](provenance.json). The renderer version is recorded there.

`source/` contains the selected 4:3 SVG originals; `countries.json` is the
registry. `qso_flag_data.c` stores 16x12 RGB565 pixels and name/code aliases;
`qso_flag_images.c` stores stable LVGL image descriptors. Both are generated
and compiled into read-only Flash, not downloaded at runtime. Pixel storage
is exactly 98304 bytes (96 KiB); names, aliases and descriptors add Flash use.
No per-history flag buffers or extra network connections are allocated.

Country labels/codes are resolved case-insensitively, including existing model
labels, common English variants and unambiguous bounded 23-byte labels.
An unresolved country returns no picture. A supplied country is authoritative;
flags follow the app's existing reported/directory/callsign-prefix country,
which is not necessarily the operator's current physical location.

Regenerate with the existing Playwright/Chromium installation:

```bash
PLAYWRIGHT_MODULE=/path/to/playwright node tools/generate_country_flags.cjs
```

Normal firmware builds use the generated C directly and need no browser or
artwork download. `generated.json` records dimensions, count and data hash.
[preview-sheet.png](preview-sheet.png) is a magnified contact sheet of the
actual tiny raster pictures, visually inspected after conversion. The flags
are intentionally small; fine details are limited by 16x12 resolution.
