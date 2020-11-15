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
All text above, and the splash screen below must be included in any redistribution
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


#include <logging/log.h>
LOG_MODULE_REGISTER(adafruit_ssd1306, CONFIG_DISPLAY_LOG_LEVEL);

#include <zephyr.h>
#include <string.h>
#include <device.h>
#include <drivers/display.h>

#include "adafruit-gfx-defines.h"
#include "adafruit-gfx-cache.h"
#include "adafruit-gfx-api.h"
#include "adafruit-gfx-font.h"
#include "adafruit-gfx-utils.h"

static void _drawFastVLineInternal(int x, int y, int h, int color);
static void _drawFastHLineInternal(int x, int y, int w, int color);
static int _draw_pixels_masked(int x, int y, int color, uint8_t mask);

extern int ssd1306_display_write(const struct device *dev, uint8_t *buf, size_t len, bool command);

struct adafruit_ssd1306_data_t {
  const struct device *dev;
  struct adafruit_gfx_cache_t cache;
  struct adafruit_gfx_cache_source_t draw_cache;
  struct adafruit_gfx_cache_source_t adafruit_logo;
#ifndef CONFIG_ADAFRUIT_SSD1306_CACHE
  uint8_t draw_cache_buffer[SSD1306_RAM_MIRROR_SIZE];
#endif
  uint8_t buffer[16];
  int raw_width;	// Raw display, never changes
  int raw_height;	// Raw display, never changes
  int width;	// modified by current rotation
  int height;	// modified by current rotation
  int cursor_x;
  int cursor_y;
  int textcolor;
  int textbgcolor;
  int textsize;
  int rotation;
  bool wrap;
  bool cp437;  // if set, use correct CP437 characterset (default off)
  bool show_logo;
  GFXfont *gfxFont;
};

static struct adafruit_ssd1306_data_t display_data = {
  .dev = NULL,
  .rotation = 0,
  .cursor_x = 0,
  .cursor_y = 0,
  .textsize = 1,
  .textcolor = WHITE,
  .textbgcolor = WHITE,
  .wrap = true,
  .cp437 = false,
  .gfxFont = NULL,
};

int adafruit_gfx_initialize(void) {
  int ret = 0;
  
  display_data.dev = device_get_binding(DT_LABEL(DT_INST(0, solomon_ssd1306fb)));
	if (display_data.dev == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			    DT_LABEL(DT_INST(0, solomon_ssd1306fb)));
		return -EINVAL;
	}

  struct display_capabilities caps;
  display_get_capabilities(display_data.dev, &caps);
  display_data.raw_width = caps.x_resolution;
  display_data.raw_height = caps.y_resolution;
  display_data.width = display_data.raw_width;
  display_data.height = display_data.raw_height;

  ret = adafruit_gfx_cache_init(&display_data.cache);
  if (ret != 0) {
    return ret;
  }

  uint8_t *buf = NULL;
#ifndef CONFIG_ADAFRUIT_SSD1306_CACHE
  buf = display_data.draw_cache_buffer;
#endif

  ret = adafruit_gfx_cache_source_init(&display_data.cache, &display_data.draw_cache, 0, buf);
  if (ret != 0) {
    return ret;
  }

  ret = adafruit_gfx_cache_source_init(&display_data.cache, &display_data.adafruit_logo, SSD1306_RAM_MIRROR_SIZE, 
                                adafruit_logo);
  if (ret != 0) {
    return ret;
  }
  
  adafruit_gfx_reset();
  return 0;
}

void adafruit_gfx_reset(void) {
  adafruit_gfx_clearDisplay();
  display_data.show_logo = true;
}


// startScrollRight
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
int adafruit_gfx_startScrollRight(uint8_t start, uint8_t stop)
{
  uint8_t *buf = display_data.buffer;
  size_t buflen = 0;
  
  buf[buflen++] = SSD1306_RIGHT_HORIZONTAL_SCROLL;
  buf[buflen++] = 0x00;
  buf[buflen++] = start;
  buf[buflen++] = 0x00;
  buf[buflen++] = stop;
  buf[buflen++] = 0x00;
  buf[buflen++] = 0xFF;
  buf[buflen++] = SSD1306_ACTIVATE_SCROLL;

  return ssd1306_display_write(display_data.dev, buf, buflen, true);
}

