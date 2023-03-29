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

#include "lcd_kit_dbg.h"
#include "lcd_kit_sysfs.h"

/*function:  this function is used to get the type of LCD
 *input:
 *@pdata: this void point is used to converte to fb data struct.
 *output:
 *@buf: get the type of lcd
*/
static ssize_t lcd_kit_model_show(struct device* dev,
									   struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->model_show) {
		ret = sysfs_ops->model_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_type_show(struct device* dev,
									   struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->type_show) {
		ret = sysfs_ops->type_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_panel_info_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}

	if (sysfs_ops->panel_info_show) {
		ret = sysfs_ops->panel_info_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_inversion_mode_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->inversion_mode_show) {
		ret = sysfs_ops->inversion_mode_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_inversion_mode_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}

	if (sysfs_ops->inversion_mode_store) {
		sysfs_ops->inversion_mode_store(dev, attr, buf, count);
	}
	return count;

}

static ssize_t lcd_kit_scan_mode_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->scan_mode_show) {
		ret = sysfs_ops->scan_mode_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_scan_mode_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->scan_mode_store) {
		sysfs_ops->scan_mode_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_check_reg_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->check_reg_show) {
		ret = sysfs_ops->check_reg_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_gram_check_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->gram_check_show) {
		ret = sysfs_ops->gram_check_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_gram_check_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->gram_check_store) {
		sysfs_ops->gram_check_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_sleep_ctrl_show(struct device* dev,
										struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->sleep_ctrl_show) {
		ret = sysfs_ops->sleep_ctrl_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_sleep_ctrl_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->sleep_ctrl_store) {
		sysfs_ops->sleep_ctrl_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_hkadc_debug_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->hkadc_debug_show) {
		ret = sysfs_ops->hkadc_debug_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_hkadc_debug_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->hkadc_debug_store) {
		sysfs_ops->hkadc_debug_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t	lcd_kit_lcd_voltage_enable_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->voltage_enable_store) {
		sysfs_ops->voltage_enable_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_amoled_pcd_errflag_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->pcd_errflag_check_show) {
		ret = sysfs_ops->pcd_errflag_check_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_amoled_acl_ctrl_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->amoled_acl_ctrl_show) {
		ret = sysfs_ops->amoled_acl_ctrl_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_amoled_acl_ctrl_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->amoled_acl_ctrl_store) {
		sysfs_ops->amoled_acl_ctrl_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_amoled_vr_mode_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->amoled_vr_mode_show) {
		ret = sysfs_ops->amoled_vr_mode_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_amoled_vr_mode_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->amoled_vr_mode_store) {
		sysfs_ops->amoled_vr_mode_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_support_mode_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->effect_color_mode_show) {
		ret = sysfs_ops->effect_color_mode_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_support_mode_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->effect_color_mode_store) {
		sysfs_ops->effect_color_mode_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_gamma_dynamic_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->gamma_dynamic_store) {
		sysfs_ops->gamma_dynamic_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_frame_count_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->frame_count_show) {
		ret = sysfs_ops->frame_count_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_frame_update_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->frame_update_show) {
		ret = sysfs_ops->frame_update_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t	lcd_kit_frame_update_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->frame_update_store) {
		sysfs_ops->frame_update_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_mipi_dsi_clk_upt_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->mipi_dsi_clk_upt_show) {
		ret = sysfs_ops->mipi_dsi_clk_upt_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_mipi_dsi_clk_upt_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->mipi_dsi_clk_upt_store) {
		sysfs_ops->mipi_dsi_clk_upt_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_fps_scence_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->fps_scence_show) {
		ret = sysfs_ops->fps_scence_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_fps_scence_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->fps_scence_store) {
		sysfs_ops->fps_scence_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_alpm_function_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->alpm_function_show) {
		ret = sysfs_ops->alpm_function_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_alpm_function_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->alpm_function_store) {
		sysfs_ops->alpm_function_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_alpm_setting_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->alpm_setting_store) {
		sysfs_ops->alpm_setting_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_func_switch_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->func_switch_show) {
		ret = sysfs_ops->func_switch_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_func_switch_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->func_switch_store) {
		sysfs_ops->func_switch_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_test_config_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->test_config_show) {
		ret = sysfs_ops->test_config_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_test_config_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->test_config_store) {
		sysfs_ops->test_config_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_lv_detect_show(struct device* dev,
									   struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->lv_detect_show) {
		ret = sysfs_ops->lv_detect_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_current_detect_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->current_detect_show) {
		ret = sysfs_ops->current_detect_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_reg_read_show(struct device* dev,
									  struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->reg_read_show) {
		ret = sysfs_ops->reg_read_show(dev, attr, buf);
	}
	return ret;
}
static ssize_t lcd_kit_reg_read_store(struct device* dev,
									   struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->reg_read_store) {
		sysfs_ops->reg_read_store(dev, attr, buf, count);
	}
	return count;
}

static ssize_t lcd_kit_ddic_oem_info_show(struct device* dev,
		struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->ddic_oem_info_show) {
		ret = sysfs_ops->ddic_oem_info_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_ddic_oem_info_store(struct device* dev,
		struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->ddic_oem_info_store) {
		sysfs_ops->ddic_oem_info_store(dev, attr, buf, count);
	}
	return count;
}
static ssize_t lcd_kit_bl_mode_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->bl_mode_store) {
		sysfs_ops->bl_mode_store(dev, attr, buf, count);
	}
	return count;
}
static ssize_t lcd_kit_bl_mode_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->bl_mode_show) {
		ret = sysfs_ops->bl_mode_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_support_bl_mode_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->support_bl_mode_store) {
		sysfs_ops->support_bl_mode_store(dev, attr, buf, count);
	}
	return count;
}
static ssize_t lcd_kit_support_bl_mode_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->support_bl_mode_show) {
		ret = sysfs_ops->support_bl_mode_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_ldo_check_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->ldo_check_show) {
		ret = sysfs_ops->ldo_check_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_bl_self_test_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->bl_self_test_show) {
		ret = sysfs_ops->bl_self_test_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_effect_bl_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->effect_bl_show) {
		ret = sysfs_ops->effect_bl_show(dev, attr, buf);
	}
	return ret;
}

