#include <rtthread.h>
#include <rtdevice.h> 
#include "drv.h"

// 默认 4 路
#if defined(SOC_W600_A8xx)
const int gl_lines[] = {19, 20, 21, 22};
#elif defined(SOC_W601_A8xx)
const int gl_lines[] = {19, 20, 21, 22};
#endif

void line_pin(int idx, int mode)
{
  int size = sizeof(gl_lines) / sizeof(int);
  if(idx > size) {
    rt_kprintf("too large line number\r\n");
    return;
  }

  int pin = gl_lines[idx - 1];
  rt_pin_mode(pin, PIN_MODE_OUTPUT);
  rt_pin_write(pin, mode);
}

void set_line_on(int idx)
{
  line_pin(idx, PIN_LOW);
}

void set_line_off(int idx)
{
  line_pin(idx, PIN_HIGH);
}

/*
开关或照明设备驱动-本机 GPIO 驱动
*/
int device_set_line_state(rt_uint8_t line, rt_bool_t state)
{
  if(state) {
    set_line_on(line);
  } else {
    set_line_off(line);
  }

  return 0;
}

// 获取总回路数
int device_get_lines(void)
{
  return sizeof(gl_lines) / sizeof(int);
}

int device_get_lines_state(rt_bool_t *states, size_t max_size)
{
  return -1;
}
