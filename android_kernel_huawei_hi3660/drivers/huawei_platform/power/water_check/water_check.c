/*
 * drivers/power/water_check/water_check.c
 * Copyright (C) 2012-2018 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/pinctrl/consumer.h>
#include <huawei_platform/power/huawei_charger.h>
#include <huawei_platform/log/hw_log.h>

#define WATER_IN          (1)
#define WATER_NULL        (0)
#define WATER_CHECK_PARA_LEVEL    (8)
#define SINGLE_POSITION        (1)
#define WATERCHECK_DMDLOG_SIZE    (60)
#define WATER_CHECK_DSM_IGNORE_NUM    (99)
#define DEBOUNCE_TIME        (3000)
#define DSM_NEED_REPORT    (1)
#define DSM_REPORTED       (0)
#define GPIO_NAME_SIZE     (16)
#define BATFET_DISABLED_ACTION  (1)

#define HWLOG_TAG water_check
HWLOG_REGIST();

static struct water_check_info *g_info = NULL;
static struct wake_lock g_wc_wakelock;
enum water_check_core_para_info{
	WATER_CHECK_TYPE = 0,
	WATER_CHECK_GPIO_NAME,
	WATER_CHECK_IRQ_NO,
	WATER_MULTIPLE_HANDLE,
	WATER_DMD_NO_OFFSET,
	WATER_IS_PROMPT,
	WATER_CHECK_ACTION,
	WATER_CHECK_PARA_TOTAL,
};
struct water_check_core_para{
	int check_type;
	char gpio_name[GPIO_NAME_SIZE];
	int irq_no;
	int dmd_no_offset;
	int gpio_no;
	u8 multiple_handle;
	u8 prompt;
	u8 action;
};
struct water_check_core_data{
	int total_type;
	struct water_check_core_para detect_para[WATER_CHECK_PARA_LEVEL];
};
struct water_check_info {
	struct device *dev;
	u32 need_pinctrl_config;
	struct pinctrl *pinctrl;
	struct delayed_work water_detect_work;
	u8 last_check_status[WATER_CHECK_PARA_LEVEL];
	u8 dsm_report_status[WATER_CHECK_PARA_LEVEL];
	char dsm_buff[WATERCHECK_DMDLOG_SIZE];
	struct water_check_core_data water_check_data;
};

int is_water_intrused(void)
{
	int i = 0;
	int gpio_value = 1;
	struct water_check_info *info = g_info;

	if(NULL == info)
		return WATER_NULL;
	for(i = 0;i < info->water_check_data.total_type;i++)
	{
		if(!strncmp( info->water_check_data.detect_para[i].gpio_name,"usb",strlen("usb")) ){
			gpio_value = gpio_get_value(info->water_check_data.detect_para[i].gpio_no);
			if(!gpio_value){
				hwlog_info("water is detected in usb!\n");
				return WATER_IN;
			}
		}
	}
	return WATER_NULL;
}

static void water_intruded_handle(int index,struct water_check_info *info)
{
	char single_position_dsm_buff[WATERCHECK_DMDLOG_SIZE];
	if(NULL == info) {
		hwlog_err("%s:info null\n",__func__);
		return;
	}
	if(index < 0 || index > info->water_check_data.total_type) {
		hwlog_err("%s:index is invalid\n",__func__);
		return;
	}

	/*dsm report*/
	if(WATER_CHECK_DSM_IGNORE_NUM != info->water_check_data.detect_para[index].dmd_no_offset) {
		if(SINGLE_POSITION  == info->water_check_data.detect_para[index].check_type) {
			memset(single_position_dsm_buff,0,WATERCHECK_DMDLOG_SIZE);
			snprintf(single_position_dsm_buff, WATERCHECK_DMDLOG_SIZE,
						"water check is triggered in: %s",info->water_check_data.detect_para[index].gpio_name);
			hwlog_info("[%s]:single_position_dsm_buff:%s\n",__func__, single_position_dsm_buff);

			  //single position dsm report one time
			power_dsm_dmd_report(POWER_DSM_BATTERY, \
			ERROR_NO_WATER_CHECK_BASE+info->water_check_data.detect_para[index].dmd_no_offset, \
			single_position_dsm_buff);
			msleep(150);
			info->dsm_report_status[index] = DSM_REPORTED;
		}
		else {
			hwlog_info("[%s]:multiple_intruded_dsm_buff:%s\n",__func__,info->dsm_buff);
			power_dsm_dmd_report(POWER_DSM_BATTERY, \
				ERROR_NO_WATER_CHECK_BASE+info->water_check_data.detect_para[index].dmd_no_offset, \
				info->dsm_buff);
		}
	}
	/*other functions*/
	switch (info->water_check_data.detect_para[index].action) {
		case BATFET_DISABLED_ACTION:
			hwlog_info("%s: do charge_set_batfet_disable!\n",__func__);
			msleep(50);
			charge_set_batfet_disable(true);
			break;
		default:
			break;
	}

}

