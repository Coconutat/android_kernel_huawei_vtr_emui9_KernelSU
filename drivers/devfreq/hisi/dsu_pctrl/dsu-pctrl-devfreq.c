/*
 * A devfreq driver for ARM DynamIQ Shared Unit (DSU) CPU memory power
 * managemnt.
 *
 * Copyright (c) 2015-2017 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *		Sean McGoogan <Sean.McGoogan@arm.com>
 *		Lukasz Luba <lukasz.luba@arm.com>
 *		Ionela Voinescu <Ionela.Voinescu@arm.com>
 *		David Guillen Fandos, ARM Ltd.
 *
 * The cache in DSU supports partial power down by splitting the cache into an
 * implementation-specific number of portions. This driver provides an
 * energy-cost justified demand-driven policy for controlling the number of
 * portions enabled.
 *
 * The policy maps the number of portions enabled to frequency (1 portion ==
 * 1Hz) and provides total/busy time statistics such that
 * busy_time = (total_time * hit_count) / access_count.
 *
 * This also provides a specific DevFreq governor that will implement the
 * desired energy-cost-justification policy as this version is not suitable
 * for use with the simple_ondemand governor.
 * DevFreq min/max/governor controls work as usual.
 *
 *
 * There is no relation to actual frequency control of the cache device.
 */

#define pr_fmt(fmt) "dsu_pctrl: " fmt

#include <linux/devfreq.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <asm/compiler.h>
#include <trace/events/power.h>
#include <linux/cpu_pm.h>
#include <linux/cpu.h>
#include <linux/perf_event.h>

#include "governor.h"

#define CREATE_TRACE_POINTS
#include <trace/events/dsu_pctrl.h>
#include "hisi_l3share.h"


#define DSU_PCTRL_PLATFORM_DEVICE_NAME			"dsu_pctrl"
#define DSU_PCTRL_GOVERNOR_NAME				"dsu_pc_governor"

#define DSU_PCTRL_DEFAULT_POLLING_MS			10
#define POLLING_DOWN_INTERVAL				10
/* Required polling time in miliseconds */
#define DSU_PCTRL_DEFAULT_REQUIRED_GOV_POLLING		10000

/* Default cache line size */
#define DSU_PCTRL_DEFAULT_LINE_SIZE			64

/* Default size of the cache: 1MB */
#define DSU_PCTRL_DEFAULT_SIZE_KB			1024

/* Default static energy leaked by the cache per MB */
#define DSU_PCTRL_DEFAULT_CACHE_LEAKAGE			10000

/*
 * Amount of energy used by the DRAM system per MB
 * of transferred data (on average) expressed in uJ/MB
 */
#define DSU_PCTRL_DEFAULT_DRAM_ENERGY_PER_MB		130

#define SZ_1KB						(1ULL << 10)
#define SZ_1MB						(1ULL << 20)
#define ROUNDING_INTEGER_MUL				SZ_1MB
#define PERCENT_MAX					100

/*
 * Portion threshold to use for down-sizing (Td)
 * between 0 and 100
 * 90 for 90%
 */
#define DOWNSIZE_PORTION_THRESHOLD			90

/*
 * Portion threshold to use for up-sizing (Tu)
 * between 0 and 100
 * 90 for 90%
 */
#define UPSIZE_PORTION_THRESHOLD			90

/* Bit-field positions for the CLUSTERPWRCTLR_EL1 register */
#define PORTION_1					4
#define PORTION_2					5
#define PORTION_3					6
#define PORTION_4					7
/* Bit-masks for the CLUSTERPWRCTLR_EL1 register */
#define PORTION_MASK		(BIT(PORTION_1) | BIT(PORTION_2) | \
				 BIT(PORTION_3) | BIT(PORTION_4))

#define CLUSTERIDR_VARIANT_BASE				4
#define CLUSTERIDR_VARIANT_MASK	(BIT(4) | BIT(5) | BIT(6) | BIT(7))

//#define DSU_PCTRL_HRTIMER_ENABLE


#define MAX_COUNT_VAL 0xFFFFFFFF
#define MAX_COUNT_LEN 32

#if defined(CONFIG_ARM64)
/*
 * Read the system register 'sysreg' (using MRS), and then
 * explicitly clear it, by writing zero to it (using MSR).
 * Put the value read into 'result'.
 */
#define SYS_REG_READ_THEN_CLEAR(sysreg, result)		\
do {							\
	__asm__ __volatile__(				\
		"mrs	%0, " #sysreg		"\n\t"	\
		"msr	" #sysreg ", XZR"		\
		: "=r" (result) /* only one output */	\
		/* no inputs */				\
		/* no clobbers */			\
	);						\
} while (0)

/*
 * Read the system register 'sysreg' (using MRS).
 * Put the value read into 'result'.
 */
#define SYS_REG_READ(sysreg, result)			\
do {							\
	__asm__ __volatile__(				\
		"mrs	%0, " #sysreg			\
		: "=r" (result) /* only one output */	\
		/* no inputs */				\
		/* no clobbers */			\
	);						\
} while (0)

/*
 * Write 'value' to the system register 'sysreg' (using MSR).
 */
#define SYS_REG_WRITE(sysreg, value)			\
do {							\
	__asm__ __volatile__(				\
		"msr	" #sysreg ", %0"		\
		: /* no outputs */			\
		: "r" (value) /* only one input */	\
		/* no clobbers */			\
	);						\
} while (0)

#else

#define SYS_REG_READ_THEN_CLEAR(sysreg, result)	do {} while (0)
#define SYS_REG_READ(sysreg, result)	do {} while (0)
#define SYS_REG_READ_THEN_CLEAR(sysreg, result)	do {} while (0)
#define SYS_REG_WRITE(sysreg, result)	do {} while (0)

#endif


#ifdef CONFIG_HISI_L3CACHE_SHARE_PERF
#define FORBID_SHARE_NUM		1
#define L3D_CACHE_REFILL_EV		0x02A
#define L3D_CACHE_EV			0x02B

enum ev_index {
	L3D_CACHE_REFILL_IDX,
	L3D_CACHE_IDX,
	NUM_EVENTS
};

struct event_data {
	struct perf_event *pevent;
	unsigned long prev_count;
};

struct dev_stats {
	unsigned long l3d_refill_cnt;
	unsigned long l3d_cnt;
};

struct cpu_pmu_stats {
	struct event_data events[NUM_EVENTS];
	ktime_t prev_ts;
};

struct cpu_grp_info {
	cpumask_t cpus;
	cpumask_t inited_cpus;
	unsigned int event_ids[NUM_EVENTS];
	struct cpu_pmu_stats *cpustats;
	struct notifier_block perf_cpu_notif;
	struct list_head mon_list;
};

struct perf_hwmon{
	struct cpu_grp_info *cpu_grp;
	cpumask_t cpus;
	unsigned int num_cores;
	struct dev_stats *core_stats;
	unsigned long total_refill;
	unsigned long total_access;
};


#define to_cpustats(cpu_grp, cpu) \
	(&cpu_grp->cpustats[cpu - cpumask_first(&cpu_grp->cpus)])
#define to_devstats(hw, cpu) \
	(&hw->core_stats[cpu - cpumask_first(&hw->cpus)])

static LIST_HEAD(perf_mon_list);
static DEFINE_MUTEX(list_lock);


extern struct mutex l3c_acp_lock;
extern bool req_pending_flag;
extern bool acp_enable_flag;
extern bool acp_first_flag;

extern void l3_cache_enable_acp(unsigned int id, unsigned int size);
extern void l3_cache_disable_acp(unsigned int id);
#endif



