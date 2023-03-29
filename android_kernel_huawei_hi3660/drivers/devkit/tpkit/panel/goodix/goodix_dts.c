#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/of.h>

#include <linux/i2c.h>
#include <linux/delay.h>
#include "goodix_ts.h"
#include "goodix_dts.h"
/**
 * goodix_parse_dts - parse goodix private properties
 */
int goodix_parse_dts(struct goodix_ts_data *ts)
{
	struct device_node *device = ts->pdev->dev.of_node;
	int ret = 0;

	/* get tp color flag */
	ret = of_property_read_u32(device,  "support_get_tp_color", &ts->support_get_tp_color);
	if (ret) {
		TS_LOG_INFO("%s, get device support_get_tp_color failed, will use default value: 0 \n ", __func__);
		ts->support_get_tp_color = 0; //default 0: no need know tp color
	}
	TS_LOG_INFO("%s, support_get_tp_color = %d \n", __func__, ts->support_get_tp_color);

	ret = of_property_read_u32(device, GTP_PRAM_PROJECTID_ADDR, &ts->pram_projectid_addr);
	if (ret) {
		ts->pram_projectid_addr = GTP_REG_PROJECT_ID;
		TS_LOG_INFO("%s:get pram_projectid_addr from dts failed ,use default gtp pram addr\n", __func__);
	}

	ret = of_property_read_u32(device, GTP_X_MAX_MT, &ts->max_x);
	if (ret) {
		GTP_ERROR("Get x_max_mt failed");
		ret = -EINVAL;
		goto err;
	}

	ret = of_property_read_u32(device, GTP_Y_MAX_MT, &ts->max_y);
	if (ret) {
		GTP_ERROR("Get y_max_mt failed");
		ret = -EINVAL;
		goto err;
	}

#ifdef ROI
	ret = of_property_read_u32_index(device, "roi_data_size", 0,
			&ts->roi.roi_rows);
	if (ret) {
		GTP_ERROR("Get ROI rows failed");
		ret = -EINVAL;
		goto err;
	}

	ret = of_property_read_u32_index(device, "roi_data_size", 1,
			&ts->roi.roi_cols);
	if (ret) {
		GTP_ERROR("Get ROI cols failed");
		ret = -EINVAL;
		goto err;
	}
#endif

	ret = of_property_read_bool(device, "tools_support");
	if (!ret) {
		ts->tools_support = true;
		GTP_INFO("Tools support enabled");
	}

	ret = of_property_read_u32(device, GTP_OPEN_ONCE_THRESHOLD, &ts->only_open_once_captest_threshold);
	if (ret) {
		ts->only_open_once_captest_threshold = 0;
		GTP_INFO("get only_open_once_captest_threshold from dts failed,use default 0");
	} else {
		GTP_INFO("get only_open_once_captest_threshold = %d", ts->only_open_once_captest_threshold);
	}

	ret = of_property_read_u32(device, GTP_LOAD_CFG_VIA_PROJECT_ID, &ts->load_cfg_via_project_id);
	if (ret) {
		ts->load_cfg_via_project_id = 0;
		GTP_INFO("get load_cfg_via_project_id from dts failed,use default 0");
	} else {
		GTP_INFO("get load_cfg_via_project_id = %d", ts->load_cfg_via_project_id);
	}

	return 0;
err:
	return ret;
}
int goodix_parse_specific_dts(struct goodix_ts_data *ts)
{
	struct device_node *device;
	const char *producer = NULL;
	char project_id[20] = {0};
	u32 value;
	int ret = 0;

	sprintf(project_id, "goodix-sensorid-%u", ts->hw_info.sensor_id);
	GTP_INFO("Parse specific dts:%s", project_id);
	device = of_find_compatible_node(ts->pdev->dev.of_node, NULL, project_id);
	if (!device) {
		GTP_INFO("No chip specific dts: %s, need to parse",
			    project_id);
		return -EINVAL;
	}
	/*goodix get vender name from dts*/
	ret = of_property_read_string(device, "producer", &producer);
	if (!ret) {
		strncpy(ts->vendor_name, producer, GTP_VENDOR_NAME_LEN);
		strncpy(ts->dev_data->module_name,producer,GTP_VENDOR_NAME_LEN);
		TS_LOG_INFO("GTP read vendor name:%s\n", ts->vendor_name);
	} else {
		TS_LOG_ERR("GTP read vendor name fail, ret=%d\n", ret);
	}

	ret = of_property_read_u32(device, GTP_X_MAX_MT, &value);
	if (!ret)
		ts->max_x = value;

	ret = of_property_read_u32(device, GTP_Y_MAX_MT, &value);
	if (!ret)
		ts->max_y = value;

	return 0;
}

/**
 * goodix_parse_dt_cfg - parse config data from devices tree.
 * @dev: device that this driver attached.
 * @cfg: pointer of the config array.
 * @cfg_len: pointer of the config length.
 * @sid: sensor id.
 * Return: 0-succeed, -1-faileds
 */
