#ifndef _HISI_HI6555V200_COUL_H_
#define _HISI_HI6555V200_COUL_H_

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <asm/irq.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/hisi/hisi_adc.h>
#include <pmic_interface.h>

#ifndef BIT
#define BIT(x)      (1 << (x))
#endif


#define HI6555_COUL_REGS_UNLOCK_START      (0x1ff)

#define HI6555V200_VOL_OFFSET_B_ADDR      PMIC_OTP_34_R_ADDR(0)
#define HI6555V200_VOL_OFFSET_A_ADDR_0    PMIC_OTP_34_R_ADDR(0)
#define HI6555V200_VOL_OFFSET_A_ADDR_1    PMIC_OTP_35_R_ADDR(0)

#define HI6555V200_CURRENT                PMIC_CURRENT_0_ADDR(0)
#define HI6555V200_V_OUT                  PMIC_V_OUT_0_ADDR(0)
#define HI6555V200_CL_OUT_BASE            PMIC_CL_OUT0_ADDR(0)
#define HI6555V200_CL_IN_BASE             PMIC_CL_IN0_ADDR(0)
#define HI6555V200_CHG_TIMER_BASE         PMIC_CHG_TIMER0_ADDR(0)
#define HI6555V200_LOAD_TIMER_BASE        PMIC_LOAD_TIMER0_ADDR(0)
#define HI6555V200_CL_INT_BASE            PMIC_CL_INT0_ADDR(0)
#define HI6555V200_VOL_INT_BASE           PMIC_V_INT0_ADDR(0)
#define HI6555V200_OFFSET_CURRENT         PMIC_OFFSET_CURRENT0_ADDR(0)
#define HI6555V200_OFFSET_VOLTAGE         PMIC_OFFSET_VOLTAGE0_ADDR(0)
#define HI6555V200_OCV_VOLTAGE_BASE       PMIC_OCV_VOLTAGE0_ADDR(0)
#define HI6555V200_OCV_CURRENT_BASE       PMIC_OCV_CURRENT0_ADDR(0)
#define HI6555V200_ECO_OUT_CLIN_REG_BASE  PMIC_ECO_OUT_CLIN_0_ADDR(0)
#define HI6555V200_ECO_OUT_CLOUT_REG_BASE PMIC_ECO_OUT_CLOUT_0_ADDR(0)
#define HI6555V200_VOL_FIFO_BASE          PMIC_V_OUT0_PRE0_ADDR(0)
#define HI6555V200_CUR_FIFO_BASE          PMIC_CURRENT0_PRE0_ADDR(0)
#define HI6555V200_COUL_ECO_MASK          PMIC_COUL_ECO_MASK_ADDR(0)

#define HI6555V200_FIFO_CLEAR             PMIC_CLJ_CTRL_REGS2_ADDR(0)         //use bit 1
#define HI6555V200_OFFSET_CUR_MODIFY_BASE PMIC_OFFSET_CURRENT_MOD_0_ADDR(0)

/*coul reserverd regs use */
#define HI6555V200_BATTERY_MOVE_ADDR      PMIC_HRST_REG0_ADDR(0)
#define BATTERY_MOVE_MAGIC_NUM            0xc3
#define BATTERY_PLUGOUT_SHUTDOWN_MAGIC_NUM 0x18

#define HI6555V200_OCV_CHOOSE             PMIC_HRST_REG1_ADDR(0) /*use bit5*/
#define HI6555V200_COUL_TEMP_PROTECT      PMIC_HRST_REG1_ADDR(0) /*use bit 4*/
#define HI6555V200_DELTA_RC_SCENE         PMIC_HRST_REG1_ADDR(0) /*use bit 3*/
#define HI6555V200_PD_BY_OCV_WRONG        PMIC_HRST_REG1_ADDR(0) /*use bit 2*/
#define HI6555V200_NV_READ_SUCCESS        PMIC_HRST_REG1_ADDR(0) /*use bit 1*/
#define HI6555V200_NV_SAVE_SUCCESS        PMIC_HRST_REG1_ADDR(0) /*use bit 0*/
#define USE_SAVED_OCV_FLAG                BIT(5)
#define TEMP_PROTECT_BITMASK              BIT(4)
#define DELTA_RC_SCENE_BITMASK            BIT(3)
#define PD_BY_OCV_WRONG_BIT               BIT(2)
#define NV_READ_BITMASK                   BIT(1)
#define NV_SAVE_BITMASK                   BIT(0)

