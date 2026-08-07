idf_component_register(SRCS "proj_vl53l0x.c"
PRIV_REQUIRES "esp_driver_gpio" "esp_driver_i2c" "vl53l0x" "proj_wifi" "proj_mqtt"
                    INCLUDE_DIRS "include")
