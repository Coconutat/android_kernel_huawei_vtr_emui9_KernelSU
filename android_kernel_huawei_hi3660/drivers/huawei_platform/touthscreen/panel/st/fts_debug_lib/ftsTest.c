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
//#include <linux/sec_sysfs.h>
#include <../../../huawei_touchscreen_chips.h>

static int node_check = 0;

int computeAdjHoriz(u8* data, int row, int column, u8** result)
{
	int i, j;
	int size = row*(column - 1);

	if (column < 2) {
		TS_LOG_ERR("%s computeAdjHoriz: ERROR % 02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("computeAdjHoriz: invalid number of rows = %d or columns = %d\n", row,column);
		return ERROR_OP_NOT_ALLOW;
	}

	*result = (u8 *)kmalloc(size*sizeof(u8), GFP_KERNEL);
	if (*result == NULL) {
		TS_LOG_ERR("%s computeAdjHoriz: ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	for (i = 0; i < row; i++) {
		for (j = 1; j < column; j++) {
			*(*result + (i*(column-1) + (j-1))) = abs(data[i*column + j] - data[i*column + (j - 1)]);
		}
	}

	return OK;

}

int computeAdjHorizTotal(u16* data, int row, int column, u16** result)
{
	int i, j;
	int size = row*(column - 1);

	if (column < 2) {
		TS_LOG_ERR("%s computeAdjHorizTotal: ERROR % 02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("computeAdjHorizTotal: invalid number of rows = %d or columns = %d\n", row,column);
		return ERROR_OP_NOT_ALLOW;
	}

	*result = (u16 *)kmalloc(size*sizeof(u16), GFP_KERNEL);
	if (*result == NULL) {
		TS_LOG_ERR("%s computeAdjHorizTotal: ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	for (i = 0; i < row; i++) {
		for (j = 1; j < column; j++) {
			*(*result + (i*(column - 1) + (j - 1))) = abs(data[i*column + j] - data[i*column + (j - 1)]);
		}
	}

	return OK;

}

int computeAdjVert(u8* data, int row, int column, u8**result) 
{
	int i, j;
	int size = (row - 1)*(column);

	if (row < 2) {
		TS_LOG_ERR("%s computeAdjVert: ERROR % 02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("computeAdjVert: invalid number of rows = %d or columns = %d\n", row,column);
		return ERROR_OP_NOT_ALLOW;
	}

	*result = (u8 *)kmalloc(size*sizeof(u8), GFP_KERNEL);
	if (*result == NULL) {
		TS_LOG_ERR("%s computeAdjVert: ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	for (i = 1; i < row; i++) {
		for (j = 0; j < column; j++) {
			*(*result + ((i - 1)*column + j)) = abs(data[i*column + j] - data[(i-1)*column + j]);
		}
	}

	return OK;
}

int computeAdjVertTotal(u16* data, int row, int column, u16**result) 
{
	int i, j;
	int size = (row - 1)*(column);

	if (row < 2) {
		TS_LOG_ERR("%s computeAdjVertTotal: ERROR % 02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("computeAdjVertTotal: invalid number of rows = %d or columns = %d\n", row,column);
		return ERROR_OP_NOT_ALLOW;
	}

	*result = (u16 *)kmalloc(size*sizeof(u16), GFP_KERNEL);
	if (*result == NULL) {
		TS_LOG_ERR("%s computeAdjVertTotal: ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	for (i = 1; i < row; i++) {
		for (j = 0; j < column; j++) {
			*(*result + ((i - 1)*column + j)) = abs(data[i*column + j] - data[(i - 1)*column + j]);
		}
	}

	return OK;
}

int computeTotal(u8* data, u8 main, int row, int column, int m, int n, u16**result)
{
	int i, j;
	int size = (row)*(column);

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return -1;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("computeTotal: invalid number of rows = %d or columns = %d\n", row,column);
		return -1;
	}

	*result = (u16 *)kmalloc(size*sizeof(u16), GFP_KERNEL);
	if (*result == NULL) {
		TS_LOG_ERR("%s computeTotal : ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			*(*result + (i*column + j)) = m*main+n*data[i*column + j];
		}
	}

	return OK;
}

int checkLimitsMinMaxU16(u16 *data, int row, int column, int min, int max) 
{
	int i, j;
	int count = 0;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return -1;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("checkLimitsMinMaxU16: invalid number of rows = %d or columns = %d\n", row,column);
		return -1;
	}

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i*column + j]<min || data[i*column + j]>max) {
				TS_LOG_ERR("%s checkLimitsMinMax: Node[%d,%d] = %d exceed limit ]%d, %d[ \n", __func__, i, j, data[i*column + j], min, max);
				count++;
			}
		}
	}

	return count;																	//if count is 0 = OK, test completed succesfully 
}

int checkLimitsMinMaxU8(u8 *data, int row, int column, int min, int max) 
{
	int i, j;
	int count = 0;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return -1;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("checkLimitsMinMaxU8: invalid number of rows = %d or columns = %d\n", row,column);
		return -1;
	}

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i*column + j]<min || data[i*column + j]>max) {
				TS_LOG_ERR("%s checkLimitsMinMax: Node[%d,%d] = %d exceed limit ]%d, %d[ \n", __func__, i, j, data[i*column + j], min, max);
				count++;
			}
		}
	}

	return count;																	//if count is 0 = OK, test completed succesfully 
}

int checkLimitsMinMaxByTab(short *data, int row, int column, int32_t *maxtab, int32_t *mintab)
{
	int i, j;
	int count = 0;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return -1;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("checkLimitsMinMaxByTab: invalid number of rows = %d or columns = %d\n", row,column);
		return -1;
	}

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i*column + j]< mintab[i*column + j] || data[i*column + j]> maxtab[i*column + j]) {
				TS_LOG_ERR("%s checkLimitsMinMax: Node[%d,%d] = %d exceed limit ]%d, %d[ \n", __func__, i, j, data[i*column + j], mintab[i*column + j], maxtab[i*column + j]);
				count++;
			}
		}
	}

	return count;
}

int checkLimitsMinMax(short *data, int row, int column, int min, int max)
{
	int i, j;
	int count = 0;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return -1;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("checkLimitsMinMax: invalid number of rows = %d or columns = %d\n", row,column);
		return -1;
	}

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i*column + j]<min || data[i*column + j]>max) {
				TS_LOG_ERR("%s checkLimitsMinMax: Node[%d,%d] = %d exceed limit ]%d, %d[ \n", __func__, i, j, data[i*column + j], min, max);
				count++;
			}
		}
	}

	return count;																	//if count is 0 = OK, test completed succesfully 
}

int checkLimitsGap(short *data, int row, int column, int threshold)
{
	int i, j;
	int min_node;
	int max_node;

	if (NULL == data) {
		TS_LOG_ERR("data is NULL pointer\n");
		return -1;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("checkLimitsGap: invalid number of rows = %d or columns = %d\n", row,column);
		return -1;
	}

	min_node = data[0];
	max_node = data[0];

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i*column + j]<min_node) {
				min_node = data[i*column + j];
			}
			else {
				if(data[i*column + j]>max_node)
					max_node = data[i*column + j];
			}
		}
	}

	if (max_node - min_node > threshold) {
		TS_LOG_ERR("checkLimitsGap: GAP = %d exceed limit  %d \n", max_node - min_node, threshold);
		return -1;
	} else
		return OK;
}

int checkLimitsMap(u8 *data, int row, int column, int *min, int *max) 
{
	int i, j;
	int count = 0;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return -1;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("checkLimitsMap: invalid number of rows = %d or columns = %d\n", row,column);
		return -1;
	}

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i*column + j]<min[i*column + j] || data[i*column + j]>max[i*column + j]) {
				TS_LOG_ERR("%s checkLimitsMap: Node[%d,%d] = %d exceed limit ]%d, %d[ \n", __func__, i, j, data[i*column + j], min[i*column + j], max[i*column + j]);
				count++;
			}
		}
	}

	return count;																	//if count is 0 = OK, test completed succesfully 
}

int checkLimitsMapTotal(u16 *data, int row, int column, int *min, int *max)
{
	int i, j;
	int count = 0;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return -1;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("checkLimitsMapTotal: invalid number of rows = %d or columns = %d\n", row,column);
		return -1;
	}

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i*column + j]<min[i*column + j] || data[i*column + j]>max[i*column + j]) {
				TS_LOG_ERR("%s checkLimitsMapTotal: Node[%d,%d] = %d exceed limit ]%d, %d[ \n", __func__, i, j, data[i*column + j], min[i*column + j], max[i*column + j]);
				count++;
			}
		}
	}

	return count;																	//if count is 0 = OK, test completed succesfully 
}

