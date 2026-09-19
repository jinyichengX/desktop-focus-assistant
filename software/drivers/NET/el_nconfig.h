#ifndef EL_NCONFIG_H
#define EL_NCONFIG_H

/* 数据包参数 */
#ifndef NETBUF_PACKET_NUM
#define NETBUF_PACKET_NUM                                           20                                  /* 约占用4KB */
#endif
#ifndef NETBUF_BLOCK_NUM
#define NETBUF_BLOCK_NUM                                            100                                 /* 约占用100KB */
#endif
#ifndef NETBUF_BLOCK_SIZE
#define NETBUF_BLOCK_SIZE                                           500                                 /* 值越大分配数据包越快，收发效率越高 */
#endif

#ifndef NET_PACKET_MAX_SIZE
#define NET_PACKET_MAX_SIZE                                         1514                                /* 网络数据包最大长度，一般不需要改，除非你知道你自己在干什么 */
#endif

/* 配置默认网卡参数 */
#ifndef NETIF_DEFAULT_IP
#define NETIF_DEFAULT_IP	                                        "192.168.8.105"                     /* 无效参数，可删除 */
#endif 

#ifndef NETIF_DEFAULT_MASK
#define NETIF_DEFAULT_MASK	                                        "255.255.255.0"                     /* 无效参数，可删除 */
#endif

#ifndef NETIF_DEFAULT_GATE
#define NETIF_DEFAULT_GATE                                          "192.168.8.1"                       /* 无效参数，可删除 */
#endif

/* 接口层 */
#ifndef NETIF_NAME_LEN
#define NETIF_NAME_LEN                                              5                                   /* 网卡名称长度 */
#endif

#ifndef NETIF_MAXIUM_NUM
#define NETIF_MAXIUM_NUM	                                        2                                   /* 最大支持的网卡数量 */
#endif

#ifndef NETIF_SQUEUE_SIZE
#define NETIF_SQUEUE_SIZE                                           20                                  /* 网卡发送队列大小（以帧为单位） */
#endif

#ifndef NETIF_RQUEUE_SIZE
#define NETIF_RQUEUE_SIZE                                           20                                  /* 网卡接收队列大小（以帧为单位） */
#endif

#define USE_NETIF_BANDWIDTH_STATISTIC                               1                                   /* 是否使用网卡收发带宽统计（对所有网卡生效） */
#if USE_NETIF_BANDWIDTH_STATISTIC
#define BANDWIDTH_STATISTIC_SMOOTH                                  0                                   /* 是否使用带宽平滑（对所有网卡生效） */
#ifndef NETIF_RECV_BANDWIDTH_STATISTIC_TIMER_CALLBACK_TIME
#define NETIF_RECV_BANDWIDTH_STATISTIC_TIMER_CALLBACK_TIME          1000                                /* 网卡接收带宽统计定时器回调时间(ms) */
#endif
#endif

/* 接口层以太网相关 */
#ifndef ARP_ENTRY_NUM
#define ARP_ENTRY_NUM                                               20                                  /* ARP表条目数量 */
#endif

#ifndef ARP_ENTRY_ALIVE_TIME
#define ARP_ENTRY_ALIVE_TIME                                        (20 * 60)                           /* ARP条目有效时间（秒） */
#endif

#ifndef ARP_TIMER_CALLBACK_SECS
#define ARP_TIMER_CALLBACK_SECS                                     1                                   /* ARP定时器回调时间间隔（秒） */
#endif

#ifndef ARP_TIMER_CALLBACK_TIME
#define ARP_TIMER_CALLBACK_TIME                                     (ARP_TIMER_CALLBACK_SECS * 1000)    /* ARP定时器回调时间(ms) */
#endif

#ifndef ARP_REQUEST_COUNT
#define ARP_REQUEST_COUNT                                           5                                   /* ARP请求重拾次数（或秒） */
#endif

#ifndef NETIF_HARDWARE_ADDR_LEN
#define NETIF_HARDWARE_ADDR_LEN                                     10                                  /* 硬件地址长度（不建议小于以太网mac地址长度6） */
#endif

