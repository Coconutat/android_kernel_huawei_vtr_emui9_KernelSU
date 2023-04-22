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

#ifndef __LCD_KIT_FNODE_H_
#define __LCD_KIT_FNODE_H_

#define LCD_KIT_SYSFS_MAX	101
enum lcd_kit_sysfs_index{
	LCD_MODEL_INDEX,
	LCD_TYPE_INDEX,
	PANEL_INFO_INDEX,
	INVERSION_INDEX,
	SCAN_INDEX,
	CHECK_REG_INDEX,
	CHECKSUM_INDEX,
	SLEEP_CTRL_INDEX,
	HKADC_INDEX,
	VOLTAGE_ENABLE_INDEX,
	PCD_ERRFLAG_INDEX,
	ACL_INDEX,
	VR_INDEX,
	SUPPORT_MODE_INDEX,
	GAMMA_DYNAMIC_INDEX,
	FRAME_COUNT_INDEX,
	FRAME_UPDATE_INDEX,
	MIPI_DSI_CLK_UPT_INDEX,
	FPS_SCENCE_INDEX,
	ALPM_FUNCTION_INDEX,
	ALPM_SETTING_INDEX,
	FUNC_SWITCH_INDEX,
	TEST_CONFIG_INDEX,
	LV_DETECT_INDEX,
	CURRENT_DETECT_INDEX,
	REG_READ_INDEX,
	DDIC_OEM_INDEX,
	BL_MODE_INDEX,
	BL_SUPPORT_MODE_INDEX,
	LDO_CHECK_INDEX,
	BL_SELF_TEST_INDEX,
	EFFECT_BL_INDEX,
};
/*sysfs support enum*/
enum lcd_kit_sysfs_support{
	SYSFS_NOT_SUPPORT,
	SYSFS_SUPPORT,
};

struct lcd_kit_sysfs_ops* lcd_kit_get_sysfs_ops(void);
int lcd_kit_sysfs_ops_register(struct lcd_kit_sysfs_ops* ops);
int lcd_kit_sysfs_ops_unregister(struct lcd_kit_sysfs_ops* ops);
int lcd_kit_create_sysfs(struct kobject* obj);
void lcd_kit_remove_sysfs(struct kobject* obj);

struct lcd_kit_sysfs_ops {
	int (*check_support)(int index);
	ssize_t (*model_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*type_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*panel_info_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*inversion_mode_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*inversion_mode_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*scan_mode_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*scan_mode_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*check_reg_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*gram_check_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*gram_check_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*sleep_ctrl_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*sleep_ctrl_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*hkadc_debug_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*hkadc_debug_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*voltage_enable_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*pcd_errflag_check_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*amoled_acl_ctrl_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*amoled_acl_ctrl_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*amoled_vr_mode_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*amoled_vr_mode_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*oem_info_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*oem_info_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*effect_color_mode_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*effect_color_mode_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*test_config_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*test_config_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*reg_read_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*reg_read_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*bl_mode_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*bl_mode_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*gamma_dynamic_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*frame_count_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*frame_update_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*frame_update_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*mipi_dsi_clk_upt_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*mipi_dsi_clk_upt_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*fps_scence_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*fps_scence_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*alpm_function_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*alpm_function_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*alpm_setting_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*func_switch_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*func_switch_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*lv_detect_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*current_detect_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*ddic_oem_info_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*ddic_oem_info_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*support_bl_mode_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*support_bl_mode_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
	ssize_t (*ldo_check_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*bl_self_test_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*effect_bl_show)(struct device* dev, struct device_attribute* attr, char* buf);
	ssize_t (*effect_bl_store)(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
};
#endif
