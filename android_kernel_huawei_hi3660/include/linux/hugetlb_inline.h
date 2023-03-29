#ifndef _LINUX_HUGETLB_INLINE_H
#define _LINUX_HUGETLB_INLINE_H

#ifdef CONFIG_HUGETLB_PAGE

#include <linux/mm.h>

static inline bool is_vm_hugetlb_page(struct vm_area_struct *vma)
{
#ifdef CONFIG_SPECULATIVE_PAGE_FAULT
	return !!(READ_ONCE(vma->vm_flags) & VM_HUGETLB);
#else
	return !!(vma->vm_flags & VM_HUGETLB);
#endif
}

#else

static inline bool is_vm_hugetlb_page(struct vm_area_struct *vma)
{
	return false;
}

#endif

#endif
