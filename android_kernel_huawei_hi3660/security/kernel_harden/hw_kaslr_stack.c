/*
 * Huawei Kernel Harden, stack randomize
 *
 * Copyright (c) 2016 Huawei.
 *
 * Authors:
 * Wang Zhitong <wangzhitong1@huawei.com>
 * Ma Yaohui <stesen.ma@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include "hw_kaslr_stack.h"

unsigned int kstack_offset;
EXPORT_SYMBOL(kstack_offset);

void kstack_randomize_init(void)
{
	kstack_offset = (kaslr_get_random() %
			(STACK_RANDOMIZE_MAX / STACK_ALIGN)) *
		STACK_ALIGN;
}
