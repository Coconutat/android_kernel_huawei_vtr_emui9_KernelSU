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

static int use_ovp_cutoff_wired_channel = 0;
static int gpio_ovp_chsw_en = 0;
static int ovp_gpio_initialized = 0;
static int wired_channel_status = WIRED_CHANNEL_RESTORE;

#define HWLOG_TAG ovp_channel_switch
HWLOG_REGIST();

static int ovp_chsw_get_wired_channel(void)
{
	return wired_channel_status;
}

static int ovp_chsw_set_wired_channel(int flag)
{
	int ret = 0;

	if (!ovp_gpio_initialized) {
		hwlog_err("error: ovp channel switch not initialized!\n");
		return -ENODEV;
	}

	if (WIRED_CHANNEL_CUTOFF == flag) {
		ret = gpio_direction_output(gpio_ovp_chsw_en, 1); /* cutoff */
	}
	else {
		ret = gpio_direction_input(gpio_ovp_chsw_en); /* restore */
	}
	hwlog_info("ovp channel switch set en(%d)\n", (WIRED_CHANNEL_CUTOFF == flag) ? 1 : 0);

	wired_channel_status = flag;

	return ret;
}

static struct wired_chsw_device_ops chsw_ops = {
	.get_wired_channel = ovp_chsw_get_wired_channel,
	.set_wired_channel = ovp_chsw_set_wired_channel,
};

static void ovp_chsw_parse_dts(struct device_node *np)
{
	int ret = 0;

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,wired_channel_switch"),
			"use_ovp_cutoff_wired_channel", &use_ovp_cutoff_wired_channel);
	if (ret) {
		hwlog_err("error: use_ovp_cutoff_wired_channel dts read failed!\n");
		use_ovp_cutoff_wired_channel = 0;
	}

	hwlog_info("use_ovp_cutoff_wired_channel=%d\n", use_ovp_cutoff_wired_channel);
}
static int ovp_chsw_gpio_init(struct device_node *np)
{
	int ret = 0;

	gpio_ovp_chsw_en = of_get_named_gpio(np, "gpio_ovp_chsw_en", 0);
	hwlog_info("gpio_ovp_chsw_en=%d\n", gpio_ovp_chsw_en);

	if (!gpio_is_valid(gpio_ovp_chsw_en)) {
		hwlog_err("error: gpio(gpio_ovp_chsw_en) is not valid!\n");
		return -EINVAL;
	}

	if (gpio_request(gpio_ovp_chsw_en, "gpio_ovp_chsw_en")) {
		hwlog_err("error: gpio(gpio_ovp_chsw_en) request fail!\n");
		return  -ENOMEM;
	}

	ret = gpio_direction_input(gpio_ovp_chsw_en); /* avoid ovp_en to hiz mode */
	if (ret) {
		hwlog_err("error: gpio(gpio_ovp_chsw_en) set input fail!\n");
		gpio_free(gpio_ovp_chsw_en);
		return -1;
	}

	ovp_gpio_initialized = 1;

	return 0;
}

static int ovp_chsw_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = (&pdev->dev)->of_node;

	hwlog_info("probe begin\n");

	ovp_chsw_parse_dts(np);

	if (use_ovp_cutoff_wired_channel) {
		ret = ovp_chsw_gpio_init(np);
		if (ret) {
			return -1;
		}

		ret = wired_chsw_ops_register(&chsw_ops);
		if (ret) {
			hwlog_err("error: ovp channel switch ops register fail!\n");
			gpio_free(gpio_ovp_chsw_en);
			return -1;
		}

		hwlog_info("ovp channel switch ops register success\n");
	}

	hwlog_info("probe end\n");
	return 0;
}

static int ovp_chsw_remove(struct platform_device *pdev)
{
	hwlog_info("remove begin\n");

	if(!gpio_is_valid(gpio_ovp_chsw_en))
		gpio_free(gpio_ovp_chsw_en);

	hwlog_info("remove end\n");
	return 0;
}

static struct of_device_id ovp_chsw_match_table[] = {
	{
		.compatible = "huawei,ovp_channel_switch",
		.data = NULL,
	},
	{},
};

static struct platform_driver ovp_chsw_driver = {
	.probe = ovp_chsw_probe,
	.remove = ovp_chsw_remove,
	.driver = {
		.name = "huawei,ovp_channel_switch",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ovp_chsw_match_table),
	},
};
static int __init ovp_chsw_init(void)
{
	return platform_driver_register(&ovp_chsw_driver);
}
static void __exit ovp_chsw_exit(void)
{
	platform_driver_unregister(&ovp_chsw_driver);
}

fs_initcall_sync(ovp_chsw_init);
module_exit(ovp_chsw_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ovp switch module driver");
MODULE_AUTHOR("HUAWEI Inc");
