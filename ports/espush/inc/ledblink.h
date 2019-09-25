#ifndef _GUARD_H_LED_BLINK_H_
#define _GUARD_H_LED_BLINK_H_

#include <rtthread.h>

/*
LED 闪烁实现，有以下几种状态：
常闭，慢闪，快闪，常开
要求尽量简洁实现
尽量不要新开线程
可在应用里随时修改

由于主要做指示灯用，所以可以用 100ms 做单位
*/

void led_always_on(void);

void led_always_off(void);

void led_blink(rt_uint8_t on, rt_uint8_t off);

#endif
