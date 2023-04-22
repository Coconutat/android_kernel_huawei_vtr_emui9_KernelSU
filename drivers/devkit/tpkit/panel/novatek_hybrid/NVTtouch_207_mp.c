/* drivers/input/touchscreen/nt11207/NVTtouch_207_mp_ctrlram.c
 *
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision$
 * $Date$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>
#include <linux/jiffies.h>

#include "NVTtouch_207.h"
#include "NVTtouch_207_mp.h"

#include <../../huawei_ts_kit.h>
#include <linux/regulator/consumer.h>
#include <huawei_platform/log/log_jank.h>
#include "../../huawei_ts_kit_algo.h"
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif

#if NVT_HYBRID_TOUCH_MP
static uint8_t NVT_HYBRID_AIN_RX[NVT_HYBRID_IC_RX_CFG_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 16, 0xFF, 0xFF, 11, 15, 13, 10, 12, 14, 0xFF, 8, 9, 7, 0xFF, 6, 5, 0xFF, 3, 4, 0xFF, 0xFF, 2, 1, 0};
static uint8_t NVT_HYBRID_AIN_TX[NVT_HYBRID_IC_TX_CFG_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 0xFF, 0xFF};
static uint8_t NVT_HYBRID_AIN_RX_Order[NVT_HYBRID_IC_RX_CFG_SIZE] = {0};
static uint8_t NVT_HYBRID_AIN_TX_Order[NVT_HYBRID_IC_TX_CFG_SIZE] = {0};

static uint8_t NVT_Hybrid_RecordResult_FWMutual[40*40] = {0};
static uint8_t NVT_Hybrid_RecordResultShort_RXRX[40] = {0};
static uint8_t NVT_Hybrid_RecordResultOpen[40 * 40] = {0};
static uint8_t NVT_Hybrid_RecordResultPixelRaw[40 * 40] = {0};
static uint8_t NVT_Hybrid_RecordResult_FW_Diff[40 * 40] = {0};
static uint8_t NVT_Hybrid_Rawdata_Buf[40*40] = {0};

static int32_t NVT_Hybrid_TestResult_FWMutual = 0;
static int32_t NVT_Hybrid_TestResult_Short_RXRX = 0;
static int32_t NVT_Hybrid_TestResult_Open = 0;
static int32_t NVT_Hybrid_TestResult_PixelRaw = 0;
static int32_t NVT_Hybrid_TestResult_Noise = 0;
static int32_t NVT_Hybrid_TestResult_FW_Diff = 0;

static int32_t nvt_hybrid_rawdata_fwMutual[40 * 40] = {0}; 
static int32_t nvt_hybrid_rawdata_diff[40 * 40] = {0};
static int32_t nvt_hybrid_rawdata_short_rxrx[40] = {0};
static int32_t nvt_hybrid_rawdata_short_rxrx0[40] = {0};
static int32_t nvt_hybrid_rawdata_short_rxrx1[40] = {0};
static int32_t nvt_hybrid_rawdata_open[40 * 40] = {0};
static int64_t NVT_Hybrid_PixelRawCmRatio[40 * 40] = {0};
static int32_t NVT_Hybrid_PixelRawCmRatioMax[40 * 40] = {0};

static int64_t NVT_Hybrid_StatisticsNum[NVT_Hybrid_MaxStatisticsBuf];
static int64_t NVT_Hybrid_StatisticsSum[NVT_Hybrid_MaxStatisticsBuf];

static struct proc_dir_entry *NVT_HYBRID_proc_selftest_entry = NULL;

extern struct nvt_hybrid_ts_data *nvt_hybrid_ts;
extern char nvt_hybrid_product_id[];
extern void nvt_hybrid_hw_reset(void);
extern int32_t nvt_hybrid_ts_i2c_read(struct i2c_client *client, uint16_t i2c_addr, uint8_t *buf, uint16_t len);
extern int32_t nvt_hybrid_ts_i2c_write(struct i2c_client *client, uint16_t i2c_addr, uint8_t *buf, uint16_t len);
extern void nvt_hybrid_sw_reset_idle(void);
extern void nvt_hybrid_bootloader_reset(void);
extern int32_t nvt_hybrid_check_fw_reset_state(NVT_HYBRID_RST_COMPLETE_STATE check_reset_state);
extern int32_t nvt_hybrid_clear_fw_status(void);
extern int32_t nvt_hybrid_check_fw_status(void);
extern void nvt_hybrid_change_mode(uint8_t mode);
extern int8_t nvt_hybrid_get_fw_info(void);
extern uint8_t nvt_hybrid_get_fw_pipe(void);
extern void nvt_hybrid_read_mdata(uint32_t xdata_addr, uint32_t xdata_btn_addr);
extern void nvt_hybrid_get_mdata(int32_t *buf, uint8_t *m_x_num, uint8_t *m_y_num);


static char mmitest_result[TP_MMI_RESULT_LEN] = {0};/*store mmi test result*/

static void nvt_hybrid_cal_ain_order(void)
{
	uint8_t i = 0;

	for (i = 0; i < NVT_HYBRID_IC_RX_CFG_SIZE; i++) {
		if (NVT_HYBRID_AIN_RX[i] == 0xFF)
			continue;
		NVT_HYBRID_AIN_RX_Order[NVT_HYBRID_AIN_RX[i]] = i;
	}

	for (i = 0; i < NVT_HYBRID_IC_TX_CFG_SIZE; i++) {
		if (NVT_HYBRID_AIN_TX[i] == 0xFF)
			continue;
		NVT_HYBRID_AIN_TX_Order[NVT_HYBRID_AIN_TX[i]] = i;
	}

	TS_LOG_INFO("NVT_HYBRID_AIN_RX_Order:\n");
	for (i = 0; i < NVT_HYBRID_IC_RX_CFG_SIZE; i++)
		printk("%d, ", NVT_HYBRID_AIN_RX_Order[i]);
	printk("\n");

	TS_LOG_INFO("NVT_HYBRID_AIN_TX_Order:\n");
	for (i = 0; i < NVT_HYBRID_IC_TX_CFG_SIZE; i++)
		printk("%d, ", NVT_HYBRID_AIN_TX_Order[i]);
	printk("\n");
}


