#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "key.h"
#include "malloc.h" 
#include "sd.h"
#include "flash.h"
#include "ff.h" 
#include "fatfs_app.h"
#include "exti.h"
#include "time.h"  
#include "ov7670.h"
#include "bmp.h"

#include "esp8266_drive.h"
#include "sta_tcpclent.h"

extern u8 ov_sta;	//在exit.c里面定义
extern u8 ov_frame;	//在time.c里面定义

//更新LCD显示
void camera_refresh(void)
{
	u32 j;
	u16 i;
 	u16 color;
	u16 temp;
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
			LCD_WriteData_Color(color);			
		}
		
			ov_sta=0;					//清零帧中断标记
			ov_frame++; 
			LCD_Display_Dir(0);
	} 
}

//文件名自增（避免覆盖）
//组合成:形如"0:PHOTO/PIC13141.bmp"的文件名
void camera_new_pathname(u8 *pname)
{	 
	u8 res;					 
	u16 index=0;
	while(index<0XFFFF)
	{
		sprintf((char*)pname,"0:PHOTO/PIC%05d.bmp",index);
		res=f_open(ftemp,(const TCHAR*)pname,FA_READ);//尝试打开这个文件
		if(res==FR_NO_FILE)break;		//该文件名不存在=正是我们需要的.
		index++;
	}
}

int main()
{
	u8 i=0;
	u8 sbuf[15];
	u8 count;
	u8 res;
	u8 sd_ok;
	u8 *pname;				//带路径的文件名 
	u8 key;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	
	//esp8266初始化
	ESP8266_Init(115200);
	ESP8266_STA_TCPInit();
	
	TFTLCD_Init();			//LCD初始化
	KEY_Init();
	
	EN25QXX_Init();				//初始化EN25Q128	  
	my_mem_init(SRAMIN);		//初始化内部内存池
	
	while(OV7670_Init())//初始化OV7670
	{
		LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"OV7670 Error!");
		delay_ms(200);
		LCD_Fill(10,10,239,206,WHITE);
		delay_ms(200);
	}
 	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"OV7670 OK!");
	delay_ms(1500);	
	
  	
	while(FATFS_Init()){
		LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"FATFS Error!");
		delay_ms(200);
		LCD_Fill(10,30,239,206,WHITE);
		delay_ms(200);
	}
	
	//挂载SD卡
	//创建PHOTO文件夹
	do{
		f_mount(fs[0],"0:",1);
		res=f_mkdir("0:/PHOTO");
		if(res!=FR_EXIST&&res!=FR_OK) 	//发生了错误
		{		    
			LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"SD error!");
			delay_ms(200);				  
			LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,16,"photo error!");
			sd_ok=0;  	
		}else
		{
			LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"photo ok!");
			delay_ms(200);				  
			LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,16,"key0 take photo!");
			sd_ok=1;  	  
		}	
	}while(sd_ok!=1);
	
	pname=mymalloc(SRAMIN,30);	//为带路径的文件名分配30个字节的内存		    
 	while(pname==NULL)			//内存分配出错
 	{	    
		LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"memory error!");
		delay_ms(200);				  
		LCD_Fill(10,30,239,206,WHITE);    
		delay_ms(200);				  
	}
	
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"FATFS OK!");
	delay_ms(1500);
	
	OV7670_Light_Mode(0);
	OV7670_Color_Saturation(2);
	OV7670_Brightness(2);
	OV7670_Contrast(2);
 	OV7670_Special_Effects(0);
		
	TIM4_Init(10000,7199);			//10Khz计数频率,1秒钟中断									  
	EXTI7_Init();			
	OV7670_Window_Set(12,176,240,320);	//设置窗口	
  OV7670_CS=0;	
	LCD_Clear(BLACK);
	
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY_UP)
		{
			if(sd_ok)
			{
				camera_new_pathname(pname);//得到文件名		    
				if(bmp_encode(pname,(tftlcd_data.width-240)/2,(tftlcd_data.height-320)/2,240,320,0))
				{
					LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"error!");		 
				}else 
				{
					LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"ok!");	
		 		}
			}
			delay_ms(200);
			LCD_Clear(BLACK);
		}else if(key==KEY_DOWN){
				ESP8266_STA_TCPSend("hello world!");
				LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"ok!");
		}
		
		camera_refresh();
		i++;
		if(i%20==0)
		{
			led1 =!led1;
		}
	}
	
}
