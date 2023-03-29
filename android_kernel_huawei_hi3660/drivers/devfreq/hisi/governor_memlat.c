/*
 * Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
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

#define pr_fmt(fmt) "mem_lat: " fmt

#include <linux/kernel.h>
#include <linux/sizes.h>
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
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/devfreq.h>
#include <linux/slab.h>
#include <linux/cpu_pm.h>
#include <linux/cpufreq.h>
#include "governor.h"
#include "governor_memlat.h"

#include <trace/events/power.h>

#define DEFAULT_TARGET_RATIO 600
static unsigned int default_target_ratios[] = {DEFAULT_TARGET_RATIO};

struct memlat_node {
	unsigned int *target_ratios;
	int ntarget_ratios;
	unsigned int monitor_enable;
	struct mutex mon_mutex_lock;
	bool mon_started;
	struct list_head list;
	void *orig_data;

	/* idle notifier */
	spinlock_t idle_notif_spinlock;
	bool idle_vote_enabled;
	unsigned long prev_idle_freq;

	/*cpufreq notifier*/
	unsigned int min_core_freq;

	bool governor_started;
	bool monitor_paused;
	bool pending_change;
	unsigned int switch_on_cpufreq;
	unsigned int new_cpufreq;
	struct mutex cpufreq_mutex_lock;
	struct delayed_work work;

	struct memlat_hwmon *hw;
	struct devfreq_governor *gov;
	struct attribute_group *attr_grp;

	struct notifier_block perf_event_idle_nb;
	struct notifier_block notifier_trans_block;
};

static LIST_HEAD(memlat_list);
static DEFINE_MUTEX(list_lock);

static int use_cnt;
static DEFINE_MUTEX(state_lock);

#ifdef CONFIG_HISI_DEBUG_FS
static ssize_t show_map(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct devfreq *df = to_devfreq(dev);
	struct memlat_node *n = df->data;
	struct core_dev_map *map = n->hw->freq_map;
	unsigned int cnt = 0;

	cnt += scnprintf(buf, PAGE_SIZE, "Core freq (MHz)\tDevice BW\n");

	while (map->core_mhz && cnt < PAGE_SIZE) {
		cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "%15u\t%9u\n",
				map->core_mhz, map->target_freq);
		map++;
	}
	if (cnt < PAGE_SIZE)
		cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "\n");

	return cnt;
}

static DEVICE_ATTR(freq_map, 0444, show_map, NULL);

static ssize_t show_node_info(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct devfreq *df = to_devfreq(dev);
	struct memlat_node *node = df->data;
	unsigned int cnt = 0;

	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "node: %s\n", dev_name(node->hw->dev));
	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "\t->monitor_enable=%u\n", node->monitor_enable);
	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "\t->switch_on_freq=%d\n", node->switch_on_cpufreq);
	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "\t->monitor_paused=%d\n", node->monitor_paused);
	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "\t->new_cpufreq=%d\n", node->new_cpufreq);
	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "\t->pending_change=%d\n", node->pending_change);
	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "\t->idle_vote_enabled=%d\n", node->idle_vote_enabled);
	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "\n");

	return cnt;
}
static DEVICE_ATTR(node, 0444, show_node_info, NULL);
#endif
static unsigned int *get_tokenized_data(const char *buf, int *num_tokens)
{
	const char *cp;
	int i;
	int ntokens = 1;
	unsigned int *tokenized_data;
	int err = -EINVAL;

	cp = buf;
	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if (!(ntokens & 0x1))
		goto err;

	tokenized_data = kmalloc(ntokens * sizeof(unsigned int), GFP_KERNEL);
	if (!tokenized_data) {
		err = -ENOMEM;
		goto err;
	}

	cp = buf;
	i = 0;
	while (i < ntokens) {
		if (sscanf(cp, "%u", &tokenized_data[i++]) != 1) /* [false alarm]:fortify */
			goto err_kfree;

		cp = strpbrk(cp, " :");
		if (!cp)
			break;
		cp++;
	}

	if (i != ntokens)
		goto err_kfree;

	*num_tokens = ntokens;
	return tokenized_data;

err_kfree:
	kfree(tokenized_data);
err:
	return ERR_PTR(err);
}
static ssize_t show_target_ratios(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int i;
	ssize_t ret = 0;
	struct devfreq *df = to_devfreq(dev);
	struct memlat_node *n = df->data;

	mutex_lock(&n->mon_mutex_lock);
	for (i = 0; i < n->ntarget_ratios; i++)
		ret += sprintf(buf + ret, "%u%s", n->target_ratios[i],
			       i & 0x1 ? ":" : " "); /*lint !e421*/

	sprintf(buf + ret - 1, "\n"); /*lint !e421*/
	mutex_unlock(&n->mon_mutex_lock);

	return ret;
}

