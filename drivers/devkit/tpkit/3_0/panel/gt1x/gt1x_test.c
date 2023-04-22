/*
 * gt1x_ts_test.c - TP test
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
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include "gt1x.h"
#include "../../huawei_ts_kit.h"

#define SHORT_TO_GND_RESISTER(sig)  (div_s64(5266285, (sig) & (~0x8000)) - 40 * 100)	/* 52662.85/code-40 */
#define SHORT_TO_VDD_RESISTER(sig, value) (div_s64((s64)(36864 * ((value) - 9)) * 100, (((sig) & (~0x8000)) * 7)) - 40 * 100)

#define FLOAT_AMPLIFIER 1000
#define MAX_U16_VALUE	65535
#define RAWDATA_TEST_TIMES	10
#define MAX_ACCEPT_FAILE_CNT	2

#define STATISTICS_DATA_LEN  	32
#define MAX_TEST_ITEMS		10   /* 0P-1P-2P-3P-5P total test items */

#define GTP_CAP_TEST	1
#define GTP_DELTA_TEST	2
#define GTP_NOISE_TEST	3
#define GTP_SHORT_TEST	5

#define GTP_TEST_PASS		1
#define GTP_PANEL_REASON	2
#define SYS_SOFTWARE_REASON	3


#define CSV_TP_UNIFIED_LIMIT       "unified_raw_limit"
#define CSV_TP_NOISE_LIMIT         "noise_data_limit"
#define CSV_TP_SPECIAL_RAW_MIN     "specail_raw_min"
#define CSV_TP_SPECIAL_RAW_MAX     "specail_raw_max"
#define CSV_TP_SPECIAL_RAW_DELTA   "special_raw_delta"
#define CSV_TP_SHORT_THRESHOLD     "shortciurt_threshold"

char *gt1x_strncat(char *dest, char *src, size_t dest_size);
/**
 * struct ts_test_params - test parameters
 * drv_num: touch panel tx(driver) number
 * sen_num: touch panel tx(sensor) number
 * max_limits: max limits of rawdata
 * min_limits: min limits of rawdata
 * deviation_limits: channel deviation limits
 * short_threshold: short resistance threshold
 * r_drv_drv_threshold: resistance threshold between drv and drv
 * r_drv_sen_threshold: resistance threshold between drv and sen
 * r_sen_sen_threshold: resistance threshold between sen and sen
 * r_drv_gnd_threshold: resistance threshold between drv and gnd
 * r_sen_gnd_threshold: resistance threshold between sen and gnd
 * avdd_value: avdd voltage value
 */
struct ts_test_params {
	u32 drv_num;
	u32 sen_num;

	u32 max_limits[MAX_DRV_NUM * MAX_SEN_NUM];
	u32 min_limits[MAX_DRV_NUM * MAX_SEN_NUM];
	u32 deviation_limits[MAX_DRV_NUM * MAX_SEN_NUM];

	u32 noise_threshold;
	u32 short_threshold;
	u32 r_drv_drv_threshold;
	u32 r_drv_sen_threshold;
	u32 r_sen_sen_threshold;
	u32 r_drv_gnd_threshold;
	u32 r_sen_gnd_threshold;
	u32 avdd_value;
};

/**
 * struct ts_test_rawdata - rawdata structure
 * data: rawdata buffer
 * size: rawdata size
 */
struct ts_test_rawdata {
	u16 data[MAX_SEN_NUM * MAX_DRV_NUM];
	u32 size;
};

/**
 * struct gt1x_ts_test - main data structrue
 * ts: gt1x touch screen data
 * test_config: test mode config data
 * orig_config: original config data
 * test_param: test parameters
 * rawdata: raw data structure
 * test_result: test result string
 */
struct gt1x_ts_test {
	struct gt1x_ts_data *ts;
	struct gt1x_ts_config test_config;
	struct gt1x_ts_config orig_config;
	struct ts_test_params test_params;
	struct ts_test_rawdata rawdata;
	struct ts_test_rawdata noisedata;

	/*[0][0][0][0][0]..  0 without test; 1 pass, 2 panel failed; 3 software failed */
	char test_result[MAX_TEST_ITEMS];
	char test_info[TS_RAWDATA_RESULT_MAX];
};

struct gt1x_ts_test *gts_test = NULL;

static int gt1x_init_testlimits(struct gt1x_ts_test *ts_test)
{
	int ret = 0, i;
	char file_path[100] = {0};
	char file_name[GT1X_FW_NAME_LEN] = {0};
	u32 data_buf[MAX_DRV_NUM] = {0};
	struct gt1x_ts_data *ts = ts_test->ts;
	struct ts_test_params *test_params = NULL;

	snprintf(file_name, GT1X_FW_NAME_LEN, "%s_%s_%s_%s_limits.csv", 
		ts->dev_data->ts_platform_data->product_name, GT1X_OF_NAME, 
		ts->project_id, ts->dev_data->module_name);

	#ifdef BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE
	snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/%s", file_name);
	#else
	snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s", file_name);
	#endif
	TS_LOG_INFO("%s: csv_file_path =%s\n", __func__, file_path);

	test_params = &ts_test->test_params;
	/* <max_threshold, min_threshold, delta_threshold> */
	ret = ts_kit_parse_csvfile(file_path, CSV_TP_UNIFIED_LIMIT, data_buf, 1, 3);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_TP_UNIFIED_LIMIT);
		return ret;
	}
	/* store data to test_parms */
	for(i = 0; i < MAX_DRV_NUM * MAX_SEN_NUM; i++) {
		test_params->max_limits[i] = data_buf[0];
		test_params->min_limits[i] = data_buf[1];
		test_params->deviation_limits[i] = data_buf[2];
	}

	ret = ts_kit_parse_csvfile(file_path, CSV_TP_NOISE_LIMIT, data_buf, 1, 1);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_TP_NOISE_LIMIT);
		return ret;
	}
	test_params->noise_threshold = data_buf[0];

	/* shortciurt_threshold <short_threshold,drv_to_drv,
		drv_to_sen,sen_to_sen, drv_to_gnd, sen_to_gnd, avdd_r> */
	ret = ts_kit_parse_csvfile(file_path, CSV_TP_SHORT_THRESHOLD, data_buf, 1, 7);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_TP_SHORT_THRESHOLD);
		return ret;
	}
	test_params->short_threshold = data_buf[0];
	test_params->r_drv_drv_threshold = data_buf[1];
	test_params->r_drv_sen_threshold = data_buf[2];
	test_params->r_sen_sen_threshold = data_buf[3];
	test_params->r_drv_gnd_threshold = data_buf[4];
	test_params->r_sen_gnd_threshold = data_buf[5];
	test_params->avdd_value = data_buf[6];

	ret = ts_kit_parse_csvfile(file_path, CSV_TP_SPECIAL_RAW_MAX, test_params->max_limits,
			test_params->drv_num, test_params->sen_num);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_TP_SPECIAL_RAW_MAX);
		ret = 0;	/* if does not specialed the node value, we will use unified limits setting */
	}

	ret = ts_kit_parse_csvfile(file_path, CSV_TP_SPECIAL_RAW_MIN, test_params->min_limits,
			test_params->drv_num, test_params->sen_num);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_TP_SPECIAL_RAW_MIN);
		ret = 0;
	}

	ret = ts_kit_parse_csvfile(file_path, CSV_TP_SPECIAL_RAW_DELTA, test_params->deviation_limits,
			test_params->drv_num, test_params->sen_num);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_TP_SPECIAL_RAW_DELTA);
		ret = 0;
	}

	return ret;
}

