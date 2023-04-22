/*
 * Header file for hisi vbat drop protect device driver
 */

#ifndef	__HISI_VBAT_DROP_PROTECT_H
#define	__HISI_VBAT_DROP_PROTECT_H

#include <linux/workqueue.h>
#include <linux/mfd/hisi_pmic.h>
#include <soc_pmctrl_interface.h>
#include <linux/wakelock.h>
#include <pmic_interface.h>

#ifndef BIT
#define BIT(x)      (1 << (x))
#endif

#define VBAT_DROP_TEST /* for get core auto div status*/


/*interrupt reg in pmic*/
#define PMIC_VSYS_DROP_VOL_SET              PMIC_VSYS_DROP_SET_ADDR(0)
#define PMIC_VSYS_DROP_IRQ_REG              PMIC_VSYS_DROP_IRQ_ADDR(0)
#define PMIC_VSYS_DROP_IRQ_CLEAR            BIT(0)
#define PMIC_VSYS_DROP_IRQ_MASK_REG         PMIC_IRQ_MASK_13_ADDR(0)
#define PMIC_VSYS_DROP_IRQ_MASK             BIT(0)
#define PMIC_VBAT_DROP_VOL_SET_REG          PMIC_VSYS_DROP_SET_ADDR(0)
#define VBAT_DROP_VOL_DEFAULT               3100

#define VBAT_DROP_VOL_NORMAL_CNT            2
#define VBAT_DROP_VOL_INC_MV                200

/*core enable reg in pmctrl*/
#define LITTLE_VOL_DROP_EN_ADDR(base)       SOC_PMCTRL_VS_CTRL_LITTLE_ADDR(base)
#define MIDDLE_VOL_DROP_EN_ADDR(base)       SOC_PMCTRL_VS_CTRL_MIDDLE_ADDR(base)
#define BIG_VOL_DROP_EN_ADDR(base)          SOC_PMCTRL_VS_CTRL_BIG_ADDR(base)
#define L3_VOL_DROP_EN_ADDR(base)           SOC_PMCTRL_VS_CTRL_L3_ADDR(base)
#define GPU_VOL_DROP_EN_ADDR(base)          SOC_PMCTRL_VS_CTRL_GPU_ADDR(base)

#define LITTLE_VOL_DROP_EN_BIT              BIT(SOC_PMCTRL_VS_CTRL_LITTLE_vol_drop_en_little_START)
#define MIDDLE_VOL_DROP_EN_BIT              BIT(SOC_PMCTRL_VS_CTRL_MIDDLE_vol_drop_en_middle_START)
#define BIG_VOL_DROP_EN_BIT                 BIT(SOC_PMCTRL_VS_CTRL_BIG_vol_drop_en_big_START)
#define L3_VOL_DROP_EN_BIT                  BIT(SOC_PMCTRL_VS_CTRL_L3_vol_drop_en_l3_START)
#define GPU_VOL_DROP_EN_BIT                 BIT(SOC_PMCTRL_VS_CTRL_GPU_vol_drop_en_gpu_START)


#ifdef VBAT_DROP_TEST
/*core enable status register in pmctrl*/
#define LITTLE_VOL_DROP_EN_STAT_ADDR(base)  SOC_PMCTRL_VS_TEST_STAT_LITTLE_ADDR(base)
#define MIDDLE_VOL_DROP_EN_STAT_ADDR(base)  SOC_PMCTRL_VS_TEST_STAT_MIDDLE_ADDR(base)
#define BIG_VOL_DROP_EN_STAT_ADDR(base)     SOC_PMCTRL_VS_TEST_STAT_BIG_ADDR(base)
#define L3_VOL_DROP_EN_STAT_ADDR(base)      SOC_PMCTRL_VS_TEST_STAT_L3_ADDR(base)
#define GPU_VOL_DROP_EN_STAT_ADDR(base)     SOC_PMCTRL_VS_TEST_STAT_GPU_ADDR(base)

#define LITTLE_VOL_DROP_EN_STAT_BIT         BIT(SOC_PMCTRL_VS_TEST_STAT_LITTLE_vbat_drop_protect_ind_little_START)
#define MIDDLE_VOL_DROP_EN_STAT_BIT         BIT(SOC_PMCTRL_VS_TEST_STAT_MIDDLE_vbat_drop_protect_ind_middle_START)
#define BIG_VOL_DROP_EN_STAT_BIT            BIT(SOC_PMCTRL_VS_TEST_STAT_BIG_vbat_drop_protect_ind_big_START)
#define L3_VOL_DROP_EN_STAT_BIT             BIT(SOC_PMCTRL_VS_TEST_STAT_L3_vbat_drop_protect_ind_l3_START)
#define GPU_VOL_DROP_EN_STAT_BIT            BIT(SOC_PMCTRL_VS_TEST_STAT_GPU_vbat_drop_protect_ind_gpu_START)
#endif


/*pmic reg read and write interface macro */
#define HISI_VBAT_DROP_PMIC_REG_READ(regAddr)             hisi_pmic_reg_read(regAddr)
#define HISI_VBAT_DROP_PMIC_REG_WRITE(regAddr,regval)     hisi_pmic_reg_write((int)(regAddr),(int)regval)
#define HISI_VBAT_DROP_PMIC_REGS_READ(regAddr,buf,size)   hisi_pmic_array_read((int)(regAddr),(char*)(buf),(int)(size))
#define HISI_VBAT_DROP_PMIC_REGS_WRITE(regAddr,buf,size)  hisi_pmic_array_write((int)(regAddr),(char*)(buf),(int)(size))


/*cpu and gpu freq state*/
enum vbat_drop_freq {

    MIN_FREQ,
    RESTOR_FREQ
};

/*auto div core type*/
enum drop_freq_en {

    BIG_CPU    = 0,
    MIDDLE_CPU = 1,
    LITTLE_CPU = 2,
    L3_CPU     = 3,
    GPU_CPU    = 4,
    ALL
};


struct hisi_vbat_drop_protect_dev {
	struct device		    *dev;
    void __iomem            *pmctrl_base;
    struct wake_lock        vbatt_check_lock;
	struct delayed_work     vbat_drop_irq_work;
    int                     big_cpu_auto_div_en;     /* control big cpu auto 2 div enable */
    int                     middle_cpu_auto_div_en;
    int                     little_cpu_auto_div_en;
    int                     l3_auto_div_en;          /* control l3 auto 2 div enable */
    int                     gpu_auto_div_en;
    unsigned int            vbat_drop_vol_mv;
    int                     vbat_drop_irq;
};

#endif		/* __HISI_VBAT_DROP_PROTECT_H */
