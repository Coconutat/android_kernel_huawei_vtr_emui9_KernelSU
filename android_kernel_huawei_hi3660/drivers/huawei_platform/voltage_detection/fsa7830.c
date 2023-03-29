

/*	linux	*/
#include <linux/hisi/hisi_adc.h>

/*	huawei	*/
#include <huawei_platform/log/hw_log.h>

/*	module fsa7830	*/
#include "fsa7830.h"

#include "hw_voltage_dev.h"

/*	define register	*/
#define FSA7830_DEVICEID			0x01
#define FSA7830_CONTROL			0x02
#define FSA7830_SWCTL				0x03
#define FSA7830_INT				0x04
#define FSA7830_INT_MASK			0x05
#define FSA7830_OVP				0x06

/* reg default value	*/

#define FSA7830_REG_INT_MASK			0x80
#define FSA7830_REG_CONTROL_MASK		0xf0
#define FSA7830_REG_OVP_MASK			0x07

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG fsa7830
HWLOG_REGIST();

/* ****************************************************************************/
static int fsa7830_chip_flag;
int fsa7830_no_exist_flag;

#if 0
static int fsa7830_check_reg_val(char reg_addr, char *reg_val)
{
	char val = *reg_val;
	int addr = (int)reg_addr;
	int ret = 0;

	switch (addr) {
	case FSA7830_CONTROL:
		val &= FSA7830_REG_CONTROL_MASK;
		break;
	case FSA7830_SWCTL:
		break;
	case FSA7830_INT_MASK:
		val &= FSA7830_REG_INT_MASK;
		break;
	case FSA7830_OVP:
		val &= FSA7830_REG_OVP_MASK;
		break;
	case FSA7830_DEVICEID:
	case FSA7830_INT:
	default:
		ret = -1;
		hwlog_err("[%s] invalid addr! addr:0x%x\n", __func__, addr);
		break;
	}

	*reg_val = val;
	return ret;
}
#endif

/*
*	if need try 3 times add in this func
*/
static int fsa7830_get_volt(int channel)
{
	return hisi_adc_get_value(channel);
}

static int fsa7830_write_reg_val(const struct i2c_client *client,
			u8 reg, u8 val)
{
	int ret = 0;

	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0)
		hwlog_err("%s: i2c write error!!! ret=%d\n", __func__, ret);

	return ret;
}

static int fsa7830_read_reg_val(const struct i2c_client *client,
			u8 reg, u8 *val)
{
	int ret = -1;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		hwlog_err("%s: i2c read error!!! ret=%d\n", __func__, ret);
		return ret;
	}
	*val = (u8) ret;

	return 0;
}

/*
*   bit 7 sw1 ctl : 1 enable; 0 disable
*   bit 4~6 000~111 : V1~V8 to Vint
*   bit 3 sw2 ctl : 1 enable; 0: disable
*   bit 1~2 00~11 : Vint (1/3)Vint (2/3)Vint Reserved
*   bit 0 sw3 ctl : 1 enable; 0 disable
*/
static int fsa7830_swctl_write(struct fsa7830_data *fdata, u8 val)
{
	u8 reg = FSA7830_SWCTL;
	u8 buf = val;
	int ret = -1;

	ret = fsa7830_write_reg_val(fdata->client, reg, buf);
	if (ret < 0)
		hwlog_err("ret:%d switch control failed !\n", ret);

	return ret;
}

#if 0
static int fsa7830_irq_control(struct fsa7830_data *fdata, char val)
{
	u8 reg = 0x05;
	u8 buf = 0x00;
	int ret = -1;

	buf = val & 0x80;

	ret = fsa7830_write_reg_val(fdata->client, reg, buf);
	if (ret < 0)
		hwlog_err("ret:%d irq control failed !\n", ret);

	return ret;
}
#endif

