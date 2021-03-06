#include <rtthread.h>
#include <finsh.h>
#include <sys/socket.h>
#include <dfs_posix.h>
#include <ulog.h>

#include "protoframe.h"

const int MAX_FRAME_LEFT = 2048;

Frame* malloc_empty_frame(uint8 method)
{
	Frame *rsp = espush_malloc(sizeof(Frame));
	if(!rsp) {
		return NULL;
	}
	
	espush_memset(rsp, 0, sizeof(Frame));
	
	rsp->data_flag = FRAME_STATIC_DATA;
	rsp->method = method;
	rsp->data = NULL;
	
	return rsp;
}

Frame* new_login_frame(char* imei)
{
	static char body_buf[64];
	Frame* out = malloc_empty_frame(method_login);
	if(!out) {
		return NULL;
	}
	
	sprintf(body_buf, "{\"imei\":\"%s\",\"method\":\"LOGIN\"}", imei);
	int body_length = espush_strlen(body_buf);
	set_static_data(out, (uint8*)body_buf, body_length);
	
	return out;
}

int frame_length(Frame *f)
{
	RT_ASSERT(f);
	return 2 + 1 + 1 + f->length;
}

void set_dyn_data(Frame *f, uint8* data, size_t d_length)
{
	RT_ASSERT(f);
	RT_ASSERT(data);
	RT_ASSERT(d_length);
	RT_ASSERT(!f->data);

	f->data = data;
	f->length = d_length;
	f->data_flag = FRAME_DYN_DATA;
}

void set_static_data(Frame *f, uint8* data, size_t d_length)
{
	RT_ASSERT(f);
	RT_ASSERT(data);
	RT_ASSERT(d_length);
	RT_ASSERT(!f->data);
	
	f->data = data;
	f->length = d_length;
	f->data_flag = FRAME_STATIC_DATA;
}

void free_frame(Frame *f)
{
	RT_ASSERT(f);
	if(f->data && f->data_flag == FRAME_DYN_DATA) {
		espush_free(f->data);
		f->data = NULL;
	}
	
	espush_free(f);
}

int serialize_frame(Frame *f, uint8* out, size_t out_max_length)
{
	RT_ASSERT(f);
	RT_ASSERT(out);
	size_t length = 1 + 1 + f->length;

	RT_ASSERT(out_max_length >= (length + 2));
	union uint16_writer w;
	w.val = htons(length);
	
	espush_memcpy(out, w.data, sizeof(union uint16_writer));
	out[2] = f->method;
	out[3] = f->txid;
	espush_memcpy(out + 4, f->data, f->length);

	return length + 2;
}

void serialize_f(Frame *f, uint8* out)
{
	size_t l = 2 + 1 + 1 + f->length;
	serialize_frame(f, out, l);
}

int deserialize_frame(uint8* in_data, size_t in_length, Frame *out)
{
	RT_ASSERT(in_data);
	RT_ASSERT(out);
	RT_ASSERT(in_length > 2);
	
	union uint16_writer w;
	espush_memcpy(w.data, in_data, 2);
	int left = ntohs(w.val);
	
	// 2 + left == frame length, +1 for null;
	if(in_length < (2 + left + 1)) {
		LOG_W("in_length not enough, %d, %d.", in_length, left);
		return -1;
	}
	if(left > MAX_FRAME_LEFT) {
		LOG_W("too big frame left %d.", left);
		return -1;
	}
	
	out->method = in_data[2];
	out->txid = in_data[3];
	
	out->length = left - 2;
	if(!out->length) {
		// 如果数据包无 body，则无需 free，也无需malloc
		out->data_flag = FRAME_STATIC_DATA;
	} else {
		out->data_flag = FRAME_DYN_DATA;
		out->data = espush_malloc(left - 2);
		if(!out->data) {
			LOG_W("espush malloc failed. %d", left);
			return -1;
		}
	}
	
	espush_memcpy(out->data, in_data + 4, left - 2);
	out->data[left-2] = 0;
	
	return 0;
}

int _write_frame_buf(int fd, Frame* f, uint8* buf, size_t buf_length)
{
	int rc = serialize_frame(f, buf, buf_length);
	if(rc < 0) {
		LOG_W("serialize failed.");
		return -1;
	}

	return send(fd, buf, rc, 0);	
}

