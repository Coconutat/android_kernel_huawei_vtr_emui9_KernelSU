/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
#include <linux/timekeeper_internal.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/syscore_ops.h>
#include <linux/clocksource.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/tick.h>
#include <linux/compiler.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/rtc.h>

unsigned long hisi_pmu_rtc_readcount(void);

static struct rtc_device *rtc_timekeeper = NULL;

/**
 * read_persistent_clock -  Return time from the persistent clock.
 *
 * Weak dummy function for arches that do not yet support it.
 * Reads the time from the battery backed persistent clock.
 * Returns a timespec with tv_sec=0 and tv_nsec=0 if unsupported.
 *
 *  XXX - Do be sure to remove it once all arches implement it.
 */
void read_persistent_clock(struct timespec *ts)
{
	ts->tv_sec = 0;
	ts->tv_nsec = 0;

	if (rtc_timekeeper == NULL)
		return;

	ts->tv_sec = hisi_pmu_rtc_readcount();
	return;
}

static int __init hisi_timekeeper_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc = NULL;
	struct device_node *np = pdev->dev.of_node;
	const char *def_device_name = "rtc0";
	const char *device_name = NULL;

	if (of_property_read_string(np, "device_name", &device_name) < 0) {
		pr_info("hisi_timekeeper : no device_name, use default\n");
		device_name = def_device_name;
	}

	pr_info("hisi_timekeeper : use %s as timekeeper\n", device_name);

	rtc = rtc_class_open(device_name);
	if (!rtc) {
		printk("%s no RTC found !\n", __FUNCTION__);
		return -ENODEV;
	}

	rtc_timekeeper = rtc;

	return 0;
}

static const struct of_device_id hisi_timekeeper_of_match[] = {
	{.compatible = "hisilicon,hisi-timekeeper" },/*lint !e785*/
	{ }/*lint !e785*/
};

MODULE_DEVICE_TABLE(of, hisi_timekeeper_of_match);

static struct platform_driver hisi_timekeeper_driver = {
	.probe = hisi_timekeeper_probe,
	.driver = {
		.name  = "hisi-timekeeper",
		.of_match_table = of_match_ptr(hisi_timekeeper_of_match),
	},/*lint !e785*/
};/*lint !e785*/

static int __init hisi_timekeeper_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&hisi_timekeeper_driver);/*lint !e64 !e838*/
	if (ret)
		pr_err("register hisi timekeeper driver failed.\n");

	return ret;
}
/*lint -e528 -esym(528,__initcall_hisi_timekeeper_init6,__exitcall_hisi_timekeeper_exit)*/
late_initcall(hisi_timekeeper_init);
/*lint -e528 +esym(528,__initcall_hisi_timekeeper_init6,__exitcall_hisi_timekeeper_exit)*/

/*lint -e753 -esym(753,*)*/
MODULE_DESCRIPTION("Hisilicon Timekeeper Driver");
MODULE_LICENSE("GPL V2");
/*lint -e753 +esym(753,*)*/

