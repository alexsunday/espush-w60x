
#include <wm_type_def.h>
#include <wm_efuse.h>
#include <ulog.h>

#include "utils.h"
#include "oswrap.h"


void bin2hex(const unsigned char *bin, size_t len, char* out)
{
	size_t  i;

	if (bin == NULL || len == 0) {
		return;
	}

	for (i=0; i<len; i++) {
		out[i*2]   = "0123456789ABCDEF"[bin[i] >> 4];
		out[i*2+1] = "0123456789ABCDEF"[bin[i] & 0x0F];
	}
	out[len*2] = '\0';
}

int hexchr2bin(const char hex, char *out)
{
	if (out == NULL)
		return 0;

	if (hex >= '0' && hex <= '9') {
		*out = hex - '0';
	} else if (hex >= 'A' && hex <= 'F') {
		*out = hex - 'A' + 10;
	} else if (hex >= 'a' && hex <= 'f') {
		*out = hex - 'a' + 10;
	} else {
		return 0;
	}

	return 1;
}

size_t hexs2bin(const char *hex, unsigned char *out)
{
	size_t len;
	char   b1;
	char   b2;
	size_t i;

	if (hex == NULL || *hex == '\0' || out == NULL)
		return 0;

	len = espush_strlen(hex);
	if (len % 2 != 0)
		return 0;
	len /= 2;

	espush_memset(out, 'A', len);
	for (i=0; i<len; i++) {
		if (!hexchr2bin(hex[i*2], &b1) || !hexchr2bin(hex[i*2+1], &b2)) {
			return 0;
		}
		
		out[i] = (b1 << 4) | b2;
	}
	
	return len;
}

void show_raw(char* buf, int len)
{
	int i;
	for(i=0; i!=len; ++i) {
		rt_kprintf("%02X ", (uint8_t)buf[i]);
	}
	rt_kprintf("\r\n");
}

void get_mac_addr(void)
{
	char mac[6];
	tls_get_mac_addr(mac);
	show_raw(mac, sizeof(mac));
}

void get_chipid(void)
{
	int ret;
	uint8_t chipbuf[16];
	rt_memset(chipbuf, 0, sizeof(chipbuf));
	ret = tls_get_chipid(chipbuf);

	LOG_D("ret: [%d]", ret);
	show_raw(chipbuf, sizeof(chipbuf));
}

void get_imei(uint8* out)
{
	int i;
	char mac[6];
	const char* prefix = "W60X";

	rt_memset(mac, 0, sizeof(mac));
	rt_strncpy(out, prefix, sizeof(prefix));
	tls_get_mac_addr(mac);
	for(i=0; i!=sizeof(mac); ++i) {
		sprintf(out + 4 + (2 * i), "%02X", (uint8_t)mac[i]);
	}
	out[sizeof(mac) * 2 + rt_strlen(prefix)] = 0;
}

void show_imei(void)
{
	char imei[24];
	get_imei(imei);
	rt_kprintf("%s\r\n", imei);
}

MSH_CMD_EXPORT(show_imei, get_chipid and show.);