static void water_detect_work(struct work_struct *work)
{
	int i;
	u8 gpio_val = 0;
	int water_intruded_num = 0;
	struct water_check_info *info = NULL;

	info = container_of(work, struct water_check_info,water_detect_work.work);
	if(NULL == info) {
		hwlog_err("%s:info null\n",__func__);

		/*enable irq*/
		for(i = 0;i < info->water_check_data.total_type;i++) {
			if(SINGLE_POSITION == info->water_check_data.detect_para[i].check_type)
				enable_irq(info->water_check_data.detect_para[i].irq_no);
		}
		wake_unlock(&g_wc_wakelock);
		return;
	}

	memset(&info->dsm_buff,0,WATERCHECK_DMDLOG_SIZE);
	snprintf(info->dsm_buff,WATERCHECK_DMDLOG_SIZE,"water check is triggered in: ");
	hwlog_info("%s:start\n",__func__);

	for(i = 0;i < info->water_check_data.total_type;i++) {
		if(SINGLE_POSITION == info->water_check_data.detect_para[i].check_type) {
			gpio_val = gpio_get_value_cansleep(info->water_check_data.detect_para[i].gpio_no);
			if(!gpio_val) {
				if(info->water_check_data.detect_para[i].multiple_handle) {
					water_intruded_num++;
					snprintf(info->dsm_buff+strlen(info->dsm_buff),WATERCHECK_DMDLOG_SIZE,"%s ",
						info->water_check_data.detect_para[i].gpio_name);
				}
				if((gpio_val ^ (info->last_check_status[i]))
					|| ((info->dsm_report_status[i])))
					water_intruded_handle(i,info);
			}
			if(gpio_val) {
				info->dsm_report_status[i] = DSM_NEED_REPORT;
			}
			info->last_check_status[i] = gpio_val;
		}
	}
	/*multiple intruded*/
	if(water_intruded_num > SINGLE_POSITION) {
		for(i = 0;i < info->water_check_data.total_type;i++) {
			if(water_intruded_num == info->water_check_data.detect_para[i].check_type) {
				water_intruded_handle(i,info);
			}
		}
	}

	/*enable irq*/
	for(i = 0;i < info->water_check_data.total_type;i++) {
		if(SINGLE_POSITION == info->water_check_data.detect_para[i].check_type)
			enable_irq(info->water_check_data.detect_para[i].irq_no);
	}
	wake_unlock(&g_wc_wakelock);
	hwlog_info("%s:end\n",__func__);

}

static irqreturn_t water_check_irq_handler(int irq, void *p)
{
	int i;
	struct water_check_info *info = (struct water_check_info *)p;
	if(!info) {
		hwlog_err("info null\n");
		return IRQ_NONE;
	}

	wake_lock(&g_wc_wakelock);
	hwlog_info("%s  irq_no:%d++ \n",__func__,irq );
	for(i = 0;i < info->water_check_data.total_type;i++)
	{
		if(SINGLE_POSITION == info->water_check_data.detect_para[i].check_type)
			disable_irq_nosync(info->water_check_data.detect_para[i].irq_no);
	}

	schedule_delayed_work(&info->water_detect_work, msecs_to_jiffies(DEBOUNCE_TIME));

	hwlog_info("%s  irq_no:%d-- \n",__func__,irq );
	return IRQ_HANDLED;

}

