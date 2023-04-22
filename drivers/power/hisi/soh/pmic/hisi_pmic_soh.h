#ifndef _HISI_PMIC_SOH_H_
#define _HISI_PMIC_SOH_H_


#include <linux/mfd/hisi_pmic.h>
#include <pmic_interface.h>

#ifndef BIT
#define BIT(x)      (1 << (x))
#endif

/*reg read and write interface macro */
#define HISI_SOH_PMIC_REG_READ(regAddr)             hisi_pmic_reg_read(regAddr)
#define HISI_SOH_PMIC_REG_WRITE(regAddr,regval)     hisi_pmic_reg_write((int)(regAddr),(int)regval)
#define HISI_SOH_PMIC_REGS_READ(regAddr,buf,size)   hisi_pmic_array_read((int)(regAddr),(char*)(buf),(int)(size))
#define HISI_SOH_PMIC_REGS_WRITE(regAddr,buf,size)  hisi_pmic_array_write((int)(regAddr),(char*)(buf),(int)(size))
#define SOH_PMIC_INFO
#ifndef SOH_PMIC_INFO
/*pimc soh print interface*/
#define hisi_pmic_soh_err(fmt,args...)              do {} while (0)
#define hisi_pmic_soh_evt(fmt,args...)              do {} while (0)
#define hisi_pmic_soh_inf(fmt,args...)              do {} while (0)
#define hisi_pmic_soh_debug(fmt,args...)            /*do { printk(KERN_ERR    "[hisi_soh_pmic:debug]" fmt, ## args); } while (0)*/
#else
/*pimc soh print interface*/
#define hisi_pmic_soh_err(fmt,args...)              do { printk(KERN_ERR    "[hisi_soh_pmic]" fmt, ## args); } while (0)
#define hisi_pmic_soh_evt(fmt,args...)              do { printk(KERN_WARNING"[hisi_soh_pmic]" fmt, ## args); } while (0)
#define hisi_pmic_soh_inf(fmt,args...)              do { printk(KERN_INFO   "[hisi_soh_pmic]" fmt, ## args); } while (0)
#define hisi_pmic_soh_debug(fmt,args...)            /*do { printk(KERN_ERR    "[hisi_soh_pmic:debug]" fmt, ## args); } while (0)*/
#endif

/*pmic register write lock/unlock*/
#define SOH_WRITE_PRO	                            PMIC_DEBUG_WRITE_PRO_ADDR(0)
#define SOH_WRITE_LOCK		                        0x56
#define SOH_WRITE_UNLOCK	                        0xA9

/*acr macro definition start */
#define ACR_EN_ADDR                                 PMIC_ACR_EN_ADDR(0)
#define ACR_EN                                      0x5a
#define ACR_DIS                                     0xac

#define ACR_FLAG_EVENT_ADDR                         PMIC_ACR_SOH_RECORD_ADDR(0)//PMIC_ACR_SOH_IRQ_ADDR(0)
#define ACR_OCP_EVENT_ADDR                          PMIC_NP_SCP1_ADDR(0)
#define ACR_FLAG_EVENT_BIT                          BIT(PMIC_ACR_SOH_RECORD_np_acr_flag_START)
#define ACR_OCP_EVNET_BIT                           BIT(PMIC_NP_SCP1_np_acr_ocp_START)

#define ACR_FLAG_INT_ADDR                           PMIC_ACR_SOH_IRQ_ADDR(0)//PMIC_ACR_SOH_IRQ_ADDR(0)
#define ACR_OCP_INT_ADDR                            PMIC_SCP_IRQ1_ADDR(0)
#define ACR_FLAG_INT_BIT                            BIT(PMIC_ACR_SOH_IRQ_acr_flag_START)
#define ACR_OCP_INT_BIT                             BIT(PMIC_SCP_IRQ1_acr_ocp_START)

#define ACR_FLAG_INT_MASK_REG                       PMIC_IRQ_MASK_11_ADDR(0)
#define ACR_FLAG_INT_MASK_BIT                       BIT(0)
#define ACR_OCP_INT_MASK_REG                        PMIC_IRQ_MASK_9_ADDR(0)
#define ACR_OCP_INT_MASK_BIT                        BIT(4)

#define ACR_MUL_SEL_ADDR                            PMIC_ACR_CFG0_ADDR(0)
#define ACR_MUL_MASK                                0x3f
#define ACR_MUL_SHIFT                               (PMIC_ACR_CFG0_acr_mul_sel_START)

