/* Copyright (c) 2017-2018, Huawei terminal Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#include "hisi_fb.h"
#include "lcd_kit_common.h"
#include "lcd_kit_core.h"
#include "lcd_kit_disp.h"
#include "lcd_kit_sysfs.h"
#include "lcd_kit_sysfs_hs.h"
#include "lcd_kit_bl.h"
#include <linux/hisi/hw_cmdline_parse.h>

/*extern declare*/
extern int hisi_adc_get_value(int adc_channel);

static ssize_t lcd_model_show(struct device* dev,
										struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;

	if ((!runmode_is_factory()) && (NULL != common_info->panel_model)){
		return snprintf(buf, PAGE_SIZE, "%s\n", common_info->panel_model);
	}

	if (common_ops->get_panel_name) {
		ret = common_ops->get_panel_name(buf);
	}
	return ret;
}

static ssize_t lcd_type_show(struct device* dev,
										struct device_attribute* attr, char* buf)
{
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf){
		LCD_KIT_ERR("NULL_PTR ERROR!\n");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", is_mipi_cmd_panel(hisifd) ? 1 : 0);
}

static ssize_t lcd_panel_info_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_ops->get_panel_info) {
		ret = common_ops->get_panel_info(buf);
	}
	return ret;
}

static ssize_t lcd_fps_scence_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf) {
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	ret = snprintf(buf, PAGE_SIZE, "lcd_fps = %d \n", hisifd->panel_info.fps);
	return ret;
}

static ssize_t lcd_fps_scence_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	ssize_t ret = LCD_KIT_OK;
	unsigned long val = 0;
	struct hisi_fb_data_type* hisifd = NULL;
	struct hisi_fb_panel_data* pdata = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata ) {
		LCD_KIT_ERR("lcd fps scence store pdata NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf) {
		LCD_KIT_ERR("lcd fps scence store buf NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	val = simple_strtoul(buf, NULL, 0);
	if (!hisifd->panel_power_on) {
		LCD_KIT_ERR("fb%d, panel power off!\n", hisifd->index);
		return LCD_KIT_FAIL;
	}
	if (disp_info->fps.support) {
		if (pdata->lcd_fps_scence_handle) {
			ret = pdata->lcd_fps_scence_handle(hisifd->pdev, val);
		}
	}
	return count;
}

static ssize_t lcd_alpm_function_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf) {
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	if (disp_info->alpm.support) {
		ret = snprintf(buf, PAGE_SIZE, "aod_function = %d \n", hisifd->aod_function);
	}
	return ret;
}

static ssize_t lcd_alpm_function_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	ssize_t ret = LCD_KIT_OK;
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf) {
		LCD_KIT_ERR("lcd fps scence store buf NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	if (strlen(buf) >= MAX_BUF) {
		LCD_KIT_ERR("buf overflow!\n");
		return LCD_KIT_FAIL;
	}
	if (disp_info->alpm.support) {
		ret = sscanf(buf, "%u", &hisifd->aod_function);
		if (!ret) {
			LCD_KIT_ERR("sscanf return invaild:%d\n", ret);
			return LCD_KIT_FAIL;
		}
	}
	return count;
}

static ssize_t lcd_alpm_setting_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	ssize_t ret = LCD_KIT_OK;
	struct hisi_fb_data_type* hisifd = NULL;
	unsigned int mode = 0;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf) {
		LCD_KIT_ERR("lcd fps scence store buf NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	if (strlen(buf) >= MAX_BUF) {
		LCD_KIT_ERR("buf overflow!\n");
		return LCD_KIT_FAIL;
	}
	ret = sscanf(buf, "%u", &mode);
	if (!ret) {
		LCD_KIT_ERR("sscanf return invaild:%d\n", ret);
		return LCD_KIT_FAIL;
	}
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		if (disp_info->alpm.support) {
			hisifb_activate_vsync(hisifd);
			lcd_kit_alpm_setting(hisifd, mode);
			hisifb_deactivate_vsync(hisifd);
		}
	}
	up(&hisifd->blank_sem);
	return count;
}

static ssize_t lcd_inversion_mode_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;

	if (common_ops->inversion_get_mode) {
		ret = common_ops->inversion_get_mode(buf);
	}
	return ret;
}

static ssize_t lcd_inversion_mode_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	ssize_t ret = LCD_KIT_OK;
	unsigned long val = 0;
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf) {
		LCD_KIT_ERR("lcd fps scence store buf NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	val = simple_strtoul(buf, NULL, 0);
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		if (common_ops->inversion_set_mode) {
			hisifb_activate_vsync(hisifd);
			ret = common_ops->inversion_set_mode(hisifd, val);
			hisifb_deactivate_vsync(hisifd);
		}
	}
	up(&hisifd->blank_sem);
	return count;
}

static ssize_t lcd_scan_mode_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_ops->scan_get_mode) {
		ret = common_ops->scan_get_mode(buf);
	}
	return ret;
}

static ssize_t lcd_scan_mode_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	ssize_t ret = LCD_KIT_OK;
	unsigned long val = 0;
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf) {
		LCD_KIT_ERR("lcd fps scence store buf NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	val = simple_strtoul(buf, NULL, 0);
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		if (common_ops->scan_set_mode) {
			hisifb_activate_vsync(hisifd);
			ret = common_ops->scan_set_mode(hisifd, val);
			hisifb_deactivate_vsync(hisifd);
		}
	}
	up(&hisifd->blank_sem);
	return count;
}

