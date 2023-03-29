/*
 *  linux/drivers/thermal/cpu_cooling.c
 *
 *  Copyright (C) 2012	Samsung Electronics Co., Ltd(http://www.samsung.com)
 *  Copyright (C) 2012  Amit Daniel <amit.kachhap@linaro.org>
 *
 *  Copyright (C) 2014  Viresh Kumar <viresh.kumar@linaro.org>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/cpu_cooling.h>

#include <trace/events/thermal.h>

#ifdef CONFIG_HISI_DRG
#include <linux/hisi/hisi_drg.h>
#endif

#ifdef CONFIG_HISI_IPA_THERMAL
#include <trace/events/thermal_power_allocator.h>
#ifdef CONFIG_HISI_THERMAL_SPM
#ifdef CONFIG_HISI_THERMAL_TRIPPLE_CLUSTERS
#define NUM_CLUSTERS 3
#else
#define NUM_CLUSTERS 2
#endif
extern unsigned int get_powerhal_profile(enum ipa_actor actor);
extern unsigned int get_minfreq_profile(enum ipa_actor actor);
extern bool is_spm_mode_enabled(void);

u32 profile_freq[NUM_CLUSTERS];
int get_profile_power(enum ipa_actor actor, unsigned int *power);
int hisi_calc_static_power(const struct cpumask *cpumask, int temp,
				unsigned long u_volt, u32 *static_power);
int get_soc_target_temp(struct thermal_cooling_device *cdev, int *target_temp);
#endif
extern unsigned int g_ipa_freq_limit[];
extern unsigned int g_ipa_soc_freq_limit[];
extern unsigned int g_ipa_board_freq_limit[];
extern unsigned int g_ipa_board_state[];
extern unsigned int g_ipa_soc_state[];
#endif

/*
 * Cooling state <-> CPUFreq frequency
 *
 * Cooling states are translated to frequencies throughout this driver and this
 * is the relation between them.
 *
 * Highest cooling state corresponds to lowest possible frequency.
 *
 * i.e.
 *	level 0 --> 1st Max Freq
 *	level 1 --> 2nd Max Freq
 *	...
 */

/**
 * struct power_table - frequency to power conversion
 * @frequency:	frequency in KHz
 * @power:	power in mW
 *
 * This structure is built when the cooling device registers and helps
 * in translating frequency to power and viceversa.
 */
struct power_table {
	u32 frequency;
	u32 power;
};

/**
 * struct cpufreq_cooling_device - data for cooling device with cpufreq
 * @id: unique integer value corresponding to each cpufreq_cooling_device
 *	registered.
 * @cool_dev: thermal_cooling_device pointer to keep track of the
 *	registered cooling device.
 * @cpufreq_state: integer value representing the current state of cpufreq
 *	cooling	devices.
 * @clipped_freq: integer value representing the absolute value of the clipped
 *	frequency.
 * @max_level: maximum cooling level. One less than total number of valid
 *	cpufreq frequencies.
 * @allowed_cpus: all the cpus involved for this cpufreq_cooling_device.
 * @node: list_head to link all cpufreq_cooling_device together.
 * @last_load: load measured by the latest call to cpufreq_get_requested_power()
 * @time_in_idle: previous reading of the absolute time that this cpu was idle
 * @time_in_idle_timestamp: wall time of the last invocation of
 *	get_cpu_idle_time_us()
 * @dyn_power_table: array of struct power_table for frequency to power
 *	conversion, sorted in ascending order.
 * @dyn_power_table_entries: number of entries in the @dyn_power_table array
 * @cpu_dev: the first cpu_device from @allowed_cpus that has OPPs registered
 * @plat_get_static_power: callback to calculate the static power
 *
 * This structure is required for keeping information of each registered
 * cpufreq_cooling_device.
 */
struct cpufreq_cooling_device {
	int id;
	struct thermal_cooling_device *cool_dev;
	unsigned int cpufreq_state;
	unsigned int clipped_freq;
	unsigned int max_level;
	unsigned int *freq_table;	/* In descending order */
	struct cpumask allowed_cpus;
	struct list_head node;
	u32 last_load;
	u64 *time_in_idle;
	u64 *time_in_idle_timestamp;
	struct power_table *dyn_power_table;
	int dyn_power_table_entries;
	struct device *cpu_dev;
	get_static_t plat_get_static_power;
};
static DEFINE_IDR(cpufreq_idr);
static DEFINE_MUTEX(cooling_cpufreq_lock);

static unsigned int cpufreq_dev_count;

static DEFINE_MUTEX(cooling_list_lock);
static LIST_HEAD(cpufreq_dev_list);

/**
 * get_idr - function to get a unique id.
 * @idr: struct idr * handle used to create a id.
 * @id: int * value generated by this function.
 *
 * This function will populate @id with an unique
 * id, using the idr API.
 *
 * Return: 0 on success, an error code on failure.
 */
static int get_idr(struct idr *idr, int *id)
{
	int ret;

	mutex_lock(&cooling_cpufreq_lock);
	ret = idr_alloc(idr, NULL, 0, 0, GFP_KERNEL);
	mutex_unlock(&cooling_cpufreq_lock);
	if (unlikely(ret < 0))
		return ret;
	*id = ret;

	return 0;
}

/**
 * release_idr - function to free the unique id.
 * @idr: struct idr * handle used for creating the id.
 * @id: int value representing the unique id.
 */
static void release_idr(struct idr *idr, int id)
{
	mutex_lock(&cooling_cpufreq_lock);
	idr_remove(idr, id);
	mutex_unlock(&cooling_cpufreq_lock);
}

/* Below code defines functions to be used for cpufreq as cooling device */

