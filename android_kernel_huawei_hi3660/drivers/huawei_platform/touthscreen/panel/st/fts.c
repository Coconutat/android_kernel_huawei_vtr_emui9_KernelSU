/*
 * fts.c
 *
 * FTS Capacitive touch screen controller (FingerTipS)
 *
 * Copyright (C) 2012, 2013 STMicroelectronics Limited.
 * Authors: AMS(Analog Mems Sensor)
 *        : Victor Phay <victor.phay@st.com>
 *        : Li Wu <li.wu@st.com>
 *        : Giuseppe Di Giore <giuseppe.di-giore@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG
#include <linux/device.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/completion.h>
#include <linux/wakelock.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_HAS_EARLYSUSPEND //hank modified
#include <linux/earlysuspend.h>
#endif
#include "fts.h"
#include <linux/notifier.h>
#include <linux/fb.h>
#ifdef KERNEL_ABOVE_2_6_38
#include <linux/input/mt.h>
#endif
#ifdef CONFIG_JANUARY_BOOSTER
#include <linux/input/janeps_booster.h>
#endif
#ifdef CONFIG_USE_CAMERA3_ARCH
#include <media/huawei/hw_extern_pmic.h>
#define VCI_PMIC_POWER_VOLTAGE		3200000
#define VDDIO_PMIC_POWER_VOLTAGE	1850000
#endif
#define VCI_VALUE (3300000)

unsigned char tune_version_same = 0x0;
extern struct ts_data g_ts_data;

static u16 pre_finger_status = 0;
static u16 temp_finger_status = 0;

/*
 * Event installer helpers
 */
#define event_id(_e)     EVENTID_##_e
#define handler_name(_h) st_##_h##_event_handler
	
#define install_handler(_i, _evt, _hnd) \
do { \
	_i->event_dispatch_table[event_id(_evt)] = handler_name(_hnd); \
} while (0)

