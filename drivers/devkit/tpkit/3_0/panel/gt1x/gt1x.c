/*
 * gt1x.c - Main touch driver file of gt1x
 *
 * Copyright (C) 2015 - 2016 gt1x Technology Incorporated
 * Copyright (C) 2015 - 2016 Yulong Cai <caiyulong@gt1x.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the gt1x's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/firmware.h>
#include "gt1x.h"
#include "gt1x_dts.h"
#include <linux/completion.h>
#include <linux/module.h>

#if defined (CONFIG_TEE_TUI)
#include "tui.h"
#endif

#define INDEX_0					0
#define INDEX_1 				1
#define I2C_RETRY_SLEEP_TIME	50
#define I2C_WD_DATA_LENGTH	4
#define I2C_ENABLE_BIT			1

#define TP_COLOR_REG			0x99a4
#define TP_COLOR_DATA_LENGTH	18
#define TP_COLOR_CHECKSUM		0
#define GOODIX_TP_COLOR_SIZE	15

#define GT1X_EDGE_DATA_SIZE		20

#define GT1X_EXIST				1
#define GT1X_NOT_EXIST			0

#define GT1X_RESET_SLEEP_TIME	80 /* 80ms */
#define GT1X_RESET_PIN_D_U_TIME	150 /* 150us */
#define GT1X_DELAY_10	10 /* 10ms */
#define GT1X_DELAY_80	80 /* 80ms */
#define GT1X_DELAY_INIT 55 /* 55ms */

#define GT1X_ROI_SRC_STATUS_INDEX		2
#define GT1X_BIT_AND_0x0F		0x0f
#define GT1X_BIT_AND_0x40		0x40
#if defined (CONFIG_TEE_TUI)
extern struct ts_tui_data tee_tui_data;
#endif
extern void gt1x_test_free(void);
static struct completion roi_data_done;
struct gt1x_ts_data *gt1x_ts = NULL;
static u16 pre_index = 0;
static u16 cur_index = 0;
static struct mutex wrong_touch_lock;
static DEFINE_MUTEX(wrong_touch_lock);
static void gt1x_pinctrl_select_normal(void);
static void gt1x_pinctrl_select_suspend(void);
static void gt1x_resume_chip_reset(void);
static int gt1x_reset_output(int level);
static void gt1x_irq_output(int level);
static int gt1x_power_release(void);
/**
 * gt1x_i2c_write - i2c write.
 * @addr: register address.
 * @buffer: data buffer.
 * @len: the bytes of data to write.
 * Return: 0: success, otherwise: failed
 */
int gt1x_i2c_write(u16 addr, u8 * buffer, s32 len)
{
	struct ts_kit_device_data *dev_data =  gt1x_ts->dev_data;
	u8 stack_mem[32], *data = NULL;
	int ret = 0;

	if (len + 2 > sizeof(stack_mem)) {
		data = kzalloc(len + 2, GFP_KERNEL);
		if (!data) {
			TS_LOG_ERR("%s: No memory\n", __func__);
			return -ENOMEM;
		}
	} else {
		data = &stack_mem[0];
	}

	data[0] = addr >> 8 & 0xff;
	data[1] = addr & 0xff;
	memcpy(&data[2], buffer, len);
	ret = dev_data->ts_platform_data->bops->bus_write(data, len + 2);
	if (ret < 0)
		TS_LOG_ERR("%s:i2c write error,addr:%04x bytes:%d\n", __func__, addr, len);

	if (data != &stack_mem[0]){
		kfree(data);
		data = NULL;
	}

	return ret;
}

/**
 * gt1x_i2c_read - i2c read.
 * @addr: register address.
 * @buffer: data buffer.
 * @len: the bytes of data to write.
 * Return: 0: success, otherwise: failed
 */
int gt1x_i2c_read(u16 addr, u8 * buffer, s32 len)
{
	struct ts_kit_device_data *dev_data = gt1x_ts->dev_data;
	int ret = 0;

	addr = cpu_to_be16(addr);
	ret = dev_data->ts_platform_data->bops->bus_read((u8 *)&addr, 2, buffer, len);
	if (ret < 0)
		TS_LOG_ERR("%s:i2c read error,addr:%04x bytes:%d\n", __func__, addr, len);

	return ret;
}

/**
 * gt1x_i2c_read_dbl_check - read twice and double check
 * @addr: register address
 * @buffer: data buffer
 * @len: bytes to read
 * Return    <0: i2c error, 0: ok, 1:fail
 */
int gt1x_i2c_read_dbl_check(u16 addr, u8 * buffer, s32 len)
{
	u8 buf[16] = {0};
	u8 confirm_buf[16] = {0};
	int ret = 0;

	if (!buffer) {
		TS_LOG_ERR("%s:buffer is null\n", __func__);
		return -EINVAL;
	}

	if (len > 16) {
		TS_LOG_ERR("%s:i2c_read_dbl_check length %d is too long, exceed %zu\n",
			__func__, len, sizeof(buf));
		return -EINVAL;
	}

	ret = gt1x_i2c_read(addr, buf, len);
	if (ret < 0){
		TS_LOG_ERR("%s:i2c_read failed\n", __func__);
		return ret;
	}

	msleep(5);
	ret = gt1x_i2c_read(addr, confirm_buf, len);
	if (ret < 0){
		TS_LOG_ERR("%s:i2c_read failed\n", __func__);
		return ret;
	}

	if (!memcmp(buf, confirm_buf, len)) {
		memcpy(buffer, confirm_buf, len);
		return NO_ERR;
	}

	ret = 1;
	TS_LOG_ERR("%s:i2c read 0x%04X,%d bytes, double check failed\n", __func__, addr, len);
	return ret;
}

/**
 * gt1x_send_cfg - Send config data to hw
 * @cfg_ptr: pointer to config data structure
 * Return 0--success,non-0--fail.
 */
