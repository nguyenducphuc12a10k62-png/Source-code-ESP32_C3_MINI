set(srcs
    "app_main.c"
)
set(include_dirs "include")

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES app_driver app_wifi app_storage app_webserver led_strip button light_driver 
)

