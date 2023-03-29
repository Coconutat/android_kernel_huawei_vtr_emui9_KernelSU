/*
 * Huawei Kernel Harden, stack randomize strong
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
#include <linux/init_task.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include "hw_kaslr_stack.h"

unsigned long kti_offset;
EXPORT_SYMBOL(kti_offset);

void kti_randomize_init(void)
{
	kti_offset = (kaslr_get_random() %
			(STACK_RANDOMIZE_STRONG_MAX / STACK_ALIGN)) *
		STACK_ALIGN;
}

void set_init_thread_info(unsigned long addr)
{
	struct thread_info *ti;

	ti = (struct thread_info *)(addr + kti_offset);

	ti->task = &init_task;
	ti->flags = 0;
	ti->preempt_count = INIT_PREEMPT_COUNT;
	ti->addr_limit = KERNEL_DS;
}
