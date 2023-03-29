
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

//#include "huawei_touchscreen_chips.h"
#include <huawei_ts_kit.h>

#define SUPPORT_UNIQUE_TEST 1
#define STRTOL_LEN 10
#define UNIQUE_TEST_FAIL -10

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
	} while((*copy_from != '\n') && (*copy_from != '\r') && (*copy_from != '\0'));
	*copy_to = '\0';
}

static void goto_next_line(char **ptr)
{
	do {
		*ptr = *ptr + 1;
	} while (**ptr != '\n' && **ptr != '\0');
	if (**ptr == '\0') {
		return;
	}
	*ptr = *ptr + 1;
}

static void parse_valid_data(char *buf_start, loff_t buf_size,
				char *ptr, int32_t* data, int rows)
{
	int i = 0;
	int j = 0;
	char *token = NULL;
	char *tok_ptr = NULL;
	char row_data[512] = { 0 };

	if (!ptr) {
		TS_LOG_ERR("%s, ptr is NULL\n", __func__);
		return;
	}
	if (!data) {
		TS_LOG_ERR("%s, data is NULL\n", __func__);
		return;
	}

	for (i = 0; i < rows; i++) {
		// copy this line to row_data buffer
		memset(row_data, 0, sizeof(row_data));
		copy_this_line(row_data, ptr);
		tok_ptr = row_data;
		while ((token = strsep(&tok_ptr, ", \t\n\r\0"))) {
			if (strlen(token) == 0)
				continue;

			data[j] = (int32_t)simple_strtol(token, NULL, STRTOL_LEN);
			j ++;
		}
		goto_next_line(&ptr);				//next row
		if(!ptr || (0 == strlen(ptr)) || (ptr >= (buf_start + buf_size))) {
			TS_LOG_INFO("invalid ptr, return\n");
			break;
		}
	}

	return;
}

static void print_data(char *target_name, int32_t * data, int rows, int columns)
{
	int i, j;
	(void)target_name;

	if (NULL == data) {
		TS_LOG_ERR("rawdata is NULL\n");
		return;
	}

	for (i = 0; i < rows; i++) {
		for (j = 0; j < columns; j++) {
			TS_LOG_DEBUG("\t%d", data[i * columns + j]);
		}
		TS_LOG_DEBUG("\n");
	}

	return;
}

/*lint -save -e* */
int ts_kit_parse_csvfile(char *file_path, char *target_name, int32_t  *data, int rows, int columns)
{
	struct file *fp = NULL;
	struct kstat stat;
	int ret = 0;
	int32_t read_ret = 0;
	char *buf = NULL;
	char *ptr = NULL;
	mm_segment_t org_fs;
	loff_t pos = 0;
	org_fs = get_fs();
	set_fs(KERNEL_DS);

	if (NULL == file_path) {
		TS_LOG_ERR("file path pointer is NULL\n");
		ret = -EPERM;
		goto exit_free;
	}

	if (NULL == target_name) {
		TS_LOG_ERR("target path pointer is NULL\n");
		ret = -EPERM;
		goto exit_free;
	}

	TS_LOG_INFO("%s, file name is %s, target is %s.\n", __func__,file_path,target_name);

	fp = filp_open(file_path, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		TS_LOG_ERR("%s, filp_open error, file name is %s.\n", __func__,file_path);
		ret = -EPERM;
		goto exit_free;
	}

	ret = vfs_stat(file_path, &stat);
	if (ret) {
		TS_LOG_ERR("%s, failed to get file stat.\n", __func__);
		ret = -ENOENT;
		goto exit_free;
	}

	buf = (char *)kzalloc(stat.size + 1, GFP_KERNEL);
	if(NULL == buf) {
		TS_LOG_ERR("%s: kzalloc %lld bytes failed.\n", __func__, stat.size);
		ret = -ESRCH;
		goto exit_free;
	}

	read_ret = vfs_read(fp, buf, stat.size, &pos);
	if (read_ret > 0) {
		buf[stat.size] = '\0';
		ptr = buf;
		ptr = strstr(ptr, target_name);
		if (ptr == NULL) {
			TS_LOG_ERR("%s: load %s failed 1!\n", __func__,target_name);
			ret = -EINTR;
			goto exit_free;
		}
		// walk thru this line
		goto_next_line(&ptr);
		if ((NULL == ptr) || (0 == strlen(ptr))) {
			TS_LOG_ERR("%s: load %s failed 2!\n", __func__,target_name);
			ret = -EIO;
			goto exit_free;
		}

		//analyze the data
		if (data) {
			parse_valid_data(buf, stat.size, ptr, data, rows);
			print_data(target_name, data,  rows, columns);
		}else{
			TS_LOG_ERR("%s: load %s failed 3!\n", __func__,target_name);
			ret = -EINTR;
			goto exit_free;
		}
	}
	else {
		TS_LOG_ERR("%s: ret=%d,read_ret=%d, buf=%p, stat.size=%lld\n", __func__, ret, read_ret, buf, stat.size);
		ret = -ENXIO;
		goto exit_free;
	}
	ret = 0;
 exit_free:
	TS_LOG_INFO("%s exit free\n", __func__);
	set_fs(org_fs);
	if (buf) {
		TS_LOG_INFO("kfree buf\n");
		kfree(buf);
		buf = NULL;
	}

	if (!IS_ERR_OR_NULL(fp)) {	//fp open fail not means fp is NULL, so free fp may cause Uncertainty
		TS_LOG_INFO("filp close\n");
		filp_close(fp, NULL);
		fp = NULL;
	}
	return ret;
}

EXPORT_SYMBOL(ts_kit_parse_csvfile);
/*lint -restore*/