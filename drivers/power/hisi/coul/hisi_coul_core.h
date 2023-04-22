#ifndef _HISI_COUL_CORE_H_
#define _HISI_COUL_CORE_H_

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <asm/irq.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/reboot.h>
#include <asm/bug.h>
#include <linux/hisi/hisi_adc.h>
#include <linux/mtd/hisi_nve_interface.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include <linux/power/hisi/hisi_battery_data.h>
#include <linux/power/hisi/hisi_bci_battery.h>
#include <linux/suspend.h>
#include <linux/timer.h>
#include <linux/rtc.h>
#include <linux/syscalls.h>
#include <linux/semaphore.h>
#include <linux/wakelock.h>
#ifdef CONFIG_HUAWEI_CHARGER
#include <huawei_platform/power/huawei_charger.h>
#else
#include <linux/power/hisi/charger/hisi_charger.h>
#endif
#ifdef CONFIG_DIRECT_CHARGER
#include <huawei_platform/power/direct_charger.h>
#endif
#include <linux/hisi/ipc_msg.h>
#include <linux/hisi/hisi_rproc.h>
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif
/*TP ocv*/
#ifndef min
#define min(a,b)    ((a)<(b)?(a):(b))
#endif

#define IPC_BAT_OCVINFO IPC_CMD(OBJ_LPM3, OBJ_AP, CMD_NOTIFY, TYPE_BAT)
#define SMARTSTAR_DEBUG
#define RBATT_ADJ 1

#ifndef BIT
#define BIT(x)      (1 << (x))
#endif

/*fifo max*/
#define VOL_FIFO_MAX     10
#define VOL_MAX_DIFF_UV  5000

/*low temp opt*/
#define LOW_TEMP_OPT_OPEN    1
#define LOW_TEMP_OPT_CLOSE   0
#define UUC_MIN_CURRENT_MA  (400)

#define NOT_UPDATE_FCC  0x1
#define IS_UPDATE_FCC   0


#define DI_LOCK()    do {mutex_lock(&di->soc_mutex);} while (0)
#define DI_UNLOCK()  do {mutex_unlock(&di->soc_mutex);} while (0)


#define ABNORMAL_BATT_TEMPERATURE_LOW   (-40)
#define ABNORMAL_BATT_TEMPERATURE_HIGH  (80)
#define DEFAULT_TEMP             25         /* SFT  UDP */
#define TEMP_TOO_HOT            (60)
#define TEMP_TOO_COLD           (-20)
#define TEMP_IPC_GET_ABNORMAL	(100)
#define TEMP_OCV_ALLOW_CLEAR    (5)
#define TEMP_THRESHOLD_CALI     (2)
#define ABNORMAL_DELTA_SOC      (10)

#define CHARGING_STATE_CHARGE_START         (1)
#define CHARGING_STATE_CHARGE_DONE          (2)
#define CHARGING_STATE_CHARGE_STOP          (3)
#define CHARGING_STATE_CHARGE_RECHARGE      (4)
#define CHARGING_STATE_CHARGE_NOT_CHARGE    (5)
#define CHARGING_STATE_CHARGE_UNKNOW        (0x0)

#define BOOT_LOAD  (1)
#define CHANGE_LOAD  (0)
#define IAVG_SAMPLES 10
#define CHARGING_IAVG_MA 250
#define MIN_CHARGING_CURRENT_OFFSET (-10)

#define CHANGE_BATTERY_MOVE    1    /* bettery move */
#define CHANGE_BATTERY_NEED_RESTORE 2    /* need restore chargecycle after battery plug in */
#define RESTORE_CYCLE_MAX    (40*100)    /* need not restore chargecycle,because overtime */
#define DELTA_FCC_INVALID_RADIO    5    /* maximum error standard for two times learning */
#define NEW_BATT_DIFF_RADIO    97    /* the standard near the new battery */
#define OLD_BATT_DIFF_RADIO    3    /* maximum error standard for original battery */

#define BASP_FCC_LERAN_TEMP_MIN (200) /*tenth degree*/
#define BASP_FCC_LERAN_TEMP_MAX (450) /*tenth degree*/
#define BASP_DEFAULT_RECORD_FCC_AVG   9999

