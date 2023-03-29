

#ifndef _FOCALTECH_TEST_FT8716_H_
#define _FOCALTECH_TEST_FT8716_H_

#include <linux/kernel.h>
#include <focaltech_core.h>

#define bool  unsigned char
#define BYTE  unsigned char
#define false 0
#define true  1

#define TX_NUM_MAX			30
#define RX_NUM_MAX			40
#define DEVICE_MODE_ADDR		0x00
#define DEVICE_MODE_WORK		0x00
#define DEVICE_MODE_FACTORY		0x40

#define FTS_SCAN_MAX_TIME		160
#define FTS_SCAN_QUERY_DELAY		8

#define FTS_5X46_SCAN_MAX_TIME          4000
#define FTS_5X46_SCAN_QUERY_DELAY      16

#define BYTES_PER_TIME			128
#define MAX_RETRY_TIMES			3
#define TP_TEST_FAILED_REASON_LEN			20
#define FTS_RESULT_CODE_LEN		4
#define FTS_TEST_NAME_LEN		16
#define FTS_STATISTICS_DATA_LEN		32
#define FTS_DATA_TYPE_DIFF		1
#define FTS_DATA_TYPE_RAW		0
#define FTS_LCD_NOISE_TEST_FRAME_COUNT	50

#define FTS_LCD_PRE_FRAME_TIME		16
#define FTS_MAX_CAP_TEST_NUM		7
#define FTS_8201_MAX_CAP_TEST_NUM	7
#define FTS_LCD_NOISE_TEST_DELAY_TIME	50
#define FTS_ENTER_FACTORY_DELAY_TIME	100
#define FTS_OPEN_HDFILT_REG_DELAY_TIME	100
#define FTS_ENTER_WORK_DELAY_TIME	200
#define FTS_WRITE_REG_DELAY_TIME	20
#define FTS_5X46_NOISE_TEST_FRAME	50
#define FTS_5X46_NOISE_TEST_HIGH_RATE	0x81

#define FTS_FT8716_PRODUCT_NAME		"barca-v8r5"
#define FTS_FT5X46_PRODUCT_NAME		"figo"

#define FTS_THRESHOLD_NAME_LEN		64

#define	TEST_SUCCESS   0
#define	SOFTWARE_REASON  1
#define	PANEL_REASON   2
#define SHORT_CIRCUIT_COUNT 3

#define FTS_REG_VALUE_0X20		0x20
#define FTS_REG_VALUE_0X21		0x21
#define FTS_REG_VALUE_0X23		0x23
#define FTS_REG_VALUE_0X31		0x31
#define FTS_REG_VALUE_0X32		0x32
#define FTS_REG_VALUE_0X0E	0x0e
#define FTS_REG_VALUE_0XA0	0xa0
#define FTS_REG_VALUE_0XD0	0xd0
#define FTS_WRITE_VALUE_0X03			0x03
#define FTS_WRITE_VALUE_0X02			0x02
#define FTS_WRITE_VALUE_0X01			0x01
#define FTS_WAIT_TIMES_VALUE			500
#define FTS_STATE_INIT_VALUE			0xff
#define FTS_10MS_MSLEEP_VALUE		10
#define FTS_40MS_MSLEEP_VALUE		40
#define FTS_50MS_MSLEEP_VALUE		50
#define FTS_80MS_MSLEEP_VALUE		80
#define FTS_200MS_MSLEEP_VALUE		200
#define FTS_5MS_MSLEEP_VALUE			5
#define FTS_INIT_VALUE					0
#define FTS_8716_VALUE					2
#define FTS_8716U_VALUE					6
#define FTS_8719_VALUE					11
#define FTS_8006U_VALUE					11/*do not use this macro because it is same with 8719,please use ic_type in dts to distinguish different ICs*/
#define OPEN_ROW_COLUMN_TEST		1
#define OPEN_LCD_NOISE_DATA_TEST	1
#define CAP_TEST_BUF_SIZE		100
#define RAW_POINT_BY_POINT_JUDGE	1
#define ROW_COLUMN_DELTA_TEST_POINT_BY_POINT	1
#define CB_TEST_POINT_BY_POINT			1
#define SHORT_TEST_POINT_BY_POINT		1
#define OPEN_TEST_POINT_BY_POINT		1
#define FTS_RETRY_READ_RAWDATA		3
#define RAW_DATA_BUF_SIZE		2
struct focal_test_threshold {
	int raw_data_min;
	int raw_data_max;

	int cb_test_min;
	int cb_test_max;

	int scap_raw_data_min;
	int scap_raw_data_max;
	
	int short_circuit_min;
	int  lcd_noise_max;
	int open_test_cb_min;
	int row_column_delta_max;
	int cb_uniformity_x;
	int cb_uniformity_y;
	int cb_increase_level;
	int cb_increase_min;
	int panel_differ_min_array[TX_NUM_MAX*RX_NUM_MAX];
	int panel_differ_max_array[TX_NUM_MAX*RX_NUM_MAX];
	int raw_data_min_array[TX_NUM_MAX*RX_NUM_MAX];
	int raw_data_max_array[TX_NUM_MAX*RX_NUM_MAX];
	int cb_test_min_array[TX_NUM_MAX*RX_NUM_MAX];
	int cb_test_max_array[TX_NUM_MAX*RX_NUM_MAX];
	int open_test_cb_min_array[TX_NUM_MAX*RX_NUM_MAX];
	int open_test_cb_max_array[TX_NUM_MAX*RX_NUM_MAX];
	int short_test_min_array[TX_NUM_MAX*RX_NUM_MAX];
	int row_column_delta_max_array[TX_NUM_MAX*RX_NUM_MAX*2];
};

struct focal_test_params {
	struct focal_test_threshold threshold;
	int row_column_delta;
	int row_column_delta_test_point_by_point;
	int lcd_noise_data;
	int in_csv_file;
	int point_by_point_judge;
	int opentest_charge_time;
	int opentest_reset_time;
	u8 channel_x_num;
	u8 channel_y_num;
	u8 key_num;
	int cb_test_point_by_point;
	int open_test_cb_point_by_point;
	int short_test_point_by_point;
};

struct focal_test_result {
	int result;
	int *values;
	size_t size;
	char test_name[FTS_TEST_NAME_LEN];
	char result_code[FTS_RESULT_CODE_LEN];
	char tptestfailedreason[TP_TEST_FAILED_REASON_LEN];
	char statistics_data[FTS_STATISTICS_DATA_LEN];
	u8 typeindex;
	char testresult;
};



int focal_parse_cap_test_config(struct focal_platform_data *pdata,
	struct focal_test_params *params);
void focal_set_max_channel_num(struct focal_test_params *params);

int focal_start_test_tp(struct focal_test_params *params,
	struct ts_rawdata_info *info);

int focal_get_debug_data_(int data_type, int *data, size_t size,int chl_x,int chl_y);
int focal_get_channel_num(u8 *chl_x, u8 *chl_y);

int focal_get_raw_data(struct ts_rawdata_info *info,
	struct ts_cmd_node *out_cmd);
int focal_get_debug_data(struct ts_diff_data_info *info,
	struct ts_cmd_node *out_cmd);
int focal_chip_get_capacitance_test_type(struct ts_test_type_info *info);
void focal_prase_threshold_for_csv(const char *project_id,
	struct focal_test_threshold *threshold,
	struct focal_test_params *params);
void focal_8201_prase_threshold_for_csv(const char *project_id,
	struct focal_test_threshold *threshold,
	struct focal_test_params *params);

#endif
