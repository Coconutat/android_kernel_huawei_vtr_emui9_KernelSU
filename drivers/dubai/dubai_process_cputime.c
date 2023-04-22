/*
 * DUBAI process cputime.
 *
 * Copyright (C) 2017 Huawei Device Co.,Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/hashtable.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/net.h>
#include <linux/profile.h>
#include <linux/slab.h>

#include <chipset_common/dubai/dubai_common.h>

#define PROC_HASH_BITS			(10)
#define MAX_CMDLINE_LEN			(128)
#define BASE_COUNT				(500)
#define UID_SYSTEM				(1000)
#define KERNEL_TGID				(0)
#define KERNEL_NAME				"kernel"
#define SYSTEM_SERVER			"system_server"
#define CMP_TASK_COMM(p, q) 	strncmp(p, q, TASK_COMM_LEN - 1)
#define COPY_TASK_COMM(p, q)	strncpy(p, q, TASK_COMM_LEN - 1)/* unsafe_function_ignore: strncpy */
#define COPY_PROC_NAME(p, q)	strncpy(p, q, NAME_LEN - 1)/* unsafe_function_ignore: strncpy */
#define LOG_ENTRY_SIZE(count) \
	sizeof(struct dubai_cputime_transmit) \
		+ (long long)(count) * sizeof(struct dubai_cputime)

enum {
    CMDLINE_NEED_TO_GET = 0,
    CMDLINE_PROCESS,
    CMDLINE_THREAD,
};

