/* drivers/misc/uid_sys_stats.c
 *
 * Copyright (C) 2014 - 2015 Google, Inc.
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
#include <linux/cpufreq_times.h>
#include <linux/err.h>
#include <linux/hashtable.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/profile.h>
#include <linux/rtmutex.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/blk-cgroup.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <log/log_usertype/log-usertype.h>

#define UID_HASH_BITS	10
DECLARE_HASHTABLE(hash_table, UID_HASH_BITS);

static DEFINE_RT_MUTEX(uid_lock);
static struct proc_dir_entry *cpu_parent;
static struct proc_dir_entry *io_parent;
static struct proc_dir_entry *proc_parent;

struct io_stats {
	u64 read_bytes;
	u64 write_bytes;
	u64 rchar;
	u64 wchar;
	u64 fsync;
};

#define UID_STATE_FOREGROUND	0
#define UID_STATE_BACKGROUND	1
#define UID_STATE_BUCKET_SIZE	2

#define UID_STATE_TOTAL_CURR	2
#define UID_STATE_TOTAL_LAST	3
#define UID_STATE_DEAD_TASKS	4
#define UID_STATE_SIZE		5

#define MAX_TASK_COMM_LEN 256

#define PID_NODE_CNT_WARNING_THRESHOLD 5000
#define PID_ENTRY_CNT_WARNING_THRESHOLD 1000

struct task_entry {
	char comm[MAX_TASK_COMM_LEN];
	pid_t pid;
	struct io_stats io[UID_STATE_SIZE];
	struct hlist_node hash;
};

struct uid_entry {
	uid_t uid;
	cputime_t utime;
	cputime_t stime;
	cputime_t active_utime;
	cputime_t active_stime;
#ifdef CONFIG_CPU_FREQ_POWER_STAT
	unsigned long long active_power;
	unsigned long long power;
#endif
	int state;
	struct io_stats io[UID_STATE_SIZE];
	struct hlist_node hash;
#ifdef CONFIG_UID_SYS_STATS_DEBUG
	DECLARE_HASHTABLE(task_entries, UID_HASH_BITS);
#endif
};

static u64 compute_write_bytes(struct task_struct *task)
{
	if (task->ioac.write_bytes <= task->ioac.cancelled_write_bytes)
		return 0;

	return task->ioac.write_bytes - task->ioac.cancelled_write_bytes;
}

static void compute_io_bucket_stats(struct io_stats *io_bucket,
					struct io_stats *io_curr,
					struct io_stats *io_last,
					struct io_stats *io_dead)
{
	/* tasks could switch to another uid group, but its io_last in the
	 * previous uid group could still be positive.
	 * therefore before each update, do an overflow check first
	 */
	int64_t delta;

	delta = io_curr->read_bytes + io_dead->read_bytes -
		io_last->read_bytes;
	io_bucket->read_bytes += delta > 0 ? delta : 0;
	delta = io_curr->write_bytes + io_dead->write_bytes -
		io_last->write_bytes;
	io_bucket->write_bytes += delta > 0 ? delta : 0;
	delta = io_curr->rchar + io_dead->rchar - io_last->rchar;
	io_bucket->rchar += delta > 0 ? delta : 0;
	delta = io_curr->wchar + io_dead->wchar - io_last->wchar;
	io_bucket->wchar += delta > 0 ? delta : 0;
	delta = io_curr->fsync + io_dead->fsync - io_last->fsync;
	io_bucket->fsync += delta > 0 ? delta : 0;

	io_last->read_bytes = io_curr->read_bytes;
	io_last->write_bytes = io_curr->write_bytes;
	io_last->rchar = io_curr->rchar;
	io_last->wchar = io_curr->wchar;
	io_last->fsync = io_curr->fsync;

	memset(io_dead, 0, sizeof(struct io_stats));
}

#ifdef CONFIG_UID_SYS_STATS_DEBUG
static void get_full_task_comm(struct task_entry *task_entry,
		struct task_struct *task)
{
	int i = 0, offset = 0, len = 0;
	/* save one byte for terminating null character */
	int unused_len = MAX_TASK_COMM_LEN - TASK_COMM_LEN - 1;
	char buf[unused_len];
	struct mm_struct *mm = task->mm;

	/* fill the first TASK_COMM_LEN bytes with thread name */
	__get_task_comm(task_entry->comm, TASK_COMM_LEN, task);
	i = strlen(task_entry->comm);
	while (i < TASK_COMM_LEN)
		task_entry->comm[i++] = ' ';

	/* next the executable file name */
	if (mm) {
		down_read(&mm->mmap_sem);
		if (mm->exe_file) {
			char *pathname = d_path(&mm->exe_file->f_path, buf,
					unused_len);

			if (!IS_ERR(pathname)) {
				len = strlcpy(task_entry->comm + i, pathname,
						unused_len);
				i += len;
				task_entry->comm[i++] = ' ';
				unused_len--;
			}
		}
		up_read(&mm->mmap_sem);
	}
	unused_len -= len;

	/* fill the rest with command line argument
	 * replace each null or new line character
	 * between args in argv with whitespace */
	len = get_cmdline(task, buf, unused_len);
	while (offset < len) {
		if (buf[offset] != '\0' && buf[offset] != '\n')
			task_entry->comm[i++] = buf[offset];
		else
			task_entry->comm[i++] = ' ';
		offset++;
	}

	/* get rid of trailing whitespaces in case when arg is memset to
	 * zero before being reset in userspace
	 */
	while (task_entry->comm[i-1] == ' ')
		i--;
	task_entry->comm[i] = '\0';
}

static struct task_entry *find_task_entry(struct uid_entry *uid_entry,
		struct task_struct *task)
{
	struct task_entry *task_entry;

	hash_for_each_possible(uid_entry->task_entries, task_entry, hash,
			task->pid) {
		if (task->pid == task_entry->pid) {
			/* if thread name changed, update the entire command */
			int len = strnchr(task_entry->comm, ' ', TASK_COMM_LEN)
				- task_entry->comm;

			if (strncmp(task_entry->comm, task->comm, len))
				get_full_task_comm(task_entry, task);
			return task_entry;
		}
	}
	return NULL;
}

static struct task_entry *find_or_register_task(struct uid_entry *uid_entry,
		struct task_struct *task)
{
	struct task_entry *task_entry;
	pid_t pid = task->pid;

	task_entry = find_task_entry(uid_entry, task);
	if (task_entry)
		return task_entry;

	task_entry = kzalloc(sizeof(struct task_entry), GFP_ATOMIC);
	if (!task_entry)
		return NULL;

	get_full_task_comm(task_entry, task);

	task_entry->pid = pid;
	hash_add(uid_entry->task_entries, &task_entry->hash, (unsigned int)pid);

	return task_entry;
}

static void remove_uid_tasks(struct uid_entry *uid_entry)
{
	struct task_entry *task_entry;
	unsigned long bkt_task;
	struct hlist_node *tmp_task;

	hash_for_each_safe(uid_entry->task_entries, bkt_task,
			tmp_task, task_entry, hash) {
		hash_del(&task_entry->hash);
		kfree(task_entry);
	}
}

static void set_io_uid_tasks_zero(struct uid_entry *uid_entry)
{
	struct task_entry *task_entry;
	unsigned long bkt_task;

	hash_for_each(uid_entry->task_entries, bkt_task, task_entry, hash) {
		memset(&task_entry->io[UID_STATE_TOTAL_CURR], 0,
			sizeof(struct io_stats));
	}
}