/* gt1x_read_origconfig
 *
 * read original config data
 */
static int gt1x_cache_origconfig(struct gt1x_ts_test *ts_test)
{
	int ret = -ENODEV;
	int cfg_size = 0;
	cfg_size = GTP_CONFIG_ORG_LENGTH;

	if (ts_test->ts->ops.i2c_read) {
		ret = ts_test->ts->ops.i2c_read(GTP_REG_CONFIG_DATA,&ts_test->orig_config.data[0], cfg_size);
		if (ret < 0) {
			TS_LOG_ERR("Failed to read original config data\n");
			return ret;
		}
		if (ts_test->orig_config.data[EXTERN_CFG_OFFSET] & 0x40)  {
			ret = ts_test->ts->ops.i2c_read(GTP_REG_EXT_CONFIG,&ts_test->orig_config.data[cfg_size],
				GTP_CONFIG_EXT_LENGTH);
			if (ret < 0) {
				TS_LOG_ERR("Failed to read external config data\n");
				return ret;
			}
			cfg_size += GTP_CONFIG_EXT_LENGTH;
		}
		ts_test->orig_config.size = cfg_size;
		ts_test->orig_config.delay_ms = SEND_CFG_FLASH;
		strncpy(ts_test->orig_config.name, "original_config", MAX_STR_LEN);
		ts_test->orig_config.initialized = true;
	}

	return ret;
}

/* gt1x_tptest_prepare
 *
 * preparation before tp test
 */
static int gt1x_tptest_prepare(struct gt1x_ts_test *ts_test)
{
	int ret = 0;

	TS_LOG_INFO("TP test preparation\n");
	ret = gt1x_cache_origconfig(ts_test);
	if (ret) {
		TS_LOG_ERR("Failed cache origin config\n");
		return ret;
	}
	/*same config version, ensure firmware will accept this test config data */
	ts_test->ts->test_config.data[0] = ts_test->orig_config.data[0];

	ret = gt1x_get_channel_num(
			&ts_test->test_params.sen_num,
			&ts_test->test_params.drv_num);
	if (ret) {
		TS_LOG_ERR("Failed get channel num:%d\n", ret);
		ts_test->test_params.sen_num = MAX_DRV_NUM;
		ts_test->test_params.drv_num = MAX_SEN_NUM;
		return ret;
	}

	if (true == ts_test->ts->open_threshold_status){
		/* parse test limits from csv */
		ret = gt1x_init_testlimits(ts_test);
		if (ret) {
			TS_LOG_ERR("Failed to init testlimits from csv:%d\n", ret);
			return ret;
		}
		if (ts_test->ts->only_open_once_captest_threshold) {
			ts_test->ts->open_threshold_status = false;
		}
	}
	if (ts_test->ts->ops.send_cfg) {
		ts_test->ts->noise_test_config.data[6] |=0x80;
		ret = ts_test->ts->ops.send_cfg(&ts_test->ts->noise_test_config);
		if (ret) {
			ts_test->ts->noise_test_config.data[6] &=0x7f;
			TS_LOG_ERR("Failed to send noise test config config:%d\n", ret);
			return ret;
		}
		ts_test->ts->noise_test_config.data[6] &=0x7f;
		TS_LOG_INFO("send noise test config success :%d\n", ret);
	}
	return ret;
}

/* gt1x_tptest_finish
 *
 * finish test
 */
static int gt1x_tptest_finish(struct gt1x_ts_test *ts_test)
{
	int ret = RESULT_ERR;

	TS_LOG_INFO("TP test finish\n");
	ret = gt1x_chip_reset();
	if (ret < 0)
		TS_LOG_ERR("%s: chip reset failed\n", __func__);
	if (ts_test->ts->ops.send_cfg) {
		ret = ts_test->ts->ops.send_cfg(&ts_test->orig_config);
		if (ret) {
			TS_LOG_ERR("Failed to send normal config:%d\n", ret);
			return ret;
		}
	} else
		TS_LOG_ERR("%s: send_cfg is NULL, recover origin cfg failed\n", __func__);

	return ret;
}

/**
 * gt1x_cache_rawdata - cache rawdata
 */
static int gt1x_cache_rawdata(struct gt1x_ts_test *ts_test)
{
	int i=0, j;
	int retry;
	int ret = -EINVAL;
	u8 buf[1] = {0x00};
	u32 rawdata_size=0;
	u16 rawdata_addr = 0;

	TS_LOG_DEBUG("Cache rawdata\n");
	rawdata_size = ts_test->test_params.sen_num *
			ts_test->test_params.drv_num;

	if (rawdata_size > MAX_DRV_NUM * MAX_SEN_NUM || rawdata_size <= 0) {
		TS_LOG_ERR("Invalid rawdata size(%u)\n", rawdata_size);
		return ret;
	}
	ts_test->rawdata.size = rawdata_size;
	if (IC_TYPE_9P == ts_test->ts->ic_type)
		rawdata_addr = GTP_RAWDATA_ADDR_9P;
	else
		rawdata_addr = GTP_RAWDATA_ADDR_9PT;
	TS_LOG_INFO("Rawdata address=0x%x\n", rawdata_addr);

	for(j = 0; j< GT1X_RETRY_NUM -1; j++){
		ret = ts_test->ts->ops.send_cmd(GTP_CMD_RAWDATA, 0x00, GT1X_NEED_SLEEP);
		if (ret){
			TS_LOG_ERR("%s:Failed send rawdata cmd:ret%d\n", __func__, ret);
			goto cache_exit;
		}else{
			TS_LOG_INFO("%s: Success change to rawdata mode\n", __func__);
		}
		for (retry = 0;retry < GT1X_RETRY_NUM; retry++) {
			ret = ts_test->ts->ops.i2c_read(GTP_READ_COOR_ADDR, &buf[0], 1);
			if ((ret== 0) && (buf[0] & 0x80) == 0x80) {
				TS_LOG_INFO("Success read rawdata\n");
				break;
			}
			TS_LOG_INFO("Rawdata in not ready:ret=%d, buf[0]=0x%x\n", ret, buf[0]);
			msleep(15);
		}

		if (retry < GT1X_RETRY_NUM) {
			/* read rawdata */
			ret = ts_test->ts->ops.i2c_read(
					rawdata_addr,
					(u8 *)&ts_test->rawdata.data[0],
					rawdata_size * 2);
			if (ret < 0) {
				TS_LOG_ERR("Failed to read rawdata:%d\n", ret);
				goto cache_exit;
			}
			for (i = 0; i < rawdata_size; i++)
				ts_test->rawdata.data[i] =be16_to_cpu(ts_test->rawdata.data[i]);
			TS_LOG_INFO("Rawdata ready\n");
			break;
		} else {
			TS_LOG_ERR("%s : Read rawdata timeout, retry[%d] send command\n", __func__, j);
			ret = -EFAULT;
		}
	}

cache_exit:
	/* clear data ready flag */
	buf[0] = 0x00;
	ts_test->ts->ops.i2c_write(GTP_READ_COOR_ADDR, &buf[0], 1);
	return ret;
}

