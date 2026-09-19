/**
  ******************************************************************************
  * @file    bsp_ili9341_lcd.c
  * @version V1.0
  * @date    2013-xx-xx
  * @brief   触摸屏驱动
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火 F103-MINI STM32 开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */ 
#include "stm32f4xx_gpio.h"

unsigned char LCD_SCAN_MODE = 0;
#include "bsp_xpt2046_lcd.h"
//#include "./lcd/bsp_ili9341_lcd.h"
//#include "./led/bsp_led.h"
//#include "./font/fonts.h"
//#include "./flash/bsp_spi_flash.h"
//#include "palette.h"
#include <stdio.h> 
#include <string.h>
#include "led.h"
strType_XPT2046_Coordinate cinfo={-1,-1,-1,-1,0};
/******************************* 声明 XPT2046 相关的静态函数 ***************************/
static void                   XPT2046_DelayUS                       ( __IO uint32_t ulCount );
static void                   XPT2046_WriteCMD                      ( uint8_t ucCmd );
static uint16_t               XPT2046_ReadCMD                       ( void );
static uint16_t               XPT2046_ReadAdc                       ( uint8_t ucChannel );
static void                   XPT2046_ReadAdc_XY                    ( int16_t * sX_Ad, int16_t * sY_Ad );
static uint8_t                XPT2046_ReadAdc_Smooth_XY             ( strType_XPT2046_Coordinate * pScreenCoordinate );
static uint8_t                XPT2046_Calculate_CalibrationFactor   ( strType_XPT2046_Coordinate * pDisplayCoordinate, strType_XPT2046_Coordinate * pScreenSample, strType_XPT2046_Calibration * pCalibrationFactor );
static void                   ILI9341_DrawCross                     ( uint16_t usX, uint16_t usY );



/******************************* 定义 XPT2046 全局变量 ***************************/
//默认触摸参数，不同的屏幕稍有差异，可重新调用触摸校准函数获取
strType_XPT2046_TouchPara strXPT2046_TouchPara[] = { 	
 -0.006464,   -0.073259,  280.358032,    0.074878,    0.002052,   -6.545977,//扫描方式0
	0.086314,    0.001891,  -12.836658,   -0.003722,   -0.065799,  254.715714,//扫描方式1
	0.002782,    0.061522,  -11.595689,    0.083393,    0.005159,  -15.650089,//扫描方式2
	0.089743,   -0.000289,  -20.612209,   -0.001374,    0.064451,  -16.054003,//扫描方式3
	0.000767,   -0.068258,  250.891769,   -0.085559,   -0.000195,  334.747650,//扫描方式4
 -0.084744,    0.000047,  323.163147,   -0.002109,   -0.066371,  260.985809,//扫描方式5
 -0.001848,    0.066984,  -12.807136,   -0.084858,   -0.000805,  333.395386,//扫描方式6
 -0.085470,   -0.000876,  334.023163,   -0.003390,    0.064725,   -6.211169,//扫描方式7
};

volatile uint8_t ucXPT2046_TouchFlag = 0;



/**
  * @brief  XPT2046 初始化函数
  * @param  无
  * @retval 无
  */	


void XPT2046_Init ( void )
{
	stm32_gpio_ctl_t xpt2046_pin_ctl = {
		.mode  = GPIO_MODE_AF,
		.otype = GPIO_OTYPE_PP,
		.speed = GPIO_SPEED_VERY_HIGH,
		.pupd  = GPIO_PUPD_UP,
		.af    = 0,                 /* not set yet, set it when init */
	};
	RCC->AHB1ENR|=1<<1;//使能PORTB时钟 
	RCC->AHB1ENR|=1<<4;//使能PORTE时钟 


	GPIO_Set(GPIOB,PIN6,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE); //CS
	GPIO_Set(GPIOB,PIN7,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE); //CLK
	GPIO_Set(GPIOB,PIN9,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_NONE); //MOSI
	GPIO_Set(GPIOB,PIN8,GPIO_MODE_INPUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PU); //MISO
	
	GPIO_Set(GPIOE,PIN4,GPIO_MODE_INPUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PU); //PEN

	XPT2046_CS = 0;
}



