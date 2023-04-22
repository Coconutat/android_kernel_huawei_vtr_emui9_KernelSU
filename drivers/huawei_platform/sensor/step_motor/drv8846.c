/*
 * drv_8846.c
 *
 * step motor driver
 *
 * Copyright (c) 2018-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/timekeeping.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include "huawei_platform/log/hw_log.h"

#ifdef CONFIG_PWM
#include <linux/pwm.h>
#endif
#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG step_motor
HWLOG_REGIST();

#ifdef FAILURE
#undef FAILURE
#endif
#define FAILURE (-1)

#ifdef SUCCESS
#undef SUCCESS
#endif
#define SUCCESS 0

#ifdef INVALID_POS
#undef INVALID_POS
#endif
#define INVALID_POS (-1)

#ifdef GPIO_VAL_HIGH
#undef GPIO_VAL_HIGH
#endif
#define GPIO_VAL_HIGH 1

#ifdef GPIO_VAL_LOW
#undef GPIO_VAL_LOW
#endif
#define GPIO_VAL_LOW 0

#define MOTOR_A_CIRCLE_DEGREE              360
#define DIST_STD_MM_TENTH                  88
#define DIST_MAX_MM_TENTH                  160
#define DIST_MIN_MM_TENTH                  20
#define WALK_TIME_MIN_MS                   0
#define WALK_TIME_MAX_MS                   2000
#define MOTOR_STEP_ANGLE                   18
#define MOTOR_GEAR_RATIO                   36
#define MOTOR_WALK_DIST_PER_CYCLE_MM_TENTH 24
#define MOTOR_DRV_MICROSTEPS_MIN           1
#define MOTOR_DRV_MICROSTEPS_TWO           2
#define MOTOR_DRV_MICROSTEPS_FOUR          4
#define MOTOR_DRV_MICROSTEPS_EIGHT         8
#define MOTOR_DRV_MICROSTEPS_SIXTEEN       16
#define MOTOR_DRV_MICROSTEPS_MAX           32

#define ONE_SECOND_NANO                    1000000000
#define SEC_MS_RATIO                       1000
#define MS_NSEC_RATIO                      1000000

#define PARAM_NAME_LEN                     50
#define PARAM_VALUE_LEN                    50

#define POWER_ON_DELAY_MS                  1
#define SLEEP_WAKE_DELAY_MS                1
#define DIR_SET_DELAY_NS                   200

#define FAULT_IRQ_ENABLE_DELAY_MS          50

#define STARTING_SPEED_MIN                 400
#define STARTING_SPEED_MAX                 1600
#define STARTING_SPEED_STD                 800
#define STARTING_SPEED_STD_TIFULL          800
#define SPEED_STEP_MIN                     100
#define SPEED_STEP_MAX                     2000
#define STARTING_SPEED_STEP_STD            800
#define STARTING_SPEED_STEP_STD_TIFULL     800
#define DUR_PER_STEP_MIN                   10
#define DUR_PER_STEP_MAX                   200
#define START_DUR_PER_STEP_MS              100

#define TARGET_SPEED_MIN                   400
#define TARGET_SPEED_MAX                   2600
#define TARGET_SPEED_NORMAL_STD            2400
#define TARGET_SPEED_NORMAL_STD_TIFULL     2400
#define TARGET_SPEED_FAST_STD              2400

#define WORK_DURATION_MIN                  100
#define WORK_DURATION_MAX                  3000
#define WORK_DURATION_MS_NORMAL_STD        2000
#define WORK_DURATION_MS_FAST_STD          2000

#define HALF_DIVISOR                       2
#define DOUBLE_MULTIPLIER                  2
#define ONE_MULTIPLIER                     1

#define ST_VENDOR                          0
#define TI_VENDOR                          1

#define DEFAULT_CURVE_STYLE NO_START_NO_STOP
#define DEFAULT_DIST_CTRL_TYPE PULSE_COUNTER
#define DEFAULT_STEP_GEN_MANNER GPIO_STEP

enum direction {
	DIR_ROTATE_NO,
	DIR_ROTATE_IN,
	DIR_ROTATE_OUT
};

enum speed_mode {
	SPEED_NORMAL,
	SPEED_FAST
};

enum drv_state {
	STATE_STILL,
	STATE_START,
	STATE_SPEED_UP,
	STATE_SPEED_FULL,
	STATE_SPEED_DOWN,
	STATE_EMERGENCY_SPEED_DOWN,
	STATE_NORMAL_POWER_DOWN,
	STATE_OVERTIME_POWER_DOWN,
	STATE_FAULT_POWER_DOWN,
	STATE_EMERGENCY_POWER_DOWN
};

enum control_cmd {
	CMD_PUSH_OUT = 1001,
	CMD_PULL_BACK_NORMAL,
	CMD_PULL_BACK_FAST,
	CMD_STOP_PUSH_OUT,
	CMD_STOP_PULL_BACK
};

enum dist_control_type {
	PULSE_COUNTER = 100,
	PULSE_GEN_DUR_TIMER
};

enum drv_step_mode {
	STEP_MODE_GUARD_MIN,
	FULL_STEP_RISING_ONLY,
	TWO_STEP_RISING_ONLY,
	FOUR_STEP_RISING_ONLY,
	EIGHT_STEP_RISING_ONLY,
	EIGHT_STEP_RISING_FALLING,
	SIXTEEN_STEP_RISING_ONLY,
	SIXTEEN_STEP_RISING_FALLING,
	THIRTY_TWO_STEP_RISING_ONLY,
	THIRTY_TWO_STEP_RISING_FALLING,
	STEP_MODE_GUARD_MAX
};

/*
 * Slow start is for ensuring motor's strength.
 * So there is no NO_START_SLOW_STOP style.
 */
enum start_stop_curve_style {
	NO_START_NO_STOP,
	SLOW_START_NO_STOP,
	SLOW_START_SLOW_STOP
};

enum step_gen_manner {
	GPIO_STEP,
	PWM_STEP
};
/*
 * struct drv_data
 * @pdev: device struct pointer
 */
struct drv_data {
	struct device *pdev;
	struct work_struct fault_int_work;
	struct hrtimer step_timer;
	struct hrtimer overtime_timer;
	struct hrtimer irq_enable_timer;
	struct hrtimer speed_control;
	struct work_struct stop_work;
	struct mutex start_stop_mutex;
	struct mutex cmd_mutex;
	wait_queue_head_t wq;

	/* control param */
	int walk_distance_mm;
	int start_speed;
	int target_speed;
	int speed_step;
	int dur_per_step_ms;
	int speed_up_run_time_ms;
	int full_speed_dur_ms;
	int speed_down_run_time_ms;
	int target_speed_normal;
	int target_speed_fast;
	int work_dur_normal_ms;
	int work_dur_fast_ms;
	int cur_work_dur_ms;
	int cur_step_value;
	enum speed_mode speed_mode;
	enum direction cur_direction;
	int power_on_delay_ms;
	int sleep_wake_delay_ms;
	int dir_set_delay_ns;
	int irq_enable_delay_ms;
	spinlock_t state_change_lock;
	int cur_speed;
	enum drv_state cur_state;
	enum start_stop_curve_style curve_style;
	int motor_walk_distance_tenth;
	int motor_walk_time_ms;
	enum dist_control_type dist_control_type;
	int cur_pulse_edge_count;
	int total_pulse_edge_count;
	int start_pulse_edge_count;
	int microsteps;
	bool rising_edge_valid;
	bool falling_edge_valid;
	enum step_gen_manner step_gen_manner;
	struct pwm_device *step_device;

	/* pinctrl */
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinctrl_idle;
	struct pinctrl_state *pinctrl_default;
	/* gpio */
	int gpio_vm;
	int gpio_step;
	int gpio_dir;
	int gpio_decay;
	int gpio_enable;
	int gpio_sleep;
	int gpio_fault;
	int irq_fault;
	int gpio_mode1;
	int gpio_st_en;
	int ic_vendor;
	int direction;
};

static const struct of_device_id g_drv8846_match[] = {
	{.compatible = "huawei,step_motor"},
	{},
};
enum drv_step_mode g_step_work_mode;

/*
 * Print timestamp, used for debug.
 *
 * @param di the device info struct.
 * @param tag the prefix explanation string for timestamp.
 * @return void.
 */
static void print_ts(const char *tag)
{
	struct timeval tv;

	do_gettimeofday(&tv);
	if (tag)
		hwlog_info("%s:%lus, %luus\n", tag, tv.tv_sec, tv.tv_usec);
	else
		hwlog_info("%lus, %luus\n", tv.tv_sec, tv.tv_usec);
}

static void speed_start_control(struct drv_data *di, bool speed_control)
{
	s64 secs = 0;
	unsigned long nsecs = 0;
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	if (speed_control) {
		if (hrtimer_active(&di->speed_control))
			hrtimer_cancel(&di->speed_control);

		secs = di->dur_per_step_ms / SEC_MS_RATIO;
		nsecs = (di->dur_per_step_ms % SEC_MS_RATIO) * MS_NSEC_RATIO;
		hrtimer_start(&di->speed_control,
			ktime_set(secs, nsecs), HRTIMER_MODE_REL);
	} else {
		if (hrtimer_active(&di->speed_control))
			hrtimer_cancel(&di->speed_control);
	}
}