/* test noise data */
static void gt1x_test_noisedata(struct gt1x_ts_test *ts_test)
{
	int ret = 0, i = 0;
	u32 data_size;
	int test_cnt;
	int fail_cnt;
	u32 find_bad_node = 0;
	u8 buf[1] = {0x00};
	u8 *data_buf;
	u32 noisedata_addr = 0;

	ts_test->noisedata.size = 0;
	data_size = ts_test->test_params.sen_num *
			ts_test->test_params.drv_num;

	if (IC_TYPE_9P == ts_test->ts->ic_type)
		noisedata_addr = GTP_REG_NOISEDATA_9P;
	else
		noisedata_addr = GTP_REG_NOISEDATA_9PT;

	if (data_size <= 0 || data_size > MAX_DRV_NUM * MAX_SEN_NUM) {
		TS_LOG_ERR("%s: Bad data_size[%d]\n", __func__, data_size);
		ts_test->test_result[GTP_NOISE_TEST] = SYS_SOFTWARE_REASON;
		return;
	}
	data_buf = (u8*) kzalloc(data_size, GFP_KERNEL);
	if (!data_buf) {
		TS_LOG_ERR("%s:Failed alloc memory\n", __func__);
		ts_test->test_result[GTP_NOISE_TEST] = SYS_SOFTWARE_REASON;
		return;
	}
	/* change to rawdata mode */
	ret = ts_test->ts->ops.send_cmd(GTP_CMD_RAWDATA, 0x00 ,GT1X_NEED_SLEEP);
	if (ret) {
		TS_LOG_ERR("%s: Failed send rawdata command:ret%d\n", __func__, ret);
		ts_test->test_result[GTP_NOISE_TEST] = SYS_SOFTWARE_REASON;
		goto exit;
	}

	TS_LOG_INFO("%s: Enter rawdata mode\n", __func__);
	fail_cnt = 0;
	for (test_cnt = 0; test_cnt < RAWDATA_TEST_TIMES; test_cnt++) {	
		for (i = 0; i < GT1X_RETRY_NUM; i++) {
			ret = ts_test->ts->ops.i2c_read(GTP_READ_COOR_ADDR, &buf[0], 1);
			if ((ret == 0) && (buf[0] & 0x80))
				break;
			else if (ret) {
				TS_LOG_ERR("%s: failed read noise data status\n", __func__);
				goto soft_err_out;
			}
			msleep(15); /* waiting for noise data ready */
		}

		if (i >= GT1X_RETRY_NUM) {
			fail_cnt++;
			TS_LOG_INFO("%s: wait for noise data timeout, retry[%d] send command\n", __func__, fail_cnt);
			ret = ts_test->ts->ops.send_cmd(GTP_CMD_RAWDATA, 0x00 ,GT1X_NEED_SLEEP);
			if (ret) {
				TS_LOG_ERR("%s: Failed send rawdata command, fail_cnt[%d]\n", __func__, fail_cnt);
				goto soft_err_out;
			}
			continue;
		}

		/* read noise data */
		ret = ts_test->ts->ops.i2c_read(noisedata_addr,
				data_buf, data_size);
		if (!ret) {
			/* check noise data */
			find_bad_node = 0;
			for (i = 0; i < data_size; i++) {
				if (data_buf[i] > ts_test->test_params.noise_threshold) {
					find_bad_node++;
					TS_LOG_ERR("noise check failed: niose[%d][%d]:%u, > %u\n", 
						(u32)div_s64(i, ts_test->test_params.sen_num),
						i % ts_test->test_params.sen_num, data_buf[i],
						ts_test->test_params.noise_threshold);
				}
			}
			if (find_bad_node) {
				TS_LOG_INFO("%s:noise test find bad node, test times=%d\n",__func__, test_cnt );
				fail_cnt++;
			}
		} else {
			TS_LOG_INFO("%s:Failed read noise data\n", __func__);
			goto soft_err_out;
		}
		/* clear data ready flag */
		buf[0] = 0x00;
		ret = ts_test->ts->ops.i2c_write(GTP_READ_COOR_ADDR, &buf[0], 1);
		if (ret) {
			TS_LOG_ERR("%s:failed clear noise data ready register\n", __func__);
			goto soft_err_out;
		}
	}

	if (fail_cnt <= MAX_ACCEPT_FAILE_CNT) {
		ts_test->test_result[GTP_NOISE_TEST] = GTP_TEST_PASS;
	} else {
		TS_LOG_ERR("%s :Noise test failed\n", __func__);
		ts_test->test_result[GTP_NOISE_TEST] = GTP_PANEL_REASON;
	}
	/* cache noise data */
	for (i = 0; i < data_size; i++) {
		ts_test->noisedata.data[i] = (u16)data_buf[i];
		ts_test->noisedata.size = data_size;
	}

	TS_LOG_INFO("%s:Noise test fail_cnt =%d\n", __func__, fail_cnt);
	ret = ts_test->ts->ops.send_cmd(GTP_CMD_NORMAL, 0x00, GT1X_NEED_SLEEP);
	if (ret)
		TS_LOG_ERR("Failed send normal mode cmd:ret%d\n", ret);
	goto exit;

soft_err_out:
	buf[0] = 0x00;
	ts_test->ts->ops.i2c_write(GTP_READ_COOR_ADDR, &buf[0], 1);
	ts_test->noisedata.size = 0;
	ts_test->test_result[GTP_NOISE_TEST] = SYS_SOFTWARE_REASON;
exit:
	if(data_buf){
		TS_LOG_INFO("%s: kfree data_buf.\n", __func__);
		kfree(data_buf);
		data_buf = NULL;
	}
	return;
}


static int gt1x_rawcapacitance_test(struct ts_test_rawdata *rawdata,
		struct ts_test_params *test_params)
{
	int i =0;
	int ret = NO_ERR;