static ssize_t lcd_check_reg_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		if (common_ops->check_reg) {
			hisifb_activate_vsync(hisifd);
			ret = common_ops->check_reg(hisifd, buf);
			hisifb_deactivate_vsync(hisifd);
		}
	}
	up(&hisifd->blank_sem);
	return ret;
}

static ssize_t lcd_gram_check_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;
	int checksum_result = 0;
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		if (disp_info->checksum.support) {
			hisifb_activate_vsync(hisifd);
			checksum_result = lcd_kit_checksum_check(hisifd);
			hisifb_deactivate_vsync(hisifd);
			ret = snprintf(buf, PAGE_SIZE, "%d\n", checksum_result);
		}
	}
	up(&hisifd->blank_sem);
	return ret;
}

static ssize_t lcd_gram_check_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	struct hisi_fb_data_type* hisifd = NULL;
	int ret = LCD_KIT_OK;
	unsigned long index = 0;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	ret = strict_strtoul(buf, 0, &index);
	if (ret) {
		return ret;
	}
	LCD_KIT_INFO("val=%ld\n", index);
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		if (disp_info->checksum.support) {
			hisifb_activate_vsync(hisifd);
			ret = lcd_kit_checksum_set(hisifd, index);
			hisifb_deactivate_vsync(hisifd);
		}
	}
	up(&hisifd->blank_sem);
	return count;
}

static ssize_t lcd_sleep_ctrl_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_ops->get_sleep_mode) {
		ret = common_ops->get_sleep_mode(buf);
	}
	return ret;
}

static ssize_t lcd_sleep_ctrl_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	int ret = LCD_KIT_OK;
	unsigned long val = 0;
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	ret = strict_strtoul(buf, 0, &val);
	if (ret) {
		LCD_KIT_ERR("invalid parameter!\n");
		return ret;
	}
	if (!hisifd->panel_power_on) {
		LCD_KIT_ERR("panel is power off!\n");
		return LCD_KIT_FAIL;
	}
	if (common_ops->get_sleep_mode) {
		ret = common_ops->set_sleep_mode(val);
	}
	return count;
}

static ssize_t lcd_hkadc_debug_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;

	if (disp_info->hkadc.support) {
		ret = snprintf(buf, PAGE_SIZE, "%d\n", disp_info->hkadc.value);
	}
	return ret;
}

static ssize_t lcd_hkadc_debug_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	int ret = LCD_KIT_OK;
	int channel = 0;

	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	if (disp_info->hkadc.support) {
		ret = sscanf(buf, "%u", &channel);
		if (ret) {
			LCD_KIT_ERR("ivalid parameter!\n");
			return ret;
		}
		disp_info->hkadc.value = hisi_adc_get_value(channel);
	}
	return count;
}

static ssize_t lcd_amoled_pcd_errflag_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	int ret = PCD_ERRFLAG_SUCCESS;
	int pcd_gpio = 0;
	int errflag_gpio = 0;
	u8 result_value = 0;

	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}

	if(disp_info->pcd_errflag_check_support) {
		if(disp_info->gpio_pcd != 0) {
			ret = gpio_request(disp_info->gpio_pcd,GPIO_LCDKIT_PCD_NAME);
			if(ret != 0) {
				result_value = PCD_ERRFLAG_FAIL;
				LCD_KIT_ERR("pcd__gpio[%d] request fail!\n",disp_info->gpio_pcd);
				goto err_out;
			}
			ret = gpio_direction_input(disp_info->gpio_pcd);
			if(ret != 0) {
				gpio_free(disp_info->gpio_pcd);
				result_value = PCD_ERRFLAG_FAIL;
				LCD_KIT_ERR("pcd__gpio[%d] direction set fail!\n",disp_info->gpio_pcd);
				goto err_out;
			}
			pcd_gpio = gpio_get_value(disp_info->gpio_pcd);
			gpio_free(disp_info->gpio_pcd);
		}
		if(disp_info->gpio_errflag != 0) {
			ret = gpio_request(disp_info->gpio_errflag,GPIO_LCDKIT_ERRFLAG_NAME  );
			if(ret != 0) {
				result_value = PCD_ERRFLAG_FAIL;
				LCD_KIT_ERR("pcd_errflag_gpio[%d] request fail!\n",disp_info->gpio_errflag);
				goto err_out;
			}
			ret = gpio_direction_input(disp_info->gpio_errflag);
			if(ret != 0) {
				gpio_free(disp_info->gpio_errflag);
				result_value = PCD_ERRFLAG_FAIL;
				LCD_KIT_ERR("pcd_errflag_gpio[%d] direction set fail!\n",disp_info->gpio_errflag);
				goto err_out;
			}
			errflag_gpio = gpio_get_value(disp_info->gpio_errflag);
			gpio_free(disp_info->gpio_errflag);
		}

		LCD_KIT_INFO("pcd__gpio[%d]= %d,pcd_errflag_gpio[%d]= %d\n",disp_info->gpio_pcd,pcd_gpio,disp_info->gpio_errflag,errflag_gpio);

		if ((pcd_gpio == GPIO_LOW_PCDERRFLAG) && (errflag_gpio == GPIO_LOW_PCDERRFLAG))
		{
			result_value = PCD_ERRFLAG_SUCCESS; // PCD_ERR_FLAG_SUCCESS
		}
		else if ((pcd_gpio == GPIO_HIGH_PCDERRFLAG) && (errflag_gpio == GPIO_LOW_PCDERRFLAG))
		{
			result_value = PCD_FAIL; //only  PCD_FAIL
		}
		else if ((pcd_gpio == GPIO_LOW_PCDERRFLAG) && (errflag_gpio == GPIO_HIGH_PCDERRFLAG))
		{
			result_value = ERRFLAG_FAIL; //only ERRFLAG FAIL
		}
		else if ((pcd_gpio == GPIO_HIGH_PCDERRFLAG) && (errflag_gpio == GPIO_HIGH_PCDERRFLAG))
		{
			result_value = PCD_ERRFLAG_FAIL; //PCD_ERR_FLAG_FAIL
		} else {
			result_value = PCD_ERRFLAG_SUCCESS;
		}
	} else {
		LCD_KIT_INFO("pcd_errflag check default pass\n");
	}

