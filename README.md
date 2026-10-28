[简体中文](README.zh_CN.md) · **English**

# QSO VISION for AI Passport

**Portable, receive-only BrandMeister talkgroup monitoring.**
Developed by **limesoft (9W3SPY)**. Current app build: **2.3.3a**.

Listen to available live DMR talkgroups over Wi-Fi, follow the current caller,
and browse recent QSOs using the Passport's physical buttons. No Android APK
or external phone is needed to operate or configure Wi-Fi.

[Download 2.3.3a](https://github.com/lim3soft/QSO-Vision-for-Ai-Passport/releases/tag/v2.3.3a) · [User guide](docs/USER_GUIDE.md) ·
[Build/install](docs/BUILD.md) · [Changelog](docs/CHANGELOG.md) ·
[Web version](https://qso.nightfall.monster) · [Creator](https://github.com/lim3soft)

## Screenshots

Native 2.3.3a UI renders with **sample data**, not device photographs or live QSO records.
The owner confirmed the preceding feature build; the final version-labelled rebuild has not been independently flashed by the publishing agent.

| On air, flags and reported quality | Idle with five visible history entries |
| --- | --- |
| ![On-air preview](assets/images/qso-vision/2.3.3a/cover.png) | ![History preview](assets/images/qso-vision/2.3.3a/gallery-history.png) |
| Presets and custom talkgroups | Saved brightness |
| ![Talkgroups](assets/images/qso-vision/2.3.3a/gallery-talkgroups.png) | ![Brightness](assets/images/qso-vision/2.3.3a/gallery-brightness.png) |

[Screen timeout preview](assets/images/qso-vision/2.3.3a/gallery-timeout.png)

## Features

- Presets **91, 45401, 46001, 46020, 50210**, plus button-entered custom talkgroups.
- Live Hoseline audio with background playback while the display is dimmed/off.
- Available callsign, name, country and QSO duration, with **15 history records,
  five visible**, scrolling one entry at a time.
- Current clock: **hh:mm:ss DAY dd/mm/yy**, default Malaysia UTC+08:00, synced
  over the network. Corner EQ bars animate activity and settle to zero when idle;
  they are a visual activity animation, not a measured frequency spectrum.
- On-device Wi-Fi scan/password keyboard and saved profiles; automatically tries other available saved networks if the last-used one cannot connect.
- Saved **1–100%** awake brightness. Timeouts: **1 / 5 / 10 minutes, Smart, Never**.
  Default Smart dims to **1% after five quiet seconds**; waking restores the saved level.
- Offline picture flags before callsigns (256 designs), uppercase countries and reported BER/loss in history; missing values show `--`.
- Fixed volume/mute status beside right-to-left button instructions.
- Battery percentage, mute/connection icons and a scrollable About/status page.

## Quick start and controls

1. For a complete install, flash `QSO-Vision.bin` at **0x0**; settings may reset.
   On existing 2.3.3/2.3.3a, use `QSO-Vision-APP-0x10000.bin` at **0x10000**
   to retain Wi-Fi profiles, without erasing first. See [installation steps](docs/BUILD.md#install-the-downloaded-release).
2. Short **OK → Options → Wi-Fi connection**; select your 2.4GHz network. Use
   UP/DOWN to choose password characters (hold for faster scrolling), OK to add,
   double UP to delete, and **double OK to connect**. Password limit: 25 characters.
   Success saves the profile and returns home; on failure, long OK returns to retry.
3. Short **OK → Change talkgroup** and choose a preset. For Custom: UP/DOWN
   chooses digits, OK adds, double UP deletes, double OK applies the group.

| Monitoring screen | Action |
| --- | --- |
| Short UP / DOWN | Scroll history one record toward newer / older |
| Hold UP / DOWN | Increase / decrease volume continuously |
| Double DOWN | Mute / unmute |
| Short OK | Open Options |
| Long OK | Fade screen off / wake at saved brightness; audio continues |
| In menus: UP/DOWN, OK, long OK | Select, confirm, back |

Only **long OK** wakes an explicitly blanked screen. Brightness and timeout
settings are under Options. See the full [English guide](docs/USER_GUIDE.md) or
[bilingual text guide](firmware/2.3.3a/User-Guide-CN-EN.txt).

App-only update: [QSO-Vision-APP-0x10000.bin](https://github.com/lim3soft/QSO-Vision-for-Ai-Passport/releases/download/v2.3.3a/QSO-Vision-APP-0x10000.bin), flash only at **0x10000**. The full image and app-only image use different offsets.

## Releases and original firmware

| Version | Download | Source status |
| --- | --- | --- |
| **2.3.3a — current** | [Release](https://github.com/lim3soft/QSO-Vision-for-Ai-Passport/releases/tag/v2.3.3a) · [BIN](https://github.com/lim3soft/QSO-Vision-for-Ai-Passport/releases/download/v2.3.3a/QSO-Vision.bin) | Current source and tag v2.3.3a |
| 2.3.3 — historical | [Release](https://github.com/lim3soft/QSO-Vision-for-Ai-Passport/releases/tag/v2.3.3) · [BIN](https://github.com/lim3soft/QSO-Vision-for-Ai-Passport/releases/download/v2.3.3/QSO-Vision.bin) | Matching source retained in tag v2.3.3 |
| 2.3 — historical | [Release](https://github.com/lim3soft/QSO-Vision-for-Ai-Passport/releases/tag/v2.3) · [BIN](https://github.com/lim3soft/QSO-Vision-for-Ai-Passport/releases/download/v2.3/QSO-Vision.bin) | Matching source retained in tag v2.3 |
| 2.0 — historical | [Release](https://github.com/lim3soft/QSO-Vision-for-Ai-Passport/releases/tag/v2.0) · [BIN](https://github.com/lim3soft/QSO-Vision-for-Ai-Passport/releases/download/v2.0/QSO-Vision.bin) | Original published firmware and guide; archive-only tag |
| 1.0 — historical | [Release](https://github.com/lim3soft/QSO-Vision-for-Ai-Passport/releases/tag/v1.0) · [BIN](https://github.com/lim3soft/QSO-Vision-for-Ai-Passport/releases/download/v1.0/QSO-Vision.bin) | Last preserved 1.0-labelled image; archive-only tag |

Each release includes a manifest and SHA-256 checksums. The `.bin` files are
also retained under `firmware/<version>/` as explicitly requested. Historical
1.0/2.0 tags **do not pretend to be original source snapshots**: matching source
was not retained. Use 2.3.3a for current operation and the full buildable source.

The 2.3.3a merged image is **1,896,672 bytes**, SHA-256:
`19e0959545a257dea019a090ece4dac1ddf66676d887a83f8d7aa9a3bdef19e8`.
Build scripts/metadata are separate from the preserved release firmware;
a rebuild can produce a different digest due to timestamps/debug paths.

## Build, tests and limitations

Target: **FoloToy AI Passport, ESP32-C3, 8 MB flash, no PSRAM**. Use **ESP-IDF 5.5.3**.
Activate IDF, then run `./tools/validate.sh --firmware`. Run `./tools/test_qso.sh`
for application host tests. See [build instructions](docs/BUILD.md),
[architecture](docs/brandmeister-monitor.md) and [validation details](docs/VALIDATION.md).

Build/layout/archive and application host/UI tests passed. The owner confirmed
the preceding feature build; this final version-labelled rebuild has not been
independently flashed here. Exhaustive router, acoustic, endurance and
battery-life measurements have not been performed. The inherited template's
full gate still fails its baseline `on_key` contract; it is retained and is
not claimed to pass. Application-specific CI runs the relevant tests separately.

Audio and identity/history depend on external services and may be unavailable,
delayed or incomplete. Custom TGs use buttons; voice recognition is not included.
The device UI is English; documentation is bilingual. Charging animation is not
enabled without a verified charging signal. This is **receive-only**, with no
DMR transmit function.

## Credits and licensing

Application: **limesoft (9W3SPY)**. Hardware support and template:
[FoloToy/ai-passport](https://github.com/FoloToy/ai-passport), base commit `006c227`.
The original MIT copyright/license is retained in [LICENSE](LICENSE).
[Font licenses](assets/fonts/DejaVu-LICENSE.txt) and [NOTICE](NOTICE) retain attribution.
Flags: [flag-icons](assets/images/country-flags/README.md), MIT; the complete notice is supplied and embedded in firmware.
ESP-IDF and managed components keep their respective licenses.

Independent project; no affiliation with or endorsement by BrandMeister,
Hoseline, RadioID or FoloToy. Their services, data and trademarks remain subject
to their owners' terms/licenses. [Creator](https://github.com/lim3soft) ·
[Web QSO VISION](https://qso.nightfall.monster).
