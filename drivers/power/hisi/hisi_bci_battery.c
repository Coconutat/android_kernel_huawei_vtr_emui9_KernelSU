/*
 * linux/drivers/power/hisi/hisi_bci_battery.c
 *
 * hisi:battery driver for Linux
 *
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/wakelock.h>
#include <linux/notifier.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <linux/power/hisi/hisi_bci_battery.h>
#include <linux/interrupt.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include <linux/timer.h>
#include <linux/rtc.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/version.h>
#ifdef CONFIG_HUAWEI_TYPEC
#include <huawei_platform/usb/hw_typec_platform.h>
#include <huawei_platform/usb/hw_typec_dev.h>
#endif
#ifdef CONFIG_HUAWEI_CHARGER
#include <huawei_platform/power/huawei_charger.h>
#else
#include <linux/power/hisi/charger/hisi_charger.h>
#endif

#ifdef CONFIG_DIRECT_CHARGER
#include <huawei_platform/power/direct_charger.h>
#endif
#ifdef CONFIG_TCPC_CLASS
#include <huawei_platform/usb/hw_pd_dev.h>
#endif

#include <huawei_platform/power/power_dsm.h>

#ifdef  CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
#include <huawei_platform/power/usb_short_circuit_protect.h>
#endif
#ifdef CONFIG_WIRELESS_CHARGER
#include <huawei_platform/power/wireless_charger.h>
#endif
#include "securec.h"

#define BCI_LOG_INFO
#ifndef BCI_LOG_INFO
#define bci_debug(fmt, args...)do {} while (0)
#define bci_info(fmt, args...) do {} while (0)
#define bci_warn(fmt, args...) do {} while (0)
#define bci_err(fmt, args...)  do {} while (0)
#else
#define bci_debug(fmt, args...)do { printk(KERN_DEBUG   "[hisi_bci]" fmt, ## args); } while (0)
#define bci_info(fmt, args...) do { printk(KERN_INFO    "[hisi_bci]" fmt, ## args); } while (0)
#define bci_warn(fmt, args...) do { printk(KERN_WARNING"[hisi_bci]" fmt, ## args); } while (0)
#define bci_err(fmt, args...)  do { printk(KERN_ERR   "[hisi_bci]" fmt, ## args); } while (0)
#endif

#define WINDOW_LEN                   (10)
#define CAPACITY_DEC_WINDOW_LEN                   (20)
struct batt_dsm {
	int error_no;
	bool notify_enable;
	void (*dump)(char*);
	int (*check_error) (char *buf);
};
static struct wake_lock low_power_lock;
static int is_low_power_locked = 0;
static unsigned int capacity_filter[WINDOW_LEN];
static unsigned int capacity_sum;
static unsigned int capacity_dec_filter[CAPACITY_DEC_WINDOW_LEN];
static unsigned int capacity_dec_sum;
static unsigned int capacity_dec_init_value;
static unsigned int capacity_dec_cnt;
static unsigned int suspend_capacity;
static bool capacity_debounce_flag = false;/*lint !e551*/
static int removable_batt_flag = 0;
static int capacity_dec_start_event_flag = 0;
static int is_board_type = 0;
static int is_fake_battery = 0;
static int google_battery_node = 0;
struct kobject *g_sysfs_bq_bci = NULL;
module_param(is_fake_battery, int, 0644);

struct hisi_bci_device_info {
	int bat_voltage;
	int bat_max_volt;
	int bat_temperature;
	int bat_exist;
	int bat_health;
	int bat_capacity;
	int bat_capacity_level;
	int bat_technolog;
	int bat_design_fcc;
	int bat_rm;
	int bat_fcc;
	int bci_soc_at_term;
	int bat_current;
	unsigned int bat_err;
	int charge_status;
	int power_supply_status;
	u8 usb_online;
	u8 ac_online;
	u8 chargedone_stat;
	u16 monitoring_interval;
	int watchdog_timer_status;
	unsigned long event;
	unsigned int capacity;
	unsigned int capacity_filter_count;
	unsigned int prev_capacity;
	unsigned int charge_full_count;
	unsigned int wakelock_enabled;
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
	struct power_supply bat;
	struct power_supply usb;
	struct power_supply bat_google;
	struct power_supply ac;
	struct power_supply bk_bat;
	#else
	struct power_supply    *bat;
	struct power_supply    *usb;
	struct power_supply    *bat_google;
	struct power_supply    *ac;
	struct power_supply    *wireless;
	struct power_supply    *bk_bat;
	#endif
	struct device *dev;
	struct notifier_block nb;
	struct delayed_work hisi_bci_monitor_work;
	struct work_interval_para interval_data[WORK_INTERVAL_PARA_LEVEL];
	struct hrtimer capacity_dec_timer;
	int capacity_dec_base_decimal;
	int capacity_dec_sample_nums;
	int capacity_dec_timer_interval;
	int capacity_dec_total_level;
	struct capacity_dec_para cap_dec_para[CAPATICY_DEC_PARA_LEVEL];
	unsigned int capacity_dec;
    struct cap_lock_para vth_correct_data[CAP_LOCK_PARA_LEVEL];
    unsigned int vth_correct_en;
};

struct hisi_bci_device_info *g_hisi_bci_dev;

BLOCKING_NOTIFIER_HEAD(notifier_list);/*lint !e64 !e570 !e651*/

static enum power_supply_property hisi_bci_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_LIMIT_FCC,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CAPACITY_RM,
	POWER_SUPPLY_PROP_CAPACITY_FCC,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_ID_VOLTAGE,
	POWER_SUPPLY_PROP_BRAND,
	POWER_SUPPLY_PROP_FCP_STATUS,
	POWER_SUPPLY_PROP_SCP_STATUS,
	POWER_SUPPLY_PROP_BAT_OVP,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_CAPACITY_DEC,
	POWER_SUPPLY_PROP_BAT_QMAX,
};

static enum power_supply_property hisi_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CURRENT_MAX,
};

static enum power_supply_property hisi_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,
};

static enum power_supply_property hisi_wireless_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property hisi_bk_bci_battery_props[] = {
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static int calc_capacity_from_voltage(void)
{
	int data = 50;
	int battery_voltage = 0;
	int battery_current = 0;

	battery_current = -hisi_battery_current();
	battery_voltage = hisi_battery_voltage();
	if (battery_voltage <= BAT_VOL_3200) {
		data = 0;
		return data;
	}
	battery_voltage = hisi_battery_voltage() - 120 * battery_current / 1000;
	if (battery_voltage < BAT_VOL_3500)
		data = 2;
	else if (battery_voltage < BAT_VOL_3550)
		data = 10;
	else if (battery_voltage < BAT_VOL_3600)
		data = 20;
	else if (battery_voltage < BAT_VOL_3700)
		data = 30;
	else if (battery_voltage < BAT_VOL_3800)
		data = 40;
	else if (battery_voltage < BAT_VOL_3850)
		data = 50;
	else if (battery_voltage < BAT_VOL_3900)
		data = 60;
	else if (battery_voltage < BAT_VOL_3950)
		data = 65;
	else if (battery_voltage < BAT_VOL_4000)
		data = 70;
	else if (battery_voltage < BAT_VOL_4250)
		data = 85;
	else if (battery_voltage >= BAT_VOL_4250)
		data = 100;
	return data;
}

/*only charge-work can not reach full(95%).Change capacity to full after 40min.*/
static int hisi_force_full_timer(int curr_capacity,
				 struct hisi_bci_device_info *di)
{
	if (curr_capacity > CHG_CANT_FULL_THRESHOLD) {
		di->charge_full_count++;
		if (di->charge_full_count >= CHARGE_FULL_TIME) {
			bci_info("FORCE_CHARGE_FULL = %d\n", curr_capacity);
			di->charge_full_count = CHARGE_FULL_TIME;
			curr_capacity = CAPACITY_FULL;
		}
	} else {
		di->charge_full_count = 0;
	}

	return curr_capacity;
}

static int hisi_capacity_pulling_filter(int curr_capacity,
					struct hisi_bci_device_info *di)
{
	int index = 0;
	di->bat_exist = is_hisi_battery_exist();

	if ((!di->bat_exist) || (is_fake_battery)) {
		curr_capacity = calc_capacity_from_voltage();
		return curr_capacity;
	}
	index = di->capacity_filter_count % WINDOW_LEN;

	capacity_sum -= capacity_filter[index];
	capacity_filter[index] = curr_capacity;
	capacity_sum += capacity_filter[index];

	if (++di->capacity_filter_count >= WINDOW_LEN)
		di->capacity_filter_count = 0;

	/*rounding-off 0.5 method */
	curr_capacity = (capacity_sum) / WINDOW_LEN;

	return curr_capacity;
}

static int hisi_capacity_dec_pulling_filter(int curr_capacity,
					struct hisi_bci_device_info *di)
{
	static int index = 0;

	index %= CAPACITY_DEC_WINDOW_LEN;

	capacity_dec_sum -= capacity_dec_filter[index];
	capacity_dec_filter[index] = curr_capacity;
	capacity_dec_sum += capacity_dec_filter[index];

	if (++index >= CAPACITY_DEC_WINDOW_LEN)
		index = 0;

