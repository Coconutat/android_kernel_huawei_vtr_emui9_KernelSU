/* Copyright (c) 2014-2017, The Linux Foundation. All rights reserved.
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

#define pr_fmt(fmt)	"core_ctl: " fmt

#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/topology.h>
#include <linux/cpufreq.h>
#include <linux/kthread.h>
#include <linux/percpu.h>
#include <linux/math64.h>
#include <linux/version.h>
#include "sched.h"
#include "walt.h"
#include <linux/sched/rt.h>
#include <linux/capability.h>
#include <linux/hisi/hisi_core_ctl.h>

#include <trace/events/sched.h>

#define NR_AVG_AMP	128

struct cluster_data {
	bool inited;
	bool pending;
	bool enable;
	bool spread_affinity;
	unsigned int min_cpus;
	unsigned int max_cpus;
	unsigned int offline_delay_ms;
	unsigned int busy_thres;
	unsigned int idle_thres;
	unsigned int active_cpus;
	unsigned int num_cpus;
	unsigned int nr_isolated_cpus;
	unsigned int need_cpus;
	unsigned int task_thres;
	unsigned int open_thres;
	unsigned int close_thres;
	unsigned int first_cpu;
	unsigned int boost;
	unsigned int capacity;
	unsigned int nrrun;
	unsigned int last_nrrun;
	unsigned int max_nrrun;
	unsigned int last_max_nrrun;
	cpumask_t cpu_mask;
	s64 need_ts;
	spinlock_t pending_lock;
	struct task_struct *core_ctl_thread;
	struct list_head lru;
	struct list_head cluster_node;
	struct kobject kobj;
};

struct cpu_data {
	bool is_busy;
	bool not_preferred;
	bool isolated_by_us;
	int cpu;
	unsigned int load;
	u64 isolate_ts;
	u64 isolate_cnt;
	u64 isolated_time;
	struct cluster_data *cluster;
	struct list_head sib;
};

struct sched_nr_stat {
	unsigned int avg;
	unsigned int iowait_avg;
	unsigned int big_avg;
	unsigned int nr_max;
};

static DEFINE_PER_CPU(u64, nr_prod_sum);
static DEFINE_PER_CPU(u64, nr_big_prod_sum);
static DEFINE_PER_CPU(u64, iowait_prod_sum);
static DEFINE_PER_CPU(u64, last_time);
static DEFINE_PER_CPU(u64, last_get_time);
static DEFINE_PER_CPU(u64, nr);
static DEFINE_PER_CPU(u64, nr_max);
static DEFINE_PER_CPU(spinlock_t, nr_lock) = __SPIN_LOCK_UNLOCKED(nr_lock);

static DEFINE_PER_CPU(struct cpu_data, cpu_state);
static DEFINE_SPINLOCK(state_lock);
static LIST_HEAD(cluster_list);
static bool initialized;
static unsigned int rq_avg_update_interval_ms = 20;
static s64 rq_avg_update_timestamp_ms;

static void apply_need(struct cluster_data *state, bool bypass_time_limit);
static void wake_up_core_ctl_thread(struct cluster_data *state);
static unsigned int get_active_cpu_count(const struct cluster_data *cluster);
static void update_isolated_time(struct cpu_data *cpu);
static void __core_ctl_set_boost(struct cluster_data *cluster);


/* ========================= sysfs interface =========================== */

static ssize_t store_min_cpus(struct cluster_data *state,
			      const char *buf, size_t count)
{
	unsigned int val;
	unsigned long flags;

	if (sscanf(buf, "%u\n", &val) != 1)/* unsafe_function_ignore: sscanf */
		return -EINVAL;

	spin_lock_irqsave(&state_lock, flags);
	state->min_cpus = min(val, state->max_cpus);
	spin_unlock_irqrestore(&state_lock, flags);

	apply_need(state, false);

	return count;
}

static ssize_t show_min_cpus(const struct cluster_data *state, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", state->min_cpus);/* unsafe_function_ignore: scnprintf */
}

static ssize_t store_max_cpus(struct cluster_data *state,
			      const char *buf, size_t count)
{
	unsigned int val;
	unsigned long flags;


	if (sscanf(buf, "%u\n", &val) != 1)/* unsafe_function_ignore: sscanf */
		return -EINVAL;

	spin_lock_irqsave(&state_lock, flags);
	val = min(val, state->num_cpus);
	state->max_cpus = val;
	state->min_cpus = min(state->min_cpus, state->max_cpus);
	spin_unlock_irqrestore(&state_lock, flags);

	apply_need(state, true);

	return count;
}

static ssize_t show_max_cpus(const struct cluster_data *state, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", state->max_cpus);/* unsafe_function_ignore: scnprintf */
}

static ssize_t store_offline_delay_ms(struct cluster_data *state,
				      const char *buf, size_t count)
{
	unsigned int val;
	unsigned long flags;

	if (sscanf(buf, "%u\n", &val) != 1)/* unsafe_function_ignore: sscanf */
		return -EINVAL;

	spin_lock_irqsave(&state_lock, flags);
	state->offline_delay_ms = val;
	spin_unlock_irqrestore(&state_lock, flags);

	apply_need(state, false);

	return count;
}

static ssize_t show_offline_delay_ms(const struct cluster_data *state,
				     char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", state->offline_delay_ms);/* unsafe_function_ignore: scnprintf */
}

static ssize_t store_task_thres(struct cluster_data *state,
				const char *buf, size_t count)
{
	unsigned int val;
	unsigned long flags;

	if (sscanf(buf, "%u\n", &val) != 1)/* unsafe_function_ignore: sscanf */
		return -EINVAL;

	if (val < state->num_cpus)
		return -EINVAL;

	spin_lock_irqsave(&state_lock, flags);
	state->task_thres = val;
	spin_unlock_irqrestore(&state_lock, flags);

	apply_need(state, false);

	return count;
}

