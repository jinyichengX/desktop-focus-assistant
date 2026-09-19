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

#include "el_nbuf.h"
#include "elnet.h"
#include "global.h"
#include "defs.h"
#include <math.h>

#ifndef TIMEOUT
#define TIMEOUT(x) (x)
#endif
#define NBUF_DEBUG 0
#define NBUF_UGLY_DEBUG 0
#if NBUF_USAGE_STATISTICS == 1
volatile static uint32_t nbuf_blk_used = 0;
volatile static uint32_t nbuf_header_used = 0;
volatile static sys_mutex_t nbuf_mutex = NULL;
volatile static sys_mutex_t nbuf_mutex_h = NULL;
#endif
static void
_memcpy(uint8_t * _tar, uint8_t * _src, uint16_t len);
/* init nbuf */
net_err_t nbuf_init(void)
{
    net_err_t err = NET_ERR_OK;

    /* init nbuf and block pool */
    err |= mempool_create(&net_buf_pool, (void *)nbuf_manager, sizeof(nbuf_t), NETBUF_PACKET_NUM);
    err |= mempool_create(&nbuf_block_pool, (void *)nbuf_blk, sizeof(nbuf_blk_t), NETBUF_BLOCK_NUM);
#if NBUF_USAGE_STATISTICS == 1
    err |= (NULL== (nbuf_mutex = sys_mutex_create())) ? NET_ERR_NOK : NET_ERR_OK;
    err |= (NULL== (nbuf_mutex_h = sys_mutex_create())) ? NET_ERR_NOK : NET_ERR_OK;
#endif
    return (err == NET_ERR_OK) ? NET_ERR_OK : NET_ERR_NOK;
}

/* get block free size */
uint16_t nblock_freesz(nbuf_blk_t * blk)
{
    return (uint16_t)((uint8_t *)blk->payload - (uint8_t *)(void *)blk->data);
}

/* get block used size */
uint16_t nblock_usedsz(nbuf_blk_t * blk)
{
    return blk->size;
}

/* get block discarded size */
uint16_t nblock_tail_size(nbuf_blk_t * blk)
{
    return (uint16_t)((blk->data + NETBUF_BLOCK_SIZE) - (((uint8_t *)blk->payload) + blk->size));
}

#if NBUF_DEBUG == 1
/* check nbuf */
net_err_t nbuf_check(nbuf_t * nbuf)
{
    int total_size = 0;
    struct list_head * pos, * next;
    uint16_t used_size = 0, free_size = 0, discarded_size = 0;
    nbuf_blk_t * blk;
    int i = 0;

#if NBUF_UGLY_DEBUG
    plat_printf("---------------------------------------\r\n");
#endif
    plat_printf("nbuf total size = %d\r\n", nbuf->total_size);
#if NBUF_UGLY_DEBUG
    plat_printf("nbuf node addr = %p ", (void *)&nbuf->node);
    plat_printf("nbuf node next addr = %p , prev addr = %p\r\n", (void *)nbuf->node.next, (void *)nbuf->node.prev);
#endif
    /* traverse all block list */
    list_for_each_safe(pos, next, &nbuf->node)
    {
        blk = NBUF_BLOCK_ENTRY(pos);
        /* prev free size */
        free_size = nblock_freesz(blk);
        /* used size */
        used_size = nblock_usedsz(blk);
        /* discarded size */
        discarded_size = nblock_tail_size(blk);
#if NBUF_UGLY_DEBUG == 0
        plat_printf("nblock  addr = %p \r\n", (void *)blk);
#endif
#if NBUF_UGLY_DEBUG == 1
        plat_printf("nblock node addr = %p ", (void *)&blk->node);
        plat_printf("nblock node next addr = %p , prev addr = %p\r\n", (void *)blk->node.next,(void *)blk->node.prev);
#endif
#if NBUF_UGLY_DEBUG == 1
        plat_printf("nbuf blk id = %d:free size = %d, used size = %d, discarded size = %d\r\n", ++i, free_size, used_size, discarded_size);
#endif
        if((free_size + used_size + discarded_size) != NETBUF_BLOCK_SIZE)
        {
            return NET_ERR_NOK;
        }
        total_size += used_size;

    }
    if(total_size != nbuf->total_size)
    {
        plat_printf("total size != nbuf->total_size\r\n");
        return NET_ERR_NOK;
    }
#if NBUF_UGLY_DEBUG == 1
    plat_printf("nbuf total off = %d, current block addr = %p, current block off = %d\r\n", nbuf->t_off, (void *)nbuf->cur_blk, nbuf->cur_off);
#endif
    return NET_ERR_OK;
}
#else
#define nbuf_check(nbuf) NET_ERR_OK
#endif

