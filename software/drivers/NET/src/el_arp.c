
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

#include "el_arp.h"
#include "el_netif.h"
#include "global.h"
#include "el_ether.h"

static void arp_timer_proc(timer_t * tmr, void * args);

/* arp raw output */
static net_err_t arp_out_raw(netif_t * netif, uint16_t op_code, uint8_t * d_mac, ip4addr_t * d_ip)
{
    nbuf_t * nbuffer;
    arp_hdr_t * arp_hdr;

    /* alloc nbuf for arp packet */
    if(NET_ERR_NOK == nbuf_alloc(&nbuffer, ARP_PACKET_SIZE)){
        return NET_ERR_NOK;
    }

    /* extract constant arp packet */
    nbuf_extract_prefix(nbuffer, ARP_PACKET_SIZE);

    /* fill arp header */
    arp_hdr = (arp_hdr_t *)nbuf_data(nbuffer);

    arp_hdr->hw_type = _htons(1);
    arp_hdr->prot_type = _htons(ARP_PROT_TYPE_IP);
    arp_hdr->hw_size = ETHER_MAC_ADDR_LEN;
    arp_hdr->prot_size = IPV4_ADDR_LEN;
    arp_hdr->opcode = _htons(op_code);

    plat_memcpy( arp_hdr->s_mac, netif->hwaddr, ETHER_MAC_ADDR_LEN);
    IPV4_ADDR_VAL_SET(&arp_hdr->s_ip, IPV4_ADDR_VAL_GET(&netif->ipaddr));
    plat_memcpy( arp_hdr->d_mac, d_mac, ETHER_MAC_ADDR_LEN);
    IPV4_ADDR_VAL_SET(&arp_hdr->d_ip, IPV4_ADDR_VAL_GET(d_ip));

    /* 如果ARP包目的MAC全0，则是ARP请求，以太网帧目的MAC为广播MAC */
    if(0 == plat_memcmp(ether_addr_any, d_mac, ETHER_MAC_ADDR_LEN)){
        d_mac = (uint8_t *)ether_broadcast_addr;
    }

    /* send arp packet */
    return ether_out_raw(netif, d_mac, nbuffer, ETHER_TYPE_ARP,0);
}

/* send arp request */
static inline net_err_t arp_request(netif_t * netif, ip4addr_t * d_ip)
{
    return arp_out_raw(netif, ARP_OPCODE_REQUEST, (uint8_t *)ether_addr_any, d_ip);
}

/* send arp reply */
static inline net_err_t _arp_reply(netif_t * netif, uint8_t * d_mac, ip4addr_t * d_ip)
{
    return arp_out_raw(netif, ARP_OPCODE_REPLY, d_mac, d_ip);
}

/* send arp reply from nbuf */
static inline net_err_t arp_reply(netif_t * netif, nbuf_t * nbuf)
{
    arp_hdr_t * arp_hdr;

    /* set arp packet constant */
    nbuf_extract_prefix(nbuf, ARP_PACKET_SIZE);
    arp_hdr = (arp_hdr_t *)nbuf_data(nbuf);

    /* send arp packet
     * get dest ip and ether addr from arp header
     */
    return _arp_reply(netif, (uint8_t *)arp_hdr->s_mac, &arp_hdr->s_ip);
}

/* arp no reply request */
net_err_t arp_announcement(netif_t * netif)
{
    return arp_request(netif, &netif->ipaddr);
} 

/* arp table init */
static inline net_err_t arp_entry_cache_init(void)
{
    return mempool_create(&arp_entry_pool, (void *)arp_table, sizeof(arp_entry_t), ARP_ENTRY_NUM);
}

