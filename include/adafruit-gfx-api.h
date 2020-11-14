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
 
 /*
 * Copyright (c) 2020 Gavin Hurlbut
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#ifndef __adafruit_gfx_api_h_
#define __adafruit_gfx_api_h_

#include <zephyr.h>
#include "adafruit-gfx-defines.h"
#include "adafruit-gfx-font.h"


int SSD1306_initialize(void);
void SSD1306_reset(void);

void SSD1306_clearDisplay(void);
int SSD1306_display();

int SSD1306_startScrollRight(uint8_t start, uint8_t stop);
int SSD1306_startScrollLeft(uint8_t start, uint8_t stop);

int SSD1306_startScrollDiagRight(uint8_t start, uint8_t stop);
int SSD1306_startScrollDiagLeft(uint8_t start, uint8_t stop);
int SSD1306_stopScroll(void);

void SSD1306_drawPixel(int16_t x, int16_t y, uint16_t color);

void SSD1306_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void SSD1306_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

extern const uint8_t adafruit_logo[SSD1306_RAM_MIRROR_SIZE];
extern const uint8_t adafruit_gfx_default_font[];

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

#endif /* __adafruit_gfx_api_h_ */
