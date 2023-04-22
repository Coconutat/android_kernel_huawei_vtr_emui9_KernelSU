/*
 * slowpath_count.c
 *
 * count front and visible app into slow path page_alloc times.
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * Chenjun <chenjun@hisilicon.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/cred.h>
#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/seq_file.h>
#include <slowpath_count.h>

#define FIRST_APP_UID KUIDT_INIT(10000)
#define LAST_APP_UID  KUIDT_INIT(19999)

#define VISIBLE_APP_ADJ    (1)
#define FOREGROUND_APP_ADJ (0)

static u64 last_jiffs;
static pid_t prev_pid;
static bool enable = false;
atomic_long_t pgalloc_count = ATOMIC_LONG_INIT(0);
atomic_long_t slowpath_pgalloc_count[ORDER_LIMIT] = {ATOMIC_LONG_INIT(0)};
EXPORT_SYMBOL(slowpath_pgalloc_count);

module_param_named(enable, enable, bool, S_IRUGO | S_IWUSR);

static int is_background(void)
{
	kuid_t uid;
	int adj;
	struct mm_struct *mm;

	adj = current->signal->oom_score_adj;
	mm = current->mm;
	uid = current_uid();/*lint !e666*/

	if (!mm || uid_lt(uid, FIRST_APP_UID) || uid_gt(uid, LAST_APP_UID))
		return 1;
	if (adj != VISIBLE_APP_ADJ && adj != FOREGROUND_APP_ADJ)
		return 1;

	return 0;
}

/*
 * pgalloc_count_inc - count slow path page_alloc times
 * @is_slowpath: slow path or not
 * @order: page_order
 *
 * This function count slow path page_alloc times and total page_alloc times.
 */
void pgalloc_count_inc(bool is_slowpath, unsigned int order)
{
	u64 jiffs = 0;

	if (!enable || is_background())
		return;

	if (is_slowpath) {
		jiffs = get_jiffies_64();
		if (current->pid != prev_pid ||
		    time_after64(jiffs, last_jiffs + HZ)) {
			last_jiffs = get_jiffies_64();
			pr_err("enter slow path page alloc,order=%u\n", order);
		}

		/*sum up all the order larger than 4-order as one*/
		if (order > ORDER_LIMIT - 1)
			order = ORDER_LIMIT - 1;
		atomic_long_inc(&slowpath_pgalloc_count[order]);
		prev_pid = current->pid;
	} else {
		atomic_long_inc(&pgalloc_count);
	}
}
EXPORT_SYMBOL(pgalloc_count_inc);

static int slowpath_count_show(struct seq_file *s, void *unused)
{
	int i;
	long sum = 0;

	for (i = 0; i < ORDER_LIMIT; i++)
		sum += atomic_long_read(&slowpath_pgalloc_count[i]);

	seq_printf(s, "Total page alloc count:%ld\n",
		   atomic_long_read(&pgalloc_count));
	seq_printf(s, "Total slow path page alloc count:%ld\n", sum);
	return 0;
}

static int slowpath_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, slowpath_count_show, NULL);
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = slowpath_count_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init slowpath_count_init(void)
{
	debugfs_create_file("slowpath_count", 0444, NULL, NULL, &fops);
	return 0;
}

module_init(slowpath_count_init);
MODULE_LICENSE("GPL");