#ifdef CONFIG_PWM
static void pwm_step_control(struct drv_data *di, bool enable)
{
	struct pwm_state state;
	int ret = 0;
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	if (di->step_gen_manner != PWM_STEP)
		return;
	if ((di->step_device == NULL) || IS_ERR(di->step_device)) {
		hwlog_err("%s, pwm_device is invalid\n", __func__);
		return;
	}
	if (di->cur_speed == 0) {
		hwlog_err("start_control, cur_speed is 0\n");
		return;
	}
	ret = pinctrl_select_state(di->pinctrl, di->pinctrl_default);
	if (ret < 0) {
		hwlog_err("%s pinctrl_select_state idle error, error=%d\n",
			__func__, ret);
		return;
	}
	state.period = ONE_SECOND_NANO / di->cur_speed;
	state.duty_cycle = state.period / HALF_DIVISOR;
	state.polarity = PWM_POLARITY_NORMAL;
	state.enabled = enable;
	if (pwm_apply_state(di->step_device, &state) != 0)
		hwlog_err("start_control, pwm_apply_state fail\n");
}
#else
static void pwm_step_control(struct drv_data *di, bool enable)
{
	return;
}
#endif
/*
 * Translate the power down enum of drv_state to string format.
 *
 * @param pd_state the power down enum.
 * @return const char * the power down reason in string format.
 */
static const char *power_down_reason(enum drv_state pd_state)
{
	switch (pd_state) {
	case STATE_NORMAL_POWER_DOWN:
		return "normal";
	case STATE_OVERTIME_POWER_DOWN:
		return "overtime";
	case STATE_FAULT_POWER_DOWN:
		return "fault";
	case STATE_EMERGENCY_POWER_DOWN:
		return "emergency";
	default:
		return "unknown";
	}
}

/*
 * Common function for request gpio resource.
 *
 * @param di the device info struct.
 * @param name the gpio name in device tree.
 * @param gpio_out the output gpio num.
 * @return int the operation result SUCCESS or FAILURE.
 */
static int drv_request_gpio(struct drv_data *di, const char *name,
	int *gpio_out)
{
	struct device_node *np = NULL;

	if (!di || !name) {
		hwlog_err("%s di or name is null\n", __func__);
		return FAILURE;
	}
	np = di->pdev->of_node;

	*gpio_out = of_get_named_gpio(np, name, 0);

	if (!gpio_is_valid(*gpio_out)) {
		hwlog_err("%s is invalid\n", name);
		return FAILURE;
	}

	if (devm_gpio_request(di->pdev, *gpio_out, name)) {
		hwlog_err("request %s fail\n", name);
		return FAILURE;
	}

	return SUCCESS;
}

/*
 * Enable drv8846's power.
 *
 * @param di the device info struct.
 * @return void.
 */
static void drv8846_power(int gpio_vm)
{
	if (gpio_direction_output(gpio_vm, GPIO_VAL_HIGH))
		hwlog_err("turn on power failed\n");
}

/*
 * Schedule one stop work for stopping drv8846, used in irq context.
 *
 * @param di the device info struct.
 * @return void.
 */
static void drv8846_stop(struct drv_data *di)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	schedule_work(&di->stop_work);
}

/*
 * Calculate the duration of running in full speed state.
 *
 * @param di the device info struct.
 * @param pulse_count_per_cycle the pulse count needed for rotating one cycle.
 * @return void.
 */
static void drv8846_calc_full_speed_run_time(struct drv_data *di,
	int pulse_count_per_cycle)
{
	int start_stop_multiplier = ONE_MULTIPLIER;
	int total_run_time;
	int time_diff;

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	if (di->curve_style == SLOW_START_SLOW_STOP)
		start_stop_multiplier = DOUBLE_MULTIPLIER;

	// walk distance is too short
	if (di->total_pulse_edge_count < pulse_count_per_cycle) {
		di->full_speed_dur_ms = 0;
	} else {
		if (di->target_speed == 0) {
			hwlog_err("%s di->target_speed is 0\n", __func__);
			return;
		}
		di->full_speed_dur_ms = (di->total_pulse_edge_count -
			start_stop_multiplier * di->start_pulse_edge_count) *
			SEC_MS_RATIO / di->target_speed;
	}
	if (di->rising_edge_valid && di->falling_edge_valid)
		di->full_speed_dur_ms /= HALF_DIVISOR;

	if (di->motor_walk_time_ms != 0) {
		total_run_time = di->speed_up_run_time_ms +
			di->full_speed_dur_ms + di->speed_down_run_time_ms;
		if (di->motor_walk_time_ms < total_run_time) {
			time_diff = total_run_time - di->motor_walk_time_ms;
			if (di->full_speed_dur_ms > time_diff)
				di->full_speed_dur_ms -= time_diff;

		} else {
			time_diff = di->motor_walk_time_ms - total_run_time;
			di->full_speed_dur_ms += time_diff;
		}
	}

	// minus the first speed up time
	if (di->curve_style == NO_START_NO_STOP) {
		if (di->full_speed_dur_ms > di->dur_per_step_ms)
			di->full_speed_dur_ms -= di->dur_per_step_ms;
	}
}

/*
 * Calculate the running params.
 *
 * @param di the device info struct.
 * @return int SUCCESS or FAILURE.
 */
static int drv8846_param_init_calc(struct drv_data *di)
{
	int start_average_speed;
	int start_steps_count;
	int pulse_count_per_cycle;

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return FAILURE;
	}
	pulse_count_per_cycle = MOTOR_A_CIRCLE_DEGREE *
		di->microsteps / MOTOR_STEP_ANGLE;
	di->total_pulse_edge_count = di->motor_walk_distance_tenth *
		pulse_count_per_cycle * MOTOR_GEAR_RATIO /
			MOTOR_WALK_DIST_PER_CYCLE_MM_TENTH;

	di->cur_state = STATE_SPEED_UP;
	if (di->curve_style != NO_START_NO_STOP) {
		di->cur_speed = di->start_speed;

		if (di->speed_step <= 0 || di->target_speed <= 0 ||
			di->start_speed <= 0 ||
			di->target_speed < di->start_speed ||
			di->target_speed < di->speed_step)
			return FAILURE;

		start_average_speed = (di->target_speed - di->speed_step +
			di->start_speed) / HALF_DIVISOR;
		start_steps_count = (di->target_speed - di->start_speed) /
			di->speed_step;
		di->start_pulse_edge_count = start_average_speed *
			start_steps_count * di->dur_per_step_ms / SEC_MS_RATIO;
		if (di->rising_edge_valid && di->falling_edge_valid)
			di->start_pulse_edge_count *= DOUBLE_MULTIPLIER;
		// this is strict, maybe only one slow
		if (di->total_pulse_edge_count <
			DOUBLE_MULTIPLIER * di->start_pulse_edge_count)
			return FAILURE;

	} else {
		di->cur_speed = di->target_speed;
	}
	switch (di->curve_style) {
	case NO_START_NO_STOP:
		di->speed_up_run_time_ms = 0;
		di->speed_down_run_time_ms = 0;
		break;
	case SLOW_START_NO_STOP:
		di->speed_up_run_time_ms = start_steps_count *
			di->dur_per_step_ms;
		di->speed_down_run_time_ms = 0;
		break;
	case SLOW_START_SLOW_STOP:
		di->speed_up_run_time_ms = start_steps_count *
			di->dur_per_step_ms;
		di->speed_down_run_time_ms = di->speed_up_run_time_ms;
		break;
	default:
		hwlog_err("curve_style is invalid\n");
		break;
	}

	drv8846_calc_full_speed_run_time(di, pulse_count_per_cycle);
	return SUCCESS;
}

/*
 * The entrance for calculating running params.
 *
 * @param di the device info struct.
 * @return int SUCCESS or FAILURE.
 */
static int drv8846_param_init(struct drv_data *di)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return FAILURE;
	}
	spin_lock(&di->state_change_lock);

	switch (di->speed_mode) {
	case SPEED_NORMAL:
		di->target_speed = di->target_speed_normal;
		di->cur_work_dur_ms = di->work_dur_normal_ms;
		break;

	case SPEED_FAST:
		di->target_speed = di->target_speed_fast;
		di->cur_work_dur_ms = di->work_dur_fast_ms;
		break;

	default:
		goto error_end;
	}
	if (drv8846_param_init_calc(di) == FAILURE)
		goto error_end;

	spin_unlock(&di->state_change_lock);
	hwlog_info("total_pulse_edge_count:%d\n", di->total_pulse_edge_count);
	hwlog_info("start_pulse_edge_count:%d\n", di->start_pulse_edge_count);
	hwlog_info("target_speed:%d\n", di->target_speed);
	hwlog_info("full_speed_dur_ms:%d\n", di->full_speed_dur_ms);
	return SUCCESS;

error_end:
	di->cur_state = STATE_STILL;
	spin_unlock(&di->state_change_lock);
	hwlog_err("speed_step:%d, target_speed:%d, start_speed:%d\n",
		di->speed_step, di->target_speed, di->start_speed);
	return FAILURE;
}

/*
 * The handler for speed control timer in speed up state.
 *
 * @param di the device info struct.
 * @return void.
 */
static void drv8846_speed_control_handle_speed_up(struct drv_data *di)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	di->cur_speed += di->speed_step;
	if (di->cur_speed > di->target_speed)
		di->cur_speed = di->target_speed;

	if (di->cur_speed < di->target_speed) {
		hrtimer_forward_now(&di->speed_control,
			ktime_set(di->dur_per_step_ms / SEC_MS_RATIO,
				(di->dur_per_step_ms % SEC_MS_RATIO) *
					MS_NSEC_RATIO));

	} else {
		if (di->full_speed_dur_ms == 0) { // no target stage
			di->cur_state = STATE_SPEED_DOWN;
			di->cur_speed -= di->speed_step;

			if (di->cur_speed < di->start_speed)
				di->cur_speed = di->start_speed;
			hrtimer_forward_now(&di->speed_control,
				ktime_set(di->dur_per_step_ms / SEC_MS_RATIO,
					(di->dur_per_step_ms % SEC_MS_RATIO) *
						MS_NSEC_RATIO));

		} else {
			di->cur_state = STATE_SPEED_FULL;
			hrtimer_forward_now(&di->speed_control,
				ktime_set(di->full_speed_dur_ms / SEC_MS_RATIO,
					(di->full_speed_dur_ms % SEC_MS_RATIO) *
						MS_NSEC_RATIO));
		}
	}
}

