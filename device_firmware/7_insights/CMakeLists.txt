cmake_minimum_required(VERSION 3.5)
add_compile_options(-fdiagnostics-color=always)



set(EXTRA_COMPONENT_DIRS
  ${CMAKE_CURRENT_LIST_DIR}/components/app_insights
  ${CMAKE_CURRENT_LIST_DIR}/components/light_driver
  ${CMAKE_CURRENT_LIST_DIR}/components/button
  ${CMAKE_CURRENT_LIST_DIR}/components/app_storage
  ${CMAKE_CURRENT_LIST_DIR}/components/app_wifi
  ${CMAKE_CURRENT_LIST_DIR}/components/qrcode
  ${CMAKE_CURRENT_LIST_DIR}/components/esp_rainmaker
  ${CMAKE_CURRENT_LIST_DIR}/components/esp_schedule
  ${CMAKE_CURRENT_LIST_DIR}/components/json_generator
  ${CMAKE_CURRENT_LIST_DIR}/components/json_parser
  ${CMAKE_CURRENT_LIST_DIR}/components/rmaker_common
)

# ... các dòng thiết lập RAIMAKER_DIR, EXTRA_COMPONENT_DIRS ở trên ...

# BẮT BUỘC đặt trước project.cmake
set(EXCLUDE_COMPONENTS wifi_provisioning)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(PROJECT_VER "3.8")

project(7_insights)  
