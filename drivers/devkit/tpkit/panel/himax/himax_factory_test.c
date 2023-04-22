/* Himax Android Driver Sample Code for Himax chipset
*
* Copyright (C) 2016/12/09 Himax Corporation.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include "himax_ic.h"

#define HX_FAC_LOG_PRINT	//debug dmesg rawdata test switch
#define HIMAX_PROC_FACTORY_TEST_FILE	"ts/himax_threshold.csv" //"touchscreen/tp_capacitance_data"
#define ABS(x)	(((x) < 0) ? -(x) : (x))
#define hxfree(x) do {if(x) kfree(x);x = NULL;} while(0)

#define RESULT_LEN  100

#define IIR_DIAG_COMMAND   1
#define DC_DIAG_COMMAND     2
#define BANK_DIAG_COMMAND   3
#define BASEC_GOLDENBC_DIAG_COMMAND  5
#define CLOSE_DIAG_COMMAND  0

#define IIR_CMD   1
#define DC_CMD        2
#define BANK_CMD   3
#define GOLDEN_BASEC_CMD  5
#define BASEC_CMD  7
#define RTX_DELTA_CMD  8

#define HX_RAW_DUMP_FILE "/data/hx_fac_dump.txt"

static void himax_free_Rawmem(void);
static int himax_ctoi(char *buf, uint32_t count);
static int himax_parse_threshold_file(void);
static int himax_get_one_value(const char *buf, uint32_t *offset);
static int himax_parse_threshold_file_method(const char *buf, uint32_t file_size);
static int himax_enter_iq_mode(void);
static int himax_write_iq_criteria(void);
static int himax_exit_iq_self_mode(void);
static int himax_iq_self_test(void);
static int hx852xf_self_test(void);
static int read_rawdata_sram(void);

int (*himax_factory_start)(struct himax_ts_data *ts,struct ts_rawdata_info *info_top);
/* ASCII */
#define ASCII_LF (0x0A)
#define ASCII_CR (0x0D)
#define ASCII_COMMA (0x2C)
#define ASCII_ZERO (0x30)
#define ASCII_NINE (0x39)
#define GOLDEN_DATA_NUM 1024
#define HX_CRITERIA_CNT	20
#define HX_CRITERIA_FAC_CNT 14
#define HX_CRITERIA_IQ_CNT 6
#define MAX_CASE_CNT 3
#define HX_CRITERIA 0
#define DC_GOLDEN_LIMIT 1
#define BANK_GOLDEN_LIMIT 2
#define FACTORY_TEST_START_LABEL  "CRITERIA_SELF_BANK_UP"

static uint8_t *fac_dump_buffer = NULL;
static const struct firmware *fw_entry;

int index_count = 0;

int hx_selftest_flag = 0;

uint8_t hx_criteria [HX_CRITERIA_CNT]=  {0};
uint8_t dc_golden_data[GOLDEN_DATA_NUM] = {0};
uint8_t bank_golden_data[GOLDEN_DATA_NUM] = {0};

static char buf_test_result[RESULT_LEN] = { 0 };	/*store mmi test result*/

atomic_t hmx_mmi_test_status = ATOMIC_INIT(0);
#define HX_RAW_DATA_SIZE   (PAGE_SIZE * 60)

struct ts_rawdata_info *info = NULL;
extern struct himax_ts_data *g_himax_ts_data;
extern char *himax_product_id[];

struct get_csv_data {
	uint64_t size;
	int32_t csv_data[];
};

#define CSV_CAP_RADATA_LIMIT_UP "cap_rawdata_limit_up"
#define CSV_CAP_RADATA_LIMIT_DW "cap_rawdata_limit_dw"
#define CSV_CAP_RADATA_SELF_LIMIT_UP "cap_rawdata_self_limit_up"
#define CSV_CAP_RADATA_SELF_LIMIT_DW "cap_rawdata_self_limit_dw"
#define CSV_TX_DELTA_LIMIT_UP "tx_delta_limit_up"
#define CSV_RX_DELTA_LIMIT_UP "rx_delta_limit_up"
#define CSV_BANK_SELF_LIMIT_UP "bank_self_limit_up"
#define CSV_BANK_SELF_LIMIT_DW "bank_self_limit_dw"
#define CSV_BANK_MUTUAL_LIMIT_UP "bank_mutual_limit_up"
#define CSV_BANK_MUTUAL_LIMIT_DW "bank_mutual_limit_dw"
#define CSV_AVG_BANK_LIMIT_UP "avg_bank_limit_up"
#define CSV_AVG_BANK_LIMIT_DW "avg_bank_limit_dw"
#define CSV_NOISE_LIMIT "noise_limit"
#define CSV_NOISE_SELF_LIMIT "noise_self_limit"

int32_t *p2p_on_cap_rawdata_limit_up = NULL;
int32_t *p2p_on_cap_rawdata_limit_dw = NULL;

int32_t *p2p_on_cap_rawdata_self_limit_up = NULL;
int32_t *p2p_on_cap_rawdata_self_limit_dw = NULL;

int32_t *p2p_on_tx_delta_limit_up = NULL;
int32_t *p2p_on_rx_delta_limit_up = NULL;

int32_t *p2p_bank_self_limit_up = NULL;
int32_t *p2p_bank_self_limit_dw= NULL;

int32_t *p2p_bank_mutual_limit_up= NULL;
int32_t *p2p_bank_mutual_limit_dw = NULL;

int32_t *p2p_on_noise_limit = NULL;
int32_t *p2p_on_noise_self_limit = NULL;

int32_t avg_bank_limit_up = 0;
int32_t avg_bank_limit_dw = 0;

extern uint8_t *mutual_data; /* from himax_ic*/
extern uint8_t *self_data; /*from himax_ic*/
extern int g_state_get_frame;

//{self_bank_up_limit,self_bank_down_limit,mutual_bank_up_limit,mutual_bank_down_limit,self_DC_up_limit,self_DC_down_limit,mutual_DC_up_limit,mutual_DC_down_limit,self_baseC_dev,mutual_baseC_dev,self_IIR_up_limit,multual_IIR_up_limit}

static uint8_t *mutual_iir 	= NULL;
static uint8_t *self_iir 	= NULL;

static uint8_t *tx_delta 	= NULL;
static uint8_t *rx_delta 	= NULL;

static uint8_t *mutual_bank	= NULL;
static uint8_t *self_bank 	= NULL;

static uint8_t *mutual_dc 	= NULL;
static uint8_t *self_dc 	= NULL;

static uint8_t *mutual_basec = NULL;
static uint8_t *self_basec 	= NULL;

static uint8_t *mutual_golden_basec	= NULL;
static uint8_t *self_golden_basec	= NULL;

static int current_index = 0;

static int g_hx_self_test_result = 0;

static uint8_t *mutual_tmp = NULL;
static uint8_t *self_tmp = NULL;

static uint8_t *rawdata_temp = NULL;

enum hx_test_item
{
	test0 = 0,
	test1,
	test2,
	test3,
	test4,
	test5,
	test6,
	test7,

};
enum hx_limit_index
{
	bank_self_up = 0,
	bank_self_down,
	bank_mutual_up,
	bank_mutual_down,
	dc_self_up,
	dc_self_down,
	dc_mutual_up,
	dc_mutual_down,
	basec_self,
	basec_mutual,
	iir_self,
	iir_mutual,
	delta_up,
	delta_down,

};
char hx_result_fail_str[4] = {0};
char hx_result_pass_str[4] = {0};
static int hx_result_status[8] = {0};

 /*0:bank ,2: iir, 4:basec, 6:dc, 8:golden basec*/
void himax_fac_dump(uint16_t raw_data_step,uint16_t mutual_num,uint16_t self_num,uint8_t *mutual,uint8_t *self)
{
	uint16_t raw_dump_addr = 0;

	raw_dump_addr= (mutual_num+self_num)*raw_data_step;
	TS_LOG_INFO("%s:raw_dump_addr =%d\n", __func__,raw_dump_addr);
	memcpy(fac_dump_buffer+raw_dump_addr, mutual, mutual_num);
	raw_dump_addr += mutual_num;
	TS_LOG_INFO("%s:raw_dump_addr =%d\n", __func__,raw_dump_addr);
	memcpy(fac_dump_buffer+raw_dump_addr, self, self_num);
}

static int himax_alloc_Rawmem(void)
{
	uint16_t self_num = 0;
	uint16_t mutual_num = 0;
	uint16_t tx_delta_num = 0;
	uint16_t rx_delta_num = 0;
	uint16_t rx = getXChannel();
	uint16_t tx = getYChannel();

	mutual_num	= rx * tx;
	self_num	= rx + tx;
	tx_delta_num = rx * (tx - 1);
	rx_delta_num = (rx - 1) * tx;

	mutual_tmp = kzalloc((mutual_num)*sizeof(uint8_t),GFP_KERNEL);
	if (mutual_tmp == NULL) {
		TS_LOG_ERR("%s:mutual_tmp is NULL\n", __func__);
		goto exit_mutual_tmp;
	}

	self_tmp = kzalloc((self_num)*sizeof(uint8_t),GFP_KERNEL);
	if (self_tmp == NULL) {
		TS_LOG_ERR("%s:self_tmp is NULL\n", __func__);
		goto exit_self_tmp;
	}

	mutual_bank = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_bank == NULL) {
		TS_LOG_ERR("%s:mutual_bank is NULL\n", __func__);
		goto exit_mutual_bank;
	}

	self_bank = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_bank == NULL) {
		TS_LOG_ERR("%s:self_bank is NULL\n", __func__);
		goto exit_self_bank;
	}

	mutual_dc = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_dc == NULL) {
		TS_LOG_ERR("%s:mutual_dc is NULL\n", __func__);
		goto exit_mutual_dc;
	}

	self_dc = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_dc == NULL) {
		TS_LOG_ERR("%s:self_dc is NULL\n", __func__);
		goto exit_self_dc;
	}

	tx_delta = kzalloc(tx_delta_num * sizeof(uint8_t), GFP_KERNEL);
	if (tx_delta == NULL) {
		TS_LOG_ERR("%s:tx_delta is NULL\n", __func__);
		goto exit_tx_delta;
	}

	rx_delta = kzalloc(rx_delta_num * sizeof(uint8_t), GFP_KERNEL);
	if (rx_delta == NULL) {
		TS_LOG_ERR("%s:rx_delta is NULL\n", __func__);
		goto exit_rx_delta;
	}
	mutual_basec = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_basec == NULL) {
		TS_LOG_ERR("%s:mutual_basec is NULL\n", __func__);
		goto exit_mutual_basec;
	}
	self_basec = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_basec == NULL) {
		TS_LOG_ERR("%s:self_basec is NULL\n", __func__);
		goto exit_self_basec;
	}

	mutual_golden_basec = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_golden_basec == NULL) {
		TS_LOG_ERR("%s:mutual_golden_basec is NULL\n", __func__);
		goto exit_mutual_golden_basec;
	}
	self_golden_basec = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_golden_basec == NULL) {
		TS_LOG_ERR("%s:self_golden_basec is NULL\n", __func__);
		goto exit_self_golden_basec;
	}

	mutual_iir = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_iir == NULL) {
		TS_LOG_ERR("%s:mutual_iir is NULL\n", __func__);
		goto exit_mutual_iir;
	}
	self_iir = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_iir == NULL) {
		TS_LOG_ERR("%s:self_iir is NULL\n", __func__);
		goto exit_self_iir;
	}

	memset(mutual_bank, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_bank, 0xFF, self_num * sizeof(uint8_t));
	memset(mutual_dc, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_dc, 0xFF, self_num * sizeof(uint8_t));
	memset(tx_delta, 0xFF, tx_delta_num * sizeof(uint8_t));
	memset(rx_delta, 0xFF, rx_delta_num * sizeof(uint8_t));

	memset(mutual_basec, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_basec, 0xFF, self_num * sizeof(uint8_t));
	memset(mutual_golden_basec, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_golden_basec, 0xFF, self_num * sizeof(uint8_t));

	memset(mutual_iir, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_iir, 0xFF, self_num * sizeof(uint8_t));

	return NO_ERR;

