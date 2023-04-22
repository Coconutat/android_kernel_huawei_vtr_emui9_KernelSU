/* drivers/input/touchscreen/sec_ts_fn.c
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 * http://www.samsungsemi.com/
 *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/unaligned.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/ctype.h>
#include <linux/hrtimer.h>
#include <linux/firmware.h>
#include <linux/uaccess.h>
#include "sec_ts.h"

#define SEC_TS_SELFTEST_STR_LEN	80

#define GLOVE_MODE_EN		(1 << 0)
#define CLEAR_COVER_EN		(1 << 1)
#define FAST_GLOVE_MODE_EN	(1 << 2)

int sec_ts_glove_mode_enables(struct sec_ts_data *ts, bool enable)
{
	int ret = NO_ERR;

	if (enable)
		ts->touch_functions = (ts->touch_functions |
					SEC_TS_BIT_SETFUNC_GLOVE |
					SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC);
	else
		ts->touch_functions = ((ts->touch_functions &
			(~SEC_TS_BIT_SETFUNC_GLOVE)) |
			SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: pwr off, glove:%d, status:%x\n", __func__,
					enable, ts->touch_functions);
		goto glove_enable_err;
	}

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHFUNCTION,
			(u8 *)&ts->touch_functions, 2);
	if (ret < 0) {
		TS_LOG_ERR("%s: Failed to send command", __func__);
		goto glove_enable_err;
	}

	TS_LOG_INFO("%s: glove:%d, status:%x\n", __func__,
		enable, ts->touch_functions);

	return ret;

glove_enable_err:
	return -EIO;
}

int sec_ts_charger_mode_enables(struct sec_ts_data *ts, bool enable)
{
	int mode = (enable) ? 0x1 : 0x0;

	TS_LOG_INFO("%s: charger %s\n", __func__, enable ? "CONNECTED" : "DISCONNECTED");
	return gpio_direction_output(ts->charger_notify_gpio, mode);
}

int sec_ts_cover_mode_enalbes(struct sec_ts_data *ts, bool enable)
{
	int ret = NO_ERR;

	TS_LOG_INFO("%s: %d\n", __func__, ts->cover_type);

	switch (ts->cover_type) {
	case SEC_TS_VIEW_WIRELESS:
	case SEC_TS_VIEW_COVER:
	case SEC_TS_VIEW_WALLET:
	case SEC_TS_FLIP_WALLET:
	case SEC_TS_LED_COVER:
	case SEC_TS_MONTBLANC_COVER:
	case SEC_TS_CLEAR_FLIP_COVER:
	case SEC_TS_QWERTY_KEYBOARD_EUR:
	case SEC_TS_QWERTY_KEYBOARD_KOR:
		ts->cover_cmd = (u8)ts->cover_type;
		break;
	case SEC_TS_CHARGER_COVER:
	case SEC_TS_COVER_NOTHING1:
	case SEC_TS_COVER_NOTHING2:
	default:
		ts->cover_cmd = 0;
		TS_LOG_ERR("%s: not chage touch state, %d\n",
				__func__, ts->cover_type);
		break;
	}

	if (enable)
		ts->touch_functions = (ts->touch_functions | SEC_TS_BIT_SETFUNC_COVER | SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC);
	else
		ts->touch_functions = ((ts->touch_functions & (~SEC_TS_BIT_SETFUNC_COVER)) | SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: pwr off, close:%d, status:%x\n", __func__,
					enable, ts->touch_functions);
		goto cover_enable_err;
	}

	if (enable) {
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_COVERTYPE, &ts->cover_cmd, 1);
		if (ret < 0) {
			TS_LOG_ERR("%s: Failed to send covertype command: %d", __func__, ts->cover_cmd);
			goto cover_enable_err;
		}
	}

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHFUNCTION, (u8 *)&(ts->touch_functions), 2);
	if (ret < 0) {
		TS_LOG_ERR("%s: Failed to send command", __func__);
		goto cover_enable_err;
	}

	TS_LOG_INFO("%s: close:%d, status:%x\n", __func__,
		enable, ts->touch_functions);

	return 0;

cover_enable_err:
	return -EIO;

}


int sec_ts_fix_tmode(struct sec_ts_data *ts, u8 mode_type, u8 state)
{
	int ret = NO_ERR;
	u8 onoff[1] = {STATE_MANAGE_OFF};
	u8 tBuff[2] = { mode_type, state };

	TS_LOG_INFO("%s\n", __func__);
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, onoff, 1);
	if (ret < 0)
		TS_LOG_ERR("%s: i2c write SEC_TS_CMD_STATEMANAGE_ON err: %d\n", __func__, ret);

	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, tBuff, sizeof(tBuff));
	if (ret < 0)
		TS_LOG_ERR("%s: i2c write SEC_TS_CMD_STATEMANAGE_ON err: %d\n", __func__, ret);

	sec_ts_delay(20);

	return ret;
}

int sec_ts_release_tmode(struct sec_ts_data *ts)
{
	int ret = NO_ERR;
	u8 onoff[1] = {STATE_MANAGE_ON};

	TS_LOG_INFO("%s\n", __func__);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, onoff, 1);
	sec_ts_delay(20);

	return ret;
}

int execute_selftest(struct sec_ts_data *ts, bool save_result)
{
	int rc = 0;
	u8 tpara[2] = {0x3F, 0x40}; /* test command */

	TS_LOG_INFO("%s: Self test start!\n", __func__);
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELFTEST, tpara, 2);
	if (rc < 0) {
		TS_LOG_ERR("%s: Send selftest cmd failed!\n", __func__);
		goto err_exit;
	}

	sec_ts_delay(SEC_TS_SELFTEST_EXE_DELAY);
	TS_LOG_ERR("%s:dealyed %d\n", __func__, SEC_TS_SELFTEST_EXE_DELAY);

	rc = sec_ts_wait_for_ready(ts, SEC_TS_VENDOR_ACK_SELF_TEST_DONE);
	if (rc < 0) {
		TS_LOG_ERR("%s: Selftest execution time out!\n", __func__);
		goto err_exit;
	}

	TS_LOG_ERR("%s: Self test done!\n", __func__);
