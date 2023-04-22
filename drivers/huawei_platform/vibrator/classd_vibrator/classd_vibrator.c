#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/hisi_pmic.h>
#include <../../../drivers/staging/android/timed_output.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/hisi/hisi_vibrator.h>

#define CLASSD_VIBRATOR "classd_vibrator"

#define ON 1
#define OFF 0

struct classd_vibrator_data {
	struct timed_output_dev dev;
	struct hrtimer timer;
	struct mutex lock;
	u32 classd_ctl_reg;
	u32 classd_ctl_value;
	int vib_status;
	struct regulator *vout_ldo;
};

static struct classd_vibrator_data *classd_vibrator_pdata;

#ifdef CONFIG_HISI_VIBRATOR
extern volatile int vibrator_shake;
#else
volatile int vibrator_shake;
#endif

static void classd_vibrator_switch(int status)
{
	int error;

	BUG_ON(classd_vibrator_pdata == NULL);
	if (status == classd_vibrator_pdata->vib_status)
		return;

	BUG_ON(IS_ERR_OR_NULL(classd_vibrator_pdata->vout_ldo));
	pr_info("---- classd_vibrator switch %s ----\n", status ? "ON" : "OFF");
	mutex_lock(&classd_vibrator_pdata->lock);
	if (status) {
		error = regulator_enable(classd_vibrator_pdata->vout_ldo);
		if (error < 0) {
			pr_err("%s: failed to enable classd-vibrator regulator\n", __func__);
			goto unlock;
		}
		hisi_pmic_reg_write(classd_vibrator_pdata->classd_ctl_reg, classd_vibrator_pdata->classd_ctl_value);
	} else {
		hisi_pmic_reg_write(classd_vibrator_pdata->classd_ctl_reg, 0x00);
		error = regulator_disable(classd_vibrator_pdata->vout_ldo);
		if (error < 0) {
			pr_err("%s: failed to disable classd-vibrator regulator\n", __func__);
			goto unlock;
		}
	}
	classd_vibrator_pdata->vib_status = status;
	vibrator_shake = status;

unlock:
	mutex_unlock(&classd_vibrator_pdata->lock);
	return;
}

static enum hrtimer_restart classd_vibrator_timer_func(struct hrtimer *timer)
{
	pr_info("classd_vibrator_timer_func called\n");
	classd_vibrator_switch(OFF);

	return HRTIMER_NORESTART;
}

static int classd_vibrator_get_time(struct timed_output_dev *dev)
{
	struct classd_vibrator_data *pdata =
			container_of(dev, struct classd_vibrator_data, dev);
	if (hrtimer_active(&pdata->timer)) {
		ktime_t r = hrtimer_get_remaining(&pdata->timer);
		return ktime_to_ns(r) / NSEC_PER_MSEC; /* convert ktime to millisecond */
	} else
		return 0;
}

static void classd_vibrator_enable(struct timed_output_dev *dev, int value)
{
	struct classd_vibrator_data *pdata = container_of(dev, struct classd_vibrator_data, dev);

	pr_info("classd_vibrator_enable, value=%d\n",value);
	if (value < 0) {
		pr_err("error:vibrator_enable value:%d is negative\n", value);
		return;
	}
	/* cancel previous timer */
	if (hrtimer_active(&pdata->timer))
		hrtimer_cancel(&pdata->timer);

	if (value > 0) {
		if (value < TIMEOUT_MIN)
			value = TIMEOUT_MIN;
		classd_vibrator_switch(ON);
		hrtimer_start(&pdata->timer,
			ns_to_ktime((u64)value * NSEC_PER_MSEC), /* use millisecond to construct ktime_t */
			HRTIMER_MODE_REL);
	} else {
		classd_vibrator_switch(OFF);
	}
}

#ifdef CONFIG_OF
static const struct of_device_id classd_vibrator_match[] = {
	{ .compatible = "huawei,classd-vibrator",},
	{},
};
MODULE_DEVICE_TABLE(of, classd_vibrator_match);

static int classd_vibrator_get_vout(struct platform_device *pdev, struct classd_vibrator_data *pdata){
	int min_voltage=0;
	int max_voltage=0;
	int err = 0;

	pdata->vout_ldo = devm_regulator_get(&pdev->dev, "vibrator-vdd");

	if (IS_ERR_OR_NULL(pdata->vout_ldo)){
		dev_err(&pdev->dev, "%s: classd_vibrator_vout_reg error\n", __func__);
		return -ENODEV;
	}

	err = of_property_read_u32(pdev->dev.of_node, "vibrator_vout_min_voltage", (u32 *)&min_voltage);
	if (err) {
		dev_err(&pdev->dev, "%s: min_voltage read failed\n", __func__);
		return err;
	}

	err = of_property_read_u32(pdev->dev.of_node, "vibrator_vout_max_voltage", (u32 *)&max_voltage);
	if (err) {
		dev_err(&pdev->dev, "%s: max_voltage read failed\n", __func__);
		return err;
	}

	if (regulator_set_voltage(pdata->vout_ldo, min_voltage, max_voltage)){
		dev_err(&pdev->dev, "%s: vibrator set voltage error\n", __func__);
		return -EPERM;
	}

	return 0;
}

