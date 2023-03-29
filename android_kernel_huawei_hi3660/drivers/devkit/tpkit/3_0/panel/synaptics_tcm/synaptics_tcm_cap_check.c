/*
 * Synaptics TCM touchscreen driver
 *
 * Copyright (C) 2017-2018 Synaptics Incorporated. All rights reserved.
 *
 * Copyright (C) 2017-2018 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION DOES
 * NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS'
 * TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S.
 * DOLLARS.
 */

#include <linux/gpio.h>
#include <linux/interrupt.h>

#include "synaptics_tcm_core.h"
#include "synaptics_tcm_cap_check.h"

#include "../../huawei_ts_kit_algo.h"
#include "../../tpkit_platform_adapter.h"
#include "../../huawei_ts_kit_api.h"

#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif
#if defined (CONFIG_TEE_TUI)
#include "tui.h"
#endif
#include "../../huawei_ts_kit.h"

#define SYSFS_DIR_NAME "testing"
#define SYNA_TCM_LIMITS_CSV_FILE "/odm/etc/firmware/ts/syna_tcm_cap_limits.csv"

#define SYNA_TCM_TEST_BUF_LEN 50
#define RES_TIMEOUT_MS 50
#define REPORT_TIMEOUT_MS 200
#define REPORT_TIMEOUT_MS_1 800

#define READ_REPORT_RETRY_TIMES 3

#define _TEST_PASS_ 1
#define _TEST_FAIL_ 0
struct syna_tcm_test_params *test_params = NULL;

static int syna_tcm_abs(int value)
{
	return value < 0 ? -value : value;
}

//extern char synaptics_raw_data_limit_flag;
extern struct ts_kit_platform_data g_ts_kit_platform_data;

enum test_code {
	TEST_TRX_TRX_SHORTS = 0,
	TEST_TRX_SENSOR_OPENS = 1,
	TEST_TRX_GROUND_SHORTS = 2,
	TEST_FULL_RAW_CAP = 5,
	TEST_DYNAMIC_RANGE = 7,
	TEST_OPEN_SHORT_DETECTOR = 8,
	TEST_NOISE = 10,
	TEST_PT11 = 11,
	TEST_PT12 = 12,
	TEST_PT13 = 13,
	TEST_PT18 = 18, //abs raw cap
	TEST_DISCRETE_PT196 = 196,       /* support discrete production test */
};

/* support discrete production test + */
#define MAX_PINS (64)
#define CFG_IMAGE_TXES_OFFSET (3184)
#define CFG_IMAGE_TXES_LENGTH (256)
#define CFG_IMAGE_RXES_OFFSET (2640)
#define CFG_IMAGE_RXES_LENGTH (544)
#define CFG_NUM_TXGUARD_OFFSET (3568)
#define CFG_NUM_TXGUARD_LENGTH (16)
#define CFG_NUM_RXGUARD_OFFSET (3552)
#define CFG_NUM_RXGUARD_LENGTH (16)
#define CFG_TX_GUARD_PINS_OFFSET (6640)
#define CFG_TX_GUARD_PINS_LENGTH (48)
#define CFG_RX_GUARD_PINS_OFFSET (6576)
#define CFG_RX_GUARD_PINS_LENGTH (64)

#define CFG_IMAGE_CBCS_OFFSET (2032)
#define CFG_IMAGE_CBCS_LENGTH (544)
#define CFG_REF_LO_TRANS_CAP_OFFSET (3664)
#define CFG_REF_LO_TRANS_CAP_LENGTH (16)
#define CFG_REF_LO_XMTR_PL_OFFSET (6560)
#define CFG_REF_LO_XMTR_PL_LENGTH (16)
#define CFG_REF_HI_TRANS_CAP_OFFSET (3648)
#define CFG_REF_HI_TRANS_CAP_LENGTH (16)
#define CFG_REF_HI_XMTR_PL_OFFSET (6528)
#define CFG_REF_HI_XMTR_PL_LENGTH (16)
#define CFG_REF_GAIN_CTRL_OFFSET (3632)
#define CFG_REF_GAIN_CTRL_LENGTH (16)

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
/* support discrete production test - */

struct testing_hcd {
	short *mmi_buf;
	bool result;
	unsigned char report_type;
	unsigned int report_index;
	unsigned int num_of_reports;
	struct kobject *sysfs_dir;
	struct syna_tcm_buffer out;
	struct syna_tcm_buffer resp;
	struct syna_tcm_buffer report;
	struct syna_tcm_buffer process;
	struct syna_tcm_buffer output;
	struct syna_tcm_hcd *tcm_hcd;
	/* support discrete production test + */
	unsigned char *satic_cfg_buf;
	short tx_pins[MAX_PINS];
	short tx_assigned;
	short rx_pins[MAX_PINS];
	short rx_assigned;
	short guard_pins[MAX_PINS];
	short guard_assigned;
	/* support discrete production test - */

};
extern int reflash_after_fw_update_do_reset(void);
static int testing_discrete_ex_trx_short(struct ts_rawdata_info_new* info);
static int testing_pt1(struct ts_rawdata_info_new* info);

static struct testing_hcd *testing_hcd = NULL;

static void syna_tcm_get_thr_from_csvfile(void)
{
	int ret = NO_ERR;
	unsigned int rows = 0;
	unsigned int cols = 0;
	struct syna_tcm_app_info *app_info = NULL;
	struct syna_tcm_test_threshold *threshold = &test_params->threshold;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;
	char file_path[MAX_STR_LEN*5] = {0};
	char file_name[MAX_STR_LEN*4] = {0};


	snprintf(file_name, (MAX_STR_LEN*4), "%s_%s_%s_limits.csv",
		tcm_hcd->syna_tcm_chip_data->ts_platform_data->product_name, SYNAPTICS_VENDER_NAME,
		tcm_hcd->tcm_mod_info.project_id_string);

	#ifdef BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE
	snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/%s", file_name);
	#else
	snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s", file_name);
	#endif

	app_info = &tcm_hcd->app_info;
	rows = le2_to_uint(app_info->num_of_image_rows);
	cols = le2_to_uint(app_info->num_of_image_cols);

	TS_LOG_INFO("rows: %d, cols: %d\n", rows, cols);
	ret = ts_kit_parse_csvfile(file_path, CSV_RAW_DATA_MIN_ARRAY,
			threshold->raw_data_min_limits, rows, cols);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_RAW_DATA_MIN_ARRAY);
	}

	ret = ts_kit_parse_csvfile(file_path, CSV_RAW_DATA_MAX_ARRAY,
			threshold->raw_data_max_limits, rows, cols);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_RAW_DATA_MAX_ARRAY);
	}

	ret = ts_kit_parse_csvfile(file_path, CSV_OPEN_SHORT_MIN_ARRAY,
			threshold->open_short_min_limits, rows, cols);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_OPEN_SHORT_MIN_ARRAY);
	}

	ret = ts_kit_parse_csvfile(file_path, CSV_OPEN_SHORT_MAX_ARRAY,
			threshold->open_short_max_limits, rows, cols);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_OPEN_SHORT_MAX_ARRAY);
	}

	ret = ts_kit_parse_csvfile(file_path, CSV_LCD_NOISE_ARRAY,
			threshold->lcd_noise_max_limits, rows, cols);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_LCD_NOISE_ARRAY);
	}

	ret = ts_kit_parse_csvfile(file_path, CSV_RAW_DATA_TRX_DELTA_MAX_ARRAY,
			threshold->raw_data_Trx_delta_max_limits, rows*2, cols);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_RAW_DATA_TRX_DELTA_MAX_ARRAY);
	}

	ret = ts_kit_parse_csvfile(file_path, CSV_RAW_DATA_COL_MIN_ARRAY,
			threshold->raw_data_col_min_limits, 1, cols);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_RAW_DATA_COL_MIN_ARRAY);
	}
	ret = ts_kit_parse_csvfile(file_path, CSV_RAW_DATA_COL_MAX_ARRAY,
			threshold->raw_data_col_max_limits, 1, cols);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_RAW_DATA_COL_MAX_ARRAY);
	}

	ret = ts_kit_parse_csvfile(file_path, CSV_RAW_DATA_ROW_MIN_ARRAY,
			threshold->raw_data_row_min_limits, 1, rows);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_RAW_DATA_COL_MIN_ARRAY);
	}
	ret = ts_kit_parse_csvfile(file_path, CSV_RAW_DATA_ROW_MAX_ARRAY,
			threshold->raw_data_row_max_limits, 1, rows);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, CSV_RAW_DATA_COL_MAX_ARRAY);
	}

	test_params->cap_thr_is_parsed = true;
}

static void syna_tcm_get_frame_size_words(unsigned int *size, bool image_only)
{
	unsigned int rows = 0;
	unsigned int cols = 0;
	unsigned int hybrid = 0;
	unsigned int buttons = 0;
	struct syna_tcm_app_info *app_info = NULL;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;

	app_info = &tcm_hcd->app_info;

	rows = le2_to_uint(app_info->num_of_image_rows);
	cols = le2_to_uint(app_info->num_of_image_cols);
	hybrid = le2_to_uint(app_info->has_hybrid_data);
	buttons = le2_to_uint(app_info->num_of_buttons);

	*size = rows * cols;

	if (!image_only) {
		if (hybrid)
			*size += rows + cols;
		*size += buttons;
	}

	return;
}

static void syna_tcm_put_device_info(struct ts_rawdata_info_new *info)
{
	/* put ic data */
	strncpy(info->deviceinfo, "-syna_tcm;", sizeof(info->deviceinfo));
}

