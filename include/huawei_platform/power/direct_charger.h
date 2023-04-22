#ifndef __DIRECT_CHARGER_H_
#define __DIRECT_CHARGER_H_

#include <linux/module.h>
#include <linux/device.h>
#include <linux/notifier.h>
#include <huawei_platform/power/huawei_charger.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

/*move from direct_charger.c*/
#include <huawei_platform/log/hw_log.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/wakelock.h>
#ifdef CONFIG_HISI_COUL
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#endif
#ifdef CONFIG_HISI_BCI_BATTERY
#include <linux/power/hisi/hisi_bci_battery.h>
#endif
#include <huawei_platform/power/huawei_charger.h>
#ifdef CONFIG_TCPC_CLASS
#include <huawei_platform/usb/hw_pd_dev.h>
#endif
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#ifdef CONFIG_SUPERSWITCH_FSC
#include <huawei_platform/usb/superswitch/fsc/core/hw_scp.h>
#endif
#ifdef CONFIG_WIRELESS_CHARGER
#include <huawei_platform/power/wireless_charger.h>
#endif
#ifdef CONFIG_DP_AUX_SWITCH
#include "huawei_platform/dp_aux_switch/dp_aux_switch.h"
#endif
#define CHARGE_DMDLOG_SIZE      (2048)
#define REVERSE_OCP_CNT          3
#define OTP_CNT                  3
#define ADAPTOR_OTP_CNT          3

#ifndef BIT
#define BIT(x) (1 << x)
#endif
/*scp adaptor register*/
#define ADAPTOR_SCP_DETECT_FAIL -1

#define SCP_ADP_TYPE 0x80
#define SCP_ADP_TYPE_B_MASK (BIT(5)|BIT(4))
#define SCP_ADP_TYPE_B 0x10

#define SCP_ADP_TYPE0 0x7E
#define SCP_ADP_TYPE0_B_MASK (BIT(4))
#define SCP_ADP_TYPE0_B_SC_MASK (BIT(3))
#define SCP_ADP_TYPE0_B_LVC_MASK (BIT(2))

#define SCP_B_ADP_TYPE 0x81
#define SCP_B_DIRECT_ADP 0x10
#define DOUBLE_SIZE 2

/* sensor_id#scene_id#stage */
#define SCP_THERMAL_REASON_SIZE    16