int checkLimitsMapAdj(u8 *data, int row, int column, int *max)
{
	int i, j;
	int count = 0;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return -1;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("checkLimitsMapAdj: invalid number of rows = %d or columns = %d\n", row,column);
		return -1;
	}

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i*column + j]>max[i*column + j]) {
				TS_LOG_ERR("%s checkLimitsMapAdj: Node[%d,%d] = %d exceed limit < %d \n", __func__, i, j, data[i*column + j], max[i*column + j]);
				count++;
			}
		}
	}

	return count;																	//if count is 0 = OK, test completed succesfully 
}

int checkLimitsMapAdjTotal(u16 *data, int row, int column, int *max) 
{
	int i, j;
	int count = 0;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return -1;
	}

	if ((row <= 0) || (column <= 0)) {
		TS_LOG_ERR("checkLimitsMapAdjTotal: invalid number of rows = %d or columns = %d\n", row,column);
		return -1;
	}

	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			if (data[i*column + j]>max[i*column + j]) {
				TS_LOG_ERR("%s checkLimitsMapAdjTotal: Node[%d,%d] = %d exceed limit < %d \n", __func__, i, j, data[i*column + j], max[i*column + j]);
				count++;
			}
		}
	}

	return count;//if count is 0 = OK, test completed succesfully 
}

static int production_test_ito(TestResult *result) 
{
	int res=OK;
	u8 cmd;
	u8 readData[FIFO_EVENT_SIZE];
	int eventToSearch[2] = {EVENTID_ERROR_EVENT, EVENT_TYPE_ITO};//look ito event 

	TS_LOG_DEBUG("%s ITO Production test is starting...\n", __func__);

	res = system_reset();
	if (res < 0) {
		TS_LOG_ERR("%s: ERROR %02X \n", __func__, ERROR_PROD_TEST_ITO);
		return (res | ERROR_PROD_TEST_ITO);
	}

	cmd = FTS_CMD_ITO_CHECK;

	TS_LOG_DEBUG("%s ITO Check comand sent \n", __func__);
	if (writeCmd(&cmd, 1) < 0) {
		TS_LOG_ERR("%s: error: %02X \n", __func__, (ERROR_I2C_W | ERROR_PROD_TEST_ITO));
		return (ERROR_I2C_W | ERROR_PROD_TEST_ITO);
	}

	TS_LOG_DEBUG("%s Looking for ITO Event \n", __func__);
	res = pollForEvent(eventToSearch, 2, readData, TIMEOUT_ITO_TEST_RESULT);
	if (res < 0) {
		TS_LOG_ERR("%s: ITO Production test failed error: %02X\n", __func__, ERROR_PROD_TEST_ITO);
		return (res | ERROR_PROD_TEST_ITO);
	}

	if (readData[2] != 0x00 || readData[3] != 0x00) {
		TS_LOG_ERR("%s ITO Production testes finished!FAILED  error: %02X\n", __func__, (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_ITO));
		res = (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_ITO);
		result->ITO_Test_Res = false;
	} else {
		res = OK;
		result->ITO_Test_Res = true;
	}

	return res;
}

static int production_test_initialization(TestResult *result) 
{
	int res;
	u8 cmd;
	u8 readData[FIFO_EVENT_SIZE];
	int eventToSearch[2] = { EVENTID_STATUS_UPDATE, EVENT_TYPE_FULL_INITIALIZATION};

	TS_LOG_DEBUG("%s Production test is starting...\n", __func__);

	res = system_reset();
	if (res < 0) {
		TS_LOG_ERR("%s: error: %02X \n", __func__, ERROR_PROD_TEST_INITIALIZATION);
		result->Init_Res = false;
		return (res | ERROR_PROD_TEST_INITIALIZATION);
	}
#if 0
	TS_LOG_DEBUG("%s INITIALIZATION comand sent \n", __func__);
	cmd = FTS_CMD_FULL_INITIALIZATION;
	if (writeCmd(&cmd, 1) < 0) {
		TS_LOG_ERR("%s: ERROR %02X \n", __func__, (ERROR_I2C_W | ERROR_PROD_TEST_INITIALIZATION));
		return (ERROR_I2C_W | ERROR_PROD_TEST_INITIALIZATION);
	}

	TS_LOG_DEBUG("%s Looking for INITIALIZATION Event \n", __func__);
	res = pollForEvent(eventToSearch, 2, readData,TIMEOUT_INITIALIZATION_TEST_RESULT);
	if (res < 0) {
		TS_LOG_ERR("%s: INITIALIZATION Production test failed error: %02X\n", __func__, ERROR_PROD_TEST_INITIALIZATION);
		return (res | ERROR_PROD_TEST_INITIALIZATION);
	}

	if (readData[2] != 0x00) {
		TS_LOG_ERR("%s INITIALIZATION Production testes finished!FAILED  error: %02X\n", __func__, (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_INITIALIZATION));
		res = (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_INITIALIZATION);
		result->Init_Res = false;
	} else {
		res = OK;
		result->Init_Res = true;
	}
#endif
	result->Init_Res = true;
	return OK;
}

int production_test_main(struct ts_rawdata_info *info, int stop_on_fail, TestToDo *todo, struct production_data_limit *limitdata, TestResult *result) 
{
	int res,ret;

	ret = production_test_data(info, stop_on_fail, todo, limitdata, result);
	if (ret < 0) {
		TS_LOG_ERR("%s Error during production data test! error: %02X\n", __func__, ret);
	} else {
		TS_LOG_INFO("%s production data test OK!\n", __func__);
	}

	res = production_test_initialization(result);
	if (res < 0) {
		TS_LOG_ERR("%s Error during  initialization test! error: %02X\n", __func__, (u8)res);
		if (stop_on_fail) goto END;
	} else {
		TS_LOG_INFO("%s initialization test OK!\n", __func__);
	}

	res = production_test_ito(result);
	if (res < 0) {
		TS_LOG_ERR("%s Error during ITO test! error: %02X\n", __func__, (u8)res);
		goto END; //in case of ITO TEST failure is no sense keep going
	} else {
		TS_LOG_INFO("%s ITO test OK!\n", __func__);
	}

	ret = cleanUp(1);
	if (ret < 0) {
		TS_LOG_ERR("%s clean up error: %02X\n", __func__, ret);
		res |= ret;
		if (stop_on_fail) goto END;
	}

	res |= ret;// the OR is important because if the data test is OK but the inizialization test fail, the main production test result should = FAIL
END:
	if (res < 0) {
		TS_LOG_ERR("%s MAIN Production test finished: FAILED \n", __func__);
		return res;
	} else {
		TS_LOG_INFO("%s MAIN Production test finished: OK\n", __func__);
		return OK;
	}
}

static void print_data_u8(u8 *data, int rows, int columns)
{
	int i, j;
	int count = 0;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return;
	}

	if((rows <= 0) || (columns <= 0)) {
		TS_LOG_ERR("Invalid! row is %d, column is %d\n", rows, columns);
		return;
	}

	for (i = 0; i < rows; i++) {
		for (j = 0; j < columns; j++) {
			printk("\t%u", data[i*columns + j]);
		}
		printk("\n");
	}
}

static void copy_data_from_csv(int32_t *data, int rows, int columns, u16 datatype, int32_t *data_limit)
{
	int i, j;
	int count = 0;
	int k = 0;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return;
	}

	if(NULL == data_limit) {
		TS_LOG_ERR("%s:data_limit is NULL\n", __func__);
		return;
	}

	if((rows <= 0) || (columns <= 0)) {
		TS_LOG_ERR("Invalid! row is %d, column is %d\n", rows, columns);
		return;
	}

	if(MUTUALRAWTYPE == datatype) {
		TS_LOG_INFO("MutualLimitTab:\n");
		for (i = 0; i < (rows-1); i++) {
			for (j = 0; j < (columns-1); j++) {
				data_limit[k] = data[i*columns + j];
				printk("%d	", data_limit[k]);
				k ++;
				if(k > (rows-1)*(columns-1)) {
					TS_LOG_INFO("overflow\n");
					return;
				}
			}
			printk("\n");
		}
	}

	if(SELFFORCERAWTYPE == datatype) {
		TS_LOG_INFO("SelfForceLimitTab:\n");
		for (i = 0; i < rows; i++) {
			data_limit[k] = data[(i+1)*columns - 1];
			printk("%d	", data_limit[k]);
			k ++;
			if(k > (rows -1)) {
				TS_LOG_INFO("overflow\n");
				return;
			}
		}
		printk("\n");
	}

	if(SELFSENSERAWTYPE == datatype) {
		TS_LOG_INFO("SelfSenseLimitTab:\n");
		for (j = 0; j < columns; j++) {
			data_limit[k] = data[(rows-1)*columns + j];
			printk("%d	", data_limit[k]);
			k ++;
			if(k > (columns -1)) {
				TS_LOG_INFO("overflow\n");
				return;
			}
		}
		printk("\n");
	}
}

