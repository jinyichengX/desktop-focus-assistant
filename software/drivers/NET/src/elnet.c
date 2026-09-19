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

 #include "elnet.h"
 #include "el_mempool.h"
 #include "global.h"
 #include "el_nbuf.h"
 #include "plat.h"
 #include "el_netif.h"
 #include "el_udp.h"
 #include "el_ether.h"
 #include "async.h"
 #include "nmem.h"

mem_mng_t g_mem_mng;
 /* endian checker */
net_err_t net_plat_endian_checker(void)
{
    union e_cont{
        unsigned short val;
        unsigned char ch[2];
    };

    union e_cont emp = {.val = 0x90eb};
    if(emp.ch[0] == 0xeb)
    {
        net_endian_large = 0;
        if( ENDIANNESS_LITTLE == 0 )
            return NET_ERR_NOK;
    }
    else
    {
        net_endian_large = 1;
        if( ENDIANNESS_LITTLE == 1 )
            return NET_ERR_NOK;
    }
    
    return NET_ERR_OK;
}
 extern void statistic_init(void);
 /* standard checksum16 */
 uint16_t net_checksum16(uint16_t pre_chksum, void * buf, int nbytes, bool neg)
 {
     unsigned long sum;
     uint16_t * p = (uint16_t *)buf;
     uint16_t high;
 
     sum = pre_chksum;
 
     while(nbytes > 1){
         sum += *((uint16_t *)(p++));
         nbytes -= 2;
     }
 
     if(nbytes){
        //  sum += (*(uint8_t *)(p)) << 8; /* ？ */
         sum += *(uint8_t *)(p);
     }
 
     while ((high = sum >> 16) != 0){
         sum = high + (sum & 0xffff);
     }
   
     return neg ? (~sum) : (sum);
 }
 
 #if defined(_WIN32) || defined(_WIN64)
 DWORD netif_package_send(LPVOID args)
 #elif defined(__LINUX__)
 void netif_package_send(void * args)
 #elif defined(__ELOS__)
 void netif_package_send(void * args)
 #else
void netif_package_send(void * args)
 #endif
 {
     /* ethernet packet max length: 1514 */
     static uint8_t netif_packet[NET_PACKET_MAX_SIZE];
     netif_t * netif = (netif_t *)args;
     void * priv_arg = netif->priv_args;
     int size;
     nbuf_t * nbuf_send = NULL;
     void * msg = NULL;
     uint8_t u1,u2,u3,u4;
     /* send package loop */
     for( ; ; ){
         /* wait queue */
         if(NET_ERR_NOK == netif_send_queue_pend(netif, &msg, 0xffffffff)){
             plat_printf("error: wait send queue fail!\r\n");
             continue;
         }
 
         /* read data from nbuf_send to constant memory */
         nbuf_send = (nbuf_t *)msg;
         nbuf_acc_reset(nbuf_send);
         size = NMIN(NET_PACKET_MAX_SIZE, nbuf_send->total_size);
         nbuf_read(nbuf_send, netif_packet, size);
 
         /* free nbuf */
         nbuf_free(nbuf_send);
         /* call netif driver */
         if(netif->ops->send(netif, (void *)netif_packet, size)){
            //plat_printf("error: send fail!\r\n");
         }
     }
 }
 
 /* recv therad */
 #if defined(_WIN32) || defined(_WIN64)
 DWORD netif_package_recv(LPVOID args)
 #elif defined(__LINUX__)
 void netif_package_recv(void * args)
 #elif defined(__ELOS__)
 void netif_package_recv(void * args)
#else
void netif_package_recv(void * args)
#endif
 {
     netif_t * netif = (netif_t *)args;
     while(1){
         /* send to kernel msgqueue */
     }
 
 }
 
 /* handle timeout event and fetch message */
 net_err_t sys_timeout_kmsg_fetch(queue_t * q, void ** item)
 {
     /* look out timeout event */
     sys_tick_t tick_start;
     uint32_t next_tmo;
 
     next_tmo = net_timer_first_tmo();
 
     while(1)
     {
         tick_start = sys_tick_now();
         
         if( !next_tmo ){
             return queue_fetch_ptr(q, item, 0xffffffff);
         }
         if(queue_fetch_ptr(q, item, next_tmo) == NET_ERR_OK){
             net_timer_process(sys_tick_gone(tick_start));
             return NET_ERR_OK;
         }
         else{
             net_timer_process(next_tmo);
             next_tmo = net_timer_first_tmo();
             continue;
         }
     }
 
     return NET_ERR_OK;
 }

/* kernel network thread */
#if defined(_WIN32) || defined(_WIN64)
DWORD tcpip_thread(LPVOID args)
#elif defined(__LINUX__)
void tcpip_thread(void * args)
#elif defined(__ELOS__)
void tcpip_thread(void * args)
#else
void tcpip_thread(void * args)
#endif
{
	kmsg_t * msg_fetched = NULL;
	void * netbuffer = NULL;
	while(1){
		 /*  handle timeout event 
		 * or fetch message kernel message queue
		 */
		 sys_timeout_kmsg_fetch(&k_msg_queue, (void *)&msg_fetched);

		 if( msg_fetched == NULL ) continue;

		 switch( msg_fetched->msg_type ){
			case TCP_IP_MESSAGE_TYPE_RECV:{

			};
			default : break;
		 }
	}
}
void socket_init(void);
/* net init */
net_err_t el_net_init(void)
{
    net_err_t net_err = NET_ERR_OK;
    net_err |= net_plat_endian_checker();
    if(ENDIANNESS_LITTLE == net_endian_large){
        return NET_ERR_NOK;
    }
	net_err |= (net_err_t)mem_init(&g_mem_mng, (void *)g_heap, G_HEAP_SIZE);
    //    if( 0 != async_init())
    //        net_err |= NET_ERR_NOK;
    net_err |= net_timer_init();
    net_err |= nbuf_init();
    net_err |= netif_init();
    net_err |= ether_init(NULL, NULL);
    net_err |= arp_init();
    net_err |= ipv4_init();
    net_err |= udp_init();

    socket_init();
    statistic_init();
    while((int)net_err);

    return NET_ERR_OK;
}
 
 /* start network */
 net_err_t net_start(void)
 {
     net_err_t net_err = NET_ERR_OK;
 
     net_err |= el_net_init();
 
     /* create kernel message queue */
     queue_init(&k_msg_queue, (void *)k_msg_buf, KQ_PENDING_NUM, sizeof(void *));
 
     /* create kernel tcpip thread fetching message */
     net_thread = sys_thread_create(tcpip_thread, NULL);
     
     if( net_thread != 0 )
         net_err |= NET_ERR_NOK;
     
     return net_err;
 }