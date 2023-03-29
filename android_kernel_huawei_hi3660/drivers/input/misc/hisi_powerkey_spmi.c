/*lint -e752 -esym(752,*)*/
/*lint -e753 -esym(753,*)*/
/*lint -e528 -esym(528,*)*/
/*lint -save -e715 -e747 -e838 -e732 -e785 -e528 -e753 -e752 */
/*
 * hisi_hisi_powerkey.c - Hisilicon MIC powerkey driver
 *
 * Copyright (C) 2013 Hisilicon Ltd.
 * Copyright (C) 2013 Linaro Ltd.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License. See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/version.h>

#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/wakelock.h>
#include <asm/irq.h>
#include <linux/hisi/util.h>
#include <linux/kthread.h>
#include <linux/syscalls.h>
#include <linux/hisi/hisi_bootup_keypoint.h>

#include <linux/hisi-spmi.h>
#include <linux/of_hisi_spmi.h>

#ifdef CONFIG_HISI_HI6XXX_PMIC
#include <soc_smart_interface.h>
#endif
#include <linux/hisi/hisi_powerkey_event.h>
#ifdef CONFIG_HISI_BB
#include <linux/hisi/rdr_hisi_platform.h>
#endif
#ifdef CONFIG_HW_ZEROHUNG
#include <chipset_common/hwzrhung/zrhung.h>
#endif
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include <linux/hisi/hisi_log.h>

#define HISI_LOG_TAG HISI_POWERKEY_TAG

#define POWER_KEY_RELEASE	(0)
#define POWER_KEY_PRESS		(1)

#ifdef CONFIG_HUAWEI_DSM
#define PRESS_KEY_INTERVAL	(80)	/*the minimum press interval*/
#define STATISTIC_INTERVAL	(60)	/*the statistic interval for key event*/
#define MAX_PRESS_KEY_COUNT	(120)	/*the default press count for a normal use*/

static int powerkey_press_count;
static unsigned long powerkey_last_press_time;
static struct timer_list dsm_powerkey_timer;	/*used to reset the statistic variable*/

static struct dsm_dev dsm_power_key = {
	.name = "dsm_power_key",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = 1024,
};
static struct dsm_client *power_key_dclient;
#endif

struct hisi_powerkey_info {
	struct input_dev *idev;
	int irq[6];
	struct wake_lock pwr_wake_lock;
};

static struct semaphore long_presspowerkey_happen_sem;

int long_presspowerkey_happen(void *data)
{
	pr_err("long_presspowerkey_happen start\n");
	while (!kthread_should_stop()) {
		down(&long_presspowerkey_happen_sem);
#ifdef CONFIG_HISI_BB
		rdr_long_press_powerkey();
#endif
#ifdef CONFIG_HW_ZEROHUNG
		zrhung_save_lastword();
#endif
	}
	return 0;
}

#ifdef CONFIG_HUAWEI_DSM
static void powerkey_timer_func(unsigned long data)
{
	if (powerkey_press_count > MAX_PRESS_KEY_COUNT) {
		if (!dsm_client_ocuppy(power_key_dclient)) {
			dsm_client_record(power_key_dclient,
					"powerkey trigger on the abnormal style.\n");
			dsm_client_notify(power_key_dclient,
					DSM_POWER_KEY_ERROR_NO);
		}
	}

	/* reset the statistic variable */
	powerkey_press_count = 0;
	mod_timer(&dsm_powerkey_timer, jiffies + STATISTIC_INTERVAL * HZ);

	return;
}
#endif

#ifdef CONFIG_HISI_POWERKEY_DEBUG
static int hisi_test_powerkey_notifier_call(struct notifier_block *powerkey_nb, unsigned long event, void *data)
{
	if (event == HISI_PRESS_KEY_DOWN) {
		pr_err("[%s] test power key press event!\n",__func__);
	}else if (event == HISI_PRESS_KEY_UP) {
		pr_err("[%s] test power key release event!\n",__func__);
	}else if (event == HISI_PRESS_KEY_1S) {
		pr_err("[%s] test response long press 1s event!\n",__func__);
	}else if (event == HISI_PRESS_KEY_6S) {
		pr_err("[%s] test response long press 6s event!\n",__func__);
	} else if (event == HISI_PRESS_KEY_8S) {
		pr_info("[%s] test response long press 8s event!\n",__func__);
	} else if (event == HISI_PRESS_KEY_10S) {
		pr_info("[%s] test response long press 10s event!\n",__func__);
	} else {
		pr_err("[%s]invalid event %d!\n", __func__,(int) (event));
	}
	return 0;
}

static struct notifier_block  hisi_test_powerkey_nb;
#endif

