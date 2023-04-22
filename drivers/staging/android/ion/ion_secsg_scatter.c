/*
 * ion_secsg_scatter.c
 *
 * Copyright (C) 2018 Hisilicon, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) "secsg: " fmt

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/genalloc.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/cma.h>
#include <linux/sizes.h>
#include <linux/list.h>
#include <linux/version.h>
#include <linux/hisi/hisi_cma.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/hisi/hisi_mm.h>

#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

#include "ion.h"
#include "ion_priv.h"
#include "hisi/ion_sec_priv.h"

static int change_scatter_prop(struct ion_secsg_heap *secsg_heap,
			       struct ion_buffer *buffer,
			       u32 cmd)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	struct sg_table *table = buffer->priv_virt;
#else
	struct sg_table *table = buffer->sg_table;
#endif
	struct scatterlist *sg;
	struct page *page;
	struct mem_chunk_list mcl;
	struct tz_pageinfo *pageinfo = NULL;
	unsigned int nents = table->nents;
	int ret = 0;
	u32 i;

	if (!secsg_heap->TA_init) {
		pr_err("[%s] TA not inited.\n", __func__);
		return -EINVAL;
	}

	if (cmd == ION_SEC_CMD_ALLOC) {
		pageinfo = kcalloc(nents, sizeof(*pageinfo), GFP_KERNEL);
		if (!pageinfo)
			return -ENOMEM;

		for_each_sg(table->sgl, sg, table->nents, i) {
			page = sg_page(sg);
			pageinfo[i].addr = page_to_phys(page);
			pageinfo[i].nr_pages = sg->length / PAGE_SIZE;
		}

		mcl.phys_addr = (void *)pageinfo;
		mcl.nents = nents;
		mcl.protect_id = SEC_TASK_DRM;
	} else if (cmd == ION_SEC_CMD_FREE) {
		mcl.protect_id = SEC_TASK_DRM;
		mcl.buff_id = buffer->id;
		mcl.phys_addr = NULL;
	} else {
		pr_err("%s: Error cmd\n", __func__);
		return -EINVAL;
	}

	ret = secmem_tee_exec_cmd(secsg_heap->session, &mcl, cmd);
	if (ret) {
		pr_err("%s:exec cmd[%d] fail\n", __func__, cmd);
		ret = -EINVAL;
	} else {
		if (cmd == ION_SEC_CMD_ALLOC)
			buffer->id = mcl.buff_id;
	}

	if (pageinfo)
		kfree(pageinfo);

	return ret;
}

static struct page *__secsg_alloc_large(struct ion_secsg_heap *secsg_heap,
					struct ion_buffer *buffer,
					unsigned long size,
					unsigned long align)
{
	struct page *page;
	int count = 0;
#ifdef CONFIG_HISI_KERNELDUMP
	int k;
	struct page *tmp_page = NULL;
#endif

	count = size / PAGE_SIZE;
	page = cma_alloc(secsg_heap->cma, count, get_order(align));
	if (!page) {
		pr_err("alloc cma fail, count:0x%x\n", count);
		return NULL;
	}

#ifdef CONFIG_HISI_KERNELDUMP
	tmp_page = page;
	for (k = 0; k < count; k++) {
		SetPageMemDump(tmp_page);
		tmp_page++;
	}
#endif
	return page;
}

int __secsg_alloc_scatter(struct ion_secsg_heap *secsg_heap,
			  struct ion_buffer *buffer, unsigned long size)
{
	struct sg_table *table;
	struct scatterlist *sg;
	struct page *page;
	unsigned long per_bit_sz = secsg_heap->per_bit_sz;
	unsigned long size_remaining = ALIGN(size, per_bit_sz);
	unsigned long alloc_size = 0;
	unsigned long nents = ALIGN(size, SZ_2M) / SZ_2M;
	int ret = 0;
	u32 i = 0;

	secsg_debug("%s: enter, ALIGN size 0x%lx\n", __func__, size_remaining);
	table = kzalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		return -ENOMEM;

	if (sg_alloc_table(table, nents, GFP_KERNEL))
		goto free_table;

	/*
	 * DRM memory alloc from CMA pool.
	 * In order to speed up the allocation, we will apply for memory
	 * in units of 2MB, and the memory portion of less than 2MB will
	 * be applied for one time.
	 */
	sg = table->sgl;
	while (size_remaining) {
		if (size_remaining > SZ_2M)
			alloc_size = SZ_2M;
		else
			alloc_size = size_remaining;

		page = __secsg_alloc_large(secsg_heap, buffer,
					   alloc_size, per_bit_sz);
		if (!page) {
			pr_err("%s: alloc largest available failed!\n",
			       __func__);
			goto free_pages;
		}

		/*
		 * Before set memory in secure region, we need change kernel
		 * pgtable from normal to device to avoid big CPU Speculative
		 * read.
		 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
		create_mapping_late(page_to_phys(page),
				    (unsigned long)page_address(page),
				    alloc_size, __pgprot(PROT_DEVICE_nGnRE));
#else
		change_secpage_range(page_to_phys(page),
				     (unsigned long)page_address(page),
				     alloc_size, __pgprot(PROT_DEVICE_nGnRE));
#endif
		size_remaining -= alloc_size;
		sg_set_page(sg, page, alloc_size, 0);
		sg = sg_next(sg);
		i++;
	}

	/*
	 * After change the pgtable prot, we need flush TLB and cache.
	 */
	flush_tlb_all();
	ion_flush_all_cpus_caches();

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	buffer->priv_virt = table;
#else
	buffer->sg_table = table;
#endif

	/*
	 * Send cmd to secos change the memory form normal to protect,
	 * and record some information of the sg_list for secos va map.
	 */
	ret = change_scatter_prop(secsg_heap, buffer, ION_SEC_CMD_ALLOC);
	if (ret)
		goto free_pages;

	secsg_debug("%s: exit\n", __func__);

	return 0;
free_pages:
	nents = i;
	pr_err("free %ld pages in err runtime\n", nents);
	for_each_sg(table->sgl, sg, nents, i) {
		page = sg_page(sg);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
		create_mapping_late(page_to_phys(page),
				    (unsigned long)page_address(page),
				    sg->length, PAGE_KERNEL);
#else
		change_secpage_range(page_to_phys(page),
				     (unsigned long)page_address(page),
				     sg->length, PAGE_KERNEL);
#endif
		flush_tlb_all();
		cma_release(secsg_heap->cma, page, sg->length / PAGE_SIZE);
	}
	flush_tlb_all();
	sg_free_table(table);
free_table:
	kfree(table);

	return -ENOMEM;
}

