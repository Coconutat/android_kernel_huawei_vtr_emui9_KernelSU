#include "ilitek_ts.h"
#include "ilitek_dts.h"
#if defined (CONFIG_HUAWEI_DSM)
extern struct dsm_client *ts_dclient;
#endif

u8 TOUCH_DRIVER_DEBUG_LOG_LEVEL = CONFIG_TOUCH_DRIVER_DEFAULT_LOG_LEVEL;
static DEFINE_MUTEX(ts_power_gpio_sem);
static int ts_power_gpio_ref = 1;

extern int ilitek_upgrade_firmware(void);
#ifdef TOOL
struct dev_data {
	dev_t devno;
	struct cdev cdev;
	struct class *class;
};
extern struct dev_data dev_ilitek;
extern struct proc_dir_entry * proc_ilitek;
extern int create_tool_node(void);
extern int remove_tool_node(void);
#endif

extern int ilitek_into_glove_mode(bool status);
extern int ilitek_into_fingersense_mode(bool status);
extern int ilitek_into_hall_halfmode(bool status);

 int driver_information[] = {DERVER_VERSION_MAJOR,DERVER_VERSION_MINOR,CUSTOMER_ID,MODULE_ID,PLATFORM_ID,PLATFORM_MODULE,ENGINEER_ID};
 char Report_Flag;
 volatile char int_Flag;
 volatile char update_Flag;
 int update_timeout;
 char EXCHANG_XY = 0;
 char REVERT_X = 0;
 char REVERT_Y = 0;
 char DBG_FLAG,DBG_COR;

#define MAX_X 1079
#define MAX_Y 1919
#define MAX_TOUCH_POINT 10
#define JUDGE_ZERO 0

struct i2c_data * ilitek_data = NULL;
/*
 * functions body
 */
 static int ilitek_irq_top_half(struct ts_cmd_node *cmd)
{
	cmd->command = TS_INT_PROCESS;
	return NO_ERR;
}

static int ilitek_chip_get_info(struct ts_chip_info_param *info)
{
	int ret = NO_ERR;
	size_t ic_vendor_size = 0;
	size_t fw_vendor_size = 0;
	unsigned char buf[4] = {0};
	int write_len = 0;
	int read_len = 0;
	char comp_name[ILITEK_VENDOR_COMP_NAME_LEN] = {0};
	const char *producer = NULL;
	//char project_id[ILITEK_PROJECT_ID_LEN] = "ws29080";
	char * project_id = ilitek_data->product_id;
	struct device_node *np = NULL;
	ret = ilitek_get_vendor_compatible_name(project_id, comp_name,ILITEK_VENDOR_COMP_NAME_LEN);
	if (ret) {
		TS_LOG_ERR("%s:get compatible name fail, ret=%d\n",__func__, ret);
		return ret;
	}
	TS_LOG_INFO("ilitek comp_name = %s\n", comp_name);
	np = of_find_compatible_node(NULL, NULL, comp_name);
	if (!np) {
		TS_LOG_ERR("%s:find dev node faile, compatible name:%s\n", __func__, comp_name);
		return -ENODEV;
	}
	ret = of_property_read_string(np, "producer", &producer);
	if (ret){
		TS_LOG_ERR("%s:find producer in dts fail, ret=%d\n", __func__, ret);
		return ret;
	}
	buf[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
	buf[1] = ILITEK_TP_CMD_GET_FIRMWARE_VERSION;
	write_len = I2C_WRITE_TWO_LENGTH_DATA;
	read_len = I2C_READ_ZERO_LENGTH_DATA;
	if(ilitek_i2c_write_and_read( buf, write_len, ILITEK_WRITE_READ_DELAY, buf, read_len) < 0)
	{
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return TRANSMIT_ERROR;
	}
	buf[0] = ILITEK_TP_CMD_GET_FIRMWARE_VERSION;
	write_len = I2C_WRITE_ONE_LENGTH_DATA; 
	read_len = I2C_READ_THREE_LENGTH_DATA; 
	if(ilitek_i2c_write_and_read( buf, write_len, ILITEK_WRITE_READ_NOT_DELAY, ilitek_data->firmware_ver, read_len) < 0)
	{
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return TRANSMIT_ERROR;
	}
	TS_LOG_INFO("%s, firmware   version:%d.%d.%d\n", __func__, ilitek_data->firmware_ver[0], ilitek_data->firmware_ver[1], ilitek_data->firmware_ver[2]);
	TS_LOG_INFO("ilitek producer = %s\n", producer);
	ic_vendor_size = CHIP_INFO_LENGTH * 2;
	strncpy(info->ic_vendor, ILITEK_CHIP_NAME, ic_vendor_size);
	ilitek_strncat(info->ic_vendor, "-", ic_vendor_size);
	ilitek_strncat(info->ic_vendor,ilitek_data->product_id, ic_vendor_size);
	strncpy(info->mod_vendor, producer, CHIP_INFO_LENGTH);
	fw_vendor_size = CHIP_INFO_LENGTH * 2;
	snprintf(info->fw_vendor, fw_vendor_size, "%d.%d.%d", ilitek_data->firmware_ver[0], ilitek_data->firmware_ver[1], ilitek_data->firmware_ver[2]);
	return NO_ERR;
}

int ilitek_chip_get_capacitance_test_type(struct ts_test_type_info *info)
{
	int ret = 0;
	struct ts_kit_device_data *dev_data = NULL;
	dev_data = ilitek_data->ilitek_chip_data;
	if (!info) {
		TS_LOG_ERR("%s:info is Null\n", __func__);
		return -ENOMEM;
	}

	switch (info->op_action) {
	case TS_ACTION_READ:
		memcpy(info->tp_test_type,dev_data->tp_test_type, TS_CAP_TEST_TYPE_LEN);
		TS_LOG_INFO("%s:test_type=%s\n", __func__, info->tp_test_type);
		break;
	case TS_ACTION_WRITE:
		break;
	default:
		TS_LOG_ERR("%s:invalid op action:%d\n",__func__, info->op_action);
		ret = -EINVAL;
		break;
	}
	return ret;
}


int ilitek_reset(int delay)
{
	int ret = 0;
	int reset_gpio = 0;
	reset_gpio = ilitek_data->ilitek_chip_data->ts_platform_data->reset_gpio;
	ret = gpio_direction_output(reset_gpio, 1);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 1 fail, ret=%d\n", __func__, ret);
		return ret;
	}
	msleep(1);
	ret = gpio_direction_output(reset_gpio, 0);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 0 fail, ret=%d\n", __func__, ret);
		return ret;
	}
	mdelay(5);
	ret = gpio_direction_output(reset_gpio, 1);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 1 fail, ret=%d\n", __func__, ret);
		return ret;
	}
	TS_LOG_INFO("%s:reset  delay:%d\n", __func__, delay);
	mdelay(delay);
	return 0;
}
static int ilitek_reset_device(void)
{
	return ilitek_reset(ILITEK_RESET_MODEL_NORMAL_DELAY);
}
int ilitek_i2c_write(u8 *values, u16 values_size)
{
	int ret = 0;
	struct ts_bus_info *bops = NULL;
	bops = ilitek_data->ilitek_chip_data->ts_platform_data->bops;
	ret = bops->bus_write(values, values_size);
	if (ret) {
		TS_LOG_ERR("%s:fail, addrs: 0x%x", __func__, ilitek_data->ilitek_chip_data->slave_addr);
	}
	return ret;
}

int ilitek_i2c_read(u8 *addrs, u16 addr_size, u8 *values, u16 values_size)
{
	int ret = 0;
	struct ts_bus_info *bops = NULL;
	bops = ilitek_data->ilitek_chip_data->ts_platform_data->bops;
	ret = bops->bus_read(addrs, addr_size, values, values_size);
	if (ret) {
		TS_LOG_ERR("%s:fail, addrs: 0x%x", __func__, ilitek_data->ilitek_chip_data->slave_addr);
	}
	return ret;
}



