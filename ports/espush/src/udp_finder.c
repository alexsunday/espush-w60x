#include <rtthread.h>
#include <string.h>
#include "utils.h"

#if !defined(SAL_USING_POSIX)
#error "Please enable SAL_USING_POSIX!"
#endif

#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ulog.h>

void wait_network(void)
{
    rt_bool_t state;
    // 等待网络就绪
    while(1) {
        state = rt_wlan_is_ready();
        if(!state) {
            rt_thread_mdelay(500);
            continue;
        }
        return;
    }
}

#define FINDER_USING_MDNS

#ifdef FINDER_USING_UDP

static int is_running = 0;
static int port = 21502;

/*
=> beep_espush_finder
<= beep_:imei
当下比较简单，只做设备发现
收到固定字符串后，返回 IMEI，即可
*/
static void handle_recv(int sock, const char* data, size_t length, struct sockaddr_in* client)
{
    int rc;
    char rsp_buf[32];
    /* 输出接收的数据 */
    LOG_I("Received data = %s", data);
    const char beep[] = "beep_espush_finder";
    const char* prefix = "beep_";

    if(!rt_memcmp(data, beep, rt_strlen(beep))) {
        // 返回 IMEI
        LOG_I("recv finder password.");
        rt_memset(rsp_buf, 0, sizeof(rsp_buf));

        rt_memcpy(rsp_buf, prefix, rt_strlen(prefix));
        get_imei(rsp_buf + rt_strlen(prefix));

        rc = sendto(sock, rsp_buf, rt_strlen(rsp_buf), 0,
            (struct sockaddr*)client, sizeof(struct sockaddr_in));
        if(rc < 0) {
            LOG_E("send udp failed. %d", rc);
        }
    }
}

static int finder_main(int sock)
{
    int bytes_read;
    int flag = 1;
    uint32_t addr_len;
    char recv_data[32];

    fd_set readset;
    struct timeval timeout;
    struct sockaddr_in server_addr, client_addr;

    // 配置接收广播报文
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag));
    rt_memset(recv_data, 0, sizeof(recv_data));
    /* 初始化服务端地址 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* 绑定socket到服务端地址 */
    if (bind(sock, (struct sockaddr *)&server_addr,
             sizeof(struct sockaddr)) == -1) {
        LOG_E("Unable to bind");
        return -1;
    }

    addr_len = sizeof(struct sockaddr);
    LOG_I("UDPServer Waiting for client on port %d...", port);

    is_running = 1;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    while (is_running) {
        FD_ZERO(&readset);
        FD_SET(sock, &readset);

        /* Wait for read or write */
        if (select(sock + 1, &readset, RT_NULL, RT_NULL, &timeout) == 0)
            continue;

        /* 从sock中收取最大BUFSZ - 1字节数据 */
        bytes_read = recvfrom(sock, recv_data, sizeof(recv_data) - 1, 0,
                              (struct sockaddr *)&client_addr, &addr_len);
        if (bytes_read < 0) {
            LOG_E("Received error, close the connect.");
            return -1;
        } else if (bytes_read == 0) {
            LOG_W("Received warning, recv function return 0.");
            continue;
        } else {
            recv_data[bytes_read] = '\0'; /* 把末端清零 */
            handle_recv(sock, recv_data, bytes_read, &client_addr);
        }
    }

    return 0;
}


static void udp_finder_task(void* params)
{
    int sock, rc;

    while(1) {
        // 等待网络就绪
        wait_network();
        
        // new socket
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == -1) {
            LOG_E("Create socket error");
            rt_thread_mdelay(1000);
            continue;
        }

        // udp 监听循环
        rc = finder_main(sock);
        if(rc < 0) {
            LOG_E("finder failed.");
        }
        closesocket(sock);
        is_running = 0;
    }
}
#endif

void mdns_finder_task(void* params);
int finder_init(void)
{
    rt_thread_t tid;

    // tid = rt_thread_create("finder", udp_finder_task, RT_NULL, 2048, 24, 5);
    tid = rt_thread_create("finder", mdns_finder_task, RT_NULL, 4096, 24, 5);
    if (tid != RT_NULL) {
        rt_thread_startup(tid);
    }

    return 0;
}

INIT_APP_EXPORT(finder_init);
