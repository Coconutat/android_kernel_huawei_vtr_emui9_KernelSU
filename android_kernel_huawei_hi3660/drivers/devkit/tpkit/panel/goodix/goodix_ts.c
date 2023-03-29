/*
 * goodix_ts.c - Main touch driver file of Goodix
 *
 * Copyright (C) 2015 - 2016 Goodix Technology Incorporated   
 * Copyright (C) 2015 - 2016 Yulong Cai <caiyulong@goodix.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the GOODiX's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include "goodix_ts.h"
#include "goodix_dts.h"

bool gtp_detect_sucess = false;
struct goodix_ts_data *goodix_ts;
struct ts_kit_device_data *g_goodix_dev_data = NULL;

extern u8 cypress_ts_kit_color[TP_COLOR_SIZE];

 void goodix_int_sync(s32 ms);
 int goodix_reset_select_addr(void);

/**
 * goodix_i2c_write - i2c write.
 * @addr: register address.
 * @buffer: data buffer.
 * @len: the bytes of data to write.
 * Return: 0: success, otherwise: failed
 */
int goodix_i2c_write(u16 addr, u8 * buffer, u16 len)
{
	struct ts_kit_device_data *dev_data = goodix_ts->dev_data;
	u8 stack_mem[32], *data;
	int ret;

	if ( !dev_data->ts_platform_data->bops->bus_write)
		return -ENODEV;

	if (len + 2 > sizeof(stack_mem)) {
		data = kmalloc(len + 2, GFP_KERNEL);
		if (!data) {
			GTP_ERROR("No memory");
			return -ENOMEM;
		}
	} else {
		data = &stack_mem[0];
	}

	data[0] = addr >> 8 & 0xff;
	data[1] = addr & 0xff;
	memcpy(&data[2], buffer, (unsigned long)len);
	ret = dev_data->ts_platform_data->bops->bus_write(data, len + 2);
	if (ret < 0)
		GTP_ERROR("i2c write error,addr:%04x bytes:%d ret:%d", addr, len,ret);

	if (data != &stack_mem[0])
		kfree(data);

	return ret;
}

/**
 * goodix_i2c_read - i2c read.
 * @addr: register address.
 * @buffer: data buffer.
 * @len: the bytes of data to write.
 * Return: 0: success, otherwise: failed
 */
int goodix_i2c_read(u16 addr, u8 * buffer, u16 len)
{
	struct ts_kit_device_data *dev_data = goodix_ts->dev_data;
	int ret;
	
	if ( !dev_data->ts_platform_data->bops->bus_read)
		return -ENODEV;

	addr = cpu_to_be16(addr);
	ret = dev_data->ts_platform_data->bops->bus_read((u8 *)&addr, 2, buffer, len);
	if (ret < 0)
		GTP_ERROR("i2c read error,addr:%04x bytes:%d", addr, len);

	return ret;
}

/**
 * goodix_i2c_read_dbl_check - read twice and double check
 * @addr: register address
 * @buffer: data buffer
 * @len: bytes to read
 * Return    <0: i2c error, 0: ok, 1:fail
 */
int goodix_i2c_read_dbl_check(u16 addr, u8 * buffer, u16 len)
{
	u8 buf[16] = {0};
	u8 confirm_buf[16] = {0};
	int ret;

	if (len > 16) {
		GTP_ERROR("i2c_read_dbl_check length %d is too long, exceed %zu",
			len, sizeof(buf));
		return -EINVAL;
	}

	memset(buf, 0xAA, sizeof(buf));
	ret = goodix_i2c_read(addr, buf, len);
	if (ret < 0)
		return ret;

	msleep(5);
	memset(confirm_buf, 0, sizeof(confirm_buf));
	ret = goodix_i2c_read(addr, confirm_buf, len);
	if (ret < 0)
		return ret;

	if (!memcmp(buf, confirm_buf, len)) {
		memcpy(buffer, confirm_buf, len);
		return 0;
	}

	GTP_ERROR("i2c read 0x%04X, %d bytes, double check failed!", addr, len);
	return 1;
}

/**
 * goodix_send_cfg - Send config data to hw
 * @cfg_ptr: pointer to config data structure
 * Return 0--success,non-0--fail.
 */
int goodix_send_cfg(struct goodix_ts_config *cfg_ptr)
{
	//static DEFINE_MUTEX(mutex_cfg);
	u8 *config;
	int i, cfg_len;
	s32 ret = 0, retry = 0;
	u16 checksum = 0;

	if (!cfg_ptr || !cfg_ptr->initialized) {
		GTP_ERROR("Invalid config data");
		return -EINVAL;
	}

	config = &cfg_ptr->data[0];
	cfg_len = cfg_ptr->size;

	mutex_lock(&goodix_ts->mutex_cfg);
	GTP_INFO("Send %s,ver:%02x size:%d", cfg_ptr->name,
			config[0], cfg_len);

	if (cfg_len != GTP_CONFIG_ORG_LENGTH
		&& cfg_len != GTP_CONFIG_ORG_LENGTH + GTP_CONFIG_EXT_LENGTH) {
		GTP_ERROR("Invalid config size:%d", cfg_len);
		mutex_unlock(&goodix_ts->mutex_cfg);
		return -1;
	}

	for (i = 0, checksum = 0; i < cfg_len - 1 ; i ++)
		checksum += config[i] ;
	if ((checksum&0xFF) != 0) {
		GTP_ERROR("Invalid config,all of the bytes is zero");
		mutex_unlock(&goodix_ts->mutex_cfg);
		return -1;
	}
	config[cfg_len] = (~checksum) + 1;
	retry = 0;
	while (retry++ < 3) {
		ret = goodix_i2c_write(GTP_REG_CONFIG_DATA, config, cfg_len);
		if (!ret) {
			if (cfg_ptr->delay_ms > 0)
				msleep(cfg_ptr->delay_ms);
			mutex_unlock(&goodix_ts->mutex_cfg);
			GTP_INFO("Send config successfully");
			return 0;
		}
	}

	GTP_ERROR("Send config failed");
	mutex_unlock(&goodix_ts->mutex_cfg);
	return ret;
}

/**
 * goodix_send_cmd - seng cmd
 * must write data & checksum first
 * byte    content
 * 0       cmd
 * 1       data
 * 2       checksum
 * Returns 0 - succeed,non-0 - failed
 */
static int goodix_send_cmd(u8 cmd, u8 data)
{
	s32 ret;
	//static DEFINE_MUTEX(cmd_mutex);
	u8 buffer[3] = { cmd, data, 0 };

	mutex_lock(&goodix_ts->mutex_cmd);
	buffer[2] = (u8) ((0 - cmd - data) & 0xFF);
	ret = goodix_i2c_write(GTP_REG_CMD + 1, &buffer[1], 2);
	ret |= goodix_i2c_write(GTP_REG_CMD, &buffer[0], 1);
	msleep(50);
	mutex_unlock(&goodix_ts->mutex_cmd);

	return ret;
}

/**
 * goodix_init_watchdog - esd mechannism
 * 
 * Returns  0--success,non-0--fail.
 */
static inline int goodix_init_watchdog(void)
{
	/* 0x8040 ~ 0x8043 */
	u8 value[] = {0xAA};

	GTP_DEBUG("Init watchdog");
	return goodix_i2c_write(GTP_REG_CMD + 1, &value[0], 1);
}


/**
 * goodix_switch_config - Switch config data.
 * @cfg_type: GOODIX_NORMAL_CFG - normal config data
 *			  GOODIX_GLOVE_CFG - glove config data
 *			  GOODIX_HOLSTER_CFG - holster config data
 * Returns  0--success,non-0--fail.
 */
static int goodix_switch_config(int cfg_type)
{
	struct goodix_ts_data *ts = goodix_ts;
	struct goodix_ts_config *config;
	int ret;

	if (!ts)
		return -EINVAL;

	switch (cfg_type) {
	case GOODIX_NORMAL_CFG:
		config = &ts->normal_config;
		break;
	case GOODIX_GLOVE_CFG:
		config = &ts->glove_config;
		break;
	case GOODIX_HOLSTER_CFG:
		config = &ts->holster_config;
		break;
	default:
		return -EINVAL;
	}

	ret = goodix_send_cfg(config);
	return ret;
}

