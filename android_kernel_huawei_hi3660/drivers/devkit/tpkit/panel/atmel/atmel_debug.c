#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include "../../huawei_ts_kit_algo.h"
#include "../../tpkit_platform_adapter.h"

#include <linux/regulator/consumer.h>
#include "atmel.h"
#define DEBUG_MSG_MAX		200
#define MXT_T25_PROCESS_TIMEOUT 50
#define MXT_T25_PROCESS_RETRY 100

#define MIN_T6_SIZE 6
#define MIN_T37_SIZE 3
#define SLEF_TEST_PAGES_NUM 3
extern struct mxt_data *mxt_core_data;

void atmel_get_average_max_min_data(struct mxt_data *data, char *buf)
{
	int i = 0;
	if(!data || !buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}

	snprintf((buf), PAGE_SIZE,
		 "[%4d,%4d,%4d]",
		 data->refs_delta_data.refs_data_Average,
		 data->refs_delta_data.refs_data_MaxNum,
		 data->refs_delta_data.refs_data_MinNum);

	i = strlen(buf);
	if (i >= MAX_BUF_SIZE) {
		TS_LOG_ERR("over buf limit, buf size is %ld\n", sizeof(buf));
		return;
	}
	snprintf((buf + i), PAGE_SIZE,
		 "[%4d,%4d,%4d]",
		 data->refs_delta_data.deltas_data_Average,
		 data->refs_delta_data.deltas_data_MaxNum,
		 data->refs_delta_data.deltas_data_MinNum);

	return;
}

int atmel_get_refs_rx2rx_delta_test(struct mxt_data *data)
{
	int tx_n= 0, rx_n = 0;
	s16 rx_delta_value = 0;
	size_t result = 0;
	int rx_num = 0, tx_num = 0;
	int index = 0;
	int index_limit = 0;

	if(!data || !data->T37_buf || !data->rawdata_rx2rx_up_limit_buf
		|| !data->rawdata_rx2rx_low_limit_buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return 0;
	}
	tx_num = (int)(data->x_size - data->x_origin);
	rx_num = (int)(data->y_size - data->y_origin);
	/*print rx2rx data for debug*/
	TS_LOG_INFO("%s, atmel rx_delta data\n", __func__);
	for (tx_n = 0; tx_n < tx_num; tx_n++) {
		for (rx_n = 0; rx_n < rx_num - 1; rx_n++) {
			index = tx_n * rx_num + rx_n;
			rx_delta_value = data->T37_buf[index + 1] -
				data->T37_buf[index];
			printk("%6d", rx_delta_value);
		}
		printk("\n");
	}

	for (tx_n = 0; tx_n < tx_num; tx_n++) {
		for (rx_n = 0; rx_n < rx_num - 1; rx_n++) {
			index = tx_n * rx_num + rx_n;
			index_limit = tx_n * (rx_num - 1) + rx_n;
			rx_delta_value = data->T37_buf[index + 1] -
				data->T37_buf[index];
			if ((rx_delta_value <= data->rawdata_rx2rx_up_limit_buf[index_limit])
			    && (rx_delta_value >= data->rawdata_rx2rx_low_limit_buf[index_limit])) {
				result++;
			} else {
				TS_LOG_ERR("%s, Rx:%d,Tx:%d, rx_delta:%d out of range[%d, %d]\n", __func__,
					rx_n, tx_n,rx_delta_value,
					data->rawdata_rx2rx_low_limit_buf[index_limit],
					data->rawdata_rx2rx_up_limit_buf[index_limit]);
			}
			/*printk("%5d", rx_delta_value);*/
		}
		/*printk("\n");*/
	}

	if (result ==
	    (data->T37_buf_size / sizeof(int) - tx_num)) {
		TS_LOG_INFO("rawdata rx diff is all right, result = %zu\n",
			    result);
		return 1;
	} else
		return 0;
}

int atmel_get_refs_tx2tx_delta_test(struct mxt_data *data)
{
	int tx_n = 0, rx_n = 0;
	s16 tx_delta_value = 0;
	size_t result = 0;
	int rx_num = 0, tx_num = 0;
	int index = 0;

	if(!data || !data->T37_buf || !data->rawdata_tx2tx_up_limit_buf
		|| !data->rawdata_tx2tx_low_limit_buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return 0;
	}

	tx_num = (int)(data->x_size - data->x_origin);
	rx_num = (int)(data->y_size - data->y_origin);
	/*print tx2tx data for debug*/
	TS_LOG_INFO("%s, atmel tx_delta data\n", __func__);
	for (tx_n = 0; tx_n < tx_num - 1; tx_n++) {
		for (rx_n = 0; rx_n < rx_num; rx_n++) {
			tx_delta_value =
			    data->T37_buf[(tx_n + 1) *
					  rx_num +
					  rx_n] -
			    data->T37_buf[tx_n *
					  rx_num +
					  rx_n];
			printk("%6d", tx_delta_value);
		}
		printk("\n");
	}

	for (tx_n = 0; tx_n < tx_num - 1; tx_n++) {
		for (rx_n = 0; rx_n < rx_num; rx_n++) {
			index = tx_n * rx_num + rx_n;
			tx_delta_value =
			    data->T37_buf[(tx_n + 1) *
					  rx_num +
					  rx_n] -
			    data->T37_buf[tx_n *
					  rx_num +
					  rx_n];
			if ((tx_delta_value <= data->rawdata_tx2tx_up_limit_buf[index])
			    && (tx_delta_value >= data->rawdata_tx2tx_low_limit_buf[index])) {
				result++;
			} else {
				TS_LOG_ERR("%s, Rx:%d,Tx:%d, tx_delta:%d,out of range[%d, %d]\n", __func__,
					rx_n, tx_n,tx_delta_value,
					data->rawdata_tx2tx_low_limit_buf[index],
					data->rawdata_tx2tx_up_limit_buf[index]);
			}
			/*printk("%5d", tx_delta_value);*/
		}
		/*printk("\n");*/
	}

	if (result ==
	    (data->T37_buf_size / sizeof(int) - rx_num)) {
		TS_LOG_INFO("rawdata tx diff is all right, result = %zu\n",
			    result);
		return 1;
	} else
		return 0;
}

