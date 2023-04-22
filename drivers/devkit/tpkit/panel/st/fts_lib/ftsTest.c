/*

 **************************************************************************
 **                        STMicroelectronics 		                **
 **************************************************************************
 **                        marco.cali@st.com				**
 **************************************************************************
 *                                                                        *
 *               	FTS API for MP test				 *
 *                                                                        *
 **************************************************************************
 **************************************************************************

 */

#include "ftsCrossCompile.h"
#include "ftsCompensation.h"
#include "ftsError.h"
#include "ftsFrame.h"
#include "ftsHardware.h"
#include "ftsIO.h"
#include "ftsSoftware.h"
#include "ftsTest.h"
#include "ftsTime.h"
#include "ftsTool.h"
#include "../fts.h"

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <stdarg.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/serio.h>
#include <linux/time.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/power_supply.h>
#include <linux/firmware.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>


int check_MutualRawResGap(void);

static int check_gap_limit(short* data, int row, int column, int threshold)
{
	int i;
	int j;
	int value;
	int failed = false;

	/* colume check */
	for (i = 0; i < row; i++) {
		for (j = 1; j < column; j++) {
			value = abs(data[i * column + j] - data[i * column + (j - 1)]);
			if (value > threshold) {
				TS_LOG_ERR("%s:column gap[%d][%d]=%d out range(%d)\n",
					__func__, i, j, threshold,data[i * column + (j - 1)]);
				failed = true;
			}
		}
	}

	/* colume check */
	for (i = 1; i < row; i++) {
		for (j = 0; j < column; j++) {
			value = abs(data[i * column + j] - data[(i - 1) * column + j]);
			if(value > threshold) {
				TS_LOG_ERR("@@%s:raw gap[%d][%d]=%d out range(%d)\n",
					__func__, i, j, threshold, data[(i - 1) * column + j]);
				failed = true;
			}
		}
	}

	return failed;
}

int computeAdjHoriz(short* data, int row, int column, u8** result) {
	int i, j;
	int size = row * (column - 1);

	if (column < 2) {
		TS_LOG_ERR("%s computeAdjHoriz: ERROR %02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	*result = (u8 *) kmalloc(size * sizeof (u8), GFP_KERNEL);
	if (*result == NULL) {
		TS_LOG_ERR("%s computeAdjHoriz: ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	for (i = 0; i < row; i++) {
		for (j = 1; j < column; j++) {
			*(*result + (i * (column - 1) + (j - 1))) = abs(data[i * column + j] - data[i * column + (j - 1)]);
		}
	}

	return OK;

}

int computeAdjHorizTotal(short* data, int row, int column, u16** result) {
	int i, j;
	int size = row * (column - 1);

	if (column < 2) {
		TS_LOG_ERR("%s computeAdjHorizTotal: ERROR %02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	*result = (u16 *) kmalloc(size * sizeof (u16), GFP_KERNEL);
	if (*result == NULL) {
		TS_LOG_ERR("%s computeAdjHorizTotal: ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	for (i = 0; i < row; i++) {
		for (j = 1; j < column; j++) {
			*(*result + (i * (column - 1) + (j - 1))) = abs(data[i * column + j] - data[i * column + (j - 1)]);
		}
	}

	return OK;

}

int computeAdjVert(short* data, int row, int column, u8**result) {
	int i, j;
	int size = (row - 1)*(column);

	if (row < 2) {
		TS_LOG_ERR("%s computeAdjVert: ERROR %02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	*result = (u8 *) kmalloc(size * sizeof (u8), GFP_KERNEL);
	if (*result == NULL) {
		TS_LOG_ERR("%s computeAdjVert: ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	for (i = 1; i < row; i++) {
		for (j = 0; j < column; j++) {
			*(*result + ((i - 1) * column + j)) = abs(data[i * column + j] - data[(i - 1) * column + j]);
		}
	}

	return OK;
}

int computeAdjVertTotal(short* data, int row, int column, u16**result) {
	int i, j;
	int size = (row - 1)*(column);

	if (row < 2) {
		TS_LOG_ERR("%s computeAdjVertTotal: ERROR %02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	*result = (u16 *) kmalloc(size * sizeof (u16), GFP_KERNEL);
	if (*result == NULL) {
		TS_LOG_ERR("%s computeAdjVertTotal: ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	for (i = 1; i < row; i++) {
		for (j = 0; j < column; j++) {
			*(*result + ((i - 1) * column + j)) = abs(data[i * column + j] - data[(i - 1) * column + j]);
		}
	}

	return OK;
}

int computeTotal(short* data, u8 main, int row, int column, int m, int n, u16**result) {
	int i, j;
	int size = (row)*(column);

	*result = (u16 *) kmalloc(size * sizeof (u16), GFP_KERNEL);
	if (*result == NULL) {
		TS_LOG_ERR("%s computeTotal : ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			*(*result + (i * column + j)) = m * main + n * data[i * column + j];
		}
	}

	return OK;
}

int checkLimitsMinMax(short *data, int row, int column, int min, int max) {
	int i, j;
	int count = 0;

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i * column + j] < min || data[i * column + j] > max) {
				TS_LOG_ERR("%s checkLimitsMinMax: Node[%d,%d] = %d exceed limit [%d, %d]\n", __func__, i, j, data[i * column + j], min, max);
				count++;
			}
		}
	}

	return count; //if count is 0 = OK, test completed successfully
}

int checkLimitsGap(short *data, int row, int column, int threshold) {
	int i, j;
	int min_node;
	int max_node;

	if (row == 0 || column == 0) {
		TS_LOG_ERR("%s checkLimitsGap: invalid number of rows = %d or columns = %d  ERROR %02x\n",
			__func__, row, column, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	min_node = data[0];
	max_node = data[0];

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i * column + j] < min_node) {
				min_node = data[i * column + j];
			} else {
				if (data[i * column + j] > max_node)
					max_node = data[i * column + j];
			}
		}
	}

	if (max_node - min_node > threshold) {
		TS_LOG_ERR("%s checkLimitsGap: GAP = %d exceed limit  %d\n", __func__, max_node - min_node, threshold);
		return ERROR_TEST_CHECK_FAIL;
	} else {
		return OK;
	}

}

int checkLimitsMap(short *data, int row, int column, int *min, int *max) {
	int i, j;
	int count = 0;

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i * column + j] < min[i * column + j] || data[i * column + j] > max[i * column + j]) {
				TS_LOG_ERR("%s checkLimitsMap: Node[%d,%d] = %d exceed limit [%d, %d]\n", __func__, i, j, data[i * column + j], min[i * column + j], max[i * column + j]);
				count++;
			}
		}
	}

	return count; //if count is 0 = OK, test completed successfully
}

int checkLimitsMapTotal(short *data, int row, int column, int *min, int *max) {
	int i, j;
	int count = 0;

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i * column + j] < min[i * column + j] || data[i * column + j] > max[i * column + j]) {
				TS_LOG_ERR("%s checkLimitsMapTotal: Node[%d,%d] = %d exceed limit [%d, %d]\n", __func__, i, j, data[i * column + j], min[i * column + j], max[i * column + j]);
				count++;
			}
		}
	}

	return count; //if count is 0 = OK, test completed successfully
}

int checkLimitsMapAdj(short *data, int row, int column, int *max) {
	int i, j;
	int count = 0;

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i * column + j] > max[i * column + j]) {
				TS_LOG_ERR("%s checkLimitsMapAdj: Node[%d,%d] = %d exceed limit > %d\n", __func__, i, j, data[i * column + j], max[i * column + j]);
				count++;
			}
		}
	}

	return count; //if count is 0 = OK, test completed successfully
}

int checkLimitsMapAdjTotal(short *data, int row, int column, int *max) {
	int i, j;
	int count = 0;

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i * column + j] > max[i * column + j]) {
				TS_LOG_ERR("%s checkLimitsMapAdjTotal: Node[%d,%d] = %d exceed limit > %d\n", __func__, i, j, data[i * column + j], max[i * column + j]);
				count++;
			}
		}
	}

	return count; //if count is 0 = OK, test completed successfully
}

