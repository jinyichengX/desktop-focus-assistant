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

#include "nmem.h"
#include "dbg.h"

#define ALIGN_UP(x, align)          ((((unsigned int)(x)) + (align) - 1) & (~((align) - 1)))

#define BLK_FREE_FLAG_SHIFT          0
#define BLK_PREV_PHY_FREE_FLAG_SHIFT 1

/* mask of block free/prev phy free */
#define BLK_FREE_FLAG_MASK          (1U << BLK_FREE_FLAG_SHIFT)
#define BLK_PREV_PHY_FREE_FLAG_MASK (1U << BLK_PREV_PHY_FREE_FLAG_SHIFT)
#define BLK_FLAG_MASK               (BLK_FREE_FLAG_MASK | BLK_PREV_PHY_FREE_FLAG_MASK)

#define BLOCK_SIZE(x)               (((x)->size) & (~(BLK_FLAG_MASK)))

#define BLK_FIELD_SIZE_SIZE         (offset_of(mem_blk_t, next) - offset_of(mem_blk_t, size))

static inline void block_mark_used(mem_blk_t * blk)
{
    blk->size &= ~BLK_FREE_FLAG_MASK;
}

static inline void block_prev_phy_mark_used(mem_blk_t * blk)
{
    blk->size &= ~BLK_PREV_PHY_FREE_FLAG_MASK;
}

static inline void block_mark_free(mem_blk_t * blk)
{
    blk->size |= BLK_FREE_FLAG_MASK;
}

static inline void block_prev_phy_mark_free(mem_blk_t * blk)
{
    blk->size |= BLK_PREV_PHY_FREE_FLAG_MASK;
}

static inline int block_is_free(mem_blk_t * blk)
{
    return (!!(blk->size & BLK_FREE_FLAG_MASK));
}

static inline int block_prev_phy_is_free(mem_blk_t * blk)
{
    return (!!(blk->size & BLK_PREV_PHY_FREE_FLAG_MASK));
}

static inline mem_blk_t * ptr_to_blk(void * p)
{
    return (mem_blk_t *)((unsigned int)p - offset_of(mem_blk_t, next));
}

static inline void * blk_to_ptr(mem_blk_t * blk)
{
    return (void *)((unsigned int)blk + offset_of(mem_blk_t, next));
}

static inline void * blk_to_size(mem_blk_t * blk)
{
    return (void *)((unsigned int)blk + offset_of(mem_blk_t, size));
}

static mem_blk_t * block_next_phy(mem_blk_t * blk)
{
    return (mem_blk_t *)((unsigned int)blk + BLOCK_SIZE(blk));
}

static inline void set_block_size(mem_blk_t * blk, unsigned int size)
{
    blk->size = size | (blk->size & BLK_FLAG_MASK);
}

/* find highest bit set(32 ~ 1) */
static int generic_fls(unsigned int v)
{
    int ret = 32;
    if (!v) return 0;
    if (!(v & 0xffff0000u)) { v <<= 16; ret -= 16; }
    if (!(v & 0xff000000u)) { v <<= 8;  ret -= 8;  }
    if (!(v & 0xf0000000u)) { v <<= 4;  ret -= 4;  }
    if (!(v & 0xc0000000u)) { v <<= 2;  ret -= 2;  }
    if (!(v & 0x80000000u)) { v <<= 1;  ret -= 1;  }
    return ret;
}

/* find lowest bit set(1 ~ 32) */
static int generic_ffs(unsigned int v)
{
    int ret = 1;
    if (!v) return 0;
    if (!(v & 0x0000ffffu)) { ret += 16; v >>= 16; }
    if (!(v & 0x000000ffu)) { ret += 8;  v >>= 8;  }
    if (!(v & 0x0000000fu)) { ret += 4;  v >>= 4;  }
    if (!(v & 0x00000003u)) { ret += 2;  v >>= 2;  }
    if (!(v & 0x00000001u)) { ret += 1;  v >>= 1;  }
    return ret;
}

/* insert to first */
static void block_insert(mem_blk_t * blk, mem_blk_t ** start)
{
    mem_blk_t * first = * start;
    * start = blk;
    blk->next = first;
    blk->prev = (mem_blk_t *)0;
    if (first) {
        first->next = (mem_blk_t *)0;
        first->prev = blk;
    }
}

/* remove block */
static void block_remove(mem_blk_t * blk, mem_blk_t ** start)
{
//    DBG_ASSERT(* start, "remove first err, blk is null");

    mem_blk_t * first = * start;
    
    if (blk == first) {
        * start = first->next;
        if (* start)
            (* start)->prev = (mem_blk_t *)0;
    } else {
        blk->prev->next = blk->next;
        if (blk->next)
            blk->next->prev = blk->prev;
    }
}

