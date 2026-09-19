#include "stm32f4xx_gpio.h"
#include "stm32f4xx_eth.h"
#include "mii.h"
#include "stm32f429xx.h"
#include "nmem.h"
#include "led.h"
#include "ipgui_membox.h"
typedef unsigned short u16;
typedef unsigned char u8;
extern void delay_ms(u16 nms);

static stm32_gpio_ctl_t rmii_pin_ctl = {
    .mode  = GPIO_MODE_AF,
    .otype = GPIO_OTYPE_PP,
    .speed = GPIO_SPEED_VERY_HIGH,
    .pupd  = GPIO_PUPD_UP,
    .af    = 0,                 /* not set yet, set it when init */
};

/* macros of stm32 rcc */
#define RCC_BASE_ADDR           0x40023800                                  /* RCC memory map base */
#define RCC_AHB1RSTR_ADDR       (RCC_BASE_ADDR + 0x10)                      /* RCC AHB1 peripheral reset register */
#define RCC_AHB1ENR_ADDR        (RCC_BASE_ADDR + 0x30)                      /* RCC AHB1 peripheral clock enable register */
#define RCC_APB2ENR_ADDR        (RCC_BASE_ADDR + 0x44)                      /* RCC APB2ENR register address */

/* macros of stm32 syscfg */
#define SYSCFG_BASE_ADDR        0x40013800                                  /* SYSCFG memory map base */
#define SYSCFG_PMC_ADDR         (SYSCFG_BASE_ADDR + 0x04)                   /* SYSCFG PMC register address */

/* macros of stm32 eth */
#define ETH_BASE_ADDR           0x40028000                                  /* ETH memory map base */

#define ETH_MACCR_ADDR			(ETH_BASE_ADDR + 0)							/* ETH MAC configuration register */
#define ETH_MACFFR_ADDR         (ETH_BASE_ADDR + 0x04)                      /* ETH MAC frame filter register */

#define ETH_MACMIIAR_ADDR       (ETH_BASE_ADDR + 0x10)                      /* ETH MAC MII Address Register */
#define ETH_MACMIIDR_ADDR       (ETH_BASE_ADDR + 0x14)                      /* ETH MAC MII Register Data Register */
#define ETH_MACFCR_ADDR         (ETH_BASE_ADDR + 0x18)                      /* ETH MAC flow control register */

typedef struct {
    const char * name;
    stm32_gpio_dsc_t gpio_dsc;
    int af_num;                                                             /* 复用号 */
}pad_dsc_t;

static char tx_buf_index;
static char rx_buf_index;

volatile static stm32_rx_desc_t rx_desc[NUM_RX_BUF];  /* DMA接收描述符 */
volatile static stm32_tx_desc_t tx_desc[NUM_TX_BUF];  /* DMA发送描述符 */

static unsigned int rx_buf[NUM_RX_BUF][ETH_BUF_SIZE >> 2]; /* DMA接收描述符缓冲 */
static unsigned int tx_buf[NUM_TX_BUF][ETH_BUF_SIZE >> 2]; /* DMA发送描述符缓冲 */

static int read_phy(int paddr, int raddr)
{
    int timeout = 0;
    int reg;

    /* read */
    reg = readl(ETH_MACMIIAR_ADDR);
    reg &= 0x1c;
    reg |= (paddr << 11) | (raddr << 6) | (1 << 0);                         /* 给busy位置1就可触发读操作 */
	writel(ETH_MACMIIAR_ADDR, reg);
	
    timeout = 0x0004FFFF;

    /* wait MMAR_MB cleared */
    while (timeout --) {
        reg = readl(ETH_MACMIIAR_ADDR);
        if ((reg & 0x01U) == 0)
            break;
    }
    if (timeout <= 0) return -1;
    else return readl(ETH_MACMIIDR_ADDR);
}

static int write_phy(int paddr, int raddr, unsigned short val)
{
    int timeout = 0;
    int reg;

    /* write data to send */
    writel(ETH_MACMIIDR_ADDR, val);

    /* write */
    reg = readl(ETH_MACMIIAR_ADDR);
    reg &= 0x1c;
    reg |= (paddr << 11) | (raddr << 6) | (1 << 1) | (1 << 0);              /* 给busy位置1就可触发写操作 */
	writel(ETH_MACMIIAR_ADDR, reg);
	
    timeout = 0x0004FFFF;
	
    /* wait MMAR_MB cleared */
    while (timeout --) {
        reg = readl(ETH_MACMIIAR_ADDR);
        if ((reg & 0x01U) == 0)
            break;
    }
    if (timeout <= 0) return -1;
    else return 0;
}

