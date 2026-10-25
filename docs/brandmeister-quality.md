<p align="right"><a href="brandmeister-quality.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# BrandMeister BER and loss in QSO history

This revision is **2.3.3**, prepared after the owner confirmed the new features
on the device. It differs from the previously published 2.3 binary. It adds quality information to the existing
15-record history; five records remain visible and UP/DOWN still move one entry.

The approved layout shows an orange callsign followed by the **UPPERCASE cyan
country without a leading dot**. Date `dd/mm/yy`, time `hh:mm:ss`, duration `m:ss`
(for example `1:05`), BER and LOSS percentages share a **single second row**.
Aligned DATE / TIME / DUR / BER / LOSS column headings use the same 8 px font as
the detail values. BER is cyan and LOSS is lime. Five bubbles remain visible.
The live card is slightly tighter. A compact 8 px footer shows fixed `VOL-85%`
or `MUTED` at the left, with an independently clipped button-instruction strip
to the right. The instructions move **right to left at 25 pixels/second**, loop
continuously, and cover history scrolling, held-key volume, double-DOWN mute,
OK menu and held-OK screen off/wake. A fixed gap separates the two areas. The
history panel grows from 130 to 135 pixels while retaining five bubbles.

The ticker uses static ROM text, one clipped viewport and a child label. It
reuses the existing UI timer at 20 ms rather than creating another task/timer. It
pauses in menus and with explicit screen-off, and resumes on return/wake. Menu
instructions remain static and retain the full footer width. Native checks cover
left-to-right movement, wrap, clipping, unchanged volume pixels while scrolling,
menu pause, normal/muted views and separation from the history panel. The footer shares the activity timer; see the [flag/performance update](country-flags-and-performance.md)
for elapsed-time pacing, limited redraws and current validation.

## Source and meaning

Verified against BrandMeister's public services on 26 October 2026:

- [Current dashboard](https://brandmeister.network/#/lh) JavaScript
  [index-C18yufc6.js](https://brandmeister.network/assets/index-C18yufc6.js)
  formats `BER` as a fraction multiplied by 100. The app shows two decimal places,
  rather than the dashboard's rounded whole-percent tooltip. For example,
  `0.007092198356986046` becomes `0.71%`.
- The official master's [Last Heard implementation](https://master.brandmeister.es/status/extra/heard.js)
  computes loss from `LossCount / TotalCount * 100`, truncated to two decimal
  places. The app uses the same calculation: `1 / 79` becomes `1.26%`.
- A read-only probe of the existing public Last Heard Socket.IO v2 feed
  (`wss://api.brandmeister.network/lh/?EIO=4&transport=websocket&version=2`)
  requested TG 91's latest 15 records. All 15 `LH-Startup` records had positive
  total counters; six had non-null BER. `searchHouseComplete` was received.
  These observations are a sample, not a guarantee about every TG or report.

These are **per-transmission BrandMeister reports associated with the originating
link**, not an aggregate TG quality score, not the Passport's Wi-Fi packet loss,
and not a measurement of the Hoseline playback connection. The app cannot infer
RF BER when the link does not report it. Missing/null/negative/out-of-range BER,
invalid counters, or a zero total display `--`; reported zero with a positive
sample count displays `0.00%`. No additional API key or network session is used.

## Implementation and checks

The parser accepts JSON numeric and quoted numeric fields. Compact fixed-point
values with independent validity and explicit-field flags pass through startup
history, live session completion, duplicate updates and page expansion. Omitted
fields preserve earlier values; explicit unavailable values can clear them when
the report is at least as complete. BER zero with no positive packet count is a
provisional value and displays `--`; a nonzero BER update remains usable before
counters arrive. A completed positive-sample zero remains `0.00%`.

A live capture found that BM emits a zero-counter stop followed by the final
sampled report, sometimes with an earlier Stop timestamp. Final measured sample
counts therefore take priority over Stop ordering when merging quality. The
history duration still retains the longest end time. Later zero-counter reports
cannot erase final measured values. Measurements are session-specific; identity
backfill does not overwrite them. Serial metadata logs now include received
sample count and BER/LOSS for comparing device reports with the UI.

A read-only sample of 15 completed reports per TG found:

| TG | Nonzero BER | Unavailable BER | Nonzero loss | Zero loss | Unavailable loss |
| --- | ---: | ---: | ---: | ---: | ---: |
| 91 | 2 | 11 | 2 | 13 | 0 |
| 45401 | 0 | 15 | 0 | 12 | 3 |
| 46020 | 0 | 10 | 3 | 12 | 0 |

The two TG 46020 reports with `LossCount=1, TotalCount=50` are exactly `2.00%`.
TG 91 included `0.71%` BER and `0.33%`/`2.53%` loss. These are a time-limited
sample, not permanent TG quality statistics. A host replay of all 75 sampled
records preserved all nonzero rates and unavailable fields through parser,
store and paging. Zero can be an actual value reported by BM; the app cannot
invent RF measurements or establish whether a sending link measured RF errors.

`bm_qso_store_t` is **2384 bytes** on the host (the sampled-report correction
adds 8 bytes over the previous BER test build). History row size remains 144
bytes. Five UI rows and task-local snapshots grow slightly too; no new TLS
session, receive buffer, task or image asset is allocated. The accepted audio
DSP, queueing, codec, volume and playback behavior is unchanged. Only metadata
report logging changes within the streaming source file. The approved single-row
layout and uppercase countries are unchanged.

Run `tools/test_qso.sh` in the ESP-IDF environment for protocol/store, missing,
invalid, zero, units, duplicate/session isolation, paging and existing app tests.
The native LVGL preview includes normal, 100%, zero and unavailable samples and
checks five rows, text widths, menu help, clock/EQ and off/wake behavior.
`tools/bm_quality_probe.cjs` is an optional read-only online schema check using
Playwright/Chromium; it saves only quality/timing fields, not caller identities.

A successful firmware build and native preview do not establish device rendering
or live audio acceptance. The inherited complete gate still stops at its original
demo-only `on_key` contract; the QSO app uses its independently tested controls.
The previously released binary and GitHub/community releases are retained.
