#ifndef _BQ25882_CHARGER_H_
#define _BQ25882_CHARGER_H_

#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/workqueue.h>

#ifndef BIT
#define BIT(x)   (1 << (x))
#endif

/*************************struct define area***************************/
struct bq25882_device_info {
	struct i2c_client *client;
	struct device *dev;
	struct work_struct irq_work;
	int gpio_cd;
	int gpio_int;
	int irq_int;
	int irq_active;
};

/*************************marco define area***************************/
#define BQ25882_REG_NUM                             (38)

/* Battery Voltage Regulation Limit */
#define BQ25882_REG_BVRL                            0x00

#define BQ25882_REG_BVRL_VREG_MASK                  (0xff)
#define BQ25882_REG_BVRL_VREG_SHIFT                 (0)

#define VCHARGE_MIN                                 (6800)
#define VCHARGE_MAX                                 (9200)
#define VCHARGE_STEP                                (10)

/* Charger Current Limit */
#define BQ25882_REG_CCL                             0x01

#define BQ25882_REG_CCL_EN_HIZ_MASK                 (BIT(7))
#define BQ25882_REG_CCL_EN_HIZ_SHIFT                (7)
#define BQ25882_REG_CCL_EN_ILIM_MASK                (BIT(6))
#define BQ25882_REG_CCL_EN_ILIM_SHIFT               (6)
#define BQ25882_REG_CCL_ICHG_MASK                   (BIT(5) | BIT(4) | BIT(3) |  BIT(2) | BIT(1) | BIT(0))
#define BQ25882_REG_CCL_ICHG_SHIFT                  (0)

#define ICHG_MAX                                    (2200)
#define ICHG_MIN                                    (100)
#define ICHG_STEP                                   (50)

/* Input Voltage Limit */
#define BQ25882_REG_IVL                             0x02

#define BQ25882_REG_IVL_EN_BAT_DISCHG_MASK          (BIT(6))
#define BQ25882_REG_IVL_EN_BAT_DISCHG_SHIFT         (6)
#define BQ25882_REG_IVL_VINDPM_MASK                 (BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define BQ25882_REG_IVL_VINDPM_SHIFT                (0)

#define VINDPM_MAX                                  (5500)
#define VINDPM_MIN                                  (3900)
#define VINDPM_OFFSET                               (3900)
#define VINDPM_STEP                                 (100)

/* Input Current Limit */
#define BQ25882_REG_ICL                             0x03
#define BQ25882_REG_ICL_FORCE_ICO_MASK              (BIT(7))
#define BQ25882_REG_ICL_FORCE_ICO_SHIFT             (7)
#define BQ25882_REG_ICL_FORCE_INDET_MASK            (BIT(6))
#define BQ25882_REG_ICL_FORCE_INDET_SHIFT           (6)
#define BQ25882_REG_ICL_EN_ICO_MASK                 (BIT(5))
#define BQ25882_REG_ICL_EN_ICO_SHIFT                (5)
#define BQ25882_REG_ICL_IINDPM_MASK                 (BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define BQ25882_REG_ICL_IINDPM_SHIFT                (0)

#define IINLIM_MAX                                  (3300)
#define IINLIM_MIN                                  (500)
#define IINLIM_STEP                                 (100)
#define IINLIM_OFFSET                               (500)

/* Precharge and Termination Current Limit */
#define BQ25882_REG_PTCL                            0x04

#define BQ25882_REG_PTCL_IPRECHG_MASK               (BIT(7) | BIT(6) | BIT(5) | BIT(4))
#define BQ25882_REG_PTCL_IPRECHG_SHIFT              (4)
#define BQ25882_REG_PTCL_ITERM_MASK                 (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define BQ25882_REG_PTCL_ITERM_SHIFT                (0)

#define ITERM_MIN                                   (50)
#define ITERM_MAX                                   (800)
#define ITERM_OFFSET                                (50)
#define ITERM_STEP                                  (50)

/* Charger Control 1 */
#define BQ25882_REG_CC1                             0x05