#define ABS(x)	(((x) < 0) ? -(x) : (x))
/*******************************************************
Description:
	Novatek touchscreen read short test RX-RX raw data
	function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_hybrid_read_short_rxrx(void)
{
	int32_t i = 0;
	uint8_t buf[128] = {0};
	int16_t sh = 0;
	int16_t sh1 = 0;

	TS_LOG_INFO("%s:++\n", __func__);

	if (nvt_hybrid_clear_fw_status())
		return -EAGAIN;

	nvt_hybrid_change_mode(NVT_HYBRID_TEST_MODE_SHORT_RXRX);

	if (nvt_hybrid_check_fw_status())
		return -EAGAIN;

	//---change xdata index---
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0x00;
	nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
	//---read data---
	buf[0] = 0x00;
	nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, NVT_HYBRID_IC_RX_CFG_SIZE * 2 + 1);
	for (i = 0; i < nvt_hybrid_ts->ain_rx_num; i++) {
		nvt_hybrid_rawdata_short_rxrx0[i] = (int16_t)(buf[NVT_HYBRID_AIN_RX_Order[i] * 2 + 1] + 256 * buf[NVT_HYBRID_AIN_RX_Order[i] * 2 + 2]);
	}

	nvt_hybrid_bootloader_reset();
	nvt_hybrid_check_fw_reset_state(NVT_HYBRID_RESET_STATE_INIT);

	if (nvt_hybrid_clear_fw_status())
		return -EAGAIN;

	nvt_hybrid_change_mode(NVT_HYBRID_TEST_MODE_SHORT_RXRX1);

	if (nvt_hybrid_check_fw_status())
		return -EAGAIN;

	//---change xdata index---
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0x00;
		nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
	//---read data---
	buf[0] = 0x00;
	nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, NVT_HYBRID_IC_RX_CFG_SIZE * 2 + 1);
	for (i = 0; i < nvt_hybrid_ts->ain_rx_num; i++) {
		nvt_hybrid_rawdata_short_rxrx1[i] = (int16_t)(buf[NVT_HYBRID_AIN_RX_Order[i] * 2 + 1] + 256 * buf[NVT_HYBRID_AIN_RX_Order[i] * 2 + 2]);
	}

	for (i = 0; i < nvt_hybrid_ts->ain_rx_num; i++) {
		sh = nvt_hybrid_rawdata_short_rxrx0[i];
		sh1 = nvt_hybrid_rawdata_short_rxrx1[i];
		if (ABS(sh) < ABS(sh1)) {
			sh = sh1;
		}
		nvt_hybrid_rawdata_short_rxrx[i] = sh;
	}

	TS_LOG_INFO("%s:--\n", __func__);
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen raw data report upper and lower test function.

return:
	Executive outcomes. 0--passwd. negative--failed.
*******************************************************/
static int32_t nvt_hybrid_rawdata_up_low(int32_t rawdata[], uint8_t RecordResult[], uint8_t x_len, uint8_t y_len, int32_t Upper_Lmt[], int32_t Lower_Lmt[])
{
	int32_t retval = 0;
	int32_t i = 0;
	int32_t j = 0;

	if (!rawdata) {
		TS_LOG_ERR("%s: rawdata is Null\n", __func__);
		retval = -ENOMEM;
		goto out;
	}

	if (!RecordResult) {
		TS_LOG_ERR("%s: RecordResult is Null\n", __func__);
		retval = -ENOMEM;
		goto out;
	}

	if (!Upper_Lmt) {
		TS_LOG_ERR("%s: Upper_Lmt is Null\n", __func__);
		retval = -ENOMEM;
		goto out;
	}

	if (!Lower_Lmt) {
		TS_LOG_ERR("%s: Upper_Lmt is Null\n", __func__);
		retval = -ENOMEM;
		goto out;
	}

	//---Check Lower & Upper Limit---
	for (j = 0 ; j < y_len ; j++) {
		for (i = 0 ; i < x_len ; i++) {
			if(rawdata[j * x_len + i] > Upper_Lmt[j * x_len + i]) {
				RecordResult[j * x_len + i] |= 0x01;
				retval = -1;
			}

			if(rawdata[j * x_len + i] < Lower_Lmt[j * x_len + i]) {
				RecordResult[j * x_len + i] |= 0x02;
				retval = -1;
			}
		}
	}

out:
	//---Return Result---
	return retval;
}


