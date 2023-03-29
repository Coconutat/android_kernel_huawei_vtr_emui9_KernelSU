/*
 * Copyright (C) 2011 Samsung Electronics Co., Ltd.
 * http://www.samsungsemi.com/
 *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

struct sec_ts_data *tsp_info;

#include "sec_ts.h"
#if defined (CONFIG_TEE_TUI)
#include "tui.h"
#endif
#include "../../tpkit_platform_adapter.h"

static struct mutex wrong_touch_lock;
static struct completion roi_data_done;
static u8 roi_data_staled;
static u8 roi_switch_on;
static unsigned char roi_data[ROI_DATA_READ_LENGTH] = { 0 };

#if defined (CONFIG_TEE_TUI)
extern struct ts_tui_data tee_tui_data;
#endif
static int sec_ts_chip_detect(struct ts_kit_platform_data *data);
static int sec_ts_wrong_touch(void);
static int sec_ts_init_chip(void);
static int sec_ts_parse_dt(struct device_node *device, struct ts_kit_device_data *chip_data);
static int sec_ts_irq_top_half(struct ts_cmd_node *cmd);
static int sec_ts_irq_bottom_half(struct ts_cmd_node *in_cmd,
				     struct ts_cmd_node *out_cmd);
static int sec_ts_fw_update_boot(char *file_name);
static int sec_ts_fw_update_sd(void);
static int sec_ts_chip_get_info(struct ts_chip_info_param *info);
static int sec_ts_set_info_flag(struct ts_kit_platform_data *info);
static int sec_ts_suspend(void);
static int sec_ts_resume(void);
static int sec_ts_wakeup_gesture_enable_switch(
	struct ts_wakeup_gesture_enable_info *info);
static void sec_ts_shutdown(void);
static int sec_ts_input_config(struct input_dev *input_dev);
static int sec_ts_get_debug_data(struct ts_diff_data_info *info,
				    struct ts_cmd_node *out_cmd);
static int sec_ts_get_rawdata(struct ts_rawdata_info *info,
				 struct ts_cmd_node *out_cmd);
static int sec_ts_get_calibration_data(struct ts_calibration_data_info *info,
		struct ts_cmd_node *out_cmd);
static int sec_ts_get_calibration_info(struct ts_calibration_info_param *info,
		struct ts_cmd_node *out_cmd);
static int sec_ts_glove_switch(struct ts_glove_info *info);
static int sec_ts_charger_switch(struct ts_charger_info *info);
static int sec_ts_holster_switch(struct ts_holster_info *info);
static int sec_ts_roi_switch(struct ts_roi_info *info);
static unsigned char *sec_ts_roi_rawdata(void);
static int sec_ts_chip_get_capacitance_test_type(
		struct ts_test_type_info *info);
static int sec_ts_calibrate(void);
static int sec_ts_regs_operate(struct ts_regs_info *info);
static void sec_ts_chip_touch_switch(void);
int sec_ts_read_information(struct sec_ts_data *ts);
void sec_ts_work_after_input_kit(void);
static int sec_ts_after_resume (void* feature_info);

struct ts_device_ops ts_kit_sec_ts_ops = {
	.chip_detect = sec_ts_chip_detect,
	.chip_init = sec_ts_init_chip,
	.chip_parse_config = sec_ts_parse_dt,
	.chip_input_config = sec_ts_input_config,
	.chip_irq_top_half = sec_ts_irq_top_half,
	.chip_irq_bottom_half = sec_ts_irq_bottom_half,
	.chip_fw_update_boot = sec_ts_fw_update_boot,
	.chip_fw_update_sd = sec_ts_fw_update_sd,
	.chip_get_info = sec_ts_chip_get_info,
	.chip_get_capacitance_test_type =
		sec_ts_chip_get_capacitance_test_type,
	.chip_set_info_flag = sec_ts_set_info_flag,
	.chip_suspend = sec_ts_suspend,
	.chip_resume = sec_ts_resume,
	.chip_after_resume = sec_ts_after_resume,
	.chip_wakeup_gesture_enable_switch =
		sec_ts_wakeup_gesture_enable_switch,
	.chip_get_rawdata = sec_ts_get_rawdata,
	.chip_get_calibration_data = sec_ts_get_calibration_data,
	.chip_get_calibration_info = sec_ts_get_calibration_info,
	.chip_get_debug_data = sec_ts_get_debug_data,
	.chip_glove_switch = sec_ts_glove_switch,
	.chip_shutdown = sec_ts_shutdown,
	.chip_charger_switch = sec_ts_charger_switch,
	.chip_holster_switch = sec_ts_holster_switch,
	.chip_roi_switch = sec_ts_roi_switch,
	.chip_roi_rawdata = sec_ts_roi_rawdata,
	.chip_regs_operate = sec_ts_regs_operate,
	.chip_calibrate = sec_ts_calibrate,
	.chip_wrong_touch = sec_ts_wrong_touch,
	.chip_work_after_input = sec_ts_work_after_input_kit,
	.chip_touch_switch = sec_ts_chip_touch_switch,
};

int sec_ts_i2c_write(struct sec_ts_data *ts, u8 reg, u8 *data, int len)
{
	u8 buf[I2C_WRITE_BUFFER_SIZE + 1];
	int ret = NO_ERR;

	if (len > I2C_WRITE_BUFFER_SIZE) {
		TS_LOG_ERR("%s: len is larger than buffer size\n", __func__);
		return -EINVAL;
	}

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: POWER_STATUS : OFF\n", __func__);
		goto err;
	}

	buf[0] = reg;
	memcpy(buf + 1, data, len);

	mutex_lock(&ts->i2c_mutex);
	if (!ts->chip_data->ts_platform_data->bops->bus_write) {
		TS_LOG_ERR("bus_write not exits\n");
		ret = -EIO;
		goto mutex_err;
	}
	ret = ts->chip_data->ts_platform_data->bops->bus_write(buf, len + 1);
	if (ret < 0) {
		TS_LOG_ERR("bus_write failed, retval = %d\n", ret);
		goto mutex_err;
	}

	mutex_unlock(&ts->i2c_mutex);

	return ret;
mutex_err:
	mutex_unlock(&ts->i2c_mutex);
err:
	return -EIO;
}

static int sec_ts_i2c_short_read(struct sec_ts_data *ts, u8 reg, u8 *data, u16 len)
{
	struct ts_kit_platform_data *platform_data;
	int ret = NO_ERR;
	unsigned char retry;

	platform_data = ts->chip_data->ts_platform_data;

	for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
		ret = platform_data->bops->bus_read(&reg, 1, data, len);
		if (ret == NO_ERR)
			break;
		usleep_range(1 * 1000, 1 * 1000);
		if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
			TS_LOG_ERR("%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
			goto err;
		}

		if (retry > 1) {
			TS_LOG_ERR("%s: I2C retry %d\n", __func__, retry + 1);
		}
	}
	return ret;

err:
	return -EIO;

}

int sec_ts_i2c_long_read(struct sec_ts_data *ts, u8 reg, u8 *data, u16 len)
{
	struct ts_kit_platform_data *platform_data;
	int ret;
	int recv_len;
	unsigned char retry;
	u8 *buf = data;
	u16 remain = len;

	platform_data = ts->chip_data->ts_platform_data;

	if (remain > ts->i2c_burstmax) {
		recv_len = ts->i2c_burstmax;
		remain -= ts->i2c_burstmax;
	} else {
		recv_len = remain;
	}

	TS_LOG_INFO("%s: I2C exceed burstmax %d, %d\n", __func__, remain, ts->i2c_burstmax);
	for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
		ret = platform_data->bops->bus_read(&reg, 1, buf, recv_len);
		if (ret == NO_ERR)
			break;
		usleep_range(1 * 1000, 1 * 1000);
		if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
			TS_LOG_ERR("%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
			goto err;
		}

		if (retry > 1) {
			TS_LOG_ERR("%s: I2C retry %d\n", __func__, retry + 1);
		}
	}
	buf += recv_len;

	if (remain > 0) {
		do {
			if (remain > ts->i2c_burstmax) {
				recv_len = ts->i2c_burstmax;
				remain -= ts->i2c_burstmax;
			} else {
				recv_len = remain;
				remain = 0;
			}

			TS_LOG_DEBUG("%s: I2C exceed burstmax %d\n", __func__, remain);
			for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
				ret = platform_data->bops->bus_read(&reg, 0, buf, recv_len);
				if (ret == NO_ERR)
					break;
				usleep_range(1 * 1000, 1 * 1000);
				if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
					TS_LOG_ERR("%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
					goto err;
				}

				if (retry > 1) {
					TS_LOG_ERR("%s: I2C retry %d\n", __func__, retry + 1);
					remain = 0;
				}
			}

			buf += recv_len;

		} while (remain > 0);
	}
	return ret;

err:
	return -EIO;

}

int sec_ts_i2c_read(struct sec_ts_data *ts, u8 reg, u8 *data, int len)
{
	int ret;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: POWER_STATUS : OFF\n", __func__);
		return -EIO;
	}

	mutex_lock(&ts->i2c_mutex);

	if (len <= ts->i2c_burstmax) {
		ret = sec_ts_i2c_short_read(ts, reg, data, len);

	} else {
		ret = sec_ts_i2c_long_read(ts, reg, data, len);
	}

	mutex_unlock(&ts->i2c_mutex);

	if (ret < 0) {
		TS_LOG_ERR("%s: I2C read fail over retry limit\n", __func__);
		ret = -EIO;
	}
	return ret;

}

static int sec_ts_i2c_write_burst(struct sec_ts_data *ts, u8 *data, int len)
{
	int ret;
	int retry;

	mutex_lock(&ts->i2c_mutex);

	for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
		if (!ts->chip_data->ts_platform_data->bops->bus_write) {
			TS_LOG_ERR("bus_write not exits\n");
			ret = -EIO;
			goto err;
		}
		ret = ts->chip_data->ts_platform_data->bops->bus_write(data, len);
		if (ret == NO_ERR)
			break;

		usleep_range(1 * 1000, 1 * 1000);

		if (retry > 1) {
			TS_LOG_ERR("%s: I2C retry %d\n", __func__, retry + 1);
		}
	}

	mutex_unlock(&ts->i2c_mutex);
	if (retry == SEC_TS_I2C_RETRY_CNT) {
		TS_LOG_ERR("%s: I2C write over retry limit\n", __func__);
		ret = -EIO;
	}
err:
	return ret;
}

static int sec_ts_i2c_read_bulk(struct sec_ts_data *ts, u8 *data, int len)
{
	int ret;
	unsigned char retry;
	int remain = len;
	struct i2c_msg msg;
	u8 reg = 0;

	mutex_lock(&ts->i2c_mutex);

	do {
		if (remain > ts->i2c_burstmax)
			msg.len = ts->i2c_burstmax;
		else
			msg.len = remain;

		remain -= ts->i2c_burstmax;

		for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
			ret = ts->chip_data->ts_platform_data->bops->bus_read(&reg, 0, data,
														len);
			if (ret == NO_ERR)
				break;
			usleep_range(1 * 1000, 1 * 1000);

			if (retry > 1) {
				TS_LOG_ERR("%s: I2C retry %d\n", __func__, retry + 1);
			}
		}

		if (retry == SEC_TS_I2C_RETRY_CNT) {
			TS_LOG_ERR("%s: I2C read over retry limit\n", __func__);
			ret = -EIO;

			break;
		}
		msg.buf += msg.len;

	} while (remain > 0);

	mutex_unlock(&ts->i2c_mutex);

	if (ret == NO_ERR)
		return 0;

	return -EIO;
}

void sec_ts_delay(unsigned int ms)
{
	if (ms < 20)
		usleep_range(ms * 1000, ms * 1000);
	else
		msleep(ms);
}

static int sec_ts_i2c_boot_check(struct sec_ts_data *ts)
{
	int rc = -ENODATA;
	int retry = 0;
	u8 tBuff[SEC_TS_STATUSEVENT_SIZE] = {0,};

	while ((sec_ts_i2c_read(ts, SEC_TS_READ_ONE_EVENT, tBuff,
					SEC_TS_STATUSEVENT_SIZE) == NO_ERR)) {
		if (((tBuff[0] >> 2) & 0xF) == TYPE_STATUS_EVENT_INFO) {
			if (tBuff[1] == SEC_TS_ACK_BOOT_COMPLETE) {
				rc = NO_ERR;
				break;
			}
		}

		if (retry++ > SEC_TS_I2C_RETRY_CNT) {
			TS_LOG_ERR("%s: Time Over\n", __func__);
			break;
		}
		sec_ts_delay(20);
	}

	TS_LOG_INFO(
		"%s: %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X [%d]\n",
		__func__, tBuff[0], tBuff[1], tBuff[2], tBuff[3],
		tBuff[4], tBuff[5], tBuff[6], tBuff[7], retry);

	return rc;
}

int sec_ts_wait_for_ready(struct sec_ts_data *ts, unsigned int ack)
{
	int rc = -ENODATA;
	int retry = 0;
	u8 tBuff[SEC_TS_STATUSEVENT_SIZE] = {0,};

	while ((sec_ts_i2c_read(ts, SEC_TS_READ_ONE_EVENT, tBuff,
					SEC_TS_STATUSEVENT_SIZE) == NO_ERR)) {
		if (((tBuff[0] >> 2) & 0xF) == TYPE_STATUS_EVENT_INFO) {
			if (tBuff[1] == ack) {
				rc = NO_ERR;
				break;
			}
		} else if (((tBuff[0] >> 2) & 0xF) == TYPE_STATUS_EVENT_VENDOR_INFO) {
			if (tBuff[1] == ack) {
				rc = NO_ERR;
				break;
			}
		}

		if (retry++ > SEC_TS_WAIT_RETRY_CNT) {
			TS_LOG_ERR("%s: Time Over\n", __func__);
			break;
		}
		sec_ts_delay(20);
	}

	TS_LOG_INFO(
		"%s: %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X [%d]\n",
		__func__, tBuff[0], tBuff[1], tBuff[2], tBuff[3],
		tBuff[4], tBuff[5], tBuff[6], tBuff[7], retry);

	return rc;
}

static void sec_ts_chip_touch_switch(void)
{
	unsigned char value = 0;
	unsigned long get_value = 0;
	char *ptr_begin = NULL, *ptr_end = NULL;
	char in_data[MAX_STR_LEN] = {0};
	int len = 0;
	unsigned char stype = 0, soper = 0, param = 0;
	u8 buff[2];
	int error = 0;

	TS_LOG_INFO("%s +\n", __func__);

	if (NULL == tsp_info->chip_data) {
		TS_LOG_ERR("error chip data\n");
		goto out;
	}

	if (TS_SWITCH_TYPE_DOZE !=
		(tsp_info->chip_data->touch_switch_flag & TS_SWITCH_TYPE_DOZE)) {
		TS_LOG_ERR("touch switch not supprot\n");
		goto out;
	}

	/* SWITCH_OPER,ENABLE_DISABLE,PARAM */
	memcpy(in_data, tsp_info->chip_data->touch_switch_info, MAX_STR_LEN);
	TS_LOG_INFO("in_data:%s\n", in_data);

	/* get switch type */
	ptr_begin = in_data;
	ptr_end = strstr(ptr_begin, ",");
	if (!ptr_end) {
		TS_LOG_ERR("%s get stype fail\n", __func__);
		goto out;
	}
	len = ptr_end - ptr_begin;
	if (len <= 0 || len > 3) {
		TS_LOG_ERR("%s stype len error\n", __func__);
		goto out;
	}
	*ptr_end = 0;
	error = strict_strtoul(ptr_begin, 0, &get_value);
	if (error) {
		TS_LOG_ERR("strict_strtoul return invaild :%d\n", error);
		goto out;
	} else {
		stype = (unsigned char)get_value;
		TS_LOG_INFO("%s get stype:%u\n", __func__, stype);
	}

	/* get switch operate */
	ptr_begin = ptr_end + 1;
	if (!ptr_begin) {
		TS_LOG_ERR("%s get soper fail\n", __func__);
		goto out;
	}
	ptr_end = strstr(ptr_begin, ",");
	if (!ptr_end) {
		TS_LOG_ERR("%s get soper fail\n", __func__);
		goto out;
	}
	len = ptr_end - ptr_begin;
	if (len <= 0 || len > 3) {
		TS_LOG_ERR("%s soper len error\n", __func__);
		goto out;
	}
	*ptr_end = 0;
	error = strict_strtoul(ptr_begin, 0, &get_value);
	if (error) {
		TS_LOG_ERR("strict_strtoul return invaild :%d\n", error);
		goto out;
	} else {
		soper = (unsigned char)get_value;
		TS_LOG_INFO("%s get soper:%u\n", __func__, soper);
	}

	/* get param */
	ptr_begin = ptr_end + 1;
	if (!ptr_begin) {
		TS_LOG_ERR("%s get param fail\n", __func__);
		goto out;
	}
	error = strict_strtoul(ptr_begin, 0, &get_value);
	if (error) {
		TS_LOG_ERR("strict_strtoul return invaild :%d\n", error);
		goto out;
	} else {
		param = (unsigned char)get_value;
		TS_LOG_INFO("%s get param:%u\n", __func__, param);
	}

	if (TS_SWITCH_TYPE_DOZE != (stype & TS_SWITCH_TYPE_DOZE)) {
		TS_LOG_ERR("stype not  TS_SWITCH_TYPE_DOZE:%d, invalid\n", stype);
		goto out;
	}

	switch (soper) {
	case TS_SWITCH_DOZE_ENABLE:
		/* 0x000C Value"0"->On; Value"1"->Off */
		value = 0;
		if (param > 0) {
			buff[0] = (((param * 100) >> 8) & 0xff); /* MSB */
			buff[1] = ((param * 100) & 0xff); /* LSB */
			/* 0x0010 Value"1"(Dex)->1ms; Max->25500(Dex) */
			error = sec_ts_i2c_write(tsp_info,
						SEC_TS_CMD_DOZE_PARAM,
						buff, 2);
			if (error < 0) {
				TS_LOG_ERR("write doze hold off time addr:%02x=%u error\n",
					tsp_info->chip_data->touch_switch_hold_off_reg, param);
				break;
			}
		}
		break;
	case TS_SWITCH_DOZE_DISABLE:
		/* Value"0"->On; Value"1"->Off */
		buff[0] = ((tsp_info->idle_delay >> 8) & 0xff); /* MSB */
		buff[1] = (tsp_info->idle_delay & 0xff); /* LSB */

		error = sec_ts_i2c_write(tsp_info,
			SEC_TS_CMD_DOZE_PARAM, buff, 2);
		if (error < 0) {
			TS_LOG_ERR("write doze delay:%u=%u error\n",
				tsp_info->chip_data->touch_switch_reg, value);
			break;
		}

		break;
	default:
		TS_LOG_ERR("soper unknown:%d, invalid\n", soper);
		break;
	}

	TS_LOG_INFO("%s -\n", __func__);
