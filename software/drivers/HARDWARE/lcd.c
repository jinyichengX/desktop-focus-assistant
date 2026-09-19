/*
 * MIT License
 *
 * Copyright (c) 2025 JinYiCheng
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

#if !defined(STM32F429xx) && !defined(STM32F439xx)
#include "ipgui_utils.h"
#endif
#include "lcd.h"
#include "ipgui_cm4.h"
#ifndef ipgui_writel
#define ipgui_writel(a, v)                  (*(volatile unsigned int *)(a) = (v))
#endif
#ifndef ipgui_readl
#define ipgui_readl(a)                      (*(volatile unsigned int *)(a))
#endif
#include "stm32f429xx.h"
#include "stm32f4xx.h"
#include "db_stm32f4xx_gpio.h"
/* 这个驱动文件仅仅适配正点原子的阿波罗STM32F429IGT6
 * 适配其他型号的STM32芯片或者板卡需要修改驱动, 需要修改（1）面板参数 （2）像素时钟LCD_CLK （3）LCD引脚
 */
void LTDC_Color_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 *color);
void ltdc_layer1_reload_framebuffer_imediately(void * fb);
void dma2d_irq_init(void);
#define LCD_FRAME_BUF_ADDR			0XC0000000  
unsigned short ltdc_lcd_framebuf[LCD_WIDTH * LCD_HEIGHT] __attribute__((at(LCD_FRAME_BUF_ADDR)));	//定义最大屏分辨率时,LCD所需的帧缓存数组大小
unsigned short ltdc_lcd_framebuf1[LCD_WIDTH * LCD_HEIGHT] __attribute__((at(LCD_FRAME_BUF_ADDR + sizeof(unsigned short) * LCD_WIDTH * LCD_HEIGHT)));	//定义最大屏分辨率时,LCD所需的帧缓存数组大小
unsigned short * fore_lcd_framebuffer; //foreground framebuffer
unsigned short * back_lcd_framebuffer; //background framebuffer
char back_state = 0;
char back_render_ready = 0;
char dma_busy = 0;
int x1,y1,x2,y2; 
unsigned short * color;
/* clock */
#define RCC_BASE_ADDR           0x40023800                                  /* RCC memory map base */
#define RCC_AHB1ENR_ADDR        (RCC_BASE_ADDR + 0x30)                      /* RCC AHB1 peripheral clock enable register */
#define RCC_APB2ENR_ADDR        (RCC_BASE_ADDR + 0x44)                      /* RCC APB2 peripheral clock enable register */

/* gpio registers */
#define GPIO_BASE_ADDR          0x40020000                                  /* GPIO memory map base */
#define GPIO_PORT(n)            (GPIO_BASE_ADDR + (0x400 * (n)))
#define GPIOR(n, i)             (GPIO_PORT(n) + 0x4 * (i))

/* ltdc general registers */
#define LTDC_BASE_ADDR          0x40016800                                  /* LTDC memory map base */

#define LTDC_SSCR_ADDR          (LTDC_BASE_ADDR + 0x08)                     /* LTDC Synchronization Size Configuration Register */
#define LTDC_BPCR_ADDR          (LTDC_BASE_ADDR + 0x0c)                     /* LTDC Back Porch Configuration Register */
#define LTDC_AWCR_ADDR          (LTDC_BASE_ADDR + 0x10)                     /* LTDC Active Width Configuration Register */
#define LTDC_TWCR_ADDR          (LTDC_BASE_ADDR + 0x14)                     /* LTDC Total Width Configuration Register */
#define LTDC_GCR_ADDR           (LTDC_BASE_ADDR + 0x18)                     /* LTDC Global Control Register */
#define LTDC_SRCR_ADDR          (LTDC_BASE_ADDR + 0x24)                     /* LTDC Shadow Reload Configuration Register */
#define LTDC_BCCR_ADDR          (LTDC_BASE_ADDR + 0x2c)                     /* LTDC Back Color Configuration Register */