int atmel_get_refs_max_minus_min_test(struct mxt_data *data, int data_limit)
{
	int max_num = 0;
	int min_num = 0;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return 0;
	}
	max_num = data->refs_delta_data.refs_data_MaxNum;
	min_num = data->refs_delta_data.refs_data_MinNum;

	if (max_num - min_num < data_limit) {
		TS_LOG_INFO("%s, max min delta success\n", __func__);
		return 1;
	} else {
		TS_LOG_INFO("%s, max min delta failed, delta value:%d, data_limit:%d\n", __func__, max_num - min_num, data_limit);
		return 0;
	}
}

int atmel_get_rawdata_test(struct mxt_data *data)
{
	int i = 0, j = 0;
	size_t result = 0;
	int DataSum = 0;
	int Data_Val = 0;
	int Data_Max = 0, Data_Min = 0, Data_ave = 0;
	int Data_count = 0;
	int rx_num = 0, tx_num = 0, rawdata_value = 0;

	if(!data || !data->T37_buf
		|| !data->rawdata_up_limit_buf
		|| !data->rawdata_low_limit_buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return 0;
	}
	tx_num = (int)(data->x_size - data->x_origin);
	rx_num = (int)(data->y_size - data->y_origin);
	/*print rawdata data for debug*/
	TS_LOG_INFO("%s, atmel rawdata\n", __func__);
	for (i = 0; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			rawdata_value = data->T37_buf[i * rx_num + j];
			printk("%6d", rawdata_value);
		}
		printk("\n");
	}

	Data_Max = data->T37_buf[0];
	Data_Min = data->T37_buf[0];

	for (i = 0; i < data->T37_buf_size / sizeof(int); i++) {
		if ((data->T37_buf[i] <= data->rawdata_up_limit_buf[i])
		    && (data->T37_buf[i] >= data->rawdata_low_limit_buf[i]))
			result++;
		else
			TS_LOG_INFO("overlimit[%d]:[%d]\n", i,
				    data->T37_buf[i]);

		Data_Val = data->T37_buf[i];
		DataSum += Data_Val;
		if (Data_Val > Data_Max)
			Data_Max = Data_Val;
		if (Data_Val < Data_Min)
			Data_Min = Data_Val;
	}

	Data_count = data->T37_buf_size / sizeof(int);
	Data_ave = DataSum / Data_count;
	data->refs_delta_data.MaxNum = Data_Max;
	data->refs_delta_data.MinNum = Data_Min;
	data->refs_delta_data.Average = Data_ave;

	TS_LOG_INFO("DataSum:%d, count:%ld, Average:%d, MaxNum:%d, MinNum:%d\n",
		    DataSum, data->T37_buf_size / sizeof(int),
		    data->refs_delta_data.Average, data->refs_delta_data.MaxNum,
		    data->refs_delta_data.MinNum);
	if (result == data->T37_buf_size / sizeof(int)) {
		TS_LOG_INFO("all data is ok, result:%zu\n", result);
		return 1;
	} else {
		TS_LOG_INFO("some data is bad, result:%zu\n", result);
		return 0;
	}
}


int atmel_get_refs_or_deltas_data_test(struct mxt_data *data)
{
	int i = 0, j = 0;
	size_t result = 0;
	int DataSum = 0;
	int Data_Val = 0;
	int Data_Max = 0, Data_Min = 0, Data_ave = 0;
	int Data_count = 0;
	int rx_num = 0, tx_num = 0, noise_value = 0;
	int index = 0;

	if(!data || !data->T37_buf
		|| !data->noise_up_limit_buf
		|| !data->noise_low_limit_buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return 0;
	}

	Data_Max = data->T37_buf[0];
	Data_Min = data->T37_buf[0];
	tx_num = (int)(data->x_size - data->x_origin);
	rx_num = (int)(data->y_size - data->y_origin);
	/*print rx2rx data for debug*/
	TS_LOG_INFO("%s, atmel noise data\n", __func__);
	for (i = 0; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			noise_value = data->T37_buf[i * rx_num + j];
			printk("%6d", noise_value);
		}
		printk("\n");
	}

	for (i = 0; i < data->T37_buf_size / sizeof(int); i++) {
		if ((data->T37_buf[i] <= data->noise_up_limit_buf[i])
		    && (data->T37_buf[i] >= data->noise_low_limit_buf[i]))
			result++;
		else
			TS_LOG_INFO("overlimit[%d]:[%d]\n", i,
				    data->T37_buf[i]);

		Data_Val = data->T37_buf[i];
		DataSum += Data_Val;
		if (Data_Val > Data_Max)
			Data_Max = Data_Val;
		if (Data_Val < Data_Min)
			Data_Min = Data_Val;
	}

	Data_count = data->T37_buf_size / sizeof(int);
	Data_ave = DataSum / Data_count;
	data->refs_delta_data.MaxNum = Data_Max;
	data->refs_delta_data.MinNum = Data_Min;
	data->refs_delta_data.Average = Data_ave;

	TS_LOG_INFO("DataSum:%d, count:%ld, Average:%d, MaxNum:%d, MinNum:%d\n",
		    DataSum, data->T37_buf_size / sizeof(int),
		    data->refs_delta_data.Average, data->refs_delta_data.MaxNum,
		    data->refs_delta_data.MinNum);
	if (result == data->T37_buf_size / sizeof(int)) {
		TS_LOG_INFO("all data is ok, result:%zu\n", result);
		return 1;
	} else {
		TS_LOG_INFO("some data is bad, result:%zu\n", result);
		return 0;
	}
}