enum {
	PROCESS_STATE_DEAD = 0,
	PROCESS_STATE_ACTIVE,
	PROCESS_STATE_INVALID,
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

enum {
	PROC_DECOMPOSE_MIN = 0,
	PROC_DECOMPOSE_KERNEL = PROC_DECOMPOSE_MIN,
	PROC_DECOMPOSE_SYSTEMSERVER,
	PROC_DECOMPOSE_MAX,
};

// keep sync with fs/proc/array.c
static const int task_state_array[] = {
	TASK_STATE_RUNNING,
	TASK_STATE_SLEEPING,
	TASK_STATE_DISK_SLEEP,
	TASK_STATE_STOPPED,
	TASK_STATE_TRACING_STOP,
	TASK_STATE_DEAD,
	TASK_STATE_ZOMBIE,
};

struct dubai_cputime {
	uid_t uid;
	pid_t pid;
	unsigned long long time;
	unsigned long long power;
	unsigned char cmdline;
	char name[NAME_LEN];
} __packed;

static struct dubai_thread_entry {
	pid_t pid;
	cputime_t utime;
	cputime_t stime;
#ifdef CONFIG_HUAWEI_DUBAI_TASK_CPU_POWER
	unsigned long long power;
#endif
	bool alive;
	char name[NAME_LEN];
	struct list_head node;
};

static struct dubai_proc_entry {
	pid_t tgid;
	uid_t uid;
	cputime_t utime;
	cputime_t stime;
	cputime_t active_utime;
	cputime_t active_stime;
#ifdef CONFIG_HUAWEI_DUBAI_TASK_CPU_POWER
	unsigned long long active_power;
	unsigned long long power;
#endif
	bool alive;
	bool cmdline;
	char name[NAME_LEN];
	struct list_head threads;
	struct hlist_node hash;
};

static struct dubai_cputime_transmit {
	long long timestamp;
	int type;
	int count;
	unsigned char value[0];
} __packed;

struct dubai_proc_decompose {
	pid_t tgid;
	char prefix[PREFIX_LEN];
};

static DECLARE_HASHTABLE(proc_hash_table, PROC_HASH_BITS);
static DEFINE_MUTEX(dubai_proc_lock);

static atomic_t proc_cputime_enable;
static atomic_t dead_count;
static atomic_t active_count;
static int decompose_count;
static struct dubai_proc_decompose proc_decompose[PROC_DECOMPOSE_MAX];
#ifdef CONFIG_HUAWEI_DUBAI_TASK_CPU_POWER
static const atomic_t task_power_enable = ATOMIC_INIT(1);
#else
static const atomic_t task_power_enable = ATOMIC_INIT(0);
#endif

static inline unsigned long long dubai_cputime_to_usecs(cputime_t time)
{
	return ((unsigned long long)
		jiffies_to_msecs(cputime_to_jiffies(time)) * USEC_PER_MSEC);
}

// keep sync with fs/proc/array.c
static inline int dubai_get_task_state(const struct task_struct *task)
{
	unsigned int state = (task->state | task->exit_state) & TASK_REPORT;

	/*
	 * Parked tasks do not run; they sit in __kthread_parkme().
	 * Without this check, we would report them as running, which is
	 * clearly wrong, so we report them as sleeping instead.
	 */
	if (task->state == TASK_PARKED)
		state = TASK_INTERRUPTIBLE;

	BUILD_BUG_ON(1 + ilog2(TASK_REPORT)
			!= ARRAY_SIZE(task_state_array) - 1);

	return task_state_array[fls(state)];
}

static bool dubai_task_alive(const struct task_struct *task)
{
#ifdef CONFIG_HUAWEI_DUBAI_TASK_CPU_POWER
	/*
	 * if this task is exiting, we have already accounted for the
	 * time and power.
	 */
	if (task->cpu_power == ULLONG_MAX)
		return false;
#else
	if ((task == NULL)
		|| (task->flags & PF_EXITING)
		|| (task->flags & PF_EXITPIDONE)
		|| (task->flags & PF_SIGNALED)
		|| (dubai_get_task_state(task) >= TASK_STATE_DEAD))
		return false;
#endif
	else
		return true;
}

static void dubai_copy_name(char *to, const char *from)
{
	char *p;
	int len = NAME_LEN - 1;

	if (strlen(from) <= len) {
		strncpy(to, from, len);/* unsafe_function_ignore: strncpy */
		return;
	}

	p = strrchr(from, '/');
	if (p != NULL && (*(p + 1) != '\0'))
		strncpy(to, p + 1, len);/* unsafe_function_ignore: strncpy */
	else
		strncpy(to, from, len);/* unsafe_function_ignore: strncpy */
}

static void dubai_add_proc_decompose(int id, pid_t tgid, const char *prefix)
{
	if (proc_decompose[id].tgid < 0) {
		proc_decompose[id].tgid = tgid;
		strncpy(proc_decompose[id].prefix, prefix, PREFIX_LEN - 1);/* unsafe_function_ignore: strncpy */
		decompose_count++;
		DUBAI_LOGI("add decompose proc[%d]: %d, %s, count: %d", id, tgid, prefix, decompose_count);
	}
}

static void dubai_remove_proc_decompose(pid_t tgid)
{
	int id;

	for (id = PROC_DECOMPOSE_MIN; id < PROC_DECOMPOSE_MAX; id++) {
		if (proc_decompose[id].tgid == tgid) {
			DUBAI_LOGI("remove decompose proc[%d]: %d, %s", id, tgid, proc_decompose[id].prefix);
			memset(&proc_decompose[id], 0, sizeof(struct dubai_proc_decompose));/* unsafe_function_ignore: memset */
			proc_decompose[id].tgid = -1;
			decompose_count--;
			return;
		}
	}
}

static int dubai_get_decompose_id(pid_t tgid)
{
	int id;

	for (id = PROC_DECOMPOSE_MIN; id < PROC_DECOMPOSE_MAX; id++) {
		if (proc_decompose[id].tgid == tgid) {
			return id;
		}
	}

	return -1;
}

static void dubai_check_proc_decompose(void)
{
	uid_t uid;
	int id;
	struct task_struct *task;

	if (likely(decompose_count == PROC_DECOMPOSE_MAX) || system_state > SYSTEM_RUNNING)
		return;

	decompose_count = 0;
	for (id = PROC_DECOMPOSE_MIN; id < PROC_DECOMPOSE_MAX; id++) {
		if (proc_decompose[id].tgid >= 0) {
			decompose_count++;
			continue;
		}

		switch (id) {
		case PROC_DECOMPOSE_KERNEL:
			dubai_add_proc_decompose(PROC_DECOMPOSE_KERNEL, KERNEL_TGID, KERNEL_NAME);
			break;
		case PROC_DECOMPOSE_SYSTEMSERVER:
			rcu_read_lock();
			for_each_process(task) {
				if (task->flags & PF_KTHREAD || !dubai_task_alive(task))
					continue;

				uid = from_kuid_munged(current_user_ns(), task_uid(task));
				if (uid == UID_SYSTEM && !CMP_TASK_COMM(task->comm, SYSTEM_SERVER)) {
					DUBAI_LOGI("Get system server tgid: %d", task->tgid);
					dubai_add_proc_decompose(PROC_DECOMPOSE_SYSTEMSERVER, task->tgid, SYSTEM_SERVER);
					break;
				}
			}
			rcu_read_unlock();
			break;
		default:
			DUBAI_LOGE("Unknown id");
			break;
		}
	}
}

/*
 * Create log entry to store process cputime structures
 */
static struct buffered_log_entry *dubai_create_log_entry(long long timestamp, int count, int type)
{
	long long size = 0;
	struct buffered_log_entry *entry = NULL;
	struct dubai_cputime_transmit *transmit = NULL;