enum arm_dsu_version {
	ARM_DSU_R0 = 0,
	ARM_DSU_R1,
	ARM_DSU_END
};

struct dsu_pctrl_data {
	enum arm_dsu_version dsu_version;
	u32 portion_min;
	u32 portion_max;
};

struct dsu_pctrl {
	int id;
	struct dsu_pctrl_data *dsu_data;
	struct devfreq *devfreq;
	struct platform_device *pdev;
	struct devfreq_dev_profile *devfreq_profile;
	u32 portions;
	u32 size;
	u32 line_size;
	u32 up_polling_ms;
	u32 down_polling_ms;
	unsigned long initial_freq;
	u32 static_leakage_per_mb;
	u32 dram_energy_per_mb;
	u32 down_interval;
	u32 downsize_ratio;
	u32 upsize_ratio;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	unsigned int *freq_table;
#else
	unsigned long *freq_table;
#endif
	int freq_table_len;

#ifdef DSU_PCTRL_HRTIMER_ENABLE
	struct workqueue_struct *update_wq;
	struct work_struct update_handle;
#endif

	struct mutex lock;

#ifdef DSU_PCTRL_HRTIMER_ENABLE
	struct hrtimer poll_timer;
#endif

	/* Leakage (static power) for a single portion (in uW) */
	unsigned long static_leakage;
	unsigned long downsize_threshold;
	unsigned long upsize_threshold;
	unsigned long max_threshold;
	unsigned long cur_num_portions;

	/* Contains state for the algorithm. It is clean during resume */
	struct {
		unsigned long accesses_up;
		unsigned long misses_up;
		unsigned long accesses_down;
		unsigned long misses_down;
		unsigned long usec_up;
		unsigned long usec_down;
		unsigned int last_update;
	} alg;
#ifdef CONFIG_HISI_L3CACHE_SHARE_PERF
	/* when module request L3cache, at once stop hrtimer & devfreq_monitor
	or enable/disable ACP dynamically */
	struct notifier_block l3c_acp_notify;
	struct perf_hwmon *hw;
	module_id l3share_id;
	unsigned int l3share_size;

#endif
};



static atomic_t dsu_pctrl_device_id = ATOMIC_INIT(0);

static const struct dsu_pctrl_data device_data[] = {
	{.dsu_version = ARM_DSU_R0, .portion_min = 1, .portion_max = 2},
	{.dsu_version = ARM_DSU_R1, .portion_min = 1, .portion_max = 4},
};

static const struct of_device_id dsu_pctrl_devfreq_id[] = {
	{.compatible = "arm,dsu_pctrl"},
	{}
};

#define DSU_PCTRL_SVC			(0xc500f080)

typedef enum _DSU_PCTRL_CMD {
	CMD_GET_HIT_MISS_COUNT = 0,
	CMD_SET_PWR_CTLR,
} dsu_pctrl_cmd;

noinline u64 atfd_dsu_pctrl_smc(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;

	asm volatile (
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
		"smc	#0\n"
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));

	return (u64)function_id;
}

#ifndef CONFIG_HISI_L3CACHE_SHARE_PERF
static unsigned long get_l3_hit_miss_count(void)
{
	unsigned long val;

	val = atfd_dsu_pctrl_smc((u64)DSU_PCTRL_SVC, (u64)CMD_GET_HIT_MISS_COUNT, (u64)0, (u64)0);
	return val;
}
#endif

static void set_pwr_ctlr(u64 val)
{
	atfd_dsu_pctrl_smc((u64)DSU_PCTRL_SVC, (u64)CMD_SET_PWR_CTLR, (u64)val, (u64)0); /* CLUSTERPWRCTLR_EL1 */
}

#ifdef CONFIG_HISI_DSU_PCTRL_DEBUG
static ssize_t store_downsize_ratio(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct dsu_pctrl *dsu = dev_get_drvdata(devfreq->dev.parent);

	unsigned long value = 0;
	int ret = 0;

	ret = sscanf(buf, "%lu", &value);	/* unsafe_function_ignore: sscanf */
	if (ret != 1)
		return -EINVAL;

	mutex_lock(&dsu->devfreq->lock);
	if (value > 0 && value <= PERCENT_MAX) {
		dsu->downsize_ratio = value;
		dsu->downsize_threshold = (dsu->downsize_ratio * ROUNDING_INTEGER_MUL) / PERCENT_MAX;
		dsu->downsize_threshold *= dsu->static_leakage;
		dsu->downsize_threshold /= dsu->dram_energy_per_mb;
		ret = count;
	}else{
		ret = -EINVAL;
	}
	mutex_unlock(&dsu->devfreq->lock);
	return ret;
}

static ssize_t show_downsize_ratio(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct dsu_pctrl *dsu = dev_get_drvdata(devfreq->dev.parent);
	ssize_t count = 0;
	int ret = 0;

	ret = snprintf(buf + count, (PAGE_SIZE - count), "downsize_ratio = %u\n", dsu->downsize_ratio); /* unsafe_function_ignore: snprintf */
	if (ret >= (PAGE_SIZE - count) || ret < 0) {	/*lint !e574 */
		goto err;
	}
	count += ret;

	ret = snprintf(buf + count, (PAGE_SIZE - count), "downsize_threshold = %lu\n", dsu->downsize_threshold); /* unsafe_function_ignore: snprintf */
	if (ret >= (PAGE_SIZE - count) || ret < 0) {	/*lint !e574 */
		goto err;
	}
	count += ret;

err:
	return count;
}


static ssize_t store_upsize_ratio(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct dsu_pctrl *dsu = dev_get_drvdata(devfreq->dev.parent);

	unsigned long value = 0;
	int ret = 0;

	ret = sscanf(buf, "%lu", &value);	/* unsafe_function_ignore: sscanf */
	if (ret != 1)
		return -EINVAL;

	mutex_lock(&dsu->devfreq->lock);
	if (value < PERCENT_MAX) {
		dsu->upsize_ratio = value;
		dsu->upsize_threshold = ((PERCENT_MAX - dsu->upsize_ratio) * ROUNDING_INTEGER_MUL) / PERCENT_MAX;
		dsu->upsize_threshold *= dsu->static_leakage;
		dsu->upsize_threshold /= dsu->dram_energy_per_mb;
		ret = count;
	}else{
		ret = -EINVAL;
	}
	mutex_unlock(&dsu->devfreq->lock);
	return ret;
}

static ssize_t show_upsize_ratio(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct dsu_pctrl *dsu = dev_get_drvdata(devfreq->dev.parent);
	ssize_t count = 0;
	int ret = 0;

	ret = snprintf(buf + count, (PAGE_SIZE - count), "upsize_ratio = %u\n", dsu->upsize_ratio);	/* unsafe_function_ignore: snprintf */
	if (ret >= (PAGE_SIZE - count) || ret < 0) {	/*lint !e574 */
		goto err;
	}
	count += ret;

	ret = snprintf(buf + count, (PAGE_SIZE - count), "upsize_threshold = %lu\n", dsu->upsize_threshold);	/* unsafe_function_ignore: snprintf */
	if (ret >= (PAGE_SIZE - count) || ret < 0) {	/*lint !e574 */
		goto err;
	}
	count += ret;

err:
	return count;
}