exit_self_iir:
	kfree(mutual_iir);
	mutual_iir = NULL;
exit_mutual_iir:
	kfree(self_golden_basec);
	self_golden_basec = NULL;
exit_self_golden_basec:
	kfree(mutual_golden_basec);
	mutual_golden_basec = NULL;
exit_mutual_golden_basec:
	kfree(self_basec);
	self_basec = NULL;
exit_self_basec:
	kfree(mutual_basec);
	mutual_basec = NULL;
exit_mutual_basec:
	kfree(rx_delta);
	rx_delta = NULL;
exit_rx_delta:
	kfree(tx_delta);
	tx_delta = NULL;
exit_tx_delta:
	kfree(self_dc);
	self_dc = NULL;
exit_self_dc:
	kfree(mutual_dc);
	mutual_dc = NULL;
exit_mutual_dc:
	kfree(self_bank);
	self_bank = NULL;
exit_self_bank:
	kfree(mutual_bank);
	mutual_bank = NULL;
exit_mutual_bank:
	kfree(self_tmp);
	self_tmp = NULL;
exit_self_tmp:
	kfree(mutual_tmp);
	mutual_tmp = NULL;
exit_mutual_tmp:
	return ALLOC_FAIL;
}

static void himax_free_Rawmem(void)
{
	hxfree(mutual_bank);
	hxfree(self_bank);
	hxfree(mutual_dc);
	hxfree(self_dc);
	hxfree(tx_delta);
	hxfree(rx_delta);
	hxfree(mutual_basec);
	hxfree(self_basec);
	hxfree(mutual_golden_basec);
	hxfree(self_golden_basec);
	hxfree(mutual_iir);
	hxfree(self_iir);
}
/*data write format:
8C 11   ----open to write data
8B data_address
40 data
8C 00  ----close to write data
*/
int himax_wirte_golden_data(void)
{
	int retval = -1;
	int i=0, j=0;
	int num_data = 0;
	int addr_start = 0x0160;//write golden value to flash start addr
	int remain_data_num = 0;
	uint8_t write_times = 0;
	uint8_t cmdbuf[4]  = {0};
	uint16_t x_channel = getXChannel();
	uint16_t y_channel = getYChannel();
	int m = 0;
	num_data = x_channel*y_channel + x_channel + y_channel;
	TS_LOG_INFO("Number of data = %d\n",num_data);
	if (num_data%HX_RECEIVE_BUF_MAX_SIZE)
		write_times = (num_data/HX_RECEIVE_BUF_MAX_SIZE) + 1;
	else
		write_times = num_data/HX_RECEIVE_BUF_MAX_SIZE;

	TS_LOG_INFO("Wirte Golden data - Start \n");

	//Wirte Golden data - Start
	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_write( HX_REG_SRAM_SWITCH, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_wirte_golden_data write 0x8C error!\n");
		return retval;
	}
	msleep(HX_SLEEP_10MS);

	for (j = 0; j < 2; j++){
		remain_data_num = 0;
		for (i = 0; i < write_times; i++) {
			cmdbuf[0] = ((addr_start & 0xFF00) >> 8);
			cmdbuf[1] = addr_start & 0x00FF;
			remain_data_num = num_data - i*HX_RECEIVE_BUF_MAX_SIZE;

			retval = i2c_himax_write(HX_REG_SRAM_ADDR, &cmdbuf[0], 2, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
			if( retval != 0 ){
				TS_LOG_ERR("himax_wirte_golden_data write 0x8B error!\n");
				return retval;
			}

			msleep(HX_SLEEP_10MS);
			if(j == 0){
				if(remain_data_num >= HX_RECEIVE_BUF_MAX_SIZE){
					m = i*HX_RECEIVE_BUF_MAX_SIZE;
					retval = i2c_himax_write( HX_REG_FLASH_WPLACE, &dc_golden_data[m], HX_RECEIVE_BUF_MAX_SIZE, sizeof(dc_golden_data), DEFAULT_RETRY_CNT);
					if( retval != 0 ){
						TS_LOG_ERR("himax_wirte_golden_data write 0x40 error!\n");
						return retval;
					}
					addr_start = addr_start + HX_RECEIVE_BUF_MAX_SIZE;
				} else {
					m = i*HX_RECEIVE_BUF_MAX_SIZE;
					retval = i2c_himax_write( HX_REG_FLASH_WPLACE, &dc_golden_data[m], remain_data_num, sizeof(dc_golden_data), DEFAULT_RETRY_CNT);
					if( retval != 0 ){
						TS_LOG_ERR("himax_wirte_golden_data write 0x40 error!\n");
						return retval;
					}
					addr_start = addr_start + remain_data_num;
				}
				msleep(HX_SLEEP_10MS);
			}else if(j == 1){
				if(remain_data_num >= HX_RECEIVE_BUF_MAX_SIZE){
					m = i*HX_RECEIVE_BUF_MAX_SIZE;
					retval = i2c_himax_write(HX_REG_FLASH_WPLACE, &bank_golden_data[m], HX_RECEIVE_BUF_MAX_SIZE, sizeof(bank_golden_data), DEFAULT_RETRY_CNT);
					if( retval != 0 ){
						TS_LOG_ERR("himax_wirte_golden_data write 0x40 > 128 error!\n");
						return retval;
					}
					addr_start = addr_start + HX_RECEIVE_BUF_MAX_SIZE;
				} else {
					m = i*HX_RECEIVE_BUF_MAX_SIZE;
					retval = i2c_himax_write(HX_REG_FLASH_WPLACE, &bank_golden_data[m], remain_data_num, sizeof(bank_golden_data), DEFAULT_RETRY_CNT);
					if( retval != 0 ){
						TS_LOG_ERR("himax_wirte_golden_data write 0x40 < 128 error!\n");
						return retval;
					}
					addr_start = addr_start + remain_data_num;
				}
				msleep(HX_SLEEP_10MS);
			}
		}
	}

	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_wirte_golden_data write 0x8C error!\n");
		return retval;
	}
	//Write Golden data - End
	return retval;
}

/*data write format:
8C 11   ----open to write data
8B data_address
40 data
8C 00  ----close to write data
*/
int himax_wirte_criteria_data(void)
{
	int i =0;
	int retval = -1;
	int addr_criteria = 0x98;
	uint8_t cmdbuf[4] = {0};

	for(i=0;i<14;i++) {
		TS_LOG_INFO("[Himax]: self test hx_criteria is  data[%d] = %d\n", i, hx_criteria[i]);
	}

	//Write Criteria
	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_wirte_criteria_data write 0x8C error!\n");
		return retval;
	}
	msleep(HX_SLEEP_10MS);

	cmdbuf[0] = 0x00;
	cmdbuf[1] = addr_criteria;
	retval = i2c_himax_write(HX_REG_SRAM_ADDR, &cmdbuf[0], 2, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_wirte_criteria_data write 0x8B error!\n");
		return retval;
	}

	msleep(HX_SLEEP_10MS);
	retval = i2c_himax_write( HX_REG_FLASH_WPLACE, &hx_criteria[0], 12, sizeof(hx_criteria), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_wirte_criteria_data write 0x40 error!\n");
		return retval;
	}

	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_wirte_criteria_data write 0x8C error!\n");
		return retval;
	}
	return retval;
}

/*data read format:
8C 11   ----open to write data
8B data_address
5a data
8C 00  ----close to write data
*/
int himax_read_result_data(void)
{
	int i=0;
	int retval = -1;
	int addr_result = 0x96;
	uint8_t cmdbuf[4] = {0};
	uint8_t databuf[10] = {0};

	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//read result
	if( retval != 0 ){
		TS_LOG_ERR("himax_read_result_data write 0x8C error!\n");
		return retval;
	}
	msleep(HX_SLEEP_10MS);

	cmdbuf[0] = 0x00;
	cmdbuf[1] = addr_result;

	retval = i2c_himax_write(HX_REG_SRAM_ADDR ,&cmdbuf[0], 2, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_read_result_data write 0x8B error!\n");
		return retval;
	}
	msleep(HX_SLEEP_10MS);

	retval = i2c_himax_read( HX_REG_FLASH_RPLACE, databuf, 9, sizeof(databuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_read_result_data write 0x5A error!\n");
		return retval;
	}

	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_write( HX_REG_SRAM_SWITCH ,&cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_read_result_data write 0x8C error!\n");
		return retval;
	}

	for(i=0;i<9;i++) {
		TS_LOG_INFO("[Himax]: After self test %X databuf[%d] = 0x%x\n", addr_result, i, databuf[i]);
	}
	/*databuf [0] 0xAA self test open short pass*/
	if (databuf[0]==0xAA) {
		TS_LOG_INFO("[Himax]: self-test pass\n");
		return SELF_TEST_PASS;
	} else if ((databuf[0]&0xF)==0) {
		TS_LOG_INFO("[Himax]: self-test error\n");
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_report_dsm_err(DSM_TP_RAWDATA_ERROR_NO , databuf[0]);
		#endif
		return SELF_TEST_FAIL;
	} else {
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_report_dsm_err(DSM_TP_RAWDATA_ERROR_NO , databuf[0]);
		#endif
		TS_LOG_ERR("[Himax]: self-test fail:4F\n");
		return SELF_TEST_FAIL;
	}
}

int himax_bank_test(int step) //for Rawdara
{
	int result = NO_ERR;
	int rx = getXChannel();
	int tx = getYChannel();
	int index1 = 0;
#ifdef HX_FAC_LOG_PRINT
	int m = 0;
#endif
	TS_LOG_INFO("%s: Entering\n",__func__);

	TS_LOG_INFO("Bank Start:\n");
	for(index1=0;index1<rx *tx;index1++)
	{
		if(g_himax_ts_data->p2p_test_sel) {
			hx_criteria[bank_mutual_down] = (uint8_t)p2p_on_cap_rawdata_limit_dw[index1];
			hx_criteria[bank_mutual_up] = (uint8_t)p2p_on_cap_rawdata_limit_up[index1];
		}
		if(mutual_bank[index1] < hx_criteria[bank_mutual_down] || mutual_bank[index1] > hx_criteria[bank_mutual_up])
		{
			TS_LOG_INFO("%s: mutual fail in [%d]=%d\n", __func__, index1, mutual_bank[index1]);
			result=-1;
		}
	}

	for(index1=0;index1<rx +tx;index1++)
	{
		if(g_himax_ts_data->p2p_test_sel) {
			hx_criteria[bank_self_down] = (uint8_t)p2p_on_cap_rawdata_self_limit_dw[index1];
			hx_criteria[bank_self_up] = (uint8_t)p2p_on_cap_rawdata_self_limit_up[index1];
		}
		if(self_bank[index1] < hx_criteria[bank_self_down] || self_bank[index1] > hx_criteria[bank_self_up]) {
			TS_LOG_INFO("%s: self fail in [%d]=%d\n", __func__, index1, self_bank[index1]);
			result=-1;
		}
	}

#ifdef HX_FAC_LOG_PRINT
	/*=====debug log======*/
	for (index1 = 0; index1 < rx * tx; index1++) {
		printk(KERN_CONT "%4d", mutual_bank[index1]);
		if ((index1 % rx) == (rx - 1))
		{
			m = rx + index1 / rx;
			printk(KERN_CONT " %3d\n", self_bank[m]);
		}
	}
	printk(KERN_CONT "\n");
	for (index1 = 0; index1 < rx; index1++) {
		printk(KERN_CONT "%4d", self_bank[index1]);
		if ((index1 % rx) == (rx- 1))
			printk(KERN_CONT "\n");
	}
	/*=====debug log=====*/
#endif
	TS_LOG_INFO("Bank End\n");

	if(result==0 && hx_result_status[test0] == 0)
	{
		hx_result_pass_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_pass_str ,strlen(hx_result_pass_str)+1);
		TS_LOG_INFO("%s: End --> PASS\n",__func__);
	}
	else
	{
		hx_result_fail_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_fail_str,strlen(hx_result_fail_str)+1);
		TS_LOG_INFO("%s: End --> FAIL\n",__func__);
	}

	return result;
}