int write_frame(int fd, Frame* f)
{
	RT_ASSERT(fd>0);
	RT_ASSERT(f);
	
	int length = frame_length(f);
	if(length > MAX_FRAME_LEFT) {
		LOG_W("too large frame, cannot serialize.");
		return -1;
	}
	
	uint8* buf = espush_malloc(length);
	if(!buf) {
		LOG_W("cannot malloc, memory not enough.");
		return -1;
	}
	
	int rc = _write_frame_buf(fd, f, buf, length);
	espush_free(buf);
	return rc;
}

int recv_frame(int fd, Frame *f)
{
	RT_ASSERT(fd > 0);
	RT_ASSERT(f);
	// must be unused frame.
	RT_ASSERT(!f->data);
	
	char head_buf[4];
	
	// TODO: recv recycle.
	int rc = recv(fd, head_buf, sizeof(head_buf), 0);
	if(rc < 0 || rc != sizeof(head_buf)) {
		LOG_W("recv head failed, %d.", rc);
		return -1;
	}
	
	union uint16_writer w;
	espush_memcpy(w.data, head_buf, 2);
	int left = ntohs(w.val);
	LOG_D("recv left %d.", left);
	
	// all frame, include header. extra +1 for string NULL suffix.
	int length = left + 2 + 1;
	uint8* frame_buf = espush_malloc(length);
	if(!frame_buf) {
		LOG_W("malloc for frame body failed.");
		return -1;
	}
	
	// be careful, memory leak.
	espush_memcpy(frame_buf, head_buf, sizeof(head_buf));
	rc = recv(fd, frame_buf + 4, left - 2, 0);
	if(rc != (left - 2)) {
		LOG_W("recv body failed %d.", rc);
		espush_free(frame_buf);
		return -1;
	}
	
	rc = deserialize_frame(frame_buf, length, f);
	if(rc < 0) {
		LOG_W("deserialize frame failed.");
		espush_free(frame_buf);
		return -1;
	}
	
	return rc;
}


Buffer* malloc_buffer(size_t len)
{
	Buffer* out = (Buffer*)espush_malloc(sizeof(Buffer));
	if(!out) {
		return NULL;
	}
	
	out->buffer = (uint8*)espush_malloc(len);
	if(!out->buffer) {
		espush_free(out);
		return NULL;
	}
	out->len = 0;
	out->cap = len;
	return out;
}

void free_buffer(Buffer* buf)
{
	RT_ASSERT(buf && buf->buffer && buf->cap);
	espush_free(buf->buffer);
	buf->buffer = NULL;
	buf->cap = 0;
	buf->len = 0;
	espush_free(buf);
}

int buffer_add_data(Buffer* buf, uint8* ptr, size_t len)
{
	RT_ASSERT(buf && buf->buffer && buf->cap);
	RT_ASSERT(ptr);
	RT_ASSERT(len);
	
	if((buf->len + len) > buf->cap) {
		LOG_W("buffer full, len: [%d], cap: [%d], cur: [%d]", buf->len, buf->cap, len);
		return -1;
	}
	
	espush_memcpy(buf->buffer + buf->len, ptr, len);
	buf->len += len;
	return buf->len;
}

int buffer_try_extract(Buffer* buf)
{
	RT_ASSERT(buf && buf->buffer && buf->cap);
	if(buf->len < 4) {
		return 0;
	}
	
	union uint16_writer w;
	espush_memcpy(w.data, buf->buffer, 2);
	int left = ntohs(w.val);
	if(buf->len >= (left + 2)) {
		return left + 2;
	}
	
	return 0;
}

void buffer_shrink(Buffer* buf, size_t len)
{
	RT_ASSERT(buf && buf->buffer && buf->cap);
	RT_ASSERT(len <= buf->len);
	
	size_t w = buf->len - len;
	espush_memmove(buf->buffer, buf->buffer + len, w);
	buf->len = w;
	buf->buffer[w] = 0;
}

// 1 => full, 0 => not full.
int buffer_is_full(Buffer* buf)
{
	if((buf->len + 4) >= buf->cap) {
		return 1;
	}
	
	return 0;
}


// connection state;
struct connection_state* state_connection_malloc(void)
{
	struct connection_state* out = espush_malloc(sizeof(struct connection_state));
	if(!out) {
		return NULL;
	}
	