#define HI6555V200_SAVE_OCV_ADDR          PMIC_HRST_REG2_ADDR(0) /*saved OCV use 2bytes*/
#define HI6555V200_SAVE_OCV_RESERVED      PMIC_HRST_REG3_ADDR(0)
#define INVALID_TO_UPDATE_FCC             (0x8000)

#define HI6555V200_SAVE_OCV_TEMP_ADDR      PMIC_HRST_REG4_ADDR(0)/*OCV TEMP saved use 2bytes*/
#define HI6555V200_SAVE_OCV_TEMP_RESERVED  PMIC_HRST_REG5_ADDR(0)

/*record last soc*/
#define HI6555V200_SAVE_LAST_SOC  PMIC_HRST_REG6_ADDR(0)/*last soc 0-6bit */
#define HI6555V200_SAVE_LAST_SOC_VAILD  PMIC_HRST_REG6_ADDR(0)/*last soc vaild 7bit */
#define SAVE_LAST_SOC              (BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SAVE_LAST_SOC_FALG         BIT(7)
#define CLEAR_LAST_SOC_FALG         0x7F

#define HI6555V200_OCV_LEVEL_ADDR  PMIC_HRST_REG7_ADDR(0)/*last soc 2-5bit */
#define SAVE_OCV_LEVEL            (BIT(5) | BIT(4) | BIT(3) | BIT(2))
#define OCV_LEVEL_SHIFT           (2)
/************************************************************
    coul register of smartstar
************************************************************/
#define HI6555V200_COUL_CTRL_REG         PMIC_CLJ_CTRL_REG_ADDR(0)
#define COUL_CALI_ENABLE                 BIT(7)
#define COUL_ECO_FLT_50MS                0
#define COUL_ECO_FLT_100MS               BIT(4)
#define COUL_ECO_FLT_150MS               BIT(5)
#define COUL_ECO_FLT_200MS               (BIT(4) | BIT(5))
#define COUL_ECO_FLT_250MS               BIT(6)
#define COUL_ECO_FLT_300MS               (BIT(4) | BIT(6))
#define COUL_ECO_FLT_350MS               (BIT(5) | BIT(6))
#define COUL_ECO_FLT_400MS               (BIT(4) | BIT(5) | BIT(6))
#define COUL_ALL_REFLASH                 0
#define COUL_ECO_REFLASH                 BIT(3)
#define COUL_ECO_ENABLE                  BIT(2)
#define COUL_ECO_PMU_EN                  (BIT(0) | BIT(1))
#define COUL_ECO_DISABLE                 0
#define COUL_FIFO_CLEAR                  BIT(1)
#define DEFAULT_COUL_CTRL_VAL            ( COUL_ECO_FLT_100MS | COUL_ALL_REFLASH | COUL_ECO_DISABLE)
#define ECO_COUL_CTRL_VAL   	         ( COUL_ECO_FLT_200MS | COUL_ECO_REFLASH | COUL_ECO_PMU_EN)

#define COUL_CLK_MODE_ADDR               PMIC_STATUS1_ADDR(0)
#define XO32K_MODE_MSK                   BIT(2)   // tells 32k or others
#define COUL_32K_CLK_MODE                BIT(2)
#define NO_32K_MODE                      0

#define HI6555V200_COUL_IRQ_REG          PMIC_COUL_IRQ_ADDR(0)//SOC_SMART_COUL_IRQ_ADDR(0)
#define HI6555V200_COUL_IRQ_MASK_REG     PMIC_COUL_IRQ_MASK_ADDR(0)//SOC_SMART_COUL_IRQ_MASK_ADDR(0)
#define COUL_VBAT_INT_MASK               BIT(3)
#define COUL_CL_IN_MASK                  BIT(2)
#define COUL_CL_OUT_MASK                 BIT(1)
#define COUL_CL_INT_MASK                 BIT(0)
#define DEFAULT_BATTERY_VOL_2_PERCENT    3350
#define DEFAULT_BATTERY_VOL_0_PERCENT    3150

#define HI6555V200_COUL_VERSION_ADDR     PMIC_VERSION4_ADDR(0)
#define COUL_HI6555V2XX                  0x2