int himax_self_delta_test(int step)
{
	int index1 = 0;
	int index2 = 0;
	int result = NO_ERR;
	int tx = getYChannel();
	int rx = getXChannel();
	uint16_t tx_delta_num = 0;
	uint16_t rx_delta_num = 0;
	int m = 0;
	tx_delta_num = rx*(tx-1);
	rx_delta_num = (rx-1)*tx;

	/*TX Delta*/
	TS_LOG_INFO("TX Delta Start:\n");

	for(index1 = 0;index1<tx_delta_num;index1++)
	{
		m = index1+rx;
		tx_delta[index1] = ABS(mutual_bank[m] - mutual_bank[index1]);

		if(g_himax_ts_data->p2p_test_sel) {
			hx_criteria[delta_up] = (uint8_t)p2p_on_tx_delta_limit_up[index1];
		}

		if(tx_delta[index1] > hx_criteria[delta_up]) {
			TS_LOG_INFO("%s: tx delta afail in [%d]=%d\n", __func__, index1, tx_delta[index1]);
			result=-1;
		}
	}
	TS_LOG_INFO("TX Delta End\n");

	/*RX Delta*/
	TS_LOG_INFO("RX Delta Start:\n");
	/*lint -save -e* */
	for(index1 = 1;index2<rx_delta_num;index1++)
	{
		if(index1%(rx)==0)
			continue;
		rx_delta[index2] = ABS(mutual_bank[index1] - mutual_bank[index1-1]);

		if(g_himax_ts_data->p2p_test_sel) {
			hx_criteria[delta_up] = (uint8_t)p2p_on_rx_delta_limit_up[index2];
		}

		if(rx_delta[index2] > hx_criteria[delta_up]) {
			TS_LOG_INFO("%s: rx delta afail in [%d]=%d\n", __func__, index2, rx_delta[index2]);
			result=-1;
		}
		index2++;
	}
	/*lint -restore*/
#ifdef HX_FAC_LOG_PRINT
	//=====debug log======
	printk("TX start\n");
	for (index1 = 0; index1 < tx_delta_num; index1++) {
		printk(KERN_CONT "%4d", tx_delta[index1]);
		if ((index1 % rx) == (rx - 1))
			printk(KERN_CONT "\n");
	}
	printk("RX start\n");
	for (index1 = 0; index1 < rx_delta_num; index1++) {
		printk(KERN_CONT "%4d", rx_delta[index1]);
		if ((index1 % (rx - 1)) == (rx - 2))
			printk(KERN_CONT "\n");
	}
	//=====debug log=====
#endif

	TS_LOG_INFO("RX Delta End\n");

	if(result==0)
	{
		hx_result_pass_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_pass_str ,strlen(hx_result_pass_str)+1);
		TS_LOG_INFO("%s: End --> PASS\n",__func__);
	}
	else
	{
		hx_result_fail_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_fail_str,strlen(hx_result_fail_str)+1);
		TS_LOG_INFO("%s: End --> FAIL\n",__func__);
	}

	return result;
}

int himax_iir_test(int step) //for Noise Delta
{
	int index1 = 0;
	int result = NO_ERR;
	int tx = getYChannel();
	int rx = getXChannel();
#ifdef HX_FAC_LOG_PRINT
	int m=0;
#endif
	TS_LOG_INFO("%s: Entering\n",__func__);

	TS_LOG_INFO("IIR  Start:\n");

	for(index1 = 0;index1 < rx * tx;index1++)
	{
		if(g_himax_ts_data->p2p_test_sel) {
			hx_criteria[iir_mutual] = (uint8_t)p2p_on_noise_limit[index1];
		}
		if(mutual_iir[index1] > hx_criteria[iir_mutual])
		{
			TS_LOG_INFO("%s: fail mutual in [%d]=%d, now criteria=%d\n", __func__, index1, mutual_iir[index1], hx_criteria[iir_mutual]);
			result = -HX_ERROR;
		}
	}
	for(index1=0;index1<(rx+tx);index1++)
	{
		// There is no necessary for Huwawei default printer
		if(g_himax_ts_data->p2p_test_sel) {
			hx_criteria[iir_self] = (uint8_t)p2p_on_noise_self_limit[index1];
		}
		if(self_iir[index1] > hx_criteria[iir_self])
		{
			TS_LOG_INFO("%s: fail self in [%d]=%d, criteria=%d\n", __func__, index1, self_iir[index1], hx_criteria[iir_self]);
			result = -HX_ERROR;
		}
	}

#ifdef HX_FAC_LOG_PRINT
		/*=====debug log======*/
		for (index1 = 0; index1 < rx * tx; index1++) {
			printk(KERN_CONT "%4d", mutual_iir[index1]);
			if ((index1 % rx) == (rx - 1))
			{
				m = rx + index1 / rx;
				printk(KERN_CONT " %3d\n", self_iir[m]);
			}
		}
		printk(KERN_CONT "\n");
		for (index1 = 0; index1 < rx; index1++) {
			printk(KERN_CONT "%4d", self_iir[index1]);
			if (((index1) % rx) == (rx- 1))
				printk(KERN_CONT "\n");
		}
		/*=====debug log=====*/
#endif
	TS_LOG_INFO("IIR  End\n");

	if(result==0 && hx_result_status[test0] == 0)
	{
		hx_result_pass_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_pass_str ,strlen(hx_result_pass_str)+1);
		TS_LOG_INFO("%s: End --> PASS\n",__func__);
	}
	else
	{
		hx_result_fail_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_fail_str,strlen(hx_result_fail_str)+1);
		TS_LOG_INFO("%s: End --> FAIL\n",__func__);
	}

	return result;

}

int himax_basec_test(int step) //for open/short
{
	int index1 = 0;
	int retval = NO_ERR;
	int rx = getXChannel();
	int tx = getYChannel();
#ifdef HX_FAC_LOG_PRINT
	int m=0;
#endif
	if(hx_result_status[test0] == 0)
	{
		hx_result_pass_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_pass_str ,strlen(hx_result_pass_str)+1);
		TS_LOG_INFO("%s: End --> PASS\n",__func__);
	}
	else
	{
		hx_result_fail_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_fail_str,strlen(hx_result_fail_str)+1);
		TS_LOG_INFO("%s: End --> FAIL\n",__func__);
	}
#ifdef HX_FAC_LOG_PRINT
	//=====debug log======
	printk("DC start:\n");
	for (index1 = 0; index1 < rx *tx; index1++) {
		printk("%4d", mutual_dc[index1]);
		if ((index1 % rx) == (rx - 1))
		{
			m=rx + index1/rx;
			printk(" %3d\n", self_dc[m]);
		}
	}
	printk("\n");
	for (index1 = 0; index1 < rx; index1++) {
		printk("%4d", self_dc[index1]);
		if (((index1) % rx) == (rx- 1))
		printk("\n");
	}
	printk("DC end:\n");
	//=====debug log=====
	//=====debug log======
	printk("BaseC start:\n");
	for (index1 = 0; index1 < rx *tx; index1++) {
		printk("%4d", mutual_basec[index1]);
		if ((index1 % rx) == (rx - 1))
		{
			m=rx + index1/rx;
			printk(" %3d\n", self_basec[m]);
		}
	}
	printk("\n");
	for (index1 = 0; index1 < rx; index1++) {
		printk("%4d", self_basec[index1]);
		if (((index1) % rx) == (rx- 1))
		printk("\n");
	}
	printk("BaseC end:\n");
	//=====debug log=====
	//=====debug log======
	printk("Golden BaseC start:\n");
	for (index1 = 0; index1 < rx *tx; index1++) {
		printk("%4d", mutual_golden_basec[index1]);
		if ((index1 % rx) == (rx - 1))
		{
			m=rx + index1/rx;
			printk(" %3d\n", self_golden_basec[m]);
		}
	}
	printk("\n");
	for (index1 = 0; index1 < rx; index1++) {
		printk("%4d", self_golden_basec[index1]);
		if (((index1) % rx) == (rx- 1))
		printk("\n");
	}
	printk("Golden BaseC end\n");
	//=====debug log=====
#endif

	return retval;
}
#define HX_MAX_Y_CHAN 40
#define HX_MAX_X_CHAN 40
void himax_print_rawdata(int mode)
{
	int index1=0;
	uint16_t self_num = 0;
	uint16_t mutual_num = 0;
	uint16_t x_channel = getXChannel();
	uint16_t y_channel = getYChannel();
	uint8_t self_buff[HX_MAX_X_CHAN * HX_MAX_Y_CHAN]  = {0};
	uint8_t mutual_buff[HX_MAX_X_CHAN * HX_MAX_Y_CHAN] = {0};

	if(!x_channel || x_channel > HX_MAX_X_CHAN || !y_channel || y_channel > HX_MAX_Y_CHAN) {
		TS_LOG_ERR("%s, x, y larger than defined\n", __func__);
		return;
	}

	switch(mode)
	{
		case BANK_CMD:
			mutual_num	= x_channel * y_channel;
			self_num	= x_channel + y_channel;
			memcpy(mutual_buff, mutual_bank, mutual_num);
			memcpy(self_buff, self_bank, self_num);
			break;
		case RTX_DELTA_CMD:
			mutual_num	= x_channel*(y_channel-1);
			self_num	= (x_channel-1)*y_channel;
			memcpy(mutual_buff, tx_delta, mutual_num);
			memcpy(self_buff, rx_delta, self_num);
			break;
		case IIR_CMD:
			mutual_num	= x_channel * y_channel;
			self_num	= x_channel + y_channel;
			memcpy(mutual_buff, mutual_iir, mutual_num);
			memcpy(self_buff, self_iir, self_num);
			break;
		case DC_CMD:
			mutual_num	= x_channel * y_channel;
			self_num	= x_channel + y_channel;
			memcpy(mutual_buff, mutual_dc, mutual_num);
			memcpy(self_buff, self_dc, self_num);
			break;
		case GOLDEN_BASEC_CMD:
			mutual_num	= x_channel * y_channel;
			self_num	= x_channel + y_channel;
			memcpy(mutual_buff, mutual_golden_basec, mutual_num);
			memcpy(self_buff, self_golden_basec, self_num);
			break;
		case BASEC_CMD:
			mutual_num	= x_channel * y_channel;
			self_num	= x_channel + y_channel;
			memcpy(mutual_buff, mutual_basec, mutual_num);
			memcpy(self_buff, self_basec, self_num);
			break;
	}

	for(index1 = 0;index1 < mutual_num;index1++)
	{
		info->buff[current_index++] = mutual_buff[index1];
	}

	for(index1 = 0;index1 < self_num;index1++)
	{
		info->buff[current_index++] = self_buff[index1];
	}

}

