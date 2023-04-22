/*
 *  thermal.c - Generic Thermal Management Sysfs support.
 *
 *  Copyright (C) 2008 Intel Corp
 *  Copyright (C) 2008 Zhang Rui <rui.zhang@intel.com>
 *  Copyright (C) 2008 Sujith Thomas <sujith.thomas@intel.com>
 *
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>
#include <linux/idr.h>
#include <linux/thermal.h>
#include <linux/reboot.h>
#include <linux/string.h>
#include <linux/of.h>
#include <net/netlink.h>
#include <net/genetlink.h>
#include <linux/suspend.h>

#define CREATE_TRACE_POINTS
#include <trace/events/thermal.h>

#include "thermal_core.h"
#include "thermal_hwmon.h"

#ifdef CONFIG_HISI_PERF_CTRL
#include "../hisi/perf_ctrl/perf_ctrl.h"
#endif

MODULE_AUTHOR("Zhang Rui");
MODULE_DESCRIPTION("Generic thermal management sysfs support");
MODULE_LICENSE("GPL v2");

static DEFINE_IDR(thermal_tz_idr);
static DEFINE_IDR(thermal_cdev_idr);
static DEFINE_MUTEX(thermal_idr_lock);

static LIST_HEAD(thermal_tz_list);
static LIST_HEAD(thermal_cdev_list);
static LIST_HEAD(thermal_governor_list);

static DEFINE_MUTEX(thermal_list_lock);
static DEFINE_MUTEX(thermal_governor_lock);

static atomic_t in_suspend;

static struct thermal_governor *def_governor;

#ifdef CONFIG_HISI_IPA_THERMAL
#define IPA_FREQ_MAX	(~0U)
#define BOARD_THERMAL_NAME "board_thermal"
#define SOC_THERMAL_NAME "soc_thermal"
#define USER_SPACE_GOVERNOR       "user_space"
int g_ipa_freq_limit_debug = 0;

#ifdef CONFIG_HISI_THERMAL_TRIPPLE_CLUSTERS
unsigned int g_ipa_freq_limit[IPA_ACTOR_MAX] = {
	IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX
};
unsigned int g_ipa_soc_freq_limit[IPA_ACTOR_MAX] = {
	IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX
};
unsigned int g_ipa_board_freq_limit[IPA_ACTOR_MAX] = {
	IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX
};
#else
unsigned int g_ipa_freq_limit[IPA_ACTOR_MAX] = {
	IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX
};
unsigned int g_ipa_soc_freq_limit[IPA_ACTOR_MAX] = {
	               IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX
};
unsigned int g_ipa_board_freq_limit[IPA_ACTOR_MAX] = {
	              IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX
};
#endif
unsigned int g_ipa_soc_state[IPA_ACTOR_MAX] = {0};
unsigned int g_ipa_board_state[IPA_ACTOR_MAX] = {0};

#define IPA_CLUSTER0_WEIGHT_NAME       "cdev1_weight"  //default name value
#define IPA_CLUSTER1_WEIGHT_NAME       "cdev2_weight"
#ifdef CONFIG_HISI_THERMAL_TRIPPLE_CLUSTERS
#define IPA_CLUSTER2_WEIGHT_NAME       "cdev3_weight"
#endif
#define IPA_GPU_WEIGHT_NAME            "cdev0_weight"

unsigned int g_ipa_gpu_boost_weights[IPA_ACTOR_MAX] = {0};
unsigned int g_ipa_normal_weights[IPA_ACTOR_MAX] = {0};

static DEFINE_MUTEX(thermal_boost_lock);/*lint !e64 !e570 !e651 !e708 !e785*/

#ifdef CONFIG_HISI_THERMAL_SPM
unsigned int profile_power[IPA_ACTOR_MAX];
extern bool is_spm_mode_enabled(void);
extern unsigned int get_powerhal_profile(enum ipa_actor actor);
extern unsigned int get_minfreq_profile(enum ipa_actor actor);
int of_thermal_get_num_tbps(struct thermal_zone_device *tz);
int of_thermal_get_actor_weight(struct thermal_zone_device *tz, int i,
				enum ipa_actor *actor, unsigned int *weight);
#endif

bool thermal_of_get_cdev_type(struct thermal_zone_device *tzd,
			struct thermal_cooling_device *cdev);

void ipa_freq_debug(int onoff)
{
	g_ipa_freq_limit_debug = onoff;
}

#define MAX_TEMP 145000
#define MIN_TEMP -40000
s32 thermal_zone_temp_check(s32 temperature)
{
	if (temperature > MAX_TEMP) {
		temperature = MAX_TEMP;
	} else if (temperature < MIN_TEMP) {
		temperature = MIN_TEMP;
	}
	return temperature;
}

void ipa_freq_limit_init(void)
{
	int i;
	for(i = 0; i < IPA_ACTOR_MAX; i++)
		g_ipa_freq_limit[i] = IPA_FREQ_MAX;
}

void ipa_freq_limit_reset(struct thermal_zone_device *tz)
{
	int i;

	struct thermal_instance *instance;

	if(tz->is_board_thermal) {
		for(i = 0; i < IPA_ACTOR_MAX; i++){
			g_ipa_board_freq_limit[i] = IPA_FREQ_MAX;
			g_ipa_board_state[i] = 0;
		}
	} else {
		for(i = 0; i < IPA_ACTOR_MAX; i++){
			g_ipa_soc_freq_limit[i] = IPA_FREQ_MAX;
			g_ipa_soc_state[i] = 0;
		}
	}

	for(i = 0; i < IPA_ACTOR_MAX; i++) {
		if(g_ipa_soc_freq_limit[i] != 0 && g_ipa_board_freq_limit[i] !=0 )
			g_ipa_freq_limit[i] = min(g_ipa_soc_freq_limit[i], g_ipa_board_freq_limit[i]);/*lint !e1058*/
		else if (g_ipa_soc_freq_limit[i] == 0)
			g_ipa_freq_limit[i] = g_ipa_board_freq_limit[i];
		else if (g_ipa_board_freq_limit[i] ==0)
			g_ipa_freq_limit[i] = g_ipa_soc_freq_limit[i];
		else
			g_ipa_freq_limit[i] = IPA_FREQ_MAX;
	}

	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {/*lint !e64 !e826*/
		instance->target = 0;
		instance->cdev->ops->set_cur_state(instance->cdev, 0UL);
		instance->cdev->updated = true;
	}
}

unsigned int ipa_freq_limit(enum ipa_actor actor, unsigned int target_freq)
{
#ifdef CONFIG_HISI_THERMAL_SPM
	unsigned int freq, min_freq;
#endif

	if (actor >= IPA_ACTOR_MAX)
		return target_freq;

	if (g_ipa_freq_limit_debug)
#ifdef CONFIG_HISI_THERMAL_TRIPPLE_CLUSTERS
		pr_err("actor[%d]target_freq[%u]IPA:[%u][%u][%u][%u]min[%u]\n",
			actor,target_freq, g_ipa_freq_limit[IPA_CLUSTER0],
			g_ipa_freq_limit[IPA_CLUSTER1], g_ipa_freq_limit[IPA_CLUSTER2],
			g_ipa_freq_limit[IPA_GPU], min(target_freq, g_ipa_freq_limit[actor]));/*lint !e1058*/
#else
		pr_err("actor[%d]target_freq[%u]IPA:[%u][%u][%u]min[%u]\n",
			  actor,target_freq, g_ipa_freq_limit[IPA_CLUSTER0],
			  g_ipa_freq_limit[IPA_CLUSTER1], g_ipa_freq_limit[IPA_GPU],
			  min(target_freq, g_ipa_freq_limit[actor]));/*lint !e1058*/
#endif

#ifdef CONFIG_HISI_THERMAL_SPM
	if (is_spm_mode_enabled()) {
		freq = get_powerhal_profile(actor);
		min_freq = get_minfreq_profile(actor);

		if(target_freq >= freq)
			target_freq = freq;

		if(target_freq <= min_freq)
			target_freq = min_freq;

		return target_freq;
	}
#endif

	return min(target_freq, g_ipa_freq_limit[actor]);/*lint !e1058*/
}
EXPORT_SYMBOL(ipa_freq_limit);
#endif

#ifdef CONFIG_HISI_PERF_CTRL
int get_ipa_status(struct ipa_stat *status)
{
#ifdef CONFIG_HISI_IPA_THERMAL
	status->cluster0 = g_ipa_freq_limit[0];
	status->cluster1 = g_ipa_freq_limit[1];
#ifndef CONFIG_HISI_THERMAL_TRIPPLE_CLUSTERS
	status->cluster2 = 0;
	status->gpu = g_ipa_freq_limit[2];
#else
	status->cluster2 = g_ipa_freq_limit[2];
	status->gpu = g_ipa_freq_limit[3];
#endif
#endif
	return 0;
}
EXPORT_SYMBOL(get_ipa_status);
#endif

static struct thermal_governor *__find_governor(const char *name)
{
	struct thermal_governor *pos;

	if (!name || !name[0])
		return def_governor;

	list_for_each_entry(pos, &thermal_governor_list, governor_list)
		if (!strncasecmp(name, pos->name, THERMAL_NAME_LENGTH))
			return pos;

	return NULL;
}

/**
 * bind_previous_governor() - bind the previous governor of the thermal zone
 * @tz:		a valid pointer to a struct thermal_zone_device
 * @failed_gov_name:	the name of the governor that failed to register
 *
 * Register the previous governor of the thermal zone after a new
 * governor has failed to be bound.
 */
static void bind_previous_governor(struct thermal_zone_device *tz,
				   const char *failed_gov_name)
{
	if (tz->governor && tz->governor->bind_to_tz) {
		if (tz->governor->bind_to_tz(tz)) {
			dev_err(&tz->device,
				"governor %s failed to bind and the previous one (%s) failed to bind again, thermal zone %s has no governor\n",
				failed_gov_name, tz->governor->name, tz->type);
			tz->governor = NULL;
		}
	}
}

/**
 * thermal_set_governor() - Switch to another governor
 * @tz:		a valid pointer to a struct thermal_zone_device
 * @new_gov:	pointer to the new governor
 *
 * Change the governor of thermal zone @tz.
 *
 * Return: 0 on success, an error if the new governor's bind_to_tz() failed.
 */
static int thermal_set_governor(struct thermal_zone_device *tz,
				struct thermal_governor *new_gov)
{
	int ret = 0;

	if (tz->governor && tz->governor->unbind_from_tz)
		tz->governor->unbind_from_tz(tz);

	if (new_gov && new_gov->bind_to_tz) {
		ret = new_gov->bind_to_tz(tz);
		if (ret) {
			bind_previous_governor(tz, new_gov->name);

			return ret;
		}
	}

	tz->governor = new_gov;

	return ret;
}

int thermal_register_governor(struct thermal_governor *governor)
{
	int err;
	const char *name;
	struct thermal_zone_device *pos;

	if (!governor)
		return -EINVAL;

	mutex_lock(&thermal_governor_lock);

	err = -EBUSY;
	if (__find_governor(governor->name) == NULL) {
		err = 0;
		list_add(&governor->governor_list, &thermal_governor_list);
		if (!def_governor && !strncmp(governor->name,
			DEFAULT_THERMAL_GOVERNOR, THERMAL_NAME_LENGTH))
			def_governor = governor;
	}

	mutex_lock(&thermal_list_lock);

	list_for_each_entry(pos, &thermal_tz_list, node) {
		/*
		 * only thermal zones with specified tz->tzp->governor_name
		 * may run with tz->govenor unset
		 */
		if (pos->governor)
			continue;

		name = pos->tzp->governor_name;

		if (!strncasecmp(name, governor->name, THERMAL_NAME_LENGTH)) {
			int ret;

			ret = thermal_set_governor(pos, governor);
			if (ret)
				dev_err(&pos->device,
					"Failed to set governor %s for thermal zone %s: %d\n",
					governor->name, pos->type, ret);
		}
	}

	mutex_unlock(&thermal_list_lock);
	mutex_unlock(&thermal_governor_lock);

	return err;
}

