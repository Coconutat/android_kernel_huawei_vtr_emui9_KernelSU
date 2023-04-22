/*
 * Hisilicon Platforms CPUFREQ-DT support
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
#include <linux/cpumask.h>

#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/hisi/hifreq_hotplug.h>
#include <linux/version.h>

#ifdef CONFIG_HISI_HW_VOTE_CPU_FREQ
#include <linux/hisi/hisi_hw_vote.h>
#endif

#define VERSION_ELEMENTS	1
static unsigned int cpufreq_dt_version = 0;


#ifdef CONFIG_HISI_L2_DYNAMIC_RETENTION
struct l2_retention_ctrl {
	u64 l2_retention_backup;
	u64 l2_retention_dis_mask;
	u64 l2_retention_dis_value;
	u32 l2_retention_dis_cluster;
	u32 l2_retention_dis_freq;
};
static struct l2_retention_ctrl *l2_ret_ctrl = NULL;

u64 l2_retention_read(void)
{
	u64 cfg;

	asm volatile ("MRS %0,S3_1_C11_C0_3\n" \
			: "=r"(cfg) \
			: \
			: "memory");

	return cfg;
}

void l2_retention_write(u64 cfg)
{
	asm volatile ("MSR S3_1_C11_C0_3,%0\n" \
			: \
			: "r"(cfg) \
			: "memory");
}

void l2_dynamic_retention_ctrl(struct cpufreq_policy *policy, unsigned int freq)
{
	u64 cfg;
	int cluster;

	if (IS_ERR_OR_NULL(l2_ret_ctrl)) {
		pr_err("%s l2_ret_ctrl not init\n", __func__);
		return;
	}

	cluster = topology_physical_package_id(policy->cpu);
	if (cluster != l2_ret_ctrl->l2_retention_dis_cluster) {
		return;
	}

	if (freq == l2_ret_ctrl->l2_retention_dis_freq) {
		l2_ret_ctrl->l2_retention_backup = l2_retention_read();
		cfg = l2_ret_ctrl->l2_retention_backup & (~(l2_ret_ctrl->l2_retention_dis_mask));
		cfg |= l2_ret_ctrl->l2_retention_dis_value & l2_ret_ctrl->l2_retention_dis_mask;
		l2_retention_write(cfg);
	} else {
		l2_retention_write(l2_ret_ctrl->l2_retention_backup);
	}
}

int l2_dynamic_retention_init(void)
{
	struct device_node *np;
	int ret = -ENODEV;

	l2_ret_ctrl = kzalloc(sizeof(struct l2_retention_ctrl), GFP_KERNEL);
	if (!l2_ret_ctrl) {
		pr_err("%s: alloc l2_retention_ctrl err\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	np = of_find_compatible_node(NULL, NULL, "hisi,l2-retention-dis-freq");
	if (!np) {
		pr_err("[%s] doesn't have hisi,l2-retention-dis-freq node!\n", __func__);
		goto err_out_free;
	}

	ret = of_property_read_u32(np, "dis_retention_cluster", &(l2_ret_ctrl->l2_retention_dis_cluster));
	if (ret) {
		pr_err("[%s]parse dis_retention_cluster fail!\n", __func__);
		goto err_out_free;
	}

	ret = of_property_read_u32(np, "dis_retention_freq", &(l2_ret_ctrl->l2_retention_dis_freq));
	if (ret) {
		pr_err("[%s]parse dis_retention_freq fail!\n", __func__);
		goto err_out_free;
	}

	ret = of_property_read_u64(np, "dis_retention_mask", &(l2_ret_ctrl->l2_retention_dis_mask));
	if (ret) {
		pr_err("[%s]parse dis_retention_mask fail!\n", __func__);
		goto err_out_free;
	}

	ret = of_property_read_u64(np, "dis_retention_value", &(l2_ret_ctrl->l2_retention_dis_value));
	if (ret) {
		pr_err("[%s]parse dis_retention_value fail!\n", __func__);
		goto err_out_free;
	}

	l2_ret_ctrl->l2_retention_backup = l2_retention_read();

	return 0;
err_out_free:
	kfree(l2_ret_ctrl);
	l2_ret_ctrl = NULL;
	of_node_put(np);
err_out:
	return ret;
}
#endif

int hisi_cpufreq_set_supported_hw(struct cpufreq_policy *policy)
{
	int ret, cpu;
	struct device *cpu_dev;

	/* find first cpu of policy->cpus */
	cpu = cpumask_any(policy->cpus);
	cpu_dev = get_cpu_device(cpu);
	if (!cpu_dev) {
		pr_err("%s Failed to get cpu %d device!\n", __func__, cpu);
		return -ENODEV;
	}

	ret = dev_pm_opp_set_supported_hw(cpu_dev, &cpufreq_dt_version, VERSION_ELEMENTS);
	if (ret)
		pr_err("%s Failed to set supported hardware\n", __func__);

	return ret;
}

void hisi_cpufreq_put_supported_hw(struct cpufreq_policy *policy)
{
	int cpu, j;
	struct device *cpu_dev;

	/* find last cpu of policy->related_cpus */
	for_each_cpu(j, policy->related_cpus) {
		cpu = j;
	}
	cpu_dev = get_cpu_device(cpu);
	if (!cpu_dev) {
		pr_err("%s Failed to get cpu %d device!\n", __func__, cpu);
		return;
	}

	dev_pm_opp_put_supported_hw(cpu_dev);
}