int production_test_ito(TestResult *result) {
	int res = OK;
	u8 cmd;
	u8 readData[FIFO_EVENT_SIZE];
	int eventToSearch[2] = {EVENTID_ERROR_EVENT, EVENT_TYPE_ITO}; //look for ito event

	if(result != NULL)
		result->I2c_Check =false;
	res = fts_system_reset();
	if (res < 0) {
		TS_LOG_ERR("%s production_test_ito: ERROR %02X\n", __func__, ERROR_PROD_TEST_ITO);
		return (res | ERROR_PROD_TEST_ITO);
	}

	cmd = FTS_CMD_ITO_CHECK;

	if (fts_writeFwCmd(&cmd, 1) < 0) {
		TS_LOG_ERR("%s production_test_ito: ERROR %02X\n", __func__, (ERROR_I2C_W | ERROR_PROD_TEST_ITO));
		return (ERROR_I2C_W | ERROR_PROD_TEST_ITO);
	}
	if(result != NULL)
		result->I2c_Check =true;

	res = pollForEvent(eventToSearch, 2, readData, TIMEOUT_ITO_TEST_RESULT);
	if (res < 0) {
		TS_LOG_ERR("%s production_test_ito: ITO Production test failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_ITO);
		return (res | ERROR_PROD_TEST_ITO);
	}

	if (readData[2] != 0x00 || readData[3] != 0x00) {
		if(result != NULL)
			result->ITO_Test_Res = false;
		TS_LOG_ERR("%s ITO Production testes finished!..FAILED  ERROR %02X\n", __func__, (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_ITO));
		res = (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_ITO);
	} else {
		if(result != NULL)
			result->ITO_Test_Res = true;
		TS_LOG_INFO("%s ITO Production test finished!..OK\n", __func__);
		res = OK;
	}

	res |= fts_system_reset();
	if (res < 0) {
		TS_LOG_ERR("%s production_test_ito: ERROR %02X\n", __func__, ERROR_PROD_TEST_ITO);
		res = (res | ERROR_PROD_TEST_ITO);
	}
	return res;
}


/*
 * Calibrate command: C5 XX YY
 * XX is the LSB and YY is the MSB of parameter. (the definition is listed below)
 * SCAN_EN_MS_SCR			0x0001
 * SCAN_EN_MS_KEY			0x0002
 * SCAN_EN_MS_MRN			0x0004
 * SCAN_EN_SS_HVR			0x0008
 * SCAN_EN_SS_TCH			0x0010
 * SCAN_EN_SS_KEY			0x0020
 * SCAN_EN_MS_SCR_SEC		0x0040
 * SCAN_EN_MS_SCR_LP		0x0100
 * SCAN_EN_MS_SIDE_TCH		0x0200
 * For example, if XX YY = 28 00, then it triggers SsHvr + SsKey Autotune.
 * Finish events: 16 0A XX YY ZZ 00 00
 * XX and YY are the scan modes triggered in FW.
 * ZZ is the status byte: 0 for success and 1 for failure. 
 * (Any error during Autotune implies a failure)
 */

#define FTS_CAL_CMD_LEN 3
int fts_calibrate(int calibrate_type)
{
	int ret;
	u8 cmd[FTS_CAL_CMD_LEN];
	u16 address;
	u8 readData[FIFO_EVENT_SIZE];
	int eventToSearch[2] = {EVENTID_STATUS_UPDATE, EVENT_TYPE_CALIBRATE};

	TS_LOG_ERR("%s: cal type:%d\n", __func__, calibrate_type);

	switch(calibrate_type) {
	case TOUCH_CALIBRATE_TYPE:
		address = SCAN_EN_MS_SCR | SCAN_EN_MS_KEY |
					SCAN_EN_SS_TCH | SCAN_EN_SS_HVR;
		break;
	case PRESSURE_CALIBRATE_TYPE:
		address = SCAN_EN_SS_KEY;
		break;
	default:
		TS_LOG_ERR("%s:unknown cal type\n", __func__);
		return -EINVAL;
	}

	cmd[0] = FTS_CMD_CALIBRATE_CMD;
	cmd[1] = address & 0x00FF;
	cmd[2] = (address & 0xFF00) >> 8;

	TS_LOG_INFO("%s:cmd: 0x%02x 0x%02x 0x%02x\n", __func__, cmd[0], cmd[1], cmd[2]);
	/* send calibrate cmd */
	if (fts_writeCmd(cmd, FTS_CAL_CMD_LEN) < 0) {
		TS_LOG_ERR("%s: cmd write fail\n", __func__);
		return -EIO;
	}

	/* wait calibrate done */
	ret = pollForEvent(eventToSearch, 2, readData, TIMEOUT_INITIALIZATION_TEST_RESULT);
	if (ret < 0) {
		TS_LOG_ERR("%s: poll event fail\n", __func__);
		return ret;
	}

	/* check response is 16 0A XX YY 0x00 or not*/
	if (readData[2] != cmd[1] || readData[3] != cmd[2] || readData[4]) {
		TS_LOG_INFO("%s:poll event error\n", __func__);
		return -ERROR_TEST_CHECK_FAIL;
	}

	/*save calibrate result*/
	ret = save_mp_flag(1);
	if (ret < 0) {
		TS_LOG_ERR("%s: save cal result fail\n", __func__);
		return -EIO;
	}

	TS_LOG_INFO("%s: done\n", __func__);
	return 0;
}

int production_test_initialization(u32 signature,TestResult *result)
{
	int res;
	u8 cmd;
	u8 readData[FIFO_EVENT_SIZE];
	int eventToSearch[2] = {EVENTID_STATUS_UPDATE, EVENT_TYPE_FULL_INITIALIZATION};

	if(result != NULL)
		result->Init_Res = false;
	res = fts_system_reset();
	if (res < 0) {
		TS_LOG_ERR("%s production_test_initialization: ERROR %02X\n", __func__, ERROR_PROD_TEST_INITIALIZATION);
		return (res | ERROR_PROD_TEST_INITIALIZATION);
	}

	cmd = FTS_CMD_FULL_INITIALIZATION;
	if (fts_writeFwCmd(&cmd, 1) < 0) {
		TS_LOG_ERR("%s production_test_initialization: ERROR %02X\n", __func__, (ERROR_I2C_W | ERROR_PROD_TEST_INITIALIZATION));
		return (ERROR_I2C_W | ERROR_PROD_TEST_INITIALIZATION);
	}

	res = pollForEvent(eventToSearch, 2, readData, TIMEOUT_INITIALIZATION_TEST_RESULT);
	if (res < 0) {
		TS_LOG_ERR("%s production_test_initialization: INITIALIZATION Production test failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_INITIALIZATION);
		return (res | ERROR_PROD_TEST_INITIALIZATION);
	}

	if (readData[2] != 0x00) {
		TS_LOG_INFO("%s INITIALIZATION Production testes finished!.........FAILED  ERROR %02X\n", __func__, (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_INITIALIZATION));
		res = (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_INITIALIZATION);
	} else {
		TS_LOG_INFO("%s INITIALIZATION Production test......OK\n", __func__);
		res = save_mp_flag(signature);
		if(res < OK)
			TS_LOG_ERR("%s SAVE FLAG:...FAIL! ERROR %08X\n", __func__, res);
		else
			TS_LOG_INFO("%s SAVE FLAG:.. OK!\n", __func__);
	}


	res |= readChipInfo(1);	//need to update the chipInfo in order to refresh the tuning_versione

	if (res<0) {
		TS_LOG_ERR("%s production_test_initialization: read chip info ERROR %02X\n", __func__, ERROR_PROD_TEST_INITIALIZATION);
		res= (res | ERROR_PROD_TEST_INITIALIZATION);
	}else{
		if(result != NULL)
			result->Init_Res = true;
	}

	return res;

}

int ms_compensation_tuning() {
	int res;
	u8 cmd;
	u8 readData[FIFO_EVENT_SIZE];
	int eventToSearch[2] = {EVENTID_STATUS_UPDATE, EVENT_TYPE_MS_TUNING_CMPL};


	TS_LOG_INFO("%s MS INITIALIZATION command sent...\n", __func__);
	cmd = FTS_CMD_MS_COMP_TUNING;
	if (fts_writeFwCmd(&cmd, 1) < 0) {
		TS_LOG_ERR("%s ms_compensation_tuning 2: ERROR %02X\n", __func__, (ERROR_I2C_W | ERROR_MS_TUNING));
		return (ERROR_I2C_W | ERROR_MS_TUNING);
	}


	TS_LOG_INFO("%s Looking for MS INITIALIZATION Event...\n", __func__);
	res = pollForEvent(eventToSearch, 2, readData, TIMEOUT_INITIALIZATION_TEST_RESULT);
	if (res < 0) {
		TS_LOG_ERR("%s ms_compensation_tuning: MS INITIALIZATION Production test failed... ERROR %02X\n", __func__, ERROR_MS_TUNING);
		return (res | ERROR_MS_TUNING);
	}

	if (readData[2] != 0x00 || readData[3] != 0x00) {
		TS_LOG_INFO("%s MS INITIALIZATION Production test finished!.................FAILED  ERROR %02X\n", __func__, ERROR_MS_TUNING);
		res = ERROR_MS_TUNING;
	} else {
		TS_LOG_INFO("%s MS INITIALIZATION Production test finished!.................OK\n", __func__);
		res = OK;
	}

	return res;
}

int ss_compensation_tuning() {
	int res;
	u8 cmd;
	u8 readData[FIFO_EVENT_SIZE];
	int eventToSearch[2] = {EVENTID_STATUS_UPDATE, EVENT_TYPE_SS_TUNING_CMPL};

	TS_LOG_INFO("%s SS INITIALIZATION command sent...\n", __func__);
	cmd = FTS_CMD_SS_COMP_TUNING;
	if (fts_writeFwCmd(&cmd, 1) < 0) {
		TS_LOG_ERR("%s ss_compensation_tuning 2: ERROR %02X\n", __func__, (ERROR_I2C_W | ERROR_SS_TUNING));
		return (ERROR_I2C_W | ERROR_SS_TUNING);
	}


	TS_LOG_INFO("%s Looking for SS INITIALIZATION Event...\n", __func__);
	res = pollForEvent(eventToSearch, 2, readData, TIMEOUT_INITIALIZATION_TEST_RESULT);
	if (res < 0) {
		TS_LOG_ERR("%s ms_compensation_tuning: SS INITIALIZATION Production test failed... ERROR %02X\n", __func__, ERROR_SS_TUNING);
		return (res | ERROR_SS_TUNING);
	}

	if (readData[2] != 0x00 || readData[3] != 0x00) {
		TS_LOG_INFO("%s SS INITIALIZATION Production test finished!.................FAILED  ERROR %02X\n", __func__, ERROR_SS_TUNING);
		res = ERROR_SS_TUNING;
	} else {
		TS_LOG_INFO("%s SS INITIALIZATION Production test finished!.................OK\n", __func__);
		res = OK;
	}

	return res;
}

int lp_timer_calibration() {
	int res;
	u8 cmd;
	u8 readData[FIFO_EVENT_SIZE];
	int eventToSearch[2] = {EVENTID_STATUS_UPDATE, EVENT_TYPE_LPTIMER_TUNING_CMPL};

	TS_LOG_INFO("%s LP TIMER CALIBRATION command sent...\n", __func__);
	cmd = FTS_CMD_LP_TIMER_CALIB;
	if (fts_writeFwCmd(&cmd, 1) < 0) {
		TS_LOG_ERR("%s lp_timer_calibration 2: ERROR %02X\n", __func__, (ERROR_I2C_W | ERROR_LP_TIMER_TUNING));
		return (ERROR_I2C_W | ERROR_LP_TIMER_TUNING);
	}


	TS_LOG_INFO("%s Looking for LP TIMER CALIBRATION Event...\n", __func__);
	res = pollForEvent(eventToSearch, 2, readData, TIMEOUT_INITIALIZATION_TEST_RESULT);
	if (res < 0) {
		TS_LOG_ERR("%s lp_timer_calibration: LP TIMER CALIBRATION Production test failed... ERROR %02X\n", __func__, ERROR_LP_TIMER_TUNING);
		return (res | ERROR_LP_TIMER_TUNING);
	}

	if (readData[2] != 0x00 || readData[3] != 0x01) {
		TS_LOG_INFO("%s LP TIMER CALIBRATION Production test finished!.................FAILED  ERROR %02X\n", __func__, ERROR_LP_TIMER_TUNING);
		res = ERROR_LP_TIMER_TUNING;
	} else {
		TS_LOG_INFO("%s LP TIMER CALIBRATION Production test finished!.................OK\n", __func__);
		res = OK;
	}

	return res;
}

int save_cx_tuning() {
	int res;
	u8 cmd;
	u8 readData[FIFO_EVENT_SIZE];
	int eventToSearch[2] = {EVENTID_STATUS_UPDATE, EVENT_TYPE_COMP_DATA_SAVED};

	TS_LOG_INFO("%s SAVE CX command sent...\n", __func__);
	cmd = FTS_CMD_SAVE_CX_TUNING;
	if (fts_writeCmd(&cmd, 1) < 0) {
		TS_LOG_ERR("%s save_cx_tuning 2: ERROR %02X\n", __func__, (ERROR_I2C_W | ERROR_SAVE_CX_TUNING));
		return (ERROR_I2C_W | ERROR_SAVE_CX_TUNING);
	}


	TS_LOG_INFO("%s Looking for SAVE CX Event...\n", __func__);
	res = pollForEvent(eventToSearch, 2, readData, TIMEOUT_INITIALIZATION_TEST_RESULT);
	if (res < 0) {
		TS_LOG_ERR("%s save_cx_tuning: SAVE CX failed... ERROR %02X\n", __func__, ERROR_SAVE_CX_TUNING);
		return (res | ERROR_SAVE_CX_TUNING);
	}


	if (readData[2] != 0x00 || readData[3] != 0x00) {
		TS_LOG_INFO("%s SAVE CX finished!.................FAILED  ERROR %02X\n", __func__, ERROR_SAVE_CX_TUNING);
		res = ERROR_SAVE_CX_TUNING;
	} else {
		TS_LOG_INFO("%s SAVE CX finished!.................OK\n", __func__);
		res = OK;
	}

	return res;
}

int production_test_splitted_initialization(int saveToFlash) {
	int res;

	TS_LOG_INFO("%s Splitted Initialization test is starting...\n", __func__);
	res = fts_system_reset();
	if (res < 0) {
		TS_LOG_ERR("%s production_test_initialization: ERROR %02X\n", __func__, ERROR_PROD_TEST_INITIALIZATION);
		return (res | ERROR_PROD_TEST_INITIALIZATION);
	}

	TS_LOG_INFO("%s MS INITIALIZATION TEST:\n", __func__);
	res = ms_compensation_tuning();
	if (res < 0) {
		TS_LOG_INFO("%s production_test_splitted_initialization: MS INITIALIZATION TEST FAILED! ERROR %02X\n", __func__, ERROR_PROD_TEST_INITIALIZATION);
		return (res | ERROR_PROD_TEST_INITIALIZATION);
	} else {
		TS_LOG_INFO("%s MS INITIALIZATION TEST OK!\n", __func__);

		TS_LOG_INFO("%s\n", __func__);

		TS_LOG_INFO("%s SS INITIALIZATION TEST:\n", __func__);
		res = ss_compensation_tuning();
		if (res < 0) {
			TS_LOG_INFO("%s production_test_splitted_initialization: SS INITIALIZATION TEST FAILED! ERROR %02X\n", __func__, ERROR_PROD_TEST_INITIALIZATION);
			return (res | ERROR_PROD_TEST_INITIALIZATION);
		} else {
			TS_LOG_INFO("%s SS INITIALIZATION TEST OK!\n", __func__);

			TS_LOG_INFO("%s\n", __func__);

			TS_LOG_INFO("%s LP INITIALIZATION TEST:\n", __func__);
			res = lp_timer_calibration();
			if (res < 0) {
				TS_LOG_INFO("%s production_test_splitted_initialization: LP INITIALIZATION TEST FAILED! ERROR %02X\n", __func__, ERROR_PROD_TEST_INITIALIZATION);
				return (res | ERROR_PROD_TEST_INITIALIZATION);
			} else {
				TS_LOG_INFO("%s LP INITIALIZATION TEST OK!\n", __func__);
				if (saveToFlash) {

					TS_LOG_INFO("%s\n", __func__);

					TS_LOG_INFO("%s SAVE CX TEST:\n", __func__);
					res = save_cx_tuning();
					if (res < 0) {
						TS_LOG_INFO("%s  production_test_splitted_initialization: SAVE CX TEST FAILED! ERROR %02X\n", __func__, res);
						return (res | ERROR_PROD_TEST_INITIALIZATION);
					} else {
						TS_LOG_INFO("%s SAVE CX TEST OK!\n", __func__);
					}
				}
		TS_LOG_INFO("%s Refresh Chip Info...\n", __func__);
		res |= readChipInfo(1);
			if (res<0) {
					TS_LOG_ERR("%s production_test_initialization: read chip info ERROR %02X\n", __func__, ERROR_PROD_TEST_INITIALIZATION);
					res= (res | ERROR_PROD_TEST_INITIALIZATION);
			}else
					TS_LOG_INFO("%s Splitted Initialization test finished!.................OK\n", __func__);
				return res;
			}
		}
	}

}

int production_test_main(char * pathThresholds, int stop_on_fail, int saveInit,
			TestToDo *todo, u32 signature,struct ts_rawdata_info *info,TestResult *result)
{
	int res, ret;

	TS_LOG_INFO("%s MAIN Production test is starting...\n", __func__);


	res = production_test_ito(result);
	if (res < 0) {
		TS_LOG_INFO("%s Error during ITO TEST! ERROR %08X\n", __func__, res);
		if (stop_on_fail) goto END; //in case of ITO TEST failure is no sense keep going
	} else {
		TS_LOG_INFO("%s ITO TEST OK!\n", __func__);
	}


	if (saveInit == 1){
		res = production_test_initialization(signature,result);
		if (res < 0) {
			TS_LOG_INFO("%s Error during  INITIALIZATION TEST! ERROR %08X\n", __func__, res);
			if (stop_on_fail) goto END;
		} else {
			TS_LOG_INFO("%s INITIALIZATION TEST OK!\n", __func__);
		}
	} else {
		TS_LOG_INFO("%s INITIALIZATION TEST :.... SKIPPED\n", __func__);
	}


	if (saveInit == 1) {
		TS_LOG_INFO("%s Cleaning up...\n", __func__);
		ret = cleanUp(0);
		if (ret < 0) {
			TS_LOG_ERR("%s production_test_main: clean up ERROR %02X\n", __func__, ret);
			res |= ret;
			if (stop_on_fail) goto END;
		}
		TS_LOG_INFO("%s\n", __func__);
	}

	ret = production_test_data(pathThresholds, stop_on_fail, todo,info,result);
	if (ret < 0) {
		TS_LOG_INFO("%s Error during PRODUCTION DATA TEST! ERROR %08X\n", __func__, ret);
	} else {
		TS_LOG_INFO("%s PRODUCTION DATA TEST OK!\n", __func__);
	}

	res |= ret;
	/*
	 * the OR is important because if the data test is OK
	 * but the inizialization test fail, the main production
	 * test result should = FAIL
	 */

END:
	if (res < 0) {
		TS_LOG_INFO("%s MAIN Production test finished....FAILED\n", __func__);
		return res;
	} else {
		TS_LOG_INFO("%s MAIN Production test finished...OK\n", __func__);
		return OK;
	}
}
static void get_average_max_min(short *data, int rows, int columns, char *result)
{
	int i;
	short ms_raw_max = 0;
	short ms_raw_min = 0;
	short ms_raw_average = 0;
	int ms_raw_total = 0;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return;
	}

	if((rows <= 0) || (columns <= 0)) {
		TS_LOG_ERR("%s:Invalid! row is %d, column is %d\n", __func__, rows, columns);
		return;
	}

	ms_raw_max = data[0];
	ms_raw_min = data[0];
	ms_raw_average = data[0];
	for (i = 0; i < rows * columns; i++) {
		ms_raw_max = max(ms_raw_max, data[i]);
		ms_raw_min = min(ms_raw_min, data[i]);
		ms_raw_total = ms_raw_total + data[i];
	}

	ms_raw_average = ms_raw_total/(rows*columns);

	TS_LOG_INFO("ms_raw_max:%d, ms_raw_min:%d, ms_raw_average:%d\n", ms_raw_max, ms_raw_min, ms_raw_average);
	snprintf(result, ST_NP_TEST_RES_BUF_LEN - 1, "[%d,%d,%d]", ms_raw_average, ms_raw_max, ms_raw_min);
}

static void st_fill_rawdata_buf(struct ts_rawdata_info *info, short *source_data, int rows, int columns, u16 datatype)
{
	int i,j,k = 0;
	int *data_ptr;
	struct fts_ts_info *fts_info = fts_get_info();
	if(NULL == source_data) {
		TS_LOG_ERR("%s:source_data is NULL\n", __func__);
		return;
	}

	if((rows <= 0) || (columns <= 0)) {
		TS_LOG_ERR("%s:Invalid! row is %d, column is %d\n", __func__, rows, columns);
		return;
	}

	TS_LOG_DEBUG("%s:info used size=%d, hybird used size=%d data type=%d, rows=%d, columns=%d\n",
				__func__, info->used_size, info->hybrid_buff_used_size, datatype, rows, columns);

	switch(datatype) {
	case MUTUALCOMPENSATIONTYPE:
	case MUTUALRAWTYPE:
	case STRENGTHTYPE:
		if ( info->used_size + rows * columns > TS_RAWDATA_BUFF_MAX){
			TS_LOG_ERR("%s:out of buffer range\n", __func__);
			return;
		}
		data_ptr = info->buff + info->used_size;
		for (i = 0; i < rows; i++) {
			for (j = 0; j < columns; j++) {
					data_ptr[k++] = (int)source_data[i*columns + j];
					printk("%d(%d) ", data_ptr[k - 1], source_data[i*columns + j]);
			}
			printk("\n");
		}

		info->used_size += rows * columns;
		break;
	case SELFFORCERAWTYPE:
	case SELFSENSERAWTYPE:
	case SSFORCEPRXTYPE:
	case SSSENSEPRXTYPE:
	case SSSENSEDATATYPE:
		data_ptr = info->hybrid_buff + info->hybrid_buff_used_size;
		for (i = 0; i < rows; i++) {
			for (j = 0; j < columns; j++) {
					data_ptr[k++] = (int)source_data[i*columns + j];
			}
		}

		info->hybrid_buff_used_size += rows * columns;
		TS_LOG_INFO("[%s]info->hybrid_buff_used_size -> %d\n",__func__,info->hybrid_buff_used_size);
		break;
	default:
		break;
	}

	return;
}


static void print_data_s16(short *data, int rows, int columns)
{
	int i, j;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return;
	}

	if((rows <= 0) || (columns <= 0)) {
		TS_LOG_ERR("%s:Invalid! row is %d, column is %d\n", __func__, rows, columns);
		return;
	}

	for (i = 0; i < rows; i++) {
		for (j = 0; j < columns; j++) {
			printk("%d ", data[i*columns + j]);
		}
		printk("\n");
	}
}


