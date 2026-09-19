#include "db_stm32f4xx_usart.h"
#include "db_cm4_nvic.h"

#ifndef readl
#define readl(addr)         (*(volatile unsigned int *)(addr))
#endif
#ifndef writel
#define writel(addr, val)   (*(volatile unsigned int *)(addr) = (val))
#endif

/* clock registers */
#define STM32F4xx_RCC           0x40023800                                  /* RCC memory map base */

#define STM32F4xx_RCC_PLLCFGR   (STM32F4xx_RCC + 0x04) 						/* RCC PLL configuration register */
#define STM32F4xx_RCC_CFGR		(STM32F4xx_RCC + 0x08) 						/* RCC clock configuration register */

#define STM32F4xx_RCC_APB1RSTR  (STM32F4xx_RCC + 0x20)                      /* RCC APB1 peripheral clock reset register  */
#define STM32F4xx_RCC_APB2RSTR  (STM32F4xx_RCC + 0x24)                      /* RCC APB2 peripheral clock reset register  */

#define STM32F4xx_RCC_APB1ENR   (STM32F4xx_RCC + 0x40)                      /* RCC APB1 peripheral clock enable register */
#define STM32F4xx_RCC_APB2ENR   (STM32F4xx_RCC + 0x44)                      /* RCC APB2 peripheral clock enable register */
#define STM32F4xx_RCC_AHB1ENR   (STM32F4xx_RCC + 0x30)                      /* RCC AHB1 peripheral clock enable register */

/* usart registers */
#define STM32F4xx_USART1        0x40011000U
#define STM32F4xx_USART2        0x40004400U
#define STM32F4xx_USART3        0x40004800U
#define STM32F4xx_USART6        0x40011400U

#define STM32F4xx_UART4         0x40004C00U
#define STM32F4xx_UART5         0x40005000U
#define STM32F4xx_UART7         0x40007800U
#define STM32F4xx_UART8         0x40007C00U

#define _STM32F4xx_USART_REG(n, offset) \
    ((n == 1) ? (STM32F4xx_USART1 + offset) : \
     (n == 2) ? (STM32F4xx_USART2 + offset) : \
     (n == 3) ? (STM32F4xx_USART3 + offset) : \
     (n == 6) ? (STM32F4xx_USART6 + offset) : 0)

#define _STM32F4xx_UART_REG(n, offset) \
    ((n == 4) ? (STM32F4xx_UART4 + offset) : \
     (n == 5) ? (STM32F4xx_UART5 + offset) : \
     (n == 7) ? (STM32F4xx_UART7 + offset) : \
     (n == 8) ? (STM32F4xx_UART8 + offset) : 0)

//n: 1 2 3 6
#define STM32F4xx_USART_SR(n)   _STM32F4xx_USART_REG((n), 0x00)
#define STM32F4xx_USART_DR(n)   _STM32F4xx_USART_REG((n), 0x04)
#define STM32F4xx_USART_BRR(n)  _STM32F4xx_USART_REG((n), 0x08)
#define STM32F4xx_USART_CR1(n)  _STM32F4xx_USART_REG((n), 0x0C)
#define STM32F4xx_USART_CR2(n)  _STM32F4xx_USART_REG((n), 0x10)
#define STM32F4xx_USART_CR3(n)  _STM32F4xx_USART_REG((n), 0x14)
#define STM32F4xx_USART_GTPR(n) _STM32F4xx_USART_REG((n), 0x18)

//n: 4 5 7 8其中只有f42xx f43xx有usart7和8
#define STM32F4xx_UART_SR(n)    _STM32F4xx_UART_REG((n),  0x00)
#define STM32F4xx_UART_DR(n)    _STM32F4xx_UART_REG((n),  0x04)
#define STM32F4xx_UART_BRR(n)   _STM32F4xx_UART_REG((n),  0x08)
#define STM32F4xx_UART_CR1(n)   _STM32F4xx_UART_REG((n),  0x0C)
#define STM32F4xx_UART_CR2(n)   _STM32F4xx_UART_REG((n),  0x10)
#define STM32F4xx_UART_CR3(n)   _STM32F4xx_UART_REG((n),  0x14)
#define STM32F4xx_UART_GTPR(n)  _STM32F4xx_UART_REG((n),  0x18)

