/*
 * Hisilicon Platforms CPU HighFreq hotplug support
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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt


#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/topology.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/pm_opp.h>

#define CREATE_TRACE_POINTS
#include <trace/events/hifreq_hotplug.h>

#define BIG_CLUSTER	1
#define THRESHOLD_FREQ		2112000
#define REAL_FREQ_INDEX_ADDR	0xFFF0A41CUL
#define MAX_BUF_LEN	16

static struct task_struct *hotplug_task;
static struct cpumask hotplug_cpumask;
struct driver_data {
	spinlock_t hotplug_lock;
	struct cpufreq_freqs freqs;
	unsigned int cpu;
	/* request hotplug up when sum of rq->nr_running over this value */
#define DEFAULT_UP_NR_THRESHOLD	2
	unsigned int up_nr_threshold;
	/* request hotplug down when sum of rq->nr_running below this value */
#define DEFAULT_DOWN_NR_THRESHOLD	2
	unsigned int down_nr_threshold;
	/* req_up_cnt reach this value, hotplug up */
#define DEFAULT_UP_CNT_THRESHOLD	1
	unsigned int up_cnt_threshold;
	/* req_down_cnt reach this value, hotplug down */
#define DEFAULT_DOWN_CNT_THRESHOLD	15
	unsigned int down_cnt_threshold;
	/* record request hotplug up or down count */
	unsigned int req_up_cnt;
	unsigned int req_down_cnt;
	/* least time stay in up when last hotplug up */
#define DEFAULT_STAY_UP_DELAY	(300 * USEC_PER_MSEC)
	unsigned int stay_up_delay;
	/* least time stay in down when last hotplug down */
#define DEFAULT_STAY_DOWN_DELAY	(0 * USEC_PER_MSEC)
	unsigned int stay_down_delay;
#define DEFAULT_SINGLE_CORE_LOAD_UP_THRESHOLD	(30)
	unsigned int single_core_load_up_threshold;
#define DEFAULT_SINGLE_CORE_LOAD_DOWN_THRESHOLD	(80)
	unsigned int single_core_load_down_threshold;
	s64 last_up_time;
	s64 last_down_time;
	bool need_up;
	bool need_down;
	bool hotplugged_down;
	bool hotplug_in_progress;
	unsigned int boost;
};

static struct driver_data bL_cpufreq_data;
static void __iomem *real_freq_index_base;
static bool hifreq_hotplug_enabled;
static unsigned int bL_hifreq_max_load;

void bL_hifreq_hotplug_boost(void);

void set_bL_hifreq_load(unsigned int max_load)
{
	bL_hifreq_max_load = max_load;
}
EXPORT_SYMBOL_GPL(set_bL_hifreq_load);

bool cpufreq_hotplugged(int cpu)
{
	if (cpumask_test_cpu(cpu, &hotplug_cpumask))
		return bL_cpufreq_data.hotplugged_down;
	else
		return false;
}
EXPORT_SYMBOL_GPL(cpufreq_hotplugged);

bool hifreq_hotplug_is_enabled(void)
{
	return hifreq_hotplug_enabled;
}

static int bL_hifreq_hotplug_set_rate(struct device *dev, unsigned int rate)
{
	return dev_pm_opp_set_rate(dev, (unsigned long)rate * 1000);
}

#ifdef CONFIG_SCHED_HMP_BOOST
extern int set_hmp_boostpulse(int duration);
extern int get_hmp_boost(void);
#endif

extern unsigned int cpu_nr_runnings(struct cpumask *mask);
extern struct cpufreq_frequency_table *cpufreq_frequency_get_table(unsigned int cpu);

/*********************************************************************
 *                          SYSFS INTERFACE                          *
 *********************************************************************/
/*lint -e715 -e713*/
static ssize_t show_boost(struct kobject *kobj,
				 struct attribute *attr, char *buf)
{
	return snprintf(buf, MAX_BUF_LEN, "%u\n", bL_cpufreq_data.boost);
}

static ssize_t store_boost(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = kstrtouint(buf, 0, &val);
	if (ret < 0)
		return ret;

	if (val != 0 && val != 1)
		return count;

	bL_cpufreq_data.boost = val;

	if (val)
		bL_hifreq_hotplug_boost();
	return count;
}

