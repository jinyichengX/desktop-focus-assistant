
#ifndef EL_ETHER_H
#define EL_ETHER_H
#include "el_ndefs.h"
#include "el_nbuf.h"
#include "el_arp.h"
#include "el_ipv4.h"

#pragma pack(1)
typedef struct stEthernetHeader
{
	uint8_t des[ETHER_MAC_ADDR_LEN];/* 目的MAC */
	uint8_t src[ETHER_MAC_ADDR_LEN];/* 源MAC */
	uint16_t protocal;		/* 协议 */
}ether_hdr_t;
#pragma pack()
extern net_err_t ether_out_raw(netif_t * netif, uint8_t * hdw_des, nbuf_t * nbuf, uint16_t prot, uint8_t self);
extern net_err_t ether_init(netif_t * netif, void * args);
extern net_err_t ether_out_arp(netif_t * netif, ip4addr_t * des, nbuf_t * nbuf);
extern net_err_t ether_in(netif_t * netif, nbuf_t * nbuf);
#endif