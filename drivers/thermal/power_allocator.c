/*
 * A power allocator to manage temperature
 *
 * Copyright (C) 2014 ARM Ltd.
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

#define pr_fmt(fmt) "Power allocator: " fmt

#include <linux/rculist.h>
#include <linux/slab.h>
#include <linux/thermal.h>

#define CREATE_TRACE_POINTS
#include <trace/events/thermal_power_allocator.h>

#include "thermal_core.h"

#define INVALID_TRIP -1

#define FRAC_BITS 10
#define int_to_frac(x) ((x) << FRAC_BITS)
#define frac_to_int(x) ((x) >> FRAC_BITS)

#ifdef CONFIG_HISI_IPA_THERMAL
#define MIN_POWER_DIFF 50
#define BOARDIPA_PID_RESET_TEMP 2000
extern unsigned int g_ipa_board_state[];
extern unsigned int g_ipa_soc_state[];
#endif

/**
 * mul_frac() - multiply two fixed-point numbers
 * @x:	first multiplicand
 * @y:	second multiplicand
 *
 * Return: the result of multiplying two fixed-point numbers.  The
 * result is also a fixed-point number.
 */
static inline s64 mul_frac(s64 x, s64 y)
{
	return (x * y) >> FRAC_BITS;
}

/**
 * div_frac() - divide two fixed-point numbers
 * @x:	the dividend
 * @y:	the divisor
 *
 * Return: the result of dividing two fixed-point numbers.  The
 * result is also a fixed-point number.
 */
static inline s64 div_frac(s64 x, s64 y)
{
	return div_s64(x << FRAC_BITS, y);
}

/**
 * struct power_allocator_params - parameters for the power allocator governor
 * @allocated_tzp:	whether we have allocated tzp for this thermal zone and
 *			it needs to be freed on unbind
 * @err_integral:	accumulated error in the PID controller.
 * @prev_err:	error in the previous iteration of the PID controller.
 *		Used to calculate the derivative term.
 * @trip_switch_on:	first passive trip point of the thermal zone.  The
 *			governor switches on when this trip point is crossed.
 *			If the thermal zone only has one passive trip point,
 *			@trip_switch_on should be INVALID_TRIP.
 * @trip_max_desired_temperature:	last passive trip point of the thermal
 *					zone.  The temperature we are
 *					controlling for.
 */
struct power_allocator_params {
	bool allocated_tzp;
	s64 err_integral;
	s32 prev_err;
	int trip_switch_on;
	int trip_max_desired_temperature;
};

/**
 * estimate_sustainable_power() - Estimate the sustainable power of a thermal zone
 * @tz: thermal zone we are operating in
 *
 * For thermal zones that don't provide a sustainable_power in their
 * thermal_zone_params, estimate one.  Calculate it using the minimum
 * power of all the cooling devices as that gives a valid value that
 * can give some degree of functionality.  For optimal performance of
 * this governor, provide a sustainable_power in the thermal zone's
 * thermal_zone_params.
 */
static u32 estimate_sustainable_power(struct thermal_zone_device *tz)
{
	u32 sustainable_power = 0;
	struct thermal_instance *instance;
	struct power_allocator_params *params = tz->governor_data;

	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		struct thermal_cooling_device *cdev = instance->cdev;
		u32 min_power;

		if (instance->trip != params->trip_max_desired_temperature)
			continue;

		if (power_actor_get_min_power(cdev, tz, &min_power))
			continue;

		sustainable_power += min_power;
	}

	return sustainable_power;
}

/**
 * estimate_pid_constants() - Estimate the constants for the PID controller
 * @tz:		thermal zone for which to estimate the constants
 * @sustainable_power:	sustainable power for the thermal zone
 * @trip_switch_on:	trip point number for the switch on temperature
 * @control_temp:	target temperature for the power allocator governor
 * @force:	whether to force the update of the constants
 *
 * This function is used to update the estimation of the PID
 * controller constants in struct thermal_zone_parameters.
 * Sustainable power is provided in case it was estimated.  The
 * estimated sustainable_power should not be stored in the
 * thermal_zone_parameters so it has to be passed explicitly to this
 * function.
 *
 * If @force is not set, the values in the thermal zone's parameters
 * are preserved if they are not zero.  If @force is set, the values
 * in thermal zone's parameters are overwritten.
 */