static int syna_tcm_open_short_test(struct ts_rawdata_info_new *info)
{
	int retval = 0;
	unsigned int timeout = 0;
	unsigned int data_length = 0;
	signed short data = 0;
	unsigned int idx = 0;
	unsigned int row = 0;
	unsigned int col = 0;
	unsigned int rows = 0;
	unsigned int cols = 0;
	unsigned int limits_rows = 0;
	unsigned int limits_cols = 0;
	unsigned int frame_size_words = 0;
	unsigned char *temp_buf = NULL;
	unsigned char res_buf[4] = {0};
	int open_short_data_max = 0;
	int open_short_data_min = 0;
	int open_short_data_avg = 0;
	int open_short_data_size = 0;
	char testresult = CAP_TEST_PASS_CHAR;
	char failedreason[TS_RAWDATA_FAILED_REASON_LEN] = {0};
	int retry = READ_REPORT_RETRY_TIMES;

	struct syna_tcm_app_info *app_info = NULL;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;
	struct ts_rawdata_newnodeinfo* pts_node = NULL;
	struct syna_tcm_test_threshold *threshold = &test_params->threshold;

	TS_LOG_INFO("%s called\n", __func__);

	if (tcm_hcd->id_info.mode != MODE_APPLICATION ||
			tcm_hcd->app_status != APP_STATUS_OK) {
		return -ENODEV;
	}

	app_info = &tcm_hcd->app_info;
	rows = le2_to_uint(app_info->num_of_image_rows);
	cols = le2_to_uint(app_info->num_of_image_cols);
	open_short_data_size = rows * cols;

	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc pts_node failed\n");
		return -ENOMEM;
	}

	pts_node->values = kzalloc(open_short_data_size*sizeof(int), GFP_KERNEL);
	if (!pts_node->values) {
		TS_LOG_ERR("%s malloc value failed  for values\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		retval = -ENOMEM;
		goto exit;
	}

	temp_buf = kzalloc(((rows * cols + MESSAGE_HEADER_SIZE) * 2), GFP_KERNEL);
	if (!temp_buf) {
		TS_LOG_ERR("%s Failed to allocate memory for temp_buf\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		retval = -ENOMEM;
		goto exit;
	}

	retval = syna_tcm_alloc_mem(tcm_hcd,
			&testing_hcd->out,
			1);
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to allocate memory for testing_hcd->out.buf\n");
		goto exit;
	}

	testing_hcd->out.buf[0] = TEST_PT11;
	retval = syna_tcm_write_hdl_message(tcm_hcd,
			CMD_PRODUCTION_TEST,
			testing_hcd->out.buf,
			1,
			&testing_hcd->resp.buf,
			&testing_hcd->resp.buf_size,
			&testing_hcd->resp.data_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to write command %s\n",
				STR(CMD_PRODUCTION_TEST));
		goto exit;
	}

	while(retry){
		timeout = REPORT_TIMEOUT_MS_1;
		msleep(timeout);

		retval = syna_tcm_read(tcm_hcd,
				temp_buf,
				((rows * cols + MESSAGE_HEADER_SIZE) * 2));
		if (retval < 0) {
			TS_LOG_ERR("Failed to read raw data\n");
			goto exit;
		}

		if (temp_buf[0] != MESSAGE_MARKER) {
			TS_LOG_ERR("incorrect Header Marker!");
			retval = -EINVAL;
			goto exit;
		}

		if (temp_buf[1] != STATUS_OK) {
			TS_LOG_INFO("status = 0x%02x", temp_buf[1]);
		}else{
			break;
		}
		retry--;
	}

	data_length = temp_buf[2] | temp_buf[3] << 8;
	TS_LOG_INFO("%d\n", data_length);

	syna_tcm_get_frame_size_words(&frame_size_words, false);

	if (frame_size_words != data_length / 2) {
		TS_LOG_ERR("Frame size mismatch\n");
		goto exit;
	}

	idx = 0;
	open_short_data_max = open_short_data_min = le2_to_uint(&temp_buf[MESSAGE_HEADER_SIZE]);
	for (row = 0; row < rows; row++) {
		for (col = 0; col < cols; col++) {
			data = (signed short)le2_to_uint(&temp_buf[idx * 2 + MESSAGE_HEADER_SIZE]);
			pts_node->values[idx] = (int)data;
			open_short_data_max = data > open_short_data_max ? data : open_short_data_max;
			open_short_data_min = data < open_short_data_min ? data : open_short_data_min;
			open_short_data_avg += data;
			TS_LOG_DEBUG("open_short = %d", data);
			idx++;
		}
	}
	if (open_short_data_size)
		open_short_data_avg /= open_short_data_size;

	idx = 0;
	testing_hcd->result = true;
	for (row = 0; row < rows; row++) {
		for (col = 0; col < cols; col++) {
			data = (signed short)le2_to_uint(&temp_buf[idx * 2 + MESSAGE_HEADER_SIZE]);
			TS_LOG_DEBUG("open_short data=%d", data);
			if (data > threshold->open_short_max_limits[idx] ||
					data < threshold->open_short_min_limits[idx]) {
				TS_LOG_ERR("%s overlow_data = %d, row = %d, col = %d , limits [%d, %d]\n", __func__, data, row, col, threshold->open_short_min_limits[idx], threshold->open_short_max_limits[idx]);
				testing_hcd->result = false;
				testresult = CAP_TEST_FAIL_CHAR;
				strncpy(failedreason, "-panel_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
				goto exit;
			}
			idx++;
		}
	}

exit:
	pts_node->size = open_short_data_size;
	pts_node->testresult = testresult;
	pts_node->typeindex = RAW_DATA_TYPE_OpenShort;		
	strncpy(pts_node->test_name, "Open_short", TS_RAWDATA_TEST_NAME_LEN-1);
	snprintf(pts_node->statistics_data, TS_RAWDATA_STATISTICS_DATA_LEN,
			"[%d,%d,%d]",
			open_short_data_min, open_short_data_max, open_short_data_avg);
	if (CAP_TEST_FAIL_CHAR == testresult) {
		strncpy(pts_node->tptestfailedreason, failedreason, TS_RAWDATA_FAILED_REASON_LEN-1);
	}
	list_add_tail(&pts_node->node, &info->rawdata_head);
	
	if(temp_buf) {
		kfree(temp_buf);
		temp_buf = NULL;
	}
	TS_LOG_INFO("%s end retval = %d\n", __func__, retval);

	return retval;
}

static int syna_tcm_noise_test(struct ts_rawdata_info_new *info)
{
	int retval = 0;
	unsigned int idx = 0;
	signed short data = 0;
	unsigned int timeout = 0;
	unsigned int data_length = 0;
	unsigned int row = 0;
	unsigned int col = 0;
	unsigned int rows = 0;
	unsigned int cols = 0;
	unsigned int limits_rows = 0;
	unsigned int limits_cols = 0;
	unsigned int frame_size_words = 0;
	unsigned char *temp_buf = NULL;
	unsigned char res_buf[4] = {0};
	char testresult = CAP_TEST_PASS_CHAR;
	char failedreason[TS_RAWDATA_FAILED_REASON_LEN] = {0};
	int noise_data_max = 0;
	int noise_data_min = 0;
	int noise_data_avg = 0;
	int noise_data_size = 0;
	int retry = READ_REPORT_RETRY_TIMES;

	struct syna_tcm_app_info *app_info = NULL;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;
	struct ts_rawdata_newnodeinfo* pts_node = NULL;
	struct syna_tcm_test_threshold *threshold = &test_params->threshold;

	TS_LOG_INFO("%s start", __func__);

	if (tcm_hcd->id_info.mode != MODE_APPLICATION ||
			tcm_hcd->app_status != APP_STATUS_OK) {
		TS_LOG_ERR("%s Application firmware not running\n", __func__);
		return -ENODEV;
	}

	app_info = &tcm_hcd->app_info;
	rows = le2_to_uint(app_info->num_of_image_rows);
	cols = le2_to_uint(app_info->num_of_image_cols);
	noise_data_size = rows * cols;

	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc pts_node failed\n");
		return -ENOMEM;
	}

	pts_node->values = kzalloc(noise_data_size*sizeof(int), GFP_KERNEL);
	if (!pts_node->values) {
		TS_LOG_ERR("%s malloc value failed  for values\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		goto exit;
	}

	temp_buf = kzalloc(((rows * cols + MESSAGE_HEADER_SIZE) * 2), GFP_KERNEL);
	if (!temp_buf) {
		TS_LOG_ERR("%s Failed to allocate memory for temp_buf\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		retval = -ENOMEM;
		goto exit;
	}

	retval = syna_tcm_alloc_mem(tcm_hcd,
			&testing_hcd->out,
			1);
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to allocate memory for testing_hcd->out.buf\n");
		goto exit;
	}

	testing_hcd->out.buf[0] = TEST_NOISE;

	retval = syna_tcm_write_hdl_message(tcm_hcd,
			CMD_PRODUCTION_TEST,
			testing_hcd->out.buf,
			1,
			&testing_hcd->resp.buf,
			&testing_hcd->resp.buf_size,
			&testing_hcd->resp.data_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to write command %s\n",
				STR(CMD_PRODUCTION_TEST));
		goto exit;
	}

	while(retry) {
		timeout = REPORT_TIMEOUT_MS_1;   //REPORT_TIMEOUT_MS
		msleep(timeout);
		retval = syna_tcm_read(tcm_hcd,
				temp_buf,
				((rows * cols + MESSAGE_HEADER_SIZE) * 2));
		if (retval < 0) {
			TS_LOG_ERR("Failed to read raw data\n");
			goto exit;
		}

		if (temp_buf[0] != MESSAGE_MARKER) {
			TS_LOG_ERR("incorrect Header Marker!");
			retval = -EINVAL;
			goto exit;
		}

		if (temp_buf[1] != STATUS_OK) {
			TS_LOG_INFO("status = 0x%02x", temp_buf[1]);
		}else{
			break;
		}
		retry--;
	}

	data_length = temp_buf[2] | temp_buf[3] << 8;
	TS_LOG_INFO(" data_length =%d\n", data_length);
	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		syna_tcm_get_frame_size_words(&frame_size_words, false);
	} else {
		syna_tcm_get_frame_size_words(&frame_size_words, true);
	}

	if (frame_size_words != data_length / 2) {
		TS_LOG_ERR(
				"Frame size mismatch\n");
		retval = -EINVAL;
		goto exit;
	}

	idx = 0;
	noise_data_max = noise_data_min = (int)le2_to_uint(&temp_buf[MESSAGE_HEADER_SIZE]);
	for (row = 0; row < rows; row++) {
		for (col = 0; col < cols; col++) {
			data = (signed short)le2_to_uint(&temp_buf[idx * 2 + MESSAGE_HEADER_SIZE]);
			pts_node->values[idx] = (int)data;
			noise_data_max = data > noise_data_max ? data : noise_data_max;
			noise_data_min = data < noise_data_min ? data : noise_data_min;
			noise_data_avg += data;
			TS_LOG_DEBUG("noise_buf = %d", data);
			idx++;
		}
	}
	if (noise_data_size)
		noise_data_avg /= noise_data_size;
	
	idx = 0;
	testing_hcd->result = true;

	for (row = 0; row < rows; row++) {
		for (col = 0; col < cols; col++) {
			data = (signed short)le2_to_uint(&temp_buf[idx * 2 + MESSAGE_HEADER_SIZE]);
			TS_LOG_DEBUG("noise data=%d", data);
			if (data > threshold->lcd_noise_max_limits[idx]) {
				TS_LOG_ERR("%s overlow_data = %d, limits = %d, row = %d, col = %d\n", __func__, data,threshold->lcd_noise_max_limits[idx], row, col);
				testing_hcd->result = false;
				testresult = CAP_TEST_FAIL_CHAR;
				strncpy(failedreason, "-panel_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
				goto exit;
			}
			idx++;
		}
	}

exit:
	TS_LOG_INFO("%s end retval = %d", __func__, retval);

	pts_node->size = noise_data_size;
	pts_node->testresult = testresult;
	pts_node->typeindex = RAW_DATA_TYPE_Noise;		
	strncpy(pts_node->test_name, "Noise_delta", TS_RAWDATA_TEST_NAME_LEN-1);
	snprintf(pts_node->statistics_data, TS_RAWDATA_STATISTICS_DATA_LEN,
			"[%d,%d,%d]",
			noise_data_min, noise_data_max, noise_data_avg);
	if (CAP_TEST_FAIL_CHAR == testresult) {
		strncpy(pts_node->tptestfailedreason, failedreason, TS_RAWDATA_FAILED_REASON_LEN-1);
	}
	list_add_tail(&pts_node->node, &info->rawdata_head);

	if(temp_buf) {
		kfree(temp_buf);
		temp_buf = NULL;
	}
	return retval;
}