#define BQ25882_REG_CC1_EN_TERM_MASK                (BIT(7))
#define BQ25882_REG_CC1_EN_TERM_SHIFT               (7)
#define BQ25882_REG_CC1_WATCHDOG_MASK               (BIT(5) | BIT(4))
#define BQ25882_REG_CC1_WATCHDOG_SHIFT              (4)
#define BQ25882_REG_CC1_EN_TIMER_MASK               (BIT(3))
#define BQ25882_REG_CC1_EN_TIMER_SHIFT              (3)
#define BQ25882_REG_CC1_CHG_TIMER_MASK              (BIT(2) | BIT(1))
#define BQ25882_REG_CC1_CHG_TIMER_SHIFT             (1)
#define BQ25882_REG_CC1_TMR2X_EN_MASK               (BIT(0))
#define BQ25882_REG_CC1_TMR2X_EN_SHIFT              (0)

#define BQ25882_REG_CC1_WATCHDOG_DIS                (0)
#define BQ25882_REG_CC1_WATCHDOG_40                 (1)
#define BQ25882_REG_CC1_WATCHDOG_80                 (2)
#define BQ25882_REG_CC1_WATCHDOG_160                (3)

#define WATCHDOG_TIMER_40_S                         (40)
#define WATCHDOG_TIMER_80_S                         (80)
#define WATCHDOG_TIMER_160_S                        (160)

/* Charger Control 2 */
#define BQ25882_REG_CC2                             0x06

#define BQ25882_REG_CC2_EN_OTG_MASK                 (BIT(7))
#define BQ25882_REG_CC2_EN_OTG_SHIFT                (7)
#define BQ25882_REG_CC2_AUTO_INDET_EN_MASK          (BIT(6))
#define BQ25882_REG_CC2_AUTO_INDET_EN_SHIFT         (6)
#define BQ25882_REG_CC2_TREG_MASK                   (BIT(5) | BIT(4))
#define BQ25882_REG_CC2_TREG_SHIFT                  (4)
#define BQ25882_REG_CC2_EN_CHG_MASK                 (BIT(3))
#define BQ25882_REG_CC2_EN_CHG_SHIFT                (3)
#define BQ25882_REG_CC2_BATLOWV_MASK                (BIT(2))
#define BQ25882_REG_CC2_BATLOWV_SHIFT               (2)
#define BQ25882_REG_CC2_VRECHG_MASK                 (BIT(1) | BIT(0))
#define BQ25882_REG_CC2_VRECHG_SHIFT                (0)

/* Charger Control 3 */
#define BQ25882_REG_CC3                             0x07

#define BQ25882_REG_CC3_PFM_DIS_MASK                (BIT(7))
#define BQ25882_REG_CC3_PFM_DIS_SHIFT               (7)
#define BQ25882_REG_CC3_WD_RST_MASK                 (BIT(6))
#define BQ25882_REG_CC3_WD_RST_SHIFT                (6)
#define BQ25882_REG_CC3_TOPOFF_TIMER_MASK           (BIT(5) | BIT(4))
#define BQ25882_REG_CC3_TOPOFF_TIMER_SHIFT          (4)
#define BQ25882_REG_CC3_SYS_MIN_MASK                (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define BQ25882_REG_CC3_SYS_MIN_SHIFT               (0)

/* Charger Control 4 */
#define BQ25882_REG_CC4                             0x08

#define BQ25882_REG_CC4_BHOT_MASK                   (BIT(7) | BIT(6))
#define BQ25882_REG_CC4_BHOT_SHIFT                  (6)
#define BQ25882_REG_CC4_BCOLD_MASK                  (BIT(5))
#define BQ25882_REG_CC4_BCOLD_SHIFT                 (5)
#define BQ25882_REG_CC4_JEITA_VSET_MASK             (BIT(4) | BIT(3))
#define BQ25882_REG_CC4_JEITA_VSET_SHIFT            (3)
#define BQ25882_REG_CC4_JEITA_ISETH_MASK            (BIT(2))
#define BQ25882_REG_CC4_JEITA_ISETH_SHIFT           (2)
#define BQ25882_REG_CC4_JEITA_ISETC_MASK            (BIT(1) | BIT(0))
#define BQ25882_REG_CC4_JEITA_ISETC_SHIFT           (0)

/* OTG Control */
#define BQ25882_REG_OC                              0x09

#define BQ25882_REG_OC_OTG_ILIM_MASK                (BIT(7) | BIT(6) | BIT(5) | BIT(4))
#define BQ25882_REG_OC_OTG_ILIM_SHIFT               (4)
#define BQ25882_REG_OC_OTG_VLIM_MASK                (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define BQ25882_REG_OC_OTG_VLIM_SHIFT               (0)