/* init nbuf block */
void nbuf_block_init(nbuf_blk_t * blk, void * p, uint16_t sz)
{
    blk->size = sz;
    blk->payload = p;
    INIT_LIST_HEAD(&blk->node);
}

/* free nbuf block */
void nbuf_block_free(nbuf_blk_t * blk)
{
    list_del(&blk->node);
    mempool_free(&nbuf_block_pool, (void *)blk);
#if NBUF_USAGE_STATISTICS == 1
    sys_mutex_lock(nbuf_mutex);
    nbuf_blk_used --;
    sys_mutex_unlock(nbuf_mutex);
#endif
}

/* free nbuf block list */
void nbuf_block_list_free(nbuf_blk_t * start)
{
    struct list_head * header;
    struct list_head * pos, * next;
    nbuf_blk_t * nbuf_blk;

    header = start->node.prev;

    /* free nbuf block and nbuf manager */
    list_for_each_safe(pos, next, header)
    {
        nbuf_blk = NBUF_BLOCK_ENTRY(pos);
        nbuf_block_free(nbuf_blk);
    }
}

/* alloc nbuf block */
nbuf_blk_t * nbuf_block_alloc(void)
{
#if NBUF_USAGE_STATISTICS == 1
    nbuf_blk_t * blk =
#else
    return (nbuf_blk_t *)
#endif
    mempool_alloc(&nbuf_block_pool, 0);/* 如果改为0xffffffff就不需要在nbuf_alloc中定义_return了 */
#if NBUF_USAGE_STATISTICS == 1
    if(blk){
        sys_mutex_lock(nbuf_mutex);
        nbuf_blk_used ++;
        sys_mutex_unlock(nbuf_mutex);
    }
    return blk;
#endif
}

/* alloc nbuf block list and add to head */
net_err_t nbuf_block_list_alloc(nbuf_t ** nbuf, uint16_t size, ins_way_t hot)
{
    uint16_t blk_size;
    nbuf_blk_t * blk;
    void * payload;
    uint16_t fix = NETBUF_BLOCK_SIZE;
    nbuf_t * temp = * nbuf;
    
    while(size)
    {
        if (NULL == (blk = nbuf_block_alloc()))
            return NET_ERR_NOK;

        blk_size = NMIN(size, fix);

        payload = (void *)(blk->data + ((hot == INSERT_HEAD) ? (fix - blk_size) : 0));
        /* init nbuf_blk_t struct */
        nbuf_block_init(blk, payload, blk_size);

        /* add to nbuf list */
        if (hot == INSERT_HEAD)
        {
            list_add(&blk->node, &(temp->node));
        }
        else
        {
            list_add_tail(&blk->node, &(temp->node));
        }
        /* update left nbuf size */
        size -= blk_size;
        blk = NULL;
    }

    return NET_ERR_OK;
}

