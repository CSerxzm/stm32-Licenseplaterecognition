#include "test.h"

u32 color_f=0;
/*
 *改变查找颜色
 */
void change_color(u8 H,u8 S,u8 L)
{  
    
    condition[global_page].H_MIN=H-COLOR_RANG;
    condition[global_page].H_MAX=H+COLOR_RANG;
    
    condition[global_page].L_MIN=L-COLOR_RANG;
    condition[global_page].L_MAX=L+COLOR_RANG;
    
    condition[global_page].S_MIN=S-COLOR_RANG;
    condition[global_page].S_MAX=S+COLOR_RANG;
    
    printf("NEW H=%d S=%d L=%d\r\n",H,S,L);
    
    printf("H_MIN：%d,H_MAM：%d",condition[global_page].H_MIN,condition[global_page].H_MAX);
    LCD_ShowString(100,100,200,24,24,(u8 *)"color assignment complete!!");
}
/*
 *设置区域的假颜色
 */
void set_color(u16 R,u16 G,u16 B)
{
    color_f=((R&0XF8)<<8)|((G&0xFC)<<3)|((B&0xf8)>>3);
    
	
    printf("R:%d  ",(uint8_t)( ( color_f & 0xF800 ) >> 8 ));
    printf("G:%d  ",(uint8_t)( ( color_f & 0x07E0 ) >> 3 ));
    printf("B:%d  ",(uint8_t)( ( color_f & 0x001F ) << 3 ));
}
/*
 *比较色块
 */
u8 compare(void *res,u8 i,u8 *count)
{
    RESULT_t *re=(RESULT_t *)res;
    int tmp=i;

    for(;tmp>0;)
    {
        int t=--tmp;
        if( (re[i].y > re[t].y-(re[t].h/3) ) && (re[i].y < re[t].y+(re[t].h/3) ) )
        {
            *count=tmp;
            return 0;
        }
    }
    return 1;
}
/*
 *返回颜色
 */
u32 read_color(u16 x,u16 y)
{
//    if(x>100&&x<200)
//    {
//        if(y>500&&y<700)
//            return color_f;
//    }
    return LCD_ReadPoint(x,y);
}
