/*
 * MIT License
 *
 * Copyright (c) 2025 JinYiCheng
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

#include "ipgui_membox.h"
//#include "ipgui_memory.h"
#include <stdlib.h>

#define BLK_SZ_MIN1 (((sizeof(blk_node_t)) + IPGUI_MEM_ALIGN_SIZE - 1) & (~(ipgui_mem_unit_type_t)IPGUI_MEM_ALIGN_SIZE_MASK))

/* init membox */
__IPGUI_API__ ipgui_err_t ipgui_membox_init(ipgui_membox_t * mb, void * mem, unsigned short blk_sz, int blk_cnt)
{
    int idx;
    blk_node_t * iblock;

    if ((NULL == mem) || (blk_cnt < 1)) {
        //ipgui_debug_error("error: invalid membox param\r\n");
        return IPGUI_ERR_PARAM;
    }

    if (blk_sz < BLK_SZ_MIN1) {
        //ipgui_debug_warning("warning: block size is too little\r\n");
        blk_cnt = (blk_sz * blk_cnt) / BLK_SZ_MIN1;
        blk_sz = BLK_SZ_MIN1;
    }

    mb->block.next = NULL;
    iblock = &mb->block;
    iblock->next = (blk_node_t *)mem;
    for (idx = 0; idx < blk_cnt - 1; idx ++) {
        iblock = (blk_node_t *)((unsigned char *)mem + idx * blk_sz);
        iblock->next = (blk_node_t *)((unsigned char *)iblock + blk_sz);
    }

    mb->blk_cnt = blk_cnt;
    mb->blk_sz = blk_sz;
    mb->blk_used = 0;

    return IPGUI_ERR_OK;
}

///* create membox */
//__IPGUI_API__ ipgui_membox_t * ipgui_membox_create(unsigned short blk_sz, int blk_cnt)
//{
//    void * p = (void *)0;
//    if ((void *)0 == (p = ipgui_mem_alloc(ipgui_smem, sizeof(ipgui_membox_t) + blk_sz * blk_cnt)))
//        return (void *)0;

//    if (IPGUI_ERR_OK != ipgui_membox_init((ipgui_membox_t *)p, (void *)((char *)p + sizeof(ipgui_membox_t)), blk_sz, blk_cnt))
//        ipgui_mem_free(ipgui_smem, p);

//    return (ipgui_membox_t *)p;
//}

///* destroy membox */
//__IPGUI_API__ void ipgui_membox_destroy(ipgui_membox_t * mb)
//{
//    ipgui_mem_free(ipgui_smem, (void *)mb);
//}

/* allocate one block */
__IPGUI_API__ void * ipgui_membox_alloc(ipgui_membox_t * mb)
{
    void * block;

    if (mb->blk_used >= mb->blk_cnt)
        return (void *)0;

    block = (void *)mb->block.next;
    mb->block.next = ((blk_node_t *)block)->next;
    mb->blk_used ++;

    return block;
}

/* free one block */
__IPGUI_API__ void ipgui_membox_free(ipgui_membox_t * mb, void * addr)
{
    if (!addr)
        return;
    
    ((blk_node_t *)addr)->next = mb->block.next;
    mb->block.next = (blk_node_t *)addr;

    mb->blk_used --;
}

__IPGUI_API__ void ipgui_membox_expand(ipgui_membox_t * mb, void * mem, unsigned short blk_sz, int blk_cnt)
{
    if (!mem) return;
}
