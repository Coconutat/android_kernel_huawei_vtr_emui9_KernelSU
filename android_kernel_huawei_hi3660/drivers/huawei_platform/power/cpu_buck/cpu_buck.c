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

#include <huawei_platform/power/power_dsm.h>

#include <cpu_buck.h>
#ifdef CONFIG_HISI_COUL
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#endif

#define HWLOG_TAG cpu_buck
HWLOG_REGIST();

extern int g_fault_happened = 0;
static struct cpu_buck_smaple* g_cbs = NULL;

void str_to_reg(char* str, char* reg, int size)
{
	char high;
	char low;
	int i = 0;
	for (i = 0; i < size; ++i)
	{
		high = *(str + 2*i);
		low = *(str + 2*i + 1);
		high = (high > '9')?(high - 'a' + WEIGHT_10):(high - '0');
		low = (low > '9')?(low - 'a' + WEIGHT_10):(low - '0');
		*(reg + i) = (high << 4) | low;
	}
}
void cpu_buck_register(struct cpu_buck_sample* p_cbs)
{
	struct cpu_buck_sample* cbs;
	if (NULL == g_cbs)
	{
		g_cbs = p_cbs;
	}
	else
	{
		cbs = g_cbs;
		while(cbs->cbs)
		{
			cbs = cbs->cbs;
		}
		cbs->cbs = p_cbs;
	}
}
static int __init early_parse_normal_reset_type_cmdline(char * p)
{
	if (!p)
		return 0;
	hwlog_info("normal_reset_type = %s\n",p);
	if (!strncmp(p,"CPU_BUCK", 8))
	{
		hwlog_info("find CPU_BUCK\n");
	}
	return 0;
}
early_param("normal_reset_type", early_parse_normal_reset_type_cmdline);

static void update_err_msg(struct cpu_buck_info* cbi)
{
	if(NULL == cbi){
		hwlog_err("[%s] input pointer:cbi is NULL!\n",__func__);
		return ;
	}

	char extra_err_msg[MAX_EXTRA_ERR_MSG_LEN] = {0};
	int batt_temp = INVALID_TEMP_VAL;
	int batt_vol = 0;
	int batt_soc = 0;

	if(HI6422V200_VSYS_PWRON_D60UR == cbi->err_no || HI6422V200_VSYS_PWROFF_ABS_2D == cbi->err_no){
		batt_temp = hisi_battery_temperature();
		batt_vol = hisi_battery_voltage();
		batt_soc = hisi_battery_capacity();
		snprintf(extra_err_msg,MAX_EXTRA_ERR_MSG_LEN-1," batt_temp = %d,batt_vol = %d,batt_soc = %d\n",batt_temp,batt_vol,batt_soc);
	}
	if(strlen(extra_err_msg) > 0){
		strncat(cbi->error_info, extra_err_msg, strlen(extra_err_msg));
	}
}
static void cpu_buck_work(struct work_struct *work)
{
	int i = 0;
	struct cpu_buck_sample* cbs = g_cbs;
	bool already_notified = false;
	hwlog_info("g_fault happened = %d\n", g_fault_happened);
	if (!g_fault_happened)
		return;

	while(cbs)
	{
		for (i = 0; i < cbs->info_size; ++i)
		{
			if ((cbs->cbi[i].error_mask & cbs->reg[cbs->cbi[i].reg_number]) == cbs->cbi[i].error_mask)
			{
				if (!dsm_client_ocuppy(power_dsm_get_dclient(POWER_DSM_CPU_BUCK)))
				{
					hwlog_info("CPU BUCK EXCEPTION! \n");
					already_notified = true;
					hwlog_info("HI6422v200 PMU1:cpu_buck_number = 0; PMU2:cpu_buck_number = 1; \n");
					hwlog_info("cpu_buck_number = %d, record and notify: %s\n", cbs->cpu_buck_number,cbs->cbi[i].error_info);
					update_err_msg(&(cbs->cbi[i]));
					dsm_client_record(power_dsm_get_dclient(POWER_DSM_CPU_BUCK), "cpu_buck_number =  %d;cpu_buck %s happened!\n", cbs->cpu_buck_number,cbs->cbi[i].error_info);
					dsm_client_notify(power_dsm_get_dclient(POWER_DSM_CPU_BUCK), ERROR_NO_CPU_BUCK_BASE + cbs->cbi[i].err_no);
					break;
				}
			}
		}
		if (already_notified)
		{
			break;
		}
		cbs = cbs->cbs;
	}
}
static int cpu_buck_probe(struct platform_device *pdev)
{
	struct device_node* np;
	struct cpu_buck_device_info* di;
	np = pdev->dev.of_node;
	if(NULL == np)
	{
		hwlog_err("np is NULL\n");
		return -1;
	}
	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
	{
		hwlog_err("di is NULL\n");
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&di->cpu_buck_delayed_work, cpu_buck_work);
	schedule_delayed_work(&di->cpu_buck_delayed_work, 10);
	hwlog_info("cpu_buck probe ok!\n");
	return 0;
}
static struct of_device_id cpu_buck_match_table[] =
{
	{
		.compatible = "huawei,cpu_buck",
		.data = NULL,
	},
	{
	},
};
static struct platform_driver cpu_buck_driver = {
	.probe = cpu_buck_probe,
	.driver = {
		.name = "huawei,cpu_buck",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(cpu_buck_match_table),
	},
};
static int __init cpu_buck_init(void)
{
	return platform_driver_register(&cpu_buck_driver);
}

module_init(cpu_buck_init);

static void __exit cpu_buck_exit(void)
{
	platform_driver_unregister(&cpu_buck_driver);
}

module_exit(cpu_buck_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:cpu_buck");
MODULE_AUTHOR("HUAWEI Inc");
