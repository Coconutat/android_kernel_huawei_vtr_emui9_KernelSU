/*lint -e528 -e529 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/wakelock.h>
#include <linux/ioctl.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#include "huawei_platform/audio/anc_hs_default.h"

#define HWLOG_TAG anc_hs_default

HWLOG_REGIST();

struct anc_hs_default_priv {
	struct device *dev;
	int headset_type;
};

struct anc_hs_default_priv *g_anc_hs_default_priv;

/*lint -save -e* */
void anc_hs_default_refresh_headset_type(int headset_type)
{
	if (NULL != g_anc_hs_default_priv) {
		g_anc_hs_default_priv->headset_type =  headset_type;
		hwlog_info("hs_default: refresh headset_type %d.", g_anc_hs_default_priv->headset_type);
	}
}
/*lint -restore*/

struct anc_hs_ops anc_hs_default_ops = {
	.anc_hs_dev_register = NULL,
	.anc_hs_check_headset_pluged_in = NULL,
	.anc_hs_start_charge = NULL,
	.anc_hs_charge_detect = NULL,
	.anc_hs_stop_charge = NULL,
	.anc_hs_force_charge = NULL,
	.check_anc_hs_support = NULL,
	.anc_hs_plug_enable = NULL,
	.anc_hs_5v_control = NULL,
	.anc_hs_invert_hs_control = NULL,
	.anc_hs_refresh_headset_type = anc_hs_default_refresh_headset_type,
};

/**
 * anc_hs_default_ioctl - ioctl interface for userspeace
 *
 * @file: file description
 * @cmd: control commond
 * @arg: arguments
 *
 * userspeace can get charge status and force control
 * charge status.
 **/
/*lint -save -e* */
static long anc_hs_default_ioctl(struct file *file, unsigned int cmd,
							   unsigned long arg)
{
	int ret = 0;
	int adc_value;
	unsigned int __user *p_user = (unsigned int __user *) arg;

	if (g_anc_hs_default_priv == NULL)
		return -ENODEV;

	switch (cmd) {
		case IOCTL_ANC_HS_GET_HEADSET_CMD:
			ret = put_user((__u32)(g_anc_hs_default_priv->headset_type),
						   p_user);
			break;
		case IOCTL_ANC_HS_GET_HEADSET_RESISTANCE_CMD:
			adc_value = 0;
			ret = put_user((__u32)(adc_value), p_user);
			break;
		default:
			hwlog_err("unsupport cmd\n");
			ret = -EINVAL;
			break;
	}

	return (long)ret;
}
/*lint -restore*/

/*lint -save -e* */
static const struct file_operations anc_hs_default_fops = {
	.owner			   = THIS_MODULE,
	.open				= simple_open,
	.unlocked_ioctl	  = anc_hs_default_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl		= anc_hs_default_ioctl,
#endif
};
/*lint -restore*/

/*lint -save -e* */
static struct miscdevice anc_hs_default_device = {
	.minor  = MISC_DYNAMIC_MINOR,
	.name   = "anc_hs",
	.fops   = &anc_hs_default_fops,
};
/*lint -restore*/

/**********************************************************
*  Function:       anc_hs_default_probe
*  Discription:    anc_hs_default module probe
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int anc_hs_default_probe(struct platform_device *pdev)
{
	int ret = -1;
	struct device *dev = &pdev->dev;

	hwlog_info("anc_hs_default_probe++\n");

	g_anc_hs_default_priv = devm_kzalloc(dev, sizeof(*g_anc_hs_default_priv), GFP_KERNEL);
	if (NULL == g_anc_hs_default_priv) {
		hwlog_err("cannot allocate anc hs default dev data\n");
		/*lint -save -e* */
		return -ENOMEM;
		/*lint -restore*/
	}

	g_anc_hs_default_priv->dev = dev;

	ret = anc_hs_ops_register(&anc_hs_default_ops);
	if (ret) {
		hwlog_err("register anc_hs_interface ops failed!\n");
		goto err_out;
	}

	ret = misc_register(&anc_hs_default_device);
	if (ret) {
		hwlog_err("%s: anc_hs_default misc device register failed",
				  __func__);
		goto err_out;
	}

	hwlog_info("anc_hs_default_probe--\n");

	return 0;

err_out:
	devm_kfree(g_anc_hs_default_priv->dev, g_anc_hs_default_priv);
	g_anc_hs_default_priv = NULL;
	return ret;
}

/**********************************************************
*  Function:       anc_hs_default_remove
*  Discription:    anc_hs_default module remove
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int anc_hs_default_remove(struct platform_device *pdev)
{
	devm_kfree(g_anc_hs_default_priv->dev, g_anc_hs_default_priv);
	g_anc_hs_default_priv = NULL;

	misc_deregister(&anc_hs_default_device);

	hwlog_info("%s: exit\n", __func__);

	return 0;
}

static struct of_device_id anc_hs_default_match_table[] = {
	{
		.compatible = "huawei,anc_hs_default",
		.data = NULL,
	},
	{
	},
};

/*lint -save -e* */
static struct platform_driver anc_hs_default_driver = {
	.probe = anc_hs_default_probe,
	.remove = anc_hs_default_remove,

	.driver = {
		.name = "huawei,anc_hs_default",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(anc_hs_default_match_table),
	},
};
/*lint -restore*/

/**********************************************************
*  Function:       anc_hs_default_init
*  Discription:    anc_hs_default module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init anc_hs_default_init(void)
{
	return platform_driver_register(&anc_hs_default_driver);
}
/**********************************************************
*  Function:       anc_hs_default_exit
*  Discription:    anc_hs_default module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit anc_hs_default_exit(void)
{
	platform_driver_unregister(&anc_hs_default_driver);
}

/*lint -save -e* */
device_initcall_sync(anc_hs_default_init);
module_exit(anc_hs_default_exit);
/*lint -restore*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("anc hs default module driver");
MODULE_AUTHOR("HUAWEI Inc");
