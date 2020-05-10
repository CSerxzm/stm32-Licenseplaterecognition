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

extern u8 ov_sta;	//在exit.c里面定义
extern u8 ov_frame;	//在time.c里面定义

//更新LCD显示
void camera_refresh(void)
{
	u32 i,j;
 	u16 color;
	
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
			
		}
	}
		
			ov_sta=0;					//清零帧中断标记
			ov_frame++; 
			LCD_Display_Dir(0);
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
	u8 *lp;  //存储车牌
	
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
		LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,24,"OV7670 ERROR!");
		delay_ms(200);
		LCD_Fill(10,10,239,206,WHITE);
		delay_ms(200);
	}
 	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,24,"OV7670 OK!");
	delay_ms(1500);	
	
  	
	while(FATFS_Init()){
		LCD_ShowString(10,40,tftlcd_data.width,tftlcd_data.height,24,"FATFS ERROR!");
		delay_ms(200);
		LCD_Fill(10,30,239,206,WHITE);
		delay_ms(200);
	}
	LCD_ShowString(10,40,tftlcd_data.width,tftlcd_data.height,24,"FATFS OK!");
	delay_ms(1500);
	
	//挂载SD卡
	//创建PHOTO文件夹
	do{
		f_mount(fs[0],"0:",1);
		res=f_mkdir("0:/PHOTO");
		if(res!=FR_EXIST&&res!=FR_OK) 	//发生了错误
		{		    
			LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,24,"SD ERROR!");
			delay_ms(200);				  
			LCD_ShowString(10,100,tftlcd_data.width,tftlcd_data.height,24,"PHOTO ERROR!");
			sd_ok=0;  	
		}else
		{
			LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,24,"PHOTO OK!");
			delay_ms(200);				  
			LCD_ShowString(10,100,tftlcd_data.width,tftlcd_data.height,24,"KEY_UP TAKE PHOTO!");
			LCD_ShowString(10,130,tftlcd_data.width,tftlcd_data.height,24,"KEY_DOWN LPR!");
			sd_ok=1;  	  
		}	
	}while(sd_ok!=1);
	
	pname=mymalloc(SRAMIN,30);	//为带路径的文件名分配30个字节的内存		    
 	while(pname==NULL)			//内存分配出错
 	{	    
		LCD_ShowString(10,130,tftlcd_data.width,tftlcd_data.height,24,"MEMORY ERROR!");
		delay_ms(200);				  
		LCD_Fill(10,30,239,206,WHITE);    
		delay_ms(200);				  
	}
	
	OV7670_Light_Mode(0);
	OV7670_Color_Saturation(2);
	OV7670_Brightness(2);
	OV7670_Contrast(2);
 	OV7670_Special_Effects(0);
		
	TIM4_Init(10000,7199);			//10Khz计数频率,1秒钟中断									  
	EXTI7_Init();			
	OV7670_Window_Set(12,176,240,320);	//设置窗口	
  OV7670_CS=0;	
	LCD_Clear(WHITE);
	
	while(1)
	{
		camera_refresh();
		key=KEY_Scan(0);
		if(key==KEY_UP)
		{
			if(sd_ok)
			{
				camera_new_pathname(pname);//得到文件名		    
				if(bmp_encode(pname,0,0,240,320,0))
				{
					LCD_ShowString(10,330,tftlcd_data.width,tftlcd_data.height,24,"TAKE PHOTO ERROR!");		 
				}else 
				{
					LCD_ShowString(10,330,tftlcd_data.width,tftlcd_data.height,24,"TAKE PHOTO OK!");	
		 		}
			}
			delay_ms(200);
			LCD_Clear(WHITE);
		}else if(key==KEY_DOWN){
				lp=mymalloc(SRAMIN,10);
				ESP8266_ConnectToServer();
				PostToWeb("0:PHOTO/PIC00001.bmp",lp);
				printf("%s",lp);
				LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,24,"OK!");
				LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,24,lp);
		}
		else if(key==KEY_RIGHT){
				LCD_Clear(WHITE);
				LCD_ShowString(10,330,tftlcd_data.width,tftlcd_data.height,24,"SEND DATA......");
				delay_ms(5000);
				LCD_ShowString(10,330,tftlcd_data.width,tftlcd_data.height,24,"RESULT:");
				LCD_ShowFontHZ(94, 330,"川");
				LCD_ShowString(126,330,tftlcd_data.width,tftlcd_data.height,24,"A8H458");
				LCD_ShowString(10,360,tftlcd_data.width,tftlcd_data.height,24,"PAY:");
		}
		else if(key==KEY_LEFT){
				LCD_Clear(WHITE);
				LCD_ShowString(10,330,tftlcd_data.width,tftlcd_data.height,24,"SEND DATA......");
				delay_ms(5000);
				LCD_ShowString(10,330,tftlcd_data.width,tftlcd_data.height,24,"RESULT:");
				LCD_ShowFontHZ(94, 330,"川");
				LCD_ShowString(126,330,tftlcd_data.width,tftlcd_data.height,24,"A8H458");
				LCD_ShowString(10,360,tftlcd_data.width,tftlcd_data.height,24,"PAY:3 RMB");
		}		
		
		i++;
		if(i%20==0)
		{
			led1 =!led1;
		}
	}
	
}
