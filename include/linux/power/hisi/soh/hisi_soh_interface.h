#ifndef _HISI_SOH_INTERFACE_H_
#define _HISI_SOH_INTERFACE_H_

#define ACR_CHECK_CYCLE_S           (20*60)
#define ACR_MAX_BATTERY_CURRENT_MA   (100)

#define DCR_CHECK_CYCLE_S           (20*60)
#define DCR_MAX_BATTERY_CURRENT_MA   (100)

/**
 * struct acr_info - Basic representation of an acr info.
 * @batt_acr  :acr data mohm.
 * @batt_cycle:	battery charging cycles of this acr .
 * @chip_temp:	chip max and min temp in acr cal.
 * @batt_temp:  battery temperatue in acr cal.
 * @batt_vol:	battery vol(mv) in acr start.
 */
struct acr_info {
    int batt_acr;
	int batt_cycle;
	int batt_temp;
	int chip_temp[2];
	int batt_vol;
};

struct dcr_info {
    int batt_r0;
	int batt_dcr;  /*momh*/
	int batt_cycle;
	int batt_temp;
	int batt_vol;
};

/**
 * struct pd_leak_current_info - Basic representation of an pd leak current info.
 * @leak_current_ma :leak_current on poweroff.
 * @batt_cycle:	battery charging cycles of this leak current.
 * @chip_temp:	chip temp matches min and max current.
 * @batt_current: min and max current uA for pd current calculation.
 * @batt_vol:	pd voltage uv matches chip min and max current.
 * @rtc_time:	pd rtc_time matches chip min and max current.
 * @sys_pd_leak_cc:	sys standby leakage cc uah on poweroff .
 */
struct pd_leak_current_info {
	int leak_current_ma;
	int batt_cycle;
	int chip_temp[2];
	int batt_current[2];
    int batt_vol[2];
    int rtc_time[2];
    int sys_pd_leak_cc;
};


enum acr_type {
    ACR_H_PRECISION = 0, /*for pricison batt soh evaluation*/
    ACR_L_PRECISION = 1  /*for factory Aging test and low precision requirements*/
};

#ifdef CONFIG_HISI_SOH
int soh_get_acr_resistance(struct acr_info *r, enum acr_type type);
int soh_get_dcr_resistance(struct dcr_info *r);
int soh_get_poweroff_leakage(struct pd_leak_current_info *i);
void soh_acr_low_precision_cal_start(void);
#else
static int soh_get_acr_resistance(struct acr_info *r,  enum acr_type type){return 0;}
static int soh_get_dcr_resistance(struct dcr_info *r){return 0;}
static int soh_get_poweroff_leakage(struct pd_leak_current_info *i){return 0;}
static void soh_acr_low_precision_cal_start(void){;}
#endif

#endif
