/*
 * Copyright 2016 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Authors:
 * [ initial prototype implementation ]
 *   Dmitry Safonov <0x7f454c46@gmail.com> (ex-employee)
 * [ development of the current version ]
 *   Alexander Yashchenko <a.yashchenko@samsung.com>
 * [ small fixes ]
 *   Sergei Rogachev <s.rogachev@samsung.com>
 *
 * This file is part of MALI Midgard Reclaimer for NATIVE memory
 * allocations also known as "Midgard GMC" (graphical memory compression).
 *
 * "Midgard GMC" is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * "Midgard GMC" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with "Midgard GMC".  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

#define pr_fmt(fmt) "kbase-gmc: " fmt

#include "mali_kbase_gmc.h"
#include "mali_kbase_debug.h"
#include "mali_kbase_mem_linux.h"
#include "mali_kbase_hwaccess_time.h"
#include <linux/delay.h>
#include <linux/pagemap.h>
#include <linux/bug.h>
#include <linux/freezer.h>
#include <linux/page-flags.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/seq_file.h>

static atomic_t n_gmc_workers = ATOMIC_INIT(0);
static atomic_t n_gmc_workers_failed = ATOMIC_INIT(0);
static DECLARE_WAIT_QUEUE_HEAD(gmc_wait);

#define GMC_WORKER_TIMEOUT_MS 10000

#define GMC_MAX_COMPRESS_SIZE_IN_MEGA	MALI_GMC_COMPRESS_SIZE
#define GMC_PAGES_PER_MEGA	((0x100000)>> PAGE_SHIFT)
#define GPU_MEMORY_SEQ_BUF_SIZE 8*PAGE_SIZE//lint !e773

static inline bool is_region_growable(struct kbase_va_region *reg)
{
	return GROWABLE_FLAGS_REQUIRED & reg->flags;
}

static inline void seq_print_mem_type(struct seq_file *sfile,
		struct kbase_mem_phy_alloc *alloc)
{
	if (alloc)
		switch (alloc->type) {
			case KBASE_MEM_TYPE_NATIVE:
				seq_puts(sfile, "NATI");
				return;
			case KBASE_MEM_TYPE_IMPORTED_UMM:
				seq_puts(sfile, "IUMM");
				return;
			case KBASE_MEM_TYPE_ALIAS:
				seq_puts(sfile, "ALIA");
				return;
			case KBASE_MEM_TYPE_RAW:
				seq_puts(sfile, " RAW");
				return;
			case KBASE_MEM_TYPE_IMPORTED_USER_BUF:
				seq_puts(sfile, "USRB");
				return;
		}
	seq_puts(sfile, "UNKN");
}

static inline void seq_print_gpu_mappings(struct seq_file *sfile,
		struct kbase_mem_phy_alloc *alloc)
{
	seq_printf(sfile, "%6u",
			alloc ? atomic_read(&alloc->gpu_mappings) : 0);
}

static inline void seq_print_flags(struct seq_file *sfile,
		struct kbase_va_region *reg)
{
	seq_printf(sfile, "%s%s%s %s%s%s",
		(KBASE_REG_CPU_CACHED & reg->flags) ?  "c" : "-",
		(KBASE_REG_CPU_RD & reg->flags) ?  "r" : "-",
		(KBASE_REG_CPU_WR & reg->flags) ?  "w" : "-",
		(KBASE_REG_GPU_RD & reg->flags) ?  "r" : "-",
		(KBASE_REG_GPU_WR & reg->flags) ?  "w" : "-",
		(KBASE_REG_GPU_NX & reg->flags) ?  "-" : "x");
}

static inline size_t count_compressed_pages(struct kbase_mem_phy_alloc *alloc)
{
	size_t i, ret = 0;
	struct tagged_addr *p;

	for (i = 0; i < alloc->nents; i++){
		p = &alloc->pages[i];
		if (as_phys_addr_t(*p) && kbase_is_entry_compressed(*p))
			ret++;
	}
	return ret;
}

static inline void seq_print_reg(struct seq_file *sfile,
		struct kbase_va_region *reg)
{
	struct kbase_mem_phy_alloc *cpu_alloc,*gpu_alloc;

	if(!reg)	return;

	cpu_alloc = reg->cpu_alloc;
	gpu_alloc = reg->gpu_alloc;
	if(!cpu_alloc || !gpu_alloc)
		return;

	kbase_mem_phy_alloc_get(cpu_alloc);
	kbase_mem_phy_alloc_get(gpu_alloc);
	KBASE_DEBUG_ASSERT(gpu_alloc->type == cpu_alloc->type);
	seq_printf(sfile, "    %8llu %7zu ",
			reg->start_pfn, reg->nr_pages);
	if (is_region_growable(reg))
		seq_printf(sfile, "%7zu  ", reg->extent);
	else
		seq_puts(sfile, "      -  ");

	seq_printf(sfile, "%8zu", cpu_alloc->nents);
	seq_puts(sfile, "       ");
	seq_printf(sfile, "%8zu", (gpu_alloc && (cpu_alloc != gpu_alloc)) ? gpu_alloc->nents : 0);
	seq_printf(sfile, " %7zu  ", count_compressed_pages(cpu_alloc));

	seq_print_flags(sfile, reg);
	seq_putc(sfile, ' ');
	seq_print_gpu_mappings(sfile, gpu_alloc);
	seq_puts(sfile, "   ");
	seq_print_mem_type(sfile, cpu_alloc);
	seq_putc(sfile, '\n');
	kbase_mem_phy_alloc_put(cpu_alloc);
	kbase_mem_phy_alloc_put(gpu_alloc);
}


static void kbasep_show_kctx_overall_native_memory(
		struct seq_file *sfile,
		struct kbase_context *kctx)
{
	struct rb_node *node;
	struct kbase_va_region *reg = NULL;
	unsigned long backed_pages_overall = 0;
	unsigned long compressed_pages_overall = 0;
	int i = 0;
	struct rb_root *rb_root;

	kbase_gpu_vm_lock(kctx);
	for(i = 0; i < 2 ; i ++){
		if(i == 0)
			rb_root = &kctx->reg_rbtree_custom;
		else
			rb_root = &kctx->reg_rbtree_same;
		for_each_rb_node(rb_root, node) {
			reg = rb_entry(node, struct kbase_va_region, rblink);
			if (!is_region_free(reg) && reg->cpu_alloc &&
				reg->cpu_alloc->type == KBASE_MEM_TYPE_NATIVE) {
				backed_pages_overall += reg->cpu_alloc->nents;
				if (reg->cpu_alloc != reg->gpu_alloc) {
					KBASE_DEBUG_ASSERT(reg->cpu_alloc->nents != reg->gpu_alloc->nents);
					backed_pages_overall *= 2;
				}
				compressed_pages_overall += count_compressed_pages(reg->cpu_alloc);
				if (reg->cpu_alloc != reg->gpu_alloc) {
					compressed_pages_overall += count_compressed_pages(reg->gpu_alloc);
				}
			}
		}
	}
	kbase_gpu_vm_unlock(kctx);
	for (; i < 90; i++)
		seq_putc(sfile, '-');
	seq_putc(sfile, '\n');
	seq_printf(sfile, "native memory overall: %8lu (pages)\n", backed_pages_overall);
	seq_printf(sfile, "native memory compressed: %8lu (pages)\n", compressed_pages_overall);
	seq_putc(sfile, '\n');
}


static void kbasep_gpu_memory_show_kctx(
		struct seq_file *sfile,
		struct kbase_context *kctx)
{
	struct rb_node *node;
	struct kbase_va_region *reg = NULL;
	int i;
	struct rb_root *rb_root;

	kbase_gpu_vm_lock(kctx);
	seq_printf(sfile, /* header */
		"        pfn    pages  extent    backed(cpu) backed(gpu)  compr    flags  gpu_map type\n");

	/* through all gpu VAs */
	for(i = 0; i < 2 ; i ++){
		if(i == 0)
			rb_root = &kctx->reg_rbtree_custom;
		else
			rb_root = &kctx->reg_rbtree_same;
		for_each_rb_node(rb_root, node) {
			reg = rb_entry(node, struct kbase_va_region, rblink);
			if (!is_region_free(reg))
				seq_print_reg(sfile, reg);
		}
	}

	kbase_gpu_vm_unlock(kctx);
}