	for (i = 0; i < rawdata->size; i++) {
		if (rawdata->data[i] > test_params->max_limits[i]) {
			TS_LOG_ERR("rawdata[%d][%d]:%u > max_limit:%u, NG\n", 
				(u32)div_s64(i, test_params->sen_num), i % test_params->sen_num,
				rawdata->data[i], test_params->max_limits[i]);
			ret = RESULT_ERR;
		}

		if (rawdata->data[i] < test_params->min_limits[i]) {
			TS_LOG_ERR("rawdata[%d][%d]:%u < min_limit:%u, NG\n",
				(u32)div_s64(i, test_params->sen_num), i % test_params->sen_num,
				rawdata->data[i], test_params->min_limits[i]);
			ret = RESULT_ERR;
		}
	}

	return ret;
}

static int gt1x_deltacapacitance_test(struct ts_test_rawdata *rawdata,
		struct ts_test_params *test_params)
{
	int i=0;
	int ret;
	int cols = 0;
	u32 max_val = 0;
	u32 rawdata_val=0;
	u32 sc_data_num=0;
	u32 up = 0, down = 0, left = 0, right = 0;

	ret = NO_ERR;
	cols = test_params->sen_num;
	sc_data_num = test_params->drv_num * test_params->sen_num;
	if (cols <= 0) {
		TS_LOG_ERR("%s: parmas invalid\n", __func__);
		return RESULT_ERR;
	}

	for (i = 0; i < sc_data_num; i++) {
		rawdata_val = rawdata->data[i];
		max_val = 0;
		/* calculate deltacpacitance with above node */
		if (i - cols >= 0) {
			up = rawdata->data[i - cols];
			up = abs(rawdata_val - up);
			if (up > max_val)
				max_val = up;
		}

		/* calculate deltacpacitance with bellow node */
		if (i + cols < sc_data_num) {
			down = rawdata->data[i + cols];
			down = abs(rawdata_val - down);
			if (down > max_val)
				max_val = down;
		}

		/* calculate deltacpacitance with left node */
		if (i % cols) {
			left = rawdata->data[i - 1];
			left = abs(rawdata_val - left);
			if (left > max_val)
				max_val = left;
		}

		/* calculate deltacpacitance with right node */
		if ((i + 1) % cols) {
			right = rawdata->data[i + 1];
			right = abs(rawdata_val - right);
			if (right > max_val)
				max_val = right;
		}

		/* float to integer */
		if (rawdata_val) {
			max_val *= FLOAT_AMPLIFIER;
			max_val = (u32)div_s64(max_val, rawdata_val);
			if (max_val > test_params->deviation_limits[i]) {
				TS_LOG_ERR("deviation[%d][%d]:%u > delta_limit:%u, NG\n",
					(u32)div_s64(i, cols), i % cols, max_val,
					test_params->deviation_limits[i]);
				ret = RESULT_ERR;
			}
		} else {
			TS_LOG_ERR("Find rawdata=0 when calculate deltacapacitance:[%d][%d]\n",
				(u32)div_s64(i, cols), i % cols);
			ret = RESULT_ERR;
		}
	}

	return ret;
}

/* gt1x_captest_prepare
 *
 * parse test peremeters from dt
 */
static int gt1x_captest_prepare(struct gt1x_ts_test *ts_test)
{
	int ret = -EINVAL;

	if (ts_test->ts->ops.send_cfg) {
		ret = ts_test->ts->ops.send_cfg(&ts_test->ts->test_config);
		if (ret)
			TS_LOG_ERR("Failed to send test config:%d\n", ret);
		else {
			TS_LOG_INFO("Success send test config");
		}
	} else
		TS_LOG_ERR("Ops.send_cfg is NULL\n");

	return ret;
}

static void gt1x_capacitance_test(struct gt1x_ts_test *ts_test)
{
	int ret=0;

	ret = gt1x_captest_prepare(ts_test);
	if (ret) {
		TS_LOG_ERR("Captest prepare failed\n");
		ts_test->test_result[GTP_CAP_TEST] = SYS_SOFTWARE_REASON;
		ts_test->test_result[GTP_DELTA_TEST] = SYS_SOFTWARE_REASON;
		return;
	}

	/* read rawdata and calculate result,  statistics fail times */

	ret = gt1x_cache_rawdata(ts_test);
	if (ret < 0) {
		/* Failed read rawdata */
		TS_LOG_ERR("Read rawdata failed\n");
		ts_test->test_result[GTP_CAP_TEST] = SYS_SOFTWARE_REASON;
		ts_test->test_result[GTP_DELTA_TEST] = SYS_SOFTWARE_REASON;
		return;
	}
	
	ret = gt1x_rawcapacitance_test(&ts_test->rawdata,&ts_test->test_params);
	if (!ret) {
		ts_test->test_result[GTP_CAP_TEST] = GTP_TEST_PASS;
		TS_LOG_INFO("Rawdata test pass\n");
	} else {
		ts_test->test_result[GTP_CAP_TEST] = GTP_PANEL_REASON;
		TS_LOG_ERR("RawCap test failed\n");
	}
	ret = gt1x_deltacapacitance_test(&ts_test->rawdata,&ts_test->test_params);
	if (!ret)  {
		ts_test->test_result[GTP_DELTA_TEST] = GTP_TEST_PASS;
		TS_LOG_INFO("DeltaCap test pass\n");
	} else {
		ts_test->test_result[GTP_DELTA_TEST] = GTP_PANEL_REASON;
		TS_LOG_ERR("DeltaCap test failed\n");
	}

	return;
}

static void gt1x_shortcircut_test(struct gt1x_ts_test *ts_test);
static void gt1x_put_test_result(struct ts_rawdata_info *info, struct gt1x_ts_test *ts_test);

/** gt1x_put_test_failed_prepare_newformat *
* i2c prepare Abnormal failed */
static void gt1x_put_test_failed_prepare_newformat(struct ts_rawdata_info_new *info)
{
	int ret = 0;
	ret = gt1x_strncat(info->i2cinfo, "0F", sizeof(info->i2cinfo));
	if(!ret){
		TS_LOG_ERR("%s: strncat 0F failed.\n", __func__);
	}
	ret = gt1x_strncat(info->i2cerrinfo, "software reason", sizeof(info->i2cerrinfo));
	if(!ret){
		TS_LOG_ERR("%s: strncat software reason failed.\n", __func__);
	}
	return;
}

