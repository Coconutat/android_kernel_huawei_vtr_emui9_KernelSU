#ifndef _USBSWITCH_RT8979_H_
#define _USBSWITCH_RT8979_H_

#ifndef BIT
#define BIT(x)          (1 << (x))
#endif

/******************************************************************************
* Register addresses
******************************************************************************/

/* reg=0x0d, RW, muic timing setting 2 */
#define RT8979_REG_TIMING_SET_2                         (0x0d)

#define RT8979_REG_TIMING_SET_2_INTB_WATCHDOG_MASK      (BIT(7) | BIT(6))
#define RT8979_REG_TIMING_SET_2_INTB_WATCHDOG_SHIFT     (6)
#define RT8979_REG_TIMING_SET_2_DCD_TIMEOUT_SET_MASK    (BIT(5) | BIT(4))
#define RT8979_REG_TIMING_SET_2_DCD_TIMEOUT_SET_SHIFT   (4)
#define RT8979_REG_TIMING_SET_2_CHGDET_ONTIME_MASK      (BIT(3))
#define RT8979_REG_TIMING_SET_2_CHGDET_ONTIME_SHIFT     (3)
#define RT8979_REG_TIMING_SET_2_PHONEOFF_WAITTIME_MASK  (BIT(2) | BIT(1) | BIT(0))
#define RT8979_REG_TIMING_SET_2_PHONEOFF_WAITTIME_SHIFT (0)

#define RT8979_REG_TIMING_SET_2_DCDTIMEOUT              (RT8979_REG_TIMING_SET_2_DCD_TIMEOUT_SET_MASK)

#define RT8979_INTB_WATCHDOG_DISABLE                    (0)
#define RT8979_INTB_WATCHDOG_250MS                      (1)
#define RT8979_INTB_WATCHDOG_500MS                      (2)
#define RT8979_INTB_WATCHDOG_1000MS                     (3)

#define RT8979_DCD_TIMEOUT_SET_300MS                    (0)
#define RT8979_DCD_TIMEOUT_SET_600MS                    (1)
#define RT8979_DCD_TIMEOUT_SET_900MS                    (2)
#define RT8979_DCD_TIMEOUT_SET_1200MS                   (3)

#define RT8979_CHGDET_ONTIME_150MS                      (0)
#define RT8979_CHGDET_ONTIME_300MS                      (1)

#define RT8979_FM8_WAITTIME_50MS                        (0)
#define RT8979_FM8_WAITTIME_100MS                       (1)
#define RT8979_FM8_WAITTIME_150MS                       (2)
#define RT8979_FM8_WAITTIME_200MS                       (3)
#define RT8979_FM8_WAITTIME_250MS                       (4)
#define RT8979_FM8_WAITTIME_300MS                       (5)
#define RT8979_FM8_WAITTIME_350MS                       (6)
#define RT8979_FM8_WAITTIME_400MS                       (7)

/* reg=0x0e, RW, muic control 2 */
#define RT8979_REG_MUIC_CTRL                            (0x0e)

#define RT8979_REG_MUIC_CTRL_DISABLE_DCDTIMEOUT         (1 << 0)

/* reg=0x10, RW, muic control 3 */
#define RT8979_REG_MUIC_CTRL_3                          (0x10)

#define RT8979_REG_MUIC_CTRL_3_VBUS_PD_MASK             (BIT(7))
#define RT8979_REG_MUIC_CTRL_3_VBUS_PD_SHIFT            (7)
#define RT8979_REG_MUIC_CTRL_3_OVP_SEL_MASK             (BIT(5) | BIT(4))
#define RT8979_REG_MUIC_CTRL_3_OVP_SEL_SHIFT            (4)
#define RT8979_REG_MUIC_CTRL_3_ID_FLT_DEGLITCH_MASK     (BIT(3) | BIT(2))
#define RT8979_REG_MUIC_CTRL_3_ID_FLT_DEGLITCH_SHIFT    (2)

#define RT8979_REG_MUIC_CTRL_3_DISABLEID_FUNCTION       (0x28)

/* reg=0x11, RW, muic control 4 */
#define RT8979_REG_MUIC_CTRL_4                          (0x11)

#define RT8979_REG_MUIC_CTRL_4_IDFET_OCP_OFF_MASK       (BIT(4))
#define RT8979_REG_MUIC_CTRL_4_IDFET_OCP_OFF_SHIFT      (4)
#define RT8979_REG_MUIC_CTRL_4_SWEN_IDBAT1_MASK         (BIT(1))
#define RT8979_REG_MUIC_CTRL_4_SWEN_IDBAT1_SHIFT        (1)
#define RT8979_REG_MUIC_CTRL_4_SWEN_IDBAT2_MASK         (BIT(0))
#define RT8979_REG_MUIC_CTRL_4_SWEN_IDBAT2_SHIFT        (0)

#define RT8979_REG_MUIC_CTRL_4_ENABLEID2_FUNCTION       (0xFD)

/* reg=0x12, R, muic status 1 */
#define RT8979_REG_MUIC_STATUS1                         (0x12)