static ssize_t store_target_ratios(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	int ntokens;
	unsigned int *new_target_ratios = NULL;
	struct devfreq *df = to_devfreq(dev);
	struct memlat_node *n = df->data;

	mutex_lock(&n->mon_mutex_lock);
	new_target_ratios = get_tokenized_data(buf, &ntokens);
	if (IS_ERR(new_target_ratios)) {
		mutex_unlock(&n->mon_mutex_lock);
		return PTR_RET(new_target_ratios);
	}

	if (n->target_ratios != default_target_ratios)
		kfree(n->target_ratios);
	n->target_ratios = new_target_ratios;
	n->ntarget_ratios = ntokens;
	mutex_unlock(&n->mon_mutex_lock);
	return count;
}
static DEVICE_ATTR(target_ratio, 0660, show_target_ratios, store_target_ratios);

static unsigned int freq_to_targetratio(
	struct memlat_node *node, unsigned int freq)
{
	int i;
	unsigned int ret;

	for (i = 0; i < node->ntarget_ratios - 1 &&
		    freq >= node->target_ratios[i+1]; i += 2) /*lint !e679*/
		;

	ret = node->target_ratios[i];

	return ret;
}

static unsigned long core_to_dev_freq(struct memlat_node *node,
		unsigned long coref)
{
	struct memlat_hwmon *hw = node->hw;
	struct core_dev_map *map = hw->freq_map;
	unsigned long freq = 0;

	if (unlikely(!map))
		goto out;

	while (map->core_mhz && map->core_mhz < coref)
		map++;
	if (!map->core_mhz)
		map--;
	freq = map->target_freq;

out:
	pr_debug("freq: %lu -> dev: %lu\n", coref, freq);
	return freq;
}

static unsigned long get_max_possible_freq(struct memlat_node *node, unsigned int ratio, unsigned long coref)
{
	int index = -1;
	unsigned long limited_coref = 0;
	int i;

	struct memlat_hwmon *hw = node->hw;
	struct core_dev_map *map = hw->freq_map;

	if (unlikely(!map))
		return 0;

	/* find ratio with max_core_freq */
	for (i = node->ntarget_ratios -1; i >= 0; i-=2) {
		if (ratio <= node->target_ratios[i]) {
			index = i;
			break;
		}
	}

	if (index != -1) {
		index++;

		if (index >= node->ntarget_ratios)
			return 0;

		limited_coref = node->target_ratios[index];

		if (limited_coref <= coref) {
			while (map->core_mhz && map->core_mhz <= limited_coref)
				map++;

			/* max_freq */
			map--;

			return map->core_mhz;
		}
	}

	return 0;
}

static struct memlat_node *find_memlat_node(struct devfreq *df)
{
	struct memlat_node *node, *found = NULL;

	mutex_lock(&list_lock);
	list_for_each_entry(node, &memlat_list, list)
		if (node->hw->dev == df->dev.parent ||
		    node->hw->of_node == df->dev.parent->of_node) {
			found = node;
			break;
		}
	mutex_unlock(&list_lock);

	return found;
}

