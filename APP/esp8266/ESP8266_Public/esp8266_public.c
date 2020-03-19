#include "esp8266_public.h"
#include <stdarg.h>
#include "SysTick.h"
#include "usart.h"
#include "stdlib.h"
#include "esp8266_drive.h"

#include "fatfs_app.h"
#include "malloc.h"


volatile u8 TcpClosedFlag = 0;


static char *itoa( int value, char *string, int radix )
{
	int     i, d;
	int     flag = 0;
	char    *ptr = string;

	/* This implementation only works for decimal numbers. */
	if (radix != 10)
	{
		*ptr = 0;
		return string;
	}

	if (!value)
	{
		*ptr++ = 0x30;
		*ptr = 0;
		return string;
	}

	/* if this is a negative value insert the minus sign. */
	if (value < 0)
	{
		*ptr++ = '-';

		/* Make the value positive. */
		value *= -1;
		
	}

	for (i = 10000; i > 0; i /= 10)
	{
		d = value / i;

		if (d || flag)
		{
			*ptr++ = (char)(d + 0x30);
			value -= (d * i);
			flag = 1;
		}
	}

	/* Null terminate the string. */
	*ptr = 0;

	return string;

} /* NCL_Itoa */


void USART_printf ( USART_TypeDef * USARTx, char * Data, ... )
{
	const char *s;
	int d;   
	char buf[16];

	
	va_list ap;
	va_start(ap, Data);

	while ( * Data != 0 )     // 判断是否到达字符串结束符
	{				                          
		if ( * Data == 0x5c )  //'\'
		{									  
			switch ( *++Data )
			{
				case 'r':							          //回车符
				USART_SendData(USARTx, 0x0d);
				Data ++;
				break;

				case 'n':							          //换行符
				USART_SendData(USARTx, 0x0a);	
				Data ++;
				break;

				default:
				Data ++;
				break;
			}			 
		}
		
		else if ( * Data == '%')
		{									  //
			switch ( *++Data )
			{				
				case 's':										  //字符串
				s = va_arg(ap, const char *);
				
				for ( ; *s; s++) 
				{
					USART_SendData(USARTx,*s);
					while( USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET );
				}
				
				Data++;
				
				break;

				case 'd':			
					//十进制
				d = va_arg(ap, int);
				
				itoa(d, buf, 10);
				
				for (s = buf; *s; s++) 
				{
					USART_SendData(USARTx,*s);
					while( USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET );
				}
				
				Data++;
				
				break;
				
				default:
				Data++;
				
				break;
				
			}		 
		}
		
		else USART_SendData(USARTx, *Data++);
		
		while ( USART_GetFlagStatus ( USARTx, USART_FLAG_TXE ) == RESET );
		
	}
}




/////////////////////////////////////////////////////////////////////////////
//                            以下为自己添加部分
/////////////////////////////////////////////////////////////////////////////

/*

连接热点

*/
void ESP8266_STA_LinkAP(void)
{
	printf ( "\r\n正在连接AP...\r\n" );

	ESP8266_CH_PD_Pin_SetH;

	ESP8266_AT_Test();
	ESP8266_Net_Mode_Choose(STA);
	while(!ESP8266_JoinAP(User_ESP8266_SSID, User_ESP8266_PWD));
	ESP8266_Enable_MultipleId ( DISABLE );
	
	printf ( "\r\n连接AP成功!\r\n" );
}


/*

连接服务器

*/
void ESP8266_ConnectToServer(void){
  bool state;
	if(ESP8266_Send_AT_Cmd ( "AT", "OK", NULL, 1500 ))
  {
		do{
			ESP8266_Fram_Record_Struct .InfBit .FramLength = 0;               //从新开始接收新的数据包
      memset(ESP8266_Fram_Record_Struct .Data_RX_BUF,'\0',RX_BUF_MAX_LEN);
			state=ESP8266_Send_AT_Cmd ( "AT+CIPSTART=\"TCP\",\"192.168.43.137\",8080", "OK", "ALREADY", 2500 );
		}
        while(state==false);
  }
  printf("Already Connected!\r\n");
		
  //进入透传模式
  ESP8266_Send_AT_Cmd ( "AT+CIPMODE=1", "OK", 0, 800 );            //0,非透传；1，透传
	ESP8266_Send_AT_Cmd ( "AT+CIPSEND", "\r\n", ">", 500 );
}

/*

发送图片到服务器

*/
void  PostToWeb(u8 *pname)
{
	
    char str[30];
    int i;
		u8 databuf[1024];
		FIL fsrc;
		UINT  br;
		int res;
		char lp[10];
	
		res=f_open(ftemp,(const TCHAR*)pname,FA_READ|FA_OPEN_ALWAYS);//尝试打开这个文件
	
		if(res==FR_OK){
			
				ESP8266_Fram_Record_Struct .InfBit .FramLength = 0;
				ESP8266_SendString ( ENABLE, "POST /LprSever/upload HTTP/1.1\r\n"
																		"Host:192.168.43.137:8080\r\n"
																		 "Content-Type:multipart/form-data;boundary=--xzm123456789\r\n"
																				,0, Single_ID_0 );		
				sprintf(str,"Content-Length:%d\r\n\r\n",153666+274);
				ESP8266_SendString ( ENABLE, str,0, Single_ID_0 );
				ESP8266_SendString ( ENABLE, "----xzm123456789\r\n"
																		"Content-Disposition:form-data;name=\"file\";filename=\"PIC00001.bmp\"\r\n"
																		"Content-Type:image/bmp\r\n\r\n",0, Single_ID_0 );
				//无故遗失字符‘B’
				USART2->DR='B';	
			
				while(1){
							res=f_read(ftemp, databuf,sizeof(databuf), &br); 
							for(i=0;i<br;i++)
							{
									USART2->DR=databuf[i];	
									while((USART2->SR&0x40)==0);
							}
							if (res || br < sizeof(databuf))
							{
								break; 
							}
				}
				ESP8266_SendString ( ENABLE, "\r\n----xzm123456789--\r\n", 0, Single_ID_0);	
		}
		
			delay_ms(1000);//用于接受response
			ESP8266_Fram_Record_Struct.Data_RX_BUF [ ESP8266_Fram_Record_Struct.InfBit .FramLength ] = '\0';
			strcpy(lp,&ESP8266_Fram_Record_Struct.Data_RX_BUF[179]);
			lp[9]= '\0';
			printf("%s",lp);
		
		f_close(ftemp);
}




