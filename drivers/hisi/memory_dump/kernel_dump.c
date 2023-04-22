/*
 *  kernel/drivers/hisi/memory_dump/Kernel_dump.c
 *
 * balong memory/register proc-fs  dump implementation
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <asm/pgtable.h>
#include <linux/mm_types.h>
#include <linux/memblock.h>
#include <linux/percpu.h>
#include <linux/printk.h>
#include <asm/memory.h>
#include "kernel_dump.h"
#include <linux/hisi/mntn_dump.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/rculist.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <securec.h>
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG MEMORY_DUMP_TAG

/* the max size of mod->core_size is 4M */
#define MODULE_MAX_CORE_SIZE (4 * 1024 * 1024)

extern int pcpu_base_size;
extern struct mm_struct init_mm;
extern struct page *mem_map;
extern char _text[];
extern char _end[];
extern s64 phystart_addr;

struct kernel_dump_cb *g_kdump_cb;
static struct table_extra{
    u64 extra_mem_phy_base;
    u64 extra_mem_virt_base;
    u64 extra_mem_size;
} g_tbl_extra_mem[MAX_EXTRA_MEM] = {{0, 0} };
static unsigned int extra_index;
static DEFINE_RAW_SPINLOCK(g_kdump_lock);

static u32 checksum32(u32 *addr, u32 count)
{
	u64 sum = 0;
	u32 i;

	while (count > sizeof(u32) - 1) {
		/*  This is the inner loop */
		sum += *(addr++);
		count -= sizeof(u32);
	}

	if (count > 0) {
		u32 left = 0;

		i = 0;
		while (i <= count) {
			*((u8 *)&left + i) = *((u8 *)addr + i);
			i++;
		}

		sum += left;
	}

	while (sum>>32)
		sum = (sum & 0xffffffff) + (sum >> 32);

	return (~sum);
}

static int add_mem2table(u64 va, u64 pa, u64 size, bool need_crc)
{
	unsigned int i;
	bool crc_check = false;

	if ((pa == 0) || (va == 0) || (size == 0) || (extra_index >= MAX_EXTRA_MEM)) {
		return -1;
	}

	raw_spin_lock(&g_kdump_lock);
	/* Kernel dump is not inited */
	if (!g_kdump_cb) {
			g_tbl_extra_mem[extra_index].extra_mem_phy_base = pa;
			g_tbl_extra_mem[extra_index].extra_mem_virt_base = va;
			g_tbl_extra_mem[extra_index].extra_mem_size = size;
			extra_index++;
	} else {
		i = extra_index;
		if (i < MAX_EXTRA_MEM) {
			g_kdump_cb->extra_mem_phy_base[i] = pa;
			g_kdump_cb->extra_mem_virt_base[i] = va;
			g_kdump_cb->extra_mem_size[i] = size;
			extra_index += 1;

			crc_check = true;
		} else {
			pr_err("%s: extra memory(nums:%d) is out of range. \r\n", __func__, extra_index);
			goto err;
		}
	}

	if ((true == need_crc) && (true == crc_check)) {
		g_kdump_cb->crc = 0;
		g_kdump_cb->crc = checksum32((u32 *)g_kdump_cb, sizeof(struct kernel_dump_cb));
	}

	raw_spin_unlock(&g_kdump_lock);
	return 0;
err:
	raw_spin_unlock(&g_kdump_lock);
	return -1;
}

int add_extra_table(u64 pa, u64 size)
{
	return add_mem2table((u64)phys_to_virt(pa), pa, size, true);
}