static int read_rawdata_sram(void)
{
	uint16_t self_num = 0;
	uint16_t mutual_num = 0;
	uint16_t rx =0;
	uint16_t tx =0;
	uint16_t all_rawdata_size = 0;

	int retval = NO_ERR;

	uint8_t data[4] = {0};

	rx = getXChannel();
	tx = getYChannel();

	mutual_num	= rx * tx;
	self_num	= rx + tx;

	all_rawdata_size = mutual_num + self_num;

	rawdata_temp = kzalloc((all_rawdata_size)*sizeof(uint8_t),GFP_KERNEL);
	if (rawdata_temp == NULL) {
		TS_LOG_ERR("%s:rawdata_temp is NULL\n", __func__);
		goto exit_rawdata_temp;
	}

	//change to sram test
	data[0] = HX_REG_SRAM_SWITCH;
	data[1] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_master_write(&data[0],2,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;

	//read sram for bank
	data[0] = HX_REG_SRAM_ADDR;
	/*sram bank data start addr*/
	data[1] = 0x0B;
	data[2] = 0x4C;
	retval = i2c_himax_master_write(&data[0],3,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;
	retval = i2c_himax_read(HX_REG_FLASH_RPLACE,rawdata_temp,all_rawdata_size,all_rawdata_size*sizeof(uint8_t),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_100MS);
	if(retval < 0)
	goto i2c_fail;


	memcpy(mutual_bank,&rawdata_temp[0],mutual_num);
	memcpy(self_bank,&rawdata_temp[mutual_num],self_num);

	//read sram for IIR
	data[0] = HX_REG_SRAM_ADDR;
	/*sram IIR data start addr*/
	data[1] = 0x10;
	data[2] = 0x46;

	retval = i2c_himax_master_write(&data[0],3,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;

	retval = i2c_himax_read(HX_REG_FLASH_RPLACE,rawdata_temp,all_rawdata_size,all_rawdata_size*sizeof(uint8_t),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_100MS);
	if(retval < 0)
		goto i2c_fail;

	memcpy(mutual_iir,&rawdata_temp[0],mutual_num);
	memcpy(self_iir,&rawdata_temp[mutual_num],self_num);

	//read sram for DC
	data[0] = HX_REG_SRAM_ADDR;
	/*sramDC data start addr*/
	data[1] = 0x15;
	data[2] = 0x40;
	retval = i2c_himax_master_write( &data[0],3,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;

	retval = i2c_himax_read(HX_REG_FLASH_RPLACE,rawdata_temp,all_rawdata_size,all_rawdata_size*sizeof(uint8_t),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_100MS);
	if(retval < 0)
		goto i2c_fail;

	memcpy(mutual_dc,&rawdata_temp[0],mutual_num);
	memcpy(self_dc,&rawdata_temp[mutual_num],self_num);

	//read sram for BaseC
	data[0] = HX_REG_SRAM_ADDR;
	/*sram BaseC data start addr*/
	data[1] = 0x1A;
	data[2] = 0x3A;
	retval = i2c_himax_master_write(&data[0],3,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;

	retval = i2c_himax_read(HX_REG_FLASH_RPLACE,rawdata_temp,all_rawdata_size,all_rawdata_size*sizeof(uint8_t),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_100MS);
	if(retval < 0)
		goto i2c_fail;

	memcpy(mutual_basec,&rawdata_temp[0],mutual_num);
	memcpy(self_basec,&rawdata_temp[mutual_num],self_num);

	//read sram for GoldenBaseC
	data[0] = HX_REG_SRAM_ADDR;
	/*sram GoldebBaseC data start addr*/
	data[1] = 0x1F;
	data[2] = 0x34;
	retval = i2c_himax_master_write(&data[0],3,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;

	retval = i2c_himax_read(HX_REG_FLASH_RPLACE,rawdata_temp,all_rawdata_size,all_rawdata_size*sizeof(uint8_t),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_100MS);
	if(retval < 0)
		goto i2c_fail;

	memcpy(mutual_golden_basec,&rawdata_temp[0],mutual_num);
	memcpy(self_golden_basec,&rawdata_temp[mutual_num],self_num);

	//close sram test
	data[0] = HX_REG_SRAM_SWITCH;
	data[1] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_master_write(&data[0],2,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;
	retval = i2c_himax_write_command(HX_CMD_TSSON,DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_30MS);
	if(retval < 0)
		goto i2c_fail;
	retval = i2c_himax_write_command(HX_CMD_TSSLPOUT,DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_30MS);
	if(retval < 0)
		goto i2c_fail;

	return NO_ERR;

i2c_fail:
	kfree(rawdata_temp);
	rawdata_temp = NULL;
	TS_LOG_ERR("%s: Exit because there is I2C fail.\n",__func__);
	himax_free_Rawmem();
	return I2C_WORK_ERR;
exit_rawdata_temp:
	return ALLOC_FAIL;
}
static int himax_enter_iq_mode(void)
{
	int retval = 0;
	uint8_t cmdbuf[IQ_CMDBUF_LEN] = {0};
	TS_LOG_INFO("%s: start \n", __func__);
	//close IC adc
	memset(cmdbuf, 0x00, sizeof(cmdbuf));
	retval = i2c_himax_write( HX_CMD_TSSOFF,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_CMD_TSSOFF error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);
	retval = i2c_himax_write(HX_CMD_TSSLPIN,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_CMD_TSSLPIN error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);
	//open sorting mode
	cmdbuf[0] = 0x40;//open sorting mode cmd
	retval = i2c_himax_write( HX_REG_FLASH_MODE, &cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval <0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_FLASH_MODE error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);
	//enable sram mode
	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_write( HX_REG_SRAM_SWITCH, &cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_SRAM_SWITCH error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);
	//write criteria to sram addr
	cmdbuf[0] = 0x00;
	cmdbuf[1] = HX_CMD_ADDR_CRITERIA;
	retval = i2c_himax_write(HX_REG_SRAM_ADDR, &cmdbuf[0], IQ_SRAM_CMD_LEN, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_SRAM_ADDR error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);
	TS_LOG_INFO("%s: end \n", __func__);

	return HX_OK;
}
static int himax_write_iq_criteria(void)
{
	int retval = 0;
	TS_LOG_INFO("%s: start \n", __func__);
	//write criteria csv data to 0x98~0x9D register
	retval = i2c_himax_write(HX_REG_FLASH_WPLACE,  &hx_criteria[HX_CRITERIA_FAC_CNT], HX_CRITERIA_IQ_CNT, sizeof(hx_criteria), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_FLASH_WPLACE error %d!\n",__LINE__);
		return HX_ERROR;
	}
	TS_LOG_INFO("%s: end \n", __func__);

	return HX_OK;
}
static int himax_exit_iq_self_mode(void)
{
	int retval = 0;
	uint8_t cmdbuf[IQ_CMDBUF_LEN] = {0};
	memset(cmdbuf, 0x00, sizeof(cmdbuf));
	TS_LOG_INFO("%s: start \n", __func__);
	//disable sram switch
	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH, &cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_SRAM_SWITCH error %d!\n",__LINE__);
		return HX_ERROR;
	}
	cmdbuf[0] = 0;//close flash reload cmd
	retval = i2c_himax_write(HX_REG_CLOSE_FLASH_RELOAD, &cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_CLOSE_FLASH_RELOAD error %d!\n",__LINE__);
		return HX_ERROR;
	}

	cmdbuf[0] = 0x06;//back rawdata mode cmd
	retval = i2c_himax_write( HX_REG_RAWDATA_MODE,&cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_RAWDATA_MODE error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);

	retval = i2c_himax_write( HX_CMD_TSSON,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_CMD_TSSON error %d!\n",__LINE__);
		return HX_ERROR;
	}
	//open IC adc
	msleep(HX_SLEEP_120MS);
	retval = i2c_himax_write( HX_CMD_TSSLPOUT,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_CMD_TSSLPOUT error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_2S);

	retval = i2c_himax_write(HX_CMD_TSSOFF,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_CMD_TSSOFF error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);
	//back to rawdata mode
	retval = i2c_himax_write(HX_CMD_TSSLPIN,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_CMD_TSSLPIN error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);
	cmdbuf[0] = 0x00;
	retval = i2c_himax_write(HX_REG_RAWDATA_MODE,&cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_RAWDATA_MODE error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);
	//close sram register switch
	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH ,&cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_SRAM_SWITCH error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);
	TS_LOG_INFO("%s: end \n", __func__);

	return HX_OK;
}
static int himax_iq_self_test(void)
{
	int i = 0;
	int pf_value = 0x0;
	uint8_t cmdbuf[IQ_CMDBUF_LEN] = {0};
	uint8_t valuebuf[IQ_VALUEBUF_LEN] = {0};
	int retval = 0;
	memset(cmdbuf, 0x00, sizeof(cmdbuf));
	memset(valuebuf, 0x00, sizeof(valuebuf));
	TS_LOG_INFO("%s: start \n", __func__);

	for(i = HX_CRITERIA_FAC_CNT;i < HX_CRITERIA_CNT;i++) {
		TS_LOG_INFO("[Himax]: himax_iq_self_test hx_criteria is data[%d] = %d\n", i, hx_criteria[i]);
	}
	retval = himax_enter_iq_mode();
	if(retval != 0) {
		TS_LOG_ERR("enter himax_enter_iq_mode fail!\n");
		return HX_ERROR;
	}
	retval = himax_write_iq_criteria();
	if(retval != 0) {
		TS_LOG_ERR("himax_write_iq_criteria fail!\n");
		return HX_ERROR;
	}
	retval = himax_exit_iq_self_mode();
	if(retval != 0) {
		TS_LOG_ERR("himax_exit_iq_self_mode fail!\n");
		return HX_ERROR;
	}

	/*set read addr*/
	cmdbuf[0] = 0x00;
	cmdbuf[1] = HX_CMD_ADDR_RESULT;
	retval = i2c_himax_write(HX_REG_SRAM_ADDR ,&cmdbuf[0], IQ_SRAM_CMD_LEN, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_SRAM_ADDR error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);
	//read back test result to valuebuf[0]
	retval = i2c_himax_read(HX_REG_FLASH_RPLACE, valuebuf, IQ_BACK_VAL_LEN, sizeof(valuebuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_FLASH_RPLACE error %d!\n",__LINE__);
		return HX_ERROR;
	}
	//disable sram switch
	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH ,&cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_SRAM_SWITCH error %d!\n",__LINE__);
		return HX_ERROR;
	}

	for(i=0;i<IQ_BACK_VAL_LEN;i++) {
		TS_LOG_INFO("[Himax]: After iq_self_test test %X valuebuf[%d] = 0x%x\n", HX_CMD_ADDR_RESULT,i,valuebuf[i]);
	}

	if (valuebuf[0]==0xAA) {
		TS_LOG_INFO("[Himax]: himax_iq_self_test pass\n");
		pf_value = 0x0;
	} else {
		TS_LOG_ERR("[Himax]: himax_iq_self_test fail\n");
		pf_value = 0x1;
	}
	TS_LOG_INFO("%s: end \n", __func__);
	return pf_value;
}

static int hx852xf_self_test(void)
{
	int retval = NO_ERR;
	int retry_time = 0;
	uint8_t cmdbuf[4] = {0};
	int iq_test_result = 0;
	TS_LOG_INFO("%s: start \n", __func__);

	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,IRQ_DISABLE);

	if (g_himax_ts_data->support_retry_self_test == 0) {
		iq_test_result = himax_iq_self_test();//iq_test_result   :  0 pass   1  fail
	} else {
		for (retry_time = 0; retry_time < DEFAULT_RETRY_CNT; retry_time++) {
			iq_test_result = himax_iq_self_test();//iq_test_result   :  0 pass   1  fail
			if (iq_test_result != 1) {// 1:self test fail, need retry
				break;
			}
			TS_LOG_ERR("%s, himax_iq_self_test failed retry:%d\n", __func__, retry_time);
			himax_HW_reset(HX_LOADCONFIG_EN, HX_INT_EN);
		}
	}

	if(iq_test_result != 0 )
	{
		TS_LOG_ERR("himax_iq_self_test error!\n");
	}
	TS_LOG_INFO("%s: iq_test_result= %d\n",__func__, iq_test_result);

	himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_EN);
	retval = i2c_himax_write(HX_CMD_TSSOFF,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sense off--analog- close
	if( retval != 0 )
	{
		hx_result_fail_str[0] = '0';
		TS_LOG_INFO("%s: I2C Test --> fail\n",__func__);
		strncat(buf_test_result, hx_result_fail_str, strlen(hx_result_fail_str)+1);
		goto err_i2c;
	}
	else
	{
		hx_result_pass_str[0] = '0';
		TS_LOG_INFO("%s: I2C Test --> PASS\n",__func__);
		strncat(buf_test_result, hx_result_pass_str, strlen(hx_result_pass_str)+1);
	}
	msleep(HX_SLEEP_120MS);

	retval = i2c_himax_write(HX_CMD_TSSLPIN,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sleep in-- digital - close
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);

	//Write golden data
	retval = himax_wirte_golden_data();
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}

	//Disable flash reload
	cmdbuf[0] = 0x00;
	retval = i2c_himax_write(HX_REG_CLOSE_FLASH_RELOAD, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}

	//Write Criteria
	retval = himax_wirte_criteria_data();
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}

	/* sorting mode*/
	cmdbuf[0] = 0xC0;
	retval = i2c_himax_write( HX_REG_FLASH_MODE,&cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//change to sorting mode
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);

	cmdbuf[0] = BASEC_CMD;
	retval = i2c_himax_write(HX_REG_RAWDATA_MODE,&cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//written command: will be test base c
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);

	retval = i2c_himax_write( HX_CMD_TSSON,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sense on -analog- open
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);

	retval = i2c_himax_write( HX_CMD_TSSLPOUT,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sleep out -digital- open
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	/*fw run self_test*/
	msleep(HX_SLEEP_3S);

	retval = i2c_himax_write( HX_CMD_TSSOFF,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sense off
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);
	retval = i2c_himax_write( HX_CMD_TSSLPIN,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sleep in
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);
	//get result.
	g_hx_self_test_result = himax_read_result_data();

	cmdbuf[0] = CLOSE_DIAG_COMMAND;
	retval = i2c_himax_write( HX_REG_RAWDATA_MODE,&cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sleep in
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);

	/* get rawdata from sram and sense on IC */
	retval = read_rawdata_sram();
	if(retval < 0)
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}

	TS_LOG_INFO("%s: iq_test_result = %d,g_hx_self_test_result= %d\n",__func__, iq_test_result,g_hx_self_test_result);
	retval = g_hx_self_test_result + iq_test_result;  //0 :iq and self test pass  1: iq fail  or self test fail
	TS_LOG_INFO("%s: end \n", __func__);

	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,IRQ_ENABLE);
	return retval;

err_i2c:
	TS_LOG_ERR("%s: Exit because there is I2C fail.\n",__func__);
	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,IRQ_ENABLE);
	himax_free_Rawmem();
	return I2C_WORK_ERR;
}



