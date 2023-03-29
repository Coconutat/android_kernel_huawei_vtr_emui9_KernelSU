/*
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 6032 $
 * $Date: 2016-08-22 11:26:22 +0800 (週一, 22 八月 2016) $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <linux/unistd.h>

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <../../huawei_ts_kit.h>
#include <linux/regulator/consumer.h>
#include <huawei_platform/log/log_jank.h>
#include "../../huawei_ts_kit_algo.h"
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif

#include "NVTtouch_207.h"


char nvt_hybrid_vendor_name[8]={"novatek"};
char nvt_hybrid_product_id[NVT_HYBRID_PROJECT_ID_LEN+1]={"999999999"};

static int32_t nvt_hybrid_rawdata_diff[40 * 40] ;
static int32_t nvt_hybrid_rawdata_fwMutual[40 * 40] ;
struct nvt_hybrid_ts_data *nvt_hybrid_ts;
static DEFINE_MUTEX(ts_power_gpio_sem);
static int ts_power_gpio_ref = 0;

#if NVT_HYBRID_TOUCH_PROC
extern int32_t nvt_hybrid_flash_proc_init(void);
#endif

#if NVT_HYBRID_TOUCH_EXT_PROC
extern int32_t nvt_hybrid_extra_proc_init(void);
#endif

#if NVT_HYBRID_TOUCH_MP
extern int32_t nvt_hybrid_selftest(struct ts_rawdata_info * info);
extern int32_t nvt_ctrlram_selftest(struct ts_rawdata_info * info);//mp selftest for 206
#endif

extern int32_t NVT_Hybrid_Init_BootLoader(void);
extern int32_t nvt_bybrid_fw_update_boot(char *file_name);
extern int32_t nvt_hybrid_fw_update_sd(void);
extern int32_t nvt_hybrid_get_rawdata(struct ts_rawdata_info *info);
extern int32_t nvt_hybrid_read_diff(int32_t *xdata);
extern int32_t nvt_hybrid_read_raw(int32_t *xdata);
extern int32_t nvt_ctrlram_read_diff(int32_t *xdata);//mp selftest for 206
extern int32_t nvt_ctrlram_read_raw(int32_t *xdata);//mp selftest for 206
extern int8_t nvt_hybrid_get_fw_info(void);
extern int32_t nvt_206_read_tp_color(void);

#ifdef ROI
static u8 nvt_hybrid_roi_switch = 0;
static u8 nvt_hybrid_pre_finger_status = 0;
static unsigned char nvt_hybrid_roi_data[ROI_DATA_READ_LENGTH+1] = {0};
#endif

#if NVT_HYBRID_TOUCH_KEY_NUM > 0
const uint16_t nvt_hybrid_touch_key_array[NVT_HYBRID_TOUCH_KEY_NUM] = {
    KEY_BACK,
    KEY_HOME,
    KEY_MENU
};
#endif

/*******************************************************
Description:
	Novatek touchscreen i2c read function.

return:
	Executive outcomes. 2---succeed. -5---I/O error
*******************************************************/
int32_t nvt_hybrid_ts_i2c_read(struct i2c_client *client, uint16_t i2c_addr, uint8_t *buf, uint16_t len)
{
	int ret = -1;
	u16 tmp_addr = 0;

	if (!client) {
		TS_LOG_ERR("%s:client is Null\n", __func__);
		return -ENOMEM;
	}

	if (!buf) {
		TS_LOG_ERR("%s:buf is Null\n", __func__);
		return -ENOMEM;
	}

	TS_LOG_DEBUG("%s: i2c_addr=0x%02X, len=%d\n", __func__, (uint8_t)i2c_addr, (uint8_t)len);

	if (!nvt_hybrid_ts->chip_data->ts_platform_data->bops->bus_read) {
		TS_LOG_ERR("%s: error, invalid bus_read\n", __func__);
		ret = -EIO;
		goto i2c_err;
	}

	mutex_lock(&nvt_hybrid_ts->i2c_mutex);
	tmp_addr = nvt_hybrid_ts->client->addr;
	nvt_hybrid_ts->client->addr = i2c_addr;

	ret = nvt_hybrid_ts->chip_data->ts_platform_data->bops->bus_read(&buf[0], 1, &buf[1], (len - 1));
	if (ret < 0)
		TS_LOG_ERR("%s: error, bus_read fail, ret=%d\n", __func__, ret);

	nvt_hybrid_ts->client->addr = tmp_addr;
	mutex_unlock(&nvt_hybrid_ts->i2c_mutex);

i2c_err:
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen i2c dummy read function.

return:
	Executive outcomes. 1---succeed. -5---I/O error
*******************************************************/
int32_t nvt_hybrid_ts_i2c_dummy_read(struct i2c_client *client, uint16_t i2c_addr)
{
	int ret = -1;
	u16 tmp_addr = 0;
	uint8_t buf[8] = {0};

	if (!client) {
		TS_LOG_ERR("%s:client is Null\n", __func__);
		return -ENOMEM;
	}

	TS_LOG_INFO("%s: i2c_addr=0x%02X\n", __func__, (uint8_t)i2c_addr);

	if (!nvt_hybrid_ts->chip_data->ts_platform_data->bops->bus_read) {
		TS_LOG_ERR("%s: error, invalid bus_read\n", __func__);
		ret = -EIO;
		goto i2c_err;
	}

	mutex_lock(&nvt_hybrid_ts->i2c_mutex);
	tmp_addr = nvt_hybrid_ts->client->addr;
	nvt_hybrid_ts->client->addr = i2c_addr;

	ret = nvt_hybrid_ts->chip_data->ts_platform_data->bops->bus_read(&buf[0], 1, &buf[1], 1);
	if (ret < 0)
		TS_LOG_ERR("%s: error, bus_read fail, ret=%d\n", __func__, ret);

	nvt_hybrid_ts->client->addr = tmp_addr;
	mutex_unlock(&nvt_hybrid_ts->i2c_mutex);

i2c_err:
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen i2c write function.

return:
	Executive outcomes. 1---succeed. -5---I/O error
*******************************************************/
int32_t nvt_hybrid_ts_i2c_write(struct i2c_client *client, uint16_t i2c_addr, uint8_t *buf, uint16_t len)
{
	int ret = -1;
	u16 tmp_addr = 0;

	if (!client) {
		TS_LOG_ERR("%s:client is Null\n", __func__);
		return -ENOMEM;
	}

	if (!buf) {
		TS_LOG_ERR("%s:buf is Null\n", __func__);
		return -ENOMEM;
	}

	TS_LOG_DEBUG("%s: i2c_addr=0x%02X, len=%d\n", __func__, (uint8_t)i2c_addr, (uint8_t)len);

	if (!nvt_hybrid_ts->chip_data->ts_platform_data->bops->bus_read) {
		TS_LOG_ERR("%s: error, invalid bus_write\n", __func__);
		ret = -EIO;
		goto i2c_err;
	}

	mutex_lock(&nvt_hybrid_ts->i2c_mutex);
	tmp_addr = nvt_hybrid_ts->client->addr;
	nvt_hybrid_ts->client->addr = i2c_addr;

	ret = nvt_hybrid_ts->chip_data->ts_platform_data->bops->bus_write(&buf[0], len);
	if (ret < 0)
		TS_LOG_ERR("%s: error, bus_write fail, ret=%d\n", __func__, ret);

	nvt_hybrid_ts->client->addr = tmp_addr;
	mutex_unlock(&nvt_hybrid_ts->i2c_mutex);

i2c_err:
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen set i2c debounce function.

return:
	n.a.
*******************************************************/
void nvt_hybrid_set_i2c_debounce(void)
{
	int ret = 0;
	uint8_t buf[8] = {0};
	uint8_t reg1_val = 0;
	uint8_t reg2_val = 0;
	uint32_t retry = 0;

	do {
		msleep(10);

		// set xdata index to 0x1F000
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0xF0;
		ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
		if (ret) {
			TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
		}

		// set i2c debounce 34ns
		buf[0] = 0x15;
		buf[1] = 0x17;
		ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
		if (ret) {
			TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
		}

		buf[0] = 0x15;
		buf[1] = 0x00;
		nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
		reg1_val = buf[1];

		// set schmitt trigger enable
		buf[0] = 0x3E;
		buf[1] = 0x07;
		ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
		if (ret) {
			TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
		}

		buf[0] = 0x3E;
		buf[1] = 0x00;
		nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);

		reg2_val = buf[1];
	} while (((reg1_val != 0x17) || (reg2_val != 0x07)) && (retry++ < 20));

	if(retry == 20) {
		TS_LOG_ERR("%s: set i2c debounce failed, reg1_val=0x%02X, reg2_val=0x%02X\n", __func__, reg1_val, reg2_val);
	}
}

/*******************************************************
Description:
	Novatek touchscreen reset MCU then into idle mode
    function.

return:
	n.a.
*******************************************************/
void nvt_hybrid_sw_reset_idle(void)
{
	int ret = 0;
	uint8_t buf[4]={0};

	//---write i2c cmds to reset idle---
	buf[0]=0x00;
	buf[1]=0xA5;
	ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
	if (ret) {
		TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
	}

	msleep(10);
	nvt_hybrid_set_i2c_debounce();

	//--write i2c cmds to set reset2 high---
	buf[0]=0xFF;
	buf[1]=0x01;
	buf[2]=0xF0;
	ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf,3);
	if (ret) {
		TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
	}

	buf[0]=0x39;
	buf[1]=0x04;
	ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf,2);
	if (ret) {
		TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
	}

}

/*******************************************************
Description:
	Novatek touchscreen reset MCU (no boot) function.

return:
	n.a.
*******************************************************/
void nvt_hybrid_sw_reset(void)
{
	int ret = 0;
	uint8_t buf[8] = {0};

	//---write i2c cmds to reset---
	buf[0] = 0x00;
	buf[1] = 0x5A;
	ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
	if (ret) {
		TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
	}

	// need 5ms delay after sw reset
	msleep(5);
}

/*******************************************************
Description:
	Novatek touchscreen reset MCU (boot) function.

return:
	n.a.
*******************************************************/
void nvt_hybrid_bootloader_reset(void)
{
	int ret = 0;
	uint8_t buf[8] = {0};
	
	TS_LOG_INFO("%s ---enter\n", __func__);

	//---write i2c cmds to reset---
	buf[0] = 0x00;
	buf[1] = 0x69;
	ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
	if (ret) {
		TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
	}

	// need 50ms delay after bootloader reset
	msleep(50);

	TS_LOG_INFO("%s ---eixt!\n", __func__);
}

/*******************************************************
Description:
	Novatek touchscreen IC hardware reset function.

return:
	n.a.
*******************************************************/
void nvt_hybrid_hw_reset(void)
{
	int ret = 0;

	TS_LOG_INFO("%s enter\n", __func__);

	//---trigger rst-pin to reset---
	ret = gpio_direction_output(nvt_hybrid_ts->chip_data->ts_platform_data->reset_gpio, 1);
	if (ret)
		TS_LOG_ERR("%s gpio direction output to 1 fail, ret=%d\n", __func__, ret);

	mdelay(50);
	ret = gpio_direction_output(nvt_hybrid_ts->chip_data->ts_platform_data->reset_gpio, 0);
	if (ret)
		TS_LOG_ERR("%s gpio direction output to 0 fail, ret=%d\n", __func__, ret);

	mdelay(5);
	ret = gpio_direction_output(nvt_hybrid_ts->chip_data->ts_platform_data->reset_gpio, 1);
	if (ret)
		TS_LOG_ERR("%s gpio direction output to 1 fail, ret=%d\n", __func__, ret);
//	mdelay(5);
}