/* Asyncronouns command helper */
#define WAIT_WITH_TIMEOUT(_info, _timeout, _command) \
do { \
	if (wait_for_completion_timeout(&_info->cmd_done, _timeout) == 0) { \
		TS_LOG_DEBUG("Waiting for %s command: timeout\n", \
		#_command); \
	} \
} while (0)

#ifdef KERNEL_ABOVE_2_6_38
#define TYPE_B_PROTOCOL
#endif

#define ST_IRQ_GPIO "attn_gpio"
#define ST_RST_GPIO "reset_gpio"
#define ST_IRQ_CFG "irq_config"
#define ST_ALGO_ID "algo_id"
#define ST_X_MAX	 "x_max"
#define ST_Y_MAX	 "y_max"
#define ST_X_MAX_MT	 "x_max_mt"
#define ST_Y_MAX_MT	 "y_max_mt"
#define ST_VCI_GPIO_TYPE	 "vci_gpio_type"
#define ST_VCI_PMIC_TYPE	 "vci_pmic_type"
#define ST_VCI_REGULATOR_TYPE	 "vci_regulator_type"
#define ST_VDDIO_GPIO_TYPE	 "vddio_gpio_type"
#define ST_VDDIO_PMIC_TYPE	 "vddio_pmic_type"
#define ST_VDDIO_REGULATOR_TYPE	 "vddio_regulator_type"
#define ST_COVER_FORCE_GLOVE	 "force_glove_in_smart_cover"
#define ST_VCI_GPIO_CTRL "vci_ctrl_gpio"
#define ST_VDDIO_GPIO_CTRL "vddio_ctrl_gpio"
#define ST_COVER_FORCE_GLOVE	 "force_glove_in_smart_cover"

static int st_raw_limit_buf[RAWDATA_LIMIT_NUM] = {0};
/* forward declarations */
static int st_chip_detect (struct device_node *device, struct ts_device_data *chip_data, struct platform_device *ts_dev);
static int st_init_chip(void);
static int st_parse_dts(struct device_node *device, struct ts_device_data *chip_data);
static int st_irq_top_half(struct ts_cmd_node *cmd);
static int st_irq_bottom_half(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd);
static int st_fw_update_boot(char *file_name);
static int st_fw_update_sd(void);
static int st_chip_get_info(struct ts_chip_info_param *info);
static int st_set_info_flag(struct ts_data *info);
static int st_before_suspend(void);
static int st_suspend(void);
static int st_resume(void);
static int st_after_resume(void *feature_info);
static void st_shutdown(void);
static int st_input_config(struct input_dev *input_dev);
static int st_get_rawdata(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd);
static int st_palm_switch(struct ts_palm_info *info);
static int st_glove_switch(struct ts_glove_info *info);
static int st_holster_switch(struct ts_holster_info *info);
static int st_roi_switch(struct ts_roi_info *info);
static unsigned char* st_roi_rawdata(void);
static int st_regs_operate(struct ts_regs_info *info);
static int st_get_capacitance_test_type(struct ts_test_type_info *info);
static int st_calibrate(void);
static int st_set_glove_switch(u8 glove_switch);
static int st_set_holster_switch(u8 holster_switch);
static void st_get_roi_data(struct fts_ts_info *info);
static int st_interrupt_install(struct fts_ts_info *info);

#if defined (CONFIG_TEE_TUI)
static struct tui_mxt_data_t {
	char device_name[10];
} tui_st_data;
#endif

enum FW_UPDATE_STATE{
	FW_UPDATE_INIT=0,
	FW_UPDATE_START,
	FW_UPDATE_NO_NEED,
	FW_UPDATE_SUCC,
	FW_UPDATE_FAIL,
};

struct ts_device_ops ts_st_ops = {
	.chip_detect = st_chip_detect,
	.chip_init = st_init_chip,
	.chip_parse_config = st_parse_dts,
	.chip_input_config = st_input_config,
	.chip_irq_top_half = st_irq_top_half,
	.chip_irq_bottom_half = st_irq_bottom_half,
	.chip_fw_update_boot = st_fw_update_boot,
	.chip_fw_update_sd = st_fw_update_sd,
	.chip_get_info = st_chip_get_info,
	.chip_set_info_flag = st_set_info_flag,
	.chip_before_suspend = st_before_suspend,
	.chip_suspend = st_suspend,
	.chip_resume = st_resume,
	.chip_after_resume = st_after_resume,
	.chip_get_rawdata = st_get_rawdata,
	.chip_glove_switch = st_glove_switch,
	.chip_shutdown = st_shutdown,
	.chip_holster_switch = st_holster_switch,
	.chip_roi_switch = st_roi_switch,
	.chip_roi_rawdata = st_roi_rawdata,
	.chip_palm_switch = st_palm_switch,
	.chip_regs_operate = st_regs_operate,
	.chip_get_capacitance_test_type = st_get_capacitance_test_type,
	.chip_calibrate = st_calibrate,
	.chip_reset = NULL,
};

static struct fts_ts_info *st_info = NULL;

int st_i2c_write(struct fts_ts_info *st_info,
	unsigned char *data, unsigned short length)
{
	int retval;

	if (!st_info->dev_data->bops->bus_write) {
		TS_LOG_ERR("bus_write not exits\n");
		retval = -EIO;
		goto exit;
	}
	retval = st_info->dev_data->bops->bus_write(data, length);
	if (retval < 0) {
		TS_LOG_ERR("bus_write failed, retval = %d\n", retval);
		goto exit;
	}

exit:
	return retval;
}

int st_i2c_read(struct fts_ts_info *st_info,
		unsigned char *addr, unsigned short addr_len, unsigned char *data, unsigned short length)
{
	int retval=NO_ERR;

	if (!st_info->dev_data->bops->bus_read) {
		TS_LOG_ERR("error, invalid bus_read\n");
		retval = -EIO;
		goto exit;
	}
	retval = st_info->dev_data->bops->bus_read(addr,addr_len, data, length);
	if (retval != NO_ERR) {
		TS_LOG_ERR("error, bus read failed, retval  = %d\n", retval);
		goto exit;
	}

exit:
	return retval;
}

void st_interrupt_set(struct fts_ts_info *info, int enable)
{
	unsigned char regAdd[4] = {0xB6, 0x00, 0x1C, enable};
	int ret;

	ret = st_i2c_write(info, regAdd, 4);
	if (ret)
		TS_LOG_ERR(" fts:interrupt_set 0x%02x, return value %d\n", enable, ret);
}

int st_command(struct fts_ts_info *info, unsigned char cmd)
{
	unsigned char regAdd;
	int ret;

	regAdd = cmd;
	ret = st_i2c_write(info, &regAdd, sizeof(regAdd)); // 0 = ok
	if(ret)
		TS_LOG_ERR(" fts:Issued command 0x%02x, return value %d\n", cmd, ret);
	return ret;
}

static void st_chip_hw_rst(struct ts_device_data *chip_data)
{
	//reset chip
	if (chip_data->reset_gpio > 0){
		gpio_direction_output(chip_data->reset_gpio, 0);
		mdelay(3);
		gpio_direction_output(chip_data->reset_gpio, 1);
		mdelay(70);
	}
}

static int st_chip_sw_rst(struct fts_ts_info *info)
{
	int retval = NO_ERR;
	unsigned char regAdd[4] = { 0xB6, 0x00, 0x23, 0x01 };

	retval = st_i2c_write(info, regAdd, sizeof(regAdd));
	if (retval != NO_ERR){
		TS_LOG_ERR( "%s send regAdd Fail\n", __func__);
	}

	usleep_range(5000, 6000);
	return retval;
}

int st_get_fw_version(struct fts_ts_info *info)
{
	unsigned char val[8] = {0};
	unsigned char regAdd[3] = {0xB6, 0x00, 0x07};
	unsigned char regAdd2[4]={0xB2, 0x00, 0x01,0x04};
	unsigned char regAdd3 = READ_ONE_EVENT;
	int error = NO_ERR;
	int event_num = 0xfff;

	st_interrupt_set(info, INT_DISABLE);
	mdelay(10);
	error = st_command(info, FLUSHBUFFER);

	error = st_i2c_read(info, regAdd, sizeof(regAdd), val, sizeof(val));
	if (error != NO_ERR){
		TS_LOG_ERR("%s: read data error\n", __func__);
		return -ENODEV;
	}

	/*check for chip id*/
	if ((val[1] != FTS_ID0) || (val[2] != FTS_ID1)) {
		TS_LOG_ERR("Wrong version id (read 0x%02x%02x, expected 0x%02x%02x)\n",
				val[1], val[2], FTS_ID0, FTS_ID1);
		return -ENODEV;
	} else {
		info->fw_version = (val[5] << 8) | val[4];
	}

	error = st_i2c_write(info, regAdd2, sizeof(regAdd2));
	if (error != NO_ERR){
		TS_LOG_ERR("%s: write regAdd2 error\n", __func__);
		return -ENODEV;
	}
	mdelay(30);

	while (event_num >= 0) {
		error = st_i2c_read(info, &regAdd3,sizeof(regAdd3), val, FTS_EVENT_SIZE);
		if (error != NO_ERR) {
			info->config_id = 0;
			TS_LOG_ERR("Cannot read device info\n");
			return -ENODEV;
		}

		TS_LOG_INFO("%s: val[0] is 0x%x, val[1] is 0x%x, val[2] is 0x%x\n", __func__, val[0],val[1],val[2]);

		if (val[0] == 0x12 && val[1] == 0x00 && val[2] == 0x01) {
			info->config_id = (val[4] << 8) | val[3];
			break;
		}

		/* avoid loop forever */
		if (event_num == 0xfff) {
			event_num = val[7];
			/* max event num is 32(FTS_FIFO_MAX),so... */
			if (event_num > FTS_FIFO_MAX) {
				event_num = FTS_FIFO_MAX;
			}
		}
		event_num--;
	}
	TS_LOG_INFO("%s: config_id=%x\n", __func__, info->config_id);

	st_interrupt_set(info, INT_ENABLE);
	mdelay(10);

	return 0;
}

static void st_set_event_to_fingers(struct fts_ts_info *info, int x, int y, int major,
	int minor, int pressure, int status, unsigned char touchId, unsigned char touchcount)
{
	struct ts_fingers *f_info = info->fingers_info;

	TS_LOG_DEBUG("%s: x:%d, y:%d, z:%d, status:%d, touchId:%u, touchcount:%u\n",
		__func__, x, y, pressure, status, touchId, touchcount);

	if (touchId <= 0 || touchId >= 10){
		touchId = 0;
	}

	if (0xEFFE == f_info->fingers[touchId].pressure) {
		TS_LOG_INFO("Not handle pointer event, has handled leave event\n");
		return;
	}

	memset(&f_info->fingers[touchId], 0, sizeof(f_info->fingers[touchId]));
	f_info->cur_finger_number = 0;

	f_info->fingers[touchId].x = x;
	f_info->fingers[touchId].y = y;
	f_info->fingers[touchId].ewx = major;
	f_info->fingers[touchId].ewy = minor;
	f_info->fingers[touchId].pressure = pressure;
	f_info->fingers[touchId].status = status;
	f_info->cur_finger_number += touchcount;

	temp_finger_status |= (1 << touchId);
	return;
}

/*
 * New Interrupt handle implementation
 */

static inline unsigned char *st_next_event(unsigned char *evt)
{
	/* Nothing to do with this event, moving to the next one */
	evt += FTS_EVENT_SIZE;

	/* the previous one was the last event ?  */
	return (evt[-1] & 0x1F) ? evt : NULL;
}

static int st_parse_dts(struct device_node *device, struct ts_device_data *chip_data)
{
	int retval  = NO_ERR;
	const char *raw_data_dts = NULL;
	int index = 0;
	int array_len = 0;

	TS_LOG_INFO("%s st parse dts start\n", __func__);

	ts_anti_false_touch_param_achieve(chip_data);

	chip_data->irq_gpio = 
		of_get_named_gpio(device, ST_IRQ_GPIO, 0);
	if (!gpio_is_valid(chip_data->irq_gpio)) {
		TS_LOG_ERR("irq gpio is not valid, value is %d\n", chip_data->irq_gpio);
		retval = -EINVAL;
		goto err;
	}

	retval = 
	of_property_read_u32(device, ST_IRQ_CFG, &chip_data->irq_config);
	if (retval) {
		TS_LOG_ERR("get irq config failed\n");
		retval = -EINVAL;
		goto err;
	}
	
	retval = 
		of_property_read_u32(device, ST_ALGO_ID, &chip_data->algo_id);
	if (retval) {
		TS_LOG_ERR("get algo id failed\n");
		retval = -EINVAL;
		goto err;
	}
	
	retval = 
		of_property_read_u32(device, ST_X_MAX, &chip_data->x_max);
	if (retval) {
		TS_LOG_ERR("get device x_max failed\n");
		retval = -EINVAL;
		goto err;
	}
	retval = 
		of_property_read_u32(device, ST_Y_MAX, &chip_data->y_max);
	if (retval) {
		TS_LOG_ERR("get device y_max failed\n");
		retval = -EINVAL;
		goto err;
	}
	retval = 
		of_property_read_u32(device, ST_X_MAX_MT, &chip_data->x_max_mt);
	if (retval) {
		TS_LOG_ERR("get device x_max failed\n");
		retval = -EINVAL;
		goto err;
	}
	retval = 
		of_property_read_u32(device, ST_Y_MAX_MT, &chip_data->y_max_mt);
	if (retval) {
		TS_LOG_ERR("get device y_max failed\n");
		retval = -EINVAL;
		goto err;
	}
	
	retval = 
		of_property_read_u32(device, ST_VCI_GPIO_TYPE, &chip_data->vci_gpio_type);
	if (retval) {
		TS_LOG_ERR("get device ST_VCI_GPIO_TYPE failed\n");
		retval = -EINVAL;
		goto err;
	}
#ifdef CONFIG_USE_CAMERA3_ARCH
	retval = 
		of_property_read_u32(device, ST_VCI_PMIC_TYPE, &chip_data->vci_pmic_type);
	if (retval) {
		TS_LOG_ERR("get device ST_VCI_PMIC_TYPE failed\n");
	}
#endif
	retval = 
		of_property_read_u32(device, ST_VCI_REGULATOR_TYPE, &chip_data->vci_regulator_type);
	if (retval) {
		TS_LOG_ERR("get device ST_VCI_REGULATOR_TYPE failed\n");
		retval = -EINVAL;
		goto err;
	}
	retval = 
		of_property_read_u32(device, ST_VDDIO_GPIO_TYPE, &chip_data->vddio_gpio_type);
	if (retval) {
		TS_LOG_ERR("get device ST_VDDIO_GPIO_TYPE failed\n");
		retval = -EINVAL;
		goto err;
	}
#ifdef CONFIG_USE_CAMERA3_ARCH
	retval = 
		of_property_read_u32(device, ST_VDDIO_PMIC_TYPE, &chip_data->vddio_pmic_type);
	if (retval) {
		TS_LOG_ERR("get device ST_VDDIO_PMIC_TYPE failed\n");
	}
#endif
	retval = 
		of_property_read_u32(device, ST_VDDIO_REGULATOR_TYPE, &chip_data->vddio_regulator_type);
	if (retval) {
		TS_LOG_ERR("get device ST_VDDIO_REGULATOR_TYPE failed\n");
		retval = -EINVAL;
		goto err;
	}

	retval =
	    of_property_read_u32(device, "disable_reset_bit",
				 &chip_data->disable_reset);
	if (!retval) {
		TS_LOG_INFO("not use hw reset\n",
			    chip_data->disable_reset);
	} else {
		chip_data->disable_reset = 0;
		TS_LOG_INFO("use hw reset\n");
	}

	if (!chip_data->disable_reset){
		chip_data->reset_gpio = of_get_named_gpio(device, ST_RST_GPIO, 0);
		if (!gpio_is_valid(chip_data->reset_gpio)) {
			TS_LOG_ERR("reset gpio is not valid\n");
			retval = -EINVAL;
			goto err;
		}
	}

	retval =
	    of_property_read_u32(device, "supported_func_indicater", &chip_data->supported_func_indicater);
	if (retval) {
		TS_LOG_INFO("get supported_func_indicater = 0\n" );
 		chip_data->supported_func_indicater = 0;
	}
	/*0 is power supplied by gpio, 1 is power supplied by ldo*/
	if (1 == chip_data->vci_gpio_type) {
		chip_data->vci_gpio_ctrl = of_get_named_gpio(device, ST_VCI_GPIO_CTRL, 0);
		if (!gpio_is_valid(chip_data->vci_gpio_ctrl)) {
			TS_LOG_ERR("SFT: ok; ASIC: Real err----power gpio is not valid\n");
		}
	}
	if (1 == chip_data->vddio_gpio_type) {
		chip_data->vddio_gpio_ctrl = of_get_named_gpio(device, ST_VDDIO_GPIO_CTRL, 0);
		if (!gpio_is_valid(chip_data->vddio_gpio_ctrl)) {
			TS_LOG_ERR("SFT: ok; ASIC: Real err----power gpio is not valid\n");
		}
	}

	retval =
	    of_property_read_string(device, "tp_test_type",
				 (const char **)&chip_data->tp_test_type);
	if (retval) {
		TS_LOG_INFO
		    ("get device tp_test_type not exit,use default value\n");
		strncpy(chip_data->tp_test_type,
			"Normalize_type:judge_different_reslut",
			TS_CAP_TEST_TYPE_LEN);
		retval = 0;
	}

	/*0 is cover without glass, 1 is cover with glass that need glove mode*/
	retval = of_property_read_u32(device, ST_COVER_FORCE_GLOVE, &chip_data->cover_force_glove);
	if (retval) {
		TS_LOG_INFO("get device COVER_FORCE_GLOVE failed,use default!\n");
		chip_data->cover_force_glove = 0;//if not define in dtsi,set 0 to disable it
		retval = 0;
	}

	array_len = of_property_count_strings(device, "raw_data_limit");
	if (array_len <= 0 || array_len > RAWDATA_LIMIT_NUM) {
		TS_LOG_ERR("raw_data_limit length invaild or dts number is larger than:%d\n", array_len);
	}

	for(index = 0; index < array_len; index++){
		retval = of_property_read_string_index(device, "raw_data_limit", index, &raw_data_dts);
		if (retval) {
			TS_LOG_ERR("read index = %d,raw_data_limit = %s,retval = %d error,\n", index, raw_data_dts, retval);
		}

		st_raw_limit_buf[index] = simple_strtol(raw_data_dts, NULL, 10);
		TS_LOG_INFO("rawdatabuf[%d] = %d\n", index, st_raw_limit_buf[index]);
	}

	TS_LOG_INFO("reset_gpio = %d, irq_gpio = %d, irq_config = %d, algo_id = %d, x_max = %d, y_max = %d, x_mt = %d,y_mt = %d\n", \
		chip_data->reset_gpio, chip_data->irq_gpio, chip_data->irq_config, chip_data->algo_id,
		chip_data->x_max, chip_data->y_max, chip_data->x_max_mt, chip_data->y_max_mt);
err:
	return retval;
}

static int st_regulator_get(struct ts_device_data *chip_data)
{
	int error = 0;

	if (chip_data->vci_regulator_type) {
		st_info->pwr_reg = regulator_get(&st_info->pdev->dev, "st-vci");
		if (IS_ERR(st_info->pwr_reg)) {
			TS_LOG_ERR("regulator tp pwr_reg not used\n");
			return -EINVAL;
		}
		TS_LOG_INFO("set vci volatage to 3.3V\n");
		error = regulator_set_voltage(st_info->pwr_reg, VCI_VALUE, VCI_VALUE);
		if (error < 0) {
			TS_LOG_ERR("failed to set voltage regulator tp_vci error: %d\n",
				   error);
			return error;
		}
	}

	if (chip_data->vddio_regulator_type) {
		st_info->bus_reg = regulator_get(&st_info->pdev->dev, "st-io");
		if (IS_ERR(st_info->bus_reg)) {
			TS_LOG_ERR("regulator tp bus_reg not used\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int st_gpio_request(struct ts_device_data *chip_data)
{
	int retval = 0; 

	//vci gpio config
	if (chip_data->vci_gpio_type && chip_data->vci_gpio_ctrl > 0){
		retval = gpio_request(chip_data->vci_gpio_ctrl, "fts_vci_ctrl_gpio");
		if (retval){
			TS_LOG_ERR("vci gpio ctrl request fail\n");
			goto st_gpio_req_fail;
		}
	}

	//vddio gpio config
	if (chip_data->vddio_gpio_type
		&& chip_data->vddio_gpio_ctrl > 0
		&& chip_data->vddio_gpio_ctrl != chip_data->vci_gpio_ctrl){
		retval = gpio_request(chip_data->vddio_gpio_ctrl, "fts_vddio_ctrl_gpio");
		if (retval){
			TS_LOG_ERR("vddio gpio ctrl request fail\n");
			goto st_gpio_req_fail;
		}
	}

	//irq gpio request
	retval = gpio_request(chip_data->irq_gpio, "fts_irq_gpio");
	if (retval) {
		TS_LOG_ERR("irq gpio request fail\n");
		goto st_gpio_req_fail;
	}

	//reset gpio request
	if (!chip_data->disable_reset) {
		retval = gpio_request(chip_data->reset_gpio, "fts_reset_gpio");
		if (retval){
			TS_LOG_ERR("reset gpio request fail\n");
		}
	}
st_gpio_req_fail:
	return retval;
}

static int st_pinctrl_get(void)
{
	int retval = NO_ERR;

	st_info->pctrl = devm_pinctrl_get(&(st_info->pdev->dev));
	if (IS_ERR(st_info->pctrl)) {
	    st_info->pctrl = NULL;
		TS_LOG_ERR("pinctrl get fail\n");
		goto out;
	}

	st_info->pins_default = pinctrl_lookup_state(st_info->pctrl, "default");
	if (IS_ERR(st_info->pins_default)) {
		st_info->pins_default = NULL;
		TS_LOG_ERR("failed to lookup state default\n");
		goto out;
	}

	st_info->pins_idle = pinctrl_lookup_state(st_info->pctrl, "idle");
	if (IS_ERR(st_info->pins_idle)) {
		st_info->pins_idle = NULL;
		TS_LOG_ERR("failed to lookup state idle\n");
		goto out;
	}

out:
	return retval;
}

static void st_gpio_free(struct ts_device_data *chip_data)
{
	//vci gpio config
	if (chip_data->vci_gpio_type && chip_data->vci_gpio_ctrl > 0){
		gpio_free(chip_data->vci_gpio_ctrl);
	}

	//vddio gpio config
	if (chip_data->vddio_gpio_type 
		&& chip_data->vddio_gpio_ctrl > 0
		&& chip_data->vddio_gpio_ctrl != chip_data->vci_gpio_ctrl){
		gpio_free(chip_data->vddio_gpio_ctrl);
	}

	//reset gpio conifg
	if (!chip_data->disable_reset) {
		if (chip_data->reset_gpio > 0){
			gpio_free(chip_data->reset_gpio);
		}
	}
	//irq gpio config
	if (chip_data->irq_gpio > 0){
		gpio_free(chip_data->irq_gpio);
	}
	return ;
}

static void st_pinctrl_normal_mode(void)
{
	int error = 0;

	if (!st_info->pctrl) {
		TS_LOG_INFO("pctrl is NULL\n");
		return;
	}

	if (!st_info->pins_default){
		TS_LOG_INFO("pins_default is NULL\n");
		return;
	}

	error = pinctrl_select_state(st_info->pctrl, st_info->pins_default);
	if (error < 0) {
		TS_LOG_ERR("set iomux normal error, %d\n", error);
	}
}

static void st_pinctrl_lp_mode(void)
{
	int error = 0;

	if (!st_info->pctrl) {
		TS_LOG_INFO("pctrl is NULL\n");
		return;
	}

	if (!st_info->pins_idle){
		TS_LOG_INFO("pins_idle is NULL\n");
		return;
	}

	error = pinctrl_select_state(st_info->pctrl, st_info->pins_idle);
	if (error < 0) {
		TS_LOG_ERR("set iomux idle error, %d\n", error);
	}
}

static void st_gpio_lp_mode(void)
{
	if (!st_info->dev_data->disable_reset) {
		gpio_direction_output(st_info->dev_data->reset_gpio, 0);
	}else{
		TS_LOG_INFO("%s:Not use hw reset\n", __func__);
	}
}

static void st_regulator_enable(struct ts_device_data *chip_data){
	//output vci
	if (chip_data->vci_gpio_type && chip_data->vci_gpio_ctrl > 0){
		gpio_direction_output(chip_data->vci_gpio_ctrl, 1);
		mdelay(5);
	}

	if (chip_data->vci_regulator_type) {
		if (!IS_ERR(st_info->pwr_reg)) {
			TS_LOG_INFO("vci enable is called\n");
			regulator_enable(st_info->pwr_reg);
			mdelay(5);
		}
	}
#ifdef CONFIG_USE_CAMERA3_ARCH
	if (chip_data->vci_pmic_type){
		hw_extern_pmic_config(2, VCI_PMIC_POWER_VOLTAGE, 1);
		TS_LOG_INFO("vci pmic enable is called\n");
		mdelay(10);
	}
#endif
	//output vddio
	if (chip_data->vddio_gpio_type 
		&& chip_data->vddio_gpio_ctrl > 0
		&& chip_data->vddio_gpio_ctrl != chip_data->vci_gpio_ctrl){
		gpio_direction_output(chip_data->vddio_gpio_ctrl, 1);
		mdelay(5);
	}

	if (chip_data->vddio_regulator_type) {
		if (!IS_ERR(st_info->bus_reg)) {
			TS_LOG_INFO("vddio enable is called\n");
			regulator_enable(st_info->bus_reg);
			mdelay(5);
		}
	}
#ifdef CONFIG_USE_CAMERA3_ARCH
	if (chip_data->vddio_pmic_type){
		hw_extern_pmic_config(3, VDDIO_PMIC_POWER_VOLTAGE, 1);
		TS_LOG_INFO("vddio pmic enable is called\n");
		mdelay(10);
	}
#endif
}

static void st_regulator_disable(struct ts_device_data *chip_data){
	//disable vddio
	if (chip_data->vddio_gpio_type 
		&& chip_data->vddio_gpio_ctrl > 0
		&& chip_data->vddio_gpio_ctrl != chip_data->vci_gpio_ctrl){
		gpio_direction_output(chip_data->vddio_gpio_ctrl, 0);
		mdelay(5);
	}

	if (chip_data->vddio_regulator_type) {
		if (!IS_ERR(st_info->bus_reg)) {
			TS_LOG_INFO("vddio enable is called\n");
			regulator_disable(st_info->bus_reg);
			mdelay(5);
		}		
	}
#ifdef CONFIG_USE_CAMERA3_ARCH
	if (chip_data->vddio_pmic_type){
		hw_extern_pmic_config(3, VDDIO_PMIC_POWER_VOLTAGE, 0);
		TS_LOG_INFO("vddio pmic disable is called\n");
		mdelay(5);
	}
#endif
	//disable vci
	if (chip_data->vci_gpio_type && chip_data->vci_gpio_ctrl > 0){
		gpio_direction_output(chip_data->vci_gpio_ctrl, 0);
		mdelay(5);
	}

	if (chip_data->vci_regulator_type) {
		if (!IS_ERR(st_info->pwr_reg)) {
			TS_LOG_INFO("vci enable is called\n");
			regulator_disable(st_info->pwr_reg);
			mdelay(5);
		}		
	}
#ifdef CONFIG_USE_CAMERA3_ARCH
	if (chip_data->vci_pmic_type){
		hw_extern_pmic_config(2, VCI_PMIC_POWER_VOLTAGE, 0);
		TS_LOG_INFO("vci pmic disable is called\n");
		mdelay(5);
	}
#endif
}

static void st_regulator_put(struct ts_device_data *chip_data)
{
	if (chip_data->vci_regulator_type) {
		if (!IS_ERR(st_info->pwr_reg)) {
			regulator_put(st_info->pwr_reg);
		}
	}
	if (chip_data->vddio_regulator_type) {
		if (!IS_ERR(st_info->bus_reg)) {
			regulator_put(st_info->bus_reg);
		}
	}
}

static void st_power_on(struct ts_device_data *chip_data)
{
	TS_LOG_INFO("%s+\n", __func__);
	st_regulator_enable(chip_data);
	st_pinctrl_normal_mode();
	gpio_direction_input(chip_data->irq_gpio);
}

static void st_hw_reset(struct ts_device_data *chip_data)
{
	if (!chip_data->disable_reset) {
		TS_LOG_INFO("%s:use hw reset\n", __func__);
		st_chip_hw_rst(chip_data);
	}else{
		TS_LOG_INFO("%s:Not use hw reset\n", __func__);
	}
}

static void st_power_off(struct ts_device_data *chip_data)
{
	TS_LOG_INFO("%s+\n", __func__);
	st_pinctrl_lp_mode();
	st_gpio_lp_mode();
	st_regulator_disable(chip_data);
}

// add Power Cycle function  2015/11/3 20:06
void st_chip_powercycle(struct fts_ts_info *info)
{
	st_regulator_disable(info->dev_data);
	msleep(300);
	st_regulator_enable(info->dev_data);
	msleep(300);
	st_hw_reset(info->dev_data);
	msleep(300);
}

static int st_chip_detect (struct device_node *device,
		struct ts_device_data *dev_data, struct platform_device *pdev)
{
	int retval = NO_ERR;

	TS_LOG_INFO("st chip detect called\n");

	if (!device || !dev_data || !pdev) {
		TS_LOG_ERR("device, chip_data or ts_dev is NULL \n");
		return -ENOMEM;
	}

	st_info = kzalloc(sizeof(struct fts_ts_info), GFP_KERNEL);
	if (!st_info) {
		TS_LOG_ERR("st get ts_info mem fail\n");
		goto detect_fail;
	}

	st_info->dev_data = dev_data;
	st_info->pdev = pdev;
	st_info->pdev->dev.of_node = device;
	st_info->dev = &pdev->dev;

	st_info->sensor_max_x = dev_data->x_max - 1;
	st_info->sensor_max_y = dev_data->y_max - 1;
	st_info->sensor_max_x_mt = dev_data->x_max_mt - 1;
	st_info->sensor_max_y_mt = dev_data->y_max_mt - 1;
	st_info->fw_force = 0;

	mutex_init(&st_info->fts_mode_mutex);

	retval = st_interrupt_install(st_info);
	if (retval) {
		TS_LOG_ERR("Init (1) error (#errors = %d)\n", retval);
		goto st_regulator_get_fail;
	}

	retval = st_regulator_get(dev_data);
	if (retval){
		TS_LOG_ERR("st regulator get fail\n");
		goto st_regulator_get_fail;
	}

	retval = st_gpio_request(dev_data);
	if (retval){
		TS_LOG_ERR("st gpio request fail\n");
		goto st_gpio_request_fail;
	}

	retval = st_pinctrl_get();
	if (retval){
		TS_LOG_ERR("st power on fail\n");
		goto st_pinctrl_init_fail;
	}

	st_power_on(dev_data);
	st_hw_reset(dev_data);

	retval = st_chip_sw_rst(st_info);
	if (retval) {
		TS_LOG_ERR("st chip detect fail\n");
		goto st_chip_rst_fail;
	}
	TS_LOG_INFO("st detect success\n");
	return 0;

st_chip_rst_fail:
st_pinctrl_init_fail:
	st_gpio_free(dev_data);
st_gpio_request_fail:
	st_regulator_put(dev_data);
st_regulator_get_fail:
	if(st_info){
		TS_LOG_INFO("free memery for st_info\n");
		if (st_info->event_dispatch_table) {
			kfree(st_info->event_dispatch_table);
			st_info->event_dispatch_table = NULL;
		}

		kfree(st_info);
		st_info = NULL;
	}
detect_fail:
	return retval;
}

int st_systemreset(struct fts_ts_info *info)
{
	int ret;
	unsigned char regAdd[4] = { 0xB6, 0x00, 0x23, 0x01 };

	TS_LOG_INFO("Doing a system reset\n");

	ret = st_i2c_write(info, regAdd, sizeof(regAdd));

	usleep_range(5000, 6000);

	return ret;
}

int st_wait_controller_ready(struct fts_ts_info *info)
{
	unsigned int retry =0;
	unsigned int error = 0;

	unsigned char regAdd[8];
	unsigned char data[FTS_EVENT_SIZE] = {0};
	/* Read controller ready event*/
	for (retry = 0; retry <= CNRL_RDY_CNT; retry++){ // poll with time-out 5000ms
		regAdd[0] = READ_ONE_EVENT;
		error = st_i2c_read(info, regAdd,1, data, FTS_EVENT_SIZE);
		if (error){
			TS_LOG_ERR("%s : I2c READ ERROR : Cannot read device info\n",__func__);
			return -ENODEV;
		}

		if (data[0] == EVENTID_CONTROLLER_READY){
			break;
		} else {
			mdelay(10);
			if(retry == CNRL_RDY_CNT){
				TS_LOG_ERR("%s : TIMEOUT ,Cannot read controller ready event after system reset\n",__func__);
				return -ENODEV;
			}
		}
	}			
	return 0;
}

static void swipe_gesture_control(char *data, char *reg)
{
	if (!reg)
		return;

	if(data[0]&0x01)
		reg[0] |= (1<<7);
	else
		reg[0] &= ~(1<<7);
	
	if(data[0]&0x02)
		reg[1] |= (1);
	else
		reg[1] &= ~(1);
	
	if(data[0]&0x04)
		reg[1] |= (1<<1);
	else
		reg[1] &= ~(1<<1);
	
	if(data[0]&0x08)
		reg[1] |= (1<<2);
	else
		reg[1] &= ~(1<<2);
}

static void unicode_gesture_control(char *data, char *reg)
{
	/*handler V*/
	if(data[0]&0x01)
		reg[1] |= (1<<5);
	else
		reg[1] &= ~(1<<5);
	/*handler C*/
	if(data[0]&0x02)
		reg[0] |= (1<<3);
	else
		reg[0] &= ~(1<<3);
	/*handler E*/
	if(data[0]&0x04)
		reg[0] |= (1<<6);
	else
		reg[0] &= ~(1<<6);
	/*handler W*/
	if(data[0]&0x08)
		reg[0] |= (1<<5);
	else
		reg[0] &= ~(1<<5);
	/*handler M*/
	if(data[0]&0x10)
		reg[0] |= (1<<4);
	else
		reg[0] &= ~(1<<4);
	/*handler S*/
	if(data[0]&0x20)
		reg[1] |= (1<<7);
	else
		reg[1] &= ~(1<<7);
	/*handler O*/
	if(data[0]&0x80)
		reg[0] |= (1<<2);
	else
		reg[0] &= ~(1<<2);
	/*handler Z*/
	if(data[0]&0x40)
		reg[2] |= (1);
	else
		reg[2] &= ~(1);
}

static void tap_gesture_control(char *data, char *reg)
{
	if(data[0])
		reg[0] |= (1<<1);
	else
		reg[0] &= ~(1<<1);
}

static int st_set_gesture_reg(struct fts_ts_info *info, char *mode)
{
	int i;
 	unsigned char reg[6] = {0xC1, 0x06};
	unsigned char regcmd[6] = {0xC2, 0x06, 0xFF, 0xFF, 0xFF, 0xFF};

	for(i = 0; i < 4; i++){
		reg[i+2] = *(mode + i);
	}

	st_i2c_write(info, regcmd, sizeof(regcmd));
	usleep_range(5000,6000);
	st_i2c_write(info, reg, sizeof(reg));
	usleep_range(5000,6000);
 	TS_LOG_DEBUG(" set gesture mode %d %d %d\n", *mode, *(mode+1), *(mode+2));
	return 0;
}

static void st_set_gesture(struct fts_ts_info *info)
{
	//int *p = (int *)info->gesture_mask;
	int all = (info->gesture_mask[ALL_INDEX] & 0xC0) >> 6;	
	char reg_data[4] = { 0 };

	if(all == 2){/*enable some gesture*/
		swipe_gesture_control(&info->gesture_mask[SWIPE_INDEX],(reg_data));
		unicode_gesture_control(&info->gesture_mask[UNICODE_INDEX],reg_data);
		tap_gesture_control(&info->gesture_mask[TAP_INDEX], reg_data);
		reg_data[3] = 0x0;
	}else if(all == 1){/*enable all gesture*/
		reg_data[0] |= 0xFE; 
		reg_data[1] |= 0xA7; // 
		reg_data[2] |= 0x1;
		reg_data[3] = 0x0;
	}

	st_set_gesture_reg(info, reg_data);

}

static inline void st_set_sensor_mode(struct fts_ts_info *info,int mode)
{
	if(!info)
		return ;
	mutex_lock(&info->fts_mode_mutex);
	info->mode = mode ;
	mutex_unlock(&info->fts_mode_mutex);
	return;
}

/* EventId : 0x00 */
static unsigned char *st_nop_event_handler(struct fts_ts_info *info,
			unsigned char *event)
{
	TS_LOG_DEBUG("Doing nothing for event 0x%02x\n", *event);
	return st_next_event(event);
}

/* EventId : 0x03 */
static unsigned char *st_enter_pointer_event_handler(struct fts_ts_info *info,
			unsigned char *event)
{
	unsigned char touchId,touchcount;
	int x, y,  status, major, minor;

	TS_LOG_DEBUG("event0 is %d,event1 is %d,event2 is %d,event3 is %d,event4 is %d,event5 is %d,event6 is %d\n",
			event[0],event[1],event[2],event[3],event[4],event[5],event[6]);

	touchId = event[1] & 0x0F;
	touchcount = (event[1] & 0xF0) >> 4;

	//__set_bit(touchId, &info->touch_id);

	x = (event[2] << 4) | (event[4] & 0xF0) >> 4;
	y = (event[3] << 4) | (event[4] & 0x0F);
	//z = (event[5] & 0x1F);				// bit0-bit4: press
	status = (event[5] & 0xE0) >> 5;	//bit5-bit7:finger status
	major = (event[5] & 0x1F);			// bit0-bit4: major
	minor = event[6];					// event6:minor

	if (x == X_AXIS_MAX)
		x--;

	if (y == Y_AXIS_MAX)
		y--;

	st_set_event_to_fingers(info, x, y, major, minor, 1, status, touchId, touchcount);

	return st_next_event(event);
}


/* EventId : 0x04 */
static unsigned char *st_leave_pointer_event_handler(struct fts_ts_info *info,
			unsigned char *event)
{
	TS_LOG_DEBUG("%s: Received event 0x%02x\n", __func__, event[0]);

	unsigned char touchId, touchcount;
#ifdef CONFIG_JANUARY_BOOSTER
	int x, y;

	x = (event[2] << 4) | (event[4] & 0xF0) >> 4;
	y = (event[3] << 4) | (event[4] & 0x0F);

	if (x == X_AXIS_MAX)
		x--;
	if (y == Y_AXIS_MAX)
		y--;
#endif

	touchId = event[1] & 0x0F;
	touchcount = (event[1] & 0xF0) >> 4;

	st_set_event_to_fingers(info, 0, 0, 0, 0, 0xEFFE, 1, touchId, touchcount);

	return st_next_event(event);
}

/* EventId : 0x05 */
#define st_motion_pointer_event_handler st_enter_pointer_event_handler

/* EventId : 0x07 */
static unsigned char *st_hover_enter_pointer_event_handler(
			struct fts_ts_info *info, unsigned char *event)
{
	TS_LOG_DEBUG("%s: Received event 0x%02x\n", __func__, event[0]);
#ifdef HOVER_ENTER_POINTER_EVENT
	unsigned char touchId;
	int x, y, z;

	touchId = event[1] & 0x0F;

	__set_bit(touchId, &info->touch_id);

	x = (event[2] << 4) | (event[4] & 0xF0) >> 4;
	y = (event[3] << 4) | (event[4] & 0x0F);
#define HOVER_ENTER_Z_VALUE 0
	z = HOVER_ENTER_Z_VALUE;

	if (x == X_AXIS_MAX)
		x--;

	if (y == Y_AXIS_MAX)
		y--;

	input_mt_slot(info->input_dev, touchId);
	input_report_abs(info->input_dev, ABS_MT_TRACKING_ID, touchId);
	input_report_abs(info->input_dev, ABS_MT_POSITION_X, x);
	input_report_abs(info->input_dev, ABS_MT_POSITION_Y, y);
	input_report_abs(info->input_dev, ABS_MT_TOUCH_MAJOR, z);
	input_report_abs(info->input_dev, ABS_MT_TOUCH_MINOR, z);
	input_report_abs(info->input_dev, ABS_MT_PRESSURE, z);
#endif
	return st_next_event(event);
}


/* EventId : 0x08 */
#define st_hover_leave_pointer_event_handler   st_leave_pointer_event_handler

/* EventId : 0x09 */
#define st_hover_motion_pointer_event_handler st_hover_enter_pointer_event_handler


/* EventId : 0x0B */
static unsigned char *st_proximity_enter_event_handler(
			struct fts_ts_info *info, unsigned char *event)
{
	TS_LOG_DEBUG("%s: Received event 0x%02x\n", __func__, event[0]);
#ifdef PROXIMITY_ENTER_EVENT
	unsigned char touchId;
	int x, y, z;

	touchId = event[1] & 0x0F;

	__set_bit(touchId, &info->touch_id);

	x = X_AXIS_MAX / 2;
	y = Y_AXIS_MAX / 2;
#define PROXIMITY_ENTER_Z_VALUE 0
	z = PROXIMITY_ENTER_Z_VALUE;

	input_mt_slot(info->input_dev, touchId);
	input_report_abs(info->input_dev, ABS_MT_TRACKING_ID, touchId);
	input_report_abs(info->input_dev, ABS_MT_POSITION_X, x);
	input_report_abs(info->input_dev, ABS_MT_POSITION_Y, y);
	input_report_abs(info->input_dev, ABS_MT_TOUCH_MAJOR, z);
	input_report_abs(info->input_dev, ABS_MT_TOUCH_MINOR, z);
	input_report_abs(info->input_dev, ABS_MT_PRESSURE, z);
#endif
	return st_next_event(event);
}


/* EventId : 0x0C */
#define st_proximity_leave_event_handler st_leave_pointer_event_handler

/* EventId : 0x0E */
static unsigned char *st_button_status_event_handler(struct fts_ts_info *info,
			unsigned char *event)
{
#ifdef BUTTON_STATUS_EVENT
	int i;
	unsigned int buttons, changed;

	/* get current buttons status */
	buttons = event[1] | (event[2] << 8);

	/* check what is changed */
	changed = buttons ^ info->buttons;

	for (i = 0; i < 16; i++)
		if (changed & (1 << i))
			input_report_key(info->input_dev,
				BTN_0 + i,
				(!(info->buttons & (1 << i))));

	/* save current button status */
	info->buttons = buttons;
#endif
	TS_LOG_DEBUG("Event 0x%02x -  SS = 0x%02x, MS = 0x%02x\n",
				event[0], event[1], event[2]);
	return st_next_event(event);
}

/* EventId : 0x0F */
static unsigned char *st_error_event_handler(struct fts_ts_info *info,
			unsigned char *event)
{
	int error,i;
	TS_LOG_INFO("%s: Received event 0x%02x\n", __func__, event[0]);

	if(event[1] == 0x0a){
		st_chip_powercycle(info);
#if 0
		//before reset clear all slot
		for (i = 0; i < TOUCH_ID_MAX; i++){
			input_mt_slot(info->input_dev, i);
			input_mt_report_slot_state(info->input_dev,
					(i < FINGER_MAX) ? MT_TOOL_FINGER : MT_TOOL_PEN, 0);
		}
		input_sync(info->input_dev);
#endif
		error = st_systemreset(info);
		st_wait_controller_ready(info);
		error += st_command(info, FORCECALIBRATION);
		mdelay(5);
		error += st_command(info, SENSEON);
		mdelay(5);
#ifdef PHONE_KEY
		error += st_command(info, KEYON);
#endif
		error += st_command(info, FLUSHBUFFER);
		mdelay(5);
		st_interrupt_set(info, INT_ENABLE);		
		if (error){
			TS_LOG_ERR("%s: Cannot reset the device----------\n", __func__);
		}
		if(event[2] >= 0x80){ 
			TS_LOG_INFO("ESD or Low battery at gesture mode recovery \n");
			st_set_gesture(info);
			error = st_command(info, ENTER_GESTURE_MODE);
			//enable_irq_wake(info->client->irq);
			st_set_sensor_mode(info, MODE_GESTURE);
			info->gesture_enable = 1;				
		}
	}
	return st_next_event(event);
}

/* EventId : 0x10 */
static unsigned char *st_controller_ready_event_handler(
			struct fts_ts_info *info, unsigned char *event)
{
	TS_LOG_INFO("%s: Received event 0x%02x\n", __func__, event[0]);
#ifdef CONTROLLER_READY_EVENT
	info->touch_id = 0;
	info->buttons = 0;
	input_sync(info->input_dev);
#endif
	return st_next_event(event);
}

/* EventId : 0x16 */
static unsigned char *st_status_event_handler(
			struct fts_ts_info *info, unsigned char *event)
{
	TS_LOG_DEBUG("%s: Received event 0x%02x\n", __func__, event[1]);
#ifdef STATUS_EVENT
	switch (event[1]) {
	case FTS_STATUS_MUTUAL_TUNE:
	case FTS_STATUS_SELF_TUNE:
	case FTS_FORCE_CAL_SELF_MUTUAL:
		//complete(&info->cmd_done);
		break;

	case FTS_FLASH_WRITE_CONFIG:
	case FTS_FLASH_WRITE_COMP_MEMORY:
	case FTS_FORCE_CAL_SELF:
	case FTS_WATER_MODE_ON:
	case FTS_WATER_MODE_OFF:
	default:
		TS_LOG_INFO("Received unhandled status event = 0x%02x\n", event[1]);
		break;
	}
#endif
	return st_next_event(event);
}


/* EventId : 0x20 */
static unsigned char *st_gesture_event_handler(
                     struct fts_ts_info *info, unsigned char *event)
{
	TS_LOG_DEBUG("%s: fts gesture:Event 0x%02x Event1 = 0x%02x, \n", __func__,
                            event[0], event[1]);

#ifdef GESTURE_EVENT
	unsigned char touchId;

	/* always use touchId zero */
	touchId = 0;
	__set_bit(touchId, &info->touch_id);


	switch(event[1]){
	case 0x02:/*add 02-->O*/
		info->gesture_value = UNICODE_O;
		break;
	case 0x03:/*add 03-->C*/
		info->gesture_value = UNICODE_C;
		break;
	case 0x04:/*add 04-->M*/
		info->gesture_value = UNICODE_M;
		break;
	case 0x05:/*add 05-->W*/
		info->gesture_value = UNICODE_W;
		break;
	case 0x06:/*add 06-->E*/
		info->gesture_value = UNICODE_E;
		break;
	case 0x0a:/*add 0a-->UP*/
		info->gesture_value = SWIPE_Y_UP;
		break;
	case 0x09:/*add 09-->down*/
		info->gesture_value = SWIPE_Y_DOWN;
		break;
	case 0x07:/*add 07-->left*/
		info->gesture_value = SWIPE_X_RIGHT;
		break;
	case 0x08:/*add 08-->right*/
		info->gesture_value = SWIPE_X_LEFT;
		break;
	case 0x01:/*add 01-->double click*/
		#ifdef DEBUG
		tp_log("%s: case 0x01--> double click\n", __func__);
		#endif
		info->gesture_value = DOUBLE_TAP;
		break;
	case 0x0D:/*add 06-->V*/
		info->gesture_value = UNICODE_V_DOWN;
		break;
	case 0x0F:/*add 06-->S*/
		info->gesture_value = UNICODE_S;
		break;
	case 0x10:/*add 06-->Z*/
		info->gesture_value = UNICODE_Z;
		break;

	default:
		//info->gesture_value = GESTURE_ERROR;
		return 0;
	}
	//input_report_key(info->input_dev, KEY_GESTURE, 1);
	//input_report_key(info->input_dev, KEY_GESTURE, 0);
	input_sync(info->input_dev);

	/*
	* Done with gesture event, clear bit.
	*/
	__clear_bit(touchId, &info->touch_id);
	TS_LOG_DEBUG("fts gesture:Event 0x%02x Event1 = 0x%02x- ID[%d], \n",
                            event[0], event[1],touchId);
#endif
	return st_next_event(event);
}


/* EventId : 0x23 */
static unsigned char *st_pen_enter_event_handler(
			struct fts_ts_info *info, unsigned char *event)
{
	TS_LOG_DEBUG("%s: Received event 0x%02x\n", __func__, event[0]);
#ifdef PEN_ENTER_EVENT
	unsigned char touchId;
	int x, y, z;
	int eraser, barrel;

	/* always use last position as touchId */
	touchId = TOUCH_ID_MAX;

	__set_bit(touchId, &info->touch_id);

	x = (event[2] << 4) | (event[4] & 0xF0) >> 4;
	y = (event[3] << 4) | (event[4] & 0x0F);
	z = (event[5] & 0xFF);

	eraser = (event[1] * 0x80) >> 7;
	barrel = (event[1] * 0x40) >> 6;

	if (x == X_AXIS_MAX)
		x--;

	if (y == Y_AXIS_MAX)
		y--;

	input_mt_slot(info->input_dev, touchId);
	input_report_abs(info->input_dev, ABS_MT_TRACKING_ID, touchId);
	input_report_abs(info->input_dev, ABS_MT_POSITION_X, x);
	input_report_abs(info->input_dev, ABS_MT_POSITION_Y, y);
	input_report_abs(info->input_dev, ABS_MT_TOUCH_MAJOR, z);
	input_report_abs(info->input_dev, ABS_MT_TOUCH_MINOR, z);
	input_report_abs(info->input_dev, ABS_MT_PRESSURE, z);

	input_report_key(info->input_dev, BTN_STYLUS, eraser);
	input_report_key(info->input_dev, BTN_STYLUS2, barrel);
	input_mt_report_slot_state(info->input_dev, MT_TOOL_PEN, 1);
#endif
	return st_next_event(event);
}

/* EventId : 0x24 */
static unsigned char *st_pen_leave_event_handler(
			struct fts_ts_info *info, unsigned char *event)
{
	TS_LOG_DEBUG("%s: Event 0x%02x\n", __func__, event[0]);

#ifdef PEN_LEAVE_EVENT
	unsigned char touchId;

	/* always use last position as touchId */
	touchId = TOUCH_ID_MAX;

	__clear_bit(touchId, &info->touch_id);

	input_report_key(info->input_dev, BTN_STYLUS, 0);
	input_report_key(info->input_dev, BTN_STYLUS2, 0);

	input_mt_slot(info->input_dev, touchId);
	input_report_abs(info->input_dev, ABS_MT_TRACKING_ID, -1);
	TS_LOG_INFO("Event 0x%02x - release ID[%d]\n",
		event[0], touchId);
#endif

	return st_next_event(event);
}

/* EventId : 0x25 */
#define st_pen_motion_event_handler st_pen_enter_event_handler

/*EventId : 0x27*/
static unsigned char *st_orientation_pointer_event_handler(struct fts_ts_info *info,
			unsigned char *event)
{
	int orientation;
	unsigned char touchId;

	TS_LOG_DEBUG("event0 is %d,event1 is %d,event2 is %d,event3 is %d,event4 is %d,event5 is %d,event6 is %d\n",
			event[0],event[1],event[2],event[3],event[4],event[5],event[6]);

	touchId = event[1];
	orientation = event[2];

	info->fingers_info->fingers[touchId].orientation = orientation;
	return st_next_event(event);
}

static int st_interrupt_install(struct fts_ts_info *info)
{
	int i, error = 0;

	info->event_dispatch_table = kzalloc(
		sizeof(event_dispatch_handler_t) * EVENTID_LAST, GFP_KERNEL);

	if (!info->event_dispatch_table) {
		TS_LOG_ERR("OOM allocating event dispatch table\n");
		return -ENOMEM;
	}

	for (i = 0; i < EVENTID_LAST; i++)
		info->event_dispatch_table[i] = st_nop_event_handler;

	install_handler(info, ENTER_POINTER, enter_pointer);
	install_handler(info, LEAVE_POINTER, leave_pointer);
	install_handler(info, MOTION_POINTER, motion_pointer);

	install_handler(info, BUTTON_STATUS, button_status);

	install_handler(info, HOVER_ENTER_POINTER, hover_enter_pointer);
	install_handler(info, HOVER_LEAVE_POINTER, hover_leave_pointer);
	install_handler(info, HOVER_MOTION_POINTER, hover_motion_pointer);

	install_handler(info, PROXIMITY_ENTER, proximity_enter);
	install_handler(info, PROXIMITY_LEAVE, proximity_leave);

	install_handler(info, ERROR, error);
	install_handler(info, CONTROLLER_READY, controller_ready);
	install_handler(info, STATUS, status);

	install_handler(info, GESTURE, gesture);

	install_handler(info, PEN_ENTER, pen_enter);
	install_handler(info, PEN_LEAVE, pen_leave);
	install_handler(info, PEN_MOTION, pen_motion);

	install_handler(info, ORIENTATION, orientation_pointer);

	return error;
}

static int st_system_init(struct fts_ts_info *info)
{
	int error;

	error = st_systemreset(info);
	if (error) {
		TS_LOG_ERR("Cannot reset the device\n");
		return -ENODEV;
	}
	error = st_wait_controller_ready(info);
	if (error) {
		TS_LOG_ERR("Wait controller ready failed\n");
	}

	/* check for chip id */
	error = st_get_fw_version(info);
	if (error) {
		TS_LOG_ERR("Cannot initiliaze, wrong device id\n");
		return -ENODEV;
	}

	return error ? -ENODEV : 0;
}

static int st_init_hw(struct fts_ts_info *info)
{
	int error = 0;

	TS_LOG_INFO("initialize the hardware device\n");

	error += st_command(info, SENSEON);
	mdelay(5);
#ifdef PHONE_KEY
	error += st_command(info, KEYON);
#endif
	//error += st_command(info, FORCECALIBRATION);
	error += st_command(info, FLUSHBUFFER);
	mdelay(5);
	st_interrupt_set(info, INT_ENABLE);

	if (error)
		TS_LOG_ERR("Init (2) error (#errors = %d)\n", error);

	return error ? -ENODEV : 0;
}

static int request_compensation_data(struct fts_ts_info *info,unsigned int compensation_type, unsigned char *data)
{
	unsigned char regAdd[8];
	int error = 0;
	int	retry = 0;
	int	ret = 0;
	
	/* Request Compensation Data*/
	regAdd[0] = 0xB8;
	regAdd[1] = compensation_type;
	regAdd[2] = 0x00;
	error = st_i2c_write(info,regAdd, 3);
	if (error){
		TS_LOG_ERR("FTS %s: I2c WRITE ERROR : Cannot Request Compensation data\n",__func__);
		return -2;
	}
	msleep(10);
	
	/* Read completion event*/
	for (retry = 0; retry <= READ_CNT; retry++) // poll with time-out 3000ms
	{
		regAdd[0] = READ_ONE_EVENT;
		error = st_i2c_read(info, regAdd,1, data, FTS_EVENT_SIZE);
		if (error){
			TS_LOG_ERR("FTS %s: I2c READ ERROR : Cannot read device info\n",__func__);
			return -2;
		}
		printk("FTS %s: %02X %02X %02X %02X %02X %02X %02X %02X\n",__func__,
				data[0], data[1], data[2], data[3],
				data[4], data[5], data[6], data[7]);

		if ((data[0] == EVENTID_COMP_DATA_READ) && (data[1] == compensation_type) && (data[2] == 0x00)){
			break;
		} else {
			msleep(10);
			if(retry == READ_CNT) {
				TS_LOG_ERR("FTS %s : TIMEOUT ,Cannot read completion event\n",__func__);
				ret = -2;
			}
		}
	}	
	TS_LOG_INFO("%s exited \n",__func__);
	return ret;
}

static int st_get_init_status(struct fts_ts_info *info)
{
	unsigned char data[FTS_EVENT_SIZE] = {0};
	unsigned char regAdd[4] = {0xB2, 0x07, 0x29, 0x04};
	unsigned char buff_read[17] = {0};
	unsigned char regAdd1 = 0;
	unsigned char event_id = 0;
	unsigned char flag_id0 = 0;
	unsigned char flag_id1 = 0;
	unsigned char ms_tune_version = 0;
	unsigned char chip_ms_tune_version = 0;
	unsigned char ss_tune_version = 0;
	unsigned char chip_ss_tune_version = 0;
	unsigned char error;
	int retry = 0;
	int	address_offset = 0;
	int start_tx_offset = 0;

	st_interrupt_set(info, INT_DISABLE);
	st_command(info, FLUSHBUFFER);
	
	/********************Reading MS tune version*****************************/
	error = st_i2c_write(info, regAdd, sizeof(regAdd)); //READ Mutual Tune version
	if(error) {
		TS_LOG_ERR("Cannot write register for reading MS tune version\n");
		return -2;
	}
	for(retry = 0; retry <= RETRY_CNT_MS_TUNE; retry++) {
		regAdd1 = READ_ONE_EVENT;
		error = st_i2c_read(info, &regAdd1,sizeof(regAdd), data, FTS_EVENT_SIZE);
		if (error) {
			TS_LOG_ERR("Cannot read device info\n");
			return -2;
		}

		TS_LOG_DEBUG("FTS fts status event(Mutual Tune version): %02X %02X %02X %02X %02X %02X %02X %02X\n",
			data[0], data[1], data[2], data[3],data[4], data[5], data[6], data[7]);

		event_id = data[0];
		flag_id0 = data[1];
		flag_id1 = data[2]; 
		if (event_id == 0x12 && flag_id0 == 0x07 && flag_id1 == 0x29){
			ms_tune_version = data[3];
			retry = 0;	//this set is important for the timeout check and validate the case that you will found the correct event at the 40th attempt
			break;
		}else{
			msleep(10);
		}
	}

	if (retry > RETRY_CNT_MS_TUNE) {
		TS_LOG_ERR("FTS Timeout occurred while reading ms_tune_version!");
		return -2;
	}

	/* Request Compensation Data*/
	error = request_compensation_data(info,0x02,data);
	if(error != 0){
		TS_LOG_ERR("%s : Call failed ,Cannot read Compensation event \n",__func__);
		return -2;
	}	

	/* Read Offset Address for Compensation*/
	regAdd[0] = 0xD0;
	regAdd[1] = 0x00;
	regAdd[2] = 0x50;
	error = st_i2c_read(info,regAdd, 3, &buff_read[0], 4);
	if(error){
		TS_LOG_ERR("Cannot read Offset Address for Compensation 1\n");
		return -2;
	}
	TS_LOG_INFO("FTS Read Offset Address for Compensation1: %02X %02X %02X %02X\n",
				buff_read[0], buff_read[1], buff_read[2],buff_read[3]);	
	start_tx_offset = ((buff_read[2]<<8) |buff_read[1]);//if first byte is dummy byte 
	address_offset  = start_tx_offset + 0x10;

	/* Read Offset Address for f_cnt and s_cnt*/
	regAdd[0] = 0xD0;
	regAdd[1] = (unsigned char)((start_tx_offset & 0xFF00) >> 8);		
	regAdd[2] = (unsigned char)(start_tx_offset & 0xFF);
	error = st_i2c_read(info,regAdd, 3, &buff_read[0], 17);
	if(error){
		TS_LOG_ERR("Cannot read Offset Address for f_cnt and s_cnt 1\n");
		return -2;
	}
	
	TS_LOG_INFO("FTS  Read Offset Address for f_cnt and s_cnt: %02X %02X %02X %02X  %02X %02X %02X %02X\n",buff_read[0], buff_read[1], buff_read[2],buff_read[3],buff_read[4], buff_read[5], buff_read[6],buff_read[7]);	

	chip_ms_tune_version = buff_read[9];
	/********************Reading MS tune version-ENDs*****************************/
	if(chip_ms_tune_version == ms_tune_version ){
		tune_version_same = 0x1;
		TS_LOG_INFO("fts MS Tune version is same\n");
	}else{
		tune_version_same = 0x0;
		TS_LOG_INFO("fts MS Tune version not the same\n");
		goto exit_init ;//return error if ms tune version no matches
	}
	
	/********************Reading SS tune version*****************************/
	//regAdd[4] = { 0xB2, 0x07, 0x4E, 0x04};
	regAdd[0] = 0xB2;
	regAdd[1] = 0x07;
	regAdd[2] = 0x4E;
	regAdd[3] = 0x04;
	error = st_i2c_write(info, regAdd, sizeof(regAdd)); //READ Self Tune version
	if(error){
		TS_LOG_ERR("Cannot write register for reading Self Tune version\n");
		return -2;
	}
	for(retry = 0; retry <= RETRY_CNT_SS_TUNE; retry++) {
		regAdd1 = READ_ONE_EVENT;
		error = st_i2c_read(info, &regAdd1,sizeof(regAdd), data, FTS_EVENT_SIZE);
		if (error) {
			TS_LOG_ERR("Cannot read device info\n");
			return -2;
		}

		TS_LOG_DEBUG("FTS fts status event(Self Tune version): %02X %02X %02X %02X %02X %02X %02X %02X\n",
			data[0], data[1], data[2], data[3],data[4], data[5], data[6], data[7]);

		event_id = data[0];
		flag_id0 = data[1];
		flag_id1 = data[2];
		if (event_id == 0x12 && flag_id0 == 0x07 && flag_id1 == 0x4e){
			ss_tune_version = data[3];
			retry = 0;	//this set is important for the timeout check and validate the case that you will found the correct event at the 40th attempt
			break;
		}else{
			msleep(10);
		}
	}

	if (retry > RETRY_CNT_MS_TUNE) {
		printk("FTS  Timeout occurred while reading ss_tune_version!");
		return -2;
	}

	/* Request Compensation Data*/
	error = request_compensation_data(info,0x20,data);
	if(error != 0){
		TS_LOG_ERR("%s : Call failed ,Cannot read Compensation event \n",__func__);
		return -2;
	}

	/* Read Offset Address for Compensation*/
	regAdd[0] = 0xD0;
	regAdd[1] = 0x00;
	regAdd[2] = 0x50;
	error = st_i2c_read(info,regAdd, 3, &buff_read[0], 4);
	if(error) {
		TS_LOG_ERR("Cannot read Offset Address for Compensation 2\n");
		return -2;
	}
	start_tx_offset = ((buff_read[2]<<8) |buff_read[1]);//if first byte is dummy byte 
	address_offset  = start_tx_offset + 0x10;

	/* Read Offset Address for f_cnt and s_cnt*/
	regAdd[0] = 0xD0;
	regAdd[1] = (unsigned char)((start_tx_offset & 0xFF00) >> 8);		
	regAdd[2] = (unsigned char)(start_tx_offset & 0xFF);
	error = st_i2c_read(info,regAdd, 3, &buff_read[0], 17);
	if(error){
		TS_LOG_ERR("Cannot read Offset Address for f_cnt and s_cnt 2\n");
		return -2;
	}
	TS_LOG_INFO("FTS Read Offset Address for f_cnt and s_cnt: %02X %02X %02X %02X  %02X %02X %02X %02X\n",buff_read[0], buff_read[1], buff_read[2],buff_read[3],buff_read[4], buff_read[5], buff_read[6],buff_read[7]);	

	chip_ss_tune_version = buff_read[9];
	/********************Reading SS tune version-ENDs*****************************/
	if(chip_ss_tune_version == ss_tune_version ){
		tune_version_same = 0x1;
		TS_LOG_INFO("fts SS Tune version is same\n");
	}else{
		tune_version_same = 0x0;
		TS_LOG_INFO("fts SS Tune version not the same\n");
	}
exit_init:
	st_interrupt_set(info, INT_ENABLE);

	if(tune_version_same == 0){	
		TS_LOG_ERR("fts initialization status error\n");
		return -1;
	}else {
		TS_LOG_INFO("fts initialization status OK\n");
		return 0;
	}
}

static int st_init_chip(void)
{
	int retval=NO_ERR;

	TS_LOG_INFO("%s: +\n", __func__);

#if defined (CONFIG_TEE_TUI)
	strncpy(tui_st_data.device_name, "st", strlen("st"));
	tui_st_data.device_name[strlen("st")] = '\0';
	if(st_info) {
		if(st_info->dev_data) {
			st_info->dev_data->tui_data = &tui_st_data;
		}
	}
#endif

	st_i2c_cmd_sys_create(st_info);
	/* we must wait for a while to get fw version and config id after chip reset */
	mdelay(3);

	retval = st_system_init(st_info);
	if (retval){
		TS_LOG_ERR("st init flash reload fail\n");
		goto out;
	}

	retval = st_init_hw(st_info);
	if (retval) {
		TS_LOG_ERR("Cannot initialize the hardware device\n");
		retval = -ENODEV;
		goto out;
	}

	TS_LOG_INFO("%s: -\n", __func__);
	return 0;

out:
	TS_LOG_ERR("st init chip fail\n");
	return retval;
}

static int st_input_config(struct input_dev *input_dev)
{
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);

	set_bit(TS_DOUBLE_CLICK, input_dev->keybit);
	set_bit(TS_SLIDE_L2R, input_dev->keybit);
	set_bit(TS_SLIDE_R2L, input_dev->keybit);
	set_bit(TS_SLIDE_T2B, input_dev->keybit);
	set_bit(TS_SLIDE_B2T, input_dev->keybit);
	set_bit(TS_CIRCLE_SLIDE, input_dev->keybit);
	set_bit(TS_LETTER_c, input_dev->keybit);
	set_bit(TS_LETTER_e, input_dev->keybit);
	set_bit(TS_LETTER_m, input_dev->keybit);
	set_bit(TS_LETTER_w, input_dev->keybit);
	set_bit(TS_PALM_COVERED, input_dev->keybit);

	set_bit(TS_TOUCHPLUS_KEY0, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY1, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY2, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY3, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY4, input_dev->keybit);

#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
#endif
#if ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 100, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MINOR, 0, 100, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_ORIENTATION, 0, 255, 0, 0);
#else
	input_set_abs_params(input_dev, ABS_MT_DISTANCE, 0, 100, 0, 0);
#endif
	input_set_abs_params(input_dev, ABS_X,
			     0, st_info->sensor_max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_Y,
			     0, st_info->sensor_max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 15, 0, 0);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, st_info->sensor_max_x_mt, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, st_info->sensor_max_y_mt, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
#ifdef REPORT_2D_W
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, MAX_ABS_MT_TOUCH_MAJOR, 0, 0);
#endif
#ifdef TYPE_B_PROTOCOL
#ifdef KERNEL_ABOVE_3_7
	/* input_mt_init_slots now has a "flags" parameter */
	input_mt_init_slots(input_dev, st_info->num_of_fingers, INPUT_MT_DIRECT);
#else
	input_mt_init_slots(input_dev, st_info->num_of_fingers);
#endif
#endif
	st_info->input_dev = input_dev;

	return NO_ERR;
}

static int st_irq_top_half(struct ts_cmd_node *cmd)
{
	cmd->command = TS_INT_PROCESS;
	TS_LOG_DEBUG("st irq top half called\n");
	return NO_ERR;
}

/*
 * This handler is called each time there is at least
 * one new event in the FIFO
 */
static void st_event_handle(struct fts_ts_info *fts_info)
{
	struct fts_ts_info *info = fts_info;
	int error, error1;
	int left_events;
	unsigned char regAdd;
	unsigned char data[FTS_EVENT_SIZE * (FTS_FIFO_MAX+1)] = {0};
	unsigned char *event = NULL;
	unsigned char eventId;
	event_dispatch_handler_t event_handler;

	/*
	 * to avoid reading all FIFO, we read the first event and
	 * then check how many events left in the FIFO
	 */

	regAdd = READ_ONE_EVENT;
	error = st_i2c_read(info, &regAdd,
			sizeof(regAdd), data, FTS_EVENT_SIZE);

	if (!error) {

		left_events = data[7] & 0x1F;
		if ((left_events > 0) && (left_events < FTS_FIFO_MAX)) {
			/*
			 * Read remaining events.
			 */
			regAdd = READ_ALL_EVENT;
			error1 = st_i2c_read(info, &regAdd, sizeof(regAdd),
						&data[FTS_EVENT_SIZE],
						(left_events+1) * FTS_EVENT_SIZE);

			/*
			 * Got an error reading remining events,
			 * process at least * the first one that was
			 * raeding fine.
			 */
			if (error1)
				data[7] &= 0xE0;
		}

		/* At least one event is available */
		event = data;
		do {
			eventId = *event;
			event_handler = info->event_dispatch_table[eventId];

			if(eventId < EVENTID_LAST) {
				event = event_handler(info, (event));
			} else {
				event = st_next_event(event);
			}
		} while (event);
	}
}

static int st_irq_bottom_half(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	/*
	 * to avoid reading all FIFO, we read the first event and
	 * then check how many events left in the FIFO
	 */
	TS_LOG_DEBUG("%s: +\n", __func__);
	st_info->fingers_info = &out_cmd->cmd_param.pub_params.algo_param.info;
	
	out_cmd->command = TS_INPUT_ALGO;
	out_cmd->cmd_param.pub_params.algo_param.algo_order =
		st_info->dev_data->algo_id;

	temp_finger_status = 0;

	st_event_handle(st_info);

	if((temp_finger_status != pre_finger_status)
		&& ((temp_finger_status & pre_finger_status) != temp_finger_status)) {
		st_get_roi_data(st_info);
	}

	if(NULL == st_info->fingers_info){
		TS_LOG_ERR("fingers_info is NULL pointer\n");
		return NO_ERR;
	}

	if(st_info->fingers_info->cur_finger_number == 0)
		pre_finger_status = 0;
	else
		pre_finger_status = temp_finger_status;

	TS_LOG_DEBUG("%s: -\n", __func__);
	return NO_ERR;
}

static int st_fw_update_boot(char *file_name)
{
	int retval = 0;
	int retval1 = 0;
	int ret = 0;
	struct fts_ts_info *info;
	int crc_status = 0;

	info = st_info;

	st_systemreset(info);
	st_wait_controller_ready(info);
	mdelay(5);

	// check CRC status
	if((cx_crc_check(info)) == -2){
		TS_LOG_INFO("%s: CRC Error 128 K firmware update \n", __func__);
		crc_status = 1;
	} else {
		crc_status = 0;
		TS_LOG_INFO("%s:NO CRC Error 124 K firmware update \n", __func__);
	}
	/*check firmware*/
	retval = st_fw_upgrade(info, 0, 0, crc_status, file_name);
	if(retval){
		TS_LOG_ERR("%s: firmware update failed and retry!\n", __func__);
		st_chip_powercycle(info);	 // power reset
		retval1 = st_fw_upgrade(info, 0,0,crc_status, file_name);
		if(retval1){
		   TS_LOG_ERR("%s: firmware update failed again!\n", __func__);
		   return -1;
		}
	}

	ret = st_get_init_status(info); // return value 0 means initialization status correct
	if(ret != 0 && ret != -2){	// initialization status not correct or after FW update, do initialization.
		st_chip_initialization(info);
	} else {
		ret = st_init_hw(info);
		if (ret){
			TS_LOG_ERR("Cannot initialize the hardware device\n");
		}
	}

	return 0;
}

static void st_goto_sleep_mode(void)
{
	st_interrupt_set(st_info, INT_DISABLE);
	/* Read out device mode, used when resuming device */
	//fts_get_mode(info);

	/* suspend the device and flush the event FIFO */
	TS_LOG_INFO("%s: suspend send command sleep \n", __func__);
	st_command(st_info, SENSEOFF);
#ifdef PHONE_KEY
	st_command(st_info, KEYOFF);
#endif
	st_command(st_info, FLUSHBUFFER);
}

static int st_fw_update_sd(void)
{
	TS_LOG_INFO("%s +\n", __func__);

	st_info->fw_force = 1;
	st_fw_update_boot(NULL);
	st_init_hw(st_info);
	st_info->fw_force = 0;

	return 0;
}

static int st_chip_get_info(struct ts_chip_info_param *info)
{
	int retval = NO_ERR;

	snprintf(info->ic_vendor, sizeof(info->ic_vendor), "St");
	snprintf(info->fw_vendor, sizeof(info->fw_vendor),
		 "configid:%x", st_info->config_id);

	return retval;
}

static int st_set_info_flag(struct ts_data *info)
{
	TS_LOG_INFO("%s +\n", __func__);
	TS_LOG_INFO("%s -\n", __func__);
	return NO_ERR;
}

/*  do some things before power off.
*/
static int st_before_suspend(void)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s +\n", __func__);
	TS_LOG_INFO("%s -\n", __func__);
	return retval;
}

static int st_suspend(void)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s +\n", __func__);
	switch (st_info->dev_data->easy_wakeup_info.sleep_mode) {
	case TS_POWER_OFF_MODE:
		if (!g_tp_power_ctrl)
			st_power_off(st_info->dev_data);
		else
			st_goto_sleep_mode();
		break;
	/*for gesture wake up mode suspend.*/
	case TS_GESTURE_MODE:
		break;
	default:
		TS_LOG_ERR("no suspend mode\n");
		retval = -EINVAL;
		break;
	}
	TS_LOG_INFO("%s -\n", __func__);
	return retval;
}

/*    do not add time-costly function here.
*/
static int st_resume(void)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s +\n", __func__);
	switch (st_info->dev_data->easy_wakeup_info.sleep_mode) {
	case TS_POWER_OFF_MODE:
		if(!g_tp_power_ctrl){
			st_power_on(st_info->dev_data);
		}
		break;
	case TS_GESTURE_MODE:
		break;
	default:
		TS_LOG_ERR("no resume mode\n");
		return -EINVAL;
	}
	TS_LOG_INFO("%s -\n", __func__);
	return retval;
}

