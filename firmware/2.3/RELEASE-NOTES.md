[简体中文](RELEASE-NOTES.zh_CN.md) · English

# QSO VISION 2.3

- Improved speech continuity, loudness and distortion control to address gaps, crackle and quiet playback.
- Improved callsign/country completion and retention, reducing repeated unknown details after a caller has been identified.
- Retains up to 15 QSO history entries, five visible; short UP/DOWN scrolls one entry. Hold UP/DOWN for volume and double DOWN to mute/unmute.
- Added saved 1–100% screen brightness. Timeout choices are 1/5/10 minutes, Smart and Never; automatic dimming uses 1%, with the saved brightness restored on wake.
- Replaced the live card waveform with a date/time clock and segmented corner EQ activity animation that settles to zero when idle.
- Updated Chinese/English instructions and added the creator and web-version links to the description.

Complete source is included in this tag. The firmware is the exact creator-tested image, not a rebuild.
Flash the merged `QSO-Vision.bin` at **0x0**; settings may reset. Verify `SHA256SUMS.txt` first.
The UI/brightness/audio were reported working by the creator; this is not an exhaustive laboratory test.