/*******************************************************
Description:
	Novatek touchscreen read open test raw data function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_hybrid_read_open(void)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t iArrayIndex = 0;
	uint8_t buf[128]={0};

	TS_LOG_INFO("%s:++\n", __func__);

	if (nvt_hybrid_clear_fw_status())
		return -EAGAIN;

	nvt_hybrid_change_mode(NVT_HYBRID_TEST_MODE_OPEN);

	if (nvt_hybrid_check_fw_status())
		return -EAGAIN;

	for (i = 0; i < nvt_hybrid_ts->ain_tx_num; i++) {
		//---change xdata index---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0x00 + (uint8_t)(((i * nvt_hybrid_ts->ain_rx_num * 2) & 0xFF00) >> 8);	// 2 bytes for 1 point raw data, get high byte parts
		nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
		//---read data---
		buf[0] = (uint8_t)((i * nvt_hybrid_ts->ain_rx_num * 2) & 0xFF);	// 2 bytes for 1 point raw data
		nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, nvt_hybrid_ts->ain_rx_num * 2 + 1);	// 2 bytes for 1 point raw data, read all rawdata by i2c
		memcpy(NVT_Hybrid_Rawdata_Buf + i * nvt_hybrid_ts->ain_rx_num * 2, buf + 1, nvt_hybrid_ts->ain_rx_num * 2);	// 2 bytes for 1 point raw data, copy rawdata to buffer
	}
	for (i = 0; i < nvt_hybrid_ts->ain_tx_num; i++) {
		for (j = 0; j < nvt_hybrid_ts->ain_rx_num; j++) {
			iArrayIndex = i * nvt_hybrid_ts->ain_rx_num + j;
			// 2 bytes for 1 point raw data, calculate the real rawdata from high byte part (* 256) and low byte part
			nvt_hybrid_rawdata_open[iArrayIndex] = (int16_t)(NVT_Hybrid_Rawdata_Buf[iArrayIndex * 2] + 256 * NVT_Hybrid_Rawdata_Buf[iArrayIndex * 2 + 1]);
		}
	}

	TS_LOG_INFO("%s:--\n", __func__);
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen calculate G Ratio and Normal
	function.

return:
	Executive outcomes. 0---succeed. 1---failed.
*******************************************************/
static int32_t NVT_Hybrid_Test_CaluateGRatioAndNormal(int32_t boundary[], int32_t rawdata[], uint8_t x_len, uint8_t y_len)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t k = 0;
	int64_t tmpValue = 0;
	int64_t MaxSum = 0;
	int32_t SumCnt = 0;
	int32_t MaxNum = 0;
	int32_t MaxIndex = 0;
	int32_t Max = -99999999;
	int32_t Min =  99999999;
	int32_t offset = 0;
	int32_t Data = 0; // double
	int32_t StatisticsStep=0;

	if (!boundary) {
		TS_LOG_ERR("%s: boundary is Null\n", __func__);
		return -ENOMEM;
	}

	if (!rawdata) {
		TS_LOG_ERR("%s: rawdata is Null\n", __func__);
		return -ENOMEM;
	}

	//--------------------------------------------------
	//1. (Testing_CM - Golden_CM ) / Testing_CM
	//--------------------------------------------------
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < x_len; i++) {
			Data = rawdata[j * x_len + i];
			if (Data == 0)
				Data = 1;

			nvt_hybrid_golden_Ratio[j * x_len + i] = Data - boundary[j * x_len + i];
			nvt_hybrid_golden_Ratio[(j * (long)x_len + (long)i)] = div_s64((nvt_hybrid_golden_Ratio[(j * (long)x_len + (long)i)] * 1000), Data); // *1000 before division
		}
	}

	//--------------------------------------------------------
	// 2. Mutual_GoldenRatio*1000
	//--------------------------------------------------------
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < x_len; i++) {
			nvt_hybrid_golden_Ratio[j * x_len + i] *= 1000;
		}
	}

	//--------------------------------------------------------
	// 3. Calculate StatisticsStep
	//--------------------------------------------------------
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < x_len; i++) {
			if (Max < nvt_hybrid_golden_Ratio[j * x_len + i])
				Max = (int32_t)nvt_hybrid_golden_Ratio[j * x_len + i];
			if (Min > nvt_hybrid_golden_Ratio[j * x_len + i])
				Min = (int32_t)nvt_hybrid_golden_Ratio[j * x_len + i];
		}
	}

	offset = 0;
	if (Min < 0) { // add offset to get erery element Positive
		offset = 0 - Min;
		for (j = 0; j < y_len; j++) {
			for (i = 0; i < x_len; i++) {
				nvt_hybrid_golden_Ratio[j * x_len + i] += offset;
			}
		}
		Max += offset;
	}
	StatisticsStep = Max / NVT_Hybrid_MaxStatisticsBuf;
	StatisticsStep += 1;
	if (StatisticsStep < 0) {
		TS_LOG_ERR("%s: FAIL! (StatisticsStep < 0)\n", __func__);
		return 1;
	}

	//--------------------------------------------------------
	// 4. Start Statistics and Average
	//--------------------------------------------------------
	memset(NVT_Hybrid_StatisticsSum, 0, sizeof(int64_t) * NVT_Hybrid_MaxStatisticsBuf);
	memset(NVT_Hybrid_StatisticsNum, 0, sizeof(int64_t) * NVT_Hybrid_MaxStatisticsBuf);
	for (i = 0; i < NVT_Hybrid_MaxStatisticsBuf; i++) {
		NVT_Hybrid_StatisticsSum[i] = 0;
		NVT_Hybrid_StatisticsNum[i] = 0;
	}
	for (j = 0; j < y_len; j++) {
		for(i = 0; i < x_len; i++) {
			tmpValue = nvt_hybrid_golden_Ratio[j * x_len + i];
			tmpValue = div_s64(tmpValue, StatisticsStep);
			NVT_Hybrid_StatisticsNum[tmpValue] += 2;
			NVT_Hybrid_StatisticsSum[tmpValue] += (2 * nvt_hybrid_golden_Ratio[j * x_len + i]);

			if ((tmpValue + 1) < NVT_Hybrid_MaxStatisticsBuf) {
				NVT_Hybrid_StatisticsNum[tmpValue + 1] += 1;
				NVT_Hybrid_StatisticsSum[tmpValue + 1] += nvt_hybrid_golden_Ratio[j * x_len + i];
			}

			if ((tmpValue - 1) >= 0) {
				NVT_Hybrid_StatisticsNum[tmpValue - 1] += 1;
				NVT_Hybrid_StatisticsSum[tmpValue - 1] += nvt_hybrid_golden_Ratio[j * x_len + i];
			}
		}
	}
	//Find out Max Statistics
	MaxNum = 0;
	for (k = 0; k < NVT_Hybrid_MaxStatisticsBuf; k++) {
		if (MaxNum < NVT_Hybrid_StatisticsNum[k]) {
			MaxSum = NVT_Hybrid_StatisticsSum[k];
			MaxNum = NVT_Hybrid_StatisticsNum[k];
			MaxIndex = k;
		}
	}

	//Caluate Statistics Average
	if (MaxSum > 0 ) {
		if (NVT_Hybrid_StatisticsNum[MaxIndex] != 0) {
			tmpValue =(int64_t)div_s64(NVT_Hybrid_StatisticsSum[MaxIndex], NVT_Hybrid_StatisticsNum[MaxIndex]) * 2;
			SumCnt += 2;
		}

		if ((MaxIndex + 1) < (NVT_Hybrid_MaxStatisticsBuf)) {
			if (NVT_Hybrid_StatisticsNum[MaxIndex + 1] != 0) {
				tmpValue += (int64_t)div_s64(NVT_Hybrid_StatisticsSum[(long)MaxIndex + 1], NVT_Hybrid_StatisticsNum[(long)MaxIndex + 1]);
				SumCnt++;
			}
		}

		if ((MaxIndex - 1) >= 0) {
			if (NVT_Hybrid_StatisticsNum[MaxIndex - 1] != 0) {
				tmpValue += (int64_t)div_s64(NVT_Hybrid_StatisticsSum[MaxIndex - 1], NVT_Hybrid_StatisticsNum[MaxIndex - 1]);
				SumCnt++;
			}
		}

		if (SumCnt > 0)
			tmpValue = div_s64(tmpValue, SumCnt);
	} else { // Too Separately
		NVT_Hybrid_StatisticsSum[0] = 0;
		NVT_Hybrid_StatisticsNum[0] = 0;
		for (j = 0; j < y_len; j++) {
			for (i = 0; i < x_len; i++) {
				NVT_Hybrid_StatisticsSum[0] += (int64_t)nvt_hybrid_golden_Ratio[j * x_len + i];
				NVT_Hybrid_StatisticsNum[0]++;
			}
		}
		tmpValue = div_s64(NVT_Hybrid_StatisticsSum[0], NVT_Hybrid_StatisticsNum[0]);
	}
	//----------------------------------------------------------
	//----------------------------------------------------------
	//----------------------------------------------------------
	tmpValue -= offset;
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < x_len; i++) {
			nvt_hybrid_golden_Ratio[j * x_len + i] -= offset;

			nvt_hybrid_golden_Ratio[j * x_len + i] = nvt_hybrid_golden_Ratio[j * x_len + i] - tmpValue;
			nvt_hybrid_golden_Ratio[j *(long) x_len + (long)i] = div_s64(nvt_hybrid_golden_Ratio[j *(long) x_len + (long)i], 1000);
		}
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen raw data test function.

return:
	Executive outcomes. 0---passed. negative---failed.