int hx852xf_factory_start(struct himax_ts_data *ts,struct ts_rawdata_info *info_top)
{
	int retval = NO_ERR;
	uint16_t fac_dump_step = 0; //0: bank ,2: iir, 4:basec, 6:dc, 8:golden basec
	uint16_t index1 = 0;
	uint16_t self_num = 0;
	uint16_t mutual_num = 0;
	uint16_t rx = getXChannel();
	uint16_t tx = getYChannel();
	struct file *fn = NULL;
	if(!g_himax_ts_data || !g_himax_ts_data->tskit_himax_data) {
		TS_LOG_ERR("%s, param is null\n", __func__);
		return -EINVAL;
	}

	mutual_num	= rx * tx;
	self_num	= rx + tx;

	if(atomic_read(&hmx_mmi_test_status)){
		TS_LOG_ERR("%s factory test already has been called.\n",__func__);
		return FACTORY_RUNNING;
	}

	atomic_set(&hmx_mmi_test_status, 1);

	fac_dump_buffer= kzalloc((mutual_num+self_num)*10 * sizeof(uint8_t), GFP_KERNEL);
	if (!fac_dump_buffer){
		TS_LOG_ERR("device, fac_dump_buffer is NULL \n");
		goto out;
	}

	memset(fac_dump_buffer,0x00,(mutual_num+self_num)*10 );
	/*P:self test pass flag*/
	hx_result_pass_str[1] = 'P';
	hx_result_pass_str[2] = '-';
	hx_result_pass_str[3] = '\0';
	/*F:self test fail flag*/
	hx_result_fail_str[1] = 'F';
	hx_result_fail_str[2] = '-';
	hx_result_fail_str[3] = '\0';

	TS_LOG_INFO("%s :Entering\n",__func__);

	info = info_top;

	/* init */
	if(g_himax_ts_data->suspended)
	{
		TS_LOG_ERR("%s: Already suspended. Can't do factory test. \n", __func__);
		goto out;
	}

	retval = himax_parse_threshold_file();

	if (retval < 0) {
		TS_LOG_ERR("%s: Parse .CSV file Fail.\n", __func__);
	}

	memset(buf_test_result, 0, RESULT_LEN);
	//memcpy(buf_test_result, "result: ", strlen("result: ")+1);

	TS_LOG_INFO("himax_gold_self_test enter \n");

	wake_lock(&g_himax_ts_data->ts_flash_wake_lock);

	retval = himax_alloc_Rawmem();
	if( retval != 0 )
	{
		TS_LOG_ERR("%s factory test alloc_Rawmem failed.\n",__func__);
		goto err_alloc;
	}

	/* step0: himax self test*/
	hx_result_status[test0] = hx852xf_self_test(); //0 :iq and self test pass  1: iq fail  or self test fail

	/* step1: cap rawdata */
	hx_result_status[test1] = himax_bank_test(1); //for Rawdata
	himax_fac_dump(fac_dump_step,mutual_num,self_num,mutual_bank,self_bank);
	fac_dump_step += 2;

	/* step2: TX RX Delta */
	hx_result_status[test2] = himax_self_delta_test(2); //for TRX delta

	/* step3: Noise Delta */
    hx_result_status[test3] = himax_iir_test(3); //for Noise Delta
	himax_fac_dump(fac_dump_step,mutual_num,self_num,mutual_iir,self_iir);
	fac_dump_step += 2;

	/* step4: Open/Short */
	hx_result_status[test4] = himax_basec_test(4); //for short/open
	himax_fac_dump(fac_dump_step,mutual_num,self_num,mutual_basec,self_basec);
	fac_dump_step += 2;
	himax_fac_dump(fac_dump_step,mutual_num,self_num,mutual_dc,self_dc);
	fac_dump_step += 2;
	himax_fac_dump(fac_dump_step,mutual_num,self_num,mutual_golden_basec,self_golden_basec);

	//=============Show test result===================
	strncat(buf_test_result, ";",strlen(";")+1);
	strncat(buf_test_result, STR_IC_VENDOR,strlen(STR_IC_VENDOR)+1);
	strncat(buf_test_result, "-",strlen("-")+1);
	strncat(buf_test_result, ts->tskit_himax_data->ts_platform_data->chip_data->chip_name,strlen(ts->tskit_himax_data->ts_platform_data->chip_data->chip_name)+1);
	strncat(buf_test_result, "-",strlen("-")+1);
	strncat(buf_test_result, ts->tskit_himax_data->ts_platform_data->product_name,strlen(ts->tskit_himax_data->ts_platform_data->product_name)+1);

	strncat(info->result,buf_test_result,strlen(buf_test_result)+1);

	info->buff[0] = rx;
	info->buff[1] = tx;

	/*print basec and dc*/
	current_index=2;
	himax_print_rawdata(BANK_CMD);
	himax_print_rawdata(RTX_DELTA_CMD);
	himax_print_rawdata(IIR_CMD);

	if(g_himax_ts_data->tskit_himax_data->is_ic_rawdata_proc_printf) {// print all data
		himax_print_rawdata(BASEC_CMD);
		himax_print_rawdata(DC_CMD);
		himax_print_rawdata(GOLDEN_BASEC_CMD);
	}
	info->used_size = current_index;
#ifdef HX_FAC_LOG_PRINT
	//=====debug log======
	for (index1 = 0; index1 < (mutual_num+self_num)*10; index1++) {
		printk("%4d", fac_dump_buffer[index1]);
		if ((index1 % rx) == (rx - 1))
			printk("\n");
	}
	//=====debug log=====
#endif
	//========write file into system===========
	if(!(g_himax_ts_data->tskit_himax_data->is_ic_rawdata_proc_printf)) {
		TS_LOG_INFO("%s: fac write file start\n",__func__);
		fn = filp_open(HX_RAW_DUMP_FILE,O_CREAT | O_WRONLY ,0);
		if (!IS_ERR(fn))
		{
			TS_LOG_INFO("%s: fac write file \n",__func__);
			fn->f_op->write(fn,fac_dump_buffer,(mutual_num+self_num)*10*sizeof(uint8_t),&fn->f_pos);
			filp_close(fn,NULL);
		}
		else
			TS_LOG_INFO("%s: open file fail\n",__func__);
	}
	//================================

err_alloc:
	himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_DISABLE);
	wake_unlock(&g_himax_ts_data->ts_flash_wake_lock);
