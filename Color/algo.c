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


void LCD_ShowNumPoint(u16 x,u16 y,u16 num)//显示4位数+2位小数点
{
	LCD_ShowChar(x,y,num/10000+0x30,24,0);
	LCD_ShowChar(x+8*1,y,num/1000%10+0x30,24,0);
	LCD_ShowChar(x+8*2,y,num/100%10+0x30,24,0);
	LCD_ShowChar(x+8*3,y,'.',24,0);
	LCD_ShowChar(x+8*4,y,num/10%10+0x30,24,0);
	LCD_ShowChar(x+8*5,y,num%10+0x30,24,0);
}

//------------------------------240方向跳变点显示函数（纵）
void ChangePoint_Show_240()//240方向跳变点显示
{
	vu16 a=0,b=0;
	
	for(a=0;a<240;a++)//显示对应的横向跳变点
	{
		FRONT_COLOR=RED;
		LCD_DrawPoint(TableChangePoint_240[a],a);//跳变点显示，红色标记
		if(TableChangePoint_240[a]>=12)					//跳变点个数（阈值）设定       阈值调节3-（1）
		{
			for(b=35;b<40;b++)						//显示达到阈值标准的点（浮动变点）
			{ 
				FRONT_COLOR=GREEN; 
				LCD_DrawPoint(b,a);//绿色标记		
			}
		}
	}
}

void ChangePoint_Analysis_240()//240跳变点分析，即分析出车牌的纵向边界和高度
{
vu16 a = 0;
	Min_ChangePoint_240=240;Max_ChangePoint_240=0;
	
	for(a=0;a<240;a++)//240扫描	，获取上下限值	：Min_ChangePoint_240，Max_ChangePoint_240				
	{
		while(TableChangePoint_240[a]<=12)									//阈值调节3-（2）
		{
			a++;
		}
		Min_ChangePoint_240=a;
		while(TableChangePoint_240[a]>12)									//阈值调节3-（3）
		{
			a++;
		}
		Max_ChangePoint_240=a;
		if(Max_ChangePoint_240-Min_ChangePoint_240>=20) a=240;//连续性阈值   	//阈值调节2-（1）
	}
	Min_ChangePoint_240=Min_ChangePoint_240-46;//向上微调3像素
	Max_ChangePoint_240=Max_ChangePoint_240-40;//向下微调2像素
	
	for(a=0;a<240;a++)//显示上界限		蓝色定位		 
	{
		FRONT_COLOR=BLUE;
		LCD_DrawPoint(a,Max_ChangePoint_240);
	}
	for(a=0;a<240;a++)//显示下界限						
	{
		FRONT_COLOR=BROWN;
		LCD_DrawPoint(a,Min_ChangePoint_240);
	}
	for(a=30;a<240;a++)//显示50,参考50像素位置处，车牌位置不能过红线，否则不能字符的归一化处理						
	{
		FRONT_COLOR=RED;
		LCD_DrawPoint(a,Min_ChangePoint_240+50);
	}
	flag_MaxMinCompare=1;
	if(Min_ChangePoint_240>Max_ChangePoint_240)//判断合法性1：最小值>最大值
	{
		flag_MaxMinCompare=0;
	}
	if(Min_ChangePoint_240==240||Max_ChangePoint_240==0)//判断合法性2：值没有重新赋值
	{
		flag_MaxMinCompare=0;
	}
	if(Max_ChangePoint_240-Min_ChangePoint_240<50)		//判断合法性3：			//阈值调节2-（2）
	{
		flag_MaxMinCompare=0;
	}	
}


//-----------------------------------------------------------------------------------------------------蓝色区域的跳变点分析函数