#ifdef HIFREQ_HOTPLUG_SYSFS_ENABLE
static ssize_t show_up_nr_threshold(struct kobject *kobj,
				 struct attribute *attr, char *buf)
{
	return snprintf(buf, MAX_BUF_LEN, "%u\n", bL_cpufreq_data.up_nr_threshold);
}

static ssize_t show_down_nr_threshold(struct kobject *kobj,
				 struct attribute *attr, char *buf)
{
	return snprintf(buf, MAX_BUF_LEN, "%u\n", bL_cpufreq_data.down_nr_threshold);
}

static ssize_t show_up_cnt_threshold(struct kobject *kobj,
				 struct attribute *attr, char *buf)
{
	return snprintf(buf, MAX_BUF_LEN, "%u\n", bL_cpufreq_data.up_cnt_threshold);
}

static ssize_t show_down_cnt_threshold(struct kobject *kobj,
				 struct attribute *attr, char *buf)
{
	return snprintf(buf, MAX_BUF_LEN, "%u\n", bL_cpufreq_data.down_cnt_threshold);
}

static ssize_t show_stay_up_delay(struct kobject *kobj,
				 struct attribute *attr, char *buf)
{
	return snprintf(buf, MAX_BUF_LEN, "%u\n", bL_cpufreq_data.stay_up_delay);
}

static ssize_t show_stay_down_delay(struct kobject *kobj,
				 struct attribute *attr, char *buf)
{
	return snprintf(buf, MAX_BUF_LEN, "%u\n", bL_cpufreq_data.stay_down_delay);
}

static ssize_t show_single_core_load_up_threshold(struct kobject *kobj,
				 struct attribute *attr, char *buf)
{
	return snprintf(buf, MAX_BUF_LEN, "%u\n", bL_cpufreq_data.single_core_load_up_threshold);
}

static ssize_t show_single_core_load_down_threshold(struct kobject *kobj,
				 struct attribute *attr, char *buf)
{
	return snprintf(buf, MAX_BUF_LEN, "%u\n", bL_cpufreq_data.single_core_load_down_threshold);
}

static ssize_t store_down_nr_threshold(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = kstrtouint(buf, 0, &val);
	if (ret < 0)
		return ret;

	bL_cpufreq_data.down_nr_threshold = val;
	return count;
}

static ssize_t store_up_nr_threshold(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = kstrtouint(buf, 0, &val);
	if (ret < 0)
		return ret;

	bL_cpufreq_data.up_nr_threshold = val;
	return count;
}

static ssize_t store_down_cnt_threshold(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = kstrtouint(buf, 0, &val);
	if (ret < 0)
		return ret;

	bL_cpufreq_data.down_cnt_threshold = val;
	return count;
}

static ssize_t store_up_cnt_threshold(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = kstrtouint(buf, 0, &val);
	if (ret < 0)
		return ret;

	bL_cpufreq_data.up_cnt_threshold = val;
	return count;
}

static ssize_t store_stay_up_delay(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = kstrtouint(buf, 0, &val);
	if (ret < 0)
		return ret;

	bL_cpufreq_data.stay_up_delay = val;
	return count;
}

static ssize_t store_stay_down_delay(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = kstrtouint(buf, 0, &val);
	if (ret < 0)
		return ret;

	bL_cpufreq_data.stay_down_delay = val;
	return count;
}

static ssize_t store_single_core_load_up_threshold(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = kstrtouint(buf, 0, &val);
	if (ret < 0)
		return ret;

	bL_cpufreq_data.single_core_load_up_threshold = val;
	return count;
}

static ssize_t store_single_core_load_down_threshold(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = kstrtouint(buf, 0, &val);
	if (ret < 0)
		return ret;

	bL_cpufreq_data.single_core_load_down_threshold = val;
	return count;
}
#endif
/*lint +e713 +e715*/

/*lint -e84 -e866 -e778 -e514 -e846*/
#ifdef HIFREQ_HOTPLUG_SYSFS_ENABLE
define_one_global_rw(up_nr_threshold);
define_one_global_rw(down_nr_threshold);
define_one_global_rw(up_cnt_threshold);
define_one_global_rw(down_cnt_threshold);
define_one_global_rw(stay_up_delay);
define_one_global_rw(stay_down_delay);
define_one_global_rw(single_core_load_up_threshold);
define_one_global_rw(single_core_load_down_threshold);
#endif
define_one_global_rw(boost);
/*lint +e84 +e866 +e778 +e514 +e846*/