int gmc_memory_info_show(struct seq_file *sfile, void *data)
{
	struct list_head *entry;
	const struct list_head *kbdev_list;

	kbdev_list = kbase_dev_list_get();
	list_for_each(entry, kbdev_list) {
		struct kbase_device *kbdev = NULL;
		struct kbasep_kctx_list_element *element;

		kbdev = list_entry(entry, struct kbase_device, entry);
		/* output the total memory usage and cap for this device */
		seq_printf(sfile, "%-16s  %10u\n",
				kbdev->devname,
				atomic_read(&(kbdev->memdev.used_pages)));

		mutex_lock(&kbdev->kctx_list_lock);
		list_for_each_entry(element, &kbdev->kctx_list, link) {
			/* output the memory usage and cap for each kctx
			* opened on this device */

			struct task_struct *tsk;
			char tsk_name[TASK_COMM_LEN] = "";
			struct kbase_context *kctx = element->kctx;

			rcu_read_lock();
			tsk = find_task_by_vpid(kctx->tgid);
			if (tsk) {
				get_task_comm(tsk_name, tsk);
			}
			rcu_read_unlock();

			seq_printf(sfile,
				"  kctx-0x%pK pid:%u tgid:%u comm:%s all: %u OOM see:%u\n",
				kctx, kctx->pid, kctx->tgid, tsk_name,
				atomic_read(&kctx->used_pages),
				atomic_read(&kctx->nonmapped_pages));
			kbasep_gpu_memory_show_kctx(sfile, kctx);
			kbasep_show_kctx_overall_native_memory(sfile, kctx);
		}
		mutex_unlock(&kbdev->kctx_list_lock);
	}

	kbase_dev_list_put(kbdev_list);
	return 0;
}