static ssize_t store_static_leakage(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct dsu_pctrl *dsu = dev_get_drvdata(devfreq->dev.parent);
	struct dsu_pctrl_data *data = dsu->dsu_data;

	unsigned long value = 0;
	int ret = 0;

	ret = sscanf(buf, "%lu", &value);	/* unsafe_function_ignore: sscanf */
	if (ret != 1)
		return -EINVAL;

	mutex_lock(&dsu->devfreq->lock);
	if (value > 0) {
		dsu->static_leakage_per_mb = value;
		/* Leakage (static power) for a single portion (in uW) */
		dsu->static_leakage = dsu->static_leakage_per_mb * dsu->size; //lint !e647
		dsu->static_leakage /= SZ_1KB;
		dsu->static_leakage /= data->portion_max;
		dev_err(dev, "static_leakage = %lu\n", dsu->static_leakage);

		/* Downsize and upsize thresholds are given in percentages */
		dsu->downsize_threshold = dsu->downsize_ratio;
		dsu->downsize_threshold = (dsu->downsize_threshold * ROUNDING_INTEGER_MUL) / PERCENT_MAX;
		dsu->downsize_threshold *= dsu->static_leakage;
		dsu->downsize_threshold /= dsu->dram_energy_per_mb;
		dev_err(dev, "downsize_threshold = %lu\n", dsu->downsize_threshold);

		dsu->upsize_threshold = PERCENT_MAX - dsu->upsize_ratio;
		dsu->upsize_threshold = (dsu->upsize_threshold * ROUNDING_INTEGER_MUL) / PERCENT_MAX;
		dsu->upsize_threshold *= dsu->static_leakage;
		dsu->upsize_threshold /= dsu->dram_energy_per_mb;
		dev_err(dev, "upsize_threshold = %lu\n", dsu->upsize_threshold);

		dsu->max_threshold = ROUNDING_INTEGER_MUL;
		dsu->max_threshold *= dsu->static_leakage;
		dsu->max_threshold /= dsu->dram_energy_per_mb;
		dev_err(dev, "max_threshold = %lu\n", dsu->max_threshold);
		ret = count;
	}else{
		ret = -EINVAL;
	}
	mutex_unlock(&dsu->devfreq->lock);
	return ret;
}

static ssize_t show_static_leakage(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct dsu_pctrl *dsu = dev_get_drvdata(devfreq->dev.parent);

	return snprintf(buf, PAGE_SIZE, "%u\n", dsu->static_leakage_per_mb);	/* unsafe_function_ignore: snprintf */
}



static ssize_t store_dram_energy(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct dsu_pctrl *dsu = dev_get_drvdata(devfreq->dev.parent);

	unsigned long value = 0;
	int ret = 0;

	ret = sscanf(buf, "%lu", &value);	/* unsafe_function_ignore: sscanf */
	if (ret != 1)
		return -EINVAL;

	mutex_lock(&dsu->devfreq->lock);
	if (value > 0) {
		dsu->dram_energy_per_mb = value;
		/* Downsize and upsize thresholds are given in percentages */
		dsu->downsize_threshold = dsu->downsize_ratio;
		dsu->downsize_threshold = (dsu->downsize_threshold * ROUNDING_INTEGER_MUL) / PERCENT_MAX;
		dsu->downsize_threshold *= dsu->static_leakage;
		dsu->downsize_threshold /= dsu->dram_energy_per_mb;
		dev_err(dev, "downsize_threshold = %lu\n", dsu->downsize_threshold);

		dsu->upsize_threshold = PERCENT_MAX - dsu->upsize_ratio;
		dsu->upsize_threshold = (dsu->upsize_threshold * ROUNDING_INTEGER_MUL) / PERCENT_MAX;
		dsu->upsize_threshold *= dsu->static_leakage;
		dsu->upsize_threshold /= dsu->dram_energy_per_mb;
		dev_err(dev, "upsize_threshold = %lu\n", dsu->upsize_threshold);

		dsu->max_threshold = ROUNDING_INTEGER_MUL;
		dsu->max_threshold *= dsu->static_leakage;
		dsu->max_threshold /= dsu->dram_energy_per_mb;
		dev_err(dev, "max_threshold = %lu\n", dsu->max_threshold);
		ret = count;
	}else{
		ret = -EINVAL;
	}
	mutex_unlock(&dsu->devfreq->lock);
	return ret;
}

static ssize_t show_dram_energy(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct dsu_pctrl *dsu = dev_get_drvdata(devfreq->dev.parent);

	return snprintf(buf, PAGE_SIZE, "%u\n", dsu->dram_energy_per_mb);	/* unsafe_function_ignore: snprintf */
}


static ssize_t show_max_threshold(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct dsu_pctrl *dsu = dev_get_drvdata(devfreq->dev.parent);

	return snprintf(buf, PAGE_SIZE, "%lu\n", dsu->max_threshold);	/* unsafe_function_ignore: snprintf */
}



static ssize_t store_down_interval(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct dsu_pctrl *dsu = dev_get_drvdata(devfreq->dev.parent);

	unsigned long value = 0;
	int ret = 0;

	ret = sscanf(buf, "%lu", &value);	/* unsafe_function_ignore: sscanf */
	if (ret != 1)
		return -EINVAL;

	mutex_lock(&dsu->devfreq->lock);
	if (dsu->up_polling_ms && value > 0 && value <= 10) {
		dsu->down_interval = value;
		dsu->down_polling_ms = dsu->down_interval * devfreq->profile->polling_ms;
		ret = count;
	}else{
		ret = -EINVAL;
	}
	mutex_unlock(&dsu->devfreq->lock);
	return ret;
}

static ssize_t show_down_interval(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct dsu_pctrl *dsu = dev_get_drvdata(devfreq->dev.parent);
	ssize_t count = 0;
	int ret = 0;

	ret = snprintf(buf + count, (PAGE_SIZE - count), "down_interval = %u\n", dsu->down_interval); /* unsafe_function_ignore: snprintf */
	if (ret >= (PAGE_SIZE - count) || ret < 0) {	/*lint !e574 */
		goto err;
	}
	count += ret;

	ret = snprintf(buf + count, (PAGE_SIZE - count), "down_polling_ms = %u\n", dsu->down_polling_ms); /* unsafe_function_ignore: snprintf */
	if (ret >= (PAGE_SIZE - count) || ret < 0) {	/*lint !e574 */
		goto err;
	}
	count += ret;

err:
	return count;
}


static ssize_t store_power_control(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	unsigned long value = 0;
	unsigned long portion_active = 0;
	int ret = 0;

	ret = sscanf(buf, "%lu", &value);	/* unsafe_function_ignore: sscanf */
	if (ret != 1)
		return -EINVAL;

	portion_active = (1UL << value) - 1;
	set_pwr_ctlr(portion_active);

	return ret;
}

static ssize_t show_power_status(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	unsigned long val = 0;
	asm volatile("mrs %0,  s3_0_c15_c3_7" : "=r" (val));	/* CLUSTERPWRSTAT_EL1 */
	dev_err(dev, "power_status = 0x%lx\n", val);

	return snprintf(buf, PAGE_SIZE, "0x%lx\n", val);	/* unsafe_function_ignore: snprintf */
}
static DEVICE_ATTR(power, 0660, show_power_status, store_power_control);


