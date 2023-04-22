#ifndef _LINUX_HISI_BCI_BATTERY_H
#define _LINUX_HISI_BCI_BATTERY_H


enum charge_status_event{
    VCHRG_POWER_NONE_EVENT = 0,
    VCHRG_NOT_CHARGING_EVENT,
    VCHRG_START_CHARGING_EVENT,
    VCHRG_START_AC_CHARGING_EVENT,
    VCHRG_START_USB_CHARGING_EVENT,
    VCHRG_CHARGE_DONE_EVENT,
    VCHRG_STOP_CHARGING_EVENT,
    VCHRG_POWER_SUPPLY_OVERVOLTAGE,
    VCHRG_POWER_SUPPLY_WEAKSOURCE,
    VCHRG_STATE_WDT_TIMEOUT,
    VCHRG_STATE_WATER_INTRUSED,
    BATTERY_LOW_WARNING,
    BATTERY_LOW_SHUTDOWN,
    BATTERY_MOVE,
    SWITCH_INTB_WORK_EVENT,
    VCHRG_THERMAL_POWER_OFF,
    WIRELESS_TX_STATUS_CHANGED,
    VCHRG_CURRENT_FULL_EVENT,
};

#define CURRENT_OFFSET    (10)

#define DSM_BATTERY_MAX_SIZE 2048

#define IMPOSSIBLE_IAVG            (9999)

/* State-of-Charge-Threshold 1 bit */
#define BQ_FLAG_SOC1             (1<<2)
/* State-of-Charge-Threshold Final bit */
#define BQ_FLAG_SOCF             (1<<1)
#define BQ_FLAG_LOCK       		 (BQ_FLAG_SOC1 | BQ_FLAG_SOCF)


#define BAT_VOL_3200    (3200)
#define BAT_VOL_3500    (3500)
#define BAT_VOL_3550    (3550)
#define BAT_VOL_3600    (3600)
#define BAT_VOL_3700    (3700)
#define BAT_VOL_3800    (3800)
#define BAT_VOL_3850    (3850)
#define BAT_VOL_3900    (3900)
#define BAT_VOL_3950    (3950)
#define BAT_VOL_4000    (4000)
#define BAT_VOL_4200    (4200)
#define BAT_VOL_4250    (4250)

#define RECHG_PROTECT_THRESHOLD    (150)
#define WORK_INTERVAL_PARA_LEVEL    (5)
#define CAPACITY_MIN   0
#define CAPACITY_MAX 100
#define CAPACITY_FULL 100
#define WORK_INTERVAL_MIN             1
#define WORK_INTERVAL_MAX            60000 /*60s*/
#define WORK_INTERVAL_NOARMAL 10000 /*10s*/
#define WORK_INTERVAL_REACH_FULL 30000 /*30s*/

#define CHARGE_CURRENT_OVERHIGH_TH 10000
#define DISCHARGE_CURRENT_OVERHIGH_TH (-7000)

#define BATT_VOLT_OVERLOW_TH 2800
#define BATT_VOLT_OVERHIGH_TH 4550

#define BATT_TEMP_OVERLOW_TH (-10)

#define BATT_NOT_TERMINATE_TH 50

#define BATT_CAPACITY_REDUCE_TH (80)

#define DSM_CHECK_TIMES_LV1 3
#define DSM_CHECK_TIMES_LV2 5
#define DSM_CHECK_TIMES_LV3 10

#define CHG_ODD_CAPACITY                (2)
#define CHG_CANT_FULL_THRESHOLD         (95)
#define CHARGE_FULL_TIME                ((40*60*1000)/WORK_INTERVAL_REACH_FULL)	/*40min*/
#define CURRENT_THRESHOLD               (10)

#define MAX_CONFIRM_CNT      (3)
#define CONFIRM_INTERVAL     (100) /*100 ms*/

#define BASE_DECIMAL (10)
#define CAPACITY_DEC_SAMPLE_NUMS       (12)
#define CAPATICY_DEC_TIMER_INTERVAL  (800) /* time out setting, 0.8 seconds */
#define CAPATICY_DEC_PARA_LEVEL       (2)

#define CAP_LOCK_PARA_LEVEL    (2)
enum plugin_status {
    /* no charger plugin */
    PLUGIN_DEVICE_NONE,
   /* plugin usb charger */
    PLUGIN_USB_CHARGER,
   /* plugin ac charger */
   PLUGIN_AC_CHARGER,
};

enum work_interval_para_info {
	WORK_INTERVAL_CAP_MIN = 0,
	WORK_INTERVAL_CAP_MAX,
	WORK_INTERVAL_VALUE,
	WORK_INTERVAL_PARA_TOTAL,
};

struct work_interval_para {
	int cap_min;
	int cap_max;
	int work_interval;
};
enum capacity_dec_para_info {
	CAP_DEC_BASE_DECIMAL = 0,
	CAP_DEC_SAMPLE_NUMS,
	CAP_DEC_TIMER_INTERVAL,
	CAP_DEC_PARA_TOTAL,
};
struct capacity_dec_para {
	u32 cap_dec_base_decimal;
	u32 cap_dec_sample_nums;
	u32 cap_dec_timer_interval;
};
struct cap_lock_para {
	int cap;
	int level_vol;
};

typedef enum{
    BAT_BOARD_SFT  = 0,
    BAT_BOARD_UDP  = 1,
    BAT_BOARD_ASIC = 2,
    BAT_BOARD_MAX
}battery_board_type_en;
int hisi_register_notifier(struct notifier_block *nb,
                unsigned int events);
int hisi_unregister_notifier(struct notifier_block *nb,
                unsigned int events);
int hisi_bci_show_capacity(void);
struct class *hw_power_get_class(void);
extern enum fcp_check_stage_type  fcp_get_stage_status(void);
extern unsigned int get_bci_soc(void);
extern void batt_info_dump(char* pstr);

#endif
