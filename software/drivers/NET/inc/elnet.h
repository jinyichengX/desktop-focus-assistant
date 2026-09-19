#ifndef ELNET_H
#define ELNET_H

#include <stdint.h>
#include <stdbool.h>
#include "plat.h"

typedef enum 
{
    NET_ERR_OK,
    NET_ERR_NOK,
    NET_ERR_PARAM
}net_err_t;

typedef struct _tag_tcpip_message_context_t
{
    enum{
        TCP_IP_MESSAGE_TYPE_TIMEOUT,
        TCP_IP_MESSAGE_TYPE_RECV,
    }msg_type;
}kmsg_t;

static inline uint16_t little_htons(uint16_t v)
{
	return (v << 8 & 0xff00) | (v >> 8 & 0x00ff);
}

static inline uint32_t little_htonl(uint32_t v)
{
	return (v << 24) | (v << 8 & 0x00ff0000) | (v >> 8 & 0x0000ff00) | (v >> 24);
}

#if ENDIANNESS_LITTLE
#define _htons(x)        little_htons(x)
#define _ntohs(x)        little_htons(x)
#define _htonl(x)        little_htonl(x)
#define _ntohl(x)        little_htonl(x)
#else
#define _htons(x)        (x)
#define _ntohs(x)        (x)
#define _htonl(x)        (x)
#define _ntohl(x)        (x)
#endif

extern net_err_t el_net_init(void);
extern net_err_t net_start(void);
extern uint16_t net_checksum16(uint16_t, void *, int, bool);
//extern uint16_t net_checksum_pesor(uint16_t ,void * , int);
extern uint16_t _htons(uint16_t a);
extern uint32_t _htonl(uint32_t a);

#if defined(_WIN32) || defined(_WIN64)
extern DWORD netif_package_send(LPVOID args);
#elif defined(__LINUX__)
extern void netif_package_send(void * args);
#elif defined(__ELOS__)
extern void netif_package_send(void * args);
#else
extern void netif_package_send(void * args);
#endif

#if defined(_WIN32) || defined(_WIN64)
extern DWORD netif_package_recv(LPVOID args);
#elif defined(__LINUX__)
extern void netif_package_recv(void * args);
#elif defined(__ELOS__)
extern void netif_package_recv(void * args);
#else
extern void netif_package_recv(void * args);
#endif

#endif
