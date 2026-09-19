/*
 * MIT License
 *
 * Copyright (c) 2024~2025 JinYiCheng
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* 
RFC 793 - TCP标准协议文档： 这是最初的TCP标准定义文档，描述了TCP的基本功能和操作
RFC 813 - TCP窗口与确认策略： 讨论了窗口确认机制，以及在使用该机制时可能遇到的问题及解决方法
RFC 879 - TCP最大分段大小及相关主题： 讨论了MSS（最大分段大小）参数在控制TCP分组大小的重要性，以及该参数与IP分段大小的关系
RFC 896 - IP/TCP网络互联拥塞控制： 探讨了网络拥塞问题及TCP如何控制拥塞
RFC 1122 - 网络主机要求——通讯层： 讨论了TCP在主机中实现的细节
RFC 1146 - 可选的TCP校验和选项： 针对TCP设备使用可选校验和方法进行规范
RFC 1323 - 高性能下的TCP扩展： 定义了高速网络中TCP的扩展及新选项
RFC 2018 - TCP选择确认： 讨论了TCP基础功能的增强，如何选择性地制定特定字段来重传
RFC 2525 - 已知TCP的问题： 描述了当前已知的部分TCP问题
✔ RFC 2581 - TCP拥塞控制： 描述了用于拥塞控制的四种机制：慢启动、拥塞防御、快重传和快恢复
✔ RFC 2988 - TCP重传计时器计算： 讨论了与TCP重传计时器设置相关的话题，重传计时器控制报文在重传前应等待多长时间
*/

/* 可靠传输的细节问题：
 * 1. 对于不按序到达的数据应如何处理，TCP并无明确规定
 * （1）如果接收方把不按序到达的数据一律丢弃，那么接收窗口的管理将会比较简单，但这样做对网络资源的利用不利，因为发送方会重复传送较多的数据。
 * （2）TCP通常对不按序到达的数据先临时存放在接收窗口中，等到字节流中所缺少的字节收到后，再按序交付上层的应用进程。
 * 2. TCP要求接收方必须有累积确认(这一点与选择重传协议不同)和捎带确认机制。这样可以减小传输开销。接收方可以在合适的时候发送确认，也可以在自己有数据要发送时把确认信息顺便捎带上。
 * （1）接收方不应过分推迟发送确认，否则会导致发送方不必要的超时重传，这反而浪费了网络资源。TCP标准规定确认推迟的时间不应超过0.5秒。若收到一连串具有最大长度的报文段，则必须每隔一个报文段就发送一个确认[RFC 1122]。
 * （2）捎带确认实际上并不经常发生，因为大多数应用程序很少同时在两个方向上发送数据
 * 3. 发送方的发送窗口尺寸是根据接收方的接收窗口尺寸来设置的，但是并不总是和接收窗口一样大，也受拥塞控制的影响。
 */

/* tcp控制块维护一个对方的seq，如果包中的SYN置位且seq不是预期的，回发送challenge ack（质疑ack） */

#include "el_tcp.h"
#include "el_ipv4.h"
#include "el_netif.h"
#include "global.h"
#include "hash.h"

static uint32_t tcp_isn_generate(tcp_cb_t * tcp)
{
    /* md5 */
    return 0;
}

static uint16_t tcp_port_allocate(void)
{
    int port;
    for(port = REGISTERED_PORT_MAX + 1; port <= DYNAMIC_PORT_MAX; port ++)
    {
        
    }
    return 56489;
}

net_err_t tcp_init(void)
{
    /* check udp header length */
    if(UDP_HEADER_LEN != sizeof(tcp_hdr_t)){
        return NET_ERR_NOK;
    }

    if(NET_ERR_NOK == mempool_create(&tcp_cb_pool, (void *)tcp_cb, sizeof(tcp_cb_t), TCP_MAX_CONNECTION_NUM))
        return NET_ERR_NOK;

    /* 初始化全连接和半连接列表 */
    INIT_LIST_HEAD(&tcp_established_list);
    INIT_LIST_HEAD(&tcp_syn_list);
    return NET_ERR_OK;
}