#define SCP_VENDOR_ID_H 0x82
#define SCP_VENDOR_ID_L 0x83
#define SCP_MODULE_ID_H 0x84
#define SCP_MODULE_ID_L 0x85
#define SCP_SERRIAL_NO_H 0x86
#define SCP_SERRIAL_NO_L 0x87
#define SCP_PCHIP_ID 0x88
#define SCP_HWVER 0x89
#define SCP_FWVER_H 0x8a
#define SCP_FWVER_L 0x8b
#define SCP_SPID 0x8c
#define SCP_NEED_OPEN_OUTPUT2 0x31
#define SCP_MAX_POWER 0x90
#define SCP_CNT_POWER 0x91
#define SCP_MIN_VOUT 0x92
#define SCP_MAX_VOUT 0x93
#define SCP_MIN_IOUT 0x94
#define SCP_MAX_IOUT 0x95
#define SCP_MAX_IOUT_A_MASK (BIT(7) | BIT(6))
#define SCP_MAX_IOUT_A_SHIFT 6
#define SCP_MAX_IOUT_B_MASK (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SCP_VSTEP 0x96
#define SCP_ISTEP 0x97
#define SCP_MAX_VERR 0x98
#define SCP_MAX_IVRR 0x99
#define SCP_MAX_STTIME 0x9a
#define SCP_MAX_RSPTIME 0x9b
#define SCP_SCTRL 0x9e
#define SCP_OUTPUT2_ENABLE_MASK BIT(7)
#define SCP_OUTPUT2_ENABLE BIT(7)
#define SCP_CTRL_BYTE0 0xa0
#define SCP_OUTPUT_MODE_MASK BIT(6)
#define SCP_OUTPUT_MODE_ENABLE BIT(6)
#define SCP_OUTPUT_MODE_DISABLE 0
#define SCP_OUTPUT_MASK BIT(7)
#define SCP_OUTPUT_ENABLE BIT(7)
#define SCP_OUTPUT_DISABLE 0
#define SCP_ADAPTOR_RESET_MASK BIT(5)
#define SCP_ADAPTOR_RESET_ENABLE BIT(5)
#define SCP_ADAPTOR_RESET_DISABLE 0
#define SCP_CTRL_BYTE1 0xa1
#define SCP_DP_DELITCH_MASK (BIT(3) | BIT(4))
#define SCP_DP_DELITCH_5_MS (BIT(3) | BIT(4))
#define SCP_WATCHDOG_BITS_PER_SECOND (2)
#define SCP_WATCHDOG_MASK (BIT(2) | BIT(1) | BIT(0))
#define SCP_STATUS_BYTE0 0xa2
#define SCP_CABLE_STS_MASK BIT(3)
#define SCP_PORT_LEAKAGE_INFO BIT(4)
#define SCP_PORT_LEAKAGE_SHIFT 4
#define SCP_STATUS_BYTE1 0xa3
#define SCP_SSTS 0xa5
#define SCP_SSTS_A_MASK (BIT(3) | BIT(2) | BIT(1))
#define SCP_SSTS_B_MASK (BIT(0))
#define SCP_SSTS_B_SHIFT 0
#define SCP_INSIDE_TMP 0xa6
#define SCP_PORT_TMP 0xa7
#define SCP_READ_VOLT_L 0xa8
#define SCP_READ_VOLT_H 0xa9
#define SCP_READ_IOLT_L 0xaa
#define SCP_READ_IOLT_H 0xab
#define SCP_SREAD_VOUT 0xc8
#define SCP_SREAD_VOUT_OFFSET 3000
#define SCP_SREAD_VOUT_STEP 10
#define SCP_SREAD_IOUT 0xc9
#define SCP_SREAD_IOUT_STEP 50
#define SCP_DAC_VSET_L 0xac
#define SCP_DAC_VSET_H 0xad
#define SCP_DAC_ISET_L 0xae
#define SCP_DAC_ISET_H 0xaf
#define SCP_VSET_BOUNDARY_L 0xb0
#define SCP_VSET_BOUNDARY_H 0xb1
#define SCP_ISET_BOUNDARY_L 0xb2
#define SCP_ISET_BOUNDARY_H 0xb3
#define SCP_MAX_VSET_OFFSET 0xb4
#define SCP_MAX_ISET_OFFSET 0xb5
#define SCP_VSET_L 0xb8
#define SCP_VSET_H 0xb9
#define SCP_ISET_L 0xba
#define SCP_ISET_H 0xbb
#define SCP_VSSET 0xca
#define VSSET_OFFSET 3000
#define VSSET_STEP 10
#define SCP_ISSET 0xcb
#define ISSET_STEP 50
#define SCP_VSET_OFFSET_L 0xbc
#define SCP_VSET_OFFSET_H 0xbd
#define SCP_ISET_OFFSET_L 0xbe
#define SCP_ISET_OFFSET_H 0xbf
#define SCP_STEP_VSET_OFFSET 0xcc
#define SCP_STEP_ISET_OFFSET 0xcd

#define SCP_ADAPTOR_KEY_INDEX_REG 0xce
#define SCP_ADAPTOR_KEY_INDEX_BASE 0x00
#define SCP_ADAPTOR_KEY_INDEX_PUBLIC (SCP_ADAPTOR_KEY_INDEX_BASE+0x01)
#define SCP_ADAPTOR_KEY_INDEX_1 (SCP_ADAPTOR_KEY_INDEX_BASE+0x02)
#define SCP_ADAPTOR_KEY_INDEX_2 (SCP_ADAPTOR_KEY_INDEX_BASE+0x03)
#define SCP_ADAPTOR_KEY_INDEX_3 (SCP_ADAPTOR_KEY_INDEX_BASE+0x04)
#define SCP_ADAPTOR_KEY_INDEX_4 (SCP_ADAPTOR_KEY_INDEX_BASE+0x05)
#define SCP_ADAPTOR_KEY_INDEX_5 (SCP_ADAPTOR_KEY_INDEX_BASE+0x06)
#define SCP_ADAPTOR_KEY_INDEX_6 (SCP_ADAPTOR_KEY_INDEX_BASE+0x07)
#define SCP_ADAPTOR_KEY_INDEX_7 (SCP_ADAPTOR_KEY_INDEX_BASE+0x08)
#define SCP_ADAPTOR_KEY_INDEX_8 (SCP_ADAPTOR_KEY_INDEX_BASE+0x09)
#define SCP_ADAPTOR_KEY_INDEX_9 (SCP_ADAPTOR_KEY_INDEX_BASE+0x0A)
#define SCP_ADAPTOR_KEY_INDEX_10 (SCP_ADAPTOR_KEY_INDEX_BASE+0x0B)
#define SCP_ADAPTOR_KEY_INDEX_RELEASE (SCP_ADAPTOR_KEY_INDEX_BASE+0xFF)

