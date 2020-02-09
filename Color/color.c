#include "colorcfg.h"
#include "system.h"
#include "tftlcd.h"

#include <stdio.h>

RESULT_t result[TRACE_NUM];

u8 global_page=0;
TARGET_CONDITION_t condition[TRACE_NUM]={
     20,
	160,       //目标色度，H_MAX  蓝色160  绿色80
	           
	50,        //目标最小饱和度，S_MIN
	200,       //目标最大饱和度，S_MAX
	           
	50,        //目标最小亮度，L_MIN
	200,       //目标最大亮度，L_MAX
	           
	40,        //目标最小宽度，WIDTH_MIN
	40,        //目标最小高度，HEIGHT_MIN
	           
	400,       //目标最大宽度，WIDTH_MAX
	400,        //目标最大高度，HEIGHT_MAX
    

};
SEARCH_AREA_t area = {IMG_X, IMG_X+IMG_W, IMG_Y, IMG_Y+IMG_H};//定义搜索区域


//读取某点的颜色
void ReadColor( uint16_t usX, uint16_t usY, COLOR_RGB_t* color_rgb )
{
	uint16_t rgb;
	rgb = LCD_READPOINT( usX, usY );					//获取颜色数据
	
	//转换成值域为[0,255]的三原色值
	color_rgb->Red 		= (uint8_t)( ( rgb & 0xF800 ) >> 8 );
	color_rgb->Green    = (uint8_t)( ( rgb & 0x07E0 ) >> 3 );
	color_rgb->Blue 	= (uint8_t)( ( rgb & 0x001F ) << 3 );	
}
/*************************************/
//RGB转换为HLS
//H:色度
//L：亮度
//S：饱和度
void RGB2HSL( const COLOR_RGB_t* color_rgb, COLOR_HLS_t* color_hls )
{
	int r, g, b;
	int h, l, s;
	int max, min, dif;
	
	r = color_rgb->Red;
	g = color_rgb->Green;
	b = color_rgb->Blue;
	
	max = maxOf3Values( r, g, b );
	min = minOf3Values( r, g, b );
	dif = max - min;
	
	//计算l，亮度
	l = ( max + min ) * 240 / 255 / 2;
	
	//计算h，色度
	if( max == min )//无定义 RGB一样  黑灰白
	{
		s = 0;//饱和度0
		h = 0;//色度0
	}
	else
	{
		/*计算色度h*/
		if( max == r )//如果R值最大
		{
			if( min == b )//h介于0到40
			{
				h = 40 * ( g - b ) / dif;
			}
			else if( min == g )//h介于200到240
			{
				h = 40 * ( g - b ) / dif + 240;
			}
			
		}
		else if( max == g )
		{
			h = 40 * ( b - r ) / dif + 80;
		}
		else if( max == b )
		{
			h = 40 * ( r - g ) / dif + 160;
		}
		
		//计算饱和度s
		if( l == 0 )
		{
			s = 0;
		}
		else if( l <= 120 )
		{
			s = dif * 240 / ( max + min );
		}
		else
		{
			//s = dif * 240 / ( 480 - ( max + min ) );
            s = (dif)*240/(511 - (max+min));
		}		 
	}   
    color_hls->Hue = h;				//色度
	color_hls->Lightness = l;			//亮度
	color_hls->Saturation = s;			//饱和度
}

/************************************************/
 //  颜色匹配
