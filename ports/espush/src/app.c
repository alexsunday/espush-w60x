
#include <rtthread.h>
#include <finsh.h>
#include <sys/socket.h>
#include "netdb.h"
#include "espush.h"

#define THREAD_PRIORITY         25
#define THREAD_STACK_SIZE       512
#define THREAD_TIMESLICE        5
ALIGN(RT_ALIGN_SIZE)
static char espthread_stack[1024];
static struct rt_thread espthread;

static int espush(void)
{
	rt_thread_init(&espthread, "espush", sock_task, NULL, &espthread_stack[0], sizeof(espthread_stack), THREAD_PRIORITY - 1, THREAD_TIMESLICE);
	rt_thread_startup(&espthread);
	return 0;
}

static int run_espush(void)
{
	sock_task(NULL);
}

MSH_CMD_EXPORT(espush, ESPush Hello world);
MSH_CMD_EXPORT(run_espush, run espush directly);
