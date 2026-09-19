#ifndef EL_NBUF_H
#define EL_NBUF_H

#include "el_nlist.h"
#include "el_ndefs.h"
#include "el_nconfig.h"
#include "el_mempool.h"
#include "defs.h"
#include "lib.h"
#define NBUF_USAGE_STATISTICS 1
/* insert way */
typedef enum{
    INSERT_HEAD,
    INSERT_TAIL,
}ins_way_t;

typedef struct stNetPackageBlock//544B
{
	struct list_head node;          //16B
	void * payload;                 //8B
	int size;                       //4B
	uint8_t data[NETBUF_BLOCK_SIZE];//512+2 514B
}nbuf_blk_t;

typedef struct stNetPackage         //40B
{
	int total_size;                 //8B
	struct list_head node;          //16B //链接block
    struct list_head nbuf_node;     //16B //链接nbuf控制块

    /* read and write operations */
    nbuf_blk_t * cur_blk;           /* current rw block */
    uint16_t t_off;                 /* global rw offset pos */
    uint16_t cur_off;               /* current block rw offset pos */
}nbuf_t;

#define NBUF_TTSZ(nbuf)           ((nbuf)->total_size)
#define NBUF_FIRST_BLKNODE(pnbuf) (((pnbuf)->node).next)
#define NBUF_BLOCK_ENTRY(pnode)   (list_entry((pnode), nbuf_blk_t, node))
#define NBLOCK_NEXT(blk)          (((blk)->node).next) /* get next block */
// #define NBUF_LAST_BLOCK(n)        ((n).node.prev) /* get last block */
#define NBUF_EMPTY(nbuf)          ((list_empty(&(nbuf)->node)) && (nbuf->total_size == 0))
#define PAYLOAD_OFFSET(blk,off)   (blk->payload = ((void *)((uint8_t *)blk->payload+(off))))

/* nbuf total size */
#define NBUF_TOTAL_SIZE(nbuf) ((nbuf)->total_size)

#define NBUF_BLOCK_SIZE MEM_ALIGN_UP(sizeof(nbuf_blk_t),4)
#define NBUF_MANAG_SIZE MEM_ALIGN_UP(sizeof(nbuf_t),4)

#define NBUF_LOOP_BEGIN         __LOOP_BEGIN
#define NBUF_LOOP_END(cond)     __LOOP_END(cond)

typedef enum {
  more_than_lower,
  less_than_upper,
  more_and_less,
}sz_cond_t;

/* check if nbuf size is legal */
static inline char
nbuf_size_legal(nbuf_t * nbuf, uint16_t ll, uint16_t ul, sz_cond_t cond) 
{
  if(cond == more_than_lower)
    return (ll <= nbuf->total_size);
  if(cond == less_than_upper)
    return (ul >= nbuf->total_size);
  if(cond == more_and_less)
    return ((ll <= nbuf->total_size) && (ul >= nbuf->total_size));

  return 0;
}

extern net_err_t  nbuf_init(void);
extern net_err_t  nbuf_free           (nbuf_t * );
extern net_err_t  nbuf_alloc          (nbuf_t **, uint16_t size);
extern void *     nbuf_data           (nbuf_t * );
extern net_err_t  nbuf_splice         (nbuf_t *, nbuf_t * src);
extern net_err_t  nbuf_header         (nbuf_t *, int offset);
extern net_err_t  nbuf_tail_expand    (nbuf_t *, uint16_t offset);
extern net_err_t  nbuf_tail_trim      (nbuf_t * nbuf, uint16_t size);
extern net_err_t  nbuf_extract_prefix (nbuf_t *, uint16_t size);
extern uint16_t   nbuf_checksum16     (nbuf_t *, uint16_t);
extern net_err_t  nbuf_seek           (nbuf_t *, uint16_t off);
extern net_err_t  nbuf_memset         (nbuf_t *, uint16_t size, uint8_t val);
extern net_err_t  nbuf_write          (nbuf_t *, void * data, uint16_t size);
extern net_err_t  nbuf_read           (nbuf_t *, void * data, uint16_t size);
extern net_err_t  nbuf_transfer       (nbuf_t * src, nbuf_t * dst, uint16_t size, uint16_t src_off, uint16_t dst_off);
extern net_err_t  nbuf_acc_reset      (nbuf_t *);
extern void       nbuf_get_statistics (uint8_t *, uint8_t *, uint8_t *, uint8_t *);
extern net_err_t  nbuf_check          (nbuf_t * nbuf);
#endif