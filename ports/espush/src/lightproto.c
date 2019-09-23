
#include <rtthread.h>
#include <sys/socket.h>
#include <ulog.h>

#include "lightproto.h"
#include "espush.h"
#include "protoframe.h"
#include "drv.h"

const uint16 m_query = 0x01;
const uint16 m_set_line = 0x02;
const uint16 m_set_batch = 0x03;

/*
精简协议，只取 8 字节
*/
void deserialize_lightproto_frame(rt_uint8_t* data, struct lightproto* out)
{
  RT_ASSERT(data);
  RT_ASSERT(out);

  union uint16_writer w;
  rt_memcpy(w.data, data, 2);
  out->txid = ntohs(w.val);

  rt_memcpy(w.data, data + 2, 2);
  out->cmd = ntohs(w.val);

  rt_memcpy(out->data, data + 4, 4);
}

void serialize_lightproto_frame(struct lightproto* in, rt_uint8_t* out)
{
  RT_ASSERT(in);
  RT_ASSERT(out);

  union uint16_writer w;
  // txid
  w.val = htons(in->txid);
  rt_memcpy(out, w.data, 2);

  // cmd
  w.val = htons(in->cmd);
  rt_memcpy(out + 2, w.data, 2);

  // data
  rt_memcpy(out + 4, in->data, 4);
}

// 此函数不会被多线程调用，只会被 select 线程调用，故可以安全使用 static
void write_lightproto_frame(espush_connection* conn, struct lightproto* out)
{
  static char uart_buf[8];
  serialize_lightproto_frame(out, uart_buf);

	Frame *f = malloc_empty_frame(method_uart);
	if(!f) {
    LOG_W("malloc empty frame failed");
		return;
	}
	
  set_static_data(f, uart_buf, sizeof(uart_buf));
  write_frame(conn->sock, f);
}

/*
// 获取总回路数
int device_get_lines();
int device_get_lines_state(rt_bool_t *states, size_t max_size);
*/
void handle_proto_query(espush_connection* conn, struct lightproto* in)
{
  RT_ASSERT(conn);
  RT_ASSERT(in);

  uint32_t result = device_get_lines_state();
  struct lightproto out;
  out.txid = in->txid;
  out.cmd = in->cmd;
  
  union uint32_writer w;
  w.val = htonl(result);
  rt_memcpy(out.data, w.data, sizeof(w));
  write_lightproto_frame(conn, &out);
}

void handle_proto_set_line(espush_connection* conn, struct lightproto* in)
{
  RT_ASSERT(conn);
  RT_ASSERT(in);
  union uint16_writer w;

  // line
  rt_memcpy(w.data, in->data, 2);
  uint16_t line = ntohs(w.val);

  // state
  rt_memcpy(w.data, in->data + 2, 2);
  uint16_t state = ntohs(w.val);

  int rc = device_set_line_state(line, state);
  if(rc < 0) {
    LOG_W("device set line state failed. %d", rc);
  }

  write_lightproto_frame(conn, in);
}

// 全开/全关
void handle_proto_set_batch(espush_connection* conn, struct lightproto* in)
{
  RT_ASSERT(conn);
  RT_ASSERT(in);
  int i, rc;
  // state
  union uint16_writer w;
  rt_memcpy(w.data, in->data, 2);
  uint16_t state = ntohs(w.val);

  int size = device_get_lines();
  for(i=0; i!=size; ++i) {
    rc = device_set_line_state(i + 1, state);
    if(rc < 0) {
      LOG_W("device set line %d failed.", i + 1);
    }
  }

  write_lightproto_frame(conn, in);
}

/*
收到透传的串口数据，处理并返回
目前有
1，查询全部回路状态
2，设置某回路开关状态
3，设置全开、全关
*/
void handle_uart_buffer(espush_connection* conn, Frame* f)
{
  if(f->length != 8) {
    LOG_W("recv length not match %d", f->length);
    return;
  }

  struct lightproto in;
  deserialize_lightproto_frame(f->data, &in);
  if(in.cmd == m_query) {
    handle_proto_query(conn, &in);
  } else if(in.cmd == m_set_line) {
    handle_proto_set_line(conn, &in);
  } else if(in.cmd == m_set_batch) {
    handle_proto_set_batch(conn, &in);
  } else {
    LOG_W("unknown uart cmd. ");
  }
}