void ChangePoint_Analysis_320()//蓝色区域中，320跳变点分析,获得TableChangePoint_320[b]结果
{								//(先二值化，再判断白点个数，=0则是分割线）
	u16 a,b,num_color;
	u8 R1,G1,B1;
	u8 Mid_ChangePoint_240;
	u8 max_R,max_G,max_B,min_R,min_G,min_B;
	u8 mid_R,mid_G,mid_B;
	
	max_R=0;max_G=0;max_B=0;
	min_R=30;min_G=60;min_B=30;
	
	Mid_ChangePoint_240=(Min_ChangePoint_240+Max_ChangePoint_240)/2;
	for(b=Min_blue;b<Max_blue;b++)
	{
		num_color=LCD_ReadPoint(b,Mid_ChangePoint_240);//读取像素，代码优化速度有待提升 ？扫描方法也可优化，以提升速度
		
		R1=num_color>>11;
		G1=(num_color>>5)&0x3F;
		B1=num_color&0x1F;
		
		if( (R1>10) && (G1>25) && (B1>15) && (R1<=30) && (G1<=60) && (B1<=30) )//二值化,高阈值：25.55.25，较合适阈值（21,47,21）
		{
			if(max_R<R1) max_R=R1;//获得最大值和最小值
			if(max_G<G1) max_G=G1;
			if(max_B<B1) max_B=B1;
			
			if(min_R>R1) min_R=R1;
			if(min_G>G1) min_G=G1;
			if(min_B>B1) min_B=B1;		
		}
	}
	mid_R=(max_R+min_R)/2;
	mid_G=(max_G+min_G)/2;
	mid_B=(max_B+	min_B)/2;


	for(b=0;b<320;b++)//各行跳变点计数，数组清零
	{
		TableChangePoint_320[b]=0;
	}
	for(a=Min_ChangePoint_240;a<Max_ChangePoint_240;a++)								
	{
		for(b=Min_blue+1;b<Max_blue;b++)
		{
			num_color=LCD_ReadPoint(b,a);//读取像素，代码优化速度有待提升 ？扫描方法也可优化，以提升速度
			
			R1=num_color>>11;
			G1=(num_color>>5)&0x3F;
			B1=num_color&0x1F;
			
			if((R1>=mid_R) && (G1>=mid_G) && (B1>=mid_B))//二值化,高阈值：25.55.25，较合适阈值（21,47,21）
			{
				FRONT_COLOR=WHITE;
				LCD_DrawPoint(b,a);
				TableChangePoint_320[b]++;//白色，跳变点+1
			}
			else
			{
				FRONT_COLOR=BLACK;
				LCD_DrawPoint(b,a);
			}
		}
	}
}


//--------------------------------------------------------------------横向跳变点显示函数

void ChangePoint_Show_320()//320方向跳变点显示
{
		vu16 a;
	for(a=0;a<320;a++)//显示对应的横向跳变点								
	{ 
		FRONT_COLOR=RED;
		if(TableChangePoint_320[a]==0)
		{
			LCD_DrawPoint(a,0);//跳变点显示，红色标记
		}
		else
		{
			LCD_DrawPoint(a,TableChangePoint_320[a]);//跳变点显示，红色标记
		}
		
	}
}




//--------------------------------------------------颜色转化

static void RGB_HSV(u16 num)//RGB565转HSV  方便用于区分色相、饱和度、和VALUE
{
	float max,min;
	u8 r,g,b;
	r=(num>>11)*255/31;g=((num>>5)&0x3f)*255/63;b=(num&0x1f)*255/31;
	
	max=r;min=r;
	if(g>=min)max=g;
	if(b>=max)max=b;
	if(g<=max)min=g;
	if(b<=min)min=b;
	
	V=100*max/255;//转换为百分比
	S=100*(max-min)/max;//扩大100倍显示
	if(max==r) H=(g-b)/(max-min)*60;
	if(max==g) H=120+(b-r)/(max-min)*60;
	if(max==b) H=240+(r-g)/(max-min)*60;
	if(H<0) H=H-360;
}

//-------------------------------------------320方向蓝色区域的车牌定位函数
void ChangePoint_Analysis_Blue()//320蓝色区域分析,采用读取像素，得结果Min_blue,Max_blue
{																 //目的是分析出车牌的左右边界，即车牌的横向宽度。
	u16 a,b,num_color;
	u16 min_320,max_320;//各行的最小、最大值
	
	Min_blue=0;Max_blue=320;
	min_320=320;max_320=0;
	
	for(a=Min_ChangePoint_240;a<Max_ChangePoint_240;a++)								
	{
		for(b=0;b<320;b++)//不用到320    for(b=30;b<320;b++)
		{
			num_color=LCD_ReadPoint(b,a);//读取像素，代码优化速度有待提升    扫描方法也可优化，以提升速度
				RGB_HSV(num_color);//RGB565转HSV
			if( 310>H && H>190 && 100>S && S>5 && 100>V && V>20)//HSV 阈值
			{
				if(b<min_320)//获得横轴的Min和Max值，即蓝色车牌的左右边界
				{
					min_320=b;
				}
				if(b>max_320)
				{
					max_320=b;
				}
			}
		}
	}
	Min_blue=min_320+5;//获取各行的最大值//修正一点
	Max_blue=max_320-5;//获取各行的最小值//修正一点 修正加5
	
	for(a=Min_ChangePoint_240;a<Max_ChangePoint_240;a++)//显示左界限				
	{
		FRONT_COLOR=BLUE;
		LCD_DrawPoint(Min_blue,a);//LCD_DrawPoint(Min_blue,a,0xf800);
	}
	for(a=Min_ChangePoint_240;a<Max_ChangePoint_240;a++)//显示右界限					
	{
		FRONT_COLOR=BLUE;
		LCD_DrawPoint(Max_blue,a);
	}
}