static int water_check_gpio_config(struct  water_check_info *info)
{
	int i = 0;
	int ret = -1;
	if(NULL == info || NULL == info->dev) {
		hwlog_err("info null\n");
		return -ENOMEM;
	}

	/*request gpio & register interrupt handler function*/
	for(i = 0;i  < info->water_check_data.total_type;i++) {
		if(SINGLE_POSITION == info->water_check_data.detect_para[i].check_type) {
			ret = devm_gpio_request_one(info->dev,(unsigned int)info->water_check_data.detect_para[i].gpio_no,
									(unsigned long)GPIOF_DIR_IN,info->water_check_data.detect_para[i].gpio_name);
			if (ret < 0) {
				hwlog_err("failled to request gpio:%d\n",info->water_check_data.detect_para[i].gpio_no);
				goto err_gpio_config;
			}
			info->water_check_data.detect_para[i].irq_no = (unsigned int)gpio_to_irq(info->water_check_data.detect_para[i].gpio_no);
			if(info->water_check_data.detect_para[i].irq_no < 0) {
				hwlog_err("gpio_to_irq:%d  failed\n",info->water_check_data.detect_para[i].gpio_no);
				ret = -EAGAIN;
				goto err_gpio_config;
			}
			info->last_check_status[i] = gpio_get_value_cansleep(info->water_check_data.detect_para[i].gpio_no);
			info->dsm_report_status[i] = DSM_NEED_REPORT;

			ret = devm_request_irq(info->dev,(unsigned int)info->water_check_data.detect_para[i].irq_no,
						water_check_irq_handler,IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
						| IRQF_NO_SUSPEND | IRQF_ONESHOT, info->water_check_data.detect_para[i].gpio_name, info);
			if (ret < 0) {
				hwlog_err("failed to register gpio_%d irq\n",info->water_check_data.detect_para[i].gpio_no);
				goto err_gpio_config;
			}
			disable_irq_nosync(info->water_check_data.detect_para[i].irq_no);
		}
	}

	for(i = 0;i < info->water_check_data.total_type;i++) {
		if(SINGLE_POSITION == info->water_check_data.detect_para[i].check_type)
			hwlog_info("position:%s update irq_no:%d gpio_status:%d dsm_status:%d\n",info->water_check_data.detect_para[i].gpio_name,
					info->water_check_data.detect_para[i].irq_no,info->last_check_status[i],info->dsm_report_status[i]);
	}

err_gpio_config:
	return ret;

}

static int water_check_parse_extra_dts(struct device_node *np,struct water_check_info *info)
{
	int i,j;
	int ret = -1;
	int array_len = 0;
	char * tmp_string = NULL;
	char tmp_gpio_name[GPIO_NAME_SIZE];

	/*water_check para*/
	array_len = of_property_count_strings(np,"water_check_para");
	if ((array_len <= 0) ||(array_len % WATER_CHECK_PARA_TOTAL!= 0)) {
		info-> water_check_data.total_type = 0;
		hwlog_err("water_check_para error, please check it!!\n");
	} else if (array_len > WATER_CHECK_PARA_TOTAL*WATER_CHECK_PARA_LEVEL) {
		info-> water_check_data.total_type = 0;
		hwlog_err("water_check_para is too long(%d)!!\n" , array_len);
	} else {
			info->water_check_data.total_type = (int)(array_len/WATER_CHECK_PARA_TOTAL);
			hwlog_info("water_check_total_type = %d\n",info->water_check_data.total_type);
			for(i = 0;i < info->water_check_data.total_type;i++) {
				for(j = 0;j < WATER_CHECK_PARA_TOTAL;j++) {
					ret = of_property_read_string_index(np,"water_check_para",i * WATER_CHECK_PARA_TOTAL + j,&tmp_string);
					if(ret) {
						hwlog_err("get water_check_para failed\n");
						return -EINVAL;
					}
					switch (j) {
					case WATER_CHECK_TYPE:
						info->water_check_data.detect_para[i].check_type =
								simple_strtol(tmp_string, NULL, 10);
						break;
					case WATER_CHECK_GPIO_NAME:
						strncpy(info->water_check_data.detect_para[i].gpio_name,tmp_string,sizeof(info->water_check_data.detect_para[i].gpio_name));

						if(SINGLE_POSITION== info->water_check_data.detect_para[i].check_type) {
							memset(tmp_gpio_name,0,GPIO_NAME_SIZE);
							snprintf(tmp_gpio_name,sizeof(tmp_gpio_name),"gpio_%s",info->water_check_data.detect_para[i].gpio_name);
							info->water_check_data.detect_para[i].gpio_no = of_get_named_gpio(np,tmp_gpio_name,0);
							if(!gpio_is_valid(info->water_check_data.detect_para[i].gpio_no)) {
								hwlog_err("get gpio_%s no failed\n",info->water_check_data.detect_para[i].gpio_name);
								return -EINVAL;
							}
						}
						break;
					case WATER_CHECK_IRQ_NO:
						info->water_check_data.detect_para[i].irq_no =
								simple_strtol(tmp_string, NULL, 10);
						break;
					case WATER_MULTIPLE_HANDLE:
						info->water_check_data.detect_para[i].multiple_handle =
								simple_strtol(tmp_string, NULL, 10);
						break;
					case WATER_DMD_NO_OFFSET:
						info->water_check_data.detect_para[i].dmd_no_offset =
								simple_strtol(tmp_string, NULL, 10);
						break;
					case WATER_IS_PROMPT:
						info->water_check_data.detect_para[i].prompt =
								simple_strtol(tmp_string, NULL, 10);
						break;
					case WATER_CHECK_ACTION:
						info->water_check_data.detect_para[i].action =
								simple_strtol(tmp_string, NULL, 10);
						break;
					}
				}
			}
			for(i = 0;i < info->water_check_data.total_type;i++) {
				hwlog_info("water_check_para[%d]  check_type:%d  gpio_no:%-3d  gpio_name:%s  irq_no:%d multiple_handle:%d dmd_no_offset:%d  prompt:%d  action:%d \n",
							i,info->water_check_data.detect_para[i].check_type,info->water_check_data.detect_para[i].gpio_no,
							info->water_check_data.detect_para[i].gpio_name,info->water_check_data.detect_para[i].irq_no,
							info->water_check_data.detect_para[i].multiple_handle,info->water_check_data.detect_para[i].dmd_no_offset,
							info->water_check_data.detect_para[i].prompt,info->water_check_data.detect_para[i].action);
			}
	}
	return ret;
}

