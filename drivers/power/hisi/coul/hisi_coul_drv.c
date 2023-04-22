/****************************************************************************
* Filename:	hisi_coul_drv.c
*
* Discription:  this file provide interface to get the battery state such as
*			capacity, voltage, current, temperature.
* Copyright:	(C) 2013 huawei.
*
* revision history:
* 03/25/13 yuanqinshun -v1.0
*****************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include "hisi_coul_drv_test.h"

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

#define COUL_DRV_LOG_INFO
#ifndef COUL_DRV_LOG_INFO
#define coul_drv_debug(fmt, args...)do {} while (0)
#define coul_drv_info(fmt, args...) do {} while (0)
#define coul_drv_warn(fmt, args...) do {} while (0)
#define coul_drv_err(fmt, args...)  do {} while (0)
#else
#define coul_drv_debug(fmt, args...)do { printk(KERN_DEBUG   "[coul_drv]" fmt, ## args); } while (0)
#define coul_drv_info(fmt, args...) do { printk(KERN_INFO    "[coul_drv]" fmt, ## args); } while (0)
#define coul_drv_warn(fmt, args...) do { printk(KERN_WARNING"[coul_drv]" fmt, ## args); } while (0)
#define coul_drv_err(fmt, args...)  do { printk(KERN_ERR   "[coul_drv]" fmt, ## args); } while (0)
#endif
#define HISI_COUL_LOCK()    do {} while (0)
#define HISI_COUL_UNLOCK()  do {} while (0)

static int hisi_coul_drv_init = 0;
static struct mutex hisi_coul_drv_lock;
#define HISI_COUL_DRV_LOCK()    do {if(!hisi_coul_drv_init) \
                                        return; \
                                    mutex_lock(&hisi_coul_drv_lock);} while (0)
#define HISI_COUL_DRV_UNLOCK()  do {mutex_unlock(&hisi_coul_drv_lock);} while (0)

static struct hisi_coul_ops *g_hisi_coul_ops;
static enum HISI_COULOMETER_TYPE g_hisi_coul_type = COUL_UNKNOW;
/*lint -e773*/
#define LOCAL_HISI_COUL_OPS() struct hisi_coul_ops *ops = g_hisi_coul_ops
/*lint +e773*/
// cppcheck-suppress *
#define HISI_EXEC_COUL_OP(op) do {if (ops && ops->op) return ops->op(); } while (0)

/****************************************************************************
  Function:     hisi_coulometer_type()
  Description:  get the hisi coulometer type
  Input:        NA
  Output:       NA
  Return:       enum HISI_COULOMETER_TYPE

****************************************************************************/
enum HISI_COULOMETER_TYPE hisi_coulometer_type(void)
{
	return g_hisi_coul_type;
}

/****************************************************************************
  Function:     is_hisi_coul_ready
  Description:  return the coul module state
  Input:        NA
  Output:       NA
  Return:       0:coul module isn't ready
			1:coul module is ready
****************************************************************************/
int is_hisi_coul_ready(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*make sure coul moudule have registered */
	if (g_hisi_coul_ops == NULL)
		return 0;

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(is_coul_ready);

	return 0;
}

/*******************************************************
  Function:        is_hisi_fcc_debounce
  Description:     check whether fcc is debounce
  Input:           NULL
  Output:          NULL
  Return:          0: no  1: is debounce
********************************************************/
int is_hisi_fcc_debounce(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(coul_is_fcc_debounce);
	return 0;

}

/*******************************************************
  Function:        hisi_battery_get_qmax
  Description:     get battery qmax
  Input:           NULL
  Output:          NULL
  Return:          0: no  other:success
********************************************************/
int hisi_battery_get_qmax(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(get_qmax);
	return 0;
}
/*******************************************************
  Function:        hisi_coul_set_hltherm_flag
  Description:     set the flag for high or low temperature test
  Input:           temp_flag: 1 hltherm test
						0 no test
  Output:          NULL
  Return:          0: set success
				other: set fail
********************************************************/
int hisi_coul_set_hltherm_flag(int temp_flag)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();
	/*execute the operation of coul module */
	if (ops && ops->set_hltherm_flag)
		return ops->set_hltherm_flag(temp_flag);
	return 0;
}

/*******************************************************
  Function:        hisi_coul_get_hltherm_flag
  Description:     get the flag for high or low temperature test
  Input:           temp_flag: 1 hltherm test
						0 no test
  Output:          NULL
  Return:          1: set hltherm flag
			0: no set
********************************************************/
int hisi_coul_get_hltherm_flag(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();
	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(get_hltherm_flag);
	return 0;
}

