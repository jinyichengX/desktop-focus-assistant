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

#include "el_ipv4.h"
#include "global.h"
#include "el_netif.h"
#include "el_icmpv4.h"
#include "el_udp.h"
#include "async.h"
#include "defs.h"

typedef int (*container_val_compare)(void *,void *);

#ifndef _LISTNODE2CONTAINER
#define _LISTNODE2CONTAINER(pnode,off) ((void *)(((size_t)(pnode))-(off)))
#endif
#ifndef _CONTAINER2LISTNODE
#define _CONTAINER2LISTNODE(pcont,off) ((struct list_head *)(((size_t)(pcont))+(off)))
#endif

/* 升序/降序插入列表节点（通用方法） */
//static  void list_insert_sort(container_val_compare cmp,\
//						  struct list_head * head,\
//						  void * node_container,\
//						  int node_off)
//{
//	struct list_head * head_pos = head->next;
//	void * cont_idx = _LISTNODE2CONTAINER(head_pos,node_off);
//	struct list_head *new = _CONTAINER2LISTNODE(node_container,node_off);
//	while(
//		    (!list_empty(head))&&\
//     	  (cmp(node_container,cont_idx) == 1)\
//      )
//  {
//		head_pos = head_pos->next;
//    if(head_pos == head)
//      break;
//		cont_idx = _LISTNODE2CONTAINER(head_pos,node_off);
//	}
//	__list_add(new,head_pos->prev,head_pos);
//}

net_err_t route_add(ip4addr_t * network,ip4addr_t * mask,ip4addr_t * nexthop,netif_t * netif);
void ip_frgReass_timer_proc(timer_t * timer,void * args);
char netmask_prefix_len(ip4addr_t * netmask);
net_err_t ipv4_in(netif_t * netif, nbuf_t * nbuf);

const int route_entry_node_off = offsetof(route_entry_t, node);
const int ipv4_frag_node_off = offsetof(ipv4_frag_t, sort_node);

/* ipv4 string to bytes */
net_err_t ipv4_str2ipaddr(const char * ipstr, ip4addr_t * ip)
{
    char c;
    uint8_t * p = (uint8_t*)ip;
    char n = 0;
    while ((c = *ipstr++) != '\0') {
      if ((c >= '0') && (c <= '9')) {
          n = n * 10 + c - '0';
      }
      else if (c == '.') {
          *p++ = n;
          n = 0;
      }
      else {
          return NET_ERR_NOK;
      }
    }
    *p++ = n;

    return NET_ERR_OK;
}

/* check if ip is broadcast for specific netif */
char is_ipv4addr_broadcast(netif_t * netif,ip4addr_t * des)
{
    if(
        (IS_IPV4_RESTRICTED_BROADCAST(des)) ||\
        (IS_IPV4_DIRECT_BROADCAST(&netif->ipaddr, des, &netif->mask))\
    )
        return 1;
    return 0;
}

/* init ipv4 route table */
static net_err_t ipv4_route_init(void)
{
    /* init ip route table pool */
    mempool_create(&route_entry_pool, (void *)route_entry, sizeof(route_entry_t), IP_ROUTE_ENTRY_NUM);
                  
    if(NULL == (route_entry_list_lock = sys_mutex_create())){
        return NET_ERR_NOK;
    }

    /* init route entry list */
    INIT_LIST_HEAD(&route_entry_list);

    return NET_ERR_OK;
}

/* check identifier if timeout and send sem for ip packet out */
int ip_identifier_allocate(void * pArgs)
{
    return ip_identifier ++;
}

