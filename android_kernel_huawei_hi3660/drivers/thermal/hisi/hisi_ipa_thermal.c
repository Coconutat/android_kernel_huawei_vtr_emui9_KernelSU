/* Copyright (c) 2008-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
*  Porting from ARM by qushenzheng <qushenzheng@hisilicon.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#include <linux/cpu_cooling.h>
#include <linux/debugfs.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/topology.h>
#include <trace/events/thermal_power_allocator.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/kthread.h>
#include "securec.h"

#ifdef CONFIG_HISI_THERMAL_SPM
#include <linux/string.h>
#endif
#define IPA_SENSOR "tsens_max"
#define IPA_SENSOR_SYSTEM_H "system_h"
#define IPA_SENSOR_MAXID    255
#define IPA_INIT_OK  0x05a5a5a5b
#ifdef CONFIG_HISI_THERMAL_TRIPPLE_CLUSTERS
#define NUM_CLUSTERS 3
#else
#define NUM_CLUSTERS 2
#endif
#define IPA_SOC_INIT_TEMP  (85000)
#ifdef CONFIG_HISI_THERMAL_SPM
#define MAX_SHOW_STR 5
int cpufreq_update_policies(void);
#endif
#ifdef CONFIG_HISI_THERMAL_SHELL
#define NUM_SENSORS	3
#else
#define NUM_SENSORS	2
#endif
#define NUM_TEMP_SCALE_CAPS 5
#define NUM_TZD 2
#define NUM_BOARD_CDEV 3

typedef int (*ipa_get_sensor_id_t)(const char *);

/*lint -e749 -esym(749,*)*/
enum cluster_type {
	CLUSTER_LITTLE = 0,
	CLUSTER_BIG
};
/*lint -e749 +esym(749,*)*/
#ifdef CONFIG_HISI_THERMAL_SPM
struct spm_t {
	struct device *device;
	bool spm_mode;
	bool vr_mode;
};
#endif

enum {
	SOC = 0,
	BOARD
};

struct capacitances {
	u32 cluster_dyn_capacitance[NUM_CLUSTERS];
	u32 cluster_static_capacitance[NUM_CLUSTERS];
	u32 cache_capacitance[NUM_CLUSTERS];
	const char *temperature_scale_capacitance[NUM_TEMP_SCALE_CAPS];
	bool initialized;
};

struct ipa_sensor {
	u32 sensor_id;
	s32 prev_temp;
	int alpha;
};

struct ipa_thermal {
	struct ipa_sensor ipa_sensor;
	struct thermal_zone_device *tzd;
	int cdevs_num;
	struct thermal_cooling_device **cdevs;
	struct capacitances *caps;
	int init_flag;
};

struct ipa_sensor_info {
	const char *ipa_sensor_name;
	ipa_get_sensor_id_t ipa_get_sensor_id;
};

#ifdef CONFIG_HISI_THERMAL_HOTPLUG
#define MAX_MODE_LEN 15
struct hotplug_t {
	struct device *device;
	int cpu_down_threshold;
	int cpu_up_threshold;
	bool need_down;
	bool need_up;
	long current_temp;
#ifdef CONFIG_HISI_HOTPLUG_EMULATION
	int emul_temp;
#endif
	bool cpu_downed;
	struct task_struct *hotplug_task;
	spinlock_t hotplug_lock;
	bool disabled;
	bool initialized;
};
#endif

struct thermal {
#if defined(CONFIG_HISI_THERMAL_SPM) || defined(CONFIG_HISI_THERMAL_HOTPLUG)
	struct class *hisi_thermal_class;
#endif
	struct ipa_thermal ipa_thermal[NUM_TZD];
#ifdef CONFIG_HISI_THERMAL_SPM
	struct spm_t spm;
#endif
#ifdef CONFIG_HISI_THERMAL_HOTPLUG
	struct hotplug_t hotplug;
#endif
};

struct capacitances g_caps;
struct thermal thermal_info;

extern int ipa_get_sensor_value(u32 sensor, int *val);
extern int ipa_get_periph_value(u32 sensor, int *val);
extern int ipa_get_tsensor_id(char *name);
extern int ipa_get_periph_id(char *name);
#ifdef CONFIG_HISI_THERMAL_SHELL
extern int hisi_get_shell_temp(struct thermal_zone_device *thermal, int *temp);
#endif
struct ipa_sensor_info ipa_sensor_info[NUM_SENSORS] = {
													{
														.ipa_sensor_name = IPA_SENSOR,
														.ipa_get_sensor_id = (ipa_get_sensor_id_t)ipa_get_tsensor_id
													},
#ifdef CONFIG_HISI_THERMAL_SHELL
													{
														.ipa_sensor_name = IPA_SENSOR_SHELL,
														.ipa_get_sensor_id = (ipa_get_sensor_id_t)ipa_get_tsensor_id
													},
#endif
													{
														.ipa_sensor_name = IPA_SENSOR_SYSTEM_H,
														.ipa_get_sensor_id = (ipa_get_sensor_id_t)ipa_get_periph_id
													}
};

typedef u32 (*get_power_t)(void);
u32 get_camera_power(void);
u32 get_backlight_power(void);
u32 get_charger_power(void);

get_power_t get_board_power[NUM_BOARD_CDEV] = {
	get_camera_power,
	get_backlight_power,
	get_charger_power
};

struct thermal_cooling_device *board_power_cooling_register(struct device_node *np, get_power_t get_power);
void board_cooling_unregister(struct thermal_cooling_device *cdev);

#ifdef CONFIG_HISI_HOTPLUG_EMULATION
static ssize_t
hotplug_emul_temp_store(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	int temperature;

	if (dev == NULL || attr == NULL)
		return -EINVAL;

	if (kstrtoint(buf, 10, &temperature))
		return -EINVAL;

#ifdef CONFIG_HISI_IPA_THERMAL
	temperature = thermal_zone_temp_check(temperature);
#endif
	thermal_info.hotplug.emul_temp = temperature;
	pr_err("hotplug emul temp set : %d\n", temperature);

	return (long)count;
}

/*lint -e84 -e846 -e514 -e778 -e866 -esym(84,846,514,778,866,*)*/
static DEVICE_ATTR(hotplug_emul_temp, S_IWUSR, NULL, hotplug_emul_temp_store);
/*lint -e84 -e846 -e514 -e778 -e866 +esym(84,846,514,778,866,*)*/

#endif

#ifdef CONFIG_HISI_THERMAL_HOTPLUG
static ssize_t
mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (dev == NULL || attr == NULL)
		return -EINVAL;

	return snprintf(buf, (unsigned long)MAX_MODE_LEN, "%s\n", thermal_info.hotplug.disabled ? "disabled"
		       : "enabled");
}