int ilitek_i2c_write_and_read(uint8_t *cmd,
		int write_len, int delay, uint8_t *data, int read_len)
{
	int ret = 0;
	int write_reg_len = 0;
	if (read_len == 0) {
		if (write_len > 0) {
			ret = ilitek_i2c_write(cmd, write_len);
			if(ret < 0)
			{
				TS_LOG_ERR("%s, i2c write error, ret = %d\n", __func__, ret);
			}
		}
		if(delay > 1)
			msleep(delay);
	}
	else if (write_len == 0) {
		if(read_len > 0){
			ret = ilitek_i2c_read(cmd, write_reg_len, data, read_len);
			if(ret < 0)
			{
				TS_LOG_ERR("%s, i2c read error, ret = %d\n", __func__, ret);
			}
		}
	}
	else if (delay > 0) {
		if (write_len > 0) {
			ret = ilitek_i2c_write(cmd, write_len);
			if(ret < 0)
			{
				TS_LOG_ERR("%s, i2c write error, ret = %d\n", __func__, ret);
			}
		}
		if(delay > 0)
			mdelay(delay);
		if(read_len > 0){
			ret =  ilitek_i2c_read(cmd, write_reg_len, data, read_len);
			if(ret < 0)
			{
				TS_LOG_ERR("%s, i2c read error, ret = %d\n", __func__, ret);
			}
		}
	}
	else {
		ret =  ilitek_i2c_read(cmd, write_len, data, read_len);
		if(ret < 0)
		{
			TS_LOG_ERR("%s, i2c write error, ret = %d\n", __func__, ret);
		}
	}
	return ret;
}

static int i2c_communicate_check(struct ts_kit_platform_data *pdata)
{
	int i = 0;
	int ret = NO_ERR;
	u8 cmd = 0;
	u8 reg_val[3]= {0};
	int write_reg_len = 1;
	int read_len = 3;
	pdata->client->addr = ilitek_data->ilitek_chip_data->slave_addr;
	cmd = ILITEK_TP_CMD_READ_DATA;
	for (i = 0; i < ILITEK_DETECT_I2C_RETRY_TIMES; i++) {
		ret = ilitek_i2c_read(&cmd, write_reg_len, reg_val, read_len);
		if (ret < 0) {
			TS_LOG_ERR("%s:ilitek_i2c_read fail, ret=%d, i=%d\n",__func__, ret, i);
			msleep(50);
		} else {
			TS_LOG_INFO("%s:ilitek_i2c_read success, :0x%x, 0x%x, 0x%x\n",__func__,
				(unsigned int)reg_val[0], (unsigned int)reg_val[1], (unsigned int)reg_val[2]);
			return NO_ERR;
		}
	}
	return ret;
}


/*******************************************************************************
** TP VCC
* TP VCC/VDD  power control by GPIO in V8Rx,
* if controled by ldo in other products, open "return -EINVAL"
ilitek_data->vdd is 3.1V ,ilitek_data->vcc_i2c is 1.8V
*/
static int ilitek_regulator_get(void)
{
	if (ilitek_data->ilitek_chip_data->ts_platform_data->fpga_flag == 1)
		return 0;
	if (1 == ilitek_data->ilitek_chip_data->vci_regulator_type) {
		ilitek_data->vdd = regulator_get(&ilitek_data->ilitek_dev->dev, ILITEK_VDD);
		if (IS_ERR(ilitek_data->vdd)) {
			TS_LOG_ERR("regulator tp vci not used\n");
			return -EINVAL;
		}
	}
	if (1 == ilitek_data->ilitek_chip_data->vddio_regulator_type) {
		ilitek_data->vcc_i2c = regulator_get(&ilitek_data->ilitek_dev->dev, ILITEK_VBUS);
		if (IS_ERR(ilitek_data->vcc_i2c)) {
			TS_LOG_ERR("regulator tp vddio not used\n");
			regulator_put(ilitek_data->vcc_i2c);
			return -EINVAL;
		}
	}
	return 0;
}
static void ts_kit_power_gpio_enable(void)
{
	if (ilitek_data== NULL) {
		TS_LOG_ERR("ilitek_data == NULL\n");
		return;
	}

	if (ilitek_data->ilitek_chip_data == NULL) {
		TS_LOG_ERR("ilitek_chip_data == NULL\n");
		return;
	}

	mutex_lock(&ts_power_gpio_sem);
	if (ts_power_gpio_ref == 0) {
		gpio_direction_output(ilitek_data->ilitek_chip_data->vddio_gpio_ctrl, 1);
	}
	ts_power_gpio_ref++;
	TS_LOG_INFO("ts_power_gpio_ref++ = %d\n", ts_power_gpio_ref);
	mutex_unlock(&ts_power_gpio_sem);
}


static void ts_kit_power_gpio_disable(void)
{
	if (ilitek_data == NULL) {
		TS_LOG_ERR("ilitek_data == NULL\n");
		return;
	}

	if (ilitek_data->ilitek_chip_data == NULL) {
		TS_LOG_ERR("ilitek_chip_data == NULL\n");
		return;
	}

	mutex_lock(&ts_power_gpio_sem);
	if (ts_power_gpio_ref == 1) {
		gpio_direction_output(ilitek_data->ilitek_chip_data->vddio_gpio_ctrl, 0);
	}
	if (ts_power_gpio_ref > 0) {
		ts_power_gpio_ref--;
	}
	TS_LOG_INFO("ts_power_gpio_ref-- = %d\n", ts_power_gpio_ref);
	mutex_unlock(&ts_power_gpio_sem);
}

static int ilitek_gpio_request(void)
{
	int retval = NO_ERR;
	TS_LOG_INFO("ilitek_gpio_request\n");

	if ((1 == ilitek_data->ilitek_chip_data->vci_gpio_type)
	    && (1 == ilitek_data->ilitek_chip_data->vddio_gpio_type)) {
		if (ilitek_data->ilitek_chip_data->vci_gpio_ctrl ==
		    ilitek_data->ilitek_chip_data->vddio_gpio_ctrl) {
			retval = gpio_request(ilitek_data->ilitek_chip_data->vci_gpio_ctrl, "ts_vci_gpio");
			if (retval) {
				TS_LOG_ERR("SFT:Ok;  ASIC: Real ERR----unable to request vci_gpio_ctrl firset:%d\n",
				     ilitek_data->ilitek_chip_data->vci_gpio_ctrl);
				goto ts_vci_out;
			}
		} else {
			retval = gpio_request(ilitek_data->ilitek_chip_data->vci_gpio_ctrl, "ts_vci_gpio");
			if (retval) {
				TS_LOG_ERR
				    ("SFT:Ok;  ASIC: Real ERR----unable to request vci_gpio_ctrl2:%d\n",
				     ilitek_data->ilitek_chip_data->vci_gpio_ctrl);
				goto ts_vci_out;
			}
			retval = gpio_request(ilitek_data->ilitek_chip_data->vddio_gpio_ctrl, "ts_vddio_gpio");
			if (retval) {
				TS_LOG_ERR("SFT:Ok;  ASIC: Real ERR----unable to request vddio_gpio_ctrl:%d\n",
				     ilitek_data->ilitek_chip_data->vddio_gpio_ctrl);
				goto ts_vddio_out;
			}
		}
	} else {
		if (1 == ilitek_data->ilitek_chip_data->vci_gpio_type) {
			retval = gpio_request(ilitek_data->ilitek_chip_data->vci_gpio_ctrl, "ts_vci_gpio");
			if (retval) {
				TS_LOG_ERR("SFT:Ok;  ASIC: Real ERR----unable to request vci_gpio_ctrl2:%d\n",
				     ilitek_data->ilitek_chip_data->vci_gpio_ctrl);
				goto ts_vci_out;
			}
		}
		if (1 == ilitek_data->ilitek_chip_data->vddio_gpio_type) {
			retval = gpio_request(ilitek_data->ilitek_chip_data->vddio_gpio_ctrl, "ts_vddio_gpio");
			if (retval) {
				TS_LOG_ERR("SFT:Ok;  ASIC: Real ERR----unable to request vddio_gpio_ctrl:%d\n",
				     ilitek_data->ilitek_chip_data->vddio_gpio_ctrl);
				goto ts_vddio_out;
			}
		}
	}

	TS_LOG_INFO("reset:%d, irq:%d,\n", ilitek_data->ilitek_chip_data->ts_platform_data->reset_gpio,
		    ilitek_data->ilitek_chip_data->ts_platform_data->irq_gpio);

	goto ts_reset_out;
ts_vddio_out:
	gpio_free(ilitek_data->ilitek_chip_data->vddio_gpio_ctrl);
ts_vci_out:
	gpio_free(ilitek_data->ilitek_chip_data->vci_gpio_ctrl);
ts_reset_out:
	return retval;
}
static int ilitek_vci_enable(void)
{
	int retval;
	int vol_vlaue;
	if (ilitek_data->ilitek_chip_data->ts_platform_data->fpga_flag == 1)
		return 0 ;
	if (IS_ERR(ilitek_data->vdd)) {
		TS_LOG_ERR("tp_vci is err\n");
		return -EINVAL;
	}
	vol_vlaue = ilitek_data->ilitek_chip_data->regulator_ctr.vci_value;
	{
		TS_LOG_INFO("set vci voltage to %d\n", vol_vlaue);
		retval = regulator_set_voltage(ilitek_data->vdd, vol_vlaue,vol_vlaue);
		if (retval < 0) {
			TS_LOG_ERR("failed to set voltage regulator tp_vci error: %d\n",retval);
			return -EINVAL;
		}
	}
	retval = regulator_enable(ilitek_data->vdd);
	if (retval < 0) {
		TS_LOG_ERR("failed to enable regulator tp_vci\n");
		return -EINVAL;
	}
	return 0;
}

