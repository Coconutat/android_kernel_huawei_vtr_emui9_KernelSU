// ******************************************************************************
// Copyright     :  Copyright (C) 2017, Hisilicon Technologies Co., Ltd.
// File name     :  hisi_scharger_v600.h
// Version       :  1.0
// Date          :  2015-05-06
// Description   :  Define all registers/tables for HI6526

// ******************************************************************************

#ifndef __HI6526_CHARGER_H__
#define __HI6526_CHARGER_H__
#include <soc_schargerV600_interface.h>
#include <linux/i2c.h>      /*for struct device_info*/
#include <linux/device.h>   /*for struct device_info*/
#include <linux/workqueue.h>    /*for struct evice_info*/
#include <huawei_platform/usb/switch/switch_ap/switch_chip.h>
#include <huawei_platform/power/direct_charger.h>

#define HI6526_MODULE_NAME              hi6526
#define HI6526_MODULE_NAME_STR          "hi6526"
#define HI6526_I2C_SLAVE_ADDRESS        (0x6B)
#define CHG_ENABLE                      (1)
#define CHG_DISABLE                     (0)



#define HI6526_REG_MAX          (SOC_SCHARGER_OTG_RO_REG_9_ADDR(0)+1)

#define CHG_NONE_REG                    (0x00)
#define CHG_NONE_MSK                   (0xFF)
#define CHG_NONE_SHIFT                  (0x00)

#ifndef BIT
#define BIT(x)                          (1 << (x))
#endif
#ifndef MIN
#define MIN(a, b)						(((a) < (b)) ? (a) : (b))
#endif

struct reg_page {
        u8 page_addr;
        u8 page_val;
};


#define FCP_ACK_RETRY_CYCLE				(10)
#define CHG_VBATT_SOFT_OVP_CNT			(3)
#define CHG_VBATT_SOFT_OVP_MAX			(4600)
#define CHG_VBATT_CV_103(x)				((x)*103/100)
#define SCHARGER_ERR(fmt,args...) do { printk(KERN_ERR    "[hisi_scharger]" fmt, ## args); } while (0)
#define SCHARGER_EVT(fmt,args...) do { printk(KERN_WARNING"[hisi_scharger]" fmt, ## args); } while (0)
#define SCHARGER_INF(fmt,args...) do { printk(KERN_INFO   "[hisi_scharger]" fmt, ## args); } while (0)
#define SCHARGER_DBG(fmt,args...) do { printk(KERN_DEBUG   "[hisi_scharger]" fmt, ## args); } while (0)



#define FIRST_INSERT_TRUE           (1)
#define FCP_TRUE                    (1)
#define FCP_FALSE                   (0)

#define I2C_ERR_MAX_COUNT           (5)
#define RESET_VAL_ZERO              (0)

#define HI6526_FAIL -1
#define HI6526_RESTART_TIME 4
#define FCP_RETRY_TIME 5

#define FCP_ADAPTER_CNTL_REG			(SOC_SCHARGER_FCP_ADAP_CTRL_ADDR(0))
#define HI6526_ACCP_CHARGER_DET			(1 << SOC_SCHARGER_FCP_ADAP_CTRL_fcp_set_d60m_r_START)

#define HI6526_WATER_CHECKDPDN_NUM  (5)
#define HI6526_DPDM_WATER_THRESH_1460MV (1460)
#define HI6526_DPDM_WATER_THRESH_1490MV (1490)
#define HI6526_DPDM_WATER_THRESH_1560MV (1560)
#define HI6526_DPDM_WATER_THRESH_1590MV (1590)

/*************************struct define area***************************/
struct param {
    int bat_comp;
    int vclamp;
    int fcp_support;
    int scp_support;
    int r_coul_mohm;
    int dp_th_res;
};

enum chg_mode_state {
	NONE =0,
	BUCK,
	LVC,
	SC,
};

struct chip_debug_info {
        u64 ts_nsec;	  /* timestamp in nanoseconds */
        int vusb;
        int vbus;
        int ibus;
        int ibat;
        int vout;
        int vbat;
        int ibus_ref;
};

#define INFO_LEN        (10)
#define DBG_WORK_TIME  (40)

struct hi6526_device_info {
        struct i2c_client *client;
        struct device *dev;
        struct work_struct irq_work;
        struct delayed_work reverbst_work;
        struct delayed_work dc_ucp_work;
        struct param param_dts;
        enum hisi_charger_type charger_type;
        struct notifier_block   usb_nb;
        enum chg_mode_state chg_mode;
        struct mutex fcp_detect_lock;
        struct mutex i2c_lock;
        struct mutex adc_conv_lock;
        struct mutex accp_adapter_reg_lock;
        struct mutex ibias_calc_lock;
        struct nty_data dc_nty_data;
        struct delayed_work dbg_work;
        struct chip_debug_info dbg_info[INFO_LEN];
        unsigned int hi6526_version;
        unsigned int is_board_type;        /*0:sft 1:udp 2:asic */
        int buck_vbus_set;
        int input_current;
        int input_limit_flag;
        int gpio_cd;
        int gpio_int;
        int irq_int;
        int term_vol_mv;
        unsigned int adaptor_support;
        int ucp_work_first_run;
        int dc_ibus_ucp_happened;
        unsigned int dbg_index;
        unsigned long reverbst_begin;
        int delay_cunt;
        u8 reverbst_cnt;
        u8 batt_ovp_cnt_30s;
        u8 sysfs_fcp_reg_addr;
        u8 sysfs_fcp_reg_val;
        u8 dbg_work_stop;
        u8 abnormal_happened;
};

/******************************************************************************/
/*                      HI6526 Registers' Definitions                         */
/******************************************************************************/
#define CHIP_VERSION_0 (SOC_SCHARGER_VERSION0_RO_REG_0_ADDR(0))
#define CHIP_VERSION_2 (SOC_SCHARGER_VERSION2_RO_REG_0_ADDR(0))
#define CHIP_ID_6526  (0x36323536)

#define CHG_INPUT_SOURCE_REG            (SOC_SCHARGER_BUCK_CFG_REG_0_ADDR(0))
#define CHG_ILIMIT_SHIFT                (SOC_SCHARGER_BUCK_CFG_REG_0_da_buck_ilimit_START)
#define CHG_ILIMIT_MSK                  (0x1f<<CHG_ILIMIT_SHIFT)
#define CHG_ILIMIT_600MA                 (0x06)
#define CHG_IBUS_DIV                    (5)

