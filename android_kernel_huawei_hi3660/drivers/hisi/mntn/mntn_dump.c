/*
 *
 * Copyright (C) 2012 Hisilicon, Inc.
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
 */
 #include <linux/stddef.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/compaction.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <mntn_public_interface.h>
#define HISI_LOG_TAG  MNTN_DUMP_TAG
#include <linux/hisi/hisi_log.h>
#include "blackbox/rdr_print.h"

static void __iomem *g_mntn_dump_base;
static unsigned long  g_mntn_dump_reserved_addr;
static unsigned int  	g_mntn_dump_init;
static unsigned int  	g_mntn_dump_size;
static struct mdump_head  	*g_mdump_head;

#define MNTN_DUMP_NOCLEAN (0xAA)
struct mntn_dump_mem_info{
	unsigned int size; // mntn dump region size
	unsigned int clean_flag; // clean flag
};
struct mntn_dump_mem_info g_mntn_dump_mem_size[MNTN_DUMP_MAX] = {
	{MNTN_DUMP_HEAD_SIZE,0},
	{MNTN_DUMP_ETR_SIZE,0},
	{MNTN_DUMP_KERNEL_DUMP_SIZE,0},
	{MNTN_DUMP_PANIC_SIZE,0},
	{MNTN_DUMP_FTRACE_SIZE,0},
	{MNTN_DUMP_PSTORE_RAMOOPS_SIZE,0},
	{MNTN_DUMP_BC_PANIC_SIZE,MNTN_DUMP_NOCLEAN}
};

static DEFINE_RAW_SPINLOCK(g_mdump_lock);
struct mdump_mem_info{
	unsigned int size;
	unsigned int offset;
	void __iomem *vaddr;
};
struct mdump_mem_info g_mdump_mem_info[MNTN_DUMP_MAX];

static int get_mntn_dump_reserve_addr(void)
{
	struct device_node *np;
	const char *name;
	int 	len;
	const unsigned long *p;

	np = of_find_node_by_path(DTS_MNTNDUMP_NAME);
	if (!np) {
		BB_PRINT_ERR("%s: dts node(%s) not found\n", __func__, DTS_MNTNDUMP_NAME);
		return -1;
	}

	/* check if status = ok, okay or status not defined*/
	name = of_get_property(np, "status", &len);
	if (name && strncmp(name, "okay", sizeof("okay")) != 0 && strncmp(name, "ok", sizeof("ok")) != 0){
		BB_PRINT_ERR("%s: get status(%.7s)  error\n", __func__, name);
		return -1;
	}

	p = (unsigned long *)of_get_property(np, "reg", NULL);
	if (!p) {
		BB_PRINT_ERR("%s: get reg fail, len =%d\n", __func__, len);
		goto err;
	}

	g_mntn_dump_reserved_addr = (u64)be64_to_cpup((const __be64 *)p);
	g_mntn_dump_size = (unsigned int)be64_to_cpup((const __be64 *) ++p) - MNTN_DUMP_KASLR_SIZE;
	if (!g_mntn_dump_reserved_addr || !g_mntn_dump_size) {
		BB_PRINT_ERR("get_mntn_dump_addr Error: addr: 0x%lx, size:0x%x\n",
			g_mntn_dump_reserved_addr, g_mntn_dump_size);
		goto err;
	}

	BB_PRINT_PN("get_mntn_dump_addr addr 0x%lx, size:0x%x\n",
		g_mntn_dump_reserved_addr, g_mntn_dump_size);
	of_node_put(np);
	return 0;
err:
	of_node_put(np);
	return -1;
}