static void add_uid_tasks_io_stats(struct uid_entry *uid_entry,
		struct task_struct *task, int slot)
{
	struct task_entry *task_entry = find_or_register_task(uid_entry, task);
	struct io_stats *task_io_slot = &task_entry->io[slot];

	task_io_slot->read_bytes += task->ioac.read_bytes;
	task_io_slot->write_bytes += compute_write_bytes(task);
	task_io_slot->rchar += task->ioac.rchar;
	task_io_slot->wchar += task->ioac.wchar;
	task_io_slot->fsync += task->ioac.syscfs;
}

static void compute_io_uid_tasks(struct uid_entry *uid_entry)
{
	struct task_entry *task_entry;
	unsigned long bkt_task;

	hash_for_each(uid_entry->task_entries, bkt_task, task_entry, hash) {
		compute_io_bucket_stats(&task_entry->io[uid_entry->state],
					&task_entry->io[UID_STATE_TOTAL_CURR],
					&task_entry->io[UID_STATE_TOTAL_LAST],
					&task_entry->io[UID_STATE_DEAD_TASKS]);
	}
}

static void show_io_uid_tasks(struct seq_file *m, struct uid_entry *uid_entry)
{
	struct task_entry *task_entry;
	unsigned long bkt_task;

	hash_for_each(uid_entry->task_entries, bkt_task, task_entry, hash) {
		/* Separated by comma because space exists in task comm */
		seq_printf(m, "task,%s,%lu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
				task_entry->comm,
				(unsigned long)task_entry->pid,
				task_entry->io[UID_STATE_FOREGROUND].rchar,
				task_entry->io[UID_STATE_FOREGROUND].wchar,
				task_entry->io[UID_STATE_FOREGROUND].read_bytes,
				task_entry->io[UID_STATE_FOREGROUND].write_bytes,
				task_entry->io[UID_STATE_BACKGROUND].rchar,
				task_entry->io[UID_STATE_BACKGROUND].wchar,
				task_entry->io[UID_STATE_BACKGROUND].read_bytes,
				task_entry->io[UID_STATE_BACKGROUND].write_bytes,
				task_entry->io[UID_STATE_FOREGROUND].fsync,
				task_entry->io[UID_STATE_BACKGROUND].fsync);
	}
}
#else
static void remove_uid_tasks(struct uid_entry *uid_entry) {};
static void set_io_uid_tasks_zero(struct uid_entry *uid_entry) {};
static void add_uid_tasks_io_stats(struct uid_entry *uid_entry,
		struct task_struct *task, int slot) {};
static void compute_io_uid_tasks(struct uid_entry *uid_entry) {};
static void show_io_uid_tasks(struct seq_file *m,
		struct uid_entry *uid_entry) {}
#endif

static struct uid_entry *find_uid_entry(uid_t uid)
{
	struct uid_entry *uid_entry;
	hash_for_each_possible(hash_table, uid_entry, hash, uid) {
		if (uid_entry->uid == uid)
			return uid_entry;
	}
	return NULL;
}

static struct uid_entry *find_or_register_uid(uid_t uid)
{
	struct uid_entry *uid_entry;

	uid_entry = find_uid_entry(uid);
	if (uid_entry)
		return uid_entry;

	uid_entry = kzalloc(sizeof(struct uid_entry), GFP_ATOMIC);
	if (!uid_entry)
		return NULL;

	uid_entry->uid = uid;
#ifdef CONFIG_UID_SYS_STATS_DEBUG
	hash_init(uid_entry->task_entries);
#endif
	hash_add(hash_table, &uid_entry->hash, uid);

	return uid_entry;
}

static int uid_cputime_show(struct seq_file *m, void *v)
{
	struct uid_entry *uid_entry = NULL;
	struct task_struct *task, *temp;
	struct user_namespace *user_ns = current_user_ns();
	cputime_t utime;
	cputime_t stime;
	unsigned long bkt;
	uid_t uid;

	rt_mutex_lock(&uid_lock);

	hash_for_each(hash_table, bkt, uid_entry, hash) {
		uid_entry->active_stime = 0;
		uid_entry->active_utime = 0;
#ifdef CONFIG_CPU_FREQ_POWER_STAT
		uid_entry->active_power = 0;
#endif
	}

	rcu_read_lock();
	do_each_thread(temp, task) {
		uid = from_kuid_munged(user_ns, task_uid(task));
		if (!uid_entry || uid_entry->uid != uid)
			uid_entry = find_or_register_uid(uid);
		if (!uid_entry) {
			rcu_read_unlock();
			rt_mutex_unlock(&uid_lock);
			pr_err("%s: failed to find the uid_entry for uid %d\n",
				__func__, uid);
			return -ENOMEM;
		}
#ifdef CONFIG_CPU_FREQ_POWER_STAT
		/* if this task is exiting, we have already accounted for the
		 * time and power.
		 */
		if (task->cpu_power == ULLONG_MAX)
			continue;
#endif
		task_cputime_adjusted(task, &utime, &stime);
		uid_entry->active_utime += utime;
		uid_entry->active_stime += stime;
#ifdef CONFIG_CPU_FREQ_POWER_STAT
		uid_entry->active_power += task->cpu_power;
#endif
	} while_each_thread(temp, task);
	rcu_read_unlock();

	hash_for_each(hash_table, bkt, uid_entry, hash) {
		cputime_t total_utime = uid_entry->utime +
							uid_entry->active_utime;
		cputime_t total_stime = uid_entry->stime +
							uid_entry->active_stime;
#ifdef CONFIG_CPU_FREQ_POWER_STAT
		unsigned long long total_power = uid_entry->power +
							uid_entry->active_power;
		seq_printf(m, "%d: %llu %llu %llu\n", uid_entry->uid,
			(unsigned long long)jiffies_to_msecs(
				cputime_to_jiffies(total_utime)) * USEC_PER_MSEC,
			(unsigned long long)jiffies_to_msecs(
				cputime_to_jiffies(total_stime)) * USEC_PER_MSEC,
			total_power);
#else
		seq_printf(m, "%d: %llu %llu\n", uid_entry->uid,
			(unsigned long long)jiffies_to_msecs(
				cputime_to_jiffies(total_utime)) * USEC_PER_MSEC,
			(unsigned long long)jiffies_to_msecs(
				cputime_to_jiffies(total_stime)) * USEC_PER_MSEC);
#endif
	}

	rt_mutex_unlock(&uid_lock);
	return 0;
}

static int uid_cputime_open(struct inode *inode, struct file *file)
{
	return single_open(file, uid_cputime_show, PDE_DATA(inode));
}

