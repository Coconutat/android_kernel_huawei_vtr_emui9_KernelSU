#ifndef _HISI_6531_SOH_H_
#define _HISI_6531_SOH_H_

#include <soc_hi6531v100_interface.h>

#ifndef BIT
#define BIT(x)      (1 << (x))
#endif

/*pimc soh print interface*/
#define hi6531_soh_err(fmt,args...)              do { printk(KERN_ERR    "[hi6531_soh]" fmt, ## args); } while (0)
#define hi6531_soh_evt(fmt,args...)              do { printk(KERN_WARNING"[hi6531_soh]" fmt, ## args); } while (0)
#define hi6531_soh_inf(fmt,args...)              do { printk(KERN_INFO   "[hi6531_soh]" fmt, ## args); } while (0)
#define hi6531_soh_debug(fmt,args...)            /*do { printk(KERN_ERR    "[hi6531_soh:debug]" fmt, ## args); } while (0)*/


/*acr macro definition start */

/*ACR VERSION*/
#define ACR_VERSION0_ADDR                           SOC_Hi6531V100_VERSION0_ADDR(0)
#define ACR_VERSION1_ADDR                           SOC_Hi6531V100_VERSION1_ADDR(0)
#define ACR_VERSION2_ADDR                           SOC_Hi6531V100_VERSION2_ADDR(0)
#define ACR_VERSION3_ADDR                           SOC_Hi6531V100_VERSION3_ADDR(0)
#define ACR_VERSION0_DATA                           0x36
#define ACR_VERSION1_DATA                           0x35
#define ACR_VERSION2_DATA                           0x33
#define ACR_VERSION3_DATA                           0x31


/*ACR EN*/
#define ACR_EN_ADDR                                 SOC_Hi6531V100_ACR_EN_ADDR(0)
#define ACR_EN                                      0x5a
#define ACR_DIS                                     0xac

#define ACR_LDO1_EN                                 1
#define ACR_LDO1_DIS                                0

#define ACR_LDO1_EN_REG                             SOC_Hi6531V100_LDO1_EN_ONOFF_ADDR(0)
#define ACR_LDO1_EN_BIT                             BIT(SOC_Hi6531V100_LDO1_EN_ONOFF_reg_ldo1_en_START)
#define ACR_LDO1_EN_ST                              BIT(SOC_Hi6531V100_LDO1_EN_ONOFF_st_ldo1_en_START)
#define LDO_LOCK_REG                                SOC_Hi6531V100_LDO_LOCK_ADDR(0)
#define LDO_WRITE_LOCK                              0xac
#define LDO_WRITE_UNLOCK                            0x7d


/*ACR INT*/
#define ACR_INT_ADDR                                SOC_Hi6531V100_IRQ_ADDR(0)
#define ACR_FLAG_INT_BIT                            BIT(SOC_Hi6531V100_IRQ_acr_flag_irq_START)
#define ACR_OCP_INT_BIT                             BIT(SOC_Hi6531V100_IRQ_acr_ocp_irq_START)
#define ACR_LDO1_OCP_INT_BIT                        BIT(SOC_Hi6531V100_IRQ_ldo1_ocp_irq_START)
#define ACR_OTMP_140_INT_BIT                        BIT(SOC_Hi6531V100_IRQ_thsd_otmp140_irq_START)
#define ACR_OTMP_125_INT_BIT                        BIT(SOC_Hi6531V100_IRQ_thsd_otmp125_irq_START)
#define ACR_UVP_ABS_INT_BIT                         BIT(SOC_Hi6531V100_IRQ_vbat_uvp_abs_irq_START)

