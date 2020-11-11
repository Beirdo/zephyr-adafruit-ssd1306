if(CONFIG_ADAFRUIT_SSD1306)

zephyr_include_directories(../include)

set(ZEPHYR_CURRENT_LIBRARY adafruit_ssd1306)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

zephyr_library()
zephyr_library_sources(
    ../src/adafruit-ssd1306.c
    ../src/glcdfont.c
    ../src/lcd_logo.c
)

endif()