	if ((count < 0) || (type >= PROCESS_STATE_INVALID)) {
		DUBAI_LOGE("Invalid parameter, count=%d, type=%d", count, type);
		return NULL;
	}

	/*
	 * allocate more space(BASE_COUNT)
	 * size = dubai_cputime_transmit size + dubai_cputime size * count
	 */
	count += BASE_COUNT;
	size = LOG_ENTRY_SIZE(count);
	entry = create_buffered_log_entry(size, BUFFERED_LOG_MAGIC_PROC_CPUTIME);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to create buffered log entry");
		return NULL;
	}

	transmit = (struct dubai_cputime_transmit *)entry->data;
	transmit->timestamp = timestamp;
	transmit->type = type;
	transmit->count = count;

	return entry;
}

static void dubai_proc_entry_copy(unsigned char *value, const struct dubai_proc_entry *entry)
{
	struct dubai_cputime stat;
	cputime_t total_time;

	total_time = entry->active_utime + entry->active_stime;
	total_time += entry->utime + entry->stime;

	memset(&stat, 0, sizeof(struct dubai_cputime));/* unsafe_function_ignore: memset */
	stat.uid = entry->uid;
	stat.pid = entry->tgid;
	stat.time = dubai_cputime_to_usecs(total_time);
#ifdef CONFIG_HUAWEI_DUBAI_TASK_CPU_POWER
	stat.power = entry->active_power + entry->power;
#endif
	stat.cmdline = entry->cmdline? CMDLINE_PROCESS : CMDLINE_NEED_TO_GET;
	dubai_copy_name(stat.name, entry->name);

	memcpy(value, &stat, sizeof(struct dubai_cputime));/* unsafe_function_ignore: memcpy */
}

static void dubai_thread_entry_copy(unsigned char *value, uid_t uid, const struct dubai_thread_entry *thread)
{
	struct dubai_cputime stat;

	memset(&stat, 0, sizeof(struct dubai_cputime));/* unsafe_function_ignore: memset */
	stat.uid = uid;
	stat.pid = thread->pid;
	stat.time = dubai_cputime_to_usecs(thread->utime + thread->stime);
#ifdef CONFIG_HUAWEI_DUBAI_TASK_CPU_POWER
	stat.power = thread->power;
#endif
	stat.cmdline = CMDLINE_THREAD;
	dubai_copy_name(stat.name, thread->name);

	memcpy(value, &stat, sizeof(struct dubai_cputime));/* unsafe_function_ignore: memcpy */
}

