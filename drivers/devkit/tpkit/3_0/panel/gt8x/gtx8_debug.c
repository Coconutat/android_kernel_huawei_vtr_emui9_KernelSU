/*
 * gtx8_ts_test.c - TP test
 *
 * Copyright (C) 2015 - 2016 gtx8 Technology Incorporated
 * Copyright (C) 2015 - 2016 Yulong Cai <caiyulong@gtx8.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the gtx8's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include "gtx8.h"



#define SHORT_TO_GND_RESISTER(sig)  (div_s64(5266285, (sig) & (~0x8000)) - 40 * 100)	/* (52662.85/code-40) * 100 */
#define SHORT_TO_VDD_RESISTER(sig, value) (div_s64((s64)(36864 * ((value) - 9)) * 100, (((sig) & (~0x8000)) * 7)) - 40 * 100)

#define FLOAT_AMPLIFIER			1000
#define MAX_U16_VALUE			65535
#define RAWDATA_TEST_TIMES		10
#define MAX_ACCEPT_FAILE_CNT		2

#define STATISTICS_DATA_LEN		32
#define MAX_TEST_ITEMS			10   /* 0P-1P-2P-3P-5P total test items */

#define GTP_CAP_TEST			1
#define GTP_DELTA_TEST			2
#define GTP_NOISE_TEST			3
#define GTP_SHORT_TEST			5
#define GTP_SELFCAP_TEST		6
#define GTP_SELFNOISE_TEST		9

#define GTP_TEST_PASS			1
#define GTP_PANEL_REASON		2
#define SYS_SOFTWARE_REASON		3

#define DRV_CHANNEL_FLAG		0x80

#define TEST_FAIL			0
#define TEST_SUCCESS			1
#define TEST_OVER_LIMIT_TIMES		2
#define CACHE_NOISE_DATA_OK		0
#define DATA_STATUS_READ_RETRY		-1
#define CACHE_NOISE_DATA_FAIL		-2

#define CSV_TP_UNIFIED_LIMIT		"unified_raw_limit"
#define CSV_TP_NOISE_LIMIT		"noise_data_limit"
#define CSV_TP_SPECIAL_RAW_MIN		"specail_raw_min"
#define CSV_TP_SPECIAL_RAW_MAX		"specail_raw_max"
#define CSV_TP_SPECIAL_RAW_DELTA	"special_raw_delta"
#define CSV_TP_SHORT_THRESHOLD		"shortciurt_threshold"
#define CSV_TP_SELF_UNIFIED_LIMIT	"unified_selfraw_limit"
#define CSV_TP_SPECIAL_SELFRAW_MAX	"special_selfraw_max"
#define CSV_TP_SPECIAL_SELFRAW_MIN	"special_selfraw_min"
#define CSV_TP_SELFNOISE_LIMIT		"noise_selfdata_limit"

static u8 gt9886_drv_map[] = {46, 48, 49, 47, 45, 50, 56, 52, 51, 53, 55, 54, 59, 64, 57, 60, 62, 58, 65, 63, 61, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
static u8 gt9886_sen_map[] = {32, 34, 35, 30, 31, 33, 27, 28, 29, 10, 25, 26, 23, 13, 24, 12, 9, 11, 8, 7, 5, 6, 4, 3, 2, 1, 0, 73, 75, 74, 39, 72, 40, 36, 37, 38};

static u8 gt6862_drv_map[] = {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 0, 1, 2, 3, 4, 5, 6};
static u8 gt6862_sen_map[] = {34, 35, 33, 31, 32, 30, 28, 27, 29, 24, 25, 26, 21, 22, 23, 18, 19, 20, 15, 17, 16, 14, 13, 12, 11, 9, 10, 8, 7};
static u8 gt6861_drv_map[] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 56, 46, 47, 48, 49, 45, 50, 52, 51, 55, 53, 54, 64, 59, 57, 60, 62, 58, 65, 63, 61, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
static u8 gt6861_sen_map[] = {72, 37, 38, 40, 36, 74, 39, 75, 0, 73, 2, 1, 3, 4, 5, 6, 7, 8, 10, 11, 9, 12, 13, 24, 23, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};

u16 noisedata_addr = 0;
u16 self_noisedata_addr = 0;
u32 noise_data_size = 0;
u32 self_noise_data_size = 0;
int noise_fail_cnt = 0;
int self_noise_fail_cnt = 0;
int noise_test_flag = TEST_SUCCESS;
int self_noise_test_flag = TEST_SUCCESS;

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
	u16 rawdata_addr;
	u16 noisedata_addr;
	u16 self_rawdata_addr;
	u16 self_noisedata_addr;
	u16 basedata_addr;
	u32 max_drv_num;
	u32 max_sen_num;
	u32 drv_num;
	u32 sen_num;
	u8 *drv_map;
	u8 *sen_map;

	u32 max_limits[MAX_DRV_NUM * MAX_SEN_NUM];
	u32 min_limits[MAX_DRV_NUM * MAX_SEN_NUM];
	u32 deviation_limits[MAX_DRV_NUM * MAX_SEN_NUM];
	u32 self_max_limits[MAX_DRV_NUM + MAX_SEN_NUM];
	u32 self_min_limits[MAX_DRV_NUM + MAX_SEN_NUM];

	u32 noise_threshold;
	u32 self_noise_threshold;
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

struct ts_test_self_rawdata {
	u16 data[MAX_DRV_NUM + MAX_SEN_NUM];
	u32 size;
};

/**
 * struct gtx8_ts_test - main data structrue
 * ts: gtx8 touch screen data
 * test_config: test mode config data
 * orig_config: original config data
 * test_param: test parameters
 * rawdata: raw data structure
 * test_result: test result string
 */
struct gtx8_ts_test {
	struct gtx8_ts_data *ts;
	/* struct gtx8_ts_config test_config; */
	struct gtx8_ts_config orig_config;
	struct ts_test_params test_params;
	struct ts_test_rawdata rawdata;
	struct ts_test_rawdata noisedata;
	struct ts_test_self_rawdata self_rawdata;
	struct ts_test_self_rawdata self_noisedata;

	/*[0][0][0][0][0]..  0 without test; 1 pass, 2 panel failed; 3 software failed */
	char test_result[MAX_TEST_ITEMS];
	char test_info[TS_RAWDATA_RESULT_MAX];
};

struct short_record {
	u32 master;
	u32 slave;
	u16 short_code;
	u8 group1;
	u8 group2;
};

static void gtx8_check_setting_group(struct gtx8_ts_test *ts_test, struct short_record *r_data);

static int doze_awake(struct gtx8_ts_test *ts_test)
{
	int ret = -EINVAL;
	int i = 0;
	u8 r_data = 0;
	u8 w_data = 0;

	w_data = GTX8_DOZE_DISABLE_DATA;
	if (ts_test->ts->ops.write_trans(GTP_REG_DOZE_CTRL, &w_data, 1)) {
		TS_LOG_ERR("doze mode comunition disable FAILED\n");
		goto exit;
	}
	usleep_range(1000, 1100);
	for (i = 0; i < 9; i++) {
		if (ts_test->ts->ops.read_trans(GTP_REG_DOZE_STAT, &r_data, 1)) {
			TS_LOG_ERR("doze mode comunition disable FAILED\n");
			goto exit;
		}
		if (GTX8_DOZE_CLOSE_OK_DATA == r_data) {
			ret = NO_ERR;
			goto exit;
		} else if (0xAA != r_data){
			w_data = GTX8_DOZE_DISABLE_DATA;
			if (ts_test->ts->ops.write_trans(GTP_REG_DOZE_CTRL, &w_data, 1)) {
				TS_LOG_ERR("doze mode comunition disable FAILED\n");
				goto exit;
			}
		}
		usleep_range(10000, 11000);
	}
	TS_LOG_ERR("doze mode disable FAILED\n");

exit:
	return ret;
}

static int gtx8_init_testlimits(struct gtx8_ts_test *ts_test)
{
	int ret = 0, i = 0;
	char file_path[100] = {0};
	char file_name[GTX8_FW_NAME_LEN] = {0};
	u32 data_buf[MAX_DRV_NUM] = {0};
	struct gtx8_ts_data *ts = ts_test->ts;
	struct ts_test_params *test_params = NULL;

	snprintf(file_name, GTX8_FW_NAME_LEN, "%s_%s_%s_%s_limits.csv",
		ts->dev_data->ts_platform_data->product_name, GTX8_OF_NAME,
		ts->project_id, ts->dev_data->module_name);

	/* TODO change default limit file csv to vendor */
	/* snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/%s", file_name); */
	snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s", file_name);
	TS_LOG_INFO("%s: csv_file_path =%s\n", __func__, file_path);

	test_params = &ts_test->test_params;
	/* <max_threshold, min_threshold, delta_threshold> */
	ret = ts_kit_parse_csvfile(file_path, CSV_TP_UNIFIED_LIMIT, data_buf, 1, 3);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s\n", __func__, CSV_TP_UNIFIED_LIMIT);
		return ret;
	}
	/* store data to test_parms */
	for (i = 0; i < MAX_DRV_NUM * MAX_SEN_NUM; i++) {
		test_params->max_limits[i] = data_buf[0];
		test_params->min_limits[i] = data_buf[1];
		test_params->deviation_limits[i] = data_buf[2];
	}

	ret = ts_kit_parse_csvfile(file_path, CSV_TP_NOISE_LIMIT, data_buf, 1, 1);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s\n", __func__, CSV_TP_NOISE_LIMIT);
		return ret;
	}
	test_params->noise_threshold = data_buf[0];

	/* <self max_threshold, min_threshold> */
	ret = ts_kit_parse_csvfile(file_path, CSV_TP_SELF_UNIFIED_LIMIT, data_buf, 1, 2);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s\n", __func__, CSV_TP_SELF_UNIFIED_LIMIT);
		return ret;
	}
	/* store data to test_parms */
	for (i = 0; i < MAX_DRV_NUM + MAX_SEN_NUM; i++) {
		test_params->self_max_limits[i] = data_buf[0];
		test_params->self_min_limits[i] = data_buf[1];
	}
	ret = ts_kit_parse_csvfile(file_path, CSV_TP_SELFNOISE_LIMIT, data_buf, 1, 1);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s\n", __func__, CSV_TP_SELFNOISE_LIMIT);
		return ret;
	}
	test_params->self_noise_threshold = data_buf[0];
	ret = ts_kit_parse_csvfile(file_path, CSV_TP_SPECIAL_SELFRAW_MAX, test_params->self_max_limits,
				1, test_params->drv_num + test_params->sen_num);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s\n", __func__, CSV_TP_SPECIAL_SELFRAW_MAX);
		ret = 0;
	}

	ret = ts_kit_parse_csvfile(file_path, CSV_TP_SPECIAL_SELFRAW_MIN, test_params->self_min_limits,
				1, test_params->drv_num + test_params->sen_num);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s\n", __func__, CSV_TP_SPECIAL_SELFRAW_MIN);
		ret = 0;
	}

	/* shortciurt_threshold <short_threshold,drv_to_drv,
		drv_to_sen,sen_to_sen, drv_to_gnd, sen_to_gnd, avdd_r> */
	ret = ts_kit_parse_csvfile(file_path, CSV_TP_SHORT_THRESHOLD, data_buf, 1, 7);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s\n", __func__, CSV_TP_SHORT_THRESHOLD);
		return ret;
	}
	test_params->short_threshold = data_buf[0];
	test_params->r_drv_drv_threshold = data_buf[1];
	test_params->r_drv_sen_threshold = data_buf[2];
	test_params->r_sen_sen_threshold = data_buf[3];
	test_params->r_drv_gnd_threshold = data_buf[4];
	test_params->r_sen_gnd_threshold = data_buf[5];
	test_params->avdd_value = data_buf[6];