/* arp init */
net_err_t arp_init(void)
{
    if( sizeof(arp_hdr_t) != ARP_PACKET_SIZE )
        goto _arp_init_fail;

    /* create arp entry access mutex */
    if(NULL == (arp_entry_list_lock = sys_mutex_create()))
        goto _arp_init_fail;

    /* init arp table */
    if(NET_ERR_NOK == arp_entry_cache_init())
        goto _arp_init_fail;

    net_timer_add(&arp_timer, arp_timer_proc, RELOAD_FOREVER, ARP_TIMER_CALLBACK_TIME, NULL);
    /* init arp entry list */
    INIT_LIST_HEAD(&arp_entry_list);

    return NET_ERR_OK;
  _arp_init_fail:
    if(NULL != arp_entry_list_lock)
        sys_mutex_destroy(arp_entry_list_lock);

    return NET_ERR_NOK;
}

/* request for an empty arp entry */
static arp_entry_t * arp_entry_alloc(netif_t * netif)
{
    arp_entry_t * entry = NULL;

    /* request one arp entry from pool */
    if(NULL == (entry = (arp_entry_t *)mempool_alloc(&arp_entry_pool, 0))){
        plat_printf("warning: alloc arp entry error!\r\n");
        return NULL;
    }

    plat_memset(entry, 0, sizeof(arp_entry_t));

    if(NULL == (entry->sem = sys_sem_create(0))){
        mempool_free(&arp_entry_pool, (void *)entry);
        return NULL;
    }

    INIT_LIST_HEAD(&entry->buf_pend);
    INIT_LIST_HEAD(&entry->entry_node);
    entry->state = ARP_ENTRY_FREE;
    entry->netif = netif;

    return entry;
}

/* free an arp entry */
void arp_entry_free_static(arp_entry_t * entry)
{
    arp_entry_t * match = NULL;
    struct list_head * pos, * next;
    if( !entry )
        return;
    sys_mutex_lock(arp_entry_list_lock);
    list_for_each_safe(pos, next, &arp_entry_list)
    {
        match = list_entry(pos, arp_entry_t, entry_node);
        if( entry == match )
        {
            list_del(pos);
            break;
        }
    }
    if(entry == match)
        mempool_free(&arp_entry_pool, (void *)entry);
    sys_mutex_unlock(arp_entry_list_lock);
}

static void arp_entry_free(arp_entry_t * entry)
{
    mempool_free(&arp_entry_pool, (void *)entry);
}

/* arp entry init */
static void arp_entry_init(arp_entry_t * entry, ip4addr_t * ip, uint8_t * mac)
{
    /* init arp entry */
    plat_memcpy(&entry->ip, ip, IPV4_ADDR_LEN);
    if(mac)
        plat_memcpy(entry->hwa, mac, ETHER_MAC_ADDR_LEN);

    entry->t_alive = ARP_ENTRY_ALIVE_TIME;
    entry->type = ARP_ENTRY_DYNAMIC;

    INIT_LIST_HEAD(&entry->buf_pend);
}

/* match ether address from arp entry table */
net_err_t arp_entry_match(ip4addr_t * ip, arp_entry_t ** arp_entry, arp_entry_t ** oldest)
{
    arp_entry_t * entry;
    struct list_head * pos, * next;

    sys_mutex_lock(arp_entry_list_lock);
    /* search arp table */
    list_for_each_safe(pos, next, &arp_entry_list)
    {
        entry = list_entry(pos, arp_entry_t, entry_node);

        /* match one entry */
        if(IPV4_ADDR_IS_EQUAL(ip,&entry->ip)){
            /* if entry type is static,ether addr is recorded,success
             * else if entry type is dynamic,then watch entry state
             * if ARP_ENTRY_USING,return success too
             */
            if((entry->type == ARP_ENTRY_STATIC)||(entry->state == ARP_ENTRY_USING)){
                *arp_entry = entry;
                /* move entry to first node for next use
                 * Add this ARP entry to the head of 
                 * the list to speed up the next query 
                 */
                list_del(pos);
                list_add(pos, &arp_entry_list);
                sys_mutex_unlock(arp_entry_list_lock);

                return NET_ERR_OK;
            }
            else if(entry->state == ARP_ENTRY_PENDING){
                *arp_entry = entry;
                break;
            }
        }
        /* if not match, record the oldest entry */
        else if( (entry->state == ARP_ENTRY_USING ) && ( NULL != oldest ) ){
            /* if entry type is static,ignore it */
            if( entry->type == ARP_ENTRY_STATIC )
                continue;
            if( *oldest == NULL )
                *oldest = entry;
            else if( (*oldest)->t_alive < entry->t_alive )
                *oldest = entry;
        }
    }
    sys_mutex_unlock(arp_entry_list_lock);

    return NET_ERR_NOK;
}

