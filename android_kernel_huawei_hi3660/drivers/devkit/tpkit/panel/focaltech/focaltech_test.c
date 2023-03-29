#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "focaltech_test.h"
#include "focaltech_core.h"
#include "focaltech_flash.h"
#include "../../huawei_ts_kit.h"

#define DEVICE_MODE_ADDR		0x00
#define FTS_REG_DATA_TYPE		0x01
#define REG_TX_NUM			0x02
#define REG_RX_NUM			0x03

#define REG_CB_ADDR_H			0x18
#define REG_CB_ADDR_L			0x19

#define REG_RAW_BUF0			0x6A
#define REG_CB_BUF0			0x6E

#define REG_5X46_RAW_BUF0  0x36
#define REG_5X46_CB_BUF0  0x4E
#define REG_SCWORK_MODE  0x44
#define REG_SCCBADDR   0x45

#define REG_CLB				0x04
#define REG_8607_LCD_NOISE_FRAME  0X12
#define REG_8607_LCD_NOISE_START  0X11
#define REG_8607_LCD_NOISE_NUMBER 0X13
#define REG_8607_LCD_NOISE_VALUE   0X80
#define REG_DATA_TYPE			0x06
#define DATA_TYPE_RAW_DATA		0xAD
#define DATA_5X46_TYPE_RAW_DATA	0xAA
#define DEBUG_DETA_DIFF_DATA		0x01
#define DEBUG_DATA_RAW_DATA		0x00

#define REG_8716_LCD_NOISE_FRAME	0x12
#define REG_8716_LCD_NOISE_START	0X11
#define REG_8716_LCD_NOISE_VALUE	0X80
#define TEST_TIMEOUT			99
#define LEFT_SHIFT_FOUR			32
#define PERCENTAGE_DATA			100
#define REG_8719_LCD_NOISE_READY	0XAA

#define FTS_CALIBRATION_DISABLE_REG  	0xEE

#define REG_ADC_SCAN			0x0F
#define REG_5X46_ADC_SCAN		0x07

#define REG_ADC_SCAN_STATUS		0x10
#define REG_5X46_ADC_SCAN_STATUS	0x07

#define REG_5X46_IC_VERSION        0xB1
#define REG_5X46_FIR_EN			0xFB
#define REG_5X46_CHANG_FRE         0x0A
#define REG_5X46_NOISE_FRE		0x1C
#define REG_5X46_NOISE_TEST_SWITCH      0x1B
#define REG_5X46_MUL_DATA_THRESHOLD	0x0D
#define REG_5X46_RELEASE_HIGH		0xAE
#define REG_5X46_RELEASE_LOW		0xAF

#define OPEN_5X46_FIR_SW		0x01
#define CLOSE_5X46_FIR_SW		0x00
#define READ_5X46_READ_DIFF		0x01
#define READ_5X46_READ_RAW		0x00
#define OPEN_5X46_NOISE_SW		0x01
#define	FTS_5X46_RELEAEHIGH_LEFT_8	8
#define FTS_5X46_RELEASE_ID		0x0104

#define REG_ADC_DATA_ADDR		0x89
#define REG_5X46_ADC_DATA_ADDR		0xF4
#define MAX_ADC_NUM			    2047
#define MAX_ADC_NUM_FT8607	    2007
#define MAX_ADC_NUM_FT8006U	    4095
#define MIN_ADC_NUM_FT8006U	    170
#define MIN_ADC_NUM_FT8719			200
#define ADC_EX_RESIS_1_FT8719			252000
#define ADC_EX_RESIS_2_FT8719			120
#define ADC_EX_RESIS_3_FT8719			60
#define RAWDATA_TEST_REG			0x9E
#define CB_TEST_REG			0x9F
#define OPEN_TEST_REG			0xA0
#define REG_GIP			     	0x20
#define MAX_CB_VALUE			200
#define TP_TEST_FAILED_REASON_LEN			20

#define ERROR_CODE_OK			0x00
#define DELAY_TIME_OF_CALIBRATION		30

#define LCD_TEST_FRAMENUM		50
#define REG_LCD_NOISE_NUMBER		0X13
#define PRE				1
#define LCD_NOISE_DATA_PROCESS		0
#define REG_LCD_NOISE_DATA_READY	0x00

#define REG_NORMALIZE_TYPE		0x16
#define REG_PATTERN_5452				0x53
#define REG_MAPPING_SWITCH				0x54
#define REG_TX_NOMAPPING_NUM			0x55
#define REG_RX_NOMAPPING_NUM			0x56
#define NO_MAPPING						1
#define MAPPING							0
#define SHORT_TEST_RES_LEVEL			0x5A
#define SHORT_TEST_ERR_CH0				0x58

#define SHORT_TEST_STAND_ADC			1437
#define SHORT_TEST_STAND_OFFSET			1024
#define SHORT_TEST_STAND_RES			111
#define SHORT_TEST_STAND_RES0			4
#define SHORT_TEST_OFFSET_RES			395
#define READ_ADC_CMD_OFFSET				0x04
#define READ_ADC_CMD_CHL_TO_GND			0x03
#define READ_ADC_CMD_CHL_TO_CHL			0x02
#define READ_ADC_CMD_CHL_TO_ALL			0x01
#define SET_SHORT_TEST_DELAY			1
#define LOW_LEVEL_STAND_RES				70
#define SCAN_QUERY_TIME					100
#define OFFSET_RES						3
#define PANEL_DIFF_FACTOR				10

static char tp_test_failed_reason[TP_TEST_FAILED_REASON_LEN] = { "-software_reason" };
struct focal_test_params *params = NULL;
static int focal_write_flag = false;
static int  tp_cap_test_status =TEST_SUCCESS;
static u8 judge_ic_type = FTS_INIT_VALUE;
static int focal_start_scan(void);
static int focal_chip_clb(void);
static int focal_read_raw_data(u8 data_type, u8 *raw_data, size_t size);

static int focal_read_channel_x(u8 *chl_x);
static int focal_read_channel_y(u8 *chl_y);

static int focal_get_cb_data(int offset, u8 *data, size_t data_size);
static int focal_get_Scap_cb_data_5x46(int offset,u8 *data,size_t data_size);
static int focal_get_scap_raw_data_format_5x46(int raw_data_type,int *data, size_t size);
static int focal_get_adc_data(int *data, size_t size,unsigned int chl_x,unsigned int chl_y);
static int focal_get_raw_data_format(unsigned int chl_x,unsigned int chl_y,int *data, size_t size);

static void focal_print_test_data(char *msg, int row, int col, int max,
	int min, int value);

static void focal_put_test_result(struct focal_test_params *params,
	struct ts_rawdata_info *info, struct focal_test_result *test_results[],
	int size);
static void focal_put_test_result_fornewformat(struct focal_test_params *params,
	struct ts_rawdata_info_new *info, struct focal_test_result *test_results[],
	int size);

static int focal_alloc_test_container(struct focal_test_result **result,
	size_t data_size);
static void focal_free_test_container(struct focal_test_result *result);

static int focal_get_channel_form_ic(struct focal_test_params *params);
static int focal_init_test_prm(struct focal_platform_data *pdata,
	struct focal_test_params *params,struct ts_rawdata_info *info);

static int focal_scap_raw_data_test_5x46(struct focal_test_params *params,
	struct focal_test_result **result,char test_num);
static int focal_scap_raw_data_test_3528(struct focal_test_params *params,
	struct focal_test_result **result,char test_num);
static int focal_raw_data_test(struct focal_test_params *params,
	struct focal_test_result **result,char test_num);
static int focal_cb_test(struct focal_test_params *params,
	struct focal_test_result **result,char test_num);
static int focal_open_test(struct focal_test_params *params,
	struct focal_test_result **result,char test_num);
static int focal_short_circuit_test(struct focal_test_params *params,
	struct focal_test_result **result,char test_num);
static int focal_enter_work(void);
static int focal_enter_factory(void);
static int focal_lcd_noise_test(struct focal_test_params *params,
	struct focal_test_result **result,char test_num);

static int focal_abs(int value);
static int focal_ft8716_lcd_noise_test(struct focal_test_params *params,
	struct focal_test_result **result,char test_num);
static int focal_row_column_delta_test(
	struct focal_test_params *params,
	struct focal_test_result *srcresult, struct focal_test_result **result, char test_num);
static int focal_ft5x46_lcd_noise_test(
	struct focal_test_params *params, struct focal_test_result **result,char test_num);
static int focal_ft3528_short_circuit_test(struct focal_test_params *params,
	struct focal_test_result **result,char test_num);
static int focal_ft3528_panel_differ_test(
	struct focal_test_params *params,
	struct focal_test_result **result,char test_num);



static int focal_abs(int value)
{
	return value < 0 ? -value : value;
}

static int focal_enter_work(void)
{
	int i = 0;
	int ret = 0;
	u8 ret_val = 0;

	TS_LOG_INFO("%s: enter work\n", __func__);

	for (i = 0; i < MAX_RETRY_TIMES; i++) {
		ret = focal_read_reg(DEVICE_MODE_ADDR, &ret_val);
		if (ret) {
			TS_LOG_ERR("%s: read DEVICE_MODE_ADDR reg failed\n",
				__func__);
			return ret;
		}
		if (((ret_val >> 4) & 0x07) != DEVICE_MODE_WORK) {
			ret = focal_write_reg(DEVICE_MODE_ADDR, DEVICE_MODE_WORK);
			if (!ret) {
				if(FOCAL_FT8719 == g_focal_dev_data->ic_type) {
					msleep(200);
				}else{
					msleep(50);
				}
				continue;
			} else {
				TS_LOG_ERR("%s: change work mode failed\n",
					__func__);
				break;
			}
		} else {
			TS_LOG_INFO("%s: change work mode success\n", __func__);
			break;
		}
	}

	return ret;
}

static int focal_enter_factory(void)
{
	int i = 0;
	int ret = 0;
	u8 ret_val = 0;

	TS_LOG_INFO("%s: start change factory mode\n", __func__);
	for (i = 0; i < MAX_RETRY_TIMES; i++) {
		ret = focal_read_reg(DEVICE_MODE_ADDR, &ret_val);
		if (ret) {
			TS_LOG_ERR("%s: read DEVICE_MODE_ADDR error\n",
				__func__);
			return ret;
		}

		if (((ret_val >> 4) & 0x07) == 0x04) {
			TS_LOG_INFO("%s: change factory success\n", __func__);
			return 0;
		} else {
			ret = focal_write_reg(DEVICE_MODE_ADDR,
				DEVICE_MODE_FACTORY);
			if (!ret) {
				if(FOCAL_FT8719 == g_focal_dev_data->ic_type) {
					msleep(200);
				}else{
					msleep(50);
				}
				continue;
			} else {
				TS_LOG_ERR("%s: write reg failed\n",
					__func__);
				return ret;
			}
		}
	}

	TS_LOG_INFO("%s: end change factory mode\n", __func__);
	return ret;
}

static int focal_get_int_average(int *p, size_t len)
{
	long long sum = 0;
	size_t i = 0;

	if (!p)
		return 0;

	for (i = 0; i < len; i++)
		sum += p[i];

	if (len != 0)
		return (int)div_s64(sum, len);
	else
		return 0;
}

static int focal_get_int_min(int *p, size_t len)
{
	int min = 0;
	size_t i = 0;

	if (!p || len <= 0)
		return 0;

	min = p[0];
	for (i = 0; i < len; i++)
		min = min > p[i] ? p[i] : min;

	return min;
}

static int focal_get_int_max(int *p, size_t len)
{
	int max = 0;
	size_t i = 0;

	if (!p || len <= 0)
		return 0;

	max = p[0];
	for (i = 0; i < len; i++)
		max = max < p[i] ? p[i] : max;

	return max;
}

void focal_set_max_channel_num(struct focal_test_params *params)
{
}

int  focal_start_test_tp(
	struct focal_test_params *params,
	struct ts_rawdata_info *info)
{
	char cap_test_num = 0;
	int i = 0;
	int ret = 0;
	struct focal_test_result *test_results[FTS_MAX_CAP_TEST_NUM] = {0};

	TS_LOG_INFO("%s: start test tp\n",  __func__);
	if((!params )|| !(info)){
		TS_LOG_ERR("%s: parameter error\n",  __func__);
		return -EINVAL;
	}
	//first test is i2c test which is completed in  focal_init_test_prm,so there count from 1;
	/*  raw data test */
	ret = focal_raw_data_test(params, &test_results[cap_test_num],cap_test_num+1);
	if(ret){
		tp_cap_test_status = SOFTWARE_REASON;
		if(test_results[cap_test_num])
		{
			TS_LOG_ERR("%s: test_result is NULL cap_test_num is %d\n",  __func__, cap_test_num);
			strncpy(test_results[cap_test_num]->tptestfailedreason, "-software reason",TP_TEST_FAILED_REASON_LEN);
		}
		strncpy(tp_test_failed_reason, "-software reason",TP_TEST_FAILED_REASON_LEN);
	}
	cap_test_num++;

	if ((OPEN_ROW_COLUMN_TEST == params->row_column_delta)
	   || (ROW_COLUMN_DELTA_TEST_POINT_BY_POINT ==  params->row_column_delta_test_point_by_point)) {
		ret = focal_row_column_delta_test(params, test_results[cap_test_num - 1],
			&test_results[cap_test_num], cap_test_num+1);
		if(ret){
			tp_cap_test_status = SOFTWARE_REASON;
			if(test_results[cap_test_num])
			{
				TS_LOG_ERR("%s: test_result is NULL cap_test_num is %d\n",  __func__, cap_test_num);
				strncpy(test_results[cap_test_num]->tptestfailedreason, "-software reason",TP_TEST_FAILED_REASON_LEN);
			}
			strncpy(tp_test_failed_reason, "-software reason",TP_TEST_FAILED_REASON_LEN);
		}
		cap_test_num++;
	}

	if (OPEN_LCD_NOISE_DATA_TEST == params->lcd_noise_data) {
		ret = focal_ft5x46_lcd_noise_test(params, &test_results[cap_test_num], cap_test_num+1);
		if(ret){
			tp_cap_test_status = SOFTWARE_REASON;
			if(test_results[cap_test_num])
			{
				TS_LOG_ERR("%s: test_result is NULL cap_test_num is %d\n",  __func__, cap_test_num);
				strncpy(test_results[cap_test_num]->tptestfailedreason, "-software reason",TP_TEST_FAILED_REASON_LEN);
			}
			strncpy(tp_test_failed_reason, "-software reason",TP_TEST_FAILED_REASON_LEN);
		}
		cap_test_num++;
	}

	/*  cb data test */
	ret = focal_cb_test(params, &test_results[cap_test_num],cap_test_num+1);
	if(ret){
		tp_cap_test_status = SOFTWARE_REASON;
		if(test_results[cap_test_num])
		{
			TS_LOG_ERR("%s: test_result is NULL cap_test_num is %d\n",  __func__, cap_test_num);
			strncpy(test_results[cap_test_num]->tptestfailedreason, "-software reason",TP_TEST_FAILED_REASON_LEN);
		}
		strncpy(tp_test_failed_reason, "-software reason",TP_TEST_FAILED_REASON_LEN);
	}
	cap_test_num++;

	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type) {
		ret = focal_scap_raw_data_test_5x46(params, &test_results[cap_test_num], cap_test_num + 1);
		cap_test_num++;
	} else if(FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		ret = focal_scap_raw_data_test_3528(params, &test_results[cap_test_num], cap_test_num+1);
		cap_test_num++;
	}

	if(FOCAL_FT3528 == g_focal_dev_data->ic_type)
	{
		ret = focal_ft3528_panel_differ_test(params, &test_results[cap_test_num],cap_test_num+1);
		if(ret){
			tp_cap_test_status = SOFTWARE_REASON;
			strncpy(tp_test_failed_reason, "-software reason",TP_TEST_FAILED_REASON_LEN);
		}
		cap_test_num++;
	}

	if((FOCAL_FT8716 == g_focal_dev_data->ic_type && 
		0 == strcmp(g_focal_dev_data->ts_platform_data->product_name, 
		FTS_FT8716_PRODUCT_NAME)) || FOCAL_FT8719 == g_focal_dev_data->ic_type || (FOCAL_FT8006U == g_focal_dev_data->ic_type)){
		ret = focal_ft8716_lcd_noise_test(params, &test_results[cap_test_num],cap_test_num+1);
		if(ret){
			tp_cap_test_status = SOFTWARE_REASON;
			if(test_results[cap_test_num])
			{
				TS_LOG_ERR("%s: test_result is NULL cap_test_num is %d\n",  __func__, cap_test_num);
				strncpy(test_results[cap_test_num]->tptestfailedreason, "-software reason",TP_TEST_FAILED_REASON_LEN);
			}
			strncpy(tp_test_failed_reason, "-software reason",TP_TEST_FAILED_REASON_LEN);
		}
		cap_test_num++;
	}

	if(FOCAL_FT8716 == g_focal_dev_data->ic_type || FOCAL_FT8719 == g_focal_dev_data->ic_type || FOCAL_FT8006U == g_focal_dev_data->ic_type)
	{
		/* open test */
		ret = focal_open_test(params, &test_results[cap_test_num],cap_test_num+1);
		if(ret){
			tp_cap_test_status = SOFTWARE_REASON;
			if(test_results[cap_test_num])
			{
				TS_LOG_ERR("%s: test_result is NULL cap_test_num is %d\n",  __func__, cap_test_num);
				strncpy(test_results[cap_test_num]->tptestfailedreason, "-software reason",TP_TEST_FAILED_REASON_LEN);
			}
			strncpy(tp_test_failed_reason, "-software reason",TP_TEST_FAILED_REASON_LEN);
		}
		cap_test_num++;
	}
	/*  short test */
	if(FOCAL_FT3528 == g_focal_dev_data->ic_type)
	{
		ret = focal_ft3528_short_circuit_test(params, &test_results[cap_test_num],cap_test_num+1);
		if(ret){
			tp_cap_test_status = SOFTWARE_REASON;
			strncpy(tp_test_failed_reason, "-software reason",TP_TEST_FAILED_REASON_LEN);
		}
		cap_test_num++;
	} else {
		ret = focal_short_circuit_test(params, &test_results[cap_test_num],cap_test_num+1);
		if(ret){
			tp_cap_test_status = SOFTWARE_REASON;
			if(test_results[cap_test_num])
			{
				TS_LOG_ERR("%s: test_result is NULL cap_test_num is %d\n",  __func__, cap_test_num);
				strncpy(test_results[cap_test_num]->tptestfailedreason, "-software reason",TP_TEST_FAILED_REASON_LEN);
			}
			strncpy(tp_test_failed_reason, "-software reason",TP_TEST_FAILED_REASON_LEN);
		}
		cap_test_num++;
	}

	if(FOCAL_FT8607 == g_focal_dev_data->ic_type)
	{
		/* lcd_noise test */
		ret = focal_lcd_noise_test(params, &test_results[cap_test_num],cap_test_num+1);
		if(ret){
			tp_cap_test_status = SOFTWARE_REASON;
			if(test_results[cap_test_num])
			{
				TS_LOG_ERR("%s: test_result is NULL cap_test_num is %d\n",  __func__, cap_test_num);
				strncpy(test_results[cap_test_num]->tptestfailedreason, "-software reason",TP_TEST_FAILED_REASON_LEN);
			}
			strncpy(tp_test_failed_reason, "-software reason",TP_TEST_FAILED_REASON_LEN);
		}
		cap_test_num++;
	}
	if (g_focal_dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_OLDFORMAT){
		focal_put_test_result(params, info, test_results, cap_test_num);	
	}else{
		focal_put_test_result_fornewformat(params, (struct ts_rawdata_info_new *)info, test_results, cap_test_num);
	}
	
	for (i = 0; i < cap_test_num; i++) {
		focal_free_test_container(test_results[i]);
		test_results[i] = NULL;
	}

	return ret;
}

static int focal_start_scan(void)
{
	int i = 0;
	int ret = 0;
	int query_delay = 0;
	int max_query_times = 0;

	u8 reg_val = 0x00;

	ret = focal_read_reg(DEVICE_MODE_ADDR, &reg_val);
	if (ret) {
		TS_LOG_ERR("%s: read device mode fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	reg_val |= 0x80;
	ret = focal_write_reg(DEVICE_MODE_ADDR, reg_val);
	if (ret) {
		TS_LOG_ERR("%s:set device mode fail, ret=%d\n", __func__, ret);
		return ret;
	}

	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type
		|| FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		query_delay = FTS_5X46_SCAN_QUERY_DELAY;
		max_query_times = FTS_5X46_SCAN_MAX_TIME / query_delay;
	}
	else{
		query_delay = FTS_SCAN_QUERY_DELAY;
		max_query_times = FTS_SCAN_MAX_TIME / query_delay;
	}

	msleep(query_delay);
	for (i = 0; i < max_query_times; i++) {
		ret = focal_read_reg(DEVICE_MODE_ADDR, &reg_val);
		if (ret) {
			TS_LOG_ERR("%s: read device model fail, ret=%d\n",
				__func__, ret);
			msleep(query_delay);
			continue;
		}

		if ((reg_val >> 7) != 0) {
			TS_LOG_INFO("%s:device is scan, retry=%d\n",
				__func__, i);
			msleep(query_delay);
		} else {
			TS_LOG_INFO("%s:device scan finished, retry=%d\n",
				__func__, i);
			return 0;
		}
	}

	TS_LOG_ERR("%s:device scan timeout\n", __func__);
	return -ETIMEDOUT;
}

static int focal_read_raw_data(u8 data_type, u8 *raw_data, size_t size)
{
	int i = 0;
	int ret = 0;
	int pkg_size = 0;
	int pkg_count = 0;
	int readed_count = 0;

	u8 cmd = 0;

	pkg_count = size / BYTES_PER_TIME;
	if (size % BYTES_PER_TIME != 0)
		pkg_count += 1;

	if(FOCAL_FT8719 == g_focal_dev_data->ic_type || FOCAL_FT8006U == g_focal_dev_data->ic_type){
		if (true != focal_write_flag){
			ret = focal_write_reg(FTS_REG_DATA_TYPE, data_type);
			if (ret) {
				TS_LOG_ERR("%s:write data type to ret, ret=%d\n",
					__func__, ret);
				return ret;
			}
		}
	}else{
		ret = focal_write_reg(FTS_REG_DATA_TYPE, data_type);
		if (ret) {
			TS_LOG_ERR("%s:write data type to ret, ret=%d\n",
				__func__, ret);
			return ret;
		}
	}
	msleep(10);
	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type
			|| FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		cmd = REG_5X46_RAW_BUF0;
	}
	else{
		cmd = REG_RAW_BUF0;
	}
	for (i = 0; i < pkg_count; i++) {
		/*
		 * compute pkg_size value
		 * if the last package is not equal to BYTES_PER_TIME,
		 * the package size will be set to (size % BYTES_PER_TIME)
		 */
		if ((size % BYTES_PER_TIME != 0) && (i + 1 == pkg_count))
			pkg_size = size % BYTES_PER_TIME;
		else
			pkg_size = BYTES_PER_TIME;
		if (TS_BUS_I2C == g_focal_dev_data->ts_platform_data->bops->btype) {
			/* first package shuold write cmd to ic */
			if (i == 0)
				ret = focal_read(&cmd, 1,	raw_data, pkg_size);
			else
				ret = focal_read_default(raw_data + readed_count,
					pkg_size);
		} else {
			ret = focal_read(&cmd, 1, raw_data + readed_count, pkg_size);
		}
		if (ret) {
			TS_LOG_ERR("%s:read raw data fail, ret=%d\n",
				__func__, ret);
			return ret;
		}

		readed_count += pkg_size;
	}

	return 0;
}