/****************************************************************************
  Function:     is_hisi_battery_exist
  Description:  check whether battery is exist
  Input:        NA
  Output:       NA
  Return:       0:battery isn't exist
			1:battery is exist
****************************************************************************/
int is_hisi_battery_exist(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();
	if (NULL != g_hisi_coul_test_info) {
		if (g_hisi_coul_test_info->test_start_flag & 0x0001) {
			coul_drv_info("hisi_battery exist status is %d\n", g_hisi_coul_test_info->input_batt_exist);
			return g_hisi_coul_test_info->input_batt_exist;
		}
	}
	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(is_battery_exist);

	return 0;
}

/****************************************************************************
  Function:     is_hisi_battery_reach_threshold
  Description:  check whether remaining capacity of battery reach the low
			power threshold
  Input:        NA
  Output:       NA
  Return:       0: no't reach the threshold
			1: reach the threshold
****************************************************************************/
int is_hisi_battery_reach_threshold(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(is_battery_reach_threshold);

	return 0;
}

/****************************************************************************
  Function:     hisi_battery_brand
  Description:  return the brand of battery
  Input:        NA
  Output:       NA
  Return:       the battery brand in string
			Or < 0 if something fails.
****************************************************************************/
char *hisi_battery_brand(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_brand);

	return "error";
}

/****************************************************************************
  Function:     hisi_battery_id_voltage
  Description:  return the voltage of battery id
  Input:        NA
  Output:       NA
  Return:       the battery id voltage
			Or < 0 if something fails.
****************************************************************************/
int hisi_battery_id_voltage(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_id_voltage);

	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_voltage
  Description:  return the voltage of battery
  Input:        NA
  Output:       NA
  Return:       the battery Voltage in milivolts
			Or < 0 if something fails.
****************************************************************************/
int hisi_battery_voltage(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();
	if (NULL != g_hisi_coul_test_info) {
		if (g_hisi_coul_test_info->test_start_flag & 0x0004) {
			coul_drv_info("the battery voltage is %d\n", g_hisi_coul_test_info->input_batt_volt);
			return g_hisi_coul_test_info->input_batt_volt;
		}
	}
	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_voltage);

	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_voltage_uv
  Description:  return the voltage of battery
  Input:        NA
  Output:       NA
  Return:       the battery Voltage in microvolts
			Or < 0 if something fails.
****************************************************************************/
int hisi_battery_voltage_uv(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_voltage_uv);

	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_current
  Description:  return the current of battery
  Input:        NA
  Output:       NA
  Return:       the battery average current
			Note that current can be negative signed as well
			Or 0 if something fails.
****************************************************************************/
int hisi_battery_current(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();
	if (NULL != g_hisi_coul_test_info) {
		if (g_hisi_coul_test_info->test_start_flag & 0x0008) {
			coul_drv_info("the input batt cur is %d\n", g_hisi_coul_test_info->input_batt_cur);
			return g_hisi_coul_test_info->input_batt_cur;
		}
	}
	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_current);

	return 0;
}
int hisi_get_coul_calibration_status(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(get_coul_calibration_status);

	return 0;

}

/****************************************************************************
  Function:     hisi_battery_resistance
  Description:  return the resistance of battery
  Input:        NA
  Output:       NA
  Return:       return the resistance of battery.
****************************************************************************/
int hisi_battery_resistance(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_resistance);

	return 0;
}

/****************************************************************************
  Function:     hisi_coul_fifo_avg_current
  Description:   get the average current of the coul fifo
  Input:        NA
  Output:       NA
  Return:       the battery average current
			Note that current can be negative signed as well
			Or 0 if something fails.
****************************************************************************/
int hisi_coul_fifo_avg_current(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(fifo_avg_current);

	return 0;
}

/****************************************************************************
  Function:     hisi_battery_current_avg
  Description:  return the current_avg of battery
  Input:        NA
  Output:       NA
  Return:       the battery average current
			Note that current can be negative signed as well
			Or 0 if something fails.
****************************************************************************/
int hisi_battery_current_avg(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_current_avg);

	return 0;
}

