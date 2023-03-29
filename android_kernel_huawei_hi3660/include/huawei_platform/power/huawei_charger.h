/*
 * drivers/power/huawei_charger.h
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

#ifndef _HUAWEI_CHARGER
#define _HUAWEI_CHARGER

#include <linux/device.h>	/*for struct charge_device_info */
#include <linux/notifier.h>	/*for struct charge_device_info */
#include <linux/workqueue.h>	/*for struct charge_device_info */
#include <linux/power_supply.h>	/*for struct charge_device_info */
#include <huawei_platform/usb/hw_typec_dev.h>
#include <huawei_platform/usb/hw_typec_platform.h>
#include <huawei_platform/usb/hw_pd_dev.h>

#include <huawei_platform/power/power_dsm.h>
#include <huawei_platform/power/huawei_charger_power_interface.h>
#include <huawei_platform/power/power_debug.h>

#ifdef CONFIG_DIRECT_CHARGER
#include <huawei_platform/power/direct_charger.h>
#endif
#include <huawei_platform/power/huawei_charger_common.h>

/*************************marco define area***************************/
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif
#define get_index(x) (x-ERROR_FCP_VOL_OVER_HIGH)
#define ERR_NO_STRING_SIZE 256
#define CHARGELOG_SIZE      (4096)
#define CHARGE_DMDLOG_SIZE      (2048)
#define CHARGE_SYSFS_BUF_SIZE	10
#define CHIP_RESP_TIME	200

#define TERM_ERR_CNT 10

/* sensor_id#scene_id#stage */
#define THERMAL_REASON_SIZE    16

#define HIZ_MODE_ENABLE 1
#define HIZ_MODE_DISABLE 0

/*charge threshold*/
#define NO_CHG_TEMP_LOW     (0)
#define NO_CHG_TEMP_HIGH    (50)
#define BATT_EXIST_TEMP_LOW (-40)
#define DEFAULT_NORMAL_TEMP (25)


#define CHARGER_NOT_DBC_CONTROL 2 /*0:dbc close charger,1:dbc open charger,2:dbc not control charger*/

/*fcp detect */
#define FCP_ADAPTER_DETECT_FAIL 1
#define FCP_ADAPTER_DETECT_SUCC 0
#define FCP_ADAPTER_DETECT_OTHER -1

/*fcp adapter status */
#define FCP_ADAPTER_STATUS 0x28
#define FCP_ADAPTER_OVLT 0x04
#define FCP_ADAPTER_OCURRENT 0x02
#define FCP_ADAPTER_OTEMP 0x01

/*options of charge current(include input current & charge into battery current)*/
#define CHARGE_CURRENT_0000_MA            (0)
#define CHARGE_CURRENT_0100_MA            (100)
#define CHARGE_CURRENT_0150_MA            (150)
#define CHARGE_CURRENT_0500_MA            (500)
#define CHARGE_CURRENT_0800_MA            (800)
#define CHARGE_CURRENT_1000_MA            (1000)
#define CHARGE_CURRENT_1200_MA            (1200)
#define CHARGE_CURRENT_1900_MA            (1900)
#define CHARGE_CURRENT_2000_MA            (2000)
#define CHARGE_CURRENT_4000_MA            (4000)
#define CHARGE_CURRENT_MAX_MA            (32767)

#define CHARGE_CURRENT_DELAY 100

/*options of battery voltage*/
#define BATTERY_VOLTAGE_0000_MV             (0)
#define BATTERY_VOLTAGE_0200_MV             (200)

#define BATTERY_VOLTAGE_MIN_MV              (-32767)
#define BATTERY_VOLTAGE_MAX_MV              (32767)
#define BATTERY_VOLTAGE_3200_MV             (3200)
#define BATTERY_VOLTAGE_3400_MV             (3400)
#define BATTERY_VOLTAGE_4100_MV             (4200)
#define BATTERY_VOLTAGE_4200_MV             (4200)
#define BATTERY_VOLTAGE_4350_MV             (4350)
#define BATTERY_VOLTAGE_4500_MV             (4500)