static void estimate_pid_constants(struct thermal_zone_device *tz,
				   u32 sustainable_power, int trip_switch_on,
				   int control_temp, bool force)
{
	int ret;
	int switch_on_temp;
	u32 temperature_threshold;

	ret = tz->ops->get_trip_temp(tz, trip_switch_on, &switch_on_temp);
	if (ret)
		switch_on_temp = 0;

	temperature_threshold = control_temp - switch_on_temp;
	/*
	 * estimate_pid_constants() tries to find appropriate default
	 * values for thermal zones that don't provide them. If a
	 * system integrator has configured a thermal zone with two
	 * passive trip points at the same temperature, that person
	 * hasn't put any effort to set up the thermal zone properly
	 * so just give up.
	 */
	if (!temperature_threshold)
		return;

	if (!tz->tzp->k_po || force)
		tz->tzp->k_po = int_to_frac(sustainable_power) /
			temperature_threshold;

	if (!tz->tzp->k_pu || force)
		tz->tzp->k_pu = int_to_frac(2 * sustainable_power) /
			temperature_threshold;

	if (!tz->tzp->k_i || force)
		tz->tzp->k_i = int_to_frac(10) / 1000;
	/*
	 * The default for k_d and integral_cutoff is 0, so we can
	 * leave them as they are.
	 */
}

/**
 * pid_controller() - PID controller
 * @tz:	thermal zone we are operating in
 * @control_temp:	the target temperature in millicelsius
 * @max_allocatable_power:	maximum allocatable power for this thermal zone
 *
 * This PID controller increases the available power budget so that the
 * temperature of the thermal zone gets as close as possible to
 * @control_temp and limits the power if it exceeds it.  k_po is the
 * proportional term when we are overshooting, k_pu is the
 * proportional term when we are undershooting.  integral_cutoff is a
 * threshold below which we stop accumulating the error.  The
 * accumulated error is only valid if the requested power will make
 * the system warmer.  If the system is mostly idle, there's no point
 * in accumulating positive error.
 *
 * Return: The power budget for the next period.
 */
static u32 pid_controller(struct thermal_zone_device *tz,
			  int control_temp,
			  u32 max_allocatable_power)
{
	s64 p, i, d, power_range;
	s32 err, max_power_frac;
	u32 sustainable_power;
	struct power_allocator_params *params = tz->governor_data;

	max_power_frac = int_to_frac(max_allocatable_power);

	if (tz->tzp->sustainable_power) {
		sustainable_power = tz->tzp->sustainable_power;
	} else {
		sustainable_power = estimate_sustainable_power(tz);
		estimate_pid_constants(tz, sustainable_power,
				       params->trip_switch_on, control_temp,
				       true);
	}

	err = control_temp - tz->temperature;
	err = int_to_frac(err);

	/* Calculate the proportional term */
	p = mul_frac(err < 0 ? tz->tzp->k_po : tz->tzp->k_pu, err);

	/*
	 * Calculate the integral term
	 *
	 * if the error is less than cut off allow integration (but
	 * the integral is limited to max power)
	 */
	i = mul_frac(tz->tzp->k_i, params->err_integral);

	if (err < int_to_frac(tz->tzp->integral_cutoff)) {
		s64 i_next = i + mul_frac(tz->tzp->k_i, err);

		if (abs(i_next) < max_power_frac) {
			i = i_next;
			params->err_integral += err;
		}
	}

	/*
	 * Calculate the derivative term
	 *
	 * We do err - prev_err, so with a positive k_d, a decreasing
	 * error (i.e. driving closer to the line) results in less
	 * power being applied, slowing down the controller)
	 */
#ifdef CONFIG_HISI_IPA_THERMAL
	if (tz->passive_delay != 0) {
		d = mul_frac(tz->tzp->k_d, err - params->prev_err);
		d = div_frac(d, tz->passive_delay);
		params->prev_err = err;
	} else {
		d = 0;
	}
#else
	d = mul_frac(tz->tzp->k_d, err - params->prev_err);
	d = div_frac(d, tz->passive_delay);
	params->prev_err = err;
#endif

	power_range = p + i + d;

	/* feed-forward the known sustainable dissipatable power */
	power_range = sustainable_power + frac_to_int(power_range);
	/*lint -e666*/
	power_range = clamp(power_range, (s64)0, (s64)max_allocatable_power);
	/*lint +e666*/
	trace_thermal_power_allocator_pid(tz, frac_to_int(err),
					  frac_to_int(params->err_integral),
					  frac_to_int(p), frac_to_int(i),
					  frac_to_int(d), power_range);

#ifdef CONFIG_HISI_IPA_THERMAL
	/*lint -e702 -e704 -e712 -e747 -esym(702,704,712,747,*)*/
	trace_IPA_allocator_pid(frac_to_int(err),
					  frac_to_int(params->err_integral),
					  frac_to_int(p), frac_to_int(i),
					  frac_to_int(d), power_range);
	/*lint -e702 -e704 -e712 -e747 +esym(702,704,712,747,*)*/
#endif

	return power_range;
}

