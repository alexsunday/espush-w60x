
#include <rtthread.h>
#include <finsh.h>
#include <sys/socket.h>
#include "netdb.h"


int helloespush(void)
{
	rt_kprintf("HELLO, WORLD2.\r\n");
	return 0;
}


static void timer1(void* par)
{
	rt_kprintf("timer1 timeout.\r\n");
}

static void timer2(void* par)
{
	rt_kprintf("timer2 timeout.\r\n");
}

int test_timer(void)
{
	rt_timer_t t1 = rt_timer_create("timer1", timer1, NULL, 100 * 10, RT_TIMER_FLAG_PERIODIC);
	rt_timer_t t2 = rt_timer_create("timer2", timer2, NULL, 100 * 5,  RT_TIMER_FLAG_ONE_SHOT);
	
	rt_timer_start(t1);
	rt_timer_start(t2);

	rt_kprintf("timer start completed.\r\n");
	return 0;
}

MSH_CMD_EXPORT(helloespush, ESPush Hello world);
MSH_CMD_EXPORT(test_timer, ESPush timer test);