/*options of NTC battery temperature*/
#define BATTERY_TEMPERATURE_MIN             (-32767)
#define BATTERY_TEMPERATURE_MAX             (32767)
#define BATTERY_TEMPERATURE_0_C             (0)
#define BATTERY_TEMPERATURE_5_C             (5)

#define CHARGING_WORK_TIMEOUT                (30000)
#define MIN_CHARGING_CURRENT_OFFSET          (-10)
#define BATTERY_FULL_CHECK_TIMIES            (2)
#define IIN_AVG_SAMPLES                      (10)
#define IMPOSSIBLE_IIN                       (999999)
#define CURRENT_FULL_CHECK_TIMES             (3)
#define DELA_ICHG_FOR_CURRENT_FULL           (30)
#define CURRENT_FULL_VOL_OFFSET              (50)
#define CURRENT_FULL_VALID_PERCENT           (80)
#define CAP_TH_FOR_CURRENT_FULL              (80)

#define WATCHDOG_TIMER_DISABLE     (0)
#define WATCHDOG_TIMER_40_S     (40)
#define WATCHDOG_TIMER_80_S     (80)

#define ADAPTER_0V                    (0)
#define ADAPTER_5V                    (5)
#define ADAPTER_9V                    (9)
#define ADAPTER_12V                   (12)

#define PD_ADAPTER_5V              (5000)
#define PD_ADAPTER_9V              (9000)
#define PD_ADAPTER_12V            (12000)

/*options of charge voltage (for dpm voltage setting,also ovp & uvp protect)*/
#define CHARGE_VOLTAGE_4360_MV    (4360)
#define CHARGE_VOLTAGE_4520_MV    (4520)
#define CHARGE_VOLTAGE_4600_MV    (4600)
#define CHARGE_VOLTAGE_5000_MV    (5000)
#define CHARGE_VOLTAGE_6300_MV    (6300)
#define CHARGE_VOLTAGE_6500_MV    (6500)

/*options of charge states from chip*/
#define CHAGRE_STATE_NORMAL              (0x00)
#define CHAGRE_STATE_VBUS_OVP           (0x01)
#define CHAGRE_STATE_NOT_PG               (0x02)
#define CHAGRE_STATE_WDT_FAULT         (0x04)
#define CHAGRE_STATE_BATT_OVP           (0x08)
#define CHAGRE_STATE_CHRG_DONE         (0x10)
#define CHAGRE_STATE_INPUT_DPM          (0x20)
#define CHAGRE_STATE_NTC_FAULT          (0x40)

/*options of vbus voltage*/
#define VBUS_VOL_READ_CNT              (3)
#define VBUS_VOLTAGE_13400_MV              (13400)
#define SLEEP_110MS                      (110)
#define COOL_LIMIT                       (11)
#define WARM_LIMIT                       (44)
#define WARM_CUR_RATIO       35
#define RATIO_BASE      100
#define IBIS_RATIO      120


#define VBUS_VOLTAGE_7000_MV              (7000)
#define VBUS_VOLTAGE_6500_MV              (6500)
#define VBUS_VOLTAGE_ABNORMAL_MAX_COUNT (2)

/*Type C mode current*/
#define TYPE_C_HIGH_MODE_CURR (3000)
#define TYPE_C_MID_MODE_CURR (1500)
#define TYPE_C_DEFAULT_MODE_CURR (500)

/*adaptor test macro*/
#define TMEP_BUF_LEN                (10)
#define POSTFIX_LEN                 (3)
#define INVALID_RET_VAL             (-1)
#define ADAPTOR_TEST_START          (1)
#define MIN_ADAPTOR_TEST_INS_NUM    (0)
#define MAX_ADAPTOR_TEST_INS_NUM    (5)
#define MAX_EVENT_COUNT 16
#define EVENT_QUEUE_UNIT MAX_EVENT_COUNT
#define WEAKSOURCE_CNT 3

