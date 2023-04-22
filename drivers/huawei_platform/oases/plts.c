/*
 * plt manager
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#include <linux/types.h>
#include <linux/version.h>
#include <linux/bitmap.h>
#include <linux/mutex.h>
#include <linux/gfp.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#if IS_ENABLED(CONFIG_MODULES)
#include <linux/moduleloader.h>
#endif
#include <asm/cacheflush.h>
#include "plts_const.h"
#include "plts.h"
#include "util.h"

#define OASES_PLTS_COUNT (OASES_PLTS_MAX_SIZE / sizeof(struct oases_plt_entry))
#define OASES_MODULE_PLTS_COUNT (OASES_MODULE_PLTS_MAX_SIZE / sizeof(struct oases_plt_entry))

struct oases_module_plts_info {
	struct list_head list;
	void *mod;
	struct oases_plt_entry *plts;
	DECLARE_BITMAP(bitmap, OASES_MODULE_PLTS_COUNT);
	int allocated;
};

#if IS_ENABLED(CONFIG_OASES_STATIC_PLTS)
extern void *oases_plts;
#endif

/* for all plt related operations */
static DEFINE_MUTEX(oases_plts_lock);
/* plt info for modules */
static LIST_HEAD(oases_module_plts);
/* plt address for vmlinux */
static void *oases_plts_impl = NULL;
/* plt bitmap for vmlinux */
static DECLARE_BITMAP(oases_plts_bitmap, OASES_PLTS_COUNT);

#if IS_ENABLED(CONFIG_MODULES)
static void *module_core_base(void *mod)
{
	struct module *m = mod;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0)
	return m->module_core;
#else
	return m->core_layout.base;
#endif
}
#endif

void plts_lock(void *mod)
{
	mutex_lock(&oases_plts_lock);
}

void plts_unlock(void *mod)
{
	mutex_unlock(&oases_plts_lock);
}

int plts_empty(void *mod)
{
	struct oases_module_plts_info *pos, *n, *info = NULL;

	if (mod == NULL) {
		return bitmap_empty(oases_plts_bitmap, OASES_PLTS_COUNT);
	}
	list_for_each_entry_safe(pos, n, &oases_module_plts, list) {
		if (pos->mod == mod) {
			info = pos;
			break;
		}
	}
	if (info == NULL)
		return 1;
	return bitmap_empty(info->bitmap, OASES_MODULE_PLTS_COUNT);
}

/*
 * Note: __vmalloc_node_range has different args,
 * msm: 3.18+, other(hisi): 4.0+ 9 args, other have 8 args
 */
static void *plts_find(void *mod)
{
#if IS_ENABLED(CONFIG_MODULES)
	void *plts = NULL;
	unsigned long plts_bgn, plts_end;
	struct oases_module_plts_info *pos, *n, *info;

	list_for_each_entry_safe(pos, n, &oases_module_plts, list) {
		if (pos->mod == mod)
			return pos;
	}
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (info == NULL)
		return NULL;
	info->mod = mod;
#if IS_ENABLED(CONFIG_OASES_MODULE_EXT)
	// TODO: currently we don't implement this because struct module need modified.
#else
	plts_bgn = (unsigned long) module_core_base(mod);
	plts_end = plts_bgn + 128 * 1024 * 1024;
	plts = __vmalloc_node_range(OASES_MODULE_PLTS_MAX_SIZE, PAGE_SIZE,
			plts_bgn, plts_end, GFP_KERNEL, PAGE_KERNEL_EXEC,
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)) \
	|| ((LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0)) && IS_ENABLED(CONFIG_ARCH_MSM)))
			0,
#endif
			NUMA_NO_NODE, __builtin_return_address(0));
	if (plts == NULL)
		goto fail_free_info;
	info->plts = plts;
	info->allocated = 1;
	oases_debug("allocated oases plts @ %pK for module %s @ %pK\n",
				plts, module_name(mod), module_core_base(mod));
#endif
	list_add(&info->list, &oases_module_plts);
	return info;
fail_free_info:
	kfree(info);
	return NULL;
#else /* !IS_ENABLED(CONFIG_MODULES) */
	return NULL;
#endif
}

void *plts_reserve(void *mod)
{
	struct oases_module_plts_info *mp;
	unsigned long i;

	if (mod == NULL) {
		struct oases_plt_entry *plts =
			(struct oases_plt_entry *) oases_plts_impl;

		i = bitmap_find_next_zero_area(oases_plts_bitmap,
				OASES_PLTS_COUNT, 0, 1, 0);
		if (i >= OASES_PLTS_COUNT) {
			oases_error("OASES_PLTS_COUNT reached\n");
			return NULL;
		}
		bitmap_set(oases_plts_bitmap, i, 1);
		return &plts[i];
	}
	mp = plts_find(mod);
	if (mp == NULL)
		return NULL;
	i = bitmap_find_next_zero_area(mp->bitmap,
			OASES_MODULE_PLTS_COUNT, 0, 1, 0);
	if (i >= OASES_MODULE_PLTS_COUNT) {
		oases_error("OASES_MODULE_PLTS_COUNT reached\n");
		return NULL;
	}
	bitmap_set(mp->bitmap, i, 1);
	return &mp->plts[i];
}

void *plts_free(void *mod, void *entry)
{
	struct oases_module_plts_info *mp;
	struct oases_plt_entry *plts, *plte;
	unsigned long i, limit;
	unsigned long *bitmap;

	if (entry == NULL)
		return NULL;
	if (mod == NULL) {
		bitmap = oases_plts_bitmap;
		plts = (struct oases_plt_entry *) oases_plts_impl;
		plte = entry;
		limit = OASES_PLTS_COUNT;
	} else {
		mp = plts_find(mod);
		if (mp == NULL)
			return NULL;
		bitmap = mp->bitmap;
		plts = mp->plts;
		plte = entry;
		limit = OASES_MODULE_PLTS_COUNT;
	}
	i = plte - plts;
	if (i < limit) {
		bitmap_clear(bitmap, i, 1);
		return entry;
	}
	return NULL;
}

int plts_purge(void *mod)
{
#if IS_ENABLED(CONFIG_MODULES)
	struct oases_module_plts_info *pos, *n, *info = NULL;

	list_for_each_entry_safe(pos, n, &oases_module_plts, list) {
		if (pos->mod == mod) {
			info = pos;
			break;
		}
	}
	if (info == NULL) {
		return -ENOENT;
	}
	list_del(&info->list);
	if (info->allocated)
		vfree(info->plts);
	kfree(info);
	return 0;
#else
	return 0;
#endif
}

int __init oases_plts_init(void)
{
#if !IS_ENABLED(CONFIG_OASES_STATIC_PLTS)
	return -1;
#else
	oases_plts_impl = &oases_plts;
	return 0;
#endif
}

void oases_plts_free(void)
{
	/* no use */
}
