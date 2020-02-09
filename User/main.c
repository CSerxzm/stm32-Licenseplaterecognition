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

#include "color.h"
#include "value.h"
#include "discern.h"

#include "esp8266_drive.h"

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
	
	ESP8266_Init(115200);
	ESP8266_STA_LinkAP();
	
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
		CameraDiscern();
		key=KEY_Scan(0);
		if(key==KEY_UP)
		{
			if(sd_ok)
			{
				camera_new_pathname(pname);//得到文件名		    
				if(bmp_encode(pname,0,0,240,320,0))
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
				LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"ok!");
		}
		else if(key==KEY_RIGHT){
				ESP8266_ConnectToServer();
				PostToWeb("0:PHOTO/1.txt");
				LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"ok!");
		}
		
		i++;
		if(i%20==0)
		{
			led1 =!led1;
		}
	}
	
}
