
/*
 * Copyright (c) 2020 Gavin Hurlbut
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(adafruit_ssd1306, CONFIG_DISPLAY_LOG_LEVEL);

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#ifdef CONFIG_ADAFRUIT_SSD1306_CACHE
#include <drivers/ram.h>
#endif
#include "adafruit-gfx-defines.h"
#include "adafruit-gfx-cache.h"


int adafruit_gfx_cache_init(struct adafruit_gfx_cache_t *cache, size_t start_offset, const uint8_t *buf, size_t len)
{
    int ret = 0;
    
#ifdef CONFIG_ADAFRUIT_SSD1306_CACHE
    cache->dev = device_get_binding(DT_ALIAS("ssd1306_cache"));
    if (cache->dev == NULL) {
        LOG_ERR("Can't find the ssd1306_cache RAM device!");
        return -EINVAL;
    }
    cache->initialized = false;
    
    size_t ram_size;
    ret = ram_get_size(cache->dev, &ram_size);
    if (ret != 0) {
        return ret;
    }
    
    if (SSD1306_CACHE_LINE_SIZE + start_offset > ram_size) {
        LOG_ERR("Cache does not fit in RAM with given offset");
        return -EINVAL;
    }
#else
    cache->dev = NULL;
    cache->line_addr = 0;
    cache->initialized = true;
#endif
    cache->dirty = false;
    cache->start_offset = start_offset;
    
    if (buf) {
        size_t i = 0;
        do {
            if ((i % SSD1306_CACHE_LINE_SIZE) == 0) {
                memset(cache->line, 0, SSD1306_CACHE_LINE_SIZE);
            }
            
            cache->line[i % SSD1306_CACHE_LINE_SIZE] = buf[i];
            if ((i % SSD1306_CACHE_LINE_SIZE) == SSD1306_CACHE_LINE_SIZE) {
                cache->dirty = true;
                ret = adafruit_gfx_cache_save_line(cache, i, 0);
                if (ret != 0) {
                    return ret;
                }
            }
            i++;
        } while (i < len);
        
        if ((i % SSD1306_CACHE_LINE_SIZE) != 0) {
            cache->dirty = true;
            ret = adafruit_gfx_cache_save_line(cache, i, 0);
        }
    }
    
    return ret;
}


int adafruit_gfx_cache_get_pixel_addr(struct adafruit_gfx_cache_t *cache, int x, int y, uint8_t **pixel)
{
    size_t pixel_addr;
    
    int ret = adafruit_gfx_cache_load_line(cache, x, y, &pixel_addr);
    if (ret != 0) {
        return ret;
    }
    
    if (pixel) {
        *pixel = &cache->line[pixel_addr];
    }
    
    return 0;
}


void adafruit_gfx_cache_operCache(struct adafruit_gfx_cache_t *cache, int x, int y, oper_t oper_, uint8_t mask)
{
    uint8_t *addr;
    
    int ret = adafruit_gfx_cache_get_pixel_addr(cache, x, y, &addr);
    if (ret != 0) {
        return;
    }
    
    uint8_t data = *addr;
    uint8_t origdata = data;

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

    if (data != origdata) {
        *addr = data;
        cache->dirty = true;
    }
}


int adafruit_gfx_cache_load_line(struct adafruit_gfx_cache_t *cache, int x, int y, size_t *pixel_addr)
{
    int ret = 0;
    
    if (cache->initialized && adafruit_gfx_cache_is_in_line(cache, x, y)) {
        /* No need to read anything, it's already cached! */
        goto done;
    }
    
#ifdef CONFIG_ADAFRUIT_SSD1306_CACHE
    
    if (cache->dev && cache->initialized && cache->dirty) {
        /* Trying to read a new line, and the cache line is dirty.  Write it back first, THEN read in the
         * new data
         */
        ret = adafruit_gfx_cache_flush_line(cache);
        if (ret != 0) {
            return ret;
        }
    }
    
    cache->dirty = false;
    
    if (cache->dev) {
        /* We actually have a cache RAM, read the line from it. */
        cache->line_addr = SSD1306_CACHE_LINE_ADDR(x, y);
        ret = ram_read(cache->dev, cache->line_addr + cache->start_offset,
                       cache->line, SSD1306_CACHE_LINE_SIZE);
        if (ret != 0) {
            return ret;
        }
        cache->initialized = true;
    }
#endif
    
done:
    if (ret == 0 && pixel_addr) {
        *pixel_addr = SSD1306_CACHE_LINE_PIXEL_ADDR(x, y);
    }
    
    return ret;
}


int adafruit_gfx_cache_save_line(struct adafruit_gfx_cache_t *cache, int x, int y)
{
    int ret = 0;
    
    if (!cache->dev) {
        /* Not using a cache, nothing to do! */
        return 0;
    }
    
#ifdef CONFIG_ADAFRUIT_SSD1306_CACHE

    size_t line_addr = SSD1306_CACHE_LINE_ADDR(x, y);
    
    if (line_addr == cache->line_addr && cache->initialized && !cache->dirty) {
        /* The cache is clean, and we are writing to the line where it was read from.
         * Nothing to do, move along.
         */
        return 0;
    }
    
    cache->line_addr = line_addr;
    ret = ram_write(cache->dev, cache->line_addr + cache->start_offset,
                    cache->line, SSD1306_CACHE_LINE_SIZE);
                    
    if (ret == 0) {
        cache->initialized = true;
        cache->dirty = false;
    }
#endif
    return ret;
}

int adafruit_gfx_cache_flush_line(struct adafruit_gfx_cache_t *cache)
{
    return adafruit_gfx_cache_save_line(cache, cache->line_addr, 0);
}

int adafruit_gfx_cache_clear_all(struct adafruit_gfx_cache_t *cache)
{
  size_t line_addr;
  int ret = 0;
  
  memset(cache->line, 0, SSD1306_CACHE_LINE_SIZE);
  for (line_addr = 0; line_addr < SSD1306_RAM_MIRROR_SIZE; line_addr += SSD1306_CACHE_LINE_SIZE) {
      cache->line_addr = line_addr;
      cache->dirty = true;
      ret = adafruit_gfx_cache_flush_line(cache);
      if (ret != 0) {
          return ret;
      }
  }
  return 0;
}