#define CHG_ILIMIT_85                  (85)
#define CHG_ILIMIT_130                  (130)
#define CHG_ILIMIT_200                  (200)
#define CHG_ILIMIT_300                  (300)
#define CHG_ILIMIT_400                  (400)
#define CHG_ILIMIT_475                  (475)
#define CHG_ILIMIT_600                  (600)
#define CHG_ILIMIT_700                  (700)
#define CHG_ILIMIT_800                  (800)
#define CHG_ILIMIT_825                  (825)
#define CHG_ILIMIT_1000                 (1000)
#define CHG_ILIMIT_1100                 (1100)
#define CHG_ILIMIT_1200                 (1200)
#define CHG_ILIMIT_1300                 (1300)
#define CHG_ILIMIT_1400                 (1400)
#define CHG_ILIMIT_1500                 (1500)
#define CHG_ILIMIT_1600                 (1600)
#define CHG_ILIMIT_1700                 (1700)
#define CHG_ILIMIT_1800                 (1800)
#define CHG_ILIMIT_1900                 (1900)
#define CHG_ILIMIT_2000                 (2000)
#define CHG_ILIMIT_2100                 (2100)
#define CHG_ILIMIT_2200                 (2200)
#define CHG_ILIMIT_2300                 (2300)
#define CHG_ILIMIT_2400                 (2400)
#define CHG_ILIMIT_2500                 (2500)
#define CHG_ILIMIT_2600                 (2600)
#define CHG_ILIMIT_2700                 (2700)
#define CHG_ILIMIT_2800                 (2800)
#define CHG_ILIMIT_2900                 (2900)
#define CHG_ILIMIT_3000                 (3000)
#define CHG_ILIMIT_3100                 (3100)
#define CHG_ILIMIT_MAX                 (CHG_ILIMIT_3100)
#define CHG_ILIMIT_STEP_100                 (100)


/* CHG_REG1调节寄存器 */
#define CHG_FAST_CURRENT_REG            (SOC_SCHARGER_CHARGER_CFG_REG_2_ADDR(0))
#define CHG_FAST_ICHG_SHIFT             (SOC_SCHARGER_CHARGER_CFG_REG_2_da_chg_fast_ichg_START)
#define CHG_FAST_ICHG_MSK               (0x1f<<CHG_FAST_ICHG_SHIFT)
#define CHG_FAST_ICHG_STEP_100          (100)
#define CHG_FAST_ICHG_00MA              (0)


#define CHG_FAST_ICHG_100MA             (100)
#define CHG_FAST_ICHG_200MA             (200)
#define CHG_FAST_ICHG_300MA             (300)
#define CHG_FAST_ICHG_400MA             (400)
#define CHG_FAST_ICHG_500MA             (500)
#define CHG_FAST_ICHG_600MA             (600)
#define CHG_FAST_ICHG_700MA             (700)
#define CHG_FAST_ICHG_800MA             (800)
#define CHG_FAST_ICHG_900MA             (900)
#define CHG_FAST_ICHG_1000MA            (1000)
#define CHG_FAST_ICHG_1200MA            (1200)
#define CHG_FAST_ICHG_1400MA            (1400)
#define CHG_FAST_ICHG_1600MA            (1600)
#define CHG_FAST_ICHG_1800MA            (1800)
#define CHG_FAST_ICHG_2000MA            (2000)
#define CHG_FAST_ICHG_2200MA            (2200)
#define CHG_FAST_ICHG_2400MA            (2400)
#define CHG_FAST_ICHG_2500MA            (2400)
#define CHG_FAST_ICHG_2600MA            (2600)
#define CHG_FAST_ICHG_2800MA            (2800)
#define CHG_FAST_ICHG_3000MA            (3000)
#define CHG_FAST_ICHG_3200MA            (3200)
#define CHG_FAST_ICHG_MAX           (3200)

#define CHG_VBUS_VSET_REG               (SOC_SCHARGER_DET_TOP_CFG_REG_4_ADDR(0))
#define VBUS_VSET_SHIFT                 (SOC_SCHARGER_DET_TOP_CFG_REG_4_da_vbus_bkvset_START)
#define VBUS_VSET_MSK                   (0x03<<VBUS_VSET_SHIFT)
#define VBUS_VSET_5V                    (5)
#define VBUS_VSET_9V                    (9)
#define VBUS_VSET_12V                   (12)

#define CHG_DPM_MODE_REG				(SOC_SCHARGER_BUCK_CFG_REG_6_ADDR(0))
#define CHG_DPM_MODE_SHIFT               (SOC_SCHARGER_BUCK_CFG_REG_6_da_buck_dpm_auto_START)
#define CHG_DPM_MODE_MSK                 (1<<CHG_DPM_SEL_SHIFT)
#define CHG_DPM_MODE_AUTO                 (1<<CHG_DPM_SEL_SHIFT)

#define CHG_RECHG_REG    (SOC_SCHARGER_CHARGER_CFG_REG_9_ADDR(0))
#define CHG_RECHG_SHIFT         (SOC_SCHARGER_CHARGER_CFG_REG_9_da_chg_vrechg_hys_START)
#define CHG_RECHG_MSK           (0x03 << CHG_RECHG_SHIFT)
#define CHG_RECHG_150            (0x00)
#define CHG_RECHG_250            (0x01)
#define CHG_RECHG_350            (0x02)
#define CHG_RECHG_450            (0x03)

#define CHG_DPM_SEL_REG                 (SOC_SCHARGER_BUCK_CFG_REG_7_ADDR(0))
#define CHG_DPM_SEL_SHIFT               (SOC_SCHARGER_BUCK_CFG_REG_7_da_buck_dpm_sel_START)
#define CHG_DPM_SEL_MSK                 (0x0F<<CHG_DPM_SEL_SHIFT)
#define CHG_DPM_SEL_DEFAULT                 (0x06)

#define CHG_ENABLE_REG                  (SOC_SCHARGER_PULSE_CHG_CFG0_ADDR(0))
#define CHG_EN_SHIFT                    (SOC_SCHARGER_PULSE_CHG_CFG0_sc_chg_en_START)
#define CHG_EN_MSK                      (1<<CHG_EN_SHIFT)

#define BATFET_CTRL_CFG_REG             (SOC_SCHARGER_CHARGER_BATFET_CTRL_ADDR(0))

#define BATFET_CTRL_CFG_SHIFT           (SOC_SCHARGER_CHARGER_BATFET_CTRL_sc_batfet_ctrl_START)
#define BATFET_CTRL_CFG_MSK             (1<<BATFET_CTRL_CFG_SHIFT)
#define CHG_BATFET_DIS                  (0)
#define CHG_BATFET_EN                   (1)

#define CHG_PRE_ICHG_REG                (SOC_SCHARGER_CHARGER_CFG_REG_5_ADDR(0))
#define CHG_PRE_ICHG_SHIFT              (SOC_SCHARGER_CHARGER_CFG_REG_5_da_chg_pre_ichg_START)
#define CHG_PRE_ICHG_MSK                (0x03<<CHG_PRE_ICHG_SHIFT)
#define CHG_PRG_ICHG_STEP               (100)
#define CHG_PRG_ICHG_MIN                (100)
#define CHG_PRG_ICHG_100MA              (100)
#define CHG_PRG_ICHG_200MA              (200)
#define CHG_PRG_ICHG_300MA              (300)
#define CHG_PRG_ICHG_400MA              (400)
#define CHG_PRG_ICHG_MAX                (400)

#define CHG_PRE_VCHG_REG                (SOC_SCHARGER_CHARGER_CFG_REG_5_ADDR(0))
#define CHG_PRE_VCHG_SHIFT              (SOC_SCHARGER_CHARGER_CFG_REG_5_da_chg_pre_vchg_START)
#define CHG_PRE_VCHG_MSK                (0x03<<CHG_PRE_VCHG_SHIFT)
#define CHG_PRG_VCHG_2800               (2800)
#define CHG_PRG_VCHG_3000               (3000)
#define CHG_PRG_VCHG_3100               (3100)
#define CHG_PRG_VCHG_3200               (3200)