static struct dubai_thread_entry *dubai_find_or_register_thread(pid_t pid,
		const char *comm, struct dubai_proc_entry *entry)
{
	int id;
	struct dubai_thread_entry *thread;

	id = dubai_get_decompose_id(entry->tgid);
	if (id < 0 && list_empty(&entry->threads))
		return NULL;

	list_for_each_entry(thread, &entry->threads, node) {
		if (thread->pid == pid)
			return thread;
	}

	thread = kzalloc(sizeof(struct dubai_thread_entry), GFP_ATOMIC);
	if (thread == NULL) {
		DUBAI_LOGE("Failed to allocate memory");
		return NULL;
	}
	thread->pid = pid;
	thread->alive = true;
	if (id >= 0)
		snprintf(thread->name, NAME_LEN - 1, "%s_%s", proc_decompose[id].prefix, comm);/* unsafe_function_ignore: snprintf */
	else
		snprintf(thread->name, NAME_LEN - 1, "%s_%s", entry->name, comm);/* unsafe_function_ignore: snprintf */

	list_add_tail(&thread->node, &entry->threads);
	atomic_inc(&active_count);

	return thread;
}

static struct dubai_proc_entry *dubai_find_proc_entry(pid_t pid, pid_t tgid, const char *comm,
		struct dubai_thread_entry **thread)
{
	struct dubai_proc_entry *entry;

	hash_for_each_possible(proc_hash_table, entry, hash, tgid) {
		if (entry->tgid == tgid) {
			*thread = dubai_find_or_register_thread(pid, comm, entry);
			return entry;
		}
	}

	return NULL;
}

static struct dubai_proc_entry *dubai_find_or_register_proc(const struct task_struct *task,
		struct dubai_thread_entry **thread)
{
	pid_t tgid = task->tgid;
	pid_t pid = task->pid;
	char comm[TASK_COMM_LEN] = {0};
	struct dubai_proc_entry *entry = NULL;
	struct task_struct *leader = task->group_leader;

	/* combine kernel thread to same pid_entry which pid is 0 */
	if (task->flags & PF_KTHREAD) {
		tgid = KERNEL_TGID;
		COPY_TASK_COMM(comm, KERNEL_NAME);
	} else {
		if (leader != NULL)
			COPY_TASK_COMM(comm, leader->comm);
		else
			COPY_TASK_COMM(comm, task->comm);
	}

	entry = dubai_find_proc_entry(pid, tgid, task->comm, thread);
	if (entry != NULL) {
		if (likely(!entry->cmdline))
			COPY_PROC_NAME(entry->name, comm);

		return entry;
	}

	entry = kzalloc(sizeof(struct dubai_proc_entry), GFP_ATOMIC);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to allocate memory");
		return NULL;
	}
	entry->tgid = tgid;
	entry->uid = from_kuid_munged(current_user_ns(), task_uid(task));
	entry->alive = true;
	entry->cmdline = (tgid == KERNEL_TGID);
	COPY_PROC_NAME(entry->name, comm);
	INIT_LIST_HEAD(&entry->threads);
	*thread = dubai_find_or_register_thread(pid, task->comm, entry);

	hash_add(proc_hash_table, &entry->hash, tgid);
	atomic_inc(&active_count);

	return entry;
}

int dubai_update_proc_cputime(void)
{
	cputime_t utime;
	cputime_t stime;
	struct dubai_proc_entry *entry;
	struct dubai_thread_entry *thread;
	struct task_struct *task, *temp;

	rcu_read_lock();
	/* update active time from alive task */
	do_each_thread(temp, task) {
		/*
		 * check whether task is alive or not
		 * if not, do not record this task's cpu time
		 */
		if (!dubai_task_alive(task))
			continue;

		thread = NULL;
		entry = dubai_find_or_register_proc(task, &thread);
		if (entry == NULL) {
			rcu_read_unlock();
			DUBAI_LOGE("Failed to find the entry for process %d", task->tgid);
			return -ENOMEM;
		}
		task_cputime_adjusted(task, &utime, &stime);
		entry->active_utime += utime;
		entry->active_stime += stime;
#ifdef CONFIG_HUAWEI_DUBAI_TASK_CPU_POWER
		entry->active_power += task->cpu_power;
#endif
		if (thread != NULL) {
			thread->utime = utime;
			thread->stime = stime;
#ifdef CONFIG_HUAWEI_DUBAI_TASK_CPU_POWER
			thread->power = task->cpu_power;
#endif
		}
	} while_each_thread(temp, task);
	rcu_read_unlock();

	return 0;
}