/**
  * @brief  用于 XPT2046 的简单微秒级延时函数
  * @param  nCount ：延时计数值，单位为微妙
  * @retval 无
  */	
static void XPT2046_DelayUS ( __IO uint32_t ulCount )
{
	uint32_t i;


	for ( i = 0; i < ulCount; i ++ )
	{
		uint8_t uc = 12;     //设置值为12，大约延1微秒  
	      
		while ( uc -- );     //延1微秒	

	}
	
}



/**
  * @brief  XPT2046 的写入命令
  * @param  ucCmd ：命令
  *   该参数为以下值之一：
  *     @arg 0x90 :通道Y+的选择控制字
  *     @arg 0xd0 :通道X+的选择控制字
  * @retval 无
  */
static void XPT2046_WriteCMD ( uint8_t ucCmd ) 
{
	uint8_t i;


	XPT2046_MOSI_0();
	
	XPT2046_CLK_LOW();

	for ( i = 0; i < 8; i ++ ) 
	{
		( ( ucCmd >> ( 7 - i ) ) & 0x01 ) ? XPT2046_MOSI_1() : XPT2046_MOSI_0();
		
	  XPT2046_DelayUS ( 5 );
		
		XPT2046_CLK_HIGH();

	  XPT2046_DelayUS ( 5 );

		XPT2046_CLK_LOW();
	}
	
}


/**
  * @brief  XPT2046 的读取命令
  * @param  无
  * @retval 读取到的数据
  */
static uint16_t XPT2046_ReadCMD ( void ) 
{
	uint8_t i;
	uint16_t usBuf=0, usTemp;
	


	XPT2046_MOSI_0();

	XPT2046_CLK_HIGH();

	for ( i=0;i<12;i++ ) 
	{
		XPT2046_CLK_LOW();    
	
		usTemp = XPT2046_MISO();
		
		usBuf |= usTemp << ( 11 - i );
	
		XPT2046_CLK_HIGH();
		
	}
	
	return usBuf;

}


/**
  * @brief  对 XPT2046 选择一个模拟通道后，启动ADC，并返回ADC采样结果
  * @param  ucChannel
  *   该参数为以下值之一：
  *     @arg 0x90 :通道Y+的选择控制字
  *     @arg 0xd0 :通道X+的选择控制字
  * @retval 该通道的ADC采样结果
  */
static uint16_t XPT2046_ReadAdc ( uint8_t ucChannel )
{
	XPT2046_WriteCMD ( ucChannel );

  return 	XPT2046_ReadCMD ();
	
}


/**
  * @brief  读取 XPT2046 的X通道和Y通道的AD值（12 bit，最大是4096）
  * @param  sX_Ad ：存放X通道AD值的地址
  * @param  sY_Ad ：存放Y通道AD值的地址
  * @retval 无
  */
static void XPT2046_ReadAdc_XY ( int16_t * sX_Ad, int16_t * sY_Ad )  
{ 
	int16_t sX_Ad_Temp, sY_Ad_Temp; 
	
	sX_Ad_Temp = XPT2046_ReadAdc ( XPT2046_CHANNEL_X );

	XPT2046_DelayUS ( 1 ); 

	sY_Ad_Temp = XPT2046_ReadAdc ( XPT2046_CHANNEL_Y ); 
	
	
	* sX_Ad = sX_Ad_Temp; 
	* sY_Ad = sY_Ad_Temp; 
	
	
}