#define STM32F4xx_USART1_IRQ    37
#define STM32F4xx_USART2_IRQ    38
#define STM32F4xx_USART3_IRQ    39
#define STM32F4xx_UART4_IRQ     52
#define STM32F4xx_UART5_IRQ     53
#define STM32F4xx_USART6_IRQ    71
#define STM32F4xx_UART7_IRQ     82
#define STM32F4xx_UART8_IRQ     83

/* DMA registers */
#define STM32F4xx_DMA1_BASE     0x40026000//21bit offset in ahb1 enr register
#define STM32F4xx_DMA2_BASE     0x40026400//22bit offset in ahb1 enr register

#define STM32F4xx_DMA1_LISR             (STM32F4xx_DMA1_BASE + 0x00)
#define STM32F4xx_DMA1_HISR             (STM32F4xx_DMA1_BASE + 0x04)
#define STM32F4xx_DMA1_LIFCR            (STM32F4xx_DMA1_BASE + 0x08)
#define STM32F4xx_DMA1_HIFCR            (STM32F4xx_DMA1_BASE + 0x0C)
#define STM32F4xx_DMA1_CR_STREAM(n)     (STM32F4xx_DMA1_BASE + 0x10 + 0x18 * (n)) /* n from 0 ~ 7 */
#define STM32F4xx_DMA1_NTDR_STREAM(n)   (STM32F4xx_DMA1_BASE + 0x14 + 0x18 * (n)) /* n from 0 ~ 7 */
#define STM32F4xx_DMA1_PAR_STREAM(n)    (STM32F4xx_DMA1_BASE + 0x18 + 0x18 * (n)) /* n from 0 ~ 7 */
#define STM32F4xx_DMA1_M0AR_STREAM(n)   (STM32F4xx_DMA1_BASE + 0x1c + 0x18 * (n)) /* n from 0 ~ 7 */
#define STM32F4xx_DMA1_M1AR_STREAM(n)   (STM32F4xx_DMA1_BASE + 0x20 + 0x18 * (n)) /* n from 0 ~ 7 */
#define STM32F4xx_DMA1_FCR_STREAM(n)    (STM32F4xx_DMA1_BASE + 0x24 + 0x18 * (n)) /* n from 0 ~ 7 */

#define STM32F4xx_DMA2_LISR             (STM32F4xx_DMA2_BASE + 0x00)
#define STM32F4xx_DMA2_HISR             (STM32F4xx_DMA2_BASE + 0x04)
#define STM32F4xx_DMA2_LIFCR            (STM32F4xx_DMA2_BASE + 0x08)
#define STM32F4xx_DMA2_HIFCR            (STM32F4xx_DMA2_BASE + 0x0C)
#define STM32F4xx_DMA2_CR_STREAM(n)     (STM32F4xx_DMA2_BASE + 0x10 + 0x18 * (n)) /* n from 0 ~ 7 */
#define STM32F4xx_DMA2_NTDR_STREAM(n)   (STM32F4xx_DMA2_BASE + 0x14 + 0x18 * (n)) /* n from 0 ~ 7 */
#define STM32F4xx_DMA2_PAR_STREAM(n)    (STM32F4xx_DMA2_BASE + 0x18 + 0x18 * (n)) /* n from 0 ~ 7 */
#define STM32F4xx_DMA2_M0AR_STREAM(n)   (STM32F4xx_DMA2_BASE + 0x1c + 0x18 * (n)) /* n from 0 ~ 7 */
#define STM32F4xx_DMA2_M1AR_STREAM(n)   (STM32F4xx_DMA2_BASE + 0x20 + 0x18 * (n)) /* n from 0 ~ 7 */
#define STM32F4xx_DMA2_FCR_STREAM(n)    (STM32F4xx_DMA2_BASE + 0x24 + 0x18 * (n)) /* n from 0 ~ 7 */