void thermal_unregister_governor(struct thermal_governor *governor)
{
	struct thermal_zone_device *pos;

	if (!governor)
		return;

	mutex_lock(&thermal_governor_lock);

	if (__find_governor(governor->name) == NULL)
		goto exit;

	mutex_lock(&thermal_list_lock);

	list_for_each_entry(pos, &thermal_tz_list, node) {
		if (!strncasecmp(pos->governor->name, governor->name,
						THERMAL_NAME_LENGTH))
			thermal_set_governor(pos, NULL);
	}

	mutex_unlock(&thermal_list_lock);
	list_del(&governor->governor_list);
exit:
	mutex_unlock(&thermal_governor_lock);
	return;
}

static int get_idr(struct idr *idr, struct mutex *lock, int *id)
{
	int ret;

	if (lock)
		mutex_lock(lock);
	ret = idr_alloc(idr, NULL, 0, 0, GFP_KERNEL);/*lint !e456 */
	if (lock)
		mutex_unlock(lock);
	if (unlikely(ret < 0))/*lint !e456 */
		return ret;/*lint !e454 */
	*id = ret;
	return 0;/*lint !e454 */
}

static void release_idr(struct idr *idr, struct mutex *lock, int id)
{
	if (lock)
		mutex_lock(lock);
	idr_remove(idr, id);/*lint !e456 */
	if (lock)
		mutex_unlock(lock);
}/*lint !e456 !e454 */

int get_tz_trend(struct thermal_zone_device *tz, int trip)
{
	enum thermal_trend trend;

	if (tz->emul_temperature || !tz->ops->get_trend ||
	    tz->ops->get_trend(tz, trip, &trend)) {
		if (tz->temperature > tz->last_temperature)
			trend = THERMAL_TREND_RAISING;
		else if (tz->temperature < tz->last_temperature)
			trend = THERMAL_TREND_DROPPING;
		else
			trend = THERMAL_TREND_STABLE;
	}

	return trend;
}
EXPORT_SYMBOL(get_tz_trend);

struct thermal_instance *get_thermal_instance(struct thermal_zone_device *tz,
			struct thermal_cooling_device *cdev, int trip)
{
	struct thermal_instance *pos = NULL;
	struct thermal_instance *target_instance = NULL;

	mutex_lock(&tz->lock);
	mutex_lock(&cdev->lock);

	list_for_each_entry(pos, &tz->thermal_instances, tz_node) {
		if (pos->tz == tz && pos->trip == trip && pos->cdev == cdev) {
			target_instance = pos;
			break;
		}
	}

	mutex_unlock(&cdev->lock);
	mutex_unlock(&tz->lock);

	return target_instance;
}
EXPORT_SYMBOL(get_thermal_instance);

static void print_bind_err_msg(struct thermal_zone_device *tz,
			struct thermal_cooling_device *cdev, int ret)
{
	dev_err(&tz->device, "binding zone %s with cdev %s failed:%d\n",
				tz->type, cdev->type, ret);
}

static void __bind(struct thermal_zone_device *tz, int mask,
			struct thermal_cooling_device *cdev,
			unsigned long *limits,
			unsigned int weight)
{
	int i, ret;

	for (i = 0; i < tz->trips; i++) {
		if (mask & (1 << i)) {
			unsigned long upper, lower;

			upper = THERMAL_NO_LIMIT;
			lower = THERMAL_NO_LIMIT;
#ifndef CONFIG_HISI_IPA_THERMAL
			if (limits) {
				lower = limits[i * 2];/*lint !e679 */
				upper = limits[i * 2 + 1];/*lint !e679 */
			}
#endif
			ret = thermal_zone_bind_cooling_device(tz, i, cdev,
							       upper, lower,
							       weight);
			if (ret)
				print_bind_err_msg(tz, cdev, ret);
		}
	}
}

static void __unbind(struct thermal_zone_device *tz, int mask,
			struct thermal_cooling_device *cdev)
{
	int i;

	for (i = 0; i < tz->trips; i++)
		if (mask & (1 << i))
			thermal_zone_unbind_cooling_device(tz, i, cdev);
}

static void bind_cdev(struct thermal_cooling_device *cdev)
{
	int i, ret;
	const struct thermal_zone_params *tzp;
	struct thermal_zone_device *pos = NULL;

	mutex_lock(&thermal_list_lock);

	list_for_each_entry(pos, &thermal_tz_list, node) {
		if (!pos->tzp && !pos->ops->bind)
			continue;

		if (pos->ops->bind) {
			ret = pos->ops->bind(pos, cdev);
			if (ret)
				print_bind_err_msg(pos, cdev, ret);
			continue;
		}

		tzp = pos->tzp;
		if (!tzp || !tzp->tbp)
			continue;

		for (i = 0; i < tzp->num_tbps; i++) {
			if (tzp->tbp[i].cdev || !tzp->tbp[i].match)
				continue;
			if (tzp->tbp[i].match(pos, cdev))
				continue;
			tzp->tbp[i].cdev = cdev;
			__bind(pos, tzp->tbp[i].trip_mask, cdev,
			       tzp->tbp[i].binding_limits,
			       tzp->tbp[i].weight);
		}
	}

	mutex_unlock(&thermal_list_lock);
}

static void bind_tz(struct thermal_zone_device *tz)
{
	int i, ret;
	struct thermal_cooling_device *pos = NULL;
	const struct thermal_zone_params *tzp = tz->tzp;

	if (!tzp && !tz->ops->bind)
		return;

	mutex_lock(&thermal_list_lock);

	/* If there is ops->bind, try to use ops->bind */
	if (tz->ops->bind) {
		list_for_each_entry(pos, &thermal_cdev_list, node) {
			ret = tz->ops->bind(tz, pos);
			if (ret)
				print_bind_err_msg(tz, pos, ret);
		}
		goto exit;
	}

	if (!tzp || !tzp->tbp)
		goto exit;

	list_for_each_entry(pos, &thermal_cdev_list, node) {
		for (i = 0; i < tzp->num_tbps; i++) {
			if (tzp->tbp[i].cdev || !tzp->tbp[i].match)
				continue;
			if (tzp->tbp[i].match(tz, pos))
				continue;
			tzp->tbp[i].cdev = pos;
			__bind(tz, tzp->tbp[i].trip_mask, pos,
			       tzp->tbp[i].binding_limits,
			       tzp->tbp[i].weight);
		}
	}
exit:
	mutex_unlock(&thermal_list_lock);
}

static void thermal_zone_device_set_polling(struct thermal_zone_device *tz,
					    int delay)
{
#ifdef CONFIG_HISI_IPA_THERMAL
	if (delay > 1000)
		mod_delayed_work(system_freezable_power_efficient_wq, &tz->poll_queue,
				 round_jiffies(msecs_to_jiffies(delay)));
	else if (delay)
		mod_delayed_work(system_freezable_power_efficient_wq, &tz->poll_queue,
				 msecs_to_jiffies(delay));
	else
		cancel_delayed_work(&tz->poll_queue);
#else
	if (delay > 1000)
		mod_delayed_work(system_freezable_wq, &tz->poll_queue,
				 round_jiffies(msecs_to_jiffies(delay)));
	else if (delay)
		mod_delayed_work(system_freezable_wq, &tz->poll_queue,
				 msecs_to_jiffies(delay));
	else
		cancel_delayed_work(&tz->poll_queue);
#endif
}

static void monitor_thermal_zone(struct thermal_zone_device *tz)
{
	mutex_lock(&tz->lock);

	if (tz->passive)
		thermal_zone_device_set_polling(tz, tz->passive_delay);
	else if (tz->polling_delay)
		thermal_zone_device_set_polling(tz, tz->polling_delay);
	else
		thermal_zone_device_set_polling(tz, 0);

	mutex_unlock(&tz->lock);
}

static void handle_non_critical_trips(struct thermal_zone_device *tz,
			int trip, enum thermal_trip_type trip_type)
{
	tz->governor ? tz->governor->throttle(tz, trip) :
		       def_governor->throttle(tz, trip);
}

static void handle_critical_trips(struct thermal_zone_device *tz,
				int trip, enum thermal_trip_type trip_type)
{
	int trip_temp;

	tz->ops->get_trip_temp(tz, trip, &trip_temp);

	/* If we have not crossed the trip_temp, we do not care. */
	if (trip_temp <= 0 || tz->temperature < trip_temp)
		return;

	trace_thermal_zone_trip(tz, trip, trip_type);

	if (tz->ops->notify)
		tz->ops->notify(tz, trip, trip_type);

	if (trip_type == THERMAL_TRIP_CRITICAL) {
		dev_emerg(&tz->device,
			  "critical temperature reached(%d C),shutting down\n",
			  tz->temperature / 1000);
		orderly_poweroff(true);
	}
}

static void handle_thermal_trip(struct thermal_zone_device *tz, int trip)
{
	enum thermal_trip_type type;

	/* Ignore disabled trip points */
	if (test_bit(trip, &tz->trips_disabled))
		return;

	tz->ops->get_trip_type(tz, trip, &type);

	if (type == THERMAL_TRIP_CRITICAL || type == THERMAL_TRIP_HOT)
		handle_critical_trips(tz, trip, type);
	else
		handle_non_critical_trips(tz, trip, type);
	/*
	 * Alright, we handled this trip successfully.
	 * So, start monitoring again.
	 */
	monitor_thermal_zone(tz);
}

/**
 * thermal_zone_get_temp() - returns the temperature of a thermal zone
 * @tz: a valid pointer to a struct thermal_zone_device
 * @temp: a valid pointer to where to store the resulting temperature.
 *
 * When a valid thermal zone reference is passed, it will fetch its
 * temperature and fill @temp.
 *
 * Return: On success returns 0, an error code otherwise
 */
int thermal_zone_get_temp(struct thermal_zone_device *tz, int *temp)
{
	int ret = -EINVAL;
	int count;
	int crit_temp = INT_MAX;
	enum thermal_trip_type type;

	if (!tz || IS_ERR(tz) || !tz->ops->get_temp)
		goto exit;

	mutex_lock(&tz->lock);

	ret = tz->ops->get_temp(tz, temp);

	if (IS_ENABLED(CONFIG_THERMAL_EMULATION) && tz->emul_temperature) {
		for (count = 0; count < tz->trips; count++) {
			ret = tz->ops->get_trip_type(tz, count, &type);
			if (!ret && type == THERMAL_TRIP_CRITICAL) {
				ret = tz->ops->get_trip_temp(tz, count,
						&crit_temp);
				break;
			}
		}

		/*
		 * Only allow emulating a temperature when the real temperature
		 * is below the critical temperature so that the emulation code
		 * cannot hide critical conditions.
		 */
		if (!ret && *temp < crit_temp)
			*temp = tz->emul_temperature;
	}

	mutex_unlock(&tz->lock);
exit:
	return ret;
}
EXPORT_SYMBOL_GPL(thermal_zone_get_temp);

void thermal_zone_set_trips(struct thermal_zone_device *tz)
{
	int low = -INT_MAX;
	int high = INT_MAX;
	int trip_temp, hysteresis;
	int i, ret;

	mutex_lock(&tz->lock);

	if (!tz->ops->set_trips || !tz->ops->get_trip_hyst)
		goto exit;

	for (i = 0; i < tz->trips; i++) {
		int trip_low;

		tz->ops->get_trip_temp(tz, i, &trip_temp);
		tz->ops->get_trip_hyst(tz, i, &hysteresis);

		trip_low = trip_temp - hysteresis;

		if (trip_low < tz->temperature && trip_low > low)
			low = trip_low;

		if (trip_temp > tz->temperature && trip_temp < high)
			high = trip_temp;
	}

	/* No need to change trip points */
	if (tz->prev_low_trip == low && tz->prev_high_trip == high)
		goto exit;

	tz->prev_low_trip = low;
	tz->prev_high_trip = high;

	dev_dbg(&tz->device,
		"new temperature boundaries: %d < x < %d\n", low, high);

	/*
	 * Set a temperature window. When this window is left the driver
	 * must inform the thermal core via thermal_zone_device_update.
	 */
	ret = tz->ops->set_trips(tz, low, high);
	if (ret)
		dev_err(&tz->device, "Failed to set trips: %d\n", ret);

exit:
	mutex_unlock(&tz->lock);
}
EXPORT_SYMBOL_GPL(thermal_zone_set_trips);