	curr_capacity = (capacity_dec_sum) / CAPACITY_DEC_WINDOW_LEN;
	return curr_capacity;
}

/* exit scp (example : 87.5% to 87%) */
static void hisi_reset_capacity_fifo(int curr_capacity)
{
	unsigned int i = 0;

	capacity_sum = 0;

	for (i = 0; i < WINDOW_LEN; i++) {
		capacity_filter[i] = curr_capacity;
		capacity_sum += capacity_filter[i];
	}
}

#ifdef CONFIG_DIRECT_CHARGER
/* enter scp (example : 87% to 87.5%) */
static void hisi_reset_capacity_dec_fifo(struct hisi_bci_device_info *di)
{
	int curr_capacity = 0;
	unsigned int i = 0;

	curr_capacity = (capacity_sum * di->capacity_dec_base_decimal) / WINDOW_LEN;

	capacity_dec_sum = 0;

	for (i = 0; i < CAPACITY_DEC_WINDOW_LEN; i++) {
		capacity_dec_filter[i] = curr_capacity;
		capacity_dec_sum += capacity_dec_filter[i];
	}

	bci_info("%s capacity_filter=%d\n", __func__, capacity_dec_filter[0]);

	capacity_dec_init_value = curr_capacity;
	capacity_dec_cnt = 0;
}
#endif

void bci_set_work_interval(int capacity, struct hisi_bci_device_info *di)
{
	int i;

	for (i = 0; i < WORK_INTERVAL_PARA_LEVEL; i++) {
		if ((capacity >= di->interval_data[i].cap_min)
			&& (capacity <= di->interval_data[i].cap_max)) {
			di->monitoring_interval = di->interval_data[i].work_interval;
			break;
		}
	}

	if(!di->monitoring_interval)
		di->monitoring_interval = WORK_INTERVAL_NOARMAL;

    if (hisi_coul_low_temp_opt() && (hisi_battery_temperature() < 5) && (di->monitoring_interval > (WORK_INTERVAL_NOARMAL/2)))
        di->monitoring_interval = WORK_INTERVAL_NOARMAL/2;
#ifdef CONFIG_DIRECT_CHARGER
    if (get_super_charge_flag() && di->ac_online && (di->monitoring_interval > (WORK_INTERVAL_NOARMAL/2)))
        di->monitoring_interval = WORK_INTERVAL_NOARMAL/2;
#endif
	if (capacity > CHG_CANT_FULL_THRESHOLD)
		di->monitoring_interval = WORK_INTERVAL_REACH_FULL;
}
/*******************************************************
  Function:       vth_correct_soc
  Description:    correct soc according to voltage threshhold
  Input:
                  int curr_capacity ---- soc now
                  struct hisi_bci_device_info di ---- device info
  Output:         NULL
  Return:         soc after adjusted
********************************************************/
static int vth_correct_soc(int curr_capacity, struct hisi_bci_device_info *di)
{
    int battery_volt;
    int i;

    if (0 == di->vth_correct_en)
        return curr_capacity;
    if((POWER_SUPPLY_STATUS_DISCHARGING == di->charge_status) ||
	    (POWER_SUPPLY_STATUS_NOT_CHARGING == di->charge_status)) {
        for (i = 0; i < CAP_LOCK_PARA_LEVEL; i++) {
            if (curr_capacity < di->vth_correct_data[i].cap) {
				battery_volt = hisi_battery_voltage();
                if (battery_volt >= di->vth_correct_data[i].level_vol) {
                    bci_info("low capacity reported, battery_vol = %d mv,capacity = %d, lock_cap:%d\n", battery_volt, curr_capacity,  di->vth_correct_data[i].cap);
                    return di->vth_correct_data[i].cap;
                }
            }
        }
    }
    return curr_capacity;
}
static int capacity_changed(struct hisi_bci_device_info *di)
{
	int curr_capacity = 0;
        int low_temp_capacity_record = 0;
        int cap = 0;
	int low_bat_flag = is_hisi_battery_reach_threshold();

	di->bat_exist = is_hisi_battery_exist();

	/* if battery is not present we assume it is on battery simulator
	 *  if we are in factory mode, BAT FW is not updated yet, we use volt2Capacity
	 */
	if ((!di->bat_exist) || (is_fake_battery) ||
	    (strstr(saved_command_line, "androidboot.swtype=factory") && (COUL_BQ27510 == hisi_coulometer_type()))) {
		curr_capacity = calc_capacity_from_voltage();
	} else {
	        cap = hisi_battery_capacity();
		if (POWER_SUPPLY_STATUS_CHARGING == di->charge_status) {
			curr_capacity = DIV_ROUND_CLOSEST(cap*100, di->bci_soc_at_term);
		} else {
			curr_capacity = cap;
		}
		if (curr_capacity > CAPACITY_FULL ) {
			curr_capacity = CAPACITY_FULL;
		}
	}
	if ((!di->bat_exist) && strstr(saved_command_line, "androidboot.swtype=factory")) {
		/* when in facotry mode and battery is not exist ,
			keep capacity > 2 to prevent system shutdown */
		if (curr_capacity <= 2) {
			di->capacity = 3;
			di->prev_capacity = 3;
			return 1;
		}
	}

	if ((low_bat_flag & BQ_FLAG_LOCK) != BQ_FLAG_LOCK && is_low_power_locked) {
		wake_unlock(&low_power_lock);/*lint !e455*/
		is_low_power_locked = 0;
	}

	/* Debouncing of power on init. */
	if (di->capacity == -1) {/*lint !e650*/
		di->capacity = curr_capacity;
		di->prev_capacity = curr_capacity;
		return 1;
	}

	/*Only availability if the capacity changed */
	if (curr_capacity != di->prev_capacity) {
		if (abs(di->prev_capacity - curr_capacity) >= CHG_ODD_CAPACITY) {
			bci_info("prev_capacity = %d\n"
				   "curr_capacity = %d\n"
				   "curr_voltage = %d\n", di->prev_capacity,
				   curr_capacity, hisi_battery_voltage());
		}
	}

	if (curr_capacity < 2) {
		int battery_volt;

		battery_volt = hisi_battery_voltage();
		if (battery_volt < BAT_VOL_3500) {
			di->capacity = curr_capacity;
			di->prev_capacity = curr_capacity;
			return 1;
		}
		bci_info("low capacity reported, battery_vol = %d mv, capacity = %d\n", battery_volt, curr_capacity);
		return 0;
	}

	switch (di->charge_status) {
	case POWER_SUPPLY_STATUS_CHARGING:
		curr_capacity = hisi_force_full_timer(curr_capacity, di);
		break;

	case POWER_SUPPLY_STATUS_FULL:
		if (hisi_battery_current_avg() >= 0) {
			if (hisi_battery_voltage() >= (di->bat_max_volt - RECHG_PROTECT_THRESHOLD)) {
				curr_capacity = CAPACITY_FULL;
				bci_info("Force soc=100\n");
			}
		}
		di->charge_full_count = 0;
		break;

	case POWER_SUPPLY_STATUS_DISCHARGING:
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		/*capacity-count will always decrease when discharging || notcharging */
		if (di->prev_capacity <= curr_capacity)/*lint !e574*/
			return 0;
		di->charge_full_count = 0;
		break;

	default:
		bci_err("%s defualt run.\n", __func__);
		break;
	}

	bci_set_work_interval(curr_capacity, di);
    curr_capacity = vth_correct_soc(curr_capacity, di);
    low_temp_capacity_record = curr_capacity;

	/*filter */
	curr_capacity = hisi_capacity_pulling_filter(curr_capacity, di);


    if (hisi_coul_low_temp_opt() && (hisi_battery_temperature() < (-5)) && (curr_capacity - low_temp_capacity_record > 1)){
        bci_info("low_temp_opt:filter_curr_capacity = %d, low_temp_capacity_record= %d\n", curr_capacity,low_temp_capacity_record);
        curr_capacity -= 1;
        bci_info("low_temp_opt: low_temp_capacity = %d\n", curr_capacity);
    }

	if (di->prev_capacity == curr_capacity)
		return 0;

	if((POWER_SUPPLY_STATUS_DISCHARGING == di->charge_status) ||
	    (POWER_SUPPLY_STATUS_NOT_CHARGING == di->charge_status)) {
	    if (di->prev_capacity < curr_capacity)/*lint !e574*/
	        return 0;
	}

	if(POWER_SUPPLY_STATUS_CHARGING == di->charge_status
		&& (-hisi_battery_current() >= CURRENT_THRESHOLD))
	{
		if(di->prev_capacity > curr_capacity)/*lint !e574*/
			return 0;
	}

	bci_info("Capacity Updated = %d, charge_full_count = %d, charge_status = %d\n",
							curr_capacity, di->charge_full_count, di->charge_status);

	di->capacity = curr_capacity;
	di->prev_capacity = curr_capacity;
	return 1;
}

static int get_capacity_decimal(struct hisi_bci_device_info *di)
{
	int curr_capacity = 0;
	int rep_capacity = 0;

	static int capacity_prev = 0;
	int capacity_round = 0;
	int capacity_remain = 0;
	int capacity_new = 0;

	/* first: get decimal capacity (example: 875) */
	if ((int)capacity_dec_cnt < di->capacity_dec_sample_nums) {
		if ((int)capacity_dec_init_value < 80 * di->capacity_dec_base_decimal) /* soc=80% */
			curr_capacity = capacity_dec_init_value + (capacity_dec_cnt++ / 3) + 3;
		else if (((int)capacity_dec_init_value >= 80 * di->capacity_dec_base_decimal) && \
			((int)capacity_dec_init_value < 90 * di->capacity_dec_base_decimal))
			curr_capacity = capacity_dec_init_value + (capacity_dec_cnt++ / 4);
		else
			curr_capacity = capacity_dec_init_value + (capacity_dec_cnt++ / 5);
	}
	else {
		return (di->capacity * di->capacity_dec_base_decimal);
	}

	/* second : filter (example: 87.5%) */
	rep_capacity = hisi_capacity_dec_pulling_filter(curr_capacity, di);
	capacity_round = (rep_capacity / di->capacity_dec_base_decimal);
	capacity_remain =  (rep_capacity % di->capacity_dec_base_decimal);

	bci_info("%s times=%d curr_cap_dec=%d rep_cap_dec=%d prev_cap=%d rep_cap=%d\n", __func__ ,\
		capacity_dec_cnt, curr_capacity, rep_capacity, capacity_prev, capacity_round);

	/* third: reset capcity fifo */
	hisi_reset_capacity_fifo(capacity_round);
	capacity_new = (rep_capacity * WINDOW_LEN / di->capacity_dec_base_decimal) - capacity_round * (WINDOW_LEN - 1);
	hisi_capacity_pulling_filter(capacity_new, di);

	/* forth: capacity changed (example: 86% to 87%) */
	if (capacity_prev != capacity_round) {
		di->capacity = capacity_round;
		di->prev_capacity = capacity_round;

		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
		power_supply_changed(&di->bat);
		#else
		power_supply_changed(di->bat);
		#endif
	}
	capacity_prev = capacity_round;

	return rep_capacity;
}

#ifdef CONFIG_DIRECT_CHARGER
static int get_capacity_dec(struct hisi_bci_device_info *di)
{
	if( NULL == di) {
		bci_info("NULL point in [%s]\n", __func__);
		return -1;
	}
	else {
		return di->capacity_dec;
	}
}
#endif

static void capacity_dec_timer_start(struct hisi_bci_device_info *di)
{
	int interval = 0;

	if( NULL == di) {
		bci_info("NULL point in [%s]\n", __func__);
		return;
	}

	interval = di->capacity_dec_timer_interval;
	hrtimer_start(&di->capacity_dec_timer, \
			ktime_set(interval/MSEC_PER_SEC, (interval % MSEC_PER_SEC) * USEC_PER_SEC), \
			HRTIMER_MODE_REL);
}

static enum hrtimer_restart capacity_dec_timer_func(struct hrtimer *timer)
{
	struct hisi_bci_device_info *di;

