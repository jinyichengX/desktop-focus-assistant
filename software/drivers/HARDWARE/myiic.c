#include "myiic.h"
#include "delay.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//IIC 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/12/27
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	
  
//初始化IIC
void IIC_Init(void)
{					     
	RCC->AHB1ENR|=1<<1;//使能PORTB时钟 
	GPIO_Set(GPIOB,PIN7|PIN8,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_50M,GPIO_PUPD_PU);//PH4/PH5设置 
	IIC_SCL=1;
	IIC_SDA=1;
}
//产生IIC起始信号
void IIC_Start(void)
{
	SDA_OUT();     //sda线输出
	IIC_SDA=1;	  	  
	IIC_SCL=1;
	delay_us(4);
 	IIC_SDA=0;//START:when CLK is high,DATA change form high to low 
	delay_us(4);
	IIC_SCL=0;//钳住I2C总线，准备发送或接收数据 
}	  
//产生IIC停止信号
void IIC_Stop(void)
{
	SDA_OUT();//sda线输出
	IIC_SCL=0;
	IIC_SDA=0;//STOP:when CLK is high DATA change form low to high
 	delay_us(4);
	IIC_SCL=1; 
	IIC_SDA=1;//发送I2C总线结束信号
	delay_us(4);							   	
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
u8 IIC_Wait_Ack(void)
{
	int ucErrTime=0;
	SDA_IN();      //SDA设置为输入  
	IIC_SDA=1;delay_us(1);	   
	IIC_SCL=1;delay_us(1);	 
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>1000)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL=0;//时钟输出0 	   
	return 0;  
} 
//产生ACK应答
void IIC_Ack(void)
{
	IIC_SCL=0;
	SDA_OUT();
	IIC_SDA=0;
	delay_us(2);
	IIC_SCL=1;
	delay_us(2);
	IIC_SCL=0;
}
//不产生ACK应答		    
void IIC_NAck(void)
{
	IIC_SCL=0;
	SDA_OUT();
	IIC_SDA=1;
	delay_us(2);
	IIC_SCL=1;
	delay_us(2);
	IIC_SCL=0;
}					 				     
//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答			  
void IIC_Send_Byte(u8 txd)
{                        
    u8 t;   
	SDA_OUT(); 	    
    IIC_SCL=0;//拉低时钟开始数据传输
    for(t=0;t<8;t++)
    {              
        IIC_SDA=(txd&0x80)>>7;
        txd<<=1; 	  
		delay_us(2);   //对TEA5767这三个延时都是必须的
		IIC_SCL=1;
		delay_us(2); 
		IIC_SCL=0;	
		delay_us(2);
    }	 
} 	    
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
u8 IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	SDA_IN();//SDA设置为输入
    for(i=0;i<8;i++ )
	{
        IIC_SCL=0; 
        delay_us(2);
		IIC_SCL=1;
        receive<<=1;
        if(READ_SDA)receive++;   
		delay_us(1); 
    }					 
    if (!ack)
        IIC_NAck();//发送nACK
    else
        IIC_Ack(); //发送ACK   
    return receive;
}

// GT911寄存器定义
#define GT_CTRL_REG     (0x8040)
#define GT_CFGS_REG     (0x8047) 
#define GT_CHECK_REG    (0x80FF)
#define GT_PID_REG      (0x8140)
#define GT_GSTID_REG    (0x814E)
#define GT_TP1_REG      (0x8150)

// GT911设备地址
uint8_t GT911_ADDR[2] = {0x5D, 0x14};

static uint8_t gt911_dev_addr = 0;

uint8_t GT911_Write_Reg(uint16_t reg_addr, uint8_t *data, uint16_t data_len)
{
    uint8_t res = 0;
    
    IIC_Start();
    
    // 发送设备地址(写)
    IIC_Send_Byte(gt911_dev_addr << 1);
    res = IIC_Wait_Ack();
    if(res) goto end;
    
    // 发送寄存器地址高字节
    IIC_Send_Byte((uint8_t)(reg_addr >> 8));
    res = IIC_Wait_Ack();
    if(res) goto end;
    
    // 发送寄存器地址低字节  
    IIC_Send_Byte((uint8_t)(reg_addr & 0xFF));
    res = IIC_Wait_Ack();
    if(res) goto end;
    
    // 发送数据
    for(uint16_t i = 0; i < data_len; i++)
    {
        IIC_Send_Byte(data[i]);
        res = IIC_Wait_Ack();
        if(res) goto end;
    }
    
end:
    IIC_Stop();
    return res;
}