static int focal_get_Scap_cb_data_5x46(
	int offset,
	u8 *data,
	size_t data_size)
{
	int i = 0;
	int ret = 0;
	int pkg_size = 0;
	int pkg_count = 0;
	int readed_count = 0;
	u8 cmd[3] = {0};

	pkg_count = data_size / BYTES_PER_TIME;
	if (0 != (data_size % BYTES_PER_TIME))
		pkg_count += 1;
	ret = focal_start_scan();
	if (ret) {
		TS_LOG_ERR("%s:start scan fail, ret=%d\n",
			__func__, ret);
		return ret;
	}
	/*waterproof*/
	ret = focal_write_reg(REG_SCWORK_MODE,1);
	if (ret) {
		TS_LOG_ERR("%s:write work mode addr fail, ret=%d\n",
			__func__, ret);
		return ret;
	}
	ret = focal_write_reg(REG_SCCBADDR,0);
	if (ret) {
		TS_LOG_ERR("%s:write cb addr fail, ret=%d\n",
			__func__, ret);
		return ret;
	}
	cmd[0] = REG_5X46_CB_BUF0;
	if(data_size <BYTES_PER_TIME ){
		TS_LOG_ERR("cb data read size:%d\n",data_size);
		ret = focal_read(cmd, 1, data, data_size);
		if (ret) {
			TS_LOG_ERR("%s:read cb data fail, ret=%d\n",
				__func__, ret);
			return ret;
		}
	}
	else{
		for (i = 0; i < pkg_count; i++) {

			if ((i + 1 == pkg_count) && (data_size % BYTES_PER_TIME != 0)){
				pkg_size = data_size % BYTES_PER_TIME;
			}
			else{
				pkg_size = BYTES_PER_TIME;
			}
			ret = focal_read(cmd, 1, data + readed_count, pkg_size);
			if (ret) {
				TS_LOG_ERR("%s:read cb data fail, ret=%d\n",
					__func__, ret);
				return ret;
			}

			readed_count += pkg_size;
		}
	}
	/*non-waterproof*/
	ret = focal_write_reg(REG_SCWORK_MODE,0);
	if (ret) {
		TS_LOG_ERR("%s:write work mode addr fail, ret=%d\n",
			__func__, ret);
		return ret;
	}
	ret = focal_write_reg(REG_SCCBADDR,0);
	if (ret) {
		TS_LOG_ERR("%s:write cb addr fail, ret=%d\n",
			__func__, ret);
		return ret;
	}
	if(data_size <BYTES_PER_TIME ){
		TS_LOG_ERR("cb data read size:%d\n",data_size);
		ret = focal_read(cmd, 1, data+data_size, data_size);
		if (ret) {
			TS_LOG_ERR("%s:read cb data fail, ret=%d\n",
				__func__, ret);
			return ret;
		}
	}
	else{
		for (i = 0; i < pkg_count; i++) {

			if ((i + 1 == pkg_count) && (data_size % BYTES_PER_TIME != 0))
				pkg_size = data_size % BYTES_PER_TIME;
			else
				pkg_size = BYTES_PER_TIME;

			ret = focal_read(cmd, 1, data + readed_count, pkg_size);
			if (ret) {
				TS_LOG_ERR("%s:read cb data fail, ret=%d\n",
					__func__, ret);
				return ret;
			}

			readed_count += pkg_size;
		}
	}
	return 0;
}

static int focal_get_cb_data(
	int offset,
	u8 *data,
	size_t data_size)
{
	int i = 0;
	int ret = 0;
	int pkg_size = 0;
	int pkg_count = 0;
	int readed_count = 0;
	u8 cmd[3] = {0};

	pkg_count = data_size / BYTES_PER_TIME;
	if (0 != (data_size % BYTES_PER_TIME))
		pkg_count += 1;

	cmd[0] = REG_CB_BUF0;
	for (i = 0; i < pkg_count; i++) {

		if ((i + 1 == pkg_count) && (data_size % BYTES_PER_TIME != 0))
			pkg_size = data_size % BYTES_PER_TIME;
		else
			pkg_size = BYTES_PER_TIME;

		/* addr high 8 bits */
		cmd[1] = (offset + readed_count) >> 8;
		ret = focal_write_reg(REG_CB_ADDR_H, cmd[1]);
		if (ret) {
			TS_LOG_ERR("%s:write cb addr h fail, ret=%d\n",
				__func__, ret);
			return ret;
		}

		/* addr low 8 bits */
		cmd[2] = (offset + readed_count) & 0xff;
		ret = focal_write_reg(REG_CB_ADDR_L, cmd[2]);
		if (ret) {
			TS_LOG_ERR("%s:write cb addr l fail, ret=%d\n",
				__func__, ret);
			return ret;
		}

		ret = focal_read(cmd, 1, data + readed_count, pkg_size);
		if (ret) {
			TS_LOG_ERR("%s:read cb data fail, ret=%d\n",
				__func__, ret);
			return ret;
		}

		readed_count += pkg_size;
	}

	return 0;
}

/* get panelrows */
static int focal_read_channel_x(u8 *channel_x)
{
	return focal_read_reg(REG_TX_NUM, channel_x);
}

/* get panelcols */
static int focal_read_channel_y(u8 *channel_y)
{
	return focal_read_reg(REG_RX_NUM, channel_y);
}

/* auto clb */
static int focal_chip_clb(void)
{
	int ret = 0;
	u8 reg_data = 0;
	u8 retry_count = 50;

	/*  start auto clb */
	ret = focal_write_reg(REG_CLB, 4);
	if (ret) {
		TS_LOG_ERR("%s:write clb reg fail, ret=%d\n", __func__, ret);
		return ret;
	}
	msleep(100);

	while (retry_count--) {
		ret = focal_write_reg(DEVICE_MODE_ADDR, 0x04 << 4);
		if (ret) {
			TS_LOG_ERR("%s:write dev model reg fail, ret=%d\n",
				__func__, ret);
			return ret;
		}

		ret = focal_read_reg(0x04, &reg_data);
		if (ret) {
			TS_LOG_ERR("%s:read 0x04 reg fail, ret=%d\n",
				__func__, ret);
			return ret;
		}

		if (reg_data == 0x02) {
			return 0;
		}else{
			TS_LOG_INFO("%s:read 0x04 reg =%d wait 100ms retry\n", __func__, reg_data);
			msleep(100);
		}
	}

	return -ETIMEDOUT;
}

/************************************************************************
* name: focal_get_channel_num
* brief:  get num of ch_x, ch_y and key
* input: none
* output: none
* return: comm code. code = 0x00 is ok, else fail.
***********************************************************************/
int focal_get_channel_num(u8 *channel_x, u8 *channel_y)
{
	int ret = 0;
	u8 chl_x = 0;
	u8 chl_y = 0;

	/* get channel x num */
	ret = focal_read_channel_x(&chl_x);
	if (ret) {
		TS_LOG_ERR("%s:get chennel x from ic fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	/* get channel y num */
	ret = focal_read_channel_y(&chl_y);
	if (ret) {
		TS_LOG_ERR("%s:get chennel y from ic fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	if (chl_x <= 0 || chl_x > TX_NUM_MAX) {
		TS_LOG_ERR("%s:channel x value out of range, value = %d\n",
			__func__, chl_x);
		return -EINVAL;
	}

	if (chl_y <= 0 || chl_y > RX_NUM_MAX) {
		TS_LOG_ERR("%s:channel y value out of range, value = %d\n",
			__func__, chl_y);
		return -EINVAL;
	}

	*channel_x = chl_x;
	*channel_y = chl_y;

	return 0;
}

static int focal_get_channel_form_ic(struct focal_test_params *params)
{
	int ret = 0;
	u8 chl_x = 0;
	u8 chl_y = 0;

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory model fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	ret = focal_get_channel_num(&chl_x, &chl_y);
	if (ret) {
		TS_LOG_ERR("%s:get channel num fail, ret=%d\n", __func__, ret);
		return ret;
	} else {
		if(FOCAL_FT3528 == g_focal_dev_data->ic_type && g_focal_pdata->is_v3_pattern) {
			params->channel_x_num = chl_y;
			params->channel_y_num = chl_x;
		} else {
			params->channel_x_num = chl_x;
			params->channel_y_num = chl_y;
		}

		params->key_num = 0;//visual key num
		TS_LOG_INFO("%s:channel_x=%d, channel_y=%d,params->key_num=%d\n",
			__func__, chl_x, chl_y,params->key_num);
	}

	/*
	 * Deleted fts_enter_work here, then need enter factory mode again
	 */

	return 0;
}

static int get_channel_num_no_mapping(u8 *channel_x, u8 *channel_y)
{
	int ret = 0;
	u8 buffer[1] = {0}; /* = new u8; */

	TS_LOG_INFO("get tx num...\n");
	ret = focal_read_reg(REG_TX_NOMAPPING_NUM,  buffer);
	if (ret) {
		TS_LOG_ERR("%s: failed to get Tx number", __func__);
	} else {
		*channel_x = buffer[0];
	}

	TS_LOG_INFO("get rx num...\n");
	ret = focal_read_reg(REG_RX_NOMAPPING_NUM,  buffer);
	if (ret) {
		TS_LOG_ERR("%s: failed to get rx number\n", __func__);
	} else {
		*channel_y = buffer[0];
	}

	return ret;
}

static int mapping_switch(u8 str_switch, int*channel_x, int *channel_y)
{
	int ret = 0;
	u8 chl_x = 0;
	u8 chl_y = 0;
	u8 ch_pattern = 0xFF;
	u8 reg_data = 0xFF;

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	ret = focal_read_reg(REG_PATTERN_5452, &ch_pattern);
	if (ret) {
		TS_LOG_ERR("%s: switch to no_mapping failed!", __func__);
		return ret;
	}

	if (1 == ch_pattern) { /* 1: V3 Pattern */
		reg_data = 0xFF;
		ret = focal_read_reg(REG_MAPPING_SWITCH, &reg_data);
		if (ret) {
			TS_LOG_ERR("%s: read REG_MAPPING_SWITCH failed!", __func__);
			return ret;
		}

		TS_LOG_INFO("get value from REG_PATTERN_5452 is %d", reg_data);

		if (str_switch != reg_data) {
			ret = focal_write_reg(REG_MAPPING_SWITCH, str_switch);  /* 0-mapping 1-no mampping */
			if (ret) {
				TS_LOG_ERR("%s: write REG_MAPPING_SWITCH failed!", __func__);
				return ret;
			}
			msleep(20);//ic need

			if (NO_MAPPING == str_switch) {
				ret = get_channel_num_no_mapping(&chl_x, &chl_y);
				if (ret) {
					TS_LOG_ERR("get_channel_num_no_mapping failed!");
					return ret;
				}
				*channel_x = chl_x;
				*channel_y = chl_y;
			}
		}
	}
	return ret;
}


static int weak_short_get_adc_data(int all_adc_data_len, int *rev_buffer, u8 cmd)
{
    int ret = 0;
    int i = 0;
    int read_data_len = all_adc_data_len; /* offset*2 + (clb_data + tx_num + rx_num)*2*2 */
    u8 *data_send = NULL;
    u8 data = 0xFF;
    u8 w_buffer[2] = {0};
    bool adc_ok = false;
    int byte_num = 0;
    int read_num = 0;
    int packet_num = 0;
    int packet_remainder = 0;
    int scan_query_times = SCAN_QUERY_TIME;

    TS_LOG_INFO("read_data_len is %d", read_data_len);

    data_send = kzalloc(read_data_len + 1, GFP_KERNEL);
    if (NULL == data_send) {
        TS_LOG_ERR("%s:data_send buffer malloc fail\n", __func__);
        return -ENOMEM;
    }
    memset(data_send, 0, read_data_len + 1);

    ret = focal_write_reg(REG_5X46_ADC_SCAN, cmd);/* test weak short once, after host send an enable command */
    if (ret) {
        TS_LOG_ERR("%s:write reg0X07 error. \n", __func__);
        goto ADC_END;
    }

    msleep(100);//ic need

    for (i = 0; i < scan_query_times; i++) {
        msleep(10);//ic need
        ret = focal_read_reg(REG_5X46_ADC_SCAN_STATUS, &data); /* data ready,and FW set reg 0x07 to 0. */
        if (!ret) {
            if (data == 0) {
                adc_ok = true;
                break;
            }
        }
    }

    if ( !adc_ok ) {
        TS_LOG_ERR("ADC data NOT ready, error.");
        ret = -EINVAL;
        goto ADC_END;
    }
    msleep(300);//ic need
    w_buffer[0] = REG_5X46_ADC_DATA_ADDR;
    byte_num= read_data_len;
    packet_num = byte_num / BYTES_PER_TIME;
    packet_remainder = byte_num % BYTES_PER_TIME;
    if (packet_remainder)
        packet_num++;
    if (byte_num < BYTES_PER_TIME) {
        read_num = byte_num;
    } else {
        read_num = BYTES_PER_TIME;
    }

    ret = focal_read(w_buffer, 1, data_send, read_num);
    for (i = 1; i < packet_num; i++) {
        if (ret)
            break;
        if ((i == (packet_num - 1)) && packet_remainder) { /* last packet &*/
            ret = focal_read(NULL, 0, data_send + BYTES_PER_TIME * i, packet_remainder);
        } else {
            ret = focal_read(NULL, 0, data_send + BYTES_PER_TIME * i, BYTES_PER_TIME);
        }
    }

    if (!ret) {
        for (i = 0; i < read_data_len / 2; i++) {
            rev_buffer[i] = (data_send[i << 1] << 8) + data_send[(i << 1) + 1];
        }
    } else {
        TS_LOG_INFO("fts_i2c_read_write error,ret=%d", ret);
    }

ADC_END:
    if(NULL != data_send)
        kfree(data_send);

    return ret;
}

static bool short_test_channel_to_gnd(int *fm_short_resistance, int *adc_data, int offset,
				bool *is_weak_short_gnd, unsigned int chl_x, unsigned int chl_y)
{
	int ret = 0;
	int error_num = 0;
	int min_70k_num = 0;
	int total_num = 0;
	int tx_num = 0;
	int min_cg = 0;
	int min_cc = 0;
	int i = 0;
	int num = 0;
	u8 offset_res = OFFSET_RES;
	int code_1 = SHORT_TEST_STAND_ADC;
	int res_stalls = SHORT_TEST_STAND_RES;
	int fvalue = 0;
	int all_adc_data_num = 0;
	int res_stalls0 = SHORT_TEST_STAND_RES0;
	int *fg_short_resistance = NULL;
	int *tmp_adc_data = NULL;
	u8 *w_buf = NULL;
	u8 *error_ch = NULL;
	u8 *min_70k_ch = NULL;
	bool is_used = false;
	bool tmp_result = true;

	tx_num = chl_x;
	total_num = tx_num + chl_y;
	all_adc_data_num = total_num + 1;
	if(!params || !chl_x || !chl_y) {
		TS_LOG_ERR("%s , params is null\n", __func__);
		return false;
	}

	min_cc = params->threshold.short_circuit_min;
	min_cg = params->threshold.short_circuit_min;

	error_ch = kzalloc(total_num * sizeof(u8), GFP_KERNEL);
	if (NULL == error_ch) {
		TS_LOG_ERR("%s:error_ch buffer malloc fail\n", __func__);
		tmp_result = false;
		goto TEST_END;
	}

	min_70k_ch = kzalloc(total_num * sizeof(u8), GFP_KERNEL);
	if (NULL == min_70k_ch) {
		TS_LOG_ERR("%s:min_70k_ch buffer malloc fail\n", __func__);
		tmp_result = false;
		goto TEST_END;
	}

	w_buf = kzalloc((total_num + 3) * sizeof(u8), GFP_KERNEL);//first byte means reg addr, second byte means error num, reserve one byte.
	if (NULL == w_buf) {
		TS_LOG_ERR("%s:w_buf buffer malloc fail\n", __func__);
		tmp_result = false;
		goto TEST_END;
	}

	tmp_adc_data = kzalloc(total_num * total_num * sizeof(int), GFP_KERNEL);
	if (NULL == tmp_adc_data) {
		TS_LOG_ERR("%s:adc_data buffer malloc fail\n", __func__);
		tmp_result = false;
		goto TEST_END;
	}
	memset(tmp_adc_data, 0, (total_num * total_num));

	fg_short_resistance =  kzalloc(total_num * sizeof(int), GFP_KERNEL);
	if (NULL == fg_short_resistance) {
		TS_LOG_ERR("%s:fg_short_resistance buffer malloc fail\n", __func__);
		tmp_result = false;
		goto TEST_END;
	}
	memset(fg_short_resistance, 0, total_num);

	error_num = 0;
	min_70k_num = 0;

	for ( i = 0; i < total_num; i++) {
		if (fm_short_resistance[i] < min_cc) {
			error_ch[error_num] = (u8)(i + 1);
			error_num++;
		}
	}

	if (error_num > 0) {
		w_buf[0] = SHORT_TEST_ERR_CH0;
		w_buf[1] = (u8)error_num;
		for ( i = 0; i < error_num; i++) {
			w_buf[2 + i] = error_ch[i];
		}

		ret = focal_write(w_buf, (u16)(error_num + 2) );//first byte is reg addr, second byte is error num.
		if (ret) {
			TS_LOG_ERR("failed to write wbuf,ret= %d", ret);
			tmp_result = false;
			goto TEST_END;
		}

		ret = weak_short_get_adc_data(error_num * 2, tmp_adc_data, READ_ADC_CMD_CHL_TO_GND);
		msleep(50);//ic need
		if (ret) {
			TS_LOG_ERR("failed to get weak short data,ret= %d", ret);
			tmp_result = false;
			goto TEST_END;
		}

		TS_LOG_INFO("Ground ADCData:\n");
		for ( i = 0; i < error_num; i++) {
			TS_LOG_INFO(" 0x%x,	", tmp_adc_data[i]);
		}

		for ( i = 0; i < error_num; i++) {
			if (code_1 - tmp_adc_data[i] <= 0) {
				fg_short_resistance[i] = min_cg;
				continue;
			}
			fvalue = (tmp_adc_data[i] - offset + SHORT_TEST_OFFSET_RES) * res_stalls / (code_1 - tmp_adc_data[i]) - offset_res;
			TS_LOG_INFO("short_test_channel_to_gnd:  fvalue is %d",fvalue);
			if (fvalue < 0)
				fvalue = focal_abs(fg_short_resistance[i]);

			if (min_cg > fvalue) {
				fg_short_resistance[error_ch[i] - 1] = fvalue;
				adc_data[error_ch[i] - 1] = tmp_adc_data[i];
				*is_weak_short_gnd = true;
				if (fvalue > LOW_LEVEL_STAND_RES) {
					if (error_ch[i] <= tx_num) {
						TS_LOG_INFO("Tx%d with GND", error_ch[i]);
					} else {
						TS_LOG_INFO( "Rx%d with GND", (error_ch[i] - tx_num) );
					}
						TS_LOG_INFO(": %.02d, ADC: %d\n", fvalue, tmp_adc_data[i]);
						tmp_result = false;
					}

					if (fvalue < LOW_LEVEL_STAND_RES) {
						is_used = false;
						for ( num = 0; num < min_70k_num; num++) {
						if (error_ch[i] == min_70k_ch[num]) {
							is_used = true;
							break;
						}
					}
					if (!is_used) {
						min_70k_ch[min_70k_num] = error_ch[i];
						min_70k_num++;
					}
				}
			}
		}
	}

	if (min_70k_num > 0) {
		ret = focal_write_reg(SHORT_TEST_RES_LEVEL, 0x00);
		if (ret) {
			goto TEST_END;
		}
		memset(tmp_adc_data, 0, total_num * total_num);

		w_buf[0] = SHORT_TEST_ERR_CH0;
		w_buf[1] = (u8)min_70k_num;
		for ( i = 0; i < min_70k_num; i++) {
			w_buf[2 + i] = min_70k_ch[i];
		}

		ret = focal_write(w_buf, (u16)(min_70k_num + 2));//first byte is reg addr, second byte is error num.
		for ( i = 0; i < 1; i++) {
			ret = weak_short_get_adc_data(min_70k_num * 2, tmp_adc_data, READ_ADC_CMD_CHL_TO_GND);
			msleep(50);//ic need
			if (ret) {
			TS_LOG_ERR("failed to get weak short data,ret= %d", ret);
			tmp_result = false;
			goto TEST_END;
			}
		}

		for ( i = 0; i < min_70k_num; i++) {
			if (code_1 - tmp_adc_data[i] <= 0) {
				fg_short_resistance[i] = min_cg;
				continue;
			}

			fvalue = (tmp_adc_data[i] - offset + SHORT_TEST_OFFSET_RES) * res_stalls0 / (code_1 - tmp_adc_data[i]) - offset_res;

			if (fvalue < 0) {
				fvalue = focal_abs(fg_short_resistance[i]);
			}
			if (min_cg > fvalue) {
				fg_short_resistance[min_70k_ch[i] - 1] = fvalue;
				adc_data[min_70k_ch[i] - 1] = tmp_adc_data[i];
				if (min_70k_ch[i] <= tx_num) {
					TS_LOG_INFO("Tx%d with GND", min_70k_ch[i]);
				} else {
					TS_LOG_INFO( "Rx%d with GND", (min_70k_ch[i] - tx_num));
				}

				TS_LOG_INFO(": %.02d, ADC: %d\n", fvalue, tmp_adc_data[i]);
				tmp_result = false;
			}
		}
	}

TEST_END:

	if(NULL != error_ch)
		kfree(error_ch);
	if(NULL != min_70k_ch)
		kfree(min_70k_ch);
	if(NULL != tmp_adc_data)
		kfree(tmp_adc_data);
	if(NULL != fg_short_resistance)
		kfree(fg_short_resistance);
	if(NULL != w_buf)
		kfree(w_buf);

	return tmp_result;
}

static bool short_test_channel_to_channel(int *fm_short_resistance, int *adc_data, int offset,
														unsigned int chl_x, unsigned int chl_y)
{
    int ret = 0;
    int error_num = 0;
    int min_70k_num = 0;
    int total_num = 0;
    int rx_num = 0;
    int tx_num = 0;
    int min_cc = 0;
    int i = 0;
    int j = 0;
    int num = 0;
    int code_1 = SHORT_TEST_STAND_ADC;
    int res_stalls = SHORT_TEST_STAND_RES;
    int fvalue = 0;
    int adc_count = 0;
    int all_adc_data_num = 0;
    int res_stalls0 = SHORT_TEST_STAND_RES0;
    int *f_origin_resistance = NULL;
    int *tmp_adc_data = NULL;
    u8 *w_buf = NULL;
    u8 *error_ch = NULL;
    u8 *min_70k_ch = NULL ;
    bool is_used = false;
    u8 offset_res = OFFSET_RES;
    bool tmp_result = true;
    rx_num = chl_x;
    tx_num = chl_y;
    total_num = rx_num + tx_num;
    all_adc_data_num = total_num + 1;

    if(!params || !chl_x || !chl_y) {
        TS_LOG_ERR("%s, param is null\n", __func__);
        return false;
    }
    min_cc = params->threshold.short_circuit_min;

    f_origin_resistance =  kzalloc(total_num * sizeof(int), GFP_KERNEL);
    if (NULL == f_origin_resistance) {
        TS_LOG_ERR("%s:f_origin_resistance buffer malloc fail\n", __func__);
        tmp_result = false;
        goto TEST_END;
    }
    memset(f_origin_resistance, 0, total_num);

    tmp_adc_data = kzalloc(total_num * total_num * sizeof(int), GFP_KERNEL);
    if (NULL == tmp_adc_data) {
        TS_LOG_ERR("%s:adc_data buffer malloc fail\n", __func__);
        tmp_result = false;
        goto TEST_END;
    }
    memset(tmp_adc_data, 0, (total_num * total_num));

    for ( i = 0; i < total_num; i++) {
        f_origin_resistance[i] = fm_short_resistance[i];
    }

    ret = focal_write_reg(SHORT_TEST_RES_LEVEL, 0x01);
    if (ret) {
        tmp_result = false;
        goto TEST_END;
    }

    w_buf = kzalloc((total_num + 3) * sizeof(u8), GFP_KERNEL);//first byte means reg addr, second byte means error num, reserve one byte.
    if (NULL == w_buf) {
        TS_LOG_ERR("%s:w_buf buffer malloc fail\n", __func__);
        tmp_result = false;
        goto TEST_END;
    }

    error_ch = kzalloc(total_num * sizeof(u8), GFP_KERNEL);
    if (NULL == error_ch) {
        TS_LOG_ERR("%s:error_ch buffer malloc fail\n", __func__);
        tmp_result = false;
        goto TEST_END;
    }
    min_70k_ch = kzalloc(total_num * sizeof(u8), GFP_KERNEL);
    if (NULL == min_70k_ch) {
        TS_LOG_ERR("%s:min_70k_ch buffer malloc fail\n", __func__);
        tmp_result = false;
        goto TEST_END;
    }
    error_num = 0;
    min_70k_num = 0;

    for (i = 0; i < total_num; i++) {
        if (f_origin_resistance[i] < min_cc) {
            error_ch[error_num] = (u8)(i + 1);
            error_num++;
        }
    }
    if (error_num > 1) {
        w_buf[0] = SHORT_TEST_ERR_CH0;
        w_buf[1] = (u8)error_num;
        for ( i = 0; i < error_num; i++) {
            w_buf[2 + i] = error_ch[i];//first byte means reg addr, second byte means error num
        }
//        memset(tmp_adc_data, 0, (all_adc_data_num + 1));

        ret = focal_write(w_buf, (u16)(error_num + 2));//first byte means reg addr, second byte means error num
        for ( j = 0; j < 1; j++) {
            ret = weak_short_get_adc_data(error_num * (error_num - 1) * 2 / 2, tmp_adc_data, READ_ADC_CMD_CHL_TO_CHL);
            msleep(50);//ic need
            if (ret) {
                TS_LOG_ERR("failed to get weak short data,ret= %d", ret);
                tmp_result = false;
                goto TEST_END;
            }
        }

        TS_LOG_INFO("Mutual ADCData:\n");
        for ( i = 0; i < error_num; i++) {
            TS_LOG_INFO("short_test_channel_to_channel:  0x%x,	", tmp_adc_data[i]);
        }

        adc_count = 0;
        for (i = 0; i < error_num; i++) {
            for (j = i + 1; j < error_num; j++) {
                if (code_1 - tmp_adc_data[adc_count] <= 0) {
                    fvalue = min_cc;
                    continue;
                }
                fvalue = (tmp_adc_data[adc_count] - offset + SHORT_TEST_OFFSET_RES) * res_stalls / (code_1 - tmp_adc_data[adc_count]) - offset_res;
                adc_count++;
                TS_LOG_INFO("short_test_channel_to_channel:  fvalue is %d",fvalue);
                if (fvalue < 0) {
                    fvalue = focal_abs(fvalue);
                }
                if (min_cc > fvalue) {
                    fm_short_resistance[error_ch[i] - 1] = fvalue;
                    fm_short_resistance[error_ch[j] - 1] = fvalue;
                    adc_data[error_ch[i] - 1] = tmp_adc_data[adc_count];
                    adc_data[error_ch[j] - 1] = tmp_adc_data[adc_count];

                    if (fvalue > LOW_LEVEL_STAND_RES) {
                        if (error_ch[i] <= tx_num) {
                            TS_LOG_INFO("Tx%d", (error_ch[i]));
                        } else {
                            TS_LOG_INFO("Rx%d", (error_ch[i] - tx_num));
                        }

                        if (error_ch[j] <= tx_num) {
                            TS_LOG_INFO("Tx%d", (error_ch[j] ) );
                        } else {
                            TS_LOG_INFO( "Rx%d", (error_ch[j] - tx_num));
                        }
                        TS_LOG_INFO(": %.02d , ADC: %d\n", fvalue, tmp_adc_data[adc_count]);
                        tmp_result = false;
                    } else {
                        is_used = false;
                        for (num = 0; num < min_70k_num; num++) {
                            if (error_ch[i] == min_70k_ch[num]) {
                                is_used = true;
                                break;
                            }
                        }
                        if (!is_used) {
                            min_70k_ch[min_70k_num] = error_ch[i];
                            min_70k_num++;
                        }
                        is_used = false;
                        for (num = 0; num < min_70k_num; num++) {
                            if (error_ch[j] == min_70k_ch[num]) {
                                is_used = true;
                                break;
                            }
                        }
                        if (!is_used) {
                            min_70k_ch[min_70k_num] = error_ch[j];
                            min_70k_num++;
                        }
                    }
                }
            }
        }
    }

    if (min_70k_num > 0) {
        ret = focal_write_reg(SHORT_TEST_RES_LEVEL, 0x00);
        if (ret) {
            goto TEST_END;
        }
        w_buf[0] = SHORT_TEST_ERR_CH0;
        w_buf[1] = (u8)min_70k_num + 1;
        for (i = 0; i < min_70k_num; i++) {
            w_buf[2 + i] = min_70k_ch[i];
        }

        ret = focal_write(w_buf, (u16)(min_70k_num + 2));
        memset(tmp_adc_data, 0, total_num * total_num);
        for (i = 0; i < 1; i++) {
            ret = weak_short_get_adc_data(min_70k_num * (min_70k_num - 1) * 2 / 2, tmp_adc_data, READ_ADC_CMD_CHL_TO_CHL);
            msleep(50);//ic need
            if (ret) {
                TS_LOG_ERR("failed to get weak short data,ret= %d", ret);
                tmp_result = false;
                goto TEST_END;
            }
        }

        adc_count = 0;
        for (i = 0; i < min_70k_num; i++) {
            for (j = i + 1; j < min_70k_num; j++) {
                adc_count++;
                if (0 >= code_1 - tmp_adc_data[adc_count]) {
                    fvalue = min_cc;
                    continue;
                }
                fvalue = (tmp_adc_data[adc_count] - offset + SHORT_TEST_OFFSET_RES) * res_stalls0 / (code_1 - tmp_adc_data[adc_count]) - offset_res;
                TS_LOG_INFO("short_test_channel_to_channel:  fvalue is %d", fvalue);
                adc_count++;
                if (fvalue < 0) {
                    fvalue = abs(fvalue);
                }
                if (min_cc > fvalue) {
                    fm_short_resistance[min_70k_ch[i] - 1] = fvalue;
                    fm_short_resistance[min_70k_ch[j] - 1] = fvalue;
                    adc_data[min_70k_ch[i] - 1] = tmp_adc_data[adc_count];
                    adc_data[min_70k_ch[j] - 1] = tmp_adc_data[adc_count];
                    if (min_70k_ch[i] <= tx_num) {
                        TS_LOG_INFO("Tx%d", (min_70k_ch[i]));
                    } else {
                        TS_LOG_INFO( "Rx%d", (min_70k_ch[i] - tx_num));
                    }

                    if (min_70k_ch[j] <= tx_num) {
                        TS_LOG_INFO("Tx%d", (min_70k_ch[j]));
                    } else {
                        TS_LOG_INFO( "Rx%d", (min_70k_ch[j] - tx_num));
                    }

                    TS_LOG_INFO(": %.02d, ADC: %d\n", fvalue, tmp_adc_data[adc_count]);

                    tmp_result = false;
                }
            }
        }
    }

TEST_END:
    if(NULL != error_ch)
        kfree(error_ch);

    if(NULL != min_70k_ch)
        kfree(min_70k_ch);

    if(NULL != w_buf)
        kfree(w_buf);
    if(NULL != tmp_adc_data)
        kfree(tmp_adc_data);
    if(NULL != f_origin_resistance)
        kfree(f_origin_resistance);
    return tmp_result;
}


static int focal_alloc_test_container(
	struct focal_test_result **result,
	size_t data_size)
{

	*result = kzalloc(sizeof(struct focal_test_result), GFP_KERNEL);
	if (!*result)
		return -ENOMEM;

	(*result)->size = data_size;
	(*result)->values = kzalloc(data_size * sizeof(int), GFP_KERNEL);
	if (!((*result)->values)) {
		kfree(*result);
		*result = NULL;
		return -ENOMEM;
	}

	return 0;
}

static void focal_free_test_container(struct focal_test_result *result)
{

	if (result){
		if(result->values)
			kfree(result->values);
		kfree(result);
	}
}

static int focal_get_scap_raw_data_format_5x46(int raw_data_type,int *data, size_t size)
{
	unsigned int i = 0;
	unsigned int Count = 0;
	int ret = 0;
	int raw_data_size = 0;
	short raw_data_value = 0;
	u8 *scap_raw_data = NULL;
	if(!data||(0 == size)){
		TS_LOG_ERR("%s: parameter error\n",  __func__);
		return -EINVAL;
	}
	raw_data_size = size*2 ;
	scap_raw_data = kzalloc(raw_data_size*sizeof(int), GFP_KERNEL);
	if (!scap_raw_data) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		return -ENOMEM;
	}
	ret = focal_read_raw_data(raw_data_type, scap_raw_data,
		raw_data_size);
	if (ret) {
		TS_LOG_ERR("%s:read raw data fail, ret=%d\n", __func__, ret);
		goto exit;
	}

	for (i = 0; i < size; i++) {
		Count = i * 2;
		raw_data_value = (scap_raw_data[Count] << 8);
		Count ++;
		raw_data_value |= scap_raw_data[Count];

		data[i] = raw_data_value;
	}

exit:
	if(NULL!=scap_raw_data)
	{
		kfree(scap_raw_data);
	}
	//scap_raw_data = NULL;
	return ret;
}

static int focal_get_raw_data_format(unsigned int chl_x ,unsigned int chl_y,int *data, size_t size)
{
	unsigned int i = 0;
	unsigned int CurTx = 0;
	unsigned int CurRx = 0;
	int ret = 0;
	int raw_data_size = 0;

	short   raw_data_value = 0;
	u8 *original_raw_data = NULL;

	if(!data||(0 == size) ||(0 == chl_x) ||(0 == chl_y)){
		TS_LOG_ERR("%s: parameter error\n",  __func__);
		return -EINVAL;
	}
	raw_data_size = size * 2;//one data need 2 byte

	original_raw_data = kzalloc(raw_data_size, GFP_KERNEL);
	if (!original_raw_data) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		return -ENOMEM;
	}

	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type
			|| (FOCAL_FT3528 == g_focal_dev_data->ic_type)) {
		ret = focal_read_raw_data(DATA_5X46_TYPE_RAW_DATA, original_raw_data,raw_data_size);
	}else{
		ret = focal_read_raw_data(DATA_TYPE_RAW_DATA, original_raw_data,raw_data_size);
	}
	if (ret) {
		TS_LOG_ERR("%s:read raw data fail, ret=%d\n", __func__, ret);
		goto exit;
	}

	for (i = 0; i < size; i++) {
		raw_data_value = (original_raw_data[ i * 2]<<8);
		raw_data_value |= original_raw_data[i * 2 + 1];
		CurTx = i%chl_y;
		CurRx = i /chl_y;
		data[CurTx*chl_x + CurRx] = raw_data_value;
		TS_LOG_DEBUG("%s:CurTx= %d , Rx =%d ,i =%d,raw_data_value= %d \n",__func__, CurTx,CurRx,i,raw_data_value);
	}

exit:
	kfree(original_raw_data);
	original_raw_data = NULL;
	return ret;
}