static int fsa7830_enable(void *data, int flag)
{
	u8 reg = FSA7830_CONTROL;
	u8 buf = 0x00;
	int ret = 0;
	struct fsa7830_data *fdata = (struct fsa7830_data *)data;

	if (NULL == fdata) {
		hwlog_err("enable input null.\n");
		ret = -1;
		goto out;
	}
	if (flag) {
		buf = 0x80;
		ret = fsa7830_write_reg_val(fdata->client, reg, buf);
		if (ret < 0) {
			hwlog_err("ret:%d chip enable failed !\n", ret);
			goto out;
		}
		msleep(5);
		buf = 0xe0;
		ret = fsa7830_write_reg_val(fdata->client, reg, buf);
		if (ret < 0) {
			hwlog_err("ret:%d chip output failed !\n", ret);
			goto out;
		}
	} else {
		buf = 0x00;
		ret = fsa7830_write_reg_val(fdata->client, reg, buf);
		if (ret < 0) {
			hwlog_err("ret:%d chip disable failed !\n", ret);
			goto out;
		}
	}

out:
	return ret;
}

static int fsa7830_channel(void *data, struct hw_voltage_info *info)
{
	int ret = -1;
	int channel = 0;
	int input_cfg = 0;
	struct fsa7830_data *fdata = (struct fsa7830_data *)data;

	if (NULL == fdata || NULL == info) {
		hwlog_err("channel input null.\n");
		goto out;
	}

	channel = info->channel;
	input_cfg = fdata->chip_data.input_cfg;

	if (channel >= 0 && channel < 8) {
		if (input_cfg & (1 << channel)) {
			info->min_voltage =
				fdata->chip_data.min_threshold[channel];
			info->max_voltage =
				fdata->chip_data.max_threshold[channel];
			ret = 1;
		} else
			ret = 0;
	}

out:
	return ret;
}

static int fsa7830_getstate(void *data, int *state)
{
	u8 reg = FSA7830_CONTROL;
	u8 buf = 0x00;
	int ret = 0;
	struct fsa7830_data *fdata = (struct fsa7830_data *)data;

	if (NULL == fdata || NULL == state) {
		hwlog_err("get state input null.\n");
		ret = -1;
		goto out;
	}

	ret = fsa7830_read_reg_val(fdata->client, reg, &buf);
	if (ret < 0)
		goto out;

	if ((buf & 0xe0) == 0xe0)
		*state = 1;
	else
		*state = 0;

	ret = 0;

out:
	return ret;
}

static int fsa7830_getvoltage(void *data, struct hw_voltage_info *info)
{
	u8 buf = 0;
	int ret = 0;
	int channel = 0;
	int vol_value = 0;
	int adc_channel = -1;
	struct fsa7830_data *fdata = (struct fsa7830_data *)data;

	if (NULL == fdata || NULL == info) {
		ret = -1;
		goto out;
	}

	channel = info->channel;
	adc_channel = fdata->chip_data.adc_channel;

	buf = fdata->chip_data.sw_cfg[channel];
	ret = fsa7830_swctl_write(fdata, buf);
	if (ret < 0) {
		goto out;
	}
	udelay(200);

	vol_value = fsa7830_get_volt(adc_channel);
	info->voltage = vol_value;
	ret = 0;
out:
	return ret;
}

static int fsa7830_switch_enable(void *data, int enable)
{
	u8 buf = 0;
	int ret = 0;
	struct fsa7830_data *fdata = (struct fsa7830_data *)data;

	if (NULL == fdata) {
		hwlog_err("switch enable failed input null\n");
		ret = -1;
		goto out;
	}

	if (!enable) {
		ret = fsa7830_swctl_write(fdata, buf);
		if (ret < 0)
			hwlog_err("disable switch failed\n");
	} else
		hwlog_info("do nothing");

out:
	return ret;
}

#if 0
static void fsa7830_work_function(struct work_struct *work)
{
	char buf[2] = {0x00};
	int mode = 0;
	int ret = -1;

	struct fsa7830_data *data = container_of(work,
					struct fsa7830_data, work);

	switch (mode) {
	case FSA8730_ACTION_ENABLE:
		break;
	case FSA8730_ACTION_SWCFG:
		break;
	case FSA8730_ACTION_IRQ_ENABLE:
		break;
	case FSA8730_ACTION_OVP_VALUE:
		break;
	case FSA8730_ACTION_UNDEFINE:
		hwlog_err("err mode:FSA8730_ACTION_UNDEFINE\n");
		return;
	default:
		hwlog_err("err mode:%d\n", mode);
		return;
	}
}
#endif