uint8_t GT911_Read_Reg(uint16_t reg_addr, uint8_t *data, uint16_t data_len)
{
    uint8_t res = 0;
    
    IIC_Start();
    
    // 发送设备地址(写)
    IIC_Send_Byte(gt911_dev_addr << 1);
    res = IIC_Wait_Ack();
    if(res) goto end;
    
    // 发送寄存器地址高字节
    IIC_Send_Byte((uint8_t)(reg_addr >> 8));
    res = IIC_Wait_Ack();
    if(res) goto end;
    
    // 发送寄存器地址低字节
    IIC_Send_Byte((uint8_t)(reg_addr & 0xFF));
    res = IIC_Wait_Ack();
    if(res) goto end;
    
    // 重新启动IIC进行读操作
    IIC_Start();
    
    // 发送设备地址(读)
    IIC_Send_Byte((gt911_dev_addr << 1) | 0x01);
    res = IIC_Wait_Ack();
    if(res) goto end;
    
    // 读取数据
    for(uint16_t i = 0; i < data_len; i++)
    {
        if(i == data_len - 1)
            data[i] = IIC_Read_Byte(0);  // 最后一个字节发送NACK
        else
            data[i] = IIC_Read_Byte(1);  // 其他字节发送ACK
    }
    
end:
    IIC_Stop();
    return res;
}
uint8_t GT911_Init(void)
{
    uint8_t temp[5] = {0};
    uint8_t res = 0;
    uint8_t dev_found = 0;
    
    // 初始化IIC
    IIC_Init();
    
    // 延时等待GT911稳定
    delay_ms(100);
    
    // 尝试第一个设备地址
    gt911_dev_addr = GT911_ADDR[0];
    res = GT911_Read_Reg(GT_PID_REG, temp, 4);
    
    // 检查第一个地址是否成功
    if(!res && temp[0] == '9' && temp[1] == '1' && temp[2] == '1')
    {
        dev_found = 1;
    }
    else
    {
        // 尝试第二个设备地址
        gt911_dev_addr = GT911_ADDR[1];
        res = GT911_Read_Reg(GT_PID_REG, temp, 4);
        if(!res && temp[0] == '9' && temp[1] == '1' && temp[2] == '1')
        {
            dev_found = 1;
        }
    }
    
    if(dev_found)
    {
        // 软件复位
        temp[0] = 0x02;
        GT911_Write_Reg(GT_CTRL_REG, temp, 1);
        
        delay_ms(10);
        
        // 结束软件复位
        temp[0] = 0x00;
        GT911_Write_Reg(GT_CTRL_REG, temp, 1);
        
        return 0; // 初始化成功
    }
    
    return 1; // 初始化失败
}
///**
// * @brief 读取触摸点数量
// * @param touch_points_num: 触摸点数量指针
// * @return 成功返回0，失败返回1
// */
//uint8_t GT911_Get_Touch_Points_Num(uint8_t *touch_points_num)
//{
//    uint8_t temp = 0;
//    uint8_t res = GT911_Read_Reg(GT_GSTID_REG, &temp, 1);
//    
//    if(!res && (temp & 0x80))
//    {
//        *touch_points_num = temp & 0x0F;
//        return 0;
//    }
//    else
//    {
//        *touch_points_num = 0;
//        return 1;
//    }
//}

///**
// * @brief 读取触摸点坐标
// * @param touch_points_num: 触摸点数量指针
// * @param x: X坐标指针
// * @param y: Y坐标指针
// * @return 成功返回0，失败返回1
// */
//uint8_t GT911_Read_Pos(uint8_t *touch_points_num, uint16_t *x, uint16_t *y)
//{
//    uint8_t data[4];
//    uint8_t res = 0;
//    
//    if(GT911_Get_Touch_Points_Num(touch_points_num) == 0)
//    {
//        if(*touch_points_num > 0)
//        {
//            res = GT911_Read_Reg(GT_TP1_REG, data, 4);
//            if(!res)
//            {
//                *x = ((data[1] & 0x0F) << 8) + data[0];
//                *y = ((data[3] & 0x0F) << 8) + data[2];
//            }
//        }
//        
//        // 清除状态寄存器
//        data[0] = 0;
//        GT911_Write_Reg(GT_GSTID_REG, data, 1);
//    }
//    
//    return res;
//}