static const struct file_operations uid_cputime_fops = {
	.open		= uid_cputime_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int uid_remove_open(struct inode *inode, struct file *file)
{
	return single_open(file, NULL, NULL);
}

static ssize_t uid_remove_write(struct file *file,
			const char __user *buffer, size_t count, loff_t *ppos)
{
	struct uid_entry *uid_entry;
	struct hlist_node *tmp;
	char uids[128];
	unsigned long bkt;
	char *start_uid, *end_uid = NULL;
	long int uid_start = 0, uid_end = 0;

	if (count >= sizeof(uids))
		count = sizeof(uids) - 1;

	if (copy_from_user(uids, buffer, count))
		return -EFAULT;

	uids[count] = '\0';
	end_uid = uids;
	start_uid = strsep(&end_uid, "-");

	if (!start_uid || !end_uid)
		return -EINVAL;

	if (kstrtol(start_uid, 10, &uid_start) != 0 ||
		kstrtol(end_uid, 10, &uid_end) != 0 ||
		uid_start > uid_end) {
		return -EINVAL;
	}

	/* Also remove uids from /proc/uid_time_in_state */
	cpufreq_task_times_remove_uids(uid_start, uid_end);

	rt_mutex_lock(&uid_lock);

	hash_for_each_safe(hash_table, bkt, tmp, uid_entry, hash) {
		if (uid_entry->uid >= uid_start && uid_entry->uid <= uid_end) {
			remove_uid_tasks(uid_entry);
			hash_del(&uid_entry->hash);
			kfree(uid_entry);
		}
	}

	rt_mutex_unlock(&uid_lock);
	return count;
}

static const struct file_operations uid_remove_fops = {
	.open		= uid_remove_open,
	.release	= single_release,
	.write		= uid_remove_write,
};


static void add_uid_io_stats(struct uid_entry *uid_entry,
			struct task_struct *task, int slot)
{
	struct io_stats *io_slot = &uid_entry->io[slot];

	io_slot->read_bytes += task->ioac.read_bytes;
	io_slot->write_bytes += compute_write_bytes(task);
	io_slot->rchar += task->ioac.rchar;
	io_slot->wchar += task->ioac.wchar;
	io_slot->fsync += task->ioac.syscfs;

	add_uid_tasks_io_stats(uid_entry, task, slot);
}

static void update_io_stats_all_locked(void)
{
	struct uid_entry *uid_entry = NULL;
	struct task_struct *task, *temp;
	struct user_namespace *user_ns = current_user_ns();
	unsigned long bkt;
	uid_t uid;

	hash_for_each(hash_table, bkt, uid_entry, hash) {
		memset(&uid_entry->io[UID_STATE_TOTAL_CURR], 0,
			sizeof(struct io_stats));
		set_io_uid_tasks_zero(uid_entry);
	}

	rcu_read_lock();
	do_each_thread(temp, task) {
		uid = from_kuid_munged(user_ns, task_uid(task));
		if (!uid_entry || uid_entry->uid != uid)
			uid_entry = find_or_register_uid(uid);
		if (!uid_entry)
			continue;
		add_uid_io_stats(uid_entry, task, UID_STATE_TOTAL_CURR);
	} while_each_thread(temp, task);
	rcu_read_unlock();

	hash_for_each(hash_table, bkt, uid_entry, hash) {
		compute_io_bucket_stats(&uid_entry->io[uid_entry->state],
					&uid_entry->io[UID_STATE_TOTAL_CURR],
					&uid_entry->io[UID_STATE_TOTAL_LAST],
					&uid_entry->io[UID_STATE_DEAD_TASKS]);
		compute_io_uid_tasks(uid_entry);
	}
}

static void update_io_stats_uid_locked(struct uid_entry *uid_entry)
{
	struct task_struct *task, *temp;
	struct user_namespace *user_ns = current_user_ns();

	memset(&uid_entry->io[UID_STATE_TOTAL_CURR], 0,
		sizeof(struct io_stats));
	set_io_uid_tasks_zero(uid_entry);

	rcu_read_lock();
	do_each_thread(temp, task) {
		if (from_kuid_munged(user_ns, task_uid(task)) != uid_entry->uid)
			continue;
		add_uid_io_stats(uid_entry, task, UID_STATE_TOTAL_CURR);
	} while_each_thread(temp, task);
	rcu_read_unlock();

	compute_io_bucket_stats(&uid_entry->io[uid_entry->state],
				&uid_entry->io[UID_STATE_TOTAL_CURR],
				&uid_entry->io[UID_STATE_TOTAL_LAST],
				&uid_entry->io[UID_STATE_DEAD_TASKS]);
	compute_io_uid_tasks(uid_entry);
}


static int uid_io_show(struct seq_file *m, void *v)
{
	struct uid_entry *uid_entry;
	unsigned long bkt;

	rt_mutex_lock(&uid_lock);

	update_io_stats_all_locked();

	hash_for_each(hash_table, bkt, uid_entry, hash) {
		seq_printf(m, "%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
				uid_entry->uid,
				uid_entry->io[UID_STATE_FOREGROUND].rchar,
				uid_entry->io[UID_STATE_FOREGROUND].wchar,
				uid_entry->io[UID_STATE_FOREGROUND].read_bytes,
				uid_entry->io[UID_STATE_FOREGROUND].write_bytes,
				uid_entry->io[UID_STATE_BACKGROUND].rchar,
				uid_entry->io[UID_STATE_BACKGROUND].wchar,
				uid_entry->io[UID_STATE_BACKGROUND].read_bytes,
				uid_entry->io[UID_STATE_BACKGROUND].write_bytes,
				uid_entry->io[UID_STATE_FOREGROUND].fsync,
				uid_entry->io[UID_STATE_BACKGROUND].fsync);

		show_io_uid_tasks(m, uid_entry);
	}

	rt_mutex_unlock(&uid_lock);
	return 0;
}

static int uid_io_open(struct inode *inode, struct file *file)
{
	return single_open(file, uid_io_show, PDE_DATA(inode));
}

static const struct file_operations uid_io_fops = {
	.open		= uid_io_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int uid_procstat_open(struct inode *inode, struct file *file)
{
	return single_open(file, NULL, NULL);
}

static ssize_t uid_procstat_write(struct file *file,
			const char __user *buffer, size_t count, loff_t *ppos)
{
	struct uid_entry *uid_entry;
	uid_t uid;
	int argc, state;
	char input[128];

	if (count >= sizeof(input))
		return -EINVAL;

	if (copy_from_user(input, buffer, count))
		return -EFAULT;

	input[count] = '\0';

	argc = sscanf(input, "%u %d", &uid, &state);
	if (argc != 2)
		return -EINVAL;

	if (state != UID_STATE_BACKGROUND && state != UID_STATE_FOREGROUND)
		return -EINVAL;

	rt_mutex_lock(&uid_lock);

	uid_entry = find_or_register_uid(uid);
	if (!uid_entry) {
		rt_mutex_unlock(&uid_lock);
		return -EINVAL;
	}

	if (uid_entry->state == state) {
		rt_mutex_unlock(&uid_lock);
		return count;
	}

	update_io_stats_uid_locked(uid_entry);

	uid_entry->state = state;

	rt_mutex_unlock(&uid_lock);

	return count;
}

static const struct file_operations uid_procstat_fops = {
	.open		= uid_procstat_open,
	.release	= single_release,
	.write		= uid_procstat_write,
};

#ifdef CONFIG_BLK_DEV_THROTTLING
enum pid_state_enum{
	PID_STATE_FG,
	PID_STATE_BG,
	PID_STATE_SBG,
	PID_STATE_KBG,
	PID_STATE_NUM,
};
#define UID_NUMBER 20000
#define MAX_PROCTITLE_LEN 64
#define PID_HASH_BITS 10
#define PID_DEFAULT_ENABLE_FLAG 0x10000000
DECLARE_HASHTABLE(pid_hash_table, PID_HASH_BITS);
static struct workqueue_struct* pid_iostats_update_wq;
static struct work_struct	pid_iostats_work;
static spinlock_t		pid_iostats_lock;
static atomic_t pid_iostats_work_is_ongoing;
static LIST_HEAD(pid_switch_event_list_head);
static struct task_struct *current_switch_task;
static unsigned int pid_iostats_enabled;
static unsigned int debug_app_uid = 0xFFFFFFFF;
static DEFINE_RT_MUTEX(pid_lock);
static atomic_t exit_notifier_count = ATOMIC_INIT(0);
static DECLARE_WAIT_QUEUE_HEAD(pid_io_show_wait);
struct pid_node {
	u64 pid;
	unsigned int state;
	struct io_stats io_curr;
	struct io_stats io_last;
	struct io_stats io_dead;
	struct list_head list;
};
struct pid_entry {
	u64 uid;
	u64 tgid;
	unsigned int pid_node_cnt;
	struct list_head pid_list;
	unsigned char task_name[MAX_PROCTITLE_LEN];
	struct io_stats io[PID_STATE_NUM];
	struct hlist_node hash;
};
struct pid_switch_event
{
	struct list_head entry;
	struct task_struct *task;
	unsigned int state;
};
enum {
	TASK_STATE_RUNNING = 0,
	TASK_STATE_SLEEPING,
	TASK_STATE_DISK_SLEEP,
	TASK_STATE_STOPPED,
	TASK_STATE_TRACING_STOP,
	TASK_STATE_DEAD,
	TASK_STATE_ZOMBIE,
};
static const int task_state_array[] = {
	TASK_STATE_RUNNING,
	TASK_STATE_SLEEPING,
	TASK_STATE_DISK_SLEEP,
	TASK_STATE_STOPPED,
	TASK_STATE_TRACING_STOP,
	TASK_STATE_DEAD,
	TASK_STATE_ZOMBIE,
};
static inline int pid_get_task_state(const struct task_struct *task)
{
	unsigned int state = (task->state | task->exit_state) & TASK_REPORT;
	BUILD_BUG_ON(1 + ilog2(TASK_REPORT)
			!= ARRAY_SIZE(task_state_array) - 1);

	return task_state_array[fls(state)];
}
static struct pid_node * add_pid_node(struct pid_entry *pid_entry, u64 the_pid)
{
	struct list_head *pre;
	struct pid_node *cur, *saved_cur;
	struct pid_node *the_node = NULL;
	if (unlikely(NULL == pid_entry)) {
		pr_err("%s: the pid_entry invalid\n", __func__);
		return NULL;
	}
	pre = &pid_entry->pid_list;
	list_for_each_entry_safe(cur, saved_cur, &pid_entry->pid_list, list) {
		if (the_pid > cur->pid)
			break;
		else if (the_pid == cur->pid)
			return cur;
		pre = &cur->list;
	}
	the_node = kzalloc(sizeof(struct pid_node), GFP_KERNEL);
	if (unlikely(NULL == the_node))
		return NULL;
	the_node->pid = the_pid;
	the_node->state = PID_STATE_FG;
	list_add(&the_node->list, pre);
	pid_entry->pid_node_cnt++;
	if(pid_entry->pid_node_cnt >= PID_NODE_CNT_WARNING_THRESHOLD && pid_entry->pid_node_cnt % 100 == 0)
		pr_err("%s: pid_debug WARNING: %s pid_node_list length reaches %d", __func__, pid_entry->task_name, pid_entry->pid_node_cnt);
	return the_node;
}
static struct pid_node * find_pid_node(struct pid_entry *pid_entry, u64 the_pid)
{
	struct pid_node *cur, *saved_cur;
	if (unlikely(NULL == pid_entry)) {
		pr_err("%s: the pid_entry invalid\n", __func__);
		return NULL;
	}

	list_for_each_entry_safe(cur, saved_cur, &pid_entry->pid_list, list) {
		if (the_pid == cur->pid)
			return cur;
	}
	return NULL;
}
static void free_pid_node(struct pid_entry *pid_entry, struct pid_node *cur)
{
	if (pid_entry == NULL || cur == NULL)
		return;
	list_del(&cur->list);
	pid_entry->pid_node_cnt--;
	kfree(cur);
}
static void del_pid_node(struct pid_entry *pid_entry, u64 pid)
{
	struct pid_node *cur = NULL, *saved_cur = NULL;
	if (unlikely(NULL == pid_entry)) {
		pr_err("%s: the pid_entry invalid\n", __func__);
		return;
	}
	list_for_each_entry_safe(cur, saved_cur, &pid_entry->pid_list, list) {
		if (cur && pid == cur->pid) {
			free_pid_node(pid_entry, cur);
			break;
		}
	}
}
static void add_pid_io_stats(struct pid_entry *pid_entry,
			struct task_struct *task,  struct io_stats *io_curr)
{
	if (unlikely(task == NULL || pid_entry == NULL || io_curr == NULL))
		return;
	io_curr->read_bytes += task->ioac.read_bytes;
	io_curr->write_bytes += compute_write_bytes(task);
	io_curr->rchar += task->ioac.file_rchar;
	io_curr->wchar += task->ioac.file_wchar;
	io_curr->fsync += task->ioac.syscfs;
}
static void pid_update_io_stats(struct pid_entry *pid_entry, struct pid_node *curr_pid_node, struct io_stats *io_curr)
{
	struct io_stats *io_bucket, *io_last, *io_dead;
	if (unlikely(pid_entry == NULL || io_curr == NULL || curr_pid_node==NULL
		|| curr_pid_node->state >= PID_STATE_NUM))
		return;
	io_bucket = &pid_entry->io[curr_pid_node->state];
	io_last = &curr_pid_node->io_last;
	io_dead = &curr_pid_node->io_dead;
	if (pid_entry->uid == debug_app_uid) {
		pr_err("pid_debug %s: %s,start pidnode_cnt=%d, tgid=%llu, state=%d  io_bucket: %llu %llu %llu %llu %llu\n", __func__, pid_entry->task_name, pid_entry->pid_node_cnt, curr_pid_node ->pid, curr_pid_node->state, io_bucket->read_bytes, io_bucket->write_bytes, io_bucket->rchar, io_bucket->wchar, io_bucket->fsync);
		pr_err("pid_debug %s: %s,start pidnode_cnt=%d, tgid=%llu, state=%d   io curr: %llu %llu %llu %llu %llu\n", __func__, pid_entry->task_name, pid_entry->pid_node_cnt, curr_pid_node ->pid,curr_pid_node->state,io_curr->read_bytes, io_curr->write_bytes, io_curr->rchar, io_curr->wchar, io_curr->fsync);
		pr_err("pid_debug %s: %s,start pidnode_cnt=%d, tgid=%llu, state=%d   io_last: %llu %llu %llu %llu %llu\n", __func__, pid_entry->task_name, pid_entry->pid_node_cnt, curr_pid_node ->pid,curr_pid_node->state,io_last->read_bytes, io_last->write_bytes, io_last->rchar, io_last->wchar, io_last->fsync);
		pr_err("pid_debug %s: %s,start pidnode_cnt=%d, tgid=%llu, state=%d  io io_dead: %llu %llu %llu %llu %llu\n", __func__, pid_entry->task_name, pid_entry->pid_node_cnt, curr_pid_node ->pid,curr_pid_node->state,io_dead->read_bytes, io_dead->write_bytes, io_dead->rchar, io_dead->wchar, io_dead->fsync);
	}
	if (io_dead->read_bytes +  io_curr->read_bytes < io_last->read_bytes)
		pr_err("pid_debug_read_bytes_error %s: %s, pidnode_cnt=%d, tgid=%llu, state=%d,  io_bucket=%llu, io_dead=%llu, io_curr=%llu, io_last=%llu\n",  __func__, pid_entry->task_name,
			pid_entry->pid_node_cnt, curr_pid_node ->pid, curr_pid_node->state,io_bucket->read_bytes, io_dead->read_bytes, io_curr->read_bytes, io_last->read_bytes);
	else
		io_bucket->read_bytes += io_dead->read_bytes +  io_curr->read_bytes - io_last->read_bytes;
	if (io_dead->write_bytes + io_curr->write_bytes < io_last->write_bytes)
		pr_err("pid_debug_write_bytes_error %s: %s, pidnode_cnt=%d, tgid=%llu, state=%d,  io_bucket=%llu, io_dead=%llu, io_curr=%llu, io_last=%llu\n",  __func__, pid_entry->task_name,
			pid_entry->pid_node_cnt, curr_pid_node ->pid, curr_pid_node->state,io_bucket->write_bytes, io_dead->write_bytes, io_curr->write_bytes, io_last->write_bytes);
	else
		io_bucket->write_bytes += io_dead->write_bytes + io_curr->write_bytes - io_last->write_bytes;
	if (io_dead->rchar + io_curr->rchar < io_last->rchar)
		pr_err("pid_debug_rchar_error %s: %s, pidnode_cnt=%d, tgid=%llu, state=%d,  io_bucket=%llu, io_dead=%llu, io_curr=%llu, io_last=%llu\n", 	__func__, pid_entry->task_name,
			pid_entry->pid_node_cnt, curr_pid_node ->pid, curr_pid_node->state,io_bucket->rchar, io_dead->rchar, io_curr->rchar, io_last->rchar);
	else
		io_bucket->rchar += io_dead->rchar + io_curr->rchar - io_last->rchar;
	if (io_dead->wchar + io_curr->wchar < io_last->wchar)
		pr_err("pid_debug_wchar_error %s: %s, pidnode_cnt=%d, tgid=%llu, state=%d,  io_bucket=%llu, io_dead=%llu, io_curr=%llu, io_last=%llu\n", 	__func__, pid_entry->task_name,
			pid_entry->pid_node_cnt, curr_pid_node ->pid, curr_pid_node->state,io_bucket->wchar, io_dead->wchar, io_curr->wchar, io_last->wchar);
	else
		io_bucket->wchar += io_dead->wchar + io_curr->wchar - io_last->wchar;
	if (io_dead->fsync + io_curr->fsync < io_last->fsync)
		pr_err("pid_debug_fsync_error %s: %s, pidnode_cnt=%d, tgid=%llu, state=%d,  io_bucket=%llu, io_dead=%llu, io_curr=%llu, io_last=%llu\n", 	__func__, pid_entry->task_name,
			pid_entry->pid_node_cnt, curr_pid_node ->pid, curr_pid_node->state,io_bucket->fsync, io_dead->fsync, io_curr->fsync, io_last->fsync);
	else
		io_bucket->fsync += io_dead->fsync + io_curr->fsync - io_last->fsync;
	io_last->read_bytes = io_curr->read_bytes;
	io_last->write_bytes = io_curr->write_bytes;
	io_last->rchar = io_curr->rchar;
	io_last->wchar = io_curr->wchar;
	io_last->fsync = io_curr->fsync;
	memset( io_dead, 0, sizeof(struct io_stats));
	if (pid_entry->uid == debug_app_uid) {
		pr_err("pid_debug %s: %s,end pidnode_cnt=%d, tgid=%llu, state=%d  io_bucket: %llu %llu %llu %llu %llu\n", __func__, pid_entry->task_name, pid_entry->pid_node_cnt, curr_pid_node ->pid, curr_pid_node->state, io_bucket->read_bytes, io_bucket->write_bytes, io_bucket->rchar, io_bucket->wchar, io_bucket->fsync);
		pr_err("pid_debug %s: %s,end pidnode_cnt=%d, tgid=%llu, state=%d   io curr: %llu %llu %llu %llu %llu\n", __func__, pid_entry->task_name, pid_entry->pid_node_cnt, curr_pid_node ->pid,curr_pid_node->state,io_curr->read_bytes, io_curr->write_bytes, io_curr->rchar, io_curr->wchar, io_curr->fsync);
		pr_err("pid_debug %s: %s,end pidnode_cnt=%d, tgid=%llu, state=%d   io_last: %llu %llu %llu %llu %llu\n", __func__, pid_entry->task_name, pid_entry->pid_node_cnt, curr_pid_node ->pid,curr_pid_node->state,io_last->read_bytes, io_last->write_bytes, io_last->rchar, io_last->wchar, io_last->fsync);
		pr_err("pid_debug %s: %s,end pidnode_cnt=%d, tgid=%llu, state=%d  io io_dead: %llu %llu %llu %llu %llu\n", __func__, pid_entry->task_name, pid_entry->pid_node_cnt, curr_pid_node ->pid,curr_pid_node->state,io_dead->read_bytes, io_dead->write_bytes, io_dead->rchar, io_dead->wchar, io_dead->fsync);
	}
}
static struct pid_entry * pid_entry_merge(struct pid_entry *entry1, struct pid_entry *entry2)
{
	int i;
	struct pid_node *cur = NULL, *saved_cur = NULL;

	if (entry1 == NULL || entry2 == NULL)
		return NULL;
	if (entry1->pid_node_cnt == 0)
		entry1->tgid = entry2->tgid;
	entry1->pid_node_cnt += entry2->pid_node_cnt;
	for (i = 0; i < PID_STATE_NUM; i++) {
		entry1->io[i].rchar += entry2->io[i].rchar;
		entry1->io[i].read_bytes += entry2->io[i].read_bytes;
		entry1->io[i].wchar += entry2->io[i].wchar;
		entry1->io[i].write_bytes+= entry2->io[i].write_bytes;
		entry1->io[i].fsync += entry2->io[i].fsync;
	}

	list_for_each_entry_safe(cur, saved_cur, &entry2->pid_list, list) {
		list_del(&cur->list);
		list_add(&cur->list, &entry1->pid_list);
	}
	if (entry1->uid == debug_app_uid) {
		pr_err("pid_debug  %s: merge entry1: %s, %d,%llu,%llu, entry2: %s, %d,%llu,%llu,print mereged pid list \n", __func__, entry1->task_name, entry1->pid_node_cnt, entry1->uid, entry1->tgid, entry2->task_name,entry2->pid_node_cnt,entry2->uid, entry2->tgid);
		list_for_each_entry_safe(cur, saved_cur, &entry1->pid_list, list) {
			pr_err("pid_debug  %s: merge entry1: %s, %d,%llu,%llu, curr_pid=%llu \n", __func__, entry1->task_name, entry1->pid_node_cnt, entry1->uid, entry1->tgid, cur->pid);
		}
	}
	return entry1;
}
static void pid_merge_same_name_entry(void)
{
	int bkt = 0;
	struct hlist_head tmp_head;
	struct pid_entry	*pos1, *tmp1;
	struct pid_entry	*pos2, *tmp2;
	for (bkt = 0, pos1 = NULL; pos1 == NULL && bkt < HASH_SIZE(pid_hash_table); bkt++) {
		INIT_HLIST_HEAD(&tmp_head) ;
		hlist_for_each_entry_safe(pos1, tmp1, &pid_hash_table[bkt], hash){
			hlist_del_init(&pos1->hash);
			hlist_for_each_entry_safe(pos2, tmp2, &tmp_head, hash) {
				if (!strncmp(pos1->task_name, pos2->task_name, MAX_PROCTITLE_LEN -1)) {
					break;
				}
			}
			if (pos2) {
				pid_entry_merge(pos2, pos1);
				kfree(pos1);
				pos1 = NULL;
			} else {
				hlist_add_head(&pos1->hash, &tmp_head);
			}
		}
		hlist_move_list(&tmp_head, &pid_hash_table[bkt]);
		pos1 = NULL;
	}
}
static struct pid_entry *find_pid_entry(u64 uid, u64 tgid,char* find_task_name)
{
	int count = 0;
	struct pid_entry *pid_entry = NULL;
	struct pid_node *curr_pid_node = NULL;
	if (find_task_name == NULL)
		return NULL;
	hash_for_each_possible(pid_hash_table, pid_entry, hash, uid) {
		count++;
		if (uid != pid_entry->uid)
			continue;
		if (!strncmp(pid_entry->task_name, find_task_name, MAX_PROCTITLE_LEN -1)) {
			return pid_entry;
		}
		if (!strncmp(pid_entry->task_name, "<pre-initialized>", strlen("<pre-initialized>"))
			||!strncmp(pid_entry->task_name, "/init", strlen("/init"))
			||strlen(find_task_name) == 0) {
			curr_pid_node = find_pid_node(pid_entry, tgid);
			if (curr_pid_node) {
				/*while task exit, find_task_name maybe free, no need update task name*/
				if (strlen(find_task_name) != 0)
					memcpy(&pid_entry->task_name, find_task_name,  MAX_PROCTITLE_LEN - 1);
			return pid_entry;
			}
		}
	}
	if (count >= PID_ENTRY_CNT_WARNING_THRESHOLD && count % 100 == 0)
		pr_err("%s: pid_debug WARNING: pid_hash_list count is %d", __func__, count);
	return NULL;
}
static struct pid_entry *find_or_register_pid(struct task_struct *task, struct pid_node **ppcurr_pid_node)
{
	struct pid_entry *pid_entry;
	struct pid_entry *new_pid_entry;
	int res = 0;
	u64 uid;
	if (unlikely (task == NULL || ppcurr_pid_node == NULL)){
		return NULL;
	}
	new_pid_entry = kzalloc(sizeof(struct pid_entry), GFP_KERNEL);
	if (unlikely(!new_pid_entry)) {
		pr_err("%s: kzalloc failed\n", __func__);
		return NULL;
	}
	res = get_cmdline(task, new_pid_entry->task_name, MAX_PROCTITLE_LEN - 1);
	if (res == 0) {
		kfree(new_pid_entry);
		return NULL;
	}
	uid =  from_kuid_munged(current_user_ns(), task_uid(task));
	pid_entry = find_pid_entry(uid, task->tgid, new_pid_entry->task_name);
	if (pid_entry) {
		kfree(new_pid_entry);
		pid_entry->uid = uid;
		pid_entry->tgid = task->tgid;
		*ppcurr_pid_node = add_pid_node(pid_entry, task->tgid);
		if (*ppcurr_pid_node == NULL)
			return NULL;
		return pid_entry;
	}

	new_pid_entry->uid =uid;
	new_pid_entry->tgid = task->tgid;
	INIT_LIST_HEAD(&new_pid_entry->pid_list);
	*ppcurr_pid_node = add_pid_node(new_pid_entry, task->tgid);
	if (unlikely(*ppcurr_pid_node == NULL)) {
		kfree(new_pid_entry);
		return NULL;
	}
	hash_add(pid_hash_table, &new_pid_entry->hash, uid);
	return new_pid_entry;
}
static void pid_update_all_io_stats(void)
{
	struct pid_entry *pid_entry = NULL;
	struct task_struct *task, *thread;
	unsigned long bkt, need_update = 0;
	struct hlist_node *tmp;
	struct io_stats io_curr = {0};
	struct pid_node *curr_node = NULL;
	struct pid_node  *saved_cur = NULL;
	hash_for_each_safe(pid_hash_table, bkt, tmp, pid_entry, hash) {
		if (pid_entry->uid == debug_app_uid)
			pr_err("pid_debug %s: %s, start pidnode_cnt=%d\n", __func__, pid_entry->task_name, pid_entry->pid_node_cnt);
		list_for_each_entry_safe(curr_node, saved_cur, &pid_entry->pid_list, list) {
			memset(&io_curr, 0,sizeof(struct io_stats));
			need_update = 0;
			if (pid_entry->uid == debug_app_uid)
				pr_err("pid_debug %s: %s, pidnode_cnt=%d, tgid=%llu\n", __func__, pid_entry->task_name, pid_entry->pid_node_cnt, curr_node ->pid);
			rcu_read_lock();
			for_each_process(task){
				if (task->tgid != curr_node->pid)
					continue;
				thread = task;

				// cppcheck-suppress *
				do {
					add_pid_io_stats(pid_entry, thread, &io_curr);
				} while_each_thread(task, thread);
				need_update = 1;
			}
			rcu_read_unlock();
			if (need_update) {
				pid_update_io_stats(pid_entry, curr_node, &io_curr);
			} else {
				pr_err("pid_debug %s: %s, pidnode_cnt=%d, not found tgid=%llu\n", __func__, pid_entry->task_name, pid_entry->pid_node_cnt, curr_node ->pid);
				free_pid_node(pid_entry, curr_node);
			}
		}
		if (pid_entry->uid == debug_app_uid)
			pr_err("pid_debug %s: %s, end pidnode_cnt=%d\n", __func__, pid_entry->task_name, pid_entry->pid_node_cnt);
	}
}
static ssize_t pid_io_write(struct file *file,
			const char __user *ubuf, size_t count, loff_t *ppos)
{
	char input[128]={0};
	int argc, len;
	unsigned int enabled;
	unsigned int debug_uid;
	len = count >= sizeof(input)?sizeof(input):count;
	if (copy_from_user(input, ubuf, len)){
		pr_err("pid_debug %s: copy_from_user failed\n ", __func__);
		return -EFAULT;
	}
	input[sizeof(input)-1] = '\0';
	argc = sscanf(input, "%u %u", &enabled, &debug_uid);
	if (argc != 2) {
		pr_err("pid_debug %s: argc=%d\n ", __func__, argc);
		return -EINVAL;
	}
	pid_iostats_enabled = enabled;
	debug_app_uid = debug_uid;
	pr_err("%s: set pid_iostats_enabled=%u, debug_app_uid=%u\n", __func__, pid_iostats_enabled, debug_app_uid);
	return count;
}
static int pid_io_show(struct seq_file *m, void *v)
{
	struct pid_entry *pid_entry;
	unsigned long bkt;
	struct hlist_node *tmp;
	if (pid_iostats_enabled == 0)
		return 0;
	while (atomic_read(&exit_notifier_count))
		wait_event_interruptible(pid_io_show_wait, 0 == atomic_read(&exit_notifier_count));
	rt_mutex_lock(&pid_lock);
	pid_merge_same_name_entry();
	pid_update_all_io_stats();
	hash_for_each_safe(pid_hash_table, bkt, tmp, pid_entry, hash) {
		if (pid_entry->uid == debug_app_uid) {
			pr_err("pid_debug %s: %s, pidnode_cnt=%d, tgid=%llu, uid=%llu,  %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n", __func__, pid_entry->task_name, pid_entry->pid_node_cnt,
				pid_entry->tgid,
				pid_entry->uid,
				pid_entry->io[PID_STATE_KBG].rchar,
				pid_entry->io[PID_STATE_KBG].wchar,
				pid_entry->io[PID_STATE_KBG].read_bytes,
				pid_entry->io[PID_STATE_KBG].write_bytes,
				pid_entry->io[PID_STATE_BG].rchar + pid_entry->io[PID_STATE_SBG].rchar,
				pid_entry->io[PID_STATE_BG].wchar + pid_entry->io[PID_STATE_SBG].wchar,
				pid_entry->io[PID_STATE_BG].read_bytes + pid_entry->io[PID_STATE_SBG].read_bytes,
				pid_entry->io[PID_STATE_BG].write_bytes + pid_entry->io[PID_STATE_SBG].write_bytes,
				pid_entry->io[PID_STATE_KBG].fsync,
				pid_entry->io[PID_STATE_BG].fsync + pid_entry->io[PID_STATE_SBG].fsync);
		}
		seq_printf(m, "%llu %llu %s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
			pid_entry->tgid,
			pid_entry->uid,
			pid_entry->task_name,
			pid_entry->io[PID_STATE_KBG].rchar,
			pid_entry->io[PID_STATE_KBG].wchar,
			pid_entry->io[PID_STATE_KBG].read_bytes,
			pid_entry->io[PID_STATE_KBG].write_bytes,
			pid_entry->io[PID_STATE_BG].rchar + pid_entry->io[PID_STATE_SBG].rchar,
			pid_entry->io[PID_STATE_BG].wchar + pid_entry->io[PID_STATE_SBG].wchar,
			pid_entry->io[PID_STATE_BG].read_bytes + pid_entry->io[PID_STATE_SBG].read_bytes,
			pid_entry->io[PID_STATE_BG].write_bytes + pid_entry->io[PID_STATE_SBG].write_bytes,
			pid_entry->io[PID_STATE_KBG].fsync,
			pid_entry->io[PID_STATE_BG].fsync + pid_entry->io[PID_STATE_SBG].fsync);
	}
	rt_mutex_unlock(&pid_lock);
	return 0;
}
static int pid_io_open(struct inode *inode, struct file *file)
{
	return single_open(file, pid_io_show, PDE_DATA(inode));
}
static const struct file_operations pid_io_fops = {
	.open		= pid_io_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.write		= pid_io_write,
	.release	= single_release,
};
/*lint -e429*/
static void pid_add_switch_event(struct task_struct *notify_task, unsigned int state)
{
	struct pid_switch_event *new_event;
	new_event = kzalloc(sizeof(struct pid_switch_event), GFP_ATOMIC);
	if (unlikely (new_event == NULL)) {
		pr_err("%s: alloc mem failed for  %d, %p\n", __func__, state, notify_task);
		return;
	}
	new_event->state = state;
	new_event->task= notify_task;
	/* prevent task strut go away*/
	get_task_struct(notify_task);
	spin_lock(&pid_iostats_lock);
	list_add_tail(&new_event->entry, &pid_switch_event_list_head);
	spin_unlock(&pid_iostats_lock);
}

static int logusertype = 0;
static int blkcg_attach_tgid_notifier(struct notifier_block *self,
			unsigned long val, void *task)
{

	struct task_struct *notify_task = task;
	int type = (int)val;
	int state = 0;
	if (pid_iostats_enabled == 0 && logusertype == 0) {
		int logusertype = get_logusertype_flag();
		if (logusertype == BETA_USER || logusertype == OVERSEA_USER) {
			pid_iostats_enabled = PID_DEFAULT_ENABLE_FLAG;
			pr_warn("%s: get_logusertype_flag: %d, pid_iostats_enabled=%d\n", __func__, logusertype, pid_iostats_enabled);
		}
	}
	if (pid_iostats_enabled == 0)
		return NOTIFY_OK;
	if (unlikely(type >= BLK_THROTL_TYPE_NR || notify_task ==NULL)) {
		pr_err("%s: received wrong type %d, %p\n", __func__, type, notify_task);
		return -EINVAL;
	}
	switch (type) {
		case BLK_THROTL_TA:
		case BLK_THROTL_FG:
			state = PID_STATE_FG;
			break;
		case BLK_THROTL_KBG:
			state = PID_STATE_KBG;
			break;
		case BLK_THROTL_SBG:
			state = PID_STATE_SBG;
			break;
		case BLK_THROTL_BG:
			state = PID_STATE_BG;
			break;
		default:
			return NOTIFY_OK;
	}
	pid_add_switch_event(notify_task, state);
	queue_work(pid_iostats_update_wq, &pid_iostats_work);
	return NOTIFY_OK;
}
static struct notifier_block blkcg_attach_tgid_notifier_block = {
	.notifier_call	= blkcg_attach_tgid_notifier,
};
static bool pid_group_leader_alive(const struct task_struct *task)
{
	struct task_struct *leader = NULL;

	if (task == NULL) {
		pr_warn("%s:Invalid parameter", __func__);
		return false;
	}

	if (task->flags & PF_KTHREAD)
		return true;

	leader = task->group_leader;
	if ((leader == NULL)
		|| (leader->flags & PF_EXITING)
		|| (leader->flags & PF_EXITPIDONE)
		|| (leader->flags & PF_SIGNALED)
		|| (pid_get_task_state(leader) >= TASK_STATE_DEAD)) {
		return false;
	}

	return true;
}
static void pid_update_io_stats_state(struct task_struct *notify_task, int state)
{
	struct pid_entry *pid_entry = NULL;
	u64 uid;
	struct task_struct *task;
	struct io_stats io_curr = {0};
	struct pid_node *curr_pid_node = NULL;
	if (unlikely(!notify_task) || !pid_group_leader_alive(notify_task))
		return;
	uid = from_kuid_munged(current_user_ns(), task_uid(notify_task));
	if (uid > UID_NUMBER)
		return;
	rt_mutex_lock(&pid_lock);
	pid_entry = find_or_register_pid(notify_task, &curr_pid_node);
	if (unlikely(!pid_entry || !curr_pid_node))
		goto __EXIT__;
	memset(&io_curr, 0, sizeof(struct io_stats));
	rcu_read_lock();
	task = notify_task;
	do {
		add_pid_io_stats(pid_entry, task, &io_curr);
	} while_each_thread(notify_task, task);
	rcu_read_unlock();
	pid_update_io_stats(pid_entry, curr_pid_node, &io_curr);
	if (uid == debug_app_uid) {
		pr_err("pid_debug %s: %s, old state %d switch to new state %d,\n", __func__, pid_entry->task_name,curr_pid_node->state, state);
	}
	curr_pid_node->state = state;
__EXIT__:
	rt_mutex_unlock(&pid_lock);
	return;
}
static void pid_iostats_update_work(struct work_struct *work)
{
	struct pid_switch_event *pidevent , *temp;

	if (atomic_read(&pid_iostats_work_is_ongoing)) {
		pr_warn("%s: pid_iostats_work_is_ongoing, no need resched\n", __func__);
		return;
	}
	atomic_set(&pid_iostats_work_is_ongoing, 1);
	spin_lock(&pid_iostats_lock);
		list_for_each_entry_safe(pidevent, temp, &pid_switch_event_list_head, entry) {
			list_del(&pidevent->entry);
			current_switch_task = pidevent->task;
			spin_unlock(&pid_iostats_lock);
			pid_update_io_stats_state(pidevent->task, pidevent->state);
			put_task_struct(pidevent->task);
			kfree(pidevent);
			spin_lock(&pid_iostats_lock);
			current_switch_task = NULL;
		}
	spin_unlock(&pid_iostats_lock);
	atomic_set(&pid_iostats_work_is_ongoing, 0);
}
static int pid_task_switch_event_is_done(struct task_struct *task)
{
	int ret = 0;
	struct pid_switch_event *pidevent , *temp;
	if (task == NULL)
		return ret;
	spin_lock(&pid_iostats_lock);
	if (task == current_switch_task){
		ret = 1;
		goto _EXIT_;
	}
	list_for_each_entry_safe(pidevent, temp, &pid_switch_event_list_head, entry) {
		if (pidevent->task == task) {
			ret = 2;
			goto _EXIT_;
		}
	}
_EXIT_:
	spin_unlock(&pid_iostats_lock);
	return ret;
}
static void  pid_proccess_exit_notifier(struct task_struct *task)
{
	u64 uid;
	struct io_stats io_curr ={0};
	struct task_struct *t = NULL;
	struct pid_node *curr_pid_node = NULL;
	struct pid_entry *pid_entry = NULL;
	int res = 0;
	char task_name[MAX_PROCTITLE_LEN] = {0};
	if (pid_iostats_enabled == 0)
		return;
	if (!task)
		return;
	uid = from_kuid_munged(current_user_ns(), task_uid(task));
	if (uid > UID_NUMBER)
		return;
	res = get_cmdline(task, task_name, MAX_PROCTITLE_LEN - 1);
	if (res == 0) {
		return;
	}
	if (thread_group_leader(task)) {
		res = pid_task_switch_event_is_done(task);
		if (res != 0)
			pr_warn("%s: task switch event is not handle before exit, task_name=%s, res = %d\n", __func__, task_name, res);
	}

	memset(&io_curr, 0, sizeof(struct io_stats));
	atomic_inc(&exit_notifier_count);
	rt_mutex_lock(&pid_lock);
	pid_entry = find_pid_entry(uid, task->tgid, task_name);
	if (unlikely(!pid_entry)) {
		goto _EXIT_;
	}
	curr_pid_node = find_pid_node(pid_entry, task->tgid);
	if (unlikely(!curr_pid_node)) {
		goto _EXIT_;
	}
	if (thread_group_leader(task)){
		rcu_read_lock();
		t = task;
		do{
			add_pid_io_stats(pid_entry, t, &io_curr);
		} while_each_thread(task, t);
		rcu_read_unlock();
		pid_update_io_stats(pid_entry, curr_pid_node, &io_curr);
		del_pid_node(pid_entry, task->tgid);
	} else {
		add_pid_io_stats(pid_entry, task, &io_curr);
		curr_pid_node->io_dead.read_bytes+= io_curr.read_bytes;
		curr_pid_node->io_dead.write_bytes+= io_curr.write_bytes;
		curr_pid_node->io_dead.rchar+= io_curr.rchar;
		curr_pid_node->io_dead.wchar+= io_curr.wchar;
		curr_pid_node->io_dead.fsync += io_curr.fsync;
	}
_EXIT_:
	rt_mutex_unlock(&pid_lock);
	atomic_dec(&exit_notifier_count);
	wake_up(&pid_io_show_wait);
}
#endif
static int pid_and_uid_iostats_process_notifier(struct notifier_block *self,
			unsigned long cmd, void *v)
{
	struct task_struct *task = v;
	struct uid_entry *uid_entry;
	cputime_t utime, stime;
	uid_t uid;

	if (!task)
		return NOTIFY_OK;

	rt_mutex_lock(&uid_lock);
	uid = from_kuid_munged(current_user_ns(), task_uid(task));
	uid_entry = find_or_register_uid(uid);
	if (!uid_entry) {
		pr_err("%s: failed to find uid %d\n", __func__, uid);
		goto exit;
	}

	task_cputime_adjusted(task, &utime, &stime);
	uid_entry->utime += utime;
	uid_entry->stime += stime;
#ifdef CONFIG_CPU_FREQ_POWER_STAT
	uid_entry->power += task->cpu_power;
	task->cpu_power = ULLONG_MAX;
#endif

	add_uid_io_stats(uid_entry, task, UID_STATE_DEAD_TASKS);

exit:
	rt_mutex_unlock(&uid_lock);
#ifdef CONFIG_BLK_DEV_THROTTLING
	pid_proccess_exit_notifier(task);
#endif
	return NOTIFY_OK;
}

static struct notifier_block process_notifier_block = {
	.notifier_call	= pid_and_uid_iostats_process_notifier,
};

static int __init proc_uid_sys_stats_init(void)
{
	hash_init(hash_table);
#ifdef CONFIG_BLK_DEV_THROTTLING
	hash_init(pid_hash_table);
	spin_lock_init(&pid_iostats_lock);
	pid_iostats_update_wq = create_singlethread_workqueue("pid_iostats_update_wq");
	INIT_WORK(&pid_iostats_work, pid_iostats_update_work);
#endif
	cpu_parent = proc_mkdir("uid_cputime", NULL);
	if (!cpu_parent) {
		pr_err("%s: failed to create uid_cputime proc entry\n",
			__func__);
		goto err;
	}

	proc_create_data("remove_uid_range", 0222, cpu_parent,
		&uid_remove_fops, NULL);
	proc_create_data("show_uid_stat", 0444, cpu_parent,
		&uid_cputime_fops, NULL);

	io_parent = proc_mkdir("uid_io", NULL);
	if (!io_parent) {
		pr_err("%s: failed to create uid_io proc entry\n",
			__func__);
		goto err;
	}

	proc_create_data("stats", 0444, io_parent,
		&uid_io_fops, NULL);
#ifdef CONFIG_BLK_DEV_THROTTLING
	proc_create_data("pid_stats", 0444, io_parent,
		&pid_io_fops, NULL);
#endif
	proc_parent = proc_mkdir("uid_procstat", NULL);
	if (!proc_parent) {
		pr_err("%s: failed to create uid_procstat proc entry\n",
			__func__);
		goto err;
	}

	proc_create_data("set", 0222, proc_parent,
		&uid_procstat_fops, NULL);

	profile_event_register(PROFILE_TASK_EXIT, &process_notifier_block);
#ifdef CONFIG_BLK_DEV_THROTTLING
	blkcg_attach_notify_register(BLKG_ATTACH_NOTIFY_BY_TGID, &blkcg_attach_tgid_notifier_block);
#endif
	return 0;

err:
	remove_proc_subtree("uid_cputime", NULL);
	remove_proc_subtree("uid_io", NULL);
	remove_proc_subtree("uid_procstat", NULL);
	return -ENOMEM;
}

early_initcall(proc_uid_sys_stats_init);
