#ifndef _FPF2283_H_
#define _FPF2283_H_

#ifndef BIT
#define BIT(x)    (1 << (x))
#endif

/* ID reg=0x00, default=0x08, R */
#define FPF2283_ID_REG                             0x00

#define FPF2283_ID_VID_MASK                        (BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3))
#define FPF2283_ID_VID_SHIFT                       (3)
#define FPF2283_ID_RID_MASK                        (BIT(2) | BIT(1) | BIT(0))
#define FPF2283_ID_RID_SHIFT                       (0)

#define FPF2283_DEVICE_ID_FPF2283                  (BIT(3))

/* Enable reg=0x01, default=0x00, RW */
#define FPF2283_ENABLE_REG                         0x01

#define FPF2283_ENABLE_INIT                        (0x00)

#define FPF2283_ENABLE_SW_ENB_MASK                 (BIT(7))
#define FPF2283_ENABLE_SW_ENB_SHIFT                (7)
#define FPF2283_ENABLE_DET_EN_MASK                 (BIT(6))
#define FPF2283_ENABLE_DET_EN_SHIFT                (6)

#define FPF2283_ENABLE_SW_ENB_ENABLE               (0)
#define FPF2283_ENABLE_SW_ENB_DISABLED             (1)
#define FPF2283_ENABLE_DET_EN_ENABLE               (1)
#define FPF2283_ENABLE_DET_EN_DISABLED             (0)

/* Detection Status reg=0x02, default=0x00, R */
#define FPF2283_DETECTION_STATUS_REG               0x02

#define FPF2283_DETECTION_STATUS_PON_STS_MASK      (BIT(7))
#define FPF2283_DETECTION_STATUS_PON_STS_SHIFT     (7)
#define FPF2283_DETECTION_STATUS_TAG_STS_MASK      (BIT(6))
#define FPF2283_DETECTION_STATUS_TAG_STS_SHIFT     (6)
#define FPF2283_DETECTION_STATUS_TMO_STS_MASK      (BIT(5))
#define FPF2283_DETECTION_STATUS_TMO_STS_SHIFT     (5)
#define FPF2283_DETECTION_STATUS_SW_STS_MASK       (BIT(4))
#define FPF2283_DETECTION_STATUS_SW_STS_SHIFT      (4)

/* Power Switch flag reg=0x03, default=0x00, RC */
#define FPF2283_POWER_SWITCH_FLAG_REG              0x03

#define FPF2283_POWER_SWITCH_FLAG_OV_FLG_MASK      (BIT(2))
#define FPF2283_POWER_SWITCH_FLAG_OV_FLG_SHIFT     (2)
#define FPF2283_POWER_SWITCH_FLAG_OC_FLG_MASK      (BIT(1))
#define FPF2283_POWER_SWITCH_FLAG_OC_FLG_SHIFT     (1)
#define FPF2283_POWER_SWITCH_FLAG_OT_FLG_MASK      (BIT(0))
#define FPF2283_POWER_SWITCH_FLAG_OT_FLG_SHIFT     (0)

/* Interrupt Mask reg=0x04, default=0x00, RW */
#define FPF2283_INTERRUPT_MASK_REG                 0x04

#define FPF2283_INTERRUPT_MASK_INIT                (0x07)

#define FPF2283_INTERRUPT_MASK_PON_MASK            (BIT(7))
#define FPF2283_INTERRUPT_MASK_PON_SHIFT           (7)
#define FPF2283_INTERRUPT_MASK_TAG_MASK            (BIT(6))
#define FPF2283_INTERRUPT_MASK_TAG_SHIFT           (6)
#define FPF2283_INTERRUPT_MASK_TMO_MASK            (BIT(5))
#define FPF2283_INTERRUPT_MASK_TMO_SHIFT           (5)
#define FPF2283_INTERRUPT_MASK_SW_MASK             (BIT(4))
#define FPF2283_INTERRUPT_MASK_SW_SHIFT            (4)
#define FPF2283_INTERRUPT_MASK_OV_MASK             (BIT(2))
#define FPF2283_INTERRUPT_MASK_OV_SHIFT            (2)
#define FPF2283_INTERRUPT_MASK_OC_MASK             (BIT(1))
#define FPF2283_INTERRUPT_MASK_OC_SHIFT            (1)
#define FPF2283_INTERRUPT_MASK_OT_MASK             (BIT(0))
#define FPF2283_INTERRUPT_MASK_OT_SHIFT            (0)