static ssize_t
mode_store(struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t count)
{
	if (dev == NULL || attr == NULL)
		return -EINVAL;

	if (!strncmp(buf, "enabled", sizeof("enabled") - 1))
		thermal_info.hotplug.disabled = false;
	else if (!strncmp(buf, "disabled", sizeof("disabled") - 1))
		thermal_info.hotplug.disabled = true;
	else
		return -EINVAL;

	return (ssize_t)count;
}

/*lint -e84 -e846 -e514 -e778 -e866 -esym(84,846,514,778,866,*)*/
static DEVICE_ATTR(hotplug_mode, 0644, mode_show, mode_store);
/*lint -e84 -e846 -e514 -e778 -e866 +esym(84,846,514,778,866,*)*/
#endif

u32 get_camera_power(void)
{
	return 0;
}

u32 get_backlight_power(void)
{
	return 0;
}

u32 get_charger_power(void)
{
	return 0;
}

int get_sensor_id_by_name(const char *name) {
	int ret = 0;
	unsigned long i;

	for (i = 0; i < sizeof(ipa_sensor_info) / sizeof(struct ipa_sensor_info); i++) {
		if (!strncmp(name, ipa_sensor_info[i].ipa_sensor_name, strlen(ipa_sensor_info[i].ipa_sensor_name)))
			break;
	}

	if (i < sizeof(ipa_sensor_info) / sizeof(struct ipa_sensor_info)) {
		ret = ipa_sensor_info[i].ipa_get_sensor_id(name);
		if (ret < 0)
			ret = IPA_SENSOR_MAXID;
	} else {
		ret = -EINVAL;
	}

	return ret;
}

#ifdef CONFIG_HISI_THERMAL_SPM
extern struct cpufreq_frequency_table *cpufreq_frequency_get_table(unsigned int cpu);
static void update_cpufreqs(void)
{
	cpufreq_update_policy(0);
	cpufreq_update_policy(4);
}

static bool setfreq_available(unsigned int idx , u32 freq) {
	struct cpufreq_frequency_table *pos, *table = NULL;
	int ret = false;

	switch (idx) {
#ifdef CONFIG_HISI_THERMAL_TRIPPLE_CLUSTERS
			case 0: //Little core
				table = cpufreq_frequency_get_table(0);
				if (!table) {
					pr_err("%s: Unable to find freq table(0)\n", __func__);
					return false;
				}
				break;
			case 1: //middle core
				table = cpufreq_frequency_get_table(4);
				if (!table) {
					pr_err("%s: Unable to find freq table(4)\n", __func__);
					return false;
				}
				break;
			case 2: //Big core
				table = cpufreq_frequency_get_table(6);
				if (!table) {
					pr_err("%s: Unable to find freq table(6)\n", __func__);
					return false;
				}
				break;

			case 3: //GPU
					ret = true;
					break;
#else
		case 0: //Little core
			table = cpufreq_frequency_get_table(0);
			if (!table) {
				pr_err("%s: Unable to find freq table(0)\n", __func__);
				return false;
			}
			break;
		case 1: //Big core
			table = cpufreq_frequency_get_table(4);
			if (!table) {
				pr_err("%s: Unable to find freq table(4)\n", __func__);
				return false;
			}
			break;
		case 2: //GPU
			ret = true;
			break;
#endif
		default:
			break;
	}
#ifdef CONFIG_HISI_THERMAL_TRIPPLE_CLUSTERS
	if (idx == 0 || idx == 1 || idx == 2){
#else
	if (idx == 0 || idx == 1){
#endif
		cpufreq_for_each_valid_entry(pos, table) {/*lint !e613*//* [false alarm]:调用此函数时pos初始值指向了table，table在前面已经判空，因此pos不用判空 */
			if(freq == pos->frequency)/*lint !e613*/
				ret = true;
		}
		if (ret!=true)
			pr_err("idx %d : freq %d don't match any available freq.\n ", idx, freq);
	}

	return ret;
}
#ifdef CONFIG_HISI_THERMAL_TRIPPLE_CLUSTERS
#define MAX_POWERHAL_ACTOR	4UL
#else
#define MAX_POWERHAL_ACTOR	3UL
#endif
unsigned int powerhal_profiles[3][MAX_POWERHAL_ACTOR];

// cppcheck-suppress *
#define SHOW_MODE(mode_name)					\
static ssize_t show_##mode_name					\
(struct device *dev, struct device_attribute *attr, char *buf) 			\
{										\
	if (dev == NULL || attr == NULL)					\
		return 0;							\
										\
	return snprintf(buf, (unsigned long)MAX_SHOW_STR, "%d\n",		\
					(int)thermal_info.spm.mode_name);	\
}

SHOW_MODE(vr_mode);
SHOW_MODE(spm_mode);

// cppcheck-suppress *
#define STORE_MODE(mode_name)					\
static ssize_t store_##mode_name				\
(struct device *dev, struct device_attribute *attr, 		\
	const char *buf, size_t count)				\
{								\
	u32 mode_name;						\
								\
	if (dev == NULL || attr == NULL)			\
		return 0;					\
								\
	if (kstrtouint(buf, 10, &mode_name)) /*lint !e64*/	\
		return -EINVAL;					\
								\
	thermal_info.spm.mode_name = mode_name ? true : false;	\
								\
	cpufreq_update_policies();				\
	return (ssize_t)count;					\
}

STORE_MODE(vr_mode);
STORE_MODE(spm_mode);

/*lint -e84 -e846 -e514 -e778 -e866 -esym(84,846,514,778,866,*)*/
#define MODE_ATTR_RW(mode_name)				\
static DEVICE_ATTR(mode_name, S_IWUSR | S_IRUGO,	\
				show_##mode_name,	\
				store_##mode_name)

MODE_ATTR_RW(vr_mode);
MODE_ATTR_RW(spm_mode);
/*lint -e84 -e846 -e514 -e778 -e866 +esym(84,846,514,778,866,*)*/

static ssize_t
vr_freq_store(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	u32 freq;
	unsigned int i = 0;
	char *token;
	char temp_buf[40]={0};
	char *s;

	if (dev == NULL || attr == NULL)
		return 0;

	strncpy(temp_buf, buf, sizeof(temp_buf) - 1);
	s = temp_buf;

	for(token = strsep(&s, ","); (token != NULL) && (i < MAX_POWERHAL_ACTOR); token = strsep(&s, ","),i++) {
		if (kstrtouint(token, 10, &freq))
			return -EINVAL;
		else {
			if(setfreq_available(i,freq)) {
				powerhal_profiles[1][i]=freq;
			}
		}
	}
	update_cpufreqs();
	return (ssize_t)count;
}

static ssize_t
vr_freq_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	unsigned int i;
	ssize_t ret = 0;

	if (dev == NULL || devattr == NULL)
		return 0;

	for (i = 0; i < MAX_POWERHAL_ACTOR; i++)
		ret += snprintf(buf + ret, 12UL, "%u%s", powerhal_profiles[1][i],",");

	ret += snprintf(buf + ret - 1, 2UL, "\n");
	return ret;
}
static DEVICE_ATTR(vr_freq, S_IWUSR | S_IRUGO, vr_freq_show, vr_freq_store);

static ssize_t
spm_freq_store(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	u32 freq;
	unsigned int i = 0;
	char *token;
	char temp_buf[40]={0};
	char *s;

	if (dev == NULL || attr == NULL)
		return 0;

	strncpy(temp_buf, buf, sizeof(temp_buf) - 1);
	s = temp_buf;

	for(token = strsep(&s, ","); (token != NULL) && (i < MAX_POWERHAL_ACTOR); token = strsep(&s, ","), i++) {
		if (kstrtouint(token, 10, &freq))
			return -EINVAL;
		else {
			if(setfreq_available(i,freq)) {
				powerhal_profiles[0][i]=freq;
			}
		}
	}
	update_cpufreqs();
	return (ssize_t)count;
}

static ssize_t
spm_freq_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	unsigned int i;
	ssize_t ret = 0;

	if (dev == NULL || devattr == NULL)
		return 0;

	for (i = 0; i < MAX_POWERHAL_ACTOR; i++)
		ret += snprintf(buf + ret, 12UL, "%u%s", powerhal_profiles[0][i],",");

	ret += snprintf(buf + ret - 1, 2UL, "\n");
	return ret;
}
static DEVICE_ATTR(spm_freq, S_IWUSR | S_IRUGO, spm_freq_show, spm_freq_store);

