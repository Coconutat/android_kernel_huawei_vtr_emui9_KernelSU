#include <linux/module.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/hisi/hisi_adc.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/battery_voltage.h>
#include "vbat_hkadc.h"

#define HWLOG_TAG vbat_hkadc
HWLOG_REGIST();

static struct vbat_hkadc_info *g_vbat_hkadc_di = NULL;

static int get_vbat_hkadc_mv(void)
{
	int i = 0;
	int adc_vbat = 0;
	int vol = 0;

	if (g_vbat_hkadc_di == NULL) {
		hwlog_err("error: g_vbat_hkadc_di is null!\n");
		return -1;
	}

	for (i = 0; i < VBAT_HKADC_RETRY_TIMES; i++) {
		adc_vbat = hisi_adc_get_adc(g_vbat_hkadc_di->adc_channel);

		if (adc_vbat < 0) {
			hwlog_err("error: hisi adc read fail!\n");
		}
		else {
			break;
		}
	}

	if (adc_vbat < 0) {
		return -1;
	}

	vol = adc_vbat * (g_vbat_hkadc_di->coef) / VBAT_HKADC_COEF_MULTIPLE;
	hwlog_info("adc_vbat=%d, vbat_value=%d\n", adc_vbat, vol);

	return vol;

}

struct hw_batt_vol_ops vbat_hkadc_ops = {
	.get_batt_vol    =   get_vbat_hkadc_mv,
};

static int vbat_hkadc_parse_dts(struct device_node *np, struct vbat_hkadc_info *di)
{
	int ret = 0;

	ret = of_property_read_u32(np, "adc_channel", &di->adc_channel);
	if (ret) {
		hwlog_err("error: adc_channel dts read failed!\n");
		return -EINVAL;
	}
	hwlog_info("adc_channel=%d\n",di->adc_channel);

	ret = of_property_read_u32(np, "offset_channel", &di->offset_channel);
	if (ret) {
		hwlog_err("error: offset_channel dts read failed!\n");
		return -EINVAL;
	}
	hwlog_info("offset_channel=%d\n",di->offset_channel);

	ret = of_property_read_u32(np, "coef", &di->coef);
	if (ret) {
		hwlog_err("error: coef dts read failed!\n");
		return -EINVAL;
	}
	hwlog_info("coef=%d\n",di->coef);

	return 0;

}

static int vbat_hkadc_probe(struct platform_device *pdev)
{
	struct vbat_hkadc_info *di = NULL;
	struct device_node *np = NULL;
	int ret = -1;

	hwlog_info("probe begin\n");

	di = devm_kzalloc(&pdev->dev, sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("error: kzalloc failed!\n");
		return -ENOMEM;
	}
	g_vbat_hkadc_di = di;

	di->pdev = pdev;
	di->dev = &pdev->dev;
	np = pdev->dev.of_node;
	if (NULL == di->pdev || NULL == di->dev || NULL == np) {
		hwlog_err("error: device_node is null!\n");
		goto free_mem;
	}

	ret = vbat_hkadc_parse_dts(np, di);
	if (ret < 0) {
		goto free_mem;
	}

	di->offset_value = hisi_adc_get_adc(di->offset_channel);
	hwlog_info("offset_value=%d\n",di->offset_value);

	ret = hw_battery_voltage_ops_register(&vbat_hkadc_ops, "hisi_hkadc");
	if (ret) {
		hwlog_err("error: hw_battery_voltage ops register failed!\n");
	}

	platform_set_drvdata(pdev, di);

	hwlog_info("probe end\n");
	return 0;

free_mem:
	devm_kfree(&pdev->dev, di);
	g_vbat_hkadc_di = NULL;

	return ret;
}

static int vbat_hkadc_remove(struct platform_device *pdev)
{
	struct vbat_hkadc_info *info = platform_get_drvdata(pdev);

	hwlog_info("remove begin\n");

	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, info);
	g_vbat_hkadc_di = NULL;

	hwlog_info("remove end\n");

	return 0;

}

static struct of_device_id vbat_hkadc_match_table[] = {
	{
		.compatible = "huawei,vbat_hkadc",
		.data = NULL,
	},
	{ },
};

static struct platform_driver vbat_hkadc_driver = {
	.probe = vbat_hkadc_probe,
	.remove = vbat_hkadc_remove,
	.driver = {
		.name = "huawei,vbat_hkadc",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(vbat_hkadc_match_table),
	},
};

static int __init vbat_hkadc_init(void)
{
	return platform_driver_register(&vbat_hkadc_driver);
}

static void __exit vbat_hkadc_exit(void)
{
	platform_driver_unregister(&vbat_hkadc_driver);
}

module_init(vbat_hkadc_init);
module_exit(vbat_hkadc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("huawei vbat_hkadc module driver");
MODULE_AUTHOR("HUAWEI Inc");