static int gt1x_send_cfg(struct gt1x_ts_config *cfg_ptr)
{
	u8 *config = NULL;
	int i=0, cfg_len=0;
	s32 ret = 0, retry = 0;
	u16 checksum = 0;
	int total_len = 0;
	static DEFINE_MUTEX(mutex_cfg);

	if (!cfg_ptr || !cfg_ptr->initialized) {
		TS_LOG_ERR("%s:Invalid config data\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&mutex_cfg);
	config = &cfg_ptr->data[0];
	cfg_len = cfg_ptr->size;

	TS_LOG_INFO("%s:Send %s,ver:%d size:%d\n", __func__,
			cfg_ptr->name, config[0], cfg_len);

	/* Extends config */
	if (config[EXTERN_CFG_OFFSET] & 0x40) {
		total_len = GTP_CONFIG_LENGTH;
		config[GTP_CONFIG_ORG_LENGTH] |= 0x01;

		for (i = GTP_CONFIG_ORG_LENGTH; i < total_len-2; i += 2)
			checksum += (config[i] << 8) + config[i + 1];
		if (!checksum){
			TS_LOG_ERR("%s:Invalid ext config,all of the bytes is zero\n", __func__);
			mutex_unlock(&mutex_cfg);
			return -EINVAL;
		}

		checksum = 0 - checksum;
		config[total_len - 2] = (checksum >> 8) & 0xFF;
		config[total_len - 1] = checksum & 0xFF;

		do {
			ret = gt1x_i2c_write(GTP_REG_EXT_CONFIG,
				&config[GTP_CONFIG_ORG_LENGTH], GTP_CONFIG_EXT_LENGTH);
		} while (ret < 0 && retry++ < GT1X_RETRY_NUM);

		cfg_len -= GTP_CONFIG_EXT_LENGTH;
	}

	for (i = 0, checksum = 0; i < cfg_len - 3; i += 2)
		checksum += (config[i] << 8) + config[i + 1];

	if (!checksum) {
		TS_LOG_ERR("%s:Invalid config,all of the bytes is zero\n", __func__);
		mutex_unlock(&mutex_cfg);
		return -EINVAL;
	}

	checksum = 0 - checksum;
	config[cfg_len - 3] = (checksum >> 8) & 0xFF;
	config[cfg_len - 2] = checksum & 0xFF;
	config[cfg_len - 1] = 0x01;

	retry = 0;
	while (retry++ < GT1X_RETRY_NUM) {
		ret = gt1x_i2c_write(GTP_REG_CONFIG_DATA, config, cfg_len);
		if (!ret) {
			if (config[6] & 0x80) /* judge weather store the config to Flash */
				msleep(SEND_CFG_RAM);
			else
				msleep(SEND_CFG_FLASH);
			mutex_unlock(&mutex_cfg);
			TS_LOG_INFO("%s:Send config successfully\n", __func__);
			return NO_ERR;
		}
	}

	mutex_unlock(&mutex_cfg);
	TS_LOG_ERR("%s:Send config failed\n", __func__);
	return -EINVAL;
}

/**
 * gt1x_send_cmd - seng cmd
 * must write data & checksum first
 * byte    content
 * 0       cmd
 * 1       data
 * 2       checksum
 * Returns 0 - succeed,non-0 - failed
 */
static int gt1x_send_cmd(u8 cmd, u8 data, u8 issleep)
{
	s32 ret = 0;
	static DEFINE_MUTEX(cmd_mutex);
	u8 buffer[3] = { cmd, data, 0 };

	TS_LOG_DEBUG("%s: Send command:%u\n", __func__, cmd);
	mutex_lock(&cmd_mutex);
	buffer[2] = (u8) ((0 - cmd - data) & 0xFF);
	ret = gt1x_i2c_write(GTP_REG_CMD + 1, &buffer[1], 2);
	ret |= gt1x_i2c_write(GTP_REG_CMD, &buffer[0], 1);
	if(ret < 0){
		TS_LOG_ERR("%s: i2c_write command fail\n", __func__);
	}

	if(issleep) {
		msleep(50);
	}
	mutex_unlock(&cmd_mutex);

	return ret;
}

/**
 * gt1x_init_watchdog - esd mechannism
 *
 * Returns  0--success,non-0--fail.
 */
static inline int gt1x_init_watchdog(void)
{
	/* 0x8040 ~ 0x8043 */
	u8 value[I2C_WD_DATA_LENGTH] = {0xAA, 0x00, 0x56, 0xAA};
	int ret =0;
	int i = 0;
	TS_LOG_DEBUG("%s:Init watchdog\n", __func__);
	for (i = 0; i < GT1X_RETRY_NUM; i++)
	{
		ret = gt1x_i2c_write(GTP_REG_CMD + INDEX_1, &value[INDEX_1], (I2C_WD_DATA_LENGTH - I2C_ENABLE_BIT));
		if(ret < NO_ERR)
		{
			TS_LOG_ERR("%s:Init watchdog error ,i2c communicate error , retry  = [%d] \n", __func__, i);
			msleep(I2C_RETRY_SLEEP_TIME);
		}
		else
		{
			TS_LOG_INFO("%s, Init watchdog success ,i2c communicate check success\n", __func__);
			return gt1x_i2c_write(GTP_REG_CMD, &value[INDEX_0], I2C_ENABLE_BIT);
		}
	}
	TS_LOG_DEBUG("%s:Init watchdog end \n", __func__);
	return ret;
}

/**
 * gt1x_switch_wrokmode - Switch working mode.
 * @workmode: GTP_CMD_SLEEP - Sleep mode
 *			  GESTURE_MODE - gesture mode
 * Returns  0--success,non-0--fail.
 */
static int gt1x_switch_wrokmode(int wrokmode)
{
	s32 retry = 0;
	u8 cmd = 0;

	switch (wrokmode) {
	case SLEEP_MODE:
		cmd = GTP_CMD_SLEEP;
		break;
	case GESTURE_MODE:
		cmd = GTP_CMD_GESTURE_WAKEUP;
		break;
	default:
		TS_LOG_ERR("%s: no supported workmode\n", __func__);
		return -EINVAL;
	}

	TS_LOG_DEBUG("%s: Switch working mode[%02X]\n", __func__, cmd);
	while (retry++ < GT1X_RETRY_NUM) {
		if (!gt1x_send_cmd(cmd, 0, GT1X_NOT_NEED_SLEEP))
			return NO_ERR;

		TS_LOG_ERR("%s: send_cmd failed, retry = %d\n", __func__, retry);
	}

	TS_LOG_ERR("%s: Failed to switch working mode\n", __func__);
	return -EINVAL;
}

/**
 * gt1x_feature_switch - Switch touch feature.
 * @ts: gt1x ts data
 * @fea: gt1x touch feature
 * @on: SWITCH_ON, SWITCH_OFF
 * Returns  0--success,non-0--fail.
 */
static int gt1x_feature_switch(struct gt1x_ts_data *ts,
		enum gt1x_ts_feature fea, int on)
{
	struct gt1x_ts_config *config = NULL;
	int ret = 0;

	if (!ts ){
		TS_LOG_ERR("%s:invalid param\n", __func__);
		return -EINVAL;
	}

	struct ts_feature_info *info = &ts->dev_data->ts_platform_data->feature_info;

	if (!info){
		TS_LOG_ERR("%s:invalid param\n", __func__);
		return -EINVAL;
	}

	if (on == SWITCH_ON) {
		switch (fea) {
		case TS_FEATURE_NONE:
			config = &ts->normal_config;
			break;
		case TS_FEATURE_GLOVE:
			config = &ts->glove_config;
			break;
		case TS_FEATURE_HOLSTER:
			config = &ts->holster_config;
			break;
		default:
			TS_LOG_ERR("%s:invalid feature type\n", __func__);
			return -EINVAL;
		}
	} else if (on == SWITCH_OFF) {
		if (info->holster_info.holster_switch)
			config = &ts->holster_config;
		else if (info->glove_info.glove_switch)
			config = &ts->glove_config;
		else
			config = &ts->normal_config;
	}else{
		TS_LOG_ERR("%s:invalid switch status\n", __func__);
		return -EINVAL;
	}

	ts->noise_env = false;
	ret = gt1x_send_cfg(config);
	if(ret)
		TS_LOG_ERR("%s:send_cfg failed\n", __func__);
	return ret;
}

/**
 * gt1x_feature_resume - firmware feature resume
 * @ts: pointer to gt1x_ts_data
 * return 0 ok, others error
 */
static int gt1x_feature_resume(struct gt1x_ts_data *ts)
{
	struct ts_feature_info *info = &ts->dev_data->ts_platform_data->feature_info;
	struct gt1x_ts_config *config = NULL;
	int ret = 0;

	TS_LOG_INFO("holster_supported:%d,holster_switch_staus:%d,glove_supported:%d,"
		"glove_switch_status:%d,charger_supported:%d,charger_switch_status:%d\n",
		info->holster_info.holster_supported, info->holster_info.holster_switch,
		info->glove_info.glove_supported, info->glove_info.glove_switch,
		info->charger_info.charger_supported, info->charger_info.charger_switch);

	if (info->holster_info.holster_supported
		&& info->holster_info.holster_switch){
		config = &ts->holster_config;
	}else if (info->glove_info.glove_supported
		&& info->glove_info.glove_switch) {
		if (ts->noise_env)
			config = &ts->glove_noise_config;
		else
			config = &ts->glove_config;
	} else {
		if (ts->noise_env)
			config = &ts->normal_noise_config;
		else
			config = &ts->normal_config;
	}
	if (config) {
		ret = gt1x_send_cfg(config);
		if(ret < 0)
			TS_LOG_ERR("%s: send_cfg fail\n", __func__);
	}else
		TS_LOG_ERR("%s: config parm is null!\n", __func__);

	if (info->charger_info.charger_supported
		&& info->charger_info.charger_switch) {
		ret = gt1x_send_cmd(GTP_CMD_CHARGER_ON, 0x00, GT1X_NOT_NEED_SLEEP);
		if(ret)
			TS_LOG_ERR("%s: send_cmd failed\n", __func__);
	}

	return ret;
}

/**
 * gt1x_noise_ctrl - noise control
 *  switch special config
 * @ts: pointer to gt1x_ts_data
 * @on: turn on of controling of noise
 * return 0 ok, others error
 */
static int  gt1x_noise_ctrl(struct gt1x_ts_data *ts, bool on)
{
	struct ts_feature_info *info = &ts->dev_data->ts_platform_data->feature_info;
	struct gt1x_ts_config *config = NULL;
	int ret = 0;

	TS_LOG_DEBUG("%s: Noise ctrl:%d\n", __func__, on);
	if (info->holster_info.holster_supported
		&& info->holster_info.holster_switch){
		/* reserve */
	} else if (info->glove_info.glove_supported
			&& info->glove_info.glove_switch) {
		if (on)
			config = &ts->glove_noise_config;
		else
			config = &ts->glove_config;
	} else {
		if (on)
			config = &ts->normal_noise_config;
		else
			config = &ts->normal_config;
	}

	if(config){
		ret = gt1x_send_cfg(config);
		if(ret < 0){
			TS_LOG_ERR("%s: send_cfg fail\n", __func__);
		}
	}else{
		TS_LOG_ERR("%s: config parm is null!\n", __func__);
	}

	if (info->charger_info.charger_supported
			&& info->charger_info.charger_switch) {
		ret = gt1x_send_cmd(GTP_CMD_CHARGER_ON, 0x00, GT1X_NOT_NEED_SLEEP);
		TS_LOG_INFO("%s: Charger switch on\n", __func__);
	}
	return ret;
}


/**
 * gt1x_ts_roi_init - initialize roi feature
 * @roi: roi data structure
 * return 0 OK, < 0 fail
 */
static int gt1x_ts_roi_init(struct gt1x_ts_roi *roi)
{
	unsigned int roi_bytes;
	struct gt1x_ts_data *ts = gt1x_ts;

	if (!ts->dev_data->ts_platform_data->feature_info.roi_info.roi_supported
			|| !roi ){
		TS_LOG_ERR("%s: roi is not support or invalid parameter\n", __func__);
		return -EINVAL;
	}

	if (!roi->roi_rows || !roi->roi_cols) {
		TS_LOG_ERR("%s: Invalid roi config,rows:%d,cols:%d\n",
				__func__, roi->roi_rows, roi->roi_cols);
		return -EINVAL;
	}

	mutex_init(&roi->mutex);

	roi_bytes = (roi->roi_rows * roi->roi_cols + 1) * sizeof(*roi->rawdata);
	roi->rawdata = kzalloc(roi_bytes + ROI_HEAD_LEN, GFP_KERNEL);
	if (!roi->rawdata) {
		TS_LOG_ERR("%s: Failed to alloc memory for roi\n", __func__);
		return -ENOMEM;
	}

	TS_LOG_INFO("%s: ROI init,rows:%d,cols:%d\n",
				__func__, roi->roi_rows, roi->roi_cols);

	return NO_ERR;
}

static int gt1x_get_standard_matrix(struct roi_matrix *src_mat, struct roi_matrix *dst_mat)
{
	int i, j;
	short *temp_buf;
	short diff_max = 0x8000;
	int peak_row = 0, peak_col = 0;
	int src_row_start = 0, src_row_end = 0;
	int src_col_start = 0, src_col_end = 0;
	int dst_row_start = 0, dst_col_start = 0;

    /* transport src_mat matrix */
	temp_buf = kzalloc((src_mat->row * src_mat->col + 1) * sizeof(*temp_buf), GFP_KERNEL);
	if (!temp_buf) {
		TS_LOG_ERR("Failed transport the ROI matrix\n");
		return -EINVAL;
	}

	for (i = 0; i < src_mat->row; i++) {
		for (j = 0; j < src_mat->col; j++) {
			temp_buf[i + j * src_mat->row] = src_mat->data[i * src_mat->col + j];
		}
	}

	i = src_mat->row;
	src_mat->row = src_mat->col;
	src_mat->col = i;
	memcpy(src_mat->data, temp_buf, src_mat->row * src_mat->col * sizeof(*temp_buf));
	kfree(temp_buf);
	temp_buf = NULL;

	/* get peak value postion */
	for (i = 0; i< src_mat->row; i++) {
		for (j = 0; j < src_mat->col; j++) {
			if (src_mat->data[i * src_mat->col + j] > diff_max) {
				diff_max = src_mat->data[i * src_mat->col + j];
				peak_row = i;
				peak_col = j;
			}
		}
	}
	TS_LOG_DEBUG("DEBUG Peak pos[%d][%d] = %d\n", peak_row, peak_col, diff_max);
	src_row_start = 0;
	dst_row_start = ((dst_mat->row - 1) >> 1) - peak_row;
	if (peak_row >= ((dst_mat->row - 1) >> 1)) {
		src_row_start = peak_row - ((dst_mat->row - 1) >> 1);
		dst_row_start = 0;
	}

	src_row_end = src_mat->row -1;
	if (peak_row <= (src_mat->row - 1 - (dst_mat->row >> 1))) {
		src_row_end = peak_row + (dst_mat->row >> 1);
	}

	src_col_start = 0;
	dst_col_start = ((dst_mat->col -1) >> 1) - peak_col;
	if (peak_col >= ((dst_mat->col -1) >> 1)) {
		src_col_start = peak_col - ((dst_mat->col -1) >> 1);
		dst_col_start = 0;
	}

	src_col_end = src_mat->col -1;
	if (peak_col <= (src_mat->col -1 - (dst_mat->col >> 1))) {
		src_col_end = peak_col + (dst_mat->col >> 1);
	}

	/* copy peak value area to the center of ROI matrix */
	memset(dst_mat->data, 0, sizeof(short) * dst_mat->row * dst_mat->col);
	for (i = src_row_start; i <= src_row_end; i++) {
		memcpy(&dst_mat->data[dst_col_start + dst_mat->col * (dst_row_start + i - src_row_start)],
			&src_mat->data[i * src_mat->col + src_col_start],
			(src_col_end -src_col_start + 1) * sizeof(short));
	}
	return 0;
}

/**
 * gt1x_cache_roidata_device - caching roi data
 * @roi: roi data structure
 * return 0 ok, < 0 fail
 */
static int gt1x_cache_roidata_device(struct gt1x_ts_roi *roi)
{
	unsigned char status[ROI_HEAD_LEN] = {0};
	struct roi_matrix src_mat, dst_mat;
	u16 checksum = 0;
	int i = 0, ret = 0, res_write = 0;
	struct gt1x_ts_data *ts = gt1x_ts;
	memset(&src_mat, 0, sizeof(src_mat));
	memset(&dst_mat, 0, sizeof(dst_mat));

	if (!ts->dev_data->ts_platform_data->feature_info.roi_info.roi_supported
			|| !roi || !roi->enabled || !roi->rawdata) {
		TS_LOG_ERR("%s:roi is not support or invalid parameter\n", __func__);
		return -EINVAL;
	}

	roi->data_ready = false;

	ret = gt1x_i2c_read(ts->gt1x_roi_data_add, status, ROI_HEAD_LEN);
	if (ret) {
		TS_LOG_ERR("Failed read ROI HEAD\n");
		return -EINVAL;
	}
	TS_LOG_DEBUG("ROI head{%02x %02x %02x %02x}\n",
			status[0], status[1], status[2], status[3]);

	for (i = 0; i < ROI_HEAD_LEN; i++) {
		checksum += status[i];
	}

	if (unlikely((u8)checksum != 0)) {
		TS_LOG_ERR("roi status checksum error{%02x %02x %02x %02x}\n",
				status[0], status[1], status[2], status[3]);
		return -EINVAL;
	}

	if (status[0] & ROI_READY_MASK) {
		roi->track_id = status[0] & ROI_TRACKID_MASK;
	} else {
		TS_LOG_ERR("ROI data not ready\n");
		return -EINVAL;
	}

	dst_mat.row = roi->roi_rows;
	dst_mat.col = roi->roi_cols;
	src_mat.row = status[GT1X_ROI_SRC_STATUS_INDEX] & GT1X_BIT_AND_0x0F;
	src_mat.col = status[GT1X_ROI_SRC_STATUS_INDEX] >> 4;

	src_mat.data = kzalloc((src_mat.row * src_mat.col + 1) * sizeof(src_mat.data[0]), GFP_KERNEL);
	if (!src_mat.data) {
		TS_LOG_ERR("failed alloc src_roi memory\n");
		return -ENOMEM;
	}
	dst_mat.data = kzalloc((dst_mat.row * dst_mat.col + 1) * sizeof(dst_mat.data[0]), GFP_KERNEL);
	if (!dst_mat.data) {
		TS_LOG_ERR("failed alloc dst_roi memory\n");
		kfree(src_mat.data);
		src_mat.data = NULL;
		return -ENOMEM;
	}

	/* read ROI rawdata */
	ret = gt1x_i2c_read((ts->gt1x_roi_data_add + ROI_HEAD_LEN),
						(char*)src_mat.data,
						(src_mat.row * src_mat.col + 1) * sizeof(src_mat.data[0]));
	if (ret) {
		TS_LOG_ERR("Failed read ROI rawdata\n");
		ret = -EINVAL;
		goto err_out;
	}

	for (i = 0, checksum = 0; i < src_mat.row * src_mat.col + 1; i++) {
		src_mat.data[i] = be16_to_cpu(src_mat.data[i]);
		checksum += src_mat.data[i];
	}
	if (checksum) {
		TS_LOG_ERR("ROI rawdata checksum error\n");
		goto err_out;
	}

	if (gt1x_get_standard_matrix(&src_mat, &dst_mat)) {
		TS_LOG_ERR("Failed get standard matrix\n");
		goto err_out;
	}

	mutex_lock(&roi->mutex);
	memcpy(&roi->rawdata[0], &status[0], ROI_HEAD_LEN);
	memcpy(((char*)roi->rawdata) + ROI_HEAD_LEN, &dst_mat.data[0],
		   roi->roi_rows * roi->roi_cols * sizeof(*roi->rawdata));

	roi->data_ready = true;
	mutex_unlock(&roi->mutex);
	ret = 0;
err_out:
	kfree(src_mat.data);
	src_mat.data = NULL;
	kfree(dst_mat.data);
	dst_mat.data = NULL;

	/* clear the roi state flag */
	status[0] = 0x00;
	res_write = gt1x_i2c_write(ts->gt1x_roi_data_add, status, 1);
	if(res_write) {
		TS_LOG_ERR("%s:clear ROI status failed [%d]\n", __func__, res_write);
	}

	return ret;
}

/**
 * gt1x_cache_roidata_fw - caching roi data
 * @roi: roi data structure
 * return 0 ok, < 0 fail
 */
static int gt1x_cache_roidata_fw(struct gt1x_ts_roi *roi)
{
	unsigned int roi_bytes = 0;
	u8 status[ROI_HEAD_LEN + 1] = {0};
	u16 checksum = 0;
	int i = 0, ret = 0;
	struct gt1x_ts_data *ts = gt1x_ts;

	if (!ts->dev_data->ts_platform_data->feature_info.roi_info.roi_supported
			|| !roi || !roi->enabled || !roi->rawdata){
		TS_LOG_ERR("%s:roi is not support or invalid parameter\n", __func__);
		return -EINVAL;
	}

	ret = gt1x_i2c_read(ROI_STA_REG, status, ROI_HEAD_LEN);
	if (unlikely(ret < 0)){
		return ret;
	}

	for (i = 0; i < ROI_HEAD_LEN; i++)
		checksum += status[i];

	if (unlikely((u8)checksum != 0)) { /* cast to 8bit checksum,*/
		TS_LOG_ERR("%s:roi status checksum error{%02x %02x %02x %02x}\n",
				__func__, status[0], status[1], status[2], status[3]);
		return -EINVAL;
	}

	if (likely(status[0] & ROI_READY_MASK)) /* roi data ready */
		roi->track_id = status[0] & ROI_TRACKID_MASK;
	else{
		TS_LOG_ERR("%s:not read\n", __func__);
		return -EINVAL; /* not ready */
	}

	mutex_lock(&roi->mutex);
	roi->data_ready = false;
	roi_bytes = (roi->roi_rows * roi->roi_cols + 1) * 2;

	ret = gt1x_i2c_read(ROI_DATA_REG,
				(u8 *)(roi->rawdata) + ROI_HEAD_LEN, roi_bytes);
	if (unlikely(ret < 0)) {
		mutex_unlock(&roi->mutex);
		TS_LOG_ERR("%s:i2c_read failed!\n", __func__);
		return ret;
	}

	for (i = 0, checksum = 0; i < roi_bytes / 2 ; i++) {
		roi->rawdata[i + ROI_HEAD_LEN / 2] =
			be16_to_cpu(roi->rawdata[i + ROI_HEAD_LEN / 2]);
		checksum += roi->rawdata[i + ROI_HEAD_LEN / 2];
	}
	memcpy(&roi->rawdata[0], &status[0], ROI_HEAD_LEN);

	if (unlikely(checksum != 0))
		TS_LOG_ERR("%s:roi data checksum error\n", __func__);
	else
		roi->data_ready = true;

	mutex_unlock(&roi->mutex);

	status[0] = 0x00;
	ret = gt1x_i2c_write(ROI_STA_REG, status, 1);

	return ret;
}

/**
 * gt1x_request_evt_handler - firmware request
 * Return    <0: failed, 0: succeed
 */
static int gt1x_request_evt_handler(struct gt1x_ts_data *ts)
{
	u8 rqst_data = 0;
	int ret = 0;

	ret = gt1x_i2c_read(GTP_REG_RQST, &rqst_data, 1);
	if (ret){
		return ret;
	}

	TS_LOG_DEBUG("%s: Request state:0x%02x\n", __func__, rqst_data);
	switch (rqst_data) {
	case GTP_RQST_CONFIG:
		TS_LOG_INFO("%s: Request Config.\n", __func__);
		ret = gt1x_send_cfg(&ts->normal_config);
		if (ret) {
			TS_LOG_ERR("%s: Send config error\n", __func__);
		} else {
			TS_LOG_INFO("%s: Send config success\n", __func__);
			rqst_data = GTP_RQST_RESPONDED;
			gt1x_i2c_write(GTP_REG_RQST, &rqst_data, 1);
		}
		break;
	case GTP_RQST_RESET:
		TS_LOG_INFO("%s: Request Reset.\n", __func__);
		gt1x_i2c_read(0x5097, &rqst_data, 1);
		TS_LOG_INFO("%s: Reason code[0x5097]:0x%02x\n", __func__, rqst_data);
		ret = gt1x_chip_reset();
		if(ret < 0){
			TS_LOG_ERR("%s, failed to chip_reset, ret = %d\n", __func__, ret);
		}
		gt1x_feature_resume(ts);
		rqst_data = GTP_RQST_RESPONDED;
		gt1x_i2c_write(GTP_REG_RQST, &rqst_data, 1);
		break;
	case GTP_RQST_NOISE_CFG:
		TS_LOG_INFO("%s:Request noise config\n", __func__);
		ret = gt1x_noise_ctrl(ts, true);
		if (!ret)
			ts->noise_env = true;
		rqst_data = GTP_RQST_IDLE;
		gt1x_i2c_write(GTP_REG_RQST, &rqst_data, 1);
		break;
	case GTP_RQST_NORMA_CFG:
		TS_LOG_INFO("%s:Request non-noise config\n", __func__);
		ret = gt1x_noise_ctrl(ts, false);
		if (!ret)
			ts->noise_env = false;
		rqst_data = GTP_RQST_IDLE;
		gt1x_i2c_write(GTP_REG_RQST, &rqst_data, 1);
		break;
	case GTP_RQST_IDLE:
		TS_LOG_INFO("[%s] Warning : An empty request, without any processing!!\n", __func__);
		break;
	default:
		TS_LOG_INFO("[%s] Warning : An undefined request[0x%02x] , without any processing!!\n",
			__func__, rqst_data);
		break;
	}

	return NO_ERR;
}

/**
 * gt1x_touch_evt_handler - handle touch event
 * (pen event, key event, finger touch envent)
 * Return    <0: failed, 0: succeed
 */
static int gt1x_touch_evt_handler(struct gt1x_ts_data  *ts,
				struct ts_fingers *info)
{
	u8 touch_data[1 + 8 * GTP_MAX_TOUCH + 2] = {0};
	cur_index = 0;
	u8 touch_num;
	u8 *coor_data = NULL;
	u8 check_sum = 0;
	int id = 0, x = 0, y = 0, w = 0, i = 0;
	int xer = 0;
	int yer = 0;
	int ret = -1;
	int res = 0;
	u8 touch_ewxy[GT1X_EDGE_DATA_SIZE] = {0};
	u8 touch_wxy[GT1X_EDGE_DATA_SIZE] = {0};

	ret = gt1x_i2c_read(GTP_READ_COOR_ADDR, &touch_data[0], 1 + 8 + 2);
	if (unlikely(ret))
		goto exit;

	if (unlikely(!touch_data[0])) {
		/* hw request */
		gt1x_request_evt_handler(ts);
		ret = EINVAL;/*fw request event, non continue process*/
		goto exit;
	}

	touch_num = touch_data[0] & 0x0f;
	if (unlikely(touch_num > GTP_MAX_TOUCH)) {
		TS_LOG_ERR("%s: Illegal finger number!\n", __func__);
		ret = -EINVAL;
		goto exit;
	}

	if ((ts->gt1x_edge_add != 0) && (touch_num > 0)) {
		ret = ts->ops.i2c_read(ts->gt1x_edge_add,touch_ewxy,GT1X_EDGE_DATA_SIZE);
		if (ret) {
			goto exit;
		}
	}
	if((touch_num > 0) && (0 != ts->gt1x_support_wxy) && (0 != ts->gt1x_wxy_data_add)) {
		ret = ts->ops.i2c_read(ts->gt1x_wxy_data_add, touch_wxy, GT1X_EDGE_DATA_SIZE);
		if (ret) {
			goto exit;
		}
	}

	/* read the remaining coor data 
		* 0x814E(touch status) + 
		* 8(every coordinate consist of 8 bytes data) * touch num + 
		* keycode + checksum
		*/
	if (touch_num > 1) {
		ret = gt1x_i2c_read((GTP_READ_COOR_ADDR + 11),
					&touch_data[11], (touch_num - 1) * 8);
		if (ret)
			goto exit;
	}

	/* calc checksum */
	for (i = 0; i < 1 + 8 * touch_num + 2; i++)
		check_sum += touch_data[i];

	if (unlikely(check_sum)) { /* checksum error*/
		ret = gt1x_i2c_read(GTP_READ_COOR_ADDR, touch_data,
					3 + 8 * touch_num);
		if (ret)
			goto exit;

		for (i = 0, check_sum = 0; i < 3 + 8 * touch_num; i++)
			check_sum += touch_data[i];

		if (check_sum) {
			TS_LOG_ERR("%s: Checksum error[%x]\n", __func__, check_sum);
			for (i = 0; i < 3 + 8 * touch_num; i++)
				TS_LOG_ERR("coordata [%d]:%02x\n", i, touch_data[i]);

			ret = -EINVAL;
			goto exit;
		}
	}
	if (touch_data[0] & GT1X_BIT_AND_0x40) {
		TS_LOG_INFO("%s: Large Touch!\n", __func__);
	}

	memset(info, 0x00, sizeof(struct ts_fingers));
	coor_data = &touch_data[1];

	for (i = 0; i < touch_num; i++) {
		id = coor_data[i * 8] & 0x7F;
		x = le16_to_cpup((__le16 *)&coor_data[i * 8 + 1]);
		y = le16_to_cpup((__le16 *)&coor_data[i * 8 + 3]);

		if (ts->gt1x_edge_support_xyer) {
			w = coor_data[i * GT1X_EDGE_POINT_DATA_SIZE + GT1X_ADDR_EDGE_W];
			xer = coor_data[i * GT1X_EDGE_POINT_DATA_SIZE + GT1X_ADDR_EDGE_XER];
			yer = coor_data[i * GT1X_EDGE_POINT_DATA_SIZE + GT1X_ADDR_EDGE_YER];
			info->fingers[id].xer = xer;
			info->fingers[id].yer = yer;
		} else {
			w = le16_to_cpup((__le16 *)&coor_data[i * 8 + 5]);
		}

		if (unlikely(ts->flip_x))
			info->fingers[id].x = ts->max_x - x;
		else
			info->fingers[id].x = x;

		if (unlikely(ts->flip_y))
			info->fingers[id].y = ts->max_y - y;
		else
			info->fingers[id].y = y;

		info->fingers[id].ewx = touch_ewxy[2 * id];
		info->fingers[id].ewy = touch_ewxy[2 * id + 1];
		if(ts->gt1x_support_wxy) {
			info->fingers[id].wx = touch_wxy[2 * id];
			info->fingers[id].wy = touch_wxy[2 * id + 1];
		}
		info->fingers[id].pressure = w;
		info->fingers[id].status = TP_FINGER;
		cur_index |= 1 << id;
		TS_LOG_DEBUG("%s:[%d](ewx = %d, ewy =  %d, xer=%d, yer=%d, wx=%d, wy=%d)\n", __func__,
			id, info->fingers[id].ewx, info->fingers[id].ewy, xer, yer,touch_wxy[2 * id], touch_wxy[2 * id + 1]);
	}
	info->cur_finger_number = touch_num;
exit:
	return ret;
}

#define GT1X_GESTURE_PACKAGE_LEN 4
#define GT1X_INVALID_GESTURE_TYPE 0
static void gt1x_set_gesture_key(struct ts_fingers *info, struct ts_cmd_node *out_cmd)
{
	struct gt1x_ts_data *ts = gt1x_ts;

	if(true == ts->dev_data->easy_wakeup_info.off_motion_on){
		ts->dev_data->easy_wakeup_info.off_motion_on = false;
	}

	info->gesture_wakeup_value = TS_DOUBLE_CLICK;
	out_cmd->command = TS_INPUT_ALGO;
	out_cmd->cmd_param.pub_params.algo_param.algo_order = ts->dev_data->algo_id;
	TS_LOG_DEBUG("%s: ts_double_click evevnt report\n", __func__);

	return;
}
static void gt1x_double_tap_event(struct gt1x_ts_data *ts, struct ts_fingers *info, struct ts_cmd_node *out_cmd, u8 num)
{
	u8 buf[GT1X_DOUBLE_TAB_POINT_NUM * GT1X_GESTURE_ONE_POINT_BYTES] = {0};
	int i = 0, ret = 0, x = 0, y = 0;
	u16 reg_add = 0;

	if(GT1X_DOUBLE_TAB_POINT_NUM != num){
		TS_LOG_ERR("%s: points number is mismatch\n", __func__);
		return;
	}

	if (IC_TYPE_9P == ts->ic_type)
		reg_add = GTP_REG_GESTURE_POSITION_9P;
	else
		reg_add = GTP_REG_GESTURE_POSITION_9PT;
	/*double click position*/
	ret = gt1x_i2c_read(reg_add, buf, GT1X_DOUBLE_TAB_POINT_NUM * GT1X_GESTURE_ONE_POINT_BYTES);
	if (ret < 0){
		TS_LOG_ERR("%s: get_reg_wakeup_gesture_position faile\n", __func__);
		return;
	}

	memset(ts->dev_data->ts_platform_data->chip_data->easy_wakeup_info.easywake_position, 
			0, MAX_POSITON_NUMS);
	for (i = 0; i < GT1X_DOUBLE_TAB_POINT_NUM; i++) {
		x = -1;
		y = -1;

		/*
		*buf[0] : x coordinate low byte
		*buf[1] : x coordinate hight byte
		*buf[2] : y coordinate low byte
		*buf[3] : y coordinate hight byte
		*/
		x = get_unaligned_le16(&buf[i * 4]);
		y = get_unaligned_le16(&buf[ i *4 + 2]);
		ts->dev_data->ts_platform_data->chip_data->easy_wakeup_info.easywake_position[i] = 
				x << 16 |y;
	}

	mutex_lock(&wrong_touch_lock);
	gt1x_set_gesture_key(info, out_cmd);
	mutex_unlock(&wrong_touch_lock);

	return;
}

static int gt1x_gesture_evt_handler(struct gt1x_ts_data *ts,
				struct ts_fingers *info, struct ts_cmd_node *out_cmd)
{
	u8 buf[GT1X_GESTURE_PACKAGE_LEN] = {0}, gesture_type =0;
	u8 points_num = 0;
	int ret = 0;
	unsigned char ts_state = 0;

	ts_state = atomic_read(&ts->dev_data->ts_platform_data->state);
	TS_LOG_DEBUG("%s: TS_WORK_STATUS :%d\n", __func__, ts_state);
	if(TS_WORK_IN_SLEEP != ts_state){
		TS_LOG_DEBUG("%s: TS_WORK_STATUS[%d] is mismatch.\n", __func__, ts_state);
		return true;
	}

	if(false == ts->dev_data->easy_wakeup_info.off_motion_on){
		TS_LOG_INFO("%s: easy_wakeup no need report!!\n", __func__);
		goto clear_flag;
	}

	/** package: -head 4B + track points + extra info- 
		* - head -
		*  buf[0]: gesture type, 
		*  buf[1]: number of gesture points
		*/
	ret = gt1x_i2c_read(GTP_REG_WAKEUP_GESTURE, buf, GT1X_GESTURE_PACKAGE_LEN);
	if (ret < 0){
		TS_LOG_ERR("%s: get_reg_wakeup_gesture faile\n", __func__);
		goto clear_flag;
	}

	TS_LOG_DEBUG("0x%x = 0x%02X,0x%02X,0x%02X,0x%02X\n", GTP_REG_WAKEUP_GESTURE,
		buf[0], buf[1], buf[2], buf[3]);
	gesture_type = buf[0];
	points_num = buf[1];

	switch (gesture_type) {
		case GT1X_INVALID_GESTURE_TYPE:
			TS_LOG_ERR("%s: Gesture[0x%02X] has been disabled !!\n", __func__,gesture_type);
			break;
		case GT1X_DOUBLE_TAB_TYPE: //double tap
			if (ts->dev_data->easy_wakeup_info.easy_wakeup_gesture & GT1X_DOUBLE_TAB_FLAG_BIT){
				gt1x_double_tap_event(ts, info, out_cmd, points_num);
			}
			break;
		default:
			TS_LOG_ERR("%s: Warning : no support wakeup_event class !!\n", __func__);
			break;
	}

clear_flag:
	buf[0] = 0; // clear gesture flag
	gt1x_i2c_write(GTP_REG_WAKEUP_GESTURE, buf, 1);
	TS_LOG_INFO("%s:easy_wakeup_gesture_event report finish!\n", __func__);
	return NO_ERR;
}

/**
 * gt1x_irq_bottom_half - gt1x touchscreen work function.
 */
static int gt1x_irq_bottom_half(struct ts_cmd_node *in_cmd,
				struct ts_cmd_node *out_cmd)
{
	struct gt1x_ts_data *ts = gt1x_ts;
	struct ts_fingers *ts_fingers = NULL;
	u8 sync_val = 0;
	int ret = 0;

	ts_fingers = &out_cmd->cmd_param.pub_params.algo_param.info;
	out_cmd->command = TS_INVAILD_CMD;
	out_cmd->cmd_param.pub_params.algo_param.algo_order =
			ts->dev_data->algo_id;

	TS_LOG_DEBUG("%s: wakeup_gesture_enable:%u,sleep_mode:%d,ts_kit_gesture_func:%d,easy_wakeup_gesture:%d\n",
	 __func__, ts->easy_wakeup_supported,ts->dev_data->easy_wakeup_info.sleep_mode,
	 ts_kit_gesture_func,ts->dev_data->easy_wakeup_info.easy_wakeup_gesture);
	if(ts->easy_wakeup_supported && ts_kit_gesture_func
		&& (ts->dev_data->easy_wakeup_info.sleep_mode == TS_GESTURE_MODE)
		&& (ts->dev_data->easy_wakeup_info.easy_wakeup_gesture != 0)) {
		ret = gt1x_gesture_evt_handler(ts, ts_fingers, out_cmd);
		if(!ret){
			return NO_ERR;
		}
	}

	/* handle touch event 
	 * return: <0 - error, 0 - touch event handled,
	 * 			1 - hw request event handledv */
	ret = gt1x_touch_evt_handler(ts, ts_fingers);
	if (ret == 0){
		out_cmd->command = TS_INPUT_ALGO;
	}else if(ret < 0){
		TS_LOG_ERR("%s: touch_evt_handler failed!\n", __func__);
	}

	/*sync_evt:*/
	if (!ts->rawdiff_mode){
		ret = gt1x_i2c_write(GTP_READ_COOR_ADDR, &sync_val, 1);
		if(ret < 0)
			TS_LOG_ERR("%s: clean irq flag failed\n", __func__);
	}else
		TS_LOG_DEBUG("%s: Firmware rawdiff mode\n", __func__);

	return NO_ERR;
}

static int gt1x_i2c_read_hwinfo(void)
{
	u32 hw_info =0;
	int ret = NO_ERR;

	ret = gt1x_i2c_read(GTP_REG_HW_INFO, (u8 *)&hw_info,
				sizeof(hw_info));
	if (ret < NO_ERR) {
		TS_LOG_ERR("%s, Failed to read register\n", __func__);
	} else {
		TS_LOG_INFO("Hardware Info:%08X\n", hw_info);
	}
	return ret;
}

static void gt1x_power_on_gpio_set(void)
{
	struct gt1x_ts_data *ts = gt1x_ts;
	int ret =0;
#ifndef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	gt1x_pinctrl_select_normal();
	gpio_direction_input(ts->dev_data->ts_platform_data->irq_gpio);
	gpio_direction_output(ts->dev_data->ts_platform_data->reset_gpio, 1);
#else
	ret = gt1x_reset_output(1);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 1 fail, ret=%d\n",
				   __func__, ret);
		return;
	}
#endif
}