static uint8_t XPT2046_ReadAdc_Smooth_XY ( strType_XPT2046_Coordinate * pScreenCoordinate )
{
	uint8_t ucCount = 0, i;
	
	int16_t sAD_X, sAD_Y;
	int16_t sBufferArray [ 2 ] [ 10 ] = { { 0 },{ 0 } };  //坐标X和Y进行多次采样
	
	//存储采样中的最小值、最大值
	int32_t lX_Min, lX_Max, lY_Min, lY_Max;


	/* 循环采样10次 */ 
	do					       				
	{		  
		XPT2046_ReadAdc_XY ( & sAD_X, & sAD_Y );  
		
		sBufferArray [ 0 ] [ ucCount ] = sAD_X;  
		sBufferArray [ 1 ] [ ucCount ] = sAD_Y;
		
		ucCount ++;  
		
	}	while ( ( XPT2046_PENIRQ_Read() == XPT2046_PENIRQ_ActiveLevel ) && ( ucCount < 5 ) );//用户点击触摸屏时即TP_INT_IN信号为低 并且 ucCount<10
	
	
	/*如果触笔弹起*/
	if ( XPT2046_PENIRQ_Read() != XPT2046_PENIRQ_ActiveLevel )
		ucXPT2046_TouchFlag = 0;			//中断标志复位

	
	/*如果成功采样10个样本*/
	if ( ucCount == 5 )		 					
	{
//		lX_Max = lX_Min = sBufferArray [ 0 ] [ 0 ];
//		lY_Max = lY_Min = sBufferArray [ 1 ] [ 0 ];       
//		
//		for ( i = 1; i < 10; i ++ )
//		{
//			if ( sBufferArray[ 0 ] [ i ] < lX_Min )
//				lX_Min = sBufferArray [ 0 ] [ i ];
//			
//			else if ( sBufferArray [ 0 ] [ i ] > lX_Max )
//				lX_Max = sBufferArray [ 0 ] [ i ];

//		}
//		
//		for ( i = 1; i < 10; i ++ )
//		{
//			if ( sBufferArray [ 1 ] [ i ] < lY_Min )
//				lY_Min = sBufferArray [ 1 ] [ i ];
//			
//			else if ( sBufferArray [ 1 ] [ i ] > lY_Max )
//				lY_Max = sBufferArray [ 1 ] [ i ];

//		}
//		
//		
//		/*去除最小值和最大值之后求平均值*/
//		pScreenCoordinate ->x =  ( sBufferArray [ 0 ] [ 0 ] + sBufferArray [ 0 ] [ 1 ] + sBufferArray [ 0 ] [ 2 ] + sBufferArray [ 0 ] [ 3 ] + sBufferArray [ 0 ] [ 4 ] + 
//		                           sBufferArray [ 0 ] [ 5 ] + sBufferArray [ 0 ] [ 6 ] + sBufferArray [ 0 ] [ 7 ] + sBufferArray [ 0 ] [ 8 ] + sBufferArray [ 0 ] [ 9 ] - lX_Min-lX_Max ) >> 3;
//		
//		pScreenCoordinate ->y =  ( sBufferArray [ 1 ] [ 0 ] + sBufferArray [ 1 ] [ 1 ] + sBufferArray [ 1 ] [ 2 ] + sBufferArray [ 1 ] [ 3 ] + sBufferArray [ 1 ] [ 4 ] + 
//		                           sBufferArray [ 1 ] [ 5 ] + sBufferArray [ 1 ] [ 6 ] + sBufferArray [ 1 ] [ 7 ] + sBufferArray [ 1 ] [ 8 ] + sBufferArray [ 1 ] [ 9 ] - lY_Min-lY_Max ) >> 3; 
		

		cinfo.x = (int16_t)(800 - (sAD_Y - 120) * 800 / 3880 );
		cinfo.y = (int16_t)(480 - (sAD_X - 160) * 480 / 3640 );
		return 1;		
		
	}   	
	return 0;    	
}

/**
  * @brief  计算 XPT2046 触摸坐标校正系数（注意：只有在LCD和触摸屏间的误差角度非常小时,才能运用下面公式）
  * @param  pDisplayCoordinate ：屏幕人为显示的已知坐标
  * @param  pstrScreenSample ：对已知坐标点触摸时 XPT2046 产生的坐标
  * @param  pCalibrationFactor ：根据人为设定坐标和采样回来的坐标计算出来的屏幕触摸校正系数
  * @retval 计算状态
	*   该返回值为以下值之一：
  *     @arg 1 :计算成功
  *     @arg 0 :计算失败
  */
