/*
 * Hisilicon Platforms CPUFDDR_FREQ_LINK support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/cpu.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/cpufreq.h>
#include <linux/of_platform.h>
#include <linux/pm_opp.h>
#include <linux/pm_qos.h>
#include <linux/cpumask.h>
#include <linux/hisi/hisi_mailbox.h>
#include <linux/hisi/hisi_rproc.h>
#include <linux/hisi/ipc_msg.h>
#include <linux/slab.h>


int set_cpu_ddr_link_governor(unsigned int ddr_lvl)
{
	unsigned int *msg;
	int rproc_id = HISI_RPROC_LPM3_MBX13;
	int ret;

	msg = kzalloc(sizeof(u32) * 8, GFP_KERNEL);
	if (!msg) {
		pr_err("%s: cannot allocate msg space.\n", __func__);
		goto msg_err;
	}
	msg[0] = (OBJ_AP << 24) | (OBJ_LITTLE_CLUSTER << 16) | (CMD_SETTING << 8) | TYPE_DNLIMIT;
	msg[1] = ddr_lvl;
	ret = RPROC_ASYNC_SEND((rproc_id_t)rproc_id, (mbox_msg_t *)msg, 8);
	if(ret) {
		pr_err(" %s , line %d, send error\n", __func__, __LINE__);
		goto msg_err;
	}
	kfree(msg);
	msg = NULL;
	return 0;
msg_err:
	if (msg != NULL) {
		kfree(msg);
		msg = NULL;
	}
	return -1;
}

static int cpuddr_freq_link_notify(struct notifier_block *nb,
		unsigned long val, void *v)
{
	int ret;
	ret = set_cpu_ddr_link_governor((unsigned int)val);
	if (ret)
		return NOTIFY_BAD;
	else
		return NOTIFY_OK;
}

static struct notifier_block cpuddr_freq_link_notifier = {
	.notifier_call = cpuddr_freq_link_notify,
};

void cpuddr_freq_link_notifier_init(struct notifier_block *nb)
{
	pm_qos_add_notifier(PM_QOS_ACPUDDR_LINK_GOVERNOR_LEVEL, nb);
}

static __init int hisi_cpuddr_freq_link_init(void)
{
	cpuddr_freq_link_notifier_init(&cpuddr_freq_link_notifier);

	return 0;
}
module_init(hisi_cpuddr_freq_link_init);

