/*
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <huawei_platform/log/hw_log.h>
#include <../cpu_buck.h>

#define HWLOG_TAG hi6422v200
#define HI6422v200_REG_SIZE 3
#define HI6422v200_PMU1_TYPE "0005_"
#define HI6422v200_PMU2_TYPE "0105_"

HWLOG_REGIST();


static char hi6422v200_pmu1_reg_val[HI6422v200_REG_SIZE];
static char hi6422v200_pmu2_reg_val[HI6422v200_REG_SIZE];
static int g_pmu1_flag = 0;
static int g_pmu2_flag = 0;
static struct cpu_buck_sample hi6422v200_pmu1;
static struct cpu_buck_sample hi6422v200_pmu2;

static struct cpu_buck_info hi6422v200_error_array[] =
{
	/*NP_IRQ1_RECORD_REG*/
	{HI6422V200_VSYS_PWRON_D60UR, 0x10, 0, "HI6422V200_VSYS_PWRON_D60UR"},
	{HI6422V200_VSYS_OV_D200UR, 0x08, 0, "HI6422V200_VSYS_OV_D200UR"},
	{HI6422V200_VSYS_PWROFF_ABS_2D, 0x04, 0, "HI6422V200_VSYS_PWROFF_ABS_2D"},
	{HI6422V200_THSD_OTMP125_D1MR, 0x02, 0, "HI6422V200_THSD_OTMP125_D1MR"},
	{HI6422V200_THSD_OTMP140_D180UR, 0x01, 0, "HI6422V200_THSD_OTMP140_D180UR"},
	/*NP_OCP_RECORD_REG*/
	{HI6422V200_BUCK3_OCP, 0x08, 1, "HI6422V200_BUCK3_OCP"},
	{HI6422V200_BUCK2_OCP, 0x04, 1, "HI6422V200_BUCK2_OCP"},
	{HI6422V200_BUCK1_OCP, 0x02, 1, "HI6422V200_BUCK1_OCP"},
	{HI6422V200_BUCK0_OCP, 0x01, 1, "HI6422V200_BUCK0_OCP"},
	/*NP_SCP_RECORD_REG*/
	{HI6422V200_BUCK3_SCP, 0x08, 2, "HI6422V200_BUCK3_SCP"},
	{HI6422V200_BUCK2_SCP, 0x04, 2, "HI6422V200_BUCK2_SCP"},
	{HI6422V200_BUCK1_SCP, 0x02, 2, "HI6422V200_BUCK1_SCP"},
	{HI6422V200_BUCK0_SCP, 0x01, 2, "HI6422V200_BUCK0_SCP"},
};

static int __init early_parse_cpu_buck_reg_cmdline(char * p)
{
	char* start;
	int i;
	hwlog_info("cpu_buck_reg = %s\n", p);
	if (!p)
	{
		hwlog_info("cpu_buck_reg pointer is NULL\n");
		return 0;
	}
	start = strstr(p, HI6422v200_PMU1_TYPE);
	if (NULL != start)
	{
		g_pmu1_flag = 1;
		str_to_reg(start+5, hi6422v200_pmu1_reg_val, HI6422v200_REG_SIZE);
		for (i = 0; i < HI6422v200_REG_SIZE; ++i)
		{
			hwlog_info("pmu1 reg[%d] = 0x%x\n", i, hi6422v200_pmu1_reg_val[i]);
		}
	}
	start = strstr(p, HI6422v200_PMU2_TYPE);
	if (NULL != start)
	{
		g_pmu2_flag = 1;
		str_to_reg(start+5, hi6422v200_pmu2_reg_val, HI6422v200_REG_SIZE);
		for (i = 0; i < HI6422v200_REG_SIZE; ++i)
		{
			hwlog_info("pmu2 reg[%d] = 0x%x\n", i, hi6422v200_pmu2_reg_val[i]);
		}
	}
	if ((0 == g_pmu1_flag) && (0 == g_pmu2_flag))
	{
		hwlog_info("NO PMU ERR FOUND!\n");
	}
	else
	{
		g_fault_happened = 1;
	}
	return 0;
}

early_param("cpu_buck_reg", early_parse_cpu_buck_reg_cmdline);


static int hi6422v200_probe(struct platform_device *pdev)
{
	struct device_node* np;
	np = pdev->dev.of_node;
	if (NULL == np)
	{
		hwlog_err("NP is NULL\n");
		return -1;
	}
	if (1 == g_pmu1_flag)
	{
		g_pmu1_flag = 0;
		hi6422v200_pmu1.cbs = NULL;
		hi6422v200_pmu1.cbi = hi6422v200_error_array;
		hi6422v200_pmu1.reg = hi6422v200_pmu1_reg_val;
		hi6422v200_pmu1.info_size = sizeof(hi6422v200_error_array) / sizeof(struct cpu_buck_info);
		hi6422v200_pmu1.cpu_buck_number = HI6422v200_PMU1;
		cpu_buck_register(&hi6422v200_pmu1);
		hwlog_info("hi6422v200 pmu1 register\n");
	}
	if (1 == g_pmu2_flag)
	{
		g_pmu2_flag = 0;
		hi6422v200_pmu2.cbs = NULL;
		hi6422v200_pmu2.cbi = hi6422v200_error_array;
		hi6422v200_pmu2.reg = hi6422v200_pmu2_reg_val;
		hi6422v200_pmu2.info_size = sizeof(hi6422v200_error_array) / sizeof(struct cpu_buck_info);
		hi6422v200_pmu2.cpu_buck_number = HI6422v200_PMU2;
		cpu_buck_register(&hi6422v200_pmu2);
		hwlog_info("hi6422v200 pmu2 register\n");
	}
	hwlog_info("hi6422v200 register success\n");
	return 0;
}

static struct of_device_id hi6422v200_match_table[] =
{
	{
		.compatible = "huawei,hi6422v200_pmu1",
		.data = NULL,
	},
	{
		.compatible = "huawei,hi6422v200_pmu2",
		.data = NULL,
	},
	{
	},
};

static struct platform_driver hi6422v200_driver = {
	.probe = hi6422v200_probe,
	.driver = {
		.name = "huawei,hi6422v200",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hi6422v200_match_table),
	},
};

static int __init hi6422v200_init(void)
{
	return platform_driver_register(&hi6422v200_driver);
}

fs_initcall_sync(hi6422v200_init);

static void __exit hi6422v200_exit(void)
{
	platform_driver_unregister(&hi6422v200_driver);
}
module_exit(hi6422v200_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:hi6422v200");
MODULE_AUTHOR("HUAWEI Inc");
