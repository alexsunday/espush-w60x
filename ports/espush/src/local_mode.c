#include <rtthread.h>
#include <mongoose.h>
#include <ulog.h>

void wait_network(void);

static void handle_device_info(struct mg_connection *nc, struct http_message *hm)
{
    char n1[100], n2[100];
    double result;

    /* Get form variables */
    mg_get_http_var(&hm->body, "n1", n1, sizeof(n1));
    mg_get_http_var(&hm->body, "n2", n2, sizeof(n2));

    /* Send headers */
    mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");

    /* Compute the result and send it back as a JSON object */
    result = strtod(n1, NULL) + strtod(n2, NULL);
    mg_printf_http_chunk(nc, "{ \"result\": %lf }", result);
    mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
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
        struct http_message *hm = (struct http_message *) ev_data;
        char addr[32];
        mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
        LOG_I("%p: %.*s %.*s", nc, (int) hm->method.len, hm->method.p, (int) hm->uri.len, hm->uri.p);
        mg_send_response_line(nc, 200, "Content-Type: text/html\r\n"
                "Connection: close");
        mg_printf(nc, "\r\n<h1>Hello, %s!</h1>\r\n"
                "You asked for %.*s\r\n", addr, (int) hm->uri.len, hm->uri.p);
        nc->flags |= MG_F_SEND_AND_CLOSE;
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