// startScrollLeft
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
int adafruit_gfx_startScrollLeft(uint8_t start, uint8_t stop)
{
  uint8_t *buf = display_data.buffer;
  size_t buflen = 0;
  
  buf[buflen++] = SSD1306_LEFT_HORIZONTAL_SCROLL;
  buf[buflen++] = 0x00;
  buf[buflen++] = start;
  buf[buflen++] = 0x00;
  buf[buflen++] = stop;
  buf[buflen++] = 0x00;
  buf[buflen++] = 0xFF;
  buf[buflen++] = SSD1306_ACTIVATE_SCROLL;

  return ssd1306_display_write(display_data.dev, buf, buflen, true);
}

// startScrollDiagRight
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
int adafruit_gfx_startScrollDiagRight(uint8_t start, uint8_t stop)
{
  uint8_t *buf = display_data.buffer;
  size_t buflen = 0;
  
  buf[buflen++] = SSD1306_SET_VERTICAL_SCROLL_AREA;
  buf[buflen++] = 0x00;
  buf[buflen++] = SSD1306_LCDHEIGHT;
  buf[buflen++] = SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL;
  buf[buflen++] = 0x00;
  buf[buflen++] = start;
  buf[buflen++] = 0x00;
  buf[buflen++] = stop;
  buf[buflen++] = 0x01;
  buf[buflen++] = SSD1306_ACTIVATE_SCROLL;

  return ssd1306_display_write(display_data.dev, buf, buflen, true);
}

// startScrollDiagLeft
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
int adafruit_gfx_startScrollDiagLeft(uint8_t start, uint8_t stop)
{
  uint8_t *buf = display_data.buffer;
  size_t buflen = 0;
  
  buf[buflen++] = SSD1306_SET_VERTICAL_SCROLL_AREA;
  buf[buflen++] = 0x00;
  buf[buflen++] = SSD1306_LCDHEIGHT;
  buf[buflen++] = SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL;
  buf[buflen++] = 0x00;
  buf[buflen++] = start;
  buf[buflen++] = 0x00;
  buf[buflen++] = stop;
  buf[buflen++] = 0x01;
  buf[buflen++] = SSD1306_ACTIVATE_SCROLL;

  return ssd1306_display_write(display_data.dev, buf, buflen, true);
}

int adafruit_gfx_stopScroll(void)
{
  uint8_t *buf = display_data.buffer;
  size_t buflen = 0;
  
  buf[buflen++] = SSD1306_DEACTIVATE_SCROLL;

  return ssd1306_display_write(display_data.dev, buf, buflen, true);
}

int adafruit_gfx_display(void) 
{
  uint8_t *buf = display_data.buffer;
  size_t buflen = 0;
  
  buf[buflen++] = SSD1306_COLUMNADDR;
  buf[buflen++] = 0;                    // Column start address (0 = reset)
  buf[buflen++] = SSD1306_LCDWIDTH - 1; // Column end address (127 = reset)
  buf[buflen++] = SSD1306_PAGEADDR;
  buf[buflen++] = 0;                    // Page start address (0 = reset)
  buf[buflen++] = (SSD1306_LCDHEIGHT >> 3) - 1; // Page end address
  int ret = ssd1306_display_write(display_data.dev, buf, buflen, true);
  
  if (ret != 0) {
    return ret;
  }

  struct adafruit_gfx_cache_source_t *source = (display_data.show_logo ? &display_data.adafruit_logo : &display_data.draw_cache);
  ret = adafruit_gfx_cache_source_choose(&display_data.cache, source);
  if (ret != 0) {
    return ret;
  }
  
  size_t line_addr;
  uint8_t *pixel_addr;
  
  for (line_addr = 0; line_addr < SSD1306_RAM_MIRROR_SIZE; line_addr += SSD1306_CACHE_LINE_SIZE) {
    /*
     * Send one cache row at a time.  This allows us to add support for external RAM
     * with a minimal on-CPU cache.
     */
    ret = adafruit_gfx_cache_get_pixel_addr(&display_data.cache, line_addr, 0, &pixel_addr); 
    if (ret != 0) {
      return ret;
    }

    ret = ssd1306_display_write(display_data.dev, pixel_addr, SSD1306_CACHE_LINE_SIZE, false);
    if (ret != 0) {
      return ret;
    }
  }

  if (display_data.show_logo) {
    adafruit_gfx_clearDisplay();
  }
  
  return 0;
}