int kernel_dump_init(void)
{
	unsigned int i, j;
	struct kernel_dump_cb *cb;
	struct memblock_type *print_mb_cb;
	if (register_mntn_dump(MNTN_DUMP_KERNEL_DUMP, sizeof(struct kernel_dump_cb), (void **)&cb)) {
		pr_err("%s: fail to get reserve memory\r\n", __func__);
		goto err;
	}
	pr_info("%s begin, 0x%llx!\r\n", __func__, (unsigned long long)cb);

	memset((void *)cb, 0, sizeof(struct kernel_dump_cb));
	cb->magic = KERNELDUMP_CB_MAGIC;
	cb->page_shift = PAGE_SHIFT;
	cb->struct_page_size = sizeof(struct page);
	cb->phys_offset = phystart_addr;
	cb->kernel_offset = kimage_vaddr;
	cb->page_offset = PAGE_OFFSET;/*lint !e648*/
	cb->extra_mem_phy_base[0] = virt_to_phys(_text);
	cb->extra_mem_virt_base[0] = (u64)_text;
	cb->extra_mem_size[0] = ALIGN(_end - _text, PAGE_SIZE);
	cb->extra_mem_phy_base[1] = virt_to_phys(pcpu_base_addr); /* per cpu info*/
	cb->extra_mem_virt_base[1] = (u64)pcpu_base_addr; /* per cpu info*/
	cb->extra_mem_size[1] = (u64)ALIGN(pcpu_base_size, PAGE_SIZE)*CONFIG_NR_CPUS;
	for (i = 2, j = 0; i < MAX_EXTRA_MEM && j < extra_index; i++, j++) {
		cb->extra_mem_phy_base[i] = g_tbl_extra_mem[j].extra_mem_phy_base;
		cb->extra_mem_virt_base[i] = g_tbl_extra_mem[j].extra_mem_virt_base;
		cb->extra_mem_size[i] = g_tbl_extra_mem[j].extra_mem_size;
	}
	extra_index = i;

	pr_info("_text:0x%pK _end:0x%pK\n", _text, _end);

	cb->page = mem_map;
	cb->pfn_offset = PHYS_PFN_OFFSET;
	cb->section_size = 0;
       /*Subtract the base address that TTBR1 maps*/
	cb->kern_map_offset = (UL(0xffffffffffffffff) << VA_BITS);/*lint !e648*/

	cb->ttbr = virt_to_phys(init_mm.pgd);

	cb->mb_cb = (struct memblock_type *)virt_to_phys(&memblock.memory);
	print_mb_cb = &memblock.memory;
	cb->mbr_size = sizeof(struct memblock_region);

	cb->text_kaslr_offset = (u64)_text - (KIMAGE_VADDR + TEXT_OFFSET); /*lint !e648*/
	cb->linear_kaslr_offset = __phys_to_virt(memblock_start_of_DRAM()) - PAGE_OFFSET;/*lint !e648*/

	pr_info("cb->page is 0x%llx\n", (unsigned long long)(cb->page));
	pr_info("cb->page_shift is 0x%x\n", cb->page_shift);
	pr_info("cb->struct_page_size is 0x%x\n", cb->struct_page_size);
	pr_info("cb->phys_offset is 0x%llx\n", cb->phys_offset);
	pr_info("cb->page_offset is 0x%llx\n", cb->page_offset);
	pr_info("cb->pfn_offset is 0x%llx\n", cb->pfn_offset);
	pr_info("cb->ttbr is ttbr:%pK\n", (void *)cb->ttbr);
	pr_info("cb->mb_cb is 0x%llx\n", (unsigned long long)(cb->mb_cb));
	pr_info("cb->section_size is 0x%llx\n", cb->section_size);
	pr_info("cb->pmd_size is 0x%llx\n", cb->pmd_size);
	pr_info("cb->mbr_size is 0x%llx\n", cb->mbr_size);
	pr_info("cb->kern_map_offset is 0x%llx\n", (unsigned long long)(cb->kern_map_offset));
	for (i = 0; i < print_mb_cb->cnt; i++) {
		pr_info("print_mb_cb->regions base is 0x%llx\n", (print_mb_cb->regions+i)->base);
		pr_info("print_mb_cb->regions size is 0x%llx\n", (print_mb_cb->regions+i)->size);
	}
	g_kdump_cb = cb;

	cb->crc = 0;
	cb->crc = checksum32((u32 *)cb, sizeof(struct kernel_dump_cb));

	return 0;
err:
	return -1;
}
early_initcall(kernel_dump_init);