static ssize_t show_task_thres(const struct cluster_data *state, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", state->task_thres);/* unsafe_function_ignore: scnprintf */
}

static ssize_t store_busy_thres(struct cluster_data *state,
				const char *buf, size_t count)
{
	unsigned int val;
	unsigned long flags;

	if (sscanf(buf, "%u\n", &val) != 1)/* unsafe_function_ignore: sscanf */
		return -EINVAL;

	spin_lock_irqsave(&state_lock, flags);
	state->busy_thres = val;
	spin_unlock_irqrestore(&state_lock, flags);

	apply_need(state, false);

	return count;
}

static ssize_t show_busy_thres(const struct cluster_data *state, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", state->busy_thres);/* unsafe_function_ignore: scnprintf */
}

static ssize_t store_idle_thres(struct cluster_data *state,
				const char *buf, size_t count)
{
	unsigned int val;
	unsigned long flags;

	if (sscanf(buf, "%u\n", &val) != 1)/* unsafe_function_ignore: sscanf */
		return -EINVAL;

	spin_lock_irqsave(&state_lock, flags);
	state->idle_thres = val;
	spin_unlock_irqrestore(&state_lock, flags);

	apply_need(state, false);

	return count;
}

static ssize_t show_idle_thres(const struct cluster_data *state, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", state->idle_thres);/* unsafe_function_ignore: scnprintf */
}

static ssize_t store_open_thres(struct cluster_data *state,
				const char *buf, size_t count)
{
	unsigned int val;
	unsigned long flags;

	if (sscanf(buf, "%u\n", &val) != 1)/* unsafe_function_ignore: sscanf */
		return -EINVAL;

	spin_lock_irqsave(&state_lock, flags);
	state->open_thres = min(val, state->num_cpus * NR_AVG_AMP);
	spin_unlock_irqrestore(&state_lock, flags);

	apply_need(state, false);

	return count;
}

static ssize_t show_open_thres(const struct cluster_data *state,
			       char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", state->open_thres);/* unsafe_function_ignore: scnprintf */
}

static ssize_t store_close_thres(struct cluster_data *state,
				const char *buf, size_t count)
{
	unsigned int val;
	unsigned long flags;

	if (sscanf(buf, "%u\n", &val) != 1)/* unsafe_function_ignore: sscanf */
		return -EINVAL;

	spin_lock_irqsave(&state_lock, flags);
	state->close_thres = min(val, state->num_cpus * NR_AVG_AMP);
	spin_unlock_irqrestore(&state_lock, flags);

	apply_need(state, false);

	return count;
}

static ssize_t show_close_thres(const struct cluster_data *state,
			       char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", state->close_thres);/* unsafe_function_ignore: scnprintf */
}


#ifdef CONFIG_HISI_DEBUG_FS
static ssize_t show_need_cpus(const struct cluster_data *state, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", state->need_cpus);/* unsafe_function_ignore: scnprintf */
}

static ssize_t show_active_cpus(const struct cluster_data *state, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", state->active_cpus);/* unsafe_function_ignore: scnprintf */
}

static ssize_t show_global_state(const struct cluster_data *state, char *buf)
{
	struct cpu_data *c;
	struct cluster_data *cluster;
	ssize_t count = 0;
	int cpu;
	unsigned long flags;

	spin_lock_irqsave(&state_lock, flags);
	for_each_possible_cpu(cpu) {
		c = &per_cpu(cpu_state, cpu);
		cluster = c->cluster;
		if (!cluster)
			continue;

		if (!cluster->inited)
			continue;

		update_isolated_time(c);

		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "CPU%u\n", cpu);/* unsafe_function_ignore: scnprintf */
		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tOnline: %u\n",
				  cpu_online(cpu));/* unsafe_function_ignore: scnprintf */
		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tIsolated: %u\n",
				  cpu_isolated(cpu));/* unsafe_function_ignore: scnprintf */
		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tIsolate cnt: %llu\n",
				  c->isolate_cnt);/* unsafe_function_ignore: scnprintf */
		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tIsolated time: %llu\n",
				  c->isolated_time);/* unsafe_function_ignore: scnprintf */
		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tFirst CPU: %u\n",
				  cluster->first_cpu);/* unsafe_function_ignore: scnprintf */
		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tLoad%%: %u\n",
				  c->load);/* unsafe_function_ignore: scnprintf */
		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tIs busy: %u\n",
				  c->is_busy);/* unsafe_function_ignore: scnprintf */
		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tNot preferred: %u\n",
				  c->not_preferred);/* unsafe_function_ignore: scnprintf */

		if (cpu != cluster->first_cpu)
			continue;

		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tNr running: %u\n",
				  cluster->nrrun);/* unsafe_function_ignore: scnprintf */
		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tMax nr running: %u\n",
				  cluster->max_nrrun);/* unsafe_function_ignore: scnprintf */
		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tActive CPUs: %u\n",
				  get_active_cpu_count(cluster));/* unsafe_function_ignore: scnprintf */
		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tNeed CPUs: %u\n",
				  cluster->need_cpus);/* unsafe_function_ignore: scnprintf */
		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tNr isolated CPUs: %u\n",
				  cluster->nr_isolated_cpus);/* unsafe_function_ignore: scnprintf */
		count += scnprintf(buf + count, PAGE_SIZE - count,
				  "\tBoost: %u\n",
				  cluster->boost);/* unsafe_function_ignore: scnprintf */
	}
	spin_unlock_irqrestore(&state_lock, flags);

	return count;
}
#endif

