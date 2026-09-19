
#ifndef EL_UDP_H
#define EL_UDP_H

#include "elnet.h"
#include "el_nbuf.h"
#include "el_ipv4.h"
#include "el_ndefs.h"
#include "el_netif.h"
#include "sock.h"

#pragma pack(1)
typedef struct udp_header_st
{
	uint16_t sport;		
	uint16_t dport;		
	uint16_t len;		
	uint16_t checksum;
}udp_hdr_t;
#pragma pack()

typedef struct list_head udp_cb_list_t;
typedef struct list_head udp_cb_node_t;

#define UDP_STATE_BIND 0x01
#define UDP_STATE_CONNECT 0x02

typedef struct udp_context_t{
  net_sock_t net_sock;
  struct list_head node;
  char state;
}udp_t;

#define udp_state_set(u, s) (u->state |= s)
#define udp_state_is_set(u, s) (u->state & s)

extern udp_t * udp_create(void);
extern net_err_t udp_init(void);
extern net_err_t udp_bind(udp_t * udp, endpoint_t * local);
extern net_err_t udp_connect(udp_t * udp, endpoint_t * remote);
extern net_err_t udp_sendto(udp_t * udp, void * buf, uint16_t size, ip4addr_t * dst, uint16_t port);
extern net_err_t udp_recvfrom(udp_t * udp, void * buf, uint16_t size, ip4addr_t * remote, uint16_t * port, uint16_t * len);
extern net_err_t udp_send(udp_t * udp, void * buf, uint16_t size);
extern net_err_t udp_recv(udp_t * udp, void * buf, uint16_t size, uint16_t * len);
extern net_err_t generate_base_packet(nbuf_t ** nbuf, void * buffer, uint16_t len);
extern net_err_t udp_out_raw(ip4addr_t * des, ip4addr_t * src, uint16_t sport, uint16_t dport, nbuf_t * nbuf);
extern net_err_t udp_out(ip4addr_t * des, ip4addr_t * src, uint16_t sport, uint16_t dport, void * buffer, unsigned short len);
extern net_err_t udp_process_in(nbuf_t * pubNetPacket, ip4addr_t * pstSrcAddr, ip4addr_t * pstDestAddr);
extern net_err_t udp_destroy(udp_t * udp);
#endif