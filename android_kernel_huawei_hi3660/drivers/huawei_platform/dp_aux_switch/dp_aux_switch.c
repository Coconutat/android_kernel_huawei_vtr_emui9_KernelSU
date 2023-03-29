/* Copyright (c) 2008-2019, Huawei Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/ion.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/dp_aux_switch/dp_aux_switch.h>
#include <huawei_platform/audio/usb_analog_hs_interface.h>

#define HWLOG_TAG dp_aux_switch
HWLOG_REGIST();

static int g_dp_aux_gpio = -1;
static int g_dp_aux_uart_gpio = -1;

#define SET_GPIO_HIGH 1
#define SET_GPIO_LOW  0
#define DTS_DP_AUX_SWITCH "huawei,dp_aux_switch"

static bool g_aux_switch_from_fsa4476 = false;
static bool g_aux_switch_with_uart = false;
static uint32_t g_aux_ch_polarity = 0;

enum aux_switch_channel_type get_aux_switch_channel(void)
{
	if (g_aux_switch_from_fsa4476)
		return channel_fsa4476;
	else
		return channel_superswitch;
}

// aux polarity switch
void dp_aux_switch_op(uint32_t value)
{
	if (g_aux_switch_from_fsa4476) {
		g_aux_ch_polarity = value;
		return;
	}

	if (gpio_is_valid(g_dp_aux_gpio)) {
	        gpio_direction_output(g_dp_aux_gpio, value);
	} else {
	        printk(KERN_ERR "%s, Gpio is invalid:%d.\n", __func__, g_dp_aux_gpio);
	}
}

// NOTE: Don't pay attention to the name of the function!!!
// 1. aux-uart switch is existed: this func is switch aux or uart.
// 2. aux-uart switch is not existed: this func is dp aux enable or disable.
void dp_aux_uart_switch_enable(void)
{
	if (g_aux_switch_from_fsa4476) {
		// SBU bypass switch
		if (g_aux_ch_polarity) {
			// ENN L, EN1/EN2 01
			// SBU1 to SBU2_H, SBU2 to SBU1_H
			usb_analog_hs_plug_in_out_handle(DP_PLUG_IN_CROSS);
			printk(KERN_INFO "%s: dp plug in cross.\n", __func__);
		} else {
			// ENN L, EN1/EN2 00
			// SBU1 to SBU1_H, SBU2 to SBU2_H
			usb_analog_hs_plug_in_out_handle(DP_PLUG_IN);
			printk(KERN_INFO "%s: dp plug in.\n", __func__);
		}

		// 1. aux-uart switch is not existed.
		// 2. aux polarity switch of dp by fsa4476.
		// Then, return directly.
		if (!g_aux_switch_with_uart) {
			return;
		}
	}

	if (gpio_is_valid(g_dp_aux_uart_gpio)) {
	        gpio_direction_output(g_dp_aux_uart_gpio, SET_GPIO_HIGH);
	} else {
	        printk(KERN_ERR "%s, Gpio is invalid:%d.\n", __func__, g_dp_aux_uart_gpio);
	}
}

void dp_aux_uart_switch_disable(void)
{
	if (g_aux_switch_from_fsa4476) {
		// ENN H, EN1/EN2 00
		usb_analog_hs_plug_in_out_handle(DP_PLUG_OUT);
		printk(KERN_INFO "%s: dp plug out.\n", __func__);

		// aux-uart switch is not existed.
		if (!g_aux_switch_with_uart) {
			return;
		}
	}

	if (gpio_is_valid(g_dp_aux_uart_gpio)) {
	        gpio_direction_output(g_dp_aux_uart_gpio, SET_GPIO_LOW);
	} else {
	        printk(KERN_ERR "%s, Gpio is invalid:%d.\n", __func__, g_dp_aux_uart_gpio);
	}
}

static int dp_aux_switch_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;

	if (of_property_read_bool(dev->of_node, "aux_switch_with_uart")) {
		g_aux_switch_with_uart = true;
		hwlog_info("%s: aux switch with uart.\n", __func__);
	}

	if (g_aux_switch_with_uart) {
		g_dp_aux_uart_gpio = of_get_named_gpio(dev->of_node, "aux_uart-gpio", 0);
		if (!gpio_is_valid(g_dp_aux_uart_gpio)) {
			hwlog_err("%s: get aux_uart-gpio failed %d!\n", __func__, g_dp_aux_uart_gpio);
			g_dp_aux_uart_gpio = -1;
		} else {
			/*request aux uart gpio*/
			ret = gpio_request(g_dp_aux_uart_gpio, "dp_aux_uart_gpio");
			if (ret < 0) {
				hwlog_err("%s: request aux_uart-gpio failed %d!\n", __func__, ret);
				g_dp_aux_uart_gpio = -1;
			} else {
				gpio_direction_output(g_dp_aux_uart_gpio, SET_GPIO_LOW);
				hwlog_info("%s: init aux_uart-gpio success %d.\n", __func__, g_dp_aux_uart_gpio);
			}
		}
	}

	return 0;
}

