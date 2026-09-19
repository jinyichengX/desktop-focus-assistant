#ifndef EL_IPV4_H
#define EL_IPV4_H

#include "elnet.h"
#include "el_ndefs.h"
#include "el_nbuf.h"

/* ipv4 address struct */
typedef struct st_ipv4
{
	union
	{
		uint32_t ipv;
		uint8_t ipa[IPV4_ADDR_LEN];
	};
}ip4addr_t;

#pragma pack(1)

/* 小端平台上，结构体也是按低地址低位，高地址高位存放的 */
/* 结构体的元素从上往下是按照从低到高的地址排列 */
typedef struct stIPv4Header
{
    struct {
#if ENDIANNESS_LITTLE == 1
        uint8_t hd_len : 4;	        /* 首部长度（以4字节为单位）*/
        uint8_t version : 4;		/* ipv4版本 */
#else
        uint8_t version : 4;	    /* ipv4版本 */
        uint8_t hd_len : 4;		    /* 首部长度（以4字节为单位）*/
#endif
    };
	uint8_t tos;			        /* 区分服务（type of service） */
	uint16_t total_len;		        /* ip数据包总长度 */
	uint16_t id;                    /* 标识 */
    union {
        uint16_t fragFlagAndOff;    /* 分片标志和偏移 */
        uint8_t  ArrayDebug[2];     /* 用于调试 */
        struct {
#if ENDIANNESS_LITTLE == 1
            uint16_t frag_off : 13;	/* 片偏移 */
            uint16_t morefrag : 1;  /* 更多分片 */
            uint16_t dontfrag : 1;	/* 不允许分片 */
            uint16_t reserved : 1;  /* 保留 */
#else 
            uint16_t frag_off : 13;	/* 标志 */
            uint16_t morefrag : 1;  /* 更多分片 */     
            uint16_t dontfrag : 1;	/* 不允许分片 */
            uint16_t reserved : 1;  /* 保留 */
#endif
        };
    };
	uint8_t ttl;			        /* 生存时间（Time To Live） */
	uint8_t protocal;		        /* 上层协议（1:ICMP 2:IGMP 4:TCP 17:UDP 41:IPV6 89:OSPF） */
	uint16_t checksum;		        /* 首部检验和 */
	ip4addr_t src;			        /* 源IP */
	ip4addr_t des;			        /* 目的IP */
}ipv4_hdr_t;
#pragma pack()

/* ipv4分片排序节点 */
typedef struct _tag_ipv4_frag_off_sort_node_t
{
    struct list_head sort_node;     /* 分片偏移排序链表节点 */
    uint16_t usFragentOff;          /* 分片偏移 */
    nbuf_t * pubFragBuffer;         /* 待重组的ip分片 */
}ipv4_frag_t;

/* ipv4分包重组 */
typedef struct _tag_ipv4_reass_context_t
{
    struct list_head frag_node;
    struct list_head stFragOffSortList;     /* 分片偏移排序链表 */
    uint16_t usIdentifier;                  /* 分片标识 */
    char s_timeout;                         /* ip分片重组超时计数 */
#if USE_IP_FRAGMENT_REASS_SOLUTION_A
    sys_sem_t pWaitFragmentReass;           /* sem wait for reass ok */
#endif
}ipv4_reass_t;

typedef struct list_head  route_entry_list_t;
typedef struct list_head  route_entry_node_t;
typedef struct list_head  ipv4_reass_list_t;

typedef struct ipv4_route
{
    ip4addr_t des_network;	                /* 目的网络 */
    ip4addr_t netmask;		                /* 目的网络的掩码 */
    char prefix_len;                        /* 前缀长度 */
    ip4addr_t nexthop;	                    /* 下一跳（地址） */
    void * if_next;		                    /* 下一跳（网络接口） */

    enum {
        NET_RT_LOCAL,                       /* 局域网 */
        NET_RT_NETIF,                       /* 指向本接口 */
        NET_RT_WIDE,                        /* 非局域网 */
    }type;

    route_entry_node_t node;
}route_entry_t;

/* ip struct to value from ip4addr_t point */
#define IP_PSTRUCT_TO_U32(ip) ((ip)->ipv)

/* route match macro */
#define NET_MATCH(ip,mask,des) ( ( (IP_PSTRUCT_TO_U32(ip)) & (IP_PSTRUCT_TO_U32(mask)) )\
								== ( (IP_PSTRUCT_TO_U32(des)) & (IP_PSTRUCT_TO_U32(mask)) ) )

//extern char is_ipv4addr_broadcast(netif_t *,ip4addr_t *);
extern net_err_t        ipv4_init(void);
extern route_entry_t *  route_match(ip4addr_t * des);
extern net_err_t        route_delete(ip4addr_t * network);
extern net_err_t        ipv4_str2ipaddr(const char * ipstr, ip4addr_t * ip);
extern char             ipv4_netmask_legal(ip4addr_t * netmask);
extern net_err_t        ipv4_out(uint8_t prot, uint8_t ttl, ip4addr_t * des, ip4addr_t * src, nbuf_t * nbuf);
//extern net_err_t        ipv4_in(netif_t * netif, nbuf_t * nbuf);
#endif