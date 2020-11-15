/*
 * Copyright (c) 2020 Gavin Hurlbut
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __adafruit_gfx_utils_h__
#define __adafruit_gfx_utils_h__

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define clamp(x, y, z) min(max((x), (y)), (z))
#define _abs(x) ((x) < 0 ? -(x) : (x))
    
#ifndef _swap_int16
#define _swap_int16(a, b) { int16_t t = a; a = b; b = t; }
#endif

#ifndef _swap_int
#define _swap_int(a, b) { int t = a; a = b; b = t; }
#endif
    
// 32-bit word => ABCD, 16 bit word => CD
#define BYTE_A(x)  (((x) >> 24) & 0xFF)
#define BYTE_B(x)  (((x) >> 16) & 0xFF)
#define BYTE_C(x)  (((x) >> 8)  & 0xFF)
#define BYTE_D(x)  ((x)         & 0xFF)

#define TO_BYTE_A(x) (((x) & 0xFF) << 24)
#define TO_BYTE_B(x) (((x) & 0xFF) << 16)
#define TO_BYTE_C(x) (((x) & 0xFF) << 8)
#define TO_BYTE_D(x) ((x) & 0xFF)
    
#define WORD_AB(x) (((x) >> 16) & 0xFFFF)
#define WORD_CD(x) ((x)         & 0xFFFF)
    
#define TO_WORD_AB(x) (((x) & 0xFFFF) << 16)
#define TO_WORD_CD(x) ((x)  & 0xFFFF)
    
#ifndef ustrlen
#define ustrlen(x) (strlen((char *)(x)))
#endif

#define NELEMS(x) (sizeof(x) / sizeof((x)[0]))

#endif // __adafruit_gfx_utils_h__

/* [] END OF FILE */