/**
 * kbase_gmc_unlock_task - Unlock function, which unlocks mmap_sem and kbase_gpu_vm_lock
 * of appropriate task and puts related reference counters.
 *
 * @gmc_tsk:        Info about related taask
 * @op:             Operation may be GMC_COMPRESS, GMC_DECOMPRESS, GMC_COUNT_DECOMPRESSED
 *
 * Returns: nothing
 */
static void kbase_gmc_unlock_task(struct kbase_gmc_tsk *gmc_tsk, gmc_op op)
{
	if (!(op == GMC_COMPRESS)) {
		kbase_gpu_vm_unlock(gmc_tsk->kctx);
	} else {
		kbase_gpu_vm_unlock(gmc_tsk->kctx);
		up_write(&gmc_tsk->mm->mmap_sem);
		mmdrop(gmc_tsk->mm);
		put_task_struct(gmc_tsk->task);
	}
}

/**
 * kbase_gmc_trylock_task - Complex lock function, which locks mmap_sem and kbase_gpu_vm_lock
 * of appropriate task and takes needed reference counters.
 *
 * @gmc_tsk:        Info about related taask
 * @op:             Operation may be GMC_COMPRESS, GMC_DECOMPRESS, GMC_COUNT_DECOMPRESSED
 *
 * Return: 0 if lock was taken without problems, or -1 if mm or task doesn't exists
 * at the moment.
 *
 * Note:
 * This function behaves differently if op == GMC_COMPRESS. In this case the mmap_sem and mm, task
 * reference counters should be taken into consideration, because CPU mappings will be shrinked
 * before compression.
 */
static int kbase_gmc_trylock_task(struct kbase_gmc_tsk *gmc_tsk, gmc_op op)
{
	struct task_struct *tsk;
	if (!(op == GMC_COMPRESS)) {
		/*op != GMC_COMPRESS */
		kbase_gpu_vm_lock(gmc_tsk->kctx);
	} else {
		/*op == GMC_COMPRESS */
		lockdep_assert_held(&gmc_tsk->kctx->reg_lock);
		rcu_read_lock();
		tsk = find_task_by_vpid(gmc_tsk->kctx->tgid);
		if (!tsk) {
			/* Task is gone nothing to do */
			rcu_read_unlock();
			pr_debug("task is gone, can't lock\n");
			return -ESRCH;
		}
		get_task_struct(tsk);
		rcu_read_unlock();
		task_lock(tsk);
		if (tsk->mm)
			atomic_inc(&tsk->mm->mm_count);
		else {
			task_unlock(tsk);
			put_task_struct(tsk);
			pr_debug("mm is gone, can't lock\n");
			return -ESRCH;
		}
		gmc_tsk->mm = tsk->mm;
		task_unlock(tsk);
		down_write(&gmc_tsk->mm->mmap_sem);
		kbase_gpu_vm_lock(gmc_tsk->kctx);
		gmc_tsk->task = tsk;
	}
	return 0;
}

