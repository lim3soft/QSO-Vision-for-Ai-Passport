[简体中文](brandmeister-monitor.zh_CN.md) · English

# Application architecture and service integration

The complete 2.3.3 source is under `main/`, with the FoloToy hardware support in
`components/bsp/`. `main/CMakeLists.txt` lists the application and generated font
sources. Historical demo files and upstream development docs remain for reference;
they are not the monitor's UI or entry point.

Hoseline audio uses its observed Spotter WebSocket service with an 8 kHz mono
mu-law stream. A bounded compressed queue feeds the codec in 20 ms blocks, with
an initial 180 ms reserve, linear speech levelling and peak headroom. Talkgroup
changes invalidate queued packets by generation so old audio is discarded.

Last-heard metadata uses BrandMeister's observed LH v2 Socket.IO transport.
Completed records are filtered by selected talkgroup, merged by SessionID and
retained up to 15, while only five rows are copied to the UI. Source IDs may be
resolved through a bounded RadioID lookup; caller/country data can be missing
or delayed. Country-prefix mapping indicates callsign allocation, not location.
Audio is prioritized over metadata when network/TLS memory is tight.

These are observed dashboard interfaces, not a guaranteed public audio API.
Services, metadata, codecs, software and trademarks remain subject to their
owners' terms and licenses. The application is independent and receive-only.
No personal BrandMeister key is embedded or required by the current adapter.

The UI timer draws a simulated activity EQ and local clock; it does not modify
audio samples. Network time synchronization starts after Wi-Fi obtains an IP.
Default timezone is Malaysia (UTC+08:00). Screen dimming changes only backlight
PWM: saved awake brightness 1–100%, automatic dim 1%, explicit off 0%. No CPU
sleep or network/codec restart is used. Charging animation is not enabled without
a verified charging signal. Voice recognition is not wired into this release;
custom talkgroups and Wi-Fi passwords use the buttons.

See [the guide](USER_GUIDE.md), [build instructions](BUILD.md), and
[validation record](VALIDATION.md). No power-saving current measurement or
exhaustive Wi-Fi/long-duration validation is claimed.

See [flags and display performance](country-flags-and-performance.md) and [BER/loss handling](brandmeister-quality.md) for 2.3.3 details. Metadata subscribes to `dst_<TG>` to avoid unrelated talkgroup traffic; audio processing is unchanged.