/*  do some things after power on. */
static int st_after_resume(void *feature_info)
{
	int retval = NO_ERR;
	unsigned char* ret = NULL;
	struct ts_feature_info *info = &g_ts_data.feature_info;

	TS_LOG_INFO("%s +\n", __func__);

	st_hw_reset(st_info->dev_data);

	retval = st_system_init(st_info);
	if (retval){
		TS_LOG_ERR("st init flash reload fail\n");
		goto out;
	}

	retval = st_init_hw(st_info);
	if (retval) {
		TS_LOG_ERR("Cannot initialize the hardware device\n");
		retval = -ENODEV;
		goto out;
	}

	retval = st_set_glove_switch(info->glove_info.glove_switch);
	if (retval) {
		TS_LOG_ERR("st set glove switch(%d) failed: %d\n",
			info->glove_info.glove_switch, retval);
	}

	retval = st_set_holster_switch(info->holster_info.holster_switch);
	if (retval < 0) {
		TS_LOG_ERR("set holster switch(%d), failed: %d\n",
			   info->holster_info.holster_switch, retval);
	}

	TS_LOG_INFO("%s -\n", __func__);
out:
	return retval;
}

static int st_get_rawdata(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd)
{
	int retval = 0;

	st_get_rawdata_test(info, &g_ts_data, st_raw_limit_buf);

	retval = st_system_init(st_info);
	if (retval){
		TS_LOG_ERR("st init flash reload fail\n");
		return retval;
	}

	retval = st_init_hw(st_info);
	if (retval) {
		TS_LOG_ERR("Cannot initialize the hardware device\n");
		return retval;
	}

	return NO_ERR;
}

