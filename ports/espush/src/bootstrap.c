
#include <rtthread.h>

#include <webclient.h>
#include <cJSON.h>

#include <ulog.h>

#include "bootstrap.h"
#include "utils.h"

const char* cloudURL = "http://light.espush.cn/api/portal/bootstrap";
const char* localURL = "http://192.168.2.107:8001/api/portal/bootstrap";

void show_address(struct _server_addr_s *addr)
{
	LOG_D("<%s:%d TLS:%d>", addr->host, addr->port, addr->use_tls);
}

int parse_response(const char* rsp, struct _server_addr_s *addr)
{
	cJSON *root, *j_address, *j_port, *j_tls;
	RT_ASSERT(rsp);
	RT_ASSERT(addr);

	root = cJSON_Parse(rsp);
	if(!root) {
		LOG_W("parse result failed.");
		return -1;
	}
	
	j_address = cJSON_GetObjectItem(root, "address");
	if(!j_address) {
		LOG_W("extract address field failed.");
		return -1;
	}
	j_port = cJSON_GetObjectItem(root, "port");
	if(!j_port) {
		LOG_W("extract port field failed.");
		return -1;
	}
	j_tls = cJSON_GetObjectItem(root, "use_tls");
	if(!j_tls) {
		LOG_W("extract use_tls field failed.");
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

extern const int response_authorization_fail;
int bootstrap(int is_test_env, struct _server_addr_s *addr)
{
	RT_ASSERT(addr);
	int rsp_code = 0;
	const char* baseURL;
	unsigned char *result = NULL;
	
	char imei_buf[24];
	char url_buf[256];
	get_imei((uint8*)imei_buf);

	if(is_test_env) {
		baseURL = localURL;
	} else {
		baseURL = cloudURL;
	}

	rt_snprintf(url_buf, sizeof(url_buf), "%s?chip_id=%s", baseURL, imei_buf);
	// LOG_D("%s", url_buf);
	int rc = webclient_request(url_buf, NULL, NULL, &result, &rsp_code);
	if(rc < 0) {
		LOG_W("espush bootstrap failed.", rc);
		if(rsp_code != 0) {
			// bootstrap failed.
			return response_authorization_fail;
		}
		return rc;
	}
	
	// LOG_D("response: [%s]", result);
	rc = parse_response((const char*)result, addr);
	if(rc < 0) {
		LOG_W("parse response failed. %d", rc);
		return rc;
	}
	
	web_free(result);
	return 0;
}