/*
  *RAWDATA CSV VERTICAL SCREEN
  *test_params->drv_num < -> test_params->sen_num Exchange
*/
	ret = ts_kit_parse_csvfile(file_path, CSV_TP_SPECIAL_RAW_MAX, test_params->max_limits,
			test_params->sen_num, test_params->drv_num);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s\n", __func__, CSV_TP_SPECIAL_RAW_MAX);
		ret = 0;	/* if does not specialed the node value, we will use unified limits setting */
	}
	ts_kit_rotate_rawdata_abcd2cbad(test_params->drv_num, test_params->sen_num, test_params->max_limits,\
				GT_RAWDATA_CSV_VERTICAL_SCREEN);

	ret = ts_kit_parse_csvfile(file_path, CSV_TP_SPECIAL_RAW_MIN, test_params->min_limits,
			test_params->sen_num, test_params->drv_num);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s\n", __func__, CSV_TP_SPECIAL_RAW_MIN);
		ret = 0;
	}
	ts_kit_rotate_rawdata_abcd2cbad(test_params->drv_num, test_params->sen_num, test_params->min_limits,\
				GT_RAWDATA_CSV_VERTICAL_SCREEN);

	ret = ts_kit_parse_csvfile(file_path, CSV_TP_SPECIAL_RAW_DELTA, test_params->deviation_limits,
			test_params->sen_num, test_params->drv_num);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s\n", __func__, CSV_TP_SPECIAL_RAW_DELTA);
		ret = 0;
	}
	ts_kit_rotate_rawdata_abcd2cbad(test_params->drv_num, test_params->sen_num, test_params->deviation_limits,\
				GT_RAWDATA_CSV_VERTICAL_SCREEN);

	return ret;
}

static int gtx8_init_params(struct gtx8_ts_test *ts_test)
{
	int ret = 0;
	struct gtx8_ts_data *ts = ts_test->ts;
	struct ts_test_params *test_params = &ts_test->test_params;

	switch (ts->ic_type) {
	case IC_TYPE_9886:
		test_params->rawdata_addr = GTP_RAWDATA_ADDR_9886;
		test_params->noisedata_addr = GTP_NOISEDATA_ADDR_9886;
		test_params->self_rawdata_addr =  GTP_SELF_RAWDATA_ADDR_9886;
		test_params->self_noisedata_addr = GTP_SELF_NOISEDATA_ADDR_9886;
		test_params->basedata_addr = GTP_BASEDATA_ADDR_9886;
		test_params->max_drv_num = MAX_DRV_NUM_9886;
		test_params->max_sen_num = MAX_SEN_NUM_9886;
		test_params->drv_map = gt9886_drv_map;
		test_params->sen_map = gt9886_sen_map;
		break;
	case IC_TYPE_6861:
		test_params->rawdata_addr = GTP_RAWDATA_ADDR_6861;
		test_params->noisedata_addr = GTP_NOISEDATA_ADDR_6861;
		test_params->basedata_addr = GTP_BASEDATA_ADDR_6861;
		test_params->max_drv_num = MAX_DRV_NUM_6861;
		test_params->max_sen_num = MAX_SEN_NUM_6861;
		test_params->drv_map = gt6861_drv_map;
		test_params->sen_map = gt6861_sen_map;
		break;
	case IC_TYPE_6862:
		test_params->rawdata_addr = GTP_RAWDATA_ADDR_6862;
		test_params->noisedata_addr = GTP_NOISEDATA_ADDR_6862;
		test_params->basedata_addr = GTP_BASEDATA_ADDR_6862;
		test_params->max_drv_num = MAX_DRV_NUM_6862;
		test_params->max_sen_num = MAX_SEN_NUM_6862;
		test_params->drv_map = gt6862_drv_map;
		test_params->sen_map = gt6862_sen_map;
		break;
	default:
		TS_LOG_ERR("unsupport ic type:%d\n", ts->ic_type);
		ret = -1;
	}
	return ret;
}

/* gtx8_read_origconfig
 *
 * read original config data
 */
 #define GTX8_CONFIG_REFRESH_DATA 0x01
static int gtx8_cache_origconfig(struct gtx8_ts_test *ts_test)
{
	int ret = -ENODEV;
	u8 checksum = 0;
	if (ts_test->ts->ops.read_cfg) {
		ret = ts_test->ts->ops.read_cfg(&ts_test->orig_config.data[0], 0);
		if (ret < 0) {
			TS_LOG_ERR("Failed to read original config data\n");
			return ret;
		}

		ts_test->orig_config.data[1] = GTX8_CONFIG_REFRESH_DATA;
		checksum = checksum_u8(&ts_test->orig_config.data[0], 3);
		ts_test->orig_config.data[3] = (u8)(0 - checksum);

		mutex_init(&ts_test->orig_config.lock);
		ts_test->orig_config.length = ret;
		ts_test->orig_config.delay = SEND_CFG_FLASH;
		strncpy(ts_test->orig_config.name, "original_config", MAX_STR_LEN);
		ts_test->orig_config.initialized = true;
	}

	return NO_ERR;
}

/* gtx8_tptest_prepare
 *
 * preparation before tp test
 */
static int gtx8_tptest_prepare(struct gtx8_ts_test *ts_test)
{
	int ret = 0;

	TS_LOG_INFO("TP test preparation\n");
	ret = gtx8_cache_origconfig(ts_test);
	if (ret) {
		TS_LOG_ERR("Failed cache origin config\n");
		return ret;
	}

	/* init reg addr and short cal map */
	ret = gtx8_init_params(ts_test);
	if (ret) {
		TS_LOG_ERR("Failed init register address\n");
		return ret;
	}

	/* get sensor and driver num currently in use */
	ret = gtx8_get_channel_num(&ts_test->test_params.sen_num,
				   &ts_test->test_params.drv_num,
				   ts_test->orig_config.data);
	if (ret) {
		TS_LOG_ERR("Failed get channel num:%d\n", ret);
		ts_test->test_params.sen_num = MAX_DRV_NUM;
		ts_test->test_params.drv_num = MAX_SEN_NUM;
		return ret;
	}

	/* parse test limits from csv */
	ret = gtx8_init_testlimits(ts_test);
	if (ret) {
		TS_LOG_ERR("Failed to init testlimits from csv:%d\n", ret);
		return ret;
	}
	if (ts_test->ts->ops.send_cfg) {
		ret = ts_test->ts->ops.send_cfg(&ts_test->ts->noise_test_cfg);
		if (ret) {
			TS_LOG_ERR("Failed to send noise test config config:%d\n", ret);
			return ret;
		}
		TS_LOG_INFO("send noise test config success :%d\n", ret);
	}
	return ret;
}

/* gtx8_tptest_finish
 *
 * finish test
 */
static int gtx8_tptest_finish(struct gtx8_ts_test *ts_test)
{
	int ret = RESULT_ERR;

	TS_LOG_INFO("TP test finish\n");
	ts_test->ts->ops.chip_reset();

	if (doze_awake(ts_test))
		TS_LOG_INFO("WRNING: doze may enabled after reset\n");

	if (ts_test->ts->ops.send_cfg) {
		ret = ts_test->ts->ops.send_cfg(&ts_test->orig_config);
		if (ret)
			TS_LOG_ERR("Failed to send normal config:%d\n", ret);
	}

	return ret;
}

/**
 * gtx8_cache_rawdata - cache rawdata
 */
static int gtx8_cache_rawdata(struct gtx8_ts_test *ts_test)
{
	int i = 0, j = 0;
	int retry = 0;
	int ret = -EINVAL;
	u8 buf[1] = {0x00};
	u32 rawdata_size = 0;
	u16 rawdata_addr = 0;

	TS_LOG_DEBUG("Cache rawdata\n");
	ts_test->rawdata.size = 0;
	rawdata_size = ts_test->test_params.sen_num * ts_test->test_params.drv_num;
	if (rawdata_size > MAX_DRV_NUM * MAX_SEN_NUM || rawdata_size <= 0) {
		TS_LOG_ERR("Invalid rawdata size(%u)\n", rawdata_size);
		return ret;
	}

	rawdata_addr = ts_test->test_params.rawdata_addr;
	TS_LOG_INFO("Rawdata address=0x%x\n", rawdata_addr);

	for (j = 0; j < GTX8_RETRY_NUM_3 - 1; j++) {
		ret = ts_test->ts->ops.send_cmd(GTX8_CMD_RAWDATA, 0x00, GTX8_NEED_SLEEP);
		if (ret) {
			TS_LOG_ERR("%s:Failed send rawdata cmd:ret%d\n", __func__, ret);
			goto cache_exit;
		} else {
			TS_LOG_INFO("%s: Success change to rawdata mode\n", __func__);
		}
		for (retry = 0; retry < GTX8_RETRY_NUM_3; retry++) {
			ret = ts_test->ts->ops.i2c_read(GTP_REG_COOR, &buf[0], 1);
			if ((ret == 0) && (buf[0] & 0x80) == 0x80) {
				TS_LOG_INFO("Success read rawdata\n");
				break;
			}
			TS_LOG_INFO("Rawdata in not ready:ret=%d, buf[0]=0x%x\n", ret, buf[0]);
			msleep(15);
		}

		if (retry < GTX8_RETRY_NUM_3) {
			/* read rawdata */
			ret = ts_test->ts->ops.i2c_read(
					rawdata_addr,
					(u8 *)&ts_test->rawdata.data[0],
					rawdata_size * sizeof(u16));
			if (ret < 0) {
				TS_LOG_ERR("Failed to read rawdata:%d\n", ret);
				goto cache_exit;
			}
			for (i = 0; i < rawdata_size; i++)
				ts_test->rawdata.data[i] = be16_to_cpu(ts_test->rawdata.data[i]);
			ts_test->rawdata.size = rawdata_size;
			TS_LOG_INFO("Rawdata ready\n");
			break;
		} else {
			TS_LOG_ERR("%s : Read rawdata timeout, retry[%d] send command\n", __func__, j);
			ret = -EFAULT;
		}
	}

cache_exit:
	ts_test->ts->ops.send_cmd(GTX8_CMD_NORMAL, 0x00, GTX8_NOT_NEED_SLEEP);
	/* clear data ready flag */
	buf[0] = 0x00;
	ts_test->ts->ops.i2c_write(GTP_REG_COOR, &buf[0], 1);
	return ret;
}