*******************************************************/
static int32_t NVT_Hybrid_RawDataTest_Sub(int32_t boundary[], int32_t rawdata[], uint8_t RecordResult[],
								uint8_t x_ch, uint8_t y_ch,
								int32_t Tol_P, int32_t Tol_N,
								int32_t Dif_P, int32_t Dif_N,
								int32_t Rawdata_Limit_Postive, int32_t Rawdata_Limit_Negative)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t iArrayIndex = 0;
	int32_t iBoundary = 0;
	int32_t iTolLowBound = 0;
	int32_t iTolHighBound = 0;
	bool isAbsCriteria = false;
	bool isPass = true;

	if (!boundary) {
		TS_LOG_ERR("%s: boundary is Null\n", __func__);
		return -ENOMEM;
	}

	if (!rawdata) {
		TS_LOG_ERR("%s: rawdata is Null\n", __func__);
		return -ENOMEM;
	}

	if (!RecordResult) {
		TS_LOG_ERR("%s: RecordResult is Null\n", __func__);
		return -ENOMEM;
	}

	if ((Rawdata_Limit_Postive != 0) || (Rawdata_Limit_Negative != 0))
		isAbsCriteria = true;

	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			iArrayIndex = j * x_ch + i;
			iBoundary = boundary[iArrayIndex];

			RecordResult[iArrayIndex] = 0x00; // default value for PASS

			if (isAbsCriteria) {
				iTolLowBound = Rawdata_Limit_Negative;
				iTolHighBound = Rawdata_Limit_Postive;
			} else {
				if (iBoundary > 0) {
					iTolLowBound = (iBoundary * (1000 + Tol_N));
					iTolHighBound = (iBoundary * (1000 + Tol_P));
				} else {
					iTolLowBound = (iBoundary * (1000 - Tol_N));
					iTolHighBound = (iBoundary * (1000 - Tol_P));
				}
			}

			if((rawdata[iArrayIndex] * 1000) > iTolHighBound)
				RecordResult[iArrayIndex] |= 0x01;

			if((rawdata[iArrayIndex] * 1000) < iTolLowBound)
				RecordResult[iArrayIndex] |= 0x02;
		}
	}

	if (!isAbsCriteria) {
		NVT_Hybrid_Test_CaluateGRatioAndNormal(boundary, rawdata, x_ch, y_ch);

		for (j = 0; j < y_ch; j++) {
			for (i = 0; i < x_ch; i++) {
				iArrayIndex = j * x_ch + i;

				if (nvt_hybrid_golden_Ratio[iArrayIndex] > Dif_P)
					RecordResult[iArrayIndex] |= 0x04;

				if (nvt_hybrid_golden_Ratio[iArrayIndex] < Dif_N)
					RecordResult[iArrayIndex] |= 0x08;
			}
		}
	}

	//---Check RecordResult---
	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			if (RecordResult[j * x_ch + i] != 0) {
				isPass = false;
				break;
			}
		}
	}

	if( isPass == false) {
		return -1; // FAIL
	} else {
		return 0; // PASS
	}
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))
/*******************************************************
Description:
	Novatek touchscreen raw data test function.

return:
	Executive outcomes. 0---passed. negative---failed.
*******************************************************/
static int32_t NVT_Hybrid_PixelRawTest_Sub(int32_t rawdata[], int64_t PixelRawCmRatio[], int32_t PixelRawCmRatioMax[], uint8_t RecordResult[],
								uint8_t x_ch, uint8_t y_ch, int32_t PixelRaw_Diff)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t iArrayIndex = 0;
	int32_t up = 0;
	int32_t down = 0;
	int32_t right = 0;
	int32_t left = 0;
	int64_t tmpRatio[4] = {0};
	bool isPass = true;

	if (!rawdata) {
		TS_LOG_ERR("%s: rawdata is Null\n", __func__);
		return -ENOMEM;
	}

	if (!PixelRawCmRatio) {
		TS_LOG_ERR("%s: PixelRawCmRatio is Null\n", __func__);
		return -ENOMEM;
	}

	if (!PixelRawCmRatioMax) {
		TS_LOG_ERR("%s: PixelRawCmRatioMax is Null\n", __func__);
		return -ENOMEM;
	}

	if (!RecordResult) {
		TS_LOG_ERR("%s: RecordResult is Null\n", __func__);
		return -ENOMEM;
	}

	TS_LOG_INFO("%s:++\n", __func__);

	// rawdata
	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			iArrayIndex = j * x_ch + i;
			RecordResult[iArrayIndex] = 0x00;
			PixelRawCmRatio[iArrayIndex] = (int64_t)rawdata[iArrayIndex];
		}
	}

	for (j = 0; j < y_ch; j++) {
		if (j == 0) {
			up = 0;
			down = j + 1;
		} else if (j == y_ch - 1) {
			up = j - 1;
			down = y_ch -1;
		} else {
			up = j - 1;
			down = j + 1;
		}
		for (i = 0; i < x_ch; i++) {
			iArrayIndex = j * x_ch + i;

			if (i == 0) {
				left = 0;
				right = i + 1;
			} else if (i == x_ch - 1) {
				left = i - 1;
				right = x_ch - 1;
			} else {
				left = i - 1;
				right = i + 1;
			}

			tmpRatio[0] = ABS(PixelRawCmRatio[up * x_ch + i] - PixelRawCmRatio[iArrayIndex]);
			tmpRatio[1] = ABS(PixelRawCmRatio[down * x_ch + i] - PixelRawCmRatio[iArrayIndex]);
			tmpRatio[2] = ABS(PixelRawCmRatio[j * x_ch + left] - PixelRawCmRatio[iArrayIndex]);
			tmpRatio[3] = ABS(PixelRawCmRatio[j * x_ch + right] - PixelRawCmRatio[iArrayIndex]);
			PixelRawCmRatioMax[iArrayIndex] = (int32_t) MAX(MAX(tmpRatio[0], tmpRatio[1]), MAX(tmpRatio[2], tmpRatio[3]));
		}
	}

	//---Check RecordResult---
	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			iArrayIndex = j * x_ch + i;
			if (PixelRawCmRatioMax[iArrayIndex] > PixelRaw_Diff) {
				RecordResult[iArrayIndex] |= 0x01;
				isPass = false;
				TS_LOG_INFO("%s: break at x_ch=%d, y_ch=%d\n", __func__, i, j);
				break;
			}
		}
	}

	TS_LOG_INFO("%s:--\n", __func__);

	if(isPass == false) {
		return -1; // FAIL
	} else {
		return 0; // PASS
	}

}

/*******************************************************
Description:
	Novatek touchscreen read raw and diff data function.

return:
	n.a.
*******************************************************/
int32_t nvt_hybrid_read_raw_and_diff(int32_t *xdata, int32_t *xdata1)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;

	TS_LOG_INFO("%s:++\n", __func__);

	if (!xdata) {
		TS_LOG_ERR("%s: xdata is Null\n", __func__);
		return -ENOMEM;
	}

	if (!xdata1) {
		TS_LOG_ERR("%s: xdata1 is Null\n", __func__);
		return -ENOMEM;
	}

	if (nvt_hybrid_clear_fw_status()) {
		TS_LOG_ERR("%s: clear_fw_status err\n", __func__);
		return -EAGAIN;
	}

	nvt_hybrid_change_mode(NVT_HYBRID_TEST_MODE_1);

	if (nvt_hybrid_check_fw_status()) {
		TS_LOG_ERR("%s: check_fw_status err\n", __func__);
		return -EAGAIN;
	}

	nvt_hybrid_get_fw_info();

	TS_LOG_INFO("%s: Read Raw start\n", __func__);

	if (nvt_hybrid_get_fw_pipe() == 0)
		nvt_hybrid_read_mdata(NVT_HYBRID_RAW_PIPE0_ADDR, 0);
	else
		nvt_hybrid_read_mdata(NVT_HYBRID_RAW_PIPE1_ADDR, 0);

	nvt_hybrid_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			xdata[y * x_num + x] =  (int16_t)xdata[y * x_num + x];
		}
	}

	TS_LOG_INFO("%s: Read Raw end\n", __func__);
	/* Read RAW End */

	/* Read Noise Start */
	TS_LOG_INFO("%s: Read Noise start\n", __func__);
	
	if (nvt_hybrid_get_fw_pipe() == 0)
		nvt_hybrid_read_mdata(NVT_HYBRID_DIFF_PIPE0_ADDR, NVT_HYBRID_DIFF_BTN_PIPE0_ADDR);
	else
		nvt_hybrid_read_mdata(NVT_HYBRID_DIFF_PIPE1_ADDR, NVT_HYBRID_DIFF_BTN_PIPE1_ADDR);

	nvt_hybrid_get_mdata(xdata1, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			xdata1[y * x_num + x] =  (int16_t)xdata1[y * x_num + x];
		}
	}

	TS_LOG_INFO("%s: Read Noise end\n", __func__);
	nvt_hybrid_change_mode(NVT_HYBRID_NORMAL_MODE);
	/* Read Noise End */
	
	TS_LOG_INFO("%s:--\n", __func__);
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen read raw data function.

