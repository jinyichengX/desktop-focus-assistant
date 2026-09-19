/*
 * MIT License
 *
 * Copyright (c) 2024~2025 JinYiCheng
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef BITMAP_H
#define BITMAP_H

#include <plat.h>
#include "elnet.h"

/* 使用说明：位索引从0开始（如0-255） */
/* 初始化时，最大位数最好设置为uint32_t*8的整数倍 */

typedef uint32_t bmp_size_t;

typedef struct _tag_bitmap_context
{
    // sys_mutex_t  rwlock;
    uint32_t     bmp_idx_max; /* 最大索引 */ 
    uint32_t     bit_max;     /* 最大支持的位数 */
    bmp_size_t * bmp_table;   /* 位图索引 */
}bitmap_t;

/* 一个位图单元有几位 */
#define BITMAP_UNIT_BITNUM        (sizeof(bmp_size_t) * 8)
/* 由bit位得出位图索引 */
#define BITMAP_UINT_IDX(bit)      ((bit) >> 5)     //2^5=32bit
/* 由位图数得出最少需要的单元数目 */
#define NUM_BIT2NUM_UINT(bit_max) (((bit_max) + BITMAP_UNIT_BITNUM - 1) / BITMAP_UNIT_BITNUM)
/* 由bit位得出位图索引中的位置 */
#define BITMAP_BIT_IDX(bit)       ((uint32_t)1u << (BITMAP_UNIT_BITNUM - 1u - ((bit) & (BITMAP_UNIT_BITNUM - 1u))))

/* create bitmap */
static inline net_err_t
bitmap_create_empty(bitmap_t * bmp, bmp_size_t * table, uint32_t bit_max)
{
    uint32_t idx;

    if( !bit_max )
        return NET_ERR_NOK;

    // if( NULL == (bmp->rwlock = sys_mutex_create()) )
    //   return NET_ERR_NOK;

    //bmp->rwlock = mutex;
    bmp->bmp_table = table;
    bmp->bit_max = bit_max;
    bmp->bmp_idx_max = NUM_BIT2NUM_UINT(bit_max);

    for(idx = 0; idx < bmp->bmp_idx_max; idx ++)
        bmp->bmp_table[idx] = 0;

    return NET_ERR_OK;
}

static inline net_err_t
bitmap_create_full(bitmap_t * bmp, bmp_size_t * table, uint32_t bit_max)
{
    uint32_t idx;

    if( !bit_max )
        return NET_ERR_NOK;

    // if( NULL == (bmp->rwlock = sys_mutex_create()) )
    //   return NET_ERR_NOK;

    //bmp->rwlock = mutex;
    bmp->bmp_table = table;
    bmp->bit_max = bit_max;
    bmp->bmp_idx_max = NUM_BIT2NUM_UINT(bit_max);

    for(idx = 0; idx < bmp->bmp_idx_max; idx ++)
        bmp->bmp_table[idx] = ~0;

    return NET_ERR_OK;
}

/* destroy bitmap */
static inline void
bitmap_destroy(bitmap_t * bmp)
{
    //sys_mutex_destroy(bmp->rwlock);
    bmp->bmp_table = NULL;
}

/* set bit in bitmap */
static inline void
bitmap_bit_set(bitmap_t * bmp, uint32_t bit_idx)
{
    if( bmp->bit_max <= bit_idx )
        return;

    //sys_mutex_lock(bmp->rwlock);
    bmp->bmp_table[BITMAP_UINT_IDX(bit_idx)] |= BITMAP_BIT_IDX(bit_idx);
    //sys_mutex_unlock(bmp->rwlock);
}

/* clear bit in bitmap */
static inline void
bitmap_bit_clear(bitmap_t * bmp, uint32_t bit_idx)
{
    if( bmp->bit_max <= bit_idx )
        return;

    //sys_mutex_lock(bmp->rwlock);
    bmp->bmp_table[BITMAP_UINT_IDX(bit_idx)] &= ~BITMAP_BIT_IDX(bit_idx);
    //sys_mutex_unlock(bmp->rwlock);
}

/* check if bit set in bitmap */
static inline char
bitmap_bit_is_set(bitmap_t * bmp, uint32_t bit_idx)
{
    if( bmp->bit_max <= bit_idx )
        return (char)NET_ERR_NOK;

    return (bmp->bmp_table[BITMAP_UINT_IDX(bit_idx)] & BITMAP_BIT_IDX(bit_idx)) ? 1 : 0;
}

/* check if bit cleared in bitmap */
static inline char
bitmap_bit_is_clear(bitmap_t * bmp, uint32_t bit_idx)
{
    if( bmp->bit_max <= bit_idx )
        return (char)NET_ERR_NOK;
    
    return bitmap_bit_is_set(bmp, bit_idx) ? 0 : 1;
}

#if defined(__GNUC__) || defined(__clang__)
#define clz(x) __builtin_clz(x)
#else
static uint32_t clz(uint32_t x)
{
    uint32_t count = 0;
    if (x == 0) return 32;
    if ((x & 0xFFFF0000) == 0) { x <<= 16; count += 16; }
    if ((x & 0xFF000000) == 0) { x <<= 8; count += 8; }
    if ((x & 0xF0000000) == 0) { x <<= 4; count += 4; }
    if ((x & 0xC0000000) == 0) { x <<= 2; count += 2; }
    if ((x & 0x80000000) == 0) { x <<= 1; count += 1; }
    return count;
}
#endif

/* get lowest set bit index in bitmap */
static inline uint32_t
bitmap_lowest_set_idx(bitmap_t * bmp)
{
    uint32_t idx, lbs = 0;
    bmp_size_t * bmp_uint = bmp->bmp_table;

    //sys_mutex_lock(bmp->rwlock);
    for(idx = 0; idx < bmp->bmp_idx_max; idx ++)
    {
        if( *bmp_uint == 0 )
        {
            lbs += BITMAP_UNIT_BITNUM;
            bmp_uint ++;
        }
        else{
            lbs += clz(*bmp_uint);//__builtin_clz(x)返回x的二进制的前导0的个数
            break;
        }
    }
    //sys_mutex_unlock(bmp->rwlock);

    return (lbs >= bmp->bit_max) ? bmp->bit_max : lbs;
}

#endif