static void update_temperature(struct thermal_zone_device *tz)
{
	int temp, ret;

	ret = thermal_zone_get_temp(tz, &temp);
	if (ret) {
		if (ret != -EAGAIN)
			dev_warn(&tz->device,
				 "failed to read out thermal zone (%d)\n",
				 ret);
		return;
	}

	mutex_lock(&tz->lock);
	tz->last_temperature = tz->temperature;
	tz->temperature = temp;
	mutex_unlock(&tz->lock);

	trace_thermal_temperature(tz);
	if (tz->last_temperature == THERMAL_TEMP_INVALID)
		dev_dbg(&tz->device, "last_temperature N/A, current_temperature=%d\n",
			tz->temperature);
	else
		dev_dbg(&tz->device, "last_temperature=%d, current_temperature=%d\n",
			tz->last_temperature, tz->temperature);
}

static void thermal_zone_device_reset(struct thermal_zone_device *tz)
{
	struct thermal_instance *pos;

	tz->temperature = THERMAL_TEMP_INVALID;
	tz->passive = 0;
	list_for_each_entry(pos, &tz->thermal_instances, tz_node)
		pos->initialized = false;
}

#ifdef CONFIG_HISI_IPA_THERMAL
int nametoactor(const char* weight_attr_name)
{
	int actor_id = -1;

	if (!strncmp(weight_attr_name, IPA_GPU_WEIGHT_NAME, sizeof(IPA_GPU_WEIGHT_NAME) - 1))
			actor_id = IPA_GPU;
	else if (!strncmp(weight_attr_name, IPA_CLUSTER0_WEIGHT_NAME, sizeof(IPA_CLUSTER0_WEIGHT_NAME) - 1))
			actor_id = IPA_CLUSTER0;
	else if (!strncmp(weight_attr_name, IPA_CLUSTER1_WEIGHT_NAME, sizeof(IPA_CLUSTER1_WEIGHT_NAME) - 1))
			actor_id = IPA_CLUSTER1;
#ifdef CONFIG_HISI_THERMAL_TRIPPLE_CLUSTERS
	else if (!strncmp(weight_attr_name, IPA_CLUSTER2_WEIGHT_NAME, sizeof(IPA_CLUSTER2_WEIGHT_NAME) - 1))
			actor_id = IPA_CLUSTER2;
#endif
	else
		actor_id = -1;

	return actor_id;
}
#endif

#ifdef CONFIG_HISI_IPA_THERMAL
void restore_actor_weights(struct thermal_zone_device *tz)
{
	struct thermal_instance *pos;
	int actor_id = -1;

	list_for_each_entry(pos, &tz->thermal_instances, tz_node) {
		actor_id = nametoactor(pos->weight_attr_name);
		if(actor_id != -1)
			pos->weight = g_ipa_normal_weights[actor_id];
	}
}

void update_actor_weights(struct thermal_zone_device *tz)
{
	struct thermal_instance *pos;
	bool bGPUBounded = false;
	struct thermal_cooling_device *cdev;
	int actor_id = -1;

	list_for_each_entry(pos, &tz->thermal_instances, tz_node) {
		cdev = pos->cdev;
		if (!strncmp(pos->weight_attr_name, IPA_GPU_WEIGHT_NAME,sizeof(IPA_GPU_WEIGHT_NAME) - 1) && cdev->bound_event) {
			bGPUBounded = true;
			break;
		}
	}

	list_for_each_entry(pos, &tz->thermal_instances, tz_node) {
		actor_id = nametoactor(pos->weight_attr_name);

		if (bGPUBounded) {
			if (actor_id != -1)
				pos->weight = g_ipa_gpu_boost_weights[actor_id];
		} else {
			if (actor_id != -1)
				pos->weight = g_ipa_normal_weights[actor_id];
		}
	}
}
#endif

void thermal_zone_device_update(struct thermal_zone_device *tz,
				enum thermal_notify_event event)
{
	int count;

	if (atomic_read(&in_suspend))
		return;

	if (!tz->ops->get_temp)
		return;

	update_temperature(tz);

	thermal_zone_set_trips(tz);

	tz->notify_event = event;
#ifdef CONFIG_HISI_IPA_THERMAL
	update_actor_weights(tz);
#endif

	for (count = 0; count < tz->trips; count++)
		handle_thermal_trip(tz, count);
}
EXPORT_SYMBOL_GPL(thermal_zone_device_update);

static void thermal_zone_device_check(struct work_struct *work)
{
	struct thermal_zone_device *tz = container_of(work, struct
						      thermal_zone_device,
						      poll_queue.work);
#ifdef CONFIG_HISI_IPA_THERMAL
	mutex_lock(&tz->lock);
	tz->tzp->boost = THERMAL_BOOST_DISABLED;
	mutex_unlock(&tz->lock);
#endif
	thermal_zone_device_update(tz, THERMAL_EVENT_UNSPECIFIED);
}

/* sys I/F for thermal zone */

#define to_thermal_zone(_dev) \
	container_of(_dev, struct thermal_zone_device, device)

static ssize_t
type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);

	return sprintf(buf, "%s\n", tz->type);/*lint !e421 */
}

static ssize_t
temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	int temperature, ret;

	ret = thermal_zone_get_temp(tz, &temperature);

	if (ret)
		return ret;

	return sprintf(buf, "%d\n", temperature);/*lint !e421 */
}

static ssize_t
mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	enum thermal_device_mode mode;
	int result;

	if (!tz->ops->get_mode)
		return -EPERM;

	result = tz->ops->get_mode(tz, &mode);
	if (result)
		return result;

	switch (mode) {
	case THERMAL_DEVICE_DISABLED:
		return sprintf(buf, "%s\n",  "disabled");/*lint !e421 */
	case THERMAL_DEVICE_ENABLED:
		return sprintf(buf, "%s\n",  "enabled");/*lint !e421 */
	default:
		return sprintf(buf, "%s\n",  "disabled");/*lint !e421 */
	}

}

static ssize_t
mode_store(struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	int result;

	if (!tz->ops->set_mode)
		return -EPERM;

	if (!strncmp(buf, "enabled", sizeof("enabled") - 1)) {
#ifdef CONFIG_HISI_IPA_THERMAL
		restore_actor_weights(tz);
#endif
		result = tz->ops->set_mode(tz, THERMAL_DEVICE_ENABLED);
	} else if (!strncmp(buf, "disabled", sizeof("disabled") - 1)) {
#ifdef CONFIG_HISI_IPA_THERMAL
		restore_actor_weights(tz);
#endif
		result = tz->ops->set_mode(tz, THERMAL_DEVICE_DISABLED);
	} else
		result = -EINVAL;

	if (result)
		return result;

#ifdef CONFIG_HISI_IPA_THERMAL
	if(strncmp(tz->governor->name, USER_SPACE_GOVERNOR, THERMAL_NAME_LENGTH)) {/*[false alarm]*/
		mutex_lock(&tz->lock);
		ipa_freq_limit_reset(tz);
		mutex_unlock(&tz->lock);
	}
#endif
	return count;
}

#ifdef CONFIG_HISI_IPA_THERMAL
static ssize_t
boost_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	enum thermal_boost_mode mode;

	if (!tz->tzp)
		return -EIO;

	mutex_lock(&tz->lock);
	mode = tz->tzp->boost;
	mutex_unlock(&tz->lock);
	return sprintf(buf, "%s\n", mode == THERMAL_BOOST_ENABLED ? "enabled"
		       : "disabled");/*lint !e421 */
}

static ssize_t
boost_store(struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	int result = 0;

	if (!tz->tzp)
		return -EIO;

	mutex_lock(&thermal_boost_lock);

	if (!strncmp(buf, "enabled", sizeof("enabled") - 1)) {
		mutex_lock(&tz->lock);
		if (THERMAL_BOOST_ENABLED == tz->tzp->boost) {
				pr_info("perfhub boost high frequency timeout[%d]\n", tz->tzp->boost_timeout);
			}

		tz->tzp->boost = THERMAL_BOOST_ENABLED;
		if (0 == tz->tzp->boost_timeout)
			tz->tzp->boost_timeout = 100;

		thermal_zone_device_set_polling(tz, tz->tzp->boost_timeout);
		ipa_freq_limit_reset(tz);
		mutex_unlock(&tz->lock);
	} else if (!strncmp(buf, "disabled", sizeof("disabled") - 1)) {
		tz->tzp->boost = THERMAL_BOOST_DISABLED;
		thermal_zone_device_update(tz, THERMAL_EVENT_UNSPECIFIED);
	} else
		result = -EINVAL;

	mutex_unlock(&thermal_boost_lock);

	if (result)
		return result;

	trace_IPA_boost(current->pid, tz, tz->tzp->boost,
			tz->tzp->boost_timeout);

	return count;
}
static DEVICE_ATTR(boost, 0644, boost_show, boost_store);
#endif

static ssize_t
trip_point_type_show(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	enum thermal_trip_type type;
	int trip, result;

	if (!tz->ops->get_trip_type)
		return -EPERM;

	if (!sscanf(attr->attr.name, "trip_point_%d_type", &trip))
		return -EINVAL;

	result = tz->ops->get_trip_type(tz, trip, &type);
	if (result)
		return result;

	switch (type) {
	case THERMAL_TRIP_CRITICAL:
		return sprintf(buf, "critical\n");/*lint !e421 */
	case THERMAL_TRIP_HOT:
		return sprintf(buf, "hot\n");/*lint !e421 */
#if defined(CONFIG_HISI_THERMAL_TSENSOR) || defined(CONFIG_HISI_THERMAL_PERIPHERAL)
	case THERMAL_TRIP_CONFIGURABLE_HI:
		return sprintf(buf, "configurable_hi\n");/*lint !e421 */
	case THERMAL_TRIP_CONFIGURABLE_LOW:
		return sprintf(buf, "configurable_low\n");/*lint !e421 */
	case THERMAL_TRIP_CRITICAL_LOW:
		return sprintf(buf, "critical_low\n");/*lint !e421 */
#endif
#ifdef CONFIG_HISI_THERMAL_TRIP
	case THERMAL_TRIP_THROTTLING:
		return sprintf(buf, "throttling\n");/*lint !e421 */
	case THERMAL_TRIP_SHUTDOWN:
		return sprintf(buf, "shutdown\n");/*lint !e421 */
	case THERMAL_TRIP_BELOW_VR_MIN:
		return sprintf(buf, "below_vr_min\n");/*lint !e421 */
	case THERMAL_TRIP_OVER_SKIN:
		return sprintf(buf, "over_skin\n");/*lint !e421 */
#endif
	case THERMAL_TRIP_PASSIVE:
		return sprintf(buf, "passive\n");/*lint !e421 */
	case THERMAL_TRIP_ACTIVE:
		return sprintf(buf, "active\n");/*lint !e421 */
	default:
		return sprintf(buf, "unknown\n");/*lint !e421 */
	}
}

#ifdef CONFIG_HISI_THERMAL_TSENSOR
static ssize_t
trip_point_type_activate(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	int trip = 0, result = 0;

	if (!tz->ops->activate_trip_type)
		return -EPERM;

	if (!sscanf(attr->attr.name, "trip_point_%d_type", &trip))
		return -EINVAL;

	if (!strncmp(buf, "enabled", strlen("enabled"))) {
		result = tz->ops->activate_trip_type(tz, trip,
					THERMAL_TRIP_ACTIVATION_ENABLED);
		}
	else if (!strncmp(buf, "disabled", strlen("disabled"))) {
		result = tz->ops->activate_trip_type(tz, trip,
					THERMAL_TRIP_ACTIVATION_DISABLED);
		}
	else{
		result = -EINVAL;
		}
	if (result)
		return result;
	return count;
}
#endif