#define CHARGER_SET_DISABLE_FLAGS           1
#define CHARGER_CLEAR_DISABLE_FLAGS         0

/* vbus valid check timeout on powerdown charging  */
#define VBUS_VALID_CHECK_WORK_TIMEOUT  (3000)
#define PD_VOLTAGE_CHANGE_WORK_TIMEOUT (2000)

#define OTG_ENABLE                     (1)
#define OTG_DISABLE                    (0)
/*************************struct define area***************************/
enum charger_charging_event {
	CHARGER_START_CHARGING_EVENT = 0,
	CHARGER_STOP_CHARGING_EVENT,
	CHARGER_CHARGING_DONE_EVENT,
};

enum charger_event_type {
	START_SINK = 0,
	STOP_SINK,
	START_SOURCE,
	STOP_SOURCE,
	START_SINK_WIRELESS,
	STOP_SINK_WIRELESS,
	CHARGER_MAX_EVENT,
};
enum reset_adapter_source_type {
	RESET_ADAPTER_SYSFS = 0,
	RESET_ADAPTER_WIRELESS_TX,
	RESET_ADAPTER_TOTAL,
};
enum otg_ctrl_type {
	OTG_CTRL_WIRED_OTG = 0,
	OTG_CTRL_WIRELESS_TX,
	OTG_CTRL_DC,
	OTG_CTRL_TOTAL,
};

enum usb_charger_type {
	CHARGER_TYPE_USB = 0,	/*SDP*/
	CHARGER_TYPE_BC_USB,	/*CDP*/
	CHARGER_TYPE_NON_STANDARD,	/*UNKNOW*/
	CHARGER_TYPE_STANDARD,	/*DCP*/
	CHARGER_TYPE_FCP,	/*FCP*/
	CHARGER_REMOVED,	/*not connected*/
	USB_EVENT_OTG_ID,
	CHARGER_TYPE_VR,        /*VR charger*/
	CHARGER_TYPE_TYPEC,        /*PD charger*/
	CHARGER_TYPE_PD,        /*PD charger*/
	CHARGER_TYPE_SCP, /*SCP charger*/
	CHARGER_TYPE_WIRELESS,  /*wireless charger*/
};

enum charge_fault_type {
	CHARGE_FAULT_NON = 0,
	CHARGE_FAULT_BOOST_OCP,
	CHARGE_FAULT_VBAT_OVP,
	CHARGE_FAULT_SCHARGER,
	CHARGE_FAULT_I2C_ERR,
	CHARGE_FAULT_WEAKSOURCE,
	CHARGE_FAULT_TOTAL,
};

