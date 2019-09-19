#include <rtthread.h>
#include <sys/socket.h>
#include <netdb.h>

#include "espush.h"
#include "bootstrap.h"
#include "protoframe.h"
#include "utils.h"

const uint8 auth_success = 0x00;
const uint8 auth_faile = 0x01;
const int dns_query_interval = 1000;
const int dns_query_times = 10;
const int response_authorization_fail = -2;
const int heart_interval = 40;

int _nonblock_wait_recv(espush_connection *conn);

int ntp_sync(void)
{
	return 0;
}

int get_reconnect_wait_ms(int re_count)
{
	return 3000;
}

/*
1, ntp
2, bootstrap, connection, wait
*/
int run_espush(void)
{
	int rc;
	
	rc = ntp_sync();
	if(rc < 0) {
		rt_kprintf("ntp sync failed.\r\n");
		return -1;
	}
	
	while(1) {
		rc = sock_task();
		if(rc == -1) {
			//
		} else if(rc == response_authorization_fail) {
			// sleep 1 hours.
			rt_thread_mdelay(1000 * 3600);
		}
		rt_thread_mdelay(1000);
	}
}

int dns_query_byname(const char* name, struct sockaddr_in *serv_addr)
{
	RT_ASSERT(name);
	RT_ASSERT(serv_addr);
	int i;
	int rc = -1;
	struct hostent *server;

	for(i=0; i!=dns_query_times; ++i) {
		server = gethostbyname(name);
		if(!server) {
			rt_thread_mdelay(dns_query_interval);
			continue;
		} else {
			rc = 0;
			break;
		}
	}
	
	if(rc == -1) {
		return -1;
	}

	espush_memcpy(&serv_addr->sin_addr.s_addr, (char *) server->h_addr, server->h_length);
	return 0;
}

int create_socket_and_connect(int* sock, struct sockaddr_in *serv_addr)
{
	int i, fd;
	int rc = -1;
	RT_ASSERT(sock);
	RT_ASSERT(serv_addr);
	
	for(i=0; i != 5; ++i) {
		fd = socket(AF_INET, SOCK_STREAM, 0);
		if(fd < 0) {
			rt_kprintf("init socket failed.\r\n");
			return -1;
		}
		
		rc = connect(fd, (struct sockaddr*)serv_addr, sizeof(struct sockaddr));
		if(rc < 0) {
			rt_thread_mdelay(get_reconnect_wait_ms(i+1));
			closesocket(fd);
			continue;
		} else {
			*sock = fd;
			break;
		}
	}

	return rc;
}

int cloud_write_frame(espush_connection* conn, Frame* f)
{
	RT_ASSERT(conn);
	RT_ASSERT(f);
	
	++conn->txid;
	f->txid = conn->txid;
	
	return write_frame(conn->sock, f);
}

// cloud_authorization send auth info and wait recv result.
int cloud_authorization(espush_connection* conn)
{
	// send
	char imei_buf[16];
	get_imei((uint8*)imei_buf);
	
	Frame *f = new_login_frame(imei_buf);
	if(!f) {
		return -1;
	}
	
	int rc = cloud_write_frame(conn, f);
	free_frame(f);
	if(rc < 0) {
		rt_kprintf("write frame failed.\r\n");
		return -1;
	}
	
	// recv
	Frame *rsp = malloc_empty_frame(0);
	if(!rsp) {
		rt_kprintf("malloc empty frame failed.\r\n");
		return -1;
	}
	
	rc = recv_frame(conn->sock, rsp);
	if(rsp->length == 0) {
		rt_kprintf("authorization response body NULL, aborted.\r\n");
		free_frame(rsp);
		return -1;
	}
	
	uint8 code = rsp->data[0];
	if(code != auth_success) {
		rt_kprintf("authorization response code be %d.\r\n", code);
		free_frame(rsp);
		return -1;
	}
	
	free_frame(rsp);
	return 0;
}

int handle_heart_response_frame(espush_connection* conn, Frame* f)
{
	rt_kprintf("[%s] \r\n", __FUNCTION__);
	return 0;
}

int handle_force_firmware_upgrade_frame(espush_connection* conn, Frame* f)
{
	rt_kprintf("[%s] \r\n", __FUNCTION__);
	return 0;
}

int handle_uart_transport_frame(espush_connection* conn, Frame* f)
{
	rt_kprintf("[%s] \r\n", __FUNCTION__);
	return 0;
}

int handle_force_reboot_frame(espush_connection* conn, Frame* f)
{
	rt_kprintf("[%s] \r\n", __FUNCTION__);
	return 0;
}

int handle_server_probe_frame(espush_connection* conn, Frame* f)
{
	static uint8 probe_buf[] = {0, 3, 0, 0, 0xFF};
	return send(conn->sock, probe_buf, sizeof(probe_buf), 0);
}


int handle_frame(espush_connection *conn, Frame* f)
{
	int rc;

	rt_kprintf("handle frame.\r\n");
	switch (f->method)
	{
	case mle_heart_response:
		rc = handle_heart_response_frame(conn, f);
		break;
	case mle_firmware_upgrade:
		rc = handle_force_firmware_upgrade_frame(conn, f);
		break;
	case mle_uart_transport:
		rc = handle_uart_transport_frame(conn, f);
		break;
	case mle_force_reboot:
		rc = handle_force_reboot_frame(conn, f);
		break;
	case mle_server_probe:
		rc = handle_server_probe_frame(conn, f);
		break;
	default:
		rt_kprintf("unknown method, ignore.\r\n");
		break;
	}

	return rc;
}