out:
	return ;
}

void sec_ts_work_after_input_kit(void)
{
#ifdef ROI
	int i;
	int retval = NO_ERR;
	unsigned short roi_data_addr = 0;
	unsigned char roi_tmp[ROI_DATA_READ_LENGTH] = {0};

	if (roi_switch_on)  {
		if (roi_data_staled == 0)  {  /* roi_data is up to date now. */
			return;
		}

		/* We are about to refresh roi_data. To avoid stale output, use a completion to block possible readers. */
		reinit_completion(&roi_data_done);

		/* Request sensorhub to report fingersense data. */
		/* preread_fingersense_data(); */

		roi_data_addr =
			tsp_info->chip_data->ts_platform_data->feature_info.roi_info.roi_data_addr;
		TS_LOG_DEBUG("roi_data_addr=0x%04x\n", roi_data_addr);

		retval = sec_ts_i2c_read(tsp_info, roi_data_addr,
						roi_tmp, ROI_DATA_READ_LENGTH);
		if (retval < 0) {
			TS_LOG_ERR("F12 Failed to read roi data, retval= %d \n",
					retval);
		} else {
			/* roi_data pointer will be exported to app by function synaptics_roi_rawdata, so there's a race
			*  condition while app reads roi_data and our driver refreshes it at the same time. We use local
			*   variable(roi_tmp) to decrease the conflicting time window. Thus app will get consistent data,
			*   except when we are executing the following memcpy.
			*/
			memcpy(roi_data, roi_tmp, sizeof(u8) * 4);
			for (i = 4; i < ROI_DATA_READ_LENGTH; i += 2) {
				roi_data[i] = roi_tmp[i + 1];
				roi_data[i + 1] = roi_tmp[i];
			}
		}

		roi_data_staled = 0;
		complete_all(&roi_data_done);  /* If anyone has been blocked by us, wake it up. */
	}
#endif
}


int sec_ts_read_calibration_report(struct sec_ts_data *ts)
{
	int ret;
	u8 buf[5] = { 0 };

	buf[0] = SEC_TS_READ_CALIBRATION_REPORT;

	ret = sec_ts_i2c_read(ts, buf[0], &buf[1], 4);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to read, %d\n", __func__, ret);
		return ret;
	}

	TS_LOG_INFO("%s: count:%d, pass count:%d, fail count:%d, status:0x%X\n",
				__func__, buf[1], buf[2], buf[3], buf[4]);

	return buf[4];
}

static void sec_ts_swap(u8 *a, u8 *b)
{
	u8 temp = *a;

	*a = *b;
	*b = temp;
}

static void rearrange_byte(u8 *data, int length)
{
	int i;

	for(i = 0; i < length; i += 2) {
		sec_ts_swap(&data[i], &data[i + 1]);
	}
}

static void sec_ts_reinit(struct sec_ts_data *ts)
{
	int ret = NO_ERR;
	u8 w_data[2] = {0x00, 0x00};

	TS_LOG_ERR("%s: charger=0x%x, Cover=0x%x, Power mode=0x%x\n", __func__, ts->charger_mode, ts->touch_functions, ts->lowpower_status);


	if (ts->charger_mode != SEC_TS_BIT_CHARGER_MODE_NO) {
		w_data[0] = ts->charger_mode;
		ret = ts->sec_ts_i2c_write(ts, SET_TS_CMD_SET_CHARGER_MODE, (u8 *)&w_data[0], 1);
		if (ret < 0) {
			TS_LOG_ERR("%s: Failed to send command(0x%x)", __func__, SET_TS_CMD_SET_CHARGER_MODE);
			goto reinit_err;
		}
	}
	if (ts->touch_functions & SEC_TS_BIT_SETFUNC_COVER) {
		w_data[0] = ts->cover_cmd;
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_COVERTYPE, (u8 *)&w_data[0], 1);
		if (ret < 0) {
			TS_LOG_ERR("%s: Failed to send command(0x%x)", __func__, SEC_TS_CMD_SET_COVERTYPE);
			goto reinit_err;
		}
		w_data[0] = ((ts->touch_functions >> 8) & 0xff);
		w_data[1] = (ts->touch_functions & 0xff);

		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHFUNCTION, (u8 *)&(ts->touch_functions), 2);
		if (ret < 0) {
			TS_LOG_ERR("%s: Failed to send command(0x%x)", __func__, SEC_TS_CMD_SET_TOUCHFUNCTION);
			goto reinit_err;
		}
	}
	if (ts->lowpower_status == TO_LOWPOWER_MODE) {
		w_data[0] = (ts->lowpower_mode & SEC_TS_MODE_LOWPOWER_FLAG) >> 1;
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_WAKEUP_GESTURE_MODE, (u8 *)&w_data[0], 1);
		if (ret < 0) {
			TS_LOG_ERR("%s: Failed to send command(0x%x)", __func__, SEC_TS_CMD_WAKEUP_GESTURE_MODE);
			goto reinit_err;
		}

		w_data[0] = TO_LOWPOWER_MODE;
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, (u8 *)&w_data[0], 1);
		if (ret < 0) {
			TS_LOG_ERR("%s: Failed to send command(0x%x)", __func__, SEC_TS_CMD_SET_POWER_MODE);
			goto reinit_err;
		}
		sec_ts_delay(50);
	}
	return;
reinit_err:
	TS_LOG_ERR("%s: Failed to sec_ts_reinit", __func__);

}

static int sec_ts_process_status(struct sec_ts_data *ts,
					struct ts_fingers *info, u8 *event_buff)
{
	int ret;
	struct sec_ts_event_status *p_event_status;
	struct sec_ts_selfcheck *p_selfcheck;
	u8 rBuff[SEC_TS_SELFCHECK_BUFF_SIZE];

	p_event_status = (struct sec_ts_event_status *)event_buff;

	/* tchsta == 0 && ttype == 0 && eid == 0 : buffer empty */
	if ((event_buff[0] == 0) && (event_buff[1] == 0))
		return -ENODATA;
	if (p_event_status->stype > 0)
		TS_LOG_INFO("%s: STATUS %x %x %x %x %x %x %x %x\n", __func__,
				event_buff[0], event_buff[1], event_buff[2],
				event_buff[3], event_buff[4], event_buff[5],
				event_buff[6], event_buff[7]);

	/* watchdog reset -> send SENSEON command */
	if ((p_event_status->stype == TYPE_STATUS_EVENT_INFO) &&
		(p_event_status->status_id == SEC_TS_ACK_BOOT_COMPLETE) &&
		(p_event_status->status_data_1 == 0x20)) {

		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
		if (ret < 0)
			TS_LOG_ERR("%s: fail to write Sense_on\n", __func__);
		sec_ts_reinit(ts);
	}

	/* event queue full-> all finger release */
	if ((p_event_status->stype == TYPE_STATUS_EVENT_ERR) &&
		(p_event_status->status_id == SEC_TS_ERR_EVENT_QUEUE_FULL)) {
		TS_LOG_ERR("%s: IC Event Queue is full\n", __func__);
	}

	if ((p_event_status->stype == TYPE_STATUS_EVENT_ERR) &&
		(p_event_status->status_id == SEC_TS_ERR_EVENT_ESD)) {
		TS_LOG_ERR("%s: ESD detected. run reset\n", __func__);
	}

	if ((p_event_status->stype == TYPE_STATUS_EVENT_INFO) &&
		(p_event_status->status_id == SEC_TS_ACK_WET_MODE)) {
		ts->wet_mode = p_event_status->status_data_1;
		TS_LOG_INFO("%s: water wet mode %d\n",
			__func__, ts->wet_mode);
		if (ts->wet_mode)
			ts->wet_count++;

	}

	/*add some abnormal status debug log for S6Y761X,*/
	if(ts->ic_name == S6SY761X) {
		if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
			(p_event_status->status_id == SEC_TS_VENDOR_ERROR_WDT_RESET)) {
			TS_LOG_ERR("%s:WDT Reset.\n", __func__);
		}

		if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
			(p_event_status->status_id == SEC_TS_VENDOR_INFO_FREQ_HOPPING)) {
			TS_LOG_ERR("%s:Frequency hopping, the new frequency is %d kHz.\n", __func__,
				(p_event_status->status_data_2)<<8 | (p_event_status->status_data_1));
		}

		if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
			(p_event_status->status_id == SEC_TS_VENDOR_INFO_NOISE_LEVEL_CHANGED)) {
			TS_LOG_ERR("%s:Noise Level changed, the new noise level is %d .\n", __func__, (p_event_status->status_data_1));
		}

		if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
			(p_event_status->status_id == SEC_TS_VENDOR_INFO_ENTER_PALM)) {
			TS_LOG_ERR("%s:Plam reject enter.\n", __func__);
		}
	}

	if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
		(p_event_status->status_id == SEC_TS_VENDOR_ACK_SELFCHECK_DONE)) {
		if (p_event_status->status_data_1 == 0)
			TS_LOG_DEBUG("%s: self check pass\n", __func__);
		else {
			TS_LOG_ERR("%s: self check fail, %02X\n", __func__, p_event_status->status_data_1);
			ret = sec_ts_i2c_read(ts, SEC_TS_READ_SELFCHECK_RESULT, (u8 *)&rBuff, SEC_TS_SELFCHECK_BUFF_SIZE);
			if (ret < 0) {
				TS_LOG_ERR("%s: i2c read one event failed\n", __func__);
				return ret;
			}
			rearrange_byte((u8 *)&rBuff[1], SEC_TS_SELFCHECK_BUFF_SIZE - 1);
			p_selfcheck = (struct sec_ts_selfcheck *)rBuff;
			TS_LOG_DEBUG("%s: selfcheck result, %02X, %d, %d, %d, %d, %d, %d, %d, %d\n",
					__func__, p_selfcheck->result,
					p_selfcheck->mt_diff_tx, p_selfcheck->mt_diff_rx,
					p_selfcheck->mt_peak_max, p_selfcheck->mt_peak_min * -1,
					p_selfcheck->sf_txpeak_max, p_selfcheck->sf_txpeak_min * -1,
					p_selfcheck->sf_rxpeak_max, p_selfcheck->sf_rxpeak_min * -1);
		}
	}
	return ret;
}

static void sec_ts_process_coord(struct sec_ts_data *ts,
					struct ts_fingers *info, u8 *event_buff)
{
	u8 t_id = 0;
	struct sec_ts_event_coordinate *p_event_coord;

	p_event_coord = (struct sec_ts_event_coordinate *)event_buff;

	t_id = (p_event_coord->tid - 1);