/* set entry pending */
static inline void arp_entry_state_pend(arp_entry_t * entry)
{
    entry->state = ARP_ENTRY_PENDING;
    entry->t_alive = ARP_REQUEST_COUNT;
}

/* mount nbuf packet to pend list */
static inline void arp_entry_mount(arp_entry_t * entry, nbuf_t * nbuf)
{
    list_add_tail(&nbuf->nbuf_node,&entry->buf_pend);
}

/* release pending net buf */
static inline void arp_entry_release_pending_nbuf(arp_entry_t * pend)
{
    nbuf_t * nbuf;
    netif_t * netif;
    struct list_head * pos, * next;
    if( NULL == pend )
        return;
    netif = pend->netif;

    /* make sure hardware addr in entry is valid */
    if(0 == plat_memcmp(ether_addr_any, pend->hwa, ETHER_MAC_ADDR_LEN))
        return;

    list_for_each_safe(pos, next, &pend->buf_pend)
    {
        nbuf = list_entry(pos, nbuf_t, nbuf_node);

        /* delete nbuf node from pend list */
        list_del(&nbuf->nbuf_node);

        /* try best to delivery packet 
         * no matter the result is ok or not
         */
        ether_out_raw(netif, pend->hwa, nbuf, ETHER_TYPE_IP, 0);
    }
}

/* cannot find arp entry, resolve it */
arp_entry_t * arp_resolve(netif_t * netif, ip4addr_t * d_ip, nbuf_t * nbuf)
{
    arp_entry_t * entry = NULL;
    arp_entry_t * oldest = NULL;

    /* search arp entry to use */
    if(NET_ERR_OK == arp_entry_match(d_ip, &entry, &oldest))
        return entry;

    /* if arp entry state is pending
     * if not,alloc a empty entry
     */
    /* is not pending state(found none) */
    if((entry == NULL) && ( NULL == (entry = arp_entry_alloc(netif)) ))
    {
        /* may be all arp entry is pend
         * waiting for arp reply,fail and return
         */
        if( oldest == NULL )
            return NULL;

        /* give up the oldest using arp entry
         * and reinit it
         */
        oldest->netif = netif;
        arp_entry_init(oldest, d_ip, NULL);
        arp_entry_state_pend(oldest);
        goto __arp_request;
    }
    else if(entry->state == ARP_ENTRY_PENDING){
        goto __arp_request;
    }

    /* init arp entry */
    arp_entry_init(entry, d_ip, NULL);

    /* set arp entry state pending */
    arp_entry_state_pend(entry);

    sys_mutex_lock(arp_entry_list_lock);
    list_add(&entry->entry_node, &arp_entry_list);
    sys_mutex_unlock(arp_entry_list_lock);

__arp_request:
    /* if not found arp entry to use
     * mount nbuf packet to pend list
     * finally,send arp request
     */
    arp_entry_mount(entry, nbuf);
    arp_request(netif, d_ip);

    if(entry){
        sys_sem_take(entry->sem, ARP_REQUEST_COUNT * ARP_TIMER_CALLBACK_TIME);
    }

    return entry;
}