#define CHG_FASTCHG_TIMER_REG           (SOC_SCHARGER_CHARGER_CFG_REG_3_ADDR(0))
#define CHG_FASTCHG_TIMER_SHIFT         (SOC_SCHARGER_CHARGER_CFG_REG_3_da_chg_fastchg_timer_START)
#define CHG_FASTCHG_TIMER_MSK           (0x03<<CHG_FASTCHG_TIMER_SHIFT)
#define CHG_FASTCHG_TIMER_5H            (0)
#define CHG_FASTCHG_TIMER_8H            (1)
#define CHG_FASTCHG_TIMER_12H           (2)
#define CHG_FASTCHG_TIMER_20H           (3)

#define CHG_ACL_RPT_EN_REG              (SOC_SCHARGER_CHARGER_CFG_REG_0_ADDR(0))
#define CHG_ACL_RPT_EN_SHIFT            (SOC_SCHARGER_CHARGER_CFG_REG_0_da_chg_acl_rpt_en_START)
#define CHG_ACL_PRT_EN_MASK             (1 << CHG_ACL_RPT_EN_SHIFT)

#define CHG_EN_TERM_REG                 (SOC_SCHARGER_CHARGER_CFG_REG_1_ADDR(0))
#define CHG_EN_TERM_SHIFT               (SOC_SCHARGER_CHARGER_CFG_REG_1_da_chg_en_term_START)
#define CHG_EN_TERM_MSK                 (0x01<<CHG_EN_TERM_SHIFT)
#define CHG_TERM_EN                     (1)
#define CHG_TERM_DIS                    (0)

#define CHG_FAST_VCHG_REG               (SOC_SCHARGER_CHARGER_CFG_REG_3_ADDR(0))
#define CHG_FAST_VCHG_SHIFT             (SOC_SCHARGER_CHARGER_CFG_REG_3_da_chg_fast_vchg_START)
#define CHG_FAST_VCHG_MSK               (0x1f<<CHG_FAST_VCHG_SHIFT)
#define CHG_FAST_VCHG_STEP_16600UV           (16600)

#define CHG_FAST_VCHG_MIN               (4000)
#define CHG_FAST_VCHG_BASE_UV               (4000 * 1000)
#define CHG_FAST_VCHG_4250              (4250)
#define CHG_FAST_VCHG_4400              (4400)

#define CHG_FAST_VCHG_4500              (4500)
#define CHG_FAST_VCHG_MAX               (4500)

#define CHG_TERM_ICHG_REG               (SOC_SCHARGER_CHARGER_CFG_REG_8_ADDR(0))
#define CHG_TERM_ICHG_SHIFT             (SOC_SCHARGER_CHARGER_CFG_REG_8_da_chg_term_ichg_START)
#define CHG_TERM_ICHG_MSK               (0x03<<CHG_TERM_ICHG_SHIFT)
#define CHG_TERM_ICHG_150MA             (150)
#define CHG_TERM_ICHG_200MA             (200)
#define CHG_TERM_ICHG_300MA             (300)
#define CHG_TERM_ICHG_400MA             (400)

#define WATCHDOG_CTRL_REG               (SOC_SCHARGER_WDT_CTRL_ADDR(0))
#define WATCHDOG_TIMER_SHIFT            (SOC_SCHARGER_WDT_CTRL_sc_watchdog_timer_START)
#define WATCHDOG_TIMER_MSK              (0x07<<WATCHDOG_TIMER_SHIFT)
#define WATCHDOG_TIMER_01_S             (1)
#define WATCHDOG_TIMER_02_S             (2)
#define WATCHDOG_TIMER_10_S             (10)
#define WATCHDOG_TIMER_20_S             (20)
#define WATCHDOG_TIMER_40_S             (40)
#define WATCHDOG_TIMER_80_S             (80)

#define WATCHDOG_SOFT_RST_REG           (SOC_SCHARGER_WDT_SOFT_RST_ADDR(0))
#define WD_RST_N_SHIFT                  (SOC_SCHARGER_WDT_SOFT_RST_wd_rst_n_START)
#define WD_RST_N_MSK                    (1<<WD_RST_N_SHIFT)
#define WATCHDOG_TIMER_RST              (1<<WD_RST_N_SHIFT)

#define CHG_OTG_REG0                    (SOC_SCHARGER_OTG_CFG_REG_1_ADDR(0))
#define CHG_OTG_LIM_SHIFT               (SOC_SCHARGER_OTG_CFG_REG_1_da_otg_lim_set_START)
#define CHG_OTG_LIM_MSK                 (0x03<<CHG_OTG_LIM_SHIFT)
#define BOOST_LIM_MIN                   (500)
#define BOOST_LIM_500                   (500)
#define BOOST_LIM_1000                  (1000)
#define BOOST_LIM_1500                  (1500)
#define BOOST_LIM_2000                  (2000)
#define BOOST_LIM_MAX                   (2000)

#define CHG_OTG_CFG_REG                    (SOC_SCHARGER_OTG_CFG_ADDR(0))
#define CHG_OTG_EN_SHIFT                (SOC_SCHARGER_OTG_CFG_sc_otg_en_START)
#define CHG_OTG_EN_MSK                  (0x1<<CHG_OTG_EN_SHIFT)
#define OTG_ENABLE                      (1)
#define OTG_DISABLE                     (0)

#define CHG_OTG_CFG_REG_0              (SOC_SCHARGER_OTG_CFG_REG_0_ADDR(0))
#define CHG_OTG_MODE_SHIFT                (SOC_SCHARGER_OTG_CFG_REG_0_da_com_otg_mode_START)
#define CHG_OTG_MODE_MSK                  (0x1<<CHG_OTG_MODE_SHIFT)

#define CHG_BUCK_STATUS_REG         (SOC_SCHARGER_BUCK_RO_REG_26_ADDR(0))
#define HI6526_CHG_BUCK_OK              (0x01<<SOC_SCHARGER_BUCK_RO_REG_26_ad_buck_ok_START)

#define CHG_R0_REG_STATUE         (SOC_SCHARGER_CHARGER_RO_REG_10_ADDR(0))
#define CHG_IN_DPM_STATE                (0x01<<SOC_SCHARGER_CHARGER_RO_REG_10_ad_chg_dpm_state_START)
#define CHG_IN_ACL_STATE                (0x01<<SOC_SCHARGER_CHARGER_RO_REG_10_ad_chg_acl_state_START)

#define CHG_R0_REG2_STATUE         (SOC_SCHARGER_CHARGER_RO_REG_11_ADDR(0))
#define CHG_IN_THERM_STATE                (0x01<<SOC_SCHARGER_CHARGER_RO_REG_11_ad_chg_therm_state_START)

#define CHG_STATUS_REG                 (SOC_SCHARGER_IRQ_STATUS_8_ADDR(0))
#define HI6526_CHG_STAT_CHARGE_DONE     (0x03<<SOC_SCHARGER_IRQ_STATUS_8_tb_irq_status_8_START)
#define HI6526_CHG_STAT_SHIFT           (SOC_SCHARGER_IRQ_STATUS_8_tb_irq_status_8_START)
#define HI6526_CHG_STAT_MASK            (0x03<<HI6526_CHG_STAT_SHIFT)