err_exit:
	return rc;
}

static int sec_ts_read_frame(struct sec_ts_data *ts, struct sec_ts_test_mode *mode)
{
	unsigned int readbytes = 0;
	unsigned char *pRead = NULL;
	u8 mode_type = TYPE_INVALID_DATA;
	int ret = 0;
	int i = 0;

	TS_LOG_DEBUG("%s\n", __func__);

	/* set data length, allocation buffer memory */
	readbytes = ts->rx_count * ts->tx_count * 2;

	pRead = kzalloc(readbytes, GFP_KERNEL);
	if (!pRead) {
		TS_LOG_ERR("%s: Read frame kzalloc failed\n", __func__);
		return -ENOMEM;
	}

	/* set OPCODE and data type */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MUTU_RAW_TYPE, &mode->type, 1);
	if (ret < 0) {
		TS_LOG_ERR("Set rawdata type failed\n");
		goto ErrorExit;
	}

	sec_ts_delay(50);

	if (mode->type == TYPE_OFFSET_DATA_SDC) {
		/*
		  * Excute selftest for real cap offset data, because real cap data is
		  * not memory data in normal touch.
		  */
		char para = TO_TOUCH_MODE;

		disable_irq(ts->client->irq);

		execute_selftest(ts, 1);

		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
		if (ret < 0) {
			TS_LOG_ERR("%s: Set rawdata type failed\n", __func__);
			enable_irq(ts->client->irq);
			goto ErrorRelease;
		}

		enable_irq(ts->client->irq);
	}

	/* read data */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_TOUCH_RAWDATA, pRead, readbytes);
	if (ret < 0) {
		TS_LOG_ERR("%s: read rawdata failed!\n", __func__);
		goto ErrorRelease;
	}

	memset(ts->pFrame, 0x00, readbytes);

	for (i = 0; i < readbytes; i += 2)
		ts->pFrame[i / 2] = pRead[i + 1] + (pRead[i] << 8);

	mode->avg = 0;
	mode->max = mode->min = ts->pFrame[0];
	for (i = 0; i < (ts->rx_count * ts->tx_count); i++) {
		mode->min = min(mode->min, ts->pFrame[i]);
		mode->max = max(mode->max, ts->pFrame[i]);
		mode->avg += ts->pFrame[i];
	}
	if (ts->rx_count*ts->tx_count)
		mode->avg /= (ts->rx_count*ts->tx_count);
	else
		TS_LOG_ERR("%s: tx_count or rx_count is zero\n", __func__);


	TS_LOG_DEBUG("%02X%02X%02X readbytes=%d\n",
			pRead[0], pRead[1], pRead[2], readbytes);


ErrorRelease:
	/* release data monitory (unprepare AFE data memory) */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MUTU_RAW_TYPE, &mode_type, 1);
	if (ret < 0)
		TS_LOG_ERR("Set rawdata type failed\n");

ErrorExit:
	kfree(pRead);

	return ret;
}