static ssize_t
min_freq_store(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	u32 freq;
	unsigned int i = 0;
	char *token;
	char temp_buf[40]={0};
	char *s;

	if (dev == NULL || attr == NULL)
		return 0;

	strncpy(temp_buf, buf, sizeof(temp_buf) - 1);
	s = temp_buf;

	for(token = strsep(&s, ","); (token != NULL) && (i < MAX_POWERHAL_ACTOR); token = strsep(&s, ","),i++) {
		if (kstrtouint(token, 10, &freq))
			return -EINVAL;
		else {
			if(setfreq_available(i,freq)) {
				powerhal_profiles[2][i]=freq;
			}
		}
	}
	update_cpufreqs();
	return (ssize_t)count;
}

static ssize_t
min_freq_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	unsigned int i;
	ssize_t ret = 0;

	if (dev == NULL || devattr == NULL)
		return 0;

	for (i = 0; i < MAX_POWERHAL_ACTOR; i++)
		ret += snprintf(buf + ret, 12UL, "%u%s", powerhal_profiles[2][i],",");

	ret += snprintf(buf + ret - 1, 2UL, "\n");
	return ret;
}
static DEVICE_ATTR(min_freq, S_IWUSR | S_IRUGO, min_freq_show, min_freq_store);

bool is_spm_mode_enabled(void)
{
	return thermal_info.spm.spm_mode || thermal_info.spm.vr_mode;
}

EXPORT_SYMBOL(is_spm_mode_enabled);

unsigned int get_powerhal_profile(enum ipa_actor actor)
{
	int cur_policy = -1;
	unsigned int freq = 0;

#if 1
	if (thermal_info.spm.vr_mode)
		cur_policy = 1;
	else if (thermal_info.spm.spm_mode)
		cur_policy = 0;

	if (cur_policy != -1 && actor <= IPA_GPU)
		freq = powerhal_profiles[cur_policy][actor];
#else
extern int get_profile_cpu_freq(enum ipa_actor actor, unsigned int *freq);
extern unsigned int get_profile_gpu_freq(void);

	if (actor < IPA_GPU) {
		(void)get_profile_cpu_freq(actor, &freq);
	} else {
		freq = get_profile_gpu_freq();
	}
#endif

	return freq;
}
EXPORT_SYMBOL(get_powerhal_profile);

unsigned int get_minfreq_profile(enum ipa_actor actor)
{
	unsigned int freq = 0;

	if (actor <= IPA_GPU)
		freq = powerhal_profiles[2][actor];

	return freq;
}
EXPORT_SYMBOL(get_minfreq_profile);
#endif

int get_soc_temp(void)
{
	if ((IPA_INIT_OK == thermal_info.ipa_thermal[SOC].init_flag) && thermal_info.ipa_thermal[SOC].tzd) {
		if (thermal_info.ipa_thermal[SOC].tzd->temperature < 0)
			return 0;
		return thermal_info.ipa_thermal[SOC].tzd->temperature; /*lint !e571*/
	}

	return  IPA_SOC_INIT_TEMP;
}
EXPORT_SYMBOL(get_soc_temp);

static u32 get_dyn_power_coeff(enum cluster_type cluster, struct ipa_thermal *ipa_dev)
{
	if (NULL == ipa_dev) {
		pr_err("%s parm null\n", __func__);
		return 0;
	}

	return ipa_dev->caps->cluster_dyn_capacitance[cluster];
}

static u32 get_cpu_static_power_coeff(enum cluster_type cluster)
{
	struct capacitances *caps = &g_caps;

	return caps->cluster_static_capacitance[cluster];
}

static u32 get_cache_static_power_coeff(enum cluster_type cluster)
{
	struct capacitances *caps = &g_caps;

	return caps->cache_capacitance[cluster];
}

static unsigned long get_temperature_scale(int temp)
{
	int i, t_exp = 1, t_scale = 0, ret = 0;
	struct capacitances *caps = &g_caps;
	int capacitance[5] = {0};

	for (i = 0; i < 4; i++) {
		ret = kstrtoint(caps->temperature_scale_capacitance[i], 10, &capacitance[i]);
		if (ret)
			pr_warning("%s kstortoint is failed \n", __func__);
		t_scale += capacitance[i] * t_exp;
		t_exp *= temp;
	}

	ret = kstrtoint(caps->temperature_scale_capacitance[4], 10, &capacitance[4]);
	if (ret)
		pr_warning("%s kstortoint is failed \n", __func__);
	return (unsigned long)(t_scale / capacitance[4]); /*lint !e571*/
}

static unsigned long get_voltage_scale(unsigned long u_volt)
{
	unsigned long m_volt = u_volt / 1000;
	unsigned long v_scale;
	v_scale = m_volt * m_volt * m_volt; /* = (m_V^3) / (900 ^ 3) =  */
	return v_scale / 1000000; /* the value returned needs to be /(1E3)*/
}

