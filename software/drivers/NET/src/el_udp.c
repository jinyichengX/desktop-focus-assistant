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

/* UDP API的标准调用流程
 1. create---->(bind)---->connect----->send/recv---->destroy
 2. create---->(bind)---->(connect)---->sendto（无须bind）/recvfrom（必须先bind）---->destroy
*/
#define __init__

#include "el_udp.h"
#include "el_icmpv4.h"
#include "global.h"

/* udp init */
net_err_t udp_init(void)
{
    net_err_t err = NET_ERR_OK;
    /* check udp header length */
    if (UDP_HEADER_LEN != sizeof(udp_hdr_t)) {
        return NET_ERR_NOK;
    }
    INIT_LIST_HEAD(&udp_cb_list);/* 弃用 */
    udp_hash_table_lock = sys_mutex_create();
    if ( NULL == udp_hash_table_lock )
        return NET_ERR_NOK;
    err |= mempool_create(&udp_cb_pool, (void *)udp_cb_obj_pool, sizeof(udp_t), UDP_MAX_CONNECTION_NUM);
    err |= (net_err_t)hash_init(&udp_cb_hash_tbl, murmur_make, QUADRUPLE_SIZE, HASH_TBL_BUCKET_SIZE_43);
    return err;
}

udp_t * udp_create(void)
{
    udp_t * udp;
    udp = (udp_t *)mempool_alloc(&udp_cb_pool, 0);
    if (udp){
        plat_memset(udp, 0, sizeof(udp_t));
    }else goto _error;
    if (SYS_SEM_INVALID == (udp->net_sock.sock.notify = sys_sem_create(0))) {
        goto _error;
    }
    if (SYS_SEM_INVALID == (udp->net_sock.sock.lock = sys_mutex_create())) {
        sys_sem_destroy(udp->net_sock.sock.lock);
        goto _error;
    }
    INIT_LIST_HEAD(&udp->node);
    INIT_LIST_HEAD(&udp->net_sock.sock.recv_list);
    return udp;
_error:
    return (udp_t *)0;
}

/* allocate a dynamic port in the range of 49152 ~ 65535 */
static uint16_t udp_dyn_port_alloc(void)
{
    int port = 0;
    for (port = REGISTERED_PORT_MAX + 1; port <= DYNAMIC_PORT_MAX; ++ port)
    {
        /* iterate all udp control block */
        int idx = -1;
        hash_node_t * iter;
        int valid_flag;
        while (idx ++ < udp_cb_hash_tbl.bucket_count - 1)
        {
            valid_flag = 1;
            iter = udp_cb_hash_tbl.buckets[idx];
            if (!iter)
                continue;
            for (;iter != NULL; iter = iter->next)
            {
                if (((endpoint_t *)(iter->key))->port == port)
                {
                    valid_flag = 0;
                    break;
                }
            }
            if (valid_flag == 0) break;
        }
        if (valid_flag)
            break;
    }
    return port;
}

#if 0
void udp_hash_tbl_print_info(void)
{
    plat_printf("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\r\n");
    int idx = -1;
    hash_node_t * iter;
    udp_t * udp;
    while(idx ++ < udp_cb_hash_tbl.bucket_count - 1)
    {
        iter = udp_cb_hash_tbl.buckets[idx];
        if( !iter  )
            continue;
        plat_printf("current bucket index : %d : \r\n", idx);
        for( ;iter != NULL; iter = iter->next)
        {
            udp = (udp_t *)(iter->value);
            plat_printf("source ip : %d.%d.%d.%d, source port : %d, remote ip : %d.%d.%d.%d, remote port : %d\r\n", \
                udp->net_sock.src.ip4addr.ipa[0], udp->net_sock.src.ip4addr.ipa[1], udp->net_sock.src.ip4addr.ipa[2], udp->net_sock.src.ip4addr.ipa[3], udp->net_sock.src.port, \
                udp->net_sock.dst.ip4addr.ipa[0], udp->net_sock.dst.ip4addr.ipa[1], udp->net_sock.dst.ip4addr.ipa[2], udp->net_sock.dst.ip4addr.ipa[3], udp->net_sock.dst.port);
        }
    }
}
#else
#define udp_hash_tbl_print_info()
#endif

/* 测试用 */
net_err_t generate_base_packet(nbuf_t ** nbuf, void * buffer, uint16_t len)
{
    if (NET_ERR_NOK == nbuf_alloc(nbuf, len))
        return NET_ERR_NOK;

    nbuf_write(* nbuf, buffer, len);

    return NET_ERR_OK;
}

