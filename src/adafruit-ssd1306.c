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

/*
 * Since the SSD1306 driver has no export of the bus, and we need to write 
 * commands that the driver doesn't support directly, we will do the bus lookup
 * ourselves, in the same way they do
 */
#define DT_DRV_COMPAT solomon_ssd1306fb

#include <logging/log.h>
LOG_MODULE_REGISTER(adafruit_ssd1306, CONFIG_DISPLAY_LOG_LEVEL);

#include <zephyr.h>
#include <string.h>
#include <device.h>
#include <drivers/i2c.h>
#include <drivers/display.h>

#include "adafruit-ssd1306.h"
#include "utils.h"

#define draw_pixel(x, y) (cache_pixel(display_data.draw_cache, (x), (y)))
#define cache_pixel(cache, x, y) ((cache)[SSD1306_PIXEL_ADDR((x), (y))])

static void _drawFastVLineInternal(int16_t x, int16_t y, int16_t h, uint16_t color);
static void _drawFastHLineInternal(int16_t x, int16_t y, int16_t w, uint16_t color);
static void _ssd1306_command(uint8_t c);
static void _operCache(int16_t x, int16_t y, oper_t oper_, uint8_t mask);

struct adafruit_ssd1306_data_t {
  const struct device *dev;
  const struct device *bus;
  uint8_t i2c_addr;
  int8_t  vccstate;
  uint8_t draw_cache[SSD1306_RAM_MIRROR_SIZE];
  bool show_logo;
  int16_t raw_width;	// Raw display, never changes
  int16_t raw_height;	// Raw display, never changes
  int16_t width;	// modified by current rotation
  int16_t height;	// modified by current rotation
  int16_t cursor_x;
  int16_t cursor_y;
  uint16_t textcolor;
  uint16_t textbgcolor;
  uint8_t textsize;
  uint8_t rotation;
  bool wrap;
  bool cp437;  // if set, use correct CP437 characterset (default off)
  GFXfont *gfxFont;
};

static struct adafruit_ssd1306_data_t display_data = {
  .dev = NULL,
  .vccstate = SSD1306_SWITCHCAPVCC,
  .rotation = 0,
  .cursor_x = 0,
  .cursor_y = 0,
  .textsize = 1,
  .textcolor = 0xFFFF,
  .textbgcolor = 0xFFFF,
  .wrap = true,
  .cp437 = false,
  .gfxFont = NULL,
};

#if !DT_INST_ON_BUS(0, i2c)
#error This does not support SPI-connected SSD1306 at this time
#endif

int SSD1306_initialize(uint8_t vccstate) {
  display_data.vccstate = vccstate;

	display_data.bus = device_get_binding(DT_INST_BUS_LABEL(0));
	if (display_data.bus == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			    DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

  display_data.dev = device_get_binding(DT_LABEL(DT_INST(0, DT_DRV_COMPAT)));
	if (display_data.dev == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			    DT_LABEL(DT_INST(0, DT_DRV_COMPAT)));
		return -EINVAL;
	}

  display_data.i2c_addr = DT_INST_REG_ADDR(0);
  
  struct display_capabilities caps;
  display_get_capabilities(display_data.dev, &caps);
  display_data.raw_width = caps.x_resolution;
  display_data.raw_height = caps.y_resolution;
  display_data.width = display_data.raw_width;
  display_data.height = display_data.raw_height;

  SSD1306_reset();
  return 0;
}

void SSD1306_reset(void) {
  SSD1306_clearDisplay();
  display_data.show_logo = true;
}

void SSD1306_displayOff(void)
{
  _ssd1306_command(SSD1306_DISPLAYOFF);            // 0xAE
}

void SSD1306_displayOn(void)
{
  _ssd1306_command(SSD1306_DISPLAYON);             //--turn on oled panel
}

static void _operCache(int16_t x, int16_t y, oper_t oper_, uint8_t mask)
{
    uint8_t *addr = &draw_pixel(x, y);
    uint8_t data = *addr;

    switch (oper_) {
        case SET_BITS:
            data |= mask;
            break;
        case CLEAR_BITS:
            data &= ~mask;
            break;
        case TOGGLE_BITS:
            data ^= mask;
            break;
        default:
            break;
    }

    *addr = data;
}

