#ifndef _HISI_SOH_CORE_H_
#define _HISI_SOH_CORE_H_

#include <linux/wakelock.h>
#include <linux/power/hisi/soh/hisi_soh_interface.h>

#ifndef MIN
#define MIN(x,y) ((x)>(y)?(y):(x))
#endif

#ifndef STATIC
#define STATIC  static
#endif

#define SOH_CORE_INFO
#ifndef SOH_CORE_INFO
#define hisi_soh_info(fmt,args...) do {} while (0)
#define hisi_soh_warn(fmt,args...) do {} while (0)
#define hisi_soh_err(fmt,args...) do {} while (0)
#define hisi_soh_debug(fmt, args...)    /*do { printk(KERN_ERR     "[hisi_soh_core:debug]" fmt, ## args); } while (0)*/
#else
#define hisi_soh_err(fmt,args...) do { printk(KERN_ERR    "[hisi_soh_core]" fmt, ## args); } while (0)
#define hisi_soh_warn(fmt,args...) do { printk(KERN_WARNING"[hisi_soh_core]" fmt, ## args); } while (0)
#define hisi_soh_info(fmt,args...) do { printk(KERN_INFO   "[hisi_soh_core]" fmt, ## args); } while (0)
#define hisi_soh_debug(fmt, args...)    /*do { printk(KERN_ERR     "[hisi_soh_core:debug]" fmt, ## args); } while (0)*/
#endif

#define SOH_EN    1
#define SOH_DIS   0

/*acr macro start*/
#define CAL_CNT                    20    /*get 20 acr data  for calcuatle*/
#define ACR_CAL_CHIP_TEMP_MAX      70    /*35 chip Suggestions */
#define ACR_CAL_CHIP_TEMP_MIN      0     /*20 chip suggustinos, 5% Error*/
#define ACR_CAL_BAT_TEMP_MAX       55
#define ACR_CAL_BAT_TEMP_MIN       0
#define ACR_RETRY_NUM              100000

#define ACR_CAL_BAT_CUR_MAX        20    /*mA*/
#define ACR_CAL_BAT_CYCLE_INC      1     /*battery cycle  increment*/
#define ACR_CAL_BAT_SOC_MIN        95

#define ACR_RETRY_ACR_MS           5000  /*acr retry time*/
#define ACR_DATA_ERR_RETRY_CNT     5     /*acr data err number in CAL_CNT arrary*/
#define SOH_ACR_NV_DATA_NUM        3

#define ACR_DATA_INVALID_NUM       2

#define ACR_AVG_PERCENT            20 /*20%*/
/*acr macro end*/

/*dcr macro start*/
#define DCR_CAL_BAT_TEMP_MAX        55
#define DCR_CAL_BAT_TEMP_MIN        0
#define DCR_RETRY_NUM               2000
#define DCR_R0_WATI_MS              300 /*MS*/

#define DCR_CAL_BAT_CUR_INC_MAX     20  /*mA*/ /*max current increment*/
#define DCR_CAL_DISCHARGE_BASE_CUR  200
#define DCR_CAL_BAT_CUR_INC_MAX_BY_CC 20

#define DCR_CAL_BAT_CYCLE_INC       2  /*battery cycle  increment*/
#define DCR_CAL_BAT_SOC_MIN         95

#define DCR_CAL_BAT_VOL_INC_MAX     10  /*mV*/ /* max vol Difference in fifo*/

#define DCR_CAL_BAT_TEMP_DIFF       5   /*max temp Difference*/

#define DCR_FIFO_MAX                10
#define SOH_DCR_NV_DATA_NUM         3

#define DCR_CURRENT_EXCD_MAX_CYCLE  3

#define DCR_CHECK_FLAG_CYCLE_TIMER  255/*ms*/

#define UAH_PER_MAH                 1000
#define MOHM_PER_OHM                1000

/*dcr macro end*/


/*pd macro start*/
#define PD_FIFO_MAX                16
#define PD_OCV_MAX                 4500 /*mv*/
#define PD_OCV_MIN                 2000
#define SEC_PER_H                  3600
#define SOH_PD_NV_DATA_NUM         2
#define PD_WORK_DELAY_MS           30000
#define PD_OCV_CAL_MIN_NUM         2
#define UA_PER_MA                  1000
#define UV_PER_MV                  1000
/*pd macro end*/