	di = container_of(timer, struct hisi_bci_device_info, capacity_dec_timer);

	if (NULL == di) {
		bci_info("NULL point in [%s]\n", __func__);
		return HRTIMER_NORESTART;
	}

	if (di->ac_online == 0) {
		return HRTIMER_NORESTART;
	}

	if ((int)capacity_dec_cnt >= di->capacity_dec_sample_nums) {
		return HRTIMER_NORESTART;
	}

	di->capacity_dec = get_capacity_decimal(di);

	capacity_dec_timer_start(di);

	bci_info("%s,%d cnt=%d capacity_dec=%d\n", __func__, __LINE__, capacity_dec_cnt, di->capacity_dec);

	return HRTIMER_NORESTART;
}
static void update_charging_status(struct hisi_bci_device_info *di, unsigned long event)
{
	if ((di->usb_online || di->ac_online) && di->capacity == CAPACITY_FULL &&
		 di->charge_status != POWER_SUPPLY_STATUS_NOT_CHARGING )
		di->charge_status = POWER_SUPPLY_STATUS_FULL;
	/*in case charger can not get the report of charger removed, so
	 * update the status of charger.*/
#ifdef CONFIG_DIRECT_CHARGER
	if (0 == get_direct_charge_flag()) {
#endif

#ifdef CONFIG_TCPC_CLASS
		if (!pd_dpm_get_pd_finish_flag()) {
#endif
#ifdef CONFIG_WIRELESS_CHARGER
			if (WIRELESS_CHANNEL_OFF == wireless_charge_get_wireless_channel_state()) {
#endif

#ifdef CONFIG_HUAWEI_CHARGER_SENSORHUB
				if (hisi_get_charger_type() == CHARGER_TYPE_NONE) {
					di->usb_online = 0;
					di->ac_online = 0;
					di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;
					di->power_supply_status = POWER_SUPPLY_HEALTH_UNKNOWN;
					di->charge_full_count = 0;
				}
#endif

#ifdef CONFIG_WIRELESS_CHARGER
			}
#endif
#ifdef CONFIG_TCPC_CLASS
		}
#endif
#ifdef CONFIG_DIRECT_CHARGER
	}
#endif
	if (event == VCHRG_CHARGE_DONE_EVENT)
		di->chargedone_stat = 1;
	else
		di->chargedone_stat = 0;
}
static int hisi_charger_event(struct notifier_block *nb, unsigned long event,
			      void *priv_data)
{
	struct hisi_bci_device_info *di;
	int ret = 0;
	di = container_of(nb, struct hisi_bci_device_info, nb);
	di->event = event;
	switch (event) {
	case VCHRG_START_USB_CHARGING_EVENT:
		di->usb_online = 1;
		di->ac_online = 0;
		di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
		di->power_supply_status = POWER_SUPPLY_HEALTH_GOOD;
		if (g_sysfs_bq_bci)
			sysfs_notify(g_sysfs_bq_bci, NULL, "poll_charge_start_event");
		break;

	case VCHRG_START_AC_CHARGING_EVENT:
		di->ac_online = 1;
		di->usb_online = 0;
		di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
		di->power_supply_status = POWER_SUPPLY_HEALTH_GOOD;
		if (g_sysfs_bq_bci)
			sysfs_notify(g_sysfs_bq_bci, NULL, "poll_charge_start_event");
		break;

	case VCHRG_STOP_CHARGING_EVENT:
		di->usb_online = 0;
		di->ac_online = 0;
		di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;
		di->power_supply_status = POWER_SUPPLY_HEALTH_UNKNOWN;
		di->charge_full_count = 0;
		if (g_sysfs_bq_bci)
			sysfs_notify(g_sysfs_bq_bci, NULL, "poll_charge_start_event");
		break;

	case VCHRG_START_CHARGING_EVENT:
		di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
		di->power_supply_status = POWER_SUPPLY_HEALTH_GOOD;
		break;

	case VCHRG_NOT_CHARGING_EVENT:
		di->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		di->power_supply_status = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case VCHRG_CHARGE_DONE_EVENT:
   /****Do not need charge status change to full when bq24192 chargedone.
    because bq27510 will insure the charge status to full when capacity is 100.
	di->charge_status = POWER_SUPPLY_STATUS_FULL;
	di->power_supply_status = POWER_SUPPLY_HEALTH_GOOD;
    *****************************************************************/
		break;
	case VCHRG_POWER_SUPPLY_OVERVOLTAGE:
		di->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		di->power_supply_status = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		break;

	case VCHRG_POWER_SUPPLY_WEAKSOURCE:
		di->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		di->power_supply_status = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		break;
	case VCHRG_STATE_WATER_INTRUSED:
		break;
	case BATTERY_LOW_WARNING:
		break;
	case BATTERY_LOW_SHUTDOWN:
		wake_lock(&low_power_lock);
		is_low_power_locked = 1;
		mod_delayed_work(system_wq, &di->hisi_bci_monitor_work, msecs_to_jiffies(0));
		break;/*lint !e456*/
	case VCHRG_STATE_WDT_TIMEOUT:
		di->watchdog_timer_status = POWER_SUPPLY_HEALTH_WATCHDOG_TIMER_EXPIRE;
		break;/*lint !e456*/
	case BATTERY_MOVE:
		di->bat_exist = is_hisi_battery_exist();
		break;/*lint !e456*/
	case WIRELESS_TX_STATUS_CHANGED:
		break;/*lint !e456*/
	default:
		bci_err("%s defualt run.\n", __func__);
		break;/*lint !e456*/
	}

	update_charging_status(di, event);

	if (VCHRG_START_CHARGING_EVENT != event)
		bci_info("received event = %lx, charge_status = %d\n", event, di->charge_status);
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
	power_supply_changed(&di->bat);
	#else
	power_supply_changed(di->bat);
	#endif
	return ret;/*lint !e454*/
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wframe-larger-than="
/*for dmd*/
#if defined(CONFIG_HUAWEI_DSM)
int check_batt_not_exist(char *buf)
{
#ifndef CONFIG_HLTHERM_RUNTEST
	if (!is_hisi_battery_exist()) {
		return 1;
	}
#endif
	return 0;
}

int check_batt_temp_overlow(char *buf)
{
	int batt_temp = hisi_battery_temperature();
#ifndef CONFIG_HLTHERM_RUNTEST
	if (batt_temp < BATT_TEMP_OVERLOW_TH) {
		return 1;
	}
#endif
	return 0;
}

int check_batt_volt_overhigh(char *buf)
{
	int volt;
	int check_cnt = 0;
	int avg_volt = 0;

	volt = hisi_battery_voltage();
	if (BATT_VOLT_OVERHIGH_TH < volt) {
		for(check_cnt = 0 ; check_cnt < MAX_CONFIRM_CNT-1; check_cnt++){
			msleep(CONFIRM_INTERVAL);
			volt += hisi_battery_voltage();
		}
		avg_volt = (int)(volt/(MAX_CONFIRM_CNT));
		if(BATT_VOLT_OVERHIGH_TH < avg_volt){
			snprintf(buf, DSM_BATTERY_MAX_SIZE, "avg_batt_volt = %dmV\n", avg_volt);
			return 1;
		}
	}
	return 0;
}

int check_batt_volt_overlow(char *buf)
{
	int volt;
	int avg_volt = 0;
	int check_cnt = 0;

	volt = hisi_battery_voltage();
	if (volt < BATT_VOLT_OVERLOW_TH) {
		for(check_cnt = 0 ; check_cnt < MAX_CONFIRM_CNT-1; check_cnt++){
			msleep(CONFIRM_INTERVAL);
			volt += hisi_battery_voltage();
		}
		avg_volt = (int)(volt/(MAX_CONFIRM_CNT));
		if( BATT_VOLT_OVERLOW_TH > avg_volt){
			snprintf(buf, DSM_BATTERY_MAX_SIZE, "avg_batt_volt = %dmV\n", avg_volt);
			return 1;
		}
	}
	return 0;
}

int check_batt_terminate_too_early(char *buf)
{
	struct hisi_bci_device_info *di = g_hisi_bci_dev;
	int capacity = hisi_battery_capacity();

	if(NULL == di){
		bci_info("NULL point in [%s]\n", __func__);
		return -1;
	}
	if (di->chargedone_stat && capacity <= CHG_CANT_FULL_THRESHOLD) {
		return 1;
	}
	return 0;
}

int check_batt_not_terminate(char *buf)
{
	int curr_by_coultype = 1;
	int current_avg = 0;
	int current_now = 0;
	static int times = 0;
	int dpm_state = 0;
    struct hisi_bci_device_info *di = g_hisi_bci_dev;

	if(NULL == di){
		bci_info("NULL point in [%s]\n", __func__);
		return -1;
	}
	if (COUL_HISI == hisi_coulometer_type())
		curr_by_coultype = -1;

	dpm_state = charge_check_input_dpm_state();
	if ((POWER_SUPPLY_STATUS_FULL == di->charge_status) && (0 == dpm_state)) {
		current_avg = hisi_battery_current_avg();
		current_now = curr_by_coultype * hisi_battery_current();
		if (current_avg <= BATT_NOT_TERMINATE_TH && current_avg >= CURRENT_THRESHOLD
		    && current_now <= BATT_NOT_TERMINATE_TH && current_now >= CURRENT_THRESHOLD) {
			times++;
		} else {
			times = 0;
		}
		if (times == DSM_CHECK_TIMES_LV3) {
			times = 0;
			snprintf(buf, DSM_BATTERY_MAX_SIZE, "current_avg = %dmA, current_now = %dmA, "
				"charge_event = %ld\n", current_avg, current_now, di->event);
			return 1;
		}
	}
	return 0;
}
int check_batt_bad_curr_sensor(char *buf)
{
	static int times = 0;
	int current_now;
	struct hisi_bci_device_info *di = g_hisi_bci_dev;
	int plugged = 0;

	if(NULL == di){
		bci_info("NULL point in [%s]\n", __func__);
		return -1;
	}
	current_now = -hisi_battery_current();
	plugged = charge_check_charger_plugged();

	if ((!di->usb_online) && (!di->ac_online) && (0 == plugged)
	    && (current_now > CURRENT_OFFSET)) {
		times++;
	} else {
		times = 0;
	}
	if (times == DSM_CHECK_TIMES_LV2) {
		times = 0;
		snprintf(buf, DSM_BATTERY_MAX_SIZE, "current = %dmA, charge_status = %d\n",
			current_now, di->charge_status);
		return 1;
	}
	return 0;
}

int check_vbus_volt_overhigh(char *buf)
{
	static int times = 0;
	struct hisi_bci_device_info *di = g_hisi_bci_dev;

	if(NULL == di){
		bci_info("NULL point in [%s]\n", __func__);
		return -1;
	}
	if (di->power_supply_status == POWER_SUPPLY_HEALTH_OVERVOLTAGE)
		times++;
	else
		times = 0;

	if (times == DSM_CHECK_TIMES_LV2) {
		times = 0;
		return 1;
	}
	return 0;
}

int check_watchdog_timer_expiration(char *buf)
{
	struct hisi_bci_device_info *di = g_hisi_bci_dev;
	if(NULL == di){
		bci_info("NULL point in [%s]\n", __func__);
		return -1;
	}
	if (di->watchdog_timer_status == POWER_SUPPLY_HEALTH_WATCHDOG_TIMER_EXPIRE) {
		snprintf(buf, DSM_BATTERY_MAX_SIZE, "WDT_status=%s\n", "watch dog timer expiration");
		return 1;
	}
	return 0;
}

int check_charge_curr_overhigh(char *buf)
{
	int i;
	int current_now[DSM_CHECK_TIMES_LV1];

	for (i = 0; i < DSM_CHECK_TIMES_LV1; i++) {
		current_now[i] = -hisi_battery_current();
		if (current_now[i] > CHARGE_CURRENT_OVERHIGH_TH)
			msleep(100);
		else
			break;

	}
	if (DSM_CHECK_TIMES_LV1 == i) {
		snprintf(buf, DSM_BATTERY_MAX_SIZE, "current = %d, %d, %d\n",
			current_now[0], current_now[1], current_now[2]);
		return 1;
	}
	return 0;
}

int check_discharge_curr_overhigh(char *buf)
{
	int i;
	int current_now[DSM_CHECK_TIMES_LV1];
	struct hisi_bci_device_info *di = g_hisi_bci_dev;

	if(NULL == di){
		bci_info("NULL point in [%s]\n", __func__);
		return -1;
	}
	for (i = 0; i < DSM_CHECK_TIMES_LV1; i++) {
		current_now[i] = -hisi_battery_current();
		if ((!di->usb_online) && (!di->ac_online)
		    && current_now[i] < DISCHARGE_CURRENT_OVERHIGH_TH) {
			msleep(100);
		} else {
			break;
		}
	}

	if (DSM_CHECK_TIMES_LV1 == i) {
		snprintf(buf, DSM_BATTERY_MAX_SIZE, "current = %d, %d, %d\n",
			current_now[0], current_now[1], current_now[2]);
		return 1;
	}
	return 0;
}

/* During continuous suspend and resume in 30 seconds, and suspend current < 10ma test,
*  g_curr_zero_times will not exceeding 1 ,it will be cleared when resume
*/
static int g_curr_zero_times;
int check_charge_curr_zero(char *buf)
{
	int plugged = 0;
	int current_now = 0;

	current_now = -hisi_battery_current();
	plugged = charge_check_charger_plugged();

	if ((current_now < CURRENT_THRESHOLD) && (current_now > (-CURRENT_THRESHOLD)) && (0 == plugged))
		g_curr_zero_times++;
	else
		g_curr_zero_times = 0;

	if (g_curr_zero_times == DSM_CHECK_TIMES_LV2) {
		g_curr_zero_times = 0;
		return 1;
	}
	return 0;
}

#define HIGH_TEMP (50)
#define LOW_TEMP  (0)
int check_charge_temp_fault(char *buf)
{
#ifndef CONFIG_HLTHERM_RUNTEST
	struct hisi_bci_device_info *di = g_hisi_bci_dev;
	int batt_temp = INVALID_TEMP_VAL;

	if(NULL == di){
		bci_info("NULL point in [%s]\n", __func__);
		return 0;//null pointer, return no_err
	}
	batt_temp = hisi_battery_temperature();
	if ((HIGH_TEMP <= batt_temp || LOW_TEMP > batt_temp) && (di->usb_online || di->ac_online)) {
		snprintf(buf, DSM_BATTERY_MAX_SIZE, "temp fault cause not charging\n");
		return 1;
	}
#endif
	return 0;
}

#define WARM_TEMP (45)
int check_charge_warm_status(char *buf)
{
#ifndef CONFIG_HLTHERM_RUNTEST
	struct hisi_bci_device_info *di = g_hisi_bci_dev;
	int temp = hisi_battery_temperature();

	if (temp >= WARM_TEMP && temp < HIGH_TEMP && (di->usb_online || di->ac_online)) {
		snprintf(buf, DSM_BATTERY_MAX_SIZE, "warm temp triggered: temp = %d\n", temp);
		return 1;
	}
#endif
	return 0;
}

#define SHUTDOWN_TEMP (68)
int check_charge_batt_shutdown_temp(char *buf)
{
#ifndef CONFIG_HLTHERM_RUNTEST
	int temp = hisi_battery_temperature();

	if (SHUTDOWN_TEMP <= temp) {
		snprintf(buf, DSM_BATTERY_MAX_SIZE, "shutdown because batt_temp = %d\n", temp);
		return 1;
	}
#endif
	return 0;
}

int check_charge_batt_capacity(char *buf)
{
	struct hisi_bci_device_info *di = g_hisi_bci_dev;
	int full_rm = di->bat_rm;
	int fcc_design = hisi_battery_fcc_design();
	int batt_temp = hisi_battery_temperature();

	if ((VCHRG_CHARGE_DONE_EVENT == di->event)
		&& (full_rm < fcc_design*BATT_CAPACITY_REDUCE_TH/CAPACITY_FULL)
		&& (batt_temp > LOW_TEMP)) {
		snprintf(buf, DSM_BATTERY_MAX_SIZE, "full_rm = %dmAh, fcc_design = %dmAh\n", di->bat_rm, fcc_design);
		return 1;
	}
	return 0;
}

int check_charge_charger_ts(char *buf)
{
	struct hisi_bci_device_info *di = g_hisi_bci_dev;
	if (TRUE == charge_check_charger_ts()) {
		snprintf(buf, DSM_BATTERY_MAX_SIZE, " charger ts error!charge status = %d\n", di->charge_status);
		return 1;
	}
	return 0;
}

int check_charge_otg_ture(char *buf)
{
#ifdef CONFIG_HUAWEI_TYPEC
	int typec_mode = 0;
	if ((TRUE == charge_check_charger_otg_state()) && (PLEASE_PROVIDE_POWER == hisi_get_charger_type())) {
		typec_mode = typec_detect_port_mode();
		if (typec_mode < 0)
		{
			bci_err("typec not ready %d!!!\n", typec_mode);
			return 0;
		}
		if (TYPEC_DEV_PORT_MODE_DFP != typec_mode && TYPEC_DEV_PORT_MODE_NOT_READY != typec_mode) {
			snprintf(buf, DSM_BATTERY_MAX_SIZE, "charger in otg mode!typec mode = %d\n",typec_mode);
			return 1;
		}
	}
#endif
	return 0;
}

/*******************************************************
  Function:         check_soc_vary_err
  Description:     monitor the varity of SOC
  Input:             NULL
  Output:           buf: error message
  Return:           NULL
********************************************************/
#define SOC_MONITOR_INTERVAL (60000) /*1 min*/
int check_soc_vary_err(char *buf)
{
	int report_flag = 0;
#ifndef CONFIG_HLTHERM_RUNTEST
	static int data_invalid = 0;
	static int monitor_cnt = 0;
	int deta_soc = 0;
	struct hisi_bci_device_info *di = g_hisi_bci_dev;

	if( NULL == di )
	{
		bci_info("NULL point in [%s]\n", __func__);
		return -EINVAL;
	}

	data_invalid |= hisi_battery_soc_vary_flag(0, &deta_soc);
	if(monitor_cnt % (SOC_MONITOR_INTERVAL / di->monitoring_interval) == 0){
		data_invalid |= hisi_battery_soc_vary_flag(1, &deta_soc);
		if(data_invalid){
			report_flag = 0;
		}else{
			snprintf(buf, DSM_BATTERY_MAX_SIZE, "soc change fast: %d\n", deta_soc);
			report_flag = 1;
		}
		monitor_cnt = 0;//reset the counter
		data_invalid = 0;
	}
	monitor_cnt++;
#endif
	return report_flag;
}
void batt_info_dump(char* pstr)
{
	char buf[CHARGE_DMDLOG_SIZE];
	char* batt_brand = hisi_battery_brand();
	int batt_id = hisi_battery_id_voltage();
	int batt_cycle = hisi_battery_cycle_count();
	int fcc_design = hisi_battery_fcc_design();
	int fcc = hisi_battery_fcc();
	int rm = hisi_battery_rm();
	int soc = hisi_battery_capacity();

	int vbatt = hisi_battery_voltage();
	int vbus = get_charger_vbus_vol();

	int ibus = get_charger_ibus_curr();
	int ichg =  -hisi_battery_current();
	int curr_now = -hisi_battery_current();
	int avg_curr = hisi_battery_current_avg();

	int tbatt = hisi_battery_temperature();
#ifdef  CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
	int tusb = get_usb_ntc_temp();
#else
	int tusb = INVALID_TEMP_VAL;
#endif

	enum usb_charger_type charger_type = charge_get_charger_type();
	if (!pstr) {
		bci_err("%s: para null\n", __func__);
		return;
	}
	memset_s(buf, CHARGE_DMDLOG_SIZE, 0, CHARGE_DMDLOG_SIZE);

	snprintf(buf, sizeof(buf)-1, "battery: %s, batt_id = %d, batt_cycle = %d, fcc_design = %dmAh, fcc = %dmAh, rm = %dmAh, soc = %d, "
		"batt_volt = %dmV, charger_vbus = %dmV, charger_ibus = %dmA, ichrg = %dmA, curr_now = %dmA, avg_curr = %dmA, "
		"tbatt = %d, tusb = %d, charger_type = %d\n",
		batt_brand, batt_id, batt_cycle, fcc_design, fcc, rm, soc,
		vbatt, vbus, ibus, ichg, curr_now, avg_curr, tbatt, tusb, charger_type);
	strncat(pstr,buf,strlen(buf));
}
struct batt_dsm batt_dsm_array[] = {
	{ERROR_BATT_NOT_EXIST, true, .dump = batt_info_dump, check_batt_not_exist},
	{ERROR_BATT_TEMP_LOW, true, .dump = batt_info_dump, check_batt_temp_overlow},
	{ERROR_BATT_VOLT_HIGH, true, .dump = batt_info_dump, check_batt_volt_overhigh},
	{ERROR_BATT_VOLT_LOW, true, .dump = batt_info_dump, check_batt_volt_overlow},
	{ERROR_BATT_TERMINATE_TOO_EARLY, true, .dump = batt_info_dump, check_batt_terminate_too_early},
	{ERROR_BATT_NOT_TERMINATE, true, .dump = batt_info_dump, check_batt_not_terminate},
	{ERROR_BATT_BAD_CURR_SENSOR, true, .dump = batt_info_dump, check_batt_bad_curr_sensor},
	{ERROR_VBUS_VOLT_HIGH, true, .dump = batt_info_dump, check_vbus_volt_overhigh},
	{ERROR_WATCHDOG_RESET, true, .dump = batt_info_dump, check_watchdog_timer_expiration},
	{ERROR_CHARGE_CURR_OVERHIGH, true, .dump = batt_info_dump, check_charge_curr_overhigh},
	{ERROR_DISCHARGE_CURR_OVERHIGH, true, .dump = batt_info_dump, check_discharge_curr_overhigh},
	{ERROR_CHARGE_CURR_ZERO, true, .dump = batt_info_dump, check_charge_curr_zero},
	{ERROR_CHARGE_TEMP_FAULT, true, .dump = batt_info_dump, check_charge_temp_fault},
	{ERROR_CHARGE_BATT_TEMP_SHUTDOWN, true, .dump = batt_info_dump, check_charge_batt_shutdown_temp},
	{ERROR_CHARGE_BATT_CAPACITY, true, .dump = batt_info_dump, check_charge_batt_capacity},
	{ERROR_CHARGE_CHARGER_TS, true, .dump = batt_info_dump, check_charge_charger_ts},
	{ERROR_CHARGE_OTG, true, .dump = batt_info_dump, check_charge_otg_ture},
	{ERROR_BATT_SOC_CHANGE_FAST, true, .dump = batt_info_dump, check_soc_vary_err},
	{ERROR_CHARGE_TEMP_WARM, true, .dump = batt_info_dump, check_charge_warm_status},
};

static void hisi_get_error_info(struct hisi_bci_device_info *di)
{
	int i;
	char buf[DSM_BATTERY_MAX_SIZE];

	memset(buf, 0, sizeof(buf));
	for (i = 0; i < (sizeof(batt_dsm_array) / sizeof(struct batt_dsm)); ++i) {/*lint !e574*/
		if (batt_dsm_array[i].notify_enable
		    && (BAT_BOARD_ASIC == is_board_type)) {
			if (batt_dsm_array[i].check_error(buf)) {



				di->bat_err = batt_dsm_array[i].error_no;
#if defined(CONFIG_HUAWEI_DSM)
				if (!dsm_client_ocuppy(power_dsm_get_dclient(POWER_DSM_BATTERY))) {
					buf[DSM_BATTERY_MAX_SIZE-1] = 0;
					dev_err(di->dev, "%s", buf);
					if (batt_dsm_array[i].dump)
						batt_dsm_array[i].dump(buf);
					dsm_client_record(power_dsm_get_dclient(POWER_DSM_BATTERY), "%s", buf);
					dsm_client_notify(power_dsm_get_dclient(POWER_DSM_BATTERY), di->bat_err);
					batt_dsm_array[i].notify_enable = false;
					break;
				}
#else
				buf[DSM_BATTERY_MAX_SIZE-1] = 0;
				bci_err("error_no = %d!!!\n", di->bat_err);
				bci_err("error_buf = %s\n", buf);
				break;
#endif
			}
		}
	}
}
#endif
#pragma GCC diagnostic pop

static void hisi_get_battery_info(struct hisi_bci_device_info *di)
{
	di->bat_rm = hisi_battery_rm();
	di->bat_fcc = hisi_battery_fcc();

	if (!(di->bat_exist)) {
		di->bat_health = POWER_SUPPLY_HEALTH_UNKNOWN;
		di->bat_capacity_level = POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
		di->bat_technolog = POWER_SUPPLY_TECHNOLOGY_UNKNOWN;
		di->bat_temperature = 0;
	} else {
		di->bat_health = hisi_battery_health();
		di->bat_capacity_level = hisi_battery_capacity_level();
		di->bat_technolog = hisi_battery_technology();
		di->bat_temperature = hisi_battery_temperature();
	}
}

unsigned int get_bci_soc(void)
{
	/* if bci driver load uncorrectly, full soc will be returned */
	return g_hisi_bci_dev?g_hisi_bci_dev->capacity:CAPACITY_FULL;
}

static void hisi_bci_battery_work(struct work_struct *work)
{
	struct hisi_bci_device_info *di = container_of(work, struct hisi_bci_device_info, hisi_bci_monitor_work.work);

	hisi_get_battery_info(di);
#if defined (CONFIG_HUAWEI_DSM)
	hisi_get_error_info(di);
#endif
	queue_delayed_work(system_power_efficient_wq, &di->hisi_bci_monitor_work, msecs_to_jiffies(di->monitoring_interval));

	if (capacity_changed(di)) {
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
		power_supply_changed(&di->bat);
		#else
		power_supply_changed(di->bat);
		#endif
	}
}

static int hisi_ac_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
    struct hisi_bci_device_info *di = g_hisi_bci_dev;
	if( NULL == di )
	{
		bci_info("NULL point in [%s]\n", __func__);
		return -EINVAL;
    }
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (di->ac_online && CHARGER_TYPE_WIRELESS != charge_get_charger_type())
			val->intval = 1;
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = di->power_supply_status;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int hisi_wireless_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct hisi_bci_device_info *di = g_hisi_bci_dev;
	if ( NULL == di )
	{
		bci_info("NULL point in [%s]\n", __func__);
		return -EINVAL;
	}
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (CHARGER_TYPE_WIRELESS == charge_get_charger_type())
			val->intval = 1;
		else
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}


#define MILLI_TO_MICRO    (1000)
static int hisi_usb_get_property(struct power_supply *psy,
				 enum power_supply_property psp,
				 union power_supply_propval *val)
{
    struct hisi_bci_device_info *di = g_hisi_bci_dev;