int mntn_dump_init(void)
{
	int i;
	unsigned int offset;

	/* already initialized */
	if (g_mntn_dump_init) {
		return 0;
	}
	if (!g_mntn_dump_reserved_addr || !g_mntn_dump_size) {
		if (get_mntn_dump_reserve_addr()) {
			BB_PRINT_ERR("%s: reserve addr is NULL\n", __func__);
			goto err;
		}
	}

	g_mntn_dump_base = (void *) ioremap_wc((phys_addr_t)g_mntn_dump_reserved_addr, g_mntn_dump_size);
	if (!g_mntn_dump_base) {
		BB_PRINT_ERR("%s: ioremap error\n", __func__);
		goto err;
	}
	BB_PRINT_PN("%s: mntn dump base addr:%pK\n", __func__, g_mntn_dump_base);
	/* clean the memory of information struct  */
	memset((void *)g_mdump_mem_info, 0x00, sizeof(g_mdump_mem_info));


	offset = 0;
	for (i = 0; i < MNTN_DUMP_MAX; i++) {
		g_mdump_mem_info[i].offset = offset;
		g_mdump_mem_info[i].size = g_mntn_dump_mem_size[i].size;
		g_mdump_mem_info[i].vaddr = g_mntn_dump_base + offset;

		/*clean the reserve memory of mntn dump*/
		if (MNTN_DUMP_NOCLEAN != g_mntn_dump_mem_size[i].clean_flag)
			memset((void *)g_mdump_mem_info[i].vaddr, 0x00, g_mdump_mem_info[i].size);

		offset += g_mdump_mem_info[i].size;
		if (offset >= MNTN_DUMP_MAXSIZE) {
			BB_PRINT_ERR("%s: mntn dump size is out of range\n", __func__);
			goto err;
		}
	}

	/* init head information */
	g_mdump_head = (struct mdump_head  *)g_mdump_mem_info[MNTN_DUMP_HEAD].vaddr;
	g_mdump_head->magic = MNTNDUMP_MAGIC;
	g_mdump_head->version = MNTN_DUMP_VERSION;
	g_mdump_head->regs_info[0].mid = MNTN_DUMP_HEAD;
	g_mdump_head->regs_info[0].size = g_mdump_mem_info[MNTN_DUMP_HEAD].size;
	g_mdump_head->regs_info[0].offset = g_mdump_mem_info[MNTN_DUMP_HEAD].offset;
	g_mdump_head->nums = 1;

	g_mntn_dump_init = 1;
	return 0;
err:
	return -1;
}

early_initcall(mntn_dump_init);

/*
Func: register_mntn_dump()
	register a mntn_dump module, Output the virtual address.
Input:
	@mod_id: module ID
	@size:	 the actual size of mntn dump module
Output:
	@vaddr:  output the virtaul addr of reserved memory
return:
	0: OK; Others: Fail;
*/
int register_mntn_dump(int mod_id, unsigned int size, void **vaddr)
{
	int i;
	char **ptr;

	if (!vaddr) {
		BB_PRINT_ERR("%s: module id[%d], vaddr is NULL\n", __func__, mod_id);
		return -1;
	}

	ptr = (char **)vaddr;
	*ptr = 0;

	if (mod_id < MNTN_DUMP_HEAD || mod_id >= MNTN_DUMP_MAX) {
		BB_PRINT_ERR("%s: module id[%d] is invalid\n", __func__, mod_id);
		return -1;
	}

	if (!g_mntn_dump_init) {
		if (mntn_dump_init()) {
			BB_PRINT_ERR("%s: module id[%d] fail\n", __func__, mod_id);
			return -1;
		}
	}
	raw_spin_lock(&g_mdump_lock);
	if (g_mdump_mem_info[mod_id].size < size) {
		BB_PRINT_ERR("%s: module[%d] size(0x%x) is invalid\n", __func__, mod_id, size);
		goto err;
	}

	i = g_mdump_head->nums;
	if (i >= MNTN_DUMP_MAX) {
		BB_PRINT_ERR("%s: mntn dump data corruption(nums: %d)\n", __func__, i);
		goto err;
	}
	g_mdump_head->regs_info[i].mid = mod_id;
	g_mdump_head->regs_info[i].size = g_mdump_mem_info[mod_id].size;
	g_mdump_head->regs_info[i].offset = g_mdump_mem_info[mod_id].offset;
	g_mdump_head->nums += 1;

	*ptr = (char *)g_mdump_mem_info[mod_id].vaddr;
	raw_spin_unlock(&g_mdump_lock);
	return 0;
err:
	raw_spin_unlock(&g_mdump_lock);
	return -1;
}
