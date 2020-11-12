/*********************************************************************
This is a library for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

These displays use SPI to communicate, 4 or 5 pins are required to
interface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

/*
 * Reworked by Gavin Hurlbut <gjhurlbu@gmail.com> to use an attached SPI FRAM
 * for buffer storage, default to 128x64 display, I2C only
 *
 * changes (c) 2016 Gavin Hurlbut <gjhurlbu@gmail.com>
 * released under the BSD License
 *
 * Ported to C for the Cypress PSoC chipset using FreeRTOS
 * changes (c) 2019 Gavin Hurlbut <gjhurlbu@gmail.com>
 * released under an MIT License
 *
 * Ported to be used with Zephyr Project
 * changes (c) 2020 Gavin Hurlbut <gjhurlbu@gmail.com>
 * released under Apache 2.0 License
 */

#ifndef _SSD1306_H_
#define _SSD1306_H_

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include "gfxfont.h"

#define BLACK 0
#define WHITE 1
#define INVERSE 2

#define SSD1306_LCDWIDTH    DT_PROP(DT_INST(0, solomon_ssd1306fb), width)
#define SSD1306_LCDHEIGHT   DT_PROP(DT_INST(0, solomon_ssd1306fb), height)

#define SSD1306_RAM_MIRROR_SIZE (SSD1306_LCDWIDTH * SSD1306_LCDHEIGHT / 8)

#define SSD1306_PIXEL_ADDR(x, y) ((x) + ((y) >> 3) * SSD1306_LCDWIDTH)
#define SSD1306_PIXEL_MASK(y)	 (1 << ((y) & 0x07))

#if (!((SSD1306_LCDWIDTH == 128 && (SSD1306_LCDHEIGHT == 64 || SSD1306_LCDHEIGHT == 32)) || \
       (SSD1306_LCDWIDTH == 96 && SSD1306_LCDHEIGHT == 16)))
  #error "Only LCD dimensions supported are:  128x64, 128x32, 96x16  " #SSD1306_WIDTH "  " #SSD1306_HEIGHT
#endif

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF

#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA

#define SSD1306_SETVCOMDETECT 0xDB

#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9

#define SSD1306_SETMULTIPLEX 0xA8

#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10

#define SSD1306_SETSTARTLINE 0x40

#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22

#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8

#define SSD1306_SEGREMAP 0xA0

#define SSD1306_CHARGEPUMP 0x8D

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

// Scrolling #defines
#define SSD1306_ACTIVATE_SCROLL 0x2F
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

typedef enum {
    SET_BITS,
    CLEAR_BITS,
    TOGGLE_BITS,
} oper_t;

int SSD1306_initialize(uint8_t vccstate);
void SSD1306_setVccstate(uint8_t vccstate);
void SSD1306_reset(void);

void SSD1306_displayOff(void);
void SSD1306_displayOn(void);
void SSD1306_clearDisplay(void);
void SSD1306_invertDisplay(uint8_t i);
void SSD1306_display();

void SSD1306_startScrollRight(uint8_t start, uint8_t stop);
void SSD1306_startScrollLeft(uint8_t start, uint8_t stop);

void SSD1306_startScrollDiagRight(uint8_t start, uint8_t stop);
void SSD1306_startScrollDiagLeft(uint8_t start, uint8_t stop);
void SSD1306_stopScroll(void);

void SSD1306_dim(bool dim);

void SSD1306_drawPixel(int16_t x, int16_t y, uint16_t color);

void SSD1306_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void SSD1306_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

extern const uint8_t lcd_logo[SSD1306_RAM_MIRROR_SIZE];
extern const uint8_t default_font[];

// And now for the parts from the old base class.  Note: these have all been
// renamed to being SSD1306, but originally were from Adafruit_GFX class

void SSD1306_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void SSD1306_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void SSD1306_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void SSD1306_fillScreen(uint16_t color);

void SSD1306_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void SSD1306_drawCircleHelper(int16_t x0, int16_t y0, int16_t r,
      uint8_t cornername, uint16_t color);

void SSD1306_fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void SSD1306_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,
      int16_t delta, uint16_t color);
void SSD1306_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
      int16_t x2, int16_t y2, uint16_t color);
void SSD1306_fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
      int16_t x2, int16_t y2, uint16_t color);
void SSD1306_drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
      int16_t radius, uint16_t color);
void SSD1306_fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
      int16_t radius, uint16_t color);
void SSD1306_drawBitmap(int16_t x, int16_t y, uint8_t *bitmap,
      int16_t w, int16_t h, uint16_t color, uint16_t bg);
void SSD1306_drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap,
      int16_t w, int16_t h, uint16_t color);
void SSD1306_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,
      uint16_t bg, uint8_t size);
void SSD1306_setCursor(int16_t x, int16_t y);
void SSD1306_setTextColor(uint16_t c, uint16_t bg);
void SSD1306_setTextSize(uint8_t s);
void SSD1306_setTextWrap(bool w);
void SSD1306_setRotation(uint8_t r);
void SSD1306_cp437(bool x);
void SSD1306_setFont(const GFXfont *f);
void SSD1306_getTextBounds(char *string, int16_t x, int16_t y,
      int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);

size_t SSD1306_write(uint8_t);

int16_t SSD1306_height(void);
int16_t SSD1306_width(void);

uint8_t SSD1306_getRotation(void);

// get current cursor position (get rotation safe maximum values, using: width() for x, height() for y)
int16_t SSD1306_getCursorX(void);
int16_t SSD1306_getCursorY(void);

#endif /* _SSD1306_H_ */
