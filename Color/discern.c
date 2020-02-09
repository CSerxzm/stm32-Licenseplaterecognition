#include "system.h"
#include "led.h"
#include "SysTick.h"
#include "key.h"
#include "tftlcd.h"
#include "usart.h"	 
#include "string.h"
#include "ov7670.h"
#include "time.h"
#include "exti.h"
#include "value.h"
#include "discern.h"
#include "algo.h"

#define  OV7670_WINDOW_WIDTH		320 // <=320
#define  OV7670_WINDOW_HEIGHT		240 // <=240                    

extern u8 ov_sta;	//在exit.c里面定义
extern u8 ov_frame;	//在time.c里面定义

//更新LCD显示
void camera_refresh(void)
{
	u32 i,j;
 	u16 color;
	
	Min_blue = 320;                                                     //初始化记录蓝色车牌区域的值
	Max_blue = 0;
	for(i = 0; i < 240; i++) {                                          //各行跳变点计数，数组清零
		TableChangePoint_240[i] = 0;
	}
		
	if(ov_sta)//有帧中断更新
	{
		LCD_Display_Dir(1);
		
		LCD_Set_Window(0,(tftlcd_data.height-240)/2,320-1,240-1);//将显示区域设置到屏幕中央
		OV7670_RRST=0;				//开始复位读指针 
		OV7670_RCK_L;					//设置读数据时钟为低电平	
		OV7670_RCK_H;
		OV7670_RCK_L;
		OV7670_RRST=1;				//复位读指针结束 
		OV7670_RCK_H;
		
		for(j=76800;j>0;j--)//较快方式
		{
			OV7670_RCK_L;
			color=GPIOF->IDR&0XFF;	//读数据
			OV7670_RCK_H; 
			color<<=8;  
			OV7670_RCK_L;
			color|=GPIOF->IDR&0XFF;	//读数据
			OV7670_RCK_H; 
			
			LCD_WriteData_Color(color);	//显示图片
			
			R = color>>11;
			G = (color>>5)&0x3f;
			B = color&0x1f;
			
			if((R > Red_Vlaue) && (G >= Green_Value) && (B >= Blue_Value)) {//二值化,高阈值：25.55.25，较合适阈值（21,47,21）
				color = 0xffff;		
			} else {
				color = 0x0000;
			}
			
			if(color != color_save) {                                          //跳变点
				TableChangePoint_240[i]++;				//该行跳变点计数+1
			}
				color_save = color;                                             	 //保存像素值，供下一次判断和比较						 
		}
	}
		
			ov_sta=0;					//清零帧中断标记
			ov_frame++; 
			LCD_Display_Dir(0);
}

void CameraScan()
{
		camera_refresh();
		//delay_ms(10);
		//ChangePoint_Show_240();  //240方向跳变点显示
		//delay_ms(10);
		//ChangePoint_Analysis_240();	
		//delay_ms(10);
}	

void CameraDiscern(void)
{
	vu8 i = 0;
	CameraScan();
		//返回flag_MaxMinCompare，Min_ChangePoint_240，Max_ChangePoint_240
	if(flag_MaxMinCompare==1)//跳变点筛选成功
	{
		ChangePoint_Analysis_Blue();//320蓝色区域分析,采用读取像素，得结果Min_blue,Max_blue
		if(Min_blue>Max_blue) flag_MaxMinCompare=0;//进行合理性判断1
		if((Min_blue>290)||(Max_blue>290)) flag_MaxMinCompare=0;//进行合理性判断2
	}
	delay_ms(10);
	
	if(flag_MaxMinCompare == 1) {                                                //跳变点筛选成功  左右边界获取成功 并且合理
		ChangePoint_Analysis_320();                                                //蓝色区域中，320跳变点分析,获得：TableChangePoint_320[b]结果 左右边界内二值化
		ChangePoint_Show_320(); //320方向跳变点显示	
		i = SegmentationChar(); 
		
		if(i==8)//字符分割,返回分割的字符个数，用于判断合法性
		{
				while(1)
				{
					CharacterRecognition();//字符识别
					delay_ms(9999);	
					delay_ms(9999);	
					delay_ms(9999);	
					delay_ms(9999);	
					delay_ms(9999);	
					delay_ms(9999);	
					delay_ms(9999);	
					delay_ms(9999);	
					delay_ms(9999);	
					delay_ms(9999);	
					delay_ms(9999);	
					delay_ms(9999);
					break;
				}
	
    }else
		{
			LCD_Clear(0x0000);//黑屏，显示Measure Faill
			LCD_ShowChar(8*1,200,'M',24,0);
			LCD_ShowChar(8*2,200,'e',24,0);
			LCD_ShowChar(8*3,200,'a',24,0);
			LCD_ShowChar(8*4,200,'s',24,0);
			LCD_ShowChar(8*5,200,'u',24,0);
			LCD_ShowChar(8*6,200,'r',24,0);
			LCD_ShowChar(8*7,200,'e',24,0);
			
			LCD_ShowChar(8*9,200,'F',24,0);
			LCD_ShowChar(8*10,200,'a',24,0);
			LCD_ShowChar(8*11,200,'i',24,0);
			LCD_ShowChar(8*12,200,'l',24,0);
			LCD_ShowChar(8*13,200,'l',24,0);

			delay_ms(8000);
		}
		
	}
}
