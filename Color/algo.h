#ifndef _ALGO_H
#define _ALGO_H
#include "system.h"
void	ChangePoint_Show_240(void);                                                     //240方向跳变点显示
void	ChangePoint_Analysis_240(void);	
void ChangePoint_Analysis_Blue(void);

void ChangePoint_Show_240(void);
void ChangePoint_Analysis_240(void);
void ChangePoint_Analysis_Blue(void);
void ChangePoint_Analysis_320(void);
void ChangePoint_Show_320(void);
vu8 SegmentationChar(void);
void CharacterRecognition(void);
void Normalized(vu16 k, vu16 kk);
void Show_Card(vu8 i);
void WordShow(vu8 num,vu16 x,vu16 y);
static void RGB_HSV(u16 num);
 static vu8 MoShiShiBie_All(vu8 begin,vu8 end)	;
void LCD_ShowNumPoint(u16 x,u16 y,u16 num);//显示4位数+2位小数点
#endif
