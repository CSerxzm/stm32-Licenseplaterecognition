#ifndef TEST_H
#define TEST_H

#include "sys.h"
#include "colorcfg.h"
#include "lcd.h"

extern u32 color_f;
void change_color(u8 H,u8 S,u8 L);
void set_color(u16 R,u16 G,u16 B);
u32 read_color(u16 x,u16 y);
u8 compare(void *res,u8 i,u8 *count);

#endif