/*******************************************************
Description:
	Novatek touchscreen clear FW status function.

return:
	Executive outcomes. 0---succeed. -1---fail.
*******************************************************/
int32_t nvt_hybrid_clear_fw_status(void)
{
	uint8_t buf[8] = {0};
	int32_t i = 0;
	int ret = 0;
	const int32_t retry = 10;

	//---dummy read to resume TP before writing command---
	nvt_hybrid_ts_i2c_dummy_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address);

	for (i = 0; i < retry; i++) {
		//---set xdata index to 0x14700---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0x47;
		ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
		if (ret) {
			TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
		}

		//---clear fw status---
		buf[0] = 0x51;
		buf[1] = 0x00;
		ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
		if (ret) {
			TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
		}

		//---read fw status---
		buf[0] = 0x51;
		buf[1] = 0xFF;
		nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);

		if (buf[1] == 0x00)
			break;

		msleep(10);
	}

	if (i > retry) {
		TS_LOG_ERR("%s: retry = %d, data = %d\n", __func__, retry, buf[1]);
		return -1;
	}
	else
		return 0;
}

/*******************************************************
Description:
	Novatek touchscreen check FW status function.

return:
	Executive outcomes. 0---succeed. -1---failed.
*******************************************************/
int32_t nvt_hybrid_check_fw_status(void)
{
	int ret = 0;
	uint8_t buf[8] = {0};
	int32_t i = 0;
	const int32_t retry = 20;

	//---dummy read to resume TP before writing command---
	nvt_hybrid_ts_i2c_dummy_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address);

	for (i = 0; i < retry; i++) {
		//---set xdata index to 0x14700---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0x47;
		ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
		if (ret) {
			TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
		}

		//---read fw status---
		buf[0] = 0x51;
		buf[1] = 0x00;
		nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);

		if ((buf[1] & 0xF0) == 0xA0)
			break;

		msleep(10);
	}

	if (i > retry) {
		TS_LOG_ERR("%s: retry = %d, data = %d\n", __func__, retry, buf[1]);
		return -1;
	}
	else
		return 0;
}

/*******************************************************
Description:
	Novatek touchscreen check FW reset state function.

return:
	Executive outcomes. 0---succeed. -1---failed.
*******************************************************/
int32_t nvt_hybrid_check_fw_reset_state(NVT_HYBRID_RST_COMPLETE_STATE check_reset_state)
{
	uint8_t buf[8] = {0};
	int32_t ret = 0;
	int32_t retry = 0;
	TS_LOG_INFO("%s ---enter\n", __func__);
	while (1) {
		msleep(10);
		TS_LOG_DEBUG("%s --retry=%d\n", __func__,retry);
		//---read reset state---
		buf[0] = 0x60;
		buf[1] = 0x00;
		nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);

		if ((buf[1] >= check_reset_state) && (buf[1] < 0xFF)) {
			ret = 0;
			break;
		}

		retry++;
		if(unlikely(retry > 100)) {
			ret = -1;
			TS_LOG_ERR("%s: error, retry=%d, buf[1]=0x%02X\n", __func__, retry, buf[1]);
			break;
		}
	}
	TS_LOG_INFO("%s--- exit!\n", __func__);
	return ret;
}

/*******************************************************************************
** TP VCC
* TP VCC/VDD  power control by GPIO in V8Rx,
* if controled by ldo in other products, open "return -EINVAL"
rmi4_data->tp_vci is 3.1V ,rmi4_data->tp_vddio is 1.8V
*/

static void nvt_hybrid_power_gpio_disable(void)
{
	if (nvt_hybrid_ts == NULL) {
		TS_LOG_ERR("nvt_hybrid_ts == NULL\n");
		return;
	}

	if (nvt_hybrid_ts->chip_data == NULL) {
		TS_LOG_ERR("novatek_chip_data == NULL\n");
		return;
	}

	mutex_lock(&ts_power_gpio_sem);
	if (ts_power_gpio_ref == 1) {
		gpio_direction_output(nvt_hybrid_ts->chip_data->vddio_gpio_ctrl, 0);
				     
	}
	if (ts_power_gpio_ref > 0) {
		ts_power_gpio_ref--;
	}
	TS_LOG_INFO("ts_power_gpio_ref-- = %d\n", ts_power_gpio_ref);
	mutex_unlock(&ts_power_gpio_sem);
}

static int nvt_hybrid_regulator_get(void)
{
	if (1 == nvt_hybrid_ts->chip_data->vci_regulator_type) {
		nvt_hybrid_ts->tp_vci =
		    regulator_get(&nvt_hybrid_ts->ts_dev->dev, "novatek-vdd");
		if (IS_ERR(nvt_hybrid_ts->tp_vci)) {
			TS_LOG_ERR("regulator tp vci not used\n");
			return -EINVAL;
		}
	}

	if (1 == nvt_hybrid_ts->chip_data->vddio_regulator_type) {
		nvt_hybrid_ts->tp_vddio =
		    regulator_get(&nvt_hybrid_ts->ts_dev->dev, "novatek-io");
		if (IS_ERR(nvt_hybrid_ts->tp_vddio)) {
			TS_LOG_ERR("regulator tp vddio not used\n");
			regulator_put(nvt_hybrid_ts->tp_vci);
			return -EINVAL;
		}
	}

	return 0;
}

static void nvt_hybrid_regulator_put(void)
{
	if (1 == nvt_hybrid_ts->chip_data->vci_regulator_type) {
		if (!IS_ERR(nvt_hybrid_ts->tp_vci)) {
			regulator_put(nvt_hybrid_ts->tp_vci);
		}
	}
	if (1 == nvt_hybrid_ts->chip_data->vddio_regulator_type) {
		if (!IS_ERR(nvt_hybrid_ts->tp_vddio)) {
			regulator_put(nvt_hybrid_ts->tp_vddio);
		}
	}
}

static int nvt_hybrid_vci_enable(void)
{
	int retval = 0;
	int vol_vlaue = 0;	
	TS_LOG_INFO("%s vci enable\n", __func__);

	
	if (nvt_hybrid_ts->chip_data->ts_platform_data->fpga_flag == 1)
		return 0 ;

	if (IS_ERR(nvt_hybrid_ts->tp_vci)) {
		TS_LOG_ERR("tp_vci is err\n");
		return -EINVAL;
	}
	
	vol_vlaue = nvt_hybrid_ts->chip_data->regulator_ctr.vci_value;
	
	if(!IS_ERR(nvt_hybrid_ts->tp_vci))
	{
		TS_LOG_INFO("set vci voltage to %d\n", vol_vlaue);
		retval =
		    regulator_set_voltage(nvt_hybrid_ts->tp_vci, vol_vlaue,
					  vol_vlaue);
		if (retval < 0) {
			TS_LOG_ERR
			    ("failed to set voltage regulator tp_vci error: %d\n",
			     retval);
			return -EINVAL;
		}
		
		retval = regulator_enable(nvt_hybrid_ts->tp_vci);
		

		if (retval < 0) {
			TS_LOG_ERR("failed to enable regulator tp_vci\n");
			return -EINVAL;
		}
	}
	
	return 0;
}

static int nvt_hybrid_vci_disable(void)
{
	int retval = 0;
	TS_LOG_INFO("%s enter\n", __func__);
	if (nvt_hybrid_ts->chip_data->ts_platform_data->fpga_flag == 1)
		return 0;
	if (IS_ERR(nvt_hybrid_ts->tp_vci)) {
		TS_LOG_ERR("tp_vci is err\n");
		return -EINVAL;
	}
	retval = regulator_disable(nvt_hybrid_ts->tp_vci);
	if (retval < 0) {
		TS_LOG_ERR("failed to disable regulator tp_vci\n");
		return -EINVAL;
	}

	return 0;
}

static int nvt_hybrid_vddio_enable(void)
{
	int retval = 0;
	int vddio_value = 0;

	if (IS_ERR(nvt_hybrid_ts->tp_vddio)) {
		TS_LOG_ERR("tp_vddio is err\n");
		return -EINVAL;
	}

	vddio_value = nvt_hybrid_ts->chip_data->regulator_ctr.vddio_value;
	if (nvt_hybrid_ts->chip_data->regulator_ctr.need_set_vddio_value) {
		TS_LOG_INFO("set tp_vddio voltage to %d\n", vddio_value);
		retval =
		    regulator_set_voltage(nvt_hybrid_ts->tp_vddio, vddio_value,
					  vddio_value);
		if (retval < 0) {
			TS_LOG_ERR
			    ("failed to set voltage regulator tp_vddio error: %d\n",
			     retval);
			return -EINVAL;
		}
	}

	retval = regulator_enable(nvt_hybrid_ts->tp_vddio);
	if (retval < 0) {
		TS_LOG_ERR("failed to enable regulator tp_vddio\n");
		return -EINVAL;
	}

	return 0;
}


static int nvt_hybrid_vddio_disable(void)
{
	int retval = 0;

	TS_LOG_INFO("%s enter\n", __func__);

	if (nvt_hybrid_ts->chip_data->ts_platform_data->fpga_flag == 1)
		return 0;
	if (IS_ERR(nvt_hybrid_ts->tp_vddio)) {
		TS_LOG_ERR("tp_vddio is err\n");
		return -EINVAL;
	}

	retval = regulator_disable(nvt_hybrid_ts->tp_vddio);

	if (retval < 0) {
		TS_LOG_ERR("failed to disable regulator tp_vddio\n");
		return -EINVAL;
	}

	return 0;
}


/* dts */
static int nvt_hybrid_pinctrl_get_init(void)
{
	int ret = 0;

	nvt_hybrid_ts->pctrl = devm_pinctrl_get(&nvt_hybrid_ts->ts_dev->dev);
	if (IS_ERR(nvt_hybrid_ts->pctrl)) {
		TS_LOG_ERR("failed to devm pinctrl get\n");
		ret = -EINVAL;
		return ret;
	}

	nvt_hybrid_ts->pins_default =
	    pinctrl_lookup_state(nvt_hybrid_ts->pctrl, "default");
	if (IS_ERR(nvt_hybrid_ts->pins_default)) {
		TS_LOG_ERR("failed to pinctrl lookup state default\n");
		ret = -EINVAL;
		goto err_pinctrl_put;
	}

	nvt_hybrid_ts->pins_idle = pinctrl_lookup_state(nvt_hybrid_ts->pctrl, "idle");
	if (IS_ERR(nvt_hybrid_ts->pins_idle)) {
		TS_LOG_ERR("failed to pinctrl lookup state idle\n");
		ret = -EINVAL;
		goto err_pinctrl_put;
	}
	return 0;

err_pinctrl_put:
	devm_pinctrl_put(nvt_hybrid_ts->pctrl);
	return ret;
}

static int nvt_hybrid_pinctrl_select_normal(void)
{
	int retval = NO_ERR;

	retval =
	    pinctrl_select_state(nvt_hybrid_ts->pctrl, nvt_hybrid_ts->pins_default);
	if (retval < 0) {
		TS_LOG_ERR("set iomux normal error, %d\n", retval);
	}
	return retval;
}