#define OTG_ILIM_MIN                                (500)
#define OTG_ILIM_MAX                                (2000)
#define OTG_ILIM_STEP                               (100)
#define OTG_ILIM_OFFSET                             (500)

/* ICO Current Limit */
#define BQ25882_REG_ICOCL                           0x0A

#define BQ25882_REG_ICOCL_ICO_ILIM_MASK             (BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define BQ25882_REG_ICOCL_ICO_ILIM_SHIFT            (0)

#define BQ25882_REG_ICOCL_ICO_STEP_MV               (100)
#define BQ25882_REG_ICOCL_ICO_OFFSET_MV             (500)

/* Charger Status 1 */
#define BQ25882_REG_CS1                             0x0B

#define BQ25882_REG_CS1_ADC_DONE_STAT_MASK          (BIT(7))
#define BQ25882_REG_CS1_ADC_DONE_STAT_SHIFT         (7)
#define BQ25882_REG_CS1_IINDPM_STAT_MASK            (BIT(6))
#define BQ25882_REG_CS1_IINDPM_STAT_SHIFT           (6)
#define BQ25882_REG_CS1_VINDPM_STAT_MASK            (BIT(5))
#define BQ25882_REG_CS1_VINDPM_STAT_SHIFT           (5)
#define BQ25882_REG_CS1_TREG_STAT_MASK              (BIT(4))
#define BQ25882_REG_CS1_TREG_STAT_SHIFT             (4)
#define BQ25882_REG_CS1_WD_STAT_MASK                (BIT(3))
#define BQ25882_REG_CS1_WD_STAT_SHIFT               (3)
#define BQ25882_REG_CS1_CHRG_STAT_MASK              (BIT(2) | BIT(1) | BIT(0))
#define BQ25882_REG_CS1_CHRG_STAT_SHIFT             (0)

#define BQ25882_CHGR_STAT_CHARGE_DONE               (BIT(1) | BIT(2))
#define BQ25882_WATCHDOG_FAULT                      (BIT(3))

/* Charger Status 2 */
#define BQ25882_REG_CS2                             0x0C

#define BQ25882_REG_CS2_PG_STAT_MASK                (BIT(7))
#define BQ25882_REG_CS2_PG_STAT_SHIFT               (7)
#define BQ25882_REG_CS2_VBUS_STAT_MASK              (BIT(6) | BIT(5) | BIT(4))
#define BQ25882_REG_CS2_VBUS_STAT_SHIFT             (4)
#define BQ25882_REG_CS2_ICO_STAT_MASK               (BIT(2) | BIT(1))
#define BQ25882_REG_CS2_ICO_STAT_SHIFT              (1)
#define BQ25882_REG_CS2_VSYS_STAT_MASK              (BIT(0))
#define BQ25882_REG_CS2_VSYS_STAT_SHIFT             (0)

#define BQ25882_REG_CS2_ICO_MAX_IIN_DET             (BIT(2))
#define BQ25882_NOT_PG_STAT                         (BIT(7))

/* NTC Status */
#define BQ25882_REG_NS                              0x0D

#define BQ25882_REG_NS_TS_STAT_MASK                 (BIT(2) | BIT(1) | BIT(0))
#define BQ25882_REG_NS_TS_STAT_SHIFT                (0)

/* FAULT Status */
#define BQ25882_REG_FS                              0x0E

#define BQ25882_REG_FS_VBUS_OVP_STAT_MASK           BIT(7)
#define BQ25882_REG_FS_VBUS_OVP_STAT_SHIFT          7
#define BQ25882_REG_FS_TSHUT_STAT_MASK              BIT(6)
#define BQ25882_REG_FS_TSHUT_STAT_SHIFT             6
#define BQ25882_REG_FS_BATOVP_STAT_MASK             BIT(5)
#define BQ25882_REG_FS_BATOVP_STAT_SHIFT            5
#define BQ25882_REG_FS_TMR_STAT_MASK                BIT(4)
#define BQ25882_REG_FS_TMR_STAT_SHIFT               4

/* Charger Flag 1 */
#define BQ25882_REG_CF1                             0x0F

