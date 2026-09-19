#ifndef IPGUI_MEMBOX_H
#define IPGUI_MEMBOX_H

#include "ipgui_utils.h"
#include "ipgui_types.h"

IPGUI_HEADER_BEGIN _______________MARKER_______________

typedef struct blknode_ctx blk_node_t;

typedef struct blknode_ctx
{
	blk_node_t * next;
}blk_node_t;

typedef struct
{
	int blk_cnt;
    int blk_used;
	blk_node_t block;
	int blk_sz;
}ipgui_membox_t;

extern __IPGUI_API__ ipgui_err_t ipgui_membox_init(ipgui_membox_t *, void *, unsigned short, int);
extern __IPGUI_API__ ipgui_membox_t * ipgui_membox_create(unsigned short, int);
extern __IPGUI_API__ void ipgui_membox_destroy(ipgui_membox_t *);
extern __IPGUI_API__ void * ipgui_membox_alloc(ipgui_membox_t *);
extern __IPGUI_API__ void ipgui_membox_free(ipgui_membox_t *, void *);
extern __IPGUI_API__ void ipgui_membox_expand(ipgui_membox_t * mb, void * mem, unsigned short blk_sz, int blk_cnt);
IPGUI_HEADER_END   _______________MARKER_______________
#endif