static int ilitek_vci_disable(void)
{
	int retval;
	if (ilitek_data->ilitek_chip_data->ts_platform_data->fpga_flag == 1)
		return 0;
	if (IS_ERR(ilitek_data->vdd)) {
		TS_LOG_ERR("tp_vci is err\n");
		return -EINVAL;
	}
	retval = regulator_disable(ilitek_data->vdd);
	if (retval < 0) {
		TS_LOG_ERR("failed to disable regulator tp_vci\n");
		return -EINVAL;
	}
	return 0;
}

static int ilitek_vddio_enable(void)
{
	int retval;
	int vddio_value;

	if (ilitek_data->ilitek_chip_data->ts_platform_data->fpga_flag == 1)
		return 0 ;

	if (IS_ERR(ilitek_data->vcc_i2c)) {
		TS_LOG_ERR("tp_vddio is err\n");
		return -EINVAL;
	}

	vddio_value = ilitek_data->ilitek_chip_data->regulator_ctr.vddio_value;
	if (ilitek_data->ilitek_chip_data->regulator_ctr.need_set_vddio_value) {
		TS_LOG_INFO("set tp_vddio voltage to %d\n", vddio_value);
		retval = regulator_set_voltage(ilitek_data->vcc_i2c, vddio_value,vddio_value);
		if (retval < 0) {
			TS_LOG_ERR("failed to set voltage regulator tp_vddio error: %d\n",retval);
			return -EINVAL;
		}
	}

	retval = regulator_enable(ilitek_data->vcc_i2c);
	if (retval < 0) {
		TS_LOG_ERR("failed to enable regulator tp_vddio\n");
		return -EINVAL;
	}
	return 0;
}

static int ilitek_vddio_disable(void)
{
	int retval;

	if (ilitek_data->ilitek_chip_data->ts_platform_data->fpga_flag == 1)
		return 0;
	if (IS_ERR(ilitek_data->vcc_i2c)) {
		TS_LOG_ERR("tp_vddio is err\n");
		return -EINVAL;
	}

	retval = regulator_disable(ilitek_data->vcc_i2c);
	if (retval < 0) {
		TS_LOG_ERR("failed to disable regulator tp_vddio\n");
		return -EINVAL;
	}
	return 0;
}

static void ilitek_vci_on(void)
{
	TS_LOG_INFO("%s vci enable\n", __func__);
	if (1 == ilitek_data->ilitek_chip_data->vci_regulator_type) {
		if (!IS_ERR(ilitek_data->vdd)) {
			TS_LOG_INFO("vci enable is called\n");
			ilitek_vci_enable();
		}
	}

	if (ilitek_data->ilitek_chip_data->vci_gpio_type) {
		TS_LOG_INFO("%s vci switch gpio on\n", __func__);
		gpio_direction_output(ilitek_data->ilitek_chip_data->vci_gpio_ctrl, 1);
	}
}

/* dts */
static int ilitek_pinctrl_get_init(void)
{
	int ret = 0;

	if (ilitek_data->ilitek_chip_data->ts_platform_data->fpga_flag == 1)
		return 0;

	 ilitek_data->pctrl = devm_pinctrl_get(&ilitek_data->ilitek_dev->dev);
	if (IS_ERR(ilitek_data->pctrl)) {
		TS_LOG_ERR("failed to devm pinctrl get\n");
		ret = -EINVAL;
		return ret;
	}

	ilitek_data->pins_default = pinctrl_lookup_state(ilitek_data->pctrl, "default");
	if (IS_ERR(ilitek_data->pins_default)) {
		TS_LOG_ERR("failed to pinctrl lookup state default\n");
		ret = -EINVAL;
		goto err_pinctrl_put;
	}

	ilitek_data->pins_idle = pinctrl_lookup_state(ilitek_data->pctrl, "idle");
	if (IS_ERR(ilitek_data->pins_idle)) {
		TS_LOG_ERR("failed to pinctrl lookup state idle\n");
		ret = -EINVAL;
		goto err_pinctrl_put;
	}
	return 0;

err_pinctrl_put:
	devm_pinctrl_put(ilitek_data->pctrl);
	return ret;
}

static int ilitek_pinctrl_select_normal(void)
{
	int retval = NO_ERR;

	if (ilitek_data->ilitek_chip_data->ts_platform_data->fpga_flag == 1)
		return 0;

	retval = pinctrl_select_state(ilitek_data->pctrl, ilitek_data->pins_default);
	if (retval < 0) {
		TS_LOG_ERR("set iomux normal error, %d\n", retval);
	}
	return retval;
}

static int ilitek_pinctrl_select_lowpower(void)
{
	int retval = NO_ERR;
	if (ilitek_data->ilitek_chip_data->ts_platform_data->fpga_flag == 1)
		return 0;

	retval = pinctrl_select_state(ilitek_data->pctrl, ilitek_data->pins_idle);
	if (retval < 0) {
		TS_LOG_ERR("set iomux lowpower error, %d\n", retval);
	}
	return retval;
}

static void ilitek_vddio_on(void)
{
	TS_LOG_INFO("%s vddio enable\n", __func__);
	if (1 == ilitek_data->ilitek_chip_data->vddio_regulator_type) {
		if (!IS_ERR(ilitek_data->vcc_i2c)) {
			TS_LOG_INFO("vddio enable is called\n");
			ilitek_vddio_enable();
		}
	}
	if (ilitek_data->ilitek_chip_data->vddio_gpio_type) {
		TS_LOG_INFO("%s vddio switch gpio on\n", __func__);
		ts_kit_power_gpio_enable();
	}

}
static void ilitek_power_on_gpio_set(void)
{
	ilitek_pinctrl_select_normal();
	gpio_direction_input(ilitek_data->ilitek_chip_data->ts_platform_data->irq_gpio);
	gpio_direction_output(ilitek_data->ilitek_chip_data->ts_platform_data->reset_gpio, 1);
}

static void ilitek_power_on(void)
{
	TS_LOG_INFO("ilitek_power_on called\n");
	ilitek_vci_on();
	mdelay(1);
	ilitek_vddio_on();
	mdelay(1);
	ilitek_power_on_gpio_set();
}

static void  ilitek_power_off_gpio_set(void)
{
	TS_LOG_INFO("ilitek suspend RST out L\n");
	gpio_direction_output(ilitek_data->ilitek_chip_data->ts_platform_data->reset_gpio, 0);
	 ilitek_pinctrl_select_lowpower();
	//gpio_direction_input(ilitek_data->ilitek_chip_data->ts_platform_data->reset_gpio);
	mdelay(1);
}

static void ilitek_vddio_off(void)
{
	if (1 == ilitek_data->ilitek_chip_data->vddio_regulator_type) {
		if (!IS_ERR(ilitek_data->vcc_i2c)) {
			ilitek_vddio_disable();
		}
	}

	if (ilitek_data->ilitek_chip_data->vddio_gpio_type) {
		TS_LOG_INFO("%s vddio switch gpio off\n", __func__);
		ts_kit_power_gpio_disable();
	}
}