/*
 * initialize hash list
 * report dead process and delete hash node
 */
static void dubai_clear_dead_entry(struct dubai_cputime_transmit *transmit)
{
	int max;
	bool decompose;
	unsigned char *value;
	unsigned long bkt;
	struct dubai_proc_entry *entry;
	struct hlist_node *tmp;
	struct dubai_thread_entry *thread, *temp;

	value = transmit->value;
	max = transmit->count;
	transmit->count = 0;

	hash_for_each_safe(proc_hash_table, bkt, tmp, entry, hash) {
		decompose = false;
		entry->active_stime = 0;
		entry->active_utime = 0;
#ifdef CONFIG_HUAWEI_DUBAI_TASK_CPU_POWER
		entry->active_power = 0;
#endif

		if (!list_empty(&entry->threads)) {
			decompose= true;
			list_for_each_entry_safe(thread, temp, &entry->threads, node) {
				if (thread->alive)
					continue;

				if (transmit->count < max
					&& (thread->utime + thread->stime) > 0) {
					dubai_thread_entry_copy(value, entry->uid, thread);
					value += sizeof(struct dubai_cputime);
					transmit->count++;
				}
				list_del_init(&thread->node);
				kfree(thread);
			}
		}

		if (entry->alive)
			continue;

		if (!decompose
			&& transmit->count < max
			&& (entry->utime + entry->stime) > 0) {
			dubai_proc_entry_copy(value, entry);
			value += sizeof(struct dubai_cputime);
			transmit->count++;
		}
		if (list_empty(&entry->threads)) {
			hash_del(&entry->hash);
			kfree(entry);
		}
	}
}

static int dubai_clear_and_update(long long timestamp)
{
	int ret = 0, count = 0;
	struct dubai_cputime_transmit *transmit;
	struct buffered_log_entry *entry;

	count = atomic_read(&dead_count);
	entry = dubai_create_log_entry(timestamp, count, PROCESS_STATE_DEAD);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to create log entry for dead processes");
		return -ENOMEM;
	}
	transmit = (struct dubai_cputime_transmit *)(entry->data);

	mutex_lock(&dubai_proc_lock);

	dubai_clear_dead_entry(transmit);
	atomic_set(&dead_count, 0);
	dubai_check_proc_decompose();

	ret = dubai_update_proc_cputime();
	if (ret < 0) {
		DUBAI_LOGE("Failed to update process cpu time");
		mutex_unlock(&dubai_proc_lock);
		goto exit;
	}
	mutex_unlock(&dubai_proc_lock);

	if (transmit->count > 0) {
		entry->length = LOG_ENTRY_SIZE(transmit->count);
		ret = send_buffered_log(entry);
		if (ret < 0)
			DUBAI_LOGE("Failed to send dead process log entry");
	}

exit:
	free_buffered_log_entry(entry);

	return ret;
}