#define SCP_ADAPTOR_ENCRYPT_INFO_REG 0xcf
#define SCP_ADAPTOR_ENCRYPT_ENABLE BIT(7)
#define SCP_ADAPTOR_ENCRYPT_COMPLETE BIT(7)

#define SCP_ADAPTOR_RANDOM_NUM_HI_BASE_REG  0xa0
#define SCP_ADAPTOR_RANDOM_NUM_HI_LEN  8

#define SCP_ADAPTOR_RANDOM_NUM_LO_BASE_REG  0xa8
#define SCP_ADAPTOR_RANDOM_NUM_LO_LEN  8

#define SCP_ADAPTOR_DIGEST_BASE_REG  0xb0
#define SCP_ADAPTOR_DIGEST_LEN  16
#define SCP_ADAPTOR_DETECT_FAIL 1
#define SCP_ADAPTOR_DETECT_SUCC 0
#define SCP_ADAPTOR_DETECT_OTHER -1
#define SCP_CABLE_DETECT_FAIL 1
#define SCP_CABLE_DETECT_SUCC 0
#define SCP_CABLE_DETECT_ERROR -1

#define ERROR_RESISTANCE -99999
#define MAX_RESISTANCE 10000
#define SCP_CHARGE_TYPE_LVC 3
#define SCP_CHARGE_TYPE_SC 2
#define SCP_CHARGE_TYPE_FCP 4

#define DC_ERR_CNT_MAX            (8)
#define DC_OPEN_RETRY_CNT_MAX      (3)
#define DC_RESIST_LEVEL             (5)
#define DC_VOLT_LEVEL             (5)
#define DC_TEMP_LEVEL             (5)
#define DC_VOLT_GROUP_MAX             (8)
#define DC_BAT_BRAND_LEN_MAX             (16)
#define DC_VOLT_NODE_LEN_MAX             (16)

#define STRTOL_MAX_LEN     10

#define FAIL -1
#define SUCC 0

#define INVALID -1
#define VALID 0
#define WAIT_LS_DISCHARGE 200
#define MAX_TIMES_FOR_SET_ADAPTER_VOL_20 20
#define ADAPTER_VOL_DIFFRENCE_300_MV 300
#define MAX_ADAPTER_VOL_4400_MV 4400
#define MIN_ADAPTER_VOL_STEP_20_MV 20
#define MIN_CURRENT_FOR_RES_DETECT_800_MA 800
#define MAX_VOL_FOR_BATTERY_4360_MV 4360
#define CURRENT_SET_FOR_RES_DETECT_1000_MA 1000

#define VENDOR_ID_RICHTEK 0x01
#define VENDOR_ID_WELTREND 0x02
#define VENDOR_ID_IWATT 0x03
#define VENDOR_ID_0X32 0x32

#define MAX_IOUT_EXP_0 0
#define MAX_IOUT_EXP_1 1
#define MAX_IOUT_EXP_2 2
#define MAX_IOUT_EXP_3 3
#define TEN_EXP_0 1
#define TEN_EXP_1 10
#define TEN_EXP_2 100
#define TEN_EXP_3 1000
#define DROP_POWER_FLAG 1
#define DROP_POWER_FACTOR 8
#define SCP_NO_ERR 0
#define SCP_IS_ERR 1
#define ONE_BYTE_LEN 8
#define ONE_BYTE_MASK 0xff
#define ONE_BIT_EQUAL_TWO_SECONDS 2
#define WAIT_FOR_ADAPTOR_RESET 50
#define OUTPUT_MODE_ENABLE 1
#define OUTPUT_MODE_DISABLE 0
#define ADAPTOR_RESET 1
#define GPIO_ENABLE 1
#define GPIO_DISABLE 0
#define IN_SCP_CHARGING_STAGE 0
#define NOT_IN_SCP_CHARGING_STAGE -1
#define ENABLE 1
#define DISABLE 0