static void bL_hotplug_sysfs_create(void)
{
	int ret;

#ifdef HIFREQ_HOTPLUG_SYSFS_ENABLE
	ret = sysfs_create_file(cpufreq_global_kobject, &down_nr_threshold.attr);
	if (ret)
		goto err_create_sysfs;

	ret = sysfs_create_file(cpufreq_global_kobject, &up_nr_threshold.attr);
	if (ret)
		goto err_create_sysfs;

	ret = sysfs_create_file(cpufreq_global_kobject, &down_cnt_threshold.attr);
	if (ret)
		goto err_create_sysfs;

	ret = sysfs_create_file(cpufreq_global_kobject, &up_cnt_threshold.attr);
	if (ret)
		goto err_create_sysfs;

	ret = sysfs_create_file(cpufreq_global_kobject, &stay_down_delay.attr);
	if (ret)
		goto err_create_sysfs;

	ret = sysfs_create_file(cpufreq_global_kobject, &stay_up_delay.attr);
	if (ret)
		goto err_create_sysfs;

	ret = sysfs_create_file(cpufreq_global_kobject, &single_core_load_down_threshold.attr);
	if (ret)
		goto err_create_sysfs;

	ret = sysfs_create_file(cpufreq_global_kobject, &single_core_load_up_threshold.attr);
	if (ret)
		goto err_create_sysfs;
#endif

	ret = sysfs_create_file(cpufreq_global_kobject, &boost.attr);
	if (ret)
		goto err_create_sysfs;

	return;

err_create_sysfs:
	pr_err("%s: cannot create sysfs file\n", __func__);
}

void bL_hifreq_hotplug_boost(void)
{
	struct cpufreq_policy *policy = NULL;
	unsigned long flags;
	unsigned int cpu;
	int cluster = 0;
	unsigned int max, cur;

	if (!hifreq_hotplug_enabled)
		return;

	if (IS_ERR_OR_NULL(hotplug_task))
		return;

	/* find an online cpu of big cluster */
	/*lint -e570 -e713 -e737 -e574*/
	for_each_online_cpu(cpu) {
		cluster = topology_physical_package_id(cpu);
		if (cluster != BIG_CLUSTER)
			continue;

		policy = cpufreq_cpu_get(cpu);
		if (!policy) {
			pr_err("%s: no policy found\n", __func__);
			continue;
		}

		bL_cpufreq_data.cpu = cpu;
		break;
	}
	/*lint +e570 +e713 +e737 +e574*/
	if (cluster != BIG_CLUSTER || !policy) {
		pr_err("%s: no policy found %d %d\n", __func__, cpu, bL_cpufreq_data.cpu);
		return;
	}
	max = policy->max;
	cur = policy->cur;

	cpufreq_cpu_put(policy);

	/*lint -e550*/
	spin_lock_irqsave(&(bL_cpufreq_data.hotplug_lock), flags);
	bL_cpufreq_data.freqs.new = max > THRESHOLD_FREQ ? THRESHOLD_FREQ : max;
	bL_cpufreq_data.freqs.old = cur;
	bL_cpufreq_data.need_up = true;
	spin_unlock_irqrestore(&(bL_cpufreq_data.hotplug_lock), flags);
	/*lint +e550*/

	wake_up_process(hotplug_task);

	return;
}

#define FREQ_INDEX_MASK		0xF
static unsigned int bL_cpufreq_get_real_rate(unsigned int cpu)
{
	unsigned int freq, index;
	int cluster, reg_value;
	struct cpufreq_frequency_table *freq_table;

	cluster = topology_physical_package_id(cpu);
	freq_table = cpufreq_frequency_get_table(cpu);
	if (!freq_table) {
		pr_err("%s: Unable to find freq_table\n", __func__);
		return ~0U;
	}

	reg_value = readl(real_freq_index_base);
	index = ((unsigned int)reg_value >> (4 * cluster)) & FREQ_INDEX_MASK;
	freq = freq_table[index].frequency;

	return freq;
}