err_out:
	LCD_KIT_INFO("The test cmd is pcd_errflag,and the result_value = %d\n",result_value);
	ret = snprintf(buf, PAGE_SIZE, "%d\n", result_value);

	return ret;
}

static ssize_t lcd_amoled_acl_ctrl_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_ops->get_acl_mode) {
		ret = common_ops->get_acl_mode(buf);
	}
	return ret;
}

static ssize_t lcd_amoled_acl_ctrl_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	struct hisi_fb_data_type* hisifd = NULL;
	int ret = LCD_KIT_OK;
	unsigned long val = 0;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	ret = strict_strtoul(buf, 0, &val);
	if (ret) {
		LCD_KIT_ERR("invalid parameter!\n");
		return ret;
	}
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		if (common_ops->set_acl_mode) {
			hisifb_activate_vsync(hisifd);
			ret = common_ops->set_acl_mode(hisifd, val);
			hisifb_deactivate_vsync(hisifd);
		}
	}
	up(&hisifd->blank_sem);
	return count;
}

static ssize_t lcd_amoled_vr_mode_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_ops->get_vr_mode) {
		ret = common_ops->get_vr_mode(buf);
	}
	return ret;
}

static ssize_t lcd_amoled_vr_mode_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	struct hisi_fb_data_type* hisifd = NULL;
	int ret = LCD_KIT_OK;
	unsigned long val = 0;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	ret = strict_strtoul(buf, 0, &val);
	if (ret) {
		LCD_KIT_ERR("invalid parameter!\n");
		return ret;
	}
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		if (common_ops->set_vr_mode) {
			hisifb_activate_vsync(hisifd);
			ret = common_ops->set_vr_mode(hisifd, val);
			hisifb_deactivate_vsync(hisifd);
		}
	}
	up(&hisifd->blank_sem);
	return count;
}

static ssize_t lcd_effect_color_mode_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_ops->get_effect_color_mode) {
		ret = common_ops->get_effect_color_mode(buf);
	}
	return ret;
}

static ssize_t lcd_effect_color_mode_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	int ret = LCD_KIT_OK;
	unsigned long val = 0;

	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	ret = strict_strtoul(buf, 0, &val);
	if (ret) {
		LCD_KIT_ERR("invalid parameter!\n");
		return ret;
	}
	if (common_ops->set_effect_color_mode) {
		ret = common_ops->set_effect_color_mode(val);
	}
	return count;
}

static ssize_t lcd_test_config_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	struct hisi_fb_data_type* hisifd = NULL;
	int ret = LCD_KIT_OK;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (common_ops->get_test_config) {
		ret = common_ops->get_test_config(buf);
	}
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		if (buf) {
			if (!strncmp(buf, "OVER_CURRENT_DETECTION", strlen("OVER_CURRENT_DETECTION"))) {
				hisifb_activate_vsync(hisifd);
				ret = lcd_kit_current_det(hisifd);
				hisifb_deactivate_vsync(hisifd);
				up(&hisifd->blank_sem);
				return snprintf(buf, PAGE_SIZE, "%d", ret);
			}
			if (!strncmp(buf, "OVER_VOLTAGE_DETECTION", strlen("OVER_VOLTAGE_DETECTION"))) {
				hisifb_activate_vsync(hisifd);
				ret = lcd_kit_lv_det(hisifd);
				hisifb_deactivate_vsync(hisifd);
				up(&hisifd->blank_sem);
				return snprintf(buf, PAGE_SIZE, "%d", ret);
			}
		}
	}
	up(&hisifd->blank_sem);
	return ret;
}

static ssize_t lcd_test_config_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	int ret = LCD_KIT_OK;

	if (common_ops->set_test_config) {
		ret = common_ops->set_test_config(buf);
	}
	return count;
}

static ssize_t lcd_reg_read_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	struct hisi_fb_data_type* hisifd = NULL;
	int ret = LCD_KIT_OK;
	char lcd_reg_buf[LCD_REG_LENGTH_MAX] = {0};

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		if (disp_info->gamma_cal.support) {
			hisifb_activate_vsync(hisifd);
			lcd_kit_read_gamma(hisifd, lcd_reg_buf);
			hisifb_deactivate_vsync(hisifd);
			LCD_KIT_INFO("%s\n", lcd_reg_buf);
			ret = snprintf(buf, sizeof(lcd_reg_buf), "%s\n", lcd_reg_buf);
		}
	}
	up(&hisifd->blank_sem);
	return ret;
}

static ssize_t lcd_reg_read_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	unsigned int reg_value[100];
	char* cur;
	char* token;
	int i = 0;

	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	if (disp_info->gamma_cal.support) {
		/*parse buf*/
		cur = (char*)buf;
		token = strsep(&cur, ",");
		while (token) {
			reg_value[i++] = simple_strtol(token, NULL, 0);
			token = strsep(&cur, ",");
			if (i >= 100) {
				LCD_KIT_INFO("count is too long\n");
				return LCD_KIT_FAIL;
			}
		}
		disp_info->gamma_cal.addr = reg_value[0];
		disp_info->gamma_cal.length = reg_value[1];
	}
	return count;
}

