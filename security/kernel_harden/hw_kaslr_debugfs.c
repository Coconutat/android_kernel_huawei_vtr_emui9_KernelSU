/*
 * Huawei KASLR debugfs interface
 *
 * Copyright (c) 2017 Huawei.
 *
 * Authors:
 * Weilai Zhou <zhouweilai@huawei.com>
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

#include <asm/memory.h>
#include <asm/module.h>
#include <asm/sections.h>

static struct dentry *kaslr_root;

#ifdef CONFIG_RANDOMIZE_BASE
static u64 img_offset;
static u16 linear_seed;
extern u16 __initdata memstart_offset_seed;
#endif

#define KASLR_DEBUG_ENTRY(name) \
	static int kaslr_##name##_open(struct inode *inode, struct file *file) \
{ \
	    return single_open(file, kaslr_##name##_show, inode->i_private); \
} \
\
static const struct file_operations kaslr_##name##_fops = { \
	    .owner = THIS_MODULE, \
	    .open = kaslr_##name##_open, \
	    .read = seq_read, \
	    .llseek = seq_lseek, \
	    .release = single_release, \
}

#ifdef CONFIG_HUAWEI_KERNEL_STACK_RANDOMIZE
static int kaslr_kstack_offset_show(struct seq_file *m, void *v)
{
	seq_printf(m, "0x%x\n", kstack_offset);

	return 0;
}
#endif

#ifdef CONFIG_HUAWEI_KERNEL_STACK_RANDOMIZE_STRONG
static int kaslr_kti_offset_show(struct seq_file *m, void *v)
{
	seq_printf(m, "0x%lx\n", kti_offset);

	return 0;
}
#endif

#ifdef CONFIG_RANDOMIZE_BASE
static int kaslr_img_offset_show(struct seq_file *m, void *v)
{
	seq_printf(m, "0x%llx\n", img_offset);

	return 0;
}

static int kaslr_module_offset_show(struct seq_file *m, void *v)
{
	u64 module_offset, original_base;

	if (IS_ENABLED(CONFIG_RANDOMIZE_MODULE_REGION_FULL))
		original_base = VMALLOC_START;
	else
		original_base = (u64)_etext - MODULES_VSIZE;

	module_offset = module_alloc_base - original_base;
	seq_printf(m, "0x%llx\n", module_offset);

	return 0;
}

static int kaslr_linear_seed_show(struct seq_file *m, void *v)
{
	seq_printf(m, "0x%x\n", linear_seed);

	return 0;
}
#endif

#ifdef CONFIG_HUAWEI_KERNEL_STACK_RANDOMIZE
KASLR_DEBUG_ENTRY(kstack_offset);
#endif
#ifdef CONFIG_HUAWEI_KERNEL_STACK_RANDOMIZE_STRONG
KASLR_DEBUG_ENTRY(kti_offset);
#endif
#ifdef CONFIG_RANDOMIZE_BASE
KASLR_DEBUG_ENTRY(img_offset);
KASLR_DEBUG_ENTRY(module_offset);
KASLR_DEBUG_ENTRY(linear_seed);
#endif

static int __init hw_kaslr_debugfs_init(void)
{
	/*create sys/kernel/debug/kaslr for debugfs*/
	kaslr_root = debugfs_create_dir("kaslr", NULL);
	if (!kaslr_root)
		return -ENOMEM;

#ifdef CONFIG_HUAWEI_KERNEL_STACK_RANDOMIZE
	debugfs_create_file("kstack_offset",
			S_IRUGO,
			kaslr_root,
			NULL,
			&kaslr_kstack_offset_fops);
#endif

#ifdef CONFIG_HUAWEI_KERNEL_STACK_RANDOMIZE_STRONG
	debugfs_create_file("kti_offset",
			S_IRUGO,
			kaslr_root,
			NULL,
			&kaslr_kti_offset_fops);
#endif

#ifdef CONFIG_RANDOMIZE_BASE
	debugfs_create_file("img_offset",
			S_IRUGO,
			kaslr_root,
			NULL,
			&kaslr_img_offset_fops);

	debugfs_create_file("module_offset",
			S_IRUGO,
			kaslr_root,
			NULL,
			&kaslr_module_offset_fops);

	debugfs_create_file("linear_seed",
			S_IRUGO,
			kaslr_root,
			NULL,
			&kaslr_linear_seed_fops);

	img_offset = kimage_vaddr - KIMAGE_VADDR;
	linear_seed = memstart_offset_seed;
#endif

	return 0;
}

static void __exit hw_kaslr_debugfs_exit(void)
{
	debugfs_remove(kaslr_root);
}

module_init(hw_kaslr_debugfs_init);
module_exit(hw_kaslr_debugfs_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Huawei KASLR debugfs");