#define DSU_PCTRL_ATTR_RW(_name) \
	static DEVICE_ATTR(_name, 0660, show_##_name, store_##_name)

#define DSU_PCTRL_ATTR_RO(_name) \
	static DEVICE_ATTR(_name, 0440, show_##_name, NULL)


DSU_PCTRL_ATTR_RW(downsize_ratio);
DSU_PCTRL_ATTR_RW(upsize_ratio);
DSU_PCTRL_ATTR_RW(static_leakage);
DSU_PCTRL_ATTR_RW(dram_energy);
DSU_PCTRL_ATTR_RO(max_threshold);
DSU_PCTRL_ATTR_RW(down_interval);

static struct attribute *dev_entries[] = {
	&dev_attr_downsize_ratio.attr,
	&dev_attr_upsize_ratio.attr,
	&dev_attr_static_leakage.attr,
	&dev_attr_dram_energy.attr,
	&dev_attr_max_threshold.attr,
	&dev_attr_down_interval.attr,
	&dev_attr_power.attr,
	NULL,
};

static struct attribute_group dev_attr_group = {
	.name	= "dsu-pctrl",
	.attrs	= dev_entries,
};
#endif


#ifdef CONFIG_HISI_L3CACHE_SHARE_PERF
static inline unsigned long read_event(struct event_data *event)
{
	unsigned long ev_count;
	u64 total, enabled, running;

	if (!event->pevent){
		pr_err("%s pevent is NULL\n", __func__);
		return 0;
	}

	total = perf_event_read_value(event->pevent, &enabled, &running);
	ev_count = total - event->prev_count;
	event->prev_count = total;
	return ev_count;

}

static void read_perf_counters(int cpu, struct perf_hwmon *hw)
{
	struct cpu_grp_info *cpu_grp = hw->cpu_grp;
	struct cpu_pmu_stats *cpustats = to_cpustats(cpu_grp, cpu);
	struct dev_stats *devstats = to_devstats(hw, cpu);

	devstats->l3d_refill_cnt = read_event(&cpustats->events[L3D_CACHE_REFILL_IDX]);
	devstats->l3d_cnt = read_event(&cpustats->events[L3D_CACHE_IDX]);
	//trace_dsu_pctrl_read_perf_counters(cpu, devstats->l3d_refill_cnt, devstats->l3d_cnt);
}


static unsigned long dsu_pctrl_get_cnt(struct perf_hwmon *hw)
{
	int cpu;
	struct cpu_grp_info *cpu_grp = hw->cpu_grp;
	struct dev_stats *devstats =  NULL;

	hw->total_refill= 0;
	hw->total_access = 0;
	for_each_cpu(cpu, &cpu_grp->inited_cpus){
		read_perf_counters(cpu, hw);
		devstats =  to_devstats(hw, cpu);
		hw->total_refill += devstats->l3d_refill_cnt;
		hw->total_access += devstats->l3d_cnt;
	}

	trace_dsu_pctrl_get_cnt(hw->total_refill, hw->total_access);

	return 0;
}


static void delete_events(struct cpu_pmu_stats *cpustats)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(cpustats->events); i++) {
		cpustats->events[i].prev_count = 0;
		if(cpustats->events[i].pevent){
			perf_event_disable(cpustats->events[i].pevent);
			perf_event_release_kernel(cpustats->events[i].pevent);
			cpustats->events[i].pevent = NULL;
		}
	}
}

static void dsu_pctrl_stop_hwmon(struct perf_hwmon *hw)
{
	int cpu;
	struct cpu_grp_info *cpu_grp = hw->cpu_grp;
	struct dev_stats *devstats;

	get_online_cpus();
	for_each_cpu(cpu, &cpu_grp->inited_cpus) {
		delete_events(to_cpustats(cpu_grp, cpu));

		devstats = to_devstats(hw, cpu);
		devstats->l3d_refill_cnt = 0;
		devstats->l3d_cnt = 0;
		hw->total_refill = 0;
		hw->total_access = 0;
	}
	mutex_lock(&list_lock);
	if (!cpumask_equal(&cpu_grp->cpus, &cpu_grp->inited_cpus))
		list_del(&cpu_grp->mon_list);
	mutex_unlock(&list_lock);
	cpumask_clear(&cpu_grp->inited_cpus);

	put_online_cpus();

	unregister_cpu_notifier(&cpu_grp->perf_cpu_notif);
}


static struct perf_event_attr *alloc_attr(void)
{
	struct perf_event_attr *attr;

	attr = kzalloc(sizeof(struct perf_event_attr), GFP_KERNEL);
	if (!attr)
		return attr;

	attr->type = PERF_TYPE_RAW;
	attr->size = sizeof(struct perf_event_attr);
	attr->pinned = 1;
	attr->exclude_idle = 1;

	return attr;
}

static int set_events(struct cpu_grp_info *cpu_grp, int cpu)
{
	struct perf_event *pevent;
	struct perf_event_attr *attr;
	int err;
	unsigned int i = 0, j = 0;
	struct cpu_pmu_stats *cpustats = to_cpustats(cpu_grp, cpu);

	/* Allocate an attribute for event initialization */
	attr = alloc_attr();
	if (!attr)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(cpustats->events); i++) {
		attr->config = cpu_grp->event_ids[i];
		pevent = perf_event_create_kernel_counter(attr, cpu, NULL,
							  NULL, NULL);
		if (IS_ERR(pevent))
			goto err_out;
		cpustats->events[i].pevent = pevent;
		perf_event_enable(pevent);
	}

	kfree(attr);
	return 0;

err_out:
	pr_err("dsu_pctrl: fail to create %d events\n", i);
	for (j = 0; j < i; j++) {
		perf_event_disable(cpustats->events[j].pevent);
		perf_event_release_kernel(cpustats->events[j].pevent);
		cpustats->events[j].pevent = NULL;
	}
	err = PTR_ERR(pevent);
	kfree(attr);
	return err;
}



static int dsu_pctrl_perf_cpu_callback(struct notifier_block *nb,
		unsigned long action, void *hcpu)
{
	unsigned long cpu = (unsigned long)hcpu;
	struct cpu_grp_info *cpu_grp, *tmp;

	if (action != CPU_ONLINE)
		return NOTIFY_OK;

	mutex_lock(&list_lock);
	list_for_each_entry_safe(cpu_grp, tmp, &perf_mon_list, mon_list) {
		if (!cpumask_test_cpu(cpu, &cpu_grp->cpus) ||
			cpumask_test_cpu(cpu, &cpu_grp->inited_cpus))
			continue;

		if (set_events(cpu_grp, cpu))
			pr_warn("Failed to create perf ev for CPU%lu\n", cpu);
		else
			cpumask_set_cpu(cpu, &cpu_grp->inited_cpus);

		if (cpumask_equal(&cpu_grp->cpus, &cpu_grp->inited_cpus)){
			list_del(&cpu_grp->mon_list);
			}
	}
	mutex_unlock(&list_lock);

	return NOTIFY_OK;
}


static int dsu_pctrl_start_hwmon(struct perf_hwmon *hw)
{
	int cpu, ret = 0;
	struct cpu_grp_info *cpu_grp = hw->cpu_grp;

	register_cpu_notifier(&cpu_grp->perf_cpu_notif);

	get_online_cpus();
	for_each_cpu(cpu, &cpu_grp->cpus) {
		ret = set_events(cpu_grp, cpu);
		if (ret) {
			if (!cpu_online(cpu)) {
				ret = 0;
			} else {
				pr_warn("Perf event init failed on CPU%d\n", cpu);
				break;
			}
		} else {
			cpumask_set_cpu(cpu, &cpu_grp->inited_cpus);
		}
	}
	mutex_lock(&list_lock);
	if (!cpumask_equal(&cpu_grp->cpus, &cpu_grp->inited_cpus)){
		list_add_tail(&cpu_grp->mon_list, &perf_mon_list);
	}
	mutex_unlock(&list_lock);

	put_online_cpus();
	return ret;
}