static int classd_vibrator_probe(struct platform_device *pdev)
{
	struct classd_vibrator_data *p_data;
	int ret = 0;

	if (!of_match_node(classd_vibrator_match, pdev->dev.of_node)) {
		dev_err(&pdev->dev, "dev node no match. exiting.\n");
		return -ENODEV;
	}

	p_data = kzalloc(sizeof(struct classd_vibrator_data), GFP_KERNEL);
	if (p_data == NULL) {
		dev_err(&pdev->dev, "failed to allocate vibrator_device\n");
		return -ENOMEM;
	}

	ret = classd_vibrator_get_vout(pdev, p_data);
	if (ret) {
		dev_err(&pdev->dev, "failed to get vib vout\n");
		goto err;
	}
	ret = of_property_read_u32(pdev->dev.of_node, "classd-ctl-reg", (u32 *)&p_data->classd_ctl_reg);
	if (ret) {
		dev_err(&pdev->dev, "failed to get classd-ctl-reg");
		goto err;
	}
	ret = of_property_read_u32(pdev->dev.of_node, "classd-ctl-value", (u32 *)&p_data->classd_ctl_value);
	if (ret) {
		dev_err(&pdev->dev, "failed to get classd-ctl-value");
		goto err;
	}

	/* timed_output */
	p_data->dev.name = "vibrator";
	p_data->dev.get_time = classd_vibrator_get_time;
	p_data->dev.enable = classd_vibrator_enable;
	ret = timed_output_dev_register(&p_data->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register dev\n");
		goto err;
	}

	/* init lock */
	mutex_init(&p_data->lock);

	/* init timer */
	hrtimer_init(&p_data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	p_data->timer.function = classd_vibrator_timer_func;

	platform_set_drvdata(pdev, p_data);
	classd_vibrator_pdata = p_data;
	dev_info(&pdev->dev, "%s: successful!\n", __FUNCTION__);
	return 0;

err:
	devm_regulator_put(p_data->vout_ldo);
	p_data->vout_ldo = NULL;
	kfree(p_data);
	dev_err(&pdev->dev, "%s: failed!\n", __FUNCTION__);
	return ret;
}

static int classd_vibrator_remove(struct platform_device *pdev)
{
	struct classd_vibrator_data *pdata = platform_get_drvdata(pdev);

	if (pdata == NULL) {
		dev_err(&pdev->dev, "%s:pdata is NULL\n", __func__);
		return -ENODEV;
	}

	if (hrtimer_active(&pdata->timer))
		hrtimer_cancel(&pdata->timer);
	mutex_destroy(&pdata->lock);

	timed_output_dev_unregister(&pdata->dev);

	devm_regulator_put(pdata->vout_ldo);
	pdata->vout_ldo = NULL;
	kfree(pdata);
	classd_vibrator_pdata = NULL;
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static void classd_vibrator_shutdown(struct platform_device *pdev)
{
	struct classd_vibrator_data *pdata = platform_get_drvdata(pdev);
	if (pdata == NULL) {
		dev_err(&pdev->dev, "%s:pdata is NULL\n", __func__);
		return;
	}

	if (hrtimer_active(&pdata->timer))
		hrtimer_cancel(&pdata->timer);

	classd_vibrator_switch(OFF);

	return;
}

#ifdef CONFIG_PM
static int classd_vibrator_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct classd_vibrator_data *pdata = platform_get_drvdata(pdev);
	if (pdata == NULL) {
		dev_err(&pdev->dev, "%s:pdata is NULL\n", __func__);
		return -ENODEV;
	}

	if (hrtimer_active(&pdata->timer)) {
		hrtimer_cancel(&pdata->timer);
		classd_vibrator_switch(OFF);
	}
	if (!mutex_trylock(&pdata->lock)) {
		dev_err(&pdev->dev, "%s: mutex_trylock.\n", __func__);
		return -EAGAIN;
	}

	return 0;
}

static int classd_vibrator_resume(struct platform_device *pdev)
{
	struct classd_vibrator_data *pdata = platform_get_drvdata(pdev);
	if (pdata == NULL) {
		dev_err(&pdev->dev, "%s:pdata is NULL\n", __func__);
		return -ENODEV;
	}

	mutex_unlock(&pdata->lock);

	return 0;
}
#endif

#endif

static struct platform_driver classd_vibrator_driver = {
	.probe    = classd_vibrator_probe,
	.remove   = classd_vibrator_remove,
	.shutdown = classd_vibrator_shutdown,
#ifdef CONFIG_PM
	.suspend  = classd_vibrator_suspend,
	.resume   = classd_vibrator_resume,
#endif
	.driver   = {
		.name = CLASSD_VIBRATOR,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(classd_vibrator_match),
	},
};

static int __init classd_vibrator_init(void)
{
	return platform_driver_register(&classd_vibrator_driver);
}

static void __exit classd_vibrator_exit(void)
{
	platform_driver_unregister(&classd_vibrator_driver);
}

module_init(classd_vibrator_init);
module_exit(classd_vibrator_exit);

MODULE_AUTHOR("maintainer");
MODULE_DESCRIPTION("huawei classd-vibrator driver");
MODULE_LICENSE("GPL");