//color_hls ：COLOR_HLS结构体，存储HLS格式颜色数据
//condition ：TARGET_CONDITION结构体，存放希望的颜色数据阈值
// 1：像素点颜色在目标范围内；0：像素点颜色不在目标范围内。
int ColorMatch(const COLOR_HLS_t* color_hls, const TARGET_CONDITION_t* condition )
{

    
	if(	
			color_hls->Lightness > condition->L_MIN &&
			color_hls->Lightness < condition->L_MAX &&
			color_hls->Saturation > condition->S_MIN &&
			color_hls->Saturation < condition->S_MAX
	)//比较饱和度和亮度
    {
                
        if( color_hls->Hue > condition->H_MIN &&
			color_hls->Hue < condition->H_MAX  )//颜色在范围内
            return 1;
        else
        if (condition->H_MAX < condition->H_MIN)  //设定的最大颜色小于最小颜色 说明有向下溢出 可能需要和高位颜色匹配            
        {
            if(color_hls->Hue < condition->H_MAX )
                return 1;
            if(color_hls->Hue > (condition->H_MIN-65295) )
                return 1;
        }else
        if(condition->H_MAX>240)//设定的最大颜色超过240 说明有向上溢出 可能需要和低位颜色匹配
        {
            if(color_hls->Hue > condition->H_MAX )
                return 1;
            if(color_hls->Hue < (condition->H_MAX-240) )
                return 1;
        }
   
		return 0;
    }
	else
		return 0;
}



/****************************************************/
//  寻找腐蚀中心
//  x ：腐蚀中心x坐标
//  y ：腐蚀中心y坐标
//  condition ：TARGET_CONDITION结构体，存放希望的颜色数据阈值
//  area ：SEARCH_AREA结构体，查找腐蚀中心的区域
// 1：找到了腐蚀中心，x、y为腐蚀中心的坐标；0：没有找到腐蚀中心。
int SearchCenter(uint16_t* x, uint16_t* y, const TARGET_CONDITION_t* condition,const SEARCH_AREA_t* area )
{
	uint16_t i, j, k;
	uint16_t FailCount=0;
	uint16_t SpaceX, SpaceY;
	COLOR_RGB_t rgb;
	COLOR_HLS_t hls;
	
	SpaceX = condition->WIDTH_MIN / 3;//以最小宽度除以3 为 横向查询的步进的一个单位
	SpaceY = condition->HEIGHT_MIN / 3;//以最小高度除以3 为 垂直查询的步进的一个单位
	
	/*横向步进单位+垂直步进单位 组成了一个矩形的色块*/
	for(i=area->Y_Start; i<area->Y_End; i+=SpaceY)
	{
		for(j=area->X_Start; j<area->X_End; j+=SpaceX)
		{
			FailCount = 0;
			for(k=0; k<SpaceX+SpaceY; k++)
			{
				if(k<SpaceX)
					ReadColor( j+k, i+SpaceY/2, &rgb );//查询色块中间一横的颜色
				else
					ReadColor( j+SpaceX/2, i+k-SpaceX, &rgb );//查询色块中间一竖的颜色
				RGB2HSL( &rgb, &hls );
				
				if(!ColorMatch( &hls, condition ))
					FailCount++;//颜色不匹配 失败计数+1
				
				if(FailCount>( (SpaceX+SpaceY) >> ALLOW_FAIL_PER ))//失败计数大于 色块需要查询的总点数/2^容错率
					break;//失败次数太多 退出
				
			}
			
			if(k == SpaceX+SpaceY)//k坚持到查询完毕，说明基本匹配
			{
				/*记录该色块的中心点为腐蚀中心*/
				*x = j + SpaceX / 2;
				*y = i + SpaceY / 2;
				return 1;   //记录到第一个腐蚀中心后退出函数
			}
			
		}
			
	}
	
	return 0;
		
}

/***************************************************/
// 从腐蚀中心向外腐蚀，得到新的腐蚀中心
//  oldX ：先前的腐蚀中心x坐标
//  oldX ：先前的腐蚀中心y坐标
//  condition ：TARGET_CONDITION结构体，存放希望的颜色数据阈值
//  result ：RESULT结构体，存放检测结果
// 1：检测成功；0：检测失败。
 