enum BASP_POLICY_MEM_TAG {
    BASP_MEM_LEVEL,
    BASP_MEM_CHRG_CYCLES,
    BASP_MEM_FCC_RATIO,
    BASP_MEM_DC_VOLT_DEC,
    BASP_MEM_NONDC_VOLT_DEC,
    BASP_MEM_CUR_RATIO,
    BASP_MEM_CUR_RATIO_POLICY,
    BASP_MEM_ERR_NO,
    BASP_MEM_CNT
};

typedef enum BASP_FCC_LEARN_EVENT {
    EVT_START,
    EVT_PER_CHECK,/*periodic check*/
    EVT_DONE,
} LEARN_EVENT_TYPE;

typedef enum BATTERY_TEMP_USER {
    USER_COUL,
    USER_CHARGER,
    USER_CNT,
}BATTERY_TEMP_USER_TYPE;

enum avg_curr_level {
	AVG_CURR_250MA = 0,
	AVG_CURR_500MA,
	AVG_CURR_MAX,
};

enum ocv_level {
	OCV_LEVEL_0 = 0,
	OCV_LEVEL_1,
	OCV_LEVEL_2,
	OCV_LEVEL_3,
	OCV_LEVEL_4,
	OCV_LEVEL_5,
	OCV_LEVEL_6,
	OCV_LEVEL_7,
	OCV_LEVEL_MAX,
};



#define INVALID_SAVE_OCV_LEVEL  (OCV_LEVEL_MAX)
#define LOW_PRECISION_OCV_LEVEL (OCV_LEVEL_4)
#define CURR2UPDATE_OCV_TIME (10*60)
#define DELTA_SAFE_FCC_PERCENT 5
#define DELTA_MAX_DECR_FCC_PERCENT  5
#define DELTA_MAX_INCR_FCC_PERCENT  10
#define CALCULATE_SOC_MS    (20*1000)
#define READ_TEMPERATURE_MS (5*1000)
#define TEMPERATURE_INIT_STATUS 1
#define TEMPERATURE_UPDATE_STATUS 0
#define TEMPERATURE_CHANGE_LIMIT 50
#define CALIBRATE_INTERVAL (5*60*1000)    /* 5 min */
#define CHARGED_OCV_UPDATE_INTERVAL_S (10*60)


#define DEFAULT_SOC_MONITOR_LIMIT (100)
#define DEFAULT_SOC_MONITOR_TEMP_MIN (10)
#define DEFAULT_SOC_MONITOR_TEMP_MAX (45)

#define  DEFAULT_HKADC_BATT_TEMP  10
#define  DEFAULT_HKADC_BATT_ID    11

#ifdef SMARTSTAR_DEBUG
#define DBG_CNT_INC(xxx)     di->xxx++
#else
#define DBG_CNT_INC(xxx)
#endif

#define BATTERY_CC_LOW_LEV      3
#define BATTERY_CC_WARNING_LEV  10
#define BATTERY_SWITCH_ON_VOLTAGE        (3250)
#define BATTERY_VOL_LOW_ERR 2900

#define LOW_INT_NOT_SET 0
#define LOW_INT_STATE_RUNNING 1
#define LOW_INT_STATE_SLEEP 0
#define LOW_INT_VOL_OFFSET 10     /* mv */
#define BATTERY_VOL_2_PERCENT 3350

#define ZERO_VOLTAGE_PLUS_5 3200
#define ZERO_VOLTAGE_MINUS_10 3200

#define MAX_TEMPS 10
#define MAX_RECORDS_CNT 5

#define INVALID_BATT_ID_VOL  -999
#define IAVG_TIME_2MIN   6 //6*20s

#define SR_ARR_LEN             100
#define SR_MAX_RESUME_TIME     90         // 90 s
#define SR_DELTA_SLEEP_TIME    (4 * 60)   // 4 min
#define SR_DELTA_WAKEUP_TIME   30         // 30 s
#define SR_TOTAL_TIME          (30 * 60)  // 30 min
#define SR_DUTY_RATIO          95
#define SR_DEVICE_WAKEUP       1
#define SR_DEVICE_SLEEP        2

#define HISI_COUL_NV_NAME         "HICOUL"
#define HISI_BAT_CALIBRATION_NV_NAME         "BATCALI"
#define HISI_COUL_NV_NUM          316
#define HISI_BAT_CALIBRATION_NV_NUM          317