#define LTDC_IER_ADDR           (LTDC_BASE_ADDR + 0x34)                     /* LTDC Interrupt Enable Register(RW) */
#define LTDC_ISR_ADDR           (LTDC_BASE_ADDR + 0x38)                     /* LTDC Interrupt Status Register(RO) */
#define LTDC_ICR_ADDR           (LTDC_BASE_ADDR + 0x3c)                     /* LTDC Interrupt Clear Register(WO) */

/* ldtc layerx registers */
#define LTDC_LCR_ADDR(x)        (LTDC_BASE_ADDR + 0x84 + 0x80 * ((x)-1))    /* LTDC Layerx memory map base */

#define LTDC_LWHPCR_ADDR(x)      (LTDC_LCR_ADDR(x) + 0x04)                  /* LTDC Layerx Window Horizontal Position Configuration Register */
#define LTDC_LWVPCR_ADDR(x)      (LTDC_LCR_ADDR(x) + 0x08)                  /* LTDC Layerx Window Vertical Position Configuration Register */

#define LTDC_LCKCR_ADDR(x)       (LTDC_LCR_ADDR(x) + 0x0c)                  /* LTDC Layerx Color Keying Configuration Register */
#define LTDC_LPFCR_ADDR(x)       (LTDC_LCR_ADDR(x) + 0x10)                  /* LTDC Layerx Pixel Format Configuration Register */
#define LTDC_LCACR_ADDR(x)       (LTDC_LCR_ADDR(x) + 0x14)                  /* LTDC Layerx Constant Alpha Configuration Register */
#define LTDC_LDCCR_ADDR(x)       (LTDC_LCR_ADDR(x) + 0x18)                  /* LTDC Layerx Default Color Configuration Register */
#define LTDC_LBFCR_ADDR(x)       (LTDC_LCR_ADDR(x) + 0x1c)                  /* LTDC Layerx Blending Factors Configuration Register */

#define LTDC_LCFBAR_ADDR(x)      (LTDC_LCR_ADDR(x) + 0x28)                  /* LTDC Layerx Color Frame Buffer Address Register */
#define LTDC_LCFBLR_ADDR(x)      (LTDC_LCR_ADDR(x) + 0x2c)                  /* LTDC Layerx Color Frame Buffer Length Register  */
#define LTDC_LCFBLNR_ADDR(x)     (LTDC_LCR_ADDR(x) + 0x30)                  /* LTDC Layerx Color Frame Buffer Line Number Register */

#define __HAL_RCC_DMA2D_CLK_ENABLE()    do { \
                                        __IO uint32_t tmpreg = 0x00; \
                                        SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA2DEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA2DEN);\
                                        } while(0)

/* 正点原子1024*600 LCD面板参数 */
lcd_panel_param_t lcd_para_default = {
    /* 非可视区（消隐区）参数 */
    .hsw = 4,
    .hbp = 8,
    .hfp = 8,

    .vsh = 4,
    .vbp = 16,
    .vfp = 16,

    /* 可视区参数 */
    .aw = LCD_WIDTH,
    .ah = LCD_HEIGHT,
	
	.bpp = 16	/* bpp改为32的话，LCD控制器会将A值乘到RGB分量上，再送出去，frambuffer需要1024*600*4，如果A设置为0的话那么屏幕什么也不会显示 */
};
 lcd_panel_param_t atk_lcd_para_default = {
    /* 非可视区（消隐区）参数 */
    .hsw = 20,
    .hbp = 140,
    .hfp = 160,

    .vsh = 3,
    .vbp = 20,
    .vfp = 12,

    /* 可视区参数 */
    .aw = LCD_WIDTH,
    .ah = LCD_HEIGHT,
	
	.bpp = 16//16128
};

static void stm32_ltdc_set_bk(unsigned int color)
{
    ipgui_writel(LTDC_BCCR_ADDR, color & 0x00ffffffU);  /* set background color when init */
}

#define SET_PORT_PIN(x, y) dsc.port = x; dsc.pin = y
#define PIN_INIT(n, i)  SET_PORT_PIN(n, i); stm32_gpio_ll_init(&dsc, &ctl)

