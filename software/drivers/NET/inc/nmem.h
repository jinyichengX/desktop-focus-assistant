#ifndef NMEM_H
#define NMEM_H

#ifdef __cplusplus
extern "C" {
#endif

#define MEM_ALIGN_SIZE          4

#define SL_BITMAP_WIDTH         16
#define FL_BITMAP_VALID_SHIFT   7                               /* define minimum block size 128B */
#define FL_BITMAP_VALID_WIDTH   (32 - FL_BITMAP_VALID_SHIFT)    /* define first level valid width */
#define BLK_SIZE_MIN            (1 << FL_BITMAP_VALID_SHIFT)

#if SL_BITMAP_WIDTH > 32
#error "SL_BITMAP_WIDTH must be less than 32"
#endif

#define offset_of(type, field) ((unsigned int)&(((type *)0)->field))

typedef struct mem_blk_st mem_blk_t;
typedef struct mem_blk_st {
    mem_blk_t * prev_phy;  
    unsigned int size;
    mem_blk_t * next;           /* logical block */
    mem_blk_t * prev;           /* logical block */
}mem_blk_t;

typedef struct {
    unsigned int fl_bmp;
    unsigned int sl_bmp[FL_BITMAP_VALID_WIDTH];
    mem_blk_t * free[FL_BITMAP_VALID_WIDTH][SL_BITMAP_WIDTH];
}mem_mng_t;

typedef int (* mem_walk_cb_t)( mem_mng_t * mem, void * arg);

extern int mem_init(mem_mng_t * mem, void * start, unsigned int size);
extern void * mem_alloc(mem_mng_t * mem, unsigned int size);
extern void mem_free(mem_mng_t * mem, void * p);
extern int mem_walk(mem_mng_t * mem, mem_walk_cb_t cb, void * arg);

#ifdef __cplusplus
}
#endif

#endif