#define PRE_DEFINED_DATA_LENGTH		208
static int sec_ts_read_channel(struct sec_ts_data *ts, struct sec_ts_test_mode *mode)
{
	unsigned char *pRead = NULL;
	u8 mode_type = TYPE_INVALID_DATA;
	int ret = 0;
	int ii = 0;
	int jj = 0;
	unsigned int data_length = (ts->tx_count + ts->rx_count) * 2;
	u8 w_data = 0;

	TS_LOG_ERR("%s\n", __func__);

	pRead = kzalloc(data_length, GFP_KERNEL);
	if (!pRead) {
		TS_LOG_ERR("%s: out of memory\n", __func__);
		return -ENOMEM;
	}

	/* set OPCODE and data type */
	w_data = mode->type;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELF_RAW_TYPE, &w_data, 1);
	if (ret < 0) {
		TS_LOG_ERR("%s: Set rawdata type failed\n", __func__);
		goto out_read_channel;
	}

	sec_ts_delay(50);

	if (mode->type == TYPE_OFFSET_DATA_SDC) {
		/* excute selftest for real cap offset data, because real cap data is not memory data in normal touch. */
		char para = TO_TOUCH_MODE;
		disable_irq(ts->client->irq);
		execute_selftest(ts, true);
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
		if (ret < 0) {
			TS_LOG_ERR("%s: set rawdata type failed!\n", __func__);
			enable_irq(ts->client->irq);
			goto err_read_data;
		}
		enable_irq(ts->client->irq);
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_TOUCH_SELF_RAWDATA, pRead, data_length);
	if (ret < 0) {
		TS_LOG_ERR("%s: read rawdata failed!\n", __func__);
		goto err_read_data;
	}

	memset(ts->pFrame, 0x00, data_length);

	mode->avg = mode->short_avg = 0;
	for (ii = 0; ii < data_length; ii += 2) {
		ts->pFrame[jj] = ((pRead[ii] << 8) | pRead[ii + 1]);

		if (ii == 0)
			mode->min = mode->max = ts->pFrame[jj];
		if (ii == (ts->tx_count * 2))
			mode->short_min = mode->short_max = ts->pFrame[jj];
		if (ii < (ts->tx_count * 2)) {
			mode->min = min(mode->min, ts->pFrame[jj]);
			mode->max = max(mode->max, ts->pFrame[jj]);
			mode->avg += ts->pFrame[jj];
		} else {
			mode->short_min = min(mode->short_min, ts->pFrame[jj]);
			mode->short_max = max(mode->short_max, ts->pFrame[jj]);
			mode->short_avg += ts->pFrame[jj];
		}

		TS_LOG_DEBUG("%s: [%s][%d] %d\n", __func__,
				(jj < ts->tx_count) ? "TX" : "RX", jj, ts->pFrame[jj]);
		jj++;
	}
	if (ts->tx_count && ts->rx_count) {
		mode->avg = (int)(mode->avg / ts->tx_count);
		mode->short_avg = (int)(mode->short_avg / ts->rx_count);
	} else {
		TS_LOG_ERR("%s: tx or rx count is zero\n", __func__);
	}

err_read_data:
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELF_RAW_TYPE, &mode_type, 1);
	if (ret < 0)
		TS_LOG_ERR("%s: Set rawdata type failed\n", __func__);

out_read_channel:
	kfree(pRead);

	return ret;
}


int sec_ts_read_raw_data(struct sec_ts_data *ts,
				struct sec_ts_test_mode *mode)
{
	int ret = 0;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: [ERROR] Touch is stopped\n",
				__func__);
		goto error_power_state;
	}

	TS_LOG_INFO("%s: %d, %s\n",
			__func__, mode->type, mode->allnode ? "ALL" : "");

	ret = sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to fix tmode\n",
				__func__);
		goto error_test_fail;
	}

	if (mode->frame_channel)
		ret = sec_ts_read_channel(ts, mode);
	else
		ret = sec_ts_read_frame(ts, mode);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to read frame\n",
				__func__);
		goto error_test_fail;
	}

	ret = sec_ts_release_tmode(ts);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to release tmode\n",
				__func__);
		goto error_test_fail;
	}

error_test_fail:
error_power_state:

	return ret;
}

