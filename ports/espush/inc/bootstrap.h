#ifndef _GUARD_H_BOOTSTRAP_H_
#define _GUARD_H_BOOTSTRAP_H_

#include "oswrap.h"

#define MAX_HOST_STRING 32

typedef struct _server_addr_s{
	char host[MAX_HOST_STRING];
	uint16 port;
	int8 use_tls;
} server_address;


int bootstrap(int is_test_env, struct _server_addr_s *addr);


#endif