/*soh ovp macro*/
#define SOH_OVH_THRED_NUM          3
#define SOH_OVL_THRED_NUM          3
#define OVP_START_MIN_MINUTES      30
#define SEC_PER_MIN                60
#define OVP_WORK_DELAY_MS          30000
#define OVP_DISCHARGE_MIN_MS       5000
/*soh ovp macro*/

/*soh nv macro start ,same to fastboot*/
#define SOH_ACR_NV_NAME            "SOHACR"
#define SOH_ACR_NV_NUM             263
#define SOH_DCR_NV_NAME            "SOHDCR"
#define SOH_DCR_NV_NUM             264
#define SOH_PD_LEAK_NV_NAME        "SOHPDL"
#define SOH_PD_LEAK_NV_NUM         265
/*soh nv macro end*/

/*soh get nv info from reserved mem*/
#define SOH_NV_SIZE                        (128)
#define RESERVED_MEM_FOR_PMU_COUL_ADDR_128 (SOH_NV_SIZE)  /*Dependent on cou reserved memory*/
#define RESERVED_MME_FOR_PMU_ACR_ADD_128   (RESERVED_MEM_FOR_PMU_COUL_ADDR_128 + SOH_NV_SIZE)
#define RESERVED_MME_FOR_PMU_DCR_ADD_128   (RESERVED_MME_FOR_PMU_ACR_ADD_128 + SOH_NV_SIZE)
/*soh reserved mem macro end*/

/**
 * pd ocv data from chip fifo
 */
struct pd_leak_chip_info {
    int ocv_vol_uv;
    int ocv_cur_ua;
    int ocv_chip_temp;
    unsigned int ocv_rtc;
};


struct soh_ovp_temp_vol_threshold {
    int bat_vol_mv;
    int temp;
};

enum soh_ovp_type {
    SOH_OVH = 0,
    SOH_OVL = 1
};

enum dcr_timer_choose {
	DCR_TIMER_32  = 0,
	DCR_TIMER_64  = 1,
	DCR_TIMER_128 = 2,
	DCR_TIMER_256 = 3,
	DCR_TIMER_MAX,
};

/**
 * struct soh_acr_device_ops - interface to the acr driver ops
 * @enable_acr:	      enable or disable acr check.
 * @get_acr_flag:     get acr finish flag.
 * @get_acr_ocp:      get acr ocp flag.
 * @calculate_acr:    get acr data.
 * @get_acr_chip_temp: get chip temp in acr cal.
 */

struct soh_acr_device_ops {
    void         (*enable_acr)(int en);
    unsigned int (*get_acr_flag)(void);
    unsigned int (*get_acr_ocp)(void);
    void         (*clear_acr_flag)(void);
    void         (*clear_acr_ocp)(void);
    int          (*calculate_acr)(void);
    int          (*get_acr_chip_temp)(void);
    unsigned int (*get_acr_fault_status)(void);
    void         (*clear_acr_fault_status)(void);
    void         (*io_ctrl_acr_chip_en)(int en);
};

struct soh_dcr_device_ops {
    void         (*enable_dcr)(int en);
    void         (*clear_dcr_flag)(void);
    int          (*get_dcr_info)(int *dcr_current, int *dcr_vol, int num);
    void         (*set_dcr_timer)(enum dcr_timer_choose  dcr_timer);
    unsigned int (*get_dcr_flag)(void);
    unsigned int (*get_dcr_fifo_depth)(void);
    unsigned int (*get_dcr_timer)(void);
};


struct soh_pd_leak_device_ops {
    void       (*enable_pd_leak)(int en);
    unsigned   (*get_pd_leak_fifo_depth)(void);
    void       (*get_pd_leak_info)(struct pd_leak_chip_info *pd_ocv, unsigned int index);
};


struct soh_ovp_device_ops {
    int    (*set_ovp_threshold)(enum soh_ovp_type type, struct soh_ovp_temp_vol_threshold *ovp, int cnt);
    int    (*get_ovh_thred_cnt)(void);
    void   (*enable_ovp)(int en);
    void   (*enable_dischg)(int en);
    int    (*get_stop_dischg_state)(void);
    /*int    (*clear_interrupt)(enum soh_ovp_type type);*/
};

/**
 * struct acr_nv_info - acr info in nv item.
 * @order_num       : newest writted nv number.
 * @acr_contrl      : control acr function open or close by nv.
 * @soh_nv_acr_info : saved acr info
 */
struct acr_nv_info {
    int order_num;
    int acr_control;/*0:enable 1:close*/
    struct acr_info soh_nv_acr_info[SOH_ACR_NV_DATA_NUM];//24*3

};