#define RT8979_REG_MUIC_STATUS1_FMEN_MASK               (BIT(5))
#define RT8979_REG_MUIC_STATUS1_FMEN_SHIFT              (5)
#define RT8979_REG_MUIC_STATUS1_CHGDET_MASK             (BIT(4))
#define RT8979_REG_MUIC_STATUS1_CHGDET_SHIFT            (4)
#define RT8979_REG_MUIC_STATUS1_DCDT_MASK               (BIT(3))
#define RT8979_REG_MUIC_STATUS1_DCDT_SHIFT              (3)
#define RT8979_REG_MUIC_STATUS1_VIN_UVLO_MASK           (BIT(2))
#define RT8979_REG_MUIC_STATUS1_VIN_UVLO_SHIFT          (2)
#define RT8979_REG_MUIC_STATUS1_OTP_MASK                (BIT(1))
#define RT8979_REG_MUIC_STATUS1_OTP_SHIFT               (1)
#define RT8979_REG_MUIC_STATUS1_VIN_OVP_MASK            (BIT(0))
#define RT8979_REG_MUIC_STATUS1_VIN_OVP_SHIFT           (0)

#define RT8979_REG_MUIC_STATUS1_FMEN                    (1 << 5)
#define RT8979_REG_MUIC_STATUS1_DCDT                    (1 << 3)

/* reg=0x13, R, muic status 2 */
#define RT8979_REG_MUIC_STATUS2                         (0x13)

#define RT8979_REG_MUIC_STATUS2_USB_STATE_MASK          (BIT(6) | BIT(5) | BIT(4))
#define RT8979_REG_MUIC_STATUS2_USB_STATE_SHIFT         (4)
#define RT8979_REG_MUIC_STATUS2_ID_STATUS_MASK          (BIT(1) | BIT(0))
#define RT8979_REG_MUIC_STATUS2_ID_STATUS_SHIFT         (0)

/* reg=0x18, R, adc value */
#define RT8979_REG_ADC                                  (0x18)

#define RT8979_REG_ADC_VALUE_MASK                       (BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define RT8979_REG_ADC_VALUE_SHIFT                      (0)

/* reg=0x40, RW, scp status */
#define RT8979_ACCP_STATUS_MASK                         (BIT(7) | BIT(6))
#define RT8979_ACCP_STATUS_SLAVE_GOOD                   (BIT(7) | BIT(6))

/* reg=0x44, RW, accp command for transmission */
#define RT8979_REG_ACCP_CMD_STAGE1                      (0x0B)

/* reg=0x47, RW, accp address for transmission */
#define RT8979_REG_ACCP_ADDR_VAL1                       (0x00)

/* reg=0x5b, R, accp interrupt mask 1 */
#define RT8979_REG_ALLMASK                              (0xFF)

/* reg=0xa0, R */
#define RT8979_REG_EXT3                                 (0xa0)

#define RT8979_REG_EXT3_VAL                             (0x38)

#define RT8979_REG_TEST_MODE                            (0xa0)

#define RT8979_REG_TEST_MODE_VAL1                       (0x38)
#define RT8979_REG_TEST_MODE_DEFAULT_VAL                (0x00)

/* reg=0xa4, R */
#define RT8979_REG_USBCHGEN                             (0xa4)

#define RT8979_REG_USBCHGEN_DETACH_STAGE1               (0x86)
#define RT8979_REG_USBCHGEN_DETACH_STAGE2               (0xc6)
#define RT8979_REG_USBCHGEN_ACCPDET_STAGE1              (1 << 6)

/* reg=0xa7, R */
#define RT8979_REG_MUIC_EXT2                            (0xa7)

#define RT8979_REG_QC_CTRL1                             (0xa7)

/* reg=0xa8, R */
#define RT8979_REG_QC_CTRL2                             (0xa8)

/* reg=0xab, R */
#define RT8979_REG_MUIC_EXT1                            (0xab)

/* reg=0xd0, R */
#define RT8979_REG_EFUSE_PRETRIM_DATA                   (0xd0)

#define RT8979_REG_EFUSE_PRETRIM_DATA_VAL               (0x08)

/* reg=0xd1, R */
#define RT8979_REG_EFUSE_CTRL                           (0xd1)

#define RT8979_REG_EFUSE_CTRL_VAL                       (0x0a)

/* reg=0xd2, R */
#define RT8979_REG_EFUSE_PRETRIM_ENABLE                 (0xd2)

#define RT8979_REG_EFUSE_PRETRIM_ENABLE_VAL             (0x20)
#define RT8979_REG_EFUSE_PRETRIM_ENABLE_VAL1            (0x40)

#define RT8979_WRITE_OSC_PRETRIM_DELAY_MIN_DEFAULT      (30)
#define RT8979_WRITE_OSC_PRETRIM_DELAY_MIN              (50)
#define RT8979_WRITE_OSC_PRETRIM_DELAY_MAX              (80)

/* reg=0xd3, default=0x00, R */
#define RT8979_REG_EFUSE_READ_DATA                      (0xd3)

/* other defined */
#define RT8979_FM8_MODE                                 (17)

#endif /* end of _USBSWITCH_RT8979_H_ */