int production_test_ms_raw(char *path_limits, int stop_on_fail,
	TestToDo *todo,struct ts_rawdata_info *info, TestResult *result)
{
	struct fts_ts_info *fts_info = fts_get_info();
	int ret,count_fail=0;
	MutualSenseFrame msRawFrame;
	MutualSenseFrame msStrengthFrame;
	int rows, columns;

	int *thresholds = NULL;
	int trows, tcolumns;

	memset(&msStrengthFrame, 0, sizeof(MutualSenseFrame));
	memset(&msRawFrame, 0, sizeof(MutualSenseFrame));

	//******************** Mutual Sense Test **********/
	TS_LOG_INFO("%s MS RAW DATA TEST is starting...\n", __func__);
	if(result != NULL){

		columns = getSenseLen();
		rows = getForceLen();
		TS_LOG_INFO("%s:rows=%d, columns=%d\n", __func__, rows, columns);
	}

	if (todo->MutualRaw == 1 || todo->MutualRawGap == 1) {
		ret = getMSFrame2(MS_TOUCH_ACTIVE, &msRawFrame); /*rawdata*/
		if (ret < 0) {
			TS_LOG_ERR("%s : getMSFrame failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			return (ret | ERROR_PROD_TEST_DATA);
		}

		/* check mutal rew */
		if (todo->MutualRaw == 1) {
			TS_LOG_INFO("%s MS RAW MIN MAX TEST:\n", __func__);
			ret = parseProductionTestLimits(path_limits, MS_RAW_MIN_MAX, &thresholds, &trows, &tcolumns);
			if (ret < 0 || (trows != 1 || tcolumns != 2)) {
				TS_LOG_ERR("%s : parseProductionTestLimits MS_RAW_MIN_MAX failed... ERROR %02X,.%d ..%d\n", __func__, ERROR_PROD_TEST_DATA,trows,tcolumns);
				ret |= ERROR_PROD_TEST_DATA;
				goto ERROR_LIMITS;
			}

			ret = checkLimitsMinMax(msRawFrame.node_data, msRawFrame.header.force_node, msRawFrame.header.sense_node, thresholds[0], thresholds[1]);
			if (ret != OK) {
				TS_LOG_ERR("%s : checkLimitsMinMax MS RAW failed... ERROR COUNT = %d\n", __func__, ret);
				TS_LOG_INFO("%s MS RAW MIN MAX TEST:.........FAIL\n", __func__);
				count_fail+=1;
				if (stop_on_fail == 1) goto ERROR;
			}else{
				if(result != NULL)
					result->MutualRawRes = true;
				TS_LOG_INFO("%s MS RAW MIN MAX TEST:.................OK\n", __func__);
			}
			kfree(thresholds);
			thresholds = NULL;
		} 

		/* check mutal raw gap limit */
		if (todo->MutualRawGap == 1) {
			TS_LOG_INFO("%s MS RAW GAP TEST:\n", __func__);
			if(fts_info->check_MutualRawGap_after_callibrate){
				ret = parseProductionTestLimits(path_limits, MS_RAW_GAP_CHECK_AFTER_CAL, &thresholds, &trows, &tcolumns);
				if (ret < 0 || (trows != 1 || tcolumns != 1)) {
					TS_LOG_ERR("%s : parseProductionTestLimits MS_RAW_GAP failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
					ret |= ERROR_PROD_TEST_DATA;
					goto ERROR_LIMITS;
				}
				TS_LOG_INFO("%s : check_MutualRawGap_after_callibrate:thresholds =%d\n", __func__,thresholds[0]);
			}
			else
			{
				ret = parseProductionTestLimits(path_limits, MS_RAW_GAP, &thresholds, &trows, &tcolumns);
				if (ret < 0 || (trows != 1 || tcolumns != 1)) {
					TS_LOG_ERR("%s : parseProductionTestLimits MS_RAW_GAP failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
					ret |= ERROR_PROD_TEST_DATA;
					goto ERROR_LIMITS;
				}
			}
			TS_LOG_INFO(" [%s] -> %d|%d|%d\n",__func__,msRawFrame.header.force_node,
									msRawFrame.header.sense_node, thresholds[0]);

			ret = check_gap_limit(msRawFrame.node_data, msRawFrame.header.force_node,
									msRawFrame.header.sense_node, thresholds[0]);
			if (ret) {
				TS_LOG_ERR("%s : checkLimitsGap MS RAW failed... ERROR = %02X\n", __func__, ret);
				count_fail+=1;
				if(result != NULL)
					result->MutualRawResGap = false;
				if (stop_on_fail == 1) goto ERROR;

			} else {
				if(result != NULL)
					result->MutualRawResGap = true;
				TS_LOG_INFO("%s MS RAW GAP TEST:.................OK\n", __func__);
			}
			kfree(thresholds);
			thresholds = NULL;
		}

	}

	/* mutal noise test */
	if(result && info) {
		TS_LOG_INFO("%s MS RAW NOISE TEST:\n", __func__);
		ret = getMSFrame(ADDR_NORM_TOUCH, &msStrengthFrame, 0); /*detla data*/
		if (ret < 0) {
			TS_LOG_ERR("%s : getMSFrame failed error %02X\n", __func__, ERROR_PROD_TEST_DATA);
			return (ret | ERROR_PROD_TEST_DATA);
		}
		ret = parseProductionTestLimits(path_limits, MS_STRENGTH_MIN_MAX, &thresholds, &trows, &tcolumns);
		if (ret < 0 || (trows != 1 || tcolumns != 2)) {
			TS_LOG_ERR("%s : parseProductionTestLimits MS_STRENGTH_MIN_MAX failed... ERROR %02X,.%d ..%d\n", __func__, ERROR_PROD_TEST_DATA,trows,tcolumns);
			ret |= ERROR_PROD_TEST_DATA;
			goto ERROR_LIMITS;
		}


		ret = checkLimitsMinMax(msStrengthFrame.node_data,msStrengthFrame.header.force_node, msRawFrame.header.sense_node, thresholds[0], thresholds[1]);
		if (ret != OK) {
			TS_LOG_ERR("%s : checkLimitsMinMax MS RAW failed... ERROR COUNT = %d\n", __func__, ret);
			TS_LOG_INFO("%s MS RAW MIN MAX TEST:.........FAIL\n", __func__);
			count_fail+=1;
			if (stop_on_fail == 1) goto ERROR;
		}else{
			if(result != NULL)
				result->MutualStrengthRes= true;
			TS_LOG_INFO("%s MS RAW MIN MAX TEST:.................OK\n", __func__);
		}
		kfree(thresholds);
		thresholds = NULL;
	}


	if(info  && result) {
		print_data_s16(msRawFrame.node_data, rows, columns);
		print_data_s16(msStrengthFrame.node_data, rows, columns);
		st_fill_rawdata_buf(info, msRawFrame.node_data, rows, columns, MUTUALRAWTYPE);
		st_fill_rawdata_buf(info, msStrengthFrame.node_data, rows, columns, STRENGTHTYPE);
		get_average_max_min(msRawFrame.node_data, rows, columns, result->mutal_raw_res_buf);
		get_average_max_min(msStrengthFrame.node_data, rows, columns, result->mutal_noise_res_buf);
	}
ERROR:
	kfree(msRawFrame.node_data);
	msRawFrame.node_data = NULL;
	kfree(msRawFrame.node_data);
	msRawFrame.node_data = NULL;
	kfree(msStrengthFrame.node_data);
	msStrengthFrame.node_data = NULL;
	kfree(thresholds);
	if (count_fail == 0) {
		TS_LOG_INFO("%s MS RAW DATA TEST:OK\n", __func__);
		return OK;
	} else {
		TS_LOG_INFO("%s MS RAW DATA TEST:FAIL fails_count = %d\n", __func__, count_fail);
		return (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
	}

ERROR_LIMITS:
	kfree(msRawFrame.node_data);
	kfree(msStrengthFrame.node_data);
	kfree(thresholds);
	return ret;
}

int production_test_ms_key_raw(char *path_limits) {

	int ret;
	MutualSenseFrame msRawFrame;

	int *thresholds = NULL;
	int trows, tcolumns;

	//******************************* Mutual Sense Test *******************************/
	TS_LOG_INFO("%s MS KEY RAW DATA TEST is starting...\n", __func__);

	ret = getMSFrame2(MS_KEY, &msRawFrame);
	if (ret < 0) {
		TS_LOG_ERR("%s : getMSKeyFrame failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
		return (ret | ERROR_PROD_TEST_DATA);
	}

	ret = parseProductionTestLimits(path_limits, MS_KEY_RAW_MIN_MAX, &thresholds, &trows, &tcolumns);
	if (ret < 0 || (trows != 1 || tcolumns != 2)) {
		TS_LOG_ERR("%s : parseProductionTestLimits MS_KEY_RAW_MIN_MAX failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
		ret |= ERROR_PROD_TEST_DATA;
		goto ERROR_LIMITS;
	}

	ret = checkLimitsMinMax(msRawFrame.node_data, msRawFrame.header.force_node, msRawFrame.header.sense_node, thresholds[0], thresholds[1]);
	if (ret != OK) {
		TS_LOG_ERR("%s : checkLimitsMinMax MS KEY RAW failed... ERROR COUNT = %d\n", __func__, ret);
		goto ERROR;
	} else
		TS_LOG_INFO("%s MS KEY RAW TEST:.................OK\n", __func__);

	kfree(thresholds);
	thresholds = NULL;

	kfree(msRawFrame.node_data);
	msRawFrame.node_data = NULL;
	return OK;

ERROR:
	if(msRawFrame.node_data!=NULL) kfree(msRawFrame.node_data);
	if(thresholds!=NULL) kfree(thresholds);
	TS_LOG_INFO("%s MS KEY RAW TEST:.................FAIL\n", __func__);
	return (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);

ERROR_LIMITS:
	if(msRawFrame.node_data!=NULL) kfree(msRawFrame.node_data);
	if(thresholds!=NULL) kfree(thresholds);
	return ret;

}

int production_test_ms_cx(char *path_limits, int stop_on_fail, TestToDo *todo,struct ts_rawdata_info *info,TestResult *result) {

	int ret;
	int count_fail = 0;
	int *thresholds = NULL;
	int *thresholds_min = NULL;
	int *thresholds_max = NULL;
	int trows, tcolumns;
	int rows,columns;
	MutualSenseData msCompData;
	int i;

	memset(&msCompData, 0, sizeof(MutualSenseData));


	/* MS CX TEST - calibration data*/
	TS_LOG_INFO("%s:MS CX Testes are starting...\n", __func__);
	ret = readMutualSenseCompensationData(MS_TOUCH_ACTIVE, &msCompData); //read MS compensation data
	if (ret < 0) {
		TS_LOG_ERR("%s:readMutualSenseCompensationData failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
		return (ret | ERROR_PROD_TEST_DATA);
	}

	columns = msCompData.header.force_node;
	rows = msCompData.header.sense_node;

	if (todo->MutualCx2) {
		TS_LOG_INFO("%s MS CX2 MIN MAX TEST:\n", __func__);
		ret = parseProductionTestLimits(path_limits, MS_CX2_MAP_MIN, &thresholds_min, &trows, &tcolumns); //load min thresholds
		if (ret < 0) {
			TS_LOG_ERR("%s : parseProductionTestLimits MS_CX2_MAP_MIN failed... ERROR %02X %d,%d,%d,%d\n", __func__, ERROR_PROD_TEST_DATA,msCompData.header.force_node,trows,tcolumns,msCompData.header.sense_node);
			ret |= ERROR_PROD_TEST_DATA;
			goto ERROR_LIMITS;
		}

		ret = parseProductionTestLimits(path_limits, MS_CX2_MAP_MAX, &thresholds_max, &trows, &tcolumns); //load max thresholds
		if (ret < 0 ) {
			TS_LOG_ERR("%s : parseProductionTestLimits MS_CX2_MAP_MAX failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			ret |= ERROR_PROD_TEST_DATA;
	 		goto ERROR_LIMITS;
		}

		ret = checkLimitsMap(msCompData.node_data, msCompData.header.force_node, msCompData.header.sense_node, thresholds_min, thresholds_max); //check the limits
		if (ret != OK) {
			TS_LOG_ERR("%s : checkLimitsMap MS CX2 MIN MAX failed... ERROR COUNT = %d\n", __func__, ret);
			TS_LOG_INFO("%s MS CX2 MIN MAX TEST:...FAIL\n", __func__);
			count_fail += 1;
			if (stop_on_fail) goto ERROR;
		} else{
			if(result != NULL)
				result->MutualCx2Res = true;
			TS_LOG_INFO("%s MS CX2 MIN MAX TEST:...OK\n", __func__);
		}
		kfree(thresholds_min);
		thresholds_min = NULL;
		kfree(thresholds_max);
		thresholds_max = NULL;
	}

	if(info && result){
		columns= msCompData.header.sense_node;
		rows = msCompData.node_data_size/columns;

		u16 *temp_u16_array = (u16 *)kmalloc(sizeof(u16) * columns * rows, GFP_KERNEL);
		for (i = 0; i < columns * rows; i++)
			temp_u16_array[i] = (u16)msCompData.node_data[i];

		st_fill_rawdata_buf(info, temp_u16_array, rows, columns, MUTUALCOMPENSATIONTYPE);
		get_average_max_min(temp_u16_array, rows, columns, result->mutal_cal_res_buf);

		kfree(temp_u16_array);
	}


ERROR:
	if (count_fail == 0) {
		TS_LOG_INFO("%s MS CX testes finished!...OK\n", __func__);
		kfree(msCompData.node_data);
		msCompData.node_data = NULL;
		return OK;
	} else {
		TS_LOG_INFO("%s MS CX testes finished!...FAILED  fails_count = %d\n", __func__, count_fail);
		if(thresholds!= NULL) kfree(thresholds);
		if(thresholds_min!= NULL) kfree(thresholds_min);
		if(thresholds_max!= NULL) kfree(thresholds_max);
		if(msCompData.node_data != NULL) kfree(msCompData.node_data);
		return (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_DATA);
	}

ERROR_LIMITS:
	if(thresholds!= NULL) kfree(thresholds);
	if(thresholds_min!= NULL) kfree(thresholds_min);
	if(thresholds_max!= NULL) kfree(thresholds_max);
	if(msCompData.node_data != NULL) kfree(msCompData.node_data);
	return ret;
}

int production_test_ms_key_cx(char *path_limits, int stop_on_fail, TestToDo *todo) {

	int ret;
	int count_fail = 0;
	int num_keys = 0;

	int *thresholds = NULL;
	int *thresholds_min = NULL;
	int *thresholds_max = NULL;
	int trows, tcolumns;

	MutualSenseData msCompData;

	u16 container;
	u16 *total_cx = NULL;


	//MS CX TEST
	TS_LOG_INFO("%s MS KEY CX Testes are starting...\n", __func__);

	ret = readMutualSenseCompensationData(MS_KEY, &msCompData); //read MS compensation data
	if (ret < 0) {
		TS_LOG_ERR("%s : readMutualSenseCompensationData failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
		return (ret | ERROR_PROD_TEST_DATA);
	}

	if (msCompData.header.force_node > msCompData.header.sense_node) //the meaningful data are only in the first row, the other rows are only a copy of the first one
		num_keys = msCompData.header.force_node;
	else
		num_keys = msCompData.header.sense_node;

	TS_LOG_INFO("%s MS KEY CX1 TEST:\n", __func__);
	if (todo->MutualKeyCx1 == 1) {

		ret = parseProductionTestLimits(path_limits, MS_KEY_CX1_MIN_MAX, &thresholds, &trows, &tcolumns);
		if (ret < 0 || (trows != 1 || tcolumns != 2)) {
			TS_LOG_ERR("%s : parseProductionTestLimits MS_KEY_CX1_MIN_MAX failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			ret |= ERROR_PROD_TEST_DATA;
			goto ERROR_LIMITS;
		}

		container = (u16) msCompData.cx1;
		ret = checkLimitsMinMax(&container, 1, 1, thresholds[0], thresholds[1]); //check the limits
		if (ret != OK) {
			TS_LOG_ERR("%s : checkLimitsMinMax MS CX1 failed... ERROR COUNT = %d\n", __func__, ret);
			TS_LOG_INFO("%s MS KEY CX1 TEST:.................FAIL\n", __func__);
			count_fail += 1;
			if (stop_on_fail) goto ERROR;
		} else
			TS_LOG_INFO("%s MS KEY CX1 TEST:.................OK\n", __func__);
	} else
		TS_LOG_INFO("%s MS KEY CX1 TEST:.................SKIPPED\n", __func__);

	kfree(thresholds);
	thresholds = NULL;

	TS_LOG_INFO("%s MS KEY CX2 TEST:\n", __func__);
	if (todo->MutualKeyCx2 == 1) {
		ret = parseProductionTestLimits(path_limits, MS_KEY_CX2_MAP_MIN, &thresholds_min, &trows, &tcolumns); //load min thresholds
		if (ret < 0 || (trows != 1 || tcolumns != num_keys)) {
			TS_LOG_ERR("%s : parseProductionTestLimits MS_KEY_CX2_MAP_MIN failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			ret |= ERROR_PROD_TEST_DATA;
			goto ERROR_LIMITS;
		}

		ret = parseProductionTestLimits(path_limits, MS_KEY_CX2_MAP_MAX, &thresholds_max, &trows, &tcolumns); //load max thresholds
		if (ret < 0 || (trows != 1 || tcolumns != num_keys)) {
			TS_LOG_ERR("%s : parseProductionTestLimits MS_KEY_CX2_MAP_MAX failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			ret |= ERROR_PROD_TEST_DATA;
			goto ERROR_LIMITS;
		}

		ret = checkLimitsMap(msCompData.node_data, 1, num_keys, thresholds_min, thresholds_max); //check the limits
		if (ret != OK) {
			TS_LOG_ERR("%s : checkLimitsMap MS KEY CX2 failed... ERROR COUNT = %d\n", __func__, ret);
			TS_LOG_INFO("%s MS KEY CX2 TEST:.................FAIL\n", __func__);
			count_fail += 1;
			if (stop_on_fail) goto ERROR;
		} else
			TS_LOG_INFO("%s MS KEY CX2 TEST:.................OK\n", __func__);

		kfree(thresholds_min);
	thresholds_min = NULL;
		kfree(thresholds_max);
	thresholds_max = NULL;
	} else
		TS_LOG_INFO("%s MS CX2 TEST:.................SKIPPED\n", __func__);

	//START OF TOTAL CHECK
	TS_LOG_INFO("%s MS KEY TOTAL CX TEST:\n", __func__);

	if (todo->MutualKeyCxTotal == 1) {
		ret = computeTotal(msCompData.node_data, msCompData.cx1, 1, num_keys, CX1_WEIGHT, CX2_WEIGHT, &total_cx);
		if (ret < 0) {
			TS_LOG_ERR("%s : computeTotalCx failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			ret |= ERROR_PROD_TEST_DATA;
			goto ERROR_LIMITS;
		}

		ret = parseProductionTestLimits(path_limits, MS_KEY_TOTAL_CX_MAP_MIN, &thresholds_min, &trows, &tcolumns); //load min thresholds
		if (ret < 0 || (trows != 1 || tcolumns != num_keys)) {
			TS_LOG_ERR("%s : parseProductionTestLimits MS_KEY_TOTAL_CX_MAP_MIN failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			ret |= ERROR_PROD_TEST_DATA;
			goto ERROR_LIMITS;
		}

		ret = parseProductionTestLimits(path_limits, MS_KEY_TOTAL_CX_MAP_MAX, &thresholds_max, &trows, &tcolumns); //load max thresholds
		if (ret < 0 || (trows != 1 || tcolumns != num_keys)) {
			TS_LOG_ERR("%s : parseProductionTestLimits MS_KEY_TOTAL_CX_MAP_MAX failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			ret |= ERROR_PROD_TEST_DATA;
			goto ERROR_LIMITS;
		}

		ret = checkLimitsMapTotal(total_cx, 1, num_keys, thresholds_min, thresholds_max); //check the limits
		if (ret != OK) {
			TS_LOG_ERR("%s : checkLimitsMap  MS TOTAL KEY CX TEST failed... ERROR COUNT = %d\n", __func__, ret);
			TS_LOG_INFO("%s MS KEY TOTAL CX TEST:.................FAIL\n", __func__);
			count_fail += 1;
			if (stop_on_fail) goto ERROR;
		} else
			TS_LOG_INFO("%s MS KEY TOTAL CX TEST:.................OK\n", __func__);

		kfree(thresholds_min);
		thresholds_min = NULL;
		kfree(thresholds_max);
		thresholds_max = NULL;

		kfree(total_cx);
		total_cx = NULL;
	}

ERROR:
	TS_LOG_INFO("%s\n", __func__);
	if (count_fail == 0) {
		TS_LOG_INFO("%s MS KEY CX testes finished!.................OK\n", __func__);
	kfree(msCompData.node_data);
	msCompData.node_data = NULL;
		return OK;
	} else {
		print_frame_u8("MS Key Init Data (Cx2) =", array1dTo2d_u8(msCompData.node_data, msCompData.node_data_size, msCompData.header.sense_node), 1, msCompData.header.sense_node);
		TS_LOG_INFO("%s MS Key CX testes finished!.................FAILED  fails_count = %d\n", __func__, count_fail);
		if(thresholds != NULL) kfree(thresholds);
		if(thresholds_min != NULL) kfree(thresholds_min);
		if(thresholds_max != NULL) kfree(thresholds_max);
		if(msCompData.node_data != NULL) kfree(msCompData.node_data);
		if(total_cx != NULL) kfree(total_cx);
		return (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_DATA);
	}

ERROR_LIMITS:
	if(thresholds != NULL) kfree(thresholds);
	if(thresholds_min != NULL) kfree(thresholds_min);
	if(thresholds_max != NULL) kfree(thresholds_max);
	if(msCompData.node_data != NULL) kfree(msCompData.node_data);
	if(total_cx != NULL) kfree(total_cx);
	return ret;

}

int production_test_ss_raw(char *path_limits, int stop_on_fail, TestToDo *todo,struct ts_rawdata_info *info,TestResult *result) {
	int ret;
	int count_fail = 0;
	int rows, columns;
	short *ssforce_frame;
	short *sssense_frame;
	SelfSenseFrame ssRawFrame;
	SelfSenseData comData;
	short value[2] ={ 0};
	int *thresholds = NULL;
	int trows, tcolumns;

	memset(&ssRawFrame, 0, sizeof(SelfSenseFrame));
	memset(&comData , 0 ,sizeof(SelfSenseData));
	/* MS SS TEST -self cap test*/
	TS_LOG_INFO("%s:self test starting\n", __func__);
	ret = getSSFrame2(SS_TOUCH, &ssRawFrame);
	if (ret < 0) {
		TS_LOG_ERR("%s : getSSFrame failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
		return (ret | ERROR_PROD_TEST_DATA);
	}

	/* SS RAW (PROXIMITY) FORCE TEST - tx test*/
	TS_LOG_INFO("%s SS RAW (PROXIMITY) FORCE TEST\n", __func__);
	columns = 1; /* there are no data for the sense channels due to the fact that the force frame is analized */
	rows = ssRawFrame.header.force_node;

	if (todo->SelfForceRaw) {
		ret = parseProductionTestLimits(path_limits, SS_RAW_FORCE_MIN_MAX, &thresholds, &trows, &tcolumns);
		if (ret < 0 || (trows != 1 || tcolumns != 2)) {
			TS_LOG_ERR("%s : parseProductionTestLimits SS_RAW_FORCE_MIN_MAX failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			//return (ret | ERROR_PROD_TEST_DATA);
			ret |= ERROR_PROD_TEST_DATA;
			goto ERROR_LIMITS;
		}

		ret = checkLimitsMinMax(ssRawFrame.force_data, rows, columns, thresholds[0], thresholds[1]);
		if (ret != OK) {
			TS_LOG_ERR("%s : checkLimitsMinMax SS RAW (PROXIMITY) FORCE failed... ERROR COUNT = %d\n", __func__, ret);
			TS_LOG_INFO("%s SS RAW (PROXIMITY) FORCE MIN MAX TEST:...FAIL\n", __func__);
			count_fail += 1;
			if (stop_on_fail){
		 		ret = ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL;
		 		goto ERROR_LIMITS;
			}
		} else{
			if(result != NULL)
				result->SelfForceRawRes = true;
			TS_LOG_INFO("%s SS RAW (PROXIMITY) FORCE MIN MAX TEST:...OK\n", __func__);
		}

		kfree(thresholds);
		thresholds = NULL;
	}

	//SS RAW (PROXIMITY) SENSE TEST
	columns = ssRawFrame.header.sense_node;
	rows = 1; // there are no data for the force channels due to the fact that the sense frame is analized

	if (todo->SelfSenseRaw == 1) {
		TS_LOG_INFO("%s self RAW SENSE TEST\n", __func__);
		ret = parseProductionTestLimits(path_limits, SS_RAW_SENSE_MIN_MAX, &thresholds, &trows, &tcolumns);
		if (ret < 0 || (trows != 1 || tcolumns != 2)) {
			TS_LOG_ERR("%s : parseProductionTestLimits SS_RAW_SENSE_MIN_MAX failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			ret |= ERROR_PROD_TEST_DATA;
			goto ERROR_LIMITS;
		}

		ret = checkLimitsMinMax(ssRawFrame.sense_data, rows, columns, thresholds[0], thresholds[1]);
		if (ret != OK) {
			TS_LOG_ERR("%s : checkLimitsMinMax SS RAW (PROXIMITY) SENSE failed... ERROR COUNT = %d\n", __func__, ret);
			TS_LOG_INFO("%s SS RAW (PROXIMITY) SENSE MIN MAX TEST:...FAIL\n", __func__);
			count_fail += 1;
			if (stop_on_fail){
				ret= ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL;
				goto ERROR_LIMITS;
			}
		} else{
			if(result != NULL)
				result->SelfSenseRawRes = true;
			TS_LOG_INFO("%s SS RAW (PROXIMITY) SENSE MIN MAX TEST:...OK\n", __func__);
		}
		kfree(thresholds);
		thresholds = NULL;
	}

	if(result  && info) {
		TS_LOG_INFO("%s self noise cap TEST\n", __func__);
		ret = getSSFrame(ADDR_NORM_PRX_FORCE, &ssforce_frame); /*self noise tx*/
		if (ret < 0) {
			TS_LOG_ERR("%s : ADDR_NORM_PRX_FORCE failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			return (ret | ERROR_PROD_TEST_DATA);
		}
		ret = parseProductionTestLimits(path_limits, SS_PRX_FORCE_MIN_MAX, &thresholds, &trows, &tcolumns);
		if (ret < 0 || (trows != 1 || tcolumns != 2)) {
			TS_LOG_ERR("%s : parseProductionTestLimits ADDR_NORM_PRX_FORCE failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			//return (ret | ERROR_PROD_TEST_DATA);
			ret |= ERROR_PROD_TEST_DATA;
			goto ERROR_LIMITS;
		}
		ret = checkLimitsMinMax(ssforce_frame, 16, 1, thresholds[0], thresholds[1]);
		if (ret != OK) {
			TS_LOG_ERR("%s : checkLimitsMinMax SS RAW (PROXIMITY) FORCE failed... ERROR COUNT = %d\n", __func__, ret);
			TS_LOG_INFO("%s  RAW_PRX_FORCE MIN MAX TEST:...FAIL\n", __func__);
			count_fail += 1;
			if (stop_on_fail){
				ret = ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL;
				goto ERROR_LIMITS;
			}
		} else{
			result->SelfSenseStrengthData= true;
			TS_LOG_INFO("%s SS (PROXIMITY) FORCE MIN MAX TEST:..OK\n", __func__);
		}
		kfree(thresholds);
		thresholds = NULL;


		ret = getSSFrame(ADDR_NORM_PRX_SENSE, &sssense_frame); /*self noise rx*/
		if (ret < 0) {
			TS_LOG_ERR("%s : getSSFrame failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			return (ret | ERROR_PROD_TEST_DATA);
		}
		ret = parseProductionTestLimits(path_limits, SS_PRX_SENSE_MIN_MAX, &thresholds, &trows, &tcolumns);
		if (ret < 0 || (trows != 1 || tcolumns != 2)) {
			TS_LOG_ERR("%s : parseProductionTestLimits SS_PRX_SENSE_MIN_MAX failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			ret |= ERROR_PROD_TEST_DATA;
			goto ERROR_LIMITS;
		}

		ret = checkLimitsMinMax(sssense_frame, 1, 32, thresholds[0], thresholds[1]);
		if (ret != OK) {
			TS_LOG_INFO("%s  RAW_PRX_SENSE MIN MAX TEST:....FAIL\n", __func__);
			count_fail += 1;
			if (stop_on_fail){
				ret = ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL;
				goto ERROR_LIMITS;
			}
		} else{
			if(result)
				result->SelfForceStrengthData= true;
			TS_LOG_INFO("%s SS (PROXIMITY) SENSE MIN MAX TEST:...OK\n", __func__);
		}
		kfree(thresholds);
		thresholds = NULL;

		fts_disableInterrupt();
		ret = readSelfSenseCompensationData(SS_KEY, &comData);
		fts_enableInterrupt();
		if (ret < 0) {
			TS_LOG_ERR("%s : read comData failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			return (ret | ERROR_PROD_TEST_DATA);
		}
		ret = parseProductionTestLimits(path_limits, SS_COM_SENSE_DATA_MIN_MAX, &thresholds, &trows, &tcolumns);
		if (ret < 0 || (trows != 1 || tcolumns != 2)) {
			TS_LOG_ERR("%s : parseProductionTestLimits SS_COM_SENSE_DATA_MIN_MAX failed... ERROR %02X\n", __func__, ERROR_PROD_TEST_DATA);
			ret |= ERROR_PROD_TEST_DATA;
			goto ERROR_LIMITS;
		}
		value[0] = forcekeyvalue(comData.f_ix1,comData.ix2_fm[0]);
		value[1] = forcekeyvalue(comData.f_ix1,comData.ix2_fm[1]);


		TS_LOG_INFO("[%s]value[0] |value[1]  -> %d|%d\n",__func__,value[0] ,value[1] );

		ret = checkLimitsMinMax(value, 2, 1, thresholds[0], thresholds[1]);
		if (ret != OK) {
			TS_LOG_INFO("%s  SS_COM_SENSE_DATA_MIN_MAX TEST:....FAIL\n", __func__);
			count_fail += 1;
			if (stop_on_fail){
				ret = ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL;
				goto ERROR_LIMITS;
			}
		} else{
			if(result)
				result->SelfSenseData = true;
			TS_LOG_INFO("%s SS_COM_SENSE_DATA_MIN_MAX TEST:...OK\n", __func__);
		}
		kfree(thresholds);
		thresholds = NULL;

		print_data_s16(sssense_frame, 1, ssRawFrame.header.sense_node);
		st_fill_rawdata_buf(info, sssense_frame, 1, ssRawFrame.header.sense_node, SSSENSEPRXTYPE);

		print_data_s16(ssforce_frame, ssRawFrame.header.force_node, 1);
		st_fill_rawdata_buf(info, ssforce_frame, ssRawFrame.header.force_node, 1, SSFORCEPRXTYPE);

		print_data_s16(ssRawFrame.sense_data, 1, ssRawFrame.header.sense_node);
		st_fill_rawdata_buf(info, ssRawFrame.sense_data, 1, ssRawFrame.header.sense_node, SELFSENSERAWTYPE);

		print_data_s16(ssRawFrame.force_data, ssRawFrame.header.force_node, 1);
		st_fill_rawdata_buf(info, ssRawFrame.force_data, ssRawFrame.header.force_node, 1, SELFFORCERAWTYPE);

		print_data_s16(value, 1,2);
		st_fill_rawdata_buf(info, value,1,2, SSSENSEDATATYPE);


		kfree(sssense_frame);
		sssense_frame = NULL;
		kfree(ssforce_frame);
		ssforce_frame = NULL;
	}
	kfree(ssRawFrame.force_data);
	ssRawFrame.force_data = NULL;
	kfree(ssRawFrame.sense_data);
	ssRawFrame.sense_data = NULL;

	kfree(comData.ix2_fm);
	comData.ix2_fm = NULL;
	kfree(comData.ix2_sn);
	comData.ix2_sn = NULL;
	kfree(comData.cx2_fm);
	comData.cx2_fm = NULL;
	kfree(comData.cx2_sn);
	comData.cx2_sn = NULL;

	if (count_fail == 0) {
		TS_LOG_INFO("%s self RAW testes finished!...OK\n", __func__);
		return OK;
	} else {
		TS_LOG_INFO("%s self RAW testes finished!...FAILED  fails_count = %d\n", __func__, count_fail);
		return (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_DATA);
	}

ERROR_LIMITS:
	kfree(comData.ix2_fm);
	comData.ix2_fm = NULL;
	kfree(comData.ix2_sn);
	comData.ix2_sn = NULL;
	kfree(comData.cx2_fm);
	comData.cx2_fm = NULL;
	kfree(comData.cx2_sn);
	comData.cx2_sn = NULL;

	kfree(ssRawFrame.force_data);
	ssRawFrame.force_data = NULL;
	kfree(ssRawFrame.sense_data);
	ssRawFrame.sense_data = NULL;
	kfree(thresholds);
	thresholds = NULL;

	return ret;

}

int production_test_data(char *path_limits, int stop_on_fail, TestToDo *todo,struct ts_rawdata_info *info, TestResult *result) {
	int res = OK, ret;
	int fd = -1;

	if (todo == NULL) {
		TS_LOG_ERR("%s production_test_data: No TestToDo specified!! ERROR = %02X\n", __func__, (ERROR_OP_NOT_ALLOW | ERROR_PROD_TEST_DATA));
		return (ERROR_OP_NOT_ALLOW | ERROR_PROD_TEST_DATA);
	}


	TS_LOG_INFO("%s: test starting...\n", __func__);
	struct fts_ts_info *fts_info = fts_get_info();
	fd = request_firmware(&(fts_info->fw),path_limits, fts_info->i2c_cmd_dev);
	TS_LOG_INFO("%s Start to reading %s...:%d\n", __func__, path_limits,fd);
	if(fd != 0) {
		TS_LOG_ERR("%s file not found: ERROR %02X\n", __func__, ERROR_FILE_NOT_FOUND);
		return ERROR_FILE_NOT_FOUND;
	}

	ret = production_test_ms_raw(path_limits, stop_on_fail, todo, info, result);
	res |= ret;
	if (ret < 0) {
		TS_LOG_ERR("%s production_test_data: production_test_ms_raw failed... ERROR = %02X\n", __func__, ret);
		if (stop_on_fail == 1) goto END;
	}

	ret = production_test_ms_cx(path_limits, stop_on_fail, todo, info, result);
	res |= ret;
	if (ret < 0) {
		TS_LOG_ERR("%s production_test_data: production_test_ms_cx failed... ERROR = %02X\n", __func__, ret);
		if (stop_on_fail == 1) goto END;
	}

	ret = production_test_ss_raw(path_limits, stop_on_fail, todo, info,result);
	res |= ret;
	if (ret < 0) {
		TS_LOG_ERR("%s production_test_data: production_test_ss_raw failed... ERROR = %02X\n", __func__, ret);
		if (stop_on_fail == 1) goto END;
	}
END:
	release_firmware(fts_info->fw);
	fts_info->fw = NULL;
	if (res < OK)
		TS_LOG_INFO("%s DATA Production test failed!\n", __func__);
	else
		TS_LOG_INFO("%s DATA Production test finished!\n", __func__);
	return res;
}

int save_mp_flag(u32 signature){
	int res = -1;
	int i;
	u8 cmd[6] = {FTS_CMD_WRITE_MP_FLAG, 0x00, 0x00, 0x00, 0x00, 0x00};

	u32ToU8(signature,&cmd[2]);

	TS_LOG_INFO("%s Starting Saving Flag with signature = %08X ...\n", __func__, signature);

	for(i = 0; i<SAVE_FLAG_RETRY && res<OK; i++){
		TS_LOG_INFO("%s Attempt number %d to save mp flag !\n", __func__, i+1);
		TS_LOG_INFO("%s Command write flag sent...\n", __func__);
		res = fts_writeFwCmd(cmd,6);
		if(res>=OK)
			res = save_cx_tuning();

	}

	if(res<OK){
		TS_LOG_ERR("%s save_mp_flag: ERROR %08X ...\n", __func__, res);
		return res;
	}else{
		TS_LOG_ERR("%s Saving Flag DONE!\n", __func__);
		return OK;
	}
}

int parseProductionTestLimits(char * path, char *label, int **data, int *row, int *column) {

	int find=0;
	char *token=NULL;
	int i = 0;
	int j = 0;
	int z = 0;


	char *line2 = NULL;
	char line[800];
	int fd=-1;
	char *buf = NULL;
	int n,size,pointer=0, ret=OK;
	char *data_file = NULL;
	struct fts_ts_info *info = fts_get_info();
	if(info->fw != NULL){
		size = info->fw->size;
		data_file = (char * )info->fw->data;
		TS_LOG_INFO("%s Start to reading %s...\n", __func__, path);
		TS_LOG_INFO("%s The size of the limits file is %d bytes...\n", __func__, size);

		while (find == 0) {
			//start to look for the wanted label
			if (readLine(&data_file[pointer],line,size-pointer,&n) <0){
				find=-1;
				break;
			}
			pointer+=n;
			if (line[0] == '*') {														//each header row start with *  ex. *label,n_row,n_colum
				line2 = kstrdup(line,GFP_KERNEL);
				if(line2==NULL){
					TS_LOG_ERR("%s parseProductionTestLimits: kstrdup ERROR %02X\n", __func__, ERROR_ALLOC);
					ret = ERROR_ALLOC;
					goto END;
				}
				buf =line2;
				line2 += 1;
				token = strsep(&line2, ",");
				if (strcmp(token, label) == 0) {										//if the row is the wanted one i retrieve rows and columns info
					find = 1;
					token = strsep(&line2, ",");
					if (token != NULL) {
						sscanf(token, "%d", row);
						TS_LOG_INFO("%s Row = %d\n", __func__, *row);
					}
					else {
						TS_LOG_ERR("%s parseProductionTestLimits 1: ERROR %02X\n", __func__, ERROR_FILE_PARSE);
						//release_firmware(fw);
						//return ERROR_FILE_PARSE;
						ret = ERROR_FILE_PARSE;
						goto END;
					}
					token = strsep(&line2, ",");
					if (token != NULL) {
						sscanf(token, "%d", column);
						TS_LOG_INFO("%s Column = %d\n", __func__, *column);
					}
					else {
						TS_LOG_ERR("%s parseProductionTestLimits 2: ERROR %02X\n", __func__, ERROR_FILE_PARSE);
						//release_firmware(fw);
						//return ERROR_FILE_PARSE;
						ret = ERROR_FILE_PARSE;
						goto END;
					}

					kfree(buf);
					buf = NULL;
					*data = (int *)kmalloc(((*row)*(*column))*sizeof(int), GFP_KERNEL);				//allocate the memory for containing the data
					j = 0;
					if (*data == NULL)
					{
						TS_LOG_ERR("%s parseProductionTestLimits: ERROR %02X\n", __func__, ERROR_ALLOC);
						//release_firmware(fw);
						//return ERROR_ALLOC;
						ret = ERROR_ALLOC;
						goto END;
					}


					//start to read the data
					for (i = 0; i < *row; i++) {
						//line =  buf;
						if (readLine(&data_file[pointer], line, size-pointer, &n) < 0) {
							TS_LOG_ERR("%s parseProductionTestLimits : ERROR %02X\n", __func__, ERROR_FILE_READ);
							//release_firmware(fw);
							//return ERROR_FILE_READ;
							ret = ERROR_FILE_READ;
							goto END;
						}
						pointer+=n;
						line2 = kstrdup(line,GFP_KERNEL);
						if(line2==NULL){
							TS_LOG_ERR("%s parseProductionTestLimits: kstrdup ERROR %02X\n", __func__, ERROR_ALLOC);
							ret = ERROR_ALLOC;
							goto END;
						}
						buf = line2;
						token = strsep(&line2, ",");
						for (z = 0; (z < *column) && (token != NULL); z++) {
							sscanf(token, "%d", ((*data) + j));
							j++;
							token = strsep(&line2, ",");
						}
						kfree(buf);
						buf = NULL;
					}
					if (j == ((*row)*(*column))) {												//check that all the data are read
						TS_LOG_INFO("%s READ DONE!\n", __func__);
						//release_firmware(fw);
						//return OK;
						ret = OK;
						goto END;
					}
					TS_LOG_ERR("%s parseProductionTestLimits 3: ERROR %02X\n", __func__, ERROR_FILE_PARSE);
					//release_firmware(fw);
					//return ERROR_FILE_PARSE;
					ret = ERROR_FILE_PARSE;
					goto END;
				}
				kfree(buf);
				buf = NULL;
			}

		}
		TS_LOG_ERR("%s parseProductionTestLimits: ERROR %02X\n", __func__, ERROR_LABEL_NOT_FOUND);
		ret = ERROR_LABEL_NOT_FOUND;
END:
		if(buf!=NULL) kfree(buf);
		return ret;

	}
	else
	{
		TS_LOG_ERR("%s limit file not found: ERROR %02X\n", __func__, ERROR_FILE_NOT_FOUND);
		return ERROR_FILE_NOT_FOUND;
	}


}

int readLine(char * data, char *line, int size, int *n)
{
	int i=0;

	if(size<1)
		return -1;

		while(data[i]!='\n' && i<size){
			line[i] = data[i];
			i++;
	}

	*n=i+1;
	line[i] = '\0';

	return OK;
}

int st_get_rawdata_aftertest(struct ts_rawdata_info *info,u32 signature)
{
	int ret = 0;

	TestToDo todoDefault;//struct used for defining which test perform during the production test
	TestResult *TestRes;
	char file_path[FTS_LIMIT_FILE_NAME_MAX_LEN] = {0};
	char fw_version_info[FTS_LIMIT_FILE_NAME_MAX_LEN] = {0};
	int sensor_len = 0;
	int force_len = 0;
	struct fts_ts_info *fts_info = fts_get_info();

	TS_LOG_INFO("sensor_len is %d, force_len is %d\n", sensor_len, force_len);

	TestRes = kzalloc(sizeof(todoDefault), GFP_KERNEL);
	if (!TestRes) {
		TS_LOG_ERR("%s: out of mem\n", __func__);
		return -ENOMEM;
	}

	memset(&todoDefault, 0, sizeof(TestToDo));

	todoDefault.MutualRaw = 1;		//Mutual rawdata
	todoDefault.MutualRawGap = 1;
	todoDefault.MutualCx2 = 0;		//calibratin data check
	todoDefault.SelfForceRaw = 1;		//tx rawdata
	todoDefault.SelfSenseRaw = 1;		//rx rawdata

	sensor_len = getSenseLen();
	if(sensor_len <= 0) {
		sensor_len = 32; /*default value*/
		TS_LOG_INFO("use default sensor len\n");
	}
	force_len = getForceLen();
	if(force_len <= 0) {
		force_len = 16; /*default value*/
		TS_LOG_INFO("use default sensor len\n");
	}

	info->buff[0] = sensor_len;
	info->buff[1] = force_len;
	info->used_size = 2;

	info->hybrid_buff[0] = sensor_len;
	info->hybrid_buff[1] = force_len;
	info->hybrid_buff_used_size = 2;

	TS_LOG_INFO("%s:sensor_len is %d, force_len is %d\n", __func__, sensor_len, force_len);

	snprintf(file_path, sizeof(file_path), "ts/%s_st_%s_limit.csv",
			fts_info->chip_data->ts_platform_data->product_name,
			fts_info->project_id);
	TS_LOG_INFO("%s:limit file name=%s\n", __func__, file_path);

	ret = production_test_main(file_path, 0, 0, &todoDefault, signature,info, TestRes) ;
	if (TestRes->I2c_Check) {
		strncat(info->result, "0P", strlen("0P"));
	} else {
		strncat(info->result, "0F", strlen("0F"));
	}

	if (TestRes->MutualRawRes){
		strncat(info->result, "-1P", strlen("-1P"));
	} else {
		strncat(info->result, "-1F", strlen("-1F"));
	}

	if (TestRes->MutualRawResGap){
		strncat(info->result, "-2P", strlen("-2P"));
	} else {
		strncat(info->result, "-2F", strlen("-2F"));
	}

	if (TestRes->MutualStrengthRes){
		strncat(info->result, "-3P", strlen("-3P"));
	} else {
		strncat(info->result, "-3F", strlen("-3F"));
	}

	if (TestRes->ITO_Test_Res) {
		strncat(info->result, "-5P", strlen("-5P"));
	} else {
		strncat(info->result, "-5F", strlen("-5F"));
	}
	if (TestRes->SelfSenseRawRes && TestRes->SelfForceRawRes) {
		strncat(info->result, "-6P", strlen("-6P"));
	} else {
		strncat(info->result, "-6F", strlen("-6F"));
	}
	if (TestRes->SelfSenseStrengthData && TestRes->SelfForceStrengthData) {
		strncat(info->result, "-9P", strlen("-9P"));
	} else {
		strncat(info->result, "-9F", strlen("-9F"));
	}
	if (TestRes->SelfSenseData) {
		strncat(info->result, "-10P", strlen("-10P"));
	} else {
		strncat(info->result, "-10F", strlen("-10F"));
	}
	strncat(info->result, TestRes->mutal_raw_res_buf, ST_NP_TEST_RES_BUF_LEN);
	strncat(info->result, TestRes->mutal_noise_res_buf, ST_NP_TEST_RES_BUF_LEN);
	strncat(info->result, TestRes->mutal_cal_res_buf, ST_NP_TEST_RES_BUF_LEN);
	strncat(info->result, ";", strlen(";"));
	if (0 == strlen(info->result) || strstr(info->result, "F")) {
		strncat(info->result, "panel_reason-", strlen("panel_reason-"));
	}
	strncat(info->result, "st-", strlen("st-"));
	strncat(info->result, fts_info->project_id, ST_PROJECT_ID_LEN);
	snprintf(fw_version_info, sizeof(fw_version_info) - 1,
		"-%x.%x", ftsInfo.u16_fwVer, ftsInfo.u16_cfgId);
	strncat(info->result, fw_version_info, strlen(fw_version_info));

	kfree(TestRes);
	return ret;
}
