#ifndef __HW_MOISTRUE_H__
#define __HW_MOISTRUE_H__
#define BIT(n) (0x01 << n)
#define BITS_PER_BYTE 8
#define TEN 10
#define FUSB3601_VBUS_VOLTAGE_LSB_IN_ONE_TENTH_MV 125
#define FUSB3601_VBUS_VOLTAGEL 0x70
#define FUSB3601_VBUS_VOLTAGEH 0x71
#define FUSB3601_VBUS_VOLTAGEH_VAL_MASK (BIT(1)|BIT(0))
#define FUSB3601_CCSTAT   0x1d
#define FUSB3601_CC1_STAT_MASK   (BIT(1) | BIT(0))
#define FUSB3601_CC1_OPEN   0
#define FUSB3601_CC2_STAT_MASK   (BIT(3) | BIT(2))
#define FUSB3601_CC2_OPEN   0
#define FUSB3601_PWRSTAT   0x1e
#define FUSB3601_VBUS_VAL_MASK   BIT(2)
#define FUSB3601_VBUS_NOT_CONNECTED  0
#define FUSB3601_MUS_CONTROL2 0xd2
#define FUSB3601_MUS_H2O_DET_EN BIT(2)
#define MOISTURE_DETECTED_THRESHOLD 1000
#define MOISTURE_DETECTED_CNT_THRESHOLD 1

/******************************************************************************
* fcp definitions  end
******************************************************************************/

void moisture_detection_init(void);
void moisture_detection_complete(void);
void start_moisture_detection(void);
#endif
