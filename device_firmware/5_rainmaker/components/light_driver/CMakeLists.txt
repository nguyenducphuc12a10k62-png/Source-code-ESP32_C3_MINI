idf_component_register(SRCS "iot_led.c" "light_driver.c"

                       INCLUDE_DIRS "include"

                       PRIV_REQUIRES driver esp_timer esp_common freertos)