#ifdef ROI
/**
 * goodix_ts_roi_init - initialize roi feature
 * @roi: roi data structure
 * return 0 OK, < 0 fail
 */
int goodix_ts_roi_init(struct goodix_ts_roi *roi)
{
	unsigned int roi_bytes;

	if (!roi)
		return -EINVAL;

	if (!roi->roi_rows || !roi->roi_cols) {
		GTP_ERROR("Invalid roi config,rows:%d,cols:%d",
				roi->roi_rows, roi->roi_cols);
		return -EINVAL;
	}

	mutex_init(&roi->mutex);

	roi_bytes = (roi->roi_rows + roi->roi_cols) * 2;
	roi->rawdata = kmalloc(roi_bytes + ROI_HEAD_LEN, GFP_KERNEL);
	if (!roi->rawdata) {
		GTP_ERROR("Failed to alloc memory for roi");
		return -ENOMEM;
	}

	GTP_INFO("ROI init,rows:%d,cols:%d",
				roi->roi_rows, roi->roi_cols);

	return 0;
}

/**
 * goodix_cache_roidata - caching roi data
 * @roi: roi data structure
 * return 0 ok, < 0 fail
 */
static int goodix_cache_roidata(struct goodix_ts_roi *roi)
{
	unsigned roi_bytes,i;
	unsigned char status[ROI_HEAD_LEN];
	u16 checksum = 0;
	int ret;
	
	if ( !roi->enabled)
		return -EINVAL;
	ret = goodix_i2c_read(ROI_STA_REG, status, ROI_HEAD_LEN);
	if (ret < 0)
		return ret;

	for (i = 0; i < ROI_HEAD_LEN; i++)
		checksum += status[i];

	if ((u8)checksum != 0) { /* cast to 8bit checksum,*/
		GTP_ERROR("roi status checksum error");
		return -1;
	}

	if (status[0] & ROI_READY_MASK) /* roi data ready */
		roi->track_id = status[0] & ROI_TRACKID_MASK;
	else
		return -1; /* not ready */

	mutex_lock(&roi->mutex);
	roi->data_ready = false;
	roi_bytes = (roi->roi_rows * roi->roi_cols + 1) * 2;

	ret = goodix_i2c_read(ROI_DATA_REG,
				(u8 *)(roi->rawdata + ROI_HEAD_LEN), roi_bytes);
	if (ret < 0) {
		mutex_unlock(&roi->mutex);
		return ret;
	}

	for (i = ROI_HEAD_LEN, checksum = 0;
					i < roi_bytes / 2 + ROI_HEAD_LEN; i++) {
		/* 16bits */
		roi->rawdata[i] = be16_to_cpu(roi->rawdata[i]);
		checksum += roi->rawdata[i];
	}
	memcpy(&roi->rawdata[0], &status[0], ROI_HEAD_LEN);

	if (checksum != 0)
		GTP_ERROR("roi data checksum error");
	else
		roi->data_ready = true;

	mutex_unlock(&roi->mutex);

	status[0] = 0x00;
	ret = goodix_i2c_write(ROI_STA_REG, status, 1);

	return ret;
}
#endif

/**
 * goodix_request_event_handler - firmware request 
 * Return    <0: failed, 0: succeed
 */
static int goodix_request_event_handler(struct goodix_ts_data *ts)
{
	u8 rqst_data = 0;
	int ret;

	ret = goodix_i2c_read(GTP_REG_RQST, &rqst_data, 1);
	if (ret)
		return ret;

	GTP_DEBUG("Request state:0x%02x", rqst_data);
	switch (rqst_data & 0x0F) {
	case GTP_RQST_CONFIG:
		GTP_INFO("Request Config.");
		ret = goodix_send_cfg(&ts->normal_config);
		if (ret) {
			GTP_ERROR("Send config error");
		} else {
			GTP_INFO("Send config success");
			rqst_data = GTP_RQST_RESPONDED;
			goodix_i2c_write(GTP_REG_RQST, &rqst_data, 1);
		}
		break;
	case GTP_RQST_RESET:
		GTP_INFO("Request Reset.");
		goodix_chip_reset();
		rqst_data = GTP_RQST_RESPONDED;
		goodix_i2c_write(GTP_REG_RQST, &rqst_data, 1);
		break;
	default:
		break;
	}
	return 0;
}

static int goodix_check_key_gesture_report(struct ts_fingers *info,
					     struct ts_easy_wakeup_info *gesture_report_info,
					     unsigned char get_gesture_wakeup_data)
{
	unsigned int reprot_gesture_key_value = 0;

	if ((NULL == info) || (NULL == gesture_report_info)){
		TS_LOG_ERR("%s: info / gesture_report_info is null point\n", __func__);
		return -EINVAL;
	}

	TS_LOG_INFO("get_gesture_wakeup_data is %d \n", get_gesture_wakeup_data);

	switch (get_gesture_wakeup_data) {
		case DOUBLE_CLICK_WAKEUP:
			if (IS_APP_ENABLE_GESTURE(GESTURE_DOUBLE_CLICK) &
			    gesture_report_info->easy_wakeup_gesture) {
				TS_LOG_INFO("%s: DOUBLE_CLICK_WAKEUP detected\n", __func__);
				reprot_gesture_key_value = TS_DOUBLE_CLICK;
			}
			break;
		default:
			TS_LOG_INFO("%s: unknow gesture detected!\n", __func__);
			return RESULT_ERR;
	}

	if (0 != reprot_gesture_key_value) {
		info->gesture_wakeup_value = reprot_gesture_key_value;
	}else{
		TS_LOG_INFO("%s: reprot_gesture_key_value = 0 !!\n", __func__);
		return RESULT_ERR;
	}

	return NO_ERR;
}

static int goodix_check_gesture(struct ts_fingers *info)
{
	int i = 0;
	int ret = NO_ERR;
	unsigned char gesture_id[2] = {0};
	unsigned char doze_buf = 0;
	unsigned char buf = GTP_CMD_GESTURE_WAKEUP;

	struct ts_easy_wakeup_info *gesture_report_info = &g_goodix_dev_data->easy_wakeup_info;

	if (NULL == info){
		TS_LOG_ERR("%s: info is null point\n", __func__);
		return -EINVAL;
	}
	/*if the easy_wakeup_flag is false,status not in gesture;switch_value is false,gesture is no supported*/
	if ((false == g_goodix_dev_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value) ||
		(false == gesture_report_info->easy_wakeup_flag)){
		TS_LOG_DEBUG("%s: gesture \n", __func__);
		return RESULT_ERR;
	}

	ret = goodix_i2c_read(GTP_REG_WAKEUP_GESTURE, gesture_id, 2);
	if (ret < 0){
		TS_LOG_ERR("read gesture_id gesture_id fail\n");
		return RESULT_ERR;
	}
	TS_LOG_INFO("gesture_id = 0x%02X, point_num : %d ", gesture_id[0], gesture_id[1]);

	ret = goodix_check_key_gesture_report(info, gesture_report_info, gesture_id[0]);
	if (NO_ERR == ret){
		/* get gesture data susccess Clear 0x814B */
		ret = goodix_i2c_write(GTP_REG_WAKEUP_GESTURE, &doze_buf, 1);
		if(ret < 0){
			TS_LOG_ERR("%s: Clear 0x814B fail\n", __func__);
			return RESULT_ERR;
		}
	}else{
		/* get gesture data susccess Clear 0x814B */
		ret = goodix_i2c_write(GTP_REG_WAKEUP_GESTURE, &doze_buf, 1);
		if(ret < 0){
			TS_LOG_ERR("%s: Clear 0x814B fail\n", __func__);
			return RESULT_ERR;
		}

		/*Wakeup Gesture (d0) set 1 */
		while(i++ < WRITE_REG_GESTURE_RETRY){
			ret = goodix_i2c_write(GTP_REG_GESTURE, &buf,1);
			if (ret < 0){
				TS_LOG_ERR("failed to set doze flag into 0x8046, %d", ret);
				msleep(GTP_SLEEP_TIME_10);
				continue;
			}

			ret = goodix_i2c_write(GTP_REG_CMD, &buf,1);
			if (ret < 0){
				TS_LOG_ERR("failed to set doze flag into 0x8040, %d", ret);
				msleep(GTP_SLEEP_TIME_10);
				continue;
			}

			msleep(GTP_SLEEP_TIME_10);
			break;
		}

		TS_LOG_ERR("%s: read gestrue data error\n", __func__);
		return RESULT_ERR;
	}

	return NO_ERR;
}