/* destroy block list and free nbuf head */
net_err_t nbuf_free(nbuf_t * nbuf)
{
    nbuf_blk_t * start;
    struct list_head * node;

    if (NBUF_EMPTY(nbuf))
        goto _nbuf_head_free;

    node = nbuf->node.next;
    start = list_entry(node, nbuf_blk_t, node);

    /* free nbuf block list from first block */
    nbuf_block_list_free(start);
_nbuf_head_free:
    /* free nbuf head */
    mempool_free(&net_buf_pool, (void *)nbuf);
#if NBUF_USAGE_STATISTICS == 1
    sys_mutex_lock(nbuf_mutex_h);
    nbuf_header_used --;
    sys_mutex_unlock(nbuf_mutex_h);
#endif
    return NET_ERR_OK;
}

/* alloc nbuf and create block list */
net_err_t nbuf_alloc(nbuf_t ** nbuf, uint16_t size)
{
    if (size == 0) 
        return NET_ERR_NOK;

    *nbuf = (nbuf_t *)0;

    /* alloc nbuf_t struct from mempool */
    if (NULL == (*nbuf = (nbuf_t *)mempool_alloc(&net_buf_pool, 0)))
    {
        return NET_ERR_NOK;
    }
#if NBUF_USAGE_STATISTICS == 1
    sys_mutex_lock(nbuf_mutex_h);
    nbuf_header_used ++;
    sys_mutex_unlock(nbuf_mutex_h);
#endif
    plat_memset((void *)(*nbuf), 0, sizeof(nbuf_t));
    INIT_LIST_HEAD(&((*nbuf)->node));

    /* alloc nbuf block list */
    if (NET_ERR_NOK == nbuf_block_list_alloc(nbuf, size, INSERT_HEAD)) {
        goto _return;
    }

    /* init nbuf_t struct */
    INIT_LIST_HEAD(&((*nbuf)->nbuf_node));
    (*nbuf)->total_size = size;
    /* init rw parameters */
    (*nbuf)->t_off = 0;
    (*nbuf)->cur_off = 0;
    (*nbuf)->cur_blk = size ? NBUF_BLOCK_ENTRY(NBUF_FIRST_BLKNODE(*nbuf)) : NULL;

#if 0
    if(NET_ERR_NOK == nbuf_check(*nbuf)){
        plat_printf("error: nbuf check error\r\n");
        return NET_ERR_NOK;
    }
#endif

    return NET_ERR_OK;
_return:
    if (!list_empty(&((*nbuf)->node)))
    {
        nbuf_block_list_free(list_entry(((*nbuf)->node.next), nbuf_blk_t, node));
    }
    mempool_free(&net_buf_pool, (void *)(*nbuf));
    (* nbuf) = NULL;
    return NET_ERR_NOK;
}

/* get nbuf data pointer */
void * nbuf_data(nbuf_t * nbuf)
{
    struct list_head *node;
    nbuf_blk_t * blk;

    if (NBUF_EMPTY(nbuf))
        return NULL;

    /* get first nbuf_blk_t struct */
    node = NBUF_FIRST_BLKNODE(nbuf);

    if (node == &nbuf->node)
        return NULL;

    blk = NBUF_BLOCK_ENTRY(node);

    return blk->payload;
}

/* add some size for constant header */
net_err_t nbuf_header_expand(nbuf_t * nbuf, uint16_t offset)
{
    uint16_t free;
    nbuf_blk_t * blk;
    struct list_head * node;

    if (!NBUF_EMPTY(nbuf))
    {
        node = NBUF_FIRST_BLKNODE(nbuf);
        blk = NBUF_BLOCK_ENTRY(node);

        free = nblock_freesz(blk);

        blk->size += NMIN(free, offset);
        PAYLOAD_OFFSET(blk, -NMIN(free, offset));

        if (offset <= free)
            return NET_ERR_OK;

        offset -= free;
    }
    return nbuf_block_list_alloc(&nbuf, offset, INSERT_HEAD);
}

