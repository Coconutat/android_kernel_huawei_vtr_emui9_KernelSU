/*
 * File : wac_test.c
 *
 *********************************************************************
	This is not a free source file, all content are protected by NDA
 *********************************************************************
*/
//
// Test function note:
// 1. All functions call wacom i2c driver functions to communicate with wacom TC(touch controller)
//
// 2. do_query_device() was called at wacom_touch_check().
//	  This will get the basic wacom device information in struct "wacom_features"
//	  include HID_DESC for further get/set feature to wacom TC.
//
// 3. Other functions call these functions:
//	  Get Feature,  UBL_G11T_GetFeature()
//    Set Feature,  UBL_G11T_SetFeature()
//


//========================================
 // For kenel space
//========================================
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>

#include <asm-generic/errno.h>
#include "wacom.h"


// ---------------------------------Global variables-----------------------//
extern struct wacom_i2c *wac_data;

//==========================================================================
//	Pedefined parameters for project
//==========================================================================
//
//#define	WARoundup(n,d)			((INT)((n/d)+((n%d==0)?0:1)))	// Rounded up
#define PIN_CHECK_RX				9		//=WARoundup(MAX_RX, 8)
#define PIN_CHECK_TX				6		//=WARoundup(MAX_TX, 8)
#define RAW_DATA_COUNT  		4	// 16, 20	//Default parameters for RAW Check
//
//
//==========================================================================
// Macro definitions
//==========================================================================
#define FUNC_RETRY  			3
#define FIVE_HUNDRED_MS	500	// sleep 500 ms before retry
#define SEVEN_MS				7
#define TEN_MS					10
#define TRUE 1
#define FALSE 0
#define ABS(x)			(((x) < 0) ? -(x) : (x))
#define WACOM_INVALID_VALUE (0x7FFFFFFF)

#define ASCII_LF (0x0A)
#define ASCII_CR (0x0D)
#define ASCII_COMMA (0x2C)
#define ASCII_ZERO (0x30)
#define ASCII_NINE (0x39)
#define MAX_RAWDATA_VALUE  30000

#define RX_DIVIDE 8
#define TX_DIVIDE 8
#define RX_VDD_BIT 3
#define RX_GND_BIT 12
#define TX_VDD_BIT 21
#define TX_GND_BIT 27
#define PIN_CHECK_FAIL 3
#define PIN_CHECK_PASS 2
#define RX_VDD_ERR_BYTE 0
#define RX_GND_ERR_BYTE 1
#define TX_VDD_ERR_BYTE 2
#define TX_GND_ERR_BYTE 3

#define IGNORE_TOP_ITO_BYTE 0
#define IGNORE_RIGHT_ITO_BYTE 1
#define IGNORE_BOTTOM_ITO_BYTE 2
#define IGNORE_LEFT_ITO_BYTE 3

#define MAX_FAIL_REASON_SIZE 32
#define MAX_EXTRA_INFO_SIZE 200
//
//
//==========================================================================
// Global variables
//==========================================================================

// XXX (a)
//==========================================================================
//	Open device & query device, HID operation functions
//==========================================================================

static int wacom_parse_threshold_file(void);
static int wacom_parse_threshold_file_method(const char *buf, uint32_t file_size);
static int wacom_get_one_value(const char *buf, uint32_t *offset);
static int wacom_ctoi(char *buf, uint32_t count);
static int wacom_rawdata_check(void);
static int wacom_rawdata_tx2tx_diff_check(void);
static int wacom_rawdata_rx2rx_diff_check(void);
extern int wacom_gpio_reset(void);

static unsigned short max_xlut[MAX_RX] = { // XLUT
		0x003C, 0x013D, 0x013E, 0x013F, 0x0140, 0x0141, 0x0042, 0x0043,
		0x0044, 0x0031, 0x0032, 0x0133, 0x0134, 0x0135, 0x0136, 0x0137,
		0x0138, 0x0139, 0x013A, 0x013B, 0x0125, 0x0126, 0x0027, 0x0028,
		0x0129, 0x012A, 0x012B, 0x012C, 0x012D, 0x012E, 0x012F, 0x0118,
		0x0119, 0x011A, 0x011B, 0x001C, 0x001D, 0x011E, 0x011F, 0x0120,
		0x0121, 0x0122, 0x010B, 0x010C, 0x010D, 0x010E, 0x010F, 0x0110,
		0x0001, 0x0111, 0x0102, 0x0112, 0x0103, 0x0113, 0x0104, 0x0114,
		0x0105, 0x0115, 0x013A, 0x013B, 0x013C, 0x013D, 0x013E, 0x013F,
		0x0140, 0x0141, 0x0142, 0x0143, 0x0144, 0x0045, 0x0046, 0x0047
	};
static unsigned short max_ylut[MAX_TX] = { // YLUT
		0x0100, 0x0101, 0x0102, 0x0103, 0x0104, 0x0105, 0x0106, 0x0107,
		0x0108, 0x0109, 0x010A, 0x010B, 0x010C, 0x010D, 0x010E, 0x010F,
		0x0110, 0x0111, 0x0112, 0x0113, 0x0114, 0x0115, 0x0116, 0x0124,
		0x0000, 0x0000, 0x0118, 0x0119, 0x011A, 0x0121, 0x011C, 0x011D,
		0x011E, 0x0000, 0x011F, 0x0117, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0120, 0x0000, 0x011B, 0x0122, 0x0000, 0x0123
	};


static struct PANEL_INFO m_panelInfo;

//==============================================
//  HID functions
//==============================================
// Set TPKit command, for tpkit test
int set_tpkit_cmd(u8 tpkitCmd)
{
	u8 fBuf[REPSIZE_CUSTOM_CMD_G11] = {0};
	int ret= 0;

	memset(fBuf, 0x00, REPSIZE_CUSTOM_CMD_G11);
	fBuf[0] = FEATURE_REPID_CUSTOM_CMD;
	fBuf[1] = CUSTOM_CMD_TPKIT;
	fBuf[2] = tpkitCmd;
	TS_LOG_INFO("%s,set feature\n", __func__);

	ret = UBL_G11T_SetFeature(fBuf[0], REPSIZE_CUSTOM_CMD_G11, fBuf);
	msleep(10);// delay 5-10ms
	return ret;
}

// Set Report type, used for enter or exit fac test mode
int set_report_type(u8 reportType)
{
	int ret = 0;
	u8 fBuf[REPSIZE_REPORT_TYPE] = {0};

	memset(fBuf, 0, REPSIZE_REPORT_TYPE);
	fBuf[0] = FEATURE_REPID_REPORT_TYPE;
	fBuf[1] = reportType;

	ret = UBL_G11T_SetFeature(fBuf[0], REPSIZE_REPORT_TYPE, fBuf);
	msleep(10);// delay 5-10ms
	return ret;
}