	if (t_id < MAX_SUPPORT_TOUCH_COUNT + MAX_SUPPORT_HOVER_COUNT) {
		ts->coord[t_id].id = t_id;
		ts->coord[t_id].action = p_event_coord->tchsta;
		ts->coord[t_id].x = (p_event_coord->x_11_4 << 4) | (p_event_coord->x_3_0);
		ts->coord[t_id].y = (p_event_coord->y_11_4 << 4) | (p_event_coord->y_3_0);
		ts->coord[t_id].z = p_event_coord->z & 0x3F;
		ts->coord[t_id].ttype = p_event_coord->ttype_3_2 << 2 | p_event_coord->ttype_1_0 << 0;
		ts->coord[t_id].major = p_event_coord->major;
		ts->coord[t_id].minor = p_event_coord->minor;
		ts->coord[t_id].wx = p_event_coord->wx;
		ts->coord[t_id].wy = p_event_coord->wy;
		ts->coord[t_id].ewx = p_event_coord->ewx;
		ts->coord[t_id].ewy = p_event_coord->ewy;
		ts->coord[t_id].xer = p_event_coord->sgx;
		ts->coord[t_id].yer = p_event_coord->sgy;
		if(t_id < MAX_SUPPORT_TOUCH_COUNT) {
			info->fingers[t_id].status = TP_FINGER;
			if (ts->plat_data->in_recovery_mode == USE_IN_RECOVERY_MODE && ts->plat_data->use_ic_res == USE_IC_RESOLUTION && ts->plat_data->max_x != 0 && ts->plat_data->max_y != 0) {
				info->fingers[t_id].x = ts->coord[t_id].x * ts->plat_data->x_max_rc / ts->plat_data->max_x;
				info->fingers[t_id].y = ts->coord[t_id].y * ts->plat_data->y_max_rc / ts->plat_data->max_y;
			} else {
				info->fingers[t_id].x = ts->coord[t_id].x;
				info->fingers[t_id].y = ts->coord[t_id].y;
			}
			info->fingers[t_id].major = ts->coord[t_id].major;
			info->fingers[t_id].minor = ts->coord[t_id].minor;
			info->fingers[t_id].pressure = ts->coord[t_id].z;
			info->fingers[t_id].orientation = p_event_coord->orient;
			info->fingers[t_id].wx = ts->coord[t_id].wx;
			info->fingers[t_id].wy = ts->coord[t_id].wy;
			info->fingers[t_id].ewx = ts->coord[t_id].ewx;
			info->fingers[t_id].ewy = ts->coord[t_id].ewy;
			info->fingers[t_id].xer = ts->coord[t_id].xer;
			info->fingers[t_id].yer = ts->coord[t_id].yer;
		}

		if (!ts->coord[t_id].palm && (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_PALM))
			ts->coord[t_id].palm_count++;

		ts->coord[t_id].palm = (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_PALM);
		ts->coord[t_id].left_event = p_event_coord->left_event;

		if (ts->coord[t_id].z <= 0)
			ts->coord[t_id].z = 1;

		if ((ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_NORMAL)
			|| (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_PALM)
			|| (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_GLOVE)
			|| ((ts->ic_name == S6SY761X)&&(ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_WET) ) ) {

			if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_RELEASE) {
				do_gettimeofday(&ts->time_released[t_id]);
				if(t_id < MAX_SUPPORT_TOUCH_COUNT) {
					memset(&info->fingers[t_id], 0, sizeof(struct ts_finger));
				}
				if (ts->time_longest < (ts->time_released[t_id].tv_sec - ts->time_pressed[t_id].tv_sec))
					ts->time_longest = (ts->time_released[t_id].tv_sec - ts->time_pressed[t_id].tv_sec);

				if (ts->touch_count > 0) {
					ts->touch_count--;
					ts->coord[t_id].old_action = SEC_TS_COORDINATE_ACTION_RELEASE;
				}
				if (ts->touch_count == 0) {
					ts->check_multi = 0;
				}

			} else if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_PRESS) {
				do_gettimeofday(&ts->time_pressed[t_id]);
				//info->fingers[t_id].status = TS_FINGER_PRESS;
				if (ts->coord[t_id].old_action != SEC_TS_COORDINATE_ACTION_PRESS) {
					ts->touch_count++;
					ts->coord[t_id].old_action = SEC_TS_COORDINATE_ACTION_PRESS;
				}
				ts->all_finger_count++;

				ts->max_z_value = max((unsigned int)ts->coord[t_id].z, ts->max_z_value);
				ts->min_z_value = min((unsigned int)ts->coord[t_id].z, ts->min_z_value);
				ts->sum_z_value += (unsigned int)ts->coord[t_id].z;

				if ((ts->touch_count > 4) && (ts->check_multi == 0)) {
					ts->check_multi = 1;
					ts->multi_count++;
				}

			} else if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_MOVE) {
				//info->fingers[t_id].status = TS_FINGER_MOVE;
				if (ts->coord[t_id].old_action != SEC_TS_COORDINATE_ACTION_PRESS) {
					 ts->touch_count++;
					 ts->coord[t_id].old_action = SEC_TS_COORDINATE_ACTION_PRESS;
				}
				ts->coord[t_id].mcount++;
			} else {
				TS_LOG_ERR("%s: do not support coordinate action(%d)\n",
						__func__, ts->coord[t_id].action);
			}

			if (roi_switch_on)
				roi_data_staled = 1;

		} else {
			TS_LOG_ERR("%s: do not support coordinate type(%d)\n",
						__func__, ts->coord[t_id].ttype);
		}
	} else {
		TS_LOG_ERR("%s: tid(%d) is out of range\n", __func__, t_id);
	}

	if (t_id < MAX_SUPPORT_TOUCH_COUNT + MAX_SUPPORT_HOVER_COUNT) {
		if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_PRESS) {
			TS_LOG_DEBUG(
				"[P] tID:%d z:%d major:%d minor:%d tc:%d type:%X\n",
				t_id, ts->coord[t_id].z, ts->coord[t_id].major,
				ts->coord[t_id].minor, ts->touch_count, ts->coord[t_id].ttype);
		} else if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_RELEASE) {
			TS_LOG_DEBUG(
				"[R] tID:%d mc:%d tc:%d v:%02X%02X cal:%02X(%02X) id(%d,%d) p:%d P%02XT%04X F%02X%02X\n",
				t_id, ts->coord[t_id].mcount, ts->touch_count,
				ts->plat_data->img_version_of_ic[2],
				ts->plat_data->img_version_of_ic[3],
				ts->cal_status, ts->nv, ts->tspid_val,
				ts->tspicid_val, ts->coord[t_id].palm_count,
				ts->cal_count, ts->tune_fix_ver,
				ts->pressure_cal_base, ts->pressure_cal_delta);

			ts->coord[t_id].action = SEC_TS_COORDINATE_ACTION_NONE;
			ts->coord[t_id].mcount = 0;
			ts->coord[t_id].palm_count = 0;
		}
	}
}

static int sec_ts_read_info_event(struct sec_ts_data *ts, struct ts_fingers *info)
{
	int ret;
	u8 t_id;
	u8 event_id;
	u8 read_event_buff[SEC_TS_EVENT_BUFF_LONGSIZE] = { 0 };
	u8 *event_buff;
	struct sec_ts_gesture_status *p_gesture_status;
	int curr_pos;
	int remain_event = SEC_TS_EVENT_CONTINUE;

	ret = t_id = event_id = curr_pos = 0;
	/* repeat READ_ONE_EVENT until buffer is empty(No event) */


	do {
		ret = sec_ts_i2c_read(ts, SEC_TS_READ_ONE_EVENT, read_event_buff, SEC_TS_EVENT_BUFF_LONGSIZE);
		if (ret < 0) {
			TS_LOG_ERR("%s: i2c read one event failed\n", __func__);
			return ret;
		}
		event_buff = read_event_buff;
		if ((event_buff[0] == 0) && (event_buff[1] == 0)) /* empty event */
			goto exit_event;

		event_id = event_buff[0] & 0x3;

		TS_LOG_DEBUG("ALL: %02X %02X %02X %02X %02X %02X %02X %02X\n",
			event_buff[0], event_buff[1], event_buff[2], event_buff[3],
			event_buff[4], event_buff[5], event_buff[6], event_buff[7]);
		switch (event_id) {
		case SEC_TS_STATUS_EVENT:
			ret = sec_ts_process_status(ts, info, event_buff);
			if (ret < 0)
				remain_event = SEC_TS_EVENT_STOP;
			return -EIO;

		case SEC_TS_COORDINATE_EVENT:
			if (ts->input_closed) {
				TS_LOG_ERR("%s: device is closed\n", __func__);
				remain_event = SEC_TS_EVENT_STOP;
				break;
			}
			sec_ts_process_coord(ts, info, event_buff);
			remain_event = SEC_TS_EVENT_CONTINUE;
			break;

		case SEC_TS_GESTURE_EVENT:
			p_gesture_status = (struct sec_ts_gesture_status *)event_buff;
			if ((p_gesture_status->eid == 0x02) && (p_gesture_status->stype == 0x00)) {
				TS_LOG_ERR("%s: gesture event %x %x %x %x %x %x\n", __func__,
					event_buff[0], event_buff[1], event_buff[2],
					event_buff[3], event_buff[4], event_buff[5]);
				remain_event = SEC_TS_EVENT_STOP;
			}
			break;

		default:
			TS_LOG_ERR("%s: unknown event %x %x %x %x %x %x\n", __func__,
					event_buff[0], event_buff[1], event_buff[2],
					event_buff[3], event_buff[4], event_buff[5]);
			remain_event = SEC_TS_EVENT_STOP;
			break;
		}

	} while (remain_event);
exit_event:
	info->cur_finger_number = ts->touch_count;

	return 0;
}

static int sec_ts_regulator_get(struct sec_ts_data *data)
{
	struct sec_ts_data *ts = data;
	int avdd_value = ts->chip_data->regulator_ctr.vci_value;
	int dvdd_value = ts->chip_data->regulator_ctr.vddio_value;
	int ret;

	if (ts->chip_data->vddio_regulator_type) {
		TS_LOG_INFO("get regulator_dvdd\n");
		ts->regulator_dvdd = regulator_get(&ts->sec_ts_pdev->dev,
						SEC_TS_REGULATOR_DVDD);
		if (IS_ERR(ts->regulator_dvdd)) {
			TS_LOG_ERR("regulator tp vdd not used\n");
			return -EINVAL;
		}

		ret = regulator_set_voltage(ts->regulator_dvdd, dvdd_value,
						dvdd_value);
		TS_LOG_INFO("%s: vddio set volt:%d\n", __func__, dvdd_value);
		if (ret < 0) {
			TS_LOG_ERR("failed to set DVDD error: %d\n", ret);
			return -EINVAL;
		}
	}

	if (ts->chip_data->vci_regulator_type) {
		TS_LOG_INFO("get regulator_avdd\n");
		ts->regulator_avdd = regulator_get(&ts->sec_ts_pdev->dev,
					SEC_TS_REGULATOR_AVDD);
		if (IS_ERR(ts->regulator_avdd)) {
			TS_LOG_ERR("regulator tp avdd not used\n");
			return -EINVAL;
		}
		ret = regulator_set_voltage(ts->regulator_avdd, avdd_value, avdd_value);
		TS_LOG_INFO("%s: avdd set volt:%d\n", __func__, avdd_value);
		if (ret < 0) {
			TS_LOG_ERR("failed to set AVDD error: %d\n",ret);
			return -EINVAL;
		}
	}

	TS_LOG_INFO("regulator get done\n");
	return 0;
}

static void sec_ts_regulator_put(struct sec_ts_data *ts)
{
	if (ts->chip_data->vddio_regulator_type)
		regulator_put(ts->regulator_dvdd);

	if (ts->chip_data->vci_regulator_type)
		regulator_put(ts->regulator_avdd);
}

static int sec_pinctrl_select_normal(void)
{
	int retval = -1;
	struct sec_ts_data *ts = tsp_info;

	if (ts->pctrl == NULL || ts->pins_default == NULL) {
		TS_LOG_ERR("%s: pctrl or pins_default is NULL.\n", __func__);
		return retval;
	}
	retval = pinctrl_select_state(ts->pctrl, ts->pins_default);
	if (retval < 0) {
		TS_LOG_ERR("%s: set pinctrl normal error.\n", __func__);
	}
	return retval;
}
static int sec_pinctrl_select_lowpower(void)
{
	int retval = -1;
	struct sec_ts_data *ts = tsp_info;

	if (ts->pctrl == NULL || ts->pins_idle== NULL) {
		TS_LOG_ERR("%s: pctrl or pins_idle is NULL.\n", __func__);
		return retval;
	}
	retval = pinctrl_select_state(ts->pctrl, ts->pins_idle);
	if (retval < 0) {
		TS_LOG_ERR("%s: set pinctrl pins_idle error.\n", __func__);
	}
	return retval;
}