static void turn_on_lcd_backlight(void)
{
    /* user code begin here */
    int reg;
    stm32_gpio_dsc_t dsc;
    stm32_gpio_ctl_t ctl = {
        .mode = 1, .otype = 0, .pupd = 0, .speed = 3, .af = 0
    };
    reg =  ipgui_readl(RCC_AHB1ENR_ADDR);
    reg |= (1 << 1);
    ipgui_writel(RCC_AHB1ENR_ADDR, reg);

    PIN_INIT(STM32_GPIO_PORT_B, STM32_GPIO_PIN_1);

    reg =  ipgui_readl(GPIOR(dsc.port, 5));             /* read and set odr register */
    reg |= (1 << dsc.pin);
    ipgui_writel(GPIOR(dsc.port, 5), reg);
    /* user code end here */
//	    /* user code begin here */
//    int reg;
//    stm32_gpio_dsc_t dsc;
//    stm32_gpio_ctl_t ctl = {
//        .mode = 1, .otype = 0, .pupd = 1, .speed = 3, .af = 0
//    };
//    reg =  ipgui_readl(RCC_AHB1ENR_ADDR);
//    reg |= (1 << 1);
//    ipgui_writel(RCC_AHB1ENR_ADDR, reg);

//    PIN_INIT(STM32_GPIO_PORT_B, STM32_GPIO_PIN_5);

//    reg =  ipgui_readl(GPIOR(dsc.port, 5));             /* read and set odr register */
//    reg |= (1 << dsc.pin);
//    ipgui_writel(GPIOR(dsc.port, 5), reg);
//    /* user code end here */
}

static void turn_off_lcd_backlight(void)
{
    /* user code begin here */
    int reg;
    stm32_gpio_dsc_t dsc;
    dsc.port = STM32_GPIO_PORT_B; dsc.pin = STM32_GPIO_PIN_1;
    reg =  ipgui_readl(GPIOR(dsc.port, 5));             /* read and reset odr register */
    reg &= ~(1 << dsc.pin);
    ipgui_writel(GPIOR(dsc.port, 5), reg);
    /* user code end here */
}
//360 2 1----45M  //200 2 1----25M
int ltdc_clk_init(int pllsain, int pllsair, int pllsaidivr)
{ 
	int retry = 0;
	int status = 0;
	int tempreg = 0;
	RCC->CR &= ~(1 << 28);
	while (((RCC->CR & (1 << 29))) && (retry < 0x1fff)) retry ++;
 	if (retry == 0x1fff) status=1;
	else
	{
		tempreg |= pllsain << 6;
		tempreg |= pllsair << 28;
		RCC->PLLSAICFGR = tempreg;
		RCC->DCKCFGR &= ~(3 << 16);
		RCC->DCKCFGR |= pllsaidivr << 16;

		RCC->CR |= 1 << 28;
        retry = 0;
		while (((RCC->CR & (1 << 29)) == 0) && (retry < 0x1fff)) retry ++;
		if (retry == 0x1fff) status=2;
 	}
	return status;
}

/* define IRQ channel */
#define STM32F4_IRQ_LTDC        88                      /* fixed, don't change */
#define STM32F4_IRQ_LTDC_ERR    89                      /* fixed, don't change */

/* define ltdc interrupt masks */
#define LINE_MASK               1U                      /* fixed, don't change */
#define FIFO_UNDERRUN_MASK      2U                      /* fixed, don't change */
#define TRANSFER_ERR_MASK       4U                      /* fixed, don't change */
#define REG_RELOAD_MASK         8U                      /* fixed, don't change */

