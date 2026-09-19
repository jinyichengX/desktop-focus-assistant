#ifndef STM32F429_39XX_H
#define STM32F429_39XX_H

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_WIDTH 800
#define LCD_HEIGHT 480

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

typedef struct {
    unsigned int hsw : 16;      /* horizontal sync width  */
    unsigned int hbp : 8;       /* horizontal back porch  */
    unsigned int hfp : 8;       /* horizontal front porch */

    unsigned int vsh : 16;      /* vertical sync height   */
    unsigned int vbp : 8;       /* vertical back porch    */
    unsigned int vfp : 8;       /* vertical front porch   */

    unsigned int aw : 12;       /* active width           */
    unsigned int ah : 12;       /* active height          */

    unsigned int bpp : 8;       /* bits per pixel         */
}lcd_panel_param_t;
#define PERIPH_BASE             ((unsigned int)0x40000000) /*!< Peripheral base address in the alias region                                */
#define AHB1PERIPH_BASE         (PERIPH_BASE + 0x00020000)
#define RCC_BASE                (AHB1PERIPH_BASE + 0x3800)
#define RCC                     ((RCC_TypeDef *) RCC_BASE)
typedef unsigned short u16;
typedef unsigned int u32;
#ifdef __cplusplus
}
#endif
extern void stm32_ltdc_ll_init(unsigned int pix_clk, lcd_panel_param_t * pp, void * fb);
extern void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u32 color);
#endif // STM32F429_39XX_H