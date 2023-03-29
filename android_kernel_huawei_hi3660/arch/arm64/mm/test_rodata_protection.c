/*
 * test_rodata_protection.c:Early rodata protection self test
 *
 * Copyright 2018 Huawei Technologies Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/hisi/hisi_hkip.h>
#include <asm/kernel-pgtable.h>
#include <asm/sections.h>
#include <asm/sizes.h>
#include <asm/tlb.h>
#include <asm/mmu_context.h>
#include "mm.h"

#define INIT_VALUE 0xAAAA
#define NEW_VALUE 0x5555

const u32 victim = INIT_VALUE;
const u32 control = INIT_VALUE;
volatile void *reference;
bool vict;
bool ctrl;

bool inline alter_const(const u32 *target)
{
	reference = (void *)target;
	barrier();
	*(u32 *)reference = NEW_VALUE;
	barrier();
	return (*(u32 *)reference == NEW_VALUE);
}

int early_rodata_debugfs(void)
{
	struct dentry *dir;

	dir = debugfs_create_dir("early_rodata", NULL);
	if (!dir) {
		pr_err("Failed to create debugfs dir\n");
		return -1;
	}
	debugfs_create_bool("control_test_passed", 0444, dir, &ctrl);
	debugfs_create_bool("victim_test_passed", 0444, dir, &vict);
	return 0;
}
module_init(early_rodata_debugfs);

void test_early_rodata_protection(void)
{
	unsigned long start = (unsigned long)__start_rodata;
	unsigned long end = (unsigned long)__init_begin;
	unsigned long section_size = (unsigned long)end - (unsigned long)start;

	/*
	 * mark .rodata as read only. Use __init_begin rather than __end_rodata
	 * to cover NOTES and EXCEPTION_TABLE.
	 */
	ctrl = alter_const(&control);
	hkip_register_ro((void *)start, ALIGN(section_size, PAGE_SIZE));
	vict = !alter_const(&victim);
	create_mapping_late(__pa_symbol(__start_rodata), start,
			    section_size, PAGE_KERNEL_RO);
}
EXPORT_SYMBOL(test_early_rodata_protection);