static int syna_tcm_dynamic_range_test(struct ts_rawdata_info_new *info)
{
	int retval = 0;
	unsigned int idx = 0;
	signed short data = 0;
	unsigned int timeout = 0;
	unsigned int data_length = 0;
	unsigned int row = 0;
	unsigned int col = 0;
	unsigned int rows = 0;
	unsigned int cols = 0;
	unsigned int limits_rows = 0;
	unsigned int limits_cols = 0;
	unsigned int frame_size_words = 0;
	unsigned char *temp_buf = NULL;
	unsigned char res_buf[4] = {0};
	char testresult = CAP_TEST_PASS_CHAR;
	char failedreason[TS_RAWDATA_FAILED_REASON_LEN] = {0};
	int raw_data_size= 0;
	int raw_data_max = 0;
	int raw_data_min = 0;
	int raw_data_avg = 0;
	int retry = READ_REPORT_RETRY_TIMES;

	struct syna_tcm_app_info *app_info = NULL;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;
	struct ts_rawdata_newnodeinfo* pts_node = NULL;
	struct syna_tcm_test_threshold *threshold = &test_params->threshold;

	TS_LOG_INFO("%s start", __func__);

	if (tcm_hcd->id_info.mode != MODE_APPLICATION ||
			tcm_hcd->app_status != APP_STATUS_OK) {
		return -ENODEV;
	}

	app_info = &tcm_hcd->app_info;
	rows = le2_to_uint(app_info->num_of_image_rows);
	cols = le2_to_uint(app_info->num_of_image_cols);
	raw_data_size = rows * cols;

	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc pts_node failed\n");
		return -ENOMEM;
	}

	pts_node->values = kzalloc(raw_data_size*sizeof(int), GFP_KERNEL);
	if (!pts_node->values) {
		TS_LOG_ERR("%s malloc value failed  for values\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		goto exit;
	}

	temp_buf = kzalloc(((rows * cols + MESSAGE_HEADER_SIZE) * 2), GFP_KERNEL);
	if (!temp_buf) {
		TS_LOG_ERR("%s Failed to allocate memory for temp_buf\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		retval = -ENOMEM;
		goto exit;
	}

	retval = syna_tcm_alloc_mem(tcm_hcd,
			&testing_hcd->out,
			1);
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to allocate memory for testing_hcd->out.buf\n");
		goto exit;
	}

	testing_hcd->out.buf[0] = TEST_DYNAMIC_RANGE;

	retval = syna_tcm_write_hdl_message(tcm_hcd,
			CMD_PRODUCTION_TEST,
			testing_hcd->out.buf,
			1,
			&testing_hcd->resp.buf,
			&testing_hcd->resp.buf_size,
			&testing_hcd->resp.data_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to write command %s\n",
				STR(CMD_PRODUCTION_TEST));
		goto exit;
	}

	while(retry) {
		timeout = REPORT_TIMEOUT_MS_1;   //REPORT_TIMEOUT_MS
		msleep(timeout);
		retval = syna_tcm_read(tcm_hcd,
				temp_buf,
				((rows * cols + MESSAGE_HEADER_SIZE) * 2));
		if (retval < 0) {
			TS_LOG_ERR("Failed to read raw data\n");
			goto exit;
		}

		if (temp_buf[0] != MESSAGE_MARKER) {
			TS_LOG_ERR("incorrect Header Marker!");
			retval = -EINVAL;
			goto exit;
		}

		if (temp_buf[1] != STATUS_OK) {
			TS_LOG_INFO("status = 0x%02x", temp_buf[1]);
		}else{
			break;
		}
		retry--;
	}

	data_length = temp_buf[2] | temp_buf[3] << 8;
	TS_LOG_INFO(" data_length =%d\n", data_length);

	syna_tcm_get_frame_size_words(&frame_size_words, false);
	if (frame_size_words != data_length / 2) {
		TS_LOG_ERR(
				"Frame size mismatch\n");
		retval = -EINVAL;
		goto exit;
	}
	idx = 0;
	raw_data_max = raw_data_min = (int)le2_to_uint(&temp_buf[MESSAGE_HEADER_SIZE]);
	for (row = 0; row < rows; row++) {
		for (col = 0; col < cols; col++) {
			data = (short)le2_to_uint(&temp_buf[idx * 2 + MESSAGE_HEADER_SIZE]);
			pts_node->values[idx] = (int)data;
			raw_data_max = data > raw_data_max ? data : raw_data_max;
			raw_data_min = data < raw_data_min ? data : raw_data_min;
			raw_data_avg += data;
			idx++;
		}
	}
	if (raw_data_size)
		raw_data_avg /= raw_data_size;

	idx = 0;
	testing_hcd->result = true;

	for (row = 0; row < rows; row++) {
		for (col = 0; col < cols; col++) {
			data = (short)le2_to_uint(&temp_buf[idx * 2 + MESSAGE_HEADER_SIZE]);
			if (data > threshold->raw_data_max_limits[idx] ||
				data < threshold->raw_data_min_limits[idx]) {
				TS_LOG_ERR("%s overlow_data = %d, row = %d, col = %d, limits [%d,%d]\n", __func__, data, row, col, threshold->raw_data_max_limits[idx], threshold->raw_data_min_limits[idx]);
				testing_hcd->result = false;
				testresult = CAP_TEST_FAIL_CHAR;
				strncpy(failedreason, "-panel_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
				goto exit;
			}
			idx++;
		}
	}

exit:
	TS_LOG_INFO("%s end retval = %d\n", __func__, retval);
	pts_node->size = raw_data_size;
	pts_node->testresult = testresult;
	pts_node->typeindex = RAW_DATA_TYPE_CAPRAWDATA;
	strncpy(pts_node->test_name, "Cap_Rawdata", TS_RAWDATA_TEST_NAME_LEN-1);
	snprintf(pts_node->statistics_data, TS_RAWDATA_STATISTICS_DATA_LEN,
			"[%d,%d,%d]",
			raw_data_min, raw_data_max, raw_data_avg);
	if (CAP_TEST_FAIL_CHAR == testresult) {
		strncpy(pts_node->tptestfailedreason, failedreason, TS_RAWDATA_FAILED_REASON_LEN-1);
	}
	list_add_tail(&pts_node->node, &info->rawdata_head);

	testing_hcd->report_type = 0;
	if(temp_buf) {
		kfree(temp_buf);
		temp_buf = NULL;
	}

	return retval;
}

static int syna_tcm_delta_test(struct ts_rawdata_info_new *info, int* rawdata)
{
	int retval = 0;
	unsigned int rows = 0;
	unsigned int cols = 0;
	char testresult = CAP_TEST_PASS_CHAR;
	char failedreason[TS_RAWDATA_FAILED_REASON_LEN] = {0};
	int raw_data_size= 0;
	int delta_data_max = 0;
	int delta_data_min = 0;
	int delta_data_avg = 0;
	int i = 0;
	int j = 0;
	int delta_data_size = 0;
	struct syna_tcm_app_info *app_info = NULL;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;
	struct ts_rawdata_newnodeinfo* pts_node = NULL;
	struct syna_tcm_test_threshold *threshold = &test_params->threshold;

	TS_LOG_INFO("%s start", __func__);

	if (tcm_hcd->id_info.mode != MODE_APPLICATION ||
			tcm_hcd->app_status != APP_STATUS_OK) {
		TS_LOG_ERR("%s Application firmware not running\n", __func__);
		return -ENODEV;
	}

	app_info = &tcm_hcd->app_info;
	rows = le2_to_uint(app_info->num_of_image_rows);
	cols = le2_to_uint(app_info->num_of_image_cols);
	raw_data_size = rows * cols;

	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc pts_node failed\n");
		return -ENOMEM;
	}

	pts_node->values = kzalloc(raw_data_size*sizeof(int) * 2, GFP_KERNEL);
	if (!pts_node->values) {
		TS_LOG_ERR("%s malloc value failed	for values\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		goto exit;
	}
	memset(pts_node->values, 0 ,raw_data_size*sizeof(int) * 2);
	testing_hcd->result = true;
	delta_data_max = delta_data_min = syna_tcm_abs(rawdata[0]-rawdata[1]);

	for(i = 0, j=0; i < rows * cols ; i++) {
		if (i < cols) {
			/* nothing to do*/
		} else {
			pts_node->values[j] =
				syna_tcm_abs(rawdata[i] -
				rawdata[i - cols]);
			delta_data_max = delta_data_max < pts_node->values[j] ? pts_node->values[j] : delta_data_max;
			delta_data_min = delta_data_min > pts_node->values[j] ? pts_node->values[j] : delta_data_min;
			delta_data_avg += pts_node->values[j];
			j++;
		}
	}
	delta_data_size += j;

	for(i = 0, j=0 ; i < rows * cols ; i++) {
		if (i%cols == 0) {
			/* nothing to do*/
		} else {
			pts_node->values[j + raw_data_size] = syna_tcm_abs(rawdata[i]
				-rawdata[i - 1]);
			delta_data_max = delta_data_max < pts_node->values[j] ? pts_node->values[j] : delta_data_max;
			delta_data_min = delta_data_min > pts_node->values[j] ? pts_node->values[j] : delta_data_min;
			delta_data_avg += pts_node->values[j];
			j++;
		}
	}
	delta_data_size += j;

	TS_LOG_ERR("delta_data_size = %d\n", delta_data_size);

	if(delta_data_size){
		delta_data_avg /= delta_data_size;
	}

	for (i = 0; i < raw_data_size*2; i++) {
		TS_LOG_DEBUG("delta value is[%d, %d] %d > %d\n", i/cols, i%cols,
				pts_node->values[i],
				threshold->raw_data_Trx_delta_max_limits[i]);
		if (pts_node->values[i] > threshold->raw_data_Trx_delta_max_limits[i]) {
			testresult = CAP_TEST_FAIL_CHAR;
			strncpy(failedreason, "-panel_reason",TS_RAWDATA_FAILED_REASON_LEN-1);
			TS_LOG_ERR("delta value is[%d, %d] %d > %d\n", i/cols, i%cols,
				pts_node->values[i],
				threshold->raw_data_Trx_delta_max_limits[i]);
		}
	}

exit:
	TS_LOG_INFO("%s end retval = %d\n", __func__, retval);
	pts_node->size = raw_data_size*2;
	pts_node->testresult = testresult;
	pts_node->typeindex = RAW_DATA_TYPE_TrxDelta;
	strncpy(pts_node->test_name, "delta_Rawdata", TS_RAWDATA_TEST_NAME_LEN-1);
	snprintf(pts_node->statistics_data, TS_RAWDATA_STATISTICS_DATA_LEN,
			"[%d,%d,%d]",
			delta_data_min, delta_data_max, delta_data_avg);
	if (CAP_TEST_FAIL_CHAR == testresult) {
		strncpy(pts_node->tptestfailedreason, failedreason, TS_RAWDATA_FAILED_REASON_LEN-1);
	}
	list_add_tail(&pts_node->node, &info->rawdata_head);
	info->listnodenum++;

	testing_hcd->report_type = 0;

	return retval;
}