	if( NULL == di )
	{
		bci_info("NULL point in [%s]\n", __func__);
		return -EINVAL;
	}
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = di->usb_online;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = di->power_supply_status;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = 0;
#ifdef CONFIG_HUAWEI_CHARGER
		val->intval = get_charger_vbus_vol() * MILLI_TO_MICRO;
#endif
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = 0;
#ifdef CONFIG_HUAWEI_CHARGER
		val->intval = get_charge_current_max() * MILLI_TO_MICRO;
#endif
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int hisi_bk_bci_battery_get_property(struct power_supply *psy,
					    enum power_supply_property psp,
					    union power_supply_propval *val)
{

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		/*Use gpadc channel for measuring bk battery voltage */
		val->intval = 0;	/* hisi_get_gpadc_conversion(di, ADC_VBATMON);*/
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int hisi_bci_battery_get_fcp_status(struct hisi_bci_device_info *di)
{
		int intval = 0;

		if (FCP_STAGE_SUCESS <= fcp_get_stage_status() && di->ac_online)
			intval = 1;
#ifdef CONFIG_DIRECT_CHARGER
		else if (get_quick_charge_flag() && di->ac_online)
			intval = 1;
#endif
#ifdef CONFIG_TCPC_CLASS
		else if (true == pd_dpm_get_optional_max_power_status() && di->ac_online)
		{
			intval = 1;
			bci_info("pd_dpm_get_optional_max_power_status intval 1 [%s]\n", __func__);
		}
#endif
#ifdef CONFIG_WIRELESS_CHARGER
		else if (wireless_charge_get_fast_charge_flag() && di->ac_online) {
		intval = 1;
		bci_info("%s wireless fast charging\n", __func__);
		}
#endif
		else
		{
			intval = 0;
			bci_info("intval 0 [%s]\n", __func__);
		}
		return intval;
}

static int hisi_get_battery_current(void)
{
    int bat_current = 0;

    if (strstr(saved_command_line, "androidboot.swtype=factory"))
        bat_current = -hisi_battery_current();
    else
        bat_current = -hisi_coul_fifo_avg_current();

    return bat_current;
}

static int hisi_bci_battery_get_property(struct power_supply *psy,
					 enum power_supply_property psp,
					 union power_supply_propval *val)
{
    struct hisi_bci_device_info *di = g_hisi_bci_dev;
#ifdef CONFIG_DIRECT_CHARGER
    static int scp_status = 0;
#endif

