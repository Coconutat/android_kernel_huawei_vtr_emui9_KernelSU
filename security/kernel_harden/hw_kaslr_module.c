/*
 * Huawei Kernel Harden, module randomize
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

#include <linux/bitops.h>
#include <linux/elf.h>
#include <linux/gfp.h>
#include <linux/kasan.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/moduleloader.h>
#include <linux/vmalloc.h>
#include <linux/random.h>
#include <asm/alternative.h>
#include <asm/insn.h>
#include <asm/sections.h>

static unsigned long module_load_offset;
static int randomize_modules = 1;

static DEFINE_MUTEX(module_kaslr_mutex);

static int __init parse_nokaslr(char *p)
{
	randomize_modules = 0;

	return 0;
}
early_param("nokaslr", parse_nokaslr);

unsigned long get_module_load_offset(void)
{
	if (randomize_modules) {
		mutex_lock(&module_kaslr_mutex);
		if (module_load_offset == 0)
			module_load_offset =
				(get_random_int() % 1024 + 1) * PAGE_SIZE;
		mutex_unlock(&module_kaslr_mutex);
	}

	return module_load_offset;
}