// clear everything
void adafruit_gfx_clearDisplay(void) {
  display_data.show_logo = false;
  
  int ret = adafruit_gfx_cache_source_choose(&display_data.cache, &display_data.draw_cache);
  if (ret == 0) {
    adafruit_gfx_cache_clear_all(&display_data.cache);
  }
}

// the most basic function, set a single pixel
void adafruit_gfx_drawPixel(int x, int y, int color)
{
  if ((x < 0) || (x >= display_data.width) || (y < 0) || (y >= display_data.height))
    return;

  // check rotation, move pixel around if necessary
  switch (display_data.rotation) {
  case 1:
    _swap_int(x, y);
    x = display_data.raw_width - x - 1;
    break;
  case 2:
    x = display_data.raw_width - x - 1;
    y = display_data.raw_height - y - 1;
    break;
  case 3:
    _swap_int(x, y);
    y = display_data.raw_height - y - 1;
    break;
  }

  _draw_pixels_masked(x, y, color, SSD1306_PIXEL_MASK(y));
}


void adafruit_gfx_drawFastHLine(int x, int y, int w, int color) 
{
  int bSwap = 0;
  switch(display_data.rotation) {
    case 0:
      // 0 degree rotation, do nothing
      break;
    case 1:
      // 90 degree rotation, swap x & y for rotation, then invert x
      bSwap = 1;
      _swap_int(x, y);
      x = display_data.raw_width - x - 1;
      break;
    case 2:
      // 180 degree rotation, invert x and y - then shift y around for height.
      x = display_data.raw_width - x - 1;
      y = display_data.raw_height - y - 1;
      x -= (w-1);
      break;
    case 3:
      // 270 degree rotation, swap x & y for rotation, then invert y  and adjust y for w (not to become h)
      bSwap = 1;
      _swap_int(x, y);
      y = display_data.raw_height - y - 1;
      y -= (w-1);
      break;
  }

  if(bSwap) {
    _drawFastVLineInternal(x, y, w, color);
  } else {
    _drawFastHLineInternal(x, y, w, color);
  }
}

static void _drawFastHLineInternal(int x, int y, int w, int color) 
{
  // Do bounds/limit checks
  if (y < 0 || y >= display_data.raw_height) {
    return;
  }

  // make sure we don't try to draw below 0
  if (x < 0) {
    w += x;
    x = 0;
  }

  // make sure we don't go off the edge of the display
  if ((x + w) > display_data.raw_width) {
    w = (display_data.raw_width - x);
  }

  // if our width is now negative, punt
  if (w <= 0) {
    return;
  }

  int ret = adafruit_gfx_cache_source_choose(&display_data.cache, &display_data.draw_cache);
  if (ret != 0) {
    return;
  }
  
  _draw_pixels_masked(x, y, color, SSD1306_PIXEL_MASK(y));
}

void adafruit_gfx_drawFastVLine(int x, int y, int h, int color) {
  int bSwap = 0;
  switch(display_data.rotation) {
    case 0:
      break;
    case 1:
      // 90 degree rotation, swap x & y for rotation, then invert x and adjust x for h (now to become w)
      bSwap = 1;
      _swap_int(x, y);
      x = display_data.raw_width - x - 1;
      x -= (h-1);
      break;
    case 2:
      // 180 degree rotation, invert x and y - then shift y around for height.
      x = display_data.raw_width - x - 1;
      y = display_data.raw_height - y - 1;
      y -= (h-1);
      break;
    case 3:
      // 270 degree rotation, swap x & y for rotation, then invert y
      bSwap = 1;
      _swap_int(x, y);
      y = display_data.raw_height - y - 1;
      break;
  }

  if(bSwap) {
    _drawFastHLineInternal(x, y, h, color);
  } else {
    _drawFastVLineInternal(x, y, h, color);
  }
}


static const uint8_t premask[8] = { 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };
static const uint8_t postmask[8] = { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };


static void _drawFastVLineInternal(int x, int y, int h, int color) 
{
  int ret = 0;

  // do nothing if we're off the left or right side of the screen
  if (x < 0 || x >= display_data.raw_width) {
    return;
  }

  // make sure we don't try to draw below 0
  if (y < 0) {
    // y is negative, this will subtract enough from h to account for y being 0
    h += y;
    y = 0;
  }

  // make sure we don't go past the height of the display
  if (y + h > display_data.raw_height) {
    h = display_data.raw_height - y;
  }

  // if our height is now negative, punt
  if (h <= 0) {
    return;
  }

  // do the first partial byte, if necessary - this requires some masking
  register uint8_t mod = (y & 0x07);
  register uint8_t data;
  if (mod) {
    // mask off the high n bits we want to set
    mod = 8 - mod;

    // note - lookup table results in a nearly 10% performance improvement in
    // fill* functions
    register uint8_t mask = premask[mod];

    // adjust the mask if we're not going to reach the end of this byte
    if (h < mod) {
      mask &= (0xFF >> (mod - h));
    }

    ret = _draw_pixels_masked(x, y, color, mask);
    if (ret != 0) {
      return;
    }

    // fast exit if we're done here!
    if (h < mod) {
      return;
    }

    h -= mod;
    y += mod;
  }

  // write solid bytes while we can - effectively doing 8 rows at a time
  if (h >= 8) {
    if (color == INVERSE)  {
      // separate copy of the code so we don't impact performance of the
      // black/white write version with an extra comparison per loop
      do {
        adafruit_gfx_cache_operCache(&display_data.cache, x, y, TOGGLE_BITS, 0xFF);

        // adjust h & y (there's got to be a faster way for me to do this, but
        // this should still help a fair bit for now)
         h -= 8;
        y += 8;
      } while (h >= 8);
    } else {
      // store a local value to work with
      data = (color == WHITE) ? 0xFF : 0;
      uint8_t *addr;

      do  {
        ret = adafruit_gfx_cache_get_pixel_addr(&display_data.cache, x, y, &addr);
        if (ret != 0) {
            return;
        }
    
      	*addr = data;
      	display_data.cache.dirty = true;

        // adjust h & y (there's got to be a faster way for me to do this, but
        // this should still help a fair bit for now)
        h -= 8;
        y += 8;
      } while (h >= 8);
    }
  }

  // now do the final partial byte, if necessary
  if (h) {
    _draw_pixels_masked(x, y, color, postmask[h & 0x07]);
  }
}


static int _draw_pixels_masked(int x, int y, int color, uint8_t mask)
{
    if (!mask) {
      return 0;
    }
    
    int ret = adafruit_gfx_cache_source_choose(&display_data.cache, &display_data.draw_cache);
    if (ret != 0) {
      return ret;
    }

    switch (color)
    {
      case WHITE:
        adafruit_gfx_cache_operCache(&display_data.cache, x, y, SET_BITS, mask);
        break;
      case BLACK:
        adafruit_gfx_cache_operCache(&display_data.cache, x, y, CLEAR_BITS, mask);
        break;
      case INVERSE:
        adafruit_gfx_cache_operCache(&display_data.cache, x, y, TOGGLE_BITS, mask);
        break;
      default:
        return -EINVAL;
    }
    
    return 0;
}


// From Adafruit_GFX base class, ported to PSoC with FreeRTOS (and C)


// Draw a circle outline
void adafruit_gfx_drawCircle(int x0, int y0, int r, int color) 
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int x = 0;
  int y = r;

  adafruit_gfx_drawPixel(x0  , y0+r, color);
  adafruit_gfx_drawPixel(x0  , y0-r, color);
  adafruit_gfx_drawPixel(x0+r, y0  , color);
  adafruit_gfx_drawPixel(x0-r, y0  , color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    adafruit_gfx_drawPixel(x0 + x, y0 + y, color);
    adafruit_gfx_drawPixel(x0 - x, y0 + y, color);
    adafruit_gfx_drawPixel(x0 + x, y0 - y, color);
    adafruit_gfx_drawPixel(x0 - x, y0 - y, color);
    adafruit_gfx_drawPixel(x0 + y, y0 + x, color);
    adafruit_gfx_drawPixel(x0 - y, y0 + x, color);
    adafruit_gfx_drawPixel(x0 + y, y0 - x, color);
    adafruit_gfx_drawPixel(x0 - y, y0 - x, color);
  }
}