//--------------------------------------字符分割

vu8 SegmentationChar(void) {
	vu16 a = 0, b = 0;
	vu8 i = 0;                                                                                  //统计分割的字符个数，不为9说明分割有误
	 
	for(b = Max_blue; b > Min_blue; b--) {     																                  // 左右界线的扫描 
		if(TableChangePoint_320[b] == 0) {       														                      //间隙分割  根据HSV比较 跳变点为0 代表空隙
			for(a = Min_ChangePoint_240; a < Max_ChangePoint_240 ; a++) {  					                //划线--调试用 车牌高度一样的线
 				LCD_DrawFRONT_COLOR(b,a+1,0x001f);
			}
			i++;
			b--;
			while(TableChangePoint_320[b] == 0) {        														         	    //划过线后，找到跳变点不为0的地方
				b--;
				if(b <= Min_blue) break;
			}
		}
	}
	i--;
LCD_ShowNum(30,220,i,2,24);//显示分割的字符个数+1，8是正常值
printf ("%d_____\r\n",i);																										         		//显示分割的字符个数+1，8是正常值
	return i;
}


void Normalized(u16 k,u16 kk)//归一化 25*50
{
	u16 a,b;
	u16 num;//保存读取像素
	u8 Mo,Yu;//取整和取模
	u8 num2,num3;
	u8 Mo_1;//
	u8 Min_240,Max_240;//框紧字符后的上下限
	
	if((k-kk)<35)
	{
		//框紧字符
		Min_240=Min_ChangePoint_240+1;
		Max_240=Max_ChangePoint_240-1;
		while(Min_240++)//框紧后，得到: Min_240
		{
			for(b=kk+1;b<k;b++)//kk1→k1     微调+2                           
			{
				num=LCD_ReadPoint(b,Min_240);
				if(num) break;
			}
			if(num) break;
		}
		while(Max_240--)//框紧后，得到: Max_240
		{
			for(b=kk+1;b<k;b++)//kk1→k1                                
			{
				num=LCD_ReadPoint(b,Max_240);
				if(num) break;
			}
			if(num) break;
		}
		Min_240-=1;
		Max_240+=2;
		FRONT_COLOR=WHITE;
		LCD_DrawPoint(kk,Min_240);//显示复制的图片
		LCD_DrawPoint( k,Max_240);//显示复制的图片
		//显示复制的图片
		num3=0;
		for(a=Min_240+1;a<Max_240;a++)
		{
			num2=10;
			for(b=kk+1;b<k;b++)//kk1→k1                                   +1
			{
				num=LCD_ReadPoint(b,a);
				FRONT_COLOR=num;
				LCD_DrawPoint(271-(k-kk-1)+num2,191+num3);//复制像素值///////////////////////////////////////////////////////////
				num2++;
			}
			num3++;
		}
		num3=0;
		Mo=(24-(k-kk-1))/(k-kk-1);//取模
		Yu=(24-(k-kk-1))%(k-kk-1);//取余
		if(Yu!=0) 
		{
			Mo_1=24/Yu;//平均Mo_1个像素，插有一个像素，机7+1
		}

		for(a=Min_240+1;a<Max_240;a++)//宽放大为25像素  =??
		{
			num2=10;
			Yu=(24-(k-kk-1))%(k-kk-1);//取余
			
			for(b=kk+1;b<k;b++)//kk1→k1                                   +1
			{
				num=LCD_ReadPoint(b,a);
				FRONT_COLOR=num;
				LCD_DrawPoint(271+num2,191+num3);
				num2++;
				Mo=(24-(k-kk-1))/(k-kk-1);//取模
				while(Mo)
				{
					FRONT_COLOR=num;
					LCD_DrawPoint(271+num2,191+num3);
					Mo--;
					num2++;
				}
				if(Yu!=0)//横轴拉长
				{	
					if(((num2+1)%Mo_1==0) && (num2!=1))//改插入的地方7+1
					{ 
						FRONT_COLOR=num;
						LCD_DrawPoint(271+num2,191+num3);
						Yu--;
						num2++;
					}
				}
			}
			num3++;
		}
//纵轴拉长
		if((Max_240-Min_240)<50)
		{
			Mo=(50-(Max_240-Min_240+1))/(Max_240-Min_240+1);//取模
			Yu=(50-(Max_240-Min_240+1))%(Max_240-Min_240+1);//取余
			Mo_1=50/Yu;
			
			num2=0;
			for(a=0;a<(Max_240-Min_240);a++)//复制图像,考虑范围是否需要进行修正？
			{//↓
				for(b=271;b<=295;b++)//271开始复制，295才结束
				{
					num=LCD_ReadPoint(b,a+191);
					FRONT_COLOR=num;
					LCD_DrawPoint(b+25,191+num2);//复制像素值
				}
				num2++;
				while(Mo)
				{
					for(b=271;b<=295;b++)//271开始复制，295才结束
					{
						num=LCD_ReadPoint(b,a+191);
						FRONT_COLOR=num;
						LCD_DrawPoint(b+25,191+num2+a);//复制像素值
					}
					Mo--;
					num2++;
				}
				if(Yu!=0)
				{
					if((((num2+1) % Mo_1)==0)&& (num2!=1))
					{
						for(b=271;b<=295;b++)//271开始复制，295才结束
						{
							num=LCD_ReadPoint(b,a+191);
							FRONT_COLOR=num;
							LCD_DrawPoint(b+25,191+num2);//复制像素值
						}
						Yu--;
						num2++;
					}
				}					
			}
		}
	}
}