static ssize_t lcd_kit_effect_bl_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_kit_sysfs_ops* sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (NULL == sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (sysfs_ops->effect_bl_store) {
		ret = sysfs_ops->effect_bl_store(dev, attr, buf, count);
	}
	return ret;
}

static DEVICE_ATTR(lcd_model, 0644, lcd_kit_model_show, NULL);
static DEVICE_ATTR(lcd_display_type, 0644, lcd_kit_type_show, NULL);
static DEVICE_ATTR(panel_info, 0644, lcd_kit_panel_info_show, NULL);
static DEVICE_ATTR(lcd_inversion_mode, S_IRUGO | S_IWUSR, lcd_kit_inversion_mode_show, lcd_kit_inversion_mode_store);
static DEVICE_ATTR(lcd_scan_mode, S_IRUGO | S_IWUSR, lcd_kit_scan_mode_show, lcd_kit_scan_mode_store);
static DEVICE_ATTR(lcd_check_reg, S_IRUGO, lcd_kit_check_reg_show, NULL);
static DEVICE_ATTR(lcd_checksum, S_IRUGO | S_IWUSR, lcd_kit_gram_check_show, lcd_kit_gram_check_store);
static DEVICE_ATTR(lcd_sleep_ctrl, S_IRUGO | S_IWUSR, lcd_kit_sleep_ctrl_show, lcd_kit_sleep_ctrl_store);
static DEVICE_ATTR(lcd_hkadc, S_IRUGO | S_IWUSR, lcd_kit_hkadc_debug_show, lcd_kit_hkadc_debug_store);
static DEVICE_ATTR(lcd_voltage_enable, S_IWUSR, NULL, lcd_kit_lcd_voltage_enable_store);
static DEVICE_ATTR(amoled_pcd_errflag_check, 0644, lcd_kit_amoled_pcd_errflag_show, NULL);
static DEVICE_ATTR(amoled_acl, S_IRUGO | S_IWUSR, lcd_kit_amoled_acl_ctrl_show, lcd_kit_amoled_acl_ctrl_store);
static DEVICE_ATTR(amoled_vr_mode, 0644, lcd_kit_amoled_vr_mode_show, lcd_kit_amoled_vr_mode_store);
static DEVICE_ATTR(lcd_support_mode, S_IRUGO | S_IWUSR, lcd_kit_support_mode_show, lcd_kit_support_mode_store);
static DEVICE_ATTR(gamma_dynamic, S_IRUGO | S_IWUSR, NULL, lcd_kit_gamma_dynamic_store);
static DEVICE_ATTR(frame_count, S_IRUGO, lcd_kit_frame_count_show, NULL);
static DEVICE_ATTR(frame_update, S_IRUGO | S_IWUSR, lcd_kit_frame_update_show, lcd_kit_frame_update_store);
static DEVICE_ATTR(mipi_dsi_bit_clk_upt, S_IRUGO | S_IWUSR, lcd_kit_mipi_dsi_clk_upt_show, lcd_kit_mipi_dsi_clk_upt_store);
static DEVICE_ATTR(lcd_fps_scence, (S_IRUGO | S_IWUSR), lcd_kit_fps_scence_show, lcd_kit_fps_scence_store);
static DEVICE_ATTR(alpm_function, 0644, lcd_kit_alpm_function_show, lcd_kit_alpm_function_store);
static DEVICE_ATTR(alpm_setting, 0644, NULL, lcd_kit_alpm_setting_store);
static DEVICE_ATTR(lcd_func_switch, S_IRUGO | S_IWUSR, lcd_kit_func_switch_show, lcd_kit_func_switch_store);
static DEVICE_ATTR(lcd_test_config, 0640, lcd_kit_test_config_show, lcd_kit_test_config_store);
static DEVICE_ATTR(lv_detect, 0640, lcd_kit_lv_detect_show, NULL);
static DEVICE_ATTR(current_detect, 0640, lcd_kit_current_detect_show, NULL);
static DEVICE_ATTR(lcd_reg_read, 0600, lcd_kit_reg_read_show, lcd_kit_reg_read_store);
static DEVICE_ATTR(ddic_oem_info, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, lcd_kit_ddic_oem_info_show, lcd_kit_ddic_oem_info_store);
static DEVICE_ATTR(lcd_bl_mode, S_IRUGO | S_IWUSR, lcd_kit_bl_mode_show, lcd_kit_bl_mode_store);
static DEVICE_ATTR(lcd_bl_support_mode, S_IRUGO | S_IWUSR, lcd_kit_support_bl_mode_show, lcd_kit_support_bl_mode_store);
static DEVICE_ATTR(lcd_ldo_check, S_IRUGO, lcd_kit_ldo_check_show, NULL);
static DEVICE_ATTR(bl_self_test, S_IRUGO|S_IWUSR, lcd_kit_bl_self_test_show, NULL);
static DEVICE_ATTR(effect_bl, S_IRUGO | S_IWUSR, lcd_kit_effect_bl_show, lcd_kit_effect_bl_store);