static void _ssd1306_command(uint8_t c) {
  // I2C message
  uint8_t control = 0x00;    // Co = 0, D/C = 0
	int ret = i2c_reg_write_byte(display_data.bus, display_data.i2c_addr,
	                             control, c);

	ARG_UNUSED(ret);
}

// startScrolLright
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void SSD1306_startScrollRight(uint8_t start, uint8_t stop){
  _ssd1306_command(SSD1306_RIGHT_HORIZONTAL_SCROLL);
  _ssd1306_command(0x00);
  _ssd1306_command(start);
  _ssd1306_command(0x00);
  _ssd1306_command(stop);
  _ssd1306_command(0x00);
  _ssd1306_command(0xFF);
  _ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

// startScrollLeft
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void SSD1306_startScrollLeft(uint8_t start, uint8_t stop){
  _ssd1306_command(SSD1306_LEFT_HORIZONTAL_SCROLL);
  _ssd1306_command(0x00);
  _ssd1306_command(start);
  _ssd1306_command(0x00);
  _ssd1306_command(stop);
  _ssd1306_command(0x00);
  _ssd1306_command(0xFF);
  _ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

// startScrollDiagRight
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void SSD1306_startScrollDiagRight(uint8_t start, uint8_t stop){
  _ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);
  _ssd1306_command(0x00);
  _ssd1306_command(SSD1306_LCDHEIGHT);
  _ssd1306_command(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
  _ssd1306_command(0x00);
  _ssd1306_command(start);
  _ssd1306_command(0x00);
  _ssd1306_command(stop);
  _ssd1306_command(0x01);
  _ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

// startScrollDiagLeft
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void SSD1306_startScrollDiagLeft(uint8_t start, uint8_t stop){
  _ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);
  _ssd1306_command(0x00);
  _ssd1306_command(SSD1306_LCDHEIGHT);
  _ssd1306_command(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
  _ssd1306_command(0x00);
  _ssd1306_command(start);
  _ssd1306_command(0x00);
  _ssd1306_command(stop);
  _ssd1306_command(0x01);
  _ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

void SSD1306_stopScroll(void){
  _ssd1306_command(SSD1306_DEACTIVATE_SCROLL);
}

// Dim the display
// dim = true: display is dimmed
// dim = false: display is normal
void SSD1306_dim(bool dim) {
  uint8_t contrast;

  if (dim) {
    contrast = 0; // Dimmed display
  } else {
    if (display_data.vccstate == SSD1306_EXTERNALVCC) {
      contrast = 0x9F;
    } else {
      contrast = 0xCF;
    }
  }
  
  // the range of contrast to too small to be really useful
  // it is useful to dim the display
  _ssd1306_command(SSD1306_SETCONTRAST);
  _ssd1306_command(contrast);
}

void SSD1306_display(void) {
  _ssd1306_command(SSD1306_COLUMNADDR);
  _ssd1306_command(0);                  // Column start address (0 = reset)
  _ssd1306_command(SSD1306_LCDWIDTH-1); // Column end address (127 = reset)

  _ssd1306_command(SSD1306_PAGEADDR);
  _ssd1306_command(0);                  // Page start address (0 = reset)
  _ssd1306_command((SSD1306_LCDHEIGHT >> 3) - 1); // Page end address

  uint8_t *cache = (display_data.show_logo ? (uint8_t *)lcd_logo : display_data.draw_cache);

  for (int16_t y = 0; y < SSD1306_LCDHEIGHT; y += 8) {
    for (uint8_t x = 0; x < SSD1306_LCDWIDTH; x += 16) {
      // Co = 0, D/C = 1
      int ret = i2c_burst_write(display_data.bus, display_data.i2c_addr,
                                0x40, &cache_pixel(cache, x, y), 16);
      ARG_UNUSED(ret);
    }
  }

  if (display_data.show_logo) {
    SSD1306_clearDisplay();
  }
}

// clear everything
void SSD1306_clearDisplay(void) {
  display_data.show_logo = false;
  memset(display_data.draw_cache, 0, SSD1306_RAM_MIRROR_SIZE);
}

// the most basic function, set a single pixel
void SSD1306_drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= display_data.width) || (y < 0) || (y >= display_data.height))
    return;

  // check rotation, move pixel around if necessary
  switch (display_data.rotation) {
  case 1:
    _swap_int16(x, y);
    x = display_data.raw_width - x - 1;
    break;
  case 2:
    x = display_data.raw_width - x - 1;
    y = display_data.raw_height - y - 1;
    break;
  case 3:
    _swap_int16(x, y);
    y = display_data.raw_height - y - 1;
    break;
  }

  // x is which column
  uint8_t mask = (1 << (y & 0x07));
  
  switch (color)
  {
    case WHITE:   
      _operCache(x, y, SET_BITS, mask);  
      break;
    case BLACK:   
      _operCache(x, y, CLEAR_BITS, mask);  
      break;
    case INVERSE: 
      _operCache(x, y, TOGGLE_BITS, mask);  
      break;
    default:
      return;
  }
}


void SSD1306_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  int bSwap = 0;
  switch(display_data.rotation) {
    case 0:
      // 0 degree rotation, do nothing
      break;
    case 1:
      // 90 degree rotation, swap x & y for rotation, then invert x
      bSwap = 1;
      _swap_int16(x, y);
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
      _swap_int16(x, y);
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

static void _drawFastHLineInternal(int16_t x, int16_t y, int16_t w, uint16_t color) {
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

  register uint8_t mask = SSD1306_PIXEL_MASK(y);
  for (uint8_t i = x; i < x + w; i++) {
    switch (color)
    {
      case WHITE:
        _operCache(i, y, SET_BITS, mask);
        break;
      case BLACK:
        _operCache(i, y, CLEAR_BITS, mask);
        break;
      case INVERSE:
        _operCache(i, y, TOGGLE_BITS, mask);
        break;
      default:
        return;
    }
  }
}

void SSD1306_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  int bSwap = 0;
  switch(display_data.rotation) {
    case 0:
      break;
    case 1:
      // 90 degree rotation, swap x & y for rotation, then invert x and adjust x for h (now to become w)
      bSwap = 1;
      _swap_int16(x, y);
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
      _swap_int16(x, y);
      y = display_data.raw_height - y - 1;
      break;
  }

  if(bSwap) {
    _drawFastHLineInternal(x, y, h, color);
  } else {
    _drawFastVLineInternal(x, y, h, color);
  }
}


static void _drawFastVLineInternal(int16_t x, int16_t __y, int16_t __h, uint16_t color) {

  // do nothing if we're off the left or right side of the screen
  if (x < 0 || x >= display_data.raw_width) {
    return;
  }

  // make sure we don't try to draw below 0
  if (__y < 0) {
    // __y is negative, this will subtract enough from __h to account for __y being 0
    __h += __y;
    __y = 0;
  }

  // make sure we don't go past the height of the display
  if ((__y + __h) > display_data.raw_height) {
    __h = (display_data.raw_height - __y);
  }

  // if our height is now negative, punt
  if (__h <= 0) {
    return;
  }

  // this display doesn't need ints for coordinates, use local byte registers
  // for faster juggling
  register uint8_t y = __y;
  register uint8_t h = __h;

  // do the first partial byte, if necessary - this requires some masking
  register uint8_t mod = (y & 0x07);
  register uint8_t data;
  if (mod) {
    // mask off the high n bits we want to set
    mod = 8 - mod;

    // note - lookup table results in a nearly 10% performance improvement in
    // fill* functions
    // register uint8_t mask = ~(0xFF >> (mod));
    static uint8_t premask[8] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };
    register uint8_t mask = premask[mod];

    // adjust the mask if we're not going to reach the end of this byte
    if (h < mod) {
      mask &= (0xFF >> (mod-h));
    }

    switch (color)
    {
      case WHITE:
        _operCache(x, y, SET_BITS, mask);
        break;
      case BLACK:
        _operCache(x, y, CLEAR_BITS, mask);
        break;
      case INVERSE:
        _operCache(x, y, TOGGLE_BITS, mask);
        break;
      default:
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
        _operCache(x, y, TOGGLE_BITS, 0xFF);

        // adjust h & y (there's got to be a faster way for me to do this, but
        // this should still help a fair bit for now)
        h -= 8;
        y += 8;
      } while (h >= 8);
    } else {
      // store a local value to work with
      data = (color == WHITE) ? 0xFF : 0;

      do  {
      	draw_pixel(x, y) = data;

        // adjust h & y (there's got to be a faster way for me to do this, but
        // this should still help a fair bit for now)
        h -= 8;
        y += 8;
      } while (h >= 8);
    }
  }

  // now do the final partial byte, if necessary
  if (h) {
    mod = h & 0x07;
    // this time we want to mask the low bits of the byte, vs the high bits we
    // did above
    // register uint8_t mask = (1 << mod) - 1;
    // note - lookup table results in a nearly 10% performance improvement in
    // fill* functions
    static uint8_t postmask[8] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F,
                                  0x7F};
    register uint8_t mask = postmask[mod];

    switch (color)
    {
      case WHITE:
        _operCache(x, y, SET_BITS, mask);
        break;
      case BLACK:
        _operCache(x, y, CLEAR_BITS, mask);
        break;
      case INVERSE:
        _operCache(x, y, TOGGLE_BITS, mask);
        break;
    }
  }
}