void kbase_gmc_dma_unmap_page(struct kbase_device *kbdev, struct page *page)
{
	dma_unmap_page(kbdev->dev, kbase_dma_addr(page),
			PAGE_SIZE, DMA_FROM_DEVICE);
}

dma_addr_t kbase_gmc_dma_map_page(struct kbase_device *kbdev, struct page *page)
{
	dma_addr_t dma_addr;
	dma_addr = dma_map_page(kbdev->dev, page, 0, PAGE_SIZE,
			DMA_TO_DEVICE);
	if (dma_mapping_error(kbdev->dev, dma_addr)) {
		pr_alert("%s: dma_mapping_error!\n", __func__);
		return (dma_addr_t)(0ULL);
	}
	return dma_addr;
}

/**
 * kbase_gmc_page_decompress - Decompress physical page.
 *
 * @phys_addr_t:        Physical page address
 * @kbdev:              Kbase device
 * @policy_id:          Cache policy id.
 *
 * Return: 0 if page was decompressed or error code if something is wrong.
 */
static int kbase_gmc_page_decompress(struct tagged_addr *p, struct kbase_device *kbdev, u8 policy_id)
{
	struct page *page;
	dma_addr_t dma_addr;
	int err;
	gfp_t flags;
	struct gmc_storage_handle *handle;
	struct memory_group_manager_ops *mgm_ops = kbdev->hisi_dev_data.mgm_ops;
	KBASE_DEBUG_ASSERT(mgm_ops);

	BUG_ON(!kbase_is_entry_compressed(*p));

	handle = kbase_get_gmc_handle(p);
#if defined(CONFIG_ARM) && !defined(CONFIG_HAVE_DMA_ATTRS) && LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
	flags = GFP_USER | __GFP_ZERO;
#else
	flags = GFP_HIGHUSER | __GFP_ZERO;
#endif
	page = mgm_ops->mgm_alloc_pages(kbdev->hisi_dev_data.mgm_dev,
	                                policy_id, flags, 0);
	if (!page) {
		pr_err("Unable to allocate a page for decompression.\n");
		return -ENOMEM;
	}
	err = gmc_storage_get_page(kbdev->kbase_gmc_device.storage, page, handle);
	if (err) {
		pr_alert("%s: can't get page for handle: %pK err: %d\n", __func__,
				handle, (int)err);
		mgm_ops->mgm_free_pages(kbdev->hisi_dev_data.mgm_dev,
		                        policy_id, page, 0);
		return -EINVAL;
	}
	dma_addr = kbase_gmc_dma_map_page(kbdev, page);
	p->tagged_addr = page_to_phys(page);
	kbase_clear_entry_compressed(p);
	p->tagged_addr |=  (p->gmc_flag & ~PAGE_MASK);
	p->gmc_flag = 0;
	BUG_ON(dma_addr != as_phys_addr_t(*p));
	return 0;
}

/**
 * kbase_gmc_page_compress - Compress physical page.
 *
 * @phys_addr_t:        Physical page address
 * @kbdev:              Kbase device
 *
 * Return: 0 if page was compressed or error code if something is wrong.
 */