static ssize_t fsa7830_adc_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int adc_volt;
	int adc_channel;
	struct fsa7830_data *data = dev_get_drvdata(dev);

	if (NULL == data) {
		hwlog_err("[%s] data null !\n", __func__);
		return -1;
	}
	adc_channel = data->chip_data.adc_channel;
	adc_volt = fsa7830_get_volt(adc_channel);

	hwlog_info("adc_channel:%d; adc_volt:%dmv\n", adc_channel, adc_volt);
	return snprintf(buf, 50, "%d\n", adc_volt);
}

static ssize_t fsa7830_adc_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	ssize_t ret;
	unsigned long state = 0;

	ret = kstrtol(buf, 16, &state);
	if (ret < 0)
		return ret;

	hwlog_info("[%s]; current_adc_channel:%d\n", __func__, (int)state);
	return size;
}

static ssize_t fsa7830_reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fsa7830_data *data = dev_get_drvdata(dev);

	return snprintf(buf, 50, "0x%x\n", data->reg_buf);
}

static ssize_t fsa7830_reg_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	u8 wbuf[2] = {0x00};
	int rw_flag = 0;
	int ret = 0;
	unsigned long state = 0;
	struct fsa7830_data *data = dev_get_drvdata(dev);

	if (NULL == data) {
		hwlog_err("[%s] data null !\n", __func__);
		return -1;
	}

	ret = kstrtol(buf, 16, &state);
	if (ret < 0)
		return ret;
	/*
		rw_flag	:	0:write	1:read
		wbuf[0] :	reg addr
		wbuf[1] :	val
	*/
	rw_flag = !!(state & 0x10000);
	wbuf[0] = (state>>8) & 0xff;
	wbuf[1] = state & 0xff;

	hwlog_info("rw_flag:%d; reg:0x%x; val:0x%x\n",
			rw_flag, wbuf[0], wbuf[1]);
	if (rw_flag) {
		ret = fsa7830_read_reg_val(data->client, wbuf[0], &wbuf[1]);
		if (ret < 0) {
			hwlog_err("[%s]read_reg ret:%d\n", __func__, ret);
			return -1;
		}
		data->reg_buf = wbuf[1];
		hwlog_info("reg:0x%x\n", wbuf[1]);
	} else {
		ret = fsa7830_write_reg_val(data->client, wbuf[0], wbuf[1]);
		if (ret < 0) {
			hwlog_err("[%s]ret:%d\n", __func__, ret);
			return -1;
		}
	}

	return size;
}
DEVICE_ATTR(getadc, 0660, fsa7830_adc_show, fsa7830_adc_store);
DEVICE_ATTR(reg, 0660, fsa7830_reg_show, fsa7830_reg_store);

static struct attribute *fsa7830_attributes[] = {
	&dev_attr_getadc.attr,
	&dev_attr_reg.attr,
	NULL,
};

static const struct attribute_group fsa7830_attr_group = {
	.attrs = fsa7830_attributes,
};

static int fsa7830_create_file(struct fsa7830_data *data)
{
	int ret = -1;

	if (NULL != fsa7830_attributes) {
		ret = sysfs_create_group(&data->client->dev.kobj,
			&fsa7830_attr_group);
	}

	return ret;
}

static void fsa7830_remove_file(struct fsa7830_data *data)
{
	if (NULL != fsa7830_attributes)
		sysfs_remove_group(&data->client->dev.kobj,
			&fsa7830_attr_group);
}

static int fsa7830_get_common_configs(int *chip_flag,
			struct fsa830_chip_data *chip_data,
			struct device_node *node)
{
	int ret = 0;
	int chip_types = 0;
	const char *state = NULL;