int run_selftest_read_all(struct sec_ts_data *ts, struct sec_ts_test_mode *mode)
{
	memset(mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode->type = TYPE_OFFSET_DATA_SDC;
	mode->allnode = TEST_MODE_ALL_NODE;

	return sec_ts_read_raw_data(ts, mode);
}

int run_delta_read_all(struct sec_ts_data *ts, struct sec_ts_test_mode *mode)
{
	memset(mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode->type = TYPE_RAW_DATA;
	mode->allnode = TEST_MODE_ALL_NODE;

	return sec_ts_read_raw_data(ts, mode);
}

int run_self_rawcap_read_all(struct sec_ts_data *ts, struct sec_ts_test_mode *mode)
{
	memset(mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode->type = TYPE_OFFSET_DATA_SDC;
	mode->frame_channel = TEST_MODE_READ_CHANNEL;
	mode->allnode = TEST_MODE_ALL_NODE;

	return sec_ts_read_raw_data(ts, mode);
}

int run_self_delta_read_all(struct sec_ts_data *ts, struct sec_ts_test_mode *mode)
{
	memset(mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode->type = TYPE_NO_COMNOISE_DATA;
	mode->frame_channel = TEST_MODE_READ_CHANNEL;
	mode->allnode = TEST_MODE_ALL_NODE;

	if(ts->ic_name == S6SY761X){
		mode->type = TYPE_RAW_DATA;
	}

	return sec_ts_read_raw_data(ts, mode);
}

void sec_ts_set_channel_count(struct sec_ts_data *ts, struct ts_rawdata_info *info)
{
	info->buff[0] = ts->rx_count;
	info->buff[1] = ts->tx_count;
	info->used_size = 2;
}

int sec_ts_read_id_check(struct sec_ts_data *ts)
{
	u8 id_data[3] = { 0 };
	int ret = NO_ERR;

	memset(id_data, 0x0, sizeof(id_data));
	ret = ts->sec_ts_i2c_read (ts, SEC_TS_READ_ID, id_data, 3);
	if (ret < 0)
		return ret;

	if (id_data[0] == SEC_TS_APP_VER)
		return NO_ERR;
	else
		return -ENODATA;
}

/*
 * Get diff data  for debug sysfs interface
 */
int sec_ts_get_delta_debug(struct sec_ts_data *ts, struct ts_diff_data_info *info)
{
	int i = 0;
	int ret = NO_ERR;
	struct sec_ts_test_mode mode;
	int raw_count = ts->tx_count * ts->rx_count;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: Touch is stopped!\n", __func__);
		return -EIO;
	}

	memset(info->result, 0, sizeof(info->result));

	TS_LOG_ERR("%s: Get delta! tx=%d, rx=%d\n", __func__, ts->tx_count, ts->rx_count);
	ret = run_delta_read_all(ts, &mode);
	if (ret < 0) {
		TS_LOG_ERR("%s: run delta read error!\n", __func__);
		goto err_exit;
	}

	info->buff[0] = ts->tx_count;
	info->buff[1] = ts->rx_count;
	info->used_size += 2; /* head is tx num and rx num */

	for (i = 0; i < raw_count; i++)
		info->buff[i+2] = ts->pFrame[i];
	info->used_size += raw_count;

err_exit:
	return ret;
}

/*
 * Get rawcap data for debug sysfs interface
 */
int sec_ts_get_rawcap_debug(struct sec_ts_data *ts, struct ts_diff_data_info *info)
{
	int i = 0;
	int ret = NO_ERR;
	struct sec_ts_test_mode mode;
	u8 *rBuff = NULL;
	int result_size = SEC_TS_SELFTEST_REPORT_SIZE + ts->tx_count * ts->rx_count * 2;
	int raw_count = ts->tx_count * ts->rx_count;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: Touch is stopped!\n", __func__);
		return -EIO;
	}

	memset(info->result, 0, sizeof(info->result));

	TS_LOG_INFO("%s: Get rawcap!\n", __func__);
	ret = run_selftest_read_all(ts, &mode);
	if (ret < 0) {
		TS_LOG_ERR("%s: get raw data error!\n", __func__);
		goto err_exit;
	}

	rBuff = kzalloc(result_size, GFP_KERNEL);
	if (!rBuff)
		return -ENOMEM;

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_SELFTEST_RESULT, rBuff, result_size);
	if (ret < 0) {
		TS_LOG_ERR("%s: Selftest execution time out!\n", __func__);
		goto err_exit;
	}

	info->buff[0] = ts->tx_count;
	info->buff[1] = ts->rx_count;
	info->used_size += 2; /* head is tx num and rx num */

	for (i = 0; i < raw_count; i++) {
		info->buff[i+2] = (rBuff[SEC_TS_SELFTEST_REPORT_SIZE + (i * 2) + 1] << 8) +
					(rBuff[SEC_TS_SELFTEST_REPORT_SIZE + (i * 2)]);
	}

	info->used_size += raw_count;

err_exit:
	if (rBuff){
		kfree(rBuff);
		rBuff = NULL;
	}
	return ret;

}

int sec_ts_get_delta_result(struct sec_ts_data *ts,
					struct ts_rawdata_info *info,
					struct sec_ts_test_mode *mode)
{
	int i = 0;
	int ret = NO_ERR;
	struct ts_kit_device_data *chip_data = ts->chip_data;
	char result_buff[MAX_STR_LEN] = {0};
	int *buf_ptr = NULL;
	int raw_count = ts->tx_count * ts->rx_count;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: Touch is stopped!\n", __func__);
		return -EIO;
	}

	raw_count = ts->tx_count * ts->rx_count;
	if (info->used_size + raw_count > TS_RAWDATA_BUFF_MAX) {
		TS_LOG_ERR("%s: out of data buf\n", __func__);
		strncat(result_buff, "3F-", MAX_STR_LEN);
		return -ENOBUFS;
	}

	TS_LOG_ERR("%s: Get delta!\n", __func__);
	ret = run_delta_read_all(ts, mode);
	if (ret < 0) {
		TS_LOG_ERR("%s: run delta read error!\n", __func__);
		goto err_exit;
	}

	if ((mode->max < chip_data->raw_limit_buf[4]) &&
		(mode->min > chip_data->raw_limit_buf[5])) {
		strncat(result_buff, "3P-", MAX_STR_LEN);
	} else {
		strncat(result_buff, "3F-", MAX_STR_LEN);
		TS_LOG_ERR("mutual delta test fail-3F\n");
	}

	TS_LOG_INFO("mutual delta data:[%d,%d] limit:[%d,%d]\n",
			mode->max, mode->min,
			chip_data->raw_limit_buf[4],
			chip_data->raw_limit_buf[5]);

	strncat(info->result, result_buff, MAX_STR_LEN);
	buf_ptr = &info->buff[info->used_size];
	for (i = 0; i < raw_count; i++)
		buf_ptr[i] = ts->pFrame[i];
	info->used_size += raw_count;

err_exit:
	return ret;
}