/* test noise data
static void gtx8_test_noisedata(struct gtx8_ts_test *ts_test)
*/
/**
* gtx8_cache_selfrawdata - cache selfrawdata
*/
static int gtx8_cache_self_rawdata(struct gtx8_ts_test *ts_test)
{

	int i = 0, j = 0;
	int retry = 0;
	int ret = -EINVAL;
	u8 buf[1] = {0x00};
	u16 self_rawdata_size =0;
	u16 self_rawdata_addr = 0;
	TS_LOG_DEBUG("Cache selfrawdata\n");
	ts_test->self_rawdata.size = 0;
	self_rawdata_size = ts_test->test_params.sen_num +ts_test->test_params.drv_num;

	if (self_rawdata_size > MAX_DRV_NUM + MAX_SEN_NUM || self_rawdata_size <= 0) {
		TS_LOG_ERR("Invalid selfrawdata size(%u)\n", self_rawdata_size);
		return ret;
	}

	self_rawdata_addr = ts_test->test_params.self_rawdata_addr;
	TS_LOG_INFO("Selfraw address=0x%x\n", self_rawdata_addr);

	for (j = 0; j < GTX8_RETRY_NUM_3 - 1; j++) {
		ret = ts_test->ts->ops.send_cmd(GTX8_CMD_RAWDATA, 0x00, GTX8_NEED_SLEEP);
		if (ret) {
			TS_LOG_ERR("%s:Failed send rawdata cmd:ret%d\n", __func__, ret);
			goto cache_exit;
		} else {
			TS_LOG_INFO("%s: Success change to rawdata mode\n", __func__);
		}
		for (retry = 0; retry < GTX8_RETRY_NUM_3; retry++) {
			ret = ts_test->ts->ops.i2c_read(GTP_REG_COOR, &buf[0], 1);
			if ((ret == 0) && (buf[0] & 0x80) == 0x80) {
				TS_LOG_INFO("Success read rawdata\n");
				break;
			}
			TS_LOG_INFO("Rawdata in not ready:ret=%d, buf[0]=0x%x\n", ret, buf[0]);
			msleep(15);
		}

		if (retry < GTX8_RETRY_NUM_3) {
			/* read selfrawdata */
			ret = ts_test->ts->ops.i2c_read(self_rawdata_addr,
					(u8 *)&ts_test->self_rawdata.data[0],
					self_rawdata_size * sizeof(u16));
			if (ret < 0) {
				TS_LOG_ERR("Failed to read self_rawdata:%d\n", ret);
				goto cache_exit;
			}
			for (i = 0; i < self_rawdata_size; i++)
				ts_test->self_rawdata.data[i] = be16_to_cpu(ts_test->self_rawdata.data[i]);
			ts_test->self_rawdata.size = self_rawdata_size;
			TS_LOG_INFO("self_Rawdata ready\n");
			break;
		} else {
			TS_LOG_ERR("%s : Read self_rawdata timeout, retry[%d] send command\n", __func__, j);
			ret = -EFAULT;
		}
	}

cache_exit:
	ts_test->ts->ops.send_cmd(GTX8_CMD_NORMAL, 0x00, GTX8_NOT_NEED_SLEEP);
	/* clear data ready flag */
	buf[0] = 0x00;
	ts_test->ts->ops.i2c_write(GTP_REG_COOR, &buf[0], 1);
	return ret;
}

/**
 * gtx8_noisetest_prepare- noisetest prepare
 */
static int gtx8_noisetest_prepare(struct gtx8_ts_test *ts_test)
{
	int ret = -EINVAL;

	ts_test->noisedata.size = 0;
	noise_data_size = ts_test->test_params.sen_num * ts_test->test_params.drv_num;

	ts_test->self_noisedata.size = 0;
	self_noise_data_size = ts_test->test_params.sen_num + ts_test->test_params.drv_num;

	if (noise_data_size <= 0 || noise_data_size > MAX_DRV_NUM * MAX_SEN_NUM) {
		TS_LOG_ERR("%s: Bad noise_data_size[%d]\n", __func__, noise_data_size);
		noise_test_flag = TEST_FAIL;
		ts_test->test_result[GTP_NOISE_TEST] = SYS_SOFTWARE_REASON;
	}

	if (self_noise_data_size <= 0 || self_noise_data_size > MAX_DRV_NUM + MAX_SEN_NUM) {
		TS_LOG_ERR("%s: Bad self_noise_data_size[%d]\n", __func__, self_noise_data_size);
		self_noise_test_flag = TEST_FAIL;
		ts_test->test_result[GTP_SELFNOISE_TEST] = SYS_SOFTWARE_REASON;
	}

	if (noise_test_flag == TEST_FAIL && self_noise_test_flag == TEST_FAIL) {
		TS_LOG_ERR("%s: Noise and Selfnoise data size check failed\n", __func__);
		return ret;
	}

	/* change to rawdata mode */
	ret = ts_test->ts->ops.send_cmd(GTX8_CMD_RAWDATA, 0x00 , GTX8_NEED_SLEEP);
	if (ret) {
		TS_LOG_ERR("%s: Failed send rawdata command:ret%d\n", __func__, ret);
		ts_test->test_result[GTP_NOISE_TEST] = SYS_SOFTWARE_REASON;
		ts_test->test_result[GTP_SELFNOISE_TEST] = SYS_SOFTWARE_REASON;
		return ret;
	}

	TS_LOG_INFO("%s: Enter rawdata mode\n", __func__);

	return ret;
}

/**
 * gtx8_cache_noisedata- cache noisedata
 */
static int gtx8_cache_noisedata(struct gtx8_ts_test *ts_test)
{
	int ret = CACHE_NOISE_DATA_OK;
	int i = 0;
	u8 buf[1] = {0};

	noisedata_addr = ts_test->test_params.noisedata_addr;
	self_noisedata_addr = ts_test->test_params.self_noisedata_addr;

	for (i = 0; i < GTX8_RETRY_NUM_3; i++) {
		ret = ts_test->ts->ops.i2c_read(GTP_REG_COOR, &buf[0], 1);
		if ((ret == 0) && (buf[0] & 0x80)) {
			break;
		}else if (ret) {
			TS_LOG_ERR("%s: failed read noise data status\n", __func__);
			goto err_out;
		}
		msleep(15); /* waiting for noise data ready */
	}

	if (i >= GTX8_RETRY_NUM_3) {
		if (noise_test_flag == TEST_SUCCESS)
			noise_fail_cnt++;
		if (self_noise_test_flag == TEST_SUCCESS)
			self_noise_fail_cnt++;
		TS_LOG_INFO("%s: wait for noise data timeout\n", __func__);
		ret = ts_test->ts->ops.send_cmd(GTX8_CMD_RAWDATA, 0x00 , GTX8_NEED_SLEEP);
		if (ret) {
			TS_LOG_ERR("%s: Failed send rawdata command, noise_fail_cnt[%d]\n", __func__, noise_fail_cnt);
			goto err_out;
		}
		return DATA_STATUS_READ_RETRY;
	}

	if (noise_test_flag == TEST_SUCCESS) {
		/* read noise data */
		ret = ts_test->ts->ops.i2c_read(noisedata_addr,
					(u8 *)&ts_test->noisedata.data[0],
					noise_data_size * sizeof(u16));
		if (ret) {
			TS_LOG_ERR("%s: Failed read noise data\n", __func__);
			noise_test_flag = TEST_FAIL;
			ts_test->noisedata.size = 0;
			ts_test->test_result[GTP_NOISE_TEST] = SYS_SOFTWARE_REASON;
		}


	}

	if (self_noise_test_flag == TEST_SUCCESS) {
		/* read self noise data */
		ret = ts_test->ts->ops.i2c_read(self_noisedata_addr,
					(u8 *)&ts_test->self_noisedata.data[0],
					self_noise_data_size * sizeof(u16));
		if (ret) {
			TS_LOG_ERR("%s: Failed read self noise data\n", __func__);
			self_noise_test_flag = TEST_FAIL;
			ts_test->self_noisedata.size = 0;
			ts_test->test_result[GTP_SELFNOISE_TEST] = SYS_SOFTWARE_REASON;
		}


	}
	if ((noise_test_flag == TEST_FAIL) && (self_noise_test_flag == TEST_FAIL)) {
		TS_LOG_ERR("%s noise test fail", __func__);
		return CACHE_NOISE_DATA_FAIL;
	} else {
		buf[0] = 0x00;
		ret = ts_test->ts->ops.i2c_write(GTP_REG_COOR, &buf[0], 1);
		if(ret ) {
			TS_LOG_ERR("%s: i2c_write command fail\n", __func__);
			goto err_out;
		   }
		return CACHE_NOISE_DATA_OK;
	}
err_out:
	noise_test_flag = TEST_FAIL;
	self_noise_test_flag = TEST_FAIL;
	return CACHE_NOISE_DATA_FAIL;
}

/**
 * gtx8_analyse_noisedata- analyse noisedata
 */
static void gtx8_analyse_noisedata(struct gtx8_ts_test *ts_test)
{
	int i = 0;
	u32 find_bad_node = 0;
	u16 noise_value = 0;

	for (i = 0; i < noise_data_size; i++) {
		noise_value = be16_to_cpu(ts_test->noisedata.data[i]);
		ts_test->noisedata.data[i] = abs(noise_value);
		if (ts_test->noisedata.data[i] > ts_test->test_params.noise_threshold) {
			find_bad_node++;
			TS_LOG_ERR("noise check failed: niose[%d][%d]:%u, > %u\n",
				(u32)div_s64(i, ts_test->test_params.sen_num),
				i % ts_test->test_params.sen_num,
				ts_test->noisedata.data[i],
				ts_test->test_params.noise_threshold);
		}
	}
	ts_test->noisedata.size = noise_data_size;

	if (find_bad_node) {
		TS_LOG_INFO("%s:noise test find bad node\n", __func__);
		noise_fail_cnt++;
	}

	if (noise_fail_cnt > MAX_ACCEPT_FAILE_CNT)
		noise_test_flag = TEST_OVER_LIMIT_TIMES;

	return;
}

/**
 * gtx8_analyse_self_noisedata- analyse self noisedata
 */