static struct attribute* lcd_kit_sysfs_attrs[LCD_KIT_SYSFS_MAX] = {NULL};
struct attribute *lcd_kit_conf[] = {
	&dev_attr_lcd_model.attr,
	&dev_attr_lcd_display_type.attr,
	&dev_attr_panel_info.attr,
	&dev_attr_lcd_inversion_mode.attr,
	&dev_attr_lcd_scan_mode.attr,
	&dev_attr_lcd_check_reg.attr,
	&dev_attr_lcd_checksum.attr,
	&dev_attr_lcd_sleep_ctrl.attr,
	&dev_attr_lcd_hkadc.attr,
	&dev_attr_lcd_voltage_enable.attr,
	&dev_attr_amoled_pcd_errflag_check.attr,
	&dev_attr_amoled_acl.attr,
	&dev_attr_amoled_vr_mode.attr,
	&dev_attr_lcd_support_mode.attr,
	&dev_attr_gamma_dynamic.attr,
	&dev_attr_frame_count.attr,
	&dev_attr_frame_update.attr,
	&dev_attr_mipi_dsi_bit_clk_upt.attr,
	&dev_attr_lcd_fps_scence.attr,
	&dev_attr_alpm_function.attr,
	&dev_attr_alpm_setting.attr,
	&dev_attr_lcd_func_switch.attr,
	&dev_attr_lcd_test_config.attr,
	&dev_attr_lv_detect.attr,
	&dev_attr_current_detect.attr,
	&dev_attr_lcd_reg_read.attr,
	&dev_attr_ddic_oem_info.attr,
	&dev_attr_lcd_bl_mode.attr,
	&dev_attr_lcd_bl_support_mode.attr,
	&dev_attr_lcd_ldo_check.attr,
	&dev_attr_bl_self_test.attr,
	&dev_attr_effect_bl.attr,
};