	if (NULL == chip_flag || NULL == chip_data || NULL == node) {
		hwlog_err("[%s] null ptr !\n", __func__);
		ret = -1;
		goto error;
	}

	ret = of_property_read_u32(node, FSA7830_HUAWEI_ADC,
			&chip_data->adc_channel);
	if (ret) {
		hwlog_err("Failed to get adc channel; ret:%d\n", ret);
		goto error;
	}

	ret = of_property_read_u32(node, FSA7830_HUAWEI_ID, &chip_data->id);
	if (ret) {
		hwlog_err("Failed to get id; ret:%d\n", ret);
		goto error;
	}

	ret = of_property_read_u32(node, FSA7830_HUAWEI_CHIP_FLAG,
			&chip_data->chip_flag);
	if (ret) {
		hwlog_err("Failed to get chip id reg; ret:%d\n", ret);
		goto error;
	}
	if (!(*chip_flag))
		*chip_flag = chip_data->chip_flag;
	else if (*chip_flag != chip_data->chip_flag) {
		hwlog_err("chip flag not err; flag local:%d; new:%d\n",
			*chip_flag, chip_data->chip_flag);
		ret = -1;
		goto error;
	}

	ret = of_property_read_u32(node, FSA7830_HUAWEI_ID_REG,
			&chip_data->id_reg);
	if (ret) {
		hwlog_err("Failed to get chip id reg; ret:%d\n", ret);
		goto error;
	}

	ret = of_property_read_u32(node, FSA7830_HUAWEI_INPUT_CFG,
			&chip_data->input_cfg);
	if (ret) {
		hwlog_err("Failed to get chip input cfg; ret:%d\n", ret);
		goto error;
	}

	ret = of_property_read_u32(node, FSA7830_HUAWEI_MAIN_CHANNEL,
			&chip_data->main_channel_count);
	if (ret) {
		hwlog_err("Failed to get chip id reg; ret:%d\n", ret);
		goto error;
	}

	ret = of_property_read_u32(node, FSA7830_HUAWEI_SEC_CHANNEL,
			&chip_data->sec_channel_count);
	if (ret) {
		hwlog_err("Failed to get chip sec channel count; ret:%d\n",
			ret);
		goto error;
	}

	ret = of_property_read_string(node, FSA7830_HUAWEI_INFO, &state);
	if (ret) {
		hwlog_err("Failed to get chip id reg; ret:%d\n", ret);
		goto error;
	}
	memset(chip_data->chip_info, 0, sizeof(chip_data->chip_info));
	strncpy(chip_data->chip_info, state, sizeof(chip_data->chip_info)-1);

	chip_types = of_property_count_u32_elems(node, FSA7830_HUAWEI_ID_VAL);
	if (chip_types < 0) {
		ret = chip_types;
		hwlog_err("Failed to get chip id types; ret:%d\n", ret);
		goto error;
	}

	if (chip_types > FSA7830_CHIP_TYPES) {
		hwlog_warn("chipe types err. chip_types:%d\n", chip_types);
		chip_types = FSA7830_CHIP_TYPES;
	}
	hwlog_info("chip types:%d\n", chip_types);
	chip_data->chip_type_counts = chip_types;
	ret = of_property_read_u32_array(node, FSA7830_HUAWEI_ID_VAL,
			chip_data->id_val, chip_types);
	if (ret) {
		hwlog_err("Failed to get chip id reg; ret:%d\n", ret);
		goto error;
	}

	ret = of_property_read_u32_array(node, FSA7830_HUAWEI_OVP_THRESHOLD,
			chip_data->ovp_threshold, FSA7830_CONFIG_COUNT);
	if (ret) {
		hwlog_err("Failed to get chip ovp threshold; ret:%d\n", ret);
		goto error;
	}

	ret = of_property_read_u32_array(node, FSA7830_HUAWEI_OVP_ACTION,
			chip_data->ovp_action, FSA7830_CONFIG_COUNT);
	if (ret) {
		hwlog_err("Failed to get ovp action; ret:%d\n", ret);
		goto error;
	}