static void ilitek_vci_off(void)
{
	if (1 == ilitek_data->ilitek_chip_data->vci_regulator_type) {
		if (!IS_ERR(ilitek_data->vdd)) {
			ilitek_vci_disable();
		}
	}

	if (ilitek_data->ilitek_chip_data->vci_gpio_type) {
		TS_LOG_INFO("%s vci switch gpio off\n", __func__);
		gpio_direction_output(ilitek_data->ilitek_chip_data->vci_gpio_ctrl, 0);
	}
}

static void ilitek_power_off(void)
{
	TS_LOG_INFO("ilitek_power_off called\n");
	ilitek_power_off_gpio_set();
	ilitek_vddio_off();
	mdelay(1);
	ilitek_vci_off();
	mdelay(1);
}

static void ilitek_regulator_put(void)
{
	if (ilitek_data->ilitek_chip_data->ts_platform_data->fpga_flag == 1)
		return;

	if (1 == ilitek_data->ilitek_chip_data->vci_regulator_type) {
		if (!IS_ERR(ilitek_data->vdd)) {
			regulator_put(ilitek_data->vdd);
		}
	}
	if (1 == ilitek_data->ilitek_chip_data->vddio_regulator_type) {
		if (!IS_ERR(ilitek_data->vcc_i2c)) {
			regulator_put(ilitek_data->vcc_i2c);
		}
	}
}

static void ilitek_gpio_free(void)
{
	TS_LOG_INFO("ilitek_gpio_free\n");

	/*0 is power supplied by gpio, 1 is power supplied by ldo */
	if (1 == ilitek_data->ilitek_chip_data->vci_gpio_type) {
		if (ilitek_data->ilitek_chip_data->vci_gpio_ctrl)
			gpio_free(ilitek_data->ilitek_chip_data->vci_gpio_ctrl);
	}
	if (1 == ilitek_data->ilitek_chip_data->vddio_gpio_type) {
		if (ilitek_data->ilitek_chip_data->vddio_gpio_ctrl)
			gpio_free(ilitek_data->ilitek_chip_data->vddio_gpio_ctrl);
	}
}

static int ilitek_chip_detect(struct ts_kit_platform_data *pdata)
{
	int ret = NO_ERR;
	if (!pdata) {
		TS_LOG_ERR("%s:device, chip_data  is NULL\n", __func__);
		ret = -ENOMEM;
		goto out;
	}
	ilitek_data->ilitek_chip_data->ts_platform_data = pdata;
	ilitek_data->ilitek_dev = pdata->ts_dev;
	ilitek_data->ilitek_dev->dev.of_node = ilitek_data->ilitek_chip_data->cnode;
	/*setting the default data*/
	ilitek_data->ilitek_chip_data->is_i2c_one_byte = false;
	ilitek_data->ilitek_chip_data->is_new_oem_structure= false;
	TS_LOG_INFO("%scnode.name=%s\n", __func__,ilitek_data->ilitek_dev->dev.of_node->name);
	ilitek_parse_dts(ilitek_data->ilitek_dev->dev.of_node,ilitek_data->ilitek_chip_data);
	ret = ilitek_regulator_get();
	if (ret < 0) {
            TS_LOG_ERR("ilitek_regulator_get error %d \n",ret);
		goto out;
	}
	ret = ilitek_gpio_request();
	if (ret < 0) {
            TS_LOG_ERR("ilitek_gpio_request error %d \n",ret);
	      goto gpio_err;
	}
	ret = ilitek_pinctrl_get_init();
	if (ret < 0) {
            TS_LOG_ERR("ilitek_gpio_request error %d \n",ret);
	     goto pinctrl_get_err;
	}

	ilitek_power_on();

	ret = ilitek_reset(ILITEK_RESET_MODEL_CHECKFW_DELAY);
	if (ret) {
		TS_LOG_ERR("%s:hardware reset fail, ret=%d\n",__func__, ret);
		return ret;
	}
	ret = i2c_communicate_check(pdata);
	if (ret < 0) {
		TS_LOG_ERR("%s:not find ilitek device, ret=%d\n", __func__, ret);
		goto check_err;
	} else {
		TS_LOG_INFO("%s:find ilitek device\n", __func__);
		strncpy(ilitek_data->ilitek_chip_data->chip_name, ILITEK_CHIP_NAME, MAX_STR_LEN);
	}

	TS_LOG_INFO("%s:ilitek chip detect success\n", __func__);

	return NO_ERR;

check_err:
	ilitek_power_off();
pinctrl_get_err:
	ilitek_gpio_free();
gpio_err:
	ilitek_regulator_put();
out:
	TS_LOG_ERR("detect ilitek error\n");

	return ret;
}


#ifdef DEBUG_NETLINK
static struct sock *netlink_sock;
bool debug_flag = false;
static void udp_reply(int pid,int seq,void *payload,int size)
{
	struct sk_buff	*skb;
	struct nlmsghdr	*nlh;
	int		len = NLMSG_SPACE(size);
	void		*data;
	int ret;
	skb = alloc_skb(len, GFP_ATOMIC);
	if (!skb) {
		TS_LOG_ERR("ilitek alloc_skb failed\n");
		return;
	}
	TS_LOG_INFO("ilitek udp_reply\n");
	nlh= nlmsg_put(skb, pid, seq, 0, size, 0);
	nlh->nlmsg_flags = 0;
	data=NLMSG_DATA(nlh);
	memcpy(data, payload, size);
	NETLINK_CB(skb).portid = 0;         /* from kernel */
	NETLINK_CB(skb).dst_group = 0;  /* unicast */
	ret=netlink_unicast(netlink_sock, skb, pid, MSG_DONTWAIT);
	if (ret <0)
	{
		TS_LOG_ERR("ilitek send failed\n");
		return;
	}
	return;

}

/* Receive messages from netlink socket. */
u_int pid = 100, seq = 23/*, sid*/;
kuid_t uid;
static void udp_receive(struct sk_buff  *skb)
{
	void			*data;
	uint8_t buf[64] = {0};
	struct nlmsghdr *nlh;

	nlh = (struct nlmsghdr *)skb->data;
	pid  = 100;//NETLINK_CREDS(skb)->pid;
	uid  = NETLINK_CREDS(skb)->uid;
	//sid  = NETLINK_CB(skb).sid;
	seq  = 23;//nlh->nlmsg_seq;
	data = NLMSG_DATA(nlh);
	//printk("recv skb from user space uid:%d pid:%d seq:%d,sid:%d\n",uid,pid,seq,sid);
	if(!strcmp(data,"Hello World!"))
	{
		//printk("recv skb from user space uid:%d pid:%d seq:%d\n",uid,pid,seq);
		printk("data is :%s\n",(char *)data);
		udp_reply(pid,seq,data,sizeof("Hello World!"));
		//udp_reply(pid,seq,data);
	}
	else
	{
		memcpy(buf,data,64);
	}
	//kfree(data);
	return ;
}
#endif

int ilitek_poll_int(void)
{
	return gpio_get_value(ilitek_data->ilitek_chip_data->ts_platform_data->irq_gpio);
}

int ilitek_check_int_high(int retry) {
	int i = 0;
	int int_status = ILITEK_INT_STATUS_LOW;
	for (i = 0; i < retry; i++) {
		int_status = ilitek_poll_int();
		if (int_status == ILITEK_INT_STATUS_HIGH) {
			break;
		}
		else {
			mdelay(5);
		}
	}
	if (i >= retry) {
		TS_LOG_ERR("ilitek_check_int_high failed retry = %d\n", i);
		return ILITEK_INT_STATUS_LOW;
	}
	else {
		TS_LOG_INFO("ilitek_check_int_high RIGHT retry = %d\n", i);
		return ILITEK_INT_STATUS_HIGH;
	}
}

