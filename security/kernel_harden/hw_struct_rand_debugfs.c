/*
 * Huawei struct layout randomize debugfs interface
 *
 * Copyright (c) 2018 Huawei co.ltd Copyright
 *
 * Authors:
 * yinyouzhan <yinyouzhan@huawei.com>
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

#ifdef CONFIG_GCC_PLUGIN_RANDSTRUCT_DEBUGFS
static struct dentry *struct_rand_root;
struct randdomize_struct_test_hw{
	unsigned long a1;
	unsigned char b2;
	unsigned int  c3;
	unsigned long d4;
	unsigned char e5;
	unsigned long f6;
	unsigned char g7;
	unsigned int  h8;
	unsigned long i9;
	unsigned char j10;
} __randomize_layout;

struct randdomize_ops_test_hw{
	void (*check)(void);
	void (*get_result)(void);
	void (*get_setting)(void);
	void (*set_setting)(void);
};


struct randdomize_struct_test_hw struct_randomize_sample = {
	.a1=1,
	.b2=2,
	.c3=3,
	.d4=4,
	.e5=5,
	.f6=6,
	.g7=7,
	.h8=8,
	.i9=9
};
struct randdomize_ops_test_hw ops_randomize_sample = {
	.check = NULL,
	.get_result = NULL,
	.get_setting = NULL,
	.set_setting = NULL
};
#define STRUCT_RAND_DEBUG_ENTRY(name) \
	static int struct_rand_##name##_open(struct inode *inode, struct file *file) \
{ \
	    return single_open(file, struct_rand_##name##_show, inode->i_private); \
} \
\
static const struct file_operations struct_rand_##name##_fops = { \
	    .owner = THIS_MODULE, \
	    .open = struct_rand_##name##_open, \
	    .read = seq_read, \
	    .llseek = seq_lseek, \
	    .release = single_release, \
}


static int struct_rand_offset_show(struct seq_file *m, void *v)
{
	seq_printf(m,"--struct_randomize display--\n original structure define is:\n\
	struct randdomize_struct_test_hw{\n\
		unsigned long a1;\n\
		unsigned char b2;\n\
		unsigned int  c3;\n\
		unsigned long d4;\n\
		unsigned char e5;\n\
		unsigned long f6;\n\
		unsigned char g7;\n\
		unsigned int  h8;\n\
		unsigned long i9;\n\
		unsigned char j10;\n\
	}__randomize_layout;\n\,actual memory address:\na1_addr=0x%p ,b2_addr=0x%p ,c3_addr=0x%p ,d4_addr=0x%p, e5_addr=0x%p, f6_addr=0x%p, g7_addr=0x%p, h8_addr=0x%p, i9_addr=0x%p, j10_addr=0x%p-----\n",
	&struct_randomize_sample.a1,&struct_randomize_sample.b2,&struct_randomize_sample.c3,&struct_randomize_sample.d4,&struct_randomize_sample.e5,
	&struct_randomize_sample.f6,&struct_randomize_sample.g7,&struct_randomize_sample.h8,&struct_randomize_sample.i9,&struct_randomize_sample.j10);

	seq_printf(m,"--ops_randomize display--\n original ops  define is:\n\
	struct randdomize_ops_test_hw{\n\
	void (*check)(void);\n\
	void (*get_result)(void);\n\
	void (*get_setting)(void);\n\
	void (*set_setting)(void);\n\
	}\n\,actual memory address:\ncheck=0x%p ,get_result=0x%p ,get_setting=0x%p ,set_setting=0x%p-----\n",
	&ops_randomize_sample.check,&ops_randomize_sample.get_result,&ops_randomize_sample.get_setting,&ops_randomize_sample.set_setting);

	return 0;
}

STRUCT_RAND_DEBUG_ENTRY(offset);

static int __init struct_rand_debugfs_init(void)
{
	/*create sys/kernel/debug/struct_rand for debugfs*/
	struct_rand_root = debugfs_create_dir("struct_rand", NULL);
	if (!struct_rand_root)
		return -ENOMEM;

	debugfs_create_file("offset",
			S_IRUGO,
			struct_rand_root,
			NULL,
			&struct_rand_offset_fops);


	return 0;
}

static void __exit struct_rand_debugfs_exit(void)
{
	debugfs_remove(struct_rand_root);
}

module_init(struct_rand_debugfs_init);
module_exit(struct_rand_debugfs_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HUAWEI STRUCT LAYOUT RANDOMIZE DEBUGFS");
#endif //CONFIG_GCC_PLUGIN_RANDSTRUCT_DEBUGFS
