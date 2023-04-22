/*
 * Huawei Kernel Harden, ptrace log upload
 *
 * Copyright (c) 2016 Huawei.
 *
 * Authors:
 * yinyouzhan <yinyouzhan@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/capability.h>
#include <linux/export.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/ptrace.h>
#include <linux/security.h>
#include <linux/signal.h>
#include <linux/uio.h>
#include <linux/audit.h>
#include <linux/pid_namespace.h>
#include <chipset_common/security/hw_kernel_stp_interface.h>

void record_ptrace_info_before_return(long request, struct task_struct *child)
{
	struct task_struct *tracer;
	char tcomm_child[sizeof(child->comm) + 8] = {0}; /*8 is reserved for unknown string*/
	char tcomm_tracer[sizeof(child->comm) + 8] = {0};/*comm size is same within any task*/
	static unsigned int  g_ptrace_log_counter = 0;

	if (child == NULL)
		return;

	if (g_ptrace_log_counter >= 100) /* only 100 log upload since power on */
		return;
	g_ptrace_log_counter++;

	(void)get_task_comm(tcomm_child, child);
	rcu_read_lock();
	tracer = ptrace_parent(child);
	if (tracer) {
		(void)get_task_comm(tcomm_tracer, tracer);
	} else {
		(void)strncpy(tcomm_tracer, "unknown", sizeof("unknown"));
	}
	rcu_read_unlock();
	struct stp_item item;
	char add_info[sizeof(tcomm_child) + sizeof(tcomm_tracer) + 1] = {0};
	(void)memset(&item, 0, sizeof(item));
	item.id = item_info[PTRACE].id;
	item.status = STP_RISK;
	item.credible = STP_CREDIBLE;
	item.version = 0;
	(void)strncpy(item.name, item_info[PTRACE].name, STP_ITEM_NAME_LEN - 1);
	(void)snprintf(add_info, sizeof(add_info) -1, "%s%s",tcomm_child, tcomm_tracer);
	int ret = kernel_stp_upload(item, add_info);
	if (ret != 0) {
		pr_err("stp ptrace upload fail, child_cmdline=%s, tracer_cmdline=%s\n",tcomm_child,tcomm_tracer);
	}
	else {
		pr_err("stp ptrace upload succ, child_cmdline=%s, tracer_cmdline=%s\n",tcomm_child,tcomm_tracer);
	}

	return;
}