static int sec_pinctrl_get_init(void)
{
	int ret = 0;
	struct sec_ts_data *ts = tsp_info;

	TS_LOG_INFO("%s enter\n", __func__);

	ts->pctrl = devm_pinctrl_get(&ts->sec_ts_pdev->dev);
	if (IS_ERR(ts->pctrl)) {
		TS_LOG_ERR("%s failed to devm pinctrl get\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	ts->pins_default = pinctrl_lookup_state(ts->pctrl, "default");
	if (IS_ERR(ts->pins_default)) {
		TS_LOG_ERR("%s fail to pinctrl lookup state default\n", __func__);
		ret = -EINVAL;
		goto err_pinctrl_put;
	}

	ts->pins_idle = pinctrl_lookup_state(ts->pctrl, "idle");
	if (IS_ERR(ts->pins_idle)) {
		TS_LOG_ERR("%s failed to pinctrl lookup state idle\n", __func__);
		ret = -EINVAL;
		goto err_pinctrl_put;
	}
	return 0;

err_pinctrl_put:
	devm_pinctrl_put(ts->pctrl);
	return ret;
}

static int sec_ts_power(struct sec_ts_data *data, bool on)
{
	struct sec_ts_data *ts = data;
	static bool enabled;
	int ret = 0;

	TS_LOG_DEBUG("%s: enable=%d, on=%d\n", __func__, enabled, on);
	if (enabled == on) {
		TS_LOG_ERR("%s: already is %d\n", __func__, on);
		return ret;
	}

	if (on) {

		if (ts->chip_data->vddio_regulator_type) {
			TS_LOG_DEBUG("%s:enable dvdd regulator\n", __func__);
			ret = regulator_enable(ts->regulator_dvdd);
			if (ret) {
				TS_LOG_ERR("%s: Failed to enable dvdd: %d\n", __func__, ret);
				goto out;
			}
		} else if (ts->chip_data->vddio_gpio_type) {
			TS_LOG_DEBUG("%s vddio switch gpio on\n", __func__);
			gpio_direction_output(ts->chip_data->vddio_gpio_ctrl, 1);
		}
		sec_ts_delay(1);

		if (ts->chip_data->vci_regulator_type) {
			TS_LOG_DEBUG("%s:enable avdd regulator\n", __func__);
			ret = regulator_enable(ts->regulator_avdd);
			if (ret) {
				TS_LOG_ERR("%s: Failed to enable avdd: %d\n", __func__, ret);
				goto out;
			}
		} else if (ts->chip_data->vci_gpio_type) {
			TS_LOG_DEBUG("%s vci switch gpio on\n", __func__);
			gpio_direction_output(ts->chip_data->vci_gpio_ctrl, 1);
		}

		if(ts->is_need_set_pinctrl){
			sec_pinctrl_select_normal();
		}

		sec_ts_delay(1);
		gpio_direction_output(ts->chip_data->ts_platform_data->reset_gpio, 1);
		TS_LOG_INFO("%s: reset gpio on\n", __func__);
	} else {
		gpio_direction_output(ts->chip_data->ts_platform_data->reset_gpio, 0);
		TS_LOG_INFO("%s: reset gpio off\n", __func__);
		if(ts->ic_name == S6SY761X) {
			if (ts->chip_data->vci_regulator_type) {
				TS_LOG_DEBUG("%s: regulator disable avdd\n", __func__);
				if (regulator_disable(ts->regulator_avdd)) {
					TS_LOG_ERR("%s: disable avdd regulator failed\n",__func__);
				}
			} else if (ts->chip_data->vci_gpio_type) {
				TS_LOG_DEBUG("%s vci switch gpio off\n", __func__);
				gpio_direction_output(tsp_info->chip_data->vci_gpio_ctrl, 0);
			}
			/*the new specification requires avdd to be powered off for more than 4ms than iovcc,
			 so delay 5ms here.						*/
			sec_ts_delay(5); //ic need

			if (ts->chip_data->vddio_regulator_type) {
				TS_LOG_DEBUG("%s: regulator disable dvdd\n",__func__);
				if (regulator_disable(ts->regulator_dvdd)) {
					TS_LOG_ERR("%s: disable dvdd regulator failed\n",__func__);
				}
			} else if (ts->chip_data->vddio_gpio_type) {
				TS_LOG_DEBUG("%s vddio switch gpio off\n", __func__);
				gpio_direction_output(tsp_info->chip_data->vddio_gpio_ctrl, 0);
			}
		} else {
			if (ts->chip_data->vddio_regulator_type) {
				TS_LOG_DEBUG("%s: regulator disable dvdd\n",__func__);
				if (regulator_disable(ts->regulator_dvdd)) {
					TS_LOG_ERR("%s: disable dvdd regulator failed\n",__func__);
				}
			} else if (ts->chip_data->vddio_gpio_type) {
				TS_LOG_DEBUG("%s vddio switch gpio off\n", __func__);
				gpio_direction_output(tsp_info->chip_data->vddio_gpio_ctrl, 0);
			}
			sec_ts_delay(1);
			if (ts->chip_data->vci_regulator_type) {
				TS_LOG_DEBUG("%s: regulator disable avdd\n", __func__);
				if (regulator_disable(ts->regulator_avdd)) {
					TS_LOG_ERR("%s: disable avdd regulator failed\n",__func__);
				}
			} else if (ts->chip_data->vci_gpio_type) {
				TS_LOG_DEBUG("%s vci switch gpio off\n", __func__);
				gpio_direction_output(tsp_info->chip_data->vci_gpio_ctrl, 0);
			}
		}
		if(ts->is_need_set_pinctrl){
			sec_pinctrl_select_lowpower();
		}
	}

	enabled = on;

out:
	TS_LOG_INFO("%s: %s\n", __func__, on ? "on" : "off");


	return ret;
}

static void sec_ts_set_firmware_name(struct sec_ts_data *ts)
{
	const char *fw_name = ts->plat_data->firmware_name;

	strncpy(fw_name, SEC_TS_STR_FWPATH, sizeof(SEC_TS_STR_FWPATH));
	if (ts->module_name == NULL)
		ts->module_name = SEC_TS_MODULE_VENDOR;
	strncat(fw_name, ts->module_name, strlen(ts->module_name));
	strncat(fw_name, SEC_TS_STR_UNDERSCORE, sizeof(SEC_TS_STR_UNDERSCORE));
	strncat(fw_name, SEC_TS_VENDOR_NAME, sizeof(SEC_TS_VENDOR_NAME));
	strncat(fw_name, ts->project_id, sizeof(ts->project_id));
	strncat(fw_name, SEC_TS_STR_FWTYPE, sizeof(SEC_TS_STR_FWTYPE));
}

static void sec_ts_parse_ic_name(struct device_node *device,
					struct ts_kit_device_data *chip_data)
{
	int ret = NO_ERR;
	int read_val= 0;
	if(!device || !chip_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	ret = of_property_read_u32(device, "ic_name", &read_val);
	if (!ret) {
		tsp_info->ic_name =  read_val;
	}

	TS_LOG_INFO("tsp_info->ic_name = %d\n", tsp_info->ic_name);
}

static void sec_ts_default_trx_num(struct device_node *device,
					struct ts_kit_device_data *chip_data)
{
	int ret = NO_ERR;
	int read_val= 0;
	if(!device || !chip_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	ret = of_property_read_u32(device, "tx_default_num", &read_val);
	if (!ret) {
		tsp_info->tx_default_num = read_val;
	}

	ret = of_property_read_u32(device, "rx_default_num", &read_val);
	if (!ret) {
		tsp_info->rx_default_num = read_val;
	}

	TS_LOG_INFO("tsp_info->tx_default_num = %d, tsp_info->rx_default_num = %d\n",
		tsp_info->tx_default_num, tsp_info->tx_default_num);
}

/*parse is need calibrate after update fw flag, default need not calibrate*/
static void sec_ts_is_need_calibrate_after_update(struct device_node *device,
					struct ts_kit_device_data *chip_data)
{
	int ret = NO_ERR;
	int read_val= 0;
	if(!device || !chip_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	ret = of_property_read_u32(device, "is_need_calibrate_after_update_fw", &read_val);
	if (!ret) {
		tsp_info->is_need_calibrate_after_update_fw = read_val;
	} else {
		tsp_info->is_need_calibrate_after_update_fw = 0;
	}

	TS_LOG_INFO("tsp_info->is_need_calibrate_after_update_fw = %d\n",
		tsp_info->is_need_calibrate_after_update_fw);
}

static void sec_ts_parse_holster_supported(struct device_node *device,
					struct ts_kit_device_data *chip_data)
{
	int ret = NO_ERR;
	int read_val= 0;
	if(!device || !chip_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	ret = of_property_read_u32(device, "holster_supported", &read_val);
	if (!ret) {
		tsp_info->chip_data->ts_platform_data->feature_info.holster_info.holster_supported = read_val;
	} else {
		tsp_info->chip_data->ts_platform_data->feature_info.holster_info.holster_supported = 0;
	}

	TS_LOG_INFO("tsp_info->holster_support = %d\n",
		tsp_info->chip_data->ts_platform_data->feature_info.holster_info.holster_supported);
}

static void sec_ts_parse_is_need_set_pinctrl(struct device_node *device,
					struct ts_kit_device_data *chip_data)
{
	int ret = NO_ERR;
	int read_val = 0;
	if(!device || !chip_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	ret = of_property_read_u32(device, "is_need_set_pinctrl", &read_val);
	if (!ret) {
		tsp_info->is_need_set_pinctrl =  read_val;
	}

	TS_LOG_INFO("tsp_info->is_need_set_pinctrl = %d\n", tsp_info->is_need_set_pinctrl);
}

static void sec_ts_parse_default_projectid(struct device_node *device,
					struct ts_kit_device_data *chip_data)
{
	int ret = NO_ERR;
	int read_val = 0;
	if(!device || !chip_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}

	ret = of_property_read_string(device, "default_projectid", &tsp_info->default_projectid);
	if(ret) {
		TS_LOG_ERR("%s, can't parse default_projectid from dts, use default\n", __func__);
		tsp_info->default_projectid = SEC_TS_HW_PROJECTID;
	}
	TS_LOG_INFO("%s,default_projectid:%s\n", __func__, tsp_info->default_projectid);
}

static void sec_ts_parse_is_need_set_reseved_bit(struct device_node *device,
					struct ts_kit_device_data *chip_data)
{
	int ret = NO_ERR;
	int read_val = 0;
	if(!device || !chip_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}

	ret = of_property_read_u32(device, "is_need_set_reseved_bit", &read_val);
	if (!ret) {
		tsp_info->is_need_set_reseved_bit =  read_val;
	}
	TS_LOG_INFO("tsp_info->is_need_set_reseved_bit = %d\n", tsp_info->is_need_set_reseved_bit);
}

static void sec_ts_parse_recovery_res(struct device_node *device,
					struct ts_kit_device_data *chip_data)
{
	struct sec_ts_plat_data *pdata = tsp_info->plat_data;
	int ret = 0;
	struct device_node *np = device;

	if(!device || !chip_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}

	/* convert ic resolution x[0,4095] y[0,4095] to real resolution, fix inaccurate touch in recovery mode,
	 * there is no coordinate conversion in recovery mode.
	 */
	if (get_into_recovery_flag_adapter()) {
		pdata->in_recovery_mode = USE_IN_RECOVERY_MODE;
		TS_LOG_INFO("Now driver in recovery mode\n");
	} else {
		pdata->in_recovery_mode = USE_NOT_IN_RECOVERY_MODE;
	}

	ret = of_property_read_u32(np, "use_ic_res", &pdata->use_ic_res);
	if (ret) {
		TS_LOG_INFO("%s:get device use_ic_res fail, use default, ret=%d\n",
			__func__, ret);
		pdata->use_ic_res = USE_LCD_RESOLUTION;
	}

	ret = of_property_read_u32(np, "x_max_rc", &pdata->x_max_rc);
	if (ret) {
		TS_LOG_INFO("%s:get device x_max_rc fail,use default, ret=%d\n",
			__func__, ret);
		pdata->x_max_rc = 0;
	}

	ret = of_property_read_u32(np, "y_max_rc", &pdata->y_max_rc);
	if (ret) {
		TS_LOG_INFO("%s:get device y_max_rc fail,use default, ret=%d\n",
			__func__, ret);
		pdata->y_max_rc = 0;
	}

	TS_LOG_INFO("%s:%s=%d, %s=%d, %s=%d, %s=%d\n", __func__,
		"use_ic_res", pdata->use_ic_res,
		"x_max_rc", pdata->x_max_rc,
		"y_max_rc", pdata->y_max_rc,
		"in_recovery_mode", pdata->in_recovery_mode);
}



static int sec_ts_parse_dt(struct device_node *device,
					struct ts_kit_device_data *chip_data)
{
	struct sec_ts_plat_data *pdata = tsp_info->plat_data;
	struct device_node *np = device;
	int ret = 0;
	int index;
	int array_len;
	int read_val;
	const char *raw_data_dts = NULL;
	const char *producer = NULL;
	char id_buf[SEC_TS_MAX_FW_PATH] = {0};
	int value;

	ret = of_property_read_u32(np, "i2c_address", &chip_data->slave_addr);
	if (ret) {
		TS_LOG_ERR("get i2c address failed\n");
		return -ENODATA;
	}

	ret = of_property_read_u32(np, "ic_type", &chip_data->ic_type);
	if (ret) {
		TS_LOG_ERR("get i2c address failed\n");
		return -ENODATA;
	}

	ret = of_property_read_u32(np, "irq_config", &chip_data->irq_config);
	if (ret) {
		TS_LOG_ERR("get irq config failed\n");
		return -ENODATA;
	}

	ret = of_property_read_u32(np, "algo_id", &chip_data->algo_id);
	if (ret) {
		TS_LOG_ERR("%s:get algo id failed\n", __func__);
		return -ENODATA;
	}

	ret = of_property_read_u32(np, "x_max", &chip_data->x_max);
	if (ret) {
		TS_LOG_ERR("%s:get device x_max fail, ret=%d\n",
			__func__, ret);
		return -ENODATA;
	}

	ret = of_property_read_u32(np, "y_max", &chip_data->y_max);
	if (ret) {
		TS_LOG_ERR("%s:get device y_max fail, ret=%d\n",
			__func__, ret);
		return -ENODATA;
	}

	ret = of_property_read_u32(np, "x_max_mt", &chip_data->x_max_mt);
	if (ret) {
		TS_LOG_ERR("%s:get device x_max fail, ret=%d\n",
			__func__, ret);
		return -ENODATA;
	}

	ret = of_property_read_u32(np, "y_max_mt", &chip_data->y_max_mt);
	if (ret) {
		TS_LOG_ERR("%s:get device y_max fail, ret=%d\n",
			__func__, ret);
		return -ENODATA;
	}

	TS_LOG_INFO("%s:%s=%d, %s=%d, %s=%d, %s=%d, %s=%d\n", __func__,
		"algo_id", chip_data->algo_id,
		"x_max", chip_data->x_max,
		"y_max", chip_data->y_max,
		"x_mt", chip_data->x_max_mt,
		"y_mt", chip_data->y_max_mt);

	array_len = of_property_count_strings(np, "raw_data_limit");
	if (array_len <= 0 || array_len > RAWDATA_NUM) {
		TS_LOG_ERR
			("raw_data_limit length invaild or dts number is larger than:%d\n",
				array_len);
	}

	for (index = 0; index < array_len && index < RAWDATA_NUM; index++) {
		ret =
			of_property_read_string_index(np,
						"raw_data_limit",
						index, &raw_data_dts);
		if (ret) {
			TS_LOG_ERR
				("read index = %d,raw_data_limit = %s,retval = %d error,\n",
					index, raw_data_dts, ret);
		}

		if (!ret) {
			chip_data->raw_limit_buf[index] =
				simple_strtol(raw_data_dts, NULL, 10);
			TS_LOG_INFO("rawdatabuf[%d] = %d\n", index,
					chip_data->raw_limit_buf[index]);
		}
	}

	array_len = of_property_count_strings(np, "cal_data_limit");
	if (array_len <= 0 || array_len > SEC_TS_CAL_LIMITDATA_MAX) {
		TS_LOG_ERR
			("cal_data_limit length invaild or dts number is larger than:%d\n",
				array_len);
	}

	for (index = 0; index < array_len && index < SEC_TS_CAL_LIMITDATA_MAX; index++) {
		ret = of_property_read_string_index(np, "cal_data_limit",
							index, &raw_data_dts);
		if (ret) {
			TS_LOG_ERR
				("read index = %d,cal_data_limit = %s,retval = %d error,\n",
					index, raw_data_dts, ret);
		}

		if (!ret) {
			tsp_info->cal_limit[index] =
				simple_strtol(raw_data_dts, NULL, 10);
			TS_LOG_INFO("cal_limit[%d] = %d\n", index,
					tsp_info->cal_limit[index]);
		}
	}

	array_len = of_property_count_strings(np, "nolh_data_limit");
	if (array_len <= 0 || array_len > SEC_TS_NOLH_LIMITDATA_MAX) {
		TS_LOG_ERR
			("cal_data_limit length invaild or dts number is larger than:%d\n",
				array_len);
	}

	for (index = 0; index < array_len && index < SEC_TS_NOLH_LIMITDATA_MAX; index++) {
		ret = of_property_read_string_index(np, "nolh_data_limit",
							index, &raw_data_dts);
		if (ret) {
			TS_LOG_ERR
				("read index = %d,nolh_data_limit = %s,retval = %d error,\n",
					index, raw_data_dts, ret);
		}

		if (!ret) {
			tsp_info->nolh_limit[index] =
				simple_strtol(raw_data_dts, NULL, 10);
			TS_LOG_INFO("nolh_limit[%d] = %d\n", index,
					tsp_info->nolh_limit[index]);
		}
	}

	ret = of_property_read_u32(device, "vci_gpio_type",
					&chip_data->vci_gpio_type);
	if (ret) {
		TS_LOG_ERR("get device vci_gpio_type failed\n");
	}
	ret = of_property_read_u32(np, "vci_regulator_type",
					&chip_data->vci_regulator_type);
	if (ret) {
		TS_LOG_ERR("get device vci_regulator_type failed\n");
	}
	TS_LOG_INFO("%s:vci_regulator_type %d\n", __func__,
			chip_data->vci_regulator_type);

	ret = of_property_read_u32(device, "vddio_gpio_type",
					&chip_data->vddio_gpio_type);
	if (ret) {
		TS_LOG_ERR("get device vddio_gpio_type failed\n");
	}
	ret =  of_property_read_u32(np, "vddio_regulator_type",
					&chip_data->vddio_regulator_type);
	if (ret) {
		TS_LOG_ERR
			("get device vddio_regulator_type failed\n");
	}
	TS_LOG_INFO("%s:vddio_regulator_type %d\n", __func__,
				chip_data->vddio_regulator_type);

	pdata->power = sec_ts_power;

	ret = of_property_read_u32(np, "avdd_value",
				 &chip_data->regulator_ctr.vci_value);
	if (ret) {
		TS_LOG_INFO("Not define Vci value in Dts, use fault value\n");
		chip_data->regulator_ctr.vci_value = 3300000;
	}

	ret = of_property_read_u32(np, "dvdd_value",
					&chip_data->regulator_ctr.vddio_value);
	if (ret) {
		TS_LOG_INFO("Not define Vci value in Dts, use fault value\n");
		chip_data->regulator_ctr.vddio_value = 1800000;
	}

	if (chip_data->vci_gpio_type) {
		chip_data->vci_gpio_ctrl =
			of_get_named_gpio(device, "vci_ctrl_gpio", 0);
		if (!gpio_is_valid(chip_data->vci_gpio_ctrl)) {
			TS_LOG_ERR
				("vci power gpio is not valid\n");
		}
	}
	if (chip_data->vddio_gpio_type) {
		chip_data->vddio_gpio_ctrl =
			of_get_named_gpio(device, "vddio_ctrl_gpio", 0);
		if (!gpio_is_valid(chip_data->vddio_gpio_ctrl)) {
			TS_LOG_ERR
				("vddio power gpio is not valid\n");
		}
	}

	if (of_property_read_u32(np, "i2c-burstmax", &pdata->i2c_burstmax)) {
		TS_LOG_ERR("%s: Failed to get i2c_burstmax property\n", __func__);
		pdata->i2c_burstmax = 256;
	}

	ret = of_property_read_u32(np, "supported_func_indicater", &read_val);
	if (!ret) {
		TS_LOG_INFO("get supported_func_indicater = %d\n", read_val);
		chip_data->supported_func_indicater = read_val;
	} else {
		chip_data->supported_func_indicater = 0;
	}

	if (!of_property_read_u32(np, "use_lcdkit_power_notify", &read_val))
		chip_data->use_lcdkit_power_notify = read_val;
	else
		chip_data->use_lcdkit_power_notify = 0;

	TS_LOG_INFO("%s:use_lcdkit_power_notify = %d\n", __func__,
					chip_data->use_lcdkit_power_notify);

	ret = of_property_read_string(np, "tp_test_type",
				&tsp_info->cap_test_type);
	if (ret) {
		TS_LOG_INFO("get device tp_test_type not exit,use default value\n");
		tsp_info->cap_test_type = "Normalize_type:judge_different_reslut";
	}

	value = of_get_named_gpio(np, "charger_notify_gpio", 0);
	if (!gpio_is_valid(value)) {
		TS_LOG_ERR("%s: get charger_notify_gpio failed\n", __func__);
	} else {
		tsp_info->charger_notify_gpio = value;
	}
	TS_LOG_INFO("%s: charger_notify_gpio is %d\n", __func__, value);

	ret = of_property_read_u32(np, "charger_supported", &read_val);
	if (!ret) {
		TS_LOG_INFO("charger_supported = %d\n", read_val);
		tsp_info->chip_data->ts_platform_data->feature_info.charger_info.charger_supported
				= (u8) read_val;
	} else {
		TS_LOG_ERR("can not get charger_supported value\n");
		tsp_info->chip_data->ts_platform_data->feature_info.charger_info.charger_supported = 0;
	}
	sec_ts_parse_ic_name(device, chip_data);
	sec_ts_parse_holster_supported(device, chip_data);
	sec_ts_parse_is_need_set_pinctrl(device, chip_data);

	if(tsp_info->is_need_set_pinctrl){
		sec_pinctrl_get_init();
	}
	sec_ts_parse_default_projectid(device, chip_data);
	sec_ts_parse_is_need_set_reseved_bit(device, chip_data);
	sec_ts_default_trx_num(device, chip_data);
	sec_ts_is_need_calibrate_after_update(device, chip_data);
	sec_ts_parse_recovery_res(device, chip_data);

	tsp_info->module_name = NULL;

	memcpy(id_buf, SEC_TS_VENDOR_NAME, strlen(SEC_TS_VENDOR_NAME));
	strncat(id_buf, tsp_info->default_projectid, strlen(SEC_TS_HW_PROJECTID));

	device = of_find_compatible_node(NULL, NULL, id_buf);
	if (!device) {
		TS_LOG_ERR("No chip specific dts: %s, need to parse\n",
				id_buf);
		return -ENODATA;
	}
	ret = of_property_read_u32(device, "roi_supported", &read_val);
	if (!ret) {
		TS_LOG_INFO("get chip specific roi_supported = %d\n", read_val);
		tsp_info->chip_data->ts_platform_data->feature_info.roi_info.roi_supported = (u8) read_val;
	} else {
		TS_LOG_ERR("can not get roi_supported value\n");
		tsp_info->chip_data->ts_platform_data->feature_info.roi_info.roi_supported = 0;
	}

	ret = of_property_read_u32(device, "roi_control_addr", &read_val);
	if (!ret) {
		TS_LOG_INFO("get chip specific roi_control_addr = 0x%04x\n",
				read_val);
		tsp_info->chip_data->ts_platform_data->
					feature_info.roi_info.roi_control_addr =
				(u16) read_val;
	}

	ret = of_property_read_u32(device, "roi_control_bit", &read_val);
	if (!ret) {
		TS_LOG_INFO("get chip specific roi_control_bit = %d\n",
				read_val);
		tsp_info->chip_data->ts_platform_data->
					feature_info.roi_info.roi_control_bit = (u8) read_val;
	}

	ret = of_property_read_u32(device, "roi_data_addr", &read_val);
	if (!ret) {
		TS_LOG_INFO("get chip specific roi_data_addr = 0x%04x\n",
					read_val);
		tsp_info->chip_data->ts_platform_data->
					feature_info.roi_info.roi_data_addr = (u16) read_val;
	}

	ret = of_property_read_string(device, "producer", &producer);
	if (NULL != producer) {
		TS_LOG_INFO("producer = %s\n", producer);
		tsp_info->module_name = producer;
	} else {
		TS_LOG_ERR("fail parse TP producer \n");
	}

	pdata->max_x = chip_data->x_max - 1;
	pdata->max_y = chip_data->y_max - 1;

	return ret;
}


static int sec_ts_gpio_request(struct sec_ts_data *ts)
{
	int retval = NO_ERR;
	TS_LOG_INFO("sec_ts_gpio_request\n");

	retval = gpio_request(ts->charger_notify_gpio, "ts_TA_gpio");
	if (retval)
		TS_LOG_ERR("fail to request charger_notify_gpio:%d\n",
			ts->charger_notify_gpio);

	if ((1 == ts->chip_data->vci_gpio_type)
		&& (1 == ts->chip_data->vddio_gpio_type)) {
		if (ts->chip_data->vci_gpio_ctrl ==
			ts->chip_data->vddio_gpio_ctrl) {
			retval =
				gpio_request(ts->chip_data->
					vci_gpio_ctrl, "ts_vci_gpio");
			if (retval) {
				TS_LOG_ERR
					("SFT:Ok;  ASIC: Real ERR----unable to request vci_gpio_ctrl firset:%d\n",
					ts->chip_data->
					vci_gpio_ctrl);
				goto ts_vci_out;
			}
		} else {
			retval =
				gpio_request(ts->chip_data->
					vci_gpio_ctrl, "ts_vci_gpio");
			if (retval) {
				TS_LOG_ERR
					("SFT:Ok;  ASIC: Real ERR----unable to request vci_gpio_ctrl2:%d\n",
					ts->chip_data->
					vci_gpio_ctrl);
				goto ts_vci_out;
			}
			retval =
				gpio_request(ts->chip_data->
						vddio_gpio_ctrl, "ts_vddio_gpio");
			if (retval) {
				TS_LOG_ERR
					("SFT:Ok;  ASIC: Real ERR----unable to request vddio_gpio_ctrl:%d\n",
						ts->chip_data->vddio_gpio_ctrl);
				goto ts_vddio_out;
			}
		}
	} else {
		if (1 == ts->chip_data->vci_gpio_type) {
			retval =
				gpio_request(ts->chip_data->
					vci_gpio_ctrl, "ts_vci_gpio");
			if (retval) {
				TS_LOG_ERR
					("SFT:Ok;  ASIC: Real ERR----unable to request vci_gpio_ctrl2:%d\n",
					ts->chip_data->
					vci_gpio_ctrl);
				goto ts_vci_out;
			}
		}
		if (1 == ts->chip_data->vddio_gpio_type) {
			retval =
				gpio_request(ts->chip_data->
					vddio_gpio_ctrl, "ts_vddio_gpio");
			if (retval) {
				TS_LOG_ERR
					("SFT:Ok;  ASIC: Real ERR----unable to request vddio_gpio_ctrl:%d\n",
					ts->chip_data->
					vddio_gpio_ctrl);
				goto ts_vddio_out;
			}
		}
	}

	goto ts_reset_out;

ts_vddio_out:
	gpio_free(ts->chip_data->vci_gpio_ctrl);
ts_vci_out:
ts_reset_out:
	return retval;
}

static void sec_ts_gpio_free(struct sec_ts_data *ts)
{
	TS_LOG_INFO("sec_ts_gpio_free\n");

	/*0 is power supplied by gpio, 1 is power supplied by ldo */
	if (ts->chip_data->vci_gpio_type) {
		if (ts->chip_data->vci_gpio_ctrl)
			gpio_free(ts->chip_data->vci_gpio_ctrl);
	}
	if (ts->chip_data->vddio_gpio_type) {
		if (ts->chip_data->vddio_gpio_ctrl)
			gpio_free(ts->chip_data->vddio_gpio_ctrl);
	}
}

int sec_ts_read_information(struct sec_ts_data *ts)
{
	unsigned char data[13] = { 0 };
	int ret;

	memset(data, 0x0, 3);
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_ID, data, 3);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to read device id(%d)\n", __func__, ret);
		goto err_i2c;
	}

	TS_LOG_INFO("%s: %X, %X, %X\n",	__func__, data[0], data[1], data[2]);
	memset(data, 0x0, 11);
	ret = sec_ts_i2c_read(ts,  SEC_TS_READ_PANEL_INFO, data, 11);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to read sub id(%d)\n", __func__, ret);
		goto err_i2c;
	}

	TS_LOG_INFO("%s: nTX:%X, nRX:%X, rY:%d, rX:%d\n",
			__func__, data[8], data[9],
			(data[2] << 8) | data[3], (data[0] << 8) | data[1]);

	/* Set X,Y Resolution from IC information. */
	if (((data[0] << 8) | data[1]) > 0)
		ts->plat_data->max_x = ((data[0] << 8) | data[1]) - 1;

	if (((data[2] << 8) | data[3]) > 0)
		ts->plat_data->max_y = ((data[2] << 8) | data[3]) - 1;

	if(S6SY761X == tsp_info->ic_name) {
		if (ts->chip_data->x_max - 1 != ts->plat_data->max_x) {
			TS_LOG_INFO("%s: read x information isn't match : %d\n",
					__func__, ts->chip_data->x_max);
			ts->plat_data->max_x = ts->chip_data->x_max - 1;
		}

		if (ts->chip_data->y_max - 1 != ts->plat_data->max_y) {
			TS_LOG_INFO("%s: read y information isn't match : %d\n",
					__func__, ts->chip_data->y_max);
			ts->plat_data->max_y = ts->chip_data->y_max - 1;
		}
	} else {
		if (ts->chip_data->x_max != ts->plat_data->max_x) {
			TS_LOG_INFO("%s: read x information isn't match : %d\n",
					__func__, ts->chip_data->x_max);
			ts->plat_data->max_x = ts->chip_data->x_max;
		}

		if (ts->chip_data->y_max != ts->plat_data->max_y) {
			TS_LOG_INFO("%s: read y information isn't match : %d\n",
					__func__, ts->chip_data->y_max);
			ts->plat_data->max_y = ts->chip_data->y_max;
		}
	}

	ts->tx_count = data[8];
	ts->rx_count = data[9];
	if(ts->ic_name == S6SY761X) {
		if(ts->tx_count != ts->tx_default_num
			|| ts->rx_count != ts->rx_default_num) {
			TS_LOG_ERR("%s, tx or tx invalid, [%d, %d], use default trx num\n",
				__func__, ts->tx_count, ts->rx_count);
			ts->rx_count = ts->rx_default_num;
			ts->tx_count = ts->tx_default_num;
		}
	}
	ts->chip_data->tx_num = ts->tx_count;
	ts->chip_data->rx_num = ts->rx_count;

	TS_LOG_INFO("%s: tx_count=%d, rx_count=%d\n", __func__, ts->tx_count,
				ts->rx_count);

	data[0] = 0;
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_BOOT_STATUS, data, 1);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to read sub id(%d)\n", __func__, ret);
		goto err_i2c;
	}

	TS_LOG_INFO("%s: STATUS : %X\n", __func__, data[0]);

	memset(data, 0x0, 4);
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_TS_STATUS, data, 4);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to read sub id(%d)\n", __func__, ret);
		goto err_i2c;
	}

	TS_LOG_INFO("%s: TOUCH STATUS : %02X, %02X, %02X, %02X\n",
			__func__, data[0], data[1], data[2], data[3]);

	memset(data, 0x0, 2);
	ret = sec_ts_i2c_read(ts, SEC_TS_CMD_DOZE_PARAM, data, 2);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to read idle delay (%d)\n", __func__, ret);
		goto err_i2c;
	} else
		ts->idle_delay = ((data[0] << 8)|data[1]);
	TS_LOG_INFO("%s: Idle delay : %d\n", __func__, ts->idle_delay);

	ret = sec_ts_i2c_read(ts, SEC_TS_CMD_SET_TOUCHFUNCTION,
				(u8 *)&(ts->touch_functions), 2);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to read touch functions(%d)\n",
				__func__, ret);
		goto err_i2c;
	}

	ts->touch_functions |= SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC;
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHFUNCTION,
					(u8 *)&ts->touch_functions, 2);
	if (ret < 0) {
		TS_LOG_ERR("%s: Failed to send touch func_mode", __func__);
		goto err_i2c;
	}

	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0) {
		TS_LOG_ERR("%s: fail to write Sense_on\n", __func__);
		goto err_i2c;
	}

	ts->pFrame = kzalloc(ts->tx_count * ts->rx_count * 2, GFP_KERNEL);
	if (!ts->pFrame) {
		ret = -ENOMEM;
		TS_LOG_ERR("%s: fail to allacate pFrame\n", __func__);
		goto err_i2c;

	}

	TS_LOG_INFO("%s: Functions : %02X\n", __func__, ts->touch_functions);