static int st_set_glove_switch(u8 glove_switch)
{
	int ret = NO_ERR;
	unsigned char regAdd[2]= {0};

	if (GLOVE_SWITCH_ON == glove_switch) {
		regAdd[0] = 0xC1;
		regAdd[1] = 0x01;
	} else {
		regAdd[0] = 0xC2;
		regAdd[1] = 0x01;
	}

	ret = st_i2c_write(st_info, regAdd, sizeof(regAdd));
	return ret;
}

static int st_glove_switch(struct ts_glove_info *info)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s +\n", __func__);
	if (!info) {
		TS_LOG_ERR("st_glove_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}
	switch (info->op_action) {
		case TS_ACTION_READ:
			TS_LOG_INFO("read_glove_switch=%d, 1:on 0:off\n",info->glove_switch);
			break;
		case TS_ACTION_WRITE:
			TS_LOG_INFO("write_glove_switch=%d\n",info->glove_switch);
			if ((GLOVE_SWITCH_ON != info->glove_switch)
				&& (GLOVE_SWITCH_OFF != info->glove_switch)) {
				TS_LOG_ERR("write wrong state: switch = %d\n", info->glove_switch);
				retval = -EFAULT;
				break;
			}
			retval = st_set_glove_switch(info->glove_switch);
			if (retval < 0) {
				TS_LOG_ERR("set glove switch(%d), failed : %d", info->glove_switch,
					   retval);
			}
			break;
		default:
			TS_LOG_ERR("invalid switch status: %d", info->glove_switch);
			retval = -EINVAL;
			break;
	}
	TS_LOG_INFO("%s -\n", __func__);
	return retval;
}

static void st_shutdown(void)
{
	TS_LOG_INFO("%s +\n", __func__);
	st_power_off(st_info->dev_data);
	TS_LOG_INFO("%s -\n", __func__);
	return;
}

static int st_set_holster_switch(u8 holster_switch)
{
	int ret = NO_ERR;
	unsigned char regAdd[2] = {0};

	if (HOLSTER_SWITCH_ON == holster_switch) {
		regAdd[0] = 0xC1;
		regAdd[1] = 0x04;
	} else {
		regAdd[0] = 0xC2;
		regAdd[1] = 0x04;
	}

	ret = st_i2c_write(st_info, regAdd, sizeof(regAdd));
	return ret;

}

static int st_holster_switch(struct ts_holster_info *info)
{
	int retval = NO_ERR;

	TS_LOG_INFO("holster switch(%d) action: %d\n", info->holster_switch, info->op_action);
	if (!info) {
		TS_LOG_ERR("synaptics_holster_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}

	switch (info->op_action) {
		case TS_ACTION_WRITE:
			retval = st_set_holster_switch(info->holster_switch);
			if (retval < 0) {
				TS_LOG_ERR("set holster switch(%d), failed: %d\n",
					   info->holster_switch, retval);
			}
			break;
		case TS_ACTION_READ:
			TS_LOG_INFO
				("invalid holster switch(%d) action: TS_ACTION_READ\n",
				 info->holster_switch);
			break;
		default:
			TS_LOG_INFO("invalid holster switch(%d) action: %d\n", info->holster_switch, info->op_action);
			retval = -EINVAL;
			break;
	}
	return retval;
}

static u16 st_roi_addr_get(void)
{
	int ret = 0;
	struct fts_ts_info * info = st_info;
	unsigned char regAdd[4] = {0xD0, 0x00, 0x5A, 0x03};
	unsigned char data[3] = {0};

	ret = st_i2c_read(info, regAdd, sizeof(regAdd)-1, data, 3);
	if(ret){
		TS_LOG_ERR("Cannot read Offset Address for f_cnt and s_cnt 2\n");
		return -1;
	}

	TS_LOG_DEBUG("roi addr is 0x%x, 0x%x ,0x%x, 0x%x\n", data[1] | (data[2] << 8), data[0], data[1], data[2]);
	return ((data[2] << 8) | data[1]);
}

static int st_roi_switch(struct ts_roi_info *info)
{
	return 0;
}

static unsigned char* st_roi_rawdata(void)
{
	return roi_data;
}

static void st_get_roi_data(struct fts_ts_info *info)
{
	int ret = 0;
	unsigned char regAdd[4] = {0};
	unsigned char data[99] = {0};
	u16 roi_addr = 0;

	roi_addr = st_roi_addr_get();

	regAdd[0] = 0xD0;
	regAdd[1] = (roi_addr & 0xFF00) >> 8;
	regAdd[2] = (roi_addr & 0xFF);
	regAdd[3] = 0x63;

	TS_LOG_DEBUG("roi_addr:0x%x, %x %x %x %x\n", roi_addr, regAdd[0], regAdd[1], regAdd[2], regAdd[3]);
	ret = st_i2c_read(info, regAdd, sizeof(regAdd) - 1, data, sizeof(data)/sizeof(data[0]));
	if(ret){
		TS_LOG_ERR("Cannot read roi data\n");
	}

	memcpy(&roi_data[4], &data[1], sizeof(data)/sizeof(data[0]) -1);
	return;
}

static int st_palm_switch(struct ts_palm_info *info)
{
	int retval = NO_ERR;

	TS_LOG_DEBUG("%s +\n", __func__);
	if (!info) {
		TS_LOG_ERR("synaptics_palm_set_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}
	switch (info->op_action) {
	case TS_ACTION_READ:
		break;
	case TS_ACTION_WRITE:
		break;
	default:
		TS_LOG_ERR("invalid switch status: %d\n", info->palm_switch);
		retval = -EINVAL;
		break;
	}
	TS_LOG_DEBUG("%s -\n", __func__);
	return retval;
}

static int st_regs_operate(struct ts_regs_info *info)
{
	int retval = NO_ERR;
	TS_LOG_INFO("%s +\n", __func__);

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		break;
	case TS_ACTION_READ:
		break;
	default:
		TS_LOG_ERR("reg operate default invalid action %d\n", info->op_action);
		retval = -EINVAL;
		break;
	}

	TS_LOG_INFO("%s -\n", __func__);
	return retval;
}

static int st_get_capacitance_test_type(struct ts_test_type_info *info)
{
	int error = NO_ERR;
	struct fts_ts_info *data = st_info;

	if (!info) {
		TS_LOG_ERR("%s:info=%ld\n", __func__, PTR_ERR(info));
		error = -ENOMEM;
		return error;
	}
	switch (info->op_action) {
	case TS_ACTION_READ:
		memcpy(info->tp_test_type, data->dev_data->tp_test_type,
		       TS_CAP_TEST_TYPE_LEN);
		TS_LOG_INFO("read_chip_get_test_type=%s, \n",
			    info->tp_test_type);
		break;
	case TS_ACTION_WRITE:
		break;
	default:
		TS_LOG_ERR("invalid status: %s", info->tp_test_type);
		error = -EINVAL;
		break;
	}
	return error;
}

static int st_calibrate(void)
{
	int ret =0;

	ret = st_chip_initialization(st_info);
	if (ret) {
		TS_LOG_ERR("%s error\n", __func__);
	}

	return ret;
}
