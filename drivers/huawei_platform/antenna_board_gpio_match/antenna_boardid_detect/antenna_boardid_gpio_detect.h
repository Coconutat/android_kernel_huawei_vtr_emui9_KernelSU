/*
 * drivers/antenna_boardid_detect/antenna_boardid_gpio_detect.h
 *
 * huawei antenna boardid gpio detect driver
 *
*/

#ifndef _ANTENNA_BOARDID_GPIO_DETECT
#define _ANTENNA_BOARDID_GPIO_DETECT
#include <linux/device.h>



struct antenna_device_info {
    struct device *dev;
};

enum antenna_type {
    ANTENNA_BOARDID_GPIO_DETECT,
    ANTENNA_BOARDID_GPIO_STATUS,
};

#define ANATENNA_DETECT_SUCCEED 1
#define ANATENNA_DETECT_FAIL    0
#endif
