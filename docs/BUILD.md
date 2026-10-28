[简体中文](BUILD.zh_CN.md) · English

# Build and install

Use **ESP-IDF 5.5.3**, targeting ESP32-C3 / 8 MB flash, with no PSRAM. The SDK and
managed components are development tools/dependencies; they are not copied into
the device as an 800 MB image. The 2.3.3a complete firmware is 1,896,672 bytes.
See the retained [environment bootstrap](development/engineering/environment-setup.md)
for supported setup, including Windows and Linux prerequisites.

## Build the complete firmware

```sh
git clone https://github.com/lim3soft/QSO-Vision-for-Ai-Passport.git
cd QSO-Vision-for-Ai-Passport
# Activate your installed ESP-IDF 5.5.3 environment first.
# Linux/macOS example: . /path/to/esp-idf/export.sh
idf.py --version
./tools/validate.sh --firmware
```

The gate uses tracked `sdkconfig.defaults` in an isolated build, fetches the
managed components using the manifests/lock, verifies image layout, then writes
`build/FoloToy-AI-Passport-full.bin` and a hash-addressed debug archive. That
full image is the locally built equivalent of the release's `QSO-Vision.bin`.
Build timestamps and debug paths change hashes; rebuilding is not a claim of
bit-for-bit identity to the preserved creator-tested release image.

## Test and preview

```sh
./tools/test_qso.sh
# Optional: SANITIZE=0 ./tools/test_qso.sh
# After the managed LVGL component has been fetched:
cmake -S tests/ui_preview -B build/ui-preview
cmake --build build/ui-preview -j4
build/ui-preview/qso_ui_preview
```

The host suite uses cJSON from an active IDF installation, `CJSON_SOURCE`, or
`pkg-config libcjson` (Ubuntu: `libcjson-dev` and `pkg-config`). It exercises
production adapters with stubs; it does not require a live BrandMeister feed.
The retained upstream `./tools/validate.sh` full gate still contains a baseline
`on_key` contract that is not applicable to this app's `on_button` handler;
see the [validation record](VALIDATION.md). Do not describe that gate as passing.

## Install the downloaded release

1. Download firmware and `SHA256SUMS.txt` from the [v2.3.3a release](https://github.com/lim3soft/QSO-Vision-for-Ai-Passport/releases/tag/v2.3.3a), or use the community installer. Historical releases remain available.
2. Verify the digest, connect the Passport using a USB data cable, and close any
   serial monitor using the port. The full merged image must be written at **0x0**
   on the ESP32-C3 / 8 MB target. Do not write an application-only image at 0x0.
3. Use your compatible ESP flashing tool to write the image, then restart the
   unit. No full-chip erase is required. A complete merged flash can reset Wi-Fi
   and brightness settings; configure them again if needed.
4. Follow the [user guide](USER_GUIDE.md) to connect Wi-Fi and select a talkgroup.

For a development checkout with a compatible existing partition layout,
`idf.py -p COM5 flash monitor` (Windows) or
`idf.py -p /dev/ttyACM0 flash monitor` (Linux) uses the SDK's segmented-flash flow.
Confirm the actual port first. This does not guarantee preservation of data.

## App-only update from existing 2.3.3/2.3.3a

Download `QSO-Vision-APP-0x10000.bin` and verify it against the same release's `SHA256SUMS.txt`. Write only the application partition at **0x10000**, without erasing first, to retain Wi-Fi profiles/settings in NVS. This release's partition table was checked against published 2.3.3 and matches. This update path is for that compatible layout; use the full installation above for other or unknown firmware layouts.

Close the serial monitor, then use ESP-IDF 5.5.3/esptool (replace COM5 with your actual port):

```sh
python -m esptool --chip esp32c3 --port COM5 write_flash 0x10000 QSO-Vision-APP-0x10000.bin
```

Never flash the app-only image at 0x0, or the full `QSO-Vision.bin` at 0x10000.