static int dp_aux_switch_remove(struct platform_device *pdev)
{
	if (g_aux_switch_with_uart) {
		if (gpio_is_valid(g_dp_aux_uart_gpio)) {
			gpio_free((unsigned)g_dp_aux_uart_gpio);
			g_dp_aux_uart_gpio = -1;
		}
	}
	return 0;
}

static const struct of_device_id dp_aux_switch_match[] = {
	{ .compatible = "huawei,dp_aux_switch", },
	{},
};

static struct platform_driver dp_aux_switch_driver = {
	.driver = {
		.name  = "dp_aux_switch",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(dp_aux_switch_match),
	},
	.probe  = dp_aux_switch_probe,
	.remove = dp_aux_switch_remove,
};

static int __init dp_aux_switch_init(void)
{
	int ret = 0;
	struct device_node *np = NULL;

	hwlog_info("%s: enter...\n", __func__);
	ret = platform_driver_register(&dp_aux_switch_driver);
	if (ret < 0) {
		printk("%s: register dp_aux_switch_driver failed!\n", __func__);
		goto err_return;
	}

	np = of_find_compatible_node(NULL, NULL, DTS_DP_AUX_SWITCH);
	if (!np) {
		printk(KERN_ERR "NOT FOUND device node %s!\n", DTS_DP_AUX_SWITCH);
		goto err_return;
	}

	if (!of_device_is_available(np)) {
		hwlog_info("%s: dts node %s not available.\n", __func__, np->name);
		return 0;
	}

	if (of_property_read_bool(np, "aux_switch_from_fsa4476")) {
		g_aux_switch_from_fsa4476 = true;
		printk(KERN_INFO "%s: aux switch from fsa4476\n", __func__);
		return 0;
	}

	g_dp_aux_gpio = of_get_named_gpio(np, "cs-gpios", 0);
	g_dp_aux_uart_gpio = of_get_named_gpio(np, "cs-gpios", 1);
	hwlog_info("%s: get aux gpio %d, %d.\n", __func__, g_dp_aux_gpio, g_dp_aux_uart_gpio);

	if (!gpio_is_valid(g_dp_aux_gpio)) {
		printk(KERN_ERR "%s, Gpio is invalid:%d.\n", __func__, g_dp_aux_gpio);
		return 0;
	} else {
		/*request aux gpio*/
		ret = gpio_request(g_dp_aux_gpio, "dp_aux_gpio");
		if (ret < 0) {
			printk(KERN_ERR "%s, Fail to request gpio:%d. ret = %d\n", __func__, g_dp_aux_gpio, ret);
			// NOTE:
			// For blanc, here gpio_request failed, because of gpio requested by fsa4476.
			// But, g_dp_aux_uart_gpio need gpio_request, so don't goto return.
			//goto err_return;
		}
		/*set aux gpio output low*/
		gpio_direction_output(g_dp_aux_gpio, SET_GPIO_LOW);
	}

	if (!gpio_is_valid(g_dp_aux_uart_gpio)) {
		printk(KERN_ERR "%s, Gpio is invalid:%d.\n", __func__, g_dp_aux_uart_gpio);
		return 0;
	} else {
		/*request aux uart gpio*/
		ret = gpio_request(g_dp_aux_uart_gpio, "dp_aux_uart_gpio");
		if (ret < 0) {
			printk(KERN_ERR "%s, Fail to request gpio:%d. ret = %d\n", __func__, g_dp_aux_uart_gpio, ret);
			goto err_return;
		}
		gpio_direction_output(g_dp_aux_uart_gpio, SET_GPIO_LOW);
	}

	/*set aux uart gpio output low*/
	printk(KERN_INFO "%s: sucess %d\n", __func__, ret);

err_return:
	return ret;
}

static void __exit dp_aux_switch_exit(void)
{
	hwlog_info("%s: enter...\n", __func__);
	platform_driver_unregister(&dp_aux_switch_driver);

	if (gpio_is_valid(g_dp_aux_gpio)) {
		gpio_free((unsigned)g_dp_aux_gpio);
		g_dp_aux_gpio = -1;
	}

	if (gpio_is_valid(g_dp_aux_uart_gpio)) {
		gpio_free((unsigned)g_dp_aux_uart_gpio);
		g_dp_aux_uart_gpio = -1;
	}
}

module_init(dp_aux_switch_init);
module_exit(dp_aux_switch_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Huawei dp aux driver");
MODULE_AUTHOR("<wangping48@huawei.com>");