int Corrode(uint16_t oldX, uint16_t oldY, const TARGET_CONDITION_t* condition, RESULT_t* result )
{
	uint16_t Xmin, Xmax, Ymin, Ymax;
	uint16_t i;
	uint16_t FailCount=0;
	COLOR_RGB_t rgb;
	COLOR_HLS_t hls;
	
	for(i=oldX; i>IMG_X; i--)//从旧x点向左腐蚀
	{
		ReadColor(i, oldY, &rgb);//读点
		RGB2HSL(&rgb, &hls);//转换
		if(!ColorMatch(&hls, condition))
			FailCount++;//不匹配计数自加1
		
        if( FailCount> ((condition->WIDTH_MIN)/ALLOW_FAIL_PER) )//当识别失败点大于最小宽度/ALLOW_FAIL_PER是跳出            
			break;
	}
	Xmin=i;//获得最新蔓延的x最左边的值
	
	FailCount=0;
	for(i=oldX; i<IMG_X+IMG_W; i++)//从旧x点向右腐蚀
	{
		ReadColor(i, oldY, &rgb);
		RGB2HSL(&rgb, &hls);
		if(!ColorMatch(&hls, condition))
			FailCount++;

        if( FailCount> ((condition->WIDTH_MIN)/ALLOW_FAIL_PER) )
			break;
	}
	Xmax=i;
	
	FailCount=0;
	for(i=oldY; i>IMG_Y; i--)//从旧y点向上腐蚀
	{
		ReadColor(oldX, i, &rgb);
		RGB2HSL(&rgb, &hls);
		if(!ColorMatch(&hls, condition))
			FailCount++;

        if( FailCount> ((condition->WIDTH_MIN)/ALLOW_FAIL_PER) )
			break;
	}
	Ymin=i;
	
	FailCount=0;
	for(i=oldY; i<IMG_Y+IMG_H; i++)//从旧y点向下腐蚀
	{
		ReadColor(oldX, i, &rgb);
		RGB2HSL(&rgb, &hls);
		if(!ColorMatch(&hls, condition))
			FailCount++;
        
        if( FailCount> ((condition->WIDTH_MIN)/ALLOW_FAIL_PER) )
			break;
	}
	Ymax=i;
	
	FailCount=0;
	
	//获得腐蚀区域的中点和xy范围
	result->x = (Xmin + Xmax) / 2;
	result->y = (Ymin + Ymax) / 2;
	result->w = (Xmax - Xmin);
	result->h = (Ymax - Ymin);
	
	if( (result->w > condition->WIDTH_MIN) && (result->w < condition->WIDTH_MAX) &&
			(result->h > condition->HEIGHT_MIN) && (result->h < condition->HEIGHT_MAX)  )
		return 1;//如果腐蚀后的区域没有超过最大限定区域且没有小于最小限定区域 有效！！
	else
		return 0;
}

/*
 *返回0识别失败，1成功
 *得到匹配色块的信息
 */
int Trace(const TARGET_CONDITION_t* condition, RESULT_t* result_final)
{
	uint16_t i;
	static uint16_t x0, y0;	
	RESULT_t result;
	
    if(!SearchCenter(&x0, &y0, condition, &area))//寻找腐蚀中心
    {
        area.X_Start = IMG_X;
		area.X_End   = IMG_X+IMG_W;
		area.Y_Start = IMG_Y;
        area.Y_End   = IMG_Y+IMG_H;
        return 0;
	}
	//找到腐蚀中心 得到中点
	result.x = x0;
	result.y = y0;
	
	for(i=0; i<ITERATER_NUM; i++)//多次迭代 精确中心
	{
		Corrode(result.x, result.y, condition, &result);	//从腐蚀中心向外腐蚀，得到新的腐蚀中心
		
	}
	
	if( Corrode(result.x, result.y, condition, &result) )//重新腐蚀成功
	{
		result_final->x = result.x;
		result_final->y = result.y;
		result_final->w = result.w;
		result_final->h = result.h;
#if TRACE_NUM==1 //只有一个图像时才使用快速查找		
		/*为了快速对下一个图像进行找腐蚀中心，直接定义本次图像的腐蚀中心为下一个图像的扫描区域*/
		area.X_Start = result.x - ((result.w)>>1);
		area.X_End   = result.x + ((result.w)>>1);
		area.Y_Start = result.y - ((result.h)>>1);
		area.Y_End   = result.y + ((result.h)>>1);
#endif		
		return 1;
	}
	else
	{
		return 0;
	}

}
