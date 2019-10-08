
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


int webclient_request2(const char *URI, const char *header, const char *post_data, unsigned char **response, int *rsp_code)
{
    struct webclient_session *session;
    int rc = WEBCLIENT_OK;
    int totle_length;

    RT_ASSERT(URI);
    RT_ASSERT(rsp_code);

    if (post_data == RT_NULL && response == RT_NULL)
    {
        LOG_E("request get failed, get response data cannot be empty.");
        return -1;
    }

    if (post_data == RT_NULL)
    {
        /* send get request */
        session = webclient_session_create(WEBCLIENT_HEADER_BUFSZ);
        if (session == RT_NULL)
        {
            rc = -1;
            goto __exit;
        }

        if (header != RT_NULL)
        {
            char *header_str, *header_ptr;
            int header_line_length;

            for(header_str = (char *)header; (header_ptr = rt_strstr(header_str, "\r\n")) != RT_NULL; )
            {
                header_line_length = header_ptr + rt_strlen("\r\n") - header_str; 
                webclient_header_fields_add(session, "%.*s", header_line_length, header_str);
                header_str += header_line_length;
            }
        }

        *rsp_code = webclient_get(session, URI);
        if (*rsp_code != 200)
        {
            rc = -1;
            goto __exit;
        }

        totle_length = webclient_response(session, response);
        if (totle_length <= 0)
        {
            rc = -1;
            goto __exit;
        }
    }
    else
    {
        /* send post request */
        session = webclient_session_create(WEBCLIENT_HEADER_BUFSZ);
        if (session == RT_NULL)
        {
            rc = -1;
            goto __exit;
        }

        if (header != RT_NULL)
        {
            char *header_str, *header_ptr;
            int header_line_length;

            for(header_str = (char *)header; (header_ptr = rt_strstr(header_str, "\r\n")) != RT_NULL; )
            {
                header_line_length = header_ptr + rt_strlen("\r\n") - header_str; 
                webclient_header_fields_add(session, "%.*s", header_line_length, header_str);
                header_str += header_line_length;
            }
        }

        if (rt_strstr(session->header->buffer, "Content-Length") == RT_NULL)
        {
            webclient_header_fields_add(session, "Content-Length: %d\r\n", rt_strlen(post_data));
        }

        if (rt_strstr(session->header->buffer, "Content-Type") == RT_NULL)
        {
            webclient_header_fields_add(session, "Content-Type: application/octet-stream\r\n");
        }

        *rsp_code = webclient_post(session, URI, post_data);
        if (*rsp_code != 200)
        {
            rc = -1;
            goto __exit;
        }
        
        totle_length = webclient_response(session, response);
        if (totle_length <= 0)
        {
            rc = -1;
            goto __exit;
        }
    }

__exit:
    if (session)
    {
        webclient_close(session);
        session = RT_NULL;
    }

    if (rc < 0)
    {
        return rc;
    }

    return totle_length;
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
	int rc = webclient_request2(url_buf, NULL, NULL, &result, &rsp_code);
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