#define SCP_ADAPTOR_DETECT 1
#define SCP_ADAPTOR_NOT_DETECT 0

#define VBUS_ON_THRESHOLD 3000
#define VBAT_VBUS_DIFFERENCE 150
#define KICK_WATCHDOG_TIME 1000
#define WATCHDOG_TIMEOUT 2000
#define BATTERY_CAPACITY_HIGH_TH 95

#define DIRECT_CHARGER_SET_DISABLE_FLAGS        1
#define DIRECT_CHARGER_CLEAR_DISABLE_FLAGS      0

#define DIRECT_CHARGER_LS_RECOVERY_DELAY      500 /*ms*/
#define DIRECT_CHARGER_COUL_CURRENT_UNIT_DEVIDE	1000 /*1000 ua equal 1ma*/

#define SCCAL_NV_CMDLINE_MAX_LEN                30
#define C_OFFSET_MAX_LEN                        20
#define C_OFFSET_A_MAX                          1200000
#define C_OFFSET_A_MIN                          800000

enum direct_charge_mode{
	UNDEFINED_MODE = 0x0,
	LVC_MODE = 0x1,
	SC_MODE = 0x2,
};

enum adapter_vendor{
	RICHTEK_ADAPTER,
	IWATT_ADAPTER,
	WELTREND_ADAPTER,
	ID0X32_ADAPTER,
};
enum direct_charge_error_code {
	DIRECT_CHARGE_SUCC,
	DIRECT_CHARGE_ERROR_CHARGE_DISABLED,
	DIRECT_CHARGE_ERROR_ADAPTOR_DETECT,
	DIRECT_CHARGE_ERROR_BAT_TEMP,
	DIRECT_CHARGE_ERROR_BAT_VOL,
	DIRECT_CHARGE_ERROR_SWITCH,
	DIRECT_CHARGE_ERROR_INIT,
	DIRECT_CHARGE_ERROR_USB_PORT_LEAKAGE_CURRENT,
	DIRECT_CHARGE_ERROR_ADAPTOR_VOLTAGE_ACCURACY,
	DIRECT_CHARGE_ERROR_OPEN_CHARGE_PATH,
	DIRECT_CHARGE_ERROR_FULL_REISISTANCE,
	DIRECT_CHARGE_ERROR_ANTI_FAKE_ADAPTOR,
};
enum scp_retry_operate_type {
	SCP_RETRY_OPERATE_DEFAUTL,
	SCP_RETRY_OPERATE_RESET_ADAPTER,
	SCP_RETRY_OPERATE_RESET_CHIP,
	SCP_RETRY_OPERATE_UNVALID,
};

enum scp_stage_type {
	SCP_STAGE_DEFAULT,
	SCP_STAGE_SUPPORT_DETECT,
	SCP_STAGE_ADAPTER_DETECT,
	SCP_STAGE_SWITCH_DETECT,
	SCP_STAGE_CHARGE_INIT,
	SCP_STAGE_SECURITY_CHECK,
	SCP_STAGE_SUCCESS,
	SCP_STAGE_CHARGING,
	SCP_STAGE_CHARGE_DONE,
};

enum dc_resist_info {
	DC_RESIST_MIN =0,
	DC_RESIST_MAX,
	DC_RESIST_CUR_MAX,
	DC_RESIST_TOTAL,
};

enum dc_temp_info {
	DC_TEMP_MIN =0,
	DC_TEMP_MAX,
	DC_CUR_MAX,
	DC_TEMP_TOTAL,
};

enum dc_volt_info {
	DC_PARA_VOL_TH = 0,
	DC_PARA_CUR_TH_HIGH,
	DC_PARA_CUR_TH_LOW,
	DC_PARA_VOLT_TOTAL,
};
enum dc_bat_info {
	DC_PARA_BAT_ID = 0,
	DC_PARA_TEMP_LOW,
	DC_PARA_TEMP_HIGH,
	DC_PARA_INDEX,
	DC_PARA_BAT_TOTAL,
};