/*lint -e550*/
static void hifreq_hotplug_cpu(bool offline)
{
	unsigned int cpu;
	struct device *cpu_dev;
	unsigned long flags;

	spin_lock_irqsave(&(bL_cpufreq_data.hotplug_lock), flags);
	bL_cpufreq_data.hotplug_in_progress = true;
	spin_unlock_irqrestore(&(bL_cpufreq_data.hotplug_lock), flags);

	/*lint -e570 -e713 -e574 -e737 -e730*/
	for_each_cpu(cpu, &hotplug_cpumask) {
		cpu_dev = get_cpu_device(cpu);
		device_lock(cpu_dev);

		if (offline) {
			if (cpu_online(cpu))
				cpu_down(cpu);
		} else {
			if (cpu_is_offline(cpu))
				cpu_up(cpu);
		}

		/* update offline manually */
		cpu_dev->offline = offline;
		device_unlock(cpu_dev);
	}
	/*lint +e570 +e713 +e574 +e737 +e730*/

	spin_lock_irqsave(&(bL_cpufreq_data.hotplug_lock), flags);
	bL_cpufreq_data.hotplug_in_progress = false;
	bL_cpufreq_data.hotplugged_down = offline;
	if (offline)
		bL_cpufreq_data.last_down_time = ktime_to_us(ktime_get());
	else
		bL_cpufreq_data.last_up_time = ktime_to_us(ktime_get());
	spin_unlock_irqrestore(&(bL_cpufreq_data.hotplug_lock), flags);
}

/*lint -e715 -e446 -e666*/
static int cpufreq_bL_hotplug_task(void *data)
{
	struct cpufreq_policy *policy;
	int ret;
	unsigned long flags;
	struct driver_data tmp_data;
	int retry_count;
	struct device *cpu_dev;

	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);
		spin_lock_irqsave(&(bL_cpufreq_data.hotplug_lock), flags);

		if ((!(bL_cpufreq_data.need_up)) && (!(bL_cpufreq_data.need_down))) {
			spin_unlock_irqrestore(&(bL_cpufreq_data.hotplug_lock), flags);
			schedule();

			if (kthread_should_stop())
				break;

			spin_lock_irqsave(&(bL_cpufreq_data.hotplug_lock), flags);
		}

		set_current_state(TASK_RUNNING);
		tmp_data = bL_cpufreq_data;
		bL_cpufreq_data.need_down = false;
		bL_cpufreq_data.need_up = false;
		spin_unlock_irqrestore(&(bL_cpufreq_data.hotplug_lock), flags);

		if (tmp_data.need_down)
			hifreq_hotplug_cpu(true); /*lint !e747*/

		if (tmp_data.freqs.old != tmp_data.freqs.new) {
			policy = cpufreq_cpu_get(tmp_data.cpu);
			if (!policy) {
				pr_err("%s: Failed to get policy: %d\n", __func__, tmp_data.cpu);
				break;
			}

			cpu_dev = get_cpu_device(tmp_data.cpu);
			if (!cpu_dev) {
				pr_err("%s: Failed to get policy: %d\n", __func__, tmp_data.cpu);
				break;
			}

			cpufreq_freq_transition_begin(policy, &(tmp_data.freqs));
			ret = bL_hifreq_hotplug_set_rate(cpu_dev, tmp_data.freqs.new);
			if (ret)
				pr_err("%s: Failed to change cpu frequency: %d\n", __func__, tmp_data.freqs.new);
			cpufreq_freq_transition_end(policy, &(tmp_data.freqs), ret);
			cpufreq_cpu_put(policy);
		}

		if (tmp_data.need_up) {
			retry_count = 2;
			/* need delay to wait freq down */
			if (tmp_data.freqs.old > THRESHOLD_FREQ)
				usleep_range(1500UL, 2500UL);

retry:
			if (bL_cpufreq_get_real_rate(tmp_data.cpu) > THRESHOLD_FREQ) {
				retry_count--;
				pr_err("%s: freq not down\n", __func__);
				if (retry_count > 0) {
					usleep_range(1500UL, 2500UL);
					goto retry;
				}
			} else {
				hifreq_hotplug_cpu(false); /*lint !e747*/
#ifdef CONFIG_SCHED_HMP_BOOST
				set_hmp_boostpulse(20000);
#endif
			}
		}

		trace_hifreq_hotplug_task(tmp_data.cpu, tmp_data.freqs.old,
					tmp_data.freqs.new, bL_cpufreq_data.hotplugged_down);
	}

	return 0;
}
/*lint +e715 +e446 +e666*/

