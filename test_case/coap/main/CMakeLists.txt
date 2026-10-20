idf_component_register(SRCS "app_main.c" "app_driver.c"
                       INCLUDE_DIRS "."
                       # Bổ sung các component hệ thống mà code của bạn đang dùng
                       REQUIRES button app_storage libcoap light_driver esp_wifi esp_event nvs_flash
                      )