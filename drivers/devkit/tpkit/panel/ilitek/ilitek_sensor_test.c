#include "ilitek_sensor_test.h"
#include "ilitek_dts.h"
#define ILITEK_CONNECT_TEST_PASS		"0P-"
#define ILITEK_CONNECT_TEST_FAIL		"0F-"
#define ILITEK_ALLNODE_TEST_PASS		"1P-"
#define ILITEK_ALLNODE_TEST_FAIL		"1F-"
#define ILITEK_OPEN_PASS		"2P-"
#define ILITEK_OPEN_FAIL		"2F-"
#define ILITEK_P2P_TEST_PASS		"3P-"
#define ILITEK_P2P_TEST_FAIL		"3F-"
#define ILITEK_SHORT_TEST_PASS		"4P-"
#define ILITEK_SHORT_TEST_FAIL		"4F-"
#define ILITEK_TX_RX_TEST_PASS		"5P-"
#define ILITEK_TX_RX_TEST_FAIL		"5F-"
#define ILITEK_CHANGERATE_TEST_PASS		"6P"
#define ILITEK_CHANGERATE_TEST_FAIL		"6F"
#define TP_TEST_FAILED_REASON_LEN     20
static char tp_test_failed_reason[TP_TEST_FAILED_REASON_LEN] = { "-software_reason" };
extern struct i2c_data *ilitek_data;
extern int read_product_id(void);

static int i2c_connect = NO_ERR;
static int short_test_result = NO_ERR;
static int open_test_result = NO_ERR;
static int allnode_test_result = NO_ERR;
static int Tx_Rx_delta_test_result = NO_ERR;
static int p2p_noise_test_result = NO_ERR;
static int change_rate_test_result = NO_ERR;