#define HW_COUL_NV_NAME  "HWCOUL"
#define HW_COUL_NV_NUM   392
#define NV_OPERATE_SUCC  0
#define NV_NOT_DEFINED   -22

#define MIN_BEGIN_PERCENT_FOR_LEARNING 60
#define MIN_BEGIN_PERCENT_FOR_SAFE 20
#define MAX_DELTA_RC_PC 1

#define FCC_UPDATE_TEMP_MIN (100) /*tenth degree*/
#define FCC_UPDATE_TEMP_MAX (450) /*tenth degree*/
#define FCC_UPDATE_CHARGING_CURR_MIN_MA  200

/*high pre qmax*/
#define MIN_BEGIN_PERCENT_FOR_QMAX   20

/*get ocv by vol*/
#define CURRENT_LIMIT (20*1000)
#define DEFAULT_BATTERY_OHMIC_RESISTANCE 100 /* mohm */

#define FLAG_USE_CLAC_OCV 0xABCD
#define FLAG_USE_PMU_OCV  0xCDEF


#define DEFAULT_V_OFF_A 1014000
#define DEFAULT_V_OFF_B 0
#define DEFAULT_C_OFF_A 1000000
#define DEFAULT_C_OFF_B 0

#define CHARGING_CURRENT_OFFSET (-10*1000)


#define BATTERY_PLUG_OUT 1
#define BATTERY_PLUG_IN  0
#define BATTERY_CHECK_TIME_MS (1*1000)

#define PMU_NV_ADDR_CMDLINE_MAX_LEN 30

/*flag:1 success 0:fail*/
#define NV_SAVE_SUCCESS 1
#define NV_SAVE_FAIL 0
#define NV_READ_SUCCESS 1
#define NV_READ_FAIL    0

/*NTC Table length*/
#define T_V_ARRAY_LENGTH 31

#define ABNORMAL_BATT_TEMPERATURE_POWEROFF 670
#define DELTA_TEMP 150
#define LOW_BATT_TEMP_CHECK_THRESHOLD -100

#define HALF            2
#define QUARTER   4
#define SUCCESS    0
#define ERROR         1
#define ENABLED     1
#define DISABLED    0
#define ISCD_VALID           1
#define ISCD_INVALID       0
#define INVALID_ISC   999999

#define UOHM_PER_MOHM 1000
#define MOHM_PER_OHM 1000

#define UVOLT_PER_MVOLT 1000
#define UA_PER_MA      1000

#define SEC_PER_HOUR 3600

#define SOC_FULL        100
#define TENTH               10
#define PERCENT         100
#define PERMILLAGE  1000

#define FCC_MAX_PERCENT  135
#define FCC_UPDATE_MAX_OCV_INTERVAL   12

#define ISCD_SAMPLE_RETYR_CNT    3
#define ISCD_SMAPLE_TIME_MIN  (3*3600) //3h
#define ISCD_SMAPLE_LEN_MAX 30
#define ISCD_SAMPLE_INTERVAL_MAX  (12*3600)  //12h
#define ISCD_SAMPLE_INTERVAL_DELTA 20 //s
#define ISCD_DEFAULT_OCV_MIN 4000000  //4V
#define ISCD_DEFAULT_TBATT_MIN 100
#define ISCD_DEFAULT_TBATT_MAX 500
#define ISCD_DEFAULT_TBATT_DIFF 100
#define ISCD_DEFAULT_SAMPLE_INTERVAL 600 //10min
#define ISCD_DEFAULT_SAMPLE_DELAY 10 //s
#define ISCD_DEFAULT_CALC_INTERVAL_MIN 3600 //1h
#define ISCD_CALC_INTERVAL_900S 900 //15min
#define ISCD_INVALID_SAMPLE_CNT_FROM 1
#define ISCD_INVALID_SAMPLE_CNT_TO 2
#define ISCD_STANDBY_SAMPLE_CNT (-1)  //for standby mode
#define ISCD_CHARGE_CYCLE_MIN  10
#define ISCD_CHRG_DELAY_CYCLES 0
#define ISCD_DELAY_CYCLES_ENABLE 0