// From Adafruit_GFX base class, ported to PSoC with FreeRTOS (and C)


// Draw a circle outline
void SSD1306_drawCircle(int16_t x0, int16_t y0, int16_t r,
 uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  SSD1306_drawPixel(x0  , y0+r, color);
  SSD1306_drawPixel(x0  , y0-r, color);
  SSD1306_drawPixel(x0+r, y0  , color);
  SSD1306_drawPixel(x0-r, y0  , color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    SSD1306_drawPixel(x0 + x, y0 + y, color);
    SSD1306_drawPixel(x0 - x, y0 + y, color);
    SSD1306_drawPixel(x0 + x, y0 - y, color);
    SSD1306_drawPixel(x0 - x, y0 - y, color);
    SSD1306_drawPixel(x0 + y, y0 + x, color);
    SSD1306_drawPixel(x0 - y, y0 + x, color);
    SSD1306_drawPixel(x0 + y, y0 - x, color);
    SSD1306_drawPixel(x0 - y, y0 - x, color);
  }
}

void SSD1306_drawCircleHelper( int16_t x0, int16_t y0,
 int16_t r, uint8_t cornername, uint16_t color) {
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if (cornername & 0x4) {
      SSD1306_drawPixel(x0 + x, y0 + y, color);
      SSD1306_drawPixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      SSD1306_drawPixel(x0 + x, y0 - y, color);
      SSD1306_drawPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      SSD1306_drawPixel(x0 - y, y0 + x, color);
      SSD1306_drawPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      SSD1306_drawPixel(x0 - y, y0 - x, color);
      SSD1306_drawPixel(x0 - x, y0 - y, color);
    }
  }
}

