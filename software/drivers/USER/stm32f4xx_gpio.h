#ifndef STM32_GPIO_H
#define STM32_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#define	STM32_GPIO_PORT_A 0
#define	STM32_GPIO_PORT_B 1
#define	STM32_GPIO_PORT_C 2
#define	STM32_GPIO_PORT_D 3
#define	STM32_GPIO_PORT_E 4
#define	STM32_GPIO_PORT_F 5
#define	STM32_GPIO_PORT_G 6
#define	STM32_GPIO_PORT_H 7
#define	STM32_GPIO_PORT_I 8

#define STM32_GPIO_PIN_0 0
#define STM32_GPIO_PIN_1 1
#define STM32_GPIO_PIN_2 2
#define STM32_GPIO_PIN_3 3
#define STM32_GPIO_PIN_4 4
#define STM32_GPIO_PIN_5 5
#define STM32_GPIO_PIN_6 6
#define STM32_GPIO_PIN_7 7
#define STM32_GPIO_PIN_8 8
#define STM32_GPIO_PIN_9 9
#define STM32_GPIO_PIN_10 10
#define STM32_GPIO_PIN_11 11
#define STM32_GPIO_PIN_12 12
#define STM32_GPIO_PIN_13 13
#define STM32_GPIO_PIN_14 14
#define STM32_GPIO_PIN_15 15

#define STM32_GPIO_MODE_INPUT 0
#define STM32_GPIO_MODE_OUTPUT 1
#define STM32_GPIO_MODE_AF 2
#define STM32_GPIO_MODE_ANALOG 3

#define STM32_GPIO_AF_0 0
#define STM32_GPIO_AF_1 1
#define STM32_GPIO_AF_2 2
#define STM32_GPIO_AF_3 3
#define STM32_GPIO_AF_4 4
#define STM32_GPIO_AF_5 5
#define STM32_GPIO_AF_6 6
#define STM32_GPIO_AF_7 7
#define STM32_GPIO_AF_8 8
#define STM32_GPIO_AF_9 9
#define STM32_GPIO_AF_10 10
#define STM32_GPIO_AF_11 11
#define STM32_GPIO_AF_12 12
#define STM32_GPIO_AF_13 13
#define STM32_GPIO_AF_14 14
#define STM32_GPIO_AF_15 15

typedef struct {
    unsigned int port : 16;
    unsigned int pin : 16;
}stm32_gpio_dsc_t;

typedef struct {
    unsigned int mode : 2;
    unsigned int otype : 1;
    unsigned int speed : 2;
    unsigned int pupd : 2;
    unsigned int af : 4;
}stm32_gpio_ctl_t;

#ifndef writel
#define writel(a, v)            (*(volatile unsigned int *)(a) = (v))
#endif
#ifndef readl
#define readl(a)                (*(volatile unsigned int *)(a))
#endif

/* gpio registers */
#define GPIO_BASE_ADDR          0x40020000                                  /* GPIO memory map base */
#define GPIO_PORT(n)            (GPIO_BASE_ADDR + (0x400 * (n)))
#define GPIOR(n, i)             (GPIO_PORT(n) + 0x4 * (i))

/* gpio mode definition */
#define GPIO_MODE_INPUT         0                                           /*input, reset state */
#define GPIO_MODE_OUTPUT        1
#define GPIO_MODE_AF            2
#define GPIO_MODE_ANALOG        3

/* gpio output type definition */
#define GPIO_OTYPE_PP           0                                           /* push-pull, reset state */
#define GPIO_OTYPE_OD           1                                           /* open-drain */

/* gpio output speed definition */
#define GPIO_SPEED_LOW          0                                           /* low speed, reset state */
#define GPIO_SPEED_MEDIUM       1                                           /* medium speed */
#define GPIO_SPEED_HIGH         2                                           /* high speed */                            
#define GPIO_SPEED_VERY_HIGH    3                                           /* very high speed */

/* gpio pull-up/pull-down definition */
#define GPIO_PUPD_NONE          0                                           /* no pull-up/pull-down, reset state */
#define GPIO_PUPD_UP            1
#define GPIO_PUPD_DOWN          2

static void stm32_gpio_ll_init(stm32_gpio_dsc_t * dsc, stm32_gpio_ctl_t * ctl)
{
    int reg;

    /* configure mode register */
    reg =  readl(GPIOR(dsc->port, 0));
    reg &= ~(0x03U << (dsc->pin << 1));
    reg |= (ctl->mode << (dsc->pin << 1));
    writel(GPIOR(dsc->port, 0), reg);

    /* configure output type register */
    reg = readl(GPIOR(dsc->port, 1));
    reg &= ~(1U << dsc->pin);
    reg |= (ctl->otype << dsc->pin);
    writel(GPIOR(dsc->port, 1), reg);

    /* configure output speed register */
    reg =  readl(GPIOR(dsc->port, 2));
    reg &= ~(0x03U << (dsc->pin << 1));
    reg |= (ctl->speed << (dsc->pin << 1));
    writel(GPIOR(dsc->port, 2), reg);

    /* configure pull-up/pull-down register */
    reg =  readl(GPIOR(dsc->port, 3));
    reg &= ~(0x03U << (dsc->pin << 1));
    reg |= (ctl->pupd << (dsc->pin << 1));
    writel(GPIOR(dsc->port, 3), reg);
    
    /* if needed, configure alternate function register */
    if (2 == ctl->mode)
    {
        if (dsc->pin < 8)
        {
            reg =  readl(GPIOR(dsc->port, 8));
            reg &= ~(0x0fU << (dsc->pin << 2));
            reg |= (ctl->af << (dsc->pin << 2));
            writel(GPIOR(dsc->port, 8), reg);
        } else
        {
            reg =  readl(GPIOR(dsc->port, 9));
            reg &= ~(0x0fU << ((dsc->pin - 8) << 2));
            reg |= (ctl->af << ((dsc->pin - 8) << 2));
            writel(GPIOR(dsc->port, 9), reg);
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif // STM32F429_39XX_H