#define HI6526_CHG_OTG_ON               (0x01<<SOC_SCHARGER_STATUS0_otg_on_START)

#define CHG_OTG_SWITCH_CFG_REG            SOC_SCHARGER_OTG_CFG_REG_6_ADDR(0)
#define CHG_OTG_SWITCH_SHIFT              (SOC_SCHARGER_OTG_CFG_REG_6_da_otg_switch_START)
#define CHG_OTG_SWITCH_MASK               (1 << CHG_OTG_SWITCH_SHIFT)



#define CHG_STAT_ENABLE                 (1)
#define CHG_STAT_DISABLE                (0)

#define WATCHDOG_STATUS_REG                 (SOC_SCHARGER_DIG_STATUS0_ADDR(0))
#define HI6526_WATCHDOG_OK              (0x01<<SOC_SCHARGER_DIG_STATUS0_wdt_time_out_n_START)

#define CHG_ADC_CTRL_REG                (SOC_SCHARGER_HKADC_CTRL0_ADDR(0))
#define CHG_ADC_EN_SHIFT                (SOC_SCHARGER_HKADC_CTRL0_sc_hkadc_en_START)
#define CHG_ADC_EN_MSK                  (1<<CHG_ADC_EN_SHIFT)
#define CHG_ADC_EN                      (1)
#define CHG_ADC_DIS                     (0)

#define CHG_ADC_CH_SEL_H               (SOC_SCHARGER_HKADC_SEQ_CH_H_ADDR(0))
#define CHG_ADC_CH_SEL_L               (SOC_SCHARGER_HKADC_SEQ_CH_L_ADDR(0))

#define CHG_ADC_CH_VUSB                 (0)
#define CHG_ADC_CH_IBUS                 (1)
#define CHG_ADC_CH_VBUS                 (2)
#define CHG_ADC_CH_VOUT                 (3)
#define CHG_ADC_CH_VBAT                 (4)
#define CHG_ADC_CH_IBAT                 (5)
#define CHG_ADC_CH_VACR                 (6)
#define CHG_ADC_CH_L_MAX                (8)
#define CHG_ADC_CH_IBUS_REF             (0 + CHG_ADC_CH_L_MAX)
#define CHG_ADC_CH_DPDM                  (2 + CHG_ADC_CH_L_MAX)
#define CHG_ADC_CH_TSBAT                (3 + CHG_ADC_CH_L_MAX)
#define CHG_ADC_CH_TSBUS                (4 + CHG_ADC_CH_L_MAX)
#define CHG_ADC_CH_TSCHIP               (5 + CHG_ADC_CH_L_MAX)

#define IBUS_INVALID_VAL                (5333)


#define CHG_ADC_CTRL               (SOC_SCHARGER_HKADC_CTRL0_ADDR(0))
#define CHG_ADC_CONV_SHIFT             (SOC_SCHARGER_HKADC_CTRL0_sc_hkadc_seq_conv_START)
#define CHG_ADC_CONV_MSK               (1<<CHG_ADC_START_SHIFT)

#define CHG_ADC_CTRL1               (SOC_SCHARGER_HKADC_CTRL1_ADDR(0))
#define CHG_ADC_CTRL1_DEFAULT_VAL               (0x67)

#define CHG_ADC_RD_SEQ             (SOC_SCHARGER_HKADC_RD_SEQ_ADDR(0))



#define CHG_ADC_START_REG               (SOC_SCHARGER_HKADC_START_ADDR(0))
#define CHG_ADC_START_SHIFT             (SOC_SCHARGER_HKADC_START_sc_hkadc_start_cfg_START)
#define CHG_ADC_START_MSK               (1<<CHG_ADC_START_SHIFT)

#define CHG_ADC_CONV_STATUS_REG         (SOC_SCHARGER_PULSE_NON_CHG_FLAG_ADDR(0))
#define CHG_ADC_CONV_STATUS_SHIFT       (SOC_SCHARGER_PULSE_NON_CHG_FLAG_hkadc_data_valid_START)
#define CHG_ADC_CONV_STATUS_MSK         (1<<CHG_ADC_CONV_STATUS_SHIFT)

#define CHG_PULSE_NO_CHG_FLAG_SHIFT      (SOC_SCHARGER_PULSE_NON_CHG_FLAG_pulse_non_chg_flag_START)
#define CHG_PULSE_NO_CHG_FLAG_MSK         (1<<CHG_PULSE_NO_CHG_FLAG_SHIFT)


#define CHG_ADC_APPDET_REG				(SOC_SCHARGER_DET_TOP_CFG_REG_0_ADDR(0))
#define CHG_ADC_APPDET_EN				(1)
#define CHG_ADC_APPDET_DIS				(0)

#define DP_RES_IQSEL_REG				(SOC_SCHARGER_DET_TOP_CFG_REG_0_ADDR(0))
#define DP_RES_IQSEL_SHIFT				(SOC_SCHARGER_DET_TOP_CFG_REG_0_da_dp_res_det_iqsel_START)
#define DP_RES_IQSEL_MASK				(0x03 << DP_RES_IQSEL_SHIFT)

#define APPLE_DETECT_ENABLE             (1)
#define APPLE_DETECT_DISABLE            (0)
#define APPLE_DETECT_SHIFT             (SOC_SCHARGER_DET_TOP_CFG_REG_0_da_app_det_en_START)
#define APPLE_DETECT_MASK             (0x01<<APPLE_DETECT_SHIFT)

#define CHG_ADC_APPDET_CHSEL_SHIFT		(SOC_SCHARGER_DET_TOP_CFG_REG_0_da_app_det_chsel_START)
#define CHG_ADC_APPDET_CHSEL_MSK		(1<<CHG_ADC_APPDET_CHSEL_SHIFT)
#define CHG_ADC_APPDET_DPLUS			(1)
#define CHG_ADC_APPDET_DMINUS		(0)

#define CHG_ADC_DP_RES_DET_SHIFT		(SOC_SCHARGER_DET_TOP_CFG_REG_0_da_dp_res_det_en_START)
#define CHG_ADC_DP_RES_DET_MSK		(1<<CHG_ADC_DP_RES_DET_SHIFT)
#define DP_RES_DET_EN                   (1)
#define DP_RES_DET_DIS                  (0)

#define CHG_ADC_DATA_L_REG                (SOC_SCHARGER_VUSB_ADC_L_ADDR(0))
#define CHG_ADC_DATA_H_REG                (SOC_SCHARGER_VUSB_ADC_H_ADDR(0))

#define CHG_HIZ_CTRL_REG                (SOC_SCHARGER_SC_BUCK_ENB_ADDR(0))
#define CHG_HIZ_ENABLE_SHIFT            (SOC_SCHARGER_SC_BUCK_ENB_sc_buck_enb_START)
#define CHG_HIZ_ENABLE_MSK              (1<<CHG_HIZ_ENABLE_SHIFT)

#define CHG_IRQ_STATUS0                        (SOC_SCHARGER_IRQ_STATUS_0_ADDR(0))
#define CHG_VBAT_OVP                    (BIT(4))


#define CHG_IRQ_ADDR                      (SOC_SCHARGER_IRQ_FLAG_ADDR(0))
#define CHG_FCP_IRQ                      (BIT(0))
#define CHG_OTHERS_IRQ               (BIT(1))
#define CHG_PD_IRQ                       (BIT(2))
#define CHG_LVC_SC_IRQ                (BIT(3))
#define CHG_BUCK_IRQ                   (BIT(4))