static void gt1x_power_off_gpio_set(void)
{
	struct gt1x_ts_data *ts = gt1x_ts;
	int ret =0;
#ifndef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	gt1x_pinctrl_select_normal();
	gpio_direction_input(ts->dev_data->ts_platform_data->irq_gpio);
	gpio_direction_output(ts->dev_data->ts_platform_data->reset_gpio, 0);
#else
	ret = gt1x_reset_output(0);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 1 fail, ret=%d\n",
				   __func__, ret);
		return;
	}
#endif
}

/**
 * gt1x_pinctrl_init - pinctrl init
 */
static int gt1x_pinctrl_init(void)
{
	int ret = 0;
	struct gt1x_ts_data *ts = gt1x_ts;

	ts->pinctrl = devm_pinctrl_get(&ts->pdev->dev);
	if (IS_ERR_OR_NULL(ts->pinctrl)) {
		TS_LOG_ERR("%s : Failed to get pinctrl\n", __func__);
		ret = PTR_ERR(ts->pinctrl);
		return ret;
	}
#ifndef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	ts->pins_default = pinctrl_lookup_state(ts->pinctrl, "default");
	if (IS_ERR_OR_NULL(ts->pins_default)) {
		TS_LOG_ERR("%s: Pin state[default] not found\n", __func__);
		ret = PTR_ERR(ts->pins_default);
		goto err_pinctrl_put;
	}

	ts->pins_suspend = pinctrl_lookup_state(ts->pinctrl, "idle");
	if (IS_ERR_OR_NULL(ts->pins_suspend)) {
		TS_LOG_ERR("%s: Pin state[suspend] not found\n", __func__);
		ret = PTR_ERR(ts->pins_suspend);
		goto err_pinctrl_put;
	}
#else
	ts->pinctrl_state_reset_high= pinctrl_lookup_state(ts->pinctrl, PINCTRL_STATE_RESET_HIGH);
	if (IS_ERR_OR_NULL(ts->pinctrl_state_reset_high))
	{
		TS_LOG_ERR("Can not lookup %s pinstate \n",PINCTRL_STATE_RESET_HIGH);
		ret = -EINVAL;
		goto err_pinctrl_put;
	}
	ts->pinctrl_state_reset_low= pinctrl_lookup_state(ts->pinctrl, PINCTRL_STATE_RESET_LOW);
	if (IS_ERR_OR_NULL(ts->pinctrl_state_reset_low))
    {
		TS_LOG_ERR("-Can not lookup %s pinstate \n",PINCTRL_STATE_RESET_LOW);
		ret = -EINVAL;
		goto err_pinctrl_put;
	}
	ts->pinctrl_state_as_int= pinctrl_lookup_state(ts->pinctrl, PINCTRL_STATE_AS_INT);
	if (IS_ERR_OR_NULL(ts->pinctrl_state_as_int))
	{
		TS_LOG_ERR("-Can not lookup %s pinstate \n",PINCTRL_STATE_AS_INT);
		ret = -EINVAL;
		goto err_pinctrl_put;
	}
	ts->pinctrl_state_int_high= pinctrl_lookup_state(ts->pinctrl,PINCTRL_STATE_INT_HIGH);
	if (IS_ERR_OR_NULL(ts->pinctrl_state_int_high))
	{
		TS_LOG_ERR("-Can not lookup %s pinstate \n",PINCTRL_STATE_INT_HIGH);
		ret = -EINVAL;
		goto err_pinctrl_put;
	}
	ts->pinctrl_state_int_low= pinctrl_lookup_state(ts->pinctrl,PINCTRL_STATE_INT_LOW);
	if (IS_ERR_OR_NULL(ts->pinctrl_state_int_low))
	{
		TS_LOG_ERR("-Can not lookup %s pinstate \n", PINCTRL_STATE_INT_LOW);
		ret = -EINVAL;
		goto err_pinctrl_put;
	}

	pinctrl_select_state(ts->pinctrl, ts->pinctrl_state_int_high);
#endif
	return ret;

err_pinctrl_put:
	if(ts->pinctrl){
		devm_pinctrl_put(ts->pinctrl);
		ts->pinctrl = NULL;
		ts->pins_suspend = NULL;
		ts->pins_default = NULL;
	}
	return ret;
}