#define ISCD_WARNING_LEVEL_THREHOLD  10000 //uA
#define ISCD_ERROR_LEVEL_THREHOLD  30000 //uA
#define ISCD_CRITICAL_LEVEL_THREHOLD  100000 //uA
#define ISCD_FATAL_LEVEL_THREHOLD  200000 //uA
#define ISCD_RECHARGE_CC   1000  //uAh
#define ISCD_LARGE_ISC_THREHOLD  50000 //uA
#define ISCD_LARGE_ISC_VALID_SIZE      10
#define ISCD_LARGE_ISC_VALID_PERCENT  50  //50%
#define ISCD_SMALL_ISC_VALID_SIZE_1      40
#define ISCD_SMALL_ISC_VALID_SIZE_2      30
#define ISCD_SMALL_ISC_VALID_PERCENT     50  //50%


#define ISCD_OCV_FIFO_VALID_CNT 3
#define ISCD_OCV_UV_VAR_THREHOLD   3000000  //ocv variance threhold
#define ISCD_OCV_UV_MIN  2500000  //2.5V
#define ISCD_OCV_UV_MAX 4500000 //4.5V
#define ISCD_OCV_DELTA_MAX  10000 //10mV
#define ISCD_CC_DELTA_MAX  10000 //10mAh

#define ISCD_ISC_MAX_SIZE 512

#define ISCD_MAX_LEVEL 10
#define ISCD_DSM_REPORT_CNT_MAX  3
#define ISCD_DSM_REPORT_INTERVAL   (24*3600)   //24h
#define ISCD_DSM_LOG_SIZE_MAX      (2048)
#define ISCD_ERR_NO_STR_SIZE 128

#define MAX_FATAL_ISC_NUM               20
#define MAX_TRIGGER_LEVEL_NUM           5
#define ELEMS_PER_CONDITION             3
#define ISC_DAYS_PER_YEAR               365
#define TM_YEAR_OFFSET                  1900
#define DEFAULT_FATAL_ISC_DEADLINE      15
#define DEFAULT_FATAL_ISC_UPLIMIT_SOC   60
#define DEFAULT_FATAL_ISC_RECHAGE_SOC   55
#define SUCCESSIVE_ISC_JUDGEMENT_TIME   1
#define INVALID_ISC_JUDGEMENT           0
#define UPLIMIT                         0
#define RECHARGE                        1
#define F2FS_MOUNTS_INFO                "/proc/mounts"
#define MOUNTS_INFO_FILE_MAX_SIZE       (PAGE_SIZE - 1)
#define SPLASH2_MOUNT_INFO              "/splash2"
#define ISC_DATA_DIRECTORY              "/splash2/isc"
#define ISC_DATA_FILE                   "/splash2/isc/isc.data"
#define ISC_CONFIG_DATA_FILE            "/splash2/isc/isc_config.data"
#define WAIT_FOR_SPLASH2_START          5000
#define WAIT_FOR_SPLASH2_INTERVAL       1000
#define ISC_SPLASH2_INIT_RETRY          3
#define FATAL_ISC_DMD_RETRY             5
#define FATAL_ISC_DMD_RETRY_INTERVAL    2500
#define FATAL_ISC_DMD_LINE_SIZE         42
#define FATAL_ISC_MAGIC_NUM             0x5a3c7711
#define FATAL_ISC_TRIGGER_ISC_OFFSET    0
#define FATAL_ISC_TRIGGER_NUM_OFFSET    1
#define FATAL_ISC_TRIGGER_DMD_OFFSET    2
#define FATAL_ISC_DMD_ONLY              1
#define ISC_LIMIT_CHECKING_INTERVAL     5000
#define ISC_LIMIT_START_CHARGING_STAGE  0
#define ISC_LIMIT_UNDER_MONITOR_STAGE   1
#define ISC_LIMIT_STOP_CHARGING_STAGE   2
#define ISC_LIMIT_BOOT_STAGE            3
#define ISC_TRIGGER_WITH_TIME_LIMIT     1
#define ISC_APP_READY                   1
#define FATAL_ISC_OCV_UPDATE_THRESHOLD  20
#define FATAL_ISC_ACTION_DMD_ONLY       0x01 //enable dmd report only

