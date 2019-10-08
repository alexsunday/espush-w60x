#include <rtthread.h>
#include <string.h>

#if !defined(SAL_USING_POSIX)
#error "Please enable SAL_USING_POSIX!"
#endif

#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ulog.h>

#define BUFSZ   1024

static int is_running = 0;
static int port = 21502;

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

static void udpserv(void *paramemter)
{
    int sock;
    int flag = 1;
    int bytes_read;
    char *recv_data;
    socklen_t addr_len;
    struct sockaddr_in server_addr, client_addr;

    struct timeval timeout;
    fd_set readset;

    wait_network();
    /* 分配接收用的数据缓冲 */
    recv_data = rt_malloc(BUFSZ);
    if (recv_data == RT_NULL)
    {
        LOG_E("No memory");
        return;
    }

    /* 创建一个socket，类型是SOCK_DGRAM，UDP类型 */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        LOG_E("Create socket error");
        goto __exit;
    }
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag));

    /* 初始化服务端地址 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* 绑定socket到服务端地址 */
    if (bind(sock, (struct sockaddr *)&server_addr,
             sizeof(struct sockaddr)) == -1)
    {
        LOG_E("Unable to bind");
        goto __exit;
    }

    addr_len = sizeof(struct sockaddr);
    LOG_I("UDPServer Waiting for client on port %d...", port);

    is_running = 1;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    while (is_running)
    {
        FD_ZERO(&readset);
        FD_SET(sock, &readset);

        /* Wait for read or write */
        if (select(sock + 1, &readset, RT_NULL, RT_NULL, &timeout) == 0)
            continue;

        /* 从sock中收取最大BUFSZ - 1字节数据 */
        bytes_read = recvfrom(sock, recv_data, BUFSZ - 1, 0,
                              (struct sockaddr *)&client_addr, &addr_len);
        if (bytes_read < 0)
        {
            LOG_E("Received error, close the connect.");
            goto __exit;
        }
        else if (bytes_read == 0)
        {
            LOG_W("Received warning, recv function return 0.");
            continue;
        }
        else
        {
            recv_data[bytes_read] = '\0'; /* 把末端清零 */

            /* 输出接收的数据 */
            LOG_I("Received data = %s", recv_data);

            /* 如果接收数据是exit，退出 */
            if (strcmp(recv_data, "exit") == 0)
            {
                goto __exit;
            }
        }
    }

__exit:
    if (recv_data)
    {
        rt_free(recv_data);
        recv_data = RT_NULL;
    }
    if (sock >= 0)
    {
        closesocket(sock);
        sock = -1;
    }
    is_running = 0;
}


int finder_init(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("udp_serv",
        udpserv, RT_NULL,
        2048, RT_THREAD_PRIORITY_MAX/3, 20);
    if (tid != RT_NULL) {
        rt_thread_startup(tid);
    }

    return 0;
}

INIT_APP_EXPORT(finder_init);
