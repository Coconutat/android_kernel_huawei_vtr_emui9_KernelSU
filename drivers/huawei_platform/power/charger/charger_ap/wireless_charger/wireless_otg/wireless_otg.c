#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/wired_channel_switch.h>
#include <huawei_platform/power/wireless_otg.h>
#include <huawei_platform/power/wireless_charger.h>
#ifdef CONFIG_BOOST_5V
#include <huawei_platform/power/boost_5v.h>
#endif

static int gpio_otg_switch;
static int otg_channel = 0;
static bool otg_mode = NOT_IN_OTG_MODE;

#define HWLOG_TAG wireless_otg
HWLOG_REGIST();
int wireless_otg_get_mode(void)
{
	return otg_mode;
}

static void wireless_otg_start_config(int flag)
{
	hwlog_info("---->START OTG MODE, flag = %d\n", flag);
	otg_mode = IN_OTG_MODE;
	wired_chsw_set_wired_channel(WIRED_CHANNEL_CUTOFF);
	msleep(100);
#ifdef CONFIG_BOOST_5V
	boost_5v_enable(BOOST_5V_ENABLE, BOOST_CTRL_WIRELESS_OTG);
#endif
	gpio_set_value(gpio_otg_switch, OTG_SWITCH_ENABLE);
}
static void wireless_otg_stop_config(int flag)
{
	hwlog_info("---->STOP OTG MODE, flag = %d\n", flag);
	gpio_set_value(gpio_otg_switch, OTG_SWITCH_DISABLE);
#ifdef CONFIG_BOOST_5V
	boost_5v_enable(BOOST_5V_DISABLE, BOOST_CTRL_WIRELESS_OTG);
#endif
	if (flag)
		otg_mode = NOT_IN_OTG_MODE;
	if (WIRELESS_CHANNEL_OFF ==
		wireless_charge_get_wireless_channel_state()  && flag) {
		//wired_chsw_set_wired_channel(WIRED_CHANNEL_RESTORE);
       }
}
void wireless_otg_detach_handler(int flag)
{
	if (!otg_channel) {
		hwlog_info("%s use charger boost\n", __func__);
		return;
	}
	hwlog_info("case = USB_EVENT_NONE-> (IM)\n");
	wireless_otg_stop_config(flag);
}
void wireless_otg_attach_handler(int flag)
{
	if (!otg_channel) {
		hwlog_info("%s use charger boost\n", __func__);
		return;
	}
	hwlog_info("case = USB_EVENT_OTG_ID-> (IM)\n");
	wireless_otg_start_config(flag);
}
static int wireless_otg_gpio_init(struct device_node *np)
{
	int ret = 0;

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,pd_dpm"),
		"otg_channel", &otg_channel);
	if (ret) {
		hwlog_err("%s: get otg_channel failed\n", __func__);
		otg_channel = 0;
		return -1;
	}
	hwlog_info("%s: otg_channel  = %d\n", __func__, otg_channel);

	gpio_otg_switch = of_get_named_gpio(np, "gpio_otg_switch", 0);
	hwlog_info("%s: gpio_otg_switch = %d\n", __func__, gpio_otg_switch);
	if (!gpio_is_valid(gpio_otg_switch)) {
		hwlog_err("gpio_otg_switch is not valid\n");
		ret =  -EINVAL;
		goto gpio_init_fail_0;
	}
	ret = gpio_request(gpio_otg_switch, "gpio_otg_switch");
	if (ret) {
		hwlog_err("could not request gpio_otg_switch\n");
		ret =  -ENOMEM;
		goto gpio_init_fail_0;
	}
	gpio_direction_output(gpio_otg_switch, 0);
	return 0;

gpio_init_fail_0:
	otg_channel = 0;
	return ret;
}
static int wireless_otg_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = (&pdev->dev)->of_node;
	ret = wireless_otg_gpio_init(np);
	if (ret) {
		hwlog_info("%s otg gpio init fail\n", __func__);
		return -1;
	}

	hwlog_info("wireless_otg probe ok.\n");
	return 0;
}

static struct of_device_id wireless_otg_match_table[] = {
	{
	 .compatible = "huawei,wireless_otg",
	 .data = NULL,
	},
	{ },
};

static struct platform_driver wireless_otg_driver = {
	.probe = wireless_otg_probe,
	.driver = {
		.name = "huawei,wireless_otg",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(wireless_otg_match_table),
	},
};
static int __init wireless_otg_init(void)
{
	hwlog_info("wireless_otg init ok.\n");

	return platform_driver_register(&wireless_otg_driver);
}

static void __exit wireless_otg_exit(void)
{
	platform_driver_unregister(&wireless_otg_driver);
}

device_initcall_sync(wireless_otg_init);
module_exit(wireless_otg_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("wireless otg module driver");
MODULE_AUTHOR("HUAWEI Inc");