/**
 * goodix_touch_evt_handler - handle touch event
 * (pen event, key event, finger touch envent)
 * Return    <0: failed, 0: succeed
 */
static int goodix_touch_evt_handler(struct goodix_ts_data  *ts,
				struct ts_fingers *info)
{
	u8 touch_data[GTP_MAX_TOUCH_DATA_LEN] = {0};
	static u16 pre_index = 0;
	static u16 pre_touch = 0;
	u16 cur_index = 0;
	u8 touch_num;
	//u8 *coor_data = NULL;
	int id, x, y, w, i;
	int id_offset;
	int ret = -1;
	int data_len = 0;

	ret = goodix_check_gesture(info);
	if (NO_ERR == ret){
		TS_LOG_DEBUG("%s: get gesture value success & return\n", __func__);
		goto exit;
	}

	ret = goodix_i2c_read(GTP_READ_COOR_ADDR, &touch_data[0], 1 + 8 + 2);
	if (ret < 0)
		goto exit;

	if ((touch_data[0]) == 0x00){
		TS_LOG_INFO("%s:touch data not ready!",__func__);
		return RESULT_ERR;
	}

	if((touch_data[0] & 0x80) == 0){
		TS_LOG_INFO("%s:No finger number!",__func__);
	        goto exit;
	}

	if((touch_data[0] == 0x80) && ( pre_touch == 0 )){
		TS_LOG_DEBUG("%s:empty packet!",__func__);
	        return RESULT_ERR;
	}

	touch_num = touch_data[0] & 0x0f;
	if (touch_num > GTP_MAX_TOUCH) {
		GTP_ERROR("Illegal finger number!");
		goto exit;
	}

	/* read the remaining coor data
		* 0x814E(touch status) +
		* 8(every coordinate consist of 8 bytes data) * touch num +
		* keycode + checksum
		*/
	if (touch_num > 1) {
		data_len = (touch_num - 1) * 8;
		if(data_len > 72){
		GTP_ERROR("Illegal finger number!");
		goto exit;
	}
		ret = goodix_i2c_read((GTP_READ_COOR_ADDR + 11),
					&touch_data[11], data_len);
		if (ret)
			goto exit;
	}

	/*  start check current event */

	memset(info, 0x00, sizeof(struct ts_fingers));
	//coor_data = &touch_data[1];
	if(touch_num>0){
	for (i = 0; i < touch_num; i++) {
		id_offset = (i<<3) + 1;
		id =( touch_data[id_offset]);

		x = le16_to_cpup((__le16 *)(&touch_data[id_offset+1]));
		y = le16_to_cpup((__le16 *)(&touch_data[id_offset+3]));
		w = le16_to_cpup((__le16 *)(&touch_data[id_offset+5]));

		if (ts->flip_x)
			info->fingers[id].x = ts->max_x - x;
		else
			info->fingers[id].x = x;

		if (ts->flip_y)
			info->fingers[id].y = ts->max_y - y;
		else
			info->fingers[id].y = y;

		info->fingers[id].major = w;
		info->fingers[id].minor = w;
		info->fingers[id].pressure = w;
		info->fingers[id].status = TP_FINGER;
		cur_index |= 1 << id;
	}
	info->cur_finger_number = touch_num;
	}
	else if (pre_touch){
		info->cur_finger_number = 0;
	}
		pre_touch = touch_num;

#ifdef ROI
	if (pre_index != cur_index && (cur_index & pre_index) != cur_index)
		goodix_cache_roidata(&ts->roi);
#endif

	pre_index = cur_index;
exit:
	return ret;
}

/**
 * goodix_irq_bottom_half - Goodix touchscreen work function.
 */
static int goodix_irq_bottom_half(struct ts_cmd_node *in_cmd,
				struct ts_cmd_node *out_cmd)
{
	struct goodix_ts_data *ts = goodix_ts;
	struct ts_fingers *ts_fingers;
	u8 sync_val = 0;
	int ret = 0;

	if (!ts)
		return -ENODEV;

	ts_fingers = &out_cmd->cmd_param.pub_params.algo_param.info;
	out_cmd->command = TS_INVAILD_CMD;
	out_cmd->cmd_param.pub_params.algo_param.algo_order =
			ts->dev_data->algo_id;

	/* handle touch event 
	 * return: <0 - error, 0 - touch event handled,
	 * 			1 - hw request event handledv */
	ret = goodix_touch_evt_handler(ts, ts_fingers);
	if (ret == 0)
		out_cmd->command = TS_INPUT_ALGO;
	ret = goodix_i2c_write(GTP_READ_COOR_ADDR, &sync_val, 1);

	return ret;
}

static int goodix_i2c_test(void)
{
	u8 hw_info;
	int ret;

	ret = goodix_i2c_read(GTP_REG_CONFIG_DATA,&hw_info,sizeof(hw_info));

	TS_LOG_INFO("IIC test Info:%08X", hw_info);
	return ret;
}


/**
 * goodix_pinctrl_init - pinctrl init
 */
static int goodix_pinctrl_init(struct goodix_ts_data *ts)
{
      int ret = 0;
       /* Get pinctrl if target uses pinctrl */
      ts->pinctrl = devm_pinctrl_get(&(ts->pdev->dev));
      if (IS_ERR_OR_NULL(ts->pinctrl)) {
	  	TS_LOG_ERR("Target does not use pinctrl %d\n", ret);
		ret = -EINVAL;
		return ret;
	}
	ts->pinctrl_state_active= pinctrl_lookup_state(ts->pinctrl,PINCTRL_STATE_ACTIVE);
	if (IS_ERR_OR_NULL(ts->pinctrl_state_active)) {
		TS_LOG_ERR("Can not lookup %s pinstate \n",PINCTRL_STATE_ACTIVE);
		ret = -EINVAL;
		goto err_pinctrl_lookup;
	}
	ts->pinctrl_state_suspend= pinctrl_lookup_state(ts->pinctrl,PINCTRL_STATE_SUSPEND);
	if (IS_ERR_OR_NULL(ts->pinctrl_state_suspend)) {
		TS_LOG_ERR("Can not lookup %s pinstate \n",PINCTRL_STATE_SUSPEND);
		ret = -EINVAL;
		goto err_pinctrl_lookup;
	}
	ts->pinctrl_state_release= pinctrl_lookup_state(ts->pinctrl,PINCTRL_STATE_RELEASE);
	if (IS_ERR_OR_NULL(ts->pinctrl_state_release)) {
		TS_LOG_ERR("-Can not lookup %s pinstate \n",PINCTRL_STATE_RELEASE);
		ret = -EINVAL;
		goto err_pinctrl_lookup;
	}
	ts->pinctrl_state_int_high= pinctrl_lookup_state(ts->pinctrl,PINCTRL_STATE_INT_HIGH);
	if (IS_ERR_OR_NULL(ts->pinctrl_state_int_high)) {
		TS_LOG_ERR("-Can not lookup %s pinstate \n",PINCTRL_STATE_RELEASE);
	}
	ts->pinctrl_state_int_low= pinctrl_lookup_state(ts->pinctrl,PINCTRL_STATE_INT_LOW);
	if (IS_ERR_OR_NULL(ts->pinctrl_state_int_low)) {
		TS_LOG_ERR("-Can not lookup %s pinstate \n", PINCTRL_STATE_RELEASE);
	}

	return 0;
err_pinctrl_lookup:
			devm_pinctrl_put(ts->pinctrl);
       return ret;

}
int goodix_pinctrl_select_normal(struct goodix_ts_data *ts)
{
	int ret = 0;
	if (goodix_ts->pinctrl && goodix_ts->pinctrl_state_active) {
	ret = pinctrl_select_state(goodix_ts->pinctrl, goodix_ts->pinctrl_state_active);
		if (ret < 0)
			TS_LOG_ERR("Set active pin state error:%d", ret);
	}
	return ret;
}
int goodix_pinctrl_select_suspend(struct goodix_ts_data *ts)
{
	int ret = 0;
	if (goodix_ts->pinctrl && goodix_ts->pinctrl_state_suspend) {
	ret = pinctrl_select_state(goodix_ts->pinctrl, goodix_ts->pinctrl_state_suspend);
		if (ret < 0)
			TS_LOG_ERR("Set suspend pin state error:%d", ret);
	}
	return ret;
}