enum direct_charge_sysfs_type {
	DIRECT_CHARGE_SYSFS_ENABLE_CHARGER = 0,
	DIRECT_CHARGE_SYSFS_IIN_THERMAL,
	DIRECT_CHARGE_SYSFS_ADAPTOR_DETECT,
	DIRECT_CHARGE_SYSFS_LOADSWITCH_ID,
	DIRECT_CHARGE_SYSFS_LOADSWITCH_NAME,
	DIRECT_CHARGE_SYSFS_VBAT,
	DIRECT_CHARGE_SYSFS_IBAT,
	DIRECT_CHARGE_SYSFS_VADAPT,
	DIRECT_CHARGE_SYSFS_IADAPT,
	DIRECT_CHARGE_SYSFS_LS_VBUS,
	DIRECT_CHARGE_SYSFS_LS_IBUS,
	DIRECT_CHARGE_SYSFS_FULL_PATH_RESISTANCE,
	DIRECT_CHARGE_SYSFS_DIRECT_CHARGE_SUCC,
	DIRECT_CHARGE_SYSFS_SET_RESISTANCE_THRESHOLD,
	DIRECT_CHARGE_SYSFS_SET_CHARGETYPE_PRIORITY,
	DIRECT_CHARGE_SYSFS_THERMAL_REASON,
};

enum loadswitch_id {
	LOADSWITCH_RICHTEK,
	LOADSWITCH_TI,
	LOADSWITCH_FAIRCHILD,
	LOADSWITCH_NXP,
	LOADSWITCH_SCHARGERV600,
	LOADSWITCH_FPF2283,
	LOADSWITCH_TOTAL,
};

enum switchcap_id {
	SWITCHCAP_TI_BQ25970,
	SWITCHCAP_SCHARGERV600,
	SWITCHCAP_LTC7820,
	SWITCHCAP_TOTAL,
};

enum direct_charge_fault_type {
	DIRECT_CHARGE_FAULT_NON = 0,
	DIRECT_CHARGE_FAULT_VBUS_OVP,
	DIRECT_CHARGE_FAULT_REVERSE_OCP,
	DIRECT_CHARGE_FAULT_OTP,
	DIRECT_CHARGE_FAULT_TSBUS_OTP,
	DIRECT_CHARGE_FAULT_TSBAT_OTP,
	DIRECT_CHARGE_FAULT_TDIE_OTP,
	DIRECT_CHARGE_FAULT_INPUT_OCP,
	DIRECT_CHARGE_FAULT_VDROP_OVP,

	DIRECT_CHARGE_FAULT_AC_OVP,
	DIRECT_CHARGE_FAULT_VBAT_OVP,
	DIRECT_CHARGE_FAULT_IBAT_OCP,
	DIRECT_CHARGE_FAULT_IBUS_OCP,
	DIRECT_CHARGE_FAULT_CONV_OCP,

	DIRECT_CHARGE_FAULT_LTC7820,

	DIRECT_CHARGE_FAULT_TOTAL,
};

enum disable_direct_charger_type {
	DIRECT_CHARGER_SYS_NODE = 0,
	DIRECT_CHARGER_FATAL_ISC_TYPE,
	DIRECT_CHARGER_WIRELESS_TX,
	DIRECT_CHARGER_BATT_CERTIFICATION_TYPE,
	__MAX_DISABLE_DIRECT_CHAGER,
};

static const char *const loadswitch_name[] = {
	[0] = "RT9748",
	[1] = "BQ25870",
	[2] = "FAN54161",
	[3] = "PCA9498",
	[4] = "HI6526",
	[5] = "FPF2283",
	[6] = "ERROR",
};

static const char *const switchcap_name[] = {
	[0] = "BQ25970",
	[1] = "HI6526",
	[2] = "LTC7820",
	[3] = "ERROR",
};

static const char *const scp_check_stage[] = {
	[0] = "SCP_STAGE_DEFAULT",
	[1] = "SCP_STAGE_SUPPORT_DETECT",
	[2] = "SCP_STAGE_ADAPTER_DETECT",
	[3] = "SCP_STAGE_SWITCH_DETECT",
	[4] = "SCP_STAGE_CHARGE_INIT",
	[5] = "SCP_STAGE_SECURITY_CHECK",
	[6] = "SCP_STAGE_SUCCESS",
	[7] = "SCP_STAGE_CHARGING",
	[8] = "SCP_STAGE_CHARGE_DONE",
};