typedef struct {
    int id;
    unsigned int enr;   /* clock enable register */
    unsigned int rstr;  /* clock reset register */
    int offset;         /* clock enable or reset bit offset */
    int irq_num;
}usart_clk_irq_tbl_t;

static const usart_clk_irq_tbl_t clk_irq_tbl[] = {
	{0,0,0,0},
    {STM32_USART1, STM32F4xx_RCC_APB2ENR, STM32F4xx_RCC_APB2RSTR, 4 , STM32F4xx_USART1_IRQ},
    {STM32_USART2, STM32F4xx_RCC_APB1ENR, STM32F4xx_RCC_APB1RSTR, 17, STM32F4xx_USART2_IRQ},
    {STM32_USART3, STM32F4xx_RCC_APB1ENR, STM32F4xx_RCC_APB1RSTR, 18, STM32F4xx_USART3_IRQ},
    {STM32_USART4, STM32F4xx_RCC_APB1ENR, STM32F4xx_RCC_APB1RSTR, 19, STM32F4xx_UART4_IRQ },
    {STM32_USART5, STM32F4xx_RCC_APB1ENR, STM32F4xx_RCC_APB1RSTR, 20, STM32F4xx_UART5_IRQ },
    {STM32_USART6, STM32F4xx_RCC_APB2ENR, STM32F4xx_RCC_APB2RSTR, 5 , STM32F4xx_USART6_IRQ},
    {STM32_USART7, STM32F4xx_RCC_APB1ENR, STM32F4xx_RCC_APB1RSTR, 30, STM32F4xx_UART7_IRQ },
    {STM32_USART8, STM32F4xx_RCC_APB1ENR, STM32F4xx_RCC_APB1RSTR, 31, STM32F4xx_UART8_IRQ },
};

int db_stm32f4xx_APB1_clock_get(void);
int db_stm32f4xx_APB2_clock_get(void);

void db_stm32f4xx_usart_putc(usart_dsc_t * usart_dsc, char c)
{
    while(0 == (readl(STM32F4xx_USART_SR(usart_dsc->id)) & STM32F4xx_USART_SR_TXE));
    writel(STM32F4xx_USART_DR(usart_dsc->id), c);
}

void db_stm32f4xx_usart_puts(usart_dsc_t * usart_dsc, const char * s)
{
    while (*s != '\0') {
        db_stm32f4xx_usart_putc(usart_dsc, *s++);
    }
}

static unsigned int db_stm32f4xx_hsi_get(void)
{
    return 16000000U; /* user define this value */
}

static unsigned int db_stm32f4xx_hse_get(void)
{
    return 25000000U;  /* user define this value */
}

static unsigned int db_stm32f4xx_pllm_get(void)
{
    unsigned int pllm;
    pllm = readl(STM32F4xx_RCC_PLLCFGR) & 0x3f;
    if(pllm <= 1) {
        /* wrong value*/ 
        return 0;
    } else {
        return pllm;
    }
}

static unsigned int db_stm32f4xx_plln_get(void)
{
    unsigned int plln;
    plln = (readl(STM32F4xx_RCC_PLLCFGR) >> 6) & 0x1ff;
    if((plln <= 1) || (plln >= 433)) {
        /* wrong value */
        return 0;
    } else {
        return plln;
    }
}

static unsigned int db_stm32f4xx_pllp_get(void)
{
    unsigned int temp;
    temp = (readl(STM32F4xx_RCC_PLLCFGR) >> 16) & 0x03;
    if(temp == 0) return 2;
    else if(temp == 1) return 4;
    else if(temp == 2) return 6;
    else if(temp == 3) return 8;
}

static unsigned int db_stm32f4xx_pll_get(void)
{
    unsigned int pll_src_freq;
    /* get pll source(hsi or hse) */
    if (readl(STM32F4xx_RCC_PLLCFGR) & (1 << 22)) {
        /* hse */
        pll_src_freq = db_stm32f4xx_hse_get();
    } else {
        /* hsi */
        pll_src_freq = db_stm32f4xx_hsi_get();
    }
    return (unsigned int) 
        (((unsigned long long)pll_src_freq \
        * db_stm32f4xx_plln_get()) \
        / db_stm32f4xx_pllm_get()  \
        / db_stm32f4xx_pllp_get());
}