/****************************************************************************
  Function:     hisi_battery_unfiltered_capacity
  Description:  return the unfilted capacity of battery
  Input:        NA
  Output:       NA
  Return:       the battery Relative State-of-Charge
			The reture value is 0 - 100%
			Or < 0 if something fails.
****************************************************************************/
int hisi_battery_unfiltered_capacity(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_unfiltered_capacity);

	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_capacity
  Description:  return the capacity of battery
  Input:        NA
  Output:       NA
  Return:       the battery Relative State-of-Charge
			The reture value is 0 - 100%
			Or < 0 if something fails.
****************************************************************************/
int hisi_battery_capacity(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();
	if (NULL != g_hisi_coul_test_info) {
		if (g_hisi_coul_test_info->test_start_flag & 0x0010) {
			coul_drv_info("input batt cap is %d\n", g_hisi_coul_test_info->input_batt_capacity);
			return g_hisi_coul_test_info->input_batt_capacity;
		}
	}
	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_capacity);

	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_temperature
  Description:  return the temperature of battery
  Input:        NA
  Output:       NA
  Return:       the battery temperature in Celcius degrees
			Or < 0 if something fails.
****************************************************************************/
int hisi_battery_temperature(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();
	if (NULL != g_hisi_coul_test_info) {
		if (g_hisi_coul_test_info->test_start_flag & 0x0020) {
			coul_drv_info("input batt temp is %d\n", g_hisi_coul_test_info->input_batt_temp);
			return g_hisi_coul_test_info->input_batt_temp;
		}
	}
	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_temperature);

	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_temperature
  Description:  return the temperature of battery
  Input:        NA
  Output:       NA
  Return:       the battery temperature in Celcius degrees
			Or < 0 if something fails.
****************************************************************************/
int hisi_battery_temperature_for_charger(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();
	if (NULL != g_hisi_coul_test_info) {
		if (g_hisi_coul_test_info->test_start_flag & 0x0020) {
			coul_drv_info("input batt temp is %d\n", g_hisi_coul_test_info->input_batt_temp);
			return g_hisi_coul_test_info->input_batt_temp;
		}
	}
	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_temperature_for_charger);

	return -EPERM;
}

/****************************************************************************
  Function:       hisi_battery_soc_vary_flag
  Description:   monitor soc if vary too fast
  Input:           monitor flag: 0:monitoring in one period
                                         1:one period done
  Output:         deta_soc: variety of soc
  Return:         data valid:0: data is valid( soc err happened)
                                    others: data is invalid
****************************************************************************/
int hisi_battery_soc_vary_flag(int monitor_flag, int *deta_soc)
{
    /*declare the local variable of struct hisi_coul_ops */
    LOCAL_HISI_COUL_OPS();

    /*execute the operation of coul module */
    if (ops && ops->get_soc_vary_flag)
    {
        return ops->get_soc_vary_flag(monitor_flag, deta_soc);
    }
    return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_rm
  Description:  return the RemainingCapacity of battery
  Input:        NA
  Output:       NA
  Return:       battery RemainingCapacity,the reture value is mAh
			Or < 0 if something fails.
****************************************************************************/
int hisi_battery_rm(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_rm);

	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_fcc
  Description:  return the FullChargeCapacity of battery
  Input:        NA
  Output:       NA
  Return:       battery FullChargeCapacity,the reture value is mAh
			Or < 0 if something fails.
****************************************************************************/
int hisi_battery_fcc(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();
	if (NULL != g_hisi_coul_test_info) {
		if (g_hisi_coul_test_info->test_start_flag & 0x0040) {
			coul_drv_info("input batt fcc is %d\n", g_hisi_coul_test_info->input_batt_fcc);
			return g_hisi_coul_test_info->input_batt_fcc;
		}
	}
	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_fcc);

	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_fcc_design
  Description:  return the design FullChargeCapacity of battery in mAh
  Input:        NA
  Output:       NA
  Return:       battery FullChargeCapacity,the reture value is mAh
			Or < 0 if something fails.
****************************************************************************/
int hisi_battery_fcc_design(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_fcc_design);

	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_tte
  Description:  return the time to empty in minute
  Input:        NA
  Output:       NA
  Return:       time to empty in minute
			Or < 0 if something fails.
****************************************************************************/
int hisi_battery_tte(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_tte);

	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_ttf
  Description:  return the time to full in minute
  Input:        NA
  Output:       NA
  Return:       time to full in minute
			Or < 0 if something fails.
****************************************************************************/
int hisi_battery_ttf(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_ttf);

	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_health
  Description:  get the health state of battery
  Input:        NA
  Output:       NA
  Return:       0->"Unknown", 1->"Good", 2->"Overheat", 3->"Dead",
			4->"Over voltage", 5->"Unspecified failure", 6->"Cold",
****************************************************************************/
int hisi_battery_health(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_health);

	return 0;
}

