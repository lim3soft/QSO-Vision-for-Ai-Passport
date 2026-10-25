<p align="right"><strong>简体中文</strong> · <a href="README.md">English</a></p>

# QSO Vision 字体

`qso_bold_12.c`、`qso_bold_14.c` 和 `qso_bold_28.c` 包含 ASCII
U+0020–U+007E 的 4 位、未压缩 LVGL 字形。使用真正的 DejaVu Sans
Condensed 粗体：历史呼号 12 px，QSO VISION 标题 14 px，当前呼号及
空闲 TG 28 px。这些字体不包含中文字形。

来源：[DejaVu Fonts](https://dejavu-fonts.github.io/)，版本 2.37，
Debian 软件包 `fonts-dejavu-core`。原文件为
`/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf`，SHA-256：
`21f02ebcea75529d3ed9d26e089cd1b9c59a4b784119ba3a96e9bc83cf755ec7`。
再分发许可见 `DejaVu-LICENSE.txt`。

生成的 C 数据直接编译到固件 Flash，不需要运行时字体加载、字体下载、
FreeType 或额外帧缓冲。固件与原生预览使用相同的字体源码。

在项目根目录重新生成（需要 Node/npm）：

```sh
for size in 12 14 28; do
  npm exec --yes --package=lv_font_conv@1.5.3 -- lv_font_conv \
    --font /usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf \
    --size "$size" --bpp 4 --format lvgl --range 32-126 --no-compress \
    --lv-include lvgl.h --lv-font-name "qso_bold_$size" \
    -o "assets/fonts/qso_bold_$size.c"
done
```

## 易读的常规字体

`qso_text_11.c`、`qso_text_12.c`、`qso_text_14.c`、`qso_text_16.c` 使用常规 DejaVu Sans
Condensed，分别用于分列历史（11 px）、状态（12 px）、About 及输入信息（14 px）、
自绘菜单项（16 px）。包含 ASCII、Latin-1 可打印字符、圆点及两种破折号，
不新增中文字形。继续适用 `DejaVu-LICENSE.txt`。源字体 SHA-256：
`6de96c09bf752e3774583eea6bc3fb0b60f1733c595f22a926c89219ba8ade43`。

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

`qso_bold_11.c` 是上述同一粗体源字体的数字/百分号/短横线子集，用于电池内
更易读且完整显示 `100%`。

```sh
npm exec --yes --package=lv_font_conv@1.5.3 -- lv_font_conv \
  --font /usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf \
  --size 11 --bpp 4 --format lvgl --symbols '0123456789%-' --no-compress \
  --lv-include lvgl.h --lv-font-name qso_bold_11 -o assets/fonts/qso_bold_11.c
```