static int nvt_hybrid_pinctrl_select_lowpower(void)
{
	int retval = NO_ERR;

	retval = pinctrl_select_state(nvt_hybrid_ts->pctrl, nvt_hybrid_ts->pins_idle);
	if (retval < 0) {
		TS_LOG_ERR("set iomux lowpower error, %d\n", retval);
	}
	return retval;
}

static void nvt_hybrid_power_on_gpio_set(void)
{
	int retval = 0;

	nvt_hybrid_pinctrl_select_normal();
	retval = gpio_direction_input(nvt_hybrid_ts->chip_data->ts_platform_data->irq_gpio);
	if (retval) {
		TS_LOG_ERR("%s: gpio_direction_input for reset gpio failed\n", __func__);
	}

	retval = gpio_direction_output(nvt_hybrid_ts->chip_data->ts_platform_data->reset_gpio, 1);
	if (retval) {
		TS_LOG_ERR("%s: gpio_direction_output for reset gpio failed\n", __func__);
	}

}

static void nvt_hybrid_vci_on(void)
{
	TS_LOG_INFO("%s vci enable\n", __func__);
	if (1 == nvt_hybrid_ts->chip_data->vci_regulator_type) {
		if (!IS_ERR(nvt_hybrid_ts->tp_vci)) {
			TS_LOG_INFO("vci enable is called\n");
			nvt_hybrid_vci_enable();
		}
	}

	if (nvt_hybrid_ts->chip_data->vci_gpio_type) {
		TS_LOG_INFO("%s vci switch gpio on\n", __func__);
		gpio_direction_output(nvt_hybrid_ts->chip_data->vci_gpio_ctrl, 1);
				      
	}

}

static void nvt_hybrid_vddio_on(void)
{
	TS_LOG_INFO("%s vddio enable\n", __func__);
	if (1 == nvt_hybrid_ts->chip_data->vddio_regulator_type) {
		if (!IS_ERR(nvt_hybrid_ts->tp_vddio)) {
			TS_LOG_INFO("vddio enable is called\n");
			nvt_hybrid_vddio_enable();
		}
	}

	if (nvt_hybrid_ts->chip_data->vddio_gpio_type) {
		TS_LOG_INFO("%s vddio switch gpio on\n", __func__);
		mutex_lock(&ts_power_gpio_sem);
		if (ts_power_gpio_ref == 0) {
			gpio_direction_output(nvt_hybrid_ts->chip_data->vddio_gpio_ctrl, 1);
		}
		ts_power_gpio_ref++;
		TS_LOG_INFO("ts_power_gpio_ref++ = %d\n", ts_power_gpio_ref);
		mutex_unlock(&ts_power_gpio_sem);

	}
}

static void nvt_hybrid_power_on(void)
{
	TS_LOG_INFO("%s  satart!\n", __func__);

	nvt_hybrid_vci_on();
	mdelay(5);
	nvt_hybrid_vddio_on();
	mdelay(5);
	nvt_hybrid_power_on_gpio_set();
	mdelay(2);
}

static void nvt_hybrid_power_off_gpio_set(void)
{
	int retval = 0;
	TS_LOG_INFO("%s enter\n", __func__);

	retval = gpio_direction_output(nvt_hybrid_ts->chip_data->ts_platform_data->reset_gpio, 0);
	if (retval) {
		TS_LOG_ERR("%s: gpio_direction_output for reset gpio failed\n", __func__);
	}

	nvt_hybrid_pinctrl_select_lowpower();
	//gpio_direction_input(nvt_hybrid_ts->chip_data->ts_platform_data->reset_gpio);
	mdelay(1);
}

static void nvt_hybrid_vddio_off(void)
{

	TS_LOG_INFO("%s enter\n", __func__);

	if (1 == nvt_hybrid_ts->chip_data->vddio_regulator_type) {
		if (!IS_ERR(nvt_hybrid_ts->tp_vddio)) {
			nvt_hybrid_vddio_disable();
		}
	}

	if (nvt_hybrid_ts->chip_data->vddio_gpio_type) {
		TS_LOG_INFO("%s vddio switch gpio off\n", __func__);
		//gpio_direction_output(nvt_hybrid_ts->chip_data->vddio_gpio_ctrl, 0);
		nvt_hybrid_power_gpio_disable();
	}

}

static void nvt_hybrid_vci_off(void)
{

	TS_LOG_INFO("%s enter\n", __func__);
	if (1 == nvt_hybrid_ts->chip_data->vci_regulator_type) {
		if (!IS_ERR(nvt_hybrid_ts->tp_vci)) {
			nvt_hybrid_vci_disable();
		}
	}

	if (nvt_hybrid_ts->chip_data->vci_gpio_type) {
		TS_LOG_INFO("%s vci switch gpio off\n", __func__);
		gpio_direction_output(nvt_hybrid_ts->chip_data->vci_gpio_ctrl, 0);
				     
	}
}

static void nvt_hybrid_power_off(void)
{
//	uint8_t buf[2] = {0};

	TS_LOG_INFO("%s enter\n", __func__);
/*
	//---dummy read to resume TP before writing command---
	nvt_hybrid_ts_i2c_dummy_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address);

	//---write i2c command to enter "deep sleep mode"---
	buf[0] = 0x50;
	buf[1] = 0x12;
	nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
*/
	nvt_hybrid_power_off_gpio_set();
	nvt_hybrid_vddio_off();
	mdelay(10);
	nvt_hybrid_vci_off();
	mdelay(10);

}

static int nvt_hybrid_gpio_request(void)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s enter\n", __func__);
	if ((1 == nvt_hybrid_ts->chip_data->vci_gpio_type)
	    && (1 == nvt_hybrid_ts->chip_data->vddio_gpio_type)) {
		if (nvt_hybrid_ts->chip_data->vci_gpio_ctrl ==
		    nvt_hybrid_ts->chip_data->vddio_gpio_ctrl) {
			retval = gpio_request(nvt_hybrid_ts->chip_data->vci_gpio_ctrl, "ts_vci_gpio");
			if (retval) {
				TS_LOG_ERR
				    ("SFT:Ok;  ASIC: Real ERR----unable to request vci_gpio_ctrl firset:%d\n",
				     nvt_hybrid_ts->chip_data->vci_gpio_ctrl);
				goto ts_vci_out;
			}
		} else {
			retval = gpio_request(nvt_hybrid_ts->chip_data->vci_gpio_ctrl, "ts_vci_gpio");
			if (retval) {
				TS_LOG_ERR
				    ("SFT:Ok;  ASIC: Real ERR----unable to request vci_gpio_ctrl2:%d\n",
				     nvt_hybrid_ts->chip_data->vci_gpio_ctrl);
				goto ts_vci_out;
			}
			retval = gpio_request(nvt_hybrid_ts->chip_data->vddio_gpio_ctrl, "ts_vddio_gpio");
			if (retval) {
				TS_LOG_ERR
				    ("SFT:Ok;  ASIC: Real ERR----unable to request vddio_gpio_ctrl:%d\n",
				     nvt_hybrid_ts->chip_data->vddio_gpio_ctrl);
				goto ts_vddio_out;
			}
		}
	} else {
		if (1 == nvt_hybrid_ts->chip_data->vci_gpio_type) {
			retval = gpio_request(nvt_hybrid_ts->chip_data->vci_gpio_ctrl, "ts_vci_gpio");
			if (retval) {
				TS_LOG_ERR
				    ("SFT:Ok;  ASIC: Real ERR----unable to request vci_gpio_ctrl2:%d\n",
				     nvt_hybrid_ts->chip_data->vci_gpio_ctrl);
				goto ts_vci_out;
			}
		}
		if (1 == nvt_hybrid_ts->chip_data->vddio_gpio_type) {
			retval = gpio_request(nvt_hybrid_ts->chip_data->vddio_gpio_ctrl, "ts_vddio_gpio");
			if (retval) {
				TS_LOG_ERR
				    ("SFT:Ok;  ASIC: Real ERR----unable to request vddio_gpio_ctrl:%d\n",
				     nvt_hybrid_ts->chip_data->vddio_gpio_ctrl);
				goto ts_vddio_out;
			}
		}
	}

	TS_LOG_INFO("reset:%d, irq:%d,\n",
		    nvt_hybrid_ts->chip_data->ts_platform_data->reset_gpio,
		    nvt_hybrid_ts->chip_data->ts_platform_data->irq_gpio);

	goto ts_vci_out;

ts_vddio_out:
	gpio_free(nvt_hybrid_ts->chip_data->vci_gpio_ctrl);
ts_vci_out:
	return retval;
}

static void nvt_hybrid_gpio_free(void)
{
	TS_LOG_INFO("%s enter\n", __func__);

	/*0 is power supplied by gpio, 1 is power supplied by ldo */
	if (1 == nvt_hybrid_ts->chip_data->vci_gpio_type) {
		if (nvt_hybrid_ts->chip_data->vci_gpio_ctrl)
			gpio_free(nvt_hybrid_ts->chip_data->vci_gpio_ctrl);
	}
	if (1 == nvt_hybrid_ts->chip_data->vddio_gpio_type) {
		if (nvt_hybrid_ts->chip_data->vddio_gpio_ctrl)
			gpio_free(nvt_hybrid_ts->chip_data->vddio_gpio_ctrl);
	}
}

/*******************************************************
Description:
	Novatek touchscreen get firmware related information
	function.

return:
	Executive outcomes. 0---success. -1---fail.
*******************************************************/
void nvt_hybrid_get_fw_ver(void)
{
	int ret = 0;
	uint8_t buf[3] = {0};

	//---dummy read to resume TP before writing command---
	nvt_hybrid_ts_i2c_dummy_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address);

	//---set xdata index to 0x14700---
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0x47;
	ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
	if (ret) {
		TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
	}

	//---read fw info---
	buf[0] = 0x78;
	nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
	snprintf(nvt_hybrid_ts->chip_data->version_name, PAGE_SIZE, "%02X", buf[1]);
	TS_LOG_INFO("%s: fwver is %s\n", __func__, nvt_hybrid_ts->chip_data->version_name);
}


/*******************************************************
Description:
	Novatek touchscreen read chip id function.

return:
	Executive outcomes. 0x26---succeed.
*******************************************************/
static uint8_t nvt_hybrid_ts_read_chipid(void)
{
	uint8_t buf[8] = {0};
	int32_t retry = 0;
	int ret = 0;

	//---dummy read to resume TP before writing command---
	nvt_hybrid_ts_i2c_dummy_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address);

	// reset idle to keep default addr 0x01 to read chipid
	buf[0] = 0x00;
	buf[1] = 0xA5;
	ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
	if (ret) {
		TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
	}

	msleep(10);

	for (retry = 5; retry > 0; retry--) {
		//write i2c index to 0x1F000
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0xF0;
		ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, 0x01, buf, 3);
		if (ret) {
			TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
		}

		//read hw chip id
		buf[0] = 0x00;
		buf[1] = 0x00;
		nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, 0x01, buf, 2);

		if (buf[1] == 0x26)
			break;
	}

	return buf[1];
}

/*  query the configure from dts and store in prv_data */
static int nvt_hybrid_parse_dts(struct device_node *device,
			       struct ts_kit_device_data *chip_data)
{

