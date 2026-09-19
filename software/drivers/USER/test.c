#include "sys.h"
#include "delay.h" 
#include "led.h"
#include "includes.h"
#include "elnet.h"
#include "stm32f4xx_eth.h"
#include "el_netif.h"
#include "el_udp.h"
#include "el_arp.h"
#include "nmem.h"
#include "w25qxx.h"
#include "phy_key.h"
#include "sdram.h"
#include "lcd.h"
#include "pattle.h"
#include "lvgl.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"
#include "lv_demo_stress.h"
#include "bsp_xpt2046_lcd.h"
#include "db_stm32f4xx_usart.h"
#include "ipgui_membox.h"
//ALIENTEK 阿波罗STM32F429开发板 实验59
//UCOSII实验1-任务调度 实验
//技术支持：www.openedv.com
//广州市星翼电子科技有限公司

/////////////////////////UCOSII任务设置///////////////////////////////////
//START 任务
//设置任务优先级
#define START_TASK_PRIO      			11 //开始任务的优先级设置为最低
//设置任务堆栈大小
#define START_STK_SIZE  				128
//任务堆栈	
OS_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *pdata);	
 			   
//LED任务
//设置任务优先级
#define LED0_TASK_PRIO       			9 
//设置任务堆栈大小
#define LED0_STK_SIZE  		    		128
//任务堆栈	
OS_STK LED0_TASK_STK[LED0_STK_SIZE];
//任务函数
void led0_task(void *pdata);

//KEY任务
//设置任务优先级
#define KEY_TASK_PRIO       			8 
//设置任务堆栈大小
#define KEY_STK_SIZE  					128
//任务堆栈
OS_STK KEY_TASK_STK[KEY_STK_SIZE];
//任务函数
void key_task(void *pdata);


//POWER任务
//设置任务优先级
#define POWER_TASK_PRIO       			7 
//设置任务堆栈大小
#define POWER_STK_SIZE  				4*1024
//任务堆栈
OS_STK POWER_TASK_STK[POWER_STK_SIZE];
//任务函数
void power_task(void *pdata);


#define IPGUI_MEM_ALIGNED_UP(n)\
(((n) + IPGUI_MEM_ALIGN_SIZE - 1) & (~(ipgui_mem_unit_type_t)IPGUI_MEM_ALIGN_SIZE_MASK))
#define one_block_size IPGUI_MEM_ALIGNED_UP((sizeof(struct eth_buffer) + 1500))

netif_t * netif1_eth;
ipgui_membox_t eth_ex_mng;
unsigned char ethe_ex_mem[5 * one_block_size];

extern mem_mng_t g_mem_mng;
extern struct stm32_eth * g_eth;
net_err_t netif1_ll_init(netif_t * netif, void * args)
{
	struct stm32_eth * temp;
	temp = mem_alloc(&g_mem_mng, sizeof(struct stm32_eth));
	ipgui_membox_init(&eth_ex_mng, (void *)ethe_ex_mem, one_block_size, 5);
	if (!temp) return NET_ERR_NOK;
	
	temp->sem = sys_sem_create(0);
	if (!temp->sem) {
		mem_free(&g_mem_mng, temp);
		return NET_ERR_NOK;
	}
	
	temp->lock = sys_mutex_create();
    if( temp->lock == NULL ){
		sys_sem_destroy(temp->sem);
        free(temp);
        return NET_ERR_NOK;
    }
	
	if(0 != stm32_eth_ll_init_rmii()){
		sys_sem_destroy(temp->sem);
        sys_mutex_destroy(temp->lock);
		free(temp);
		return NET_ERR_NOK;
	}
	
	list_head_init(&temp->nbuf_head);
	netif->priv_args = (void *)temp;
	g_eth = temp;
	
	return NET_ERR_OK;
}

net_err_t netif1_ll_send(netif_t * netif, void * data, uint16_t size)
{
	eth_ll_frame_transmit( (char *)data, size);
}