/*
 * The handler for speed control timer in speed down state.
 *
 * @param di the device info struct.
 * @return hrtimer_restart whether the timer still needs restart.
 */
static enum hrtimer_restart drv_speed_ctrl_handle_down(struct drv_data *di)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return HRTIMER_NORESTART;
	}
	if (di->cur_speed > di->start_speed) {
		di->cur_speed -= di->speed_step;
		if (di->cur_speed < di->start_speed)
			di->cur_speed = di->start_speed;
		hrtimer_forward_now(&di->speed_control,
			ktime_set(di->dur_per_step_ms / SEC_MS_RATIO,
				(di->dur_per_step_ms % SEC_MS_RATIO) *
					MS_NSEC_RATIO));
		return HRTIMER_RESTART;

	} else {
		if ((di->dist_control_type == PULSE_COUNTER) &&
			(di->cur_state != STATE_EMERGENCY_SPEED_DOWN))
			return HRTIMER_NORESTART;

		if (di->cur_state == STATE_EMERGENCY_SPEED_DOWN)
			di->cur_state = STATE_EMERGENCY_POWER_DOWN;
		else
			di->cur_state = STATE_NORMAL_POWER_DOWN;
		drv8846_stop(di);
		return HRTIMER_NORESTART;
	}
}

/*
 * The common handler for timer of speed control.
 *
 * @param timer the hrtimer struct.
 * @return hrtimer_restart whether the timer still needs restart.
 */
static enum hrtimer_restart drv_speed_ctrl_timer_func(struct hrtimer *timer)
{
	int old_speed;
	struct drv_data *di = container_of(timer,
		struct drv_data, speed_control);

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return HRTIMER_NORESTART;
	}

	old_speed = di->cur_speed;
	spin_lock(&di->state_change_lock);

	if (di->cur_state == STATE_SPEED_UP) {
		drv8846_speed_control_handle_speed_up(di);
		goto speed_timer_restart;

	} else if (di->cur_state == STATE_SPEED_FULL) {
		if (di->curve_style == NO_START_NO_STOP ||
			di->curve_style == SLOW_START_NO_STOP) {
			if (di->dist_control_type == PULSE_COUNTER)
				goto speed_timer_norestart;
			di->cur_state = STATE_NORMAL_POWER_DOWN;
			drv8846_stop(di);
			goto speed_timer_norestart;
		}

		di->cur_state = STATE_SPEED_DOWN;
		di->cur_speed -= di->speed_step;
		if (di->cur_speed < di->start_speed)
			di->cur_speed = di->start_speed;
		hrtimer_forward_now(&di->speed_control,
			ktime_set(di->dur_per_step_ms / SEC_MS_RATIO,
				(di->dur_per_step_ms % SEC_MS_RATIO) *
					MS_NSEC_RATIO));
		goto speed_timer_restart;

	} else if ((di->cur_state == STATE_SPEED_DOWN) ||
		(di->cur_state == STATE_EMERGENCY_SPEED_DOWN)) {
		if (drv_speed_ctrl_handle_down(di) == HRTIMER_RESTART)
			goto speed_timer_restart;
		else
			goto speed_timer_norestart;
	}

speed_timer_norestart:
	if ((di->cur_speed != old_speed) &&
		(di->cur_state < STATE_NORMAL_POWER_DOWN))
		pwm_step_control(di, true);
	spin_unlock(&di->state_change_lock);
	return HRTIMER_NORESTART;

speed_timer_restart:
	if (di->cur_speed != old_speed)
		pwm_step_control(di, true);
	spin_unlock(&di->state_change_lock);
	return HRTIMER_RESTART;
}

/*
 * The handler for timer of step pulse generator.
 *
 * @param timer the hrtimer struct.
 * @return hrtimer_restart whether the timer still needs restart.
 */
static enum hrtimer_restart drv8846_step_timer_func(struct hrtimer *timer)
{
	int current_speed;
	struct drv_data *di = NULL;

	di = container_of(timer, struct drv_data, step_timer);
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return HRTIMER_NORESTART;
	}
	di->cur_step_value = di->cur_step_value == GPIO_VAL_LOW ?
		GPIO_VAL_HIGH : GPIO_VAL_LOW;
	gpio_set_value(di->gpio_step, di->cur_step_value);

	spin_lock(&di->state_change_lock);
	if (di->cur_state >= STATE_NORMAL_POWER_DOWN) {
		spin_unlock(&di->state_change_lock);
		return HRTIMER_NORESTART;
	}

	if (di->dist_control_type == PULSE_COUNTER) {
		if (di->cur_pulse_edge_count >= di->total_pulse_edge_count) {
			di->cur_state = STATE_NORMAL_POWER_DOWN;
			drv8846_stop(di);
			spin_unlock(&di->state_change_lock);
			return HRTIMER_NORESTART;
		}
		if (di->cur_step_value == GPIO_VAL_HIGH) {
			if (di->rising_edge_valid)
				di->cur_pulse_edge_count++;
		} else {
			if (di->falling_edge_valid)
				di->cur_pulse_edge_count++;
		}
	}
	current_speed = di->cur_speed;
	spin_unlock(&di->state_change_lock);
	if (current_speed == 0) {
		hwlog_err("%s current_speed is 0\n", __func__);
		return HRTIMER_NORESTART;
	}
	hrtimer_forward_now(&di->step_timer, ktime_set(0,
		(ONE_SECOND_NANO / current_speed) / HALF_DIVISOR));
	return HRTIMER_RESTART;
}

/*
 * Start the step pulse generator timer.
 *
 * @param di the device info struct.
 * @return void.
 */
static void drv8846_step_start(struct drv_data *di)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	if (di->step_gen_manner == PWM_STEP) {
		pwm_step_control(di, true);
		speed_start_control(di, true);
		return;
	}
	if (hrtimer_active(&di->step_timer))
		hrtimer_cancel(&di->step_timer);
	if (hrtimer_active(&di->speed_control))
		hrtimer_cancel(&di->speed_control);

	di->cur_step_value = GPIO_VAL_LOW; // init low level
	if (gpio_direction_output(di->gpio_step, di->cur_step_value))
		hwlog_err("step init set gpio step failed\n");
	di->cur_pulse_edge_count = 0;
	if (di->cur_speed == 0) {
		hwlog_err("%s di->cur_speed is 0\n", __func__);
		return;
	}
	hrtimer_start(&di->step_timer, ktime_set(0,
		(ONE_SECOND_NANO / di->cur_speed) / HALF_DIVISOR),
			HRTIMER_MODE_REL);
	if (di->dist_control_type == PULSE_COUNTER &&
		di->curve_style == NO_START_NO_STOP)
		return;
	hrtimer_start(&di->speed_control,
		ktime_set(di->dur_per_step_ms / SEC_MS_RATIO,
			(di->dur_per_step_ms % SEC_MS_RATIO) * MS_NSEC_RATIO),
		HRTIMER_MODE_REL);
}

/*
 * Stop the step pulse generator timer.
 *
 * @param di the device info struct.
 * @return void.
 */
static void drv8846_step_stop(const struct drv_data *di)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	if (di->step_gen_manner == PWM_STEP) {
		pwm_step_control(di, false);
		speed_start_control(di, false);
		return;
	}
	if (hrtimer_active(&di->step_timer))
		hrtimer_cancel(&di->step_timer);
	if (hrtimer_active(&di->speed_control))
		hrtimer_cancel(&di->speed_control);
}

/*
 * Set the rotation direction of the motor drived by drv8846.
 *
 * @param di the device info struct.
 * @param dir the needed direction.
 * @return void.
 */
static void drv8846_direction(int gpio_dir, int dir)
{
	if (gpio_direction_output(gpio_dir, dir))
		hwlog_err("set rotate direction:%d fail\n", dir);
}

/*
 * Wrapper for setting rotation direction to be back in.
 *
 * @param di the device info struct.
 * @param dir the needed direction.
 * @return void.
 */
static void drv8846_set_dir_in(int gpio_dir, int dir)
{
	if (dir == 0)
		drv8846_direction(gpio_dir, GPIO_VAL_LOW);
	else
		drv8846_direction(gpio_dir, GPIO_VAL_HIGH);
}

/*
 * Wrapper for setting rotation direction to be out.
 *
 * @param di the device info struct.
 * @param dir the needed direction.
 * @return void.
 */
static void drv8846_set_dir_out(int gpio_dir, int dir)
{
	if (dir == 0)
		drv8846_direction(gpio_dir, GPIO_VAL_HIGH);
	else
		drv8846_direction(gpio_dir, GPIO_VAL_LOW);
}

/*
 * Set the auto decay pin state of drv8846.
 *
 * @param di the device info struct.
 * @param val HIGH or LOW.
 * @return void.
 */
static void drv8846_adec(int gpio_decay, int val)
{
	if (gpio_direction_output(gpio_decay, val))
		hwlog_err("set drv8846_decay:%d fail\n", val);
}

/*
 * Set the sleep pin state of drv8846.
 *
 * @param di the device info struct.
 * @param enable true to enter sleep state, false to get out of sleep state.
 * @return void.
 */
static void drv8846_sleep(int gpio_sleep, bool enable)
{
	if (enable) {
		if (gpio_direction_output(gpio_sleep, GPIO_VAL_LOW))
			hwlog_err("enable sleep failed\n");
	} else {
		if (gpio_direction_output(gpio_sleep, GPIO_VAL_HIGH))
			hwlog_err("disable sleep failed\n");
	}
}