static int focal_scap_raw_data_test_5x46(
	struct focal_test_params *params,
	struct focal_test_result **result,char test_num)
{
	int i = 0,size=0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int raw_data_min = 0;
	int raw_data_max = 0;
	int raw_data_size = 0;
	int retry = 3;
	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_ERR("%s: test item raw data,%d\n", __func__,test_num);
	if(!params || !result || !(params->channel_x_num)|| !(params->channel_y_num)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	raw_data_min = params->threshold.scap_raw_data_min;
	raw_data_max = params->threshold.scap_raw_data_max;
	raw_data_size = chl_x +chl_y;

	if (g_focal_dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
		test_num = 	RAW_DATA_TYPE_SelfCap;
	}	
	result_code[0] = test_num+'0';
	result_code[1] = RAWDATA_TEST_FAIL_CHAR; //default result_code is failed
	result_code[2] = '\0';
	size=raw_data_size*2;
	ret = focal_alloc_test_container(&test_result, size);
	if (ret) {
		TS_LOG_ERR("%s:alloc raw data container fail, ret=%d\n",
			__func__, ret);
		*result = NULL;
		return ret;
	}
	test_result->testresult = RAWDATA_TEST_FAIL_CHAR;
	test_result->typeindex = RAW_DATA_TYPE_SelfCap;
	test_result->result = true;
	strncpy(test_result->test_name, "Self_Cap_test", FTS_TEST_NAME_LEN - 1);

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
			__func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	/* get scap rawdata with water proof on&off,0xAC:Sc water,0xAB:Sc normal*/
	for (i = 0; i < retry; i++) {
		ret = focal_start_scan();
		if (ret) {
			TS_LOG_ERR("%s:scan fail, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto test_finish;
		}

		ret = focal_get_scap_raw_data_format_5x46(0xAC,test_result->values,
			raw_data_size);
		if (ret) {
			TS_LOG_ERR("%s:get raw data fail, ret=%d\n",
				__func__, ret);

			test_result->result = false;
			goto test_finish;
		}
		ret = focal_get_scap_raw_data_format_5x46(0xAB,test_result->values+raw_data_size,
			raw_data_size);
		if (ret) {
			TS_LOG_ERR("%s:get raw data fail, ret=%d\n",
				__func__, ret);

			test_result->result = false;
			goto test_finish;
		}
	}
	/* compare raw data with shreshold */
	for (i = 0; i < raw_data_size*2-chl_x; i++) {
		if ((test_result->values[i] < raw_data_min)
			|| (test_result->values[i] > raw_data_max)) {

			test_result->result = false;
			strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
			strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
			focal_print_test_data("raw data test fail",
				i / chl_y,
				i % chl_y,
				raw_data_max,
				raw_data_min,
				test_result->values[i]);
		}
	}

test_finish:
	if (test_result->result) {
		result_code[1] = RAWDATA_TEST_PASS_CHAR; //test pass
		test_result->testresult = RAWDATA_TEST_PASS_CHAR;
		TS_LOG_INFO("%s:scap raw data test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:scap raw data test fail\n", __func__);
	}
	tp_cap_test_status =(tp_cap_test_status && test_result->result);
	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN - 1);
	*result = test_result;

	return ret;
}

static int focal_scap_raw_data_test_3528(
	struct focal_test_params *params,
	struct focal_test_result **result,char test_num)
{
	int i = 0,size=0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int raw_data_min = 0;
	int raw_data_max = 0;
	int raw_data_size = 0;
	int act_read_data_size = 0;
	int temp = 0;
	int map_chl_x = 0, map_chl_y = 0;
	int retry = 3;//retry times
	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_ERR("%s: test item raw data,%d\n", __func__,test_num);
	if(!params || !result || !(params->channel_x_num)|| !(params->channel_y_num)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	map_chl_x = chl_x;
	map_chl_y = chl_y;
	raw_data_min = params->threshold.scap_raw_data_min;
	raw_data_max = params->threshold.scap_raw_data_max;

	if(FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		/* if it is V3 pattern, get Tx/Rx num again */
		ret = mapping_switch(NO_MAPPING, &chl_x, &chl_y);
		if (ret || !chl_x || !chl_y) {
			TS_LOG_ERR("%s:switch mapping status fail, ret=%d\n",
				__func__, ret);
			test_result->result = false;
			goto test_finish;
		}
		if(g_focal_pdata->is_v3_pattern) {
			temp = chl_x;
			chl_x= chl_y;
			chl_y = temp;
		}
		/* about FT3528 , no mapping tx num is 14, rx num is 36, after mapping  tx num is 18,  rx num is 26.
		ic need get 14 +36 data. we need print data mapped rx num data per line, in order to keep the
		print data in a neat format and separate from other data,  malloc 3 * Rx = 78 = (14 + 36) + 28, extra 
		28 data is padded with 0. */
		raw_data_size = map_chl_x * 3;
		act_read_data_size = chl_y + chl_x;
	}

	TS_LOG_INFO("%s, x:%d,y :%d\n", __func__, chl_x, chl_y);
	if (g_focal_dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
		test_num = RAW_DATA_TYPE_SelfCap;
	}
	result_code[0] = test_num+'0';
	result_code[1] = RAWDATA_TEST_FAIL_CHAR; //default result_code is failed
	result_code[2] = '\0';
	size=raw_data_size*2;
	/*sef rawdata test needs to get data in waterproof mode and non-waterproof mode.
		so size needs to be multiplied by 2*/
	ret = focal_alloc_test_container(&test_result, size);
	if (ret) {
		TS_LOG_ERR("%s:alloc raw data container fail, ret=%d\n",
			__func__, ret);
		*result = NULL;
		return ret;
	}
	test_result->testresult = RAWDATA_TEST_FAIL_CHAR;
	test_result->typeindex = RAW_DATA_TYPE_SelfCap;
	test_result->result = true;
	strncpy(test_result->test_name, "Self_Cap_test", FTS_TEST_NAME_LEN - 1);

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
			__func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	/* get scap rawdata with water proof on&off,0xAC:Sc water,0xAB:Sc normal*/
	for (i = 0; i < retry; i++) {
		ret = focal_start_scan();
		if (ret) {
			TS_LOG_ERR("%s:scan fail, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto test_finish;
		}

		ret = focal_get_scap_raw_data_format_5x46(0xAC,test_result->values,
			act_read_data_size);
		if (ret) {
			TS_LOG_ERR("%s:get raw data fail, ret=%d\n",
				__func__, ret);

			test_result->result = false;
			goto test_finish;
		}
		ret = focal_get_scap_raw_data_format_5x46(0xAB,test_result->values+raw_data_size,
			act_read_data_size);
		if (ret) {
			TS_LOG_ERR("%s:get raw data fail, ret=%d\n",
				__func__, ret);

			test_result->result = false;
			goto test_finish;
		}
	}
	/* compare raw data with shreshold */
	if(FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		raw_data_size = chl_x  + chl_y;
	}
	TS_LOG_INFO("first value = %d\n", test_result->values[0]);
	for (i = 0; i < raw_data_size; i++) {
		if ((test_result->values[i] < raw_data_min)
			|| (test_result->values[i] > raw_data_max)) {

			test_result->result = false;
			strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
			strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
			focal_print_test_data("raw data test fail",
				i / chl_y,
				i % chl_y,
				raw_data_max,
				raw_data_min,
				test_result->values[i]);
		}
	}
	if(FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		raw_data_size = chl_x;
		for (i = 0; i < raw_data_size; i++) {
			if ((test_result->values[i + map_chl_x*3] < raw_data_min)//map_chl_x*3 is data offset
				|| (test_result->values[i + map_chl_x*3] > raw_data_max)) {

				test_result->result = false;
				strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				focal_print_test_data("raw data test fail",
					i / chl_y,
					i % chl_y,
					raw_data_max,
					raw_data_min,
					test_result->values[i + map_chl_x*3]);
			}
		}
	}

test_finish:
	if(FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		ret = mapping_switch(MAPPING, &chl_x, &chl_y);
		if (ret) {
			TS_LOG_ERR("%s:switch mapping status fail, ret=%d\n",
				__func__, ret);
		}
	}
	if (test_result->result) {
		result_code[1] = RAWDATA_TEST_PASS_CHAR; //test pass
		test_result->testresult = RAWDATA_TEST_PASS_CHAR;
		TS_LOG_INFO("%s:scap raw data test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:scap raw data test fail\n", __func__);
	}
	tp_cap_test_status =(tp_cap_test_status && test_result->result);
	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN - 1);
	*result = test_result;

	return ret;
}


static int focal_raw_data_test(
	struct focal_test_params *params,
	struct focal_test_result **result,char test_num)
{
	int i = 0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int raw_data_min = 0;
	int raw_data_max = 0;
	int raw_data_size = 0;

	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s: test item raw data,%d\n", __func__,test_num);
	if(!params || !result || !(params->channel_x_num)|| !(params->channel_y_num)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	raw_data_min = params->threshold.raw_data_min;
	raw_data_max = params->threshold.raw_data_max;
	raw_data_size = chl_x * chl_y;
	if (g_focal_dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
		test_num = 	RAW_DATA_TYPE_CAPRAWDATA;
	}	
	result_code[0] = test_num+'0';
	result_code[1] = RAWDATA_TEST_FAIL_CHAR; //default result_code is failed
	result_code[2] = '\0';
	ret = focal_alloc_test_container(&test_result, raw_data_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc raw data container fail, ret=%d\n",
			__func__, ret);
		*result = NULL;
		return ret;
	}
	test_result->testresult = RAWDATA_TEST_FAIL_CHAR;
	test_result->typeindex = RAW_DATA_TYPE_CAPRAWDATA;
	test_result->result = true;
	strncpy(test_result->test_name, "Cap_Rawdata", FTS_TEST_NAME_LEN - 1);

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
			__func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type
			|| (FOCAL_FT3528 == g_focal_dev_data->ic_type)) {
		if(FOCAL_FT3528 == g_focal_dev_data->ic_type){
			ret = focal_write_reg(REG_NORMALIZE_TYPE, 0x01);
			if(ret){
				TS_LOG_ERR("%s:write_charnge_fre fail, ret=%d\n", __func__, ret);
				test_result->result = false;
				goto test_finish;
			}
		}
		ret = focal_write_reg(REG_5X46_CHANG_FRE, 0x81);
		if(ret){
			TS_LOG_ERR("%s:write_charnge_fre fail, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto test_finish;
		}

		ret = focal_write_reg(REG_5X46_FIR_EN, 1);
		if(ret){
			TS_LOG_ERR("%s:write_charnge_fre fail, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto test_finish;
		}
	}

	if(FOCAL_FT8719 == g_focal_dev_data->ic_type){
		ret = focal_write_reg(RAWDATA_TEST_REG, 0x01);
		if(ret){
			TS_LOG_ERR("%s:write rawdata test fail, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto test_finish;
		}
	}

	/* start scanning */
	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type
				|| (FOCAL_FT3528 == g_focal_dev_data->ic_type)) {
		for (i = 0; i < 3; i++) {
			ret = focal_start_scan();
			if (ret) {
				TS_LOG_INFO("%s:scan fail, ret=%d\n", __func__, ret);
				test_result->result = false;
				goto test_finish;
			}

			ret = focal_get_raw_data_format(chl_x,chl_y,test_result->values,test_result->size);
			if (ret) {
				TS_LOG_ERR("%s:get raw data fail, ret=%d\n",
					__func__, ret);

				test_result->result = false;
				goto test_finish;
			}
		}
	}else{
		ret = focal_start_scan();
		if (ret) {
			TS_LOG_INFO("%s:scan fail, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto test_finish;
		}

		for (i = 0; i < 3; i++) {
			ret = focal_get_raw_data_format(chl_x,chl_y,test_result->values,test_result->size);
			if (ret) {
				TS_LOG_ERR("%s:get raw data fail, ret=%d\n",
					__func__, ret);

				test_result->result = false;
				goto test_finish;
			}
		}
	}


	if (RAW_POINT_BY_POINT_JUDGE == params->point_by_point_judge) {
		TS_LOG_INFO("%s: test %d\n", __func__, params->point_by_point_judge);
		/* compare raw data with shreshold */
		for (i = 0; i < raw_data_size; i++) {
			if ((test_result->values[i] < params->threshold.raw_data_min_array[i])
				|| (test_result->values[i] > params->threshold.raw_data_max_array[i])) {

				test_result->result = false;
				tp_cap_test_status = PANEL_REASON;
				strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				focal_print_test_data("raw data test fail",
					i / chl_x,
					i % chl_x,
					params->threshold.raw_data_max_array[i],
					params->threshold.raw_data_min_array[i],
					test_result->values[i]);
			}
		}
	} else {
		/* compare raw data with shreshold */
		for (i = 0; i < raw_data_size; i++) {
			if ((test_result->values[i] < raw_data_min)
				|| (test_result->values[i] > raw_data_max)) {

				test_result->result = false;
				tp_cap_test_status = PANEL_REASON;
				strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				focal_print_test_data("raw data test fail",
					i / chl_x,
					i % chl_x,
					raw_data_max,
					raw_data_min,
					test_result->values[i]);
			}
		}
	}

test_finish:
	if(FOCAL_FT8719 == g_focal_dev_data->ic_type){
		ret = focal_write_reg(RAWDATA_TEST_REG, 0x0);
		if(ret){
			TS_LOG_ERR("%s:restore rawdata test fail, ret=%d\n", __func__, ret);
			test_result->result = false;
		}
	}

	if (test_result->result) {
		result_code[1] = RAWDATA_TEST_PASS_CHAR; //test pass
		test_result->testresult = RAWDATA_TEST_PASS_CHAR;
		TS_LOG_INFO("%s:raw data test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:raw data test fail\n", __func__);
	}
	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN - 1);
	*result = test_result;
	return ret;
}

static int focal_ft3528_panel_differ_test(
	struct focal_test_params *params,
	struct focal_test_result **result,char test_num)
{
	int ret = 0;
	int value = 0;
	int i = 0;
	int chl_x = 0;
	int chl_y = 0;
	int panel_differ_data_size = 0;
	u8 origin_rawdata_type = 0xFF;
	u8 origin_frequecy = 0xFF;
	u8 origin_fir_state = 0xFF;

	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s: test item panel differ data,%d\n", __func__,test_num);

	if(!params || !result || !(params->channel_x_num)|| !(params->channel_y_num)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;

	panel_differ_data_size = chl_x * chl_y;

	result_code[0] = test_num+'0';
	result_code[1] = 'F'; //default result_code is failed
	result_code[2] = '\0';
	ret = focal_alloc_test_container(&test_result, panel_differ_data_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc raw data container fail, ret=%d\n",
			__func__, ret);
		*result = NULL;
		return ret;
	}

	test_result->result = true;
	strncpy(test_result->test_name, "panel_differ_test", FTS_TEST_NAME_LEN - 1);

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode failed, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	msleep(FTS_ENTER_FACTORY_DELAY_TIME);

	ret = focal_read_reg(REG_NORMALIZE_TYPE, &origin_rawdata_type);
	if(ret){
		TS_LOG_ERR("read reg 0x16 is fail, ret = %d\n", ret);
		test_result->result = false;
		goto test_finish;
	}

	if (origin_rawdata_type != 0) {
		ret = focal_write_reg(REG_NORMALIZE_TYPE, 0x00);
		msleep(50);//ic need
		if (ret) {
			TS_LOG_ERR("write reg 0x16 failed.");
			test_result->result = false;
			goto restore_rawdata_type;
		}
	}

	ret = focal_read_reg(REG_5X46_CHANG_FRE, &origin_frequecy);
	if(ret){
		TS_LOG_ERR("read reg 0x0A failed,ret = %d\n", ret);
		test_result->result = false;
		goto restore_rawdata_type;
	}

	ret = focal_write_reg(REG_5X46_CHANG_FRE, 0x81);
	if(ret){
		TS_LOG_ERR("write reg 0x0A failed,ret = %d\n", ret);
		test_result->result = false;
		goto restore_frequency_register;
	}

	ret = focal_read_reg(REG_5X46_FIR_EN, &origin_fir_state);
	if(ret){
		TS_LOG_ERR("read reg 0xFB failed,ret = %d\n", ret);
		test_result->result = false;
		goto restore_frequency_register;
	}

	ret = focal_write_reg(REG_5X46_FIR_EN, 0x00);
	if(ret){
		TS_LOG_ERR("write reg 0xFB failed,ret = %d\n", ret);
		test_result->result = false;
		goto close_hardware_filter;
	}

	/* change register value before,need to lose 2 frame data,3th frame data is valid */
	for (i = 0; i < 3; i++) {
		ret = focal_start_scan();
		if (ret) {
			TS_LOG_INFO("%s:scan fail, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto close_hardware_filter;
		}
		ret = focal_get_raw_data_format(chl_x,chl_y,test_result->values,test_result->size);
		if (ret) {
			TS_LOG_ERR("%s:get raw data fail, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto close_hardware_filter;
		}
	}

	/* compare raw data with shreshold */
	for (i = 0; i < panel_differ_data_size; i++) {
		value = test_result->values[i]/PANEL_DIFF_FACTOR;
		test_result->values[i] = value;
		if ((test_result->values[i] < params->threshold.panel_differ_min_array[i])
			|| (test_result->values[i] > params->threshold.panel_differ_max_array[i])) {

			test_result->result = false;
			tp_cap_test_status = PANEL_REASON;
			strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
			focal_print_test_data("panel differ test fail",
				i / chl_x,
				i % chl_x,
				params->threshold.panel_differ_min_array[i],
				params->threshold.panel_differ_max_array[i],
				test_result->values[i]);
		}
	}

close_hardware_filter:
	focal_write_reg(REG_5X46_FIR_EN, origin_fir_state); //restore 0xFB register status
restore_frequency_register:
	focal_write_reg(REG_5X46_CHANG_FRE, origin_frequecy); //setting default frequency
restore_rawdata_type:
	focal_write_reg(REG_NORMALIZE_TYPE, origin_rawdata_type);
test_finish:
	if (test_result->result) {
		result_code[1] = 'P'; //test pass
		TS_LOG_INFO("%s:panel differ test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:panel differ test fail\n", __func__);
	}
	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN - 1);
	*result = test_result;
	return ret;
}


static int focal_ft5x46_lcd_noise_test(
	struct focal_test_params *params, struct focal_test_result **result,char test_num)
{
	int i = 0;
	int ret;
	int chl_x;
	int chl_y;
	int index = 0;
	int inoisevalue=0;

	u8 chfir = 0;
	u8 chvalue = 0x00;
	u8 release_idl = 0;
	u8 release_idh = 0;
	u8 chnoisevalue = 0;
	unsigned short release_id = 0;

	int lcd_noise_data_min = 0;
	int lcd_noise_data_max = 0;
	int lcd_noise_data_size;

	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s: test lcd noise data,%d\n", __func__,test_num);
	if(!params || !result || (params->channel_x_num) <= 0 || (params->channel_y_num) <= 0){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	lcd_noise_data_size = chl_x * chl_y;
	if (g_focal_dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
		test_num = 	RAW_DATA_TYPE_Noise;
	}
	result_code[0] = test_num+'0';
	result_code[1] = RAWDATA_TEST_FAIL_CHAR; //default result_code is failed
	result_code[2] = '\0';
	ret = focal_alloc_test_container(&test_result, lcd_noise_data_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc raw data container fail, ret=%d\n",
			__func__, ret);
		*result = NULL;
		return ret;
	}
	test_result->testresult = RAWDATA_TEST_FAIL_CHAR;
	test_result->typeindex = RAW_DATA_TYPE_Noise;
	test_result->result = true;
	strncpy(test_result->test_name, "noise_delta", FTS_TEST_NAME_LEN - 1);

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode failed, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	msleep(FTS_ENTER_FACTORY_DELAY_TIME);

	ret = focal_write_reg(REG_DATA_TYPE, READ_5X46_READ_DIFF);//Switch read diff mode
	if (ret) {
		TS_LOG_ERR("%s:write reg failed, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	ret = focal_read_reg(REG_DATA_TYPE, &chvalue);
	if (ret || chvalue != 1) {
		TS_LOG_ERR("%s:read reg fail failed, ret=%d, chvalue = %d\n",
			__func__, ret, chvalue);
		test_result->result = false;
		goto restore_rawdata_mode;
	}

	ret = focal_read_reg(REG_5X46_FIR_EN, &chfir);//confirm hardware filter status
	if (ret) {
		TS_LOG_ERR("%s:read reg fail failed, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto restore_rawdata_mode;
	}

	ret = focal_write_reg(REG_5X46_FIR_EN, OPEN_5X46_FIR_SW);//open hardware filter
	if (ret) {
		TS_LOG_ERR("%s:write reg failed, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto restore_rawdata_mode;
	}
	msleep(FTS_OPEN_HDFILT_REG_DELAY_TIME);

	//0x0A is frequency register, write 0x81 for high frequency test
	ret = focal_write_reg(REG_5X46_CHANG_FRE, FTS_5X46_NOISE_TEST_HIGH_RATE);
	if (ret) {
		TS_LOG_ERR("%s:write reg failed, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto close_hardware_filter;
	}

	for ( i=0; i<FTS_RETRY_TIMES; i++) {
		focal_start_scan();
		msleep(FTS_WRITE_REG_DELAY_TIME);
	}

	//write frame
	ret = focal_write_reg(REG_5X46_NOISE_FRE, FTS_5X46_NOISE_TEST_FRAME);
	if (ret) {
		TS_LOG_ERR("%s:write reg failed, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto restore_frequency_register;
	}

	//open noise test switch
	ret = focal_write_reg(REG_5X46_NOISE_TEST_SWITCH, OPEN_5X46_NOISE_SW);
	if (ret) {
		TS_LOG_ERR("%s:write reg failed, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto restore_frequency_register;
	}
	msleep(FTS_WRITE_REG_DELAY_TIME);

	for (index = 0; index < FTS_RETRY_READ_RAWDATA; index++) {
		ret = focal_get_raw_data_format(chl_x, chl_y,
			test_result->values, test_result->size);

		if (0 == ret)
			break;

		if (index >= 3) {
			TS_LOG_ERR("%s:Get Raw Data failed, ret = %d\n", __func__, ret);
			test_result->result = false;
			goto close_noise_test;

		}
	}

	//read ic noise threshold
	ret = focal_read_reg(REG_5X46_MUL_DATA_THRESHOLD, &chnoisevalue);
	if (ret) {
		TS_LOG_ERR("%s:read reg fail failed, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto close_noise_test;
	}

	ret = focal_enter_work();
	if (ret) {
		TS_LOG_ERR("%s:enter work mode failed, ret = %d\n", __func__, ret);
		test_result->result = false;
		goto close_noise_test;
	}
	msleep(FTS_ENTER_WORK_DELAY_TIME);

	//read release id low position
	ret = focal_read_reg(REG_5X46_RELEASE_LOW, &release_idl);
	if (ret) {
		TS_LOG_ERR("%s:read reg fail failed, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto close_noise_test;
	}

	//read release id high position
	ret = focal_read_reg(REG_5X46_RELEASE_HIGH, &release_idh);
	if (ret) {
		TS_LOG_ERR("%s:read reg fail failed, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto close_noise_test;
	}

	release_id = (release_idh<<FTS_5X46_RELEAEHIGH_LEFT_8) + release_idl;
	if (release_id >= FTS_5X46_RELEASE_ID) {
		inoisevalue = chnoisevalue*4;//read chNoiseValue is division 4
	} else {
		inoisevalue = chnoisevalue;
	}

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode failed, ret = %d\n", __func__, ret);
		test_result->result = false;
		goto close_noise_test;
	}
	msleep(FTS_ENTER_FACTORY_DELAY_TIME);

	TS_LOG_INFO("%s:inoisevalue = %d\n ",__func__, inoisevalue);
	lcd_noise_data_max = (params->threshold.lcd_noise_max)*inoisevalue/100;

	lcd_noise_data_min = 0 - lcd_noise_data_max;
	if (0 == chl_x) {
		TS_LOG_ERR("%s: chl_x == 0\n", __func__);
		test_result->result = false;
		goto close_noise_test;
	}

	for (i = 0; i < lcd_noise_data_size; i++) {
		if ((test_result->values[i] < lcd_noise_data_min)
			|| (test_result->values[i] > lcd_noise_data_max)) {
			test_result->result = false;
			tp_cap_test_status = PANEL_REASON;
			strncpy(tp_test_failed_reason, "-panel_reason",
				TP_TEST_FAILED_REASON_LEN);
			strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
			focal_print_test_data("noise test fail", i / chl_x,
				i % chl_x, lcd_noise_data_max,
				lcd_noise_data_min,
				test_result->values[i]);
		}
	}

close_noise_test:
	//close noise test switch
	focal_write_reg(REG_5X46_NOISE_TEST_SWITCH, CLOSE_5X46_FIR_SW);
restore_frequency_register:
	focal_write_reg(REG_5X46_CHANG_FRE, 0x00);//setting default frequency
close_hardware_filter:
	//restore 0xFB register status
	focal_write_reg(REG_5X46_FIR_EN, chfir);
restore_rawdata_mode:
	//Switch read raw mode
	focal_write_reg(REG_DATA_TYPE, READ_5X46_READ_RAW);
test_finish:
	if (test_result->result) {
		result_code[1] = RAWDATA_TEST_PASS_CHAR; //test pass
		test_result->testresult = RAWDATA_TEST_PASS_CHAR;
		TS_LOG_INFO("%s:lcd noise data test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:lcd noise data test fail\n", __func__);
	}
	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN - 1);
	*result = test_result;
	return ret;
}

static void focal_print_test_data(
	char *msg,
	int row,
	int col,
	int max,
	int min,
	int value)
{

	TS_LOG_ERR("%s:%s,data[%d, %d]=%d, range=[%d, %d]\n",
		__func__, msg, row, col, value, min, max);
}

static int focal_get_cb_data_format(int *data, size_t size,unsigned int chl_x,unsigned int chl_y)
{
	unsigned int i = 0, j = 0;
	int ret = 0;
	size_t read_size = 0;
	unsigned int CurTx = 0;
	unsigned int CurRx = 0;
	unsigned int Count = 0;
	char *cb_data = NULL;
	if(!data||(0 == size) ||(0 == chl_x) ||(0 == chl_y)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	read_size = size;
	if(FOCAL_FT3528 == g_focal_dev_data->ic_type) {
	/*needs to get data in waterproof mode and non-waterproof mode.
	and the data use two bytes to save*/
		read_size = (chl_x + chl_y) * 4;
	}
	cb_data = kzalloc(read_size, GFP_KERNEL);
	if (!cb_data) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		return -ENOMEM;
	}

	if(FOCAL_FT8006U == g_focal_dev_data->ic_type){
		/* auto clb */
		ret = focal_chip_clb();
		if (ret) {
			TS_LOG_ERR("%s:clb fail, ret=%d\n", __func__, ret);
			return ret;
		}
	}

	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type
			|| (FOCAL_FT3528 == g_focal_dev_data->ic_type)) {
		ret = focal_get_Scap_cb_data_5x46(0, cb_data, read_size/2);
	}
	else{
		ret = focal_get_cb_data(0, cb_data, read_size);
	}
	if (ret) {
		TS_LOG_INFO("%s: get cb data failed\n", __func__);
		goto free_cb_data;
	}

	memset(data, 0, size);

	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type) {
		for (i = 0; i < size; i++){
			CurTx = i / chl_y;
			CurRx = i % chl_y;
			Count = CurTx*chl_y+ CurRx;
			data[Count] = cb_data[i];
			TS_LOG_DEBUG("%s:CurTx= %d , Rx =%d ,i =%d \n",__func__, CurTx,CurRx,i);
		}
	} else if (FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		for (i = 0; i < read_size/4; i++) {
			data[i] = (cb_data[i << 1] << 8) + cb_data[(i << 1) + 1];
		}
		for (i = 0; i < read_size/4; i++) {
			j = (i << 1) + read_size/2;
			data[size/2 + i] = cb_data[j] + cb_data[j + 1];
		}
	} else{
		for (i = 0; i < size; i++){
			CurTx = i % chl_y; //size = chl_x*chl_y,because  params->key_num =0
			CurRx = i / chl_y;
			Count = CurTx*chl_x + CurRx;
			data[Count] = cb_data[i];
			TS_LOG_DEBUG("%s:CurTx= %d , Rx =%d ,i =%d \n",__func__, CurTx,CurRx,i);
		}
	}

free_cb_data:

	kfree(cb_data);
	cb_data = NULL;

	return ret;
}

static int focal_cb_test(
	struct focal_test_params *params,
	struct focal_test_result **result,char test_num)
{
	int i = 0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int cb_max = 0;
	int cb_min = 0;
	int data_size=0;
	int temp = 0;
	size_t cb_data_size = 0;
	int mapping_chl_x = 0;
	int mapping_chl_y = 0;

	int *cb_data = NULL;
	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s: cb test start\n", __func__);
	if(!params || !result || !(params->channel_x_num) || !(params->channel_y_num)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	cb_min = params->threshold.cb_test_min;
	cb_max = params->threshold.cb_test_max;
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	mapping_chl_x = chl_x;
	mapping_chl_y = chl_y;

	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type) {
		data_size = (chl_x + chl_y)*2;
	}else if(FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		/* if it is V3 pattern, get Tx/Rx num again */
		ret = mapping_switch(NO_MAPPING, &chl_x, &chl_y);
		if (ret || !chl_x|| !chl_y) {
			TS_LOG_ERR("%s:switch mapping status fail, ret=%d\n",
				__func__, ret);
			test_result->result = false;
			goto test_finish;
		}
		if(g_focal_pdata->is_v3_pattern) {
			temp  = chl_x;
			chl_x = chl_y;
			chl_y = temp;
		}
		/* about FT3528 , no mapping tx num is 14, rx num is 36, after mapping  tx num is 18,  rx num is 26.
		ic need get 14 +36 data. we need print data mapped rx num data per line, in order to keep the
		print data in a neat format and separate from other data,  malloc 3 * Rx = 78 = (14 + 36) + 28, extra 
		28 data is padded with 0. Cb test needs to get data in waterproof mode and non-waterproof mode.
		so size needs to be multiplied by 2*/
		data_size = mapping_chl_x * 3 * 2;
	}
	else{
		data_size = chl_x * chl_y + params->key_num;
	}
	cb_data_size=(size_t)(unsigned)(data_size);
	if (g_focal_dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
		test_num = 	RAW_DATA_TYPE_CbCctest;
	}
	result_code[0] =  test_num+'0';
	result_code[1] = RAWDATA_TEST_FAIL_CHAR; //default result_code is failed
	result_code[2] = '\0';
	ret = focal_alloc_test_container(&test_result, cb_data_size);
	if (ret) {
		TS_LOG_INFO("%s:alloc mem for cb test result fail\n", __func__);
		*result = NULL;
		return ret;
	}
	test_result->testresult = RAWDATA_TEST_FAIL_CHAR;
	test_result->typeindex = RAW_DATA_TYPE_CbCctest;
	test_result->result = true;
	strncpy(test_result->test_name, "cb_test", FTS_TEST_NAME_LEN - 1);

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
			__func__, ret);
		test_result->result = false;
		goto test_finish;

	}

	if(FOCAL_FT8719 == g_focal_dev_data->ic_type){
		ret = focal_write_reg(CB_TEST_REG, 0x01);
		if(ret){
			TS_LOG_ERR("%s:write cb test fail, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto test_finish;
		}
	}

	ret = focal_get_cb_data_format(test_result->values, cb_data_size,chl_x,chl_y);
	if (ret) {
		TS_LOG_INFO("%s:get cb data fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type) {
		cb_data_size = cb_data_size - chl_x ;
	}
	if(FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		cb_data_size = chl_x + chl_y ;
	}
	cb_data = test_result->values;
	if(CB_TEST_POINT_BY_POINT ==  params->cb_test_point_by_point) {
		TS_LOG_INFO("%s: cb_data_size is %d\n", __func__,cb_data_size);
		for (i = 0; i < cb_data_size; i++) {
			if ((test_result->values[i] < params->threshold.cb_test_min_array[i])
				|| (test_result->values[i] > params->threshold.cb_test_max_array[i])) {

				test_result->result = false;
				tp_cap_test_status = PANEL_REASON;
				strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				focal_print_test_data("cb test fail",
					i / chl_x,
					i % chl_x,
					params->threshold.cb_test_min_array[i],
					params->threshold.cb_test_max_array[i],
					test_result->values[i]);

			}
		}

	} else {
		for (i = 0; i < cb_data_size; i++) {
			if (cb_data[i] < cb_min || cb_data[i] > cb_max) {

				test_result->result = false;
				tp_cap_test_status  = PANEL_REASON;
				strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				focal_print_test_data("cb test failed",
					i / chl_x,
					i % chl_x,
					cb_max,
					cb_min,
					cb_data[i]);
			}
		}
		if(FOCAL_FT3528 == g_focal_dev_data->ic_type) {
			for (i = 0; i < cb_data_size; i++) {
				if (cb_data[i + mapping_chl_x*3] < cb_min || cb_data[i + mapping_chl_x*3] > cb_max) {//map_chl_x*3 is data offset

					test_result->result = false;
					tp_cap_test_status  = PANEL_REASON;
					strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
					focal_print_test_data("cb test failed",
						i / chl_x,
						i % chl_x,
						cb_max,
						cb_min,
						cb_data[i + mapping_chl_x*3]);
				}
			}
		}
	}

test_finish:
	if(FOCAL_FT8719 == g_focal_dev_data->ic_type){
		ret = focal_write_reg(CB_TEST_REG, 0x0);
		if(ret){
			TS_LOG_ERR("%s:restore cb test fail, ret=%d\n", __func__, ret);
			test_result->result = false;
		}
	}
	if(FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		ret = mapping_switch(MAPPING, &chl_x, &chl_y);
		if (ret) {
			TS_LOG_ERR("%s:switch mapping status fail, ret=%d\n",
				__func__, ret);
		}
	}

	if (test_result->result) {
		result_code[1] = RAWDATA_TEST_PASS_CHAR; //test pass
		test_result->testresult = RAWDATA_TEST_PASS_CHAR;
		TS_LOG_INFO("%s:cb test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:cb test fail!\n", __func__);
	}

	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN);
	*result = test_result; 
	return ret;
}

static int focal_get_open_test_data(struct focal_test_params *params,int *data, size_t size,unsigned int chl_x,unsigned int chl_y)
{
	unsigned int i = 0;
	int ret = 0;
	size_t read_size = 0;
	u8 k1_charge_time = 0;
	u8 k2_reset_time = 0;
	unsigned int CurTx = 0;
	unsigned int CurRx = 0;
	u8 reg_value = 0;
	char *open_data = NULL;
	u8 chGipMode=FTS_STATE_INIT_VALUE, chSDMode = FTS_STATE_INIT_VALUE;
	u8 StateValue = FTS_STATE_INIT_VALUE;
	int WaitTimeOut = FTS_WAIT_TIMES_VALUE;

	if(!data||(0 == size) ||(0 == chl_x) ||(0 == chl_y)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}

	if (judge_ic_type == FTS_8716_VALUE) {
		ret = focal_read_reg(REG_GIP, &reg_value);
		if (ret) {
			TS_LOG_ERR("%s:read gip reg fail, ret=%d\n", __func__, ret);
			return ret;
		}

		ret = focal_write_reg(REG_GIP, 0x02);
		if (ret) {
			TS_LOG_ERR("%s:write gip switch fail, ret=%d\n", __func__, ret);
			return ret;
		}
	}else if (judge_ic_type == FTS_8716U_VALUE) {
		ret = focal_read_reg(FTS_REG_VALUE_0X20, &chGipMode);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("read Reg 0x20 fail\n");
			return ret;
		}

		ret = focal_read_reg(FTS_REG_VALUE_0X21, &chSDMode);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("read Reg 0x21 fail\n");
			return ret;
		}

		ret = focal_write_reg(FTS_REG_VALUE_0X21, FTS_WRITE_VALUE_0X03);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("write Reg 0x21 value 0x03 fail\n");
			return ret;
		}
		msleep(FTS_10MS_MSLEEP_VALUE);

		ret = focal_write_reg(FTS_REG_VALUE_0X20,FTS_WRITE_VALUE_0X02);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("Write Reg 0x20 value 0x02 Fail\n");
			return ret;
		}
		msleep(FTS_40MS_MSLEEP_VALUE);

		while(WaitTimeOut--)
		{
			ret = focal_read_reg(FTS_REG_VALUE_0X0E,&StateValue);
			if (ERROR_CODE_OK != ret) {
				TS_LOG_ERR("read Reg 0x0e fail\n");
				return ret;
			}

			if(ERROR_CODE_OK != StateValue) {
				msleep(FTS_5MS_MSLEEP_VALUE);
			} else {
				break;
			}

			if(FTS_INIT_VALUE == WaitTimeOut) {
				TS_LOG_ERR("Fw auto clb Failed\n");
				return -EIO;
			}
		}
	}
	if(FOCAL_FT8719 == g_focal_dev_data->ic_type){
		if(judge_ic_type == FTS_8719_VALUE) {
			ret = focal_read_reg(FTS_REG_VALUE_0X20, &chGipMode);
			if (ERROR_CODE_OK != ret) {
				TS_LOG_ERR("read Reg 0x20 fail\n");
				return ret;
			}

			ret = focal_read_reg(FTS_REG_VALUE_0X21, &chSDMode);
			if (ERROR_CODE_OK != ret) {
				TS_LOG_ERR("read Reg 0x21 fail\n");
				return ret;
			}

			ret = focal_write_reg(FTS_REG_VALUE_0X20, FTS_WRITE_VALUE_0X01);
			if (ERROR_CODE_OK != ret) {
				TS_LOG_ERR("write Reg 0x21 value 0x02 fail\n");
				return ret;
			}
			msleep(FTS_10MS_MSLEEP_VALUE);

			ret = focal_write_reg(FTS_REG_VALUE_0X21,FTS_WRITE_VALUE_0X01);
			if (ERROR_CODE_OK != ret) {
				TS_LOG_ERR("Write Reg 0x20 value 0x02 Fail\n");
				return ret;
			}

			msleep(FTS_40MS_MSLEEP_VALUE);

			/* set K1/K2 */
			ret = focal_read_reg(FTS_REG_VALUE_0X31, &k1_charge_time);
			if (ERROR_CODE_OK != ret) {
				TS_LOG_ERR("read Reg 0x31 fail\n");
				return ret;
			}
			ret = focal_read_reg(FTS_REG_VALUE_0X32, &k2_reset_time);
			if (ERROR_CODE_OK != ret) {
				TS_LOG_ERR("read Reg 0x32 fail\n");
				return ret;
			}

			if (params->opentest_charge_time) {
				ret = focal_write_reg(FTS_REG_VALUE_0X31, params->opentest_charge_time); /* value from dts */
				if (ERROR_CODE_OK != ret) {
					TS_LOG_ERR("read Reg 0x31 fail\n");
					return ret;
				}
				msleep(FTS_10MS_MSLEEP_VALUE);
			}

			if (params->opentest_reset_time) {
				ret = focal_write_reg(FTS_REG_VALUE_0X32, params->opentest_reset_time); /* value from dts */
				if (ERROR_CODE_OK != ret) {
					TS_LOG_ERR("read Reg 0x32 fail\n");
					return ret;
				}
				msleep(FTS_40MS_MSLEEP_VALUE);
			}

			while(WaitTimeOut--)
			{
				ret = focal_read_reg(FTS_REG_VALUE_0X0E,&StateValue);
				if (ERROR_CODE_OK != ret) {
					TS_LOG_ERR("read Reg 0x0e fail\n");
					return ret;
				}

				if(ERROR_CODE_OK != StateValue) {
					msleep(FTS_5MS_MSLEEP_VALUE);
				} else {
					break;
				}

				if(FTS_INIT_VALUE == WaitTimeOut) {
					TS_LOG_ERR("Fw auto clb Failed\n");
					return -EIO;
				}
			}
		}
	}
	if (FOCAL_FT8006U == g_focal_dev_data->ic_type) {
		ret = focal_read_reg(FTS_REG_VALUE_0X23, &chSDMode);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("read Reg 0x23 fail\n");
			return ret;
		}

		ret = focal_write_reg(FTS_REG_VALUE_0X23, FTS_WRITE_VALUE_0X01);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("write Reg 0x23 value 0x01 fail\n");
			return ret;
		}
		msleep(FTS_40MS_MSLEEP_VALUE);

		while(WaitTimeOut--)
		{
			ret = focal_read_reg(FTS_REG_VALUE_0X0E,&StateValue);
			if (ERROR_CODE_OK != ret) {
				TS_LOG_ERR("read Reg 0x0e fail\n");
				return ret;
			}

			if(ERROR_CODE_OK != StateValue) {
				msleep(FTS_10MS_MSLEEP_VALUE);
			} else {
				break;
			}

			if(FTS_INIT_VALUE == WaitTimeOut) {
				TS_LOG_ERR("Fw auto clb Failed\n");
				return -EIO;
			}
		}
	}
	msleep(50);

	/* auto clb */
	ret = focal_chip_clb();
	if (ret) {
		TS_LOG_ERR("%s:clb fail, ret=%d\n", __func__, ret);
		return ret;
	}

	read_size = size;
	open_data = kzalloc(read_size, GFP_KERNEL);
	if (!open_data) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		return -ENOMEM;
	}

	ret = focal_get_cb_data(0, open_data, read_size);
	if (ret) {
		TS_LOG_INFO("%s:get cb data fail, ret=%d\n", __func__, ret);
		goto free_open_data;
	}

	/* reset reg */
	if (judge_ic_type == FTS_8716_VALUE) {
		ret = focal_write_reg(REG_GIP, reg_value);
		if (ret) {
			TS_LOG_ERR("%s:reset gip reg fail, ret=%d\n", __func__, ret);
			goto free_open_data;
		}
	}else if (judge_ic_type == FTS_8716U_VALUE) {
		ret = focal_write_reg(FTS_REG_VALUE_0X20, chGipMode);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("Write Reg 0x20 value chGipMode Fail\n");
			goto free_open_data;
		}

		ret = focal_write_reg(FTS_REG_VALUE_0X21, chSDMode);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("Write Reg 0x21 value chSDMode Fail\n");
			goto free_open_data;
		}
	}
	if(FOCAL_FT8719 == g_focal_dev_data->ic_type) {
		ret = focal_write_reg(FTS_REG_VALUE_0X31, k1_charge_time);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("write Reg 0x31 fail\n");
			goto free_open_data;
		}
		msleep(FTS_40MS_MSLEEP_VALUE);
		ret = focal_write_reg(FTS_REG_VALUE_0X32, k2_reset_time);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("write Reg 0x32 fail\n");
			goto free_open_data;
		}

		msleep(FTS_40MS_MSLEEP_VALUE);
		ret = focal_write_reg(FTS_REG_VALUE_0X20, chGipMode);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("write Reg 0x20 fail\n");
			goto free_open_data;
		}
		msleep(FTS_40MS_MSLEEP_VALUE);
		ret = focal_write_reg(FTS_REG_VALUE_0X21, chSDMode);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("write Reg 0x21 fail\n");
			goto free_open_data;
		}
	}
	if (FOCAL_FT8006U == g_focal_dev_data->ic_type) {
		ret = focal_write_reg(FTS_REG_VALUE_0X23, chSDMode);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("Write Reg 0x23 value chSDMode Fail\n");
			goto free_open_data;
		}
	}
	msleep(50);

	for (i = 0; i < size; i++){
		CurTx = i % chl_y ; 
		CurRx = i / chl_y; 
		data[CurTx*chl_x + CurRx] = open_data[i];;
		TS_LOG_DEBUG("%s:CurTx= %d , Rx =%d ,i =%d \n",__func__, CurTx,CurRx,i);
	}