static void gtx8_analyse_self_noisedata(struct gtx8_ts_test *ts_test)
{
	int i = 0;
	u32 self_find_bad_node = 0;
	u16 self_noise_value = 0;

	for (i = 0; i < self_noise_data_size; i++) {
		self_noise_value = be16_to_cpu(ts_test->self_noisedata.data[i]);
		ts_test->self_noisedata.data[i] = abs(self_noise_value);

		if (ts_test->self_noisedata.data[i] > ts_test->test_params.self_noise_threshold) {
			self_find_bad_node++;
			TS_LOG_ERR("self noise check failed: self_noise[%d][%d]:%u, > %u\n",
				(u32)div_s64(i, ts_test->test_params.drv_num),
				i % ts_test->test_params.drv_num,
				ts_test->self_noisedata.data[i],
				ts_test->test_params.self_noise_threshold);
		}
	}
	ts_test->self_noisedata.size = self_noise_data_size;
	if (self_find_bad_node) {
		TS_LOG_INFO("%s:self_noise test find bad node", __func__);
		self_noise_fail_cnt++;
	}

	if (self_noise_fail_cnt > MAX_ACCEPT_FAILE_CNT)
		self_noise_test_flag = TEST_OVER_LIMIT_TIMES;

	return;
}

/**
 * gtx8_noisedata_save_result- save noisedata result
 */
static void gtx8_noisedata_save_result(struct gtx8_ts_test *ts_test)
{
	int ret = 0;
	u8 buf[1] = {0};

	if (noise_test_flag != TEST_FAIL) {
		if (noise_fail_cnt <= MAX_ACCEPT_FAILE_CNT) {
			ts_test->test_result[GTP_NOISE_TEST] = GTP_TEST_PASS;
		} else {
			TS_LOG_ERR("%s :Noise test failed\n", __func__);
			ts_test->test_result[GTP_NOISE_TEST] = GTP_PANEL_REASON;
		}
		TS_LOG_INFO("%s:Noise test fail_cnt =%d\n", __func__, noise_fail_cnt);
	}

	if (self_noise_test_flag != TEST_FAIL) {
		if (self_noise_fail_cnt <= MAX_ACCEPT_FAILE_CNT) {
			ts_test->test_result[GTP_SELFNOISE_TEST] = GTP_TEST_PASS;
		} else {
			TS_LOG_ERR("%s :Sefl_noise test failed\n", __func__);
			ts_test->test_result[GTP_SELFNOISE_TEST] = GTP_PANEL_REASON;
		}

		TS_LOG_INFO("%s: Self noise test self_fail_cnt =%d\n", __func__, self_noise_fail_cnt);
	}

	ret = ts_test->ts->ops.send_cmd(GTX8_CMD_NORMAL, 0x00, GTX8_NEED_SLEEP);
	if (ret)
		TS_LOG_ERR("Failed send normal mode cmd:ret%d\n", ret);
	/* clear data ready flag */
	buf[0] = 0x00;
	ts_test->ts->ops.i2c_write(GTP_REG_COOR, &buf[0], 1);

	return;
}

/* test noise data */
static void gtx8_test_noisedata(struct gtx8_ts_test *ts_test)
{
	int ret = 0;
	int test_cnt = 0;
	u8 buf[1] = {0};
	noisedata_addr = 0;
	self_noisedata_addr = 0;
	noise_data_size = 0;
	self_noise_data_size = 0;
	noise_fail_cnt = 0;
	self_noise_fail_cnt = 0;
	noise_test_flag = TEST_SUCCESS;
	self_noise_test_flag = TEST_SUCCESS;

	ret = gtx8_noisetest_prepare(ts_test);
	if (ret) {
		TS_LOG_ERR("%s :Noisetest prepare failed\n", __func__);
		return;
	}

	/* read noisedata and self_noisedata,calculate result */
	for (test_cnt = 0; test_cnt < RAWDATA_TEST_TIMES; test_cnt++) {
		ret = gtx8_cache_noisedata(ts_test);
		if (ret == CACHE_NOISE_DATA_FAIL) {
			TS_LOG_ERR("%s: Cache noisedata failed\n", __func__);
			goto soft_err_out;
		} else if (ret == DATA_STATUS_READ_RETRY) {
			TS_LOG_ERR("%s: Cache noisedata retry\n", __func__);
			continue;
		}
		if (noise_test_flag == TEST_SUCCESS)
			gtx8_analyse_noisedata(ts_test);

		if (self_noise_test_flag == TEST_SUCCESS)
			gtx8_analyse_self_noisedata(ts_test);

		if ((noise_test_flag == TEST_OVER_LIMIT_TIMES) && (self_noise_test_flag == TEST_OVER_LIMIT_TIMES))
			break;
	}

	gtx8_noisedata_save_result(ts_test);
	TS_LOG_INFO("%s: Noisedata and Self_noisedata test end\n", __func__);
	return;

soft_err_out:
	ts_test->ts->ops.send_cmd(GTX8_CMD_NORMAL, 0x00, GTX8_NEED_SLEEP);
	/* clear data ready flag */
	buf[0] = 0x00;
	ts_test->ts->ops.i2c_write(GTP_REG_COOR, &buf[0], 1);
	ts_test->noisedata.size = 0;
	ts_test->test_result[GTP_NOISE_TEST] = SYS_SOFTWARE_REASON;
	ts_test->self_noisedata.size = 0;
	ts_test->test_result[GTP_SELFNOISE_TEST] = SYS_SOFTWARE_REASON;
	return;
}


static int gtx8_rawcapacitance_test(struct ts_test_rawdata *rawdata,
		struct ts_test_params *test_params)
{
	int i = 0;
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



static int gtx8_self_rawcapacitance_test(struct ts_test_self_rawdata *rawdata,
		struct ts_test_params *test_params)
{
	int i = 0;
	int ret = NO_ERR;

	for (i = 0; i < rawdata->size; i++) {
		if (rawdata->data[i] > test_params->self_max_limits[i]) {
			TS_LOG_ERR("self_rawdata[%d][%d]:%u >self_max_limit:%u, NG\n",
				(u32)div_s64(i, test_params->drv_num), i % test_params->drv_num,
				rawdata->data[i], test_params->self_max_limits[i]);
			ret = RESULT_ERR;
		}

		if (rawdata->data[i] < test_params->self_min_limits[i]) {
			TS_LOG_ERR("self_rawdata[%d][%d]:%u < min_limit:%u, NG\n",
				(u32)div_s64(i, test_params->drv_num), i % test_params->drv_num,
				rawdata->data[i], test_params->self_min_limits[i]);
			ret = RESULT_ERR;
		}
	}

	return ret;
}

static int gtx8_deltacapacitance_test(struct ts_test_rawdata *rawdata,
		struct ts_test_params *test_params)
{
	int i = 0;
	int ret = NO_ERR;
	int cols = 0;
	u32 max_val = 0;
	u32 rawdata_val = 0;
	u32 sc_data_num = 0;
	u32 up = 0, down = 0, left = 0, right = 0;

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

/* gtx8_captest_prepare
 *
 * parse test peremeters from dt
 */
static int gtx8_captest_prepare(struct gtx8_ts_test *ts_test)
{
	int ret = -EINVAL;

	if (ts_test->ts->ops.send_cfg) {
		ret = ts_test->ts->ops.send_cfg(&ts_test->ts->test_cfg);
		if (ret)
			TS_LOG_ERR("Failed to send test config:%d\n", ret);
		else
			TS_LOG_INFO("Success send test config");
	} else {
		TS_LOG_ERR("Ops.send_cfg is NULL\n");
	}
	return ret;
}

static void gtx8_capacitance_test(struct gtx8_ts_test *ts_test)
{
	int ret = 0;

	ret = gtx8_captest_prepare(ts_test);
	if (ret) {
		TS_LOG_ERR("Captest prepare failed\n");
		ts_test->test_result[GTP_CAP_TEST] = SYS_SOFTWARE_REASON;
		ts_test->test_result[GTP_DELTA_TEST] = SYS_SOFTWARE_REASON;
		ts_test->test_result[GTP_SELFCAP_TEST] = SYS_SOFTWARE_REASON;
		return;
	}

	/* read rawdata and calculate result,  statistics fail times */
	ret = gtx8_cache_rawdata(ts_test);
	if (ret < 0) {
		/* Failed read rawdata */
		TS_LOG_ERR("Read rawdata failed\n");
		ts_test->test_result[GTP_CAP_TEST] = SYS_SOFTWARE_REASON;
		ts_test->test_result[GTP_DELTA_TEST] = SYS_SOFTWARE_REASON;
	} else {
		ret = gtx8_rawcapacitance_test(&ts_test->rawdata, &ts_test->test_params);
		if (!ret) {
			ts_test->test_result[GTP_CAP_TEST] = GTP_TEST_PASS;
			TS_LOG_INFO("Rawdata test pass\n");
		} else {
			ts_test->test_result[GTP_CAP_TEST] = GTP_PANEL_REASON;
			TS_LOG_ERR("RawCap test failed\n");
		}

		ret = gtx8_deltacapacitance_test(&ts_test->rawdata, &ts_test->test_params);
		if (!ret)  {
			ts_test->test_result[GTP_DELTA_TEST] = GTP_TEST_PASS;
			TS_LOG_INFO("DeltaCap test pass\n");
		} else {
			ts_test->test_result[GTP_DELTA_TEST] = GTP_PANEL_REASON;
			TS_LOG_ERR("DeltaCap test failed\n");
		}
	}

	/* read selfrawdata and calculate result,  statistics fail times */
	ret = gtx8_cache_self_rawdata(ts_test);
	if (ret < 0) {
		/* Failed read selfrawdata */
		TS_LOG_ERR("Read selfrawdata failed\n");
		ts_test->test_result[GTP_SELFCAP_TEST] = SYS_SOFTWARE_REASON;
		return;
	} else {
		ret = gtx8_self_rawcapacitance_test(&ts_test->self_rawdata, &ts_test->test_params);
		if (!ret) {
			ts_test->test_result[GTP_SELFCAP_TEST] = GTP_TEST_PASS;
			TS_LOG_INFO("selfrawdata test pass\n");
		} else {
			ts_test->test_result[GTP_SELFCAP_TEST] = GTP_PANEL_REASON;
			TS_LOG_ERR("selfrawCap test failed\n");
		}
	}

	return;
}

static void gtx8_shortcircut_test(struct gtx8_ts_test *ts_test);
static void gtx8_put_test_result(struct ts_rawdata_info *info, struct gtx8_ts_test *ts_test);
static void gtx8_put_test_failed_prepare_newformat(struct ts_rawdata_info_new *info);

int gtx8_get_rawdata(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd)
{
	int ret = 0;
	struct gtx8_ts_test *gts_test = NULL;

	if (!gtx8_ts || !info) {
		TS_LOG_ERR("%s: gtx8_ts is NULL\n", __func__);
		return -ENODEV;
	}

	gts_test = kzalloc(sizeof(struct gtx8_ts_test), GFP_KERNEL);
	if (!gts_test) {
		TS_LOG_ERR("%s: Failed to alloc mem\n", __func__);
		return -ENOMEM;
	}
	gts_test->ts = gtx8_ts;

	ret = gtx8_set_i2c_doze_mode(false);
	if (ret) {
		TS_LOG_ERR("%s: failed disable doze mode\n", __func__);
		goto exit_finish;
	}

	ret = gtx8_tptest_prepare(gts_test);
	if (ret) {
		TS_LOG_ERR("%s: Failed parse test peremeters, exit test\n", __func__);
		if (gtx8_ts->dev_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT) {
			gtx8_put_test_failed_prepare_newformat((struct ts_rawdata_info_new *)info);
		} else {
			strncpy(info->result, "0F-software reason", TS_RAWDATA_RESULT_MAX -1);
		}
		goto exit_finish;
	}
	TS_LOG_INFO("%s: TP test prepare OK\n", __func__);
	gtx8_test_noisedata(gts_test); /*3F 9F test*/
	gtx8_capacitance_test(gts_test); /* 1F 2F 6F test*/
	gtx8_shortcircut_test(gts_test); /* 5F test */
	gtx8_put_test_result(info, gts_test);
	gtx8_tptest_finish(gts_test);

exit_finish:
	if (gts_test) {
		kfree(gts_test);
		gts_test = NULL;
	}
	if (gtx8_set_i2c_doze_mode(true))
		TS_LOG_ERR("%s: failed enable doze mode\n", __func__);
	return ret;
}

void gtx8_strncat(char *dest, char *src, size_t dest_size)
{
	int  dest_len = 0;
	int avaliable_len = 0;
	dest_len = strlen(dest);
	avaliable_len = dest_size  - dest_len -1;
	if(avaliable_len > 0)
		strncat(&dest[dest_len], src, avaliable_len);
	return ;
}
void gtx8_strncatint(char *dest, int src, char *format, size_t dest_size)
{
	char src_str[MAX_STR_LEN] = {0};

	snprintf(src_str, MAX_STR_LEN -1, format, src);
	gtx8_strncat(dest, src_str, dest_size);
	return ;
}

/*
  * Inquire in deviceinfo F Replace f
  */
static void str_Ftof_Inquire(char *strf)
{
	int i = 0;
	int strf_len = 0;
	strf_len = strlen(strf);
	for(i = 0;i < strf_len && i < TS_RAWDATA_DEVINFO_MAX; i++) {
		if(strf[i] == 'F')
			strf[i] = 'f';
	}
	return;
}

/** gtx8_data_statistics
 *
 * catlculate Avg Min Max value of data
 */
static void gtx8_data_statistics(u16 *data, size_t data_size, char *result, size_t res_size)
{
	u16 i = 0;
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

	min = data[0];
	max = data[0];
	for (i = 0; i < data_size; i++) {
		sum += data[i];
		if (max < data[i])
			max = data[i];
		if (min > data[i])
			min = data[i];
	}
	avg = div_s64(sum, data_size);
	memset(result, 0, res_size);
	snprintf(result, res_size, "[%d,%d,%d]", avg, max, min);
	return;
}
/** gtx8_put_test_failed_prepare_newformat
 *
 * i2c prepare Abnormal failed
 */
static void gtx8_put_test_failed_prepare_newformat(struct ts_rawdata_info_new *info){
	gtx8_strncat(info->i2cinfo, "0F", sizeof(info->i2cinfo));
	gtx8_strncat(info->i2cerrinfo, "software reason", sizeof(info->i2cerrinfo));
	return;
}
static void gtx8_put_test_result_newformat(
		struct ts_rawdata_info_new *info,
		struct gtx8_ts_test *ts_test)
{
	int i = 0;
	int tmp_testresult = 0;
	int have_bus_error = 0;
	int have_panel_error = 0;
	char statistics_data[STATISTICS_DATA_LEN] = {0};
	struct ts_rawdata_newnodeinfo * pts_node = NULL;
	char testresut[]={' ','P','F','F'};

