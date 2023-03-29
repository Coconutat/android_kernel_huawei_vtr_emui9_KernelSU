/*
 * Hisilicon Platforms LOW TEMPERATURE cpu frequency set support
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
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/pm_qos.h>
#include <linux/hisi/hisi_rproc.h>
#include <linux/hisi/ipc_msg.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/debugfs.h>


struct cpu_lowtemp {
	void __iomem *base;
	uint32_t lowtemp_reg;
	uint32_t reg_offset;
};
static struct cpu_lowtemp *cpu_lowtemp_data;

bool is_low_temprature(void)
{
	unsigned int temprature = 0;

	temprature = readl(cpu_lowtemp_data->base + cpu_lowtemp_data->lowtemp_reg);
	temprature = temprature >> cpu_lowtemp_data->reg_offset & 0x3;

	if (temprature)
		return true;
	else
		return false;
}

static int __init cpufreq_lowtemp_init(void)
{
	int ret;
	struct device_node *np = NULL;
	uint32_t reg = 0, offset = 0;
	void __iomem *base;

	cpu_lowtemp_data = kzalloc(sizeof(struct cpu_lowtemp), GFP_KERNEL);
	if (!cpu_lowtemp_data)
		return -ENOMEM;

	np = of_find_compatible_node(NULL, NULL, "hisi,cpu-lowtemp");
	if (!np) {
		pr_err("[%s] no compatible node found.\n", __func__);
		ret =  -EPERM;
		goto out_find_node;
	}

	base = of_iomap(np, 0);
	if (!base) {
		pr_err("[%s] base iomap fail\n", __func__);
		ret = -ENOMEM;
		goto out_iomap;
	}
	ret = of_property_read_u32(np, "hisi,lowtemp-reg", &reg);
	if (ret) {
		pr_err("[%s] get lowtemp reg failed.\n", __func__);
		ret = -EPERM;
		goto out;
	}

	ret = of_property_read_u32(np, "hisi,lowtemp-reg-offset", &offset);
	if (ret) {
		pr_err("[%s] get lowtemp reg offset failed.\n", __func__);
		ret = -EPERM;
		goto out;
	}
	cpu_lowtemp_data->base = base;
	cpu_lowtemp_data->lowtemp_reg = reg;
	cpu_lowtemp_data->reg_offset = offset;

	return 0;

out:
	iounmap(base);
out_iomap:
	of_node_put(np);
out_find_node:
	kfree(cpu_lowtemp_data);
	cpu_lowtemp_data = NULL;
	return ret;
}
module_init(cpufreq_lowtemp_init);

#ifdef CONFIG_HISI_CPUFREQ_LOWTEMP_DEBUG
static struct dentry *cpufreq_lowtemp_debug_dir;

static int cpufreq_lowtemp_debugfs_show(struct seq_file *s, void *data)
{
	bool lowtemp;

	if (!s) {
		pr_err("[%s] seq_file is null\n", __func__);
		return -EINVAL;
	}

	lowtemp = is_low_temprature();
	seq_printf(s, "lowtemp state: %d\n", lowtemp);

	return 0;
}

static int cpufreq_lowtemp_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, cpufreq_lowtemp_debugfs_show, inode->i_private);
}

static const struct file_operations cpufreq_lowtemp_debugfs_fops = {
	.owner   = THIS_MODULE,
	.open    = cpufreq_lowtemp_debugfs_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int __init cpufreq_lowtemp_debug_init(void)
{
	int ret = -ENODEV;

	cpufreq_lowtemp_debug_dir = debugfs_create_dir("hisi_cpulowtemp_debug", NULL);
	if (IS_ERR_OR_NULL(cpufreq_lowtemp_debug_dir)) {
		pr_err("[%s] create cpulowtemp_debug_dir fail\n", __func__);
		goto out;
	}

	if (NULL == debugfs_create_file("lowtemp_debug", 0660, cpufreq_lowtemp_debug_dir, NULL, &cpufreq_lowtemp_debugfs_fops)) {
		debugfs_remove_recursive(cpufreq_lowtemp_debug_dir);
		goto out;
	}

	return 0;
out:
	cpufreq_lowtemp_debug_dir = NULL;
	return ret;
}
module_init(cpufreq_lowtemp_debug_init);

static void __exit cpufreq_lowtemp_debug_exit(void)
{
	debugfs_remove_recursive(cpufreq_lowtemp_debug_dir);
	cpufreq_lowtemp_debug_dir = NULL;
}
module_exit(cpufreq_lowtemp_debug_exit);

#endif
