#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/of.h>

#include <linux/i2c.h>
#include <linux/delay.h>

#include "focaltech_test.h"
#include "focaltech_core.h"
#include "focaltech_dts.h"
#include "focaltech_flash.h"

int focal_get_vendor_compatible_name(
	const char *project_id,
	char *comp_name,
	size_t size)
{
	int ret = 0;

	ret = snprintf(comp_name, size,	"%s-%s", FTS_CHIP_NAME, project_id);
	if (ret >= size) {
		TS_LOG_INFO("%s:%s, ret=%d, size=%lu\n", __func__,
			"compatible_name out of range", ret, size);
		return -EINVAL;
	}

	return 0;
}

int focal_get_vendor_name_from_dts(
	char *project_id,
	char *vendor_name,
	size_t size)
{
	int ret = 0;
	const char *producer = NULL;
	const char *new_project_id = NULL;
	char comp_name[FTS_VENDOR_COMP_NAME_LEN] = {0};
	u32 value = 0;

	struct device_node *np = NULL;

	ret = focal_get_vendor_compatible_name(project_id,
		comp_name, FTS_VENDOR_COMP_NAME_LEN);
	if (ret) {
		TS_LOG_ERR("%s:get vendor compatible name fail\n", __func__);
		return ret;
	}

	TS_LOG_INFO("%s:compatible_name is: %s\n", __func__, comp_name);
	np = of_find_compatible_node(NULL, NULL, comp_name);
	if (!np) {
		TS_LOG_INFO("%s:find vendor node fail\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_string(np, "producer", &producer);
	if (!ret) {
		strncpy(vendor_name, producer, size);
	} else {
		TS_LOG_ERR("%s:find producer in dts fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	ret = of_property_read_string(np, "project_id", &new_project_id);
	if (!ret) {
		snprintf(project_id, FTS_PROJECT_ID_LEN - 1,"%s", new_project_id);
	}
	TS_LOG_INFO("%s:project_id: %s\n", __func__, project_id);


	return 0;
}

static void focal_of_property_read_u32_default(
	struct device_node *np,
	char *prop_name,
	u32 *out_value,
	u32 default_value)
{
	int ret = 0;

	ret = of_property_read_u32(np, prop_name, out_value);
	if (ret) {
		TS_LOG_INFO("%s:%s not set in dts, use default\n",
			__func__, prop_name);
		*out_value = default_value;
	}
}

static void focal_of_property_read_u16_default(
	struct device_node *np,
	char *prop_name,
	u16 *out_value,
	u16 default_value)
{
	int ret = 0;

	ret = of_property_read_u16(np, prop_name, out_value);
	if (ret) {
		TS_LOG_INFO("%s:%s not set in dts, use default\n",
			__func__, prop_name);
		*out_value = default_value;
	}
}

static void focal_of_property_read_u8_default(
	struct device_node *np,
	char *prop_name,
	u8 *out_value,
	u8 default_value)
{
	int ret = 0;

	ret = of_property_read_u8(np, prop_name, out_value);
	if (ret) {
		TS_LOG_INFO("%s:%s not set in dts, use default\n",
			__func__, prop_name);
		*out_value = default_value;
	}
}

int focal_prase_ic_config_dts(
	struct device_node *np,
	struct ts_kit_device_data *dev_data)
{
	focal_of_property_read_u32_default(np, FTS_POWER_SELF_CTRL, &g_focal_pdata->self_ctrl_power, 0);
	TS_LOG_INFO("%s: %s= %d\n", __func__, FTS_POWER_SELF_CTRL, g_focal_pdata->self_ctrl_power);

	return NO_ERR;
}
static void focal_prase_delay_config_dts(
	struct device_node *np,
	struct focal_platform_data *pdata)
{
	struct focal_delay_time *delay_time = NULL;

	delay_time = pdata->delay_time;

	focal_of_property_read_u32_default(np, FTS_HARD_RESET_DELAY,
		&delay_time->hard_reset_delay, 200);

	focal_of_property_read_u32_default(np, FTS_ERASE_MIN_DELAY,
		&delay_time->erase_min_delay, 1350);

	focal_of_property_read_u32_default(np, FTS_CALC_CRC_DELAY,
		&delay_time->calc_crc_delay, 300);

	focal_of_property_read_u32_default(np, FTS_REBOOT_DELAY,
		&delay_time->reboot_delay, 200);

	focal_of_property_read_u32_default(np, FTS_ERASE_QUERY_DELAY,
		&delay_time->erase_query_delay, 50);

	focal_of_property_read_u32_default(np, FTS_WRITE_FLASH_QUERY_TIMES,
		&delay_time->write_flash_query_times, 30);

	focal_of_property_read_u32_default(np, FTS_READ_ECC_QUERY_TIMES,
		&delay_time->read_ecc_query_times, 100);

	focal_of_property_read_u32_default(np, FTS_ERASE_FLASH_QUERY_TIMES,
		&delay_time->erase_flash_query_times, 15);

	focal_of_property_read_u32_default(np, FTS_UPGRADE_LOOP_TIMES,
		&delay_time->upgrade_loop_times, 30);

	TS_LOG_INFO("%s:%s=%d, %s=%d, %s=%d, %s=%d, %s=%d\n", __func__,
		"hard_reset_delay", delay_time->hard_reset_delay,
		"reboot_delay", delay_time->reboot_delay,
		"erase_min_delay", delay_time->erase_min_delay,
		"erase_query_delay", delay_time->erase_query_delay,
		"calc_crc_delay", delay_time->calc_crc_delay);

	TS_LOG_INFO("%s:%s=%d, %s=%d, %s=%d, %s=%d", __func__,
		"erase_flash_query_times", delay_time->erase_flash_query_times,
		"read_ecc_query_times", delay_time->read_ecc_query_times,
		"write_flash_query_times", delay_time->write_flash_query_times,
		"upgrade_loop_times", delay_time->upgrade_loop_times);
}

static int focal_get_lcd_panel_info(void)
{
	struct device_node *dev_node = NULL;
	char *lcd_type = NULL;
	struct focal_platform_data *focal_pdata = g_focal_pdata;

	dev_node = of_find_compatible_node(NULL, NULL, LCD_PANEL_TYPE_DEVICE_NODE_NAME);
	if (!dev_node) {
		TS_LOG_ERR("%s: NOT found device node[%s]!\n", __func__, LCD_PANEL_TYPE_DEVICE_NODE_NAME);
		return -EINVAL;
	}

	lcd_type = (char*)of_get_property(dev_node, "lcd_panel_type", NULL);
	if(!lcd_type){
		TS_LOG_ERR("%s: Get lcd panel type faile!\n", __func__);
		return -EINVAL ;
	}

	strncpy(focal_pdata->lcd_panel_info, lcd_type, LCD_PANEL_INFO_MAX_LEN-1);

	return 0;
}

int focal_parse_dts(
	struct device_node *np,
	struct focal_platform_data *focal_pdata)
{
	int ret = 0;
	int read_val = 0;
	unsigned int value = 0;
	int project_atm = 0;
	const char *str_value = NULL;
	struct ts_glove_info *glove_info = NULL;
	struct ts_holster_info *holster_info = NULL;
	struct ts_roi_info *roi_info = NULL;
	struct ts_wakeup_gesture_enable_info *gesture_info = NULL;

	struct ts_kit_device_data *dev_data = NULL;

	dev_data = focal_pdata->focal_device_data;

	focal_of_property_read_u32_default(np, FTS_RESET_SELF_CTRL, &g_focal_pdata->self_ctrl_reset, 0);

	focal_prase_delay_config_dts(np, focal_pdata);

	ret = of_property_read_u32(np, FTS_PRAM_PROJECTID_ADDR, &focal_pdata->pram_projectid_addr);
	if (ret) {
		focal_pdata->pram_projectid_addr = FTS_BOOT_PROJ_CODE_ADDR2;
		TS_LOG_INFO("%s:get pram_projectid_addr from dts failed ,use default FT8716 pram addr\n", __func__);
	}
	/* get tp color flag */
	focal_pdata->support_get_tp_color = of_property_read_bool(np, "support_get_tp_color");
	TS_LOG_INFO("%s, support get tp color = %d \n", __func__, focal_pdata->support_get_tp_color);

	ret = of_property_read_u32(np, FTS_ENABLE_EDGE_TOUCH, &focal_pdata->enable_edge_touch);
	if (ret) {
		focal_pdata->enable_edge_touch = 0;
		TS_LOG_INFO("%s:get enable_edge_touch from dts failed\n", __func__);
	}
	TS_LOG_INFO("%s: get enable_edge_touch = %d\n", __func__, focal_pdata->enable_edge_touch);

	if (focal_pdata->enable_edge_touch) {
		ret = of_property_read_u32(np, FTS_EDGE_DATA_ADDR, &focal_pdata->edge_data_addr);
		if (ret) {
			focal_pdata->edge_data_addr = 0;
			TS_LOG_INFO("%s:get edge_data_addr from dts failed\n", __func__);
		}
		TS_LOG_INFO("%s: get edge_data_addr = %d\n", __func__,
			focal_pdata->edge_data_addr);
	} else {
		focal_pdata->edge_data_addr = 0;
	}

	ret = of_property_read_u32(np, FTS_FW_NEED_DISTINGUISH_LCD, &focal_pdata->need_distinguish_lcd);
	if (ret) {
		focal_pdata->need_distinguish_lcd = 0;
		TS_LOG_INFO("%s: get need_distinguish_lcd from dts failed, use default(0)\n", __func__);
	}

	ret = of_property_read_u32(np, FTS_FW_ONLY_DEPEND_ON_LCD, &focal_pdata->fw_only_depend_on_lcd);
	if (ret) {
		focal_pdata->fw_only_depend_on_lcd = 0;
		TS_LOG_INFO("%s: get fw_only_depend_on_lcd from dts failed, use default(0)\n", __func__);
	}

	ret = of_property_read_u32(np, FTS_OPEN_ONCE_THRESHOLD, &focal_pdata->only_open_once_captest_threshold);
	if (ret) {
		focal_pdata->only_open_once_captest_threshold = 0;
		TS_LOG_INFO("%s: get only_open_once_captest_threshold from dts failed, use default(0)\n", __func__);
	}

	if (focal_pdata->need_distinguish_lcd) {
		focal_get_lcd_panel_info();
		ret = of_property_read_u32(np, FTS_HIDE_PLAIN_LCD_LOG, &focal_pdata->hide_plain_lcd_log);
		if (ret) {
			focal_pdata->hide_plain_lcd_log = 0;
			TS_LOG_INFO("%s: get hide_plain_lcd_log from dts failed, use default(0)\n", __func__);
		}
		TS_LOG_INFO("%s: get hide_plain_lcd_log from is %d \n", __func__, focal_pdata->hide_plain_lcd_log);

	}

	if (focal_pdata->focal_device_data->touch_switch_flag) {
		if (TS_SWITCH_TYPE_GAME == (focal_pdata->focal_device_data->touch_switch_flag & TS_SWITCH_TYPE_GAME)) {
			ret = of_property_read_u32(np, FTS_TOUCH_SWITCH_GAME_REG, &value);
			if (ret) {
				TS_LOG_INFO("%s get touch_switch_game_reg from dts failed, use default(0).\n", __func__);
				focal_pdata->touch_switch_game_reg = 0;
			} else {
				focal_pdata->touch_switch_game_reg = (u8)(value & 0xFF);
				TS_LOG_INFO("%s get touch_switch_game_reg from dts succ, use value(0x%x).\n", __func__, focal_pdata->touch_switch_game_reg);
			}
		}

		if (TS_SWITCH_TYPE_SCENE == (focal_pdata->focal_device_data->touch_switch_flag & TS_SWITCH_TYPE_SCENE)) {
			value = 0;
			ret = of_property_read_u32(np, FTS_TOUCH_SWITCH_SCENE_REG, &value);
			if (ret) {
				TS_LOG_INFO("%s get touch_switch_scene_reg from dts failed, use default(0).\n", __func__);
				focal_pdata->touch_switch_scene_reg = 0;
			} else {
				focal_pdata->touch_switch_scene_reg = (u8)(value & 0xFF);
				TS_LOG_INFO("%s get touch_switch_scene_reg from dts succ, use value(0x%x).\n", __func__, focal_pdata->touch_switch_scene_reg);
			}
		}
	}

	ret = of_property_read_u32(np, FTS_USE_PINCTRL, &focal_pdata->fts_use_pinctrl);
	if (ret) {
		TS_LOG_INFO("%s get fts_use_pinctrl from dts failed, use default(0).\n", __func__);
		focal_pdata->fts_use_pinctrl = 0;
	}

	ret = of_property_read_u32(np, FTS_FW_UPDATE_DURATION_CHECK, &focal_pdata->fw_update_duration_check);
	if (ret) {
		TS_LOG_INFO("%s get fw_update_duration_check from dts unsucceed, use default(0).\n", __func__);
		focal_pdata->fw_update_duration_check = 0;
	} else {
		TS_LOG_INFO("%s get fw_update_duration_check from dts succeed, use cfg(%d ms).\n",
				__func__, focal_pdata->fw_update_duration_check);
	}

	value = 0;
	ret = of_property_read_u32(np, FTS_READ_DEBUG_REG_AND_DIFFER, &value);
	if (ret) {
		TS_LOG_INFO("%s get read_debug_reg_and_differ from dts unsucceed.\n", __func__);
		focal_pdata->read_debug_reg_and_differ = 0;
	} else {
		focal_pdata->read_debug_reg_and_differ = (u8)(value & 0xFF);
		TS_LOG_INFO("%s get read_debug_reg_and_differ = %d\n",__func__, focal_pdata->read_debug_reg_and_differ);
	}

	ret = of_property_read_u32(np, "aft_wxy_enable", &focal_pdata->aft_wxy_enable);
	if (ret) {
		TS_LOG_INFO("%s get aft_wxy_enable from dts failed, use default(0).\n", __func__);
		focal_pdata->aft_wxy_enable = 0;
	} else {
		TS_LOG_INFO("%s get aft_wxy_enable  = %d.\n", __func__, focal_pdata->aft_wxy_enable);
	}


	focal_of_property_read_u32_default(np, FTS_PROJECTID_LEN_CTRL_FLAG,
		&focal_pdata->projectid_length_control_flag, 0);
	TS_LOG_INFO("%s:projectid_len_control =%d\n", __func__, focal_pdata->projectid_length_control_flag);


	TS_LOG_INFO("%s: pram_projectid_addr  is  %x \n", __func__,focal_pdata->pram_projectid_addr);

	return NO_ERR;
}

static void focal_prase_test_item(struct device_node *np,
	struct focal_test_params *params)
{
	focal_of_property_read_u32_default(np, FTS_ROW_COLUMN_DELTA_TEST,
		&params->row_column_delta, 0);
	focal_of_property_read_u32_default(np, FTS_ROW_COLUMN_DELTA_TEST_POINT_BY_POINT,
		&params->row_column_delta_test_point_by_point, 0);
	focal_of_property_read_u32_default(np, FTS_LCD_NOISE_DATA_TEST,
		&params->lcd_noise_data, 0);

	focal_of_property_read_u32_default(np, FTS_OPENTEST_CHARGE_TIME,
		&params->opentest_charge_time, 0);

	focal_of_property_read_u32_default(np, FTS_OPENTEST_RESET_TIME,
		&params->opentest_reset_time, 0);

	focal_of_property_read_u32_default(np, FTS_CB_TEST_POINT_BY_POINT,
		&params->cb_test_point_by_point, 0);

	focal_of_property_read_u32_default(np, FTS_OPEN_TEST_POINT_BY_POINT,
		&params->open_test_cb_point_by_point, 0);
	focal_of_property_read_u32_default(np, FTS_SHORT_TEST_POINT_BY_POINT,
		&params->short_test_point_by_point, 0);



	TS_LOG_INFO("%s:%s=%d, %s=%d, %s=%d, %s=%d, %s=%d, %s=%d,\n", __func__,
		"row_column_delta_test", params->row_column_delta,
		"opentest_charge_time", params->opentest_charge_time,
		"opentest_reset_time", params->opentest_reset_time,
		"cb_test_point_by_point", params->cb_test_point_by_point,
		"open_test_cb_point_by_point", params->open_test_cb_point_by_point,
		"short_test_point_by_point", params->short_test_point_by_point);
}

static void focal_prase_test_threshold(
	struct device_node *np,
	struct focal_test_threshold *threshold)
{
	focal_of_property_read_u32_default(np, DTS_RAW_DATA_MIN,
		&threshold->raw_data_min, 0);

	focal_of_property_read_u32_default(np, DTS_RAW_DATA_MAX,
		&threshold->raw_data_max, 0);

	focal_of_property_read_u32_default(np, DTS_CB_TEST_MIN,
		&threshold->cb_test_min, 0);

	focal_of_property_read_u32_default(np, DTS_CB_TEST_MAX,
		&threshold->cb_test_max, 0);

	focal_of_property_read_u32_default(np, DTS_OPEN_TEST_CB_MIN,
		&threshold->open_test_cb_min, 0);

	focal_of_property_read_u32_default(np, DTS_SCAP_RAW_DATA_MIN,
		&threshold->scap_raw_data_min, 0);
	focal_of_property_read_u32_default(np, DTS_SCAP_RAW_DATA_MAX,
		&threshold->scap_raw_data_max, 0);

	/* short_circuit_test */
	focal_of_property_read_u32_default(np, DTS_SHORT_CIRCUIT_RES_MIN,
		&threshold->short_circuit_min, 0);

	focal_of_property_read_u32_default(np, DTS_LCD_NOISE_MAX,
		&threshold->lcd_noise_max, 0);

	TS_LOG_INFO("%s:%s:%s=%d, %s=%d, %s=%d, %s=%d, %s=%d, %s=%d\n",
		__func__, "cb test thresholds",
		"raw_data_min", threshold->raw_data_min,
		"raw_data_max", threshold->raw_data_max,
		"cb_test_min",  threshold->cb_test_min,
		"cb_test_max",  threshold->cb_test_max,
		"open_test_cb_min", threshold->open_test_cb_min,
		"lcd_noise_max", threshold->lcd_noise_max,
		"short_circuit_min", threshold->short_circuit_min);
}

int focal_parse_cap_test_config(
	struct focal_platform_data *pdata,
	struct focal_test_params *params)
{
	int ret = 0;

	char comp_name[FTS_VENDOR_COMP_NAME_LEN] = {0};
	struct device_node *np = NULL;
	struct focal_platform_data *fts_pdata = focal_get_platform_data();

	if (!fts_pdata) {
		TS_LOG_ERR("%s:chip data null\n", __func__);
		return -EINVAL;
	}

	/*
	 * Deleted fts_read_project_id here, project_id has already readed 
	 * in chip_init process
	 */

	ret = focal_get_vendor_compatible_name(fts_pdata->project_id, comp_name,
		FTS_VENDOR_COMP_NAME_LEN);
	if (ret) {
		TS_LOG_ERR("%s:get compatible name fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	np = of_find_compatible_node(NULL, NULL, comp_name);
	if (!np) {
		TS_LOG_ERR("%s:find dev node faile, compatible name:%s\n",
			__func__, comp_name);
		return -ENODEV;
	}

	focal_prase_test_item(np, params);

	focal_of_property_read_u32_default(np, FTS_IN_CSV_FILE,
		&params->in_csv_file, 0);

	focal_of_property_read_u32_default(np, FTS_POINT_BY_POINT_JUDGE,
		&params->point_by_point_judge, 0);

	if (FTS_THRESHOLD_IN_CSV_FILE == params->in_csv_file) {
		TS_LOG_INFO("%s: cap threshold in csv file\n", __func__);
		if (FOCAL_FT8201 == g_focal_dev_data->ic_type) {
			focal_8201_prase_threshold_for_csv(fts_pdata->project_id, &params->threshold, params);
		} else {
			focal_prase_threshold_for_csv(fts_pdata->project_id, &params->threshold, params);
		}
	} else {
		TS_LOG_INFO("%s: cap threshold in dts file\n", __func__);
		focal_prase_test_threshold(np, &params->threshold);
	}

	return 0;
}