void SSD1306_fillCircle(int16_t x0, int16_t y0, int16_t r,
 uint16_t color) {
  SSD1306_drawFastVLine(x0, y0-r, 2*r+1, color);
  SSD1306_fillCircleHelper(x0, y0, r, 3, 0, color);
}

// Used to do circles and roundrects
void SSD1306_fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
 uint8_t cornername, int16_t delta, uint16_t color) {

  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

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
      SSD1306_drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
      SSD1306_drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
    }
    if (cornername & 0x2) {
      SSD1306_drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
      SSD1306_drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
    }
  }
}

// Bresenham's algorithm - thx wikpedia
void SSD1306_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
 uint16_t color) {
  int16_t steep = _abs(y1 - y0) > _abs(x1 - x0);
  if (steep) {
    _swap_int16(x0, y0);
    _swap_int16(x1, y1);
  }

  if (x0 > x1) {
    _swap_int16(x0, x1);
    _swap_int16(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = _abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0<=x1; x0++) {
    if (steep) {
      SSD1306_drawPixel(y0, x0, color);
    } else {
      SSD1306_drawPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

// Draw a rectangle
void SSD1306_drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
 uint16_t color) {
  SSD1306_drawFastHLine(x, y, w, color);
  SSD1306_drawFastHLine(x, y+h-1, w, color);
  SSD1306_drawFastVLine(x, y, h, color);
  SSD1306_drawFastVLine(x+w-1, y, h, color);
}

void SSD1306_fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
 uint16_t color) {
  // Update in subclasses if desired!
  for (int16_t i=x; i<x+w; i++) {
    SSD1306_drawFastVLine(i, y, h, color);
  }
}

void SSD1306_fillScreen(uint16_t color) {
  SSD1306_fillRect(0, 0, display_data.width, display_data.height, color);
}

// Draw a rounded rectangle
void SSD1306_drawRoundRect(int16_t x, int16_t y, int16_t w,
 int16_t h, int16_t r, uint16_t color) {
  // smarter version
  SSD1306_drawFastHLine(x+r  , y    , w-2*r, color); // Top
  SSD1306_drawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
  SSD1306_drawFastVLine(x    , y+r  , h-2*r, color); // Left
  SSD1306_drawFastVLine(x+w-1, y+r  , h-2*r, color); // Right
  // draw four corners
  SSD1306_drawCircleHelper(x+r    , y+r    , r, 1, color);
  SSD1306_drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
  SSD1306_drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
  SSD1306_drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
}

// Fill a rounded rectangle
void SSD1306_fillRoundRect(int16_t x, int16_t y, int16_t w,
 int16_t h, int16_t r, uint16_t color) {
  // smarter version
  SSD1306_fillRect(x+r, y, w-2*r, h, color);

  // draw four corners
  SSD1306_fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
  SSD1306_fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
}

// Draw a triangle
void SSD1306_drawTriangle(int16_t x0, int16_t y0,
 int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
  SSD1306_drawLine(x0, y0, x1, y1, color);
  SSD1306_drawLine(x1, y1, x2, y2, color);
  SSD1306_drawLine(x2, y2, x0, y0, color);
}

// Fill a triangle
void SSD1306_fillTriangle(int16_t x0, int16_t y0,
 int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {

  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    _swap_int16(y0, y1); _swap_int16(x0, x1);
  }
  if (y1 > y2) {
    _swap_int16(y2, y1); _swap_int16(x2, x1);
  }
  if (y0 > y1) {
    _swap_int16(y0, y1); _swap_int16(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
    SSD1306_drawFastHLine(a, y0, b-a+1, color);
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
    if(a > b) _swap_int16(a,b);
    SSD1306_drawFastHLine(a, y, b-a+1, color);
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
    if(a > b) _swap_int16(a,b);
    SSD1306_drawFastHLine(a, y, b-a+1, color);
  }
}

// Draw a 1-bit image (bitmap) at the specified (x,y) position from the
// provided bitmap buffer using the specified foreground (for set bits)
// and background (for clear bits) colors.
// If foreground and background are the same, unset bits are transparent
void SSD1306_drawBitmap(int16_t x, int16_t y, uint8_t *bitmap,
      int16_t w, int16_t h, uint16_t color, uint16_t bg) {

  int16_t i, j, byteWidth = (w + 7) / 8;
  uint8_t byte = 0;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(i & 7) byte <<= 1;
      else      byte   = bitmap[j * byteWidth + i / 8];
      if(byte & 0x80) SSD1306_drawPixel(x+i, y+j, color);
      else if(color != bg) SSD1306_drawPixel(x+i, y+j, bg);
    }
  }
}

//Draw XBitMap Files (*.xbm), exported from GIMP,
//Usage: Export from GIMP to *.xbm, rename *.xbm to *.c and open in editor.
//C Array can be directly used with this function
void SSD1306_drawXBitmap(int16_t x, int16_t y,
 const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {

  int16_t i, j, byteWidth = (w + 7) / 8;
  uint8_t byte = 0;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(i & 7) byte >>= 1;
      else      byte   = bitmap[j * byteWidth + i / 8];
      if(byte & 0x01) SSD1306_drawPixel(x+i, y+j, color);
    }
  }
}