return:
	n.a.
*******************************************************/
int32_t nvt_hybrid_read_raw(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;

	TS_LOG_INFO("%s:++\n", __func__);

	if (!xdata) {
		TS_LOG_ERR("%s: xdata is Null\n", __func__);
		return -ENOMEM;
	}

	if (nvt_hybrid_clear_fw_status()) {
		TS_LOG_ERR("%s: clear_fw_status err\n", __func__);
		return -EAGAIN;
	}

	nvt_hybrid_change_mode(NVT_HYBRID_TEST_MODE_1);

	if (nvt_hybrid_check_fw_status()) {
		TS_LOG_ERR("%s: check_fw_status err\n", __func__);
		return -EAGAIN;
	}

	nvt_hybrid_get_fw_info();

	if (nvt_hybrid_get_fw_pipe() == 0)
		nvt_hybrid_read_mdata(NVT_HYBRID_RAW_PIPE0_ADDR, 0);
	else
		nvt_hybrid_read_mdata(NVT_HYBRID_RAW_PIPE1_ADDR, 0);

	nvt_hybrid_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			xdata[y * x_num + x] =  (int16_t)xdata[y * x_num + x];
		}
	}

	nvt_hybrid_change_mode(NVT_HYBRID_NORMAL_MODE);

	TS_LOG_INFO("%s:--\n", __func__);
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen read diff data function.

return:
	n.a.
*******************************************************/
int32_t nvt_hybrid_read_diff(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;

	TS_LOG_INFO("%s:++\n", __func__);

	if (!xdata) {
		TS_LOG_ERR("%s: xdata is Null\n", __func__);
		return -ENOMEM;
	}

	if (nvt_hybrid_clear_fw_status()) {
		TS_LOG_ERR("%s: clear_fw_status err\n", __func__);
		return -EAGAIN;
	}

	nvt_hybrid_change_mode(NVT_HYBRID_TEST_MODE_1);

	if (nvt_hybrid_check_fw_status()) {
		TS_LOG_ERR("%s: check_fw_status err\n", __func__);
		return -EAGAIN;
	}

	nvt_hybrid_get_fw_info();
	
	if (nvt_hybrid_get_fw_pipe() == 0)
		nvt_hybrid_read_mdata(NVT_HYBRID_DIFF_PIPE0_ADDR, NVT_HYBRID_DIFF_BTN_PIPE0_ADDR);
	else
		nvt_hybrid_read_mdata(NVT_HYBRID_DIFF_PIPE1_ADDR, NVT_HYBRID_DIFF_BTN_PIPE1_ADDR);

	nvt_hybrid_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			xdata[y * x_num + x] =  (int16_t)xdata[y * x_num + x];
		}
	}

	TS_LOG_INFO("%s:change mode\n", __func__);
	nvt_hybrid_change_mode(NVT_HYBRID_NORMAL_MODE);
	
	TS_LOG_INFO("%s:--\n", __func__);

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen read noise data function.

return:
	n.a.