void adafruit_gfx_drawCircleHelper( int x0, int y0,
 int r, uint8_t cornername, int color) {
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int x     = 0;
  int y     = r;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if (cornername & 0x4) {
      adafruit_gfx_drawPixel(x0 + x, y0 + y, color);
      adafruit_gfx_drawPixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      adafruit_gfx_drawPixel(x0 + x, y0 - y, color);
      adafruit_gfx_drawPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      adafruit_gfx_drawPixel(x0 - y, y0 + x, color);
      adafruit_gfx_drawPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      adafruit_gfx_drawPixel(x0 - y, y0 - x, color);
      adafruit_gfx_drawPixel(x0 - x, y0 - y, color);
    }
  }
}

void adafruit_gfx_fillCircle(int x0, int y0, int r,
 int color) {
  adafruit_gfx_drawFastVLine(x0, y0-r, 2*r+1, color);
  adafruit_gfx_fillCircleHelper(x0, y0, r, 3, 0, color);
}

// Used to do circles and roundrects
void adafruit_gfx_fillCircleHelper(int x0, int y0, int r,
     uint8_t cornername, int delta, int color) {

  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int x     = 0;
  int y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;

    if (cornername & 0x1) {
      adafruit_gfx_drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
      adafruit_gfx_drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
    }
    if (cornername & 0x2) {
      adafruit_gfx_drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
      adafruit_gfx_drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
    }
  }
}

// Bresenham's algorithm - thx wikpedia
void adafruit_gfx_drawLine(int x0, int y0, int x1, int y1, int color) {
  int16_t steep = _abs(y1 - y0) > _abs(x1 - x0);
  if (steep) {
    _swap_int(x0, y0);
    _swap_int(x1, y1);
  }

  if (x0 > x1) {
    _swap_int(x0, x1);
    _swap_int(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = _abs(y1 - y0);

  int16_t err = dx / 2;
  int ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0 <= x1; x0++) {
    if (steep) {
      adafruit_gfx_drawPixel(y0, x0, color);
    } else {
      adafruit_gfx_drawPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

// Draw a rectangle
void adafruit_gfx_drawRect(int x, int y, int w, int h, int color) {
  adafruit_gfx_drawFastHLine(x, y, w, color);
  adafruit_gfx_drawFastHLine(x, y+h-1, w, color);
  adafruit_gfx_drawFastVLine(x, y, h, color);
  adafruit_gfx_drawFastVLine(x+w-1, y, h, color);
}

void adafruit_gfx_fillRect(int x, int y, int w, int h, int color) {
  // Update in subclasses if desired!
  for (int16_t i = x; i < x + w; i++) {
    adafruit_gfx_drawFastVLine(i, y, h, color);
  }
}

void adafruit_gfx_fillScreen(int color) {
  adafruit_gfx_fillRect(0, 0, display_data.width, display_data.height, color);
}

// Draw a rounded rectangle
void adafruit_gfx_drawRoundRect(int x, int y, int w, int h, int r, int color) {
  // smarter version
  adafruit_gfx_drawFastHLine(x+r  , y    , w-2*r, color); // Top
  adafruit_gfx_drawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
  adafruit_gfx_drawFastVLine(x    , y+r  , h-2*r, color); // Left
  adafruit_gfx_drawFastVLine(x+w-1, y+r  , h-2*r, color); // Right
  // draw four corners
  adafruit_gfx_drawCircleHelper(x+r    , y+r    , r, 1, color);
  adafruit_gfx_drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
  adafruit_gfx_drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
  adafruit_gfx_drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
}

// Fill a rounded rectangle
void adafruit_gfx_fillRoundRect(int x, int y, int w,
 int h, int r, int color) {
  // smarter version
  adafruit_gfx_fillRect(x+r, y, w-2*r, h, color);

  // draw four corners
  adafruit_gfx_fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
  adafruit_gfx_fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
}

// Draw a triangle
void adafruit_gfx_drawTriangle(int x0, int y0,
 int x1, int y1, int x2, int y2, int color) {
  adafruit_gfx_drawLine(x0, y0, x1, y1, color);
  adafruit_gfx_drawLine(x1, y1, x2, y2, color);
  adafruit_gfx_drawLine(x2, y2, x0, y0, color);
}

// Fill a triangle
void adafruit_gfx_fillTriangle(int x0, int y0,
 int x1, int y1, int x2, int y2, int color) {

  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    _swap_int(y0, y1); _swap_int(x0, x1);
  } 
  if (y1 > y2) {
    _swap_int(y2, y1); _swap_int(x2, x1);
  }
  if (y0 > y1) {
    _swap_int(y0, y1); _swap_int(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
    adafruit_gfx_drawFastHLine(a, y0, b-a+1, color);
    return;
  }

  int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
  int32_t
    sa   = 0,
    sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2) last = y1;   // Include y1 scanline
  else         last = y1-1; // Skip it

  for(y=y0; y<=last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) _swap_int(a,b);
    adafruit_gfx_drawFastHLine(a, y, b-a+1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y<=y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) _swap_int(a,b);
    adafruit_gfx_drawFastHLine(a, y, b-a+1, color);
  }
}

// Draw a 1-bit image (bitmap) at the specified (x,y) position from the
// provided bitmap buffer using the specified foreground (for set bits)
// and background (for clear bits) colors.
// If foreground and background are the same, unset bits are transparent
void adafruit_gfx_drawBitmap(int x, int y, uint8_t *bitmap, int w, int h, int color, int bg) 
{
  int i, j, byteWidth = (w + 7) / 8;
  uint8_t byte = 0;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(i & 7) {
        byte <<= 1;
      } else {
        byte = bitmap[j * byteWidth + i / 8];
      }
      
      if(byte & 0x80) {
        adafruit_gfx_drawPixel(x+i, y+j, color);
      } else if(color != bg) {
        adafruit_gfx_drawPixel(x+i, y+j, bg);
      }
    }
  }
}

