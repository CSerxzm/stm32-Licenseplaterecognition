#ifndef colorcfg_h
#define colorcfg_h

#include "color.h"


/*配置色块查询的范围  图像在LCD的坐标*/
#define IMG_X 0			      //图片x坐标40
#define IMG_Y 0               //图片y坐标80 
#define IMG_W 240             //图片宽度280
#define IMG_H 320             //图片高度400

#define ALLOW_FAIL_PER       2     //容错率越大 颜色匹配越高，也越难识别 取值>1
#define ITERATER_NUM         8     //迭代次数 越多精度越准
#define COLOR_RANG           10    //设定颜色的偏移范围 越大越容易识别 太大容易误识别
#define TRACE_NUM            1     //设定追踪颜色的数目

extern u8 global_page;//当前颜色的
extern SEARCH_AREA_t area;//定义搜索区域
extern RESULT_t result[TRACE_NUM];//定义搜索结果
extern TARGET_CONDITION_t condition[TRACE_NUM];//定义目标参数

#define LCD_READPOINT( usX, usY )  LCD_ReadPoint(usX,usY)//定义读点函数

#endif

