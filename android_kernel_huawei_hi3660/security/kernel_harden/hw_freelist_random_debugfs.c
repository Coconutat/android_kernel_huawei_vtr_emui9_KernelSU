/*
 * Huawei freelist random debugfs interface
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
#define OO_MASK 65535
static struct dentry *slub_freelist_root;

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

#ifdef CONFIG_SLAB_FREELIST_RANDOM
static int slub_freelist_random_show_num(struct seq_file *m, void *v,unsigned int num)
{
    struct kmem_cache *my_cachep = kmem_cache_create("my_cache", num, 0, SLAB_HWCACHE_ALIGN, NULL);
    unsigned int object_num = ((my_cachep->oo).x & OO_MASK);
    unsigned int size = my_cachep->size;
    seq_printf(m, "The name of kmem_cache is %s\n", my_cachep->name);
    if(my_cachep->random_seq)
    {
        unsigned int i = 0;
        for(i = 0; i < object_num; i++)
        {
            seq_printf(m,"%s->random_seq[%d] is %d\n", my_cachep->name, i, my_cachep->random_seq[i]/size);
        }
    }
    kmem_cache_destroy(my_cachep);
    return 0;
}

static int slub_freelist_random_show(struct seq_file *m, void *v)
{
    /* create kmem_cache allocate object that length is 128bytes, 192bytes, 256bytes */
    slub_freelist_random_show_num(m,v,128);
    slub_freelist_random_show_num(m,v,192);
    slub_freelist_random_show_num(m,v,256);
    return 0;
}
#endif

#ifdef CONFIG_SLAB_FREELIST_RANDOM
SLUB_DEBUG_ENTRY(freelist_random);
#endif

static int __init hw_freelist_random_debugfs_init(void)
{
    /*create sys/kernel/debug/hwslub for debugfs*/
	slub_freelist_root = debugfs_create_dir("hwslub", NULL);
	if (!slub_freelist_root)
		return -ENOMEM;
#ifdef CONFIG_SLAB_FREELIST_RANDOM
	debugfs_create_file("freelist_random",
			S_IRUGO,
			slub_freelist_root,
			NULL,
			&slub_freelist_random_fops);
#endif
	return 0;
}

static void __exit hw_freelist_random_debugfs_exit(void)
{
	debugfs_remove(slub_freelist_root);
}

module_init(hw_freelist_random_debugfs_init);
module_exit(hw_freelist_random_debugfs_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Huawei Freelist Random debugfs");
