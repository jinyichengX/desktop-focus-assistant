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

#include "el_netif.h"
#include "el_mempool.h"
#include "global.h"
#include "el_ipv4.h"
#include "el_ether.h"

extern net_err_t route_add(ip4addr_t * network, ip4addr_t * mask, ip4addr_t * nexthop, netif_t * netif);
extern net_err_t route_delete_netif(netif_t * netif);
extern net_err_t ipv4_in(netif_t * netif, nbuf_t * nbuf);
/* set netif ip addr */
static void * netif_ip_set(netif_t * netif, const char * ip)
{
	ipv4_str2ipaddr(ip, &netif->ipaddr);
}

/* set netif netmask */
static void * netif_netmask_set(netif_t * netif, const char * net_mask)
{
	ipv4_str2ipaddr(net_mask, &netif->mask);
}

/* set netif gateway */
static void * netif_gw_set(netif_t * netif, ip4addr_t * gw)
{
	IPV4_ADDR_VAL_SET(&netif->gw_ip, IPV4_ADDR_VAL_GET(gw));
}

#if USE_NETIF_BANDWIDTH_STATISTIC
// void check_bandwidth(struct net_device *dev) {
//     static uint64_t last_check_time = 0;
//     static uint64_t last_sent = 0;
//     static uint64_t last_received = 0;

//     uint64_t current_time = get_current_time_ms(); // 获取当前时间（毫秒）
//     if (last_check_time == 0) {
//         last_check_time = current_time;
//         last_sent = dev->bytes_sent;
//         last_received = dev->bytes_received;
//         return;
//     }

//     uint64_t time_diff = current_time - last_check_time;
//     uint64_t sent_diff = dev->bytes_sent - last_sent;
//     uint64_t received_diff = dev->bytes_received - last_received;

//     if (time_diff > 0) {
//         double bandwidth_sent = (double)sent_diff / time_diff;
//         double bandwidth_received = (double)received_diff / time_diff;

//         // 打印或者记录带宽信息
//         printf("Device %s: Sent %f bytes/sec, Received %f bytes/sec\n",
//                dev->name, bandwidth_sent, bandwidth_received);

//         last_check_time = current_time;
//         last_sent = dev->bytes_sent;
//         last_received = dev->bytes_received;
//     }
// }
static void netif_bandwidth_statistic_proc(timer_t * tmr, void * args)
{
    netif_t * netif;
    uint32_t last_sec_recv = 0;
    struct list_head * pos, * next;

    sys_mutex_lock(netif_list_lock);
    list_for_each_safe(pos, next, &netif_list){
        netif = list_entry(pos, netif_t, node);
#if BANDWIDTH_STATISTIC_SMOOTH == 0
        plat_printf("netif %s received %f Mbits/sec, send %f Mbits/sec\r\n", netif->name, (float)netif->recv_bps/(float)1000000, (float)netif->send_bps/(float)1000000);
		if(netif->recv_bps_max < netif->recv_bps) netif->recv_bps_max = netif->recv_bps;
		netif->recv_bps_last = netif->recv_bps;
        netif->recv_bps &= 0;
        netif->send_bps &= 0;
#else
        last_sec_recv = netif->recv_bps;
        netif->recv_bps = (last_sec_recv + netif->recv_bps) >> 1;
#endif
    }
    sys_mutex_unlock(netif_list_lock);
}
#endif

net_err_t netif_init(void)
{
	net_err_t err;
	/* init netif pool */
	err = mempool_create(&netif_pool, (void *)netif_class,\
						sizeof(netif_t), NETIF_MAXIUM_NUM);
	/* init netif list */
	INIT_LIST_HEAD(&netif_list);
	netif_list_lock = sys_mutex_create();
    if( netif_list_lock == NULL )
    {
        if( err == NET_ERR_OK )
            mempool_destroy(&netif_pool);
    }
#if USE_NETIF_BANDWIDTH_STATISTIC
    net_timer_add(&netif_bandwidth_statistic_timer, netif_bandwidth_statistic_proc, \
    RELOAD_FOREVER, NETIF_RECV_BANDWIDTH_STATISTIC_TIMER_CALLBACK_TIME, NULL);
#endif
	return err;
}

