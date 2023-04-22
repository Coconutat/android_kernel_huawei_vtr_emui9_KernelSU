/*
 * hw_kaslr.h
 *
 * Huawei Kernel Harden
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * Wang Zhitong <wangzhitong1@huawei.com>
 * Ma Yaohui <stesen.ma@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/io.h>
#include <asm/bugs.h>
#include <asm/setup.h>
#include <asm/sections.h>

#define STACK_RANDOMIZE_MAX        512
#define STACK_RANDOMIZE_STRONG_MAX 1024
#define STACK_ALIGN                16

static inline u64 kaslr_get_random(void)
{
	u64 cval;

	isb();
	asm volatile("mrs %0, cntvct_el0" : "=r" (cval));

	return cval;
}
