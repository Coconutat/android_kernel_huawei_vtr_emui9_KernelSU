/*
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/notifier.h>
#include <linux/wakelock.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <linux/hisi/hisi_adc.h>
#include <linux/delay.h>
#include <huawei_platform/power/huawei_charger_sh.h>
#include <inputhub_route.h>
#include <inputhub_bridge.h>

#define HWLOG_TAG usb_short_circuit_protect
HWLOG_REGIST();

struct uscp_device_info_sh* g_device_uscp = NULL;

void dsm_uscp_report(uint32_t uscp_id)
{
	if (!power_dsm_get_dclient(POWER_DSM_USCP))
		return;

	if (!dsm_client_ocuppy(power_dsm_get_dclient(POWER_DSM_USCP))) {
                hwlog_info("sensorhub uscp record and notify\n");
                dsm_client_record(power_dsm_get_dclient(POWER_DSM_USCP), "sensorhub usb short happened!\n");
                dsm_client_notify(power_dsm_get_dclient(POWER_DSM_USCP), uscp_id);
	}
}

static int uscp_probe(struct platform_device *pdev)
{
    struct device_node* np;
    struct uscp_device_info_sh* di;
    int ret = 0;

    np = pdev->dev.of_node;
    if(NULL == np)
    {
        hwlog_err("np is NULL\n");
        return -1;
    }
    di = kzalloc(sizeof(*di), GFP_KERNEL);
    if (!di)
    {
        hwlog_err("di is NULL\n");
        return -ENOMEM;

    }
    g_device_uscp = di;

    di->gpio_uscp = of_get_named_gpio(np, "gpio_usb_short_circuit_protect",0);
    if (!gpio_is_valid(di->gpio_uscp))
    {
        hwlog_err("gpio_uscp is not valid\n");
        ret = -EINVAL;
        goto free_mem;
    }
    hwlog_info("gpio_uscp = %d\n", di->gpio_uscp);
    ret = of_property_read_u32(np, "adc_channel_uscp", &(di->adc_channel_uscp));
    if (ret)
    {
        hwlog_err("get adc_channel_uscp info fail!\n");
        ret = -EINVAL;
        goto free_mem;
    }
    hwlog_info("adc_channel_uscp = %d\n", di->adc_channel_uscp);
    ret = of_property_read_u32(np, "open_mosfet_temp", &(di->open_mosfet_temp));
    if (ret)
    {
        hwlog_err("get open_mosfet_temp info fail!\n");
        ret = -EINVAL;
        goto free_mem;
    }
    hwlog_info("open_mosfet_temp = %d\n", di->open_mosfet_temp);
    ret = of_property_read_u32(np, "close_mosfet_temp", &(di->close_mosfet_temp));
    if (ret)
    {
        hwlog_err("get close_mosfet_temp info fail!\n");
        ret = -EINVAL;
        goto free_mem;
    }
    hwlog_info("close_mosfet_temp = %d\n", di->close_mosfet_temp);
    ret = of_property_read_u32(np, "interval_switch_temp", &(di->interval_switch_temp));
    if (ret)
    {
        hwlog_err("get interval_switch_temp info fail!\n");
        ret = -EINVAL;
        goto free_mem;
    }
    hwlog_info("interval_switch_temp = %d\n", di->interval_switch_temp);

    hwlog_info("uscp sensorhub probe ok!\n");
    return 0;

free_mem:
    kfree(di);
    g_device_uscp = NULL;
    return ret;
}

static struct of_device_id uscp_match_table[] =
{
    {
        .compatible = "huawei,usb_short_circuit_protect_sensorhub",
        .data = NULL,
    },
    {
    },
};
static struct platform_driver uscp_driver = {
    .probe = uscp_probe,
    .driver = {
        .name = "huawei,usb_short_circuit_protect_sensorhub",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(uscp_match_table),
    },
};

static int __init uscp_init(void)
{
    return platform_driver_register(&uscp_driver);
}

device_initcall_sync(uscp_init);

static void __exit uscp_exit(void)
{
    platform_driver_unregister(&uscp_driver);
}

module_exit(uscp_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:uscp");
MODULE_AUTHOR("HUAWEI Inc");