static int kbase_gmc_page_compress(struct tagged_addr *p, struct kbase_device *kbdev)
{
	struct page *page;
	struct gmc_storage_handle *handle;

	BUG_ON(kbase_is_entry_compressed(*p));
	page = pfn_to_page(PFN_DOWN(as_phys_addr_t(*p)));
	if (!kbase_dma_addr(page)) {
		pr_debug("compression for physaddr %llu initiated,\
			but dma address was not set\n", (unsigned long long) as_phys_addr_t(*p));
		return -EINVAL;
	}

	kbase_gmc_dma_unmap_page(kbdev, page);

	handle = gmc_storage_put_page(kbdev->kbase_gmc_device.storage, page);
	if (IS_ERR(handle)) {
		dma_addr_t dma_addr;
		dma_addr = kbase_gmc_dma_map_page(kbdev, page);
		BUG_ON(dma_addr != as_phys_addr_t(*p) );
		if (PTR_ERR(handle) != -EFBIG) {
			pr_alert("can't compress physaddr %llu, err: %pK\n",
				(unsigned long long) as_phys_addr_t(*p), handle);
			return -EINVAL;
		}
		pr_debug("physaddr %llu is badly compressed, skipping it\n", (unsigned long long) as_phys_addr_t(*p));
		return 0;
	}

	kbase_set_gmc_handle(handle,p);
	put_page(page);
	return 0;
}

/**
 * region_should_be_compressed - should this region be compressed or not.
 *
 * @reg:        Region to be compressed
 *
 * Return: True if region should be compressed, False otherwise.
 */
static bool region_should_be_compressed(struct kbase_va_region *reg)
{
	bool compress = true;
	if (reg->cpu_alloc != reg->gpu_alloc) {
		/*Now don't touch regions with infinite cache feature */
		compress = false;
	} else if (reg->flags & KBASE_REG_DONT_NEED) {
		/*This region will be deleted soon*/
		compress = false;
	}
	return compress;
}

/**
 * kbase_gmc_compress_alloc - Compress pages in specified allocation.
 *
 * @alloc:        Physical pages allocation
 * @start_idx:    Start index
 * @nr:           Number of pages
 *
 * Return: 0 on success or error.
 */
static int kbase_gmc_compress_alloc(struct kbase_mem_phy_alloc *alloc, u64 start_idx, size_t nr)
{
	int ret = 0;
	unsigned int i;
	struct tagged_addr *tagged_page = NULL;

	for (i = start_idx; i < start_idx + nr; i++) {
		tagged_page = &alloc->pages[i];
		if(is_huge(alloc->pages[i]))		//don't compress huge head
			continue;
		if (as_phys_addr_t(*tagged_page) && kbase_is_entry_decompressed(*tagged_page)) {
			tagged_page->gmc_flag = tagged_page->tagged_addr & ~PAGE_MASK;
			tagged_page->tagged_addr = as_phys_addr_t(*tagged_page);
			ret = kbase_gmc_page_compress(tagged_page, alloc->imported.kctx->kbdev);
			if (ret) {
				pr_err("can't compress page, physaddr %llu\n", (unsigned long long) as_phys_addr_t(*tagged_page));
				return ret;
			}
		}
	}
	return ret;
}

/**
 * kbase_gmc_compress_region - Compress pages in specified region. This function
 * implements CPU & GPU unmapping before pages would be compressed.
 *
 * @reg:        Graphical memory region
 * @vpfn:       Strarting virtual pfn
 * @nr:         Number of pages
 *
 * Return: Number of pages to be compressed or error.
 *
 * Note:
 * This function must be invoked only under appropriate task (kctx) gpu_vm_lock and mmap_sem.
 */
static int kbase_gmc_compress_region(struct kbase_va_region *reg, u64 vpfn, size_t nr)
{
	int ret = 0;
	if (!region_should_be_compressed(reg))
		return ret;
	struct kbase_context *kctx = kbase_reg_flags_to_kctx(reg);
	/* unmap all pages from CPU */
	kbase_mem_shrink_cpu_mapping(kctx, reg, 0, reg->cpu_alloc->nents);

	/* unmap all pages from GPU */
	ret = kbase_mem_shrink_gpu_mapping(kctx, reg, 0, reg->gpu_alloc->nents);
	if (ret)
		return ret;

	pr_debug("%s kctx %d region %pK, pfn range [%llu, %llu]\n",
		__func__, (int)kctx->tgid, reg, vpfn, vpfn + nr);
	return kbase_gmc_compress_alloc(reg->cpu_alloc, vpfn - reg->start_pfn, nr);
}

/**
 * kbase_gmc_invalidate_alloc - Invalidates entire page set from alloc
 * @kctx:        Graphical context
 * @start:       phy addr to start with
 * @pages_num:   number of pages to invalidate
 *
 * Return: nothing
 */
