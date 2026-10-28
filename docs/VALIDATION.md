[简体中文](VALIDATION.zh_CN.md) · English

# Validation and provenance

## 2.3.3a release validation

Build, application host tests and native UI preview: PASS. The owner confirmed saved-network Wi-Fi recovery; the final four-wave icon/version rebuild has not been independently flashed by the publishing agent (Device tests: NOT RUN). The full template gate retains its demo-only `on_key` failure.

Full BIN: `19e0959545a257dea019a090ece4dac1ddf66676d887a83f8d7aa9a3bdef19e8`, 1,896,672 bytes, flash at 0x0. Matching ELF: `389a1ff5d3b00262a59adc6597493c5a79f8f56c24d6b90cf807a33901532af7`. App-only metadata is in `firmware/2.3.3a/manifest.json`; its bytes match the full image's application at 0x10000. Compiled source snapshot: `firmware/2.3.3a/source-sha256.json`.

## 2.3.3 release validation

- Build: PASS, ESP-IDF 5.5.3 isolated build, merged layout and matching debug archive.
- Host tests: PASS, application ASan/UBSan tests, 256 flags, quality fields, switching, audio and Wi-Fi regressions; native UI preview passed.
- Device tests: NOT RUN for the final version-labelled rebuild; the owner confirmed the preceding feature build. Acoustic, router and endurance acceptance remains unverified.
- The complete template gate still fails the inherited demo-only `on_key` contract; it is not claimed to pass.

Full image SHA-256: `8f9a2198fd5d764945cd96931fb7b1e81d4c7ee986e1b6e99f44079e9005a483`, 1,894,144 bytes, flash at 0x0.
Matching ELF SHA-256: `d3dbb983cb3a3e606eb0fafac0ad586474d869a7d978779afbc18ec64f9b8eb3`.
Application/build snapshot: `firmware/2.3.3/source-sha256.json`. Images are labelled UI renders with sample data.
See [flags and performance evidence](country-flags-and-performance.md) for measurements and limits.

## Historical 2.3 validation record

The released 2.3 image is the exact image tested by the creator, SHA-256
`d0f9f46e335af35d4f336ab8afc3b0d29f8a95ca2562de760f430b2324475824`.
Matching ELF SHA-256:
`49392e95092a71d95bc93c9586504dcca00620ccc00d78cfff3c05d5a1c7bcf5`.
The original development host retains its matching debug archive. Public release
manifests identify the ELF without publishing debug files or private host data.

- Build: PASS for the original ESP-IDF 5.5.3 build, merged image/layout and debug archive.
- Host tests: PASS for the application suite, including protocol/frame bounds,
  talkgroup switching/stale-packet rejection, virtual DMA continuity, speech
  processing, identity/country retention, Wi-Fi failure recovery/diagnostics,
  15-row history, clock/EQ and brightness/wake policy.
- UI preview: PASS using the same UI/font source in native LVGL, within the
  existing 24 KB pool. README images use sample data, not a live device capture.
- Device: creator reported the 2.3 brightness and final clock/EQ build working.
  No serial/instrumented acceptance suite was performed by the publishing agent.
- Unverified: exhaustive router compatibility, rare callsign data, long-duration
  streaming under all conditions, acoustic safety/distortion measurements and
  measured battery-life gains.

The complete inherited template gate still fails
`tests/test_key_wake_contract.py`, which expects `on_key` from the hardware demo.
This app uses `on_button` and its tested `qso_screen_key` guard. The upstream
check is retained and not marked as passed; `tools/test_qso.sh` and the QSO CI
job run the application-specific checks separately.

1.0 and 2.0 are historical firmware archives. Their exact original merged
images and debug-archive identity were verified before publishing, but they
were not re-tested for this release. Those tags deliberately do not claim to
contain their original application source. Complete current source is in v2.3.

The source starts from FoloToy/ai-passport commit `006c227`. The compiled
application/configuration matches the retained build snapshot; see
`firmware/2.3/source-sha256.json`. Git commits, README and CI scripts were added
for publishing; they do not change the preserved binary. Rebuilding changes
build timestamps and can change the resulting digest.