unsigned long get_max_target_freq(int cpu, unsigned long cur_cpu_freq)
{
	struct memlat_node *node;
	struct memlat_hwmon *hw, *found = NULL;
	unsigned long freq = 0;

	mutex_lock(&list_lock);
	if (list_empty(&memlat_list)) {
		mutex_unlock(&list_lock);
		return ULONG_MAX;
	}

	list_for_each_entry(node, &memlat_list, list) {
		if (node->mon_started != true)
			continue;
		hw = node->hw;
		if (cpumask_test_cpu(cpu, &hw->cpus)) {
			found = hw;
			break;
		}
	}
	mutex_unlock(&list_lock);

	/* if not found, do not limit the freq */
	if (!found)
		return ULONG_MAX;

	freq = core_to_dev_freq(node, cur_cpu_freq);

	return freq;
}

unsigned int get_min_core_freq(int cpu, unsigned long cur_dev_freq)
{
	struct memlat_node *node;
	struct memlat_hwmon *hw, *found = NULL;
	struct core_dev_map *map;
	unsigned long freq = 0;

	if (!mutex_trylock(&list_lock))
		return INT_MAX;

	if (list_empty(&memlat_list)) {
		mutex_unlock(&list_lock); /*lint !e455*/
		goto out;
	}

	list_for_each_entry(node, &memlat_list, list) {
		if (node->mon_started != true)
			continue;
		hw = node->hw;
		if (cpumask_test_cpu(cpu, &hw->cpus)) {
			found = hw;
			break;
		}
	}
	mutex_unlock(&list_lock);/*lint !e455*/

	/* if not found, do not limit the freq */
	if (!found)
		goto out;

	map = hw->freq_map;
	if (unlikely(!map))
		goto out;

	while (map->core_mhz && map->target_freq < cur_dev_freq)
		map++;

	if (!map->core_mhz)
		map--;

	freq = map->core_mhz;
	return freq;

out:
	return 0;
}

static int event_idle_notif(struct notifier_block *nb, unsigned long action,
                                                        void *data)
{
	bool cores_pwrdn;
	struct memlat_node *d = container_of(nb, struct memlat_node, perf_event_idle_nb);
	struct memlat_hwmon *hw;
	int cpu = smp_processor_id();
	struct devfreq *df = NULL;

	/* device hasn't been initilzed yet */
	if (!d->idle_vote_enabled || d->monitor_paused || !d->monitor_enable || !d->mon_started)
		return NOTIFY_OK;

	hw = d->hw;
	if (!hw || !cpumask_test_cpu(cpu, &hw->cpus))
		return NOTIFY_OK;

	df = hw->df;

	switch (action) {
	case CPU_PM_ENTER:
			cores_pwrdn = hisi_cluster_cpu_all_pwrdn();
			if (cores_pwrdn) {
				spin_lock(&d->idle_notif_spinlock);
				d->prev_idle_freq = get_dev_votefreq(df->dev.parent);
				set_dev_votefreq(df->dev.parent, 0);
				spin_unlock(&d->idle_notif_spinlock);
				trace_memlat_set_dev_freq(dev_name(df->dev.parent), "idle_vote", \
					cpu, d->min_core_freq, 0, d->prev_idle_freq);
			}

			break;

	case CPU_PM_ENTER_FAILED:
	case CPU_PM_EXIT:
			spin_lock(&d->idle_notif_spinlock);
			if (d->prev_idle_freq != 0
				&& d->prev_idle_freq != get_dev_votefreq(df->dev.parent)) {
				set_dev_votefreq(df->dev.parent, d->prev_idle_freq);
				d->prev_idle_freq = 0;

				trace_memlat_set_dev_freq(dev_name(df->dev.parent), "idle_restore_vote", \
					cpu, d->min_core_freq, 0, d->prev_idle_freq);
			}
			spin_unlock(&d->idle_notif_spinlock);

			break;
	}

	return NOTIFY_OK;
}

int start_monitor(struct devfreq *df);
void stop_monitor(struct devfreq *df);