void kbase_gmc_invalidate_alloc(struct kbase_context *kctx,
		struct tagged_addr *start, size_t pages_num)
{
	size_t i;

	lockdep_assert_held(&kctx->reg_lock);

	for (i = 0; i < pages_num; i++) {
		struct tagged_addr *p = &start[i];

		if (as_phys_addr_t(*p) && kbase_is_entry_compressed(*p)) {
			pr_debug("%s handle %pK\n", __func__, kbase_get_gmc_handle(p));
			gmc_storage_invalidate_page(kctx->kbdev->kbase_gmc_device.storage,
				kbase_get_gmc_handle(p));
			p->tagged_addr = 0;
			kbase_clear_entry_compressed(p);
			p->tagged_addr |= p->gmc_flag;
		}
	}
}

/**
 * kbase_gmc_decompress_alloc - Decompress entire page set from alloc
 * @kctx:        Graphical context
 * @start:       phy addr to start with
 * @pages_num:   number of pages to invalidate
 *
 * Return: 0 if success, or error if decompression failed
 */
static int kbase_gmc_decompress_alloc(struct kbase_mem_phy_alloc *alloc, s64 start_idx, size_t nr)
{
	int i,nr_size,ret = 0;
	struct tagged_addr *tagged_page = NULL;
	nr_size = (int)nr;

	for (i = start_idx; i < start_idx + nr_size; i++) {
		if(i < 0)		//patch for pgoff 3times aligment case
			continue;
		tagged_page = &alloc->pages[i];
		if(is_huge(alloc->pages[i]))		//bypass huge head because we didn't  compress it
			continue;
		if (as_phys_addr_t(*tagged_page) && kbase_is_entry_compressed(*tagged_page)) {
			ret = kbase_gmc_page_decompress(tagged_page, alloc->imported.kctx->kbdev, alloc->lb_policy_id);
			if (ret) {
				pr_err("can't decompress page, physaddr %llu\n", (unsigned long long) as_phys_addr_t(*tagged_page));
				return ret;
			}
		}
	}
	return ret;
}

static int kbase_gmc_decompress_region(struct kbase_va_region *reg, u64 vpfn, size_t nr)
{
	int ret = 0,ret_insert;
	struct kbase_context *kctx = kbase_reg_flags_to_kctx(reg);
	ret = kbase_gmc_decompress_alloc(reg->cpu_alloc, (s64)(vpfn - reg->start_pfn), nr);
	if(kctx->set_pt_flag)
	{
		if (reg->flags & KBASE_REG_SCRAMBLE_BIT)
			ret_insert = kbase_mmu_insert_pages_with_scramble_bit(kctx->kbdev, &kctx->mmu, vpfn,
						&reg->cpu_alloc->pages[vpfn - reg->start_pfn], nr, reg->flags, kctx->as_nr, 0);
		else
			ret_insert = kbase_mmu_insert_pages(kctx->kbdev, &kctx->mmu, vpfn,
						&reg->cpu_alloc->pages[vpfn - reg->start_pfn], nr, reg->flags, kctx->as_nr);
	}

	return ret;
}


/**
 * kbase_gmc_walk_region - Performs GMC specific action with region pages. This
 * walker function could, compress, decompress or count pages to be compressed.
 * @reg:        Graphical memory region
 * @op:         Operation - could be GMC_COMPRESS, GMC_DECOMPRESS or
 *              GMC_COUNT_DECOMPRESSED
 *
 * Return: 0 on success or error.
 */