/* 网络层 */
#ifndef IP_ROUTE_ENTRY_NUM
#define IP_ROUTE_ENTRY_NUM                                          (NETIF_MAXIUM_NUM * 6)              /* 设置路由表条目数量（由网卡数目决定） */
#endif

#ifndef IP_ROUTE_DEFAULT
#define IP_ROUTE_DEFAULT                                            1                                   /* 设置默认静态路由，暂时不用 */
#endif

#ifndef IP_TIMER_CALLBACK_SECS
#define IP_TIMER_CALLBACK_SECS                                      1                                   /* IP定时器回调时间间隔（秒） */
#endif

#ifndef IP_TIMER_CALLBACK_TIME
#define IP_TIMER_CALLBACK_TIME                                      (IP_TIMER_CALLBACK_SECS * 1000)     /* IP定时器回调时间(ms) */
#endif

#ifndef IP_FRAG_TIMEOUT_SECS
#define IP_FRAG_TIMEOUT_SECS                                        (IP_TIMER_CALLBACK_SECS * 2)        /* IP分片重组超时时间（秒） */
#endif

#ifndef IP_FRAG_TIMEOUT_TIME
#define IP_FRAG_TIMEOUT_TIME                                        (IP_FRAG_TIMEOUT_SECS * 1000)       /* IP分片重组超时时间（ms） */
#endif

#ifndef USE_IP_FRAGMENT_REASS_SOLUTION_A
#define USE_IP_FRAGMENT_REASS_SOLUTION_A                            1                                   /* 使用IP分片重组方案A */
#endif

#ifndef IP_PACKET_NEED_REASSEMBLED_NUM
#define IP_PACKET_NEED_REASSEMBLED_NUM                              10                                  /* 短时间内最大需要重组的IP分片数目 */
#endif

#ifndef IP_FRAGMENT_SUPPORTED_NUM
#define IP_FRAGMENT_SUPPORTED_NUM                                   100                                 /* 短时间内最大支持的IP分片重组数量 */
#endif


/* 传输层 */
#ifndef UDP_MAX_CONNECTION_NUM
#define UDP_MAX_CONNECTION_NUM                                      10                                  /* UDP最大连接数 */
#endif

#ifndef TCP_MAX_CONNECTION_NUM
#define TCP_MAX_CONNECTION_NUM                                      10                                  /* TCP最大连接数 */
#endif

#ifndef TCP_MAX_SEGMENT_SIZE
#define TCP_MAX_SEGMENT_SIZE                                        1460                                /* TCP最大报文段长度，对于以太网最大值是1460: 1500 - 20(ip hdr) - 20(tcp hdr) */
#if TCP_MAX_SEGMENT_SIZE > 1460
#warning "be care of TCP_MAX_SEGMENT_SIZE in the file el_config.h"
#endif
#endif

/* 用户层 */
#ifndef MAX_SOCK_NUM
#define MAX_SOCK_NUM                                                10                                  /* max sock num */
#endif

/* 应用层 */

/* system timer */
#ifndef TIMER_MAX_NUM
#define TIMER_MAX_NUM                                               20                                  /* 系统最大支持的定时器数量 *//* 最少需要4个，arp + ip4 + statistics + tcp */
#endif

/* max recv pending packet number */
#ifndef KQ_PENDING_NUM
#define KQ_PENDING_NUM                                              20                                  /* 系统最大接收挂起数据包数量 */
#endif

#ifndef ASYNC_MQ_ITEM_NUM
#define ASYNC_MQ_ITEM_NUM                                           1                                   /* 异步消息队列条目数量 */
#endif

#ifndef USE_ARP_ATTACKER
#define USE_ARP_ATTACKER                                            0                                   /* 是否使用ARP攻击 */
#endif

/* 组件的裁剪 */
#ifndef USE_CJSON
#define USE_CJSON                                                   1                                   /* 是否使用cJSON */
#endif

#endif