/**
 * divvy_up_power() - divvy the allocated power between the actors
 * @req_power:	each actor's requested power
 * @max_power:	each actor's maximum available power
 * @num_actors:	size of the @req_power, @max_power and @granted_power's array
 * @total_req_power: sum of @req_power
 * @power_range:	total allocated power
 * @granted_power:	output array: each actor's granted power
 * @extra_actor_power:	an appropriately sized array to be used in the
 *			function as temporary storage of the extra power given
 *			to the actors
 *
 * This function divides the total allocated power (@power_range)
 * fairly between the actors.  It first tries to give each actor a
 * share of the @power_range according to how much power it requested
 * compared to the rest of the actors.  For example, if only one actor
 * requests power, then it receives all the @power_range.  If
 * three actors each requests 1mW, each receives a third of the
 * @power_range.
 *
 * If any actor received more than their maximum power, then that
 * surplus is re-divvied among the actors based on how far they are
 * from their respective maximums.
 *
 * Granted power for each actor is written to @granted_power, which
 * should've been allocated by the calling function.
 */
static void divvy_up_power(u32 *req_power, u32 *max_power, int num_actors,
			   u32 total_req_power, u32 power_range,
			   u32 *granted_power, u32 *extra_actor_power)
{
	u32 extra_power, capped_extra_power;
	int i;

	/*
	 * Prevent division by 0 if none of the actors request power.
	 */
	if (!total_req_power)
		total_req_power = 1;

	capped_extra_power = 0;
	extra_power = 0;
	for (i = 0; i < num_actors; i++) {
		u64 req_range = (u64)req_power[i] * (u64)power_range;

		granted_power[i] = DIV_ROUND_CLOSEST_ULL(req_range,
							 total_req_power);

		if (granted_power[i] > max_power[i]) {
			extra_power += granted_power[i] - max_power[i];
			granted_power[i] = max_power[i];
		}

		extra_actor_power[i] = max_power[i] - granted_power[i];
		capped_extra_power += extra_actor_power[i];
	}

	if (!extra_power)
		return;

	/*
	 * Re-divvy the reclaimed extra among actors based on
	 * how far they are from the max
	 */
	 /*lint -e666*/
	extra_power = min(extra_power, capped_extra_power);
	if (capped_extra_power > 0)
		for (i = 0; i < num_actors; i++)
			granted_power[i] += (extra_actor_power[i] *
					extra_power) / capped_extra_power;
	/*lint +e666*/
}