/****************************************************************************
  Function:     hisi_battery_capacity_level
  Description:  get the capacity level of battery
  Input:        NA
  Output:       NA
  Return:       capacity level and 0 meaning unknow level
****************************************************************************/
int hisi_battery_capacity_level(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_capacity_level);

	return POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
}

/****************************************************************************
  Function:     hisi_battery_capacity_level
  Description:  get the technology of battery
  Input:        NA
  Output:       NA
  Return:       POWER_SUPPLY_TECHNOLOGY_LIPO
****************************************************************************/
int hisi_battery_technology(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_technology);

	return POWER_SUPPLY_TECHNOLOGY_LIPO;
}

/****************************************************************************
  Function:     hisi_battery_charge_params
  Description:  get the charge params of battery
  Input:        NA
  Output:       NA
  Return:       struct chrg_para_lut *chrg_para or NULL
****************************************************************************/
struct chrg_para_lut *hisi_battery_charge_params(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_charge_params);
	return NULL;
}

/****************************************************************************
  Function:     hisi_battery_ifull
  Description:  get the ifull of the battery
  Input:         NA
  Output:       NA
  Return:       ifull
****************************************************************************/
int hisi_battery_ifull(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_ifull);
	return -EPERM;
}


/****************************************************************************
  Function:     hisi_battery_vbat_max
  Description:  get the max battery voltage
  Input:         NA
  Output:       NA
  Return:       battery voltage Or < 0 if something fails
****************************************************************************/
int hisi_battery_vbat_max(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_vbat_max);
	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_get_limit_fcc
  Description:  get the battery limit fcc
  Input:         NA
  Output:       NA
  Return:       limit fcc
****************************************************************************/
int hisi_battery_get_limit_fcc(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(get_battery_limit_fcc);
	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_aging_safe_policy
  Description:  get the battery aging safe policy
  Input:        AGING_SAFE_POLICY *asp
  Output:       NA
  Return:       0 SUCCESS Or < 0 if something fails
****************************************************************************/
int hisi_battery_aging_safe_policy(AGING_SAFE_POLICY_TYPE *asp)
{
    /*declare the local variable of struct hisi_coul_ops */
    LOCAL_HISI_COUL_OPS();

    /*execute the operation of coul module */
    if (ops && ops->aging_safe_policy)
    {
        return ops->aging_safe_policy(asp);
    }
    return -EPERM;
}
/****************************************************************************
  Function:     hisi_battery_cycle_count
  Description:  get battery charging cycle count
  Input:         NA
  Output:       NA
  Return:       battery cycle count Or < 0 if something fails
****************************************************************************/
int hisi_battery_cycle_count(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_cycle_count);
	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_cc
  Description:  get battery cc
  Input:         NA
  Output:       NA
  Return:       battery cc Or < 0 if something fails
****************************************************************************/
int hisi_battery_cc(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	if (ops && ops->battery_cc) {
		return ops->battery_cc()/1000;
    }
	return -EPERM;
}

/****************************************************************************
  Function:     hisi_battery_cc_uah
  Description:  get battery cc uah
  Input:         NA
  Output:       NA
  Return:       battery cc Or < 0 if something fails
****************************************************************************/
int hisi_battery_cc_uah(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(battery_cc);
	return -EPERM;
}

/****************************************************************************
  Function:     hisi_power_supply_voltage
  Description:  get the voltage of the power suply, such as USB or AC port
  Input:        NA
  Output:       NA
  Return:       the battery Voltage in milivolts
			Or < 0 if something fails.
****************************************************************************/
int hisi_power_supply_voltage(void)
{
	return 4.2 * 1000;/*lint !e524*/
}

/****************************************************************************
  Function:     hisi_coul_charger_event_rcv()
  Description:  recevie charger event such as charging done/charging start
  Input:        NA
  Output:       NA
  Return:       NA
****************************************************************************/
void hisi_coul_charger_event_rcv(unsigned int event)
{

	extern struct blocking_notifier_head notifier_list;
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	HISI_COUL_DRV_LOCK();

	if (NULL != g_hisi_coul_test_info) {
		if (g_hisi_coul_test_info->test_start_flag & 0x0080) {
			coul_drv_info("input event is %d\n", g_hisi_coul_test_info->input_event);
			event = g_hisi_coul_test_info->input_event;
		}
	}
	/*execute the operation of coul module */
	if (ops && ops->charger_event_rcv) {
		coul_drv_info("charger event = 0x%x\n", (int)event);
		ops->charger_event_rcv(event);
	}

	blocking_notifier_call_chain(&notifier_list, event, NULL);

	HISI_COUL_DRV_UNLOCK();
}

/****************************************************************************
  Function:     hisi_coul_low_temp_opt
  Description:  get the flag of low temp opt
  Input:        NA
  Output:       NA
  Return:       flag
			    1:opt, 0:no
****************************************************************************/
int hisi_coul_low_temp_opt(void)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();

	/*execute the operation of coul module */
	HISI_EXEC_COUL_OP(coul_low_temp_opt);
	return 0;
}