/* cut some size for constant header */
net_err_t nbuf_header_cut(nbuf_t * nbuf, uint16_t size)
{
    nbuf_blk_t * idx;
    struct list_head * n, * next;

    n = (nbuf->node).next;
    next = n->next;

    /* loop until size is zero */
    for (; size; n = next, next = n->next)
    {
        idx = list_entry(n, nbuf_blk_t, node);
        /*
         * if left size is larger than current block size,
         * modify current block parameter and return
         */
        if ((uint16_t)(idx->size) > size)
        {
            idx->size -= size;
            idx->payload = (void *)((uint8_t *)idx->payload + size);
            break;
        }
        /*
         * if size is less than nbuf_blk_t payload size, free it
         */
        size -= idx->size;
        nbuf_block_free(idx);
    }

    return NET_ERR_OK;
}

/* alloc or cut some size front */
net_err_t nbuf_header(nbuf_t * nbuf, int offset)
{
    int offset_bk = offset;

    if (0 == offset)
        return NET_ERR_OK;

    /* check parameter */
    if (abs(offset) > NETBUF_BLOCK_SIZE)
        return NET_ERR_NOK;
    if ((offset < 0) && ((-offset) > nbuf->total_size))
        return NET_ERR_NOK;

    /* if offset is positive,add some size
     * else cut some size
     */
    if (offset > 0)
    {
        if (NET_ERR_NOK == nbuf_header_expand(nbuf, offset))
            return NET_ERR_NOK;
    }
    else
    {
        uint16_t size = -offset;
        nbuf_header_cut(nbuf, size);
    }

    nbuf->total_size += offset_bk;
#if 0
    nbuf_check(nbuf);
#endif
    return NET_ERR_OK;
}

/* splice two buf,merge src to tail of des */
net_err_t nbuf_splice(nbuf_t * des, nbuf_t * src)
{
    list_splice_tail(&src->node, &des->node);
    des->total_size += src->total_size;
    mempool_free(&net_buf_pool, (void *)src);

    return NET_ERR_OK;
}

/* expand some size for constant tail */
net_err_t nbuf_tail_expand(nbuf_t * nbuf, uint16_t growth)
{
    net_err_t err;
    uint16_t free = 0;
    nbuf_blk_t * tail;

    /* if nbuf last block has empty zone,use it */
    if (!NBUF_EMPTY(nbuf))
    {
        /* update tail block parameter */
        tail = NBUF_BLOCK_ENTRY(nbuf->node.prev);
        free = nblock_tail_size(tail);

        tail->size += NMIN(free, growth);

        /* if free size is enough,use it and return */
        if (free >= growth)
        {
            goto _r_succeed;
        }
    }
    /* if not enough,alloc new block */
    err = nbuf_block_list_alloc(&nbuf, growth - free, INSERT_TAIL);
_r_succeed:
    nbuf->total_size += growth;
#if 0
    nbuf_check(nbuf);
#endif
    return err;
}

#define _NOT_FOR_USER
/* trim some size from tail, but you cannot trim whole buf */
_NOT_FOR_USER
net_err_t nbuf_tail_trim(nbuf_t * nbuf, uint16_t size)
{
    nbuf_blk_t * tail;
    nbuf_blk_t * tail_prev;
    int used_size, bk_size;

    if (!size)
        return NET_ERR_OK;

    if (size >= NBUF_TTSZ(nbuf))  
        return NET_ERR_NOK;

    bk_size = size;
    while (1) {
        tail = NBUF_BLOCK_ENTRY((nbuf->node).prev);
        /* if the last block has enough size to cut 
         * then cut last block and return
         */
        used_size = nblock_usedsz(tail);
        tail->size -= NMIN(size, used_size);
        size -= NMIN(size, used_size);
        if (!tail->size) {
            /* free the last block */
            nbuf_block_free(tail);
        }
        if (!size) {
            break;
        }
    }
    nbuf->total_size -= bk_size;
    nbuf_acc_reset(nbuf);
    return NET_ERR_OK;
}