static int kbase_gmc_walk_region(struct kbase_va_region *reg, gmc_op op)
{
	int ret = 0;
	struct kbase_mem_phy_alloc *cpu_alloc = reg->cpu_alloc;
	struct kbase_mem_phy_alloc *gpu_alloc = reg->gpu_alloc;

	if (!cpu_alloc)
		return ret;

	kbase_mem_phy_alloc_get(cpu_alloc);
	kbase_mem_phy_alloc_get(gpu_alloc);

	/* if allocation is KBASE_MEM_TYPE_IMPORTED_UMM i.e.,
	 * it's used for DMA operations between drivers, so
	 * don't touch it (it's usally DRM/GEM memory) */
	if (cpu_alloc->type != KBASE_MEM_TYPE_NATIVE) {
		kbase_mem_phy_alloc_put(gpu_alloc);
		kbase_mem_phy_alloc_put(cpu_alloc);
		return ret;
	}

	switch (op) {
	case GMC_DECOMPRESS:
		ret = kbase_gmc_decompress_region(reg, reg->start_pfn, cpu_alloc->nents);
		break;
	case GMC_COMPRESS:
		ret = kbase_gmc_compress_region(reg, reg->start_pfn, cpu_alloc->nents);
		break;
	default:
		pr_err("Invalid GMC operation\n");
		ret = -EINVAL;
	}

	kbase_mem_phy_alloc_put(gpu_alloc);
	kbase_mem_phy_alloc_put(cpu_alloc);
	return ret;
}

/**
 * kbase_gmc_walk_region_work - Worker function for handling per-region workload.
 *
 * @work:      Work struct
 *
 * Return:     Nothing.
 */
void kbase_gmc_walk_region_work(struct work_struct *work)
{
	int ret = 0;
	struct kbase_va_region *reg = container_of(work, struct kbase_va_region, gmc_work);
	pr_debug("worker [%pK] have started, op %d\n", work, reg->op);
	ret = kbase_gmc_walk_region(reg, reg->op);
	if (ret) {
		pr_err("worker [%pK] has errors during operation %d\n", work, reg->op);
		atomic_inc(&n_gmc_workers_failed);
	}
	atomic_dec(&n_gmc_workers);
	wake_up_interruptible(&gmc_wait);
}

/**
 * kbase_gmc_walk_kctx - Walk through not freed kctxs regions.
 *
 * @kctx:      Graphical context
 * @op:         Operation - could be GMC_COMPRESS, GMC_DECOMPRESS or
 *
 * Return: 0 on success or error.
 */
static int kbase_gmc_walk_kctx(struct kbase_context *kctx, struct kbase_gmc_arg arg)
{
	int ret = 0;
	struct rb_node *node;
	struct kbase_va_region *reg = NULL;
	struct kbase_gmc_tsk gmc_tsk;
	int n_workers_failed = 0;
	int nr_pages_to_compressed = GMC_MAX_COMPRESS_SIZE_IN_MEGA * GMC_PAGES_PER_MEGA;
	long timeout_jiff = msecs_to_jiffies(GMC_WORKER_TIMEOUT_MS);

	gmc_tsk.kctx = kctx;
	gmc_tsk.task = NULL;

	if(arg.op == GMC_COMPRESS){
		if(arg.compress_size >= GMC_MAX_COMPRESS_SIZE_IN_MEGA || arg.compress_size == 0)
			arg.compress_size = GMC_MAX_COMPRESS_SIZE_IN_MEGA;
		nr_pages_to_compressed = arg.compress_size * GMC_PAGES_PER_MEGA;	//in  pages
	}

	KBASE_DEBUG_ASSERT(!atomic_read(&n_gmc_workers));
	atomic_set(&n_gmc_workers_failed, 0);
	ret = kbase_gmc_trylock_task(&gmc_tsk, arg.op);
	if (ret)
		return ret;

	if(arg.op == GMC_DECOMPRESS)
		kctx->set_pt_flag = true;

	for_each_rb_node(&(kctx->reg_rbtree_same), node) {
		reg = rb_entry(node, struct kbase_va_region, rblink);
		if (!is_region_free(reg)) {
			reg->op = arg.op;

			atomic_inc(&n_gmc_workers);
			queue_work(system_unbound_wq, &reg->gmc_work);

			if(arg.op == GMC_COMPRESS){
				nr_pages_to_compressed -= reg->cpu_alloc->nents;
				if(nr_pages_to_compressed <= 0)
					break;	//only compress arg->compress_size
			}
		}
	}

	for_each_rb_node(&(kctx->reg_rbtree_custom), node) {
		reg = rb_entry(node, struct kbase_va_region, rblink);
		if (!is_region_free(reg)) {
			reg->op = arg.op;

			atomic_inc(&n_gmc_workers);
			queue_work(system_unbound_wq, &reg->gmc_work);

			if(arg.op == GMC_COMPRESS){
				nr_pages_to_compressed -= reg->cpu_alloc->nents;
				if(nr_pages_to_compressed <= 0)
					break;	//only compress arg->compress_size
			}
		}
	}

	while (atomic_read(&n_gmc_workers) > 0) {
		int err = wait_event_interruptible_timeout(gmc_wait, !atomic_read(&n_gmc_workers),timeout_jiff);
		if (err <= 0) {
			pr_alert("Timeout while waiting GMC workers, \
				it takes more than %d sec\n",
				GMC_WORKER_TIMEOUT_MS);
			kbase_gmc_unlock_task(&gmc_tsk, arg.op);
			return -ETIMEDOUT;
		}
	}

	if(arg.op == GMC_DECOMPRESS)
		kctx->set_pt_flag = false;

	kbase_gmc_unlock_task(&gmc_tsk, arg.op);
	n_workers_failed = atomic_read(&n_gmc_workers_failed);
	if (n_workers_failed > 0) {
		pr_err("%d workers has failed to complete\n", n_workers_failed);
		ret = -EINVAL;
	}
	return ret;
}