int sec_ts_get_rawcapgap_result(struct sec_ts_data *ts,
					struct ts_rawdata_info *info,
					struct sec_ts_test_mode *mode)
{
	int i = 0, j = 0;
	int ret = NO_ERR;
	struct ts_kit_device_data *chip_data = ts->chip_data;
	char result_buff[MAX_STR_LEN] = {0};
	int raw_count = ts->tx_count * ts->rx_count;
	
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: Touch is stopped!\n", __func__);
		return -EIO;
	}

	raw_count = ts->tx_count * ts->rx_count;
	if (info->used_size + raw_count > TS_RAWDATA_BUFF_MAX) {
		TS_LOG_ERR("%s: out of data buf\n", __func__);
		strncat(result_buff, "2F-", MAX_STR_LEN);
		return -ENOBUFS;
	}

	TS_LOG_ERR("%s: Get rawcap!\n", __func__);

	mode->avg = 0;
	mode->max = 0;
	mode->min = ts->pFrame[0];

	for (i = 0; i < ts->tx_count; i++) {
		for (j = 0; j < ts->rx_count-1; j++) {
			int gap = abs(ts->pFrame[i * ts->rx_count + j] -
					ts->pFrame[i * ts->rx_count + j+1]);
			mode->avg += gap;
			mode->max = max(mode->max, gap);
			mode->min = min(mode->min, gap);
		}
	}

	for (i = 0; i < ts->tx_count-1; i++) {
		for (j = 0; j < ts->rx_count; j++) {
			int gap = abs(ts->pFrame[i * ts->rx_count + j] -
					ts->pFrame[(i+1) * ts->rx_count + j]);
			mode->avg += gap;
			mode->max = max(mode->max, gap);
			mode->min = min(mode->min, gap);
		}
	}

	if ((ts->tx_count*(ts->rx_count-1))+((ts->tx_count-1)*ts->rx_count))
		mode->avg /= ((ts->tx_count*(ts->rx_count-1))+((ts->tx_count-1)*ts->rx_count));
	else
		TS_LOG_ERR("%s: div count is zero\n", __func__);

	if ((mode->max < chip_data->raw_limit_buf[2]) &&
		(mode->min < chip_data->raw_limit_buf[3])) {
		strncat(result_buff, "2P-", MAX_STR_LEN);
	} else {
		strncat(result_buff, "2F-", MAX_STR_LEN);
		TS_LOG_ERR("mutual rawcap gap test fail-2F\n");
	}

	strncat(info->result, result_buff, MAX_STR_LEN);
	TS_LOG_INFO("mutual rawcap gap test data:[%d,%d] limit:[%d,%d]\n",
			mode->max, mode->min,
			chip_data->raw_limit_buf[2],
			chip_data->raw_limit_buf[3]);
	return ret;
}

int sec_ts_get_rawcap_result(struct sec_ts_data *ts,
					struct ts_rawdata_info *info,
					struct sec_ts_test_mode *mode)
{
	int i = 0;
	int ret = NO_ERR;
	struct ts_kit_device_data *chip_data = ts->chip_data;
	char result_buff[MAX_STR_LEN] = {0};
	u8 *rBuff = NULL;
	int *buf_ptr = NULL;
	int result_size = SEC_TS_SELFTEST_REPORT_SIZE + ts->tx_count * ts->rx_count * 2;
	int raw_count = ts->tx_count * ts->rx_count;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: Touch is stopped!\n", __func__);
		return -EIO;
	}

	raw_count = ts->tx_count * ts->rx_count;
	if (info->used_size + raw_count > TS_RAWDATA_BUFF_MAX) {
		TS_LOG_ERR("%s: out of data buf\n", __func__);
		strncat(result_buff, "1F-", MAX_STR_LEN);
		return -ENOBUFS;
	}

	TS_LOG_INFO("%s: Get rawcap!\n", __func__);
	rBuff = kzalloc(result_size, GFP_KERNEL);
	if (!rBuff) {
		TS_LOG_ERR("%s:rbuff alloc failed\n", __func__);
		return -ENOMEM;
	}
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_SELFTEST_RESULT, rBuff, result_size);
	if (ret < 0) {
		TS_LOG_ERR("%s: Selftest execution time out!\n", __func__);
		goto err_exit;
	}

	mode->avg = 0;
	mode->max = mode->min = (rBuff[SEC_TS_SELFTEST_REPORT_SIZE + 1] << 8) +
					rBuff[SEC_TS_SELFTEST_REPORT_SIZE + 0];

	buf_ptr = &info->buff[info->used_size];
	for (i = 0; i < raw_count; i++) {
		buf_ptr[i] = (rBuff[SEC_TS_SELFTEST_REPORT_SIZE + (i * 2) + 1] << 8) +
					(rBuff[SEC_TS_SELFTEST_REPORT_SIZE + (i * 2)]);
		mode->avg += buf_ptr[i];
		mode->max = max(mode->max, buf_ptr[i]);
		mode->min = min(mode->min, buf_ptr[i]);
	}
	if (raw_count)
		mode->avg /= raw_count;
	else
		TS_LOG_ERR("%s: raw_count count is zero\n", __func__);
	info->used_size += raw_count;

	if ((mode->max < chip_data->raw_limit_buf[0]) &&
			(mode->min > chip_data->raw_limit_buf[1])) {
		strncat(result_buff, "1P-", MAX_STR_LEN);
	} else {
		strncat(result_buff, "1F-", MAX_STR_LEN);

		TS_LOG_ERR("mutual rawcap test fail-1F\n");
	}
	strncat(info->result, result_buff, MAX_STR_LEN);
	TS_LOG_ERR("mutual rawcap data:[%d,%d] limit:[%d,%d]\n",
			mode->max, mode->min,
			chip_data->raw_limit_buf[0],
			chip_data->raw_limit_buf[1]);
