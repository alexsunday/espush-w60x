主程序中创建线程，运行 espush // 此步骤可暂时先在 finsh 中完成
专用线程配置连接对象认证信息，并尝试循环服务
DNS 解析，连接到服务器，发送认证头，等待认证响应，完毕后继续
认证成功后，进入等待接收循环，接收线程接收到数据后，如能处理则处理，否则查询等待信号量，选择并激活；
创建一个 等待请求线程，如果需要请求服务端，丢入请求对象到此线程，此线程封装数据并发送数据，创建并等待在数据ID对应信号量上


伪代码

include <cjson>
include <ntp>
include <httpclient>

typedef struct {
	int sock;
	int conn_state;
	int reconnect;
} connection;

connection gl_conn;

int main() {
	config = load_config();
	espush = new_espush(config);
	while 1 {
		run_espush();
	}
}

string get_imei() {
	return ""
}

int ntp_sync() {
	//
}

int bootstrap() {
	imei = get_imei();

}

void run_espush() {
	ntp_sync();
	address = bootstrap();
	return socket_task(address);
}

int socket_task(address) {
	int sock = connect(address);
	send_dev_reg(sock, devInfo);
	frame = recvframe(sock);
	if frame.auth != succ {
		set_disable_reconnect();
		return -1;
	}

	send_dev_info(sock);
	while(1) {
		f2 = recv_frame();
		handle_frame(f2);
	}
}

int handle_frame(frame) {
	//
}

// 计划
1，先忽略 ntp
2，先忽略 bootstrap


wifi scan
wifi join DESKTOP-1 T55o7841
test_bootstrap

需要固件工程：
1、stm32f103
2、w5500，tcp，tls，ntp/rtc


// 其他线程决定是否能发送数据的标准，如果连接已断开，则无法发送数据
struct connection_state {
	rt_mutext_t lock;
	int state;
}

// 在与服务器阻塞通讯过程中使用到的资源，连接认证成功后建立，连接被关闭则释放
struct recv_resource {
	rt_pipe_t p;
	int read_fd;
	int write_fd;
	rt_timer_t heart;
	Buffer* buffer;
}

struct connection {
	int sockfd;
	connection_state* state;
	recv_resource* res;
}

int nonblock_wait_recv(conn)
{
	int rc;
	// 准备资源
	resource_malloc

	while(1) {
		rc = select_handle(conn);
		if(rc < 0) {
			break;
		}
	}

	resource_free;
	return -1;
}

int select_handle(conn)
{
    FD_ZERO();
    FD_SET(sock);
    FD_SET(pipe);

    rc = select();

    fd_isset(sockfd) {
		async_sock_recv(conn)
    }

    fd_isset(pipe) {
		async_pipe_read(conn);
    }	
}

int async_sock_recv(conn)
{
	max_left = get_left_buf
	prt = get_ptr
	rc = recv(fd, left_buf, max_left)
	process_buf(conn->buf);
}

int async_pipe_read()
{
	write(sock, ptr, length);
}


测试：
1，一次发5个
2，一次发10个
3，一次发 15个
4，一次发 20个

// 还需要测试，pipe 在 select 中的行为，是否与 socket 一致
