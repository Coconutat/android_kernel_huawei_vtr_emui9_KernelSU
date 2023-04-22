#include <linux/module.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/thermal.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/battery_temp_fitting.h>

#define HWLOG_TAG battery_temp_fitting
HWLOG_REGIST();

#define BFT_ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

static int btf_total_tz_type = 0;
static int btf_total_ichg = 0;
static int btf_total_fitting_para = 0;

static struct btf_device * btf_g_info = NULL;

static bool btf_probe_status = false;

static void btf_dump_dts_data(void);


static int bft_calc_temp(int value, int index)
{
	struct thermal_zone_device *tz = NULL;
	int temp = 0;

	tz = btf_g_info->tz_type_para[index].tz;

	if ((0 != value) && (NULL != tz))
	{
		thermal_zone_get_temp(tz, &temp);

		temp = value * temp;
	}
	else
	{
		temp = value * 1;
	}

	//hwlog_info("value=%d,%s,%d,%d!\n", value, btf_g_info->tz_type_para[index].tz_type, index, temp);

	return temp;
}

int btf_get_battery_temp_with_current(int ichg, int *temp)
{
	int i = 0;
	int ichg_index = 0;
	int para_index = 0;
	long new_bat_temp = 0;

	/* not support or probe fail return default temperature value */
	if (btf_probe_status == false)
	{
		return -1;
	}

	for(i = 0; i < btf_g_info->total_ichg; i++)
	{
		if (ichg >= btf_g_info->ichg_para[i].ichg)
		{
			ichg_index = i;
			break;
		}
	}

	/* invalid para */
	if (i >= btf_g_info->total_ichg)
	{
		hwlog_err("error: para btf_ichg_para invalid!\n");
		return -1;
	}

	for (i = 0; i < btf_g_info->total_tz_type; i++)
	{
		para_index = ichg_index * btf_g_info->total_tz_type + i;
		new_bat_temp += bft_calc_temp(btf_g_info->fitting_para[para_index].value, i);
	}

	*temp = (int)new_bat_temp / 1000 / 1000;

	return 0;
}
EXPORT_SYMBOL_GPL(btf_get_battery_temp_with_current);

static void btf_dump_dts_data(void)
{
	int i = 0;

	hwlog_info("tz_type_para:");
	for(i = 0;i < btf_g_info->total_tz_type; i++)
	{
		printk(" %s", btf_g_info->tz_type_para[i].tz_type);
	}
	printk("\n");

	hwlog_info("ichg_para:");
	for(i = 0;i < btf_g_info->total_ichg; i++)
	{
		printk(" %d", btf_g_info->ichg_para[i].ichg);
	}
	printk("\n");

	hwlog_info("fitting_para:");
	for(i = 0;i < btf_g_info->total_fitting_data; i++)
	{
		printk(" %d", btf_g_info->fitting_para[i].value);
	}
	printk("\n");
}

static int btf_parse_dts_data(struct device_node *np, struct btf_device *info)
{
	int i = 0;
	const char * tmp_string = NULL;
	int ret = 0;

	/* btf_tz_type_para */
	for (i = 0; i < info->total_tz_type; i++)
	{
		ret = of_property_read_string_index(np, "btf_tz_type_para", i, &tmp_string);
		if(ret)
		{
			hwlog_err("error: para btf_tz_type_para get failed!\n");
			return -EINVAL;
		}

		strncpy(info->tz_type_para[i].tz_type, tmp_string, (BTF_TZ_TYPE_LENGTH - 1));

		info->tz_type_para[i].tz = thermal_zone_get_zone_by_name(info->tz_type_para[i].tz_type);
		if (IS_ERR(info->tz_type_para[i].tz))
		{
			info->tz_type_para[i].tz = NULL;
			hwlog_err("error: get thermalzone type %s fail\n", info->tz_type_para[i].tz_type);
		}
	}

	/* btf_ichg_para */
	for (i = 0; i < info->total_ichg; i++)
	{
		ret = of_property_read_string_index(np, "btf_ichg_para", i, &tmp_string);
		if(ret)
		{
			hwlog_err("error: para btf_ichg_para get failed!\n");
			return -EINVAL;
		}

		info->ichg_para[i].ichg = simple_strtol(tmp_string, NULL, 10);
	}

	/* btf_fitting_para */
	for (i = 0; i < info->total_fitting_data; i++)
	{
		ret = of_property_read_string_index(np, "btf_fitting_para", i, &tmp_string);
		if(ret) {
			hwlog_err("error: para btf_fitting_para get failed!\n");
			return -EINVAL;
		}

		info->fitting_para[i].value = simple_strtol(tmp_string, NULL, 10);
	}

	btf_dump_dts_data();

	return 0;
}

