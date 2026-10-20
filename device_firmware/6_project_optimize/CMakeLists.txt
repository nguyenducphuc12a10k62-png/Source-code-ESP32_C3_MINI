cmake_minimum_required(VERSION 3.5.1)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

add_compile_options(-fdiagnostics-color=always)

# TẤT CẢ components bạn có
set(EXTRA_COMPONENT_DIRS 
    ${CMAKE_CURRENT_LIST_DIR}/components/light_driver
    ${CMAKE_CURRENT_LIST_DIR}/components/iot_button
    ${CMAKE_CURRENT_LIST_DIR}/components/app_storage
    ${CMAKE_CURRENT_LIST_DIR}/components/app_wifi
    ${CMAKE_CURRENT_LIST_DIR}/components/qrcode
    ${CMAKE_CURRENT_LIST_DIR}/components/esp_rainmaker
    ${CMAKE_CURRENT_LIST_DIR}/components/esp_schedule
)
set(PROJECT_VER "3.5.1")
project(6_project_optimize)