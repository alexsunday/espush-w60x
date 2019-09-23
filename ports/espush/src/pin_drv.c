#include <rtthread.h>
#include <rtdevice.h>
#include <ulog.h>

#include "drv.h"

// 默认 4 路
#if defined(SOC_W600_A8xx)
const int gl_lines[] = {20, 21, 22, 23};
#elif defined(SOC_W601_A8xx)
const int gl_lines[] = {30, 31, 32, 45};
#endif

#define LINE_ON_STATE PIN_LOW
#define LINE_OFF_STATE PIN_HIGH

void line_pin(int idx, int mode)
{
  int size = sizeof(gl_lines) / sizeof(int);
  if(idx > size) {
    LOG_E("too large line number\r\n");
    return;
  }

  int pin = gl_lines[idx - 1];
  rt_pin_mode(pin, PIN_MODE_OUTPUT);
  rt_pin_write(pin, mode);
}

void set_line_on(int idx)
{
  line_pin(idx, LINE_ON_STATE);
}

void set_line_off(int idx)
{
  line_pin(idx, LINE_OFF_STATE);
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

/*
返回状态的 PIN_MAP ，由低位 到 高位
*/
uint32_t device_get_lines_state(void)
{
  int i, mode, state;
  uint32_t result = 0;

  for(i=0; i!=device_get_lines(); ++i) {
    mode = rt_pin_read(gl_lines[i]) << i;
    if(mode == LINE_ON_STATE) {
      state = 1;
    } else {
      state = 0;
    }
    
    result |= state << i;
  }

  return result;
}