/* ip init */
net_err_t ipv4_init(void)
{
    net_err_t err;

    if( sizeof(ipv4_hdr_t) != IPV4_HEADER_LEN )
        err = NET_ERR_NOK;

    /* create ip reass list access mutex */
    if(NULL == (ip_reass_list_lock = sys_mutex_create()))
        err |= NET_ERR_NOK;

    err |= ipv4_route_init();
    err |= net_timer_add(&ipv4_timer, ip_frgReass_timer_proc, RELOAD_FOREVER, IP_TIMER_CALLBACK_TIME, NULL);

    mempool_create(&ipv4_reass_pool, (void *)ipv4_reass_obj_pool, sizeof(ipv4_reass_t), IP_PACKET_NEED_REASSEMBLED_NUM);
    mempool_create(&ipv4_frag_pool, (void *)ipv4_frag_obj_pool, sizeof(ipv4_frag_t), IP_FRAGMENT_SUPPORTED_NUM);

    /* init ipv4 reass list */
    INIT_LIST_HEAD(&ip_reass_list);

    return (err == NET_ERR_OK)?err:NET_ERR_NOK;
}

/* get network from ip and mask */
uint32_t generate_network(ip4addr_t * ip, ip4addr_t * mask)
{
    return IPV4_ADDR_NETWORK(ip, mask);
}

static int route_entry_prefix_cmp(void * data1,void * data2)
{
    route_entry_t * cont1 = (route_entry_t *)data1;
    route_entry_t * cont2 = (route_entry_t *)data2;
    
    return (cont1->prefix_len) <= (cont2->prefix_len);
}

/* add one route entry */
net_err_t route_add(ip4addr_t * network, ip4addr_t * mask, ip4addr_t * nexthop, netif_t * netif)
{
    route_entry_t * entry;

    /* request one route entry from pool */
    if(NULL == (entry = (route_entry_t *)mempool_alloc(&route_entry_pool, 0))){
        plat_printf("warning: alloc route entry error!\r\n");
        return NET_ERR_NOK;
    }
    plat_memset(entry, 0, sizeof(route_entry_t));

    /* copy network and mask */
    entry->des_network.ipv = network->ipv;
    entry->netmask.ipv = mask->ipv;
    entry->prefix_len = netmask_prefix_len(mask);

    /* set nexthop and interface */
    entry->if_next = (void *)netif;

    if((IPV4_ADDR_VAL_GET(network) == NET_LOOPBACK)||(IPV4_ADDR_IS_EQUAL(&netif->ipaddr, network))){
        /* loopback route */
        entry->type = NET_RT_NETIF;
    }else if((!nexthop)||(IS_IPV4_ADDR_ANY(nexthop))){
        /* local network */
        entry->type = NET_RT_LOCAL;
    }else{
        /* wide network */
        entry->type = NET_RT_WIDE;
        entry->nexthop.ipv = nexthop->ipv;
    }
    /* add to entry list */
    sys_mutex_lock(route_entry_list_lock);
    list_insert_sort(route_entry_prefix_cmp, &route_entry_list, (void *)entry, route_entry_node_off);
    sys_mutex_unlock(route_entry_list_lock);

    return NET_ERR_OK;
}

/* route match, search netif to send */
route_entry_t * route_match(ip4addr_t * des)
{
    route_entry_node_t * idx;
    route_entry_t * entry;
    netif_t * netif = NULL;

    sys_mutex_lock(route_entry_list_lock);
    /* traverse route entry,search dest network */
    list_for_each(idx, &route_entry_list){
        entry = list_entry(idx, route_entry_t, node);
        if(NET_MATCH(&entry->netmask, &entry->des_network, des)){
            goto __return;
        }
    }
    return NULL;
__return:
    sys_mutex_unlock(route_entry_list_lock);
    return entry;
}

/* delete route entry by netif */
net_err_t route_delete_netif(netif_t * netif)
{
    struct list_head * pstIdx;
    route_entry_t * pstEntry;
    sys_mutex_lock(route_entry_list_lock);

    for( pstIdx = route_entry_list.next; pstIdx != &route_entry_list; pstIdx = pstIdx->next )
    {
        pstEntry = list_entry(pstIdx, route_entry_t, node);

        if( (netif_t *)pstEntry->if_next == netif ) 
            mempool_free(&route_entry_pool, (void *)pstEntry);
        else
            continue;
    }

    sys_mutex_unlock(route_entry_list_lock);
}