#define CAPACITY_DENSE_AREA_3200	(3200000)
#define CAPACITY_DENSE_AREA_3670	(3670000)
#define CAPACITY_DENSE_AREA_3690	(3690000)
#define CAPACITY_DENSE_AREA_3730	(3730000)
#define CAPACITY_DENSE_AREA_3800	(3800000)
#define CAPACITY_DENSE_AREA_3830	(3830000)
#define CAPACITY_INVALID_AREA_4500	(4500000)
#define CAPACITY_INVALID_AREA_2500	(2500000)
#define COUL_MINUTES(x) (x*60)
#define POLAR_OCV_TEMP_LIMIT (100)
#define POLAR_ECO_IBAT_LIMIT (50)
#define POLAR_OCV_TSAMPLE_LIMIT (5)
#define POLAR_SR_VOL0_LIMIT (3500)
#define POLAR_SR_VOL1_LIMIT (7000)

#define CURRENT_FULL_TERM_SOC 95
#define DELTA_MAX_FULL_FCC_PERCENT 10


enum ISCD_LEVEL_CONFIG {
    ISCD_ISC_MIN,
    ISCD_ISC_MAX,
    ISCD_DSM_ERR_NO,
    ISCD_DSM_REPORT_CNT,
    ISCD_DSM_REPORT_TIME,
    ISCD_PROTECTION_TYPE,
    ISCD_LEVEL_CONFIG_CNT
};

enum CHIP_TEMP_TYPE{
    ECO_OUT,
    NORMAL,
    TYPE_CNT
};



#define TIMESTAMP_STR_SIZE 32
#define DSM_BUFF_SIZE_MAX 1024
#define INDEX_MAX 3
#define COUL_ABN_COUNT 30
#define SOC_JUMP_MAX 5
#define CALI_RBATT_CC_MAX 20 //mAh
#define CALI_RBATT_CURR_MIN 500 //mA
#define BATTERY_DEC_ENABLE 1
struct coul_ocv_cali_info {
    char cali_timestamp[TIMESTAMP_STR_SIZE];
    int cali_ocv_uv;
    int cali_ocv_temp; //tenth degree
    s64 cali_cc_uah;
    int cali_rbatt;
    unsigned char cali_ocv_level;
};

/*max 104byte*/
struct ss_coul_nv_info{
    int charge_cycles;
    int v_offset_a;
    int v_offset_b;
    int c_offset_a;
    int c_offset_b;
    int limit_fcc;
    short temp[MAX_TEMPS];
    short real_fcc[MAX_TEMPS];
    /*below get data from fastboot,not need to save*/
    short calc_ocv_reg_v;
    short calc_ocv_reg_c;
    short hkadc_batt_id_voltage;
    short real_fcc_record[MAX_RECORDS_CNT];
    short latest_record_index;
    short fcc_check_sum;
    int fcc_check_sum_ext;
    short change_battery_learn_flag;
    short qmax; //92byte
    int report_full_fcc_real;
};

struct hw_coul_nv_info{
    int charge_cycles;
    short temp[MAX_TEMPS];
    short real_fcc[MAX_TEMPS];
};

struct vcdata{
    int avg_v; //mv
    int avg_c; //ma
    int min_c; //ma
    int max_c; //ma
};

