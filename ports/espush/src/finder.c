#include <rtthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ulog.h>

#include "finder.h"


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
    rt_kprintf("get socket: %d\r\n", sock);

    serv.sin_family = AF_INET;
    serv.sin_port = htons(21502);
    serv.sin_addr.s_addr = 0;

    rc = bind(sock, (struct sockaddr*)&serv, sizeof(struct sockaddr_in));
    if(rc < 0) {
        LOG_E("udp sock bind failed.");
        return -1;
    }
    rt_kprintf("bind succ.\r\n");

    while(1) {
        rt_memset(rcv_buf, 0, sizeof(rcv_buf));
        rt_kprintf("wait and recv.\r\n");
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
*/
void device_finder_task(void)
{
    rt_bool_t state;
    state = rt_wlan_is_ready();
    if(!state) {
        return;
    }

    finder();
}

MSH_CMD_EXPORT(device_finder_task, device finder);
