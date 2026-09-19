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

#include "queue.h"
#include "dbg.h"

#define QUEUE_HEAD_ITEM_PTR(q) (q->pool + (q->head * q->item_size))
#define QUEUE_HEAD_PREV_ITEM_PTR(q) (q->pool + ((q->head - 1) * q->item_size))
#define QUEUE_TAIL_ITEM_PTR(q) (q->pool + (q->tail * q->item_size))

/* queue lock and unlock */
static inline void queue_lock(queue_t * q)
{
    sys_mutex_lock(q->mutex);
}

static inline void queue_unlock(queue_t * q)
{
    sys_mutex_unlock(q->mutex);
}

/* increase queue length */
static inline void queue_item_increase(queue_t * q)
{
    q->tail = (q->tail + 1) % q->item_cnt;
    q->length ++;
}
static inline void queue_item_increase_front(queue_t * q)
{
    if( q->head == 0 )
        q->head = q->item_cnt - 1;
    else
        q->head --;
    q->length ++;
}

/* decrease queue length */
static inline void queue_item_decrease(queue_t * q)
{
    q->head = (q->head + 1) % q->item_cnt;
    q->length --;
}

/* write queue item */
static inline void queue_item_write(queue_t * q, void * src)
{
    plat_memcpy(QUEUE_TAIL_ITEM_PTR(q), src, q->item_size);
}
/* write item in front of queue */
static inline void queue_item_write_front(queue_t * q, void * src)
{
    plat_memcpy(QUEUE_HEAD_PREV_ITEM_PTR(q), src, q->item_size);
}

/* read queue item */
static inline void queue_item_read(queue_t * q, void * des)
{
    plat_memcpy(des, QUEUE_HEAD_ITEM_PTR(q), q->item_size);
}

/* check if queue is full */
static inline int queue_full(queue_t * q)
{
    return (q->length == q->item_cnt);
}

/* check if queue is empty */
static inline int queue_empty(queue_t * q)
{
    return (q->length == 0);
}

/* init queue */
/* 为确保程序的鲁棒性最好从全局的对象池中申请ipc对象 */
net_err_t queue_init(queue_t * q, void * pool, uint16_t item_cnt, uint16_t item_size)
{
    net_err_t err = NET_ERR_OK;

    q->mutex = SYS_MUTEX_INVALID;
    q->not_empty = SYS_SEM_INVALID;
    q->not_full = SYS_SEM_INVALID;

    /* create access lock */
    if( SYS_MUTEX_INVALID == (q->mutex = sys_mutex_create()) ){
        err = NET_ERR_NOK;
        goto __return_nok;
    }

    /* create sem for empty and full */
    if( SYS_SEM_INVALID == (q->not_empty = sys_sem_create(0)) ){
        err = NET_ERR_NOK;
        goto __return_nok;
    }
    if( SYS_SEM_INVALID == (q->not_full = sys_sem_create(item_cnt)) ){
        err = NET_ERR_NOK;
        goto __return_nok;
    }

    /* init queue */
    q->length = 0;
    q->head = q->tail = 0;
    q->item_cnt = item_cnt;
    q->item_size = item_size;
    q->pool = (uint8_t *)pool;

    return NET_ERR_OK;
__return_nok:
    if( q->mutex ){
        sys_mutex_destroy(q->mutex);
    }
    if( q->not_full ){
        sys_sem_destroy(q->not_full);
    }
    if( q->not_empty ){
        sys_sem_destroy(q->not_empty);
    }
    return err;
}

#if 0
/* create queue */
net_err_t queue_create(queue_t * q, uint16_t item_cnt, uint16_t item_size)
{
    void * pool = NULL;

    /* allocate queue pool */
    if(NULL == (pool = malloc(item_cnt * item_size))){
        return NET_ERR_NOK;
    }
    
    /* init queue */
    if(NET_ERR_NOK == queue_init(q, pool, item_cnt, item_size)){
        free(pool);
        return NET_ERR_NOK;
    }

    return NET_ERR_OK;
}
#endif

/* destroy queue */
net_err_t queue_destroy(queue_t * q)
{
    /* if queue is not empty, return error */
    if( !queue_empty(q) ){
        return NET_ERR_NOK;
    }
    /* free queue pool */
    ;//do nothing
    /* destroy ipc */
    sys_mutex_destroy(q->mutex);
    sys_sem_destroy(q->not_full);
    sys_sem_destroy(q->not_empty);

    return NET_ERR_OK;
}

/* try post message */
net_err_t queue_trypost(queue_t * q, void * item, uint16_t size)
{
    if( size != q->item_size )
        return NET_ERR_NOK;

    /* lock queue */
    sys_mutex_lock(q->mutex);
    if( queue_full(q) ){
        sys_mutex_unlock(q->mutex);
        return NET_ERR_NOK;
    }
    if( (ret_t)0 !=sys_sem_take(q->not_full, 0) ){
        sys_mutex_unlock(q->mutex);
        return NET_ERR_NOK;
    }
    /* post message */
    queue_item_write(q, item);
    queue_item_increase(q);
    sys_mutex_unlock(q->mutex);

    sys_sem_release(q->not_empty);

    return NET_ERR_OK;
}