net_err_t tcp_out_raw(ip4addr_t * des, ip4addr_t * src, uint16_t sport, uint16_t dport, nbuf_t * nbuf)
{
    tcp_hdr_t * tcp_hdr;
    net_err_t err = NET_ERR_NOK;
    route_entry_t * entry;
    uint16_t chksum = 0;
    uint8_t ZeroAndProt[2] = {0, IPV4_HEAD_PROTOCAL_TCP};

    /* 确定伪首部中的源目ip */
    if((!src) || (IS_IPV4_ADDR_ANY(src))){
        if(NULL == (entry = route_match(des))){
            return NET_ERR_NOK;
        }
        src = &((netif_t *)entry->if_next)->ipaddr;//使用网卡的ip
    }

    /* add tcp header */
    if(NET_ERR_OK != nbuf_header(nbuf, TCP_HEADER_LEN)){
        goto __return;
    }

    /* extract udp header */
    nbuf_extract_prefix(nbuf, TCP_HEADER_LEN); 

    /* fill udp header */
    tcp_hdr = (tcp_hdr_t *)nbuf_data(nbuf);
    tcp_hdr->src_port = _htons(sport);
    tcp_hdr->des_port = _htons(dport);
    /* udp len = udp header len+payload len */
    // tcp_hdr->len = _htons(NBUF_TTSZ(nbuf));
    tcp_hdr->checksum = 0;

    /* calculate checksum */
    chksum = net_checksum16(chksum, (void *)src, IPV4_ADDR_LEN, false);
    chksum = net_checksum16(chksum, (void *)des, IPV4_ADDR_LEN, false);
    chksum = net_checksum16(chksum, (void *)ZeroAndProt, 2, false);
    // chksum = net_checksum16(chksum, (void *)&tcp_hdr->len, 2, false);
    chksum = nbuf_checksum16(nbuf, chksum);

    tcp_hdr->checksum = chksum;

    /* send packet to ip module */
    if(err = ipv4_out(IPV4_HEAD_PROTOCAL_TCP, IPV4_HEAD_TTL_DEFAULT, des, src, nbuf)){
        plat_printf("error: udp send packet error\r\n");
    }

__return:
    return err;
}

net_err_t tcp_connect_process_in(tcp_cb_t * tcp, ip4addr_t * des, ip4addr_t * src, uint16_t sport, uint16_t dport);

/* find the tcp control block by src and des */
tcp_cb_t * tcp_control_block_match(ip4addr_t * src, uint16_t sport, ip4addr_t * dst, uint16_t dport)
{
    tcp_cb_t * tcp_cb;
    /* if dst ip in the host ip list *//* 检查目的ip是否在主机ip列表中 */

    /* match with dst ip(not ipaddr_any) *//*根据目的ip匹配四元组 */

    /* match with dst ip(ipaddr_any) */ /* 清零目的ip再匹配四元组 */

    /* debug warning */

    return NULL;
}

/* handle tcp syn */
net_err_t tcp_syn_handler(tcp_cb_t * tcp_cb)
{

}

net_err_t tcp_exmsg_handler(tcp_cb_t * tcp_cb)
{

}

net_err_t tcp_process_in(nbuf_t * nbuf, ip4addr_t * src, ip4addr_t * dst)
{
    tcp_cb_t * tcp = NULL;
    tcp_hdr_t * tcp_hdr;
    uint16_t sport = 0;
    uint16_t dport = 0;

    /* extract tcp header firstly */
    nbuf_extract_prefix(nbuf, TCP_HEADER_LEN);
    tcp_hdr = (tcp_hdr_t *)nbuf_data(nbuf);

    sport = tcp_hdr->src_port;
    dport = tcp_hdr->des_port;

    tcp = tcp_control_block_match(src, sport, dst, dport);

    if(!tcp)
        return NET_ERR_NOK;

    /* check the packet type */
    if(tcp_hdr->flag_val |= TCP_PACKET_FLAG_SYN) 
    {
        tcp_connect_process_in(tcp, dst, src, sport, dport);
    }else if(tcp_hdr->flag_val |= TCP_PACKET_FLAG_PSH)
    {

    }else if(tcp_hdr->flag_val |= TCP_PACKET_FLAG_FIN)
    {
        
    }else if(tcp_hdr->flag_val |= TCP_PACKET_FLAG_RST)
    {
        
    }else if(tcp_hdr->flag_val |= TCP_PAKCET_FLAG_URG)
    {

    }


    /* user data */

    /* cut off the tcp header */
    nbuf_header(nbuf, -TCP_HEADER_LEN);

    return NET_ERR_OK;
}

tcp_cb_t * tcp_create( void )
{

}

/* connect to remote */
net_err_t tcp_connect_process_in(tcp_cb_t * tcp, ip4addr_t * des, ip4addr_t * src, uint16_t sport, uint16_t dport)
{
    tcp_hdr_t * tcp_hdr;
    nbuf_t * nbuf = NULL;
    nbuf_alloc(&nbuf, TCP_HEADER_LEN);

    /* send syn packet */
    nbuf_extract_prefix(nbuf, TCP_HEADER_LEN);
    tcp_hdr = (tcp_hdr_t *)nbuf_data(nbuf);

    switch(tcp->state)
    {
        case TCP_STATE_CLOSED:
            tcp_hdr->src_port = _htons(sport);
            tcp_hdr->des_port = _htons(dport);

            plat_memset(&tcp_hdr->flags, 0, sizeof(tcp_hdr->flags));
            tcp_hdr->flags.syn = 1;
            uint32_t isn = tcp_isn_generate(tcp);
            tcp_hdr->seq_num = isn;
            break;
        case TCP_STATE_LISTEN:

            break;
        default:
            break;
    }
    
    return NET_ERR_OK;
}