/* udp send packet */
/* suppose src and des ip is correct or src is empty */
net_err_t udp_out_raw(ip4addr_t * des, ip4addr_t * src, uint16_t sport, uint16_t dport, nbuf_t * nbuf)
{
    udp_hdr_t * udp_hdr;
    net_err_t err = NET_ERR_NOK;
    route_entry_t * entry;
    uint16_t chksum = 0;
    uint8_t ZeroAndProt[2] = {0, IPV4_HEAD_PROTOCAL_UDP};

    /* 确定伪首部中的源目ip */
    if ((!src) || (IS_IPV4_ADDR_ANY(src))) {
        if (NULL == (entry = route_match(des))) {
            return NET_ERR_NOK;
        }
        src = &((netif_t *)entry->if_next)->ipaddr;//使用网卡的ip
    }

    /* add udp header */
    if (NET_ERR_OK != nbuf_header(nbuf, UDP_HEADER_LEN)) {
        goto __return;
    }

    /* extract udp header */
    nbuf_extract_prefix(nbuf, UDP_HEADER_LEN); 

    /* fill udp header */
    udp_hdr = (udp_hdr_t *)nbuf_data(nbuf);
    udp_hdr->sport = _htons(sport);
    udp_hdr->dport = _htons(dport);
    /* udp len = udp header len+payload len */
    /* udp首部长度字段包含了1. udp首部长度（一般是20字节）2. udp携带的数据载荷长度 */
    udp_hdr->len = _htons(NBUF_TTSZ(nbuf));  
    udp_hdr->checksum = 0;

    /* calculate checksum */
    chksum = net_checksum16(chksum, (void *)src, IPV4_ADDR_LEN, false);
    chksum = net_checksum16(chksum, (void *)des, IPV4_ADDR_LEN, false);
    chksum = net_checksum16(chksum, (void *)ZeroAndProt, 2, false);
    chksum = net_checksum16(chksum, (void *)&udp_hdr->len, 2, false);
    chksum = nbuf_checksum16(nbuf, chksum);

    udp_hdr->checksum = chksum;

    /* send packet to ip module */
    if (err = ipv4_out(IPV4_HEAD_PROTOCAL_UDP,IPV4_HEAD_TTL_DEFAULT, des, src, nbuf)) {
        plat_printf("error: udp send packet error\r\n");
    }

__return:
    return err;
}

/* generate udp packet and send out */
/* 测试用 */
net_err_t udp_out(ip4addr_t * des, ip4addr_t * src, uint16_t sport, uint16_t dport, void * buffer, unsigned short len)
{
    nbuf_t * nbuf;
    net_err_t err = NET_ERR_OK;
    /* generate udp packet */
    if (NET_ERR_OK != (err = generate_base_packet(&nbuf, buffer, len))) {
        return err;
    }
    /* send packet */
    if (NET_ERR_OK != (err = udp_out_raw(des, src, sport, dport, nbuf))) {
        return err;
    }

    return err;
}