/*
 * Helper function for performing a T6 diagnostic command
 */
static int mxt_T6_diag_cmd(struct mxt_data *data, struct mxt_object *T6, u8 cmd)
{
	int ret = 0;
	u16 addr = 0;

	if(!data|| !T6) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	addr = T6->start_address + MXT_COMMAND_DIAGNOSTIC;

	ret = atmel_write_reg(data, addr, cmd);
	if (ret) {
		TS_LOG_ERR("%s: write reg %u error\n", __func__, addr);
		return ret;
	}

	/*
	 * Poll T6.diag until it returns 0x00, which indicates command has
	 * completed.
	 */
	while (cmd != 0) {
		ret = __atmel_read_reg(data, addr, 1, &cmd);
		if (ret) {
			TS_LOG_ERR("%s: read reg %u error\n", __func__, addr);
			return ret;
		}
	}
	return 0;
}

/*
 *Function for reading DELTAS and REFERENCE values for T37 object
 *
 * For both modes, a T37_buf is allocated to stores matrix_xsize * matrix_ysize
 * 2-byte (little-endian) values, which are returned to userspace unmodified.
 *
 * It is left to userspace to parse the 2-byte values.
 * - deltas are signed 2's complement 2-byte little-endian values.
 *     s32 delta = (b[0] + (b[1] << 8));
 * - refs are signed 'offset binary' 2-byte little-endian values, with offset
 *   value 0x4000:
 *     s32 ref = (b[0] + (b[1] << 8)) - 0x4000;
 */