size_t SSD1306_write(uint8_t c) {
  if(!display_data.gfxFont) { // 'Classic' built-in font

    if(c == '\n') {
      display_data.cursor_y += display_data.textsize*8;
      display_data.cursor_x  = 0;
    } else if(c == '\r') {
      // skip em
    } else {
      if(display_data.wrap && ((display_data.cursor_x + display_data.textsize * 6) >= display_data.width)) { // Heading off edge?
        display_data.cursor_x  = 0;            // Reset x to zero
        display_data.cursor_y += display_data.textsize * 8; // Advance y one line
      }
      SSD1306_drawChar(display_data.cursor_x, display_data.cursor_y, c, display_data.textcolor, display_data.textbgcolor, display_data.textsize);
      display_data.cursor_x += display_data.textsize * 6;
    }

  } else { // Custom font

    if(c == '\n') {
      display_data.cursor_x  = 0;
      display_data.cursor_y += (int16_t)display_data.textsize * display_data.gfxFont->yAdvance;
    } else if(c != '\r') {
      uint8_t first = display_data.gfxFont->first;
      if((c >= first) && (c <= display_data.gfxFont->last)) {
        uint8_t   c2    = c - display_data.gfxFont->first;
        GFXglyph *glyph = &(display_data.gfxFont->glyph[c2]);
        uint8_t   w     = glyph->width,
                h     = glyph->height;
        if((w > 0) && (h > 0)) { // Is there an associated bitmap?
          int16_t xo = glyph->xOffset;
          if(display_data.wrap && ((display_data.cursor_x + display_data.textsize * (xo + w)) >= display_data.width)) {
            // Drawing character would go off right edge; wrap to new line
            display_data.cursor_x  = 0;
            display_data.cursor_y += (int16_t)display_data.textsize * display_data.gfxFont->yAdvance;
          }
          SSD1306_drawChar(display_data.cursor_x, display_data.cursor_y, c, display_data.textcolor, display_data.textbgcolor, display_data.textsize);
        }
        display_data.cursor_x += glyph->xAdvance * (int16_t)display_data.textsize;
      }
    }
  }

  return 1;
}

