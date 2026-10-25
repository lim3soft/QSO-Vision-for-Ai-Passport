[English](BUILD.md) · 简体中文

# 构建及安装

使用 **ESP-IDF 5.5.3**，目标为 ESP32-C3、8 MB 闪存、无 PSRAM。
SDK/组件是开发工具与依赖，不会把约 800 MB 的开发环境写入设备；2.3.3 完整固件
只有 1,894,144 字节。Windows/Linux 的环境准备见保留的
[环境部署文档](development/engineering/environment-setup.zh_CN.md)。

## 构建完整固件

```sh
git clone https://github.com/lim3soft/QSO-Vision-for-Ai-Passport.git
cd QSO-Vision-for-Ai-Passport
# 先激活已安装的 ESP-IDF 5.5.3 环境。
# Linux/macOS 示例：. /path/to/esp-idf/export.sh
idf.py --version
./tools/validate.sh --firmware
```

检查工具以已跟踪的 `sdkconfig.defaults` 在隔离目录中构建，按清单/锁定文件获取
组件，验证布局，生成 `build/FoloToy-AI-Passport-full.bin` 和按校验值索引的调试归档。
本地完整镜像相当于发布文件 `QSO-Vision.bin`，但时间戳/调试路径会改变校验值，
不声称重新构建与保留的作者实测镜像逐字节相同。

## 测试及预览

```sh
./tools/test_qso.sh
# 可选：SANITIZE=0 ./tools/test_qso.sh
# 获取 managed LVGL 组件后：
cmake -S tests/ui_preview -B build/ui-preview
cmake --build build/ui-preview -j4
build/ui-preview/qso_ui_preview
```

主机测试使用已激活 IDF 的 cJSON、`CJSON_SOURCE`，或 `pkg-config libcjson`
（Ubuntu 安装 `libcjson-dev`、`pkg-config`）。测试用桩驱动生产适配器，不需要在线
BM 音频流。保留的上游 `./tools/validate.sh` 全量检查仍含模板 `on_key` 契约，
不适用于本应用的 `on_button`，见[验证记录](VALIDATION.zh_CN.md)，不能宣称全量检查通过。

## 安装发布固件

1. 从同一版本下载 `QSO-Vision.bin` 与 `SHA256SUMS.txt`。日常使用建议 2.3.3；
   1.0/2.0 仅供有意回溯历史版本时选用。
2. 核对校验值，用 USB 数据线连接设备并关闭占用串口的监视器。
   ESP32-C3 / 8 MB 目标上的完整合并镜像必须写入 **0x0**，不能将单独应用镜像写入此地址。
3. 用兼容的 ESP 刷写工具写入并重启。不要求全片擦除；完整刷写可能重置 Wi-Fi/亮度等设置。
4. 根据[使用指南](USER_GUIDE.zh_CN.md)重新配网并选择通话组。

开发环境可对兼容分区使用 `idf.py -p COM5 flash monitor`（Windows）或
`idf.py -p /dev/ttyACM0 flash monitor`（Linux）进行 SDK 分段刷写，须先确认实际端口，
并不保证数据一定保留。