static void gt1x_pinctrl_release(void)
{
	struct gt1x_ts_data *ts = gt1x_ts;

	if (ts->pinctrl)
		devm_pinctrl_put(ts->pinctrl);
	ts->pinctrl = NULL;
	ts->pins_gesture = NULL;
	ts->pins_suspend = NULL;
	ts->pins_default = NULL;
#if defined (CONFIG_HUAWEI_DEVKIT_MTK_3_0)
	ts->pinctrl_state_reset_high = NULL;
	ts->pinctrl_state_int_high = NULL;
	ts->pinctrl_state_int_low = NULL;
	ts->pinctrl_state_reset_low = NULL;
	ts->pinctrl_state_as_int = NULL;
#endif
}

/**
 * gt1x_pinctrl_select_normal - set normal pin state
 *  Irq pin *must* be set to *pull-up* state.
 */
static void gt1x_pinctrl_select_normal(void)
{
	int ret = 0;
	struct gt1x_ts_data *ts = gt1x_ts;

	if (ts->pinctrl && ts->pins_default) {
		ret = pinctrl_select_state(ts->pinctrl, ts->pins_default);
		if (ret < 0)
			TS_LOG_ERR("%s:Set normal pin state error:%d\n", __func__, ret);
	}

	return;
}

/**
 * gt1x_pinctrl_select_suspend - set suspend pin state
 *  Irq pin *must* be set to *pull-up* state.
 */
static void gt1x_pinctrl_select_suspend(void)
{
	int ret = 0;
	struct gt1x_ts_data *ts = gt1x_ts;

	if (ts->pinctrl && ts->pins_suspend) {
		ret = pinctrl_select_state(ts->pinctrl, ts->pins_suspend);
		if (ret < 0)
			TS_LOG_ERR("%s:Set suspend pin state error:%d\n", __func__, ret);
	}

	return;
}

/**
 * gt1x_pinctrl_select_gesture - set gesture pin state
 *  Irq pin *must* be set to *pull-up* state.
 */
static int gt1x_pinctrl_select_gesture(void)
{
	int ret = 0;
	struct gt1x_ts_data *ts = gt1x_ts;

	if (ts->pinctrl && ts->pins_gesture) {
		ret = pinctrl_select_state(ts->pinctrl, ts->pins_gesture);
		if (ret < 0)
			TS_LOG_ERR("%s:Set gesture pin state error:%d\n", __func__, ret);
	}

	return ret;
}

static unsigned int gt1x_get_chip_id(u8 *chip_name)
{
	int chip_id = 0;

	if(!memcmp(chip_name, GT1X_1151Q_CHIP_ID, GT1X_PRODUCT_ID_LEN)){ /* GT1151Q */
		chip_id = GT_1151Q_ID;
	}else{
		TS_LOG_ERR("%s:invalid chip id failed[%s]!\n", __func__,chip_name);
		chip_id = GT_DEFALT_ID;
	}

	return chip_id;
}

static void gt1x_get_vendor_name(unsigned int vendor_id)
{
	char vendor_name[MAX_STR_LEN] ={0};
	struct gt1x_ts_data *ts = gt1x_ts;

	switch (vendor_id) {
		case MODULE_OFILM_ID:
			strncpy(vendor_name, "ofilm", MAX_STR_LEN);
			break;
		case MODULE_EELY_ID:
			strncpy(vendor_name, "eely", MAX_STR_LEN);
			break;
		case MODULE_TRULY_ID:
			strncpy(vendor_name, "truly", MAX_STR_LEN);
			break;
		default:
			TS_LOG_ERR("%s:user default string!\n", __func__);
			strncpy(vendor_name, "default", MAX_STR_LEN);
	}

	memset(ts->dev_data->module_name, 0, MAX_STR_LEN);
	strncpy(ts->dev_data->module_name, vendor_name, MAX_STR_LEN -1);
}

/**
 * gt1x_creat_project_id - get project_id.
 * @value: 0 is create project_id, other is use default project
 */
static void gt1x_creat_project_id(void)
{
	struct gt1x_ts_data *ts = gt1x_ts;
	unsigned int chip_id = 0;

	if(ts->create_project_id_supported && !ts->fw_damage_flag){
		memset(ts->project_id, 0, sizeof(ts->project_id));

		chip_id = gt1x_get_chip_id(ts->hw_info.product_id);
		snprintf(ts->project_id, MAX_STR_LEN, "%s%02d%02d0",
				ts->dev_data->ts_platform_data->product_name,
				chip_id, ts->dev_data->ts_platform_data->panel_id);
	}else{
		TS_LOG_INFO("%s: no support create_project_id, use default project_id\n", __func__);
	}

	TS_LOG_INFO("%s: project_id = %s\n", __func__, ts->project_id);
	return;
}

/**
 * gt1x_read_version - Read gt1x version info.
 * @hw_info: address to store version info
 * Return 0-succeed.
 */
int gt1x_read_version(struct gt1x_hw_info * hw_info)
{
	u8 buf[GT1X_VERSION_LEN] = { 0 };
	u32 mask_id = 0, patch_id = 0;
	u8 sensor_id = 0;
	int i = 0, retry = GT1X_RETRY_NUM;
	u8 checksum = 0;
	int ret = -1;
	struct gt1x_ts_data *ts = gt1x_ts;

	if(!hw_info){
		TS_LOG_ERR("%s: Invalid parameter!\n", __func__);
		return -EINVAL;
	}

	ts->sensor_id_valid = false;
	ts->fw_damage_flag = false;
	strncpy(ts->dev_data->chip_name, GT1X_OF_NAME, MAX_STR_LEN -1);
	while (retry--) {
		ret = gt1x_i2c_read(GTP_REG_VERSION, buf, sizeof(buf));
		/*gtp firmware version infomation
		 *0 ~ 3  : product id
		 *   4   : patch CID(soft flag)
		 *  5, 6 : patch version
		 * 7 ~ 9 : mask id
		 *  10   : vendor id
		 *  11   : checksum
		 */
		for (i = 0; i < sizeof(buf); i++)
			TS_LOG_INFO("%s: buf[%d]=0x%x\n", __func__, i, buf[i]);

		if (!ret) {
			for (i = 0, checksum = 0; i < sizeof(buf); i++)
				checksum += buf[i];

			if (checksum == 0 &&/* first 3 bytes must be number or char */
				IS_NUM_OR_CHAR(buf[0]) && IS_NUM_OR_CHAR(buf[1])
				&& IS_NUM_OR_CHAR(buf[2]) && buf[10] != 0xFF) {
				break;
			} else if (IC_TYPE_9P == ts->ic_type) {
				if (checksum == (u8)(buf[11] * 2) && buf[10] != 0xFF)/* checksum calculated by boot code */
					break;
			}
		}
		TS_LOG_ERR("%s : Read version failed,retry: %d\n", __func__, retry);
		msleep(100);
	}

	if (retry <= 0){
		TS_LOG_ERR("%s: failed read_version, retry[%d]\n", __func__, retry);
		return -ENODEV;
	}

	mask_id = (u32) ((buf[7] << 16) | (buf[8] << 8) | buf[9]);
	sensor_id = buf[10] & 0x0F;
	patch_id = (u32) ((buf[4] << 16) | (buf[5] << 8) | buf[6]);

	if (sensor_id > GT1X_MAX_SENSOR_ID) {
		TS_LOG_ERR("%s: Invalid sensor ID\n", __func__);
		return -EINVAL;
	}
	hw_info->sensor_id = sensor_id;
	hw_info->mask_id = mask_id;
	hw_info->patch_id = patch_id; 

	memset(hw_info->product_id, 0, GT1X_PRODUCT_ID_LEN +1);
	memcpy(hw_info->product_id, buf, GT1X_PRODUCT_ID_LEN);
	if(memcmp(hw_info->product_id, ts->chip_name, GT1X_PRODUCT_ID_LEN)){
		TS_LOG_ERR("%s: The firmware is damaged.\n", __func__); 
		memset(hw_info->product_id, 0, GT1X_PRODUCT_ID_LEN + 1);
		memcpy(hw_info->product_id, ts->chip_name, GT1X_PRODUCT_ID_LEN);
		ts->fw_damage_flag = true;
		return -EINVAL;
	}

	TS_LOG_INFO("%s :IC Version:GT%s_%06X(FW)_%04X(Boot)_%02X(SensorID)\n",
			__func__, hw_info->product_id, patch_id, mask_id >> 8, sensor_id);

	gt1x_ts->sensor_id_valid = true;

	snprintf(ts->dev_data->version_name, MAX_STR_LEN -1, "0x%04X", patch_id);

	return NO_ERR;
}

static int gt1x_prepar_parse_dts(struct ts_kit_platform_data *pdata)
{
	int ret;
	struct gt1x_ts_data *ts = gt1x_ts;
	struct ts_kit_device_data *chip_data = ts->dev_data;

	ret = of_property_read_u32(pdata->chip_data->cnode, GT1X_INIT_DELAY,
		&ts->init_delay);
	if (ret) {
		TS_LOG_INFO("%s: init_delay use default value\n", __func__);
		ts->init_delay = false;
	}
	TS_LOG_INFO("%s:init_delay value: [%d]\n", __func__, ts->init_delay);

	return NO_ERR;
}

static void gt1x_strtolower(const char *str, const int len)
{
	int index = 0;
	struct gt1x_ts_data *ts = gt1x_ts;

	memset(ts->project_id, 0, sizeof(ts->project_id));
	for (index = 0; index < len && index < MAX_STR_LEN; index++) {
		ts->project_id[index] = tolower(str[index]); 
	}
}

static int gt1x_get_lcd_module_name(void)
{
	char temp[LCD_PANEL_INFO_MAX_LEN] = {0};
	int i = 0;
	strncpy(temp, gt1x_ts->lcd_panel_info, LCD_PANEL_INFO_MAX_LEN-1);
	for(i=0;i<MAX_STR_LEN-1;i++)
	{
		if(temp[i] == '_'){
			break;
		} else if(temp[i] == '-'){
			break;
		}
		gt1x_ts->lcd_module_name[i] = tolower(temp[i]);
	}
	TS_LOG_INFO("lcd_module_name = %s.\n", gt1x_ts->lcd_module_name);
	return 0;
}
static int gt1x_get_lcd_panel_info(void)
{
	struct device_node *dev_node = NULL;
	char *lcd_type = NULL;
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

	strncpy(gt1x_ts->lcd_panel_info, lcd_type, LCD_PANEL_INFO_MAX_LEN-1);
	TS_LOG_INFO("lcd_panel_info = %s.\n", gt1x_ts->lcd_panel_info);
	return 0;
}

/**
 * gt1x_parse_dts - parse gt1x private properties
 */