static int syna_tcm_full_raw_cap_test(struct ts_rawdata_info_new *info)
{
	int retval = 0;
	unsigned int idx = 0;
	signed short data = 0;
	unsigned int timeout = 0;
	unsigned int data_length = 0;
	unsigned int row = 0;
	unsigned int col = 0;
	unsigned int rows = 0;
	unsigned int cols = 0;
	unsigned int frame_size_words = 0;
	unsigned char *temp_buf = NULL;
	char testresult = CAP_TEST_PASS_CHAR;
	char failedreason[TS_RAWDATA_FAILED_REASON_LEN] = {0};
	int raw_data_size= 0;
	int raw_data_max = 0;
	int raw_data_min = 0;
	int raw_data_avg = 0;
	int retry = READ_REPORT_RETRY_TIMES;

	struct syna_tcm_app_info *app_info = NULL;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;
	struct ts_rawdata_newnodeinfo* pts_node = NULL;
	struct syna_tcm_test_threshold *threshold = &test_params->threshold;

	TS_LOG_INFO("%s start", __func__);

	if (tcm_hcd->id_info.mode != MODE_APPLICATION ||
			tcm_hcd->app_status != APP_STATUS_OK) {
		TS_LOG_ERR("%s Application firmware not running\n", __func__);
		return -ENODEV;
	}

	app_info = &tcm_hcd->app_info;
	rows = le2_to_uint(app_info->num_of_image_rows);
	cols = le2_to_uint(app_info->num_of_image_cols);
	raw_data_size = rows * cols;

	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc pts_node failed\n");
		return -ENOMEM;
	}

	pts_node->values = kzalloc(raw_data_size*sizeof(int), GFP_KERNEL);
	if (!pts_node->values) {
		TS_LOG_ERR("%s malloc value failed  for values\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		goto exit;
	}

	temp_buf = kzalloc(((rows * cols + MESSAGE_HEADER_SIZE) * 2), GFP_KERNEL);
	if (!temp_buf) {
		TS_LOG_ERR("%s Failed to allocate memory for temp_buf\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		retval = -ENOMEM;
		goto exit;
	}

	retval = syna_tcm_alloc_mem(tcm_hcd,
			&testing_hcd->out,
			1);
	if (retval < 0) {
		TS_LOG_ERR("Failed to allocate memory for testing_hcd->out.buf\n");
		goto exit;
	}

	testing_hcd->out.buf[0] = TEST_FULL_RAW_CAP;

	retval = syna_tcm_write_hdl_message(tcm_hcd,
			CMD_PRODUCTION_TEST,
			testing_hcd->out.buf,
			1,
			&testing_hcd->resp.buf,
			&testing_hcd->resp.buf_size,
			&testing_hcd->resp.data_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n", STR(CMD_PRODUCTION_TEST));
		goto exit;
	}

	while(retry) {
		timeout = REPORT_TIMEOUT_MS_1;   //REPORT_TIMEOUT_MS
		msleep(timeout);
		retval = syna_tcm_read(tcm_hcd,
				temp_buf,
				((rows * cols + MESSAGE_HEADER_SIZE) * 2));
		if (retval < 0) {
			TS_LOG_ERR("Failed to read raw data\n");
			goto exit;
		}

		if (temp_buf[0] != MESSAGE_MARKER) {
			TS_LOG_ERR("incorrect Header Marker!");
			retval = -EINVAL;
			goto exit;
		}

		if (temp_buf[1] != STATUS_OK) {
			TS_LOG_INFO("status = 0x%02x", temp_buf[1]);
		}else{
			break;
		}
		retry--;
	}

	data_length = temp_buf[2] | temp_buf[3] << 8;
	TS_LOG_INFO(" data_length =%d\n", data_length);

	syna_tcm_get_frame_size_words(&frame_size_words, true);
	if (frame_size_words != data_length / 2) {
		TS_LOG_ERR("Frame size mismatch\n");
		retval = -EINVAL;
		goto exit;
	}
	idx = 0;
	raw_data_max = raw_data_min = (int)le2_to_uint(&temp_buf[MESSAGE_HEADER_SIZE]);
	for (row = 0; row < rows; row++) {
		for (col = 0; col < cols; col++) {
			data = (short)le2_to_uint(&temp_buf[idx * 2 + MESSAGE_HEADER_SIZE]);
			pts_node->values[idx] = (int)data;
			raw_data_max = data > raw_data_max ? data : raw_data_max;
			raw_data_min = data < raw_data_min ? data : raw_data_min;
			raw_data_avg += data;
			idx++;
		}
	}
	if (raw_data_size)
		raw_data_avg /= raw_data_size;

	idx = 0;
	testing_hcd->result = true;

	for (row = 0; row < rows; row++) {
		for (col = 0; col < cols; col++) {
			data = (short)le2_to_uint(&temp_buf[idx * 2 + MESSAGE_HEADER_SIZE]);
			if (data > threshold->raw_data_max_limits[idx] ||
				data < threshold->raw_data_min_limits[idx]) {
				TS_LOG_ERR("%s overlow_data = %d, row = %d, col = %d, limits [%d,%d]\n", __func__, data, row, col, threshold->raw_data_max_limits[idx], threshold->raw_data_min_limits[idx]);
				testing_hcd->result = false;
				testresult = CAP_TEST_FAIL_CHAR;
				strncpy(failedreason, "-panel_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
			}
			idx++;
		}
	}
	retval = syna_tcm_delta_test(info, pts_node->values);
	if(retval) {
		TS_LOG_INFO("%s syna_tcm_delta_test FAILED = %d\n", __func__, retval);
	}
exit:
	TS_LOG_INFO("%s end retval = %d\n", __func__, retval);
	pts_node->size = raw_data_size;
	pts_node->testresult = testresult;
	pts_node->typeindex = RAW_DATA_TYPE_CAPRAWDATA;
	strncpy(pts_node->test_name, "Cap_Rawdata", TS_RAWDATA_TEST_NAME_LEN-1);
	snprintf(pts_node->statistics_data, TS_RAWDATA_STATISTICS_DATA_LEN,
			"[%d,%d,%d]",
			raw_data_min, raw_data_max, raw_data_avg);
	if (CAP_TEST_FAIL_CHAR == testresult) {
		strncpy(pts_node->tptestfailedreason, failedreason, TS_RAWDATA_FAILED_REASON_LEN-1);
	}
	list_add_tail(&pts_node->node, &info->rawdata_head);

	testing_hcd->report_type = 0;
	if(temp_buf) {
		kfree(temp_buf);
		temp_buf = NULL;
	}
	return retval;
}

static int syna_tcm_abs_raw_cap_test(struct ts_rawdata_info_new *info)
{
	int retval = 0;
	unsigned int idx = 0;
	unsigned int timeout = 0;
	unsigned int data_length = 0;
	unsigned int row = 0;
	unsigned int col = 0;
	unsigned int rows = 0;
	unsigned int cols = 0;
	unsigned char *temp_buf = NULL;
	int *temp_buf_row_int_p= NULL;
	int *temp_buf_col_int_p = NULL;
	char testresult = CAP_TEST_PASS_CHAR;
	char failedreason[TS_RAWDATA_FAILED_REASON_LEN] = {0};
	int noise_data_max = 0;
	int noise_data_min = 0;
	int noise_data_avg = 0;
	int abs_raw_data_size = 0;
	int retry = READ_REPORT_RETRY_TIMES;

	struct syna_tcm_app_info *app_info = NULL;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;
	struct ts_rawdata_newnodeinfo* pts_node = NULL;
	struct syna_tcm_test_threshold *threshold = &test_params->threshold;

	TS_LOG_INFO("%s start", __func__);

	if (tcm_hcd->id_info.mode != MODE_APPLICATION ||
			tcm_hcd->app_status != APP_STATUS_OK) {
		TS_LOG_ERR("%s Application firmware not running\n", __func__);
		return -ENODEV;
	}

	app_info = &tcm_hcd->app_info;
	rows = le2_to_uint(app_info->num_of_image_rows);
	cols = le2_to_uint(app_info->num_of_image_cols);
	abs_raw_data_size = rows + cols;

	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc pts_node failed\n");
		return -ENOMEM;
	}

	pts_node->values = kzalloc(abs_raw_data_size*sizeof(int), GFP_KERNEL);
	if (!pts_node->values) {
		TS_LOG_ERR("%s malloc value failed  for values\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		goto exit;
	}
	temp_buf_row_int_p = kzalloc(rows*sizeof(int), GFP_KERNEL);
	if (!temp_buf_row_int_p) {
		TS_LOG_ERR("%s malloc temp_buf_row_int_p failed  for values\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		retval = -ENOMEM;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		goto exit;
	}
	temp_buf_col_int_p = kzalloc(cols*sizeof(int), GFP_KERNEL);
	if (!temp_buf_col_int_p) {
		TS_LOG_ERR("%s malloc temp_buf_col_int_p failed  for values\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		retval = -ENOMEM;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		goto exit;
	}

	temp_buf = kzalloc(((rows + cols) * 4 + MESSAGE_HEADER_SIZE + 1), GFP_KERNEL);
	if (!temp_buf) {
		TS_LOG_ERR("%s Failed to allocate memory for temp_buf\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		retval = -ENOMEM;
		goto exit;
	}

	retval = syna_tcm_alloc_mem(tcm_hcd,
			&testing_hcd->out,
			1);
	if (retval < 0) {
		TS_LOG_ERR("Failed to allocate memory for testing_hcd->out.buf\n");
		goto exit;
	}

	testing_hcd->out.buf[0] = TEST_PT18;

	retval = syna_tcm_write_hdl_message(tcm_hcd,
			CMD_PRODUCTION_TEST,
			testing_hcd->out.buf,
			1,
			&testing_hcd->resp.buf,
			&testing_hcd->resp.buf_size,
			&testing_hcd->resp.data_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to write command %s\n",
				STR(CMD_PRODUCTION_TEST));
		goto exit;
	}

	while(retry) {
		timeout = REPORT_TIMEOUT_MS_1;   //REPORT_TIMEOUT_MS
		msleep(timeout);
		retval = syna_tcm_read(tcm_hcd,
				temp_buf,
				((rows + cols) * 4 + MESSAGE_HEADER_SIZE + 1));
		if (retval < 0) {
			TS_LOG_ERR("Failed to read raw data\n");
			goto exit;
		}

		if (temp_buf[0] != MESSAGE_MARKER) {
			TS_LOG_ERR("incorrect Header Marker!");
			retval = -EINVAL;
			goto exit;
		}

		if (temp_buf[1] != STATUS_OK) {
			TS_LOG_INFO("status = 0x%02x", temp_buf[1]);
		}else{
			break;
		}
		retry--;
	}

	data_length = temp_buf[2] | temp_buf[3] << 8;
	TS_LOG_INFO(" data_length =%d\n", data_length);

	if ((temp_buf[1] != STATUS_OK) || (data_length != ((rows + cols) * 4))) {
		TS_LOG_ERR("not correct status or data_length status:%x  length:%x\n", temp_buf[1], data_length);
		retval = -EINVAL;
		goto exit;
	}

	idx = MESSAGE_HEADER_SIZE;
	testing_hcd->result = true;

	for (col = 0; col < cols; col++) {
		temp_buf_col_int_p[col] = (unsigned int)(temp_buf[idx] & 0xff) | (unsigned int)(temp_buf[idx+1] << 8) |
					(unsigned int)(temp_buf[idx+2] << 16) | (unsigned int)(temp_buf[idx+3] << 24);
			TS_LOG_DEBUG("col=%-2d data = %d , limit = [%d, %d]\n",
					col, temp_buf_col_int_p[col],
					threshold->raw_data_col_min_limits[col],
					threshold->raw_data_col_max_limits[col]);
		if (temp_buf_col_int_p[col] < threshold->raw_data_col_min_limits[col]
				|| temp_buf_col_int_p[col] > threshold->raw_data_col_max_limits[col]) {
			testing_hcd->result = false;
			testresult = CAP_TEST_FAIL_CHAR;
			strncpy(failedreason, "-panel_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
			retval = -EINVAL;
			TS_LOG_ERR("fail at col=%-2d data = %d (0x%2x 0x%2x 0x%2x 0x%2x), limit = [%d,%d]\n",
					col, temp_buf_col_int_p[col],
					temp_buf[idx], temp_buf[idx+1], temp_buf[idx+2], temp_buf[idx+3],
					threshold->raw_data_col_min_limits[col],
					threshold->raw_data_col_max_limits[col]);
		}
		idx+=4;
	}

	for (row = 0; row < rows; row++) {
		temp_buf_row_int_p[row] = (unsigned int)(temp_buf[idx] & 0xff) | (unsigned int)(temp_buf[idx+1] << 8) |
					(unsigned int)(temp_buf[idx+2] << 16) | (unsigned int)(temp_buf[idx+3] << 24);
			TS_LOG_DEBUG("row=%-2d data = %d, limit = [%d, %d]\n",
					row, temp_buf_row_int_p[row],
					threshold->raw_data_row_min_limits[row],
					threshold->raw_data_row_max_limits[row]);
		if (temp_buf_row_int_p[row] < threshold->raw_data_row_min_limits[row]
				|| temp_buf_row_int_p[row] > threshold->raw_data_row_max_limits[row]) {
			testing_hcd->result = false;
			testresult = CAP_TEST_FAIL_CHAR;
			strncpy(failedreason, "-panel_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
			retval = -EINVAL;
			TS_LOG_ERR("fail at row=%-2d data = %d (0x%2x 0x%2x 0x%2x 0x%2x), limit = [%d, %d]\n",
					row, temp_buf_row_int_p[row],
					temp_buf[idx], temp_buf[idx+1], temp_buf[idx+2], temp_buf[idx+3],
					threshold->raw_data_row_min_limits[row],
					threshold->raw_data_row_max_limits[row]);
		}
		idx+=4;
	}

	memcpy(pts_node->values, temp_buf_row_int_p, rows*sizeof(int));
	memcpy(&(pts_node->values[rows]), temp_buf_col_int_p, cols*sizeof(int));

exit:
	TS_LOG_INFO("%s end retval = %d", __func__, retval);

	pts_node->size = abs_raw_data_size;
	pts_node->testresult = testresult;
	pts_node->typeindex = RAW_DATA_TYPE_SelfCap;
	strncpy(pts_node->test_name, "abs raw data", TS_RAWDATA_TEST_NAME_LEN-1);
	snprintf(pts_node->statistics_data, TS_RAWDATA_STATISTICS_DATA_LEN,
			"[%d,%d,%d]",
			noise_data_min, noise_data_max, noise_data_avg);
	if (CAP_TEST_FAIL_CHAR == testresult) {
		strncpy(pts_node->tptestfailedreason, failedreason, TS_RAWDATA_FAILED_REASON_LEN-1);
	}
	list_add_tail(&pts_node->node, &info->rawdata_head);

	if(temp_buf) {
		kfree(temp_buf);
		temp_buf = NULL;
	}
	if(temp_buf_row_int_p) {
		kfree(temp_buf_row_int_p);
		temp_buf_row_int_p = NULL;
	}
	if(temp_buf_col_int_p) {
		kfree(temp_buf_col_int_p);
		temp_buf_col_int_p = NULL;
	}
	return retval;
}

int syna_tcm_cap_test(struct ts_rawdata_info_new *info,
			struct ts_cmd_node *out_cmd)
{
	int retval = NO_ERR;
	unsigned int rows = 0;
	unsigned int cols = 0;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;
	struct syna_tcm_app_info *app_info = NULL;
	struct ts_rawdata_newnodeinfo* pts_node = NULL;

	TS_LOG_INFO("%s called\n", __func__);

	testing_hcd->tcm_hcd = tcm_hcd;
	app_info = &tcm_hcd->app_info;
	rows = le2_to_uint(app_info->num_of_image_rows);
	cols = le2_to_uint(app_info->num_of_image_cols);
	if(0 == rows || 0 == cols) {
		TS_LOG_ERR("rows or cols is NULL\n");
		return -ENOMEM;
	}
	if (!test_params->cap_thr_is_parsed)
		syna_tcm_get_thr_from_csvfile();

	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc failed\n");
		return -ENOMEM;
	}

	if (tcm_hcd->id_info.mode != MODE_APPLICATION ||
			tcm_hcd->app_status != APP_STATUS_OK) {
		pts_node->typeindex = RAW_DATA_TYPE_IC;	
		pts_node->testresult = CAP_TEST_FAIL_CHAR;
		list_add_tail(&pts_node->node, &info->rawdata_head);
		return -ENODEV;
	} else {
		pts_node->typeindex = RAW_DATA_TYPE_IC;	
		pts_node->testresult = CAP_TEST_PASS_CHAR;
		list_add_tail(&pts_node->node, &info->rawdata_head);
	}

	syna_tcm_put_device_info(info);

	syna_tcm_enable_touch(tcm_hcd, false);

	TS_LOG_INFO("%s: before disable doze\n", __func__);
	retval = testing_hcd->tcm_hcd->set_dynamic_config(testing_hcd->tcm_hcd,
			DC_NO_DOZE, 1);
	if (retval < 0) {
		TS_LOG_ERR("%s failed to set dc\n", __func__);
	}
	retval = syna_tcm_noise_test(info);

	if (retval < 0) {
		TS_LOG_ERR("%s lcd noise testing failed.\n", __func__);
	}
	info->listnodenum++;
	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		retval = syna_tcm_dynamic_range_test(info);
	} else {
		retval = syna_tcm_full_raw_cap_test(info);
	}
	if (retval < 0) {
		TS_LOG_ERR("%s raw data testing failed.\n", __func__);
	}
	info->listnodenum++;
	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		/* do nothing */
	} else {
		retval = syna_tcm_abs_raw_cap_test(info);
		if (retval < 0) {
			TS_LOG_ERR("%s raw data testing failed.\n", __func__);
		}
		info->listnodenum++;
	}

	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		retval = syna_tcm_open_short_test(info);
		if (retval < 0) {
			TS_LOG_ERR("%s open short testing failed.\n", __func__);
		}
	} else {
		/* retval = testing_discrete_ex_trx_short(info); */
		/* if (retval < 0) {
			TS_LOG_ERR("%s fail to do testing_discrete_ex_trx_short", __func__);
		} */
	}
	info->listnodenum++;

	retval = testing_hcd->tcm_hcd->set_dynamic_config(testing_hcd->tcm_hcd,
			DC_NO_DOZE, 0);
	if (retval < 0) {
		TS_LOG_ERR("%s failed to set dc\n", __func__);
	}
	TS_LOG_INFO("%s: after enable doze\n", __func__);

	syna_tcm_enable_touch(tcm_hcd, true);
	
	info->tx = cols;
	info->rx = rows;

	if (TS_BUS_I2C == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		gpio_set_value(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, 0);
		msleep(10);
		gpio_set_value(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, 1);
	}

	return 0;
}
static unsigned char temp_buf[2000], temp_buf_2[2000];
static bool testing_get_pins_assigned(unsigned short pin,
		short *tx_pins, short tx_assigned, short *rx_pins, short rx_assigned,
		short *guard_pins, short guard_assigned)
{
	int i = 0;

	for (i = 0; i < tx_assigned; i++) {
		if (pin == tx_pins[i]) {
			return true;
		}
	}
	for (i = 0; i < rx_assigned; i++) {
		if (pin == rx_pins[i]) {
			return true;
		}
	}
	for (i = 0; i < guard_assigned; i++) {
		if (pin == guard_pins[i]) {
			return true;
		}
	}

	return false;
}

static int testing_pins_mapping(unsigned char *cfg_data, unsigned int cfg_data_len)
{
	int i = 0, j = 0;
	int idx = 0;
	int offset_rx_pin = CFG_IMAGE_RXES_OFFSET/8;
	int length_rx_pin = CFG_IMAGE_RXES_LENGTH/8;
	int offset_tx_pin = CFG_IMAGE_TXES_OFFSET/8;
	int length_tx_pin = CFG_IMAGE_TXES_LENGTH/8;
	int num_rx_guard = 0;
	int offset_num_rx_guard= CFG_NUM_RXGUARD_OFFSET/8;
	int length_num_rx_guard= CFG_NUM_RXGUARD_LENGTH/8;
	int offset_rx_guard= CFG_RX_GUARD_PINS_OFFSET/8;
	int length_rx_guard= CFG_RX_GUARD_PINS_LENGTH/8;
	int num_tx_guard = 0;
	int offset_num_tx_guard= CFG_NUM_TXGUARD_OFFSET/8;
	int length_num_tx_guard= CFG_NUM_TXGUARD_LENGTH/8;
	int offset_tx_guard= CFG_TX_GUARD_PINS_OFFSET/8;
	int length_tx_guard= CFG_TX_GUARD_PINS_LENGTH/8;

	if (!cfg_data) {
		TS_LOG_ERR("invalid parameter\n");
		return -EINVAL;
	}

	testing_hcd->tx_assigned = 0;
	testing_hcd->rx_assigned = 0;
	testing_hcd->guard_assigned = 0;

	/* get tx pins mapping */
	if (cfg_data_len > offset_tx_pin + length_tx_pin) {
		testing_hcd->tx_assigned = (length_tx_pin/2);
		idx = 0;
		for (i = 0; i < (length_tx_pin/2); i++) {
			testing_hcd->tx_pins[i] = (short)cfg_data[offset_tx_pin + idx] |
									(short)(cfg_data[offset_tx_pin + idx + 1] << 8);
			idx += 2;
			TS_LOG_DEBUG("tx[%d] = %2d\n", i, testing_hcd->tx_pins[i]);
		}
	}

	/* get rx pins mapping */
	if (cfg_data_len > offset_rx_pin + length_rx_pin) {
		testing_hcd->rx_assigned = (length_rx_pin/2);
		idx = 0;
		for (i = 0; i < (length_rx_pin/2); i++) {
			testing_hcd->rx_pins[i] = (short)cfg_data[offset_rx_pin + idx] |
									(short)(cfg_data[offset_rx_pin + idx + 1] << 8);
			idx += 2;
			TS_LOG_DEBUG("rx[%d] = %2d\n", i, testing_hcd->rx_pins[i]);
		}
	}

	/* get number of tx guards */
	if (cfg_data_len > offset_num_tx_guard + length_num_tx_guard) {
		num_tx_guard = (short)cfg_data[offset_num_tx_guard] |
						(short)(cfg_data[offset_num_tx_guard + 1] << 8);

		testing_hcd->guard_assigned += num_tx_guard;
	}

	/* get number of rx guards */
	if (cfg_data_len > offset_num_rx_guard + length_num_rx_guard) {
		num_rx_guard = (short)cfg_data[offset_num_rx_guard] |
						(short)(cfg_data[offset_num_rx_guard + 1] << 8);

		testing_hcd->guard_assigned += num_rx_guard;
	}
	if (testing_hcd->guard_assigned > 0) {
		TS_LOG_ERR("num of guards = %2d (tx: %d, rx: %d)\n",
					testing_hcd->guard_assigned, num_tx_guard, num_rx_guard);
		j = 0;
	}

	/* get tx guards mapping */
	if ((num_tx_guard > 0) && (num_tx_guard <= cfg_data_len/2) &&
		(cfg_data_len > offset_tx_guard + length_tx_guard)) {
		idx = 0;
		for (i = 0; i < num_tx_guard; i++) {
			testing_hcd->guard_pins[j] = (short)cfg_data[offset_tx_guard + idx] |
										(short)(cfg_data[offset_tx_guard + idx + 1] << 8);
			TS_LOG_ERR("guard_pins[%d] = %2d\n", i, testing_hcd->guard_pins[j]);
			idx += 2;
			j += 1;
		}
	}

	/* get rx guards mapping */
	if ((num_rx_guard > 0) && (num_rx_guard < cfg_data_len/2) &&
		(cfg_data_len > offset_rx_guard + length_rx_guard)) {
		for (i = 0; i < num_rx_guard; i++) {
			testing_hcd->guard_pins[j] = (short)cfg_data[offset_rx_guard + idx] |
										(short)(cfg_data[offset_rx_guard + idx + 1] << 8);
			TS_LOG_ERR("guard_pins[%d] = %2d\n", i, testing_hcd->guard_pins[j]);
			idx += 2;
			j += 1;
		}
	}

	return 0;
}

static int testing_get_static_config(unsigned char *buf, unsigned int buf_len)
{
	int retval = NO_ERR;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;

	if (!buf) {
		TS_LOG_ERR("invalid parameter\n");
		return -EINVAL;
	}

	retval = syna_tcm_write_hdl_message(tcm_hcd,
			CMD_GET_STATIC_CONFIG,
			NULL,
			0,
			&testing_hcd->resp.buf,
			&testing_hcd->resp.buf_size,
			&testing_hcd->resp.data_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n",
				STR(CMD_GET_STATIC_CONFIG));
		goto exit;
	}

	msleep(REPORT_TIMEOUT_MS);
	retval = syna_tcm_read(tcm_hcd,
		temp_buf,
		sizeof(temp_buf));

	if (retval < 0) {
		TS_LOG_ERR("Failed to temp_buf data\n");
		return retval;
	}

	if ((temp_buf[0] != MESSAGE_MARKER) || (temp_buf[1] != STATUS_OK)) {
		TS_LOG_ERR("message error %x, %x %x %x\n", temp_buf[0], temp_buf[1], temp_buf[2], temp_buf[3]);
		retval = -EINVAL;
		goto exit;
	}

	TS_LOG_ERR("data length:%d\n", (temp_buf[2] | temp_buf[3] << 8));

	retval = secure_memcpy(buf,
				buf_len,
				&temp_buf[4],
				buf_len,
				buf_len);
	if (retval < 0) {
		TS_LOG_ERR("Failed to copy cfg data\n");
		goto exit;
	}

exit:
	return retval;
}

static int testing_set_static_config(unsigned char *buf, unsigned int buf_len)
{
	int retval = NO_ERR;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;
	if (!buf) {
		TS_LOG_ERR("invalid parameter\n");
		return -EINVAL;
	}

	retval = syna_tcm_alloc_mem(tcm_hcd,
			&testing_hcd->out,
			buf_len);
	if (retval < 0) {
		TS_LOG_ERR("Failed to allocate memory for testing_hcd->out.buf\n");
		return retval;
	}

	retval = secure_memcpy(testing_hcd->out.buf,
				testing_hcd->out.buf_size,
				buf,
				buf_len,
				buf_len);
	if (retval < 0) {
		TS_LOG_ERR("Failed to copy cfg data\n");
		return retval;
	}

	retval = syna_tcm_write_hdl_message(tcm_hcd,
			CMD_SET_STATIC_CONFIG,
			testing_hcd->out.buf,
			buf_len,
			&testing_hcd->resp.buf,
			&testing_hcd->resp.buf_size,
			&testing_hcd->resp.data_length,
			NULL,
			0);

	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n",
				STR(CMD_SET_STATIC_CONFIG));
		return retval;
	}

	msleep(REPORT_TIMEOUT_MS);

	retval = syna_tcm_read(tcm_hcd,
		temp_buf,
		5);

	if (retval < 0) {
		TS_LOG_ERR("Failed to temp_buf data\n");
		return retval;
	}

	if (temp_buf[0] != MESSAGE_MARKER) {
		if ((temp_buf[1] != STATUS_OK) && (temp_buf[1] != 0)) {
			TS_LOG_ERR("message error when set write config %x, %x %x %x\n", temp_buf[0], temp_buf[1], temp_buf[2], temp_buf[3]);
			retval = -EINVAL;
			goto exit;
		}
	}