err_exit:
	kfree(rBuff);
	return ret;

}

int sec_ts_get_openshort_result(struct sec_ts_data *ts, struct ts_rawdata_info *info)
{
	int ret = NO_ERR;
	u8 *rBuff = NULL;
	int result_size = SEC_TS_SELFTEST_REPORT_SIZE + ts->tx_count * ts->rx_count * 2;

	TS_LOG_INFO("%s: Get open/short!\n", __func__);
	rBuff = kzalloc(result_size, GFP_KERNEL);
	if (!rBuff)
		return -ENOMEM;

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_SELFTEST_RESULT, rBuff, result_size);
	if (ret < 0) {
		TS_LOG_ERR("%s: Selftest execution time out!\n", __func__);
		goto err_exit;
	}

	if ((rBuff[16] != 0) && (rBuff[17] != 0) &&
		(rBuff[18] != 0) && (rBuff[19] != 0)) {
		strncat(info->result, "5F-", MAX_STR_LEN);
		TS_LOG_ERR("openshort test fail-5F "
			"data: %d, %d, %d, %d\n",
			rBuff[16], rBuff[17], rBuff[18],
			rBuff[19]);
	} else {
		strncat(info->result, "5P-", MAX_STR_LEN);
	}
err_exit:
	kfree(rBuff);
	return ret;

}

int sec_ts_get_self_rawcap_result(struct sec_ts_data *ts,
					struct ts_rawdata_info *info, int offset,
					struct sec_ts_test_mode *mode)
{
	struct ts_kit_device_data *chip_data = ts->chip_data;
	int *buf_ptr = NULL;
	int i = 0;
	int ret = NO_ERR;
	int result_size = SEC_TS_SELFTEST_REPORT_SIZE + ts->tx_count * ts->rx_count * 2;
	int raw_count = 0;
	short read_cap = 0;
	u8 *rBuff = NULL;
	u8 *result_ptr = NULL;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: Touch is stopped!\n", __func__);
		return -EIO;
	}

	raw_count = ts->tx_count + ts->rx_count;
	if (offset + raw_count > TS_RAWDATA_BUFF_MAX) {
		TS_LOG_ERR("%s: out of data buf\n", __func__);
		strncat(info->result, "7F-", MAX_STR_LEN);
		return -ENOBUFS;
	}

	result_size += (raw_count * 2);
	TS_LOG_INFO("%s: Get rawcap!\n", __func__);
	rBuff = kzalloc(result_size, GFP_KERNEL);
	if (!rBuff) {
		TS_LOG_ERR("%s: out of memory 1\n", __func__);
		return -ENOMEM;
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_SELFTEST_RESULT, rBuff, result_size);
	if (ret < 0) {
		TS_LOG_ERR("%s: Selftest execution time out!\n", __func__);
		goto err_exit;
	}

	mode->short_avg = 0;
	buf_ptr = info->hybrid_buff + offset;
	result_ptr = rBuff + SEC_TS_SELFTEST_REPORT_SIZE;
	result_ptr += (ts->tx_count * ts->rx_count * 2);
	result_ptr += (ts->tx_count * 2);
	read_cap = (result_ptr[0]|(result_ptr[1] << 8));
	mode->short_max = mode->short_min = read_cap;
	for (i = 0; i < ts->rx_count * 2; i += 2) {
		read_cap = ((result_ptr[i + 1] << 8)|(result_ptr[i]));
		buf_ptr[i / 2] = read_cap;
		TS_LOG_DEBUG("%s: rx_data[%d]=%d\n", __func__, i/2, buf_ptr[i / 2]);
		mode->short_min = min(mode->short_min, buf_ptr[i / 2]);
		mode->short_max = max(mode->short_max, buf_ptr[i / 2]);
		mode->short_avg += buf_ptr[i / 2];
	}
	if (ts->rx_count)
		mode->short_avg = (mode->short_avg / ts->rx_count);
	else
		TS_LOG_ERR("%s: rx count is zero\n", __func__);

	mode->avg = 0;
	result_ptr -= (ts->tx_count * 2);
	read_cap = (int)((result_ptr[1] << 8)|result_ptr[0]);
	mode->max = mode->min = read_cap;
	buf_ptr = buf_ptr + ts->rx_count;
	for (i = 0; i < ts->tx_count * 2; i += 2) {
		read_cap = ((result_ptr[i + 1] << 8)|result_ptr[i]);
		buf_ptr[i / 2] = read_cap;
		TS_LOG_DEBUG("%s: tx_data[%d]=%d\n", __func__, i/2, buf_ptr[i / 2]);
		mode->min = min(mode->min, buf_ptr[i / 2]);
		mode->max = max(mode->max, buf_ptr[i / 2]);
		mode->avg += buf_ptr[i / 2];
	}
	if (ts->tx_count)
		mode->avg = (mode->avg / ts->tx_count);
	else
		TS_LOG_ERR("%s: tx count is zero\n", __func__);

	if ((mode->max < chip_data->raw_limit_buf[6]) &&
		(mode->min > chip_data->raw_limit_buf[7]) &&
		(mode->short_max < chip_data->raw_limit_buf[8]) &&
		(mode->short_min > chip_data->raw_limit_buf[9])) {
		strncat(info->result, "7P-", MAX_STR_LEN);
	} else {
		strncat(info->result, "7F-", MAX_STR_LEN);
		TS_LOG_ERR("self rawcap test fail-7F\n");
	}
	TS_LOG_INFO("self raw: col_data:[%d,%d] limit:[%d,%d] row_data:[%d,%d] limit:[%d,%d]\n",
			mode->max, mode->min,
			chip_data->raw_limit_buf[6],
			chip_data->raw_limit_buf[7],
			mode->short_max, mode->short_min,
			chip_data->raw_limit_buf[8],
			chip_data->raw_limit_buf[9]);
