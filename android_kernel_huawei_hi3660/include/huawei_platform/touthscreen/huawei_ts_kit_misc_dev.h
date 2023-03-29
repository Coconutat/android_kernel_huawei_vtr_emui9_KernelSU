/*
 * Huawei Touchpanel driver
 *
 * Copyright (C) 2013 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */

#ifndef __HUAWEI_TS_KIT_MISC_DEV_H_
#define __HUAWEI_TS_KIT_MISC_DEV_H_

#include <linux/ioctl.h>
#include <linux/miscdevice.h>

#define DEVICE_AFT_GET_INFO  "ts_aft_get_info"
#define DEVICE_AFT_SET_INFO  "ts_aft_set_info"
/* commands */
#define  INPUT_AFT_GET_IO_TYPE  (0xBA)
#define  INPUT_AFT_SET_IO_TYPE  (0xBB)

#define INPUT_AFT_IOCTL_CMD_GET_TS_FINGERS_INFO \
    _IOWR(INPUT_AFT_GET_IO_TYPE, 0x01, \
struct ts_fingers)

#define INPUT_AFT_IOCTL_CMD_GET_ALGO_PARAM_INFO \
    _IOWR(INPUT_AFT_GET_IO_TYPE, 0x02, \
struct ts_aft_algo_param)

#define INPUT_AFT_IOCTL_CMD_SET_COORDINATES \
    _IOWR(INPUT_AFT_SET_IO_TYPE, 0x01, \
    struct ts_fingers)

#endif

