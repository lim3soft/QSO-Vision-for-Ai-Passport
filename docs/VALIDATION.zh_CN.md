[English](VALIDATION.md) · 简体中文

# 验证及来源

## 2.3.3 发布验证

- Build：PASS，ESP-IDF 5.5.3 隔离构建、合并布局及匹配调试归档。
- Host tests：PASS，应用 ASan/UBSan 主机测试、256 旗帜、质量字段、切组、音频与 Wi-Fi 回归；原生 UI 预览通过。
- Device tests：最终版本重编译包 NOT RUN；作者确认此前功能测试版正常。声学、路由器及长期验收仍未完成。
- 完整模板检查仍失败于演示专用 `on_key` 检查，未宣称通过。

完整镜像 SHA-256：`8f9a2198fd5d764945cd96931fb7b1e81d4c7ee986e1b6e99f44079e9005a483`，1,894,144 字节，从 0x0 刷入。
匹配 ELF SHA-256：`d3dbb983cb3a3e606eb0fafac0ad586474d869a7d978779afbc18ec64f9b8eb3`。
应用/构建快照见 `firmware/2.3.3/source-sha256.json`。图像为明确标注示例资料的界面渲染。
性能观测与限制见[旗帜及性能记录](country-flags-and-performance.zh_CN.md)。

## 历史 2.3 验证记录

2.3 发布镜像是作者实测的原镜像，SHA-256：
`d0f9f46e335af35d4f336ab8afc3b0d29f8a95ca2562de760f430b2324475824`。
对应 ELF SHA-256：
`49392e95092a71d95bc93c9586504dcca00620ccc00d78cfff3c05d5a1c7bcf5`。
原开发主机保留匹配的调试归档；公开清单记录 ELF 标识，不上传调试文件或私人主机资料。

- 构建：原 ESP-IDF 5.5.3 构建、合并布局及调试归档验证 PASS。
- 主机测试：协议/帧边界、切组与旧包拒绝、虚拟 DMA 连续性、语音处理、身份/国家
  保留、Wi-Fi 失败恢复/状态、15 条历史、时钟/EQ、亮度/唤醒策略均 PASS。
- UI 预览：共享 UI/字体源码的原生 LVGL 测试 PASS，沿用 24 KB 池。
  README 图片为示例数据渲染，不是实机实时截屏。
- 实机：作者反馈 2.3 亮度及最终时钟/EQ 正常；发布代理未执行串口/仪器化验收套件。
- 未验证：全部路由器兼容性、罕见呼号、所有条件下的长期播放、声学失真/安全测量及
  实测续航改善。

完整上游模板检查仍失败于 `tests/test_key_wake_contract.py` 的 `on_key` 断言。
本应用使用 `on_button` 与独立验证的 `qso_screen_key`。该检查保留且不宣称通过；
应用专用测试由 `tools/test_qso.sh` 及 QSO CI 单独运行。

1.0/2.0 为历史固件归档：发布前核对原合并镜像及调试归档标识，本次不重新实机测试，
标签不冒充原始应用源码树。完整当前源码位于 v2.3。

源码基于 FoloToy/ai-passport 的 `006c227` 提交，编译用的应用/配置与原快照一致，
见 `firmware/2.3/source-sha256.json`。新增 Git 提交、README、CI 不改变已保留镜像。
重新构建会改变时间戳，可能改变校验值。
