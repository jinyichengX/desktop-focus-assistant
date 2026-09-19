
#ifndef EL_MEMPOOL_H
#define EL_MEMPOOL_H

#include <stdint.h>
#include "elnet.h"
#include "plat.h"

typedef struct blknode block_node_t;

typedef struct blknode
{
	block_node_t * next;
}block_node_t;

typedef struct
{
	uint16_t blk_sz;
	block_node_t block;
	sys_sem_t sem;
	sys_mutex_t mutex;
    uint16_t blk_max;

    /* for isr */
    bool for_isr;
    uint16_t blk_used;
    /* isr on/off apis in plat.h */
}mempool_t;

extern net_err_t  mempool_create(mempool_t * mp, void * mem, uint16_t blk_sz, int blk_cnt);
extern void mempool_destroy(mempool_t * mp);
extern void *     mempool_alloc(mempool_t * mp, uint32_t timeout_tick);
extern void       mempool_free(mempool_t * mp, void * addr);
#endif