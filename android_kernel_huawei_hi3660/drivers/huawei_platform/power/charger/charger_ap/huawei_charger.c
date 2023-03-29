/*
 * drivers/power/huawei_charger.c
 *
 *huawei charger driver
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
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/hrtimer.h>
#include <linux/wakelock.h>
#include <linux/usb/otg.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/power_supply.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <linux/mfd/hisi_pmic.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/usb/switch/usbswitch_common.h>
#include <huawei_platform/usb/switch/switch_ap/switch_usb_class.h>
#include <linux/delay.h>

#ifdef CONFIG_TCPC_CLASS
#include <huawei_platform/usb/pd/richtek/tcpm.h>
#include <huawei_platform/usb/hw_pd_dev.h>
#endif

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

#include <linux/raid/pq.h>
#ifdef CONFIG_HISI_BCI_BATTERY
#include <linux/power/hisi/hisi_bci_battery.h>
#endif
#include <huawei_platform/power/huawei_charger.h>
#ifdef CONFIG_HISI_COUL
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#endif
#include "charging_core.h"
#ifdef CONFIG_SWITCH_FSA9685
#include <huawei_platform/usb/switch/switch_fsa9685.h>
#endif
#ifdef CONFIG_DIRECT_CHARGER
#include <huawei_platform/power/direct_charger.h>
#endif
#ifdef  CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
#include <huawei_platform/power/usb_short_circuit_protect.h>
#endif
#ifdef CONFIG_WIRELESS_CHARGER
#include <huawei_platform/power/wireless_charger.h>
#include <huawei_platform/power/wireless_transmitter.h>
#endif
#ifdef CONFIG_HUAWEI_TYPEC
#include <huawei_platform/usb/hw_typec_dev.h>
#endif

/* battery safety notifier head */
BLOCKING_NOTIFIER_HEAD(charger_event_notify_head);

#ifdef CONFIG_OTG_GPIO_ID
#include <huawei_platform/usb/huawei_ycable.h>

extern struct ycable_info *ycable;
#endif

#ifdef CONFIG_FORCE_FAST_CHARGE
#include <linux/fastchg.h>
#endif

/*adaptor test result*/
struct adaptor_test_attr adptor_test_tbl[] = {
	{TYPE_SCP, "SCP", DETECT_FAIL},
	{TYPE_FCP, "FCP", DETECT_FAIL},
	{TYPE_PD, "PD", DETECT_FAIL},
	{TYPE_SC, "SC", DETECT_FAIL},
};

/*lint -save -e* */
#ifdef  HWLOG_TAG
#undef  HWLOG_TAG
#endif
#define HWLOG_TAG huawei_charger
HWLOG_REGIST();
BLOCKING_NOTIFIER_HEAD(charge_wake_unlock_list);
/*lint -restore*/

static struct wake_lock charge_lock;
static struct wake_lock otg_lock;
static struct wake_lock stop_charge_lock;
static struct wake_lock uscp_plugout_lock;
extern struct charge_device_ops *g_ops;
static struct water_detect_ops *g_wd_ops;
static struct charge_switch_ops *g_sw_ops;
static struct fcp_adapter_device_ops *g_fcp_ops;
static enum fcp_check_stage_type fcp_stage = FCP_STAGE_DEFAUTL;
struct device *charge_dev;
static struct charge_device_info *g_di;
static u32 is_board_type;	/*0:sft 1:udp 2:asic */
static struct mutex charge_wakelock_flag_lock;
static struct work_struct resume_wakelock_work;
static enum charge_wakelock_flag charge_lock_flag = CHARGE_NO_NEED_WAKELOCK;
static bool charge_done_sleep_dts;
static bool ts_flag = FALSE;
static bool otg_flag = FALSE;
static bool cancel_work_flag = FALSE;
static int pd_charge_flag = false;
static bool term_err_chrg_en_flag = TRUE;
static int term_err_cnt = 0;
static int batt_ifull = 0;
static int clear_iin_avg_flag = 0;
static int last_vset = 0;
#ifdef CONFIG_TCPC_CLASS
static u32 charger_pd_support = 0;
static u32 charger_pd_cur_trans_ratio = 0;
#endif
static bool charger_type_ever_notify = false;
static int fcp_charge_flag;
#define REG_NUM 21
struct hisi_charger_bootloader_info {
	bool info_vaild;
	int ibus;
	char reg[REG_NUM];
};
extern char *get_charger_info_p(void);
extern int is_water_intrused(void);
static struct hisi_charger_bootloader_info hisi_charger_info = { 0 };

#ifdef CONFIG_HISI_CHARGER_SYS_WDG
#define CHARGE_SYS_WDG_TIMEOUT  180
extern void charge_enable_sys_wdt(void);
extern void charge_stop_sys_wdt(void);
extern void charge_feed_sys_wdt(unsigned int timeout);
#endif
static void charge_wake_unlock(void);

static int input_current_step;
static int input_current_delay;
static int input_current_now;
static int iin_set_complete_flag;
#define CHARGER_BASE_ADDR 512
#define VBUS_REPORT_NUM 4
#define WORK_DELAY_5000MS 5000

static int vbus_flag;
static int output_num;
static int detect_num;
static int switch_status_num;
static int nonfcp_vbus_higher_count;
static int fcp_vbus_lower_count;
static int pd_vbus_abnormal_cnt;

static int ico_enable;
static int nonstand_detect_times;
static bool charger_type_update = FALSE;
static unsigned int otg_ctrl_enable = 0;


static int dcp_set_enable_charger(unsigned int val)
{
	struct charge_device_info *di = g_di;
	int ret = 0;
	int batt_temp = DEFAULT_NORMAL_TEMP;
	enum charge_status_event events = VCHRG_POWER_NONE_EVENT;
	enum hisi_charger_type type;

	if (NULL == di) {
		hwlog_err("%s di is NULL\n", __func__);
		return -1;
	}

	if ((val < 0) || (val > 1)) {
		return -1;
	}

	ret = set_charger_disable_flags(
		val?CHARGER_CLEAR_DISABLE_FLAGS:CHARGER_SET_DISABLE_FLAGS,
		CHARGER_SYS_NODE);
	di->sysfs_data.charge_limit = TRUE;
	/*why should send events in this command?
		because it will get the /sys/class/power_supply/Battery/status immediately
		to check if the enable/disable command set successfully or not in some product line station
	*/
	hwlog_info("dcp: RUNNINGTEST set charge enable = %d\n", di->sysfs_data.charge_enable);

	batt_temp = hisi_battery_temperature_for_charger();
	if(di->sysfs_data.charge_enable){
		if((batt_temp > BATT_EXIST_TEMP_LOW && batt_temp <= NO_CHG_TEMP_LOW) || batt_temp >= NO_CHG_TEMP_HIGH){
			hwlog_err("battery temp is %d, abandon enable_charge.\n", batt_temp);
			return -1;
		}
	}

	type = hisi_get_charger_type();
	if(PLEASE_PROVIDE_POWER == type && CHARGER_TYPE_WIRELESS != di->charger_type)
	{
		hwlog_err("hisi charge type:%d,charger_type: %d, abandon enable_charge.\n", type,di->charger_type);
		return -1;
	}

	di->ops->set_charge_enable(di->sysfs_data.charge_enable);
#ifdef CONFIG_DIRECT_CHARGER
	if (NOT_IN_SCP_CHARGING_STAGE == is_in_scp_charging_stage()) {
#endif
		if (hisi_pmic_get_vbus_status() && CHARGER_REMOVED != di->charger_type) {
			if (di->sysfs_data.charge_enable)
				events = VCHRG_START_CHARGING_EVENT;
			else
				events = VCHRG_NOT_CHARGING_EVENT;
			hisi_coul_charger_event_rcv(events);
		}
#ifdef CONFIG_DIRECT_CHARGER
	}
#endif

	return 0;
}

static int dcp_get_enable_charger(unsigned int *val)
{
	struct charge_device_info *di = g_di;

	if (NULL == di) {
		hwlog_err("%s di is NULL\n", __func__);
		return -1;
	}

	*val = di->sysfs_data.charge_enable;

	return 0;
}

/* define public power interface */
static struct power_if_ops dcp_power_if_ops = {
	.set_enable_charger = dcp_set_enable_charger,
	.get_enable_charger = dcp_get_enable_charger,
	.type_name = "dcp",
};


static void fcp_reg_dump(char* pstr);
#if defined CONFIG_HUAWEI_DSM
struct charger_dsm {
	int error_no;
	bool notify_enable;
	bool notify_always;
	void (*dump)(char*);
	char buf[ERR_NO_STRING_SIZE];
};
struct charger_dsm err_count[] = {
	{ERROR_FCP_VOL_OVER_HIGH, true, false, .dump = fcp_reg_dump, "fcp vbus is low "},
	{ERROR_FCP_DETECT, true, false, .dump = fcp_reg_dump, "fcp detect fail "},
	{ERROR_FCP_OUTPUT, true, false, .dump = fcp_reg_dump, "fcp voltage output fail "},
	{ERROR_SWITCH_ATTACH, true, false, .dump = fcp_reg_dump, "fcp adapter connect fail "},
	{ERROR_ADAPTER_OVLT, true, false, .dump = fcp_reg_dump, "fcp adapter voltage over high "},
	{ERROR_ADAPTER_OCCURRENT, true, false, .dump = fcp_reg_dump, "fcp adapter current over high "},
	{ERROR_ADAPTER_OTEMP, true, false, .dump = fcp_reg_dump, "fcp adapter temp over high "},
	{ERROR_SAFE_PLOICY_LEARN, true, false, .dump = batt_info_dump, "battery safe ploicy learn "},
	{ERROR_SAFE_PLOICY_LEARN1, true, false, .dump = batt_info_dump, "safe ploicy learn 1 "},
	{ERROR_SAFE_PLOICY_LEARN2, true, false, .dump = batt_info_dump, "safe ploicy learn 2 "},
	{ERROR_SAFE_PLOICY_LEARN3, true, false, .dump = batt_info_dump, "safe ploicy learn 3 "},
	{ERROR_BOOST_OCP, true, false, NULL, "otg ocp"},
	{ERROR_CHARGE_VBAT_OVP, true, false, .dump = batt_info_dump, "vbat ovp "},
	{ERROR_SCHARGERV200_FAULT, true, false, NULL, "schargerv200 fault"},
	{ERROR_CHARGE_I2C_RW, true, false, NULL, "Scharger I2C trans error"},
	{ERROR_WEAKSOURCE_HAPPEN, true, true, NULL, "weaksource happen, do dpm regulation"},
	{ERROR_VBUS_VOL_OVER_13400MV, true, false, NULL, "vbus over 13400 mv"},
	{ERROR_WEAKSOURCE_STOP_CHARGE, true, false, NULL, "weaksource detected, stop charging"},
};
#endif
static void charge_wake_lock(void);
static void charge_start_charging(struct charge_device_info *di);
static void charge_stop_charging(struct charge_device_info *di);
static void charge_send_uevent(struct charge_device_info *di);
static void charger_type_handler(enum usb_charger_type type, void*data);
int pmic_vbus_irq_is_enabled(void);
enum charger_event_type pd_dpm_get_source_sink_state(void);
/* event queue imlementation
 */
bool get_stop_charge_sync_flag(void)
{
	return cancel_work_flag;
}
static const char *charger_event_type_string(enum charger_event_type event)
{
	static const char *const charger_event_strings[] = {
		[START_SINK]          = "START_SINK",
		[STOP_SINK]           = "STOP_SINK",
		[START_SOURCE]        = "START_SOURCE",
		[STOP_SOURCE]         = "STOP_SOURCE",
		[START_SINK_WIRELESS] = "START_SINK_WIRELESS",
		[STOP_SINK_WIRELESS]  = "STOP_SINK_WIRELESS",
		[CHARGER_MAX_EVENT]   = "CHARGER_MAX_EVENT",
	};

	if (event < START_SINK || event > CHARGER_MAX_EVENT)
		return "illegal event";

	return charger_event_strings[event];
}
static int charger_event_queue_create(struct charger_event_queue *event_queue, unsigned int count)
{
	if (!event_queue) {
		hwlog_err("%s:bad argument (0x%pK)\n", __func__, event_queue);
		return -EINVAL;
	}

	count = (count >= MAX_EVENT_COUNT ? MAX_EVENT_COUNT : count);
	event_queue->max_event = count;
	event_queue->num_event = (count >= EVENT_QUEUE_UNIT ? EVENT_QUEUE_UNIT : count);

	event_queue->event = kzalloc(event_queue->num_event * sizeof(enum otg_dev_event_type), GFP_KERNEL);
	if (!event_queue->event) {
		hwlog_err("%s:Can't alloc space:%d!\n", __func__, event_queue->num_event);
		return -ENOMEM;
	}

	event_queue->enpos = 0;
	event_queue->depos = 0;
	event_queue->overlay = 0;
	event_queue->overlay_index = 0;

	return 0;
}

static void charger_event_queue_destroy(struct charger_event_queue *event_queue)
{
	if (!event_queue) {
		return ;
	}

	kfree(event_queue->event);
	event_queue->event = NULL;
	event_queue->enpos = 0;
	event_queue->depos = 0;
	event_queue->num_event = 0;
	event_queue->max_event = 0;
	event_queue->overlay = 0;
	event_queue->overlay_index = 0;
}

static int charger_event_queue_isfull(struct charger_event_queue *event_queue)
{
	if (!event_queue) {
		return -EINVAL;
	}

	return (((event_queue->enpos + 1) % event_queue->num_event) == (event_queue->depos));
}

static int charger_event_queue_isempty(struct charger_event_queue *event_queue)
{
	if (!event_queue) {
		return -EINVAL;
	}

	return (event_queue->enpos == event_queue->depos);
}

static inline void charger_event_queue_set_overlay(struct charger_event_queue *event_queue)
{
	if (event_queue->overlay) {
		return;
	}
	event_queue->overlay = 1;
	event_queue->overlay_index = event_queue->enpos;
}

static inline void charger_event_queue_clear_overlay(struct charger_event_queue *event_queue)
{
	event_queue->overlay = 0;
	event_queue->overlay_index = 0;
}

static int charger_event_enqueue(struct charger_event_queue *event_queue, enum charger_event_type event)
{
	/* no need verify argument, isfull will check it */
	if (charger_event_queue_isfull(event_queue)) {
		hwlog_err("event queue full!\n");
		return -ENOSPC;
	}

	if (event_queue->overlay) {
		if (event_queue->overlay_index == event_queue->enpos) {
			event_queue->enpos = ((event_queue->enpos + 1) % event_queue->num_event);
		}

		if (charger_event_queue_isempty(event_queue)) {
			hwlog_info("queue is empty, just enqueue!\n");
			event_queue->overlay_index = ((event_queue->overlay_index + 1) % event_queue->num_event);
			event_queue->enpos = ((event_queue->enpos + 1) % event_queue->num_event);
			event_queue->overlay = 0;
		}

		event_queue->event[event_queue->overlay_index] = event;
	} else {
		event_queue->event[event_queue->enpos] = event;
		event_queue->enpos = ((event_queue->enpos + 1) % event_queue->num_event);
	}

	return 0;
}

static enum charger_event_type charger_event_dequeue(struct charger_event_queue *event_queue)
{
	enum charger_event_type event;

	/* no need verify argument, isempty will check it */
	if (charger_event_queue_isempty(event_queue)) {
		return CHARGER_MAX_EVENT;
	}

	event = event_queue->event[event_queue->depos];
	event_queue->depos = ((event_queue->depos + 1) % event_queue->num_event);

	return event;
}

static int charger_event_check(enum charger_event_type new_event)
{
	int ret = 0;
	struct charge_device_info *di = g_di;

	if (di->event == CHARGER_MAX_EVENT)
		return 1;
	switch (new_event) {
	case START_SINK:
		if (di->event == STOP_SINK || di->event == STOP_SOURCE ||
			di->event == START_SINK_WIRELESS || di->event == STOP_SINK_WIRELESS)
			ret = 1;
		break;
	case STOP_SINK:
		if (di->event == START_SINK)
			ret = 1;
		break;
	case START_SOURCE:
		if (di->event == STOP_SINK || di->event == STOP_SOURCE ||
			di->event == START_SINK_WIRELESS || di->event == STOP_SINK_WIRELESS)
			ret = 1;
		break;
	case STOP_SOURCE:
		if ( di->event == START_SOURCE ||
			di->event == START_SINK_WIRELESS || di->event == STOP_SINK_WIRELESS)
			ret = 1;
		break;
	case START_SINK_WIRELESS:
		if (di->event == START_SOURCE || di->event == STOP_SOURCE ||
			di->event == STOP_SINK || di->event == STOP_SINK_WIRELESS)
			ret = 1;
		break;
	case STOP_SINK_WIRELESS:
		if (di->event == START_SINK_WIRELESS ||
			di->event == START_SOURCE || di->event == STOP_SOURCE)
			ret = 1;
		break;
	default:
		hwlog_err("%s: error event:%d\n", __func__, new_event);
		break;
	}
	return ret;
}
static void charger_handle_event(struct charge_device_info* di, enum charger_event_type event)
{
	mutex_lock(&di->event_type_lock);
	switch (event) {
	case START_SINK:
#ifdef CONFIG_TCPC_CLASS
		charger_type_handler(CHARGER_TYPE_SDP, NULL);
#endif
		charge_start_charging(di);
		break;
	case START_SINK_WIRELESS:
		di->charger_source = POWER_SUPPLY_TYPE_MAINS;
		di->charger_type = CHARGER_TYPE_WIRELESS;
		charge_send_uevent(di);
		charge_start_charging(di);
		break;
	case STOP_SINK:
#ifdef CONFIG_WIRELESS_CHARGER
		wireless_charge_wired_vbus_disconnect_handler();
#endif
	case STOP_SINK_WIRELESS:
		di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
		di->charger_type = CHARGER_REMOVED;
		charge_stop_charging(di);
		charge_send_uevent(di);
		break;
	case START_SOURCE:
		charge_wake_unlock();
		charge_otg_mode_enable(OTG_ENABLE, OTG_CTRL_WIRED_OTG);
		break;
	case STOP_SOURCE:
		charge_stop_charging(di);
		break;
	default:
		hwlog_err("%s: error event:%d\n", __func__, event);
		break;
	}
	mutex_unlock(&di->event_type_lock);
}
static void charger_event_work(struct work_struct *work)
{
	unsigned long flags;
	enum charger_event_type event;
	unsigned long timeout;
	unsigned long flag;
	struct charge_device_info *di = g_di;

	hwlog_info("%s+\n", __func__);

	while (!charger_event_queue_isempty(&di->event_queue)) {
		spin_lock_irqsave(&(di->event_spin_lock), flags);
		event = charger_event_dequeue(&di->event_queue);
		spin_unlock_irqrestore(&(di->event_spin_lock), flags);

		charger_handle_event(di, event);
	}

	spin_lock_irqsave(&(di->event_spin_lock), flags);
	charger_event_queue_clear_overlay(&di->event_queue);
	spin_unlock_irqrestore(&(di->event_spin_lock), flags);
	hwlog_info("%s-\n", __func__);
}

void charger_source_sink_event(enum charger_event_type event)
{
	unsigned long flags;
        int pmic_vbus_irq_enabled = 1;
	struct charge_device_info *di = g_di;

#ifdef CONFIG_TCPC_CLASS
        pmic_vbus_irq_enabled = pmic_vbus_irq_is_enabled();
#endif

	if (pmic_vbus_irq_enabled) {
		return;
	}
	if (NULL == di) {
		hwlog_info("%s di is NULL\n", __func__);
		return;
	}
	spin_lock_irqsave(&(di->event_spin_lock), flags);

	if (charger_event_check(event)) {
		hwlog_info("case = %s\n", charger_event_type_string(event));
		di->event = event;

		if (!charger_event_enqueue(&di->event_queue, event)) {
			if (!queue_work(system_power_efficient_wq,
					&di->event_work)) {
				hwlog_err("schedule event_work wait:%d\n", event);
			}
			charge_wake_lock();
		} else {
			hwlog_err("%s: can't enqueue event:%d\n", __func__, event);
		}

		if ((STOP_SOURCE == event) || (STOP_SINK == event) ||(STOP_SINK_WIRELESS == event)) {
			charger_event_queue_set_overlay(&di->event_queue);
		}
	} else {
		hwlog_err("last event: [%s], event [%s] was rejected.\n",
				charger_event_type_string(di->event), charger_event_type_string(event));
	}

	spin_unlock_irqrestore(&(di->event_spin_lock), flags);
}

