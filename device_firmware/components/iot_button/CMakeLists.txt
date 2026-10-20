idf_component_register(
    SRCS "iot_button.c"
    INCLUDE_DIRS "include"
    # REQUIRES = PUBLIC deps (cần cho người khác include header của mình)
    REQUIRES esp_driver_gpio freertos
    # PRIV_REQUIRES = PRIVATE deps (chỉ .c của mình dùng)
    PRIV_REQUIRES log
)