static unsigned int db_stm32f4xx_sysclk_get(void)
{
#define SYS_CLK_SELECTED_HSI     0U
#define SYS_CLK_SELECTED_HSE     1U
#define SYS_CLK_SELECTED_PLL     2U
#define SYS_CLK_SELECTED_INVALID 3U
    int sys_clk_src, src_freq;
    sys_clk_src = (readl(STM32F4xx_RCC_CFGR) >> 2) & 0x03;
    switch (sys_clk_src) {
        case SYS_CLK_SELECTED_HSI: src_freq = db_stm32f4xx_hsi_get(); break;
        case SYS_CLK_SELECTED_HSE: src_freq = db_stm32f4xx_hse_get(); break;
        case SYS_CLK_SELECTED_PLL: src_freq = db_stm32f4xx_pll_get(); break;
        default: /* 0b11 : SYS_CLK_SELECTED_INVALID */break;
    }
	return src_freq;
}

static unsigned int db_stm32f4xx_hclk_get(void)
{
	int hpre;
	hpre = (readl(STM32F4xx_RCC_CFGR) >> 4) & 0x0f;
	switch(hpre) {
		case 8:  hpre = 2;   break;
		case 9:  hpre = 4;   break;
		case 10: hpre = 8;   break;
		case 11: hpre = 16;  break;
		case 12: hpre = 64;  break;
		case 13: hpre = 128; break;
		case 14: hpre = 256; break;
		case 15: hpre = 512; break;
		default/* 0xxx */ :hpre = 1;   break;
	}
	return db_stm32f4xx_sysclk_get() / hpre;
}

static int db_stm32f4xx_APB1_clock_get(void)
{
	int ppre1;
	ppre1 = (readl(STM32F4xx_RCC_CFGR) >> 10) & 0x07;
	switch(ppre1) {
		case 0: case 1:	case 2: case 3: ppre1 = 1; break;
		case 4: ppre1 = 2;  break;
		case 5: ppre1 = 4;  break;
		case 6: ppre1 = 8;  break;
		case 7: ppre1 = 16; break;
		default : return -1;
	}
	return db_stm32f4xx_hclk_get() / ppre1;
}

static int db_stm32f4xx_APB2_clock_get(void)
{
	int ppre2;
	ppre2 = (readl(STM32F4xx_RCC_CFGR) >> 13) & 0x07;
	switch(ppre2) {
		case 0: case 1:	case 2: case 3: ppre2 = 1; break;
		case 4: ppre2 = 2;  break;
		case 5: ppre2 = 4;  break;
		case 6: ppre2 = 8;  break;
		case 7: ppre2 = 16; break;
		default : return -1;
	}
	return db_stm32f4xx_hclk_get() / ppre2;
}

/* Equation 1: Baud rate for standard USART (SPI mode included) 
 * BUADRATE = fck / (8* (2 - OVER8) * UARTDIV) OVER8 = 0/1
 * Equation 2: Baud rate in Smartcard, LIN and IrDA modes
 * BUADRATE = fck / (16 * USARTDIV)
 */
static int db_stm32f4xx_baudrate_set(usart_dsc_t * usart_dsc, int bps)//used for oversample 16
{
    int reg = 0;
    unsigned int apb_clk = 0;
    unsigned int int_div, frac_div;

    if ((usart_dsc->id == 1) || \
        (usart_dsc->id == 6)) {
        apb_clk = db_stm32f4xx_APB2_clock_get();                    /* apb2 clock max: 90Mhz */
        if (apb_clk > 90000000)
            return 1;
    } else {
        apb_clk = db_stm32f4xx_APB1_clock_get();                    /* apb2 clock max: 45Mhz */
        if (apb_clk > 45000000)
            return 1;
    }

    int_div = (25 * apb_clk) / (4 * bps); //actual div * 100
    reg = ((int_div / 100) << 4);                                     /* fill int division */
    frac_div = int_div % 100;
    reg |= ((((frac_div * 16) + 50) / 100) << 0);                     /* fill frac division */
    writel(STM32F4xx_USART_BRR(usart_dsc->id), reg);

    return 0;
}