// get_raw_act_data
int get_raw_act_data(int readCount, u8 data_type)
{
	u8 fBuf[REPSIZE_MNT] = {0};
	int i = 0, count=0, err_code = 0, retry_count=0;
	UINT ptr_x = 0, ptr_y = 0, idx, idy;
	TMP_DATA tmpData;
	u8 ucType = 0;
	struct wacom_i2c *data = wac_data;
	int *out_data = NULL;
	TBL_CELL sDivider = (TBL_CELL)readCount;
	int *tmp_data_pool = NULL;

	TS_LOG_INFO("%s, call\n", __func__);
	if(!data || !data->m_rawdata || !data->m_actdata) {
		TS_LOG_ERR("%s, data is empty\n", __func__);
		err_code = -EINVAL;
		goto exit;
	}
	tmp_data_pool = (int*)kzalloc(MAX_TX * MAX_RX * sizeof(int), GFP_KERNEL);
	if(tmp_data_pool == NULL) {
		TS_LOG_ERR("%s, malloc tmp_data_pool failed\n", __func__);
		err_code = -ENOMEM;
		goto exit;
	}

	// Clear the output data pool and send command to firmware
	if (TPKIT_CMD_GET_ACTDATA == data_type) {
		out_data = data->m_actdata;
	} else {
		out_data = data->m_rawdata;
	}

	memset(out_data, 0, MAX_TX * MAX_RX * sizeof(int));
	memset(&tmpData, 0, sizeof(tmpData));

	if(sDivider == 0) {
		TS_LOG_INFO("%s, readCount error, use default value 1\n", __func__);
		sDivider = 1;
	}

	err_code = set_tpkit_cmd(data_type);
	if (NO_ERR != err_code) {
		if(TPKIT_CMD_RAW_CHECK == data_type)
			TS_LOG_ERR("Failed to set TPKIT_CMD_RAW_CHECK.\n");
		else
			TS_LOG_ERR("Failed to set TPKIT_CMD_GET_ACTDATA.\n");
		goto exit;
	}

	while (count < readCount)
	{
try_raw_act_again:
		ptr_x = 0, ptr_y = 0;
		msleep(SEVEN_MS);
		memset(tmp_data_pool, 0, MAX_TX * MAX_RX * sizeof(int));
		// 1 data
		while ( TRUE )
		{
			//	Keep get data until end
			err_code = UBL_G11T_GetFeature(FEATURE_REPID_MNT, REPSIZE_MNT, fBuf);
			if (NO_ERR != err_code) {
				msleep(FIVE_HUNDRED_MS);
				break;
			}
			//msleep(10);

			ucType = fBuf[1] & 0x0F;//byte 1:uctype   rawdata or actual data
			if ((data_type != ucType) || (ptr_x != fBuf[2]) || (ptr_y != fBuf[3])) {//byte 2:x, byte 3 :y
				TS_LOG_ERR("@%s, ucType=0x%02x, ptr_x=0x%x, ptr_y=0x%x\n", __func__, ucType, ptr_x, ptr_y);
				TS_LOG_ERR("@%s, fBuf[]= 0x%x, 0x%x, 0x%x, 0x%x\n", __func__, fBuf[0], fBuf[1], fBuf[2], fBuf[3]);//0: report id
				err_code = -1;
				break;
			}

			for (i=4; i<REPSIZE_MNT; i=i+2)//4:the first four bytes:repord id, uctype, x, y
			{
				tmpData.b[0] = fBuf[i];
				tmpData.b[1] = fBuf[i+1];

				if (ptr_y != m_panelInfo.y) {
				//	(*out_data)[ptr_y][ptr_x] += (TBL_CELL)tmpData.sw;
				//  Because of error retry, we need put data into tempory pool
					tmp_data_pool[ptr_y * MAX_RX + ptr_x] = (TBL_CELL)tmpData.sw;//union,  copy here
				}
				ptr_x++;
				if (ptr_x >= m_panelInfo.x) {
					ptr_x = 0;
					ptr_y++;
				}
				if (ptr_y >= m_panelInfo.y) {
					ptr_y = 0;
					break;
				}
			} // End for

			if ((ptr_x == 0) && (ptr_y == 0)) {
				// Add value into pool for get the average at end of functions
				for (idy=0; idy<m_panelInfo.y; idy++) {
					for (idx=0; idx<m_panelInfo.x; idx++) {
						out_data[idy * MAX_RX + idx] += tmp_data_pool[idy * MAX_RX + idx];
					}
				}
				break;
			}
		} // End while TRUE
		if (NO_ERR != err_code) {
			retry_count++;
			TS_LOG_DEBUG("@%s, err=%d, retry_count= %d\n", __func__, err_code, retry_count);
			if (retry_count <= FUNC_RETRY) {
				goto try_raw_act_again;
			}
			else {
				TS_LOG_DEBUG("@%s, err, retry over %d times\n", __func__, FUNC_RETRY);
				err_code = -1;
				goto exit_get_raw_act_data;
			}
		}
		count++;
	} // End while count
	//
	//	Average
	//
	TS_LOG_INFO("%s, get %s data\n", __func__, (data_type == TPKIT_CMD_GET_ACTDATA) ? "act" :"rawdata");
	for (ptr_y=0; ptr_y<m_panelInfo.y; ptr_y++) {
		for (ptr_x=0; ptr_x<m_panelInfo.x; ptr_x++) {
			out_data[ptr_y * MAX_RX + ptr_x] /= (TBL_CELL)sDivider;
			printk("%6d", out_data[ptr_y * MAX_RX + ptr_x]);
		}
		printk("\n");
	}
exit_get_raw_act_data:
	TS_LOG_INFO("%s, get act data finish\n", __func__);
exit:
	if(tmp_data_pool) {
		kfree(tmp_data_pool);
		tmp_data_pool = NULL;
	}
	return err_code;
}

// Get Pin check
int GetPincheck(void)
{
	int i = 0, j = 0, err_code = NO_ERR;
	INT vdd = 0, gnd = 0;
	u8 fBuf[REPSIZE_MNT] = {0};
	u8 ucType = 0;
	struct wacom_i2c *data = wac_data;

	if(!data){
		TS_LOG_ERR("%s, data is empty\n", __func__);
		return -EINVAL;
	}
	memset(data->m_pinCheck.Rx, 0, sizeof(data->m_pinCheck.Rx));
	memset(data->m_pinCheck.Tx, 0, sizeof(data->m_pinCheck.Tx));
	memset(data->m_pinCheck.Error, 0, sizeof(data->m_pinCheck.Error));

	memset(fBuf, 0, REPSIZE_MNT);
	err_code=set_tpkit_cmd(TPKIT_CMD_SHORT_CHECK);
	if (NO_ERR != err_code) {
		TS_LOG_DEBUG("Failed to set CMD Pin Check.\n");
		goto out_GetPincheck;
	}
	//
	//	Get Result and Check
	//
	err_code = UBL_G11T_GetFeature(FEATURE_REPID_MNT, REPSIZE_MNT, fBuf);
	if ( NO_ERR != err_code ) {
		TS_LOG_ERR("%s, get pincheck fail\n", __func__);
		goto out_GetPincheck;
	}
	TS_LOG_INFO("%s, parse pincheck\n", __func__);
	ucType = fBuf[1] & 0x0F;//1:byte 1:uctype
	if ( ucType != TPKIT_CMD_SHORT_CHECK )
		goto out_GetPincheck;
	//
	//	Check result
	//
	// Rx
	for (i=0; i<PIN_CHECK_RX; i++) //max rx 72
	{
		for (j=0; j < RX_DIVIDE; j++)//each rx use one bit,8 means one byte
		{
			vdd = (INT)((fBuf[i + RX_VDD_BIT] >> j) & 0x01);//3:rx vdd shift bit
			gnd = (INT)((fBuf[i + RX_GND_BIT] >> j) & 0x01);//12:rx gnd shift bit

			data->m_pinCheck.Rx[RX_DIVIDE * i + j].vdd = vdd;//8 * i + j means each rx
			data->m_pinCheck.Rx[RX_DIVIDE * i + j].gnd = gnd;

			// error
			if (0 == vdd && 0 == gnd)
				data->m_pinCheck.Rx[RX_DIVIDE * i + j].state = PIN_CHECK_PASS;	//2: pass
			else {
				data->m_pinCheck.Rx[RX_DIVIDE * i + j].state = PIN_CHECK_FAIL;	//3:fail

				if (0 != vdd)
					data->m_pinCheck.Error[RX_VDD_ERR_BYTE]++;//0: rx vdd error byte

				if (0 != gnd)
					data->m_pinCheck.Error[RX_GND_ERR_BYTE]++;//1: rx gnd error byte
			}
		}
	}
	// Tx
	for (i=0; i<PIN_CHECK_TX; i++)
	{
		for (j=0; j<TX_DIVIDE; j++)
		{
			vdd = (INT)((fBuf[i + TX_VDD_BIT] >> j) & 0x01);//21:tx vdd shift bit
			gnd = (INT)((fBuf[i + TX_GND_BIT] >> j) & 0x01);//27:tx gnd shift bit

			data->m_pinCheck.Tx[TX_DIVIDE * i + j].vdd = vdd;
			data->m_pinCheck.Tx[TX_DIVIDE * i + j].gnd = gnd;

			// error
			if (0 == vdd && 0 == gnd)
				data->m_pinCheck.Tx[TX_DIVIDE * i + j].state = PIN_CHECK_PASS;	// 2:pass
			else
			{
				data->m_pinCheck.Tx[TX_DIVIDE * i + j].state = PIN_CHECK_FAIL;	// 3:fail

				if (0 != vdd)
					data->m_pinCheck.Error[TX_VDD_ERR_BYTE]++;//2:tx vdd err byte

				if (0 != gnd)
					data->m_pinCheck.Error[TX_GND_ERR_BYTE]++;//3:tx gnd err byte
			}
		}
	}
out_GetPincheck:
	TS_LOG_INFO("%s, err_code = %d\n", __func__, err_code);
	return err_code;
}
//
//==========================================================================