int water_detect_ops_register(struct water_detect_ops *ops)
{
	int ret = 0;

	if (ops != NULL) {
		g_wd_ops = ops;
	} else {
		hwlog_err("water_detect ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}
void send_water_intrused_event(bool flag)
{
	struct charge_device_info *di = g_di;
	if (NULL == di)
		return;
	if (flag) {
		di->sysfs_data.water_intrused = 1;
		power_dsm_dmd_report(POWER_DSM_BATTERY, ERROR_NO_WATER_CHECK_IN_USB, "water check is triggered");
	} else{
		di->sysfs_data.water_intrused = 0;
	}
	hisi_coul_charger_event_rcv(VCHRG_STATE_WATER_INTRUSED);
}
void water_detect(void)
{
	int ret;
	struct charge_device_info *di = g_di;

    if (NULL == di)
        return;
    if (strstr(saved_command_line, "androidboot.mode=charger")
		||strstr(saved_command_line, "androidboot.swtype=factory"))
		return;
	if (g_wd_ops && g_wd_ops->is_water_intrused) {
		ret = g_wd_ops->is_water_intrused();
		if (1 == ret) {
			di->sysfs_data.water_intrused = 1;
			hwlog_info("water intruced!\n");
			hisi_coul_charger_event_rcv(VCHRG_STATE_WATER_INTRUSED);
			power_dsm_dmd_report(POWER_DSM_BATTERY, ERROR_NO_WATER_CHECK_IN_USB, "water check is triggered");
		}
	}
}
enum charge_done_type get_charge_done_type(void)
{
	struct charge_device_info *di = g_di;
	if (NULL == di)
		return CHARGE_DONE_NON;
	return di->sysfs_data.charge_done_status;
}

/**********************************************************
*  Function:       charge_wake_lock
*  Description:   apply charge wake_lock
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void charge_wake_lock(void)
{
	if (!wake_lock_active(&charge_lock)) {
		wake_lock(&charge_lock);
		hwlog_info("charge wake lock\n");
	}
}
/**********************************************************
*  Function:       uscp_plugout_wake_lock
*  Description:   apply uscp plugout_wake_lock
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void uscp_plugout_wake_lock(void)
{
	if (!wake_lock_active(&uscp_plugout_lock)) {
		wake_lock(&uscp_plugout_lock);
		hwlog_info("uscp_plugout_lock\n");
	}
}
/**********************************************************
*  Function:       uscp_plugout_wake_unlock
*  Description:   apply uscp plugout_wake_unlock
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void uscp_plugout_wake_unlock(void)
{
	if (wake_lock_active(&uscp_plugout_lock)) {
		wake_unlock(&uscp_plugout_lock);
		hwlog_info("uscp_plugout_unlock\n");
	}
}
/**********************************************************
*  Function:       charge_wake_unlock
*  Description:   release charge wake_lock
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void charge_wake_unlock(void)
{
	if (wake_lock_active(&charge_lock)) {
		wake_unlock(&charge_lock);
		hwlog_info("charge wake unlock\n");
	}
}
static void otg_wake_lock(void)
{
	if (!wake_lock_active(&otg_lock)) {
		wake_lock(&otg_lock);
		hwlog_info("otg wake lock\n");
	}
}
static void otg_wake_unlock(void)
{
	if (wake_lock_active(&otg_lock)) {
		wake_unlock(&otg_lock);
		hwlog_info("otg wake unlock\n");
	}
}
int charge_get_vbus(void)
{
	struct charge_device_info *di = g_di;
	unsigned int vbus = 0;

	if(NULL != di && NULL != di->ops){
		if(di->ops->get_vbus){
			di->ops->get_vbus(&vbus);
			return vbus;
		}
	}
	hwlog_err("NULL pointer\n",__func__);
	return 0;
}
static void charge_reset_hiz_state(void)
{
	struct charge_device_info *di = g_di;
	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}
	mutex_lock(&di->mutex_hiz);
	di->hiz_ref = 0;
	hwlog_info("[%s] charger disconnect, clear hiz_refcnt\n", __func__);
	mutex_unlock(&di->mutex_hiz);
}
static void fcp_reg_dump(char* pstr)
{
	struct charge_device_info *di = g_di;
	unsigned int vbus = 0;
	int ibus = 0;
	char buf[ERR_NO_STRING_SIZE] = {0};

	if (!di || !di->fcp_ops || !di->fcp_ops->reg_dump) {
		hwlog_err("%s ops is null\n", __func__);
	} else {
		di->fcp_ops->reg_dump(pstr);
	}

	if(NULL != di && NULL != di->ops){
		if(di->ops->get_ibus){
			ibus = di->ops->get_ibus();
		}
		if(di->ops->get_vbus){
			di->ops->get_vbus(&vbus);
		}
	}

	snprintf(buf, sizeof(buf)-1, "vbus = %dmV, ibus = %dmA\n", vbus, ibus);
	strncat(pstr,buf,strlen(buf));
}

/**********************************************************
*  Function:       charger_dsm_report
*  Description:    charger dsm report
*  Parameters:   err_no val
*  return value:  0:succ ;-1 :fail
**********************************************************/
int charger_dsm_report(int err_no, int *val)
{
	int ret = -1;
#if defined CONFIG_HUAWEI_DSM
	char dsm_buff[CHARGE_DMDLOG_SIZE] = { 0 };
	char buf[ERR_NO_STRING_SIZE] = { 0 };
	int i;
	int err_count_size = sizeof(err_count)/sizeof(struct charger_dsm);

	for (i = 0; i < err_count_size; i++) {
		if ((err_no == err_count[i].error_no) &&
			(true == err_count[i].notify_enable)) {/*every err_no report one times */
			strncat(dsm_buff, err_count[i].buf, ERR_NO_STRING_SIZE - 1);
			if (val) {  /*need report reg */
				snprintf(buf, sizeof(buf), "val= %d\n", *val);
				strncat(dsm_buff, buf, strlen(buf));
			}
			if (err_count[i].dump)
				err_count[i].dump(dsm_buff);
			if (!power_dsm_dmd_report(POWER_DSM_BATTERY, err_no, dsm_buff)) {
				/*when it be set 1,it will not report */
				if(false == err_count[i].notify_always){
					err_count[i].notify_enable = false;
				}
				ret = 0;
				break;
			}
		}
	}
#endif
	return ret;
}

static int dump_bootloader_info(char *reg_value)
{
	char buff[26] = { 0 };
	int i = 0;

	memset(reg_value, 0, CHARGELOG_SIZE);
	snprintf(buff, 26, "%-8.2d", hisi_charger_info.ibus);
	strncat(reg_value, buff, strlen(buff));
	for (i = 0; i < REG_NUM; i++) {
		snprintf(buff, 26, "0x%-8.2x", hisi_charger_info.reg[i]);
		strncat(reg_value, buff, strlen(buff));
	}
	return 0;
}

static int copy_bootloader_charger_info(void)
{
	char *p = NULL;
	p = get_charger_info_p();

	if (NULL == p) {
		hwlog_err("bootloader pointer NULL!\n");
		return -1;
	}

	memcpy(&hisi_charger_info, p + CHARGER_BASE_ADDR,
	       sizeof(hisi_charger_info));
	hwlog_info("bootloader ibus %d\n", hisi_charger_info.ibus);

	return 0;
}
/**********************************************************
*  Function:       fcp_check_switch_status
*  Description:    check switch chip status
*  Parameters:  void
*  return value:  void
**********************************************************/

/*lint -save -e* */
static void fcp_check_switch_status(struct charge_device_info *di)
{
	int val = -1;
	int reg = 0;
	int ret = -1;

	/*check usb is on or not ,if not ,can not detect the switch status */
	if (di->ops->get_charge_state) {
		ret = di->ops->get_charge_state(&reg);
		if (ret) {
			hwlog_info("%s:read PG STAT fail.\n", __func__);
			return;
		}
		if (reg & CHAGRE_STATE_NOT_PG) {
			hwlog_info
			    ("%s:PG NOT GOOD can not check switch status.\n", __func__);
			return;
		}
	}

	if (!di->fcp_ops->fcp_read_switch_status) {
		hwlog_err("di->ops->fcp_read_switch_status is null.\n");
		return;
	}

	val = di->fcp_ops->fcp_read_switch_status();
	if (val) {
		switch_status_num = switch_status_num + 1;
	} else {
		switch_status_num = 0;
	}
	if (VBUS_REPORT_NUM <= switch_status_num) {
		switch_status_num = 0;
		charger_dsm_report(ERROR_SWITCH_ATTACH, NULL);
	}
}

/**********************************************************
*  Function:       fcp_check_adapter_status
*  Description:    check adapter status
*  Parameters:     void
*  return value:  void
**********************************************************/
static void fcp_check_adapter_status(struct charge_device_info *di)
{
	int val = -1;
	int reg = 0;
	int ret = -1;

	/*check usb is on or not ,if not ,can not detect the switch status */
	if (di->ops->get_charge_state) {
		ret = di->ops->get_charge_state(&reg);
		if (ret) {
			hwlog_info("%s:read PG STAT fail.\n", __func__);
			return;
		}
		if (reg & CHAGRE_STATE_NOT_PG) {
			hwlog_info
			    ("%s:PG NOT GOOD can not check adapter status.\n", __func__);
			return;
		}
	}

	if (!di->fcp_ops->fcp_read_adapter_status) {
		hwlog_err("di->ops->fcp_read_adapter_status is null.\n");
		return;
	}
	val = di->fcp_ops->fcp_read_adapter_status();

	if (FCP_ADAPTER_OVLT == val) {
		charger_dsm_report(ERROR_ADAPTER_OVLT, NULL);
	}

	if (FCP_ADAPTER_OCURRENT == val) {
		charger_dsm_report(ERROR_ADAPTER_OCCURRENT, NULL);
	}

	if (FCP_ADAPTER_OTEMP == val) {
		charger_dsm_report(ERROR_ADAPTER_OTEMP, NULL);
	}
}

ATOMIC_NOTIFIER_HEAD(fault_notifier_list);
/*lint -restore*/
#ifdef CONFIG_WIRELESS_CHARGER
void charge_set_input_current_prop(int iin_step, int iin_delay)
{
	input_current_delay = iin_delay;
	input_current_step = iin_step;
	input_current_now = CHARGE_CURRENT_0100_MA;

	hwlog_info("[%s] iin_delay = %d iin_step = %d\n",
		__func__, input_current_delay, input_current_step);
}
static void charge_set_iin_step_by_step(struct charge_device_info *di, int iin)
{
	int ret;
	int iin_tmp = input_current_now;
	iin_set_complete_flag = 0;

	if (iin > input_current_now) {
		do {
			ret = di->ops->set_input_current(iin_tmp);
			if (ret)
				hwlog_err("set input current %d fail!\n", iin_tmp);
			input_current_now = iin_tmp;
			iin_tmp += input_current_step;
			if (!iin_set_complete_flag)
				msleep(input_current_delay);
		} while (iin_tmp < iin && !iin_set_complete_flag
				&&input_current_delay && input_current_step);
	}
	if (iin < input_current_now) {
		do {
			ret = di->ops->set_input_current(iin_tmp);
			if (ret)
				hwlog_err("set input current %d fail!\n", iin_tmp);
			input_current_now = iin_tmp;
			iin_tmp -= input_current_step;
			if (!iin_set_complete_flag)
				msleep(input_current_delay);
		} while (iin_tmp > iin && !iin_set_complete_flag
				&& input_current_delay && input_current_step);
	}
	if (!iin_set_complete_flag) {
		input_current_now = iin;
		ret = di->ops->set_input_current(iin);
		if (ret)
			hwlog_err("set input current %d fail!\n", iin);
		iin_set_complete_flag = 1;
	}
}
static void wireless_tx_limit_input_current(struct charge_device_info *di)
{
	unsigned int tx_iin = wireless_tx_get_tx_iin_limit(di->charger_type);
	if (!tx_iin) {
		di->iin_limit = 0;
		return;
	}
	switch (di->charger_type) {
		case CHARGER_TYPE_FCP:
		case CHARGER_TYPE_STANDARD:
		case CHARGER_TYPE_PD:
			di->iin_limit = di->core_data->iin_ac - tx_iin;
			break;
		case CHARGER_TYPE_NON_STANDARD:
			di->iin_limit = di->core_data->iin_nonstd - tx_iin;
			break;
		case CHARGER_TYPE_BC_USB:
			di->iin_limit = di->core_data->iin_bc_usb - tx_iin;
			break;
		default:
			di->iin_limit = 0;
			break;
	}
}
#endif
int charge_set_input_current(int iin)
{
	struct charge_device_info *di = g_di;
	int ret;

	if (NULL == di || NULL == di->ops || NULL == di->ops->set_input_current) {
		hwlog_err("%s:di ops null!\n",__func__);
		return -1;
	}

	iin = min(iin, di->input_current);

#ifdef CONFIG_WIRELESS_CHARGER
	if (input_current_delay && input_current_step) {
		iin = min(iin, wireless_charge_get_rx_iout_limit());
		if (!iin_set_complete_flag) {
			iin_set_complete_flag = 1;
			msleep(2*input_current_delay); //double time, only used here
		}
		charge_set_iin_step_by_step(di, iin);
	} else {
#endif
		hwlog_info("%s:set_input_current = %d!\n",__func__,iin);
		ret = di->ops->set_input_current(iin);
		if (ret) {
			hwlog_err("set input current %d fail!\n", iin);
			return ret;
		}
#ifdef CONFIG_WIRELESS_CHARGER
	}
#endif

	return 0;
}
int charge_set_input_current_max(void)
{
	struct charge_device_info *di = g_di;
	int ret;

	if (NULL == di || NULL == di->ops || NULL == di->ops->set_input_current) {
		hwlog_err("%s:di ops null!\n", __func__);
		return -1;
	}

	di->input_current = di->core_data->iin_max;
	ret = di->ops->set_input_current(di->input_current);
	if (ret) {
		hwlog_err("%s:set input current %d fail!\n", __func__, di->input_current);
		return -1;
	}
	hwlog_info("%s:set input current %d succ!\n", __func__, di->input_current);
	return 0;
}
int charge_get_charger_iinlim_regval(void)
{
	struct charge_device_info *di = g_di;

	if (NULL == di || NULL == di->ops || NULL == di->ops->get_iin_set) {
		hwlog_err("%s:di ops null!\n",__func__);
		return CHARGE_CURRENT_0500_MA;
	}
	return di->ops->get_iin_set();
}
/**********************************************************
*  Function:       charge_rename_charger_type
*  Description:    rename the charger_type from USB PHY to charger
*  Parameters:   type:charger type from USB PHY
*                      di:charge_device_info
*                      update_flag
*  return value:  true:notify work /false:not notify work
**********************************************************/
static int charge_rename_charger_type(enum hisi_charger_type type,
				       struct charge_device_info *di ,bool update_flag)
{
	int ret = TRUE;
	hwlog_info("%s:type = %d, last di->charger_type = %d\n",__func__,type, di->charger_type);
	switch (type) {
	case CHARGER_TYPE_SDP:
		if(CHARGER_REMOVED == di->charger_type || CHARGER_TYPE_NON_STANDARD == di->charger_type
			|| CHARGER_TYPE_USB == di->charger_type ||CHARGER_TYPE_WIRELESS == di->charger_type)
		{
			di->charger_type = CHARGER_TYPE_USB;
			di->charger_source = POWER_SUPPLY_TYPE_USB;
			ret = FALSE;
		}
		break;
	case CHARGER_TYPE_CDP:
		if(CHARGER_REMOVED == di->charger_type || CHARGER_TYPE_NON_STANDARD == di->charger_type
			||CHARGER_TYPE_WIRELESS == di->charger_type || charger_type_update)
		{
			di->charger_type = CHARGER_TYPE_BC_USB;
			di->charger_source = POWER_SUPPLY_TYPE_USB;
			charger_type_update = FALSE;
			ret = FALSE;
		}
		break;
	case CHARGER_TYPE_DCP:
		if (CHARGER_TYPE_STANDARD != di->charger_type) {
			di->charger_type = CHARGER_TYPE_STANDARD;
			di->charger_source = POWER_SUPPLY_TYPE_MAINS;
			if (1 == di->support_standard_ico)
				ico_enable = 1;
			else
				ico_enable = 0;
			ret = FALSE;
		}
		break;
	case CHARGER_TYPE_UNKNOWN:
		if(CHARGER_REMOVED == di->charger_type ||CHARGER_TYPE_WIRELESS == di->charger_type || CHARGER_TYPE_USB == di->charger_type
			|| charger_type_update)
		{
			di->charger_type = CHARGER_TYPE_NON_STANDARD;
			di->charger_source = POWER_SUPPLY_TYPE_MAINS;
			charger_type_update = FALSE;
			ret = FALSE;
		}
		break;
	case CHARGER_TYPE_NONE:
		if(CHARGER_REMOVED != di->charger_type)
		{
			di->charger_type = CHARGER_REMOVED;
			di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
		}
		ret = FALSE;
		break;
	case PLEASE_PROVIDE_POWER:
		if(CHARGER_REMOVED == di->charger_type)
		{
			di->charger_type = USB_EVENT_OTG_ID;
			di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
			ret = FALSE;
		}
		break;
	case CHARGER_TYPE_WIRELESS:
		di->charger_type = CHARGER_TYPE_WIRELESS;
		di->charger_source = POWER_SUPPLY_TYPE_MAINS;
		ret = FALSE;
		break;
	default:
		di->charger_type = CHARGER_REMOVED;
		di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
		ret = FALSE;
		break;
	}
	return ret;
}

/**********************************************************
*  Function:       charge_update_charger_type
*  Description:    update charger_type from fsa9685 when the charger_type is CHARGER_TYPE_NON_STANDARD
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void charge_update_charger_type(struct charge_device_info *di)
{
	enum hisi_charger_type type = CHARGER_TYPE_NONE;

	if (di->charger_type != CHARGER_TYPE_NON_STANDARD)
		return;

	nonstand_detect_times++;

	if(di->sw_ops && di->sw_ops->get_charger_type) {
		type = di->sw_ops->get_charger_type();
		if (1 == usbswitch_common_dcd_timeout_status()) {
			if (CHARGER_TYPE_DCP == type) {
				charge_rename_charger_type(type, di, TRUE);
				hwlog_info("[%s]charger type is update to DCP from nonstd charger! type is [%d]\n", __func__, di->charger_type);
			}
		} else {
			if (type != CHARGER_TYPE_NONE) {
				charge_rename_charger_type(type, di,TRUE);
				hwlog_info("[%s]charger type is update to[%d] from nonstd charger!\n", __func__, di->charger_type);
			}
		}
	}
}

static void charger_event_notify(int event)
{
	blocking_notifier_call_chain(&charger_event_notify_head, event, NULL);
}

/**********************************************************
*  Function:       charge_send_uevent
*  Discription:    send charge uevent immediately after charger type is recognized
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void charge_send_uevent(struct charge_device_info *di)
{
	/*send events */
	enum charge_status_event events;
	static bool wired_vbus_flag = true;
	if (di == NULL) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return;
	}

	if (di->charger_source == POWER_SUPPLY_TYPE_MAINS) {
		events = VCHRG_START_AC_CHARGING_EVENT;
		charger_event_notify(CHARGER_START_CHARGING_EVENT);
		hisi_coul_charger_event_rcv(events);
	} else if (di->charger_source == POWER_SUPPLY_TYPE_USB) {
		events = VCHRG_START_USB_CHARGING_EVENT;
		charger_event_notify(CHARGER_START_CHARGING_EVENT);
		hisi_coul_charger_event_rcv(events);
	} else if (di->charger_source == POWER_SUPPLY_TYPE_BATTERY) {
		events = VCHRG_STOP_CHARGING_EVENT;
		charger_event_notify(CHARGER_STOP_CHARGING_EVENT);
		di->current_full_status = 0;
		charge_reset_hiz_state();
#ifdef CONFIG_DIRECT_CHARGER
		set_direct_charger_disable_flags
			(DIRECT_CHARGER_CLEAR_DISABLE_FLAGS, DIRECT_CHARGER_WIRELESS_TX);
#endif
		hisi_coul_charger_event_rcv(events);
	} else {
		hwlog_err("[%s]error charger source!\n", __func__);
		/*do nothing*/
	}
#ifdef CONFIG_WIRELESS_CHARGER
	if (di->charger_type == CHARGER_REMOVED) {
		if (true == wired_vbus_flag)
			wireless_charge_wired_vbus_disconnect_handler();
		wired_vbus_flag = false;
	} else if (di->charger_type != CHARGER_TYPE_WIRELESS) {
		wireless_charge_wired_vbus_connect_handler();
		wired_vbus_flag = true;
	}
#endif
}

static enum fcp_check_stage_type fcp_get_stage(void)
{
	return fcp_stage;
}

/**********************************************************
*  Function:       fcp_set_stage_status
*  Description:    set the stage of fcp charge
*  Parameters:     di:charge_device_info
*  return value:   NULL
**********************************************************/
void fcp_set_stage_status(enum fcp_check_stage_type stage_type)
{
	fcp_stage = stage_type;
}

int get_fcp_charging_flag(void)
{
	return fcp_charge_flag;
}

void set_fcp_charging_flag(int val)
{
	fcp_charge_flag = val;
}

/**********************************************************
*  Function:       fcp_retry_pre_operate
*  Discription:    pre operate before retry fcp enable
*  Parameters:   di:charge_device_info,type : enum fcp_retry_operate_type
*  return value:  0: fcp pre operate success
*                     -1:fcp pre operate fail
**********************************************************/

/*lint -save -e* */
static int fcp_retry_pre_operate(enum fcp_retry_operate_type type,
				 struct charge_device_info *di)
{
	int pg_state = 0, ret = -1;
	ret = di->ops->get_charge_state(&pg_state);
	if (ret < 0) {
		hwlog_err("get_charge_state fail!!ret = 0x%x\n", ret);
		return ret;
	}
	/*check status charger not power good state */
	if (pg_state & CHAGRE_STATE_NOT_PG) {
		hwlog_err("state is not power good \n");
		return -1;
	}
	switch (type) {
	case FCP_RETRY_OPERATE_RESET_ADAPTER:
		if (NULL != di->fcp_ops->fcp_adapter_reset) {
			hwlog_info("send fcp adapter reset cmd \n");
			ret = di->fcp_ops->fcp_adapter_reset();
		} else {
			ret = -1;
		}
		break;
	case FCP_RETRY_OPERATE_RESET_SWITCH:
		if (NULL != di->fcp_ops->switch_chip_reset) {
			hwlog_info(" switch_chip_reset \n");
			ret = di->fcp_ops->switch_chip_reset();
			msleep(2000);
		} else {
			ret = -1;
		}
		break;
	default:
		break;
	}
	return ret;
}
/*lint -restore*/

/**********************************************************
*  Function:       fcp_start_charging
*  Description:    enter fcp charging mode
*  Parameters:   di:charge_device_info
*  return value:  0: fcp start success
*                    -1:fcp start fail
**********************************************************/

/*lint -save -e* */
static int fcp_start_charging(struct charge_device_info *di)
{
	int ret = -1;
	int output_vol = 0;
	struct chip_init_crit init_crit;
#ifdef CONFIG_DIRECT_CHARGER
	int max_retry_num = 2;
	static int fcp_output_vol_retry_cnt = 0;
#endif
	fcp_set_stage_status(FCP_STAGE_SUPPORT_DETECT);
	if ((NULL == di->fcp_ops) || (NULL == di->fcp_ops->is_support_fcp)
	    || (NULL == di->fcp_ops->detect_adapter)
	    || (NULL == di->fcp_ops->set_adapter_output_vol)
	    || (NULL == di->fcp_ops->get_adapter_output_current)) {
		hwlog_err("fcp ops is NULL!\n");
		return -1;
	}
	/*check whether support fcp detect */
	if (di->fcp_ops->is_support_fcp()) {
		hwlog_err("not support fcp!\n");
		return -1;
	}
	/*To avoid to effect accp detect , input current need to be lower than 1A,we set 0.5A */
	di->input_current = CHARGE_CURRENT_0500_MA;
	charge_set_input_current(di->input_current);

	/*detect fcp adapter */
	fcp_set_stage_status(FCP_STAGE_ADAPTER_DETECT);
	ret = di->fcp_ops->detect_adapter();
#ifdef CONFIG_DIRECT_CHARGER
	if (!is_direct_charge_failed()) {
		fcp_output_vol_retry_cnt = 0;
#endif
		if (FCP_ADAPTER_DETECT_SUCC != ret) {
			hwlog_err("fcp detect fail!\n");
			if (FCP_ADAPTER_DETECT_FAIL == ret) {
				return 1;
			}
			return -1;
		}
#ifdef CONFIG_DIRECT_CHARGER
	}
	if (fcp_output_vol_retry_cnt < max_retry_num) {
		fcp_output_vol_retry_cnt++;
	} else {
		return -1;
	}
#endif
	chg_set_adaptor_test_result(TYPE_FCP,DETECT_SUCC);
	fcp_set_stage_status(FCP_STAGE_ADAPTER_ENABLE);
	init_crit.vbus = ADAPTER_9V;
	di->ops->chip_init(&init_crit);

	/*set fcp adapter output vol */
	if (di->fcp_ops->set_adapter_output_vol(&output_vol)) {
		init_crit.vbus = ADAPTER_5V;
		di->ops->chip_init(&init_crit);
		hwlog_err("fcp set vol fail!\n");
		ret = di->fcp_ops->switch_chip_reset();
		return 1;
	}
	hwlog_info("output vol = %d\n", output_vol);

	if (di->ops->set_vbus_vset) {
		ret = di->ops->set_vbus_vset(output_vol);
		if (ret) {
			hwlog_err("set vbus_vset fail!\n");
		}
	}
#ifdef CONFIG_DIRECT_CHARGER
	fcp_output_vol_retry_cnt = 0;
#endif
	if (cancel_work_flag) {
		fcp_set_stage_status(FCP_STAGE_DEFAUTL);
		hwlog_info("[%s] charge already stop\n", __func__);
		return 0;
	}
	chg_set_adaptor_test_result(TYPE_FCP, PROTOCOL_FINISH_SUCC);
#ifdef CONFIG_DIRECT_CHARGER
	direct_charge_send_quick_charge_uevent();
#endif
	di->charger_type = CHARGER_TYPE_FCP;
	fcp_set_stage_status(FCP_STAGE_SUCESS);
	set_fcp_charging_flag(TRUE);
	msleep(CHIP_RESP_TIME);
	hwlog_info("fcp charging start success!\n");
	return 0;
}
/*lint -restore*/

/**********************************************************
*  Function:        charge_vbus_voltage_check
*  Description:     check whether the voltage of vbus is normal
*  Parameters:   di:charge_device_info
*  return value:   NULL
**********************************************************/

/*lint -save -e* */
static void charge_vbus_voltage_check(struct charge_device_info *di)
{
	int ret = 0;
	unsigned int vbus_vol = 0, vbus_ovp_cnt = 0;
	int i = 0;
	unsigned int state = CHAGRE_STATE_NORMAL;

	if (NULL == di || NULL == di->ops || NULL == di->ops->get_vbus)
		return;

	if (di->ops->set_covn_start)
		ret = di->ops->set_covn_start(true);

	if (ret) {
		hwlog_err("[%s]set covn start fail.\n", __func__);
		return;
	}
	ret = di->ops->get_charge_state(&state);
	if (ret < 0) {
		hwlog_err("get_charge_state fail!!ret = 0x%x\n", ret);
		return;
	}

	for (i = 0; i < VBUS_VOL_READ_CNT; ++i) {
		ret = di->ops->get_vbus(&vbus_vol);
		if (ret) {
			hwlog_err("vbus vol read fail.\n");
		}
		hwlog_info("vbus vbus_vol:%u.\n", vbus_vol);

		if (vbus_vol > VBUS_VOLTAGE_13400_MV) {
			if (!(state & CHAGRE_STATE_NOT_PG)) {
				vbus_ovp_cnt++;//if power ok, then count plus one.
			}
			msleep(25);//Wait for chargerIC to be in stable state!
		} else {
			break;
		}
	}
	if (vbus_ovp_cnt == VBUS_VOL_READ_CNT) {
		hwlog_err("[%s]vbus_vol = %u.\n", __func__, vbus_vol);
		charger_dsm_report(ERROR_VBUS_VOL_OVER_13400MV, &vbus_vol);
	}
	if (CHARGER_TYPE_PD == di->charger_type) {
#ifdef CONFIG_TCPC_CLASS
		if (!pd_charge_flag)
			return;
		if (pd_dpm_get_high_voltage_charging_status()
				&& vbus_vol > VBUS_VOLTAGE_7000_MV) {
			if (pd_dpm_get_pd_reset_adapter() == PD_ADAPTER_9V) {
				last_vset = PD_ADAPTER_9V;
			} else {
				ret = pd_dpm_set_voltage(PD_ADAPTER_5V);
				if (!ret) {
					hwlog_err("[%s]set voltage failed \n", __func__);
				}
			}
			pd_vbus_abnormal_cnt = 0;
			return;
		}
		if (!pd_dpm_get_high_voltage_charging_status()
				&& vbus_vol < VBUS_VOLTAGE_7000_MV) {
			if (!pd_dpm_get_optional_max_power_status()
				|| pd_dpm_get_pd_reset_adapter() == PD_ADAPTER_5V) {
				last_vset = PD_ADAPTER_5V;
			} else {
				ret = pd_dpm_set_voltage(PD_ADAPTER_9V);
				if (!ret) {
					hwlog_err("[%s]set voltage failed \n", __func__);
				}
			}
			pd_vbus_abnormal_cnt = 0;
			return;
		}
		if (++pd_vbus_abnormal_cnt >= VBUS_VOLTAGE_ABNORMAL_MAX_COUNT) {
			hwlog_err("%s: pd_vbus_abnormal_cnt = %d, hard_reset\n",
				__func__, VBUS_VOLTAGE_ABNORMAL_MAX_COUNT);
			pd_dpm_hard_reset();
		}
		return;
#endif
	}
#ifdef CONFIG_WIRELESS_CHARGER
	if (di->charger_type == CHARGER_TYPE_WIRELESS) {
		return;
	}
#endif
	if (FCP_STAGE_SUCESS == fcp_get_stage_status()) {
		/* fcp stage : vbus must be higher than 7000 mV */
		if (vbus_vol < VBUS_VOLTAGE_7000_MV) {
			fcp_vbus_lower_count += 1;
			hwlog_err
			    ("[%s]fcp output vol =%d mV, lower 7000 mV , fcp_vbus_lower_count =%d!!\n",
			     __func__, vbus_vol, fcp_vbus_lower_count);
		} else {
			fcp_vbus_lower_count = 0;
		}
		/* check continuous abnormal vbus cout  */
		if (fcp_vbus_lower_count >= VBUS_VOLTAGE_ABNORMAL_MAX_COUNT) {
			vbus_flag = vbus_flag + 1;
			fcp_check_adapter_status(di);
			fcp_set_stage_status(FCP_STAGE_DEFAUTL);
			di->charger_type = CHARGER_TYPE_STANDARD;
			if (di->fcp_ops->fcp_adapter_reset && di->fcp_ops->fcp_adapter_reset()) {
				hwlog_err("adapter reset failed \n");
			}
			if (di->ops->set_vbus_vset) {
				ret = di->ops->set_vbus_vset(ADAPTER_5V);
				if(ret)
					hwlog_err("set vbus_vset fail!\n");
			}
			fcp_vbus_lower_count = VBUS_VOLTAGE_ABNORMAL_MAX_COUNT;
		}
		if (VBUS_REPORT_NUM <= vbus_flag) {
			vbus_flag = 0;
			charger_dsm_report(ERROR_FCP_VOL_OVER_HIGH, NULL);
		}
		nonfcp_vbus_higher_count = 0;
	} else {
		/*non fcp stage : vbus must be lower than 6500 mV */
		if (vbus_vol > VBUS_VOLTAGE_6500_MV) {
			nonfcp_vbus_higher_count += 1;
			hwlog_info
			    ("[%s]non standard fcp and vbus voltage is %d mv,over 6500mv ,nonfcp_vbus_higher_count =%d!!\n",
			     __func__, vbus_vol, nonfcp_vbus_higher_count);
		} else {
			nonfcp_vbus_higher_count = 0;
		}
		/* check continuous abnormal vbus cout  */
		if (nonfcp_vbus_higher_count >= VBUS_VOLTAGE_ABNORMAL_MAX_COUNT) {
			di->charge_enable = FALSE;
			nonfcp_vbus_higher_count =
			    VBUS_VOLTAGE_ABNORMAL_MAX_COUNT;
			charger_dsm_report(ERROR_FCP_VOL_OVER_HIGH, &vbus_vol);
			if (di->fcp_ops && di->fcp_ops->is_fcp_charger_type
			    && di->fcp_ops->is_fcp_charger_type()) {
				if (di->fcp_ops->fcp_adapter_reset && di->fcp_ops->fcp_adapter_reset()) {
					hwlog_err("adapter reset failed \n");
				}

				hwlog_info("[%s]is fcp adapter!!\n", __func__);
			} else {
				hwlog_info("[%s] is not fcp adapter!!\n", __func__);
			}
		}
		fcp_vbus_lower_count = 0;
	}
}
static void pd_charge_check(struct charge_device_info *di)
{
#ifdef CONFIG_TCPC_CLASS
	int ret = 0, i = 0;
	struct chip_init_crit init_crit;

	if (pd_charge_flag)
		return;
	if (cancel_work_flag) {
		hwlog_info("[%s] charge already stop\n", __func__);
		return;
	}

	if (di->charger_type != CHARGER_TYPE_PD
	    || !(is_hisi_battery_exist())) {
		return;
	}

	if (true == pd_dpm_get_high_power_charging_status()) {
		init_crit.vbus = ADAPTER_9V;
	} else {
		init_crit.vbus = ADAPTER_5V;
	}
	di->ops->chip_init(&init_crit);

	if (di->ops->set_vbus_vset) {
		if (true == pd_dpm_get_high_power_charging_status())
			ret = di->ops->set_vbus_vset(ADAPTER_9V);
		else
			ret = di->ops->set_vbus_vset(ADAPTER_5V);

		if (ret) {
			hwlog_err("set vbus_vset fail!\n");
		}
	}

	hwlog_info("%s :  ok \n", __func__);
	pd_charge_flag = true;
#endif
}
/*lint -restore*/

/**********************************************************
*  Function:       fcp_charge_check
*  Description:    check whether start fcp charging,if support try to start fcp charging
*  Parameters:     di:charge_device_info
*  return value:   NULL
**********************************************************/
static void fcp_charge_check(struct charge_device_info *di)
{
	int ret = 0, i = 0;

	if (cancel_work_flag) {
		hwlog_info("[%s] charge already stop\n", __func__);
		return;
	}

	if (FCP_STAGE_SUCESS == fcp_get_stage_status())
		fcp_check_switch_status(di);

	if ((di->charger_type != CHARGER_TYPE_STANDARD
		&& di->charger_type != CHARGER_TYPE_FCP)
	    || !(is_hisi_battery_exist())) {
		return;
	}
#ifdef CONFIG_DIRECT_CHARGER
	if (is_direct_charge_failed() && (FCP_STAGE_SUCESS > fcp_get_stage_status())) {
		fcp_set_stage_status(FCP_STAGE_DEFAUTL);
	}
#endif
	if(get_pd_charge_flag() == true){
		msleep(FCP_DETECT_DELAY_IN_POWEROFF_CHARGE);
	}
	if ((FCP_STAGE_DEFAUTL == fcp_get_stage_status() && !(di->reset_adapter & (1 << RESET_ADAPTER_WIRELESS_TX)))
		||(FCP_STAGE_RESET_ADAPTOR == fcp_get_stage_status() && !di->reset_adapter)) {
		ret = fcp_start_charging(di);
		for (i = 0; i < 3 && ret == 1; i++) {
			/* reset adapter and try again */
			if ((fcp_retry_pre_operate(FCP_RETRY_OPERATE_RESET_ADAPTER, di)) < 0) {
				hwlog_err("reset adapter failed \n");
				break;
			}
			ret = fcp_start_charging(di);
		}
		if (ret == 1) {
			/* reset fsa9688 chip and try again */
			if ((fcp_retry_pre_operate(FCP_RETRY_OPERATE_RESET_SWITCH, di)) == 0) {
				ret = fcp_start_charging(di);
			} else {
				hwlog_err
				    ("%s : fcp_retry_pre_operate failed \n", __func__);
			}
		}

		if (ret == 1) {
			if (FCP_STAGE_ADAPTER_ENABLE == fcp_get_stage_status())
				output_num = output_num + 1;
			else
				output_num = 0;

			if (VBUS_REPORT_NUM <= output_num) {
#ifdef CONFIG_DIRECT_CHARGER
				if (!is_direct_charge_failed())
#endif
					charger_dsm_report(ERROR_FCP_OUTPUT, NULL);
			}
			if (FCP_STAGE_ADAPTER_DETECT == fcp_get_stage_status())
				detect_num = detect_num + 1;
			else
				detect_num = 0;

			if (VBUS_REPORT_NUM <= detect_num) {
				charger_dsm_report(ERROR_FCP_DETECT, NULL);
			}
			if(VBUS_REPORT_NUM > output_num && VBUS_REPORT_NUM > detect_num)
				fcp_set_stage_status(FCP_STAGE_DEFAUTL);
		}

		hwlog_info("[%s]fcp stage  %s !!! \n", __func__,
			   fcp_check_stage[fcp_get_stage_status()]);
	}

	if(di->reset_adapter && FCP_STAGE_SUCESS == fcp_get_stage_status()
		&& NULL != di->fcp_ops->fcp_adapter_reset) {
		if (di->fcp_ops->fcp_adapter_reset()) {
			hwlog_err("adapter reset failed \n");
			return;
		}
		hwlog_info("reset adapter by user\n");
		fcp_set_stage_status(FCP_STAGE_RESET_ADAPTOR);
		if (di->ops->set_vbus_vset) {
			ret = di->ops->set_vbus_vset(ADAPTER_5V);
			if(ret)
				hwlog_err("set vbus_vset fail!\n");
		}
		if (cancel_work_flag) {
			hwlog_info("[%s] charge already stop\n", __func__);
			return;
		}
		di->charger_type = CHARGER_TYPE_STANDARD;
		msleep(CHIP_RESP_TIME);
	}
}
/****************************************************************************
  Function:     fcp_test_is_support
  Description:  check if fcp is support
  Input:        NA
  Output:      NA
  Return:      0: success
               other: fail
***************************************************************************/
int fcp_test_is_support(void)
{
	if(g_fcp_ops && g_fcp_ops->is_support_fcp){
		return g_fcp_ops->is_support_fcp();
	}else{
		hwlog_info("fcp detect fail \n");
		return -1;
	}
}

/****************************************************************************
  Function:     fcp_test_detect_adapter
  Description:  check if fcp adapter detect is true
  Input:        NA
  Output:      NA
  Return:      0: success
               -1: other fail
               1:fcp adapter but detect fail
***************************************************************************/
int fcp_test_detect_adapter(void)
{
	if(g_fcp_ops && g_fcp_ops->detect_adapter){
		return g_fcp_ops->detect_adapter();
	}else{
		hwlog_info("fcp detect fail \n");
		return -1;
	}
}

/**********************************************************
*  Function:       fcp_support_show
*  Discription:    file node for mmi testing fast charge protocol
*  Parameters:     NA
*  return value:   0:success
*                       1:fail
*                       2:not support
**********************************************************/
static int fcp_support_show(void)
{
	int result = FCP_TEST_FAIL;
	enum hisi_charger_type type = hisi_get_charger_type();

	/* judge whether support fcp */
	if(fcp_test_is_support())
	{
		result = FCP_NOT_SUPPORT;
		hwlog_err("fcp support fail\n");
		goto fcp_test_done;
	}
	/*to avoid the usb port cutoff when CTS test*/
	if ((type == CHARGER_TYPE_SDP) || (type == CHARGER_TYPE_CDP))
	{
		result = FCP_TEST_FAIL;
		hwlog_err("fcp detect fail 1,charge type is %d \n",type);
		goto fcp_test_done;
	}
	/* fcp adapter detect */
	if(fcp_test_detect_adapter()){
		hwlog_err("fcp detect fail 2,charge type is %d \n",type);
		result = FCP_TEST_FAIL;
	}else{
		result = FCP_TEST_SUCC;
	}

fcp_test_done:
	hwlog_info("%s: fcp test result %d\n",__func__,result);
	return result;
}

/**********************************************************
*  Function:       chg_set_adaptor_test_result
*  Discription:    set special adaptor protocol test result
*  Parameters:    charger_type: adaptor to be test
*                       result: test result
*  return value:   NULL
**********************************************************/
void chg_set_adaptor_test_result(enum adaptor_name charger_type, enum test_state result)
{
	int i = 0;//init the table index to zero
	int adt_test_tbl_len = 0;//init the table length to zero

	adt_test_tbl_len = sizeof(adptor_test_tbl)/(sizeof(adptor_test_tbl[0]));
	for(i = 0; i < adt_test_tbl_len; i++){
		if(adptor_test_tbl[i].charger_type == charger_type){
			adptor_test_tbl[i].result = result;
			break;
		}
	}
	if(i == adt_test_tbl_len){
		hwlog_err("adaptor type is out of range!\n");
	}
}

/**********************************************************
*  Function:        chg_get_adaptor_test_result
*  Discription:     get special adaptor protocol test result
*  Parameters:     buf: target string buffer for saving the result
*  return value:   negtave: fail to copy the result
*                       positive: the total number success writen
**********************************************************/
static int chg_get_adaptor_test_result(char* buf)
{
	int i = 0;//init the table index to zero
	int adt_test_tbl_len = 0;//init the table length to zero
	int succ_char_sum = 0;//init the return val to 0
	int real_num_read = 0;//init the number to write to 0
	char temp_buf[TMEP_BUF_LEN] = {0};//init temp buffer to null
	int local_mode= 0;

	if(NULL == buf){
		return INVALID_RET_VAL;
	}
#ifdef CONFIG_DIRECT_CHARGER
	local_mode = direct_charge_get_local_mode();
	adt_test_tbl_len = sizeof(adptor_test_tbl)/(sizeof(adptor_test_tbl[0]));
	for(i = 0; i < adt_test_tbl_len; i++){
		if( !(local_mode & SC_MODE) && (adptor_test_tbl[i].charger_type == TYPE_SC))
		{
			continue;
		}
		if(i != adt_test_tbl_len-1){
			succ_char_sum += snprintf(temp_buf, TMEP_BUF_LEN, "%s:%d,", adptor_test_tbl[i].adaptor_str_name, adptor_test_tbl[i].result);
			strncat(buf,temp_buf,strlen(temp_buf));
		}else{
			succ_char_sum += snprintf(temp_buf, TMEP_BUF_LEN, "%s:%d\n", adptor_test_tbl[i].adaptor_str_name, adptor_test_tbl[i].result);
			strncat(buf,temp_buf,strlen(temp_buf));
		}
		real_num_read += (strlen(adptor_test_tbl[i].adaptor_str_name) + POSTFIX_LEN);
		if(succ_char_sum != real_num_read){
			succ_char_sum = INVALID_RET_VAL;
			break;
		}
	}
	buf[strlen(buf)-1] = '\n';
	hwlog_info("succ_writen_char = %d, real_to_write = %d, buf = %s\n",succ_char_sum, real_num_read, buf);
#endif

	return succ_char_sum;
}

/**********************************************************
*  Function:        clear_adaptor_test_result
*  Discription:     clear special adaptor protocol test result
*  Parameters:     NULL
*  return value:    NULL
**********************************************************/
static void clear_adaptor_test_result(void)
{
	int i = 0;//init the table index to zero
	int adt_test_tbl_len = 0;//init the table length to zero

	adt_test_tbl_len = sizeof(adptor_test_tbl)/(sizeof(adptor_test_tbl[0]));
	for(i = 0; i < adt_test_tbl_len; i++){
		adptor_test_tbl[i].result = DETECT_FAIL;
	}

	return;
}
#ifdef CONFIG_TCPC_CLASS
static void huawei_pd_typec_current(struct charge_device_info *di)
{
	enum pd_dpm_cc_voltage_type cc_type = pd_dpm_get_cc_voltage();
	switch (cc_type) {
	case PD_DPM_CC_VOLT_SNK_3_0:
		di->input_current = di->input_current < TYPE_C_HIGH_MODE_CURR ?
			di->input_current : TYPE_C_HIGH_MODE_CURR;
		hwlog_info("[%s]PD_DPM_CC_VOLT_SINK_3_0, !!! \n", __func__);
		break;
	case PD_DPM_CC_VOLT_SNK_1_5:
		di->input_current = TYPE_C_MID_MODE_CURR;
		hwlog_info("[%s]PD_DPM_CC_VOLT_SINK_1_5, !!! \n", __func__);
		break;
	case PD_DPM_CC_VOLT_SNK_DFT:
		hwlog_info("[%s]PD_DPM_CC_VOLT_SINK_DFT, !!! \n", __func__);
		break;
	default:
		hwlog_info("[%s]PD_DPM_CC_VOLT_TYPE = %d, !!! \n", __func__, cc_type);
		break;
	}
}
#endif
/**********************************************************
*  Function:       charge_typec_current
*  Discription:    select typec current or bc1.2 type current
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void charge_typec_current(struct charge_device_info *di)
{
	switch (di->typec_current_mode) {
	case TYPEC_DEV_CURRENT_HIGH:
		di->input_current = di->input_current < TYPE_C_HIGH_MODE_CURR ?
			di->input_current : TYPE_C_HIGH_MODE_CURR;
		di->charge_current = di->charge_current < TYPE_C_HIGH_MODE_CURR ?
			di->charge_current : TYPE_C_HIGH_MODE_CURR;
		break;
	case TYPEC_DEV_CURRENT_MID:
		di->input_current = TYPE_C_MID_MODE_CURR;
		di->charge_current = TYPE_C_MID_MODE_CURR;
		break;
	case TYPEC_DEV_CURRENT_DEFAULT:
	case TYPEC_DEV_CURRENT_NOT_READY:
	default:
		break;
	}
	hwlog_info("[%s]Type C type   %d,%d,%d !!! \n", __func__,
			   di->typec_current_mode, di->input_current,
			   di->charge_current);
	return;
}

/**********************************************************
*  Function:       select_ico_current
*  Description:    select ico current
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void select_ico_current(struct charge_device_info *di)
{
	int ret = -1;
	static struct ico_input input;
	static struct ico_output output;

	if (di->start_attemp_ico) {
		if (!di->ops || !di->ops->turn_on_ico) {
			return;
		} else {
			input.charger_type = di->charger_type;
			input.iin_max = di->core_data->iin_max;
			input.ichg_max = di->core_data->ichg_max;
			input.vterm = di->core_data->vterm < di->sysfs_data.vterm_rt ?
						di->core_data->vterm : di->sysfs_data.vterm_rt;
			output.input_current = di->input_current;
			output.charge_current = di->charge_current;
			ret = di->ops->turn_on_ico(&input, &output);
			if (!ret) {
				if (!di->ico_all_the_way) {
					di->start_attemp_ico = 0;
					hwlog_info("ico result: input current is %dmA, charge current is %dmA\n",
						output.input_current, output.charge_current);
				} else {
					di->start_attemp_ico = 1;
				}
			} else {
				hwlog_info("ico current detect fail.");
			}
		}
	}

	di->input_current = output.input_current;
	di->charge_current =  output.charge_current;
}

/**********************************************************
*  Function:       charge_select_charging_current
*  Description:    get the input current and charge current from different charger_type and charging limited value
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void charge_select_charging_current(struct charge_device_info *di)
{
	static unsigned int first_in = 1;

	if (cancel_work_flag) {
		hwlog_info("[%s] charge already stop\n", __func__);
		return;
	}

	switch (di->charger_type) {
	case CHARGER_TYPE_USB:
		di->input_current = di->core_data->iin_usb;
		di->charge_current = di->core_data->ichg_usb;
#ifdef CONFIG_FORCE_FAST_CHARGE
		if (force_fast_charge > 0)
		{
			di->input_current = CHARGE_CURRENT_0800_MA;
			di->charge_current = CHARGE_CURRENT_0800_MA;
		}
#endif
		break;
	case CHARGER_TYPE_NON_STANDARD:
		di->input_current = di->core_data->iin_nonstd;
		di->charge_current = di->core_data->ichg_nonstd;
		break;
	case CHARGER_TYPE_BC_USB:
		di->input_current = di->core_data->iin_bc_usb;
		di->charge_current = di->core_data->ichg_bc_usb;
		break;
	case CHARGER_TYPE_STANDARD:
		di->input_current = di->core_data->iin_ac;
		di->charge_current = di->core_data->ichg_ac;
		break;
	case CHARGER_TYPE_VR:
		di->input_current = di->core_data->iin_vr;
		di->charge_current = di->core_data->ichg_vr;
		break;
	case CHARGER_TYPE_FCP:
		di->input_current = di->core_data->iin_fcp;
		di->charge_current = di->core_data->ichg_fcp;
		break;
	case CHARGER_TYPE_PD:
		di->input_current = di->pd_input_current;
		di->charge_current = di->pd_charge_current;
		hwlog_info("CHARGER_TYPE_PD input_current %d  charge_current = %d",
			di->input_current, di->charge_current);
		if(di->charge_current > di->core_data->ichg_max)
			di->charge_current = di->core_data->ichg_max;
		break;
#ifdef CONFIG_WIRELESS_CHARGER
	case CHARGER_TYPE_WIRELESS:
		di->input_current = di->core_data->iin_wireless;
		di->charge_current = di->core_data->ichg_wireless;
		break;
#endif
	default:
		di->input_current = CHARGE_CURRENT_0500_MA;
		di->charge_current = CHARGE_CURRENT_0500_MA;
		break;
	}
#ifdef CONFIG_OTG_GPIO_ID
	if ((ycable != NULL) && ycable->ycable_support && (YCABLE_CHARGER == ycable->y_cable_status)) {
		di->input_current = ycable->ycable_iin_curr;
		di->charge_current = ycable->ycable_ichg_curr;
		hwlog_info("ycable input curr = %d, ichg curr = %d\n", di->input_current, di->charge_current);
	}
#endif
	/*only the typec is supported ,we need read typec result and
	when adapter is fcp adapter ,we set current by fcp adapter rule */
	if (di->core_data->typec_support && (FCP_STAGE_SUCESS != fcp_get_stage_status())) {
		charge_typec_current(di);
	}
#ifdef CONFIG_TCPC_CLASS
	if (charger_pd_support) {
		switch (di->charger_type) {
			case CHARGER_TYPE_USB:
			case CHARGER_TYPE_NON_STANDARD:
			case CHARGER_TYPE_BC_USB:
			case CHARGER_TYPE_STANDARD:
					huawei_pd_typec_current(di);
				break;
			default:
				break;
		}
	}
#endif

	if((1 == di->sysfs_data.support_ico)
		&& (1 == di->sysfs_data.charge_enable)
		&& (1 == ico_enable))
		select_ico_current(di);

#ifndef CONFIG_HLTHERM_RUNTEST
	if (strstr(saved_command_line, "androidboot.swtype=factory")
	    && (!is_hisi_battery_exist())) {
		if (first_in) {
			hwlog_info
			    ("facory_version and battery not exist, enable charge\n");
			first_in = 0;
		}
	} else {
#endif
		if (di->sysfs_data.charge_limit == TRUE) {
			di->input_current = di->input_current < di->core_data->iin ?
				di->input_current : di->core_data->iin;
			di->input_current = di->input_current < di->sysfs_data.iin_thl ?
				di->input_current : di->sysfs_data.iin_thl;
			di->input_current = di->input_current < di->sysfs_data.iin_rt ?
				di->input_current : di->sysfs_data.iin_rt;

			di->charge_current = di->charge_current < di->core_data->ichg ?
				di->charge_current : di->core_data->ichg;
			di->charge_current = di->charge_current < di->sysfs_data.ichg_thl ?
				di->charge_current : di->sysfs_data.ichg_thl;
			di->charge_current = di->charge_current < di->sysfs_data.ichg_rt ?
				di->charge_current : di->sysfs_data.ichg_rt;
		}
		//if charger detected as weaksource, reset iuput current
		if((CHARGER_TYPE_FCP != di->charger_type)
			|| ((CHARGER_TYPE_FCP ==di->charger_type) && (FCP_STAGE_CHARGE_DONE == fcp_get_stage()))){
			if(di->ops && di->ops->rboost_buck_limit){
				if(WEAKSOURCE_TRUE == di->ops->rboost_buck_limit()
					&& INVALID_CURRENT_SET != di->core_data->iin_weaksource){
					hwlog_info("Weak source, reset iin_limit!\n");
					di->input_current = di->input_current < di->core_data->iin_weaksource ?
					di->input_current : di->core_data->iin_weaksource;
				}
			}
		}
#ifndef CONFIG_HLTHERM_RUNTEST
	}
#endif

	if (0 != di->sysfs_data.inputcurrent)
		di->input_current = di->sysfs_data.inputcurrent;

	if (1 == di->sysfs_data.batfet_disable)
		di->input_current = CHARGE_CURRENT_2000_MA;
#ifdef CONFIG_WIRELESS_CHARGER
	wireless_tx_limit_input_current(di);
	if (di->iin_limit) {
		di->input_current = min(di->iin_limit, di->input_current);
	}
#endif
}