/* delete specific route entry */
net_err_t route_delete(ip4addr_t * network)
{
    route_entry_t * entry = NULL;
    struct list_head * pos, * tmp;

    sys_mutex_lock(route_entry_list_lock);

    /* search route entry */
    list_for_each_safe(pos, tmp, &route_entry_list)
    {
        entry = list_entry(pos, route_entry_t, node);
        if(NET_MATCH(&entry->netmask, &entry->des_network, network))
        {
            /* delete from entry list */
            list_del(&entry->node);
            break;
        }
    }

    sys_mutex_unlock(route_entry_list_lock);
    mempool_free(&route_entry_pool, (void *)entry);

    return NET_ERR_OK;
}

/* ipv4 packet fragment and put out */ 
net_err_t ip_frags_out(netif_t * netif, uint8_t prot, uint8_t ttl, uint16_t id, ip4addr_t * nexthop, ip4addr_t * des, ip4addr_t * src, nbuf_t * nbuf)
{
    struct list_head * pos,* next;
    struct list_head frag_list;
    uint16_t total_len = NBUF_TTSZ(nbuf);
    uint16_t usFragPayloadLen = 0, usPayloadLen = 0;
    nbuf_t * frag_nbuf;
    ipv4_hdr_t * ipv4_hdr;
    char tail = 0;

    INIT_LIST_HEAD(&frag_list);

    while(1) {
        /* calculate current net buf payload length */
        if(( total_len -= usFragPayloadLen ) > netif->mtu)
            usFragPayloadLen = ((netif->mtu - IPV4_HEADER_LEN) / 8) * 8;//分出来的数据载荷长度
        else
        {
            usFragPayloadLen = total_len - IPV4_HEADER_LEN;
            tail = 1;
        }

        if(NET_ERR_NOK == nbuf_alloc(&frag_nbuf, usFragPayloadLen + IPV4_HEADER_LEN)){
            goto __frag_error;
        }

        nbuf_extract_prefix(frag_nbuf, IPV4_HEADER_LEN);
        ipv4_hdr = (ipv4_hdr_t *)nbuf_data(frag_nbuf);
        ipv4_hdr->version = IPV4_HEAD_VERSION;
        ipv4_hdr->hd_len = IPV4_HEADER_LEN / 4;
        ipv4_hdr->tos = _htons(0);
        ipv4_hdr->total_len = _htons(NBUF_TTSZ(frag_nbuf));

        /* frag parameters */
        ipv4_hdr->id = _htons(id);
        ipv4_hdr->morefrag = tail ? 0 : 1;
        ipv4_hdr->dontfrag = 0;
        ipv4_hdr->reserved = 0;
        ipv4_hdr->frag_off = usPayloadLen / 8;
        ipv4_hdr->fragFlagAndOff = _htons(ipv4_hdr->fragFlagAndOff);

        ipv4_hdr->ttl = ttl ? ttl : IPV4_HEAD_TTL_DEFAULT;
        ipv4_hdr->protocal = prot;
        ipv4_hdr->checksum = 0;
        IPV4_ADDR_VAL_SET(&ipv4_hdr->src, IPV4_ADDR_VAL_GET(src));
        IPV4_ADDR_VAL_SET(&ipv4_hdr->des, IPV4_ADDR_VAL_GET(des));
        /* calculate header checksum */
        ipv4_hdr->checksum = net_checksum16(0, (void *)ipv4_hdr, IPV4_HEADER_LEN, true);

        /* transfer source nbuf to fragment */
        nbuf_transfer( nbuf, frag_nbuf, usFragPayloadLen, IPV4_HEADER_LEN + usPayloadLen, IPV4_HEADER_LEN );
        usPayloadLen += usFragPayloadLen;

        /* netif send package */
        netif_out(netif, nexthop, frag_nbuf);

        if( tail )
            break;
    }
    nbuf_free(nbuf);

    return NET_ERR_OK;
__frag_error:
    nbuf_free(nbuf);
    /* free all net buf in frag list */
    list_for_each_safe(pos, next, &frag_list){
        frag_nbuf = list_entry(pos, nbuf_t, nbuf_node);
        nbuf_free(frag_nbuf);
    }

    return NET_ERR_NOK;
}