int atmel_T37_fetch(struct mxt_data *data, u8 mode)
{
	struct mxt_object *T6 = NULL, *T37= NULL;
	u8 *T37_buf = NULL;
	ssize_t ret = 0;
	size_t i = 0, j = 0;
	size_t num_pages = 0;
	size_t pos = 0;
	u8 *T37_tem_buf = NULL;
	size_t T37_tem_buf_size = 0;
	int *tem_buf = NULL;
	size_t tem_buf_size = 0;
	u8 cmd = 0;
	size_t chunk_len = 0;
	size_t matrix_add_size = 0;
	size_t matrix_mul_size = 0;

	if (!data || !data->info) {
		TS_LOG_ERR("data=%ld\n", PTR_ERR(data));
		return -ENODEV;
	}

	if (!data->object_table) {
		TS_LOG_ERR("data=%ld data->object_table=%ld\n",
			PTR_ERR(data), PTR_ERR(data->object_table));
		return -ENODEV;
	}
	matrix_add_size = (size_t)((data->info->matrix_xsize) + (data->info->matrix_ysize));
	matrix_mul_size = (size_t)((data->info->matrix_xsize) * data->info->matrix_ysize);
	T6 = atmel_get_object(data, MXT_GEN_COMMANDPROCESSOR_T6);
	T37 = atmel_get_object(data, MXT_DEBUG_DIAGNOSTIC_T37);
	if (!T6 || mxt_obj_size(T6) < MIN_T6_SIZE || !T37 || mxt_obj_size(T37) < MIN_T37_SIZE) {
		TS_LOG_ERR("Invalid T6 or T37 object\n");
		return -ENODEV;
	}
	if (MXT_T6_DEBUG_DELTA_SC == mode)
	{
		tem_buf_size = matrix_add_size * sizeof(int) * 2;
		tem_buf = kzalloc(tem_buf_size, GFP_KERNEL);

		if (!tem_buf) {
			TS_LOG_ERR("alloc tem_buf failed\n");
			return -ENOMEM;
		}
		T37_tem_buf_size = matrix_add_size * sizeof(u8) * 2 * 2;
		T37_tem_buf = kzalloc(T37_tem_buf_size, GFP_KERNEL);
		if (!T37_tem_buf) {
			TS_LOG_ERR("alloc mem for T37_tem_buf failed\n");
			ret = -ENOMEM;
			goto out;
		}

		/* Temporary buffer used to fetch one T37 page */
		T37_buf = kmalloc(mxt_obj_size(T37), GFP_KERNEL);
		if (!T37_buf) {
			TS_LOG_ERR("alloc mem for obuf failed\n");
			ret = -ENOMEM;
			goto out;
		}

		num_pages = SLEF_TEST_PAGES_NUM;
		pos = 0;

		for (i = 0; i < num_pages; i++) {
			cmd = 0;
			chunk_len = 0;
			cmd = (i == 0) ? mode : MXT_T6_DEBUG_PAGEUP;
			ret = mxt_T6_diag_cmd(data, T6, cmd);
			if (ret) {
				TS_LOG_ERR("mxt_T6_diag_cmd failed ret=%zu\n",ret);
				goto out;
			}
			if(1 == i) {
				;
			} else {
				ret = __atmel_read_reg(data, T37->start_address,
					mxt_obj_size(T37), T37_buf);
				if (ret) {
					TS_LOG_ERR("read MXT_DEBUG_DIAGNOSTIC_T37 failed\n");
					goto out;
				}
				/* Verify first two bytes are current mode and page # */
				if (mode != T37_buf[0]) {
					TS_LOG_ERR("Unexpected mode (%u != %u)\n", T37_buf[0], mode);
					ret = -EIO;
					goto out;
				}
				if (i != T37_buf[1]) {
					TS_LOG_ERR("Unexpected page (%u != %zu)\n", T37_buf[1], i);
					ret = -EIO;
					goto out;
				}
				TS_LOG_INFO("Current Page is obuf[1]=%d",T37_buf[1]);
				switch(i){
					case 0://page 0
						chunk_len = (size_t)matrix_add_size * 2;
						break;
					case 2://page 2
						chunk_len = (size_t)(data->info->matrix_ysize) * 2;
						break;
					default:
						break;
				}
				if(chunk_len + pos > T37_tem_buf_size) {
					TS_LOG_ERR("sc mode out of memery range, buf_size = %lu, data_size = %lu\n", T37_tem_buf_size, chunk_len + pos);
					ret = -EIO;
					goto out;
				}
				memcpy(&T37_tem_buf[pos], &T37_buf[2], chunk_len);
				pos += chunk_len;
			}
		}
		for (i = 0, j = 0; i < (size_t)(data->info->matrix_xsize*2 + data->info->matrix_ysize*4); i += 2, j++) {
			tem_buf[j] =
			(short) ((T37_tem_buf[i + 1] << 8) |
				(T37_tem_buf[i]));
		}
		i = 0;
		j = 0;
		while (i < (size_t)(data->info->matrix_xsize + data->info->matrix_ysize*2)) {
			data->T37_buf[j] = tem_buf[i];
			i++;
			j++;
		}
		ret = 0;
		num_pages = 0;
		pos = 0;
	} else {
		tem_buf_size = matrix_mul_size * sizeof(int);
		tem_buf = kzalloc(tem_buf_size, GFP_KERNEL);
		if (!tem_buf) {
			TS_LOG_ERR("alloc tem_buf failed\n");
			return -ENOMEM;
		}
		/*T37_tem_buf_size = (data->x_size -data->x_origin) * (data->y_size -data->y_origin)*sizeof(u8)* 2;*/
		T37_tem_buf_size = matrix_mul_size * sizeof(u8) * 2;
		T37_tem_buf = kzalloc(T37_tem_buf_size, GFP_KERNEL);
		if (!T37_tem_buf) {
			TS_LOG_ERR("alloc mem for T37_tem_buf failed\n");
			ret = -ENOMEM;
			goto out;
		}

		/* Something has gone wrong if T37_buf is already allocated */
		/* if (data->T37_buf)*/
		/*      return -EINVAL;*/

		/* Temporary buffer used to fetch one T37 page */
		T37_buf = kzalloc(mxt_obj_size(T37), GFP_KERNEL);
		if (!T37_buf) {
			TS_LOG_ERR("alloc mem for obuf failed\n");
			ret = -ENOMEM;
			goto out;
		}

		num_pages = DIV_ROUND_UP(T37_tem_buf_size, mxt_obj_size(T37) - 2);
		TS_LOG_INFO("%s, num_pages = %zu\n", __func__, num_pages);
		pos = 0;
		for (i = 0; i < num_pages; i++) {
			cmd = 0;
			chunk_len = 0;

			/* For first page, send mode as cmd, otherwise PageUp */
			cmd = (i == 0) ? mode : MXT_T6_DEBUG_PAGEUP;
			ret = mxt_T6_diag_cmd(data, T6, cmd);
			if (ret) {
				TS_LOG_ERR("mxt_T6_diag_cmd failed ret=%zu\n",ret);
				goto out;
			}

			ret = __atmel_read_reg(data, T37->start_address,
					     mxt_obj_size(T37), T37_buf);
			if (ret) {
				TS_LOG_ERR("read MXT_DEBUG_DIAGNOSTIC_T37 failed\n");
				goto out;
			}

			/* Verify first two bytes are current mode and page # */
			if (T37_buf[0] != mode) {
				TS_LOG_ERR("Unexpected mode (%u != %u)\n", T37_buf[0],
					   mode);
				ret = -EIO;
				goto out;
			}

			if (T37_buf[1] != i) {
				TS_LOG_ERR("Unexpected page (%u != %zu)\n", T37_buf[1], i);
				ret = -EIO;
				goto out;
			}

		/*
		 * Copy the data portion of the page, or however many bytes are
		 * left, whichever is less.
		 */
		 	if(chunk_len + pos > T37_tem_buf_size) {
				TS_LOG_ERR("other mode out of memery range, buf_size = %lu, data_size = %lu\n", T37_tem_buf_size, chunk_len + pos);
				ret = -EIO;
				goto out;
			}
			chunk_len = min((size_t)mxt_obj_size(T37) - 2, (size_t)T37_tem_buf_size - pos);

		/*memcpy(&data->T37_buf[pos], &obuf[2], chunk_len);*/
			memcpy(&T37_tem_buf[pos], &T37_buf[2], chunk_len);
			pos += chunk_len;
		}

		for (i = 0, j = 0; i < T37_tem_buf_size; i += 2, j++) {
			if (mode == MXT_T6_DEBUG_REF) {
				tem_buf[j] =
					(u16) ((T37_tem_buf[i + 1] << 8) |
					(T37_tem_buf[i]));
			} else if (mode == MXT_T6_DEBUG_DELTA) {
				tem_buf[j] =
				    (s16) ((T37_tem_buf[i + 1] << 8) |
					   (T37_tem_buf[i]));
			}
		}

		/*remove extra data */
		i = 0;
		j = 0;
		while ((i < matrix_mul_size)
			&& (j < (size_t)(data->x_size - data->x_origin) * (data->y_size -
			data->y_origin))) {
			data->T37_buf[j] = tem_buf[i];
			i++;
			j++;
			if (0 == j % (size_t)(data->y_size - data->y_origin)) {
				i += (size_t)(data->info->matrix_ysize - data->y_size +
					data->y_origin);
			}
		}
		ret = 0;
	}

out:

	if (T37_tem_buf) {
		kfree(T37_tem_buf);
		T37_tem_buf = NULL;
	}

	if (tem_buf) {
		kfree(tem_buf);
		tem_buf = NULL;
	}

	if (T37_buf) {
		kfree(T37_buf);
		T37_buf = NULL;
	}
	return ret;
}