static irqreturn_t hisi_powerkey_handler(int irq, void *data)
{
	struct hisi_powerkey_info *info = (struct hisi_powerkey_info *)data;

	wake_lock_timeout(&info->pwr_wake_lock, HZ);

	if (info->irq[0] == irq) {
		pr_err("[%s] power key press interrupt!\n",
		       __func__);
		hisi_call_powerkey_notifiers(HISI_PRESS_KEY_DOWN,data);
#ifdef CONFIG_HUAWEI_DSM
		powerkey_press_count++;
		if ((jiffies - powerkey_last_press_time) <
				msecs_to_jiffies(PRESS_KEY_INTERVAL)) {
			if (!dsm_client_ocuppy(power_key_dclient)) {
				dsm_client_record(power_key_dclient,
						"power key trigger on the abnormal style.\n");
				dsm_client_notify(power_key_dclient,
						DSM_POWER_KEY_ERROR_NO);
			}
		}
		powerkey_last_press_time = jiffies;
#endif
		input_report_key(info->idev, KEY_POWER, POWER_KEY_PRESS);
		input_sync(info->idev);
	} else if (info->irq[1] == irq) {
		pr_err("[%s]power key release interrupt!\n",
		       __func__);
		hisi_call_powerkey_notifiers(HISI_PRESS_KEY_UP,data);
		input_report_key(info->idev, KEY_POWER, POWER_KEY_RELEASE);
		input_sync(info->idev);
	} else if (info->irq[2] == irq) {
		pr_err("[%s]response long press 1s interrupt!\n",
			__func__);
		hisi_call_powerkey_notifiers(HISI_PRESS_KEY_1S,data);
		input_report_key(info->idev, KEY_POWER, POWER_KEY_PRESS);
		input_sync(info->idev);
	} else if (irq == info->irq[3]) {
		pr_err("[%s]response long press 6s interrupt!\n",
			__func__);
		hisi_call_powerkey_notifiers(HISI_PRESS_KEY_6S,data);
		up(&long_presspowerkey_happen_sem);
	} else if (irq == info->irq[4]) {
		pr_info("[%s]response long press 8s interrupt!\n",
			__func__);
		hisi_call_powerkey_notifiers(HISI_PRESS_KEY_8S,data);
	} else if (irq == info->irq[5]) {
		pr_info("[%s]response long press 10s interrupt!\n",
			__func__);
		hisi_call_powerkey_notifiers(HISI_PRESS_KEY_10S,data);
	} else {
		pr_err("[%s]invalid irq %d!\n", __func__, irq);
	}

	return IRQ_HANDLED;
}

static int hisi_powerkey_probe(struct spmi_device *pdev)
{
	struct hisi_powerkey_info *info = NULL; //lint !e429
	struct device *dev = &pdev->dev;
	int ret = 0;

	if (NULL == pdev) {
		dev_err(dev, "[Pwrkey]parameter error!\n");
		ret = -EINVAL;
		return ret;
	}

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->idev = input_allocate_device();
	if (!info->idev) {
		dev_err(&pdev->dev, "Failed to allocate input device\n");
		ret = -ENOENT;
		return ret;/*lint !e429 */
	}

	info->idev->name = "hisi_on";
	info->idev->phys = "hisi_on/input0";
	info->idev->dev.parent = &pdev->dev;
	info->idev->evbit[0] = BIT_MASK(EV_KEY);
	__set_bit(KEY_POWER, info->idev->keybit);

	wake_lock_init(&info->pwr_wake_lock, WAKE_LOCK_SUSPEND, "android-pwr");

#if defined (CONFIG_HUAWEI_DSM)
	/* initialize the statistic variable */

	powerkey_press_count = 0;
	powerkey_last_press_time = 0;
	setup_timer(&dsm_powerkey_timer, powerkey_timer_func,
			(unsigned long)info);
#endif

#ifdef CONFIG_HISI_POWERKEY_DEBUG
	hisi_test_powerkey_nb.notifier_call = hisi_test_powerkey_notifier_call;
	hisi_powerkey_register_notifier(&hisi_test_powerkey_nb);
#endif

	sema_init(&long_presspowerkey_happen_sem, 0);

	if (!kthread_run(long_presspowerkey_happen, NULL, "long_powerkey")) {
		pr_err("create thread long_presspowerkey_happen faild.\n");
	}

	info->irq[0] = spmi_get_irq_byname(pdev, NULL, "down");
	if (info->irq[0] < 0) {
		dev_err(dev, "failed to get down irq id\n");
		ret = -ENOENT;
		goto unregister_err;
	}

	ret = devm_request_irq(dev, info->irq[0], hisi_powerkey_handler,
			       IRQF_NO_SUSPEND, "down", info);
	if (ret < 0) {
		dev_err(dev, "failed to request down irq\n");
		ret = -ENOENT;
		goto unregister_err;
	}

	info->irq[1] = spmi_get_irq_byname(pdev, NULL, "up");
	if (info->irq[1] < 0) {
		dev_err(dev, "failed to get up irq id\n");
		ret = -ENOENT;
		goto unregister_err;
	}

	ret = devm_request_irq(dev, info->irq[1], hisi_powerkey_handler,
			       IRQF_NO_SUSPEND, "up", info);
	if (ret < 0) {
		dev_err(dev, "failed to request up irq\n");
		ret = -ENOENT;
		goto unregister_err;
	}

	info->irq[2] = spmi_get_irq_byname(pdev, NULL, "hold 1s");
	if (info->irq[2] < 0) {
		dev_err(dev, "failed to get hold 1s irq id\n");
		ret = -ENOENT;
		goto unregister_err;
	}

	ret = devm_request_irq(dev, info->irq[2], hisi_powerkey_handler,
			       0, "hold 1s", info);
	if (ret < 0) {
		dev_err(dev, "failed to request hold 1s irq\n");
		ret = -ENOENT;
		goto unregister_err;
	}

	info->irq[3] = spmi_get_irq_byname(pdev, NULL, "hold 6s");
	if (info->irq[3] >= 0) {
		ret = devm_request_irq(dev, info->irq[3], hisi_powerkey_handler,
				       0, "hold 6s", info);
		if (ret < 0) {
			dev_err(dev, "failed to request hold 6s irq\n");
			ret = -ENOENT;
			goto unregister_err;
		}
	}

	info->irq[4] = spmi_get_irq_byname(pdev, NULL, "hold 8s");
	if (info->irq[4] >= 0) {
		ret = devm_request_irq(dev, info->irq[4], hisi_powerkey_handler,
				       0, "hold 8s", info);
		if (ret < 0) {
			dev_err(dev, "failed to request hold 8s irq\n");
			ret = -ENOENT;
			goto unregister_err;
		}
	}

	info->irq[5] = spmi_get_irq_byname(pdev, NULL, "hold 10s");
	if (info->irq[5] >= 0) {
		ret = devm_request_irq(dev, info->irq[5], hisi_powerkey_handler,
				       0, "hold 10s", info);
		if (ret < 0) {
			dev_err(dev, "failed to request hold 10s irq\n");
			ret = -ENOENT;
			goto unregister_err;
		}
	}

	ret = input_register_device(info->idev);
	if (ret) {
		dev_err(&pdev->dev, "Can't register input device: %d\n", ret);
		ret = -ENOENT;
		goto input_err;
	}

	dev_set_drvdata(&pdev->dev, info);

#ifdef CONFIG_HUAWEI_DSM
	if (!power_key_dclient) {
		power_key_dclient = dsm_register_client(&dsm_power_key);
	}
	mod_timer(&dsm_powerkey_timer, jiffies + STATISTIC_INTERVAL * HZ);
#endif

	return ret; //lint !e429

input_err:
unregister_err:
	wake_lock_destroy(&info->pwr_wake_lock);
	input_free_device(info->idev);

	return ret; //lint !e429
}