struct attribute_group lcd_kit_sysfs_attr_group = {
	.attrs = lcd_kit_sysfs_attrs,
};

static struct lcd_kit_sysfs_ops *g_lcd_kit_sysfs_ops = NULL;
int lcd_kit_sysfs_ops_register(struct lcd_kit_sysfs_ops* ops)
{
	if (g_lcd_kit_sysfs_ops) {
		LCD_KIT_ERR("g_lcd_kit_sysfs_ops has already been registered!\n");
		return LCD_KIT_FAIL;
	}
	g_lcd_kit_sysfs_ops = ops;
	LCD_KIT_INFO("g_lcd_kit_sysfs_ops register success!\n");
	return LCD_KIT_OK;
}

int lcd_kit_sysfs_ops_unregister(struct lcd_kit_sysfs_ops* ops)
{
	if (g_lcd_kit_sysfs_ops == ops) {
		g_lcd_kit_sysfs_ops = NULL;
		LCD_KIT_INFO("g_lcd_kit_sysfs_ops unregister success!\n");
		return LCD_KIT_OK;
	}
	LCD_KIT_ERR("g_lcd_kit_sysfs_ops unregister fail!\n");
	return LCD_KIT_FAIL;
}

struct lcd_kit_sysfs_ops* lcd_kit_get_sysfs_ops(void)
{
	return g_lcd_kit_sysfs_ops;
}

static int lcd_kit_check_support(int index)
{
	struct lcd_kit_sysfs_ops *sysfs_ops = NULL;

	sysfs_ops = lcd_kit_get_sysfs_ops();
	if (!sysfs_ops) {
		LCD_KIT_ERR("sysfs_ops is null\n");
		return LCD_KIT_OK;
	}
	if (sysfs_ops->check_support) {
		return sysfs_ops->check_support(index);
	} else {
		LCD_KIT_INFO("not register config function\n");
		return LCD_KIT_OK;
	}
}

int lcd_kit_create_sysfs(struct kobject* obj)
{
	int rc;
	int i = 0;
	int count = 0;

	for (i = 0; i < (int)(sizeof(lcd_kit_conf)/sizeof(lcd_kit_conf[0])); i++) {
		if (i >= (LCD_KIT_SYSFS_MAX - 1)) {
			LCD_KIT_ERR("dev attr number exceed sysfs max\n");
			return LCD_KIT_FAIL;
		}
		if (lcd_kit_check_support(i) && count < LCD_KIT_SYSFS_MAX) {
			lcd_kit_sysfs_attrs[count++] = lcd_kit_conf[i];
		}
	}
	rc = sysfs_create_group(obj, &lcd_kit_sysfs_attr_group);
	if (rc) {
		LCD_KIT_ERR("sysfs group creation failed, rc=%d\n", rc);
	}
	return rc;
}

void lcd_kit_remove_sysfs(struct kobject* obj)
{
	sysfs_remove_group(obj, &lcd_kit_sysfs_attr_group);
}
