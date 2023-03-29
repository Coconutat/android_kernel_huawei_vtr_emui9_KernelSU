/* drivers/huawei_platform/uid_iostats.c
 *
 * Copyright (C) 2014 - 2015 Huawei, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/atomic.h>
#include <linux/err.h>
#include <linux/hashtable.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/profile.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/task_io_accounting_ops.h>

#ifdef CONFIG_HUAWEI_IO_MONITOR
#include <iotrace/io_monitor.h>
#endif

/*lint -save -e651 -e570 -e64 -e563 -e530 -e666 -e514 -e30 -e84 */
/*lint -save -e516 -e62 -e438 -e50 -e1058 -e529 -e40 -e528 -e574*/
#define UID_HASH_BITS	10
DECLARE_HASHTABLE(ioflowmeter_hash_table, UID_HASH_BITS);

static DEFINE_MUTEX(uid_lock);
static struct proc_dir_entry *parent;

static void ioflowmeter_add_uid_action(uid_t uid);
static void ioflowmeter_rm_uid_action(uid_t uid);

enum action_type {
	ACTION_REMOVE = 0,
	ACTION_ADD,
	ACTION_UNKNOWN
};

typedef void (*action_handler)(uid_t uid);

struct action_handler {
	enum action_type type;
	action_handler	func;
};


struct action_handler g_handlers[] = {
	{ACTION_REMOVE,	ioflowmeter_rm_uid_action},
	{ACTION_ADD,	ioflowmeter_add_uid_action},
	{ACTION_UNKNOWN,	NULL},
};

struct pid_node {
	pid_t pid;
	struct list_head list;
};

struct uid_entry {
	uid_t uid;
	u64 read_bytes;
	u64 write_bytes;
#ifdef CONFIG_HUAWEI_IO_MONITOR
	u64 fg_read_bytes;
	u64 fg_write_bytes;
#endif
	struct list_head pid_list;
	struct hlist_node hash;

#ifdef CONFIG_HUAWEI_IO_MONITOR
	u64 last_read_bytes;
	u64 last_write_bytes;
	u64 last_fg_read_bytes;
	u64 last_fg_write_bytes;
#endif
};

static int ioflowmeter_io_accounting(
			struct task_struct *task,
			struct task_io_accounting *acct,
			int whole)
{
	unsigned long flags;
	int result = 0;

	if (whole && lock_task_sighand(task, &flags)) {
		struct task_struct *t = task;

		task_io_accounting_add(acct, &task->signal->ioac);
		while_each_thread(task, t)
			task_io_accounting_add(acct, &t->ioac);

		unlock_task_sighand(task, &flags);
	}
	return result;
}

static void ioflowmeter_del_pid(struct uid_entry *uid_entry, pid_t pid)
{
	struct pid_node *cur, *saved_cur;

	if (NULL == uid_entry) {
		pr_err("%s: the uid_entry invalid\n", __func__);
		return;
	}

	list_for_each_entry_safe(cur, saved_cur, &uid_entry->pid_list, list) {
		if (cur && pid == cur->pid) {
			pr_debug("%s: the pid is %u\n", __func__, cur->pid);
			list_del(&cur->list);
			kfree(cur);
			break;
		}
	}

}

static void ioflowmeter_del_pid_list(struct uid_entry *uid_entry)
{
	struct pid_node *cur, *saved_cur;

	if (NULL == uid_entry) {
		pr_err("%s: the uid_entry invalid\n", __func__);
		return;
	}

	list_for_each_entry_safe(cur, saved_cur, &uid_entry->pid_list, list) {
		if (cur) {
			pr_debug("%s: the pid is %u\n", __func__, cur->pid);
			list_del(&cur->list);
			kfree(cur);
		}
	}

}