void atmel_debug_msg_remove(struct mxt_data *data)
{
	if(!data || !data->dev) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	if (data->debug_msg_attr.attr.name) {
		sysfs_remove_bin_file(&data->dev->kobj,
				      &data->debug_msg_attr);
	}
}

/* Firmware Version is returned as Major.Minor.Build */
static ssize_t mxt_fw_version_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = mxt_core_data;

	if (!data || !dev || !data->info || !buf) {
		TS_LOG_ERR("%s, param NULL\n", __func__);
		return -ENODEV;
	}

	return scnprintf(buf, PAGE_SIZE, "%u.%u.%02X\n",
			 data->info->version >> 4, data->info->version & 0xf,
			 data->info->build);
}

/* Hardware Version is returned as FamilyID.VariantID */
static ssize_t mxt_hw_version_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = mxt_core_data;

	if (!data || !dev || !data->info || !buf) {
		TS_LOG_ERR("%s, param NULL\n", __func__);
		return -ENODEV;
	}

	return scnprintf(buf, PAGE_SIZE, "%02x.%02x\n",
			 data->info->family_id, data->info->variant_id);
}

static bool mxt_object_readable(unsigned int type)
{
	switch (type) {
	/*
	case MXT_GEN_COMMAND_T6:
	case MXT_GEN_POWER_T7:
	case MXT_GEN_ACQUIRE_T8:
	case MXT_GEN_DATASOURCE_T53:
	case MXT_TOUCH_MULTI_T9:
	case MXT_TOUCH_KEYARRAY_T15:
	case MXT_TOUCH_PROXIMITY_T23:
	case MXT_TOUCH_PROXKEY_T52:
	case MXT_PROCI_GRIPFACE_T20:
	case MXT_PROCG_NOISE_T22:
	case MXT_PROCI_ONETOUCH_T24:
	case MXT_PROCI_TWOTOUCH_T27:
	case MXT_PROCI_GESTURE_T92:
	case MXT_PROCI_GRIP_T40:
	case MXT_PROCI_PALM_T41:
	case MXT_PROCI_TOUCHSUPPRESSION_T42:
	case MXT_PROCI_STYLUS_T47:
	case MXT_PROCG_NOISESUPPRESSION_T48:
	case MXT_SPT_COMMSCONFIG_T18:
	case MXT_SPT_GPIOPWM_T19:
	case MXT_SPT_SELFTEST_T25:
	case MXT_SPT_CTECONFIG_T28:
	case MXT_SPT_USERDATA_T38:
	case MXT_SPT_DIGITIZER_T43:
	case MXT_SPT_CTECONFIG_T46:
	*/
		return true;
	default:
		return false;
	}
}

static ssize_t mxt_show_instance(char *buf, int count,
				 struct mxt_object *object, int instance,
				 const u8 *val)
{
	int i = 0;

	if(!buf || !object || !val) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return 0;
	}
	if (mxt_obj_instances(object) > 1)
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "Instance %u\n", instance);

	for (i = 0; i < mxt_obj_size(object); i++)
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "\t[%2u]: %02x (%d)\n", i, val[i], val[i]);
	count += scnprintf(buf + count, PAGE_SIZE - count, "\n");

	return count;
}

static ssize_t mxt_object_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = mxt_core_data;
	struct mxt_object *object = NULL;
	int count = 0;
	int i = 0, j = 0;
	int error = 0;
	u8 *obuf = NULL;

	if (!data ||!dev ||!buf || !data->info) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -ENODEV;
	}
	/* Pre-allocate buffer large enough to hold max sized object. */
	obuf = kzalloc(256, GFP_KERNEL);
	if (!obuf) {
		TS_LOG_ERR("%s , malloc obuf failed\n", __func__);
		return -ENOMEM;
	}
	error = 0;
	for (i = 0; i < data->info->object_num; i++) {
		object = data->object_table + i;

		if (!mxt_object_readable(object->type))
			continue;

		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "T%u:\n", object->type);

		for (j = 0; j < mxt_obj_instances(object); j++) {
			u16 size = mxt_obj_size(object);
			u16 addr = object->start_address + j * size;

			error = __atmel_read_reg(data, addr, size, obuf);
			if (error)
				goto done;

			count = mxt_show_instance(buf, count, object, j, obuf);
		}
	}

done:
	kfree(obuf);
	return error ? : count;
}

static ssize_t mxt_update_fw_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	return count;
}

static ssize_t atmel_update_cfg_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	return count;
}

static void mxt_debug_msg_enable(struct mxt_data *data)
{
	if (!data || data->debug_v2_enabled)
		return;

	mutex_lock(&data->debug_msg_lock);

	data->debug_msg_data = kcalloc(DEBUG_MSG_MAX,
				       data->T5_msg_size, GFP_KERNEL);
	if (!data->debug_msg_data) {
		TS_LOG_ERR("Failed to allocate buffer\n");
		mutex_unlock(&data->debug_msg_lock);
		return;
	}

	data->debug_v2_enabled = true;
	mutex_unlock(&data->debug_msg_lock);

	TS_LOG_DEBUG("Enabled message output\n");
}

static void mxt_debug_msg_disable(struct mxt_data *data)
{
	if (!data || !data->debug_v2_enabled)
		return;

	TS_LOG_DEBUG("disabling message output\n");
	data->debug_v2_enabled = false;

	mutex_lock(&data->debug_msg_lock);
	kfree(data->debug_msg_data);
	data->debug_msg_data = NULL;
	data->debug_msg_count = 0;
	mutex_unlock(&data->debug_msg_lock);
	TS_LOG_DEBUG("Disabled message output\n");
}

