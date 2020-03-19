#ifndef _esp8266_public_H
#define _esp8266_public_H


#include "system.h"

#define User_ESP8266_SSID	  "HUAWEI P30"	      //要连接的热点的名称
#define User_ESP8266_PWD	  "xzm111789"	  //要连接的热点的密码

#define User_ESP8266_TCPServer_IP	  "192.168.43.137"	  //要连接的服务器的IP  
#define User_ESP8266_TCPServer_PORT	  "8080"	  //要连接的服务器的端口


extern volatile uint8_t TcpClosedFlag;  //定义一个全局变量

//static char *itoa( int value, char *string, int radix );
void USART_printf( USART_TypeDef * USARTx, char * Data, ... );


void ESP8266_STA_LinkAP(void);
void ESP8266_ConnectToServer(void);
void ESP8266_STA_TCPSend(char *str);

void  PostToWeb(u8 *pname);


#endif