/**
 * get_level: Find the level for a particular frequency
 * @cpufreq_dev: cpufreq_dev for which the property is required
 * @freq: Frequency
 *
 * Return: level on success, THERMAL_CSTATE_INVALID on error.
 */
static unsigned long get_level(struct cpufreq_cooling_device *cpufreq_dev,
			       unsigned int freq)
{
	unsigned long level;

	for (level = 0; level <= cpufreq_dev->max_level; level++) {
		if (freq == cpufreq_dev->freq_table[level])
			return level;

		if (freq > cpufreq_dev->freq_table[level])
			break;
	}

	return THERMAL_CSTATE_INVALID;/*lint !e501*/
}

/**
 * cpufreq_cooling_get_level - for a given cpu, return the cooling level.
 * @cpu: cpu for which the level is required
 * @freq: the frequency of interest
 *
 * This function will match the cooling level corresponding to the
 * requested @freq and return it.
 *
 * Return: The matched cooling level on success or THERMAL_CSTATE_INVALID
 * otherwise.
 */
unsigned long cpufreq_cooling_get_level(unsigned int cpu, unsigned int freq)
{
	struct cpufreq_cooling_device *cpufreq_dev;

	mutex_lock(&cooling_list_lock);
	list_for_each_entry(cpufreq_dev, &cpufreq_dev_list, node) {
		if (cpumask_test_cpu(cpu, &cpufreq_dev->allowed_cpus)) {
			unsigned long level = get_level(cpufreq_dev, freq);

			mutex_unlock(&cooling_list_lock);
			return level;
		}
	}
	mutex_unlock(&cooling_list_lock);

	pr_err("%s: cpu:%d not part of any cooling device\n", __func__, cpu);
	return THERMAL_CSTATE_INVALID;/*lint !e501*/
}
EXPORT_SYMBOL_GPL(cpufreq_cooling_get_level);

/**
 * cpufreq_thermal_notifier - notifier callback for cpufreq policy change.
 * @nb:	struct notifier_block * with callback info.
 * @event: value showing cpufreq event for which this function invoked.
 * @data: callback-specific data
 *
 * Callback to hijack the notification on cpufreq policy transition.
 * Every time there is a change in policy, we will intercept and
 * update the cpufreq policy with thermal constraints.
 *
 * Return: 0 (success)
 */
static int cpufreq_thermal_notifier(struct notifier_block *nb,
				    unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;
	unsigned long clipped_freq;
	struct cpufreq_cooling_device *cpufreq_dev;
#ifdef CONFIG_HISI_THERMAL_SPM
	enum ipa_actor actor;
	unsigned int min_freq = 0, freq = 0;
#endif

	if (event != CPUFREQ_ADJUST)
		return NOTIFY_DONE;

	mutex_lock(&cooling_list_lock);
	list_for_each_entry(cpufreq_dev, &cpufreq_dev_list, node) {
		if (!cpumask_test_cpu(policy->cpu, &cpufreq_dev->allowed_cpus))
			continue;

		/*
		 * policy->max is the maximum allowed frequency defined by user
		 * and clipped_freq is the maximum that thermal constraints
		 * allow.
		 *
		 * If clipped_freq is lower than policy->max, then we need to
		 * readjust policy->max.
		 *
		 * But, if clipped_freq is greater than policy->max, we don't
		 * need to do anything.
		 */
		clipped_freq = cpufreq_dev->clipped_freq;
#ifndef CONFIG_HISI_THERMAL_SPM
		if (policy->max > clipped_freq)
			cpufreq_verify_within_limits(policy, 0, clipped_freq);
#else
		if (is_spm_mode_enabled()) {
			actor = (enum ipa_actor)topology_physical_package_id(policy->cpu);
			freq = get_powerhal_profile(actor);
			min_freq = get_minfreq_profile(actor);
			cpufreq_verify_within_limits(policy, min_freq, freq);
		} else {
		    if (policy->max != clipped_freq)
			    cpufreq_verify_within_limits(policy, 0, clipped_freq);
		}
#endif
		break;
	}
	mutex_unlock(&cooling_list_lock);

	return NOTIFY_OK;
}

/**
 * build_dyn_power_table() - create a dynamic power to frequency table
 * @cpufreq_device:	the cpufreq cooling device in which to store the table
 * @capacitance: dynamic power coefficient for these cpus
 *
 * Build a dynamic power to frequency table for this cpu and store it
 * in @cpufreq_device.  This table will be used in cpu_power_to_freq() and
 * cpu_freq_to_power() to convert between power and frequency
 * efficiently.  Power is stored in mW, frequency in KHz.  The
 * resulting table is in ascending order.
 *
 * Return: 0 on success, -EINVAL if there are no OPPs for any CPUs,
 * -ENOMEM if we run out of memory or -EAGAIN if an OPP was
 * added/enabled while the function was executing.
 */