static ssize_t mxt_debug_v2_enable_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct mxt_data *data = mxt_core_data;
	int i = 0;

	if (!data || !dev || !buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -ENODEV;
	}

	if (sscanf(buf, "%u", &i) == 1 && i < 2) {
		if (i == 1) {
			mxt_debug_msg_enable(data);
			data->do_calibration = false;
		} else {
			mxt_debug_msg_disable(data);
			data->do_calibration = true;
		}

		return count;
	} else {
		TS_LOG_DEBUG("debug_enabled write error\n");
		return -EINVAL;
	}
}

static ssize_t mxt_debug_notify_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	if(!dev || !buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	return sprintf(buf, "0\n");
}

static ssize_t mxt_debug_enable_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = mxt_core_data;
	char c = '\0';

	if (!data || !dev || !buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -ENODEV;
	}

	c = data->debug_enabled ? '1' : '0';
	return scnprintf(buf, PAGE_SIZE, "%c\n", c);
}

static ssize_t mxt_debug_enable_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct mxt_data *data = mxt_core_data;
	int i = 0;

	if (!data || !dev || !buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -ENODEV;
	}

	if (sscanf(buf, "%u", &i) == 1 && i < 2) {
		data->debug_enabled = (i == 1);

		TS_LOG_DEBUG("%s\n", i ? "debug enabled" : "debug disabled");
		return count;
	} else {
		TS_LOG_DEBUG("debug_enabled write error\n");
		return -EINVAL;
	}
}

static ssize_t mxt_bootloader_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = mxt_core_data;
	char c = '\0';

	if (!data || !dev || !buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -ENODEV;
	}
	c = data->in_bootloader ? '1' : '0';
	return scnprintf(buf, PAGE_SIZE, "%c\n", c);
}

static ssize_t mxt_bootloader_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct mxt_data *data = mxt_core_data;
	int i = 0;

	if (!data || !dev || !buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -ENODEV;
	}
	if (sscanf(buf, "%u", &i) == 1 && i < 2) {
		data->in_bootloader = (i == 1);

		TS_LOG_DEBUG("%s\n", i ? "in bootloader" : "app mode");
		return count;
	} else {
		TS_LOG_DEBUG("in_bootloader write error\n");
		return -EINVAL;
	}
}

static ssize_t mxt_t19_gpio_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = mxt_core_data;

	if (!data || !dev || !buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -ENODEV;
	}

	return scnprintf(buf, PAGE_SIZE, "%02x (GPIO 0x%02x)\n",
			 data->t19_msg[0], (data->t19_msg[0] >> 2) & 0xf);
}

static int mxt_t19_command(struct mxt_data *data, bool enable, bool wait)
{
	u16 reg = 0;
	int timeout_counter = 0;
	int ret = 0;
	u8 val[1] = {0};

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	reg = data->T19_address;
	val[0] = 60;

	ret = __atmel_write_reg(data, reg + 3, sizeof(val), val);
	if (ret)
		return ret;

	val[0] = 7;
	ret = __atmel_write_reg(data, reg, sizeof(val), val);
	if (ret)
		return ret;

	if (!wait)
		return 0;

	do {
		msleep(25);//delay 25ms
		ret = __atmel_read_reg(data, reg, 1, &val[0]);
		if (ret)
			return ret;
	} while ((val[0] & 0x4) && (timeout_counter++ <= 100));//100 retry times

	if (timeout_counter > 100) {//100 retry times
		TS_LOG_ERR("Command failed!\n");
		return -EIO;
	}

	return 0;
}

static ssize_t mxt_t19_gpio_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct mxt_data *data = mxt_core_data;
	u8 cmd = 0;

	if (!data || !dev || !buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -ENODEV;
	}
	if (sscanf(buf, "%hhd", &cmd) == 1) {
		if (cmd == 0) {
			data->alt_chip = 0;
			return count;
		} else if (cmd == 1) {
			if (mxt_t19_command(data, !!cmd, 1) == 0) {
				data->alt_chip = cmd;
				return count;
			}
			TS_LOG_DEBUG("mxt_t19_cmd_store write cmd %x error\n",
				     cmd);
		}
		return -EINVAL;
	} else {
		TS_LOG_DEBUG("mxt_t19_cmd_store write error\n");
		return -EINVAL;
	}
}

/* Firmware Version is returned as Major.Minor.Build */
static ssize_t atmel_t25_selftest_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = mxt_core_data;
	ssize_t offset = 0;

	if (!data || !dev || !buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -ENODEV;
	}

	if (data->t25_msg[0] == 0xFE)//0xFE means open short test success
		offset += scnprintf(buf, PAGE_SIZE, "PASS\n");
	else
		offset += scnprintf(buf, PAGE_SIZE, "FAILED\n");

	offset += scnprintf(buf + offset, PAGE_SIZE, "%x %x %x %x %x %x\n",
			    data->t25_msg[0],
			    data->t25_msg[1],
			    data->t25_msg[2],
			    data->t25_msg[3],
			    data->t25_msg[4], data->t25_msg[5]);
	return offset;
}