struct coul_device_ops{
    int   (*calculate_cc_uah)(void);
    void  (*save_cc_uah)(int cc_uah);
    int   (*convert_ocv_regval2ua)(short reg_val);
    int   (*convert_ocv_regval2uv)(short reg_val);
    int   (*is_battery_moved)(void);
    void  (*set_battery_moved_magic_num)(int);
    void  (*get_fifo_avg_data)(struct vcdata *vc);
    int   (*get_fifo_depth)(void);
    int   (*get_delta_rc_ignore_flag)(void);
    int   (*get_nv_read_flag)(void);
    void  (*set_nv_save_flag)(int nv_flag);
    int   (*set_hltherm_flag)(int temp_protect_flag);
    int   (*get_hltherm_flag)(void);
    int   (*get_use_saved_ocv_flag)(void);
    int   (*get_fcc_invalid_up_flag)(void);
    void  (*save_ocv)(int ocv, int invalid_fcc_up_flag);
    short (*get_ocv)(void);
    void  (*clear_ocv)(void);
    void  (*save_ocv_temp)(short ocv_temp);
    short (*get_ocv_temp)(void);
    void  (*clear_ocv_temp)(void);
    void  (*set_low_low_int_val)(int vol_mv);
    int   (*get_abs_cc)(void);
    unsigned int   (*get_coul_time)(void);
    void  (*clear_coul_time)(void);
    void  (*clear_cc_register)(void);
    void  (*cali_adc)(void);
    int   (*get_battery_voltage_uv)(void);
    int   (*get_battery_current_ua)(void);
    int   (*get_battery_vol_uv_from_fifo)(unsigned int fifo_order);
    int   (*get_battery_cur_ua_from_fifo)(unsigned int fifo_order);
    short (*get_offset_current_mod)(void);
    short (*get_offset_vol_mod)(void);
    void  (*set_offset_vol_mod)(void);
    int   (*get_ate_a)(void);
    int   (*get_ate_b)(void);
    void  (*irq_enable)(void);
    void  (*irq_disable)(void);
    void  (*show_key_reg)(void);
    void  (*enter_eco)(void);
    void  (*exit_eco)(void);
    int   (*calculate_eco_leak_uah)(void);
    void  (*save_last_soc)(short soc);
    void  (*get_last_soc)(short *soc);
    void  (*clear_last_soc_flag)(void);
    void  (*get_last_soc_flag)(bool *valid);
    void  (*cali_auto_off)(void);
    void  (*save_ocv_level)(u8 level);
    void  (*get_ocv_level)(u8 *level);
    void  (*set_i_in_event_gate)(int ma);
    void  (*set_i_out_event_gate)(int ma);
    int   (*get_chip_temp)(enum CHIP_TEMP_TYPE type);
    int   (*get_bat_temp)(void);
    int   (*convert_regval2uv)(unsigned int reg_val);
    int   (*convert_regval2ua)(unsigned int reg_val);
    int   (*convert_regval2temp)(unsigned int reg_val);
    unsigned int   (*convert_uv2regval)(int uv_val);
    int   (*convert_regval2uah)(u64 reg_val);
    void (*set_eco_sample_flag)(u8 set_val);
    void (*get_eco_sample_flag)(u8 *get_val);
    void (*clr_eco_data)(u8 set_val);
    int   (*get_coul_calibration_status)(void);
    int   (*get_drained_battery_flag)(void);
    void  (*clear_drained_battery_flag)(void);
    void (*set_bootocv_sample)(u8 set_val);
};

enum coul_fault_type{
    COUL_FAULT_NON = 0,
    COUL_FAULT_LOW_VOL,
    COUL_FAULT_CL_INT,
    COUL_FAULT_CL_IN,
    COUL_FAULT_CL_OUT,
    COUL_FAULT_I_OUT,
    COUL_FAULT_I_IN,
    COUL_FAULT_TOTAL,
};

enum nv_operation_type {
    NV_WRITE_TYPE = 0,
    NV_READ_TYPE,
};

struct coul_cali_nv_info
{
    int v_offset_a;
    int v_offset_b;
    int c_offset_a;
    int c_offset_b;
    int c_chip_temp;
};
struct iscd_sample_info
{
    struct timespec sample_time;
    int sample_cnt; //sample counts since charge/recharge done
    int ocv_volt_uv; //uV
    s64 ocv_soc_uAh; //uAh, capacity get by OCV form battery model parameter table
    s64 cc_value; //uAh
    int tbatt;
};
struct iscd_level_config
{
    int isc_min;
    int isc_max;
    int isc_valid_cnt;
    int dsm_err_no;
    int dsm_report_cnt;
    time_t dsm_report_time;
    int protection_type;
};
/**
 * struct isc_history - isc history information
 *
 * @isc_status: isc status(0 means normal)
 * @valid_num: max valid isc(fcc...) number(0 means nothing valid)
 */
typedef struct {
    unsigned char isc_status;
    unsigned char valid_num;
    unsigned char dmd_report;
    unsigned char trigger_type;
    unsigned int magic_num;
    int isc[MAX_FATAL_ISC_NUM];
    int rm[MAX_FATAL_ISC_NUM];
    int fcc[MAX_FATAL_ISC_NUM];
    int qmax[MAX_FATAL_ISC_NUM];
    unsigned int charge_cycles[MAX_FATAL_ISC_NUM];
    unsigned short year[MAX_FATAL_ISC_NUM];
    unsigned short yday[MAX_FATAL_ISC_NUM];
} isc_history;

