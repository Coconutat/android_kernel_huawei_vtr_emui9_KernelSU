/*
 * ARM64 generic CPU idle driver.
 *
 * Copyright (C) 2014 ARM Ltd.
 * Author: Lorenzo Pieralisi <lorenzo.pieralisi@arm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "Idlesleep: " fmt

#include <linux/module.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/hisi/hisi_idle_sleep.h>
#include <linux/spinlock.h>
#include <soc_acpu_baseaddr_interface.h>
#include <soc_sctrl_interface.h>
#include <linux/of_platform.h>

static DEFINE_SPINLOCK(s_idle_sleep_lock);//lint !e64 !e570 !e708 !e785
/*sync with lpm3*/
u32 *idle_sleep_vote_addr;

/*
 * hisi_idle_sleep_vote - vote for idle sleep in lpm3
 *
 * modid: modid
 * val:  0, can enter idle sleep;1 for not
 *
 * Called from the device that need access peripheral zone.
 */
s32 hisi_idle_sleep_vote(u32 modid, u32 val)
{
	unsigned long flags;

	if (modid >= ID_MUX)
		return -EINVAL;
	if (!idle_sleep_vote_addr)
		return -EFAULT;

	spin_lock_irqsave(&s_idle_sleep_lock, flags);//lint !e550
	if (val)
		*idle_sleep_vote_addr |= (u32)BIT(modid);
	else
		*idle_sleep_vote_addr &=  ~ (u32)BIT(modid);
	spin_unlock_irqrestore(&s_idle_sleep_lock, flags);//lint !e550

	return 0;
}
EXPORT_SYMBOL(hisi_idle_sleep_vote);

u32 hisi_idle_sleep_getval(void)
{
	return *idle_sleep_vote_addr;
}

static int __init hisi_idle_sleep_init(void)
{
	struct device_node *np = NULL;
	uint32_t vote_addr;
	int ret;

	idle_sleep_vote_addr = 0;

	np = of_find_compatible_node(NULL, NULL, "hisi,idle-sleep");//lint !e838
	if (!np) {
		pr_err("maybe idle sleep vote not supported\n");
		return -ENODEV;
	}

	ret = of_property_read_u32_array(np, "vote-addr", &vote_addr, 1UL);
	if (ret) {
		pr_err("failed to find vote-addr node\n");
		return -EFAULT;
	}

	idle_sleep_vote_addr = (u32 *)ioremap((u64)vote_addr, 8UL);
	if (!idle_sleep_vote_addr) {
		pr_err("failed to remap vote_addr\n");
		return -EIO;
	}

	return 0;
}

subsys_initcall(hisi_idle_sleep_init);