	int retval = NO_ERR;
	int read_val = 0;
	const char *raw_data_dts = NULL;
	const char *mp_selftest_mode_dts = NULL;
	char *producer=NULL;

	retval =
	    of_property_read_u32(device, NVT_HYBRID_IRQ_CFG,
				 &chip_data->irq_config);
	if (retval) {
		TS_LOG_ERR("get irq config failed\n");
	}
	retval =
	    of_property_read_u32(device, NVT_HYBRID_ALGO_ID,
				 &chip_data->algo_id);
	if (retval) {
		TS_LOG_ERR("get algo id failed\n");
	}
	retval =
	    of_property_read_u32(device, NVT_HYBRID_IC_TYPES,
				 &chip_data->ic_type);
	if (retval) {
		TS_LOG_ERR("get device ic_type failed\n");
	} else {
		g_tskit_ic_type = chip_data->ic_type;
	}
	retval =
	    of_property_read_u32(device, NVT_HYBRID_VCI_LDO_VALUE,
				 &chip_data->regulator_ctr.vci_value);
	if (retval) {
		TS_LOG_INFO("Not define Vci value in Dts, use fault value\n");
		chip_data->regulator_ctr.vci_value = 3100000;
	}

	retval =
	    of_property_read_u32(device, NVT_HYBRID_NEED_SET_VDDIO_VALUE,
				 &chip_data->regulator_ctr.
				 need_set_vddio_value);
	if (retval) {
		TS_LOG_INFO
		    ("Not define need set Vddio value in Dts, use fault value\n");
		chip_data->regulator_ctr.need_set_vddio_value = 0;
	} else {
		retval =
		    of_property_read_u32(device, NVT_HYBRID_VDDIO_LDO_VALUE,
					 &chip_data->regulator_ctr.vddio_value);
		if (retval) {
			TS_LOG_INFO
			    ("Not define Vddio value in Dts, use fault value\n");
			chip_data->regulator_ctr.vddio_value = 1800000;
		}
	}

	retval =
	    of_property_read_u32(device, NVT_HYBRID_WD_CHECK,
				 &chip_data->need_wd_check_status);
	if (retval) {
		TS_LOG_ERR("get device ic_type failed\n");
	}

	retval =
	    of_property_read_u32(device, NVT_HYBRID_UNIT_CAP_TEST_INTERFACE,
				 &chip_data->unite_cap_test_interface);
	if (retval) {
		TS_LOG_ERR("get unite_cap_test_interface failed\n");
	}

	retval =
	    of_property_read_u32(device, NVT_HYBRID_X_MAX, &chip_data->x_max);
	if (retval) {
		TS_LOG_ERR("get device x_max failed\n");
	}
	retval =
	    of_property_read_u32(device, NVT_HYBRID_Y_MAX, &chip_data->y_max);
	if (retval) {
		TS_LOG_ERR("get device y_max failed\n");
	}
	retval =
	    of_property_read_u32(device, NVT_HYBRID_X_MAX_MT,
				 &chip_data->x_max_mt);
	if (retval) {
		TS_LOG_ERR("get device x_max failed\n");
	}
	retval =
	    of_property_read_u32(device, NVT_HYBRID_Y_MAX_MT,
				 &chip_data->y_max_mt);
	if (retval) {
		TS_LOG_ERR("get device y_max failed\n");
	}

	retval =
	    of_property_read_u32(device, NVT_HYBRID_AIN_TX_NUM,
				 &nvt_hybrid_ts->ain_tx_num);
		TS_LOG_INFO("get device AIN_TX_NUM = %d\n", nvt_hybrid_ts->ain_tx_num);
	if (retval) {
		TS_LOG_ERR("get device AIN_TX_NUM failed\n");
	}

	retval =
	    of_property_read_u32(device, NVT_HYBRID_AIN_RX_NUM,
				 &nvt_hybrid_ts->ain_rx_num);
		TS_LOG_INFO("get device AIN_RX_NUM = %d\n", nvt_hybrid_ts->ain_rx_num);
	if (retval) {
		TS_LOG_ERR("get device AIN_RX_NUM failed\n");
	}

	retval =
	    of_property_read_u32(device, NVT_HYBRID_VCI_GPIO_TYPE,
				 &chip_data->vci_gpio_type);
	if (retval) {
		TS_LOG_ERR("get device NVT_HYBRID_VCI_GPIO_TYPE failed\n");
	}
	retval =
	    of_property_read_u32(device, NVT_HYBRID_VCI_REGULATOR_TYPE,
				 &chip_data->vci_regulator_type);
	if (retval) {
		TS_LOG_ERR("get device NVT_HYBRID_VCI_REGULATOR_TYPE failed\n");
	}
	retval =
	    of_property_read_u32(device, NVT_HYBRID_VDDIO_GPIO_TYPE,
				 &chip_data->vddio_gpio_type);
	if (retval) {
		TS_LOG_ERR("get device NVT_HYBRID_VDDIO_GPIO_TYPE failed\n");
	}
	retval =
	    of_property_read_u32(device, NVT_HYBRID_PROJECTID_LEN,
				 &chip_data->projectid_len);
	if (retval) {
		TS_LOG_ERR("get device projectid_len failed\n");
		chip_data->projectid_len = 0;
	}
	retval =
	    of_property_read_u32(device, NVT_HYBRID_VDDIO_REGULATOR_TYPE,
				 &chip_data->vddio_regulator_type);
	if (retval) {
		TS_LOG_ERR
		    ("get device NVT_HYBRID_VDDIO_REGULATOR_TYPE failed\n");
	}

	retval =
	    of_property_read_string(device, NVT_HYBRID_TEST_TYPE,
				 &raw_data_dts);
	if (retval) {
		TS_LOG_INFO
		    ("get device NVT_HYBRID_TEST_TYPE not exist, use default value\n");
		strncpy(chip_data->tp_test_type, "Normalize_type:judge_last_result", TS_CAP_TEST_TYPE_LEN);
	}
	else {
		snprintf(chip_data->tp_test_type, PAGE_SIZE, "%s", raw_data_dts);
	}
	TS_LOG_INFO("%s get test type = %s\n", __func__, chip_data->tp_test_type);

	retval =
	    of_property_read_string(device, NVT_HYBRID_MP_SELFTEST_MODE,
				 &mp_selftest_mode_dts);
	if (retval) {
		TS_LOG_INFO
		    ("get device NVT_HYBRID_MP_SELFTEST_MODE not exist, use default value\n");
		strncpy(nvt_hybrid_ts->mp_selftest_mode, "firmware", NVT_HYBRID_MP_SELFTEST_MODE_LEN);
	}
	else {
		snprintf(nvt_hybrid_ts->mp_selftest_mode, PAGE_SIZE, "%s", mp_selftest_mode_dts);
	}
	TS_LOG_INFO("%s get NVT_HYBRID_MP_SELFTEST_MODE = %s\n", __func__, nvt_hybrid_ts->mp_selftest_mode);

	/*0 is power supplied by gpio, 1 is power supplied by ldo */
	if (1 == chip_data->vci_gpio_type) {
		chip_data->vci_gpio_ctrl =
		    of_get_named_gpio(device, NVT_HYBRID_VCI_GPIO_CTRL, 0);
		if (!gpio_is_valid(chip_data->vci_gpio_ctrl)) {
			TS_LOG_ERR ("SFT: ok; ASIC: Real err----power gpio is not valid\n");
		}
	}
	if (1 == chip_data->vddio_gpio_type) {
		chip_data->vddio_gpio_ctrl =
		    of_get_named_gpio(device, NVT_HYBRID_VDDIO_GPIO_CTRL, 0);
		if (!gpio_is_valid(chip_data->vddio_gpio_ctrl)) {
			TS_LOG_ERR("SFT: ok; ASIC: Real err----power gpio is not valid\n");
		}
	}

	/*0 is cover without glass, 1 is cover with glass that need glove mode */
	retval =
	    of_property_read_u32(device, NVT_HYBRID_COVER_FORCE_GLOVE,
				 &chip_data->cover_force_glove);
	if (retval) {
		TS_LOG_INFO("get device NVT_HYBRID_COVER_FORCE_GLOVE failed,use default!\n");
		chip_data->cover_force_glove = 0;	/*if not define in dtsi,set 0 to disable it*/
	}

	retval =
	    of_property_read_u32(device, "bootloader_update_enable",
				 &chip_data->bootloader_update_enable);
	if (retval) {
		chip_data->bootloader_update_enable = 0;
		TS_LOG_INFO("get device bootloader_update_enable fail\n");
	}


	retval = of_property_read_u32(device, GHOST_DETECT_SUPPORT, &chip_data->ghost_detect_support);
	if (retval) {
		chip_data->ghost_detect_support = 0;
		TS_LOG_ERR("%s:%s device %s not exit,use default value.\n",
			__func__, GHOST_LOG_TAG, GHOST_DETECT_SUPPORT);
	}else{
		TS_LOG_INFO("%s:%s get device %s : %d\n",
			__func__, GHOST_LOG_TAG, GHOST_DETECT_SUPPORT, chip_data->ghost_detect_support);
		if (chip_data->ghost_detect_support){
			ts_kit_algo_det_ght_init();
		}
	}

	/* get holster mode value */
	retval = of_property_read_u32(device, NVT_HYBRID_HOLSTER_SUPPORTED, &read_val);
	if (!retval) {
		TS_LOG_INFO("%s: get chip specific holster_supported = %d\n", __func__, read_val);
		 chip_data->ts_platform_data->feature_info.holster_info.holster_supported = (u8) read_val;
	} else {
		TS_LOG_ERR("%s: can not get holster_supported value\n", __func__);
		chip_data->ts_platform_data->feature_info.holster_info.holster_supported = 0;
	}

	/* get glove mode value */
	retval = of_property_read_u32(device, NVT_HYBRID_GLOVE_SUPPORTED, &read_val);
	if (!retval) {
		TS_LOG_INFO("%s: get chip specific glove_supported = %d\n", __func__, read_val);
		 chip_data->ts_platform_data->feature_info.glove_info.glove_supported = (u8) read_val;
	} else {
		TS_LOG_ERR("%s: can not get glove_supported value\n", __func__);
		chip_data->ts_platform_data->feature_info.glove_info.glove_supported = 0;
	}

	/* get roi mode value */
	retval = of_property_read_u32(device, NVT_HYBRID_ROI_SUPPORTED, &read_val);
	if (!retval) {
		TS_LOG_INFO("%s: get chip specific roi_supported = %d\n", __func__, read_val);
		 chip_data->ts_platform_data->feature_info.roi_info.roi_supported = (u8) read_val;
	} else {
		TS_LOG_ERR("%s: can not get roi_supported value\n", __func__);
		chip_data->ts_platform_data->feature_info.roi_info.roi_supported = 0;
	}
	
