/*
 * File      : mg_locals.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2017, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-08-19     armink       first version
 */
#ifndef _MONGOOSE_MG_LOCALS_H_
#define _MONGOOSE_MG_LOCALS_H_

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <lwip/sockets.h>
#include <dfs_posix.h>
#include <rtthread.h>

#if defined(PKG_MONGOOSE_ENABLE_DEBUG)
    #define CS_ENABLE_DEBUG                      1
#else
    #define CS_ENABLE_DEBUG                      0
#endif

#define CS_PLATFORM                              CS_P_STM32
#define CS_DEFINE_DIRENT                         0

#define MG_ENABLE_HTTP_SSI                       0

#define MG_NET_IF                                MG_NET_IF_SOCKET
#define MG_LWIP                                  1
#define RTOS_SDK                                 1

#define MG_MALLOC                                rt_malloc
#define MG_CALLOC                                rt_calloc
#define MG_REALLOC                               rt_realloc
#define MG_FREE                                  rt_free
#define MG_STRDUP                                rt_strdup
#define WEAK                                     RT_WEAK

#if defined(PKG_MONGOOSE_ENABLE_HTTP_WEBSOCKET)
    #define MG_ENABLE_HTTP_WEBSOCKET             1
#else
    #define MG_ENABLE_HTTP_WEBSOCKET             0
#endif

#if defined(PKG_MONGOOSE_ENABLE_FILESYSTEM)
    #define MG_ENABLE_FILESYSTEM                 1
#else
    #define MG_ENABLE_FILESYSTEM                 0
#endif

#if defined(PKG_MONGOOSE_ENABLE_HTTP_STREAMING_MULTIPART)
    #define MG_ENABLE_HTTP_STREAMING_MULTIPART   1
#else
    #define MG_ENABLE_HTTP_STREAMING_MULTIPART   0
#endif

#if defined(PKG_MONGOOSE_ENABLE_MQTT)
    #define MG_ENABLE_MQTT                       1
#else
    #define MG_ENABLE_MQTT                       0
#endif

#if defined(PKG_MONGOOSE_ENABLE_COAP)
    #define MG_ENABLE_COAP                       1
#else
    #define MG_ENABLE_COAP                       0
#endif

#if defined(PKG_MONGOOSE_ENABLE_HEXDUMP)
    #define MG_ENABLE_HEXDUMP                    1
#else
    #define MG_ENABLE_HEXDUMP                    0
#endif

#define fcntl(s,cmd,val)                         lwip_fcntl(s,cmd,val)

/**
 * Mongoose Library port initialization
 */
void mongoose_port_init(void);

#endif /* _MONGOOSE_MG_LOCALS_H_ */