static void goodix_pinctrl_release(struct goodix_ts_data *ts)
{
	if (ts->pinctrl)
		devm_pinctrl_put(ts->pinctrl);
	ts->pinctrl = NULL;
	ts->pinctrl_state_active = NULL;
	ts->pinctrl_state_suspend = NULL;
	ts->pinctrl_state_release = NULL;
}

/**
 * goodix_read_version - Read gt1x version info.
 * @hw_info: address to store version info
 * Return 0-succeed.
 */
int goodix_read_version(struct goodix_hw_info * hw_info)
{
	u8 buf[12] = { 0 };
	u32 mask_id, patch_id;
	u8 product_id[5] = {0};
	u8 sensor_id, match_opt;
	int retry = 3;
	unsigned int i;
	u8 checksum = 0;
	int ret = -1;

	while (retry--) {
		ret = goodix_i2c_read(GTP_REG_VERSION, buf, sizeof(buf));

		if (!ret) {
			for (i = 0, checksum = 0; i < sizeof(buf); i++)
				checksum += buf[i];

			if (checksum == 0 &&/* first 3 bytes must be number or char */
				IS_NUM_OR_CHAR(buf[0]) && IS_NUM_OR_CHAR(buf[1])
				&& IS_NUM_OR_CHAR(buf[2]) && buf[10] != 0xFF) {
				break;
			} else if (checksum == (u8)(buf[11] * 2) && buf[10] != 0xFF) {
				/* checksum calculated by boot code */
				break;
			} else{
			GTP_ERROR("Invalid version info:%c%c%c", buf[0], buf[1], buf[2]);
			break;}
		}

		GTP_DEBUG("Read version failed,retry: %d", retry);
		msleep(100);
	}

	if (retry <= 0)
		return -ENODEV;

	mask_id = (u32) ((buf[7] << 16) | (buf[8] << 8) | buf[9]);
	patch_id = (u32) ((buf[5] << 16) | (buf[4] << 8) | buf[6]);
	memcpy(product_id, buf, 4);
	sensor_id = buf[10] & 0x0F;
	match_opt = (buf[10] >> 4) & 0x0F;

	GTP_INFO("IC Version:GT%s_%06X(FW)_%04X(Boot)_%02X(SensorID)",
		product_id, patch_id, mask_id >> 8, sensor_id);

	sprintf(g_goodix_dev_data->version_name,"%02X",patch_id>>8);

	if (hw_info != NULL) {
		hw_info->mask_id = mask_id;
		hw_info->patch_id = patch_id;
		memcpy(hw_info->product_id, product_id, 5);
		hw_info->sensor_id = sensor_id;
		hw_info->match_opt = match_opt;
	}

	goodix_ts->sensor_id_valid = true;
	return 0;
}

/**
 * goodix_init_configs - Prepare config data for touch ic,
 * don't call this function after initialization.
 *
 * Return 0--success,<0 --fail.
 */
int goodix_init_configs(struct goodix_ts_data *ts)
{
	u8 sensor_id, *cfg_data;
	int cfg_len = 0;
	int ret = 0;

	sensor_id = ts->hw_info.sensor_id;
	if (sensor_id > 5) {
		GTP_ERROR("Invalid sensor ID");
		return -EINVAL;
	}

	/* max config data length */
	cfg_len = sizeof(ts->normal_config.data);
	cfg_data = kzalloc(cfg_len, GFP_KERNEL);
	if (!cfg_data)
		return -ENOMEM;

	/* parse normal config data */
	ret = goodix_parse_cfg_data(ts, "normal_config", cfg_data,
				&cfg_len, sensor_id);
	if (ret < 0) {
		GTP_ERROR("Failed to parse normal_config data:%d", ret);
		goto exit_kfree;
	}

	cfg_data[0] &= 0x7F; /* mask config version */
	GTP_INFO("Normal config version:%d,size:%d", cfg_data[0], cfg_len);
	memcpy(&ts->normal_config.data[0], cfg_data, cfg_len);
	ts->normal_config.size = cfg_len;
	ts->normal_config.delay_ms = 200;
	ts->normal_config.name = "normal_config";
	ts->normal_config.initialized = true;

	/* parse glove config data */
	ret = goodix_parse_cfg_data(ts, "glove_config", cfg_data,
				&cfg_len, sensor_id);
	if (ret < 0) {
		GTP_ERROR("Failed to parse glove_config data:%d", ret);
		ts->glove_config.initialized = false;
		ret = 0;
	} else if (cfg_len == ts->normal_config.size) {
		cfg_data[0] &= 0x7F; /* mask config version */
		GTP_INFO("Glove config version:%d,size:%d", cfg_data[0], cfg_len);
		memcpy(&ts->glove_config.data[0], cfg_data, cfg_len);
		ts->glove_config.size = cfg_len;
		ts->glove_config.delay_ms = 20;
		ts->glove_config.name = "glove_config";
		ts->glove_config.initialized = true;
	} else {
		ts->glove_config.initialized = false;
	}

	/* parse glove config data */
	ret = goodix_parse_cfg_data(ts, "holster_config", cfg_data,
				&cfg_len, sensor_id);
	if (ret < 0) {
		GTP_ERROR("Failed to parse holster_config data:%d", ret);
		ts->holster_config.initialized = false;
		ret = 0;
	} else if (cfg_len == ts->normal_config.size) {
		cfg_data[0] &= 0x7F; /* mask config version */
		GTP_INFO("Holster config version:%d,size:%d", cfg_data[0], cfg_len);
		memcpy(&ts->holster_config.data[0], cfg_data, cfg_len);
		ts->holster_config.size = cfg_len;
		ts->holster_config.delay_ms = 20;
		ts->holster_config.name = "holster_config";
		ts->holster_config.initialized = true;
	} else {
		ts->holster_config.initialized = false;
	}

exit_kfree:
	kfree(cfg_data);
	return ret;
}

int goodix_read_projetct_id(void)
{
	int ret = 0;
	u16 cmd = 0;
	u8 buf[GTP_PROJECT_ID_LEN] = {0};

	cmd = goodix_ts->pram_projectid_addr;

	ret = goodix_i2c_read(cmd, buf, sizeof(buf));
	if (ret < 0){
		TS_LOG_ERR("GTP read projetct id fail");
		return ret;
	}

	if (0xFF == buf[0]){
		memset(buf, 0, GTP_PROJECT_ID_LEN);
	}else if(0xFF == buf[9]){
		buf[9] = '\0';
	}

	strncpy(goodix_ts->project_id, buf, GTP_PROJECT_ID_LEN);

	TS_LOG_INFO("GTP read projetct id: %s %d\n", goodix_ts->project_id, buf[0]);

	return ret;
}

