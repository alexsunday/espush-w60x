#ifndef _GUARD_H_UTILS_H_
#define _GUARD_H_UTILS_H_

#include "oswrap.h"

void bin2hex(const unsigned char *bin, size_t len, char* out);
size_t hexs2bin(const char *hex, unsigned char *out);

void get_imei(uint8* out);

#endif