// XXX (b)
//==========================================================================
//	Prepare test functions
// do_init_wacom_hid(), do_set_test_mode()
//==========================================================================
//
// do_init_wacom_hid
void do_init_wacom_hid(void)
{
	int i=0;
	struct wacom_i2c *data = wac_data;
	if(!data) {
		TS_LOG_ERR("%s, data is empty\n", __func__);
	}

	// Clear the error array
	TS_LOG_INFO("%s\n", __func__);
	memset(data->m_Error, 0, sizeof(data->m_Error));

	for(i = 0; i < MAX_RX; i++){
		data->m_ini.xlut[i].w = max_xlut[i];
	}

	for(i = 0; i < MAX_TX; i++){
		data->m_ini.ylut[i].w = max_ylut[i];
	}

	//  Set the Panel X, Y, this is initial value, as real RX_COUNT, TX_COUNT
	m_panelInfo.x = data->m_ini.rx_num;//MAX_RX;
	m_panelInfo.y = data->m_ini.tx_num;
}

//
//==========================================================================


// XXX (d)
//==========================================================================
//	Check result and Dump Log functions
// Result_Short_Check(), Result_Raw_Check(), Result_Dump_Log()
//==========================================================================
//
// Return the check result
int Result_Short_Check(void)
{
	int err_code = NO_ERR;
	int i = 0, tflg = 0, rflg = 0;
	struct wacom_i2c *data = wac_data;

	if(!data) {
		TS_LOG_ERR("%s, data is empty\n", __func__);
		return -EINVAL;
	}
	//
	//	Short : Tx Vdd/Gnd
	//
	// XXX, Fix this
	// We need the LUT table to determine the error
	memset(data->m_Error, 0 , sizeof(data->m_Error));
	for (i=0; i<MAX_TX; i++)
	{
		if ( data->m_ini.ylut[i].u.en == 1 ) {//1:means tx i has been used
			if (data->m_pinCheck.Tx[i].vdd != 0) {
				data->m_Error[ERR_SHORT_TX_VDD]++;
				TS_LOG_ERR("%s, tx[%d] vdd = %d\n", __func__, i, data->m_pinCheck.Tx[i].vdd);
			}

			if (data->m_pinCheck.Tx[i].gnd != 0) {
				data->m_Error[ERR_SHORT_TX_GND]++;
				TS_LOG_ERR("%s, tx[%d] gnd = %d\n", __func__, i, data->m_pinCheck.Tx[i].gnd);
			}

			if (data->m_pinCheck.Tx[i].vdd != 0 && data->m_pinCheck.Tx[i].gnd != 0) {
				tflg++;
				TS_LOG_ERR("%s, tx[%d] vdd = %d, gnd = %d\n", __func__, i, data->m_pinCheck.Tx[i].vdd, data->m_pinCheck.Tx[i].gnd);
			}
		}
	}
	data->m_Error[ERR_SHORT_TX] = data->m_Error[ERR_SHORT_TX_VDD] + data->m_Error[ERR_SHORT_TX_GND];

	//
	//	Short : Rx Vdd/Gnd
	//
	for (i=0; i<MAX_RX; i++)
	{
		if ( data->m_ini.xlut[i].u.en == 1 ) {//1:means rx i has been used
			if (data->m_pinCheck.Rx[i].vdd != 0) {
				data->m_Error[ERR_SHORT_RX_VDD]++;
				TS_LOG_ERR("%s, Rx[%d] vdd = %d\n", __func__, i, data->m_pinCheck.Rx[i].vdd);
			}

			if (data->m_pinCheck.Rx[i].gnd != 0) {
				data->m_Error[ERR_SHORT_RX_GND]++;
				TS_LOG_ERR("%s, Rx[%d] gnd = %d\n", __func__, i, data->m_pinCheck.Rx[i].gnd);
			}

			if (data->m_pinCheck.Rx[i].vdd != 0 && data->m_pinCheck.Rx[i].gnd != 0) {
				rflg++;
				TS_LOG_ERR("%s, Rx[%d] vdd = %d, gnd = %d\n", __func__, i, data->m_pinCheck.Rx[i].vdd, data->m_pinCheck.Rx[i].gnd);
			}
		}
	}
	data->m_Error[ERR_SHORT_RX] = data->m_Error[ERR_SHORT_RX_VDD] + data->m_Error[ERR_SHORT_RX_GND];
	data->m_Error[ERR_SHORT] = data->m_Error[ERR_SHORT_TX] + data->m_Error[ERR_SHORT_RX];

	//
	//	Error analysis
	//
	// XXX, Fix this
	// Set the error string code and cat "sh" means "SHORT"
	// I change it to return error code
	if ( data->m_Error[ERR_SHORT]!= 0)
	{
		//	Tx-Tx	: VDD/GND short, two or more in the Tx
		//	Rx-Rx	: VDD/GND short, two or more in the Tx
		//	Tx-Rx	: VDD/GND short, more than one in the Tx and Rx
		//	Tx-Gnd	: short of GND only in Tx
		//	Tx-Vdd	: short of VDD only in Tx
		//	Rx-Gnd	: short of GND only in Rx
		//	Rx-Vdd	: short of VDD only in Rx

		if (tflg > 1)
		{
			TS_LOG_ERR("Short Check ... NG (Tx-Tx)");
			err_code = ERR_CODE_SHORT_TT;
		}
		else if (rflg > 1)
		{
			TS_LOG_ERR("Short Check ... NG (Rx-Rx)");
			err_code = ERR_CODE_SHORT_RR;
		}
		else if (tflg > 0 && rflg > 0)
		{
			TS_LOG_ERR("Short Check ... NG (Tx-Rx)");
			err_code = ERR_CODE_SHORT_TR;
		}
		else if (data->m_Error[ERR_SHORT_TX_GND] != 0)
		{
			TS_LOG_ERR("Short Check ... NG (Tx-Gnd)");
			err_code = ERR_CODE_SHORT_TG;
		}
		else if (data->m_Error[ERR_SHORT_TX_VDD] != 0)
		{
			TS_LOG_ERR("Short Check ... NG (Tx-Vdd)");
			err_code = ERR_CODE_SHORT_TV;
		}
		else if (data->m_Error[ERR_SHORT_RX_GND] != 0)
		{
			TS_LOG_ERR("Short Check ... NG (Rx-Gnd)");
			err_code = ERR_CODE_SHORT_RG;
		}
		else
		{
			TS_LOG_ERR("Short Check ... NG (Rx-Vdd)");
			err_code = ERR_CODE_SHORT_RV;
		}
	}
	return err_code;
}