static ssize_t
trip_point_temp_store(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	int trip, ret;
	int temperature;

	if (!tz->ops->set_trip_temp)
		return -EPERM;

	if (!sscanf(attr->attr.name, "trip_point_%d_temp", &trip))
		return -EINVAL;

	if (kstrtoint(buf, 10, &temperature))
		return -EINVAL;

	ret = tz->ops->set_trip_temp(tz, trip, temperature);
#ifdef CONFIG_HISI_IPA_THERMAL
	update_pid_value(tz);
#endif
	if (ret)
		return ret;

	thermal_zone_device_update(tz, THERMAL_EVENT_UNSPECIFIED);

	return count;
}

static ssize_t
trip_point_temp_show(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	int trip, ret;
	int temperature;

	if (!tz->ops->get_trip_temp)
		return -EPERM;

	if (!sscanf(attr->attr.name, "trip_point_%d_temp", &trip))
		return -EINVAL;

	ret = tz->ops->get_trip_temp(tz, trip, &temperature);

	if (ret)
		return ret;

	return sprintf(buf, "%d\n", temperature);/*lint !e421 */
}

static ssize_t
trip_point_hyst_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	int trip, ret;
	int temperature;

	if (!tz->ops->set_trip_hyst)
		return -EPERM;

	if (!sscanf(attr->attr.name, "trip_point_%d_hyst", &trip))
		return -EINVAL;

	if (kstrtoint(buf, 10, &temperature))
		return -EINVAL;

	/*
	 * We are not doing any check on the 'temperature' value
	 * here. The driver implementing 'set_trip_hyst' has to
	 * take care of this.
	 */
	ret = tz->ops->set_trip_hyst(tz, trip, temperature);

	if (!ret)
		thermal_zone_set_trips(tz);

	return ret ? ret : count;
}

static ssize_t
trip_point_hyst_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	int trip, ret;
	int temperature;

	if (!tz->ops->get_trip_hyst)
		return -EPERM;

	if (!sscanf(attr->attr.name, "trip_point_%d_hyst", &trip))
		return -EINVAL;

	ret = tz->ops->get_trip_hyst(tz, trip, &temperature);

	return ret ? ret : sprintf(buf, "%d\n", temperature);/*lint !e421 */
}

static ssize_t
passive_store(struct device *dev, struct device_attribute *attr,
		    const char *buf, size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	struct thermal_cooling_device *cdev = NULL;
	int state;

	if (!sscanf(buf, "%d\n", &state))
		return -EINVAL;

	/* sanity check: values below 1000 millicelcius don't make sense
	 * and can cause the system to go into a thermal heart attack
	 */
	if (state && state < 1000)
		return -EINVAL;

	if (state && !tz->forced_passive) {
		mutex_lock(&thermal_list_lock);
		list_for_each_entry(cdev, &thermal_cdev_list, node) {
			if (!strncmp("Processor", cdev->type,
				     sizeof("Processor")))
				thermal_zone_bind_cooling_device(tz,
						THERMAL_TRIPS_NONE, cdev,
						THERMAL_NO_LIMIT,
						THERMAL_NO_LIMIT,
						THERMAL_WEIGHT_DEFAULT);
		}
		mutex_unlock(&thermal_list_lock);
		if (!tz->passive_delay)
			tz->passive_delay = 1000;
	} else if (!state && tz->forced_passive) {
		mutex_lock(&thermal_list_lock);
		list_for_each_entry(cdev, &thermal_cdev_list, node) {
			if (!strncmp("Processor", cdev->type,
				     sizeof("Processor")))
				thermal_zone_unbind_cooling_device(tz,
								   THERMAL_TRIPS_NONE,
								   cdev);
		}
		mutex_unlock(&thermal_list_lock);
		tz->passive_delay = 0;
	}

	tz->forced_passive = state;

	thermal_zone_device_update(tz, THERMAL_EVENT_UNSPECIFIED);

	return count;
}

static ssize_t
passive_show(struct device *dev, struct device_attribute *attr,
		   char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);

	return sprintf(buf, "%d\n", tz->forced_passive);/*lint !e421 */
}

static ssize_t
policy_store(struct device *dev, struct device_attribute *attr,
		    const char *buf, size_t count)
{
	int ret = -EINVAL;
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	struct thermal_governor *gov;
	char name[THERMAL_NAME_LENGTH];

	snprintf(name, sizeof(name), "%s", buf);

	mutex_lock(&thermal_governor_lock);
	mutex_lock(&tz->lock);

	gov = __find_governor(strim(name));
	if (!gov)
		goto exit;

	ret = thermal_set_governor(tz, gov);
	if (!ret)
		ret = count;

exit:
	mutex_unlock(&tz->lock);
	mutex_unlock(&thermal_governor_lock);
	return ret;
}

static ssize_t
policy_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);

	return sprintf(buf, "%s\n", tz->governor->name);/*lint !e421 */
}

static ssize_t
available_policies_show(struct device *dev, struct device_attribute *devattr,
			char *buf)
{
	struct thermal_governor *pos;
	ssize_t count = 0;
	ssize_t size = PAGE_SIZE;

	mutex_lock(&thermal_governor_lock);

	list_for_each_entry(pos, &thermal_governor_list, governor_list) {
		size = PAGE_SIZE - count;
		count += scnprintf(buf + count, size, "%s ", pos->name);
	}
	count += scnprintf(buf + count, size, "\n");

	mutex_unlock(&thermal_governor_lock);

	return count;
}

static ssize_t
emul_temp_store(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	int ret = 0;
	int temperature;

	if (kstrtoint(buf, 10, &temperature))
		return -EINVAL;

#ifdef CONFIG_HISI_IPA_THERMAL
	temperature = thermal_zone_temp_check(temperature);
#endif
	if (!tz->ops->set_emul_temp) {
		mutex_lock(&tz->lock);
		tz->emul_temperature = temperature;
		mutex_unlock(&tz->lock);
	} else {
		ret = tz->ops->set_emul_temp(tz, temperature);
	}

	if (!ret)
		thermal_zone_device_update(tz, THERMAL_EVENT_UNSPECIFIED);

	return ret ? ret : count;
}
static DEVICE_ATTR(emul_temp, S_IWUSR, NULL, emul_temp_store);

static ssize_t
sustainable_power_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);

	if (tz->tzp)
		return sprintf(buf, "%u\n", tz->tzp->sustainable_power);/*lint !e421*/
	else
		return -EIO;
}

static ssize_t
sustainable_power_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	u32 sustainable_power;

	if (!tz->tzp)
		return -EIO;

	if (kstrtou32(buf, 10, &sustainable_power))
		return -EINVAL;

	tz->tzp->sustainable_power = sustainable_power;

#ifdef CONFIG_HISI_IPA_THERMAL
	update_pid_value(tz);
#endif

	return count;
}
static DEVICE_ATTR(sustainable_power, S_IWUSR | S_IRUGO, sustainable_power_show,
		sustainable_power_store);/*lint !e421 */

#ifdef CONFIG_HISI_IPA_THERMAL
static ssize_t
boost_timeout_show(struct device *dev, struct device_attribute *devattr,
			char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);

	if (tz->tzp)
		return snprintf(buf, PAGE_SIZE, "%u\n", tz->tzp->boost_timeout);
	else
		return -EIO;
}

#define BOOST_TIMEOUT_THRES	5000
static ssize_t
boost_timeout_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	u32 boost_timeout;

	if (!tz->tzp)
		return -EIO;

	if (kstrtou32(buf, 10, &boost_timeout))
		return -EINVAL;

	if(boost_timeout > BOOST_TIMEOUT_THRES)
		return -EINVAL;

	if(boost_timeout)
		tz->tzp->boost_timeout = boost_timeout;

	trace_IPA_boost(current->pid, tz, tz->tzp->boost,
			tz->tzp->boost_timeout);
	return count;
}
static DEVICE_ATTR(boost_timeout, (S_IWUSR | S_IRUGO), boost_timeout_show,
		boost_timeout_store);
#endif

#define create_s32_tzp_attr(name)					\
	static ssize_t							\
	name##_show(struct device *dev, struct device_attribute *devattr, \
		char *buf)						\
	{								\
	struct thermal_zone_device *tz = to_thermal_zone(dev);		\
									\
	if (tz->tzp)							\
		return sprintf(buf, "%d\n", tz->tzp->name);		\
	else								\
		return -EIO;						\
	}								\
									\
	static ssize_t							\
	name##_store(struct device *dev, struct device_attribute *devattr, \
		const char *buf, size_t count)				\
	{								\
		struct thermal_zone_device *tz = to_thermal_zone(dev);	\
		s32 value;						\
									\
		if (!tz->tzp)						\
			return -EIO;					\
									\
		if (kstrtos32(buf, 10, &value))				\
			return -EINVAL;					\
									\
		tz->tzp->name = value;					\
									\
		return count;						\
	}								\
	static DEVICE_ATTR(name, S_IWUSR | S_IRUGO, name##_show, name##_store)

create_s32_tzp_attr(k_po);/*lint !e421 */
create_s32_tzp_attr(k_pu);/*lint !e421 */
create_s32_tzp_attr(k_i);/*lint !e421 */
create_s32_tzp_attr(k_d);/*lint !e421 */
create_s32_tzp_attr(integral_cutoff);/*lint !e421 */
create_s32_tzp_attr(slope);/*lint !e421 */
create_s32_tzp_attr(offset);/*lint !e421 */
#undef create_s32_tzp_attr

static ssize_t cur_power_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct thermal_zone_device *tz;
	struct thermal_instance *instance;
	ssize_t buf_len;
	ssize_t size = PAGE_SIZE;
	if (dev == NULL || attr == NULL || buf == NULL){
		return -EIO;
	}

	tz = to_thermal_zone(dev);
	if (tz == NULL || tz->tzp == NULL){
		return -EIO;
	}

	mutex_lock(&tz->lock);
	buf_len = scnprintf(buf, size, "%llu,%u,", tz->tzp->cur_ipa_total_power, tz->tzp->check_cnt);
	if (buf_len < 0)
		goto unlock;
	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		struct thermal_cooling_device *cdev = instance->cdev;
		ssize_t ret;
		size = PAGE_SIZE - buf_len;
		if (size < 0 || size > (ssize_t)PAGE_SIZE)
			break;
		ret = scnprintf(buf + buf_len, size, "%llu,", cdev->cdev_cur_power);
		if (ret < 0)
			break;
		buf_len += ret;
	}
unlock:
	mutex_unlock(&tz->lock);

	if (buf_len > 0)
		buf[buf_len-1] = '\0';  ///set string end with '\0'
	return buf_len;
}

static DEVICE_ATTR(cur_power, S_IRUGO, cur_power_show, NULL);

static ssize_t
cur_enable_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);

	if (tz->tzp)
		return scnprintf(buf, PAGE_SIZE, "%u\n", tz->tzp->cur_enable);/*lint !e421*/
	else
		return -EIO;
}

static ssize_t
cur_enable_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	u32 cur_enable;

	if (!tz->tzp)
		return -EIO;

	if (kstrtou32(buf, 10, &cur_enable))
		return -EINVAL;

	tz->tzp->cur_enable = (cur_enable > 0);

	return count;
}

static DEVICE_ATTR(cur_enable, S_IWUSR | S_IRUGO, cur_enable_show, cur_enable_store);

static struct device_attribute *dev_tzp_attrs[] = {
	&dev_attr_sustainable_power,
	&dev_attr_k_po,
	&dev_attr_k_pu,
	&dev_attr_k_i,
	&dev_attr_k_d,
	&dev_attr_integral_cutoff,
	&dev_attr_slope,
	&dev_attr_offset,
#ifdef CONFIG_HISI_IPA_THERMAL
	&dev_attr_boost,
	&dev_attr_boost_timeout,
#endif
	&dev_attr_cur_power,
	&dev_attr_cur_enable,
};

