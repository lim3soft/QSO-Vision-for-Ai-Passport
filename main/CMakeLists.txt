idf_component_register(
    SRCS "main.c" "qso_ui.c" "qso_ui_math.c" "qso_screen.c" "qso_dimmer.c" "qso_controls.c" "bm_qso_store.c" "bm_dsp.c" "bm_identity.c" "bm_identity_lookup.c" "talkgroup_model.c" "talkgroup_parser.c" "wifi_monitor.c" "wifi_info.c" "wifi_retry.c" "hoseline_audio.c" "bm_protocol.c"
        "../assets/fonts/qso_bold_12.c" "../assets/fonts/qso_bold_14.c" "../assets/fonts/qso_bold_28.c"
        "../assets/fonts/qso_text_12.c" "../assets/fonts/qso_text_14.c" "../assets/fonts/qso_text_16.c"
        "../assets/fonts/qso_text_11.c"
        "../assets/fonts/qso_bold_11.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_event esp_hw_support esp_netif esp_timer esp_wifi nvs_flash esp_websocket_client esp_http_client json
)
