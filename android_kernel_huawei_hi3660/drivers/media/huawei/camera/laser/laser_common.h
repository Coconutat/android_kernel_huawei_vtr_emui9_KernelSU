/*
 *  Hisilicon K3 SOC camera driver source file
 *
 *  Copyright (C) Huawei Technology Co., Ltd.
 *
 * Author:
 * Email:
 * Date:      2016-12-16
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LASER_COMMON_H_
#define _LASER_COMMON_H_

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <media/v4l2-subdev.h>
#include <media/huawei/laser_cfg.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include "cam_log.h"
#include "hwcam_intf.h"

#define LDO_IOVDD_1P8V              (1800000)
#define LDO_AVDD_2P85V              (2850000)
#define LDO_AVDD_2P8V               (2800000)
#define LDO_AVDD_3P3V               (3300000)
#define LDO_XSHUT_1P8V              (1800000)

#define LASER_SUPPLY_LDO_NO_USE         (0)

#define LDO_MAX                         (2)
#define LDO_ENABLE_NUMS                 (1)

typedef enum {
    XSHUT = 0,
    IOVDD,
    IO_MAX
} gpio_t;

/* laser controler struct define */
struct hw_laser_info {
    const char* product_name;
    const char* laser_name;
    unsigned int i2c_index;

    /* gpio: laser xshut, avdd */
    struct gpio laser_gpio[IO_MAX];
    int gpio_num;

    /* ldo: laser avdd */
    int ldo_num;
    struct regulator_bulk_data ldo[LDO_MAX];

    /* pin control config */
    struct pinctrl *pinctrl;
    struct pinctrl_state *pins_default;
    struct pinctrl_state *pins_idle;
};


typedef struct _tag_hw_laser_t
{
    struct v4l2_subdev subdev;
    struct platform_device *pdev;
    struct hw_laser_info *laser_info;
    struct mutex lock;
} hw_laser_t;

//for ldo config
typedef enum
{
    LDO_POWER_OFF = 0,
    LDO_POWER_ON
} power_ctrl_t;

typedef enum
{
    LDO_AVDD = 0,
    LDO_IOVDD,
    LDO_XSHUT,
} laser_ldo_t;

typedef struct
{
    laser_ldo_t ldo_t;//use as index
    char *supply_names;
    int config_val;
}laser_power_settings_t;

int laser_ldo_config(hw_laser_t *s,
                    laser_ldo_t ldo_t,
                    power_ctrl_t power_state,
                    const laser_power_settings_t *power_setting,
                    int power_setting_num);

#endif