// XXX (c)
//==========================================================================
//	Test functions
//==========================================================================
int enter_test_mode(void)
{
	return set_report_type(REPTYPE_WACOM);
}

int exit_test_mode(void)
{
	int ret = NO_ERR;
	msleep(1000);//wait for get pincheck
	ret = set_tpkit_cmd(TPKIT_CMD_RESET);

	ret = set_report_type(REPTYPE_NORMAL);//REPTYPE_NORMAL

	return ret;
}
//
//==========================================================================
//	Application Interface (API)
//==========================================================================
//

int wacom_touch_short_test(int *result)
{
	int err_code = NO_ERR;
	TS_LOG_INFO("%s call\n", __func__);
	err_code = GetPincheck();
	return err_code;
}

static int wacom_rawdata_tx2tx_diff_check(void)
{
	UINT x = 0, y = 0;
	int ret = NO_ERR;
	int gapv = 0;

	struct wacom_i2c *data = wac_data;
	int index1 = 0, index2 = 0;

	if(!data || !data->m_diff
		|| !(data->m_ini.m_gav)) {
		TS_LOG_ERR("%s, data is empty\n", __func__);
		return -EINVAL;
	}
	TS_LOG_INFO("%s ,print rawdata tx2tx diff\n", __func__);
	for (y = 0; y < m_panelInfo.y - 1; y++) {
		for (x = 0; x < m_panelInfo.x; x++) {
			index1 = y * MAX_RX + x;
			index2 = (y + 1) * MAX_RX + x;
			gapv = data->m_diff[index1] - data->m_diff[index2];
			gapv = ABS(gapv);
			printk("%6d", gapv);
		}
		printk("\n");
	}
	for (y = 0; y < m_panelInfo.y - 1; y++) {
		for (x = 0; x < m_panelInfo.x; x++) {
			index1 = y * MAX_RX + x;
			index2 = (y + 1) * MAX_RX + x;
			gapv = data->m_diff[index1] - data->m_diff[index2];
			gapv = ABS(gapv);
			if(gapv > data->m_ini.m_gav[index1]) {
				ret = -1;
				TS_LOG_ERR("%s, tx:[%d], rx:[%d], rx2rx[%d] out of limit[%d]\n", __func__,
					y, x,
					gapv, data->m_ini.m_gav[index1]);
			}
		}
	}

	return ret;
}

static int wacom_rawdata_rx2rx_diff_check(void)
{
	UINT x = 0, y = 0;
	int ret = NO_ERR;
	int gaph = 0;
	struct wacom_i2c *data = wac_data;
	int index = 0;

	if(!data || !data->m_diff
		|| !(data->m_ini.m_gap)) {
		TS_LOG_ERR("%s, data is empty\n", __func__);
		return -EINVAL;
	}
	TS_LOG_INFO("%s ,print rawdata rx2rx diff\n", __func__);

	for (y = 0; y < m_panelInfo.y; y++) {
		for (x = 0; x < m_panelInfo.x - 1; x++) {
			index = y * MAX_RX + x;
			gaph = data->m_diff[index] - data->m_diff[index + 1];
			gaph = ABS(gaph);
			printk("%6d", gaph);
		}
		printk("\n");
	}
	for (y = 0; y < m_panelInfo.y; y++) {
		for (x = 0; x < m_panelInfo.x - 1; x++) {
			gaph = data->m_diff[index] - data->m_diff[index + 1];
			gaph = ABS(gaph);
			if(gaph > data->m_ini.m_gap[index]) {
				ret = -1;
				TS_LOG_ERR("%s, tx:[%d], rx:[%d], rx2rx[%d] out of limit[%d]\n", __func__,
					y, x,
					gaph, data->m_ini.m_gap[index]);
			}
		}
	}

	return ret;
}

static int wacom_actual_check(void)
{
	int ret = NO_ERR;
	int x = 0, y = 0;
	struct wacom_i2c *data = wac_data;
	int act_value = 0;
	if(!data || !data->m_actdata) {
		TS_LOG_ERR("%s, data is empty\n", __func__);
		return -EINVAL;
	}

	memset(data->m_CalThr, 0, sizeof(data->m_CalThr));
	for (y = 0; y < m_panelInfo.y; y++) {
		for (x = 0; x < m_panelInfo.x - 1; x++) {
			if ( (!( y < data->m_ini.calth_ignoreline[IGNORE_TOP_ITO_BYTE] )) && ( y < m_panelInfo.y - data->m_ini.calth_ignoreline[IGNORE_BOTTOM_ITO_BYTE] ) &&
				(!( x < data->m_ini.calth_ignoreline[IGNORE_LEFT_ITO_BYTE] )) && ( x < m_panelInfo.x - data->m_ini.calth_ignoreline[IGNORE_RIGHT_ITO_BYTE] )
					&& ( data->m_ini.cal_thr != 0 ) )
				{
					act_value = data->m_actdata[y * MAX_RX + x];
					if (y == data->m_ini.calth_ignoreline[0]) // upper edge
					{
						data->m_CalThr[x].min.pos = data->m_ini.calth_ignoreline[0];

						data->m_CalThr[x].min.val = act_value;
						data->m_CalThr[x].max.pos = data->m_ini.calth_ignoreline[0];

						data->m_CalThr[x].max.val = act_value;
					} else {
						// min
						if (data->m_CalThr[x].min.val > act_value)
						{
							data->m_CalThr[x].min.pos = y;
							data->m_CalThr[x].min.val = act_value;
						}

						// max
						if (data->m_CalThr[x].max.val < act_value)
						{
							data->m_CalThr[x].max.pos = y;
							data->m_CalThr[x].max.val = act_value;
						}
					}
						// diff
					data->m_CalThr[x].dif = data->m_CalThr[x].max.val - data->m_CalThr[x].min.val;
					if (data->m_CalThr[x].dif > data->m_ini.cal_thr)
					{
						ret = -1;
						TS_LOG_ERR("%s, rx:[%d] actual data[%d] out of limit[%d], min,max[%d, %d]\n", __func__,
							x, data->m_CalThr[x].dif, data->m_ini.cal_thr, data->m_CalThr[x].min.val, data->m_CalThr[x].max.val);
					}
				}

		}
	}

	return ret;
}