/* voltage in uV and temperature in mC */
/*lint -e715 -esym(715,*)*/
static int hisi_cluster_get_static_power(cpumask_t *cpumask, int interval,
			    unsigned long u_volt, u32 *static_power)
{
	int temperature;
	unsigned long t_scale, v_scale;
	u32 cpu_coeff;
	u32 nr_cpus = cpumask_weight(cpumask);
	enum cluster_type cluster =
		(enum cluster_type)topology_physical_package_id(cpumask_any(cpumask));

	temperature = get_soc_temp();
	temperature /= 1000;

	cpu_coeff = get_cpu_static_power_coeff(cluster);

	t_scale = get_temperature_scale(temperature);
	v_scale = get_voltage_scale(u_volt);
	*static_power = (u32)(nr_cpus * (cpu_coeff * t_scale * v_scale) / 1000000UL);

	if (nr_cpus) {
		u32 cache_coeff = get_cache_static_power_coeff(cluster);
		*static_power += (u32)((cache_coeff * v_scale * t_scale) / 1000000UL); /* cache leakage */
	}

	return 0;
}
/*lint -e715 -esym(715,*)*/
#ifdef CONFIG_HISI_THERMAL_SPM
int hisi_calc_static_power(const struct cpumask *cpumask, int temp,
				unsigned long u_volt, u32 *static_power)
{
	unsigned long t_scale, v_scale;
	int temperature;
	u32 cpu_coeff;
	int nr_cpus = (int)cpumask_weight(cpumask);
	int cluster =
		topology_physical_package_id(cpumask_any(cpumask));

	temperature = temp / 1000;

	cpu_coeff = (u32)get_cpu_static_power_coeff((enum cluster_type)cluster);

	t_scale = get_temperature_scale(temperature);
	v_scale = get_voltage_scale(u_volt);
	*static_power = (u32)(nr_cpus * (cpu_coeff * t_scale * v_scale) / 1000000); //lint !e737

	if (nr_cpus) {
		u32 cache_coeff = get_cache_static_power_coeff((enum cluster_type)cluster);
		*static_power += (u32)((cache_coeff * v_scale * t_scale) / 1000000); /* cache leakage */
	}

	return 0;
}
EXPORT_SYMBOL(hisi_calc_static_power);
#endif

#ifdef CONFIG_HISI_THERMAL_HOTPLUG
/*lint -e715*/
static int thermal_hotplug_task(void *data)
{
	unsigned int cpu_num;
	struct device *cpu_dev;
	long sensor_val;
	unsigned long flags;

	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);/*lint !e446 !e666*/
		spin_lock_irqsave(&thermal_info.hotplug.hotplug_lock, flags); /*lint !e550*/
		if ((!thermal_info.hotplug.need_up) && (!thermal_info.hotplug.need_down)) {
			spin_unlock_irqrestore(&thermal_info.hotplug.hotplug_lock, flags);

			schedule();

			if (kthread_should_stop())
				break;

			spin_lock_irqsave(&thermal_info.hotplug.hotplug_lock, flags); /*lint !e550*/
		}

		set_current_state(TASK_RUNNING);/*lint !e446 !e666*/
		sensor_val = thermal_info.hotplug.current_temp;

		if (thermal_info.hotplug.need_down) {
			thermal_info.hotplug.need_down = false;
			thermal_info.hotplug.cpu_downed = true;
			spin_unlock_irqrestore(&thermal_info.hotplug.hotplug_lock, flags);

			pr_err("cluster_big temp = %ld, cpu_down!!!\n", sensor_val);
			for (cpu_num = 4; cpu_num <= 7; cpu_num++) {
				cpu_dev = get_cpu_device(cpu_num);
				if (cpu_dev != NULL) {
					device_lock(cpu_dev);
					cpu_down(cpu_num);
					cpu_dev->offline = true;
					device_unlock(cpu_dev);
				}
			}
		} else if (thermal_info.hotplug.need_up) {
			thermal_info.hotplug.need_up = false;
			thermal_info.hotplug.cpu_downed = false;
			spin_unlock_irqrestore(&thermal_info.hotplug.hotplug_lock, flags);

			pr_err("cluster_big temp = %ld, cpu_up!!!\n", sensor_val);
			for (cpu_num = 7; cpu_num >= 4; cpu_num--) {
				cpu_dev = get_cpu_device(cpu_num);
				if (cpu_dev != NULL) {
					device_lock(cpu_dev);
					cpu_up(cpu_num);
					cpu_dev->offline = false;
					device_unlock(cpu_dev);
				}
			}
		}

		trace_IPA_hot_plug(thermal_info.hotplug.cpu_downed, sensor_val, thermal_info.hotplug.cpu_up_threshold,
				thermal_info.hotplug.cpu_down_threshold);
	}

	return 0;
}
/*lint +e715*/
#endif

#define FRAC_BITS 8
#define int_to_frac(x) ((x) << FRAC_BITS)

/**
 * mul_frac() - multiply two fixed-point numbers
 * @x:	first multiplicand
 * @y:	second multiplicand
 *
 * Return: the result of multiplying two fixed-point numbers.  The
 * result is also a fixed-point number.
 */
 /*lint -e702*/
static inline s32 mul_frac(s32 x, s32 y)
{
	return (x * y) >> FRAC_BITS;
}
/*lint +e702*/
#ifdef CONFIG_HISI_THERMAL_HOTPLUG
void hisi_thermal_hotplug_check(int *temp)
{
	unsigned long flags;

#ifdef CONFIG_HISI_HOTPLUG_EMULATION
	if (thermal_info.hotplug.emul_temp)
		*temp = thermal_info.hotplug.emul_temp;
#endif
	if (thermal_info.hotplug.initialized && !thermal_info.hotplug.disabled) {
		spin_lock_irqsave(&thermal_info.hotplug.hotplug_lock, flags); /*lint !e550*/
		thermal_info.hotplug.current_temp = *temp;

		if (thermal_info.hotplug.cpu_downed) {
			if (*temp < thermal_info.hotplug.cpu_up_threshold) {
				thermal_info.hotplug.need_up = true;
				spin_unlock_irqrestore(&thermal_info.hotplug.hotplug_lock, flags);

				wake_up_process(thermal_info.hotplug.hotplug_task);
			} else {
				spin_unlock_irqrestore(&thermal_info.hotplug.hotplug_lock, flags);
			}
		} else {
			if (*temp > thermal_info.hotplug.cpu_down_threshold) {
				thermal_info.hotplug.need_down = true;
				spin_unlock_irqrestore(&thermal_info.hotplug.hotplug_lock, flags);

				wake_up_process(thermal_info.hotplug.hotplug_task);
			} else {
				spin_unlock_irqrestore(&thermal_info.hotplug.hotplug_lock, flags);
			}
		}
	}
}
#endif

