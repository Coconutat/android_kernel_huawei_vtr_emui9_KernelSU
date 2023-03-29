#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/mutex.h>

#include "huawei_platform/audio/usb_analog_hs_interface.h"

#define LOG_TAG "usb_analog_headset_interface"

#define PRINT_INFO  1
#define PRINT_WARN  1
#define PRINT_DEBUG 0
#define PRINT_ERR   1

#if PRINT_INFO
#define logi(fmt, ...) printk(LOG_TAG"[I]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define logi(fmt, ...)
#endif

#if PRINT_WARN
#define logw(fmt, ...) printk(LOG_TAG"[W]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define logw(fmt, ...)
#endif

#if PRINT_DEBUG
#define logd(fmt, ...) printk(LOG_TAG"[D]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define logd(fmt, ...)
#endif

#if PRINT_ERR
#define loge(fmt, ...) printk(LOG_TAG"[E]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define loge(fmt, ...)
#endif

struct usb_analog_hs_ops *g_usb_ana_hs_ops = NULL;

bool check_usb_analog_hs_support(void)
{
	if (NULL == g_usb_ana_hs_ops)
		return false;

	if(NULL == g_usb_ana_hs_ops->check_usb_ana_hs_support)
		return false;

	return g_usb_ana_hs_ops->check_usb_ana_hs_support();
}

int usb_analog_hs_dev_register(struct usb_analog_hs_dev *dev, void *codec_data)
{
	if (NULL == g_usb_ana_hs_ops)
		return -ENODEV;

	if(NULL == g_usb_ana_hs_ops->usb_ana_hs_dev_register)
		return -ENODEV;

	return g_usb_ana_hs_ops->usb_ana_hs_dev_register(dev, codec_data);
}

int usb_analog_hs_check_headset_pluged_in(void)
{
	if (NULL == g_usb_ana_hs_ops)
		return USB_ANA_HS_PLUG_OUT;

	if(NULL == g_usb_ana_hs_ops->usb_ana_hs_check_headset_pluged_in)
		return USB_ANA_HS_PLUG_OUT;

	return g_usb_ana_hs_ops->usb_ana_hs_check_headset_pluged_in();
}

void usb_analog_hs_plug_in_out_handle(int hs_state)
{
	if (NULL == g_usb_ana_hs_ops)
		return;

	if(NULL == g_usb_ana_hs_ops->usb_ana_hs_plug_in_out_handle)
		return;

	g_usb_ana_hs_ops->usb_ana_hs_plug_in_out_handle(hs_state);
}

void usb_ana_hs_mic_swtich_change_state(void)
{
	if (NULL == g_usb_ana_hs_ops)
		return;

	if(NULL == g_usb_ana_hs_ops->usb_ana_hs_mic_swtich_change_state)
		return;

	g_usb_ana_hs_ops->usb_ana_hs_mic_swtich_change_state();
}

/**********************************************************
*  Function:       usb_analog_hs_ops_register
*  Discription:    register the handler ops for inner usb ana hs moudle
*  Parameters:   ops:operations interface of usb ana hs swtich device
*  return value:  0-sucess or others-fail
**********************************************************/
int usb_analog_hs_ops_register(struct usb_analog_hs_ops *ops)
{
	if (ops != NULL) {
		g_usb_ana_hs_ops = ops;
		return 0;
	} else {
		loge("anc_hs interface ops register fail!\n");
		return -ENODEV;
	}
}

static int usb_analog_hs_interface_probe(struct platform_device *pdev)
{
	return 0;
}

static int usb_analog_hs_interface_remove(struct platform_device *pdev)
{
	return 0;
}

static struct of_device_id usb_analog_hs_interface_match_table[] = {
	{
		.compatible = "huawei,usb_analog_hs_interface",
		.data = NULL,
	},
	{
	},
};

static struct platform_driver usb_analog_hs_interface_driver = {
	.probe = usb_analog_hs_interface_probe,
	.remove = usb_analog_hs_interface_remove,

	.driver = {
		.name = "huawei,usb_analog_hs_interface",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(usb_analog_hs_interface_match_table),
	},
};

/**********************************************************
*  Function:       anc_hs_interface_init
*  Discription:    anc_hs module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init usb_analog_hs_interface_init(void)
{
	return platform_driver_register(&usb_analog_hs_interface_driver);
}
/**********************************************************
*  Function:       anc_hs_interface_exit
*  Discription:    anc_hs module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit usb_analog_hs_interface_exit(void)
{
	platform_driver_unregister(&usb_analog_hs_interface_driver);
}

subsys_initcall(usb_analog_hs_interface_init);
module_exit(usb_analog_hs_interface_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("usb analog interface hs module driver");
MODULE_AUTHOR("HUAWEI Inc");
