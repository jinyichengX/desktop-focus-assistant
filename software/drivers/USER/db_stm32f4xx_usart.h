#ifndef __DB_STM32F4XX_USART_H__
#define __DB_STM32F4XX_USART_H__

#include "db_stm32f4xx_gpio.h"

#define STM32_USART1    1
#define STM32_USART2    2
#define STM32_USART3    3
#define STM32_USART4    4
#define STM32_USART5    5
#define STM32_USART6    6
#define STM32_USART7    7
#define STM32_USART8    8

/* macros of usart clock bit */
#define STM32F4xx_USART_SR_TXE (1 << 7)

typedef struct
{
    int id; /* 1 ~ 8 */
	stm32_gpio_dsc_t uio_tx;
    stm32_gpio_dsc_t uio_rx;
	const char * name;
}usart_dsc_t;

typedef struct
{
    unsigned int stop_bit : 2; /* 0: 1bit
                         1: 0.5bit 
                         2: 2bit 
                         3: 1.5bit */
    unsigned int parity : 2; /* 0: none
                       1: odd奇校验
                       2: even偶校验 */
	unsigned int reserved : 28; 
    int bps;
}usart_ctl_t;
extern void db_stm32f4xx_usart_putc(usart_dsc_t * usart_dsc, char c);
extern void db_stm32f4xx_usart_puts(usart_dsc_t * usart_dsc, const char * s);
extern int  db_stm32f4xx_usart_init(usart_dsc_t * usart_dsc, usart_ctl_t * ctl);

#endif