/* record one new arp entry */
static net_err_t arp_entry_record(netif_t * netif, arp_entry_t * ety, ip4addr_t * ip, uint8_t * mac, arp_type_t type)
{
    net_err_t err;
    arp_entry_t * entry = NULL;

    if( ety == NULL )
    {
        if( type != ARP_ENTRY_STATIC )
            goto _alloc_and_record;
        else
            return NET_ERR_NOK;
    }

    /* check if arp entry is empty */
    if( (0 != plat_memcmp(mac, ety->hwa, ETHER_MAC_ADDR_LEN)) || ( ety->state == ARP_ENTRY_PENDING ) ){
        /* flush ether addr in the entry */
        plat_memcpy(ety->hwa, mac, ETHER_MAC_ADDR_LEN);

        ety->state = ARP_ENTRY_USING;
    }
    /* update arp entry alive time */
    ety->t_alive = ARP_ENTRY_ALIVE_TIME;

    return NET_ERR_OK;

_alloc_and_record:
    /* alloc one arp entry */
    if( NULL == (entry = arp_entry_alloc(netif)) ){
        return NET_ERR_NOK;
    }

    /* init arp entry */
    arp_entry_init(entry, ip, mac);

    /* arp entry set using */
    entry->state = ARP_ENTRY_USING;

    /* add to arp table */
    sys_mutex_lock(arp_entry_list_lock);
    list_add(&entry->entry_node, &arp_entry_list);
    sys_mutex_unlock(arp_entry_list_lock);

    return NET_ERR_OK;
}

/* add one static arp entry */
/* 这个函数暂时能用，还不知道对其他逻辑有没有影响 */
net_err_t arp_entry_add_static(ip4addr_t * ip, uint8_t * mac)
{
    arp_entry_t * entry;

    if( NULL == (entry = mempool_alloc(&arp_entry_pool, 0)) )
        return NET_ERR_NOK;

    plat_memcpy(entry->hwa, mac, ETHER_MAC_ADDR_LEN);
    plat_memcpy(&entry->ip, ip, IPV4_ADDR_LEN);
    entry->type = ARP_ENTRY_STATIC;
    entry->state = ARP_ENTRY_USING;

    sys_mutex_lock(arp_entry_list_lock);
    list_add(&entry->entry_node, &arp_entry_list);
    sys_mutex_unlock(arp_entry_list_lock);
}

/* handle arp input packet */
net_err_t arp_in(netif_t * netif, nbuf_t * nbuf)
{
    ip4addr_t from;
    uint8_t hdw_from[ETHER_MAC_ADDR_LEN];
    arp_entry_t * entry = NULL;
    arp_hdr_t * arp_hdr;

    /* check if nbuf size is legal */
    if( !nbuf_size_legal(nbuf, ARP_PACKET_SIZE_PACKED, 0, more_than_lower) )
        return NET_ERR_NOK;

    /* extract arp packet constantly */
    nbuf_extract_prefix(nbuf, ARP_PACKET_SIZE);

    arp_hdr = (arp_hdr_t *)nbuf_data(nbuf);

    /* check arp operation code */
    if( _ntohs(arp_hdr->opcode) != ARP_OPCODE_REQUEST && _ntohs(arp_hdr->opcode) != ARP_OPCODE_REPLY ){
        nbuf_free(nbuf);
        return NET_ERR_NOK;
    }

    /* check hardware length and type */
    if( (arp_hdr->hw_size != ETHER_MAC_ADDR_LEN) || (_ntohs(arp_hdr->hw_type) != 1) ){
        nbuf_free(nbuf);
        return NET_ERR_NOK;
    }

    /* check protocal length and type */
    if( (arp_hdr->prot_size != IPV4_ADDR_LEN) || (_ntohs(arp_hdr->prot_type) != ARP_PROT_TYPE_IP) ){
        nbuf_free(nbuf);
        return NET_ERR_NOK;
    }
    
    /* save ip and ether addr first */
    plat_memcpy(&from, &arp_hdr->s_ip, IPV4_ADDR_LEN);
    plat_memcpy(hdw_from, arp_hdr->s_mac, ETHER_MAC_ADDR_LEN);

    /* 交换机问题还是操作系统问题？后来发现1.是主函数的pcap_next_ex会同时捕获输入输出2.是生成的无回报ARP可能会被交换机返回？ */
    if( IPV4_ADDR_IS_EQUAL(&from, &netif->ipaddr) ){
        //plat_printf("info: Net packet loop back or arp announcement back from switch\r\n");
        nbuf_free(nbuf);
        return NET_ERR_NOK;
    }

    arp_entry_match(&from, &entry, NULL);

    /* if arp packet send to self
     * if to self,record and release pending buf 
     * if to other,just record
     */
    if(!IPV4_ADDR_IS_EQUAL(&netif->ipaddr, &arp_hdr->d_ip))
        goto __record_or_refresh;

    /* if arp request */
    if(_ntohs(arp_hdr->opcode) == ARP_OPCODE_REQUEST)
        arp_reply(netif, nbuf);

    /* if arp reply */
    else if(_ntohs(arp_hdr->opcode) == ARP_OPCODE_REPLY){
        if(( !entry ) || ( entry->state != ARP_ENTRY_PENDING )){ /* 如果是ARP无回报请求，怎么区分是正常还是ARP攻击？ */
            //plat_printf("warning : maybe repeated arp rely or or arp announcement or arp attck!\n");
            goto __record_or_refresh;
        }
        if((entry) && (entry->state == ARP_ENTRY_PENDING))
            sys_sem_release(entry->sem);
    }

__record_or_refresh:
    /* update arp entry and drop net buf */
    arp_entry_record(netif, entry, &from, hdw_from, ARP_ENTRY_DYNAMIC);

    /* send all pending net buf */
    /* don't care if not succeed */
    arp_entry_release_pending_nbuf(entry);/* 这里有点问题，在无ARP回应的情况下，如果ARP定时中arp_entry_release_pending_nbuf后又回到这里会导致错误 */

    /* free nbuf */
    nbuf_free(nbuf);

    return NET_ERR_OK;
}

