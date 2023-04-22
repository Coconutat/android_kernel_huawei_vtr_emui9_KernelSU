/*
 * Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "devbw: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/devfreq.h>
#include <linux/of.h>
#include <trace/events/power.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/cpumask.h>
#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/cpufreq.h>
#include "governor_memlat.h"

struct vote_reg {
        void __iomem    *reg;           /* dev frequency vote register */
        u8              shift;
        u8              width;
        u32             mbits;          /* mask bits */
};

struct dev_data {
	unsigned long cur_dev_freq;
	struct vote_reg hw_vote;
	cpumask_t cpus;
	spinlock_t vote_spinlock;
	struct devfreq *df;
	struct devfreq_dev_profile dp;
};

#define WIDTH_TO_MASK(width)    ((1UL << (width)) - 1)

static void find_freq(struct devfreq_dev_profile *p, unsigned long *freq,
			u32 flags);

void set_hw_vote_reg(struct vote_reg *hw_vote, unsigned long value)
{
	u32 data = 0;

	value >>= 4;
	if (value > WIDTH_TO_MASK(hw_vote->width))
		value = WIDTH_TO_MASK(hw_vote->width);

	data |= value << hw_vote->shift;
	data |= hw_vote->mbits;
	writel(data, hw_vote->reg);
}

void set_dev_freq(struct dev_data *d, unsigned long new_freq)
{
	if (new_freq == d->cur_dev_freq)
		return ;

	set_hw_vote_reg(&d->hw_vote, new_freq);

	d->cur_dev_freq = new_freq;
}

void set_dev_votefreq(struct device *dev, unsigned long new_freq)
{
	struct dev_data *d = dev_get_drvdata(dev);
	unsigned long freq = new_freq;

	spin_lock(&d->vote_spinlock);
	find_freq(&d->dp, &freq, 0);

	if (freq == d->cur_dev_freq) {
		spin_unlock(&d->vote_spinlock);
		return ;
	}

	set_hw_vote_reg(&d->hw_vote, freq);

	d->cur_dev_freq = freq;
	spin_unlock(&d->vote_spinlock);
}
EXPORT_SYMBOL(set_dev_votefreq);

unsigned long get_dev_votefreq(struct device *dev)
{
	struct dev_data *d = dev_get_drvdata(dev);
	unsigned int freq;

	spin_lock(&d->vote_spinlock);
	freq = d->cur_dev_freq;
	spin_unlock(&d->vote_spinlock);

	return freq;
}
EXPORT_SYMBOL(get_dev_votefreq);

static void find_freq(struct devfreq_dev_profile *p, unsigned long *freq,
			u32 flags)
{
	int i;
	unsigned long atmost, atleast, f;

	atmost = p->freq_table[0];
	atleast = p->freq_table[p->max_state-1];
	for (i = 0; i < p->max_state; i++) { /*lint !e574*/
		f = p->freq_table[i];
		if (f <= *freq)
			atmost = max(f, atmost);
		if (f >= *freq)
			atleast = min(f, atleast);
	}

	if (flags & DEVFREQ_FLAG_LEAST_UPPER_BOUND)
		*freq = atmost;
	else
		*freq = atleast;
}

static int devbw_target(struct device *dev, unsigned long *freq, u32 flags)
{
	struct dev_data *d = dev_get_drvdata(dev);

	find_freq(&d->dp, freq, flags);

	spin_lock(&d->vote_spinlock);
	set_dev_freq(d, *freq);
	spin_unlock(&d->vote_spinlock);
	trace_memlat_set_dev_freq(dev_name(dev), "periodic_update",	\
			cpumask_any(&d->cpus), 0, 0, *freq);

	return 0;
}

static int devbw_get_dev_status(struct device *dev,
				struct devfreq_dev_status *stat)
{
	return 0;
}

static int get_mask_from_dev_handle(struct platform_device *pdev,
					cpumask_t *mask)
{
	struct device *dev = &pdev->dev;
	struct device_node *dev_phandle;
	struct device *cpu_dev;
	int cpu, i = 0;
	int ret = -ENOENT;

	dev_phandle = of_parse_phandle(dev->of_node, "hisi,cpulist", i++);
	while (dev_phandle) {
		for_each_possible_cpu(cpu) {
			cpu_dev = get_cpu_device(cpu);
			if (cpu_dev && cpu_dev->of_node == dev_phandle) {
				cpumask_set_cpu(cpu, mask);
				ret = 0;
				break;
			}
		}
		dev_phandle = of_parse_phandle(dev->of_node,
						"hisi,cpulist", i++);
	}

	return ret;
}

#define PROP_TBL "hisi,freq-tbl"