free_open_data:
	kfree(open_data);

	return ret;
}

static int focal_open_test(
	struct focal_test_params *params,
	struct focal_test_result **result,char test_num)
{
	int i = 0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	size_t read_size = 0;
	int panel_data_max = 0;
	int panel_data_min = 0;

	int *panel_data = NULL;
	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s:open test\n", __func__);
	if(!params || !result || !(params->channel_x_num) || !(params->channel_y_num)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	read_size = chl_x * chl_y + params->key_num;
	if (g_focal_dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
		test_num = 	RAW_DATA_TYPE_OpenShort;
	}
	result_code[0] = test_num+ '0';
	result_code[1] = RAWDATA_TEST_FAIL_CHAR; //default result_code is failed
	result_code[2] = '\0';
	ret = focal_alloc_test_container(&test_result, read_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		*result = NULL;
		return ret;
	}
	test_result->testresult = RAWDATA_TEST_FAIL_CHAR;
	test_result->typeindex = RAW_DATA_TYPE_OpenShort;
	test_result->result = true;
	strncpy(test_result->test_name, "open_test", FTS_TEST_NAME_LEN - 1);

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory model fail, ret=%d\n",
			__func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	if(FOCAL_FT8719 == g_focal_dev_data->ic_type){
		ret = focal_write_reg(OPEN_TEST_REG, 0x01);
		if(ret){
			TS_LOG_ERR("%s:write open test fail, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto test_finish;
		}
	}

	ret = focal_get_open_test_data(params, test_result->values, read_size,chl_x,chl_y);
	if (ret) {
		TS_LOG_ERR("%s:get open test data fail\n", __func__);
		test_result->result = false;
		goto test_finish;
	}

	panel_data = test_result->values;
	panel_data_min = params->threshold.open_test_cb_min;
	panel_data_max = MAX_CB_VALUE;
	if(OPEN_TEST_POINT_BY_POINT ==  params->open_test_cb_point_by_point) {
		TS_LOG_INFO("%s: open_data_size is %d\n", __func__,test_result->size);
		for (i = 0; i < test_result->size; i++) {
			if ((test_result->values[i] < params->threshold.open_test_cb_min_array[i])
				|| (test_result->values[i] > params->threshold.open_test_cb_max_array[i])) {

				test_result->result = false;
				tp_cap_test_status = PANEL_REASON;
				strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				focal_print_test_data("open test fail",
					i / chl_x,
					i % chl_x,
					params->threshold.open_test_cb_min_array[i],
					params->threshold.open_test_cb_max_array[i],
					test_result->values[i]);

			}
		}

	} else {
		for (i = 0; i < test_result->size; i++) {
			if ((panel_data[i] > panel_data_max)
				|| (panel_data[i] < panel_data_min)) {

				test_result->result = false;
				tp_cap_test_status = PANEL_REASON;
				strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				focal_print_test_data("open test failed",
						i / chl_x,
						i % chl_x,
						panel_data_max,
						panel_data_min,
						panel_data[i]);
			}
		}
	}

test_finish:
	if(FOCAL_FT8719 == g_focal_dev_data->ic_type){
		ret = focal_write_reg(OPEN_TEST_REG, 0x0);
		if(ret){
			TS_LOG_ERR("%s:restore open test fail, ret=%d\n", __func__, ret);
			test_result->result = false;
		}
	}

	if (test_result->result) {
		result_code[1] = RAWDATA_TEST_PASS_CHAR; //test pass
		test_result->testresult = RAWDATA_TEST_PASS_CHAR;
		TS_LOG_INFO("%s:open test pass!\n", __func__);
	} else {
		TS_LOG_ERR("%s: open test fail!\n", __func__);
	}
	if(FOCAL_FT8006U == g_focal_dev_data->ic_type){
		/* auto clb */
		ret = focal_chip_clb();
		if (ret) {
			TS_LOG_ERR("%s:clb fail, ret=%d\n", __func__, ret);
			*result = test_result;
			return ret;
		}
    }

	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN);
	*result = test_result;
	return ret;
}

static int focal_get_short_circuit_data(int *data, size_t size,unsigned int chl_x,unsigned int chl_y)
{
	int i = 0;
	int ret = 0;
	int adc_data_tmp = 0;

	if(!data||(0 == size) ||(0 == chl_x) ||(0 == chl_y)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}

	ret = focal_get_adc_data(data, size,chl_x,chl_y);
	if (ret) {
		TS_LOG_ERR("%s:get adc data fail, ret=%d\n", __func__, ret);
		return ret;
	}

	/* data check */
	for (i = 0; i < size; i++) {
		if(FOCAL_FT8607 == g_focal_dev_data->ic_type){
			if (MAX_ADC_NUM_FT8607< data[i]) {
				TS_LOG_DEBUG("%s:%s,adc_data[%d]=%d\n", __func__,
					"adc data out of range", i, data[i]);
				data[i] = MAX_ADC_NUM_FT8607;
			}
		}
		else if(FOCAL_FT8719 == g_focal_dev_data->ic_type) {
			if (MIN_ADC_NUM_FT8719 > data[i]) {
				TS_LOG_DEBUG("%s:%s,adc_data[%d]=%d\n", __func__,
					"adc data out of range", i, data[i]);
				data[i] = MIN_ADC_NUM_FT8719;
			}
		}else if(FOCAL_FT8006U == g_focal_dev_data->ic_type){
			if (MIN_ADC_NUM_FT8006U > data[i]) {
				TS_LOG_DEBUG("%s:%s,adc_data[%d]=%d\n", __func__,
					"adc data out of range", i, data[i]);
				data[i] = MIN_ADC_NUM_FT8006U;
			}else if(MAX_ADC_NUM_FT8006U < data[i]){
				TS_LOG_DEBUG("%s:%s,adc_data[%d]=%d\n", __func__,
					"adc data out of range", i, data[i]);
				data[i] = MAX_ADC_NUM_FT8006U;
			}
		}
		else{
			if (MAX_ADC_NUM <= data[i]) {
				TS_LOG_DEBUG("%s:%s,adc_data[%d]=%d\n", __func__,
					"adc data out of range", i, data[i]);
				data[i] = MAX_ADC_NUM - 1;
			}
		}
			
	}

	/* data exchange */
	if(FOCAL_FT8719 == g_focal_dev_data->ic_type || FOCAL_FT8006U == g_focal_dev_data->ic_type) {
		for (i = 0; i < size; i++) {
				adc_data_tmp = ADC_EX_RESIS_1_FT8719/(data[i]-ADC_EX_RESIS_2_FT8719)-ADC_EX_RESIS_3_FT8719;
				data[i] = adc_data_tmp;
		}
	}else{
		for (i = 0; i < size; i++) {
			adc_data_tmp = data[i] * 100 / (MAX_ADC_NUM - data[i]);
			data[i] = adc_data_tmp;
		}
	}

	return 0;
}

static int focal_get_short_circuit_data_5x46(int *data, size_t size,unsigned int chl_x,unsigned int chl_y)
{
	int i = 0;
	int ret = 0;
	int adc_data_tmp = 0;
	int read_size;
	int *read_data = NULL;
	int data_offset = 0, clbdata_ground = 0,clbdata_mutual = 0;
	int clbChannelNum = 0;
	int cg_data = 0,cc_data = 0,data_cal = 0,mutual = 0;
	int short_data_pass = 2000;
	int RSense = 57;
	int ground_read_data_count = 0;
	int channel_read_data_count = 0;
	int channel_data_count = 0;
	unsigned char IcValue = 0;

	if(!data||(0 == size) ||(0 == chl_x) ||(0 == chl_y)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}

	clbChannelNum = size/2;
	read_size = size+3;//79
	read_data = kzalloc(read_size*sizeof(int), GFP_KERNEL);
	if (!read_data) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		return -ENOMEM;
	}
	ret = focal_enter_work();
	if (ret) {
		TS_LOG_ERR("%s:enter_work fail, ret=%d\n", __func__, ret);
		goto free_mem;
	}
	ret = focal_read_reg(REG_5X46_IC_VERSION, &IcValue);
	if (ret) {
		TS_LOG_ERR("%s:get IC_VERSION fail, ret=%d\n", __func__, ret);
		goto free_mem;
	}
	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:get IC_VERSION fail, ret=%d\n", __func__, ret);
		goto free_mem;
	}
	ret = focal_get_adc_data(read_data, read_size,chl_x,chl_y);
	if (ret) {
		TS_LOG_ERR("%s:get adc data fail, ret=%d\n", __func__, ret);
		goto free_mem;
	}

	/* data check */

	data_offset = read_data[0]-1024;
	clbdata_ground = read_data[1];
	clbdata_mutual = read_data[2+size/2];
	data_cal = clbdata_mutual > (data_offset+116) ? clbdata_mutual : (data_offset+116);

	/* data exchange */
	for (i = 0; i < clbChannelNum; i++) {
		/* Channel and Ground */
		ground_read_data_count = i+2;
		cg_data = read_data[ground_read_data_count];
		if((2047 + data_offset) - cg_data <= 0)
			adc_data_tmp = short_data_pass;
		else{
		        if (IcValue <= 0x05 || IcValue == 0xff){
		        	adc_data_tmp = (cg_data -data_offset +410)* 25/( 2047 + data_offset - cg_data ) -3;
		        }
			else{
				if (clbdata_ground-cg_data<=0)
					adc_data_tmp = short_data_pass;
				else
					adc_data_tmp = (cg_data -data_offset +384)/(clbdata_ground-cg_data)*57-1;
			}
		}
		if(adc_data_tmp<0)
			adc_data_tmp = 0;

		data[i] = adc_data_tmp;

		/* Channel and Channel */

		channel_read_data_count = i+clbChannelNum+3;
		cc_data = read_data[channel_read_data_count];//i+3+38

		if (IcValue <= 0x05 || IcValue == 0xff){
			if(cc_data - clbdata_mutual < 0){
				adc_data_tmp = short_data_pass;
			}
			else{
				mutual = cc_data - data_cal;//2+38
				mutual = mutual ? mutual : 1;
				adc_data_tmp =( (2047 + data_offset - data_cal)*24/mutual -27) -6;
			}
		}
		else{
			if ( clbdata_mutual  - cc_data<= 0){
				adc_data_tmp = short_data_pass;
				}
			else
				adc_data_tmp = ( cc_data - data_offset - 123 ) * RSense / (clbdata_mutual - cc_data /*temp*/ ) - 2;
		}
		if((adc_data_tmp <0) && (adc_data_tmp >=(-240)))
			adc_data_tmp = 0;
		else if(adc_data_tmp <(-240)){
			adc_data_tmp = short_data_pass;
		}
		channel_data_count = i+clbChannelNum;
		data[channel_data_count] = adc_data_tmp;
	}