static void ltdc_irq_init(void)
{
    /* user code begin here */
    ipgui_writel(LTDC_IER_ADDR, LINE_MASK);             /* enable ltdc irq */
    /* user code end here */

    ipgui_NVIC_irq_prio_set(STM32F4_IRQ_LTDC, 255);
    ipgui_NVIC_enable_irq(STM32F4_IRQ_LTDC);
}
#include "lv_hal_disp.h"
extern lv_disp_drv_t disp_drv; 
/* irq callback function */
/* 在中断中，LCD控制器不会停止运行，仍然继续读framebuffer并显示 */
void LTDC_IRQHandler(void)
{
    int irq_status = ipgui_readl(LTDC_ISR_ADDR);
    /* check which irq is active */
    if (irq_status & 0x01) {                            /* line interrupt */
        /* 通过示波器测量LED翻转次数，来计算每秒进了几次中断，测量后每秒大约进入了52.63次中断
         * 而LCD物理帧数是45Mhz/853440 ≈ 52.72fps，跟测量值吻合
         * 853440（个pixel clock） = (vsh + vbp + ah + vfp) * (hsw + hbp + aw + hfp)
         * 此中断是在VSYNC的第一行触发（因为中断行位置寄存器被设置为0）
         * 比Reload中断（VFP触发）晚（aw + hsw + hbp + aw + hfp）* vfp * （1/45Mhz）约360us，已通过示波器观察验证
         */
        if (back_state == 1) {/* lvgl ready */
            if(!dma_busy) {
                dma_busy = 1;
                LTDC_Color_Fill(x1, y1, x2, y2, color);
            }
            //    // LED1 = !LED1;
            // ltdc_layer1_reload_framebuffer_imediately((void *)back_lcd_framebuffer);
            // /* 交换前后framebuffer */
            // {
            //     unsigned short * temp;
            //     temp = fore_lcd_framebuffer;
            //     fore_lcd_framebuffer = back_lcd_framebuffer;
            //     back_lcd_framebuffer = temp;
            // }
            // back_state = 0;
            // lv_disp_flush_ready(&disp_drv);
        }
        if (back_render_ready == 1) {
            ltdc_layer1_reload_framebuffer_imediately((void *)back_lcd_framebuffer);
            /* 交换前后framebuffer */
            unsigned short * temp;
            temp = fore_lcd_framebuffer;
            fore_lcd_framebuffer = back_lcd_framebuffer;
            back_lcd_framebuffer = temp;
            back_state = 0;
            back_render_ready = 0;
                dma_busy = 0;
            lv_disp_flush_ready(&disp_drv);
        }
    }
    if (irq_status & 0x02) {                            /* fifo underrun interrupt */
        
    }
    if (irq_status & 0x04) {                            /* transform error interrupt */
        
    }
    if (irq_status & 0x08) {                            /* register reload interrupt */
        /* 实验现象表明此中断比行中断（行设置为0，VSYNC时触发）早（aw + hsw + hbp + aw + hfp）* vfp * （1/45Mhz）约360us */
        // LED0 = !LED0;        
    }
    ipgui_writel(LTDC_ICR_ADDR, irq_status);            /* clear irq status */
}