static int get_temp_value(void *data, int *temp)
{
	int sensor_val[IPA_SENSOR_NUM] = {0};
	struct ipa_sensor *sensor = (struct ipa_sensor *)data;
	int val = 0;
	int val_max = 0;
	int ret = -EINVAL;
	int id = 0;
	int est_temp;
#ifdef CONFIG_HISI_THERMAL_SHELL
	struct thermal_zone_device *shell_tz = NULL;
#endif

	if (IPA_SENSOR_MAXID == sensor->sensor_id) {
		/*read all sensor*/
		for (id = 0; id < IPA_SENSOR_NUM; id++) {
			ret = ipa_get_sensor_value((u32)id, &val);
			sensor_val[id] = val;
			if (ret)
				return ret;
		}
#ifdef CONFIG_HISI_THERMAL_HOTPLUG
		hisi_thermal_hotplug_check(&sensor_val[CLUSTER_BIG]);
#endif
		val_max = sensor_val[0];
		for (id = 1; id < IPA_SENSOR_NUM; id++) {
			if (sensor_val[id] > val_max)
				val_max = sensor_val[id];
		}

		val = val_max;

		trace_IPA_get_tsens_value(sensor_val[0], sensor_val[1], sensor_val[2], val_max);
#ifdef CONFIG_HISI_THERMAL_SHELL
	} else if (IPA_SENSOR_SHELLID == sensor->sensor_id) {
		shell_tz = thermal_zone_get_zone_by_name(IPA_SENSOR_SHELL);
		ret = hisi_get_shell_temp(shell_tz, temp);
		trace_IPA_get_tsens_value(0, 0, 0, *temp);
		if (ret)
			return ret;
		else
			return 0;
#endif
 	} else if (sensor->sensor_id < IPA_PERIPH_NUM) {
		ret = ipa_get_periph_value(sensor->sensor_id, &val);
		trace_IPA_get_tsens_value(0, 0, 0, val);
		if (ret)
			return ret;
		val = val < 0 ? 0 : val;
	} else {
		return ret;
	}

	if (!sensor->prev_temp)
		sensor->prev_temp = val;
	 /*lint -e776*/
	est_temp = mul_frac(sensor->alpha, val) +
		mul_frac(int_to_frac(1) - sensor->alpha, sensor->prev_temp);
	 /*lint +e776*/
	sensor->prev_temp = est_temp;
	*temp = est_temp;

	return 0;
}

static void update_debugfs(struct ipa_sensor *sensor_data)
{
#ifdef CONFIG_HISI_DEBUG_FS
	struct dentry *dentry_f, *filter_d;
	char filter_name[25];

	snprintf(filter_name, sizeof(filter_name), "thermal_lpf_filter%d", sensor_data->sensor_id);
	filter_d = debugfs_create_dir(filter_name, NULL);
	if (IS_ERR_OR_NULL(filter_d)) {
		pr_warning("unable to create debugfs directory for the LPF filter\n");
		return;
	}

	dentry_f = debugfs_create_u32("alpha", S_IWUSR | S_IRUGO, filter_d,
				      (u32 *)&sensor_data->alpha);
	if (IS_ERR_OR_NULL(dentry_f)) {
		debugfs_remove(filter_d);
		pr_warn("IPA:Unable to create debugfsfile: alpha\n");
		return;
	}
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 15)
/*lint -e785*/
static const struct thermal_zone_of_device_ops hisi_ipa_thermal_ops = {
	.get_temp = get_temp_value,
};
#ifdef CONFIG_HISI_THERMAL_SHELL
extern struct thermal_zone_device_ops shell_thermal_zone_ops;
#endif
/*lint +e785*/
#endif

#ifdef CONFIG_HISI_THERMAL_HOTPLUG
static int thermal_hotplug_init(void)
{
	struct device_node *hotplug_np;
	int ret;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };

	if (!thermal_info.hotplug.initialized) {
		hotplug_np = of_find_node_by_name(NULL, "cpu_temp_threshold");
		if (!hotplug_np) {
			pr_err("cpu_temp_threshold node not found\n");
			return -ENODEV;
		}

		ret = of_property_read_u32(hotplug_np, "hisilicon,up_threshold", (u32 *)&thermal_info.hotplug.cpu_up_threshold);
		if (ret) {
			pr_err("%s hotplug up threshold read err\n", __func__);
			goto node_put;
		}

		ret = of_property_read_u32(hotplug_np, "hisilicon,down_threshold", (u32 *)&thermal_info.hotplug.cpu_down_threshold);
		if (ret) {
			pr_err("%s hotplug down threshold read err\n", __func__);
			goto node_put;
		}

		spin_lock_init(&thermal_info.hotplug.hotplug_lock);
		thermal_info.hotplug.hotplug_task = kthread_create(thermal_hotplug_task, NULL, "thermal_hotplug");
		if (IS_ERR(thermal_info.hotplug.hotplug_task)) {
			pr_err("%s: thermal hotplug task create fail\n", __func__);
			thermal_info.hotplug.hotplug_task = NULL;
			goto node_put;
		}

		sched_setscheduler_nocheck(thermal_info.hotplug.hotplug_task, SCHED_FIFO, &param);
		get_task_struct(thermal_info.hotplug.hotplug_task);

		wake_up_process(thermal_info.hotplug.hotplug_task);

		thermal_info.hotplug.initialized = true;
		of_node_put(hotplug_np);
	}

	return 0;

node_put:
	if (!thermal_info.hotplug.initialized)
		of_node_put(hotplug_np);

	return ret;
}
#endif

static int of_parse_thermal_zone_caps(void)
{
	int ret, i;
	struct device_node *np;

	if (!g_caps.initialized) {
		np = of_find_node_by_name(NULL, "capacitances");
		if (!np) {
			pr_err("Capacitances node not found\n");
			return -ENODEV;
		}

		ret = of_property_read_u32_array(np, "hisilicon,cluster_dyn_capacitance", g_caps.cluster_dyn_capacitance, (unsigned long)NUM_CLUSTERS);
		if (ret) {
			pr_err("%s actor_dyn_capacitance read err\n", __func__);
			goto node_put;
		}

		ret = of_property_read_u32_array(np, "hisilicon,cluster_static_capacitance", g_caps.cluster_static_capacitance, (unsigned long)NUM_CLUSTERS);
		if (ret) {
			pr_err("%s actor_dyn_capacitance read err\n", __func__);
			goto node_put;
		}

		ret = of_property_read_u32_array(np, "hisilicon,cache_capacitance", g_caps.cache_capacitance, (unsigned long)NUM_CLUSTERS);
		if (ret) {
			pr_err("%s actor_dyn_capacitance read err\n", __func__);
			goto node_put;
		}

		for (i = 0; i < NUM_TEMP_SCALE_CAPS; i++) {
			ret =  of_property_read_string_index(np, "hisilicon,temperature_scale_capacitance", i, &g_caps.temperature_scale_capacitance[i]);
			if (ret) {
				pr_err("%s temperature_scale_capacitance [%d] read err\n", __func__, i);
				goto node_put;
			}
		}
		g_caps.initialized = true;
		of_node_put(np);
	}
	return 0;

node_put:
	if (!g_caps.initialized)
		of_node_put(np);

	return ret;
}