/* extract prefix from nbuf as a constant access zone */
net_err_t nbuf_extract_prefix(nbuf_t * nbuf, uint16_t size)
{
    uint16_t remain = size;
    nbuf_blk_t * blk, * first;
    struct list_head * next;

    if ((size > nbuf->total_size) || (size > NETBUF_BLOCK_SIZE))
    {
        plat_printf("error: prefix size overflow\r\n");
        return NET_ERR_NOK;
    }
    first = blk = NBUF_BLOCK_ENTRY(NBUF_FIRST_BLKNODE(nbuf));
    next = blk->node.next;

    /* check if first block has enough data */
    if (blk->size >= size)
        return NET_ERR_OK;

    _memcpy(blk->data, (uint8_t *)blk->payload, blk->size);
    blk->payload = blk->data;

    while (remain -= blk->size)
    {
        blk = NBUF_BLOCK_ENTRY(next);
        next = next->next;

        _memcpy((uint8_t *)first->payload + first->size, blk->data, NMIN(remain, blk->size));
        first->size += NMIN(remain, blk->size);

        /* current block size larger than remain,cut block and return */
        if (remain < blk->size)
        {
            PAYLOAD_OFFSET(blk, remain);
            blk->size = blk->size - remain;
            break;
        }
        /* current block size little than remain,free block */
        nbuf_block_free(blk);
    }
    /* flush rw parameters */
    nbuf_seek(nbuf, nbuf->t_off);
#if 0
    nbuf_check(nbuf);
#endif
    return NET_ERR_OK;
}

/* calculate checksum with net buffer */
uint16_t nbuf_checksum16(nbuf_t * nbuf, uint16_t pre_chk)
{
    uint16_t total;
    struct list_head * node;
    uint16_t chksum = pre_chk;
    nbuf_blk_t * blk;
    uint16_t blk_size = 0;

    if (NBUF_EMPTY(nbuf))
    {
        plat_printf("error: nbuf empty!");
        return chksum;
    }

    total = NBUF_TOTAL_SIZE(nbuf);
    node = NBUF_FIRST_BLKNODE(nbuf);
    blk = NBUF_BLOCK_ENTRY(node);

    /* calculate checksum while no data */
    NBUF_LOOP_BEGIN
    blk_size = blk->size;

    // /* 这段逻辑不知道是否需要，先加上 2025/06/28 */
	// if ((blk_size & 0x01) && (total > blk->size)) {
    //     nbuf_extract_prefix(nbuf, blk_size + 1);
    //     blk_size = blk->size;
    // }
    
    /* calculate current block checksum */
    chksum =
        net_checksum16(chksum, blk->payload, blk->size,
                       (total <= blk->size) ? true : false);

    /* get next block */
    node = NBLOCK_NEXT(blk);
    blk = NBUF_BLOCK_ENTRY(node);
    NBUF_LOOP_END(total -= blk_size);

    return chksum;
}

/* specific memory copy function for nbuf */
static void
_memcpy(uint8_t * _tar, uint8_t * _src, uint16_t len)
{
    int i, temp;

    if ((!_tar) || (!_src) || (!len))
        return;

    /* calculate aligned unit size */
    temp = len / sizeof(size_t);

    /* copy the aligned data firstly */
    for (i = 0; i < temp; i ++)
        ((size_t *)_tar)[i] = ((size_t *)_src)[i];

    /* then calculate unaligned data size */
    i *= sizeof(size_t);

    /* copy the left scattered data */
    for (; i < len; i ++)
        _tar[i] = _src[i];
}

/* nbuf left size to read and write */
static inline uint16_t nbuf_left(nbuf_t *nbuf)
{
    return nbuf->total_size - nbuf->t_off;
}