int gt1x_get_rawdata(struct ts_rawdata_info *info,struct ts_cmd_node *out_cmd)
{
	int ret = 0;

	if (!gt1x_ts || !info){
		TS_LOG_ERR("%s: gt1x_ts is NULL\n", __func__);
		return -ENODEV;
	}

	if (!gts_test) {
		gts_test = kzalloc(sizeof(struct gt1x_ts_test),GFP_KERNEL);
		if (!gts_test) {
			TS_LOG_ERR("%s: Failed to alloc mem\n", __func__);
			return -ENOMEM;
		}
		gts_test->ts = gt1x_ts;
	} else {
		memset(gts_test->test_result, 0, MAX_TEST_ITEMS);
		memset(gts_test->test_info, 0, TS_RAWDATA_RESULT_MAX);
		memset(&(gts_test->rawdata), 0, sizeof(struct ts_test_rawdata));
		memset(&(gts_test->noisedata), 0, sizeof(struct ts_test_rawdata));
	}

	ret = gt1x_tptest_prepare(gts_test);
	if (ret) {
		TS_LOG_ERR("%s: Failed parse test peremeters, exit test\n", __func__);
		if (gt1x_ts->dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT) {
			gt1x_put_test_failed_prepare_newformat((struct ts_rawdata_info_new *)info);
		} else {
			strncpy(info->result, "0F-software reason", TS_RAWDATA_RESULT_MAX -1);
		}
		goto exit_finish;
	}
	TS_LOG_INFO("%s: TP test prepare OK\n", __func__);
	gt1x_test_noisedata(gts_test); /*3F test*/
	gt1x_capacitance_test(gts_test); /* 1F 2F test*/
	gt1x_shortcircut_test(gts_test); /* 5F test */
	gt1x_put_test_result(info, gts_test);
	gt1x_tptest_finish(gts_test);

	return ret;

exit_finish:
	if(gts_test)
		kfree(gts_test);
	gts_test = NULL;
	gt1x_ts->open_threshold_status = true;
	return ret;
}

char *gt1x_strncat(char *dest, char *src, size_t dest_size)
{
	size_t dest_len = 0;

	dest_len = strnlen(dest, dest_size);
	return strncat(&dest[dest_len], src, (dest_size > dest_len ? (dest_size - dest_len - 1) : 0));
}
char *gt1x_strncatint(char * dest, int src, char * format, size_t dest_size)
{
	char src_str[MAX_STR_LEN] = {0};

	snprintf(src_str, MAX_STR_LEN, format, src);
	return gt1x_strncat(dest, src_str, dest_size);
}

/** gt1x_data_statistics
 *
 * catlculate Avg Min Max value of data
 */
static void gt1x_data_statistics(u16 *data, size_t data_size, char *result, size_t res_size)
{
	u16 i=0;
	u16 avg = 0;
	u16 min = 0;
	u16 max = 0;
	long long sum = 0;

	if (!data || !result) {
		TS_LOG_ERR("parameters error please check *data and *result value\n");
		return;
	}

	if (data_size <= 0 || res_size <= 0) {
		TS_LOG_ERR("input parameter is illegva:data_size=%ld, res_size=%ld\n",
			data_size, res_size);
		return;
	}

	min=data[0];
	max=data[0];
		for (i = 0; i < data_size; i++) {
		sum += data[i];
		if (max < data[i])
			max = data[i];
		if (min > data[i])
			min=data[i];
	 }
	avg=div_s64(sum, data_size);
	memset(result, 0, res_size);
	snprintf(result, res_size, "[%d,%d,%d]", avg, max, min);
	return;
}

static void gt1x_put_test_result_newformat(
		struct ts_rawdata_info_new *info,
		struct gt1x_ts_test *ts_test)
{
	int i = 0;
	int have_bus_error = 0;
	int have_panel_error = 0;
	char statistics_data[STATISTICS_DATA_LEN] = {0};
	struct ts_rawdata_newnodeinfo * pts_node = NULL;
	char testresut[]={' ','P','F','F'};

	TS_LOG_INFO("%s :\n",__func__);
	info->tx = ts_test->test_params.sen_num;
	info->rx = ts_test->test_params.drv_num;

	/* i2c info */
	for (i = 0; i < MAX_TEST_ITEMS; i++) {
		if (ts_test->test_result[i] == SYS_SOFTWARE_REASON)
			have_bus_error = 1;
		else if (ts_test->test_result[i] == GTP_PANEL_REASON)
			have_panel_error = 1;
	}

	if (have_bus_error)
		gt1x_strncat(info->i2cinfo, "0F", sizeof(info->i2cinfo));
	else
		gt1x_strncat(info->i2cinfo, "0P", sizeof(info->i2cinfo));

	/********************************************************************/
	/*                                enum ts_raw_data_type
	/*   							  {
	/*   								 RAW_DATA_TYPE_IC = 0,
	/*   #define GTP_CAP_TEST	1        RAW_DATA_TYPE_CAPRAWDATA,
	/*   #define GTP_DELTA_TEST	2        RAW_DATA_TYPE_TrxDelta,
	/*   #define GTP_NOISE_TEST	3        RAW_DATA_TYPE_Noise,
	/*                                   RAW_DATA_TYPE_FreShift,
	/*   #define GTP_SHORT_TEST	5        RAW_DATA_TYPE_OpenShort,
	/*								     RAW_DATA_TYPE_SelfCap,
	/*									 RAW_DATA_TYPE_CbCctest,
	/*									 RAW_DATA_TYPE_highResistance,
	/*									 RAW_DATA_TYPE_SelfNoisetest,
	/*									 RAW_DATA_END,
	/*								  };
	/*  value is same 
	**********************************************************************/
	/* CAP data info */ 

	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc failed\n");
		return;
	}
	if (ts_test->rawdata.size) {
		pts_node->values = kzalloc(ts_test->rawdata.size*sizeof(int), GFP_KERNEL);
		if (!pts_node->values) {
			TS_LOG_ERR("malloc failed  for values\n");
			kfree(pts_node);
			pts_node = NULL;
			return;
		}
		for (i = 0; i < ts_test->rawdata.size; i++)
			pts_node->values[i] = ts_test->rawdata.data[i];
		/* calculate rawdata min avg max vale*/
		gt1x_data_statistics(&ts_test->rawdata.data[0],ts_test->rawdata.size,statistics_data,STATISTICS_DATA_LEN-1);
		strncpy(pts_node->statistics_data,statistics_data,sizeof(pts_node->statistics_data)-1);
	}
	pts_node->size = ts_test->rawdata.size;
	pts_node->testresult = testresut[ts_test->test_result[GTP_CAP_TEST]];
	pts_node->typeindex = GTP_CAP_TEST;
	strncpy(pts_node->test_name,"Cap_Rawdata",sizeof(pts_node->test_name)-1);
	list_add_tail(&pts_node->node, &info->rawdata_head);

	/* DELTA */
	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc failed\n");
		return;
	}
	pts_node->size = 0;
	pts_node->testresult = testresut[ts_test->test_result[GTP_DELTA_TEST]];
	pts_node->typeindex = GTP_DELTA_TEST;
	strncpy(pts_node->test_name,"Trx_delta",sizeof(pts_node->test_name)-1);
	list_add_tail(&pts_node->node, &info->rawdata_head);

	/* save noise data to info->buff */
	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc failed\n");
		return;
	}
	if (ts_test->noisedata.size) {
		pts_node->values = kzalloc(ts_test->noisedata.size*sizeof(int), GFP_KERNEL);
		if (!pts_node->values) {
			TS_LOG_ERR("malloc failed  for values\n");
			kfree(pts_node);
			pts_node = NULL;
			return;
		}
		for (i = 0; i < ts_test->noisedata.size; i++)
			pts_node->values[i] = ts_test->noisedata.data[i];
		/* calculate rawdata min avg max vale*/
		gt1x_data_statistics(&ts_test->noisedata.data[0],ts_test->noisedata.size,statistics_data,STATISTICS_DATA_LEN-1);
		strncpy(pts_node->statistics_data,statistics_data,sizeof(pts_node->statistics_data)-1);
	}
	pts_node->size = ts_test->noisedata.size;
	pts_node->testresult = testresut[ts_test->test_result[GTP_NOISE_TEST]];
	pts_node->typeindex = GTP_NOISE_TEST;
	strncpy(pts_node->test_name,"noise_delta",sizeof(pts_node->test_name)-1);
	list_add_tail(&pts_node->node, &info->rawdata_head);

	/* shortcircut */
	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc failed\n");
		return;
	}
	pts_node->size = 0;
	pts_node->testresult = testresut[ts_test->test_result[GTP_SHORT_TEST]];
	pts_node->typeindex = GTP_SHORT_TEST;
	strncpy(pts_node->test_name,"open_test",sizeof(pts_node->test_name)-1);
	list_add_tail(&pts_node->node, &info->rawdata_head);

	/* dev info */
	gt1x_strncat(info->deviceinfo, "-GT", sizeof(info->deviceinfo));
	gt1x_strncat(info->deviceinfo, ts_test->ts->hw_info.product_id, sizeof(info->deviceinfo)-strlen(info->deviceinfo));
	gt1x_strncat(info->deviceinfo, "-", sizeof(info->deviceinfo)-strlen(info->deviceinfo));
	gt1x_strncatint(info->deviceinfo, ts_test->ts->dev_data->ts_platform_data->panel_id,"%d;",sizeof(info->deviceinfo)-strlen(info->deviceinfo));

	return;
}
static void gt1x_put_test_result(
		struct ts_rawdata_info *info,
		struct gt1x_ts_test *ts_test)
{
	int i = 0;
	int have_bus_error = 0;
	int have_panel_error = 0;
	char statistics_data[STATISTICS_DATA_LEN] = {0};

