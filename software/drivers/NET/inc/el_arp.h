#ifndef EL_ARP_H
#define EL_ARP_H

#include "el_ndefs.h"
#include "el_netif.h"
#include "el_nlist.h"
#include "el_nbuf.h"
#include "plat.h"
#include "timer.h"

#pragma pack(1)
typedef struct
{
	uint16_t hw_type;				    /* 硬件类型 */
	uint16_t prot_type;					/* 协议类型 */
	uint8_t	 hw_size;					/* 硬件长度 */
	uint8_t  prot_size;					/* 协议长度 */
	uint16_t opcode;					/* 操作码 */
	uint8_t	 s_mac[ETHER_MAC_ADDR_LEN];	/* 源MAC */
	ip4addr_t s_ip;						/* 源IP */
	uint8_t	 d_mac[ETHER_MAC_ADDR_LEN];	/* 目的MAC */
	ip4addr_t d_ip;						/* 目的IP */
}arp_hdr_t; 
#pragma()

typedef enum {
    ARP_ENTRY_FREE,                     /* 空闲 */
    ARP_ENTRY_PENDING,                  /* 等待arp请求回应 */
    ARP_ENTRY_USING,                    /* 使用中 */
}arp_entry_state_t;

typedef enum{
    ARP_ENTRY_DYNAMIC,
    ARP_ENTRY_STATIC,
}arp_type_t;

typedef struct
{
	uint8_t	 hwa[ETHER_MAC_ADDR_LEN];	/* 源MAC */
	ip4addr_t ip;					    /* 源IP */
	netif_t * netif;					/* 网络接口 */
	arp_type_t type;					/* 类型（静态/动态） */

    arp_entry_state_t state;            /* 表状态 */

    struct list_head entry_node;        /* 挂载ARP表项的节点 */
    struct list_head buf_pend;          /* 挂载网络数据包的节点 */

    int t_alive;                        /* arp表保活时间 */
    sys_sem_t sem;                      /* 通知ARP请求回应到达 */
}arp_entry_t;

typedef struct list_head arp_entry_list_t;

extern net_err_t arp_init(void);
extern void arp_entry_free_static(arp_entry_t * entry);
extern net_err_t arp_entry_add_static(ip4addr_t * ip, uint8_t * mac);/* 这个函数暂时不能用，不知道对其他逻辑有没有影响 */
extern net_err_t arp_entry_match(ip4addr_t * ip, arp_entry_t ** arp_entry, arp_entry_t ** oldest);
extern arp_entry_t * arp_resolve(netif_t * netif, ip4addr_t * d_ip, nbuf_t * nbuf);
extern net_err_t arp_in(netif_t * netif, nbuf_t * nbuf);
extern net_err_t arp_announcement(netif_t * netif);
#endif