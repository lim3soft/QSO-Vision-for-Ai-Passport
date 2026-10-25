#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
qso_test_dir="$(mktemp -d /tmp/qso-host.XXXXXX)"
trap 'rm -rf -- "$qso_test_dir"' EXIT
qso_cc="${CC:-cc}"
qso_flags=(-std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -g -Imain)
if [[ "${SANITIZE:-1}" == 1 ]]; then
  qso_flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
fi
qso_json_flags=()
if [[ -n "${CJSON_SOURCE:-}" ]]; then
  qso_json_flags=("-I$(dirname "$CJSON_SOURCE")" "$CJSON_SOURCE")
elif [[ -n "${IDF_PATH:-}" && -f "$IDF_PATH/components/json/cJSON/cJSON.c" ]]; then
  qso_json_flags=("-I$IDF_PATH/components/json/cJSON" "$IDF_PATH/components/json/cJSON/cJSON.c")
elif pkg-config --exists libcjson; then
  read -r -a qso_json_flags <<< "$(pkg-config --cflags --libs libcjson)"
else
  echo 'Activate ESP-IDF, set CJSON_SOURCE, or install libcjson-dev/pkg-config.' >&2
  exit 1
fi
run_test() {
  local name="$1"; shift
  "$qso_cc" "${qso_flags[@]}" "$@" -o "$qso_test_dir/$name"
  "$qso_test_dir/$name"
}
run_test talkgroup main/test_talkgroup_parser.c main/talkgroup_model.c main/talkgroup_parser.c
run_test activity tests/test_qso_activity.c main/qso_ui_math.c
run_test dimmer tests/test_qso_dimmer.c main/qso_dimmer.c main/qso_controls.c
run_test screen tests/test_qso_screen.c main/qso_screen.c
run_test dsp tests/test_bm_dsp.c main/bm_dsp.c
run_test dsp-quality tests/test_bm_dsp_quality.c main/bm_dsp.c -lm
run_test identity tests/test_bm_identity.c main/bm_identity.c main/bm_protocol.c "${qso_json_flags[@]}"
run_test protocol tests/test_bm_protocol.c main/bm_protocol.c main/bm_qso_store.c main/qso_ui_math.c "${qso_json_flags[@]}"
run_test switch -Itests/hoseline_stubs tests/test_bm_switch.c main/bm_protocol.c main/bm_qso_store.c main/bm_identity.c main/bm_dsp.c "${qso_json_flags[@]}"
run_test directory -Itests/identity_http_stubs -Itests/hoseline_stubs tests/test_bm_identity_http.c main/bm_identity_lookup.c main/bm_identity.c main/bm_protocol.c "${qso_json_flags[@]}"
run_test wifi-info -Itests/wifi_info_stubs tests/test_wifi_info.c main/wifi_info.c
run_test wifi-recovery -Itests/wifi_monitor_stubs tests/test_wifi_recovery.c main/wifi_retry.c
printf 'QSO application host tests: PASS\n'
