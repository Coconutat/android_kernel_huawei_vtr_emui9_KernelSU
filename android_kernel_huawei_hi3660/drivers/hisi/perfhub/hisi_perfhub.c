/* Copyright (C) 2016-2017. Hisilicon Tech. Co., Ltd. All Rights Reserved.
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/threads.h>
#include <linux/cpu.h>
#include <linux/security.h>
#include <linux/cpuset.h>
#include <libhwsecurec/securec.h>
#include "../../../kernel/sched/sched.h"
#include <trace/events/sched.h>

#define CPU_TOTAL        8

static unsigned int g_last_mask;
static int g_last_pid;

/**
 * find_process_by_pid - find a process with a matching PID value.
 * @pid: the pid in question.
 *
 * The task of @pid, if found. %NULL otherwise.
 */
static struct task_struct *find_process_by_pid(pid_t pid)
{
	return pid ? find_task_by_vpid(pid) : current;
}


static int bind_cpu_cluster(unsigned int setmask, pid_t pid)
{
	cpumask_var_t cpus_allowed, new_mask;
	struct task_struct *p;
	int retval;
	struct cpumask mask;
	int i;

	cpumask_clear(&mask);

	for(i = 0; i < CPU_TOTAL; i++)
	{
		if(setmask & (0x01 << i))
			cpumask_set_cpu(i, &mask);
	}


	get_online_cpus();
	rcu_read_lock();

	p = find_process_by_pid(pid);
	if (!p) {
		rcu_read_unlock();
		put_online_cpus();
		return -ESRCH;
	}

	/* Prevent p going away */
	get_task_struct(p);
	rcu_read_unlock();

	if (p->flags & PF_NO_SETAFFINITY) {
		retval = -EINVAL;
		goto out_put_task;
	}
	if (!alloc_cpumask_var(&cpus_allowed, GFP_KERNEL)) {
		retval = -ENOMEM;
		goto out_put_task;
	}
	if (!alloc_cpumask_var(&new_mask, GFP_KERNEL)) {
		retval = -ENOMEM;
		goto out_free_cpus_allowed;
	}

	retval = security_task_setscheduler(p);
	if (retval)
		goto out_unlock;

	cpuset_cpus_allowed(p, cpus_allowed);
	cpumask_and(new_mask, &mask, cpus_allowed);

	/*
	 * Since bandwidth control happens on root_domain basis,
	 * if admission test is enabled, we only admit -deadline
	 * tasks allowed to run on all the CPUs in the task's
	 * root_domain.
	 */
	if (task_has_dl_policy(p) && dl_bandwidth_enabled()) {
		rcu_read_lock();
		if (!cpumask_subset(task_rq(p)->rd->span, new_mask)) {
			retval = -EBUSY;
			rcu_read_unlock();
			goto out_unlock;
		}
		rcu_read_unlock();
	}

again:
	retval = set_cpus_allowed_ptr(p, new_mask);

	if (!retval) {
		cpuset_cpus_allowed(p, cpus_allowed);
		if (!cpumask_subset(new_mask, cpus_allowed)) {
			/*
			 * We must have raced with a concurrent cpuset
			 * update. Just reset the cpus_allowed to the
			 * cpuset's cpus_allowed
			 */
			cpumask_copy(new_mask, cpus_allowed);
			goto again;
		}

		trace_sched_set_affinity(p, new_mask);
	}
out_unlock:
	free_cpumask_var(new_mask);
out_free_cpus_allowed:
	free_cpumask_var(cpus_allowed);
out_put_task:
	put_task_struct(p);
	put_online_cpus();

	return retval;
}

static ssize_t perfhub_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	if (buf == NULL)
	{
		pr_err("buf is NULL.");
		return -EINVAL;
	}

	return snprintf(buf, PAGE_SIZE, "%x|%d\n", g_last_mask, g_last_pid);// unsafe_function_ignore: snprintf
}

static ssize_t perfhub_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	long cpumask;
	long pid;
	int ret;
	char maskStr[64] = {0};
	const char *pstrmask = NULL;
	const char *pstrchr = NULL;
	char *next_p = NULL;
	const char *delimiter = "|";

	if (buf == NULL)
	{
		pr_err("buf is NULL.");
		return -EINVAL;
	}

	strcpy_s(maskStr, sizeof(maskStr)-1, buf);

	pstrmask = strtok_s(maskStr, delimiter, &next_p);
	if(pstrmask)
		pstrchr = strtok_s(NULL, delimiter, &next_p);
	else
		return -EINVAL;

	if (NULL == pstrchr)
		return -EINVAL;

	ret = strict_strtol(pstrmask, 16, &cpumask);
	if (ret != 0)
		return -EINVAL;

	ret = strict_strtol(pstrchr, 10, &pid);
	if (ret != 0)
		return -EINVAL;

	g_last_mask = cpumask;
	g_last_pid = pid;

	if (cpumask < 0 || cpumask > ((1 << CPU_TOTAL) - 1) )
	{
		pr_err("invalid cpumask");
		return -EINVAL;
	}

	ret = bind_cpu_cluster((unsigned int)cpumask, pid);
	if (0 != ret)
		return -EINVAL;

	return count;
}

struct kobj_attribute perfhub_attribute = __ATTR(cpuaffinity, 0660, perfhub_show, perfhub_store);

struct kobject *perfhub_kobj = NULL;

static int __init perfhub_init(void)
{
	int retval = 0;

	perfhub_kobj = kobject_create_and_add("perfhub", kernel_kobj);
	if (!perfhub_kobj)
		return -ENOMEM;

	retval = sysfs_create_file(perfhub_kobj, &perfhub_attribute.attr);
	if (retval) {
		kobject_put(perfhub_kobj);
		perfhub_kobj = NULL;
	}

	return retval;
}

static void __exit perfhub_exit(void)
{
	if (perfhub_kobj) {
		sysfs_remove_file(perfhub_kobj, &perfhub_attribute.attr);
		kobject_put(perfhub_kobj);
		perfhub_kobj = NULL;
	}
}

module_init(perfhub_init);
module_exit(perfhub_exit);

MODULE_DESCRIPTION("Hisilicon hisi perf hub");
MODULE_LICENSE("GPL v2");

