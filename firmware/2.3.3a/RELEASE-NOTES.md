[简体中文](RELEASE-NOTES.zh_CN.md) · English

# QSO VISION 2.3.3a

- Automatically finds and connects to another available saved Wi-Fi network when the last-used one cannot connect, using its stored password.
- Keeps a healthy connection, pauses automatic selection during manual Wi-Fi setup, and resumes recovery after leaving setup. Scans again later when no saved network is available.
- Preserves saved networks/passwords and prevents old settings from resetting the last-used network choice; failed attempts do not delete profiles.
- Replaced the Wi-Fi symbol with four clear wave lines plus a dot, green when connected and red when disconnected.

Full install: flash `QSO-Vision.bin` at **0x0**; Wi-Fi/settings may reset. On compatible existing 2.3.3/2.3.3a, update with `QSO-Vision-APP-0x10000.bin` at **0x10000**, without erasing first, to retain profiles. Both files are listed in `SHA256SUMS.txt`.

Build, application host tests and native UI preview passed. The owner confirmed the preceding Wi-Fi recovery build; the final icon rebuild was not hardware-tested by the publishing agent. The inherited template gate retains its demo-only `on_key` failure and is not claimed to pass. Previews are UI renders with sample data.