static void get_cur_power(struct thermal_zone_device *tz)
{
	struct thermal_instance *instance;
	struct power_allocator_params *params = tz->governor_data;
	u32 total_req_power;
	int trip_max_desired_temperature = params->trip_max_desired_temperature;

	if (0 == tz->tzp->cur_enable)
		return;

	mutex_lock(&tz->lock);

	total_req_power = 0;

	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		struct thermal_cooling_device *cdev = instance->cdev;
		u32 req_power;

		if (instance->trip != trip_max_desired_temperature)
			continue;

		if (!cdev_is_power_actor(cdev))
			continue;

		if (cdev->ops->get_requested_power(cdev, tz, &req_power))
			continue;

		total_req_power += req_power;

		cdev->cdev_cur_power += req_power;
	}

	tz->tzp->cur_ipa_total_power += total_req_power;
	tz->tzp->check_cnt ++;

	mutex_unlock(&tz->lock);
}

#ifdef CONFIG_HISI_IPA_THERMAL
static inline int power_actor_set_powers(struct thermal_zone_device *tz,
				struct thermal_instance *instance, u32 *soc_sustainable_power,
				u32 granted_power)
{
	unsigned long state = 0;
	int ret;
	int actor_id; // 0:Little, 1:Big, 2:GPU,

	if (!cdev_is_power_actor(instance->cdev))
		return -EINVAL;

	ret = instance->cdev->ops->power2state(instance->cdev, instance->tz, granted_power, &state);
	if (ret)
		return ret;

	if(!strncmp(instance->cdev->type, "thermal-devfreq-0", 17)){
		actor_id = IPA_GPU;
	} else if (!strncmp(instance->cdev->type, "thermal-cpufreq-0", 17)) {
		actor_id = IPA_CLUSTER0;
	} else if (!strncmp(instance->cdev->type, "thermal-cpufreq-1", 17)) {
		actor_id = IPA_CLUSTER1;
#ifdef CONFIG_HISI_THERMAL_TRIPPLE_CLUSTERS
	} else if (!strncmp(instance->cdev->type, "thermal-cpufreq-2", 17)) {
		actor_id = IPA_CLUSTER2;
#endif
	} else
		actor_id = -1;

	if (actor_id < 0){
		instance->target = state;
	} else if (tz->is_board_thermal) {
		g_ipa_board_state[actor_id] = state;
		instance->target = max(g_ipa_board_state[actor_id],g_ipa_soc_state[actor_id]);/*lint !e1058*/
	} else {
		g_ipa_soc_state[actor_id] = state;
		instance->target = max(g_ipa_board_state[actor_id],g_ipa_soc_state[actor_id]);/*lint !e1058*/
	}

	instance->cdev->updated = false;
	thermal_cdev_update(instance->cdev);

	return 0;
}

#ifdef CONFIG_HISI_IPA_THERMAL
int thermal_zone_cdev_get_power(const char *thermal_zone_name, const char *cdev_name, unsigned int *power)
{
	struct thermal_instance *instance;
	int ret = 0;
	struct thermal_zone_device *tz;
	int find = 0;

	tz = thermal_zone_get_zone_by_name(thermal_zone_name);
	if (IS_ERR(tz)) {
		return -ENODEV;
	}

	mutex_lock(&tz->lock);
	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		struct thermal_cooling_device *cdev = instance->cdev;

		if (!cdev_is_power_actor(cdev))
			continue;

		if (strncasecmp(instance->cdev->type, cdev_name, THERMAL_NAME_LENGTH))
			continue;

		if (!cdev->ops->get_requested_power(cdev, tz, power)) {
			find = 1;
			break;
		}
	}

	if(!find)
		ret = -ENODEV;

	mutex_unlock(&tz->lock);
	return ret;
}
EXPORT_SYMBOL(thermal_zone_cdev_get_power);

#endif

static int allocate_power(struct thermal_zone_device *tz,
			int control_temp,
			int switch_temp)
#else
static int allocate_power(struct thermal_zone_device *tz,
			int control_temp)