/* different pids in the list */
static void ioflowmeter_add_pid(struct uid_entry *uid_entry, pid_t the_pid)
{
	struct list_head *pre;
	struct pid_node *cur, *saved_cur;
	struct pid_node *the_node = NULL;

	pre = &uid_entry->pid_list;
	list_for_each_entry_safe(cur, saved_cur, &uid_entry->pid_list, list) {
		pr_debug("%s: the pid is %u\n", __func__, cur->pid);
		if (the_pid > cur->pid)
			break;
		else if (the_pid == cur->pid)
			return;

		pre = &cur->list;
	}

	the_node = kzalloc(sizeof(struct pid_node), GFP_KERNEL);
	if (NULL == the_node)
		return;

	the_node->pid = the_pid;

	list_add(&the_node->list, pre);

}

static struct uid_entry *ioflowmeter_find_uid(uid_t uid)
{
	struct uid_entry *uid_entry;

	hash_for_each_possible(ioflowmeter_hash_table, uid_entry, hash, uid) {
		if (uid_entry && uid_entry->uid == uid) {
			pr_debug("%s: find the uid_entry for %u\n",
					__func__, uid);
			return uid_entry;
		}
	}

	return NULL;
}

static struct uid_entry *ioflowmeter_register_uid(uid_t uid)
{
	struct uid_entry *uid_entry;

	uid_entry = ioflowmeter_find_uid(uid);
	if (uid_entry) {
		pr_debug("%s: find the uid_entry\n", __func__);
		return uid_entry;
	}

	uid_entry = kzalloc(sizeof(struct uid_entry), GFP_ATOMIC);
	if (!uid_entry)
		return NULL;

	uid_entry->uid = uid;
	INIT_LIST_HEAD(&uid_entry->pid_list);

	hash_add(ioflowmeter_hash_table, &uid_entry->hash, uid);

	return uid_entry;
}

static int ioflowmeter_show_stats(struct seq_file *m, void *v)
{
	struct uid_entry *uid_entry;
	unsigned long bkt;
	struct pid_node *cur, *saved_cur;
	struct task_struct *t = NULL;
	u64 pids_read_bytes = 0;
	u64 pids_write_bytes = 0;
	struct task_io_accounting acct;

	mutex_lock(&uid_lock);

	hash_for_each(ioflowmeter_hash_table, bkt, uid_entry, hash) {
		pids_read_bytes = 0;
		pids_write_bytes = 0;
		list_for_each_entry_safe(cur, saved_cur,
						&uid_entry->pid_list, list) {
			t = find_task_by_pid_ns(cur->pid, &init_pid_ns);
			if (t) {
				memset((void *)&acct, 0, sizeof(acct));
				acct = t->ioac;
				ioflowmeter_io_accounting(t, &acct, 1);
				pids_read_bytes += acct.read_bytes;
				pids_write_bytes += acct.write_bytes;
				pr_debug("%s: %d-%d: %llu %llu %llu %llu\n",
					__func__,
					uid_entry->uid, cur->pid,
					acct.read_bytes, acct.write_bytes,
					pids_read_bytes, pids_write_bytes);
			}
		}
		pr_debug("%s: %d: %llu %llu\n", __func__,
			uid_entry->uid, uid_entry->read_bytes,
			uid_entry->write_bytes);
		seq_printf(m, "%d: %llu %llu\n", uid_entry->uid,
			uid_entry->read_bytes + pids_read_bytes,
			uid_entry->write_bytes + pids_write_bytes);
	}

	mutex_unlock(&uid_lock);
	return 0;
}

static int ioflowmeter_show_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, ioflowmeter_show_stats, PDE_DATA(inode));
}