/*ACR INT MASK*/
#define ACR_INT_MASK_REG                           SOC_Hi6531V100_IRQ_MASK_ADDR(0)
#define ACR_OCP_INT_MASK_BIT                       BIT(SOC_Hi6531V100_IRQ_MASK_acr_ocp_mk_START)
#define ACR_FLAG_INT_MASK_BIT                      BIT(SOC_Hi6531V100_IRQ_MASK_acr_flag_mk_START)
#define ACR_UVP_ABS_INT_MASK_BIT                   BIT(SOC_Hi6531V100_IRQ_MASK_vbat_uvp_abs_mk_START)
#define ACR_OTMP_125_INT_MASK_BIT                  BIT(SOC_Hi6531V100_IRQ_MASK_thsd_otmp_125_mk_START)
#define ACR_OTMP_140_INT_MASK_BIT                  BIT(SOC_Hi6531V100_IRQ_MASK_thsd_otmp_140_mk_START)
#define ACR_LDO1_OCP_INT_MASK_BIT                  BIT(SOC_Hi6531V100_IRQ_MASK_ldo1_ocp_mk_START)
#define ACR_MASK_ALL_BIT                           (ACR_OCP_INT_MASK_BIT | ACR_FLAG_INT_MASK_BIT | ACR_UVP_ABS_INT_MASK_BIT | ACR_OTMP_125_INT_MASK_BIT | ACR_OTMP_140_INT_MASK_BIT | ACR_LDO1_OCP_INT_MASK_BIT)

/*ACR MUL*/
#define ACR_MUL_SEL_ADDR                            SOC_Hi6531V100_ACR_CFG0_ADDR(0)
#define ACR_MUL_MASK                                0x3f
#define ACR_MUL_SHIFT                               (SOC_Hi6531V100_ACR_CFG0_acr_mul_sel_START)

#define ACR_H_DATA_BASE                             SOC_Hi6531V100_ACR_DATA0_H_ADDR(0)
#define ACR_L_DATA_BASE                             SOC_Hi6531V100_ACR_DATA0_L_ADDR(0)
#define ACR_DATA_REG_NUM                            2

#define ACR_DATA_H_SHIFT                            8
#define ACR_DATA_FIFO_DEPTH                         8


#define ACR_CHIP_TEMP_CFG_ADDR                      SOC_Hi6531V100_ACR_RESERVE_CFG_ADDR(0)
#define ACR_CHIP_TEMP_EN_ADDR                       SOC_Hi6531V100_ACRADC_CTRL_ADDR(0)
#define ACR_CHIP_TEMP_ADC_START_ADDR                SOC_Hi6531V100_ACRADC_START_ADDR(0)
#define ACR_CHIP_TEMP_ADC_STATUS_ADDR               SOC_Hi6531V100_ACRCONV_STATUS_ADDR(0)
#define ACR_CHIP_TEMP_ADC_DATA1_ADDR                SOC_Hi6531V100_ACRADC_DATA1_ADDR(0)
#define ACR_CHIP_TEMP_ADC_DATA0_ADDR                SOC_Hi6531V100_ACRADC_DATA0_ADDR(0)
#define ACR_CHIP_TEMP_CFG                           0x4
#define ACR_CHIP_TEMP_EN                            0x2
#define ACR_CHIP_TEMP_ADC_START                     0x1
#define ACR_CHIP_TEMP_CFG_DEFAULT                   0
#define ACR_CHIP_TEMP_EN_DEFAULT                    0x80
#define ACR_CHIP_TEMP_ADC_START_DEFAULT             0
#define ACR_CHIP_TEMP_ADC_READY                     1
#define ACR_CHIP_TEMP_H_SHIFT                       8
#define INVALID_TEMP                                (-99)
#define ACR_GET_TEMP_RETRY                          100
#define ACR_ADC_WAIT_US                             15

#define ACR_CAL_MAGNIFICATION                       1000000
#define ACR_MOHM_TO_UOHM                            1000

#define HI6531_REG_TOTAL_NUM                       (0xDF)

/*acr macro definition end */



/*acr mul sel*/
enum acr_mul {
	ACR_MUL_35  = 0,
	ACR_MUL_70  = 1,
	ACR_MUL_140 = 2,
	ACR_MUL_280 = 3,
	ACR_MUL_MAX,
};


struct hi6531_device_info {
    int acr_support;
    int acr_gpio_en;
    struct device *dev;
    struct i2c_client *client;
};

#endif