/****************************************************************************
  Function:     hisi_coul_battery_fifo_curr
  Description:  get the fifo curr in ua
  Input:        NA
  Output:       NA
  Return:       current in ua
			    0:no ops registered
****************************************************************************/
int hisi_coul_battery_fifo_curr(unsigned int index)
{
	/*declare the local variable of struct hisi_coul_ops */
	LOCAL_HISI_COUL_OPS();
    if (ops && ops->battery_fifo_curr) {
        return ops->battery_fifo_curr(index);
    }
	return 0;
}

/****************************************************************************
  Function:     hisi_coul_battery_fifo_depth
  Description:  get the depth of fifo
  Input:        NA
  Output:       NA
  Return:       depth of fifo
			    0:no ops registered
****************************************************************************/
int hisi_coul_battery_fifo_depth(void)
{
	/*declare the local variable of struct hisi_coul_ops */
    LOCAL_HISI_COUL_OPS();
	/*execute the operation of coul module */
    HISI_EXEC_COUL_OP(battery_fifo_depth);
	return 0;
}

/****************************************************************************
  Function:     hisi_coul_battery_fifo_depth
  Description:  get the depth of fifo
  Input:        NA
  Output:       NA
  Return:       depth of fifo
			    0:no ops registered
****************************************************************************/
int hisi_coul_battery_ufcapacity_tenth(void)
{
	/*declare the local variable of struct hisi_coul_ops */
    LOCAL_HISI_COUL_OPS();
	/*execute the operation of coul module */
    HISI_EXEC_COUL_OP(battery_ufcapacity_tenth);
	return 0;
}

/****************************************************************************
  Function:     hisi_battery_removed_before_boot
  Description:  battery removed between this boot and last boot
  Input:        NA
  Output:       NA
  Return:       1 battery is removed between this boot and last boot
                0 battery is not removed between this boot and last boot
                -1 unknow yet
****************************************************************************/
int hisi_battery_removed_before_boot(void)
{
    /*declare the local variable of struct hisi_coul_ops */
    LOCAL_HISI_COUL_OPS();
    /*execute the operation of coul module */
    HISI_EXEC_COUL_OP(battery_removed_before_boot);
    return -1;
}
/****************************************************************************
  Function:     hisi_coul_ops_register
  Description:  register the hisi coul ops
  Input:        struct hisi_coul_ops *ops
  Output:       NA
  Return:       0: register successfull
			-EBUSY: register already
****************************************************************************/
int hisi_coul_ops_register(struct hisi_coul_ops *coul_ops, enum HISI_COULOMETER_TYPE coul_type)
{

	HISI_COUL_LOCK();
	if (g_hisi_coul_ops) {
		HISI_COUL_UNLOCK();
		coul_drv_err("coul ops have registered already\n");
		return -EBUSY;
	}

	g_hisi_coul_ops = coul_ops;
	g_hisi_coul_type = coul_type;
	HISI_COUL_UNLOCK();

	return 0;
}

/****************************************************************************
  Function:     hisi_coul_convert_regval2ua
  Description:  convert current regval to ua
  Input:        reg_val
  Output:       NA
  Return:       ua
****************************************************************************/
int hisi_coul_convert_regval2ua(unsigned int reg_val)
{
    /*declare the local variable of struct hisi_coul_ops */
    LOCAL_HISI_COUL_OPS();

    /*execute the operation of coul module */
    if (ops && ops->convert_regval2ua)
    {
        return ops->convert_regval2ua(reg_val);
    }
    return -EPERM;
}