static int charge_update_pd_vbus_voltage_check(struct charge_device_info *di)
{
	if (!di) {
		hwlog_err("input para is null, just return false.\n");
		return FALSE;
	}
	if (di->charger_type == CHARGER_TYPE_PD) {
		int ret = 0;
		unsigned int vbus_vol = 0;

		if (di->ops && di->ops->get_vbus) {
			ret = di->ops->get_vbus(&vbus_vol);
		} else {
			hwlog_err("Not support VBUS check.\n");
			ret = -EINVAL;
		}
		if (ret) {
			hwlog_err("[%s]vbus vol read fail.\n",__func__);
			ret = pd_dpm_get_high_power_charging_status() ? TRUE : FALSE;
		} else {
			ret = vbus_vol > VBUS_VOLTAGE_7000_MV ? TRUE : FALSE;
		}
		return ret;
	}
	return FALSE;
}

/**********************************************************
*  Function:       charge_update_vindpm
*  Description:    update the input dpm voltage setting by battery capacity
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void charge_update_vindpm(struct charge_device_info *di)
{
	int ret = 0;
	int vindpm = CHARGE_VOLTAGE_4520_MV;

	if (cancel_work_flag) {
		hwlog_info("[%s] charge already stop\n", __func__);
		return;
	}
	if (FCP_STAGE_SUCESS == fcp_get_stage_status()
#ifdef CONFIG_TCPC_CLASS
		|| charge_update_pd_vbus_voltage_check(di)
#endif
		)
	{
		vindpm = di->fcp_vindpm;
	} else if (POWER_SUPPLY_TYPE_MAINS == di->charger_source) {
		vindpm = di->core_data->vdpm;
	} else if (POWER_SUPPLY_TYPE_USB == di->charger_source) {
		if (di->core_data->vdpm > CHARGE_VOLTAGE_4520_MV) {
			vindpm = di->core_data->vdpm;
		}
	} else {
		/*do nothing:*/
	}

	if (di->ops->set_dpm_voltage) {
		ret = di->ops->set_dpm_voltage(vindpm);
		if (ret > 0) {
			hwlog_info("dpm voltage is out of range:%dmV!!\n", ret);
			ret = di->ops->set_dpm_voltage(ret);
			if (ret < 0)
				hwlog_err("set dpm voltage fail!!\n");
		} else if (ret < 0)
			hwlog_err("set dpm voltage fail!!\n");
	}
}