int ilitek_check_int_low(int retry) {
	int i = 0;
	int int_status = ILITEK_INT_STATUS_HIGH;
	for (i = 0; i < retry; i++) {
		int_status = ilitek_poll_int();
		if (int_status == ILITEK_INT_STATUS_LOW) {
			break;
		}
		else {
			mdelay(5);
		}
	}
	if (i >= retry) {
		TS_LOG_ERR("ilitek_check_int_low failed retry = %d\n", i);
		return ILITEK_INT_STATUS_HIGH;
	}
	else {
		TS_LOG_INFO("ilitek_check_int_low RIGHT retry = %d\n", i);
		return ILITEK_INT_STATUS_LOW;
	}
}


static int ilitek_input_config(struct input_dev *input_dev)
{
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);
	set_bit(TS_PALM_COVERED, input_dev->keybit);
#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
#endif
	input_set_abs_params(input_dev, ABS_X,
		0, (ilitek_data->ilitek_chip_data->x_max - 1), 0, 0);
	input_set_abs_params(input_dev, ABS_Y,
		0, (ilitek_data->ilitek_chip_data->y_max - 1), 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 10, 0, 0);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0,
		(ilitek_data->ilitek_chip_data->x_max - 1), 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0,
		(ilitek_data->ilitek_chip_data->y_max - 1), 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);

#ifdef REPORT_2D_W
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0,
		MAX_ABS_MT_TOUCH_MAJOR, 0, 0);
#endif

#ifdef TYPE_B_PROTOCOL
#ifdef KERNEL_ABOVE_3_7
	/* input_mt_init_slots now has a "flags" parameter */
	input_mt_init_slots(input_dev, 10, INPUT_MT_DIRECT);
#else
	input_mt_init_slots(input_dev, 10);
#endif
#endif
	return NO_ERR;
}

char *ilitek_strncat(unsigned char *dest, char *src, size_t dest_size)
{
	size_t dest_len = 0;
	char *start_index = NULL;
	dest_len = strnlen(dest, dest_size);
	start_index = dest + dest_len;
	return strncat(&dest[dest_len], src, dest_size - dest_len - 1);
}

char *ilitek_strncatint(unsigned char *dest, int src, char *format, size_t dest_size)
{
	char src_str[16] = {0};
	snprintf(src_str, 16, format, src);
	return ilitek_strncat(dest, src_str, dest_size);
}



static int ilitek_irq_bottom_half(struct ts_cmd_node *in_cmd,
	struct ts_cmd_node *out_cmd)
{
	int x = 0;
	int y = 0;
	int  len = 0;
	int  tp_status = 0;
	int i = 0;
	int ret = 0;
	int touch_count = 0;
	struct algo_param *algo_p = NULL;
	struct ts_fingers *info = NULL;

	algo_p = &out_cmd->cmd_param.pub_params.algo_param;
	info = &algo_p->info;

	out_cmd->command = TS_INPUT_ALGO;
	algo_p->algo_order = ilitek_data->ilitek_chip_data->algo_id;
	TS_LOG_DEBUG("%s:algo_order:%d\n", __func__, algo_p->algo_order);
	atomic_set(&ilitek_data->ts_interrupts, ILITEK_TS_HAVE_INTERRUPTS);
#if 1
	static int pre_fingers_status = 0;
	int temp_fingers_status = 0;
#endif
#ifdef REPORT_PRESSURE
	int pressure = 0;
#endif
	int major = 40;
	int minor = 10;
#ifndef TOUCH_PROTOCOL_B
	int release_counter = 0;
#endif
	unsigned char buf[128]={0};
	int offset = 0;
	if(ilitek_data->report_status == 0){
		TS_LOG_INFO("ilitek_data->report_status = 0 return\n");
		return 1;
	}
	buf[0] = ILITEK_TP_CMD_READ_DATA;
	ret = ilitek_i2c_write_and_read( buf, 1, 0, buf, ILITEK_TP_CMD_REPORT_DATA_LEN);
	len = buf[0];//num of fingers
	if (ret < 0) {
		TS_LOG_ERR("ilitek ILITEK_TP_CMD_READ_DATA error return & release\n");
		info->cur_finger_number = 0;
		temp_fingers_status = 0;
		return ret;
	}
	else {
		ret = 0;
	}
	TS_LOG_DEBUG(" buf[0] = 0x%X, buf[1] = 0x%X, buf[2] = 0x%X buf[3] = 0x%X\n", buf[0], buf[1], buf[2], buf[3]);

	if (buf[0] == ILITEK_ESD_CHECK_DATA && buf[1] == ILITEK_ESD_CHECK_DATA
		&& buf[2] == ILITEK_ESD_CHECK_DATA && buf[3] == ILITEK_ESD_CHECK_DATA_END ) {
		ilitek_reset(ILITEK_RESET_MODEL_CHECKFW_DELAY);
#if defined (CONFIG_HUAWEI_DSM)
					if (!dsm_client_ocuppy(ts_dclient)) {
						dsm_client_record(ts_dclient, "TP  find ESD  data(LCD can't send TSVD/TSHD signal)  0x%X, 0x%X, 0x%X, 0x%X \n\n", buf[0], buf[1], buf[2], buf[3]);
						dsm_client_notify(ts_dclient,DSM_TP_FREEZE_ERROR_NO);
					}
#endif
		atomic_set(&ilitek_data->ilitek_chip_data->ts_platform_data->ts_esd_state, TS_ESD_HAPPENDED);
		TS_LOG_ERR("%s  find ESD  data  0x%X, 0x%X, 0x%X, 0x%X \n", __func__, buf[0], buf[1], buf[2], buf[3]);
		info->cur_finger_number = 0;
		temp_fingers_status = 0;
		return ret;
	}
#ifdef DEBUG_NETLINK
		if (debug_flag) {
			udp_reply(pid,seq,buf,sizeof(buf));
		}
		if (buf[1] == ILITEK_DEBUG_DATA) {
			out_cmd->command = -1;
			TS_LOG_INFO("debug data not report out_cmd->command = -1\n");
			return NO_ERR;
		}
#endif
	//buf[0]:num of fingers,buf[1]:fw ok or not,buf[2]:gesture
	if (len > ilitek_data->max_tp) {
		TS_LOG_ERR("ilitek len > ilitek_data->max_tp  return & release\n");
		info->cur_finger_number = 0;
		temp_fingers_status = 0;
		return ret;
	}
	for(i = 0; i < ilitek_data->max_tp; i++){
		tp_status = buf[i*ILITEK_ONE_FINGER_DATA_LEN+ILITEK_REPORT_DATA_HEAD_LEN] >> 7;
		x = (((int)(buf[i*ILITEK_ONE_FINGER_DATA_LEN+ILITEK_X_COORD_H] & 0x3F) << 8) + buf[i*ILITEK_ONE_FINGER_DATA_LEN+ILITEK_X_COORD_L]);
		y = (((int)(buf[i*ILITEK_ONE_FINGER_DATA_LEN+ILITEK_Y_COORD_H] & 0x3F) << 8) + buf[i*ILITEK_ONE_FINGER_DATA_LEN+ILITEK_Y_COORD_L]);
#ifdef REPORT_PRESSURE
		pressure = buf[i * ILITEK_ONE_FINGER_DATA_LEN + ILITEK_PRESS];
#endif
		if (tp_status) {
			info->fingers[i].status = TP_FINGER;
			info->fingers[i].x = x;
			info->fingers[i].y = y;
			info->fingers[i].major = major;
			info->fingers[i].minor = minor;
			info->fingers[i].pressure = pressure;

			touch_count = i + 1;
			ret = 0;
			temp_fingers_status |= (1 << i);
		}
	}
	info->cur_finger_number = touch_count;
	if(len == 0)
	{
		info->cur_finger_number = 0;
	}
#if 1
	TS_LOG_DEBUG("%s,ilitek_data->ilitek_roi_enabled=%d pre_fingers_status = %d temp_fingers_status = %d\n",
	__func__,ilitek_data->ilitek_roi_enabled, pre_fingers_status, temp_fingers_status);
	if (ilitek_data->ilitek_roi_enabled) {
		if (pre_fingers_status < temp_fingers_status) {
			mutex_lock(&(ilitek_data->roi_mutex));
			buf[0] = ILITEK_TP_CMD_READ_ROI_DATA;
			ret = ilitek_i2c_write_and_read( buf, 1, ILITEK_WRITE_READ_NOT_DELAY, (ilitek_data->ilitek_roi_data), ROI_DATA_READ_LENGTH);
			if (ret) {
				TS_LOG_ERR("ilitek read fingersense data failed\n");
			}
			mutex_unlock(&(ilitek_data->roi_mutex));
		}
	}
	pre_fingers_status = temp_fingers_status;
#endif
	return ret;
}