static int wacom_rawdata_check(void)
{
	UINT x = 0, y = 0;
	int ret = NO_ERR;
	struct wacom_i2c *data = wac_data;
	int index = 0;

	if(!data || !data->m_rawdata || !data->m_diff 
		|| !(data->m_ini.m_ref) || !(data->m_ini.m_lwr) || !(data->m_ini.m_upr)
		|| !data->upper_record || !data->lower_record) {
		TS_LOG_ERR("%s, data is empty\n", __func__);
		return -EINVAL;
	}

	memset(data->upper_record, 0 , MAX_TX * MAX_RX * sizeof(int));
	memset(data->lower_record, 0 , MAX_TX * MAX_RX * sizeof(int));
	TS_LOG_INFO("%s, print rawdata diff\n", __func__);
	for (y = 0; y < m_panelInfo.y; y++) {
		for (x = 0; x < m_panelInfo.x; x++) {
			index = y * MAX_RX + x;
			data->m_diff[index] = data->m_rawdata[index] - data->m_ini.m_ref[index];
			printk("%6d", data->m_diff[index]);
		}
		printk("\n");
	}
	for (y = 0; y < m_panelInfo.y; y++) {
		for (x = 0; x < m_panelInfo.x; x++) {
			index = y * MAX_RX + x;
			//data->m_diff[index] = data->m_rawdata[index] - data->m_ini.m_ref[index];
			data->m_ini.m_lwr[index] = 0 - data->m_ini.m_upr[index];
			if (data->m_diff[index] > data->m_ini.m_upr[index]) {
				ret = -1;
				TS_LOG_ERR("%s, tx:[%d], rx:[%d] rawdata diff[%d] out of up limit[%d]\n", __func__,
					y, x,
					data->m_diff[index], data->m_ini.m_upr[index]);
				if (data->m_rawdata[y * MAX_RX + x] > MAX_RAWDATA_VALUE) {//MAX_RAWDATA_VALUE
					TS_LOG_ERR("%s, tx:[%d], rx:[%d] tx short\n", __func__, y, x);
				}
			}
			if (data->m_diff[index] < data->m_ini.m_lwr[index])
			{
				ret = -1;
				TS_LOG_ERR("%s, tx:[%d], rx:[%d] rawdata diff[%d] out of low limit[%d]\n", __func__,
					y, x,
					data->m_diff[index], data->m_ini.m_lwr[index]);
				data->lower_record[index]++;

				if ((y == 1) && (data->lower_record[x] > 0)) {
					TS_LOG_ERR("%s, tx:[%d], rx:[%d] rx open\n", __func__, y, x);

				}

				if ((y == m_panelInfo.y - 1) && (data->lower_record[(m_panelInfo.y - 2) * MAX_RX + x] > 0)){
					TS_LOG_ERR("%s, tx:[%d], rx:[%d] rx open\n", __func__, y, x);
				}

				if ((x == 1) && (data->lower_record[y * MAX_RX] > 0)){
					TS_LOG_ERR("%s, tx:[%d], rx:[%d] tx open\n", __func__, y, x);
				}
				if ((x == m_panelInfo.x - 1) && (data->lower_record[y * MAX_RX + m_panelInfo.x - 2] > 0)) {
					TS_LOG_ERR("%s, tx:[%d], rx:[%d] tx open\n", __func__, y, x);
				}
			}
		}
	}

	return ret;
}

static int wacom_request_memory(void)
{
	struct wacom_i2c *data = wac_data;
	int ret = NO_ERR;

	TS_LOG_INFO("%s call\n", __func__);
	if(!data) {
		TS_LOG_ERR("%s, data is empty\n", __func__);
		return -EINVAL;
	}
	data->m_rawdata = (int*)kzalloc(MAX_TX * MAX_RX * sizeof(int*), GFP_KERNEL);
	if(data->m_rawdata == NULL) {
		TS_LOG_ERR("%s, malloc m_rawdata failed\n", __func__);
		return -ENOMEM;
	}
	data->m_actdata = (int*)kzalloc(MAX_TX * MAX_RX * sizeof(int*), GFP_KERNEL);
	if(data->m_rawdata == NULL) {
		TS_LOG_ERR("%s, malloc m_actdata failed\n", __func__);
		return -ENOMEM;
	}
	data->m_diff = (int*)kzalloc(MAX_TX * MAX_RX * sizeof(int*), GFP_KERNEL);
	if(data->m_diff == NULL) {
		TS_LOG_ERR("%s, malloc m_diff failed\n", __func__);
		return -ENOMEM;
	}
	data->m_ini.m_ref = (int*)kzalloc(MAX_TX * MAX_RX * sizeof(int*), GFP_KERNEL);
	if(data->m_ini.m_ref == NULL) {
		TS_LOG_ERR("%s, malloc m_ref failed\n", __func__);
		return -ENOMEM;
	}
	data->m_ini.m_upr = (int*)kzalloc(MAX_TX * MAX_RX * sizeof(int*), GFP_KERNEL);
	if(data->m_ini.m_upr == NULL) {
		TS_LOG_ERR("%s, malloc m_upr failed\n", __func__);
		return -ENOMEM;
	}
	data->m_ini.m_lwr = (int*)kzalloc(MAX_TX * MAX_RX * sizeof(int*), GFP_KERNEL);
	if(data->m_ini.m_lwr == NULL) {
		TS_LOG_ERR("%s, malloc m_lwr failed\n", __func__);
		return -ENOMEM;
	}
	data->m_ini.m_gap = (int*)kzalloc(MAX_TX * MAX_RX * sizeof(int*), GFP_KERNEL);
	if(data->m_ini.m_gap == NULL) {
		TS_LOG_ERR("%s, malloc m_gap failed\n", __func__);
		return -ENOMEM;
	}
	data->m_ini.m_gav = (int*)kzalloc(MAX_TX * MAX_RX * sizeof(int*), GFP_KERNEL);
	if(data->m_ini.m_gav == NULL) {
		TS_LOG_ERR("%s, malloc m_gav failed\n", __func__);
		return -ENOMEM;
	}
	data->upper_record = (int*)kzalloc(MAX_TX * MAX_RX * sizeof(int*), GFP_KERNEL);
	if(data->upper_record == NULL) {
		TS_LOG_ERR("%s, malloc upper_record failed\n", __func__);
		return -ENOMEM;
	}
	data->lower_record = (int*)kzalloc(MAX_TX * MAX_RX * sizeof(int*), GFP_KERNEL);
	if(data->lower_record == NULL) {
		TS_LOG_ERR("%s, malloc lower_record failed\n", __func__);
		return -ENOMEM;
	}
	return ret;
}

static void wacom_free_memory(void) {
	struct wacom_i2c *data = wac_data;

	TS_LOG_INFO("%s call\n", __func__);
	if(!data) {
		TS_LOG_ERR("%s, data is empty\n", __func__);
		return;
	}
	if(data->m_rawdata) {
		kfree(data->m_rawdata);
		data->m_rawdata = NULL;
	}
	if(data->m_rawdata) {
		kfree(data->m_rawdata);
		data->m_rawdata = NULL;
	}
	if(data->m_diff) {
		kfree(data->m_diff);
		data->m_diff = NULL;
	}
	if(data->m_ini.m_ref) {
		kfree(data->m_ini.m_ref);
		data->m_ini.m_ref = NULL;
	}
	if(data->m_ini.m_upr) {
		kfree(data->m_ini.m_upr);
		data->m_ini.m_upr = NULL;
	}
	if(data->m_ini.m_lwr) {
		kfree(data->m_ini.m_lwr);
		data->m_ini.m_lwr = NULL;
	}
	if(data->m_ini.m_gap) {
		kfree(data->m_ini.m_gap);
		data->m_ini.m_gap = NULL;
	}
	if(data->m_ini.m_gav) {
		kfree(data->m_ini.m_gav);
		data->m_ini.m_gav = NULL;
	}
	if(data->upper_record) {
		kfree(data->upper_record);
		data->upper_record = NULL;
	}
	if(data->lower_record) {
		kfree(data->lower_record);
		data->lower_record = NULL;
	}
}

