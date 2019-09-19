#ifndef _GUARD_H_LIGHT_PROTO_H_
#define _GUARD_H_LIGHT_PROTO_H_

#include "rtdef.h"

/*
2字节 唯一识别码  2字节 命令枚举 剩余 4 字节内容区
查询全部回路开关状态，后四字节有效，所以最多允许 32 路
请求：FE FE 00 01
响应：FE FE 00 01 00 00 00 01
设置某回路开关状态 回路1为开
请求：FE FE 00 02 00 01 00 01
响应：FE FE 00 02 00 00 00 01
设置设备全开，全关
请求：FE FE 00 03 00 01/00
响应：FE FE 00 03 00 01/00
查询某回路调光状态
设置某回路调光状态
*/

struct lightproto {
    uint16_t txid;
    uint16_t cmd;
    char data[4];
};


#endif
