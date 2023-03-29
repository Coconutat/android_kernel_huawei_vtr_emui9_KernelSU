/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/hisi/hisi_rproc.h>
#include <asm/compiler.h>
#include <linux/compiler.h>

#define TSENS_DRIVER_NAME	"hisi-tsens"
static struct tsens_tm_device	*g_tmdev;

#define CLUSTER0				0
#define CLUSTER1				1
#define GPU					2
#define MODEM					3
#define DDR					4
#define TSENSOR_MAX			5

#define	READTEMP_SVC_REG_RD	0xc5009900UL

char *hisi_tsensor_name[] = {
	[CLUSTER0] = "cluster0",
	[CLUSTER1] = "cluster1",
	[GPU] = "gpu",
	[MODEM] = "modem",
	[DDR] = "ddr",
};
EXPORT_SYMBOL_GPL(hisi_tsensor_name);

/* Trips: warm and cool */
enum tsens_trip_type {
	TSENS_TRIP_ORIGNUM = 0,
#ifdef CONFIG_HISI_THERMAL_TRIP
	TSENS_TRIP_THROTTLING = TSENS_TRIP_ORIGNUM,
	TSENS_TRIP_SHUTDOWN,
	TSENS_TRIP_BELOW_VR_MIN,
	TSENS_TRIP_NUM,
#endif
};

enum sensor_type {
	TYPE_TSENSOR = 0,
	TYPE_PVTSENSOR,
};


struct tsens_tm_device_sensor {
	struct thermal_zone_device		*tz_dev;
	enum thermal_device_mode		mode;
	int							reg_no;
	unsigned int			sensor_num;
	int				sensor_type;
#ifdef CONFIG_HISI_THERMAL_TRIP
	s32				temp_throttling;
	s32				temp_shutdown;
	s32				temp_below_vr_min;
#endif
};

struct tsens_tm_device {
	struct platform_device		*pdev;
	int				tsens_num_sensor;
	u32				adc_start_value;
	u32				adc_end_value;
	u32				pvtsensor_adc_start_value;
	u32				pvtsensor_adc_end_value;
	struct tsens_tm_device_sensor	sensor[0];/*put the struct last place*/
};

#define TSENSOR_8BIT			8
#define TSENS_THRESHOLD_MIN_CODE	0x0
#define TSENS_TEMP_START_VALUE		(-40)/* -40 deg C */
#define TSENS_TEMP_END_VALUE		125


int g_tsensor_debug = 0;
void tsen_debug(int onoff)
{
	g_tsensor_debug = onoff;
}
static int tsens_tz_code_to_degc(int adc_val, int sensor_type)
{
	int temp = 0;
	int adc_start_value, adc_end_value;

	if (!g_tmdev)
		goto ERROR;

	if(sensor_type == TYPE_TSENSOR){
		adc_start_value = (int)g_tmdev->adc_start_value;
		adc_end_value = (int)g_tmdev->adc_end_value;
	} else if (sensor_type == TYPE_PVTSENSOR) {
		adc_start_value = (int)g_tmdev->pvtsensor_adc_start_value;
		adc_end_value = (int)g_tmdev->pvtsensor_adc_end_value;
	} else {
		adc_start_value = 0;
		adc_end_value = 0;
	}

	if ((adc_start_value > adc_val) || (adc_val > adc_end_value) || (adc_start_value == 0) || (adc_end_value == 0))
		goto ERROR;

	temp = ((adc_val - adc_start_value) * (TSENS_TEMP_END_VALUE - TSENS_TEMP_START_VALUE))
			/ (adc_end_value - adc_start_value);
	temp = temp + TSENS_TEMP_START_VALUE;

	return temp;

ERROR:
	pr_debug("adc_to_temp_conversion failed \n");
	return temp;
}

/*lint -e715*/
noinline int atfd_readtemp_smc(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;
	asm volatile (
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
		"smc	#0\n"
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));

	return (int)function_id;
}
/*lint +e715*/

int hisi_sec_readtemp_read(u64 reg_address)
{
	return atfd_readtemp_smc(READTEMP_SVC_REG_RD, reg_address, 0UL, 0UL);
}

static int tsens_tz_get_temp(struct thermal_zone_device *thermal,
			     int *temp)
{
	struct tsens_tm_device_sensor *tm_sensor = thermal->devdata;
	int adc_value = 0;

	if (!tm_sensor || tm_sensor->mode != THERMAL_DEVICE_ENABLED || !temp)
		return -EINVAL;

	if (tm_sensor->reg_no >= 0 && tm_sensor->reg_no < g_tmdev->tsens_num_sensor) {
		adc_value = hisi_sec_readtemp_read(tm_sensor->reg_no);
		*temp = tsens_tz_code_to_degc(adc_value, tm_sensor->sensor_type);
	} else
		*temp = TSENS_TEMP_START_VALUE;

	return 0;
}