free_mem:
	if(NULL!=read_data)
	{
		kfree(read_data);
	}
	//read_data = NULL;
	return 0;
}

static int focal_set_lcd_noise_data_type(void)
{
	int i = 0;
	int ret = 0;
	int iLCDNoiseTestFrame = 50;//ft8607 lcd noise frame defauit num = 100

	/*write diff mode*/
	ret = focal_write_reg(REG_DATA_TYPE, 1);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		return ret;
	}
	ret = focal_write_reg(0x01, 0xAD);//clean counter
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		return ret;
	}
	ret = focal_write_reg(REG_8607_LCD_NOISE_FRAME, iLCDNoiseTestFrame);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		return ret;
	}
	return 0;
}

static int focal_lcd_noise_test(
	struct focal_test_params *params,
	struct focal_test_result **result,char test_num)
{
	int i = 0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int lcd_data_size = 0;
	int lcd_noise_max =0;
	int lcd_noise_min =0;
	unsigned char lcd_value = 0;
	unsigned char	oldMode = 0;
	unsigned char status = 0xff;

	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s:start\n", __func__);
	if(!params || !result || !(params->channel_x_num) || !(params->channel_y_num)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	lcd_data_size = chl_x * chl_y + params->key_num;
	if (g_focal_dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
		test_num = 	RAW_DATA_TYPE_Noise;
	}
	result_code[0] = test_num +'0';
	result_code[1] = RAWDATA_TEST_FAIL_CHAR; //default result_code is failed
	result_code[2] = '\0';
	
	ret = focal_alloc_test_container(&test_result, lcd_data_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		*result = NULL;
		return ret;
	}
TS_LOG_INFO("%s:focal_alloc_test_container\n",__func__);
	test_result->result = true;
	test_result->testresult = RAWDATA_TEST_FAIL_CHAR;
	test_result->typeindex = RAW_DATA_TYPE_Noise;
	strncpy(test_result->test_name,
		"noise_delta", FTS_TEST_NAME_LEN - 1);

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
			__func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	TS_LOG_INFO("%s:focal_enter_factory\n",__func__);
	/*read old data type*/
	ret = focal_read_reg(REG_DATA_TYPE, &oldMode);
	if (ret) {
		TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
		goto test_finish;
	}
	ret = focal_set_lcd_noise_data_type();
	if (ret) {
		TS_LOG_ERR("%s:set data type  fail, ret=%d\n",
			__func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	
	ret = focal_write_reg(REG_8607_LCD_NOISE_START, 0x01);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		goto test_finish;
	}
	TS_LOG_DEBUG("%s:start lcd noise test\n",__func__);
	msleep(FTS_LCD_NOISE_TEST_FRAME_COUNT * FTS_LCD_PRE_FRAME_TIME);
	for ( i = 0; i < 10; i++ )
  	  {
		 ret = focal_read_reg(REG_8607_LCD_NOISE_START, &status);
		if (ret) {
			TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
			goto test_finish;
		}
	        if ( ERROR_CODE_OK == ret && status == 0x00  )
	        {
	            TS_LOG_INFO(" read register 0x%x, get value is:%d. \n ", REG_8607_LCD_NOISE_START, status);
	            break;
	        }
		msleep(50);
	}
	if ( i == 10 )
  	{
           test_result->result = false;
	    TS_LOG_ERR("Scan Noise Time Out!\n");
	    goto test_finish;
    	}
	TS_LOG_INFO("%s:start read raw data\n",__func__);
	for (i = 0; i < 3; i++) {
		ret = focal_get_raw_data_format(chl_x,chl_y,test_result->values,
			test_result->size);
		if (ret) {
			TS_LOG_ERR("%s:get raw data fail, ret=%d\n",
				__func__, ret);
			test_result->result = false;
			goto test_finish;
		}
	}
	TS_LOG_INFO("%s: read raw data  success\n",__func__);
	ret = focal_enter_work();
	if (ret) {
		TS_LOG_ERR("%s:enter work mode fail, ret=%d\n",
			__func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	msleep(100);
	ret = focal_read_reg(REG_8607_LCD_NOISE_VALUE, &lcd_value);
	if (ret) {
		TS_LOG_ERR("%s :reed reg dats read  fail, ret=%d\n", __func__, ret);
		goto test_finish;
	}
	TS_LOG_INFO("get lcd  reg value: %d\n",lcd_value);
	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
			__func__, ret);
		test_result->result = false;
		goto test_finish;
	}
//	msleep(100);
	
	ret = focal_write_reg(REG_DATA_TYPE, oldMode);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		goto test_finish;
	}

	for (i = 0; i < lcd_data_size; i++) {
		test_result->values[i] = focal_abs(test_result->values[i]);
	}
	
	lcd_noise_max = (params->threshold.lcd_noise_max) * lcd_value *32/100;
	/* data compare */
	for (i = 0; i < lcd_data_size; i++) {
		if ((test_result->values[i] >lcd_noise_max )||(test_result->values[i]<lcd_noise_min))
		{
			tp_cap_test_status = PANEL_REASON;
			strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
			strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
			test_result->result = false;
			TS_LOG_ERR("%s:LCD Noise test failured,data[%d] = %d",
				__func__, i, test_result->values[i]);
		}
	}

test_finish:
	if (test_result->result) {
		result_code[1] = RAWDATA_TEST_PASS_CHAR; //test pass
		test_result->testresult = RAWDATA_TEST_PASS_CHAR;
		TS_LOG_INFO("%s:lcd noise test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:lcd noise test fail\n", __func__);
	}
	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN);
	*result = test_result;
	ret = focal_write_reg(REG_DATA_TYPE, oldMode);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		return ret;
	}
	return ret;
}