	if( NULL == di )
	{
		bci_info("NULL point in [%s]\n", __func__);
		return -EINVAL;
	}
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = di->charge_status;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		di->bat_voltage = hisi_battery_voltage();
		if (COUL_HISI == hisi_coulometer_type())
			val->intval = hisi_battery_voltage_uv();
		else
			val->intval = di->bat_voltage * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		di->bat_current = hisi_get_battery_current();
		val->intval = di->bat_current;
		break;
	case POWER_SUPPLY_PROP_TEMP:
#ifdef CONFIG_HLTHERM_RUNTEST
		val->intval = 250;
#else
		val->intval = di->bat_temperature * 10;
#endif
		break;
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = di->bat_exist;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = di->bat_health;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = di->capacity;
		break;
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		val->intval = di->bat_capacity_level;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = di->bat_technolog;
		break;
	case POWER_SUPPLY_PROP_CAPACITY_RM:
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = di->bat_rm;
		break;
	case POWER_SUPPLY_PROP_CAPACITY_FCC:
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		val->intval = di->bat_fcc;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = di->bat_design_fcc;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = di->bat_max_volt;
		break;
	case POWER_SUPPLY_PROP_ID_VOLTAGE:
		val->intval = hisi_battery_id_voltage();
		break;
	case POWER_SUPPLY_PROP_BRAND:
		val->strval = hisi_battery_brand();
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		val->intval = hisi_battery_cycle_count();
		break;
	case POWER_SUPPLY_PROP_LIMIT_FCC:
		val->intval = hisi_battery_get_limit_fcc();
		break;
	case POWER_SUPPLY_PROP_FCP_STATUS:
		val->intval = 0;
		#if (defined (CONFIG_HUAWEI_CHARGER) || defined (CONFIG_HISI_CHARGER_ARCH))
		val->intval = hisi_bci_battery_get_fcp_status(di);
		#endif
		break;
	case POWER_SUPPLY_PROP_SCP_STATUS:
		val->intval = 0;
#ifdef CONFIG_DIRECT_CHARGER
		if (get_super_charge_flag() && di->ac_online)
			val->intval = 1;
		else
			val->intval = 0;