	if (gt1x_ts->dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
		gt1x_put_test_result_newformat((struct ts_rawdata_info_new *)info,ts_test);
	}else{
		TS_LOG_ERR("%s : Not support old capacitance data format !!\n");
	}

	return ;
}


/* short test */
static int gt1x_short_test_prepare(struct gt1x_ts_test *ts_test)
{
	u8 chksum = 0x00;
	int ret=0, i=0, retry = GT1X_RETRY_NUM;
	u16 drv_offest=0, sen_offest=0;
	u8 data[MAX_DRV_NUM + MAX_SEN_NUM];

	TS_LOG_INFO("Short test prepare+\n");
	while (--retry) {
		/* switch to shrot test system */
		ret = ts_test->ts->ops.send_cmd(0x42, 0x0, GT1X_NEED_SLEEP);  /*bagan test command*/
		if (ret) {
			TS_LOG_ERR("Can not switch to short test system\n");
			return ret;
		}

		/* check firmware running */
		for (i = 0; i < GT1X_RETRY_FIVE; i++) {
			TS_LOG_INFO("Check firmware running..");
			ret = ts_test->ts->ops.i2c_read(SHORT_STATUS_REG, &data[0], 1);   //SHORT_STATUS_REG is 0x5095
			if (ret) {
				TS_LOG_ERR("Check firmware running failed\n");
				return ret;
			} else if (data[0] == 0xaa) {
				TS_LOG_INFO("Short firmware is running\n");
				break;
			}
			msleep(50);
		}
		if (i < GT1X_RETRY_FIVE)
			break;
		else
			ts_test->ts->ops.chip_reset();
	}
	if (retry <= 0) {
		ret = -EINVAL;
		TS_LOG_ERR("Switch to short test mode timeout\n");
		return ret;
	}

	TS_LOG_INFO("Firmware in short test mode\n");
	data[0] = (ts_test->test_params.short_threshold >> 8) & 0xff;
	data[1] = ts_test->test_params.short_threshold & 0xff;
	ret = ts_test->ts->ops.i2c_write(SHORT_THRESHOLD_REG, data, 2);  /* SHORT THRESHOLD_REG 0X8808 */
	if (ret < 0)
		return ret;

	/* ADC Read Delay */
	data[0] = (ADC_RDAD_VALUE >> 8) & 0xff;
	data[1] = ADC_RDAD_VALUE & 0xff;
	ret = ts_test->ts->ops.i2c_write(ADC_READ_DELAY_REG, data, 2);
	if (ret < 0)
		return ret;

	/* DiffCode Short Threshold */
	data[0] = (DIFFCODE_SHORT_VALUE >> 8) & 0xff;
	data[1] = DIFFCODE_SHORT_VALUE & 0xff;
	ret = ts_test->ts->ops.i2c_write(DIFFCODE_SHORT_THRESHOLD, data, 2);
	if (ret) {
		TS_LOG_ERR("Failed writ diff short threshold\n");
		return ret;
	}

	memset(data, 0xFF, sizeof(data));
	if (IC_TYPE_9P == ts_test->ts->ic_type) {
		drv_offest = DRIVER_CH0_REG_9P - GTP_REG_CONFIG_DATA;
		sen_offest = SENSE_CH0_REG_9P - GTP_REG_CONFIG_DATA;
	} else {
		drv_offest = DRIVER_CH0_REG_9PT - GTP_REG_CONFIG_DATA;
		sen_offest = SENSE_CH0_REG_9PT - GTP_REG_CONFIG_DATA;
	}

	for(i = 0; i < ts_test->test_params.sen_num; i++)
		data[i] = ts_test->ts->test_config.data[sen_offest + i];

	for(i = 0; i < ts_test->test_params.drv_num; i++)
		data[32 + i] = ts_test->ts->test_config.data[drv_offest + i];

	for (i = 0; i < sizeof(data); i++)
		chksum += data[i];

	chksum = 0 - chksum;
	ret = ts_test->ts->ops.i2c_write(DRV_CONFIG_REG, &data[MAX_DRV_NUM], MAX_DRV_NUM);
	if (ret) {
		TS_LOG_ERR("Failed writ tp driver config\n");
		return ret;
	}
	ret = ts_test->ts->ops.i2c_write(SENSE_CONFIG_REG, &data[0], MAX_SEN_NUM);
	if (ret) {
		TS_LOG_ERR("Failed writ tp sensor config\n");
		return ret;
	}


	ret = ts_test->ts->ops.i2c_write( DRVSEN_CHECKSUM_REG, &chksum, 1); /* DRVSEN_CHECKSUM_REG   0x884E */
	if (ret) {
		TS_LOG_ERR("Failed write drv checksum\n");
		return ret;
	}

	data[0] = 0x01;
	ret = ts_test->ts->ops.i2c_write(CONFIG_REFRESH_REG, data, 1);	/* CONFIG_REFRESH_REG   0x813E  */
	if (ret) {
		TS_LOG_ERR("Failed write config refresh reg\n");
		return ret;
	}

	/* clr 5095, runing dsp */
	data[0] = 0x00;
	ret = ts_test->ts->ops.i2c_write(SHORT_STATUS_REG, data, 1);     /* SHORT_STATUS_REG 0X5095 */
	if (ret) {
		TS_LOG_ERR("Failed write running dsp reg\n");
		return ret;
	}

	TS_LOG_INFO("Short test prepare-\n");
	return 0;
}

