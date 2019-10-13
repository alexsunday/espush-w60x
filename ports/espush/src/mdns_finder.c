#include <rtthread.h>
#include <lwip/apps/mdns.h>
#include <ulog.h>

static void srv_txt(struct mdns_service *service, void *txt_userdata)
{
    err_t rc = mdns_resp_add_service_txtitem(service, "path=/", 6);
    LWIP_ERROR("mdns add service txt failed\n", (rc == ERR_OK), return);
}

extern struct netif *netif_default;
void mdns_finder_task(void* params)
{
    // 等待网络就绪
    wait_network();
    LOG_I("network ready.");

    // 初始化 mdns
    mdns_resp_init();
    LOG_I("mdns init completed.");

    // 查找默认 rtt 网卡

    // 查找 lwip 网卡
    // struct netif *netif_find(const char *name)

    // 或者使用 extern struct netif *netif_default;
    LOG_I("default netif 0x%p", netif_default);

    // 设置 mdns 域名信息 light-espush    
    err_t rc = mdns_resp_add_netif(netif_default, "lighter", 64);
    if(rc == ERR_OK) {
        LOG_I("add netif succ.");
    } else {
        LOG_E("add netif failed. %d", rc);
    }

    // rc = mdns_resp_add_service(netif_default, "_espush", "_http", DNSSD_PROTO_TCP, 80, 3600, srv_txt, NULL);
    rc = mdns_resp_add_service(netif_default, "lighter", "_espush", DNSSD_PROTO_TCP, 80, 3600, srv_txt, NULL);
    if(rc == ERR_OK) {
        LOG_I("add service succ.");
    } else {
        LOG_E("add service failed.");
    }

    // 每 3 秒钟重新刷一次
    while(1) {
        rt_thread_mdelay(1000 * 3);
        mdns_announce(netif_default, IP4_ADDR_ANY);
    }
}


void netdev_test(void)
{
    mdns_finder_task(NULL);
}

MSH_CMD_EXPORT(netdev_test, netdev_test.);
