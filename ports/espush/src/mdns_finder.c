#include <rtthread.h>
#include <lwip/apps/mdns.h>
#include <ulog.h>

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

    // 设置 mdns 域名信息 light-espush    
    err_t rc = mdns_resp_add_netif(netif_default, "light-espush", 64);
    if(rc == ERR_OK) {
        LOG_I("add netif succ.");
    } else {
        LOG_I("add netif failed. %d", rc);
    }
}


void netdev_test(void)
{
    mdns_finder_task(NULL);
}

MSH_CMD_EXPORT(netdev_test, netdev_test.);