err_exit:
	kfree(rBuff);
	return ret;
}

int sec_ts_get_self_delta_result(struct sec_ts_data *ts,
					struct ts_rawdata_info *info, int offset,
					struct sec_ts_test_mode *mode)
{
	struct ts_kit_device_data *chip_data = ts->chip_data;
	int *buf_ptr = NULL;
	int i = 0;
	int ret = NO_ERR;

	ret = run_self_delta_read_all(ts, mode);
	if (ret < 0)
		return ret;

	if (offset + ts->rx_count + ts->tx_count > TS_RAWDATA_BUFF_MAX) {
		TS_LOG_ERR("%s: out of data buf\n", __func__);
		strncat(info->result, "7F-", MAX_STR_LEN);
		return -ENOBUFS;
	}

	buf_ptr = info->hybrid_buff + offset;
	for (i = 0; i < ts->rx_count; i++) {
		buf_ptr[i] = ts->pFrame[ts->tx_count + i];
	}
	for (i = 0; i < ts->tx_count; i++) {
		buf_ptr[ts->rx_count + i] = ts->pFrame[i];
	}
	if ((mode->max < chip_data->raw_limit_buf[10]) &&
		(mode->min > chip_data->raw_limit_buf[11]) &&
		(mode->short_max < chip_data->raw_limit_buf[12]) &&
		(mode->short_min > chip_data->raw_limit_buf[13])) {
		strncat(info->result, "9P-", MAX_STR_LEN);
	} else {
		strncat(info->result, "9F-", MAX_STR_LEN);
		TS_LOG_ERR("self delta test fail-9F\n");
	}

	TS_LOG_INFO("self delta: col_data:[%d,%d] limit:[%d,%d] row_data:[%d,%d] limit:[%d,%d]\n",
			mode->max, mode->min,
			chip_data->raw_limit_buf[10],
			chip_data->raw_limit_buf[11],
			mode->short_max, mode->short_min,
			chip_data->raw_limit_buf[12],
			chip_data->raw_limit_buf[13]);

	return ret;
}

static void sec_ts_run_caldata_read_all(struct sec_ts_data *ts,
		struct ts_rawdata_info *info, struct sec_ts_test_mode *mode)
{
	int readbytes = ts->rx_count * ts->tx_count;
	int i = 0;
	int index = info->used_size;

	memset(mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode->type = TYPE_OFFSET_DATA_SEC;
	mode->allnode = TEST_MODE_ALL_NODE;
	sec_ts_read_raw_data(ts, mode);

	if (info->used_size + readbytes > TS_RAWDATA_BUFF_MAX) {
		TS_LOG_ERR("%s: out of data buf\n", __func__);
		return;
	}

	for (i = 0; i < readbytes; i++)
		info->buff[i + index] = ts->pFrame[i];

	info->used_size += readbytes;
}


int sec_ts_run_self_test(struct sec_ts_data *ts, struct ts_rawdata_info *info)
{
	int ret = NO_ERR;
	struct ts_kit_device_data *chip_data = ts->chip_data;
	struct sec_ts_test_mode mode[SEC_TS_SELFTEST_ITEM_MAX];
	char result_buff[SEC_CMD_STR_LEN] = {0};
	int raw_count = ts->tx_count * ts->rx_count;
	int sf_raw_count = ts->tx_count + ts->rx_count;
	int sf_offset = 0;
	int ibuff_offset = 0;
	u8 img_ver[4] = {0,};

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: Touch is stopped!\n", __func__);
		return -ENOMEM;
	}
	if(ts->ic_name == S6SY761X) {
		wake_lock(&ts->wakelock);//add wakelock ,avoid i2c suspend
	}
	raw_count = ts->tx_count * ts->rx_count;
	memset(info->result, 0, sizeof(info->result));

	TS_LOG_INFO("%s: tx_count+%d, rx_count=%d\n", __func__,
			ts->tx_count, ts->rx_count);

	sec_ts_set_channel_count(ts, info);