struct scp_init_data {
	int scp_mode_enable;
	int vset_boundary;
	int iset_boundary;
	int init_adaptor_voltage;
	int watchdog_timer;
};

struct nty_data {
	unsigned short addr;
	u8 event1;
	u8 event2;
	u8 event3;
};

struct loadswitch_ops {
	int (*ls_init)(void);
	int (*ls_exit)(void);
	int (*ls_enable)(int);
	int (*ls_discharge)(int);
	int (*is_ls_close)(void);
	int (*get_ls_id)(void);
	int (*watchdog_config_ms)(int time);
	int (*kick_watchdog)(void);
	int (*ls_status)(void);
};
struct batinfo_ops {
	int (*init)(void);
	int (*exit)(void);
	int (*get_bat_btb_voltage)(void);
	int (*get_bat_package_voltage)(void);
	int (*get_vbus_voltage)(int* vol);
	int (*get_bat_current)(int* cur);
	int (*get_ls_ibus)(int *ibus);
	int (*get_ls_temp)(int *temp);
};

/*currently we support 5 cc stage*/
struct dc_volt_para_info {
	int vol_th;
	int cur_th_high;
	int cur_th_low;
};
struct dc_bat_para_info {
	int temp_low;
	int temp_high;
	int parse_ok;
	char batid[DC_BAT_BRAND_LEN_MAX];
	char volt_para_index[DC_VOLT_NODE_LEN_MAX];
};
struct dc_volt_para_info_group {
	struct dc_volt_para_info volt_info[DC_VOLT_LEVEL];
	struct dc_bat_para_info bat_info;
	int stage_size;
};
struct dc_temp_para_info {
	int temp_min;
	int temp_max;
	int cur_max;
};

struct dc_resist_para_info {
	int resist_min;
	int resist_max;
	int resist_cur_max;
};

struct adaptor_info {
	int b_adp_type;
	int vendor_id_h;
	int vendor_id_l;
	int module_id_h;
	int module_id_l;
	int serrial_no_h;
	int serrial_no_l;
	int pchip_id;
	int hwver;
	int fwver_h;
	int fwver_l;
};

struct direct_charge_cable_detect_ops {
	int (*direct_charge_cable_detect)(void);
};