static ssize_t lcd_gamma_dynamic_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	if (disp_info->gamma_cal.support) {
		if (count != GM_IGM_LEN) {
			HISI_FB_ERR("gamma count error! count = %d \n", (int)count);
			return LCD_KIT_FAIL;
		}
		hisifb_update_dynamic_gamma(hisifd, buf, count);
	}
	return count;
}

static ssize_t lcd_frame_count_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf) {
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	return snprintf(buf, PAGE_SIZE, "%u\n", hisifd->frame_count);
}

static ssize_t lcd_frame_update_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf) {
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	return snprintf(buf, PAGE_SIZE, "%u\n", hisifd->vsync_ctrl.vsync_infinite);
}

static ssize_t lcd_frame_update_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	struct hisi_fb_data_type* hisifd = NULL;
	unsigned long val = 0;
	static uint32_t esd_enable = 0;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}

	val = (int)simple_strtoul(buf, NULL, 0);
	down(&hisifd->blank_sem);
	g_enable_dirty_region_updt =  (val > 0) ? 0 : 1;
	hisifd->frame_update_flag = (val > 0) ? 1 : 0;
	hisifb_set_vsync_activate_state(hisifd, (val > 0) ? true : false);
	if (!is_mipi_cmd_panel(hisifd)) {
		goto err_out;
	}
	if (!hisifd->panel_power_on) {
		HISI_FB_DEBUG("fb%d, panel power off!\n", hisifd->index);
		goto err_out;
	}
	hisifb_activate_vsync(hisifd);
	if (val == 1) {
		esd_enable = hisifd->panel_info.esd_enable;
		hisifd->panel_info.esd_enable = 0;
		mdelay(50);
	}
	ldi_frame_update(hisifd, (val > 0) ? true : false);
	if (val == 0) {
		hisifd->vactive0_start_flag = 1;
		mdelay(50);
		hisifd->panel_info.esd_enable = esd_enable;
		esd_enable = 0;
	}
	hisifb_deactivate_vsync(hisifd);
err_out:
	up(&hisifd->blank_sem);
	return count;
}

static ssize_t lcd_mipi_clk_upt_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct hisi_fb_data_type* hisifd = NULL;
	struct hisi_fb_panel_data* pdata = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf) {
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		if (pdata->mipi_dsi_bit_clk_upt_show) {
			hisifb_activate_vsync(hisifd);
			ret = pdata->mipi_dsi_bit_clk_upt_show(hisifd->pdev, buf);
			hisifb_deactivate_vsync(hisifd);
		}
	}
	up(&hisifd->blank_sem);
	return ret;
}

static ssize_t lcd_mipi_clk_upt_store(struct device* dev, struct device_attribute* attr,
		const char* buf, size_t count)
{
	ssize_t ret = LCD_KIT_OK;
	struct hisi_fb_data_type* hisifd = NULL;
	struct hisi_fb_panel_data* pdata = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf) {
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		if (pdata->mipi_dsi_bit_clk_upt_store) {
			hisifb_activate_vsync(hisifd);
			ret = pdata->mipi_dsi_bit_clk_upt_store(hisifd->pdev, buf, count);
			hisifb_deactivate_vsync(hisifd);
		}
	}
	up(&hisifd->blank_sem);
	return ret;
}

static ssize_t lcd_func_switch_show(struct device* dev,
										struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct hisi_fb_data_type* hisifd = NULL;
	struct hisi_panel_info* pinfo = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	pinfo = &(hisifd->panel_info);
	if (NULL == pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return LCD_KIT_FAIL;	
	}
	ret = snprintf(buf, PAGE_SIZE,
				   "sbl=%d\n"
				   "xcc_support=%d\n"
				   "dsi_bit_clk_upt=%d\n"
				   "dirty_region_upt=%d\n"
				   "fps_updt_support=%d\n"
				   "ifbc_type=%d\n"
				   "esd_enable=%d\n"
				   "blpwm_input_ena=%d\n"
				   "lane_nums=%d\n"
				   "panel_effect_support=%d\n"
				   "color_temp_rectify_support=%d\n"
				   "ddic_rgbw_support=%d\n"
				   "hiace=%d\n"
				   "effect_enable=%d\n"
				   "effect_debug=%d\n"
				   "fps_support=%d\n",
				   pinfo->sbl_support,
				   pinfo->xcc_support,
				   pinfo->dsi_bit_clk_upt_support,
				   pinfo->dirty_region_updt_support,
				   pinfo->fps_updt_support,
				   pinfo->ifbc_type,
				   pinfo->esd_enable,
				   pinfo->blpwm_input_ena,
				   pinfo->mipi.lane_nums + 1,
				   pinfo->panel_effect_support,
				   pinfo->color_temp_rectify_support,
				   pinfo->rgbw_support,
				   pinfo->hiace_support,
				   g_enable_effect,
				   g_debug_effect,
				   disp_info->fps.support);

	return ret;
}

static ssize_t lcd_func_switch_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct hisi_fb_data_type* hisifd = NULL;
	char command[MAX_BUF] = {0};

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (NULL == buf) {
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	if (strlen(buf) >= MAX_BUF) {
		HISI_FB_ERR("buf overflow!\n");
		return LCD_KIT_FAIL;
	}
	if (!sscanf(buf, "%s", command)) { 
		LCD_KIT_INFO("bad command(%s)\n", command);
		return count;
	}
	lcd_kit_parse_switch_cmd(hisifd, command);
 	return count;
}