/****************************************************************************
  Function:     hisi_coul_convert_regval2uv
  Description:  convert vol regval to uv
  Input:        reg_val
  Output:       NA
  Return:       uv
****************************************************************************/
int hisi_coul_convert_regval2uv(unsigned int reg_val)
{
    /*declare the local variable of struct hisi_coul_ops */
    LOCAL_HISI_COUL_OPS();

    /*execute the operation of coul module */
    if (ops && ops->convert_regval2uv)
    {
        return ops->convert_regval2uv(reg_val);
    }
    return -EPERM;
}


/****************************************************************************
  Function:     hisi_coul_convert_regval2temp
  Description:  convert chip temperature regval to бу
  Input:        reg_val
  Output:       NA
  Return:       бу
****************************************************************************/
int hisi_coul_convert_regval2temp(unsigned int reg_val)
{
    /*declare the local variable of struct hisi_coul_ops */
    LOCAL_HISI_COUL_OPS();

    /*execute the operation of coul module */
    if (ops && ops->convert_regval2temp)
    {
        return ops->convert_regval2temp(reg_val);
    }
    return -EPERM;
}

/****************************************************************************
  Function:     hisi_coul_convert_mv2regval
  Description:  convert voltage to regval
  Input:        vol mv
  Output:       NA
  Return:       regval
****************************************************************************/
int hisi_coul_convert_mv2regval(int vol_mv)
{
    /*declare the local variable of struct hisi_coul_ops */
    LOCAL_HISI_COUL_OPS();

    /*execute the operation of coul module */
    if (ops && ops->convert_mv2regval)
    {
        return ops->convert_mv2regval(vol_mv);
    }
    return -EPERM;
}

/****************************************************************************
  Function:     hisi_coul_cal_uah_by_ocv
  Description:  cal uah by ocv
  Input:        ocv_uv
  Output:       ocv_soc_uAh
  Return:       SUCCESS ro ERROR;
****************************************************************************/
int hisi_coul_cal_uah_by_ocv(int ocv_uv, int *ocv_soc_uAh)
{
    /*declare the local variable of struct hisi_coul_ops */
    LOCAL_HISI_COUL_OPS();

    /*execute the operation of coul module */
    if (ops && ops->cal_uah_by_ocv)
    {
        return ops->cal_uah_by_ocv(ocv_uv, ocv_soc_uAh);
    }
    return -EPERM;
}

/****************************************************************************
  Function:     hisi_coul_temp_to_adc
  Description:  convernt temp to adc value.
  Input:        temperature.
  Output:       NA
  Return:       adc value.
****************************************************************************/
int hisi_coul_convert_temp_to_adc(int temp)
{
    /*declare the local variable of struct hisi_coul_ops */
    LOCAL_HISI_COUL_OPS();

    /*execute the operation of coul module */
    if (ops && ops->convert_temp_to_adc)
    {
        return ops->convert_temp_to_adc(temp);
    }
    return -EPERM;
}
/****************************************************************************
  Function:     hisi_coul_ops_unregister
  Description:  UNregister the hisi coul ops
  Input:        struct hisi_coul_ops *ops
  Output:       NA
  Return:       0: unregister successfull
			-EINVAL: unregister failed
****************************************************************************/
int hisi_coul_ops_unregister(struct hisi_coul_ops *coul_ops)
{
	HISI_COUL_LOCK();
	if (g_hisi_coul_ops == coul_ops) {
		g_hisi_coul_ops = NULL;
		g_hisi_coul_type = COUL_UNKNOW;
		HISI_COUL_UNLOCK();
		return 0;
	}
	HISI_COUL_UNLOCK();

	return -EINVAL;
}

int __init hisi_coul_init(void)
{
	mutex_init(&hisi_coul_drv_lock);

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	/* detect coul device successful, set the flag as present */
	if (NULL != g_hisi_coul_ops && NULL != g_hisi_coul_ops->dev_check) {
		if (COUL_IC_GOOD == g_hisi_coul_ops->dev_check()) {
			coul_drv_info("coul ic is good.\n");
			set_hw_dev_flag(DEV_I2C_COUL);
		} else {
			coul_drv_err("coul ic is bad.\n");
		}
	} else {
		coul_drv_err("ops dev_check is null.\n");
	}
#endif
	hisi_coul_drv_init = 1; //here 1 present init seccess
	coul_drv_info("hisi_coul_init\n");
	return 0;
}

module_init(hisi_coul_init);

void __exit hisi_coul_exit(void)
{
	return;
}

module_exit(hisi_coul_exit);

MODULE_AUTHOR("HUAWEI_HISI");
MODULE_DESCRIPTION("hisi coul module driver");
MODULE_LICENSE("GPL");