out:
	if(fac_dump_buffer) {
		kfree(fac_dump_buffer);
		fac_dump_buffer = NULL;
	}
	atomic_set(&hmx_mmi_test_status, 0);
	TS_LOG_INFO("%s: End \n",__func__);
	return retval;
}
#define ADDR_IRQ_MODE_SW 0xF4
#define DATA_IRQ_MODE_SW_LEVL 0x80
#define DATA_IRQ_MODE_SW_RET 0x00
#define GET_RAWDATA_MAX_TIMES 200
int himax_get_raw_stack(int step)
{
	uint8_t command_F1h_bank[2];
	uint8_t command_f4h[2];
	uint16_t mutual_num = 0;
	uint16_t self_num = 0;
	int retval = NO_ERR;
	int loop_i =0;

	g_state_get_frame = 0x00;
	mutual_num = getXChannel() * getYChannel();
	self_num = getXChannel() + getYChannel();

	hx_selftest_flag = HX_SELFTEST_EN;
	//change fw behavior
	command_f4h[0] = ADDR_IRQ_MODE_SW;
	command_f4h[1] = DATA_IRQ_MODE_SW_LEVL;
	retval = i2c_himax_master_write(&command_f4h[0],2, sizeof(command_f4h),DEFAULT_RETRY_CNT);
	if(retval != 0)
	{
		TS_LOG_INFO("I2C Fail!\n");
		goto BUS_FAIL_END;
	}
	//=============================== Get Bank value Start===============================
	setDiagCommand(BANK_CMD);
	command_F1h_bank[0] = HX_REG_RAWDATA_MODE;
	command_F1h_bank[1] = BANK_CMD;

	retval = i2c_himax_master_write(&command_F1h_bank[0],2, sizeof(command_F1h_bank),DEFAULT_RETRY_CNT);
	if(retval != 0)
	{
		TS_LOG_INFO("I2C Fail!\n");
		goto BUS_FAIL_END;
	}

	for(loop_i =0;loop_i <300;loop_i++)
	{
		TS_LOG_INFO("loop_i = %d,hx_selftest_flag = %d",loop_i, hx_selftest_flag);
		if(himax_int_gpio_read(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id) == 0)
			himax_get_rawdata_work();
		if (hx_selftest_flag ==HX_SELFTEST_DIS)
			break;
		msleep(HX_SLEEP_10MS);
	}

	/* get bank from polling(ts_work) */
	if(mutual_data == NULL || self_data == NULL){
		TS_LOG_ERR("mutual_bank/self_bank null");
		}else{
		memcpy(mutual_bank,&mutual_data[0],mutual_num);
		memcpy(self_bank,&self_data[0],self_num);
	}
	setDiagCommand(CLOSE_DIAG_COMMAND);
	command_F1h_bank[1] = CLOSE_DIAG_COMMAND ;
	retval = i2c_himax_master_write(&command_F1h_bank[0],2, sizeof(command_F1h_bank),DEFAULT_RETRY_CNT);
	if(retval != 0)
	{
		TS_LOG_INFO("I2C Fail!\n");
		goto BUS_FAIL_END;
	}
	msleep(HX_SLEEP_10MS);
	//=============================== Get Bank value end===============================
	//=============================== Get IIR value Start===============================
	hx_selftest_flag = 1;
	g_state_get_frame = 0x00;
	setDiagCommand(IIR_CMD);
	command_F1h_bank[0] = HX_REG_RAWDATA_MODE;
	command_F1h_bank[1] = IIR_CMD;

	retval = i2c_himax_master_write(&command_F1h_bank[0],2, sizeof(command_F1h_bank),DEFAULT_RETRY_CNT);
	if(retval != 0)
	{
		TS_LOG_INFO("I2C Fail!\n");
		goto BUS_FAIL_END;
	}

	for(loop_i =0;loop_i < GET_RAWDATA_MAX_TIMES;loop_i++)
	{
		TS_LOG_INFO("loop_i = %d,hx_selftest_flag = %d",loop_i, hx_selftest_flag);
		if(himax_int_gpio_read(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id) == 0)
			himax_get_rawdata_work();
		if (hx_selftest_flag ==0)
			break;
		msleep(HX_SLEEP_10MS);
	}

	/* get bank from polling(ts_work) */
	if(mutual_data == NULL || self_data == NULL){
		TS_LOG_ERR("mutual_iir/self_iir null");
	}else {
		memcpy(mutual_iir,&mutual_data[0],mutual_num);
		memcpy(self_iir,&self_data[0],self_num);
	}
	setDiagCommand(CLOSE_DIAG_COMMAND);
	command_F1h_bank[1] = CLOSE_DIAG_COMMAND;
	retval = i2c_himax_master_write(&command_F1h_bank[0],2, sizeof(command_F1h_bank),DEFAULT_RETRY_CNT);
	if(retval != 0)
	{
		TS_LOG_ERR("I2C Fail!\n");
		goto BUS_FAIL_END;
	}
	msleep(HX_SLEEP_10MS);
	//=============================== Get IIR value End===============================

BUS_FAIL_END:

	hx_selftest_flag = HX_SELFTEST_DIS;
	//return fw behavior
	command_f4h[0] = ADDR_IRQ_MODE_SW;
	command_f4h[1] = DATA_IRQ_MODE_SW_RET;
	retval = i2c_himax_master_write(&command_f4h[0],2, sizeof(command_f4h),DEFAULT_RETRY_CNT);
	if(retval != 0)
	{
		TS_LOG_ERR("I2C Fail!\n");
	}

	if( retval != 0 )
	{
		hx_result_fail_str[0] = '0';
		TS_LOG_INFO("%s: I2C Test --> fail\n",__func__);
		strncat(buf_test_result, hx_result_fail_str, strlen(hx_result_fail_str)+1);
	}
	else
	{
		hx_result_pass_str[0] = '0';
		TS_LOG_INFO("%s: I2C Test --> PASS\n",__func__);
		strncat(buf_test_result, hx_result_pass_str, strlen(hx_result_pass_str)+1);
	}

	TS_LOG_INFO("%s:End get raw data\n", __func__);
	return NO_ERR;

}

#define HX_SEL_TEST_F1 0xF1
#define HX_SEL_TEST_F2 0xF2
#define HX_SEL_TEST_F3 0xF3
#define HX_SEL_TEST_OK 0xAA
#define BANK_UP_LIMIT 245
#define BANK_DW_LIMIT 10

static uint8_t hx852xes_self_test(int step)
{
	uint16_t i;
	uint16_t x_channel = getXChannel();
	uint16_t y_channel = getYChannel();
	uint16_t mutual_num = x_channel * y_channel;
	uint16_t self_num = x_channel + y_channel;
	uint32_t bank_sum, m;
	uint8_t bank_avg;
	uint8_t bank_ulmt[mutual_num];
	uint8_t bank_dlmt[mutual_num];
	uint8_t bank_min, bank_max;
	uint8_t slf_tx_fail_cnt, slf_rx_fail_cnt;
	uint16_t mut_fail_cnt;
	int fail_flag;
	uint8_t bank_val_tmp;

	uint8_t set_bnk_ulmt[mutual_num];
	uint8_t set_bnk_dlmt[mutual_num];
	uint8_t set_avg_bnk_ulmt;
	uint8_t set_avg_bnk_dlmt;
	uint8_t set_slf_bnk_ulmt[self_num];
	uint8_t set_slf_bnk_dlmt[self_num];

	int result = -1;

	TS_LOG_INFO("%s:Start self_test\n", __func__);

	//mutual_bank = kzalloc(mutual_num * sizeof(uint16_t), GFP_KERNEL);
	//self_bank = kzalloc(self_num * sizeof(uint16_t), GFP_KERNEL);


	TS_LOG_INFO("%s:Enter Test flow \n", __func__);
	for (i = 0; i < mutual_num; i++) {
		set_bnk_ulmt[i] = (uint8_t)(p2p_bank_mutual_limit_up[i]);
		set_bnk_dlmt[i] = (uint8_t)(p2p_bank_mutual_limit_dw[i]);
	}

	set_avg_bnk_ulmt = avg_bank_limit_up;
	set_avg_bnk_dlmt = avg_bank_limit_dw;

	for (i = 0; i < self_num; i++) {
		set_slf_bnk_ulmt[i] = (uint8_t)(p2p_bank_self_limit_up[i]);
		set_slf_bnk_dlmt[i] = (uint8_t)(p2p_bank_self_limit_dw[i]);
	}

	fail_flag = 0;
	bank_sum = 0;
	bank_avg = 0;
	mut_fail_cnt = 0;

	//Calculate Bank Average
	for (m = 0; m < mutual_num; m++)
	{
		bank_sum += (uint8_t)mutual_bank[m];
	}
	TS_LOG_INFO(" bank_sum = %d \n", bank_sum);
	bank_avg = (bank_sum / mutual_num);
	TS_LOG_INFO(" bank_avg = %d \n", bank_avg);
	//======Condition 1 devation test======Check average bank with absolute value
	if ((bank_avg > set_avg_bnk_ulmt) || (bank_avg < set_avg_bnk_dlmt))
		fail_flag = 1;
	TS_LOG_INFO(" fail_flag = %d\n", fail_flag);
	if (fail_flag)
	{
		result = HX_SEL_TEST_F1;     //Fail ID for Condition 1
		TS_LOG_INFO(" Fail = 0x%02X \n", result);
		goto END_FUNC;
	}
	else
	{
        //======Condition 2 bank open/short ======Check every block's bank with average value
#if 1//def SLF_TEST_BANK_ABS_LMT
        memcpy(bank_ulmt, set_bnk_ulmt, mutual_num);
        memcpy(bank_dlmt, set_bnk_dlmt, mutual_num);
#else
        if ((bank_avg + set_bnk_ulmt) > BANK_UP_LIMIT)
            bank_ulmt = BANK_UP_LIMIT;
        else
            bank_ulmt = bank_avg + set_bnk_ulmt;

        if (bank_avg > set_bnk_dlmt)
        {
            bank_dlmt = bank_avg - set_bnk_dlmt;
            if (bank_dlmt < BANK_DW_LIMIT)
                bank_dlmt = BANK_DW_LIMIT;
        }
        else
            bank_dlmt = BANK_DW_LIMIT;
#endif

        bank_min = 0xFF;
        bank_max = 0x00;
        // TS_LOG_INFO(" bank_ulmt = %d, bank_dlmt = %d \n", bank_ulmt, bank_dlmt);
        for (m = 0; m < mutual_num; m++)
        {
            bank_val_tmp = (uint8_t)mutual_bank[m];
            if ((bank_val_tmp > bank_ulmt[m]) || (bank_val_tmp < bank_dlmt[m])) {
                fail_flag = 1;
                mut_fail_cnt++;
            } else {
				 //TS_LOG_INFO("Value in range!\n");
			}

            //Bank information record
            if (bank_val_tmp > bank_max)
                bank_max = bank_val_tmp;
            else if (bank_val_tmp < bank_min)
                bank_min = bank_val_tmp;
			else {
				//TS_LOG_INFO("no more change edge limit value!\n");
			}
        }

        TS_LOG_INFO(" fail_flag = %d, mut_fail_cnt = %d \n", fail_flag, mut_fail_cnt);
        TS_LOG_INFO(" bank_avg = %d, bank_max = %d, bank_min = %d\n", bank_avg, bank_max, bank_min);
        if (fail_flag)
        {
            result = HX_SEL_TEST_F2; //Fail ID for Condition 2
            TS_LOG_INFO(" Fail = 0x%02X \n", result);
			goto END_FUNC;
        }
        else
        {
            //======Condition 3 self open/short======Check every self channel bank
            slf_rx_fail_cnt = 0x00; //Check SELF RX BANK
            slf_tx_fail_cnt = 0x00; //Check SELF TX BANK
            for (i = 0; i < (x_channel + y_channel); i++)
            {
                bank_val_tmp = (uint8_t)self_bank[i];
                if ((bank_val_tmp > set_slf_bnk_ulmt[i]) ||
                        (bank_val_tmp < set_slf_bnk_dlmt[i]))
                {
                    fail_flag = 1;
                    if (i < x_channel)
                        slf_rx_fail_cnt++;
                    else
                        slf_tx_fail_cnt++;
                }
            }

            TS_LOG_INFO(" slf_rx_fail_cnt = %d, slf_tx_fail_cnt = %d \n", slf_rx_fail_cnt, slf_tx_fail_cnt);
            if (fail_flag)
            {
                result = HX_SEL_TEST_F3; //Fail ID for Condition 3
                TS_LOG_INFO(" Fail = 0x%02X \n", result);
				goto END_FUNC;
            }
            else
            {
                result = HX_SEL_TEST_OK; ////PASS ID
            }
        }
    }
END_FUNC:
	if(result == HX_SEL_TEST_OK)
	{
		hx_result_pass_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_pass_str ,strlen(hx_result_pass_str)+1);
		TS_LOG_INFO("%s: End --> PASS\n",__func__);
		result = 0x00;
	} else {
		hx_result_fail_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_fail_str,strlen(hx_result_fail_str)+1);
		TS_LOG_INFO("%s: End --> FAIL\n",__func__);
	}

    return result;
}
#define FILE_PATH_MAX_LEN 100
#define FILE_NAME_MAX_LEN 64
static int himax_get_threshold_from_csvfile(int columns, int rows, char* target_name, struct get_csv_data *data)
{
	char file_path[FILE_PATH_MAX_LEN] = {0};
	char file_name[FILE_NAME_MAX_LEN] = {0};
	int ret = 0;
	int result = 0;

	TS_LOG_INFO("%s called\n", __func__);

	if (!data || !target_name || columns*rows > data->size) {
		TS_LOG_ERR("parse csvfile failed: data or target_name is NULL\n");
		return HX_ERROR;
	}

	snprintf(file_name, sizeof(file_name) - 1, "%s_%s_%s_%s_raw.csv",
			g_himax_ts_data->tskit_himax_data->ts_platform_data->product_name,
			g_himax_ts_data->tskit_himax_data->ts_platform_data->chip_data->chip_name,
			himax_product_id,
			g_himax_ts_data->tskit_himax_data->ts_platform_data->chip_data->module_name);

	snprintf(file_path, sizeof(file_path) - 1, "/odm/etc/firmware/ts/%s", file_name);
	TS_LOG_INFO("threshold file name:%s, rows_size=%d, columns_size=%d, target_name = %s\n",
		file_path, rows, columns, target_name);

	result =  ts_kit_parse_csvfile(file_path, target_name, data->csv_data, rows, columns);
	if (HX_OK == result){
		ret = HX_OK;
		TS_LOG_INFO("Get threshold successed form csvfile\n");
	} else {
		TS_LOG_INFO("csv file parse fail:%s\n", file_path);
		ret = HX_ERROR;
	}
	return ret;
}