	TS_LOG_INFO("%s :\n",__func__);
	info->rx = ts_test->test_params.sen_num;
	info->tx = ts_test->test_params.drv_num;

	/* i2c info */
	for (i = 0; i < MAX_TEST_ITEMS; i++) {
		if (ts_test->test_result[i] == SYS_SOFTWARE_REASON)
			have_bus_error = 1;
		else if (ts_test->test_result[i] == GTP_PANEL_REASON)
			have_panel_error = 1;
	}

	if (have_bus_error)
		gtx8_strncat(info->i2cinfo, "0F", sizeof(info->i2cinfo));
	else
		gtx8_strncat(info->i2cinfo, "0P", sizeof(info->i2cinfo));

	/* CAP data info 1P */
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
		ts_kit_rotate_rawdata_abcd2cbad(info->rx, info->tx, pts_node->values, GT_RAWDATA_CSV_VERTICAL_SCREEN);
		/* calculate rawdata min avg max vale*/
		gtx8_data_statistics(&ts_test->rawdata.data[0],ts_test->rawdata.size,statistics_data,STATISTICS_DATA_LEN-1);
		strncpy(pts_node->statistics_data,statistics_data,sizeof(pts_node->statistics_data)-1);
	}
	pts_node->size = ts_test->rawdata.size;
	tmp_testresult = ts_test->test_result[GTP_CAP_TEST];
	pts_node->testresult = testresut[tmp_testresult];
	pts_node->typeindex = GTP_CAP_TEST;
	strncpy(pts_node->test_name,"Cap_Rawdata",sizeof(pts_node->test_name)-1);
	list_add_tail(&pts_node->node, &info->rawdata_head);

	/* tp test failed reason */
	if (have_bus_error)
		strncpy(pts_node->tptestfailedreason, "software reason", sizeof(pts_node->tptestfailedreason)-1);
	else if(have_panel_error)
		strncpy(pts_node->tptestfailedreason, "panel_reason", sizeof(pts_node->tptestfailedreason)-1);

	/* DELTA  2P*/
	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc failed\n");
		return;
	}
	pts_node->size = 0;
	tmp_testresult = ts_test->test_result[GTP_DELTA_TEST];
	pts_node->testresult = testresut[tmp_testresult];
	pts_node->typeindex = GTP_DELTA_TEST;
	strncpy(pts_node->test_name,"Trx_delta",sizeof(pts_node->test_name)-1);
	list_add_tail(&pts_node->node, &info->rawdata_head);

	/* save noise data to info->buff  3P*/
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
		ts_kit_rotate_rawdata_abcd2cbad(info->rx, info->tx, pts_node->values, GT_RAWDATA_CSV_VERTICAL_SCREEN);
		/* calculate rawdata min avg max vale*/
		gtx8_data_statistics(&ts_test->noisedata.data[0],ts_test->noisedata.size,statistics_data,STATISTICS_DATA_LEN-1);
		strncpy(pts_node->statistics_data,statistics_data,sizeof(pts_node->statistics_data)-1);
	}
	pts_node->size = ts_test->noisedata.size;
	tmp_testresult = ts_test->test_result[GTP_NOISE_TEST];
	pts_node->testresult = testresut[tmp_testresult];
	pts_node->typeindex = GTP_NOISE_TEST;
	strncpy(pts_node->test_name,"noise_delta",sizeof(pts_node->test_name)-1);
	list_add_tail(&pts_node->node, &info->rawdata_head);

	/* shortcircut  5P*/
	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc failed\n");
		return;
	}
	pts_node->size = 0;
	tmp_testresult = ts_test->test_result[GTP_SHORT_TEST];
	pts_node->testresult = testresut[tmp_testresult];
	pts_node->typeindex = GTP_SHORT_TEST;
	strncpy(pts_node->test_name,"open_test",sizeof(pts_node->test_name)-1);
	list_add_tail(&pts_node->node, &info->rawdata_head);
	/*SelfCap 6P*/
	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc failed\n");
		return;
	}
	if (ts_test->self_rawdata.size) {
		pts_node->values = kzalloc(ts_test->self_rawdata.size*sizeof(int), GFP_KERNEL);
		if (!pts_node->values) {
			TS_LOG_ERR("malloc failed  for values\n");
			kfree(pts_node);
			pts_node = NULL;
			return;
		}
		for (i = 0; i < ts_test->self_rawdata.size; i++)
			pts_node->values[i] = ts_test->self_rawdata.data[i];
		gtx8_data_statistics(&ts_test->self_rawdata.data[0],ts_test->self_rawdata.size,statistics_data,STATISTICS_DATA_LEN-1);
		strncpy(pts_node->statistics_data,statistics_data,sizeof(pts_node->statistics_data)-1);
	}
	pts_node->size = ts_test->self_rawdata.size;
	tmp_testresult = ts_test->test_result[GTP_SELFCAP_TEST];
	pts_node->testresult = testresut[tmp_testresult];
	pts_node->typeindex = GTP_SELFCAP_TEST;
	strncpy(pts_node->test_name,"Self_Cap",sizeof(pts_node->test_name)-1);
	list_add_tail(&pts_node->node, &info->rawdata_head);
	/*GTP_SELFNOISE_TEST 9P*/
	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc failed\n");
		return;
	}
	if (ts_test->self_noisedata.size) {
		pts_node->values = kzalloc(ts_test->self_noisedata.size*sizeof(int), GFP_KERNEL);
		if (!pts_node->values) {
			TS_LOG_ERR("malloc failed  for values\n");
			kfree(pts_node);
			pts_node = NULL;
			return;
		}
		for (i = 0; i < ts_test->self_noisedata.size; i++)
			pts_node->values[i] = ts_test->self_noisedata.data[i];
		gtx8_data_statistics(&ts_test->self_noisedata.data[0],ts_test->self_noisedata.size,statistics_data,STATISTICS_DATA_LEN-1);
		strncpy(pts_node->statistics_data,statistics_data,sizeof(pts_node->statistics_data)-1);
	}
	pts_node->size = ts_test->self_noisedata.size;
	tmp_testresult = ts_test->test_result[GTP_SELFNOISE_TEST];
	pts_node->testresult = testresut[tmp_testresult];
	pts_node->typeindex = GTP_SELFNOISE_TEST;
	strncpy(pts_node->test_name,"self_noisse",sizeof(pts_node->test_name)-1);
	list_add_tail(&pts_node->node, &info->rawdata_head);

	/* dev info */
	gtx8_strncat(info->deviceinfo, "-GT", TS_RAWDATA_DEVINFO_MAX);
	gtx8_strncat(info->deviceinfo, ts_test->ts->hw_info.pid, TS_RAWDATA_DEVINFO_MAX);
	gtx8_strncat(info->deviceinfo, "-", TS_RAWDATA_DEVINFO_MAX);
	gtx8_strncat(info->deviceinfo, ts_test->ts->project_id, TS_RAWDATA_DEVINFO_MAX);
	gtx8_strncat(info->deviceinfo, "-", TS_RAWDATA_DEVINFO_MAX);
	gtx8_strncat(info->deviceinfo, ts_test->ts->dev_data->version_name, TS_RAWDATA_DEVINFO_MAX);

	gtx8_strncat(info->deviceinfo, "-", TS_RAWDATA_DEVINFO_MAX);
	gtx8_strncat(info->deviceinfo, "bin_normal-", TS_RAWDATA_DEVINFO_MAX);
	gtx8_strncatint(info->deviceinfo, ts_test->ts->normal_cfg.data[0], "%d", TS_RAWDATA_DEVINFO_MAX);
	gtx8_strncat(info->deviceinfo, ";", TS_RAWDATA_DEVINFO_MAX);
	str_Ftof_Inquire(info->deviceinfo);
	return;
}
static void gtx8_put_test_result(
		struct ts_rawdata_info *info,
		struct gtx8_ts_test *ts_test)
{
	int i = 0;
	int have_bus_error = 0;
	int have_panel_error = 0;
	char statistics_data[STATISTICS_DATA_LEN] = {0};