static int build_dyn_power_table(struct cpufreq_cooling_device *cpufreq_device,
				 u32 capacitance)
{
	struct power_table *power_table;
	struct dev_pm_opp *opp;
	struct device *dev = NULL;
	int num_opps = 0, cpu, i, ret = 0;
	unsigned long freq;
#ifdef CONFIG_HISI_IPA_THERMAL
	u32 static_power;
	int nr_cpus;
#endif

	for_each_cpu(cpu, &cpufreq_device->allowed_cpus) {
		dev = get_cpu_device(cpu);
		if (!dev) {
			dev_warn(&cpufreq_device->cool_dev->device,
				 "No cpu device for cpu %d\n", cpu);
			continue;
		}

		num_opps = dev_pm_opp_get_opp_count(dev);
		if (num_opps > 0)
			break;
		else if (num_opps < 0)
			return num_opps;
	}

	if (num_opps == 0)
		return -EINVAL;

	power_table = kcalloc(num_opps, sizeof(*power_table), GFP_KERNEL);/*lint !e433*/
	if (!power_table)
		return -ENOMEM;

	rcu_read_lock();

	for (freq = 0, i = 0;
	     opp = dev_pm_opp_find_freq_ceil(dev, &freq), !IS_ERR(opp);
	     freq++, i++) {
		u32 freq_mhz, voltage_mv;
		u64 power;

		if (i >= num_opps) {
			rcu_read_unlock();
			ret = -EAGAIN;
			goto free_power_table;
		}

		freq_mhz = freq / 1000000;
		voltage_mv = dev_pm_opp_get_voltage(opp) / 1000;

		/*
		 * Do the multiplication with MHz and millivolt so as
		 * to not overflow.
		 */
		power = (u64)capacitance * freq_mhz * voltage_mv * voltage_mv;
		do_div(power, 1000000000);

		/* frequency is stored in power_table in KHz */
		power_table[i].frequency = freq / 1000;

		/* power is stored in mW */
		power_table[i].power = power;

#ifdef CONFIG_HISI_IPA_THERMAL
		nr_cpus = (int)cpumask_weight(&cpufreq_device->allowed_cpus);
		if (0 == nr_cpus)
			nr_cpus = 1;
		cpufreq_device->plat_get_static_power(&cpufreq_device->allowed_cpus, 0, (unsigned long)(voltage_mv * 1000), &static_power); /*lint !e647*/

		/* hisi static_power givern in cluster */
		static_power = static_power / (u32)nr_cpus;

		pr_debug("  %u MHz @ %u mV :  %u + %u = %u mW\n",
			freq_mhz, voltage_mv, power_table[i].power, static_power, power_table[i].power+static_power);
#endif
	}

	rcu_read_unlock();

	if (i != num_opps) {
		ret = PTR_ERR(opp);
		goto free_power_table;
	}

	cpufreq_device->cpu_dev = dev;
	cpufreq_device->dyn_power_table = power_table;
	cpufreq_device->dyn_power_table_entries = i;

	return 0;

free_power_table:
	kfree(power_table);

	return ret;
}

static u32 cpu_freq_to_power(struct cpufreq_cooling_device *cpufreq_device,
			     u32 freq)
{
	int i;
	struct power_table *pt = cpufreq_device->dyn_power_table;

	for (i = 1; i < cpufreq_device->dyn_power_table_entries; i++)
		if (freq < pt[i].frequency)
			break;

	return pt[i - 1].power;
}

static u32 cpu_power_to_freq(struct cpufreq_cooling_device *cpufreq_device,
			     u32 power)
{
	int i;
	struct power_table *pt = cpufreq_device->dyn_power_table;

	for (i = 1; i < cpufreq_device->dyn_power_table_entries; i++)
		if (power < pt[i].power)
			break;

	return pt[i - 1].frequency;
}

/**
 * get_load() - get load for a cpu since last updated
 * @cpufreq_device:	&struct cpufreq_cooling_device for this cpu
 * @cpu:	cpu number
 * @cpu_idx:	index of the cpu in cpufreq_device->allowed_cpus
 *
 * Return: The average load of cpu @cpu in percentage since this
 * function was last called.
 */
static u32 get_load(struct cpufreq_cooling_device *cpufreq_device, int cpu,
		    int cpu_idx)
{
	u32 load;
	u64 now, now_idle, delta_time, delta_idle;

	now_idle = get_cpu_idle_time(cpu, &now, 0);
	delta_idle = now_idle - cpufreq_device->time_in_idle[cpu_idx];
	delta_time = now - cpufreq_device->time_in_idle_timestamp[cpu_idx];

	if (delta_time <= delta_idle)
		load = 0;
	else
		load = div64_u64(100 * (delta_time - delta_idle), delta_time);

	cpufreq_device->time_in_idle[cpu_idx] = now_idle;
	cpufreq_device->time_in_idle_timestamp[cpu_idx] = now;

	return load;
}

/**
 * get_static_power() - calculate the static power consumed by the cpus
 * @cpufreq_device:	struct &cpufreq_cooling_device for this cpu cdev
 * @tz:		thermal zone device in which we're operating
 * @freq:	frequency in KHz
 * @power:	pointer in which to store the calculated static power
 *
 * Calculate the static power consumed by the cpus described by
 * @cpu_actor running at frequency @freq.  This function relies on a
 * platform specific function that should have been provided when the
 * actor was registered.  If it wasn't, the static power is assumed to
 * be negligible.  The calculated static power is stored in @power.
 *
 * Return: 0 on success, -E* on failure.
 */
static int get_static_power(struct cpufreq_cooling_device *cpufreq_device,
			    struct thermal_zone_device *tz, unsigned long freq,
			    u32 *power)
{
	struct dev_pm_opp *opp;
	unsigned long voltage;
	struct cpumask *cpumask = &cpufreq_device->allowed_cpus;
	unsigned long freq_hz = freq * 1000;

	if (!cpufreq_device->plat_get_static_power ||
	    !cpufreq_device->cpu_dev) {
		*power = 0;
		return 0;
	}

	rcu_read_lock();

	opp = dev_pm_opp_find_freq_exact(cpufreq_device->cpu_dev, freq_hz,
					 true);
	voltage = dev_pm_opp_get_voltage(opp);

	rcu_read_unlock();

	if (voltage == 0) {
		dev_warn_ratelimited(cpufreq_device->cpu_dev,
				     "Failed to get voltage for frequency %lu: %ld\n",
				     freq_hz, IS_ERR(opp) ? PTR_ERR(opp) : 0);
		return -EINVAL;
	}

	return cpufreq_device->plat_get_static_power(cpumask, tz->passive_delay,
						     voltage, power);
}