static int ipa_register_soc_cdev(struct ipa_thermal *thermal_data, struct platform_device *pdev)
{
	int ret = 0;
	int cpuid;
	enum cluster_type cluster;
	struct device_node *cdev_np;
	int i;
	struct cpumask cpu_masks[NUM_CLUSTERS];
	int cpu;
	char node[16];


	memset(cpu_masks, 0, sizeof(struct cpumask) * NUM_CLUSTERS);
	for_each_online_cpu(cpu) { /*lint !e713*/
		int cluster_id = topology_physical_package_id(cpu);
		if (cluster_id > NUM_CLUSTERS) {
			pr_warn("IPA:Cluster id: %d > %d\n", cluster_id, NUM_CLUSTERS);
			return -ENODEV;
		}
		cpumask_set_cpu((u32)cpu, &cpu_masks[cluster_id]);
	}

	thermal_data->cdevs = kcalloc((size_t)NUM_CLUSTERS, sizeof(struct thermal_cooling_device *), GFP_KERNEL); /*lint !e433*/
	if (!thermal_data->cdevs) {
		ret = -ENOMEM;
		goto end;
	}

	for (i = 0; i < NUM_CLUSTERS; i++) {
		cpuid = (int)cpumask_any(&cpu_masks[i]);
		if (cpuid >= nr_cpu_ids)
			continue;
		cluster = (enum cluster_type)topology_physical_package_id(cpuid);

		snprintf(node, sizeof(node), "cluster%d", i);
		cdev_np = of_find_node_by_name(NULL, node);

		if (!cdev_np) {
			dev_err(&pdev->dev, "Node not found: %s\n", node);
			continue;
		}

		thermal_data->cdevs[i] =
			of_cpufreq_power_cooling_register(cdev_np,
						&cpu_masks[i],
						get_dyn_power_coeff(cluster, thermal_data),
						hisi_cluster_get_static_power);
		if (IS_ERR(thermal_data->cdevs[i])) {
			ret = (int)PTR_ERR(thermal_data->cdevs[i]);
			dev_err(&pdev->dev,
				"IPA:Error registering cpu power actor:  cluster [%d] ERROR_ID [%d]\n",
				i, ret);
			goto cdevs_unregister;
		}
		thermal_data->cdevs_num++;
		of_node_put(cdev_np);
	}

	return 0;

cdevs_unregister:
	for (i = 0; i < thermal_data->cdevs_num; i++) {  /*lint !e574*/
		cpufreq_cooling_unregister(thermal_data->cdevs[i]);
	}
	thermal_data->cdevs_num = 0;
	kfree(thermal_data->cdevs);
	thermal_data->cdevs = NULL;
end:
	return ret;
}

static int ipa_register_board_cdev(struct ipa_thermal *thermal_data, struct platform_device *pdev)
{
	struct device_node *board_np;
	int ret = 0, i;
	struct device_node *child = NULL;

	board_np = of_find_node_by_name(NULL, "board-map");
	if (!board_np) {
		dev_err(&pdev->dev, "Board map node not found\n");
		goto end;
	}

	ret = of_get_child_count(board_np);
	if (ret <= 0) {
		ret = -EINVAL;
		of_node_put(board_np);
		pr_err("IPA: board map child count err\n");
		goto end;
	}

	thermal_data->cdevs = kzalloc(sizeof(struct thermal_cooling_device *) * (unsigned long)ret, GFP_KERNEL); /*lint !e571*/
	if (!thermal_data->cdevs) {
		ret = -ENOMEM;
		goto end;
	}

	for_each_child_of_node(board_np, child) {
		thermal_data->cdevs[thermal_data->cdevs_num] =
			board_power_cooling_register(child, get_board_power[thermal_data->cdevs_num]);
		if (IS_ERR(thermal_data->cdevs[thermal_data->cdevs_num])) {
			of_node_put(board_np);
			ret = PTR_ERR(thermal_data->cdevs[thermal_data->cdevs_num]); /*lint !e712*/
			dev_err(&pdev->dev,
				"IPA:Error registering board power actor: ERROR_ID [%d]\n", ret);
			goto cdevs_unregister;
		}
		thermal_data->cdevs_num++;
	}
	of_node_put(board_np);

	return 0;

cdevs_unregister:
	for (i = 0; i < thermal_data->cdevs_num; i++) { /*lint !e574*/
		board_cooling_unregister(thermal_data->cdevs[i]);
	}
	thermal_data->cdevs_num = 0;
	kfree(thermal_data->cdevs);
	thermal_data->cdevs = NULL;
end:
	return ret;
}

static inline void cooling_device_unregister(struct ipa_thermal *thermal_data)
{
	int i;

	if (!thermal_data->cdevs)
		return;

	for (i = 0; i < thermal_data->cdevs_num; i++) {
		if (strstr(thermal_data->cdevs[i]->type, "thermal-board"))
			board_cooling_unregister(thermal_data->cdevs[i]);
		else
			cpufreq_cooling_unregister(thermal_data->cdevs[i]);
	}

	kfree(thermal_data->cdevs);
}