struct dcr_nv_info {
    int order_num;
    int dcr_control;
    struct dcr_info soh_nv_dcr_info[SOH_DCR_NV_DATA_NUM];//16*3

};


struct pd_leak_nv_info {
    int order_num;
    int pd_control;
    struct pd_leak_current_info  soh_nv_pd_leak_current_info[SOH_PD_NV_DATA_NUM];//44*2
};



struct soh_acr_device {

    struct acr_info             soh_acr_info;
    struct delayed_work         acr_work;    /*calculate acr*/
    struct soh_acr_device_ops   *acr_ops;
    struct notifier_block       soh_acr_notify;
    struct acr_nv_info          acr_nv;
    int                         acr_support; /*dst support */
    int                         acr_work_flag;
    enum acr_type               acr_prec_type;
};

struct soh_dcr_device {

    struct dcr_info             soh_dcr_info;
    struct dcr_nv_info          dcr_nv;
    struct delayed_work         dcr_work;    /*calculate acr*/
    struct soh_dcr_device_ops   *dcr_ops;
    struct notifier_block       soh_dcr_notify;
    int                         dcr_support;
    int                         dcr_work_flag;
};

struct soh_pd_leak_device {
    struct pd_leak_current_info    soh_pd_leak_current_info;
    struct soh_pd_leak_device_ops  *pd_leak_ops;
    struct pd_leak_nv_info         pd_leak_nv;
    struct delayed_work            pd_leak_work;    /*calculate pd leak*/
    int                            pd_leak_support;
};

/**
 * struct soh_ovp_device - Basic representation of an soh ovp device
 * @soh_ovp_work       : Workqueue to be used for ovp irq handle.
 * @soh_ovp_notify     : notify by ovp driver.
 * @soh_ovh_thres      : ovp start discharging threshold.
 * @soh_ovl_thres      : ovp stop discharging threshold.
 * @soh_ovl_safe_thres : ovp stop discharging threshold for last Intercept.
 * @soh_ovp_start_time : Time interval between ovh interrupt received and start discharging.
 * @ovp_support        : ovp function enable by dts control.
 * @soh_ovp_type       : ovp type(ovh and ovl).
 */
struct soh_ovp_device {
    struct delayed_work                soh_ovp_work;
    struct soh_ovp_device_ops          *soh_ovp_ops;
    struct notifier_block              soh_ovp_notify;
    struct soh_ovp_temp_vol_threshold  soh_ovh_thres[SOH_OVH_THRED_NUM];
    struct soh_ovp_temp_vol_threshold  soh_ovl_thres[SOH_OVL_THRED_NUM];
    struct soh_ovp_temp_vol_threshold  soh_ovl_safe_thres;
    int                                soh_ovp_start_time;
    int                                ovp_support;
    enum soh_ovp_type                  ovp_type;
};


/**
 * struct hisi_soh_device - Basic representation of an soh device
 * @dev             : Driver model representation of soh device.
 * @soh_acr_device  : acr device struct.
 * @soh_dcr_device  : dcr device struct.
 * @soh_ovp_dev     : acr device struct.
 * @soh_pd_leak_dev : dcr device struct.
 * @soh_lock        : soh wake lock.
 */
struct hisi_soh_device {

    struct soh_acr_device       soh_acr_dev;
    struct soh_dcr_device       soh_dcr_dev;
    struct soh_pd_leak_device   soh_pd_leak_dev;
    struct soh_ovp_device       soh_ovp_dev;
    struct wake_lock            soh_wake_lock;
    struct mutex                soh_mutex;
    struct device               *dev;
};

enum soh_drv_ops_type {
	ACR_DRV_OPS     = 0,
	DCR_DRV_OPS     = 1,
	PD_LEAK_DRV_OPS = 2,
	SOH_OVP_DRV_OPS = 3,
};

enum soh_type {
	SOH_ACR      = 0,
	SOH_DCR      = 1,
	SOH_PD_LEAK  = 2,
	SOH_OVP      = 3,
	SOH_OVP_DIS  = 4
};

enum nv_rw_type {
    NV_READ,
    NV_WRITE
};

enum acr_cal_result {
    ACR_NORMAL   = 0,
    ACR_ERROR    = 1,
    ACR_FATAL
};

int soh_core_drv_ops_register(void *ops, enum soh_drv_ops_type ops_type);
extern struct device *soh_dev;

#endif
