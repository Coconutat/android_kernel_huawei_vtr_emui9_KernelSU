/*
 * Huawei Hidden kernel symbols feature
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
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>

#define MAX_HIDESYMS_NUM	10
#define SYM_NAME_LEN		128

static DEFINE_MUTEX(hidesyms_lock);
LIST_HEAD(hidesyms_blacklist);

struct symbol_node {
	char *sym;
	struct list_head list;
};

static const char *hw_litehidesymbs_blacklist[MAX_HIDESYMS_NUM] = {
	"commit_creds",
	"prepare_kernel_cred"
};

static inline struct symbol_node *litehide_sym_node_alloc(void)
{
	struct symbol_node *node;

	node = (struct symbol_node *)kzalloc(sizeof(struct symbol_node), GFP_KERNEL);
	if (!node)
		return NULL;

	node->sym = kzalloc(SYM_NAME_LEN, GFP_KERNEL);
	if (!node->sym) {
		kfree(node);
		return NULL;
	}

	return node;
}

static inline void litehide_sym_node_free(struct symbol_node *node)
{
	if (node) {
		kfree(node->sym);
		kfree(node);
	}
}

/*Clear hidesyms_blacklist and free all buffer*/
static void clear_blacklist(void)
{
	struct symbol_node *node, *tmp;

	mutex_lock(&hidesyms_lock);
	list_for_each_entry_safe(node, tmp, &hidesyms_blacklist, list) {
		list_del(&node->list);

		kfree(node->sym);
		kfree(node);
	}
	mutex_unlock(&hidesyms_lock);
}

/*
 * search the symbol from the hidden symbols' blacklist.
 * if found, return the symbol_node; if not, return NULL.
 */
static inline struct symbol_node *find_sym_from_blacklist(const char *symbol)
{
	struct symbol_node *node = NULL;

	if (!symbol)
		return NULL;

	list_for_each_entry(node, &hidesyms_blacklist, list) {
		if (node->sym == NULL)
			continue;

		if (!strcmp(symbol, node->sym))
			return node;
	}

	return NULL;
}

/*
 * check if it's a hidden symbol in the blacklist
 */
bool is_hide_symbols(const char *symbol)
{
	struct symbol_node *hidesym_node;

	mutex_lock(&hidesyms_lock);

	hidesym_node = find_sym_from_blacklist(symbol);
	if (hidesym_node) {
		mutex_unlock(&hidesyms_lock);
		return true;
	}

	mutex_unlock(&hidesyms_lock);

	return false;
}
EXPORT_SYMBOL(is_hide_symbols);

#ifdef CONFIG_HUAWEI_HIDESYMS_DEBUGFS
static struct dentry *hidesym_d;

static int s_show(struct seq_file *file, void *v)
{
	const struct symbol_node *p = list_entry(v, struct symbol_node, list);
	if (p == NULL || p->sym == NULL)
		return -EINVAL;

	seq_printf(file, "%s\n", p->sym);

	return 0;
}

static void *s_start(struct seq_file *m, loff_t *pos)
{
	mutex_lock(&hidesyms_lock);
	return seq_list_start(&hidesyms_blacklist, *pos);
}

static void *s_next(struct seq_file *file, void *v, loff_t *pos)
{
	return seq_list_next(v, &hidesyms_blacklist, pos);
}

static void s_stop(struct seq_file *m, void *p)
{
	mutex_unlock(&hidesyms_lock);
}

static const struct seq_operations hide_syms_sops = {
	.start = s_start,
	.next = s_next,
	.stop = s_stop,
	.show = s_show,
};

static int parse_user_input(struct symbol_node *node, const char __user *ubuf,
		size_t cnt)
{
	char ch;
	size_t read = 0, index = 0;
	int ret = -1;

	ret = get_user(ch, ubuf++);
	if (ret)
		goto out;

	read++;
	cnt--;

	while (cnt && !isspace(ch)) {
		if (index < SYM_NAME_LEN - 1)
			node->sym[index++] = ch;
		else {
			ret = -EINVAL;
			goto out;
		}

		ret = get_user(ch, ubuf++);
		if (ret)
			goto out;

		read++;
		cnt--;
	}

	if (isspace(ch) || cnt == 0)
		node->sym[index] = '\0';
	else {
		ret = -EINVAL;
		goto out;
	}

	ret = read;

out:
	return ret;
}

static int litehide_syms_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &hide_syms_sops);
}

static ssize_t litehide_syms_write(struct file *filp, const char __user *ubuf,
	size_t cnt, loff_t *ppos)
{
	struct symbol_node *node;
	ssize_t write = 0;

	/* Clear the hidesym_blacklist by 'echo > litehide_syms' */
	if (cnt == 1 && *ubuf == '\n') {
		clear_blacklist();

		return cnt;
	}

	mutex_lock(&hidesyms_lock);

	while (cnt > write) {
		int count;

		node = litehide_sym_node_alloc();
		if (!node) {
			mutex_unlock(&hidesyms_lock);
			return -ENOMEM;
		}

		count = parse_user_input(node, ubuf + write, cnt - write);
		if (count > 0) {
			if (!find_sym_from_blacklist(node->sym))
				list_add_tail(&node->list, &hidesyms_blacklist);

			*ppos += count;
			write += count;
			count = 0;
		} else {
			litehide_sym_node_free(node);
			mutex_unlock(&hidesyms_lock);

			return -EINVAL;
		}
	}

	mutex_unlock(&hidesyms_lock);

	return write;
}

static const struct file_operations hide_syms_fops = {
	.owner = THIS_MODULE,
	.open = litehide_syms_open,
	.read = seq_read,
	.write = litehide_syms_write,
	.llseek = seq_lseek,
	.release = seq_release,
};
#endif

static __init int initialize_blacklist(void)
{
	int i;
	struct symbol_node *node;

	for (i = 0; i < MAX_HIDESYMS_NUM; i++) {
		if (!hw_litehidesymbs_blacklist[i])
			continue;

		node = litehide_sym_node_alloc();
		if (!node)
			return -ENOMEM;

		if (strlen(hw_litehidesymbs_blacklist[i]) > SYM_NAME_LEN - 1) {
			litehide_sym_node_free(node);
			return -EINVAL;
		}
		(void)strncpy(node->sym, hw_litehidesymbs_blacklist[i], strlen(hw_litehidesymbs_blacklist[i]));

		list_add_tail(&node->list, &hidesyms_blacklist);
	}

	return 0;
}

static int __init hw_litehide_symbols_init(void)
{
#ifdef CONFIG_HUAWEI_HIDESYMS_DEBUGFS
	/*create sys/kernel/debug/litehide_syms for debug*/
	hidesym_d = debugfs_create_file("litehide_syms", 0644, NULL, NULL, &hide_syms_fops);
	if (!hidesym_d)
		return -ENOMEM;
#endif
	initialize_blacklist();

	return 0;
}

static void __exit hw_litehide_symbols_exit(void)
{
#ifdef CONFIG_HUAWEI_HIDESYMS_DEBUGFS
	debugfs_remove(hidesym_d);
#endif
	clear_blacklist();
}

module_init(hw_litehide_symbols_init);
module_exit(hw_litehide_symbols_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Huawei hide symbols module");
