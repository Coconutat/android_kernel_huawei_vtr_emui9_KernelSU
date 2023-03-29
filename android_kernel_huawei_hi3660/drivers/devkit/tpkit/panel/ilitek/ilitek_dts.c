#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/of.h>

#include <linux/i2c.h>
#include <linux/delay.h>
#include "ilitek_dts.h"
#include "ilitek_ts.h"
extern struct i2c_data * ilitek_data;
int ilitek_get_vendor_compatible_name(
	const char *project_id,
	char *comp_name,
	size_t size)
{
	int ret = 0;

	ret = snprintf(comp_name, size,"%s-%s", ILITEK_CHIP_NAME, project_id);
	if (ret >= size) {
		TS_LOG_INFO("%s:%s, ret=%d, size=%lu\n", __func__,
			"compatible_name out of range", ret, size);
		return -EINVAL;
	}

	return 0;
}

int ilitek_get_vendor_name_from_dts(
	const char *project_id,
	char *vendor_name,
	size_t size)
{
	int ret = 0;
	const char *producer = NULL;
	char comp_name[ILITEK_VENDOR_COMP_NAME_LEN] = {0};

	struct device_node *np = NULL;

	ret = ilitek_get_vendor_compatible_name(project_id,
		comp_name, ILITEK_VENDOR_COMP_NAME_LEN);
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
		TS_LOG_INFO("ilitek producer = %s\n", producer);
		strncpy(vendor_name, producer, size);
	} else {
		TS_LOG_ERR("%s:find producer in dts fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	return 0;
}

static void ilitek_of_property_read_u32_default(
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

static void ilitek_of_property_read_u16_default(
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

/*  query the configure from dts and store in prv_data */
int ilitek_parse_dts(struct device_node *device,
			       struct ts_kit_device_data *chip_data)
{
	int retval = NO_ERR;
	const char *raw_data_dts = NULL;
	int index = 0;
	int array_len = 0;
	retval = of_property_read_u32(device, ILITEK_SLAVE_ADDR, &chip_data->slave_addr);
	if (retval) {
		TS_LOG_ERR("get slave_addr failed use fault value\n");
		chip_data->slave_addr = 0x41;
	}
	TS_LOG_INFO("%s:%s=0x%x.\n", __func__, "slave_addr", chip_data->slave_addr);
	retval = of_property_read_u32(device, ILITEK_IRQ_CFG, &chip_data->irq_config);
	if (retval) {
		TS_LOG_ERR("get irq config failed\n");
	}
	retval = of_property_read_u32(device, ILITEK_ALGO_ID, &chip_data->algo_id);
	if (retval) {
		TS_LOG_ERR("get algo id failed\n");
	}
	retval = of_property_read_u32(device, ILITEK_VCI_LDO_VALUE, &chip_data->regulator_ctr.vci_value);
	if (retval) {
		TS_LOG_INFO("Not define Vci value in Dts, use fault value\n");
		chip_data->regulator_ctr.vci_value = 3100000;
	}

	retval = of_property_read_u32(device, ILITEK_NEED_SET_VDDIO_VALUE, &chip_data->regulator_ctr.need_set_vddio_value);
	if (retval) {
		TS_LOG_INFO("Not define need set Vddio value in Dts, use fault value\n");
		chip_data->regulator_ctr.need_set_vddio_value = 0;
	} else {
		retval = of_property_read_u32(device, ILITEK_VDDIO_LDO_VALUE, &chip_data->regulator_ctr.vddio_value);
		if (retval) {
			TS_LOG_INFO("Not define Vddio value in Dts, use fault value\n");
			chip_data->regulator_ctr.vddio_value = 1800000;
		}
	}
	retval =of_property_read_u32(device, ILITEK_WD_CHECK,&chip_data->need_wd_check_status);
	if (retval) {
		TS_LOG_ERR("get device ic_type failed\n");
	}
	//chip_data->need_wd_check_status = 0;
	TS_LOG_INFO("%s,chip_data->need_wd_check_status = %d\n",  __func__, chip_data->need_wd_check_status);
	retval = of_property_read_u32(device, "check_status_watchdog_timeout", &chip_data->check_status_watchdog_timeout);
	if (retval) {
		chip_data->check_status_watchdog_timeout = 0;
		TS_LOG_ERR("device check_status_watchdog_timeout not exit,use default value.\n");
	}else{
		TS_LOG_INFO("get device check_status_watchdog_timeout :%d\n", chip_data->check_status_watchdog_timeout);
	}
	retval = of_property_read_u32(device, ILITEK_UNIT_CAP_TEST_INTERFACE, &chip_data->unite_cap_test_interface);
	if (retval) {
		TS_LOG_ERR("get unite_cap_test_interface failed\n");
	}
	retval = of_property_read_u32(device, ILITEK_REPORT_RATE_TEST, &chip_data->report_rate_test);
	if (retval) {
		TS_LOG_ERR("get report_rate_test failed\n");
	}
	retval =  of_property_read_u32(device, ILITEK_X_MAX, &chip_data->x_max);
	if (retval) {
		TS_LOG_ERR("get device x_max failed\n");
	}
	retval =
	    of_property_read_u32(device, ILITEK_Y_MAX, &chip_data->y_max);
	if (retval) {
		TS_LOG_ERR("get device y_max failed\n");
	}

	retval =  of_property_read_u32(device, ILITEK_VCI_GPIO_TYPE, &chip_data->vci_gpio_type);
	if (retval) {
		TS_LOG_ERR("get device ILITEK_VCI_GPIO_TYPE failed\n");
	}
	retval = of_property_read_u32(device, ILITEK_VCI_REGULATOR_TYPE, &chip_data->vci_regulator_type);
	if (retval) {
		TS_LOG_ERR("get device ILITEK_VCI_REGULATOR_TYPE failed\n");
	}
	retval =  of_property_read_u32(device, ILITEK_VDDIO_GPIO_TYPE, &chip_data->vddio_gpio_type);
	if (retval) {
		TS_LOG_ERR("get device ILITEK_VDDIO_GPIO_TYPE failed\n");
	}
	retval =  of_property_read_u32(device, ILITEK_PROJECTID_LEN, &chip_data->projectid_len);
	if (retval) {
		TS_LOG_ERR("get device projectid_len failed\n");
		chip_data->projectid_len = 0;
	}
	retval = of_property_read_u32(device, ILITEK_VDDIO_REGULATOR_TYPE, &chip_data->vddio_regulator_type);
	if (retval) {
		TS_LOG_ERR("get device ILITEK_VDDIO_REGULATOR_TYPE failed\n");
	}
	retval = of_property_read_string(device, ILITEK_TEST_TYPE, &raw_data_dts);
	if (retval) {
		TS_LOG_INFO("get device ILITEK_TEST_TYPE not exit,use default value judge_last_result\n");
		strncpy(chip_data->tp_test_type,"Normalize_type:judge_last_result",TS_CAP_TEST_TYPE_LEN);
	}
	else {
		strncpy(chip_data->tp_test_type,raw_data_dts,TS_CAP_TEST_TYPE_LEN);
	}
	TS_LOG_INFO("ilitek chip_data->tp_test_type = %s\n", chip_data->tp_test_type);
	/*0 is power supplied by gpio, 1 is power supplied by ldo */
	if (1 == chip_data->vci_gpio_type) {
		chip_data->vci_gpio_ctrl = of_get_named_gpio(device, ILITEK_VCI_GPIO_CTRL, 0);
		if (!gpio_is_valid(chip_data->vci_gpio_ctrl)) {
			TS_LOG_ERR("SFT: ok; ASIC: Real err----power gpio is not valid\n");
		}
	}
	if (1 == chip_data->vddio_gpio_type) {
		chip_data->vddio_gpio_ctrl = of_get_named_gpio(device, ILITEK_VDDIO_GPIO_CTRL, 0);
		if (!gpio_is_valid(chip_data->vddio_gpio_ctrl)) {
			TS_LOG_ERR("SFT: ok; ASIC: Real err----power gpio is not valid\n");
		}
	}

	retval = of_property_read_u32(device, "charger_supported", &chip_data->ts_platform_data->feature_info.charger_info.charger_supported);
	if (!retval) {
		TS_LOG_INFO("get ts_platform_data.feature_info.charger_info.charger_supported = %d\n", 
			chip_data->ts_platform_data->feature_info.charger_info.charger_supported);
	} else {
		TS_LOG_INFO("can not ts_platform_data.feature_info.charger_info.charger_supported, use default\n");
		chip_data->ts_platform_data->feature_info.charger_info.charger_supported = 0;
	}
	TS_LOG_INFO("irq_config = %d, algo_id = %d, ic_type = %d, x_max = %d, y_max = %d, x_mt = %d,y_mt = %d, bootloader_update_enable = %d\n",
	     chip_data->irq_config,
	     chip_data->algo_id, chip_data->ic_type, chip_data->x_max,
	     chip_data->y_max, chip_data->x_max_mt, chip_data->y_max_mt,
	     chip_data->bootloader_update_enable);

	return NO_ERR;
}