/*lint -e429*/
int devfreq_add_devbw(struct platform_device *pdev)
{
	struct device *dev=&pdev->dev;
	struct dev_data *d;
	struct devfreq_dev_profile *p;
	u32 *data;
	const char *gov_name;
	int ret, len, i;
	u32 temp[2] = {0};
	void __iomem *reg_base;
	struct vote_reg *hw_vote;
	const char *name_str = NULL;

	reg_base = of_iomap(dev->of_node, 0);
	if (!reg_base) {
		dev_err(dev, "fail to map io!\n");
		ret = -ENOMEM;
		goto error;
	}

	d = devm_kzalloc(dev, sizeof(*d), GFP_KERNEL);
	if (!d) {
		dev_err(dev, "fail to alloc mem!\n");
		ret = -ENOMEM;
		goto error;
	}
	dev_set_drvdata(dev, d);

	if (get_mask_from_dev_handle(pdev, &d->cpus)) {
		dev_err(dev, "CPU list is empty\n");
		ret = -ENODEV;
		goto error;
	}

	name_str = of_device_get_match_data(dev);
	if (name_str) {
		dev_set_name(dev, "%s%d", name_str, cpumask_first(&d->cpus));
	} else {
		dev_err(dev, "name_str cannot found!\n");
	}

	spin_lock_init(&d->vote_spinlock);

	hw_vote = &d->hw_vote;
	if (of_property_read_u32_array(dev->of_node, "hisi,vote-reg",
								   &temp[0], 2)) {
			dev_err(dev, "[%s] node %s doesn't have hisi,vote-reg property!\n",
					__func__, dev->of_node->name);
		ret = -EINVAL;
		goto error;
	}

	hw_vote->reg = reg_base + temp[0];
	hw_vote->shift = ffs(temp[1]) - 1;
	hw_vote->width = fls(temp[1]) - ffs(temp[1]) + 1;
	hw_vote->mbits = temp[1] << 16;

	p = &d->dp;
	p->polling_ms = 50;
	p->target = devbw_target;
	p->get_dev_status = devbw_get_dev_status;

	if (of_find_property(dev->of_node, PROP_TBL, &len)) {
		len /= sizeof(*data); /*lint !e573*/
		data = devm_kzalloc(dev, len * sizeof(*data), GFP_KERNEL);
		if (!data) {
			dev_err(dev, "fail to alloc mem for freq_table\n");
			ret = -ENOMEM;
			goto error;
		}

		p->freq_table = devm_kzalloc(dev,
					     len * sizeof(*p->freq_table),
					     GFP_KERNEL);
		if (!p->freq_table) {
			dev_err(dev, "fail to alloc mem for freq_table\n");
			ret = -ENOMEM;
			goto error;
		}

		ret = of_property_read_u32_array(dev->of_node, PROP_TBL,
						 data, len);
		if (ret) {
			dev_err(dev, "fail to alloc mem for freq_table\n");
			goto error;
		}

		for (i = 0; i < len; i++)
			p->freq_table[i] = data[i];
		p->max_state = len;
	}

	if (of_property_read_string(dev->of_node, "governor", &gov_name))
		gov_name = "mem_latency";

	d->df = devfreq_add_device(dev, p, gov_name, NULL);
	if (IS_ERR(d->df)) {
		dev_err(dev, "fail to register devfreq memlatency\n");
		ret = PTR_ERR(d->df);
		goto error;
	}

	return 0; /*lint !e593*/
error:
	iounmap(reg_base);
	return ret; /*lint !e593*/
}
/*lint +e429*/

int devfreq_remove_devbw(struct device *dev)
{
	struct dev_data *d = dev_get_drvdata(dev);
	unsigned long freq = 0;

	find_freq(&d->dp, &freq, 0);

	spin_lock(&d->vote_spinlock);
	set_dev_freq(d, freq);
	spin_unlock(&d->vote_spinlock);

	devfreq_remove_device(d->df);
	return 0;
}

int devfreq_suspend_devbw(struct device *dev)
{
	struct dev_data *d = dev_get_drvdata(dev);
	unsigned long freq = 0;

	find_freq(&d->dp, &freq, 0);

	spin_lock(&d->vote_spinlock);
	set_dev_freq(d, freq);
	spin_unlock(&d->vote_spinlock);

	return devfreq_suspend_device(d->df);
}

int devfreq_resume_devbw(struct device *dev)
{
	struct dev_data *d = dev_get_drvdata(dev);
	return devfreq_resume_device(d->df);
}

static int devfreq_devbw_probe(struct platform_device *pdev)
{
	return devfreq_add_devbw(pdev);
}

static int devfreq_devbw_remove(struct platform_device *pdev)
{
	return devfreq_remove_devbw(&pdev->dev);
}

static struct of_device_id match_table[] = {
	{ .compatible = "hisi,devbw", .data="memlat_cpu" },
	{ .compatible = "hisi,l3-devbw", .data="l3_memlat_cpu"},
	{}
};

static struct platform_driver devbw_driver = {
	.probe = devfreq_devbw_probe,
	.remove = devfreq_devbw_remove,
	.driver = {
		.name = "devbw",
		.of_match_table = match_table,
		.owner = THIS_MODULE,
	},
};

static int __init devbw_init(void)
{
	platform_driver_register(&devbw_driver);
	return 0;
}
device_initcall(devbw_init);

MODULE_DESCRIPTION("Device DDR Frequency hw-voting driver");
MODULE_LICENSE("GPL v2");