void stm32_ltdc_ll_init(unsigned int pix_clk, lcd_panel_param_t * pp, void * fb)
{																																										
    int reg = 0, reg1 = 0;
    stm32_gpio_dsc_t dsc;
    stm32_gpio_ctl_t ctl;
    ctl.mode = 2;
    ctl.otype = 0;
    ctl.pupd = 1;
    ctl.speed = 3;
    ctl.af = 14;
    /* configure sys clk */                     
	if(pix_clk == 25) ltdc_clk_init(200, 2, 1);/* 设置像素时钟25Mhz */
    else if(pix_clk == 45) ltdc_clk_init(360, 2, 1);                           /* 设置像素时钟45Mhz */
    /* configure lcd gpio */
    /* 下面的引脚对应关系不是唯一的，下面只列出了STM32F429IGT6的（部分）引脚，对于不同的硬件需要修改引脚 */
    /* R0-PH2   G0-PE5   B0-PE4  */         /* DE-PF10 */
    /* R1-PH3   G1-PE6   B1-PG12 */         /* CLK-PG7 */
    /* R2-PH8   G2-PH13  B2-PG10  */        /* VSYNC-PI9 */    
    /* R3-PH9   G3-PH14  B3-PG11 */         /* HSYNC-PI10  */
    /* R4-PH10  G4-PH15  B4-PI4  */
    /* R5-PH11  G5-PI0   B5-PI5  */
    /* R6-PH12  G6-PI1   B6-PI6  */
    /* R7-PG6   G7-PI2   B7-PI7  */
    reg =  ipgui_readl(RCC_AHB1ENR_ADDR);               /* enable gpioE/F/G/H/I clock first */
    reg |= 0x1f0U;
    ipgui_writel(RCC_AHB1ENR_ADDR, reg);

    if ((pp->bpp == 24) || (pp->bpp == 32)) {
    PIN_INIT(STM32_GPIO_PORT_H, STM32_GPIO_PIN_2 );     /* configure LCD R0 - R7 */
    PIN_INIT(STM32_GPIO_PORT_H, STM32_GPIO_PIN_3 );
    PIN_INIT(STM32_GPIO_PORT_H, STM32_GPIO_PIN_8 );
    }
    PIN_INIT(STM32_GPIO_PORT_H, STM32_GPIO_PIN_9 );
    PIN_INIT(STM32_GPIO_PORT_H, STM32_GPIO_PIN_10);
    PIN_INIT(STM32_GPIO_PORT_H, STM32_GPIO_PIN_11);
    PIN_INIT(STM32_GPIO_PORT_H, STM32_GPIO_PIN_12);
    PIN_INIT(STM32_GPIO_PORT_G, STM32_GPIO_PIN_6 );

    if ((pp->bpp == 24) || (pp->bpp == 32)) {
    PIN_INIT(STM32_GPIO_PORT_E, STM32_GPIO_PIN_5 );     /* configure LCD G0 - G7 */
    PIN_INIT(STM32_GPIO_PORT_E, STM32_GPIO_PIN_6 );
    }
    PIN_INIT(STM32_GPIO_PORT_H, STM32_GPIO_PIN_13);
    PIN_INIT(STM32_GPIO_PORT_H, STM32_GPIO_PIN_14);
    PIN_INIT(STM32_GPIO_PORT_H, STM32_GPIO_PIN_15);
    PIN_INIT(STM32_GPIO_PORT_I, STM32_GPIO_PIN_0 );
    PIN_INIT(STM32_GPIO_PORT_I, STM32_GPIO_PIN_1 );
    PIN_INIT(STM32_GPIO_PORT_I, STM32_GPIO_PIN_2 );

    if ((pp->bpp == 24) || (pp->bpp == 32)) {
    PIN_INIT(STM32_GPIO_PORT_E, STM32_GPIO_PIN_4 );     /* configure LCD B0 - B7 */
    PIN_INIT(STM32_GPIO_PORT_G, STM32_GPIO_PIN_12);
    PIN_INIT(STM32_GPIO_PORT_G, STM32_GPIO_PIN_10);
    }
    PIN_INIT(STM32_GPIO_PORT_G, STM32_GPIO_PIN_11);
    PIN_INIT(STM32_GPIO_PORT_I, STM32_GPIO_PIN_4 );
    PIN_INIT(STM32_GPIO_PORT_I, STM32_GPIO_PIN_5 );
    PIN_INIT(STM32_GPIO_PORT_I, STM32_GPIO_PIN_6 );
    PIN_INIT(STM32_GPIO_PORT_I, STM32_GPIO_PIN_7 );

    PIN_INIT(STM32_GPIO_PORT_F, STM32_GPIO_PIN_10);     /* configure LCD DE */
    PIN_INIT(STM32_GPIO_PORT_G, STM32_GPIO_PIN_7 );     /* configure LCD CLK */
    PIN_INIT(STM32_GPIO_PORT_I, STM32_GPIO_PIN_9 );     /* configure LCD VSYNC */
    PIN_INIT(STM32_GPIO_PORT_I, STM32_GPIO_PIN_10);     /* configure LCD HSYNC */

    /* configure lcd clock */
    reg =  ipgui_readl(RCC_APB2ENR_ADDR);
    reg |= 1 << 26;
    ipgui_writel(RCC_APB2ENR_ADDR, reg);                /* enable ltdc clock */

    /* configure synchronous signals and clock polarity  */
	reg =  0 << 28;                                     /* Pixel Clock Polarity */
	reg |= 0 << 29;                                     /* Data Enable polarity is active low */
	reg |= 0 << 30;                                     /* Vertical synchronization polarity is active low */
	reg |= 0 << 31;                                     /* Horizontal synchronization polarity is active low */
    ipgui_writel(LTDC_GCR_ADDR, reg);

    /* configure synchronous timings */
    reg =  pp->vsh - 1;
    reg |= (pp->hsw - 1) << 16;
    ipgui_writel(LTDC_SSCR_ADDR, reg);

    reg =  pp->vsh + pp->vbp - 1;
    reg |= (pp->hsw + pp->hbp - 1) << 16;
    ipgui_writel(LTDC_BPCR_ADDR, reg);

    reg =  pp->vsh + pp->vbp + pp->ah - 1;
    reg |= (pp->hsw + pp->hbp + pp->aw - 1) << 16;
    ipgui_writel(LTDC_AWCR_ADDR, reg);

    reg =  pp->vsh + pp->vbp + pp->ah + pp->vfp - 1;
    reg |= (pp->hsw + pp->hbp + pp->aw + pp->hfp - 1) << 16;
    ipgui_writel(LTDC_TWCR_ADDR, reg);

    /* set background color if needed */
    stm32_ltdc_set_bk(0xffffffff);

    /* configure layer1 and layer2, if two layers are enabled, 
     * the layer2 is the top displayed window, we only need to set layer1
     */
    reg =  ipgui_readl(LTDC_BPCR_ADDR);                 /* set layer1 window size */
    reg1=  ipgui_readl(LTDC_AWCR_ADDR);
    reg =  (((reg & 0x0fff0000U) >> 16) + 1);
    reg |= (reg1 & 0x0fff0000U);
    ipgui_writel(LTDC_LWHPCR_ADDR(1), reg);

    reg = ipgui_readl(LTDC_BPCR_ADDR);
    reg1= ipgui_readl(LTDC_AWCR_ADDR);
    reg = (reg & 0x7ffU) + 1;
    reg |= ((reg1 & 0x7ffU) << 16);
    ipgui_writel(LTDC_LWVPCR_ADDR(1), reg);

    ipgui_writel(LTDC_LCACR_ADDR(1), 255U);             /* set layer1 alpha 255, it means background color is all covered, we don't need backgroud, everything will be controled by software or GUI framework!! */

    ipgui_writel(LTDC_LDCCR_ADDR(1), 0x0U);             /* don't need default color, this is a dog's shit register */

    ipgui_writel(LTDC_LBFCR_ADDR(1), 0x00000607U);      /* don't need hardware blending color, set defualt*/

	if (pp->bpp == 32) {
		reg = 0;
	} else if(pp->bpp == 24) {                          /* set layer1 pixel format */
        reg = 1;
    } else if (pp->bpp == 16) {
		reg = 2;
	}
    ipgui_writel(LTDC_LPFCR_ADDR(1), reg);

    ipgui_writel(LTDC_LCFBAR_ADDR(1), (int)fb);         /* set framebuffer addr */

    reg = (pp->bpp / 8) * pp->aw;
    reg = (reg << 16) | (reg + 3);
    ipgui_writel(LTDC_LCFBLR_ADDR(1), reg);             /* set bytes per line */

    ipgui_writel(LTDC_LCFBLNR_ADDR(1), pp->ah);         /* set color framebuffer line number, corresponding to active height */

    ipgui_writel(LTDC_LCR_ADDR(1), 0x1U);               /* enable layer1 */

    reg = ipgui_readl(LTDC_SRCR_ADDR);
    reg |= 1U << 0;
    ipgui_writel(LTDC_SRCR_ADDR, reg);                  /* reload layer1/2 immediately, must do it! */

    /* configure interruptes if needed */
    ltdc_irq_init();

    /* enable ltdc */
    reg = ipgui_readl(LTDC_GCR_ADDR);
    reg |= 1 << 0;
    ipgui_writel(LTDC_GCR_ADDR, reg);

    /* turn on LCD backlight if needed  */
    turn_on_lcd_backlight();
	
	fore_lcd_framebuffer = ltdc_lcd_framebuf1;
    back_lcd_framebuffer = ltdc_lcd_framebuf;
    back_state = 0;
	
	__HAL_RCC_DMA2D_CLK_ENABLE();	//使能DM2D时钟
	dma2d_irq_init();
}