static ssize_t store_not_preferred(struct cluster_data *state,
				   const char *buf, size_t count)
{
	struct cpu_data *c;
	int cpu;
	unsigned int val;
	unsigned long flags;
	int ret;

	ret = sscanf(buf, "%d %u\n", &cpu, &val);/* unsafe_function_ignore: sscanf */
	if (ret != 2 || cpu >= nr_cpu_ids)
		return -EINVAL;

	spin_lock_irqsave(&state_lock, flags);
	if (cpumask_test_cpu(cpu, &state->cpu_mask)) {
		c = &per_cpu(cpu_state, cpu);
		c->not_preferred = !!val;
	}
	spin_unlock_irqrestore(&state_lock, flags);

	return count;
}

static ssize_t show_not_preferred(const struct cluster_data *state, char *buf)
{
	struct cpu_data *c;
	ssize_t count = 0;
	unsigned long flags;
	int cpu;

	spin_lock_irqsave(&state_lock, flags);
	for_each_cpu(cpu, &state->cpu_mask) {
		c = &per_cpu(cpu_state, cpu);
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "CPU#%d: %u\n", cpu, c->not_preferred);
	}
	spin_unlock_irqrestore(&state_lock, flags);

	return count;
}

static ssize_t store_update_interval_ms(struct cluster_data *state,
					const char *buf, size_t count)
{
	unsigned int val;
	unsigned long flags;

	if (sscanf(buf, "%u\n", &val) != 1)/* unsafe_function_ignore: sscanf */
		return -EINVAL;

	spin_lock_irqsave(&state_lock, flags);
	rq_avg_update_interval_ms =
		clamp(val, 10U, 1000U);/*lint !e666*/
	spin_unlock_irqrestore(&state_lock, flags);

	return count;
}

static ssize_t show_update_interval_ms(const struct cluster_data *state,
				       char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", rq_avg_update_interval_ms);/* unsafe_function_ignore: scnprintf */
}

static ssize_t store_boost(struct cluster_data *state,
			   const char *buf, size_t count)
{
	unsigned int val;

	if (sscanf(buf, "%u\n", &val) != 1)/* unsafe_function_ignore: sscanf */
		return -EINVAL;

	if (1 == val)
		__core_ctl_set_boost(state);

	return count;
}

static ssize_t show_boost(const struct cluster_data *state,
			  char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", state->boost);/* unsafe_function_ignore: scnprintf */
}

static ssize_t store_enable(struct cluster_data *state,
			    const char *buf, size_t count)
{
	unsigned int val;
	bool enable;

	if (sscanf(buf, "%u\n", &val) != 1)/* unsafe_function_ignore: sscanf */
		return -EINVAL;

	enable = !!val;
	if (enable != state->enable) {
		state->enable = enable;
		apply_need(state, false);
	}

	return count;
}

static ssize_t show_enable(const struct cluster_data *state, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", state->enable);/* unsafe_function_ignore: scnprintf */
}

static ssize_t store_spread_affinity(struct cluster_data *state,
				     const char *buf, size_t count)
{
	unsigned int val;

	if (sscanf(buf, "%u\n", &val) != 1)/* unsafe_function_ignore: sscanf */
		return -EINVAL;

	state->spread_affinity = !!val;

	return count;
}

static ssize_t show_spread_affinity(const struct cluster_data *state, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", state->spread_affinity);/* unsafe_function_ignore: scnprintf */
}


struct core_ctl_attr {
	struct attribute attr;
	ssize_t (*show)(const struct cluster_data *, char *);
	ssize_t (*store)(struct cluster_data *, const char *, size_t count);
};