exit:
	return retval;
}

static unsigned char trx_result[MAX_PINS] = {0};

static int testing_pt1(struct ts_rawdata_info_new* info)
{
	int retval = NO_ERR;
	int i = 0, j = 0, ii = 0;
 	int phy_pin = 0;
	bool do_pin_test = false;
	bool is_rx = false;
	unsigned char *pt1_data = NULL;
	unsigned int pt1_data_size = 8;
	unsigned char *satic_cfg_buf = NULL;
	unsigned int satic_cfg_length;
	short *tx_pins = testing_hcd->tx_pins;
	short tx_assigned = 0;
	short *rx_pins = testing_hcd->rx_pins;
	short rx_assigned = 0;
	short *guard_pins = testing_hcd->guard_pins;
	short guard_assigned = 0;
	struct syna_tcm_app_info *app_info;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;
	int failure_cnt_pt1 = 0;
	int data_length = 0;
	int pt1_data_min = 0;
	int pt1_data_max = 0;
	int pt1_data_avg = 0;

	char testresult = CAP_TEST_PASS_CHAR;
	char failedreason[TS_RAWDATA_FAILED_REASON_LEN] = {0};
	int pt1_size = 8;
	struct ts_rawdata_newnodeinfo* pts_node = NULL;

	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc pts_node failed\n");
		return -ENOMEM;
	}

	pts_node->values = kzalloc(MAX_PINS*sizeof(int), GFP_KERNEL);
	if (!pts_node->values) {
		TS_LOG_ERR("%s malloc value failed	for values\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		goto exit;
	}

	app_info = &tcm_hcd->app_info;

	memset(trx_result, 0, sizeof(trx_result));

	if (!testing_hcd->satic_cfg_buf) {
		satic_cfg_length = le2_to_uint(app_info->static_config_size);

		satic_cfg_buf = kzalloc(satic_cfg_length, GFP_KERNEL);
		if (!satic_cfg_buf) {
			TS_LOG_ERR("Failed on memory allocation for satic_cfg_buf\n");
			goto exit;
		}

		retval = testing_get_static_config(satic_cfg_buf, satic_cfg_length);
		if (retval < 0) {
			TS_LOG_ERR("Failed to get static config\n");
			goto exit;
		}
		testing_hcd->satic_cfg_buf = satic_cfg_buf;
	}

	// get pins mapping
	if ( (testing_hcd->tx_assigned <= 0) ||
		 (testing_hcd->rx_assigned <= 0) ||
		 (testing_hcd->guard_assigned <= 0) ){

		if (satic_cfg_buf) {
			retval = testing_pins_mapping(satic_cfg_buf, satic_cfg_length);
			if (retval < 0) {
				TS_LOG_ERR("Failed to get pins mapping\n");
				goto exit;
			}

			TS_LOG_ERR("tx_assigned = %d, rx_assigned = %d, guard_assigned = %d",
						testing_hcd->tx_assigned, testing_hcd->rx_assigned,
						testing_hcd->guard_assigned);
		}
	}

	tx_assigned = testing_hcd->tx_assigned;
	rx_assigned = testing_hcd->rx_assigned;
	guard_assigned = testing_hcd->guard_assigned;

	retval = syna_tcm_alloc_mem(tcm_hcd,
			&testing_hcd->out,
			1);
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to allocate memory for testing_hcd->out.buf\n");
		goto exit;
	}

	testing_hcd->out.buf[0] = TEST_TRX_SENSOR_OPENS;

	retval = syna_tcm_write_hdl_message(tcm_hcd,
			CMD_PRODUCTION_TEST,
			testing_hcd->out.buf,
			1,
			&testing_hcd->resp.buf,
			&testing_hcd->resp.buf_size,
			&testing_hcd->resp.data_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to write command %s\n",
				STR(CMD_PRODUCTION_TEST));
		goto exit;
	}

	msleep(REPORT_TIMEOUT_MS);
	retval = syna_tcm_read(tcm_hcd,
			temp_buf,
			pt1_data_size + MESSAGE_HEADER_SIZE + 1);

	if (retval < 0) {
		TS_LOG_ERR("Failed to read raw data\n");
		goto exit;
	}

	if (temp_buf[0] != MESSAGE_MARKER) {
		TS_LOG_ERR("incorrect Header Marker!");
		retval = -EINVAL;
		goto exit;

	}

	data_length = temp_buf[2] | temp_buf[3] << 8;
	TS_LOG_INFO("%d\n", data_length);

	testing_hcd->result = true;

	pt1_data = kzalloc(sizeof(unsigned char)*pt1_data_size, GFP_KERNEL);
	if (!pt1_data) {
		TS_LOG_ERR("Failed to allocate mem to pt1_data\n");
		goto exit;
	}
	retval = secure_memcpy(pt1_data,
				pt1_data_size,
				&temp_buf[4],
				pt1_data_size,
				pt1_data_size);
	if (retval < 0) {
		TS_LOG_ERR("Failed to copy pt1 data\n");
		goto exit;
	}

	pt1_data_max = pt1_data_min = trx_result[0];

	for (i = 0; i < pt1_data_size; i++) {
		TS_LOG_ERR("[%d]: %2d\n",i , pt1_data[i]);
		pts_node->values[i] = trx_result[i];
		pt1_data_max = pt1_data_max < pts_node->values[i] ? pts_node->values[i] : pt1_data_max;
		pt1_data_min = pt1_data_min > pts_node->values[i] ? pts_node->values[i] : pt1_data_min;
		pt1_data_avg += pts_node->values[i];

		for (j = 0; j < 8; j++) {
			phy_pin = (i*8 + j);
			do_pin_test = testing_get_pins_assigned(phy_pin,
			tx_pins, tx_assigned,
			rx_pins, rx_assigned,
			guard_pins, guard_assigned);
			if (do_pin_test) {
				if (CHECK_BIT(pt1_data[i], j) == 0) {
					TS_LOG_DEBUG("pin-%2d : pass\n", phy_pin);
				} else {
					// check pin-0, 1, 32, 33
					if (( 0 == phy_pin) || ( 1 == phy_pin) || (32 == phy_pin) || (33 == phy_pin)) {
						for (ii = 0; ii < rx_assigned; ii++) {
							is_rx = false;
							if (phy_pin == rx_pins[ii]) {
								is_rx = true;
								break;
							}
						}
						if (is_rx) {
							TS_LOG_ERR("pin-%2d : n/a (is rx)\n", phy_pin);
						}
						else {
							TS_LOG_ERR("pin-%2d : fail (byte %d)\n", phy_pin, i);
							trx_result[phy_pin] = 1;
							failure_cnt_pt1 += 1;
						}
					}
					else {
						TS_LOG_ERR("pin-%2d : fail (byte %d)\n", phy_pin, i);
						trx_result[phy_pin] = 1;
						failure_cnt_pt1 += 1;
					}
				}
			}// end if(do_pin_test)
		}
	}

	if(pt1_data_size) {
		pt1_data_avg /= pt1_data_size;
	}
	testing_hcd->result = (failure_cnt_pt1 == 0);

	retval = failure_cnt_pt1;

	pts_node->size = MAX_PINS;
	pts_node->testresult = testresult;
	pts_node->typeindex = RAW_DATA_TYPE_OpenShort;
	strncpy(pts_node->test_name, "extend-trx short", TS_RAWDATA_TEST_NAME_LEN-1);
	snprintf(pts_node->statistics_data, TS_RAWDATA_STATISTICS_DATA_LEN,
			"[%d,%d,%d]",
			pt1_data_min, pt1_data_max, pt1_data_avg);
	if (CAP_TEST_FAIL_CHAR == testresult) {
		strncpy(pts_node->tptestfailedreason, failedreason, TS_RAWDATA_FAILED_REASON_LEN-1);
	}
	list_add_tail(&pts_node->node, &info->rawdata_head);

