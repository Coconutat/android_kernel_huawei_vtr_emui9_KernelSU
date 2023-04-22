#ifndef _HISI_HI6XXX_COUL_H_
#define _HISI_HI6XXX_COUL_H_
/*****************************************************************************************
* Filename:	hisi_hi6xxx_coul.h
*
* Discription:  hi6xxx coulometer driver headfile.
* Copyright: 	(C) 2013 huawei.
*
* revision history:
*
******************************************************************************************/
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

#define DEFULT_I_GATE_VALUE   5000  // 5000 mA

#define HI6XXX_I_OUT_GATE       PMIC_I_OUT_GATE0_ADDR(0)
#define HI6XXX_I_IN_GATE      PMIC_I_IN_GATE0_ADDR(0)

#define HI6XXX_VOL_OFFSET_B_ADDR       PMIC_OTP0_59_W_ADDR(0)
#define HI6XXX_VOL_OFFSET_A_ADDR_0     PMIC_OTP0_59_W_ADDR(0)
#define HI6XXX_VOL_OFFSET_A_ADDR_1     PMIC_OTP0_58_W_ADDR(0)

#define HI6XXX_COUL_ECO_CONFIG_ADDR     PMIC_CLJ_CTRL_REGS3_ADDR(0)
#define HI6XXX_COUL_WAIT_COMP_ADDR     PMIC_CLJ_CTRL_REGS3_ADDR(0)

#define WAIT_COMP_EN_SHIFT              PMIC_CLJ_CTRL_REGS3_wait_comp_en_START
#define WAIT_COMP_EN_MASK               (1 << WAIT_COMP_EN_SHIFT)

#define ECO_DELAY_EN_SHIFT              PMIC_CLJ_CTRL_REGS3_eco_delay_en_START
#define ECO_DELAY_EN_MASK               (1 << ECO_DELAY_EN_SHIFT)
#define ECO_DELAY_SEL_SHIFT              PMIC_CLJ_CTRL_REGS3_coul_eco_dly_sel_START
#define ECO_DELAY_SEL_MASK               (0x03 << ECO_DELAY_SEL_SHIFT)

#define ECO_DELAY_500MS         (0)
#define ECO_DELAY_2S         (1)
#define ECO_DELAY_5S         (2)
#define ECO_DELAY_10S         (3)
#define ECO_DELAY_EN        (1)
#define ECO_DELAY_DIS        (0)

#define WAIT_COMP_EN            (1)
#define WAIT_COMP_DIS            (0)

#define HI6XXX_COUL_TEMP_CTRL PMIC_COUL_TEMP_CTRL_ADDR(0)
#define TEMP_EN     BIT(0)
#define TEMP_RDY  BIT(1)
#define VOUT_RDY    BIT(2)

#define HI6XXX_COUL_TEMP_DATA   PMIC_TEMP0_RDATA_ADDR(0)
#define HI6XXX_COUL_OCV_TEMP_DATA   PMIC_OCV_TEMP0_ADDR(0)
#define HI6XXX_COUL_ECOOUT_TEMP_DATA   PMIC_ECO_OUT_TEMP_0_ADDR(0)

#define HI6XXX_SOH_TBAT_DATA_BASE          PMIC_ACRADC_TBAT_DATA_L_ADDR(0)

#define HI6XXX_CURRENT                PMIC_CURRENT_0_ADDR(0)
#define HI6XXX_V_OUT                  PMIC_V_OUT_0_ADDR(0)
#define HI6XXX_CL_OUT_BASE            PMIC_CL_OUT0_ADDR(0)
#define HI6XXX_CL_IN_BASE             PMIC_CL_IN0_ADDR(0)
#define HI6XXX_CHG_TIMER_BASE         PMIC_CHG_TIMER0_ADDR(0)
#define HI6XXX_LOAD_TIMER_BASE        PMIC_LOAD_TIMER0_ADDR(0)
#define HI6XXX_CL_INT_BASE            PMIC_CL_INT0_ADDR(0)
#define HI6XXX_VOL_INT_BASE           PMIC_V_INT0_ADDR(0)
#define HI6XXX_OFFSET_CURRENT         PMIC_OFFSET_CURRENT0_ADDR(0)
#define HI6XXX_OFFSET_VOLTAGE         PMIC_OFFSET_VOLTAGE0_ADDR(0)
#define HI6XXX_OCV_VOLTAGE_BASE       PMIC_OCV_VOLTAGE0_ADDR(0)
#define HI6XXX_OCV_CURRENT_BASE       PMIC_OCV_CURRENT0_ADDR(0)
#define HI6XXX_ECO_OUT_CLIN_REG_BASE  PMIC_ECO_OUT_CLIN_0_ADDR(0)
#define HI6XXX_ECO_OUT_CLOUT_REG_BASE PMIC_ECO_OUT_CLOUT_0_ADDR(0)
#define HI6XXX_VOL_FIFO_BASE           PMIC_V_PRE0_OUT0_ADDR(0)
#define HI6XXX_CUR_FIFO_BASE          PMIC_CURRENT_PRE0_OUT0_ADDR(0)