/*
0，检查 buffer 的建康状况，是否已满，若已满，返回 -1 失败
1，尝试取出一个 frame，如失败，返回 0
2，如成功，则左移缓冲区，重建 buf 指针与 size 值
3，调用 handle_frame

// TODO: 此处需要循环处理，有可能有多个数据报
*/
int handle_buffer(espush_connection *conn, Buffer* buf)
{
	Frame* f;
	int rc, length;
	if(buffer_is_full(buf)) {
		return -1;
	}
	
	rc = buffer_try_extract(buf);
	if(rc == 0) {
		return 0;
	}
	length = rc;
	
	f = malloc_empty_frame(0);
	if(!f) {
		rt_kprintf("malloc empty frame on handle buffer failed.\r\n");
		return -1;
	}
	
	// length + 1 ，反序列化时，会在 data 最后一个字节 置 0，所以多传一个字节
	rt_kprintf("prepare deserialize frame, left: %d\r\n", length);
	rc = deserialize_frame(buf->buffer, length + 1, f);
	buffer_shrink(buf, length);
	if(rc < 0) {
		rt_kprintf("deserialize frame failed.\r\n");
		free_frame(f);
		return -1;
	}
	
	rc = handle_frame(conn, f);
	if(rc < 0) {
		rt_kprintf("handle frame failed.\r\n");
		free_frame(f);
		return -1;
	}
	
	return rc;
}

// run in interupt, must be quick.
void heart_timer_timeout(void* params)
{
	rt_kprintf("trace1.\r\n");
	RT_ASSERT(params);
	rt_kprintf("trace2.\r\n");
	espush_connection *conn = (espush_connection*)params;
	if(!conn->sock) {
		return;
	}
	// state handle.
	
	static uint8 heart_data[8] = {0, 3, 0, 0, 1};
	rt_kprintf("trace3.\r\n");
	send(conn->sock, heart_data, sizeof(heart_data), 0);
	rt_kprintf("trace4.\r\n");
}

// _nonblock_wait_recv
int cloud_wait_forever(espush_connection* conn)
{
	RT_ASSERT(conn);
	RT_ASSERT(!conn->state);
	conn->state = state_connection_malloc();
	if(!conn->state) {
		rt_kprintf("connection state malloc failed.\r\n");
		return -1;
	}
	
	int rc = _nonblock_wait_recv(conn);

	// 这里不能马上 free，timer 可能会使用？？？
	state_connection_free(conn->state);
	conn->state = NULL;
	
	rt_kprintf("wait recv completed. %d\r\n", rc);
	return rc;
}


int send_dev_info(espush_connection* conn)
{
	return 0;
}

int sock_task(void)
{
	int rc;
	int sock = -1;
	espush_connection conn;
	struct _server_addr_s addr;
	struct sockaddr_in serv_addr;

	espush_memset(&conn, 0, sizeof(espush_connection));
	rc = bootstrap(&addr);
	if(rc < 0) {
		rt_kprintf("bootstrap failed.\r\n");
		return -1;
	}
	
	// dns 10 times
	espush_memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	rc = dns_query_byname(addr.host, &serv_addr);
	if(rc < 0) {
		rt_kprintf("dns query failed.\r\n");
		return -1;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(addr.port);

	// socket connect 5 times
	rc = create_socket_and_connect(&sock, &serv_addr);
	if(rc < 0) {
		rt_kprintf("connect failed.\r\n");
		return -1;
	}
	
	// send login && recv login result
	conn.sock = sock;
	rc = cloud_authorization(&conn);
	if(rc < 0) {
		rt_kprintf("device authorization failed.\r\n");
		rc = response_authorization_fail;
		goto task_clean;
	}
	
	// send info
	rc = send_dev_info(&conn);
	if(rc < 0) {
		rt_kprintf("send device info failed.\r\n");
		rc = -1;
		goto task_clean;
	}

	// wait recv forever.
	rc = cloud_wait_forever(&conn);
	if(rc < 0) {
		rt_kprintf("cloud disconnected.\r\n");
		rc = -1;
		goto task_clean;
	}

task_clean:
	// closesocket
	closesocket(sock);
	return rc;
}

int test_dns(void)
{
	struct sockaddr_in addr;
	
	int rc = dns_query_byname("gw.espush.cn", &addr);
	if(rc < 0) {
		rt_kprintf("dns query failed.\r\n");
	} else {
		rt_kprintf("result: [%d]\r\n", addr.sin_addr.s_addr);
	}
	
	rc = dns_query_byname("192.168.12.32", &addr);
	if(rc < 0) {
		rt_kprintf("dns query failed.\r\n");
	} else {
		rt_kprintf("result: [%d]\r\n", addr.sin_addr.s_addr);
	}
	
	return 0;
}

int test_espush(void)
{
	sock_task();
	
	return 0;
}

MSH_CMD_EXPORT(test_espush, ESPush cloud TEST);
MSH_CMD_EXPORT(test_dns, ESPush DNS TEST);