int goodix_parse_cfg_data(struct goodix_ts_data *ts,
				char *cfg_type, u8 *cfg, int *cfg_len, u8 sid)
{
	struct device_node *self;
	struct property *prop;
	char project_id[20];
	int correct_len;

	if (ts->load_cfg_via_project_id) {
		self = of_find_node_by_name(ts->pdev->dev.of_node, ts->project_id);
		if (!self) {
			GTP_ERROR("Find %s node error", ts->project_id);
			return -EINVAL;
		}
		GTP_INFO("Parse [%s] data from dts[SENSORID:%u][ProjectID:%s]", cfg_type,
			sid, ts->project_id);
	} else {
		sprintf(project_id, "goodix-sensorid-%u", sid);
		self = of_find_compatible_node(ts->pdev->dev.of_node,
				NULL, project_id);
		if (!self) {
			GTP_ERROR("No chip specific dts: %s, need to parse",
				    project_id);
			return -EINVAL;
		}
		GTP_INFO("Parse [%s] data from dts[SENSORID:%u]", cfg_type, sid);
	}

	prop = of_find_property(self, cfg_type, cfg_len);
	if (!prop || !prop->value || *cfg_len == 0)
		return -EINVAL;/* fail */
	memcpy(cfg, prop->value, *cfg_len);
	if (*cfg_len > GTP_CONFIG_ORG_LENGTH &&
					cfg[EXTERN_CFG_OFFSET] & 0x40)
		/* extends config */
		correct_len = GTP_CONFIG_ORG_LENGTH + GTP_CONFIG_EXT_LENGTH;
	else
		correct_len = GTP_CONFIG_ORG_LENGTH;

	if (*cfg_len > correct_len) {
		GTP_ERROR("Invalid config size:%d", *cfg_len);
		return -EINVAL;
	}

	return 0;
}

int goodix_chip_parse_config(struct device_node *device,
				struct ts_kit_device_data *chip_data)
{
	int ret = 0;
	const char *str_value = NULL;

	GTP_INFO("Parse config");
	if (!device || !chip_data)
		return -ENODEV;
	ret = of_property_read_u32(device, GTP_IRQ_CFG,
						&chip_data->irq_config);
	if (ret) {
		GTP_ERROR("Get irq config failed");
		ret = -EINVAL;
		goto err;
	}

	ret = of_property_read_u32(device, GTP_ALGO_ID,
						&chip_data->algo_id);
	if (ret) {
		GTP_ERROR("Get algo id failed");
		ret = -EINVAL;
		goto err;
	}

	ret = of_property_read_u32(device, GTP_WD_CHECK,
						&chip_data->need_wd_check_status);
	if (ret) {
		GTP_ERROR("need_wd_check_status parmerter not exit use data");
		chip_data->need_wd_check_status = false;
	}

	ret = of_property_read_u32(device, GTP_WD_TIMEOUT,
						&chip_data->check_status_watchdog_timeout);
	if (ret) {
		GTP_ERROR("check_status_watchdog_timeout not exit use data");
		chip_data->check_status_watchdog_timeout = 0;
	}

	ret = of_property_read_u32(device, GTP_GESTURE_SUPPORTED,
						&chip_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value);
	if (ret) {
		chip_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value = false;
	}
	TS_LOG_INFO("gesture_supported = %d\n",chip_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value);

	ret = of_property_read_string(device, GTP_TEST_TYPE, &str_value);
	if (ret) {
		GTP_INFO("tp_test_type not exit,use default value");
		strncpy(chip_data->tp_test_type, GTP_TEST_TYPE_DEFAULT, TS_CAP_TEST_TYPE_LEN);
		ret = 0;
	}  else {
		strncpy(chip_data->tp_test_type, str_value, TS_CAP_TEST_TYPE_LEN);
		TS_LOG_INFO("tp_test_type = %s\n", chip_data->tp_test_type);
	}

	ret = of_property_read_u32(device, TEST_CAPACITANCE_VIA_CSVFILE, &chip_data->test_capacitance_via_csvfile);
	if (ret) {
		TS_LOG_INFO("test_capacitance_via_csvfile is 0.\n");
		chip_data->test_capacitance_via_csvfile = 0;
		ret = 0;
	} else {
		TS_LOG_INFO("test_capacitance_via_csvfile = %d\n", chip_data->test_capacitance_via_csvfile);
	}

	ret = of_property_read_u32(device, CSVFILE_USE_PRODUCT_SYSTEM_TYPE, &chip_data->csvfile_use_product_system);
	if (ret) {
		TS_LOG_INFO("csvfile use product system is 0.\n");
		chip_data->csvfile_use_product_system = 0;
		ret = 0;
	} else {
		TS_LOG_INFO("csvfile_use_product_system = %d\n", chip_data->csvfile_use_product_system);
	}

err:
	return ret;
}