#define core_ctl_attr_ro(_name)		\
static struct core_ctl_attr _name =	\
__ATTR(_name, 0440, show_##_name, NULL)

#define core_ctl_attr_rw(_name)			\
static struct core_ctl_attr _name =		\
__ATTR(_name, 0640, show_##_name, store_##_name)

core_ctl_attr_rw(min_cpus);
core_ctl_attr_rw(max_cpus);
core_ctl_attr_rw(offline_delay_ms);
core_ctl_attr_rw(busy_thres);
core_ctl_attr_rw(idle_thres);
core_ctl_attr_rw(task_thres);
core_ctl_attr_rw(open_thres);
core_ctl_attr_rw(close_thres);
#ifdef CONFIG_HISI_DEBUG_FS
core_ctl_attr_ro(need_cpus);
core_ctl_attr_ro(active_cpus);
core_ctl_attr_ro(global_state);
#endif
core_ctl_attr_rw(not_preferred);
core_ctl_attr_rw(update_interval_ms);
core_ctl_attr_rw(boost);
core_ctl_attr_rw(enable);
core_ctl_attr_rw(spread_affinity);

static struct attribute *default_attrs[] = {
	&min_cpus.attr,
	&max_cpus.attr,
	&offline_delay_ms.attr,
	&busy_thres.attr,
	&idle_thres.attr,
	&task_thres.attr,
	&open_thres.attr,
	&close_thres.attr,
#ifdef CONFIG_HISI_DEBUG_FS
	&need_cpus.attr,
	&active_cpus.attr,
	&global_state.attr,
#endif
	&not_preferred.attr,
	&update_interval_ms.attr,
	&boost.attr,
	&enable.attr,
	&spread_affinity.attr,
	NULL
};

#define to_cluster_data(k) container_of(k, struct cluster_data, kobj)
#define to_attr(a) container_of(a, struct core_ctl_attr, attr)
static ssize_t show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct cluster_data *data = to_cluster_data(kobj);
	struct core_ctl_attr *cattr = to_attr(attr);
	ssize_t ret = -EIO;

	if (cattr->show)
		ret = cattr->show(data, buf);

	return ret;
}

static ssize_t store(struct kobject *kobj, struct attribute *attr,
		     const char *buf, size_t count)
{
	struct cluster_data *data = to_cluster_data(kobj);
	struct core_ctl_attr *cattr = to_attr(attr);
	ssize_t ret = -EIO;

	if (cattr->store)
		ret = cattr->store(data, buf, count);

	return ret;
}

static const struct sysfs_ops sysfs_ops = {
	.show	= show,
	.store	= store,
};

static struct kobj_type ktype_core_ctl = {
	.sysfs_ops	= &sysfs_ops,
	.default_attrs	= default_attrs,
};

/* ==================== runqueue based core count =================== */

/**
 * core_ctl_update_nr_prod
 * @updated_rq: The updated runqueue.
 * @delta: Adjust nr by 'delta' amount
 * @inc: Whether we are increasing or decreasing the count
 * @return: N/A
 *
 * Update average with latest nr_running value for CPU
 */
void core_ctl_update_nr_prod(struct rq *updated_rq)
{
	int cpu = cpu_of(updated_rq);
	u64 diff;
	u64 curr_time;
	unsigned long flags, prev_nr_running;

	spin_lock_irqsave(&per_cpu(nr_lock, cpu), flags);

	prev_nr_running = per_cpu(nr, cpu);
	per_cpu(nr, cpu) = updated_rq->nr_running;
	curr_time = sched_clock();
	diff = curr_time - per_cpu(last_time, cpu);
	per_cpu(last_time, cpu) = curr_time;

	if (per_cpu(nr, cpu) > per_cpu(nr_max, cpu))
		per_cpu(nr_max, cpu) = per_cpu(nr, cpu);

	if ((s64)diff > 0) {
		per_cpu(nr_prod_sum, cpu) += prev_nr_running * diff;
		per_cpu(nr_big_prod_sum, cpu) +=
				updated_rq->nr_heavy_running * diff;
		per_cpu(iowait_prod_sum, cpu) += nr_iowait_cpu(cpu) * diff;
	}

	spin_unlock_irqrestore(&per_cpu(nr_lock, cpu), flags);
}
EXPORT_SYMBOL(core_ctl_update_nr_prod);

/**
 * get_nr_running_avg
 * @return: Average nr_running, iowait and nr_big_tasks value since last poll.
 *	    Returns the avg * NR_AVG_AMP to return up to two decimal points
 *	    of accuracy.
 *
 * Obtains the average nr_running value since the last poll.
 * This function may not be called concurrently with itself
 */
static void get_nr_running_avg(struct cluster_data *cluster,
			struct sched_nr_stat *stat)
{
	int cpu, time_cpu = cpumask_first(&cluster->cpu_mask);
	u64 curr_time = sched_clock();
	u64 diff = curr_time - per_cpu(last_get_time, time_cpu);
	u64 tmp_avg = 0, tmp_iowait = 0, tmp_big_avg = 0;
	unsigned int tmp_nr_max = 0;

	stat->avg = 0;
	stat->iowait_avg = 0;
	stat->big_avg = 0;
	stat->nr_max = 0;

	/* read and reset nr_running counts */
	for_each_cpu(cpu, &cluster->cpu_mask) {
		unsigned long flags;

		spin_lock_irqsave(&per_cpu(nr_lock, cpu), flags);
		curr_time = sched_clock();
		diff = curr_time - per_cpu(last_time, cpu);
		if ((s64)diff < 0)
			diff = 0;

		tmp_avg += per_cpu(nr_prod_sum, cpu);
		tmp_avg += per_cpu(nr, cpu) * diff;

		tmp_big_avg += per_cpu(nr_big_prod_sum, cpu);
		tmp_big_avg += cpu_rq(cpu)->nr_heavy_running * diff;

		tmp_iowait += per_cpu(iowait_prod_sum, cpu);
		tmp_iowait +=  nr_iowait_cpu(cpu) * diff;

		per_cpu(last_time, cpu) = curr_time;

		per_cpu(nr_prod_sum, cpu) = 0;
		per_cpu(nr_big_prod_sum, cpu) = 0;
		per_cpu(iowait_prod_sum, cpu) = 0;

		if (tmp_nr_max < per_cpu(nr_max, cpu))
			tmp_nr_max = per_cpu(nr_max, cpu);

		per_cpu(nr_max, cpu) = per_cpu(nr, cpu);

		spin_unlock_irqrestore(&per_cpu(nr_lock, cpu), flags);
	}

	diff = curr_time - per_cpu(last_get_time, time_cpu);

	for_each_cpu(cpu, &cluster->cpu_mask)
		per_cpu(last_get_time, cpu) = curr_time;

	/* do not take the nrrun into account when the cluster is isolated */
	if (!diff || 0 == cluster->active_cpus)
		return;

	stat->avg = (unsigned int)div64_u64(tmp_avg * NR_AVG_AMP, diff);
	stat->big_avg = (unsigned int)div64_u64(tmp_big_avg * NR_AVG_AMP, diff);
	stat->iowait_avg = (unsigned int)div64_u64(tmp_iowait * NR_AVG_AMP, diff);
	stat->nr_max = tmp_nr_max;

	trace_core_ctl_get_nr_running_avg(&cluster->cpu_mask,
					  stat->avg, stat->big_avg,
					  stat->iowait_avg, stat->nr_max);
}

static int update_running_avg(void)
{
	struct sched_nr_stat stat;
	unsigned int overflow_nrrun, overall_nrrun = 0;
	s64 now;
	unsigned long flags;
	struct cluster_data *cluster, *temp;

	spin_lock_irqsave(&state_lock, flags);

	now = ktime_to_ms(ktime_get());
	if (now < rq_avg_update_timestamp_ms + rq_avg_update_interval_ms) {
		spin_unlock_irqrestore(&state_lock, flags);
		return -1;
	}
	rq_avg_update_timestamp_ms = now;

	list_for_each_entry(cluster, &cluster_list, cluster_node) {
		overflow_nrrun = 0;
		cluster->last_nrrun = cluster->nrrun;
		cluster->last_max_nrrun = cluster->max_nrrun;
		get_nr_running_avg(cluster, &stat);
		overall_nrrun += stat.avg;

		cluster->nrrun = overall_nrrun;
		cluster->max_nrrun = stat.nr_max;
		/*
		 * Big cluster only need to take care of big tasks, but if
		 * there are not enough big cores, big tasks need to be run
		 * on little as well. Thus for little's runqueue stat, it
		 * has to use overall runqueue average, or derive what big
		 * tasks would have to be run on little. The latter approach
		 * is not easy to get given core control reacts much slower
		 * than scheduler, and can't predict scheduler's behavior.
		 */
		if (cluster->cluster_node.prev != &cluster_list) {
			temp = list_prev_entry(cluster, cluster_node);

			/* this cluster is overload,
			 * need share load to a bigger cluster
			 */
			if (stat.avg > cluster->num_cpus * NR_AVG_AMP) {
				overflow_nrrun = stat.avg -
						 (cluster->num_cpus * NR_AVG_AMP);
				if (overflow_nrrun > stat.big_avg)
					stat.big_avg = overflow_nrrun;
			}

			stat.big_avg = min(stat.big_avg, temp->num_cpus * NR_AVG_AMP);

			/* prevent unisolate a cluster for little pulse load */
			if (temp->active_cpus > 0 ||
			    stat.big_avg > temp->open_thres)
				temp->nrrun += stat.big_avg;

			if (temp->nrrun < temp->close_thres)
				temp->nrrun = 0;

			temp->nrrun = DIV_ROUND_UP(temp->nrrun, NR_AVG_AMP);
		}
	}

	if (!list_empty(&cluster_list)) {
		temp = list_last_entry(&cluster_list,
				       struct cluster_data, cluster_node);
		temp->nrrun = DIV_ROUND_UP(temp->nrrun, NR_AVG_AMP);
	}

	spin_unlock_irqrestore(&state_lock, flags);

	return 0;
}

int update_misfit_task(void)
{
	struct cluster_data *cluster, *temp;
	unsigned int misfits;
	int cpu, ret = -1;

	list_for_each_entry(cluster, &cluster_list, cluster_node) {
		misfits = 0;
		for_each_cpu(cpu, &cluster->cpu_mask) {
			misfits += !!cpu_rq(cpu)->misfit_task;/*lint !e514*/
		}

		if (cluster->cluster_node.prev != &cluster_list) {
			temp = list_prev_entry(cluster, cluster_node);
			if (temp->nrrun < misfits) {
				temp->nrrun = misfits;
				ret = 0;
			}
		}
	}

	return ret;
}

/* adjust needed CPUs based on current runqueue information */
static unsigned int apply_task_need(const struct cluster_data *cluster,
				    unsigned int new_need)
{
	/* only unisolate more cores if there are tasks to run */
	if (cluster->nrrun > new_need)
		new_need = cluster->nrrun;
	else if (cluster->nrrun == new_need && new_need)
		return new_need + 1;

	if (cluster->max_nrrun > cluster->task_thres)
		new_need++;

	return new_need;
}

/* ======================= load based core count  ====================== */

static unsigned int apply_limits(const struct cluster_data *cluster,
				 unsigned int new_cpus)
{
	return clamp(new_cpus, cluster->min_cpus, cluster->max_cpus);/*lint !e666*/
}

static unsigned int get_active_cpu_count(const struct cluster_data *cluster)
{
	return cluster->num_cpus -
				sched_isolate_count(&cluster->cpu_mask, true);
}

static bool is_active(const struct cpu_data *state)
{
	return cpu_online(state->cpu) && !cpu_isolated(state->cpu);
}

static bool adjustment_possible(const struct cluster_data *cluster,
							unsigned int need)
{
	return (need < cluster->active_cpus || (need > cluster->active_cpus &&
						cluster->nr_isolated_cpus));
}

static bool eval_need(struct cluster_data *cluster, bool bypass_time_limit)
{
	unsigned long flags;
	struct cpu_data *c;
	unsigned int busy_cpus = 0, last_need;
	int ret = 0;
	bool need_flag = false;
	unsigned int new_need;
	s64 now, elapsed = 0;

	if (unlikely(!cluster->inited))
		return 0;

	spin_lock_irqsave(&state_lock, flags);

	cluster->active_cpus = get_active_cpu_count(cluster);

	if (cluster->boost || !cluster->enable) {
		new_need = cluster->max_cpus;
		cluster->boost = 0;
	} else {
		list_for_each_entry(c, &cluster->lru, sib) {
			if (walt_cpu_high_irqload(c->cpu) ||
			    c->load >= cluster->busy_thres)
				c->is_busy = true;
			else if (c->load < cluster->idle_thres)
				c->is_busy = false;
			busy_cpus += c->is_busy;
		}
		new_need = apply_task_need(cluster, busy_cpus);
	}
	new_need = apply_limits(cluster, new_need);
	need_flag = adjustment_possible(cluster, new_need);

	last_need = cluster->need_cpus;
	now = ktime_to_ms(ktime_get());

	if (new_need >= cluster->active_cpus || bypass_time_limit) {
		ret = 1;
	} else {
		if (new_need == last_need) {
			cluster->need_ts = now;
			spin_unlock_irqrestore(&state_lock, flags);
			return 0;
		}

		elapsed = now - cluster->need_ts;
		ret = elapsed >= cluster->offline_delay_ms;
	}

	if (ret) {
		cluster->need_ts = now;
		cluster->need_cpus = new_need;
	}

	trace_core_ctl_eval_need(&cluster->cpu_mask,
				 cluster->nrrun, busy_cpus,
				 cluster->active_cpus, last_need,
				 new_need, elapsed, ret && need_flag);

	spin_unlock_irqrestore(&state_lock, flags);

	return ret && need_flag;
}

static void apply_need(struct cluster_data *cluster, bool bypass_time_limit)
{
	if (eval_need(cluster, bypass_time_limit))
		wake_up_core_ctl_thread(cluster);
}

static void core_ctl_update_busy(int cpu, unsigned int load, bool check_load)
{
	struct cpu_data *c = &per_cpu(cpu_state, cpu);
	struct cluster_data *cluster = c->cluster;
	bool old_is_busy = c->is_busy;
	unsigned int old_load;
	int ret;

	if (!cluster)
		return;

	if (!cluster->inited)
		return;

	old_load = c->load;
	c->load = load;

	if (!check_load &&
	    cpumask_next(cpu, &cluster->cpu_mask) < nr_cpu_ids)/*lint !e574*/
		return;

	ret = update_running_avg();

	if (check_load && old_load == load && ret)
		return;

	apply_need(cluster, false);

	trace_core_ctl_update_busy(cpu, load, old_is_busy, c->is_busy);
}

/* ========================= core count enforcement ==================== */

static void wake_up_core_ctl_thread(struct cluster_data *cluster)
{
	unsigned long flags;

	spin_lock_irqsave(&cluster->pending_lock, flags);
	cluster->pending = true;
	spin_unlock_irqrestore(&cluster->pending_lock, flags);

	wake_up_process(cluster->core_ctl_thread);
}

static void __core_ctl_set_boost(struct cluster_data *cluster)
{
	unsigned long flags;

	if (unlikely(!initialized))
		return;

	spin_lock_irqsave(&state_lock, flags);

	if (cluster == NULL)
		cluster = list_first_entry(&cluster_list,
					  struct cluster_data, cluster_node);
	cluster->boost = 1;

	spin_unlock_irqrestore(&state_lock, flags);

	apply_need(cluster, false);

	trace_core_ctl_set_boost(cluster->boost);
}

void core_ctl_set_boost(void)
{
	__core_ctl_set_boost(NULL);
}
EXPORT_SYMBOL(core_ctl_set_boost);

void core_ctl_spread_affinity(cpumask_t *allowed_mask)
{
	struct cluster_data *cluster;

	if (unlikely(!initialized || !allowed_mask))
		return;

	if (cpumask_empty(allowed_mask))
		return;

	if (capable(CAP_SYS_ADMIN))
		return;

	cluster = list_first_entry(&cluster_list,
				   struct cluster_data, cluster_node);

	if (cluster->enable &&
	    cluster->spread_affinity &&
	    cpumask_subset(allowed_mask, &cluster->cpu_mask) &&
	    cluster->cluster_node.next != &cluster_list) {
		cluster = list_next_entry(cluster, cluster_node);
		cpumask_or(allowed_mask, allowed_mask, &cluster->cpu_mask);
	}
}

void core_ctl_check(void)
{
	struct cluster_data *cluster;

	if (unlikely(!initialized))
		return;

	if (!update_running_avg() || !update_misfit_task()) {
		list_for_each_entry(cluster, &cluster_list, cluster_node) {
			apply_need(cluster, false);
		}
	}
}

static void move_cpu_lru(struct cpu_data *state, bool forward)
{
	list_del(&state->sib);
	if (forward)
		list_add_tail(&state->sib, &state->cluster->lru);
	else
		list_add(&state->sib, &state->cluster->lru);
}

static void __try_to_isolate(struct cluster_data *cluster,
			     unsigned int need, bool isolate_busy)
{
	struct cpu_data *c, *tmp;
	unsigned long flags;
	unsigned int num_cpus = cluster->num_cpus;
	unsigned int nr_isolated = 0;
	int ret;

	/*
	 * Protect against entry being removed (and added at tail) by other
	 * thread (hotplug).
	 */
	spin_lock_irqsave(&state_lock, flags);

	list_for_each_entry_safe(c, tmp, &cluster->lru, sib) {
		if (!num_cpus--)
			break;

		if (!is_active(c))
			continue;

		if (isolate_busy) {
			if (cluster->active_cpus <= cluster->max_cpus)
				break;
		} else {
			if (cluster->active_cpus == need)
				break;

			if (c->is_busy)
				continue;
		}

		pr_debug("Trying to isolate CPU%d\n", c->cpu);

		spin_unlock_irqrestore(&state_lock, flags);
		ret = sched_isolate_cpu_unlocked(c->cpu);
		spin_lock_irqsave(&state_lock, flags);

		if (!ret) {
			c->isolated_by_us = true;
			c->isolate_cnt++;
			c->isolate_ts = ktime_to_ms(ktime_get());
			move_cpu_lru(c, true);
			nr_isolated++;
		} else {
			pr_debug("Unable to isolate CPU%d\n", c->cpu);
		}
		cluster->active_cpus = get_active_cpu_count(cluster);
	}

	cluster->nr_isolated_cpus += nr_isolated;

	spin_unlock_irqrestore(&state_lock, flags);
}

static void try_to_isolate(struct cluster_data *cluster, unsigned int need)
{
	__try_to_isolate(cluster, need, false);
	/*
	 * If the number of active CPUs is within the limits, then
	 * don't force isolation of any busy CPUs.
	 */
	if (cluster->active_cpus <= cluster->max_cpus)
		return;

	__try_to_isolate(cluster, need, true);
}

static void update_isolated_time(struct cpu_data *cpu)
{
	u64 ts;

	if (!cpu->isolated_by_us)
		return;

	ts = ktime_to_ms(ktime_get());

	cpu->isolated_time += ts - cpu->isolate_ts;

	cpu->isolate_ts = ts;
}

static void __try_to_unisolate(struct cluster_data *cluster,
			       unsigned int need, bool force)
{
	struct cpu_data *c, *tmp;
	unsigned long flags;
	unsigned int num_cpus = cluster->num_cpus;
	unsigned int nr_unisolated = 0;
	int ret;

	/*
	 * Protect against entry being removed (and added at tail) by other
	 * thread (hotplug).
	 */
	spin_lock_irqsave(&state_lock, flags);

	list_for_each_entry_safe_reverse(c, tmp, &cluster->lru, sib) {
		if (!num_cpus--)
			break;

		if (!c->isolated_by_us)
			continue;
		if ((cpu_online(c->cpu) && !cpu_isolated(c->cpu)) ||
			(!force && c->not_preferred))
			continue;
		if (cluster->active_cpus == need)
			break;

		pr_debug("Trying to unisolate CPU%d\n", c->cpu);

		spin_unlock_irqrestore(&state_lock, flags);
		ret = sched_unisolate_cpu_unlocked(c->cpu, false);
		spin_lock_irqsave(&state_lock, flags);

		if (!ret) {
			update_isolated_time(c);
			c->isolated_by_us = false;
			move_cpu_lru(c, false);
			nr_unisolated++;
		} else {
			pr_debug("Unable to unisolate CPU%d\n", c->cpu);
		}
		cluster->active_cpus = get_active_cpu_count(cluster);
	}

	cluster->nr_isolated_cpus -= nr_unisolated;

	spin_unlock_irqrestore(&state_lock, flags);
}

static void try_to_unisolate(struct cluster_data *cluster, unsigned int need)
{
	__try_to_unisolate(cluster, need, false);

	if (cluster->active_cpus == need)
		return;

	__try_to_unisolate(cluster, need, true);
}

static void __ref do_core_ctl(struct cluster_data *cluster)
{
	unsigned int need;

	need = apply_limits(cluster, cluster->need_cpus);

	if (adjustment_possible(cluster, need)) {
		pr_debug("Trying to adjust group %u from %u to %u\n",
			 cluster->first_cpu, cluster->active_cpus, need);

		cpu_maps_update_begin();

		if (cluster->active_cpus > need)
			try_to_isolate(cluster, need);
		else if (cluster->active_cpus < need)
			try_to_unisolate(cluster, need);

		cpu_maps_update_done();
	}
}

static int __ref try_core_ctl(void *data)
{
	struct cluster_data *cluster = data;
	unsigned long flags;

	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);/*lint !e446 !e666*/
		spin_lock_irqsave(&cluster->pending_lock, flags);
		if (!cluster->pending) {
			spin_unlock_irqrestore(&cluster->pending_lock, flags);
			schedule();
			if (kthread_should_stop())
				break;
			spin_lock_irqsave(&cluster->pending_lock, flags);
		}
		set_current_state(TASK_RUNNING);/*lint !e446 !e666*/
		cluster->pending = false;
		spin_unlock_irqrestore(&cluster->pending_lock, flags);

		do_core_ctl(cluster);
	}

	return 0;
}

