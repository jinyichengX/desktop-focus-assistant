#ifndef EL_TCP_H
#define EL_TCP_H

#include "elnet.h"
#include "el_nbuf.h"
#include "el_netif.h"
#include "timer.h"
#include "sock.h"

#pragma pack(1)
typedef struct _tag_tcp_header_st
{
    uint16_t src_port;
    uint16_t des_port;
    uint32_t seq_num;
    uint32_t ack_num;
    union{
        struct {
    #if ENDIANNESS_LITTLE == 1
        uint16_t fin : 1;
        uint16_t syn : 1;
        uint16_t rst : 1;
        uint16_t psh : 1;
        uint16_t ack : 1;
        uint16_t urg : 1;
        uint16_t reserved : 6;
        uint16_t data_off : 4;
    #else
        uint16_t data_off : 4;
        uint16_t reserved : 6;
        uint16_t urg : 1;
        uint16_t ack : 1;
        uint16_t psh : 1;
        uint16_t rst : 1;
        uint16_t syn : 1;
        uint16_t fin : 1;
    #endif
        }flags;
        uint16_t flag_val;
    };
    uint16_t win_size;
    uint16_t checksum;
    uint16_t urg_ptr;
    uint8_t ext_options[];
}tcp_hdr_t;
#pragma pack()

#define TCP_PACKET_FLAG_FIN 0x01
#define TCP_PACKET_FLAG_SYN 0x02
#define TCP_PACKET_FLAG_RST 0x04
#define TCP_PACKET_FLAG_PSH 0x08
#define TCP_PAKCET_FLAG_ACK 0x10
#define TCP_PAKCET_FLAG_URG 0x20

typedef enum
{
    TCP_STATE_CLOSED = 0,
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_SENT,
    TCP_STATE_SYN_RECEIVED,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT_1,
    TCP_STATE_FIN_WAIT_2,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_CLOSING,
    TCP_STATE_LAST_ACK,
    TCP_STATE_TIME_WAIT,
}tcp_state_t;

typedef struct
{
    uint32_t seq_base;
    uint32_t seq_next;  /* 接收窗口此参数无效 */
    uint32_t seq_end;
    bool overflow;
}tcp_slide_window_t;

typedef struct tcp_control_block_context
{

    con_net_sock_t sock;
    endpoint_t src;
    endpoint_t des;
    
    tcp_state_t state;

    timer_t send_tmo_cb;        /* 发送超时定时器 */
    tcp_slide_window_t swnd;
    queue_t send_queue;

    tcp_slide_window_t rwnd;
    queue_t recv_queue;
}tcp_cb_t;

typedef struct list_head tcp_cb_list_t;
typedef struct list_head tcp_cb_node_t;

#define tcp_state_set(cb, s) (cb->state = s)

/* size of send window */
#define TCP_SWND_TOTAL_SIZE(w) (w->seq_end - w->seq_base)
/* sent but not acked */
#define TCP_SWND_NACK_SIZE(w) (w->seq_next - w->seq_base) /* 发送窗口内的数据全部发送完毕后 若一定时间内接收不到接收方的确认序号会产生超时重传 */
/* not send yet */
#define TCP_SWND_VALID_SIZE(w) (w->seq_end - w->seq_next)

// extern net_err_t tcp_process_in( netif_t * netif, nbuf_t * nbuf );
extern net_err_t tcp_connect_to(tcp_cb_t * tcp_cb, ip4addr_t * serv_ip, uint16_t serv_port);
#endif