enum charge_sysfs_type {
	CHARGE_SYSFS_IIN_THERMAL = 0,
	CHARGE_SYSFS_ICHG_THERMAL,
	CHARGE_SYSFS_IIN_THERMAL_AUX,
	CHARGE_SYSFS_ICHG_THERMAL_AUX,
	CHARGE_SYSFS_IIN_RUNNINGTEST,
	CHARGE_SYSFS_ICHG_RUNNINGTEST,
	CHARGE_SYSFS_ENABLE_CHARGER,
	CHARGE_SYSFS_LIMIT_CHARGING,
	CHARGE_SYSFS_REGULATION_VOLTAGE,
	CHARGE_SYSFS_BATFET_DISABLE,
	CHARGE_SYSFS_WATCHDOG_DISABLE,
	CHARGE_SYSFS_CHARGELOG,
	CHARGE_SYSFS_CHARGELOG_HEAD,
	CHARGE_SYSFS_IBUS,
	CHARGE_SYSFS_VBUS,
	CHARGE_SYSFS_HIZ,
	CHARGE_SYSFS_CHARGE_TYPE,
	CHARGE_SYSFS_CHARGE_DONE_STATUS,
	CHARGE_SYSFS_CHARGE_DONE_SLEEP_STATUS,
	CHARGE_SYSFS_INPUTCURRENT,
	CHARGE_SYSFS_VOLTAGE_SYS,
	CHARGE_SYSFS_BOOTLOADER_CHARGER_INFO,
	CHARGE_SYSFS_ICHG_REG,
	CHARGE_SYSFS_ICHG_ADC,
	CHARGE_SYSFS_ICHG_REG_AUX,
	CHARGE_SYSFS_ICHG_ADC_AUX,
	CHARGE_SYSFS_ADC_CONV_RATE,
	CHARGE_SYSFS_VR_CHARGER_TYPE,
	CHARGE_SYSFS_SUPPORT_ICO,
	CHARGE_SYSFS_WATER_INTRUSED,
	CHARGE_SYSFS_CHARGE_TERM_VOLT_DESIGN,
	CHARGE_SYSFS_CHARGE_TERM_CURR_DESIGN,
	CHARGE_SYSFS_CHARGE_TERM_VOLT_SETTING,
	CHARGE_SYSFS_CHARGE_TERM_CURR_SETTING,
	CHARGE_SYSFS_FCP_SUPPORT,
	CHARGE_SYSFS_DBC_CHARGE_CONTROL,
	CHARGE_SYSFS_DBC_CHARGE_DONE,
	CHARGE_SYSFS_ADAPTOR_TEST,
	CHARGE_SYSFS_ADAPTOR_VOLTAGE,
	CHARGE_SYSFS_PLUGUSB,
	CHARGE_SYSFS_THERMAL_REASON,
};
enum charge_done_type {
	CHARGE_DONE_NON = 0,
	CHARGE_DONE,
};
enum charge_done_sleep_type {
	CHARGE_DONE_SLEEP_DISABLED = 0,
	CHARGE_DONE_SLEEP_ENABLED,
};
enum fcp_check_stage_type {
	FCP_STAGE_DEFAUTL,
	FCP_STAGE_SUPPORT_DETECT,
	FCP_STAGE_ADAPTER_DETECT,
	FCP_STAGE_ADAPTER_ENABLE,
	FCP_STAGE_SUCESS,
	FCP_STAGE_CHARGE_DONE,
	FCP_STAGE_RESET_ADAPTOR,
	FCP_STAGE_ERROR,
};
enum fcp_retry_operate_type {
	FCP_RETRY_OPERATE_DEFAUTL,
	FCP_RETRY_OPERATE_RESET_ADAPTER,
	FCP_RETRY_OPERATE_RESET_SWITCH,
	FCP_RETRY_OPERATE_UNVALID,
};
enum fcp_test_result{
    FCP_TEST_SUCC,
    FCP_TEST_FAIL,
    FCP_NOT_SUPPORT,
};

enum disable_charger_type {
	CHARGER_SYS_NODE = 0,
	CHARGER_FATAL_ISC_TYPE,
	CHARGER_WIRELESS_TX,
	BATT_CERTIFICATION_TYPE,
	__MAX_DISABLE_CHAGER,
};