	retval = of_property_read_string(device, "producer", &producer);
	if (!retval && NULL != producer) {
		strcpy(chip_data->module_name, producer);
	}
	TS_LOG_INFO("module_name: %s\n", chip_data->module_name);
	/*rawdata test timeout*/
	retval = of_property_read_u32(device, NVT_HYBRID_RAWDATA_TIMEOUT, &read_val);
	if (!retval) {
		TS_LOG_INFO("get chip rawdata limit time = %d\n", read_val);
		chip_data->rawdata_get_timeout = read_val;
	} else {
		chip_data->rawdata_get_timeout = NVT_RAWDATA_TEST_TIME_DEFAULT;
		TS_LOG_INFO("can not get chip rawdata limit time, use default\n");
	}
	TS_LOG_INFO
	    ("irq_config = %d, algo_id = %d, ic_type = %d, x_max = %d, y_max = %d, x_mt = %d,y_mt = %d, bootloader_update_enable = %d\n",
	     chip_data->irq_config,
	     chip_data->algo_id, chip_data->ic_type, chip_data->x_max,
	     chip_data->y_max, chip_data->x_max_mt, chip_data->y_max_mt,
	     chip_data->bootloader_update_enable);
	/* get tp color flag */
	retval = of_property_read_u32(device,  "support_get_tp_color", &read_val);
	if (retval) {
		TS_LOG_INFO("%s, get device support_get_tp_color failed, will use default value: 0 \n ", __func__);
		read_val = 0; //default 0: no need know tp color
	}
	nvt_hybrid_ts->support_get_tp_color = (uint8_t)read_val;
	TS_LOG_INFO("%s, support_get_tp_color = %d \n", __func__, nvt_hybrid_ts->support_get_tp_color);
	return NO_ERR;
}

static int novatek_hybrid_chip_detect(struct ts_kit_platform_data *data)
{
	int retval = NO_ERR;
	uint8_t buf[8] = {0};

	TS_LOG_INFO("%s enter\n", __func__);

	if (!data) {
		TS_LOG_ERR("device, ts_kit_platform_data *data is NULL \n");
		retval =  -EINVAL;
		return -ENOMEM;
	}
	
	mutex_init(&nvt_hybrid_ts->i2c_mutex);
	nvt_hybrid_ts->chip_data->ts_platform_data = data;
	nvt_hybrid_ts->ts_dev = data->ts_dev;
	nvt_hybrid_ts->ts_dev->dev.of_node = nvt_hybrid_ts->chip_data->cnode;

	nvt_hybrid_ts->client = data->client;

	nvt_hybrid_parse_dts(nvt_hybrid_ts->ts_dev->dev.of_node, nvt_hybrid_ts->chip_data);

	nvt_hybrid_ts->abs_x_max = nvt_hybrid_ts->chip_data->x_max;
	nvt_hybrid_ts->abs_y_max = nvt_hybrid_ts->chip_data->y_max;
	nvt_hybrid_ts->max_touch_num = NVT_HYBRID_TOUCH_MAX_FINGER_NUM;
	
#if NVT_HYBRID_TOUCH_KEY_NUM > 0
	nvt_hybrid_ts->max_button_num = NVT_HYBRID_TOUCH_KEY_NUM;
#endif


	retval = nvt_hybrid_regulator_get();
	if (retval < 0) {
		goto regulator_err;
	}

	retval = nvt_hybrid_gpio_request();
	if (retval < 0) {
		goto gpio_err;
	}

	retval = nvt_hybrid_pinctrl_get_init();
	if (retval < 0) {
		goto pinctrl_get_err;
	}
	//---power up the chip---
	nvt_hybrid_power_on();

	//---reset the chip---
	nvt_hybrid_hw_reset();
	TS_LOG_INFO("chip has been reset\n");

	//---check i2c read befor checking chipid, John 20161202
	retval = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client,  NVT_HYBRID_I2C_HW_Address,  buf, 2);
	if (retval < 0) {
		/* double confirm i2c communication after hw reset */
		nvt_hybrid_hw_reset();
		retval = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client,  NVT_HYBRID_I2C_HW_Address,  buf, 2);
		if (retval < 0) {
			TS_LOG_INFO("not find novatek devices\n");
			goto check_err;
		}
	}
	//------------------------------------------- 
	
	//---check chip id---
	retval = nvt_hybrid_ts_read_chipid();
	if (retval != 0x26) {
		TS_LOG_ERR("nvt_hybrid_ts_read_chipid is not 0x26. retval=0x%02X\n", retval);
		TS_LOG_ERR("not find novatek device\n");
		retval = -EINVAL;
		goto check_err;
	} else {
		TS_LOG_INFO("find novatek device\n");
		
		// please make sure display reset(RESX) sequence and mipi dsi cmds sent before this
		nvt_hybrid_bootloader_reset();
		nvt_hybrid_check_fw_reset_state(NVT_HYBRID_RESET_STATE_INIT);
	}

	TS_LOG_INFO("%s done\n", __func__);
	return NO_ERR;

check_err:
	nvt_hybrid_power_off();
pinctrl_get_err:
	nvt_hybrid_gpio_free();
gpio_err:
	nvt_hybrid_regulator_put();
regulator_err:
	if(nvt_hybrid_ts->chip_data)
		kfree(nvt_hybrid_ts->chip_data);
	if (nvt_hybrid_ts)
		kfree(nvt_hybrid_ts);
	nvt_hybrid_ts = NULL;
	TS_LOG_ERR("no power\n");
	return retval;
}

static int novatek_hybrid_input_config(struct input_dev *input_dev)
{
	TS_LOG_INFO("%s enter\n", __func__);

	if (!input_dev) {
		TS_LOG_ERR("%s: input_dev is Null\n", __func__);
		return -ENOMEM;
	}

	//---set input device info.---
	input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	input_dev->propbit[0] = BIT(INPUT_PROP_DIRECT);

#if NVT_HYBRID_TOUCH_MAX_FINGER_NUM > 1
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);	  //area major = 255
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR, 0, 255, 0, 0);	  //area minor = 255

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, (nvt_hybrid_ts->abs_x_max - 1), 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, (nvt_hybrid_ts->abs_y_max - 1), 0, 0);
#endif

	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, nvt_hybrid_ts->max_touch_num, 0, 0);

#if NVT_HYBRID_TOUCH_KEY_NUM > 0
	for (retry = 0; retry < NVT_HYBRID_TOUCH_KEY_NUM; retry++) {
		input_set_capability(input_dev, EV_KEY, nvt_hybrid_touch_key_array[retry]);
	}
#endif

	return NO_ERR;
}

static int novatek_hybrid_irq_top_half(struct ts_cmd_node *cmd)
{
	if (!cmd) {
		TS_LOG_ERR("%s: cmd is Null\n", __func__);
		return -ENOMEM;
	}

	cmd->command = TS_INT_PROCESS;

	TS_LOG_DEBUG("%s enter\n", __func__);

	return NO_ERR;
}

static int novatek_hybrid_irq_bottom_half(struct ts_cmd_node *in_cmd,
				     struct ts_cmd_node *out_cmd)
{
	struct ts_fingers *info = NULL;
	int32_t ret = -1;
	uint8_t point_data[64] = {0};
	uint32_t position = 0;
	uint32_t input_x = 0;
	uint32_t input_y = 0;
	uint32_t input_w_major = 0;
	uint32_t input_w_minor = 0;
	uint8_t input_id = 0;
	uint8_t press_id[NVT_HYBRID_TOUCH_MAX_FINGER_NUM] = {0};

	int32_t i = 0;
	int32_t finger_cnt = 0;

	uint8_t roi_diff[ROI_DATA_READ_LENGTH+1] = {0};
	int32_t temp_finger_status = 0;

	if (!in_cmd) {
		TS_LOG_ERR("%s: in_cmd is Null\n", __func__);
		return -ENOMEM;
	}

	if (!out_cmd) {
		TS_LOG_ERR("%s: out_cmd is Null\n", __func__);
		return -ENOMEM;
	}

	info = &out_cmd->cmd_param.pub_params.algo_param.info;

	out_cmd->command = TS_INPUT_ALGO;
	out_cmd->cmd_param.pub_params.algo_param.algo_order = nvt_hybrid_ts->chip_data->algo_id;
	TS_LOG_DEBUG("order: %d\n",
		     out_cmd->cmd_param.pub_params.algo_param.algo_order);

	ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, point_data, 62 + 1);	//event buffer length 62
	if (ret < 0) {
		TS_LOG_ERR("%s: nvt_hybrid_ts_i2c_read failed. ret=%d\n", __func__, ret);
		goto XFER_ERROR;
	}

	//--- dump I2C buf ---
	//for (i = 0; i < 10; i++) {
	//	printk("%02X %02X %02X %02X %02X %02X  ", point_data[1+i*6], point_data[2+i*6], point_data[3+i*6], point_data[4+i*6], point_data[5+i*6], point_data[6+i*6]);
	//}
	//printk("\n");

	finger_cnt = 0;
	input_id = (uint8_t)(point_data[1] >> 3);

	for (i = 0; i < nvt_hybrid_ts->max_touch_num; i++) {
		/*
		 * Each 2-bit finger status field represents the following:
		 * 00 = finger not present
		 * 01 = finger present and data accurate
		 * 10 = finger present but data may be inaccurate
		 * 11 = reserved
		 */
		info->fingers[i].status = 0;
	}

	for (i = 0; i < nvt_hybrid_ts->max_touch_num; i++) {
		position = 1 + 6 * i;
		input_id = (uint8_t)(point_data[position + 0] >> 3);
		if (input_id > NVT_HYBRID_TOUCH_MAX_FINGER_NUM)
			continue;

		if ((point_data[position] & 0x07) == 0x03) {	// finger up (break)
			continue;
		} else if (((point_data[position] & 0x07) == 0x01) || ((point_data[position] & 0x07) == 0x02)) {	//finger down (enter & moving)
			input_x = (uint32_t)(point_data[position + 1] << 4) + (uint32_t) (point_data[position + 3] >> 4);
			input_y = (uint32_t)(point_data[position + 2] << 4) + (uint32_t) (point_data[position + 3] & 0x0F);
			input_w_major = (uint32_t)(point_data[position + 4]);
			if (input_w_major > 255)
				input_w_major = 255;
			input_w_minor = (uint32_t)(point_data[position + 5]);
			if (input_w_minor > 255)
				input_w_minor = 255;

			if ((input_x > nvt_hybrid_ts->abs_x_max)||(input_y > nvt_hybrid_ts->abs_y_max))
				continue;

			press_id[input_id - 1] = 1;
//			input_report_key(nvt_hybrid_ts->input_dev, BTN_TOUCH, 1);
//			input_report_abs(nvt_hybrid_ts->input_dev, ABS_MT_POSITION_X, input_x);
//			input_report_abs(nvt_hybrid_ts->input_dev, ABS_MT_POSITION_Y, input_y);
//			input_report_abs(nvt_hybrid_ts->input_dev, ABS_MT_TOUCH_MAJOR, input_w);
//			input_report_abs(nvt_hybrid_ts->input_dev, ABS_MT_PRESSURE, input_p);
//			input_report_abs(nvt_hybrid_ts->input_dev, ABS_MT_TRACKING_ID, input_id - 1);

//			input_mt_sync(nvt_hybrid_ts->input_dev);

			info->fingers[input_id - 1].status = 1;
			info->fingers[input_id - 1].x = input_x;
			info->fingers[input_id - 1].y = input_y;
			info->fingers[input_id - 1].major = input_w_major;
			info->fingers[input_id - 1].minor = input_w_minor;
			finger_cnt++;
			temp_finger_status++;
		}
	}
//	if (finger_cnt == 0) {
//		input_report_key(nvt_hybrid_ts->input_dev, BTN_TOUCH, 0);

