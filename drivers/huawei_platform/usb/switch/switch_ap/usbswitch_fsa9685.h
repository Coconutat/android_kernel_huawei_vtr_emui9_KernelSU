#ifndef _USBSWITCH_FSA9685_H_
#define _USBSWITCH_FSA9685_H_

#ifndef BIT
#define BIT(x)          (1 << (x))
#endif

/******************************************************************************
* Register addresses
******************************************************************************/

/* reg=0x0d, RW, timing setting 2 */
#define FSA9685_REG_TIMING_SET_2                         (0x0d)

#define FSA9685_REG_TIMING_SET_2_PHONEOFF_WAITTIME_MASK  (BIT(2) | BIT(1) | BIT(0))
#define FSA9685_REG_TIMING_SET_2_PHONEOFF_WAITTIME_SHIFT (0)

#define FSA9685_FM8_WAITTIME_50MS                        (0)
#define FSA9685_FM8_WAITTIME_100MS                       (1)
#define FSA9685_FM8_WAITTIME_150MS                       (2)
#define FSA9685_FM8_WAITTIME_200MS                       (3)
#define FSA9685_FM8_WAITTIME_250MS                       (4)
#define FSA9685_FM8_WAITTIME_300MS                       (5)
#define FSA9685_FM8_WAITTIME_350MS                       (6)
#define FSA9685_FM8_WAITTIME_400MS                       (7)

#endif /* end of _USBSWITCH_FSA9685_H_ */