static ssize_t lcd_lv_detect_show(struct device* dev,
									   struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	ret = lcd_kit_lv_det(hisifd);
	return snprintf(buf, PAGE_SIZE, "%d", ret); 
}

static ssize_t lcd_current_detect_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	ret = lcd_kit_current_det(hisifd);
	return snprintf(buf, PAGE_SIZE, "%d", ret);
}

/*oem info*/
static int oem_info_type = -1;
static int lcd_get_2d_barcode(char *oem_data,struct hisi_fb_data_type *hisifd);
static int lcd_get_project_id(char *oem_data,struct hisi_fb_data_type *hisifd);
static int lcd_get_brightness_colorpoint(char *oem_data,struct hisi_fb_data_type *hisifd);
static void lcd_set_brightness_xcc_rgbw(char *oem_data, struct hisi_fb_data_type *hisifd);

static struct oem_info_cmd oem_read_cmds[] = {
	{PROJECT_ID_TYPE, lcd_get_project_id},
	{BARCODE_2D_TYPE, lcd_get_2d_barcode},
	{BRIGHTNESS_COLOROFWHITE_TYPE, lcd_get_brightness_colorpoint},
};

static struct oem_info_cmd oem_write_cmds[] = {
	{BRIGHTNESS_COLOROFWHITE_TYPE, lcd_set_brightness_xcc_rgbw},
};

static int lcd_get_project_id(char *oem_data, struct hisi_fb_data_type *hisifd)
{
	struct lcd_kit_ops *lcd_ops = NULL;
	char project_id[PROJECT_ID_LENGTH + 1] = {0};
	int ret = 0;
	int i = 0;

	lcd_ops = lcd_kit_get_ops();
	if (!lcd_ops) {
		LCD_KIT_ERR("lcd_ops is null\n");
		return LCD_KIT_FAIL;
	}
	if (disp_info->project_id.support) {
		if (lcd_ops->get_project_id) {
			ret = lcd_ops->get_project_id(project_id);
		}
		oem_data[0] = PROJECT_ID_TYPE;
		oem_data[1] = OEM_INFO_BLOCK_NUM;
		for (i = 0; i < PROJECT_ID_LENGTH; i++) {
			oem_data[i + 2] = project_id[i];
		}
	}
	return ret;
}

static int lcd_get_2d_barcode(char *oem_data, struct hisi_fb_data_type *hisifd)
{
	char read_value[OEM_INFO_SIZE_MAX+1] = {0};
	int ret = 0;
	struct lcd_kit_panel_ops *panel_ops = NULL;

	panel_ops = lcd_kit_panel_get_ops();
	if (panel_ops && panel_ops->lcd_get_2d_barcode) {
		return panel_ops->lcd_get_2d_barcode(oem_data, hisifd);
	}

	if (disp_info->oeminfo.barcode_2d.support) {
		ret = lcd_kit_dsi_cmds_rx(hisifd, read_value, &disp_info->oeminfo.barcode_2d.cmds);
		oem_data[0] = BARCODE_2D_TYPE;
		oem_data[1] = BARCODE_BLOCK_NUM;
		strncat(oem_data, read_value, strlen(read_value));
	}
	return ret;
}

static int lcd_get_brightness_colorpoint(char *oem_data, struct hisi_fb_data_type *hisifd)
{
	int ret = 0;
	char read_value[OEM_INFO_SIZE_MAX+1] = {0};
	struct lcd_kit_brightness_color_oeminfo *bri_col_info = NULL;
    uint32_t value_temp = 0;

	if (disp_info->oeminfo.brightness_color_uniform.support) {
		lcd_kit_dsi_cmds_rx(hisifd, read_value, &disp_info->oeminfo.brightness_color_uniform.brightness_color_cmds);
		oem_data[0] = BRIGHTNESS_COLOROFWHITE_TYPE;
		oem_data[1] = OEM_INFO_BLOCK_NUM * 2;
		bri_col_info = lcd_kit_get_brightness_color_oeminfo();
		if (!bri_col_info) {
			LCD_KIT_ERR("bri_col_info is null\n");
			return LCD_KIT_FAIL;
		}
		/*brightness data*/
		oem_data[2] = 0x00;
		oem_data[3] = 0x00;
		oem_data[4] = read_value[0];
		oem_data[5] = read_value[1];

		/* Color point of White x(2Bytes)*/
		oem_data[6] = 0x00;
		oem_data[7] = 0x00;
		oem_data[8] = read_value[2];
		oem_data[9] = read_value[3];

		/* Color point of White y(2Bytes)*/
		oem_data[10] = 0x00;
		oem_data[11] = 0x00;
		oem_data[12] = read_value[4];
		oem_data[13] = read_value[5];

		/* modulesn:Serial number of inspection system(4Bytes) */
		value_temp = bri_col_info->panel_id.modulesn;
		oem_data[14] = (uint8_t)((value_temp >> 24) & 0xFF);
		oem_data[15] = (uint8_t)((value_temp >> 16) & 0xFF);
		oem_data[16] = (uint8_t)((value_temp >> 8) & 0xFF);
		oem_data[17] = (uint8_t)(value_temp & 0xFF);

		/* equipid(1Bytes) */
		value_temp = bri_col_info->panel_id.equipid;
		oem_data[18] = (uint8_t)((value_temp >> 24) & 0xFF);
		oem_data[19] = (uint8_t)((value_temp >> 16) & 0xFF);
		oem_data[20] = (uint8_t)((value_temp >> 8) & 0xFF);
		oem_data[21] = (uint8_t)(value_temp & 0xFF);

		/* modulemanufactdate:year(1Bytes)-month(1Bytes)-date(1Bytes)*/
		value_temp = bri_col_info->panel_id.modulemanufactdate;
		oem_data[22] = (uint8_t)((value_temp >> 24) & 0xFF);
		oem_data[23] = (uint8_t)((value_temp >> 16) & 0xFF);
		oem_data[24] = (uint8_t)((value_temp >> 8) & 0xFF);
		oem_data[25] = (uint8_t)(value_temp & 0xFF);

		/* vendorid(1Bytes) */
		value_temp = bri_col_info->panel_id.vendorid;
		oem_data[26] = (uint8_t)((value_temp >> 24) & 0xFF);
		oem_data[27] = (uint8_t)((value_temp >> 16) & 0xFF);
		oem_data[28] = (uint8_t)((value_temp >> 8) & 0xFF);
		oem_data[29] = (uint8_t)(value_temp & 0xFF);
	}
	return ret;
}

