idf_component_register(SRCS "ldr.c" "display.c" "m5_lcd.c" "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES aht i2cdev driver esp_adc)
