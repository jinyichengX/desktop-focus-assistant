#ifndef GLOBAL_H
#define GLOBAL_H

#include "el_mempool.h"
#include "el_nconfig.h"
#include "el_netif.h"
#include "el_ipv4.h"
#include "timer.h"
#include "el_nbuf.h"
#include "el_arp.h"
#include "plat.h"
#include "el_ndefs.h"
#include "el_udp.h"
#include "el_tcp.h"
#include "sock.h"
#include "hash.h"

#define G_HEAP_SIZE         (20 * 1024)

#define QUADRUPLE_SIZE      (2 * sizeof(endpoint_t))

extern  char                g_heap[G_HEAP_SIZE];
extern  char                net_endian_large;
extern  mempool_t           netif_pool;
extern  netif_list_t        netif_list;
extern  sys_mutex_t         netif_list_lock;
extern  netif_t             netif_class[NETIF_MAXIUM_NUM];
extern  timer_t             netif_bandwidth_statistic_timer;
extern  mempool_t           route_entry_pool;
extern  route_entry_list_t  route_entry_list;
extern  sys_mutex_t         route_entry_list_lock;
extern  route_entry_t       route_entry[IP_ROUTE_ENTRY_NUM];
extern  timer_list_t        timer_list;
extern  timer_list_t        timer_proc_pend_list;
extern  mempool_t           arp_entry_pool;
extern  arp_entry_t         arp_table[ARP_ENTRY_NUM];
extern  arp_entry_list_t    arp_entry_list;
extern  sys_mutex_t         arp_entry_list_lock;
extern  timer_t             arp_timer;
extern  ipv4_reass_list_t   ip_reass_list;
extern  sys_mutex_t         ip_reass_list_lock;
extern  mempool_t           ipv4_reass_pool;
extern  ipv4_reass_t        ipv4_reass_obj_pool[IP_PACKET_NEED_REASSEMBLED_NUM];
extern  mempool_t           ipv4_frag_pool;
extern  ipv4_frag_t         ipv4_frag_obj_pool[IP_FRAGMENT_SUPPORTED_NUM];
extern  timer_t             ipv4_timer;
extern  uint16_t            ip_identifier;
extern  mempool_t           udp_cb_pool;
extern  udp_t               udp_cb_obj_pool[UDP_MAX_CONNECTION_NUM];
extern  udp_cb_list_t       udp_cb_list;
extern  sys_mutex_t         udp_hash_table_lock;
extern  hash_tbl_t          udp_cb_hash_tbl;
extern  tcp_cb_list_t       tcp_established_list;
extern  tcp_cb_list_t       tcp_syn_list;
extern  mempool_t           tcp_cb_pool;
extern  tcp_cb_t            tcp_cb[TCP_MAX_CONNECTION_NUM];
extern  const uint8_t       ether_broadcast_addr[ETHER_MAC_ADDR_LEN];
extern  const uint8_t       ether_addr_any[ETHER_MAC_ADDR_LEN];
extern  mempool_t           net_buf_pool;
extern  mempool_t           nbuf_block_pool;
extern  nbuf_blk_t          nbuf_blk[NETBUF_BLOCK_NUM];
extern  nbuf_t              nbuf_manager[NETBUF_PACKET_NUM];
extern  sys_thread_t        net_thread;
extern  queue_t             k_msg_queue;
extern  uint8_t             k_msg_buf[KQ_PENDING_NUM * sizeof(void *)];
extern  sys_thread_t        async_thread_handler;
extern  queue_t             async_handler_queue;
extern  uint8_t             async_handler_pool[ASYNC_MQ_ITEM_NUM * sizeof(void *)];

#endif