/* init netif send queue */
static inline 
net_err_t netif_squeue_init(netif_t * netif)
{
  return queue_init(&netif->send_queue, (void *)netif->sqb, NETIF_SQUEUE_SIZE, sizeof(void *));
}

/* init netif recv queue */
static inline 
net_err_t netif_rqueue_init(netif_t * netif)
{
  return queue_init(&netif->recv_queue, (void *)netif->rqb, NETIF_RQUEUE_SIZE, sizeof(void *));
}

/* add netif */
netif_t * netif_add(const char * name,\
                    const char * ip,\
                    const char * mask,\
                    ip4addr_t * gw_ip,\
                    linker_type_t link_type,\
                    netif_ll_ops_t * netif_ops)
{
	netif_t * netif = NULL;
    sys_thread_t netif_thread;
    ip4addr_t ip_temp, mask_temp;

	if( ( !ip ) || ( !mask ) || ( !netif_ops ))
		goto __return;

    /* if gateway is legal */
    if( gw_ip ){
        ipv4_str2ipaddr(ip, &ip_temp);
        ipv4_str2ipaddr(mask, &mask_temp);
        if(IPV4_ADDR_NETWORK(&ip_temp, &mask_temp) != IPV4_ADDR_NETWORK(gw_ip, &mask_temp))
            goto __return;
    }
  
	/* alloc one net interface manager */
    if(NULL == (netif = (netif_t *)mempool_alloc(&netif_pool, 0))){
        plat_printf("warning: supported network cards number has reached maximum!\r\n");
        goto __return;
    }
    plat_memset((void *)netif, 0, sizeof(netif_t));
	
    netif->ops = netif_ops;
  
    /* init netif or network card */
    if( (netif->ops->open) && (netif->ops->open(netif, NULL) == NET_ERR_NOK) ){
        goto __return;
    }

	/* set linker max transform unit */
	if(link_type == LINKER_TYPE_ETHER){
		/* check hardware addr */
		//if( !netif->mac[0] ) goto __return;
#if 1
		netif->hwaddr[0] = 0x1E;
		netif->hwaddr[1] = 0x30;
		netif->hwaddr[2] = 0x6C;
		netif->hwaddr[3] = 0xA2;
		netif->hwaddr[4] = 0x45;
		netif->hwaddr[5] = 0x5E;
#endif
    /* if user not init hardware addr,return not ok */
        if(!plat_memcmp((uint8_t *)ether_addr_any, netif->hwaddr, ETHER_MAC_ADDR_LEN))
            goto __return;

        netif->mtu = ETHER_MTU;
        netif->linkout = ether_out_arp;
        netif->init = ether_init;
        netif->linkin = ether_in;
	}
	else if(link_type == LINKER_TYPE_LOOP){
		netif->mtu = -0xffff;
	}
	else{
		/* not support */
		plat_printf("error: unkown netif linker type\r\n");
        goto __return;
	}
	netif->recv_bps_max = 0;
	netif->name = name;
	/* set netif ip, mask, gateway */
	netif_netmask_set(netif, mask);
    if(1 == ipv4_netmask_legal(&netif->mask))
        goto __return;
    netif_ip_set(netif, ip);
	netif_gw_set(netif, gw_ip);

    /* create send thread */
    netif_thread = sys_thread_create(netif_package_send, netif);
    if(netif_thread != SYS_THREAD_VALID){
        plat_printf("error: create netif send thread fail\r\n");
        goto __return;
    }

//    netif_thread = sys_thread_create(netif_package_recv, netif);
//    if(netif_thread != SYS_THREAD_VALID){
//        plat_printf("error: create netif recv thread fail\r\n");
//        goto __return;
//    }

    if((NET_ERR_NOK == netif_squeue_init(netif))\
        ||(NET_ERR_NOK == netif_rqueue_init(netif))){
        plat_printf("error: create netif send or receive queue fail\r\n");
        goto __return;
    }
#if USE_NETIF_BANDWIDTH_STATISTIC == 1
    netif->recv_bandwidth_mutex = sys_mutex_create();
    netif->send_bandwidth_mutex = sys_mutex_create();
#endif
    /* add local network route(局域网其他主机) */
    IPV4_ADDR_VAL_SET(&ip_temp, IPV4_ADDR_NETWORK(&netif->ipaddr, &netif->mask));
    route_add(&ip_temp, &netif->mask, NULL, netif);
    
    /* add loopback route(本机ip) */
    IPV4_ADDR_VAL_SET(&ip_temp, MASK_ADDR_BROADCAST);
    route_add(&netif->ipaddr, &ip_temp, NULL, netif);

    /* add loopback route(本机环回地址) */
    IPV4_ADDR_VAL_SET(&ip_temp, _htonl( NET_LOOPBACK ));
    IPV4_ADDR_VAL_SET(&mask_temp, _htonl( MASK_LOOPBACK ));
    route_add(&ip_temp, &mask_temp, NULL, netif);

    /* add wide network route(网关(外网)) */
    if( gw_ip ){
        IPV4_ADDR_VAL_SET(&ip_temp, IPV4_ADDR_ANY);
        IPV4_ADDR_VAL_SET(&mask_temp, MASK_ADDR_ANY);
        route_add(&ip_temp, &mask_temp, &netif->gw_ip, netif);
    }

    /* add broadcast route(广播) */
    IPV4_ADDR_VAL_SET(&ip_temp, IPV4_RESTRICTED_BROADCAST);
    IPV4_ADDR_VAL_SET(&mask_temp, IPV4_RESTRICTED_BROADCAST);
    route_add(&ip_temp, &mask_temp, NULL, netif);

    /* add to netif list */
    INIT_LIST_HEAD(&netif->node);
	plat_printf("info: no mutex netif list operating...\r\n");
	list_add(&netif_list, &netif->node);
	plat_printf("info: no mutex netif list operated\r\n");

	return netif;
__return:
    /* destroy thread and queue if have */
    (void)0;

	mempool_free(&netif_pool, (void *)netif);
	return netif;
}

