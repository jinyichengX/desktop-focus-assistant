#ifndef EL_NDEFS_H
#define EL_NDEFS_H

/* 存放了网络相关的定义，不可修改 */
#include <stdint.h>

/* 数据链路层以太网链路相关定义 */
#define ETHER_MAC_ADDR_LEN			                        6						                            /* 以太网物理地址长度 */

#define ETHER_TYPE_ARP				                        0x0806						                        /* 以太网帧数据类型是ARP包 */

#define ETHER_TYPE_IP				                        0x0800						                        /* 以太网帧数据类型是IP包 */

#define ETHER_BROADCAST_ADDR		                        0xffffffff				                            /* 广播地址 */

#define ETHER_BROADCAST_ADDR_STR	                        "255.255.255.255"	                                /* 广播地址 */

#define ETHER_HEADER_LEN                                    14                                                  /* 以太网帧头长度 */

#define ETHER_MTU					                        1500							                    /* 以太网帧最大传输单元 */

#define ETHER_MIN_TU                                        46                                                  /* 以太网帧最小传输单元 */

#define ETHER_FCS_LEN                                       4                                                   /* 以太网帧FCS(frame check sum)长度 */

#define ETHER_PACKET_MIN_LEN                                (ETHER_HEADER_LEN + ETHER_MIN_TU)                   /* 以太网数据包最小长度60 */

#define ETHER_PACKET_MAX_LEN                                (ETHER_HEADER_LEN + ETHER_MTU)                      /* 以太网数据包最大长度1514 */

#define ETHER_FRAME_MIN_LEN                                 (ETHER_HEADER_LEN + ETHER_MIN_TU + ETHER_FCS_LEN)   /* 以太网帧最小长度64 */

#define ETHER_FRAME_MAX_LEN                                 (ETHER_HEADER_LEN + ETHER_MTU + ETHER_FCS_LEN)      /* 以太网帧最大长度1518 */

/* 网络层相关定义 */
#define ARP_PACKET_SIZE                                     28                                                  /* ARP包长度 */

#define ARP_OPCODE_REQUEST                                  1                                                   /* ARP请求 */

#define ARP_OPCODE_REPLY                                    2                                                   /* ARP应答 */

#define ARP_PACKET_SIZE_PACKED                              ETHER_MIN_TU                                        /* ARP请求/应答帧长度 */

#define IPV4_ADDR_ANY                                       0x00000000                                          /* 任意IP地址 */

#define IPV4_MUTICAST_NETWORK                               0xe0000000                                          /* 多播地址网络号 */

#define IPV4_ADDR_BROADCAST                                 0xffffffff                                          /* 广播地址 */

#define MASK_ADDR_ANY                                       IPV4_ADDR_ANY                                       /* 全0掩码 */

#define MASK_ADDR_BROADCAST                                 IPV4_ADDR_BROADCAST                                 /* 全1掩码 */

#define IPV4_RESTRICTED_BROADCAST_STR                       "255.255.255.255"                                   /* 受限广播地址 */

#define IPV4_RESTRICTED_BROADCAST                           IPV4_ADDR_BROADCAST                                 /* 受限广播地址 */

#define IPV4_NETWORK_ANY                                    "0.0.0.0"                                           /* 任意网络 */

#define IPV4_MASK_ANY                                       "0.0.0.0"

#define NET_LOOPBACK_STR                                    "127.0.0.0"                                         /* 本地环回网络 */

#define NET_LOOPBACK                                        0x7f000000                                          /* 本地环回网络 */

#define MASK_LOOPBACK_STR                                   "255.0.0.0"                                         /* 本地环回掩码 */

#define MASK_LOOPBACK                                       0xff000000                                          /* 本地环回掩码 */

#define ARP_PROT_TYPE_IP				                    0x0800					                            /* ARP协议服务类型为ipv4 */

#define ARP_PROT_TYPE_IPV6                                  0x86DD                                              /* ARP协议服务类型为ipv6 */

/* IPV4相关begin */
#define IPV4_ADDR_LEN                                       4								                    /* IPV4地址长度 */
/* IPV4首部相关begin */
#define IPV4_HEAD_VERSION                                   4                                                   /* IPV4首部版本 */

#define IPV4_HEADER_LEN                                     20                                                  /* IPV4首部固定长度 */

#define IPV4_HEAD_TTL_DEFAULT                               64                                                  /* 生存时间（以跳数为单位） */

#define IPV4_HEAD_PROTOCAL_ICMP                             0x01                                                /* IPV4首部协议类型：ICMPV4 */

#define IPV4_HEAD_PROTOCAL_TCP                              0x06                                                /* IPV4首部协议类型：TCP */