/**
 * get_dynamic_power() - calculate the dynamic power
 * @cpufreq_device:	&cpufreq_cooling_device for this cdev
 * @freq:	current frequency
 *
 * Return: the dynamic power consumed by the cpus described by
 * @cpufreq_device.
 */
static u32 get_dynamic_power(struct cpufreq_cooling_device *cpufreq_device,
			     unsigned long freq)
{
	u32 raw_cpu_power;

	raw_cpu_power = cpu_freq_to_power(cpufreq_device, freq);
	return (raw_cpu_power * cpufreq_device->last_load) / 100;
}

/* cpufreq cooling device callback functions are defined below */

/**
 * cpufreq_get_max_state - callback function to get the max cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the max cooling state.
 *
 * Callback for the thermal cooling device to return the cpufreq
 * max cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpufreq_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;

	*state = cpufreq_device->max_level;
	return 0;
}

/**
 * cpufreq_get_cur_state - callback function to get the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the current cooling state.
 *
 * Callback for the thermal cooling device to return the cpufreq
 * current cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpufreq_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;

	*state = cpufreq_device->cpufreq_state;

	return 0;
}

/**
 * cpufreq_set_cur_state - callback function to set the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: set this variable to the current cooling state.
 *
 * Callback for the thermal cooling device to change the cpufreq
 * current cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpufreq_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;
	unsigned int cpu = cpumask_any(&cpufreq_device->allowed_cpus);
	unsigned int clip_freq;
#ifdef CONFIG_HISI_IPA_THERMAL
	unsigned int cur_cluster;
	unsigned long limit_state;
#endif

	/* Request state should be less than max_level */
	if (WARN_ON(state > cpufreq_device->max_level))
		return -EINVAL;

#ifdef CONFIG_HISI_IPA_THERMAL
	cur_cluster = (unsigned int)topology_physical_package_id(cpu);

	if(g_ipa_soc_state[cur_cluster] <= cpufreq_device->max_level)
		g_ipa_soc_freq_limit[cur_cluster] = cpufreq_device->freq_table[g_ipa_soc_state[cur_cluster]];

	if(g_ipa_board_state[cur_cluster] <= cpufreq_device->max_level)
		g_ipa_board_freq_limit[cur_cluster] = cpufreq_device->freq_table[g_ipa_board_state[cur_cluster]];

	limit_state = max(g_ipa_soc_state[cur_cluster],g_ipa_board_state[cur_cluster]);/*lint !e1058*/

	/* only change new state when limit_state less than max_level */
	if (!WARN_ON(limit_state > cpufreq_device->max_level))
		state = max(state, limit_state);
#endif

	/* Check if the old cooling action is same as new cooling action */
	if (cpufreq_device->cpufreq_state == state)
		return 0;

	clip_freq = cpufreq_device->freq_table[state];
	cpufreq_device->cpufreq_state = state;
	cpufreq_device->clipped_freq = clip_freq;

#ifdef CONFIG_HISI_IPA_THERMAL
	g_ipa_freq_limit[cur_cluster] = clip_freq;
#endif
#ifdef CONFIG_HISI_DRG
	drg_cpufreq_cooling_update(cpu, clip_freq);
#endif
	cpufreq_update_policy(cpu);

	return 0;
}

/**
 * cpufreq_get_requested_power() - get the current power
 * @cdev:	&thermal_cooling_device pointer
 * @tz:		a valid thermal zone device pointer
 * @power:	pointer in which to store the resulting power
 *
 * Calculate the current power consumption of the cpus in milliwatts
 * and store it in @power.  This function should actually calculate
 * the requested power, but it's hard to get the frequency that
 * cpufreq would have assigned if there were no thermal limits.
 * Instead, we calculate the current power on the assumption that the
 * immediate future will look like the immediate past.
 *
 * We use the current frequency and the average load since this
 * function was last called.  In reality, there could have been
 * multiple opps since this function was last called and that affects
 * the load calculation.  While it's not perfectly accurate, this
 * simplification is good enough and works.  REVISIT this, as more
 * complex code may be needed if experiments show that it's not
 * accurate enough.
 *
 * Return: 0 on success, -E* if getting the static power failed.
 */