/* pack ipv4 package and send */
net_err_t ipv4_out(uint8_t prot, uint8_t ttl, ip4addr_t * des, ip4addr_t * src, nbuf_t * nbuf)
{
    route_entry_t * entry = NULL;
    ipv4_hdr_t * ipv4_hdr;
    net_err_t err;
    ip4addr_t next_hop;
    static int nIdentifier = 0;

    if(!des)
        return NET_ERR_NOK;

    /* match route entry first */
    if(NULL == (entry = route_match(des))){
        plat_printf("err: no route entry found!\r\n");
        return NET_ERR_NOK;
    }

    /* check src address,if src empty,match route */
    if((!src)||(IS_IPV4_ADDR_ANY(src))){
        /* use matched netif ip address */
        src = &((netif_t *)entry->if_next)->ipaddr;//使用网卡的ip
    }else{
        if(!IPV4_ADDR_IS_EQUAL(src,&((netif_t *)(entry->if_next))->ipaddr)){
            plat_printf("warning: src ip address not equal with matched source ip!\r\n");
            /* use matched netif ip address */
            src = &((netif_t *)entry->if_next)->ipaddr;//路由条目中网卡ip和传入的ip不同，也使用网卡的ip
        }
    }

    /* get next hop ip address */
    if(IS_IPV4_ADDR_ANY(&entry->nexthop)){
        /* local network */
        next_hop.ipv = des->ipv;//局域网或者内部接口
    }else{
        /* remote network */
        next_hop.ipv = entry->nexthop.ipv;//外部网络，则下一跳为网关，由网关转发数据包
    }

    /* alloc size for ipv4 package header */
    if(NET_ERR_OK != (err = nbuf_header(nbuf, IPV4_HEADER_LEN))){
        plat_printf("err: alloc ipv4 header error!\r\n");
        return NET_ERR_NOK;
    }
    nIdentifier ++;//重大bug!!!!!
    // async_once_request_post(ip_identifier_allocate, NULL, WAIT_RET, &nIdentifier);//这里会阻塞等待标识id分配结束

    /* check if packet size is larger than netif mtu */
    if( ((netif_t *)entry->if_next)->mtu < NBUF_TTSZ(nbuf) )
    {
        /* the packet need split into fragments */
        return ip_frags_out(((netif_t *)entry->if_next), prot, ttl, (uint16_t)nIdentifier, &next_hop, des, src, nbuf);
    }

    /* extract ipv4 header */
    nbuf_extract_prefix(nbuf, IPV4_HEADER_LEN);

    /* ipv4 package header fill */
    ipv4_hdr = (ipv4_hdr_t *)nbuf_data(nbuf);

    ipv4_hdr->version = IPV4_HEAD_VERSION;
    ipv4_hdr->hd_len = IPV4_HEADER_LEN / 4;
    ipv4_hdr->tos = _htons(0);
    ipv4_hdr->total_len = _htons(NBUF_TTSZ(nbuf));

    /* frag parameters */
    ipv4_hdr->id = _htons((uint16_t)nIdentifier);
    ipv4_hdr->fragFlagAndOff = 0;

    ipv4_hdr->ttl = ttl ? ttl : IPV4_HEAD_TTL_DEFAULT;
    ipv4_hdr->protocal = prot;
    ipv4_hdr->checksum = 0;
    IPV4_ADDR_VAL_SET(&ipv4_hdr->src, IPV4_ADDR_VAL_GET(src));
    IPV4_ADDR_VAL_SET(&ipv4_hdr->des, IPV4_ADDR_VAL_GET(des));
    
    /* calculate header checksum */
    ipv4_hdr->checksum = net_checksum16(0, (void *)ipv4_hdr, IPV4_HEADER_LEN, true);

    /* netif send package */
    netif_out((netif_t *)entry->if_next, &next_hop, nbuf);

    return NET_ERR_OK;
}