static int dsu_pctrl_hwmon_setup(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
	struct perf_hwmon *hw = NULL;
	struct cpu_grp_info *cpu_grp = NULL;
	int ret = 0;

	hw = devm_kzalloc(dev, sizeof(*hw), GFP_KERNEL);
	if (!hw)
		return -ENOMEM;
	dsu->hw = hw;

	cpu_grp = devm_kzalloc(dev, sizeof(*cpu_grp), GFP_KERNEL);
	if (!cpu_grp)
		return -ENOMEM;
	hw->cpu_grp = cpu_grp;

	cpu_grp->perf_cpu_notif.notifier_call = dsu_pctrl_perf_cpu_callback;

	cpumask_copy(&cpu_grp->cpus, cpu_online_mask);
	cpumask_copy(&hw->cpus, &cpu_grp->cpus);

	hw->num_cores = cpumask_weight(&cpu_grp->cpus);
	hw->core_stats = devm_kzalloc(dev, hw->num_cores *
				sizeof(*(hw->core_stats)), GFP_KERNEL);
	if (!hw->core_stats)
		return -ENOMEM;

	cpu_grp->cpustats = devm_kzalloc(dev, hw->num_cores *
		sizeof(*(cpu_grp->cpustats)), GFP_KERNEL);
	if (!cpu_grp->cpustats)
		return -ENOMEM;

	cpu_grp->event_ids[L3D_CACHE_REFILL_IDX] = L3D_CACHE_REFILL_EV;
	cpu_grp->event_ids[L3D_CACHE_IDX] = L3D_CACHE_EV;

	ret =  dsu_pctrl_start_hwmon(hw);
	if(ret){
		dev_err(dev, "cannot start hwmon hw.\n");
		goto err;
	}

	return ret;

err:
	dsu_pctrl_stop_hwmon(hw);
	return ret;
}
#endif


static int dsu_pctrl_set_active_portions(struct device *dev,
					 unsigned long *portions)
{
	struct dsu_pctrl *dsu = dev_get_drvdata(dev);
	struct dsu_pctrl_data *data = dsu->dsu_data;
	unsigned long portion_active;
	//unsigned long portion_control;

	if (*portions < data->portion_min) {
		dev_warn(dev, "%s: Target %lu < min = %u. Target set to min.\n",
			 __func__, *portions, data->portion_min);
		*portions = data->portion_min;
	} else if (*portions > data->portion_max) {
		dev_warn(dev, "%s: Target %lu > max = %u. Target set to max.\n",
			 __func__, *portions, data->portion_max);
		*portions = data->portion_max;
	}

	if (*portions == dsu->cur_num_portions) {
		return 0;
	}

	/*
	 * Considering that the cache data ram can only be powered on/off in
	 * portions of 50% of the cache while the tag rams can be powered
	 * on/off in portions of 25% of the cache, results in the vast majority
	 * of the leakage being no different at 1 and 2 portions, and likewise
	 * at 3 and 4 portions.
	 * The current powerdown register interface is linked to the tag ram
	 * split of portions, but for performance reasons we'll consider 1
	 * portion as the equivalent of 50% of the cache and write the
	 * powerdown register accordingly.
	 *
	 * Set the number of portions in the DSU to portions
	 *
	 * portions	   Set of bit-fields to Enable
	 * ---------	   ---------------------------
	 * 2 - fully on	   PORTION_1|PORTION_2|PORTION_3|PORTION_4
	 * 1 - half on	   PORTION_1|PORTION_2
	 * 0 - fully off   <none>
	 */
#ifdef CONFIG_HISI_L3CACHE_SHARE_PERF
	mutex_lock(&l3c_acp_lock);
	if (true == req_pending_flag && *portions <= FORBID_SHARE_NUM){
		if (false == acp_enable_flag){
			acp_enable_flag = true;
			l3_cache_enable_acp(dsu->l3share_id, dsu->l3share_size);
			dev_dbg(dev, "enable acp id %d\n", dsu->l3share_id);
			trace_dsu_pctrl_target_acp("enable acp", dsu->l3share_id);
		}
	}else if (true == acp_enable_flag && *portions > FORBID_SHARE_NUM){
			acp_enable_flag = false;
			req_pending_flag = true;
			l3_cache_disable_acp(dsu->l3share_id);
			dev_dbg(dev, "disable acp id %d\n", dsu->l3share_id);
			trace_dsu_pctrl_target_acp("disable acp", dsu->l3share_id);
	}
	mutex_unlock(&l3c_acp_lock);
#endif

	portion_active = (1UL << *portions) - 1;		//[1, 4]
	set_pwr_ctlr(portion_active);

	trace_dsu_pctrl_set_active_portions(*portions, portion_active);
	trace_clock_set_rate("dsu_pctrl", *portions, raw_smp_processor_id());

	/* Update current number of portions */
	dsu->cur_num_portions = *portions;

	return 0;
}

static int dsu_pctrl_devfreq_target(struct device *dev,
				    unsigned long *portions, u32 flags)
{
	/* Set requested portions */
	return dsu_pctrl_set_active_portions(dev, portions);
}

static int dsu_pctrl_devfreq_get_dev_status(struct device *dev,
					    struct devfreq_dev_status *stat)
{
	struct dsu_pctrl *dsu = dev_get_drvdata(dev);
	unsigned int const usec = ktime_to_us(ktime_get());
	unsigned int delta;
	unsigned long hits = 0;
	unsigned long misses = 0;
	unsigned long accesses;
#ifndef CONFIG_HISI_L3CACHE_SHARE_PERF
	unsigned long total = 0;
#endif

	delta = usec - dsu->alg.last_update;
	dsu->alg.last_update = usec;

	stat->current_frequency = dsu->cur_num_portions;

#ifdef CONFIG_HISI_L3CACHE_SHARE_PERF
	dsu_pctrl_get_cnt(dsu->hw);
#else
	total = get_l3_hit_miss_count();
#endif

#ifdef CONFIG_HISI_L3CACHE_SHARE_PERF
	misses = dsu->hw->total_refill;
	accesses = dsu->hw->total_access;
	hits = accesses - misses;
#else
	hits = total & MAX_COUNT_VAL;
	misses = (total >> MAX_COUNT_LEN) & MAX_COUNT_VAL;
	accesses = hits + misses;
#endif

	dsu->alg.accesses_up   += accesses;
	dsu->alg.accesses_down += accesses;
	dsu->alg.misses_up     += misses;
	dsu->alg.misses_down   += misses;
	dsu->alg.usec_up       += delta;
	dsu->alg.usec_down     += delta;

	if (!accesses)
		accesses = 1;

	/*
	 * Although this is not yet suitable for use with the simple ondemand
	 * governor we'll fill these usage statistics.
	 */
	stat->total_time = delta;
	stat->busy_time  = stat->total_time * hits / accesses;

	trace_dsu_pctrl_dev_status(dsu->id, hits, misses,
				   dsu->cur_num_portions, stat->busy_time,
				   stat->total_time);

	return 0;
}

static int dsu_pctrl_up_size_check(struct dsu_pctrl *dsu)
{
	struct dsu_pctrl_data *data = dsu->dsu_data;
	unsigned long cache_miss_bw;
	int ret = 0;

	/*
	 * If we are at the maximum number of active portions and we know
	 * there won't be a downsize attempt due to polling time constraints,
	 * we can skip calculations that won't lead to any action.
	 */
	if ((dsu->cur_num_portions >= data->portion_max) &&
	    (dsu->alg.usec_down < dsu->down_polling_ms * 1000)){ //lint !e647
			goto cleanup;
		}

	cache_miss_bw = dsu->line_size * dsu->alg.misses_up;
	cache_miss_bw *= ROUNDING_INTEGER_MUL;
	cache_miss_bw /= dsu->alg.usec_up;

	if (cache_miss_bw > dsu->upsize_threshold)
		ret = 1;

cleanup:
	dsu->alg.usec_up = 0;
	dsu->alg.misses_up = 0;
	dsu->alg.accesses_up = 0;

	return ret;
}