static int focal_ft8716_lcd_noise_test(
	struct focal_test_params *params,
	struct focal_test_result **result,char test_num)
{
	int i = 0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int lcd_data_size = 0;
	int lcd_noise_max = 0;
	int lcd_noise_min = 0;
	int framenum = 0;
	unsigned char lcd_value = 0;
	unsigned char  oldmode = 0;
	unsigned char  newmode = 0;
	unsigned char regdata = 0;
	unsigned char dataready = 0;
	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s:start\n", __func__);
	if (!params || !result || !(params->channel_x_num) || !(params->channel_y_num)) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	lcd_data_size = chl_x * chl_y + params->key_num;
	if (g_focal_dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
		test_num = 	RAW_DATA_TYPE_Noise;
	}
	result_code[0] = test_num +'0';
	result_code[1] = RAWDATA_TEST_FAIL_CHAR; //default result_code is failed
	result_code[2] = '\0';

	ret = focal_alloc_test_container(&test_result, lcd_data_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		*result = NULL;
		return ret;
	}
	TS_LOG_INFO("%s:focal_alloc_test_container\n",__func__);
	test_result->testresult = RAWDATA_TEST_FAIL_CHAR;
	test_result->typeindex = RAW_DATA_TYPE_Noise;
	test_result->result = true;
	strncpy(test_result->test_name, "noise_delta", FTS_TEST_NAME_LEN - 1);

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	TS_LOG_INFO("%s:focal_enter_factory\n",__func__);
	msleep(FTS_LCD_NOISE_TEST_DELAY_TIME);
	/*read old data type*/
	ret = focal_read_reg(REG_DATA_TYPE, &oldmode);
	if (ret) {
		TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
		goto test_finish;
	}
	/*write diff mode*/
	ret = focal_write_reg(REG_DATA_TYPE, 0x01);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		goto test_finish;
	}
	msleep(FTS_LCD_NOISE_TEST_DELAY_TIME);
	/*read new data type*/
	ret = focal_read_reg(REG_DATA_TYPE, &newmode);
	if (ret != ERROR_CODE_OK || newmode != 1) {
		TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	if(FOCAL_FT8719 == g_focal_dev_data->ic_type){
		/*clean counter*/
		ret = focal_write_reg(FTS_REG_DATA_TYPE, DATA_TYPE_RAW_DATA);
		if (ret) {
			TS_LOG_ERR("%s:write reg dats read	fail, ret=%d\n", __func__, ret);
			goto test_finish;
		}
	}
	/*write test framenum */
	framenum = LCD_TEST_FRAMENUM/4;//100 is test framenum
	ret = focal_write_reg(REG_8716_LCD_NOISE_FRAME, framenum);//0x12
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	if(FOCAL_FT8006U == g_focal_dev_data->ic_type){
		/*clean counter*/
		ret = focal_write_reg(FTS_REG_DATA_TYPE, DATA_TYPE_RAW_DATA);
		if (ret) {
			TS_LOG_ERR("%s:write reg dats read	fail, ret=%d\n", __func__, ret);
			return ret;
		}
	}

	//Set the parameters after the delay for a period of time waiting for FW to prepare
	msleep(FTS_LCD_NOISE_TEST_DELAY_TIME);
	ret = focal_write_reg(REG_8716_LCD_NOISE_START, 0x01);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		goto test_finish;
	}
	TS_LOG_INFO("%s:start lcd noise test\n",__func__);
	if (FOCAL_FT8719 == g_focal_dev_data->ic_type) {
		focal_write_flag = true;
		msleep(LCD_TEST_FRAMENUM * 8);
		for ( i=0; i<=TEST_TIMEOUT; i++) {
			msleep(FTS_LCD_NOISE_TEST_DELAY_TIME);
			ret = focal_read_reg(REG_8716_LCD_NOISE_START, &dataready);
			if (ret) {
				TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
				goto test_finish;
			}

			TS_LOG_INFO("%s,dataready[%d]\n", __func__, dataready);
			if (dataready == REG_8719_LCD_NOISE_READY) {
				break;
			}
		}
	}else if (FOCAL_FT8006U == g_focal_dev_data->ic_type)
		{
		    focal_write_flag = true;
			for ( i=0; i<=TEST_TIMEOUT; i++) {
				msleep(FTS_LCD_NOISE_TEST_DELAY_TIME);
				ret = focal_read_reg(REG_8716_LCD_NOISE_START, &dataready);//0x00
				if (ret) {
					TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
					goto test_finish;
				}

				TS_LOG_INFO("%s,dataready[%d]\n", __func__, dataready);

				if (0x00 == dataready) {
					break;
				}
			}
		}
	else{
		for ( i=0; i<=TEST_TIMEOUT; i++) {
			msleep(FTS_LCD_NOISE_TEST_DELAY_TIME);
			ret = focal_read_reg(REG_LCD_NOISE_DATA_READY, &dataready);
			if (ret) {
				TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
				goto test_finish;
			}

			if (0x00 == (dataready>>7)) {
				msleep(FTS_5MS_MSLEEP_VALUE);
				ret = focal_read_reg(REG_8716_LCD_NOISE_START, &dataready);//0x11
				if (ret) {
					TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
					goto test_finish;
				}
				if (dataready == 0x00) {
					break;
				}
			}
		}
	}

	if (TEST_TIMEOUT == i) {
		ret = focal_write_reg(REG_8716_LCD_NOISE_START, 0x00);
		if (ret) {
			TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		}
		TS_LOG_ERR("%s:lcd noise test Time out, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	TS_LOG_INFO("%s:start read raw data i = %d\n",__func__, i);
	ret = focal_get_raw_data_format(chl_x,chl_y, test_result->values, test_result->size);
	if (ret) {
		TS_LOG_ERR("%s:get raw data fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	if (FOCAL_FT8719 == g_focal_dev_data->ic_type || FOCAL_FT8006U == g_focal_dev_data->ic_type) {
		focal_write_flag = false;
	}

	TS_LOG_INFO("%s: read raw data  success\n",__func__);
	ret = focal_write_reg(REG_8716_LCD_NOISE_START, 0x00);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	ret = focal_read_reg(REG_LCD_NOISE_NUMBER, &regdata);//0x13
	if (ret) {
		TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	ret = focal_write_reg(REG_DATA_TYPE, oldmode);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		goto test_finish;
	}

	if(FOCAL_FT8719 == g_focal_dev_data->ic_type) {
		lcd_value = g_focal_pdata->lcd_noise_threshold;
		TS_LOG_INFO("get lcd reg value from lcd_noise_threshold: %d\n",lcd_value);
	}else{
		ret = focal_enter_work();
		if (ret) {
			TS_LOG_ERR("%s:enter work mode fail, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto test_finish;
		}
		msleep(100);
		ret = focal_read_reg(REG_8716_LCD_NOISE_VALUE, &lcd_value);//0x80
		if (ret) {
			TS_LOG_ERR("%s :reed reg dats read  fail, ret=%d\n", __func__, ret);
			goto test_finish;
		}
		TS_LOG_INFO("get lcd  reg value: %d\n",lcd_value);
		ret = focal_enter_factory();
		if (ret) {
			TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto test_finish;
		}
	}
	lcd_noise_max = (params->threshold.lcd_noise_max) * lcd_value *LEFT_SHIFT_FOUR/PERCENTAGE_DATA;
	/* data compare */
	for (i = 0; i < lcd_data_size; i++) {
		if ((test_result->values[i] >lcd_noise_max )||(test_result->values[i]<lcd_noise_min)) {
			tp_cap_test_status = PANEL_REASON;
			strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
			strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
			test_result->result = false;
			TS_LOG_ERR("%s:LCD Noise test failured,data[%d] = %d", __func__, i, test_result->values[i]);
		}
	}

test_finish:
	if (test_result->result) {
		result_code[1] = RAWDATA_TEST_PASS_CHAR; //test pass
		test_result->testresult = RAWDATA_TEST_PASS_CHAR;
		TS_LOG_INFO("%s:lcd noise test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:lcd noise test fail\n", __func__);
	}
	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN);
	*result = test_result;
	ret = focal_write_reg(REG_DATA_TYPE, oldmode);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		return ret;
	}

	if(FOCAL_FT8719 == g_focal_dev_data->ic_type || FOCAL_FT8006U == g_focal_dev_data->ic_type){
		msleep(FTS_50MS_MSLEEP_VALUE);
	}
	return ret;
}

static int focal_short_circuit_test(
	struct focal_test_params *params,
	struct focal_test_result **result,char test_num)
{
	int i = 0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int adc_data_size = 0;
	int short_data_min = 0;
	int judge_count = SHORT_CIRCUIT_COUNT;

	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s:short circuit test start\n", __func__);
	if(!params || !result || !(params->channel_x_num) || !(params->channel_y_num)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;

	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type) {
 		adc_data_size = (chl_x + chl_y)*2;
	}
	else{
		adc_data_size = chl_x * chl_y + params->key_num;
	}
	if (g_focal_dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
		test_num = 	RAW_DATA_TYPE_OpenShort;
	}
	result_code[0] = test_num + '0';
	result_code[1] = RAWDATA_TEST_FAIL_CHAR; //default result_code is failed
	result_code[2] = '\0';
	ret = focal_alloc_test_container(&test_result, adc_data_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		*result = NULL;
		return ret;
	}
	while(judge_count > 0){
			
		test_result->testresult = RAWDATA_TEST_FAIL_CHAR;
		test_result->typeindex = RAW_DATA_TYPE_OpenShort;
		test_result->result = true;
		strncpy(test_result->test_name,
			"short_test", FTS_TEST_NAME_LEN - 1);

		ret = focal_enter_factory();
		if (ret) {
			TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
				__func__, ret);
			test_result->result = false;
			goto test_finish;
		}
		if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type) {
			ret = focal_get_short_circuit_data_5x46(test_result->values,
			adc_data_size,chl_x,chl_y);
		}
		else{
			ret = focal_get_short_circuit_data(test_result->values,adc_data_size,chl_x,chl_y);
		}
		if (ret) {
			TS_LOG_ERR("%s:get short circuit data fail, ret=%d\n",
				__func__, ret);
			test_result->result = false;
			goto test_finish;
		}

		/* data compare */
		short_data_min = params->threshold.short_circuit_min;
		if(SHORT_TEST_POINT_BY_POINT == params->short_test_point_by_point) {
			TS_LOG_INFO("%s: short_data_size is %d\n", __func__,test_result->size);
			for (i = 0; i < adc_data_size; i++) {
				if ((test_result->values[i] < params->threshold.short_test_min_array[i])) {

					test_result->result = false;
					tp_cap_test_status = PANEL_REASON;
					strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
					strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
					if(FOCAL_FT8006U != g_focal_dev_data->ic_type){
						focal_print_test_data("short circuit test fail",
							i / chl_x,
							i % chl_x,
							0,
							params->threshold.short_test_min_array[i],
							test_result->values[i]);
					}

				}
			}			
		} else {
			for (i = 0; i < adc_data_size; i++) {
				if (short_data_min > test_result->values[i]) {

					test_result->result = false;
					tp_cap_test_status = PANEL_REASON;
					strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
					strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
					if(FOCAL_FT8006U != g_focal_dev_data->ic_type){
						focal_print_test_data("short circuit test fail:",
							i / chl_x,
							i % chl_x,
							0,
							short_data_min,
							test_result->values[i]);
					}
				}
			}
		}
		if (test_result->result == false) {
			judge_count--;
		} else {
			break;
		}
	}
test_finish:
	if (test_result->result) {
		result_code[1] = RAWDATA_TEST_PASS_CHAR; //test pass
		test_result->testresult = RAWDATA_TEST_PASS_CHAR;
		TS_LOG_INFO("%s:short circuit test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:short circuit test fail\n", __func__);
	}

	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN);
	*result = test_result;

	return ret;
}

static int focal_ft3528_short_circuit_test(struct focal_test_params *params,
	struct focal_test_result **result,char test_num)
{
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int adc_data_size = 0;
	int short_data_min = 0;
	int offset = 0;
	int total_num = 0;
	int *adc_data  = NULL;
	int *fm_short_resistance = NULL;
	int code_1 = SHORT_TEST_STAND_ADC;
	int res_stalls = SHORT_TEST_STAND_RES;
	int offset_value[4] = {0};
	u8 stall_value = 1;
	int i = 0;
	int count = 0;
	int temp = 0;
	u8 offset_size = 1;
	u8 offset_res  = OFFSET_RES;
	bool is_weak_short_gnd = false;
	bool is_weak_short_mut = false;