#define ACR_H_DATA_BASE                             PMIC_ACR_DATA0_H_ADDR(0)
#define ACR_L_DATA_BASE                             PMIC_ACR_DATA0_L_ADDR(0)
#define ACR_DATA_REG_NUM                            2

#define ACR_DATA_L_SHIFT                            4
#define ACR_DATA_H_SHIFT                            4
#define ACR_DATA_FIFO_DEPTH                         8

#define ACR_CHIP_TEMP_CFG_ADDR                      PMIC_ACR_RESERVE_CFG_ADDR(0)
#define ACR_CHIP_TEMP_EN_ADDR                       PMIC_ACRADC_CTRL_ADDR(0)
#define ACR_CHIP_TEMP_ADC_START_ADDR                PMIC_ACRADC_START_ADDR(0)
#define ACR_CHIP_TEMP_ADC_STATUS_ADDR               PMIC_ACRCONV_STATUS_ADDR(0)
#define ACR_CHIP_TEMP_ADC_DATA1_ADDR                PMIC_ACRADC_DATA1_ADDR(0)
#define ACR_CHIP_TEMP_ADC_DATA0_ADDR                PMIC_ACRADC_DATA0_ADDR(0)
#define ACR_CHIP_TEMP_CFG                           0x4
#define ACR_CHIP_TEMP_EN                            0x2
#define ACR_CHIP_TEMP_ADC_START                     0x1
#define ACR_CHIP_TEMP_CFG_DEFAULT                   0
#define ACR_CHIP_TEMP_EN_DEFAULT                    0x80
#define ACR_CHIP_TEMP_ADC_START_DEFAULT             0
#define ACR_CHIP_TEMP_ADC_READY                     1
#define ACR_CHIP_TEMP_L_SHIFT                       4
#define ACR_CHIP_TEMP_H_SHIFT                       4
#define INVALID_TEMP                                (-99)
#define ACR_GET_TEMP_RETRY                          100
#define ACR_ADC_WAIT_US                             15

#define ACR_CAL_MAGNIFICATION                       1000000
#define ACR_MOHM_TO_UOHM                            1000

/*acr macro definition end */

/*dcr macro definition start */

#define DCR_FIFO_LEN                                (10)

#define DCR_EN_REG                                  PMIC_DCR_EN_ADDR(0)
#define DCR_ENABLE                                  (0x5A)
#define DCR_DISABLE                                 (0xAC)

#define DCR_V_BASE_REG                              PMIC_DCR_V_PRE0_OUT0_ADDR(0)
#define DCR_I_BASE_REG                              PMIC_DCR_I_PRE0_OUT0_ADDR(0)
#define DCR_VI_REG_NUM                              3

#define DCR_IRQ_FLAG_INT_REG                        PMIC_ACR_SOH_IRQ_ADDR(0)
#define DCR_FLAG_MASK                               BIT(PMIC_ACR_SOH_IRQ_dcr_flag_START)

#define DCR_FLAG_EVENT_ADDR                         PMIC_ACR_SOH_RECORD_ADDR(0)//PMIC_ACR_SOH_IRQ_ADDR(0)
#define DCR_FLAG_EVENT_BIT                          BIT(PMIC_ACR_SOH_RECORD_np_dcr_flag_START)

#define DCR_FLAG_INT_MASK_REG                       PMIC_IRQ_MASK_11_ADDR(0)
#define DCR_FLAG_INT_MASK_BIT                       BIT(3)

#define DCR_TIMER_REG                               PMIC_DCR_TIMER_ADDR(0)
#define DCR_TIMER_MASK                              0x3

/*dcr macro definition end */

/*pd ocv macro definition start */

#define PD_OCV_FIFO_LEN                             (8)
#define PD_OCV_ONOFF_REG                            PMIC_CLJ_DEBUG1_ADDR(0)
#define PD_OCV_ONOFF_BITMASK                        BIT(PMIC_CLJ_DEBUG1_coul_ocv_onoff_START)