static void cpufreq_trans_handler(struct work_struct *work)
{
	struct memlat_node *d;
	bool need_start = false;
	bool need_stop = false;

	d = container_of(work, struct memlat_node, work.work);

	mutex_lock(&d->cpufreq_mutex_lock);
	if (d->new_cpufreq < d->switch_on_cpufreq) {
		if (!d->monitor_paused) {
			need_stop = true;
			d->pending_change = true;
		}
	} else {
		if (d->monitor_paused) {
			need_start= true;
			d->pending_change = true;
		}
	}
	mutex_unlock(&d->cpufreq_mutex_lock);

	mutex_lock(&d->mon_mutex_lock);
	if (need_stop)
		stop_monitor(d->hw->df);

	if (need_start) {
		if (unlikely(start_monitor(d->hw->df))) {
			need_start = false;
		}
	}
	mutex_unlock(&d->mon_mutex_lock);

	mutex_lock(&d->cpufreq_mutex_lock);
	/* update monitor_paused */
	if (need_stop)
		d->monitor_paused = true;

	if (need_start)
		d->monitor_paused = false;

	d->pending_change = false;
	mutex_unlock(&d->cpufreq_mutex_lock);
}


static int cpufreq_notifier_trans(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct cpufreq_freqs *freq = (struct cpufreq_freqs *)data;
	unsigned int cpu = freq->cpu, new_freq = freq->new;
	struct memlat_node *d;
	unsigned long max_target_freq;
	unsigned int min_core_freq;
	struct devfreq *df = NULL;
	struct memlat_hwmon *hw;

	if (val != CPUFREQ_POSTCHANGE || !new_freq)
		return 0;

	d = container_of(nb, struct memlat_node, notifier_trans_block);
	/* device hasn't been initilzed yet */
	if (!d->monitor_enable || !d->governor_started)
		return NOTIFY_OK;

	/*
	 * if the hw is not ready or the cpu isn't in the list,
	 * then ignore the notification
	 */
	hw = d->hw;
	if (!hw || !cpumask_test_cpu(cpu, &hw->cpus))
		return NOTIFY_OK;

	df = hw->df;

	/* KHZ -> MHZ */
	new_freq = new_freq / 1000;

	mutex_lock(&d->cpufreq_mutex_lock);
	if (!d->pending_change) {
		if (new_freq < d->switch_on_cpufreq && d->monitor_paused) {
			mutex_unlock(&d->cpufreq_mutex_lock);
			return 0;
		}

		/* to keep the d->new_cpufreq is only update once */
		if (d->new_cpufreq != new_freq) {
			d->new_cpufreq = new_freq;
			queue_delayed_work(system_power_efficient_wq, &d->work, 0);
		}
	}
	mutex_unlock(&d->cpufreq_mutex_lock);

	max_target_freq = get_max_target_freq(cpu, new_freq);
	if (max_target_freq == ULONG_MAX) {
		return 0;
	}
	min_core_freq = get_min_core_freq(cpu, max_target_freq);
	/*
	 * need to re-evaluate if the cpu_freq
	 * is lower than the threshold.
	 */
	spin_lock(&d->idle_notif_spinlock);
	if (new_freq < d->min_core_freq) {
		trace_memlat_set_dev_freq(dev_name(df->dev.parent), "freq_change", \
				cpu, d->min_core_freq, new_freq, max_target_freq);
		set_dev_votefreq(df->dev.parent, max_target_freq);
		d->min_core_freq = min_core_freq;
	}
	spin_unlock(&d->idle_notif_spinlock);


	return 0;
}

static ssize_t show_monitor_enable(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	ssize_t ret = 0;
	struct devfreq *df = to_devfreq(dev);
	struct memlat_node *node = df->data;

	mutex_lock(&node->mon_mutex_lock);
	ret = scnprintf(buf, PAGE_SIZE, "%u\n", node->monitor_enable);
	mutex_unlock(&node->mon_mutex_lock);

	return ret;
}

static ssize_t store_monitor_enable(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	struct devfreq *df = to_devfreq(dev);
	struct memlat_node *node = df->data;
	unsigned int val;
	int ret = 0;

	ret = kstrtouint(buf, 10, &val);
	if (ret)
		return ret;

	if (val != 0)
		val = 1;

	mutex_lock(&node->mon_mutex_lock);
	if (node->monitor_enable == 0 && val == 1) {
		node->monitor_enable = 1;
		if (unlikely(start_monitor(df))) {
			/* handle error */
			node->monitor_enable = 0;
			ret = -EINVAL;
		} else {
			/* update monitor status */
			mutex_lock(&node->cpufreq_mutex_lock);
			node->monitor_paused = false;
			node->pending_change = false;
			mutex_unlock(&node->cpufreq_mutex_lock);
		}
	} else if (node->monitor_enable == 1 && val == 0) {
		stop_monitor(df);
		node->monitor_enable = 0;
	}
	mutex_unlock(&node->mon_mutex_lock);

	return  ret ? ret : count;
}
static DEVICE_ATTR(monitor_enable, 0660, show_monitor_enable, store_monitor_enable);