static int gt1x_parse_dts(void)
{
	struct gt1x_ts_data *ts = gt1x_ts;
	struct device_node *device = ts->pdev->dev.of_node;
	struct ts_kit_device_data *chip_data = ts->dev_data;
	char *tmp_buff = NULL;
	int ret = 0;
	int value =0;

	ret = of_property_read_string(device, GTP_CHIP_NAME, (const char**)&tmp_buff);
	if (ret || tmp_buff == NULL)
	{
		TS_LOG_ERR("%s: chip_name read failed\n", __func__);
		ret = -EINVAL;
		goto err;
	}
	strncpy(ts->chip_name, tmp_buff, GT1X_PRODUCT_ID_LEN);
	TS_LOG_INFO("%s: chip_name = %s  \n", __func__,ts->chip_name);
	ret = of_property_read_string(device, GTP_PROJECT_ID, (const char**)&tmp_buff);
	if (ret || tmp_buff == NULL)
	{
		TS_LOG_ERR("%s: project_id read failed\n", __func__);
		goto err;
	}
	gt1x_strtolower(tmp_buff, strlen(tmp_buff));//exchange name to lower

	ret = of_property_read_u32(device, GTP_IC_TYPE, &value);
	if (ret) {
		TS_LOG_ERR("%s:ic_type read failed, Use default value: 9PT\n", __func__);
		value = IC_TYPE_9PT;
	}
	ts->ic_type = value;
	TS_LOG_INFO("%s: ic_type = %s  \n", __func__,ts->ic_type);

	ret = of_property_read_u32(device, FW_ONLY_DEPEND_ON_LCD, &value);
	if (ret) {
		TS_LOG_ERR("%s:fw_only_depend_on_lcd read failed, Use default value: 0\n", __func__);
		value = 0;
	}
	ts->fw_only_depend_on_lcd = value;
	TS_LOG_INFO("%s: fw_only_depend_on_lcd = %d  \n", __func__,ts->fw_only_depend_on_lcd);

	ret = of_property_read_u32(device, GTP_EDGE_ADD, &value);
	if (ret) {
		TS_LOG_ERR("%s:edge_add read failed, Use default value: 0\n", __func__);
		value = 0;
	}
	ts->gt1x_edge_add = value;
	TS_LOG_INFO("%s:edge_add value: [%x]\n", __func__, ts->gt1x_edge_add);

	ret = of_property_read_u32(device, GTP_SUPPORT_EDGE_XYER, &value);
	if (ret) {
		TS_LOG_ERR("%s:enable edge touch read failed, Use default value: 0\n", __func__);
		value = 0;
	}
	ts->gt1x_edge_support_xyer= value;
	TS_LOG_INFO("%s:gt1x_edge_support_xyer value: %d\n", __func__, ts->gt1x_edge_support_xyer);

	ret = of_property_read_u32(device, GTP_SUPPORT_WXY, &value);
	if (ret) {
		TS_LOG_ERR("%s:support wx wy read failed, Use default value: 0\n", __func__);
		value = 0;
	}
	ts->gt1x_support_wxy = value;
	TS_LOG_INFO("%s:gt1x_support_wxy value: [%d]\n", __func__, ts->gt1x_support_wxy);

	if(ts->gt1x_support_wxy) {
		ret = of_property_read_u32(device, GTP_WXY_DATA_ADD, &value);
		if (ret) {
			TS_LOG_ERR("%s:gt1x_wxy_data_add read failed, Use default value: 0\n", __func__);
			value = 0;
		}
		ts->gt1x_wxy_data_add = value;
		TS_LOG_INFO("%s:gt1x_wxy_data_add value: [%x]\n", __func__, ts->gt1x_wxy_data_add);
	}

	ret = of_property_read_u32(device, GTP_ROI_DATA_ADD, &value);
	if (ret) {
		TS_LOG_INFO("%s:roi_data address read failed, Use default value. \n", __func__);
		value = 0;
	}
	ts->gt1x_roi_data_add = value;
	TS_LOG_INFO("%s:roi_data address used is: [%x]\n", __func__, ts->gt1x_roi_data_add);


	ret = of_property_read_u32(device, GTP_ROI_FW_SUPPORTED, &value);
	if (ret) {
		TS_LOG_INFO("%s: gt1x_roi_fw_supported use default value\n", __func__);
		value = GT1X_EXIST;
	}
	ts->gt1x_roi_fw_supported = value;
	TS_LOG_INFO("%s:gt1x_roi_fw_supported value: [%d]\n", __func__, ts->gt1x_roi_fw_supported);

	ret = of_property_read_u32(device, GTP_EASY_WAKE_SUPPORTED, &value);
	if (ret) {
		value = 0;
		TS_LOG_INFO("%s: easy_wakeup_supported use default value\n", __func__);
	}
	ts->easy_wakeup_supported = (u8)value;
	TS_LOG_INFO("%s:easy_wakeup_supported value: [%d]\n", __func__, ts->easy_wakeup_supported);

	ret = of_property_read_u32(device, GTP_CREATE_PROJECT_ID, &value);
	if (ret) {
		value = 0;
		TS_LOG_INFO("%s: create_project_id_flag use default value\n", __func__);
	}
	ts->create_project_id_supported= (u8)value;
	TS_LOG_INFO("%s:create_project_id_supported value: [%d]\n", __func__, ts->create_project_id_supported);

	if(ts->dev_data->ts_platform_data->feature_info.roi_info.roi_supported){
		ret = of_property_read_u32_index(device, GTP_ROI_DATA_SIZE, 0,
				&ts->roi.roi_rows);
		if (ret) {
			TS_LOG_ERR("%s : Get ROI rows failed\n", __func__);
			ret = -EINVAL;
			goto err;
		}
		ret = of_property_read_u32_index(device, GTP_ROI_DATA_SIZE, 1,
				&ts->roi.roi_cols);
		if (ret) {
			TS_LOG_ERR("%s : Get ROI cols failed\n", __func__);
			ret = -EINVAL;
			goto err;
		}
		TS_LOG_INFO("%s : roi_rows = [%d], roi_cols = [%d] \n", __func__, ts->roi.roi_rows, ts->roi.roi_cols);
	}
	ret = of_property_read_u32(device, GTP_TOOL_SUPPORT, &value);
	if (ret) {
		value = 0;
		TS_LOG_INFO("%s: tools_support use default value\n", __func__);
		ts->tools_support = false;

	}
	ts->tools_support = value;
	TS_LOG_INFO("%s:tools_support value: [%d]\n", __func__, ts->tools_support);

	ret = of_property_read_u32(device, GT1X_SUPPORT_TP_COLOR, &value);
	if (ret) {
		value = 0;
		TS_LOG_INFO("%s: support_get_tp_color use default value\n", __func__);
		ts->support_get_tp_color = false;

	}
	ts->support_get_tp_color = value;

	ret = of_property_read_u32(device, GT1X_OPEN_ONCE_THRESHOLD, &value);
	if (ret) {
		value = 0;
		TS_LOG_INFO("%s: only_open_once_captest_threshold use default value\n", __func__);
		ts->only_open_once_captest_threshold = 0;
	}
	ts->only_open_once_captest_threshold = value;
	TS_LOG_INFO("%s: only_open_once_captest_threshold = %d\n", __func__,
		ts->only_open_once_captest_threshold);

	TS_LOG_INFO("%s: gt1x_parse_dts end\n", __func__);
	ret = NO_ERR;
err:
	return ret;
}

static int gt1x_parse_specific_dts(void)
{
	struct device_node *device = NULL;
	char project_id[20] = {0};
	u32 value = 0;
	int ret = 0;
	char *tmp_buff = NULL;
	struct gt1x_ts_data *ts = gt1x_ts;

	snprintf(project_id, sizeof(project_id), "gt1x-sensorid-%u", ts->hw_info.sensor_id);
	TS_LOG_INFO("%s: Parse specific dts:%s\n", __func__, project_id);
	device = of_find_compatible_node(ts->pdev->dev.of_node, NULL, project_id);
	if (!device) {
		TS_LOG_INFO("%s : No chip specific dts: %s, need to parse\n",
			__func__, project_id);
		return -EINVAL;
	}

	ret = of_property_read_u32(device, "x_max_mt", &value);
	if (!ret)
		ts->max_x = value;

	ret = of_property_read_u32(device, "y_max_mt", &value);
	if (!ret)
		ts->max_y = value;

	ret = of_property_read_u32(device, GTP_PANEL_ID, &value);
	if (ret){
		TS_LOG_ERR("%s: panel_id read failed\n", __func__);
		return -EINVAL;
	}

	/*provide panel_id for sensor*/
	ts->dev_data->ts_platform_data->panel_id = value;

	ret = of_property_read_string(device, GTP_MODULE_VENDOR, (const char**)&tmp_buff);
	if (ret || tmp_buff == NULL){
		TS_LOG_ERR("%s: vendor_name read failed\n", __func__);
		return -EINVAL;
	}
	memset(ts->dev_data->module_name, 0, MAX_STR_LEN);
	strncpy(ts->dev_data->module_name, tmp_buff, MAX_STR_LEN -1);

	ret = of_property_read_string(device, GTP_PROJECT_ID, (const char**)&tmp_buff);
	if (ret || tmp_buff == NULL)
	{
		TS_LOG_ERR("%s: project_id read failed\n", __func__);
		return -EINVAL;
	}
	gt1x_strtolower(tmp_buff, strlen(tmp_buff));//exchange name to lower

	TS_LOG_INFO("%s: x_max_mt=%d,y_max_mt=%d,panel_id=%d,vendor_name=%s,project_id=%s\n", 
		__func__, ts->max_x, ts->max_y,ts->dev_data->ts_platform_data->panel_id,ts->dev_data->module_name,
		ts->project_id);
	return NO_ERR;
}

/**
 * gt1x_parse_dt_cfg - parse config data from devices tree.
 * @dev: device that this driver attached.
 * @cfg: pointer of the config array.
 * @cfg_len: pointer of the config length.
 * @sid: sensor id.
 * Return: 0-succeed, -1-faileds
 */

 static int gt1x_parse_cfg_data(const struct firmware *cfg_bin,
				char *cfg_type, u8 *cfg, int *cfg_len, u8 sid)
{
	int i = 0, config_status = 0, one_cfg_count = 0;

	u8 bin_group_num = 0, bin_cfg_num = 0;
	u16 cfg_checksum = 0, checksum = 0;
	u8 sid_is_exist = GT1X_NOT_EXIST;

	if (getU32(cfg_bin->data) + BIN_CFG_START_LOCAL != cfg_bin->size) {
		TS_LOG_ERR("%s:Bad firmware!(file length: %d, header define: %d)\n", 
			__func__, cfg_bin->size, getU32(cfg_bin->data));
		goto exit;
	}

	// check firmware's checksum
	cfg_checksum = getU16(&cfg_bin->data[4]);

	for (i = BIN_CFG_START_LOCAL; i < (cfg_bin->size) ; i++)
		checksum += cfg_bin->data[i];

	if ((checksum) != cfg_checksum) {
		TS_LOG_ERR("%s:Bad firmware!(checksum: 0x%04X, header define: 0x%04X)\n", 
			__func__, checksum, cfg_checksum);
		goto exit;
	}
	/* check head end  */

	bin_group_num = cfg_bin->data[MODULE_NUM];
	bin_cfg_num = cfg_bin->data[CFG_NUM];
	TS_LOG_INFO("%s:bin_group_num = %d, bin_cfg_num = %d\n",__func__, bin_group_num , bin_cfg_num);

	if(!strncmp(cfg_type, GT1X_TEST_CONFIG, strlen(GT1X_TEST_CONFIG)))
		config_status = 0;
	else if(!strncmp(cfg_type, GT1X_NORMAL_CONFIG, strlen(GT1X_NORMAL_CONFIG)))
		config_status = 1;
	else if(!strncmp(cfg_type, GT1X_NORMAL_NOISE_CONFIG, strlen(GT1X_NORMAL_NOISE_CONFIG)))
		config_status = 2;
	else if(!strncmp(cfg_type, GT1X_GLOVE_CONFIG, strlen(GT1X_GLOVE_CONFIG)))
		config_status = 3;
	else if(!strncmp(cfg_type, GT1X_GLOVE_NOISE_CONFIG, strlen(GT1X_GLOVE_NOISE_CONFIG)))
		config_status = 4;
	else if(!strncmp(cfg_type, GT1X_HOLSTER_CONFIG, strlen(GT1X_HOLSTER_CONFIG)))
		config_status = 5;
	else if(!strncmp(cfg_type, GT1X_HOLSTER_NOISE_CONFIG, strlen(GT1X_HOLSTER_NOISE_CONFIG)))
		config_status = 6;
	else if(!strncmp(cfg_type, GT1X_NOISE_TEST_CONFIG, strlen(GT1X_NOISE_TEST_CONFIG)))
		config_status = 7;
	else if (!strncmp(cfg_type, GT1X_GAME_SCENE_CONFIG, strlen(GT1X_GAME_SCENE_CONFIG)))
		config_status = 8;
	else{
		TS_LOG_ERR("%s: invalid config text field\n", __func__);
		goto exit;
	}

	for(i = 0 ; i < bin_group_num*bin_cfg_num ; i++ )
	{
		if( sid == (cfg_bin->data[CFG_HEAD_BYTES+ i*CFG_INFO_BLOCK_BYTES] )) //find cfg's sid in cfg.bin
		{
			sid_is_exist = GT1X_EXIST;
			if( config_status == (cfg_bin->data[CFG_HEAD_BYTES+1+ i*CFG_INFO_BLOCK_BYTES] ))
			{
				one_cfg_count = getU16(&cfg_bin->data[CFG_HEAD_BYTES+2+ i*CFG_INFO_BLOCK_BYTES]);
				if( one_cfg_count == CFG_SIZE_MAX || one_cfg_count == CFG_SIZE_MIN)
				{
					memcpy(cfg, &cfg_bin->data[CFG_HEAD_BYTES+bin_group_num*bin_cfg_num*
							CFG_INFO_BLOCK_BYTES+i*one_cfg_count], one_cfg_count);
					*cfg_len = one_cfg_count;
					TS_LOG_ERR("%s:one_cfg_count = %d, cfg_data1 = 0x%02x, cfg_data2 = 0x%02x\n", 
							__func__, one_cfg_count , cfg[0], cfg[1]);
					break;
				}else{
					TS_LOG_ERR("%s:(one_cfg_count_length: %d)\n", __func__, one_cfg_count);
					goto exit;
				}
			}
		}
	}
#if defined (CONFIG_HUAWEI_DSM)
	if(GT1X_NOT_EXIST == sid_is_exist) {
		ts_dmd_report(DSM_TP_FWUPDATE_ERROR_NO, "sensor_id (%d) is not exist.\n", sid);
	}
#endif
	if( i >= bin_group_num*bin_cfg_num ){
		TS_LOG_ERR("%s:(not find config ,config_status: %d)\n", __func__, config_status);
		goto exit;
	}

	return NO_ERR;
exit:
	return RESULT_ERR;
}
/**
 * gt1x_init_configs - Prepare config data for touch ic,
 * don't call this function after initialization.
 *
 * Return 0--success,<0 --fail.
 */
static int gt1x_get_cfg_data(const struct firmware *cfg_bin ,char *config_name, struct gt1x_ts_config *config)
{
	struct gt1x_ts_data *ts = gt1x_ts;
	u8 cfg_data[GTP_CONFIG_LENGTH] = {0};
	int cfg_len = 0;
	int ret = 0;

	config->initialized = false;

	/* parse config data */
	ret = gt1x_parse_cfg_data(cfg_bin, config_name, cfg_data,
				&cfg_len, ts->hw_info.sensor_id);
	if (ret < 0) {
		TS_LOG_ERR("%s: parse %s data failed\n", __func__, config_name);
		ret = -EINVAL;
		goto exit;
	}

	cfg_data[0] &= 0x7F; /* mask config version */
	TS_LOG_INFO("%s: %s  version:%d , size:%d\n", 
			__func__, config_name, cfg_data[0], cfg_len);
	memcpy(config->data, cfg_data, cfg_len);
	config->size = cfg_len;

	strncpy(config->name, config_name, MAX_STR_LEN);
	config->initialized = true;

exit:
	return ret; 
 }

static int gt1x_get_cfg_parms(struct gt1x_ts_data *ts, const char *filename)
{
	int ret = 0;
	const struct firmware *cfg_bin = NULL;
	ret = request_firmware(&cfg_bin, filename,&gt1x_ts->pdev->dev);
	if (ret < 0) {
		TS_LOG_ERR("%s:Request config firmware failed - %s (%d)\n",
					__func__, filename, ret);
		goto exit;
	}

	// compare file length with the length field in the firmware header
	if ((cfg_bin->size) < CFG_BIN_SIZE_MIN || cfg_bin->data == NULL) {
		TS_LOG_ERR("%s:Bad firmware!(config firmware sizle: %u)\n", __func__, cfg_bin->size);
		goto exit;
	}

	TS_LOG_INFO("%s: cfg_bin_size=%u\n", __func__, cfg_bin->size);
	/* parse normal config data */
	ret = gt1x_get_cfg_data(cfg_bin,GT1X_NORMAL_CONFIG, &ts->normal_config);
	if (ret < 0) {
		TS_LOG_ERR("%s: Failed to parse normal_config data:%d\n", __func__, ret);
	}
	ret = gt1x_get_cfg_data(cfg_bin,GT1X_TEST_CONFIG, &ts->test_config);
	if (ret < 0) {
		TS_LOG_ERR("%s: Failed to parse test_config data:%d\n", __func__, ret);
	}
	/* parse normal noise config data */
	ret = gt1x_get_cfg_data(cfg_bin,GT1X_NORMAL_NOISE_CONFIG, &ts->normal_noise_config);
	if (ret < 0) {
		TS_LOG_ERR("%s: Failed to parse normal_noise_config data\n", __func__);
	}

	/* parse noise test config data */
	ret = gt1x_get_cfg_data(cfg_bin,GT1X_NOISE_TEST_CONFIG, &ts->noise_test_config);
	if (ret < 0) {
		memcpy(&(ts->noise_test_config), &(ts->normal_config), sizeof(ts->noise_test_config));
		TS_LOG_ERR("%s: Failed to parse noise_test_config data,use normal_config data\n", __func__);
	}
	/* parse glove config data */
	if(ts->dev_data->ts_platform_data->feature_info.glove_info.glove_supported){
		ret = gt1x_get_cfg_data(cfg_bin,GT1X_GLOVE_CONFIG, &ts->glove_config);
		if (ret < 0) {
			TS_LOG_ERR("%s: Failed to parse glove_config data:%d\n", __func__, ret);
		}

		/* parse glove noise noise config data */
		ret = gt1x_get_cfg_data(cfg_bin,GT1X_GLOVE_NOISE_CONFIG, &ts->glove_noise_config);
		if (ret < 0) {
			TS_LOG_ERR("%s: Failed to parse glove_noise config data:%d\n", __func__, ret);
		}
	}

	/* parse holster config data */
	if(ts->dev_data->ts_platform_data->feature_info.holster_info.holster_supported){
		ret = gt1x_get_cfg_data(cfg_bin,GT1X_HOLSTER_CONFIG, &ts->holster_config);
		if (ret < 0) {
			TS_LOG_ERR("%s: Failed to parse holster_config data:%d\n", __func__, ret);
		}
		ret = gt1x_get_cfg_data(cfg_bin,GT1X_HOLSTER_NOISE_CONFIG, &ts->holster_noise_config);
		if (ret < 0) {
			TS_LOG_ERR("%s: Failed to parse holster_noise_config data:%d\n", __func__, ret);
		}
	}

	/* parse game scene config data */
	if (TS_SWITCH_TYPE_GAME == (ts->dev_data->touch_switch_flag & TS_SWITCH_TYPE_GAME)){
		ret = gt1x_get_cfg_data(cfg_bin, GT1X_GAME_SCENE_CONFIG, &ts->game_scene_config);
		if (ret < 0) {
			TS_LOG_ERR("%s: Failed to parse game scene data:%d\n", __func__, ret);
		}
	}

exit:
	if(cfg_bin != NULL){
		release_firmware(cfg_bin);
		cfg_bin = NULL;
	}
	return NO_ERR;
}

