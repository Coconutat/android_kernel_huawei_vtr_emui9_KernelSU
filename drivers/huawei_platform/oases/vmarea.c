/*
 * vmarea.c - permission controls for OASES framework
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#include <linux/version.h>
#include <linux/vmalloc.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

#include "vmarea.h"
#include "patch_api.h"

/* TODO: */
#if defined(__arm__)

int oases_set_vmarea_ro(unsigned long addr, int numpages)
{
	return 0;
}

int oases_set_vmarea_rw(unsigned long addr, int numpages)
{
	return 0;
}

int oases_set_vmarea_x(unsigned long addr, int numpages)
{
	return 0;
}

int oases_set_vmarea_nx(unsigned long addr, int numpages)
{
	return 0;
}

#else


/*
 * From 4.5.0 and on, set_memory_* allows vmalloc regions
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0)

#ifndef IS_ALIGNED
#define IS_ALIGNED(x, a) (((x) & ((typeof(x))(a) - 1)) == 0)
#endif

#ifndef PAGE_ALIGNED
#define PAGE_ALIGNED(addr) IS_ALIGNED((unsigned long)(addr), PAGE_SIZE)
#endif

/*
 * Mainline: clear_pte_bit/set_pte_bit moved from mm/pageattr.c
 * to asm/pgtable.h from 3.18+
 *
 * some lower versions from vendor backported this
 */
static inline pte_t oases_clear_pte_bit(pte_t pte, pgprot_t prot)
{
	pte_val(pte) &= ~pgprot_val(prot);
	return pte;
}

static inline pte_t oases_set_pte_bit(pte_t pte, pgprot_t prot)
{
	pte_val(pte) |= pgprot_val(prot);
	return pte;
}

struct page_change_data {
	pgprot_t set_mask;
	pgprot_t clear_mask;
};

static int change_page_range(pte_t *ptep, pgtable_t token, unsigned long addr,
			void *data)
{
	struct page_change_data *cdata = data;
	pte_t pte = *ptep;

	pte = oases_clear_pte_bit(pte, cdata->clear_mask);
	pte = oases_set_pte_bit(pte, cdata->set_mask);

	set_pte(ptep, pte);
	return 0;
}

/*
 * This function assumes that the range is mapped with PAGE_SIZE pages.
 */
static int __change_memory_common(unsigned long start, unsigned long size,
				pgprot_t set_mask, pgprot_t clear_mask)
{
	struct page_change_data data;
	int ret;

	data.set_mask = set_mask;
	data.clear_mask = clear_mask;
	ret = apply_to_page_range(&init_mm, start, size,
			change_page_range, &data);
	flush_tlb_kernel_range(start, start + size);
	return ret;
}

static int change_memory_common(unsigned long addr, int numpages,
				pgprot_t set_mask, pgprot_t clear_mask)
{
	unsigned long start = addr;
	unsigned long size = PAGE_SIZE*numpages;
	unsigned long end = start + size;
	struct vm_struct *area;

	if (!PAGE_ALIGNED(addr)) {
		return -EINVAL;
	}

	area = find_vm_area((void *)addr);
	if (!area ||
		end > (unsigned long)area->addr + area->size ||
		!(area->flags & VM_ALLOC))
		return -EINVAL;

	if (!numpages)
		return 0;

	return __change_memory_common(start, size, set_mask, clear_mask);
}

int oases_set_vmarea_ro(unsigned long addr, int numpages)
{
	return change_memory_common(addr, numpages,
					__pgprot(PTE_RDONLY),
					__pgprot(PTE_WRITE));
}

int oases_set_vmarea_rw(unsigned long addr, int numpages)
{
	return change_memory_common(addr, numpages,
					__pgprot(PTE_WRITE),
					__pgprot(PTE_RDONLY));
}

int oases_set_vmarea_x(unsigned long addr, int numpages)
{
	return change_memory_common(addr, numpages,
					__pgprot(0),
					__pgprot(PTE_PXN));
}

int oases_set_vmarea_nx(unsigned long addr, int numpages)
{
	return change_memory_common(addr, numpages,
					__pgprot(PTE_PXN),
					__pgprot(0));
}

#else
int oases_set_vmarea_ro(unsigned long addr, int numpages)
{
	return set_memory_ro(addr, numpages);
}

int oases_set_vmarea_rw(unsigned long addr, int numpages)
{
	return set_memory_rw(addr, numpages);
}

int oases_set_vmarea_x(unsigned long addr, int numpages)
{
	return set_memory_x(addr, numpages);
}

int oases_set_vmarea_nx(unsigned long addr, int numpages)
{
	return set_memory_nx(addr, numpages);
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0) */

#endif