static int mxt_t25_command(struct mxt_data *data, u8 cmd, bool wait)
{
	u16 reg = 0;
	int timeout_counter = 0;
	int ret = 0;
	u8 val[2] = {0};

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	reg = data->T25_address;
	val[0] = 0x3;//0x3 means enable the T25 & also enable message report of T25
	val[1] = cmd;

	ret = __atmel_write_reg(data, reg, sizeof(val), val);
	if (ret) {
		TS_LOG_ERR("write T25 command failed\n");
		return ret;
	}

	if (!wait) {
		TS_LOG_INFO("no need to wait\n");
		return 0;
	}

	do {
		msleep(14);//ic need wait when retry
		ret = __atmel_read_reg(data, reg + 1, 1, &val[1]);
		if (ret) {
			TS_LOG_ERR
			    ("read T25 command failed,already try %d times\n",
			     timeout_counter);
			return ret;
		}
	} while ((val[1] != 0) && (timeout_counter++ <= MAX_TIMEOUT_TIMES));//50:retry times

	if (timeout_counter > MAX_TIMEOUT_TIMES) {//50:retry times
		TS_LOG_ERR("Command failed!\n");
		return -EIO;
	}
	return 0;
}

int atmel_t25_selftest_polling(struct mxt_data *data, int cmd)
{
	u32 i = 0;

	if(!data) {
		TS_LOG_ERR("%s, data invalid\n", __func__);
		return 0;
	}

	memset(data->t25_msg, 0, sizeof(data->t25_msg));
	if (mxt_t25_command(data,(u8)cmd,1) == 0) {
		TS_LOG_INFO("mxt_t25_command ok\n");
	}

	mdelay(100);//wait t25 command
	for ( i = 0 ; i < MXT_T25_PROCESS_RETRY ; i ++ ) {
		mxt_process_messages_t25(data);
		if (data->t25_msg[0] == 0xFE) {//0xFE means open short test success
			break;
		}
		mdelay(10);//wait parse message
	}

	if ( i == MXT_T25_PROCESS_RETRY) {
		TS_LOG_INFO("mxt_t25_selftest fail\n");
		return 0;
	}
	TS_LOG_INFO("mxt_t25_selftest success\n");
	return 1;

}

int atmel_t25_selftest(struct mxt_data *data, int cmd)
{
	u32 i = 0;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return 0;
	}

	data->t25_msg[0] = 0;
	data->t25_msg[5] = 0;

	if (mxt_t25_command(data, (u8) cmd, 1) == 0) {
		TS_LOG_INFO("mxt_t25_command ok\n");
	}

	mdelay(100);//ic need
	for (i = 0; i < MXT_T25_PROCESS_RETRY; i++) {
		atmel_process_messages(data, MXT_T25_PROCESS_TIMEOUT);
		TS_LOG_DEBUG("%02X %02X %02X %02X %02X %02X\n",
			     data->t25_msg[0], data->t25_msg[1],
			     data->t25_msg[2], data->t25_msg[3],
			     data->t25_msg[4], data->t25_msg[5]);
		if (data->t25_msg[0] == 0xFE) {//0xFE means open short test success
			break;
		}
	}

	if (i == MXT_T25_PROCESS_RETRY) {
		TS_LOG_INFO("atmel_t25_selftest fail\n");

		return 0;
	}
	TS_LOG_INFO("atmel_t25_selftest success\n");
	return 1;

}

static ssize_t atmel_t25_selftest_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct mxt_data *data = mxt_core_data;
	u32 cmd= 0;

	if (!data || !dev || !buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -ENODEV;
	}

	if (sscanf(buf, "%x", &cmd) == 1) {
		if (mxt_t25_command(data, (u8) cmd, 1) == 0)
			return count;

		TS_LOG_DEBUG("mxt_t25_cmd_store write cmd %x error\n", cmd);
		return -EINVAL;
	} else {
		TS_LOG_DEBUG("mxt_t25_cmd_store write error\n");
		return -EINVAL;
	}
}

static DEVICE_ATTR(fw_version, S_IRUGO, mxt_fw_version_show, NULL);
static DEVICE_ATTR(hw_version, S_IRUGO, mxt_hw_version_show, NULL);
static DEVICE_ATTR(object, S_IRUGO, mxt_object_show, NULL);
static DEVICE_ATTR(update_fw, S_IWUSR /*| S_IWGRP | S_IWOTH */, NULL,
		   mxt_update_fw_store);
static DEVICE_ATTR(update_cfg, S_IWUSR /* | S_IWGRP | S_IWOTH */, NULL,
		   atmel_update_cfg_store);
static DEVICE_ATTR(debug_v2_enable, S_IWUSR /*S_IWUSR | S_IRUSR */, NULL,
		   mxt_debug_v2_enable_store);
static DEVICE_ATTR(debug_notify, S_IRUGO, mxt_debug_notify_show, NULL);
static DEVICE_ATTR(debug_enable, S_IWUSR | S_IRUSR, mxt_debug_enable_show,
		   mxt_debug_enable_store);
static DEVICE_ATTR(bootloader, S_IWUSR | S_IRUSR, mxt_bootloader_show,
		   mxt_bootloader_store);
static DEVICE_ATTR(t19, S_IWUSR | S_IRUSR /*| S_IWGRP | S_IWOTH */,
		   mxt_t19_gpio_show,
		   mxt_t19_gpio_store);
static DEVICE_ATTR(t25, S_IWUSR | S_IRUSR /*| S_IWGRP | S_IWOTH */,
		   atmel_t25_selftest_show,
		   atmel_t25_selftest_store);

static struct attribute *mxt_attrs[] = {
	&dev_attr_fw_version.attr,
	&dev_attr_hw_version.attr,
	&dev_attr_object.attr,
	&dev_attr_update_fw.attr,
	&dev_attr_update_cfg.attr,
	&dev_attr_debug_enable.attr,
	&dev_attr_debug_v2_enable.attr,
	&dev_attr_debug_notify.attr,
	&dev_attr_bootloader.attr,
	&dev_attr_t19.attr,
	&dev_attr_t25.attr,
	NULL
};

static const struct attribute_group mxt_attr_group = {
	.attrs = mxt_attrs,
};