		scp_status = val->intval;
#endif
		break;
	case POWER_SUPPLY_PROP_BAT_OVP:
		val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		val->intval = hisi_battery_cc();
		val->intval = abs(val->intval) + 1;
		break;
	case POWER_SUPPLY_PROP_CAPACITY_DEC:
		val->intval = (di->capacity * di->capacity_dec_base_decimal);
#ifdef CONFIG_DIRECT_CHARGER
		if ((1 == scp_status) && (1 == capacity_dec_start_event_flag)) {
			val->intval = get_capacity_dec(di);
		}
#endif
		break;
    case POWER_SUPPLY_PROP_BAT_QMAX:
        val->intval = hisi_battery_get_qmax();
        break;
	default:
		return -EINVAL;
	}
	return 0;
}

int hisi_bci_show_capacity(void)
{
	struct hisi_bci_device_info *di = g_hisi_bci_dev;

	if( NULL == di )
	{
		bci_info("NULL point in [%s]\n", __func__);
		return -EINVAL;
	}

	if(di->capacity > CAPACITY_FULL) {
		bci_err("error capacity, will rewrite from %d to 100\n", di->capacity);
		di->capacity = CAPACITY_FULL;
	}
	return di->capacity;
}

int hisi_register_notifier(struct notifier_block *nb, unsigned int events)
{
	return blocking_notifier_chain_register(&notifier_list, nb);
}

EXPORT_SYMBOL_GPL(hisi_register_notifier);

int hisi_unregister_notifier(struct notifier_block *nb, unsigned int events)
{
	return blocking_notifier_chain_unregister(&notifier_list, nb);
}

EXPORT_SYMBOL_GPL(hisi_unregister_notifier);

static ssize_t hisi_bci_show_batt_removable(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{	// cppcheck-suppress *
	return sprintf(buf, "%d\n", removable_batt_flag);/*lint !e421*/
}

static ssize_t hisi_bci_poll_charge_start_event(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{

	struct hisi_bci_device_info *di = dev_get_drvdata(dev);

	if (di)
		// cppcheck-suppress *
		return sprintf(buf, "%ld\n", di->event);/*lint !e421*/
	else
		return 0;
}

static ssize_t hisi_bci_set_charge_event(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct hisi_bci_device_info *di = dev_get_drvdata(dev);
	long val = 0;

	if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 3000))
		return -EINVAL;
	di->event = val;
	sysfs_notify(g_sysfs_bq_bci, NULL, "poll_charge_start_event");
	return count;
}

static ssize_t hisi_bci_get_capacity_dec_start_event(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	return sprintf(buf, "%d\n", capacity_dec_start_event_flag);/*lint !e421*/
}

static ssize_t hisi_bci_set_capacity_dec_start_event(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
#ifdef CONFIG_DIRECT_CHARGER
	long val = 0;
#endif
	struct hisi_bci_device_info *di = g_hisi_bci_dev;
	int cap_dec_index = 0;

	if (NULL == di) {
		bci_info("NULL point in [%s]\n", __func__);
		return -EINVAL;
	}

#ifdef CONFIG_DIRECT_CHARGER
	if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1)) {
		capacity_dec_start_event_flag = 0;
		return -EINVAL;
	}

	if ((1 == val) && (1 == get_super_charge_flag())) {
		capacity_dec_start_event_flag = 1;
		if (di->capacity_dec_total_level > 1 &&
			direct_charge_get_adaptor_mode() == SC_MODE) {
			cap_dec_index = 1;  //sc charge
		}
		di->capacity_dec_base_decimal = di->cap_dec_para[cap_dec_index].cap_dec_base_decimal;
		di->capacity_dec_sample_nums = di->cap_dec_para[cap_dec_index].cap_dec_sample_nums;
		di->capacity_dec_timer_interval = di->cap_dec_para[cap_dec_index].cap_dec_timer_interval;
		hisi_reset_capacity_dec_fifo(di);

		di->capacity_dec = capacity_dec_init_value;

		capacity_dec_timer_start(di);
	} else {
		capacity_dec_start_event_flag = 0;
	}
#endif

	return count;
}

static DEVICE_ATTR(batt_removable, (S_IWUSR | S_IRUGO),
		   hisi_bci_show_batt_removable, NULL);

static DEVICE_ATTR(poll_charge_start_event, (S_IWUSR | S_IRUGO),
		   hisi_bci_poll_charge_start_event, hisi_bci_set_charge_event);

static DEVICE_ATTR(capacity_dec_start_event, (S_IWUSR | S_IRUGO),
		   hisi_bci_get_capacity_dec_start_event, hisi_bci_set_capacity_dec_start_event);

static struct attribute *hisi_bci_attributes[] = {
	&dev_attr_batt_removable.attr,
	&dev_attr_poll_charge_start_event.attr,
	&dev_attr_capacity_dec_start_event.attr,
	NULL,
};

static const struct attribute_group hisi_bci_attr_group = {
	.attrs = hisi_bci_attributes,
};

static char *hisi_bci_supplied_to[] = {
	"hisi_bci_battery",
};

struct class *hw_power_get_class(void);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
static const struct power_supply_config hisi_bci_battery_cfg = {
	.supplied_to = hisi_bci_supplied_to,
	.num_supplicants = ARRAY_SIZE(hisi_bci_supplied_to),
};

static const struct power_supply_desc hisi_bci_battery_desc = {
	.name			= "Battery",
	.type			= POWER_SUPPLY_TYPE_BATTERY,
	.properties		= hisi_bci_battery_props,
	.num_properties		= ARRAY_SIZE(hisi_bci_battery_props),
	.get_property		= hisi_bci_battery_get_property,
};

static const struct power_supply_desc hisi_bci_usb_desc = {
	.name			= "USB",
	.type			= POWER_SUPPLY_TYPE_USB,
	.properties		= hisi_usb_props,
	.num_properties		= ARRAY_SIZE(hisi_usb_props),
	.get_property		= hisi_usb_get_property,
};

static const struct power_supply_desc hisi_bci_mains_desc = {
	.name			= "Mains",
	.type			= POWER_SUPPLY_TYPE_MAINS,
	.properties		= hisi_ac_props,
	.num_properties		= ARRAY_SIZE(hisi_ac_props),
	.get_property		= hisi_ac_get_property,
};

static const struct power_supply_desc hisi_bci_wireless_desc = {
	.name			= "Wireless",
	.type			= POWER_SUPPLY_TYPE_WIRELESS,
	.properties		= hisi_wireless_props,
	.num_properties		= ARRAY_SIZE(hisi_wireless_props),
	.get_property		= hisi_wireless_get_property,
};

static const struct power_supply_desc hisi_bci_bk_battery_desc = {
	.name			= "hisi_bk_battery",
	.type			= POWER_SUPPLY_TYPE_UPS,
	.properties		= hisi_bk_bci_battery_props,
	.num_properties		= ARRAY_SIZE(hisi_bk_bci_battery_props),
	.get_property		= hisi_bk_bci_battery_get_property,
};