/*
 * Set the enable pin state of drv8846.
 *
 * @param di the device info struct.
 * @param enable true to enable false to disable.
 * @return void.
 */
static void ti_gpio_enable(int gpio_enable, bool enable)
{
	if (enable) {
		if (gpio_direction_output(gpio_enable, GPIO_VAL_LOW))
			hwlog_err("enable failed\n");
	} else {
		if (gpio_direction_output(gpio_enable, GPIO_VAL_HIGH))
			hwlog_err("disable failed\n");
	}
}

static void st_gpio_enable(int gpio_st_en, bool enable)
{
	if (enable) {
		if (gpio_direction_output(gpio_st_en, GPIO_VAL_HIGH))
			hwlog_err("enable failed\n");
	} else {
		if (gpio_direction_output(gpio_st_en, GPIO_VAL_LOW))
			hwlog_err("disable failed\n");
	}
}

static void drv8846_enable(const struct drv_data *di, bool enable)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	if (di->ic_vendor == ST_VENDOR)
		st_gpio_enable(di->gpio_st_en, enable);
	else if (di->ic_vendor == TI_VENDOR)
		ti_gpio_enable(di->gpio_enable, enable);
}

/*
 * Enable the fault irq of drv8846.
 *
 * @param di the device info struct.
 * @return void.
 */
static void drv8846_enable_fault_irq(int irq_fault)
{
	enable_irq(irq_fault);
}

/*
 * Disable the fault irq of drv8846.
 *
 * @param di the device info struct.
 * @return void.
 */
static void drv8846_disable_fault_irq(int irq_fault)
{
	disable_irq_nosync(irq_fault);
}

/*
 * Set the pins of drv8846 to idle state in case waste of power.
 *
 * @param di the device info struct.
 * @return void.
 */
static void drv8846_reset_pins_state(struct drv_data *di)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	if (IS_ERR(di->pinctrl) || IS_ERR(di->pinctrl_idle))
		return;

	if (pinctrl_select_state(di->pinctrl, di->pinctrl_idle) < 0)
		hwlog_err("could not set pins to default state\n");

	gpio_direction_output(di->gpio_step, GPIO_VAL_LOW);
	hwlog_info("make step to low\n");
}

/*
 * Stop the drv8846, used in process context.
 *
 * @param di the device info struct.
 * @return void.
 */
static void drv8846_stop_immediately(struct drv_data *di)
{
	enum drv_state cur_state;

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	mutex_lock(&di->start_stop_mutex);

	spin_lock(&di->state_change_lock);
	cur_state = di->cur_state;
	spin_unlock(&di->state_change_lock);
	if (cur_state != STATE_NORMAL_POWER_DOWN &&
		cur_state != STATE_OVERTIME_POWER_DOWN &&
		cur_state != STATE_FAULT_POWER_DOWN &&
		cur_state != STATE_EMERGENCY_POWER_DOWN)
		goto func_end;

	if (hrtimer_active(&di->overtime_timer))
		hrtimer_cancel(&di->overtime_timer);
	if (hrtimer_active(&di->irq_enable_timer))
		hrtimer_cancel(&di->irq_enable_timer);
	drv8846_disable_fault_irq(di->irq_fault);

	drv8846_step_stop(di);
	drv8846_enable(di, false);
	drv8846_sleep(di->gpio_sleep, true);
	drv8846_power(di->gpio_vm);

	drv8846_reset_pins_state(di);

	hwlog_info("power down state:%s, cur_pulse_edge_count:%d\n",
	power_down_reason(di->cur_state), di->cur_pulse_edge_count);

	spin_lock(&di->state_change_lock);
	di->cur_state = STATE_STILL;
	di->cur_pulse_edge_count = 0;
	spin_unlock(&di->state_change_lock);

	wake_up_interruptible(&di->wq);
	print_ts("drv8846_stop");

func_end:
	mutex_unlock(&di->start_stop_mutex);
}

/*
 * The work function for stopping drv8846.
 *
 * @param work the work_struct for stopping drv8846.
 * @return void.
 */
static void drv8846_stop_work_func(struct work_struct *work)
{
	struct drv_data *di = container_of(work, struct drv_data, stop_work);

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	drv8846_stop_immediately(di);
}

static enum hrtimer_restart drv8846_protect_timer_func(struct hrtimer *timer)
{
	struct drv_data *di = container_of(timer, struct drv_data,
		overtime_timer);

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return HRTIMER_NORESTART;
	}
	spin_lock(&di->state_change_lock);
	di->cur_state = STATE_OVERTIME_POWER_DOWN;
	spin_unlock(&di->state_change_lock);

	drv8846_stop(di);

	return HRTIMER_NORESTART;
}

/*
 * Handler for drv8846 enable irq timer.
 *
 * @param timer the hrtimer struct of enable irq timer.
 * @return hrtimer_restart HRTIMER_NORESTART this is oneshot timer,
 *  not restart it.
 */
static enum hrtimer_restart drv_irq_enable_timer_func(struct hrtimer *timer)
{
	struct drv_data *di = container_of(timer, struct drv_data,
		irq_enable_timer);

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return HRTIMER_NORESTART;
	}
	drv8846_enable_fault_irq(di->irq_fault);
	return HRTIMER_NORESTART;
}

/*
 * Start the drv8846, used in process context.
 *
 * @param di the device info struct.
 * @return int SUCCESS or FAILURE.
 */
static void st_set_mode(struct drv_data *di, enum drv_step_mode step_mode)
{
	int ret = 0;
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	ret = pinctrl_select_state(di->pinctrl, di->pinctrl_idle);
	if (ret < 0) {
		hwlog_err("%s pinctrl_select_state idle error, error=%d\n",
			__func__, ret);
		return;
	}
	switch (step_mode) {
	case FULL_STEP_RISING_ONLY:
		gpio_direction_output(di->gpio_mode1, GPIO_VAL_LOW);
		gpio_direction_output(di->gpio_decay, GPIO_VAL_LOW);
		gpio_direction_output(di->gpio_step, GPIO_VAL_LOW);
		gpio_direction_output(di->gpio_dir, GPIO_VAL_LOW);
		break;
	case THIRTY_TWO_STEP_RISING_ONLY:
		gpio_direction_output(di->gpio_mode1, GPIO_VAL_LOW);
		gpio_direction_output(di->gpio_decay, GPIO_VAL_HIGH);
		gpio_direction_output(di->gpio_step, GPIO_VAL_LOW);
		gpio_direction_output(di->gpio_dir, GPIO_VAL_LOW);
		break;
	default:
		hwlog_err("st_set_mode unknown step_mode:%d\n", step_mode);
		break;
	}
}

static void ti_set_mode(struct drv_data *di, enum drv_step_mode step_mode)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	switch (step_mode) {
	case FULL_STEP_RISING_ONLY:
		gpio_direction_output(di->gpio_mode1, GPIO_VAL_LOW);
		gpio_direction_output(di->gpio_st_en, GPIO_VAL_LOW);
		break;
	case THIRTY_TWO_STEP_RISING_ONLY:
		gpio_direction_input(di->gpio_mode1);
		gpio_direction_output(di->gpio_st_en, GPIO_VAL_HIGH);
		break;
	default:
		hwlog_err("ti_set_mode unknown step_mode:%d\n", step_mode);
		break;
	}
}
static void st_start_motor(struct drv_data *di)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	drv8846_sleep(di->gpio_sleep, true);
	drv8846_enable(di, false);
	drv8846_power(di->gpio_vm);
	st_set_mode(di, g_step_work_mode);
	usleep_range(2, 3); // 2us
	drv8846_sleep(di->gpio_sleep, false);
	usleep_range(100, 200); // 100us
	if (di->cur_direction == DIR_ROTATE_IN)
		drv8846_set_dir_in(di->gpio_dir, di->direction);
	else if (di->cur_direction == DIR_ROTATE_OUT)
		drv8846_set_dir_out(di->gpio_dir, di->direction);
	else
		hwlog_err("unknown direction\n");
	drv8846_enable(di, true);
	drv8846_step_start(di);
}

static void ti_start_motor(const struct drv_data *di)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	drv8846_sleep(di->gpio_sleep, true);
	drv8846_enable(di, false);
	drv8846_power(di->gpio_vm);
	ti_set_mode(di, g_step_work_mode);

	mdelay(di->power_on_delay_ms);
	drv8846_adec(di->gpio_decay, GPIO_VAL_HIGH);
	if (di->cur_direction == DIR_ROTATE_IN)
		drv8846_set_dir_in(di->gpio_dir, di->direction);
	else if (di->cur_direction == DIR_ROTATE_OUT)
		drv8846_set_dir_out(di->gpio_dir, di->direction);
	else
		hwlog_err("unknown direction\n");

	drv8846_sleep(di->gpio_sleep, false);
	mdelay(di->sleep_wake_delay_ms);
	drv8846_enable(di, true);
	drv8846_step_start(di);
}

static int drv8846_start_immediately(struct drv_data *di)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return FAILURE;
	}
	mutex_lock(&di->start_stop_mutex);

	// init failed, the state have been reset in init func
	if (drv8846_param_init(di) != SUCCESS) {
		hwlog_err("init_state_speed_param failed\n");
		mutex_unlock(&di->start_stop_mutex);
		return FAILURE;
	}

	drv8846_disable_fault_irq(di->irq_fault);
	if (hrtimer_active(&di->irq_enable_timer))
		hrtimer_cancel(&di->irq_enable_timer);
	hrtimer_start(&di->irq_enable_timer,
		ktime_set(di->irq_enable_delay_ms / SEC_MS_RATIO,
			(di->irq_enable_delay_ms % SEC_MS_RATIO) *
				MS_NSEC_RATIO),
		HRTIMER_MODE_REL);

	if (di->ic_vendor == ST_VENDOR)
		st_start_motor(di);
	else if (di->ic_vendor == TI_VENDOR)
		ti_start_motor(di);
	hwlog_info("ic_vendor = %d\n", di->ic_vendor);
	if (hrtimer_active(&di->overtime_timer))
		hrtimer_cancel(&di->overtime_timer);
	hrtimer_start(&di->overtime_timer,
		ktime_set(di->cur_work_dur_ms / SEC_MS_RATIO,
			(di->cur_work_dur_ms % SEC_MS_RATIO) * MS_NSEC_RATIO),
		HRTIMER_MODE_REL);
	print_ts("drv8846_start");

	mutex_unlock(&di->start_stop_mutex);
	return SUCCESS;
}