// Draw a character
void SSD1306_drawChar(int16_t x, int16_t y, unsigned char c,
 uint16_t color, uint16_t bg, uint8_t size) {

  if(!display_data.gfxFont) { // 'Classic' built-in font

    if((x >= display_data.width)   || // Clip right
       (y >= display_data.height)  || // Clip bottom
       ((x + 6 * size - 1) < 0)   || // Clip left
       ((y + 8 * size - 1) < 0))     // Clip top
      return;

    if(!display_data.cp437 && (c >= 176)) c++; // Handle 'classic' charset behavior

    for(int8_t i=0; i<6; i++ ) {
      uint8_t line;
      if(i < 5) line = default_font[(c*5)+i];
      else      line = 0x0;
      for(int8_t j=0; j<8; j++, line >>= 1) {
        if(line & 0x1) {
          if(size == 1) SSD1306_drawPixel(x+i, y+j, color);
          else          SSD1306_fillRect(x+(i*size), y+(j*size), size, size, color);
        } else if(bg != color) {
          if(size == 1) SSD1306_drawPixel(x+i, y+j, bg);
          else          SSD1306_fillRect(x+i*size, y+j*size, size, size, bg);
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

    uint16_t bo = glyph->bitmapOffset;
    uint8_t  w  = glyph->width,
             h  = glyph->height;
    
    //uint8_t xa = glyph->xAdvance;
    int8_t   xo = glyph->xOffset,
             yo = glyph->yOffset;
    uint8_t  xx, yy, bits = 0, bit = 0;
    int16_t  xo16 = 0, yo16 = 0;

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
        if(!(bit++ & 7)) {
          bits = bitmap[bo++];
        }
        if(bits & 0x80) {
          if(size == 1) {
            SSD1306_drawPixel(x+xo+xx, y+yo+yy, color);
          } else {
            SSD1306_fillRect(x+(xo16+xx)*size, y+(yo16+yy)*size, size, size, color);
          }
        }
        bits <<= 1;
      }
    }

  } // End classic vs custom font
}

void SSD1306_setCursor(int16_t x, int16_t y) {
  display_data.cursor_x = x;
  display_data.cursor_y = y;
}

int16_t SSD1306_getCursorX(void) {
  return display_data.cursor_x;
}

int16_t SSD1306_getCursorY(void) {
  return display_data.cursor_y;
}

void SSD1306_setTextSize(uint8_t s) {
  display_data.textsize = (s > 0) ? s : 1;
}

void SSD1306_setTextColor(uint16_t c, uint16_t b) {
  // For 'transparent' background, we'll set the bg
  // to the same as fg instead of using a flag
  display_data.textcolor   = c;
  display_data.textbgcolor = b;
}

void SSD1306_setTextWrap(bool w) {
  display_data.wrap = w;
}

uint8_t SSD1306_getRotation(void) {
  return display_data.rotation;
}

void SSD1306_setRotation(uint8_t x) {
  display_data.rotation = (x & 3);
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
void SSD1306_cp437(bool x) {
  display_data.cp437 = x;
}

void SSD1306_setFont(const GFXfont *f) {
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

// Pass string and a cursor position, returns UL corner and W,H.
void SSD1306_getTextBounds(char *str, int16_t x, int16_t y,
 int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
  uint8_t c; // Current character

  *x1 = x;
  *y1 = y;
  *w  = *h = 0;

  if(display_data.gfxFont) {

    GFXglyph *glyph;
    uint8_t   first = display_data.gfxFont->first,
            last  = display_data.gfxFont->last,
            gw, gh, xa;
    int8_t    xo, yo;
    int16_t   minx = display_data.width, miny = display_data.height, maxx = -1, maxy = -1,
              gx1, gy1, gx2, gy2, ts = (int16_t)display_data.textsize,
              ya = ts * display_data.gfxFont->yAdvance;

    while((c = *str++)) {
      if(c != '\n') { // Not a newline
        if(c != '\r') { // Not a carriage return, is normal char
          if((c >= first) && (c <= last)) { // Char present in current font
            c    -= first;
            glyph = &(display_data.gfxFont->glyph[c]);
            gw    = glyph->width;
            gh    = glyph->height;
            xa    = glyph->xAdvance;
            xo    = glyph->xOffset;
            yo    = glyph->yOffset;
            if(display_data.wrap && ((x + (((int16_t)xo + gw) * ts)) >= display_data.width)) {
              // Line wrap
              x  = 0;  // Reset x to 0
              y += ya; // Advance y by 1 line
            }
            gx1 = x   + xo * ts;
            gy1 = y   + yo * ts;
            gx2 = gx1 + gw * ts - 1;
            gy2 = gy1 + gh * ts - 1;
            if(gx1 < minx) minx = gx1;
            if(gy1 < miny) miny = gy1;
            if(gx2 > maxx) maxx = gx2;
            if(gy2 > maxy) maxy = gy2;
            x += xa * ts;
          }
        } // Carriage return = do nothing
      } else { // Newline
        x  = 0;  // Reset x
        y += ya; // Advance y by 1 line
      }
    }
    // End of string
    *x1 = minx;
    *y1 = miny;
    if(maxx >= minx) *w  = maxx - minx + 1;
    if(maxy >= miny) *h  = maxy - miny + 1;

  } else { // Default font

    uint16_t lineWidth = 0, maxWidth = 0; // Width of current, all lines

    while((c = *str++)) {
      if(c != '\n') { // Not a newline
        if(c != '\r') { // Not a carriage return, is normal char
          if(display_data.wrap && ((x + display_data.textsize * 6) >= display_data.width)) {
            x  = 0;            // Reset x to 0
            y += display_data.textsize * 8; // Advance y by 1 line
            if(lineWidth > maxWidth) maxWidth = lineWidth; // Save widest line
            lineWidth  = display_data.textsize * 6; // First char on new line
          } else { // No line wrap, just keep incrementing X
            lineWidth += display_data.textsize * 6; // Includes interchar x gap
          }
        } // Carriage return = do nothing
      } else { // Newline
        x  = 0;            // Reset x to 0
        y += display_data.textsize * 8; // Advance y by 1 line
        if(lineWidth > maxWidth) maxWidth = lineWidth; // Save widest line
        lineWidth = 0;     // Reset lineWidth for new line
      }
    }
    // End of string
    if(lineWidth) y += display_data.textsize * 8; // Add height of last (or only) line
    if(lineWidth > maxWidth) maxWidth = lineWidth; // Is the last or only line the widest?
    *w = maxWidth - 1;               // Don't include last interchar x gap
    *h = y - *y1;

  } // End classic vs custom font
}

// Return the size of the display (per current rotation)
int16_t SSD1306_width(void) {
  return display_data.width;
}

int16_t SSD1306_height(void) {
  return display_data.height;
}