int start_monitor(struct devfreq *df)
{
	struct memlat_node *node = df->data;
	struct memlat_hwmon *hw;
	struct device *dev;
	int ret;

	if (!node || !node->monitor_enable || node->mon_started) {
		return 0;
	}

	hw = node->hw;
	dev = df->dev.parent;

	ret = hw->start_hwmon(hw);

	if (ret) {
		dev_err(dev, "Unable to start HW monitor! (%d)\n", ret);
		hw->stop_hwmon(hw);
		return ret;
	}

	mutex_lock(&df->lock);
	devfreq_monitor_start(df);
	mutex_unlock(&df->lock);

	node->mon_started = true;

	return 0;
}

void stop_monitor(struct devfreq *df)
{
	struct memlat_node *node;
	struct memlat_hwmon *hw;

	if (!df)
		return ;

	node = df->data;
	if (!node || !node->monitor_enable || !node->mon_started) {
		return ;
	}

	hw = node->hw;

	node->mon_started = false;

	devfreq_monitor_stop(df);
	hw->stop_hwmon(hw);

	/* set dev vote freq to 0 */
	set_dev_votefreq(df->dev.parent, 0);
	trace_memlat_set_dev_freq(dev_name(df->dev.parent), "stop_monitor", \
		0, 0, 0, 0);
	spin_lock(&node->idle_notif_spinlock);
	node->prev_idle_freq = 0;
	spin_unlock(&node->idle_notif_spinlock);
}

static int gov_start(struct devfreq *df)
{
	int ret = -ENODEV;
	struct device *dev = df->dev.parent;
	struct memlat_node *node;
	struct memlat_hwmon *hw;

	node = find_memlat_node(df);
	if (!node) {
		dev_err(dev, "Unable to find HW monitor!\n");
		return ret;
	}

	node->ntarget_ratios = ARRAY_SIZE(default_target_ratios);
	node->target_ratios = default_target_ratios;
	hw = node->hw;

	hw->df = df;
	node->orig_data = df->data;
	df->data = node;

	mutex_lock(&node->mon_mutex_lock);
	node->monitor_enable = 1;

	if (unlikely(start_monitor(df))) {
		node->monitor_enable = 0;
		mutex_unlock(&node->mon_mutex_lock);
		goto err_start;
	}
	mutex_unlock(&node->mon_mutex_lock);

	ret = sysfs_create_group(&df->dev.kobj, node->attr_grp);
	if (ret)
		goto err_sysfs;

	node->governor_started = true;
	return 0;

err_sysfs:
	stop_monitor(df);
err_start:
	df->data = node->orig_data;
	node->orig_data = NULL;
	hw->df = NULL;

	return ret;
}

static void gov_stop(struct devfreq *df)
{
	struct memlat_node *node = df->data;
	struct memlat_hwmon *hw = node->hw;

	node->governor_started = false;
	sysfs_remove_group(&df->dev.kobj, node->attr_grp);
	mutex_lock(&node->mon_mutex_lock);
	stop_monitor(df);
	node->monitor_enable = 0;
	if (node->target_ratios != default_target_ratios) {
		kfree(node->target_ratios);
		node->target_ratios = default_target_ratios;
	}
	mutex_unlock(&node->mon_mutex_lock);
	df->data = node->orig_data;
	node->orig_data = NULL;
	hw->df = NULL;

}