static int gt1x_init_configs(struct gt1x_ts_data *ts)
{
	char filename[GT1X_FW_NAME_LEN +1] = {0};
	int ret = 0;

	snprintf(filename, GT1X_FW_NAME_LEN, "ts/%s_%s_%s",
			ts->dev_data->ts_platform_data->product_name, 
			GT1X_OF_NAME, ts->hw_info.product_id);

	if(ts->fw_only_depend_on_lcd) { //TP fw only depend on LCD module
		ret = gt1x_get_lcd_panel_info();
		if(ret){
			TS_LOG_ERR("%s: get lcd panel info faile!\n ",__func__);
		} else {
			ret = gt1x_get_lcd_module_name();
			if(!ret) {
				strncat(filename, "_", GT1X_FW_NAME_LEN - strlen(filename));
				strncat(filename, ts->lcd_module_name, GT1X_FW_NAME_LEN - strlen(filename));
			}
		}
	}
	strncat(filename, ".bin", GT1X_FW_NAME_LEN - strlen(filename));
	TS_LOG_INFO("%s: filename =%s\n", __func__, filename);
	return gt1x_get_cfg_parms(ts, filename);
}
static int gt1x_power_parameter_config(void)
{

	TS_LOG_INFO("%s called\n" , __func__);
	return ts_kit_power_supply_get(TS_KIT_VCC);
}

static int gt1x_power_on(void)
{
	int ret = 0;

	ret = gt1x_reset_output(0);
	if(ret){
		TS_LOG_ERR("%s:gpio direction output fail, ret=%d\n",__func__, ret);
	}
	ret = ts_kit_power_supply_ctrl(TS_KIT_VCC, TS_KIT_POWER_ON, 10);
	if(ret){
		goto exit_regulator_put;
	}
	gt1x_power_on_gpio_set();

	return 0;
exit_regulator_put:
	gt1x_power_release();
	return ret;
}
static void gt1x_power_off(void)
{
	if(gt1x_reset_output(0)){
		TS_LOG_ERR("%s:gpio direction output to fail.\n",__func__);
	}
	udelay(GT1X_RESET_PIN_D_U_TIME);
	ts_kit_power_supply_ctrl(TS_KIT_VCC, TS_KIT_POWER_OFF, 5);
}
static int gt1x_power_release(void)
{
	TS_LOG_INFO("%s called\n" , __func__);
	ts_kit_power_supply_put(TS_KIT_VCC);
	return 0;
}
static int gt1x_chip_detect(struct ts_kit_platform_data *pdata)
{
	int ret = NO_ERR;

	TS_LOG_INFO("%s : Chip detect.\n", __func__);

	if (!pdata){
		TS_LOG_ERR("%s device, ts_kit_platform_data *data is NULL \n", __func__);
		ret = -ENOMEM;
		goto exit;
	}else if(!pdata->ts_dev){
		TS_LOG_ERR("%s device, ts_kit_platform_data data->ts_dev is NULL \n", __func__);
		ret = -ENOMEM;
		goto exit;
	}

	pdata->client->addr = gt1x_ts->dev_data->slave_addr;
	gt1x_ts->pdev = pdata->ts_dev;
	gt1x_ts->dev_data->ts_platform_data =pdata;
	gt1x_ts->pdev->dev.of_node = pdata->chip_data->cnode;
	gt1x_ts->ops.i2c_read = gt1x_i2c_read;
	gt1x_ts->ops.i2c_write = gt1x_i2c_write;
	gt1x_ts->ops.chip_reset = gt1x_chip_reset;
	gt1x_ts->ops.send_cmd = gt1x_send_cmd;
	gt1x_ts->ops.send_cfg = gt1x_send_cfg;
	gt1x_ts->ops.i2c_read_dbl_check = gt1x_i2c_read_dbl_check;
	gt1x_ts->ops.read_version = gt1x_read_version;
	gt1x_ts->ops.parse_cfg_data = gt1x_parse_cfg_data;
	gt1x_ts->ops.feature_resume = gt1x_feature_resume;
	gt1x_ts->tools_support = true;
	gt1x_ts->open_threshold_status = true;

	/* Do *NOT* remove these logs */
	TS_LOG_INFO("%s:Driver Version: %s\n", __func__, GTP_DRIVER_VERSION);
	ret = gt1x_prepar_parse_dts(pdata);
	if (ret < 0){
		TS_LOG_ERR("%s: prepare_parse_dts fail!\n", __func__);
		return ret;
	}

	ret = gt1x_pinctrl_init();
	if (ret < 0){
		TS_LOG_ERR("%s: pinctrl_init fail\n", __func__);
		goto err_pinctrl_init;
	}

	ret = gt1x_power_parameter_config();
	if (ret < 0){
		TS_LOG_ERR("%s : get regulators fail!\n", __func__);
		goto exit;
	}

	/* power on */
	ret = gt1x_power_on();
	if (ret < 0){
		TS_LOG_ERR("%s, failed to enable power, ret = %d\n", __func__, ret);
		goto err_pinctrl_init;
	}

	/* reset chip */
	ret = gt1x_chip_reset();
	if (ret < 0){
		TS_LOG_ERR("%s, failed to chip_reset, ret = %d\n", __func__, ret);
		goto err_power_off;
	}

	ret = gt1x_i2c_read_hwinfo();
	if (ret < 0){
		TS_LOG_ERR("%s, failed to i2c_testt, ret = %d\n", __func__, ret);
		goto err_power_off;
	}

	init_completion(&roi_data_done);
	return ret;

err_power_off:
	gt1x_power_off();
	gt1x_power_release();
err_pinctrl_init:
	gt1x_pinctrl_release();
exit:
	if(NULL != gt1x_ts->dev_data){
		kfree(gt1x_ts->dev_data);
		gt1x_ts->dev_data = NULL;
	}
	if(NULL != gt1x_ts){
		kfree(gt1x_ts);
		gt1x_ts = NULL;
	}
	return ret;
}

static int gt1x_chip_init(void)
{
	struct gt1x_ts_data *ts = gt1x_ts;
	int ret = -1;
	u8 reg_val[1] = {0};

#if defined (CONFIG_TEE_TUI)
	strncpy(tee_tui_data.device_name, "gt1x", strlen("gt1x"));
	tee_tui_data.device_name[strlen("gt1x")] = '\0';
#endif

	/* check main system firmware */
	ret = gt1x_i2c_read_dbl_check(GTP_REG_FW_CHK_MAINSYS, reg_val, 1);
	if (ret ||reg_val[0] != FW_IS_OK) {/*Check the firmware integrity*/
		TS_LOG_ERR("%s : Check main system not pass[0x%2X]\n",
				__func__, reg_val[0]);
	}

	/* check subsystem firmware */ 
	ret = gt1x_i2c_read_dbl_check(GTP_REG_FW_CHK_SUBSYS, reg_val, 1);
	if (ret ||reg_val[0] == FW_IS_RUNING) {/*Check the firmware running status*/
		TS_LOG_ERR("%s : Check main system not pass[0x%2X]\n",
				__func__, reg_val[0]);
	}

	/* obtain gt1x dt properties */
	ret = gt1x_parse_dts();
	if (ret < 0){
		TS_LOG_ERR("%s: parse_dts fail!\n", __func__);
		return ret;
	}
	/* read version information. pid/vid/sensor id */
	ret = gt1x_read_version(&ts->hw_info);
	if (ret < 0){
		TS_LOG_ERR("%s: read_version fail!\n", __func__);
	}

	/* obtain specific dt properties */
	ret	= gt1x_parse_specific_dts();
	if (ret < 0){
		TS_LOG_ERR("%s: parse_specific_dts fail!\n", __func__);
		//return ret;
	}

	if (ts->tools_support){
		ret = gt1x_init_tool_node();
		if (ret < 0)
			TS_LOG_ERR("%s : init_tool_node fail!\n", __func__);
	}

	if (ts->dev_data->ts_platform_data->feature_info.roi_info.roi_supported) {
		ret = gt1x_ts_roi_init(&gt1x_ts->roi);
		if (ret < 0){
			TS_LOG_ERR("%s: gt1x_roi_init fail!\n", __func__);
			return ret;
		}
	}

	/* init config data, normal/glove/hoslter config data */
	ret = gt1x_init_configs(ts);
	if (ret < 0) {
		TS_LOG_ERR("%s: init config failed\n", __func__);
		//return ret;
	}

	ret = gt1x_feature_resume(ts);
	if (ret < 0){
		TS_LOG_ERR("%s: gt1x_feature_resume fail!\n", __func__);
		//return ret;
	}

	TS_LOG_INFO("%s: end\n", __func__);
	return NO_ERR;
}
static void gt1x_chip_shutdown(void)
{

}

static int gt1x_input_config(struct input_dev *input_dev)
{
	struct gt1x_ts_data *ts = gt1x_ts;

	if (ts == NULL)
		return -ENODEV;

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
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

#ifdef INPUT_TYPE_B_PROTOCOL
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 7, 0))
	input_mt_init_slots(input_dev, GTP_MAX_TOUCH, INPUT_MT_DIRECT);
#else
	input_mt_init_slots(input_dev, GTP_MAX_TOUCH);
#endif
#endif
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, gt1x_ts->dev_data->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, gt1x_ts->dev_data->y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, GT1X_ABS_MAX_VALUE, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, GT1X_ABS_MAX_VALUE, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, GTP_MAX_TOUCH, 0, 0);

	return 0;
}

static int gt1x_wakeup_gesture_switch(struct ts_wakeup_gesture_enable_info* info)
{
	TS_LOG_INFO("%s:done\n", __func__);
	return NO_ERR;
}

static int gt1x_chip_wrong_touch(void)
{
	int rc = NO_ERR;
	mutex_lock(&wrong_touch_lock);
	gt1x_ts->dev_data->easy_wakeup_info.off_motion_on = true;
	mutex_unlock(&wrong_touch_lock);
	TS_LOG_INFO("%s:done\n", __func__);
	return rc;
}

static int gt1x_chip_resume(void)
{
	struct gt1x_ts_data *ts = gt1x_ts;
	int tskit_pt_station_flag = 0;
	int rc = NO_ERR;

	TS_LOG_INFO(" %s enter\n", __func__);

	ts_kit_get_pt_station_status(&tskit_pt_station_flag);

	switch (ts->dev_data->easy_wakeup_info.sleep_mode) {
	case TS_POWER_OFF_MODE:
		gt1x_pinctrl_select_normal();

		if (!tskit_pt_station_flag) {
			gt1x_power_on();
		}
		break;
	case TS_GESTURE_MODE:
	default:
		gt1x_pinctrl_select_normal();
		break;
	}
	rc = gt1x_chip_reset();
	if(rc){
		TS_LOG_ERR("%s chip_reset fail\n", __func__);
	}

	TS_LOG_INFO(" %s:exit\n", __func__);
	return NO_ERR;
}

static int gt1x_chip_after_resume(void *feature_info)
{
	TS_LOG_INFO("%s: enter\n", __func__);
	msleep(GT1X_RESET_SLEEP_TIME);
	gt1x_init_watchdog();
	gt1x_ts->rawdiff_mode = false;
	gt1x_feature_resume(gt1x_ts);
	return NO_ERR;
}

static int gt1x_chip_suspend(void)
{
	struct gt1x_ts_data *ts = gt1x_ts;
	int tskit_pt_station_flag = 0;

	TS_LOG_INFO(" %s enter\n", __func__);

	ts_kit_get_pt_station_status(&tskit_pt_station_flag);

	switch (ts->dev_data->easy_wakeup_info.sleep_mode) {
	case TS_POWER_OFF_MODE:
		gt1x_pinctrl_select_suspend();
		if (!tskit_pt_station_flag) {
			TS_LOG_INFO("%s: enter power_off mode\n", __func__);
			gt1x_power_off();
		}else{
			TS_LOG_INFO("%s: enter sleep mode\n", __func__);
			gt1x_switch_wrokmode(SLEEP_MODE); /*sleep mode*/
		}
		break;
		/*for gesture wake up mode suspend. */
	case TS_GESTURE_MODE:
		if(!ts->easy_wakeup_supported){
			TS_LOG_ERR("%s: disable easy_wakeup, enter deep sleep!!\n", __func__);
			gt1x_pinctrl_select_suspend();
			gt1x_switch_wrokmode(SLEEP_MODE);
		}else{
			TS_LOG_INFO("%s: enter gesture mode\n", __func__);
			mutex_lock(&wrong_touch_lock);
			ts->dev_data->easy_wakeup_info.off_motion_on = true;
			mutex_unlock(&wrong_touch_lock);
			gt1x_pinctrl_select_gesture();
			gt1x_switch_wrokmode(GESTURE_MODE);
		}
		break;
	default:
		TS_LOG_INFO("%s: default enter sleep mode\n", __func__);
		gt1x_pinctrl_select_suspend();
		gt1x_switch_wrokmode(SLEEP_MODE);
		break;
	}
	TS_LOG_INFO("%s: exit\n", __func__);
	return NO_ERR;
}

/**
 * gt1x_fw_update_boot - update firmware while booting
 */
static int gt1x_fw_update_boot(char *file_name)
{
	struct gt1x_ts_data *ts = gt1x_ts;

	if ( !file_name) {
		TS_LOG_ERR("%s: Invalid file name\n", __func__);
		return -ENODEV;
	}
	memset(ts->firmware_name, 0, sizeof(ts->firmware_name));

	snprintf(ts->firmware_name, GT1X_FW_NAME_LEN, "ts/%s%s_%s.img", 
				file_name, ts->project_id, ts->dev_data->module_name);

	TS_LOG_INFO("%s: file_name = %s\n", __func__, ts->firmware_name);
	return gt1x_update_firmware();
}

/**
 * gt1x_fw_update_sd - update firmware from sdcard
 *  firmware path should be '/sdcard/gt1x_fw.bin'
 */
