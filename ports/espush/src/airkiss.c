
#include <rtthread.h>
#include <ulog.h>

#include "button.h"
#include <smartconfig.h>

#if defined(SOC_W600_A8xx)
// W600 EVB 按钮为 低电平触发, PB7 ==> 27
#define KEY_DOWN_EDGE PIN_LOW
#define KEY_PIN 27
#elif defined(SOC_W601_A8xx)
#define KEY_DOWN_EDGE PIN_LOW
#define KEY_PIN 27
#endif

static Button_t airkiss_btn;
static rt_timer_t btn_timer;

rt_uint8_t read_airkiss_btn_level(void)
{
  return rt_pin_read(KEY_PIN);
}

void btn_down_cb(void *btn)
{
  LOG_D("airkiss button Click!");
}

void btn_double_click_cb(void *btn)
{
  LOG_D("airkiss button Double click!");
}

extern void smartconfig_demo(void);
void btn_long_click_cb(void *btn)
{
  rt_err_t result;

  LOG_I("airkiss button Long press!");
  result = rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
  if(result == RT_EOK) {
    LOG_I("start airkiss.");
    smartconfig_demo();
  }
}

static void btn_timeout(void* params)
{
  Button_Process();
}

static void button_init(void)
{
  Button_Create("airkiss_btn", &airkiss_btn, read_airkiss_btn_level, KEY_DOWN_EDGE);
  Button_Attach(&airkiss_btn,BUTTON_DOWM,btn_down_cb);
  Button_Attach(&airkiss_btn,BUTTON_DOUBLE,btn_double_click_cb);
  Button_Attach(&airkiss_btn,BUTTON_LONG,btn_long_click_cb);

  // rt_timer 每隔 40 ms 做一次按键检测
  btn_timer = rt_timer_create("btn", btn_timeout, NULL, 40, RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
  if(btn_timer) {
    rt_timer_start(btn_timer);
  }
}

/*
WiFi 自动配网
1，按钮事件初始化
2，配网功能与回调
*/
int airkiss_and_btn_init(void)
{
  button_init();

  return 0;
}

INIT_APP_EXPORT(airkiss_and_btn_init);