static void print_data_s16(short *data, int rows, int columns)
{
	int i, j;
	int count = 0;

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
			printk("\t%d", data[i*columns + j]);
		}
		printk("\n");
	}	
}

static void print_data_u16(u16 *data, int rows, int columns)
{
	int i, j;
	int count = 0;

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
			printk("\t%u", data[i*columns + j]);
		}
		printk("\n");
	}	
}

static void st_fill_rawdata_buf(struct ts_rawdata_info *info, short *source_data, int rows, int columns, u16 datatype)
{
	static int offset = 0;
	int i,j,k = 0;
	int *temp_value = NULL;
	int mutual_rows = 0;
	int strength_rows = 0;
	static int tx_num = 0;
	static int rx_num = 0;

	if(NULL == source_data) {
		TS_LOG_ERR("%s:source_data is NULL\n", __func__);
		return;
	}

	if((rows <= 0) || (columns <= 0)) {
		TS_LOG_ERR("%s:Invalid! row is %d, column is %d\n", __func__, rows, columns);
		return;
	}

	temp_value = (int*)kmalloc((rows+1)*(columns+1)*sizeof(int), GFP_KERNEL);
	if(NULL == temp_value) {
		TS_LOG_ERR("temp_value alloc failed\n");
		return;
	}

	if (MUTUALRAWTYPE == datatype) {
		for (i = 0; i <= rows; i++) {
			for (j = 0; j < columns; j++) {
				if (i == rows)
					temp_value[k] = 0;
				else 
					temp_value[k] = source_data[i*columns + j];
				k++;
			}
			temp_value[k] = 0;
			k++;
		}

		tx_num = rows;  //15+1
		rx_num = columns; //26+1
	}

	if (MUTUALRAWTYPE == datatype) {
		memcpy(&info->buff[2], temp_value, (rows+1)*(columns+1)*sizeof(int));
		offset = (rows+1)*(columns+1);
	}

	if (SELFFORCERAWTYPE == datatype) {
		for (i = 0; i < rows; i++) {
			for (j = 0; j < columns; j++) {
				temp_value[k] = source_data[i*columns + j];
				k++;
			}
		}

		for (i = 0; i < rows; i++) {
			info->buff[2+(rx_num)+(rx_num+1)*i] = temp_value[i];
		}
	}

 	if (SELFSENSERAWTYPE == datatype) {
		for (i = 0; i < rows; i++) {
			for (j = 0; j < columns; j++) {
				temp_value[k] = source_data[i*columns + j];
				k++;
			}
		}

		for (j = 0; j < columns; j ++) {
			info->buff[2+(rx_num+1)*tx_num+j] = temp_value[j];
		}
	}

	if (STRENGTHTYPE == datatype) {
		for (i = 0; i <= rows; i++) {
			for (j = 0; j < columns; j++) {
				if (i == rows)
					temp_value[k] = 0;
				else 
					temp_value[k] = source_data[i*columns + j];
				k++;
			}
			temp_value[k] = 0;
			k++;
		}
	}

	if (STRENGTHTYPE == datatype) {
		memcpy(&info->buff[2+offset], temp_value, (rows+1)*(columns+1)*sizeof(int));
	}

	if (temp_value) {
		kfree(temp_value);
		temp_value = NULL;
	}
}