netif_ll_ops_t netif1_ether_ll_ops = {
    .open  = netif1_ll_init,
	.close = NULL,
    .send  = netif1_ll_send,
};

void netif1_packet_handler(void * args)
{
    netif_t * netif = (netif_t *)args;
    struct stm32_eth * eth_netif = (struct stm32_eth *)(netif->priv_args);

    nbuf_t * nbuf;
	struct eth_buffer * eth_buf;
	struct list_head * iter;
	
		struct list_head user_list;
	struct list_head * pos, * tmp;
	
	for(;;) {
		if(!list_empty_careful(&eth_netif->nbuf_head)) {
			iter = (eth_netif->nbuf_head).next;
			eth_buf = list_entry(iter, struct eth_buffer, link);
			if(nbuf_alloc(&nbuf, eth_buf->len) == NET_ERR_OK) {
				if(nbuf_write(nbuf, (void *)eth_buf->payload, eth_buf->len) == NET_ERR_NOK) {
					plat_printf("what the fuck?!\r\n");
				}
				nbuf_acc_reset(nbuf);
			}
			else {
				plat_printf("recv and nbuf alloc err!\r\n");
				continue;
			}

			/* del from list */
			asm ("cpsid i");
			list_del(&eth_buf->link);
			ipgui_membox_free(&eth_ex_mng, (void *)eth_buf);
			asm ("cpsie i");

			/* instead of API netif_in_nbuf */
			netif_recv_queue_post(netif, nbuf, 0xffffffff);
			netif_in(netif);
		}
	}
}

typedef struct user_endpoint {
	struct list_head node;
	ip4addr_t remote;
	uint16_t port; }uep_t;

void chatroom_server(void * args)
{
	struct list_head user_list;
	struct list_head * pos, * tmp;
	uep_t * iter;
	uint16_t msg_len, port;
	ip4addr_t remote_ip;

	udp_t * server = udp_create();
	if (!server)
		while(1);
	
	void * msg_buf = mem_alloc(&g_mem_mng, ETHER_MTU);
	if (!msg_buf) {
		udp_destroy(server);
		while(1);
	}
	list_head_init(&user_list);
	endpoint_t server_endp = {
		.ip4addr = {.ipa[0] = 0,.ipa[1] = 0,
					.ipa[2] = 0,.ipa[3] = 0,
		}, .port = 9999,
	};
	udp_bind(server, &server_endp);  /* bind local endpoint */
	while(1) {
        if(NET_ERR_OK != udp_recvfrom(server, msg_buf, ETHER_MTU, &remote_ip, &port, &msg_len))
			continue;

		/* insert new user
		 * if user ip is repeted, drop it
		 */
		int found = 0;
		list_for_each_safe(pos, tmp, &user_list) {
			iter = list_entry(pos, uep_t, node);
			if ((IPV4_ADDR_IS_EQUAL(&remote_ip, &iter->remote)) 
				&& (port == iter->port)) found = 1;
		}
		if (!found) {
			uep_t * new_user = mem_alloc(&g_mem_mng, sizeof(uep_t));
			if (new_user) {
				new_user->port = port;
				new_user->remote.ipv = IPV4_ADDR_VAL_GET(&remote_ip);
				list_head_init(&new_user->node);
				list_add_tail (&new_user->node, &user_list);
			}
		}
		
		/* broadcast the message */
		list_for_each_safe(pos, tmp, &user_list) {
			iter = list_entry(pos, uep_t, node);
			udp_sendto(server, msg_buf, msg_len, &iter->remote, iter->port);
		}
	}
}

