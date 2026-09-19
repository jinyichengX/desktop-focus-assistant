#ifndef TIMER_H
#define TIMER_H

#include "el_nlist.h"
#include "el_smlist.h"
#include "elnet.h"
#include <stdint.h>

#define RELOAD_FOREVER 0xffffffff

typedef struct _timer timer_t;
typedef struct list_head timer_list_t;
typedef struct list_head timer_node_t;

typedef void (*timer_proc_t)(timer_t * timer, void * args);

typedef struct _timer
{
  uint32_t reload;            /* 重载值 */
  uint32_t reload_cnt;        /* 重载次数 */
  uint32_t timeout_tick;      /* 超时时间 */
  timer_node_t node;          /* 钩子节点 */
  timer_proc_t proc;          /* 定时器回调函数 */
  void * args;                /* 私有参数 */
}timer_t;

extern sys_tick_t net_sys_tick;

extern net_err_t net_timer_init(void);
extern net_err_t net_timer_add(timer_t * timer, timer_proc_t proc,\
                        uint32_t reload_cnt, uint32_t reload, void * args);
extern net_err_t net_timer_del(timer_t * timer);
extern void net_timer_process(uint32_t tick_gone);
extern uint32_t net_timer_first_tmo(void);
#endif