static void lcd_set_brightness_xcc_rgbw(char *oem_data, struct hisi_fb_data_type *hisifd)
{
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t ret = 0;
	uint32_t *tmp = NULL;
	uint32_t count = 9;
	struct lcd_kit_brightness_color_oeminfo *oeminfo = NULL;
	char buf[128] = {0};

	if ((NULL == hisifd) || (NULL == oem_data)) {
		LCD_KIT_ERR("NULL pointer\n");
		return;
	}
	for (i=0;i<oem_data[1];i++) {
		LCD_KIT_INFO("oem_data[%d] = %d\n", i,oem_data[i]);
	}

	oeminfo = lcd_kit_get_brightness_color_oeminfo();
	tmp = (uint32_t *)oeminfo;
	for(i=0,j=2; i<sizeof(struct lcd_kit_brightness_color_oeminfo)/sizeof(uint32_t); i++) {
		tmp[i] = (uint32_t)((((uint32_t)oem_data[j])<<24)|(((uint32_t)oem_data[(long)j+1])<<16)|(((uint32_t)oem_data[(long)j+2])<<8)|((uint32_t)oem_data[(long)j+3]));
		j = j+4;
	}
	snprintf(buf, sizeof(buf), "%d,%d,%d,%d,%d,%d,%d,%d,%d",
			oeminfo->color_params.mxcc_matrix[0][0],oeminfo->color_params.mxcc_matrix[0][1],
			oeminfo->color_params.mxcc_matrix[0][2],oeminfo->color_params.mxcc_matrix[1][0],
			oeminfo->color_params.mxcc_matrix[1][1],oeminfo->color_params.mxcc_matrix[1][2],
			oeminfo->color_params.mxcc_matrix[2][0],oeminfo->color_params.mxcc_matrix[2][1],
			oeminfo->color_params.mxcc_matrix[2][2]);
	LCD_KIT_INFO("buf:%s\n", buf);
	ret = lcd_kit_realtime_set_xcc(hisifd, buf, count);
	if(ret != 0) {
		LCD_KIT_ERR("realtime set lcd xcc failed!\n");
	}
}

static ssize_t lcd_oem_info_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;
	int i = 0;
	struct hisi_fb_data_type* hisifd = NULL;
	char oem_info_data[OEM_INFO_SIZE_MAX] = {0};
	char str_oem[OEM_INFO_SIZE_MAX+1] = {0};
	char str_tmp[OEM_INFO_SIZE_MAX+1] = {0};

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (!disp_info->oeminfo.support) {
		LCD_KIT_ERR("oem info is not support\n");
		return LCD_KIT_FAIL;
	}
	if (oem_info_type == -1) {
		LCD_KIT_ERR("first write ddic_oem_info, then read\n");
		return LCD_KIT_FAIL;
	}
	/*execute cmd func*/
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		hisifb_activate_vsync(hisifd);
		for (i = 0; i < sizeof(oem_read_cmds)/sizeof(oem_read_cmds[0]); i++) {
			if (oem_info_type == oem_read_cmds[i].type) {
				LCD_KIT_INFO("cmd = %d\n",oem_info_type);
				if (oem_read_cmds[i].func) {
			   		(*oem_read_cmds[i].func)(oem_info_data, hisifd);
				}
			}
		}
		hisifb_deactivate_vsync(hisifd);
	}
	up(&hisifd->blank_sem);
	/*parse data*/
	memset(str_oem, 0, sizeof(str_oem));
	for(i = 0; i < oem_info_data[1]; i++) {
		memset(str_tmp, 0, sizeof(str_tmp));
		snprintf(str_tmp, sizeof(str_tmp), "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,",
		oem_info_data[0+i*BARCODE_BLOCK_LEN],oem_info_data[1+i*BARCODE_BLOCK_LEN],oem_info_data[2+i*BARCODE_BLOCK_LEN],oem_info_data[3+i*BARCODE_BLOCK_LEN],
		oem_info_data[4+i*BARCODE_BLOCK_LEN],oem_info_data[5+i*BARCODE_BLOCK_LEN],oem_info_data[6+i*BARCODE_BLOCK_LEN],oem_info_data[7+i*BARCODE_BLOCK_LEN],
		oem_info_data[8+i*BARCODE_BLOCK_LEN],oem_info_data[9+i*BARCODE_BLOCK_LEN],oem_info_data[10+i*BARCODE_BLOCK_LEN],oem_info_data[11+i*BARCODE_BLOCK_LEN],
		oem_info_data[12+i*BARCODE_BLOCK_LEN],oem_info_data[13+i*BARCODE_BLOCK_LEN],oem_info_data[14+i*BARCODE_BLOCK_LEN],oem_info_data[15+i*BARCODE_BLOCK_LEN]);
		strncat(str_oem, str_tmp,strlen(str_tmp));
	}
	LCD_KIT_INFO("str_oem = %s\n", str_oem);
	ret = snprintf(buf, PAGE_SIZE, "%s\n", str_oem);
	return ret;
}