#define BQ25882_REG_CF1_ADC_DONE_FLAG_MASK          BIT(7)
#define BQ25882_REG_CF1_ADC_DONE_FLAG_SHIFT         7
#define BQ25882_REG_CF1_IINDPM_FLAG_MASK            BIT(6)
#define BQ25882_REG_CF1_IINDPM_FLAG_SHIFT           6
#define BQ25882_REG_CF1_VINDPM_FLAG_MASK            BIT(5)
#define BQ25882_REG_CF1_VINDPM_FLAG_SHIFT           5
#define BQ25882_REG_CF1_TREG_FLAG_MASK              BIT(4)
#define BQ25882_REG_CF1_TREG_FLAG_SHIFT             4
#define BQ25882_REG_CF1_WD_FLAG_MASK                BIT(3)
#define BQ25882_REG_CF1_WD_FLAG_SHIFT               3
#define BQ25882_REG_CF1_CHRG_FLAG_MASK              BIT(0)
#define BQ25882_REG_CF1_CHRG_FLAG_SHIFT             0

/* Charger Flag 2 */
#define BQ25882_REG_CF2                             0x10

#define BQ25882_REG_CF2_PG_FLAG_MASK                BIT(7)
#define BQ25882_REG_CF2_PG_FLAG_SHIFT               7
#define BQ25882_REG_CF2_VBUS_FLAG_MASK              BIT(4)
#define BQ25882_REG_CF2_VBUS_FLAG_SHIFT             4
#define BQ25882_REG_CF2_TS_FLAG_MASK                BIT(2)
#define BQ25882_REG_CF2_TS_FLAG_SHIFT               2
#define BQ25882_REG_CF2_ICO_FLAG_MASK               BIT(1)
#define BQ25882_REG_CF2_ICO_FLAG_SHIFT              1
#define BQ25882_REG_CF2_VSYS_FLAG_MASK              BIT(0)
#define BQ25882_REG_CF2_VSYS_FLAG_SHIFT             0

/* FAULT Flag */
#define BQ25882_REG_FF                              0x11

#define BQ25882_REG_FF_VBUS_OVP_FLAG                BIT(7)
#define BQ25882_REG_FF_TSHUT_FLAG                   BIT(6)
#define BQ25882_REG_FF_BATOVP_FLAG                  BIT(5)
#define BQ25882_REG_FF_TMR_FLAG                     BIT(4)
#define BQ25882_REG_FF_SYS_SHORT_FLAG               BIT(3)
#define BQ25882_REG_FF_OTG_FLAG_MASK                BIT(0)
#define BQ25882_REG_FF_OTG_FLAG_SHIFT               0

/* Charger Mask 1 */
#define BQ25882_REG_CM1                             0x12

#define BQ25882_REG_ADC_DONE_MASK                   BIT(7)
#define BQ25882_REG_CM1_IINDPM_MASK                 BIT(6)
#define BQ25882_REG_CM1_VINDPM_MASK                 BIT(5)
#define BQ25882_REG_CM1_TREG_MASK                   BIT(4)
#define BQ25882_REG_CM1_WD_MASK                     BIT(3)
#define BQ25882_REG_CM1_CHRG_MASK                   BIT(0)

/* Charger Mask 2 */
#define BQ25882_REG_CM2                             0x13

#define BQ25882_REG_CM2_PG_MASK                     BIT(7)
#define BQ25882_REG_CM2_VBUS_MASK                   BIT(4)
#define BQ25882_REG_CM2_TS_MASK                     BIT(2)
#define BQ25882_REG_CM2_ICO_MASK                    BIT(1)
#define BQ25882_REG_CM2_VSYS_MASK                   BIT(0)

/* FAULT Mask */
#define BQ25882_REG_FM                              0x14

#define BQ25882_REG_FM_VBUS_OVP_MASK_MASK           BIT(7)
#define BQ25882_REG_FM_VBUS_OVP_MASK_SHIFT          7
#define BQ25882_REG_FM_TSHUT_MASK_MASK              BIT(6)
#define BQ25882_REG_FM_TSHUT_MASK_SHIFT             6
#define BQ25882_REG_FM_BATOVP_MASK_MASK             BIT(5)
#define BQ25882_REG_FM_BATOVP_MASK_SHIFT            5
#define BQ25882_REG_FM_TMR_MASK_MASK                BIT(4)
#define BQ25882_REG_FM_TMR_MASK_SHIFT               4
#define BQ25882_REG_FM_SYS_SHORT_MASK_MASK          BIT(3)
#define BQ25882_REG_FM_SYS_SHORT_MASK_SHIFT         3
#define BQ25882_REG_FM_OTG_MASK_MASK                BIT(0)
#define BQ25882_REG_FM_OTG_MASK_SHIFT               0