static void str_low(char *str)
{
	unsigned int i=0;

	if(NULL == str) {
		TS_LOG_ERR("%s: str is Null\n", __func__);
		return;
	}
	for (i = 0; i < strlen(str); i++){
		if ((str[i] >= 65) && (str[i] <= 90)){
			str[i] += 32;
		}
	}
}

static unsigned long str_to_hex(char *p)
{
	unsigned long hex = 0;
	unsigned long length = strlen(p), shift = 0;
	unsigned char dig = 0;

	str_low(p);
	length = strlen(p);

	if (length == 0){
		return 0;
	}

	do {
		dig = p[--length];
		dig = dig < 'a' ? (dig - '0') : (dig - 'a' + 0xa);
		hex |= (long)(unsigned)(dig << shift);
		shift += 4;
	} while (length);
	return hex;
}

int goodix_read_tp_color(void)
{
	int ret = 0;
	u16 cmd = 0;
	u8 buf[GTP_TP_COLOR_LEN] = {0};

	cmd = goodix_ts->pram_projectid_addr+(GTP_PROJECT_ID_LEN -1);
	TS_LOG_INFO("tp color addr is 0x%x\n",cmd);

	ret = goodix_i2c_read(cmd, buf, sizeof(buf));
	if (ret < 0){
		TS_LOG_ERR("GTP read tp color fail");
		return ret;
	}
	buf[GTP_TP_COLOR_LEN-1] = '\0';
	cypress_ts_kit_color[0]=(u8)str_to_hex(buf);
	TS_LOG_INFO("%s:tp color is 0x%x\n",__func__,cypress_ts_kit_color[0]);
	return ret;
}

int goodix_param_init(void)
{
	/* init project id*/
	goodix_read_projetct_id();

	if(goodix_ts->support_get_tp_color){
		/* Get tp_color */
		goodix_read_tp_color();
	}

	/* init IC name*/
	strncpy(goodix_ts->dev_data->chip_name , GTP_CHIP_NAME, MAX_STR_LEN);

	return 0;
}

int goodix_check_normal_config_version(struct goodix_ts_config *cfg_ptr){
	int ret = 0;
	s32 i = 0;
	u8 opr_buf[16] = {0};
	u8 drv_cfg_version = 0;
	u8 flash_cfg_version = 0;
	u8 check_sum = 0;

	/*get ic normal config version*/
	ret = goodix_i2c_read_dbl_check(GTP_REG_CONFIG_DATA, &opr_buf[0], 1);
	if (0 == ret) {
		TS_LOG_INFO("Config Version: 0x%02X; IC Config Version: 0x%02X\n",cfg_ptr->data[0],  opr_buf[0]);

		flash_cfg_version = opr_buf[0];
		drv_cfg_version = cfg_ptr->data[0];

		/*if flash_cfg_version > drv_cfg_version, update*/
		if (flash_cfg_version < 90 && flash_cfg_version > drv_cfg_version) {
			TS_LOG_INFO("flash_cfg_version > drv_cfg_version\n");
			cfg_ptr->data[0] = 0x00;
		}else{
			TS_LOG_INFO("flash_cfg_version < drv_cfg_version\n");
			return -1;
		}
	}else{
		TS_LOG_ERR("Failed to get ic config version!No config sent!\n");
		return -1;
	}
	check_sum = 0;
	TS_LOG_INFO("drv cfg Config size:%d\n",cfg_ptr->size);
	for (i = 0; i < cfg_ptr->size-GTP_ADDR_LENGTH; i++)
	{
		check_sum += cfg_ptr->data[i];
	}
	cfg_ptr->data[cfg_ptr->size-GTP_ADDR_LENGTH] = (~check_sum) + 1;

	ret = goodix_send_cfg(cfg_ptr);
	if (ret < 0) {
		TS_LOG_ERR("send config failed");
		return ret;
	}
	msleep(GTP_SLEEP_TIME_300);
	if (flash_cfg_version < 90 && flash_cfg_version > drv_cfg_version) {
		check_sum = 0;
		cfg_ptr->data[0] = drv_cfg_version;
		for (i = 0; i < cfg_ptr->size-GTP_ADDR_LENGTH; i++) {
			check_sum += cfg_ptr->data[i];
		}
		cfg_ptr->data[cfg_ptr->size-GTP_ADDR_LENGTH] = (~check_sum) + 1;
	}
	return 0;
}

static int goodix_chip_init(void)
{
	struct goodix_ts_data *ts = goodix_ts;
	int ret = -1;

	ret = goodix_chip_parse_config(goodix_ts->dev_data->cnode , goodix_ts->dev_data);
	if (ret < 0)
		return ret;

	/* read version information. pid/vid/sensor id */
	ret = goodix_read_version(&ts->hw_info);
	if (ret < 0)
		return ret;

	/* obtain goodix dt properties */
	ret = goodix_parse_dts(ts);
	if (ret < 0)
		return ret;

	/* obtain specific dt properties */
	ret	= goodix_parse_specific_dts(ts);
	if (ret < 0)
		return ret;

	if (ts->tools_support)
		init_wr_node();

	goodix_param_init();

	/* init config data, normal/glove/hoslter config data */
	ret = goodix_init_configs(ts);
	if (ret < 0) {
		GTP_ERROR("Init panel failed");
		return ret;
	}

	ret = goodix_check_normal_config_version(&ts->normal_config);
	if(ret < 0){
		TS_LOG_INFO("no need check normal config version\n");
	}else{
		TS_LOG_INFO("check normal config version success\n");
	}

	ret = goodix_send_cfg(&ts->normal_config);
	if (ret < 0) {
	GTP_ERROR("send config failed");
		return ret;
	}
#ifdef ROI
	goodix_ts_roi_init(&goodix_ts->roi);
#endif

	goodix_get_fw_data();

	return 0;
}

static int goodix_input_config(struct input_dev *input_dev)
{
	struct goodix_ts_data *ts = goodix_ts;

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
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ts->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ts->max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, GTP_MAX_TOUCH, 0, 0);

	return 0;
}

int goodix_pinctr_int_ouput_low(void)
{
	int ret = 0;
	if (goodix_ts->pinctrl && goodix_ts->pinctrl_state_int_low) {
	ret = pinctrl_select_state(goodix_ts->pinctrl, goodix_ts->pinctrl_state_int_low);
	if (ret < 0)
		TS_LOG_ERR("Set irq pin state error:%d", ret);
	}
	return ret;
}

static int goodix_sleep_mode_in(void)
{
	int ret = RESULT_ERR;
	s8 i = 0;
	u8 buf = GTP_CMD_SLEEP;

	goodix_pinctr_int_ouput_low();
	msleep(5);
	while(i++ < WRITE_REG_GESTURE_RETRY){
		ret = goodix_i2c_write(GTP_REG_CMD, &buf,1);
		if (0 == ret){
			msleep(GTP_SLEEP_TIME_10);
			TS_LOG_INFO("GTP enter sleep!\n");
			break;
		}
		msleep(GTP_SLEEP_TIME_10);
	}

	if (i >= WRITE_REG_GESTURE_RETRY){
		TS_LOG_ERR("GTP send sleep cmd failed.\n");
		return RESULT_ERR;
	}

	return NO_ERR;
}

static int goodix_put_device_into_easy_wakeup(void)
{
	int ret = RESULT_ERR;
	s8 i = 0;
	u8 buf = GTP_CMD_GESTURE_WAKEUP;
	struct ts_easy_wakeup_info *info = &g_goodix_dev_data->easy_wakeup_info;

	TS_LOG_INFO("goodix_put_device_into_easy_wakeup  info->easy_wakeup_flag =%x \n",
	     		info->easy_wakeup_flag);
	/*if the sleep_gesture_flag is ture,it presents that  the tp is at sleep state*/

	if (true == info->easy_wakeup_flag) {
		TS_LOG_INFO("goodix_put_device_into_easy_wakeup  info->easy_wakeup_flag =%x \n",
					info->easy_wakeup_flag);
		return NO_ERR;
	}

	/*Wakeup Gesture (d0) set 1 */
	while(i++ < WRITE_REG_GESTURE_RETRY){
		ret = goodix_i2c_write(GTP_REG_GESTURE, &buf,1);
		if (ret < 0){
			TS_LOG_ERR("failed to set doze flag into 0x8046, %d", ret);
			msleep(GTP_SLEEP_TIME_10);
			continue;
		}

		ret = goodix_i2c_write(GTP_REG_CMD, &buf,1);
		if (ret < 0){
			TS_LOG_ERR("failed to set doze flag into 0x8040, %d", ret);
			msleep(GTP_SLEEP_TIME_10);
			continue;
		}

		msleep(GTP_SLEEP_TIME_10);
		enable_irq_wake(g_goodix_dev_data->ts_platform_data->irq_id);
		info->easy_wakeup_flag = true;
		break;
	}

	return ret;
}

