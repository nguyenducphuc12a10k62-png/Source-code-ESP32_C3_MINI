idf_component_register(SRCS "./light_driver.c" "./iot_led.c"
                       INCLUDE_DIRS "." "./include"
                       REQUIRES driver esp_timer app_storage
                       PRIV_REQUIRES esp_adc)