/* buck irq  */
#define CHG_BUCK_IRQ_ADDR         (SOC_SCHARGER_IRQ_FLAG_0_ADDR(0))
#define BUCK_IRQ_MASK                 (0xE0FFFF)
#define CHG_BUCK_IRQ_ADDR2         (SOC_SCHARGER_IRQ_FLAG_2_ADDR(0))
#define CHG_RBOOST_IRQ_SHIFT			(7)
#define CHG_RBOOST_IRQ_MSK				(1 << CHG_RBOOST_IRQ_SHIFT)

/* lcv/scc irq  */
#define CHG_LVC_SC_IRQ_ADDR         (SOC_SCHARGER_IRQ_FLAG_2_ADDR(0))
#define LVC_SC_IRQ_MASK                 (0xFFFF1F)
#define LVC_REGULATOR_IRQ_MASK                 (0x000F00)
#define IRQ_IBUS_DC_UCP_MASK            (0x000001)

/* pd irq  */
#define CHG_PD_IRQ_ADDR              (SOC_SCHARGER_IRQ_FLAG_5_ADDR(0))
#define PD_IRQ_MASK                       (0xE0)

/* others irq  */
#define CHG_OTHERS_IRQ_ADDR              (SOC_SCHARGER_IRQ_FLAG_5_ADDR(0))
#define OTHERS_IRQ_MASK                      (0xFFFF1F)
#define OTHERS_SOH_IRQ_MASK                      (0x00000C)

#define OTHERS_VDROP_IRQ_MASK                      (0x300000)
#define OTHERS_OTP_IRQ_MASK                      (0x000200)

#define IRQ_VDROP_MIN_MASK                      (0x200000)
#define IRQ_VDROP_OVP_MASK                      (0x100000)

/* fcp irq  */
#define CHG_FCP_IRQ_ADDR1              (SOC_SCHARGER_ISR1_ADDR(0))
#define OTHERS_IRQ_MASK1                      (0x1FFE)
#define CHG_FCP_IRQ_ADDR2              (SOC_SCHARGER_FCP_IRQ3_ADDR(0))
#define OTHERS_IRQ_MASK2                      (0xFFFF)
#define CHG_FCP_IRQ_ADDR3              (SOC_SCHARGER_FCP_IRQ5_ADDR(0))
#define OTHERS_IRQ_MASK3                      (0x03)

#define CHG_FCP_IRQ5_MASK_ADDR			(SOC_SCHARGER_FCP_IRQ5_MASK_ADDR(0))
#define CHG_IRQ_MASK_ALL_ADDR             (SOC_SCHARGER_IRQ_MASK_ADDR(0))
#define CHG_IRQ_MASK_ALL_SHIFT			(0)
#define CHG_IRQ_MASK_ALL_MSK				(0x1F << CHG_IRQ_MASK_ALL_SHIFT)

#define CHG_IRQ_UNMASK_DEFAULT				(0x05)

#define CHG_IRQ_MASK_0                     (SOC_SCHARGER_IRQ_MASK_0_ADDR(0))
#define CHG_IRQ_MASK_1                     (SOC_SCHARGER_IRQ_MASK_1_ADDR(0))
#define CHG_IRQ_MASK_2                     (SOC_SCHARGER_IRQ_MASK_2_ADDR(0))
#define CHG_IRQ_MASK_3                     (SOC_SCHARGER_IRQ_MASK_3_ADDR(0))
#define CHG_IRQ_MASK_4                     (SOC_SCHARGER_IRQ_MASK_4_ADDR(0))
#define CHG_IRQ_MASK_5                     (SOC_SCHARGER_IRQ_MASK_5_ADDR(0))
#define CHG_IRQ_MASK_6                     (SOC_SCHARGER_IRQ_MASK_6_ADDR(0))
#define CHG_IRQ_MASK_7                     (SOC_SCHARGER_IRQ_MASK_7_ADDR(0))

#define irq_sleep_mod_mask              BIT(0)

#define CHG_IRQ_MASK_0_VAL                     (0x00)
#define CHG_IRQ_MASK_1_VAL                     (irq_sleep_mod_mask)
#define CHG_IRQ_MASK_2_VAL                     (0x00)
#define CHG_IRQ_MASK_3_VAL                     (0x00)
#define CHG_IRQ_MASK_4_VAL                     (0x00)
#define CHG_IRQ_MASK_5_VAL                     (0x00)
#define CHG_IRQ_MASK_6_VAL                     (0x00)
#define CHG_IRQ_MASK_7_VAL                     (0x00)

#define CHG_IRQ_VBUS_UVP_SHIFT			(5)
#define CHG_IRQ_VBUS_UVP_MSK				(0x1 << CHG_IRQ_VBUS_UVP_SHIFT)
#define IRQ_VBUS_UVP_MASK       (0x01)
#define IRQ_VBUS_UVP_UNMASK       (0x00)

/* chg_reg5电池通道补偿寄存器 */
#define CHG_IR_COMP_REG                 (SOC_SCHARGER_CHARGER_CFG_REG_4_ADDR(0))
#define CHG_IR_COMP_SHIFT               (SOC_SCHARGER_CHARGER_CFG_REG_4_da_chg_ir_set_START)
#define CHG_IR_COMP_MSK                 (0x07<<CHG_IR_COMP_SHIFT)
#define CHG_IR_COMP_MIN                 (0)
#define CHG_IR_COMP_MAX                 (105)
#define CHG_IR_COMP_STEP_15                 (15)



#define CHG_IR_VCLAMP_REG               (SOC_SCHARGER_CHARGER_CFG_REG_8_ADDR(0))
#define CHG_IR_VCLAMP_SHIFT             (SOC_SCHARGER_CHARGER_CFG_REG_8_da_chg_vclamp_set_START)
#define CHG_IR_VCLAMP_MSK               (0x07<<CHG_IR_VCLAMP_SHIFT)
#define CHG_IR_VCLAMP_STEP              (50)
#define CHG_IR_VCLAMP_MIN               (0)
#define CHG_IR_VCLAMP_MAX               (350)

#define CHG_OVP_VOLTAGE_REG         (SOC_SCHARGER_DET_TOP_CFG_REG_5_ADDR(0))
#define CHG_BUCK_OVP_VOLTAGE_SHIFT       (SOC_SCHARGER_DET_TOP_CFG_REG_5_da_vbus_ov_bk_START)
#define CHG_BUCK_OVP_VOLTAGE_MSK         (0x03 << CHG_BUCK_OVP_VOLTAGE_SHIFT)

#define CHG_UVP_VOLTAGE_REG         (SOC_SCHARGER_DET_TOP_CFG_REG_6_ADDR(0))
#define CHG_BUCK_UVP_VOLTAGE_SHIFT       (SOC_SCHARGER_DET_TOP_CFG_REG_6_da_vbus_uv_bk_START)
#define CHG_BUCK_UVP_VOLTAGE_MSK         (0x03 << CHG_BUCK_UVP_VOLTAGE_SHIFT)