void ilitek_i2c_irq_enable(void)
{
	if (ilitek_data->irq_status == false){
		ilitek_data->irq_status = true;
		enable_irq(ilitek_data->ilitek_chip_data->ts_platform_data->irq_id);
		TS_LOG_DEBUG("ilitek_i2c_irq_enable ok.\n");
	}
	else {
		TS_LOG_DEBUG("no enable\n");
	}
}

void ilitek_i2c_irq_disable(void)
{
	if (ilitek_data->irq_status == true){
		ilitek_data->irq_status = false;
		disable_irq(ilitek_data->ilitek_chip_data->ts_platform_data->irq_id);
		TS_LOG_DEBUG("ilitek_i2c_irq_disable ok.\n");
	}
	else {
		TS_LOG_DEBUG("no disable\n");
	}
}

static unsigned char read_flash_data(unsigned int address) {
	int temp = 0, ret = 0;
	int i = 0;
	unsigned char buf;
	temp = (address << 8) + REG_FLASH_CMD_MEMORY_READ;
	ret = outwrite(REG_FLASH_CMD, temp, 4);
	if (ret ==  (TRANSMIT_ERROR)) {
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return ret;
	}
	ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_TRIGGER_KEY, 4);
	if (ret ==  (TRANSMIT_ERROR)) {
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return ret;
	}
	for (i = 0; i < 50; i++) {
		ret = inwrite(REG_CHK_FLAG);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		if (ret & 0x01) {
			msleep(2);
		}
		else {
			break;
		}
	}
	buf = (unsigned char)(inwrite(FLASH_READ_DATA));
	return buf;
}