void st_get_rawdata_test(struct ts_rawdata_info *info, struct ts_data *chip_data, int *st_rawdata_limit)
{
	int ret = 0;
	ret = openChannel(chip_data->client);
	TestToDo todoDefault;//struct used for defining which test perform during the production test
	TestResult TestRes;
	struct production_data_limit limit_data;
	int rows_size = 0;
	int columns_size = 0;
	char file_path[64] = {0};
	int sensor_len = 0;
	int force_len = 0;
	int i = 0;

	for(i = 0; i < 20; i ++) {
		TS_LOG_INFO("st_rawdata_limit[%d] = %d\n", i, st_rawdata_limit[i]);
	}

	todoDefault.MutualRaw = 1;		//Mutual rawdata
	todoDefault.MutualCx1 = 0;
	todoDefault.MutualCx2 = 1;
	todoDefault.MutualCxTotal = 0;	//Mutual different cap

	todoDefault.SelfForceRaw = 1;		//tx rawdata
	todoDefault.SelfForceIx1 = 0;
	todoDefault.SelfForceIx2 = 0;
	todoDefault.SelfForceIxTotal = 1;	//tx self different cap
	todoDefault.SelfForceCx1 = 0;
	todoDefault.SelfForceCx2 = 0;
	todoDefault.SelfForceCxTotal = 0;

	todoDefault.SelfSenseRaw = 1;		//rx rawdata
	todoDefault.SelfSenseIx1 = 0;
	todoDefault.SelfSenseIx2 = 0;
	todoDefault.SelfSenseIxTotal = 1;	//rx self different cap
	todoDefault.SelfSenseCx1 = 0;
	todoDefault.SelfSenseCx2 = 0; 
	todoDefault.SelfSenseCxTotal = 0;

	limit_data.MutualRawMax = st_rawdata_limit[0];//5400;
	limit_data.MutualRawMin = st_rawdata_limit[1];//4600;
	limit_data.MutualCx2Max = st_rawdata_limit[2];//36;
	limit_data.MutualCx2Min = st_rawdata_limit[3];//5;
	limit_data.SelfForceRawMax = st_rawdata_limit[4];//10500;
	limit_data.SelfForceRawMin = st_rawdata_limit[5];//7500;
	limit_data.SelfForceIxTotalMax = st_rawdata_limit[6];//170;
	limit_data.SelfForceIxTotalMin = st_rawdata_limit[7];//130;
	limit_data.SelfSenseRawMax = st_rawdata_limit[8];//10500;
	limit_data.SelfSenseRawMin = st_rawdata_limit[9];//7500;
	limit_data.SelfSenseIxTotalMax = st_rawdata_limit[10];//105;
	limit_data.SelfSenseIxTotalMin = st_rawdata_limit[11];//65;

	limit_data.MutualStrengthMin = st_rawdata_limit[12];//-100;
	limit_data.MutualStrengthMax = st_rawdata_limit[13];//100;

	limit_data.MutualRawGapLimit = st_rawdata_limit[14];

	columns_size = PANEL_SENSELEN;
	rows_size = PANEL_FORCELEN;
	TS_LOG_INFO("rows_size:%d, columns_size:%d, totol size(columns_size*rows_size):%d\n", rows_size, columns_size, rows_size*columns_size);

#ifdef BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE
	snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/st/fts.csv");
#else
	snprintf(file_path, sizeof(file_path), "/vendor/firmware/ts/st/fts.csv");
#endif

	limit_data.limit_tab_data.MutualRawMax = (int32_t*)kzalloc((rows_size+1)*(columns_size+1)*sizeof(int32_t), GFP_KERNEL);
	limit_data.limit_tab_data.MutualRawMin = (int32_t*)kzalloc((rows_size+1)*(columns_size+1)*sizeof(int32_t), GFP_KERNEL);
	limit_data.MutualRawMaxTab = (int32_t*)kzalloc(rows_size*columns_size*sizeof(int32_t), GFP_KERNEL);
	limit_data.MutualRawMinTab = (int32_t*)kzalloc(rows_size*columns_size*sizeof(int32_t), GFP_KERNEL);
	limit_data.SelfForceRawMaxTab = (int32_t*)kzalloc(rows_size*sizeof(int32_t), GFP_KERNEL);
	limit_data.SelfForceRawMinTab = (int32_t*)kzalloc(rows_size*sizeof(int32_t), GFP_KERNEL);
	limit_data.SelfSensorRawMaxTab = (int32_t*)kzalloc(columns_size*sizeof(int32_t), GFP_KERNEL);
	limit_data.SelfSensorRawMinTab = (int32_t*)kzalloc(columns_size*sizeof(int32_t), GFP_KERNEL);

	if(limit_data.limit_tab_data.MutualRawMax && limit_data.limit_tab_data.MutualRawMin) {
		ret = ts_parse_csvfile(columns_size+1, rows_size+1, &limit_data.limit_tab_data, file_path);
		if (!ret) {
			if(limit_data.MutualRawMaxTab && limit_data.MutualRawMinTab
				&& limit_data.SelfForceRawMaxTab && limit_data.SelfForceRawMinTab
				&& limit_data.SelfSensorRawMaxTab && limit_data.SelfSensorRawMinTab) {
				copy_data_from_csv(limit_data.limit_tab_data.MutualRawMax,
						rows_size+1, columns_size+1, MUTUALRAWTYPE, limit_data.MutualRawMaxTab);
				copy_data_from_csv(limit_data.limit_tab_data.MutualRawMax,
						rows_size+1, columns_size+1, SELFFORCERAWTYPE, limit_data.SelfForceRawMaxTab);
				copy_data_from_csv(limit_data.limit_tab_data.MutualRawMax,
						rows_size+1, columns_size+1, SELFSENSERAWTYPE, limit_data.SelfSensorRawMaxTab);

				copy_data_from_csv(limit_data.limit_tab_data.MutualRawMin,
						rows_size+1, columns_size+1, MUTUALRAWTYPE, limit_data.MutualRawMinTab);
				copy_data_from_csv(limit_data.limit_tab_data.MutualRawMin,
						rows_size+1, columns_size+1, SELFFORCERAWTYPE, limit_data.SelfForceRawMinTab);
				copy_data_from_csv(limit_data.limit_tab_data.MutualRawMin,
						rows_size+1, columns_size+1, SELFSENSERAWTYPE, limit_data.SelfSensorRawMinTab);
				node_check = 1;
			}
		} else
			node_check = 0;
	}

	ret = production_test_main(info, 0, &todoDefault, &limit_data, &TestRes);
	if (TestRes.Init_Res) {
		strncat(info->result, "0P", strlen("0P"));
	} else {
		strncat(info->result, "0F", strlen("0F"));
		return;
	}

	if (TestRes.MutualRawRes && TestRes.MutualRawResGap && TestRes.SelfForceRawRes && TestRes.SelfSenseRawRes){
		strncat(info->result, "-1P", strlen("-1P"));
	} else {
		strncat(info->result, "-1F", strlen("-1F"));
	}

	if (TestRes.MutualCx2Res & TestRes.SelfForceIxTotalRes && TestRes.SelfSenseIxTotalRes){
		strncat(info->result, "-2P", strlen("-2P"));
	} else {
		strncat(info->result, "-2F", strlen("-2F"));
	}

	if (TestRes.MutualStrengthRes){
		strncat(info->result, "-3P", strlen("-3P"));
	} else {
		strncat(info->result, "-3F", strlen("-3F"));
	}

	if (TestRes.ITO_Test_Res) {
		strncat(info->result, "-4P", strlen("-4P"));
	} else {
		strncat(info->result, "-4F", strlen("-4F"));
	}
	strncat(info->result, TestRes.max_min_aver_buf, strlen(TestRes.max_min_aver_buf));
	strncat(info->result, ";", strlen(";"));
	if (0 == strlen(info->result) || strstr(info->result, "F")) {
		strncat(info->result, "panel_reason-", strlen("panel_reason-"));
	}
	strncat(info->result, "st_touch", strlen("st_touch"));

	sensor_len = getSenseLen();
	if(sensor_len <= 0) {
		TS_LOG_INFO("use default sensor len\n");
		sensor_len = PANEL_SENSELEN;
	}
	force_len = getForceLen();
	if(force_len <= 0) {
		TS_LOG_INFO("use default sensor len\n");
		force_len = PANEL_FORCELEN;
	}

	TS_LOG_INFO("result size is %d, sensor_len is %d, force_len is %d\n", strlen(info->result), sensor_len, force_len);

	info->buff[0] = sensor_len + 1;
	info->buff[1] = force_len + 1;

	info->used_size = (info->buff[0]) * (info->buff[1]) * 2 + 2;

	if(limit_data.limit_tab_data.MutualRawMax) {
		kfree(limit_data.limit_tab_data.MutualRawMax);
		limit_data.limit_tab_data.MutualRawMax = NULL;
	}

	if(limit_data.limit_tab_data.MutualRawMin) {
		kfree(limit_data.limit_tab_data.MutualRawMin);
		limit_data.limit_tab_data.MutualRawMin = NULL;
	}

	if(limit_data.MutualRawMaxTab) {
		kfree(limit_data.MutualRawMaxTab);
		limit_data.MutualRawMaxTab = NULL;
	}

	if(limit_data.MutualRawMinTab) {
		kfree(limit_data.MutualRawMinTab);
		limit_data.MutualRawMinTab = NULL;
	}

	if(limit_data.SelfForceRawMaxTab) {
		kfree(limit_data.SelfForceRawMaxTab);
		limit_data.SelfForceRawMaxTab = NULL;
	}

	if(limit_data.SelfForceRawMinTab) {
		kfree(limit_data.SelfForceRawMinTab);
		limit_data.SelfForceRawMinTab = NULL;
	}

	if(limit_data.SelfSensorRawMaxTab) {
		kfree(limit_data.SelfSensorRawMaxTab);
		limit_data.SelfSensorRawMaxTab = NULL;
	}

	if(limit_data.SelfSensorRawMinTab) {
		kfree(limit_data.SelfSensorRawMinTab);
		limit_data.SelfSensorRawMinTab = NULL;
	}
}

static void get_average_max_min_ms_raw(short *data, int rows, int columns, TestResult *result)
{
	int i, j;
	int ms_raw_max = 0;
	int ms_raw_min = 0;
	int ms_raw_average = 0;
	int ms_raw_total = 0;
	ssize_t remain_size = 0;

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
	for (i = 0; i < rows; i++) {
		for (j = 0; j < columns; j++) {
			if (data[i*columns + j] > ms_raw_max) {
				ms_raw_max = data[i*columns + j];
			}
			if (data[i*columns + j] < ms_raw_min) {
				ms_raw_min = data[i*columns + j];
			}

			ms_raw_total = ms_raw_total + data[i*columns + j];
		}
	}

	ms_raw_average = ms_raw_total/(rows*columns);

	TS_LOG_INFO("ms_raw_max:%d, ms_raw_min:%d, ms_raw_average:%d\n", ms_raw_max, ms_raw_min, ms_raw_average);
	i = strlen(result->max_min_aver_buf);
	if (i >= 50) {
		TS_LOG_ERR("over buf limit, buf size is %ld\n", sizeof(result->max_min_aver_buf));
		return;
	}
	remain_size = sizeof(result->max_min_aver_buf) - i;
	snprintf((result->max_min_aver_buf), PAGE_SIZE, "[%d,%d,%d]", ms_raw_average, ms_raw_max, ms_raw_min);
}

static void get_average_max_min_ms_strength(short *data, int rows, int columns, TestResult *result)
{
	int i, j;
	int ms_strength_max = 0;
	int ms_strength_min = 0;
	int ms_strength_average = 0;
	int ms_strength_total = 0;
	ssize_t remain_size = 0;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return;
	}

	if((rows <= 0) || (columns <= 0)) {
		TS_LOG_ERR("%s:Invalid! row is %d, column is %d\n", __func__, rows, columns);
		return;
	}

	ms_strength_max = data[0];
	ms_strength_min = data[0];
	ms_strength_average = data[0];
	for (i = 0; i < rows; i++) {
		for (j = 0; j < columns; j++) {
			if (data[i*columns + j] > ms_strength_max) {
				ms_strength_max = data[i*columns + j];
			}
			if (data[i*columns + j] < ms_strength_min) {
				ms_strength_min = data[i*columns + j];
			}

			ms_strength_total = ms_strength_total + data[i*columns + j];
		}
	}

	ms_strength_average = ms_strength_total/(rows*columns);
	TS_LOG_INFO("ms_strength_max:%d, ms_strength_min:%d, ms_strength_average:%d\n", ms_strength_max, ms_strength_min, ms_strength_average);
	i = strlen(result->max_min_aver_buf);
	if (i >= 50) {
		TS_LOG_ERR("over buf limit, buf size is %ld\n", sizeof(result->max_min_aver_buf));
		return;
	}
	remain_size = sizeof(result->max_min_aver_buf) - i;
	snprintf((result->max_min_aver_buf + i), sizeof(result->max_min_aver_buf), "[%d,%d,%d]", 
		ms_strength_average, ms_strength_max, ms_strength_min);	
}