/* ADC Control */
#define BQ25882_REG_AC                              0x15

#define BQ25882_REG_AC_ADC_EN_MASK                  (BIT(7))
#define BQ25882_REG_AC_ADC_EN_SHIFT                 (7)
#define BQ25882_REG_AC_ADC_RATE_MASK                (BIT(6))
#define BQ25882_REG_AC_ADC_RATE_SHIFT               (6)
#define BQ25882_REG_AC_ADC_SAMPLE_MASK              (BIT(5) | BIT(4))
#define BQ25882_REG_AC_ADC_SAMPLE_SHIFT             (4)

#define ADC_SAMPLE_3_MS                             (BIT(4) | BIT(5))
#define ADC_SAMPLE_6_MS                             (BIT(5))
#define ADC_SAMPLE_12_MS                            (BIT(4))
#define ADC_SAMPLE_24_MS                            (0)

/* ADC Function Disable */
#define BQ25882_REG_AFD                             0x16

#define BQ25882_REG_AC_IBUS_ADC_DIS_MASK            BIT(7)
#define BQ25882_REG_AC_IBUS_ADC_DIS_SHIFT           7
#define BQ25882_REG_AC_ICHG_ADC_DIS_MASK            BIT(6)
#define BQ25882_REG_AC_ICHG_ADC_DIS_SHIFT           6
#define BQ25882_REG_AC_VBUS_ADC_DIS_MASK            BIT(5)
#define BQ25882_REG_AC_VBUS_ADC_DIS_SHIFT           5
#define BQ25882_REG_AC_VBAT_ADC_DIS_MASK            BIT(4)
#define BQ25882_REG_AC_VBAT_ADC_DIS_SHIFT           4
#define BQ25882_REG_AC_VSYS_ADC_DIS_MASK            BIT(3)
#define BQ25882_REG_AC_VSYS_ADC_DIS_SHIFT           3
#define BQ25882_REG_AC_TS_ADC_DIS_MASK              BIT(2)
#define BQ25882_REG_AC_TS_ADC_DIS_SHIFT             2
#define BQ25882_REG_AC_TDIE_ADC_DIS_MASK            BIT(0)
#define BQ25882_REG_AC_TDIE_ADC_DIS_SHIFT           0

/* IBUS ADC */
#define BQ25882_REG_IBUS_ADC1                       0x17
#define BQ25882_REG_IBUS_ADC0                       0x18

#define BQ25882_REG_IBUS_ADC_STEP_MA                1

/* ICHG ADC */
#define BQ25882_REG_ICHG_ADC1                       0x19
#define BQ25882_REG_ICHG_ADC0                       0x1A

/* VBUS ADC */
#define BQ25882_REG_VBUS_ADC1                       0x1B
#define BQ25882_REG_VBUS_ADC0                       0x1C

#define BQ25882_REG_VBUS_ADC_STEP_MV                1

/* VBAT ADC */
#define BQ25882_REG_VBAT_ADC1                       0x1D
#define BQ25882_REG_VBAT_ADC0                       0x1E

#define BQ25882_REG_VBAT_ADC_STEP_MV                (1)

/* VSYS ADC */
#define BQ25882_REG_VSYS_ADC1                       0x1F
#define BQ25882_REG_VSYS_ADC0                       0x20

/* TS  ADC */
#define BQ25882_REG_TS_ADC1                         0x21
#define BQ25882_REG_TS_ADC0                         0x22

/* TDIE ADC */
#define BQ25882_REG_TDIE_ADC1                       0x23
#define BQ25882_REG_TDIE_ADC0                       0x24

/* Part Information */
#define BQ25882_REG_PI                              0x25

#define BQ25882_REG_PI_REG_RST_MASK                 (BIT(7))
#define BQ25882_REG_PI_REG_RST_SHIFT                (7)
#define BQ25882_REG_PI_PN_MASK                      (BIT(6) | BIT(5) | BIT(4) | BIT(3))
#define BQ25882_REG_PI_PN_SHIFT                     (3)
#define BQ25882_REG_PI_DEV_REV_MASK                 (BIT(2) | BIT(1) | BIT(0))
#define BQ25882_REG_PI_DEV_REV_SHIFT                (0)

#define BQ25882                                     (BIT(4))
#define CHIP_REVISION                               (BIT(0))

#endif /* end of _BQ25882_CHARGER_H_ */
