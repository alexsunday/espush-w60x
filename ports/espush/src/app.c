
#include <rtthread.h>
#include <finsh.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ulog.h>

#include "espush.h"

#define THREAD_PRIORITY         25
#define THREAD_STACK_SIZE       512
#define THREAD_TIMESLICE        5
ALIGN(RT_ALIGN_SIZE)
static char espthread_stack[4096];
static struct rt_thread espthread;

static int espush(void)
{
	rt_thread_init(&espthread, "espush", sock_task, NULL,
		&espthread_stack[0], sizeof(espthread_stack),
		THREAD_PRIORITY - 1, THREAD_TIMESLICE);
	rt_thread_startup(&espthread);
	return 0;
}

static int run_espush(void)
{
	sock_task(NULL);
}

extern const int response_authorization_fail;
static void espush_task(void* params)
{
	int rc;
	rt_bool_t state;
	while(1) {
		state = rt_wlan_is_ready();
		if(!state) {
			rt_thread_mdelay(1000);
			continue;
		}
		LOG_I("prepare connect to cloud.");
		rc = sock_task(params);
		if(rc == response_authorization_fail) {
			rt_thread_mdelay(1000 * 3600);
			continue;
		}
	}
}

/*
系统启动后，即运行 espush 静态线程
等待在 网络准备好 事件里，若网络准备好，则开始
若因故退出，则重新等待网络准备好
*/
int espush_init(void)
{
	rt_kprintf("Welcome to espush w60x. version 0.1\r\n");

	rt_thread_init(&espthread, "espush", espush_task, NULL,
		&espthread_stack[0], sizeof(espthread_stack),
		THREAD_PRIORITY - 1, THREAD_TIMESLICE);
	rt_thread_startup(&espthread);

	return 0;
}

// MSH_CMD_EXPORT(espush, ESPush Hello world);
// MSH_CMD_EXPORT(run_espush, run espush directly);
INIT_APP_EXPORT(espush_init);