/* reset nbuf read and write offset */
net_err_t nbuf_seek(nbuf_t * nbuf, uint16_t off)
{
    struct list_head * node;
    nbuf_blk_t * blk;
    int tmp = off;

    if (NBUF_EMPTY(nbuf))
    {
        plat_printf("error: nbuf empty!");
        return NET_ERR_NOK;
    }

    if ((off > NBUF_TTSZ(nbuf)) || (!nbuf))
        return NET_ERR_NOK;

    node = NBUF_FIRST_BLKNODE(nbuf);
    blk = NBUF_BLOCK_ENTRY(node);

    NBUF_LOOP_BEGIN
    if (tmp < blk->size)
    {
        if ((void *)blk == (void *)(&nbuf->node))
        {
            nbuf->cur_blk = NBUF_BLOCK_ENTRY(blk->node.prev);
            nbuf->t_off = off;
            nbuf->cur_off = nbuf->cur_blk->size;
        }
        else
        {
            nbuf->cur_blk = blk;
            nbuf->t_off = off;
            nbuf->cur_off = tmp;
        }
        break;
    }
    tmp -= blk->size;
    /* get next block */
    node = NBLOCK_NEXT(blk);
    blk = NBUF_BLOCK_ENTRY(node);
    NBUF_LOOP_END(tmp >= 0);

#if 0
    nbuf_check(nbuf);
#endif

    return NET_ERR_OK;
}

/* nbuf memory set value,like write */
net_err_t nbuf_memset(nbuf_t * nbuf, uint16_t size, uint8_t val)
{
    uint16_t rw;
    nbuf_blk_t * blk;
    struct list_head * node;
    uint16_t tmp_off;
    uint16_t tmp_size = size;

    /* if nbuf have enough size to write */
    if (nbuf_left(nbuf) < size)
    {
        plat_printf("error: nbuf left size not enough!\r\n");
        return NET_ERR_NOK;
    }

    blk = nbuf->cur_blk;
    tmp_off = nbuf->cur_off;

    /* copy data to nbuf */
    NBUF_LOOP_BEGIN
    /* current block left size to write */
    rw = NMIN(blk->size - tmp_off, size);

    plat_memset((uint8_t *)blk->payload + tmp_off, val, rw);

    /* get next block */
    node = NBLOCK_NEXT(blk);
    blk = NBUF_BLOCK_ENTRY(node);
    tmp_off &= 0;
    NBUF_LOOP_END(size -= rw);

    /* reset nbuf read and write offset */
    nbuf_seek(nbuf, nbuf->t_off + tmp_size);

#if 0
    nbuf_check(nbuf);
#endif

    return NET_ERR_OK;
}

/* copy data from constant buffer to nbuf */
net_err_t nbuf_write(nbuf_t * nbuf, void * data, uint16_t size)
{
    uint16_t bw;
    uint16_t d_off = 0;
    nbuf_blk_t * blk;
    struct list_head * node;
    uint16_t tmp_off;
    uint16_t tmp_size = size;

    /* if nbuf have enough size to write */
    if (nbuf_left(nbuf) < size)
    {
        plat_printf("error: nbuf left size not enough!\r\n");
        return NET_ERR_NOK;
    }

    blk = nbuf->cur_blk;
    tmp_off = nbuf->cur_off;

    /* copy data to nbuf */
    NBUF_LOOP_BEGIN
    /* current block left size to write */
    bw = NMIN(blk->size - tmp_off, size);

    _memcpy((uint8_t *)blk->payload + tmp_off, (uint8_t *)data + d_off, bw);
    d_off += bw;

    /* get next block */
    node = NBLOCK_NEXT(blk);
    blk = NBUF_BLOCK_ENTRY(node);
    tmp_off &= 0;
    NBUF_LOOP_END(size -= bw);

    /* reset nbuf read and write offset */
    nbuf_seek(nbuf, nbuf->t_off + tmp_size);

#if 0
    nbuf_check(nbuf);
#endif

    return NET_ERR_OK;
}