/*add for IPA*/
int tsens_get_temp(u32 sensor, int *temp)
{
	int i = 0;
	struct thermal_zone_device *thermal;
	int ret = -EINVAL;
	int tmp = 0;

	for (i = 0; i < g_tmdev->tsens_num_sensor; i++) {
		if (sensor == g_tmdev->sensor[i].sensor_num) {
			thermal = g_tmdev->sensor[i].tz_dev;
			ret = tsens_tz_get_temp(thermal, &tmp);
			if (tmp < 0)
				tmp = 0;
			*temp = tmp * 1000;
		}
	}

	return ret;
}

int ipa_get_tsensor_id(const char *name)
{
	int ret = -ENODEV;
	u32 id = 0;
	u32 sensor_num = sizeof(hisi_tsensor_name)/sizeof(char *);

	pr_info("IPA sensor_num =%d\n", sensor_num);

	for (id = 0; id < sensor_num; id++) {
		pr_info("IPA: sensor_name=%s, hisi_tsensor_name[%d]=%s\n", name, id, hisi_tsensor_name[id]);

		if (!strncmp(name, hisi_tsensor_name[id], sizeof(name) - 1)) {
			ret = id;
			pr_info("sensor_id=%d\n", ret);
			return ret;/*break;*/
		}
	}

#ifdef CONFIG_HISI_THERMAL_SHELL
	if (!strncmp(name, IPA_SENSOR_SHELL, strlen(IPA_SENSOR_SHELL)))
		ret = IPA_SENSOR_SHELLID;
#endif

	return ret;

}
EXPORT_SYMBOL_GPL(ipa_get_tsensor_id);

int ipa_get_sensor_value(u32 sensor, int *val)
{
	int ret = -EINVAL;
	u32 id = 0;
	u32 sensor_num = sizeof(hisi_tsensor_name)/sizeof(char *);
	if (!val) {
		pr_err("[%s]parm null\n", __func__);
		return ret;
	}

	if (sensor < sensor_num)
		ret = tsens_get_temp(sensor, val);
	else
		pr_err("tsensor id=%d is not supported!\n", id);

	return ret;
}
EXPORT_SYMBOL_GPL(ipa_get_sensor_value);
/*end of add for IPA*/

static int tsens_tz_get_mode(struct thermal_zone_device *thermal,
			      enum thermal_device_mode *mode)
{
	struct tsens_tm_device_sensor *tm_sensor = thermal->devdata;

	if (!tm_sensor || !mode)
		return -EINVAL;

	*mode = tm_sensor->mode;

	return 0;
}

static int tsens_tz_get_trip_type(struct thermal_zone_device *thermal,
				   int trip, enum thermal_trip_type *type)
{
	struct tsens_tm_device_sensor *tm_sensor = thermal->devdata;
	int ret = 0;

	if (!tm_sensor || trip < 0 || !type)
		return -EINVAL;

	switch (trip) {
#ifdef CONFIG_HISI_THERMAL_TRIP
	case TSENS_TRIP_THROTTLING:
		*type = THERMAL_TRIP_THROTTLING;
		break;
	case TSENS_TRIP_SHUTDOWN:
		*type = THERMAL_TRIP_SHUTDOWN;
		break;
	case TSENS_TRIP_BELOW_VR_MIN:
		*type = THERMAL_TRIP_BELOW_VR_MIN;
		break;
#endif
	default:
		ret = -EINVAL;
	}

	return ret;
}
/*lint -e764 -e527 -esym(764,527,*)*/
static int tsens_tz_get_trip_temp(struct thermal_zone_device *thermal,
				   int trip, int *temp)
{
	struct tsens_tm_device_sensor *tm_sensor = thermal->devdata;
	int ret = 0;

	if (!tm_sensor || trip < 0 || !temp)
		return -EINVAL;

	switch (trip) {
#ifdef CONFIG_HISI_THERMAL_TRIP
	case TSENS_TRIP_THROTTLING:
		*temp = tm_sensor->temp_throttling;
		break;
	case TSENS_TRIP_SHUTDOWN:
		*temp = tm_sensor->temp_shutdown;
		break;
	case TSENS_TRIP_BELOW_VR_MIN:
		*temp = tm_sensor->temp_below_vr_min;
		break;
#endif
	default:
		ret = -EINVAL;
	}

	return ret;
}
/*lint -e764 -e527 +esym(764,527,*)*/