static int dubai_send_active_process(long long timestamp)
{
	int ret = 0, max = 0;
	cputime_t active_time;
	unsigned char *value;
	unsigned long bkt;
	struct dubai_proc_entry *entry;
	struct dubai_cputime_transmit *transmit;
	struct buffered_log_entry *log_entry;
	struct dubai_thread_entry *thread;

	max = atomic_read(&active_count);
	log_entry = dubai_create_log_entry(timestamp, max, PROCESS_STATE_ACTIVE);
	if (log_entry == NULL) {
		DUBAI_LOGE("Failed to create log entry for active processes");
		return -ENOMEM;
	}
	transmit = (struct dubai_cputime_transmit *)(log_entry->data);
	value = transmit->value;
	max = transmit->count;
	transmit->count = 0;

	mutex_lock(&dubai_proc_lock);
	hash_for_each(proc_hash_table, bkt, entry, hash) {
		active_time = entry->active_utime + entry->active_stime;
		if (active_time == 0 || !entry->alive)
			continue;

		if (transmit->count >= max)
			break;

		if (list_empty(&entry->threads)) {
			dubai_proc_entry_copy(value, entry);
			value += sizeof(struct dubai_cputime);
			transmit->count++;
			continue;
		}

		list_for_each_entry(thread, &entry->threads, node) {
			if ((thread->utime + thread->stime) == 0 || !thread->alive)
				continue;

			if (transmit->count >= max)
				break;

			dubai_thread_entry_copy(value, entry->uid, thread);
			value += sizeof(struct dubai_cputime);
			transmit->count++;
		}
	}
	mutex_unlock(&dubai_proc_lock);

	if (transmit->count > 0) {
		log_entry->length = LOG_ENTRY_SIZE(transmit->count);
		ret = send_buffered_log(log_entry);
		if (ret < 0)
			DUBAI_LOGE("Failed to send active process log entry");
	}
	free_buffered_log_entry(log_entry);

	return ret;
}

/*
 * if there are dead processes in the list,
 * we should clear these dead processes
 * in case of pid reused
 */
int dubai_get_proc_cputime(long long timestamp)
{
	int ret = 0;

	if (!atomic_read(&proc_cputime_enable))
		return -EPERM;

	ret = dubai_clear_and_update(timestamp);
	if (ret < 0) {
		DUBAI_LOGE("Failed to clear dead process and update list");
		goto exit;
	}

	ret = dubai_send_active_process(timestamp);
	if (ret < 0) {
		DUBAI_LOGE("Failed to send active process cpu time");
	}

exit:
	return ret;
}

int dubai_get_proc_name(struct process_name *process)
{
	struct task_struct *task, *leader;
	char cmdline[MAX_CMDLINE_LEN] = {0};
	int ret = 0, compare = 0;

	if (process == NULL || process->pid <= 0) {
		DUBAI_LOGE("Invalid parameter");
		return -EINVAL;
	}

	rcu_read_lock();
	task = find_task_by_vpid(process->pid);
	if (task)
		get_task_struct(task);
	rcu_read_unlock();
	if (!task)
		return -EFAULT;

	leader = task->group_leader;
	compare = CMP_TASK_COMM(process->comm, task->comm);
	if (compare != 0 && leader != NULL)
		compare = CMP_TASK_COMM(process->comm, leader->comm);

	ret = get_cmdline(task, cmdline, MAX_CMDLINE_LEN - 1);
	cmdline[MAX_CMDLINE_LEN - 1] = '\0';
	if (ret > 0 && (compare == 0 || strstr(cmdline, process->comm) != NULL))
		dubai_copy_name(process->name, cmdline);

	put_task_struct(task);

	return ret;
}

