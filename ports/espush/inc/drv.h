#ifndef  _GUARD_H_LIGHT_DRIVER_H_
#define  _GUARD_H_LIGHT_DRIVER_H_

#include "rtdef.h"

/*
照明设备、开关设备驱动
1，对某个回路执行 开/关
2，查询所有回路状态
3，批量执行 开、关 操作
4，【可选】调光查询与设置
5，【可选】设置默认状态
*/

int device_set_line_state(rt_uint8_t line, rt_bool_t state);

// 获取总回路数
int device_get_lines(void);

uint32_t device_get_lines_state(void);

#endif