/**********************************************************
*  Function:       charge_update_external_setting
*  Description:    update the others chargerIC setting
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void charge_update_external_setting(struct charge_device_info *di)
{
	int ret = 0;
	unsigned int batfet_disable = FALSE;
	unsigned int watchdog_timer = WATCHDOG_TIMER_80_S;

	if (cancel_work_flag) {
		hwlog_info("[%s] charge already stop\n", __func__);
		return;
	}

	/*update batfet setting */
	if (di->sysfs_data.batfet_disable == TRUE) {
		batfet_disable = TRUE;
	}
	if (di->ops->set_batfet_disable) {
		ret = di->ops->set_batfet_disable(batfet_disable);
		if (ret)
			hwlog_err("set batfet disable fail!!\n");
	}
	/*update watch dog timer setting */
	if (di->sysfs_data.wdt_disable == TRUE) {
		watchdog_timer = WATCHDOG_TIMER_DISABLE;
	}
	if (di->ops->set_watchdog_timer) {
		ret = di->ops->set_watchdog_timer(watchdog_timer);
		if (ret)
			hwlog_err("set watchdog timer fail!!\n");
	}
}

/*******************************************************
  Function:        calc_avg_input_current_ma
  Description:     calc_avg_input_current_ma
  Input:           input_current_ma
  Output:          iavg_ma
  Return:         iavg_ma
********************************************************/
static int calc_avg_input_current_ma(struct charge_device_info *di,int input_current_ma)
{
	int i;
	int iavg_ma = 0;
	static int iavg_samples[IIN_AVG_SAMPLES];
	static int iavg_index = 0;
	static int iavg_num_samples;
	if( NULL == di ) {
		hwlog_info("NULL point in [%s]\n", __func__);
		return IMPOSSIBLE_IIN;
	}
	if(clear_iin_avg_flag) {
		iavg_index = 0;
		iavg_num_samples =0;
		clear_iin_avg_flag = 0;
	}
	if(IMPOSSIBLE_IIN == input_current_ma) {
		hwlog_info("input_current_ma is invalid [%s]\n", __func__);
		return IMPOSSIBLE_IIN;
	}
	iavg_samples[iavg_index] = input_current_ma;
	iavg_index = (iavg_index + 1) % IIN_AVG_SAMPLES;
	iavg_num_samples++;
	if (iavg_num_samples >= IIN_AVG_SAMPLES)
		iavg_num_samples = IIN_AVG_SAMPLES;

	iavg_ma = 0;
	for (i = 0; i < iavg_num_samples; i++) {
		iavg_ma += iavg_samples[i];
	}

	iavg_ma = DIV_ROUND_CLOSEST(iavg_ma, iavg_num_samples);

	if (iavg_num_samples > IIN_AVG_SAMPLES/2)
	{
		return  iavg_ma;
	} else {
		return IMPOSSIBLE_IIN;
	}
}

/**********************************************************
*  Function:       charge_is_charging_full
*  Description:    check the battery is charging full  or not
*  Parameters:   di:charge_device_info
*  return value:  TURE-is full or FALSE-no full
**********************************************************/
static int charge_is_charging_full(struct charge_device_info *di)
{
	int ichg = -hisi_battery_current();
	int ichg_avg = hisi_battery_current_avg();
	int val = FALSE;
	int term_allow = FALSE;

	if(!(di->charge_enable) || !(is_hisi_battery_exist()))
		return val;

	if(((ichg > MIN_CHARGING_CURRENT_OFFSET) && (ichg_avg > MIN_CHARGING_CURRENT_OFFSET))
		|| di->core_data->warm_triggered)
		term_allow = TRUE;

	if (term_allow && (ichg < (int)di->core_data->iterm)
	    && (ichg_avg < (int)di->core_data->iterm)) {
		di->check_full_count++;
		if (di->check_full_count >= BATTERY_FULL_CHECK_TIMIES) {
			di->check_full_count = BATTERY_FULL_CHECK_TIMIES;
			val = TRUE;
			hwlog_info
			    ("battery is full!capacity = %d,ichg = %d,ichg_avg = %d\n",
			     hisi_battery_capacity(), ichg, ichg_avg);
		}
	} else {
		di->check_full_count = 0;
	}

	return val;
}
static void charge_check_termination(struct charge_device_info *di, int full_flag)
{
	int ret;
	unsigned int state = CHAGRE_STATE_NORMAL;
	int soc = hisi_battery_capacity();

	if(NULL == di || NULL == di->ops){
		hwlog_err("%s, di or ops is null, return.\n",__func__);
		return;
	}

	ret = di->ops->get_charge_state(&state);
	if (ret < 0) {
		hwlog_err("get_charge_state fail!!ret = 0x%x\n", ret);
		return;
	}

	if (soc < CAPACITY_FULL) {
		term_err_chrg_en_flag = TRUE;
		term_err_cnt = 0;
	} else if (full_flag && soc >= CAPACITY_FULL
		&& !(state & CHAGRE_STATE_CHRG_DONE)
		&& term_err_chrg_en_flag == TRUE) {
		term_err_cnt++;
		if (term_err_cnt >= di->term_exceed_time) {
			term_err_cnt = di->term_exceed_time;
			term_err_chrg_en_flag = FALSE;
			hwlog_err("%s err, stop charging\n", __func__);
		}
	} else {
		term_err_cnt = 0;
	}
}


/**********************************************************
*  Function:       is_charge_current_full
*  Description:    check charging full  by charge current
*  Parameters:   di:charge_device_info
*  return value:  TURE-is full or FALSE-no full
**********************************************************/
static int is_charge_current_full(struct charge_device_info *di)
{
	int ichg = -hisi_battery_current();
	int ichg_avg = hisi_battery_current_avg();
	int bat_vol = hisi_battery_voltage();
	int bat_cap = hisi_battery_capacity();
	int val = FALSE;
	int current_full_allow = FALSE;

	if(NULL == di)
		return val;

	if (!(di->charge_enable) || !(is_hisi_battery_exist()))
		return val;

	if(((ichg > MIN_CHARGING_CURRENT_OFFSET) && (ichg_avg > MIN_CHARGING_CURRENT_OFFSET)))
		current_full_allow = TRUE;

	if(current_full_allow && di->ops->check_input_vdpm_state && di->ops->check_input_idpm_state ) {
		if(di->ops->check_input_vdpm_state() || di->ops->check_input_idpm_state()) {
			current_full_allow = FALSE;
		}
	}

	if (current_full_allow && !di->core_data->warm_triggered &&
		ichg <= batt_ifull + DELA_ICHG_FOR_CURRENT_FULL &&
		ichg_avg >= batt_ifull - DELA_ICHG_FOR_CURRENT_FULL && ichg_avg <= batt_ifull + DELA_ICHG_FOR_CURRENT_FULL &&
		di->charge_current  >= batt_ifull && di->input_current >= batt_ifull &&
		di->sysfs_data.ibus <  di->max_iin_ma* CURRENT_FULL_VALID_PERCENT /100 && IMPOSSIBLE_IIN != di->max_iin_ma &&
		bat_vol > di->core_data->vterm - CURRENT_FULL_VOL_OFFSET  && bat_cap>= CAP_TH_FOR_CURRENT_FULL) {
		di->check_current_full_count++;
		if (di->check_current_full_count >= CURRENT_FULL_CHECK_TIMES) {
			di->check_current_full_count = CURRENT_FULL_CHECK_TIMES;
			val = TRUE;
		}
	} else {
		di->check_current_full_count = 0;
	}
	hwlog_info("[%s] ichg %d,ichg_avg %d,bat_vol %d,ibus %d,max_iin_ma %d,warm_triggered %d,is_current_full %d\n",
		__func__,ichg,ichg_avg,bat_vol,di->sysfs_data.ibus,di->max_iin_ma,di->core_data->warm_triggered,val);
	return val;
}

/**********************************************************
*  Function:       charge_full_handle
*  Description:    set term enable flag by charge current is lower than iterm
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void charge_full_handle(struct charge_device_info *di)
{
	int ret = 0;
	int is_battery_full = charge_is_charging_full(di);

	if (cancel_work_flag) {
		hwlog_info("[%s] charge already stop\n", __func__);
		return;
	}

	if (di->ops->set_term_enable) {
		ret = di->ops->set_term_enable(is_battery_full);
		if (ret)
			hwlog_err("set term enable fail!!\n");
	}
	/*set terminal current */
	ret = di->ops->set_terminal_current(di->core_data->iterm);
	if (ret > 0) {
		di->ops->set_terminal_current(ret);
	} else if (ret < 0) {
		hwlog_err("set terminal current fail!\n");
	}

	/* reset adapter to 5v after fcp charge done,avoid long-term at high voltage */
	if (is_battery_full && hisi_battery_capacity() == 100
		&& NULL != di->fcp_ops
		&& NULL != di->fcp_ops->fcp_adapter_reset) {
		if(FCP_STAGE_RESET_ADAPTOR == fcp_get_stage_status()) {
			fcp_set_stage_status(FCP_STAGE_CHARGE_DONE);
			hwlog_info("reset adapter to 5v already!\n");
		} else if(FCP_STAGE_SUCESS == fcp_get_stage_status()) {
			if((1 == di->charge_done_maintain_fcp) && (!strstr(saved_command_line, "androidboot.mode=charger"))){
			  	hwlog_info("fcp charge done, no reset adapter to 5v !\n");
		      	}else{
				if (di->fcp_ops->fcp_adapter_reset()) {
					hwlog_err("adapter reset failed \n");
				}
				fcp_set_stage_status(FCP_STAGE_CHARGE_DONE);
				if (di->ops->set_vbus_vset) {
					ret = di->ops->set_vbus_vset(ADAPTER_5V);
					if(ret)
						hwlog_err("set vbus_vset fail!\n");
				}
				hwlog_info("fcp charge done, reset adapter to 5v !\n");
			}
		}
	}
	charge_check_termination(di, is_battery_full);
}

static int set_charge_state(int state)
{
	int old_state;
	int chg_en = 0;
	struct charge_device_info *di = g_di;

	if ((di == NULL)
	    || ((state != 0) && (state != 1))
	    || NULL == di->ops->get_charge_enable_status ){
	    return -1;
	}

	old_state = di->ops->get_charge_enable_status();
	chg_en = state;
	di->ops->set_charge_enable(chg_en);

	return old_state;
}

/**********************************************************
*  Function:       charge_kick_watchdog
*  Description:    kick watchdog timer
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void charge_kick_watchdog(struct charge_device_info *di)
{
	int ret = 0;
	ret = di->ops->reset_watchdog_timer();
	if (ret)
		hwlog_err("charge kick watchdog timer fail!!\n");
#ifdef CONFIG_HISI_CHARGER_SYS_WDG
	charge_feed_sys_wdt(CHARGE_SYS_WDG_TIMEOUT);
#endif
}

/**********************************************************
*  Function:       charge_disable_watchdog
*  Description:   disable watchdog timer
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void charge_disable_watchdog(struct charge_device_info *di)
{
	int ret = 0;
	ret = di->ops->set_watchdog_timer(WDT_STOP);
	if (ret)
		hwlog_err("charge disable watchdog timer fail!!\n");
	else
		hwlog_info("charge disable watchdog timer");
#ifdef CONFIG_HISI_CHARGER_SYS_WDG
	charge_stop_sys_wdt();
#endif
}

bool charge_get_hiz_state(void)
{
	bool ret = false;
	struct charge_device_info *di = g_di;
	if (!di)
		return false;
	mutex_lock(&di->mutex_hiz);
	if (di->hiz_ref)
		ret = true;
	else
		ret = false;
	mutex_unlock(&di->mutex_hiz);
	return ret;
}
/**********************************************************
*  Function:       charge_set_hiz_enable
*  Description:   set hiz enable
*  Parameters:   0 -- disable hiz; 1 -- enable_hiz
*  return value:  NULL
**********************************************************/
int charge_set_hiz_enable(int enable)
{
	struct charge_device_info *di = g_di;
	int ret = 0;

	if (!di || !di->ops || !di->ops->set_charger_hiz) {
		return -1;
	}
	mutex_lock(&di->mutex_hiz);
	hwlog_info("set hiz enable = %d refcnt = %u\n",enable, di->hiz_ref);
	if (enable) {
		ret = di->ops->set_charger_hiz(enable);
		di->hiz_ref++;
	} else {
		if (di->hiz_ref == 0) {
			ret = di->ops->set_charger_hiz(enable);
			hwlog_err("%s: Unbalanced hiz mode\n", __func__);
		} else {
			if (1 == di->hiz_ref) {
				ret = di->ops->set_charger_hiz(enable);
			}
			di->hiz_ref--;
		}
	}
	mutex_unlock(&di->mutex_hiz);
	return ret;
}

/**********************************************************
  *  Function:      charge_set_batfet_disable
  *  Description:   set batfet disable
  *  Parameters:   1 -- disable; 0 -- enable
  *  return value:  NULL
  **********************************************************/
void charge_set_batfet_disable(int val)
{
	int ret = 0;
	struct charge_device_info *di = g_di;
	if(!di || !di->ops || !di->ops->set_batfet_disable) {
		hwlog_err("di or di->ops_set_batfet_disable is NULL return!\n");
		return;
	}

	ret = di->ops->set_batfet_disable(val);
	if(ret) {
		hwlog_err("set batfet disable failed!\n");
	}
}

int disable_chargers(int charger_type, int disable, int role)
{
	int ret = 0;
	if(!charger_type) {
		ret |= set_charger_disable_flags(disable, role);
#ifdef CONFIG_DIRECT_CHARGER
		ret |= set_direct_charger_disable_flags(disable, role);
#endif
	}
	return ret;
}
int set_charger_disable_flags(int val, int type)
{
	struct charge_device_info *di = g_di;
	int i;
	int disable = 0;

	if(!di)
	{
		hwlog_err("NULL pointer(di) found in %s.\n", __func__);
		return -1;
	}

	if(type < 0 || type >= __MAX_DISABLE_CHAGER){
		hwlog_err("Set direct charger to %d with wrong type(%d) in %s.\n",
					val, type, __func__);
		return -1;
	}

	di->sysfs_data.disable_charger[type] = val;
	for( i = 0; i < __MAX_DISABLE_CHAGER; i++){
		disable |= di->sysfs_data.disable_charger[i];
	}
	if(di->sysfs_data.charge_enable == disable) {
		di->sysfs_data.charge_enable = !disable;
		if(!di->sysfs_data.charge_enable) {
			di->ops->set_charge_enable(di->sysfs_data.charge_enable);
		}
	}
	return 0;
}

/**********************************************************
*  Function:       charge_start_charging
*  Description:    enter into charging mode
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void charge_start_charging(struct charge_device_info *di)
{
	int ret = 0;
	struct chip_init_crit init_crit;

	hwlog_info("---->START CHARGING\n");
	charge_wake_lock();
	if (!strstr(saved_command_line, "androidboot.swtype=factory")) {
		set_charger_disable_flags(CHARGER_CLEAR_DISABLE_FLAGS, CHARGER_SYS_NODE);
	}
	term_err_chrg_en_flag = TRUE;
	term_err_cnt = 0;
	di->check_full_count = 0;
	di->start_attemp_ico = 1;
	clear_iin_avg_flag = 1;
	di->current_full_status = 0;
	di->avg_iin_ma = IMPOSSIBLE_IIN;
	di->max_iin_ma = IMPOSSIBLE_IIN;
	/*chip init */
	init_crit.vbus = ADAPTER_5V;
	ret = di->ops->chip_init(&init_crit);
	if (ret)
		hwlog_err("chip init fail!!\n");
#ifdef CONFIG_HISI_CHARGER_SYS_WDG
	charge_enable_sys_wdt();
#endif
	mod_delayed_work(system_wq, &di->charge_work, msecs_to_jiffies(0));

	schedule_delayed_work(&di->vbus_valid_check_work,
			      msecs_to_jiffies(VBUS_VALID_CHECK_WORK_TIMEOUT));
}

/**********************************************************
*  Function:       charge_stop_charging
*  Description:    exit charging mode
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/

static void charge_stop_otg(struct charge_device_info *di)
{
	int ret = 0;

	hwlog_info("---->STOP OTG ++\n");
	ret = di->ops->set_charge_enable(FALSE);
	if (ret)
		hwlog_err("[%s]set charge enable fail!\n", __func__);
	ret = di->ops->set_otg_enable(FALSE);
	if (ret)
		hwlog_err("[%s]set otg enable fail!\n", __func__);
	otg_flag = FALSE;
	cancel_delayed_work_sync(&di->otg_work);

	if ((di->sysfs_data.wdt_disable == TRUE) && (di->ops->set_watchdog_timer)) {	/*when charger stop ,disable watch dog ,only for hiz */
		if (di->ops->set_watchdog_timer(WATCHDOG_TIMER_DISABLE))
			hwlog_err("set watchdog timer fail for hiz!!\n");
	}
	hwlog_info("---->STOP OTG --\n");
}

static void charge_stop_charging(struct charge_device_info *di)
{
	int ret = 0;

	hwlog_info("---->STOP CHARGING\n");
	pd_charge_flag = false;
	cancel_work_flag = true;
	charge_wake_lock();
	nonstand_detect_times = 0;
	ico_enable = 0;
	if (!strstr(saved_command_line, "androidboot.swtype=factory")) {
		set_charger_disable_flags(CHARGER_SET_DISABLE_FLAGS, CHARGER_SYS_NODE);
	}
	di->sysfs_data.adc_conv_rate = 0;
	di->sysfs_data.water_intrused = 0;
	di->sysfs_data.charge_done_status = CHARGE_DONE_NON;
	di->weaksource_cnt = 0;
#ifdef CONFIG_TCPC_CLASS
	di->pd_input_current = 0;
	di->pd_charge_current = 0;
#endif

#ifdef CONFIG_DIRECT_CHARGER
	if (!direct_charge_get_cutoff_normal_flag())
	{
		direct_charge_set_scp_stage_default();
	}
#endif
	charger_type_update = FALSE;
	output_num = 0;
	detect_num = 0;
	vbus_flag = 0;
	nonfcp_vbus_higher_count = 0;
	fcp_vbus_lower_count = 0;
	pd_vbus_abnormal_cnt = 0;

	if (di->ops->set_adc_conv_rate)
		di->ops->set_adc_conv_rate(di->sysfs_data.adc_conv_rate);
	di->check_full_count = 0;
	ret = di->ops->set_charge_enable(FALSE);
	if (ret)
		hwlog_err("[%s]set charge enable fail!\n", __func__);
	charge_otg_mode_enable(OTG_DISABLE, OTG_CTRL_WIRED_OTG);
	otg_flag = FALSE;
	cancel_delayed_work_sync(&di->charge_work);
	cancel_delayed_work_sync(&di->otg_work);
	cancel_work_flag = false;
	if ((di->sysfs_data.wdt_disable == TRUE) && (di->ops->set_watchdog_timer)) {	/*when charger stop ,disable watch dog ,only for hiz */
		if (di->ops->set_watchdog_timer(WATCHDOG_TIMER_DISABLE))
			hwlog_err("set watchdog timer fail for hiz!!\n");
	}
	if (di->ops->stop_charge_config) {
		if (di->ops->stop_charge_config()) {
			hwlog_err(" stop charge config failed \n");
		}
	}
	if (di->fcp_ops && di->fcp_ops->stop_charge_config) {
		if (di->fcp_ops->stop_charge_config()) {
			hwlog_err(" fcp stop charge config failed \n");
		}
	}
	stop_charging_core_config();
	/*flag must be clear after charge_work has been canceled */
	fcp_set_stage_status(FCP_STAGE_DEFAUTL);
	set_fcp_charging_flag(FALSE);
#ifdef CONFIG_DIRECT_CHARGER
	if (!direct_charge_get_cutoff_normal_flag())
	{
		direct_charge_stop_charging();
	}
	direct_charge_update_cutoff_flag();
#endif

	wake_lock_timeout(&stop_charge_lock, HZ);
	mutex_lock(&charge_wakelock_flag_lock);
	charge_lock_flag = CHARGE_NO_NEED_WAKELOCK;
	charge_wake_unlock();
	mutex_unlock(&charge_wakelock_flag_lock);
#ifdef CONFIG_HISI_CHARGER_SYS_WDG
	charge_stop_sys_wdt();
#endif
}
extern void hisi_usb_otg_bc_again(void);
/**********************************************************
*  Function:       charge_type_dcp_detected_notify
*  Description:    check high voltage charge once dcp detected
*  Parameters:     NULL
*  return value:   NULL
**********************************************************/
void charge_type_dcp_detected_notify(void)
{
        int pmic_vbus_irq_enabled = 1;
#ifdef CONFIG_TCPC_CLASS
        pmic_vbus_irq_enabled = pmic_vbus_irq_is_enabled();
#endif

	if (g_di && (CHARGER_TYPE_NON_STANDARD == g_di->charger_type)) {
		hisi_usb_otg_bc_again();
		hwlog_info(" stop phy enter! \n");
		if (!pmic_vbus_irq_enabled) {
			mutex_lock(&g_di->event_type_lock);
			if (g_di->event == START_SINK) {
				mod_delayed_work(system_wq, &g_di->charge_work, msecs_to_jiffies(0));
			}
			mutex_unlock(&g_di->event_type_lock);
		} else {
			mod_delayed_work(system_wq, &g_di->charge_work, msecs_to_jiffies(0));
		}
	}
}

static void charge_pd_voltage_change_work(struct work_struct *work)
{
#ifdef CONFIG_TCPC_CLASS
	struct charge_device_info *di =
	    container_of(work, struct charge_device_info, pd_voltage_change_work.work);
	int vset = PD_ADAPTER_9V;
	int ret = 0;

	if (di->reset_adapter) {
		vset = PD_ADAPTER_5V;
	} else {
		vset = PD_ADAPTER_9V;
	}
	if (vset != last_vset) {
		ret = pd_dpm_set_voltage(vset);
		if (!ret) {
			hwlog_err("[%s]set voltage failed \n", __func__);
		}
	}
#endif
}
void charge_set_adapter_voltage(int val, enum reset_adapter_source_type type, unsigned int delay_time)
{
	struct charge_device_info *di = g_di;
        int pmic_vbus_irq_enabled = 1;
#ifdef CONFIG_TCPC_CLASS
        pmic_vbus_irq_enabled = pmic_vbus_irq_is_enabled();
#endif

	if (NULL == di) {
		hwlog_err("[%s] di is NULL!\n", __func__);
		return;
	}
	if (type < 0 || type >= RESET_ADAPTER_TOTAL) {
		hwlog_err("%s: type(%d) is invalid!\n", __func__, type);
		return;
	}

	if(ADAPTER_5V == val) {
		di->reset_adapter = di->reset_adapter | (1 << type);
		hwlog_info("Reset adaptor to 5V, reset_adapter = 0x%x\n", di->reset_adapter);
	} else {
		di->reset_adapter = di->reset_adapter & (~(1 << type));
		hwlog_info("Restore adaptor to 9V, reset_adapter = 0x%x\n", di->reset_adapter);
	}

	/*fcp adapter reset*/
	if (di->charger_type == CHARGER_TYPE_FCP || di->charger_type == CHARGER_TYPE_STANDARD) {
		if (!pmic_vbus_irq_enabled) {
			if (di->event == START_SINK) {
				mod_delayed_work(system_wq, &di->charge_work, msecs_to_jiffies(0));
			}
		} else {
			mod_delayed_work(system_wq, &di->charge_work, msecs_to_jiffies(0));
		}
	}
	/*pd adapter reset*/
	if (di->charger_type == CHARGER_TYPE_PD) {
		cancel_delayed_work_sync(&di->pd_voltage_change_work);
		schedule_delayed_work(&di->pd_voltage_change_work, msecs_to_jiffies(delay_time));
	}
}
/**********************************************************
*  Function:       charge_start_usb_otg
*  Description:    enter into otg mode
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void charge_start_usb_otg(struct charge_device_info *di)
{
	int ret = 0;

	if (otg_flag) {
		hwlog_info("---->OTG Already Enabled\n");
		return;
	}

	hwlog_info("---->START OTG MODE\n");


#ifdef CONFIG_OTG_GPIO_ID
	if ((ycable != NULL) && ycable->ycable_support) {
		charge_wake_lock();
		otg_flag = TRUE;
		ycable->y_cable_status = YCABLE_UNKNOW;
		ycable->ycable_wdt_time_out = 0;
		charge_kick_watchdog(di);
		schedule_delayed_work(&ycable->ycable_work, msecs_to_jiffies(YCABLE_WORK_TIMEOUT));
		charge_wake_unlock();
		return;
	}
#endif

	otg_wake_lock();

	ret = di->ops->set_charge_enable(FALSE);
	if (ret)
		hwlog_err("[%s]set charge enable fail!\n", __func__);
	ret = di->ops->set_otg_enable(TRUE);
	if (ret)
		hwlog_err("[%s]set otg enable fail!\n", __func__);
	otg_flag = TRUE;
	if (di->ops->set_otg_current) {
		ret = di->ops->set_otg_current(di->core_data->otg_curr);	/*otg current set 500mA form dtsi */
		if (ret)
			hwlog_err("[%s]set otg current fail!\n", __func__);
	}
	schedule_delayed_work(&di->otg_work, msecs_to_jiffies(0));

	otg_wake_unlock();
}
/**********************************************************
*  Function:       charge_otg_work
*  Description:    monitor the otg mode status
*  Parameters:   work:otg workqueue
*  return value:  NULL
**********************************************************/