static const struct file_operations ioflowmeter_show_stats_fops = {
	.open		= ioflowmeter_show_stats_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void ioflowmeter_add_uid_action(uid_t uid)
{
	struct uid_entry *uid_entry;

	pr_debug("%s: the uid is %u\n", __func__, uid);

	mutex_lock(&uid_lock);

	uid_entry = ioflowmeter_register_uid(uid);
	if (!uid_entry) {
		pr_err("%s: failed to find the uid_entry for uid %u\n",
			__func__, uid);
	}

	mutex_unlock(&uid_lock);
}

static void ioflowmeter_rm_uid_action(uid_t uid)
{
	struct uid_entry *uid_entry = NULL;

	pr_debug("%s: the uid is %u\n", __func__, uid);

	mutex_lock(&uid_lock);

	uid_entry = ioflowmeter_find_uid(uid);
	if (uid_entry) {
		hash_del(&uid_entry->hash);
		ioflowmeter_del_pid_list(uid_entry);
		kfree(uid_entry);
		uid_entry = NULL;
	}

	mutex_unlock(&uid_lock);
}


static void ioflowmeter_uid_action(char *uids, int type)
{
	char *start_uid = NULL;
	char *end_uid = NULL;
	long int uid_start;


	if (!uids || '\0' == uids[0]) {
		pr_err("%s: the uids is invalid\n", __func__);
		return;
	}

	start_uid = &uids[0];

	do {
		end_uid = strstr(start_uid, ",");
		if (end_uid)
			*end_uid = '\0';

		if (kstrtol(start_uid, 10, &uid_start) != 0) {
			pr_err("%s: the input include invalid value\n",
						__func__);
			break;
		}

		g_handlers[type].func(uid_start);

		if (end_uid)
			start_uid = end_uid + 1;
		else
			start_uid = end_uid;

	} while (start_uid && *start_uid != '\0');
}

static int ioflowmeter_rm_uid_open(struct inode *inode, struct file *file)
{
	return single_open(file, NULL, NULL);
}

static ssize_t ioflowmeter_rm_uid_write(struct file *file,
			const char __user *buffer, size_t count, loff_t *ppos)
{
	char *uids = NULL;
	size_t uid_len = 0;

	if (count >= ((size_t) -1)) {
		pr_err("%s: the buffer from user is too big\n", __func__);
		return -EFAULT;
	}

	uid_len = count + 1;

	uids = kzalloc(uid_len, GFP_KERNEL);
	if (!uids)
		return -EFAULT;

	if (copy_from_user(uids, buffer, count)) {
		kfree(uids);
		return -EFAULT;
	}

	uids[uid_len - 1] = '\0';

	ioflowmeter_uid_action(uids, ACTION_REMOVE);

	kfree(uids);
	return count;
}

static const struct file_operations ioflowmeter_rm_uid_fops = {
	.open		= ioflowmeter_rm_uid_open,
	.release	= single_release,
	.write		= ioflowmeter_rm_uid_write,
};

static int ioflowmeter_add_uid_open(struct inode *inode, struct file *file)
{
	return single_open(file, NULL, NULL);
}

static ssize_t ioflowmeter_add_uid_write(struct file *file,
			const char __user *buffer, size_t count, loff_t *ppos)
{
	char *uids = NULL;
	size_t uid_len = 0;

	if (count >= ((size_t) -1)) {
		pr_err("%s: the buffer from user is too big\n", __func__);
		return -EFAULT;
	}
	uid_len = count + 1;

	uids = kzalloc(uid_len, GFP_KERNEL);
	if (!uids)
		return -EFAULT;

	if (copy_from_user(uids, buffer, count)) {
		pr_err("%s: cannot copy from user\n", __func__);
		kfree(uids);
		return -EFAULT;
	}

	uids[uid_len - 1] = '\0';

	ioflowmeter_uid_action(uids, ACTION_ADD);

	kfree(uids);
	return count;
}

static const struct file_operations ioflowmeter_add_uid_fops = {
	.open		= ioflowmeter_add_uid_open,
	.release	= single_release,
	.write		= ioflowmeter_add_uid_write,
};

static int process_exit_notifier(struct notifier_block *self,
			unsigned long cmd, void *v)
{
	struct task_struct *task = v;
	struct uid_entry *uid_entry = NULL;
	uid_t uid;
	struct task_io_accounting acct;

	if (!task || !thread_group_leader(task))
		return NOTIFY_OK;

	mutex_lock(&uid_lock);
	uid = from_kuid_munged(current_user_ns(), task_uid(task));
	uid_entry = ioflowmeter_find_uid(uid);
	if (!uid_entry)
		goto exit;

	memset((void *)&acct, 0, sizeof(struct task_io_accounting));
	acct = task->ioac;
	ioflowmeter_io_accounting(task, &acct, 1);
	pr_debug("%s: the task %s's pid is %d, tgid is %d, the uid is %d, group leader is %d\n",
			__func__,
			task->comm,
			task->pid, task->tgid, uid,
			thread_group_leader(task));

	pr_debug("%s: the task write_bytes is %llu\n",
					__func__,
					acct.write_bytes);

	uid_entry->read_bytes += acct.read_bytes;
	uid_entry->write_bytes += acct.write_bytes;
#ifdef CONFIG_HUAWEI_IO_MONITOR
	uid_entry->fg_read_bytes += acct.fg_read_bytes;
	uid_entry->fg_write_bytes += acct.fg_write_bytes;
#endif

	ioflowmeter_del_pid(uid_entry, task->pid);

exit:
	mutex_unlock(&uid_lock);
	return NOTIFY_OK;
}

static struct notifier_block process_exit_notifier_block = {
	.notifier_call	= process_exit_notifier,
};

static int process_entry_notifier(struct notifier_block *self,
			unsigned long cmd, void *v)
{
	struct task_struct *task = v;
	struct uid_entry *uid_entry = NULL;
	uid_t uid;

	if (!task || !thread_group_leader(task))
		return NOTIFY_OK;

	mutex_lock(&uid_lock);
	uid = from_kuid_munged(current_user_ns(), task_uid(task));
	uid_entry = ioflowmeter_find_uid(uid);
	if (!uid_entry)
		goto exit;

	ioflowmeter_add_pid(uid_entry, task->pid);
	pr_debug("%s: the task %s's pid is %d, tgid is %d, the uid is %d, group leader is %d\n",
				__func__,
				task->comm,
				task->pid, task->tgid, uid,
				thread_group_leader(task));


exit:
	mutex_unlock(&uid_lock);
	return NOTIFY_OK;
}

static struct notifier_block process_entry_notifier_block = {
	.notifier_call	= process_entry_notifier,
};
#ifdef CONFIG_HUAWEI_IO_MONITOR
struct io_flow_file_stat {
	u64 all_read;
	u64 all_write;
	u64 all_fg_read;
	u64 all_fg_write;
	u64 all_bg_read;
	u64 all_bg_write;
};

static int io_monitor_show_vfs(unsigned char *m, int len)
{
	struct uid_entry *uid_entry;
	unsigned long bkt;
	struct pid_node *cur, *saved_cur;
	struct task_struct *t = NULL;
	struct task_io_accounting acct;
	int ret = 0;

	struct io_flow_file_stat *ret_flow = NULL;

	if (len < sizeof(struct io_flow_file_stat))
		return 0;
	ret_flow = (struct io_flow_file_stat *)m;

	mutex_lock(&uid_lock);
	hash_for_each(ioflowmeter_hash_table, bkt, uid_entry, hash) {
		u64 uid_read_bytes = 0;
		u64 uid_write_bytes = 0;
		u64 uid_fg_read_bytes = 0;
		u64 uid_fg_write_bytes = 0;

		list_for_each_entry_safe(cur, saved_cur,
				&uid_entry->pid_list, list) {
			t = find_task_by_pid_ns(cur->pid, &init_pid_ns);
			if (t) {
				memset((void *)&acct, 0, sizeof(acct));
				acct = t->ioac;
				ioflowmeter_io_accounting(t, &acct, 1);
				uid_read_bytes += acct.read_bytes;
				uid_write_bytes += acct.write_bytes;
				uid_fg_read_bytes += acct.fg_read_bytes;
				uid_fg_write_bytes += acct.fg_write_bytes;
			}
		}

		uid_read_bytes += uid_entry->read_bytes;
		uid_write_bytes += uid_entry->write_bytes;
		uid_fg_read_bytes += uid_entry->fg_read_bytes;
		uid_fg_write_bytes += uid_entry->fg_write_bytes;

		ret_flow->all_read += (uid_read_bytes -
			uid_entry->last_read_bytes);
		ret_flow->all_write += (uid_write_bytes -
			uid_entry->last_write_bytes);
		ret_flow->all_fg_read += (uid_fg_read_bytes -
			uid_entry->last_fg_read_bytes);
		ret_flow->all_fg_write += (uid_fg_write_bytes -
			uid_entry->last_fg_write_bytes);

		uid_entry->last_read_bytes = uid_read_bytes;
		uid_entry->last_write_bytes = uid_write_bytes;
		uid_entry->last_fg_read_bytes = uid_fg_read_bytes;
		uid_entry->last_fg_write_bytes = uid_fg_write_bytes;

		ret = sizeof(struct io_flow_file_stat);
	}

	ret_flow->all_bg_read = (ret_flow->all_read - ret_flow->all_fg_read);
	ret_flow->all_bg_write = (ret_flow->all_write - ret_flow->all_fg_write);
	mutex_unlock(&uid_lock);

	return ret;
}

#define KB  (1024)
#define MB  (KB * KB)
#define GB  (KB * MB)

static int io_monitor_output(unsigned char *m, int len)
{
	struct io_flow_file_stat *ret_flow = (struct io_flow_file_stat *)m;

	pr_debug("IO_MON:all_read: %llu, all_write: %llu\n",
			ret_flow->all_read / MB,
			ret_flow->all_write / MB);
	pr_debug("IO_MON:all_fg_read: %llu, all_fg_write: %llu\n",
			ret_flow->all_fg_read / MB,
			ret_flow->all_fg_write / MB);
	pr_debug("IO_MON:all_bg_read: %llu, all_bg_write: %llu\n",
			ret_flow->all_bg_read / MB,
			ret_flow->all_bg_write / MB);

	return 0;
}
static int io_monitor_set_vfs_param(struct imonitor_eventobj *obj,
		unsigned char *buff)
{
	int ret = 0;
	struct io_flow_file_stat *ret_flow = NULL;

	ret_flow = (struct io_flow_file_stat *)buff;
	ret = ret | imonitor_set_param(obj,
		E914008000_APP_TOTAL_READ_INT, ret_flow->all_read / MB);
	ret = ret | imonitor_set_param(obj,
		E914008000_APP_TOTAL_WRITE_INT, ret_flow->all_write / MB);
	ret = ret | imonitor_set_param(obj,
		E914008000_APP_TOTAL_READ_FG_INT, ret_flow->all_fg_read / MB);
	ret = ret | imonitor_set_param(obj,
		E914008000_APP_TOTAL_WRITE_FG_INT, ret_flow->all_fg_write / MB);
	ret = ret | imonitor_set_param(obj,
		E914008000_APP_TOTAL_READ_BG_INT, ret_flow->all_bg_read / MB);
	ret = ret | imonitor_set_param(obj,
		E914008000_APP_TOTAL_WRITE_BG_INT, ret_flow->all_bg_write / MB);

	return ret;
}

#ifdef CONFIG_HUAWEI_IO_TRACING
struct vfs_io_iotrace {
	u32 uid;
	u64 uid_read_bytes;
	u64 uid_write_bytes;
	u64 uid_fg_read_bytes;
	u64 uid_fg_write_bytes;
	u64 uid_bg_read_bytes;
	u64 uid_bg_write_bytes;
};

int vfs_report_to_iotrace(unsigned char *buff, int len)
{
	struct uid_entry *uid_entry;
	unsigned long bkt;
	struct pid_node *cur, *saved_cur;
	struct task_struct *t = NULL;
	struct task_io_accounting acct;
	int i, min_index = 0, cnt = 0;
	struct vfs_io_iotrace vfs_iotrace;
	struct vfs_io_iotrace *begin = (struct vfs_io_iotrace *)buff;
	int ret = sizeof(struct vfs_io_iotrace) * 10;

	if (ret > len)
		return 0;

	memset(buff, 0, ret);
	mutex_lock(&uid_lock);
	hash_for_each(ioflowmeter_hash_table, bkt, uid_entry, hash) {
		struct vfs_io_iotrace *v, *min;

		memset(&vfs_iotrace, 0, sizeof(struct vfs_io_iotrace));
		vfs_iotrace.uid = uid_entry->uid;
		list_for_each_entry_safe(cur, saved_cur,
			&uid_entry->pid_list, list) {
			t = find_task_by_pid_ns(cur->pid, &init_pid_ns);
			if (t) {
				memset((void *)&acct, 0, sizeof(acct));
				acct = t->ioac;
				ioflowmeter_io_accounting(t, &acct, 1);
				vfs_iotrace.uid_read_bytes +=
					acct.read_bytes;
				vfs_iotrace.uid_write_bytes +=
					acct.write_bytes;
				vfs_iotrace.uid_fg_read_bytes +=
					acct.fg_read_bytes;
				vfs_iotrace.uid_fg_write_bytes +=
					acct.fg_write_bytes;
			}
		}
		vfs_iotrace.uid_write_bytes += uid_entry->write_bytes;
		if (vfs_iotrace.uid_write_bytes == 0)
			continue;
		vfs_iotrace.uid_read_bytes += uid_entry->read_bytes;
		vfs_iotrace.uid_fg_read_bytes  += uid_entry->fg_read_bytes;
		vfs_iotrace.uid_fg_write_bytes += uid_entry->fg_write_bytes;
		vfs_iotrace.uid_bg_read_bytes  = vfs_iotrace.uid_read_bytes -
			vfs_iotrace.uid_fg_read_bytes;
		vfs_iotrace.uid_bg_write_bytes = vfs_iotrace.uid_write_bytes -
			vfs_iotrace.uid_fg_write_bytes;
		/*write bytes continue*/
		min = begin + min_index;
		if (cnt < 10) {
			v = begin + cnt;
			*v = vfs_iotrace;
			if (v->uid_write_bytes <= min->uid_write_bytes)
				min_index = cnt;
			cnt++;
			continue;
		}
		/*large than 10*/
		if (min->uid_write_bytes >= vfs_iotrace.uid_write_bytes)
			continue;

		*min = vfs_iotrace;
		for (i = 0; i < cnt; i++) {
			v = begin + i;
			if (v->uid_write_bytes < min->uid_write_bytes)
				min_index = i;
		}
	}
	mutex_unlock(&uid_lock);

	return ret;
}
EXPORT_SYMBOL(vfs_report_to_iotrace);
#endif

static struct io_module_template ioflow = {
	.mod_id = IO_MONITOR_VFS,
	.event_id = 914008000,
	.base_interval = 3600000, /*ms*/
	.ops = {
		.log_record = io_monitor_show_vfs,
		.log_output = io_monitor_output,
		.log_set_param = io_monitor_set_vfs_param,
		.log_upload = NULL,
	},
};

#endif

static int __init proc_ioflowmeter_init(void)
{
	hash_init(ioflowmeter_hash_table);

	parent = proc_mkdir_mode("uid_iostats",
					S_IRUSR | S_IXUSR | S_IRGRP
					| S_IXGRP | S_IROTH | S_IXOTH,
					NULL);
	if (!parent) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return -ENOMEM;
	}
	proc_create_data("uid_iomonitor_list",
			S_IWUSR | S_IWGRP, parent, &ioflowmeter_add_uid_fops,
			NULL);

	proc_create_data("remove_uid_list",
			S_IWUSR | S_IWGRP, parent, &ioflowmeter_rm_uid_fops,
			NULL);

	proc_create_data("show_uid_iostats",
			S_IRUSR | S_IRGRP, parent, &ioflowmeter_show_stats_fops,
			NULL);

	profile_event_register(PROFILE_TASK_EXIT,
			&process_exit_notifier_block);
	profile_event_register(PROFILE_TASK_END_FORK,
			&process_entry_notifier_block);
	profile_event_register(PROFILE_END_SETRESUID,
			&process_entry_notifier_block);
#ifdef CONFIG_HUAWEI_IO_MONITOR
	io_monitor_mod_register(IO_MONITOR_VFS, &ioflow);
#endif
	return 0;
}
/*lint -restore*/
/*lint -restore*/
/*lint -e528 -esym(528,*)*/
early_initcall(proc_ioflowmeter_init);
/*lint -e528 +esym(528,*)*/