static int create_tzp_attrs(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dev_tzp_attrs); i++) {/*lint !e574 */
		int ret;
		struct device_attribute *dev_attr = dev_tzp_attrs[i];

		ret = device_create_file(dev, dev_attr);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 * power_actor_get_max_power() - get the maximum power that a cdev can consume
 * @cdev:	pointer to &thermal_cooling_device
 * @tz:		a valid thermal zone device pointer
 * @max_power:	pointer in which to store the maximum power
 *
 * Calculate the maximum power consumption in milliwats that the
 * cooling device can currently consume and store it in @max_power.
 *
 * Return: 0 on success, -EINVAL if @cdev doesn't support the
 * power_actor API or -E* on other error.
 */
int power_actor_get_max_power(struct thermal_cooling_device *cdev,
			      struct thermal_zone_device *tz, u32 *max_power)
{
	if (!cdev_is_power_actor(cdev))
		return -EINVAL;

	return cdev->ops->state2power(cdev, tz, 0, max_power);
}

/**
 * power_actor_get_min_power() - get the mainimum power that a cdev can consume
 * @cdev:	pointer to &thermal_cooling_device
 * @tz:		a valid thermal zone device pointer
 * @min_power:	pointer in which to store the minimum power
 *
 * Calculate the minimum power consumption in milliwatts that the
 * cooling device can currently consume and store it in @min_power.
 *
 * Return: 0 on success, -EINVAL if @cdev doesn't support the
 * power_actor API or -E* on other error.
 */
int power_actor_get_min_power(struct thermal_cooling_device *cdev,
			      struct thermal_zone_device *tz, u32 *min_power)
{
	unsigned long max_state;
	int ret;

	if (!cdev_is_power_actor(cdev))
		return -EINVAL;

	ret = cdev->ops->get_max_state(cdev, &max_state);
	if (ret)
		return ret;

	return cdev->ops->state2power(cdev, tz, max_state, min_power);
}

/**
 * power_actor_set_power() - limit the maximum power that a cooling device can consume
 * @cdev:	pointer to &thermal_cooling_device
 * @instance:	thermal instance to update
 * @power:	the power in milliwatts
 *
 * Set the cooling device to consume at most @power milliwatts.
 *
 * Return: 0 on success, -EINVAL if the cooling device does not
 * implement the power actor API or -E* for other failures.
 */
int power_actor_set_power(struct thermal_cooling_device *cdev,
			  struct thermal_instance *instance, u32 power)
{
	unsigned long state;
	int ret;

	if (!cdev_is_power_actor(cdev))
		return -EINVAL;

	ret = cdev->ops->power2state(cdev, instance->tz, power, &state);
	if (ret)
		return ret;

	instance->target = state;
	mutex_lock(&cdev->lock);
	cdev->updated = false;
	mutex_unlock(&cdev->lock);
	thermal_cdev_update(cdev);

	return 0;
}

static DEVICE_ATTR(type, 0444, type_show, NULL);
static DEVICE_ATTR(temp, 0444, temp_show, NULL);
static DEVICE_ATTR(mode, 0644, mode_show, mode_store);
static DEVICE_ATTR(passive, S_IRUGO | S_IWUSR, passive_show, passive_store);
static DEVICE_ATTR(policy, S_IRUGO | S_IWUSR, policy_show, policy_store);
static DEVICE_ATTR(available_policies, S_IRUGO, available_policies_show, NULL);

/* sys I/F for cooling device */
#define to_cooling_device(_dev)	\
	container_of(_dev, struct thermal_cooling_device, device)

static ssize_t
thermal_cooling_device_type_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct thermal_cooling_device *cdev = to_cooling_device(dev);

	return sprintf(buf, "%s\n", cdev->type);/*lint !e421 */
}

static ssize_t
thermal_cooling_device_max_state_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct thermal_cooling_device *cdev = to_cooling_device(dev);
	unsigned long state;
	int ret;

	ret = cdev->ops->get_max_state(cdev, &state);
	if (ret)
		return ret;
	return sprintf(buf, "%ld\n", state);/*lint !e421 */
}

static ssize_t
thermal_cooling_device_cur_state_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct thermal_cooling_device *cdev = to_cooling_device(dev);
	unsigned long state;
	int ret;

	ret = cdev->ops->get_cur_state(cdev, &state);
	if (ret)
		return ret;
	return sprintf(buf, "%ld\n", state);/*lint !e421 */
}

static ssize_t
thermal_cooling_device_cur_state_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct thermal_cooling_device *cdev = to_cooling_device(dev);
	unsigned long state;
	int result;

	if (!sscanf(buf, "%ld\n", &state))
		return -EINVAL;

	if ((long)state < 0)
		return -EINVAL;

	result = cdev->ops->set_cur_state(cdev, state);
	if (result)
		return result;
	return count;
}

static struct device_attribute dev_attr_cdev_type =
__ATTR(type, 0444, thermal_cooling_device_type_show, NULL);
static DEVICE_ATTR(max_state, 0444,
		   thermal_cooling_device_max_state_show, NULL);
static DEVICE_ATTR(cur_state, 0644,
		   thermal_cooling_device_cur_state_show,
		   thermal_cooling_device_cur_state_store);

static ssize_t
thermal_cooling_device_trip_point_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct thermal_instance *instance;

	instance =
	    container_of(attr, struct thermal_instance, attr);

	if (instance->trip == THERMAL_TRIPS_NONE)
		return sprintf(buf, "-1\n");/*lint !e421 */
	else
		return sprintf(buf, "%d\n", instance->trip);/*lint !e421 */
}

static struct attribute *cooling_device_attrs[] = {
	&dev_attr_cdev_type.attr,
	&dev_attr_max_state.attr,
	&dev_attr_cur_state.attr,
	NULL,
};

static const struct attribute_group cooling_device_attr_group = {
	.attrs = cooling_device_attrs,
};

static const struct attribute_group *cooling_device_attr_groups[] = {
	&cooling_device_attr_group,
	NULL,
};

static ssize_t
thermal_cooling_device_weight_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct thermal_instance *instance;

	instance = container_of(attr, struct thermal_instance, weight_attr);

	return sprintf(buf, "%d\n", instance->weight);/*lint !e421 */
}

static ssize_t
thermal_cooling_device_weight_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
#ifdef CONFIG_HISI_IPA_THERMAL
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	enum thermal_device_mode mode;
	int result;
	int actor_id = -1;
#endif

	struct thermal_instance *instance;
	int ret, weight;

	ret = kstrtoint(buf, 0, &weight);
	if (ret)
		return ret;

#ifdef CONFIG_HISI_IPA_THERMAL
	if (!tz->ops->get_mode)
	return -EPERM;

	result = tz->ops->get_mode(tz, &mode);
	if (result)
		return result;
#endif

	instance = container_of(attr, struct thermal_instance, weight_attr);
	instance->weight = weight;

#ifdef CONFIG_HISI_IPA_THERMAL
	actor_id = nametoactor(instance->weight_attr_name);
	if(actor_id != -1)
		g_ipa_normal_weights[actor_id] = weight;
#endif

	return count;
}
/* Device management */

/**
 * thermal_zone_bind_cooling_device() - bind a cooling device to a thermal zone
 * @tz:		pointer to struct thermal_zone_device
 * @trip:	indicates which trip point the cooling devices is
 *		associated with in this thermal zone.
 * @cdev:	pointer to struct thermal_cooling_device
 * @upper:	the Maximum cooling state for this trip point.
 *		THERMAL_NO_LIMIT means no upper limit,
 *		and the cooling device can be in max_state.
 * @lower:	the Minimum cooling state can be used for this trip point.
 *		THERMAL_NO_LIMIT means no lower limit,
 *		and the cooling device can be in cooling state 0.
 * @weight:	The weight of the cooling device to be bound to the
 *		thermal zone. Use THERMAL_WEIGHT_DEFAULT for the
 *		default value
 *
 * This interface function bind a thermal cooling device to the certain trip
 * point of a thermal zone device.
 * This function is usually called in the thermal zone device .bind callback.
 *
 * Return: 0 on success, the proper error value otherwise.
 */