static struct thermal_zone_device_ops tsens_thermal_zone_ops = {
	.get_temp = tsens_tz_get_temp,
	.get_mode = tsens_tz_get_mode,
	.get_trip_type = tsens_tz_get_trip_type,
	.get_trip_temp = tsens_tz_get_trip_temp,
};

static int get_device_tree_data(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct device_node *of_node = pdev->dev.of_node; /*lint !e578*/
	unsigned int register_info = 0;
	u32 rc = 0;
	int tsens_num_sensors = 0;

	/* parse .hisi tsensor number */
	rc = of_property_read_s32(of_node, "hisi,sensors", &tsens_num_sensors);
	if (rc) {
		dev_err(&pdev->dev, "missing sensor number\n");
		return -ENODEV;
	}

	g_tmdev = devm_kzalloc(&pdev->dev, sizeof(struct tsens_tm_device) +
			tsens_num_sensors * sizeof(struct tsens_tm_device_sensor), GFP_KERNEL);
	if (g_tmdev == NULL) {
		dev_err(&pdev->dev, "kzalloc() failed.\n");
		return -ENOMEM;
	}

	g_tmdev->tsens_num_sensor = tsens_num_sensors;

	rc = (u32)of_property_read_u32(of_node, "hisi,tsensor_adc_start_value", &register_info);
	if (rc) {
		dev_err(dev, "no hisi,tsensor_adc_start_value!\n");
		goto dt_parse_common_end;
	}
	g_tmdev->adc_start_value = register_info;

	rc = (u32)of_property_read_u32(of_node, "hisi,tsensor_adc_end_value", &register_info);
	if (rc) {
		dev_err(dev, "no hisi,tsensor_adc_end_value!\n");
		goto dt_parse_common_end;
	}
	g_tmdev->adc_end_value = register_info;

	rc = (u32)of_property_read_u32(of_node, "hisi,pvtsensor_adc_start_value", &register_info);
	if (rc) {
		dev_err(dev, "no hisi,pvtsensor_adc_start_value!\n");
		g_tmdev->pvtsensor_adc_start_value = 0;
	} else
		g_tmdev->pvtsensor_adc_start_value = register_info;

	rc = (u32)of_property_read_u32(of_node, "hisi,pvtsensor_adc_end_value", &register_info);
	if (rc) {
		dev_err(dev, "no hisi,pvtsensor_adc_end_value!\n");
		g_tmdev->pvtsensor_adc_end_value = 0;
	} else
		g_tmdev->pvtsensor_adc_end_value = register_info;

	return 0;
dt_parse_common_end:
	return -EINVAL;
}

static int tsens_tm_probe(struct platform_device *pdev)
{
	int rc = 0;

	if (g_tmdev) {
		dev_err(&pdev->dev, "TSENS device already in use\n");
		return -EBUSY;
	}

	if (pdev->dev.of_node) {
		rc = get_device_tree_data(pdev);
		if (rc) {
			dev_err(&pdev->dev, "Error reading TSENS DT\n");
			return rc;
		}
	} else
		return -ENODEV;
	g_tmdev->pdev = pdev;

	platform_set_drvdata(pdev, g_tmdev);

	return 0;
}