static int mxt_check_mem_access_params(struct mxt_data *data, loff_t off,
				       size_t *count, int max_size)
{
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	if (off >= data->mem_size)
		return -EIO;

	if (off + *count > data->mem_size)
		*count = data->mem_size - off;

	if (*count > max_size)
		*count = max_size;

	return 0;
}

static ssize_t mxt_debug_msg_read(struct file *filp, struct kobject *kobj,
				  struct bin_attribute *bin_attr, char *buf,
				  loff_t off, size_t bytes)
{
	struct mxt_data *data = mxt_core_data;

	int count = 0;
	size_t bytes_read = 0;

	if (!data || !buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return 0;
	}

	mutex_lock(&data->debug_msg_lock);

	if (!data->debug_msg_data) {
		TS_LOG_ERR("No buffer!\n");
		mutex_unlock(&data->debug_msg_lock);
		return 0;
	}
	if(!data->T5_msg_size){
		data->T5_msg_size = 1;
	}

	count = bytes / data->T5_msg_size;

	if (count > DEBUG_MSG_MAX)
		count = DEBUG_MSG_MAX;

	if (count > data->debug_msg_count)
		count = data->debug_msg_count;

	bytes_read = count * data->T5_msg_size;

	memcpy(buf, data->debug_msg_data, bytes_read);
	data->debug_msg_count = 0;

	mutex_unlock(&data->debug_msg_lock);

	return bytes_read;
}
static ssize_t mxt_debug_msg_write(struct file *filp, struct kobject *kobj,
				    struct bin_attribute *bin_attr, char *buf,
				    loff_t off, size_t count)
{
	return -EIO;
}
static ssize_t mxt_mem_access_read(struct file *filp, struct kobject *kobj,
				   struct bin_attribute *bin_attr, char *buf,
				   loff_t off, size_t count)
{
	struct mxt_data *data = mxt_core_data;
	int ret = 0;

	if (!data)
		return -ENODEV;

	ret =
	    mxt_check_mem_access_params(data, off, &count, MXT_MAX_BLOCK_READ);
	if (ret < 0)
		return ret;

	mutex_lock(&data->access_mutex);

	if (count > 0)
		ret = __atmel_read_reg(data, off, count, buf);

	mutex_unlock(&data->access_mutex);

	return ret == 0 ? count : ret;
}

static ssize_t mxt_mem_access_write(struct file *filp, struct kobject *kobj,
				    struct bin_attribute *bin_attr, char *buf,
				    loff_t off, size_t count)
{
	struct mxt_data *data = mxt_core_data;
	int ret = 0;

	if (!data)
		return -ENODEV;

	ret =
	    mxt_check_mem_access_params(data, off, &count, MXT_MAX_BLOCK_WRITE);
	if (ret < 0)
		return ret;

	mutex_lock(&data->access_mutex);

	if (count > 0)
		ret = __atmel_write_reg(data, off, count, buf);

	mutex_unlock(&data->access_mutex);

	return ret == 0 ? count : 0;
}

void atmel_debug_msg_add(struct mxt_data *data, u8 *msg)
{
	if(!data || !data->dev) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}

	mutex_lock(&data->debug_msg_lock);

	if (!data->debug_msg_data) {
		TS_LOG_ERR("No buffer!\n");
		mutex_unlock(&data->debug_msg_lock);
		return;
	}

	if (data->debug_msg_count < DEBUG_MSG_MAX) {
		memcpy(data->debug_msg_data +
		       data->debug_msg_count * data->T5_msg_size, msg,
		       data->T5_msg_size);
		data->debug_msg_count++;
	} else {
		TS_LOG_DEBUG("Discarding %u messages\n", data->debug_msg_count);
		data->debug_msg_count = 0;
	}

	mutex_unlock(&data->debug_msg_lock);

	sysfs_notify(&data->dev->kobj, NULL, "debug_notify");
}

int atmel_mem_access_init(struct mxt_data *data)
{
	int error = 0;

	if(!data || !data->dev) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	error =
	    sysfs_create_group(&data->dev->kobj, &mxt_attr_group);
	if (error) {
		TS_LOG_ERR("Failure %d creating sysfs group\n", error);
		return 0;
	}

	sysfs_bin_attr_init(&data->mem_access_attr);
	data->mem_access_attr.attr.name = "mem_access";
	data->mem_access_attr.attr.mode = 0640 /*S_IRUGO | S_IWUSR */ ;
	data->mem_access_attr.read = mxt_mem_access_read;
	data->mem_access_attr.write = mxt_mem_access_write;
	data->mem_access_attr.size = data->mem_size;

	if (sysfs_create_bin_file(&data->dev->kobj,
				  &data->mem_access_attr) < 0) {
		TS_LOG_ERR("Failed to create %s\n",
			   data->mem_access_attr.attr.name);
		goto err_remove_sysfs_group;
	}

	return 0;
err_remove_sysfs_group:
	sysfs_remove_group(&data->dev->kobj, &mxt_attr_group);
	return 0;
}

int atmel_debug_msg_init(struct mxt_data *data)
{
	if(!data || !data->dev) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	sysfs_bin_attr_init(&data->debug_msg_attr);
	data->debug_msg_attr.attr.name = "debug_msg";
	data->debug_msg_attr.attr.mode = 0640;
	data->debug_msg_attr.read = mxt_debug_msg_read;
	data->debug_msg_attr.write = mxt_debug_msg_write;
	data->debug_msg_attr.size = data->T5_msg_size * DEBUG_MSG_MAX;

	if (sysfs_create_bin_file(&data->dev->kobj,
				  &data->debug_msg_attr) < 0) {
		TS_LOG_ERR("Failed to create %s\n",
			   data->debug_msg_attr.attr.name);
		return -EINVAL;
	}

	return NO_ERR;
}