/**
 * gt1x_calc_resisitance - calculate resistance
 * @self_capdata: self-capacitance value
 * @adc_signal: ADC signal value
 * return: resistance value
 */
static inline long gt1x_calc_resisitance(u16 self_capdata, u16 adc_signal)
{
	long ret;

	ret = div_s64(self_capdata * 81 * FLOAT_AMPLIFIER, adc_signal) ;
	ret -= (81 * FLOAT_AMPLIFIER);
	return ret;
}

static int gt1x_check_resistance_to_gnd(struct ts_test_params *test_params,
				u16 adc_signal, u8 pos)
{
	long r=0;
	u16 r_th=0, avdd_value=0;

	avdd_value = test_params->avdd_value;
	if (adc_signal == 0 || adc_signal == 0x8000)
		adc_signal |= 1;

	if ((adc_signal & 0x8000) == 0)	/* short to GND */
		r = SHORT_TO_GND_RESISTER(adc_signal);
	else	/* short to VDD */
		r = SHORT_TO_VDD_RESISTER(adc_signal, avdd_value);
	r *= 2;
	r = (long)div_s64(r, 100);
	r = r > MAX_U16_VALUE ? MAX_U16_VALUE : r;
	r = r < 0 ? 0 : r;

	if (pos < MAX_DRV_NUM)
		r_th = test_params->r_drv_gnd_threshold;
	else
		r_th = test_params->r_sen_gnd_threshold;

	if (r < r_th) {
		if ((adc_signal & (0x8000)) == 0) {
			if (pos < MAX_DRV_NUM)
				TS_LOG_ERR("Tx%d shortcircut to GND,R=%ldK,R_Threshold=%dK\n",
					pos, r, r_th);
			else
				TS_LOG_ERR("Rx%d shortcircut to GND,R=%ldK,R_Threshold=%dK\n",
					pos - MAX_DRV_NUM, r, r_th);
		} else {
			if (pos < MAX_DRV_NUM)
				TS_LOG_ERR("Tx%d shortcircut to VDD,R=%ldK,R_Threshold=%dK\n",
					pos, r, r_th);
			else
				TS_LOG_ERR("Rx%d shortcircut to VDD,R=%ldK,R_Threshold=%dK\n",
					pos - MAX_DRV_NUM, r, r_th);
		}
		return RESULT_ERR;
	}
	return NO_ERR;
}

static int gt1x_check_short_resistance(u16 self_capdata, u16 adc_signal,
				u32 short_r_th)
{
	long r;
	int ret = 0;
	unsigned short short_val;

	if (self_capdata == 0xffff || self_capdata == 0)
		return 0;

	r = gt1x_calc_resisitance(self_capdata, adc_signal);
	r = (long)div_s64(r, FLOAT_AMPLIFIER);

	r = r > MAX_U16_VALUE ? MAX_U16_VALUE : r;
	short_val = (r >= 0 ? r : 0);
	if (short_val < short_r_th) {
		TS_LOG_ERR("Short circut:R=%dK,R_Threshold=%dK\n",
					short_val, short_r_th);
		ret = -EINVAL;
	}

	return ret;
}