static const struct power_supply_desc hisi_bci_bat_google_desc = {
	.name			= "battery",
	.type			= POWER_SUPPLY_TYPE_UNKNOWN,
	.properties		= hisi_bci_battery_props,
	.num_properties		= ARRAY_SIZE(hisi_bci_battery_props),
	.get_property		= hisi_bci_battery_get_property,
};
#endif
/**********************************************************
*  Function:       parse_cap_dec_dts
*  Discription:    parse the module dts config value of capacity decimal
*  Parameters:   np:device_node
*                      di:hisi_bci_device_info
*  return value:  null
**********************************************************/
static void parse_cap_dec_dts(struct device_node *np, struct hisi_bci_device_info *di)
{
	int ret = 0;
	int i = 0;
	int array_len = 0;
	u32 cap_dec_tmp[CAP_DEC_PARA_TOTAL*CAPATICY_DEC_PARA_LEVEL] = {0};

	array_len = of_property_count_u32_elems(np, "capacity_dec_para");
	if ((array_len <= 0) ||(array_len % CAP_DEC_PARA_TOTAL != 0)) {
		bci_err("capacity_dec_para is invaild, please check capacity_dec_para number!!\n");
		goto FuncEnd;
	} else if (array_len > CAPATICY_DEC_PARA_LEVEL * CAP_DEC_PARA_TOTAL) {
		bci_err("capacity_dec_para is too long, please check capacity_dec_para number!!\n");
		goto FuncEnd;
	} else {
		ret = of_property_read_u32_array(np, "capacity_dec_para", cap_dec_tmp, array_len);
		if (ret) {
			bci_err("dts:get capacity_dec_para fail!\n");
			goto FuncEnd;
		} else {
			di->capacity_dec_total_level = array_len/CAP_DEC_PARA_TOTAL;
			for (i = 0; i < di->capacity_dec_total_level; i++) {
				di->cap_dec_para[i].cap_dec_base_decimal =
					cap_dec_tmp[(int)(CAP_DEC_BASE_DECIMAL+CAP_DEC_PARA_TOTAL*i)];
				di->cap_dec_para[i].cap_dec_sample_nums =
					cap_dec_tmp[(int)(CAP_DEC_SAMPLE_NUMS+CAP_DEC_PARA_TOTAL*i)];
				di->cap_dec_para[i].cap_dec_timer_interval =
					cap_dec_tmp[(int)(CAP_DEC_TIMER_INTERVAL+CAP_DEC_PARA_TOTAL*i)];
				bci_info("cap_dec_para[%d], base_decimal: %-3d sample_nums: %-3d timer_interval: %-3d\n",
						i, di->cap_dec_para[i].cap_dec_base_decimal,
						di->cap_dec_para[i].cap_dec_sample_nums, di->cap_dec_para[i].cap_dec_timer_interval);
			}
			return;
		}
	}

FuncEnd:
	di->capacity_dec_total_level = 1;
	di->cap_dec_para[0].cap_dec_base_decimal = BASE_DECIMAL;
	di->cap_dec_para[0].cap_dec_sample_nums = CAPACITY_DEC_SAMPLE_NUMS;
	di->cap_dec_para[0].cap_dec_timer_interval = CAPATICY_DEC_TIMER_INTERVAL;
	bci_info("cap_dec_para[0], base_decimal: %-3d sample_nums: %-3d timer_interval: %-3d\n",
			di->cap_dec_para[0].cap_dec_base_decimal,
			di->cap_dec_para[0].cap_dec_sample_nums, di->cap_dec_para[0].cap_dec_timer_interval);
}
/**********************************************************
*  Function:       parse_cap_lock_dts
*  Discription:    parse the module dts config value
*  Parameters:   np:device_node
*                      di:hisi_bci_device_info
*  return value:  0-sucess or others-fail
**********************************************************/
static int parse_vth_correct_dts(struct device_node *np, struct hisi_bci_device_info *di)
{
	int i = 0;

	if (of_property_read_u32(np, "vth_correct_en", (u32 *)&di->vth_correct_en)) {
		di->vth_correct_en = 0;
		bci_err("error:get vth_correct_en value failed, used default value!\n");
	}

    if (of_property_read_u32_array(np, "vth_correct_para",
            (u32 *)&di->vth_correct_data[0],
            sizeof(di->vth_correct_data) / sizeof(int))) {
        di->vth_correct_en = 0;
		bci_err("get vth_correct_para fail!!\n");
		return -EINVAL;
	}

    for (i = 0; i < CAP_LOCK_PARA_LEVEL; i++) {
        bci_info("di->vth_correct_data[%d]:%d,%d\n", i,
            di->vth_correct_data[i].cap, di->vth_correct_data[i].level_vol);
    }
    return 0;

}

/**********************************************************
*  Function:       hisi_bci_parse_dts
*  Discription:    parse the module dts config value
*  Parameters:   np:device_node
*                      di:hisi_bci_device_info
*  return value:  0-sucess or others-fail
**********************************************************/
static int hisi_bci_parse_dts(struct device_node *np, struct hisi_bci_device_info *di)
{
	int ret = 0;
	int array_len = 0;
	int i = 0;
	const char *bci_data_string = NULL;
	int idata = 0;

	if (of_property_read_u32(np, "battery_design_fcc", (u32 *)&di->bat_design_fcc)) {
		di->bat_design_fcc = hisi_battery_fcc_design();
		bci_err("error:get battery_design_fcc value failed, used default value from batt_parm!\n");
	}

	if (of_property_read_u32(np, "bci_soc_at_term", (u32 *)&di->bci_soc_at_term)) {
		di->bci_soc_at_term = 100;
		bci_err("error:get bci_soc_at_term value failed, no early term in bci !\n");
	}
	bci_info("bci_soc_at_term =%d",di->bci_soc_at_term);

	if (of_property_read_u32(np, "battery_is_removable", (u32 *)&removable_batt_flag)) {
		removable_batt_flag = 0;
		bci_err("error:get removable_batt_flag value failed!\n");
	}
	if (of_property_read_u32(np, "battery_board_type",(u32 *)&is_board_type)) {
		is_board_type = BAT_BOARD_SFT;
		bci_err("error:get battery_board_type value failed!\n");
	}
	if (of_property_read_u32(np, "google_battery_node", (u32 *)&google_battery_node)) {
		google_battery_node = 0;
		bci_err("error:get google_battery_node value failed!\n");
	}

	/*bci_work_interval_para*/
	array_len = of_property_count_strings(np, "bci_work_interval_para");
	if ((array_len <= 0) || (array_len % WORK_INTERVAL_PARA_TOTAL != 0)) {
		bci_err("bci_work_interval_para is invaild,please check iput number!!\n");
		return -EINVAL;
	}

	if (array_len > WORK_INTERVAL_PARA_LEVEL * WORK_INTERVAL_PARA_TOTAL) {
		bci_err("bci_work_interval_para is too long,use only front %d paras!!\n", array_len);
		return -EINVAL;
	}

	memset(di->interval_data, 0, WORK_INTERVAL_PARA_LEVEL * sizeof(struct work_interval_para));

	for (i = 0; i < array_len; i++) {
		ret = of_property_read_string_index(np, "bci_work_interval_para", i, &bci_data_string);
		if (ret) {
			bci_err("get bci_work_interval_para failed\n");
			return -EINVAL;
		}

		idata = simple_strtol(bci_data_string, NULL, 10);
		switch (i % WORK_INTERVAL_PARA_TOTAL) {
		case WORK_INTERVAL_CAP_MIN:
			if ((idata < CAPACITY_MIN) || (idata > CAPACITY_MAX)) {
				bci_err("the bci_work_interval_para cap_min is out of range!!\n");
				return -EINVAL;
			}
			di->interval_data[i / (WORK_INTERVAL_PARA_TOTAL)].cap_min = idata;
			break;
		case WORK_INTERVAL_CAP_MAX:
			if ((idata < CAPACITY_MIN) || (idata > CAPACITY_MAX)) {
				bci_err("the bci_work_interval_para cap_max is out of range!!\n");
				return -EINVAL;
			}
			di->interval_data[i / (WORK_INTERVAL_PARA_TOTAL)].cap_max = idata;
			break;
		case WORK_INTERVAL_VALUE:
			if( (idata < WORK_INTERVAL_MIN) || (idata > WORK_INTERVAL_MAX)) {
				bci_err("the bci_work_interval_para work_interval is out of range!!\n");
				return -EINVAL;
			}
			di->interval_data[i / (WORK_INTERVAL_PARA_TOTAL)].work_interval = idata;
			break;
		}
		bci_info("di->interval_data[%d][%d] = %d\n",
					i / (WORK_INTERVAL_PARA_TOTAL), i % (WORK_INTERVAL_PARA_TOTAL), idata);
	}
	parse_cap_dec_dts(np,di);
    ret |= parse_vth_correct_dts(np, di);
	return ret;
}