static int ipa_thermal_probe(struct platform_device *pdev)
{
	struct ipa_thermal *thermal_data = &thermal_info.ipa_thermal[SOC];
	int sensor;
	struct device *dev = &pdev->dev;
	struct device_node *dev_node = dev->of_node;
	int ret;
	const char *ch;

	if (!of_device_is_available(dev_node)) {
		dev_err(&pdev->dev, "IPA thermal dev not found\n");
		return -ENODEV;
	}

	if (!cpufreq_frequency_get_table(0)) {
		dev_info(&pdev->dev,
			"IPA:Frequency table not initialized. Deferring probe...\n");
		return -EPROBE_DEFER;
	}

#ifdef CONFIG_HISI_THERMAL_HOTPLUG
	ret = thermal_hotplug_init();
	if (ret)
		pr_err("thermal hotplug init error\n");
#endif

	ret = of_parse_thermal_zone_caps();
	if (ret) {
		pr_err("thermal zone caps parse error\n");
		goto end;
	}

	if (!strncmp(pdev->name, "ipa-sensor@0", sizeof("ipa-sensor@0") - 1)) {
		thermal_data = &thermal_info.ipa_thermal[SOC];
		thermal_data->caps = &g_caps;

		ret = ipa_register_soc_cdev(thermal_data, pdev);
	} else if (!strncmp(pdev->name, "ipa-sensor@1", sizeof("ipa-sensor@1") - 1)) {
		thermal_data = &thermal_info.ipa_thermal[BOARD];
		thermal_data->caps = &g_caps;

		ret = ipa_register_board_cdev(thermal_data, pdev);
	}

	if (ret)
		goto end;

	ret =  of_property_read_string(dev_node, "type", &ch);
	if (ret) {
		dev_err(dev, "%s sensor name read err\n", __func__);
		goto cdevs_unregister;
	}

	sensor = get_sensor_id_by_name(ch);
	if (sensor < 0) {
		ret = sensor;
		goto cdevs_unregister;
	}

	thermal_data->ipa_sensor.sensor_id = (u32)sensor;
	dev_info(&pdev->dev, "IPA:Probed %s sensor. Id=%hu\n", ch, sensor);

	/*
	* alpha ~= 2 / (N + 1) with N the window of a rolling mean We
	 * use 8-bit fixed point arithmetic.  For a rolling average of
	 * window 20, alpha = 2 / (20 + 1) ~= 0.09523809523809523 .
	 * In 8-bit fixed point arigthmetic, 0.09523809523809523 * 256
	 * ~= 24
	 */

	thermal_data->ipa_sensor.alpha = 24;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 15)
	thermal_data->tzd = thermal_zone_of_sensor_register(&pdev->dev,
									(int)thermal_data->ipa_sensor.sensor_id,
									&thermal_data->ipa_sensor,
									&hisi_ipa_thermal_ops);
#else
	thermal_data->tzd = thermal_zone_of_sensor_register(&pdev->dev,
									thermal_data->ipa_sensor.sensor_id,
									&thermal_data->ipa_sensor,
									get_temp_value, NULL);
#endif

	if (IS_ERR(thermal_data->tzd)) {
		dev_err(&pdev->dev, "IPA ERR:registering sensor tzd error.\n");
		ret = PTR_ERR(thermal_data->tzd); /*lint !e712*/
		goto cdevs_unregister;
	}

	update_debugfs(&thermal_data->ipa_sensor);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	thermal_zone_device_update(thermal_data->tzd);
#else
	thermal_zone_device_update(thermal_data->tzd, THERMAL_EVENT_UNSPECIFIED);
#endif

	platform_set_drvdata(pdev, thermal_data);
	thermal_data->init_flag = IPA_INIT_OK;

	return 0;

cdevs_unregister:
	cooling_device_unregister(thermal_data);
end:
	return ret;
}