static int devfreq_memlat_get_freq(struct devfreq *df,
					unsigned long *freq)
{
	int i, lat_dev = 0;
	struct memlat_node *node = df->data;
	struct memlat_hwmon *hw = node->hw;
	unsigned long max_freq = 0, possible_freq;
	unsigned int ratio, target_ratio_ceil;

	if (!mutex_trylock(&node->mon_mutex_lock)) {
		*freq = 0;
		return 0;
	}

	if (!node->mon_started) {
		*freq = 0;
		mutex_unlock(&node->mon_mutex_lock);/*lint !e455*/
		return 0;
	}

	hw->get_cnt(hw);
	mutex_unlock(&node->mon_mutex_lock);/*lint !e455*/

	for (i = 0; i < hw->num_cores; i++) { /*lint !e574*/
		ratio = hw->core_stats[i].inst_count;

		if (hw->core_stats[i].mem_count)
			ratio /= hw->core_stats[i].mem_count;

		trace_memlat_dev_meas(dev_name(df->dev.parent),
					hw->core_stats[i].id,
					hw->core_stats[i].inst_count,
					hw->core_stats[i].mem_count,
					hw->core_stats[i].freq, ratio);

		target_ratio_ceil = freq_to_targetratio(node, hw->core_stats[i].freq);
		if (ratio && ratio <= target_ratio_ceil
		    && hw->core_stats[i].freq > max_freq) {
			lat_dev = i;
			max_freq = hw->core_stats[i].freq;
		} else {

			/* slow path: find max_freeq accroding ratio */
			if (ratio && hw->core_stats[i].freq > max_freq) {
				possible_freq = get_max_possible_freq(node, ratio, hw->core_stats[i].freq);
				if (possible_freq > max_freq) {
					lat_dev = i;
					max_freq = possible_freq;
				}
			}
		}

	}

	if (max_freq) {
		max_freq = core_to_dev_freq(node, max_freq);
		trace_memlat_dev_update(dev_name(df->dev.parent),
					hw->core_stats[lat_dev].id,
					hw->core_stats[lat_dev].inst_count,
					hw->core_stats[lat_dev].mem_count,
					hw->core_stats[lat_dev].freq,
					max_freq);
	}

	*freq = max_freq;
	return 0;
}

static struct attribute *dev_attr[] = {
#ifdef CONFIG_HISI_DEBUG_FS
	&dev_attr_freq_map.attr,
	&dev_attr_node.attr,
#endif
	&dev_attr_target_ratio.attr,
	&dev_attr_monitor_enable.attr,
	NULL,
};

static struct attribute_group dev_attr_group = {
	.name = "mem_latency",
	.attrs = dev_attr,
};

#define MIN_MS	10U
#define MAX_MS	500U
static int devfreq_memlat_ev_handler(struct devfreq *df,
					unsigned int event, void *data)
{
	int ret;
	unsigned int sample_ms;

	switch (event) {
	case DEVFREQ_GOV_START:
		sample_ms = df->profile->polling_ms;
		sample_ms = max(MIN_MS, sample_ms);
		sample_ms = min(MAX_MS, sample_ms);
		df->profile->polling_ms = sample_ms;

		ret = gov_start(df);
		if (ret)
			return ret;

		dev_dbg(df->dev.parent,
			"Enabled Memory Latency governor\n");
		break;

	case DEVFREQ_GOV_STOP:
		gov_stop(df);

		dev_dbg(df->dev.parent,
			"Disabled Memory Latency governor\n");

		break;

	case DEVFREQ_GOV_INTERVAL:
		sample_ms = *(unsigned int *)data;
		sample_ms = max(MIN_MS, sample_ms);
		sample_ms = min(MAX_MS, sample_ms);

		mutex_lock(&df->lock);
		df->profile->polling_ms = sample_ms;
		mutex_unlock(&df->lock);
		break;
	}

	return 0;
}

static struct devfreq_governor devfreq_gov_memlat = {
	.name = "mem_latency",
	.get_target_freq = devfreq_memlat_get_freq,
	.event_handler = devfreq_memlat_ev_handler,
};