static void get_average_max_min_cx2(u8 *data, int rows, int columns, TestResult *result)
{
	int i, j;
	int ms_cx2_max = 0;
	int ms_cx2_min = 0;
	int ms_cx2_average = 0;
	int ms_cx2_total = 0;
	ssize_t remain_size = 0;

	if(NULL == data) {
		TS_LOG_ERR("%s:data is NULL\n", __func__);
		return;
	}

	if((rows <= 0) || (columns <= 0)) {
		TS_LOG_ERR("%s:Invalid! row is %d, column is %d\n", __func__, rows, columns);
		return;
	}

	ms_cx2_max = data[0];
	ms_cx2_min = data[0];
	ms_cx2_average = data[0];
	for (i = 0; i < rows; i++) {
		for (j = 0; j < columns; j++) {
			if (data[i*columns + j] > ms_cx2_max) {
				ms_cx2_max = data[i*columns + j];
			}
			if (data[i*columns + j] < ms_cx2_min) {
				ms_cx2_min = data[i*columns + j];
			}

			ms_cx2_total = ms_cx2_total + data[i*columns + j];
		}
	}

	ms_cx2_average = ms_cx2_total/(rows*columns);
	TS_LOG_INFO("ms_cx2_max:%d, ms_cx2_min:%d, ms_cx2_average:%d\n", ms_cx2_max, ms_cx2_min, ms_cx2_average);
	i = strlen(result->max_min_aver_buf);
	if (i >= 50) {
		TS_LOG_ERR("over buf limit, buf size is %ld\n", sizeof(result->max_min_aver_buf));
		return;
	}

	remain_size = sizeof(result->max_min_aver_buf) - i;
	snprintf((result->max_min_aver_buf + i), remain_size, "[%d,%d,%d]", 
		ms_cx2_average, ms_cx2_max, ms_cx2_min);	
}

int production_test_ms_raw(struct ts_rawdata_info *info, struct production_data_limit *limitdata, TestResult *result)
{
	int ret;
	int ret1 = 0;
	int ret2 = 0;
	int ret3 = 0;
	int ret4 = 0;
	int ret5 = 0;
	short *msRawFrame = NULL;//frame which contains the ms raw data
	short *msStrengthFrame = NULL;
	int rows, columns;

	int thresholds[2];
	int trows, tcolumns;

	thresholds[0] = limitdata->MutualRawMin;
	thresholds[1] = limitdata->MutualRawMax;
	TS_LOG_INFO("%s : MutualRawMax:%d, MutualRawMin:%d \n", __func__, thresholds[0], thresholds[1]);	

	ret = senseOn();//active SenseOn for generating new frames
	if (ret < 0) {
		TS_LOG_ERR("%s senseOn failed error %02X \n", __func__, ERROR_PROD_TEST_DATA);
		return (ret | ERROR_PROD_TEST_DATA);
	}

	mdelay(WAIT_FOR_FRESH_FRAMES);

	ret = senseOff();//send SenseOff for freezing the data of the frame to be read
	if (ret < 0) {
		TS_LOG_ERR("%s senseOn failed error %02X \n", __func__, ERROR_PROD_TEST_DATA);
		ret1 = (ret | ERROR_PROD_TEST_DATA);
	}

	mdelay(WAIT_FOR_FRESH_FRAMES);

	ret = getMSFrame(ADDR_RAW_TOUCH, &msRawFrame, 0);
	if (ret < 0) {
		TS_LOG_ERR("%s : getMSFrame failed error %02X \n", __func__, ERROR_PROD_TEST_DATA);
		ret2 = (ret | ERROR_PROD_TEST_DATA);
	}

	ret = getMSFrame(ADDR_NORM_TOUCH, &msStrengthFrame, 0);
	if (ret < 0) {
		TS_LOG_ERR("%s : getMSFrame failed error %02X \n", __func__, ERROR_PROD_TEST_DATA);
		ret3 = (ret | ERROR_PROD_TEST_DATA);
	}

	columns = getSenseLen();
	rows = ret / columns;//compute the number of row in this way is safer because in general we don't know if the first row is included or not

	print_data_s16(msRawFrame, rows, columns);
	if(node_check){
		ret = checkLimitsMinMaxByTab(msRawFrame, rows, columns, limitdata->MutualRawMaxTab, limitdata->MutualRawMinTab);
	} else {
		ret = checkLimitsMinMax(msRawFrame, rows, columns, thresholds[0], thresholds[1]);
	}
	if (ret != OK) {
		TS_LOG_ERR("%s: checkLimitsMinMax MS raw failed error:%d \n", __func__, ret);
		ret4 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		result->MutualRawRes = false;
	}else {
		TS_LOG_INFO("%s MS raw test:OK \n\n", __func__);
		result->MutualRawRes = true;
	}

	thresholds[0] = limitdata->MutualRawGapLimit;
	TS_LOG_INFO("%s : MutualRawGapLimit:%d\n", __func__, thresholds[0]);

	ret = checkLimitsGap(msRawFrame, rows, columns, thresholds[0]);
	if (ret != OK) {
		TS_LOG_ERR("%s: checkLimitsGap MS raw failed error:%d \n", __func__, ret);
		result->MutualRawResGap = false;
	} else {
		TS_LOG_INFO("%s checkLimitsGap MS raw:OK \n\n", __func__);
		result->MutualRawResGap = true;
	}

	thresholds[0] = limitdata->MutualStrengthMin;
	thresholds[1] = limitdata->MutualStrengthMax;
	TS_LOG_INFO("%s : MutualStrengthMax:%d, MutualStrengthMin:%d \n", __func__, thresholds[0], thresholds[1]);

	print_data_s16(msStrengthFrame, rows, columns);
	ret = checkLimitsMinMax(msStrengthFrame, rows, columns, thresholds[0], thresholds[1]);
	if (ret != OK) {
		TS_LOG_ERR("%s: checkLimitsMinMax MS raw failed error:%d \n", __func__, ret);
		ret5 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		result->MutualStrengthRes = false;
	}else {
		TS_LOG_INFO("%s MS raw test:OK \n\n", __func__);
		result->MutualStrengthRes = true;
	}

	st_fill_rawdata_buf(info, msRawFrame, rows, columns, MUTUALRAWTYPE);
	st_fill_rawdata_buf(info, msStrengthFrame, rows, columns, STRENGTHTYPE);

	get_average_max_min_ms_raw(msRawFrame, rows, columns, result);
	get_average_max_min_ms_strength(msStrengthFrame, rows, columns, result);

	if (msStrengthFrame) {
		kfree(msStrengthFrame);
		msStrengthFrame = NULL;
	}
	if (msRawFrame) {
		kfree(msRawFrame);
		msRawFrame = NULL;
	}
	return ret;
}