	if (gtx8_ts->dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
			gtx8_put_test_result_newformat((struct ts_rawdata_info_new *)info,ts_test);
			return;
	}
	/* save rawdata to info->buff */
	info->used_size = 0;

	if (ts_test->rawdata.size) {
		info->buff[0] = ts_test->test_params.sen_num;
		info->buff[1] = ts_test->test_params.drv_num;
		info->hybrid_buff[0] = ts_test->test_params.sen_num;
		info->hybrid_buff[1] = ts_test->test_params.drv_num;
		for (i = 0; i < ts_test->rawdata.size; i++)
			info->buff[i + 2] = ts_test->rawdata.data[i];

		info->used_size += ts_test->rawdata.size + 2;
	}

	/* save noise data to info->buff */
	if (ts_test->noisedata.size) {
		for (i = 0; i < ts_test->noisedata.size; i++)
			info->buff[info->used_size + i] = ts_test->noisedata.data[i];

		info->used_size += ts_test->noisedata.size;
	}

	/* save self_rawdata to info->buff */
	if (ts_test->self_rawdata.size) {
		for (i = 0; i < ts_test->self_rawdata.size; i++)
			info->buff[info->used_size + i] = ts_test->self_rawdata.data[i];

		info->used_size += ts_test->self_rawdata.size;
	}

	/* save self_noisedata to info->buff */
	if (ts_test->self_noisedata.size) {
		for (i = 0; i < ts_test->self_noisedata.size; i++){
			info->buff[info->used_size + i] = ts_test->self_noisedata.data[i];
			}
		info->used_size += ts_test->self_noisedata.size;
	}

	/* check if there have bus error */
	for (i = 0; i < MAX_TEST_ITEMS; i++) {
		if (ts_test->test_result[i] == SYS_SOFTWARE_REASON)
			have_bus_error = 1;
		else if (ts_test->test_result[i] == GTP_PANEL_REASON)
			have_panel_error = 1;
	}
	TS_LOG_INFO("Have bus error:%d", have_bus_error);

	if (have_bus_error)
		gtx8_strncat(ts_test->test_info, "0F-", TS_RAWDATA_RESULT_MAX);
	else
		gtx8_strncat(ts_test->test_info, "0P-", TS_RAWDATA_RESULT_MAX);

	for (i = 1; i < MAX_TEST_ITEMS; i++) {
		/* if have tested, show result */
		if (ts_test->test_result[i]) {
			if (GTP_TEST_PASS == ts_test->test_result[i])
				gtx8_strncatint(ts_test->test_info, i, "%dP-", TS_RAWDATA_RESULT_MAX);
			else
				gtx8_strncatint(ts_test->test_info, i, "%dF-", TS_RAWDATA_RESULT_MAX);
		}
		TS_LOG_INFO("test_result_info [%d]%d\n", i, ts_test->test_result[i]);
	}

	if (ts_test->rawdata.size) {
		/* calculate rawdata min avg max vale*/
		gtx8_data_statistics(
				&ts_test->rawdata.data[0],
				ts_test->rawdata.size,
				statistics_data,
				STATISTICS_DATA_LEN);
		gtx8_strncat(ts_test->test_info, statistics_data, TS_RAWDATA_RESULT_MAX);
	} else {
		TS_LOG_INFO("NO valiable rawdata\n");
		gtx8_strncat(ts_test->test_info, "[0,0,0]", TS_RAWDATA_RESULT_MAX);
	}

	if (ts_test->noisedata.size) {
		/* calculate noise data min avg max vale*/
		gtx8_data_statistics(
				&ts_test->noisedata.data[0],
				ts_test->noisedata.size,
				statistics_data,
				STATISTICS_DATA_LEN);
		gtx8_strncat(ts_test->test_info, statistics_data, TS_RAWDATA_RESULT_MAX);
	} else {
		TS_LOG_INFO("NO valiable noisedata\n");
		gtx8_strncat(ts_test->test_info, "[0,0,0]", TS_RAWDATA_RESULT_MAX);
	}

	if (ts_test->self_rawdata.size) {
		/* calculate self_rawdata min avg max vale*/
		gtx8_data_statistics(
				&ts_test->self_rawdata.data[0],
				ts_test->self_rawdata.size,
				statistics_data,
				STATISTICS_DATA_LEN);
		gtx8_strncat(ts_test->test_info, statistics_data, TS_RAWDATA_RESULT_MAX);
	} else {
		TS_LOG_INFO("NO valiable self_rawdata\n");
		gtx8_strncat(ts_test->test_info, "[0,0,0]", TS_RAWDATA_RESULT_MAX);
	}

	if (ts_test->self_noisedata.size) {
		/* calculate self_noisedata min avg max vale*/
		gtx8_data_statistics(
				&ts_test->self_noisedata.data[0],
				ts_test->self_noisedata.size,
				statistics_data,
				STATISTICS_DATA_LEN);
		gtx8_strncat(ts_test->test_info, statistics_data, TS_RAWDATA_RESULT_MAX);
	} else {
		TS_LOG_INFO("NO valiable self_noisedata\n");
		gtx8_strncat(ts_test->test_info, "[0,0,0]", TS_RAWDATA_RESULT_MAX);
	}

	if (have_bus_error)
		gtx8_strncat(ts_test->test_info, "-software_reason", TS_RAWDATA_RESULT_MAX);
	else if (have_panel_error)
		gtx8_strncat(ts_test->test_info, "-panel_reason", TS_RAWDATA_RESULT_MAX);

	gtx8_strncat(ts_test->test_info, "-GT", TS_RAWDATA_RESULT_MAX);
	gtx8_strncat(ts_test->test_info, ts_test->ts->hw_info.pid, TS_RAWDATA_RESULT_MAX);
	gtx8_strncat(ts_test->test_info, "-", TS_RAWDATA_RESULT_MAX);
	gtx8_strncat(ts_test->test_info, ";", TS_RAWDATA_RESULT_MAX);
	TS_LOG_INFO("ts_test->test_info:%s\n", ts_test->test_info);
	strncpy(info->result, ts_test->test_info, TS_RAWDATA_RESULT_MAX - 1);

	return;
}

/* short test */
static int gtx8_short_test_prepare(struct gtx8_ts_test *ts_test)
{
	int ret = 0, i = 0, retry = GTX8_RETRY_NUM_3;
	u8 data[MAX_DRV_NUM + MAX_SEN_NUM] = {0};

	TS_LOG_INFO("Short test prepare+\n");
	while (--retry) {
		/* switch to shrot test system */
		ret = ts_test->ts->ops.send_cmd(0x0b, 0x0, GTX8_NEED_SLEEP);  /*bagan test command*/
		if (ret) {
			TS_LOG_ERR("Can not switch to short test system\n");
			return ret;
		}

		/* check firmware running */
		for (i = 0; i < 20; i++) {
			TS_LOG_INFO("Check firmware running..");
			ret = ts_test->ts->ops.i2c_read(SHORT_STATUS_REG, &data[0], 1);   /* SHORT_STATUS_REG is 0x5095 */
			if (ret) {
				TS_LOG_ERR("Check firmware running failed\n");
				return ret;
			} else if (data[0] == 0xaa) {
				TS_LOG_INFO("Short firmware is running\n");
				break;
			}
			msleep(10);
		}
		if (i < 20) {
			break;
		} else {
			ts_test->ts->ops.chip_reset();
			if (doze_awake(ts_test))
				TS_LOG_INFO("WRNING: doze may enabled after reset\n");
		}
	}
	if (retry <= 0) {
		ret = -EINVAL;
		TS_LOG_ERR("Switch to short test mode timeout\n");
		return ret;
	}

	data[0] = 0;
	/* turn off watch dog timer */
	ret = ts_test->ts->ops.i2c_write(WATCH_DOG_TIMER_REG, data, 1);
	if (ret < 0) {
		TS_LOG_ERR("Failed turn off watch dog timer\n");
		return ret;
	}

	TS_LOG_INFO("Firmware in short test mode\n");

	data[0] = (ts_test->test_params.short_threshold >> 8) & 0xff;
	data[1] = ts_test->test_params.short_threshold & 0xff;

	/* write tx/tx, tx/rx, rx/rx short threshold value to 0x8408 */
	ret = ts_test->ts->ops.i2c_write(TXRX_THRESHOLD_REG, data, 2);  /* SHORT THRESHOLD_REG 0X8808 */
	if (ret < 0) {
		TS_LOG_ERR("Failed write tx/tx, tx/rx, rx/rx short threshold value\n");
		return ret;
	}
	data[0] = (GNDAVDD_SHORT_VALUE >> 8) & 0xff;
	data[1] = GNDAVDD_SHORT_VALUE & 0xff;
	/* write default txrx/gndavdd short threshold value 16 to 0x804A*/
	ret = ts_test->ts->ops.i2c_write(GNDVDD_THRESHOLD_REG, data, 2);  /* SHORT THRESHOLD_REG 0X8808 */
	if (ret < 0) {
		TS_LOG_ERR("Failed write txrx/gndavdd short threshold value\n");
		return ret;
	}

	/* Write ADC dump data num to 0x840c */
	data[0] = (ADC_DUMP_NUM >> 8) & 0xff;
	data[1] = ADC_DUMP_NUM & 0xff;
	ret = ts_test->ts->ops.i2c_write(ADC_DUMP_NUM_REG, data, 2);
	if (ret < 0) {
		TS_LOG_ERR("Failed write ADC dump data number\n");
		return ret;
	}

	/* write 0x01 to 0x5095 start short test */
	data[0] = 0x01;
	ret = ts_test->ts->ops.i2c_write(SHORT_STATUS_REG, data, 1);     /* SHORT_STATUS_REG 0X5095 */
	if (ret) {
		TS_LOG_ERR("Failed write running dsp reg\n");
		return ret;
	}

	TS_LOG_INFO("Short test prepare-\n");
	return 0;
}