typedef struct {
    unsigned int write_flag;
    unsigned int delay_cycles;
    unsigned int magic_num;
    unsigned int has_reported;
} isc_config;

typedef struct {
    unsigned int valid_num;
    unsigned int deadline;
    unsigned int trigger_num[MAX_TRIGGER_LEVEL_NUM];
    int trigger_isc[MAX_TRIGGER_LEVEL_NUM];
    int dmd_no[MAX_TRIGGER_LEVEL_NUM];
} isc_trigger;

typedef struct {
    struct delayed_work work;
    char * buff;
    int err_no;
    int retry;
} fatal_isc_dmd;

enum fatal_isc_action_type {
    UPLOAD_DMD_ACTION = 0,
    NORAML_CHARGING_ACTION,
    DIRECT_CHARGING_ACTION,
    UPLOAD_UEVENT_ACTION,
    UPDATE_OCV_ACTION,
    __MAX_FATAL_ISC_ACTION_TYPE,
};

struct iscd_info {
    int size;
    int isc;//internal short current, uA
    int isc_valid_cycles;
    int isc_valid_delay_cycles;
    int isc_chrg_delay_cycles;
    int isc_delay_cycles_enable;
    int has_reported;
    int write_flag;
    unsigned int isc_status;
    unsigned int fatal_isc_trigger_type;
    unsigned int fatal_isc_soc_limit[2];
    unsigned int fatal_isc_action;
    unsigned int fatal_isc_action_dts;
    fatal_isc_dmd dmd_reporter;
    spinlock_t boot_complete;
    unsigned int app_ready;
    unsigned int uevent_wait_for_send;
    int isc_prompt;
    int isc_splash2_ready;
    isc_history fatal_isc_hist;
    isc_config fatal_isc_config;
    isc_trigger fatal_isc_trigger;
    s64 full_update_cc;//uAh
    int last_sample_cnt; //last sample counts since charge/recharge done
    struct iscd_sample_info sample_info[ISCD_SMAPLE_LEN_MAX];
    int rm_bcd;   //rm bofore charging done
    int fcc_bcd;  //fcc bofore charging done

    int enable;
    int ocv_min;
    int tbatt_min;
    int tbatt_max;
    int tbatt_diff_max;
    int sample_time_interval;
    int sample_time_delay;
    int calc_time_interval_min;
    int calc_time_interval_max;
    int isc_warning_threhold;
    int isc_error_threhold;
    int isc_critical_threhold;
    int total_level;
    int isc_buff[ISCD_ISC_MAX_SIZE];  //isc_buff[0]:buffer size
    struct timespec last_sample_time;
    struct iscd_level_config level_config[ISCD_MAX_LEVEL];
    struct delayed_work delayed_work;/*ISCD: detect_battery short current*/
    struct hrtimer timer;
    struct mutex iscd_lock;

    int need_monitor;
    struct delayed_work isc_limit_work;
    struct blocking_notifier_head isc_limit_func_head;
    struct notifier_block fatal_isc_direct_chg_limit_soc_nb;
    struct notifier_block fatal_isc_chg_limit_soc_nb;
    struct work_struct fatal_isc_uevent_work;
    struct notifier_block fatal_isc_uevent_notify_nb;
    struct notifier_block fatal_isc_ocv_update_nb;
    struct notifier_block fatal_isc_dsm_report_nb;
    struct notifier_block isc_listen_to_charge_event_nb;

    struct delayed_work isc_splash2_work;
    int isc_datum_init_retry;

    unsigned int ocv_update_interval;
};