#define HI6XXX_COUL_ECO_MASK          PMIC_COUL_ECO_MASK_ADDR(0)

#define HI6XXX_FIFO_CLEAR             PMIC_CLJ_CTRL_REGS2_ADDR(0)         //use bit 1
#define HI6XXX_DEBUG1_REG             PMIC_CLJ_DEBUG1_ADDR(0)

#define HI6XXX_OFFSET_CUR_MODIFY_BASE PMIC_OFFSET_CURRENT_MOD_0_ADDR(0)
#define HI6XXX_OFFSET_VOL_MODIFY_BASE PMIC_OFFSET_VOLTAGE_MOD_0_ADDR(0)

/*coul reserverd regs use */
#define HI6XXX_BATTERY_MOVE_ADDR      PMIC_HRST_REG0_ADDR(0)
#define BATTERY_MOVE_MAGIC_NUM            0xc3
#define BATTERY_PLUGOUT_SHUTDOWN_MAGIC_NUM 0x18

#define HI6XXX_OCV_CHOOSE             PMIC_HRST_REG1_ADDR(0) /*use bit5*/
#define HI6XXX_COUL_TEMP_PROTECT      PMIC_HRST_REG1_ADDR(0) /*use bit 4*/
#define HI6XXX_DELTA_RC_SCENE         PMIC_HRST_REG1_ADDR(0) /*use bit 3*/
#define HI6XXX_PD_BY_OCV_WRONG        PMIC_HRST_REG1_ADDR(0) /*use bit 2*/
#define HI6XXX_NV_READ_SUCCESS        PMIC_HRST_REG1_ADDR(0) /*use bit 1*/
#define HI6XXX_NV_SAVE_SUCCESS        PMIC_HRST_REG1_ADDR(0) /*use bit 0*/
#define USE_SAVED_OCV_FLAG                BIT(5)
#define TEMP_PROTECT_BITMASK              BIT(4)
#define DELTA_RC_SCENE_BITMASK            BIT(3)
#define PD_BY_OCV_WRONG_BIT               BIT(2)
#define NV_READ_BITMASK                   BIT(1)
#define NV_SAVE_BITMASK                   BIT(0)

#define HI6XXX_SAVE_OCV_ADDR          PMIC_HRST_REG2_ADDR(0) /*use 2byte,reserved3 and reserved4*/
#define HI6XXX_SAVE_OCV_RESERVED      PMIC_HRST_REG3_ADDR(0)
#define INVALID_TO_UPDATE_FCC             (0x8000)

#define HI6XXX_SAVE_OCV_TEMP_ADDR      PMIC_HRST_REG4_ADDR(0)/*OCV TEMP saved use 2bytes*/
#define HI6XXX_SAVE_OCV_TEMP_RESERVED  PMIC_HRST_REG5_ADDR(0)

/*record last soc*/
#define HI6XXX_SAVE_LAST_SOC  PMIC_HRST_REG6_ADDR(0)/*last soc 0-6bit */
#define HI6XXX_SAVE_LAST_SOC_VAILD  PMIC_HRST_REG6_ADDR(0)/*last soc vaild 7bit */
#define SAVE_LAST_SOC              (BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SAVE_LAST_SOC_FALG         BIT(7)
#define CLEAR_LAST_SOC_FALG         0x7F

#define HI6XXX_OCV_LEVEL_ADDR  PMIC_HRST_REG7_ADDR(0)/*last soc 2-5bit */
#define SAVE_OCV_LEVEL            (BIT(5) | BIT(4) | BIT(3) | BIT(2))
#define OCV_LEVEL_SHIFT           (2)

#define HI6XXX_ECO_OCV_ADDR  PMIC_HRST_REG7_ADDR(0)/*6-7bit */
#define EN_ECO_SAMPLE            BIT(6)
#define CLR_ECO_SAMPLE           BIT(7)

/************************************************************
    coul register of smartstar
************************************************************/
#define HI6XXX_COUL_CTRL_REG         PMIC_CLJ_CTRL_REG_ADDR(0)
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
#define ECO_COUL_CTRL_VAL                ( COUL_ECO_FLT_200MS | COUL_ECO_REFLASH | COUL_ECO_PMU_EN)
#define CALI_AUTO_ONOFF_CTRL		 BIT(5)
#define CALI_AUTO_TIME_15S		 0
#define CALI_AUTO_TIME_30S		 BIT(2)
#define CALI_AUTO_TIME_60S		 BIT(3)
#define CALI_AUTO_TIME_2MIN		 (BIT(2) | BIT(3))
#define CALI_AUTO_TIME_4MIN		 BIT(4)
#define CALI_AUTO_TIME_8MIN		 (BIT(2) | BIT(4))
#define CALI_AUTO_TIME_16MIN		 (BIT(4) | BIT(3))
#define CALI_AUTO_TIME_32MIN		 (BIT(2) | BIT(3) | BIT(4))
#define HI6XXX_CLJ_CTRL                PMIC_CLJ_CTRL_REGS2_ADDR(0)
#define COUL_CTRL_ONOFF_REG                BIT(0)
#define CALI_CLJ_DEFAULT_VALUE		(CALI_AUTO_TIME_4MIN | CALI_AUTO_ONOFF_CTRL)
#define MASK_CALI_AUTO_OFF		0xDF