/* delete netif */
net_err_t netif_delete(netif_t ** netif)
{
    struct list_head * pos, * tmp;
    netif_t * netif_tmp = NULL;
    route_entry_t * route;

	if( !netif ){
		plat_printf("warning: unkown netif!\r\n");
		return NET_ERR_NOK;
	}

    /* if could matched netif list */
    list_for_each_safe(pos, tmp, &netif_list){
        if( (netif_tmp = list_entry(pos, netif_t, node)) == * netif ){
            /* delete from netif list if match */
            break;
        }
    }

    /* if not found netif card */
    if(netif_tmp != (*netif))
        return NET_ERR_NOK;

    /* if have pending buf 
    * return not ok
    */
    //if(  )

    /* firstly, do netif close operation */
    if( netif_tmp->ops->close ){
        if(NET_ERR_NOK == netif_tmp->ops->close(netif_tmp, NULL))
            return NET_ERR_NOK;
    }

    /* delete route entry*/
    list_for_each_safe(pos, tmp, &route_entry_list){
        route = list_entry(pos, route_entry_t, node);
        if( (netif_t *)route->if_next == netif_tmp ){
            //route_delete(route);
        }
    }

    /* delete specific list node of netif */
    list_del(&netif_tmp->node);

	/* free netif to object pool */
	mempool_free(&netif_pool, (void *)netif_tmp);

    return NET_ERR_OK;
}

/* 查询ip地址是否有匹配的网卡 */
int netif_ip_match(ip4addr_t * ip)
{
    int ret = 0;
    struct list_head * pos, * tmp;
    netif_t * iter = NULL;
    sys_mutex_lock(netif_list_lock);
    list_for_each_safe(pos, tmp, &netif_list){
        iter = list_entry(pos, netif_t, node);
        if(IPV4_ADDR_IS_EQUAL(ip, &iter->ipaddr)){
            ret = 1;
            break;
        }
    }
    sys_mutex_unlock(netif_list_lock);
    return ret;
}

