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

#include "global.h"

/* global platform endian */
char                            net_endian_large = 0;

/* global heap */
char                            g_heap[G_HEAP_SIZE];

/* net buf relevant */
mempool_t                       net_buf_pool;
nbuf_t                          nbuf_manager[NETBUF_PACKET_NUM];                //15个控制块

mempool_t                       nbuf_block_pool;
nbuf_blk_t                      nbuf_blk[NETBUF_BLOCK_NUM];                     //100*（512+头部）

/* global netif mempool manager */
mempool_t                       netif_pool;

/* registed netif list */
netif_list_t                    netif_list;

/* global route table access lock */
sys_mutex_t                     netif_list_lock = NULL;

/* global netif object mempool */
netif_t                         netif_class[NETIF_MAXIUM_NUM];

/* global netif bandwidth statistic timer */
timer_t                         netif_bandwidth_statistic_timer;

/* global broadcast ethernet hardware addr */
const uint8_t                   ether_broadcast_addr[ETHER_MAC_ADDR_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff};
const uint8_t                   ether_addr_any[ETHER_MAC_ADDR_LEN] = {0, 0, 0, 0, 0, 0};

/* global ip route entry mempool manager */
mempool_t                       route_entry_pool;

/* global registed route entry list */
route_entry_list_t              route_entry_list;

/* global route table access lock */
sys_mutex_t                     route_entry_list_lock = NULL;

/* global ip route entry object mempool */
route_entry_t                   route_entry[IP_ROUTE_ENTRY_NUM];

/* gloabl ARP entry mempool manager */
mempool_t                       arp_entry_pool;

/* global ARP entry object mempool */
arp_entry_t                     arp_table[ARP_ENTRY_NUM];

/* global ARP entry list */
arp_entry_list_t                arp_entry_list;

/* global ARP entry list access lock */
sys_mutex_t                     arp_entry_list_lock = NULL;

/* global ARP timeout callback timer */
timer_t                         arp_timer;

/* global ipv4 reass list */
ipv4_reass_list_t               ip_reass_list;

/* global ip reass list access lock */
sys_mutex_t                     ip_reass_list_lock = NULL;

/* global ip reass mempool manager */
mempool_t                       ipv4_reass_pool;

/* global ip reass mempool */
ipv4_reass_t                    ipv4_reass_obj_pool[IP_PACKET_NEED_REASSEMBLED_NUM] = {0};

/* global ip frag mempool manager */
mempool_t                       ipv4_frag_pool;

/* global ip fragment mempool */
ipv4_frag_t                     ipv4_frag_obj_pool[IP_FRAGMENT_SUPPORTED_NUM] = {0};

/* global ip timeout callback timer */
timer_t                         ipv4_timer;

/* global ip identifier counter */
uint16_t                        ip_identifier = 0;       

/* global udp control block mempool */
mempool_t                       udp_cb_pool;

udp_t                           udp_cb_obj_pool[UDP_MAX_CONNECTION_NUM];
/* global udp control block list */
udp_cb_list_t                   udp_cb_list;/* 弃用，使用哈希表管理udp连接 */

sys_mutex_t                     udp_hash_table_lock = NULL;
hash_tbl_t                      udp_cb_hash_tbl;

/* global tcp control block list */
tcp_cb_list_t                   tcp_established_list;
tcp_cb_list_t                   tcp_syn_list;

mempool_t                       tcp_cb_pool;

/* gloabl tcp control block */
tcp_cb_t                        tcp_cb[TCP_MAX_CONNECTION_NUM];

/* gloabl timer list */
timer_list_t                    timer_list;

/* gloabl timer proc pend list */
timer_list_t                    timer_proc_pend_list;

/* kernel */
sys_thread_t                    net_thread;
queue_t                         k_msg_queue;
uint8_t                         k_msg_buf[KQ_PENDING_NUM * sizeof(void *)];

sys_thread_t                    async_thread_handler;
queue_t                         async_handler_queue;
uint8_t                         async_handler_pool[ASYNC_MQ_ITEM_NUM * sizeof(void *)];