static int cpufreq_get_requested_power(struct thermal_cooling_device *cdev,
				       struct thermal_zone_device *tz,
				       u32 *power)
{
	unsigned long freq;
	int i = 0, cpu, ret;
	u32 static_power, dynamic_power, total_load = 0;
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;
	u32 *load_cpu = NULL;
#ifdef CONFIG_HISI_IPA_THERMAL
	u32 max_load = 0;
#endif

	cpu = cpumask_any_and(&cpufreq_device->allowed_cpus, cpu_online_mask);

	/*
	 * All the CPUs are offline, thus the requested power by
	 * the cdev is 0
	 */
	if (cpu >= nr_cpu_ids) {
		*power = 0;
		return 0;
	}

	freq = cpufreq_quick_get(cpu);
#ifdef CONFIG_HISI_IPA_THERMAL
	/* policy->cur equals 0, means the policy data of this cpu was NULL,
	   return early to avoid find voltage of freq(0) in the opp
	*/
	if (!freq) {
		*power = 0;
		return 0;
	}
#endif

#ifdef CONFIG_HISI_IPA_THERMAL
	if (1) { /*lint !e774*/
#else
	if (trace_thermal_power_cpu_get_power_enabled()) {
#endif
		u32 ncpus = cpumask_weight(&cpufreq_device->allowed_cpus);

		load_cpu = kcalloc(ncpus, sizeof(*load_cpu), GFP_KERNEL);
	}

	for_each_cpu(cpu, &cpufreq_device->allowed_cpus) {
		u32 load;

		if (cpu_online(cpu))
			load = get_load(cpufreq_device, cpu, i);
		else
			load = 0;

		total_load += load;

#ifdef CONFIG_HISI_IPA_THERMAL
		if (load > max_load)
			max_load = load;
#endif

#ifdef CONFIG_HISI_IPA_THERMAL
		if (load_cpu)
#else
		if (trace_thermal_power_cpu_limit_enabled() && load_cpu)
#endif
			load_cpu[i] = load;

		i++;
	}

	cpufreq_device->last_load = total_load;

	dynamic_power = get_dynamic_power(cpufreq_device, freq);
	ret = get_static_power(cpufreq_device, tz, freq, &static_power);
	if (ret) {
		kfree(load_cpu);/*lint !e668*/
		return ret;
	}

	if (load_cpu) {
		trace_thermal_power_cpu_get_power(
			&cpufreq_device->allowed_cpus,
			freq, load_cpu, i, dynamic_power, static_power);

#ifdef CONFIG_HISI_IPA_THERMAL
	if (tz->is_soc_thermal) {
		trace_IPA_actor_cpu_get_power(&cpufreq_device->allowed_cpus, freq,
				load_cpu, (unsigned long)((long)i), dynamic_power,
				static_power,(static_power + dynamic_power));
	}
#endif

		kfree(load_cpu);
	}

	*power = static_power + dynamic_power;

#ifdef CONFIG_HISI_IPA_THERMAL
	cdev->current_freq = freq;
	if (load_cpu)
		cdev->current_load = max_load;
#endif

	return 0;
}

/**
 * cpufreq_state2power() - convert a cpu cdev state to power consumed
 * @cdev:	&thermal_cooling_device pointer
 * @tz:		a valid thermal zone device pointer
 * @state:	cooling device state to be converted
 * @power:	pointer in which to store the resulting power
 *
 * Convert cooling device state @state into power consumption in
 * milliwatts assuming 100% load.  Store the calculated power in
 * @power.
 *
 * Return: 0 on success, -EINVAL if the cooling device state could not
 * be converted into a frequency or other -E* if there was an error
 * when calculating the static power.
 */
static int cpufreq_state2power(struct thermal_cooling_device *cdev,
			       struct thermal_zone_device *tz,
			       unsigned long state, u32 *power)
{
	unsigned int freq, num_cpus;
	cpumask_t cpumask;
	u32 static_power, dynamic_power;
	int ret;
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;

	cpumask_and(&cpumask, &cpufreq_device->allowed_cpus, cpu_online_mask);
	num_cpus = cpumask_weight(&cpumask);

	/* None of our cpus are online, so no power */
	if (num_cpus == 0) {
		*power = 0;
		return 0;
	}

	freq = cpufreq_device->freq_table[state];
	if (!freq)
		return -EINVAL;

	dynamic_power = cpu_freq_to_power(cpufreq_device, freq) * num_cpus;
	ret = get_static_power(cpufreq_device, tz, freq, &static_power);
	if (ret)
		return ret;

	*power = static_power + dynamic_power;
	return 0;
}

/**
 * cpufreq_power2state() - convert power to a cooling device state
 * @cdev:	&thermal_cooling_device pointer
 * @tz:		a valid thermal zone device pointer
 * @power:	power in milliwatts to be converted
 * @state:	pointer in which to store the resulting state
 *
 * Calculate a cooling device state for the cpus described by @cdev
 * that would allow them to consume at most @power mW and store it in
 * @state.  Note that this calculation depends on external factors
 * such as the cpu load or the current static power.  Calling this
 * function with the same power as input can yield different cooling
 * device states depending on those external factors.
 *
 * Return: 0 on success, -ENODEV if no cpus are online or -EINVAL if
 * the calculated frequency could not be converted to a valid state.
 * The latter should not happen unless the frequencies available to
 * cpufreq have changed since the initialization of the cpu cooling
 * device.
 */
static int cpufreq_power2state(struct thermal_cooling_device *cdev,
			       struct thermal_zone_device *tz, u32 power,
			       unsigned long *state)
{
	unsigned int cpu, cur_freq, target_freq;
	int ret;
	s32 dyn_power;
	u32 last_load, normalised_power, static_power;
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;

	cpu = cpumask_any_and(&cpufreq_device->allowed_cpus, cpu_online_mask);

	/* None of our cpus are online */
	if (cpu >= nr_cpu_ids)/*lint !e574*/
		return -ENODEV;

	cur_freq = cpufreq_quick_get(cpu);
#ifdef CONFIG_HISI_IPA_THERMAL
	if (!cur_freq)
		return -EINVAL;
#endif

	ret = get_static_power(cpufreq_device, tz, cur_freq, &static_power);
	if (ret)
		return ret;

	dyn_power = power - static_power;
	dyn_power = dyn_power > 0 ? dyn_power : 0;
	last_load = cpufreq_device->last_load ?: 1;
	normalised_power = (dyn_power * 100) / last_load;/*lint !e573*/
	target_freq = cpu_power_to_freq(cpufreq_device, normalised_power);

	*state = cpufreq_cooling_get_level(cpu, target_freq);
	if (*state == THERMAL_CSTATE_INVALID) {/*lint !e501*/
		dev_warn_ratelimited(&cdev->device,
				     "Failed to convert %dKHz for cpu %d into a cdev state\n",
				     target_freq, cpu);
		return -EINVAL;
	}

	trace_thermal_power_cpu_limit(&cpufreq_device->allowed_cpus,
				      target_freq, *state, power);
#ifdef CONFIG_HISI_IPA_THERMAL
	trace_IPA_actor_cpu_limit(&cpufreq_device->allowed_cpus, target_freq,
					*state, power);
#endif
	return 0;
}

/* Bind cpufreq callbacks to thermal cooling device ops */

static struct thermal_cooling_device_ops cpufreq_cooling_ops = {
	.get_max_state = cpufreq_get_max_state,
	.get_cur_state = cpufreq_get_cur_state,
	.set_cur_state = cpufreq_set_cur_state,
};

static struct thermal_cooling_device_ops cpufreq_power_cooling_ops = {
	.get_max_state		= cpufreq_get_max_state,
	.get_cur_state		= cpufreq_get_cur_state,
	.set_cur_state		= cpufreq_set_cur_state,
	.get_requested_power	= cpufreq_get_requested_power,
	.state2power		= cpufreq_state2power,
	.power2state		= cpufreq_power2state,
};

/* Notifier for cpufreq policy change */
static struct notifier_block thermal_cpufreq_notifier_block = {
	.notifier_call = cpufreq_thermal_notifier,
};

static unsigned int find_next_max(struct cpufreq_frequency_table *table,
				  unsigned int prev_max)
{
	struct cpufreq_frequency_table *pos;
	unsigned int max = 0;

	cpufreq_for_each_valid_entry(pos, table) {
		if (pos->frequency > max && pos->frequency < prev_max)
			max = pos->frequency;
	}

	return max;
}

#ifdef CONFIG_HISI_THERMAL_SPM
/*lint -e64 -e826 -e771 -esym(64,826,771,*)*/
int cpufreq_update_policies(void)
{
	struct cpufreq_cooling_device *cpufreq_dev;
	unsigned int cpus[NUM_CLUSTERS];
	int i, num = 0;

	mutex_lock(&cooling_cpufreq_lock);
	list_for_each_entry(cpufreq_dev, &cpufreq_dev_list, node) {
		if (num >= NUM_CLUSTERS)
			break;
		cpus[num] = cpumask_any(&cpufreq_dev->allowed_cpus);
		num++;
	}
	mutex_unlock(&cooling_cpufreq_lock);

	for (i = 0; i < num; i++)
		cpufreq_update_policy(cpus[i]);

	return 0;
}
/*lint -e64 -e826 -e771 +esym(64,826,771,*)*/
EXPORT_SYMBOL(cpufreq_update_policies);

static int cpufreq_freq2volt(struct cpufreq_cooling_device *cpufreq_device, unsigned long freq,
				unsigned long *voltage)
{
	struct dev_pm_opp *opp;
	unsigned long freq_hz = freq * 1000;

	if (!cpufreq_device->cpu_dev) {
		*voltage = 0;
		return -EINVAL;
	}

	rcu_read_lock();

	opp = dev_pm_opp_find_freq_exact(cpufreq_device->cpu_dev, freq_hz,
					 (bool)true);
	*voltage = dev_pm_opp_get_voltage(opp);

	rcu_read_unlock();

	if (*voltage == 0) {
		/*lint -e64 -e570 -e785 -esym(64,570,785,*)*/
		dev_warn_ratelimited(cpufreq_device->cpu_dev,
				     "Failed to get voltage for frequency %lu: %ld\n",
				     freq_hz, IS_ERR(opp) ? PTR_ERR(opp) : 0);
		/*lint -e64 -e570 -e785 +esym(64,570,785,*)*/
		return -EINVAL;
	}

	return 0;
}

static int cpufreq_power2freq(struct thermal_cooling_device *cdev, u32 power, u32 *freq)
{
	int target_temp = 0;
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;
	struct power_table *pt = cpufreq_device->dyn_power_table;
	int i, ret;
	u32 static_power, num_cpus;
	unsigned long voltage;

	ret = get_soc_target_temp(cdev, &target_temp);
	if (ret)
		return ret;

	num_cpus = cpumask_weight(&cpufreq_device->allowed_cpus);

	for (i = 1; i < cpufreq_device->dyn_power_table_entries; i++) {
		ret = cpufreq_freq2volt(cpufreq_device, (unsigned long)pt[i].frequency, &voltage);
		if (ret)
			return ret;
		hisi_calc_static_power(&cpufreq_device->allowed_cpus, target_temp, voltage, &static_power);
		if ((pt[i].power * num_cpus + static_power) > power)
			break;
	}
	*freq = pt[i - 1].frequency;

	return 0;
}

int get_profile_cpu_freq(enum ipa_actor actor, u32 *freq)
{
	if (actor >= IPA_GPU)
		return -EDOM;

	*freq = profile_freq[actor]; /* [false alarm]:  check done */

	return 0;
}
EXPORT_SYMBOL(get_profile_cpu_freq);
#endif

/**
 * __cpufreq_cooling_register - helper function to create cpufreq cooling device
 * @np: a valid struct device_node to the cooling device device tree node
 * @clip_cpus: cpumask of cpus where the frequency constraints will happen.
 * Normally this should be same as cpufreq policy->related_cpus.
 * @capacitance: dynamic power coefficient for these cpus
 * @plat_static_func: function to calculate the static power consumed by these
 *                    cpus (optional)
 *
 * This interface function registers the cpufreq cooling device with the name
 * "thermal-cpufreq-%x". This api can support multiple instances of cpufreq
 * cooling devices. It also gives the opportunity to link the cooling device
 * with a device tree node, in order to bind it via the thermal DT code.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
static struct thermal_cooling_device *
__cpufreq_cooling_register(struct device_node *np,
			const struct cpumask *clip_cpus, u32 capacitance,
			get_static_t plat_static_func)
{
	struct cpufreq_policy *policy;
	struct thermal_cooling_device *cool_dev;
	struct cpufreq_cooling_device *cpufreq_dev;
	char dev_name[THERMAL_NAME_LENGTH];/*lint !e578*/
	struct cpufreq_frequency_table *pos, *table;
	struct cpumask temp_mask;
	unsigned int freq, i, num_cpus;
	int ret;
	struct thermal_cooling_device_ops *cooling_ops;
#ifdef CONFIG_HISI_THERMAL_SPM
	int cpu_id;
	enum ipa_actor actor;
	u32 power;
#endif

	cpumask_and(&temp_mask, clip_cpus, cpu_online_mask);
	policy = cpufreq_cpu_get(cpumask_first(&temp_mask));
	if (!policy) {
		pr_debug("%s: CPUFreq policy not found\n", __func__);
		return ERR_PTR(-EPROBE_DEFER);
	}

	table = policy->freq_table;
	if (!table) {
		pr_debug("%s: CPUFreq table not found\n", __func__);
		cool_dev = ERR_PTR(-ENODEV);
		goto put_policy;
	}

	cpufreq_dev = kzalloc(sizeof(*cpufreq_dev), GFP_KERNEL);
	if (!cpufreq_dev) {
		cool_dev = ERR_PTR(-ENOMEM);
		goto put_policy;
	}

	num_cpus = cpumask_weight(clip_cpus);
	cpufreq_dev->time_in_idle = kcalloc(num_cpus,
					    sizeof(*cpufreq_dev->time_in_idle),
					    GFP_KERNEL);
	if (!cpufreq_dev->time_in_idle) {
		cool_dev = ERR_PTR(-ENOMEM);
		goto free_cdev;
	}

	cpufreq_dev->time_in_idle_timestamp =
		kcalloc(num_cpus, sizeof(*cpufreq_dev->time_in_idle_timestamp),
			GFP_KERNEL);
	if (!cpufreq_dev->time_in_idle_timestamp) {
		cool_dev = ERR_PTR(-ENOMEM);
		goto free_time_in_idle;
	}

	/* Find max levels */
	cpufreq_for_each_valid_entry(pos, table)
		cpufreq_dev->max_level++;

	cpufreq_dev->freq_table = kmalloc(sizeof(*cpufreq_dev->freq_table) *
					  cpufreq_dev->max_level, GFP_KERNEL);
	if (!cpufreq_dev->freq_table) {
		cool_dev = ERR_PTR(-ENOMEM);
		goto free_time_in_idle_timestamp;
	}

	/* max_level is an index, not a counter */
	cpufreq_dev->max_level--;

	cpumask_copy(&cpufreq_dev->allowed_cpus, clip_cpus);

	if (capacitance) {
		cpufreq_dev->plat_get_static_power = plat_static_func;

		ret = build_dyn_power_table(cpufreq_dev, capacitance);
		if (ret) {
			cool_dev = ERR_PTR(ret);
			goto free_table;
		}

		cooling_ops = &cpufreq_power_cooling_ops;
	} else {
		cooling_ops = &cpufreq_cooling_ops;
	}

	ret = get_idr(&cpufreq_idr, &cpufreq_dev->id);
	if (ret) {
		cool_dev = ERR_PTR(ret);
		goto free_power_table;
	}

	/* Fill freq-table in descending order of frequencies */
	for (i = 0, freq = -1; i <= cpufreq_dev->max_level; i++) {/*lint !e570*/
		freq = find_next_max(table, freq);
		cpufreq_dev->freq_table[i] = freq;

		/* Warn for duplicate entries */
		if (!freq)
			pr_warn("%s: table has duplicate entries\n", __func__);
		else
			pr_debug("%s: freq:%u KHz\n", __func__, freq);
	}

	snprintf(dev_name, sizeof(dev_name), "thermal-cpufreq-%d",
		 cpufreq_dev->id);

	cool_dev = thermal_of_cooling_device_register(np, dev_name, cpufreq_dev,
						      cooling_ops);
	if (IS_ERR(cool_dev))
		goto remove_idr;

	cpufreq_dev->clipped_freq = cpufreq_dev->freq_table[0];
	cpufreq_dev->cool_dev = cool_dev;

	mutex_lock(&cooling_cpufreq_lock);

	mutex_lock(&cooling_list_lock);
	list_add(&cpufreq_dev->node, &cpufreq_dev_list);
	mutex_unlock(&cooling_list_lock);

	/* Register the notifier for first cpufreq cooling device */
	if (!cpufreq_dev_count++)
		cpufreq_register_notifier(&thermal_cpufreq_notifier_block,
					  CPUFREQ_POLICY_NOTIFIER);
	mutex_unlock(&cooling_cpufreq_lock);

#ifdef CONFIG_HISI_THERMAL_SPM
	cpu_id = (int)cpumask_any(clip_cpus);
	actor = (enum ipa_actor)topology_physical_package_id(cpu_id);

	get_profile_power(actor, &power);
	cpufreq_power2freq(cool_dev, power, &profile_freq[actor]);
	pr_err("IPA: actor: %d, freq: %d\n", actor, profile_freq[actor]);
#endif
	goto put_policy;

remove_idr:
	release_idr(&cpufreq_idr, cpufreq_dev->id);
free_power_table:
	kfree(cpufreq_dev->dyn_power_table);
free_table:
	kfree(cpufreq_dev->freq_table);
free_time_in_idle_timestamp:
	kfree(cpufreq_dev->time_in_idle_timestamp);
free_time_in_idle:
	kfree(cpufreq_dev->time_in_idle);
free_cdev:
	kfree(cpufreq_dev);
put_policy:
	cpufreq_cpu_put(policy);

	return cool_dev;/*lint !e593*/
}