/* mapping size to fl and sl */
static void mapping_index(unsigned int size, int * fl, int * sl)
{
    int fl_idx, sl_idx;

    fl_idx = generic_fls(size) - 1;
    sl_idx = (size - (1 << fl_idx)) * SL_BITMAP_WIDTH / (1 << fl_idx);
    fl_idx -= FL_BITMAP_VALID_SHIFT;

    *fl = fl_idx;
    *sl = sl_idx;
}

static void insert_free_block(mem_mng_t * mem, mem_blk_t * blk)
{
//    DBG_ASSERT(block_is_free(blk), "insert free err, blk is used");

    int fl, sl;
    mapping_index(BLOCK_SIZE(blk), &fl, &sl);
    block_insert(blk, &mem->free[fl][sl]);

    /* update fl_bmp and sl_bmp */
    mem->fl_bmp |= (1 << fl);
    mem->sl_bmp[fl] |= (1 << sl);
}

static void remove_free_block(mem_mng_t * mem, mem_blk_t * blk)
{
    int fl, sl;
    mapping_index(BLOCK_SIZE(blk), &fl, &sl);
    block_remove(blk, &mem->free[fl][sl]);

    /* update fl_bmp and sl_bmp */
    if (mem->free[fl][sl] == (mem_blk_t *)0) {
        mem->sl_bmp[fl] &= (~(1 << sl));
    }
    if (0 == mem->sl_bmp[fl]) {
        mem->fl_bmp &= (~(1 << fl));
    }
}

/* at most 4GB memory can be managed */
int mem_init(mem_mng_t * mem, void * start, unsigned int size)
{
//    DBG_ASSERT(mem && start, "invalid parameter, mem and start must be valid");
	if (size == 0) return 0;
    int valid_size;
    int fl, sl;
    mem_blk_t * blk;
    unsigned int pool_start =\
    ALIGN_UP((unsigned int)start, MEM_ALIGN_SIZE);
    pool_start += offset_of(mem_blk_t, size);

    /* check pool size */
    valid_size = size - (pool_start - (unsigned int)start);
    if (valid_size < BLK_SIZE_MIN) {
        return 1;
    }

    /* init management struct */
    mem->fl_bmp = 0;
    for (int iter = 0; iter < FL_BITMAP_VALID_WIDTH; ++ iter) {
        mem->sl_bmp[iter] = 0;
        for (int idx = 0; idx < SL_BITMAP_WIDTH; ++ idx) {
            mem->free[iter][idx] = (mem_blk_t *)0;
        }
    }

    /* insert first whole free block */
    blk = (mem_blk_t *)((unsigned int)pool_start - offset_of(mem_blk_t, size));
    
    set_block_size(blk, valid_size);
    block_prev_phy_mark_used(blk);
    block_mark_free(blk);
    insert_free_block(mem, blk);

    return 0;
}

static mem_blk_t * find_suitbale_block(mem_mng_t * mem, int size,
                                        int * pfl, int * psl)
{
    int fl, sl;
    unsigned int fl_bmp, sl_bmp;
    mem_blk_t * blk = (mem_blk_t *)0;
    mapping_index(size, &fl, &sl);

    sl_bmp = mem->sl_bmp[fl] & ((~0U) << sl);
    if (!sl_bmp)/* fl is 0 */
    {   
        fl_bmp = mem->fl_bmp & ((~0U) << fl);
        if (!fl_bmp) 
        {
            return (mem_blk_t *)0;
        }
        fl = generic_ffs(fl_bmp) - 1;
    }

    /* find lowest bit set */
    sl = generic_ffs(mem->sl_bmp[fl]) - 1;
    blk = mem->free[fl][sl];
    * pfl = fl; * psl = sl;

    return blk;
}

static void split_block(mem_blk_t * blk, int size, mem_blk_t ** pnext)
{
    * pnext = (mem_blk_t *)((unsigned int)blk_to_size(blk) + size);
    * pnext = (mem_blk_t *)((unsigned int)(* pnext) - offset_of(mem_blk_t, size));
}

