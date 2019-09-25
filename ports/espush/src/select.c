
#include <rtthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <dfs_posix.h>
#include <netdb.h>
#include <ulog.h>

#include "oswrap.h"
#include "espush.h"
#include "protoframe.h"

#define MAX_VAL(A, B) ((A) > (B) ? (A) : (B))
int handle_buffer(espush_connection *conn, Buffer* buf);


int async_sock_recv(espush_connection* conn)
{
	int rc, has_recv;
	uint8 rcv_buf[128];
	
	rc = recv(conn->sock, rcv_buf, sizeof(rcv_buf), 0);
	if(rc < 0) {
		LOG_W("socket recv, but response < 0, %d", rc);
		return -1;
	}
	if(rc == 0) {
		LOG_W("connection closed.");
		return -1;
	}
	has_recv = rc;
	
	rc = buffer_add_data(conn->res->buffer, rcv_buf, has_recv);
	if(rc < 0) {
		LOG_W("add data to buffer failed.");
		return -1;
	}
	
	return handle_buffer(conn, conn->res->buffer);
}

int async_pipe_read(espush_connection* conn)
{
	int rc;
	uint8 rcv_buf[128];
	
	LOG_D("pipe readable ");
	rc = read(conn->res->read_fd, rcv_buf, sizeof(rcv_buf));
	if(rc == 0) {
		LOG_W("pipe read rsp code 0.");
		return 0;
	}
	if(rc < 0) {
		LOG_W("pipe read failed. %d", rc);
		return -1;
	}
	LOG_D("recv %d bytes from pipe.", rc);
	
	rc = send(conn->sock, rcv_buf, rc, 0);
	LOG_D("send completed, sent: %d", rc);
	return 0;
}

int select_handle(espush_connection *conn, fd_set *rdset)
{
	int rc, maxfd;
	FD_ZERO(rdset);
	
	FD_SET(conn->sock, rdset);
	FD_SET(conn->res->read_fd, rdset);
	
	maxfd = MAX_VAL(conn->sock, conn->res->read_fd) + 1;
	rc = select(maxfd, rdset, NULL, NULL, NULL);
	if(rc == 0) {
		LOG_W("timeout???");
		return 0;
	}
	
	if(rc < 0) {
		LOG_W("select return code: [%d]", rc);
		return -1;
	}
	
	if(FD_ISSET(conn->sock, rdset)) {
		LOG_D("network recv data.");
		rc = async_sock_recv(conn);
		if(rc < 0) {
			LOG_W("async sock recv failed.");
			return -1;
		}
	}
	
	if(FD_ISSET(conn->res->read_fd, rdset)) {
		LOG_D("pipe read data.");
		rc = async_pipe_read(conn);
		if(rc < 0) {
			LOG_W("async pipe recv failed.");
			return -1;
		}
	}
	
	return 0;
}

void heart_timeout(void* params)
{
	RT_ASSERT(params);
	espush_connection* conn = (espush_connection*)params;

	RT_ASSERT(conn->state);
	// 其实这里是否不用判断，因为 heart_timeout 一定在循环体内部才会生效，退出连接循环后，定时器即被关闭

	static uint8 heart_buf[] = {0, 3, 0, 0, 0xFF};
	write(conn->res->write_fd, heart_buf, sizeof(heart_buf));
}

int _nonblock_wait_recv(espush_connection *conn)
{
	RT_ASSERT(conn);
	RT_ASSERT(conn->state);
	RT_ASSERT(!conn->res);
	
	int rc;
	fd_set rdset;
	
	conn->res = resource_malloc();
	if(!conn->res) {
		LOG_W("malloc resource failed.");
		return -1;
	}
	
	rc = resource_heart_timer_init(conn->res, heart_timeout, conn);
	if(rc < 0) {
		LOG_W("heart timer init failed.");
		goto wait_clean;
	}
	
	while(1) {
		rc = select_handle(conn, &rdset);
		if(rc < 0) {
			LOG_W("select return code error.");
			break;
		}
	}
	
wait_clean:	
	// error, resource_free will deinit timer.
	resource_free(conn->res);
	conn->res = NULL;
	return -1;
}

// test code.
int dns_query_byname(const char* name, struct sockaddr_in *serv_addr);
int new_socket_and_dial(void)
{
	int rc, fd;
	struct sockaddr_in serv_addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0) {
		return -1;
	}
	
	rt_memset(&serv_addr, 0, sizeof(struct sockaddr_in));

	rc = dns_query_byname("192.168.2.105", &serv_addr);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(12345);

	rc = connect(fd, (struct sockaddr*)(&serv_addr), sizeof(struct sockaddr));
	if(rc < 0) {
		closesocket(fd);
		return rc;
	}

	return fd;
}

void set_sock_nonblock(int fd)
{
	static int v = 1;
	ioctlsocket(fd, FIONBIO, &v);
}