/*fcp detect */
#define CHG_FCP_ADAPTER_DETECT_FAIL     (1)
#define CHG_FCP_ADAPTER_DETECT_SUCC     (0)
#define CHG_FCP_ADAPTER_DETECT_OTHER    (-1)
#define CHG_FCP_POLL_TIME               (100)   /* 100ms */
#define CHG_FCP_DETECT_MAX_COUT         (20)  /* fcp detect MAX COUT */

/*fcp adapter status */
#define FCP_ADAPTER_5V                      (5000)
#define FCP_ADAPTER_9V                      (9000)
#define FCP_ADAPTER_12V                     (12000)
#define FCP_ADAPTER_MAX_VOL                 (FCP_ADAPTER_12V)
#define FCP_ADAPTER_MIN_VOL                 (FCP_ADAPTER_5V)
#define FCP_ADAPTER_RST_VOL                 (FCP_ADAPTER_5V)
#define FCP_ADAPTER_VOL_CHECK_ERROR         (500)
#define FCP_ADAPTER_VOL_CHECK_POLLTIME      (100)
#define FCP_ADAPTER_VOL_CHECK_TIMEOUT       (10)

#define CHG_FCP_CMD_REG                 (SOC_SCHARGER_CMD_ADDR(0))
#define CHG_FCP_ADDR_REG                (SOC_SCHARGER_ADDR_ADDR(0))
#define CHG_FCP_WDATA_REG              (SOC_SCHARGER_DATA0_ADDR(0))
#define CHG_FCP_RDATA_REG               (SOC_SCHARGER_FCP_RDATA0_ADDR(0))

#define CHG_FCP_LEN_REG               (SOC_SCHARGER_FCP_LENGTH_ADDR(0))
#define FCP_DATA_LEN               (8)

#define CHG_FCP_CTRL_REG                (SOC_SCHARGER_CNTL_ADDR(0))
#define CHG_FCP_EN_SHIFT                (SOC_SCHARGER_CNTL_enable_START)
#define CHG_FCP_EN_MSK                  (1<<CHG_FCP_EN_SHIFT)
#define CHG_FCP_MSTR_RST_SHIFT          (SOC_SCHARGER_CNTL_mstr_rst_START)
#define CHG_FCP_MSTR_RST_MSK            (1<<CHG_FCP_MSTR_RST_SHIFT)
#define CHG_FCP_SNDCMD_SHIFT            (SOC_SCHARGER_CNTL_sndcmd_START)
#define CHG_FCP_SNDCMD_MSK              (1<<CHG_FCP_SNDCMD_SHIFT)
#define CHG_FCP_EN                      (1)
#define CHG_FCP_DIS                     (0)

#define CHG_FCP_DET_CTRL_REG            (SOC_SCHARGER_FCP_CTRL_ADDR(0))
#define CHG_FCP_DET_EN_SHIFT            (SOC_SCHARGER_FCP_CTRL_fcp_det_en_START)
#define CHG_FCP_DET_EN_MSK              (1<<CHG_FCP_DET_EN_SHIFT)
#define CHG_FCP_CMP_EN_SHIFT            (SOC_SCHARGER_FCP_CTRL_fcp_cmp_en_START)
#define CHG_FCP_CMP_EN_MSK              (1<<CHG_FCP_CMP_EN_SHIFT)
#define CHG_FCP_DET_EN                  (1)

#define CHG_FCP_SOFT_RST_REG            (SOC_SCHARGER_FCP_SOFT_RST_CTRL_ADDR(0))
#define CHG_FCP_SOFT_RST_VAL            (0xAC)
#define CHG_FCP_SOFT_RST_DEFULT            (0x5A)

#define CHG_FCP_STATUS_REG              (SOC_SCHARGER_STATUIS_ADDR(0))
#define CHG_FCP_DVC_SHIFT               (SOC_SCHARGER_STATUIS_dvc_START)
#define CHG_FCP_DVC_MSK                 (0x03<<CHG_FCP_DVC_SHIFT)
#define CHG_FCP_ATTATCH_SHIFT           (SOC_SCHARGER_STATUIS_attach_START)
#define CHG_FCP_ATTATCH_MSK             (1<<CHG_FCP_ATTATCH_SHIFT)
#define CHG_FCP_SLAVE_GOOD              (CHG_FCP_DVC_MSK | CHG_FCP_ATTATCH_MSK)

#define CHG_FCP_SET_STATUS_REG          (SOC_SCHARGER_FCP_ADAP_CTRL_ADDR(0))
#define CHG_FCP_SET_STATUS_SHIFT        (SOC_SCHARGER_FCP_ADAP_CTRL_fcp_set_d60m_r_START)
#define CHG_FCP_SET_STATUS_MSK          (1<<CHG_FCP_SET_STATUS_SHIFT)

#define CHG_FCP_ISR1_REG                (SOC_SCHARGER_ISR1_ADDR(0))
#define CHG_FCP_CMDCPL                  (1<<SOC_SCHARGER_ISR1_cmdcpl_START)
#define CHG_FCP_ACK                     (1<<SOC_SCHARGER_ISR1_ack_START)
#define CHG_FCP_NACK                    (1<<SOC_SCHARGER_ISR1_nack_START)
#define CHG_FCP_CRCPAR                  (1<<SOC_SCHARGER_ISR1_crcpar_START)

#define CHG_FCP_ISR2_REG                (SOC_SCHARGER_ISR2_ADDR(0))
#define CHG_FCP_CRCRX                   (1<<SOC_SCHARGER_ISR2_crcrx_START)
#define CHG_FCP_PARRX                   (1<<SOC_SCHARGER_ISR2_parrx_START)
#define CHG_FCP_PROTSTAT                (1<<SOC_SCHARGER_ISR2_protstat_START)

#define CHG_FCP_IRQ3_REG                (SOC_SCHARGER_FCP_IRQ3_ADDR(0))
#define CHG_FCP_TAIL_HAND_FAIL          (1<<SOC_SCHARGER_FCP_IRQ3_tail_hand_fail_irq_START)
#define CHG_FCP_IRQ4_REG                (SOC_SCHARGER_FCP_IRQ4_ADDR(0))

/* fcp 中断寄存器5 */
#define CHG_FCP_IRQ5_REG                (SOC_SCHARGER_FCP_IRQ5_ADDR(0))
#define CHG_FCP_SET_DET_SHIFT           (SOC_SCHARGER_FCP_IRQ5_fcp_set_d60m_int_START)
#define CHG_FCP_SET_DET_MSK             (1<<CHG_FCP_SET_DET_SHIFT)

/* adapter cmd */
#define CHG_FCP_CMD_SBRRD                   (0x0c)
#define CHG_FCP_CMD_SBRWR                   (0x0b)
#define CHG_FCP_CMD_MBRRD                   (0x1c)
#define CHG_FCP_CMD_MBRWR                   (0x1b)