/* udp process packet in */
net_err_t udp_process_in(nbuf_t * nbuf, ip4addr_t * src, ip4addr_t * dst)
{
    udp_t * udp = NULL;
    udp_hdr_t * udp_hdr;
    uint16_t chksum = 0;
	endpoint_t ep[2] = {0};
    uint8_t fill[2] = {0, IPV4_HEAD_PROTOCAL_UDP};
    uint16_t port_local, port_remote;
    /* extract udp header */
    nbuf_extract_prefix(nbuf, UDP_HEADER_LEN);

    udp_hdr = (udp_hdr_t *)nbuf_data(nbuf);

    port_local = _ntohs(udp_hdr->dport);
    port_remote = _ntohs(udp_hdr->sport);

    endpoint_t from1 = {.ip4addr = * src, .port = port_remote};
    /* match udp_t block struct to link packet */
#if 0
    udp_cb_node_t * iter;
    for(iter = udp_cb_list.next;\
        iter != &udp_cb_list;\
        iter = iter->next, udp = NULL)
    {
        udp = list_entry(iter, udp_t, node);

        /* 不需要完全匹配，这段逻辑有问题 */
        if( (IPV4_ADDR_IS_EQUAL( dst,&udp->net_sock.src.ip4addr )) &&/* 数据包目的IP和本地源IP匹配 */\
            (_ntohs(udp_hdr->dport) == udp->net_sock.src.port) &&/* 数据包目的端口号和本地源端口号匹配 */\
            (IPV4_ADDR_IS_EQUAL( src,&udp->net_sock.dst.ip4addr )) &&/* 数据包源IP和本地目的IP匹配 */\
            (_ntohs(udp_hdr->sport) == udp->net_sock.dst.port)/* 数据包源端口号和本地目的端口号匹配 */\
          )
            break;
    }
#endif
    {
        /* case 1: match (local)ip:port, (remote)ip:port */

		ep[0].ip4addr = * dst;
		ep[0].port = port_local;
		ep[1].ip4addr = * src;
		ep[1].port = port_remote;
        if (hash_search(&udp_cb_hash_tbl, (unsigned char *)ep, &udp) == 0) {
            if (udp->net_sock.src.port != port_local) {
                goto unreach_handler;
            }else goto post_handler;
        }
        /* case 2: match (local)ip:port, (remote)0:0 */
        ep[1].ip4addr.ipv = 0;
        ep[1].port = 0;
        if (hash_search(&udp_cb_hash_tbl, (unsigned char *)ep, &udp) == 0) {
            if (udp->net_sock.src.port != port_local) {
                goto unreach_handler;
            }else goto post_handler;
        }
        /* case 3: match (local)0:port, (remote)ip:port */
        ep[0].ip4addr.ipv = 0;
        ep[1].ip4addr = * src;
        ep[1].port = port_remote;
        if (hash_search(&udp_cb_hash_tbl, (unsigned char *)ep, &udp) == 0) {
            if (udp->net_sock.src.port != port_local) {
                goto unreach_handler;
            }else goto post_handler;
        }
        /* case 4: match (local)0:port, (remote)0:0 */
        ep[1].ip4addr.ipv = 0;
        ep[1].port = 0;
        if (hash_search(&udp_cb_hash_tbl, (unsigned char *)ep, &udp) == 0) {
            if (udp->net_sock.src.port != port_local) {
                plat_printf("what fuck happened when match udp block\r\n");
                goto unreach_handler;
            }else goto post_handler;
        }else {
            /* udp control block donot exsit */
            goto unreach_handler;
        }
    }
post_handler:
    chksum = net_checksum16(chksum, (void *)src, IPV4_ADDR_LEN, false);
    chksum = net_checksum16(chksum, (void *)dst, IPV4_ADDR_LEN, false);
    chksum = net_checksum16(chksum, (void *)fill, 2, false);
    chksum = net_checksum16(chksum, (void *)&udp_hdr->len, 2, false);
    chksum = nbuf_checksum16(nbuf, chksum);
    if (chksum != 0) {
        nbuf_free(nbuf);
        plat_printf("error: udp packet checksum err\r\n");
        return NET_ERR_NOK;
    }
    plat_memcpy(nbuf_data(nbuf), &from1, sizeof(endpoint_t));
    /* insert packet to matched sock list or queue */
    sys_mutex_lock(udp->net_sock.sock.lock);
    list_add_tail(&nbuf->nbuf_node, &udp->net_sock.sock.recv_list);
    sys_mutex_unlock(udp->net_sock.sock.lock);
    /* sem post */
    sys_sem_release(udp->net_sock.sock.notify);

    return NET_ERR_OK;
unreach_handler:
    nbuf_free(nbuf);
    if (!udp) {
        /* send port unreachable icmp packet */
        // icmpv4_report_err_out(ICMPV4_TYPE_DEST_UNREACH, ICMPV4_CODE_DEST_UNREACH_PORT, nbuf);
        plat_printf("info: udp port unreachable\r\n");
        return NET_ERR_NOK;
    }
    return NET_ERR_NOK;
}

