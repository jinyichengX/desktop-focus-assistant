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

#include "el_ether.h"
#include "el_netif.h"
#include "el_ipv4.h"
#include "global.h"

extern char is_ipv4addr_broadcast(netif_t * netif, ip4addr_t * des);
extern net_err_t ipv4_in(netif_t * netif, nbuf_t * nbuf);

/* init ethernet */
net_err_t ether_init(netif_t * netif, void * args)
{
    /* check ether header size */
    if(sizeof(ether_hdr_t) != ETHER_HEADER_LEN){
        return NET_ERR_NOK;
    }

    return NET_ERR_OK;
}

/* pack ethernet net package use raw */
net_err_t ether_out_raw(netif_t * netif, uint8_t * hdw_des,\
                        nbuf_t * nbuf, uint16_t prot, uint8_t self)
{
    int size;
    uint16_t to_expand;
    ether_hdr_t * ether_hdr;
    size = NBUF_TTSZ(nbuf);

    /* check if size little than 46 */
    if(size < ETHER_MIN_TU){
        to_expand = ETHER_MIN_TU - size;

        /* too small to pack ethernet frame,resize buff and fill frame with 0 */
        if(nbuf_tail_expand(nbuf, to_expand) == NET_ERR_NOK)
            return NET_ERR_NOK;

        nbuf_seek(nbuf, size);
        
        /* set filled tail size all zero */
        nbuf_memset(nbuf, to_expand, 0);
    }

    if(nbuf_header(nbuf, sizeof(ether_hdr_t)) != NET_ERR_OK){
        return NET_ERR_NOK;
    }

    nbuf_extract_prefix(nbuf, sizeof(ether_hdr_t));

    /* fill header */
    ether_hdr = (ether_hdr_t *)nbuf_data(nbuf);
    plat_memcpy(ether_hdr->src, netif->hwaddr, ETHER_MAC_ADDR_LEN);
    plat_memcpy(ether_hdr->des, hdw_des, ETHER_MAC_ADDR_LEN);
    ether_hdr->protocal = _htons(prot);

    /* push to netif queue
    * if send to self, post to receive queue
    */
    if(self){
        /* post to receive queue */
        netif_recv_queue_post(netif, nbuf, 0xffffffff);
    }
    else{
        /* post to send queue */
        netif_send_queue_post(netif, nbuf, 0xffffffff);
    }

    return NET_ERR_OK;
}

/* pack ethernet net package use arp */
net_err_t ether_out_arp(netif_t * netif, ip4addr_t * des, nbuf_t * nbuf)
{
    arp_entry_t * arp_entry;
    const uint8_t * hdw_des;

    /* make sure ether addr rely to netif and des_mac following */

    /* 可能为受限广播和直接广播 */
    if(is_ipv4addr_broadcast(netif, des)){
        /* broadcast */
        hdw_des = ether_broadcast_addr;
        return ether_out_raw(netif, (uint8_t *)hdw_des, nbuf, ETHER_TYPE_IP, 0);
    }

    /* unicast（单播情况1：目的IP和网口ip完全一致） */
    if(IPV4_ADDR_IS_EQUAL(&netif->ipaddr, des)){
        /* send to self */
        return ether_out_raw(netif, netif->hwaddr, nbuf, ETHER_TYPE_IP, 1);
    }
    /* unicast（单播情况2：本地环回地址127.0.0.1-127.255.255.254） */
    else if(IS_IPV4_LOOPBACK(des)){
        /* send to self */
        return ether_out_raw(netif, netif->hwaddr, nbuf, ETHER_TYPE_IP, 1);
    }

    /* unicast（单播情况3：本网络内其他主机或本局域网外主机） */
    /* search mac address */
    if(NET_ERR_OK == arp_entry_match(des, &arp_entry, NULL)){
        return ether_out_raw(netif, arp_entry->hwa, nbuf, ETHER_TYPE_IP, 0);
    }else{
        /* send arp request,and send packet later */
        if(arp_resolve(netif, des, nbuf) == NULL){
            return NET_ERR_NOK;
        }
    }

    return NET_ERR_OK;
}

/* ethernet package in */
net_err_t ether_in(netif_t * netif, nbuf_t * nbuf)
{
    ether_hdr_t * ether_hdr;
    uint16_t prot;

    /* check if size is little than 46 or check nbuf if legal */
    if((nbuf->total_size < ETHER_PACKET_MIN_LEN) ||\
        (nbuf->total_size > ETHER_PACKET_MAX_LEN)){
        plat_printf("error: ethernet packet size %d\n", nbuf->total_size);
        nbuf_free(nbuf);
        return NET_ERR_NOK;
    }

    nbuf_extract_prefix(nbuf, ETHER_HEADER_LEN);
    ether_hdr = (ether_hdr_t *)nbuf_data(nbuf);

    /* if des hardware addr not equal with ff:ff:ff:ff:ff:ff
        or ethernetif hardware addr, return error */
    if( (0 != plat_memcmp(ether_broadcast_addr, ether_hdr->des, ETHER_MAC_ADDR_LEN))&&\
        ( 0 != plat_memcmp(netif->hwaddr, ether_hdr->des, ETHER_MAC_ADDR_LEN))){
        // plat_printf("error: ethernet des addr %x:%x:%x:%x:%x:%x\n",\
        // ether_hdr->des[0],ether_hdr->des[1],ether_hdr->des[2],\
        // ether_hdr->des[3],ether_hdr->des[4],ether_hdr->des[5]);
        nbuf_free(nbuf);
        return NET_ERR_NOK;
    }
#if USE_NETIF_BANDWIDTH_STATISTIC
    if( netif->recv_bandwidth_mutex ){
        sys_mutex_lock(netif->recv_bandwidth_mutex);
        netif->recv_bps += (nbuf->total_size << 3);
        sys_mutex_unlock(netif->recv_bandwidth_mutex);
    }
#endif
    prot = _ntohs(ether_hdr->protocal);

    /* cut ether header */
    nbuf_header(nbuf, -ETHER_HEADER_LEN);

    if(prot == ETHER_TYPE_IP){
#if 0
        // AUTBUS项目调试
        nbuf_free(nbuf);
        printf("11111\r\n");
        return NET_ERR_OK;
#endif
        /* ip packet */
        if(NET_ERR_NOK == ipv4_in(netif, nbuf))
        {
            return NET_ERR_NOK;
        }
    }
    else if(prot == ETHER_TYPE_ARP){
        /* arp packet */
        return arp_in(netif, nbuf);
    }
    else{
        /* unkown packet */
        plat_printf("error: ethernet unknown protocal %d\n",prot);
        nbuf_free(nbuf);
        return NET_ERR_NOK;
    }

    return NET_ERR_OK;
}
