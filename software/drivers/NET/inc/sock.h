#ifndef SOCK_H
#define SOCK_H

#include <stdint.h>
#include "el_ipv4.h"
#include "queue.h"

typedef struct _sock_context_st
{
    struct list_head recv_list;
    sys_sem_t notify;
    sys_mutex_t lock;             
}sock_t;

typedef struct ip4_and_port_st
{
    ip4addr_t ip4addr;
    int port;
}endpoint_t;

//udp
typedef struct _net_sock_context_st
{
    endpoint_t src;
    endpoint_t dst;
    sock_t sock;
}net_sock_t;

//tcp
typedef struct _connection_net_sock_context_st
{
    net_sock_t net_sock;
    void * state;
}con_net_sock_t;

#endif 