*******************************************************/
static int32_t nvt_hybrid_read_noise(void)
{
	int32_t x = 0;
	int32_t y = 0;

	if (nvt_hybrid_read_diff(nvt_hybrid_rawdata_diff)) {
		return 1; // read data failed
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen calculate selftest data for huawei_touchscreen.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int16_t nvt_hybrid_get_avg(int32_t *p, uint8_t x_ch, uint8_t y_ch)
{
	int32_t sum=0;
	int i = 0;

	if (!p) {
		TS_LOG_ERR("%s: p is Null\n", __func__);
		return -ENOMEM;
	}

	for (i = 0 ; i < (x_ch * y_ch) ; i++)
		sum += p[i];

	return (int16_t) (sum / (x_ch * y_ch));
}


static int16_t nvt_hybrid_get_max(int32_t *p, uint8_t x_ch, uint8_t y_ch)
{
	int32_t max=INT_MIN;
	int i = 0;

	if (!p) {
		TS_LOG_ERR("%s: p is Null\n", __func__);
		return -ENOMEM;
	}

	for (i = 0 ; i < (x_ch * y_ch) ; i++)
		max = max > p[i] ? max : p[i];

	return (int16_t) max;
}

static int16_t nvt_hybrid_get_min(int32_t *p, uint8_t x_ch, uint8_t y_ch)
{
	int32_t min=INT_MAX;
	int i = 0;

	if (!p) {
		TS_LOG_ERR("%s: p is Null\n", __func__);
		return -ENOMEM;
	}

	for (i = 0 ; i < (x_ch * y_ch) ; i++)
		min = min < p[i] ? min : p[i];

	return (int16_t) min;
}

/*******************************************************
Description:
	Novatek touchscreen print mmi data result function.

return:
	n.a.
*******************************************************/
static int nvt_hybrid_mmi_add_static_data(void)
{
 	int32_t i = 0;

	TS_LOG_ERR("%s enter\n", __func__);

	//1: RawData:	nvt_hybrid_rawdata_fwMutual
	i=  strlen(mmitest_result);
	if  (i >= TP_MMI_RESULT_LEN) {
		return -EINVAL;
	}

	snprintf((mmitest_result + i), (TP_MMI_RESULT_LEN - i), "[%d,%d,%d]",
			nvt_hybrid_get_avg(nvt_hybrid_rawdata_fwMutual, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_hybrid_get_max(nvt_hybrid_rawdata_fwMutual, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_hybrid_get_min(nvt_hybrid_rawdata_fwMutual, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num));

	//2: TRx Delta(Pixel Raw):	NVT_Hybrid_PixelRawCmRatioMax
	i=  strlen(mmitest_result);
	if  (i >= TP_MMI_RESULT_LEN) {
		return -EINVAL;
	}

	snprintf((mmitest_result + i), (TP_MMI_RESULT_LEN - i), "[%d,%d,%d]",
			nvt_hybrid_get_avg(NVT_Hybrid_PixelRawCmRatioMax, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_hybrid_get_max(NVT_Hybrid_PixelRawCmRatioMax, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_hybrid_get_min(NVT_Hybrid_PixelRawCmRatioMax, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num));

	//3: Noise(Diff):	nvt_hybrid_rawdata_diff
	i=  strlen(mmitest_result);
	if  (i >= TP_MMI_RESULT_LEN) {
		return -EINVAL;
	}

	snprintf((mmitest_result + i), (TP_MMI_RESULT_LEN - i), "[%d,%d,%d]",
			nvt_hybrid_get_avg(nvt_hybrid_rawdata_diff, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_hybrid_get_max(nvt_hybrid_rawdata_diff, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_hybrid_get_min(nvt_hybrid_rawdata_diff, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num));

	//4-1: Short:		nvt_hybrid_rawdata_short_rxrx
	i=  strlen(mmitest_result);
	if  (i >= TP_MMI_RESULT_LEN) {
		return -EINVAL;
	}

	snprintf((mmitest_result + i), (TP_MMI_RESULT_LEN - i), "[%d,%d,%d]",
			nvt_hybrid_get_avg(nvt_hybrid_rawdata_short_rxrx, nvt_hybrid_ts->ain_rx_num, 1),
			nvt_hybrid_get_max(nvt_hybrid_rawdata_short_rxrx, nvt_hybrid_ts->ain_rx_num, 1),
			nvt_hybrid_get_min(nvt_hybrid_rawdata_short_rxrx, nvt_hybrid_ts->ain_rx_num, 1));

	//4-2: Open:		nvt_hybrid_rawdata_open
	i= strlen(mmitest_result);
	if  (i >= TP_MMI_RESULT_LEN) {
		return -EINVAL;
	}

	snprintf((mmitest_result + i), (TP_MMI_RESULT_LEN - i), "[%d,%d,%d]",
			nvt_hybrid_get_avg(nvt_hybrid_rawdata_open, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_hybrid_get_max(nvt_hybrid_rawdata_open, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_hybrid_get_min(nvt_hybrid_rawdata_open, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num));

	return 0;
	
}

/*******************************************************
Description:
	Novatek touchscreen print all raw data for debugging selftest
	function.

return:
	n.a.
*******************************************************/
static void nvt_hybrid_print_raw_data(uint32_t *xdata, uint32_t x_num, uint32_t y_num) {
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;

	//---use printk to make sure log is the table like format for debug---
	printk("%s:\n", __func__);
	for (y = 0 ; y < y_num ; y++) {
		for (x = 0 ; x < x_num ; x++) {
			iArrayIndex =y* x_num + x;
			printk("%5d, ", xdata[iArrayIndex]);
		}
		printk("\n");
	}
	printk("\n");
}

/*******************************************************
Description:
	Novatek touchscreen self-test criteria print
	function.

return:
	n.a.
*******************************************************/
static void nvt_hybrid_print_criteria(void)
{
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;

	TS_LOG_INFO("%s:++\n", __func__);

	//---NVT_Hybrid_Tolerance_Mutual---
	printk("NVT_Hybrid_Tolerance_Postive_Mutual: %d\n", NVT_Hybrid_Tolerance_Postive_Mutual);
	printk("NVT_Hybrid_Tolerance_Negative_Mutual: %d\n", NVT_Hybrid_Tolerance_Negative_Mutual);

	//---NVT_Hybrid_DiffLimitG_Mutual---
	printk("NVT_Hybrid_DiffLimitG_Postive_Mutual: %d\n", NVT_Hybrid_DiffLimitG_Postive_Mutual);
	printk("NVT_Hybrid_DiffLimitG_Negative_Mutual: %d\n", NVT_Hybrid_DiffLimitG_Negative_Mutual);

	//---NVT_Hybrid_PixelRaw_Diff---
	printk("NVT_Hybrid_PixelRaw_Diff: %d\n", NVT_Hybrid_PixelRaw_Diff);

	//---NVT_Hybrid_Rawdata_Limit_Short_RXRX---
	printk("NVT_Hybrid_Rawdata_Limit_Postive_Short_RXRX: %d\n", NVT_Hybrid_Rawdata_Limit_Postive_Short_RXRX);
	printk("NVT_Hybrid_Rawdata_Limit_Negative_Short_RXRX: %d\n", NVT_Hybrid_Rawdata_Limit_Negative_Short_RXRX);	

	//---NVT_Hybrid_Lmt_FW_Diff---
	printk("NVT_Hybrid_Lmt_FW_Diff_P: %d\n", NVT_Hybrid_Lmt_FW_Diff_P);
	printk("NVT_Hybrid_Lmt_FW_Diff_N: %d\n", NVT_Hybrid_Lmt_FW_Diff_N);
	
	//---NVT_Hybrid_Lmt_FW_Rawdata---
	printk("NVT_Hybrid_Lmt_FW_Rawdata_P:\n");
	for (y = 0; y < nvt_hybrid_ts->ain_tx_num; y++) {
		for (x = 0; x < nvt_hybrid_ts->ain_rx_num; x++) {
			iArrayIndex = y * nvt_hybrid_ts->ain_rx_num + x;
			printk("%5d ", NVT_Hybrid_Lmt_FW_Rawdata_P[iArrayIndex]);
		}
		printk("\n");
	}
	printk("NVT_Hybrid_Lmt_FW_Rawdata_N:\n");
	for (y = 0; y < nvt_hybrid_ts->ain_tx_num; y++) {
		for (x = 0; x < nvt_hybrid_ts->ain_rx_num; x++) {
			iArrayIndex = y * nvt_hybrid_ts->ain_rx_num + x;
			printk("%5d ", NVT_Hybrid_Lmt_FW_Rawdata_N[iArrayIndex]);
		}
		printk("\n");
	}

	//---NVT_Hybrid_BoundaryOpen---
	printk("NVT_Hybrid_BoundaryOpen:\n");
	for (y = 0; y < nvt_hybrid_ts->ain_tx_num; y++) {
		for (x = 0; x < nvt_hybrid_ts->ain_rx_num; x++) {
			iArrayIndex = y * nvt_hybrid_ts->ain_rx_num + x;
			printk("%5d ", NVT_Hybrid_BoundaryOpen[iArrayIndex]);
		}
		printk("\n");
	}

	TS_LOG_INFO("%s:--\n", __func__);
}

/*******************************************************
Description:
	Novatek touchscreen selftest function for huawei_touchscreen.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
#define NVT_HYBRID_TP_TEST_FAILED_REASON_LEN 20
static char selftest_failed_reason[NVT_HYBRID_TP_TEST_FAILED_REASON_LEN] = { "-software_reason" };
int32_t nvt_hybrid_selftest(struct ts_rawdata_info *info)
{
	unsigned long timer_start=jiffies, timer_end=0;
	uint8_t buf[2] = {0};
	NVT_Hybrid_TestResult_FWMutual =0;
	NVT_Hybrid_TestResult_Short_RXRX = 0;
	NVT_Hybrid_TestResult_Open = 0;
	NVT_Hybrid_TestResult_PixelRaw = 0;
	NVT_Hybrid_TestResult_Noise = 0;
	NVT_Hybrid_TestResult_FW_Diff = 0;
	char test_0_result[4]={0};
	char test_1_result[4]={0};
	char test_2_result[4]={0};
	char test_3_result[4]={0};
	char test_4_result[4]={0};

	nvt_hybrid_ts->sensor_testing = true;

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		return -ENOMEM;
	}

	//---print criteria, Taylor 20161115-----
	if(nvt_hybrid_ts->print_criteria == true) {
		nvt_hybrid_print_criteria();
		nvt_hybrid_ts->print_criteria = false;
	}

	//---Test I2C Transfer---
	buf[0] = 0x00;
	if (nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2) < 0) {
		TS_LOG_ERR("%s: I2C READ FAIL!", __func__);
		strcpy(test_0_result, "0F-");
		//strcpy(test_1_result, "1F-");
		//strcpy(test_2_result, "2F-");
		//strcpy(test_3_result, "3F-");
		//strcpy(test_4_result, "4F");
		goto err_nvt_i2c_read;
	} else {
		strcpy(test_0_result, "0P-");
	}
	
	nvt_hybrid_cal_ain_order();

	if(mutex_lock_interruptible(&nvt_hybrid_ts->mp_mutex)) {
		TS_LOG_ERR("%s: mutex_lock_interruptible FAIL!", __func__);
		//strcpy(test_0_result, "0F-");
		//strcpy(test_1_result, "1F-");
		//strcpy(test_2_result, "2F-");
		//strcpy(test_3_result, "3F-");
		//strcpy(test_4_result, "4F");
		goto err_mutex_lock_interruptible;
	}

	/*
	 * FW Rawdata & Noise Test
	 */
	if (nvt_hybrid_read_raw_and_diff(nvt_hybrid_rawdata_fwMutual, nvt_hybrid_rawdata_diff) != 0) {
		NVT_Hybrid_TestResult_FWMutual = 1;	// 1: ERROR
		NVT_Hybrid_TestResult_Noise = 1;
		NVT_Hybrid_TestResult_FW_Diff = 1;
		TS_LOG_ERR("%s: nvt_hybrid_read_raw_and_diff ERROR!\n", __func__);
		TS_LOG_ERR("%s: NVT_Hybrid_TestResult_FWMutual=%d, NVT_Hybrid_TestResult_Noise=%d\n", __func__, NVT_Hybrid_TestResult_FWMutual, NVT_Hybrid_TestResult_Noise);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		//Raw Check
		NVT_Hybrid_TestResult_FWMutual = nvt_hybrid_rawdata_up_low(nvt_hybrid_rawdata_fwMutual, NVT_Hybrid_RecordResult_FWMutual, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num, 
													NVT_Hybrid_Lmt_FW_Rawdata_P, NVT_Hybrid_Lmt_FW_Rawdata_N);
		if (NVT_Hybrid_TestResult_FWMutual == -1){
			TS_LOG_ERR("%s: FW RAWDATA TEST FAIL! NVT_Hybrid_TestResult_FWMutual=%d\n", __func__, NVT_Hybrid_TestResult_FWMutual);
			nvt_hybrid_print_raw_data(NVT_Hybrid_RecordResult_FWMutual, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num);
		} else {
			TS_LOG_INFO("%s: FW RAWDATA TEST PASS! NVT_Hybrid_TestResult_FWMutual=%d\n", __func__, NVT_Hybrid_TestResult_FWMutual);
		}
		//Noise Check
		NVT_Hybrid_TestResult_FW_Diff = NVT_Hybrid_RawDataTest_Sub(NVT_Hybrid_BoundaryDiff, nvt_hybrid_rawdata_diff, NVT_Hybrid_RecordResult_FW_Diff,
									nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num,
									NVT_HYBRID_NONE_Criteria, NVT_HYBRID_NONE_Criteria,
									NVT_HYBRID_NONE_Criteria, NVT_HYBRID_NONE_Criteria,
									NVT_Hybrid_Lmt_FW_Diff_P, NVT_Hybrid_Lmt_FW_Diff_N);
		if (NVT_Hybrid_TestResult_FW_Diff == -1) {
			NVT_Hybrid_TestResult_Noise = -1;
			TS_LOG_ERR("%s: NOISE TEST FAIL! NVT_Hybrid_TestResult_Noise=%d\n", __func__, NVT_Hybrid_TestResult_Noise);
			nvt_hybrid_print_raw_data(nvt_hybrid_rawdata_diff, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num);
		} else {
			NVT_Hybrid_TestResult_Noise = 0;
			TS_LOG_INFO("%s: NOISE TEST PASS! NVT_Hybrid_TestResult_Noise=%d\n", __func__, NVT_Hybrid_TestResult_Noise);
		}
	}
	
	//--- Result for FW Rawdata---
	if (NVT_Hybrid_TestResult_FWMutual != 0) {
		strcpy(test_1_result, "1F-");
	} else {
		strcpy(test_1_result, "1P-");
	}

	//--- Result for Noise Test---
	if (NVT_Hybrid_TestResult_Noise != 0) {
		strcpy(test_3_result, "3F-");
	} else {
		strcpy(test_3_result, "3P-");
	}

	nvt_hybrid_bootloader_reset();
	if (nvt_hybrid_check_fw_reset_state(NVT_HYBRID_RESET_STATE_INIT)) {
		TS_LOG_ERR("%s: Check fw reset state error!\n", __func__);
		goto err_nvt_check_fw_reset_state;
	}

	/*
	 * Short Test RX-RX
	 */
	if (nvt_hybrid_read_short_rxrx() != 0) {
		NVT_Hybrid_TestResult_Short_RXRX = 1; // 1:ERROR
		TS_LOG_ERR("%s: nvt_hybrid_read_short_rxrx ERROR! TestResult_Short = %d\n", __func__, NVT_Hybrid_TestResult_Short_RXRX);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		//---Self Test Check --- // 0:PASS, -1:FAIL
		NVT_Hybrid_TestResult_Short_RXRX = NVT_Hybrid_RawDataTest_Sub(NVT_Hybrid_BoundaryShort_RXRX, nvt_hybrid_rawdata_short_rxrx, NVT_Hybrid_RecordResultShort_RXRX,
												nvt_hybrid_ts->ain_rx_num, 1,
												NVT_HYBRID_NONE_Criteria, NVT_HYBRID_NONE_Criteria,
												NVT_HYBRID_NONE_Criteria, NVT_HYBRID_NONE_Criteria,
												NVT_Hybrid_Rawdata_Limit_Postive_Short_RXRX, NVT_Hybrid_Rawdata_Limit_Negative_Short_RXRX);
		if (NVT_Hybrid_TestResult_Short_RXRX == -1){
			TS_LOG_ERR("%s: SHORT RXRX TEST FAIL! NVT_Hybrid_TestResult_Short_RXRX = %d\n", __func__, NVT_Hybrid_TestResult_Short_RXRX);
			nvt_hybrid_print_raw_data(nvt_hybrid_rawdata_short_rxrx, nvt_hybrid_ts->ain_rx_num, 1);
		} else {
			TS_LOG_INFO("%s: SHORT RXRX TEST PASS! NVT_Hybrid_TestResult_Short_RXRX = %d\n", __func__, NVT_Hybrid_TestResult_Short_RXRX);
		}
	}

	nvt_hybrid_bootloader_reset();
	if (nvt_hybrid_check_fw_reset_state(NVT_HYBRID_RESET_STATE_INIT)) {
		TS_LOG_ERR("%s: Check fw reset state error!\n", __func__);
		goto err_nvt_check_fw_reset_state;
	}

	/*
	 * Open Test---
	 */
	if (nvt_hybrid_read_open() != 0) {
		NVT_Hybrid_TestResult_Open = 1;	// 1:ERROR
		TS_LOG_ERR("%s: nvt_hybrid_read_open ERROR! NVT_Hybrid_TestResult_Open=%d\n", __func__, NVT_Hybrid_TestResult_Open);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		//---Self Test Check --- // 0:PASS, -1:FAIL
		NVT_Hybrid_TestResult_Open = NVT_Hybrid_RawDataTest_Sub(NVT_Hybrid_BoundaryOpen, nvt_hybrid_rawdata_open, NVT_Hybrid_RecordResultOpen,
											nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num,
											NVT_Hybrid_Tolerance_Postive_Mutual, NVT_Hybrid_Tolerance_Negative_Mutual,
											NVT_Hybrid_DiffLimitG_Postive_Mutual, NVT_Hybrid_DiffLimitG_Negative_Mutual,
											NVT_HYBRID_NONE_Criteria, NVT_HYBRID_NONE_Criteria);
		if (NVT_Hybrid_TestResult_Open == -1){
			TS_LOG_ERR("%s: OPEN TEST FAIL! NVT_Hybrid_TestResult_Open=%d\n", __func__, NVT_Hybrid_TestResult_Open);
			nvt_hybrid_print_raw_data(nvt_hybrid_rawdata_open, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num);
		} else {
			TS_LOG_INFO("%s: OPEN TEST PASS! NVT_Hybrid_TestResult_Open=%d\n", __func__, NVT_Hybrid_TestResult_Open);
		}
	}

	//--- Result for Open & Short Test---
	if ((NVT_Hybrid_TestResult_Short_RXRX != 0) || (NVT_Hybrid_TestResult_Open != 0)) {
		strncat(test_4_result, "4F", (strlen("4F")+1));
	} else {
		strncat(test_4_result, "4P", (strlen("4P")+1));
	}

	/*	
	 * PixelRaw Test
	 */
	if (NVT_Hybrid_TestResult_Open == 1) {
		NVT_Hybrid_TestResult_PixelRaw = 1;
	} else {
		NVT_Hybrid_TestResult_PixelRaw = NVT_Hybrid_PixelRawTest_Sub(nvt_hybrid_rawdata_open, NVT_Hybrid_PixelRawCmRatio, NVT_Hybrid_PixelRawCmRatioMax, NVT_Hybrid_RecordResultPixelRaw,
												nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num, NVT_Hybrid_PixelRaw_Diff);
		if(NVT_Hybrid_TestResult_PixelRaw == -1) {
			TS_LOG_ERR("%s: Pixel Raw TEST FAIL! NVT_Hybrid_TestResult_PixelRaw=%d\n", __func__, NVT_Hybrid_TestResult_PixelRaw);
			nvt_hybrid_print_raw_data(NVT_Hybrid_PixelRawCmRatioMax, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num);
		}
	}

	if (NVT_Hybrid_TestResult_PixelRaw != 0) {
		strncat(test_2_result, "2F-", (strlen("2F-")+1));
	} else {
		strncat(test_2_result, "2P-", (strlen("2P-")+1));
	}

err_nvt_check_fw_reset_state:
	//---Reset IC---
	nvt_hybrid_hw_reset();
	nvt_hybrid_bootloader_reset();
	nvt_hybrid_check_fw_reset_state(NVT_HYBRID_RESET_STATE_INIT);

	mutex_unlock(&nvt_hybrid_ts->mp_mutex);

	//---Copy Data to info->buff---
	info->buff[0] = nvt_hybrid_ts->ain_rx_num;
	info->buff[1] = nvt_hybrid_ts->ain_tx_num;
	//info->used_size = nvt_hybrid_ts->ain_rx_num * nvt_hybrid_ts->ain_tx_num * 2 + 2;
	info->used_size = (nvt_hybrid_ts->ain_rx_num * nvt_hybrid_ts->ain_tx_num * 3) + (nvt_hybrid_ts->ain_rx_num  * 2) + 2;	// (Rawdata+Noise+Open) + (Short) + 2

	//one data use 4 buffer data size. the memory size will *4
	//+2 is use for buff[0] and buff[1] used
	memcpy(&info->buff[2], nvt_hybrid_rawdata_fwMutual, (nvt_hybrid_ts->ain_rx_num*nvt_hybrid_ts->ain_tx_num*4));	//Raw data
	memcpy(&info->buff[nvt_hybrid_ts->ain_rx_num * nvt_hybrid_ts->ain_tx_num + 2], nvt_hybrid_rawdata_diff, (nvt_hybrid_ts->ain_rx_num*nvt_hybrid_ts->ain_tx_num*4));	//Diff data
	memcpy(&info->buff[nvt_hybrid_ts->ain_rx_num * nvt_hybrid_ts->ain_tx_num * 2 + 2], nvt_hybrid_rawdata_short_rxrx0, (nvt_hybrid_ts->ain_rx_num*4));	//Short RXRX data
	memcpy(&info->buff[nvt_hybrid_ts->ain_rx_num * nvt_hybrid_ts->ain_tx_num * 2 + nvt_hybrid_ts->ain_rx_num + 2], nvt_hybrid_rawdata_short_rxrx1, (nvt_hybrid_ts->ain_rx_num*4));	//Short RXRX1 data
	memcpy(&info->buff[nvt_hybrid_ts->ain_rx_num * nvt_hybrid_ts->ain_tx_num * 2 + nvt_hybrid_ts->ain_rx_num * 2 + 2], nvt_hybrid_rawdata_open, (nvt_hybrid_ts->ain_rx_num*nvt_hybrid_ts->ain_tx_num*4));	//Open data

err_mutex_lock_interruptible:
err_nvt_i2c_read:
	//---Check Fail Reason---
	if((NVT_Hybrid_TestResult_Short_RXRX == -1)
		|| (NVT_Hybrid_TestResult_Open == -1)
		|| (NVT_Hybrid_TestResult_PixelRaw == -1)
		|| (NVT_Hybrid_TestResult_Noise ==-1) 
		|| (NVT_Hybrid_TestResult_FW_Diff == -1)
		|| (NVT_Hybrid_TestResult_FWMutual == -1))
		strncpy(selftest_failed_reason, "-panel_reason", NVT_HYBRID_TP_TEST_FAILED_REASON_LEN);

	//---String Copy---
	memset(mmitest_result, 0, sizeof(mmitest_result));
	strncat(mmitest_result, test_0_result, strlen(test_0_result));
	strncat(mmitest_result, test_1_result, strlen(test_1_result));
	strncat(mmitest_result, test_2_result, strlen(test_2_result));
	strncat(mmitest_result, test_3_result, strlen(test_3_result));
	strncat(mmitest_result, test_4_result, strlen(test_4_result));
	nvt_hybrid_mmi_add_static_data();
	strncat(mmitest_result, ";", strlen(";"));

	if (0 == strlen(mmitest_result) || strstr(mmitest_result, "F")) {
		strncat(mmitest_result, selftest_failed_reason, strlen(selftest_failed_reason));
	}

	strncat(mmitest_result, "-novatek_", strlen("-novatek_"));
	strncat(mmitest_result, nvt_hybrid_product_id, NVT_HYBRID_PROJECT_ID_LEN); 

	//---Copy String to Result---
	memcpy(info->result, mmitest_result, strlen(mmitest_result));
	//TODO: print result in kmsg

	nvt_hybrid_ts->sensor_testing = false;
	timer_end = jiffies;
	TS_LOG_INFO("%s: self test time:%d\n", __func__, jiffies_to_msecs(timer_end-timer_start));

	return NO_ERR;
}
#endif