#define STM32_DMA2D_IRQn 90
void dma2d_irq_init(void)
{
	/* enable ltdc irq */
    ipgui_NVIC_irq_prio_set(STM32_DMA2D_IRQn, 5);
    ipgui_NVIC_enable_irq(STM32_DMA2D_IRQn);
}

/* reload layer1 framebuffer address imediately */
void ltdc_layer1_reload_framebuffer_imediately(void * fb)
{
    ipgui_writel(LTDC_LCFBAR_ADDR(1), (int)fb);         /* set framebuffer addr */
    ipgui_writel(LTDC_SRCR_ADDR, 0x1U);                 /* reload layer1 */
}
 
/* 在垂直消隐区的第一行更改framebuffer，也就是VFP的第一行
 * 如果调用了这个函数，且又开启了reload中断，那么到下一个垂直消隐区的第一行（VFP）时就会触发中断
 * 每次调用只触发一次Reload中断（如果Reload中断开启）
 * 如果调用了此函数后再且在reload中断触发前又调用了ltdc_layer1_reload_framebuffer_imediately，那么reload中断不会响应
 */
/* reload layer1 framebuffer address at first line of vertical blanking */
void ltdc_layer1_reload_framebuffer_blanking(void * fb)
{
    ipgui_writel(LTDC_LCFBAR_ADDR(1), (int)fb);         /* set framebuffer addr */
    ipgui_writel(LTDC_SRCR_ADDR, 0x2U);                 /* reload layer1 later */
}

