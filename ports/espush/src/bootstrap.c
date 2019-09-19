
#include <rtthread.h>

#include <webclient.h>
#include <cJSON.h>

#include "bootstrap.h"

const char* URL = "http://light.espush.cn/api/portal/light/bootstrap";

void show_address(struct _server_addr_s *addr)
{
	rt_kprintf("<%s:%d TLS:%d>", addr->host, addr->port, addr->use_tls);
}

int parse_response(const char* rsp, struct _server_addr_s *addr)
{
	cJSON *root, *j_address, *j_port, *j_tls;
	RT_ASSERT(rsp);
	RT_ASSERT(addr);

	root = cJSON_Parse(rsp);
	if(!root) {
		rt_kprintf("parse result failed.\r\n");
		return -1;
	}
	
	j_address = cJSON_GetObjectItem(root, "address");
	if(!j_address) {
		rt_kprintf("extract address field failed.\r\n");
		return -1;
	}
	j_port = cJSON_GetObjectItem(root, "port");
	if(!j_port) {
		rt_kprintf("extract port field failed.\r\n");
		return -1;
	}
	j_tls = cJSON_GetObjectItem(root, "use_tls");
	if(!j_tls) {
		rt_kprintf("extract use_tls field failed.\r\n");
		return -1;
	}
	
	int host_length = espush_strlen(j_address->valuestring);
	espush_memcpy(addr->host, j_address->valuestring, host_length);
	addr->host[host_length] = 0;
	
	addr->port = j_port->valueint;
	addr->use_tls = j_tls->valueint;
	
	cJSON_Delete(root);
	return 0;
}

int bootstrap(struct _server_addr_s *addr)
{
	RT_ASSERT(addr);
	unsigned char *result = NULL;
		
	int rc = webclient_request(URL, NULL, NULL, &result);
	if(rc < 0) {
		return rc;
	}
	
	rt_kprintf("response: [%s]\r\n", result);
	
	rc = parse_response((const char*)result, addr);
	if(rc < 0) {
		return rc;
	}
	
	web_free(result);
	return 0;
}


int test_bootstrap(void)
{
	rt_kprintf("prepare bootstrap.\r\n");
	
	struct _server_addr_s addr;
	int rc = bootstrap(&addr);
	if(rc < 0) {
		rt_kprintf("bootstrap returned -1\r\n");
	}
	show_address(&addr);
	
	return 0;
}

MSH_CMD_EXPORT(test_bootstrap, ESPush bootstrap test);

