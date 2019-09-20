#ifndef _GUARD_H_PROTO_FRAME_H_
#define _GUARD_H_PROTO_FRAME_H_

#include <rtthread.h>
#include <ipc/pipe.h>
#include "oswrap.h"

#define FRAME_STATIC_DATA 0
#define FRAME_DYN_DATA 1

#define method_login 0x01
#define method_uart  0x03
#define cs_connected 1
#define cs_unconnected 0
#define cs_connecting 2

#define MAX_RECV_BUFFER_LENGTH 256

/*
data_flag special data field which is static or malloc data.
*/
typedef struct {
	uint8 method;
	uint8 txid;
	int8 data_flag;
	uint8* data;
	size_t length;
} Frame ;

Frame* malloc_empty_frame(uint8 method);
Frame* new_login_frame(char* imei);
void set_dyn_data(Frame *f, uint8* data, size_t d_length);
void set_static_data(Frame *f, uint8* data, size_t d_length);

void free_frame(Frame *f);

int write_frame(int fd, Frame* f);
int recv_frame(int fd, Frame *f);

int serialize_frame(Frame *f, uint8* out, size_t out_max_length);
void serialize_f(Frame *f, uint8* out);

int deserialize_frame(uint8* in_data, size_t in_length, Frame *out);

typedef struct {
	uint8* buffer;
	size_t len;
	size_t cap;
} Buffer;


union uint16_writer {
	uint16 val;
	uint8  data[2];
};

Buffer* malloc_buffer(size_t len);
void free_buffer(Buffer* buf);

int buffer_is_full(Buffer* buf);

// 返回 -1 代表已满，出错，该重置，返回 > 0 为当前 buffer 的length
int buffer_add_data(Buffer* buf, uint8* ptr, size_t len);

// 返回 0 代表无法取出，返回 > 0 代表 若干字节可以组装一个 frame
int buffer_try_extract(Buffer* buf);

// 将 前 len 个字节收缩掉
void buffer_shrink(Buffer* buf, size_t len);


struct connection_state {
	rt_mutex_t lock;
	int state;
};

struct connection_state* state_connection_malloc(void);
void state_connection_free(struct connection_state* s);
int state_get_current(struct connection_state* s);
void state_set(struct connection_state* s, int state);


struct recv_resource {
	char pipename[16];
	rt_pipe_t *p;
	int read_fd;
	int write_fd;
	rt_timer_t heart;
	Buffer* buffer;
};

struct recv_resource* resource_malloc(void);
void resource_free(struct recv_resource* r);

/*
1，创建 pipe 管道
2，打开 管道 文件，读与写
*/
int resource_pipe_init(struct recv_resource* r);
void resource_pipe_deinit(struct recv_resource* r);

int resource_heart_timer_init(struct recv_resource* r, void (*timeout)(void *), void* params);
void resource_heart_timer_deinit(struct recv_resource* r);


/*
0x80	服务器响应心跳数据包，回复限定5 秒内
0x81	服务器回应 认证报文
0x82	服务器发送 执行升级 指令
0x83	服务器发送 透传数据
0x84	服务器发送 强制重启 指令
0x85    服务器发送 主动探测 指令
*/
enum method_list_enum {
	mle_heart_response = 0x80,
	mle_authorization = 0x81,
	mle_firmware_upgrade = 0x82,
	mle_uart_transport = 0x83,
	mle_force_reboot = 0x84,
	mle_server_probe = 0x85,
};

#endif
