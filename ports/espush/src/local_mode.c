#include <rtthread.h>
#include <mongoose.h>
#include <ulog.h>

void wait_network(void);
void get_imei(char* out);

/*
返回 IMEI，设备类型，使用协议，芯片型号，回路数 等等
chip_id: W60X123456789012
type: light
chip: W600/W601
lines: 1/4
*/
static void handle_device_info(struct mg_connection *nc, struct http_message *hm)
{
    char resp[128];
    char imei[24];
    mg_send_response_line(nc, 200, "Content-Type: text/html\r\nConnection: close\r\n\r\n");

    rt_memset(resp, 0, sizeof(resp));
    rt_memset(imei, 0, sizeof(imei));
    get_imei(imei);
    mg_printf(nc, "{\"chip_id\":\"%.*s\",\"type\":\"light\",\"chip\":\"W60X\",\"lines\":4}", rt_strlen(imei), imei);
}

static void handle_404(struct mg_connection *nc, struct http_message *hm)
{
    mg_send_response_line(nc, 404, "Content-Type: text/html\r\nConnection: close\r\n\r\n");
    mg_printf(nc, "Not Found.");
}

void handle_http_request(struct mg_connection *nc, void* ev_data)
{
    char addr[32];
    struct http_message *hm = (struct http_message *) ev_data;

    mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
    LOG_I("%p: %.*s %.*s", nc, (int) hm->method.len, hm->method.p, (int) hm->uri.len, hm->uri.p);

    // 这里通过 URL 进行分发
    if(!rt_memcmp(hm->uri.p, "/device/info", rt_strlen("/device/info"))) {
        handle_device_info(nc, hm);
    } else {
        handle_404(nc, hm);
    }
    mg_send_http_chunk(nc, "", 0);
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

static void http_server_ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    if (ev == MG_EV_POLL)
        return;
    LOG_I("ev %d", ev);
    switch (ev) {
    case MG_EV_ACCEPT: {
        char addr[32];
        mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
        MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
        LOG_I("%p: Connection from %s", nc, addr);
        break;
    }
    case MG_EV_HTTP_REQUEST: {
        handle_http_request(nc, ev_data);
        break;
    }
    case MG_EV_RECV : {
        struct mbuf *io = &nc->recv_mbuf;
        LOG_I("recv data: %.*s", io->len, io->buf);
        break;
    }
    case MG_EV_CLOSE: {
        LOG_I("%p: Connection closed", nc);
        break;
    }
    }
}

static void mg_pool_circle(void* parameter) {
    struct mg_mgr *mgr = (struct mg_mgr *) parameter;
    while (1) {
        mg_mgr_poll(mgr, 1000);
        rt_thread_delay(rt_tick_from_millisecond(100));
    }
}

void lmode_task(void* params)
{
    const char *err;
    static struct mg_mgr mgr;
    static struct mg_connection *conn = NULL;
    struct mg_bind_opts opts = { NULL };

    /* http server test */
    opts.error_string = &err;
    // wait network ready.
    wait_network();
    mg_mgr_init(&mgr, NULL);
    conn = mg_bind_opt(&mgr, ":80", http_server_ev_handler, opts);
    if (conn == NULL) {
        LOG_D("Failed to create listener: %s", err);
        return;
    }
    mg_set_protocol_http_websocket(conn);
    LOG_I("Server address: http://%s:%d/", inet_ntoa(conn->sa.sin.sin_addr), htons(conn->sa.sin.sin_port));

    mg_pool_circle(&mgr);
}

int lmode_init(void)
{
    rt_thread_t tid;

    // tid = rt_thread_create("finder", udp_finder_task, RT_NULL, 2048, 24, 5);
    tid = rt_thread_create("lmode", lmode_task, RT_NULL, 4096, 24, 5);
    if (tid != RT_NULL) {
        rt_thread_startup(tid);
    }

    return 0;
}

INIT_APP_EXPORT(lmode_init);