/*
 * Common function for pushing or pulling motor, it will be blocked.
 *
 * @param di the device info struct.
 * @param mode the speed mode, NORMAL or FAST.
 * @param dir the rotation direction needed.
 * @return -ERESTARTSYS if it was interrupted by a signal, SUCCESS otherwise.
 */
static int drv8846_push_pull(struct drv_data *di, enum speed_mode mode,
	enum direction dir)
{
	int ret = 0;

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return FAILURE;
	}
	mutex_lock(&di->cmd_mutex);
	if (di->step_device == NULL && di->step_gen_manner == PWM_STEP) {
		#ifdef CONFIG_PWM
		di->step_device = pwm_get(di->pdev, NULL);
		if (IS_ERR(di->step_device)) {
			di->step_device = NULL;
			hwlog_err("push_pull, pwm_device get fail\n");
			mutex_unlock(&di->cmd_mutex);
			// ignore it, this will not occur in normal case.
			return SUCCESS;
		}
		#else
			return SUCCESS;
		#endif
	}

	spin_lock(&di->state_change_lock);
	if (di->cur_state == STATE_STILL) {
		di->cur_state = STATE_START;
		di->speed_mode = mode;
		di->cur_direction = dir;
	} else {
		spin_unlock(&di->state_change_lock);
		mutex_unlock(&di->cmd_mutex);
		// the hal restart the syscall
		goto wait_for_still;
	}
	spin_unlock(&di->state_change_lock);
	ret = drv8846_start_immediately(di);
	mutex_unlock(&di->cmd_mutex);

wait_for_still:
	if (ret == SUCCESS) {
		ret = wait_event_interruptible_timeout(di->wq,
			(di->cur_state == STATE_STILL),
			msecs_to_jiffies(WORK_DURATION_MAX));
		if (ret == -ERESTARTSYS)
			return ret;
	}
	return SUCCESS; // leave the failure for hal to check
}

/*
 * Wrapper for pushing motor out.
 *
 * @param di the device info struct.
 * @param mode the speed mode, NORMAL or FAST.
 * @return -ERESTARTSYS if it was interrupted by a signal, SUCCESS otherwise.
 */
static int drv8846_push_out(struct drv_data *di, enum speed_mode mode)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return FAILURE;
	}
	return drv8846_push_pull(di, mode, DIR_ROTATE_OUT);
}

/*
 * Wrapper for pulling motor back.
 *
 * @param di the device info struct.
 * @param mode the speed mode, NORMAL or FAST.
 * @return -ERESTARTSYS if it was interrupted by a signal, SUCCESS otherwise.
 */
static int drv8846_pull_back(struct drv_data *di, enum speed_mode mode)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return FAILURE;
	}
	return drv8846_push_pull(di, mode, DIR_ROTATE_IN);
}

/*
 * Notify the dirver to stop current driving process, this is nonblock.
 *
 * @param di the device info struct.
 * @param cur_dir the rotating direction in hal layer.
 * @return void.
 */
static void drv8846_stop_emergency(struct drv_data *di, enum direction cur_dir)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	mutex_lock(&di->cmd_mutex);

	spin_lock(&di->state_change_lock);
	if (di->cur_state != STATE_STILL && cur_dir == di->cur_direction) {
		if (di->speed_mode == SPEED_FAST &&
			di->cur_direction == DIR_ROTATE_IN)
			goto func_end; // don't stop fast pull back

		if (di->curve_style != NO_START_NO_STOP) {
			if (di->cur_state < STATE_SPEED_DOWN)
				di->cur_state = STATE_EMERGENCY_SPEED_DOWN;
		} else {
			if (di->cur_state < STATE_NORMAL_POWER_DOWN) {
				di->cur_state = STATE_EMERGENCY_POWER_DOWN;
				drv8846_stop(di);
			}
		}
	}
func_end:
	spin_unlock(&di->state_change_lock);

	mutex_unlock(&di->cmd_mutex);
}

/*
 * Wrapper for stop pushing out urgently.
 *
 * @param di the device info struct.
 * @return void.
 */
static void drv8846_stop_push_out(struct drv_data *di)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	drv8846_stop_emergency(di, DIR_ROTATE_OUT);
}

/*
 * Wrapper for stop pulling back urgently.
 *
 * @param di the device info struct.
 * @return void.
 */
static void drv8846_stop_pull_back(struct drv_data *di)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	drv8846_stop_emergency(di, DIR_ROTATE_IN);
}

/*
 * Handler for fault irq, shutdown the drv8846 immediately,
 * running in process context.
 *
 * @param work the work_struct in work queue.
 * @return void.
 */
static void drv8846_fault_int_work_func(struct work_struct *work)
{
	struct drv_data *di = container_of(work, struct drv_data,
		fault_int_work);

	spin_lock(&di->state_change_lock);
	di->cur_state = STATE_FAULT_POWER_DOWN;
	spin_unlock(&di->state_change_lock);

	drv8846_stop_immediately(di);
	drv8846_enable_fault_irq(di->irq_fault);
	hwlog_err("fault_int occurred\n");
}

/*
 * Handler for fault irq, disable it and shutdown the drv8846 immediately,
 * running in irq context.
 *
 * @data the data private to this irq.
 * @return irqreturn_t IRQ_HANDLED this irq is handled.
 */
static irqreturn_t drv8846_fault_irq_handle(int irq, void *data)
{
	struct drv_data *di = data;

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return IRQ_NONE;
	}
	drv8846_disable_fault_irq(di->irq_fault);
	schedule_work(&di->fault_int_work);

	return IRQ_HANDLED;
}

/*
 * Common entrance for controlling drv8846 for user space command.
 *
 * @param di the device info struct.
 * @val the command from user space.
 * @return int the command processed result -ERESTARTSYS or SUCCESS.
 */
static int drv8846_control(struct drv_data *di, int val)
{
	enum control_cmd cmd = (enum control_cmd)val;

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return FAILURE;
	}
	switch (cmd) {
	case CMD_PUSH_OUT:
		return drv8846_push_out(di, SPEED_NORMAL);
	case CMD_PULL_BACK_NORMAL:
		return drv8846_pull_back(di, SPEED_NORMAL);
	case CMD_PULL_BACK_FAST:
		return drv8846_pull_back(di, SPEED_FAST);
	case CMD_STOP_PUSH_OUT:
		drv8846_stop_push_out(di);
		break;
	case CMD_STOP_PULL_BACK:
		drv8846_stop_pull_back(di);
		break;
	default:
		hwlog_err("unknown control cmd:%d\n", val);
		break;
	}
	return SUCCESS;
}

/*
 * Request the pinctrl handle.
 *
 * @param di the device info struct.
 * @return void.
 */
static void drv8846_get_pinctrl_res(struct drv_data *di)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	di->pinctrl = pinctrl_get(di->pdev);

	if (IS_ERR(di->pinctrl)) {
		hwlog_err("could not get pinctrl\n");
		return;
	}

	di->pinctrl_idle = pinctrl_lookup_state(di->pinctrl,
		PINCTRL_STATE_IDLE);
	if (IS_ERR(di->pinctrl_idle)) {
		hwlog_err("could not get idle state\n");
		return;
	}

	di->pinctrl_default = pinctrl_lookup_state(di->pinctrl,
		PINCTRL_STATE_DEFAULT);
	if (IS_ERR(di->pinctrl_default)) {
		hwlog_err("could not get default state\n");
		return;
	}
}

/*
 * Validate the param name part, format must be strictly name=value,
 * where only lower letter and _ is * valid for name.
 *
 * @param name the name string.
 * @param len the length of the name.
 * @return bool true if valid, false otherwise.
 */
static bool drv8846_is_param_name_valid(const char *name, int len)
{
	int i;

	if (!name || len < 0) {
		hwlog_err("%s name is null or len is less than 0\n", __func__);
		return false;
	}
	for (i = 0; i < len; i++) {
		if (!((name[i] == '_') || ((name[i] >= 'a') &&
			(name[i] <= 'z'))))
			return false;
	}
	return true;
}

/*
 * Retrieve the command name from user space in sysfs.
 *
 * @param buf the buffer containing the input.
 * @param count the length in bytes for buf.
 * @param name the output buffer pointer.
 * @param max_len the maximum length for output buffer.
 * @return int SUCCESS of FAILURE.
 */
static int drv8846_get_param_name(const char *buf, size_t count,
	char *name, size_t max_len)
{
	int len = count;
	int start_pos = INVALID_POS;
	int end_pos = INVALID_POS;
	int name_len;
	int i;

	if (!buf || !name || count <= 0 || max_len <= 0) {
		hwlog_err("%s param is invalid\n", __func__);
		return FAILURE;
	}

	for (i = 0; i < len; i++) {
		if (start_pos == INVALID_POS) {
			if (buf[i] == '=')
				return FAILURE;
			start_pos = i;
		} else if (end_pos == INVALID_POS) {
			if (buf[i] == '=')
				end_pos = i - 1;
		} else {
			break;
		}
	}

	if (start_pos == INVALID_POS || end_pos == INVALID_POS)
		return FAILURE;

	name_len = end_pos - start_pos + 1;
	if (name_len > max_len)
		return FAILURE;
	strncpy(name, buf + start_pos, name_len);
	if (drv8846_is_param_name_valid(name, name_len))
		return SUCCESS;
	else
		return FAILURE;
}

