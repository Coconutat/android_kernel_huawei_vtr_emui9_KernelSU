/*
 * Copyright (C) 2016 HUAWEI
 * Author:  BiaoPeng HW
 */

#ifndef __FAN54151_H_
#define __FAN54151_H_

#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <huawei_platform/power/direct_charger.h>

#ifndef BIT
#define BIT(x)    (1 << (x))
#endif

struct fan54151_device_info {
	struct i2c_client *client;
	struct device *dev;
	struct work_struct irq_work;
	int gpio_reset;
	int gpio_int;
	int irq_int;
	int irq_active;
	int chip_already_init;
};
/*************************register part id****************************/
#define FAN54151_MAIN_REG_TOTAL_NUM   (8)
#define FAN54151_REG_NONE 0x00
#define FAN54151_REG_NONE_NONE_MASK  (0xFF)
#define FAN54151_REG_NONE_NONE_SHIFT (0x00)
#define BQ25892_REG_TOTAL_NUM   (21)
#define FAN54151_DEVICE_ID 0x00

/*************************register control0***************************/
#define FAN54151_CONTROL0 0x01

#define SWITCH_ENABLE 1
#define SWITCH_DISABLE 0
#define SWITCH_ENABLE_MASK BIT(0)
#define SWITCH_ENABLE_SHIFT 0

#define OCP_LIMIT_4600_MA 4600
#define OCP_LIMIT_4600_MA_REG 0x00
#define OCP_LIMIT_5400_MA 5400
#define OCP_LIMIT_5400_MA_REG 0x01
#define OCP_LIMIT_6200_MA 6200
#define OCP_LIMIT_6200_MA_REG 0x10
#define OCP_LIMIT_6900_MA 6900
#define OCP_LIMIT_6900_MA_REG 0x11
#define OCP_MASK (BIT(1) | BIT(2))
#define OCP_SHIFT 1

#define SOVP_MASK (BIT(6) | BIT(5) | BIT(4) | BIT(3))
#define SOVP_SHIFT 3
#define SOVP_LIMIT_4200_MV 4200
#define SOVP_LIMIT_4575_MV 4575
#define SOVP_STEP 25

#define FOVP_MASK (BIT(1) | BIT(0))
#define FOVP_SHIFT 0
#define FOVP_LIMIT_4700_MV 4700
#define FOVP_LIMIT_4850_MV 4850
#define FOVP_STEP 50

/*************************register control1***************************/
#define FAN54151_CONTROL1 0x02

/*************************register status*****************************/
#define FAN54151_STATUS 0x03

/*************************register protect enable*********************/
#define FAN54151_PROTECT_ENABLE 0x04
#define UVLO_ENABLE_MASK BIT(0)
#define UVLO_ENABLE_SHIFT 0

/*************************register FDS********************************/
#define FAN54151_FAULT_DETECTION_STATUS 0x05

/*************************register irq********************************/
#define FAN54151_IRQ 0x06

/*************************register irq mask***************************/
#define FAN54151_IRQ_MASK 0x07
#endif
