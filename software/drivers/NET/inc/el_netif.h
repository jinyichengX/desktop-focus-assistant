#ifndef EL_NETIF_H
#define EL_NETIF_H

#include "elnet.h"
#include "el_nconfig.h"
#include "el_ndefs.h"
#include "el_smlist.h"
#include "el_ipv4.h"
#include "el_nbuf.h"
#include "queue.h"

typedef struct _net_interface_st netif_t;

/* network interface driver operations */
typedef struct netif_operations_ctx
{
    net_err_t (*open)(netif_t *, void * args);
    net_err_t (*close)(netif_t *, void * args);
    net_err_t (*send)(netif_t * netif, void * data, uint16_t size);
}netif_ll_ops_t;

/* link layer protocal */
typedef enum
{
    LINKER_TYPE_OTHER,                                      /* 未指明 */
    LINKER_TYPE_LOOP,						                /* 环回接口 */
    LINKER_TYPE_ETHER,						                /* 以太网链路 */
    LINKER_TYPE_ISO88023,                                   /* IEEE 802.3以太网（CSMA/CD） */
    LINKER_TYPE_ISO88025,                                   /* IEEE 802.5令牌环 */
    LINKER_TYPE_FDDI,                                       /* 光纤分布式数据接口 */
    LINKER_TYPE_SLIP,                                       /* SLIP */
}linker_type_t;

typedef struct _net_interface_st
{
    ip4addr_t ipaddr;						                /*主机ip*/
    ip4addr_t mask;							                /*掩码*/
    ip4addr_t gw_ip;						                /*网关*/

    int mtu;								                /*最大传输单元*/
    linker_type_t linker_type;				                /*链路层类型*/
    uint8_t hwaddr[NETIF_HARDWARE_ADDR_LEN];                /*网络接口链路层硬件地址*/

    netif_ll_ops_t * ops;                                   /*网卡底层驱动*/

    net_err_t (*init)(netif_t *, void *);	                /*链路层初始化*/
    net_err_t (*linkout)(netif_t *, ip4addr_t *, nbuf_t *);	/*链路层发送*/
    net_err_t (*linkin)(netif_t *, nbuf_t *);               /*链路层接收*/

    struct list_head node;							        /*网卡节点*/
    const char * name;						                /*网卡名*/

    queue_t send_queue;                                     /*发送队列*/
    queue_t recv_queue;                                     /*接收队列*/
    uint8_t sqb[sizeof(void *) * NETIF_SQUEUE_SIZE];        /*发送队列缓冲区*/
    uint8_t rqb[sizeof(void *) * NETIF_RQUEUE_SIZE];        /*接收队列缓冲区*/

    void * priv_args;                                       /*私有参数*/

#if USE_NETIF_BANDWIDTH_STATISTIC == 1
    sys_mutex_t send_bandwidth_mutex;                       /*发送带宽统计互斥锁,用于多个线程同时使用该网卡发包*/
    uint32_t send_bps;                                      /*sent bits per second*/
    sys_mutex_t recv_bandwidth_mutex;                       /*接收带宽统计互斥锁,用于多个线程同时使用该网卡收包*/
    uint32_t recv_bps;										/*received bits per second*/
	uint32_t recv_bps_max; 
		uint32_t recv_bps_last; 
#endif
}netif_t;

typedef struct list_head netif_list_t;

/* extern netif functions */
extern net_err_t netif_init(void);
extern netif_t * netif_add(const char * name,\
				            const char * ip,\
				            const char * mask,\
				            ip4addr_t * gw_ip,\
				            linker_type_t link_type,\
                    netif_ll_ops_t * netif_ops);
extern net_err_t netif_delete(netif_t ** netif);
extern int netif_ip_match(ip4addr_t * ip);
extern net_err_t netif_out(netif_t * netif, ip4addr_t * ip_toMatch_mac, nbuf_t * nbuf);
extern net_err_t netif_send_queue_post(netif_t * netif, nbuf_t * nbuf, uint32_t tmo);
extern net_err_t netif_send_queue_pend(netif_t * netif, void ** item, uint32_t tmo);
extern net_err_t netif_recv_queue_post(netif_t * netif, nbuf_t * nbuf, uint32_t tmo);
extern net_err_t netif_recv_queue_pend(netif_t * netif, void ** item, uint32_t tmo);
extern net_err_t netif_in(netif_t * netif);
extern net_err_t netif_in_nbuf(netif_t * netif, nbuf_t * nbuf);
#endif