int thermal_zone_bind_cooling_device(struct thermal_zone_device *tz,
				     int trip,
				     struct thermal_cooling_device *cdev,
				     unsigned long upper, unsigned long lower,
				     unsigned int weight)
{
	struct thermal_instance *dev;
	struct thermal_instance *pos;
	struct thermal_zone_device *pos1;
	struct thermal_cooling_device *pos2;
	unsigned long max_state;
	int result, ret;

	if (trip >= tz->trips || (trip < 0 && trip != THERMAL_TRIPS_NONE))
		return -EINVAL;

	list_for_each_entry(pos1, &thermal_tz_list, node) {
		if (pos1 == tz)
			break;
	}
	list_for_each_entry(pos2, &thermal_cdev_list, node) {
		if (pos2 == cdev)
			break;
	}

	if (tz != pos1 || cdev != pos2)
		return -EINVAL;

	ret = cdev->ops->get_max_state(cdev, &max_state);
	if (ret)
		return ret;

	/* lower default 0, upper default max_state */
	lower = lower == THERMAL_NO_LIMIT ? 0 : lower;
	upper = upper == THERMAL_NO_LIMIT ? max_state : upper;

	if (lower > upper || upper > max_state)
		return -EINVAL;

	dev =
	    kzalloc(sizeof(struct thermal_instance), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;
	dev->tz = tz;
	dev->cdev = cdev;
	dev->trip = trip;
	dev->upper = upper;
	dev->lower = lower;
	dev->target = THERMAL_NO_TARGET;/*lint !e501 */
	dev->weight = weight;
#ifdef CONFIG_HISI_IPA_THERMAL
	dev->is_soc_cdev = thermal_of_get_cdev_type(tz, cdev);
#endif

	result = get_idr(&tz->idr, &tz->lock, &dev->id);
	if (result)
		goto free_mem;

	sprintf(dev->name, "cdev%d", dev->id);/*lint !e421 */
	result =
	    sysfs_create_link(&tz->device.kobj, &cdev->device.kobj, dev->name);
	if (result)
		goto release_idr;

	sprintf(dev->attr_name, "cdev%d_trip_point", dev->id);/*lint !e421 */
	sysfs_attr_init(&dev->attr.attr);
	dev->attr.attr.name = dev->attr_name;
	dev->attr.attr.mode = 0444;
	dev->attr.show = thermal_cooling_device_trip_point_show;
	result = device_create_file(&tz->device, &dev->attr);
	if (result)
		goto remove_symbol_link;

	sprintf(dev->weight_attr_name, "cdev%d_weight", dev->id);/*lint !e421 */
	sysfs_attr_init(&dev->weight_attr.attr);
	dev->weight_attr.attr.name = dev->weight_attr_name;
	dev->weight_attr.attr.mode = S_IWUSR | S_IRUGO;
	dev->weight_attr.show = thermal_cooling_device_weight_show;
	dev->weight_attr.store = thermal_cooling_device_weight_store;

	result = device_create_file(&tz->device, &dev->weight_attr);
	if (result)
		goto remove_trip_file;

	mutex_lock(&tz->lock);
	mutex_lock(&cdev->lock);
	list_for_each_entry(pos, &tz->thermal_instances, tz_node)
	    if (pos->tz == tz && pos->trip == trip && pos->cdev == cdev) {
		result = -EEXIST;
		break;
	}
	if (!result) {
		list_add_tail(&dev->tz_node, &tz->thermal_instances);
		list_add_tail(&dev->cdev_node, &cdev->thermal_instances);
		atomic_set(&tz->need_update, 1);
	}
	mutex_unlock(&cdev->lock);
	mutex_unlock(&tz->lock);

	if (!result)
		return 0;/*lint !e429 */

	device_remove_file(&tz->device, &dev->weight_attr);
remove_trip_file:
	device_remove_file(&tz->device, &dev->attr);
remove_symbol_link:
	sysfs_remove_link(&tz->device.kobj, dev->name);
release_idr:
	release_idr(&tz->idr, &tz->lock, dev->id);
free_mem:
	kfree(dev);
	return result;
}/*lint !e429 */
EXPORT_SYMBOL_GPL(thermal_zone_bind_cooling_device);

/**
 * thermal_zone_unbind_cooling_device() - unbind a cooling device from a
 *					  thermal zone.
 * @tz:		pointer to a struct thermal_zone_device.
 * @trip:	indicates which trip point the cooling devices is
 *		associated with in this thermal zone.
 * @cdev:	pointer to a struct thermal_cooling_device.
 *
 * This interface function unbind a thermal cooling device from the certain
 * trip point of a thermal zone device.
 * This function is usually called in the thermal zone device .unbind callback.
 *
 * Return: 0 on success, the proper error value otherwise.
 */
int thermal_zone_unbind_cooling_device(struct thermal_zone_device *tz,
				       int trip,
				       struct thermal_cooling_device *cdev)
{
	struct thermal_instance *pos, *next;

	mutex_lock(&tz->lock);
	mutex_lock(&cdev->lock);
	list_for_each_entry_safe(pos, next, &tz->thermal_instances, tz_node) {
		if (pos->tz == tz && pos->trip == trip && pos->cdev == cdev) {
			list_del(&pos->tz_node);
			list_del(&pos->cdev_node);
			mutex_unlock(&cdev->lock);
			mutex_unlock(&tz->lock);
			goto unbind;
		}
	}
	mutex_unlock(&cdev->lock);
	mutex_unlock(&tz->lock);

	return -ENODEV;

unbind:
	device_remove_file(&tz->device, &pos->weight_attr);
	device_remove_file(&tz->device, &pos->attr);
	sysfs_remove_link(&tz->device.kobj, pos->name);
	release_idr(&tz->idr, &tz->lock, pos->id);
	kfree(pos);
	return 0;
}
EXPORT_SYMBOL_GPL(thermal_zone_unbind_cooling_device);

static void thermal_release(struct device *dev)
{
	struct thermal_zone_device *tz;
	struct thermal_cooling_device *cdev;

	if (!strncmp(dev_name(dev), "thermal_zone",
		     sizeof("thermal_zone") - 1)) {
		tz = to_thermal_zone(dev);
		kfree(tz);
	} else if(!strncmp(dev_name(dev), "cooling_device",
			sizeof("cooling_device") - 1)){
		cdev = to_cooling_device(dev);
		kfree(cdev);
	}
}

static struct class thermal_class = {
	.name = "thermal",
	.dev_release = thermal_release,
};

/**
 * __thermal_cooling_device_register() - register a new thermal cooling device
 * @np:		a pointer to a device tree node.
 * @type:	the thermal cooling device type.
 * @devdata:	device private data.
 * @ops:		standard thermal cooling devices callbacks.
 *
 * This interface function adds a new thermal cooling device (fan/processor/...)
 * to /sys/class/thermal/ folder as cooling_device[0-*]. It tries to bind itself
 * to all the thermal zone devices registered at the same time.
 * It also gives the opportunity to link the cooling device to a device tree
 * node, so that it can be bound to a thermal zone created out of device tree.
 *
 * Return: a pointer to the created struct thermal_cooling_device or an
 * ERR_PTR. Caller must check return value with IS_ERR*() helpers.
 */
static struct thermal_cooling_device *
__thermal_cooling_device_register(struct device_node *np,
				  char *type, void *devdata,
				  const struct thermal_cooling_device_ops *ops)
{
	struct thermal_cooling_device *cdev;
	struct thermal_zone_device *pos = NULL;
	int result;

	if (type && strlen(type) >= THERMAL_NAME_LENGTH)
		return ERR_PTR(-EINVAL);

	if (!ops || !ops->get_max_state || !ops->get_cur_state ||
	    !ops->set_cur_state)
		return ERR_PTR(-EINVAL);

	cdev = kzalloc(sizeof(struct thermal_cooling_device), GFP_KERNEL);
	if (!cdev)
		return ERR_PTR(-ENOMEM);

	result = get_idr(&thermal_cdev_idr, &thermal_idr_lock, &cdev->id);
	if (result) {
		kfree(cdev);
		return ERR_PTR(result);
	}

	strlcpy(cdev->type, type ? : "", sizeof(cdev->type));
	mutex_init(&cdev->lock);
	INIT_LIST_HEAD(&cdev->thermal_instances);
	cdev->np = np;
	cdev->ops = ops;
	cdev->updated = false;
	cdev->device.class = &thermal_class;
	cdev->device.groups = cooling_device_attr_groups;
	cdev->devdata = devdata;
	cdev->bound_event = false;
	cdev->ipa_enabled = false;
	dev_set_name(&cdev->device, "cooling_device%d", cdev->id);
	result = device_register(&cdev->device);
	if (result) {
		release_idr(&thermal_cdev_idr, &thermal_idr_lock, cdev->id);
		kfree(cdev);
		return ERR_PTR(result);
	}

	/* Add 'this' new cdev to the global cdev list */
	mutex_lock(&thermal_list_lock);
	list_add(&cdev->node, &thermal_cdev_list);
	mutex_unlock(&thermal_list_lock);

	/* Update binding information for 'this' new cdev */
	bind_cdev(cdev);

	mutex_lock(&thermal_list_lock);
	list_for_each_entry(pos, &thermal_tz_list, node)
		if (atomic_cmpxchg(&pos->need_update, 1, 0))
			thermal_zone_device_update(pos,
						   THERMAL_EVENT_UNSPECIFIED);
	mutex_unlock(&thermal_list_lock);

	return cdev;
}

/**
 * thermal_cooling_device_register() - register a new thermal cooling device
 * @type:	the thermal cooling device type.
 * @devdata:	device private data.
 * @ops:		standard thermal cooling devices callbacks.
 *
 * This interface function adds a new thermal cooling device (fan/processor/...)
 * to /sys/class/thermal/ folder as cooling_device[0-*]. It tries to bind itself
 * to all the thermal zone devices registered at the same time.
 *
 * Return: a pointer to the created struct thermal_cooling_device or an
 * ERR_PTR. Caller must check return value with IS_ERR*() helpers.
 */
struct thermal_cooling_device *
thermal_cooling_device_register(char *type, void *devdata,
				const struct thermal_cooling_device_ops *ops)
{
	return __thermal_cooling_device_register(NULL, type, devdata, ops);
}
EXPORT_SYMBOL_GPL(thermal_cooling_device_register);

/**
 * thermal_of_cooling_device_register() - register an OF thermal cooling device
 * @np:		a pointer to a device tree node.
 * @type:	the thermal cooling device type.
 * @devdata:	device private data.
 * @ops:		standard thermal cooling devices callbacks.
 *
 * This function will register a cooling device with device tree node reference.
 * This interface function adds a new thermal cooling device (fan/processor/...)
 * to /sys/class/thermal/ folder as cooling_device[0-*]. It tries to bind itself
 * to all the thermal zone devices registered at the same time.
 *
 * Return: a pointer to the created struct thermal_cooling_device or an
 * ERR_PTR. Caller must check return value with IS_ERR*() helpers.
 */
struct thermal_cooling_device *
thermal_of_cooling_device_register(struct device_node *np,
				   char *type, void *devdata,
				   const struct thermal_cooling_device_ops *ops)
{
	return __thermal_cooling_device_register(np, type, devdata, ops);
}
EXPORT_SYMBOL_GPL(thermal_of_cooling_device_register);

/**
 * thermal_cooling_device_unregister - removes the registered thermal cooling device
 * @cdev:	the thermal cooling device to remove.
 *
 * thermal_cooling_device_unregister() must be called when the device is no
 * longer needed.
 */
void thermal_cooling_device_unregister(struct thermal_cooling_device *cdev)
{
	int i;
	const struct thermal_zone_params *tzp;
	struct thermal_zone_device *tz;
	struct thermal_cooling_device *pos = NULL;

	if (!cdev)
		return;

	mutex_lock(&thermal_list_lock);
	list_for_each_entry(pos, &thermal_cdev_list, node)
	    if (pos == cdev)
		break;
	if (pos != cdev) {
		/* thermal cooling device not found */
		mutex_unlock(&thermal_list_lock);
		return;
	}
	list_del(&cdev->node);

	/* Unbind all thermal zones associated with 'this' cdev */
	list_for_each_entry(tz, &thermal_tz_list, node) {
		if (tz->ops->unbind) {
			tz->ops->unbind(tz, cdev);
			continue;
		}

		if (!tz->tzp || !tz->tzp->tbp)
			continue;

		tzp = tz->tzp;
		for (i = 0; i < tzp->num_tbps; i++) {
			if (tzp->tbp[i].cdev == cdev) {
				__unbind(tz, tzp->tbp[i].trip_mask, cdev);
				tzp->tbp[i].cdev = NULL;
			}
		}
	}

	mutex_unlock(&thermal_list_lock);

	if (cdev->type[0])
		device_remove_file(&cdev->device, &dev_attr_cdev_type);
	device_remove_file(&cdev->device, &dev_attr_max_state);
	device_remove_file(&cdev->device, &dev_attr_cur_state);

	release_idr(&thermal_cdev_idr, &thermal_idr_lock, cdev->id);
	device_unregister(&cdev->device);
	return;
}
EXPORT_SYMBOL_GPL(thermal_cooling_device_unregister);

void thermal_cdev_update(struct thermal_cooling_device *cdev)
{
	struct thermal_instance *instance;
	unsigned long target = 0;

	mutex_lock(&cdev->lock);
	/* cooling device is updated*/
	if (cdev->updated) {
		mutex_unlock(&cdev->lock);
		return;
	}

	/* Make sure cdev enters the deepest cooling state */
	list_for_each_entry(instance, &cdev->thermal_instances, cdev_node) {
		dev_dbg(&cdev->device, "zone%d->target=%lu\n",
				instance->tz->id, instance->target);
		if (instance->target == THERMAL_NO_TARGET)/*lint !e501 */
			continue;
		if (instance->target > target)
			target = instance->target;
	}
	cdev->ops->set_cur_state(cdev, target);
	cdev->updated = true;
	mutex_unlock(&cdev->lock);
	trace_cdev_update(cdev, target);
	dev_dbg(&cdev->device, "set to state %lu\n", target);
}
EXPORT_SYMBOL(thermal_cdev_update);

/**
 * thermal_notify_framework - Sensor drivers use this API to notify framework
 * @tz:		thermal zone device
 * @trip:	indicates which trip point has been crossed
 *
 * This function handles the trip events from sensor drivers. It starts
 * throttling the cooling devices according to the policy configured.
 * For CRITICAL and HOT trip points, this notifies the respective drivers,
 * and does actual throttling for other trip points i.e ACTIVE and PASSIVE.
 * The throttling policy is based on the configured platform data; if no
 * platform data is provided, this uses the step_wise throttling policy.
 */
void thermal_notify_framework(struct thermal_zone_device *tz, int trip)
{
	handle_thermal_trip(tz, trip);
}
EXPORT_SYMBOL_GPL(thermal_notify_framework);

/**
 * create_trip_attrs() - create attributes for trip points
 * @tz:		the thermal zone device
 * @mask:	Writeable trip point bitmap.
 *
 * helper function to instantiate sysfs entries for every trip
 * point and its properties of a struct thermal_zone_device.
 *
 * Return: 0 on success, the proper error value otherwise.
 */
static int create_trip_attrs(struct thermal_zone_device *tz, int mask)
{
	int indx;
	int size = sizeof(struct thermal_attr) * tz->trips;

	tz->trip_type_attrs = kzalloc(size, GFP_KERNEL);
	if (!tz->trip_type_attrs)
		return -ENOMEM;

	tz->trip_temp_attrs = kzalloc(size, GFP_KERNEL);
	if (!tz->trip_temp_attrs) {
		kfree(tz->trip_type_attrs);
		return -ENOMEM;
	}

	if (tz->ops->get_trip_hyst) {
		tz->trip_hyst_attrs = kzalloc(size, GFP_KERNEL);
		if (!tz->trip_hyst_attrs) {
			kfree(tz->trip_type_attrs);
			kfree(tz->trip_temp_attrs);
			return -ENOMEM;
		}
	}


	for (indx = 0; indx < tz->trips; indx++) {
		/* create trip type attribute */
		snprintf(tz->trip_type_attrs[indx].name, THERMAL_NAME_LENGTH,
			 "trip_point_%d_type", indx);

		sysfs_attr_init(&tz->trip_type_attrs[indx].attr.attr);
		tz->trip_type_attrs[indx].attr.attr.name =
						tz->trip_type_attrs[indx].name;
		tz->trip_type_attrs[indx].attr.attr.mode = S_IRUGO;
		tz->trip_type_attrs[indx].attr.show = trip_point_type_show;
#ifdef CONFIG_HISI_THERMAL_TSENSOR
		if (IS_ENABLED(CONFIG_THERMAL_WRITABLE_TRIPS) &&
			mask & (1 << indx)) {
			if (tz->ops->activate_trip_type) {
				tz->trip_type_attrs[indx].attr.attr.mode |= S_IWUSR;
				tz->trip_type_attrs[indx].attr.store =
								trip_point_type_activate;
			}
		}
#endif

		device_create_file(&tz->device,
				   &tz->trip_type_attrs[indx].attr);

		/* create trip temp attribute */
		snprintf(tz->trip_temp_attrs[indx].name, THERMAL_NAME_LENGTH,
			 "trip_point_%d_temp", indx);

		sysfs_attr_init(&tz->trip_temp_attrs[indx].attr.attr);
		tz->trip_temp_attrs[indx].attr.attr.name =
						tz->trip_temp_attrs[indx].name;
		tz->trip_temp_attrs[indx].attr.attr.mode = S_IRUGO;
		tz->trip_temp_attrs[indx].attr.show = trip_point_temp_show;
		if (IS_ENABLED(CONFIG_THERMAL_WRITABLE_TRIPS) &&
		    mask & (1 << indx)) {
			tz->trip_temp_attrs[indx].attr.attr.mode |= S_IWUSR;
			tz->trip_temp_attrs[indx].attr.store =
							trip_point_temp_store;
		}

		device_create_file(&tz->device,
				   &tz->trip_temp_attrs[indx].attr);

		/* create Optional trip hyst attribute */
		if (!tz->ops->get_trip_hyst)
			continue;
		snprintf(tz->trip_hyst_attrs[indx].name, THERMAL_NAME_LENGTH,
			 "trip_point_%d_hyst", indx);

		sysfs_attr_init(&tz->trip_hyst_attrs[indx].attr.attr);
		tz->trip_hyst_attrs[indx].attr.attr.name =
					tz->trip_hyst_attrs[indx].name;
		tz->trip_hyst_attrs[indx].attr.attr.mode = S_IRUGO;
		tz->trip_hyst_attrs[indx].attr.show = trip_point_hyst_show;
		if (tz->ops->set_trip_hyst) {
			tz->trip_hyst_attrs[indx].attr.attr.mode |= S_IWUSR;
			tz->trip_hyst_attrs[indx].attr.store =
					trip_point_hyst_store;
		}

		device_create_file(&tz->device,
				   &tz->trip_hyst_attrs[indx].attr);
	}
	return 0;
}

static void remove_trip_attrs(struct thermal_zone_device *tz)
{
	int indx;

	for (indx = 0; indx < tz->trips; indx++) {
		device_remove_file(&tz->device,
				   &tz->trip_type_attrs[indx].attr);
		device_remove_file(&tz->device,
				   &tz->trip_temp_attrs[indx].attr);
		if (tz->ops->get_trip_hyst)
			device_remove_file(&tz->device,
				  &tz->trip_hyst_attrs[indx].attr);
	}
	kfree(tz->trip_type_attrs);
	kfree(tz->trip_temp_attrs);
	kfree(tz->trip_hyst_attrs);
}

#ifdef CONFIG_HISI_IPA_THERMAL
static inline void set_tz_type(struct thermal_zone_device *tz)
{
	if (!strncmp(tz->type, BOARD_THERMAL_NAME, sizeof(BOARD_THERMAL_NAME) - 1))
		tz->is_board_thermal = true;
	else if (!strncmp(tz->type, SOC_THERMAL_NAME, sizeof(SOC_THERMAL_NAME) - 1))
		tz->is_soc_thermal = true;
}
#endif

/**
 * thermal_zone_device_register() - register a new thermal zone device
 * @type:	the thermal zone device type
 * @trips:	the number of trip points the thermal zone support
 * @mask:	a bit string indicating the writeablility of trip points
 * @devdata:	private device data
 * @ops:	standard thermal zone device callbacks
 * @tzp:	thermal zone platform parameters
 * @passive_delay: number of milliseconds to wait between polls when
 *		   performing passive cooling
 * @polling_delay: number of milliseconds to wait between polls when checking
 *		   whether trip points have been crossed (0 for interrupt
 *		   driven systems)
 *
 * This interface function adds a new thermal zone device (sensor) to
 * /sys/class/thermal folder as thermal_zone[0-*]. It tries to bind all the
 * thermal cooling devices registered at the same time.
 * thermal_zone_device_unregister() must be called when the device is no
 * longer needed. The passive cooling depends on the .get_trend() return value.
 *
 * Return: a pointer to the created struct thermal_zone_device or an
 * in case of error, an ERR_PTR. Caller must check return value with
 * IS_ERR*() helpers.
 */
struct thermal_zone_device *thermal_zone_device_register(const char *type,
	int trips, int mask, void *devdata,
	struct thermal_zone_device_ops *ops,
	struct thermal_zone_params *tzp,
	int passive_delay, int polling_delay)
{/*lint !e18*/
	struct thermal_zone_device *tz;
	enum thermal_trip_type trip_type;
	int trip_temp;
	int result;
	int count;
	int passive = 0;
	struct thermal_governor *governor;

	if (type && strlen(type) >= THERMAL_NAME_LENGTH)
		return ERR_PTR(-EINVAL);

	if (trips > THERMAL_MAX_TRIPS || trips < 0 || mask >> trips)
		return ERR_PTR(-EINVAL);

	if (!ops)
		return ERR_PTR(-EINVAL);
#ifdef CONFIG_HISI_IPA_THERMAL
	if (trips > 0 && !ops->get_trip_type)
#else
	if (trips > 0 && (!ops->get_trip_type || !ops->get_trip_temp))
#endif
		return ERR_PTR(-EINVAL);

	tz = kzalloc(sizeof(struct thermal_zone_device), GFP_KERNEL);
	if (!tz)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&tz->thermal_instances);
	idr_init(&tz->idr);
	mutex_init(&tz->lock);
	result = get_idr(&thermal_tz_idr, &thermal_idr_lock, &tz->id);
	if (result) {
		kfree(tz);
		return ERR_PTR(result);
	}

	strlcpy(tz->type, type ? : "", sizeof(tz->type));
	tz->ops = ops;
	tz->tzp = tzp;
	tz->device.class = &thermal_class;
	tz->devdata = devdata;
	tz->trips = trips;
	tz->passive_delay = passive_delay;
	tz->polling_delay = polling_delay;
	/* A new thermal zone needs to be updated anyway. */
	atomic_set(&tz->need_update, 1);

#ifdef CONFIG_HISI_IPA_THERMAL
	set_tz_type(tz);
#endif

	dev_set_name(&tz->device, "thermal_zone%d", tz->id);
	result = device_register(&tz->device);
	if (result) {
		release_idr(&thermal_tz_idr, &thermal_idr_lock, tz->id);
		kfree(tz);
		return ERR_PTR(result);
	}

	/* sys I/F */
	if (type) {
		result = device_create_file(&tz->device, &dev_attr_type);
		if (result)
			goto unregister;
	}

	result = device_create_file(&tz->device, &dev_attr_temp);
	if (result)
		goto unregister;

	if (ops->get_mode) {
		result = device_create_file(&tz->device, &dev_attr_mode);
		if (result)
			goto unregister;
	}

	result = create_trip_attrs(tz, mask);
	if (result)
		goto unregister;

	for (count = 0; count < trips; count++) {
		if (tz->ops->get_trip_type(tz, count, &trip_type))
			set_bit(count, &tz->trips_disabled);
		if (trip_type == THERMAL_TRIP_PASSIVE)
			passive = 1;
#ifdef CONFIG_HISI_IPA_THERMAL
		if (tz->ops->get_trip_temp && tz->ops->get_trip_temp(tz, count, &trip_temp))
			set_bit(count, &tz->trips_disabled);
#else
		if (tz->ops->get_trip_temp(tz, count, &trip_temp))
			set_bit(count, &tz->trips_disabled);
#endif
		/* Check for bogus trip points */
		if (trip_temp == 0)/*lint !e644 */
			set_bit(count, &tz->trips_disabled);
	}

	if (!passive) {
		result = device_create_file(&tz->device, &dev_attr_passive);
		if (result)
			goto unregister;
	}

	if (IS_ENABLED(CONFIG_THERMAL_EMULATION)) {
		result = device_create_file(&tz->device, &dev_attr_emul_temp);
		if (result)
			goto unregister;
	}

	/* Create policy attribute */
	result = device_create_file(&tz->device, &dev_attr_policy);
	if (result)
		goto unregister;

	/* Add thermal zone params */
	result = create_tzp_attrs(&tz->device);
	if (result)
		goto unregister;

	/* Create available_policies attribute */
	result = device_create_file(&tz->device, &dev_attr_available_policies);
	if (result)
		goto unregister;

	/* Update 'this' zone's governor information */
	mutex_lock(&thermal_governor_lock);

	if (tz->tzp)
		governor = __find_governor(tz->tzp->governor_name);
	else
		governor = def_governor;

	result = thermal_set_governor(tz, governor);
	if (result) {
		mutex_unlock(&thermal_governor_lock);
		goto unregister;
	}

	mutex_unlock(&thermal_governor_lock);

	if (!tz->tzp || !tz->tzp->no_hwmon) {
		result = thermal_add_hwmon_sysfs(tz);
		if (result)
			goto unregister;
	}

	mutex_lock(&thermal_list_lock);
	list_add_tail(&tz->node, &thermal_tz_list);
	mutex_unlock(&thermal_list_lock);

	/* Bind cooling devices for this zone */
	bind_tz(tz);

	INIT_DELAYED_WORK(&(tz->poll_queue), thermal_zone_device_check);

	thermal_zone_device_reset(tz);
	/* Update the new thermal zone and mark it as already updated. */
	if (atomic_cmpxchg(&tz->need_update, 1, 0))
		thermal_zone_device_update(tz, THERMAL_EVENT_UNSPECIFIED);

	return tz;

unregister:
	release_idr(&thermal_tz_idr, &thermal_idr_lock, tz->id);
	device_unregister(&tz->device);
	kfree(tz);
	return ERR_PTR(result);
}
EXPORT_SYMBOL_GPL(thermal_zone_device_register);