static int gt1x_fw_update_sd(void)
{
	struct gt1x_ts_data *ts = gt1x_ts;
	int ret =0;

	TS_LOG_INFO("%s: start ...\n", __func__);
	gt1x_get_cfg_parms(ts, GT1X_CONFIG_FW_SD_NAME);
	msleep(100);
	memset(ts->firmware_name, 0, sizeof(ts->firmware_name));
	strncpy(ts->firmware_name, GT1X_FW_SD_NAME, GT1X_FW_NAME_LEN);
	update_info.force_update = true;
	TS_LOG_INFO("%s: file_name = %s\n", __func__, ts->firmware_name);
	ret = gt1x_update_firmware();
	update_info.force_update = false;
	return ret;
}

static int gt1x_chip_get_info(struct ts_chip_info_param *info)
{
	struct gt1x_ts_data *ts = gt1x_ts;
	int len = 0;

	TS_LOG_INFO("%s: enter\n", __func__);
	if (!info || !ts || !ts->dev_data || !ts->dev_data->ts_platform_data){
		TS_LOG_ERR("%s: info or ts is null\n", __func__);
		return -EINVAL;
	}

	memset(info->ic_vendor, 0, sizeof(info->ic_vendor));
	memset(info->mod_vendor, 0, sizeof(info->mod_vendor));
	memset(info->fw_vendor, 0, sizeof(info->fw_vendor));

	if(!ts->dev_data->ts_platform_data->hide_plain_id) {
		len = (sizeof(info->ic_vendor) - 1) > sizeof(GT1X_OF_NAME) ?
				sizeof(GT1X_OF_NAME) : (sizeof(info->ic_vendor) - 1);
		strncpy(info->ic_vendor, GT1X_OF_NAME, len);
	} else {
		len = (sizeof(info->ic_vendor) - 1) > strlen(ts->project_id) ?
			strlen(ts->project_id) : (sizeof(info->ic_vendor) - 1);
		strncpy(info->ic_vendor, ts->project_id, len);
	}

	len = (sizeof(info->mod_vendor) -1 ) > strlen(ts->project_id)? 
		strlen(ts->project_id): (sizeof(info->mod_vendor) -1 );
	strncpy(info->mod_vendor, ts->project_id, sizeof(info->mod_vendor) -1);

	snprintf(info->fw_vendor, sizeof(info->fw_vendor) -1,"%s-0x%02X", 
		ts->dev_data->module_name, ts->hw_info.patch_id);

	return NO_ERR;
}

#ifdef CONFIG_HUAWEI_DEVKIT_MTK_3_0
static void gt1x_irq_output(int level)
{
	int error = 0;
	TS_LOG_INFO("%s:  level = %d\n", __func__, level);
	if (level)
		error = pinctrl_select_state(gt1x_ts->pinctrl, gt1x_ts->pinctrl_state_int_high);
	else
		error = pinctrl_select_state(gt1x_ts->pinctrl, gt1x_ts->pinctrl_state_int_low);

	if (error < 0)
			TS_LOG_ERR("%s:Set irq pin state error:%d\n", __func__, error);
}
#endif
static int gt1x_reset_output(int level)
{
	int error = 0;
	int reset_gpio = 0;

	TS_LOG_INFO("%s:  level = %d\n", __func__, level);
#ifdef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	if (level)
		error = pinctrl_select_state(gt1x_ts->pinctrl, gt1x_ts->pinctrl_state_reset_high);
	else
		error = pinctrl_select_state(gt1x_ts->pinctrl, gt1x_ts->pinctrl_state_reset_low);
#else
	reset_gpio = gt1x_ts->dev_data->ts_platform_data->reset_gpio;
	if (level)
		error = gpio_direction_output(reset_gpio, 1);
	else
		error = gpio_direction_output(reset_gpio, 0);
#endif
	if (error < 0)
			TS_LOG_ERR("%s:Set reset pin state error:%d\n", __func__, error);
	return error;
}

static int gt1x_gpio_reset(void)
{
	int ret = 0;

	ret = gt1x_reset_output(1);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 1 fail, ret=%d\n",
				   __func__, ret);
		return ret;
	}
	msleep(1);

	ret = gt1x_reset_output(0);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 0 fail, ret=%d\n",
				   __func__, ret);
		return ret;
	}
	msleep(1);

	ret = gt1x_reset_output(1);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 1 fail, ret=%d\n",
				   __func__, ret);
		return ret;
	}
	return 0;
}

static void gt1x_select_addrs(void)
{
	int ret = 0;

	TS_LOG_INFO("%s start\n", __func__);
	ret = gt1x_reset_output(PULL_DOWN);
	if (ret < 0){
		TS_LOG_ERR("%s:Set reset output to 0 fail:%d\n", __func__, ret);
	}
	mdelay(GT1X_DELAY_10);
	gt1x_irq_output(PULL_UP);
	mdelay(GT1X_DELAY_10);
	ret = gt1x_reset_output( PULL_UP);
	if (ret < 0){
		TS_LOG_ERR("%s:Set reset output to 1 fail:%d\n", __func__, ret);
	}
	if (gt1x_ts->init_delay)
		msleep(GT1X_DELAY_INIT);
	else
		msleep(GT1X_DELAY_80);
	ret = pinctrl_select_state(gt1x_ts->pinctrl, gt1x_ts->pinctrl_state_as_int);
	if (ret < 0){
		TS_LOG_ERR("%s:Set pinctrl_state_as_int fail:%d\n", __func__, ret);
	}
	return;
}

/**
 * gt1x_chip_reset - reset chip
 */
int gt1x_chip_reset(void)
{
#ifndef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	int ret = 0;

	ret = gt1x_reset_output(PULL_DOWN);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 0 fail, ret=%d\n",
				   __func__, ret);
		return ret;
	}
	udelay(GT1X_RESET_PIN_D_U_TIME);
	ret = gt1x_reset_output(PULL_UP);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 1 fail, ret=%d\n",
				   __func__, ret);
		return ret;
	}
	msleep(GT1X_RESET_SLEEP_TIME); /* chip initialize */
#else
	gt1x_select_addrs();
#endif
	return gt1x_init_watchdog();
}

/**
 * gt1x_resume_chip_reset - LCD resume reset chip
 */
static void gt1x_resume_chip_reset(void)
{
	int ret = 0;
	TS_LOG_INFO("%s: Chip reset\n", __func__);

	ret = gt1x_reset_output(0);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 0 fail\n", __func__);
	}
	udelay(GT1X_RESET_PIN_D_U_TIME);
	ret = gt1x_reset_output(1);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 1 fail\n", __func__);
	}
}
/**
 * gt1x_glove_switch - switch to glove mode
 */