static u32 map_die2pin(struct ts_test_params *test_params, u32 chn_num)
{
	int i = 0;
	u32 res = 255;

	if (chn_num & DRV_CHANNEL_FLAG)
		chn_num = (chn_num & ~DRV_CHANNEL_FLAG) + test_params->max_sen_num;

	for (i = 0; i < test_params->max_sen_num; i++) {
		if (test_params->sen_map[i] == chn_num) {
			res = i;
			break;
		}
	}

	/* res != 255 mean found the corresponding channel num */
	if (res != 255)
		return res;

	/* if cannot find in SenMap try find in DrvMap */
	for (i = 0; i < test_params->max_drv_num; i++) {
		if (test_params->drv_map[i] == chn_num) {
			res = i;
			break;
		}
	}
	if (i >= test_params->max_drv_num)
		TS_LOG_ERR("Faild found corrresponding channel num:%d\n", chn_num);

	return res;
}

static int gtx8_check_resistance_to_gnd(struct ts_test_params *test_params,
				u16 adc_signal, u32 pos)
{
	long r = 0;
	u16 r_th = 0, avdd_value = 0;
	u32 chn_id_tmp = 0;
	u32 pin_num = 0;

	avdd_value = test_params->avdd_value;
	if (adc_signal == 0 || adc_signal == 0x8000)
		adc_signal |= 1;

	if ((adc_signal & 0x8000) == 0)	/* short to GND */
		r = SHORT_TO_GND_RESISTER(adc_signal);
	else	/* short to VDD */
		r = SHORT_TO_VDD_RESISTER(adc_signal, avdd_value);

	r = (long)div_s64(r, 100);
	r = r > MAX_U16_VALUE ? MAX_U16_VALUE : r;
	r = r < 0 ? 0 : r;

	if (pos < MAX_DRV_NUM)
		r_th = test_params->r_drv_gnd_threshold;
	else
		r_th = test_params->r_sen_gnd_threshold;

	chn_id_tmp = pos;
	if (chn_id_tmp < test_params->max_drv_num)
		chn_id_tmp |= DRV_CHANNEL_FLAG;
	else
		chn_id_tmp -= test_params->max_drv_num;

	if (r < r_th) {
		pin_num = map_die2pin(test_params, chn_id_tmp);
		TS_LOG_ERR("%s%d shortcircut to %s,R=%ldK,R_Threshold=%dK\n",
					(pin_num & DRV_CHANNEL_FLAG) ? "DRV" : "SEN",
					(pin_num & ~DRV_CHANNEL_FLAG),
					(adc_signal & 0x8000) ? "VDD" : "GND",
					r, r_th);

		return RESULT_ERR;
	}
	return NO_ERR;
}

#define  SHORT_TEST_SAME_SETTING_PARM    64
#define  SHORT_TEST_DIFF_SETTING_PARM    81
#define SHORT_TEST_MASK  0x80
static u32 gtx8_short_resistance_calc(struct gtx8_ts_test *ts_test,
			struct short_record *r_data, u16 self_capdata, u8 flag)
{
	u16 lineDrvNum = 0, lineSenNum = 0;
	u8 DieNumber1 = 0, DieNumber2 = 0;
	long r = 0;

	lineDrvNum = ts_test->test_params.max_drv_num;
	lineSenNum = ts_test->test_params.max_sen_num;

	if (flag == 0) {
		if (r_data->group1 != r_data->group2) {	/* different Group */
			r = div_s64(self_capdata * SHORT_TEST_DIFF_SETTING_PARM * FLOAT_AMPLIFIER, r_data->short_code);
			r -= (SHORT_TEST_DIFF_SETTING_PARM * FLOAT_AMPLIFIER);
		} else {
			DieNumber1 = ((r_data->master & SHORT_TEST_MASK) == SHORT_TEST_MASK) ? (r_data->master + lineSenNum) : r_data->master;
			DieNumber2 = ((r_data->slave & SHORT_TEST_MASK) == SHORT_TEST_MASK) ? (r_data->slave + lineSenNum) : r_data->slave;
			DieNumber1 = (DieNumber1 >= DieNumber2) ? (DieNumber1 - DieNumber2) : (DieNumber2 - DieNumber1);
			if ((DieNumber1 > 3) && (r_data->group1 == 0)) {
				r = div_s64(self_capdata * SHORT_TEST_DIFF_SETTING_PARM * FLOAT_AMPLIFIER, r_data->short_code);
				r -= (SHORT_TEST_DIFF_SETTING_PARM * FLOAT_AMPLIFIER);
			} else {
				r = div_s64(self_capdata * SHORT_TEST_SAME_SETTING_PARM * FLOAT_AMPLIFIER, r_data->short_code);
				r -= (SHORT_TEST_SAME_SETTING_PARM * FLOAT_AMPLIFIER);
			}
		}
	} else {
		r = div_s64(self_capdata * SHORT_TEST_DIFF_SETTING_PARM * FLOAT_AMPLIFIER, r_data->short_code);
		r -= (SHORT_TEST_DIFF_SETTING_PARM * FLOAT_AMPLIFIER);
	}

	r = (long)div_s64(r, FLOAT_AMPLIFIER);
	r = r > MAX_U16_VALUE ? MAX_U16_VALUE : r;

	if(r >= 0)
		return (u32)r;
	else
		return 0;
}