/**
 * goodix_switch_wrokmode - Switch working mode.
 * @workmode: GTP_CMD_SLEEP - Sleep mode
 *			  GESTURE_MODE - gesture mode
 * Returns  0--success,non-0--fail.
 */
static int goodix_switch_wrokmode(int wrokmode)
{
	s32 retry = 0;
	u8 cmd;
	switch (wrokmode) {
	case SLEEP_MODE:
		cmd = GTP_CMD_SLEEP;
		goodix_pinctr_int_ouput_low();//gpio_direction_output(irq_gpio, 0);
		msleep(5);
		break;
	case GESTURE_MODE:
		cmd = GTP_CMD_GESTURE_WAKEUP;
		break;
	default:
		return -EINVAL;
	}

	GTP_INFO("Switch working mode[%02X]", cmd);
	while (retry++ < 3) {
		if (!goodix_send_cmd(cmd, 0))
			return 0;
		msleep(20);
	}

	GTP_ERROR("Failed to switch working mode");
	return -1;
}

static int goodix_sleep_mode_out(void)
{
	struct goodix_ts_data *ts = goodix_ts;

	if (NULL == ts){
		TS_LOG_ERR("%s: ts is null point\n", __func__);
		return -EINVAL;
	}

	goodix_pinctrl_select_normal(ts);
	return NO_ERR;
}

static int goodix_put_device_outof_easy_wakeup(void)
{
	struct ts_easy_wakeup_info *info = &g_goodix_dev_data->easy_wakeup_info;

	if (NULL == info) {
		TS_LOG_ERR("%s: info is null point\n", __func__);
		return -EINVAL;
	}
	TS_LOG_INFO("goodix_put_device_outof_easy_wakeup  info->easy_wakeup_flag =%d\n",
				info->easy_wakeup_flag);

	info->easy_wakeup_flag = false;

	return NO_ERR;
}

static int goodix_chip_resume(void)
{
	struct goodix_ts_data *ts = goodix_ts;

	if (ts == NULL)
		return -ENODEV;

	switch (ts->dev_data->easy_wakeup_info.sleep_mode) {
	case TS_POWER_OFF_MODE:
		goodix_sleep_mode_out();
		break;
	case TS_GESTURE_MODE:
		if(true == g_goodix_dev_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value){
			goodix_put_device_outof_easy_wakeup();
		}else{
			goodix_sleep_mode_out();
		}
		break;
	default:
		goodix_sleep_mode_out();
		break;
	}
	goodix_reset_select_addr();

	GTP_INFO("Resume end");
	return 0;
}

static int goodix_after_resume(void *feature_info)
{
	int ret = 0;
	struct goodix_ts_data *ts = goodix_ts;

	TS_LOG_INFO("Enter after_resume\n");
	if (ts == NULL){
		TS_LOG_ERR("%s:goodix_ts is NULL\n",__func__);
		return -ENODEV;
	}
	goodix_int_sync(50);
	ret = goodix_init_watchdog();
	if(ret < 0){
		TS_LOG_ERR("Init watchdog fail\n");
	}
	ts->enter_suspend = false;

	return NO_ERR;
}
static int goodix_before_suspend(void)
{
	return NO_ERR;
}
static int goodix_chip_suspend(void)
{
	struct goodix_ts_data *ts = goodix_ts;

	if (ts == NULL)
		return -ENODEV;

	switch (ts->dev_data->easy_wakeup_info.sleep_mode) {
	case TS_POWER_OFF_MODE:
		goodix_pinctrl_select_suspend(ts);
		goodix_sleep_mode_in();
		break;
	case TS_GESTURE_MODE:
		if(true == g_goodix_dev_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value){
			goodix_put_device_into_easy_wakeup();
		}else{
			goodix_pinctrl_select_suspend(ts);
			goodix_sleep_mode_in();
		}
		break;
	default:
		goodix_pinctrl_select_suspend(ts);
		goodix_sleep_mode_in();
		break;
	}
	ts->enter_suspend = true;
	msleep(GTP_SLEEP_TIME_58);
	GTP_INFO("Suspend end");
	return 0;
}

int goodix_strtolow(char *src_str, size_t size)
{
	char *str = NULL;

	if (NULL == src_str){
		return -EINVAL;
	}

	str = src_str;
	while (*str != '\0' && size > 0) {
		if (*str >= 'A' && *str <= 'Z')
			*str += ('a' - 'A');
		str++;
		size--;
	}

	return 0;
}

/**
 * goodix_fw_update_boot - update firmware while booting
 */
static int goodix_fw_update_boot(char *file_name)
{
	char project_id[GTP_PROJECT_ID_LEN] = {0};
	char vendor_name[GTP_VENDOR_NAME_LEN] = {0};

	if (goodix_ts == NULL || !file_name) {
		GTP_ERROR("Invalid file name");
		return -ENODEV;
	}

	strncpy(vendor_name, goodix_ts->vendor_name, GTP_VENDOR_NAME_LEN - 1);
	strncpy(project_id, goodix_ts->project_id, GTP_PROJECT_ID_LEN-1);

	goodix_strtolow(project_id, GTP_PROJECT_ID_LEN);
	snprintf(goodix_ts->firmware_name, sizeof(goodix_ts->firmware_name),
			"ts/%s%s_%s.img", file_name, project_id, vendor_name);
	TS_LOG_INFO("%s:firmware_name is %s\n",__func__,goodix_ts->firmware_name);

	return gup_update_proc(UPDATE_TYPE_HEADER);
}

/**
 * goodix_fw_update_sd - update firmware from sdcard
 *  firmware path should be '/sdcard/_goodix_update_.bin'
 */
static int goodix_fw_update_sd(void)
{
	if (goodix_ts == NULL)
		return -ENODEV;

	return gup_update_proc(UPDATE_TYPE_FILE);
}

static int goodix_chip_get_info(struct ts_chip_info_param *info)
{
	struct goodix_ts_data *ts = goodix_ts;
	if (!info || !ts)
		return -EINVAL;
	sprintf(info->ic_vendor, "GT%c%c%c%c-%s", ts->hw_info.product_id[0],
		ts->hw_info.product_id[1], ts->hw_info.product_id[2], ts->hw_info.product_id[3],
		ts->project_id);
	sprintf(info->fw_vendor, "%02X", ts->hw_info.patch_id>>8);
	sprintf(info->mod_vendor, "%s", ts->dev_data->module_name);
	info->ttconfig_version = 0x00;
	//info->fw_verctrl_num = 0x00;

	return 0;
}

 void goodix_int_sync(s32 ms)
{
	int irq_gpio;
	irq_gpio = goodix_ts->dev_data->ts_platform_data->irq_gpio;
	goodix_pinctr_int_ouput_low();//gpio_direction_output(irq_gpio, 0);
	msleep(ms);
	goodix_pinctrl_select_normal(goodix_ts);
	gpio_direction_input(irq_gpio);
}
int goodix_reset_select_addr(void)
{
	int reset_gpio;

	reset_gpio = goodix_ts->dev_data->ts_platform_data->reset_gpio;

	gpio_direction_output(reset_gpio, 0);
	msleep(20);
	goodix_pinctr_int_ouput_low();
	msleep(2);
	gpio_direction_output(reset_gpio, 1);
	msleep(6);
	gpio_direction_input(reset_gpio);
	return 0;
}
 /**
 * goodix_chip_reset - reset chip
 */
