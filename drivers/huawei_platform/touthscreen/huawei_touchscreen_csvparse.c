
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "huawei_touchscreen_chips.h"

 enum data_type {
	MUTUAL_RAW_MAX = 1,
	MUTUAL_RAW_MIN,
};

static void copy_this_line(char *dest, char *src)
{
	char *copy_from;
	char *copy_to;

	copy_from = src;
	copy_to = dest;
	do {
		*copy_to = *copy_from;
		copy_from++;
		copy_to++;
	} while((*copy_from != '\n') && (*copy_from != '\r'));
	*copy_to = '\0';
}

static void goto_next_line(char **ptr)
{
	do {
		*ptr = *ptr + 1;
	} while (**ptr != '\n');
	*ptr = *ptr + 1;
}

static void parse_valid_data(char *buf_start, loff_t buf_size,
				char *ptr, enum data_type type, int32_t* rawdata, int rows)
{
	int i = 0;
	int j = 0;
	char *token = NULL;
	char *tok_ptr = NULL;
	char row_data[512] = {0};

	if(!ptr) {
		TS_LOG_ERR("%s, ptr is NULL\n", __func__, ptr);
		return;
	}
	for (i = 0; i < rows; i++) {
		// copy this line to row_data buffer
		memset(row_data, 0, sizeof(row_data));
		copy_this_line(row_data, ptr);
		tok_ptr = row_data;
		while ((token = strsep(&tok_ptr,", \t\n\r\0"))) {
			if (strlen(token) == 0)
				continue;
			if(type == MUTUAL_RAW_MAX) {
				rawdata[j] = (int32_t)simple_strtol(token, NULL, 10);
			} else if (type == MUTUAL_RAW_MIN) {
				rawdata[j] = (int32_t)simple_strtol(token, NULL, 10);
			} else
				TS_LOG_ERR("no such data type\n");

			j ++;
		}
		goto_next_line(&ptr);				//next row
		if(!ptr || (0 == strlen(ptr)) || (ptr >= (buf_start + buf_size))) {
			TS_LOG_INFO("invalid ptr, return\n");
			break;
		}
	}
}

static void print_data(enum data_type type, int32_t* rawdata, int rows, int columns)
{
	int i = 0;
	int j = 0;

	if(NULL == rawdata) {
		TS_LOG_ERR("rawdata is NULL\n");
		return;
	}

	for (i = 0; i < rows; i++) {
		for (j = 0; j < columns; j++) {
			printk("\t%u", rawdata[i*columns + j]);
		}
		printk("\n");
	}
}

int ts_parse_csvfile(int columns, int rows, struct ts_rawdata_limit_tab* limit_tab, char *file_path)
{
	struct file *fp = NULL;
	struct kstat stat;
	int ret = 0;
	char *buf = NULL;
	char *ptr = NULL;
	mm_segment_t org_fs;
	int32_t read_ret = 0;
	loff_t pos = 0;

	org_fs = get_fs();
	set_fs(KERNEL_DS);

	TS_LOG_INFO("%s, file name is %s, rows is %d, columns is %d.\n", __func__,file_path,rows,columns);

	if(NULL == file_path) {
		TS_LOG_ERR("file path pointer is NULL\n");
		ret = -1;
		goto exit_free;
	}

	fp = filp_open(file_path, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		TS_LOG_ERR("%s, filp_open error, file name is %s.\n", __func__,file_path);
		ret = -1;
		goto exit_free;
	}

	ret = vfs_stat(file_path, &stat);
	if(ret) {
		TS_LOG_ERR("%s, failed to get file stat.\n", __func__);
		ret = -2;
		goto exit_free;
	}

	buf = (char *)kzalloc(stat.size + 1, GFP_KERNEL);
	if(NULL == buf) {
		TS_LOG_ERR("%s: kzalloc %lld bytes failed.\n", __func__, stat.size);
		ret = -3;
		goto exit_free;
	}

	read_ret = vfs_read(fp, buf, stat.size, &pos);
	if (read_ret > 0) {
		buf[stat.size] = '\0';
		ptr = buf;
		ptr = strstr(ptr, "MutualRawMax");
		if (ptr == NULL) {
			TS_LOG_ERR("%s: load MutualRawMax failed 1!\n", __func__);
			ret = -4;
			goto exit_free;
		}
		// walk thru this line
		goto_next_line(&ptr);					//next row: MutualRawMax
		if ((NULL == ptr) || (0 == strlen(ptr))) {
			TS_LOG_ERR("%s: load MutualRawMax failed 2!\n", __func__);
			ret = -4;
			goto exit_free;
		}

		parse_valid_data(buf, stat.size, ptr, MUTUAL_RAW_MAX, limit_tab->MutualRawMax, rows);
		print_data(MUTUAL_RAW_MAX, limit_tab->MutualRawMax, rows, columns);

		ptr = buf;
		ptr = strstr(ptr, "MutualRawMin");
		if (ptr == NULL) {
			TS_LOG_ERR("%s: load MutualRawMin failed 1!\n", __func__);
			ret = -5;
			goto exit_free;
		}
		goto_next_line(&ptr);					//next row: MutualRawMin
		if ((NULL == ptr) || (0 == strlen(ptr))) {
			TS_LOG_ERR("%s: load MutualRawMin failed 2!\n", __func__);
			ret = -5;
			goto exit_free;
		}
		parse_valid_data(buf, stat.size, ptr, MUTUAL_RAW_MIN, limit_tab->MutualRawMin, rows);
		print_data(MUTUAL_RAW_MIN, limit_tab->MutualRawMin, rows, columns);
	} else {
		TS_LOG_ERR("%s: ret=%d,read_ret=%d, buf=%p, stat.size=%lld\n", __func__, ret, read_ret, buf, stat.size);
		ret = -6;
		goto exit_free;
	}
	ret = 0;
exit_free:
	TS_LOG_INFO("%s exit free\n", __func__);
	set_fs(org_fs);
	if(buf) {
		TS_LOG_INFO("kfree buf\n");
		kfree(buf);
		buf = NULL;
	}

	if (!IS_ERR_OR_NULL(fp)) {		//fp open fail not means fp is NULL, so free fp may cause Uncertainty
		TS_LOG_INFO("filp close\n");
		filp_close(fp, NULL);
		fp = NULL;
	}
	return ret;
}