//		input_mt_sync(nvt_hybrid_ts->input_dev);
//	}


	info->cur_finger_number = finger_cnt;

#if NVT_HYBRID_TOUCH_KEY_NUM > 0
	if (point_data[61] == 0xF8) {
		info->special_button_flag = 1;
		for (i = 0; i < NVT_HYBRID_TOUCH_KEY_NUM; i++) {
				info->special_button_key = nvt_hybrid_touch_key_array[i];
		}
	} else {
		info->special_button_flag = 0;
	}
#endif

#ifdef ROI
	if (nvt_hybrid_roi_switch) {
		TS_LOG_DEBUG("%s: temp_finger_status = %d, nvt_hybrid_pre_finger_status = %d\n", __func__, temp_finger_status, nvt_hybrid_pre_finger_status);
		if (temp_finger_status && temp_finger_status != nvt_hybrid_pre_finger_status){
			roi_diff[0] = 0x99;
			ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, roi_diff, ROI_DATA_READ_LENGTH + 1);
			if (ret < 0) {
				TS_LOG_ERR("%s: nvt_hybrid_ts_i2c_read failed(ROI). ret=%d\n", __func__, ret);
				goto XFER_ERROR;
			}

			for(i=0;i<ROI_DATA_READ_LENGTH;i++) {
				nvt_hybrid_roi_data[i] = roi_diff[i+1];
			}
		}
	}
	nvt_hybrid_pre_finger_status = temp_finger_status;
#endif

XFER_ERROR:
	return NO_ERR;
}

static int novatek_hybrid_glove_switch(struct ts_glove_info *info)
{
	uint8_t buf[4] = {0};
	int retval = NO_ERR;
	u8 sw = 0;

	TS_LOG_INFO("%s enter\n", __func__);

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}

	if (!nvt_hybrid_ts->chip_data->ts_platform_data->feature_info.glove_info.glove_supported) {
		TS_LOG_INFO("%s: Not Support glove feature, supported = %d\n",
					__func__, nvt_hybrid_ts->chip_data->ts_platform_data->feature_info.glove_info.glove_supported);
		return retval;
	}

	switch (info->op_action) {
		case TS_ACTION_READ:
			buf[0] = 0x5A;
			retval = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
			if (retval < 0) {
				TS_LOG_ERR("%s: get glove_switch(%d), failed : %d", __func__, info->glove_switch, retval);
				break;
			}
			info->glove_switch =  ((buf[1]>>1) & 0x01);//buf[1] & 0x02;
			TS_LOG_INFO("%s: read glove_switch=%d, 1:on 0:off\n", __func__, info->glove_switch);
			break;

		case TS_ACTION_WRITE:
			TS_LOG_INFO("%s: write glove_switch=%d\n", __func__, info->glove_switch);

			sw = info->glove_switch;
			if ((NVT_HYBRID_GLOVE_SWITCH_ON != sw)
			    && (NVT_HYBRID_GLOVE_SWITCH_OFF != sw)) {
				TS_LOG_ERR("%s: write wrong state: switch = %d\n", __func__, sw);
				retval = -EFAULT;
				break;
			}

			if(NVT_HYBRID_GLOVE_SWITCH_ON == sw)	{
				//---enable glove mode---
				buf[0] = 0x50;
				buf[1] = 0x71;
				retval = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
				if (retval < 0) {
					TS_LOG_ERR("%s: set glove switch(%d), failed : %d", __func__, sw, retval);
				}
			}
			else {
				//---disable glove mode---
				buf[0] = 0x50;
				buf[1] = 0x72;
				retval = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
				if (retval < 0) {
					TS_LOG_ERR("%s: set glove switch(%d), failed : %d", __func__, sw, retval);
				}
			}

			//wait for 1 frame
			msleep(NVT_HYBRID_DELAY_FRAME_TIME);

			break;

		default:
			TS_LOG_ERR("%s: invalid switch status: %d", __func__, info->glove_switch);
			retval = -EINVAL;
			break;
	}

	return retval;
}

static int novatek_hybrid_holster_switch(struct ts_holster_info *info)
{
	uint8_t buf[12] = {0};
	int retval = NO_ERR;
	u8 sw = 0;
	int x0 = nvt_hybrid_ts->chip_data->ts_platform_data->feature_info.window_info.top_left_x0;
	int y0 = nvt_hybrid_ts->chip_data->ts_platform_data->feature_info.window_info.top_left_y0;
	int x1 = nvt_hybrid_ts->chip_data->ts_platform_data->feature_info.window_info.bottom_right_x1;
	int y1 = nvt_hybrid_ts->chip_data->ts_platform_data->feature_info.window_info.bottom_right_y1;

	TS_LOG_INFO("%s enter\n", __func__);

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}

	if (!nvt_hybrid_ts->chip_data->ts_platform_data->feature_info.holster_info.holster_supported) {
		TS_LOG_INFO("%s: Not Support hloster feature, supported = %d\n",
					__func__, nvt_hybrid_ts->chip_data->ts_platform_data->feature_info.holster_info.holster_supported);
		return retval;
	}

	switch (info->op_action) {
		case TS_ACTION_READ:
			buf[0] = 0x5A;
			retval = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
			if (retval < 0) {
				TS_LOG_ERR("%s: get holster_switch(%d), failed : %d", __func__, info->holster_switch, retval);
				break;
			}
			info->holster_switch =  ((buf[1]>>2) & 0x01);//buf[1] & 0x04;
			TS_LOG_INFO("%s: read holster_switch=%d, 1:on 0:off\n", __func__, info->holster_switch);
			break;

		case TS_ACTION_WRITE:
			TS_LOG_INFO("%s: write holster_switch=%d, x0=%d, y0=%d, x1=%d, y1=%d\n",
				__func__, info->holster_switch, x0, y0, x1, y1);

			sw = info->holster_switch;
			if ((NVT_HYBRID_HOLSTER_SWITCH_ON != sw)
			    && (NVT_HYBRID_HOLSTER_SWITCH_OFF != sw)) {
				TS_LOG_ERR("%s: write wrong state: switch = %d\n", __func__, sw);
				retval = -EFAULT;
				break;
			}

			if(NVT_HYBRID_HOLSTER_SWITCH_ON == sw)	{

				//---enable holster mode---
				buf[0] = 0x50;
				buf[1] = 0x75;
				retval = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
				if (retval < 0) {
					TS_LOG_ERR("%s: set holster switch(%d), failed : %d", __func__, sw, retval);
				}
			}
			else {
				//---disable holster mode---
				buf[0] = 0x50;
				buf[1] = 0x76;
				retval = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
				if (retval < 0) {
					TS_LOG_ERR("%s: set holster switch(%d), failed : %d", __func__, sw, retval);
				}
			}
			
			//wait for 1 frame
			msleep(NVT_HYBRID_DELAY_FRAME_TIME);

			break;

		default:
			TS_LOG_ERR("%s: invalid switch status: %d", __func__, info->holster_switch);
			retval = -EINVAL;
			break;
	}

	return retval;
}

static int novatek_hybrid_roi_switch(struct ts_roi_info *info)
{
#ifdef ROI
	uint8_t buf[4] = {0};
	int retval = NO_ERR;
	u8 sw = 0;

	TS_LOG_INFO("%s enter\n", __func__);

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}

	if (!nvt_hybrid_ts->chip_data->ts_platform_data->feature_info.roi_info.roi_supported) {
		TS_LOG_INFO("%s: Not Support ROI feature, supported = %d\n",
					__func__, nvt_hybrid_ts->chip_data->ts_platform_data->feature_info.roi_info.roi_supported);
		return retval;
	}

	switch (info->op_action) {
		case TS_ACTION_READ:
			buf[0] = 0x5A;
			retval = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
			if (retval < 0) {
				TS_LOG_ERR("%s: get nvt_hybrid_roi_switch(%d), failed : %d", __func__, info->roi_switch, retval);
				break;
			}
			info->roi_switch = ((buf[1]>>3) & 0x01);//buf[1] & 0x08;
			TS_LOG_INFO("%s: read nvt_hybrid_roi_switch=%d, 1:on 0:off\n", __func__, info->roi_switch);
			nvt_hybrid_roi_switch = info->roi_switch;
			break;

		case TS_ACTION_WRITE:
			TS_LOG_INFO("%s: write nvt_hybrid_roi_switch=%d\n", __func__, info->roi_switch);

			sw = info->roi_switch;
			if ((ROI_SWITCH_ON != sw)
			    && (ROI_SWITCH_OFF != sw)) {
				TS_LOG_ERR("%s: write wrong state: switch = %d\n", __func__, sw);
				retval = -EFAULT;
				break;
			}

			if(ROI_SWITCH_ON == sw)	{
				//---enable roi mode---
				buf[0] = 0x50;
				buf[1] = 0x77;
				retval = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
				if (retval < 0) {
					TS_LOG_ERR("%s: set roi switch(%d), failed : %d", __func__, sw, retval);
				}
			}
			else {
				//---disable roi mode---
				buf[0] = 0x50;
				buf[1] = 0x78;
				retval = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
				if (retval < 0) {
					TS_LOG_ERR("%s: set roi switch(%d), failed : %d", __func__, sw, retval);
				}
			}
			nvt_hybrid_roi_switch = info->roi_switch;

			//wait for 1 frame
			msleep(NVT_HYBRID_DELAY_FRAME_TIME);

			break;

		default:
			TS_LOG_ERR("%s: invalid switch status: %d", __func__, info->roi_switch);
			retval = -EINVAL;
			break;
	}

	return retval;
#else
	return NO_ERR;
#endif
}

static unsigned char *novatek_hybrid_roi_rawdata(void)
{
#ifdef ROI
	TS_LOG_DEBUG("%s enter\n", __func__);
	return (unsigned char *)nvt_hybrid_roi_data;
#else
	return NULL;
#endif
}

/*  do some things before power off.
*/
static int novatek_hybrid_before_suspend(void)
{
	int retval = NO_ERR;
	
	if(nvt_hybrid_ts->firmware_updating) {
		TS_LOG_INFO("%s: tp fw is updating, return\n", __func__);
		return NO_ERR;
	}
	if(nvt_hybrid_ts->sensor_testing) {
		TS_LOG_INFO("%s: tp fw is sensor_testing, return\n", __func__);
		return NO_ERR;
	}
	
	TS_LOG_INFO("%s  enter\n", __func__);
	
	TS_LOG_INFO("%s exit\n", __func__);	
	return retval;
}

static int novatek_hybrid_suspend(void)
{
	int ret = 0;
	uint8_t buf[4] = {0};
	int retval = NO_ERR;
	
	if(nvt_hybrid_ts->firmware_updating) {
		TS_LOG_INFO("%s: tp fw is updating, return\n", __func__);
		return NO_ERR;
	}
	if(nvt_hybrid_ts->sensor_testing) {
		TS_LOG_INFO("%s: tp fw is sensor_testing, return\n", __func__);
		return NO_ERR;
	}

	TS_LOG_INFO("%s enter\n", __func__);

	if (g_tskit_ic_type != ONCELL) 
	{
		if (!g_tskit_pt_station_flag) {
			//----porwer off -------
			nvt_hybrid_ts->no_power = true;
			nvt_hybrid_power_off();
		}
		else{
			//---dummy read to resume TP before writing command---
			nvt_hybrid_ts_i2c_dummy_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address);

			//---write i2c command to enter "deep sleep mode"---
			buf[0] = 0x50;
			buf[1] = 0x12;
			ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
			if (ret) {
				TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
			}
		}
	}
	else
	{
		//---dummy read to resume TP before writing command---
		nvt_hybrid_ts_i2c_dummy_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address);

		//---write i2c command to enter "deep sleep mode"---
		buf[0] = 0x50;
		buf[1] = 0x12;
		ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
		if (ret) {
			TS_LOG_ERR("%s: nvt i2c write fail\n", __func__);
		}
		nvt_hybrid_power_off();
	}
	TS_LOG_INFO("%s exit\n", __func__);
	return retval;
}