static int dsu_pctrl_down_size_check(struct dsu_pctrl *dsu)
{
	struct dsu_pctrl_data *data = dsu->dsu_data;
	unsigned long cache_hit_bw;
	int ret = 0;

	if (dsu->cur_num_portions <= data->portion_min)
		goto cleanup;

	cache_hit_bw = dsu->alg.accesses_down - dsu->alg.misses_down;
	cache_hit_bw *= dsu->line_size;
	cache_hit_bw *= ROUNDING_INTEGER_MUL;
	cache_hit_bw /= dsu->alg.usec_down;

	if (cache_hit_bw < (dsu->max_threshold * dsu->cur_num_portions -
			    dsu->downsize_threshold))
		ret = 1;

cleanup:
	dsu->alg.usec_down = 0;
	dsu->alg.misses_down = 0;
	dsu->alg.accesses_down = 0;

	return ret;
}

static unsigned long dsu_pctrl_calc_next_portions(struct dsu_pctrl *dsu)
{
	unsigned long portions = dsu->cur_num_portions;
	int upsize = 0, downsize = 0;

	upsize = dsu_pctrl_up_size_check(dsu);
	if (upsize > 0)
		return dsu->cur_num_portions + upsize;

	if (dsu->alg.usec_down >= dsu->down_polling_ms * 1000) { //lint !e647
		downsize = dsu_pctrl_down_size_check(dsu);
		if (downsize > 0)
			portions = dsu->cur_num_portions - downsize;
	}

	return portions;
}

static int dsu_pctrl_governor_get_target_portions(struct devfreq *df,
						  unsigned long *portions)
{
	struct dsu_pctrl *dsu = dev_get_drvdata(df->dev.parent);
	struct dsu_pctrl_data *data = dsu->dsu_data;
	int err;
	err = devfreq_update_stats(df);
	if (err)
		return err;

	*portions = dsu_pctrl_calc_next_portions(dsu);

	if (*portions < data->portion_min) {
		*portions = data->portion_min;
	} else if (*portions > data->portion_max) {
		*portions = data->portion_max;
	}

	return 0;
}


#define MIN_MS	10U
#define MAX_MS	500U
static int dsu_pctrl_governor_event_handler(struct devfreq *devfreq,
					    unsigned int event, void *data)
{
	int ret = 0;
	unsigned int sample_ms;

	switch (event) {
	case DEVFREQ_GOV_START:
		//devfreq_monitor_start(devfreq);
		break;

	case DEVFREQ_GOV_STOP:
		//devfreq_monitor_stop(devfreq);
		break;

	case DEVFREQ_GOV_SUSPEND:
		devfreq_monitor_suspend(devfreq);
		break;

	case DEVFREQ_GOV_RESUME:
		devfreq_monitor_resume(devfreq);
		break;

	case DEVFREQ_GOV_INTERVAL:
		sample_ms = *(unsigned int *)data;
		sample_ms = max(MIN_MS, sample_ms);
		sample_ms = min(MAX_MS, sample_ms);

		mutex_lock(&devfreq->lock);
		devfreq->profile->polling_ms = sample_ms;
		mutex_unlock(&devfreq->lock);
		break;
	}

	return ret;
}

static struct devfreq_governor dsu_pctrl_devfreq_governor = {
	.name		 = DSU_PCTRL_GOVERNOR_NAME,
	.immutable = 1,
	.get_target_freq = dsu_pctrl_governor_get_target_portions,
	.event_handler	 = dsu_pctrl_governor_event_handler,
};

#ifdef DSU_PCTRL_HRTIMER_ENABLE
static void dsu_pctrl_handle_update(struct work_struct *work)
{
	struct dsu_pctrl *dsu = container_of(work, struct dsu_pctrl,
					     update_handle);

	mutex_lock(&dsu->devfreq->lock);
	update_devfreq(dsu->devfreq);
	mutex_unlock(&dsu->devfreq->lock);
}

static enum hrtimer_restart dsu_pctrl_polling(struct hrtimer *hrtimer)
{
	struct dsu_pctrl *dsu = container_of(hrtimer, struct dsu_pctrl,
					     poll_timer);

	queue_work(dsu->update_wq, &dsu->update_handle);

	hrtimer_forward_now(&dsu->poll_timer,
			    ms_to_ktime(dsu->up_polling_ms));
	return HRTIMER_RESTART;
}
#endif

static int dsu_pctrl_reinit_device(struct device *dev)
{
	struct dsu_pctrl *dsu = dev_get_drvdata(dev);

	/* Clean the algorithm statistics and start from scrach */
	memset(&dsu->alg, 0, sizeof(dsu->alg));	/* unsafe_function_ignore: memset */
	dsu->alg.last_update = ktime_to_us(ktime_get());

	return 0;

	//return dsu_pctrl_set_active_portions(dev, &(dsu->initial_freq));
}

static int dsu_pctrl_setup_devfreq_profile(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
	struct devfreq_dev_profile *df_profile;

	dsu->devfreq_profile = devm_kzalloc(&pdev->dev,
			sizeof(struct devfreq_dev_profile), GFP_KERNEL);
	if (IS_ERR(dsu->devfreq_profile)) {
		dev_err(&pdev->dev, "no memory.\n");
		return PTR_ERR(dsu->devfreq_profile);
	}

	df_profile = dsu->devfreq_profile;

	df_profile->target = dsu_pctrl_devfreq_target;
	df_profile->get_dev_status = dsu_pctrl_devfreq_get_dev_status;
	df_profile->freq_table = dsu->freq_table;
	df_profile->max_state = dsu->freq_table_len;
	df_profile->polling_ms = dsu->up_polling_ms;
	//df_profile->polling_ms = DSU_PCTRL_DEFAULT_REQUIRED_GOV_POLLING;
	df_profile->initial_freq = dsu->initial_freq;

	return 0;
}

static int dsu_pctrl_parse_dt(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
	int ret = 0;

	of_node_get(node);

	ret = of_property_read_u32(node, "static-leakage-per-mb",
				   &dsu->static_leakage_per_mb);
	if (ret)
		dsu->static_leakage_per_mb = DSU_PCTRL_DEFAULT_CACHE_LEAKAGE;

	ret = of_property_read_u32(node, "dram-energy-per-mb",
				   &dsu->dram_energy_per_mb);
	if (ret)
		dsu->dram_energy_per_mb = DSU_PCTRL_DEFAULT_DRAM_ENERGY_PER_MB;

	ret = of_property_read_u32(node, "size", &dsu->size);
	if (ret)
		dsu->size = DSU_PCTRL_DEFAULT_SIZE_KB;

	ret = of_property_read_u32(node, "line-size", &dsu->line_size);
	if (ret)
		dsu->line_size = DSU_PCTRL_DEFAULT_LINE_SIZE;

	ret = of_property_read_u32(node, "polling", &dsu->up_polling_ms);
	if (ret)
		dsu->up_polling_ms = DSU_PCTRL_DEFAULT_POLLING_MS;
	dev_info(&pdev->dev, "up_polling_ms = %d\n", dsu->up_polling_ms);

	ret = of_property_read_u32(node, "down-interval", &dsu->down_interval);
	if (ret)
		dsu->down_interval = POLLING_DOWN_INTERVAL;
	dev_info(&pdev->dev, "down_interval = %d\n", dsu->down_interval);

	dsu->down_polling_ms = dsu->down_interval * dsu->up_polling_ms;
	dev_info(&pdev->dev, "down_polling_ms = %d\n", dsu->down_polling_ms);

	ret = of_property_read_u32(node, "downsize-portion-threshold", &dsu->downsize_ratio);
	if (ret)
		dsu->downsize_ratio = DOWNSIZE_PORTION_THRESHOLD;
	dev_info(&pdev->dev, "downsize_ratio = %d\n", dsu->downsize_ratio);

	ret = of_property_read_u32(node, "upsize-portion-threshold", &dsu->upsize_ratio);
	if (ret)
		dsu->upsize_ratio = UPSIZE_PORTION_THRESHOLD;
	dev_info(&pdev->dev, "upsize_ratio = %d\n", dsu->upsize_ratio);

	of_node_put(node);

	return 0;
}