static const char *const fcp_check_stage[] = {
	[0] = "FCP_STAGE_DEFAUTL",
	[1] = "FCP_STAGE_SUPPORT_DETECT",
	[2] = "FCP_STAGE_ADAPTER_DETECT",
	[3] = "FCP_STAGE_ADAPTER_ENABLE",
	[4] = "FCP_STAGE_SUCESS",
};
struct charger_event_queue {
	enum charger_event_type *event;
	unsigned int num_event;
	unsigned int max_event;
	unsigned int enpos, depos;
	unsigned int overlay, overlay_index;
};
struct ico_input {
	enum usb_charger_type charger_type;
	unsigned int iin_max;
	unsigned int ichg_max;
	unsigned int vterm;
};
struct ico_output {
	unsigned int input_current;
	unsigned int charge_current;
};
struct chip_init_crit {
	int vbus;
};
struct charge_sysfs_data {
	unsigned int adc_conv_rate;
	unsigned int iin_thl;
	unsigned int ichg_thl;
	unsigned int iin_thl_main;
	unsigned int ichg_thl_main;
	unsigned int iin_thl_aux;
	unsigned int ichg_thl_aux;
	unsigned int iin_rt;
	unsigned int ichg_rt;
	unsigned int vterm_rt;
	unsigned int charge_limit;
	unsigned int wdt_disable;
	unsigned int charge_enable;
	unsigned int disable_charger[__MAX_DISABLE_CHAGER];
	unsigned int batfet_disable;
	unsigned int vr_charger_type;
	unsigned int dbc_charge_control;
	enum charge_done_type charge_done_status;
	enum charge_done_sleep_type charge_done_sleep_status;
	int ibus;
	int vbus;
	int inputcurrent;
	int voltage_sys;
	struct mutex dump_reg_lock;
	struct mutex dump_reg_head_lock;
	struct mutex bootloader_info_lock;
	char *reg_value;
	char *reg_head;
	char *bootloader_info;
	unsigned int support_ico;
	unsigned int water_intrused;
	unsigned int fcp_support;
};

struct charge_core_data {
	unsigned int iin;
	unsigned int ichg;
	unsigned int vterm;
	unsigned int iin_ac;
	unsigned int ichg_ac;
	unsigned int iin_usb;
	unsigned int ichg_usb;
	unsigned int iin_nonstd;
	unsigned int ichg_nonstd;
	unsigned int iin_bc_usb;
	unsigned int ichg_bc_usb;
	unsigned int iin_vr;
	unsigned int ichg_vr;
	unsigned int iin_pd;
	unsigned int ichg_pd;
	unsigned int iin_fcp;
	unsigned int ichg_fcp;
	unsigned int iin_weaksource;
	unsigned int iterm;
	unsigned int vdpm;
	unsigned int vdpm_control_type;
	unsigned int vdpm_buf_limit;
	unsigned int iin_max;
	unsigned int ichg_max;
	unsigned int otg_curr;
	unsigned int iin_typech;
	unsigned int ichg_typech;
	unsigned int typec_support;
	unsigned int segment_type;
	unsigned int segment_level;
	unsigned int temp_level;
	unsigned int high_temp_limit;
	bool warm_triggered;
#ifdef CONFIG_WIRELESS_CHARGER
	unsigned int iin_wireless;
	unsigned int ichg_wireless;
#endif
	unsigned int vterm_basp;
};

struct charge_device_ops {
	int (*chip_init)(struct chip_init_crit* init_crit);
	int (*set_adc_conv_rate)(int rate_mode);
	int (*set_input_current)(int value);
	void (*set_input_current_thermal)(int val1, int val2);
	int (*set_charge_current)(int value);
	void (*set_charge_current_thermal)(int val1, int val2);
	int (*dev_check)(void);
	int (*get_ichg_reg)(int flag);
	int (*get_ichg_adc)(int flag);
	int (*set_terminal_voltage)(int value);
	int (*set_dpm_voltage)(int value);
	int (*set_terminal_current)(int value);
	int (*set_charge_enable)(int enable);
	int (*get_charge_enable_status)(void);
	int (*set_otg_enable)(int enable);
	int (*set_term_enable)(int enable);
	int (*get_charge_state)(unsigned int *state);
	int (*reset_watchdog_timer)(void);
	int (*dump_register)(char *reg_value);
	int (*get_register_head)(char *reg_head);
	int (*set_watchdog_timer)(int value);
	int (*set_batfet_disable)(int disable);
	int (*get_ibus)(void);
	int (*get_vbus)(unsigned int *value);
	int (*check_charger_plugged)(void);
	int (*check_input_dpm_state)(void);
	int (*check_input_vdpm_state)(void);
	int (*check_input_idpm_state)(void);
	int (*set_covn_start)(int enable);
	int (*set_charger_hiz)(int enable);
	int (*set_otg_current)(int value);
	int (*stop_charge_config)(void);
        int (*set_otg_switch_mode_enable)(int enable);
	int (*get_vbat_sys)(void);
	int (*set_vbus_vset)(u32);
	int (*set_uvp_ovp)(void);
	int (*turn_on_ico)(struct ico_input *, struct ico_output *);
	int (*set_force_term_enable)(int enable);
	int (*get_charger_state)(void);
	int (*soft_vbatt_ovp_protect)(void);
	int (*rboost_buck_limit)(void);
	int (*get_charge_current)(void);
	int (*get_iin_set)(void);
	int (*set_boost_voltage)(int voltage);
	int (*get_dieid)(char *dieid, unsigned int len);
};
struct water_detect_ops {
	int (*is_water_intrused)(void);
};