static int himax_parse_threshold_csvfile_p2p(void)
{
	int retval = 0;
	struct get_csv_data *rawdata_limit = NULL;
	int rawdata_limit_row = getYChannel();
	int rawdata_limit_col = getXChannel();

	rawdata_limit = kzalloc(rawdata_limit_col * rawdata_limit_row * sizeof(int32_t) + sizeof(struct get_csv_data), GFP_KERNEL);
	if (NULL == rawdata_limit){
		TS_LOG_ERR("%s:rawdata_limit alloc mem fail.\n", __func__);
		return -HX_ERROR;
	}
	/*	1	*/
	rawdata_limit->size = rawdata_limit_col * rawdata_limit_row;
	if (HX_ERROR== himax_get_threshold_from_csvfile(rawdata_limit_col, rawdata_limit_row, CSV_CAP_RADATA_LIMIT_UP,rawdata_limit)) {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		memcpy (p2p_on_cap_rawdata_limit_up,rawdata_limit->csv_data, rawdata_limit_col*rawdata_limit_row * sizeof(int32_t));
	}
	/*	2	*/
	if (HX_ERROR== himax_get_threshold_from_csvfile(rawdata_limit_col, rawdata_limit_row, CSV_CAP_RADATA_LIMIT_DW,rawdata_limit)) {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		memcpy (p2p_on_cap_rawdata_limit_dw,rawdata_limit->csv_data, rawdata_limit_col*rawdata_limit_row * sizeof(int32_t));
	}

	/*	3	*/
	rawdata_limit->size = rawdata_limit_col + rawdata_limit_row;
	if (HX_ERROR== himax_get_threshold_from_csvfile(rawdata_limit_col + rawdata_limit_row, 1, CSV_CAP_RADATA_SELF_LIMIT_UP,rawdata_limit)) {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		memcpy (p2p_on_cap_rawdata_self_limit_up,rawdata_limit->csv_data, (rawdata_limit_col + rawdata_limit_row) * sizeof(int32_t));
	}
	/*	4.	*/
	if (HX_ERROR== himax_get_threshold_from_csvfile(rawdata_limit_col + rawdata_limit_row, 1, CSV_CAP_RADATA_SELF_LIMIT_DW,rawdata_limit)) {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		memcpy (p2p_on_cap_rawdata_self_limit_dw,rawdata_limit->csv_data, (rawdata_limit_col + rawdata_limit_row) * sizeof(int32_t));
	}
	/*	5	*/
	rawdata_limit->size = rawdata_limit_col *(rawdata_limit_row - 1);
	if (HX_ERROR== himax_get_threshold_from_csvfile(rawdata_limit_col, (rawdata_limit_row - 1), CSV_TX_DELTA_LIMIT_UP,rawdata_limit))  {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		memcpy (p2p_on_tx_delta_limit_up,rawdata_limit->csv_data, rawdata_limit_col*(rawdata_limit_row - 1) * sizeof(int32_t));
	}
	/*	6	*/
	rawdata_limit->size = (rawdata_limit_col - 1) * rawdata_limit_row;
	if (HX_ERROR== himax_get_threshold_from_csvfile((rawdata_limit_col - 1), rawdata_limit_row, CSV_RX_DELTA_LIMIT_UP,rawdata_limit))  {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		memcpy (p2p_on_rx_delta_limit_up,rawdata_limit->csv_data, (rawdata_limit_col - 1)*rawdata_limit_row * sizeof(int32_t));

	}
	/*	7	*/
	rawdata_limit->size = rawdata_limit_col * rawdata_limit_row;
	if (HX_ERROR== himax_get_threshold_from_csvfile(rawdata_limit_col, rawdata_limit_row, CSV_BANK_MUTUAL_LIMIT_UP,rawdata_limit))  {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		memcpy (p2p_bank_mutual_limit_up,rawdata_limit->csv_data, rawdata_limit_col*rawdata_limit_row * sizeof(int32_t));
	}
	/*	8	*/
	if (HX_ERROR== himax_get_threshold_from_csvfile(rawdata_limit_col, rawdata_limit_row, CSV_BANK_MUTUAL_LIMIT_DW,rawdata_limit))  {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		memcpy (p2p_bank_mutual_limit_dw,rawdata_limit->csv_data, rawdata_limit_col*rawdata_limit_row * sizeof(int32_t));
	}
	/*	9	*/
	rawdata_limit->size = rawdata_limit_col + rawdata_limit_row;
	if (HX_ERROR== himax_get_threshold_from_csvfile(rawdata_limit_col + rawdata_limit_row, 1, CSV_BANK_SELF_LIMIT_UP,rawdata_limit))  {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		memcpy (p2p_bank_self_limit_up,rawdata_limit->csv_data, (rawdata_limit_col + rawdata_limit_row) * sizeof(int32_t));
	}
	/*	10	*/
	if (HX_ERROR== himax_get_threshold_from_csvfile(rawdata_limit_col + rawdata_limit_row, 1, CSV_BANK_SELF_LIMIT_DW,rawdata_limit))  {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		memcpy (p2p_bank_self_limit_dw,rawdata_limit->csv_data, (rawdata_limit_col + rawdata_limit_row) * sizeof(int32_t));
	}
	/*	11	*/
	rawdata_limit->size = rawdata_limit_col * rawdata_limit_row;
	if (HX_ERROR== himax_get_threshold_from_csvfile(rawdata_limit_col, rawdata_limit_row, CSV_NOISE_LIMIT,rawdata_limit))  {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		memcpy (p2p_on_noise_limit,rawdata_limit->csv_data, rawdata_limit_col*rawdata_limit_row * sizeof(int32_t));
	}

	/*	12	*/
	rawdata_limit->size = rawdata_limit_col + rawdata_limit_row;
	if (HX_ERROR== himax_get_threshold_from_csvfile(rawdata_limit_col + rawdata_limit_row, 1, CSV_NOISE_SELF_LIMIT,rawdata_limit))  {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		TS_LOG_INFO("Success to get p2p_on_noise_self_limit\n");
		memcpy (p2p_on_noise_self_limit,rawdata_limit->csv_data, (rawdata_limit_col + rawdata_limit_row) * sizeof(int32_t));
	}

	/*	13	*/
	rawdata_limit->size = 1;
	if (HX_ERROR== himax_get_threshold_from_csvfile(1, 1, CSV_AVG_BANK_LIMIT_UP,rawdata_limit))  {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		 avg_bank_limit_up = rawdata_limit->csv_data[0];
	}

	/*	14	*/
	if (HX_ERROR== himax_get_threshold_from_csvfile(1, 1, CSV_AVG_BANK_LIMIT_DW,rawdata_limit))  {
		TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
		retval = -1;
		goto exit;
	} else {
		 avg_bank_limit_dw = rawdata_limit->csv_data[0];
	}

	exit:
	if (rawdata_limit) {
		kfree(rawdata_limit);
		rawdata_limit = NULL;
	}


	TS_LOG_INFO("%s: END\n", __func__);

	return retval;
}

static void himax_p2p_test_deinit(void)
{
	hxfree(p2p_on_cap_rawdata_limit_up);
	hxfree(p2p_on_cap_rawdata_limit_dw);
	hxfree(p2p_on_cap_rawdata_self_limit_up);
	hxfree(p2p_on_cap_rawdata_self_limit_dw);
	hxfree(p2p_bank_self_limit_up);
	hxfree(p2p_bank_self_limit_dw);
	hxfree(p2p_bank_mutual_limit_up);
	hxfree(p2p_bank_mutual_limit_dw);
	hxfree(p2p_on_tx_delta_limit_up);
	hxfree(p2p_on_rx_delta_limit_up);
	hxfree(p2p_on_noise_limit);
	hxfree(p2p_on_noise_self_limit);
}

static int himax_p2p_test_init(void)
{
	int rawdata_limit_row = getYChannel();
	int rawdata_limit_col = getXChannel();

	p2p_on_cap_rawdata_limit_up = kzalloc(rawdata_limit_col * rawdata_limit_row * sizeof(int32_t), GFP_KERNEL);
	p2p_on_cap_rawdata_limit_dw = kzalloc(rawdata_limit_col * rawdata_limit_row * sizeof(int32_t), GFP_KERNEL);
	p2p_on_cap_rawdata_self_limit_up = kzalloc((rawdata_limit_col + rawdata_limit_row) * sizeof(int32_t), GFP_KERNEL);
	p2p_on_cap_rawdata_self_limit_dw = kzalloc((rawdata_limit_col + rawdata_limit_row) * sizeof(int32_t), GFP_KERNEL);
	p2p_bank_self_limit_up = kzalloc((rawdata_limit_col + rawdata_limit_row) * sizeof(int32_t), GFP_KERNEL);
	p2p_bank_self_limit_dw = kzalloc((rawdata_limit_col + rawdata_limit_row) * sizeof(int32_t), GFP_KERNEL);
	p2p_bank_mutual_limit_up = kzalloc(rawdata_limit_col * rawdata_limit_row * sizeof(int32_t), GFP_KERNEL);
	p2p_bank_mutual_limit_dw = kzalloc(rawdata_limit_col * rawdata_limit_row * sizeof(int32_t), GFP_KERNEL);
	p2p_on_tx_delta_limit_up = kzalloc(rawdata_limit_col *(rawdata_limit_row - 1)* sizeof(int32_t), GFP_KERNEL);
	p2p_on_rx_delta_limit_up = kzalloc((rawdata_limit_col - 1) * rawdata_limit_row * sizeof(int32_t), GFP_KERNEL);
	p2p_on_noise_limit = kzalloc(rawdata_limit_col * rawdata_limit_row * sizeof(int32_t), GFP_KERNEL);
	p2p_on_noise_self_limit = kzalloc((rawdata_limit_col + rawdata_limit_row) * sizeof(int32_t), GFP_KERNEL);

	if (!p2p_on_cap_rawdata_limit_up ||
		!p2p_on_cap_rawdata_limit_dw ||
		!p2p_on_cap_rawdata_self_limit_up ||
		!p2p_on_cap_rawdata_self_limit_dw ||
		!p2p_bank_self_limit_up ||
		!p2p_bank_self_limit_dw ||
		!p2p_bank_mutual_limit_up ||
		!p2p_bank_mutual_limit_dw ||
		!p2p_on_tx_delta_limit_up ||
		!p2p_on_rx_delta_limit_up ||
		!p2p_on_noise_limit ||
		!p2p_on_noise_self_limit) {

		TS_LOG_ERR("%s: alloc mem fail! p2p_test_sel = %d \n", __func__, g_himax_ts_data->p2p_test_sel);
		return HX_ERROR;
	}
	return HX_OK;

}