//Draw XBitMap Files (*.xbm), exported from GIMP,
//Usage: Export from GIMP to *.xbm, rename *.xbm to *.c and open in editor.
//C Array can be directly used with this function
void adafruit_gfx_drawXBitmap(int x, int y,
 const uint8_t *bitmap, int w, int h, int color) {

  int i, j, byteWidth = (w + 7) / 8;
  uint8_t byte = 0;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(i & 7) {
        byte >>= 1;
      } else {
        byte = bitmap[j * byteWidth + i / 8];
      }
      
      if(byte & 0x01) {
        adafruit_gfx_drawPixel(x+i, y+j, color);
      }
    }
  }
}

size_t adafruit_gfx_write(uint8_t c) {
  GFXfont *font = display_data.gfxFont;
  
  if(!font) { // 'Classic' built-in font
    font = (GFXfont *)&adafruit_gfx_default_font;
  }
  
  if(c == '\n') {
    display_data.cursor_x  = 0;
    display_data.cursor_y += display_data.textsize * display_data.gfxFont->yAdvance;
  } else if (c == '\r') {
    return 0;
  } else {
    uint8_t first = display_data.gfxFont->first;
    if((c < first) || (c > display_data.gfxFont->last)) {
      return 0;
    }
    
    uint8_t c2 = c - display_data.gfxFont->first;
    GFXglyph *glyph = display_data.gfxFont->fixed_glyph;
    if (!glyph) {
      glyph = &(display_data.gfxFont->glyph[c2]);
    }
    
    int w = glyph->width;
    int h = glyph->height;
    
    if((w > 0) && (h > 0)) { // Is there an associated bitmap?
      int xo = glyph->xOffset;
      if(display_data.wrap && ((display_data.cursor_x + display_data.textsize * (xo + w)) >= display_data.width)) {
        // Drawing character would go off right edge; wrap to new line
        display_data.cursor_x = 0;
        display_data.cursor_y += display_data.textsize * display_data.gfxFont->yAdvance;
      }
      adafruit_gfx_drawChar(display_data.cursor_x, display_data.cursor_y, c, 
          display_data.textcolor, display_data.textbgcolor, display_data.textsize);
    }
    display_data.cursor_x += glyph->xAdvance * display_data.textsize;
  }

  return 1;
}