struct charge_switch_ops {
	enum hisi_charger_type (*get_charger_type)(void);
	int (*is_water_intrused) (void);
};

struct fcp_adapter_device_ops {
	int (*get_adapter_output_current)(void);
	int (*set_adapter_output_vol)(int *);
	int (*detect_adapter)(void);
	int (*is_support_fcp)(void);
	int (*switch_chip_reset)(void);
	int (*fcp_adapter_reset)(void);
	int (*stop_charge_config)(void);
	int (*is_fcp_charger_type)(void);
	int (*fcp_read_adapter_status)(void);
	int (*fcp_read_switch_status)(void);
	void (*reg_dump)(char*);
};
#ifdef CONFIG_TCPC_CLASS
struct tcpc_device;
#endif
struct charge_device_info {
	struct device *dev;
	struct notifier_block usb_nb;
	struct notifier_block fault_nb;
	struct notifier_block typec_nb;
	struct delayed_work charge_work;
	struct delayed_work otg_work;
	struct delayed_work plugout_uscp_work;
	struct delayed_work pd_voltage_change_work;
	struct work_struct usb_work;
	struct work_struct fault_work;
	struct charge_device_ops *ops;
	struct charge_switch_ops *sw_ops;
	struct fcp_adapter_device_ops *fcp_ops;
	struct charge_core_data *core_data;
	struct charge_sysfs_data sysfs_data;
	struct hrtimer timer;
	struct mutex mutex_hiz;
#ifdef CONFIG_TCPC_CLASS
	struct notifier_block tcpc_nb;
	struct tcpc_device *tcpc;
	unsigned int tcpc_otg;
	struct mutex tcpc_otg_lock;
	struct pd_dpm_vbus_state *vbus_state;
#endif
	unsigned int pd_input_current;
	unsigned int pd_charge_current;
	enum usb_charger_type charger_type;
	enum typec_input_current typec_current_mode;
	enum power_supply_type charger_source;
	enum charge_fault_type charge_fault;
	unsigned int charge_enable;
	unsigned int input_current;
	unsigned int charge_current;
	unsigned int input_typec_current;
	unsigned int charge_typec_current;
	unsigned int enable_current_full;
	unsigned int check_current_full_count;
	unsigned int check_full_count;
	unsigned int is_dual_charger;
	unsigned int start_attemp_ico;
	unsigned int support_standard_ico;
	unsigned int ico_current_mode;
	unsigned int ico_all_the_way;
	unsigned int water_check_enabled;
	unsigned int fcp_vindpm;
	unsigned int hiz_ref;
	unsigned int check_ibias_sleep_time;
#ifdef CONFIG_DIRECT_CHARGER
	int ignore_pluggin_and_plugout_flag;
	int support_scp_power;
#endif
	int reset_adapter;
#ifdef CONFIG_WIRELESS_CHARGER
	struct notifier_block wireless_nb;
	int wireless_vbus;
	int otg_channel;
	int gpio_otg_switch;
	unsigned int iin_limit;
#endif
	int weaksource_cnt;
	struct mutex event_type_lock;
	unsigned int charge_done_maintain_fcp;
	unsigned int term_exceed_time;
	struct work_struct event_work;
	spinlock_t event_spin_lock;
	enum charger_event_type event;
	struct charger_event_queue event_queue;
	struct delayed_work vbus_valid_check_work;
	int clear_water_intrused_flag_after_read;
	char thermal_reason[THERMAL_REASON_SIZE];
	int avg_iin_ma;
	int max_iin_ma;
	int current_full_status;
};
enum charge_wakelock_flag {
	CHARGE_NEED_WAKELOCK,
	CHARGE_NO_NEED_WAKELOCK,
};