/* 在读指针（硬件自动）写指针（程序写framebuffer）同时对同一个物理内存SDRAM framebuffer操作时，会导致屏幕出现大片雪花，应避免这个问题 */

/* 下面是bpp为32和24时写framebuffer的例子 */
void sw_fill_pixel(int x, int y, unsigned int color)//这里传入的color带Alpha通道
{
//	    /* 舍弃color中的A */    //写法2
//    ltdc_lcd_framebuf[LCD_WIDTH*y+x] = color | 0xff000000;
	ltdc_lcd_framebuf[LCD_WIDTH*y+x] = (unsigned short)color;
#if 0
    //32bit

    /* 舍弃color中的A */    //写法1
	*(u8*)(ltdc_framebuf[0]+4*(1024*y+x)) = color & 0xff;//B
	*(u8*)(ltdc_framebuf[0]+4*(1024*y+x)+1) = (color & 0xff00) >> 8;//G
	*(u8*)(ltdc_framebuf[0]+4*(1024*y+x)+2) = (color & 0xff0000) >> 16;//R
	*(u8*)(ltdc_framebuf[0]+4*(1024*y+x)+3) = 200;/* A分量会与上面的RGB相乘 */
    /* 舍弃color中的A */    //写法2
    *(u32*)(ltdc_framebuf[0]+4*(1024*y+x)) = color | 0xff000000;

    /* 保留color中的A */    //写法
    *(u32*)(ltdc_framebuf[0]+4*(1024*y+x)) = color;

    //24bit
    *(u8*)(ltdc_framebuf[0]+3*(1024*y+x)) = color & 0xff;//B
	*(u8*)(ltdc_framebuf[0]+3*(1024*y+x)+1) = (color & 0xff00) >> 8;//G
	*(u8*)(ltdc_framebuf[0]+3*(1024*y+x)+2) = (color & 0xff0000) >> 16;//R
#endif
}