static uint8_t XPT2046_Calculate_CalibrationFactor ( strType_XPT2046_Coordinate * pDisplayCoordinate, strType_XPT2046_Coordinate * pScreenSample, strType_XPT2046_Calibration * pCalibrationFactor )
{
	uint8_t ucRet = 1;

	
	/* K＝ ( X0－X2 )  ( Y1－Y2 )－ ( X1－X2 )  ( Y0－Y2 ) */
	pCalibrationFactor -> Divider =  ( ( pScreenSample [ 0 ] .x - pScreenSample [ 2 ] .x ) *  ( pScreenSample [ 1 ] .y - pScreenSample [ 2 ] .y ) ) - 
									                 ( ( pScreenSample [ 1 ] .x - pScreenSample [ 2 ] .x ) *  ( pScreenSample [ 0 ] .y - pScreenSample [ 2 ] .y ) ) ;
	
	
	if (  pCalibrationFactor -> Divider == 0  )
		ucRet = 0;

	else
	{
		/* A＝ (  ( XD0－XD2 )  ( Y1－Y2 )－ ( XD1－XD2 )  ( Y0－Y2 ) )／K	*/
		pCalibrationFactor -> An =  ( ( pDisplayCoordinate [ 0 ] .x - pDisplayCoordinate [ 2 ] .x ) *  ( pScreenSample [ 1 ] .y - pScreenSample [ 2 ] .y ) ) - 
								                ( ( pDisplayCoordinate [ 1 ] .x - pDisplayCoordinate [ 2 ] .x ) *  ( pScreenSample [ 0 ] .y - pScreenSample [ 2 ] .y ) );
		
		/* B＝ (  ( X0－X2 )  ( XD1－XD2 )－ ( XD0－XD2 )  ( X1－X2 ) )／K	*/
		pCalibrationFactor -> Bn =  ( ( pScreenSample [ 0 ] .x - pScreenSample [ 2 ] .x ) *  ( pDisplayCoordinate [ 1 ] .x - pDisplayCoordinate [ 2 ] .x ) ) - 
								                ( ( pDisplayCoordinate [ 0 ] .x - pDisplayCoordinate [ 2 ] .x ) *  ( pScreenSample [ 1 ] .x - pScreenSample [ 2 ] .x ) );
		
		/* C＝ ( Y0 ( X2XD1－X1XD2 )+Y1 ( X0XD2－X2XD0 )+Y2 ( X1XD0－X0XD1 ) )／K */
		pCalibrationFactor -> Cn =  ( pScreenSample [ 2 ] .x * pDisplayCoordinate [ 1 ] .x - pScreenSample [ 1 ] .x * pDisplayCoordinate [ 2 ] .x ) * pScreenSample [ 0 ] .y +
								                ( pScreenSample [ 0 ] .x * pDisplayCoordinate [ 2 ] .x - pScreenSample [ 2 ] .x * pDisplayCoordinate [ 0 ] .x ) * pScreenSample [ 1 ] .y +
								                ( pScreenSample [ 1 ] .x * pDisplayCoordinate [ 0 ] .x - pScreenSample [ 0 ] .x * pDisplayCoordinate [ 1 ] .x ) * pScreenSample [ 2 ] .y ;
		
		/* D＝ (  ( YD0－YD2 )  ( Y1－Y2 )－ ( YD1－YD2 )  ( Y0－Y2 ) )／K	*/
		pCalibrationFactor -> Dn =  ( ( pDisplayCoordinate [ 0 ] .y - pDisplayCoordinate [ 2 ] .y ) *  ( pScreenSample [ 1 ] .y - pScreenSample [ 2 ] .y ) ) - 
								                ( ( pDisplayCoordinate [ 1 ] .y - pDisplayCoordinate [ 2 ] .y ) *  ( pScreenSample [ 0 ] .y - pScreenSample [ 2 ] .y ) ) ;
		
		/* E＝ (  ( X0－X2 )  ( YD1－YD2 )－ ( YD0－YD2 )  ( X1－X2 ) )／K	*/
		pCalibrationFactor -> En =  ( ( pScreenSample [ 0 ] .x - pScreenSample [ 2 ] .x ) *  ( pDisplayCoordinate [ 1 ] .y - pDisplayCoordinate [ 2 ] .y ) ) - 
								                ( ( pDisplayCoordinate [ 0 ] .y - pDisplayCoordinate [ 2 ] .y ) *  ( pScreenSample [ 1 ] .x - pScreenSample [ 2 ] .x ) ) ;
		
		
		/* F＝ ( Y0 ( X2YD1－X1YD2 )+Y1 ( X0YD2－X2YD0 )+Y2 ( X1YD0－X0YD1 ) )／K */
		pCalibrationFactor -> Fn =  ( pScreenSample [ 2 ] .x * pDisplayCoordinate [ 1 ] .y - pScreenSample [ 1 ] .x * pDisplayCoordinate [ 2 ] .y ) * pScreenSample [ 0 ] .y +
								                ( pScreenSample [ 0 ] .x * pDisplayCoordinate [ 2 ] .y - pScreenSample [ 2 ] .x * pDisplayCoordinate [ 0 ] .y ) * pScreenSample [ 1 ] .y +
								                ( pScreenSample [ 1 ] .x * pDisplayCoordinate [ 0 ] .y - pScreenSample [ 0 ] .x * pDisplayCoordinate [ 1 ] .y ) * pScreenSample [ 2 ] .y;
			
	}
	
	
	return ucRet;
	
	
}