static int water_check_probe(struct platform_device *pdev)
{
	int ret = -1;
	struct water_check_info *info = NULL;
	struct device_node *np = NULL;
	struct pinctrl_state *pinctrl_def;

	info = devm_kzalloc(&pdev->dev,sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	g_info = info;
	info->dev = &pdev->dev;
	np = pdev->dev.of_node;
	if(NULL == np || NULL == info->dev) {
		hwlog_err("device_node is NULL!\n");
		return -ENOMEM;
	}

	ret = of_property_read_u32(np,"need_pinctrl_config",&info->need_pinctrl_config);
	if (ret) {
		hwlog_err("get need_pinctrl_config failed\n");
		return -EINVAL;
	}
	if(info->need_pinctrl_config) {
		info->pinctrl = devm_pinctrl_get(&pdev->dev);
		if (IS_ERR(info->pinctrl)) {
			hwlog_err("failed to get pinctrl\n");
			goto err_pinctrl_get;
		}

		pinctrl_def = pinctrl_lookup_state(info->pinctrl, "default");
		if (IS_ERR(pinctrl_def)) {
			hwlog_err("failed to get pinctrl_def\n");
			goto err_pinctrl_look_state;
		}

		/* configure the gpios as no-pull state */
		ret = pinctrl_select_state(info->pinctrl, pinctrl_def);
		if (ret) {
			hwlog_err("failed to set pins to default state\n");
			goto err_pinctrl_set;
		}
	}

	ret = water_check_parse_extra_dts(np,info);
	if(ret < 0) {
		hwlog_err("get water_check_para from dts failed!\n");
		goto err_parse_dts;
	}

	ret = water_check_gpio_config(info);
	if(ret < 0){
		hwlog_err("water_check_gpio_config failed!\n");
		goto err_gpio_config;
	}

	wake_lock_init(&g_wc_wakelock,WAKE_LOCK_SUSPEND, "watercheck_wakelock");
	INIT_DELAYED_WORK(&info->water_detect_work, water_detect_work);
	schedule_delayed_work(&info->water_detect_work, msecs_to_jiffies(8000));//used only here
	dev_set_drvdata(&pdev->dev, info);

	hwlog_info("water check driver probes successfully\n");
	return ret;

err_gpio_config:
err_parse_dts:
err_pinctrl_set:
err_pinctrl_look_state:
	if(info->need_pinctrl_config)
		devm_pinctrl_put(info->pinctrl);
err_pinctrl_get:
	devm_kfree(&pdev->dev, info);
	g_info = NULL;
	hwlog_err("water check driver probes failed\n");
	return ret;
}

static int water_check_remove(struct platform_device *pdev)
{
	struct water_check_info *info = dev_get_drvdata(&pdev->dev);

	wake_lock_destroy(&g_wc_wakelock);
	cancel_delayed_work(&info->water_detect_work);
	if(info->need_pinctrl_config)
		devm_pinctrl_put(info->pinctrl);
	devm_kfree(&pdev->dev, info);
	dev_set_drvdata(&pdev->dev, NULL);
	g_info = NULL;

	return 0;
}

static struct of_device_id water_check_of_match[] = {
	{.compatible = "hisilicon,water_check",},
	{},
};

static struct platform_driver water_check_drv = {
	.probe		= water_check_probe,
	.remove		= water_check_remove,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "water_check",
		.of_match_table	= water_check_of_match,
	},
};

static int __init water_check_init(void)
{
	return platform_driver_register(&water_check_drv);
}

static void __exit water_check_exit(void)
{
	platform_driver_unregister(&water_check_drv);
}

/*
 * The gpios used for ear and usb water check are codec hi6402 gpio0 and gpio2,
 * while the codec hi6402 driver will change the gpio0 configuration, so call
 * our probe function later to make sure our configuration takes effect.
 */
late_initcall_sync(water_check_init);
module_exit(water_check_exit);

MODULE_DESCRIPTION("This module uses for water check at phone positon of ear sim key or usb port");
MODULE_LICENSE("GPL v2");