	out->lock = rt_mutex_create("state", RT_IPC_FLAG_PRIO);
	if(!out->lock) {
		espush_free(out);
		return NULL;
	}

	out->state = cs_unconnected;
	
	return out;
}

void state_connection_free(struct connection_state* s)
{
	RT_ASSERT(s);
	RT_ASSERT(s->lock);
	
	rt_mutex_delete(s->lock);
	s->lock = NULL;
	s->state = cs_unconnected;
	espush_free(s);
}

int state_get_current(struct connection_state* s)
{
	RT_ASSERT(s);
	RT_ASSERT(s->lock);
	
	rt_mutex_take(s->lock, RT_WAITING_FOREVER);
	int rc = s->state;
	rt_mutex_release(s->lock);
	return rc;
}

void state_set(struct connection_state* s, int state)
{
	RT_ASSERT(s);
	RT_ASSERT(s->lock);
	
	rt_mutex_take(s->lock, RT_WAITING_FOREVER);
	s->state = state;
	rt_mutex_release(s->lock);	
}

int resource_pipe_init(struct recv_resource* r)
{
	RT_ASSERT(r);
	char devname[32];
	static int pipeno = 0;
	rt_pipe_t *p;
	
	snprintf(r->pipename, sizeof(r->pipename), "pipe%d", pipeno++);
	p = rt_pipe_create(r->pipename, PIPE_BUFSZ);
	if(!p) {
		LOG_W("create pipe failed.");
		return -1;
	}
	r->p = p;
	
	snprintf(devname, sizeof(devname), "/dev/%s", r->pipename);
	r->read_fd = open(devname, O_RDONLY, 0);
	if(r->read_fd < 0) {
		LOG_W("open read pipe failed.");
		rt_pipe_delete(r->pipename);
		return -1;
	}
	
	r->write_fd = open(devname, O_WRONLY, 0);
	if(r->write_fd < 0) {
		LOG_W("open write pipe failed.");
		rt_pipe_delete(r->pipename);
		close(r->read_fd);
		return -1;
	}

	return 0;
}

void resource_pipe_deinit(struct recv_resource* r)
{
	RT_ASSERT(r);
	int rc;
	
	rc = close(r->read_fd);
	if(rc < 0) {
		LOG_W("close read pipe fd failed.");
	}
	
	rc = close(r->write_fd);
	if(rc < 0) {
		LOG_W("close read pipe fd failed.");
	}
	
	rt_pipe_delete(r->pipename);
}


int resource_heart_timer_init(struct recv_resource* r, void (*timeout)(void *), void* params)
{
	RT_ASSERT(r);
	RT_ASSERT(!r->heart);
	RT_ASSERT(timeout);
	
	uint8 flag = RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER;
	r->heart = rt_timer_create("heart", timeout, params, 30 * RT_TICK_PER_SECOND, flag);
	if(!r->heart) {
		return -1;
	}
	
	rt_timer_start(r->heart);
	return 0;
}

void resource_heart_timer_deinit(struct recv_resource* r)
{
	rt_timer_stop(r->heart);
	rt_timer_delete(r->heart);
	r->heart = NULL;
}

// resource.
struct recv_resource* resource_malloc(void)
{
	struct recv_resource* out = (struct recv_resource*)espush_malloc(sizeof(struct recv_resource));
	if(!out) {
		return NULL;
	}
	
	espush_memset(out, 0, sizeof(struct recv_resource));
	out->buffer = malloc_buffer(MAX_RECV_BUFFER_LENGTH);
	if(!out->buffer) {
		espush_free(out);
		return NULL;
	}

	int rc = resource_pipe_init(out);
	if(rc < 0) {
		free_buffer(out->buffer);
		out->buffer = NULL;
		espush_free(out);
		out = NULL;
		return NULL;
	}
	
	return out;
}

void resource_free(struct recv_resource* r)
{
	RT_ASSERT(r);
	RT_ASSERT(r->buffer);
	
	// free timer
	if(r->heart) {
		resource_heart_timer_deinit(r);
	}
	
	// free pipe
	resource_pipe_deinit(r);
	
	// free buffer
	free_buffer(r->buffer);
	
	// free this
	espush_free(r);
}
