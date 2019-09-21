#ifndef _GUARD_H_ESPUSH_H_
#define _GUARD_H_ESPUSH_H_

#include "oswrap.h"

enum conn_state {
	CS_UNCONNECTED,
	CS_CONNECTING,
};

struct connection_state;
struct recv_resource;
typedef struct {
	int sock;
	uint8 txid;
	struct connection_state* state;
	struct recv_resource* res;
} espush_connection;


int ntp_sync(void);

int sock_task(void* params);

#endif