exit:
	if (pt1_data) {
		kfree(pt1_data);
		pt1_data = NULL;
	}
	if (satic_cfg_buf) {
		kfree(satic_cfg_buf);
		satic_cfg_buf = NULL;
	}
	return retval;
}

static int testing_get_test_item_response_data(int item, unsigned char *data, int length)
{

	int retval = 0;
	int timeout = 0;
	int data_length = 0;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;

	retval = syna_tcm_alloc_mem(tcm_hcd,
			&testing_hcd->out,
			1);
	if (retval < 0) {
		TS_LOG_ERR("Failed to allocate memory for testing_hcd->out.buf\n");
		goto exit;
	}

	testing_hcd->out.buf[0] = item;

	retval = syna_tcm_write_hdl_message(tcm_hcd,
			CMD_PRODUCTION_TEST,
			testing_hcd->out.buf,
			1,
			&testing_hcd->resp.buf,
			&testing_hcd->resp.buf_size,
			&testing_hcd->resp.data_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to write command %s\n",
				STR(CMD_PRODUCTION_TEST));
		goto exit;
	}

	timeout = REPORT_TIMEOUT_MS;

	msleep(timeout);
	retval = syna_tcm_read(tcm_hcd,
			temp_buf,
			sizeof(temp_buf));

	if (retval < 0) {
		TS_LOG_ERR("Failed to read raw data\n");
		goto exit;
	}

	if (temp_buf[0] != MESSAGE_MARKER) {
		TS_LOG_ERR("incorrect Header Marker!");
		retval = -EINVAL;
		goto exit;

	}

	data_length = temp_buf[2] | temp_buf[3] << 8;
	TS_LOG_INFO("%d\n", data_length);

	retval = secure_memcpy(data,
				length,
				&temp_buf[4],
				length,
				length);
	if (retval < 0) {
		TS_LOG_ERR("Failed to copy cfg data\n");
		return retval;
	}
