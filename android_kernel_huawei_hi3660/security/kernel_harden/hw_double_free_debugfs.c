/*
 * Huawei double free check debugfs interface
 *
 * Copyright (c) 2017 Huawei.
 *
 * Authors:
 * chenli <chenli45@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/slub_def.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mm_types.h>

#include <asm/memory.h>
#include <asm/module.h>
#include <asm/sections.h>


static struct dentry *slub_double_free_root;

#define SLUB_DEBUG_ENTRY(name) \
	static int slub_##name##_open(struct inode *inode, struct file *file) \
{ \
	    return single_open(file, slub_##name##_show, inode->i_private); \
} \
\
static const struct file_operations slub_##name##_fops = { \
	    .owner = THIS_MODULE, \
	    .open = slub_##name##_open, \
	    .read = seq_read, \
	    .llseek = seq_lseek, \
	    .release = single_release, \
}

#ifdef CONFIG_HW_SLUB_SANITIZE
static int slub_double_free_show(struct seq_file *m, void *v)
{
    /* create kmem_cache allocate object which size is 128bytes */
    struct kmem_cache *my_cachep = kmem_cache_create("my_cache", 128, 0, SLAB_HWCACHE_ALIGN, NULL);
    void *freelist = this_cpu_read(my_cachep->cpu_slab->freelist);
    seq_printf(m, "The freelist object's addr is 0x%lx\n", freelist);
    void *object = kmem_cache_alloc(my_cachep, GFP_KERNEL);
    seq_printf(m, "The allocated object's addr is 0x%lx\n", object);
    kfree(object);
    freelist = this_cpu_read(my_cachep->cpu_slab->freelist);
    seq_printf(m, "After free object, the freelist object's addr is 0x%lx\n", freelist);

#ifdef CONFIG_HW_SLUB_DF
    if (!(my_cachep->flags & SLAB_DOUBLEFREE_CHECK))
#else
    if (!(my_cachep->flags & SLAB_CLEAR))
#endif
    {
        seq_printf(m, "Before object is double freed, Don't detect double free\n");
    }

    kfree(object);

#ifdef CONFIG_HW_SLUB_DF
    if (my_cachep->flags & SLAB_DOUBLEFREE_CHECK)
#else
    if (my_cachep->flags & SLAB_CLEAR)
#endif
    {
        seq_printf(m, "After object is double freed, Detect double free\n");
    }
    kmem_cache_destroy(my_cachep);
}
#endif

#ifdef CONFIG_HW_SLUB_SANITIZE
SLUB_DEBUG_ENTRY(double_free);
#endif

static int __init hwslub_double_free_debugfs_init(void)
{
    /*create sys/kernel/debug/hwslub_double_free for debugfs*/
	slub_double_free_root = debugfs_create_dir("hwslub_double_free", NULL);
	if (!slub_double_free_root)
		return -ENOMEM;
#ifdef CONFIG_HW_SLUB_SANITIZE
	debugfs_create_file("double_free",
			S_IRUGO,
			slub_double_free_root,
			NULL,
			&slub_double_free_fops);
#endif
	return 0;
}

static void __exit hwslub_double_free_debugfs_exit(void)
{
	debugfs_remove(slub_double_free_root);
}

module_init(hwslub_double_free_debugfs_init);
module_exit(hwslub_double_free_debugfs_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Huawei double free debugfs");