static void bL_hifreq_hotplug_clear_req(void)
{
	bL_cpufreq_data.req_up_cnt = 0;
	bL_cpufreq_data.req_down_cnt = 0;
}

static void inc_up_req(s64 now)
{
	bL_cpufreq_data.req_up_cnt++;
	if (bL_cpufreq_data.req_up_cnt >= bL_cpufreq_data.up_cnt_threshold &&
		now - bL_cpufreq_data.last_down_time >= bL_cpufreq_data.stay_down_delay)
		bL_cpufreq_data.need_up = true;
}

static void inc_down_req(s64 now)
{
	bL_cpufreq_data.req_down_cnt++;
	if (bL_cpufreq_data.req_down_cnt >= bL_cpufreq_data.down_cnt_threshold &&
		now - bL_cpufreq_data.last_up_time >= bL_cpufreq_data.stay_up_delay)
		bL_cpufreq_data.need_down = true;
}

int bL_hifreq_hotplug_set_target(struct cpufreq_policy *policy, struct device *dev,
	unsigned int target_freq)
{
	unsigned int cpu = policy->cpu;
	int ret, hmp_boost = 0;
	unsigned int nr_runnings = 0;
	struct cpufreq_freqs freqs = {.old = policy->cur, .flags = 0};
	unsigned long flags;
	s64 now;
	bool offline = false;
	unsigned int cluster = topology_physical_package_id(cpu);

	/* only big cluster supprot hifreq hotplug now */
	if (cluster != BIG_CLUSTER)
		goto set_freq;

	/* only warsaw support */
	if (!hifreq_hotplug_enabled)
		goto set_freq;

	if (IS_ERR_OR_NULL(hotplug_task))
		goto verify_freq;

	/* performance governor when system boot up, we do nothing */
	if (!strncasecmp(policy->governor->name, "performance", (unsigned long)CPUFREQ_NAME_LEN)) /* [false alarm]: no problem - fortify check */
		goto verify_freq;

	spin_lock_irqsave(&(bL_cpufreq_data.hotplug_lock), flags);
	if (bL_cpufreq_data.hotplug_in_progress) {
		spin_unlock_irqrestore(&(bL_cpufreq_data.hotplug_lock), flags);
		return 0;
	}
	spin_unlock_irqrestore(&(bL_cpufreq_data.hotplug_lock), flags);

#ifdef CONFIG_SCHED_HMP_BOOST
	hmp_boost = get_hmp_boost();
#endif
	now = ktime_to_us(ktime_get());
	nr_runnings = cpu_nr_runnings(policy->cpus);

	spin_lock_irqsave(&(bL_cpufreq_data.hotplug_lock), flags);
	offline = bL_cpufreq_data.hotplugged_down;

	trace_hifreq_hotplug_info(cpu, policy->cur, target_freq, nr_runnings,
		bL_cpufreq_data.boost, offline, bL_hifreq_max_load,
		offline ? bL_cpufreq_data.req_up_cnt : bL_cpufreq_data.req_down_cnt);

	if (offline) {
		if (hmp_boost || bL_cpufreq_data.boost ||
			nr_runnings > bL_cpufreq_data.up_nr_threshold ||
			bL_hifreq_max_load < bL_cpufreq_data.single_core_load_up_threshold)
			inc_up_req(now);
		else
			bL_hifreq_hotplug_clear_req();

		if (bL_cpufreq_data.need_up == true) {
			bL_hifreq_hotplug_clear_req();
			bL_cpufreq_data.cpu = cpu;
			bL_cpufreq_data.freqs.new = policy->max > THRESHOLD_FREQ ? THRESHOLD_FREQ : policy->max;
			bL_cpufreq_data.freqs.old = policy->cur;
		} else {
			spin_unlock_irqrestore(&(bL_cpufreq_data.hotplug_lock), flags);
			goto set_freq;
		}
	} else {
		if (!hmp_boost && !bL_cpufreq_data.boost &&
			(bL_hifreq_max_load * policy->cur / THRESHOLD_FREQ) > bL_cpufreq_data.single_core_load_down_threshold &&
			nr_runnings <= bL_cpufreq_data.down_nr_threshold)
			inc_down_req(now);
		else
			bL_hifreq_hotplug_clear_req();

		if (bL_cpufreq_data.need_down == true) {
			bL_hifreq_hotplug_clear_req();
			bL_cpufreq_data.cpu = cpu;
			bL_cpufreq_data.freqs.new = policy->max;
			bL_cpufreq_data.freqs.old = policy->cur;
		} else {
			spin_unlock_irqrestore(&(bL_cpufreq_data.hotplug_lock), flags);
			goto verify_freq;
		}
	}

	trace_hifreq_hotplug_wakeup(cpu, policy->cur,
				target_freq, nr_runnings, bL_cpufreq_data.boost, offline);

	spin_unlock_irqrestore(&(bL_cpufreq_data.hotplug_lock), flags);
	wake_up_process(hotplug_task);

	return 0;

verify_freq:
	if (target_freq > THRESHOLD_FREQ)
		target_freq = THRESHOLD_FREQ;

set_freq:
	if (target_freq == policy->cur)
		return 0;

	freqs.new = target_freq;
	cpufreq_freq_transition_begin(policy, &freqs);
	ret = bL_hifreq_hotplug_set_rate(dev, freqs.new);
	if (ret)
		pr_err("%s: Failed to change cpu frequency: %d\n", __func__, freqs.new);
	cpufreq_freq_transition_end(policy, &freqs, ret);

	return 0;
}
/*lint +e550*/

