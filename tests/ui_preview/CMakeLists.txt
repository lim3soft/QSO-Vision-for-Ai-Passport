cmake_minimum_required(VERSION 3.16)
project(qso_ui_preview C CXX)
set(CMAKE_C_STANDARD 11)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
set(LV_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(LV_BUILD_SET_CONFIG_OPTS ON CACHE BOOL "" FORCE)
add_subdirectory(../../managed_components/lvgl__lvgl lvgl)
add_executable(qso_ui_preview preview.c ../../main/qso_ui.c ../../main/qso_ui_math.c
    ../../assets/fonts/qso_bold_12.c ../../assets/fonts/qso_bold_14.c ../../assets/fonts/qso_bold_28.c
    ../../assets/fonts/qso_text_12.c ../../assets/fonts/qso_text_14.c ../../assets/fonts/qso_text_16.c
    ../../assets/fonts/qso_text_11.c ../../assets/fonts/qso_bold_11.c)
target_include_directories(qso_ui_preview PRIVATE ../../main)
target_link_libraries(qso_ui_preview PRIVATE lvgl m)