/**
 * cpufreq_cooling_register - function to create cpufreq cooling device.
 * @clip_cpus: cpumask of cpus where the frequency constraints will happen.
 *
 * This interface function registers the cpufreq cooling device with the name
 * "thermal-cpufreq-%x". This api can support multiple instances of cpufreq
 * cooling devices.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
struct thermal_cooling_device *
cpufreq_cooling_register(const struct cpumask *clip_cpus)
{
	return __cpufreq_cooling_register(NULL, clip_cpus, 0, NULL);
}
EXPORT_SYMBOL_GPL(cpufreq_cooling_register);

/**
 * of_cpufreq_cooling_register - function to create cpufreq cooling device.
 * @np: a valid struct device_node to the cooling device device tree node
 * @clip_cpus: cpumask of cpus where the frequency constraints will happen.
 *
 * This interface function registers the cpufreq cooling device with the name
 * "thermal-cpufreq-%x". This api can support multiple instances of cpufreq
 * cooling devices. Using this API, the cpufreq cooling device will be
 * linked to the device tree node provided.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
struct thermal_cooling_device *
of_cpufreq_cooling_register(struct device_node *np,
			    const struct cpumask *clip_cpus)
{
	if (!np)
		return ERR_PTR(-EINVAL);

	return __cpufreq_cooling_register(np, clip_cpus, 0, NULL);
}
EXPORT_SYMBOL_GPL(of_cpufreq_cooling_register);

/**
 * cpufreq_power_cooling_register() - create cpufreq cooling device with power extensions
 * @clip_cpus:	cpumask of cpus where the frequency constraints will happen
 * @capacitance:	dynamic power coefficient for these cpus
 * @plat_static_func:	function to calculate the static power consumed by these
 *			cpus (optional)
 *
 * This interface function registers the cpufreq cooling device with
 * the name "thermal-cpufreq-%x".  This api can support multiple
 * instances of cpufreq cooling devices.  Using this function, the
 * cooling device will implement the power extensions by using a
 * simple cpu power model.  The cpus must have registered their OPPs
 * using the OPP library.
 *
 * An optional @plat_static_func may be provided to calculate the
 * static power consumed by these cpus.  If the platform's static
 * power consumption is unknown or negligible, make it NULL.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
struct thermal_cooling_device *
cpufreq_power_cooling_register(const struct cpumask *clip_cpus, u32 capacitance,
			       get_static_t plat_static_func)
{
	return __cpufreq_cooling_register(NULL, clip_cpus, capacitance,
				plat_static_func);
}
EXPORT_SYMBOL(cpufreq_power_cooling_register);

/**
 * of_cpufreq_power_cooling_register() - create cpufreq cooling device with power extensions
 * @np:	a valid struct device_node to the cooling device device tree node
 * @clip_cpus:	cpumask of cpus where the frequency constraints will happen
 * @capacitance:	dynamic power coefficient for these cpus
 * @plat_static_func:	function to calculate the static power consumed by these
 *			cpus (optional)
 *
 * This interface function registers the cpufreq cooling device with
 * the name "thermal-cpufreq-%x".  This api can support multiple
 * instances of cpufreq cooling devices.  Using this API, the cpufreq
 * cooling device will be linked to the device tree node provided.
 * Using this function, the cooling device will implement the power
 * extensions by using a simple cpu power model.  The cpus must have
 * registered their OPPs using the OPP library.
 *
 * An optional @plat_static_func may be provided to calculate the
 * static power consumed by these cpus.  If the platform's static
 * power consumption is unknown or negligible, make it NULL.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
struct thermal_cooling_device *
of_cpufreq_power_cooling_register(struct device_node *np,
				  const struct cpumask *clip_cpus,
				  u32 capacitance,
				  get_static_t plat_static_func)
{
	if (!np)
		return ERR_PTR(-EINVAL);

	return __cpufreq_cooling_register(np, clip_cpus, capacitance,
				plat_static_func);
}
EXPORT_SYMBOL(of_cpufreq_power_cooling_register);

/**
 * cpufreq_cooling_unregister - function to remove cpufreq cooling device.
 * @cdev: thermal cooling device pointer.
 *
 * This interface function unregisters the "thermal-cpufreq-%x" cooling device.
 */
void cpufreq_cooling_unregister(struct thermal_cooling_device *cdev)
{
	struct cpufreq_cooling_device *cpufreq_dev;

	if (!cdev)
		return;

	cpufreq_dev = cdev->devdata;

	/* Unregister the notifier for the last cpufreq cooling device */
	mutex_lock(&cooling_cpufreq_lock);
	if (!--cpufreq_dev_count)
		cpufreq_unregister_notifier(&thermal_cpufreq_notifier_block,
					    CPUFREQ_POLICY_NOTIFIER);

	mutex_lock(&cooling_list_lock);
	list_del(&cpufreq_dev->node);
	mutex_unlock(&cooling_list_lock);

	mutex_unlock(&cooling_cpufreq_lock);

	thermal_cooling_device_unregister(cpufreq_dev->cool_dev);
	release_idr(&cpufreq_idr, cpufreq_dev->id);
	kfree(cpufreq_dev->dyn_power_table);
	kfree(cpufreq_dev->time_in_idle_timestamp);
	kfree(cpufreq_dev->time_in_idle);
	kfree(cpufreq_dev->freq_table);
	kfree(cpufreq_dev);
}
EXPORT_SYMBOL_GPL(cpufreq_cooling_unregister);