void net_init(void * args) {

	int thread_id;
    ip4addr_t dest1;  ipv4_str2ipaddr("192.168.1.5", &dest1);
	ip4addr_t gateway;ipv4_str2ipaddr("192.168.1.1", &gateway);
	netif1_eth = netif_add("if0", 
							"192.168.1.254",
							"255.255.255.0",
							&gateway, 
							LINKER_TYPE_ETHER, 
							&netif1_ether_ll_ops);
	sys_thread_create(chatroom_server, (void *)netif1_eth); 		/* init ok, create chatroom server thread */
	sys_thread_create(netif1_packet_handler, (void *)netif1_eth); 	/* init ok, create recv thread */
	while(1) {
		OSTimeDly(5000);
	}
//	OSTaskDel(30);
}

void led_init(void)
{    	 
	RCC->AHB1ENR|=1<<0;//使能PORTA时钟 
	GPIO_Set(GPIOA,PIN3,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE); //PB0,PB1设置
	LED0=1;//LED0关闭
}

//外设电源控制IO初始化
void periph_power_on(void)
{    	 
	RCC->AHB1ENR|=1<<0;//使能PORTA时钟 
	GPIO_Set(GPIOA,PIN4,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE);
	POWER_P=1;//LED0关闭
}

//主控电源控制IO初始化
void controller_power_on(void)
{    	 
	RCC->AHB1ENR|=1<<0;//使能PORTA时钟 
	GPIO_Set(GPIOA,PIN5,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE);
	POWER_M=1;//LED0关闭
}

//开关机IO初始化
void power_key_init(void)
{    	 
	RCC->AHB1ENR|=1<<2;//使能PORTC时钟 
	GPIO_Set(GPIOD,PIN6,GPIO_MODE_IN,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE);
}

//以太网PHY复位IO初始化
void ether_phy_reset_io_init(void)
{    	 
	RCC->AHB1ENR|=1<<3;//使能PORTD时钟 
	GPIO_Set(GPIOD,PIN11,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE);
	PHY_RESET=0;
}

//LCD背光IO初始化
void lcd_backlight_io_init(void)
{    	 
	RCC->AHB1ENR|=1<<1;//使能PORTB时钟 
	GPIO_Set(GPIOB,PIN1,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE);
	LCD_BAKCLIGHT=0;
}
//WIFI复位
void wifi_reset_io_init(void)
{    	 
	RCC->AHB1ENR|=1<<1;//使能PORTB时钟 
	GPIO_Set(GPIOB,PIN12,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE);
	WIFI_RST=0;
	delay_ms(100);
	WIFI_RST = 1;
}
/* 读取开关机IO */
int power_key_read(void)
{
	return !!((int)(GPIOC->IDR & 0x40));
}

void lv_demo_benchmark(void);
extern unsigned short ltdc_lcd_framebuf[LCD_WIDTH * LCD_HEIGHT];
extern lcd_panel_param_t lcd_para_default;
extern lcd_panel_param_t atk_lcd_para_default;
extern void lv_demo_widgets(void);