char rx_buffer[100] = {0};
int rx_len;/* 一次接收到的数据包长度 */

int db_stm32f4xx_usart_init(usart_dsc_t * usart_dsc, usart_ctl_t * ctl)
{
    int reg = 1000, ret;

    stm32_gpio_ctl_t uio_ctl;

    uio_ctl.mode  = GPIO_MODE_AF;
    uio_ctl.otype = GPIO_OTYPE_PP;
    uio_ctl.pupd  = GPIO_PUPD_UP;
    uio_ctl.speed = GPIO_SPEED_VERY_HIGH;

    if ((usart_dsc->id > 8) || (usart_dsc->id < 1))
        return 1;

    if ((usart_dsc->id == 1) || (usart_dsc->id == 2) || (usart_dsc->id == 3))
        uio_ctl.af = 7;
    else 
        uio_ctl.af = 8;

    /* reset usart clock */
    reg = readl(clk_irq_tbl[usart_dsc->id].rstr);
    reg |= (1 << clk_irq_tbl[usart_dsc->id].offset);
    writel(clk_irq_tbl[usart_dsc->id].rstr, reg);
    while (reg --);

    /* stop reset usart clock */
    reg = readl(clk_irq_tbl[usart_dsc->id].rstr);
    reg &= ~(1 << clk_irq_tbl[usart_dsc->id].offset);
    writel(clk_irq_tbl[usart_dsc->id].rstr, reg);

    /* enable usart gpio clock 
     * and configure usart gpio
     */
    stm32_gpio_clk_enable(&usart_dsc->uio_tx);
    stm32_gpio_ll_init(&usart_dsc->uio_tx, &uio_ctl);
	stm32_gpio_clk_enable(&usart_dsc->uio_rx);
    stm32_gpio_ll_init(&usart_dsc->uio_rx, &uio_ctl);
	
    /* enable usart clock 
     * and configure usart
     */
    reg = readl(clk_irq_tbl[usart_dsc->id].enr);              /* enable usart clock */
    reg |= (1 << clk_irq_tbl[usart_dsc->id].offset);
    writel(clk_irq_tbl[usart_dsc->id].enr, reg);

	if (ctl->parity != 0) {
		reg = readl(STM32F4xx_USART_CR1(usart_dsc->id));    /* set word length = 9 if has parity, because parity bit will override MSB */
		reg |= (1 << 12) | (1 << 10);
		if (ctl->parity == 1) reg |= (1 << 9);
		else reg &= ~(1 << 9);
		writel(STM32F4xx_USART_CR1(usart_dsc->id), reg);
	}

	reg = readl(STM32F4xx_USART_CR2(usart_dsc->id)); 		/* set stop bit length */
	reg &= ~(3 << 12);
	reg |= (ctl->stop_bit << 12);
    writel(STM32F4xx_USART_CR2(usart_dsc->id), reg);
	
    ret = db_stm32f4xx_baudrate_set(usart_dsc, ctl->bps);   /* set buadrate */
    if (ret != 0) return 2;

#if 1
    /* enable interrupt source first
     * and enable usart/uart global interrupt(NVIC)
     */
    reg = readl(STM32F4xx_USART_CR1(usart_dsc->id));        /* enable RXNE and IDLE interrupt */
//	reg |= (1 << 5);/* RXNE */
    reg |= (1 << 4);/* IDLE */
    writel(STM32F4xx_USART_CR1(usart_dsc->id), reg);
#endif

#if 1 //下面这部分代码只对串口1有效，其他串口参考这部分代码
    /* enable interrupt source first
     * and enable usart/uart global interrupt(NVIC)
     * configure DMA
     * enable DMA for receiver and transmitter
     */
/*     
                                            DMA1 request mapping
Peripheral  |   Stream0    |   Stream1    |   Stream2    |   Stream3    |   Stream4    |   Stream5    |   Stream6    |   Stream7       
requests    |              |
-----------------------------------------------------------------------------------------------------------------------------------
Channel 0   |   X          |   X          |   X          |   X          |   X          |   X          |   X          |   X
Channel 1   |   X          |   X          |   X          |   X          |   X          |   X          |   X          |   X
Channel 2   |   X          |   X          |   X          |   X          |   X          |   X          |   X          |   X
Channel 3   |   X          |   X          |   X          |   X          |   X          |   X          |   X          |   X
Channel 4   |   UART5_RX   |   USART3_RX  |   UART4_RX   |   USART3_TX  |   UART4_TX   |   USART2_RX  |   USART2_TX  |   UART5_TX
Channel 5   |   UART8_TX   |   UART7_TX   |   X          |   UART7_RX   |   X          |   X          |   UART8_RX   |   X
Channel 6   |   X          |   X          |   X          |   X          |   X          |   X          |   X          |   X
Channel 7   |   X          |   X          |   X          |   X          |   USART3_TX  |   X          |   X          |   X



                                            DMA2 request mapping
Peripheral  |   Stream0    |   Stream1    |   Stream2    |   Stream3    |   Stream4    |   Stream5    |   Stream6    |   Stream7       
requests    |              |
-----------------------------------------------------------------------------------------------------------------------------------
Channel 0   |   X          |   X          |   X          |   X          |   X          |   X          |   X          |   X
Channel 1   |   X          |   X          |   X          |   X          |   X          |   X          |   X          |   X
Channel 2   |   X          |   X          |   X          |   X          |   X          |   X          |   X          |   X
Channel 3   |   X          |   X          |   X          |   X          |   X          |   X          |   X          |   X
Channel 4   |   X          |   X          |   USART1_RX  |   X          |   X          |   USART1_RX  |   X          |   USART1_TX
Channel 5   |   X          |   USART6_RX  |   USART6_RX  |   X          |   X          |   X          |   USART6_TX  |   USART6_TX
Channel 6   |   X          |   X          |   X          |   X          |   X          |   X          |   X          |   X
Channel 7   |   X          |   X          |   X          |   X          |   X          |   X          |   X          |   X

*/
    /* enable dma1/dma2 clock first 
     * here only enable dma2 for usart1
     */
    reg = readl(STM32F4xx_RCC_AHB1ENR); /* enable dma2 clock */
    reg |= (1 << 22); // | (1 << 21);
    writel(STM32F4xx_RCC_AHB1ENR, reg);

    /* configure dma2 for usart1 */
    /* 配置为直接模式（与之对应的是突发模式，需要借助FIFO）和单次传输（与之对应的是循环传输，可以使用双缓冲） */
    reg = readl(STM32F4xx_DMA2_CR_STREAM(2));
    reg &= ~(1 << 0); /* 先失能DMA，否则下面的配置无效 */
    writel(STM32F4xx_DMA2_CR_STREAM(2), reg);

    reg = readl(STM32F4xx_DMA2_FCR_STREAM(2));/* 选择直接模式 */
    reg &= ~(1 << 2);
    writel(STM32F4xx_DMA2_FCR_STREAM(2), reg);

    reg = readl(STM32F4xx_DMA2_CR_STREAM(2));
    reg &= ~(0x07 << 25); 
    reg |= (4 << 25); /* usart1 rx选择channel 4 */
	writel(STM32F4xx_DMA2_CR_STREAM(2), reg);

    reg &= ~(0x0f << 11); /* 配置PSIZE和MSIZE都为8bit */
    reg |= (1 << 10); /* 内存增长 */
    reg &= ~(1 << 9);   /* 外设固定 */
    
    reg &= ~(0x03 << 6); /* peripheral to memory */

    reg &= ~(1 << 5);/* use peripheral flow control? */
    writel(STM32F4xx_DMA2_CR_STREAM(2), reg);

    writel(STM32F4xx_DMA2_PAR_STREAM(2), STM32F4xx_USART_DR(usart_dsc->id));/* 设置外设地址为usart的DR */
    writel(STM32F4xx_DMA2_M0AR_STREAM(2), (int)rx_buffer);/* 设置内存地址 */
    writel(STM32F4xx_DMA2_NTDR_STREAM(2), 0xffff);/* 设置为最大 *//* 当开启外设流(bit5 = 1)时，不管写什么值都无效，在Stream被使能时都会被强制置为0xffff */

    reg = readl(STM32F4xx_DMA2_CR_STREAM(2));/* 使能DMA stream2 */
    reg |= (1 << 0);
    writel(STM32F4xx_DMA2_CR_STREAM(2), reg);

    reg = readl(STM32F4xx_USART_CR3(usart_dsc->id));        /* enable DMA for receiver and transmitter */
    // reg |= (1 << 7);/* transmitter */
    reg |= (1 << 6);/* receiver */
    writel(STM32F4xx_USART_CR3(usart_dsc->id), reg);
#endif

    db_NVIC_irq_prio_set(clk_irq_tbl[usart_dsc->id].irq_num, 0);
    db_NVIC_enable_irq(clk_irq_tbl[usart_dsc->id].irq_num);

    reg = readl(STM32F4xx_USART_CR1(usart_dsc->id));        /* enable tx and rx */
    reg |= (1 << 3) | (1 << 2);
    writel(STM32F4xx_USART_CR1(usart_dsc->id), reg);

    reg = readl(STM32F4xx_USART_CR1(usart_dsc->id));        /* enable usart */
    reg |= (1 << 13);
    writel(STM32F4xx_USART_CR1(usart_dsc->id), reg);
	
	return 0;
}