/* adapter registers */
#define CHG_FCP_SLAVE_ID_OUT0               (0x04)
#define CHG_FCP_SLAVE_DISCRETE_CAPABILITIES (0x21)
#define CHG_FCP_SLAVE_MAX_PWR               (0x22)
#define CHG_FCP_SLAVE_OUTPUT_CONTROL        (0x2b)
#define CHG_FCP_SLAVE_VOUT_CONFIG           (0x2c)
#define CHG_FCP_SLAVE_REG_DISCRETE_OUT_V(n) (0x30+(n))
/* Register FCP_SLAVE_OUTPUT_CONTROL (0x2b)*/
#define CHG_FCP_SLAVE_SET_VOUT              (1<<0)
#define CHG_FCP_VOL_STEP                    (10)
#define CHG_FCP_OUTPUT_VOL_5V               (5)
#define CHG_FCP_OUTPUT_VOL_9V               (9)
#define CHG_FCP_OUTPUT_VOL_12V              (12)

/* anti-reverbst */
#define CHG_ANTI_REVERBST_REG                 (SOC_SCHARGER_BUCK_CFG_REG_2_ADDR(0))
#define CHG_ANTI_REVERBST_EN_SHIFT            (7)
#define CHG_ANTI_REVERBST_EN_MSK              (1<<CHG_ANTI_REVERBST_EN_SHIFT)
#define CHG_ANTI_REVERBST_EN                  (1)
#define CHG_ANTI_REVERBST_DIS                 (0)

#define CHG_BUCK_MODE_CFG    SOC_SCHARGER_BUCK_MODE_CFG_ADDR(0)
#define CHG_BUCK_MODE_CFG_SHIFT    (SOC_SCHARGER_BUCK_MODE_CFG_buck_mode_cfg_START)
#define CHG_BUCK_MODE_CFG_MASK     (1 << CHG_BUCK_MODE_CFG_SHIFT)

#define REG_PAGE_SELECT   (SOC_SCHARGER_VDM_PAGE_SELECT_ADDR(0))

#define SOH_TH0_L_REG                   SOC_SCHARGER_SOH_OVH_TH0_L_ADDR(0)
#define SOH_OVP_REG                      SOC_SCHARGER_SOH_OVP_REAL_ADDR(0)
#define SOH_DISCHG_REG                 SOC_SCHARGER_SOH_DISCHG_EN_ADDR(0)

#define SOH_DISCHG_EN              (0x5A)
#define SOH_DISCHG_DIS              (0xAC)
#define SOH_OVH_TEMP_LV2       (0x0C)
#define SOH_SOH_EN_REG          SOC_SCHARGER_HKADC_CTRL2_ADDR(0)
#define SOH_SOH_EN_SHIFT          (SOC_SCHARGER_HKADC_CTRL2_sc_sohov_en_START)
#define SOH_SOH_EN_MASK          (1 << SOH_SOH_EN_SHIFT)

#define SOH_SOH_TEST_SHIFT          (SOC_SCHARGER_HKADC_CTRL2_sc_soh_test_mode_START)
#define SOH_SOH_TEST_MASK          (1 << SOC_SCHARGER_HKADC_CTRL2_sc_soh_test_mode_START)


#define ACR_CTRL_REG		(SOC_SCHARGER_ACR_CTRL_ADDR(0))
#define ACR_EN_SHIFT          (SOC_SCHARGER_ACR_CTRL_sc_acr_en_START)
#define ACR_EN_MASK          (1 << ACR_EN_SHIFT)

#define ACR_DATA_REG		(SOC_SCHARGER_ACR_DATA0_L_ADDR(0))

#define SC_CHG_EN_REG           (SOC_SCHARGER_PULSE_CHG_CFG1_ADDR(0))
#define SC_CHG_EN_SHIFT            (SOC_SCHARGER_PULSE_CHG_CFG1_sc_sc_en_START)
#define SC_CHG_EN_MASK                 (1 << SC_CHG_EN_SHIFT)

#define SC_CHG_MODE_REG           (SOC_SCHARGER_SC_MODE_CFG_ADDR(0))
#define SC_CHG_MODE_SHIFT            (SOC_SCHARGER_SC_MODE_CFG_sc_mode_cfg_START)
#define SC_CHG_MODE_MASK                 (1 << SC_CHG_MODE_SHIFT)

#define LVC_CHG_EN_REG           (SOC_SCHARGER_PULSE_CHG_CFG0_ADDR(0))
#define LVC_CHG_EN_SHIFT            (SOC_SCHARGER_PULSE_CHG_CFG0_sc_lvc_en_START)
#define LVC_CHG_EN_MASK                 (1 << LVC_CHG_EN_SHIFT)

#define LVC_CHG_MODE_REG           (SOC_SCHARGER_LVC_MODE_CFG_ADDR(0))
#define LVC_CHG_MODE_SHIFT            (SOC_SCHARGER_LVC_MODE_CFG_lvc_mode_cfg_START)
#define LVC_CHG_MODE_MASK                 (1 << LVC_CHG_MODE_SHIFT)

#define SC_CHG_MODE_FLAG_REG           (SOC_SCHARGER_SCHG_LOGIC_CFG_REG_2_ADDR(0))
#define SC_CHG_MODE_FLAG_SHIFT             (6)
#define SC_CHG_MODE_FLAG_MASK                 (1 << SC_CHG_MODE_FLAG_SHIFT)

#define LVC_CHG_MODE_FLAG_REG           (SOC_SCHARGER_SCHG_LOGIC_CFG_REG_2_ADDR(0))
#define LVC_CHG_MODE_FLAG_SHIFT            (7)
#define LVC_CHG_MODE_FLAG_MASK                 (1 << LVC_CHG_MODE_FLAG_SHIFT)

#define ADC_PMID_DPDM_SEL_REG    (SOC_SCHARGER_SCHG_LOGIC_CFG_REG_2_ADDR(0))
#define ADC_PMID_DPDM_SEL_SHIFT    (4)
#define ADC_PMID_DPDM_SEL_MASK                 (1 << ADC_PMID_DPDM_SEL_SHIFT)

#define IBAT_RES_SEL_REG        (SOC_SCHARGER_DC_TOP_CFG_REG_9_ADDR(0))
#define IBAT_RES_SEL_SHIFT      (SOC_SCHARGER_DC_TOP_CFG_REG_9_da_ibat_res_sel_START)
#define IBAT_RES_SEL_MASK       (1 << IBAT_RES_SEL_SHIFT)

#define DPDM_PMID_SEL_REG       (SOC_SCHARGER_SCHG_LOGIC_CFG_REG_2_ADDR(0))
#define DPDM_PMID_SEL_SHIFT       (4 )
#define DPDM_PMID_SEL_MASK       (1 << DPDM_PMID_SEL_SHIFT )

#define IQ_SEL_1UA           (1)
#define IQ_SEL_10UA           (10)
#define IQ_SEL_100UA           (100)

#define DP_RES_IQSEL_REG				(SOC_SCHARGER_DET_TOP_CFG_REG_0_ADDR(0))
#define DP_RES_IQSEL_SHIFT				(SOC_SCHARGER_DET_TOP_CFG_REG_0_da_dp_res_det_iqsel_START)
#define DP_RES_IQSEL_MASK				(0x03 << DP_RES_IQSEL_SHIFT)
#define DP_VOLT_200MV                                                           (200)

#define SEL_PMID    0x0
#define SEL_DPDM    0x1

#define DP_DET  (0)
#define DM_DET  (1)

#define EFUSE1          (0)
#define EFUSE2          (1)
#define EFUSE3          (2)

#define EFUSE_BYTE5             (5)
#define EFUSE_BYTE6             (6)
#define EFUSE_BYTE7             (7)

