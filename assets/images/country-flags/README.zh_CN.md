<p align="right"><strong>简体中文</strong> · <a href="README.md">English</a></p>

# 离线国家旗帜图片

共 256 个设计：249 个 ISO 国家及地区，加英格兰、北爱尔兰、苏格兰、威尔士、
科索沃、阿森松岛和特里斯坦-达库尼亚。

来源：[lipis/flag-icons](https://github.com/lipis/flag-icons)，包版本 7.6.0。
原始图片采用 MIT 许可，版权为 2013 Panayiotis Lipiridis；完整
[LICENSE](LICENSE) 已保留，重新分发时须一并提供。
[provenance.json](provenance.json) 记录下载压缩包、每个 SVG 的校验值及渲染器版本。

`source/` 保存选用的 4:3 SVG 原图，`countries.json` 为索引。
生成的 `qso_flag_data.c` 保存 16x12 RGB565 像素和名称/代码别名；
`qso_flag_images.c` 保存生命周期稳定的 LVGL 图片描述符。两者编译到只读 Flash，
运行时无需下载。像素数据恰为 98304 字节（96 KiB），名称、别名和描述符另占少量
Flash，不为每条历史分配图片缓存，也不增加网络连接。

名称及代码忽略大小写，支持现有应用名称、常见英文别名及无歧义的 23 字节截断名称。
国家无法确认时不显示旗帜。已有国家信息优先，沿用现有网络/目录/呼号前缀解析结果，
不代表操作者当前的实际地理位置。

使用现有 Playwright/Chromium 重新生成：

```bash
PLAYWRIGHT_MODULE=/path/to/playwright node tools/generate_country_flags.cjs
```

正常编译直接使用生成的 C 文件，不需要浏览器或下载图片。
`generated.json` 记录尺寸、数量及数据校验值。
[preview-sheet.png](preview-sheet.png) 为实际小图放大的总览，转换后已打开检查。
16x12 分辨率适合小图标，复杂旗帜的细节受尺寸限制。
