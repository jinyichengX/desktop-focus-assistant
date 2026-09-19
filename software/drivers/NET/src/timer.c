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

#include "timer.h"
#include "global.h"

sys_tick_t net_sys_tick = 0;

/* timer list init */
net_err_t net_timer_init(void)
{
  /* init timer list */
  INIT_LIST_HEAD(&timer_list);
  INIT_LIST_HEAD(&timer_proc_pend_list);

  net_sys_tick = sys_tick_now();

  return NET_ERR_OK;
}

/* insert timer to timer list */
static void net_timer_insert(timer_t * timer)
{
    timer_t * tmp;
    timer_node_t * pos;
    
    /* insert to timer list */
    list_for_each(pos, &timer_list){
        tmp = list_entry(pos, timer_t, node);
        if(timer->timeout_tick < tmp->timeout_tick){
            tmp->timeout_tick -= timer->timeout_tick;
            __list_add(&timer->node, pos->prev, pos);
            return;
        }
        timer->timeout_tick -= tmp->timeout_tick;
    }
    list_add_tail(&timer->node, &timer_list);
}

/* remove timer from timer list */
static void net_timer_remove(timer_t * timer)
{
    timer_t * next;

    if(!timer) return;

    if(timer->node.next != &timer_list){
        next = list_entry(timer->node.next, timer_t, node);
        next->timeout_tick += timer->timeout_tick;
    }

    /* remove from timer list */
    list_del(&timer->node);
}

/* init timer */
static void net_timerst_init(timer_t * timer, timer_proc_t proc,\
                            uint32_t reload_cnt, uint32_t reload, void * args)
{
    timer->args = args;
    timer->proc = proc;
    timer->reload = reload;
    timer->timeout_tick = reload;
    timer->reload_cnt = reload_cnt;
    INIT_LIST_HEAD(&timer->node);
}

/* add timer */
net_err_t net_timer_add(timer_t * timer, timer_proc_t proc,\
                        uint32_t reload_cnt, uint32_t reload, void * args)
{
    /* init timer */
    net_timerst_init(timer, proc, reload_cnt, reload, args);

    /* insert to timer list */
    net_timer_insert(timer);

    return NET_ERR_OK;
}

/* delete timer */
net_err_t net_timer_del(timer_t * timer)
{
    /* delete from timer list */
    net_timer_remove(timer);

    return NET_ERR_NOK;
}

/* reload timer */
void net_timer_reload(timer_t * timer)
{
    /* insert to timer list */
    if((timer->reload_cnt == RELOAD_FOREVER)||(--timer->reload_cnt)){
        timer->timeout_tick = timer->reload;
        net_timer_insert(timer);
    }
}

/* loop timer process call in thread */
void net_timer_process(uint32_t tick_gone)
{
    timer_t * timer;
    timer_node_t * pos,* tmp;
    /* if timer list have timeout timer */
    /* if have, remove and add to pend list */
    list_for_each_safe(pos, tmp, &timer_list){
        timer = list_entry(pos, timer_t, node);
        if(tick_gone < timer->timeout_tick){
            timer->timeout_tick -= tick_gone;
            break;
        }
        tick_gone = tick_gone - timer->timeout_tick;

        list_del(&timer->node);

        /* add to pend list */
        list_add_tail(&timer->node, &timer_proc_pend_list);
    }

    /* if have timeout timer,call it's proc */
    list_for_each_safe(pos, tmp, &timer_proc_pend_list){
        timer = list_entry(pos, timer_t, node);

        /* call proc */
        if(timer->proc != NULL)
            timer->proc(timer, timer->args);

        list_del(&timer->node);

        /* if reload, add to timer list */
        net_timer_reload(timer);
    }
}

/* get first timer timeout tick */
uint32_t net_timer_first_tmo(void)
{
    timer_t * timer;
    timer_node_t * node;
    
    if(!list_empty(&timer_list)){
        node = timer_list.next;
        timer = list_entry(node, timer_t, node);
        return timer->timeout_tick;
    }

    return 0;
}