static int dsu_pctrl_create_configuration(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
	struct dsu_pctrl_data *data = dsu->dsu_data;
	int i;
	unsigned long portion;

	dsu->initial_freq = data->portion_max;
	dsu->cur_num_portions = dsu->initial_freq;

	dsu->freq_table_len = data->portion_max - data->portion_min + 1;
	dsu->freq_table = devm_kcalloc(&pdev->dev, dsu->freq_table_len,
				       sizeof(*dsu->freq_table),
				       GFP_KERNEL);
	if (IS_ERR_OR_NULL(dsu->freq_table))
		return -ENOMEM;

	portion = data->portion_min;
	for (i = 0; portion <= data->portion_max; i++) //lint !e440
		dsu->freq_table[i] = portion++;

	/* Leakage (static power) for a single portion (in uW) */
	dsu->static_leakage = dsu->static_leakage_per_mb * dsu->size; //lint !e647
	dsu->static_leakage /= SZ_1KB;
	dsu->static_leakage /= data->portion_max;
	dev_info(&pdev->dev, "static_leakage = %lu\n", dsu->static_leakage);

	/* Downsize and upsize thresholds are given in percentages */
	dsu->downsize_threshold = dsu->downsize_ratio;
	dsu->downsize_threshold = (dsu->downsize_threshold * ROUNDING_INTEGER_MUL) / PERCENT_MAX;
	dsu->downsize_threshold *= dsu->static_leakage;
	dsu->downsize_threshold /= dsu->dram_energy_per_mb;
	dev_info(&pdev->dev, "downsize_threshold = %lu\n", dsu->downsize_threshold);

	dsu->upsize_threshold = PERCENT_MAX - dsu->upsize_ratio;
	dsu->upsize_threshold = (dsu->upsize_threshold * ROUNDING_INTEGER_MUL) / PERCENT_MAX;
	dsu->upsize_threshold *= dsu->static_leakage;
	dsu->upsize_threshold /= dsu->dram_energy_per_mb;
	dev_info(&pdev->dev, "upsize_threshold = %lu\n", dsu->upsize_threshold);

	dsu->max_threshold = ROUNDING_INTEGER_MUL;
	dsu->max_threshold *= dsu->static_leakage;
	dsu->max_threshold /= dsu->dram_energy_per_mb;
	dev_info(&pdev->dev, "max_threshold = %lu\n", dsu->max_threshold);

	return 0;
}

static void dsu_pctrl_remove_opps(struct platform_device *pdev)
{
	struct dev_pm_opp *opp;
	int i, count;
	unsigned long freq;

	count = dev_pm_opp_get_opp_count(&pdev->dev);
	if (count <= 0)
		return;

	rcu_read_lock();
	for (i = 0, freq = 0; i < count; i++, freq++) {
		opp = dev_pm_opp_find_freq_ceil(&pdev->dev, &freq);
		if (!IS_ERR(opp))
			dev_pm_opp_remove(&pdev->dev, freq);
	}
	rcu_read_unlock();
}

static int dsu_pctrl_enable_opps(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
	int i, ret;
	int opp_count = 0;

	if (dsu->freq_table_len <= 0)
		return -EINVAL;

	for (i = 0; i < dsu->freq_table_len; i++) {
		ret = dev_pm_opp_add(&pdev->dev, dsu->freq_table[i],
				     dsu->freq_table[i]);
		if (ret){
			dev_err(&pdev->dev, "cannot add a new OPP.\n");
			while (i-- > 0){
					dev_pm_opp_remove(&pdev->dev, dsu->freq_table[i]);
					opp_count--;
			}
			break;
		}
		else
			opp_count++;
	}

	if (opp_count == 0) {
		dev_err(&pdev->dev, "device has no OPP registered.\n");
		return -ENODEV;
	}

	return 0;
}

static int dsu_pctrl_setup(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
	unsigned long variant = 0;
	int ret = 0;

	/* Extract DSU variant */
	SYS_REG_READ(S3_0_c15_c3_1, variant);
	variant &= CLUSTERIDR_VARIANT_MASK;
	variant = variant >> CLUSTERIDR_VARIANT_BASE;

	/*
	 * Set implementation specific min and max limits:
	 * Variant R0 will match behaviour for ARM_DSU_R0
	 * All other variants will match behaviour for ARM_DSU_R1
	 */
	variant = !!variant;
	//dev_err(&pdev->dev, "variant is %lu.\n", variant);
	dsu->dsu_data = (struct dsu_pctrl_data *)&device_data[variant];

#ifdef DSU_PCTRL_HRTIMER_ENABLE
	dsu->update_wq = create_workqueue("arm_dsu_pctrl_wq");
	if (IS_ERR(dsu->update_wq)) {
		dev_err(&pdev->dev, "cannot create workqueue.\n");
		return PTR_ERR(dsu->update_wq);
	}
	INIT_WORK(&dsu->update_handle, dsu_pctrl_handle_update);
#endif

	mutex_init(&dsu->lock);

	ret = dsu_pctrl_create_configuration(pdev);
	if (ret) {
		dev_err(&pdev->dev, "cannot create configuration.\n");
		return ret;
	}

	ret = dsu_pctrl_enable_opps(pdev);
	if (ret) {
		dev_err(&pdev->dev, "device enable opps failed.\n");
		return ret;
	}

	ret = dsu_pctrl_setup_devfreq_profile(pdev);
	if (ret) {
		dev_err(&pdev->dev, "device setup devfreq profile failed.\n");
		goto remove_opps;
	}

	dsu->alg.last_update = ktime_to_us(ktime_get());

	dsu->devfreq = devm_devfreq_add_device(&pdev->dev,
					       dsu->devfreq_profile,
					       DSU_PCTRL_GOVERNOR_NAME, NULL);

	if (IS_ERR(dsu->devfreq)) {
		dev_err(&pdev->dev, "registering to devfreq failed.\n");
		ret = PTR_ERR(dsu->devfreq);
		goto remove_opps;
	}

	mutex_lock(&dsu->devfreq->lock);
	dsu->devfreq->min_freq = dsu->dsu_data->portion_min;
	dsu->devfreq->max_freq = dsu->dsu_data->portion_max;
	mutex_unlock(&dsu->devfreq->lock);

	return 0;

remove_opps:
	dsu_pctrl_remove_opps(pdev);
	return ret;
}


static void dsu_pctrl_unsetup(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);

	devm_devfreq_remove_device(&pdev->dev, dsu->devfreq);
	dsu_pctrl_remove_opps(pdev);
}