/*lint -save -e* */
static void charge_otg_work(struct work_struct *work)
{
	struct charge_device_info *di =
	    container_of(work, struct charge_device_info, otg_work.work);
	int ret = 0;

	if (cancel_work_flag) {
		hwlog_info("[%s] charge already stop\n", __func__);
		return;
	}

	ret = di->ops->set_otg_enable(TRUE);
	if (ret)
		hwlog_err("[%s]set otg enable fail!\n", __func__);

	charge_kick_watchdog(di);
	schedule_delayed_work(&di->otg_work,
			      msecs_to_jiffies(CHARGING_WORK_TIMEOUT));
}
#ifdef CONFIG_OTG_GPIO_ID
/**********************************************************
*  Function:       ycable_otg_enable
*  Description:    enable OTG Device
*  Parameters:     di:charger struct
*  return value:   NULL
**********************************************************/
static void ycable_otg_enable(struct charge_device_info *di)
{
	int ret = 0;

	if (NULL == di) {
		return;
	}

	otg_flag = TRUE;

	if (di->ops->set_otg_current && di->core_data) {
		ret = di->ops->set_otg_current(di->core_data->otg_curr);
		if (ret < 0) {
			hwlog_err("[%s]set otg current fail!\n", __func__);
			return;
		}
	}

	if (di->ops->set_otg_enable) {
		ret = di->ops->set_otg_enable(TRUE);
		if (ret < 0) {
			hwlog_err("[%s]set otg enable fail!\n", __func__);
		}
	}
}

/**********************************************************
*  Function:       ycable_start_charger
*  Description:    ycable start charger mode
*  Parameters:     charge_device_info
*  return value:   TRUE   --  need exceute again and open charger
*                  FALSE  --  open charger in func fist excute
**********************************************************/
static bool ycable_start_charger(struct charge_device_info *di)
{
	int ret = 0;

	ycable->ycable_otg_enable_flag = FALSE;
	if (YCABLE_UNKNOW == ycable->y_cable_status) {
		ycable->ycable_charger_enable_flag = TRUE;
	}
	if (!ycable->ycable_charger_enable_flag) {
		ret = di->ops->set_otg_enable(FALSE);
		if (ret) {
			hwlog_info("[%s]set otg disabled fail!\n", __func__);
		}
		ycable->ycable_charger_enable_flag = TRUE;
		schedule_delayed_work(&ycable->ycable_work, msecs_to_jiffies(YCABLE_OTG_ENABLE_WORK_TIMEOUT));
		return TRUE;
	} else {
		ycable->y_cable_status = YCABLE_CHARGER;
		di->charger_type = CHARGER_TYPE_NON_STANDARD;
		di->charger_source = POWER_SUPPLY_TYPE_MAINS;
		charge_start_charging(di);
	}

	return FALSE;
}

/**********************************************************
*  Function:       ycable_start_otg
*  Description:    ycable start otg mode
*  Parameters:     charge_device_info
*  return value:   TRUE   --  need exceute again and open otg
*                  FALSE  --  open otg mode in func fist excute
**********************************************************/
static bool ycable_start_otg(struct charge_device_info *di)
{
	ycable->ycable_charger_enable_flag = FALSE;

	if (YCABLE_UNKNOW == ycable->y_cable_status) {
		ycable->ycable_otg_enable_flag = TRUE;
	}
	if (!ycable->ycable_otg_enable_flag) {
		charge_stop_charging(di);
		hisi_coul_charger_event_rcv(VCHRG_STOP_CHARGING_EVENT);
		ycable->ycable_otg_enable_flag = TRUE;
		schedule_delayed_work(&ycable->ycable_work, msecs_to_jiffies(YCABLE_OTG_ENABLE_WORK_TIMEOUT));
		return TRUE;
	} else {
		ycable->y_cable_status = YCABLE_OTG;
		ycable_otg_enable(di);
	}

	return FALSE;
}

/**********************************************************
*  Function:       ycable_exit
*  Description:    ycable ADC check voltage is not in range
*  Parameters:     charge_device_info
*  return value:   NULL
**********************************************************/
static void ycable_exit(struct charge_device_info *di)
{
	ycable->y_cable_status = YCABLE_UNKNOW;
	ycable->ycable_otg_enable_flag = FALSE;
	ycable->ycable_charger_enable_flag = FALSE;
	ycable->ycable_wdt_time_out = 0;
	di->charger_type = CHARGER_REMOVED;
	di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
	charge_stop_charging(di);
	hisi_coul_charger_event_rcv(VCHRG_STOP_CHARGING_EVENT);
	hwlog_err("[%s]ycable device invalid!\n", __func__);
}


/**********************************************************
*  Function:       charge_ycable_work
*  Description:    monitor the ycable otg mode status
*  Parameters:     work:otg workqueue
*  return value:   NULL
**********************************************************/
static void charge_ycable_work(struct work_struct *work)
{
	struct charge_device_info *di = g_di;
	int vol_value = 0;
	bool gpio_value = 0;
	bool need_return = FALSE;

	if ((NULL == di) ||(NULL == di->ops)) {
		hwlog_err("di is NULL Pointer!\n");
		return;
	}

	if (!gpio_is_valid(ycable->ycable_gpio)) {
		hwlog_err("otg-gpio is not valid\n");
		return;
	}

	gpio_value = gpio_get_value(ycable->ycable_gpio);
	if (gpio_value) {
		hwlog_info("ycable charger or OTG removed!\n");
		ycable_exit(di);
		return;
	}

	ycable->ycable_wdt_time_out += YCABLE_WORK_TIMEOUT;
	if (ycable->ycable_wdt_time_out >= YCABLE_DETECT_TIMEOUT) {
		ycable->ycable_wdt_time_out = 0;
		charge_kick_watchdog(di);
	}

	if (0 != ycable->otg_adc_channel) {
		vol_value = hisi_adc_get_value(ycable->otg_adc_channel);
	} else {
		vol_value = YCABLE_OTG_THRESHOLD_VOLTAGE_MIN;
	}

	if ((vol_value >= YCABLE_CHG_THRESHOLD_VOLTAGE_MIN) &&
		(vol_value <= YCABLE_CHG_THRESHOLD_VOLTAGE_MAX) &&
		 (YCABLE_CHARGER != ycable->y_cable_status)) {
		hwlog_err("ycable Charger plugin, disabled otg and charger enable! vol_value = %d\n", vol_value);
		need_return = ycable_start_charger(di);
		if (need_return) {
			return;
		}

	} else if ((YCABLE_OTG_THRESHOLD_VOLTAGE_MIN <= vol_value) &&
			(vol_value <= YCABLE_OTG_THRESHOLD_VOLTAGE_MAX) &&
			 (YCABLE_OTG != ycable->y_cable_status)) {
		hwlog_err("ycable OTG Device insert, disabled charging! vol_value = %d\n", vol_value);
		need_return = ycable_start_otg(di);
		if (need_return) {
			return;
		}
	} else if ((YCABLE_CHG_THRESHOLD_VOLTAGE_MAX <= vol_value) ||
			((YCABLE_OTG_THRESHOLD_VOLTAGE_MAX <= vol_value) &&
			 (vol_value <= YCABLE_CHG_THRESHOLD_VOLTAGE_MIN))){
		ycable_exit(di);
	}

	schedule_delayed_work(&ycable->ycable_work, msecs_to_jiffies(YCABLE_WORK_TIMEOUT));
}
#endif

/**********************************************************
*  Function:       charge_fault_work
*  Description:    handler the fault events from chargerIC
*  Parameters:   work:fault workqueue
*  return value:  NULL
**********************************************************/
static void charge_fault_work(struct work_struct *work)
{
	struct charge_device_info *di =
	    container_of(work, struct charge_device_info, fault_work);

	switch (di->charge_fault) {
	case CHARGE_FAULT_BOOST_OCP:
		if (otg_flag) {
			hwlog_err("vbus overloaded in boost mode,close otg mode!!\n");
			di->ops->set_otg_enable(FALSE);
			di->charge_fault = CHARGE_FAULT_NON;
			otg_flag = FALSE;
			charger_dsm_report(ERROR_BOOST_OCP, NULL);
		}
		break;
	case CHARGE_FAULT_VBAT_OVP:
		if(!di->core_data->warm_triggered)
		{
			hwlog_err("vbat_ovp happend\n!!\n");
			di->charge_fault = CHARGE_FAULT_NON;
			if (di->core_data->vterm == di->core_data->vterm_basp)
			{
				charger_dsm_report(ERROR_CHARGE_VBAT_OVP, NULL);
			}
		}
		break;
	case CHARGE_FAULT_SCHARGER:
		hwlog_err("hisi_schargerV200 fault!!\n");
		di->charge_fault = CHARGE_FAULT_NON;
		charger_dsm_report(ERROR_SCHARGERV200_FAULT, NULL);
		break;
	case CHARGE_FAULT_I2C_ERR:
		hwlog_err("Scharger I2C trans error!\n");
		di->charge_fault = CHARGE_FAULT_NON;
		charger_dsm_report(ERROR_CHARGE_I2C_RW, NULL);
		break;
	case CHARGE_FAULT_WEAKSOURCE:
		hwlog_err("Weaksource happened!\n");
		di->charge_fault = CHARGE_FAULT_NON;
		charger_dsm_report(ERROR_WEAKSOURCE_HAPPEN, NULL);
		break;
	default:
		break;
	}
}


static void vbus_valid_check_update_status(void)
{
	/*send events */
	enum charge_status_event events;
	static int vbus_off_continuous_cnt = 0;

	/* powerdown charging mode */
	#ifdef CONFIG_DIRECT_CHARGER
	if (NOT_IN_SCP_CHARGING_STAGE == is_in_scp_charging_stage())
	{
	#endif
		if (0 == hisi_pmic_get_vbus_status())
		{
		    hwlog_err("%s vbus is absent(%d)\n", __func__,vbus_off_continuous_cnt);

		    if (vbus_off_continuous_cnt++ < 2)
		    {
		        return;
		    }

		    events = VCHRG_STOP_CHARGING_EVENT;
		    hisi_coul_charger_event_rcv(events);

		    return;
		}
		else
		{
		    vbus_off_continuous_cnt = 0;
		}
	#ifdef CONFIG_DIRECT_CHARGER
	}
	#endif

	return;
}

/**********************************************************
*  Function:       vbus_valid_check_work
*  Description:    handler the vbus remove event on the powerdown charging
*  Parameters:   work:vbus_valid_check_work workqueue
*  return value:  NULL
**********************************************************/
static void vbus_valid_check_work(struct work_struct *work)
{
	struct charge_device_info *di =
	    container_of(work, struct charge_device_info, vbus_valid_check_work.work);

	if (strstr(saved_command_line, "androidboot.mode=charger"))
	{
		vbus_valid_check_update_status();

		schedule_delayed_work(&di->vbus_valid_check_work,
			      msecs_to_jiffies(VBUS_VALID_CHECK_WORK_TIMEOUT));
	}
}
#define IBIAS_RETRY_TIMES 3
static void check_ibias_current_safe(struct charge_device_info *di)
{
	int ichg_coul = 0;
	int ichg_set_before =  0;
	int ichg_set_after =  0;
	int temp =  0;
	int warm_current =  0;
	int retry_times = 0;
	int err_flag = 0;

	if((NULL == di)||(CHARGER_TYPE_PD == di->charger_type)
		||(NULL == di->ops)||(NULL == di->ops->get_charge_current))
	{
		return;
	}
	do
	{
		ichg_set_before = di->ops->get_charge_current();
		temp = hisi_battery_temperature_for_charger();
		msleep(di->check_ibias_sleep_time);
		if(hisi_get_coul_calibration_status()){
			hwlog_info("In hisi_coul_calibration_status!\n");
			return;
		}
		ichg_coul = -hisi_battery_current();
		ichg_set_after = di->ops->get_charge_current();
		warm_current = (WARM_CUR_RATIO*hisi_battery_fcc_design())/RATIO_BASE;
		retry_times++;

		if((ichg_set_before ==ichg_set_after)&&(ichg_set_after > warm_current)
			&&(COOL_LIMIT < temp)&&(temp < WARM_LIMIT)&&(ichg_coul > CHARGE_CURRENT_0000_MA))
		{
			if(ichg_coul>= (IBIS_RATIO*ichg_set_after)/RATIO_BASE){
				err_flag++;
			}
			if(IBIAS_RETRY_TIMES == err_flag){
				hwlog_err("check_ibas_current_safe ichg_coul = %d ichg_set_after = %d warm_current = %d retry_times =%d\n",
				ichg_coul,ichg_set_after,warm_current,retry_times);
			}
		}
	}while(err_flag == retry_times && retry_times < IBIAS_RETRY_TIMES);
}
/*lint -restore*/

/**********************************************************
*  Function:       charge_update_status
*  Description:    update the states of charging
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void charge_update_status(struct charge_device_info *di)
{
	enum charge_status_event events = VCHRG_POWER_NONE_EVENT;
	unsigned int state = CHAGRE_STATE_NORMAL;
	int ret = 0;
	static bool last_warm_triggered;
	struct chip_init_crit init_crit;

	if (cancel_work_flag) {
		hwlog_info("[%s] charge already stop\n", __func__);
		return;
	}
#ifdef CONFIG_DIRECT_CHARGER
	if (di->ignore_pluggin_and_plugout_flag)
	{
		hwlog_info("no need to check\n");
		return;
	}
#endif
	ret = di->ops->get_charge_state(&state);
	if (ret < 0) {
		hwlog_err("get_charge_state fail!!ret = 0x%x\n", ret);
		return;
	}

	if(di->ops->get_ibus) {
		di->sysfs_data.ibus = di->ops->get_ibus();
		di->avg_iin_ma = calc_avg_input_current_ma(di,di->sysfs_data.ibus);
		if((di->max_iin_ma < di->avg_iin_ma || di->max_iin_ma == IMPOSSIBLE_IIN) && di->avg_iin_ma != IMPOSSIBLE_IIN ) {
			di->max_iin_ma = di->avg_iin_ma;
		}
	}

	/*check status charger not power good state */
	if(CHARGER_TYPE_PD != di->charger_type) {
		if ((state & CHAGRE_STATE_NOT_PG) && (++di->weaksource_cnt > WEAKSOURCE_CNT)) {
			di->weaksource_cnt = 0;
			hwlog_err("VCHRG_POWER_SUPPLY_WEAKSOURCE\n");
			charger_dsm_report(ERROR_WEAKSOURCE_STOP_CHARGE, NULL);
			events = VCHRG_POWER_SUPPLY_WEAKSOURCE;
			hisi_coul_charger_event_rcv(events);
		}
	}
	/*check status charger ovp err */
	if (state & CHAGRE_STATE_VBUS_OVP) {
		hwlog_err("VCHRG_POWER_SUPPLY_OVERVOLTAGE\n");
		events = VCHRG_POWER_SUPPLY_OVERVOLTAGE;
		hisi_coul_charger_event_rcv(events);
	}
	/*check status watchdog timer expiration */
	if (state & CHAGRE_STATE_WDT_FAULT) {
		hwlog_err("CHAGRE_STATE_WDT_TIMEOUT\n");
		/*init chip register when watchdog timeout */
		init_crit.vbus = ADAPTER_5V;
		di->ops->chip_init(&init_crit);
		events = VCHRG_STATE_WDT_TIMEOUT;
		hisi_coul_charger_event_rcv(events);
	}
	/*check status battery ovp */
	if (state & CHAGRE_STATE_BATT_OVP) {
		hwlog_err("CHAGRE_STATE_BATT_OVP\n");
	}

	/*check charger TS state*/
	if(state & CHAGRE_STATE_NTC_FAULT) {
		ts_flag = TRUE;
	}
	else {
		ts_flag = FALSE;
	}
	/*check status charge done, ac charge and usb charge */
	if ((di->charge_enable) && (is_hisi_battery_exist())) {
		di->sysfs_data.charge_done_status = CHARGE_DONE_NON;
		if(last_warm_triggered ^ di->core_data->warm_triggered) {
			last_warm_triggered = di->core_data->warm_triggered;
			ret = di->ops->set_charge_enable(FALSE);
			msleep(100);
			ret |= di->ops->set_charge_enable(TRUE & di->sysfs_data.charge_enable);
			if (ret)
				hwlog_err("[%s]set charge enable fail!\n", __func__);
			hwlog_info("warm status changed, resume charging\n");
			return;
		}

		if(di->enable_current_full)
		{
			if(!state && is_charge_current_full(di) &&  !di->current_full_status) {
				events =  VCHRG_CURRENT_FULL_EVENT;
				hisi_coul_charger_event_rcv(events);
				di->current_full_status = 1;
			}
		}

		if ((state & CHAGRE_STATE_CHRG_DONE)
			&& !di->core_data->warm_triggered) {
			events = VCHRG_CHARGE_DONE_EVENT;
			hwlog_info("VCHRG_CHARGE_DONE_EVENT\n");
			di->sysfs_data.charge_done_status = CHARGE_DONE;
			charger_event_notify(CHARGER_CHARGING_DONE_EVENT);
			/*charge done sleep has been configured as enable and battery is full, then allow phone to sleep*/
			if (((CHARGE_DONE_SLEEP_ENABLED ==
			     di->sysfs_data.charge_done_sleep_status)
			    || charge_done_sleep_dts) && CAPACITY_FULL == hisi_bci_show_capacity()) {
				if ((CHARGER_TYPE_STANDARD == di->charger_type)
				    || (CHARGER_TYPE_NON_STANDARD == di->charger_type)
					|| (CHARGER_TYPE_FCP == di->charger_type)
					|| (CHARGER_TYPE_PD == di->charger_type)) {
					blocking_notifier_call_chain(&charge_wake_unlock_list, NULL, NULL);
					mutex_lock(&charge_wakelock_flag_lock);
					charge_lock_flag = CHARGE_NEED_WAKELOCK;
					charge_wake_unlock();
					mutex_unlock
					    (&charge_wakelock_flag_lock);
					hwlog_info
					    ("charge wake unlock while charging done\n");
				}
			}
		}else if (di->charger_source == POWER_SUPPLY_TYPE_MAINS) {
			events = VCHRG_START_AC_CHARGING_EVENT;
		}else if (di->charger_source == POWER_SUPPLY_TYPE_BATTERY) {
			events = VCHRG_NOT_CHARGING_EVENT;
			hwlog_info("VCHRG_NOT_CHARGING_EVENT, power_supply: BATTERY\n");
		}else {
			events = VCHRG_START_USB_CHARGING_EVENT;
		}
	} else {
		events = VCHRG_NOT_CHARGING_EVENT;
		hwlog_info("VCHRG_NOT_CHARGING_EVENT\n");
	}
	if (cancel_work_flag) {
		hwlog_info("[%s] charge already stop\n", __func__);
		return;
	}
	hisi_coul_charger_event_rcv(events);
}

/**********************************************************
*  Function:       charge_turn_on_charging
*  Description:    turn on charging, set input and charge currrent /CV termminal voltage / charge enable
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/

/*lint -save -e* */
static void charge_turn_on_charging(struct charge_device_info *di)
{
	int ret = 0;
	unsigned int vterm = 0;

	if (cancel_work_flag) {
		hwlog_info("[%s] charge already stop\n", __func__);
		return;
	}

	di->charge_enable = TRUE;
	/* check vbus voltage ,if vbus is abnormal disable charge or abort from fcp */
	charge_vbus_voltage_check(di);
	/*set input current */
	charge_set_input_current(di->input_current);
	/*check if allow charging or not */
	if (di->charge_current == CHARGE_CURRENT_0000_MA) {
		di->charge_enable = FALSE;
		hwlog_info("charge current is set 0mA, turn off charging!\n");
	} else {
		/*set CC charge current */
		ret = di->ops->set_charge_current(di->charge_current);
		if (ret > 0) {
			hwlog_info("charge current is out of range:%dmA!!\n",
				   ret);
			di->ops->set_charge_current(ret);
		} else if (ret < 0)
			hwlog_err("set charge current fail!\n");
		/*set CV terminal voltage */
		if (strstr(saved_command_line, "androidboot.swtype=factory")
			&& (!is_hisi_battery_exist())) {
			vterm = hisi_battery_vbat_max();
			hwlog_info("facory_version and battery not exist, vterm is set to %d\n", vterm);
		} else {
			vterm =
				((di->core_data->vterm < di->sysfs_data.vterm_rt) ?
				di->core_data->vterm : di->sysfs_data.vterm_rt);
		}
		ret = di->ops->set_terminal_voltage(vterm);
		if (ret > 0) {
			hwlog_info("terminal voltage is out of range:%dmV!!\n",
				   ret);
			di->ops->set_terminal_voltage(ret);
		} else if (ret < 0)
			hwlog_err("set terminal voltage fail!\n");
	}
	/*enable/disable charge */
	di->charge_enable &= di->sysfs_data.charge_enable;
	di->charge_enable &= term_err_chrg_en_flag;
	ret = di->ops->set_charge_enable(di->charge_enable);
	if ( !di->sysfs_data.charge_enable ) {
		hwlog_info("Disable flags: sysnode = %d, isc = %d",
			        di->sysfs_data.disable_charger[CHARGER_SYS_NODE],
			        di->sysfs_data.disable_charger[CHARGER_FATAL_ISC_TYPE]);
	}
	if (ret)
		hwlog_err("set charge enable fail!!\n");
	hwlog_debug
	    ("input_current is [%d],charge_current is [%d],terminal_voltage is [%d],charge_enable is [%d]\n",
	     di->input_current, di->charge_current, vterm, di->charge_enable);
}

/**********************************************************
*  Function:       water_check
*  Description:   water check and its operations
*  Parameters:   charge_device_info
*  return value:  NULL
**********************************************************/
static void water_check(struct charge_device_info *di)
{
#ifdef CONFIG_HUAWEI_WATER_CHECK
	if (strstr(saved_command_line, "androidboot.mode=charger")
		||strstr(saved_command_line, "androidboot.swtype=factory"))
		return;
	if(is_water_intrused()){
		di->sysfs_data.water_intrused = 1;
		hisi_coul_charger_event_rcv(VCHRG_STATE_WATER_INTRUSED);
	}
#endif

	if(!di->water_check_enabled) /*if disabled, don't start water check*/
		return;
	/*in shutdown charge mode or factory version, don't start water check*/
	if (strstr(saved_command_line, "androidboot.mode=charger")
		||strstr(saved_command_line, "androidboot.swtype=factory"))
		return;
	if (CHARGER_TYPE_NON_STANDARD == di->charger_type) {
		if (di->sw_ops && di->sw_ops->is_water_intrused
			&& di->sw_ops->is_water_intrused()) {
			hwlog_info("start protection against water intrusion.\n");
			power_dsm_dmd_report(POWER_DSM_BATTERY, ERROR_NO_WATER_CHECK_IN_USB, "water check is triggered");
			di->sysfs_data.water_intrused = 1;
			hisi_coul_charger_event_rcv(VCHRG_STATE_WATER_INTRUSED);
		}
	}
}
/*lint -restore*/

/**********************************************************
*  Function:       charge_safe_protect
*  Discription:    do safe protect ops
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static void charge_safe_protect(struct charge_device_info *di)
{
	if (NULL == di) {
		hwlog_err("%s:NULL pointer!!\n", __func__);
		return;
	}
	if (cancel_work_flag) {
		hwlog_info("[%s] charge already stop\n", __func__);
		return;
	}
	/*do soft ovp protect for 5V/9V charge*/
	if (di->ops && di->ops->soft_vbatt_ovp_protect) {
		di->ops->soft_vbatt_ovp_protect();
	}
	check_ibias_current_safe(di);
}

/**********************************************************
*  Function:       charge_monitor_work
*  Description:    monitor the charging process
*  Parameters:   work:charge workqueue
*  return value:  NULL
**********************************************************/

/*lint -save -e* */
static void charge_monitor_work(struct work_struct *work)
{
	struct charge_device_info *di =
	    container_of(work, struct charge_device_info, charge_work.work);
	charge_kick_watchdog(di);
#ifdef CONFIG_DIRECT_CHARGER
	if (NOT_IN_SCP_CHARGING_STAGE == is_in_scp_charging_stage())
	{
#endif
		/* update type before get params */
		charge_update_charger_type(di);
		if (di->core_data->typec_support) {	/*only the typec is supported ,we need read typec result */
#ifdef CONFIG_HUAWEI_TYPEC
			di->typec_current_mode = typec_current_mode_detect();
#endif
		}
#ifdef CONFIG_DIRECT_CHARGER
		if (di->charger_type == CHARGER_TYPE_STANDARD)
		{
			direct_charge_check();
		}
	}

	if (NOT_IN_SCP_CHARGING_STAGE == is_in_scp_charging_stage())
	{
#endif
		pd_charge_check(di);
		fcp_charge_check(di);

		di->core_data = charge_core_get_params();
		if (NULL == di->core_data) {
			hwlog_err("[%s], di->core_data is NULL\n", __func__);
			return;
		}
		charge_select_charging_current(di);

		charge_turn_on_charging(di);
		charge_safe_protect(di);

		charge_full_handle(di);
		charge_update_vindpm(di);
		charge_update_external_setting(di);

		charge_update_status(di);
		charge_kick_watchdog(di);
#ifdef CONFIG_DIRECT_CHARGER
	}
#endif

	schedule_delayed_work(&di->charge_work,
			      msecs_to_jiffies(CHARGING_WORK_TIMEOUT));
}