/* transfer nbuf to another nbuf */
net_err_t nbuf_transfer(nbuf_t * src, nbuf_t * dst,
                        uint16_t size,
                        uint16_t src_off, uint16_t dst_off)
{
    uint16_t bw;
    nbuf_blk_t * blk;
    struct list_head * node;
    uint16_t tmp_off;

    if ((NET_ERR_NOK == nbuf_seek(src, src_off)) ||
        (NET_ERR_NOK == nbuf_seek(dst, dst_off)))
    {
        plat_printf("error: nbuf seek error!\r\n");
        return NET_ERR_NOK;
    }

    /* if nbuf have enough size to write/read */
    if ((nbuf_left(src) < size) || (nbuf_left(dst) < size))
    {
        plat_printf("error: nbuf left size not enough!\r\n");
        return NET_ERR_NOK;
    }

    blk = src->cur_blk;
    tmp_off = src->cur_off;

    /* copy data to nbuf */
    NBUF_LOOP_BEGIN
    /* current block left size to write */
    bw = NMIN(blk->size - tmp_off, size);

    nbuf_write(dst, (void *)((uint8_t *)blk->payload + tmp_off), bw);

    /* get next block */
    node = NBLOCK_NEXT(blk);
    blk = NBUF_BLOCK_ENTRY(node);
    tmp_off &= 0;
    NBUF_LOOP_END(size -= bw);

    return NET_ERR_OK;
}

/* copy data from nbuf to constant buffer */
net_err_t nbuf_read(nbuf_t * nbuf, void * data, uint16_t size)
{
    uint16_t br;
    uint16_t d_off = 0;
    nbuf_blk_t * blk;
    struct list_head * node;
    uint16_t tmp_off;
    uint16_t tmp_size = size;

    /* if nbuf have enough size to write */
    if (nbuf_left(nbuf) < size)
    {
        plat_printf("error: nbuf left size not enough!\r\n");
        return NET_ERR_NOK;
    }

    blk = nbuf->cur_blk;
    tmp_off = nbuf->cur_off;

    /* copy data to nbuf */
    NBUF_LOOP_BEGIN
    /* current block left size to write */
    br = NMIN(blk->size - tmp_off, size);

    _memcpy((uint8_t *)data + d_off, (uint8_t *)blk->payload + tmp_off, br);
    d_off += br;

    /* get next block */
    node = NBLOCK_NEXT(blk);
    blk = NBUF_BLOCK_ENTRY(node);
    tmp_off &= 0;
    NBUF_LOOP_END(size -= br);

    /* reset nbuf read and write offset */
    nbuf_seek(nbuf, nbuf->t_off + tmp_size);

#if 0
    nbuf_check(nbuf);
#endif

    return NET_ERR_OK;
}

/* reset nbuf read and write offset */
net_err_t nbuf_acc_reset(nbuf_t * nbuf)
{
    return nbuf_seek(nbuf, 0);
}

#if NBUF_USAGE_STATISTICS == 1
void nbuf_get_statistics(uint8_t * u1, uint8_t * u2, uint8_t * u3, uint8_t * u4)
{
    *u1 = (nbuf_blk_used * 100) / NETBUF_BLOCK_NUM;
    *u2 = ((nbuf_blk_used * 100) % NETBUF_BLOCK_NUM) * 100 / NETBUF_BLOCK_NUM;
    plat_printf(" nbuf statistics : usage rate %d.%d%%\r\n", *u1, *u2);

    *u3 = (nbuf_header_used * 100) / NETBUF_PACKET_NUM;
    *u4 = ((nbuf_header_used * 100) % NETBUF_PACKET_NUM) * 100 / NETBUF_PACKET_NUM;
    plat_printf(" nbuf header statistics : usage rate %d.%d%%\r\n", *u3, *u4);
}
#endif

/* 20250613理论分析后认为这两个接口不安全 */
/* APIs adapted for interrupt */
net_err_t nbuf_alloc_from_isr(nbuf_t ** nbuf, uint16_t size)
{
    
}

net_err_t  nbuf_write_from_isr(nbuf_t * nbuf, void * data, uint16_t size)
{

}