/**
 * thermal_device_unregister - removes the registered thermal zone device
 * @tz: the thermal zone device to remove
 */
void thermal_zone_device_unregister(struct thermal_zone_device *tz)
{
	int i;
	const struct thermal_zone_params *tzp;
	struct thermal_cooling_device *cdev;
	struct thermal_zone_device *pos = NULL;

	if (!tz)
		return;

	tzp = tz->tzp;

	mutex_lock(&thermal_list_lock);
	list_for_each_entry(pos, &thermal_tz_list, node)
	    if (pos == tz)
		break;
	if (pos != tz) {
		/* thermal zone device not found */
		mutex_unlock(&thermal_list_lock);
		return;
	}
	list_del(&tz->node);

	/* Unbind all cdevs associated with 'this' thermal zone */
	list_for_each_entry(cdev, &thermal_cdev_list, node) {
		if (tz->ops->unbind) {
			tz->ops->unbind(tz, cdev);
			continue;
		}

		if (!tzp || !tzp->tbp)
			break;

		for (i = 0; i < tzp->num_tbps; i++) {
			if (tzp->tbp[i].cdev == cdev) {
				__unbind(tz, tzp->tbp[i].trip_mask, cdev);
				tzp->tbp[i].cdev = NULL;
			}
		}
	}

	mutex_unlock(&thermal_list_lock);

	thermal_zone_device_set_polling(tz, 0);

	if (tz->type[0])
		device_remove_file(&tz->device, &dev_attr_type);
	device_remove_file(&tz->device, &dev_attr_temp);
	if (tz->ops->get_mode)
		device_remove_file(&tz->device, &dev_attr_mode);
	device_remove_file(&tz->device, &dev_attr_policy);
	device_remove_file(&tz->device, &dev_attr_available_policies);
	remove_trip_attrs(tz);
	thermal_set_governor(tz, NULL);

	thermal_remove_hwmon_sysfs(tz);
	release_idr(&thermal_tz_idr, &thermal_idr_lock, tz->id);
	idr_destroy(&tz->idr);
	mutex_destroy(&tz->lock);
	device_unregister(&tz->device);
	return;
}
EXPORT_SYMBOL_GPL(thermal_zone_device_unregister);