struct direct_charge_device {
	struct device *dev;
	struct smart_charge_ops* scp_ops;
	struct scp_power_supply_ops* scp_ps_ops;
	struct loadswitch_ops* ls_ops;
	struct batinfo_ops* bi_ops;
	struct direct_charge_cable_detect_ops* direct_charge_cable_detect;
	struct hrtimer threshold_caculation_timer;
	struct hrtimer charge_control_timer;
	struct hrtimer kick_watchdog_timer;
	struct workqueue_struct *direct_charge_wq;
	struct workqueue_struct *direct_charge_watchdog_wq;
	struct work_struct threshold_caculation_work;
	struct work_struct charge_control_work;
	struct work_struct fault_work;
	struct work_struct kick_watchdog_work;
	struct notifier_block fault_nb;
	struct dc_volt_para_info volt_para[DC_VOLT_LEVEL];
	struct dc_volt_para_info orig_volt_para[DC_VOLT_LEVEL];
	struct dc_volt_para_info_group *orig_volt_para_p;
	struct dc_temp_para_info temp_para[DC_TEMP_LEVEL];
	struct dc_resist_para_info resist_para[DC_RESIST_LEVEL];
	struct adaptor_info adp_info;
	struct nty_data *fault_data;
	int stage_need_to_jump[2*DC_VOLT_LEVEL];
	int error_cnt;
	enum direct_charge_mode scp_mode;
	unsigned int use_5A;
	unsigned int use_8A;
	int sysfs_enable_charger;
	int sysfs_disable_charger[__MAX_DISABLE_DIRECT_CHAGER];
	int sysfs_iin_thermal;
	int threshold_caculation_interval;
	int charge_control_interval;
	int cur_stage;
	int cur_vbat_th;
	int cur_ibat_th_high;
	int cur_ibat_th_low;
	int vbat;
	int ibat;
	int ibat_abnormal_cnt;
	int ibat_abnormal_th;
	int vadapt;
	int iadapt;
	int tadapt;
	int ls_vbus;
	int ls_ibus;
	int tls;
	int full_path_resistance;
	int stage_size;
	int stage_group_size;
	int stage_group_cur;
	int pre_stage;
	int adaptor_vset;
	int max_adaptor_vset;
	unsigned int max_tadapt;
	unsigned int max_tls;
	int adaptor_iset;
	int max_adaptor_iset;
	int init_delt_vset;
	int init_adapter_vset;
	int delta_err;
	int vstep;
	int scp_stop_charging_flag_info;
	int scp_stop_charging_flag_error;
	int max_dc_bat_vol;
	int min_dc_bat_vol;
	int bst_ctrl;
	int scp_power_en;
	int compensate_r;
	int compensate_v;
	int ls_id;
	char* ls_name;
	int vol_err_th;
	int full_path_res_max;
	int standard_cable_full_path_res_max;
	int full_path_res_threshold;
	int adaptor_leakage_current_th;
	int direct_charge_succ_flag;
	int first_cc_stage_timer_in_min;
	int max_adaptor_cur;
	int scp_cable_detect_enable;
	unsigned long first_cc_stage_timeout;
	int cc_cable_detect_enable;
	int cc_cable_detect_ok;
	int max_current_for_none_standard_cable;
	enum direct_charge_fault_type charge_fault;
	int adaptor_vendor_id;
	int scp_work_on_charger;
	int anti_fake_adaptor;
	int dc_err_report_flag;
	int dc_open_retry_cnt;
	int reverse_ocp_cnt;
	int otp_cnt;
	int adaptor_otp_cnt;
	int dc_volt_ratio;
	enum scp_stage_type scp_stage;
	int cutoff_normal_flag;
	int quick_charge_flag;
	int super_charge_flag;
	int scp_adaptor_detect_flag;
	char dc_err_dsm_buff[CHARGE_DMDLOG_SIZE];
	int scp_stop_charging_complete_flag;
	struct wake_lock direct_charge_lock;
	int adaptor_test_result_type;
	char thermal_reason[SCP_THERMAL_REASON_SIZE];
	char adaptor_antifake_key_index;
	int adaptor_antifake_check_enable;
	int dc_antifake_result;
	struct completion dc_af_completion;
	int sc_conv_ocp_count;
	int iin_thermal_default;
};

struct smart_charge_ops {
	int (*is_support_scp)(void);
	int (*scp_init)(struct scp_init_data*);
	int (*scp_exit)(struct direct_charge_device*);
	int (*scp_adaptor_detect)(void);
	int (*scp_set_adaptor_voltage)(int);
	int (*scp_get_adaptor_voltage)(void);
	int (*scp_set_adaptor_current)(int);
	int (*scp_get_adaptor_current)(void);
	int (*scp_get_adaptor_current_set)(void);
	int (*scp_get_adaptor_max_current)(void);
	int (*scp_adaptor_reset)(void);
	int (*scp_adaptor_output_enable)(int);
	int (*scp_chip_reset)(void);
	int (*scp_stop_charge_config)(void);
	int (*is_scp_charger_type)(void);
	int (*scp_get_adaptor_status)(void);
	int (*scp_get_chip_status)(void);
	int (*scp_get_adaptor_info)(void*);
	int (*scp_cable_detect)(void);
	int (*scp_get_adaptor_temp)(int*);
	int (*scp_get_adapter_vendor_id)(void);
	int (*scp_get_usb_port_leakage_current_info)(void);
	void (*scp_set_direct_charge_mode)(int);
	int (*scp_get_adaptor_type)(void);
	int (*scp_set_adaptor_encrypt_enable)(int);
	int (*scp_get_adaptor_encrypt_enable)(void);
	int (*scp_set_adaptor_random_num)(char *);
	int (*scp_get_adaptor_encrypt_completed)(void);
	int (*scp_get_adaptor_random_num)(char *);
	int (*scp_get_adaptor_encrypted_value)(char *);
};

struct scp_power_supply_ops {
	int (*scp_power_enable)(int);
};