#define USER_PHY_ADDR 0x00

static void rx_descr_init (void)
{
    unsigned int i, next;

    rx_buf_index = 0;

    for (i = 0, next = 0; i < NUM_RX_BUF; i++)
    {
        if (++next == NUM_RX_BUF) next = 0;

        rx_desc[i].stat = DMA_RX_OWN;
        rx_desc[i].ctrl = DMA_RX_RCH | ETH_BUF_SIZE;
        rx_desc[i].addr = (unsigned int)&rx_buf[i];
        rx_desc[i].next = (unsigned int)&rx_desc[next];
    }

    /* 接收描述符列表地址寄存器指向接收描述符列表的起始处 */
    ETH->DMARDLAR = (unsigned int)&rx_desc[0];
}

static void tx_descr_init (void)
{
    unsigned int i, next;

    tx_buf_index = 0;

    for (i = 0, next = 0; i < NUM_TX_BUF; i++)
    {
        if (++next == NUM_TX_BUF) next = 0;

        tx_desc[i].ctrlstat = DMA_TX_TCH | DMA_TX_LS | DMA_TX_FS;
        tx_desc[i].addr     = (unsigned int)&tx_buf[i];
        tx_desc[i].next     = (unsigned int)&tx_desc[next];
    }

    /* 发送描述符列表地址寄存器指向发送描述符列表的起始处 */
    ETH->DMATDLAR = (unsigned int)&tx_desc[0];
}
char     own_hw_adr[6] = {_MAC1, _MAC2, _MAC3, _MAC4, _MAC5, _MAC6};
/* init ethernet controller with RMII phy interface */
int stm32_eth_ll_init_rmii(void)
{
    int reg, idx = 0;
	int timeout;
	
    /* decription of port and pin for init */                               /* 用于初始化，适配不同的硬件连接方式 */
    static pad_dsc_t eth_phy_rmii[] = {
        {.name = "RMII_TXD0",    .gpio_dsc = {STM32_GPIO_PORT_G, STM32_GPIO_PIN_13}, .af_num = STM32_GPIO_AF_11},
        {.name = "RMII_TXD1",    .gpio_dsc = {STM32_GPIO_PORT_G, STM32_GPIO_PIN_14}, .af_num = STM32_GPIO_AF_11},
        {.name = "RMII_TX_EN",   .gpio_dsc = {STM32_GPIO_PORT_B, STM32_GPIO_PIN_11}, .af_num = STM32_GPIO_AF_11},
        {.name = "RMII_RXD0",    .gpio_dsc = {STM32_GPIO_PORT_C, STM32_GPIO_PIN_4 }, .af_num = STM32_GPIO_AF_11},
        {.name = "RMII_RXD1",    .gpio_dsc = {STM32_GPIO_PORT_C, STM32_GPIO_PIN_5 }, .af_num = STM32_GPIO_AF_11},
        {.name = "RMII_CRS_DV",  .gpio_dsc = {STM32_GPIO_PORT_A, STM32_GPIO_PIN_7 }, .af_num = STM32_GPIO_AF_11}, /* rx data valid */
        {.name = "RMII_REF_CLK", .gpio_dsc = {STM32_GPIO_PORT_A, STM32_GPIO_PIN_1 }, .af_num = STM32_GPIO_AF_11},  /* input, from 50Mhz OSC or from PHY IC */
    };

    /* F429IGT6引脚定义，使用其他芯片需要修改 */
    static pad_dsc_t smi_bus[] = {
        {.name = "SMI_MDC",      .gpio_dsc = {STM32_GPIO_PORT_C, STM32_GPIO_PIN_1 }, .af_num = STM32_GPIO_AF_11},     /* smi clock */
        {.name = "SMI_MDIO",     .gpio_dsc = {STM32_GPIO_PORT_A, STM32_GPIO_PIN_2 }, .af_num = STM32_GPIO_AF_11},     /* smi data */
    };

    /* no differences between stm32f4xx series */
#if defined(STM32F429_439xx) || defined(STM32F42xx) \
    || defined(STM32F43xx)
    (void)0;
#elif defined(STM32F405xx) || defined(STM32F407xx) \
      || defined(STM32F415xx) || defined(STM32F417xx)
    (void)0;
#endif

    /* enable syscfg clock */
    reg = readl(RCC_APB2ENR_ADDR);
    reg |= 1 << 14; 
    writel(RCC_APB2ENR_ADDR, reg);

    /* reset ether mac controller */
    reg = readl(RCC_AHB1RSTR_ADDR);
    reg |= (1 << 25);
    writel(RCC_AHB1RSTR_ADDR, reg);

    /* configure syscfg pmc(peripheral mode configuration) to select rmii interface */
    reg = readl(SYSCFG_PMC_ADDR);
    reg |= (1 << 23);
    writel(SYSCFG_PMC_ADDR, reg);

    /* stop reset ether mac controller */
    reg = readl(RCC_AHB1RSTR_ADDR);
    reg &= ~(1 << 25);
    writel(RCC_AHB1RSTR_ADDR, reg);

	/* enable ethernet clock */
	reg = readl(RCC_AHB1ENR_ADDR);
	reg |= (0xfU << 25);
	writel(RCC_AHB1ENR_ADDR, reg);

    /* Init RMII interface */
    for ( ; idx < sizeof(eth_phy_rmii) / sizeof(eth_phy_rmii[0]); ++ idx) {

        reg = readl(RCC_AHB1ENR_ADDR);                                      /* enable gpio clock first, although init so many times, it doesn't matter */
        reg |= (1 << eth_phy_rmii[idx].gpio_dsc.port);
        writel(RCC_AHB1ENR_ADDR, reg);

        rmii_pin_ctl.af = eth_phy_rmii[idx].af_num;
        stm32_gpio_ll_init(&eth_phy_rmii[idx].gpio_dsc, &rmii_pin_ctl);
        rmii_pin_ctl.af = 0;
    }

    /* Init SMI interface */
    idx = 0;
    for ( ; idx < sizeof(smi_bus) / sizeof(smi_bus[0]); ++ idx) {

        reg = readl(RCC_AHB1ENR_ADDR);                                      /* enable gpio clock first, although init so many times, it doesn't matter */
        reg |= (1 << smi_bus[idx].gpio_dsc.port);
        writel(RCC_AHB1ENR_ADDR, reg);

        rmii_pin_ctl.af = smi_bus[idx].af_num;
        stm32_gpio_ll_init(&smi_bus[idx].gpio_dsc, &rmii_pin_ctl);
        rmii_pin_ctl.af = 0;
    }

	ETH->DMABMR  |= 0x00000001;

    while (ETH->DMABMR & 0x00000001);
	
    /* Init SMI interface and init PHY IC firstly,
     * and then according to the phy status to configure the mac controller
     * (comment: 先配置SMI接口和PHY IC，然后根据PHY IC的状态来配置MAC控制器)
     */

    writel(ETH_MACMIIAR_ADDR, 0x10U);                                       	/* set SMI clock freq */
	
	int phy_reg;
	
	PHY_RESET = 0;													/* hardware reset */
 	OSTimeDly(500);	
	PHY_RESET = 1;

    if(write_phy(USER_PHY_ADDR, MII_BCR, MII_BCR_RESET)) return -1;         	/* soft reset and wait reset bit cleared */
	while (((0x8000) & (phy_reg = read_phy(USER_PHY_ADDR, MII_BCR)))){};
	while (0 == ((0x04) & (phy_reg = read_phy(USER_PHY_ADDR, MII_BSR)))){};    	/* wait for link status 检查网线是否插上了 */
	write_phy(USER_PHY_ADDR, MII_BCR, MII_BCR_AUTO_NEG);						/* enable auto-negotiation */
	while (0 == ((0x20) & (phy_reg = read_phy(USER_PHY_ADDR, MII_BSR)))){};		/* wait util auto-negotiation complete */

    /* enable ether irq if needed */
	(void)0;
	
	ETH->MACCR  = MCR_ROD;
		
	/* configure MAC registers */
    phy_reg = read_phy(USER_PHY_ADDR, 31);	                                    /* get speed status register */
    reg = readl(ETH_MACCR_ADDR);
	if ( phy_reg & 0x04 ) { /* speed 10Mbps */

    } else {                /* speed 100Mbps */
        reg |= (1 << 14);
    }
	
    if ( phy_reg & 0x10 ) { /* double duplex */
        reg |= (1 << 11);
    } else {                /* half duplex */

    }
//    reg |= (3 << 2);        /* enable recieve and transmit */
    writel(ETH_MACCR_ADDR, reg);

//    reg = (1 << 10) | (1 << 4);
//    writel(ETH_MACFFR_ADDR, reg); /* configure filter */

//    writel(ETH_MACFCR_ADDR, 1 << 7); /* configure flow control */

    ETH->MACFFR = MFFR_HPF | MFFR_PAM;	   									 /* MACFFR 以太网帧过滤寄存器，配置可接收所有MAC组播包，即MAC地址第一个字节的bit0 = 1 */


    ETH->MACFCR = MFCR_ZQPD;    														/* MACFCR 以太网流控制寄存器，ZQPD零时间片暂停禁止 */


    ETH->MACA0HR = ((unsigned int)own_hw_adr[5] <<  8) | (unsigned int)own_hw_adr[4];    /* 设置以太网MAC地址寄存器 */
    ETH->MACA0LR = ((unsigned int)own_hw_adr[3] << 24) | (unsigned int)own_hw_adr[2] << 16 |
                   ((unsigned int)own_hw_adr[1] <<  8) | (unsigned int)own_hw_adr[0];
	/* DMA init
	 * first,create DMA descriptor
	 * then,init DMA registers
	 */
	rx_descr_init ();					/* init descriptors */
    tx_descr_init ();
	
	ETH->DMAOMR = DOMR_FTF | DOMR_ST | DOMR_SR;

    ETH->MACCR |= MCR_TE | MCR_RE;
    ETH->DMASR  = 0xFFFFFFFF;
    ETH->DMAIER = ETH_DMAIER_NISE | ETH_DMAIER_AISE | ETH_DMAIER_RBUIE | ETH_DMAIER_RIE;

    NVIC_SetPriority(ETH_IRQn, 0);
	NVIC->ISER[1] = 1 << 29;
	return 0;
}