#endif
{
	struct thermal_instance *instance;
	struct power_allocator_params *params = tz->governor_data;
	u32 *req_power, *max_power, *granted_power, *extra_actor_power;
	u32 *weighted_req_power;
	u32 total_req_power, max_allocatable_power, total_weighted_req_power;
	u32 total_granted_power, power_range;
	int i, num_actors, total_weight, ret = 0;
	int trip_max_desired_temperature = params->trip_max_desired_temperature;
	u32 cur_enable;
#ifdef CONFIG_HISI_IPA_THERMAL
	u32 soc_sustainable_power = 0;
#endif

	mutex_lock(&tz->lock);

	num_actors = 0;
	total_weight = 0;
	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		if ((instance->trip == trip_max_desired_temperature) &&
		    cdev_is_power_actor(instance->cdev)) {
			num_actors++;
			total_weight += instance->weight;
			instance->cdev->ipa_enabled = true;
		}
	}

	if (!num_actors) {
		ret = -ENODEV;
		goto unlock;
	}

	/*
	 * We need to allocate five arrays of the same size:
	 * req_power, max_power, granted_power, extra_actor_power and
	 * weighted_req_power.  They are going to be needed until this
	 * function returns.  Allocate them all in one go to simplify
	 * the allocation and deallocation logic.
	 */
	BUILD_BUG_ON(sizeof(*req_power) != sizeof(*max_power));
	BUILD_BUG_ON(sizeof(*req_power) != sizeof(*granted_power));
	BUILD_BUG_ON(sizeof(*req_power) != sizeof(*extra_actor_power));
	BUILD_BUG_ON(sizeof(*req_power) != sizeof(*weighted_req_power));
	req_power = kcalloc(num_actors * 5, sizeof(*req_power), GFP_KERNEL);/*lint !e647*/
	if (!req_power) {
		ret = -ENOMEM;
		goto unlock;
	}

	max_power = &req_power[num_actors];
	granted_power = &req_power[2 * num_actors];/*lint !e662 !e679*/
	extra_actor_power = &req_power[3 * num_actors];/*lint !e662 !e679*/
	weighted_req_power = &req_power[4 * num_actors];/*lint !e662 !e679*/

	i = 0;
	total_weighted_req_power = 0;
	total_req_power = 0;
	max_allocatable_power = 0;
	cur_enable = tz->tzp->cur_enable;

	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		int weight;
		struct thermal_cooling_device *cdev = instance->cdev;
		instance->cdev->ipa_enabled = true;

		if (instance->trip != trip_max_desired_temperature)
			continue;

		if (!cdev_is_power_actor(cdev))
			continue;

		if (cdev->ops->get_requested_power(cdev, tz, &req_power[i]))
			continue;

		if (!total_weight)
			weight = 1 << FRAC_BITS;
		else
			weight = instance->weight;

		weighted_req_power[i] = frac_to_int(weight * req_power[i]);/*lint !e661 !e662*/

		if (power_actor_get_max_power(cdev, tz, &max_power[i]))
			continue;

		total_req_power += req_power[i];
		max_allocatable_power += max_power[i];/*lint !e661*/
		total_weighted_req_power += weighted_req_power[i];/*lint !e661 !e662*/
		if (cur_enable > 0)
			cdev->cdev_cur_power += req_power[i];

		i++;
	}
	if (cur_enable > 0)
	{
		tz->tzp->cur_ipa_total_power += total_req_power;
		tz->tzp->check_cnt ++;
	}

	power_range = pid_controller(tz, control_temp, max_allocatable_power);

	divvy_up_power(weighted_req_power, max_power, num_actors,
		       total_weighted_req_power, power_range, granted_power,
		       extra_actor_power);

	total_granted_power = 0;
	i = 0;
	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		if (instance->trip != trip_max_desired_temperature)
			continue;

		if (!cdev_is_power_actor(instance->cdev))
			continue;
#ifdef CONFIG_HISI_IPA_THERMAL
		power_actor_set_powers(tz, instance, &soc_sustainable_power, granted_power[i]);/*lint !e661 !e662*/
#else
		power_actor_set_power(instance->cdev, instance,
				      granted_power[i]);/*lint !e661 !e662*/
#endif
		total_granted_power += granted_power[i];/*lint !e661 !e662*/

		i++;
	}

	trace_thermal_power_allocator(tz, req_power, total_req_power,
				      granted_power, total_granted_power,
				      num_actors, power_range,
				      max_allocatable_power, tz->temperature,
				      control_temp - tz->temperature);