/* disconnect from remote */
net_err_t tcp_disconnect_process_in(tcp_cb_t * tcp, ip4addr_t * des, ip4addr_t * src, uint16_t sport, uint16_t dport)
{

}

/* bind local ip and port 
 * whatever host is server or client, both can bind local ip and port.
 */
/* 服务端和客户端共用 */
net_err_t tcp_bind(tcp_cb_t * tcp_cb, ip4addr_t * local, uint16_t port)
{
    tcp_cb_t * iter = NULL;

    /* if have route */
    if( local )
    {

    }

    /* iterate all tcp control blocks */
    for(;;)
    {
        if( !iter ) 
            break;
        if( (iter->sock.net_sock.src.ip4addr.ipv == local->ipv \
            || IPV4_ADDR_ANY == iter->sock.net_sock.src.ip4addr.ipv ) \
            && tcp_cb->sock.net_sock.src.port == port )
        {
            plat_printf("err: tcp connection exists\n");
            return NET_ERR_NOK;
        }
    }

    if(local)
        tcp_cb->sock.net_sock.src.ip4addr = * local;
    else
        plat_memset(&tcp_cb->sock.net_sock.src.ip4addr, 0, sizeof(ip4addr_t));
    tcp_cb->sock.net_sock.src.port = port;

    return NET_ERR_OK;
}

net_err_t tcp_listen(tcp_cb_t * tcp_cb, uint32_t backlog)
{

}

/* tcp client api only */
net_err_t tcp_connect_to(tcp_cb_t * tcp_cb, ip4addr_t * serv_ip, uint16_t serv_port)
{
    tcp_hdr_t * tcp_hdr;
    nbuf_t * nbuf = NULL;

    if( tcp_cb->state != TCP_STATE_CLOSED )
        return NET_ERR_NOK;

    /* check local ip and port */
    if( !tcp_cb->sock.net_sock.src.port ){
        uint16_t port = tcp_port_allocate();
        if( !port ){
            plat_printf("err: tcp_port_allocate failed\n");
            return NET_ERR_NOK;
        }
        tcp_cb->sock.net_sock.src.port = port;
    }
    if( tcp_cb->sock.net_sock.src.ip4addr.ipv == IPV4_ADDR_ANY );

    /* if tcp connection exist, use hash table */
    tcp_cb_t * iter = NULL;
    for(;;)
    {
        if( !iter ) 
            break;
        if( (iter->sock.net_sock.src.ip4addr.ipv == serv_ip->ipv \
            || IPV4_ADDR_ANY == iter->sock.net_sock.src.ip4addr.ipv ) \
            && tcp_cb->sock.net_sock.src.port == serv_port )
        {
            plat_printf("err: tcp connection exists\n");
            return NET_ERR_NOK;
        }
    }

    if(NET_ERR_NOK == nbuf_alloc(&nbuf, TCP_HEADER_LEN)){
        plat_printf("err: nbuf_alloc failed\n");
        return NET_ERR_NOK;
    }
    nbuf_extract_prefix(nbuf, TCP_HEADER_LEN);
    tcp_hdr = (tcp_hdr_t *)nbuf_data(nbuf);

    plat_memset(tcp_hdr, 0, TCP_HEADER_LEN);
    tcp_hdr->flags.syn = 1;
    tcp_hdr->flags.data_off = TCP_HEADER_LEN / 4;
    tcp_hdr->flag_val = _htons(tcp_hdr->flag_val);
    tcp_hdr->seq_num  = _htons(tcp_isn_generate(tcp_cb));
    tcp_hdr->des_port = _htons(serv_port);
    tcp_hdr->src_port = _htons(tcp_cb->sock.net_sock.src.port);
    tcp_hdr->win_size = _htons(1100);
    tcp_hdr->checksum = nbuf_checksum16(nbuf, 0);

    tcp_state_set(tcp_cb, TCP_STATE_SYN_SENT);

    if(NET_ERR_NOK == ipv4_out(IPV4_HEAD_PROTOCAL_TCP, IPV4_HEAD_TTL_DEFAULT, serv_ip, &tcp_cb->sock.net_sock.src.ip4addr, nbuf)){
        tcp_state_set(tcp_cb, TCP_STATE_CLOSED);
        nbuf_free(nbuf);
        plat_printf("error: tcp send packet error\r\n");
    }

    return NET_ERR_OK;
}

/* int listen (int socketfd, int backlog) 中backlog参数的含义是已建立连接的tcp的数量，填0就行，这个参数一般来说没什么用 */

/* TCP Congestion Control begin */

/* TCP Congestion Control end */