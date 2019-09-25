#include <rtthread.h>
#include <rtdevice.h>

#include "ledblink.h"


// 默认 4 路
#if defined(SOC_W600_A8xx)
#define DTU_LED_PIN 19
#elif defined(SOC_W601_A8xx)
#define DTU_LED_PIN 30
#endif

#define LINE_ON_STATE PIN_LOW
#define LINE_OFF_STATE PIN_HIGH


struct dtu_led {
  rt_uint8_t state;
  rt_uint8_t on;
  rt_uint8_t off;
  rt_uint8_t tick;
  rt_base_t pin;
};
static struct dtu_led gled;
static struct rt_timer timer;


void led_on(void)
{
  rt_pin_write(gled.pin, LINE_ON_STATE);
}

void led_off(void)
{
  rt_pin_write(gled.pin, LINE_OFF_STATE);
}
/*
1, 定时器关
2, led on
*/
void led_always_on(void)
{
  rt_timer_stop(&timer);
  led_on();
}


void led_always_off(void)
{
  rt_timer_stop(&timer);
  led_off();
}


void led_blink(rt_uint8_t on, rt_uint8_t off)
{
  // 不能等于0
  RT_ASSERT(on);
  RT_ASSERT(off);

  // 先关闭现有定时器
  rt_timer_stop(&timer);

  // 重置状态机 闪烁总是以 on 开始
  led_on();
  gled.on = on;
  gled.off = off;
  gled.tick = 0;
  gled.state = 1;
  
  // 再开启定时器
  rt_timer_start(&timer);
}

/*
如果 state 为 1
tick += 1
if tick >= on
  tick = 0
  state = 0
  led_off
*/
void timer_timeout(void* params)
{
  gled.tick += 1;
  if(gled.state == 1) {
    if(gled.tick >= gled.on) {
      gled.tick = 0;
      gled.state = 0;
      led_off();
    }
  } else {
    if(gled.tick >= gled.off) {
      gled.tick = 0;
      gled.state = 1;
      led_on();
    }
  }
}

// 默认应该是关闭状态
int dtu_led_init(void)
{
  rt_memset(&gled, 0, sizeof(gled));

  gled.pin = DTU_LED_PIN;
  rt_pin_mode(gled.pin, PIN_MODE_OUTPUT);
  rt_timer_init(&timer, "LED", timer_timeout, NULL, 100, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
  led_off();

  return 0;
}

MSH_CMD_EXPORT(led_always_off, led off);
MSH_CMD_EXPORT(led_always_on, led on);

INIT_APP_EXPORT(dtu_led_init);