static int __ref cpuhp_core_ctl_online(unsigned int cpu)
{
	struct cpu_data *state = &per_cpu(cpu_state, cpu);
	struct cluster_data *cluster = state->cluster;
	unsigned long flags;

	if (unlikely(!cluster))
		return -ENODEV;

	if (unlikely(!cluster->inited))
		return 0;

	/*
	 * Moving to the end of the list should only happen in
	 * CPU_ONLINE and not on CPU_UP_PREPARE to prevent an
	 * infinite list traversal when thermal (or other entities)
	 * reject trying to online CPUs.
	 */
	spin_lock_irqsave(&state_lock, flags);
	move_cpu_lru(state, false);
	spin_unlock_irqrestore(&state_lock, flags);

	apply_need(cluster, false);

	return 0;
}

static int __ref cpuhp_core_ctl_offline(unsigned int cpu)
{
	struct cpu_data *state = &per_cpu(cpu_state, cpu);
	struct cluster_data *cluster = state->cluster;
	unsigned long flags;

	if (unlikely(!cluster))
		return -ENODEV;

	if (unlikely(!cluster->inited))
		return 0;

	/*
	 * We don't want to have a CPU both offline and isolated.
	 * So unisolate a CPU that went down if it was isolated by us.
	 */
	spin_lock_irqsave(&state_lock, flags);
	if (state->isolated_by_us) {
		update_isolated_time(state);
		state->isolated_by_us = false;
		cluster->nr_isolated_cpus--;
	} else {
		/* Move a CPU to the end of the LRU when it goes offline. */
		move_cpu_lru(state, true);
	}

	state->load = 0;
	spin_unlock_irqrestore(&state_lock, flags);

	apply_need(cluster, false);

	return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
static int __ref cpu_callback(struct notifier_block *nfb,
			      unsigned long action, void *hcpu)
{
	uint32_t cpu = (uintptr_t)hcpu;

	switch (action & ~CPU_TASKS_FROZEN) {
	case CPU_ONLINE:
		if (cpuhp_core_ctl_online(cpu))
			return NOTIFY_DONE;
		break;

	case CPU_DEAD:
		if (cpuhp_core_ctl_offline(cpu))
			return NOTIFY_DONE;
		break;
	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct notifier_block __refdata cpu_notifier = {
	.notifier_call = cpu_callback,
};
#endif

/* ============================ init code ============================== */

static cpumask_var_t core_ctl_disable_cpumask;
static bool core_ctl_disable_cpumask_present;

static int __init core_ctl_disable_setup(char *str)
{
	if (!str) {
		pr_err("core_ctl: no cmdline str\n");
		return -EINVAL;
	}

	if (0 == *str) {
		pr_err("core_ctl: no valid cmdline\n");
		return -EINVAL;
	}

	alloc_bootmem_cpumask_var(&core_ctl_disable_cpumask);

	if (cpulist_parse(str, core_ctl_disable_cpumask) < 0) {
		pr_err("core_ctl: parse cpumask err\n");
		free_bootmem_cpumask_var(core_ctl_disable_cpumask);
		return -EINVAL;
	}

	core_ctl_disable_cpumask_present = true;
	pr_info("disable_cpumask=%*pbl\n",
			cpumask_pr_args(core_ctl_disable_cpumask));/*lint !e663*/

	return 0;
}
early_param("core_ctl_disable_cpumask", core_ctl_disable_setup);

static bool should_skip(const struct cpumask *mask)
{
	if (!core_ctl_disable_cpumask_present)
		return false;

	/*
	 * We operate on a cluster basis. Disable the core_ctl for
	 * a cluster, if all of it's cpus are specified in
	 * core_ctl_disable_cpumask
	 */
	return cpumask_subset(mask, core_ctl_disable_cpumask);
}

static struct cluster_data *find_cluster_by_first_cpu(unsigned int first_cpu)
{
	struct cluster_data *temp;

	list_for_each_entry(temp, &cluster_list, cluster_node) {
		if (temp->first_cpu == first_cpu)
			return temp;
	}

	return NULL;
}

static void insert_cluster_by_cap(struct cluster_data *cluster)
{
	struct cluster_data *temp;

	/* bigger capacity first */
	list_for_each_entry(temp, &cluster_list, cluster_node) {
		if (temp->capacity < cluster->capacity) {
			list_add_tail(&cluster->cluster_node,
				      &temp->cluster_node);
			return;
		}
	}

	list_add_tail(&cluster->cluster_node, &cluster_list);
}

static int cluster_init(const struct cpumask *mask)
{
	struct device *dev;
	unsigned int first_cpu = cpumask_first(mask);
	struct cluster_data *cluster;
	struct cpu_data *state;
	int cpu;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };

	/* disabled by cmdline */
	if (should_skip(mask))
		return 0;

	/* cluster data has been inited */
	if (find_cluster_by_first_cpu(first_cpu))
		return 0;

	dev = get_cpu_device(first_cpu);
	if (!dev) {
		pr_err("core_ctl: fail to get cpu device\n");
		return -ENODEV;
	}

	pr_info("Creating CPU group %d\n", first_cpu);

	cluster = devm_kzalloc(dev, sizeof(*cluster), GFP_KERNEL);
	if (!cluster) {
		pr_err("core_ctl: alloc cluster err\n");
		return -ENOMEM;
	}

	cpumask_copy(&cluster->cpu_mask, mask);
	cluster->num_cpus = cpumask_weight(mask);
	cluster->first_cpu = first_cpu;
	cluster->min_cpus = cluster->num_cpus;
	cluster->max_cpus = cluster->num_cpus;
	cluster->need_cpus = cluster->num_cpus;
	cluster->offline_delay_ms = 100;
	cluster->task_thres = UINT_MAX;
	cluster->nrrun = cluster->num_cpus;
	cluster->enable = true;
	cluster->spread_affinity = true;
	cluster->capacity = capacity_orig_of(first_cpu);
	INIT_LIST_HEAD(&cluster->lru);
	spin_lock_init(&cluster->pending_lock);

	for_each_cpu(cpu, mask) {
		state = &per_cpu(cpu_state, cpu);
		state->cluster = cluster;
		state->cpu = cpu;
		list_add(&state->sib, &cluster->lru);
	}
	cluster->active_cpus = get_active_cpu_count(cluster);

	cluster->core_ctl_thread = kthread_run(try_core_ctl, (void *) cluster,
					       "core_ctl/%d", first_cpu);
	if (IS_ERR(cluster->core_ctl_thread)) {
		pr_err("core_ctl: thread create err\n");
		return PTR_ERR(cluster->core_ctl_thread);/*lint !e593*/
	}

	sched_setscheduler_nocheck(cluster->core_ctl_thread, SCHED_FIFO,
				   &param);

	cluster->inited = true;

	insert_cluster_by_cap(cluster);

	kobject_init(&cluster->kobj, &ktype_core_ctl);
	return kobject_add(&cluster->kobj, &dev->kobj, "core_ctl");/*lint !e593*/
}

static int cpufreq_gov_cb(struct notifier_block *nb, unsigned long val,
			  void *data)
{
	struct cpufreq_govinfo *info = data;

	switch (val) {
	case CPUFREQ_LOAD_CHANGE:
		core_ctl_update_busy(info->cpu, info->load,
				     info->sampling_rate_us != 0);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block cpufreq_gov_nb = {
	.notifier_call = cpufreq_gov_cb,
};

static int __init core_ctl_init(void)
{
	int cpu;

	if (should_skip(cpu_possible_mask))
		return 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	register_cpu_notifier(&cpu_notifier);
#else
	cpuhp_setup_state_nocalls(CPUHP_AP_ONLINE_DYN, "core_ctl:online",
				  cpuhp_core_ctl_online, NULL);

	cpuhp_setup_state_nocalls(CPUHP_CORE_CTRL_DEAD, "core_ctl:dead",
				  NULL, cpuhp_core_ctl_offline);
#endif
	cpufreq_register_notifier(&cpufreq_gov_nb, CPUFREQ_GOVINFO_NOTIFIER);

	cpu_maps_update_begin();
	for_each_online_cpu(cpu) {
		int ret;

		ret = cluster_init(topology_core_cpumask(cpu));
		if (ret)
			pr_err("create core ctl group%d failed: %d\n",
			       cpu, ret);
	}
	cpu_maps_update_done();
	initialized = true;
	return 0;
}

late_initcall(core_ctl_init);