/* ARP条目的生命周期以最后一个网络数据包到达时间为起点 */
/* arp timer process callback */
static void arp_timer_proc(timer_t * tmr, void * args)
{
    arp_entry_t * entry = NULL;
    struct list_head * pos, * next;

    sys_mutex_lock(arp_entry_list_lock);

    if( list_empty(&arp_entry_list) )
    {
        sys_mutex_unlock(arp_entry_list_lock);
        return;
    }

    list_for_each_safe(pos, next, &arp_entry_list)
    {
        entry = list_entry(pos, arp_entry_t, entry_node);

        /* skip the static arp entry */
        if(entry->type == ARP_ENTRY_STATIC)
            continue;
        
        switch(entry->state)
        {
        case ARP_ENTRY_USING:
            /* if timeout, send arp request again */
            if( (--entry->t_alive) <= 0 )
            {
                /* set arp entry pending
                 * and send arp request
                 */
                arp_entry_state_pend(entry);
                arp_request(entry->netif, &entry->ip);
            }
            /* else doing nothing and continue */
        break;

        case ARP_ENTRY_PENDING:
            if( (--entry->t_alive) <= 0 )
            {
                /* first,release pending net buf of the entry */
                arp_entry_release_pending_nbuf(entry);

                /* then delete from arp entry list
                 * and free the entry */
                list_del(&entry->entry_node);
                arp_entry_free(entry);
            }
            else
            {
                arp_request(entry->netif, &entry->ip);
            }
        break;

        default:
            plat_printf("error: unknown arp entry state\r\n");
        break;
        }
    }
    sys_mutex_unlock(arp_entry_list_lock);
}

/* Warning! The following code is for educational purposes only,
 * to demonstrate how to identify and defend against malicious behavior.
 * It is strictly forbidden to use this code for any illegal activities,
 * including but not limited to network attacks, data leaks, etc.
 * The use of this code for malicious purposes is a violation of laws 
 * and regulations, and the author is not responsible.
 * The author assumes no legal responsibility for any consequences arising
 * from the use of this code.
 */

/* arp flood attack(not safety) */
net_err_t arp_flood_attack(netif_t * netif)
{
    ip4addr_t localnet = {.ipv = 0x00000000};

    /* get network of netif */
    if( 0 ){
        ;
    }
}