#define HI6555V200_CHIP_ID               PMIC_VERSION5_ADDR(0)
#define COUL_HI6555V200                  0x0

#define HI6555V200_COUL_STATE_REG         PMIC_STATE_TEST_ADDR(0)   //Warning: bit change
#define COUL_CALI_ING                    (BIT(0) | BIT(1))

/*bit uah*/
#define BIT_FOR_UAH_32K                  143127
#define BIT_FOR_UAH_DCXO_586             143153

#define FIFO_DEPTH                       10
#define IRQ_MAX                          4
#define R_COUL_MOHM                      10      /* resisitance mohm */

#define VOL_OFFSET_A_STEP                100
#define VOL_OFFSET_B_STEP                1000
#define VOL_OFFSET_A_BASE                974400
#define VOL_OFFSET_B_BASE                (-31500)
#define VOL_OFFSET_B_VALID_MASK          0x7C
#define VOL_OFFSET_A_HIGH_VALID_MASK     0x1fc
#define VOL_OFFSET_A_LOW_VALID_MASK      0x3
#define VOL_OFFSET_A_VALID_MASK          0x3FF
#define CALI_AUTO_ONOFF_CTRL		 BIT(5)
#define CALI_AUTO_TIME_15S		 0
#define CALI_AUTO_TIME_30S		 BIT(2)
#define CALI_AUTO_TIME_60S		 BIT(3)
#define CALI_AUTO_TIME_2MIN		 (BIT(2) | BIT(3))
#define CALI_AUTO_TIME_4MIN		 BIT(4)
#define CALI_AUTO_TIME_8MIN		 (BIT(2) | BIT(4))
#define CALI_AUTO_TIME_16MIN		 (BIT(4) | BIT(3))
#define CALI_AUTO_TIME_32MIN		 (BIT(2) | BIT(3) | BIT(4))
#define HI6555V200_CLJ_CTRL                PMIC_CLJ_CTRL_REGS2_ADDR(0)
#define COUL_CTRL_ONOFF_REG                BIT(0)
#define CALI_CLJ_DEFAULT_VALUE		(CALI_AUTO_TIME_4MIN | CALI_AUTO_ONOFF_CTRL | COUL_CTRL_ONOFF_REG)
#define MASK_CALI_AUTO_OFF		0xDF

extern unsigned int hisi_pmic_reg_read (int reg_addr);
extern void hisi_pmic_reg_write(int addr, int val);
extern int hisi_pmic_array_read(int addr, char *buff, unsigned int len);
extern int hisi_pmic_array_write(int addr, char *buff, unsigned int len);

extern bool g_ec_6555v200;


#define HI6555V200_REG_READ(regAddr)             hisi_pmic_reg_read(regAddr)
#define HI6555V200_REG_WRITE(regAddr,regval)     hisi_pmic_reg_write((int)(regAddr),(int)regval)
#define HI6555V200_REGS_READ(regAddr,buf,size)   hisi_pmic_array_read((int)(regAddr),(char*)(buf),(int)(size))
#define HI6555V200_REGS_WRITE(regAddr,buf,size)  hisi_pmic_array_write((int)(regAddr),(char*)(buf),(int)(size))
#define HI6555V200_COUL_INFO
#ifndef HI6555V200_COUL_INFO
#define HI6555V200_COUL_ERR(fmt,args...)              do {} while (0)
#define HI6555V200_COUL_EVT(fmt,args...)              do {} while (0)
#define HI6555V200_COUL_INF(fmt,args...)              do {} while (0)
#else
#define HI6555V200_COUL_ERR(fmt,args...) do { printk(KERN_ERR    "[hisi_HI6555V200_coul]" fmt, ## args); } while (0)
#define HI6555V200_COUL_EVT(fmt,args...) do { printk(KERN_WARNING"[hisi_HI6555V200_coul]" fmt, ## args); } while (0)
#define HI6555V200_COUL_INF(fmt,args...) do { printk(KERN_INFO   "[hisi_HI6555V200_coul]" fmt, ## args); } while (0)
#endif

struct hi6555v200_coul_device_info
{
    struct device *dev;
    struct delayed_work irq_work;
    int irq;
    unsigned char irq_mask;
};
#endif