static int dsu_pctrl_init_device(struct platform_device *pdev)
{
#ifdef DSU_PCTRL_HRTIMER_ENABLE
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
#endif
	dsu_pctrl_reinit_device(&pdev->dev);


#ifdef DSU_PCTRL_HRTIMER_ENABLE
	hrtimer_init(&dsu->poll_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	dsu->poll_timer.function = dsu_pctrl_polling;
	hrtimer_start(&dsu->poll_timer, ms_to_ktime(dsu->up_polling_ms),
		      HRTIMER_MODE_REL);
#endif

	return 0;
}



#ifdef CONFIG_HISI_L3CACHE_SHARE_PERF
static int l3c_acp_callback(struct notifier_block *nb,
		unsigned long mode, void *data)
{
	struct dsu_pctrl *dsu = container_of(nb, struct dsu_pctrl,
					     l3c_acp_notify);

	struct l3_cache_request_params *l3share;
	unsigned long portion_active = 0xF;
	unsigned long portions = dsu->dsu_data->portion_max;

	if (IS_ERR_OR_NULL(data)){
		pr_err("%s data is NULL\n", __func__);
		return NOTIFY_OK;
	}

	l3share = (struct l3_cache_request_params *) data;

	switch (mode) {
		case L3C_ACP_PENDING:
			dsu->l3share_id = l3share->id;
			dsu->l3share_size = l3share->request_size;
			devfreq_monitor_start(dsu->devfreq);
			pr_debug("%s start devfreq monitor\n", __func__);
			break;
		case L3C_ACP_RELEASE:
			devfreq_monitor_stop(dsu->devfreq);
			portions = dsu->dsu_data->portion_max;
			portion_active = (1UL << portions) - 1;
			set_pwr_ctlr(portion_active);
			dsu->cur_num_portions = portions;
			pr_debug("%s stop devfreq monitor, portion_active = 0x%lx\n", __func__, portion_active);
			break;
		default:
			pr_err("%s error case %lu\n", __func__, mode);
			break;
	}

	return NOTIFY_OK;
}
#endif


/*lint -e429*/
static int dsu_pctrl_devfreq_probe(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu;
	int ret = 0;

	dev_err(&pdev->dev, "registering DSU portion control device.\n");

	dsu = devm_kzalloc(&pdev->dev, sizeof(*dsu), GFP_KERNEL);
	if (!dsu)
		return -ENOMEM;

	platform_set_drvdata(pdev, dsu);
	dsu->pdev = pdev;
	dsu->id = atomic_inc_return(&dsu_pctrl_device_id);

	ret = dsu_pctrl_parse_dt(pdev);
	if (ret){
		dev_err(&pdev->dev, "parse dt failed\n");
		goto failed;
	}

#ifdef CONFIG_HISI_L3CACHE_SHARE_PERF
	ret = dsu_pctrl_hwmon_setup(pdev);
	if (ret){
		dev_err(&pdev->dev, "hwmon setup failed\n");
		goto failed;
	}
#endif

	ret = dsu_pctrl_setup(pdev);
	if (ret){
		dev_err(&pdev->dev, "setup failed\n");
		goto hwmon_unsetup;
	}

	ret = dsu_pctrl_init_device(pdev);
	if (ret){
		dev_err(&pdev->dev, "init device failed\n");
		goto unsetup;
	}

#ifdef CONFIG_HISI_L3CACHE_SHARE_PERF
	dsu->l3c_acp_notify.notifier_call = l3c_acp_callback;
	ret = register_l3c_acp_notifier(&dsu->l3c_acp_notify);
	if (ret) {
		dev_err(&pdev->dev, "register l3c acp notifier failed!\n");
		goto unsetup;
	}
#endif

#ifdef CONFIG_HISI_DSU_PCTRL_DEBUG
	ret = sysfs_create_group(&dsu->devfreq->dev.kobj, &dev_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "sysfs create err %d\n", ret);
		goto unreg_acp;
	}
#endif

	dev_err(&pdev->dev, "DSU portion control device registered.\n");

	return 0;

unreg_acp:
#ifdef CONFIG_HISI_L3CACHE_SHARE_PERF
	unregister_l3c_acp_notifier(&dsu->l3c_acp_notify);
#endif
unsetup:
	dsu_pctrl_unsetup(pdev);
hwmon_unsetup:
#ifdef CONFIG_HISI_L3CACHE_SHARE_PERF
	dsu_pctrl_stop_hwmon(dsu->hw);
#endif
failed:
	dev_err(&pdev->dev, "failed to register driver, err %d.\n", ret);
	return ret;
}
/*lint +e429*/

static int dsu_pctrl_devfreq_remove(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
	int ret = 0;

	dev_info(&pdev->dev, "unregistering DSU portion control device.\n");

	ret = dsu_pctrl_set_active_portions(&pdev->dev, &(dsu->initial_freq));
#ifdef DSU_PCTRL_HRTIMER_ENABLE
	/* Cancel hrtimer */
	if (hrtimer_active(&dsu->poll_timer))
		hrtimer_cancel(&dsu->poll_timer);

	/* Wait for pending work */
	flush_workqueue(dsu->update_wq);
	/* Destroy workqueue */
	destroy_workqueue(dsu->update_wq);
#endif

#ifdef CONFIG_HISI_DSU_PCTRL_DEBUG
	sysfs_remove_group(&dsu->devfreq->dev.kobj, &dev_attr_group);
#endif

#ifdef CONFIG_HISI_L3CACHE_SHARE_PERF
	unregister_l3c_acp_notifier(&dsu->l3c_acp_notify);
#endif

	devm_devfreq_remove_device(&pdev->dev, dsu->devfreq);
	dsu_pctrl_remove_opps(pdev);

#ifdef CONFIG_HISI_L3CACHE_SHARE_PERF
	dsu_pctrl_stop_hwmon(dsu->hw);
#endif

	return ret;
}

MODULE_DEVICE_TABLE(of, dsu_pctrl_devfreq_id);

static struct platform_driver dsu_pctrl_devfreq_driver = {
	.probe	= dsu_pctrl_devfreq_probe,
	.remove = dsu_pctrl_devfreq_remove,
	.driver = {
		.name = DSU_PCTRL_PLATFORM_DEVICE_NAME,
		.of_match_table = dsu_pctrl_devfreq_id,
		//.pm = &dsu_pctrl_pm,
		.owner = THIS_MODULE,
	},
};

static int __init dsu_pctrl_devfreq_init(void)
{
	int ret = 0;

	ret = devfreq_add_governor(&dsu_pctrl_devfreq_governor);
	if (ret) {
		pr_err("%s: failed to add governor: %d.\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&dsu_pctrl_devfreq_driver);
	if (ret){
		ret = devfreq_remove_governor(&dsu_pctrl_devfreq_governor);
		if(ret){
			pr_err("%s: failed to remove governor: %d.\n", __func__, ret);
		}
	}

	return ret;
}

static void __exit dsu_pctrl_devfreq_exit(void)
{
	int ret;

	ret = devfreq_remove_governor(&dsu_pctrl_devfreq_governor);
	if (ret)
		pr_err("%s: failed to remove governor: %d.\n", __func__, ret);

	platform_driver_unregister(&dsu_pctrl_devfreq_driver);
}

module_init(dsu_pctrl_devfreq_init)
module_exit(dsu_pctrl_devfreq_exit)

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ARM DSU PCTRL devfreq driver");
MODULE_AUTHOR("ARM Ltd.");
MODULE_VERSION("1.0");