#ifdef CONFIG_HISI_IPA_THERMAL
	trace_IPA_allocator(tz->temperature, control_temp, switch_temp, control_temp - tz->temperature,
					num_actors, power_range, req_power, total_req_power,
					max_power, max_allocatable_power, granted_power, total_granted_power);
#endif
	kfree(req_power);
unlock:
	mutex_unlock(&tz->lock);

	return ret;
}

/**
 * get_governor_trips() - get the number of the two trip points that are key for this governor
 * @tz:	thermal zone to operate on
 * @params:	pointer to private data for this governor
 *
 * The power allocator governor works optimally with two trips points:
 * a "switch on" trip point and a "maximum desired temperature".  These
 * are defined as the first and last passive trip points.
 *
 * If there is only one trip point, then that's considered to be the
 * "maximum desired temperature" trip point and the governor is always
 * on.  If there are no passive or active trip points, then the
 * governor won't do anything.  In fact, its throttle function
 * won't be called at all.
 */
static void get_governor_trips(struct thermal_zone_device *tz,
			       struct power_allocator_params *params)
{
	int i, last_active, last_passive;
	bool found_first_passive;

	found_first_passive = false;
	last_active = INVALID_TRIP;
	last_passive = INVALID_TRIP;

	for (i = 0; i < tz->trips; i++) {
		enum thermal_trip_type type;
		int ret;

		ret = tz->ops->get_trip_type(tz, i, &type);
		if (ret) {
			dev_warn(&tz->device,
				 "Failed to get trip point %d type: %d\n", i,
				 ret);
			continue;
		}

		if (type == THERMAL_TRIP_PASSIVE) {
			if (!found_first_passive) {
				params->trip_switch_on = i;
				found_first_passive = true;
			} else  {
				last_passive = i;
			}
		} else if (type == THERMAL_TRIP_ACTIVE) {
			last_active = i;
		} else {
			break;
		}
	}

	if (last_passive != INVALID_TRIP) {
		params->trip_max_desired_temperature = last_passive;
	} else if (found_first_passive) {
		params->trip_max_desired_temperature = params->trip_switch_on;
		params->trip_switch_on = INVALID_TRIP;
	} else {
		params->trip_switch_on = INVALID_TRIP;
		params->trip_max_desired_temperature = last_active;
	}
}

static void reset_pid_controller(struct power_allocator_params *params)
{
	params->err_integral = 0;
	params->prev_err = 0;
}

static void allow_maximum_power(struct thermal_zone_device *tz)
{
	struct thermal_instance *instance;
	struct power_allocator_params *params = tz->governor_data;

	mutex_lock(&tz->lock);
	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		if ((instance->trip != params->trip_max_desired_temperature) ||
		    (!cdev_is_power_actor(instance->cdev)))
			continue;

		instance->target = 0;
		mutex_lock(&instance->cdev->lock);
		instance->cdev->updated = false;
		mutex_unlock(&instance->cdev->lock);
		thermal_cdev_update(instance->cdev);
	}
	mutex_unlock(&tz->lock);
}

/**
 * power_allocator_bind() - bind the power_allocator governor to a thermal zone
 * @tz:	thermal zone to bind it to
 *
 * Initialize the PID controller parameters and bind it to the thermal
 * zone.
 *
 * Return: 0 on success, or -ENOMEM if we ran out of memory.
 */
static int power_allocator_bind(struct thermal_zone_device *tz)
{
	int ret;
	struct power_allocator_params *params;
	int control_temp;

	params = kzalloc(sizeof(*params), GFP_KERNEL);
	if (!params)
		return -ENOMEM;

	if (!tz->tzp) {
		tz->tzp = kzalloc(sizeof(*tz->tzp), GFP_KERNEL);
		if (!tz->tzp) {
			ret = -ENOMEM;
			goto free_params;
		}

		params->allocated_tzp = true;
	}

	if (!tz->tzp->sustainable_power)
		dev_warn(&tz->device, "power_allocator: sustainable_power will be estimated\n");

	get_governor_trips(tz, params);

	if (tz->trips > 0) {
		ret = tz->ops->get_trip_temp(tz,
					params->trip_max_desired_temperature,
					&control_temp);
		if (!ret)
			estimate_pid_constants(tz, tz->tzp->sustainable_power,
					       params->trip_switch_on,
					       control_temp, false);
	}

	reset_pid_controller(params);

	tz->governor_data = params;

	return 0;

free_params:
	kfree(params);

	return ret;
}