void direct_charge_get_g_scp_ops(struct smart_charge_ops **ops);
void direct_charge_get_g_scp_ps_ops(struct scp_power_supply_ops **ops);
void direct_charge_get_g_cable_detect_ops(struct direct_charge_cable_detect_ops **ops);
void direct_charge_lvc_get_fault_notifier(struct atomic_notifier_head **notifier);
void direct_charge_sc_get_fault_notifier(struct atomic_notifier_head **notifier);
int direct_charge_parse_dts(struct device_node* np, struct direct_charge_device* di);
int batinfo_sc_ops_register(struct batinfo_ops* ops);
int sc_ops_register(struct loadswitch_ops* ops);
int direct_charge_fault_notifier_call(struct notifier_block *fault_nb, unsigned long event, void *data);
void charge_control_work(struct work_struct *work);
enum hrtimer_restart charge_control_timer_func(struct hrtimer *timer);
enum hrtimer_restart kick_watchdog_timer_func(struct hrtimer *timer);
enum hrtimer_restart threshold_caculation_timer_func(struct hrtimer *timer);
void kick_watchdog_work(struct work_struct *work);
void threshold_caculation_work(struct work_struct *work);
void direct_charge_or_set_local_mode(int dc_mode);
void direct_charge_and_set_local_mode(int dc_mode);
void battery_aging_safe_policy(struct direct_charge_device *di);
void scp_start_charging(void);
int scp_security_check(void);
int scp_direct_charge_init(void);
void scp_stop_charging(void);
int cutoff_normal_charge(void);
int can_battery_temp_do_direct_charge(void);
void scp_cable_detect(void);
void scp_set_stage_status(enum scp_stage_type stage_type);
int is_support_scp(void);
void direct_charge_send_normal_charge_uevent(void);
int is_direct_charge_ops_valid(struct direct_charge_device *di);
int can_battery_vol_do_direct_charge(void);
int direct_charge_get_lvc_di(struct direct_charge_device **di);
int direct_charge_get_sc_di(struct direct_charge_device **di);
void direct_charge_lvc_check(void);
void direct_charge_sc_check(void);
int direct_charge_get_local_mode(void);
int scp_adaptor_type_detect(int *mode);
void direct_charge_set_di(struct direct_charge_device *di);
int cable_detect_ops_register(struct direct_charge_cable_detect_ops*);
int scp_ops_register(struct smart_charge_ops*);
int scp_power_supply_ops_register(struct scp_power_supply_ops* ops);
int loadswitch_ops_register(struct loadswitch_ops*);
int batinfo_lvc_ops_register(struct batinfo_ops*);
int batinfo_sc_ops_register(struct batinfo_ops*);
void direct_charge_check(void);
void direct_charge_update_cutoff_flag(void);
void direct_charge_set_scp_stage_default(void);
void direct_charge_stop_charging(void);
enum scp_stage_type scp_get_stage_status(void);
void scp_set_stop_charging_flag(int flag);
int is_scp_stop_charging_complete(void);
int get_ls_vbus(void);
int scp_adaptor_detect(void);
int scp_adaptor_set_output_enable(int enable);
int direct_charge_get_cutoff_normal_flag(void);
int get_quick_charge_flag(void);
int get_super_charge_flag(void);
int is_direct_charge_failed(void);
int is_in_scp_charging_stage(void);
void direct_charge_send_quick_charge_uevent(void);
#ifdef CONFIG_SCHARGER_V300
#define HI6523_CV_CUT 150
extern bool is_hi6523_cv_limit(void);
#endif
extern int set_direct_charger_disable_flags(int, int);
extern struct blocking_notifier_head direct_charger_control_head;
bool direct_charge_check_sc_mode(void);
int direct_charge_gen_nl_init(struct platform_device *pdev);
int do_adpator_antifake_check(void);
enum direct_charge_mode direct_charge_get_adaptor_mode(void);

#ifdef CONFIG_SYSFS
#define DIRECT_CHARGE_SYSFS_FIELD(_name, n, m, store)                \
{                                                   \
    .attr = __ATTR(_name, m, direct_charge_sysfs_show, store),    \
    .name = DIRECT_CHARGE_SYSFS_##n,          \
}

#define DIRECT_CHARGE_SYSFS_FIELD_RW(_name, n)               \
	DIRECT_CHARGE_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO, direct_charge_sysfs_store)

#define DIRECT_CHARGE_SYSFS_FIELD_RO(_name, n)               \
	DIRECT_CHARGE_SYSFS_FIELD(_name, n, S_IRUGO, NULL)

struct direct_charge_sysfs_field_info {
	struct device_attribute attr;
	u8 name;
};
#endif

#ifdef CONFIG_LLT_TEST
#endif
#endif