err_i2c:
	return ret;
}

static int sec_ts_check_bootstatus(struct sec_ts_data *ts)
{
	int ret = 0;
	unsigned char data[5] = { 0 };

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_BOOT_STATUS, &data[0], 1);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to read sub id(%d)\n",
					__func__, ret);
	} else {
		ret = sec_ts_i2c_read(ts, SEC_TS_READ_TS_STATUS, &data[1], 4);
		if (ret < 0) {
			TS_LOG_ERR("%s: failed to touch status(%d)\n",
						__func__, ret);
		}
	}
	TS_LOG_ERR("%s: TOUCH STATUS : %02X || %02X, %02X, %02X, %02X\n",
				__func__, data[0], data[1], data[2], data[3], data[4]);

	if (data[0] == SEC_TS_STATUS_BOOT_MODE)
		ts->boot_mode = TOUCH_BOOT;
	else if (data[0] == SEC_TS_STATUS_APP_MODE)
		ts->boot_mode = TOUCH_APP;

	return ret;

}
static int i2c_communicate_check(void)
{
	struct sec_ts_data *ts = tsp_info;
	int ret = 0;
	bool valid_firmware_integrity = false;
	unsigned char data[5] = { 0 };
	unsigned char deviceID[5] = { 0 };
	unsigned char result = 0;

	ret = sec_ts_i2c_boot_check(ts);
	if (ret < 0) {
		TS_LOG_ERR("%s: timeover waiting for boot\n", __func__);
		ret = sec_ts_check_bootstatus(ts);
		if (ret < 0) {
			TS_LOG_ERR("%s: failed to check bootstatus\n", __func__);
			goto err_init;
		}

	} else
		ts->boot_mode = TOUCH_APP;

	if ( ts->boot_mode != TOUCH_APP)
		goto err_init;

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_DEVICE_ID, deviceID, 5);
	if (ret < 0)
		TS_LOG_ERR("%s: failed to read device ID(%d)\n", __func__, ret);
	else
		TS_LOG_INFO("%s: TOUCH DEVICE ID : %02X, %02X, %02X, %02X, %02X\n",
					__func__, deviceID[0], deviceID[1],
					deviceID[2], deviceID[3], deviceID[4]);

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_FIRMWARE_INTEGRITY, &result, 1);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to integrity check (%d)\n", __func__, ret);
	} else {
		if (result & 0x80) {
			valid_firmware_integrity = true;
		} else if (result & 0x40) {
			valid_firmware_integrity = false;
			TS_LOG_ERR("%s: invalid firmware (0x%x)\n", __func__, result);
		} else {
			valid_firmware_integrity = false;
			TS_LOG_ERR("%s: invalid integrity result (0x%x)\n", __func__, result);
		}
	}

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_BOOT_STATUS, &data[0], 1);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to read sub id(%d)\n",
					__func__, ret);
	} else {
		ret = sec_ts_i2c_read(ts, SEC_TS_READ_TS_STATUS, &data[1], 4);
		if (ret < 0) {
			TS_LOG_ERR("%s: failed to touch status(%d)\n",
						__func__, ret);
		}
	}
	TS_LOG_INFO("%s: TOUCH STATUS : %02X || %02X, %02X, %02X, %02X\n",
				__func__, data[0], data[1], data[2], data[3], data[4]);

	if (data[0] == SEC_TS_STATUS_BOOT_MODE)
		ts->checksum_result = 1;

	if ((((data[0] == SEC_TS_STATUS_APP_MODE) &&
		(data[2] == TOUCH_SYSTEM_MODE_FLASH)) ||
		(ret < 0)) && (valid_firmware_integrity == false))
		ts->boot_mode = TOUCH_APP_FLASH;

	ret = sec_ts_read_pid(ts);
	if (ret < 0)
		goto err_init;

	ret = sec_ts_read_information(ts);
	if (ret < 0) {
		TS_LOG_ERR("%s: fail to read information 0x%x\n", __func__, ret);
		goto err_init;
	}

	TS_LOG_INFO("%s: communication check success!\n", __func__);
	return ret;