int goodix_chip_reset(void)
{
	int ret = 0;

	if (goodix_ts == NULL)
		return -ENODEV;
	TS_LOG_INFO("Chip reset");
	goodix_pinctrl_select_normal(goodix_ts);
	goodix_reset_select_addr();
	goodix_int_sync(50);
	ret = goodix_init_watchdog();
	
	return ret;
}

/**
 * goodix_glove_switch - switch to glove mode
 */
static int goodix_glove_switch(struct ts_glove_info *info)
{
	static bool glove_en = false;
	int ret = 0;
	u8 buf = 0;

	if (!info || !goodix_ts) {
		GTP_ERROR("info is Null");
		return -ENOMEM;
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
			ret = goodix_switch_config(GOODIX_GLOVE_CFG);
			if (!ret)
				glove_en = true;
		} else {
			/* disable glove feature */
			ret = goodix_switch_config(GOODIX_NORMAL_CFG);
			if (!ret)
				glove_en = false;
		}

		if (ret < 0)
			GTP_ERROR("set glove switch(%d), failed : %d", buf, ret);
		break;
	default:
		GTP_ERROR("invalid switch status: %d", info->glove_switch);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static void goodix_chip_shutdown(void)
{
	struct goodix_ts_data *ts = goodix_ts;

	if (ts == NULL)
		return;
	return;
}

static int goodix_palm_switch(struct ts_palm_info *info)
{
	if (goodix_ts == NULL)
		return -ENODEV;

	return 0;
}

static int goodix_holster_switch(struct ts_holster_info *info)
{
	int ret = 0;

	if (!info || !goodix_ts) {
		GTP_ERROR("holster_switch: info is Null\n");
		ret = -ENOMEM;
		return ret;
	}

	switch (info->op_action) {
		case TS_ACTION_WRITE:
			if (info->holster_switch)
				ret = goodix_switch_config(GOODIX_HOLSTER_CFG);
			else 
				ret = goodix_switch_config(GOODIX_NORMAL_CFG);
			if (ret < 0)
				GTP_ERROR("set holster switch(%d), failed: %d",
							info->holster_switch, ret);
			break;
		case TS_ACTION_READ:
			GTP_INFO("invalid holster switch(%d) action: TS_ACTION_READ",
							info->holster_switch);
			break;
		default:
			GTP_INFO("invalid holster switch(%d) action: %d\n",
							info->holster_switch, info->op_action);
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int goodix_esdcheck_tp_reset(void)
{
	u8 esd_buf[5] = {0};
	struct goodix_ts_data *ts = goodix_ts;
	int ret = 0;

	esd_buf[0] = GTP_ESD_RESET_VALUE1;
	esd_buf[1] = GTP_ESD_RESET_VALUE2;
	esd_buf[2] = GTP_ESD_RESET_VALUE3;
	esd_buf[3] = GTP_ESD_RESET_VALUE3;
	esd_buf[4] = GTP_ESD_RESET_VALUE3;

	if (ts == NULL){
		TS_LOG_ERR("ts is NULL\n");
		return 0;
	}

	ret = goodix_i2c_write(GTP_REG_CMD, esd_buf, 5);
	if(ret < 0){
		TS_LOG_ERR("%s: goodix_i2c_write  fail\n",__func__);
	}
	msleep(GTP_SLEEP_TIME_50);

	ret = goodix_chip_reset();
	if(ret < 0){
		TS_LOG_ERR("%s: goodix_chip_reset  fail\n",__func__);
	}
	msleep(GTP_SLEEP_TIME_50);

	ret = goodix_send_cfg(&ts->normal_config);
	if(ret < 0){
		TS_LOG_ERR("%s: goodix_send_cfg  fail\n",__func__);
#if defined (CONFIG_HUAWEI_DSM)
		if (!dsm_client_ocuppy(ts_dclient)) {
			TS_LOG_INFO("%s: try to client record DSM_TP_ESD_ERROR_NO(%d) \n", __func__, DSM_TP_ESD_ERROR_NO);
			dsm_client_record(ts_dclient, "try to client record DSM_TP_ESD_ERROR_NO(%d): goodix ESD reset.\n",
						DSM_TP_ESD_ERROR_NO);
			dsm_client_notify(ts_dclient, DSM_TP_ESD_ERROR_NO);
		}
#endif
	}

	return 0;
}

static int goodix_esdcheck_func(void)
{
	struct goodix_ts_data *ts =  goodix_ts;
	u8 esd_buf[4] = {0};
	u8 chk_buf[4] = {0};
	int ret = 0;
	int i = 0;

	esd_buf[0] = GTP_REG_CMD_ADDR1;
	esd_buf[1] = GTP_REG_CMD_ADDR2;
	chk_buf[0] = GTP_REG_CMD_ADDR1;
	chk_buf[1] = GTP_REG_CMD_ADDR2;

	if (ts == NULL){
		TS_LOG_ERR("%s: ts is NULL \n",__func__);
		return 0;
	}

	if (ts->enter_suspend || ts->enter_update || ts->enter_rawtest){
		TS_LOG_INFO("%s: Esd suspended \n",__func__);
		return ret;
	}

	for (i = 0; i < CHECK_HW_STATUS_RETRY; i++){
		ret = goodix_i2c_read(GTP_REG_CMD, esd_buf, 4);
		if (ret < 0){
			/* IIC communication problem */
			TS_LOG_ERR("%s: goodix_i2c_read  fail!\n",__func__);
			continue;
		}else{
			if ((esd_buf[0] == GTP_CMD_ESD_CHECK) || (esd_buf[1] != GTP_CMD_ESD_CHECK)){
				/* ESD check IC works abnormally */
				goodix_i2c_read(GTP_REG_CMD, chk_buf, 4);
				TS_LOG_ERR("%s,%d:[Check]0x8040 = 0x%02X, 0x8041 = 0x%02X",__func__,__LINE__, chk_buf[0], chk_buf[1]);
				if ((chk_buf[0] == GTP_CMD_ESD_CHECK) || (chk_buf[1] != GTP_CMD_ESD_CHECK)){
				    i = CHECK_HW_STATUS_RETRY;
				    break;
				}else{
				    continue;
				}
			}else{
				/* IC works normally, Write 0x8040 0xAA, feed the dog */
				esd_buf[0] = GTP_CMD_ESD_CHECK;
				ret = goodix_i2c_write(GTP_REG_CMD, esd_buf, 2);
				if(ret < 0){
					TS_LOG_ERR("%s: goodix_i2c_write  fail!\n",__func__);
					continue;
				}
				break;
			}
		}
		TS_LOG_DEBUG("[Esd]0x8040 = 0x%02X, 0x8041 = 0x%02X\n", esd_buf[0], esd_buf[1]);
	}

	if (i >= CHECK_HW_STATUS_RETRY){
		TS_LOG_INFO("%s: IC working abnormally! Process reset guitar\n", __func__);
		goodix_esdcheck_tp_reset();
	}

	return 0;
}
/*
 * goodix_check_hw_status - Hw exception checking
 */
static int goodix_check_hw_status(void)
{
	 return goodix_esdcheck_func();
}

#ifdef ROI
static int goodix_roi_switch(struct ts_roi_info *info)
{
	int ret = 0;

	if (!info || !goodix_ts) {
		GTP_ERROR("roi_switch: info is Null");
		ret = -ENOMEM;
		return ret;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		if (info->roi_switch == 1) {
			goodix_ts->roi.enabled = true;
		} else if (info->roi_switch == 0) {
			goodix_ts->roi.enabled = false;
		} else {
			GTP_ERROR("Invalid roi switch value:%d", info->roi_switch);
			ret = -EINVAL;
		}
		break;
	case TS_ACTION_READ:
		break;
	default:
		break;
	}
	return ret;
}

static u8* goodix_roi_rawdata(void)
{
	u8 * rawdata_ptr = NULL;

	if (goodix_ts == NULL)
		return NULL;

	mutex_lock(&goodix_ts->roi.mutex);
	if (goodix_ts->roi.enabled && goodix_ts->roi.data_ready)
		rawdata_ptr = (u8 *)goodix_ts->roi.rawdata;
	mutex_unlock(&goodix_ts->roi.mutex);

	return rawdata_ptr;
}
#endif

static int goodix_chip_get_capacitance_test_type(
				struct ts_test_type_info *info)
{
	int ret = 0;

	if (!info) {
		GTP_ERROR("info is Null");
		ret = -ENOMEM;
		return ret;
	}

	switch (info->op_action) {
	case TS_ACTION_READ:
		memcpy(info->tp_test_type,
		       goodix_ts->dev_data->tp_test_type,
		       TS_CAP_TEST_TYPE_LEN);
		GTP_INFO("test_type=%s",info->tp_test_type);
		break;
	case TS_ACTION_WRITE:
		break;
	default:
		GTP_ERROR("invalid status: %s", info->tp_test_type);
		ret = -EINVAL;
		break;
	}
	return ret;
}


static int 
	goodix_chip_detect(struct ts_kit_platform_data *pdata)
{
	int ret = NO_ERR;
	int slave_addr = 0;
	if (!pdata){
		TS_LOG_ERR("%s device, ts_kit_platform_data *data or data->ts_dev is NULL \n", __func__);
		ret = -ENOMEM;
		goto exit;
	}
	GTP_INFO("Chip detect");

	goodix_ts->pdev = pdata->ts_dev;
	g_goodix_dev_data->ts_platform_data =pdata;
	goodix_ts->dev_data = g_goodix_dev_data;
	goodix_ts->pdev->dev.of_node = g_goodix_dev_data->cnode;
	goodix_ts->ops.i2c_read = goodix_i2c_read;
	goodix_ts->ops.i2c_write = goodix_i2c_write;
	goodix_ts->ops.chip_reset = goodix_chip_reset;
	goodix_ts->ops.send_cmd = goodix_send_cmd;
	goodix_ts->ops.send_cfg = goodix_send_cfg;
	goodix_ts->ops.i2c_read_dbl_check = goodix_i2c_read_dbl_check;
	goodix_ts->ops.read_version = goodix_read_version;
	goodix_ts->ops.parse_cfg_data = goodix_parse_cfg_data;
	goodix_ts->tools_support = true;
	goodix_ts->open_threshold_status = false;
	goodix_ts->rawdiff_mode = false;
	mutex_init(&goodix_ts->mutex_cfg);
	mutex_init(&goodix_ts->mutex_cmd);

	/* Do *NOT* remove these logs */
	GTP_INFO("Driver Version: %s", GTP_DRIVER_VERSION);

	ret = of_property_read_u32(g_goodix_dev_data->cnode , GOODIX_SLAVE_ADDR ,
								&slave_addr);
	pdata->client->addr = slave_addr;
	ret = goodix_pinctrl_init(goodix_ts);
	if (!ret && goodix_ts->pinctrl) {
		/*
		 * Pinctrl handle is optional. If pinctrl handle is found
		 * let pins to be configured in active state. If not
		 * found continue further without error.
		 */
		ret = pinctrl_select_state( goodix_ts->pinctrl,
					goodix_ts->pinctrl_state_active);
		if (ret< 0) {
			GTP_ERROR("failed to select pin to active state %d\n", ret);
		}
	}
	if (ret < 0)
		goto err_get_regs;//goto err_pinctrl_init;

	/* detect chip */
	ret = goodix_chip_reset();
	if (ret < 0)
		goto err_power_on;
	ret = goodix_i2c_test();
	if (ret < 0)
		goto err_power_on;

	gtp_detect_sucess = true;

	return 0;

err_power_on:
	goodix_pinctrl_release(goodix_ts);
err_get_regs:
exit:
	if(NULL != goodix_ts){
		kfree(goodix_ts);
	}
	goodix_ts = NULL;
	return ret;
}
struct ts_device_ops ts_goodix_ops = {
	.chip_detect = goodix_chip_detect,
	.chip_init = goodix_chip_init,
	.chip_parse_config = goodix_chip_parse_config,
	.chip_input_config = goodix_input_config,
	.chip_irq_bottom_half = goodix_irq_bottom_half,
	.chip_reset = goodix_chip_reset,
	.chip_fw_update_boot = goodix_fw_update_boot,
	.chip_fw_update_sd = goodix_fw_update_sd,
	.chip_get_info = goodix_chip_get_info,
//    .chip_set_info_flag = goodix_set_info_flag,
	.chip_before_suspend = goodix_before_suspend,
	.chip_suspend = goodix_chip_suspend,
	.chip_resume = goodix_chip_resume,
	.chip_after_resume = goodix_after_resume,
	.chip_get_rawdata = goodix_get_rawdata,
	.chip_glove_switch = goodix_glove_switch,
	.chip_shutdown = goodix_chip_shutdown,
	.chip_palm_switch = goodix_palm_switch,
	.chip_holster_switch = goodix_holster_switch,
#ifdef ROI
	.chip_roi_switch = goodix_roi_switch,
	.chip_roi_rawdata = goodix_roi_rawdata,
#endif
	.chip_check_status = goodix_check_hw_status,
	.chip_get_capacitance_test_type =
			goodix_chip_get_capacitance_test_type,
//    .chip_regs_operate = goodix_regs_operate,
#if defined (CONFIG_HUAWEI_DSM)
//    .chip_dsm_debug = goodix_dsm_debug,
#endif

#ifdef HUAWEI_TOUCHSCREEN_TEST
//    .chip_test = test_dbg_cmd_test,
#endif
//    .chip_wrong_touch=goodix_wrong_touch,
};
static int __init goodix_ts_module_init(void)
{
	int ret = NO_ERR;
	bool found = false;
	struct device_node *child = NULL;
	struct device_node *root = NULL;

	TS_LOG_INFO("%s: called\n", __func__);

	goodix_ts =
		kzalloc(sizeof(struct goodix_ts_data), GFP_KERNEL);
	if (NULL == goodix_ts) {
		GTP_ERROR("%s:alloc mem for device data fail\n", __func__);
		ret = -ENOMEM;
		goto error_exit;
	}
	
	g_goodix_dev_data =
		kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
	if (NULL == g_goodix_dev_data) {
		GTP_ERROR("%s:alloc mem for ts_kit_device data fail\n", __func__);
		ret = -ENOMEM;
		goto error_exit;
	}
	root = of_find_compatible_node(NULL, NULL, HUAWEI_TS_KIT);
	if (!root) {
		GTP_ERROR("%s:find_compatible_node error\n", __func__);
		ret = -EINVAL;
		goto error_exit;
	}

	for_each_child_of_node(root, child) {
		if (of_device_is_compatible(child, GTP_CHIP_NAME)) {
			found = true;
			break;
		}
	}

	if (!found) {
		GTP_ERROR("%s:device tree node not found, name=%s\n",
			__func__, "goodix");
		ret = -EINVAL;
		goto error_exit;
	}
	g_goodix_dev_data->cnode = child;
	g_goodix_dev_data->ops = &ts_goodix_ops;
	ret = huawei_ts_chip_register(g_goodix_dev_data);
	if (ret) {
		TS_LOG_ERR("%s:chip register fail, ret=%d\n", __func__, ret);
		goto error_exit;
	}

	TS_LOG_INFO("%s:success\n", __func__);
	return 0;

error_exit:
	if(NULL != goodix_ts){
		kfree(goodix_ts);
	}	//kfree(g_goodix_dev_data);
	goodix_ts = NULL;
	TS_LOG_INFO("%s:fail\n", __func__);
	return ret;
}

static void __exit goodix_ts_module_exit(void)
{
	kfree(goodix_ts);
	goodix_ts = NULL;
	return;
}


late_initcall(goodix_ts_module_init);
module_exit(goodix_ts_module_exit);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");
