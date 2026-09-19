#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include "el_ndefs.h"

#ifndef NMIN
#define NMIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef NMAX
#define NMAX(a,b) ((a) > (b) ? (a) : (b))
#endif

/* ip地址的值 */
#define IPV4_ADDR_VAL_GET(ip)                   ((ip)->ipv)
#define IPV4_ADDR_VAL_SET(ip,v)                 ((ip)->ipv = (v))

/* 由ip结构体获得32位ip值 */
#define IPV4_STRUCT_TO_IP(ipaddr)               IPV4_ADDR_VAL_GET(ipaddr)

/* 由ip和掩码获得网络地址 */
#define IPV4_ADDR_NETWORK(ip,mask)              (IPV4_STRUCT_TO_IP(ip) & IPV4_STRUCT_TO_IP(mask))

/* 两个ip是否在同一网段 */
#define IPV4_ADDR_NETCMP(ip1,ip2,mask)          (IPV4_ADDR_NETWORK((ip1),(mask)) == IPV4_ADDR_NETWORK((ip2),(mask)))

/* 两个ip是否相等 */
#define IPV4_ADDR_IS_EQUAL(ip1,ip2)             (IPV4_ADDR_VAL_GET(ip1) == IPV4_ADDR_VAL_GET(ip2))

/* 任意IP */
#define IS_IPV4_ADDR_ANY(ip)                    (IPV4_ADDR_VAL_GET(ip) == IPV4_ADDR_ANY)

/* 受限广播ip */
#define IS_IPV4_RESTRICTED_BROADCAST(ip)        (IPV4_ADDR_VAL_GET(ip) == IPV4_RESTRICTED_BROADCAST)

/* 直接广播ip */
#define IS_IPV4_DIRECT_BROADCAST(src,des,mask)  ( IPV4_ADDR_NETWORK(src,mask) & IPV4_ADDR_BROADCAST ) == IPV4_ADDR_VAL_GET(des)

/* 多播/组播ip */
#define IS_IPV4_MUTICAST(ip)                    ((IPV4_ADDR_VAL_GET(ip) & IPV4_MUTICAST_NETWORK) == IPV4_MUTICAST_NETWORK)

/* 本地环回地址 */
#define IS_IPV4_LOOPBACK(ip)                    ((IPV4_ADDR_VAL_GET(ip) > NET_LOOPBACK) && (IPV4_ADDR_VAL_GET(ip) < ((~MASK_LOOPBACK) | NET_LOOPBACK)))

#define MEM_ALIGN_UP(size,nb)			        (((size) + ((nb)-1) ) & (~((size_t)((nb)-1))))
#define MEM_ALIGN_DOWN(size,nb)   		        ((size) & (~(size_t)((nb)-1) ))

#endif