static int dubai_process_notifier(struct notifier_block *self, unsigned long cmd, void *v)
{
	int ret;
	uid_t uid;
	bool got_cmdline = false;
	cputime_t utime, stime;
	struct task_struct *task = v;
	pid_t tgid, pid;
	struct task_struct *leader = NULL;
	struct dubai_proc_entry *entry = NULL;
	struct dubai_thread_entry *thread = NULL;
	char cmdline[MAX_CMDLINE_LEN] = {0};

	if (task == NULL || !atomic_read(&proc_cputime_enable))
		return NOTIFY_OK;

	tgid = task->tgid;
	pid = task->pid;
	leader = task->group_leader;

	/* kernel thread, we assume tgid is 0 */
	if (task->flags & PF_KTHREAD)
		tgid = KERNEL_TGID;

	if (tgid == task->pid) {
		ret = get_cmdline(task, cmdline, MAX_CMDLINE_LEN - 1);
		cmdline[MAX_CMDLINE_LEN - 1] = '\0';
		if (ret > 0 && strlen(cmdline) > 0)
			got_cmdline = true;
	}

	mutex_lock(&dubai_proc_lock);

	dubai_check_proc_decompose();

	if (task->tgid == task->pid || dubai_task_alive(leader)) {
		entry = dubai_find_or_register_proc(task, &thread);
	} else {
		entry = dubai_find_proc_entry(pid, tgid, task->comm, &thread);
	}

	if (entry == NULL)
		goto exit;

	if (got_cmdline) {
		dubai_copy_name(entry->name, cmdline);
		entry->cmdline = true;
	}

	if (!entry->alive) {
		uid = from_kuid_munged(current_user_ns(), task_uid(task));
		if (entry->tgid == task->pid || uid != entry->uid)
			goto exit;
		if (dubai_task_alive(leader)
			&& (strstr(entry->name, task->comm) == NULL)
			&& (strstr(entry->name, leader->comm) == NULL))
			goto exit;
	}

	task_cputime_adjusted(task, &utime, &stime);
	entry->utime += utime;
	entry->stime += stime;
#ifdef CONFIG_HUAWEI_DUBAI_TASK_CPU_POWER
	entry->power += task->cpu_power;
#endif
	if (thread != NULL) {
		thread->utime = utime;
		thread->stime = stime;
#ifdef CONFIG_HUAWEI_DUBAI_TASK_CPU_POWER
		thread->power = task->cpu_power;
#endif
	}

	/* process has died */
	if (entry->tgid == task->pid && entry->alive) {
		entry->alive = false;
		atomic_dec(&active_count);
		atomic_inc(&dead_count);
		dubai_remove_proc_decompose(entry->tgid);
	}

exit:
	if (thread != NULL && thread->alive) {
		thread->alive = false;
		atomic_dec(&active_count);
		atomic_inc(&dead_count);
	}

	mutex_unlock(&dubai_proc_lock);

	return NOTIFY_OK;
}

static struct notifier_block process_notifier_block = {
	.notifier_call	= dubai_process_notifier,
	.priority = INT_MAX,
};

static void dubai_proc_cputime_reset(void)
{
	int id;
	unsigned long bkt;
	struct dubai_proc_entry *entry;
	struct hlist_node *tmp;
	struct dubai_thread_entry *thread, *temp;

	mutex_lock(&dubai_proc_lock);

	hash_for_each_safe(proc_hash_table, bkt, tmp, entry, hash) {
		list_for_each_entry_safe(thread, temp, &entry->threads, node) {
			list_del_init(&thread->node);
			kfree(thread);
		}
		hash_del(&entry->hash);
		kfree(entry);
	}

	for (id = PROC_DECOMPOSE_MIN; id < PROC_DECOMPOSE_MAX; id++) {
		memset(&proc_decompose[id], 0, sizeof(struct dubai_proc_decompose));/* unsafe_function_ignore: memset */
		proc_decompose[id].tgid = -1;
	}
	decompose_count = 0;
	atomic_set(&dead_count, 0);
	atomic_set(&active_count, 0);
	atomic_set(&proc_cputime_enable, 0);
	hash_init(proc_hash_table);

	mutex_unlock(&dubai_proc_lock);
}

void dubai_proc_cputime_enable(bool enable)
{
	dubai_proc_cputime_reset();

	if (enable)
		atomic_set(&proc_cputime_enable, 1);
}

bool dubai_get_task_cpupower_enable(void) {
	return !!atomic_read(&task_power_enable);
}

void dubai_proc_cputime_init(void)
{
	dubai_proc_cputime_reset();
	profile_event_register(PROFILE_TASK_EXIT, &process_notifier_block);
}

void dubai_proc_cputime_exit(void)
{
	profile_event_unregister(PROFILE_TASK_EXIT, &process_notifier_block);
	dubai_proc_cputime_reset();
}
