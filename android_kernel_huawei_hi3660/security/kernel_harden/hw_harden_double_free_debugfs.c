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
#include <linux/random.h>
#include <asm/memory.h>
#include <asm/module.h>
#include <asm/sections.h>

static struct dentry *slub_double_free_root;

#ifdef CONFIG_HW_SLUB_DF

#define MAX_LENGTH 16
#define USER_AREA_TEST 1
#define DOUBLE_FREE_TEST 2
#define PRESS_TEST 3
#define PRESS_TEST_TIMES 100000
#define LOOP_TIMES 10000
#define MAX_MALLOC_SIZE 4600
#define MAX_STACK_LENGTH 1000

static int slub_harden_double_free_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	return 0;
}

static ssize_t slub_harden_double_free_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
	char buf[MAX_LENGTH];
	memset(buf, 0, sizeof(buf));
	long result, ret;
	void *object = NULL;
	void *object1 = NULL;

	if (count <= 0 || count > MAX_LENGTH || copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
	{
		pr_err("double free test copy data from user failed\n");
		return -EFAULT;
	}

	/* convert the string to decimal number */
	ret = kstrtol(buf, 10, &result);

	if (ret)
		return ret;

	/* create kmem_cache allocate object which size is 128bytes */
	struct kmem_cache *my_cachep = kmem_cache_create("my_cache", 128, 0, SLAB_HWCACHE_ALIGN, NULL);

	/* Test when the data in object equals magic number hw_random_free */
	if (result == USER_AREA_TEST)
	{
		object = kmem_cache_alloc(my_cachep, GFP_KERNEL);
		if (NULL == object)
			return -ENOMEM;
		unsigned long *canary = (unsigned long *)(object + sizeof(void*));
		unsigned long random_free = my_cachep->hw_random_free;
		*canary = (random_free ^ (unsigned long)canary) & CANARY_MASK;
		kfree(object);
		object = NULL;
	}
	else if (result == DOUBLE_FREE_TEST) //Test normal double free
 	{
		object = kmem_cache_alloc(my_cachep, GFP_KERNEL);
		if (object == NULL) {
			return -ENOMEM;
		}
		object1 = kmem_cache_alloc(my_cachep, GFP_KERNEL);
		if (object1 == NULL) {
			kfree(object);
			object = NULL;
			return -ENOMEM;
		}
		kfree(object);
		kfree(object1);
		kfree(object);
		object = NULL;
		object1 = NULL;
	}
	else if (result == PRESS_TEST) //Press test
	{
		unsigned long count = 0;
		unsigned long random = 0;
		unsigned long pos = 0;
		unsigned long loop = 0;
		bool malloc_result = true;
		void **stack = kzalloc(sizeof(void*) * MAX_STACK_LENGTH, GFP_KERNEL);
		if (stack == NULL)
		{
			pr_err("harden double free press test kzalloc fail\n");
			return -ENOMEM;
		}
		for ( ;loop < LOOP_TIMES; loop++)
		{
			for (count = 0; count < PRESS_TEST_TIMES; count++)
			{
				pos = get_random_long() % MAX_STACK_LENGTH;
				if (stack[pos] == NULL)
				{
					random = get_random_long() % MAX_MALLOC_SIZE + 1;
					stack[pos] = kmalloc(random, GFP_KERNEL);
					if (stack[pos] == NULL)
					{
						pr_err("harden double free press test kmalloc fail\n");
						malloc_result = false;
						goto clear;
					}
				}
				else
				{
					kfree(stack[pos]);
					stack[pos] = NULL;
				}
			}
			msleep(10); //sleep 10ms
		}
clear:
		for (pos = 0; pos < MAX_STACK_LENGTH; pos++)
		{
			if (stack[pos] != NULL)
			{
				kfree(stack[pos]);
				stack[pos] = NULL;
			}
		}
		kfree(stack);
		if (malloc_result)
			pr_info("harden double free press test succ\n");
		else
			pr_err("harden double free press test fail\n");
	}
	kmem_cache_destroy(my_cachep);
	return count;
}

static ssize_t slub_harden_double_free_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	const char *enable = "ENABLE\n";
	const char *disable = "DISABLE\n";
	char buf[MAX_LENGTH];
	int ret;
	memset(buf, 0, sizeof(buf));

	/* create kmem_cache allocate object which size is 128bytes */
	struct kmem_cache *my_cachep = kmem_cache_create("my_cache", 128, 0, SLAB_HWCACHE_ALIGN, NULL);
	if (my_cachep->flags & SLAB_DOUBLEFREE_CHECK)
	{
		strncpy(buf, enable, strlen(enable) + 1);
	}
	else
	{
		strncpy(buf, disable, strlen(disable) + 1);
	}
	ret = simple_read_from_buffer(ubuf, count, ppos, buf, strlen(buf));
	kmem_cache_destroy(my_cachep);
	return ret;
}

static const struct file_operations slub_harden_double_free_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = slub_harden_double_free_read,
	.write = slub_harden_double_free_write,
};

#endif

static int __init hwslub_harden_double_free_debugfs_init(void)
{
    /*create sys/kernel/debug/hwslub_harden_double_free for debugfs*/
	slub_double_free_root = debugfs_create_dir("hwslub_harden_double_free", NULL);
	if (!slub_double_free_root)
		return -ENOMEM;
#ifdef CONFIG_HW_SLUB_DF
	debugfs_create_file("harden_double_free",
			S_IRUGO,
			slub_double_free_root,
			NULL,
			&slub_harden_double_free_fops);
#endif
	return 0;
}

static void __exit hwslub_harden_double_free_debugfs_exit(void)
{
	debugfs_remove(slub_double_free_root);
}

module_init(hwslub_harden_double_free_debugfs_init);
module_exit(hwslub_harden_double_free_debugfs_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Huawei harden double free debugfs");