	TS_LOG_INFO("%s: Touch ID check!\n", __func__);
	ret = sec_ts_read_id_check(ts);
	if (ret == NO_ERR)
		strncat(info->result, "0P-", MAX_STR_LEN);
	else
		strncat(info->result, "0F-", MAX_STR_LEN);


	TS_LOG_INFO("%s: run selftest rawcap!\n", __func__);
	ret = run_selftest_read_all(ts, mode);
	if (ret < 0) {
		TS_LOG_ERR("%s: run selfttest error!\n", __func__);
		goto err_exit;
	}

	ret = sec_ts_get_rawcap_result(ts, info, &mode[SEC_TS_RAWCAP_TEST]);
	if (ret < 0) {
		TS_LOG_ERR("%s: get rawcap fail!\n", __func__);
		goto err_exit;
	}

	ret = sec_ts_get_rawcapgap_result(ts, info, &mode[SEC_TS_RAWCAPGAP_TEST]);
	if (ret < 0) {
		TS_LOG_ERR("%s: get delta gap fail!\n", __func__);
		goto err_exit;
	}

	ret = sec_ts_get_delta_result(ts, info, &mode[SEC_TS_DETLA_TEST]);
	if (ret < 0) {
		TS_LOG_ERR("%s: get delta fail!\n", __func__);
		goto err_exit;
	}

	ret = sec_ts_get_openshort_result(ts, info);
	if (ret < 0) {
		TS_LOG_ERR("%s: get open/short fail!\n", __func__);
		goto err_exit;
	}

	chip_data->self_cap_test = true;

	info->hybrid_buff[0] = ts->rx_count;
	info->hybrid_buff[1] = ts->tx_count;
	sf_offset = (2 + sf_raw_count);
	ret = sec_ts_get_self_rawcap_result(ts, info, sf_offset, &mode[SEC_TS_SELF_CAP_TEST]);
	if (ret < 0) {
		TS_LOG_ERR("%s: get self rawcap fail!\n", __func__);
		goto err_exit;
	}

	sf_offset = 2;
	ret = sec_ts_get_self_delta_result(ts, info, sf_offset, &mode[SEC_TS_SELF_DETLA_TEST]);
	if (ret < 0) {
		TS_LOG_ERR("%s: get self delta fail!\n", __func__);
		goto err_exit;
	}

	sec_ts_run_caldata_read_all(ts, info, &mode[SEC_TS_CALDATA_TEST]);

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_IMG_VERSION, img_ver, 4);
	if (ret < 0) {
		TS_LOG_ERR("%s: Image version read error\n", __func__);
		goto err_exit;
	}
	TS_LOG_INFO("%s: IC Image version info : %x.%x.%x.%x\n",
			__func__, img_ver[0], img_ver[1], img_ver[2], img_ver[3]);

err_exit:
	if(ts->ic_name == S6SY761X) {
		wake_unlock(&ts->wakelock);//add wakelock ,avoid i2c suspend
	}
	snprintf(result_buff, SEC_CMD_STR_LEN,
		"[%d,%d,%d],[%d,%d,%d],[%d,%d,%d],[%d,%d,%d];",
		mode[SEC_TS_RAWCAP_TEST].avg, mode[SEC_TS_RAWCAP_TEST].max, mode[SEC_TS_RAWCAP_TEST].min,
		mode[SEC_TS_RAWCAPGAP_TEST].avg, mode[SEC_TS_RAWCAPGAP_TEST].max, mode[SEC_TS_RAWCAPGAP_TEST].min,
		mode[SEC_TS_DETLA_TEST].avg, mode[SEC_TS_DETLA_TEST].max, mode[SEC_TS_DETLA_TEST].min,
		mode[SEC_TS_CALDATA_TEST].avg, mode[SEC_TS_CALDATA_TEST].max, mode[SEC_TS_CALDATA_TEST].min);

	/* add failed reason */
	if (ret < 0) {
		strcat(result_buff, "F-software_reason");
	} else if (strchr(info->result, 'F')) {
		strcat(result_buff, "-panel_reason");
		TS_LOG_ERR("%s:self test fail panel_reason: %s\n",
				__func__, result_buff);
	}
	ibuff_offset = strlen(result_buff);

	if(ts->ic_name == S6SY761X){
		snprintf(result_buff + ibuff_offset, SEC_CMD_STR_LEN - ibuff_offset, "%s-%s%s-%x.%x.%x.%x",
			ts->module_name, SEC_TS_VENDOR_NAME, ts->project_id,
			img_ver[0], img_ver[1], img_ver[2], img_ver[3]);
	} else {
		snprintf(result_buff + ibuff_offset, SEC_CMD_STR_LEN - ibuff_offset, "%s-%s%s-%x.%x.%x.%x",
			ts->module_name, SEC_TS_VENDOR_NAME, SEC_TS_HW_PROJECTID,
			img_ver[0], img_ver[1], img_ver[2], img_ver[3]);
	}

	strncat(info->result, result_buff, TS_RAWDATA_RESULT_MAX-1);
	TS_LOG_INFO("%s, %s\n", __func__, info->result);

	return ret;
}