/**
 * thermal_zone_get_zone_by_name() - search for a zone and returns its ref
 * @name: thermal zone name to fetch the temperature
 *
 * When only one zone is found with the passed name, returns a reference to it.
 *
 * Return: On success returns a reference to an unique thermal zone with
 * matching name equals to @name, an ERR_PTR otherwise (-EINVAL for invalid
 * paramenters, -ENODEV for not found and -EEXIST for multiple matches).
 */
struct thermal_zone_device *thermal_zone_get_zone_by_name(const char *name)
{
	struct thermal_zone_device *pos = NULL, *ref = ERR_PTR(-EINVAL);
	unsigned int found = 0;

	if (!name)
		goto exit;

	mutex_lock(&thermal_list_lock);
	list_for_each_entry(pos, &thermal_tz_list, node)
		if (!strncasecmp(name, pos->type, THERMAL_NAME_LENGTH)) {
			found++;
			ref = pos;
		}
	mutex_unlock(&thermal_list_lock);

	/* nothing has been found, thus an error code for it */
	if (found == 0)
		ref = ERR_PTR(-ENODEV);
	else if (found > 1)
	/* Success only when an unique zone is found */
		ref = ERR_PTR(-EEXIST);

exit:
	return ref;
}
EXPORT_SYMBOL_GPL(thermal_zone_get_zone_by_name);

/**
 * thermal_zone_get_slope - return the slope attribute of the thermal zone
 * @tz: thermal zone device with the slope attribute
 *
 * Return: If the thermal zone device has a slope attribute, return it, else
 * return 1.
 */
int thermal_zone_get_slope(struct thermal_zone_device *tz)
{
	if (tz && tz->tzp)
		return tz->tzp->slope;
	return 1;
}
EXPORT_SYMBOL_GPL(thermal_zone_get_slope);

/**
 * thermal_zone_get_offset - return the offset attribute of the thermal zone
 * @tz: thermal zone device with the offset attribute
 *
 * Return: If the thermal zone device has a offset attribute, return it, else
 * return 0.
 */
int thermal_zone_get_offset(struct thermal_zone_device *tz)
{
	if (tz && tz->tzp)
		return tz->tzp->offset;
	return 0;
}
EXPORT_SYMBOL_GPL(thermal_zone_get_offset);

#ifdef CONFIG_NET
static const struct genl_multicast_group thermal_event_mcgrps[] = {
	{ .name = THERMAL_GENL_MCAST_GROUP_NAME, },
};

static struct genl_family thermal_event_genl_family = {
	.id = GENL_ID_GENERATE,
	.name = THERMAL_GENL_FAMILY_NAME,
	.version = THERMAL_GENL_VERSION,
	.maxattr = THERMAL_GENL_ATTR_MAX,
	.mcgrps = thermal_event_mcgrps,
	.n_mcgrps = ARRAY_SIZE(thermal_event_mcgrps),
};

int thermal_generate_netlink_event(struct thermal_zone_device *tz,
					enum events event)
{
	struct sk_buff *skb;
	struct nlattr *attr;
	struct thermal_genl_event *thermal_event;
	void *msg_header;
	int size;
	int result;
	static unsigned int thermal_event_seqnum;

	if (!tz)
		return -EINVAL;

	/* allocate memory */
	size = nla_total_size(sizeof(struct thermal_genl_event)) +
	       nla_total_size(0);

	skb = genlmsg_new(size, GFP_ATOMIC);
	if (!skb)
		return -ENOMEM;

	/* add the genetlink message header */
	msg_header = genlmsg_put(skb, 0, thermal_event_seqnum++,
				 &thermal_event_genl_family, 0,
				 THERMAL_GENL_CMD_EVENT);
	if (!msg_header) {
		nlmsg_free(skb);
		return -ENOMEM;
	}

	/* fill the data */
	attr = nla_reserve(skb, THERMAL_GENL_ATTR_EVENT,
			   sizeof(struct thermal_genl_event));

	if (!attr) {
		nlmsg_free(skb);
		return -EINVAL;
	}

	thermal_event = nla_data(attr);
	if (!thermal_event) {
		nlmsg_free(skb);
		return -EINVAL;
	}

	memset(thermal_event, 0, sizeof(struct thermal_genl_event));

	thermal_event->orig = tz->id;
	thermal_event->event = event;

	/* send multicast genetlink message */
	genlmsg_end(skb, msg_header);

	result = genlmsg_multicast(&thermal_event_genl_family, skb, 0,
				   0, GFP_ATOMIC);
	if (result)
		dev_err(&tz->device, "Failed to send netlink event:%d", result);

	return result;
}
EXPORT_SYMBOL_GPL(thermal_generate_netlink_event);

static int genetlink_init(void)
{
	return genl_register_family(&thermal_event_genl_family);
}

static void genetlink_exit(void)
{
	genl_unregister_family(&thermal_event_genl_family);
}
#else /* !CONFIG_NET */
static inline int genetlink_init(void) { return 0; }
static inline void genetlink_exit(void) {}
#endif /* !CONFIG_NET */

static int __init thermal_register_governors(void)
{
	int result;

	result = thermal_gov_step_wise_register();
	if (result)
		return result;

	result = thermal_gov_fair_share_register();
	if (result)
		return result;

	result = thermal_gov_bang_bang_register();
	if (result)
		return result;

	result = thermal_gov_user_space_register();
	if (result)
		return result;

	return thermal_gov_power_allocator_register();
}

static void thermal_unregister_governors(void)
{
	thermal_gov_step_wise_unregister();
	thermal_gov_fair_share_unregister();
	thermal_gov_bang_bang_unregister();
	thermal_gov_user_space_unregister();
	thermal_gov_power_allocator_unregister();
}

static int thermal_pm_notify(struct notifier_block *nb,
				unsigned long mode, void *_unused)
{
	struct thermal_zone_device *tz;

	switch (mode) {
	case PM_HIBERNATION_PREPARE:
	case PM_RESTORE_PREPARE:
	case PM_SUSPEND_PREPARE:
		atomic_set(&in_suspend, 1);
		break;
	case PM_POST_HIBERNATION:
	case PM_POST_RESTORE:
	case PM_POST_SUSPEND:
		atomic_set(&in_suspend, 0);
		list_for_each_entry(tz, &thermal_tz_list, node) {
#ifdef CONFIG_HISI_IPA_THERMAL
			if(strncmp(tz->governor->name, USER_SPACE_GOVERNOR, THERMAL_NAME_LENGTH)){ /*[false alarm]*/
				thermal_zone_device_reset(tz);
				thermal_zone_device_update(tz, THERMAL_EVENT_UNSPECIFIED);
			}
#else
			thermal_zone_device_reset(tz);
			thermal_zone_device_update(tz,
						   THERMAL_EVENT_UNSPECIFIED);
#endif
		}
		break;
	default:
		break;
	}
	return 0;
}

static struct notifier_block thermal_pm_nb = {
	.notifier_call = thermal_pm_notify,
};

#ifdef CONFIG_HISI_THERMAL_SPM
static int init_profile_power(void)
{
	struct thermal_zone_device *pos;
	unsigned int sustainable_power;
	unsigned int total_weight = 0;
	unsigned int weight;
	int i, num_tbps = 0;
	enum ipa_actor actor;
	int ret = 0;

	mutex_lock(&thermal_list_lock);

	/*lint -e64 -e826 -esym(64,826,*)*/
	list_for_each_entry(pos, &thermal_tz_list, node) {
		if (!strncasecmp(pos->type, "soc_thermal", sizeof("soc_thermal") - 1)) {
			sustainable_power = pos->tzp->sustainable_power;
			num_tbps = of_thermal_get_num_tbps(pos);
			for (i = 0; i < num_tbps; i++) {
				ret = of_thermal_get_actor_weight(pos, i, &actor, &weight);
				if (ret)
					goto EXIT;

				total_weight += weight;
				profile_power[actor] = weight * sustainable_power;

			}
			break;
		}
	}
	/*lint -e64 -e826 +esym(64,826,*)*/

	if (total_weight) {
		for (i = 0; i < num_tbps; i++) {
			profile_power[i] /= total_weight;
			pr_err("IPA: actor %d, power: %d\n", i, profile_power[i]);
		}
	}
EXIT:
	mutex_unlock(&thermal_list_lock);

	return ret;
}

int get_profile_power(enum ipa_actor actor, unsigned int *power)
{
	if (actor >= IPA_ACTOR_MAX || actor < IPA_CLUSTER0)
		return -EDOM;

	*power = profile_power[actor];

	return 0;
}
EXPORT_SYMBOL(get_profile_power);

int get_soc_target_temp(struct thermal_cooling_device *cdev, int *target_temp)
{
	bool found = false;
	struct thermal_instance *instance;
	struct thermal_zone_device *thermal_zone;

	mutex_lock(&cdev->lock);
	/*lint -e64 -e826 -esym(64,826,*)*/
	list_for_each_entry(instance, &cdev->thermal_instances, cdev_node) {
		if (!strncasecmp(instance->tz->type, "soc_thermal", sizeof("soc_thermal") - 1)) {
			found = true;
			thermal_zone = instance->tz;
			thermal_zone->ops->get_trip_temp(thermal_zone, instance->trip, target_temp);
			break;
		}
	}
	/*lint -e64 -e826 +esym(64,826,*)*/
	mutex_unlock(&cdev->lock);

	if (!found)
		return -EINVAL;

	pr_err("IPA: soc target temp: %d", *target_temp);
	return 0;
}
EXPORT_SYMBOL(get_soc_target_temp);

#endif

static int __init thermal_init(void)
{
	int result;

	result = thermal_register_governors();
	if (result)
		goto error;

	result = class_register(&thermal_class);
	if (result)
		goto unregister_governors;

	result = genetlink_init();
	if (result)
		goto unregister_class;

	result = of_parse_thermal_zones();
	if (result)
		goto exit_netlink;

#ifdef CONFIG_HISI_THERMAL_SPM
	result = init_profile_power();
	if (result)
		goto exit_netlink;
#endif

	result = register_pm_notifier(&thermal_pm_nb);
	if (result)
		pr_warn("Thermal: Can not register suspend notifier, return %d\n",
			result);

	return 0;

exit_netlink:
	genetlink_exit();
unregister_class:
	class_unregister(&thermal_class);
unregister_governors:
	thermal_unregister_governors();
error:
	idr_destroy(&thermal_tz_idr);
	idr_destroy(&thermal_cdev_idr);
	mutex_destroy(&thermal_idr_lock);
	mutex_destroy(&thermal_list_lock);
	mutex_destroy(&thermal_governor_lock);
#ifdef CONFIG_HISI_IPA_THERMAL
	mutex_destroy(&thermal_boost_lock);
#endif
	return result;
}

static void __exit thermal_exit(void)
{
	unregister_pm_notifier(&thermal_pm_nb);
	of_thermal_destroy_zones();
	genetlink_exit();
	class_unregister(&thermal_class);
	thermal_unregister_governors();
	idr_destroy(&thermal_tz_idr);
	idr_destroy(&thermal_cdev_idr);
	mutex_destroy(&thermal_idr_lock);
	mutex_destroy(&thermal_list_lock);
	mutex_destroy(&thermal_governor_lock);
#ifdef CONFIG_HISI_IPA_THERMAL
	mutex_destroy(&thermal_boost_lock);
#endif
}

fs_initcall(thermal_init);
module_exit(thermal_exit);