/**
  * @brief  XPT2046 触摸屏校准
	* @param	LCD_Mode：指定要校正哪种液晶扫描模式的参数
  * @note  本函数调用后会把液晶模式设置为LCD_Mode
  * @retval 校准结果
	*   该返回值为以下值之一：
  *     @arg 1 :校准成功
  *     @arg 0 :校准失败
  */
	int ret = 0;
uint8_t XPT2046_Touch_Calibrate ( uint8_t LCD_Mode ) 
{

		uint8_t i;
		
		char cStr [ 100 ];
		
		uint16_t usTest_x = 0, usTest_y = 0, usGap_x = 0, usGap_y = 0;
		
	  char * pStr = 0;
	
    strType_XPT2046_Coordinate strCrossCoordinate[4], strScreenSample[4];
	  
	  strType_XPT2046_Calibration CalibrationFactor;
		
		
		/* 设定“十”字交叉点的坐标 */ 
		strCrossCoordinate [0].x = 800 >> 2;
		strCrossCoordinate[0].y = 480 >> 2;
		
		strCrossCoordinate[1].x = strCrossCoordinate[0].x;
		strCrossCoordinate[1].y = ( 480 * 3 ) >> 2;
		
		strCrossCoordinate[2].x = ( 800 * 3 ) >> 2;
		strCrossCoordinate[2].y = strCrossCoordinate[1].y;
		
		strCrossCoordinate[3].x = strCrossCoordinate[2].x;
		strCrossCoordinate[3].y = strCrossCoordinate[0].y;	
		
		
		for ( i = 0; i < 4; i ++ )
		{ 
		
			XPT2046_DelayUS ( 300000 );		                     //适当的延时很有必要
		

			while ( ! XPT2046_ReadAdc_Smooth_XY ( & strScreenSample [i] ) );               //读取XPT2046数据到变量pCoordinate，当ptr为空时表示没有触点被按下

		}
		
		
		XPT2046_Calculate_CalibrationFactor ( strCrossCoordinate, strScreenSample, & CalibrationFactor ) ;  	 //用原始参数计算出 原始参数与坐标的转换系数
		
		if ( CalibrationFactor.Divider == 0 ) return 0;;
		
			
		usTest_x = ( ( CalibrationFactor.An * strScreenSample[3].x ) + ( CalibrationFactor.Bn * strScreenSample[3].y ) + CalibrationFactor.Cn ) / CalibrationFactor.Divider;		//取一个点计算X值	 
		usTest_y = ( ( CalibrationFactor.Dn * strScreenSample[3].x ) + ( CalibrationFactor.En * strScreenSample[3].y ) + CalibrationFactor.Fn ) / CalibrationFactor.Divider;    //取一个点计算Y值
		
		usGap_x = ( usTest_x > strCrossCoordinate[3].x ) ? ( usTest_x - strCrossCoordinate[3].x ) : ( strCrossCoordinate[3].x - usTest_x );   //实际X坐标与计算坐标的绝对差
		usGap_y = ( usTest_y > strCrossCoordinate[3].y ) ? ( usTest_y - strCrossCoordinate[3].y ) : ( strCrossCoordinate[3].y - usTest_y );   //实际Y坐标与计算坐标的绝对差
		
    if ( ( usGap_x > 15 ) || ( usGap_y > 15 ) ) return 0;;      //可以通过修改这两个值的大小来调整精度    
		

    /* 校准系数为全局变量 */ 
		strXPT2046_TouchPara[LCD_Mode].dX_X = ( CalibrationFactor.An * 1.0 ) / CalibrationFactor.Divider;
		strXPT2046_TouchPara[LCD_Mode].dX_Y = ( CalibrationFactor.Bn * 1.0 ) / CalibrationFactor.Divider;
		strXPT2046_TouchPara[LCD_Mode].dX   = ( CalibrationFactor.Cn * 1.0 ) / CalibrationFactor.Divider;
		
		strXPT2046_TouchPara[LCD_Mode].dY_X = ( CalibrationFactor.Dn * 1.0 ) / CalibrationFactor.Divider;
		strXPT2046_TouchPara[LCD_Mode].dY_Y = ( CalibrationFactor.En * 1.0 ) / CalibrationFactor.Divider;
		strXPT2046_TouchPara[LCD_Mode].dY   = ( CalibrationFactor.Fn * 1.0 ) / CalibrationFactor.Divider;

			

  XPT2046_DelayUS ( 1000000 );
ret = 1;
	return 1;    
		
}


