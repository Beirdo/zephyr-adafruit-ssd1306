if(CONFIG_ADAFRUIIT_SSD1306)

zephyr_include_directories(../include)

zephyr_library(adafruit_ssd1306)
zephyr_library_sources(
    ../src/adafruit-ssd1306.c
    ../src/glcdfont.c
    ../src/lcd_logo.c
)

endif()
