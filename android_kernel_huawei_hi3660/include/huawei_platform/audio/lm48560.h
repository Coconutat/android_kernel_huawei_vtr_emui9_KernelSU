/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _LM48560_H_
#define _LM48560_H_

#include <linux/regmap.h>

#define LM48560_SHUTDOWN_REG    0x00
#define LM48560_TURNON_NORMAL    0x00
#define LM48560_TURNON_FAST    0x08
#define LM48560_TURNON_SHIFT    0x03
#define LM48560_TURNON_MASK    0x01

#define LM48560_INSEL_INPUT1    0x00
#define LM48560_INSEL_INPUT2    0x04
#define LM48560_INSEL_SHIFT    0x02
#define LM48560_INSEL_MASK    0x01

#define LM48560_BOOST_DISABLED    0x00
#define LM48560_BOOST_ENABLED    0x02
#define LM48560_BOOST_SHIFT    0x01
#define LM48560_BOOST_MASK    0x01
#define LM48560_DEVICE_SHUTDOWN    0x00
#define LM48560_DEVICE_ENABLED    0x01

#define LM48560_NOCLIP_REG    0x01
#define LM48560_RLT_0P5S    0x00
#define LM48560_RLT_0P3S    0x20
#define LM48560_RLT_0P2S    0x40
#define LM48560_RLT_0P1S    0x60
#define LM48560_RLT_SHIFT    0x05
#define LM48560_RLT_MASK    0x03

#define LM48560_ATK_0P8S    0x00
#define LM48560_ATK_1P2S    0x08
#define LM48560_ATK_1P5S    0x10
#define LM48560_ATK_2P2S    0x18
#define LM48560_ATK_SHIFT    0x03
#define LM48560_ATK_MASK    0x03

#define LM48560_VLIM_DIS1    0x00
#define LM48560_VLIM_14VPP    0x01
#define LM48560_VLIM_17VPP    0x02
#define LM48560_VLIM_20VPP    0x03
#define LM48560_VLIM_22VPP    0x04
#define LM48560_VLIM_25VPP    0x05
#define LM48560_VLIM_28VPP    0x06
#define LM48560_VLIM_DIS2    0x07
#define LM48560_VLIM_MASK    0x07

#define LM48560_GAIN_REG        0x02
#define LM48560_GAIN_MASK    0x03
#define LM48560_BSTDIS_GAIN0DB  0x00
#define LM48560_BSTDIS_GAIN6DB  0x01
#define LM48560_BSTDIS_GAIN12DB 0x02
#define LM48560_BSTDIS_GAIN18DB 0x03
#define LM48560_BSTEN_GAIN21DB  0x00
#define LM48560_BSTEN_GAIN24DB  0x01
#define LM48560_BSTEN_GAIN27DB  0x02
#define LM48560_BSTEN_GAIN30DB  0x03

#define LM48560_REG_MIN_NUM    1
#define LM48560_REG_MAX_NUM    128
/* commands used for debug purpose */
#define LM48560_CMD_TYPE_IND    0
#define LM48560_CMD_REG_ADDR_IND    1
#define LM48560_CMD_REG_VAL1_IND    2
#define LM48560_CMD_LEN    2
/*commands set bit*/
#define LM48560_CMD_REG_MASK_IND    2
#define LM48560_CMD_REG_SETBIT_VAL1_IND    3
#define LM48560_CMD_SETBIT_LEN    4
/*commands shdn*/
#define LM48560_CMD_SHDN_STATUS_IND    1
/*commands types*/
#define LM48560_CMD_REG_WITE            1
#define LM48560_CMD_REG_READ            2
#define LM48560_CMD_REG_SETBIT          3
#define LM48560_CMD_SHDN                4
/* commands used for API call */
#define LM48560_CMD_START               5
#define LM48560_CMD_STOP                6

/* lm48560 status*/
#define LM48560_ON                      1
#define LM48560_OFF                     0

struct lm48560_data {
    struct device *dev;
    struct regmap *mpRegmap;
    struct mutex lock;
    struct mutex file_lock;
    int mnGPIOEN;
    int mnTurnOn;
    int mnInSel;
    int mnBstEn;
    int mnReleaseTime;
    int mnAttackTime;
    int mnVoltageLimiter;
    int mnGain;
    int mnCmd;
    int mnCurrentReg;
    int mbPowerUp;
};
extern int lm48560_power_status (void);
extern void lm48560_opt (unsigned int status);
#endif /* _LM48560_H_ */