/**********************************************************
*  Function:       charge_usb_work
*  Description:    handler interface by different charger_type
*  Parameters:   work:usb workqueue
*  return value:  NULL
**********************************************************/
static void charge_usb_work(struct work_struct *work)
{
	struct charge_device_info *di =
	    container_of(work, struct charge_device_info, usb_work);

#ifdef CONFIG_TCPC_CLASS
	if(charger_pd_support)
		mutex_lock(&di->tcpc_otg_lock);
#endif
	water_check(di);

	switch (di->charger_type) {
	case CHARGER_TYPE_USB:
		hwlog_info("case = CHARGER_TYPE_USB-> \n");
		charge_start_charging(di);
		break;
	case CHARGER_TYPE_NON_STANDARD:
		hwlog_info("case = CHARGER_TYPE_NON_STANDARD -> \n");
		charge_start_charging(di);
		break;
	case CHARGER_TYPE_BC_USB:
		hwlog_info("case = CHARGER_TYPE_BC_USB -> \n");
		charge_start_charging(di);
		break;
	case CHARGER_TYPE_STANDARD:
		hwlog_info("case = CHARGER_TYPE_STANDARD-> \n");
		charge_start_charging(di);
		break;
	case CHARGER_REMOVED:
		hwlog_info("case = USB_EVENT_NONE-> \n");
		charge_stop_charging(di);
		break;
	case USB_EVENT_OTG_ID:
		hwlog_info("case = USB_EVENT_OTG_ID-> \n");
		charge_otg_mode_enable(OTG_ENABLE, OTG_CTRL_WIRED_OTG);
		break;
	case CHARGER_TYPE_PD:
		hwlog_info("case = CHARGER_TYPE_PD-> \n");
		charge_start_charging(di);
		break;
	case CHARGER_TYPE_WIRELESS:
		hwlog_info("case = CHARGER_TYPE_WIRELESS-> \n");
		charge_start_charging(di);
		break;
	default:
		break;
	}
#ifdef CONFIG_TCPC_CLASS
	if(charger_pd_support)
		mutex_unlock(&di->tcpc_otg_lock);
#endif
}
/*lint -restore*/

/**********************************************************
*  Function:       charge_process_vr_charge_event
*  Description:    deal with vr charge events
*  Parameters:     charge_device_info * di
*  return value:   NULL
**********************************************************/
static void charge_process_vr_charge_event(struct charge_device_info *di)
{
	charge_stop_charging(di);
	charge_wake_lock();

	switch (di->sysfs_data.vr_charger_type) {
	case CHARGER_TYPE_SDP:
		di->charger_type = CHARGER_TYPE_USB;
		di->charger_source = POWER_SUPPLY_TYPE_USB;
		charge_send_uevent(di);
		charge_start_charging(di);
		break;
	case CHARGER_TYPE_CDP:
		di->charger_type = CHARGER_TYPE_BC_USB;
		di->charger_source = POWER_SUPPLY_TYPE_USB;
		charge_send_uevent(di);
		charge_start_charging(di);
		break;
	case CHARGER_TYPE_DCP:
		di->charger_type = CHARGER_TYPE_VR;
		di->charger_source = POWER_SUPPLY_TYPE_MAINS;
		charge_send_uevent(di);
		charge_start_charging(di);
		break;
	case CHARGER_TYPE_UNKNOWN:
		di->charger_type = CHARGER_TYPE_NON_STANDARD;
		di->charger_source = POWER_SUPPLY_TYPE_MAINS;
		charge_send_uevent(di);
		charge_start_charging(di);
		break;
	case CHARGER_TYPE_NONE:
		di->charger_type = USB_EVENT_OTG_ID;
		di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
		charge_send_uevent(di);
		charge_wake_unlock();
		charge_otg_mode_enable(OTG_ENABLE, OTG_CTRL_WIRED_OTG);
		break;
	default:
		hwlog_info("Invalid vr charger type! vr_charge_type = %d\n",
			   di->sysfs_data.vr_charger_type);
		break;
	}
}

/**********************************************************
*  Function:       charge_resume_wakelock_work
*  Description:    apply wake_lock when resume
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/

/*lint -save -e* */
static void charge_resume_wakelock_work(void)
{
	mutex_lock(&charge_wakelock_flag_lock);
	if (CHARGE_NEED_WAKELOCK == charge_lock_flag) {
		charge_wake_lock();
		hwlog_info("charge wake lock when resume during charging\n");
	}
	mutex_unlock(&charge_wakelock_flag_lock);
}

static void uscp_plugout_send_uevent(void)
{
	struct charge_device_info *di = g_di;

	if (NULL == di) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return;
	}
	charge_send_uevent(di);
	uscp_plugout_wake_unlock();
}
#ifdef CONFIG_TCPC_CLASS
int is_pd_supported(void)
{
	return charger_pd_support;
}
#endif
static void charge_uevent_process(struct charge_device_info *di, unsigned long event)
{
#ifdef CONFIG_DIRECT_CHARGER
	if (!di->ignore_pluggin_and_plugout_flag && !direct_charge_get_cutoff_normal_flag()) {
#endif

#ifdef  CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
	if (is_in_uscp_mode() && CHARGER_REMOVED == di->charger_type) {
		uscp_plugout_wake_lock();
		schedule_delayed_work(&di->plugout_uscp_work, msecs_to_jiffies(WORK_DELAY_5000MS));
	} else {
		charge_send_uevent(di);
	}
#else
		charge_send_uevent(di);
#endif

#ifdef CONFIG_DIRECT_CHARGER
	}
	else
	{
		hwlog_info("%s ignore evnet : %ld\n", __func__, event);
	}
#endif
}
/*lint -restore*/
#ifdef CONFIG_WIRELESS_CHARGER
static int wireless_charger_vbus_notifier_call(struct notifier_block *wireless_nb,
				    unsigned long event, void *data)
{
	int* vbus = (int *)data;
	int ret;
	struct chip_init_crit init_crit;
	struct charge_device_info *di =
	    container_of(wireless_nb, struct charge_device_info, wireless_nb);
	if (!vbus || !di) {
		hwlog_err("%s para null\n", __func__);
		return NOTIFY_OK;
	}

	if (WIRELESS_CHANNEL_OFF == wireless_charge_get_wireless_channel_state()) {
		hwlog_info("%s not in wireless charging\n", __func__);
		return NOTIFY_OK;
	}
	di->wireless_vbus = *vbus;
	hwlog_info("[%s]: wireless tx_vbus(set) %dmV\n", __func__, di->wireless_vbus);
	if (event == CHARGER_TYPE_WIRELESS) {
		if (di->wireless_vbus >= ADAPTER_12V * MVOLT_PER_VOLT)
			init_crit.vbus = ADAPTER_12V;
		else if (di->wireless_vbus >= ADAPTER_9V * MVOLT_PER_VOLT)
			init_crit.vbus = ADAPTER_9V;
		else
			init_crit.vbus = ADAPTER_5V;
		ret = di->ops->chip_init(&init_crit);
		if (ret)
			hwlog_err("%s: chip init fail!\n", __func__);
		if (di->ops->set_vbus_vset) {
			ret = di->ops->set_vbus_vset(di->wireless_vbus/MVOLT_PER_VOLT);
			if (ret)
				hwlog_err("%s: set vbus_vset fail!\n", __func__);
		}
		mod_delayed_work(system_wq, &di->charge_work, msecs_to_jiffies(0));
	}
	return NOTIFY_OK;
}
#endif
/**********************************************************
*  Function:       charge_usb_notifier_call
*  Description:    respond the charger_type events from USB PHY
*  Parameters:   usb_nb:usb notifier_block
*                      event:charger type event name
*                      data:unused
*  return value:  NOTIFY_OK-success or others
**********************************************************/

/*lint -save -e* */
static int charge_usb_notifier_call(struct notifier_block *usb_nb,
				    unsigned long event, void *data)
{
	struct charge_device_info *di =
	    container_of(usb_nb, struct charge_device_info, usb_nb);
	int filter_flag = TRUE;
	int ret = 0;

	charger_type_ever_notify = true;
	charge_wake_lock();

	filter_flag = charge_rename_charger_type((enum hisi_charger_type)event, di,FALSE);
	if(filter_flag)
	{
		hwlog_info("not use work,filter_flag=%d\n",filter_flag);
		return NOTIFY_OK;
	}

	charge_uevent_process(di, event);

	ret = schedule_work(&di->usb_work);
	if(!ret)
	{
		hwlog_err("usb work state ret = %d\n",ret);
	}
	return NOTIFY_OK;
}
static enum usb_charger_type huawei_get_charger_type(void)
{
	struct charge_device_info *di = g_di;

	if (NULL == di) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return CHARGER_REMOVED;
	}
	return di->charger_type;
}
#ifdef CONFIG_WIRELESS_CHARGER
void wireless_charge_connect_send_uevent(void)
{
	struct charge_device_info *di = g_di;

	if (NULL == di) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return ;
	}
	di->charger_type = CHARGER_TYPE_WIRELESS;
	di->charger_source = POWER_SUPPLY_TYPE_MAINS;
	charge_send_uevent(di);
}
#endif

#ifdef CONFIG_DIRECT_CHARGER
void direct_charger_connect_send_uevent(void)
{
	struct charge_device_info *di = g_di;

	if (NULL == di) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return ;
	}
	di->charger_type = CHARGER_TYPE_STANDARD;
	di->charger_source = POWER_SUPPLY_TYPE_MAINS;
	charge_send_uevent(di);
}
void direct_charger_disconnect_update_charger_type(void)
{
	struct charge_device_info *di = g_di;

	if (NULL == di) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return ;
	}
	di->charger_type = CHARGER_REMOVED;
	di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
}
void direct_charger_disconnect_send_uevent(void)
{
	struct charge_device_info *di = g_di;

	if (NULL == di) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return ;
	}
	charge_send_uevent(di);
}
void ignore_pluggin_and_pluggout_interrupt(void)
{
	struct charge_device_info *di = g_di;

	if (NULL == di) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return ;
	}
	di->ignore_pluggin_and_plugout_flag = 1;
}
void restore_pluggin_pluggout_interrupt(void)
{
	struct charge_device_info *di = g_di;

	if (NULL == di) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return ;
	}
	di->ignore_pluggin_and_plugout_flag = 0;
}
int get_direct_charge_flag(void)
{
	struct charge_device_info *di = g_di;

	if (NULL == di)
		return 0;
	return di->ignore_pluggin_and_plugout_flag;
}
#endif

static int huawei_get_charge_current_max(void)
{
	struct charge_device_info *di = g_di;

	if (NULL == di) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return 0;
	}
	return di->charge_current;
}
int set_otg_switch_mode_enable(void)
{
	struct charge_device_info *di = g_di;

	if (NULL == di || NULL == di->ops || NULL == di->ops->set_otg_switch_mode_enable) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return 0;
	}
        di->ops->set_otg_switch_mode_enable(1);
        return 0;
}
EXPORT_SYMBOL(set_otg_switch_mode_enable);

int huawei_charger_get_dieid(char *dieid, unsigned int len)
{
	struct charge_device_info *di = g_di;
	if (di && di->ops && di->ops->get_dieid) {
		return di->ops->get_dieid(dieid, len);
	}

	return -1;
}
EXPORT_SYMBOL(huawei_charger_get_dieid);

/**********************************************************
*  Function:       charge_fault_notifier_call
*  Description:    respond the fault events from chargerIC
*  Parameters:   fault_nb:fault notifier_block
*                      event:fault event name
*                      data:unused
*  return value:  NOTIFY_OK-success or others
**********************************************************/
static int charge_fault_notifier_call(struct notifier_block *fault_nb,
				      unsigned long event, void *data)
{
	struct charge_device_info *di =
	    container_of(fault_nb, struct charge_device_info, fault_nb);

	di->charge_fault = (enum charge_fault_type)event;
	schedule_work(&di->fault_work);
	return NOTIFY_OK;
}
/*lint -restore*/


#ifdef CONFIG_TCPC_CLASS

/*lint -save -e* */
static void charge_rename_pd_charger_type(enum hisi_charger_type type,
				       struct charge_device_info *di,
					bool ext_power)
{
	switch (type) {
	case PD_DPM_VBUS_TYPE_PD:
		di->charger_type = CHARGER_TYPE_PD;
		hwlog_info("%s is ext_power %d", __func__, ext_power);
		if(ext_power)
			di->charger_source = POWER_SUPPLY_TYPE_MAINS;
		else
			di->charger_source = POWER_SUPPLY_TYPE_USB;
		break;
	case PD_DPM_VBUS_TYPE_TYPEC:
		di->charger_type = CHARGER_TYPE_TYPEC;
		di->charger_source = POWER_SUPPLY_TYPE_MAINS;
		break;
	default:
		di->charger_type = CHARGER_REMOVED;
		di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
		hwlog_info("%s default type %u", __func__, type);
		break;
	}
}
static void pd_typec_current_handler(void)
{
	struct charge_device_info *di = g_di;
	if (NULL == di)
		return;
	charge_select_charging_current(di);
	hwlog_info("%s input_current %u", __func__, di->input_current);
	charge_set_input_current(di->input_current);
}
static void charger_type_handler(enum usb_charger_type type, void*data)
{
	struct charge_device_info *di = g_di;
	struct pd_dpm_vbus_state *vbus_state;
	int need_resched_work = 0;
	if (NULL == di)
		return;
	switch (type) {
	case CHARGER_TYPE_SDP:
		if(CHARGER_TYPE_USB == di->charger_type ||CHARGER_TYPE_WIRELESS == di->charger_type || CHARGER_REMOVED == di->charger_type)
		{
			di->charger_type = CHARGER_TYPE_USB;
			di->charger_source = POWER_SUPPLY_TYPE_USB;
			hwlog_info("%s case = CHARGER_TYPE_SDP\n", __func__);
			need_resched_work = 1;
		}
		break;
	case CHARGER_TYPE_CDP:
		if(CHARGER_TYPE_NON_STANDARD == di->charger_type || CHARGER_TYPE_WIRELESS == di->charger_type || CHARGER_REMOVED == di->charger_type)
		{
			di->charger_type = CHARGER_TYPE_BC_USB;
			di->charger_source = POWER_SUPPLY_TYPE_USB;
			hwlog_info("%s case = CHARGER_TYPE_CDP\n", __func__);
			need_resched_work = 1;
		}
		break;
	case CHARGER_TYPE_DCP:
		if (CHARGER_TYPE_STANDARD != di->charger_type && CHARGER_TYPE_PD != di->charger_type && CHARGER_TYPE_FCP != di->charger_type) {
			di->charger_type = CHARGER_TYPE_STANDARD;
			di->charger_source = POWER_SUPPLY_TYPE_MAINS;
			if (1 == di->support_standard_ico)
				ico_enable = 1;
			else
				ico_enable = 0;
			need_resched_work = 1;
			hwlog_info("%s case = CHARGER_TYPE_DCP\n", __func__);
		}
		break;
	case CHARGER_TYPE_UNKNOWN:
		if (CHARGER_TYPE_WIRELESS == di->charger_type || CHARGER_TYPE_USB == di->charger_type || CHARGER_REMOVED == di->charger_type)
		{
			di->charger_type = CHARGER_TYPE_NON_STANDARD;
			di->charger_source = POWER_SUPPLY_TYPE_MAINS;
			charger_type_update = FALSE;
			need_resched_work = 1;
			hwlog_info("%s case = CHARGER_TYPE_NONSTANDARD\n", __func__);
		}
		break;
	case PD_DPM_VBUS_TYPE_TYPEC:
		hwlog_debug("%s case = CHARGER_TYPE_TYPEC\n", __func__);
		pd_typec_current_handler();
		return;
	case PD_DPM_VBUS_TYPE_PD:
		vbus_state = (struct pd_dpm_vbus_state *) data;
		di->charger_type = CHARGER_TYPE_PD;
		di->pd_input_current = vbus_state->ma;
		di->pd_charge_current = (vbus_state->mv * vbus_state->ma * charger_pd_cur_trans_ratio) / (100 * di->core_data->vterm);
		if (vbus_state->ext_power)
			di->charger_source = POWER_SUPPLY_TYPE_MAINS;
		else
			di->charger_source = POWER_SUPPLY_TYPE_USB;
		need_resched_work = 1;
		hwlog_info("%s case = CHARGER_TYPE_PD\n", __func__);
		break;
	case CHARGER_TYPE_WIRELESS:
		di->charger_type = CHARGER_TYPE_WIRELESS;
		di->charger_source = POWER_SUPPLY_TYPE_MAINS;
		need_resched_work = 1;
		hwlog_info("%s case = CHARGER_TYPE_WIRELESS\n", __func__);
		break;
	default:
		hwlog_info("%s ignore type = %d\n", __func__, type);
		return;
	}
	charge_send_uevent(di);
	if (need_resched_work)
		mod_delayed_work(system_wq, &di->charge_work, msecs_to_jiffies(0));
}
/**********************************************************
*  Function:       tcpc_notifier_call
*  Description:    respond the Type C port event from TCPC
*  Parameters:   tcpc_nb: tcpc notifier_block
*                      event:				PD_DPM_VBUS_TYPE_PD,
*								PD_DPM_VBUS_TYPE_TYPEC,
*								hisi_charger_type,
*                      data: pointer of struct tcp_notify
*  return value:  NOTIFY_OK-success or others
**********************************************************/
static int pd_dpm_notifier_call(struct notifier_block *tcpc_nb, unsigned long event, void *data)
{
	bool ret;
	struct pd_dpm_vbus_state *vbus_state;
	struct charge_device_info *di = container_of(tcpc_nb, struct charge_device_info, tcpc_nb);
	hwlog_info("%s ++ , event = %d\n", __func__, event);
	charger_type_ever_notify = true;

	if (PD_DPM_VBUS_TYPE_PD == event)
		pd_charge_flag = false;
	if (!pmic_vbus_irq_is_enabled())
	{
		mutex_lock(&di->event_type_lock);
		if (di->event == START_SINK) {
			charger_type_handler(event,data);
		}
		mutex_unlock(&di->event_type_lock);
		return NOTIFY_OK;
	}
	if (PD_DPM_VBUS_TYPE_TYPEC == event) {
		hwlog_info("%s typec_notify\n", __func__);
		pd_typec_current_handler();
		return NOTIFY_OK;
	}

	if (PD_DPM_VBUS_TYPE_PD == event)
	{
		vbus_state = (struct pd_dpm_vbus_state *) data;
		di->pd_input_current = vbus_state->ma;
		di->pd_charge_current = (vbus_state->mv * vbus_state->ma * charger_pd_cur_trans_ratio) / (100 * di->core_data->vterm);
		charge_rename_pd_charger_type(event, di, vbus_state->ext_power);
	}
	else
	{
		ret = charge_rename_charger_type(event, di,FALSE);
		if (ret) {
			hwlog_info("unbalanced charger type notify\n");
			return NOTIFY_OK;
		}
	}
	charge_wake_lock();

	charge_uevent_process(di, event);

	if (event == PLEASE_PROVIDE_POWER) {
		hwlog_info("case = USB_EVENT_OTG_ID-> (IM)\n");
		charge_wake_unlock();
		mutex_lock(&di->tcpc_otg_lock);
		charge_otg_mode_enable(OTG_ENABLE, OTG_CTRL_WIRED_OTG);
		mutex_unlock(&di->tcpc_otg_lock);
	} else if (di->charger_type == CHARGER_REMOVED && otg_flag) {
		hwlog_info("case = USB_EVENT_NONE-> (IM)\n");
		mutex_lock(&di->tcpc_otg_lock);
		charge_stop_charging(di);
		mutex_unlock(&di->tcpc_otg_lock);
	} else {
		schedule_work(&di->usb_work);
	}

	return NOTIFY_OK;
}

#endif
/*lint -restore*/
#ifdef CONFIG_HUAWEI_TYPEC
static int typec_current_notifier_call(struct notifier_block *typec_nb, unsigned long event, void *data)
{
	struct charge_device_info *di = container_of(typec_nb, struct charge_device_info, typec_nb);
	if(!di) {
		return NOTIFY_OK;
	}

	if(TYPEC_CURRENT_CHANGE == event) {
		di->typec_current_mode = typec_current_mode_detect();

		charge_select_charging_current(di);
		hwlog_info("%s input_current %u\n", __func__, di->input_current);
		charge_set_input_current(di->input_current);
	}
	return NOTIFY_OK;
}
#endif

/*lint -save -e* */
#ifdef CONFIG_SYSFS
#define CHARGE_SYSFS_FIELD(_name, n, m, store)                \
{                                                   \
    .attr = __ATTR(_name, m, charge_sysfs_show, store),    \
    .name = CHARGE_SYSFS_##n,          \
}

#define CHARGE_SYSFS_FIELD_RW(_name, n)               \
	CHARGE_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO, charge_sysfs_store)

#define CHARGE_SYSFS_FIELD_RO(_name, n)               \
	CHARGE_SYSFS_FIELD(_name, n, S_IRUGO, NULL)

static ssize_t charge_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf);
static ssize_t charge_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count);

struct charge_sysfs_field_info {
	struct device_attribute attr;
	u8 name;
};

static struct charge_sysfs_field_info charge_sysfs_field_tbl[] = {
	CHARGE_SYSFS_FIELD_RW(adc_conv_rate, ADC_CONV_RATE),
	CHARGE_SYSFS_FIELD_RW(iin_thermal, IIN_THERMAL),
	CHARGE_SYSFS_FIELD_RW(ichg_thermal, ICHG_THERMAL),
	CHARGE_SYSFS_FIELD_RW(iin_thermal_aux, IIN_THERMAL_AUX),
	CHARGE_SYSFS_FIELD_RW(ichg_thermal_aux, ICHG_THERMAL_AUX),
	/*iin_runningtest will be used for running test and audio test */
	CHARGE_SYSFS_FIELD_RW(iin_runningtest, IIN_RUNNINGTEST),
	CHARGE_SYSFS_FIELD_RW(ichg_runningtest, ICHG_RUNNINGTEST),
	CHARGE_SYSFS_FIELD_RW(enable_charger, ENABLE_CHARGER),
	CHARGE_SYSFS_FIELD_RW(limit_charging, LIMIT_CHARGING),
	CHARGE_SYSFS_FIELD_RW(regulation_voltage, REGULATION_VOLTAGE),
	CHARGE_SYSFS_FIELD_RW(shutdown_q4, BATFET_DISABLE),
	CHARGE_SYSFS_FIELD_RW(shutdown_watchdog, WATCHDOG_DISABLE),
	CHARGE_SYSFS_FIELD_RO(chargelog, CHARGELOG),
	CHARGE_SYSFS_FIELD_RO(chargelog_head, CHARGELOG_HEAD),
	CHARGE_SYSFS_FIELD_RO(Ibus, IBUS),
	CHARGE_SYSFS_FIELD_RO(Vbus, VBUS),
	CHARGE_SYSFS_FIELD_RW(enable_hiz, HIZ),
	CHARGE_SYSFS_FIELD_RO(chargerType, CHARGE_TYPE),
	CHARGE_SYSFS_FIELD_RO(charge_done_status, CHARGE_DONE_STATUS),
	CHARGE_SYSFS_FIELD_RW(charge_done_sleep_status, CHARGE_DONE_SLEEP_STATUS),
	CHARGE_SYSFS_FIELD_RW(inputcurrent, INPUTCURRENT),
	CHARGE_SYSFS_FIELD_RO(voltage_sys, VOLTAGE_SYS),
	CHARGE_SYSFS_FIELD_RO(bootloader_charger_info, BOOTLOADER_CHARGER_INFO),
	CHARGE_SYSFS_FIELD_RO(ichg_reg, ICHG_REG),
	CHARGE_SYSFS_FIELD_RO(ichg_adc, ICHG_ADC),
	CHARGE_SYSFS_FIELD_RO(ichg_reg_aux, ICHG_REG_AUX),
	CHARGE_SYSFS_FIELD_RO(ichg_adc_aux, ICHG_ADC_AUX),
	CHARGE_SYSFS_FIELD_RW(vr_charger_type, VR_CHARGER_TYPE),
	CHARGE_SYSFS_FIELD_RW(support_ico,    SUPPORT_ICO),
	CHARGE_SYSFS_FIELD_RW(water_intrused,    WATER_INTRUSED),
	CHARGE_SYSFS_FIELD_RO(charge_term_volt_design, CHARGE_TERM_VOLT_DESIGN),
	CHARGE_SYSFS_FIELD_RO(charge_term_curr_design, CHARGE_TERM_CURR_DESIGN),
	CHARGE_SYSFS_FIELD_RO(charge_term_volt_setting, CHARGE_TERM_VOLT_SETTING),
	CHARGE_SYSFS_FIELD_RO(charge_term_curr_setting, CHARGE_TERM_CURR_SETTING),
	CHARGE_SYSFS_FIELD_RO(fcp_support, FCP_SUPPORT),
	/*lint -save -e846 -e514 -e778 -e866 -e84*/
	CHARGE_SYSFS_FIELD_RW(dbc_charge_control, DBC_CHARGE_CONTROL),
	CHARGE_SYSFS_FIELD_RO(dbc_charge_done, DBC_CHARGE_DONE),
	/*lint -restore*/
	CHARGE_SYSFS_FIELD_RW(adaptor_test, ADAPTOR_TEST),
	CHARGE_SYSFS_FIELD_RW(adaptor_voltage, ADAPTOR_VOLTAGE),
	CHARGE_SYSFS_FIELD_RW(plugusb, PLUGUSB),
	CHARGE_SYSFS_FIELD_RW(thermal_reason, THERMAL_REASON),
};