int hx852xes_factory_start(struct himax_ts_data *ts,struct ts_rawdata_info *info_top)
{
	int retval = NO_ERR;
	uint16_t index1 = 0;
	uint16_t self_num = 0;
	uint16_t mutual_num = 0;
	uint16_t rx = getXChannel();
	uint16_t tx = getYChannel();
	unsigned long timer_start = 0;
	unsigned long timer_end = 0;
	struct file *fn;

	/* use for fetch test time */
	timer_start=jiffies;

	mutual_num	= rx * tx;
	self_num	= rx + tx;

	/*P:self test pass flag*/
	hx_result_pass_str[1] = 'P';
	hx_result_pass_str[2] = '-';
	hx_result_pass_str[3] = '\0';
	/*F:self test fail flag*/
	hx_result_fail_str[1] = 'F';
	hx_result_fail_str[2] = '-';
	hx_result_fail_str[3] = '\0';

	TS_LOG_INFO("%s :Entering\n",__func__);

	info = info_top;

	/* init */
	if(g_himax_ts_data->suspended)
	{
		TS_LOG_ERR("%s: Already suspended. Can't do factory test. \n", __func__);
		return SUSPEND_IN;
	}

	if(atomic_read(&hmx_mmi_test_status)){
		TS_LOG_ERR("%s factory test already has been called.\n",__func__);
		return FACTORY_RUNNING;
	}
	atomic_set(&hmx_mmi_test_status, 1);
	TS_LOG_INFO("himax_self_test enter \n");

	wake_lock(&g_himax_ts_data->ts_flash_wake_lock);

	retval = himax_alloc_Rawmem();
	if( retval != 0 )
	{
		TS_LOG_ERR("%s factory test alloc_Rawmem failed.\n",__func__);
		goto err_alloc;
	}

	retval = himax_p2p_test_init();
	if (retval)
		goto err_alloc_p2p_test;

	retval = himax_parse_threshold_csvfile_p2p();
	if (retval) {
		TS_LOG_INFO("%s: Parse .CSV file Fail.\n", __func__);
		goto err_parse_criteria_file;
	}

	memset(buf_test_result, 0, RESULT_LEN);

	/* step0: bus test*/
	hx_result_status[test0] = himax_get_raw_stack(test0); // bus test

	/* step1: cap rawdata */
	hx_result_status[test1] = himax_bank_test(test1); //for Rawdata

	/* step2: TX RX Delta */
	hx_result_status[test2] = himax_self_delta_test(test2); //for TRX delta

	/* step3: Noise Delta */
    hx_result_status[test3] = himax_iir_test(test3); //for Noise Delta

	/* step4: Open/Short */
	hx_result_status[test4] = hx852xes_self_test(test4); //for short/open

	//=============Show test result===================
	strncat(buf_test_result, STR_IC_VENDOR,strlen(STR_IC_VENDOR)+1);
	strncat(buf_test_result, "-",strlen("-")+1);
	strncat(buf_test_result, ts->tskit_himax_data->ts_platform_data->chip_data->chip_name,strlen(ts->tskit_himax_data->ts_platform_data->chip_data->chip_name)+1);
	strncat(buf_test_result, "-",strlen("-")+1);
	strncat(buf_test_result, ts->tskit_himax_data->ts_platform_data->product_name,strlen(ts->tskit_himax_data->ts_platform_data->product_name)+1);
	strncat(buf_test_result, ";",strlen(";"));

	strncat(info->result,buf_test_result,strlen(buf_test_result)+1);

	info->buff[0] = rx;
	info->buff[1] = tx;

	/*print basec and dc*/
	current_index = 2;
	himax_print_rawdata(BANK_CMD);
	himax_print_rawdata(RTX_DELTA_CMD);
	himax_print_rawdata(IIR_CMD);
	info->used_size = current_index;

	TS_LOG_INFO("%s: End \n",__func__);

	himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_DISABLE);

err_parse_criteria_file:
err_alloc_p2p_test:
	himax_free_Rawmem();
	himax_p2p_test_deinit();

	/* use for fetch test time */
	timer_end = jiffies;
	TS_LOG_INFO("%s: self test time:%d\n", __func__, jiffies_to_msecs(timer_end-timer_start));
err_alloc:
	wake_unlock(&g_himax_ts_data->ts_flash_wake_lock);
	atomic_set(&hmx_mmi_test_status, 0);

	return retval;

}

static int himax_parse_threshold_file(void)
{
	int retval = 0;
	char file_name[HIMAX_THRESHOLD_NAME_LEN] = {0};
	struct himax_ts_data *cd = NULL;

	if (!g_himax_ts_data || !g_himax_ts_data->tskit_himax_data
		|| !g_himax_ts_data->tskit_himax_data->ts_platform_data
		|| !g_himax_ts_data->tskit_himax_data->ts_platform_data->chip_data) {
		TS_LOG_ERR("param error\n");
		return -EINVAL;
	}

	cd = g_himax_ts_data;
	if (g_himax_ts_data->threshold_associated_with_projectid) {
		snprintf(file_name, sizeof(file_name) - 1, "ts/%s_%s_%s_%s_threshold.csv",
			g_himax_ts_data->tskit_himax_data->ts_platform_data->product_name,
			g_himax_ts_data->tskit_himax_data->ts_platform_data->chip_data->chip_name,
			himax_product_id,
			g_himax_ts_data->tskit_himax_data->ts_platform_data->chip_data->module_name);
	} else {
		strncpy(file_name, "ts/CHOPIN_auo_threshold.csv", strlen("ts/CHOPIN_auo_threshold.csv") + 1);
	}
	TS_LOG_INFO("%s: threshold file name:%s\n", __func__, file_name);
	retval = request_firmware(&fw_entry, file_name, cd->dev);
	if (retval < 0) {
		TS_LOG_ERR("%s: Fail request firmware\n", __func__);
		goto exit;
	}
	if (fw_entry == NULL) {
		TS_LOG_ERR("%s: fw_entry == NULL\n", __func__);
		retval = -1;
		goto exit;
	}
	if (fw_entry->data == NULL || fw_entry->size == 0) {
		TS_LOG_ERR("%s: No firmware received\n", __func__);
		retval = -2;
		goto exit;
	}
	TS_LOG_INFO("%s: fw_entry->size = %zu\n", __func__, fw_entry->size);

	TS_LOG_DEBUG("%s: Found cmcp threshold file.\n", __func__);

	retval = himax_parse_threshold_file_method(&fw_entry->data[0], fw_entry->size);
	if (retval < 0) {
		TS_LOG_ERR("%s: Parse Cmcp file\n", __func__);
		retval = -3;
		goto exit;
	}

	return retval;

exit:
	release_firmware(fw_entry);
	return retval;
}
static int himax_parse_threshold_file_method(const char *buf, uint32_t file_size)
{
	int retval = 0;
	int index1 = 0;
	int case_num = 0;
	int rx = getXChannel();
	int tx = getYChannel();
	int gold_array_size = rx * tx  + tx  + rx;
	int buf_offset = strlen(FACTORY_TEST_START_LABEL);
	int m=(int)file_size;
	TS_LOG_INFO("%s: rawdata:array_size = %d HX_RX_NUM =%d HX_TX_NUM =%d \n", __func__, gold_array_size,rx,tx);

	for(case_num = 0;case_num < MAX_CASE_CNT; case_num++) {

		switch(case_num){
			case HX_CRITERIA:
				for(index_count = 0; index_count < HX_CRITERIA_CNT; ){
					retval = himax_get_one_value(buf,&buf_offset);
					if(retval >= 0)
					{
						hx_criteria[index_count++] = retval;
						TS_LOG_INFO("%s: rawdata:hx_criteria = %d\n", __func__, retval);
					}
					else if(buf_offset >= m)
					{
						break;
					}
					else
					{
						buf_offset++;//try next
					}
				}
				continue;
			case DC_GOLDEN_LIMIT:
					for(index_count = 0; index_count < gold_array_size; )
					{
						retval = himax_get_one_value(buf,&buf_offset);
						if(retval >= 0)
						{
							dc_golden_data[index_count++] = retval;
						}
						else if(buf_offset >= m)
						{
							break;
						}
						else
						{
							buf_offset++;//try next
						}
					}
					continue;//break;
			case BANK_GOLDEN_LIMIT:
					for(index_count = 0; index_count < gold_array_size; )
					{
						retval = himax_get_one_value(buf,&buf_offset);
						if(retval >= 0)
						{
							bank_golden_data[index_count++] = retval;
						}
						else if(buf_offset >= m)
						{
							break;
						}
						else
						{
							buf_offset++;//try next
						}
					}
					continue;
		}
	}

	//=====debug log======
	printk("[himax]%s:himax criteria:",__func__);
	for (index1 = 0; index1 < HX_CRITERIA_CNT; index1++) {
		printk("%4d", hx_criteria[index1]);
	}
	printk("\n");

	printk("[himax]%s:himax dc golden:",__func__);
	for (index1 = 0; index1 < rx *tx+rx+tx; index1++) {
		printk("%4d", dc_golden_data[index1]);
		if (((index1) %  (rx-1)) == 0 && index1 > 0)
		printk("\n");

	}
	printk("\n");

	printk("[himax]%s:himax bank golden:",__func__);
	for (index1 = 0; index1 < rx *tx+rx+tx; index1++) {
		printk("%4d", bank_golden_data[index1]);
		if (((index1) %  (rx-1)) == 0 && index1 > 0)
		printk("\n");
	}

	return retval;
}

static int himax_get_one_value(const char *buf, uint32_t *offset)
{
	int value = -1;
	char tmp_buffer[10] = {0};
	uint32_t count = 0;
	uint32_t tmp_offset = *offset;
	int m=0,n=0;
	/* Bypass extra commas */
	m=tmp_offset + 1;
	while (buf[tmp_offset] == ASCII_COMMA
			&& buf[m] == ASCII_COMMA)
	{
		tmp_offset++;
		m=tmp_offset + 1;
	}
	/* Windows and Linux difference at the end of one line */
	m=tmp_offset + 1;
	n=tmp_offset + 2;
	if (buf[tmp_offset] == ASCII_COMMA
			&& buf[m] == ASCII_CR
			&& buf[n] == ASCII_LF)
		tmp_offset += 2;
	else if (buf[tmp_offset] == ASCII_COMMA
			&& buf[m] == ASCII_LF)
		tmp_offset += 1;

	/* New line for multiple lines start*/
	m=tmp_offset + 1;
	if (buf[tmp_offset] == ASCII_LF && buf[m] == ASCII_COMMA) {
		tmp_offset++;
		m=tmp_offset + 1;
//		line_count++;
		/*dev_vdbg(dev, "\n");*/
	}
	if (buf[tmp_offset] == ASCII_LF && buf[m]!= ASCII_COMMA) {
		for(;;){
			tmp_offset++;
			m=tmp_offset + 1;
			if (buf[m] == ASCII_COMMA) {
				break;
			}
		}
	}
	/* New line for multiple lines end*/
	/* Beginning */
	if (buf[tmp_offset] == ASCII_COMMA) {
		tmp_offset++;
		for (;;) {
			if ((buf[tmp_offset] >= ASCII_ZERO)
			&& (buf[tmp_offset] <= ASCII_NINE)) {
				tmp_buffer[count++] = buf[tmp_offset] - ASCII_ZERO;
				tmp_offset++;
			} else {
				if (count != 0) {
					value = himax_ctoi(tmp_buffer,count);
					/*dev_vdbg(dev, ",%d", value);*/
				} else {
					/* 0 indicates no data available */
					value = -1;
				}
				break;
			}
		}
	} else {
	/*do plus outside tmp_offset++;*/
	}

	*offset = tmp_offset;

	return value;
}

static int himax_ctoi(char *buf, uint32_t count)
{
	int value = 0;
	uint32_t index = 0;
	uint32_t base_array[] = {1, 10, 100, 1000};

	for (index = 0; index < count; index++)
		value += buf[index] * base_array[count - 1 - index];
	return value;
}

int hx852xf_factory_init(void)
{
	himax_factory_start = hx852xf_factory_start;
	return NO_ERR;
}
int hx852xes_factory_init(void)
{
	himax_factory_start = hx852xes_factory_start;
	return NO_ERR;
}
