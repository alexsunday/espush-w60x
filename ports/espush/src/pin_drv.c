#include "drv.h"

// 默认 4 路
#if defined(SOC_W600_A8xx)
#define PIN_1 19
#define PIN_2 20
#define PIN_3 21
#define PIN_4 22
#elif defined(SOC_W601_A8xx)
#define PIN_1 19
#define PIN_2 20
#define PIN_3 21
#define PIN_4 22
#endif

/*
开关或照明设备驱动-本机 GPIO 驱动
*/
int device_set_line_state(rt_uint8_t line, rt_bool_t state)
{
  return -1;
}

// 获取总回路数
int device_get_lines(void)
{
  return -1;
}

int device_get_lines_state(rt_bool_t *states, size_t max_size)
{
  return -1;
}
