#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/wakelock.h>
#include <linux/miscdevice.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <sound/jack.h>
#include <linux/fs.h>
#include <linux/regmap.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <huawei_platform/usb/hw_pd_dev.h>

#include "huawei_platform/audio/usb_audio_common.h"

#define HWLOG_TAG usb_audio_common
HWLOG_REGIST();

struct usb_audio_common_data {
	unsigned int usb_typec_plugin;
	struct notifier_block usb_device_notifier;
	struct mutex notifier_lock;
};

static struct usb_audio_common_data *pdata;

static const struct of_device_id usb_audio_common_of_match[] = {
	{
		.compatible = "huawei,usb_audio_common",
	},
	{ },
};


/*lint -save -e* */
MODULE_DEVICE_TABLE(of, usb_audio_common_of_match);
/*lint -restore*/

static long usb_audio_common_ioctl(struct file *file, unsigned int cmd,
							   unsigned long arg)
{
	int ret = 0;
	unsigned int __user *p_user = (unsigned int __user *) arg;

	if (pdata == NULL)
		return -EBUSY;

	switch (cmd) {
		case IOCTL_USB_AUDIO_COMMON_GET_TYPEC_STATE:
			hwlog_info("usb typec plugin:%d\n", pdata->usb_typec_plugin);
			ret = put_user((__u32)(pdata->usb_typec_plugin), p_user);
			break;
		default:
			hwlog_err("unsupport cmd\n");
			ret = -EINVAL;
			break;
	}

	return (long)ret;

}


static const struct file_operations usb_audio_common_fops = {
	.owner           = THIS_MODULE,
	.open            = simple_open,
	.unlocked_ioctl  = usb_audio_common_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl    = usb_audio_common_ioctl,
#endif

};

static struct miscdevice usb_audio_common_miscdev = {
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "usb_audio_common",
	.fops =     &usb_audio_common_fops,
};

static int usb_audio_common_notifier_call(struct notifier_block *typec_nb, unsigned long event, void *data)
{
	char envp_ext0[ENVP_LENTH];
	char *envp_ext[2] = { envp_ext0, NULL };

	hwlog_info("usb_audio_common_notifier_call %d\n", event);
	mutex_lock(&pdata->notifier_lock);
	if (true == pdata->usb_typec_plugin && (PD_DPM_USB_TYPEC_DETACHED == event || PD_DPM_USB_TYPEC_NONE == event)) {
		pdata->usb_typec_plugin = false;
		snprintf(envp_ext0, ENVP_LENTH, "plugout");
	} else if (false == pdata->usb_typec_plugin && (PD_DPM_USB_TYPEC_DETACHED != event && PD_DPM_USB_TYPEC_NONE != event)) {
		pdata->usb_typec_plugin = true;
		snprintf(envp_ext0, ENVP_LENTH, "plugin");
	} else {
		mutex_unlock(&pdata->notifier_lock);
		return 0;
	}

	kobject_uevent_env(&usb_audio_common_miscdev.this_device->kobj, KOBJ_CHANGE, envp_ext);
	mutex_unlock(&pdata->notifier_lock);

    return 0;
}

/*lint -save -e* */
static int usb_audio_common_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;
	int event = PD_DPM_USB_TYPEC_NONE;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (NULL == pdata) {
		hwlog_err("cannot allocate usb_audio_common data\n");
		return -ENOMEM;
	}
	mutex_init(&pdata->notifier_lock);
	pdata->usb_typec_plugin = false;
	pdata->usb_device_notifier.notifier_call = usb_audio_common_notifier_call;

	ret = misc_register(&usb_audio_common_miscdev);
	if (0 != ret) {
		hwlog_err("%s: can't register usb audio common miscdev, ret:%d.\n", __func__, ret);
		goto err_out;
	}

	register_pd_dpm_portstatus_notifier(&pdata->usb_device_notifier);
	pd_dpm_get_typec_state(&event);
	usb_audio_common_notifier_call(NULL, (unsigned long)event, NULL);

	hwlog_info("usb_audio_common probe success!\n");

	return 0;

err_out:
	devm_kfree(dev, pdata);
	pdata = NULL;

	return ret;
}
/*lint -restore*/

static int usb_audio_common_remove(struct platform_device *pdev)
{

	if (pdata) {
		unregister_pd_dpm_portstatus_notifier(&pdata->usb_device_notifier);
		hwlog_info("%s: unregister_pd_dpm_portstatus_notifier\n", __func__);
		devm_kfree(&pdev->dev, pdata);
		pdata = NULL;
	}


	misc_deregister(&usb_audio_common_miscdev);

	hwlog_info("%s: exit\n", __func__);

	return 0;
}

static struct platform_driver usb_audio_common_driver = {
	.driver = {
		.name   = "usb_audio_common",
		.owner  = THIS_MODULE,
		.of_match_table = usb_audio_common_of_match,
	},
	.probe  = usb_audio_common_probe,
	.remove = usb_audio_common_remove,
};

static int __init usb_audio_common_init(void)
{
	return platform_driver_register(&usb_audio_common_driver);
}

static void __exit usb_audio_common_exit(void)
{
	platform_driver_unregister(&usb_audio_common_driver);
}

/*lint -save -e* */
device_initcall_sync(usb_audio_common_init);
module_exit(usb_audio_common_exit);
/*lint -restore*/

MODULE_DESCRIPTION("usb audio common control driver");
MODULE_LICENSE("GPL");
