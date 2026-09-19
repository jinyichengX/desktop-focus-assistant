#ifndef SOCKET_H
#define SOCKET_H

#include "bsd_types.h"

typedef int socklen_t;

/* domain */
#define _AF_INET         2   //IPV4协议族
#define _AF_INET6        10  //IPV6协议族

/* type */
#define _SOCK_STREAM     1
#define _SOCK_DGRAM      2    
#define _SOCK_RAW        3

/* protocol */
#define _IPPROTO_IP      0
#define _IPPROTO_ICMP    1
#define _IPPROTO_IGMP    2
#define _IPPROTO_TCP     6
#define _IPPROTO_UDP     17

typedef struct inaddr_ctx {
    union
    {
        struct
        {
            u_char addr1;
            u_char addr2;
            u_char addr3;
            u_char addr4;
        } s_un_b;
        u_int in_addr;
    }s_un;
}inaddr_t;

typedef struct sockaddr_ctx
{
    short sa_family;    //指定地址族ipv4或ipv6，它的值决定了如何解释sa_data成员
    char sa_data[14];   
}sockaddr_t;

typedef struct sockaddr_in_ctx
{
    short sin_family;
    u_short sin_port;
    inaddr_t sin_addr;
    char sin_zero[8];
}sockaddr_in_t;

extern int __socket(int domain, int type, int protocol);
extern int __close(int sockfd);
#endif