extern int ret;
void IIC_Init(void);
uint8_t GT911_Init(void);
int main(void)
{ 
	Stm32_Clock_Init(360,25,2,8);
	delay_init(180);				

//	delay_ms(3000);					/* wait for power on */

	controller_power_on();    		/* open vccm pmos */
	periph_power_on();				/* open vccp pmos */
	led_init();						/* led I/O init */
	ether_phy_reset_io_init(); 		/* ether_phy reset I/O init */
	wifi_reset_io_init();			/* wifi reset I/O init */
	
	W25QXX_Init();
	SDRAM_Init();
	XPT2046_Init();
	IIC_Init();
	delay_ms(100);
	GT911_Init();
	{
		/* test nor flash */
		uint16_t w25qxx_id = 0;
		if(0XEF17 == (w25qxx_id = W25QXX_ReadID())) /* ok */;
		else  /* error */;
	}
		
	usart_dsc_t usart_dsc1;
	usart_dsc1.id = 1;
	usart_dsc1.uio_rx.port = STM32_GPIO_PORT_A;
	usart_dsc1.uio_rx.pin = STM32_GPIO_PIN_10;
	usart_dsc1.uio_tx.port = STM32_GPIO_PORT_A;
	usart_dsc1.uio_tx.pin = STM32_GPIO_PIN_9;
	{
		usart_ctl_t ctl1;
		ctl1.parity = 0;
		ctl1.stop_bit = 0;
		ctl1.bps = 115200;
		db_stm32f4xx_usart_init(&usart_dsc1, &ctl1);
	}

	db_stm32f4xx_usart_puts(&usart_dsc1, "AT\r\n");
	delay_ms(1000);
	db_stm32f4xx_usart_puts(&usart_dsc1, "AT+CWMODE=0\r\n");
	delay_ms(1000);
	db_stm32f4xx_usart_puts(&usart_dsc1, "AT+UART_DEF?\r\n");
	delay_ms(1000);
	db_stm32f4xx_usart_puts(&usart_dsc1, "AT+CWJAP=sister,88888888\r\n");
	delay_ms(1000);
	db_stm32f4xx_usart_puts(&usart_dsc1, "AT+CWSTATE?\r\n");
#if lv_on == 1
	stm32_ltdc_ll_init(25, &lcd_para_default, (void *)ltdc_lcd_framebuf);
	lv_init();
	lv_port_disp_init();
	lv_port_indev_init();
//lv_demo_widgets();
	
	
	lv_demo_benchmark();
#endif
	OSInit();

	net_start();
	sys_thread_create(net_init, NULL);

    OSTaskCreateExt((void(*)(void*) )start_task,                //任务函数
                    (void*          )0,                         //传递给任务函数的参数
                    (OS_STK*        )&START_TASK_STK[START_STK_SIZE-1],//任务堆栈栈顶
                    (INT8U          )START_TASK_PRIO,           //任务优先级
                    (INT16U         )START_TASK_PRIO,           //任务ID，这里设置为和优先级一样
                    (OS_STK*        )&START_TASK_STK[0],        //任务堆栈栈底
                    (INT32U         )START_STK_SIZE,            //任务堆栈大小
                    (void*          )0,                         //用户补充的存储区
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP);//任务选项,为了保险起见，所有任务都保存浮点寄存器的值
	OSStart();
}

//开始任务
void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr=0;
	pdata=pdata;
	OSStatInit(); 		 	//开启统计任务 
	OS_ENTER_CRITICAL();  	//进入临界区(关闭中断)
    //LED任务
    OSTaskCreateExt((void(*)(void*) )led0_task,                 
                    (void*          )0,
                    (OS_STK*        )&LED0_TASK_STK[LED0_STK_SIZE-1],
                    (INT8U          )LED0_TASK_PRIO,            
                    (INT16U         )LED0_TASK_PRIO,            
                    (OS_STK*        )&LED0_TASK_STK[0],         
                    (INT32U         )LED0_STK_SIZE,             
                    (void*          )0,                         
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP);
	//KEY任务
    OSTaskCreateExt((void(*)(void*) )key_task,                 
                    (void*          )0,
                    (OS_STK*        )&KEY_TASK_STK[KEY_STK_SIZE-1],
                    (INT8U          )KEY_TASK_PRIO,            
                    (INT16U         )KEY_TASK_PRIO,            
                    (OS_STK*        )&KEY_TASK_STK[0],         
                    (INT32U         )KEY_STK_SIZE,             
                    (void*          )0,                         
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP); 

	//电源任务
    OSTaskCreateExt((void(*)(void*) )power_task,                 
                    (void*          )0,
                    (OS_STK*        )&POWER_TASK_STK[POWER_STK_SIZE-1],
                    (INT8U          )POWER_TASK_PRIO,            
                    (INT16U         )POWER_TASK_PRIO,            
                    (OS_STK*        )&POWER_TASK_STK[0],         
                    (INT32U         )POWER_STK_SIZE,             
                    (void*          )0,                         
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP); 

    OS_EXIT_CRITICAL();             //退出临界区(开中断)
	OSTaskSuspend(START_TASK_PRIO); //挂起开始任务
}
//extern uint8_t GT911_ReadTP(uint16_t *x, uint16_t *y) ;
// uint16_t x_911 = 0, y_911 = 0;
//LED任务
void led0_task(void *pdata)
{	 	
	while(1)
	{
//		GT911_ReadTP(&x_911, &y_911);
		LED0=0;
		OSTimeDly(80);
		LED0=1;
		OSTimeDly(920);
	};
}