/**
  * @brief  获取 XPT2046 触摸点（校准后）的坐标
  * @param  pDisplayCoordinate ：该指针存放获取到的触摸点坐标
  * @param  pTouchPara：坐标校准系数
  * @retval 获取情况
	*   该返回值为以下值之一：
  *     @arg 1 :获取成功
  *     @arg 0 :获取失败
  */
uint8_t XPT2046_Get_TouchedPoint ( strType_XPT2046_Coordinate * pDisplayCoordinate, strType_XPT2046_TouchPara * pTouchPara )
{
	uint8_t ucRet = 1;           //若正常，则返回0
	
	strType_XPT2046_Coordinate strScreenCoordinate; 
	

  if (XPT2046_ReadAdc_Smooth_XY(&strScreenCoordinate))
  {    
//		pDisplayCoordinate ->x = ( ( pTouchPara[LCD_SCAN_MODE].dX_X * strScreenCoordinate.x ) + ( pTouchPara[LCD_SCAN_MODE].dX_Y * strScreenCoordinate.y ) + pTouchPara[LCD_SCAN_MODE].dX );        
//		pDisplayCoordinate ->y = ( ( pTouchPara[LCD_SCAN_MODE].dY_X * strScreenCoordinate.x ) + ( pTouchPara[LCD_SCAN_MODE].dY_Y * strScreenCoordinate.y ) + pTouchPara[LCD_SCAN_MODE].dY );

  }
	 
	else ucRet = 0;            //如果获取的触点信息有误，则返回0
		
  return ucRet;
} 





/**
  * @brief  触摸屏检测状态机
  * @retval 触摸状态
	*   该返回值为以下值之一：
  *     @arg TOUCH_PRESSED :触摸按下
  *     @arg TOUCH_NOT_PRESSED :无触摸
  */