/*adaptor protocol test*/
enum adaptor_name {
	TYPE_SCP,
	TYPE_FCP,
	TYPE_PD,
	TYPE_SC,
	TYPE_OTHER,
};
enum test_state {
	DETECT_FAIL = 0,
	DETECT_SUCC = 1,
	PROTOCOL_FINISH_SUCC = 2,
};
struct adaptor_test_attr{
	enum adaptor_name charger_type;
	char adaptor_str_name[10];
	enum test_state result;
};

#define WDT_TIME_80_S 80
#define WDT_STOP 0
#define CHARGE_IC_GOOD 0
#define CHARGE_IC_BAD    1
#define MAIN_CHARGER 1
#define AUX_CHARGER   0
#define WEAKSOURCE_TRUE  (1)
#define WEAKSOURCE_FALSE (0)
#define FCP_DETECT_DELAY_IN_POWEROFF_CHARGE 2000
/****************variable and function declarationn area******************/
extern struct blocking_notifier_head charge_wake_unlock_list;
extern struct atomic_notifier_head fault_notifier_list;
extern struct device *charge_dev;
extern unsigned int get_pd_charge_flag(void);
extern bool pd_dpm_set_voltage(int vol);
extern void pd_dpm_set_pd_reset_adapter(int ra);

int water_detect_ops_register(struct water_detect_ops *ops);
void water_detect(void);
int charge_ops_register(struct charge_device_ops *ops);
int charge_switch_ops_register(struct charge_switch_ops *ops);
int fcp_adapter_ops_register(struct fcp_adapter_device_ops *ops);
void charge_type_dcp_detected_notify(void);
int fcp_test_is_support(void);
int fcp_test_detect_adapter(void);
enum fcp_check_stage_type fcp_get_stage_status(void);
int get_fcp_charging_flag(void);
int charge_set_input_current(int iset);
int charge_get_charger_iinlim_regval(void);
void chg_set_adaptor_test_result(enum adaptor_name charger_type, enum test_state result);
enum charge_done_type get_charge_done_type(void);
#ifdef CONFIG_TCPC_CLASS
int is_pd_supported(void);
#endif
#ifdef CONFIG_DIRECT_CHARGER
void direct_charger_connect_send_uevent(void);
void direct_charger_disconnect_update_charger_type(void);
void direct_charger_disconnect_send_uevent(void);
void ignore_pluggin_and_pluggout_interrupt(void);
void restore_pluggin_pluggout_interrupt(void);
int get_direct_charge_flag(void);
#endif
#ifdef CONFIG_WIRELESS_CHARGER
void charge_set_input_current_prop(int iin_step, int iin_delay);
void wireless_channel_off_update_charger_type(void);
void wireless_charge_connect_send_uevent(void);
#endif
int disable_chargers(int charger_type, int disable, int role);
int set_charger_disable_flags(int, int);
extern struct blocking_notifier_head charger_event_notify_head;

int charge_get_vbus(void);
void charger_source_sink_event(enum charger_event_type event);
void send_water_intrused_event(bool flag);
bool get_stop_charge_sync_flag(void);
int charge_otg_mode_enable(int enable, enum otg_ctrl_type type);
void charge_set_adapter_voltage(int val, enum reset_adapter_source_type type, unsigned int delay_time);
int charge_set_input_current_max(void);
bool charge_get_hiz_state(void);
int charge_set_hiz_enable(int enable);
void charge_set_batfet_disable(int val);

#endif
