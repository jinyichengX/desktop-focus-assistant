#ifndef QUEUE_H
#define QUEUE_H

#include "plat.h"
#include "el_nlist.h"
#include <stdlib.h>
#include "elnet.h"

/* queue */
typedef struct msq_queue
{
  uint16_t head;        /* index to read */
  uint16_t tail;        /* index to write */  
  uint16_t length;      /* valid data num */

  uint8_t * pool;

  uint16_t item_cnt;
  uint16_t item_size;

  sys_mutex_t mutex;
  sys_sem_t not_full;
  sys_sem_t not_empty;
}queue_t;

/* queue v2 */
typedef struct msg_queue_v2
{
  struct list_head msg_list;
  sys_mutex_t mutex;
  sys_sem_t sem;
}queue_v2_t;

/* queue operations */
extern net_err_t queue_init(queue_t * q, void * pool, uint16_t item_cnt, uint16_t item_size);
extern net_err_t queue_destroy(queue_t * q);
extern net_err_t queue_trypost(queue_t * q, void * item, uint16_t size);
extern net_err_t queue_post(queue_t * q, void * item, uint16_t size, uint32_t tmo);
extern net_err_t queue_post_ptr(queue_t * q, void * item, uint32_t tmo);
extern net_err_t queue_tryfetch(queue_t * q, void * item);
extern net_err_t queue_fetch(queue_t * q, void * item, uint32_t tmo);
extern net_err_t queue_fetch_ptr(queue_t * q, void ** item, uint32_t tmo);

/* queue v2 operations */
extern net_err_t queuev2_init(queue_v2_t * queue);
extern net_err_t queuev2_post(queue_v2_t * queue, struct list_head * msg_node);
extern net_err_t queuev2_pend(queue_v2_t * queue, struct list_head * msg_node, uint32_t tmo);

/* queue v3 operations */
// extern net_err_t queuev3_init(queue_v3_t * q);

#endif