static int btf_get_dts_para_length(struct device_node *np)
{
	int array_len = 0;

	/* btf_tz_type_para */
	array_len = of_property_count_strings(np, "btf_tz_type_para");
	if (array_len <= 0)
	{
		btf_total_tz_type = 0;
		hwlog_err("error: para btf_tz_type_para invalid [%d]!\n", array_len);
		return -EINVAL;
	}
	btf_total_tz_type = array_len;

	hwlog_info("btf_total_tz_type:%d\n", btf_total_tz_type);

	/* btf_ichg_para */
	array_len = of_property_count_strings(np, "btf_ichg_para");
	if (array_len <= 0)
	{
		btf_total_ichg = 0;
		hwlog_err("error: para btf_ichg_para invalid [%d]!\n", array_len);
		return -EINVAL;
	}
	btf_total_ichg = array_len;

	hwlog_info("btf_total_ichg:%d\n", btf_total_ichg);

	/* btf_fitting_para */
	array_len = of_property_count_strings(np, "btf_fitting_para");
	if ((array_len <= 0) || \
		(array_len % btf_total_tz_type != 0) || \
		(array_len > btf_total_tz_type * btf_total_ichg))
	{
		btf_total_fitting_para = 0;
		hwlog_err("error: para btf_fitting_para invalid [%d]!\n", array_len);
		return -EINVAL;
	}
	btf_total_fitting_para = array_len;

	hwlog_info("btf_total_fitting_para:%d\n", btf_total_fitting_para);

	return 0;
}

static int battery_temp_fitting_probe(struct platform_device *pdev)
{
	struct device_node *np = NULL;
	struct btf_device *info = NULL;

	hwlog_info("probe begin\n");

	np = pdev->dev.of_node;
	if(NULL == np)
	{
		hwlog_err("error: device_node is NULL!\n");
		goto free_mem_0;
	}

	if (btf_get_dts_para_length(np) != 0)
	{
		goto free_mem_0;
	}

	info = devm_kzalloc(&pdev->dev, \
		sizeof(struct btf_device), GFP_KERNEL);
	if (NULL == info)
	{
		hwlog_err("error: info devm_kzalloc failed!\n");
		goto free_mem_0;
	}

	info->tz_type_para = devm_kzalloc(&pdev->dev, \
		sizeof(struct btf_tz_type_para) * btf_total_tz_type, GFP_KERNEL);
	if (NULL == info->tz_type_para)
	{
		hwlog_err("error: tz_type_para devm_kzalloc failed!\n");
		goto free_mem_1;
	}

	info->ichg_para = devm_kzalloc(&pdev->dev, \
		sizeof(struct btf_ichg_para) * btf_total_ichg, GFP_KERNEL);
	if (NULL == info->ichg_para)
	{
		hwlog_err("error: ichg_para devm_kzalloc failed!\n");
		goto free_mem_2;
	}

	info->fitting_para = devm_kzalloc(&pdev->dev, \
		sizeof(struct btf_fitting_para) * btf_total_fitting_para, GFP_KERNEL);
	if (NULL == info->fitting_para)
	{
		hwlog_err("error: fitting_para devm_kzalloc failed!\n");
		goto free_mem_3;
	}

	info->pdev = pdev;
	if(NULL == info->pdev)
	{
		hwlog_err("error: device_node is NULL!\n");
		goto free_mem_4;
	}

	btf_g_info = info;
	info->total_tz_type = btf_total_tz_type;
	info->total_ichg = btf_total_ichg;
	info->total_fitting_data = btf_total_fitting_para;

	if(btf_parse_dts_data(np, info) != 0)
	{
		goto free_mem_4;
	}

	platform_set_drvdata(pdev, info);

	btf_probe_status = true;

	hwlog_info("probe end\n");
	return 0;

free_mem_4:
	devm_kfree(&pdev->dev, info->fitting_para);
free_mem_3:
	devm_kfree(&pdev->dev, info->ichg_para);
free_mem_2:
	devm_kfree(&pdev->dev, info->tz_type_para);
free_mem_1:
	devm_kfree(&pdev->dev, info);
free_mem_0:
	btf_g_info = NULL;
	btf_probe_status = false;

	return -1;
}


static int battery_temp_fitting_remove(struct platform_device *pdev)
{
	struct btf_device *info = platform_get_drvdata(pdev);

	hwlog_info("remove begin\n");

	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, info->fitting_para);
	devm_kfree(&pdev->dev, info->ichg_para);
	devm_kfree(&pdev->dev, info->tz_type_para);
	devm_kfree(&pdev->dev, info);

	btf_g_info = NULL;
	btf_probe_status = false;

	hwlog_info("remove end\n");

	return 0;
}

static struct of_device_id battery_temp_fitting_match_table[] = {
	{
	 .compatible = "huawei,battery_temp_fitting",
	 .data = NULL,
	},
	{ },
};

static struct platform_driver battery_temp_fitting_driver = {
	.probe = battery_temp_fitting_probe,
	.remove = battery_temp_fitting_remove,
	.driver = {
		.name = "huawei,battery_temp_fitting",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(battery_temp_fitting_match_table),
	},
};

static int __init battery_temp_fitting_init(void)
{
	return platform_driver_register(&battery_temp_fitting_driver);
}

static void __exit battery_temp_fitting_exit(void)
{
	platform_driver_unregister(&battery_temp_fitting_driver);
}

module_init(battery_temp_fitting_init);
module_exit(battery_temp_fitting_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("power battery temp fitting module driver");
MODULE_AUTHOR("HUAWEI Inc");