void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u32 color)
{ 
//	u32 psx,psy,pex,pey;	//以LCD面板为基准的坐标系,不随横竖屏变化而变化
//	u32 timeout=0; 
//	u16 offline;
//	u32 addr; 
//	//坐标系转换
//	if(1)	//横屏
//	{
//		psx=sx;psy=sy;
//		pex=ex;pey=ey;
//	}
//	offline=LCD_WIDTH-(pex-psx+1);
//	addr=((u32)ltdc_lcd_framebuf+4*(LCD_WIDTH*psy+psx));
//	__HAL_RCC_DMA2D_CLK_ENABLE();//使能DM2D时钟
//	DMA2D->CR&=~(DMA2D_CR_START);	//先停止DMA2D
//	DMA2D->CR=((uint32_t)0x00030000);			//寄存器到存储器模式
//	DMA2D->OPFCCR=0;	//设置颜色格式ARGB8888
//	DMA2D->OOR=offline;				//设置行偏移 

//	DMA2D->OMAR=addr;				//输出存储器地址
//	DMA2D->NLR=(pey-psy+1)|((pex-psx+1)<<16);	//设定行数寄存器
//	DMA2D->OCOLR=color | 0xff000000;						//设定输出颜色寄存器 
//	DMA2D->CR|=DMA2D_CR_START;				//启动DMA2D
//	while((DMA2D->ISR&(((uint32_t)0x00000002)))==0)	//等待传输完成
//	{
//		timeout++;
//		if(timeout>0X1FFFFF)break;	//超时退出
//	} 
//	DMA2D->IFCR|=((uint32_t)0x00000002);		//清除传输完成标志 		
    for (int x = sx; x < ex; x++)
    {
        for (int y = sy; y < ey; y++)
        {
            sw_fill_pixel(x, y, color);
        }
    }
}

void DMA2D_IRQHandler(void)
{
    int isr = DMA2D->ISR;

    if (isr & DMA2D_ISR_TCIF)   /* transfer complete interrupt */
    {
        DMA2D->CR &= ~DMA2D_CR_TCIE;        //传输完成中断失能
        DMA2D->IFCR |= ((uint32_t)DMA2D_ISR_TCIF);				//清除传输完成中断标志  	

        back_render_ready = 1;
    }

}

void LTDC_Color_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 *color)
{
	u32 psx, psy, pex, pey;	//以LCD面板为基准的坐标系,不随横竖屏变化而变化
	u16 offline;
	u32 addr; 

	psx = sx; psy = sy;
	pex = ex; pey = ey;

	offline = LCD_WIDTH - (pex - psx + 1);
	addr = (u32)(((u32)back_lcd_framebuffer) + 2 * (LCD_WIDTH * psy + psx));

	DMA2D->CR &= ~(DMA2D_CR_START);	//先停止DMA2D
	DMA2D->CR = ((uint32_t)0x00000000);			//存储器到存储器模式
	DMA2D->FGPFCCR = 0X02;	//设置颜色格式
	DMA2D->FGOR = 0;					//前景层行偏移为0
	DMA2D->OOR = offline;				//设置行偏移 

	DMA2D->FGMAR = (u32)color;		//源地址
	DMA2D->OMAR = addr;				//输出存储器地址
	DMA2D->NLR = (pey - psy + 1) | ((pex - psx + 1) << 16);	//设定行数寄存器 

    DMA2D->CR |= DMA2D_CR_TCIE;    //传输完成中断使能
	DMA2D->CR |= DMA2D_CR_START;					//启动DMA2D
    // u32 timeout = 0; 
	// while((DMA2D->ISR&(((uint32_t)0x00000002)))==0)		//等待传输完成
	// {
	// 	timeout++;
	// 	if(timeout>0X1FFFFF)break;	//超时退出
	// } 
	// DMA2D->IFCR|=((uint32_t)0x00000002);				//清除传输完成标志  	
}  