static void power_allocator_unbind(struct thermal_zone_device *tz)
{
	struct power_allocator_params *params = tz->governor_data;

	dev_dbg(&tz->device, "Unbinding from thermal zone %d\n", tz->id);

	if (params->allocated_tzp) {
		kfree(tz->tzp);
		tz->tzp = NULL;
	}

	kfree(tz->governor_data);
	tz->governor_data = NULL;
}

static int power_allocator_throttle(struct thermal_zone_device *tz, int trip)
{
	int ret;
	int switch_on_temp, control_temp;
	struct power_allocator_params *params = tz->governor_data;
#ifdef CONFIG_HISI_IPA_THERMAL
	enum thermal_device_mode mode;

	ret = tz->ops->get_mode(tz, &mode);
	if (ret || THERMAL_DEVICE_DISABLED == mode) {
		return ret;
	}
#endif
	/*
	 * We get called for every trip point but we only need to do
	 * our calculations once
	 */
	if (trip != params->trip_max_desired_temperature)
		return 0;

	ret = tz->ops->get_trip_temp(tz, params->trip_switch_on,
				     &switch_on_temp);
	if (!ret && (tz->temperature < switch_on_temp)) {
		tz->passive = 0;
#ifdef CONFIG_HISI_IPA_THERMAL
		mutex_lock(&tz->lock);
		ipa_freq_limit_reset(tz);
		mutex_unlock(&tz->lock);
#endif
		reset_pid_controller(params);
		allow_maximum_power(tz);
		get_cur_power(tz);
		return 0;
	}

	tz->passive = 1;

	ret = tz->ops->get_trip_temp(tz, params->trip_max_desired_temperature,
				&control_temp);
	if (ret) {
		dev_warn(&tz->device,
			 "Failed to get the maximum desired temperature: %d\n",
			 ret);
		return ret;
	}

#ifdef CONFIG_HISI_IPA_THERMAL
	if (!ret && (tz->temperature <= (control_temp - BOARDIPA_PID_RESET_TEMP)) && tz->is_board_thermal) {
		reset_pid_controller(params);
	}

	return allocate_power(tz, control_temp, switch_on_temp);
#else
	return allocate_power(tz, control_temp);
#endif
}

#ifdef CONFIG_HISI_IPA_THERMAL
void update_pid_value(struct thermal_zone_device *tz)
{
	int ret;
	int control_temp;
	u32 sustainable_power = 0;
	struct power_allocator_params *params = tz->governor_data;

	mutex_lock(&tz->lock);
	if(!strncmp(tz->governor->name,"power_allocator",strlen("power_allocator"))) {
		ret = tz->ops->get_trip_temp(tz, params->trip_max_desired_temperature,
								&control_temp);
		if (ret) {	/*lint -e774 */
			dev_dbg(&tz->device,
			"Update PID failed to get control_temp: %d\n", ret);
		} else {
			if (tz->tzp->sustainable_power) {
				sustainable_power = tz->tzp->sustainable_power;
			} else {
				sustainable_power = estimate_sustainable_power(tz);
			}
			if (tz->is_soc_thermal)
				estimate_pid_constants(tz, sustainable_power,
					params->trip_switch_on,
					control_temp, (bool)true);
		}
	}
	mutex_unlock(&tz->lock);
}
#endif

static struct thermal_governor thermal_gov_power_allocator = {
	.name		= "power_allocator",
	.bind_to_tz	= power_allocator_bind,
	.unbind_from_tz	= power_allocator_unbind,
	.throttle	= power_allocator_throttle,
};

int thermal_gov_power_allocator_register(void)
{
	return thermal_register_governor(&thermal_gov_power_allocator);
}

void thermal_gov_power_allocator_unregister(void)
{
	thermal_unregister_governor(&thermal_gov_power_allocator);
}
