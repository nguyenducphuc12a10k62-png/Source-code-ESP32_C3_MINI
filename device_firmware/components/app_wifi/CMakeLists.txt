idf_component_register(SRCS "app_wifi.c"
                       INCLUDE_DIRS "."
                       # Thêm 'esp_rainmaker' vào đây
                       REQUIRES wifi_provisioning qrcode nvs_flash esp_rainmaker esp_pm
                      )

if(CONFIG_APP_WIFI_SHOW_DEMO_INTRO_TEXT)
    target_compile_definitions(${COMPONENT_TARGET} PRIVATE "-D RMAKER_DEMO_PROJECT_NAME=\"${CMAKE_PROJECT_NAME}\"")
endif()