/*
 * Validate the param value part, format must be strictly name=value,
 * where only digital number is * * valid for value.
 *
 * @param value the value string.
 * @param len the length of the value.
 * @return bool true if valid, false otherwise.
 */
static bool drv8846_is_param_value_valid(const char *value, int len)
{
	int i;

	if (!value || len < 0) {
		hwlog_err("%s value is null or len is less than 0\n", __func__);
		return false;
	}
	for (i = 0; i < len; i++) {
		if (!((value[i] >= '0') && (value[i] <= '9')))
			return false;
	}
	return true;
}

/*
 * Retrieve the command value from user space in sysfs.
 *
 * @param buf the buffer containing the input.
 * @param count the length in bytes for buf.
 * @param value the output buffer pointer.
 * @param max_len the maximum length for output buffer.
 * @return int SUCCESS of FAILURE.
 */
static int drv8846_get_param_value(const char *buf, size_t count,
	char *value, size_t max_len)
{
	int len = count;
	int equal_pos = INVALID_POS;
	int start_pos = INVALID_POS;
	int end_pos = INVALID_POS;
	int value_len;
	int i;

	if (!buf || !value || count <= 0 || max_len <= 0) {
		hwlog_err("%s param is invalid\n", __func__);
		return FAILURE;
	}

	for (i = 0; i < len; i++) {
		if (equal_pos == INVALID_POS) {
			if (buf[i] == '=')
				equal_pos = i;
		} else if (start_pos == INVALID_POS) {
			start_pos = i;
		} else if (end_pos == INVALID_POS) {
			if (buf[i] == '\n' || buf[i] == '\r')
				end_pos = i - 1;
		} else {
			break;
		}
	}

	if (equal_pos == INVALID_POS || start_pos == INVALID_POS)
		return FAILURE;
	if (end_pos == INVALID_POS)
		end_pos = len - 1;
	value_len = end_pos - start_pos + 1;
	if (value_len > max_len)
		return FAILURE;
	strncpy(value, buf + start_pos, value_len);
	if (drv8846_is_param_value_valid(value, value_len))
		return SUCCESS;
	else
		return FAILURE;
}

/*
 * Translate the step_mode to drv8846's related params.
 *
 * @param di the device info struct.
 * @param step_mode the enum val.
 * @return void.
 */
static void drv8846_param_set_step_mode(struct drv_data *di,
	enum drv_step_mode step_mode)
{
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	di->rising_edge_valid = false;
	di->falling_edge_valid = false;
	switch (step_mode) {
	case FULL_STEP_RISING_ONLY:
		di->microsteps = MOTOR_DRV_MICROSTEPS_MIN;
		di->rising_edge_valid = true;
		break;
	case TWO_STEP_RISING_ONLY:
		di->microsteps = MOTOR_DRV_MICROSTEPS_TWO;
		di->rising_edge_valid = true;
		break;
	case FOUR_STEP_RISING_ONLY:
		di->microsteps = MOTOR_DRV_MICROSTEPS_FOUR;
		di->rising_edge_valid = true;
		break;
	case EIGHT_STEP_RISING_ONLY:
		di->microsteps = MOTOR_DRV_MICROSTEPS_EIGHT;
		di->rising_edge_valid = true;
		break;
	case EIGHT_STEP_RISING_FALLING:
		di->microsteps = MOTOR_DRV_MICROSTEPS_EIGHT;
		di->rising_edge_valid = true;
		di->falling_edge_valid = true;
		break;
	case SIXTEEN_STEP_RISING_ONLY:
		di->microsteps = MOTOR_DRV_MICROSTEPS_SIXTEEN;
		di->rising_edge_valid = true;
		break;
	case SIXTEEN_STEP_RISING_FALLING:
		di->microsteps = MOTOR_DRV_MICROSTEPS_SIXTEEN;
		di->rising_edge_valid = true;
		di->falling_edge_valid = true;
		break;
	case THIRTY_TWO_STEP_RISING_ONLY:
		di->microsteps = MOTOR_DRV_MICROSTEPS_MAX;
		di->rising_edge_valid = true;
		break;
	case THIRTY_TWO_STEP_RISING_FALLING:
		di->microsteps = MOTOR_DRV_MICROSTEPS_MAX;
		di->rising_edge_valid = true;
		di->falling_edge_valid = true;
		break;
	default:
		di->microsteps = MOTOR_DRV_MICROSTEPS_MIN;
		di->rising_edge_valid = true;
		break;
	}
}

#ifdef STEP_MOTOR_DEBUG
/*
 * Debug function for setting gpio pin's state.
 *
 * @param di the device info struct.
 * @param name the name of gpio pin.
 * @param val the value to set.
 * @return bool true for handled or false otherwise.
 */
static bool drv8846_handle_gpio_set(const struct drv_data *di,
	const char *name, int val)
{
	bool handled = true;
	int value = val;

	if (!di || !name || val <= 0) {
		hwlog_err("%s param is invalid\n", __func__);
		return false;
	}
	if (strncmp("gpio_", name, strlen("gpio_")) != 0)
		return false;

	if (strcmp("gpio_enable", name) == 0) {
		drv8846_enable(di, value == GPIO_VAL_LOW);
	} else if (strcmp("gpio_sleep", name) == 0) {
		drv8846_sleep(di->gpio_sleep, value == GPIO_VAL_LOW);
	} else if (strcmp("gpio_step", name) == 0) {
		if (value == GPIO_VAL_HIGH)
			drv8846_step_start(di);
		else
			drv8846_step_stop(di);
	} else if (strcmp("gpio_adec", name) == 0) {
		drv8846_adec(di->gpio_decay, value);
	} else if (strcmp("gpio_mode", name) == 0) {
		st_set_mode(di, g_step_work_mode);
	} else if (strcmp("gpio_dir", name) == 0) {
		drv8846_direction(di->gpio_dir, value);
	} else if (strcmp("gpio_vm", name) == 0) {
		drv8846_power(di->gpio_vm);
	} else {
		handled = false;
	}

	return handled;
}

/*
 * Debug function for setting delay realted params.
 *
 * @param di the device info struct.
 * @param name the name of delay param.
 * @param val the value to set.
 * @return bool true for handled or false otherwise.
 */
static bool drv8846_handle_delay_param_set(struct drv_data *di,
	const char *name, int val)
{
	bool handled = true;

	if (!di || !name || val <= 0) {
		hwlog_err("%s param is invalid\n", __func__);
		return false;
	}
	if (strncmp("delay_", name, strlen("delay_")) != 0)
		return false;

	if (strcmp("delay_irq_enable_ms", name) == 0)
		di->irq_enable_delay_ms = val;
	else if (strcmp("delay_sleep_wake_ms", name) == 0)
		di->sleep_wake_delay_ms = val;
	else if (strcmp("delay_power_on_ms", name) == 0)
		di->power_on_delay_ms = val;
	else if (strcmp("delay_dir_set_ns", name) == 0)
		di->dir_set_delay_ns = val;
	else
		handled = false;
	return handled;
}

/*
 * Common debug function for setting some variable.
 *
 * @param di the device info struct.
 * @param val the value to set.
 * @param var the pointer of the variable to be set.
 * @return void.
 */
static void drv8846_set_val_per_step_mode(const struct drv_data *di, int val,
	int *var)
{
	if (!di || !var || val <= 0) {
		hwlog_err("%s param is invalid\n", __func__);
		return;
	}
	// 烧片版本debug专用，溢出无异常
	*var = val * di->microsteps;
	if (di->rising_edge_valid && di->falling_edge_valid)
		*var = (*var) / HALF_DIVISOR;
}

/*
 * Debug function for setting start stop curve realted params.
 *
 * @param di the device info struct.
 * @param name the name of param.
 * @param val the value to set.
 * @return bool true for handled or false otherwise.
 */
static bool drv8846_handle_speed_param_start_stop_curve(struct drv_data *di,
	const char *name, int val)
{
	bool handled = true;

	if (!di || !name || val <= 0) {
		hwlog_err("%s param is invalid\n", __func__);
		return false;
	}
	if (strcmp("speed_target_fast", name) == 0) {
		if (val >= TARGET_SPEED_MIN && val <= TARGET_SPEED_MAX)
			drv8846_set_val_per_step_mode(di, val,
				&di->target_speed_fast);
		else
			hwlog_err("invalid speed_target_fast\n");
	} else if (strcmp("speed_target_normal", name) == 0) {
		if (val >= TARGET_SPEED_MIN && val <= TARGET_SPEED_MAX)
			drv8846_set_val_per_step_mode(di, val,
				&di->target_speed_normal);
		else
			hwlog_err("invalid speed_target_normal\n");
	} else if (strcmp("speed_dur_per_step_ms", name) == 0) {
		if (val >= DUR_PER_STEP_MIN && val <= DUR_PER_STEP_MAX)
			di->dur_per_step_ms = val;
		else
			hwlog_err("invalid speed_dur_per_step_ms\n");
	} else if (strcmp("speed_step", name) == 0) {
		if (val >= SPEED_STEP_MIN && val <= SPEED_STEP_MAX)
			drv8846_set_val_per_step_mode(di, val, &di->speed_step);
		else
			hwlog_err("invalid speed_step\n");
	} else if (strcmp("speed_starting", name) == 0) {
		if (val >= STARTING_SPEED_MIN && val <= STARTING_SPEED_MAX)
			drv8846_set_val_per_step_mode(di, val,
				&di->start_speed);
		else
			hwlog_err("invalid speed_starting\n");
	} else {
		handled = false;
	}
	return handled;
}

