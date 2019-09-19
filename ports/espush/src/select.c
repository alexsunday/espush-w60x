
#include <rtthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <dfs_posix.h>
#include <netdb.h>
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
		rt_kprintf("socket recv, but response < 0, %d\r\n", rc);
		return -1;
	}
	if(rc == 0) {
		rt_kprintf("connection closed.\r\n");
		return -1;
	}
	has_recv = rc;
	
	rc = buffer_add_data(conn->res->buffer, rcv_buf, has_recv);
	if(rc < 0) {
		rt_kprintf("add data to buffer failed.\r\n");
		return -1;
	}
	
	return handle_buffer(conn, conn->res->buffer);
}

int async_pipe_read(espush_connection* conn)
{
	int rc;
	uint8 rcv_buf[128];
	
	rt_kprintf("pipe readable \r\n");
	rc = read(conn->res->read_fd, rcv_buf, sizeof(rcv_buf));
	if(rc == 0) {
		rt_kprintf("pipe read rsp code 0.\r\n");
		return 0;
	}
	if(rc < 0) {
		rt_kprintf("pipe read failed. %d\r\n", rc);
		return -1;
	}
	rt_kprintf("recv %d bytes from pipe.\r\n", rc);
	
	rc = send(conn->sock, rcv_buf, rc, 0);
	rt_kprintf("send completed, sent: %d\r\n", rc);
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
		rt_kprintf("timeout???\r\n");
		return 0;
	}
	
	if(rc < 0) {
		rt_kprintf("select return code: [%d]\r\n", rc);
		return -1;
	}
	
	if(FD_ISSET(conn->sock, rdset)) {
		rt_kprintf("network recv data.\r\n");
		rc = async_sock_recv(conn);
		if(rc < 0) {
			rt_kprintf("async sock recv failed.\r\n");
			return -1;
		}
	}
	
	if(FD_ISSET(conn->res->read_fd, rdset)) {
		rt_kprintf("pipe read data.\r\n");
		rc = async_pipe_read(conn);
		if(rc < 0) {
			rt_kprintf("async pipe recv failed.\r\n");
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
		rt_kprintf("malloc resource failed.\r\n");
		return -1;
	}
	
	rc = resource_heart_timer_init(conn->res, heart_timeout, conn);
	if(rc < 0) {
		rt_kprintf("heart timer init failed.\r\n");
		goto wait_clean;
	}
	
	while(1) {
		rc = select_handle(conn, &rdset);
		if(rc < 0) {
			rt_kprintf("select return code error.\r\n");
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

int test_select(void)
{
	int rc, fd, maxfd;
	char rcv_buf[128];
	fd_set readset;
	
	rc = new_socket_and_dial();
	if(rc < 0) {
		rt_kprintf("new socket failed.\r\n");
		return -1;
	}
	fd = rc;
	
	maxfd = fd + 1;
	while(1) {
		FD_ZERO(&readset);
		FD_SET(fd, &readset);
		
		rc = select(maxfd, &readset, NULL, NULL, NULL);
		if(rc == 0) {
			rt_kprintf("continue...\r\n");
			continue;
		}
		
		if(FD_ISSET(fd, &readset)) {
			rt_memset(rcv_buf, 0, sizeof(rcv_buf));
			rc = recv(fd, rcv_buf, 10, 0);
			rcv_buf[10] = 0;
			rt_kprintf("rc: [%d], rs: [%s]\r\n", rc, rcv_buf);
			if(rc == 0) {
				break;
			}
		}
	}

	rc = recv(fd, rcv_buf, 100, 0);
	rt_kprintf("recv code: %d\r\n", rc);
	
	closesocket(fd);
	return -1;
}

int pipe_test(void)
{
	return -1;
}


MSH_CMD_EXPORT(test_select, select test);
MSH_CMD_EXPORT(pipe_test, pipe test);

// INIT_APP_EXPORT(test_select);