err_init:
	return ret;


}

static int sec_ts_chip_detect(struct ts_kit_platform_data *data)
{
	struct sec_ts_data *ts = tsp_info;
	struct sec_ts_plat_data *pdata;
	int retval = NO_ERR;

	TS_LOG_INFO("sec_ts chip detect called\n");
	if ((!data) && (!data->ts_dev)) {
		TS_LOG_ERR("device, ts_kit_platform_data *data or data->ts_dev is NULL \n");
		retval =  -EINVAL;
		goto out;
	}

	ts->chip_data->ts_platform_data = data;
	ts->sec_ts_pdev = data->ts_dev;
	ts->sec_ts_pdev->dev.of_node = ts->chip_data->cnode;

	/*setting the default data*/
	ts->chip_data->is_i2c_one_byte = 0;
	ts->chip_data->is_new_oem_structure = 0;
	ts->chip_data->is_parade_solution = 0;

	ts->sec_ts_i2c_read = sec_ts_i2c_read;
	ts->sec_ts_i2c_write = sec_ts_i2c_write;
	ts->sec_ts_i2c_write_burst = sec_ts_i2c_write_burst;
	ts->sec_ts_i2c_read_bulk = sec_ts_i2c_read_bulk;

	pdata = kzalloc(sizeof(struct sec_ts_plat_data), GFP_KERNEL);
	if (!pdata)
		goto error_allocate_pdatamem;

	ts->plat_data = pdata;
	ts->client = ts->chip_data->ts_platform_data->client;
	ts->event_length = SEC_TS_EVENT_BUFF_SIZE;

	ts->event_buff = kzalloc((MAX_EVENT_COUNT + 1) * ts->event_length, GFP_KERNEL);
	if (!ts->event_buff)
		goto event_err;

	retval = sec_ts_parse_dt(ts->sec_ts_pdev->dev.of_node, ts->chip_data);
	if (retval) {
		TS_LOG_ERR("%s: dts parse fail\n", __func__);
		goto dts_err;
	}

	ts->client->addr = ts->chip_data->slave_addr;

	sec_ts_gpio_request(ts);
	sec_ts_regulator_get(ts);
	ts->i2c_burstmax = pdata->i2c_burstmax;

	mutex_init(&ts->lock);
	mutex_init(&ts->device_mutex);
	mutex_init(&ts->i2c_mutex);
	mutex_init(&ts->eventlock);

	mutex_init(&wrong_touch_lock);

	/*power up the chip */
	sec_ts_power(ts, true);
	sec_ts_delay(SEC_TS_POWER_ON_DEALY_MS);
	ts->power_status = SEC_TS_STATE_POWER_ON;
	ts->external_factory = false;

	TS_LOG_INFO("%s: power enable\n", __func__);

	retval = i2c_communicate_check();
	if (retval < 0) {
		TS_LOG_ERR("not find sec_ts device\n");
		goto check_err;
	} else {
		TS_LOG_INFO("find sec_ts device\n");
		strncpy(ts->chip_data->chip_name, SEC_TS_I2C_NAME,
			(MAX_STR_LEN > strlen(SEC_TS_I2C_NAME)) ?
				strlen(SEC_TS_I2C_NAME):(MAX_STR_LEN - 1));
	}

	if(S6SY761X == tsp_info->ic_name) {
		strncpy(tsp_info->chip_data->module_name, tsp_info->module_name, MAX_STR_LEN - 1);
	}
	sec_ts_raw_device_init(ts);
#ifdef ROI
	init_completion(&roi_data_done);
#endif
	tsp_info = ts;
	ts->probe_done = true;
	TS_LOG_INFO("%s: done\n", __func__);
	TS_LOG_INFO("sec_ts chip detect successful\n");
	return NO_ERR;


check_err:
	sec_ts_power(ts, false);
	sec_ts_regulator_put(ts);
	sec_ts_gpio_free(ts);
dts_err:
	kfree(ts->event_buff);
event_err:
	kfree(ts->plat_data);
error_allocate_pdatamem:
	retval = -ENODEV;
out:
	if (tsp_info->chip_data)
		kfree(tsp_info->chip_data);
	if (tsp_info)
		kfree(tsp_info);
	tsp_info = NULL;
	return retval;

}

static int sec_ts_init_chip(void)
{
	struct sec_ts_data *ts = tsp_info;
	struct sec_ts_plat_data *pdata = tsp_info->plat_data;
	int ret = 0;

	TS_LOG_INFO("%s\n", __func__);

	ts->flash_page_size = SEC_TS_FW_BLK_SIZE_DEFAULT;
	ts->i2c_burstmax = pdata->i2c_burstmax;
	ts->touch_count = 0;
	ts->max_z_value = 0;
	ts->min_z_value = 0xFFFFFFFF;
	ts->sum_z_value = 0;
	ts->flip_enable = false;
	ts->lowpower_mode = false;

	wake_lock_init(&ts->wakelock, WAKE_LOCK_SUSPEND, "tsp_wakelock");
	init_completion(&ts->resume_done);

#if defined (CONFIG_TEE_TUI)
	/* i2c slave address is 0x48, distinguish from device name with i2c address 0x17 */
	if(ts->chip_data->slave_addr == SEC_TS_DEFAULT_I2C_ADDR) {
		strncpy(tee_tui_data.device_name, IC_SEC_Y761, strlen(IC_SEC_Y761));
		tee_tui_data.device_name[strlen(IC_SEC_Y761)] = '\0';
	} else { /* i2c slave address is 0x17, use the default device name */
		strncpy(tee_tui_data.device_name, "sec", strlen("sec"));
		tee_tui_data.device_name[strlen("sec")] = '\0';
	}
	TS_LOG_INFO("device_name:%s\n", tee_tui_data.device_name);
#endif
	TS_LOG_INFO("%s: init resource\n", __func__);

	return ret;
}

static int sec_ts_input_config(struct input_dev *input_dev)
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

	if (tsp_info->plat_data->in_recovery_mode == USE_IN_RECOVERY_MODE && tsp_info->plat_data->use_ic_res == USE_IC_RESOLUTION) {
		input_set_abs_params(input_dev, ABS_X,
				0, tsp_info->plat_data->x_max_rc, 0, 0);
		input_set_abs_params(input_dev, ABS_Y,
				0, tsp_info->plat_data->y_max_rc, 0, 0);
	} else {
		input_set_abs_params(input_dev, ABS_X,
				0, tsp_info->plat_data->max_x, 0, 0);
		input_set_abs_params(input_dev, ABS_Y,
				0, tsp_info->plat_data->max_y, 0, 0);
	}
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 15, 0, 0);

	if (tsp_info->plat_data->in_recovery_mode == USE_IN_RECOVERY_MODE && tsp_info->plat_data->use_ic_res == USE_IC_RESOLUTION) {
		input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0,
				tsp_info->plat_data->x_max_rc, 0, 0);
		input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0,
				tsp_info->plat_data->y_max_rc, 0, 0);
	} else {
		input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0,
				tsp_info->plat_data->max_x, 0, 0);
		input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0,
				tsp_info->plat_data->max_y, 0, 0);
	}
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
#if ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 100, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MINOR, 0, 100, 0, 0);
#else
	input_set_abs_params(input_dev, ABS_MT_DISTANCE, 0, 100, 0, 0);
#endif
#ifdef REPORT_2D_W
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0,
				MAX_ABS_MT_TOUCH_MAJOR, 0, 0);
#endif
	tsp_info->input_dev = input_dev;

	return NO_ERR;
}

static int sec_ts_chip_get_info(struct ts_chip_info_param *info)
{
	memcpy(&info->ic_vendor, SEC_TS_IC_VENDOR, strlen(SEC_TS_IC_VENDOR));

	memcpy(&info->mod_vendor, tsp_info->project_id, strlen(tsp_info->project_id));

	snprintf(info->fw_vendor, CHIP_INFO_LENGTH * 2 - 1, "%02X.%02X.%02X.%02X",
			tsp_info->plat_data->img_version_of_ic[0],
			tsp_info->plat_data->img_version_of_ic[1],
			tsp_info->plat_data->img_version_of_ic[2],
			tsp_info->plat_data->img_version_of_ic[3]);

	return NO_ERR;
}

static int sec_ts_chip_get_capacitance_test_type(struct ts_test_type_info
							*info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR
			("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}
	switch (info->op_action) {
	case TS_ACTION_READ:
		strncpy(info->tp_test_type,
				tsp_info->cap_test_type,
				TS_CAP_TEST_TYPE_LEN - 1);
		TS_LOG_INFO("read_chip_get_test_type=%s, \n",
				tsp_info->cap_test_type);
		break;
	case TS_ACTION_WRITE:
		break;
	default:
		TS_LOG_ERR("invalid status: %s", info->tp_test_type);
		retval = -EINVAL;
		break;
	}
	return retval;
}


static int sec_ts_set_info_flag(struct ts_kit_platform_data *info)
{
	tsp_info->chip_data->ts_platform_data->get_info_flag = info->get_info_flag;
	return NO_ERR;
}

static int sec_ts_fw_update_boot(char *file_name)
{
	int ret = NO_ERR;

	msleep(1000);

	if (tsp_info->boot_mode == TOUCH_APP) {
		sec_ts_set_firmware_name(tsp_info);
		ret = sec_ts_firmware_update_on_probe(tsp_info, false);
		if (ret < 0)
			TS_LOG_ERR("%s: boot firmware download failed: %s\n",
				__func__, file_name);
	} else {
		ret = sec_ts_read_pid(tsp_info);
		if (ret < 0)
			TS_LOG_ERR("%s: fail to read pid from bl\n",__func__);

		sec_ts_set_firmware_name(tsp_info);
		ret = sec_ts_firmware_update_on_probe(tsp_info, true);
		if (ret < 0)
			TS_LOG_ERR("%s: fail to download boot firmware download: %s\n",
					__func__, file_name);

		ret = sec_ts_read_information(tsp_info);
		if (ret < 0) {
			TS_LOG_ERR("%s: fail to read information 0x%x\n", __func__, ret);
		}
	}

	return ret;
}

static int sec_ts_fw_update_sd(void)
{
	int ret = NO_ERR;

	ret = sec_ts_load_fw_request(tsp_info, true);
	if (ret < 0)
		TS_LOG_ERR("%s: firmware download failed\n", __func__);
	return ret;
}

static int sec_ts_wakeup_gesture_enable_switch(
	struct ts_wakeup_gesture_enable_info *info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}

	if (info->op_action == TS_ACTION_WRITE) {
		tsp_info->lowpower_mode = SEC_TS_MODE_LOWPOWER_FLAG;

		TS_LOG_DEBUG("write deep_sleep switch: %d\n",
					info->switch_value);
		if (retval < 0) {
			TS_LOG_ERR("set deep_sleep switch(%d), failed: %d\n",
						info->switch_value, retval);
		}
	} else {
		TS_LOG_INFO("invalid deep_sleep switch(%d) action: %d\n",
					info->switch_value, info->op_action);
		retval = -EINVAL;
	}

	return retval;
}