	ret = of_property_read_u32_array(node, FSA7830_HUAWEI_MAX_THRESHOLD,
			chip_data->max_threshold, FSA7830_CONFIG_COUNT);
	if (ret) {
		hwlog_err("Failed to get max threshold; ret:%d\n", ret);
		goto error;
	}

	ret = of_property_read_u32_array(node, FSA7830_HUAWEI_MIN_THRESHOLD,
			chip_data->min_threshold, FSA7830_CONFIG_COUNT);
	if (ret) {
		hwlog_err("Failed to get min threshold; ret:%d\n", ret);
		goto error;
	}

	ret = of_property_read_u32_array(node, FSA7830_HUAWEI_SWITCH_CFG,
			chip_data->sw_cfg, FSA7830_CONFIG_COUNT);
	if (ret) {
		hwlog_err("Failed to get chip witch cfg; ret:%d\n", ret);
		goto error;
	}
error:
	return ret;
}

static int fsa7830_detect(struct fsa7830_data *data)
{
	u8 buf = 0x00;
	u8 val = 0x00;
	int i = 0;
	int ret = -1;
	int chip_types = 0;

	if (NULL == data) {
		hwlog_err("[%s] data null\n", __func__);
		goto out;
	}

	chip_types = data->chip_data.chip_type_counts;

	ret = fsa7830_read_reg_val(data->client,
			data->chip_data.id_reg, &buf);
	if (ret < 0) {
		hwlog_err("[%s] err! ret:%d\n", __func__, ret);
		goto out;
	}

	if (chip_types > FSA7830_CHIP_TYPES) {
		hwlog_warn("chip type over. chip_types:%d\n", chip_types);
		chip_types = FSA7830_CHIP_TYPES;
	}

	ret = -1;
	for (i = 0; i < chip_types; i++) {
		val = (u8)data->chip_data.id_val[i];
		hwlog_info("val:%2x; buf:%2x\n", val, buf);
		if (val == buf) {
			ret = 0;
			break;
		}
	}

out:
	return ret;
}

static int fsa7830_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret = -1;
	int fsa7830_exist_gpio = 0;
	static int gpio_temp = 0, gpio_status = 0;
	struct fsa7830_data *data;
	struct device_node *node;

	hwlog_info("[%s] gpio_temp:%d, gpio_status:%d!\n", __func__,gpio_temp,gpio_status);
	if(fsa7830_no_exist_flag == 1){
		hwlog_info("[%s] fsa7830 chip no exist!\n", __func__);
		return 0;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		hwlog_err("[%s] i2c check err !\n", __func__);
		ret = -EIO;
		goto error;
	}

	node = client->dev.of_node;
	if (NULL == node) {
		hwlog_err("[%s] node null !\n", __func__);
		ret = -ENODEV;
		goto error;
	}

	data = (struct fsa7830_data *)devm_kzalloc(&client->dev,
				sizeof(struct fsa7830_data), GFP_KERNEL);
	if (!data) {
		hwlog_err("[%s] devm_kzalloc err !\n", __func__);
		ret = -ENOMEM;
		goto error;
	}
	data->client = client;

	ret = fsa7830_get_common_configs(&fsa7830_chip_flag,
				&data->chip_data, node);
	if (ret) {
		hwlog_err("[%s] get dts config err! ret:%d\n", __func__, ret);
		goto error;
	}

	ret = snprintf(data->dev_name, sizeof(data->dev_name), "vol_detect%d",
				data->chip_data.id);
	if (ret < 0) {
		hwlog_err("[%s]set devive name err! ret:%d\n", __func__, ret);
		goto error;
	}

	/*compatible no fsa7830 chip in the mass-produced*/
	fsa7830_exist_gpio = of_get_named_gpio(node, "huawei,fsa7830_exist_gpio", 0);
	hwlog_info("[%s] fsa7830 exist gpio:%d!\n", __func__,fsa7830_exist_gpio);
	if (!gpio_is_valid(fsa7830_exist_gpio)) {
		hwlog_err("failed to get firm!\n");
	}else{
		if(fsa7830_exist_gpio != gpio_temp){
			/*config the gpio to input-PU*/
			data->pctrl = devm_pinctrl_get(&client->dev);
			if (IS_ERR(data->pctrl)) {
				data->pctrl = NULL;
				hwlog_err("failed to get clk pin!\n");
				return -ENODEV;
			}

			data->pins_default = pinctrl_lookup_state(data->pctrl, "default");
			data->pins_idle = pinctrl_lookup_state(data->pctrl, "idle");
			ret = pinctrl_select_state(data->pctrl, data->pins_default);
			if (ret < 0) {
				hwlog_err("%s: unapply new state!\n", __func__);
				return -ENODEV;
			}

			if (gpio_request(fsa7830_exist_gpio, "fsa7830-exist") < 0) {
				hwlog_err("[%s] error requesting fsa7830-exist gpio!\n", __func__);
				goto error;
			}
			ret = gpio_direction_input(fsa7830_exist_gpio);
			gpio_status = gpio_get_value(fsa7830_exist_gpio);
			gpio_temp = fsa7830_exist_gpio;
		}

		if(gpio_status == 0){
			fsa7830_no_exist_flag = 1;
			hwlog_info("[%s] fsa7830 chip no exist!\n", __func__);
			goto fsa7830_no_exist;
		}else if(gpio_status == 1){
			fsa7830_no_exist_flag = 0;
			hwlog_info("[%s] fsa7830 chip exist!\n", __func__);
		}
	}
	/*compatible no fsa7830 chip in the mass-produced*/

	ret = fsa7830_detect(data);
	if (ret)
		goto error;

