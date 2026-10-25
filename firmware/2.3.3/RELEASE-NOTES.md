[简体中文](RELEASE-NOTES.zh_CN.md) · English

# QSO VISION 2.3.3

- Added 256 offline flag pictures before current and history callsigns, with uppercase country labels.
- Added reported BER/loss to history. Missing or pending values show --; final-result updates preserve genuine zero measurements.
- Shortened history dates to dd/mm/yy and durations to m:ss, keeping date, time, duration and quality values on one line.
- Restored a fixed bottom-left volume/mute status beside right-to-left button instructions, with no overlap.
- Reduced repeated redraws and unrelated talkgroup data processing, improving instruction/EQ responsiveness and reducing background work while listening.
- Added interface gallery previews.

Download `QSO-Vision.bin` and verify `SHA256SUMS.txt`. This is the full merged image; flash at **0x0**. Wi-Fi/settings may need reconfiguration after a full flash. See the bundled bilingual guide or repository user guide. The audio processing and working date/time clock are unchanged.

Build: PASS. Application host/native UI tests: PASS. Device tests: NOT RUN for this final version-labelled rebuild; the owner confirmed the preceding feature build. The inherited template gate still fails the demo-only `on_key` check. Acoustic/endurance/router acceptance remains unverified.

Gallery images are native UI previews with sample data, not device photographs or live QSO records. Flags: flag-icons (MIT); keep the supplied license when redistributing artwork. Historical 1.0, 2.0 and 2.3 releases are retained.