#define COUL_CLK_MODE_ADDR               PMIC_STATUS1_ADDR(0)
#define XO32K_MODE_MSK                   BIT(2)   // tells 32k or others
#define COUL_32K_CLK_MODE                     BIT(2)
#define NO_32K_MODE                      0

#define HI6XXX_COUL_IRQ_REG          PMIC_COUL_IRQ_ADDR(0)//SOC_SMART_COUL_IRQ_ADDR(0)
#define HI6XXX_COUL_IRQ_MASK_REG     PMIC_COUL_IRQ_MASK_ADDR(0)//SOC_SMART_COUL_IRQ_MASK_ADDR(0)
#define COUL_I_OUT_MASK                     BIT(5)
#define COUL_I_IN_MASK                     BIT(4)
#define COUL_VBAT_INT_MASK               BIT(3)
#define COUL_CL_IN_MASK                  BIT(2)
#define COUL_CL_OUT_MASK                 BIT(1)
#define COUL_CL_INT_MASK                 BIT(0)
#define COUL_INT_MASK_ALL             (COUL_I_OUT_MASK|COUL_I_IN_MASK|COUL_VBAT_INT_MASK | COUL_CL_IN_MASK | COUL_CL_OUT_MASK|COUL_CL_INT_MASK)
#define DEFAULT_BATTERY_VOL_2_PERCENT    3350
#define DEFAULT_BATTERY_VOL_0_PERCENT    3150

#define HI6XXX_COUL_VERSION_ADDR     PMIC_VERSION0_ADDR(0)
#define COUL_HI6XXX                  0x36
#define COUL_HI6421V700              0x700  // TODO!!

#define HI6XXX_COUL_STATE_REG         PMIC_STATE_TEST_ADDR(0)   //Warning: bit change
#define COUL_WORKING     (0x5)
#define COUL_CALI_ING     (0x4)
#define COUL_MSTATE_MASK  (0x0f)

#ifdef HISI_COUL_HI6421V700
#define FIFO_DEPTH                       (10)
#else
#define FIFO_DEPTH                       (8)
#endif


#define R_COUL_MOHM                      10      /* resisitance mohm */

/* vol offset a/b value*/
#define VOL_OFFSET_A_STEP 39
#define VOL_OFFSET_B_STEP 78125
#define VOL_OFFSET_A_BASE 990000
#define VOL_OFFSET_B_BASE (-5000000)
#define VOL_OFFSET_B_VALID_MASK  0xFE
#define VOL_OFFSET_A_HIGH_VALID_MASK 0x1FE
#define VOL_OFFSET_A_LOW_VALID_MASK 0x001
#define VOL_OFFSET_A_VALID_MASK 0x1FF

/*v700 register write lock/unlock*/
#define HI6XXX_DEBUG_WRITE_PRO	PMIC_DEBUG_WRITE_PRO_ADDR(0)
#define COUL_WRITE_LOCK		0x56
#define COUL_WRITE_UNLOCK	0xA9

#define INVALID_TEMP                    (-99)

extern unsigned int hisi_pmic_reg_read (int reg_addr);
extern void hisi_pmic_reg_write(int addr, int val);
extern int hisi_pmic_array_read(int addr, char *buff, unsigned int len);
extern int hisi_pmic_array_write(int addr, char *buff, unsigned int len);

#define HI6XXX_REG_READ(regAddr)             hisi_pmic_reg_read(regAddr)
#define HI6XXX_REG_WRITE(regAddr,regval)     hisi_pmic_reg_write((int)(regAddr),(int)regval)
#define HI6XXX_REGS_READ(regAddr,buf,size)   hisi_pmic_array_read((int)(regAddr),(char*)(buf),(int)(size))
#define HI6XXX_REGS_WRITE(regAddr,buf,size)  hisi_pmic_array_write((int)(regAddr),(char*)(buf),(int)(size))
#define HI6XXX_COUL_INFO
#ifndef HI6XXX_COUL_INFO
#define HI6XXX_COUL_ERR(fmt,args...)              do {} while (0)
#define HI6XXX_COUL_EVT(fmt,args...)              do {} while (0)
#define HI6XXX_COUL_INF(fmt,args...)              do {} while (0)
#else
#define HI6XXX_COUL_ERR(fmt,args...) do { printk(KERN_ERR    "[hisi_hi6xxx_coul]" fmt, ## args); } while (0)
#define HI6XXX_COUL_EVT(fmt,args...) do { printk(KERN_WARNING"[hisi_hi6xxx_coul]" fmt, ## args); } while (0)
#define HI6XXX_COUL_INF(fmt,args...) do { printk(KERN_INFO   "[hisi_hi6xxx_coul]" fmt, ## args); } while (0)
#endif

struct hi6xxx_coul_device_info
{
    struct device *dev;
    struct delayed_work irq_work;
    int irq;
    unsigned char irq_mask;
    u16 chip_proj;
    u16 chip_version;
};
#endif