#define PD_OCV_V_OCV_BASE                           PMIC_V_OCV_PRE1_OUT0_ADDR(0)
#define PD_OCV_I_OCV_BASE                           PMIC_I_OCV_PRE1_OUT0_ADDR(0)
#define PD_OCV_OFTV_OCV_BASE                        PMIC_OFTV_OCV_PRE1_OUT0_ADDR(0)
#define PD_OCV_T_OCV_BASE                           PMIC_T_OCV_PRE1_OUT0_ADDR(0)
#define PD_OCV_RTC_OCV_BASE                         PMIC_RTC_OCV_PRE1_OUT0_ADDR(0)
#define PD_OCV_REG_NUM                              3
#define PD_OCV_RTC_REG_NUM                          4
#define PD_OCV_SHIFT                                8
#define PD_OCV_DEFAULT                              0xffffff
#define PD_OCV_OFFSET_DEFAULT                       0

/*pd ocv macro definition end */

/*ovp macro definition start */
#define SOH_OVP_EN_REG                             PMIC_SOH_OVP_EN_ADDR(0)
#define SOH_OVP_EN                                 BIT(PMIC_SOH_OVP_EN_sc_en_sohov_START)

#define SOH_OVH_CHECK_REG                          PMIC_SOH_OVH_MASK_ADDR(0)
#define SOH_OVH_CHECK_MASK                         BIT(PMIC_SOH_OVH_MASK_sc_soh_ovh_mask_START)

#define SOH_TIME_EN_REG                            PMIC_SOHOV_TIME_SOHEN_ADDR(0)
#define SOH_TIME_10S                               0
#define SOH_TIME_20S                               1
#define SOH_TIME_30S                               2
#define SOH_TIME_40S                               3

#define SOH_DISCHARGE_EN_REG                       PMIC_EN_SOHOV_ADDR(0)
#define SOH_DISCHARGER_EN                          0x5A
#define SOH_DISCHARGER_DIS                         0xAC

#define SOH_OVH_VOL_H_REG(x)                      (PMIC_SOH_OVH_TH0_H_ADDR(0) + 4*x)
#define SOH_OVH_VOL_L_REG(x)                      (PMIC_SOH_OVH_TH0_L_ADDR(0) + 4*x)

#define SOH_OVH_TEMP_H_REG(x)                     (PMIC_TMP_OVH_TH0_H_ADDR(0) + 4*x)
#define SOH_OVH_TEMP_L_REG(x)                     (PMIC_TMP_OVH_TH0_L_ADDR(0) + 4*x)

#define SOH_OVL_VOL_H_REG                          PMIC_SOH_OVL_TH_H_ADDR(0)
#define SOH_OVL_VOL_L_REG                          PMIC_SOH_OVL_TH_L_ADDR(0)
#define SOH_OVL_TEMP_H_REG                         PMIC_TMP_OVL_TH_H_ADDR(0)
#define SOH_OVL_TEMP_L_REG                         PMIC_TMP_OVL_TH_L_ADDR(0)

#define SOH_OVP_IRQ_MASK_REG                       PMIC_IRQ_MASK_11_ADDR(0)
#define SOH_OVH_IRQ_MASK                           BIT(2)
#define SOH_OVL_IRQ_MASK                           BIT(1)

#define OVP_MAX_VOL_MV                             4500
#define OVP_MIN_VOL_MV                             0
#define OVP_TEMP_SHIFT                             4
#define OVP_TEMP_H_ADDR_MASK                       0xff0
#define OVP_TEMP_L_ADDR_MASK                       0xf
#define OVP_VOL_H_ADDR_MASK                        0xff0000
#define OVP_VOL_H_ADDR_SHIFT                       16
#define OVP_VOL_L_ADDR_MASK                        0xf000
#define OVP_VOL_L_ADDR_SHIFT                       8

#define OVP_THRED_CNT                              3

#define SOH_OVP_IDIS_SEL                           PMIC_ACR_CFG0_ADDR(0)
#define OVP_IDIS_MASK                              0xf0
#define OVP_IDIS_SHIFT                             (PMIC_ACR_CFG0_ovp_idis_sel_START)
#define OVP_IDIS_50MA                              0
#define OVP_IDIS_200MA                             0xf
/*ovp macro definition end */


/*acr mul sel*/
enum acr_mul {
	ACR_MUL_35  = 0,
	ACR_MUL_70  = 1,
	ACR_MUL_140 = 2,
	ACR_MUL_280 = 3,
	ACR_MUL_MAX,
};

enum soh_thd_type {
    VOL_MV = 0,
    TEMP,
};

struct soh_pmic_device {
    int pmic_acr_support;
    int pmic_dcr_support;
    int pmic_pd_leak_support;
    int pmic_soh_ovp_support;
    int soh_ovh_irq;
    int soh_ovh_dis_irq;
    struct device *dev;
};

#endif