static int gtx8_shortcircut_analysis(struct gtx8_ts_test *ts_test)
{
	int ret = 0, err = 0;
	u32 r_threshold = 0, short_r = 0;
	int size = 0, i = 0, j = 0;
	u32 master_pin_num = 0, slave_pin_num = 0;
	u16 adc_signal = 0, data_addr = 0;
	u8 short_flag = 0, *data_buf = NULL, short_status[3] = {0};
	u16 self_capdata[MAX_DRV_NUM + MAX_SEN_NUM] = {0}, short_die_num = 0;
	struct short_record temp_short_info;
	struct ts_test_params *test_params = &ts_test->test_params;

	ret = ts_test->ts->ops.i2c_read(TEST_RESULT_REG, &short_flag, 1);  /* TEST_RESULT_REG  0x8401 */
	if (ret < 0) {
		TS_LOG_ERR("Read TEST_RESULT_REG falied\n");
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

	/* shortcircut to gnd&vdd */
	if (short_flag & 0x08) {
		/* read diff code, diff code will be used to calculate
		  * resistance between channel and GND */
		size = (MAX_DRV_NUM + MAX_SEN_NUM) * 2;
		ret = ts_test->ts->ops.i2c_read(DIFF_CODE_REG, data_buf, size); /* DIFF_CODE_REG   0xA97A */
		if (ret < 0) {
			TS_LOG_ERR("Failed read to-gnd rawdata\n");
			goto shortcircut_analysis_error;
		}
		for (i = 0; i < size; i += 2) {
			adc_signal = be16_to_cpup((__be16 *)&data_buf[i]);
			ret = gtx8_check_resistance_to_gnd(test_params,
						adc_signal, i >> 1); /* i >> 1 = i / 2 */
			if (ret) {
				TS_LOG_ERR("Resistance to-gnd test failed\n");
				err = -EINVAL;
			}
		}
	}

	/* read self-capdata+ */
	size = (MAX_DRV_NUM + MAX_SEN_NUM) * 2;
	ret = ts_test->ts->ops.i2c_read(DRV_SELF_CODE_REG, data_buf, size);  /* DRV_SELF_CODE_REG   0xa8e0 */
	if (ret) {
		TS_LOG_ERR("Failed read selfcap rawdata\n");
		goto shortcircut_analysis_error;
	}
	for (i = 0; i < MAX_DRV_NUM + MAX_SEN_NUM; i++)
		self_capdata[i] = be16_to_cpup((__be16 *)&data_buf[i * 2]) & 0x7fff;
	/* read self-capdata- */

	/* read tx tx short number
	**   short_status[0]: tr tx
	**   short_status[1]: tr rx
	**   short_status[2]: rx rx
	*/
	ret = ts_test->ts->ops.i2c_read(TX_SHORT_NUM, &short_status[0], 3);  /* TX_SHORT_NUM   0x8402 */
	if (ret) {
		TS_LOG_ERR("Failed read tx-to-tx short rawdata\n");
		goto shortcircut_analysis_error;
	}
	TS_LOG_INFO("Tx&Tx:%d,Rx&Rx:%d,Tx&Rx:%d\n", short_status[0], short_status[1], short_status[2]);

	/* drv&drv shortcircut check */
	data_addr = 0x8460;
	for (i = 0; i < short_status[0]; i++) {
		size = SHORT_CAL_SIZE(MAX_DRV_NUM);	/* 4 + MAX_DRV_NUM * 2 + 2; */
		ret = ts_test->ts->ops.i2c_read(data_addr, data_buf, size);
		if (ret) {
			TS_LOG_ERR("Failed read drv-to-drv short rawdata\n");
			goto shortcircut_analysis_error;
		}

		r_threshold = test_params->r_drv_drv_threshold;
		short_die_num = be16_to_cpup((__be16 *)&data_buf[0]);
		if (short_die_num > MAX_DRV_NUM + MAX_SEN_NUM ||
			short_die_num < MAX_SEN_NUM) {
			TS_LOG_INFO("invalid short pad num:%d\n", short_die_num);
			continue;
		}

		/* TODO: j start position need recheck */
		if(short_die_num < test_params->max_sen_num){
			TS_LOG_ERR("Failed ! invalid data\n");
			break;
		}
		short_die_num -= test_params->max_sen_num;
		for (j = short_die_num + 1; j < MAX_DRV_NUM; j++) {
			adc_signal = be16_to_cpup((__be16 *)&data_buf[4 + j * 2]);

			if (adc_signal > test_params->short_threshold) {
				temp_short_info.master = short_die_num | DRV_CHANNEL_FLAG;
				temp_short_info.slave = j | DRV_CHANNEL_FLAG;
				temp_short_info.short_code = adc_signal;
				gtx8_check_setting_group(ts_test, &temp_short_info);

				if (self_capdata[short_die_num] == 0xffff ||
					self_capdata[short_die_num] == 0) {
					TS_LOG_INFO("invalid self_capdata:0x%x\n", self_capdata[short_die_num]);
					continue;
				}

				short_r = gtx8_short_resistance_calc(ts_test, &temp_short_info,
							self_capdata[short_die_num], 0);
				if (short_r < r_threshold) {
					master_pin_num = map_die2pin(test_params, temp_short_info.master);
					slave_pin_num = map_die2pin(test_params, temp_short_info.slave);
					TS_LOG_ERR("Tx/Tx short circut:R=%dK,R_Threshold=%dK\n",
								short_r, r_threshold);
					TS_LOG_ERR("%s%d--%s%d shortcircut\n",
							   (master_pin_num & DRV_CHANNEL_FLAG) ? "DRV" : "SEN",
							   (master_pin_num & ~DRV_CHANNEL_FLAG),
							   (slave_pin_num & DRV_CHANNEL_FLAG) ? "DRV" : "SEN",
							   (slave_pin_num & ~DRV_CHANNEL_FLAG));
					err |= -EINVAL;
				}
			}
		}
		data_addr += size;
	}

	/* sen&sen shortcircut check */
	data_addr = 0x91d0;
	for (i = 0; i < short_status[1]; i++) {
		size =   SHORT_CAL_SIZE(MAX_SEN_NUM);     /* 4 + MAX_SEN_NUM * 2 + 2; */
		ret = ts_test->ts->ops.i2c_read(data_addr, data_buf, size);
		if (ret) {
			TS_LOG_ERR("Failed read sen-to-sen short rawdata\n");
			goto shortcircut_analysis_error;
		}

		r_threshold = ts_test->test_params.r_sen_sen_threshold;
		short_die_num = be16_to_cpup((__be16 *)&data_buf[0]);
		if (short_die_num > MAX_SEN_NUM)
			continue;

		for (j = short_die_num + 1; j < MAX_SEN_NUM; j++) {
			adc_signal = be16_to_cpup((__be16 *)&data_buf[4 + j * 2]);
			if (adc_signal > ts_test->test_params.short_threshold) {
				temp_short_info.master = short_die_num;
				temp_short_info.slave = j;
				temp_short_info.short_code = adc_signal;
				gtx8_check_setting_group(ts_test, &temp_short_info);

				if (self_capdata[short_die_num + test_params->max_drv_num] == 0xffff ||
				    self_capdata[short_die_num + test_params->max_drv_num] == 0) {
					TS_LOG_INFO("invalid self_capdata:0x%x\n",
						self_capdata[short_die_num + test_params->max_drv_num]);
					continue;
				}

				short_r = gtx8_short_resistance_calc(ts_test, &temp_short_info,
							self_capdata[short_die_num + test_params->max_drv_num], 0);
				if (short_r < r_threshold) {
					master_pin_num = map_die2pin(test_params, temp_short_info.master);
					slave_pin_num = map_die2pin(test_params, temp_short_info.slave);
					TS_LOG_ERR("Rx/Rx short circut:R=%dK,R_Threshold=%dK\n",
								short_r, r_threshold);
					TS_LOG_ERR("%s%d--%s%d shortcircut\n",
							   (master_pin_num & DRV_CHANNEL_FLAG) ? "DRV" : "SEN",
							   (master_pin_num & ~DRV_CHANNEL_FLAG),
							   (slave_pin_num & DRV_CHANNEL_FLAG) ? "DRV" : "SEN",
							   (slave_pin_num & ~DRV_CHANNEL_FLAG));
					err |= -EINVAL;
				}
			}
		}
		data_addr += size;
	}

	/* sen&drv shortcircut check */
	data_addr = 0x9cc8;
	for (i = 0; i < short_status[2]; i++) {
		size =  SHORT_CAL_SIZE(MAX_DRV_NUM);                        /* size = 4 + MAX_SEN_NUM * 2 + 2; */
		ret = ts_test->ts->ops.i2c_read(data_addr, data_buf, size);
		if (ret) {
			TS_LOG_ERR("Failed read sen-to-drv short rawdata\n");
			 goto shortcircut_analysis_error;
		}

		r_threshold = ts_test->test_params.r_drv_sen_threshold;
		short_die_num = be16_to_cpup((__be16 *)&data_buf[0]);
		if (short_die_num > MAX_SEN_NUM)
			continue;

		for (j = 0; j < MAX_DRV_NUM; j++) {
			adc_signal = be16_to_cpup((__be16 *)&data_buf[4 + j * 2]);
			if (adc_signal > ts_test->test_params.short_threshold) {
				temp_short_info.master = short_die_num;
				temp_short_info.slave = j | DRV_CHANNEL_FLAG;
				temp_short_info.short_code = adc_signal;
				gtx8_check_setting_group(ts_test, &temp_short_info);

				if (self_capdata[short_die_num + test_params->max_drv_num] == 0xffff ||
				    self_capdata[short_die_num + test_params->max_drv_num] == 0) {
					TS_LOG_INFO("invalid self_capdata:0x%x\n",
						self_capdata[short_die_num + test_params->max_drv_num]);
					continue;
				}

				short_r = gtx8_short_resistance_calc(ts_test, &temp_short_info,
							self_capdata[short_die_num + test_params->max_drv_num], 0);
				if (short_r < r_threshold) {
					master_pin_num = map_die2pin(test_params, temp_short_info.master);
					slave_pin_num = map_die2pin(test_params, temp_short_info.slave);
					TS_LOG_ERR("Rx/Tx short circut:R=%dK,R_Threshold=%dK\n",
								short_r, r_threshold);
					TS_LOG_ERR("%s%d--%s%d shortcircut\n",
							   (master_pin_num & DRV_CHANNEL_FLAG) ? "DRV" : "SEN",
							   (master_pin_num & ~DRV_CHANNEL_FLAG),
							   (slave_pin_num & DRV_CHANNEL_FLAG) ? "DRV" : "SEN",
							   (slave_pin_num & ~DRV_CHANNEL_FLAG));
					err |= -EINVAL;
				}
			}
		}
		data_addr += size;
	}

	if (data_buf) {
		kfree(data_buf);
		data_buf = NULL;
	}

	return ( err | ret ) ? -EFAULT : NO_ERR;
shortcircut_analysis_error:
	if (data_buf != NULL) {
		kfree(data_buf);
		data_buf = NULL;
	}
	return -EINVAL;
}

static void gtx8_shortcircut_test(struct gtx8_ts_test *ts_test)
{
	int i = 0;
	int ret = 0;
	u8 data[2] = {0};

	ts_test->test_result[GTP_SHORT_TEST] = GTP_TEST_PASS;
	ret = gtx8_short_test_prepare(ts_test);
	if (ret < 0) {
		TS_LOG_ERR("Failed enter short test mode\n");
		ts_test->test_result[GTP_SHORT_TEST] = SYS_SOFTWARE_REASON;
		return;
	}

	//msleep(3000);
	for (i = 0; i < 150; i++) {
		msleep(50);
		TS_LOG_INFO("waitting for short test end...:retry=%d\n", i);
		ret = ts_test->ts->ops.i2c_read(SHORT_TESTEND_REG, data, 1);   /* SHORT_TESTEND_REG   0x8400 */
		if (ret)
			TS_LOG_ERR("Failed get short test result: retry%d\n", i);
		else if (data[0] == 0x88)  /* test ok*/
			break;
	}

	if (i < 150) {
		ret = gtx8_shortcircut_analysis(ts_test);
		if (ret) {
			ts_test->test_result[GTP_SHORT_TEST] = GTP_PANEL_REASON;
			TS_LOG_ERR("Short test failed\n");
		} else {
			TS_LOG_ERR("Short test success\n");
		}
	} else {
		TS_LOG_ERR("Wait short test finish timeout:reg_val=0x%x\n", data[0]);
		ts_test->test_result[GTP_SHORT_TEST] = SYS_SOFTWARE_REASON;
	}
	return;
}


static void gtx8_check_setting_group(struct gtx8_ts_test *ts_test, struct short_record *r_data)
{
	u32 dMaster = 0;
	u32 dSlave = 0;

	if (r_data->master & SHORT_TEST_MASK)
		dMaster = ts_test->test_params.max_sen_num;

	if (r_data->slave & SHORT_TEST_MASK)
		dSlave = ts_test->test_params.max_sen_num;

	dMaster += (r_data->master & 0x7f);
	dSlave += (r_data->slave & 0x7f);

	if ((dMaster < 9))   /* pad s0~s8 */
	    r_data->group1 = 5;

	else if ((dMaster >= 9) && (dMaster < 14))   /* pad s9~s13 */
	    r_data->group1 = 4;

	else if ((dMaster >= 14) && (dMaster < 18))  /* pad s14~s17 */
	    r_data->group1 = 3;

	else if ((dMaster >= 18) && (dMaster < 27))  /* pad s18~s26 */
	    r_data->group1 = 2;

	else if ((dMaster >= 27) && (dMaster < 32))  /* pad s27~s31 */
	    r_data->group1 = 1;

	else if ((dMaster >= 32) && (dMaster < 36))  /* pad s32~s35 */
	    r_data->group1 = 0;

	else if ((dMaster >= 36) && (dMaster < 45))  /* pad d0~d8 */
	    r_data->group1 = 5;

	else if ((dMaster >= 45) && (dMaster < 54))  /* pad d9~d17 */
	    r_data->group1 = 2;

	else if ((dMaster >= 54) && (dMaster < 59))  /* pad d18~d22 */
	    r_data->group1 = 1;

	else if ((dMaster >= 59) && (dMaster < 63))  /*  pad d23~d26 */
	    r_data->group1 = 0;

	else if ((dMaster >= 63) && (dMaster < 67))  /* pad d27~d30 */
	    r_data->group1 = 3;

	else if ((dMaster >= 67) && (dMaster < 72))  /* pad d31~d35 */
	    r_data->group1 = 4;

	else if ((dMaster >= 72) && (dMaster < 76))  /* pad d36~d39 */
	    r_data->group1 = 0;


	if ((dSlave > 0) && (dSlave < 9))   /* pad s0~s8 */
	    r_data->group2 = 5;

	else if ((dSlave >= 9) && (dSlave < 14))   /* pad s9~s13 */
	    r_data->group2 = 4;

	else if ((dSlave >= 14) && (dSlave < 18))  /* pad s14~s17 */
	    r_data->group2 = 3;

	else if ((dSlave >= 18) && (dSlave < 27))  /* pad s18~s26 */
	    r_data->group2 = 2;

	else if ((dSlave >= 27) && (dSlave < 32))  /* pad s27~s31 */
	    r_data->group2 = 1;

	else if ((dSlave >= 32) && (dSlave < 36))  /* pad s32~s35 */
	    r_data->group2 = 0;

	else if ((dSlave >= 36) && (dSlave < 45))  /* pad d0~d8 */
	    r_data->group2 = 5;

	else if ((dSlave >= 45) && (dSlave < 54))  /* pad d9~d17 */
	    r_data->group2 = 2;

	else if ((dSlave >= 54) && (dSlave < 59))  /* pad d18~d22 */
	    r_data->group2 = 1;

	else if ((dSlave >= 59) && (dSlave < 63))  /* pad d23~d26 */
	    r_data->group2 = 0;

	else if ((dSlave >= 63) && (dSlave < 67))  /* pad d27~d30 */
	    r_data->group2 = 3;

	else if ((dSlave >= 67) && (dSlave < 72))  /* pad d31~d35 */
	    r_data->group2 = 4;

	else if ((dSlave >= 72) && (dSlave < 76))  /* pad d36~d39 */
	    r_data->group2 = 0;
}