	int min_cc = 0;

	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s:short circuit test start\n", __func__);
	if(!params || !result || !(params->channel_x_num) || !(params->channel_y_num)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	min_cc = params->threshold.short_circuit_min;
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	/* if it is V3 pattern, get Tx/Rx num again */
	ret = mapping_switch(NO_MAPPING, &chl_x, &chl_y);
	if (ret || !chl_x || !chl_y) {
		TS_LOG_ERR("%s:switch mapping status fail, ret=%d\n",
			__func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	if(g_focal_pdata->is_v3_pattern) {
		temp  = chl_x;
		chl_x = chl_y;
		chl_y = temp;
	}
	TS_LOG_INFO("%s:chx:%d,chy:%d\n", __func__, chl_x, chl_y);
	adc_data_size = chl_x + chl_y;
	total_num = adc_data_size;

	result_code[0] = test_num + '0';
	result_code[1] = 'F'; //default result_code is failed
	result_code[2] = '\0';
	ret = focal_alloc_test_container(&test_result, adc_data_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		*result = NULL;
		test_result->result = false;
		goto test_finish;
	}

	test_result->result = true;
	strncpy(test_result->test_name,
		"short_circuit_test", FTS_TEST_NAME_LEN - 1);

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
			__func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	TS_LOG_INFO("%s:focal_enter_factory\n",__func__);

	adc_data = kzalloc((adc_data_size + 1) * sizeof(int), GFP_KERNEL);
	if (NULL == adc_data) {
		TS_LOG_ERR("%s:adc_data buffer malloc fail\n", __func__);
		test_result->result = false;
		goto free_mem;
	}
	memset(adc_data, 0, (adc_data_size + 1));

	fm_short_resistance = kzalloc(total_num * sizeof(int), GFP_KERNEL);
	if (NULL == fm_short_resistance) {
		TS_LOG_ERR("%s:fm_short_resistance buffer malloc fail\n", __func__);
		test_result->result = false;
		goto free_mem;
	}
	memset(fm_short_resistance, 0, total_num);

	ret = focal_read_reg(SHORT_TEST_RES_LEVEL, &stall_value);
	if (ret) {
		TS_LOG_ERR("%s:get value from reg0x5A fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto free_mem;
	}
	ret = focal_write_reg(SHORT_TEST_RES_LEVEL, SET_SHORT_TEST_DELAY);
	if (ret) {
		TS_LOG_ERR("%s:write reg0x5A fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto free_mem;
	}

	ret = weak_short_get_adc_data(offset_size * 2, offset_value, READ_ADC_CMD_OFFSET);
	if (ret) {
		TS_LOG_ERR("%s:failed to get weak short data,ret= %d", __func__, ret);
		test_result->result = false;
		goto free_mem;
	}
	msleep(50);//ic need

	offset = offset_value[0] - SHORT_TEST_STAND_OFFSET;

	/* get short resistance and exceptional channel */
	ret = weak_short_get_adc_data(adc_data_size * 2, adc_data, READ_ADC_CMD_CHL_TO_ALL);
	if (ret) {
		TS_LOG_ERR("%s:failed to get weak short data,ret= %d", __func__, ret);
		test_result->result = false;
		goto free_mem;
	}
	msleep(50);//ic need

	for (i = 0; i < total_num; i++) {
		if (i < chl_y) {
			TS_LOG_INFO("Tx%02d: %4d\n", i + 1, adc_data[i]);
		} else  if (i < total_num ) {
			TS_LOG_INFO("Rx%02d: %4d\n", i - chl_y + 1, adc_data[i]);
		}
		test_result->values[i] = adc_data[i];
	}
	TS_LOG_INFO("\r\n");

	count = 0;
	for ( i = 0; i < total_num; i++) {
		if (code_1 - adc_data[i] <= 0) {
			fm_short_resistance[i] = min_cc;
			continue;
		}
		fm_short_resistance[i] = (adc_data[i] - offset + SHORT_TEST_OFFSET_RES) * res_stalls / (code_1 - adc_data[i]) - offset_res;
		if (fm_short_resistance[i] < 0)
		    fm_short_resistance[i] = abs(fm_short_resistance[i]);
	}

	/* data compare */
	short_data_min = params->threshold.short_circuit_min;
	for (i = 0; i < total_num; i++) {
		if (short_data_min > fm_short_resistance[i]) {
			test_result->result = false;
			tp_cap_test_status = PANEL_REASON;
			strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
			focal_print_test_data("short circuit test fail:",
				i / chl_x,
				i % chl_x,
				0,
				short_data_min,
				fm_short_resistance[i]);
		}
	}

	if(test_result->result == false) {
		test_result->result = true;
		is_weak_short_mut = true;
	}

	if (is_weak_short_mut) {
		/* use the exceptional channel to conduct channel to ground short circuit test. */
		test_result->result = short_test_channel_to_gnd(fm_short_resistance, adc_data, offset, &is_weak_short_gnd, chl_x, chl_y);
		TS_LOG_INFO("channel to gnd is %d", test_result->result);

		/* use the exceptional channel to conduct channel to channel short circuit test. */
		test_result->result &= short_test_channel_to_channel(fm_short_resistance, adc_data, offset, chl_x, chl_y);
		TS_LOG_INFO("channel to channel is %d", test_result->result);
	}

	ret = focal_write_reg(SHORT_TEST_RES_LEVEL, stall_value);

	TS_LOG_INFO("code1:%d, offset:%d\n", code_1, offset);

free_mem:

	if(NULL != adc_data)
		kfree(adc_data);
	if(NULL != fm_short_resistance)
		kfree(fm_short_resistance);


test_finish:
	if(FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		ret = mapping_switch(MAPPING, &chl_x, &chl_y);
		if (ret) {
			TS_LOG_ERR("%s:switch mapping status fail, ret=%d\n",
				__func__, ret);
		}
	}
	if (test_result->result) {
		result_code[1] = 'P'; //test pass
		TS_LOG_INFO("%s:short circuit test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:short circuit test fail\n", __func__);
	}

	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN);
	*result = test_result;

	return ret;
}

static int focal_row_column_delta_test(
	struct focal_test_params *params,
	struct focal_test_result *srcresult, struct focal_test_result **result, char test_num)
{
	int i = 0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int adc_data_size = 0;
	int row_column_delta_max = 0;
	int malloc_test_result = 0;

	TS_LOG_INFO("%s:row column delta test start,%d\n", __func__, test_num);
	if(!params || !result || (params->channel_x_num) <= 0 || (params->channel_y_num) <= 0){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}

	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	adc_data_size = chl_x * chl_y;
	malloc_test_result = adc_data_size*2;
	if (g_focal_dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
		test_num = RAW_DATA_TYPE_TrxDelta;
	}
	result_code[0] = test_num+ '0';
	result_code[1] = RAWDATA_TEST_FAIL_CHAR; //default result_code is failed
	result_code[2] = '\0';

	ret = focal_alloc_test_container(&test_result, malloc_test_result);
	if (ret) {
		TS_LOG_ERR("%s:alloc raw data container fail, ret=%d\n",
			__func__, ret);
		*result = NULL;
		return ret;
	}
	test_result->testresult = RAWDATA_TEST_FAIL_CHAR;
	test_result->typeindex = RAW_DATA_TYPE_TrxDelta;
	test_result->result = true;
	strncpy(test_result->test_name, "Trx_delta", FTS_TEST_NAME_LEN - 1);

	for(i = 0; i < chl_x * chl_y ; i++) {
		if (i%chl_x == 0) {
			test_result->values[i] = 0;
		} else {
			test_result->values[i] = focal_abs(srcresult->values[i]
				-srcresult->values[i - 1]);
		}
	}

	for(i = 0; i < chl_x * chl_y ; i++) {
		if (i < chl_x) {
			test_result->values[i + adc_data_size] = 0;
		} else {
			test_result->values[i + adc_data_size] =
				focal_abs(srcresult->values[i] -
				srcresult->values[i - chl_x]);
		}
	}

	if(ROW_COLUMN_DELTA_TEST_POINT_BY_POINT ==  params->row_column_delta_test_point_by_point) {
		for (i = 0; i < adc_data_size*2; i++) {
			if (test_result->values[i] > params->threshold.row_column_delta_max_array[i]) {
				test_result->result = false;
				tp_cap_test_status = PANEL_REASON;
				strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);

				focal_print_test_data("row column delta test point by point fail",
					i / chl_x,
					i % chl_x,
					params->threshold.row_column_delta_max_array[i],
					0,
					test_result->values[i]);
			}
		}
	} else {
		row_column_delta_max = params->threshold.row_column_delta_max;
		/* compare row_column_delta with shreshold */
		for (i = 0; i < adc_data_size*2; i++) {
			if (test_result->values[i] > row_column_delta_max) {
				test_result->result = false;
				tp_cap_test_status = PANEL_REASON;
				strncpy(test_result->tptestfailedreason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
				focal_print_test_data("raw data test fail",
					i / chl_x,
					i % chl_x,
					row_column_delta_max,
					0,
					test_result->values[i]);
			}
		}
	}
test_finish:
	if (test_result->result) {
		result_code[1] = RAWDATA_TEST_PASS_CHAR; //test pass
		test_result->testresult = RAWDATA_TEST_PASS_CHAR;
		TS_LOG_INFO("%s:row_column_delta_test test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:row_column_delta_test test fail\n", __func__);
	}
	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN - 1);
	*result = test_result;
	return ret;
}

static int focal_adc_scan(void)
{
	int i = 0;
	int ret = 0;
	int scan_query_times = 50;
	u8 test_finish = 0x00;
	u8 reg_val = 0;

	/* start adc sample */
	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type) {
		ret = focal_write_reg(REG_5X46_ADC_SCAN, 1);
	}
	else{
		ret = focal_write_reg(REG_ADC_SCAN, 1);
	}
	if (ret) {
		TS_LOG_ERR("%s:adc scan fail, ret=%d\n", __func__, ret);
		return ret;
	}

	if(FOCAL_FT8719 == g_focal_dev_data->ic_type){
		test_finish = REG_8719_LCD_NOISE_READY;
		msleep(1000);
	} else {
		msleep(50);
	}

	for (i = 0; i < scan_query_times; i++) {

		if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type) {
			ret = focal_read_reg(REG_5X46_ADC_SCAN_STATUS, &reg_val);
		}
		else{
			ret = focal_read_reg(REG_ADC_SCAN_STATUS, &reg_val);
		}
		if (!ret) {
			if (reg_val == test_finish) {
				TS_LOG_INFO("%s:adc scan success[%x]\n", __func__, reg_val);
				return 0;
			} else {
				TS_LOG_INFO("%s:adc scan status:0x%02X\n",
					__func__, reg_val);
				msleep(50);
			}
		} else {
			return ret;
		}
	}

	TS_LOG_ERR("%s:adc scan timeout\n", __func__);
	return -ETIMEDOUT;
}

static int focal_get_adc_data(int *data, size_t size,unsigned int chl_x,unsigned int chl_y)
{
	unsigned int i = 0;
	int ret = 0;
	int pkg_size = 0;
	int pkg_count = 0;
	int readed_count = 0;
	int adc_data_size = 0;
	int  temp_data= 0;
	u8 cmd[2] = {0};
	u8 *adc_data = NULL;
	unsigned int CurTx = 0;
	unsigned int CurRx = 0;
	unsigned int Count = 0;
	if(!data||(0 == size) ||(0 == chl_x) ||(0 == chl_y)){
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	adc_data_size = size * 2;
	pkg_count = adc_data_size / BYTES_PER_TIME;
	if (adc_data_size % BYTES_PER_TIME != 0)
		pkg_count += 1;

	adc_data = kzalloc(adc_data_size, GFP_KERNEL);
	if (!adc_data) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		return -ENOMEM;
	}

	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type) {
		ret = focal_start_scan();
		if (ret) {
			TS_LOG_ERR("%s: rawdata test scan fail\n", __func__);
		}
		ret =  focal_adc_scan();
		if (ret) {
		TS_LOG_ERR("%s:adc scan fail, ret=%d\n", __func__, ret);
		goto free_mem;
		}
		msleep(300);
		cmd[0] = REG_5X46_ADC_DATA_ADDR;
	}
	else{
		ret =  focal_adc_scan();
		if (ret) {
			TS_LOG_ERR("%s:adc scan fail, ret=%d\n", __func__, ret);
			goto free_mem;
		}
		cmd[0] = REG_ADC_DATA_ADDR;
	}

	for (i = 0; i < pkg_count; i++) {

		/* if the last package is not full, set package size */
		if ((i + 1 == pkg_count)
			&& (adc_data_size % BYTES_PER_TIME != 0)) {

			pkg_size = adc_data_size % BYTES_PER_TIME;
		} else {
			pkg_size = BYTES_PER_TIME;
		}

		if (TS_BUS_I2C == g_focal_dev_data->ts_platform_data->bops->btype) {
			/* the first package should send a command and read */
			if (i == 0) {
				ret = focal_read(cmd, 1, adc_data, pkg_size);
			} else {
				ret = focal_read_default(adc_data + readed_count,
					pkg_size);
			}
		} else {
			ret = focal_read(cmd, 1, adc_data+ readed_count, pkg_size);
		}

		if (ret) {
			TS_LOG_ERR("%s:read adc data fail, ret=%d\n",
				__func__, ret);
			goto free_mem;
		}

		readed_count += pkg_size;
	}

	TS_LOG_INFO("%s:readed cout=%d, size=%lu\n", __func__,
		readed_count, size);

	if (FOCAL_FT5X46 == g_focal_dev_data->ic_type || FOCAL_FT5422U == g_focal_dev_data->ic_type) {
		for (i = 0; i < size; i++){
			Count = i * 2;
			temp_data =( adc_data[Count] << 8);
			Count ++;
			temp_data |=  adc_data[Count];
			CurTx = i / chl_y;
			CurRx = i % chl_y;
			Count = CurTx*chl_y + CurRx;
			data[Count] = temp_data;
			TS_LOG_DEBUG("%s:CurTx= %d , Rx =%d ,i =%d \n",__func__, CurTx,CurRx,i);
		}
	}
	else{
		for (i = 0; i < size; i++){
			Count = i * 2;
			temp_data =( adc_data[Count] << 8);
			Count ++;
			temp_data |= adc_data[Count];
			CurTx = i % chl_y;
			CurRx = i /chl_y;
			Count = CurTx*chl_x + CurRx;
			data[Count] = temp_data;
			TS_LOG_DEBUG("%s:CurTx= %d , Rx =%d ,i =%d \n",__func__, CurTx,CurRx,i);
		}
	}

	TS_LOG_INFO("%s:copy data to buff finished\n", __func__);

	ret = 0;

free_mem:
	kfree(adc_data);
	adc_data = NULL;

	return ret;
}

static int focal_get_statistics_data(
	int *data, size_t data_size, char *result, size_t res_size)
{
	int avg = 0;
	int min = 0;
	int max = 0;

	if (!data) {
		TS_LOG_ERR("%s:data is null\n", __func__);
		return -ENODATA;
	}

	if (!result) {
		TS_LOG_ERR("%s:result is null\n", __func__);
		return -ENODATA;
	}

	if (data_size <= 0 || res_size <= 0) {
		TS_LOG_ERR("%s:%s, data_size=%ld, res_size=%ld\n", __func__,
			"input parameter is illega",
			data_size, res_size);
		return -EINVAL;
	}

	avg = focal_get_int_average(data, data_size);
	min = focal_get_int_min(data, data_size);
	max = focal_get_int_max(data, data_size);

	memset(result, 0, res_size);
	return snprintf(result, res_size, "[%4d,%4d,%4d]", avg, max, min);
}

static void focal_put_device_info(struct ts_rawdata_info *info)
{
	struct focal_platform_data *focal_pdata = NULL;

	focal_pdata = focal_get_platform_data();

	/* put ic data */
	focal_strncat(info->result, "-ft", TS_RAWDATA_RESULT_MAX);
	if(FOCAL_FT8006U == g_focal_dev_data->ic_type) {
		/*the chip id "FT8006" will make the test fail because of the "F",so we change it to "ft8006"*/
		focal_strncatint(info->result, focal_pdata->chip_id, "%x",
			TS_RAWDATA_RESULT_MAX);
	} else {
		focal_strncatint(info->result, focal_pdata->chip_id, "%X",
                        TS_RAWDATA_RESULT_MAX);
	}

	focal_strncat(info->result, "-", TS_RAWDATA_RESULT_MAX);
	focal_strncat(info->result, focal_pdata->vendor_name,
		TS_RAWDATA_RESULT_MAX);

	focal_strncat(info->result, "-", TS_RAWDATA_RESULT_MAX);
	focal_strncatint(info->result, focal_pdata->fw_ver, "%d;",
		TS_RAWDATA_RESULT_MAX);
}

static void focal_put_device_info_fornewformat(struct ts_rawdata_info_new *info)
{
	struct focal_platform_data *focal_pdata = NULL;

	focal_pdata = focal_get_platform_data();

	/* put ic data */
	focal_strncat(info->deviceinfo, "-ft", sizeof(info->deviceinfo));
	focal_strncatint(info->deviceinfo, focal_pdata->chip_id, "%X",
		sizeof(info->deviceinfo));

	focal_strncat(info->deviceinfo, "-", sizeof(info->deviceinfo));
	focal_strncat(info->deviceinfo, focal_pdata->vendor_name,
		sizeof(info->deviceinfo));

	focal_strncat(info->deviceinfo, "-", sizeof(info->deviceinfo));
	focal_strncatint(info->deviceinfo, focal_pdata->fw_ver, "%d;",
		sizeof(info->deviceinfo));
}

static void focal_put_test_result_fornewformat(
	struct focal_test_params *params,
	struct ts_rawdata_info_new *info,
	struct focal_test_result *test_results[],
	int size)
{
	int i = 0;
	int j = 0;
	int buff_index = 0;
	char result[TS_RAWDATA_RESULT_MAX]={0};
	char statistics_data[FTS_STATISTICS_DATA_LEN] = {0};
	char resulttemp[TS_RAWDATA_RESULT_CODE_LEN]={0};
	struct ts_rawdata_newnodeinfo * pts_node = NULL;	

	TS_LOG_DEBUG("%s: begin\n", __func__);	

	focal_strncat (result, info->i2cinfo, TS_RAWDATA_RESULT_MAX);
	focal_strncat (result, "-", TS_RAWDATA_RESULT_MAX);
	for(i = 0; i < size; i++) {
		if (!test_results[i]) {
			TS_LOG_INFO("%s:test result is null, index=%d\n", __func__, i);
			continue;
		}
		resulttemp[0] = test_results[i]->typeindex + '0';
		resulttemp[1] = test_results[i]->testresult;
		resulttemp[2] = '\0';
		focal_strncat (result, resulttemp, TS_RAWDATA_RESULT_MAX);
		focal_strncat (result, "-", TS_RAWDATA_RESULT_MAX);
	}
	/* get statistics data */
	for (i = 0; i < size; i++) {
		if (!test_results[i]) {
			TS_LOG_INFO("%s:test result is null, index=%d\n",
				__func__, i);
			continue;
		}

		focal_get_statistics_data(test_results[i]->values,
			test_results[i]->size,
			statistics_data,
			FTS_STATISTICS_DATA_LEN);

		strncpy(test_results[i]->statistics_data, statistics_data,
			FTS_STATISTICS_DATA_LEN-1);

		focal_strncat (result, statistics_data, TS_RAWDATA_RESULT_MAX);
	}

	/* get device info */
	focal_put_device_info_fornewformat(info);

	focal_strncat (result, info->deviceinfo, TS_RAWDATA_RESULT_MAX);

	TS_LOG_INFO("%s:%s\n", __func__, result);
	/* put test data */
	info->tx = params->channel_x_num;
	info->rx = params->channel_y_num;
	info->listnodenum = size;

	for (i = 0; i < size; i++) {

		if (!test_results[i]) {
			TS_LOG_INFO("%s:test result is null, index=%d\n",
				__func__, i);
			continue;
		}
	
		pts_node = (struct ts_rawdata_newnodeinfo *)kzalloc(sizeof(struct ts_rawdata_newnodeinfo), GFP_KERNEL);
		if (!pts_node) {
			TS_LOG_ERR("malloc failed\n");
			goto out;
		}		
		pts_node->values = kzalloc(test_results[i]->size*sizeof(int), GFP_KERNEL);
		if (!pts_node->values) {
			TS_LOG_ERR("malloc failed  for values\n");
			kfree(pts_node);
			pts_node = NULL;
			goto out;
		}
		memcpy(pts_node->values,test_results[i]->values,test_results[i]->size*sizeof(int));
		TS_LOG_DEBUG("%s: put data test_name:%s,size :%d\n", __func__,test_results[i]->test_name,test_results[i]->size);		
		pts_node->size = test_results[i]->size;
		pts_node->testresult = test_results[i]->testresult;
		pts_node->typeindex = test_results[i]->typeindex;		
		strncpy(pts_node->test_name,test_results[i]->test_name,FTS_TEST_NAME_LEN-1);
		strncpy(pts_node->statistics_data,test_results[i]->statistics_data,FTS_STATISTICS_DATA_LEN-1);
		strncpy(pts_node->tptestfailedreason,test_results[i]->tptestfailedreason,TP_TEST_FAILED_REASON_LEN-1);		
		list_add_tail(&pts_node->node, &info->rawdata_head);	
	}	
out:
	return;
}

static void focal_put_test_result(
	struct focal_test_params *params,
	struct ts_rawdata_info *info,
	struct focal_test_result *test_results[],
	int size)
{
	int i = 0;
	int j = 0;
	int buff_index = 0;
	char statistics_data[FTS_STATISTICS_DATA_LEN] = {0};

	if(FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		if(size > FTS_MAX_CAP_TEST_NUM) {
			TS_LOG_ERR("%s. size larger than define,size:%d, define %d\n", __func__, size, FTS_MAX_CAP_TEST_NUM);
			return;
		}
	}
	/* put test result */
	for (i = 0; i < size; i++) {

		if (!test_results[i]) {
			TS_LOG_INFO("%s:test result is null, index=%d\n",
				__func__, i);
			focal_strncat(info->result, "FF", TS_RAWDATA_RESULT_MAX);

			continue;
		}

		focal_strncat(info->result, test_results[i]->result_code,
			TS_RAWDATA_RESULT_MAX);
		if (i != size - 1)
			focal_strncat(info->result, "-", TS_RAWDATA_RESULT_MAX);
	}

	/* put statistics data */
	for (i = 0; i < size; i++) {
		if (!test_results[i]) {
			TS_LOG_INFO("%s:test result is null, index=%d\n",
				__func__, i);
			continue;
		}

		focal_get_statistics_data(test_results[i]->values,
			test_results[i]->size,
			statistics_data,
			FTS_STATISTICS_DATA_LEN);

		focal_strncat(info->result, statistics_data,
			TS_RAWDATA_RESULT_MAX);
	}

	/* put test failed reason */
	if(tp_cap_test_status)//TEST_SUCCESS = 0
	{
		focal_strncat(info->result, tp_test_failed_reason,TP_TEST_FAILED_REASON_LEN);
	}
	focal_put_device_info(info);
	/* put test data */
	memset(info->buff, 0, TS_RAWDATA_BUFF_MAX);
	info->buff[buff_index++] = params->channel_x_num;
	info->buff[buff_index++] = params->channel_y_num;
	for (i = 0; i < size; i++) {

		if (!test_results[i]) {
			TS_LOG_INFO("%s:test result is null, index=%d\n",
				__func__, i);
			continue;
		}

		for (j = 0; j < test_results[i]->size; j++) {

			if (buff_index >= TS_RAWDATA_BUFF_MAX) {
				TS_LOG_INFO("%s:buff is full, len=%d\n",
					__func__, buff_index);
				break;
			}

			info->buff[buff_index++] = test_results[i]->values[j];
		}
	}

	info->used_size = buff_index;

}

/*
 *
 * param - data_type : 0:raw data, 1:diff data
 *
 */
 int focal_get_debug_data_(int data_type, int *data, size_t size,int chl_x,int chl_y)
{
	int rc = 0;
	int ret = 0;

	ret = focal_write_reg(REG_DATA_TYPE, data_type);
	if (ret) {
		TS_LOG_ERR("%s:write data type fail, ret=%lu\n", __func__, ret);
		goto exit;
	}

	ret = focal_start_scan();
	if (ret) {
		TS_LOG_ERR("%s:scan fail, ret=%d\n", __func__, ret);
		goto reset_data_type;
	}

	TS_LOG_ERR("%s:data size=%lu\n", __func__, size);

	ret = focal_get_raw_data_format(chl_x,chl_y,data, size);
	if (ret) {
		TS_LOG_ERR("%s:get debug data fail, ret=%d\n", __func__, ret);
		goto reset_data_type;
	}

reset_data_type:

	rc = focal_write_reg(REG_DATA_TYPE, 0);
	if (rc) {
		TS_LOG_ERR("%s: write reg fail\n", __func__);
	}
exit:

	return ret;
}


 int focal_get_cb_debug_data_( int *data, size_t size,int chl_x,int chl_y)
{
	int ret = 0;
	TS_LOG_INFO("%s: cb debug  test start\n", __func__);
	if(!data || (0 == chl_x) || ( 0 == chl_y)){
		TS_LOG_INFO("%s: parameters is invalid .\n", __func__);
		return -EINVAL;
	}
	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",__func__, ret);
		goto test_finish;
	}

	ret = focal_get_cb_data_format(data, size,chl_x,chl_y);
	if (ret) {
		TS_LOG_INFO("%s:get cb data fail, ret=%d\n", __func__, ret);
		goto test_finish;
	}