#define NUM_COLS	2
/*lint -e429*/
static struct core_dev_map *init_core_dev_map(struct device *dev,
		char *prop_name)
{
	int len, nf, i, j;
	u32 data;
	struct core_dev_map *tbl;
	int ret;

	if (!of_find_property(dev->of_node, prop_name, &len))
		return NULL;
	len /= sizeof(data); /*lint !e573*/

	if (len % NUM_COLS || len == 0)
		return NULL;
	nf = len / NUM_COLS;

	tbl = devm_kzalloc(dev, (nf + 1) * sizeof(struct core_dev_map),
			GFP_KERNEL);
	if (!tbl)
		return NULL;

	for (i = 0, j = 0; i < nf; i++, j += 2) {
		ret = of_property_read_u32_index(dev->of_node, prop_name, j,
				&data);
		if (ret)
			return NULL;
		tbl[i].core_mhz = data / 1000;

		ret = of_property_read_u32_index(dev->of_node, prop_name, j + 1,
				&data);
		if (ret)
			return NULL;
		tbl[i].target_freq = data;
		pr_debug("Entry%d CPU:%u, Dev:%u\n", i, tbl[i].core_mhz,
				tbl[i].target_freq);
	}
	tbl[i].core_mhz = 0;

	return tbl;
}

/*lint -e593*/
int register_memlat(struct device *dev, struct memlat_hwmon *hw)
{
	int ret = 0;
	u32 temp = 0;
	struct memlat_node *node;

	if (!hw->dev && !hw->of_node)
		return -EINVAL;

	node = devm_kzalloc(dev, sizeof(*node), GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	node->gov = &devfreq_gov_memlat;
	node->attr_grp = &dev_attr_group;

	node->monitor_enable = 0;
	node->hw = hw;

	hw->freq_map = init_core_dev_map(dev, "hisi,core-dev-table");
	if (!hw->freq_map) {
		dev_err(dev, "Couldn't find the core-dev freq table!\n");
		return -EINVAL;
	}

	node->idle_vote_enabled = of_property_read_bool(dev->of_node, "idle-vote-enabled");

	if (of_property_read_u32(dev->of_node, "hisi,switch-on-cpufreq", &temp)) {
		dev_err(dev, "[%s] node %s doesn't have hisi,switch-on-cpufreq property!\n",
				__func__, hw->of_node->name);
		return -EINVAL;
	}
	node->switch_on_cpufreq = temp;

	node->new_cpufreq = 0;
	node->monitor_paused = false;
	node->pending_change = false;
	node->governor_started = false;
	node->min_core_freq = INT_MAX;
	mutex_init(&node->cpufreq_mutex_lock);
	mutex_init(&node->mon_mutex_lock);
	spin_lock_init(&node->idle_notif_spinlock);
	spin_lock(&node->idle_notif_spinlock);
	node->prev_idle_freq = 0;
	spin_unlock(&node->idle_notif_spinlock);

	INIT_DELAYED_WORK(&node->work, cpufreq_trans_handler);
	node->perf_event_idle_nb.notifier_call = event_idle_notif;
	node->notifier_trans_block.notifier_call = cpufreq_notifier_trans;
	if (node->idle_vote_enabled)
		cpu_pm_register_notifier(&node->perf_event_idle_nb);

	cpufreq_register_notifier(&node->notifier_trans_block,
					CPUFREQ_TRANSITION_NOTIFIER);

	mutex_lock(&list_lock);
	list_add_tail(&node->list, &memlat_list);
	mutex_unlock(&list_lock);


	mutex_lock(&state_lock);
	if (!use_cnt) {
		ret = devfreq_add_governor(&devfreq_gov_memlat);
		if (!ret) {
			use_cnt++;
		}
	}
	mutex_unlock(&state_lock);

	if (!ret) {
		dev_info(dev, "Memory Latency governor registered.\n");
	} else {
		/* if register fail, then remove the notifications */
		if (node->idle_vote_enabled)
			cpu_pm_unregister_notifier(&node->perf_event_idle_nb);

		cpufreq_unregister_notifier(&node->notifier_trans_block,
						CPUFREQ_TRANSITION_NOTIFIER);
		dev_err(dev, "Memory Latency governor registration failed!\n");
	}

	return ret;
}
/*lint +e593*/
/*lint +e429*/

MODULE_DESCRIPTION("HW monitor based dev DDR bandwidth voting driver");
MODULE_LICENSE("GPL v2");