/* check if netmask legal or not */
/* parameter netmask->ipv like 0xffffff00 */
char ipv4_netmask_legal(ip4addr_t * netmask)
{
    uint32_t ipv4_v;
    char zero_flag = 0;
    ip4addr_t ip4_cmp = {
        .ipv = 0x80000000
    };

    ipv4_v = IPV4_ADDR_VAL_GET(netmask);

    if((ipv4_v == MASK_ADDR_ANY)||(ipv4_v == MASK_ADDR_BROADCAST))
        return 0;

    ipv4_v = _htonl(ipv4_v);

    /* check every bit */
    while(ip4_cmp.ipv){
        if((ip4_cmp.ipv & ipv4_v) == 0){
            /* find one zero bit */
            zero_flag = 1;
        }
        else if(zero_flag){
            /* find one non-zero bit after zero bit,it's illegal */
            return 1;
        }
        ip4_cmp.ipv = ip4_cmp.ipv >> 1;
    }

    return 0;
}

/* get netmask prefix length */
char netmask_prefix_len(ip4addr_t * netmask)
{
#if defined(__GNUC__) && (__GNUC__ >= 4)
    return __builtin_popcount(IPV4_ADDR_VAL_GET(netmask));
#else
    uint32_t u = IPV4_ADDR_VAL_GET(netmask);
    u = (u & 0x55555555) + ((u >> 1)  & 0x55555555);
    u = (u & 0x33333333) + ((u >> 2)  & 0x33333333);
    u = (u & 0x0f0f0f0f) + ((u >> 4)  & 0x0f0f0f0f);
    u = (u & 0x00ff00ff) + ((u >> 8)  & 0x00ff00ff);
    u = (u & 0x0000ffff) + ((u >> 16) & 0x0000ffff);
    return u;
#endif
}

/* check if route entry legal or not */
char route_entry_legal(route_entry_t * entry)
{
    if( !entry )
        return -1;

    return ((ipv4_netmask_legal(&entry->netmask)) && ((entry->des_network.ipv & entry->netmask.ipv) == entry->des_network.ipv));
}

#if USE_IP_FRAGMENT_REASS_SOLUTION_A == 1
ipv4_reass_t * ip_frag_reass_entry_alloc(void)
{
    //return mempool_alloc();
}

/* sort ip fragment by frag offset */
static int ip_fragment_offset_sort(void * pIpv4Hdr1, void * pIpv4Hdr2)
{
    int unCmpRet = 0;

    ipv4_hdr_t  * pstIpv4Hdr1 = (ipv4_hdr_t *)(((ipv4_frag_t *)pIpv4Hdr1)->pubFragBuffer);
    ipv4_hdr_t  * pstIpv4Hdr2 = (ipv4_hdr_t *)(((ipv4_frag_t *)pIpv4Hdr2)->pubFragBuffer);
    pstIpv4Hdr1->fragFlagAndOff = _ntohs(pstIpv4Hdr1->fragFlagAndOff);
    pstIpv4Hdr2->fragFlagAndOff = _ntohs(pstIpv4Hdr2->fragFlagAndOff);

    unCmpRet = pstIpv4Hdr1->frag_off < pstIpv4Hdr2->frag_off;
    pstIpv4Hdr1->fragFlagAndOff = _htons(pstIpv4Hdr1->fragFlagAndOff);
    pstIpv4Hdr2->fragFlagAndOff = _htons(pstIpv4Hdr2->fragFlagAndOff);
    return unCmpRet;
}

/* check if all fragments arrived */
int ipv4_frag_all_arrived(ipv4_reass_t * pstIpReassEntry)
{
    ipv4_frag_t * pstIpFragEntry;
    struct list_head * pstPos, * pstNext;
    uint16_t usPktSizeDynamic = 0, usFragTotalSize = 0;

    list_for_each_safe(pstPos, pstNext, &pstIpReassEntry->stFragOffSortList) {
        pstIpFragEntry = list_entry( pstPos, ipv4_frag_t, sort_node);
        usFragTotalSize += pstIpFragEntry->usFragentOff * 8;
        /* if frag sum(*8) not equal with packet total size,return -1 */
        if( usFragTotalSize != usPktSizeDynamic )
            return 0;
        usPktSizeDynamic += NBUF_TTSZ(pstIpFragEntry->pubFragBuffer);
    }

    return ((usPktSizeDynamic == usFragTotalSize) && (((ipv4_hdr_t *)(pstIpFragEntry->pubFragBuffer))->morefrag == 0)) ? 1 : 0;
}