//--------------------------------------------图片->数组table_picture  待修改  
void Picture_String()//图片->数组table_picture      有个BUG，先放着吧
{
	u16 a,b,num1,num;
	for(a=0;a<150;a++)//归零
	{
		table_picture[a]=0x00;	
	}	
	for(a=0;a<50;a++)//50排
	{
		for(b=0;b<24;b++)//24行
		{
			num1=LCD_ReadPoint(b+296,a+190);
//			printf("num1%d",num1);
			if(num1==0xffff)
			{
				table_picture[b/8+a*3]|=(1<<(7-b%8));
				num++;
				
			}	
	
		}
	}	//printf("num%d",num);
}

//-------------------------------------------------------------------------------------------------------------进行字符匹配
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
static vu8 MoShiShiBie_All(vu8 begin,vu8 end)																																																//字符匹配，模式识别,选择性匹配begin-end
{
	vu16 num_save = 0;
	vu8  a = 0, b = 0, e = 0, a_save = 0, st1 = 0, st2 = 0, s1 = 0, s2 = 0;
	int num1 = 0;
	
	for(a = begin; a < end; a++) {																					                         																					//36
		num1 = 0; 
		for(b = 0; b < 150; b++) { 																																																							//每个字符包含了150个字节字模数据： 即像素24*50=1200。 1200/8=150字节。
					st1 = table_picture[b];																																																					  //得到的图片装换的 数组
					st2 = Table[150 * a + b];
					for(e = 0; e < 8; e++) {																																																					//逐个字节逐个位进行比较	
						s1 = st1 & (1 << e);
						s2 = st2 & (1 << e);
						if(s1 == s2) num1++;                                                                                                            //相同则增加
						if(s1 != s2) num1--;                                                                                                            //不同则减少
					}
		}
		if(num_save < num1) {
			num_save = num1;
			a_save = a;
		}
		
		LCD_ShowNum(50, 220, a, 2,16);			                                                                                                      	//显示匹配的字符是"a"			<调试用>
		if(num1 < 0) {
			LCD_ShowNum(70, 220, 0, 4,16);	                                                                                                   	  		//显示匹配的正确像素数       <调试用>
		} else {
		LCD_ShowNum(70, 220, num1, 4,16);	                                                                                                   			//显示匹配的正确像素数     <调试用> 
		}		
		LCD_ShowNum(120, 220, num_save, 4,16);                                                                                                   	//匹配的最大值显示         <调试用> 	
	}
	return a_save;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//--------------------------------------------------------------------------------------------------------------显示16X16的汉字

void WordShow(u8 num,u16 x,u16 y)//显示汉字16*16
{
	u16 a,b,num1;
	u8 table1[32];
	if(num==1)
	{
		for(a=0;a<32;a++)
		{
			table1[a]=table_yu[a];	
		}		
	}
	if(num==2)
	{
		for(a=0;a<32;a++)
		{
			table1[a]=table_min[a];	
		}		
	}
	if(num==3)
	{
		for(a=0;a<3;a++)
		{
			table1[a]=table_lu[a];	
		}		
	}
	if(num==4)
	{
		for(a=0;a<32;a++)
		{
			table1[a]=table_zhe[a];	
		}		
	}
	if(num==5)
	{
		for(a=0;a<32;a++)
		{
			table1[a]=table_shan[a];	
		}		
	}
	if(num==6)
	{
		for(a=0;a<32;a++)
		{
			table1[a]=table_cuan[a];	
		}		
	}
	for(a=0;a<16;a++)
	{
		for(b=0;b<16;b++)
		{
			if(table1[b/8+a*2]&(1<<(7-b%8)))
			{
				num1=0xffff;
			}
			else
			{
				num1=0x0000;
			}
			FRONT_COLOR=num1;
			LCD_DrawPoint(b+x,a+y);//画点
		}				
	}	
}

//-------------------------------------------------------------------------------------------------------------显示第几组车牌

void Show_Card(u8 i)//显示第几组车牌
{
	u16 t0;
//显示汉字
	if(table_card[i][0]!=0)
	{
		WordShow(table_card[i][0],9,i*16+50);//
	}
	
	if(table_card[i][1]<10)
	{
		LCD_ShowNum(25,i*16+50,table_char[table_card[i][1]],1,24);
	}
	else
	{
		LCD_ShowChar(25,i*16+50,table_char[table_card[i][1]],24,0);
	}
	LCD_ShowChar(33,i*16+50,'.',24,0);														//点
	if(table_card[i][2]<10)
	{
		LCD_ShowNum(41,i*16+50,table_char[table_card[i][2]],1,24);
	}
	else
	{
		LCD_ShowChar(41,i*16+50,table_char[table_card[i][2]],24,0);
	}
	if(table_card[i][3]<10)
	{
		LCD_ShowNum(49,i*16+50,table_char[table_card[i][3]],1,24);
	}
	else
	{
		LCD_ShowChar(49,i*16+50,table_char[table_card[i][3]],24,0);
	}
	if(table_card[i][4]<10)
	{
		LCD_ShowNum(57,i*16+50,table_char[table_card[i][4]],1,24);
	}
	else
	{
		LCD_ShowChar(57,i*16+50,table_char[table_card[i][4]],24,0);
	}
	if(table_card[i][5]<10)
	{
		LCD_ShowNum(65,i*16+50,table_char[table_card[i][5]],1,24);
	}
	else
	{
		LCD_ShowChar(65,i*16+50,table_char[table_card[i][5]],24,0);
	}
	if(table_card[i][6]<10)
	{
		LCD_ShowNum(73,i*16+50,table_char[table_card[i][6]],1,24);
	}
	else
	{
		LCD_ShowChar(73,i*16+50,table_char[table_card[i][6]],24,0);
	}
	t0=table_card[i][7];
	LCD_ShowNum(100,i*16+50,t0,6,24);//显示时间

	if(t0<60)
	{
		LCD_ShowNumPoint(168,i*16+50,t0*8);
	}
	else
	{
		LCD_ShowNumPoint(168,i*16+50,t0/60*500+t0%60*8);
	}
	
}


//-------------------------------------------------------------------------------------------------------------字符识别函数

void CharacterRecognition()//字符识别
{

	u16 a,b,u,i;
	u8 Result;//识别结果
	for(b=Max_blue-1;b>Min_blue;b--)//由右至左识别，获取各个字符的 K KK值, 即字符边界
	{
		while(TableChangePoint_320[b]==0)b--;//取第1个字符
		k1=b+1;//+1
		while(TableChangePoint_320[b]>0) b-- ;
		kk1=b;
		if((k1-kk1)<4)//省略低于三个像素的位置
		{
			while(TableChangePoint_320[b]==0) b--;//
			k1=b+1;//+1
			while(TableChangePoint_320[b]>0) b--;
			kk1=b;
		}
		printf("A");
		while(TableChangePoint_320[b]==0) b--;//取第2个字符
		k2=b+1;
		while(TableChangePoint_320[b]>0) b--;
		kk2=b;
		if((k2-kk2)<4)//省略低于3个像素的位置
		{
			while(TableChangePoint_320[b]==0) b--;//
			k2=b+1;//+1
			while(TableChangePoint_320[b]>0) b--;
			kk2=b;
		}
		printf("B");
		while(TableChangePoint_320[b]==0) b--;//取第3个字符
		k3=b+1;//+1
		while(TableChangePoint_320[b]>0) b--;
		kk3=b;
		if((k3-kk3)<4)//省略低于3个像素的位置
		{
			while(TableChangePoint_320[b]==0) b--;//
			k3=b+1;//+1
			while(TableChangePoint_320[b]>0) b--;
			kk3=b;
		}
		printf("C");
		while(TableChangePoint_320[b]==0) b--;//取第4个字符
		k4=b+1;
		while(TableChangePoint_320[b]>0) b--;
		kk4=b;
		if((k4-kk4)<4)//省略低于3个像素的位置
		{
			while(TableChangePoint_320[b]==0) b--;//
			k4=b+1;//+1
			while(TableChangePoint_320[b]>0) b--;
			kk4=b;
		}
		printf("D");
		while(TableChangePoint_320[b]==0) b--;//取第5个字符
		k5=b+1;//+1
		while(TableChangePoint_320[b]>0) b--;
		kk5=b;
		if((k5-kk5)<4)//省略低于3个像素的位置
		{
			while(TableChangePoint_320[b]==0) b--;//
			k5=b+1;//+1
			while(TableChangePoint_320[b]>0) b--;
			kk5=b;
		}
		printf("E");
		while(TableChangePoint_320[b]==0) b--;//取第6个字符
		k6=b+1;
		while(TableChangePoint_320[b]>0) b--;
		kk6=b;
		if((k6-kk6)<4)
		{
			while(TableChangePoint_320[b]==0)b--;
			k6=b+1;
			while(TableChangePoint_320[b]>0)b--;
			kk6=b;
		}
		printf("F");
		while(TableChangePoint_320[b]==0) b--;//取第7个字符
		k7=b+1;//+1
		while(TableChangePoint_320[b]>0) b--;
		kk7=b;
		if((k7-kk7)<4)//省略低于3个像素的位置
		{
			while(TableChangePoint_320[b]==0) b--;//
			k7=b+1;//+1
			while(TableChangePoint_320[b]>0) b--;
			kk7=b;
		}
		printf("G");
		while(TableChangePoint_320[b]==0) b--;//取第八个字符
		k8=b+1;
 		while(TableChangePoint_320[b]>0) 
		{
			if(b<=Min_blue)
			{
				break;
			}
			b--;
		}
		kk8=b;
		b=Min_blue;//以防万一，还满足for循环条件
	for(a=Min_ChangePoint_240;a<Max_ChangePoint_240;a++)//划线
	{
		FRONT_COLOR=BLUE;
		LCD_DrawPoint(k1,a+1);
		LCD_DrawPoint(kk1,a+1);
		LCD_DrawPoint(k2,a+1);
		LCD_DrawPoint(kk2,a+1);
		LCD_DrawPoint(k3,a+1);
		LCD_DrawPoint(kk3,a+1);
		LCD_DrawPoint(k4,a+1);
		LCD_DrawPoint(kk4,a+1);
		LCD_DrawPoint(k5,a+1);
		LCD_DrawPoint(kk5,a+1);
		LCD_DrawPoint(k6,a+1);
		LCD_DrawPoint(kk6,a+1);
		LCD_DrawPoint(k7,a+1);
		LCD_DrawPoint(kk7,a+1);
		LCD_DrawPoint(k8,a+1);
		LCD_DrawPoint(kk8,a+1);
	}
//归一化处理：大小为25*50
//第1个字符：
	Normalized(k1,kk1);//归一化 24*24
	Picture_String();//图片->数组
	Result=MoShiShiBie_All(0,36);//字符匹配，模式识别,返回a,0<= a <36
printf("RESULT 1 %d\r\n",Result);
	if(Result<10)
	{
		LCD_ShowNum(30,200,table_char[Result],1,24);
	}
	else
	{
		FRONT_COLOR=GREEN;
		LCD_ShowChar(30,200,table_char[Result],24,0);
	}
	table_cardMeasure[6]=Result;//保存识别的车牌结果
	
//第2个字符：
	Normalized(k2,kk2);//归一化 25*50
	Picture_String();//图片->数组
	Result=MoShiShiBie_All(0,36);//字符匹配，模式识别
	printf("RESULT 2 =%d\r\n",Result);
	if(Result<10)
	{
		LCD_ShowNum(40,200,table_char[Result],1,24);
	}
	else
	{
		LCD_ShowChar(40,200,table_char[Result],24,0);
	}
	table_cardMeasure[5]=Result;//保存识别的车牌结果
	
	Normalized(k3,kk3);//归一化 25*50
	Picture_String();//图片->数组
	Result=MoShiShiBie_All(0,36);//字符匹配，模式识别
	printf("RESULT 3 =%d\r\n",Result);
	if(Result<36)
	{
		LCD_ShowNum(50,200,table_char[Result],1,24);
	}
	else
	{
		LCD_ShowChar(500,200,table_char[Result],24,0);
	}
	table_cardMeasure[4]=Result;//保存识别的车牌结果
	
	Normalized(k4,kk4);//归一化 25*50
	Picture_String();//图片->数组
	Result=MoShiShiBie_All(0,36);//字符匹配，模式识别
	printf("RESULT 4=%d\r\n",Result);
	if(Result<10)
	{
		LCD_ShowNum(60,200,table_char[Result],1,24);
	}
	else
	{
		LCD_ShowChar(60,200,table_char[Result],24,0);
	}
	table_cardMeasure[3]=Result;//保存识别的车牌结果
	
	Normalized(k5,kk5);//归一化 25*50
	Picture_String();//图片->数组
	Result=MoShiShiBie_All(0,36);//字符匹配，模式识别
	printf("RESULT 5 =%d\r\n",Result);
	if(Result<10)
	{
		LCD_ShowNum(70,200,table_char[Result],1,24);
	}
	else
	{
		LCD_ShowChar(70,200,table_char[Result],24,0);
	}
	table_cardMeasure[2]=Result;//保存识别的车牌结果
	LCD_ShowChar(80,200,'.',24,0);

	Normalized(k7,kk7);//归一化 25*50
	Picture_String();//图片->数组
	Result=MoShiShiBie_All(10,36);//字符匹配，模式识别，只匹配字母
	printf("RESULT 7 =%d\r\n",Result);
	if(Result<10)
	{
		LCD_ShowNum(90,200,table_char[Result],1,24);
	}
	else
	{
		LCD_ShowChar(90,200,table_char[Result],24,0);
	}
	table_cardMeasure[1]=Result;//保存识别的车牌结果
	
	Normalized(k8,kk8);//归一化 25*50					最后一个汉字，不做识别
	Picture_String();//图片->数组
	Result=MoShiShiBie_All(36,42);//字符匹配，匹配汉字
	WordShow(Result-35,160,40);//显示汉字
	table_cardMeasure[0]=Result-35;//保存识别的车牌结果
	printf("RESULT 8 =%d\r\n",Result);
}
//if(Result == 36) {
//		sprintf((char*)temp, "识别结果：渝");
//	} else if(Result == 37) {
//		sprintf((char*)temp, "识别结果：闽");
//	} else if(Result == 38) {
//		sprintf((char*)temp, "识别结果：沪");
//	} else if(Result == 39) {
//		sprintf((char*)temp, "识别结果：浙");
//	} else if(Result == 40) {
//	sprintf((char*)temp, "识别结果：苏");
//	} else if(Result == 41) {
//		sprintf((char*)temp, "识别结果：粤");
//	}
//	
//	sprintf((char*)temp1, "%c.%c%c%c%c%c\r\n"
//		, table_char_char[table_cardMeasure[1]], table_char_char[table_cardMeasure[2]],
//	    table_char_char[table_cardMeasure[3]], table_char_char[table_cardMeasure[4]],
//    	table_char_char[table_cardMeasure[5]], table_char_char[table_cardMeasure[6]]);

}