static ssize_t lcd_oem_info_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct hisi_fb_data_type* hisifd = NULL;
	char *cur;
	char *token;
	char oem_info[OEM_INFO_SIZE_MAX] = {0};
	int i = 0;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (!disp_info->oeminfo.support) {
		LCD_KIT_ERR("oem info is not support\n");
		return LCD_KIT_FAIL;
	}
	if (strlen(buf) < OEM_INFO_SIZE_MAX) {
		cur = (char*)buf;
		token = strsep(&cur, ",");
		while (token) {
			oem_info[i++] = (unsigned char)simple_strtol(token, NULL, 0);
			token = strsep(&cur, ",");
		}
	} else {
		memcpy(oem_info, "INVALID", strlen("INVALID") + 1);
		LCD_KIT_INFO("invalid cmd\n");
	}
	LCD_KIT_INFO("write Type=0x%x , data len=%d\n", oem_info[0], oem_info[1]);
	oem_info_type = oem_info[0];
	/*if the data length is 0, then just store the type*/
	if (oem_info[1] == 0) {
		LCD_KIT_INFO("Just store type:0x%x and then finished\n",oem_info[0]);
		return count;
	}
	/*execute cmd func*/
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		hisifb_activate_vsync(hisifd);
		for (i = 0; i < sizeof(oem_write_cmds)/sizeof(oem_write_cmds[0]); i++) {
			if (oem_info_type == oem_write_cmds[i].type) {
				(*oem_write_cmds[i].func)(oem_info, hisifd);
			}
		}
		hisifb_deactivate_vsync(hisifd);
	}
	up(&hisifd->blank_sem);
	LCD_KIT_INFO("oem_info = %s\n", oem_info);
	return count;
}

extern int hisi_adc_get_current(int adc_channel);
static ssize_t lcd_ldo_check_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
    int i,j = 0;
    int cur_val = 0;
    int sum_current = 0;
    int len = sizeof(struct lcd_kit_ldo_check);
    int temp_max_value = 0;
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (NULL == hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (!disp_info->ldo_check.support) {
		LCD_KIT_ERR("ldo check not support\n");
		return LCD_KIT_FAIL;
	}

	down(&hisifd->blank_sem);
	if (!hisifd->panel_power_on) {
		LCD_KIT_ERR("panel is power off\n");
		up(&hisifd->blank_sem);
		return LCD_KIT_FAIL;
	}
	for(i = 0; i < disp_info->ldo_check.ldo_num; i++) {
		sum_current = 0;
		temp_max_value = 0;
		for(j=0; j< LDO_CHECK_COUNT; j++) {
			cur_val = hisi_adc_get_current(disp_info->ldo_check.ldo_channel[i]);
			if(cur_val < 0) {
				sum_current = -1;
				break;
			} else {
				sum_current = sum_current + cur_val;
				if(temp_max_value < cur_val) {
					temp_max_value = cur_val;
				}
			}
		}

		if(sum_current == -1) {
			disp_info->ldo_check.ldo_current[i]= -1;
		}
		else {
			disp_info->ldo_check.ldo_current[i]= (sum_current - temp_max_value)/(LDO_CHECK_COUNT - 1);
		}
	}
	for(i=0; i< disp_info->ldo_check.ldo_num; i++) {
		LCD_KIT_INFO("ldo[%d]: name=%s, current=%d, channel=%d,threshold=%d!\n",i,disp_info->ldo_check.ldo_name[i],
		disp_info->ldo_check.ldo_current[i],disp_info->ldo_check.ldo_channel[i],disp_info->ldo_check.curr_threshold[i]);
	}
	memcpy(buf, &disp_info->ldo_check, len);
	LCD_KIT_INFO("ldo check finished\n");
	up(&hisifd->blank_sem);
	return len;
}