/* ipv4 fragment nbuf list splice */
static nbuf_t * nbuf_reassemble(ipv4_reass_t * pstReass)
{
    ipv4_hdr_t * pstIpv4Hdr;
    struct list_head * pstNext = pstReass->stFragOffSortList.next;
    nbuf_t * pstFragNext;
    nbuf_t * pstPacketReassed = (list_entry(pstNext, ipv4_frag_t, sort_node))->pubFragBuffer;
    ipv4_frag_t * pstFrag;

    for( ;pstNext != pstReass->stFragOffSortList.next; pstNext = pstNext->next,\
            pstFrag = list_entry(pstNext, ipv4_frag_t,\
              sort_node), pstFragNext = pstFrag->pubFragBuffer ) {
        /* trim ipv4 header,splice to first fragment */
        nbuf_header(pstFragNext, -IPV4_HEADER_LEN);
        nbuf_splice(pstPacketReassed, pstFragNext);

        mempool_free(&ipv4_frag_pool, (void *)pstFrag);
    }
    mempool_free(&ipv4_reass_pool, (void *)pstReass);

    pstIpv4Hdr = nbuf_data(pstPacketReassed);
    nbuf_extract_prefix( pstPacketReassed, IPV4_HEADER_LEN );
    pstIpv4Hdr->fragFlagAndOff = 0;

    return pstPacketReassed;
}

/* reassemble ip fragments */
net_err_t ip_frag_reassemble(netif_t * netif, nbuf_t * pubNetPacket)
{
    struct list_head * pstPos, * pstNext;
    ipv4_reass_t * pstIpReassEntry;
    ipv4_frag_t * pstIpFragToReassEntry = NULL;
    ipv4_hdr_t * pstIpv4Hdr = (ipv4_hdr_t *)nbuf_data(pubNetPacket);
    uint16_t usPacketId = _ntohs(pstIpv4Hdr->id);

    sys_mutex_lock(ip_reass_list_lock);
    /* traverse reassemble list */
    list_for_each_safe(pstPos, pstNext, &ip_reass_list) 
    {
        pstIpReassEntry = list_entry(pstPos, ipv4_reass_t, frag_node);

        if( usPacketId == pstIpReassEntry->usIdentifier)
            goto _frag_insert_and_return;
    }

    /* alloc one reassemble entry */
    if(NULL == (pstIpReassEntry = (ipv4_reass_t *)mempool_alloc(&ipv4_reass_pool, 0))) {
        sys_mutex_unlock(ip_reass_list_lock);
        return NET_ERR_NOK;
    }
    
    pstIpReassEntry->usIdentifier = usPacketId;
    INIT_LIST_HEAD(&pstIpReassEntry->frag_node);
    INIT_LIST_HEAD(&pstIpReassEntry->stFragOffSortList);

    /* insert reassemble entry to reassemble list */
    list_add_tail(&ip_reass_list, &pstIpReassEntry->frag_node);

_frag_insert_and_return:
    if(NULL == (pstIpFragToReassEntry = (ipv4_frag_t *)mempool_alloc(&ipv4_frag_pool, 0))) {
        sys_mutex_unlock(ip_reass_list_lock);
        return NET_ERR_NOK;
    }
    pstIpFragToReassEntry->pubFragBuffer = pubNetPacket;
    //pstIpFragToReassEntry->usFragentOff = pstIpv4Hdr->
    INIT_LIST_HEAD(&pstIpFragToReassEntry->sort_node);
    if( pstIpFragToReassEntry )
        list_insert_sort(ip_fragment_offset_sort, &pstIpReassEntry->stFragOffSortList, (void *)pstIpFragToReassEntry, ipv4_frag_node_off);

    /* check if all fragments arrived */
    if( 1 != ipv4_frag_all_arrived(pstIpReassEntry) ) 
    {
        /* not all fragments arrived,wait for more fragments */
        sys_mutex_unlock(ip_reass_list_lock);
        return NET_ERR_OK;
    }
    else
    {
        /* reass ip fragments and do ip process in 
         * or return to ipv4_process_in 
         */
        pubNetPacket = nbuf_reassemble(pstIpReassEntry);
        sys_mutex_unlock(ip_reass_list_lock);
#if (USE_IP_FRAGMENT_REASS_SOLUTION_A == 0)
        sys_sem_release( NULL );
#else
        ipv4_in(netif, pubNetPacket);
#endif
    }

    return NET_ERR_OK;
}