/*
 * Debug function for setting the target speed according to step mode.
 *
 * @param di the device info struct.
 * @param step_mode the step mode set by user.
 * @return void.
 */
static void drv8846_reset_target_speed(struct drv_data *di,
	enum drv_step_mode step_mode)
{
	int old_microsteps;

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	old_microsteps = di->microsteps;
	if (di->rising_edge_valid && di->falling_edge_valid)
		old_microsteps /= HALF_DIVISOR;
	drv8846_param_set_step_mode(di, step_mode);

	if (old_microsteps == 0) {
		hwlog_err("%s old_microsteps is 0\n", __func__);
		return;
	}
	di->target_speed_normal /= old_microsteps;
	di->target_speed_fast /= old_microsteps;
	di->target_speed_normal *= di->microsteps;
	di->target_speed_fast *= di->microsteps;
	if (di->rising_edge_valid && di->falling_edge_valid) {
		di->target_speed_normal /= HALF_DIVISOR;
		di->target_speed_fast /= HALF_DIVISOR;
	}
	di->target_speed = di->target_speed_normal;
}

/*
 * Debug function for setting speed realted params.
 *
 * @param di the device info struct.
 * @param name the name of param.
 * @param val the value to set.
 * @return bool true for handled or false otherwise.
 */
static bool drv8846_handle_speed_param_set(struct drv_data *di,
	const char *name, int val)
{
	bool handled = true;

	if (!di || !name || val <= 0) {
		hwlog_err("%s param is invalid\n", __func__);
		return false;
	}
	if (strncmp("speed_", name, strlen("speed_")) != 0)
		return false;

	if (drv8846_handle_speed_param_start_stop_curve(di, name, val)) {
		return true;
	} else if (strcmp("speed_mode", name) == 0) {
		if (val >= SPEED_NORMAL && val <= SPEED_FAST)
			di->speed_mode = val;
		else
			hwlog_err("invalid speed_mode\n");
	} else if (strcmp("speed_start_stop_curve_style", name) == 0) {
		if (val >= NO_START_NO_STOP && val <= SLOW_START_SLOW_STOP)
			di->curve_style = val;
		else
			hwlog_err("invalid start_stop_curve_style\n");
	} else if (strcmp("speed_motor_walk_distance_tenth", name) == 0) {
		if (val >= DIST_MIN_MM_TENTH && val <= DIST_MAX_MM_TENTH)
			di->motor_walk_distance_tenth = val;
		else
			hwlog_err("invalid walk_distance\n");
	} else if (strcmp("speed_motor_walk_time_user_ms", name) == 0) {
		if (val >= WALK_TIME_MIN_MS && val <= WALK_TIME_MAX_MS) {
			di->motor_walk_time_ms = val;
			if (di->step_gen_manner == GPIO_STEP) {
				if (di->motor_walk_time_ms != 0)
					di->dist_control_type = PULSE_GEN_DUR_TIMER;
				else
					di->dist_control_type = DEFAULT_DIST_CTRL_TYPE;
			}
		} else {
			hwlog_err("invalid walk_time\n");
		}
	} else if (strcmp("speed_step_mode", name) == 0) {
		if (val > STEP_MODE_GUARD_MIN && val < STEP_MODE_GUARD_MAX)
			drv8846_reset_target_speed(di, (enum drv_step_mode)val);
		else
			hwlog_err("invalid step_mode\n");
	} else {
		handled = false;
	}
	return handled;
}

/*
 * Debug function for setting overtime realted params.
 *
 * @param di the device info struct.
 * @param name the name of param.
 * @param val the value to set.
 * @return bool true for handled or false otherwise.
 */
static bool drv8846_handle_overtime_protect_param_set(struct drv_data *di,
	const char *name, int val)
{
	bool handled = true;

	if (!di || !name || val <= 0) {
		hwlog_err("%s param is invalid\n", __func__);
		return false;
	}
	if (strncmp("overtime_", name, strlen("overtime_")) != 0)
		return false;

	if (strcmp("overtime_protect_normal_ms", name) == 0) {
		if (val >= WORK_DURATION_MIN && val <= WORK_DURATION_MAX)
			di->work_dur_normal_ms = val;
		else
			hwlog_err("invalid overtime_protect_normal_ms\n");
	} else if (strcmp("overtime_protect_fast_ms", name) == 0) {
		if (val >= WORK_DURATION_MIN && val <= WORK_DURATION_MAX)
			di->work_dur_fast_ms = val;
		else
			hwlog_err("invalid overtime_protect_fast_ms\n");
	} else {
		handled = false;
	}

	return handled;
}
#endif

/*
 * Common function for handling param set in sysfs from user space.
 *
 * @param dev the related device to sysfs node.
 * @param attr the related attribute for this calling.
 * @param buf set value from user space.
 * @param count the length of buf.
 * @return ssize_t the count consumed by this function.
 */
static ssize_t drv8846_param_set_value(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct drv_data *di = NULL;
	int ret;
	int val;
	char name[PARAM_NAME_LEN + 1] = {0};
	char value[PARAM_VALUE_LEN + 1] = {0};

	if (!dev || !attr || !buf) {
		hwlog_err("invalid param!\n");
		return count;
	}

	di = dev_get_drvdata(dev);
	if (di == NULL) {
		hwlog_err("di get from dev is NULL\n");
		return count;
	}

	ret = drv8846_get_param_name(buf, count, name, PARAM_NAME_LEN);
	if (ret != SUCCESS) {
		hwlog_err("invalid format name\n");
		return count;
	}
	ret = drv8846_get_param_value(buf, count, value, PARAM_VALUE_LEN);
	if (ret != SUCCESS) {
		hwlog_err("invalid format value\n");
		return count;
	}

	ret = kstrtoint(value, 0, &val);
	if (ret < 0 || val < 0) {
		hwlog_err("invalid value\n");
		return count;
	}

	hwlog_info("user cmd:%s=%s\n", name, value);

	if (strcmp("control_cmd", name) == 0) {
		ret = drv8846_control(di, val);
		if (ret != SUCCESS)
			return ret;
		return count;
	}

#ifdef STEP_MOTOR_DEBUG
	if (drv8846_handle_gpio_set(di, name, val))
		return count;
	else  if (drv8846_handle_delay_param_set(di, name, val))
		return count;
	else if (drv8846_handle_speed_param_set(di, name, val))
		return count;
	else if (drv8846_handle_overtime_protect_param_set(di, name, val))
		return count;
	hwlog_err("unknown param:%s\n", name);
#endif

	return count;
}

static DEVICE_ATTR(param, 0200, NULL, drv8846_param_set_value);
static DEVICE_ATTR(emergency, 0200, NULL, drv8846_param_set_value);

static struct attribute *g_drv8846_attributes[] = {
	&dev_attr_param.attr,
	&dev_attr_emergency.attr,
	NULL
};

static const struct attribute_group g_drv8846_attr_group = {
	.attrs = g_drv8846_attributes,
};

static void parse_dt_configs(struct drv_data *di)
{
	struct device_node *np = NULL;
	u32 tmp = 0;
	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}

	np = di->pdev->of_node;
	if (of_property_read_u32(np, "step_gen_manner", &tmp) == 0)
		di->step_gen_manner = (enum step_gen_manner)tmp;
	else
		di->step_gen_manner = DEFAULT_STEP_GEN_MANNER;
	if (di->step_gen_manner == PWM_STEP)
		di->dist_control_type = PULSE_GEN_DUR_TIMER;
	else
		di->dist_control_type = PULSE_COUNTER;
	hwlog_info("di->dist_control_type is :%d\n", di->dist_control_type);
	if (of_property_read_u32(np, "set_step_mode", &tmp) == 0) {
		g_step_work_mode = (enum drv_step_mode)tmp;
		if (di->ic_vendor == TI_VENDOR)
			g_step_work_mode = FULL_STEP_RISING_ONLY;
        } else {
		g_step_work_mode = FULL_STEP_RISING_ONLY;
        }
	hwlog_info("step_work_mode is :%d, tmp: %d\n", g_step_work_mode, tmp);
	if (of_property_read_u32(np, "set_start_stop_curve_style", &tmp) == 0) {
		di->curve_style = (enum start_stop_curve_style)tmp;
		if (di->ic_vendor == TI_VENDOR)
			di->curve_style = DEFAULT_CURVE_STYLE;
        } else {
		di->curve_style = DEFAULT_CURVE_STYLE;
        }
	hwlog_info("set_start_stop_curve_style is :%d, tmp: %d\n", di->curve_style, tmp);
}

