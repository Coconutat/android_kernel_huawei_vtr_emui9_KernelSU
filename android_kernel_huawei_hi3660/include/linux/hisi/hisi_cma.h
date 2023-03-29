/*
 * hisi_cma.h base on
 * Copyright (c) 2010-2011 by Hisicon Electronics.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License or (at your optional) any later version of the license.
 *
 */
/*
 *of_hisi_cma_contiguous_reserve_area() - reserve area(s) for contiguous
 *memory handling
 *@limit: End address of the reserved memory (optional, 0 for any).
 *
 *This function reserves memory from early allocator. It should be
 *called by arch specific code once the early allocator (memblock or
 *bootmem) has been activated and all other subsystems have already
 *allocated/reserved memory.call it @arm64_memblock_init
 */
int of_hisi_cma_contiguous_reserve_area(phys_addr_t limit);

#ifdef CONFIG_HISI_CMA
/*
 * some module will get the hisi cma device;
 *and then will call cma_alloc.
 */
struct device *hisi_get_cma_area_device(char *name);
#else
static inline struct device *hisi_get_cma_area_device(char *name)
{
	return NULL;
}
#endif
