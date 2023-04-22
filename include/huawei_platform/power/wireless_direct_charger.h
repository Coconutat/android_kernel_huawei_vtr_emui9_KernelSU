#ifndef __WIRELESS_SC_H_
#define __WIRELESS_SC_H_

#include <huawei_platform/power/direct_charger.h>

#define WLDC_VOLT_LEVEL                   3
#define WLDC_ERR_CNT_MAX                  8
#define WLDC_WARNING_CNT_MAX              3
#define WLDC_OPEN_RETRY_CNT_MAX           3


#define WLDC_TBATT_MIN                    10
#define WLDC_TBATT_MAX                    45
#define WLDC_DEFAULT_VBATT_MAX            4350
#define WLDC_DEFAULT_VBATT_MIN            3550

#define WLDC_DEFAULT_VOUT_ERR_TH          100
#define WLDC_DEFAULT_IOUT_ERR_TH          150
#define WLDC_VOUT_ACCURACY_CHECK_CNT      3
#define WLDC_INIT_DELT_VOUT               300
#define WLDC_OPEN_PATH_CNT                6
#define WLDC_OPEN_PATH_IOUT_MIN           500
#define WLDC_DEFAULT_VSTEP                100
#define WLDC_LEAK_CURRENT_CHECK_CNT       6
#define WLDC_LEAK_CURRENT_MIN             100
#define WLDC_DIFF_VOLT_CHECK_CNT          3
#define WLDC_DIFF_VOLT_CHECK_TH           1200

#define WLDC_IOUT_MAX                     1400
#define SC_DEFAULT_VOLT_RATIO             2

#define WLDC_DEFAULT_CTRL_INTERVAL        100  //ms
#define WLDC_DEFAULT_CALC_INTERVAL        50 //ms

struct wldc_sysfs_data {
	int enable_charger;
	int iin_thermal;
};
enum wldc_stage {
	WLDC_STAGE_DEFAULT = 0,
	WLDC_STAGE_CHECK,
	WLDC_STAGE_SUCCESS,
	WLDC_STAGE_CHARGING,
	WLDC_STAGE_CHARGE_DONE,
	WLDC_STAGE_STOP_CHARGING,
	WLDC_STAGE_TOTAL,
};
enum wldc_sysfs_type {
	WLDC_SYSFS_ENABLE_CHARGER = 0,
	WLDC_SYSFS_IIN_THERMAL,
};

enum wldc_volt_info {
	WLDC_PARA_VBATT_TH = 0,
	WLDC_PARA_IOUT_TH_HIGH,
	WLDC_PARA_IOUT_TH_LOW,
	WLDC_PARA_TOTAL,
};

struct wldc_volt_para {
	int vbatt_th;
	int iout_th_high;
	int iout_th_low;
};
struct wldc_device_info {
	struct device *dev;
	struct loadswitch_ops* ls_ops;
	struct batinfo_ops* bi_ops;
	struct wldc_sysfs_data sysfs_data;
	struct wldc_volt_para volt_para[WLDC_VOLT_LEVEL];
	struct wldc_volt_para orig_volt_para[WLDC_VOLT_LEVEL];
	struct delayed_work wldc_ctrl_work;
	struct delayed_work wldc_calc_work;
	bool wldc_stop_charging_complete_flag;
	int wldc_stage;
	int stage_size;
	int ctrl_interval;
	int calc_interval;
	int wldc_stop_flag_error;
	int wldc_stop_flag_warning;
	int wldc_stop_flag_info;
	int volt_ratio;
	int error_cnt;
	int warning_cnt;
	int dc_open_retry_cnt;
	int vbatt_max;
	int vbatt_min;
	int rx_init_vout;
	int rx_vout_err_th;
	int rx_iout_err_th;
	int rx_init_delt_vout;
	int cur_vbat_th;
	int cur_iout_th_high;
	int cur_iout_th_low;
	int pre_stage;
	int cur_stage;
	int vstep;
	int rx_vout_set;
	int wldc_err_report_flag;
	char wldc_err_dsm_buff[CHARGE_DMDLOG_SIZE];
};

void wldc_set_di(struct wldc_device_info *di);
int wireless_sc_get_di(struct wldc_device_info **di);

int wldc_rx_ops_register(struct wireless_charge_device_ops *ops);
int wireless_sc_ops_register(struct loadswitch_ops* ops);
int wireless_sc_batinfo_ops_register(struct batinfo_ops* ops);

int can_vbatt_do_wldc_charge(struct wldc_device_info *di);
int can_tbatt_do_wldc_charge(void);

int wldc_retore_normal_charge_path(void);
int wldc_cutt_off_normal_charge_path(void);

int wireless_direct_charge_check(void);
int wireless_sc_charge_check(void);

int wldc_parse_dts(struct device_node *np, struct wldc_device_info *di);
int wldc_check_ops_valid(struct wldc_device_info *di);
int wldc_chip_init(struct wldc_device_info *di);
int wldc_security_check(struct wldc_device_info *di);

void wldc_start_charging(struct wldc_device_info *di);
void wldc_stop_charging(struct wldc_device_info *di);


void wldc_set_charge_stage(enum wldc_stage sc_stage);
int wldc_set_rx_init_vout(struct wldc_device_info *di);

void wldc_update_basp_para(struct wldc_device_info *di);
void wldc_control_work(struct work_struct *work);
void wldc_calc_work(struct work_struct *work);

int wldc_get_warning_cnt(void);
int wldc_get_error_cnt(void);
void wldc_tx_disconnect_handler(void);
bool wldc_is_stop_charging_complete(void);


#endif