int production_test_ms_cx(struct ts_rawdata_info *info, int stop_on_fail, TestToDo *todo, struct production_data_limit *limitdata, TestResult *result)
{
	int ret;
	int ret1 = 0;
	int ret2 = 0;
	int ret3 = 0;
	int ret4 = 0;
	int ret5 = 0;
	int count_fail = 0;
	int thresholds[2] = {0};
	int trows, tcolumns;
	MutualSenseData msCompData;
	u16 container;
	u16 *total_cx = NULL;

	memset(&msCompData, 0, sizeof(msCompData));
	//MS CX TEST
	ret = readMutualSenseCompensationData(MS_TOUCH_ACTIVE, &msCompData);	//read MS compensation data
	if (ret < 0) {
		TS_LOG_ERR("%s: readMutualSenseCompensationData failed... ERROR %02X \n", __func__, ERROR_PROD_TEST_DATA);
		ret1 = (ret | ERROR_PROD_TEST_DATA);
		result->MutualCx2Res = false;
		count_fail += 1;
		goto ms_cx_test_done;
	}

	if (todo->MutualCx1 == 1) {
		TS_LOG_INFO("%s MS CX1 TEST: \n", __func__);
		container = (u16)msCompData.cx1;
		ret = checkLimitsMinMax(&container, 1, 1, thresholds[0], thresholds[1]);//check the limits
		if (ret != OK) {
			TS_LOG_ERR("%s : checkLimitsMinMax MS CX1 failed... ERROR COUNT = %d \n", __func__, ret);
			count_fail += 1;
			if (stop_on_fail)
				ret2 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else
			TS_LOG_INFO("%s MS CX1 TEST:OK \n\n", __func__);
	} else
		TS_LOG_DEBUG("%s MS CX1 TEST:SKIPPED \n\n", __func__);

	if (todo->MutualCx2==1) {
		thresholds[0] = limitdata->MutualCx2Min;
		thresholds[1] = limitdata->MutualCx2Max;
		TS_LOG_INFO("%s MS CX2 TEST: MutualCx2Max:%d, MutualCx2Min:%d\n", __func__, thresholds[0], thresholds[1]);
		/*print node data*/
		print_data_u8(msCompData.node_data, msCompData.header.force_node, msCompData.header.sense_node);
		get_average_max_min_cx2(msCompData.node_data, msCompData.header.force_node, msCompData.header.sense_node, result);
		ret = checkLimitsMinMaxU8(msCompData.node_data, msCompData.header.force_node, msCompData.header.sense_node, thresholds[0], thresholds[1]);				//check the limits
		if (ret != OK) {
			TS_LOG_ERR("%s MS CX2 TEST:FAIL \n\n", __func__);
			result->MutualCx2Res = false;
			count_fail += 1;
			if (stop_on_fail)
				ret3 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else {
			TS_LOG_INFO("%s MS CX2 TEST:OK \n\n", __func__);
			result->MutualCx2Res = true;
		}
	} else
		TS_LOG_DEBUG("%s MS CX2 TEST:SKIPPED \n\n", __func__);

	//START OF TOTAL CHECK
	if (todo->MutualCxTotal == 1) {
		ret = computeTotal(msCompData.node_data, msCompData.cx1, msCompData.header.force_node, msCompData.header.sense_node, CX1_WEIGHT, CX2_WEIGHT, &total_cx);
		if (ret < 0) {
			TS_LOG_ERR("%s: computeTotalCx failed error: %02X \n", __func__, ERROR_PROD_TEST_DATA);
			ret4 = (ret | ERROR_PROD_TEST_DATA);
		}

		ret = checkLimitsMinMaxU16(total_cx, msCompData.header.force_node, msCompData.header.sense_node, thresholds[0], thresholds[1]);				//check the limits
		if (ret != OK) {
			TS_LOG_ERR("%s MS TOTAL CX TEST:FAIL \n\n", __func__);
			count_fail += 1;
			if (stop_on_fail) 
				ret5 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else
			TS_LOG_INFO("%s MS TOTAL CX TEST:OK \n\n", __func__);

	} else
		TS_LOG_DEBUG("%s MS TOTAL CX TEST:SKIPPED \n", __func__);

ms_cx_test_done:
	if(total_cx) {
		TS_LOG_INFO("free memory total_cx\n");
		kfree(total_cx);
		total_cx = NULL;
	}
	if (msCompData.node_data) {
		TS_LOG_INFO("free memory node_data\n");
		kfree(msCompData.node_data);
		msCompData.node_data = NULL;
	}
	if (count_fail == 0) {
		TS_LOG_INFO("%s MS CX testes finished!...OK\n", __func__);
		return OK;
	} else {
		TS_LOG_INFO("%s MS CX testes finished!...FAILED  fails_count = %d\n\n", __func__, count_fail);
		return (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_DATA);
	}
}

int production_test_ss_raw(struct ts_rawdata_info *info, int stop_on_fail, 
	TestToDo *todo, struct production_data_limit *limitdata, TestResult *result) 
{
	int ret;
	int ret1 = 0;
	int ret2 = 0;
	int ret3 = 0;
	int ret4 = 0;
	int ret5 = 0;
	int ret6 = 0;
	int count_fail = 0;
	int rows, columns;
	short *ssRawFrame = NULL;
	int thresholds[2];
	int trows, tcolumns;

	//******************************* Self Sense Test *******************************/

	ret = senseOn();//active SenseOn for generating new frames
	if (ret < 0) {
		TS_LOG_ERR("%s : senseOn failed... ERROR %02X \n", __func__, ERROR_PROD_TEST_DATA);
		ret1 = (ret | ERROR_PROD_TEST_DATA);
	}

	mdelay(WAIT_FOR_FRESH_FRAMES);

	ret = senseOff();//send SenseOff for freezing the data of the frame to be read
	if (ret < 0) {
		TS_LOG_ERR("%s : senseOn failed... ERROR %02X \n", __func__, ERROR_PROD_TEST_DATA);
		ret2 = (ret | ERROR_PROD_TEST_DATA);
	}

	//SS RAW (PROXIMITY) FORCE TEST
	if (todo->SelfForceRaw == 1) {
		thresholds[0] = limitdata->SelfForceRawMin;
		thresholds[1] = limitdata->SelfForceRawMax;
		TS_LOG_INFO("SelfForceRawTest: SelfForceRawMax:%d, SelfForceRawMin:%d\n", thresholds[0], thresholds[1]);

		ret = getSSFrame(ADDR_RAW_PRX_FORCE, &ssRawFrame);
		if (ret < 0) {
			TS_LOG_ERR("%s : getSSFrame RAW FORCE failed... ERROR %02X \n", __func__, ERROR_PROD_TEST_DATA);
			ret3 = (ret | ERROR_PROD_TEST_DATA);
		}
		columns = 1;//there are no data for the sense channels due to the fact that the force frame is analized
		rows = getForceLen();

		print_data_s16(ssRawFrame, rows, columns);

		if(node_check){
			ret = checkLimitsMinMaxByTab(ssRawFrame, rows, columns, limitdata->SelfForceRawMaxTab, limitdata->SelfForceRawMinTab);
		} else {
			ret = checkLimitsMinMax(ssRawFrame, rows, columns, thresholds[0], thresholds[1]);
		}
		if (ret != OK) {
			TS_LOG_INFO("%s SS RAW (PROXIMITY) FORCE TEST:FAIL \n\n", __func__);
			result->SelfForceRawRes = false;
			count_fail += 1;
			if (stop_on_fail)
				ret4 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else {
			TS_LOG_INFO("%s SS RAW (PROXIMITY) FORCE TEST:OK \n\n", __func__);
			result->SelfForceRawRes = true;
		}

		st_fill_rawdata_buf(info, ssRawFrame, rows, columns, SELFFORCERAWTYPE);
		if (ssRawFrame) {
			kfree(ssRawFrame);
			ssRawFrame = NULL;
		}
	}else
		TS_LOG_DEBUG("%s SS RAW (PROXIMITY) FORCE TEST:SKIPPED \n\n", __func__);

	//SS RAW (PROXIMITY) SENSE TEST
	if (todo->SelfSenseRaw==1) {
		thresholds[0] = limitdata->SelfSenseRawMin;
		thresholds[1] = limitdata->SelfSenseRawMax;
		TS_LOG_INFO("SelfSenseRawMax:%d, SelfSenseRawMin:%d\n", thresholds[0], thresholds[1]);

		ret = getSSFrame(ADDR_RAW_PRX_SENSE, &ssRawFrame);
		if (ret < 0) {
			TS_LOG_ERR("%s: getSSFrame  RAW SENSE failed error: %02X \n", __func__, ERROR_PROD_TEST_DATA);
			ret5 = (ret | ERROR_PROD_TEST_DATA);
		}

		columns = getSenseLen();
		rows = 1;// there are no data for the force channels due to the fact that the sense frame is analized

		print_data_s16(ssRawFrame, rows, columns);
		if(node_check){
			ret = checkLimitsMinMaxByTab(ssRawFrame, rows, columns, limitdata->SelfSensorRawMaxTab, limitdata->SelfSensorRawMinTab);
		} else {
			ret = checkLimitsMinMax(ssRawFrame, rows, columns, thresholds[0], thresholds[1]);
		}
		if (ret != OK) {
			TS_LOG_ERR("%s SS RAW (PROXIMITY) SENSE TEST:FAIL \n", __func__);
			result->SelfSenseRawRes = false;
			count_fail += 1;
			if (stop_on_fail)
				ret6 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else {
			TS_LOG_INFO("%s SS RAW (PROXIMITY) SENSE TEST:OK \n", __func__);
			result->SelfSenseRawRes = true;
		}

		st_fill_rawdata_buf(info, ssRawFrame, rows, columns, SELFSENSERAWTYPE);
		if (ssRawFrame) {
			kfree(ssRawFrame);
			ssRawFrame = NULL;
		}
	}else
		TS_LOG_DEBUG("%s SS RAW (PROXIMITY) SENSE TEST:SKIPPED \n", __func__);

	if (count_fail == 0) {
		TS_LOG_INFO("%s SS RAW testes finished!...OK\n\n", __func__);
		return OK;
	} else {
		TS_LOG_ERR("%s SS RAW testes finished!...FAILED  fails_count = %d\n\n", __func__, count_fail);
		return (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_DATA);
	}
}

int production_test_ss_ix_cx(struct ts_rawdata_info *info, int stop_on_fail, TestToDo *todo, struct production_data_limit *limitdata, TestResult *result) 
{
	int ret,ret1,ret2,ret3,ret4,ret5,ret6,ret7,ret8,ret9,ret10,ret11,ret12,ret13,ret14,ret15,ret16,ret17;
	int count_fail = 0;
	int thresholds[2] = {0};
	int trows, tcolumns;
	SelfSenseData ssCompData;
	u16 container;
	u16 *total_ix = NULL;
	u16 *total_cx = NULL;

	memset(&ssCompData, 0, sizeof(ssCompData));
	ret = readSelfSenseCompensationData(SS_TOUCH, &ssCompData);																							//read the SS compensation data
	if (ret < 0) {
		TS_LOG_ERR("%s : readSelfSenseCompensationData failed... error: %02X\n", __func__, ERROR_PROD_TEST_DATA);
		ret1 = (ret | ERROR_PROD_TEST_DATA);
		result->SelfForceIxTotalRes = false;
		count_fail += 1;
		goto ss_ix_cx_test_done;
	}

	//SS IX1 FORCE TEST
	if (todo->SelfForceIx1 == 1) {
		TS_LOG_INFO("%s:SS IX1 FORCE TEST:\n", __func__);
		container = (u16)ssCompData.f_ix1;
		ret = checkLimitsMinMaxU16(&container, 1, 1, thresholds[0], thresholds[1]);				//check the limits
		if (ret != OK) {
			TS_LOG_ERR("%s:checkLimitsMinMax SS IX1 FORCE TEST failed... error: %d\n", __func__, ret);
			count_fail += 1;
			if (stop_on_fail) 
				ret2 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else
			TS_LOG_INFO("%s:SS IX1 FORCE TEST:OK\n", __func__);
	} else
		TS_LOG_DEBUG("%s:SS IX1 FORCE TEST:SKIPPED\n", __func__);

	//SS IX2 FORCE TEST
	if (todo->SelfForceIx2 == 1) {
		TS_LOG_INFO("%s:SS IX2 FORCE TEST:	\n", __func__);
		ret = checkLimitsMinMaxU8(ssCompData.ix2_fm, ssCompData.header.force_node, 1, thresholds[0], thresholds[1]);											//check the values with thresholds
		if (ret != OK) {
			TS_LOG_ERR("%s:SS IX2 FORCE TEST:FAIL \n\n", __func__);
			count_fail += 1;
			if (stop_on_fail) 
				ret3 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else
			TS_LOG_INFO("%s:SS IX2 FORCE TEST:OK \n\n", __func__);
	} else 
		TS_LOG_DEBUG("%s:SS IX2 FORCE TEST:SKIPPED \n\n", __func__);

	//SS TOTAL FORCE IX
	if (todo->SelfForceIxTotal == 1) {
		thresholds[0] = limitdata->SelfForceIxTotalMin;
		thresholds[1] = limitdata->SelfForceIxTotalMax;
		TS_LOG_INFO("%s:SS TOTAL IX FORCE TEST: SelfForceIxTotalMax:%d, SelfForceIxTotalMin:%d\n", __func__, thresholds[0], thresholds[1]);

		ret = computeTotal(ssCompData.ix2_fm, ssCompData.f_ix1, ssCompData.header.force_node, 1, SS_FORCE_IX1_WEIGHT, SS_FORCE_IX2_WEIGHT, &total_ix);
		if (ret < 0) {
			TS_LOG_ERR("%s:computeTotal Ix Force failed. ERROR %02X \n", __func__, ERROR_PROD_TEST_DATA);
			ret4 = (ret | ERROR_PROD_TEST_DATA);
		}
		print_data_u16(total_ix, ssCompData.header.force_node, 1);

		ret = checkLimitsMinMaxU16(total_ix, ssCompData.header.force_node, 1, thresholds[0], thresholds[1]);											//check the values with thresholds
		if (ret != OK) {
			TS_LOG_ERR("%s:SS TOTAL IX FORCE TEST:FAIL \n\n", __func__);
			result->SelfForceIxTotalRes = false;
			count_fail += 1;
			if (stop_on_fail) 
				ret5 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else {
			TS_LOG_INFO("%s:SS TOTAL IX FORCE TEST:OK \n\n", __func__);
			result->SelfForceIxTotalRes = true;
		}
		if (total_ix) {
			kfree(total_ix);
			total_ix = NULL;
		}
	}else
		TS_LOG_DEBUG("%s:SS TOTAL IX FORCE TEST:SKIPPED \n\n", __func__);

	//SS IX1 SENSE TEST
	if (todo->SelfSenseIx1 == 1) {
		TS_LOG_INFO("%s:SS IX1 SENSE TEST:	\n", __func__);

		container = (u16)ssCompData.s_ix1;
		ret = checkLimitsMinMaxU16(&container, 1, 1, thresholds[0], thresholds[1]);//check the limits
		if (ret != OK) {
			TS_LOG_ERR("%s:checkLimitsMinMax SS IX1 SENSE TEST failed... error: %d \n", __func__, ret);
			count_fail += 1;
			if (stop_on_fail) 
				ret6 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else
			TS_LOG_INFO("%s:SS IX1 SENSE TEST:OK \n\n", __func__);
	} else
		TS_LOG_DEBUG("%s:SS IX1 SENSE TEST:SKIPPED \n\n", __func__);

	//SS IX2 SENSE TEST
	if (todo->SelfSenseIx2 == 1) {
		TS_LOG_INFO("%s:SS IX2 SENSE TEST:	\n", __func__);
		ret = checkLimitsMinMaxU8(ssCompData.ix2_sn, 1, ssCompData.header.sense_node, thresholds[0], thresholds[1]);											//check the values with thresholds
		if (ret != OK) {
			TS_LOG_INFO("%s:SS IX2 SENSE TEST:FAIL \n\n", __func__);
			count_fail += 1;
			if (stop_on_fail) 
				ret7 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else
			TS_LOG_INFO("%s:SS IX2 SENSE TEST:OK \n\n", __func__);
	}else
		TS_LOG_DEBUG("%s:SS IX2 SENSE TEST:SKIPPED  \n", __func__);

	//SS TOTAL IX SENSE
	if (todo->SelfSenseIxTotal == 1) {
		thresholds[0] = limitdata->SelfSenseIxTotalMin;
		thresholds[1] = limitdata->SelfSenseIxTotalMax;
		TS_LOG_INFO("%s:SS TOTAL IX SENSE TEST: SelfSenseIxTotalMax:%d, SelfSenseIxTotalMin:%d\n", __func__, thresholds[0], thresholds[1]);

		ret = computeTotal(ssCompData.ix2_sn, ssCompData.s_ix1, 1, ssCompData.header.sense_node, SS_SENSE_IX1_WEIGHT, SS_SENSE_IX2_WEIGHT, &total_ix);
		if (ret < 0) {
			TS_LOG_ERR("%s:computeTotal Ix Sense failed... ERROR %02X \n", __func__, ERROR_PROD_TEST_DATA);
			ret8 = (ret | ERROR_PROD_TEST_DATA);
		}
		print_data_u16(total_ix, 1, ssCompData.header.sense_node);

		ret = checkLimitsMinMaxU16(total_ix, 1, ssCompData.header.sense_node, thresholds[0], thresholds[1]);//check the limits
		if (ret != OK) {
			TS_LOG_ERR("%s: checkLimitsMinMax SS TOTAL IX SENSE failed... error: %d \n", __func__, ret);
			result->SelfSenseIxTotalRes = false;
			count_fail += 1;
			if (stop_on_fail) 
				ret9 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else {
			TS_LOG_INFO("%s:SS CX1 FORCE TEST:OK \n\n", __func__);
			result->SelfSenseIxTotalRes = true;
		}
		if (total_ix) {
			kfree(total_ix);
			total_ix = NULL;
		}
	} else
		TS_LOG_DEBUG("%s:SS TOTAL IX SENSE TEST:SKIPPED  \n", __func__);

	//SS CX1 FORCE TEST
	if (todo->SelfForceCx1 == 1) {
		TS_LOG_INFO("%s:SS CX1 FORCE TEST:\n", __func__);
		container = (u16)ssCompData.f_cx1;

		print_data_u16(&container, 1, 1);

		ret = checkLimitsMinMaxU16(&container, 1, 1, thresholds[0], thresholds[1]);//check the limits
		if (ret != OK) {
			TS_LOG_ERR("%s:checkLimitsMinMax SS CX1 FORCE TEST failed... error: %d \n", __func__, ret);
			count_fail += 1;
			if (stop_on_fail) 
				ret10 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else
			TS_LOG_INFO("%s:SS CX1 FORCE TEST:OK \n\n", __func__);
	} else
		TS_LOG_DEBUG("%s:SS CX1 FORCE TEST:SKIPPED \n\n", __func__);
	
	//SS CX2 FORCE TEST
	if (todo->SelfForceCx2 == 1) {
		TS_LOG_INFO("%s:SS CX2 FORCE TEST:	\n", __func__);
		ret = checkLimitsMinMaxU16(ssCompData.cx2_fm, ssCompData.header.force_node, 1, thresholds[0], thresholds[1]);											//check the values with thresholds
		if (ret != OK) {
			TS_LOG_INFO("%s:SS CX2 FORCE TEST:FAIL \n\n", __func__);
			count_fail += 1;
			if (stop_on_fail) 
				ret11 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else
			TS_LOG_INFO("%s:SS CX2 FORCE TEST:OK \n\n", __func__);
	} else
		TS_LOG_DEBUG("%s:SS CX2 FORCE TEST:SKIPPED \n\n", __func__);

	//SS TOTAL CX FORCE
	if (todo->SelfForceCxTotal == 1) {
		TS_LOG_INFO("%s:SS TOTAL CX FORCE TEST:  \n", __func__);
		ret = computeTotal(ssCompData.cx2_fm, ssCompData.f_cx1, ssCompData.header.force_node, 1, CX1_WEIGHT, CX2_WEIGHT, &total_cx);
		if (ret < 0) {
			TS_LOG_ERR("%s:computeTotal Cx Force failed error: %02X \n", __func__, ERROR_PROD_TEST_DATA);
			ret12 = (ret | ERROR_PROD_TEST_DATA);
		}

		ret = checkLimitsMinMaxU16(total_cx, ssCompData.header.force_node, 1, thresholds[0], thresholds[1]);											//check the values with thresholds
		if (ret != OK) {
			TS_LOG_ERR("%s:SS TOTAL FORCE TEST:FAIL \n\n", __func__);
			count_fail += 1;
			if (stop_on_fail) 
				ret13 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else
			TS_LOG_INFO("%s:SS TOTAL FORCE TEST:OK \n\n", __func__);
		if (total_cx) {
			kfree(total_cx);
			total_cx = NULL;
		}
	} else
		TS_LOG_DEBUG("%s:SS TOTAL FORCE TEST:SKIPPED \n\n", __func__);

	//SS CX1 SENSE TEST
	if (todo->SelfSenseCx1 == 1) {
		TS_LOG_INFO("%s:SS CX1 SENSE TEST:	\n", __func__);
		container = (u16)ssCompData.s_cx1;
		ret = checkLimitsMinMaxU16(&container, 1, 1, thresholds[0], thresholds[1]);//check the limits
		if (ret != OK) {
			TS_LOG_INFO("%s:checkLimitsMinMax SS CX1 SENSE TEST failed... ERROR COUNT = %d \n", __func__, ret);
			count_fail += 1;
			if (stop_on_fail) 
				ret14 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else
			TS_LOG_INFO("%s:SS CX1 SENSE TEST:OK \n\n", __func__);
	} else
		TS_LOG_DEBUG("%s:SS CX1 SENSE TEST:SKIPPED \n\n", __func__);

	//SS CX2 SENSE TEST
	if (todo->SelfSenseCx2 == 1) {
		TS_LOG_INFO("%s:SS CX2 SENSE TEST:	\n", __func__);
		ret = checkLimitsMinMaxU8(ssCompData.cx2_sn, 1, ssCompData.header.sense_node, thresholds[0], thresholds[1]);											//check the values with thresholds
		if (ret != OK) {
			TS_LOG_ERR("%s:SS CX2 SENSE TEST:FAIL \n\n", __func__);
			count_fail += 1;
			if (stop_on_fail) 
				ret15 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else
			TS_LOG_INFO("%s:SS CX2 SENSE TEST:OK \n\n", __func__);
	} else
		TS_LOG_DEBUG("%s:SS CX2 SENSE TEST:SKIPPED \n\n", __func__);

	//SS TOTAL CX SENSE
	if (todo->SelfSenseCxTotal == 1) {
		TS_LOG_INFO("%s:SS TOTAL CX SENSE TEST:  \n", __func__);
		ret = computeTotal(ssCompData.cx2_sn, ssCompData.s_cx1, 1, ssCompData.header.sense_node, CX1_WEIGHT, CX2_WEIGHT, &total_cx);
		if (ret < 0) {
			TS_LOG_INFO("%s:computeTotal Cx Sense failed error: %02X \n", __func__, ERROR_PROD_TEST_DATA);
			ret16 = (ret | ERROR_PROD_TEST_DATA);
		}

		ret = checkLimitsMapTotal(total_cx, 1, ssCompData.header.sense_node, thresholds[0], thresholds[1]);											//check the values with thresholds
		if (ret != OK) {
			TS_LOG_ERR("%s:SS TOTAL CX SENSE TEST:FAIL \n\n", __func__);
			count_fail += 1;
			if (stop_on_fail) 
				ret17 = (ERROR_PROD_TEST_DATA | ERROR_TEST_CHECK_FAIL);
		} else
			TS_LOG_INFO("%s:SS TOTAL CX SENSE TEST:OK \n\n", __func__);
		if (total_cx){
			kfree(total_cx);
			total_cx = NULL;
		}
	} else
		TS_LOG_DEBUG("%s:SS TOTAL CX SENSE TEST:SKIPPED  \n", __func__);

ss_ix_cx_test_done:
	// SS compensation data are no usefull anymore
	if(ssCompData.ix2_fm) {
		TS_LOG_INFO("free ix2_fm\n");
		kfree(ssCompData.ix2_fm);
		ssCompData.ix2_fm = NULL;
	}
	if(ssCompData.ix2_sn) {
		TS_LOG_INFO("free ix2_sn\n");
		kfree(ssCompData.ix2_sn);
		ssCompData.ix2_sn = NULL;
	}
	if(ssCompData.cx2_fm){
		TS_LOG_INFO("free cx2_fm\n");
		kfree(ssCompData.cx2_fm);
		ssCompData.cx2_fm = NULL;
	}
	if(ssCompData.cx2_sn){
		TS_LOG_INFO("free cx2_sn\n");
		kfree(ssCompData.cx2_sn);
		ssCompData.cx2_sn = NULL;
	}

	if (count_fail == 0) {
		TS_LOG_INFO("%s:SS IX CX testes finished!...OK\n\n", __func__);
		return OK;
	} else {
		TS_LOG_ERR("%s:SS IX CX testes finished!...FAILED  fails_count = %d\n\n", __func__, count_fail);
		return (ERROR_TEST_CHECK_FAIL | ERROR_PROD_TEST_DATA);
	}
}

int production_test_data(struct ts_rawdata_info *info, int stop_on_fail, TestToDo *todo, struct production_data_limit *limitdata, TestResult *result) 
{
	int res=OK, ret;
	TestToDo todoDefault;

	if (todo == NULL) {
		TS_LOG_INFO("%s: No TestToDo specified!! ERROR = %02X \n", __func__, (ERROR_OP_NOT_ALLOW|ERROR_PROD_TEST_DATA));
		return (ERROR_OP_NOT_ALLOW|ERROR_PROD_TEST_DATA);
	}

	if (todo->MutualRaw == 1) {
		ret = production_test_ms_raw(info, limitdata, result);
		res |= ret;
		if (ret < 0) {
			TS_LOG_ERR("%s: production_test_ms_raw failed error:%02X \n", __func__, ret);
			if (stop_on_fail == 1) goto END;
		}
	} else
		TS_LOG_DEBUG("%s MS RAW TEST:SKIPPED\n", __func__);

	ret = production_test_ms_cx(info, stop_on_fail, todo, limitdata, result);
	res |= ret;
	if (res < 0 ) {
		TS_LOG_ERR("%s: production_test_ms_cx failed error:%02X \n", __func__, ret);
		if (stop_on_fail == 1) goto END;
	}

	ret = production_test_ss_raw(info, stop_on_fail, todo, limitdata, result);
	res |= ret;
	if (ret < 0 ) {
		TS_LOG_ERR("%s: production_test_ss_raw failed error:%02X \n", __func__, ret);
		if (stop_on_fail == 1) goto END;
	}

	ret = production_test_ss_ix_cx(info, stop_on_fail, todo, limitdata, result);
	res |= ret;
	if (ret < 0 ) {
		TS_LOG_ERR("%s: production_test_ss_ix_cx failed error:%02X \n", __func__, ret);
		if (stop_on_fail == 1) goto END;
	}

END:
	if(res<OK)
		TS_LOG_ERR("%s DATA Production test failed:FAIL!\n", __func__);
	else
		TS_LOG_INFO("%s DATA Production test finished:OK!\n", __func__);
	return res;
}