/* OVLO Trigger Level reg=0x05, default=0x00, RW */
#define FPF2283_OVP_REG                            0x05

#define FPF2283_OVP_OFFSET_MASK                    (BIT(6) | BIT(5) | BIT(4))
#define FPF2283_OVP_OFFSET_SHIFT                   (4)
#define FPF2283_OVP_CENTER_VALUE_MASK              (BIT(1) | BIT(0))
#define FPF2283_OVP_CENTER_VALUE_SHIFT             (0)

#define FPF2283_OVP_OFFSET_N600MV                  (0x0) /* negative */
#define FPF2283_OVP_OFFSET_N400MV                  (0x1) /* negative */
#define FPF2283_OVP_OFFSET_N200MV                  (0x2) /* negative */
#define FPF2283_OVP_OFFSET_0MV                     (0x3)
#define FPF2283_OVP_OFFSET_200MV                   (0x4)
#define FPF2283_OVP_OFFSET_400MV                   (0x5)
#define FPF2283_OVP_OFFSET_600MV                   (0x6)
#define FPF2283_OVP_OFFSET_800MV                   (0x7)

#define FPF2283_OVP_CENTER_VALUE_6800MV            (0x0)
#define FPF2283_OVP_CENTER_VALUE_11500MV           (0x1)
#define FPF2283_OVP_CENTER_VALUE_17000MV           (0x2)
#define FPF2283_OVP_CENTER_VALUE_23000MV           (0x3)

/* Isource to VIN reg=0x06, default=0x00, RW */
#define FPF2283_ISRC_AMPLITUDE_REG                 0x06

#define FPF2283_ISRC_AMPLITUDE_MASK                (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define FPF2283_ISRC_AMPLITUDE_SHIFT               (0)

#define FPF2283_ISRC_AMPLITUDE_0UA                 (0x0)
#define FPF2283_ISRC_AMPLITUDE_1UA                 (0x1)
#define FPF2283_ISRC_AMPLITUDE_2UA                 (0x2)
#define FPF2283_ISRC_AMPLITUDE_3UA                 (0x3)
#define FPF2283_ISRC_AMPLITUDE_4UA                 (0x4)
#define FPF2283_ISRC_AMPLITUDE_5UA                 (0x5)
#define FPF2283_ISRC_AMPLITUDE_10UA                (0x6)
#define FPF2283_ISRC_AMPLITUDE_20UA                (0x7)
#define FPF2283_ISRC_AMPLITUDE_50UA                (0x8)
#define FPF2283_ISRC_AMPLITUDE_100UA               (0x9)
#define FPF2283_ISRC_AMPLITUDE_200UA               (0xa)
#define FPF2283_ISRC_AMPLITUDE_500UA               (0xb)
#define FPF2283_ISRC_AMPLITUDE_1000UA              (0xc)
#define FPF2283_ISRC_AMPLITUDE_2000UA              (0xd)
#define FPF2283_ISRC_AMPLITUDE_5000UA              (0xe)
#define FPF2283_ISRC_AMPLITUDE_10000UA             (0xf)

/* Isource for work time reg=0x07, default=0x00, RW */
#define FPF2283_ISRC_PULSE_REG                     0x07

#define FPF2283_ISRC_PULSE_TDET_MASK               (BIT(7) | BIT(6) | BIT(5) | BIT(4))
#define FPF2283_ISRC_PULSE_TDET_SHIFT              (4)
#define FPF2283_ISRC_PULSE_TBLK_MASK               (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define FPF2283_ISRC_PULSE_TBLK_SHIFT              (0)

