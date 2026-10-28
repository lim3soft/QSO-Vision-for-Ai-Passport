[简体中文](CHANGELOG.zh_CN.md)

# QSO VISION changelog

## 2.3.3a — 2026-10-28

- Automatically finds and connects to another available saved Wi-Fi network when the last-used one cannot connect, using its stored password.
- Keeps a healthy connection, pauses automatic selection during manual Wi-Fi setup, and resumes recovery after leaving setup. Scans again later when no saved network is available.
- Preserves saved networks/passwords and prevents old settings from resetting the last-used network choice; failed attempts do not delete profiles.
- Replaced the Wi-Fi symbol with four clear wave lines plus a dot, green when connected and red when disconnected.

## 2.3.3 — 2026-10-26

- Added 256 offline flag pictures before current and history callsigns, with uppercase country labels.
- Added reported BER/loss to history. Missing or pending values show --; final-result updates preserve genuine zero measurements.
- Shortened history dates to dd/mm/yy and durations to m:ss, keeping date, time, duration and quality values on one line.
- Restored a fixed bottom-left volume/mute status beside right-to-left button instructions, with no overlap.
- Reduced repeated redraws and unrelated talkgroup data processing, improving instruction/EQ responsiveness and reducing background work while listening.
- Added interface gallery previews.

## 2.3 — 2026-10-25

- Improved speech continuity, loudness and distortion control to address gaps, crackle and quiet playback.
- Improved callsign/country completion and retention, reducing repeated unknown details after a caller has been identified.
- Retains up to 15 QSO history entries, five visible; short UP/DOWN scrolls one entry. Hold UP/DOWN for volume and double DOWN to mute/unmute.
- Added saved 1–100% screen brightness. Timeout choices are 1/5/10 minutes, Smart and Never; automatic dimming uses 1%, with the saved brightness restored on wake.
- Replaced the live card waveform with a date/time clock and segmented corner EQ activity animation that settles to zero when idle.
- Updated Chinese/English instructions and added the creator and web-version links to the description.

## 2.0 — 2026-10-24 (historical archive)

Earlier published firmware: Wi-Fi profiles, preset/custom TGs, live audio/caller data, five historical QSOs and screen-off listening. Original binary and guide retained; exact historical source snapshot unavailable.

## 1.0 — 2026-10-24 (historical archive)

Last preserved firmware labelled Build: 1.0. Layout/archive verified; not re-tested. Exact historical source snapshot unavailable. Prefer 2.3.