static int ipa_thermal_remove(struct platform_device *pdev)
{
	struct ipa_thermal *thermal_data = platform_get_drvdata(pdev);

#ifdef CONFIG_HISI_THERMAL_HOTPLUG
	if (thermal_info.hotplug.hotplug_task) {
		kthread_stop(thermal_info.hotplug.hotplug_task);
		put_task_struct(thermal_info.hotplug.hotplug_task);
		thermal_info.hotplug.hotplug_task = NULL;
	}
#endif

	if (NULL == thermal_data) {
		dev_warn(&pdev->dev, "%s sensor is null!\n", __func__);
		return -1;
	}

	thermal_zone_of_sensor_unregister(&pdev->dev, thermal_data->tzd);
	cooling_device_unregister(thermal_data);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

/*lint -e785 -esym(785,*)*/
static struct of_device_id ipa_thermal_of_match[] = {
	{ .compatible = "arm,ipa-thermal0" },
	{ .compatible = "arm,ipa-thermal1" },
	{},
};
/*lint -e785 +esym(785,*)*/

MODULE_DEVICE_TABLE(of, ipa_thermal_of_match);

static struct platform_driver ipa_thermal_platdrv = {
	.driver = {
		.name		= "ipa-thermal",
		.owner		= THIS_MODULE, //lint !e64
		.of_match_table = ipa_thermal_of_match
	},
	.probe	= ipa_thermal_probe,
	.remove	= ipa_thermal_remove,
};

/*lint -e64 -e528 -esym(64,528,*)*/
module_platform_driver(ipa_thermal_platdrv);
/*lint -e64 -e528 +esym(64,528,*)*/

#ifdef CONFIG_HISI_THERMAL_SPM
static int powerhal_cfg_init(void)
{
	struct device_node *node;
	int ret;
	unsigned int data[MAX_POWERHAL_ACTOR] = {0};

	node = of_find_compatible_node(NULL, NULL, "hisi,powerhal");
	if (!node) {
		pr_err("%s cannot find powerhal dts.\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32_array(node, "hisi,powerhal-spm-cfg", data, MAX_POWERHAL_ACTOR);
	if (ret) {
		pr_err("%s cannot find hisi,powerhal-spm-cfg.\n", __func__);
		return -ENODEV;
	}
	memcpy(powerhal_profiles[0], data, sizeof(data));

	ret = of_property_read_u32_array(node, "hisi,powerhal-vr-cfg", data, MAX_POWERHAL_ACTOR);
	if (ret) {
		pr_err("%s cannot find hisi,powerhal-spm-cfg.\n", __func__);
		return -ENODEV;
	}
	memcpy(powerhal_profiles[1], data, sizeof(data));

	ret = of_property_read_u32_array(node, "hisi,powerhal-min-cfg", data, MAX_POWERHAL_ACTOR);
	if (ret) {
		pr_err("%s cannot find hisi,powerhal-spm-cfg.\n", __func__);
		return -ENODEV;
	}

	memcpy(powerhal_profiles[2], data, sizeof(data));
	return 0;

}
#endif

#ifdef CONFIG_HISI_THERMAL_TRIPPLE_CLUSTERS
#define MAX_WEIGHTSL_ACTOR	4UL
#define IPA_CLUSTER2_WEIGHT_VALUE      (256)
#define IPA_CLUSTER2_WEIGHT_BOOST      (256)
#else
#define MAX_WEIGHTSL_ACTOR	3UL
#endif
#define IPA_CLUSTER0_WEIGHT_VALUE      (768)  //default value
#define IPA_CLUSTER1_WEIGHT_VALUE      (256)
#define IPA_GPU_WEIGHT_VALUE           (256)
#define IPA_CLUSTER0_WEIGHT_BOOST      (768)  //boost value
#define IPA_CLUSTER1_WEIGHT_BOOST      (256)
#define IPA_GPU_WEIGHT_BOOST           (768)


unsigned int weights_profiles[2][MAX_WEIGHTSL_ACTOR] = {
#ifdef CONFIG_HISI_THERMAL_TRIPPLE_CLUSTERS
		{IPA_CLUSTER0_WEIGHT_VALUE,IPA_CLUSTER1_WEIGHT_VALUE,IPA_CLUSTER2_WEIGHT_VALUE,IPA_GPU_WEIGHT_VALUE},
		{IPA_CLUSTER0_WEIGHT_BOOST,IPA_CLUSTER1_WEIGHT_BOOST,IPA_CLUSTER2_WEIGHT_BOOST,IPA_GPU_WEIGHT_BOOST}};
#else
                                            {IPA_CLUSTER0_WEIGHT_VALUE,IPA_CLUSTER1_WEIGHT_VALUE,IPA_GPU_WEIGHT_VALUE},
                                            {IPA_CLUSTER0_WEIGHT_BOOST,IPA_CLUSTER1_WEIGHT_BOOST,IPA_GPU_WEIGHT_BOOST}};
#endif
extern unsigned int g_ipa_gpu_boost_weights[IPA_ACTOR_MAX];
extern unsigned int g_ipa_normal_weights[IPA_ACTOR_MAX];

static int ipa_weights_cfg_init(void)
{
	struct device_node *node;
	int ret;
	unsigned int data[2][MAX_WEIGHTSL_ACTOR] = {{0}, {0}};

#if CONFIG_OF
	node = of_find_compatible_node(NULL, NULL, "hisi,weights");
	if (!node) {
		pr_err("%s cannot find weights dts.\n", __func__);
		return -ENODEV;
	}
#endif

#if CONFIG_OF
	ret = of_property_read_u32_array(node, "hisi,weights-default-cfg", data[0], IPA_ACTOR_MAX);
	if (ret) {
		pr_err("%s cannot find hisi,weights-default-cfg.\n", __func__);
		return -ENODEV;
	}
#endif
	memcpy_s(weights_profiles[0], sizeof(weights_profiles[0]), data[0], sizeof(weights_profiles[0]));

#if CONFIG_OF
	ret = of_property_read_u32_array(node, "hisi,weights-boost-cfg", data[1], IPA_ACTOR_MAX);
	if (ret) {
		pr_err("%s cannot find hisi,weights-boost-cfg.\n", __func__);
		return -ENODEV;
	}/*lint !e419*/
#endif
	memcpy_s(weights_profiles[1], sizeof(weights_profiles[1]), data[1], sizeof(weights_profiles[1]));

	return 0;
}

void dynipa_get_weights_cfg(unsigned int * weight0, unsigned int * weight1)
{
	memcpy_s(weight0, sizeof(*weight0) * MAX_WEIGHTSL_ACTOR, weights_profiles[0], sizeof(*weight0) * MAX_WEIGHTSL_ACTOR);
	memcpy_s(weight1, sizeof(*weight1) * MAX_WEIGHTSL_ACTOR, weights_profiles[1], sizeof(*weight1) * MAX_WEIGHTSL_ACTOR);
}
EXPORT_SYMBOL(dynipa_get_weights_cfg);

static int hisi_thermal_init(void)
{
	int ret = 0;

	if (ipa_weights_cfg_init()) {
		pr_err("%s: ipa_weights_init error, use default value.\n", __func__);
	}

	dynipa_get_weights_cfg(g_ipa_normal_weights, g_ipa_gpu_boost_weights);

#if defined(CONFIG_HISI_THERMAL_SPM) || defined(CONFIG_HISI_THERMAL_HOTPLUG)
	thermal_info.hisi_thermal_class = class_create(THIS_MODULE, "hisi_thermal");  //lint !e64
	if (IS_ERR(thermal_info.hisi_thermal_class)) {
		pr_err("Hisi thermal class create error\n");
		return (int)PTR_ERR(thermal_info.hisi_thermal_class);
	}
#endif

#ifdef CONFIG_HISI_THERMAL_SPM
	if (powerhal_cfg_init()) {
		pr_err("%s: powerhal_init error\n", __func__);
		ret = -ENODEV;
		goto class_destroy;
	}

	thermal_info.spm.device =
	    device_create(thermal_info.hisi_thermal_class, NULL, 0, NULL,
			  "spm");
	if (IS_ERR(thermal_info.spm.device)) {
		pr_err("Spm device create error\n");
		ret = (int)PTR_ERR(thermal_info.spm.device);
		goto class_destroy;
	}

	ret = device_create_file(thermal_info.spm.device, &dev_attr_spm_mode);
	if (ret) {
		pr_err("Spm mode create error\n");
		goto device_destroy;
	}

	ret = device_create_file(thermal_info.spm.device, &dev_attr_spm_freq);
	if (ret) {
		goto device_destroy;
	}
	ret = device_create_file(thermal_info.spm.device, &dev_attr_vr_mode);
	if (ret) {
		pr_err("VR mode create error\n");
		goto device_destroy;
	}

	ret = device_create_file(thermal_info.spm.device, &dev_attr_vr_freq);
	if (ret) {
		goto device_destroy;
	}

	ret = device_create_file(thermal_info.spm.device, &dev_attr_min_freq);
	if (ret) {
		goto device_destroy;
	}
#endif

#ifdef CONFIG_HISI_THERMAL_HOTPLUG
	thermal_info.hotplug.device =
	    device_create(thermal_info.hisi_thermal_class, NULL, 0, NULL,
			  "hotplug");
	if (IS_ERR(thermal_info.hotplug.device)) {
		pr_err("Hotplug device create error\n");
		ret = (int)PTR_ERR(thermal_info.hotplug.device);
#ifdef CONFIG_HISI_THERMAL_SPM
		goto device_destroy;
#else
		goto class_destroy;
#endif
	}

#ifdef CONFIG_HISI_HOTPLUG_EMULATION
	ret = device_create_file(thermal_info.hotplug.device, &dev_attr_hotplug_emul_temp);
	if (ret) {
		pr_err("Hotplug emulation temp create error\n");
		goto device_destroy;
	}
#endif
	ret = device_create_file(thermal_info.hotplug.device, &dev_attr_hotplug_mode);
	if (ret) {
		pr_err("Hotplug mode create error\n");
		goto device_destroy;
	}

#endif
	return 0;

#if defined(CONFIG_HISI_THERMAL_SPM) || defined(CONFIG_HISI_THERMAL_HOTPLUG)
device_destroy:
	device_destroy(thermal_info.hisi_thermal_class, 0);
class_destroy:
	class_destroy(thermal_info.hisi_thermal_class);
	thermal_info.hisi_thermal_class = NULL;

	return ret;
#endif
}

static void hisi_thermal_exit(void)
{
#if defined(CONFIG_HISI_THERMAL_SPM) || defined(CONFIG_HISI_THERMAL_HOTPLUG)
	if (thermal_info.hisi_thermal_class) {
		device_destroy(thermal_info.hisi_thermal_class, 0);
		class_destroy(thermal_info.hisi_thermal_class);
	}
#endif
}

/*lint -e528 -esym(528,*)*/
module_init(hisi_thermal_init);
module_exit(hisi_thermal_exit);
/*lint -e528 +esym(528,*)*/

MODULE_LICENSE("GPL");