/* post message */
net_err_t queue_post(queue_t * q, void * item, uint16_t size, uint32_t tmo)
{
    if( size != q->item_size )
        return NET_ERR_NOK;

    /* wait not full sem */
    if( (ret_t)0 != sys_sem_take(q->not_full, tmo) )
        return NET_ERR_NOK;

    /* lock queue */
    sys_mutex_lock(q->mutex);
    /* post message */
    queue_item_write(q, item);
    /* update queue length and tail pointer */
    queue_item_increase(q);
    /* unlock queue */
    sys_mutex_unlock(q->mutex);

    sys_sem_release(q->not_empty);

    return NET_ERR_OK;
}

/* post message pointer */
net_err_t queue_post_ptr(queue_t * q, void * item, uint32_t tmo)
{
    return queue_post(q, &item, q->item_size, tmo);
}

/* simulate priority queue */
net_err_t queue_post_front(queue_t * q, void * item, uint16_t size, uint32_t tmo)
{
    if( size != q->item_size )
        return NET_ERR_NOK;

    /* wait not full sem */
    if( (ret_t)0 != sys_sem_take(q->not_full, tmo) )
        return NET_ERR_NOK;

    /* lock queue */
    sys_mutex_lock(q->mutex);
    /* post message in front */
    queue_item_write_front(q, item);
    /* update queue length and tail pointer */
    queue_item_increase_front(q);
    /* unlock queue */
    sys_mutex_unlock(q->mutex);

    sys_sem_release(q->not_empty);

    return NET_ERR_OK;
}

net_err_t queue_post_front_ptr(queue_t * q, void * item, uint32_t tmo)
{
    return queue_post_front(q, &item, q->item_size, tmo);
}

/* try fetch message */
net_err_t queue_tryfetch(queue_t * q, void * item)
{
    /* lock queue */
    sys_mutex_lock(q->mutex);
    if( queue_empty(q) ){
        sys_mutex_unlock(q->mutex);
        return NET_ERR_NOK;
    }
    if( (ret_t)0 !=sys_sem_take(q->not_empty, 0) ){
        sys_mutex_unlock(q->mutex);
        return NET_ERR_NOK;
    }
    /* post message */
    queue_item_read(q, item);
    queue_item_decrease(q);
    sys_mutex_unlock(q->mutex);

    sys_sem_release(q->not_full);

    return NET_ERR_OK;
}

/* fetch message */
net_err_t queue_fetch(queue_t * q, void * item, uint32_t tmo)
{
    /* wait not empty sem */
    if( (ret_t)0 !=sys_sem_take(q->not_empty, tmo) )
        return NET_ERR_NOK;
    
    /* lock queue */
    sys_mutex_lock(q->mutex);
    /* fetch message */
    queue_item_read(q, item);
    /* update queue length and head pointer */
    queue_item_decrease(q);
    /* unlock queue */
    sys_mutex_unlock(q->mutex);

    sys_sem_release(q->not_full);

    return NET_ERR_OK;
}

/* fetch message pointer */
net_err_t queue_fetch_ptr(queue_t * q, void ** item, uint32_t tmo)
{
    return queue_fetch(q, item, tmo);
}

/* APIs adapted for interrupt */
net_err_t queue_post_from_isr(queue_t * q, void * item, uint16_t size)
{

}

net_err_t queue_post_ptr_from_isr(queue_t * q, void ** item, uint16_t size)
{

}

net_err_t queue_fetch_from_isr(queue_t * q, void * item)
{
    
}

net_err_t queue_fetch_ptr_from_isr(queue_t * q, void ** item)
{
    
}

/* init queue */
/* 同上问题 */
net_err_t queuev2_init(queue_v2_t * queue)
{
    /* create access lock */
    if( NULL == (queue->mutex = sys_mutex_create()) ){
        goto __return_nok;
    }

    /* create sem to record queue length */
    if( NULL == (queue->sem = sys_sem_create(0)) ){
        goto __return_nok;
    }

    INIT_LIST_HEAD(&queue->msg_list);

    return NET_ERR_OK;
__return_nok:
    if( queue->mutex ){
        sys_mutex_destroy(queue->mutex);
    }
    if( queue->sem ){
        sys_sem_destroy(queue->sem);
    }
    return NET_ERR_NOK;
}

/* post message */
net_err_t queuev2_post(queue_v2_t * queue, struct list_head * msg_node)
{
    sys_mutex_lock(queue->mutex);
    list_add_tail(msg_node, &queue->msg_list);
    sys_mutex_unlock(queue->mutex);

    sys_sem_release(queue->sem);

    return NET_ERR_OK;
}

/* fetch message */
net_err_t queuev2_pend(queue_v2_t * queue, struct list_head * msg_node, uint32_t tmo)
{
    msg_node = NULL;

    if( (ret_t)0 !=sys_sem_take(queue->sem, tmo) )
        return NET_ERR_NOK;
    
    sys_mutex_lock(queue->mutex);
    msg_node = queue->msg_list.next;
    list_del(queue->msg_list.next);   

    sys_mutex_unlock(queue->mutex);

    return NET_ERR_OK;
}