/* wait for ip fragments recombinated to one packet */
net_err_t ip_frag_reassemble_async(nbuf_t * pubNetPacket, uint32_t unTimeoutTick)
{
    ipv4_reass_t stIpReassEntry = {
        .pWaitFragmentReass = NULL,
    };
    if(NULL == (stIpReassEntry.pWaitFragmentReass = sys_sem_create(0)))
        return NET_ERR_NOK;

    if(NET_ERR_NOK == async_once_request_post( ip_frag_reassemble, (void *)pubNetPacket, NWAIT_RET, NULL))
        return NET_ERR_NOK;

    sys_sem_take(stIpReassEntry.pWaitFragmentReass, unTimeoutTick);
    sys_sem_release(stIpReassEntry.pWaitFragmentReass);
    sys_sem_destroy(stIpReassEntry.pWaitFragmentReass);

    if( 1 );

    return NET_ERR_OK;
}
#endif

/* handle input ip packet */
net_err_t ipv4_in(netif_t * netif, nbuf_t * nbuf)
{
    ipv4_hdr_t * ipv4_hdr;
    uint16_t prot;
    ip4addr_t src,des;
    net_err_t err;
    nbuf_t * nbuf_reassembled;

    nbuf_extract_prefix(nbuf, IPV4_HEADER_LEN);
    ipv4_hdr = (ipv4_hdr_t *)nbuf_data(nbuf);

    /* check checksum */
    if( net_checksum16(0,(void *)ipv4_hdr, IPV4_HEADER_LEN, 1) != 0 ){
        nbuf_free(nbuf);
        return NET_ERR_NOK;
    }

    /* check ip version,return if not v4 */
    if( ipv4_hdr->version != IPV4_HEAD_VERSION ){
        nbuf_free(nbuf);
        return NET_ERR_NOK;
    }

    /* check ipv4 header length */
    if(ipv4_hdr->hd_len * 4 != IPV4_HEADER_LEN){
        nbuf_free(nbuf);
        return NET_ERR_NOK;
    }

    /* 处理掉尾部填充的0 */
    nbuf_tail_trim(nbuf, NBUF_TTSZ(nbuf) - _ntohs(ipv4_hdr->total_len));

    /* if not broadcast or unicast packet,drop it*/
    if( (!is_ipv4addr_broadcast(netif, &ipv4_hdr->des)) && (!IPV4_ADDR_IS_EQUAL(&netif->ipaddr, &ipv4_hdr->des)) ){
//        printf("src ip addr=%d.%d.%d.%d,", ipv4_hdr->src.ipa[0], ipv4_hdr->src.ipa[1], ipv4_hdr->src.ipa[2], ipv4_hdr->src.ipa[3]);
//        printf("des ip addr=%d.%d.%d.%d\r\n", ipv4_hdr->des.ipa[0], ipv4_hdr->des.ipa[1], ipv4_hdr->des.ipa[2], ipv4_hdr->des.ipa[3]);
//        printf("invalid ip address\r\n");
        nbuf_free(nbuf);
        return NET_ERR_NOK;
    }

    /* update arp entry from ip packet */

#if 0
    /* check ttl, not necessary for devices which are not router */
    if( --ipv4_hdr->ttl < 0 ){
        /* ttl = 0, drop it */
        nbuf_free(nbuf);
        return NET_ERR_NOK;
    }
#endif

    prot = ipv4_hdr->protocal;
    IPV4_ADDR_VAL_SET(&src, IPV4_ADDR_VAL_GET(&ipv4_hdr->src));
    IPV4_ADDR_VAL_SET(&des, IPV4_ADDR_VAL_GET(&ipv4_hdr->des));

    /* check if ip packet is fraged */
    if( (ipv4_hdr->fragFlagAndOff) || (ipv4_hdr->morefrag == 1) ){
#if 1
        /* fragment not support now */
        nbuf_free(nbuf);
        plat_printf("warning: not supportted transfer protocal %d\r\n", prot);
        return NET_ERR_OK;
#endif
        /* 方式1：采用异步回调的方式，信号量超时机制检查分片是否超时 */
        /* 方式2：定时器定期检查 */
#if USE_IP_FRAGMENT_REASS_SOLUTION_A == 0
        /* 这种异步接收方式只适合多线程接收数据包，不支持单线程 */
        /* wait ip fragments reassembled */
        if(NET_ERR_OK != ip_frag_reassemble_async(nbuf, IP_FRAG_TIMEOUT_TIME))
            return NET_ERR_NOK;
#else
        ip_frag_reassemble(netif, nbuf);
#endif
    }

    if( prot == IPV4_HEAD_PROTOCAL_ICMP ){
        /* trim ipv4 header from nbuf */
        nbuf_header(nbuf, -IPV4_HEADER_LEN);
        err = icmpv4_process_in(netif, &src, nbuf);
    }
    // else if( prot == IPV4_HEAD_PROTOCAL_TCP ){ /* don't support tcp now */
    //         /* trim ipv4 header from nbuf */
    //     nbuf_header(nbuf, -IPV4_HEADER_LEN);
    //     err = tcp_process_in(netif, nbuf);
    // }
    else if( prot == IPV4_HEAD_PROTOCAL_UDP ){
	    nbuf_header(nbuf, -IPV4_HEADER_LEN);
        err = udp_process_in(nbuf, &src, &des);
    }
    else{
        /* other protocal, not support now 
         * send back protocal unreachable icmpv4 message
         */
        nbuf_tail_trim(nbuf, NBUF_TTSZ(nbuf) - IPV4_HEADER_LEN - 8); /* 保留IP首部和IP数据载荷的前8字节，共28字节 */
        icmpv4_urc_out(nbuf, ICMPV4_CODE_DEST_UNREACH_PROTO);
        if (prot == 17)
            plat_printf("warning: do not supportted transfer protocal %s\r\n", "udp");
        else if (prot == 6)
            plat_printf("warning: do not supportted transfer protocal %s\r\n", "tcp");
        else if (prot == 1)
            plat_printf("warning: do not supportted icmpv4 protocal %s\r\n");
        else
            plat_printf("warning: unknown protocal %d\r\n", prot);
        return NET_ERR_OK;
    }

    return err;
}