int ilitek_i2c_read_tp_info( void)
{
	unsigned char buf[64] = {0};
	int i = 0;
	TS_LOG_DEBUG("%s, driver version:%d.%d.%d\n", __func__, driver_information[0], driver_information[1], driver_information[2]);
	int write_len = 0;
	int read_len = 0;
	for (i = 0; i < 3; i++) {
		buf[0] = ILITEK_TP_CMD_READ_DATA;
		write_len = 1;
		read_len = 3;
		if(ilitek_i2c_write_and_read(buf, write_len, ILITEK_WRITE_READ_DELAY, buf, read_len) < 0)
		{
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return TRANSMIT_ERROR;
		}
		TS_LOG_INFO("%s, 0x10 cmd read data :%X %X %X\n", __func__, buf[0], buf[1], buf[2]);
		if (buf[1] >= FW_OK) {
			break;
		}
		else {
			msleep(3);
		}
	}
	if (i >= 3) {
		TS_LOG_INFO("%s, 0x10 cmd read data error:%X %X %X\n", __func__, buf[0], buf[1], buf[2]);
	}

	buf[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
	buf[1] = ILITEK_TP_CMD_GET_FIRMWARE_VERSION;
	write_len = 2;
	read_len = 0;
	if(ilitek_i2c_write_and_read( buf, write_len, ILITEK_WRITE_READ_DELAY, buf, read_len) < 0)
	{
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return TRANSMIT_ERROR;
	}
	buf[0] = ILITEK_TP_CMD_GET_FIRMWARE_VERSION;
	write_len = 1;
	read_len = 3;
	if(ilitek_i2c_write_and_read( buf, write_len, ILITEK_WRITE_READ_NOT_DELAY, buf, read_len) < 0)
	{
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return TRANSMIT_ERROR;
	}
	TS_LOG_INFO("%s, firmware version:%d.%d.%d\n", __func__, buf[0], buf[1], buf[2]);
	for(i = 0; i < 3; i++)
	{
		ilitek_data->firmware_ver[i] = buf[i];
		if (ilitek_data->firmware_ver[i] == 0xFF) {
			TS_LOG_INFO("%s, firmware version:[%d] = 0xFF so set 0 \n", __func__, i);
			ilitek_data->firmware_ver[0] = 0;
			ilitek_data->firmware_ver[1] = 0;
			ilitek_data->firmware_ver[2] = 0;
			break;
		}
	}

	buf[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
	buf[1] = ILITEK_TP_CMD_GET_PROTOCOL_VERSION;
	write_len = 2;
	read_len = 0;
	if(ilitek_i2c_write_and_read(buf, write_len, ILITEK_WRITE_READ_DELAY, buf, read_len) < 0)
	{
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return TRANSMIT_ERROR;
	}
	buf[0] = ILITEK_TP_CMD_GET_PROTOCOL_VERSION;
	write_len = 1;
	read_len = 2;
	if(ilitek_i2c_write_and_read(buf, write_len, ILITEK_WRITE_READ_NOT_DELAY, buf, read_len) < 0)
	{
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return TRANSMIT_ERROR;
	}
	ilitek_data->protocol_ver = (((int)buf[0]) << 8) + buf[1];
	TS_LOG_INFO("%s, protocol version:%d.%d\n", __func__, buf[0], buf[1]);

	buf[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
	buf[1] = ILITEK_TP_CMD_GET_RESOLUTION;
	write_len = 2;
	read_len = 0;
	if(ilitek_i2c_write_and_read( buf, write_len, ILITEK_WRITE_READ_DELAY, buf, read_len) < 0)
	{
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return TRANSMIT_ERROR;
	}
	buf[0] = ILITEK_TP_CMD_GET_RESOLUTION;
	write_len = 1;
	read_len = 10;
	if(ilitek_i2c_write_and_read( buf, write_len, ILITEK_WRITE_READ_NOT_DELAY, buf, read_len) < 0)
	{
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return TRANSMIT_ERROR;
	}
	ilitek_data->max_x = buf[2];
	ilitek_data->max_x+= ((int)buf[3]) * 256;
	if (JUDGE_ZERO == ilitek_data->max_x)
	{
		ilitek_data->max_x = MAX_X;
	}

	ilitek_data->max_y = buf[4];
	ilitek_data->max_y+= ((int)buf[5]) * 256;
	if (JUDGE_ZERO == ilitek_data->max_y)
	{
		ilitek_data->max_y = MAX_Y;
	}
	ilitek_data->min_x = buf[0];
	ilitek_data->min_y = buf[1];
	ilitek_data->x_ch = buf[6];
	if (JUDGE_ZERO == ilitek_data->x_ch)
	{
		ilitek_data->x_ch = X_CHANNEL_NUM;
	}

	ilitek_data->y_ch = buf[7];
	if (JUDGE_ZERO == ilitek_data->y_ch)
	{
		ilitek_data->y_ch = Y_CHANNEL_NUM;
	}

	ilitek_data->max_tp = buf[8];
	if (JUDGE_ZERO == ilitek_data->max_tp)
	{
		ilitek_data->max_tp = MAX_TOUCH_POINT;
	}

	ilitek_data->keycount = buf[9];

	TS_LOG_INFO("%s, min_x: %d, max_x: %d, min_y: %d, max_y: %d, ch_x: %d, ch_y: %d, max_tp: %d, key_count: %d \n"
			, __func__, ilitek_data->min_x, ilitek_data->max_x, ilitek_data->min_y, ilitek_data->max_y, ilitek_data->x_ch, ilitek_data->y_ch, ilitek_data->max_tp, ilitek_data->keycount);
	return 0;
}

static void ilitek_shutdown(void)
{
	TS_LOG_INFO("ilitek_shutdown\n");
	ilitek_data->suspend = true;
	ilitek_power_off();
	ilitek_regulator_put();
	return;
}

int ilitek_suspend(void)
{
	int ret = 0;
	int i = 0;
	uint8_t cmd[2] = {0};
	TS_LOG_INFO("ilitek_suspend +\n");
	ilitek_data->suspend = true;
	if(ilitek_data->firmware_updating) {
		TS_LOG_INFO("%s: tp fw is updating,return\n", __func__);
		goto out;
	}
	if(ilitek_data->sensor_testing) {
		TS_LOG_INFO("%s: tp fw is sensor_testing,return\n", __func__);
		goto out;
	}
	if (ilitek_data->apk_use) {
		TS_LOG_INFO("%s: open the ilitek_ctrl file for apk use,return\n", __func__);
		goto out;
	}
	ilitek_i2c_irq_disable();
	if (g_tskit_ic_type != ONCELL)
	{
		if (!g_tskit_pt_station_flag) {
			ilitek_power_off();
		}
		else{
			TS_LOG_INFO("+++++++++++++++++++++++++++\n");
			//wait to handler the last irq
			ret = ilitek_check_int_high(INT_POLL_SUSPEND_RESUME_RETRY);
			if (ret != ILITEK_INT_STATUS_HIGH) {
				TS_LOG_ERR("%s, ilitek_check_int_high fail \n", __func__);
			}
			cmd[0] = ILITEK_TP_CMD_SENSE;
			cmd[1] = FUN_DISABLE;
			ret = ilitek_i2c_write(cmd, 2);
			if(ret < 0){
				TS_LOG_ERR("%s, 0x01 0x00 set tp suspend err, ret %d\n", __func__, ret);
			}
			msleep(10);
			cmd[0] = ILITEK_TP_CMD_SLEEP;
			cmd[1] = FUN_DISABLE;
			ret = ilitek_i2c_write(cmd, 2);
			if(ret < 0){
				TS_LOG_ERR("%s, 0x02 0x00 set tp suspend err, ret %d\n", __func__, ret);
			}
		}
	}
out:
	TS_LOG_INFO("ilitek_suspend -\n");
	return 0;
}


/*  do some thing after power on. */
static int ilitek_after_resume(void *feature_info)
{
	int ret = NO_ERR;
	unsigned char buf[3]={0};
	TS_LOG_INFO("ilitek_after_resume +\n");
	if(ilitek_data->firmware_updating) {
		TS_LOG_INFO("%s: tp fw is updating,return\n", __func__);
		return 0;
	}
	if(ilitek_data->sensor_testing) {
		TS_LOG_INFO("%s: tp fw is sensor_testing,return\n", __func__);
		return 0;
	}
	if (ilitek_data->apk_use) {
		TS_LOG_INFO("%s: open the ilitek_ctrl file for apk use,return\n", __func__);
		return 0;
	}

	buf[0] = ILITEK_TP_CMD_READ_DATA;
	ret = ilitek_i2c_write_and_read( buf, 1, ILITEK_WRITE_READ_NOT_DELAY, buf, 3);
	if (ret) {
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
	}
	TS_LOG_INFO("%s ilitek after reset 0x10 read  = 0x%X, 0x%X, 0x%X\n", __func__, buf[0], buf[1], buf[2]);
	if (ilitek_data->glove_status) {
		ilitek_into_glove_mode(true);
	}
	if (ilitek_data->ilitek_roi_enabled) {
		ilitek_into_fingersense_mode(true);
	}
	if (ilitek_data->ilitek_chip_data->ts_platform_data->feature_info.holster_info.holster_switch) {
		ilitek_into_hall_halfmode(true);
	}

	ilitek_i2c_irq_enable();

	TS_LOG_INFO("ilitek_after_resume -\n");
	return ret;
}

int ilitek_resume(void)
{
	int i = 0;
	int ret = NO_ERR;
	unsigned char buf[3]={0};
	TS_LOG_INFO("ilitek_resume +\n");
	if(ilitek_data->firmware_updating) {
		TS_LOG_INFO("%s: tp fw is updating,return\n", __func__);
		goto out;
	}
	if(ilitek_data->sensor_testing) {
		TS_LOG_INFO("%s: tp fw is sensor_testing,return\n", __func__);
		goto out;
	}
	if (ilitek_data->apk_use) {
		TS_LOG_INFO("%s: open the ilitek_ctrl file for apk use,return\n", __func__);
		goto out;
	}
	if (g_tskit_ic_type != ONCELL)
	{
		if (!g_tskit_pt_station_flag) {
			ilitek_power_on();
		}
	}
	ilitek_reset(ILITEK_RESET_MODEL_CHECKFW_DELAY);
	//after reset,wait 10ms,if int is low,means ic is read to give data
out:
	ilitek_i2c_irq_enable();
	ilitek_data->suspend = false;
	TS_LOG_INFO("ilitek_resume -\n");
	return 0;
}

int read_product_id(void)
{
	int product_id_len = ILITEK_PRODUCT_ID_LEN;
	unsigned char buf[4] = {0};
	int ret = 0, tmp = 0;
	int i = 0;
	int write_len = 1;
	int read_len = 3;
	buf[0] = ILITEK_TP_CMD_READ_DATA;
	ret = ilitek_i2c_write_and_read( buf, write_len, ILITEK_WRITE_READ_DELAY, buf, read_len);
	if (buf[1] < FW_OK) {
		TS_LOG_ERR("ilitek reset int pull low but 0x10 read < 0x80 buf[1] = 0x%x force upgrade\n", buf[1]);
		ilitek_data->force_upgrade = true;
	}
	if (ret < 0) {
		TS_LOG_INFO("%s, 0x10 cmd read data :%X %X %X\n", __func__, buf[0], buf[1], buf[2]);
		return TRANSMIT_ERROR;
	}
	ilitek_data->product_id =  kzalloc(sizeof(char) *(product_id_len + 1), GFP_KERNEL);
	if (NULL == ilitek_data->product_id) {
		TS_LOG_ERR("kzalloc ilitek_data->product_id  error\n");
	}
	else {
		ret = outwrite(ENTER_ICE_MODE, 0x0, 0);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		ret = outwrite(REG_FLASH_CMD, REG_FLASH_CMD_RELEASE_FROM_POWER_DOWN, 1);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_TRIGGER_KEY, 4);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		ret = outwrite(REG_TIMING_SET, REG_TIMING_SET_10MS, 1);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		ret = outwrite(REG_CHK_EN, REG_CHK_EN_PARTIAL_READ, 1);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		ret = outwrite(REG_READ_NUM, REG_READ_NUM_1, 2);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		for (i = PRODUCT_ID_STARTADDR; i <= PRODUCT_ID_ENDADDR; i++) {
			ilitek_data->product_id[i - PRODUCT_ID_STARTADDR] = read_flash_data(i);
			TS_LOG_INFO("ilitek flash_buf[0x%X] = 0x%X\n", i, ilitek_data->product_id[i - PRODUCT_ID_STARTADDR]);
			TS_LOG_INFO("ilitek flash_buf[0x%X] = %c\n", i, ilitek_data->product_id[i - PRODUCT_ID_STARTADDR]);
		}
		ilitek_data->product_id[product_id_len + 1] = '\0';
		TS_LOG_INFO("ilitek product id = %s\n", ilitek_data->product_id);
		buf[0] = (unsigned char)(EXIT_ICE_MODE & DATA_SHIFT_0);
		buf[1] = (unsigned char)((EXIT_ICE_MODE & DATA_SHIFT_8) >> 8);
		buf[2] = (unsigned char)((EXIT_ICE_MODE & DATA_SHIFT_16) >> 16);
		buf[3] = (unsigned char)((EXIT_ICE_MODE & DATA_SHIFT_24) >> 24);
		ret = ilitek_i2c_write( buf, 4);
		if (ret) {
			TS_LOG_ERR("%s, line = %d exit ice mode fail TRANSMIT_ERROR reset \n", __func__, __LINE__);
			ilitek_reset(ILITEK_RESET_MODEL_CHECKFW_DELAY);
		}
	}
	return NO_ERR;
}

static int ilitek_init_chip(void)
{
	int retval = 0;
#ifdef DEBUG_NETLINK
		struct netlink_kernel_cfg cfg = {
			.groups = 0,
			.input = udp_receive,
		};
#endif
	ilitek_data->ilitek_chip_data->is_in_cell = true;
	ilitek_data->ilitek_chip_data->easy_wakeup_info.sleep_mode = TS_POWER_OFF_MODE;
	ilitek_data->ilitek_chip_data->easy_wakeup_info.easy_wakeup_gesture = false;
	ilitek_data->ilitek_chip_data->easy_wakeup_info.easy_wakeup_flag = false;
	ilitek_data->ilitek_chip_data->easy_wakeup_info.palm_cover_flag = false;
	ilitek_data->ilitek_chip_data->easy_wakeup_info.palm_cover_control = false;
	ilitek_data->ilitek_chip_data->easy_wakeup_info.off_motion_on = false;
	ilitek_data->ilitek_chip_data->ts_platform_data->feature_info.holster_info.holster_switch = false;
	ilitek_data->ilitek_chip_data->ts_platform_data->feature_info.roi_info.roi_supported = 1;
	TS_LOG_INFO("Enter ilitek_init_chip \n");
#ifdef TOOL
	memset(&dev_ilitek, 0, sizeof(struct dev_data));
#endif
	mutex_init(&ilitek_data->mutex);
	mutex_init(&(ilitek_data->roi_mutex));
	ilitek_data->report_status = true;
	ilitek_data->irq_status = true;
	ilitek_data->suspend = false;
	atomic_set(&ilitek_data->ts_interrupts, ILITEK_TS_NO_INTERRUPTS);

	retval = read_product_id();
	if (retval) {
		TS_LOG_ERR("ilitek read_product_id fail\n");
	}
	retval = ilitek_i2c_read_tp_info();
	if(retval < 0){
		TS_LOG_ERR("ilitek read tp info fail\n");
	}
	TS_LOG_INFO("%s, register input device, success\n", __func__);
#ifdef TOOL
	create_tool_node();
#endif

	Report_Flag=0;
#ifdef DEBUG_NETLINK
		netlink_sock = netlink_kernel_create(&init_net, 21, &cfg);
#endif
	return retval;
}

static int ilitek_chip_check_status(void){
	int retval;
	unsigned char buf[4]={0};
	TS_LOG_DEBUG("%s + \n", __func__);
	TS_LOG_DEBUG("%s -\n", __func__);
	return 0;
}

static int ilitek_regs_operate(struct ts_regs_info *info)
{
	int retval = NO_ERR;
	u8 value[TS_MAX_REG_VALUE_NUM] = { 0 };
	u8 value_tmp = info->values[0];
	int i = 0;

	TS_LOG_INFO("addr(%d),op_action(%d),bit(%d),num(%d)\n", info->addr,
		    info->op_action, info->bit, info->num);

	for (i = 0; i < info->num; i++) {
		TS_LOG_INFO("value[%d]=%d\n", i, info->values[i]);
	}
	#if 1
	switch (info->op_action) {
	case TS_ACTION_WRITE:
		for (i = 0; i < (info->num); i++) {
			value[i] = info->values[i];
		}

		retval = ilitek_i2c_write(&info->addr, 1);
		if (retval < 0) {
			TS_LOG_ERR("TS_ACTION_READ error, addr(%d)\n",info->addr);
			retval = -EINVAL;
			goto out;
		}

		break;
	case TS_ACTION_READ:
		retval = ilitek_i2c_read(&info->addr, 1, value, info->num);
		if (retval < 0) {
			TS_LOG_ERR("TS_ACTION_READ error, addr(%d)\n",info->addr);
			retval = -EINVAL;
			goto out;
		}

		if ((1 == info->num) && (8 > info->bit)) {
			ilitek_data->ilitek_chip_data->reg_values[0] = (value[0] >> info->bit) & 0x01;
		} else {
			for (i = 0; i < info->num; i++) {
				ilitek_data->ilitek_chip_data->reg_values[i] =  value[i];
			}
		}
		break;
	default:
		TS_LOG_ERR("%s, reg operate default invalid action %d\n",
			   __func__, info->op_action);
		retval = -EINVAL;
		break;
	}
	#endif
out:
	return retval;
}

struct ts_device_ops ts_ilitek_ops = {
	.chip_detect = ilitek_chip_detect,
	.chip_init = ilitek_init_chip,
	.chip_input_config = ilitek_input_config,
	.chip_irq_top_half = ilitek_irq_top_half,
	.chip_irq_bottom_half = ilitek_irq_bottom_half,
	.chip_fw_update_boot = ilitek_fw_update_boot,
	.chip_fw_update_sd = ilitek_fw_update_sd,
	.chip_get_info = ilitek_chip_get_info,
	.chip_get_capacitance_test_type = ilitek_chip_get_capacitance_test_type,
	//.chip_set_info_flag = ilitek_set_info_flag,
	//.chip_before_suspend = ilitek_before_suspend,
	.chip_suspend = ilitek_suspend,
	.chip_resume = ilitek_resume,
	.chip_after_resume = ilitek_after_resume,
	//.chip_wakeup_gesture_enable_switch = ilitek_wakeup_gesture_enable_switch,
	.chip_get_rawdata = ilitek_get_raw_data,
	.chip_get_debug_data = ilitek_get_debug_data,
	.chip_glove_switch = ilitek_glove_switch,
	.chip_shutdown = ilitek_shutdown,
	.chip_holster_switch = ilitek_holster_switch,
	.chip_roi_switch = ilitek_roi_switch,
	.chip_roi_rawdata = ilitek_roi_rawdata,
	//.chip_palm_switch = ilitek_palm_switch,
	.chip_regs_operate = ilitek_regs_operate,
	//.chip_calibrate = ilitek_calibrate,
	//.chip_calibrate_wakeup_gesture = ilitek_calibrate_wakeup_gesture,
	.chip_reset = ilitek_reset_device,
#if defined (HUAWEI_CHARGER_FB)
	.chip_charger_switch = ilitek_charger_switch,
#endif
#if defined(CONFIG_HUAWEI_DSM)
	//.chip_dsm_debug = ilitek_dsm_debug,
#endif
#ifdef HUAWEI_TOUCHSCREEN_TEST
	.chip_test = test_dbg_cmd_test,
#endif
	//.chip_wrong_touch = ilitek_wrong_touch,
	.chip_check_status = ilitek_chip_check_status,

};

static int __init ilitek_module_init(void)
{
	int ret = NO_ERR;
	bool found = false;
	struct device_node *child = NULL;
	struct device_node *root = NULL;

	TS_LOG_INFO("%s: called\n", __func__);
	ilitek_data=kzalloc(sizeof(struct i2c_data), GFP_KERNEL);
	if (NULL == ilitek_data) {
		TS_LOG_ERR("%s:alloc mem for device data fail\n", __func__);
		ret = -ENOMEM;
		goto error_exit;
	}
	memset(ilitek_data, 0, sizeof(struct i2c_data));

	ilitek_data->ilitek_chip_data = kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
	if (!ilitek_data->ilitek_chip_data) {
		TS_LOG_ERR("Failed to alloc mem for struct ilitek_chip_data\n");
		goto error_exit;
	}
	root = of_find_compatible_node(NULL, NULL, HUAWEI_TS_KIT);
	if (!root) {
		TS_LOG_ERR("%s:find_compatible_node error\n", __func__);
		ret = -EINVAL;
		goto error_exit;
	}

	for_each_child_of_node(root, child) {
		if (of_device_is_compatible(child, ILITEK_CHIP_NAME)) {
			found = true;
			TS_LOG_INFO("%s:find child node ilitek success\n",__func__);
			break;
		}
	}
	if (!found) {
		TS_LOG_ERR("%s:device tree node not found, name=%s\n",
			__func__, ILITEK_CHIP_NAME);
		ret = -EINVAL;
		goto error_exit;
	}

	ilitek_data->ilitek_chip_data->cnode = child;
	ilitek_data->ilitek_chip_data->ops = &ts_ilitek_ops;
	ret = huawei_ts_chip_register(ilitek_data->ilitek_chip_data);
	if (ret) {
		TS_LOG_ERR("%s:chip register fail, ret=%d\n", __func__, ret);
		goto error_exit;
	}

	TS_LOG_INFO("%s:success\n", __func__);
	return 0;

error_exit:
	if (ilitek_data->ilitek_chip_data) {
		kfree(ilitek_data->ilitek_chip_data);
		ilitek_data->ilitek_chip_data = NULL;
	}
	if (ilitek_data) {
		kfree(ilitek_data);
		ilitek_data = NULL;
	}
	TS_LOG_INFO("%s:fail\n", __func__);
	return ret;
}

static void ilitek_module_exit(void)
{
	#ifdef TOOL
	remove_tool_node();
	#endif
	TS_LOG_INFO("%s\n", __func__);
}

late_initcall(ilitek_module_init);
module_exit(ilitek_module_exit);