static int _tsens_register_thermal(void)
{
	struct platform_device *pdev;
	int rc = 0, i, j;
	char name[18];
	int trips_num = 0;
#ifdef CONFIG_HISI_THERMAL_TRIP
	struct device_node *np;
	char node_name[30] = {0};
	s32 temp_throttling, temp_shutdown, temp_below_vr_min;
#endif
	struct device_node *node;
	char reg_no_name[40];
	int reg_no = 0;
	char sensor_type_name[40];
	int sensor_type = 0;

	memset(name, 0, sizeof(name));
	if (!g_tmdev) {
		pr_err("TSENS early init not done!\n");
		return -ENODEV;
	}

	pdev = g_tmdev->pdev;
	node = pdev->dev.of_node;

	for (i = 0, j = 0; i < g_tmdev->tsens_num_sensor; i++, j++) {
		int mask = 0;
		memset((void *)name, 0, sizeof(name));
		snprintf(name, sizeof(name), hisi_tsensor_name[i]); /*lint !e592*/
		g_tmdev->sensor[i].mode = THERMAL_DEVICE_ENABLED;
		g_tmdev->sensor[i].sensor_num = i;

		memset(reg_no_name, 0, sizeof(reg_no_name));
		snprintf(reg_no_name, sizeof(reg_no_name), "hisi,detect_%s_regno", name);
		rc = of_property_read_s32(node, reg_no_name, &reg_no);
		if (rc) {
			dev_err(&pdev->dev, "%s node not found!\n", reg_no_name);
			reg_no = -1;
		}
		g_tmdev->sensor[i].reg_no = reg_no;

		memset(sensor_type_name, 0, sizeof(sensor_type_name));
		snprintf(sensor_type_name, sizeof(sensor_type_name), "hisi,detect_%s_sensortype", name);
		rc = of_property_read_s32(node, sensor_type_name, &sensor_type);
		if (rc) {
			dev_err(&pdev->dev, "%s node not found, use default type!\n", sensor_type_name);
			sensor_type = 0; /*default tsensor*/
		}
		g_tmdev->sensor[i].sensor_type = sensor_type;

#ifdef CONFIG_HISI_THERMAL_TRIP
		memset((void *)node_name, 0, sizeof(node_name));
		snprintf(node_name, sizeof(node_name), "hisi_tsens_%s",  hisi_tsensor_name[i]);
		np = of_find_node_by_name(node, node_name);
		if (np) {
			rc = of_property_read_s32(np, "temp_throttling", &temp_throttling);
			if (rc) {
				dev_err(&pdev->dev, "temp_throttling node not found!\n");
				temp_throttling = 0;
			}
			g_tmdev->sensor[i].temp_throttling = temp_throttling;

			rc = of_property_read_s32(np, "temp_shutdown", &temp_shutdown);
			if (rc) {
				dev_err(&pdev->dev, "temp_shutdown node not found!\n");
				temp_shutdown = 0;
			}
			g_tmdev->sensor[i].temp_shutdown = temp_shutdown;

			rc = of_property_read_s32(np, "temp_below_vr_min", &temp_below_vr_min);
			if (rc) {
				dev_err(&pdev->dev, "temp_below_vr_min node not found!\n");
				temp_below_vr_min = 0;
			}
			g_tmdev->sensor[i].temp_below_vr_min = temp_below_vr_min;

			of_node_put(np);

			trips_num = TSENS_TRIP_NUM;
		} else {
			trips_num = TSENS_TRIP_ORIGNUM;
		}
#else
		trips_num = TSENS_TRIP_ORIGNUM;
#endif
		mask |= ((1 << trips_num) - 1);

		g_tmdev->sensor[i].tz_dev = thermal_zone_device_register(name,
				trips_num, mask, &g_tmdev->sensor[i],
				&tsens_thermal_zone_ops, NULL, 0, 0);
		if (IS_ERR(g_tmdev->sensor[i].tz_dev)) {
			dev_err(&pdev->dev, "Tsensor thermal_zone_device_register() failed\n");
			rc = -ENODEV;
			goto register_fail;
		}
	}

	platform_set_drvdata(pdev, g_tmdev);

	return 0;

register_fail:
	for (i = 0; i < j; i++)
		thermal_zone_device_unregister(g_tmdev->sensor[i].tz_dev);
	return rc;
}

static int tsens_tm_remove(struct platform_device *pdev)
{
	int i;
	struct tsens_tm_device *tmdev = platform_get_drvdata(pdev);
	if (tmdev == NULL)
		return -1;

	for (i = 0; i < tmdev->tsens_num_sensor; i++)
		thermal_zone_device_unregister(tmdev->sensor[i].tz_dev);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct of_device_id tsens_match[] = {
	{.compatible = "hisi,tsens",
	},
	{}
};

static struct platform_driver tsens_tm_driver = {
	.probe = tsens_tm_probe,
	.remove = tsens_tm_remove,
	.driver = {
		.name = "hisi-tsens",
		.owner = THIS_MODULE,
		.of_match_table = tsens_match,
	},
};

static int __init tsens_tm_init_driver(void)
{
	return platform_driver_register(&tsens_tm_driver);
}
arch_initcall(tsens_tm_init_driver);

static int __init tsens_thermal_register(void)
{
	return _tsens_register_thermal();
}
module_init(tsens_thermal_register);

static void __exit _tsens_tm_remove(void)
{
	platform_driver_unregister(&tsens_tm_driver);
}
module_exit(_tsens_tm_remove);

MODULE_ALIAS("platform:" TSENS_DRIVER_NAME);
MODULE_LICENSE("GPL v2");