static int gt1x_shortcircut_analysis(struct gt1x_ts_test *ts_test)
{
	int ret = 0, err = 0;
	u32 r_threshold=0;
	int size=0, i=0, j=0;
	u16 adc_signal=0, data_addr=0;
	u8 short_flag, *data_buf = NULL, short_status[3]={0};
	u16 self_capdata[MAX_DRV_NUM + MAX_SEN_NUM]={0}, short_pin_num=0;

	ret = ts_test->ts->ops.i2c_read(TEST_RESTLT_REG, &short_flag, 1);  /* TEST_RESTLT_REG  0x8801 */
	if (ret < 0) {
		TS_LOG_ERR("Read TEST_TESULT_REG falied\n");
		goto shortcircut_analysis_error;
	} else if ((short_flag & 0x0F) == 0x00) {
		TS_LOG_INFO("No shortcircut\n");
		return NO_ERR;
	}

	data_buf = kzalloc((MAX_DRV_NUM + MAX_SEN_NUM) * 2, GFP_KERNEL);
	if (!data_buf) {
		TS_LOG_ERR("Failed to alloc memory\n");
		 goto shortcircut_analysis_error;
	}

	/* shortcircut to gnd */
	if (short_flag & 0x08) {
		/* read diff code, diff code will be used to calculate
		  * resistance between channel and GND */
		size = (MAX_DRV_NUM + MAX_SEN_NUM) * 2;
		ret = ts_test->ts->ops.i2c_read(DIFF_CODE_REG, data_buf, size); /* DIFF_CODE_REG   0xA322 */
		if (ret < 0) {
			TS_LOG_ERR("Failed read to-gnd rawdata\n");
			goto shortcircut_analysis_error;
		}
		for (i = 0; i < size; i += 2) {
			adc_signal = be16_to_cpup((__be16*)&data_buf[i]);
			ret = gt1x_check_resistance_to_gnd(&ts_test->test_params,
						adc_signal, i >> 1); /* i >> 1 = i / 2 */
			if (ret) {
				TS_LOG_ERR("Resistance to-gnd test failed\n");
				err |= ret;
			}
		}
	}

	/* read self-capdata+ */
	size = (MAX_DRV_NUM + MAX_SEN_NUM) * 2;
	ret = ts_test->ts->ops.i2c_read(DRV_SELF_CODE_REG, data_buf, size);  /* DRV_SELF_CODE_REG   0xA2A0 */
	if (ret) {
		TS_LOG_ERR("Failed read selfcap rawdata\n");
		goto shortcircut_analysis_error;
	}
	for (i = 0; i < MAX_DRV_NUM + MAX_SEN_NUM; i++)
		self_capdata[i] = be16_to_cpup((__be16*)&data_buf[i * 2]) & 0x7fff;
	/* read self-capdata- */

	/* read tx tx short number */
	ret = ts_test->ts->ops.i2c_read(TX_SHORT_NUM, &short_status[0], 3);  /* TX_SHORT_NUM   0x8802 */
	if (ret) {
		TS_LOG_ERR("Failed read tx-to-tx short rawdata\n");
		goto shortcircut_analysis_error;
	}
	/*   short_status[0]: tr tx
	**   short_status[1]: tr rx
	**   short_status[2]: rx rx
	*/
	TS_LOG_INFO("Tx&Tx:%d,Rx&Rx:%d,Tx&Rx:%d\n",short_status[0],short_status[1],short_status[2]);

	/* drv&drv shortcircut check */
	data_addr = 0x8800 + 0x60;
	for (i = 0; i < short_status[0]; i++) {
		size =   SHORT_CAL_SIZE(MAX_DRV_NUM );						// 4 + MAX_DRV_NUM * 2 + 2;
		ret = ts_test->ts->ops.i2c_read(data_addr, data_buf, size);
		if (ret) {
			TS_LOG_ERR("Failed read drv-to-drv short rawdata\n");
			goto shortcircut_analysis_error;
		}

		r_threshold = ts_test->test_params.r_drv_drv_threshold;
		short_pin_num = be16_to_cpup((__be16 *)&data_buf[0]);
		if (short_pin_num > (MAX_DRV_NUM + MAX_SEN_NUM - 1))
			continue;

		for (j = i + 1; j < MAX_DRV_NUM; j++) {
			adc_signal = be16_to_cpup((__be16 *)&data_buf[4 + j * 2]);
			if (adc_signal > ts_test->test_params.short_threshold) {
				ret = gt1x_check_short_resistance(
						self_capdata[short_pin_num], adc_signal,
						r_threshold);
				if (ret < 0) {
					err |= ret;
					TS_LOG_ERR("Tx%d-Tx%d shortcircut\n", short_pin_num, j);
				}
			}
		}
		data_addr += size;
	}

	/* sen&sen shortcircut check */
	data_addr = 0x9120;
	for (i = 0; i < short_status[1]; i++) {
		size =   SHORT_CAL_SIZE(MAX_SEN_NUM);     // 4 + MAX_SEN_NUM * 2 + 2;
		ret = ts_test->ts->ops.i2c_read(data_addr, data_buf, size);
		if (ret) {
			TS_LOG_ERR("Failed read sen-to-sen short rawdata\n");
			goto shortcircut_analysis_error;
		}

		r_threshold = ts_test->test_params.r_sen_sen_threshold;
		short_pin_num = be16_to_cpup((__be16 *)&data_buf[0]) + MAX_DRV_NUM;
		if (short_pin_num > (MAX_DRV_NUM + MAX_SEN_NUM -1))
			continue;

		for (j = 0; j < MAX_SEN_NUM; j++) {
			if(j == i || (j < i && (j & 0x01) == 0))
				continue;
			adc_signal = be16_to_cpup((__be16 *)&data_buf[4 + j * 2]);
			if (adc_signal > ts_test->test_params.short_threshold) {
				ret = gt1x_check_short_resistance(
						self_capdata[short_pin_num], adc_signal,
						r_threshold);
				if (ret < 0) {
					err |= (u32)ret;
					TS_LOG_ERR("Rx%d-Rx%d shortcircut\n",
							short_pin_num - MAX_DRV_NUM, j);
				}
			}
		}
		data_addr += size;
	}

	/* sen&drv shortcircut check */
	data_addr = 0x99e0;
	for (i = 0; i < short_status[2]; i++) {
		size =  SHORT_CAL_SIZE(MAX_SEN_NUM);                           //size = 4 + MAX_SEN_NUM * 2 + 2;
		ret = ts_test->ts->ops.i2c_read(data_addr, data_buf, size);
		if (ret) {
			TS_LOG_ERR("Failed read sen-to-drv short rawdata\n");
			 goto shortcircut_analysis_error;
		}

		r_threshold = ts_test->test_params.r_drv_sen_threshold;
		short_pin_num = be16_to_cpup((__be16 *)&data_buf[0]) + MAX_DRV_NUM;
		if (short_pin_num > (MAX_DRV_NUM + MAX_SEN_NUM - 1))
			continue;

		for (j = 0; j < MAX_DRV_NUM; j++) {
			adc_signal = be16_to_cpup((__be16 *)&data_buf[4 + j * 2]);
			if (adc_signal > ts_test->test_params.short_threshold) {
				ret = gt1x_check_short_resistance(
						self_capdata[short_pin_num], adc_signal,
						r_threshold);
				if (ret < 0) {
					err |= ret;
					TS_LOG_ERR("Rx%d-Tx%d shortcircut\n",
							short_pin_num - MAX_DRV_NUM, j);
				}
			}
		}
		data_addr += size;
	}

	if(data_buf){
		kfree(data_buf);
		data_buf = NULL;
	}

	return err | (u32)ret ? -EFAULT :  NO_ERR;
shortcircut_analysis_error:
	if(data_buf!=NULL){
		kfree(data_buf);
		data_buf = NULL;
	}
	return -EINVAL;
}

static void gt1x_shortcircut_test(struct gt1x_ts_test *ts_test)
{
	int i = 0;
	int ret=0;
	u8 data[2]={0};

	ts_test->test_result[GTP_SHORT_TEST] = GTP_TEST_PASS;
	ret = gt1x_short_test_prepare(ts_test);
	if (ret < 0){
		TS_LOG_ERR("Failed enter short test mode\n");
		ts_test->test_result[GTP_SHORT_TEST] = SYS_SOFTWARE_REASON;
		return;
	}

	for (i = 0; i < GT1X_RETRY_FIVE; i++) {
		msleep(300);
		TS_LOG_INFO("waitting for short test end...:retry=%d\n", i);
		ret = ts_test->ts->ops.i2c_read(TEST_SHORT_STATUS_REG, data, 1);   /* Test_SHORT_STATUS_REG   0x8800 */
		if (ret)
			TS_LOG_ERR("Failed get short test result: retry%d\n", i);
		else if (data[0] == 0x88)  /* test ok*/
			break;
	}

	if (i < GT1X_RETRY_FIVE) {
		ret = gt1x_shortcircut_analysis(ts_test);
		if (ret){
			ts_test->test_result[GTP_SHORT_TEST] = GTP_PANEL_REASON;
			TS_LOG_ERR("Short test failed\n");
		} else
			TS_LOG_ERR("Short test success\n");
	} else {
		TS_LOG_ERR("Wait short test finish timeout\n");
		ts_test->test_result[GTP_SHORT_TEST] = SYS_SOFTWARE_REASON;
	}
	return;
}

void gt1x_test_free(void)
{
	if(NULL != gts_test){
		kfree(gts_test);
		gts_test = NULL;
	}
}