/*
	INIT_WORK(&data->work, fsa7830_work_function);
*/

	ret = fsa7830_create_file(data);
	if (ret) {
		hwlog_err("[%s] create file err !\n", __func__);
		goto error;
	}

fsa7830_no_exist:
	data->vol_data = hw_voltage_register(data, data->chip_data.id);
	if (NULL == data->vol_data) {
		hwlog_err("voltage register err. id:%d\n", data->chip_data.id);
		goto error_file;
	}
	data->vol_data->hw_voltage_chennel = fsa7830_channel;
	data->vol_data->hw_voltage_enable = fsa7830_enable;
	data->vol_data->hw_voltage_getstate = fsa7830_getstate;
	data->vol_data->hw_voltage_getvoltage = fsa7830_getvoltage;
	data->vol_data->hw_voltage_switch_enable = fsa7830_switch_enable;

	i2c_set_clientdata(client, data);

#if 0
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	fsa7830_chip_flag &= ~(1<<(data->chip_data.id));
	if (!fsa7830_chip_flag)
		set_hw_dev_flag(DEV_I2C_VOLTAGE_DETECTION);
#endif
#endif

	hwlog_info("[%s] fsa7830[%d] probe success !\n", __func__,
					data->chip_data.id);

	return 0;

error_file:
	fsa7830_remove_file(data);
error:
	return ret;
}

static int fsa7830_remove(struct i2c_client *client)
{
	struct fsa7830_data *data = i2c_get_clientdata(client);

	if (data)
		fsa7830_remove_file(data);

	return 0;
}

static const struct i2c_device_id fsa7830_id_table[] = {
		{"fsa7830", 0},
		{ },
};
MODULE_DEVICE_TABLE(i2c, fsa7830_id_table);

static const struct of_device_id fsa7830_of_id_table[] = {
		{.compatible = HUAWEI_VOLTAGE_COMPATIBLE},
		{ },
};

static struct i2c_driver fsa7830_i2c_driver = {
		.driver = {
			.name = "fsa7830",
			.owner = THIS_MODULE,
			.of_match_table = fsa7830_of_id_table,
		},
		.probe = fsa7830_probe,
		.remove = fsa7830_remove,
		.id_table = fsa7830_id_table,
};

module_i2c_driver(fsa7830_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huawei Corp.");
MODULE_DESCRIPTION("Driver for fsa7830");