static int sec_ts_get_rawdata(struct ts_rawdata_info *info,
				struct ts_cmd_node *out_cmd)
{
	int ret = NO_ERR;
	struct sec_ts_data *ts = tsp_info;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: Touch is stopped!\n", __func__);
		return -EIO;
	}

	ret = sec_ts_run_self_test(ts, info);
	if (ret < 0) {
		TS_LOG_ERR("%s: failed to get rawdata!\n", __func__);
	}
	TS_LOG_INFO("%s: finished getting rawdata!\n", __func__);
	return ret;
}

static int sec_ts_calibration_test(void)
{
	int retval = NO_ERR;
	struct sec_ts_data *ts = tsp_info;
	char buff[SEC_CMD_STR_LEN] = {0};
	char mis_cal_data = 0xF0;
	char mis_amb_data = 0xF0;

	disable_irq(ts->client->irq);

	buff[0] = 0;
	retval = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, buff, 1);
	if (retval < 0) {
		TS_LOG_ERR("%s: mis_cal_check error[1] ret: %d\n", __func__, retval);
	}

	buff[0] = 0x2;
	buff[1] = 0x2;
	retval = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, buff, 2);
	if (retval < 0) {
		TS_LOG_ERR("%s: mis_cal_check error[2] ret: %d\n", __func__, retval);
	}

	TS_LOG_ERR("%s: try mis Amb. check\n", __func__);
	retval = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MIS_CAL_CHECK, ts->cal_limit, 3);
	if (retval < 0) {
		TS_LOG_ERR("%s: mis_cal_check error[4] ret: %d\n", __func__, retval);
	}

	buff[0] = SEC_TS_PARA_AMBTEST;
	retval = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MIS_CAL_CHECK, buff, 1);
	if (retval < 0) {
		TS_LOG_ERR("%s: mis_cal_check error[3] ret: %d\n", __func__, retval);
	}
	sec_ts_delay(200);

	retval = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_MIS_CAL_READ, &mis_amb_data, 1);
	if (retval < 0) {
		TS_LOG_ERR("%s: i2c fail!, %d\n", __func__, retval);
		mis_cal_data = 0xF3;
	} else {
		TS_LOG_ERR("%s: miss cal data : %d\n", __func__, mis_cal_data);
	}

	TS_LOG_ERR("%s: try mis Cal. check\n", __func__);
	retval = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MIS_CAL_CHECK, ts->cal_limit, 3);
	if (retval < 0) {
		TS_LOG_ERR("%s: mis_cal_check error[4] ret: %d\n", __func__, retval);
	}

	buff[0] = SEC_TS_PARA_CALTEST;
	retval = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MIS_CAL_CHECK, buff, 1);
	if (retval < 0) {
		TS_LOG_ERR("%s: mis_cal_check error[5] ret: %d\n", __func__, retval);
	}
	sec_ts_delay(200);

	retval = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_MIS_CAL_READ, &mis_cal_data, 1);
	if (retval < 0) {
		TS_LOG_ERR("%s: i2c fail!, %d\n", __func__, retval);
		mis_cal_data = 0xF3;
	} else {
		TS_LOG_ERR("%s: miss cal data : %d\n", __func__, mis_cal_data);
	}

	buff[0] = 1;
	retval = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, buff, 1);
	if (retval < 0) {
		TS_LOG_ERR("%s: mis_cal_check error[6] ret: %d\n", __func__, retval);
	}

	if ((mis_amb_data != 0) && (mis_cal_data != 0))
		retval = -EINVAL;

	return retval;
}

static int sec_ts_get_calibration_data(struct ts_calibration_data_info *info, struct ts_cmd_node *out_cmd)
{
	int retval = NO_ERR;
	int used_datasize;
	struct sec_ts_data *ts = tsp_info;
	struct sec_ts_test_mode mode;

	if (ts->touch_count > 0) {
		TS_LOG_ERR("%s: NG_FINGER_ON\n", __func__);
		return -1;
	}

	sec_ts_calibration_test();

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_AMBIENT_DATA;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, &mode);
	TS_LOG_ERR("%s: MIS CAL", __func__);

	enable_irq(ts->client->irq);

	used_datasize = ts->tx_count*ts->rx_count;
	memcpy(info->data, ts->pFrame, used_datasize);

	info->used_size = used_datasize;
	TS_LOG_INFO("info->used_size = %d\n", info->used_size);
	info->tx_num = ts->tx_count;
	info->rx_num = ts->rx_count;
	TS_LOG_INFO("info->tx_num = %d\n", info->tx_num);
	TS_LOG_INFO("info->rx_num = %d\n", info->rx_num);

	return retval;

}

static int sec_ts_get_calibration_info(struct ts_calibration_info_param *info, struct ts_cmd_node *out_cmd)
{

	int retval = NO_ERR;
	int cal_test_report = 0;
	retval = sec_ts_calibration_test();
	if (retval < 0)
		cal_test_report = 0;
	else
		cal_test_report = 1;
	info->calibration_crc = cal_test_report;
	TS_LOG_INFO("info->calibration_crc = %d\n", info->calibration_crc);

	return retval;
}

static int sec_ts_get_debug_data(struct ts_diff_data_info*info,
						struct ts_cmd_node *out_cmd)
{
	int retval = NO_ERR;
	struct ts_diff_data_info *info_diff = (struct ts_diff_data_info *)info;

	TS_LOG_INFO("%s: start\n", __func__);

	switch (info_diff->debug_type) {
	case READ_DIFF_DATA:
		retval = sec_ts_get_delta_debug(tsp_info, info_diff);
		if (retval < 0)
			TS_LOG_ERR("%s: failed to run delta\n", __func__);
		break;
	case READ_RAW_DATA:
		retval = sec_ts_get_rawcap_debug(tsp_info, info_diff);
		if (retval < 0)
			TS_LOG_ERR("%s: failed to run delta\n", __func__);
		break;
	default:
		TS_LOG_ERR("failed to recognize ic_ver\n");
		break;
	}

	TS_LOG_INFO("%s: end\n", __func__);
	return retval;

}

static int sec_ts_glove_switch(struct ts_glove_info *info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}
	switch (info->op_action) {
	case TS_ACTION_READ:
		info->glove_switch = (tsp_info->touch_functions & SEC_TS_BIT_SETFUNC_GLOVE)?1:0;
		TS_LOG_INFO("read_glove_switch=%d, 1:on 0:off\n",
					info->glove_switch);
		break;
	case TS_ACTION_WRITE:
		TS_LOG_INFO("write_glove_switch=%d\n", info->glove_switch);
		retval = sec_ts_glove_mode_enables(tsp_info, info->glove_switch);
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

	return retval;
}

static int sec_ts_charger_switch(struct ts_charger_info *info)
{
	int retval = NO_ERR;

#if defined(HUAWEI_CHARGER_FB)
	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		retval = sec_ts_charger_mode_enables(tsp_info, info->charger_switch);
		if (retval < 0) {
			TS_LOG_ERR("set charger switch(%d), failed: %d\n",
						info->charger_switch, retval);
		}
		break;
	default:
		TS_LOG_INFO("%s, invalid cmd\n", __func__);
		retval = -EINVAL;
		break;
	}
#endif

	return retval;
}

static int sec_ts_holster_switch(struct ts_holster_info *info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}

	if(S6SY761X == tsp_info->ic_name && info->holster_supported == 0) {
		TS_LOG_INFO("%s, not support holster\n", __func__);
		return NO_ERR;
	}
	switch (info->op_action) {
	case TS_ACTION_WRITE:
		tsp_info->flip_enable = info->holster_switch;
		tsp_info->cover_type = SEC_TS_VIEW_COVER;
		retval = sec_ts_cover_mode_enalbes(tsp_info, info->holster_switch);
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
		TS_LOG_INFO("invalid holster switch(%d) action: %d\n",
				info->holster_switch, info->op_action);
		retval = -EINVAL;
		break;
	}

	return retval;
}

static int sec_ts_set_roi_switch(u8 roi_switch)
{
	int retval = NO_ERR;
	int i;
	unsigned short roi_ctrl_addr = 0;
	u8 roi_control_bit, temp_roi_switch;

#ifdef ROI
	roi_ctrl_addr = tsp_info->chip_data->ts_platform_data->feature_info.roi_info.roi_control_addr;
	roi_control_bit = tsp_info->chip_data->ts_platform_data->feature_info.roi_info.roi_control_bit;
	TS_LOG_INFO
		("roi_ctrl_write_addr=0x%04x, roi_control_bit=%d, roi_switch=%d\n",
			roi_ctrl_addr, roi_control_bit, roi_switch);

	if (!roi_ctrl_addr) {
		TS_LOG_ERR
			("roi_ctrl_addr is null, roi feature is not supported.\n");
		goto out;
	}

	retval =
		sec_ts_i2c_read(tsp_info, roi_ctrl_addr, &temp_roi_switch, 1);
	if (retval < 0) {
		TS_LOG_ERR("read roi_control value information failed: %d\n",
					retval);
		return retval;
	}

	if (roi_switch)
		temp_roi_switch |= (1 << roi_control_bit);
	else
		temp_roi_switch &= (~(1 << roi_control_bit));

	retval =
		sec_ts_i2c_write(tsp_info, roi_ctrl_addr, &temp_roi_switch, 1);
	if (retval < 0) {
		TS_LOG_ERR
			("set roi switch failed: %d, roi_ctrl_write_addr=0x%04x\n",
				retval, roi_ctrl_addr);
		return retval;
	}
	roi_switch_on = temp_roi_switch;
	if (!roi_switch) {
		for (i = 0; i < ROI_DATA_READ_LENGTH; i++) {
			roi_data[i] = 0;
		}
	}
#endif
out:
	return retval;
}

static int sec_ts_read_roi_switch(void)
{
	int retval = NO_ERR;
	unsigned short roi_ctrl_addr = 0;
	u8 roi_control_bit = 0;
	u8 roi_switch = 0;
#ifdef ROI
	roi_ctrl_addr = tsp_info->chip_data->ts_platform_data->feature_info.roi_info.roi_control_addr;
	roi_control_bit = tsp_info->chip_data->ts_platform_data->feature_info.roi_info.roi_control_bit;

	if (!roi_ctrl_addr) {
		TS_LOG_ERR
			("roi_ctrl_addr is null, roi feature is not supported.\n");
		goto out;
	}

	retval =
		sec_ts_i2c_read(tsp_info, roi_ctrl_addr, &roi_switch,
					sizeof(roi_switch));
	if (retval < 0) {
		TS_LOG_ERR("read roi switch failed: %d\n", retval);
		return retval;
	}
	roi_switch &= (1 << roi_control_bit);
	tsp_info->chip_data->ts_platform_data->feature_info.roi_info.roi_switch = roi_switch;
	roi_switch_on = roi_switch;
#endif
out:
	TS_LOG_INFO("roi_ctrl_read_addr=0x%04x, roi_switch=%d\n", roi_ctrl_addr,
				roi_switch);
	return retval;
}


static int sec_ts_roi_switch(struct ts_roi_info *info)
{
	int retval = NO_ERR;

#ifdef ROI
	if (!info) {
		TS_LOG_ERR("sec_ts_roi_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		retval = sec_ts_set_roi_switch(info->roi_switch);
		if (retval < 0) {
			TS_LOG_ERR("%s, sec_ts_set_roi_switch faild\n",
						__func__);
		}
		break;
	case TS_ACTION_READ:

		retval = sec_ts_read_roi_switch();
#endif
		break;
	default:
		TS_LOG_INFO("invalid roi switch(%d) action: %d\n",
					info->roi_switch, info->op_action);
		retval = -EINVAL;
		break;
	}
	return retval;
}

static unsigned char *sec_ts_roi_rawdata(void)
{
#ifdef ROI
	if (roi_switch_on)  {
		if (roi_data_staled)  {  /* roi_data may be refreshing now, wait it for some time(30ms). */
			if (!wait_for_completion_interruptible_timeout(&roi_data_done, msecs_to_jiffies(30))) {
				roi_data_staled = 0;  /* Never wait again if data refreshing gets timeout. */
				memset(roi_data, 0, sizeof(roi_data));
			}
		}
	}

	return (unsigned char *)roi_data;
#else
	return NULL;
#endif
}

static int sec_ts_regs_operate(struct ts_regs_info *info)
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
	switch (info->op_action) {
	case TS_ACTION_WRITE:
		for (i = 0; i < info->num; i++) {
			value[i] = info->values[i];
		}

		if ((1 == info->num) && (8 > info->bit)) {
			retval =
			    sec_ts_i2c_read(tsp_info, info->addr,
						    value, info->num);
			if (retval < 0) {
				TS_LOG_ERR("TS_ACTION_READ error, addr(%d)\n",
					   info->addr);
				retval = -EINVAL;
				goto out;
			}

			if (0 == value_tmp) {
				value[0] &= ~(1 << info->bit);
			} else if (1 == value_tmp) {
				value[0] |= (1 << info->bit);
			}
		}

		retval =
		    sec_ts_i2c_write(tsp_info, info->addr, value,
					     info->num);
		if (retval < 0) {
			TS_LOG_ERR("TS_ACTION_WRITE error, addr(%d)\n",
				   info->addr);
			retval = -EINVAL;
			goto out;
		}
		break;
	case TS_ACTION_READ:
		retval =
		    sec_ts_i2c_read(tsp_info, info->addr, value,
					    info->num);
		if (retval < 0) {
			TS_LOG_ERR("TS_ACTION_READ error, addr(%d)\n",
				   info->addr);
			retval = -EINVAL;
			goto out;
		}

		if ((1 == info->num) && (8 > info->bit)) {
			tsp_info->chip_data->reg_values[0] =
			    (value[0] >> info->bit) & 0x01;
		} else {
			for (i = 0; i < info->num; i++) {
				tsp_info->chip_data->reg_values[i] =
				    value[i];
			}
		}
		break;
	default:
		TS_LOG_ERR("%s, reg operate default invalid action %d\n",
					__func__, info->op_action);
		retval = -EINVAL;
		break;
	}
out:
	return retval;
}