/*    do not add time-costly function here.
*/
static int novatek_hybrid_resume(void)
{
	int retval = NO_ERR;

	if(nvt_hybrid_ts->firmware_updating) {
		TS_LOG_INFO("%s: tp fw is updating, return\n", __func__);
		return NO_ERR;
	}
	if(nvt_hybrid_ts->sensor_testing) {
		TS_LOG_INFO("%s: tp fw is sensor_testing, return\n", __func__);
		return NO_ERR;
	}

	TS_LOG_INFO("%s enter\n", __func__);

	if (g_tskit_ic_type != ONCELL) 
	{
		if (!g_tskit_pt_station_flag) {
			//---power_on-----
			nvt_hybrid_ts->no_power = false;
			nvt_hybrid_power_on();
		}	
	}
	else
	{
		nvt_hybrid_power_on();
	}
	nvt_hybrid_hw_reset();
	TS_LOG_INFO("%s exit\n", __func__);
	return retval;
}

/*  do some things after power on. */
static int novatek_hybrid_after_resume(void *feature_info)
{
	int retval = NO_ERR;
	struct ts_feature_info *info = NULL;
	struct ts_roi_info roi_info;
	struct ts_holster_info holster_info;
	struct ts_glove_info glove_info;

	TS_LOG_INFO("%s enter\n", __func__);

	if(nvt_hybrid_ts->firmware_updating) {
		TS_LOG_INFO("%s: tp fw is updating, return\n", __func__);
		return NO_ERR;
	}

	if(nvt_hybrid_ts->sensor_testing) {
		TS_LOG_INFO("%s: tp fw is sensor_testing, return\n", __func__);
		return NO_ERR;
	}

	if (!feature_info) {
		TS_LOG_ERR("%s: feature_info is Null\n", __func__);
		return -ENOMEM;
	}

	info = (struct ts_feature_info *)feature_info;

	// please make sure display reset(RESX) sequence and mipi dsi cmds sent before this
	nvt_hybrid_bootloader_reset();
	nvt_hybrid_check_fw_reset_state(NVT_HYBRID_RESET_STATE_INIT);
	//----------------------------------------------------------------------------------
	/*Glove Switch recovery*/
	if(info->glove_info.glove_supported) {
		glove_info.op_action = TS_ACTION_WRITE;
		glove_info.glove_switch = info->glove_info.glove_switch;
		retval = novatek_hybrid_glove_switch(&glove_info);
		if (retval < 0) {
			TS_LOG_ERR("Failed to set glove switch(%d), err: %d\n",
				   info->glove_info.glove_switch, retval);
		}
	}

	/*Holster Switch recovery*/
	if(info->holster_info.holster_supported) {
		holster_info.op_action = TS_ACTION_WRITE;
		holster_info.holster_switch = info->holster_info.holster_switch;
		retval = novatek_hybrid_holster_switch(&holster_info);
		if (retval < 0) {
			TS_LOG_ERR("Failed to set holster switch(%d), err: %d\n",
				   info->holster_info.holster_switch, retval);
		}
	}

	/*roi Switch recovery*/
	if (info->roi_info.roi_supported) {
		roi_info.op_action = TS_ACTION_WRITE;
		roi_info.roi_switch = info->roi_info.roi_switch;
		retval = novatek_hybrid_roi_switch(&roi_info);
		if (retval < 0) {
			TS_LOG_ERR("Failed to set roi switch(%d), err: %d\n",
				   info->roi_info.roi_switch, retval);
		}
	}

	TS_LOG_INFO("%s exit\n", __func__);
	return retval;
}


int32_t nvt_hybrid_read_projectid(void)
{
	uint8_t buf[64] = {0};
	int retval = NO_ERR;

	nvt_hybrid_get_fw_ver();
	
	nvt_hybrid_sw_reset_idle();

	// Step 1 : initial bootloader
	retval = NVT_Hybrid_Init_BootLoader();
	if (retval) {
		return retval;
	}

	// Step 2 : unlock
	buf[0] = 0x00;
	buf[1] = 0x35;
	retval = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
	if (retval < 0) {
		TS_LOG_ERR("%s: write unlock error!!(%d)\n", __func__, retval);
		return retval;
	}
	msleep(10);
	
	//Step 3 : Flash Read Command
	buf[0] = 0x00;
	buf[1] = 0x03;
	buf[2] = 0x01;
	buf[3] = 0xF0;
	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = 0x20;
	retval = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 7);
	if (retval < 0) {
		TS_LOG_ERR("%s: write Read Command error!!(%d)\n", __func__, retval);
		return retval;
	}
	msleep(10);

	// Check 0xAA (Read Command)
	buf[0] = 0x00;
	buf[1] = 0x00;
	retval = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
	if (retval < 0) {
		TS_LOG_ERR("%s: Check 0xAA (Read Command) error!!(%d)\n", __func__, retval);
		return retval;
	}
	if (buf[1] != 0xAA) {
		TS_LOG_ERR("%s: Check 0xAA (Read Command) error!! status=0x%02X\n", __func__, buf[1]);
		return -1;
	}
	msleep(10);

	//Step 4 : Read Flash Data
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0x40;
	retval = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_BLDR_Address, buf, 3);
	if (retval < 0) {
		TS_LOG_ERR("%s: change index error!!(%d)\n", __func__, retval);
		return retval;
	}
	msleep(10);

	// Read Back
	buf[0] = 0x00;
	retval = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_BLDR_Address, buf, 33);
	if (retval < 0) {
		TS_LOG_ERR("%s: Check 0xAA (Read Command) error!!(%d)\n", __func__, retval);
		return retval;
	}
	msleep(10);

	//buf[3:12]	=> novatek_product id
	strncpy(nvt_hybrid_product_id, &buf[3], NVT_HYBRID_PROJECT_ID_LEN);

	//---debug, Taylor 20161114---
	//TS_LOG_INFO("novatek_kit_product_id=0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
	//				buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12]);
	//--------------------------

	TS_LOG_INFO("novatek_kit_product_id=%s\n", nvt_hybrid_product_id);
	
	nvt_hybrid_bootloader_reset();

	snprintf(nvt_hybrid_ts->chip_data->chip_name, PAGE_SIZE, nvt_hybrid_vendor_name);
	//snprintf(nvt_hybrid_ts->chip_data->module_name, PAGE_SIZE, "ctc");

	return retval;
}

static int novatek_hybrid_get_info(struct ts_chip_info_param *info)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s enter\n", __func__);
	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}
	snprintf(info->ic_vendor, MAX_STR_LEN, "novatek-%s", nvt_hybrid_product_id);
	snprintf(info->mod_vendor, CHIP_INFO_LENGTH, "%s", nvt_hybrid_ts->chip_data->module_name);
	snprintf(info->fw_vendor, MAX_STR_LEN, "%s", nvt_hybrid_ts->chip_data->version_name);

	return retval;
}

#define BitSet(data, n)		((data) |= (1 << (n)))
#define BitClear(data, n)	((data) &= ~(1 << (n)))
static int32_t novatek_hybrid_regs_operators(struct ts_regs_info *info) {
	int32_t ret = 0;
	int32_t i = 0;
	uint8_t buf[TS_MAX_REG_VALUE_NUM + 1] = {0};

	TS_LOG_INFO("%s enter\n", __func__);

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		ret = -ENOMEM;
		goto out;
	}

	switch (info->op_action) {
		case TS_ACTION_WRITE:
			// 1. set xdata index to 0x1xxyy
			buf[0] = 0xFF;
			buf[1] = 0x01;	//5st bytes (ex. 01)
			buf[2] = (info->addr >> 8) & 0xFF;	//2~3 bytes (ex. xx)
			ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: TS_ACTION_WRITE set index error, addr(0x%04X)\n",
						__func__, info->addr);
				goto out;
			}

			// 2. write i2c data to 0x1xxyy
			buf[0] = (info->addr) & 0xFF;	//0~1 bytes (ex. yy)
			for (i = 0 ; i < info->num ; i++) {
				buf[i+1] = info->values[i];
			}
			if ((info->bit>0)&&(info->bit < 8) && (info->num == 1)) {
				//get regs value from ic
				buf[1] = 0x00;
				ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, info->num+1);
				if (ret < 0) {
					TS_LOG_ERR("%s: TS_ACTION_WRITE read data error, addr(0x%04X)\n",
							__func__, info->addr);
					goto out;
				}

				//bit operator
				buf[0] = (info->addr) & 0xFF;	//0~1 bytes (ex. yy)

				if (info->values[0] == 0)
					buf[1] = BitClear(buf[1], info->bit);
				else if (info->values[0] == 1)
					BitSet(buf[1], info->bit);
			}

			ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, info->num+1);
			if (ret < 0) {
			TS_LOG_ERR("%s: TS_ACTION_WRITE write regs error, addr(0x%04X) len(%d)\n",
						__func__, info->addr, info->num);
				goto out;
			}
			break;
			
		case TS_ACTION_READ:
			// 1. set xdata index to 0x1xxyy
			buf[0] = 0xFF;
			buf[1] = 0x01;	//5st bytes (ex. 01)
			buf[2] = (info->addr >> 8) & 0xFF;	//2~3 bytes (ex. xx)
			ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: TS_ACTION_READ set index error, addr(0x%04X)\n",
						__func__, info->addr);
				goto out;
			}

			// 2. read i2c data to 0x1xxyy
			memset(buf, 0, sizeof(buf));
			buf[0] = (info->addr) & 0xFF;	//0~1 bytes (ex. yy)
			ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, info->num+1);
			TS_LOG_INFO("%s --read   buf[1]=%d----\n", __func__,buf[i+1]);

			if (ret < 0) {
				TS_LOG_ERR("%s: TS_ACTION_READ read data error, addr(0x%04X)\n",
						__func__, info->addr);
				goto out;	
			}
			
			TS_LOG_INFO("buf[0]=0x%02X, buf[1]=0x%02X \n",buf[0],buf[1]);
			
			if ((info->bit>0)&&(info->bit < 8) && (info->num == 1)) {
				nvt_hybrid_ts->chip_data->reg_values[0] = buf[1] & (0x01 << (unsigned int)(info->bit));
			} else {
				//copy buf to chip_data->reg_values[]
				for (i = 0 ; i < info->num ; i++) {
					nvt_hybrid_ts->chip_data->reg_values[i] = buf[i+1];
					
				}
			}

	 		TS_LOG_INFO("reg_values=0x%02X\n", nvt_hybrid_ts->chip_data->reg_values[0]);
			
			break;
		default:
			TS_LOG_ERR("%s: invalid action type %d\n", __func__, info->op_action);
			ret = -EINVAL;
	}

	//  set xdata index to 0x14700
	buf[0] = 0xFF;
	buf[1] = 0x01;	
	buf[2] = 0x47;	
	ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
	if (ret < 0) {
		TS_LOG_ERR("%s: TS_ACTION_WRITE set index error, addr(0x%04X)\n",
				__func__, info->addr);
	}
