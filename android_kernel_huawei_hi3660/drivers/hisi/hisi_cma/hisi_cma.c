/*
 * hisi Contiguous Memory Allocator base on
 * Copyright (c) 2010-2011 by Hisicon Electronics.
 * Written by:
 *     Dongbin Yu <Yudongbin@hisilicon.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License or (at your optional) any later version of the license.
 */

/*
	cma-memory {
		#address-cells = <2>;
		#size-cells = <2>;
		ranges;

		tui-cma-heap-s {
			reg = <0x0 0x0 0x0 0x800000>;
			hisi,cma-name = "tui-heap";
			hisi,cma-dynamic;
			hisi,cma-sec;
		};

		drm-cma-heap {
			reg = <0x0 0xb000000 0x0 0x3000000>;
			hisi,cma-name = "drm-heap";
			hisi,cma-dynamic;
		};
	};
*/
#define pr_fmt(fmt) "hisi_cma: " fmt
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0)
#include <linux/device.h>
#endif
#include <asm/dma-contiguous.h>
#include <linux/err.h>
#include <linux/sizes.h>
#include <linux/dma-contiguous.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0)

#else
#include <linux/cma.h>
#endif
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/kernel.h>

#define HISI_CMA_AREA_NR 8

struct hisi_cma {
	struct device cma_dev;
	struct cma *cma_area;
	char *cma_name;
	long align;
	bool fixed;
	bool sec_prot;
};

static struct hisi_cma hisi_cma[HISI_CMA_AREA_NR];
static int hisi_cma_area_nr;

static int __init hisi_cma_reserve_mem_fdt_scan(unsigned long node,
		const char *uname, int depth, void *data)
{
	static int found;
	int ret;
	int len;
	bool fixed = false;
	phys_addr_t limit = *((phys_addr_t *)data);
	char *status;
	char *cma_name;
	struct cma **cma_area;
	struct device *cma_dev;
	phys_addr_t base, size;
	const __be32 *prop;
	unsigned long size_cells = dt_root_size_cells;
	unsigned long addr_cells = dt_root_addr_cells;

	if (!found && depth == 1 && strcmp(uname, "cma-memory") == 0) { /*lint !e421 */
		prop = of_get_flat_dt_prop(node, "#size-cells", NULL);
		if (prop)
			size_cells = be32_to_cpup(prop);

		prop = of_get_flat_dt_prop(node, "#address-cells", NULL);
		if (prop)
			addr_cells = be32_to_cpup(prop);

		found = 1;
		/* scan next node */
		return 0;
	} else if (!found) {
		/* scan next node */
		return 0;
	} else if (found && depth < 2) {
		/* scanning of /cma-reserve-memory has been finished */
		return 1;
	}

	status = (char *)of_get_flat_dt_prop(node, "status", NULL);
	/*
	 * Yes, we actually want strncmp here to check for a prefix
	 * ok vs. okay
	 */
	if (status && (strncmp(status, "ok", 2) != 0))
		return 0;

	if (of_get_flat_dt_prop(node, "hisi,cma-fixed", NULL)) {
		hisi_cma[hisi_cma_area_nr].fixed = 1;
		fixed = hisi_cma[hisi_cma_area_nr].fixed;
	}

	if (!!(cma_name = (char *)of_get_flat_dt_prop(node, "hisi,cma-name",
					NULL)))
		hisi_cma[hisi_cma_area_nr].cma_name = cma_name;

	if (of_get_flat_dt_prop(node, "hisi,cma-sec", NULL))
		hisi_cma[hisi_cma_area_nr].sec_prot = 1;

	prop = (const __be32 *)of_get_flat_dt_prop(node, "hisi,cma-align",
			NULL);
	if (prop)
		hisi_cma[hisi_cma_area_nr].align = be32_to_cpup(prop);

	prop = (const __be32 *)of_get_flat_dt_prop(node, "reg", &len);
	if (!prop || (depth != 2))
		return 0;

	base = (phys_addr_t)dt_mem_next_cell(addr_cells, &prop);
	size = (phys_addr_t)dt_mem_next_cell(size_cells, &prop);

	cma_area = &hisi_cma[hisi_cma_area_nr].cma_area;

	/**
	 * The cma base can over 4G, think of fama memory.
	 * Set the limit to 48 bits all ok.
	 * Since the ARM V8 support the max phys size is 48bits.
	 */
	limit = DMA_BIT_MASK(48); /*lint !e838*/

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0)
	ret = dma_contiguous_reserve_area(size, base, limit, cma_area, 1);
#else
	ret = cma_declare_contiguous(base, size, limit,  0, 0, fixed, cma_area);
#endif
	if (ret) {
		WARN_ON(1);
		return 1;
	}

	cma_dev = &hisi_cma[hisi_cma_area_nr].cma_dev;
	dev_set_cma_area(cma_dev, *cma_area);

	BUG_ON(++hisi_cma_area_nr == HISI_CMA_AREA_NR + 1);

	return 0;
}

/*
 * of_hisi_dma_contiguous_reserve() - reserve area(s) for contiguous
 * memory handling
 * @limit: End address of the reserved memory (optional, 0 for any).
 *
 * This function reserves memory from early allocator. It should be
 * called by arch specific code once the early allocator (memblock or bootmem)
 * has been activated and all other subsystems have already allocated/reserved
 * memory.
 * call it @init.c
 */
int __init of_hisi_cma_contiguous_reserve_area(phys_addr_t limit)
{
	return of_scan_flat_dt(hisi_cma_reserve_mem_fdt_scan, &limit);
}

/*
 * some module will get the hisi cma device;
 * and then will call cma_alloc.
 */
struct device *hisi_get_cma_area_device(char *name)
{
	int i;

	for (i = 0; i < hisi_cma_area_nr; i++) {
		if (!hisi_cma[i].cma_area)
			continue;
		if (!strcmp(hisi_cma[i].cma_name, name)) /*lint !e421 */
			break;
	}
	if (hisi_cma_area_nr == i)
		return NULL;
	return &(hisi_cma[i].cma_dev);
}