#define FPF2283_ISRC_PULSE_TBLK_SINGLE_PULSE       (0x0)
#define FPF2283_ISRC_PULSE_TBLK_10MS               (0x1)
#define FPF2283_ISRC_PULSE_TBLK_20MS               (0x2)
#define FPF2283_ISRC_PULSE_TBLK_50MS               (0x3)
#define FPF2283_ISRC_PULSE_TBLK_100MS              (0x4)
#define FPF2283_ISRC_PULSE_TBLK_200MS              (0x5)
#define FPF2283_ISRC_PULSE_TBLK_500MS              (0x6)
#define FPF2283_ISRC_PULSE_TBLK_1S                 (0x7)
#define FPF2283_ISRC_PULSE_TBLK_2S                 (0x8)
#define FPF2283_ISRC_PULSE_TBLK_3S                 (0x9)
#define FPF2283_ISRC_PULSE_TBLK_6S                 (0xa)
#define FPF2283_ISRC_PULSE_TBLK_12S                (0xb)
#define FPF2283_ISRC_PULSE_TBLK_30S                (0xc)
#define FPF2283_ISRC_PULSE_TBLK_60S                (0xd)
#define FPF2283_ISRC_PULSE_TBLK_120S               (0xd)
#define FPF2283_ISRC_PULSE_TBLK_300S               (0xf)

#define FPF2283_ISRC_PULSE_TDET_200US              (0x0)
#define FPF2283_ISRC_PULSE_TDET_400US              (0x1)
#define FPF2283_ISRC_PULSE_TDET_1MS                (0x2)
#define FPF2283_ISRC_PULSE_TDET_2MS                (0x3)
#define FPF2283_ISRC_PULSE_TDET_4MS                (0x4)
#define FPF2283_ISRC_PULSE_TDET_10MS               (0x5)
#define FPF2283_ISRC_PULSE_TDET_20MS               (0x6)
#define FPF2283_ISRC_PULSE_TDET_40MS               (0x7)
#define FPF2283_ISRC_PULSE_TDET_100MS              (0x8)
#define FPF2283_ISRC_PULSE_TDET_200MS              (0x9)
#define FPF2283_ISRC_PULSE_TDET_400MS              (0xa)
#define FPF2283_ISRC_PULSE_TDET_1S                 (0xb)
#define FPF2283_ISRC_PULSE_TDET_2S                 (0xc)
#define FPF2283_ISRC_PULSE_TDET_4S                 (0xd)
#define FPF2283_ISRC_PULSE_TDET_10S                (0xd)
#define FPF2283_ISRC_PULSE_TDET_ALWAYS_ON          (0xf)

/* Voltage on VIN reg=0x08, default=0x00, LSB=8MV, R */
#define FPF2283_VOL_ON_VIN_REG                     0x08

#define FPF2283_VOL_ON_VIN_STEP                    (8) /* mv */

/* Set Tag of VIN reg=0x09, default=0xff , RW */
#define FPF2283_TAG_OF_VIN_REG                     0x09

/* register count */
#define FPF2283_MAX_REGS                           (10)

#define FPF2283_CHIP_ENABLE                        (1)
#define FPF2283_CHIP_DISABLE                       (0)

#define FPF2283_INIT_FINISH                        (1)
#define FPF2283_NOT_INIT                           (0)
#define FPF2283_ENABLE_INTERRUPT_NOTIFY            (1)
#define FPF2283_DISABLE_INTERRUPT_NOTIFY           (0)

#define FPF2283_NOT_USED                           (0)
#define FPF2283_USED                               (1)
#define FPF2283_DEVICE_ID_GET_FAIL                 (-1)

struct fpf2283_device_info {
	struct device *dev;
	struct i2c_client *client;
	struct work_struct irq_work;
	struct nty_data nty_data;
	int gpio_int;
	int irq_int;
	int irq_active;
	int gpio_en;
	int chip_already_init;
	int device_id;
};

#endif /* end of _FPF2283_H_ */