/* 先读SR再读RDR以清除IDLE中断挂起状态 */
void clear_idle_irq_pend_flag(int uid)
{
	int reg;
	reg = readl(STM32F4xx_USART_SR(uid));
	reg = readl(STM32F4xx_USART_DR(uid));
}

/* 读RDR以清除RXNE中断挂起状态 */
void clear_rxne_irq_pend_flag(int uid)
{
	int reg;
	reg = readl(STM32F4xx_USART_DR(uid));
}

void USART1_IRQHandler(void)
{
	/* RXNE receive logic */
#if 0 /* if needed, set 1 */
	while (readl(STM32F4xx_USART_SR(1)) & (1 << 5)) {
		count ++;
		if (rx_num < 99) {
			rx_buffer[rx_num ++] = readl(STM32F4xx_USART_DR(1));

		} else {
			clear_rxne_irq_pend_flag(1);//不接收数据，就得手动清除中断标志位
		}
	}
#endif
	
	/* DMA receive logic */
	int reg;
	if (readl(STM32F4xx_USART_SR(1)) & (1 << 4)) {
		clear_idle_irq_pend_flag(1);
		
		rx_len = readl(STM32F4xx_DMA2_NTDR_STREAM(2));
		rx_len = 0xffff - rx_len;
		
		/* reset DMA NTDR */
		reg = readl(STM32F4xx_DMA2_CR_STREAM(2));/* 必须先失能DMA stream，但是失能DMA为什么会置位TCIF传输完成中断标志位?（猜测是硬件特性） */
		reg &= ~(1 << 0);
		writel(STM32F4xx_DMA2_CR_STREAM(2), reg);
		
		writel(STM32F4xx_DMA2_LIFCR, 0x200000);/* 必须先清除由失能DMA引起的中断标志，否则下面重新使能DMA stream会失败 */
		
		writel(STM32F4xx_DMA2_NTDR_STREAM(2), 0xffff);/* 设置为最大 */
		
		reg = readl(STM32F4xx_DMA2_CR_STREAM(2));/* 使能DMA stream */
		reg |= (1 << 0);
		writel(STM32F4xx_DMA2_CR_STREAM(2), reg);
	}
}

