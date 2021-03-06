/*
 * Copyright (c) 2020 Gavin Hurlbut
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __adafruit_gfx_cache_h_
#define __adafruit_gfx_cache_h_

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include "adafruit-gfx-defines.h"


struct adafruit_gfx_cache_source_t {
  const struct device *dev;
  size_t cache_offset;
  uint8_t *buffer;
};

struct adafruit_gfx_cache_t {
  struct adafruit_gfx_cache_source_t *source;
#ifdef CONFIG_ADAFRUIT_SSD1306_CACHE
  uint8_t line[SSD1306_CACHE_LINE_SIZE];
#endif
  bool initialized;
  size_t line_addr;
  bool dirty;
};


int adafruit_gfx_cache_init(struct adafruit_gfx_cache_t *cache);
int adafruit_gfx_cache_source_init(struct adafruit_gfx_cache_t *cache, 
        struct adafruit_gfx_cache_source_t *source, size_t start_offset, 
        const uint8_t *buf);

static inline void adafruit_gfx_cache_set_dirty(struct adafruit_gfx_cache_t *cache, bool dirty) {
    cache->dirty = dirty;
}

int adafruit_gfx_cache_source_choose(struct adafruit_gfx_cache_t *cache, struct adafruit_gfx_cache_source_t *source);
void adafruit_gfx_cache_operCache(struct adafruit_gfx_cache_t *cache, int x, int y, oper_t oper_, uint8_t mask);

int adafruit_gfx_cache_load_line(struct adafruit_gfx_cache_t *cache, int x, int y, size_t *pixel_addr);
int adafruit_gfx_cache_save_line(struct adafruit_gfx_cache_t *cache, int x, int y);
int adafruit_gfx_cache_flush_line(struct adafruit_gfx_cache_t *cache);
int adafruit_gfx_cache_clear_all(struct adafruit_gfx_cache_t *cache);
int adafruit_gfx_cache_get_pixel_addr(struct adafruit_gfx_cache_t *cache, int x, int y, uint8_t **pixel);

static inline bool adafruit_gfx_cache_is_in_line(struct adafruit_gfx_cache_t *cache, int x, int y) {
    if (!cache->initialized) {
        return false;
    }
    
    size_t delta = SSD1306_PIXEL_ADDR(x, y) - cache->line_addr;
    return (delta >= 0 && delta <= SSD1306_CACHE_LINE_SIZE);
}


#endif /* __adafruite_gfx_cache_h_ */

