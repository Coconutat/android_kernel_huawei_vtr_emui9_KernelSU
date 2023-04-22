/*
 * blackbox header file (blackbox: kernel run data recorder.)
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __RDR_TYPES_H__
#define __RDR_TYPES_H__

#include <linux/types.h>

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

#define RDR_INT     (long long)
#define RDR_PTR     (void*)
#define RDR_NVE     u64

#define RDR_TRUE    1
#define RDR_FALSE   0

#define RDR_UINT_INVALID 0xFFFFFFFF

#endif/* End #define __RDR_TYPES_H__ */

