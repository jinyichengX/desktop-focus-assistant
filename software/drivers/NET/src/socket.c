#include "socket.h"
#include "el_udp.h"
#include "el_ipv4.h"
#include "el_netif.h"
#include "queue.h"

//udp connect后必须调用send和recv

/* global sock for allocating to user */ 
static con_net_sock_t sock[MAX_SOCK_NUM];
static con_net_sock_t * psock[MAX_SOCK_NUM];
static int sock_remain = MAX_SOCK_NUM;

/* socket idx init */
void socket_init(void)
{
    for(int i = 0; i < MAX_SOCK_NUM; i ++)
        psock[i] = &sock[i];
}

static inline void sock_decrease(void)
{
    sock_remain --;
}
static inline void sock_increase(void)
{
    sock_remain ++;
}

/* create new socket */
int __socket(int domain, int type, int protocol)
{
    // int sockfd;
    // if(!sock_remain)
    //     return -1;

    // /* only support udp now (date:Beijing Time 20241024) */
    // if (domain != _AF_INET || type != _SOCK_DGRAM)
    //     return -1;

    // sockfd = psock[sock_remain - 1] - sock;
    // plat_memset(&sock[sockfd], 0, sizeof(con_net_sock_t));
    // if(NULL == (sock[sockfd].net_sock.sock.pubSemToRecv = sys_sem_create(0)))
    //     return -1;

    // INIT_LIST_HEAD(&sock[sockfd].net_sock.sock.pstRecvList);
    // sock_decrease();
    // return sockfd;//return 0 ~ MAX_SOCK_NUM - 1
}


/* close socket */
int __close(int sockfd)
{
    // if (sockfd < 0 || sockfd >= MAX_SOCK_NUM)
    //     return -1;

    // psock[sock_remain] = &sock[sockfd];
    // sys_sem_destroy(sock[sockfd].net_sock.sock.pubSemToRecv);
    // sock_increase();
    // return 0;
}

// // 绑定socket到特定的端口和IP地址
// int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
// {
//     if (sockfd < 0 || sockfd >= MAX_SOCK_NUM){
//         return -1; // 无效的socket描述符
//     }

//     if (addr->sa_family != AF_INET) {
//         return -1; // 目前只支持IPv4
//     }

//     struct sockaddr_in *in_addr = (struct sockaddr_in *)addr;
//     net_sock_t *sock = &sockets[sockfd];

//     IPV4_ADDR_VAL_SET(&sock->net_sock.stSrcAddr, _ntohl(in_addr->sin_addr.s_addr));
//     sock->net_sock.usSrcPort = _ntohs(in_addr->sin_port);

//     return 0;
// }

// // 发送数据
// int sendto(int sockfd, const char * buf, size_t nbytes, int flags, const struct sockaddr * to, socklen_t addrlen)
// {
//     if (sockfd < 0 || sockfd >= MAX_SOCK_NUM) {
//         return -1; // 无效的socket描述符
//     }

//     if (to->sa_family != AF_INET) {
//         return -1; // 目前只支持IPv4
//     }

//     struct sockaddr_in *in_addr = (struct sockaddr_in *)to;
//     net_sock_t *sock = &sockets[sockfd];

//     IPV4_ADDR_VAL_SET(&sock->net_sock.stDestAddr, _ntohl(in_addr->sin_addr.s_addr));
//     sock->net_sock.usDestPort = _ntohs(in_addr->sin_port);

//     nbuf_t *nbuf;
//     if (generate_base_packet(&nbuf, buf, nbytes) != NET_ERR_OK) {
//         return -1;
//     }

//     return udp_out_raw(&sock->net_sock.stDestAddr, &sock->net_sock.stSrcAddr,
//                     sock->net_sock.usSrcPort, sock->net_sock.usDestPort, nbuf);
// }

// // 接收数据
// int recvfrom(int sockfd, char * buf, size_t len, int flags, struct sockaddr * from, socklen_t * addrlen)
// {
//     if (sockfd < 0 || sockfd >= MAX_SOCK_NUM) {
//         return -1; // 无效的socket描述符
//     }

//     net_sock_t *sock = &sockets[sockfd];

//     if (sem_take(&sock->sock.pubSemToRecv, 0xffffffff) != 0) {
//         return -1;
//     }

//     nbuf_t *nbuf = NULL;
//     list_for_each_entry(nbuf, &sock->sock.pstRecvList, nbuf_node) {
//         break;
//     }

//     if (nbuf) {
//         memcpy(buf, nbuf_data(nbuf), len);
//         nbuf_free(nbuf);
//         list_del(&nbuf->nbuf_node);
//     }

//     if (from) {
//         struct sockaddr_in *in_addr = (struct sockaddr_in *)from;
//         IPV4_ADDR_VAL_GET(&sock->net_sock.stSrcAddr, &in_addr->sin_addr.s_addr);
//         in_addr->sin_port = _htons(sock->net_sock.usSrcPort);
//         *addrlen = sizeof(struct sockaddr_in);
//     }

//     return len; // 返回接收到的数据长度
// }

int __connect(void)
{

}


int __accept(void)
{
    /* tcp 从半连接队列中取一个tcp控制块 */
}
