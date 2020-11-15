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

void adafruit_gfx_drawPixel(int x, int y, int color);

void adafruit_gfx_drawFastVLine(int x, int y, int h, int color);
void adafruit_gfx_drawFastHLine(int x, int y, int w, int color);

extern const uint8_t adafruit_logo[SSD1306_RAM_MIRROR_SIZE];
extern const GFXfont adafruit_gfx_default_font;

void adafruit_gfx_drawLine(int x0, int y0, int x1, int y1, int color);
void adafruit_gfx_drawRect(int x, int y, int w, int h, int color);
void adafruit_gfx_fillRect(int x, int y, int w, int h, int color);
void adafruit_gfx_fillScreen(int color);

void adafruit_gfx_drawCircle(int x0, int y0, int r, int color);
void adafruit_gfx_drawCircleHelper(int x0, int y0, int r,
      uint8_t cornername, int color);

void adafruit_gfx_fillCircle(int x0, int y0, int r, int color);
void adafruit_gfx_fillCircleHelper(int x0, int y0, int r, uint8_t cornername,
      int delta, int color);
void adafruit_gfx_drawTriangle(int x0, int y0, int x1, int y1,
      int x2, int y2, int color);
void adafruit_gfx_fillTriangle(int x0, int y0, int x1, int y1,
      int x2, int y2, int color);
void adafruit_gfx_drawRoundRect(int x0, int y0, int w, int h,
      int radius, int color);
void adafruit_gfx_fillRoundRect(int x0, int y0, int w, int h,
      int radius, int color);
void adafruit_gfx_drawBitmap(int x, int y, uint8_t *bitmap,
      int w, int h, int color, int bg);
void adafruit_gfx_drawXBitmap(int x, int y, const uint8_t *bitmap,
      int w, int h, int color);
void adafruit_gfx_drawChar(int x, int y, unsigned char c, int color,
      int bg, int size);
void adafruit_gfx_setCursor(int x, int y);
void adafruit_gfx_setTextColor(int c, int bg);
void adafruit_gfx_setTextSize(int ts);
void adafruit_gfx_setTextWrap(bool w);
void adafruit_gfx_setRotation(int r);
void adafruit_gfx_cp437(bool x);
void adafruit_gfx_setFont(const GFXfont *f);
void adafruit_gfx_getTextBounds(char *string, int x, int y, int ts,
      int *x1, int *y1, int *w, int *h);

size_t adafruit_gfx_write(uint8_t c);

int adafruit_gfx_height(void);
int adafruit_gfx_width(void);

int adafruit_gfx_getRotation(void);

// get current cursor position (get rotation safe maximum values, using: width() for x, height() for y)
int adafruit_gfx_getCursorX(void);
int adafruit_gfx_getCursorY(void);

#endif /* __adafruit_gfx_api_h_ */