int kbase_get_compressed_region(struct kbase_va_region *reg, u64 vpfn, size_t nr)
{
	return kbase_gmc_decompress_region(reg, vpfn, nr);
}

int kbase_get_compressed_alloc(struct kbase_mem_phy_alloc *alloc, u64 start_idx, size_t nr)
{
	return kbase_gmc_decompress_alloc(alloc, start_idx, nr);
}

/**
 * kbase_gmc_walk_device - Walk through device kctxs and find one associated
 * with pid.
 *
 * @kbdev:      Kbase_device
 * @pid:        Traget pid to compress
 * @op:         Operation - could be GMC_COMPRESS, GMC_DECOMPRESS or
 *              GMC_COUNT_DECOMPRESSED
 * Return: Number of pages handled by operation op.
 */
int kbase_gmc_walk_device(struct kbase_device *kbdev, pid_t pid, struct kbase_gmc_arg arg)
{
	int ret = 0;
	struct kbasep_kctx_list_element *element, *tmp;
	mutex_lock(&kbdev->kctx_list_lock);
	list_for_each_entry_safe(element, tmp, &kbdev->kctx_list, link) {
		struct kbase_context *kctx = element->kctx;
		if (kctx->tgid == pid || pid == GMC_HANDLE_ALL_KCTXS) {
			ret = kbase_gmc_walk_kctx(kctx, arg);
			if (ret)
				goto out;
		}
	}
out:
	mutex_unlock(&kbdev->kctx_list_lock);
	return ret;
}

/**
 * kbase_gmc_compress - Compress graphical kctx data, associated with pid
 * @pid:        Traget pid to compress
 * @gmc_dev:    graphical memory compression device passed from generic layer
 *
 * Return: 0 if success error if compression is failed.
 */
int kbase_gmc_compress(pid_t pid, struct gmc_device *gmc_dev,long compress_size)
{
	struct kbase_device *kbdev = container_of(gmc_dev, struct kbase_device, kbase_gmc_device);
	struct kbase_gmc_arg arg;
      arg.op = GMC_COMPRESS;
      arg.compress_size = compress_size;

	return kbase_gmc_walk_device(kbdev, pid, arg);
}

/**
 * kbase_gmc_decompress - Decompress graphical kctx data, associated with pid
 * @pid:        Traget pid to decompress
 * @gmc_dev:    graphical memory compression device passed from generic layer
 *
 * Return: 0 if success error if decompression is failed.
 */
int kbase_gmc_decompress(pid_t pid, struct gmc_device *gmc_dev)
{
	struct kbase_device *kbdev = container_of(gmc_dev, struct kbase_device, kbase_gmc_device);
	struct kbase_gmc_arg arg;
	arg.op = GMC_DECOMPRESS;
	arg.compress_size = 0;

	return kbase_gmc_walk_device(kbdev, pid, arg);
}

int kbase_gmc_meminfo_open(struct inode *in, struct file *file)
{
	return single_open_size(file, gmc_memory_info_show, NULL,GPU_MEMORY_SEQ_BUF_SIZE);
}
#pragma GCC diagnostic pop