// Draw a character
void adafruit_gfx_drawChar(int x, int y, unsigned char c, int color, int bg, int size) {
  GFXfont *font = display_data.gfxFont;
  
  if(!font) { // 'Classic' built-in font
    font = (GFXfont *)&adafruit_gfx_default_font;

    if((x >= display_data.width)   || // Clip right
       (y >= display_data.height)  || // Clip bottom
       ((x + 6 * size - 1) < 0)   || // Clip left
       ((y + 8 * size - 1) < 0))     // Clip top
      return;

    if(!display_data.cp437 && (c >= 176)) {
      c++; // Handle 'classic' charset behavior
    }

    for (int i = 0; i < 6; i++) {
      int line = 0x00;
      
      if (i < 5) {
        line = font->bitmap[(c*5)+i];
      }

      for(int j = 0; j < 8; j++, line >>= 1) {
        if(line & 0x1) {
          if(size == 1) {
            adafruit_gfx_drawPixel(x+i, y+j, color);
          } else {
            adafruit_gfx_fillRect(x+(i*size), y+(j*size), size, size, color);
          }
        } else if(bg != color) {
          if(size == 1) {
            adafruit_gfx_drawPixel(x+i, y+j, bg);
          } else {
            adafruit_gfx_fillRect(x+i*size, y+j*size, size, size, bg);
          }
        }
      }
    }
  } else { // Custom font

    // Character is assumed previously filtered by write() to eliminate
    // newlines, returns, non-printable characters, etc.  Calling drawChar()
    // directly with 'bad' characters of font may cause mayhem!

    c -= display_data.gfxFont->first;
    GFXglyph *glyph  = &(display_data.gfxFont->glyph[c]);
    uint8_t  *bitmap = display_data.gfxFont->bitmap;

    int bo = glyph->bitmapOffset;
    int w = glyph->width;
    int h = glyph->height;
    
    int xo = glyph->xOffset;
    int yo = glyph->yOffset;
    
    int xx;
    int yy;
    int bits = 0;
    int bit = 0;
    int xo16 = 0;
    int yo16 = 0;

    if(size > 1) {
      xo16 = xo;
      yo16 = yo;
    }

    // Todo: Add character clipping here

    // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
    // THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
    // has typically been used with the 'classic' font to overwrite old
    // screen contents with new data.  This ONLY works because the
    // characters are a uniform size; it's not a sensible thing to do with
    // proportionally-spaced fonts with glyphs of varying sizes (and that
    // may overlap).  To replace previously-drawn text when using a custom
    // font, use the getTextBounds() function to determine the smallest
    // rectangle encompassing a string, erase the area with fillRect(),
    // then draw new text.  This WILL infortunately 'blink' the text, but
    // is unavoidable.  Drawing 'background' pixels will NOT fix this,
    // only creates a new set of problems.  Have an idea to work around
    // this (a canvas object type for MCUs that can afford the RAM and
    // displays supporting setAddrWindow() and pushColors()), but haven't
    // implemented this yet.

    for(yy=0; yy<h; yy++) {
      for(xx=0; xx<w; xx++) {
        if(!(bit++ & 0x07)) {
          bits = bitmap[bo++];
        }
        
        if(bits & 0x80) {
          if(size == 1) {
            adafruit_gfx_drawPixel(x+xo+xx, y+yo+yy, color);
          } else {
            adafruit_gfx_fillRect(x+(xo16+xx)*size, y+(yo16+yy)*size, size, size, color);
          }
        }
        bits <<= 1;
      }
    }
  } // End classic vs custom font
}

void adafruit_gfx_setCursor(int x, int y) {
  display_data.cursor_x = x;
  display_data.cursor_y = y;
}

int adafruit_gfx_getCursorX(void) {
  return display_data.cursor_x;
}

int adafruit_gfx_getCursorY(void) {
  return display_data.cursor_y;
}

void adafruit_gfx_setTextSize(int ts) {
  display_data.textsize = min(ts, 1);
}

void adafruit_gfx_setTextColor(int c, int b) {
  // For 'transparent' background, we'll set the bg
  // to the same as fg instead of using a flag
  display_data.textcolor   = c;
  display_data.textbgcolor = b;
}