static int hisi_powerkey_remove(struct spmi_device *pdev)
{
	struct hisi_powerkey_info *info = dev_get_drvdata(&pdev->dev);
#ifdef CONFIG_HISI_POWERKEY_DEBUG
	hisi_powerkey_unregister_notifier(&hisi_test_powerkey_nb);
#endif
	if (NULL != info) {
		wake_lock_destroy(&info->pwr_wake_lock);
		input_free_device(info->idev);
		input_unregister_device(info->idev);
	}
	return 0;
}

#ifdef CONFIG_PM
static int hisi_powerkey_suspend(struct spmi_device *pdev,
				 pm_message_t state)
{
	pr_info("[%s]suspend successfully\n", __func__);
	return 0;
}

static int hisi_powerkey_resume(struct spmi_device *pdev)
{
	pr_info("[%s]resume successfully\n", __func__);
	return 0;
}
#endif

static struct of_device_id hisi_powerkey_of_match[] = {
	{.compatible = "hisilicon-hisi-powerkey",},
	{},
};

static struct spmi_device_id powerkey_spmi_id[] = {
	{"hisilicon-hisi-powerkey", 0},
	{},
};

MODULE_DEVICE_TABLE(of, hisi_powerkey_of_match);

static struct spmi_driver hisi_powerkey_driver = {
	.probe = hisi_powerkey_probe,
	.remove = hisi_powerkey_remove,
	.driver = {
		   .owner = THIS_MODULE,/*lint !e64*/
		   .name = "hisi-powerkey",
		   .of_match_table = hisi_powerkey_of_match,
		   },
#ifdef CONFIG_PM
		.suspend = hisi_powerkey_suspend,
		.resume = hisi_powerkey_resume,
#endif
	.id_table = powerkey_spmi_id,

};

static int __init hisi_powerkey_init(void)
{
	return spmi_driver_register(&hisi_powerkey_driver);
}

static void __exit hisi_powerkey_exit(void)
{
	spmi_driver_unregister(&hisi_powerkey_driver);
}


module_init(hisi_powerkey_init);
module_exit(hisi_powerkey_exit);

MODULE_AUTHOR("Zhiliang Xue <xuezhiliang@huawei.com");
MODULE_DESCRIPTION("Hisi PMIC Power key driver");
MODULE_LICENSE("GPL v2");
/*lint -restore */
/*lint -e528 +esym(528,*)*/
/*lint -e753 +esym(753,*)*/
/*lint -e752 +esym(752,*)*/
