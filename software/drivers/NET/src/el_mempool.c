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

#include "el_mempool.h"
#include "elnet.h"
#include "defs.h"
#include "dbg.h"

#define BLK_SZ_MIN MEM_ALIGN_UP(sizeof(block_node_t),4)

/*create mempool and initialize*/
net_err_t mempool_create(mempool_t * mp, void * mem, uint16_t blk_sz, int blk_cnt)
{
    int idx;
    block_node_t * iblock;

    if((NULL == mem) || (blk_cnt < 1)){
        plat_printf("error: invalid mempool param\r\n");
        return NET_ERR_NOK;
    }

    if(blk_sz < BLK_SZ_MIN){
        plat_printf("warning: block size is too little\r\n");
        blk_cnt = (blk_sz * blk_cnt)/BLK_SZ_MIN;
        blk_sz = BLK_SZ_MIN;
    }

    /* init control block */
    mp->block.next = NULL;

    /* link block */
    iblock = &mp->block;
    iblock->next = (block_node_t *)mem;
    for(idx = 0; idx < blk_cnt - 1; idx ++){
        iblock = (block_node_t *)((uint8_t *)mem + idx * blk_sz);
        iblock->next = (block_node_t *)((uint8_t *)iblock + blk_sz);
    }

    /* create and initialize pool mutex */
    if(SYS_MUTEX_INVALID == (mp->mutex = sys_mutex_create())){
        plat_printf("error: pool sem create err\r\n");
        return NET_ERR_NOK;
    }

    /* create and initialize block sem*/
    if(SYS_SEM_INVALID == (mp->sem = sys_sem_create(blk_cnt))){
        sys_mutex_destroy(mp->mutex);
        plat_printf("error: pool sem create err\r\n");
        return NET_ERR_NOK;
    }

    mp->blk_max = blk_cnt;
    mp->blk_sz = blk_sz;
    mp->blk_used = 0;

    return NET_ERR_OK;
}

/* destroy mempool */
void mempool_destroy(mempool_t * mp)
{
    if(!mp)
        return;

    /* destroy pool mutex */
    if(SYS_MUTEX_INVALID != mp->mutex){
        sys_mutex_destroy(mp->mutex);
        mp->mutex = SYS_MUTEX_INVALID;
    }

    /* destroy block sem*/
    if(SYS_SEM_INVALID != mp->sem){
        sys_sem_destroy(mp->sem);
        mp->sem = SYS_SEM_INVALID;
    }

    mp->block.next = NULL;
    mp->blk_sz = 0;
}

/* allocate from pool */
void * mempool_alloc(mempool_t * mp, uint32_t timeout_tick)
{
    void * block;
    
    /* wait sem */
    if(0 != sys_sem_take(mp->sem, timeout_tick))
        return NULL;

    /* lock pool,remove first node */
    sys_mutex_lock(mp->mutex);
    block = (void *)mp->block.next;
    mp->block.next = ((block_node_t *)block)->next;
    sys_mutex_unlock(mp->mutex);

    mp->blk_used ++;

    /* return first block addr */
    return block;
}

/* free one block */
void mempool_free(mempool_t * mp, void * addr)
{
    if(!addr)
        return;
    
    /* lock pool,add node */
    sys_mutex_lock(mp->mutex);
    ((block_node_t *)addr)->next = mp->block.next;
    mp->block.next = (block_node_t *)addr;
    sys_mutex_unlock(mp->mutex);
    
    /* release sem */
    sys_sem_release(mp->sem);

    mp->blk_used --;
}

/* APIs adapated for interrupt start here */
net_err_t mempool_create_for_isr(mempool_t * mp, void * mem, uint16_t blk_sz, int blk_cnt)
{
    int idx;
    block_node_t * iblock;

    if((NULL == mem) || (blk_cnt < 1)){
        plat_printf("error: invalid mempool param\r\n");
        return NET_ERR_NOK;
    }

    if(blk_sz < BLK_SZ_MIN){
        plat_printf("warning: block size is too little\r\n");
        blk_cnt = (blk_sz * blk_cnt)/BLK_SZ_MIN;
        blk_sz = BLK_SZ_MIN;
    }

    /* init control block */
    mp->block.next = NULL;

    /* link block */
    iblock = &mp->block;
    iblock->next = (block_node_t *)mem;
    for(idx = 0; idx < blk_cnt - 1; idx ++){
        iblock = (block_node_t *)((uint8_t *)mem + idx * blk_sz);
        iblock->next = (block_node_t *)((uint8_t *)iblock + blk_sz);
    }

    mp->blk_max = blk_cnt;
    mp->blk_sz = blk_sz;
    mp->blk_used = 0;

    return NET_ERR_OK;
}

//void *  mempool_alloc_from_isr(mempool_t * mp)
//{
//    size_t isr_flag;
//    void * block = NULL;
//    
//    /* disable isr */
//    SAVE_AND_DISABLE_INTERRUPTS(isr_flag);

//    if( mp->blk_used == mp->blk_max )
//        goto _return;

//    block = (void *)mp->block.next;
//    mp->block.next = ((block_node_t *)block)->next;
//    mp->blk_used ++;

//_return:
//    /* enable isr */
//    RESTORE_INTERRUPTS(isr_flag);

//    /* return first block addr */
//    return block;
//}

//void mempool_free_from_isr(mempool_t * mp, void * addr)
//{
//    if(!addr)
//        return;

//    /* disable isr */
//    SAVE_AND_DISABLE_INTERRUPTS(isr_flag);

//    ((block_node_t *)addr)->next = mp->block.next;
//    mp->block.next = (block_node_t *)addr;
//    mp->blk_used --;

//    /* enable isr */
//    RESTORE_INTERRUPTS(isr_flag);
//}