exit:
	return retval;
}

static int testing_discrete_ex_trx_short(struct ts_rawdata_info_new* info)
{
	int retval = NO_ERR;
	int i = 0, j = 0, k = 0, pin = 0;
	unsigned char *buf = NULL;
	unsigned int rows = 0;
	unsigned int cols = 0;
	unsigned char *satic_cfg_buf = NULL;
	unsigned int satic_cfg_length = 0;
	struct syna_tcm_app_info *app_info = NULL;
	struct syna_tcm_hcd *tcm_hcd = testing_hcd->tcm_hcd;
	int result_pt1 = 0;
	int result_pt196 = 0;
	unsigned int pt196_size = 0;
	unsigned short *pt196_base = NULL;
	unsigned short *pt196_delta = NULL;
	unsigned short *min_rx = NULL;
	unsigned short *max_rx = NULL;
	unsigned short tmp_data = 0;
	unsigned short extend[4] = {0, 1, 32, 33};
	bool do_pin_test = true;
	unsigned short logical_pin = 0;

	char testresult = CAP_TEST_PASS_CHAR;
	char failedreason[TS_RAWDATA_FAILED_REASON_LEN] = {0};
	int trx_short_size = 0;
	struct ts_rawdata_newnodeinfo* pts_node = NULL;

	app_info = &tcm_hcd->app_info;

	rows = le2_to_uint(app_info->num_of_image_rows);
	cols = le2_to_uint(app_info->num_of_image_cols);

	trx_short_size = rows * cols * 4;

	pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
	if (!pts_node) {
		TS_LOG_ERR("malloc pts_node failed\n");
		return -ENOMEM;
	}

	pts_node->values = kzalloc(trx_short_size*sizeof(int), GFP_KERNEL);
	if (!pts_node->values) {
		TS_LOG_ERR("%s malloc value failed  for values\n", __func__);
		testresult = CAP_TEST_FAIL_CHAR;
		strncpy(failedreason, "-software_reason", TS_RAWDATA_FAILED_REASON_LEN-1);
		goto exit;
	}

	pt196_size = rows * cols * 2;

	satic_cfg_length = le2_to_uint(app_info->static_config_size);
	if (testing_hcd->satic_cfg_buf == NULL) {
		satic_cfg_buf = kzalloc(satic_cfg_length, GFP_KERNEL);
		if (!satic_cfg_buf) {
			TS_LOG_ERR("Failed on memory allocation for satic_cfg_buf\n");
			goto exit;
		}
		testing_hcd->satic_cfg_buf = satic_cfg_buf;
	} else {
		satic_cfg_buf = testing_hcd->satic_cfg_buf;
	}

	retval = testing_get_static_config(satic_cfg_buf, satic_cfg_length);
	if (retval < 0) {
		TS_LOG_ERR("Failed to get static config\n");
		goto exit;
	}

	// get pins mapping
	if ( (testing_hcd->tx_assigned <= 0) ||
		 (testing_hcd->rx_assigned <= 0) ||
		 (testing_hcd->guard_assigned <= 0) ){

		if (satic_cfg_buf) {
			retval = testing_pins_mapping(satic_cfg_buf, satic_cfg_length);
			if (retval < 0) {
				TS_LOG_ERR("Failed to get pins mapping\n");
				goto exit;
			}

			TS_LOG_ERR("tx_assigned = %d, rx_assigned = %d, guard_assigned = %d",
						testing_hcd->tx_assigned, testing_hcd->rx_assigned,
						testing_hcd->guard_assigned);
		}
	}

	/* do pt1 testing */
	retval = testing_pt1(info);
	if (retval < 0) {
		TS_LOG_ERR("Failed to run pt1 test\n");
		goto exit;
	}
	result_pt1 = retval;  // copy the result of pt1

	reflash_after_fw_update_do_reset();
	// change analog settings
	for (i = 0; i < (CFG_IMAGE_CBCS_LENGTH/8); i++) {
		satic_cfg_buf[(CFG_IMAGE_CBCS_OFFSET/8) + i] = 0x00;
	}
	satic_cfg_buf[(CFG_REF_LO_TRANS_CAP_OFFSET/8)] = 0x06;
	satic_cfg_buf[(CFG_REF_LO_XMTR_PL_OFFSET/8)] = 0x01;
	satic_cfg_buf[(CFG_REF_HI_TRANS_CAP_OFFSET/8)] = 0x06;
	satic_cfg_buf[(CFG_REF_HI_XMTR_PL_OFFSET/8)] = 0x00;
	satic_cfg_buf[(CFG_REF_GAIN_CTRL_OFFSET/8)] = 0x00;

	retval = testing_set_static_config(satic_cfg_buf, satic_cfg_length);

	if (retval < 0) {
		TS_LOG_ERR("Failed to set static config\n");
		goto exit;
	}

	pt196_base = kzalloc(sizeof(unsigned short)*rows*cols, GFP_KERNEL);
	if (!pt196_base) {
		TS_LOG_ERR("Failed on memory allocation for pt196_base\n");
		UNLOCK_BUFFER(testing_hcd->resp);
		retval = -EINVAL;
		goto exit;
	}

	retval = testing_get_test_item_response_data(TEST_DISCRETE_PT196, temp_buf_2, rows*cols*2);
	if (retval < 0) {
		TS_LOG_ERR("fail to do test item TEST_DISCRETE_PT196");
		retval = -EINVAL;
		goto exit;
	}
	buf = temp_buf_2;

	k = 0;
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			pt196_base[i * cols + j] =
					(unsigned short)(buf[k] &0xff) | (unsigned short)(buf[k+1] << 8);
			k+=2;
		}
	}

	pt196_delta = kzalloc(sizeof(unsigned short)*rows*cols, GFP_KERNEL);
	if (!pt196_delta) {
		TS_LOG_ERR("Failed on memory allocation for pt196_delta\n");
		retval = -EINVAL;
		goto exit;
	}
	min_rx= kzalloc(sizeof(unsigned short)*(testing_hcd->rx_assigned), GFP_KERNEL);
	if (!min_rx) {
		TS_LOG_ERR("Failed on memory allocation for min_rx\n");
		retval = -EINVAL;
		goto exit;
	}
	max_rx = kzalloc(sizeof(unsigned short)*(testing_hcd->rx_assigned), GFP_KERNEL);
	if (!max_rx) {
		TS_LOG_ERR("Failed on memory allocation for max_rx\n");
		retval = -EINVAL;
		goto exit;
	}

	// walk through all extend pins
	for (pin = 0; pin < 4; pin++) {

		do_pin_test = testing_get_pins_assigned(extend[pin],
				 					testing_hcd->tx_pins, testing_hcd->tx_assigned,
				 					testing_hcd->rx_pins, testing_hcd->rx_assigned,
				 					testing_hcd->guard_pins, testing_hcd->guard_assigned);
		if (!do_pin_test)
			continue;  // skip if pin is not assigned

		for (i = 0; i < testing_hcd->rx_assigned; i++) {
			do_pin_test = false;
			if (extend[pin] == testing_hcd->rx_pins[i]) {
				do_pin_test = true;
				logical_pin = i;
				break;
			}
		}

		if (!do_pin_test)
			continue;  // skip if pin is not rx


		for (i = 0; i < testing_hcd->rx_assigned; i++) {
			min_rx[i] = 5000;
			max_rx[i] = 0;
		}

		TS_LOG_ERR("pin = %d, logical pin = %d\n", extend[pin], logical_pin);

		// adjust cbc for the target logical pin
		for (i = 0; i < (CFG_IMAGE_CBCS_LENGTH/8); i++) {
			if (i == logical_pin*2)
				satic_cfg_buf[(CFG_IMAGE_CBCS_OFFSET/8) + i] = 0x0f;
			else
				satic_cfg_buf[(CFG_IMAGE_CBCS_OFFSET/8) + i] = 0x00;
		}
		retval = testing_set_static_config(satic_cfg_buf, satic_cfg_length);
		if (retval < 0) {
			TS_LOG_ERR("Failed to set static config for logical pin %d\n",
					logical_pin);
			goto exit;
		}

		retval = testing_get_test_item_response_data(TEST_DISCRETE_PT196, temp_buf_2, rows*cols*2);
		if (retval < 0) {
			TS_LOG_ERR("fail to do test item TEST_DISCRETE_PT196");
			retval = -EINVAL;
			goto exit;
		}
		buf = temp_buf_2;
		k = 0;
		for (i = 0; i < rows; i++) {
			for (j = 0; j < cols; j++) {

				tmp_data = (unsigned short)(buf[k] &0xff) | (unsigned short)(buf[k+1] << 8);
				pt196_delta[i * cols + j] = abs(tmp_data - pt196_base[i * cols + j]);
				//pts_node->values[pin * rows * cols  + i * cols + j] = pt196_delta[i * cols + j];
				if (testing_hcd->rx_assigned == cols) {
					min_rx[j] = MIN(min_rx[j], pt196_delta[i * cols + j]);
					max_rx[j] = MAX(max_rx[j], pt196_delta[i * cols + j]);
				}
				else if (testing_hcd->rx_assigned == rows) {
					min_rx[i] = MIN(min_rx[i], pt196_delta[i * cols + j]);
					max_rx[i] = MAX(max_rx[i], pt196_delta[i * cols + j]);
				}

				k+=2;
			}
		}

		// data verification
		for (i = 0; i < testing_hcd->rx_assigned; i++) {

			if (i == logical_pin) {
				// the delta should be higher than limit
				if (min_rx[i] < 2000) {
					TS_LOG_ERR("fail at pin %d (logical: %d), delta: (%d, %d)\n",
							extend[pin], logical_pin, min_rx[i], max_rx[i]);
					trx_result[extend[pin]] = 1;
					result_pt196 += 1;
				}
			}
			else {
				// if it is not the extended pin
				// the delta should be less than limit
				if (max_rx[i] >= 200) {
					TS_LOG_ERR("fail at logical pin %d, delta: (%d, %d)\n",
							i, min_rx[i], max_rx[i]);
					trx_result[extend[pin]] = 1;
					result_pt196 += 1;
				}
			}
		}

	}

	retval = 0;
	testing_hcd->result = ((result_pt1 == 0) && (result_pt196 == 0));

	pts_node->size = trx_short_size;
	pts_node->testresult = testresult;
	pts_node->typeindex = RAW_DATA_TYPE_Noise;

	if (CAP_TEST_FAIL_CHAR == testresult) {
		strncpy(pts_node->tptestfailedreason, failedreason, TS_RAWDATA_FAILED_REASON_LEN-1);
	}

