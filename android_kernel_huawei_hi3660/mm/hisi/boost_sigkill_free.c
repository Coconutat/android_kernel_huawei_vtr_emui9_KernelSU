/*
 * mm/boost_sigkill_free.c
 *
 * Boost memory free for SIGKILLed process
 *
 *  Copyright (C) 2016 Huawei Technologies Co., Ltd.
 */

#include <linux/mm.h>
#include <linux/hugetlb.h>
#include <linux/boost_sigkill_free.h>

#include <asm/tlb.h>
#include "internal.h"

unsigned int sysctl_boost_sigkill_free;

static void __fast_free_user_mem(struct mm_struct *mm)
{
	struct vm_area_struct *vma;
	struct mmu_gather tlb;

	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		if (is_vm_hugetlb_page(vma))
			continue;
		/*
		 * mlocked VMAs require explicit munlocking before unmap.
		 * Let's keep it simple here and skip such VMAs.
		 */
		if (vma->vm_flags & VM_LOCKED)
			continue;

		if (vma_is_anonymous(vma) || !(vma->vm_flags & VM_SHARED)) {
			tlb_gather_mmu(&tlb, mm, vma->vm_start, vma->vm_end);
			unmap_page_range(&tlb, vma, vma->vm_start,
					vma->vm_end, NULL);
			tlb_finish_mmu(&tlb, vma->vm_start, vma->vm_end);
		}
	}
}

void fast_free_user_mem(void)
{
	struct mm_struct *mm = current->mm;

	if (!mm)
		return;

	down_read(&mm->mmap_sem);
	if (test_and_set_bit(MMF_FAST_FREEING, &mm->flags)) {
		up_read(&mm->mmap_sem);
		return;
	}

	__fast_free_user_mem(mm);

	up_read(&mm->mmap_sem);
}
