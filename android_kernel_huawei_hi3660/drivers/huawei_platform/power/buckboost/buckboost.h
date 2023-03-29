/*
 * Copyright (C) 2012-2015 HUAWEI
 * Author:  L.JH HW
 */

#ifndef _BUCKBOOST_H
#define _BUCKBOST_H

#include <linux/i2c.h>		/*for struct bq2419x_device_info */
#include <linux/device.h>	/*for struct bq2419x_device_info */
#include <linux/workqueue.h>	/*for struct bq2419x_device_info */

#ifndef BIT
#define BIT(x)		(1 << (x))
#endif



/*************************struct define area***************************/
struct param {
	int bat_comp;
	int vclamp;
	int fcp_support;
};
struct max77813_device_info {
	struct i2c_client *client;
	struct device *dev;
	struct work_struct irq_work;
	struct param param_dts;
	int gpio_pok;
	int irq_pok;
};
/*************************marco define area***************************/
#define MAX77813_INFO_REG00  0x00
#define MAX77813_STATUS_REG01 0x01
#define MAX77813_FORCED_PWM_SHIFT 0
#define MAX77813_FORCED_PWM_MASK BIT(0)
#define MAX77813_CONFIG1 0x02

int max77813_forced_pwm_enable(int enable);
#endif
