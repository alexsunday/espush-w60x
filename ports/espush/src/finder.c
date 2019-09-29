#include <rtthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ulog.h>

#include "finder.h"

#define THREAD_PRIORITY         24
#define THREAD_STACK_SIZE       512
#define THREAD_TIMESLICE        5
ALIGN(RT_ALIGN_SIZE)
static char finder_stack[2048];
static struct rt_thread finder_thread;

int finder(void)
{
    int sock = -1;
    int rc;
    unsigned char opt_val = 1;
    char rcv_buf[128];
    struct sockaddr_in serv, client;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        LOG_E("new udp socket failed.");
        return -1;
    }
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void*)&opt_val, sizeof(opt_val));
    LOG_D("get socket: %d\r\n", sock);

    serv.sin_family = AF_INET;
    serv.sin_port = htons(21502);
    serv.sin_addr.s_addr = 0;

    rc = bind(sock, (struct sockaddr*)&serv, sizeof(struct sockaddr_in));
    if(rc < 0) {
        LOG_E("udp sock bind failed.");
        return -1;
    }
    LOG_D("bind succ.\r\n");

    while(1) {
        rt_memset(rcv_buf, 0, sizeof(rcv_buf));
        LOG_D("wait and recv.\r\n");
        rc = recvfrom(sock, rcv_buf, sizeof(rcv_buf), 0, (struct sockaddr*)&client, sizeof(struct sockaddr_in));
        if(rc < 0) {
            LOG_E("recv from udp failed.");
            rt_thread_mdelay(2000);
            continue;
        }
        LOG_I("recv from udp %d", rc);
    }
}

/*
UDP 设备发现机制
当设备处于 STA 模式时，监听 UDP 的某个端口，由 APP 本地局域网广播
收到如下完整数据包：[ESPUSH DEVICE FINDER BROADCAST]
收到广播数据时，予以回复。
回复自身 ChipID 即可: [ESPUSH W60X112233445566]

1，确认当前网络已就绪，且处于 sta 模式
2，监听 UDP
3，如果汇报已完成设备发现，考虑退出？
*/
void device_finder_task(void* params)
{
    int rc;
    rt_bool_t state;

    while(1) {
        state = rt_wlan_is_ready();
        if(!state) {
            rt_thread_mdelay(1000);
            continue;
        }

        rc = finder();
        if(rc < 0) {
            rt_kprintf("finder error.\r\n");
            break;
        }
    }
}

int finder_init(void)
{
	rt_kprintf("espush device finder v0.1\r\n");

	rt_thread_init(&finder_thread, "finder", device_finder_task, NULL,
		&finder_stack[0], sizeof(finder_stack),
		THREAD_PRIORITY, THREAD_TIMESLICE);
	rt_thread_startup(&finder_thread);

	return 0;
}

// INIT_APP_EXPORT(finder_init);