uint8_t XPT2046_TouchDetect(void)
{ 
	static enumTouchState touch_state = XPT2046_STATE_RELEASE;
	static uint32_t i;
	uint8_t detectResult = TOUCH_NOT_PRESSED;
	
	switch(touch_state)
	{
		case XPT2046_STATE_RELEASE:	
			if(XPT2046_PENIRQ_Read() == XPT2046_PENIRQ_ActiveLevel) //第一次出现触摸信号
			{
				touch_state = XPT2046_STATE_WAITING;
				detectResult =TOUCH_NOT_PRESSED;
				}
			else	//无触摸
			{
				touch_state = XPT2046_STATE_RELEASE;
				detectResult =TOUCH_NOT_PRESSED;
			}
			break;
				
		case XPT2046_STATE_WAITING:
				if(XPT2046_PENIRQ_Read() == XPT2046_PENIRQ_ActiveLevel)
				{
					 i++;
					//等待时间大于阈值则认为触摸被按下
					//消抖时间 = DURIATION_TIME * 本函数被调用的时间间隔
					//如在定时器中调用，每10ms调用一次，则消抖时间为：DURIATION_TIME*10ms
					if(i > DURIATION_TIME)		
					{
						i=0;
						touch_state = XPT2046_STATE_PRESSED;
						detectResult = TOUCH_PRESSED;
					}
					else												//等待时间累加
					{
						touch_state = XPT2046_STATE_WAITING;
						detectResult =	 TOUCH_NOT_PRESSED;					
					}
				}
				else	//等待时间值未达到阈值就为无效电平，当成抖动处理					
				{
				    i = 0;
            touch_state = XPT2046_STATE_RELEASE; 
						detectResult = TOUCH_NOT_PRESSED;
				}
		
			break;
		
		case XPT2046_STATE_PRESSED:	
				if(XPT2046_PENIRQ_Read() == XPT2046_PENIRQ_ActiveLevel)		//触摸持续按下
				{
					touch_state = XPT2046_STATE_PRESSED;
					detectResult = TOUCH_PRESSED;
				}
				else	//触摸释放
				{
					touch_state = XPT2046_STATE_RELEASE;
					detectResult = TOUCH_NOT_PRESSED;
				}
			break;			
		
		default:
				touch_state = XPT2046_STATE_RELEASE;
				detectResult = TOUCH_NOT_PRESSED;
				break;
	
	}		
	
	return detectResult;
}

/**
	* @brief   检测到触摸中断时调用的处理函数,通过它调用tp_down 和tp_up汇报触摸点
	*	@note 	 本函数需要在while循环里被调用，也可使用定时器定时调用
	*			例如，可以每隔5ms调用一次，消抖阈值宏DURIATION_TIME可设置为2，这样每秒最多可以检测100个点。
	*						可在XPT2046_TouchDown及XPT2046_TouchUp函数中编写自己的触摸应用
	* @param   none
	* @retval  none
	*/

void XPT2046_TouchEvenHandler(void )
{

	
		if(XPT2046_TouchDetect() == TOUCH_PRESSED)
		{
			//获取触摸坐标
			XPT2046_Get_TouchedPoint(&cinfo,strXPT2046_TouchPara);
			LED0=!LED0;
			/*更新触摸信息到pre xy*/
			cinfo.pre_x = cinfo.x; cinfo.pre_y = cinfo.y;  
			
			if (cinfo.x > 800) cinfo.x = 800;
			if (cinfo.x < 0) cinfo.x = 0;
			if (cinfo.y > 480) cinfo.y = 480;
			if (cinfo.y < 0) cinfo.y = 0;
            cinfo.pressed = 1;
		}
		else
		{
            cinfo.pressed = 0;
			/*触笔释放，把 xy 重置为负*/
			cinfo.x = -1;
			cinfo.y = -1; 
			cinfo.pre_x = -1;
			cinfo.pre_y = -1;
		}

}


/***************************end of file*****************************/