static int run_force_calibration(void)
{
	struct sec_ts_data *ts = tsp_info;
	char buff[SEC_CMD_STR_LEN] = {0};
	int rc = NO_ERR;

	struct sec_ts_test_mode mode;
	char mis_cal_data = 0xF0;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: Touch is stopped!\n", __func__);
		return -EIO;
	}

	rc = sec_ts_read_calibration_report(ts);
	if (rc < 0) {
		TS_LOG_ERR("%s: cal report read error\n", __func__);
		goto out_force_cal;
	}

	if (ts->touch_count > 0) {
		TS_LOG_ERR("%s: NG_FINGER_ON\n", __func__);
		goto out_force_cal;
	}

	disable_irq(ts->client->irq);

	rc = sec_ts_execute_force_calibration(ts, OFFSET_CAL_SEC);
	if (rc < 0) {
		TS_LOG_ERR("%s: force calibration fail\n", __func__);
		goto out_force_cal;
	} else {

		if (ts->plat_data->mis_cal_check) {
			buff[0] = 0;
			rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, buff, 1);
			if (rc < 0) {
				TS_LOG_ERR("%s: mis_cal_check error[1] ret: %d\n", __func__, rc);
			}

			buff[0] = 0x2;
			buff[1] = 0x2;
			rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, buff, 2);
			if (rc < 0) {
				TS_LOG_ERR("%s: mis_cal_check error[2] ret: %d\n", __func__, rc);
			}

			TS_LOG_ERR("%s: try mis Cal. check\n", __func__);
			rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MIS_CAL_CHECK, NULL, 0);
			if (rc < 0) {
				TS_LOG_ERR("%s: mis_cal_check error[3] ret: %d\n", __func__, rc);
			}
			sec_ts_delay(200);

			rc = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_MIS_CAL_READ, &mis_cal_data, 1);
			if (rc < 0) {
				TS_LOG_ERR("%s: i2c fail!, %d\n", __func__, rc);
				mis_cal_data = 0xF3;
			} else {
				TS_LOG_ERR("%s: miss cal data : %d\n", __func__, mis_cal_data);
			}

			buff[0] = 1;
			rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, buff, 1);
			if (rc < 0) {
				TS_LOG_ERR("%s: mis_cal_check error[4] ret: %d\n", __func__, rc);
			}

			if (mis_cal_data) {
				memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
				mode.type = TYPE_AMBIENT_DATA;
				mode.allnode = TEST_MODE_ALL_NODE;

				sec_ts_read_raw_data(ts, &mode);
				TS_LOG_ERR("%s: MIS CAL\n", __func__);

				enable_irq(ts->client->irq);

				goto out_force_cal;
			}
		}
		TS_LOG_ERR("%s: calibration ok\n", __func__);
	}

	enable_irq(ts->client->irq);
	return rc;
out_force_cal:
	TS_LOG_INFO("%s: %s\n", __func__, buff);
	return rc;
}

int sec_ts_do_once_calibrate(void)
{
	int ret = NO_ERR;
	ret = run_force_calibration();
	if (ret) {
		TS_LOG_ERR("%s error\n", __func__);
	}

	return ret;
}

static int sec_ts_calibrate(void)
{
	int ret = NO_ERR;

	ret = run_force_calibration();
	if (ret) {
		TS_LOG_ERR("%s error\n", __func__);
	}

	return ret;
}

static int sec_ts_wrong_touch(void)
{
	int rc = NO_ERR;
	mutex_lock(&wrong_touch_lock);
	tsp_info->chip_data->easy_wakeup_info.off_motion_on = true;
	mutex_unlock(&wrong_touch_lock);
	TS_LOG_INFO("done\n");
	return rc;
}
void sec_ts_unlocked_release_all_finger(struct sec_ts_data *ts)
{
	int i;

	for (i = 0; i < MAX_SUPPORT_TOUCH_COUNT; i++) {
		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);

		if ((ts->coord[i].action == SEC_TS_COORDINATE_ACTION_PRESS) ||
			(ts->coord[i].action == SEC_TS_COORDINATE_ACTION_MOVE)) {

			ts->coord[i].action = SEC_TS_COORDINATE_ACTION_RELEASE;
			TS_LOG_DEBUG(
					"%s: [RA] tID:%d mc:%d tc:%d v:%02X%02X cal:%02X(%02X) id(%d,%d) p:%d\n",
					__func__, i, ts->coord[i].mcount, ts->touch_count,
					ts->plat_data->img_version_of_ic[2],
					ts->plat_data->img_version_of_ic[3],
					ts->cal_status, ts->nv, ts->tspid_val,
					ts->tspicid_val, ts->coord[i].palm_count);

			do_gettimeofday(&ts->time_released[i]);

			if (ts->time_longest < (ts->time_released[i].tv_sec - ts->time_pressed[i].tv_sec))
				ts->time_longest = (ts->time_released[i].tv_sec - ts->time_pressed[i].tv_sec);
		}

		ts->coord[i].mcount = 0;
		ts->coord[i].palm_count = 0;

	}

	ts->touchkey_glove_mode_status = false;
	ts->touch_count = 0;
	ts->check_multi = 0;
}

int sec_ts_set_lowpowermode(struct sec_ts_data *ts, u8 mode)
{
	int ret;
	int retrycnt = 0;
	u8 data;
	char para = 0;

	TS_LOG_INFO("%s: %s(%X)\n", __func__,
			mode == TO_LOWPOWER_MODE ? "ENTER" : "EXIT", ts->lowpower_mode);

	if (mode) {
		data = (ts->lowpower_mode & SEC_TS_MODE_LOWPOWER_FLAG) >> 1;
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_WAKEUP_GESTURE_MODE, &data, 1);
		if (ret < 0)
			TS_LOG_ERR("%s: Failed to set\n", __func__);
	}

retry_pmode:
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &mode, 1);
	if (ret < 0)
		TS_LOG_ERR("%s: failed\n", __func__);
	sec_ts_delay(50);

	/* read data */
	ret = sec_ts_i2c_read(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
	if (ret < 0)
		TS_LOG_ERR("%s: read power mode failed!\n", __func__);
	else
		TS_LOG_ERR("%s: power mode - write(%d) read(%d)\n", __func__, mode, para);

	if (mode != para) {
		retrycnt++;
		if (retrycnt < SEC_TS_I2C_RETRY_CNT)
			goto retry_pmode;
	}

	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
	if (ret < 0)
		TS_LOG_ERR("%s: i2c write clear event failed\n", __func__);


	if (device_may_wakeup(&ts->sec_ts_pdev->dev)) {
		if (mode)
			enable_irq_wake(ts->client->irq);
		else
			disable_irq_wake(ts->client->irq);
	}

	ts->lowpower_status = mode;
	TS_LOG_ERR("%s: end\n", __func__);

	return ret;
}

int sec_ts_set_sleepmode(struct sec_ts_data *ts, u8 mode)
{
	int ret;
	int retrycnt = 0;
	char para = 0;

	TS_LOG_ERR("%s: %s(%X)\n", __func__,
			mode == TO_SLEEP_MODE ? "ENTER" : "EXIT", ts->lowpower_mode);

retry_pmode:
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &mode, 1);
	if (ret < 0)
		TS_LOG_ERR("%s: failed\n", __func__);
	sec_ts_delay(70);

	ret = sec_ts_i2c_read(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
	if (ret < 0)
		TS_LOG_ERR("%s: read power mode failed!\n", __func__);
	else
		TS_LOG_INFO("%s: power mode - write(%d) read(%d)\n", __func__, mode, para);

	if (mode != para) {
		retrycnt++;
		if (retrycnt < SEC_TS_I2C_RETRY_CNT)
			goto retry_pmode;
	}

	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
	if (ret < 0)
		TS_LOG_ERR("%s: i2c write clear event failed\n", __func__);

	ts->lowpower_status = mode;
	TS_LOG_INFO("%s: end\n", __func__);

	return ret;
}

static int sec_ts_charger_status_ctrl(struct sec_ts_data *ts, int enable)
{
	int restore_status = ts->chip_data->ts_platform_data->feature_info.charger_info.charger_switch;
	if (!enable) {
		/* pull down charger notify pin to prevent current leak */
		gpio_direction_output(ts->charger_notify_gpio, 0);
	} else {
		gpio_direction_output(ts->charger_notify_gpio, restore_status);
	}

	return 0;
}

static int sec_ts_resume(void)
{
	struct sec_ts_data *ts = tsp_info;
	int ret;

	ts->input_closed = false;

	TS_LOG_INFO("%s\n", __func__);

	if (!g_tskit_pt_station_flag) {
		ret = sec_ts_start_device(ts);
		if (ret < 0)
			TS_LOG_ERR("%s: Failed to start device\n", __func__);
	} else {
		if (ts->lowpower_mode == SEC_TS_MODE_LOWPOWER_FLAG)
			sec_ts_set_lowpowermode(ts, TO_TOUCH_MODE);
		else
			sec_ts_set_sleepmode(ts, TO_TOUCH_MODE);
		ts->power_status = SEC_TS_STATE_POWER_ON;
	}

	sec_ts_charger_status_ctrl(ts, true);

	/* clear all touch finger status */
	memset(&ts->coord, 0, sizeof(struct sec_ts_coordinate) *
			(MAX_SUPPORT_TOUCH_COUNT + MAX_SUPPORT_HOVER_COUNT));
	ts->touch_count = 0;
	return 0;
}

static int sec_ts_after_resume (void* feature_info)
{
	struct sec_ts_data *ts = tsp_info;
	int ret;

	if (ts->chip_data->ts_platform_data->feature_info.holster_info.holster_switch) {
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_COVERTYPE, &ts->cover_cmd, 1);

		ts->touch_functions = ts->touch_functions | SEC_TS_BIT_SETFUNC_COVER;
		TS_LOG_ERR("%s: cover cmd write type:%d, mode:%x, ret:%d", __func__,
				ts->touch_functions, ts->cover_cmd, ret);
	} else {
		ts->touch_functions = (ts->touch_functions & (~SEC_TS_BIT_SETFUNC_COVER));
		TS_LOG_ERR(
			"%s: cover open, not send cmd", __func__);
	}

	ts->touch_functions = ts->touch_functions | SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC;
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHFUNCTION, (u8 *)&ts->touch_functions, 2);
	if (ret < 0)
		TS_LOG_ERR(
			"%s: Failed to send touch function command", __func__);

	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0)
		TS_LOG_ERR("%s: fail to write Sense_on\n", __func__);

	ret = sec_ts_glove_mode_enables(ts, ts->chip_data->ts_platform_data->feature_info.glove_info.glove_switch);
	if (ret)
		TS_LOG_ERR("%s:fail to restore glove status\n", __func__);

	enable_irq(ts->client->irq);

	return ret;
}

static int sec_ts_suspend(void)
{
	struct sec_ts_data *ts = tsp_info;

	ts->input_closed = true;

	TS_LOG_INFO("%s\n", __func__);

	sec_ts_charger_status_ctrl(ts, false);

	if (!g_tskit_pt_station_flag)
		sec_ts_stop_device(ts);
	else {
		if (ts->lowpower_mode == SEC_TS_MODE_LOWPOWER_FLAG)
			sec_ts_set_lowpowermode(ts, TO_LOWPOWER_MODE);
		else
			sec_ts_set_sleepmode(ts, TO_SLEEP_MODE);
		ts->power_status = SEC_TS_STATE_LPM_RESUME;
	}
	return 0;
}


static int sec_ts_remove(void)
{
	struct sec_ts_data *ts = tsp_info;

	TS_LOG_ERR("%s\n", __func__);

	ts->lowpower_mode = false;
	ts->probe_done = false;
	tsp_info = NULL;
	sec_ts_power(ts, false);
	kfree(ts->event_buff);
	kfree(ts->pFrame);
	kfree(ts);
	return 0;
}

static void sec_ts_shutdown(void)
{
	struct sec_ts_data *ts = tsp_info;

	TS_LOG_ERR("%s\n", __func__);

	sec_ts_power(ts, false);
}

int sec_ts_stop_device(struct sec_ts_data *ts)
{
	TS_LOG_ERR("%s\n", __func__);

	mutex_lock(&ts->device_mutex);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: already power off\n", __func__);
		goto out;
	}

	ts->power_status = SEC_TS_STATE_POWER_OFF;

	disable_irq(ts->client->irq);

	ts->plat_data->power(ts, false);

	if (ts->plat_data->enable_sync)
		ts->plat_data->enable_sync(false);

out:
	mutex_unlock(&ts->device_mutex);
	return 0;
}

int sec_ts_start_device(struct sec_ts_data *ts)
{
	int ret;

	TS_LOG_ERR("%s\n", __func__);

	mutex_lock(&ts->device_mutex);

	if (ts->power_status == SEC_TS_STATE_POWER_ON) {
		TS_LOG_ERR("%s: already power on\n", __func__);
		goto out;
	}

	ts->plat_data->power(ts, true);
	sec_ts_delay(100);
	ts->power_status = SEC_TS_STATE_POWER_ON;
	sec_ts_wait_for_ready(ts, SEC_TS_ACK_BOOT_COMPLETE);

	if (ts->plat_data->enable_sync)
		ts->plat_data->enable_sync(true);

out:
	mutex_unlock(&ts->device_mutex);
	return 0;
}

static int sec_ts_irq_top_half(struct ts_cmd_node *cmd)
{
	cmd->command = TS_INT_PROCESS;
	TS_LOG_DEBUG("sec_ts irq top half called\n");
	return NO_ERR;
}

static int sec_ts_irq_bottom_half(struct ts_cmd_node *in_cmd,
						struct ts_cmd_node *out_cmd)
{
	struct ts_fingers *info =
		&out_cmd->cmd_param.pub_params.algo_param.info;
	int ret;

	out_cmd->command = TS_INPUT_ALGO;
	out_cmd->cmd_param.pub_params.algo_param.algo_order =
		tsp_info->chip_data->algo_id;
	TS_LOG_DEBUG("order: %d\n",
				out_cmd->cmd_param.pub_params.algo_param.algo_order);

	if (sec_ts_read_info_event(tsp_info, info)) {
		out_cmd->command = TS_INVAILD_CMD;
		TS_LOG_DEBUG("%s:not need to report event\n", __func__);
	}

	return NO_ERR;
}

static int __init sec_ts_module_init(void)
{
	bool found = false;
	struct device_node *child = NULL;
	struct device_node *root = NULL;
	int error = NO_ERR;

	TS_LOG_INFO(" sec_ts_module_init called here\n");
	tsp_info = kzalloc(sizeof(*tsp_info) * 2, GFP_KERNEL);
	if (!tsp_info) {
		TS_LOG_ERR("Failed to alloc mem for struct sec_ts_data\n");
		error =  -ENOMEM;
		goto out;
	}
	tsp_info->chip_data = kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
	if (!tsp_info->chip_data) {
		TS_LOG_ERR("Failed to alloc mem for struct chip_data\n");
		error =  -ENOMEM;
		goto out;
	}
	root = of_find_compatible_node(NULL, NULL, "huawei,ts_kit");
	if (!root) {
		TS_LOG_ERR("huawei_ts, find_compatible_node huawei,ts_kit error\n");
		error = -EINVAL;
		goto out;
	}

	for_each_child_of_node(root, child) {
		if (of_device_is_compatible(child, "sec_ts")) {
			found = true;
			break;
		}
	}
	if (!found) {
		TS_LOG_ERR(" not found chip sec_ts child node  !\n");
		error = -EINVAL;
		goto out;
	}

	tsp_info->chip_data->cnode = child;
	tsp_info->chip_data->ops = &ts_kit_sec_ts_ops;

	error = huawei_ts_chip_register(tsp_info->chip_data);
	if (error) {
		TS_LOG_ERR(" sec_ts chip register fail !\n");
		goto out;
	}
	TS_LOG_INFO("sec_ts chip_register! err=%d\n", error);
	return error;
out:
	if (tsp_info && tsp_info->chip_data){
		kfree(tsp_info->chip_data);
		tsp_info->chip_data = NULL;
	}
	if (tsp_info)
		kfree(tsp_info);
	tsp_info = NULL;
	return error;
}

static void __exit sec_ts_module_exit(void)
{
	TS_LOG_INFO("sec_ts_module_exit called here\n");

	return;
}

late_initcall(sec_ts_module_init);
module_exit(sec_ts_module_exit);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");