static void drv8846_param_set_default_val(struct drv_data *di)
{
	int speed_multiplier;

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	parse_dt_configs(di);
	drv8846_param_set_step_mode(di, g_step_work_mode);
	speed_multiplier = di->microsteps;
	if (di->rising_edge_valid && di->falling_edge_valid)
		speed_multiplier /= HALF_DIVISOR;

	di->start_speed = STARTING_SPEED_STD * speed_multiplier;
	di->speed_step = STARTING_SPEED_STEP_STD * speed_multiplier;
	di->dur_per_step_ms = START_DUR_PER_STEP_MS;
	di->cur_speed = STARTING_SPEED_STD * speed_multiplier;
	di->target_speed_normal = TARGET_SPEED_NORMAL_STD * speed_multiplier;
	di->target_speed_fast = TARGET_SPEED_FAST_STD * speed_multiplier;
	di->target_speed = di->target_speed_normal;
	di->speed_mode = SPEED_NORMAL;

	di->work_dur_normal_ms = WORK_DURATION_MS_NORMAL_STD;
	di->work_dur_fast_ms = WORK_DURATION_MS_FAST_STD;
	di->cur_work_dur_ms = WORK_DURATION_MS_NORMAL_STD;
	di->power_on_delay_ms = POWER_ON_DELAY_MS;
	di->sleep_wake_delay_ms = SLEEP_WAKE_DELAY_MS;
	di->dir_set_delay_ns = DIR_SET_DELAY_NS;
	di->irq_enable_delay_ms = FAULT_IRQ_ENABLE_DELAY_MS;

	di->cur_state = STATE_STILL;
	di->cur_direction = DIR_ROTATE_NO;
	di->motor_walk_distance_tenth = DIST_STD_MM_TENTH;
	di->cur_pulse_edge_count = 0;
	di->total_pulse_edge_count = 0;
	di->start_pulse_edge_count = 0;
	di->speed_up_run_time_ms = 0;
	di->full_speed_dur_ms = 0;
	di->speed_down_run_time_ms = 0;
	di->motor_walk_time_ms = 0;
	di->step_device = NULL;
	if (di->ic_vendor == TI_VENDOR) {
		di->start_speed = STARTING_SPEED_STD_TIFULL * speed_multiplier;
		di->speed_step = STARTING_SPEED_STEP_STD_TIFULL * speed_multiplier;
		di->cur_speed = STARTING_SPEED_STD_TIFULL * speed_multiplier;
		di->target_speed_normal = TARGET_SPEED_NORMAL_STD_TIFULL * speed_multiplier;
	}
}

static int find_vendor_by_gpio(struct drv_data *di)
{
	int u_value;
	int d_value;
	int ret;

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return FAILURE;
	}
	ret = gpio_direction_input(di->gpio_enable);
	if (ret < 0) {
		hwlog_err("%s set input mode error, error=%d\n",
			__func__, ret);
		return FAILURE;
	}
	ret = pinctrl_select_state(di->pinctrl, di->pinctrl_default);
	if (ret < 0) {
		hwlog_err("%s pinctrl_select_state default error, error=%d\n",
			__func__, ret);
		return FAILURE;
	}
	mdelay(10);
	d_value = gpio_get_value_cansleep(di->gpio_enable);

	ret = gpio_direction_input(di->gpio_enable);
	if (ret < 0) {
		hwlog_err("%s set input mode error, error=%d\n",
			__func__, ret);
		return FAILURE;
	}
	ret = pinctrl_select_state(di->pinctrl, di->pinctrl_idle);
	if (ret < 0) {
		hwlog_err("%s pinctrl_select_state idle error, error=%d\n",
			__func__, ret);
		return FAILURE;
	}
	mdelay(10);
	u_value = gpio_get_value_cansleep(di->gpio_enable);

	if (d_value == u_value) {
		if (u_value == GPIO_VAL_LOW) {
			di->ic_vendor = ST_VENDOR;
		} else {
			hwlog_info("d_value and u_value is high\n");
			di->ic_vendor = TI_VENDOR;
		}
	} else {
		di->ic_vendor = TI_VENDOR;
	}
	hwlog_info("select vendor_id = %d\n", di->ic_vendor);
	return SUCCESS;
}

static int drv8846_gpio_init(struct drv_data *di)
{
	int ret;
	struct device_node *np = NULL;

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return FAILURE;
	}
	np = di->pdev->of_node;
	if (drv_request_gpio(di, "gpio_vm", &di->gpio_vm) != SUCCESS)
		return FAILURE;
	gpio_direction_output(di->gpio_vm, GPIO_VAL_HIGH);
	hwlog_info("gpio_direction_output di->gpio_vm = %d\n", di->gpio_vm);
	if (drv_request_gpio(di, "gpio_step", &di->gpio_step) != SUCCESS)
		return FAILURE;
	if (drv_request_gpio(di, "gpio_dir", &di->gpio_dir) != SUCCESS)
		return FAILURE;
	if (drv_request_gpio(di, "gpio_decay", &di->gpio_decay) != SUCCESS)
		return FAILURE;
	if (drv_request_gpio(di, "gpio_enable", &di->gpio_enable) != SUCCESS)
		return FAILURE;
	if (drv_request_gpio(di, "gpio_sleep", &di->gpio_sleep) != SUCCESS)
		return FAILURE;
	if (drv_request_gpio(di, "gpio_mode1", &di->gpio_mode1) != SUCCESS)
		return FAILURE;
	if (drv_request_gpio(di, "gpio_st_en", &di->gpio_st_en) != SUCCESS)
		return FAILURE;

	di->gpio_fault = of_get_named_gpio(np, "gpio_fault", 0);
	if (!gpio_is_valid(di->gpio_fault)) {
		hwlog_err("gpio fault invalid\n");
		return FAILURE;
	}
	ret = devm_gpio_request_one(di->pdev, di->gpio_fault, GPIOF_IN,
		"drv8846_fault_int");
	if (ret != 0) {
		hwlog_err("irq fault request fail, ret:%d\n", ret);
		return FAILURE;
	}
	di->irq_fault = gpio_to_irq(di->gpio_fault);
	ret = devm_request_irq(di->pdev, di->irq_fault,
	drv8846_fault_irq_handle, IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND, "drv8846",
		di);
	if (ret != 0) {
		hwlog_err("request fault irq fail\n");
		return FAILURE;
	}
	disable_irq_nosync(di->irq_fault); // init irq disabled

	return SUCCESS;
}

static void step_motor_set_direction(struct drv_data *di)
{
	int temp = 0;
	struct device_node *np = NULL;

	if (!di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	np = di->pdev->of_node;
	if (of_property_read_u32(np, "motor_direction", &temp) == SUCCESS)
		di->direction = (temp >> (di->ic_vendor)) & 0x01;
	else
		di->direction = 0;
	hwlog_info("step_direction is %d, temp is 0x%x", di->direction, temp);
}
/*
 * Device probe routine for driver.
 *
 * @param pdev the platform bus's device struct.
 * @return int error code of probing result.
 */
static int drv8846_probe(struct platform_device *pdev)
{
	struct drv_data *di = NULL;

	if (!pdev) {
		hwlog_err("%s pdev is null\n", __func__);
		return -EFAULT;
	}
	if (!of_match_node(g_drv8846_match, pdev->dev.of_node)) {
		hwlog_err("dev node is not match exiting\n");
		return -ENODEV;
	}

	di = devm_kzalloc(&pdev->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->pdev = &pdev->dev;
	drv8846_get_pinctrl_res(di);
	if (drv8846_gpio_init(di) != SUCCESS)
		return -EFAULT;
	if (find_vendor_by_gpio(di) != SUCCESS)
		return -EFAULT;
	drv8846_param_set_default_val(di);
	step_motor_set_direction(di);

	mutex_init(&di->start_stop_mutex);
	mutex_init(&di->cmd_mutex);
	spin_lock_init(&di->state_change_lock);
	init_waitqueue_head(&di->wq);
	INIT_WORK(&di->stop_work, drv8846_stop_work_func);
	INIT_WORK(&di->fault_int_work, drv8846_fault_int_work_func);

	hrtimer_init(&di->step_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	di->step_timer.function = drv8846_step_timer_func;
	hrtimer_init(&di->speed_control, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	di->speed_control.function = drv_speed_ctrl_timer_func;
	hrtimer_init(&di->overtime_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	di->overtime_timer.function = drv8846_protect_timer_func;
	hrtimer_init(&di->irq_enable_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	di->irq_enable_timer.function = drv_irq_enable_timer_func;

	platform_set_drvdata(pdev, di);

	if (sysfs_create_group(&di->pdev->kobj, &g_drv8846_attr_group) != 0)
		hwlog_err("sysfs create group fail\n");

	hwlog_info("init ok\n");

	return 0;
}

/*
 * Device remove routine for driver.
 *
 * @param pdev the platform bus's device struct.
 * @return int error code of removing result.
 */
static int drv8846_remove(struct platform_device *pdev)
{
	struct drv_data *di =
		(struct drv_data *)platform_get_drvdata(pdev);

	if (!di) {
		hwlog_err("remove di is NULL\n");
		return 0;
	}
#ifdef CONFIG_PWM
	pwm_put(di->step_device);
#endif
	mutex_destroy(&di->start_stop_mutex);
	mutex_destroy(&di->cmd_mutex);
	devm_free_irq(di->pdev, di->irq_fault, NULL);
	devm_gpio_free(di->pdev, di->gpio_vm);
	devm_gpio_free(di->pdev, di->gpio_step);
	devm_gpio_free(di->pdev, di->gpio_dir);
	devm_gpio_free(di->pdev, di->gpio_decay);
	devm_gpio_free(di->pdev, di->gpio_enable);
	devm_gpio_free(di->pdev, di->gpio_sleep);
	devm_kfree(di->pdev, di);
	return 0;
}

/*
 * Device shutdown routine for driver.
 *
 * @param pdev the platform bus's device struct.
 * @return void.
 */
static void drv8846_shutdown(struct platform_device *pdev)
{
}

/*
 * Device suspend routine for driver.
 *
 * @param pdev the platform bus's device struct.
 * @param state pm related state.
 * @return void.
 */
static int drv8846_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

/*
 * Device resume routine for driver.
 *
 * @param pdev the platform bus's device struct.
 * @return int error code for resuming.
 */
static int drv8846_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver g_drv8846_driver = {
	.probe = drv8846_probe,
	.remove = drv8846_remove,
	.shutdown = drv8846_shutdown,
#ifdef CONFIG_PM
	.suspend = drv8846_suspend,
	.resume = drv8846_resume,
#endif
	.driver = {
		.name   = "drv8846",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(g_drv8846_match),
	},
};

static int __init drv8846_init(void)
{
	return platform_driver_register(&g_drv8846_driver);
}

static void __exit drv8846_exit(void)
{
	platform_driver_unregister(&g_drv8846_driver);
}

module_init(drv8846_init);
module_exit(drv8846_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("step motor driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");


