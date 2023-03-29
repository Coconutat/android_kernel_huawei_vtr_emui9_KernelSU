/*
 * pmalloc_usercopy.h: Test for hardened usercopy
 *
 * (C) Copyright 2018 Huawei Technologies Co. Ltd.
 * Author: Igor Stoppa <igor.stoppa@huawei.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#ifndef _PMALLOC_USERCOPY_H
#define _PMALLOC_USERCOPY_H

#include <linux/mm.h>
#include <linux/vmalloc.h>

#define NO_VMALLOC_PAGE (-1)

/**
 * There is no need to check the size of the buffer being copied, because
 * all the vmap_area address ranges have, at the end, 1 page worth of
 * addresses not backed by any page mapping.
 * This means that an attack trying to overflow a non-pmalloc vma, to
 * overwrite the following pmalloc vma, would hit the canary range even
 * before reaching hte pmalloc vma, and it would cause a crash.
 *
 * The function is always inlined to minimize the overhead.
 */
static __always_inline const char *is_pmalloc_addr(const void *ptr)
{
	struct page *page = vmalloc_to_page(ptr);
	struct vmap_area *area;

	if (unlikely((unsigned long)page == NO_VMALLOC_PAGE))
		return NULL;

	area = find_vmap_area((unsigned long)ptr);
	if (unlikely(!area))
		return "no area";

	if (likely(!(area->vm->flags & VM_PMALLOC)))
		return NULL;

	return "pmalloc";
}
#endif