static ssize_t lcd_bl_self_test_show(struct device* dev,
			struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_bl_ops *bl_ops = NULL;

	bl_ops = lcd_kit_get_bl_ops();
	if (!bl_ops) {
		LCD_KIT_ERR("bl_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}

	if (disp_info->bkl_open_short_support) {
	if (bl_ops->bl_self_test) {
		ret = bl_ops->bl_self_test();
		}
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t lcd_effect_bl_show(struct device* dev,
			struct device_attribute* attr, char* buf)
{
	int ret = LCD_KIT_OK;
	if (NULL == dev)
	{
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}

	if (NULL == buf)
	{
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}

	ret = hisifb_display_effect_bl_ctrl_show(dev_get_drvdata(dev), buf);
	return ret;
}

static ssize_t lcd_effect_bl_store(struct device* dev,
			struct device_attribute* attr, const char* buf, size_t count)
{
	int ret = LCD_KIT_OK;
	if (NULL == dev)
	{
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}

	if (NULL == buf)
	{
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}

	ret = hisifb_display_effect_bl_ctrl_store(dev_get_drvdata(dev), buf, count);
	return ret;
}


static int lcd_check_support(int index)
{
	if (runmode_is_factory()) {
		return SYSFS_SUPPORT;
	}

	switch (index) {
		case LCD_MODEL_INDEX:
			return SYSFS_SUPPORT;
		case LCD_TYPE_INDEX:
			return SYSFS_SUPPORT;
		case PANEL_INFO_INDEX:
			return SYSFS_SUPPORT;
		case INVERSION_INDEX:
			return common_info->inversion.support;
		case SCAN_INDEX:
			return common_info->scan.support;
		case CHECK_REG_INDEX:
			return common_info->check_reg.support;
		case CHECKSUM_INDEX:
			return disp_info->checksum.support;
		case SLEEP_CTRL_INDEX:
			return common_info->pt.support;
		case HKADC_INDEX:
			return disp_info->hkadc.support;
		case VOLTAGE_ENABLE_INDEX:
			return SYSFS_NOT_SUPPORT;
		case PCD_ERRFLAG_INDEX:
			return disp_info->pcd_errflag_check_support;
		case ACL_INDEX:
			return common_info->acl.support;
		case VR_INDEX:
			return common_info->vr.support;
		case SUPPORT_MODE_INDEX:
			return common_info->effect_color.support;
		case GAMMA_DYNAMIC_INDEX:
			return disp_info->gamma_cal.support;
		case FRAME_COUNT_INDEX:
			return disp_info->vr_support;
		case FRAME_UPDATE_INDEX:
			return disp_info->vr_support;
		case MIPI_DSI_CLK_UPT_INDEX:
			return SYSFS_SUPPORT;
		case FPS_SCENCE_INDEX:
			return disp_info->fps.support;
		case ALPM_FUNCTION_INDEX:
			return disp_info->alpm.support;
		case ALPM_SETTING_INDEX:
			return disp_info->alpm.support;
		case FUNC_SWITCH_INDEX:
			return SYSFS_SUPPORT;
		case TEST_CONFIG_INDEX:
			return SYSFS_SUPPORT;
		case LV_DETECT_INDEX:
			return disp_info->lv_det.support;
		case CURRENT_DETECT_INDEX:
			return disp_info->current_det.support;
		case REG_READ_INDEX:
			return disp_info->gamma_cal.support;
		case DDIC_OEM_INDEX:
			return disp_info->oeminfo.support;
		case BL_MODE_INDEX:
			return SYSFS_NOT_SUPPORT;
		case BL_SUPPORT_MODE_INDEX:
			return SYSFS_NOT_SUPPORT;
		case LDO_CHECK_INDEX:
			return disp_info->ldo_check.support;
		case BL_SELF_TEST_INDEX:
			return SYSFS_SUPPORT;
		case EFFECT_BL_INDEX:
			return SYSFS_SUPPORT;
		default:
			return SYSFS_NOT_SUPPORT;
	}
}

struct lcd_kit_sysfs_ops g_lcd_sysfs_ops = {
	.check_support = lcd_check_support,
	.model_show = lcd_model_show,
	.type_show = lcd_type_show,
	.panel_info_show = lcd_panel_info_show,
	.inversion_mode_show = lcd_inversion_mode_show,
	.inversion_mode_store = lcd_inversion_mode_store,
	.scan_mode_show = lcd_scan_mode_show,
	.scan_mode_store = lcd_scan_mode_store,
	.check_reg_show = lcd_check_reg_show,
	.gram_check_show = lcd_gram_check_show,
	.gram_check_store = lcd_gram_check_store,
	.sleep_ctrl_show = lcd_sleep_ctrl_show,
	.sleep_ctrl_store = lcd_sleep_ctrl_store,
	.hkadc_debug_show = lcd_hkadc_debug_show,
	.hkadc_debug_store = lcd_hkadc_debug_store,
	.pcd_errflag_check_show = lcd_amoled_pcd_errflag_show,
	.amoled_acl_ctrl_show = lcd_amoled_acl_ctrl_show,
	.amoled_acl_ctrl_store = lcd_amoled_acl_ctrl_store,
	.amoled_vr_mode_show = lcd_amoled_vr_mode_show,
	.amoled_vr_mode_store = lcd_amoled_vr_mode_store,
	.effect_color_mode_show = lcd_effect_color_mode_show,
	.effect_color_mode_store = lcd_effect_color_mode_store,
	.test_config_show = lcd_test_config_show,
	.test_config_store = lcd_test_config_store,
	.reg_read_show = lcd_reg_read_show,
	.reg_read_store = lcd_reg_read_store,
	.gamma_dynamic_store = lcd_gamma_dynamic_store,
	.frame_count_show = lcd_frame_count_show,
	.frame_update_show = lcd_frame_update_show,
	.frame_update_store = lcd_frame_update_store,
	.mipi_dsi_clk_upt_show = lcd_mipi_clk_upt_show,
	.mipi_dsi_clk_upt_store = lcd_mipi_clk_upt_store,
	.fps_scence_show = lcd_fps_scence_show,
	.fps_scence_store = lcd_fps_scence_store,
	.alpm_function_show = lcd_alpm_function_show,
	.alpm_function_store = lcd_alpm_function_store,
	.alpm_setting_store = lcd_alpm_setting_store,
	.func_switch_show = lcd_func_switch_show,
	.func_switch_store = lcd_func_switch_store,
	.lv_detect_show = lcd_lv_detect_show,
	.current_detect_show = lcd_current_detect_show,
	.ddic_oem_info_show = lcd_oem_info_show,
	.ddic_oem_info_store = lcd_oem_info_store,
	.ldo_check_show = lcd_ldo_check_show,
	.bl_self_test_show = lcd_bl_self_test_show,
	.effect_bl_show = lcd_effect_bl_show,
	.effect_bl_store = lcd_effect_bl_store,
};

int lcd_kit_sysfs_init(void)
{
	lcd_kit_sysfs_ops_register(&g_lcd_sysfs_ops);
	return LCD_KIT_OK;
}