__init__ net_err_t udp_bind(udp_t * udp, endpoint_t * local)
{
    /* do not allow bind twice */
    if (udp_state_is_set(udp, UDP_STATE_BIND)) {
        plat_printf("info: udp control block has bound a port\r\n");
        return NET_ERR_NOK;
    }
    /* if netif(ip) is added */
    if (!IS_IPV4_ADDR_ANY(&local->ip4addr)\
         && !netif_ip_match(&local->ip4addr)) {
        plat_printf("error: bind error, local ip not found\r\n");
        return NET_ERR_NOK;
    }
    sys_mutex_lock(udp_hash_table_lock);
    int idx = -1;
    hash_node_t * iter;
    udp_t * u;
    while (idx ++ < udp_cb_hash_tbl.bucket_count - 1) {
        iter = udp_cb_hash_tbl.buckets[idx];
        if (!iter)
            continue;
        for (;iter != NULL; iter = iter->next) {
            u = (udp_t *)(iter->value);
            if (u->net_sock.src.port == local->port) {
                if (IS_IPV4_ADDR_ANY(&local->ip4addr)) {
                    goto _ret_error;
                }else if (IS_IPV4_ADDR_ANY(&u->net_sock.src.ip4addr)) {
                    goto _ret_error;
                }else if (IPV4_ADDR_IS_EQUAL(&u->net_sock.src.ip4addr, &local->ip4addr)) {
                    goto _ret_error;
                }
            }else continue;
        }
    }
    udp->net_sock.src.ip4addr = local->ip4addr;
    udp->net_sock.src.port = local->port;
    hash_insert(&udp_cb_hash_tbl, (char *)&udp->net_sock.src, (void *)udp);
    sys_mutex_unlock(udp_hash_table_lock);
    udp_state_set(udp, UDP_STATE_BIND);
    udp_hash_tbl_print_info();
    return NET_ERR_OK;
_ret_error:
    sys_mutex_unlock(udp_hash_table_lock);
    plat_memset(&udp->net_sock.src, 0, QUADRUPLE_SIZE);
    udp_hash_tbl_print_info();
    return NET_ERR_NOK;
}

/* can be called only once */
__init__ net_err_t udp_connect(udp_t * udp, endpoint_t * remote)
{
    route_entry_t * route;
    udp_t * udp_exist = NULL;
    /* if remote ip can route to */
    if (!(route = route_match(&remote->ip4addr))) {
        return NET_ERR_NOK;
    }
    if (!udp_state_is_set(udp, UDP_STATE_BIND)) {
        if(IS_IPV4_ADDR_ANY(&udp->net_sock.src.ip4addr)) {
            ip4addr_t * local = &udp->net_sock.src.ip4addr;
            uint32_t ip_val = ((netif_t *)(route->if_next))->ipaddr.ipv;
            IPV4_ADDR_VAL_SET(local, ip_val);
        }
        /* allocate a dynamic port */
        sys_mutex_lock(udp_hash_table_lock);
        udp->net_sock.src.port = udp_dyn_port_alloc();
        sys_mutex_unlock(udp_hash_table_lock);
        if ( udp->net_sock.src.port == 0 )
            return NET_ERR_NOK;
        if (NET_ERR_OK != udp_bind(udp, &udp->net_sock.src))
            return NET_ERR_NOK;
    }
    sys_mutex_lock(udp_hash_table_lock);
    if (0 == hash_search(&udp_cb_hash_tbl, (char *)&udp->net_sock.src, &udp_exist))
        hash_delete(&udp_cb_hash_tbl, (char *)&udp->net_sock.src);
    else {
        plat_printf("error: udp connect incredible fault!\r\n");
        sys_mutex_unlock(udp_hash_table_lock);
        return NET_ERR_NOK;
    }
    udp->net_sock.dst.ip4addr = remote->ip4addr;
    udp->net_sock.dst.port = remote->port;
    hash_insert(&udp_cb_hash_tbl, (char *)&udp->net_sock.src, (void *)udp);
    sys_mutex_unlock(udp_hash_table_lock);
    udp_state_set(udp, UDP_STATE_CONNECT);
    udp_hash_tbl_print_info();
    return NET_ERR_OK;
}

