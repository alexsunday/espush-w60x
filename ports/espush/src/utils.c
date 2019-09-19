
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


void get_imei(uint8* out)
{
	const char* imei = "169300033622123";
	int length = espush_strlen(imei);
	espush_memcpy(out, imei, length);
	out[length] = 0;
}