static struct attribute *charge_sysfs_attrs[ARRAY_SIZE(charge_sysfs_field_tbl) + 1];

static const struct attribute_group charge_sysfs_attr_group = {
	.attrs = charge_sysfs_attrs,
};


/**********************************************************
*  Function:       charge_sysfs_init_attrs
*  Description:    initialize charge_sysfs_attrs[] for charge attribute
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void charge_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(charge_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		charge_sysfs_attrs[i] = &charge_sysfs_field_tbl[i].attr.attr;
	}
	charge_sysfs_attrs[limit] = NULL;	/* Has additional entry for this */
}

/**********************************************************
*  Function:       charge_sysfs_field_lookup
*  Description:    get the current device_attribute from charge_sysfs_field_tbl by attr's name
*  Parameters:   name:device attribute name
*  return value:  charge_sysfs_field_tbl[]
**********************************************************/
static struct charge_sysfs_field_info *charge_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(charge_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		if (!strncmp
		    (name, charge_sysfs_field_tbl[i].attr.attr.name,
		     strlen(name)))
			break;
	}
	if (i >= limit)
		return NULL;

	return &charge_sysfs_field_tbl[i];
}
/*lint -restore*/

/**********************************************************
*  Function:       charge_sysfs_show
*  Description:    show the value for all charge device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static ssize_t charge_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct charge_sysfs_field_info *info = NULL;
	struct charge_device_info *di = dev_get_drvdata(dev);
	int ret;

	info = charge_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
	case CHARGE_SYSFS_ADC_CONV_RATE:
		return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.adc_conv_rate);
	case CHARGE_SYSFS_IIN_THERMAL:
		if (di->is_dual_charger == 0)
			return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.iin_thl);
		else
			return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.iin_thl_main);
	case CHARGE_SYSFS_ICHG_THERMAL:
		if (di->is_dual_charger == 0)
			return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.ichg_thl);
		else
			return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.ichg_thl_main);
	case CHARGE_SYSFS_IIN_THERMAL_AUX:
		return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.iin_thl_aux);
	case CHARGE_SYSFS_ICHG_THERMAL_AUX:
		return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.ichg_thl_aux);
	case CHARGE_SYSFS_ICHG_REG:
		if (di->is_dual_charger == 0)
			return -EINVAL;
		return snprintf(buf, PAGE_SIZE, "%d\n", di->ops->get_ichg_reg(MAIN_CHARGER));
	case CHARGE_SYSFS_ICHG_ADC:
		if (di->is_dual_charger == 0)
			return -EINVAL;
		return snprintf(buf, PAGE_SIZE, "%d\n", di->ops->get_ichg_adc(MAIN_CHARGER));
	case CHARGE_SYSFS_ICHG_REG_AUX:
		if (di->is_dual_charger == 0)
			return -EINVAL;
		return snprintf(buf, PAGE_SIZE, "%d\n", di->ops->get_ichg_reg(AUX_CHARGER));
	case CHARGE_SYSFS_ICHG_ADC_AUX:
		if (di->is_dual_charger == 0)
			return -EINVAL;
		return snprintf(buf, PAGE_SIZE, "%d\n", di->ops->get_ichg_adc(AUX_CHARGER));
	case CHARGE_SYSFS_IIN_RUNNINGTEST:
		return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.iin_rt);
	case CHARGE_SYSFS_ICHG_RUNNINGTEST:
		return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.ichg_rt);
	case CHARGE_SYSFS_ENABLE_CHARGER:
		return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.charge_enable);
	case CHARGE_SYSFS_LIMIT_CHARGING:
		return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.charge_limit);
	case CHARGE_SYSFS_REGULATION_VOLTAGE:
		return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.vterm_rt);
	case CHARGE_SYSFS_BATFET_DISABLE:
		return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.batfet_disable);
	case CHARGE_SYSFS_WATCHDOG_DISABLE:
		return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.wdt_disable);
	case CHARGE_SYSFS_CHARGELOG:
		if (BAT_BOARD_UDP == is_board_type) {
			ret = snprintf(buf, PAGE_SIZE, "%s\n", "");
			return ret;
		}
		mutex_lock(&di->sysfs_data.dump_reg_lock);
		di->ops->dump_register(di->sysfs_data.reg_value);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", di->sysfs_data.reg_value);
		mutex_unlock(&di->sysfs_data.dump_reg_lock);
		return ret;
	case CHARGE_SYSFS_CHARGELOG_HEAD:
		mutex_lock(&di->sysfs_data.dump_reg_head_lock);
		di->ops->get_register_head(di->sysfs_data.reg_head);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", di->sysfs_data.reg_head);
		mutex_unlock(&di->sysfs_data.dump_reg_head_lock);
		return ret;
	case CHARGE_SYSFS_IBUS:
		di->sysfs_data.ibus = 0;
		if (di->ops->get_ibus)	/*this is an optional interface for charger*/
			di->sysfs_data.ibus = di->ops->get_ibus();
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_data.ibus);
	case CHARGE_SYSFS_VBUS:
		di->sysfs_data.vbus = 0;
		if (di->ops->get_vbus)	 { /*this is an optional interface for charger*/
			ret = di->ops->get_vbus(&di->sysfs_data.vbus);
			if (ret)
				hwlog_err("[%s]vbus read failed \n", __func__);
		}
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_data.vbus);
	case CHARGE_SYSFS_HIZ:
		return snprintf(buf, PAGE_SIZE, "%u\n", charge_get_hiz_state()? HIZ_MODE_ENABLE : HIZ_MODE_DISABLE);
	case CHARGE_SYSFS_CHARGE_TYPE:
		if(get_fcp_charging_flag() != 0)
			return snprintf(buf, PAGE_SIZE, "%d\n", CHARGER_TYPE_FCP);
		else
			return snprintf(buf, PAGE_SIZE, "%d\n", di->charger_type);
	case CHARGE_SYSFS_CHARGE_DONE_STATUS:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_data.charge_done_status);
	case CHARGE_SYSFS_CHARGE_DONE_SLEEP_STATUS:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_data.charge_done_sleep_status);
	case CHARGE_SYSFS_INPUTCURRENT:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_data.inputcurrent);
		break;
	case CHARGE_SYSFS_SUPPORT_ICO:
		return snprintf(buf,PAGE_SIZE, "%d\n", di->sysfs_data.support_ico);
	case CHARGE_SYSFS_WATER_INTRUSED:
		ret = snprintf(buf,PAGE_SIZE, "%d\n", di->sysfs_data.water_intrused);
		if(1 == di->sysfs_data.water_intrused && 1 == di->clear_water_intrused_flag_after_read)
			di->sysfs_data.water_intrused = 0;
		return ret;
	case CHARGE_SYSFS_VOLTAGE_SYS:
		di->sysfs_data.voltage_sys = 0;
		if (di->ops->get_vbat_sys)
			di->sysfs_data.voltage_sys = di->ops->get_vbat_sys();
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_data.voltage_sys);
		break;
	case CHARGE_SYSFS_BOOTLOADER_CHARGER_INFO:
		return snprintf(buf, PAGE_SIZE, "\n");
	case CHARGE_SYSFS_VR_CHARGER_TYPE:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_data.vr_charger_type);
	case CHARGE_SYSFS_CHARGE_TERM_VOLT_DESIGN:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->core_data->vterm);
	case CHARGE_SYSFS_CHARGE_TERM_CURR_DESIGN:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->core_data->iterm);
	case CHARGE_SYSFS_CHARGE_TERM_VOLT_SETTING:
		return snprintf(buf, PAGE_SIZE, "%d\n",
			((di->core_data->vterm < di->sysfs_data.vterm_rt) ? di->core_data->vterm : di->sysfs_data.vterm_rt));
	case CHARGE_SYSFS_CHARGE_TERM_CURR_SETTING:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->core_data->iterm);
	case CHARGE_SYSFS_FCP_SUPPORT:
		di->sysfs_data.fcp_support = fcp_support_show();
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_data.fcp_support);
	case CHARGE_SYSFS_DBC_CHARGE_CONTROL:
		if (di->ops->set_force_term_enable) {
			return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_data.dbc_charge_control);
		}
		break;
	case CHARGE_SYSFS_DBC_CHARGE_DONE:
		if (di->ops->get_charger_state) {
			return snprintf(buf, PAGE_SIZE, "%d\n", di->ops->get_charger_state());
		}
		break;
	case CHARGE_SYSFS_ADAPTOR_TEST:
		ret = chg_get_adaptor_test_result(buf);
		return ret;
	case CHARGE_SYSFS_ADAPTOR_VOLTAGE:
		if(di->reset_adapter)
			return snprintf(buf, PAGE_SIZE, "%d\n", ADAPTER_5V);
		else
			return snprintf(buf, PAGE_SIZE, "%d\n", ADAPTER_9V);
	case CHARGE_SYSFS_PLUGUSB:
		return scnprintf(buf, PAGE_SIZE, "current state: %s\n" "usage: %s\n",
		charger_event_type_string(di->event), "echo startsink/stopsink/startsource/stopsource > plugusb\n");
	case CHARGE_SYSFS_THERMAL_REASON:
		return snprintf(buf, PAGE_SIZE, "%s\n", di->thermal_reason);
	default:
		hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return 0;
}
/*lint -restore*/

/**********************************************************
*  Function:       charge_sysfs_store
*  Description:    set the value for charge_data's node which is can be written
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static ssize_t charge_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct charge_sysfs_field_info *info = NULL;
	struct charge_device_info *di = dev_get_drvdata(dev);
	long val = 0;
	enum charge_status_event events = VCHRG_POWER_NONE_EVENT;
	int ret;
	int batt_temp = DEFAULT_NORMAL_TEMP;

	info = charge_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
		/*NOTICE:
		   it will be charging with default current when the current node has been set to 0/1,
		   include iin_thermal/ichg_thermal/iin_runningtest/ichg_runningtest node
		 */
	case CHARGE_SYSFS_ADC_CONV_RATE:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		di->sysfs_data.adc_conv_rate = val;
		if (di->ops->set_adc_conv_rate)
			di->ops->set_adc_conv_rate(di->sysfs_data.adc_conv_rate);
		hwlog_info("set adc conversion rate mode = %d\n",
			   di->sysfs_data.adc_conv_rate);
		break;
	case CHARGE_SYSFS_IIN_THERMAL:
#ifndef CONFIG_HLTHERM_RUNTEST
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > di->core_data->iin_max))
			return -EINVAL;
		if (di->is_dual_charger == 0) {
			if ((val == 0) || (val == 1))
				di->sysfs_data.iin_thl = di->core_data->iin_max;
			else if ((val > 1) && (val <= 100))
				di->sysfs_data.iin_thl = 100;
			else
				di->sysfs_data.iin_thl = val;
			if (di->input_current > di->sysfs_data.iin_thl) {
				di->input_current = di->sysfs_data.iin_thl;
				charge_set_input_current(di->input_current);
			}
			hwlog_info("THERMAL set input current = %d\n",
				   di->sysfs_data.iin_thl);
		} else {
			if ((val == 0) || (val == 1))
				di->sysfs_data.iin_thl_main = di->core_data->iin_max / 2;
			else if ((val > 1) && (val <= 100))
				di->sysfs_data.iin_thl_main = 100;
			else
				di->sysfs_data.iin_thl_main = val;

			di->sysfs_data.iin_thl =
				di->sysfs_data.iin_thl_main + di->sysfs_data.iin_thl_aux;
			di->ops->set_input_current_thermal(di->sysfs_data.iin_thl_main,
							   di->sysfs_data.iin_thl_aux);
			hwlog_info("THERMAL set input current main = %d\n",
				   di->sysfs_data.iin_thl_main);
		}
#endif
		break;
	case CHARGE_SYSFS_ICHG_THERMAL:
#ifndef CONFIG_HLTHERM_RUNTEST
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > di->core_data->ichg_max))
			return -EINVAL;
		if (di->is_dual_charger == 0) {
			if ((val == 0) || (val == 1))
				di->sysfs_data.ichg_thl = di->core_data->ichg_max;
			else if ((val > 1) && (val <= 500)) {
				hwlog_info
				    ("THERMAL set charge current = %ld is less than 500mA\n", val);
				di->sysfs_data.ichg_thl = 500;
			} else
				di->sysfs_data.ichg_thl = val;

			if (di->charge_current > di->sysfs_data.ichg_thl)
				di->ops->set_charge_current(di->sysfs_data.ichg_thl);
			hwlog_info("THERMAL set charge current = %d\n",
				   di->sysfs_data.ichg_thl);
		} else {
			if ((val == 0) || (val == 1))
				di->sysfs_data.ichg_thl_main =
				    di->core_data->ichg_max;
			else
				di->sysfs_data.ichg_thl_main = val;

			di->sysfs_data.ichg_thl =
				di->sysfs_data.ichg_thl_main + di->sysfs_data.ichg_thl_aux;
			if (di->sysfs_data.ichg_thl <= 500) {
				hwlog_info
				    ("THERMAL set charge current = %u is less than 500mA, main = %u, aux = %u\n",
				     di->sysfs_data.ichg_thl,
				     di->sysfs_data.ichg_thl_main,
				     di->sysfs_data.ichg_thl_aux);
				di->sysfs_data.ichg_thl = 500;
			}
			di->ops->set_charge_current_thermal(di->sysfs_data.ichg_thl_main,
							    di->sysfs_data.ichg_thl_aux);
			hwlog_info("THERMAL set charge current main = %d\n",
				   di->sysfs_data.ichg_thl_main);
		}
#endif
		break;
	case CHARGE_SYSFS_IIN_THERMAL_AUX:
#ifndef CONFIG_HLTHERM_RUNTEST
		if (di->is_dual_charger == 0)
			break;
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 3000))
			return -EINVAL;
		if ((val == 0) || (val == 1))
			di->sysfs_data.iin_thl_aux = di->core_data->iin_max / 2;
		else if ((val > 1) && (val <= 100))
			di->sysfs_data.iin_thl_aux = 100;
		else
			di->sysfs_data.iin_thl_aux = val;

		di->sysfs_data.iin_thl =
		    di->sysfs_data.iin_thl_main + di->sysfs_data.iin_thl_aux;

		di->ops->set_input_current_thermal(di->sysfs_data.iin_thl_main,
						   di->sysfs_data.iin_thl_aux);
		hwlog_info("THERMAL set input current aux = %d\n",
			   di->sysfs_data.iin_thl_aux);
#endif
		break;
	case CHARGE_SYSFS_ICHG_THERMAL_AUX:
#ifndef CONFIG_HLTHERM_RUNTEST
		if (di->is_dual_charger == 0)
			break;
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 3000))
			return -EINVAL;
		if ((val == 0) || (val == 1))
			di->sysfs_data.ichg_thl_aux = di->core_data->ichg_max;
		else
			di->sysfs_data.ichg_thl_aux = val;

		di->sysfs_data.ichg_thl =
		    di->sysfs_data.ichg_thl_main + di->sysfs_data.ichg_thl_aux;
		if (di->sysfs_data.ichg_thl <= 500) {
			hwlog_info
			    ("THERMAL set charge current = %u is less than 500mA, main = %u, aux = %u\n",
			     di->sysfs_data.ichg_thl,
			     di->sysfs_data.ichg_thl_main,
			     di->sysfs_data.ichg_thl_aux);
			di->sysfs_data.ichg_thl = 500;
		}
		di->ops->set_charge_current_thermal(di->sysfs_data.ichg_thl_main,
						    di->sysfs_data.ichg_thl_aux);
		hwlog_info("THERMAL set charge current aux = %d\n",
			   di->sysfs_data.ichg_thl_aux);
#endif
		break;
	case CHARGE_SYSFS_IIN_RUNNINGTEST:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > di->core_data->iin_max))
			return -EINVAL;
		if ((val == 0) || (val == 1))
			di->sysfs_data.iin_rt = di->core_data->iin_max;
		else if ((val > 1) && (val <= 100))
			di->sysfs_data.iin_rt = 100;
		else
			di->sysfs_data.iin_rt = val;

		if (di->input_current > di->sysfs_data.iin_rt) {
			di->input_current = di->sysfs_data.iin_rt;
			charge_set_input_current(di->input_current);
		}
		hwlog_info("RUNNINGTEST set input current = %d\n",
			   di->sysfs_data.iin_rt);
		break;
	case CHARGE_SYSFS_ICHG_RUNNINGTEST:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > di->core_data->ichg_max))
			return -EINVAL;
		if ((val == 0) || (val == 1))
			di->sysfs_data.ichg_rt = di->core_data->ichg_max;
		else if ((val > 1) && (val <= 205))
			di->sysfs_data.ichg_rt = 205;
		else
			di->sysfs_data.ichg_rt = val;

		if (di->charge_current > di->sysfs_data.ichg_rt)
			di->ops->set_charge_current(di->sysfs_data.ichg_rt);
		hwlog_info("RUNNINGTEST set charge current = %d\n",
			   di->sysfs_data.ichg_rt);
		break;
	case CHARGE_SYSFS_ENABLE_CHARGER:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		ret = set_charger_disable_flags(
				val?CHARGER_CLEAR_DISABLE_FLAGS:CHARGER_SET_DISABLE_FLAGS,
				CHARGER_SYS_NODE);
		di->sysfs_data.charge_limit = TRUE;
		/*why should send events in this command?
		   because it will get the /sys/class/power_supply/Battery/status immediately
		   to check if the enable/disable command set successfully or not in some product line station
		 */
		hwlog_info("RUNNINGTEST set charge enable = %d\n", di->sysfs_data.charge_enable);
		batt_temp = hisi_battery_temperature_for_charger();
		if(di->sysfs_data.charge_enable){
			if((batt_temp > BATT_EXIST_TEMP_LOW && batt_temp <= NO_CHG_TEMP_LOW) || batt_temp >= NO_CHG_TEMP_HIGH){
				hwlog_err("battery temp is %d, abandon enable_charge.\n", batt_temp);
				break;
			}
		}
		enum hisi_charger_type type = hisi_get_charger_type();
		if(PLEASE_PROVIDE_POWER == type &&
			CHARGER_TYPE_WIRELESS != di->charger_type)
		{
			hwlog_err("hisi charge type:%d,charger_type: %d, abandon enable_charge.\n",
                        type,di->charger_type);
			break;
		}
		di->ops->set_charge_enable(di->sysfs_data.charge_enable);
#ifdef CONFIG_DIRECT_CHARGER
		if (NOT_IN_SCP_CHARGING_STAGE == is_in_scp_charging_stage()) {
#endif
			if (hisi_pmic_get_vbus_status() &&
				CHARGER_REMOVED != di->charger_type) {
				if (di->sysfs_data.charge_enable)
					events = VCHRG_START_CHARGING_EVENT;
				else
					events = VCHRG_NOT_CHARGING_EVENT;
				hisi_coul_charger_event_rcv(events);
			}
#ifdef CONFIG_DIRECT_CHARGER
		}
#endif
		break;
	case CHARGE_SYSFS_LIMIT_CHARGING:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		di->sysfs_data.charge_limit = val;
		hwlog_info("PROJECTMUNE set limit charge enable = %d\n",
			   di->sysfs_data.charge_limit);
		break;
	case CHARGE_SYSFS_REGULATION_VOLTAGE:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 3200) || (val > 4400))
			return -EINVAL;
		di->sysfs_data.vterm_rt = val;
		di->ops->set_terminal_voltage(di->sysfs_data.vterm_rt);
		hwlog_info("RUNNINGTEST set terminal voltage = %d\n",
			   di->sysfs_data.vterm_rt);
		break;
	case CHARGE_SYSFS_BATFET_DISABLE:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		di->sysfs_data.batfet_disable = val;
		if (1 == val) {
			di->input_current = CHARGE_CURRENT_2000_MA;
			charge_set_input_current(di->input_current);
		}

		di->ops->set_batfet_disable(di->sysfs_data.batfet_disable);
		hwlog_info("RUNNINGTEST set batfet disable = %d\n",
			   di->sysfs_data.batfet_disable);
		break;
	case CHARGE_SYSFS_WATCHDOG_DISABLE:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		di->sysfs_data.wdt_disable = val;
		if (val == 1)
			di->ops->set_watchdog_timer(WATCHDOG_TIMER_DISABLE);
		hwlog_info("RUNNINGTEST set wdt disable = %d\n",
			   di->sysfs_data.wdt_disable);
		break;
	case CHARGE_SYSFS_HIZ:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		hwlog_info("RUNNINGTEST set hiz enable = %d\n", val);
		charge_set_hiz_enable(val? HIZ_MODE_ENABLE: HIZ_MODE_DISABLE);
		break;
	case CHARGE_SYSFS_CHARGE_DONE_STATUS:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 10))
			return -EINVAL;
		di->sysfs_data.charge_done_status = val;
		break;
	case CHARGE_SYSFS_CHARGE_DONE_SLEEP_STATUS:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 10))
			return -EINVAL;
		di->sysfs_data.charge_done_sleep_status = val;
		break;
	case CHARGE_SYSFS_INPUTCURRENT:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 100) || (val > 2000))
			return -EINVAL;
		di->sysfs_data.inputcurrent = val;
		di->input_current = di->sysfs_data.inputcurrent;
		charge_set_input_current(di->input_current);
		hwlog_info("set input currrent is: %ld\n", val);
		break;
	case CHARGE_SYSFS_SUPPORT_ICO:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		di->sysfs_data.support_ico = val;
		hwlog_info("SUPPORT_ICO = %d\n", di->sysfs_data.support_ico);
		break;
	case CHARGE_SYSFS_WATER_INTRUSED:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		di->sysfs_data.water_intrused = val;
		hwlog_info("set water_intrused = %d\n", di->sysfs_data.water_intrused);
		break;
	case CHARGE_SYSFS_VR_CHARGER_TYPE:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 4))
			return -EINVAL;
		di->sysfs_data.vr_charger_type = val;
		hwlog_info("Set vr_charger_type = %d\n",
			   di->sysfs_data.vr_charger_type);
		charge_process_vr_charge_event(di);
		break;
	case CHARGE_SYSFS_DBC_CHARGE_CONTROL:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 2))
			return -EINVAL;
		if (di->ops->set_force_term_enable) {
			ret = di->ops->set_force_term_enable((int)val);
			if (ret)
				hwlog_err("[%s]set force term enable failed \n", __func__);
			di->sysfs_data.dbc_charge_control = (unsigned int)val;
			hwlog_info("set charge term switch:%d\n", di->sysfs_data.dbc_charge_control);
		}
		break;
	case CHARGE_SYSFS_ADAPTOR_TEST:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < MIN_ADAPTOR_TEST_INS_NUM) || (val > MAX_ADAPTOR_TEST_INS_NUM))
			return -EINVAL;
		if(ADAPTOR_TEST_START == val){
			hwlog_info("Reset adaptor test result to FAIL\n");
			clear_adaptor_test_result();
		}
		break;
	case CHARGE_SYSFS_ADAPTOR_VOLTAGE:
		if ((strict_strtol(buf, CHARGE_SYSFS_BUF_SIZE, &val) < 0) || (val < 0))
			return -EINVAL;
		charge_set_adapter_voltage(val, RESET_ADAPTER_SYSFS, PD_VOLTAGE_CHANGE_WORK_TIMEOUT);
		break;
	case CHARGE_SYSFS_PLUGUSB:
		if (!strncmp(buf, "startsink", strlen("startsink"))) {
			hwlog_info("%s:start sink\n", __func__);
			charger_source_sink_event(START_SINK);
		} else if (!strncmp(buf, "stopsink", strlen("stopsink"))) {
			hwlog_info("%s:stop sink\n", __func__);
			charger_source_sink_event(STOP_SINK);
		} else if (!strncmp(buf, "startsource", strlen("startsource"))) {
			hwlog_info("%s:stop sink\n", __func__);
			charger_source_sink_event(START_SOURCE);
		} else if (!strncmp(buf, "stopsource", strlen("stopsource"))) {
			hwlog_info("%s:stop source\n", __func__);
			charger_source_sink_event(STOP_SOURCE);
		} else {
			hwlog_info("%s:error param\n", __func__);
		}
		break;
	case CHARGE_SYSFS_THERMAL_REASON:
		if (THERMAL_REASON_SIZE > strlen(buf))
			snprintf(di->thermal_reason, strlen(buf), "%s", buf);
		hwlog_info("THERMAL set reason = %s, buf = %s\n", di->thermal_reason, buf);
		break;
	default:
		hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return count;
}

static bool charge_check_ts(void)
{
    return ts_flag;
}

static bool charge_check_otg_state(void)
{
    return otg_flag;
}
/*lint -restore*/