static int hisi_bci_battery_probe(struct platform_device *pdev)
{
	struct hisi_bci_device_info *di;
	int low_bat_flag = 0;
	int ret = 0;
	unsigned int i = 0;
	struct device_node *np;

	struct class *power_class = NULL;
	struct device *new_dev = NULL;
	np = pdev->dev.of_node;
	if (NULL == np)
		return -1;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;
	g_hisi_bci_dev = di;

	di->bat_max_volt = hisi_battery_vbat_max();
	di->monitoring_interval = WORK_INTERVAL_NOARMAL;
	di->dev = &pdev->dev;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
	di->bat.name = "Battery";
	di->bat.supplied_to = hisi_bci_supplied_to;
	di->bat.num_supplicants = ARRAY_SIZE(hisi_bci_supplied_to);
	di->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties = hisi_bci_battery_props;
	di->bat.num_properties = ARRAY_SIZE(hisi_bci_battery_props);
	di->bat.get_property = hisi_bci_battery_get_property;

	di->usb.name = "USB";
	di->usb.type = POWER_SUPPLY_TYPE_USB;
	di->usb.properties = hisi_usb_props;
	di->usb.num_properties = ARRAY_SIZE(hisi_usb_props);
	di->usb.get_property = hisi_usb_get_property;

	di->ac.name = "Mains";
	di->ac.type = POWER_SUPPLY_TYPE_MAINS;
	di->ac.properties = hisi_ac_props;
	di->ac.num_properties = ARRAY_SIZE(hisi_ac_props);
	di->ac.get_property = hisi_ac_get_property;

	di->bk_bat.name = "hisi_bk_battery";
	di->bk_bat.type = POWER_SUPPLY_TYPE_UPS;
	di->bk_bat.properties = hisi_bk_bci_battery_props;
	di->bk_bat.num_properties = ARRAY_SIZE(hisi_bk_bci_battery_props);
	di->bk_bat.get_property = hisi_bk_bci_battery_get_property;

	di->bat_google.name = "battery";
	di->bat_google.supplied_to = hisi_bci_supplied_to;
	di->bat_google.num_supplicants = ARRAY_SIZE(hisi_bci_supplied_to);
	di->bat_google.type = POWER_SUPPLY_TYPE_UNKNOWN;
	di->bat_google.properties = hisi_bci_battery_props;
	di->bat_google.num_properties = ARRAY_SIZE(hisi_bci_battery_props);
	di->bat_google.get_property = hisi_bci_battery_get_property;

#endif
	di->bat_health = POWER_SUPPLY_HEALTH_GOOD;
	di->bat_exist = is_hisi_battery_exist();
	di->bat_err = 0;
	di->power_supply_status = POWER_SUPPLY_HEALTH_GOOD;
	di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;

	di->capacity = -1;/*lint !e570*/
	di->capacity_filter_count = 0;
	di->charge_full_count = 0;

	for (i = 0; i < WINDOW_LEN; i++) {
		capacity_filter[i] = (unsigned int)hisi_battery_capacity();
		capacity_sum += capacity_filter[i];
		bci_info("capacity_filter[%d] = %d\n", i, capacity_filter[i]);
	}

	hrtimer_init(&di->capacity_dec_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	di->capacity_dec_timer.function	= capacity_dec_timer_func;

	hisi_get_battery_info(di);

	platform_set_drvdata(pdev, di);

	wake_lock_init(&low_power_lock, WAKE_LOCK_SUSPEND,
		       "low_power_wake_lock");

	low_bat_flag = is_hisi_battery_reach_threshold();
	if ((low_bat_flag & BQ_FLAG_LOCK) == BQ_FLAG_LOCK) {
		wake_lock(&low_power_lock);
		is_low_power_locked = 1;
	}

	hisi_bci_parse_dts(np, di);/*lint !e456 */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
	ret = power_supply_register(&pdev->dev, &di->bat);
	if (ret) {
		bci_debug("failed to register main battery\n");
		goto batt_failed;
	}

	ret = power_supply_register(&pdev->dev, &di->usb);
	if (ret) {
		bci_debug("failed to register usb power supply\n");
		goto usb_failed;
	}

	ret = power_supply_register(&pdev->dev, &di->ac);
	if (ret) {
		bci_debug("failed to register ac power supply\n");
		goto ac_failed;
	}

	ret = power_supply_register(&pdev->dev, &di->bk_bat);
	if (ret) {
		bci_debug("failed to register backup battery\n");
		goto bk_batt_failed;
	}

	if(google_battery_node){
		ret = power_supply_register(&pdev->dev, &di->bat_google);
		if (ret) {
			bci_debug("failed to register google battery\n");
			goto bat_google_failed;
		}
	}
#else
	di->bat = power_supply_register(&pdev->dev, &hisi_bci_battery_desc, &hisi_bci_battery_cfg);/*lint !e456*/
	if (IS_ERR(di->bat)) {
		bci_debug("failed to register main battery\n");
		goto batt_failed;
	}

	di->usb = power_supply_register(&pdev->dev, &hisi_bci_usb_desc, NULL);
	if (IS_ERR(di->usb)) {
		bci_debug("failed to register usb power supply\n");
		goto usb_failed;
	}

	di->ac = power_supply_register(&pdev->dev, &hisi_bci_mains_desc, NULL);
	if (IS_ERR(di->ac)) {
		bci_debug("failed to register ac power supply\n");
		goto ac_failed;
	}

	di->wireless = power_supply_register(&pdev->dev, &hisi_bci_wireless_desc, NULL);
	if (IS_ERR(di->wireless)) {
		bci_debug("failed to register wireless power supply\n");
		goto wireless_failed;
	}

	di->bk_bat = power_supply_register(&pdev->dev, &hisi_bci_bk_battery_desc, NULL);
	if (IS_ERR(di->bk_bat)) {
		bci_debug("failed to register backup battery\n");
		goto bk_batt_failed;
	}

	if(google_battery_node){
		di->bat_google= power_supply_register(&pdev->dev, &hisi_bci_bat_google_desc, &hisi_bci_battery_cfg);
		if (IS_ERR(di->bat_google)) {
			bci_debug("failed to register google battery\n");
			goto bat_google_failed;
		}
	}
#endif
	power_class = hw_power_get_class();
	if (power_class) {
		new_dev = device_create(power_class, NULL, 0, "%s", "bq_bci");
		if (IS_ERR(new_dev))
			new_dev = NULL;

		if (new_dev) {
			dev_set_drvdata(new_dev, di);
			g_sysfs_bq_bci = &new_dev->kobj;
			ret = sysfs_create_group(&new_dev->kobj, &hisi_bci_attr_group);
			if (ret) {
				bci_err("%s, could not create sysfs files hisi_bci!\n", __func__);
			}
		} else {
			bci_err("%s, could not create dev hisi_bci!\n", __func__);
		}
	}

	INIT_DELAYED_WORK(&di->hisi_bci_monitor_work, hisi_bci_battery_work);
	queue_delayed_work(system_power_efficient_wq, &di->hisi_bci_monitor_work, 0);

	di->nb.notifier_call = hisi_charger_event;
	hisi_register_notifier(&di->nb, 1);
	bci_info("hisi_bci probe ok!\n");

	return 0;/*lint !e454*/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
bat_google_failed:
	cancel_delayed_work(&di->hisi_bci_monitor_work);
	power_supply_unregister(&di->bk_bat);
bk_batt_failed:
	if(!google_battery_node){
		cancel_delayed_work(&di->hisi_bci_monitor_work);
	}
	power_supply_unregister(&di->ac);
ac_failed:
	power_supply_unregister(&di->usb);
usb_failed:
	power_supply_unregister(&di->bat);
#else
bat_google_failed:
    cancel_delayed_work(&di->hisi_bci_monitor_work);
    power_supply_unregister(di->bk_bat);
bk_batt_failed:
    if(!google_battery_node){
		cancel_delayed_work(&di->hisi_bci_monitor_work);
    }
    power_supply_unregister(di->ac);
wireless_failed:
    power_supply_unregister(&di->wireless);
ac_failed:
    power_supply_unregister(di->usb);
usb_failed:
    power_supply_unregister(di->bat);
#endif
batt_failed:
	wake_lock_destroy(&low_power_lock);
	platform_set_drvdata(pdev, NULL);
	kfree(di);
	di = NULL;
	return ret;/*lint !e454*/
}

static int hisi_bci_battery_remove(struct platform_device *pdev)
{
	struct hisi_bci_device_info *di = platform_get_drvdata(pdev);

	if (di == NULL) {
		bci_err("di is NULL!\n");
		return -ENODEV;
	}

	hisi_unregister_notifier(&di->nb, 1);
	cancel_delayed_work(&di->hisi_bci_monitor_work);
	wake_lock_destroy(&low_power_lock);
	platform_set_drvdata(pdev, NULL);
	kfree(di);
	di = NULL;

	return 0;
}

static void hisi_bci_battery_shutdown(struct platform_device *pdev)
{
	struct hisi_bci_device_info *di = platform_get_drvdata(pdev);

	bci_info("%s ++\n", __func__);
	if (di == NULL) {
		bci_err("di is NULL!\n");
		return;
	}

	hisi_unregister_notifier(&di->nb, 1);
	cancel_delayed_work(&di->hisi_bci_monitor_work);
	wake_lock_destroy(&low_power_lock);
	bci_info("%s --\n", __func__);

	return;
}

#ifdef CONFIG_PM
static int hisi_bci_battery_suspend(struct platform_device *pdev,
				    pm_message_t state)
{
	struct hisi_bci_device_info *di = platform_get_drvdata(pdev);

	if (di == NULL) {
		bci_err("di is NULL!\n");
		return -ENODEV;
	}
	bci_info("%s:+\n", __func__);
	suspend_capacity = hisi_battery_capacity();
	cancel_delayed_work(&di->hisi_bci_monitor_work);
	bci_info("%s:-\n", __func__);
	return 0;
}

static int hisi_bci_battery_resume(struct platform_device *pdev)
{
	struct hisi_bci_device_info *di = platform_get_drvdata(pdev);
	int i = 0, resume_capacity = 0;

	if (di == NULL) {
		bci_err("di is NULL!\n");
		return -ENODEV;
	}

	bci_info("%s:+\n", __func__);
	capacity_debounce_flag = true;

	resume_capacity = hisi_battery_capacity();
	if (di->charge_status == POWER_SUPPLY_STATUS_DISCHARGING
	    || di->charge_status == POWER_SUPPLY_STATUS_NOT_CHARGING) {
		if ((suspend_capacity - resume_capacity) >= 2) {
			capacity_sum = 0;
			for (i = 0; i < WINDOW_LEN; i++) {
				capacity_filter[i] = resume_capacity;
				capacity_sum += capacity_filter[i];
			}
		}
	}
#if defined (CONFIG_HUAWEI_DSM)
	g_curr_zero_times = 0;
#endif
	queue_delayed_work(system_power_efficient_wq, &di->hisi_bci_monitor_work, 0);
	bci_info("%s:-\n", __func__);
	return 0;
}
#endif /* CONFIG_PM */

static struct class *hw_power_class;
struct class *hw_power_get_class(void)
{
	if (NULL == hw_power_class) {
		hw_power_class = class_create(THIS_MODULE, "hw_power");
		if (IS_ERR(hw_power_class)) {
			bci_err("hw_power_class create fail");
			return NULL;
		}
	}
	return hw_power_class;
}
EXPORT_SYMBOL_GPL(hw_power_get_class);

static struct of_device_id hisi_bci_battery_match_table[] = {
	{
	 .compatible = "huawei,hisi_bci_battery",
	 .data = NULL,
	 },
	{
	 },
};

static struct platform_driver hisi_bci_battery_driver = {
	.probe = hisi_bci_battery_probe,
	.remove = hisi_bci_battery_remove,
#ifdef CONFIG_PM
	.suspend = hisi_bci_battery_suspend,
	.resume = hisi_bci_battery_resume,
#endif
	.shutdown = hisi_bci_battery_shutdown,
	.driver = {
		   .name = "huawei,hisi_bci_battery",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(hisi_bci_battery_match_table),
		   },
};

static int __init hisi_battery_init(void)
{
	return platform_driver_register(&hisi_bci_battery_driver);
}

module_init(hisi_battery_init);

static void __exit hisi_battery_exit(void)
{
	platform_driver_unregister(&hisi_bci_battery_driver);
}

module_exit(hisi_battery_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("hisi_bci");
MODULE_AUTHOR("HISILICON");