/* ip timer */
void ip_frgReass_timer_proc(timer_t * timer,void * args)
{
    struct list_head * pos,* next;
    struct list_head * f;
    ipv4_reass_t * ip_reass;
    ipv4_frag_t * ip_frag;

    /* check if all fragments arrived promptly */
    sys_mutex_lock(ip_reass_list_lock);

    if( list_empty(&ip_reass_list) )
    {
        sys_mutex_unlock(ip_reass_list_lock);
        return;
    }

    list_for_each_safe(pos, next, &ip_reass_list)
    {
        ip_reass = list_entry(pos, ipv4_reass_t, frag_node);
        f = ip_reass->stFragOffSortList.next;

        /* if ip reass is timeout */
        if( (--ip_reass->s_timeout) <= 0 )
        {
            /* release nbuf and delete reass node */
            for(ip_frag = list_entry(f, ipv4_frag_t, sort_node);\
                  f = f->next, ip_frag = list_entry(f, ipv4_frag_t, sort_node);\
                    f != &ip_reass->stFragOffSortList)
            {
                nbuf_free(ip_frag->pubFragBuffer);
                mempool_free(&ipv4_frag_pool, (void *)ip_frag);
            }
            mempool_free(&ipv4_reass_pool, (void *)ip_reass);
        }
        else
            continue;
    }
    sys_mutex_unlock(ip_reass_list_lock);
}