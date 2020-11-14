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


int adafruit_gfx_initialize(void);
void adafruit_gfx_reset(void);

void adafruit_gfx_clearDisplay(void);
int adafruit_gfx_display();

int adafruit_gfx_startScrollRight(uint8_t start, uint8_t stop);
int adafruit_gfx_startScrollLeft(uint8_t start, uint8_t stop);

int adafruit_gfx_startScrollDiagRight(uint8_t start, uint8_t stop);
int adafruit_gfx_startScrollDiagLeft(uint8_t start, uint8_t stop);
int adafruit_gfx_stopScroll(void);

void adafruit_gfx_drawPixel(int16_t x, int16_t y, uint16_t color);

void adafruit_gfx_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void adafruit_gfx_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

extern const uint8_t adafruit_logo[SSD1306_RAM_MIRROR_SIZE];
extern const uint8_t adafruit_gfx_default_font[];

void adafruit_gfx_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void adafruit_gfx_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void adafruit_gfx_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void adafruit_gfx_fillScreen(uint16_t color);

void adafruit_gfx_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void adafruit_gfx_drawCircleHelper(int16_t x0, int16_t y0, int16_t r,
      uint8_t cornername, uint16_t color);

void adafruit_gfx_fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void adafruit_gfx_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,
      int16_t delta, uint16_t color);
void adafruit_gfx_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
      int16_t x2, int16_t y2, uint16_t color);
void adafruit_gfx_fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
      int16_t x2, int16_t y2, uint16_t color);
void adafruit_gfx_drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
      int16_t radius, uint16_t color);
void adafruit_gfx_fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
      int16_t radius, uint16_t color);
void adafruit_gfx_drawBitmap(int16_t x, int16_t y, uint8_t *bitmap,
      int16_t w, int16_t h, uint16_t color, uint16_t bg);
void adafruit_gfx_drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap,
      int16_t w, int16_t h, uint16_t color);
void adafruit_gfx_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,
      uint16_t bg, uint8_t size);
void adafruit_gfx_setCursor(int16_t x, int16_t y);
void adafruit_gfx_setTextColor(uint16_t c, uint16_t bg);
void adafruit_gfx_setTextSize(uint8_t s);
void adafruit_gfx_setTextWrap(bool w);
void adafruit_gfx_setRotation(uint8_t r);
void adafruit_gfx_cp437(bool x);
void adafruit_gfx_setFont(const GFXfont *f);
void adafruit_gfx_getTextBounds(char *string, int16_t x, int16_t y,
      int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);

size_t adafruit_gfx_write(uint8_t);

int16_t adafruit_gfx_height(void);
int16_t adafruit_gfx_width(void);

uint8_t adafruit_gfx_getRotation(void);

// get current cursor position (get rotation safe maximum values, using: width() for x, height() for y)
int16_t adafruit_gfx_getCursorX(void);
int16_t adafruit_gfx_getCursorY(void);

#endif /* __adafruit_gfx_api_h_ */