static int ilitek_ready_into_test(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	uint8_t buf[4] = {0};
	int write_reg_len = 0;
	int read_len = 0;
	int write_len = 0;
	cmd[0] = ILITEK_TP_CMD_READ_DATA;
	i2c_connect = NO_ERR;
	write_len = I2C_WRITE_ONE_LENGTH_DATA;
	ret = ilitek_i2c_write(cmd, write_len);
	if(ret < 0){
		i2c_connect = SENSOR_TEST_FAIL;
		TS_LOG_ERR("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		return ret;
	}
	read_len = I2C_READ_THREE_LENGTH_DATA;
	ret=ilitek_i2c_read(cmd, write_reg_len, buf, read_len);
	if(ret < 0){
		i2c_connect =SENSOR_TEST_FAIL;
		TS_LOG_ERR("%s, i2c read error, ret %d\n", __func__, ret);
		return ret;
	}
	if (buf[1] < FW_OK) {
		TS_LOG_INFO("ilitek_ready_into_test IC is not ready buf[1] = 0x%X\n", buf[1]);
	}
	cmd[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
	cmd[1] = ILITEK_TP_CMD_REPORT_STATUS;
	write_len = I2C_WRITE_TWO_LENGTH_DATA;
	ret = ilitek_i2c_write(cmd, write_len);
	if(ret < 0){
		i2c_connect = SENSOR_TEST_FAIL;
		TS_LOG_ERR("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		return ret;
	}

	cmd[0] = ILITEK_TP_CMD_REPORT_STATUS;
	write_len = I2C_WRITE_ONE_LENGTH_DATA;
	ret = ilitek_i2c_write(cmd, write_len);
	if(ret < 0){
		i2c_connect = SENSOR_TEST_FAIL;
		TS_LOG_ERR("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		return ret;
	}
	read_len = I2C_READ_TWO_LENGTH_DATA;
	ret=ilitek_i2c_read(cmd, write_reg_len, buf, read_len);
	if(ret < 0){
		i2c_connect = SENSOR_TEST_FAIL;
		TS_LOG_ERR("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		return ret;
	}
	return i2c_connect;
}

static int ilitek_into_testmode(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	int write_len = 2;
	cmd[0] = ILITEK_TP_CMD_TEST_CMD;
	cmd[1] = FUN_ENABLE;
	ret = ilitek_i2c_write(cmd, write_len);
	if(ret < 0){
		TS_LOG_ERR("%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}
#if 1
int ilitek_into_glove_mode(bool status) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	int write_len = 2;
	TS_LOG_INFO("%s, status = %d\n", __func__, status);
	cmd[0] = ILITEK_TP_CMD_GLOVE;
	if (status) {
		cmd[1] = FUN_ENABLE;
	}
	else {
		cmd[1] = FUN_DISABLE;
	}
	ret = ilitek_i2c_write(cmd, write_len);
	if(ret < 0){
		TS_LOG_INFO("%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}
#endif
#if 1
int ilitek_into_hall_halfmode(bool status) {
	int ret = 0;
	uint8_t cmd[12] = {0};
	int write_len = 2;
	TS_LOG_INFO("ilitek_into_hall_halfmode status = %d\n", status);
	if (status) {
		cmd[0] = ILITEK_TP_CMD_HALL;
		cmd[1] = FUN_DISABLE;
		ret = ilitek_i2c_write(cmd, write_len);
		if(ret < 0){
			TS_LOG_ERR("%s,  err, ret %d\n", __func__, ret);
			return ret;
		}
	}
	else {
		cmd[0] = ILITEK_TP_CMD_HALL;
		cmd[1] = FUN_ENABLE;
		ret = ilitek_i2c_write(cmd, write_len);
		if(ret < 0){
			TS_LOG_ERR("%s,  err, ret %d\n", __func__, ret);
			return ret;
		}
	}
	return 0;
}
#endif

#if 1
int ilitek_into_fingersense_mode(bool status) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	int write_len = 2;
	TS_LOG_INFO("%s, status = %d\n", __func__, status);
	cmd[0] = ILITEK_TP_CMD_FINGERSENSE;
	if (status) {
		cmd[1] = FUN_ENABLE;
	}
	else {
		cmd[1] = FUN_DISABLE;
	}
	ret = ilitek_i2c_write(cmd, write_len);
	if(ret < 0){
		TS_LOG_INFO("%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}
#endif

static int ilitek_gesture_disable_sense_start(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	int write_len = 2;
	cmd[0] = ILITEK_TP_CMD_GESTURE;
	cmd[1] = FUN_DISABLE;
	ret = ilitek_i2c_write(cmd, write_len);
	mdelay(10);
	cmd[0] = ILITEK_TP_CMD_SENSE;
	cmd[1] = FUN_ENABLE;
	ret = ilitek_i2c_write(cmd, write_len);
	mdelay(10);
	return 0;
}

static int ilitek_into_normalmode(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	int write_len = 2;
	cmd[0] = ILITEK_TP_CMD_TEST_CMD;
	cmd[1] = FUN_DISABLE;
	ret = ilitek_i2c_write(cmd, write_len);
	if(ret < 0){
		TS_LOG_ERR("%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}

static int ilitek_short_test(int *short_data1, int *short_data2) {
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0;
	uint8_t cmd[4] = {0};
	int total = ILITEK_SENSOR_TEST_DATA_MIN;
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * ilitek_data->y_ch * ilitek_data->x_ch * 2 + newMaxSize, GFP_KERNEL);
	if(NULL == buf_recv) {
		TS_LOG_ERR("%s, invalid parameter NULL\n", __func__);
		goto MALL0C_ERR;
	}

	int test_32 = (ilitek_data->x_ch * 2) / (newMaxSize - 2);
	if ((ilitek_data->x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	ilitek_data->sensor_test_data_result.short_ave = ILITEK_SENSOR_TEST_DATA_MIN;
	ilitek_data->sensor_test_data_result.short_max= ILITEK_SENSOR_TEST_DATA_MIN;
	ilitek_data->sensor_test_data_result.short_min = ILITEK_SENSOR_TEST_DATA_MAX;
	TS_LOG_INFO("ilitek_short_test test_32 = %d\n", test_32);
	ilitek_data->ilitek_short_threshold = ilitek_short_threshold;
	short_test_result = NO_ERR;
	cmd[0] = ILITEK_TP_CMD_SENSOR_TEST;
	cmd[1] = ILITEK_TP_CMD_SENSOR_TEST_SHORT;
	cmd[2] = ILITEK_TP_CMD_SENSOR_TEST_PARM0;
	ret = ilitek_i2c_write(cmd, 3);
	if(ret < 0){
		TS_LOG_ERR("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		goto TEST_ERR;
	}

	msleep(1);
	ret = ilitek_check_int_high(INT_POLL_LONG_RETRY);
	if (ret != ILITEK_INT_STATUS_HIGH) {
		TS_LOG_ERR("%s, ilitek_check_int_high fail\n", __func__);
	}

	cmd[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
	cmd[1] = ILITEK_TP_CMD_READ_DATA_CONTROL_READ_DATA;
	ret = ilitek_i2c_write(cmd, 2);
	if(ret < 0){
		TS_LOG_ERR("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		goto TEST_ERR;
	}

	cmd[0] = ILITEK_TP_CMD_READ_DATA_CONTROL_READ_DATA;
	ret = ilitek_i2c_write(cmd, 1);
	if(ret < 0){
		TS_LOG_ERR("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		goto TEST_ERR;
	}

	for(i = 0; i < test_32; i++){
		if ((ilitek_data->x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			ret=ilitek_i2c_read(cmd, 0, buf_recv + newMaxSize*i, (ilitek_data->x_ch * 2)%(newMaxSize - 2) + 2);
			if(ret < 0){
				TS_LOG_ERR("%s:%d err,2c read error ret %d\n", __func__,__LINE__,ret);
				goto TEST_ERR;
			}
		}
		else {
			ret=ilitek_i2c_read(cmd, 0, buf_recv + newMaxSize*i, newMaxSize);
			if(ret < 0){
				TS_LOG_ERR("%s:%d err,2c read error ret %d\n", __func__,__LINE__,ret);
				goto TEST_ERR;
			}
		}
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == ilitek_data->x_ch * 2) {
			break;
		}
		for(index = 2; index < newMaxSize; index++) {
			if (j < ilitek_data->x_ch) {
				short_data1[j] = buf_recv[i * newMaxSize + index];
			}
			else {
				short_data2[j - ilitek_data->x_ch] = buf_recv[i * newMaxSize + index];
			}
			j++;
			if (j == ilitek_data->x_ch * 2) {
				break;
			}
		}
	}

	for (i = 0; i < ilitek_data->x_ch; i++) {
		if (abs(short_data1[i] - short_data2[i]) > ilitek_data->ilitek_short_threshold) {
				TS_LOG_ERR("%s:%d err,short test fail index = %d short_data1[%d] = %d  short_data2[%d] = %d  \n",
					__func__,__LINE__,i, i, short_data1[i], i, short_data2[i]);
			short_test_result = SENSOR_TEST_FAIL;
		}
		if (ilitek_data->sensor_test_data_result.short_max < abs(short_data1[i] - short_data2[i])) {
			ilitek_data->sensor_test_data_result.short_max = abs(short_data1[i] - short_data2[i]);
		}
		if (ilitek_data->sensor_test_data_result.short_min > abs(short_data1[i] - short_data2[i])) {
			ilitek_data->sensor_test_data_result.short_min = abs(short_data1[i] - short_data2[i]);
		}
		total += abs(short_data1[i] - short_data2[i]);
	}
	ilitek_data->sensor_test_data_result.short_ave = total / (ilitek_data->x_ch);
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return short_test_result;
TEST_ERR:
	i2c_connect = SENSOR_TEST_FAIL;
MALL0C_ERR:
	short_test_result = SENSOR_TEST_FAIL;
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return short_test_result;
}

static int ilitek_open_test(int *open_data) {
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * ilitek_data->y_ch * ilitek_data->x_ch * 2 + newMaxSize, GFP_KERNEL);

	int test_32 = (ilitek_data->y_ch * ilitek_data->x_ch * 2) / (newMaxSize - 2);
	int total = 0;
	if ((ilitek_data->y_ch * ilitek_data->x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	if(NULL == buf_recv) {
		TS_LOG_ERR("%s, invalid parameter NULL\n", __func__);
		goto MALL0C_ERR;
	}
	ilitek_data->sensor_test_data_result.open_ave = ILITEK_SENSOR_TEST_DATA_MIN;
	ilitek_data->sensor_test_data_result.open_max= ILITEK_SENSOR_TEST_DATA_MIN;
	ilitek_data->sensor_test_data_result.open_min = ILITEK_SENSOR_TEST_DATA_MAX;
	open_test_result = NO_ERR;
	ilitek_data->ilitek_open_threshold = ilitek_open_threshold;
	cmd[0] = ILITEK_TP_CMD_SENSOR_TEST;
	cmd[1] = ILITEK_TP_CMD_SENSOR_TEST_OPEN;
	cmd[2] = ILITEK_TP_CMD_SENSOR_TEST_PARM0;
	ret = ilitek_i2c_write(cmd, 3);
	if(ret < 0){
		TS_LOG_ERR("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		goto TEST_ERR;
	}

	msleep(1);
	ret = ilitek_check_int_high(INT_POLL_LONG_RETRY);
	if (ret != ILITEK_INT_STATUS_HIGH) {
		TS_LOG_ERR("%s, ilitek_check_int_high fail\n", __func__);
	}

	cmd[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
	cmd[1] = ILITEK_TP_CMD_READ_DATA_CONTROL_READ_DATA;
	ret = ilitek_i2c_write(cmd, 2);
	if(ret < 0){
		TS_LOG_ERR("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		goto TEST_ERR;
	}

	cmd[0] = ILITEK_TP_CMD_READ_DATA_CONTROL_READ_DATA;
	ret = ilitek_i2c_write(cmd, 1);
	if(ret < 0){
		TS_LOG_ERR("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		goto TEST_ERR;
	}

	TS_LOG_INFO("ilitek_open_test test_32 = %d\n", test_32);
	for(i = 0; i < test_32; i++){
		if ((ilitek_data->y_ch * ilitek_data->x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			ret=ilitek_i2c_read(cmd, 0, buf_recv + newMaxSize*i, (ilitek_data->y_ch * ilitek_data->x_ch * 2)%(newMaxSize - 2) + 2);
			if(ret < 0){
				TS_LOG_ERR("%s:%d err,i2c read error ret %d\n", __func__,__LINE__,ret);
				goto TEST_ERR;
			}
		}
		else {
			ret=ilitek_i2c_read(cmd, 0, buf_recv + newMaxSize*i, newMaxSize);
			if(ret < 0){
				TS_LOG_ERR("%s:%d err,i2c read error ret %d\n", __func__,__LINE__,ret);
				goto TEST_ERR;
			}
		}
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == ilitek_data->y_ch * ilitek_data->x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			open_data[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < ilitek_data->ilitek_open_threshold) {
				TS_LOG_ERR("%s:%d err,open_test_result error open_data[%d] = %d,  ilitek_data->ilitek_open_threshold = %d\n",
					__func__,__LINE__, j , ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]),  ilitek_data->ilitek_open_threshold);
				open_test_result = SENSOR_TEST_FAIL;
			}
			if (ilitek_data->sensor_test_data_result.open_max < open_data[j]) {
				ilitek_data->sensor_test_data_result.open_max = open_data[j];
			}
			if (ilitek_data->sensor_test_data_result.open_min > open_data[j]) {
				ilitek_data->sensor_test_data_result.open_min = open_data[j];
			}
			total += open_data[j];
			j++;
			if(j % ilitek_data->x_ch == 0) {
			}
			if (j == ilitek_data->y_ch * ilitek_data->x_ch) {
				break;
			}
		}
	}
	ilitek_data->sensor_test_data_result.open_ave = total / ((ilitek_data->x_ch) * (ilitek_data->y_ch));
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return open_test_result;
TEST_ERR:
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	i2c_connect = SENSOR_TEST_FAIL;
MALL0C_ERR:
	open_test_result = SENSOR_TEST_FAIL;
	return open_test_result;
}

static int ilitek_Tx_Rx_test(int *allnode_data, int *allnode_Tx_delta_data, int *allnode_Rx_delta_data) {
	int i = 0;
	int j = 0;
	int index = 0;
	int total = 0;
	TS_LOG_INFO("ilitek allnode Tx delta\n");
	ilitek_data->sensor_test_data_result.Rxdelta_ave= ILITEK_SENSOR_TEST_DATA_MIN;
	ilitek_data->sensor_test_data_result.Rxdelta_max= ILITEK_SENSOR_TEST_DATA_MIN;
	ilitek_data->sensor_test_data_result.Rxdelta_min = ILITEK_SENSOR_TEST_DATA_MAX;

	ilitek_data->sensor_test_data_result.Txdelta_ave= ILITEK_SENSOR_TEST_DATA_MIN;
	ilitek_data->sensor_test_data_result.Txdelta_max= ILITEK_SENSOR_TEST_DATA_MIN;
	ilitek_data->sensor_test_data_result.Txdelta_min = ILITEK_SENSOR_TEST_DATA_MAX;
	ilitek_data->ilitek_tx_cap_max = &ilitek_tx_cap_max[0];
	ilitek_data->ilitek_rx_cap_max = &ilitek_rx_cap_max[0];
	Tx_Rx_delta_test_result = NO_ERR;
	index = 0;
	for (i = 0; i < (ilitek_data->y_ch - 1); i++) {
		for (j = 0; j < ilitek_data->x_ch; j++) {
			allnode_Tx_delta_data[index] = abs(allnode_data[i * ilitek_data->x_ch + j] - allnode_data[(i + 1) * ilitek_data->x_ch + j]);
			if (allnode_Tx_delta_data[index] > ilitek_data->ilitek_tx_cap_max[index]) {
				TS_LOG_ERR("%s:%d err,Tx_Rx_delta_test_result error allnode_Tx_delta_data[%d] = %d,  ilitek_data->ilitek_tx_cap_max[%d] = %d\n",
					__func__,__LINE__, index , allnode_Tx_delta_data[index],  index, ilitek_data->ilitek_tx_cap_max[index]);
				Tx_Rx_delta_test_result = SENSOR_TEST_FAIL;
			}
			if (ilitek_data->sensor_test_data_result.Txdelta_max < allnode_Tx_delta_data[index]) {
				ilitek_data->sensor_test_data_result.Txdelta_max = allnode_Tx_delta_data[index];
			}
			if (ilitek_data->sensor_test_data_result.Txdelta_min > allnode_Tx_delta_data[index]) {
				ilitek_data->sensor_test_data_result.Txdelta_min = allnode_Tx_delta_data[index];
			}
			total += allnode_Tx_delta_data[index];
			index++;
		}
	}
	ilitek_data->sensor_test_data_result.Txdelta_ave = total / ((ilitek_data->x_ch) * (ilitek_data->y_ch - 1));
	total = 0;
	index = 0;
	for (i = 0; i < ilitek_data->y_ch; i++) {
		for (j = 0; j < (ilitek_data->x_ch - 1); j++) {
			allnode_Rx_delta_data[index] = abs(allnode_data[i * ilitek_data->x_ch + j + 1] - allnode_data[i * ilitek_data->x_ch + j]);
			if (allnode_Rx_delta_data[index] > ilitek_data->ilitek_rx_cap_max[index]) {
				TS_LOG_ERR("%s:%d err,Tx_Rx_delta_test_result error allnode_Rx_delta_data[%d] = %d,ilitek_data->ilitek_rx_cap_max[%d] = %d\n",
					__func__,__LINE__, index , allnode_Rx_delta_data[index],  index, ilitek_data->ilitek_rx_cap_max[index]);
				Tx_Rx_delta_test_result = SENSOR_TEST_FAIL;
			}
			if (ilitek_data->sensor_test_data_result.Rxdelta_max < allnode_Rx_delta_data[index]) {
				ilitek_data->sensor_test_data_result.Rxdelta_max = allnode_Rx_delta_data[index];
			}
			if (ilitek_data->sensor_test_data_result.Rxdelta_min > allnode_Rx_delta_data[index]) {
				ilitek_data->sensor_test_data_result.Rxdelta_min = allnode_Rx_delta_data[index];
			}
			total += allnode_Rx_delta_data[index];
			index++;
		}
	}
	ilitek_data->sensor_test_data_result.Rxdelta_ave = total / ( (ilitek_data->x_ch - 1) * ilitek_data->y_ch);
	return Tx_Rx_delta_test_result;
}

static int ilitek_signal_test(int *allnode_signal_data) {
	int ret = NO_ERR;
	int i = 0;
	int j = 0;
	int index = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = NULL;
	int * allnode_data_min = NULL;
	int * allnode_data_max = NULL;
	int test_32 = (ilitek_data->y_ch * ilitek_data->x_ch * ILITEK_TP_DATA_RECEIVE_LEN_MUL2) / (ILITEK_I2C_TRANSFER_MAX_LEN - ILITEK_I2C_SENSOR_TEST_PACKET_HEAD_LEN);
	TS_LOG_INFO("ilitek_signal_test\n");
	if (NULL == allnode_signal_data) {
	    TS_LOG_ERR("%s input param is null\n", __func__);
	    return  -EINVAL;
	}
	buf_recv = kzalloc(sizeof(uint8_t) * ilitek_data->y_ch * ilitek_data->x_ch * ILITEK_TP_DATA_RECEIVE_LEN_MUL2 + ILITEK_I2C_TRANSFER_MAX_LEN, GFP_KERNEL);
	allnode_data_min = kzalloc(sizeof(int) *( ilitek_data->y_ch * ilitek_data->x_ch + ILITEK_I2C_TRANSFER_MAX_LEN), GFP_KERNEL);
	allnode_data_max = kzalloc(sizeof(int) *( ilitek_data->y_ch * ilitek_data->x_ch + ILITEK_I2C_TRANSFER_MAX_LEN), GFP_KERNEL);
	if(NULL == allnode_data_min||NULL == allnode_data_max || NULL == buf_recv) {
		TS_LOG_ERR("%s,  MALLOC ERR NULL\n", __func__);
		goto MALL0C_ERR;
	}
	if ((ilitek_data->y_ch * ilitek_data->x_ch * ILITEK_TP_DATA_RECEIVE_LEN_MUL2) % (ILITEK_I2C_TRANSFER_MAX_LEN - ILITEK_I2C_SENSOR_TEST_PACKET_HEAD_LEN) != 0) {
		test_32 += 1;
	}
	ilitek_data->ilitek_deltarawimage_max = ilitek_deltarawimage_max;
	p2p_noise_test_result = NO_ERR;

	cmd[0] = ILITEK_TP_CMD_SENSOR_TEST;
	cmd[1] = ILITEK_TP_CMD_SENSOR_TEST_P2P;
	cmd[2] = ILITEK_TP_CMD_SENSOR_TEST_PARM0;
	ret = ilitek_i2c_write(cmd, I2C_WRITE_THREE_LENGTH_DATA);
	if(ret < 0){
		TS_LOG_ERR("%s, line %d i2c tranfer error, ret %d\n", __func__, __LINE__,  ret);
		goto TEST_ERR;
	}
	msleep(1);
	ret = ilitek_check_int_high(INT_POLL_LONG_RETRY);
	if (ret != ILITEK_INT_STATUS_HIGH) {
		TS_LOG_ERR("%s, ilitek_check_int_high fail\n", __func__);
	}
	cmd[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
	cmd[1] = ILITEK_TP_CMD_READ_DATA_CONTROL_READ_DATA;
	ret = ilitek_i2c_write(cmd, I2C_WRITE_TWO_LENGTH_DATA);
	if(ret < 0){
		TS_LOG_ERR("%s, line %d i2c tranfer error, ret %d\n", __func__, __LINE__,  ret);
		goto TEST_ERR;
	}
	cmd[0] = ILITEK_TP_CMD_READ_DATA_CONTROL_READ_DATA;
	ret = ilitek_i2c_write(cmd, I2C_WRITE_ONE_LENGTH_DATA);
	if(ret < 0){
		TS_LOG_ERR("%s, line %d i2c tranfer error, ret %d\n", __func__, __LINE__,  ret);
		goto TEST_ERR;
	}
	for(i = 0; i < test_32; i++){
		if ((ilitek_data->y_ch * ilitek_data->x_ch * ILITEK_TP_DATA_RECEIVE_LEN_MUL2)%(ILITEK_I2C_TRANSFER_MAX_LEN - ILITEK_I2C_SENSOR_TEST_PACKET_HEAD_LEN) != 0 && i == test_32 - 1) {
			ret=ilitek_i2c_read(cmd, 0, buf_recv + ILITEK_I2C_TRANSFER_MAX_LEN*i, (ilitek_data->y_ch * ilitek_data->x_ch * ILITEK_TP_DATA_RECEIVE_LEN_MUL2)%(ILITEK_I2C_TRANSFER_MAX_LEN - ILITEK_I2C_SENSOR_TEST_PACKET_HEAD_LEN) + ILITEK_I2C_SENSOR_TEST_PACKET_HEAD_LEN);
			if(ret < 0){
				TS_LOG_ERR("%s, line %d i2c tranfer error, ret %d\n", __func__, __LINE__,  ret);
				goto TEST_ERR;
			}
		}
		else {
			ret=ilitek_i2c_read(cmd, 0, buf_recv + ILITEK_I2C_TRANSFER_MAX_LEN*i, ILITEK_I2C_TRANSFER_MAX_LEN);
			if(ret < 0){
				TS_LOG_ERR("%s, line %d i2c tranfer error, ret %d\n", __func__, __LINE__,  ret);
				goto TEST_ERR;
			}
		}
	}
	for(i = 0; i < test_32; i++) {
		if (j == ilitek_data->y_ch * ilitek_data->x_ch) {
			break;
		}
		for(index = ILITEK_I2C_SENSOR_TEST_PACKET_HEAD_LEN; index < ILITEK_I2C_TRANSFER_MAX_LEN; index += 2) {
			allnode_signal_data[j] = ((buf_recv[i * ILITEK_I2C_TRANSFER_MAX_LEN + index + 1] << 8) + buf_recv[i * ILITEK_I2C_TRANSFER_MAX_LEN + index])  - ILITEK_TP_SIGNAL_DATA_BASE;
			if (allnode_signal_data[j] > ilitek_data->ilitek_deltarawimage_max) {
			TS_LOG_ERR("%s:%d err,p2p_noise_test_result error allnode_p2p_data[%d] = %d,ilitek_data->ilitek_deltarawimage_max=%d \n",
				__func__,__LINE__, j , allnode_signal_data[j], ilitek_data->ilitek_deltarawimage_max);
				p2p_noise_test_result = SENSOR_TEST_FAIL;
			}
			j++;
			if (j == ilitek_data->y_ch * ilitek_data->x_ch) {
				break;
			}
		}
	}

	if (allnode_data_min) {
		kfree(allnode_data_min);
		allnode_data_min = NULL;
	}
	if (allnode_data_max) {
		kfree(allnode_data_max);
		allnode_data_max = NULL;
	}
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return p2p_noise_test_result;
TEST_ERR:
	i2c_connect = SENSOR_TEST_FAIL;
MALL0C_ERR:
	p2p_noise_test_result = SENSOR_TEST_FAIL;
	if (allnode_data_min) {
		kfree(allnode_data_min);
		allnode_data_min = NULL;
	}
	if (allnode_data_max) {
		kfree(allnode_data_max);
		allnode_data_max = NULL;
	}
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return p2p_noise_test_result;
}

static int ilitek_p2p_test(int *allnode_p2p_data) {
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0;
	int frame_count = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * ilitek_data->y_ch * ilitek_data->x_ch * 2 + newMaxSize, GFP_KERNEL);
	int * allnode_data_min = kzalloc(sizeof(int) *( ilitek_data->y_ch * ilitek_data->x_ch + newMaxSize), GFP_KERNEL);
	int * allnode_data_max = kzalloc(sizeof(int) *( ilitek_data->y_ch * ilitek_data->x_ch + newMaxSize), GFP_KERNEL);
	int total = ILITEK_SENSOR_TEST_DATA_MIN;
	int test_32 = (ilitek_data->y_ch * ilitek_data->x_ch * 2) / (newMaxSize - 2);
	if(NULL == allnode_data_min||NULL == allnode_data_max || NULL == buf_recv) {
		TS_LOG_ERR("%s,  MALLOC ERR NULL\n", __func__);
		goto MALL0C_ERR;
	}
	if ((ilitek_data->y_ch * ilitek_data->x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	TS_LOG_INFO("ilitek_p2p_test\n");
	ilitek_data->sensor_test_data_result.noise_ave= ILITEK_SENSOR_TEST_DATA_MIN;
	ilitek_data->sensor_test_data_result.noise_max= ILITEK_SENSOR_TEST_DATA_MIN;
	ilitek_data->sensor_test_data_result.noise_min = ILITEK_SENSOR_TEST_DATA_MAX;
	ilitek_data->ilitek_deltarawimage_max = ilitek_deltarawimage_max;
	p2p_noise_test_result = NO_ERR;

	for (i = 0; i < ilitek_data->y_ch * ilitek_data->x_ch; i++) {
		allnode_data_min[i] = 0xFFFF;
		allnode_data_max[i] = 0;
	}
	for (frame_count = 0; frame_count < 10; frame_count++) {
		j = 0;
		cmd[0] = ILITEK_TP_CMD_SENSOR_TEST;
		cmd[1] = ILITEK_TP_CMD_SENSOR_TEST_P2P;
		cmd[2] = ILITEK_TP_CMD_SENSOR_TEST_PARM0;
		ret = ilitek_i2c_write(cmd, 3);
		if(ret < 0){
			TS_LOG_ERR("%s, line %d i2c tranfer error, ret %d\n", __func__, __LINE__,  ret);
			goto TEST_ERR;
		}
		msleep(1);
		ret = ilitek_check_int_high(INT_POLL_LONG_RETRY);
		if (ret != ILITEK_INT_STATUS_HIGH) {
			TS_LOG_ERR("%s, ilitek_check_int_high fail\n", __func__);
		}
		cmd[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
		cmd[1] = ILITEK_TP_CMD_READ_DATA_CONTROL_READ_DATA;
		ret = ilitek_i2c_write(cmd, 2);
		if(ret < 0){
			TS_LOG_ERR("%s, line %d i2c tranfer error, ret %d\n", __func__, __LINE__,  ret);
			goto TEST_ERR;
		}
		cmd[0] = ILITEK_TP_CMD_READ_DATA_CONTROL_READ_DATA;
		ret = ilitek_i2c_write(cmd, 1);
		if(ret < 0){
			TS_LOG_ERR("%s, line %d i2c tranfer error, ret %d\n", __func__, __LINE__,  ret);
			goto TEST_ERR;
		}
		for(i = 0; i < test_32; i++){
			if ((ilitek_data->y_ch * ilitek_data->x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
				ret=ilitek_i2c_read(cmd, 0, buf_recv + newMaxSize*i, (ilitek_data->y_ch * ilitek_data->x_ch * 2)%(newMaxSize - 2) + 2);
				if(ret < 0){
					TS_LOG_ERR("%s, line %d i2c tranfer error, ret %d\n", __func__, __LINE__,  ret);
					goto TEST_ERR;
				}
			}
			else {
				ret=ilitek_i2c_read(cmd, 0, buf_recv + newMaxSize*i, newMaxSize);
				if(ret < 0){
					TS_LOG_ERR("%s, line %d i2c tranfer error, ret %d\n", __func__, __LINE__,  ret);
					goto TEST_ERR;
				}
			}
		}
		for(i = 0; i < test_32; i++) {
			if (j == ilitek_data->y_ch * ilitek_data->x_ch) {
				break;
			}
			for(index = 2; index < newMaxSize; index += 2) {
				if (allnode_data_max[j] < ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index])) {
					allnode_data_max[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
				}
				if (allnode_data_min[j] > ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index])) {
					allnode_data_min[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
				}
				j++;
				if (j == ilitek_data->y_ch * ilitek_data->x_ch) {
					break;
				}
			}
		}
	}

	index = 0;
	for (i = 0; i < ilitek_data->y_ch * ilitek_data->x_ch; i++) {
		allnode_p2p_data[index] = allnode_data_max[i] - allnode_data_min[i];
		if (allnode_p2p_data[index] > ilitek_data->ilitek_deltarawimage_max) {
		TS_LOG_ERR("%s:%d err,p2p_noise_test_result error allnode_p2p_data[%d] = %d,ilitek_data->ilitek_deltarawimage_max=%d \n",
			__func__,__LINE__, index , allnode_p2p_data[index], ilitek_data->ilitek_deltarawimage_max);
			p2p_noise_test_result = SENSOR_TEST_FAIL;
		}
		if (ilitek_data->sensor_test_data_result.noise_max < allnode_p2p_data[index]) {
			ilitek_data->sensor_test_data_result.noise_max = allnode_p2p_data[index];
		}
		if (ilitek_data->sensor_test_data_result.noise_min > allnode_p2p_data[index]) {
			ilitek_data->sensor_test_data_result.noise_min = allnode_p2p_data[index];
		}
		total += allnode_p2p_data[index];
		index++;
	}
	ilitek_data->sensor_test_data_result.noise_ave = total / ((ilitek_data->x_ch) * (ilitek_data->y_ch));
	if (allnode_data_min) {
		kfree(allnode_data_min);
		allnode_data_min = NULL;
	}
	if (allnode_data_max) {
		kfree(allnode_data_max);
		allnode_data_max = NULL;
	}
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return p2p_noise_test_result;
TEST_ERR:
	i2c_connect = SENSOR_TEST_FAIL;
MALL0C_ERR:
	p2p_noise_test_result = SENSOR_TEST_FAIL;
	if (allnode_data_min) {
		kfree(allnode_data_min);
		allnode_data_min = NULL;
	}
	if (allnode_data_max) {
		kfree(allnode_data_max);
		allnode_data_max = NULL;
	}
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return p2p_noise_test_result;
}

static int ilitek_allnode_test(int *allnode_data) {
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * ilitek_data->y_ch * ilitek_data->x_ch * 2 + newMaxSize, GFP_KERNEL);
	int total = 0;
	int test_32 = (ilitek_data->y_ch * ilitek_data->x_ch * 2) / (newMaxSize - 2);
	if(NULL == buf_recv) {
		TS_LOG_ERR("%s, invalid parameter NULL\n", __func__);
		goto MALL0C_ERR;
	}
	if ((ilitek_data->y_ch * ilitek_data->x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	ilitek_data->sensor_test_data_result.allnodedata_ave = ILITEK_SENSOR_TEST_DATA_MIN;
	ilitek_data->sensor_test_data_result.allnodedata_max= ILITEK_SENSOR_TEST_DATA_MIN;
	ilitek_data->sensor_test_data_result.allnodedata_min = ILITEK_SENSOR_TEST_DATA_MAX;

	ilitek_data->ilitek_full_raw_min_cap = &ilitek_full_raw_min_cap[0];
	ilitek_data->ilitek_full_raw_max_cap = &ilitek_full_raw_max_cap[0];
	TS_LOG_INFO("ilitek_allnode_test test_32 = %d\n", test_32);
	allnode_test_result = NO_ERR;
	cmd[0] = ILITEK_TP_CMD_SENSOR_TEST;
	cmd[1] = ILITEK_TP_CMD_SENSOR_TEST_ALLNODE;
	ret = ilitek_i2c_write(cmd, 2);
	if(ret < 0){
		TS_LOG_ERR("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
		goto TEST_ERR;
	}
	msleep(1);
	ret = ilitek_check_int_high(INT_POLL_LONG_RETRY);
	if (ret != ILITEK_INT_STATUS_HIGH) {
		TS_LOG_ERR("%s, ilitek_check_int_high fail\n", __func__);
	}
	cmd[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
	cmd[1] = ILITEK_TP_CMD_READ_DATA_CONTROL_READ_DATA;
	ret = ilitek_i2c_write(cmd, 2);
	if(ret < 0){
		TS_LOG_ERR("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
		goto TEST_ERR;
	}

	cmd[0] = ILITEK_TP_CMD_READ_DATA_CONTROL_READ_DATA;
	ret = ilitek_i2c_write(cmd, 1);
	if(ret < 0){
		TS_LOG_ERR("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
		goto TEST_ERR;
	}

	for(i = 0; i < test_32; i++){
		if ((ilitek_data->y_ch * ilitek_data->x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			ret=ilitek_i2c_read(cmd, 0, buf_recv + newMaxSize*i, (ilitek_data->y_ch * ilitek_data->x_ch * 2)%(newMaxSize - 2) + 2);
			if(ret < 0){
				TS_LOG_ERR("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
				goto TEST_ERR;
			}
		}
		else {
			ret=ilitek_i2c_read(cmd, 0, buf_recv + newMaxSize*i, newMaxSize);
			if(ret < 0){
				TS_LOG_ERR("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
				goto TEST_ERR;
			}
		}
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == ilitek_data->y_ch * ilitek_data->x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			allnode_data[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			if ( (allnode_data[j] < ilitek_data->ilitek_full_raw_min_cap[j]) ||(allnode_data[j] > ilitek_data->ilitek_full_raw_max_cap[j]) ) {
				TS_LOG_ERR("%s:%d err,allnode_test_result error allnode_data[%d] = %d, ilitek_data->ilitek_full_raw_min_cap[%d] = %d ilitek_data->ilitek_full_raw_max_cap[%d] = %d\n",
					__func__,__LINE__, j , allnode_data[j], j, ilitek_data->ilitek_full_raw_min_cap[j], j, ilitek_data->ilitek_full_raw_max_cap[j]);
				allnode_test_result = SENSOR_TEST_FAIL;
			}
			if (ilitek_data->sensor_test_data_result.allnodedata_max < allnode_data[j]) {
				ilitek_data->sensor_test_data_result.allnodedata_max = allnode_data[j];
			}
			if (ilitek_data->sensor_test_data_result.allnodedata_min > allnode_data[j]) {
				ilitek_data->sensor_test_data_result.allnodedata_min = allnode_data[j];
			}
			total += allnode_data[j];
			j++;

			if (j == ilitek_data->y_ch * ilitek_data->x_ch) {
				break;
			}
		}
	}
	ilitek_data->sensor_test_data_result.allnodedata_ave = total / (ilitek_data->x_ch * ilitek_data->y_ch);

	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return allnode_test_result;
TEST_ERR:
	i2c_connect = SENSOR_TEST_FAIL;
MALL0C_ERR:
	allnode_test_result = SENSOR_TEST_FAIL;
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return allnode_test_result;
}

static int ilitek_change_report_rate_test(void)
{
	uint8_t cmd[4] = {0};
	uint8_t buf[4] = {0};
	int ret = 0;
	cmd[0] = SENSOR_TEST_SET_CDC_INITIAL;
	cmd[1] = SENSOR_TEST_TRCRQ_TRCST_TESET;
	cmd[2] = SENSOR_TEST_COMMAND;
	change_rate_test_result = NO_ERR;
	ret = ilitek_i2c_write(cmd, 3);
	if(ret < 0){
		TS_LOG_ERR("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
		goto TEST_ERR;
	}

	msleep(1);
	ret = ilitek_check_int_high(INT_POLL_LONG_RETRY);
	if (ret != ILITEK_INT_STATUS_HIGH) {
		TS_LOG_ERR("%s, ilitek_check_int_high fail\n", __func__);
	}
	cmd[0] = SENSOR_TEST_TEAD_DATA_SELECT_CONTROL;
	cmd[1] = SENSOR_TEST_GET_CDC_RAW_DATA;
	ret = ilitek_i2c_write(cmd, 2);
	if(ret < 0){
		TS_LOG_ERR("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
		goto TEST_ERR;
	}

	cmd[0] = SENSOR_TEST_GET_CDC_RAW_DATA;
	ret = ilitek_i2c_write(cmd, 1);
	if(ret < 0){
		TS_LOG_ERR("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
		goto TEST_ERR;
	}
	ret=ilitek_i2c_read(cmd, 0, buf, 1);
	if(ret < 0){
		TS_LOG_ERR("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
		goto TEST_ERR;
	}
	TS_LOG_INFO("%s:change_rate_test_result status = 0x%x\n", __func__,buf[0]);

	if (buf[0] == CHANGE_REPORT_RATE_RESULT_SUCCESS) {
		change_rate_test_result = CHANGE_REPORT_RATE_SUCCESS;
	}
	else if (buf[0] == CHANGE_REPORT_RATE_RESULT_60HZ_FAIL) {
		change_rate_test_result = CHANGE_REPORT_RATE_60HZ_FAIL;
	}
	else if (buf[0] == CHANGE_REPORT_RATE_RESULT_120HZ_FAIL) {
		change_rate_test_result = CHANGE_REPORT_RATE_120HZ_FAIL;
	}
	else {
		change_rate_test_result = CHANGE_REPORT_RATE_FAIL;
	}
	return change_rate_test_result;
TEST_ERR:
	i2c_connect = SENSOR_TEST_FAIL;
	change_rate_test_result = SENSOR_TEST_FAIL;
	return change_rate_test_result;
}

unsigned char *ilitek_roi_rawdata(void)
{
	int i = 0;
	TS_LOG_DEBUG("%s:", __func__);
	mutex_lock(&(ilitek_data->roi_mutex));
	for (i = 0; i < (ROI_DATA_LENGTH + 4); i++) {
		 ilitek_data->ilitek_roi_data_send[i] = ilitek_data->ilitek_roi_data[i];
		TS_LOG_DEBUG("%s: roi_data[%d] = 0x%X", __func__, (i), ilitek_data->ilitek_roi_data_send[i]);
	}
	mutex_unlock(&(ilitek_data->roi_mutex));
	return (unsigned char *) ilitek_data->ilitek_roi_data_send;
}
int ilitek_into_charger_mode(bool status) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	TS_LOG_INFO("%s, status = %d\n", __func__, status);
	return 0;
}
static int ilitek_set_charger_switch(u8 charger_switch)
{
	int error = NO_ERR;
	int value = 0;
	TS_LOG_INFO("%s:", __func__);

	if (ilitek_data->firmware_updating){
		TS_LOG_ERR("%s: tp fw is updating,return\n", __func__);
		return -EINVAL;
	}

	if (charger_switch < 0) {
		TS_LOG_ERR("%s: roi_switch value %d is invalid\n", __func__, charger_switch);
		return -EINVAL;
	}

	if (charger_switch) {
		error =ilitek_into_charger_mode(true);
	}
	else {
		error =ilitek_into_charger_mode(false);
	}
	return error;
}
int ilitek_charger_switch(struct ts_charger_info *info)
{
	int retval = NO_ERR;
	TS_LOG_INFO("%s:", __func__);
#if defined(HUAWEI_CHARGER_FB)
	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		retval = ilitek_set_charger_switch(info->charger_switch);
		if (retval < 0) {
			TS_LOG_ERR("set charger switch(%d), failed: %d\n",
				   info->charger_switch, retval);
		}
		break;
	default:
		TS_LOG_INFO("%s, invalid cmd\n", __func__);
		retval = -EINVAL;
		break;
	}
#endif

	return retval;
}

void ilitek_set_sensor_test_result(struct ts_rawdata_info *info) {
	int str_cat_len = 32;
	char tmp_char[32] = {0};
	if (!i2c_connect) {
		ilitek_strncat(info->result, ILITEK_CONNECT_TEST_PASS, TS_RAWDATA_RESULT_MAX);
	}
	else {
		ilitek_strncat(info->result, ILITEK_CONNECT_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
	}
	if (!allnode_test_result) {
		ilitek_strncat(info->result, ILITEK_ALLNODE_TEST_PASS, TS_RAWDATA_RESULT_MAX);
	}
	else {
		ilitek_strncat(info->result, ILITEK_ALLNODE_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
	}
	if (!open_test_result) {
		ilitek_strncat(info->result, ILITEK_OPEN_PASS, TS_RAWDATA_RESULT_MAX);
	}
	else {
		ilitek_strncat(info->result, ILITEK_OPEN_FAIL, TS_RAWDATA_RESULT_MAX);
	}
	if (!p2p_noise_test_result) {
		ilitek_strncat(info->result, ILITEK_P2P_TEST_PASS, TS_RAWDATA_RESULT_MAX);
	}
	else {
		ilitek_strncat(info->result, ILITEK_P2P_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
	}
	if (!short_test_result) {
		ilitek_strncat(info->result, ILITEK_SHORT_TEST_PASS, TS_RAWDATA_RESULT_MAX);
	}
	else {
		ilitek_strncat(info->result, ILITEK_SHORT_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
	}
	if (!Tx_Rx_delta_test_result) {
		ilitek_strncat(info->result, ILITEK_TX_RX_TEST_PASS, TS_RAWDATA_RESULT_MAX);
	}
	else {
		ilitek_strncat(info->result, ILITEK_TX_RX_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
	}
	if (!change_rate_test_result) {
		ilitek_strncat(info->result, ILITEK_CHANGERATE_TEST_PASS, TS_RAWDATA_RESULT_MAX);
	}
	else {
		ilitek_strncat(info->result, ILITEK_CHANGERATE_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
	}
	snprintf(tmp_char, str_cat_len, "[%d,%d,%d]", ilitek_data->sensor_test_data_result.allnodedata_ave,
		ilitek_data->sensor_test_data_result.allnodedata_max, ilitek_data->sensor_test_data_result.allnodedata_min);
	ilitek_strncat(info->result, tmp_char, TS_RAWDATA_RESULT_MAX);
	snprintf(tmp_char, str_cat_len, "[%d,%d,%d]", ilitek_data->sensor_test_data_result.noise_ave,
		ilitek_data->sensor_test_data_result.noise_max, ilitek_data->sensor_test_data_result.noise_min);
	ilitek_strncat(info->result, tmp_char, TS_RAWDATA_RESULT_MAX);
	snprintf(tmp_char, str_cat_len, "[%d,%d,%d]", ilitek_data->sensor_test_data_result.short_ave,
		ilitek_data->sensor_test_data_result.short_max, ilitek_data->sensor_test_data_result.short_min);
	ilitek_strncat(info->result, tmp_char, TS_RAWDATA_RESULT_MAX);
	snprintf(tmp_char, str_cat_len, "[%d,%d,%d]", ilitek_data->sensor_test_data_result.Txdelta_ave,
		ilitek_data->sensor_test_data_result.Txdelta_max, ilitek_data->sensor_test_data_result.Txdelta_min);
	ilitek_strncat(info->result, tmp_char, TS_RAWDATA_RESULT_MAX);
	snprintf(tmp_char, str_cat_len, "[%d,%d,%d]", ilitek_data->sensor_test_data_result.Rxdelta_ave,
		ilitek_data->sensor_test_data_result.Rxdelta_max, ilitek_data->sensor_test_data_result.Rxdelta_min);
	ilitek_strncat(info->result, tmp_char, TS_RAWDATA_RESULT_MAX);
	ilitek_strncat(info->result, ";",  strlen(";"));

	if (i2c_connect) {
		ilitek_strncat(info->result, "-software_reason-", TS_RAWDATA_RESULT_MAX);
	}
	else if ((allnode_test_result) || (open_test_result) || (p2p_noise_test_result) || (short_test_result) || (Tx_Rx_delta_test_result) || (change_rate_test_result)) {
		ilitek_strncat(info->result, "-panel_reason-", TS_RAWDATA_RESULT_MAX);
	}
	ilitek_strncat(info->result, ilitek_data->ilitek_chip_data->chip_name, TS_RAWDATA_RESULT_MAX);
	ilitek_strncat(info->result, "-", TS_RAWDATA_RESULT_MAX);
	ilitek_strncat(info->result, ilitek_data->product_id, TS_RAWDATA_RESULT_MAX);
}
 int ilitek_get_raw_data(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd)
{
	int i = 0;
	int j = 0;
	int ret = -1;
	unsigned char buf[4] = {0};
	int buff_index = 0;
	int * short_data1 = kzalloc(sizeof(int) * (ilitek_data->x_ch), GFP_KERNEL);
	int * short_data2 = kzalloc(sizeof(int) * (ilitek_data->x_ch), GFP_KERNEL);
	int * open_data = kzalloc(sizeof(int) *( ilitek_data->y_ch * ilitek_data->x_ch), GFP_KERNEL);
	int * allnode_data = kzalloc(sizeof(int) * (ilitek_data->y_ch * ilitek_data->x_ch), GFP_KERNEL);
	int * allnode_p2p_data = kzalloc(sizeof(int) * (ilitek_data->y_ch * ilitek_data->x_ch), GFP_KERNEL);
	int * allnode_Tx_delta_data = kzalloc(sizeof(int) *( ilitek_data->y_ch * ilitek_data->x_ch), GFP_KERNEL);
	int * allnode_Rx_delta_data = kzalloc(sizeof(int) *( ilitek_data->y_ch * ilitek_data->x_ch), GFP_KERNEL);
	if(NULL == short_data1||NULL == short_data2||NULL == open_data
		||NULL == allnode_data || NULL == allnode_p2p_data
		||NULL == allnode_Tx_delta_data||NULL == allnode_Rx_delta_data){
		TS_LOG_ERR("%s, malloc error  NULL\n", __func__);
		ret =  -ENOMEM;
		goto MALLOC_ERR;
	}
	i = 0;
	j = 0;


	ilitek_data->sensor_testing = true;
	disable_irq(ilitek_data->ilitek_chip_data->ts_platform_data->irq_id);
	ret = ilitek_check_int_high(INT_POLL_SHORT_RETRY);
	if (ret != ILITEK_INT_STATUS_HIGH) {
		TS_LOG_ERR("%s, ilitek_check_int_high fail\n", __func__);
	}
	ret = ilitek_ready_into_test();
	//ret=-1;
	if (ret){
		ilitek_strncat(info->result, "0F-1F-2F-3F-4F-5F-6F", TS_RAWDATA_RESULT_MAX);
		ilitek_strncat(info->result, ";",  strlen(";"));

		ilitek_strncat(info->result, "-software_reason-", TS_RAWDATA_RESULT_MAX);
		ilitek_strncat(info->result, ilitek_data->ilitek_chip_data->chip_name, TS_RAWDATA_RESULT_MAX);
		ilitek_strncat(info->result, "-", TS_RAWDATA_RESULT_MAX);
		ilitek_strncat(info->result, ilitek_data->product_id, TS_RAWDATA_RESULT_MAX);
		TS_LOG_ERR("%s, i2c transfer error, ret = %d\n", __func__, ret);
		ilitek_reset(ILITEK_RESET_MODEL_CHECKFW_DELAY);
		enable_irq(ilitek_data->ilitek_chip_data->ts_platform_data->irq_id);
		ilitek_data->sensor_testing = false;
		return NO_ERR;
	}

	ret = ilitek_into_testmode();
	mdelay(30);
	ilitek_gesture_disable_sense_start();
	ret = ilitek_allnode_test(allnode_data);
	if (ret){
		TS_LOG_ERR("%s, allnode test failed, ret = %d\n", __func__, ret);
	}
	else {
		TS_LOG_INFO("%s, allnode test right, ret = %d\n", __func__, ret);
	}

	ret = ilitek_Tx_Rx_test(allnode_data, allnode_Tx_delta_data, allnode_Rx_delta_data);
	if (ret){
		TS_LOG_ERR("%s, TRX test failed, ret = %d\n", __func__, ret);
	}
	else {
		TS_LOG_INFO("%s, ilitek_Tx_Rx_test test right, ret = %d\n", __func__, ret);
	}
	ret = ilitek_p2p_test(allnode_p2p_data);
	if (ret){
		TS_LOG_ERR("%s, noise test failed, ret = %d\n", __func__, ret);
	}
	else {
		TS_LOG_INFO("%s, ilitek_p2p_test test right, ret = %d\n", __func__, ret);
	}
	ret = ilitek_open_test(open_data);
	if (ret){
		TS_LOG_ERR("%s, open test failed, ret = %d\n", __func__, ret);
	}
	else {
		TS_LOG_INFO("%s, ilitek_open_test test right, ret = %d\n", __func__, ret);
	}
	ret = ilitek_short_test(short_data1, short_data2);
	if (ret){
		TS_LOG_ERR("%s, short test failed, ret = %d\n", __func__, ret);
	}
	else {
		TS_LOG_INFO("%s, ilitek_short_test test right, ret = %d\n", __func__, ret);
	}
	ret = ilitek_change_report_rate_test();
	if (ret) {
		TS_LOG_ERR("%s, change report rate test failed, ret = %d\n", __func__, ret);
	}
	else {
		TS_LOG_INFO("%s, ilitek_change_report_rate_test test right, ret = %d\n", __func__, ret);
	}
	ret = ilitek_into_normalmode();
	ilitek_reset(ILITEK_RESET_MODEL_CHECKFW_DELAY);
	ret = ilitek_check_int_low(INT_POLL_SHORT_RETRY);
	if (ret != ILITEK_INT_STATUS_LOW) {
		TS_LOG_ERR("%s,reset ilitek_check_int_low fail\n", __func__);
	}
	buf[0] = ILITEK_TP_CMD_READ_DATA;
	ilitek_i2c_write_and_read(buf, 1, 0, buf, 3);
	TS_LOG_INFO("%s, 0x10 cmd read data :%X %X %X\n", __func__, buf[0], buf[1], buf[2]);
	enable_irq(ilitek_data->ilitek_chip_data->ts_platform_data->irq_id);
	ilitek_set_sensor_test_result(info);
	info->buff[buff_index++] = ilitek_data->x_ch;
	info->buff[buff_index++] = ilitek_data->y_ch;
	for (i = 0; i < ilitek_data->x_ch * ilitek_data->y_ch; i++) {
		info->buff[buff_index++] = allnode_data[i];
	}
	for (i = 0; i < (ilitek_data->x_ch) * (ilitek_data->y_ch); i++) {
		info->buff[buff_index++] = open_data[i];
	}
	for (i = 0; i < (ilitek_data->x_ch) * (ilitek_data->y_ch); i++) {
		info->buff[buff_index++] = allnode_p2p_data[i];
	}
	for (i = 0; i < (ilitek_data->x_ch); i++) {
		info->buff[buff_index++] = short_data1[i];
	}
	for (i = 0; i < (ilitek_data->x_ch); i++) {
		info->buff[buff_index++] = short_data2[i];
	}
	for (i = 0; i < (ilitek_data->x_ch) * (ilitek_data->y_ch - 1); i++) {
		info->buff[buff_index++] = allnode_Tx_delta_data[i];
	}
	for (i = 0; i < ilitek_data->y_ch; i++) {
		for (j = 0; j < (ilitek_data->x_ch - 1); j++) {
			info->buff[buff_index++] = allnode_Rx_delta_data[i * (ilitek_data->x_ch - 1) + j];
		}
		info->buff[buff_index++] = 0;
	}
	info->used_size = buff_index;
	TS_LOG_INFO("%s, buff_index = %d\n", __func__, buff_index);
	ret = 0;
MALLOC_ERR:
	if (short_data1) {
		kfree(short_data1);
		short_data1 = NULL;
	}
	if (short_data2) {
		kfree(short_data2);
		short_data2 = NULL;
	}
	if (open_data) {
		kfree(open_data);
		open_data = NULL;
	}
	if (allnode_data) {
		kfree(allnode_data);
		allnode_data = NULL;
	}
	if (allnode_p2p_data) {
		kfree(allnode_p2p_data);
		allnode_p2p_data = NULL;
	}
	if (allnode_Rx_delta_data) {
		kfree(allnode_Rx_delta_data);
		allnode_Rx_delta_data = NULL;
	}
	if (allnode_Tx_delta_data) {
		kfree(allnode_Tx_delta_data);
		allnode_Tx_delta_data = NULL;
	}
	ilitek_data->sensor_testing = false;
	return ret;
}

int ilitek_get_debug_data(struct ts_diff_data_info *info, struct ts_cmd_node *out_cmd)
{
	int ret = NO_ERR;
	int * debug_data = kzalloc(sizeof(int) * (ilitek_data->y_ch * ilitek_data->x_ch + 32), GFP_KERNEL);
	TS_LOG_INFO("ilitek_get_debug_data called\n");
	if (!debug_data) {
		TS_LOG_ERR("ilitek_get_debug_data kzalloc fail\n");
		ret = -1;
		goto MALLOC_ERR;
	}
	memset(debug_data, 0, (ilitek_data->y_ch * ilitek_data->x_ch + 32) * sizeof(int));
	ilitek_data->sensor_testing = true;
	switch (info->debug_type) {
	case READ_DIFF_DATA:
		disable_irq(ilitek_data->ilitek_chip_data->ts_platform_data->irq_id);
		ret = ilitek_ready_into_test();
		ret = ilitek_into_testmode();
		mdelay(30);
		ilitek_gesture_disable_sense_start();
		ret = ilitek_signal_test(debug_data);
		ret = ilitek_into_normalmode();
		enable_irq(ilitek_data->ilitek_chip_data->ts_platform_data->irq_id);
		if (!p2p_noise_test_result) {
			ilitek_strncat(info->result, ILITEK_P2P_TEST_PASS, TS_RAWDATA_RESULT_MAX);
		}
		else {
			ilitek_strncat(info->result, ILITEK_P2P_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
		}
		break;
	case READ_RAW_DATA:
		disable_irq(ilitek_data->ilitek_chip_data->ts_platform_data->irq_id);
		ret = ilitek_ready_into_test();
		ret = ilitek_into_testmode();
		mdelay(30);
		ilitek_gesture_disable_sense_start();
		ret = ilitek_allnode_test(debug_data);
		ret = ilitek_into_normalmode();
		enable_irq(ilitek_data->ilitek_chip_data->ts_platform_data->irq_id);
		if (!allnode_test_result) {
			ilitek_strncat(info->result, ILITEK_ALLNODE_TEST_PASS, TS_RAWDATA_RESULT_MAX);
		}
		else {
			ilitek_strncat(info->result, ILITEK_ALLNODE_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
		}
		break;
	default:
		TS_LOG_ERR("failed to recognize ic_ver\n");
		break;
	}
	info->buff[0] = ilitek_data->x_ch;
	info->buff[1] = ilitek_data->y_ch;
	memcpy(&(info->buff[2]), debug_data, ilitek_data->x_ch * ilitek_data->y_ch * sizeof(int));
	info->used_size = ilitek_data->x_ch * ilitek_data->y_ch +  2;
MALLOC_ERR:
	if (debug_data) {
		kfree(debug_data);
		debug_data = NULL;
	}
	ilitek_data->sensor_testing = false;
	return ret;
}


#if 1
static int ilitek_set_roi_switch(u8 roi_switch)
{
	int error = 0;
	TS_LOG_INFO("%s:roi_switch = %d\n", __func__, roi_switch);

	if (ilitek_data->firmware_updating){
		TS_LOG_ERR("%s: tp fw is updating,return\n", __func__);
		return -EINVAL;
	}

	if (roi_switch < 0) {
		TS_LOG_ERR("%s: roi_switch value %d is invalid\n", __func__, roi_switch);
		return -EINVAL;
	}

	if (roi_switch) {
		error =ilitek_into_fingersense_mode(true);
	}
	else {
		error =ilitek_into_fingersense_mode(false);
	}
	ilitek_data->ilitek_roi_enabled = roi_switch;
	TS_LOG_DEBUG("roi_switch value is %u\n", ilitek_data->ilitek_roi_enabled);
	return error;
}

static int ilitek_get_roi_switch(u8 roi_switch)
{
	TS_LOG_INFO("%s:", __func__);
	roi_switch=ilitek_data->ilitek_roi_enabled;
	return 0;
}
#endif

#if 1
static int ilitek_get_glove_switch(u8 glove_switch)
{

	TS_LOG_INFO("%s:", __func__);
	glove_switch=ilitek_data->glove_status;
	return 0;
}

static int ilitek_set_glove_switch(u8 glove_switch)
{
	int ret = 0;
	TS_LOG_INFO("%s:glove_switch = %d\n", __func__, glove_switch);
	if (ilitek_data->firmware_updating) {
		TS_LOG_ERR("%s: tp fw is updating,return\n", __func__);
		return 0;
	}
	ilitek_data->glove_status=glove_switch;

	if (0 == ilitek_data->glove_status) {
		ret=ilitek_into_glove_mode(false);
	}
	else if (1 == ilitek_data->glove_status) {
		ret=ilitek_into_glove_mode(true);
	}

	return ret;
}

#endif
#if 1
static int ilitek_set_holster_switch(u8 holster_switch)
{
	int ret = 0;
	TS_LOG_INFO("%s:holster_switch = %d\n", __func__, holster_switch);
	if (ilitek_data->firmware_updating) {
		TS_LOG_ERR("%s: tp fw is updating,return\n", __func__);
		return -EINVAL;
	}
	ilitek_data->hall_status=holster_switch;
	if (ilitek_data->hall_status) {
		ret = ilitek_into_hall_halfmode(true);
	}
	else {
		ret = ilitek_into_hall_halfmode(false);
	}
	return ret;
}

static int ilitek_get_holster_switch(u8 holster_switch)
{
	TS_LOG_INFO("%s:", __func__);
	holster_switch=ilitek_data->hall_status;
	return 0;
}
#endif
int ilitek_glove_switch(struct ts_glove_info *info)
{
	int ret = 0;
	TS_LOG_INFO("%s:", __func__);
	if (!info) {
		TS_LOG_ERR("%s:info is null\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	switch (info->op_action) {
	case TS_ACTION_READ:
		info->glove_switch = ilitek_data->glove_status;
		TS_LOG_INFO("%s:glove switch=%d\n", __func__, info->glove_switch);
		break;
	case TS_ACTION_WRITE:
		TS_LOG_INFO("%s:glove switch=%d\n",
			__func__, info->glove_switch);
		ret = ilitek_set_glove_switch(info->glove_switch);
		if (ret) {
			TS_LOG_ERR("%s:set glove switch fail, ret=%d\n",
				__func__, ret);
			return ret;
		}

		break;
	default:
		TS_LOG_ERR("%s:invalid op action:%d\n",
			__func__, info->op_action);
		return -EINVAL;
	}

	return 0;
}

int ilitek_holster_switch(struct ts_holster_info *info)
{
	int retval = NO_ERR;
	TS_LOG_INFO("%s:", __func__);
	if (!info) {
		TS_LOG_ERR("ilitek_holster_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}
	switch (info->op_action) {
	case TS_ACTION_WRITE:
		retval = ilitek_set_holster_switch(info->holster_switch);
		if (retval < 0) {
			TS_LOG_ERR("set holster switch(%d), failed: %d\n",
				   info->holster_switch, retval);
		}
		break;
	case TS_ACTION_READ:
		//retval = ilitek_get_holster_switch(info->holster_switch);
		info->holster_switch = ilitek_data->hall_status;
		TS_LOG_INFO
		    ("invalid holster switch(%d) action: TS_ACTION_READ\n",
		     info->holster_switch);
		break;
	default:
		TS_LOG_INFO("invalid holster switch(%d) action: %d\n",
			    info->holster_switch, info->op_action);
		retval = -EINVAL;
		break;
	}
	return retval;
}

int ilitek_roi_switch(struct ts_roi_info *info)
{
	int retval = NO_ERR;
	TS_LOG_INFO("%s:", __func__);
#ifdef ROI
	if (!info) {
		TS_LOG_ERR("ilitek_roi_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		retval = ilitek_set_roi_switch(info->roi_switch);
		if (retval < 0) {
			TS_LOG_ERR("%s, ilitek_set_roi_switch faild\n",
				   __func__);
		}
		break;
	case TS_ACTION_READ:
		//retval = ilitek_get_roi_switch(info->roi_switch);
		info->roi_switch = ilitek_data->ilitek_roi_enabled;
		break;
	default:
		TS_LOG_INFO("invalid roi switch(%d) action: %d\n",
			    info->roi_switch, info->op_action);
		retval = -EINVAL;
		break;
	}
#endif
	return retval;
}