void bL_hifreq_hotplug_init(void)
{
	struct device_node *np;
	int ret;
	unsigned int val;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 2};
	struct driver_data *data = &bL_cpufreq_data;

	np = of_find_compatible_node(NULL, NULL, "hisi,hifreq-hotplug");
	if (!np)
		return;

	ret = of_property_read_u32(np, "enabled", &val);
	if (!ret && val)
		hifreq_hotplug_enabled = true;

	if (!hifreq_hotplug_enabled)
		return;

	real_freq_index_base = ioremap(REAL_FREQ_INDEX_ADDR, 0x4UL);
	if (!real_freq_index_base) {
		pr_err("%s: remap real freq addr fail\n", __func__);
		return;
	}

	hotplug_task = kthread_create(cpufreq_bL_hotplug_task, NULL, "bLhotplug");
	if (IS_ERR(hotplug_task)) {
		pr_err("%s: hotplug task create fail\n", __func__);
		return;
	}

	spin_lock_init(&data->hotplug_lock);
	data->up_nr_threshold = DEFAULT_UP_NR_THRESHOLD;
	data->down_nr_threshold = DEFAULT_DOWN_NR_THRESHOLD;
	data->up_cnt_threshold = DEFAULT_UP_CNT_THRESHOLD;
	data->down_cnt_threshold = DEFAULT_DOWN_CNT_THRESHOLD;
	data->stay_up_delay = DEFAULT_STAY_UP_DELAY;
	data->stay_down_delay = DEFAULT_STAY_DOWN_DELAY;
	data->req_up_cnt = 0;
	data->req_down_cnt = 0;
	data->need_up = false;
	data->need_down = false;
	data->hotplugged_down = false;
	data->hotplug_in_progress = false;
	data->last_up_time = ktime_to_us(ktime_get());
	data->last_down_time = data->last_up_time;
	data->single_core_load_up_threshold = DEFAULT_SINGLE_CORE_LOAD_UP_THRESHOLD;
	data->single_core_load_down_threshold = DEFAULT_SINGLE_CORE_LOAD_DOWN_THRESHOLD;
	data->boost = 1;

	/* physical cpu 4&5(logical 6&7) to be hotplugged up and down */
	cpumask_set_cpu(6, &hotplug_cpumask);
	cpumask_set_cpu(7, &hotplug_cpumask);

	bL_hotplug_sysfs_create();

	sched_setscheduler_nocheck(hotplug_task, SCHED_FIFO, &param);
	get_task_struct(hotplug_task);
	/* NB: wake up so the thread does not look hung to the freezer */
	wake_up_process(hotplug_task);
}

void bL_hifreq_hotplug_exit(void)
{
	if (!hifreq_hotplug_enabled)
		return;

	kthread_stop(hotplug_task);
	put_task_struct(hotplug_task);
	iounmap(real_freq_index_base);
}