void __secsg_free_scatter(struct ion_secsg_heap *secsg_heap,
			  struct ion_buffer *buffer)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	struct sg_table *table = buffer->priv_virt;
#else
	struct sg_table *table = buffer->sg_table;
#endif
	struct scatterlist *sg;
	int ret = 0;
	u32 i;

	ret = change_scatter_prop(secsg_heap, buffer, ION_SEC_CMD_FREE);
	if (ret) {
		pr_err("release MPU protect fail! Need check DRM runtime\n");
		return;
	}

	for_each_sg(table->sgl, sg, table->nents, i) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
		create_mapping_late(page_to_phys(sg_page(sg)),
				    (unsigned long)page_address(sg_page(sg)),
				    sg->length, PAGE_KERNEL);
#else
		change_secpage_range(page_to_phys(sg_page(sg)),
				     (unsigned long)page_address(sg_page(sg)),
				     sg->length, PAGE_KERNEL);
#endif
		flush_tlb_all();
		cma_release(secsg_heap->cma, sg_page(sg),
			    sg->length / PAGE_SIZE);
	}
	flush_tlb_all();
	sg_free_table(table);
	kfree(table);
	secsg_heap->alloc_size -= buffer->size;
}

int secsg_map_iommu(struct ion_secsg_heap *secsg_heap,
		    struct ion_buffer *buffer,
		    struct ion_iommu_map *map_data)
{
	struct mem_chunk_list mcl;
	int ret;

	if (!secsg_heap->TA_init) {
		pr_err("[%s] TA not inited.\n", __func__);
		return -EINVAL;
	}

	mcl.protect_id = SEC_TASK_DRM;
	mcl.buff_id = buffer->id;
	mcl.phys_addr = NULL;
	mcl.size = (u32)buffer->size;
	if (mcl.size != buffer->size) {
		pr_err("%s:size(0x%zx) too large\n", __func__, buffer->size);
		return -EINVAL;
	}

	mutex_lock(&secsg_heap->mutex);
	ret = secmem_tee_exec_cmd(secsg_heap->session,
				  &mcl, ION_SEC_CMD_MAP_IOMMU);
	mutex_unlock(&secsg_heap->mutex);
	if (ret) {
		pr_err("%s:exec map iommu cmd fail\n", __func__);
		return -EINVAL;
	}

	map_data->format.iova_start = mcl.va;
	map_data->format.iova_size = buffer->size;

	return 0;
}

void secsg_unmap_iommu(struct ion_secsg_heap *secsg_heap,
		       struct ion_buffer *buffer,
		       struct ion_iommu_map *map_data)
{
	struct mem_chunk_list mcl;
	int ret;

	if (!secsg_heap->TA_init) {
		pr_err("[%s] TA not inited.\n", __func__);
		return;
	}

	mcl.protect_id = SEC_TASK_DRM;
	mcl.buff_id = buffer->id;
	mcl.phys_addr = NULL;

	mutex_lock(&secsg_heap->mutex);
	ret = secmem_tee_exec_cmd(secsg_heap->session,
				  &mcl, ION_SEC_CMD_UNMAP_IOMMU);
	mutex_unlock(&secsg_heap->mutex);
	if (ret)
		pr_err("%s:exec unmap iommu cmd fail\n", __func__);
}