test_finish:
	return ret;
}

void focal_prase_threshold_for_csv(const char *project_id,
	struct focal_test_threshold *threshold,
	struct focal_test_params *params)
{
	int ret;
	char file_path[CAP_TEST_BUF_SIZE] = {0};
	u32 data_buf[RAW_DATA_BUF_SIZE] = {0};
	char file_name[FTS_THRESHOLD_NAME_LEN] = {0};

	struct focal_platform_data *fts_pdata = focal_get_platform_data();

	TS_LOG_INFO("%s: TP test preparation\n", __func__);

	snprintf(file_name, FTS_THRESHOLD_NAME_LEN, "%s_%s_%s_%s_limits.csv",
		g_focal_dev_data->ts_platform_data->product_name,
		FTS_CHIP_NAME, fts_pdata->project_id,
		fts_pdata->vendor_name);

	snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s", file_name);
	TS_LOG_INFO("%s: csv_file_path =%s\n", __func__, file_path);

	/*because cb data only have max/min data, so columns is 2/rows is 1*/
	if(CB_TEST_POINT_BY_POINT ==  params->cb_test_point_by_point) {
		ret = ts_kit_parse_csvfile(file_path, DTS_CB_TEST_MIN_ARRAY, threshold->cb_test_min_array,
			params->channel_y_num, params->channel_x_num);
		if (ret) {
			TS_LOG_INFO("%s: Failed get %s \n", __func__, DTS_CB_TEST_MIN_ARRAY);
			ret = 0;
		}


		ret = ts_kit_parse_csvfile(file_path, DTS_CB_TEST_MAX_ARRAY, threshold->cb_test_max_array,
			params->channel_y_num, params->channel_x_num);
		if (ret) {
			TS_LOG_INFO("%s: Failed get %s \n", __func__, DTS_CB_TEST_MAX_ARRAY);
			ret = 0;
		}

	} else {
		ret = ts_kit_parse_csvfile(file_path, FTS_CB_TEST_CSV, data_buf, 1, 2);
		if (ret) {
			TS_LOG_INFO("%s: Failed get %s \n", __func__, FTS_CB_TEST_CSV);
			return ret;
		}
		threshold->cb_test_min = data_buf[0];
		threshold->cb_test_max = data_buf[1];
	}
	if(FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		ret = ts_kit_parse_csvfile(file_path, DTS_PANEL_DIFFER_MIN_ARRAY, threshold->panel_differ_min_array,
			params->channel_y_num, params->channel_x_num);
		if (ret) {
			TS_LOG_INFO("%s: Failed get %s \n", __func__, DTS_PANEL_DIFFER_MIN_ARRAY);
			ret = 0;
		}


		ret = ts_kit_parse_csvfile(file_path, DTS_PANEL_DIFFER_MAX_ARRAY, threshold->panel_differ_max_array,
			params->channel_y_num, params->channel_x_num);
		if (ret) {
			TS_LOG_INFO("%s: Failed get %s \n", __func__, DTS_PANEL_DIFFER_MAX_ARRAY);
			ret = 0;
		}
	}

	/*because scap raw data only have max/min data, so columns is 2/rows is 1*/
	ret = ts_kit_parse_csvfile(file_path, FTS_SCAP_RAW_FATA_CSV, data_buf, 1, 2);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, FTS_SCAP_RAW_FATA_CSV);
		return ret;
	}
	threshold->scap_raw_data_min = data_buf[0];
	threshold->scap_raw_data_max = data_buf[1];

	/*because short data only have min data, so columns is 1/rows is 1*/
	if(SHORT_TEST_POINT_BY_POINT == params->short_test_point_by_point) {
		ret = ts_kit_parse_csvfile(file_path, DTS_SHORT_CIRCUIT_RES_MIN_ARRAY,
			threshold->short_test_min_array,
			params->channel_y_num, params->channel_x_num);
		if (ret) {
			TS_LOG_INFO("%s: Failed get %s \n", __func__, DTS_CB_TEST_MIN_ARRAY);
			ret = 0;
		}
	} else {
		ret = ts_kit_parse_csvfile(file_path, DTS_SHORT_CIRCUIT_RES_MIN, data_buf, 1, 1);
		if (ret) {
			TS_LOG_INFO("%s: Failed get %s \n", __func__, DTS_SHORT_CIRCUIT_RES_MIN);
			return ret;
		}
		threshold->short_circuit_min = data_buf[0];

	}
	/* get open_test_cb_point_by_point data from csv  */
	if(OPEN_TEST_POINT_BY_POINT ==  params->open_test_cb_point_by_point) {
		ret = ts_kit_parse_csvfile(file_path, DTS_OPEN_TEST_CB_MIN_ARRAY, threshold->open_test_cb_min_array,
			params->channel_y_num, params->channel_x_num);
		if (ret) {
			TS_LOG_INFO("%s: Failed get %s \n", __func__, DTS_OPEN_TEST_CB_MIN_ARRAY);
			ret = 0;
		}


		ret = ts_kit_parse_csvfile(file_path, DTS_OPEN_TEST_CB_MAX_ARRAY, threshold->open_test_cb_max_array,
			params->channel_y_num, params->channel_x_num);
		if (ret) {
			TS_LOG_INFO("%s: Failed get %s \n", __func__, DTS_OPEN_TEST_CB_MAX_ARRAY);
			ret = 0;
		}

	}

	/*because row colums delta data only have max data, so columns is 1/rows is 1*/
	if(ROW_COLUMN_DELTA_TEST_POINT_BY_POINT ==  params->row_column_delta_test_point_by_point) {
		memset(threshold->row_column_delta_max_array, 0, sizeof(threshold->row_column_delta_max_array));
		ret = ts_kit_parse_csvfile(file_path, DTS_ROW_COLUMN_DELTA_MAX_ARRAY,
					threshold->row_column_delta_max_array,
					params->channel_y_num*2 , params->channel_x_num);
		if (ret) {
			TS_LOG_INFO("%s: Failed get %s \n", __func__, DTS_ROW_COLUMN_DELTA_MAX_ARRAY);
			return ret;
		}
	} else {
		ret = ts_kit_parse_csvfile(file_path, DTS_ROW_COLUMN_DELTA_TEST, data_buf, 1, 1);
		if (ret) {
			TS_LOG_INFO("%s: Failed get %s \n", __func__, DTS_ROW_COLUMN_DELTA_TEST);
			return ret;
		}
		threshold->row_column_delta_max = data_buf[0];
	}

	/*because lcd noise data only have max data, so columns is 1/rows is 1*/
	ret = ts_kit_parse_csvfile(file_path, DTS_LCD_NOISE_MAX, data_buf, 1, 1);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, DTS_LCD_NOISE_MAX);
		return ret;
	}
	threshold->lcd_noise_max = data_buf[0];

	ret = ts_kit_parse_csvfile(file_path, DTS_RAW_DATA_MIN_ARRAY, threshold->raw_data_min_array,
			params->channel_y_num, params->channel_x_num);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, DTS_RAW_DATA_MIN_ARRAY);
		ret = 0;	
	}
	
	ret = ts_kit_parse_csvfile(file_path, DTS_RAW_DATA_MAX_ARRAY, threshold->raw_data_max_array,
			params->channel_y_num, params->channel_x_num);
	if (ret) {
		TS_LOG_INFO("%s: Failed get %s \n", __func__, DTS_RAW_DATA_MAX_ARRAY);
		ret = 0;
	}

	TS_LOG_INFO("%s:%s:%s=%d, %s=%d, %s=%d, %s=%d, %s=%d, %s=%d, %s=%d, %s=%d, %s=%d\n",
		__func__, "cb test thresholds",
		"raw_data_min", threshold->raw_data_min,
		"raw_data_max", threshold->raw_data_max,
		"cb_test_min",  threshold->cb_test_min,
		"cb_test_max",  threshold->cb_test_max,
		"lcd_noise_max", threshold->lcd_noise_max,
		"short_circuit_min", threshold->short_circuit_min,
		"row_column_delta_max", threshold->row_column_delta_max,
		"scap_raw_data_min", threshold->scap_raw_data_min,
		"scap_raw_data_max", threshold->scap_raw_data_max);
}

 static int focal_init_test_prm(
	struct focal_platform_data *pdata,
	struct focal_test_params *params,
	struct ts_rawdata_info *info)
{
	int ret = 0;

	TS_LOG_INFO("%s: set param data called\n", __func__);
	if((!pdata ) || !(params) || !(info)){
		TS_LOG_INFO("%s: parameters invalid!\n", __func__);
		return -EINVAL;
	}
	ret = focal_get_channel_form_ic(params);
	if (ret) {
		TS_LOG_ERR("%s:get channel num fail,I2C communication error! ret=%d\n", __func__, ret);
		focal_strncat(info->result, "0F-", TS_RAWDATA_RESULT_MAX);
		focal_strncat(info->result, "software reason ", TS_RAWDATA_RESULT_MAX);
		return ret;
	}
	else{
		TS_LOG_INFO("%s: i2c test pass.ret =%d\n", __func__,ret);
		focal_strncat(info->result, "0P-", TS_RAWDATA_RESULT_MAX);
	}
	ret = focal_parse_cap_test_config(pdata, params);
	if (ret < 0) {
		TS_LOG_ERR("%s: analysis tp test data failed\n", __func__);
		return ret;
	}

	focal_set_max_channel_num(params);

	TS_LOG_INFO("%s: set param data success\n", __func__);

	return 0;
}

static int focal_init_test_prm_fornewformat(
	struct focal_platform_data *pdata,
	struct focal_test_params *params,
	struct ts_rawdata_info_new *info)
{
	int ret = 0;
	ret = focal_get_channel_form_ic(params);
	if (ret) {
		TS_LOG_ERR("%s:get channel num fail,I2C communication error! ret=%d\n", __func__, ret);
		focal_strncat(info->i2cinfo, "0F", sizeof(info->i2cinfo));
		focal_strncat(info->i2cerrinfo, "software reason ", sizeof(info->i2cerrinfo));
		return ret;
	}
	else{
		TS_LOG_INFO("%s: i2c test pass.ret =%d\n", __func__,ret);
		focal_strncat(info->i2cinfo, "0P", sizeof(info->i2cinfo));
	}
	ret = focal_parse_cap_test_config(pdata, params);
	if (ret < 0) {
		TS_LOG_ERR("%s: analysis tp test data failed\n", __func__);
		return ret;
	}
	TS_LOG_INFO("%s: set param data success\n", __func__);

	return 0;
}
int focal_get_raw_dataNewformat(
	struct ts_rawdata_info *info,
	struct ts_cmd_node *out_cmd)
{
	int ret = 0;
	int i = 0;
	struct focal_platform_data *pdata = NULL;
	struct ts_rawdata_info_new* infonew = info;

	TS_LOG_INFO("%s:focal get rawdata called for new format\n", __func__);

	pdata = focal_get_platform_data();

	memset(tp_test_failed_reason, 0, TP_TEST_FAILED_REASON_LEN);
	if (!params) {
		params = kzalloc(sizeof(struct focal_test_params), GFP_KERNEL);
		if (!params) {
			TS_LOG_ERR("%s:alloc mem for params fail\n", __func__);
			ret = -ENOMEM;
			goto err_alloc_mem;
		}
	}


	if (FOCAL_FT8719 == g_focal_dev_data->ic_type) {
		for (i = 0; i < MAX_RETRY_TIMES; i++) {
			ret = focal_read_reg(FTS_REG_VALUE_0XA0, &judge_ic_type);
			if (ERROR_CODE_OK != ret) {
				TS_LOG_ERR("read Reg 0xa0 fail\n");
				goto free_mem;
			}
			TS_LOG_INFO("judge_ic_type is %d\n",judge_ic_type);

			if (0x01 != judge_ic_type && 0xFF != judge_ic_type) {
				break;
			}
		}

		if (i >= MAX_RETRY_TIMES) {
			TS_LOG_ERR("%s get wrong judge_ic_type = %d\n", __func__, judge_ic_type);
		}
	} else {
		ret = focal_read_reg(FTS_REG_VALUE_0XA0, &judge_ic_type);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("read Reg 0xa0 fail\n");
			goto free_mem;
		}
		TS_LOG_INFO("judge_ic_type is %d\n",judge_ic_type);
	}


	if (FOCAL_FT8719 == g_focal_dev_data->ic_type) {
		for (i = 0; i < MAX_RETRY_TIMES; i++) {
			ret = focal_read_reg(REG_8716_LCD_NOISE_VALUE, &g_focal_pdata->lcd_noise_threshold);
			if (ERROR_CODE_OK != ret) {
				TS_LOG_ERR("read Reg REG_8716_LCD_NOISE_VALUE fail\n");
				goto free_mem;
			}
			TS_LOG_INFO("touch thr is %d\n",g_focal_pdata->lcd_noise_threshold);
			if ((0x01 != g_focal_pdata->lcd_noise_threshold && 0xFF != g_focal_pdata->lcd_noise_threshold)) {
				break;
			}

			if (i >= MAX_RETRY_TIMES) {
				TS_LOG_ERR("%s get wrong lcd_noise_threshold = %d\n", __func__, g_focal_pdata->lcd_noise_threshold);
			}
		}
	}

	if (true == pdata->open_threshold_status) {
		ret = focal_init_test_prm_fornewformat(pdata, params,infonew);
		if (ret < 0) {
			TS_LOG_ERR("%s:get param from dts fail, ret=%d", __func__, ret);
			ret  = 0;//use for printf I2c error information
			goto free_mem;
		}
		if (pdata->only_open_once_captest_threshold) {
			pdata->open_threshold_status = false;
		}
	}

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
			__func__, ret);
		goto free_mem;
	}
	ret =  focal_start_test_tp(params, infonew);
	if (!ret)
		TS_LOG_INFO("%s:tp test pass\n", __func__);
	else
		TS_LOG_ERR("%s:tp test fail, ret=%d\n", __func__, ret);
	ret = focal_enter_work();
	if (ret < 0)
		TS_LOG_ERR("%s:enter work model fail, ret=%d\n", __func__, ret);

	return ret;
free_mem:

	kfree(params);
	params = NULL;
	pdata->open_threshold_status = true;
	ret = focal_enter_work();
	if (ret < 0)
		TS_LOG_ERR("%s:enter work model fail, ret=%d\n", __func__, ret);
err_alloc_mem:
	return ret;
}

static int focal_get_pattern_flag(struct focal_platform_data* pdata)
{
	int ret = NO_ERR;
	u8 ch_pattern = 0;
	if(!pdata) {
		TS_LOG_ERR("%s, pdata is NULL\n", __func__);
		return -EINVAL;
	}
	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
			__func__, ret);
		return ret;
	}
	ret = focal_read_reg(REG_PATTERN_5452, &ch_pattern);
	if (ret) {
		TS_LOG_ERR("%s: switch to no_mapping failed!", __func__);
		return ret;
	} else {
		if(ch_pattern == 1) {/*1: V3 pattren*/
			pdata->is_v3_pattern = true;
			TS_LOG_INFO("this ic is v3 pattern");
		}
	}
	ret = focal_enter_work();
	if (ret < 0)
		TS_LOG_ERR("%s:enter work model fail, ret=%d\n", __func__, ret);

	return ret;
}

int focal_get_raw_data(
	struct ts_rawdata_info *info,
	struct ts_cmd_node *out_cmd)
{
	int ret = 0;
	struct focal_platform_data *pdata = NULL;
	if(!g_focal_dev_data || !g_focal_pdata || !(g_focal_dev_data->ts_platform_data) || !(g_focal_dev_data->ts_platform_data->chip_data)) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	if (g_focal_dev_data->ts_platform_data->chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
		return focal_get_raw_dataNewformat(info,out_cmd);		
	}

	TS_LOG_INFO("%s:focal get rawdata called\n", __func__);

	pdata = focal_get_platform_data();

	 memset(tp_test_failed_reason, 0, TP_TEST_FAILED_REASON_LEN);
	 if (!params) {
		params = kzalloc(sizeof(struct focal_test_params), GFP_KERNEL);
		if (!params) {
			TS_LOG_ERR("%s:alloc mem for params fail\n", __func__);
			ret = -ENOMEM;
			goto err_alloc_mem;
		}
	}
	ret = focal_read_reg(FTS_REG_VALUE_0XA0, &judge_ic_type);
	if (ERROR_CODE_OK != ret) {
		TS_LOG_ERR("read Reg 0xa0 fail\n");
		goto free_mem;
	}
	TS_LOG_INFO("judge_ic_type is %d\n",judge_ic_type);

	if (FOCAL_FT8719 == g_focal_dev_data->ic_type) {
		ret = focal_read_reg(REG_8716_LCD_NOISE_VALUE, &g_focal_pdata->lcd_noise_threshold);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("read Reg REG_8716_LCD_NOISE_VALUE fail\n");
			goto free_mem;
		}
		TS_LOG_INFO("touch thr is %d\n",g_focal_pdata->lcd_noise_threshold);
	}
	if (FOCAL_FT3528 == g_focal_dev_data->ic_type) {
		ret = focal_get_pattern_flag(g_focal_pdata);
		if (ERROR_CODE_OK != ret) {
			TS_LOG_ERR("read Reg focal_get_pattern_flag fail\n");
			goto free_mem;
		}
	}

	if (true == pdata->open_threshold_status) {
			ret = focal_init_test_prm(pdata, params,info);
		if (ret < 0) {
			TS_LOG_ERR("%s:get param from dts fail, ret=%d", __func__, ret);
			ret  = 0;//use for printf I2c error information
			goto free_mem;
		}
		if (pdata->only_open_once_captest_threshold) {
			pdata->open_threshold_status = false;
		}
	}

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
			__func__, ret);
		goto free_mem;
	}
	ret =  focal_start_test_tp(params, info);
	if (!ret)
		TS_LOG_INFO("%s:tp test pass\n", __func__);
	else
		TS_LOG_ERR("%s:tp test fail, ret=%d\n", __func__, ret);

	ret = focal_enter_work();
	if (ret < 0)
		TS_LOG_ERR("%s:enter work model fail, ret=%d\n", __func__, ret);

	return ret;

free_mem:

	kfree(params);
	params = NULL;
	pdata->open_threshold_status = true;
    ret = focal_enter_work();
	if (ret < 0)
		TS_LOG_ERR("%s:enter work model fail, ret=%d\n", __func__, ret);
err_alloc_mem:

	return ret;
}

int focal_get_debug_data(
	struct ts_diff_data_info *info,
	struct ts_cmd_node *out_cmd)
{
	int i = 0;
	int ret = 0;
	u8 chl_x = 0;
	u8 chl_y = 0;
	int used_size = 0;

	int *debug_data = NULL;
	size_t data_size = 0;

    /* first ,should disable calibration before enter factory mode ,and FW will stop to refresh baseline*/
	ret = focal_write_reg(FTS_CALIBRATION_DISABLE_REG, 1);
	if (ret) {
		TS_LOG_ERR("%s:disable calibration funtion  failed, ret=%lu\n", __func__, ret);
		 return ret;
	}

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory model fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	ret = focal_get_channel_num(&chl_x, &chl_y);
	if (ret) {
		TS_LOG_ERR("%s:get channel number fail, ret=%d\n",
			__func__, ret);
		goto exit;
	}

	data_size = chl_x * chl_y;
	debug_data = kzalloc(data_size * sizeof(int), GFP_KERNEL);
	if (!debug_data) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		goto exit;
	}

	switch (info->debug_type) {
	case READ_DIFF_DATA:
		TS_LOG_INFO("%s:read diff data\n", __func__);
		ret = focal_get_debug_data_(1, debug_data, data_size,chl_x, chl_y);
		break;
	case READ_RAW_DATA:
		TS_LOG_INFO("%s:read raw data\n", __func__);
		ret = focal_get_debug_data_(0, debug_data, data_size,chl_x, chl_y);
		break;
	case READ_CB_DATA:
		TS_LOG_INFO("%s:read CB data\n", __func__);
		ret = focal_get_cb_debug_data_( debug_data, data_size,chl_x, chl_y);
		break;
	default:
		ret = -EINVAL;
		TS_LOG_ERR("%s:debug_type mis match\n", __func__);
		break;
	}

	if (ret)
		goto free_debug_data;

	info->buff[used_size++] = chl_x;
	info->buff[used_size++] = chl_y;
	for (i = 0; i < data_size && used_size < TS_RAWDATA_BUFF_MAX; i++)
		info->buff[used_size++] = debug_data[i];

	info->used_size = used_size;

free_debug_data:
	kfree(debug_data);
	debug_data = NULL;

exit:
	focal_enter_work();
	msleep(DELAY_TIME_OF_CALIBRATION);
    /* finally ,should enable calibration before enter factory mode  */
	ret = focal_write_reg(FTS_CALIBRATION_DISABLE_REG, 0);
	if (ret) {
		TS_LOG_ERR("%s:disable calibration funtion  failed, ret=%lu\n", __func__, ret);
		 return ret;
	}

	return ret;
}

int focal_chip_get_capacitance_test_type(struct ts_test_type_info *info)
{
	int ret = 0;
	struct ts_kit_device_data *dev_data = NULL;

	dev_data = focal_get_device_data();

	if (!info) {
		TS_LOG_ERR("%s:info is Null\n", __func__);
		return -ENOMEM;
	}

	switch (info->op_action) {
	case TS_ACTION_READ:
		memcpy(info->tp_test_type,
			dev_data->tp_test_type, TS_CAP_TEST_TYPE_LEN);
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

void focal_param_kree(void)
{
	if (params) {
		kfree(params);
		params = NULL;
	}
}