/**********************************************************
*  Function:       charge_sysfs_create_group
*  Description:    create the charge device sysfs group
*  Parameters:   di:charge_device_info
*  return value:  0-sucess or others-fail
**********************************************************/
static int charge_sysfs_create_group(struct charge_device_info *di)
{
	charge_sysfs_init_attrs();
	return sysfs_create_group(&di->dev->kobj, &charge_sysfs_attr_group);
}

/**********************************************************
*  Function:       charge_sysfs_remove_group
*  Description:    remove the charge device sysfs group
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static inline void charge_sysfs_remove_group(struct charge_device_info *di)
{
	sysfs_remove_group(&di->dev->kobj, &charge_sysfs_attr_group);
}
#else
static int charge_sysfs_create_group(struct charge_device_info *di)
{
	return 0;
}

static inline void charge_sysfs_remove_group(struct charge_device_info *di)
{
}
#endif

int charge_switch_ops_register(struct charge_switch_ops *ops)
{
	int ret = 0;

	if (ops != NULL) {
		g_sw_ops = ops;
	} else {
		hwlog_err("charge switch ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}

/**********************************************************
*  Function:       fcp_adpter_ops_register
*  Description:    register the handler ops for fcp adpter
*  Parameters:   ops:operations interface of fcp adpter
*  return value:  0-sucess or others-fail
**********************************************************/
int fcp_adapter_ops_register(struct fcp_adapter_device_ops *ops)
{
	int ret = 0;

	if (ops != NULL) {
		g_fcp_ops = ops;
	} else {
		hwlog_err("fcp ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}
/**********************************************************
*  Function:       charge_parse_dts
*  Description:    parse dts
*  Parameters:   charge_device_info di
*  return value:  NULL
**********************************************************/

/*lint -save -e* */
static void charge_parse_dts(struct charge_device_info *di)
{
	int ret = 0;
	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,dual_charger"),
			"is_dual_charger", &di->is_dual_charger);
	if (ret){
		hwlog_err("get is_dual_charger fail!\n");
		di->is_dual_charger = 0;
	}
	hwlog_info("is_dual_charger = %d\n", di->is_dual_charger);

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "hisi,coul_core"),
			"current_full_enable",&di->enable_current_full );
	if(ret){
			hwlog_err("get current_full_enable fail!\n");
			di->enable_current_full = 0;
	}
	hwlog_info("current_full_enable  = %d\n",di->enable_current_full);

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger"),
			"water_check_enabled", &di->water_check_enabled);
	if (ret)
		hwlog_err("get water_check_enabled fail!\n");

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger"),
			"cust_term_exceed_time", &di->term_exceed_time);
	if (ret){
		di->term_exceed_time = TERM_ERR_CNT;
		hwlog_err("get cust_term_exceed_time fail, use default.\n");
	}
	hwlog_info("term_exceed_time = %d\n",di->term_exceed_time);

	//charge_done_maintain_fcp
	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger"),
			"charge_done_maintain_fcp", &di->charge_done_maintain_fcp);
	if (ret){
		hwlog_err("get charge_done_maintain_fcp fail!\n");
		di->charge_done_maintain_fcp = 0;
	}
	hwlog_info("charge_done_maintain_fcp = %d\n", di->charge_done_maintain_fcp);

	charge_done_sleep_dts =
		of_property_read_bool(of_find_compatible_node(NULL, NULL, "huawei,charger"),
			"charge_done_sleep_enabled");

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,hisi_bci_battery"),
			"battery_board_type", &is_board_type);
	if (ret) {
		hwlog_err("get battery_board_type fail!\n");
		is_board_type = BAT_BOARD_ASIC;
	}

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger"), \
			"support_standard_ico", &di->support_standard_ico);
	if (ret) {
		hwlog_err("get support_standard_ico fail!\n");
		di->support_standard_ico = 0;
	}
	hwlog_info("support_standard_ico = %d\n", di->support_standard_ico);

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger"), \
			"ico_all_the_way", &di->ico_all_the_way);
	if (ret) {
		hwlog_err("get ico_all_the_way fail!\n");
		di->ico_all_the_way = 0;
	}
	hwlog_info("ico_all_the_way = %d\n", di->ico_all_the_way);

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger"), \
			"fcp_vindpm", &di->fcp_vindpm);
	if (ret) {
		hwlog_err("get fcp_vindpm fail!\n");
		di->fcp_vindpm = CHARGE_VOLTAGE_4600_MV;
	}
	hwlog_info("fcp_vindpm = %d\n", di->fcp_vindpm);

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger"), \
			"check_ibias_sleep_time", &di->check_ibias_sleep_time);
	if (ret) {
		hwlog_err("get check_ibias_sleep_time fail!\n");
		di->check_ibias_sleep_time = SLEEP_110MS;
	}
	hwlog_info("check_ibias_sleep_time= %d\n", di->check_ibias_sleep_time);

#ifdef CONFIG_TCPC_CLASS
	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger"),
                        "pd_cur_trans_ratio", &charger_pd_cur_trans_ratio);
        if (ret) {
                hwlog_err("get charger_pd_cur_trans_ratio fail!\n");
        }
#endif

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger"), \
			"clear_water_intrused_flag_after_read", &di->clear_water_intrused_flag_after_read);
	if (ret) {
		hwlog_err("get clear_water_intrused_flag_after_read fail!\n");
		di->clear_water_intrused_flag_after_read = 1;/*default is clear*/
	}
	hwlog_info("clear_water_intrused_flag_after_read = %d\n", di->clear_water_intrused_flag_after_read);
}

static struct charge_extra_ops huawei_charge_extra_ops = {
	.check_ts = charge_check_ts,
	.check_otg_state = charge_check_otg_state,
	.get_stage = fcp_get_stage,
	.get_charger_type = huawei_get_charger_type,
	.set_state = set_charge_state,
	.get_charge_current = huawei_get_charge_current_max,
};
/*lint -restore*/

static struct charge_device_info *charge_device_info_alloc(void)
{
	struct charge_device_info *di;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("alloc di failed\n");
		return NULL;
	}
	di->sysfs_data.reg_value = kzalloc(sizeof(char) * CHARGELOG_SIZE, GFP_KERNEL);
	if (!di->sysfs_data.reg_value) {
		hwlog_err("alloc reg_value failed\n");
		goto alloc_fail_1;
	}
	di->sysfs_data.reg_head = kzalloc(sizeof(char) * CHARGELOG_SIZE, GFP_KERNEL);
	if (!di->sysfs_data.reg_head) {
		hwlog_err("alloc reg_head failed\n");
		goto alloc_fail_2;
	}
	di->sysfs_data.bootloader_info = kzalloc(sizeof(char) * CHARGELOG_SIZE, GFP_KERNEL);
	if (!di->sysfs_data.bootloader_info) {
		hwlog_err("alloc bootloader_info failed\n");
		goto alloc_fail_3;
	}
	return di;
alloc_fail_3:
	kfree(di->sysfs_data.reg_head);
	di->sysfs_data.reg_head = NULL;
alloc_fail_2:
	kfree(di->sysfs_data.reg_value);
	di->sysfs_data.reg_value = NULL;
alloc_fail_1:
	kfree(di);
	return NULL;
}

static void charge_device_info_free(struct charge_device_info *di)
{
	if(di != NULL) {
		if(di->sysfs_data.bootloader_info != NULL) {
			kfree(di->sysfs_data.bootloader_info);
			di->sysfs_data.bootloader_info = NULL;
		}
		if(di->sysfs_data.reg_head != NULL) {
			kfree(di->sysfs_data.reg_head);
			di->sysfs_data.reg_head = NULL;
		}
		if(di->sysfs_data.reg_value != NULL) {
			kfree(di->sysfs_data.reg_value);
			di->sysfs_data.reg_value = NULL;
		}
		kfree(di);
	}
}

int charge_otg_mode_enable(int enable, enum otg_ctrl_type type)
{
	if (g_di == NULL)
		 return -1;

	if(type < 0 || type >= OTG_CTRL_TOTAL) {
		hwlog_err("%s: type(%d) is invalid!\n", __func__, type);
		return -1;
	}
	if (enable) {
		otg_ctrl_enable |= 1 << type;
		charge_start_usb_otg(g_di);
	} else {
		otg_ctrl_enable &=  ~(1 << type);
		if(otg_ctrl_enable == 0) {
			charge_stop_otg(g_di);
		}
	}
	hwlog_info("[%s] otg_ctrl_enable = 0x%x\n",  __func__, otg_ctrl_enable);
	return 0;
}

/**********************************************************
*  Function:       charge_probe
*  Description:    chargre module probe
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static int charge_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct charge_device_info *di;
	struct device_node *np;
	enum hisi_charger_type type = CHARGER_TYPE_NONE;
	struct class *power_class = NULL;
#ifdef CONFIG_TCPC_CLASS
	struct device_node *hw_charger_node;
	unsigned long local_event;
        struct pd_dpm_vbus_state local_state;
#endif
        int pmic_vbus_irq_enabled = 1;
	di = charge_device_info_alloc();
	if(!di) {
		hwlog_err("alloc di failed\n");
		return -ENOMEM;
	}
	di->core_data = charge_core_get_params();
	if (NULL == di->core_data){
	    hwlog_err("di->core_data is NULL\n");
	    ret = -EINVAL;
	    goto charge_fail_0;
	}
	di->dev = &pdev->dev;
	np = di->dev->of_node;
	di->ops = g_ops;
	di->sw_ops = g_sw_ops;
	di->fcp_ops = g_fcp_ops;
	if ((NULL == di->ops) || (di->ops->chip_init == NULL)
	    || (di->ops->set_input_current == NULL)
	    || (di->ops->set_charge_current == NULL)
	    || (di->ops->set_charge_enable == NULL)
	    || (di->ops->set_otg_enable == NULL)
	    || (di->ops->set_terminal_current == NULL)
	    || (di->ops->set_terminal_voltage == NULL)
	    || (di->ops->dump_register == NULL)
	    || (di->ops->get_charge_state == NULL)
	    || (di->ops->reset_watchdog_timer == NULL)
	    || (di->ops->get_register_head == NULL)) {
		hwlog_err("charge ops is NULL!\n");
		ret = -EINVAL;
		goto charge_fail_1;
	}
	charge_parse_dts(di);
	platform_set_drvdata(pdev, di);

	wake_lock_init(&charge_lock, WAKE_LOCK_SUSPEND, "charge_wakelock");
	wake_lock_init(&otg_lock, WAKE_LOCK_SUSPEND, "otg_wakelock");
	wake_lock_init(&uscp_plugout_lock, WAKE_LOCK_SUSPEND, "uscp_plugout_lock");
	wake_lock_init(&stop_charge_lock, WAKE_LOCK_SUSPEND, "stop_charge_wakelock");

	if(charger_event_queue_create(&di->event_queue, MAX_EVENT_COUNT))
		goto fail_create_event_queue;
	spin_lock_init(&di->event_spin_lock);
	di->event = CHARGER_MAX_EVENT;
	mutex_init(&di->event_type_lock);
	INIT_WORK(&di->event_work, charger_event_work);
	INIT_DELAYED_WORK(&di->charge_work, charge_monitor_work);
	INIT_DELAYED_WORK(&di->otg_work, charge_otg_work);
	INIT_DELAYED_WORK(&di->pd_voltage_change_work,charge_pd_voltage_change_work);
	INIT_WORK(&di->usb_work, charge_usb_work);
	INIT_WORK(&di->fault_work, charge_fault_work);
	INIT_WORK(&resume_wakelock_work, charge_resume_wakelock_work);
#ifdef CONFIG_OTG_GPIO_ID
	if ((ycable != NULL) && ycable->ycable_support) {
		INIT_DELAYED_WORK(&ycable->ycable_work, charge_ycable_work);
	}
#endif
#ifdef  CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
	INIT_DELAYED_WORK(&di->plugout_uscp_work, uscp_plugout_send_uevent);
#endif

	INIT_DELAYED_WORK(&di->vbus_valid_check_work, vbus_valid_check_work);

#ifdef CONFIG_TCPC_CLASS

	hw_charger_node =
	    of_find_compatible_node(NULL, NULL, "huawei,charger");
	if (hw_charger_node) {
		if(of_property_read_u32(hw_charger_node, "pd_support", &charger_pd_support))
		{
			hwlog_err("get pd_support fail!\n");
		}

		hwlog_info("charger_pd_support = %d \n",charger_pd_support);
	} else {
		hwlog_err("get huawei,charger fail!\n");
	}
#ifdef CONFIG_TCPC_CLASS
	if(charger_pd_support)
	{
		mutex_init(&di->tcpc_otg_lock);
		hwlog_info("Register pd_dpm notifier\n");

		di->tcpc_nb.notifier_call = pd_dpm_notifier_call;
		ret = register_pd_dpm_notifier(&di->tcpc_nb);
		if (ret < 0)
		{
			hwlog_err("register_pd_dpm_notifier failed\n");
		}
		else
		{
			hwlog_info("register_pd_dpm_notifier OK\n");
		}
	}
	else
#endif
	{
		di->usb_nb.notifier_call = charge_usb_notifier_call;
		ret = hisi_charger_type_notifier_register(&di->usb_nb);
		if (ret < 0) {
			hwlog_err("hisi_charger_type_notifier_register failed\n");
			goto charge_fail_2;
		}
#ifdef CONFIG_HUAWEI_TYPEC
		di->typec_nb.notifier_call = typec_current_notifier_call;
		ret = typec_current_notifier_register(&di->typec_nb);
		if (ret < 0) {
			hwlog_err("hisi_charger_type_notifier_register failed\n");
		}
#endif
	}

#else
	hwlog_info("Register usb notifier\n");
	di->usb_nb.notifier_call = charge_usb_notifier_call;
	ret = hisi_charger_type_notifier_register(&di->usb_nb);
	if (ret < 0) {
		hwlog_err("hisi_charger_type_notifier_register failed\n");
		goto charge_fail_2;
	}
#ifdef CONFIG_HUAWEI_TYPEC
	di->typec_nb.notifier_call = typec_current_notifier_call;
	ret = typec_current_notifier_register(&di->typec_nb);
	if (ret < 0) {
		hwlog_err("hisi_charger_type_notifier_register failed\n");
	}
#endif
#endif

	di->fault_nb.notifier_call = charge_fault_notifier_call;
	ret =
	    atomic_notifier_chain_register(&fault_notifier_list, &di->fault_nb);
	if (ret < 0) {
		hwlog_err("charge_fault_register_notifier failed\n");
		goto charge_fail_2;
	}

	di->sysfs_data.adc_conv_rate = 0;
	di->sysfs_data.iin_thl = di->core_data->iin_max;
	di->sysfs_data.ichg_thl = di->core_data->ichg_max;
	di->sysfs_data.iin_thl_main = di->core_data->iin_max / 2;
	di->sysfs_data.ichg_thl_main = di->core_data->ichg_max;
	di->sysfs_data.iin_thl_aux = di->core_data->iin_max / 2;
	di->sysfs_data.ichg_thl_aux = di->core_data->ichg_max;
	di->sysfs_data.iin_rt = di->core_data->iin_max;
	di->sysfs_data.ichg_rt = di->core_data->ichg_max;
	di->sysfs_data.vterm_rt = hisi_battery_vbat_max();
	di->sysfs_data.charge_enable = TRUE;
	di->sysfs_data.batfet_disable = FALSE;
	di->sysfs_data.wdt_disable = FALSE;
	di->sysfs_data.charge_limit = TRUE;
	di->sysfs_data.charge_done_status = CHARGE_DONE_NON;
	di->sysfs_data.charge_done_sleep_status = CHARGE_DONE_SLEEP_DISABLED;
	di->sysfs_data.vr_charger_type = CHARGER_TYPE_NONE;
	di->sysfs_data.dbc_charge_control = CHARGER_NOT_DBC_CONTROL;
	di->sysfs_data.support_ico = 1;
	di->charger_type = CHARGER_REMOVED;
	di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
	mutex_init(&di->sysfs_data.dump_reg_lock);
	mutex_init(&di->sysfs_data.dump_reg_head_lock);
	mutex_init(&charge_wakelock_flag_lock);
	mutex_init(&di->sysfs_data.bootloader_info_lock);
	mutex_init(&di->mutex_hiz);
	batt_ifull = hisi_battery_ifull();
	hwlog_info("batt_ifull %d from coul",batt_ifull);
	di->charge_fault = CHARGE_FAULT_NON;
	di->check_full_count = 0;
	di->weaksource_cnt = 0;
	g_di = di;
#ifdef CONFIG_TCPC_CLASS
        pmic_vbus_irq_enabled = pmic_vbus_irq_is_enabled();
#endif
	type = hisi_get_charger_type();
	if (!charger_type_ever_notify && pmic_vbus_irq_enabled)
	{
#ifdef CONFIG_TCPC_CLASS
			if(charger_pd_support)
			{
				pd_dpm_get_charge_event(&local_event, &local_state);
				pd_dpm_notifier_call(&(di->tcpc_nb), local_event, &local_state);
			}
			else
			{
				charge_rename_charger_type(type, di,FALSE);
				charge_send_uevent(di);
				schedule_work(&di->usb_work);
			}
			if(CHARGER_TYPE_USB == di->charger_type)
			{
				charger_type_update = TRUE;
				hwlog_info("get charger type: usb!\n");
			}
#else
			charge_rename_charger_type(type, di,FALSE);
			charge_send_uevent(di);
			schedule_work(&di->usb_work);
#endif
	}
	if (!pmic_vbus_irq_enabled) {
		if (di->event == CHARGER_MAX_EVENT) {
#ifdef CONFIG_WIRELESS_CHARGER
			if (wireless_charge_check_tx_exist()) {
				charger_source_sink_event(START_SINK_WIRELESS);
			} else {
#endif
#ifdef CONFIG_TCPC_CLASS
				if(charger_pd_support) {
					charger_source_sink_event(pd_dpm_get_source_sink_state());
					pd_dpm_get_charge_event(&local_event, &local_state);
					pd_dpm_notifier_call(&(di->tcpc_nb), local_event, &local_state);
				}
#endif
#ifdef CONFIG_WIRELESS_CHARGER
			}
#endif
		}
	}
#ifdef CONFIG_WIRELESS_CHARGER
	di->wireless_nb.notifier_call = wireless_charger_vbus_notifier_call;
	ret = register_wireless_charger_vbus_notifier(&di->wireless_nb);
	if (ret < 0) {
		hwlog_err("register_wireless_charger_notifier failed\n");
	}
#endif

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	/* detect current device successful, set the flag as present */
	if (NULL != di->ops->dev_check) {
		if (CHARGE_IC_GOOD == di->ops->dev_check()) {
			hwlog_info("charger ic is good.\n");
			set_hw_dev_flag(DEV_I2C_CHARGER);
		} else {
			hwlog_err("charger ic is bad.\n");
		}
	} else {
		hwlog_err("ops dev_check is null.\n");
	}
#endif

	ret = charge_sysfs_create_group(di);
	if (ret)
		hwlog_err("can't create charge sysfs entries\n");
	power_class = hw_power_get_class();
	if (power_class) {
		if (charge_dev == NULL)
			charge_dev =
			    device_create(power_class, NULL, 0, NULL, "charger");
		ret =
		    sysfs_create_link(&charge_dev->kobj, &di->dev->kobj, "charge_data");
		if (ret) {
			hwlog_err("create link to charge_data fail.\n");
			goto charge_fail_3;
		}
	}

	ret = charge_extra_ops_register(&huawei_charge_extra_ops);
	if (ret) {
		hwlog_err("register extra charge ops failed!\n");
	}

	if (power_if_ops_register(&dcp_power_if_ops)) {
		hwlog_err("register power_if_ops_register failed!\n");
	}

	copy_bootloader_charger_info();

	hwlog_info("huawei charger probe ok!\n");
	return 0;

charge_fail_3:
	charge_sysfs_remove_group(di);
charge_fail_2:
	charger_event_queue_destroy(&di->event_queue);
fail_create_event_queue:
	wake_lock_destroy(&charge_lock);
	wake_lock_destroy(&otg_lock);
	wake_lock_destroy(&stop_charge_lock);
	wake_lock_destroy(&uscp_plugout_lock);
charge_fail_1:
	di->ops = NULL;
charge_fail_0:
	platform_set_drvdata(pdev, NULL);
	charge_device_info_free(di);
	di = NULL;
	return ret;
}

/**********************************************************
*  Function:       charge_remove
*  Description:    charge module remove
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int charge_remove(struct platform_device *pdev)
{
	struct charge_device_info *di = platform_get_drvdata(pdev);

	if (NULL == di) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return -ENODEV;
	}

	hisi_charger_type_notifier_unregister(&di->usb_nb);
	atomic_notifier_chain_unregister(&fault_notifier_list, &di->fault_nb);
	charge_sysfs_remove_group(di);
	wake_lock_destroy(&charge_lock);
	wake_lock_destroy(&otg_lock);
	wake_lock_destroy(&uscp_plugout_lock);
	wake_lock_destroy(&stop_charge_lock);
	cancel_delayed_work(&di->charge_work);
	cancel_delayed_work(&di->otg_work);
	cancel_delayed_work(&di->plugout_uscp_work);
#ifdef CONFIG_OTG_GPIO_ID
	if ((ycable != NULL) && ycable->ycable_support) {
		cancel_delayed_work(&ycable->ycable_work);
	}
#endif
	if (NULL != di->ops) {
		di->ops = NULL;
		g_ops = NULL;
	}
	if (NULL != di->fcp_ops) {
		di->fcp_ops = NULL;
		g_fcp_ops = NULL;
	}
	charge_device_info_free(di);
	di = NULL;
	return 0;
}

/**********************************************************
*  Function:       charge_shutdown
*  Description:    charge module shutdown
*  Parameters:   pdev:platform_device
*  return value:  NULL
**********************************************************/
static void charge_shutdown(struct platform_device *pdev)
{
	struct charge_device_info *di = platform_get_drvdata(pdev);
	int ret = 0;

	hwlog_info("%s ++\n", __func__);
	if (NULL == di) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return;
	}
	ret = di->ops->set_otg_enable(FALSE);
	if (ret) {
		hwlog_err("[%s]set otg default fail!\n", __func__);
	}

	atomic_notifier_chain_unregister(&fault_notifier_list, &di->fault_nb);

	cancel_delayed_work(&di->charge_work);
	cancel_delayed_work(&di->otg_work);
#ifdef CONFIG_OTG_GPIO_ID
	if ((ycable != NULL) && ycable->ycable_support) {
		cancel_delayed_work(&ycable->ycable_work);
	}
#endif
	hwlog_info("%s --\n", __func__);

	return;
}

#ifdef CONFIG_PM
/**********************************************************
*  Function:       charge_suspend
*  Description:    charge module suspend
*  Parameters:   pdev:platform_device
*                      state:unused
*  return value:  0-sucess or others-fail
**********************************************************/
static int charge_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct charge_device_info *di = platform_get_drvdata(pdev);
	hwlog_info("%s ++\n", __func__);

	if (di->charger_source == POWER_SUPPLY_TYPE_MAINS) {
		if (charge_is_charging_full(di)) {
			if (!wake_lock_active(&charge_lock)) {
				cancel_delayed_work(&di->charge_work);
			}
		}
	}

	if ((CHARGE_DONE_SLEEP_ENABLED ==
	     di->sysfs_data.charge_done_sleep_status)
	    || charge_done_sleep_dts) {
		charge_disable_watchdog(di);
		charge_set_input_current(di->input_current);
	}

	if (USB_EVENT_OTG_ID == di->charger_type) {
		if (!wake_lock_active(&otg_lock)) {
			cancel_delayed_work(&di->otg_work);
		}
		charge_disable_watchdog(di);
	}
	hwlog_info("%s --\n", __func__);

	return 0;
}

/**********************************************************
*  Function:       charge_resume
*  Description:    charge module resume
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int charge_resume(struct platform_device *pdev)
{
	struct charge_device_info *di = platform_get_drvdata(pdev);

	hwlog_info("%s ++\n", __func__);
	schedule_work(&resume_wakelock_work);

	if (di->charger_source == POWER_SUPPLY_TYPE_MAINS) {
#ifdef CONFIG_HISI_CHARGER_SYS_WDG
		charge_enable_sys_wdt();
#endif
		schedule_delayed_work(&di->charge_work, msecs_to_jiffies(0));
	}

	hwlog_info("%s --\n", __func__);

	return 0;
}
#endif /* CONFIG_PM */

static struct of_device_id charge_match_table[] = {
	{
	 .compatible = "huawei,charger",
	 .data = NULL,
	 },
	{
	 },
};

static struct platform_driver charge_driver = {
	.probe = charge_probe,
	.remove = charge_remove,
#ifdef CONFIG_PM
	.suspend = charge_suspend,
	.resume = charge_resume,
#endif
	.shutdown = charge_shutdown,
	.driver = {
		   .name = "huawei,charger",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(charge_match_table),
		   },
};
/*lint -restore*/

/**********************************************************
*  Function:       charge_init
*  Description:    charge module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static int __init charge_init(void)
{
	return platform_driver_register(&charge_driver);
}

/**********************************************************
*  Function:       charge_exit
*  Description:    charge module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit charge_exit(void)
{
	platform_driver_unregister(&charge_driver);
}

late_initcall(charge_init);
module_exit(charge_exit);
/*lint -restore*/

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("huawei charger module driver");
MODULE_AUTHOR("HUAWEI Inc");