static int hisi_cpufreq_get_dt_version(void)
{
	const char *target_cpu;
	int ret, index;
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "hisi,targetcpu");
	if (!np) {
		pr_err("%s Failed to find compatible node:targetcpu\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_string(np, "target_cpu", &target_cpu);
	if (ret) {
		pr_err("%s Failed to read target_cpu\n", __func__);
		of_node_put(np);
		return ret;
	}
	of_node_put(np);

	np = of_find_compatible_node(NULL, NULL, "hisi,supportedtarget");
	if (!np) {
		pr_err("%s Failed to find compatible node:supportedtarget\n", __func__);
		return -ENODEV;
	}

	ret = of_property_match_string(np, "support_name", target_cpu);
	if (ret < 0) {
		pr_err("%s Failed to get support_name\n", __func__);
		of_node_put(np);
		return ret;
	}
	of_node_put(np);

	index = ret;
	cpufreq_dt_version = BIT(index);

	return 0;
}

void hisi_cpufreq_get_suspend_freq(struct cpufreq_policy *policy)
{
	struct device_node *np;
	unsigned int value;
	int cluster, ret;

	np = of_find_compatible_node(NULL, NULL, "hisi,suspend-freq");
	if (!np)
		return;

	cluster = topology_physical_package_id(policy->cpu);
	ret = of_property_read_u32_index(np, "suspend_freq", cluster, &value);
	of_node_put(np);

	/* This overides the suspend opp */
	if (!ret)
		policy->suspend_freq = value;
}

#ifdef CONFIG_HISI_HW_VOTE_CPU_FREQ

struct hvdev *hisi_cpufreq_hv_init(struct device *cpu_dev)
{
	struct device_node *np;
	struct hvdev *cpu_hvdev = NULL;
	const char *ch_name;
	const char *vsrc;
	int ret;

	if (IS_ERR_OR_NULL(cpu_dev)) {
		pr_err("%s: cpu_dev is null!\n", __func__);
		goto err_out;
	}

	np = cpu_dev->of_node;
	if (IS_ERR_OR_NULL(np)) {
		pr_err("%s: cpu_dev no dt node!\n", __func__);
		goto err_out;
	}

	ret = of_property_read_string_index(np, "freq-vote-channel", 0, &ch_name);
	if (ret) {
		pr_err("[%s]parse freq-vote-channel fail!\n", __func__);
		goto err_out;
	}

	ret = of_property_read_string_index(np, "freq-vote-channel", 1, &vsrc);
	if (ret) {
		pr_err("[%s]parse vote src fail!\n", __func__);
		goto err_out;
	}

	cpu_hvdev = hisi_hvdev_register(cpu_dev, ch_name, vsrc);
	if (IS_ERR_OR_NULL(cpu_hvdev)) {
		pr_err("%s: cpu_hvdev register fail!\n", __func__);
	}
err_out:
	return cpu_hvdev;
}

void hisi_cpufreq_hv_exit(struct hvdev *cpu_hvdev, unsigned int cpu)
{
	if (hisi_hvdev_remove(cpu_hvdev))
		pr_err("cpu%d unregister hvdev fail\n", cpu);
}

int hisi_cpufreq_set(struct hvdev *cpu_hvdev, unsigned int freq)
{
	if (IS_ERR_OR_NULL(cpu_hvdev))
		return -ENODEV;

	return hisi_hv_set_freq(cpu_hvdev, freq);
}

unsigned int hisi_cpufreq_get(unsigned int cpu)
{
	struct cpufreq_policy *policy = cpufreq_cpu_get_raw(cpu);

	if (!policy) {
		pr_err("%s: No policy associated to cpu: %d\n", __func__, cpu);
		return 0;
	}

	return policy->cur;
}

void hisi_cpufreq_policy_cur_init(struct hvdev *cpu_hvdev, struct cpufreq_policy *policy)
{
	int ret;
	unsigned int freq_khz = 0;
	unsigned int index = 0;

	ret = hisi_hv_get_result(cpu_hvdev, &freq_khz);
	if (ret) {
		goto exception;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	ret = cpufreq_frequency_table_target(policy, policy->freq_table, freq_khz, CPUFREQ_RELATION_C, &index);
	if (ret) {
		goto exception;
	}
#else
	index = cpufreq_frequency_table_target(policy, freq_khz, CPUFREQ_RELATION_C);
#endif

	policy->cur = policy->freq_table[index].frequency;
	hisi_hv_set_freq(cpu_hvdev, policy->cur);/*update last freq in hv driver*/

	return;
exception:
	pr_err("%s:find freq%d fail\n", __func__, freq_khz);
	policy->cur = policy->freq_table[0].frequency;
	hisi_hv_set_freq(cpu_hvdev, policy->cur);/*update last freq in hv driver*/
}
#endif

static int hisi_cpufreq_init(void)
{
	int ret = 0;
	struct platform_device *pdev;

	if (!of_find_compatible_node(NULL, NULL, "arm,generic-bL-cpufreq"))
		return -ENODEV;

#ifdef CONFIG_HISI_L2_DYNAMIC_RETENTION
	l2_dynamic_retention_init();
#endif

	ret = hisi_cpufreq_get_dt_version();
	if (ret)
		return -EINVAL;

#ifdef CONFIG_HISI_BIG_MAXFREQ_HOTPLUG
	bL_hifreq_hotplug_init();
#endif

	pdev = platform_device_register_simple("cpufreq-dt", -1, NULL, 0);
	if (IS_ERR(pdev))
		return PTR_ERR(pdev);

	return ret;
}
module_init(hisi_cpufreq_init);