/* active netif operation */
net_err_t netif_active(netif_t * netif)
{
    /* 将netif_add的部分代码移植到这 */
}

/* deactive netif operation */
void netif_deactive(netif_t * netif)
{
    /* netif_active的逆过程 */
    /* delete route entry related to the netif */
    route_delete_netif(netif);
}

/* push package to netif send queue */
net_err_t netif_send_queue_post(netif_t * netif, nbuf_t * nbuf, uint32_t tmo)
{
    if( NULL == netif ) return NET_ERR_NOK;
    if( NULL == nbuf ) return NET_ERR_NOK;
    
    return queue_post_ptr(&netif->send_queue, (void *)nbuf, tmo);
}

/* take package from netif send queue */
net_err_t netif_send_queue_pend(netif_t * netif, void ** item, uint32_t tmo)
{
    if( NULL == netif ) return NET_ERR_NOK;
    if( NULL == item ) return NET_ERR_NOK; 

    return queue_fetch_ptr(&netif->send_queue, item, tmo);
}

/* push package to netif recv queue */
net_err_t netif_recv_queue_post(netif_t * netif, nbuf_t * nbuf, uint32_t tmo)
{
    if( NULL == netif ) return NET_ERR_NOK;
    if( NULL == nbuf ) return NET_ERR_NOK;

    return queue_post_ptr(&netif->recv_queue, (void *)nbuf, tmo);
}

/* take package from netif recv queue */
net_err_t netif_recv_queue_pend(netif_t * netif, void ** item, uint32_t tmo)
{
    if( NULL == netif ) return NET_ERR_NOK;
    if( NULL == item ) return NET_ERR_NOK; 

    return queue_fetch_ptr(&netif->recv_queue, item, tmo);
}

/* call netif link out operation */
net_err_t netif_out(netif_t * netif, ip4addr_t * ip_toMatch_mac, nbuf_t * nbuf)
{
    if( NULL == netif ) return NET_ERR_NOK;
    
    /* if netif support ethernet or other link layer */
    if( netif->linkout ){
        if(netif->linkout(netif,ip_toMatch_mac,nbuf)){
            plat_printf("warning: netif link out error!\r\n");
            return NET_ERR_NOK;
        }
    }
    else{
        /* call netif device out operation */
        //netif->ops->send(netif,NULL,0);
        netif_send_queue_post(netif, nbuf, 0xffffffff);
    }
#if USE_NETIF_BANDWIDTH_STATISTIC == 1
    if( netif->send_bandwidth_mutex ){
        sys_mutex_lock(netif->send_bandwidth_mutex);
        netif->send_bps += (nbuf->total_size << 3);
        sys_mutex_unlock(netif->send_bandwidth_mutex);
    }
#endif
    return NET_ERR_OK;
}

/* call netif link in operation */
net_err_t netif_in(netif_t * netif)
{
    net_err_t ret;
    void * nbuf_to_handle = NULL;

    /* fetch network packet from receive queue */
    netif_recv_queue_pend(netif, &nbuf_to_handle, 0xffffffff);
    if( NULL == nbuf_to_handle ){
        return NET_ERR_NOK;
    }

    if(netif->linkin){
        ret = netif->linkin(netif, (nbuf_t *)nbuf_to_handle);  
    }
    else{
        /* not support now, 2025/01/11 */
        ret = ipv4_in(netif, (nbuf_t *)nbuf_to_handle);
    }

    return ret;
}

net_err_t netif_in_nbuf(netif_t * netif, nbuf_t * nbuf)
{
    net_err_t ret;

    if( NULL == nbuf ){
        return NET_ERR_NOK;
    }

    if(netif->linkin){
        ret = netif->linkin(netif, nbuf);  
    }
    else{
        /* not support now, 2025/01/11 */
        ret = ipv4_in(netif, nbuf);
    }

    return ret;
}