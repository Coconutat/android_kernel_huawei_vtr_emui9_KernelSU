/*
 *  linux/drivers/cpufreq/cpufreq_performance.c
 *
 *  Copyright (C) 2002 - 2003 Dominik Brodowski <linux@brodo.de>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/module.h>

#ifdef CONFIG_ARCH_HISI
extern int get_lowbatteryflag(void);
extern void set_lowBatteryflag(int flag);
extern int hisi_test_fast_cpu(int cpu);
#endif

static void cpufreq_gov_performance_limits(struct cpufreq_policy *policy)
{
#ifdef CONFIG_ARCH_HISI
	unsigned int utarget = policy->max;
#endif
	pr_debug("setting to %u kHz\n", policy->max);
#ifdef CONFIG_ARCH_HISI
	if ((get_lowbatteryflag() == 1) && hisi_test_fast_cpu(policy->cpu))
		utarget = policy->min;

	pr_info("%s utarget=%d\n", __func__, utarget);

	__cpufreq_driver_target(policy, utarget, CPUFREQ_RELATION_H);
#else
	__cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
#endif
}

#ifdef CONFIG_ARCH_HISI
static void cpufreq_gov_performance_hisi_exit(struct cpufreq_policy *policy)
{
	set_lowBatteryflag(0);
}
#endif

#ifdef CONFIG_CPU_FREQ_GOV_PERFORMANCE_MODULE
static
#endif
struct cpufreq_governor cpufreq_gov_performance = {
	.name		= "performance",
	.owner		= THIS_MODULE,
	.limits		= cpufreq_gov_performance_limits,
#ifdef CONFIG_ARCH_HISI
	.exit		= cpufreq_gov_performance_hisi_exit,
#endif
};

static int __init cpufreq_gov_performance_init(void)
{
	return cpufreq_register_governor(&cpufreq_gov_performance);
}

static void __exit cpufreq_gov_performance_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_performance);
}

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_PERFORMANCE
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return &cpufreq_gov_performance;
}
#endif
#ifndef CONFIG_CPU_FREQ_GOV_PERFORMANCE_MODULE
struct cpufreq_governor *cpufreq_fallback_governor(void)
{
	return &cpufreq_gov_performance;
}
#endif

MODULE_AUTHOR("Dominik Brodowski <linux@brodo.de>");
MODULE_DESCRIPTION("CPUfreq policy governor 'performance'");
MODULE_LICENSE("GPL");

fs_initcall(cpufreq_gov_performance_init);
module_exit(cpufreq_gov_performance_exit);