#define IPV4_HEAD_PROTOCAL_UDP                              0x11                                                /* IPV4首部协议类型：UDP */
/* IPV4首部相关end */
/* IPV4相关end */

/* ICMPV4相关begin（只列出常用类型） */
#define ICMPV4_HEADER_LEN                                   8                                                   /* ICMPV4首部长度 */

#define ICMPV4_ECHO_HEADER_LEN                              ICMPV4_HEADER_LEN                                   /* ICMPV4回送请求/应答首部长度 */ 
/* 回送请求和回送应答 */
#define ICMPV4_TYPE_ECHO_REQUEST                            8                                                   /* 回送请求（类型） */

#define ICMPV4_TYPE_ECHO_REPLY                              0                                                   /* 回送应答（类型） */

#define ICMPV4_CODE_ECHO_REQUEST                            0                                                   /* 回送请求（代码） */

#define ICMPV4_CODE_ECHO_REPLY                              0                                                   /* 回送应答（代码） */

#define ICMPV4_ECHO_REQUEST_DUMMY_LEN                       32                                                  /* 回送请求数据长度 */
/* 目的不可达及具体类型（部分） */
#define ICMPV4_TYPE_DEST_UNREACH                            3                                                   /* 目的不可达（类型） */

#define ICMPV4_CODE_DEST_UNREACH_NET                        0                                                   /* 网络不可达（代码） */

#define ICMPV4_CODE_DEST_UNREACH_HOST                       1                                                   /* 主机不可达（代码） */

#define ICMPV4_CODE_DEST_UNREACH_PROTO                      2                                                   /* 协议不可达（代码） */

#define ICMPV4_CODE_DEST_UNREACH_PORT                       3                                                   /* 端口不可达（代码） */

#define ICMPV4_CODE_DEST_UNREACH_FRAG_NEEDED                4                                                   /* 需要分片但不分片位置位（代码） */

#define ICMPV4_CODE_DEST_UNREACH_SRC_ROUTE                  5                                                   /* 源站选路失败（代码） */

#define ICMPV4_CODE_DEST_UNREACH_NET_UNKNOWN                6                                                   /* 未知网络（代码） */

#define ICMPV4_CODE_DEST_UNREACH_HOST_UNKNOWN               7                                                   /* 未知主机（代码） */

#define ICMPV4_UNREACH_DATA_LEN                             (IPV4_HEADER_LEN + 8)                               /* 目的不可达数据长度 */
/* ICMPV4相关end */

/* 传输层相关定义 */
#define UDP_HEADER_LEN                                      8                                                   /* UDP首部长度 */

#define TCP_HEADER_LEN                                      20                                                  /* TCP固定首部长度 */

/* define some well-known ports */
#define FTP_PORT                                            21                                                  /* FTP 文件传输服务(TCP) */
#define SSH_PORT                                            22                                                  /* SSH 远程连接服务(TCP) */          
#define TELNET_PORT                                         23                                                  /* TELNET 远程登录服务(TCP) */
#define SMTP_PORT                                           25                                                  /* SMTP 简单邮件传输服务(TCP) */
#define DNS_PORT                                            53                                                  /* DNS 域名解析服务(TCP/UDP) */
#define DHCP_SERV_PORT                                      67                                                  /* DHCP 动态主机配置协议服务，服务器(UDP) */
#define DHCP_CLNT_PORT                                      68                                                  /* DHCP 动态主机配置协议服务，客户端(UDP) */
#define TFTP_PORT                                           69                                                  /* TFTP 简单文件传输服务(UDP) */
#define HTTP_PORT                                           80                                                  /* HTTP 超文本传输服务(TCP) */
#define POP3_PORT                                           110                                                 /* POP3 邮局协议服务(TCP) */
#define NTP_PORT                                            123                                                 /* NTP 网络时间协议服务(UDP) */
#define IMAP_PORT                                           143                                                 /* IMAP 邮件访问协议服务(TCP) */
#define HTTPS_PORT                                          443                                                 /* HTTPS 加密的超文本传输服务(TCP) */
#define WELL_KNOWN_PORT_MAX                                 1023                                                /* 最大知名端口号 */

/* define some registered ports */
#define MYSQL_PORT                                          3306                                                /* MYSQL 数据库服务 */
#define REGISTERED_PORT_MAX                                 49151                                               /* 最大注册端口号 */ /* 这些端口可以由应用程序注册使用，以避免与其他应用程序冲突。例如，一些数据库服务或游戏服务器可能会使用这些端口 */


#define DYNAMIC_PORT_MAX                                    65535                                               /* 最大动态端口号 */
#endif