#define EFUSE_SEL_REG				(SOC_SCHARGER_EFUSE_SEL_ADDR(0))
#define EFUSE_SEL_SHIFT				(SOC_SCHARGER_EFUSE_SEL_sc_efuse_sel_START)
#define EFUSE_SEL_MASK				(0x03 << EFUSE_SEL_SHIFT)

#define EFUSE1_SEL_REG				(SOC_SCHARGER_EFUSE_TESTBUS_CFG_ADDR(0))
#define EFUSE1_SEL_SHIFT				(SOC_SCHARGER_EFUSE_TESTBUS_CFG_sc_efuse1_testbus_sel_START)
#define EFUSE1_SEL_MASK				(0x0f << EFUSE1_SEL_SHIFT)

#define EFUSE2_SEL_REG				(SOC_SCHARGER_EFUSE_TESTBUS_SEL_ADDR(0))
#define EFUSE2_SEL_SHIFT				(SOC_SCHARGER_EFUSE_TESTBUS_SEL_sc_efuse2_testbus_sel_START)
#define EFUSE2_SEL_MASK				(0x0f << EFUSE2_SEL_SHIFT)

#define EFUSE3_SEL_REG				(SOC_SCHARGER_EFUSE_TESTBUS_SEL_ADDR(0))
#define EFUSE3_SEL_SHIFT				(SOC_SCHARGER_EFUSE_TESTBUS_SEL_sc_efuse3_testbus_sel_START)
#define EFUSE3_SEL_MASK				(0x0f << EFUSE3_SEL_SHIFT)

#define EFUSE_EN_REG                                       (SOC_SCHARGER_EFUSE_TESTBUS_CFG_ADDR(0))
#define EFUSE_EN_SHIFT                                          (SOC_SCHARGER_EFUSE_TESTBUS_CFG_sc_efuse_testbus_en_START)
#define EFUSE_EN_MASK                                           (1 << EFUSE_EN_SHIFT)
#define EFUSE_EN                                                (1)
#define EFUSE_DIS                                               (0)

#define EFUSE1_RDATA_REG                                (SOC_SCHARGER_EFUSE1_TESTBUS_RDATA_ADDR(0))
#define EFUSE2_RDATA_REG                                (SOC_SCHARGER_EFUSE2_TESTBUS_RDATA_ADDR(0))
#define EFUSE3_RDATA_REG                                (SOC_SCHARGER_EFUSE3_TESTBUS_RDATA_ADDR(0))

#define SELF_CHECK_TH_0  (500)
#define SELF_CHECK_TH_1  (2000)
#define SELF_CHECK_TH_2  (600)

#define PAGE0_NUM (0x7F + 1)
#define PAGE1_NUM (0x75 + 1)
#define PAGE2_NUM (0x76 + 1)

#define REVERBST_DELAY_ON       (1000)
#define VBUS_2600_MV             (2600)

#define IBUS_OCP_START_VAL  (500)
#define IBUS_ABNORMAL_VAL  (200)
#define IBUS_ABNORMAL_CNT  (5)
#define IBUS_ABNORMAL_TIME  (20)

#define BUF_LEN         (27)
#define DELAY_TIMES     (3)

static char* lvc_sc_irq_str[] =
{
        /* IRQ_FLAG_2 bit4 - bit0 */
        "irq_ibus_dc_ucp",
        "irq_ibus_dc_ocp",
        "irq_vbus_lvc_uv",
        "irq_vbus_lvc_ov",
        "irq_vusb_lvc_ovp",
        "none",
        "none",
        "none",

        /* IRQ_FLAG_3 bit0 - bit7 */
        "irq_ibat_clamp",
        "irq_ibus_clamp",
        "irq_vout_ov_clamp",
        "irq_bat_ov_clamp",
        "irq_vusb_scauto_ovp",
        "irq_vbat_dc_ovp",
        "irq_vdrop_lvc_ov",
        "irq_ibus_dc_rcp",

        /* IRQ_FLAG_3 bit0 - bit7 */
        "irq_ilim_bus_sc_ocp",
        "irq_ilim_sc_ocp",
        "irq_ibat_dcucp_alm",
        "irq_ibat_dc_ocp",
        "irq_vbat_sc_uvp",
        "irq_vin2vout_sc",
        "irq_vbus_sc_uv",
        "irq_vbus_sc_ov",

};
/*
flag2 bit3/4  vbus ovp   LVC     DIRECT_CHARGE_FAULT_VBUS_OVP
flag2 bit2    vbus uv    LVC     none
flag2 bit1    ibus ocp   LVC/SC  DIRECT_CHARGE_FAULT_INPUT_OCP  / DIRECT_CHARGE_FAULT_IBUS_OCP
flag2 bit0    ibus ucp   close

flag3 bit7    ibus rcp   LVC/SC  DIRECT_CHARGE_FAULT_REVERSE_OCP /
flag3 bit6    vdrop ov   LVC/SC  DIRECT_CHARGE_FAULT_VDROP_OVP  /
flag3 bit5    vbat ovp   LVC/SC  DIRECT_CHARGE_FAULT_VBAT_OVP  /
flag3 bit4    vusb ovp    SC     DIRECT_CHARGE_FAULT_VBUS_OVP

flag4 bit7    vbus ov    SC      DIRECT_CHARGE_FAULT_VBUS_OVP
flag4 bit6    vbus uv     SC
flag4 bit5    Vin-2Vout   SC     DIRECT_CHARGE_FAULT_VDROP_OVP
flag4 bit4    vbat uvp    SC
flag4 bit3    ibat ocp   LVC/SC   DIRECT_CHARGE_FAULT_IBAT_OCP /
flag4 bit2    ibat ucp   LVC/SC   DIRECT_CHARGE_FAULT_IBAT_OCP /
flag4 bit1    ibus ocp   SC       DIRECT_CHARGE_FAULT_IBUS_OCP    opt1
flag4 bit0    ibus ocp   SC       DIRECT_CHARGE_FAULT_IBUS_OCP
*/
#define FAULT_VBUS_OVP          (0x801018)
#define FAULT_IBUS_OCP          (0x030002)
#define FAULT_REVERSE_OCP       (0x008000)
#define FAULT_VDROP_OVP         (0x204000)
#define FAULT_VBAT_OVP          (0x002000)
#define FAULT_IBAT_OCP          (0x0C0000)

/* BUCK IRQ HANDLE*/
#define FAULT_BUCK_VBAT_OVP     (0x000010)
#define FAULT_OTG_OCP     (0x008000)
#define FAULT_REVERSBST     (0x800000)
#define FAULT_RECHG     (0x000200)
#define FAULT_CHG_FAULT     (0x000800)
#define FAULT_CHG_DONE     (0x000080)

#define REVERBST_RETRY          (10)

#define R_MOHM_2                (2)
#define R_MOHM_5                (5)
#define R_MOHM_DEFULT           R_MOHM_2      /* resisitance mohm */


struct opt_regs {
        u16 reg;
        u8 mask;
        u8 shift;
        u8 val;
        unsigned long before; // before delay ms
        unsigned long after; // after delay ms
};

#define REG_CFG(r, m, s, v, b,  a)  { .reg = (r), .mask = (m), .shift = (s), .val = (v), .before = (b), .after = (a),}

#endif