void adafruit_gfx_setTextWrap(bool w) {
  display_data.wrap = w;
}

int adafruit_gfx_getRotation(void) {
  return display_data.rotation;
}

void adafruit_gfx_setRotation(int x) {
  display_data.rotation = (x & 0x03);
  switch(display_data.rotation) {
   case 0:
   case 2:
    display_data.width  = display_data.raw_width;
    display_data.height = display_data.raw_height;
    break;
   case 1:
   case 3:
    display_data.width  = display_data.raw_height;
    display_data.height = display_data.raw_width;
    break;
  }
}

// Enable (or disable) Code Page 437-compatible charset.
// There was an error in glcdfont.c for the longest time -- one character
// (#176, the 'light shade' block) was missing -- this threw off the index
// of every character that followed it.  But a TON of code has been written
// with the erroneous character indices.  By default, the library uses the
// original 'wrong' behavior and old sketches will still work.  Pass 'true'
// to this function to use correct CP437 character values in your code.
void adafruit_gfx_cp437(bool x) {
  display_data.cp437 = x;
}

void adafruit_gfx_setFont(const GFXfont *f) {
  if(f) {          // Font struct pointer passed in?
    if(!display_data.gfxFont) { // And no current font struct?
      // Switching from classic to new font behavior.
      // Move cursor pos down 6 pixels so it's on baseline.
      display_data.cursor_y += 6;
    }
  } else if(display_data.gfxFont) { // NULL passed.  Current font struct defined?
    // Switching from new to classic font behavior.
    // Move cursor pos up 6 pixels so it's at top-left of char.
    display_data.cursor_y -= 6;
  }
  display_data.gfxFont = (GFXfont *)f;
}

// Pass string and a cursor position, returns UL corner and W, H.
void adafruit_gfx_getTextBounds(char *str, int x, int y, int ts,
                                int *x1, int *y1, int *w, int *h) {
  GFXfont *font = display_data.gfxFont;
  
  if(!font) { // 'Classic' built-in font
    font = (GFXfont *)&adafruit_gfx_default_font;
  }

  ts = min(ts, 1);

  GFXglyph *fixed_glyph = font->fixed_glyph;
  GFXglyph *glyph;
  uint8_t c; // Current character
  
  int minx = display_data.width;
  int miny = display_data.height;
  int maxx = -1;
  int maxy = -1;
  
  int x_ul;
  int y_ul;
  int x_lr;
  int y_lr;

  while((c = *str++)) {
    if (c == '\n') { // newline
      x  = 0;  // Reset x
      y += ts * display_data.gfxFont->yAdvance; // Advance y by 1 line
    } else if (c == '\r') {
      continue;
    } else if((c < font->first) || (c > font->last)) { // Char not present in current font
      continue;
    } else {
      c -= font->first;
      glyph = fixed_glyph;
      if (!glyph) {
        glyph = &(display_data.gfxFont->glyph[c]);
      }

      if(display_data.wrap && ((x + ((glyph->xOffset + glyph->width) * ts)) >= display_data.width)) {
        // Line wrap
        x = 0;  // Reset x to 0
        y += ts * display_data.gfxFont->yAdvance; // Advance y by 1 line
      }

      x_ul = x + glyph->xOffset * ts;
      y_ul = y + glyph->yOffset * ts;
      x_lr = x_ul + glyph->width * ts - 1;
      y_lr = y_ul + glyph->height * ts - 1;

      x += glyph->xAdvance * ts;
      
      minx = min(minx, x_ul);
      miny = min(miny, y_ul);
      maxx = max(maxx, x_lr);
      maxy = max(maxy, y_lr);
    }
  }

  if (x1) {
    *x1 = minx;
  }
  
  if (y1) {
    *y1 = miny;
  }
  
  if (w) {
    if(maxx >= minx) {
      *w = maxx - minx + 1;
    } else {
      *w = 0;
    }
  }
  
  if (h) {
    if(maxy >= miny) {
      *h = maxy - miny + 1;
    } else {
      *h = 0;
    }
  }
}

// Return the size of the display (per current rotation)
int adafruit_gfx_width(void) {
  return display_data.width;
}

int adafruit_gfx_height(void) {
  return display_data.height;
}


