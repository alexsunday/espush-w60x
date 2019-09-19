#ifndef _GUARD_H_OS_WRAP_H_
#define _GUARD_H_OS_WRAP_H_

#include <rtthread.h>

// wrap os api, such as  sleep, read_file
#define espush_malloc rt_malloc
#define espush_free   rt_free
#define espush_memset rt_memset
#define espush_memcpy rt_memcpy
#define espush_strlen rt_strlen
#define espush_memmove rt_memmove

// wrap os struct, such as uint8

typedef rt_uint8_t    	uint8;
typedef rt_int8_t 			int8;
typedef rt_uint32_t			uint32;
typedef rt_uint16_t			uint16;

#endif