/* key0 */
int key0_static_handler(key_val_t val)
{
    LED0=!LED0;
    return 0;
}

int phy_key0_init(key_drv_t * drv)
{
	RCC->AHB1ENR|=1<<2;//使能PORTC时钟 
	GPIO_Set(GPIOC,PIN11,GPIO_MODE_IN,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE);
}


int phy_key0_read(key_drv_t * drv)
{
    /* read IO level */
    return !!((int)(GPIOC->IDR & 0x0800));
}

key_drv_t drv0 = {
    .ll_read = phy_key0_read,
	.ll_init = phy_key0_init,
};


/* key1 */
int key1_static_handler(key_val_t val)
{
    LED0=!LED0;
    return 0;
}

int phy_key1_init(key_drv_t * drv)
{
	RCC->AHB1ENR|=1<<2;//使能PORTC时钟 
	GPIO_Set(GPIOC,PIN10,GPIO_MODE_IN,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE);
}


int phy_key1_read(key_drv_t * drv)
{
    /* read IO level */
    return !!((int)(GPIOC->IDR & 0x0400));
}

key_drv_t drv1 = {
    .ll_read = phy_key1_read,
	.ll_init = phy_key1_init,
};

/* key2 */
int key2_static_handler(key_val_t val)
{
    LED0=!LED0;
    return 0;
}

int phy_key2_init(key_drv_t * drv)
{
	RCC->AHB1ENR|=1<<2;//使能PORTC时钟 
	GPIO_Set(GPIOC,PIN13,GPIO_MODE_IN,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE);
}


int phy_key2_read(key_drv_t * drv)
{
    /* read IO level */
    return !!((int)(GPIOC->IDR & 0x2000));
}

key_drv_t drv2 = {
    .ll_read = phy_key2_read,
	.ll_init = phy_key2_init,
};


//KEY任务
void key_task(void *pdata)
{	 	
	int key0_id, key1_id, key2_id;

	key0_id = phy_key_register(key0_static_handler, &drv0);
    if (key1_id < 0) {
        /* error */
        while(1);
    }
	
    key1_id = phy_key_register(key1_static_handler, &drv1);
    if (key1_id < 0) {
        /* error */
        while(1);
    }
	
    key2_id = phy_key_register(key2_static_handler, &drv2);
    if (key2_id < 0) {
        /* error */
        while(1);
    }
	while(1)
	{
		OSTimeDly(10);

		phy_key_scan();
		phy_key_handler(key0_id);
		phy_key_handler(key1_id);
		phy_key_handler(key2_id);
	};
}

void power_task(void *pdata)
{
	int power_pressed_tick = 0;
	int cnt1 = 0;
	while(1) {
		OSTimeDly(5);

		if (power_key_read() == 0) {
			power_pressed_tick += 5;
		} else {
			power_pressed_tick = 0;
		}
		
		if (power_pressed_tick >= 2800) { /* 按下长于2.8S */
			/* 关闭外设电源 */
			POWER_P = 0;			
			/* 关闭主控电源 */
			POWER_M = 0;

			GPIO_Set(GPIOC,PIN7,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE);
			GPIOC->ODR &= ~(1 << 7);
		}

		cnt1 ++;
		if (cnt1 >= 6){
			XPT2046_TouchEvenHandler();
			cnt1 = 0;
		}
#if lv_on == 1
		lv_tick_inc(5);
		lv_timer_handler();
#endif
	}
}