static int get_valid_length(int datalen, int *length)
{
	if(length == NULL || datalen <= 0 || *length <= 0) {
		TS_LOG_ERR("%s, data invalid\n", __func__);
		return 0;
	}
	if(*length >= datalen) {
		*length = *length - datalen;
		//return datalen;
	} else {
		datalen = *length;
		*length = 0;
	}
	return datalen;
}
//
//	MAIN program, The test flow must be as below
//
/*
0: I2C Communication
1: Cap Rawdata
2: Trx delta
3:  noise delta  ---Act data ÅÐ¶¨ - Calibration Threshold
4: Open/Short  ---gap check (open test)
*/
int wacom_fac_test(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd)
{
	int ret = NO_ERR, ret1= NO_ERR,ret2 = NO_ERR;
	int result = 0;
	int i = 0, j = 0;
	int index_noise = 0, index_rawdata = 0;
	char fail_reason[MAX_FAIL_REASON_SIZE] = {0};
	char extra_info[MAX_EXTRA_INFO_SIZE] = {0};
	int result_length = TS_RAWDATA_RESULT_MAX - 1;
	int valid_len = 0;

	if(!info || !out_cmd ||!wac_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	wake_lock(&wac_data->ts_wake_lock);
	ret = wacom_request_memory();
	if(ret != NO_ERR) {
		TS_LOG_ERR("%s, request memory fail\n", __func__);
		valid_len = get_valid_length(strlen("0F-1F-2F-3F-4F"), &result_length);
		strncat(info->result, "0F-1F-2F-3F-4F", valid_len);
		strncpy(fail_reason, "request_mem_fail", sizeof(fail_reason));
		goto out;
	}

	ret = wacom_parse_threshold_file();
	if(ret != NO_ERR) {
		TS_LOG_ERR("%s, wacom_parse_threshold_file failed\n", __func__);
		valid_len = get_valid_length(strlen("0F-1F-2F-3F-4F"), &result_length);
		strncat(info->result, "0F-1F-2F-3F-4F", valid_len);
		strncpy(fail_reason, "parse_file_fail", sizeof(fail_reason));
		goto out;
	}

	// Init HID device and start check
	do_init_wacom_hid();

	//	Set Test mode before go check, ask firmware get into test mode
	ret = enter_test_mode();
	//I2C communicate
	if(ret != NO_ERR){
		valid_len = get_valid_length(strlen("0F"), &result_length);
		strncat(info->result, "0F", valid_len);
		strncpy(fail_reason, "i2c fail", sizeof(fail_reason));
		TS_LOG_ERR("%s, enter_test_mode_fail", __func__);
		goto out;

	}else{
		valid_len = get_valid_length(strlen("0P"), &result_length);
		strncat(info->result, "0P", valid_len);
	}
	//input reference table, upper limit, lower limit
	ret = get_raw_act_data(wac_data->m_ini.data_count, TPKIT_CMD_RAW_CHECK);
	if(ret != NO_ERR) {
		TS_LOG_ERR("%s, get rawdata failed\n", __func__);
		valid_len = get_valid_length(strlen("-1F"), &result_length);
		strncat(info->result, "-1F", valid_len);
		strncpy(fail_reason, "get_rawdata_fail", sizeof(fail_reason));
		goto exit_test;
	} else {
		TS_LOG_ERR("%s, get rawdata success\n", __func__);
	}

	ret1 = wacom_rawdata_check();
	if(ret1 != NO_ERR) {
		TS_LOG_ERR("%s, rawdata check failed\n", __func__);
		valid_len = get_valid_length(strlen("-1F"), &result_length);
		strncat(info->result, "-1F", valid_len);
	} else {
		TS_LOG_INFO("%s, wacom_rawdata_check success\n", __func__);
		valid_len = get_valid_length(strlen("-1P"), &result_length);
		strncat(info->result, "-1P", valid_len);
	}
	//2F rawdata diff
	ret1 = wacom_rawdata_rx2rx_diff_check();
	if(NO_ERR != ret1) {
		TS_LOG_ERR("%s, rawdata rx2rx diff check failed\n", __func__);
	}
	ret2 = wacom_rawdata_tx2tx_diff_check();
	if(NO_ERR != ret2) {
		TS_LOG_ERR("%s, rawdata tx2tx diff check failed\n", __func__);
	}
	if(ret1 || ret2) {
		valid_len = get_valid_length(strlen("-2F"), &result_length);
		strncat(info->result, "-2F", valid_len);
	} else {
		valid_len = get_valid_length(strlen("-2P"), &result_length);
		strncat(info->result, "-2P", valid_len);
	}

	//3F  3:  noise delta  ---Act data check - Calibration Threshold
	ret = get_raw_act_data(wac_data->m_ini.data_count, TPKIT_CMD_GET_ACTDATA);
	if(ret != NO_ERR) {
		TS_LOG_ERR("%s, get act data failed\n", __func__);
		valid_len = get_valid_length(strlen("-3F"), &result_length);
		strncat(info->result, "-3F", valid_len);
		strncpy(fail_reason, "get_actual_data_fail", sizeof(fail_reason));
		goto exit_test;
	} else {
		TS_LOG_INFO("%s, get act data success\n", __func__);
	}

	ret1 = wacom_actual_check();
	if(ret1 != NO_ERR) {
		TS_LOG_ERR("%s, wacom_actual_check failed\n", __func__);
		valid_len = get_valid_length(strlen("-3F"), &result_length);
		strncat(info->result, "-3F", valid_len);
	} else {
		valid_len = get_valid_length(strlen("-3P"), &result_length);
		strncat(info->result, "-3P", valid_len);
		TS_LOG_INFO("%s, wacom_actual_check success\n", __func__);
	}

	info->buff[0] = wac_data->m_ini.rx_num;
	info->buff[1] = wac_data->m_ini.tx_num;
	if(wac_data->m_rawdata && wac_data->m_actdata 
		&& ((wac_data->m_ini.tx_num * wac_data->m_ini.rx_num) * 2 + 2 < TS_RAWDATA_BUFF_MAX)) {
		for(i = 0; i < wac_data->m_ini.tx_num; i++){
			for(j = 0; j < wac_data->m_ini.rx_num; j++) {
				index_rawdata = 2 + i * wac_data->m_ini.rx_num + j;
				index_noise = index_rawdata + wac_data->m_ini.tx_num * wac_data->m_ini.rx_num;
				info->buff[index_rawdata] = wac_data->m_rawdata[i * MAX_RX + j];
				info->buff[index_noise] = wac_data->m_actdata[i * MAX_RX + j];
			}
		}
	} else {
		TS_LOG_ERR("%s, tx num or rx num invalid\n", __func__);
	}
	info->used_size = (info->buff[0]) * (info->buff[1]) * 2 + 2;// shift rx, tx, rawdata, actual data
	TS_LOG_INFO("%s, buff_size = %d\n", __func__, info->used_size);

	ret = wacom_touch_short_test(&result);
	if(ret != NO_ERR) {
		TS_LOG_ERR("%s, short test failed\n", __func__);
		valid_len = get_valid_length(strlen("-4F"), &result_length);
		strncat(info->result, "-4F", valid_len);
		strncpy(fail_reason, "short_test_i2c_fail", sizeof(fail_reason));
		goto exit_test;
	} else {
		TS_LOG_INFO("%s, short test success\n", __func__);
	}

	ret1 = Result_Short_Check();
	if(ret1 != NO_ERR) {
		TS_LOG_ERR("%s, short check failed\n", __func__);
		valid_len = get_valid_length(strlen("-4F"), &result_length);
		strncat(info->result, "-4F", valid_len);
	} else {
		TS_LOG_INFO("%s, short check success\n", __func__);
		valid_len = get_valid_length(strlen("-4P"), &result_length);
		strncat(info->result, "-4P", valid_len);
	}
exit_test:
	ret1 = exit_test_mode();
	if(ret1 != NO_ERR) {
		TS_LOG_ERR("%s, exit_test_mode failed\n", __func__);
	}else {
		TS_LOG_INFO("%s, exit test mode success\n", __func__);
	}

out:
	wacom_gpio_reset();
	strncat(extra_info, ";", strlen(";"));
	if(ret != NO_ERR) {
		strncat(extra_info, "-", strlen("-"));
		strncat(extra_info, fail_reason, sizeof(fail_reason));
	}
	strncat(extra_info, "-", strlen("-"));
	strncat(extra_info, wac_data->chip_name, strlen(wac_data->chip_name));
	strncat(extra_info, "-", strlen("-"));
	strncat(extra_info, wac_data->tp_module_name, strlen(wac_data->tp_module_name));
	strncat(extra_info, "-", strlen("-"));
	strncat(extra_info, wac_data->lcd_module_name, strlen(wac_data->lcd_module_name));
	valid_len = get_valid_length(strlen(extra_info), &result_length);
	TS_LOG_INFO("%s, result_length = %d\n", __func__, result_length);
	strncat(info->result, extra_info, valid_len);
	wacom_free_memory();
	wake_unlock(&wac_data->ts_wake_lock);

	TS_LOG_INFO("%s, %s\n", __func__, info->result);
	return ret;

}

static void wacom_get_threshold_file_name(char *file_name)
{
	char tmp_name[MAX_FILE_NAME_LENGTH] = {0};
	if( !file_name || !wac_data )
	{
		TS_LOG_ERR("%s : data is null\n", __func__);
		return;
	}
	strncat(tmp_name, wac_data->wacom_chip_data->ts_platform_data->product_name, MAX_STR_LEN);
	strncat(tmp_name, "_", 1);
	strncat(tmp_name, wac_data->chip_name, MAX_STR_LEN);
	strncat(tmp_name, "_", 1);
	strncat(tmp_name, wac_data->project_id, MAX_STR_LEN);
	strncat(tmp_name, "_", 1);
	strncat(tmp_name, wac_data->tp_module_name, MAX_STR_LEN);
	strncat(tmp_name, "_", 1);
	strncat(tmp_name, wac_data->lcd_module_name, MAX_STR_LEN);

	snprintf(file_name, MAX_FILE_NAME_LENGTH, "ts/%s.csv", tmp_name);
	//firmware name is like:cameron_W9015_CAME58000_ofilm_inx
	TS_LOG_INFO("%s, threshold_file name: %s\n",__func__,file_name);
}


static int wacom_parse_threshold_file(void)
{
	int retval = 0;
	const struct firmware *thresholdfile = NULL;
	char file_name[MAX_FILE_NAME_LENGTH] = {0};
	if( !wac_data || !wac_data->wacom_dev) {
		TS_LOG_ERR("%s, para invalid\n", __func__);
		return -EINVAL;
	}
	wacom_get_threshold_file_name(file_name);
	retval = request_firmware(&thresholdfile, file_name, &wac_data->wacom_dev->dev);
	if (retval < 0) {
		TS_LOG_ERR("%s: Fail request %s\n", __func__, file_name);
		return -EINVAL;
	}
	if (thresholdfile == NULL) {
		TS_LOG_ERR("%s: thresholdfile == NULL\n", __func__);
		retval = -EINVAL;
		goto exit;
	}
	if (thresholdfile->data == NULL || thresholdfile->size == 0) {
		TS_LOG_ERR("%s: No firmware received\n", __func__);
		retval = -EINVAL;
		goto exit;
	}
	TS_LOG_INFO("%s: thresholdfile->size = %zu\n", __func__, thresholdfile->size);

	TS_LOG_INFO("%s: Found threshold file.\n", __func__);

	retval = wacom_parse_threshold_file_method(&thresholdfile->data[0], thresholdfile->size);
	if (retval < 0) {
		TS_LOG_ERR("%s: Parse threshold file failed\n", __func__);
		retval = -EINVAL;
		goto exit;
	}

exit:
	release_firmware(thresholdfile);
	return retval;
}

static int wacom_get_single_item(const char *buf, uint32_t file_size, int *buf_offset,
								int *databuf, int datasize) {
	int ret = NO_ERR;
	int index_count = 0;
	int tmp_buf_size = 0;
	if(!buf || !databuf || !buf_offset) {
		TS_LOG_ERR("%s, buf invalid\n", __func__);
		return -EINVAL;
	}
	tmp_buf_size = *buf_offset;
	for(index_count = 0; index_count < datasize; ){
		ret = wacom_get_one_value(buf,&tmp_buf_size);
		if(ret != WACOM_INVALID_VALUE)
		{
			databuf[index_count] = ret;
			TS_LOG_DEBUG("%s: rawdata = %d, index_count=%d\n", __func__, ret, index_count);
			index_count++;
			ret = NO_ERR;
		}
		else if(*buf_offset >= file_size)
		{
			ret = -EINVAL;
			break;
		}
		else
		{
			tmp_buf_size++;//try next
		}
	}
	*buf_offset = tmp_buf_size;
	return ret;
}

static int wacom_get_mult_item(const char *buf, uint32_t file_size, int *buf_offset,
								int *databuf, int rx_num, int tx_num) {
	int ret = NO_ERR;
	int index_count = 0;
	int tmp_buf_size = 0;
	int datasize = rx_num * tx_num;
	int x = 0, y = 0;
	if(!buf || !databuf || !buf_offset || (datasize > (MAX_TX * MAX_RX)) || (datasize <= 0)) {
		TS_LOG_ERR("%s, buf invalid\n", __func__);
		return -EINVAL;
	}
	tmp_buf_size = *buf_offset;
	for(index_count = 0; index_count < datasize; ){
		ret = wacom_get_one_value(buf,&tmp_buf_size);
		if(ret != WACOM_INVALID_VALUE)
		{
			databuf[y * MAX_RX + x] = ret;
			TS_LOG_DEBUG("%s: rawdata = %d, index_count=%d\n", __func__, ret, index_count);
			index_count++;
			if(index_count % rx_num == 0) {
				x = 0;
				y++;
			}else {
				x++;
			}
			ret = NO_ERR;
		}
		else if(*buf_offset >= file_size)
		{
			ret = -EINVAL;
			break;
		}
		else
		{
			tmp_buf_size++;//try next
		}
	}
	*buf_offset = tmp_buf_size;
	return ret;
}

static int wacom_parse_threshold_file_method(const char *buf, uint32_t file_size)
{
	int retval = 0;
	int case_num = 0;
	int buf_offset = 0;
	struct wacom_i2c *data = wac_data;
	int max_size = MAX_TX * MAX_RX * sizeof(int);

	if(!buf || !data || !(data->m_ini.m_ref)
		|| !(data->m_ini.m_upr) || !(data->m_ini.m_lwr)
		|| !(data->m_ini.m_gap) || !(data->m_ini.m_gav)) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	memset(data->m_ini.m_upr, 0, max_size);
	memset(data->m_ini.m_lwr, 0, max_size);
	memset(data->m_ini.m_gap, 0, max_size);
	memset(data->m_ini.m_gav, 0, max_size);
	memset(data->m_ini.m_ref, 0, max_size);

	for(case_num = 0;case_num < MAX_CASE_CNT; case_num++) {
		switch(case_num){
			case REAL_RX_NUM:
				retval = wacom_get_single_item(buf, file_size, &buf_offset, &(data->m_ini.rx_num), 1);
				if(retval != NO_ERR) {
					TS_LOG_ERR("%s, get real_rx_num failed\n", __func__);
					return retval;
				}
				TS_LOG_INFO("rx num = %d\n", data->m_ini.rx_num);
				continue;
			case REAL_TX_NUM:
				retval = wacom_get_single_item(buf, file_size, &buf_offset, &(data->m_ini.tx_num), 1);
				if(retval != NO_ERR) {
					TS_LOG_ERR("%s, get real_tx_num failed\n", __func__);
					return retval;
				}
				TS_LOG_INFO("tx_num = %d\n", data->m_ini.tx_num);
				continue;
			case TEST_COUNT:
				retval = wacom_get_single_item(buf, file_size, &buf_offset, &(data->m_ini.data_count), 1);
				if(retval != NO_ERR) {
					TS_LOG_ERR("%s, get TEST_COUNT failed\n", __func__);
					return retval;
				}
				TS_LOG_INFO("data_count = %d\n", data->m_ini.data_count);
				continue;
			case ACTUAL_DATA_IGNORE_TOP:
				retval = wacom_get_single_item(buf, file_size, &buf_offset, &(data->m_ini.calth_ignoreline[IGNORE_TOP_ITO_BYTE]), 1);
				if(retval != NO_ERR) {
					TS_LOG_ERR("%s, get ACTUAL_DATA_IGNORE_TOP failed\n", __func__);
					return retval;
				}
				TS_LOG_INFO("calth_ignoreline[IGNORE_TOP_ITO_BYTE] = %d\n", data->m_ini.calth_ignoreline[IGNORE_TOP_ITO_BYTE]);
				continue;
			case ACTUAL_DATA_IGNORE_BOTTOM:
				retval = wacom_get_single_item(buf, file_size, &buf_offset, &(data->m_ini.calth_ignoreline[IGNORE_BOTTOM_ITO_BYTE]), 1);
				if(retval != NO_ERR) {
					TS_LOG_ERR("%s, get ACTUAL_DATA_IGNORE_BOTTOM failed\n", __func__);
					return retval;
				}
				TS_LOG_INFO("calth_ignoreline[IGNORE_BOTTOM_ITO_BYTE] = %d\n", data->m_ini.calth_ignoreline[IGNORE_BOTTOM_ITO_BYTE]);
				continue;
			case ACTUAL_DATA_IGNORE_LEFT:
				retval = wacom_get_single_item(buf, file_size, &buf_offset, &(data->m_ini.calth_ignoreline[IGNORE_LEFT_ITO_BYTE]), 1);
				if(retval != NO_ERR) {
					TS_LOG_ERR("%s, get ACTUAL_DATA_IGNORE_LEFT failed\n", __func__);
					return retval;
				}
				TS_LOG_INFO("calth_ignoreline[IGNORE_LEFT_ITO_BYTE] = %d\n", data->m_ini.calth_ignoreline[IGNORE_LEFT_ITO_BYTE]);
				continue;
			case ACTUAL_DATA_IGNORE_RIGHT:
				retval = wacom_get_single_item(buf, file_size, &buf_offset, &(data->m_ini.calth_ignoreline[IGNORE_RIGHT_ITO_BYTE]), 1);
				if(retval != NO_ERR) {
					TS_LOG_ERR("%s, get ACTUAL_DATA_IGNORE_RIGHT failed\n", __func__);
					return retval;
				}
				TS_LOG_INFO("calth_ignoreline[IGNORE_RIGHT_ITO_BYTE] = %d\n", data->m_ini.calth_ignoreline[IGNORE_RIGHT_ITO_BYTE]);
				continue;
			case ACTUAL_DATA_LIMIT:
				retval = wacom_get_single_item(buf, file_size, &buf_offset, &(data->m_ini.cal_thr), 1);
				if(retval != NO_ERR) {
					TS_LOG_ERR("%s, get ACTUAL_DATA_LIMIT failed\n", __func__);
					return retval;
				}
				TS_LOG_INFO("cal_thr = %d\n", data->m_ini.cal_thr);
				continue;
			case RAW_DATA_LIMIT:
				retval = wacom_get_mult_item(buf, file_size, &buf_offset, data->m_ini.m_upr, data->m_ini.rx_num, data->m_ini.tx_num);
				if(retval != NO_ERR) {
					TS_LOG_ERR("%s, get RAW_DATA_LIMIT failed\n", __func__);
					return retval;
				}
				continue;
			case RAW_DATA_REFERENCE:
				retval = wacom_get_mult_item(buf, file_size, &buf_offset, data->m_ini.m_ref, data->m_ini.rx_num, data->m_ini.tx_num);
				if(retval != NO_ERR) {
					TS_LOG_ERR("%s, get RAW_DATA_REFERENCE failed\n", __func__);
					return retval;
				}
				continue;
			case GAP_HORIZONTAL://rx2rx

				retval = wacom_get_mult_item(buf, file_size, &buf_offset, data->m_ini.m_gap, data->m_ini.rx_num - 1, data->m_ini.tx_num);
				if(retval != NO_ERR) {
					TS_LOG_ERR("%s, get GAP_HORIZONTAL failed\n", __func__);
					return retval;
				}
				continue;
			case GAP_VERTICAL://tx2tx
				retval = wacom_get_mult_item(buf, file_size, &buf_offset, data->m_ini.m_gav, data->m_ini.rx_num, data->m_ini.tx_num - 1);
				if(retval != NO_ERR) {
					TS_LOG_ERR("%s, get GAP_VERTICAL failed\n", __func__);
					return retval;
				}
				continue;
			default:
			{
				break;
			}
		}
	}
	return retval;
}

static int wacom_get_one_value(const char *buf, uint32_t *offset)
{
	int value = WACOM_INVALID_VALUE;
	char tmp_buffer[10] = {0};
	uint32_t count = 0;
	uint32_t tmp_offset = *offset;
	int m=0,n=0;
	int signed_flag = 0;

	if(!buf || !offset) {
		TS_LOG_ERR("%s,param invalid\n", __func__);
		return -EINVAL;
	}
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
		if(buf[tmp_offset] == '-') {
			signed_flag = 1;//is signed
			tmp_offset++;
		}
		for (;;) {
			if ((buf[tmp_offset] >= ASCII_ZERO)
			&& (buf[tmp_offset] <= ASCII_NINE)) {
				tmp_buffer[count++] = buf[tmp_offset] - ASCII_ZERO;
				tmp_offset++;
			} else {
				if (count != 0) {
					value = wacom_ctoi(tmp_buffer,count);
					if(signed_flag == 1) {//is signed
						value = 0 - value;
					}
					/*dev_vdbg(dev, ",%d", value);*/
				} else {
					/* 0 indicates no data available */
					value = WACOM_INVALID_VALUE;
				}
				signed_flag = 0;
				break;
			}
		}
	} else {
	/*do plus outside tmp_offset++;*/
	}

	*offset = tmp_offset;

	return value;
}

static int wacom_ctoi(char *buf, uint32_t count)
{
	int value = 0;
	uint32_t index = 0;
	u32 base_array[] = {1, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9};

	if(!buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	for (index = 0; index < count; index++)
		value += buf[index] * base_array[count - 1 - index];
	return value;
}