void eth_ll_frame_transmit(char * data, int len)
{
    unsigned int  *sp, *dp;
    unsigned int  i, j;

    j = tx_buf_index;
    while (tx_desc[j].ctrlstat & DMA_TX_OWN);

    sp = (unsigned int *)data;
    dp = (unsigned int *)(tx_desc[j].addr & ~3);
    for (i = (len + 3) >> 2; i; i--)
    {
        *dp++ = *sp++;
    }

    tx_desc[j].size      = len;
    tx_desc[j].ctrlstat |= DMA_TX_OWN;

    if (++j == NUM_TX_BUF) j = 0;
    tx_buf_index = j;

    ETH->DMASR   = DSR_TPSS;
    ETH->DMATPDR = 0;
}

extern mem_mng_t g_mem_mng;
struct stm32_eth * g_eth = NULL;
extern ipgui_membox_t eth_ex_mng;
void ETH_IRQHandler (void)
{
	if (!g_eth) return;
	
	struct eth_buffer * frame;
    unsigned int i, rx_len;
    unsigned int * sp, * dp;

    i = rx_buf_index;

    do {
        if (rx_desc[i].stat & DMA_RX_ERROR_MASK)
            goto rel;

        if ((rx_desc[i].stat & DMA_RX_SEG_MASK) != DMA_RX_SEG_MASK)
            goto rel;

        rx_len = ((rx_desc[i].stat >> 16) & 0x3FFF) - 4;

        if (rx_len > 1500)
            goto rel;
		LED0 = !LED0;
//        frame = mem_alloc(&g_mem_mng, sizeof(struct eth_buffer) + rx_len);
		frame = (struct eth_buffer *)ipgui_membox_alloc(&eth_ex_mng);
        if (frame != (struct eth_buffer *)0) {
            sp = (unsigned int *)(rx_desc[i].addr & ~3);
            dp = (unsigned int *)(frame->payload);
			frame->len = rx_len;
            for (rx_len = (rx_len + 3) >> 2; rx_len; rx_len--)
            {
                * dp++ = * sp++;
            }
			list_head_init(&frame->link);
            list_add(&frame->link, &g_eth->nbuf_head);
        } else {
			goto rel;
		}
rel:
        rx_desc[i].stat = DMA_RX_OWN;

        if (++ i == NUM_RX_BUF) i = 0;
    }
    while (!(rx_desc[i].stat & DMA_RX_OWN));

    rx_buf_index = i;

    if (ETH->DMASR & INT_RBUIE) {
        ETH->DMASR = ETH_DMASR_RBUS;
        ETH->DMARPDR = 0;
    }

    ETH->DMASR = ETH_DMASR_NIS | ETH_DMASR_AIS | ETH_DMASR_RS;

}