exit:
	TS_LOG_ERR("test result is %d  %d", result_pt1, result_pt196);
	for (i = 0; i < MAX_PINS; i++) {
		TS_LOG_DEBUG("trx_result[%d]:%d\n", i, trx_result[i]);
	}
	TS_LOG_ERR("at here, ready to call kfree");

	if (pt196_base) {
		kfree(pt196_base);
		pt196_base = NULL;
	}
	if (pt196_delta) {
		kfree(pt196_delta);
		pt196_delta = NULL;
	}
	if (min_rx) {
		kfree(min_rx);
		min_rx = NULL;
	}
	if (max_rx) {
		kfree(max_rx);
		max_rx = NULL;
	}
	if(pts_node) {
		if(pts_node->values) {
			kfree(pts_node->values);
			pts_node->values = NULL;
		}
		kfree(pts_node);
		pts_node = NULL;
	}

	reflash_after_fw_update_do_reset();
	return retval;
}

int syna_tcm_cap_test_init(struct syna_tcm_hcd *tcm_hcd)
{
	int retval = 0;

	if (!testing_hcd) {
		testing_hcd = kzalloc(sizeof(*testing_hcd), GFP_KERNEL);
		if (!testing_hcd) {
			TS_LOG_ERR(
					"Failed to allocate memory for testing_hcd\n");
			return -ENOMEM;
		}
	}

	if (!test_params) {
		test_params = vmalloc(sizeof(struct syna_tcm_test_params));
		if (!test_params) {
			TS_LOG_ERR("%s:alloc mem for params fail\n", __func__);
			retval = -ENOMEM;
			goto err_alloc_testing_hcd;
		}
		memset(test_params, 0, sizeof(struct syna_tcm_test_params));
	}

	testing_hcd->tcm_hcd = tcm_hcd;

	return 0;

err_alloc_testing_hcd:
	if (testing_hcd) {
		kfree(testing_hcd);
		testing_hcd = NULL;
	}

	return retval;
}


int syna_tcm_cap_test_remove(struct syna_tcm_hcd *tcm_hcd)
{
	if (testing_hcd) {
		if (testing_hcd->satic_cfg_buf) {
			kfree(testing_hcd->satic_cfg_buf);
			testing_hcd->satic_cfg_buf = NULL;
		}

		kfree(testing_hcd);
		testing_hcd = NULL;
	}

	return 0;
}

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics TCM Testing Module");
MODULE_LICENSE("GPL v2");