struct smartstar_coul_device
{
    int batt_exist;
    int prev_pc_unusable;
    int batt_ocv; // ocv in uv
    int batt_ocv_temp;
    int batt_ocv_valid_to_refresh_fcc;
    int batt_changed_flag;
    int batt_reset_flag;
    int soc_limit_flag;
    int batt_temp; // temperature in degree*10
    int qmax;  //uAh
    int batt_fcc;
    int batt_limit_fcc;
    int batt_limit_fcc_begin;
    int batt_report_full_fcc_real;
    int batt_report_full_fcc_cal;
    int batt_rm;
    int batt_ruc;
    int batt_uuc;
    int batt_delta_rc;
    int batt_pre_delta_rc;
    int rbatt;
    int rbatt_ratio;
    int r_pcb;
    int soc_work_interval;
    int charging_begin_soc;
    int charging_state;
    int charging_stop_soc;  /*the unit is uah*/
    int charging_stop_time;
    unsigned int dischg_ocv_enable;
    int dischg_ocv_soc;
    int batt_soc;
    int batt_soc_real;
    int batt_soc_with_uuc;
    int batt_soc_est;
    int product_index;
    unsigned int batt_id_vol;
    unsigned int batt_chargecycles; //chargecycle in percent
    int last_cali_temp; // temperature in degree*10
    int cc_end_value;
    unsigned int v_cutoff;
    unsigned int v_cutoff_sleep;
    unsigned int v_low_int_value;
    unsigned int 	get_cc_end_time;
    unsigned int   suspend_time; // time when suspend
    int   sr_suspend_time;  // sr time when suspend
    int   sr_resume_time;   // sr time when resume
    int charging_begin_cc;  /*the unit is uah*/
    int charging_begin_time;
    int suspend_cc; // cc when suspend
    int resume_cc; // cc when suspend
    unsigned int last_time;
    int last_cc;
    int last_iavg_ma;
    int last_fifo_iavg_ma;
    int fcc_real_mah;
    int ntc_compensation_is;
    struct ntc_temp_compensation_para_data ntc_temp_compensation_para[COMPENSATION_PARA_LEVEL];
    struct mutex soc_mutex;
    struct hisi_coul_battery_data *batt_data;
    struct single_row_lut    *adjusted_fcc_temp_lut;
    struct single_row_lut    adjusted_fcc_temp_lut_tbl1;
    struct single_row_lut    adjusted_fcc_temp_lut_tbl2;
    struct delayed_work    calculate_soc_delayed_work;
    struct delayed_work    battery_check_delayed_work;
    struct delayed_work read_temperature_delayed_work;
    struct iscd_info *iscd; /* this information should not in coul device */
    struct work_struct      fault_work;
    struct notifier_block   fault_nb;
    struct notifier_block reboot_nb;
    struct notifier_block   pm_notify;
    struct hisi_coul_ops  *ops;
    struct ss_coul_nv_info nv_info;
    struct coul_device_ops *coul_dev_ops;
    enum coul_fault_type  coul_fault;
    struct device *dev;
    int is_nv_read;
    int is_nv_need_save;
    int last_soc_enable;
    int startup_delta_soc;
    int soc_at_term;
    unsigned int  enable_current_full;
    unsigned int basp_level;
    unsigned int basp_total_level;
    int soc_unlimited;
    int soc_monitor_flag;
    int soc_monitor_limit;
    unsigned char last_ocv_level;
    unsigned int low_vol_filter_cnt;
    struct polar_calc_info polar;
    struct bat_ocv_info eco_info;
    struct notifier_block bat_lpm3_ipc_block;
    int chg_done_max_avg_cur_flag;/*acr max currrent flag*/
    int high_pre_qmax;
    int qmax_start_pc;
    int qmax_cc;
    int qmax_refresh_flag;
    int qmax_end_pc;
#ifdef SMARTSTAR_DEBUG
    unsigned int dbg_ocv_cng_0; /*ocv change count by wake up*/
    unsigned int dbg_ocv_cng_1; /*ocv change count by full charged*/
    unsigned int dbg_valid_vol; /*vaild voltage from FIFO vol registers*/
    unsigned int dbg_invalid_vol; /*invaild voltage from FIFO vol registers*/
    unsigned int dbg_ocv_fc_failed; /*full charged can't update OCV*/
#endif
};
struct OCV_LEVEL_PARAM {
	int duty_ratio_limit;
	int sleep_current_limit;
	int sleep_time_limit;
	int wake_time_limit;
	int max_avg_curr_limit;
	int temp_limit;
	int delta_soc_limit;
	int ocv_gap_time_limit;
	bool ocv_update_anyway;
	bool allow_fcc_update;
	bool is_enabled;
};
struct AVG_CURR2UPDATE_OCV {
	int current_ma;
	int time;
};


extern struct device *coul_dev;
extern int v_offset_a;
extern int v_offset_b;
extern int c_offset_a;
extern int c_offset_b;
extern int coul_core_ops_register(struct coul_device_ops *ops);
extern int charger_dsm_report(int err_no, int *val);

#endif