static int gt1x_glove_switch(struct ts_glove_info *info)
{
	static bool glove_en = false;
	int ret = 0;
	u8 buf = 0;

	if (!info || !info->glove_supported) {
		TS_LOG_ERR("%s: info is Null or no support glove mode\n", __func__);
		return -EINVAL;
	}

	switch (info->op_action) {
	case TS_ACTION_READ:
		if (glove_en)
			info->glove_switch = 1;
		else
			info->glove_switch = 0;
		break;
	case TS_ACTION_WRITE:
		if (info->glove_switch) {
			/* enable glove feature */
			ret = gt1x_feature_switch(gt1x_ts,
					TS_FEATURE_GLOVE, SWITCH_ON);
			if (!ret)
				glove_en = true;
		} else {
			/* disable glove feature */
			ret = gt1x_feature_switch(gt1x_ts,
					TS_FEATURE_GLOVE, SWITCH_OFF);
			if (!ret)
				glove_en = false;
		}

		if (ret < 0)
			TS_LOG_ERR("%s:set glove switch(%d), failed : %d\n", __func__, buf, ret);
		break;
	default:
		TS_LOG_ERR("%s: invalid switch status: %d\n", __func__, info->glove_switch);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int gt1x_charger_switch(struct ts_charger_info *info)
{
	int ret = 0;

	if (!info || !info->charger_supported) {
		TS_LOG_ERR("%s: info is null or not support charge_supported\n", __func__);
		return -ENOMEM;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		if (info->charger_switch){
			ret = gt1x_send_cmd(GTP_CMD_CHARGER_ON, 0x00, GT1X_NOT_NEED_SLEEP);
			TS_LOG_INFO("%s: Charger switch on\n", __func__);
		} else {
			ret = gt1x_send_cmd(GTP_CMD_CHARGER_OFF, 0x00, GT1X_NOT_NEED_SLEEP);
			TS_LOG_INFO("%s: Charger switch off\n", __func__);
		}
		break;
	default:
		TS_LOG_ERR("%s: Invalid cmd\n", __func__);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int gt1x_palm_switch(struct ts_palm_info *info)
{
    TS_LOG_INFO("%s : not support palm\n", __func__);
	return 0;
}

static int gt1x_holster_switch(struct ts_holster_info *info)
{
	int ret = 0;

	if (!info || !info->holster_supported) {
		TS_LOG_ERR("%s: info is Null or no support holster mode\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	switch (info->op_action) {
		case TS_ACTION_WRITE:
			if (info->holster_switch)
				ret = gt1x_feature_switch(gt1x_ts,
					TS_FEATURE_HOLSTER, SWITCH_ON);
			else
				ret = gt1x_feature_switch(gt1x_ts,
					TS_FEATURE_HOLSTER, SWITCH_OFF);
			if (ret < 0)
				TS_LOG_ERR("%s:set holster switch(%d), failed: %d\n",
							__func__, info->holster_switch, ret);
			break;
		case TS_ACTION_READ:
			TS_LOG_INFO("%s: invalid holster switch(%d) action: TS_ACTION_READ\n",
							__func__, info->holster_switch);
			break;
		default:
			TS_LOG_INFO("%s: invalid holster switch(%d) action: %d\n",
							__func__, info->holster_switch, info->op_action);
			ret = -EINVAL;
			break;
	}

	return ret;
}

/*
 * gt1x_check_hw_status - Hw exception checking
 */
static int gt1x_check_hw_status(void)
{
	u8 esd_buf[4] = {0};
	u8 feed_wd[3] = {0xAA, 0x00, 0x55};
	int ret = -1, i;

	for (i = 0; i < 3; i++) {
		ret = gt1x_i2c_read(GTP_REG_CMD, esd_buf, 4);
		if (unlikely(ret < 0))
			break;
		if (esd_buf[0] != 0xAA && esd_buf[3] == 0xAA)
			break; /* status:OK */
		msleep(50);
	}

	if (likely(!ret && i < 3)) {
		TS_LOG_DEBUG("%s: HW status:OK\n", __func__);
		/* IC works normally, feed the watchdog */
		ret = gt1x_i2c_write(GTP_REG_CMD, &feed_wd[0],
							sizeof(feed_wd));
	} else {
		TS_LOG_ERR("%s: HW status:Error,reg{8040h}=%02X,reg{8043h}=%02X\n",
					__func__, esd_buf[0], esd_buf[3]);
		ret = -EIO;
	}

	return ret;
}

static int gt1x_roi_switch(struct ts_roi_info *info)
{
	int ret = 0;

	if (!info || !info->roi_supported ||!gt1x_ts) {
		TS_LOG_ERR("%s: info is Null or not supported roi mode\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		if (info->roi_switch == 1) {
			gt1x_ts->roi.enabled = true;
		} else if (info->roi_switch == 0) {
			gt1x_ts->roi.enabled = false;
		} else {
			TS_LOG_ERR("%s:Invalid roi switch value:%d\n", __func__, info->roi_switch);
			ret = -EINVAL;
		}
		break;
	case TS_ACTION_READ:
		// read
		break;
	default:
		break;
	}
	return ret;
}

static u8* gt1x_roi_rawdata(void)
{
	u8 *rawdata_ptr = NULL;
	struct gt1x_ts_data *ts = gt1x_ts;

	if (!ts->dev_data->ts_platform_data->feature_info.roi_info.roi_supported || !ts->roi.rawdata){
		TS_LOG_ERR("%s : Not supported roi mode or rawadta is null.\n", __func__);
		return NULL;
	}
	if (wait_for_completion_interruptible_timeout(&roi_data_done, msecs_to_jiffies(30))) {
		mutex_lock(&gt1x_ts->roi.mutex);
		if (gt1x_ts->roi.enabled && gt1x_ts->roi.data_ready) {
			rawdata_ptr = (u8 *)gt1x_ts->roi.rawdata;
		} else {
			TS_LOG_ERR("%s : roi is not enable or roi data not ready\n", __func__);
		}
		mutex_unlock(&gt1x_ts->roi.mutex);
	} else {
		TS_LOG_ERR("%s : wait roi_data_done timeout! \n", __func__);
	}
	return rawdata_ptr;
}

static void gt1x_work_after_input_kit(void)
{
	if(gt1x_ts->dev_data->ts_platform_data->feature_info.roi_info.roi_supported){
		/* We are about to refresh roi_data. To avoid stale output, use a completion to block possible readers. */
		reinit_completion(&roi_data_done);
		TS_LOG_DEBUG("%s: pre[%d]  cur[%d]  &[%d] gt1x_roi_fw_supported[%d] \n", __func__, pre_index, cur_index, (cur_index & pre_index), gt1x_ts->gt1x_roi_fw_supported);
		if ((pre_index != cur_index) && ((cur_index & pre_index) != cur_index)) {
			if(gt1x_ts->gt1x_roi_fw_supported) {
				gt1x_cache_roidata_fw(&gt1x_ts->roi);
			} else {
				gt1x_cache_roidata_device(&gt1x_ts->roi);
			}
		}
		pre_index = cur_index;
		complete_all(&roi_data_done);  /* If anyone has been blocked by us, wake it up. */
	}
}


static int gt1x_chip_get_capacitance_test_type(
				struct ts_test_type_info *info)
{
	int ret = 0;

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	switch (info->op_action) {
	case TS_ACTION_READ:
		memcpy(info->tp_test_type,
				gt1x_ts->dev_data->tp_test_type,
				TS_CAP_TEST_TYPE_LEN);
		TS_LOG_INFO("%s: test_type=%s\n", __func__, info->tp_test_type);
		break;
	case TS_ACTION_WRITE:
		break;
	default:
		TS_LOG_ERR("%s: invalid status: %s\n", __func__, info->tp_test_type);
		ret = -EINVAL;
		break;
	}
	return ret;
}

/*
 * parse driver and sensor num only on screen
 */
int gt1x_get_channel_num(u32 *sen_num, u32 *drv_num)
{
	int ret = 0;
	u8 buf[3] = {0};
	u32 sen_num_reg = 0;
	u32 drv_num_reg = 0;

	if(!sen_num || !drv_num){
		TS_LOG_ERR("%s: invalid param\n", __func__);
		return -EINVAL;
    }

	if (IC_TYPE_9P == gt1x_ts->ic_type) {
		sen_num_reg = SEN_NUM_REG_9P;
		drv_num_reg = DRV_NUM_REG_9P;
	} else {
		sen_num_reg = SEN_NUM_REG_9PT;
		drv_num_reg = DRV_NUM_REG_9PT;
	}

	ret = gt1x_i2c_read(drv_num_reg, &buf[0], 2);
	if (ret) {
		TS_LOG_ERR("%s: Failed read drv_num reg\n", __func__);
		return ret;
	} else
		*drv_num = (buf[0] & 0x1F) + (buf[1] & 0x1F);

	ret = gt1x_i2c_read(sen_num_reg, &buf[0], 1);
	if (ret) {
		TS_LOG_ERR("%s: Failed read sen_num reg\n", __func__);
		return ret;
	} else
		*sen_num = buf[0];

	TS_LOG_INFO("%s: drv_num:%d,sen_num:%d\n", 
				__func__, *drv_num, *sen_num);

	if (*drv_num > MAX_DRV_NUM || *drv_num <= 0
		|| *sen_num > MAX_SEN_NUM || *sen_num <= 0) {
		TS_LOG_ERR("%s: invalid sensor or driver num\n", __func__);
		return -EINVAL;
	}

	return ret;
}

static int gt1x_get_debug_data_(u16 *debug_data, u32 sen_num, 
		u32 drv_num, int data_type)
{
	int i = 0;
	int ret = 0;
	int data_len = 0;
	u8 buf[1] = {0};
	u8 *ptr = NULL;
	u16 data_start_addr;
	int wait_cnt = 0;

	if (READ_RAW_DATA == data_type) {	/*  get rawdata */
		data_len = (sen_num * drv_num) * sizeof(u16); /*  raw data is u16 type */
		if (IC_TYPE_9P == gt1x_ts->ic_type)
			data_start_addr = GTP_RAWDATA_ADDR_9P;
		else
			data_start_addr = GTP_RAWDATA_ADDR_9PT;
	} else if (READ_DIFF_DATA == data_type) { /* get noisedata */
		data_len = sen_num * drv_num; /*  noise data is u8 type */
		if (IC_TYPE_9P == gt1x_ts->ic_type)
			data_start_addr = GTP_REG_NOISEDATA_9P;
		else
			data_start_addr = GTP_REG_NOISEDATA_9PT;
	} else {
		TS_LOG_ERR("%s:Not support data_type:%d\n", __func__, data_type);
		return -EINVAL;
	}

	if (data_len <= 0) {
		TS_LOG_ERR("%s:Invaliable data len: %d\n", __func__, data_len);
		return -EINVAL;
	}

	ptr = (u8*)kzalloc(data_len, GFP_KERNEL);
	if (!ptr) {
		TS_LOG_ERR("%s:Failed alloc memory\n", __func__);
		return -ENOMEM;
	}

	/* change to rawdata mode */
	ret = gt1x_send_cmd(GTP_CMD_RAWDATA, 0x00, GT1X_NEED_SLEEP);
	if (ret) {
		TS_LOG_ERR("%s: Failed send rawdata cmd:ret%d\n", __func__, ret);
		kfree(ptr);
		ptr = NULL;
		return ret;
	}

	for (wait_cnt = 0; wait_cnt < GT1X_RETRY_NUM; wait_cnt++) {
		/* waiting for data ready */
		msleep(150);
		ret = gt1x_i2c_read(GTP_READ_COOR_ADDR, &buf[0], 1);
		if ((ret == 0) && (buf[0] & 0x80))
			break;
		TS_LOG_INFO("%s:Rawdata is not ready,buf[0]=0x%x\n", __func__, buf[0]);
	}

	if (wait_cnt < GT1X_RETRY_NUM) {
		ret = gt1x_i2c_read(data_start_addr,
				ptr, data_len);
		if (ret) {
			TS_LOG_ERR("%s: Failed to read debug data\n", __func__);
			goto get_data_out;
		} else
			TS_LOG_INFO("%s:Success read debug data:datasize=%d\n", __func__, data_len);

		if (READ_RAW_DATA == data_type) {
			for (i = 0; i < data_len; i+=2)
				debug_data[i/2] = be16_to_cpup((__be16*)&ptr[i]);
		} else {
			for (i = 0; i < data_len; i++)
				debug_data[i] = ptr[i];
		}

		/* clear data ready flag */
		buf[0] = 0x00;
		gt1x_i2c_write(GTP_READ_COOR_ADDR, &buf[0], 1);
	} else {
		TS_LOG_ERR("%s:Wait for data ready timeout\n", __func__);
		ret = -EFAULT;
	}

get_data_out:
	if (ptr) {
		kfree(ptr);
		ptr = NULL;
	}
	if (gt1x_send_cmd(GTP_CMD_NORMAL, 0x00, GT1X_NEED_SLEEP))
		TS_LOG_ERR("%s: Send normal mode command falied\n", __func__);
	return ret;
}

int gt1x_get_debug_data(struct ts_diff_data_info *info,
		struct ts_cmd_node *out_cmd)
{
	int i = 0;
	int ret = 0;
	u32 sen_num = 0;
	u32 drv_num = 0;
	int used_size = 0;
	u16 *debug_data = NULL;
	size_t data_size = 0;


	ret = gt1x_get_channel_num(&sen_num, &drv_num);
	if (ret) {
		TS_LOG_ERR("%s:Failed get channel num, ret =%d\n", __func__, ret);
		goto exit;
	}

	data_size = sen_num * drv_num;
	debug_data = kzalloc(data_size * sizeof(u16), GFP_KERNEL);
	if (!debug_data) {
		ret = -ENOMEM;
		TS_LOG_ERR("%s: alloc mem fail\n", __func__);
		goto exit;
	}

	switch (info->debug_type) {
	case READ_DIFF_DATA:
		TS_LOG_INFO("%s: read diff data\n", __func__);
		ret = gt1x_get_debug_data_(debug_data, sen_num, 
				drv_num, READ_DIFF_DATA);
		break;
	case READ_RAW_DATA:
		TS_LOG_ERR("%s: read raw data\n", __func__);
		ret = gt1x_get_debug_data_(debug_data, sen_num, 
				drv_num, READ_RAW_DATA);
		break;
	default:
		ret = -EINVAL;
		TS_LOG_ERR("%s: debug_type mis match\n", __func__);
		break;
	}

	if (ret)
		goto free_debug_data;

	info->buff[used_size++] = drv_num;
	info->buff[used_size++] = sen_num;
	for (i = 0; i < data_size && used_size < TS_RAWDATA_BUFF_MAX; i++)
		info->buff[used_size++] = debug_data[i];

	info->used_size = used_size;

free_debug_data:
	if(debug_data){
		kfree(debug_data);
		debug_data = NULL;
	}

exit:
	return ret;
}

#define GT1X_DOZE_MAX_INPUT_SEPARATE_NUM 2
static void gt1x_set_doze_mode(unsigned int soper, unsigned char param)
{
	struct gt1x_ts_data *ts = gt1x_ts;
	int error = 0;

	if ((NULL == ts) || (NULL == ts->dev_data)){
		TS_LOG_ERR("%s, error chip data\n",__func__);
		goto out;
	}

	if (TS_SWITCH_TYPE_DOZE != (ts->dev_data->touch_switch_flag & TS_SWITCH_TYPE_DOZE)){
		TS_LOG_ERR("%s, doze mode does not suppored by this chip\n",__func__);
		goto out;
	}

	switch (soper){
		case TS_SWITCH_DOZE_ENABLE:
			TS_LOG_INFO("%s:enter doze_mode[param:%d]\n", __func__, param);
			error = gt1x_send_cmd(GTP_CMD_DOZE_CONFIG, param, 1);
			if (error) {
				TS_LOG_ERR("%s: Failed send doze enable cmd: error%d\n", __func__, error);
			}
			break;
		case TS_SWITCH_DOZE_DISABLE:
			TS_LOG_INFO("%s:exit doze_mode\n", __func__);
			/*holdoff > 100 means always at active status*/
			error = gt1x_send_cmd(GTP_CMD_DOZE_CONFIG, 0xFF, 1);
			if (error) {
				TS_LOG_ERR("%s: Failed send doze disable cmd: error%d\n", __func__, error);
			}
			break;
		default:
			TS_LOG_ERR("%s: soper unknown:%u, invalid\n", __func__, soper);
			break;
		}
out:
	return;
}

static void gt1x_set_game_mode(unsigned int soper)
{
	struct gt1x_ts_data *ts = gt1x_ts;
	int error = 0;
	struct gt1x_ts_config *config = NULL;
	struct ts_feature_info *info = NULL;

	if ((NULL == ts) || (NULL == ts->dev_data) || (NULL == ts->dev_data->ts_platform_data)){
		TS_LOG_ERR("%s, error chip data\n", __func__);
		goto out;
	}

	info = &ts->dev_data->ts_platform_data->feature_info;
	if (TS_SWITCH_TYPE_GAME != (ts->dev_data->touch_switch_flag & TS_SWITCH_TYPE_GAME)){
		TS_LOG_ERR("%s, game mode does not suppored by this chip\n",__func__);
		goto out;
	}

	switch (soper){
		case TS_SWITCH_GAME_ENABLE:
			TS_LOG_INFO("%s: enter game_mode\n", __func__);
			error = gt1x_send_cfg(&ts->game_scene_config);
			if (error) {
				TS_LOG_ERR("%s: Send game ENABLE config error\n", __func__);
			}
			break;
		case TS_SWITCH_GAME_DISABLE:
			if (!info) {
				TS_LOG_ERR("%s, error info data\n",__func__);
				goto out;
			}

			TS_LOG_INFO("%s: exit game_mode to [holster_switch:%d][glove_switch:%d]\n", __func__,
			info->holster_info.holster_switch,info->glove_info.glove_switch);
			if (info->holster_info.holster_switch)
				config = &ts->holster_config;
			else if (info->glove_info.glove_switch)
				config = &ts->glove_config;
			else
				config = &ts->normal_config;

			error = gt1x_send_cfg(config);
			if (error) {
				TS_LOG_ERR("%s: Send DISABLE config error\n", __func__);
			}
			break;
		default:
			TS_LOG_ERR("%s: soper unknown:%u, invalid\n", __func__, soper);
			break;
	}
out:
	return;
}

static void gt1x_chip_touch_switch(void)
{
	char in_data[MAX_STR_LEN] = {0};
	unsigned int stype = 0, soper = 0, time = 0;
	u8 param =0;
	int error = 0;
	unsigned int i = 0, cnt = 0;
	struct gt1x_ts_data *ts = gt1x_ts;


	TS_LOG_INFO("%s enter\n", __func__);

	if ((NULL == ts) || (NULL == ts->dev_data)){
		TS_LOG_ERR("%s, error chip data\n",__func__);
		goto out;
	}

	/* SWITCH_OPER,ENABLE_DISABLE,PARAM */
	memcpy(in_data, ts->dev_data->touch_switch_info, MAX_STR_LEN -1);
	TS_LOG_INFO("%s, in_data:%s\n",__func__, in_data);
	for(i = 0; i < strlen(in_data) && (in_data[i] != '\n'); i++){
		if(in_data[i] == ','){
			cnt++;
		}else if(!isdigit(in_data[i])){
			TS_LOG_ERR("%s: input format error!!\n", __func__);
			goto out;
		}
	}
	if(cnt != GT1X_DOZE_MAX_INPUT_SEPARATE_NUM){
		TS_LOG_ERR("%s: input format error[separation_cnt=%d]!!\n", __func__, cnt);
		goto out;
	}

	error = sscanf(in_data, "%u,%u,%u", &stype, &soper, &time);
	if(error <= 0){
		TS_LOG_ERR("%s: sscanf error\n", __func__);
		goto out;
	}
	TS_LOG_DEBUG("stype=%u,soper=%u,param=%u\n", stype, soper, time);
	/**
	 * enter DOZE again after a period of time. the min unit of the time is 100ms,
	 * for example, we set 30, the final time is 30 * 100 ms = 3s
	**/
	param = (u8)time;

	switch (stype) {
		case TS_SWITCH_TYPE_DOZE:
			gt1x_set_doze_mode(soper, param);
			break;
		case TS_SWITCH_SCENE_3:
		case TS_SWITCH_SCENE_4:
			TS_LOG_INFO("%s : does not suppored\n", __func__);
			break;
		case TS_SWITCH_SCENE_5:
			gt1x_set_game_mode(soper);
			break;
		default:
			TS_LOG_ERR("%s: stype unknown:%u, invalid\n", __func__, stype);
			break;
	}

out:
	return;
}

struct ts_device_ops ts_gt1x_ops = {
	.chip_detect = gt1x_chip_detect,
	.chip_init = gt1x_chip_init,
	.chip_input_config = gt1x_input_config,
	.chip_irq_bottom_half = gt1x_irq_bottom_half,
	.chip_reset = gt1x_chip_reset,
	.chip_fw_update_boot = gt1x_fw_update_boot,
	.chip_fw_update_sd = gt1x_fw_update_sd,
	.chip_get_info = gt1x_chip_get_info,
	.chip_suspend = gt1x_chip_suspend,
	.chip_resume = gt1x_chip_resume,
	.chip_after_resume = gt1x_chip_after_resume,
	.chip_get_rawdata = gt1x_get_rawdata,
	.chip_glove_switch = gt1x_glove_switch,
	.chip_shutdown = gt1x_chip_shutdown,
	.chip_charger_switch = gt1x_charger_switch,
	.chip_palm_switch = gt1x_palm_switch,
	.chip_holster_switch = gt1x_holster_switch,
	.chip_roi_switch = gt1x_roi_switch,
	.chip_roi_rawdata = gt1x_roi_rawdata,
	.chip_check_status = gt1x_check_hw_status,
	.chip_get_capacitance_test_type = gt1x_chip_get_capacitance_test_type,
	.chip_get_debug_data = gt1x_get_debug_data,
	.chip_work_after_input = gt1x_work_after_input_kit,
	.chip_wrong_touch = gt1x_chip_wrong_touch,
	.chip_wakeup_gesture_enable_switch = gt1x_wakeup_gesture_switch,
	.chip_touch_switch = gt1x_chip_touch_switch,
};

static int __init gt1x_ts_module_init(void)
{
	int ret = NO_ERR;
	bool found = false;
	struct device_node *child = NULL;
	struct device_node *root = NULL;

	TS_LOG_INFO("%s: called\n", __func__);
	gt1x_ts = kzalloc(sizeof(struct gt1x_ts_data), GFP_KERNEL);
	if (NULL == gt1x_ts) {
		TS_LOG_ERR("%s:alloc mem for device data fail\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	gt1x_ts->dev_data =
		kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
	if (NULL == gt1x_ts->dev_data) {
		TS_LOG_ERR("%s:alloc mem for ts_kit_device data fail\n", __func__);
		ret = -ENOMEM;
		goto error_exit;
	}
	root = of_find_compatible_node(NULL, NULL, HUAWEI_TS_KIT);
	if (!root) {
		TS_LOG_ERR("%s:find_compatible_node error\n", __func__);
		ret = -EINVAL;
		goto error_exit;
	}

	for_each_child_of_node(root, child) {
		if (of_device_is_compatible(child, GTP_GT1X_CHIP_NAME)) {
			found = true;
			break;
		}
	}

	if (!found) {
		TS_LOG_ERR("%s:device tree node not found, name=%s\n",
			__func__, GTP_GT1X_CHIP_NAME);
		ret = -EINVAL;
		goto error_exit;
	}
	gt1x_ts->dev_data->cnode = child;
	gt1x_ts->dev_data->ops = &ts_gt1x_ops;
	ret = huawei_ts_chip_register(gt1x_ts->dev_data);
	if (ret) {
		TS_LOG_ERR("%s:chip register fail, ret=%d\n", __func__, ret);
		goto error_exit;
	}

	TS_LOG_INFO("%s:success\n", __func__);
	return ret;

error_exit:
	if(NULL != gt1x_ts->dev_data){
		kfree(gt1x_ts->dev_data);
		gt1x_ts->dev_data = NULL;
	}
	if(NULL != gt1x_ts){
		kfree(gt1x_ts);
		gt1x_ts = NULL;
	}

	TS_LOG_INFO("%s:fail\n", __func__);
	return ret;
}

static void __exit gt1x_ts_module_exit(void)
{
	if(NULL != gt1x_ts->dev_data){
		kfree(gt1x_ts->dev_data);
		gt1x_ts->dev_data = NULL;
	}
	if(NULL != gt1x_ts->roi.rawdata) {
		kfree(gt1x_ts->roi.rawdata);
		gt1x_ts->roi.rawdata = NULL;
	}
	if(NULL != gt1x_ts){
		kfree(gt1x_ts);
		gt1x_ts = NULL;
	}
	gt1x_test_free();
	return;
}

late_initcall(gt1x_ts_module_init);
module_exit(gt1x_ts_module_exit);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");