out:
	
	return ret;
}


int novatek_hybrid_get_debug_data(struct ts_diff_data_info *info, struct ts_cmd_node *out_cmd)
{
	int ret = NO_ERR;

	TS_LOG_INFO("%s enter\n", __func__);

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		ret = -ENOMEM;
		goto out;
	}
	if (!out_cmd) {
		TS_LOG_ERR("%s: out_cmd is Null\n", __func__);
		return -ENOMEM;
	}

#if NVT_HYBRID_TOUCH_MP
	switch (info->debug_type){
		case READ_DIFF_DATA:
			TS_LOG_INFO("%s read diff \n", __func__);
			if (!strncmp(nvt_hybrid_ts->mp_selftest_mode, "firmware", 8))
				nvt_hybrid_read_diff(nvt_hybrid_rawdata_diff);
			else //mp selftest for 206
				nvt_ctrlram_read_diff(nvt_hybrid_rawdata_diff);
			
			memcpy(&info->buff[2], nvt_hybrid_rawdata_diff, (nvt_hybrid_ts->ain_rx_num*nvt_hybrid_ts->ain_tx_num*4));
			break;		
		case READ_RAW_DATA:
			TS_LOG_INFO("%s read diff \n", __func__);
			if (!strncmp(nvt_hybrid_ts->mp_selftest_mode, "firmware", 8))
				nvt_hybrid_read_raw(nvt_hybrid_rawdata_fwMutual) ;
			else //mp selftest for 206
				nvt_ctrlram_read_raw(nvt_hybrid_rawdata_fwMutual) ;
			
			memcpy(&info->buff[2], nvt_hybrid_rawdata_fwMutual, (nvt_hybrid_ts->ain_rx_num*nvt_hybrid_ts->ain_tx_num*4));	
			break;	
		default:
			TS_LOG_ERR("%s error. Unknown type %d\n", __func__, info->debug_type);
			ret = -EINVAL;
			break;
	}
	info->buff[0] = nvt_hybrid_ts->ain_rx_num;
	info->buff[1] = nvt_hybrid_ts->ain_tx_num;
	info->used_size = nvt_hybrid_ts->ain_rx_num*nvt_hybrid_ts->ain_tx_num +  2;
#endif

out:
	return ret;
}

static int novatek_hybrid_fw_update_boot(char *file_name)
{
	char joint_chr = '_';
	char firmware_file_type[5]={".bin"};
	int retval = NO_ERR;

	TS_LOG_INFO("%s: enter\n", __func__);

	if (!file_name) {
		TS_LOG_ERR("%s: file_name is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}

	/*
	  * firmware name [product_name]_[ic_name]_[module]_[vendor]
	  * Host send file_name = [product_name]_[ic_name]
	  */

	strncat(file_name, nvt_hybrid_product_id, MAX_STR_LEN);
	strncat(file_name, &joint_chr, 1);
	strncat(file_name, nvt_hybrid_ts->chip_data->module_name, MAX_STR_LEN);
	strncat(file_name, firmware_file_type, MAX_STR_LEN);

	TS_LOG_INFO("file_name name is :%s\n", file_name);

	retval = nvt_bybrid_fw_update_boot(file_name);

	if (retval < 0) {
		TS_LOG_ERR("%s: retval = %d, update fail!\n", __func__, retval);
	}else if(retval == 1) {
		TS_LOG_INFO("%s:retval = %d, no need to update!\n",__func__, retval);
		retval = 0;
	}else {
		retval = 0;
		TS_LOG_INFO("%s: update ok!or firmware not exit\n", __func__);
	}

	return retval;
}

static int novatek_hybrid_fw_update_sd(void)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s enter\n", __func__);
	retval = nvt_hybrid_fw_update_sd();

	if (retval)
		TS_LOG_ERR("%s: retval = %d, update fail!\n", __func__, retval);
	else
		TS_LOG_INFO("%s: update ok!\n", __func__);
	return retval;
}

static void novatek_hybrid_shutdown(void)
{
	TS_LOG_INFO("%s enter\n", __func__);

	nvt_hybrid_power_off();
	nvt_hybrid_gpio_free();
	nvt_hybrid_regulator_put();
	return;
}

static int novatek_hybrid_get_capacitance_test_type(struct ts_test_type_info *info)
{
	int ret = 0;

	if (!info) {
		TS_LOG_ERR("%s:info is Null\n", __func__);
		return -ENOMEM;
	}

	switch (info->op_action) {
		case TS_ACTION_READ:
			memcpy(info->tp_test_type, nvt_hybrid_ts->chip_data->tp_test_type, TS_CAP_TEST_TYPE_LEN);
			TS_LOG_INFO("%s:test_type=%s\n", __func__, info->tp_test_type);
			break;
		case TS_ACTION_WRITE:
			break;
		default:
			TS_LOG_ERR("%s:invalid op action:%d\n",
				__func__, info->op_action);
			ret = -EINVAL;
			break;
	}
	return ret;
}

static int novatek_hybrid_selftest_get_rawdata(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd)
{
	int retval = NO_ERR;
	TS_LOG_INFO("%s enter\n", __func__);

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		return -ENOMEM;
	}

	if (!out_cmd) {
		TS_LOG_ERR("%s: out_cmd is Null\n", __func__);
		return -ENOMEM;
	}

#if NVT_HYBRID_TOUCH_MP
	if (!strncmp(nvt_hybrid_ts->mp_selftest_mode, "firmware", 8))
		retval = nvt_hybrid_selftest(info);
	else //mp selftest for 206
		retval = nvt_ctrlram_selftest(info);
#endif

	if (retval < 0)
		TS_LOG_ERR("failed to get rawdata\n");

	return retval;
}


static int novatek_hybrid_init(void)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s enter\n", __func__);

	/* Get project info */
	if(nvt_hybrid_read_projectid() < 0) {
		TS_LOG_ERR("%s read project id error!\n", __func__);
	}
	msleep(100);
	nvt_hybrid_get_fw_info();

	if (nvt_hybrid_ts->support_get_tp_color) {
		/* Get tp_color */
		if(nvt_206_read_tp_color() < 0) {
			TS_LOG_ERR("%s read tp color error!\n", __func__);
		}
	}
#if NVT_HYBRID_TOUCH_PROC
	retval = nvt_hybrid_flash_proc_init();
	if (retval != 0) {
		TS_LOG_ERR("nvt flash proc init failed. retval=%d\n", retval);
		goto init_NVT_ts_err;
	}
#endif

#if NVT_HYBRID_TOUCH_EXT_PROC
	retval = nvt_hybrid_extra_proc_init();
	if (retval != 0) {
		TS_LOG_ERR("nvt extra proc init failed. retval=%d\n", retval);
		goto init_NVT_ts_err;
	}
#endif

#if NVT_HYBRID_TOUCH_MP
	mutex_init(&nvt_hybrid_ts->mp_mutex);
#endif

	TS_LOG_INFO("%s done\n", __func__);
	return NO_ERR;

init_NVT_ts_err:
	return retval;
}

struct ts_device_ops nvt_hybrid_ts_kit_ops = {
	.chip_detect = novatek_hybrid_chip_detect,
	.chip_init = novatek_hybrid_init,

	.chip_input_config = novatek_hybrid_input_config,

	.chip_shutdown = novatek_hybrid_shutdown,

	.chip_irq_top_half = novatek_hybrid_irq_top_half,
	.chip_irq_bottom_half = novatek_hybrid_irq_bottom_half,

	.chip_before_suspend = novatek_hybrid_before_suspend,
	.chip_suspend = novatek_hybrid_suspend,
	.chip_resume = novatek_hybrid_resume,
	.chip_after_resume = novatek_hybrid_after_resume,

	.chip_get_info = novatek_hybrid_get_info,

	.chip_fw_update_boot = novatek_hybrid_fw_update_boot,
	.chip_fw_update_sd = novatek_hybrid_fw_update_sd,

	.chip_get_capacitance_test_type = novatek_hybrid_get_capacitance_test_type,
	.chip_get_rawdata = novatek_hybrid_selftest_get_rawdata,

	.chip_glove_switch = novatek_hybrid_glove_switch,
	.chip_holster_switch = novatek_hybrid_holster_switch,
	.chip_roi_switch = novatek_hybrid_roi_switch,
	.chip_roi_rawdata = novatek_hybrid_roi_rawdata,

	.chip_get_debug_data = novatek_hybrid_get_debug_data,
	.chip_regs_operate = novatek_hybrid_regs_operators,
};

static int __init nvt_hybrid_module_init(void)
{
	bool found = false;
	struct device_node* child = NULL;
	struct device_node* root = NULL;
	int error = NO_ERR;

	TS_LOG_INFO("%s called here\n", __func__);

	nvt_hybrid_ts= kzalloc(sizeof(struct nvt_hybrid_ts_data), GFP_KERNEL);
	if (!nvt_hybrid_ts) {
		TS_LOG_ERR("Failed to alloc mem for struct nvt_hybrid_ts\n");
		error =  -ENOMEM;
		return error;
	}

	nvt_hybrid_ts->chip_data = kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
	if (!nvt_hybrid_ts->chip_data) {
		TS_LOG_ERR("Failed to alloc mem for struct nvt_chip_data\n");
		error =  -ENOMEM;
		goto out;
	}

	root = of_find_compatible_node(NULL, NULL, "huawei,ts_kit");
	if (!root)
	{
		TS_LOG_ERR("huawei_ts, find_compatible_node huawei,ts_kit error\n");
		error = -EINVAL;
		goto out;
	}

	for_each_child_of_node(root, child)  //find the chip node
	{
		if (of_device_is_compatible(child, NVT_HYBRID_VENDER_NAME))
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		TS_LOG_ERR(" not found chip nvt child node  !\n");
		error = -EINVAL;
		goto out;
	}

	nvt_hybrid_ts->chip_data->cnode = child;
	nvt_hybrid_ts->chip_data->ops = &nvt_hybrid_ts_kit_ops;

	nvt_hybrid_ts->firmware_updating = false;
	nvt_hybrid_ts->sensor_testing = false;
	nvt_hybrid_ts->print_criteria = true;
	nvt_hybrid_ts->no_power = false;

	TS_LOG_INFO("found novatek child node !\n");
	error = huawei_ts_chip_register(nvt_hybrid_ts->chip_data);
	if(error)
	{
		TS_LOG_ERR(" nvt chip register fail !\n");
		goto out;
	}
	TS_LOG_INFO("nvt chip_register! err=%d\n", error);
	return error;
out:
	if(nvt_hybrid_ts->chip_data)
		kfree(nvt_hybrid_ts->chip_data);
	if (nvt_hybrid_ts)
		kfree(nvt_hybrid_ts);
	nvt_hybrid_ts = NULL;
	return error;
}

static void __exit nvt_module_exit(void)
{
	TS_LOG_INFO("nvt_module_exit called here\n");

	return;
}

late_initcall(nvt_hybrid_module_init);
module_exit(nvt_module_exit);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");


