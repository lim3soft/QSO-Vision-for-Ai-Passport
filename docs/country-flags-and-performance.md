<p align="right"><a href="country-flags-and-performance.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Country flags and monitor performance testing update

Release **2.3.3**, prepared after the owner confirmed the feature build, adds real 16x12 RGB565
pictures immediately **before the live callsign and each history callsign**.
Country text stays uppercase and the five-row date/time/duration/BER/loss layout
and fixed VOL-xx%/MUTED status remain. Missing/unresolved country hides the flag
and restores the available callsign width; it never substitutes an unrelated flag.
Live flags are hidden in menus and while idle. Common long callsigns still scroll.

The 256 offline designs cover 249 ISO countries/territories and seven additional
regional/territorial flags. Names, codes, common aliases and unambiguous truncated
country labels are mapped without a network request. Existing country resolution
remains authoritative. These flags do not establish current physical location.
Original SVGs, generated C, complete MIT license, provenance and the inspected
contact sheet are in [the flag assets](../assets/images/country-flags/README.md).
Pixel data uses exactly 96 KiB of read-only Flash; aliases and descriptors add
Flash, not 256 per-device RAM bitmaps. Stable descriptors avoid stack-lifetime or
stale-pointer image bugs; the existing image/header cache remains disabled.

## Changes targeting animation/audio contention

- Subscribe to BrandMeister's official `dst_<TG>` room instead of `everything`.
  Keep the existing startup current-session request and 15-record history query.
  A newly selected TG still gets a separate socket subscription, preserving the
  existing switch/reconnect protection. The metadata task priority is 3, below
  audio ingress/playback (default 5 / 6) and display work (4).
- Do not redraw the entire history or rewrite its heading for unchanged snapshots.
  Do not repaint unchanged status icons. Monitor labels update only when their
  text changes, avoiding repeated heap formatting and callsign-scroll resets.
- The instruction strip now moves **right to left**, as corrected by the owner.
  Its 25 px/second position follows elapsed time, not callback count, so delayed
  callbacks skip to the current position instead of slowing the scroll. A shared
  20 ms timer paces EQ updates at 40 ms and clock updates at changed seconds.
  No new task or timer is introduced; menu/manual-screen-off pause is retained.
- Audio DSP, gain, packet queue, startup delay and PCM playback are unchanged.
  UI/network workload reduction is intended to protect audio scheduling; it is
  not a verified device audio result or repair of upstream lost audio.

## Evidence and limits

A simultaneous read-only 15-second public-feed probe received 259 events / 173228
text bytes in `everything` (251 unrelated to TG 91), versus 8 events / 5332 bytes
in `dst_91` (zero unrelated TGs). This is an observed sample, not a guaranteed
traffic reduction under all network activity.

A deterministic native LVGL workload ran ten simulated seconds with twenty
unchanged history/status snapshots. Compared with the previous firmware UI:

| Mode | Before: flushed pixels | After: flushed pixels | Reduction |
| --- | ---: | ---: | ---: |
| Idle monitor | 896740 | 375000 | 58.2% |
| Active EQ | 1315060 | 1002772 | 23.7% |

Unchanged-history draw callbacks fell from 120 partial-buffer callbacks to zero.
This measures native draw work, **not physical ESP32-C3 frame rate or audio**;
real frame scheduling, SPI and Wi-Fi still require the owner's device test.
The active result may vary slightly if the host wall clock crosses a second.

Host checks cover all 256 flag names/codes, aliases, unresolved inputs, bounded
names, correct TG room requests, metadata priority, existing audio queue/DMA
regressions and switching. Native checks cover flags preceding callsigns, unknown
and idle/menu hiding, separated footer regions, right-to-left elapsed-time scroll,
wrap after a delayed callback, unchanged-history redraw suppression and existing
brightness/off/wake behavior. The original template gate still fails its demo-only
`on_key` contract; the app uses its independently tested controls. Device validation
was reported successful by the owner for the feature build. This version-only
rebuild was not independently flashed here; native measurements are not hardware FPS results.
