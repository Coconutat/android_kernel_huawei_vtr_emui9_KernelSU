/*
 * arch/arm/mach-hi6620/util.c
 *
 * balong platform misc utilities function
 *
 * Copyright (C) 2012 Hisilicon, Inc.
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
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/kmod.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <linux/hisi/util.h>
#include <linux/uaccess.h>
#include <linux/hisi/hisi_bootup_keypoint.h>
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_BOOTTIME_TAG

/* record kernle boot is completed */
#define COMPLETED_MASK 0xABCDEF00
static unsigned int bootanim_complete;
extern void hisi_dump_bootkmsg(void);

/*
 *check kernel boot is completed
 *Return: 1,  kernel boot is completed
 *	        0,  no completed
 */
int is_bootanim_completed(void)
{
	return (bootanim_complete == COMPLETED_MASK);
}

static ssize_t boot_time_proc_write(struct file *file, const char __user *buf,
				    size_t nr, loff_t *off)
{
	if (is_bootanim_completed())
		return nr;

	/*only need the print time */
	pr_err("bootanim has been complete, turn to Lancher!\n");
	/*set_boot_keypoint(STAGE_KERNEL_BOOTANIM_COMPLETE);*/

	bootanim_complete = COMPLETED_MASK;

	hisi_dump_bootkmsg();
	return nr;
}

static const struct file_operations boot_time_proc_fops = {
	.write = boot_time_proc_write,
};

static int __init boot_time_proc_init(void)
{
	balong_create_stats_proc_entry("boot_time", (S_IWUSR),
				       &boot_time_proc_fops, NULL);

	return 0;
}

module_init(boot_time_proc_init);
