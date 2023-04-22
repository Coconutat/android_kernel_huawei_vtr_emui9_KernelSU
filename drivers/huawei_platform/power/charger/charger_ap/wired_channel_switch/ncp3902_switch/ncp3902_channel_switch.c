#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>

#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/wired_channel_switch.h>

static int use_wireless_switch_cutoff_wired_channel = 0;
static int gpio_chgsw_en = 0;
static int gpio_chgsw_flag_n = 0;
static int wired_channel_status = WIRED_CHANNEL_RESTORE;

#define HWLOG_TAG ncp3902_channel_switch
HWLOG_REGIST();

static int ncp3902_chgsw_get_wired_channel(void)
{
	return wired_channel_status;
}

static int ncp3902_chgsw_set_wired_channel(int flag)
{
	int gpio_val;

	if (!use_wireless_switch_cutoff_wired_channel || !gpio_chgsw_en) {
		hwlog_err("error: ncp3902 channel switch not initialized!\n");
		return -ENODEV;
	}

	gpio_val = (WIRED_CHANNEL_CUTOFF == flag) ? 1 : 0; /* 1:cutoff, 0:restore */
	gpio_set_value(gpio_chgsw_en, gpio_val);
	hwlog_info("ncp3902 channel switch set en(%d)\n", gpio_val);

	wired_channel_status = flag;

	return 0;
}
static int ncp3902_chgsw_set_wired_reverse_channel(int flag)
{
	int wired_channel_flag, gpio_val;

	if (!gpio_chgsw_flag_n) {
		hwlog_err("error: ncp3902 channel switch not initialized!\n");
		return -ENODEV;
	}

	wired_channel_flag =
		(WIRED_REVERSE_CHANNEL_CUTOFF == flag) ? WIRED_CHANNEL_CUTOFF : WIRED_CHANNEL_RESTORE;
	gpio_val = (WIRED_REVERSE_CHANNEL_CUTOFF == flag) ? 0 : 1; /* 1: mos on  0:mos off */
	ncp3902_chgsw_set_wired_channel(wired_channel_flag);
	gpio_set_value(gpio_chgsw_flag_n, gpio_val);
	hwlog_info("ncp3902 channel switch set flag_n(%d:%s)\n",
		gpio_val, (gpio_val == 0) ? "high" : "low");

	return 0;
}
static struct wired_chsw_device_ops chsw_ops = {
	.set_wired_channel = ncp3902_chgsw_set_wired_channel,
	.get_wired_channel = ncp3902_chgsw_get_wired_channel,
	.set_wired_reverse_channel = ncp3902_chgsw_set_wired_reverse_channel,
};
static void ncp3902_chgsw_parse_dts(struct device_node *np)
{
	int ret = 0;

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,wired_channel_switch"),
			"use_wireless_switch_cutoff_wired_channel", &use_wireless_switch_cutoff_wired_channel);
	if (ret) {
		hwlog_err("error: use_wireless_switch_cutoff_wired_channel dts read failed!\n");
		use_wireless_switch_cutoff_wired_channel = 0;
	}

	hwlog_info("use_wireless_switch_cutoff_wired_channel=%d\n", use_wireless_switch_cutoff_wired_channel);
}
static int ncp3902_chgsw_gpio_init(struct device_node *np)
{
	gpio_chgsw_en = of_get_named_gpio(np, "gpio_chgsw_en", 0);
	hwlog_info("gpio_chgsw_en=%d\n", gpio_chgsw_en);

	if (!gpio_is_valid(gpio_chgsw_en)) {
		hwlog_err("error: gpio(gpio_chgsw_en) is not valid!\n");
		return -EINVAL;
	}

	if (gpio_request(gpio_chgsw_en, "gpio_chgsw_en")) {
		hwlog_err("error: gpio(gpio_chgsw_en) request fail!\n");
		return  -ENOMEM;
	}

	gpio_direction_output(gpio_chgsw_en, 0); /* 0:enable 1:disable */

	gpio_chgsw_flag_n = of_get_named_gpio(np, "gpio_chgsw_flag_n", 0);
	hwlog_info("gpio_chgsw_flag_n=%d\n", gpio_chgsw_flag_n);

	if (!gpio_is_valid(gpio_chgsw_flag_n)) {
		hwlog_err("error: gpio(gpio_chgsw_flag_n) is not valid!\n");
		gpio_chgsw_flag_n = 0;
		return 0;
	}

	if (gpio_request(gpio_chgsw_flag_n, "gpio_chgsw_flag_n")) {
		hwlog_err("error: gpio(gpio_chgsw_flag_n) request fail!\n");
		return  -ENOMEM;
	}

	gpio_direction_output(gpio_chgsw_flag_n, 0); /*1:mos on  0:mos off */

	return 0;
}

static int ncp3902_chgsw_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = (&pdev->dev)->of_node;

	hwlog_info("probe begin\n");

	ncp3902_chgsw_parse_dts(np);

	if (use_wireless_switch_cutoff_wired_channel) {
		ret = ncp3902_chgsw_gpio_init(np);
		if (ret) {
			return -1;
		}

		ret = wired_chsw_ops_register(&chsw_ops);
		if (ret) {
			hwlog_err("error: ncp3902 channel switch ops register fail!\n");
			gpio_free(gpio_chgsw_en);
			return -1;
		}

		hwlog_info("ncp3902 channel switch ops register success\n");
	}

	hwlog_info("probe end\n");
	return 0;
}
static struct of_device_id ncp3902_chgsw_match_table[] = {
	{
		.compatible = "huawei,ncp3902_channel_switch",
		.data = NULL,
	},
	{},
};

static struct platform_driver ncp3902_chgsw_driver = {
	.probe = ncp3902_chgsw_probe,
	.driver = {
		.name = "huawei,ncp3902_channel_switch",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ncp3902_chgsw_match_table),
	},
};
static int __init ncp3902_chsw_init(void)
{
	return platform_driver_register(&ncp3902_chgsw_driver);
}
static void __exit ncp3902_chsw_exit(void)
{
	platform_driver_unregister(&ncp3902_chgsw_driver);
}

fs_initcall_sync(ncp3902_chsw_init);
module_exit(ncp3902_chsw_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ncp3902 switch module driver");
MODULE_AUTHOR("HUAWEI Inc");
