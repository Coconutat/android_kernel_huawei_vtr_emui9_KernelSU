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
#ifdef CONFIG_WIRELESS_CHARGER
#include <huawei_platform/power/wireless_transmitter.h>
#endif

static struct wired_chsw_device_ops *g_chsw_ops = NULL;

#define HWLOG_TAG wired_channel_switch
HWLOG_REGIST();

int wired_chsw_ops_register(struct wired_chsw_device_ops *ops)
{
	if (NULL != ops && NULL == g_chsw_ops) {
		g_chsw_ops = ops;
		return 0;
	}
	else {
		hwlog_err("error: wired_chgsw ops register fail!\n");
		return -EPERM;
	}
}
int wired_chsw_get_wired_channel(void)
{
	if (!g_chsw_ops || !g_chsw_ops->get_wired_channel) {
		hwlog_err("error: g_chsw_ops or ops is null!\n");
		return WIRED_CHANNEL_RESTORE;
	}

	return g_chsw_ops->get_wired_channel();
}

int wired_chsw_set_wired_channel(int flag)
{
	int ret;
	int wired_channel_status;

	if (!g_chsw_ops || !g_chsw_ops->set_wired_channel) {
		hwlog_err("error: g_chsw_ops or ops is null!\n");
		return 0;
	}

	wired_channel_status = wired_chsw_get_wired_channel();

	if (flag == wired_channel_status) {
		hwlog_info("wired channel is already %s, return\n",
			WIRED_CHANNEL_RESTORE == wired_channel_status ? "on" : "off");
		return 0;
	}

#ifdef CONFIG_WIRELESS_CHARGER
	wireless_tx_cancel_work();
#endif

	ret = g_chsw_ops->set_wired_channel(flag);
	if (!ret) {
		wired_channel_status = wired_chsw_get_wired_channel();
		hwlog_info("wired_chsw is set to %s\n",
			WIRED_CHANNEL_RESTORE == wired_channel_status ? "channel_on" : "channel_off");
	}

#ifdef CONFIG_WIRELESS_CHARGER
	wireless_tx_start_check();
#endif

	return 0;
}
int wired_chsw_set_wired_reverse_channel(int flag)
{
	if (!g_chsw_ops || !g_chsw_ops->set_wired_reverse_channel) {
		hwlog_err("error: g_chsw_ops or ops is null!\n");
		return -1;
	}

	return g_chsw_ops->set_wired_reverse_channel(flag);
}
static int wired_chsw_check_ops(void)
{
	int ret = 0;

	if ((NULL == g_chsw_ops) || (NULL == g_chsw_ops->set_wired_channel)) {
		hwlog_err("error: g_chsw_ops ops is null!\n");
		ret = -EINVAL;
	}

	return ret;
}
static int wired_chsw_probe(struct platform_device *pdev)
{
	int ret;

	hwlog_info("probe begin\n");

	ret = wired_chsw_check_ops();
	if (ret) {
		return -1;
	}

	hwlog_info("probe end\n");
	return 0;
}
static struct of_device_id wired_chsw_match_table[] = {
	{
		.compatible = "huawei,wired_channel_switch",
		.data = NULL,
	},
	{},
};

static struct platform_driver wired_chsw_driver = {
	.probe = wired_chsw_probe,
	.driver = {
		.name = "huawei,wired_channel_switch",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(wired_chsw_match_table),
	},
};
static int __init wired_chsw_init(void)
{
	return platform_driver_register(&wired_chsw_driver);
}
static void __exit wired_chsw_exit(void)
{
	platform_driver_unregister(&wired_chsw_driver);
}

module_init(wired_chsw_init);
module_exit(wired_chsw_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("wired charge switch module driver");
MODULE_AUTHOR("HUAWEI Inc");