void * mem_alloc(mem_mng_t * mem, unsigned int size)
{
    void * p;
    mem_blk_t * blk;
    int fl, sl;
    if (!size) return (void *)0;
	__asm ("cpsid i");
    /* size needed */
    size = ALIGN_UP(size + BLK_FIELD_SIZE_SIZE, MEM_ALIGN_SIZE);
    if (size < BLK_SIZE_MIN) {
        size = BLK_SIZE_MIN;
    }

    /* find suitbale block */
    blk = find_suitbale_block(mem, size, &fl, &sl);
    if (blk == (mem_blk_t *)0) {
        return (void *)0;
    }

    /* if remaining size larger than BLK_MIN, 
     * split block, else allocate all
     */
    if ((BLOCK_SIZE(blk) - size) >= BLK_SIZE_MIN) {
        mem_blk_t * next_blk;
        split_block(blk, size, &next_blk);
        remove_free_block(mem, blk);

        /* set size for this allocated block and next phy block */
        set_block_size(next_blk, BLOCK_SIZE(blk) - size);
        set_block_size(blk, size);

        block_prev_phy_mark_used(blk);
        block_mark_used(blk);

        block_prev_phy_mark_used(next_blk);
        block_mark_free(next_blk);
        insert_free_block(mem, next_blk);
    } else {
        block_prev_phy_mark_used(blk);
        block_mark_used(blk);
        remove_free_block(mem, blk);
    }

    p = (void *)blk_to_ptr(blk);
		__asm ("cpsie i");
    return p;
}

static mem_blk_t * merge_prev_phy(mem_mng_t * mem, mem_blk_t * blk, mem_blk_t * prev)
{
    remove_free_block(mem, prev);
    set_block_size(prev, BLOCK_SIZE(blk) + BLOCK_SIZE(prev));
    return prev;
}

static mem_blk_t * merge_next_phy(mem_mng_t * mem, mem_blk_t * blk, mem_blk_t * next)
{
    remove_free_block(mem, next);
    set_block_size(blk, BLOCK_SIZE(blk) + BLOCK_SIZE(next));
    return blk;
}

void mem_free(mem_mng_t * mem, void * p)
{
    DBG_ASSERT(mem && p, "invalid parameter, mem and p must be valid");
    
	__asm ("cpsid i");
    mem_blk_t * blk, * next_blk;

    blk = ptr_to_blk(p);
    next_blk = block_next_phy(blk);
    
    /* first, set this block free 
     * second, set next physical block's prev physical block free 
     */
    block_mark_free(blk);
    next_blk->prev_phy = blk;
    block_prev_phy_mark_free(next_blk);

    /* merge prev physical memory if possible */
    if (block_prev_phy_is_free(blk)) {
        /* merge prev phy */
        mem_blk_t * prev_blk;
        prev_blk = blk->prev_phy;
        blk = merge_prev_phy(mem, blk, prev_blk);
    }

    /* merge next physical memory if possible */
    if (block_is_free(next_blk)) {
        /* merge next phy */
        blk = merge_next_phy(mem, blk, next_blk);
    }

    insert_free_block(mem, blk);
		__asm ("cpsie i");
}

static void memset(void * pv, char v, unsigned int len)
{   
    int seg;
    unsigned int up;
    char * pe = (char *)pv + len;
    int vex = 0;

    if( !len || !pv )
        return;

    /* if length < 8, copy directly */
    if( len < 8 ){
        for( int i = 0; i < len; i++ )
            ((char *)pv)[i] = v;
        return;
    }

    /* generate vex */
    for( int i = 0; i < 4; i++ )
        vex |= (v << (i * 8));
    
    up = ALIGN_UP((unsigned int)pv, 4);

    seg = up - (unsigned int)pv;
    len -= seg;
    /* copy unaligned size */
    while(seg--)
        ((char *)pv)[seg] = v;

    /* copy aligned size */
    seg = len / 4;
    len -= seg * 4;
    while(seg --)
        ((int *)up)[seg] = vex;

    /* copy unaligned size */
    seg = len;
    while(seg--)
        (pe - len)[seg] = v;
}

void * mem_calloc(mem_mng_t * mem, unsigned int size)
{
    void * p = mem_alloc(mem, size);
    if (p) {
        memset(p, 0, size);
    }
    return p;
}

int mem_walk(mem_mng_t * mem, mem_walk_cb_t cb, void * arg)
{
    
}

#if 0
void test_nmem(void)
{
    static mem_mng_t g_mem;
    static char g_mem_pool[16 * 1024];

    void * p1, * p2, * p3, * p4;
    mem_init(&g_mem, (void *)g_mem_pool, sizeof(g_mem_pool));
    p1 = mem_alloc(&g_mem, 55);
    p2 = mem_alloc(&g_mem, 480);
    p3 = mem_alloc(&g_mem, 480);
    p4 = mem_alloc(&g_mem, 480);
    mem_free(&g_mem, p3);
    mem_free(&g_mem, p1);
    mem_free(&g_mem, p4);
    mem_free(&g_mem, p2);
}
#endif