net_err_t udp_sendto(udp_t * udp, void * buf, uint16_t size, ip4addr_t * dst, uint16_t port)
{
    nbuf_t * nbuf;
    ip4addr_t * src;
    uint16_t sport;
    route_entry_t * route;
#if 1
    /* 对于一个已connect的udp，远程ip和端口必须等于connect的ip和port */
    if (udp_state_is_set(udp, UDP_STATE_CONNECT) &&\
        (!IPV4_ADDR_IS_EQUAL(&udp->net_sock.dst.ip4addr, dst) || udp->net_sock.dst.port != port)) {
        return NET_ERR_NOK;
    }
#else
    /* 或者直接return（后来发现不行，因为udp_send会调用此函数） */
    if(udp_state_is_set(udp, UDP_STATE_CONNECT))
        return NET_ERR_NOK;
#endif

    if (!(route = route_match(dst))) {
        return NET_ERR_NOK;
    }
    /* 不确定是不是线程安全的，一个udp控制块最好在一个线程使用 */
    if (!udp_state_is_set(udp, UDP_STATE_BIND)) {
        if (IS_IPV4_ADDR_ANY(&udp->net_sock.src.ip4addr)) {
            ip4addr_t * local = &udp->net_sock.src.ip4addr;
            uint32_t ip_val = ((netif_t *)(route->if_next))->ipaddr.ipv;
            IPV4_ADDR_VAL_SET(local, ip_val);
        }
        /* allocate a dynamic port */
        sys_mutex_lock(udp_hash_table_lock);
        udp->net_sock.src.port = udp_dyn_port_alloc();
        sys_mutex_unlock(udp_hash_table_lock);
        if ( udp->net_sock.src.port == 0 )
            return NET_ERR_NOK;
        if (NET_ERR_OK != udp_bind(udp, &udp->net_sock.src))
            return NET_ERR_NOK;
    }

    src = &udp->net_sock.src.ip4addr;
    sport = udp->net_sock.src.port;
    if (NET_ERR_NOK == nbuf_alloc(&nbuf, size))
        return NET_ERR_NOK;
    
    nbuf_write(nbuf, buf, size);
    if (NET_ERR_NOK == udp_out_raw(dst, src, sport, port, nbuf)) {
        nbuf_free(nbuf);
        return NET_ERR_NOK;
    }
    return NET_ERR_OK;
}

net_err_t udp_send(udp_t * udp, void * buf, uint16_t size)
{
    ip4addr_t * dst = &udp->net_sock.dst.ip4addr;
    uint16_t port = udp->net_sock.dst.port;
    /* 不确定是不是线程安全的，一个udp控制块最好在一个线程使用 */
    return udp_state_is_set(udp, UDP_STATE_CONNECT) ?\
    udp_sendto(udp, buf, size, dst, port) : NET_ERR_NOK;
}

/* udp recv packet */
net_err_t udp_recvfrom(udp_t * udp, void * buf, uint16_t size, ip4addr_t * remote, uint16_t * port, uint16_t * len)
{
    nbuf_t * nbuf;
    net_err_t err = NET_ERR_OK;
    size_t recv_len;
    endpoint_t * from;

    if (udp_state_is_set(udp, UDP_STATE_CONNECT) ||\
        !udp_state_is_set(udp, UDP_STATE_BIND))
        return NET_ERR_NOK;

    /* fetch udp nbuf from list */
    sys_sem_take(udp->net_sock.sock.notify, 0xffffffff);
    sys_mutex_lock(udp->net_sock.sock.lock);
    if (list_empty(&udp->net_sock.sock.recv_list)) {
        /* what fuck is that??!!! */
        plat_printf("error: udp recvfrom error, why recv_list is empty\r\n");
		sys_mutex_unlock(udp->net_sock.sock.lock);
        return NET_ERR_NOK;
    }
    nbuf = list_first_entry(&udp->net_sock.sock.recv_list, nbuf_t, nbuf_node);
    list_del(&nbuf->nbuf_node);
    sys_mutex_unlock(udp->net_sock.sock.lock);

    from = (endpoint_t *)nbuf_data(nbuf);
    if (remote) {
        * remote = from->ip4addr;
    }
    if (port) {
        * port = from->port;
    }

    nbuf_header(nbuf, - UDP_HEADER_LEN);

    recv_len = NBUF_TTSZ(nbuf);
    recv_len = recv_len > size ? size : recv_len;

    /* copy data from nbuf to user buffer */
    nbuf_acc_reset(nbuf);
    nbuf_read(nbuf, buf, recv_len);
    nbuf_free(nbuf);    /* 丢弃没读完的脏包 */
    * len = recv_len;

    return err;
}

net_err_t udp_recv(udp_t * udp, void * buf, uint16_t size, uint16_t * len)
{
    return NET_ERR_OK;
}

/* 别乱用 */
net_err_t udp_destroy(udp_t * udp)
{
    hash_delete(&udp_cb_hash_tbl, (char *)&udp->net_sock.src);
    sys_sem_destroy(udp->net_sock.sock.notify);
    sys_mutex_destroy(udp->net_sock.sock.lock);
    mempool_free(&udp_cb_pool, (void *)udp);
    return NET_ERR_OK;
}