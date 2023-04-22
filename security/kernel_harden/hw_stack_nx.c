/*
 * Huawei Kernel Harden, stack nx
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
#include <asm/cacheflush.h>

void __init set_init_stack_nx(struct thread_info *ti)
{
	set_memory_nx((unsigned long)ti, THREAD_SIZE/PAGE_SIZE);
}

void set_task_stack_nx(struct thread_info *ti)
{
	set_memory_nx((unsigned long)ti, THREAD_SIZE/PAGE_SIZE);
}
