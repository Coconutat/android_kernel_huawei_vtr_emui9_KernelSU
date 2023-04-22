#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/fd.h>
#include <linux/tty.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/atomic.h>
#include <linux/notifier.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/hisi/ipc_msg.h>
#include <linux/hisi/hisi_rproc.h>
#include <linux/hisi/kirin_partition.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include "soc_acpu_baseaddr_interface.h"
#include "soc_sctrl_interface.h"
#include "hisi_hisee.h"
#include "hisi_hisee_fs.h"

/*************************************************************
函数原型：void parse_timestamp(const char *timestamp_str, timestamp_info *timestamp_value)
函数功能：根据时间戳的字符串信息解析时间戳
参数：
输入：timestamp_str:指向包含hisee.img镜像时间戳信息的字符串指针
输出：timestamp_value:指向解析后的时间戳的结构体指针
返回值：无
前置条件：无
后置条件： 无
*************************************************************/
void parse_timestamp(const char timestamp_str[HISEE_IMG_TIME_STAMP_LEN], timestamp_info *timestamp_value)
{
	unsigned short value_short;
	unsigned char  value_char;

	if (NULL != timestamp_value) {
		timestamp_value->value = 0x0;

		value_short = (timestamp_str[0] - 0x30) * 1000 + (timestamp_str[1] - 0x30) * 100
				+ (timestamp_str[2] - 0x30) * 10 + (timestamp_str[3] - 0x30); /*lint !e734*/
		timestamp_value->timestamp.year = value_short;

		value_char = (timestamp_str[5] - 0x30) * 10 + (timestamp_str[6] - 0x30); /*lint !e734*/
		timestamp_value->timestamp.month = value_char;

		value_char = (timestamp_str[8] - 0x30) * 10 + (timestamp_str[9] - 0x30); /*lint !e734*/
		timestamp_value->timestamp.day = value_char;

		value_char = (timestamp_str[11] - 0x30) * 10 + (timestamp_str[12] - 0x30); /*lint !e734*/
		timestamp_value->timestamp.hour = value_char;

		value_char = (timestamp_str[14] - 0x30) * 10 + (timestamp_str[15] - 0x30); /*lint !e734*/
		timestamp_value->timestamp.minute = value_char;

		value_char = (timestamp_str[17] - 0x30) * 10 + (timestamp_str[18] - 0x30); /*lint !e734*/
		timestamp_value->timestamp.second = value_char;
	}
}


int write_hisee_otp_value(hisee_img_file_type otp_img_index)
{
	char *buff_virt;
	phys_addr_t buff_phy = 0;
	atf_message_header *p_message_header;
	int ret = HISEE_OK;
	int image_size;
	unsigned int result_offset;

	if (otp_img_index < OTP_IMG_TYPE || otp_img_index > OTP1_IMG_TYPE) {
		pr_err("%s(): otp_img_index=%d invalid\n", __func__, (int)otp_img_index);
		set_errno_and_return(HISEE_INVALID_PARAMS);
	}
	buff_virt = (void *)dma_alloc_coherent(g_hisee_data.cma_device, HISEE_SHARE_BUFF_SIZE,
											&buff_phy, GFP_KERNEL);
	if (buff_virt == NULL) {
		pr_err("%s(): dma_alloc_coherent failed\n", __func__);
		set_errno_and_return(HISEE_NO_RESOURCES);
	}

	pr_err("%s(): entering, otp_img_index=%d\n", __func__, (int)otp_img_index);
	memset(buff_virt, 0, HISEE_SHARE_BUFF_SIZE);
	p_message_header = (atf_message_header *)buff_virt;
	set_message_header(p_message_header, CMD_UPGRADE_OTP);
	ret = filesys_hisee_read_image(otp_img_index, (buff_virt + HISEE_ATF_MESSAGE_HEADER_LEN));
	if (ret < HISEE_OK) {
		pr_err("%s(): filesys_hisee_read_image failed, ret=%d\n", __func__, ret);
		dma_free_coherent(g_hisee_data.cma_device, (unsigned long)HISEE_SHARE_BUFF_SIZE, buff_virt, buff_phy);
		set_errno_and_return(ret);
	}
	image_size = (ret + HISEE_ATF_MESSAGE_HEADER_LEN);

	result_offset = (u32)(image_size + SMC_TEST_RESULT_SIZE - 1) & (~(u32)(SMC_TEST_RESULT_SIZE -1));
	if (result_offset + SMC_TEST_RESULT_SIZE <= HISEE_SHARE_BUFF_SIZE) {
		p_message_header->test_result_phy = (unsigned int)buff_phy + result_offset;
		p_message_header->test_result_size = (unsigned int)SMC_TEST_RESULT_SIZE;
	}
	ret = send_smc_process(p_message_header, buff_phy, image_size,
							HISEE_ATF_OTP_TIMEOUT, CMD_UPGRADE_OTP);
	if (HISEE_OK != ret) {
		if (result_offset + SMC_TEST_RESULT_SIZE <= HISEE_SHARE_BUFF_SIZE) {
			pr_err("%s(): hisee reported fail code=%d\n", __func__, *((int *)(void *)(buff_virt + result_offset)));
		}
	}

	dma_free_coherent(g_hisee_data.cma_device, (unsigned long)HISEE_SHARE_BUFF_SIZE, buff_virt, buff_phy);
	return ret;
}

/** read full path file interface
 * @fullname: input, the full path name should be read
 * @buffer: output, save the data
 * @offset: the offset in this file started to read
 * @size: the count bytes should be read.if zero means read total file
 * On success, the number of bytes read is returned (zero indicates end of file),
 * On error, the return value is less than zero, please check the errcode in hisee module.
 */
int hisee_read_file(const char *fullname, char *buffer, size_t offset, size_t size)
{
	struct file *fp;
	int ret = HISEE_OK;
	ssize_t cnt;
	ssize_t read_bytes = 0;
	loff_t pos = 0;
	mm_segment_t old_fs;

	if (NULL == fullname || NULL == buffer) {
		pr_err("%s(): passed ptr is NULL\n", __func__);
		set_errno_and_return(HISEE_INVALID_PARAMS);
	}

	if (0 == size || size > HISEE_MAX_IMG_SIZE) {
		pr_err("%s(): passed size is invalid\n", __func__);
		set_errno_and_return(HISEE_INVALID_PARAMS);
	}

	old_fs = get_fs();/*lint !e501*/
	set_fs(KERNEL_DS);/*lint !e501*/
	fp = filp_open(fullname, O_RDONLY, 0600);
	if (IS_ERR(fp)) {
		pr_err("%s():open %s failed\n", __func__, fullname);
		ret = HISEE_OPEN_FILE_ERROR;
		goto out;
	}
	ret = vfs_llseek(fp, 0L, SEEK_END);/*lint !e712*/
	if (ret < 0) {
		pr_err("%s():lseek %s failed from end.\n", __func__, fullname);
		ret = HISEE_LSEEK_FILE_ERROR;
		goto close;
	}
	pos = fp->f_pos;/*lint !e613*/
	if ((offset + size) > (size_t)pos) {
		pr_err("%s(): offset(%lx), size(%lx) both invalid.\n", __func__, offset, size);
		ret = HISEE_OUTOF_RANGE_FILE_ERROR;
		goto close;
	}
	ret = vfs_llseek(fp, offset, SEEK_SET);/*lint !e712*/
	if (ret < 0) {
		pr_err("%s():lseek %s failed from begin.\n", __func__, fullname);
		ret = HISEE_LSEEK_FILE_ERROR;
		goto close;
	}

	pos = fp->f_pos;/*lint !e613*/
	read_bytes = size;

	cnt = vfs_read(fp, (char __user *)buffer, read_bytes, &pos);
	if (cnt < read_bytes) {
		pr_err("%s():read %s failed, return [%ld]\n", __func__, fullname, cnt);
		ret = HISEE_READ_FILE_ERROR;
		goto close;
	}
	read_bytes = cnt;
close:
	filp_close(fp, NULL);/*lint !e668*/
out:
	set_fs(old_fs);
	if (ret >= HISEE_OK) {
		atomic_set(&g_hisee_errno, HISEE_OK);/*lint !e1058*/
		ret = read_bytes;
	} else {
		atomic_set(&g_hisee_errno, ret);/*lint !e1058*/
	}
	return ret;
}


/****************************************************************************//**
 * @brief      : hisee_write_file
 * @param[in]  : fullname , file name in full path
 * @param[in]  : buffer , data to write
 * @param[in]  : size , data size
 * @return     : ::int
 * @note       :
********************************************************************************/
int hisee_write_file(const char *fullname, char *buffer, size_t size)
{
	struct file *fp;
	int ret = HISEE_OK;
	ssize_t cnt;
	loff_t pos = 0;
	mm_segment_t old_fs;

	if (NULL == fullname || NULL == buffer) {
		pr_err("%s(): passed ptr is NULL\n", __func__);
		set_errno_and_return(HISEE_INVALID_PARAMS);
	}

	old_fs = get_fs();/*lint !e501*/
	set_fs(KERNEL_DS);/*lint !e501*/
	fp = filp_open(fullname, O_WRONLY|O_CREAT|O_TRUNC, 0600);
	if (IS_ERR(fp)) {
		pr_err("%s():open %s failed\n", __func__, fullname);
		ret = HISEE_OPEN_FILE_ERROR;
		goto out;
	}

	cnt = vfs_write(fp, (char __user *)buffer, (unsigned long)size, &pos);
	if (cnt != (ssize_t)size) {
		pr_err("%s():write failed, return [%ld]\n", __func__, cnt);
		ret = HISEE_WRITE_FILE_ERROR;
	}

	filp_close(fp, NULL);

out:
	set_fs(old_fs);
	return ret;
}


/*************************************************************
函数原型：int hisee_atoi(const char *str)
函数功能：实现和libc的atoi函数相同的功能，字符串转成整型值，只支持正数
参数：
输入：str:指向一串整型值的字符串
输出：无。
返回值：HISEE_U32_MAX_VALUE:错误值；不是HISEE_U32_MAX_VALUE:转换后的整型值
前置条件：无。
后置条件： 无
*************************************************************/
static unsigned int hisee_atoi(const char *str)
{
	const char *p = str;
	char c;
	unsigned long ret = 0;

	if (NULL == str) {
		return HISEE_U32_MAX_VALUE;
	}
	while ((c = *p++) != '\0') {
		if ('0' <= c && c <= '9') {
			ret *= 10;
			ret += (unsigned long) ((unsigned char)c - '0');
			if (ret >= HISEE_U32_MAX_VALUE) {
				return HISEE_U32_MAX_VALUE;
			}
		} else {
			return HISEE_U32_MAX_VALUE;
		}
	}
	return (unsigned int)ret;
}

/*************************************************************
函数原型：int parse_misc_id_from_name(const img_file_info *curr_file_info,
									unsigned int *misc_image_cnt_array)
函数功能：根据hisee.img的文件的头部信息，解析出misc镜像的misc id
参数：
输入：curr_file_info:指向当前文件的文件头部信息的结构体指针；
	 misc_image_cnt_array:指向保存所有cos镜像对应的misc镜像的格式的数组的指针
输出：无。
返回值：HISEE_OK:函数执行成功；不是HISEE_OK:函数执行失败
前置条件：无。
后置条件： 无。
*************************************************************/
static int parse_misc_id_from_name(const img_file_info *curr_file_info,
									unsigned int *misc_image_cnt_array)
{
	unsigned int curr_misc_id;
	unsigned int cos_id;
	unsigned int max_misc_num = HISEE_MAX_MISC_IMAGE_NUMBER;
	unsigned int is_smx_0 = 0;

	if (NULL == curr_file_info || NULL == misc_image_cnt_array) {
		return HISEE_ERROR;
	}
	if (strncmp(curr_file_info->name, HISEE_IMG_MISC_NAME, strlen(HISEE_IMG_MISC_NAME))) {
		return HISEE_ERROR;
	}
	/*get the misc id from image name, range is [0,19]*/
	curr_misc_id = hisee_atoi(curr_file_info->name + strlen(HISEE_IMG_MISC_NAME));
	if (HISEE_MAX_MISC_ID_NUMBER <= curr_misc_id) {
		return HISEE_ERROR;
	}

	hisee_get_smx_cfg(&is_smx_0);
	if (SMX_PROCESS_0 == is_smx_0) {
		max_misc_num = HISEE_SMX_MISC_IMAGE_NUMBER;
	}
	/*group misc image index by cos_id, then counter add one*/
	cos_id = curr_misc_id / max_misc_num;
	misc_image_cnt_array[cos_id] += 1;

	if (misc_image_cnt_array[cos_id] > max_misc_num) {
		pr_err("%s():misc cnt =%d is invalid\n", __func__, misc_image_cnt_array[cos_id]);
		return HISEE_SUB_FILE_OFFSET_CHECK_ERROR;
	}

	/* get misc version*/
	if (SMX_PROCESS_0 != is_smx_0) {
		if (misc_image_cnt_array[cos_id] == max_misc_num) {
			misc_image_cnt_array[cos_id] -= HISEE_MISC_NO_UPGRADE_NUMBER;
			g_misc_version[cos_id] = curr_file_info->size;
			pr_err("hisee:%s():cos_id=%d, misc_cnt=%d,misc_version=%d\n",
				__func__, cos_id, misc_image_cnt_array[cos_id], g_misc_version[cos_id]);
		}
	} else {
		pr_err("hisee:%s():cos_id=%d, misc_cnt=%d\n", __func__, cos_id, misc_image_cnt_array[cos_id]);
	}

	return HISEE_OK;
}

static int check_img_header_is_valid(hisee_img_header *p_img_header)
{
	unsigned int i;
	int ret = HISEE_OK;
	unsigned int emmc_cos_cnt = 0;
	unsigned int rpmb_cos_cnt = 0;
	unsigned int total_size;

	total_size = HISEE_IMG_HEADER_LEN + HISEE_IMG_SUB_FILES_LEN * p_img_header->file_cnt;

	for (i = 0; i < p_img_header->file_cnt; i++) {
		if (!strncmp(p_img_header->file[i].name, HISEE_IMG_MISC_NAME, strlen(HISEE_IMG_MISC_NAME))) {
			ret = parse_misc_id_from_name(&p_img_header->file[i], &p_img_header->misc_image_cnt[0]);
			if (HISEE_OK != ret) {
				pr_err("hisee:%s():parse misc id failed, ret=%d\n", __func__, ret);
				return HISEE_IMG_SUB_FILE_NAME_ERROR;
			}
		}
		if (!strncmp(p_img_header->file[i].name, HISEE_IMG_COS_NAME, strlen(HISEE_IMG_COS_NAME))) {
			if ((p_img_header->file[i].name[strlen(HISEE_IMG_COS_NAME)] - '0') < HISEE_MAX_RPMB_COS_NUMBER)
				rpmb_cos_cnt++;
			else
				emmc_cos_cnt++;
		}
		if (0 == p_img_header->file[i].size || p_img_header->file[i].size > HISEE_MAX_IMG_SIZE) {
			pr_err("%s():size check %s failed\n", __func__, p_img_header->file[i].name);
			return HISEE_SUB_FILE_SIZE_CHECK_ERROR;
		}
		if (p_img_header->file[i].offset < total_size) {
			pr_err("%s():offset check %s failed\n", __func__, p_img_header->file[i].name);
			return HISEE_SUB_FILE_OFFSET_CHECK_ERROR;
		}
		total_size += p_img_header->file[i].size;
	}
	if ((rpmb_cos_cnt > HISEE_MAX_RPMB_COS_NUMBER) ||
		(emmc_cos_cnt > HISEE_MAX_EMMC_COS_NUMBER) ||
		(rpmb_cos_cnt + emmc_cos_cnt) > HISEE_SUPPORT_COS_FILE_NUMBER) {
		pr_err("%s():cos cnt =%d is invalid\n", __func__, rpmb_cos_cnt + emmc_cos_cnt);
		return HISEE_SUB_FILE_OFFSET_CHECK_ERROR;
	}
	p_img_header->emmc_cos_cnt = emmc_cos_cnt;
	p_img_header->rpmb_cos_cnt = rpmb_cos_cnt;
	pr_err("%s():rpmb_cos cnt =%d emmc_cos cnt =%d\n", __func__, rpmb_cos_cnt , emmc_cos_cnt);
	return ret;
}


/*static int hisee_erase_hisee_img_head(void)
{
	struct file *fp;
	char fullname[MAX_PATH_NAME_LEN + 1] = { 0 };
	char erase_head_data[HISEE_IMG_HEADER_LEN] = {0};
	int ret;
	loff_t pos;
	ssize_t cnt;
	mm_segment_t old_fs;

	ret = flash_find_ptn(HISEE_IMAGE_PARTITION_NAME, fullname);
	if (0 != ret) {
	    pr_err("%s():flash_find_ptn fail\n", __func__);
		ret = HISEE_OPEN_FILE_ERROR;
	    set_errno_and_return(ret);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(fullname, O_WRONLY, 0600);
	if (IS_ERR(fp)) {
		pr_err("%s():open %s failed\n", __func__, fullname);
		ret = HISEE_OPEN_FILE_ERROR;
		set_fs(old_fs);
		set_errno_and_return(ret);
	}

	pos = fp->f_pos;
	cnt = vfs_write(fp, (char __user *)erase_head_data, (unsigned long)HISEE_IMG_HEADER_LEN, &pos);
	if (cnt < HISEE_IMG_HEADER_LEN) {
		pr_err("%s():write failed, return [%ld]\n", __func__, cnt);
		ret = HISEE_WRITE_FILE_ERROR;
	}

	filp_close(fp, NULL);
	set_fs(old_fs);
	check_and_print_result();
	set_errno_and_return(ret);
}*/

/** parse the hisee_img partition header interface
 * @buffer: output, save the data
 * On success, zero is return,
 * On error, the return value is less than zero, please check the errcode in hisee module.
 */
int hisee_parse_img_header(char *buffer)
{
	struct file *fp;
	hisee_img_header *p_img_header;
	unsigned int sw_version_num;
	char fullname[MAX_PATH_NAME_LEN + 1] = { 0 };
	char cos_img_rawdata[HISEE_IMG_HEADER_LEN] = {0};
	int ret;
	unsigned int i;
	loff_t pos;
	ssize_t cnt;
	mm_segment_t old_fs;

	ret = flash_find_ptn(HISEE_IMAGE_PARTITION_NAME, fullname);
	if (0 != ret || NULL == buffer) {
	    pr_err("%s():flash_find_ptn or buffer params fail,ret=%d\n", __func__, ret);
		ret = HISEE_OPEN_FILE_ERROR;
	    set_errno_and_return(ret);
	}

	old_fs = get_fs();/*lint !e501*/
	set_fs(KERNEL_DS);/*lint !e501*/
	fp = filp_open(fullname, O_RDONLY, 0600);
	if (IS_ERR(fp)) {
		pr_err("%s():open %s failed\n", __func__, fullname);
		ret = HISEE_OPEN_FILE_ERROR;
		set_fs(old_fs);
		set_errno_and_return(ret);
	}
	pos = fp->f_pos;/*lint !e613*/
	cnt = vfs_read(fp, (char __user *)buffer, HISEE_IMG_HEADER_LEN, &pos);
	if (cnt < (ssize_t)HISEE_IMG_HEADER_LEN) {
		pr_err("%s():read %s failed, return [%ld]\n", __func__, fullname, cnt);
		ret = HISEE_READ_FILE_ERROR;
		goto error;
	}
	fp->f_pos = pos;/*lint !e613*/
	p_img_header = (hisee_img_header *)buffer;
	if (strncmp(p_img_header->magic, HISEE_IMG_MAGIC_VALUE, HISEE_IMG_MAGIC_LEN)) {
		pr_err("%s() hisee_img magic value is wrong.\n", __func__);
		ret = HISEE_IMG_PARTITION_MAGIC_ERROR;
		goto error;
	}
	if (p_img_header->file_cnt > HISEE_IMG_SUB_FILE_MAX) {
		pr_err("%s() hisee_img file numbers is invalid.\n", __func__);
		ret = HISEE_IMG_PARTITION_FILES_ERROR;
		goto error;
	}
	cnt = vfs_read(fp, (char __user *)(buffer + HISEE_IMG_HEADER_LEN), (unsigned long)(unsigned int)(HISEE_IMG_SUB_FILES_LEN * p_img_header->file_cnt), &pos);
	if (cnt < (ssize_t)(unsigned)(HISEE_IMG_SUB_FILES_LEN * p_img_header->file_cnt)) {
		pr_err("%s():read %s failed, return [%ld]\n", __func__, fullname, cnt);
		ret = HISEE_READ_FILE_ERROR;
		goto error;
	}

	ret = check_img_header_is_valid(p_img_header);
	if (HISEE_OK != ret) {
		pr_err("%s(): check_img_header_is_valid fail,ret=%d.\n", __func__, ret);
		goto error;
	}
	/*there is a assumption: the first file in hisee.img is always the cos image 0 , then flowed by cos[,1,2,3,...],
	 *then flowed by MISC image*/
	for (i = 0; i < p_img_header->file_cnt; i++) {
		/*if it is not cos image type, we just continue with next image.*/
		if (strncmp(p_img_header->file[i].name, HISEE_IMG_COS_NAME, strlen(HISEE_IMG_COS_NAME))) {
			continue;
		}
		fp->f_pos = p_img_header->file[i].offset;/*lint !e613*/
		pos = fp->f_pos;/*lint !e613*/
		cnt = vfs_read(fp, (char __user *)cos_img_rawdata, (unsigned long)HISEE_IMG_HEADER_LEN, &pos);
		if (cnt < (ssize_t)HISEE_IMG_HEADER_LEN) {
			pr_err("%s():read %s failed, return [%ld]\n", __func__, fullname, cnt);
			ret = HISEE_READ_FILE_ERROR;
			goto error;
		}
		sw_version_num = *((unsigned int *)(&cos_img_rawdata[HISEE_COS_VERSION_OFFSET]));/*lint !e826*/
		p_img_header->sw_version_cnt[i] = sw_version_num;
	}

error:
	filp_close(fp, NULL);/*lint !e668*/
	set_fs(old_fs);
	set_errno_and_return(ret);
}

static int get_sub_file_name(hisee_img_file_type type, char *sub_file_name)
{
	if (NULL == sub_file_name) {
		return HISEE_INVALID_PARAMS;
	}
	switch (type) {
	case SLOADER_IMG_TYPE:
		strncat(sub_file_name, HISEE_IMG_SLOADER_NAME, HISEE_IMG_SUB_FILE_NAME_LEN);
		break;
	case COS_IMG_TYPE:
		strncat(sub_file_name, HISEE_IMG_COS_NAME, HISEE_IMG_SUB_FILE_NAME_LEN);
		break;
	case OTP_IMG_TYPE:
	case OTP1_IMG_TYPE:
		strncat(sub_file_name, HISEE_IMG_OTP0_NAME, HISEE_IMG_SUB_FILE_NAME_LEN);
		break;
	case MISC0_IMG_TYPE:
	case MISC1_IMG_TYPE:
	case MISC2_IMG_TYPE:
	case MISC3_IMG_TYPE:
	case MISC4_IMG_TYPE:
		strncat(sub_file_name, HISEE_IMG_MISC_NAME, HISEE_IMG_SUB_FILE_NAME_LEN);
		sub_file_name[4] = (char)'0' + (char)(type - MISC0_IMG_TYPE);
		break;
	default:
		return HISEE_IMG_SUB_FILE_NAME_ERROR;
	}
	return HISEE_OK;
}


static int hisee_image_type_chk(hisee_img_file_type type, char *sub_file_name,
									  hisee_img_header *p_hisee_img_head, unsigned int *index)
{
	unsigned int i;
	int ret = HISEE_OK;

	if (NULL == sub_file_name || NULL == p_hisee_img_head || NULL == index) {
		return HISEE_ERROR;
	}

	for (i = 0; i < p_hisee_img_head->file_cnt; i++) {
		if (!p_hisee_img_head->file[i].name[0])
			continue;
		if (OTP_IMG_TYPE == type) {
			if (!strncmp(sub_file_name, p_hisee_img_head->file[i].name, (unsigned long)HISEE_IMG_SUB_FILE_NAME_LEN) ||
				!strncmp(HISEE_IMG_OTP_NAME, p_hisee_img_head->file[i].name, (unsigned long)HISEE_IMG_SUB_FILE_NAME_LEN))
				break;
		} else if (OTP1_IMG_TYPE == type) {
			if (!strncmp(HISEE_IMG_OTP1_NAME, p_hisee_img_head->file[i].name, (unsigned long)HISEE_IMG_SUB_FILE_NAME_LEN))
				break;
		}
		else {
			if (!strncmp(sub_file_name, p_hisee_img_head->file[i].name, (unsigned long)HISEE_IMG_SUB_FILE_NAME_LEN))
				break;
		}
	}
	if (i == p_hisee_img_head->file_cnt) {
		pr_err("%s():image type is %d, sub_file_name is %s\n", __func__, type, sub_file_name);
		pr_err("%s():hisee_read_img_header failed, ret=%d\n", __func__, ret);
		ret = HISEE_IMG_SUB_FILE_ABSENT_ERROR;
	}

	*index = i;

	return ret;
}

/* read hisee_fs partition file interface
* @type: input, the file type need to read in hisee_img partiton
* @buffer: output, save the data
* On success, the number of bytes read is returned (zero indicates end of file),
* On error, the return value is less than zero, please check the errcode in hisee module.
*/
int filesys_hisee_read_image(hisee_img_file_type type, char *buffer)
{
	int ret = HISEE_OK;
	hisee_img_header local_img_header;
	char sub_file_name[HISEE_IMG_SUB_FILE_NAME_LEN] = {0};
	char fullname[MAX_PATH_NAME_LEN + 1] = { 0 };
	unsigned int index;

	if (NULL == buffer) {
		set_errno_and_return(HISEE_INVALID_PARAMS);
	}

	ret = get_sub_file_name(type, sub_file_name);
	if (HISEE_OK != ret) {
		set_errno_and_return(ret);
	}

	memset((void *)(unsigned long)&local_img_header, 0, sizeof(hisee_img_header));
	mutex_lock(&g_hisee_data.hisee_mutex);
	/*Notice:why there still use g_hisee_data.hisee_img_head not use locl_img_header,
	 *Because g_hisee_data.hisee_img_head need to initiliaze than be accessed in otherwhere
	 */
	memset((void *)(unsigned long)&(g_hisee_data.hisee_img_head), 0, sizeof(hisee_img_header));
	ret = hisee_parse_img_header((char *)(unsigned long)&(g_hisee_data.hisee_img_head));
	if (HISEE_OK != ret) {
		pr_err("%s():hisee_read_img_header failed, ret=%d\n", __func__, ret);
		mutex_unlock(&g_hisee_data.hisee_mutex);
		set_errno_and_return(ret);
	}
	memcpy((void *)(unsigned long)&local_img_header,
			(void *)(unsigned long)&(g_hisee_data.hisee_img_head), sizeof(hisee_img_header));
	mutex_unlock(&g_hisee_data.hisee_mutex);

	ret = hisee_image_type_chk(type, sub_file_name, &local_img_header, &index);
	if (HISEE_OK != ret) {
		set_errno_and_return(ret);
	}

	ret = flash_find_ptn(HISEE_IMAGE_PARTITION_NAME, fullname);
	if (0 != ret) {
		pr_err("%s():flash_find_ptn fail\n", __func__);
		ret = HISEE_OPEN_FILE_ERROR;
		atomic_set(&g_hisee_errno, ret);/*lint !e1058*/
		return ret;
	}

	ret = hisee_read_file((const char *)fullname, buffer,
							local_img_header.file[index].offset,
							local_img_header.file[index].size);
	if (ret < HISEE_OK) {
		pr_err("%s():hisee_read_file failed, ret=%d\n", __func__, ret);
		atomic_set(&g_hisee_errno, ret);/*lint !e1058*/
		return ret;
	}
	ret = local_img_header.file[index].size;
	atomic_set(&g_hisee_errno, HISEE_OK);/*lint !e1058*/
	return ret;
}


/*************************************************************
函数原型：int filesys_read_img_from_file(const char *filename, char *buffer, size_t *file_size, size_t max_read_size)
函数功能：读取指定的cos镜像文件(ext4文件系统格式)到指定的buffer中
参数：
输入：filename:指定的cos镜像文件。
输出：buffer:指向保存读取镜像数据的buffer，file_size:读取cos镜像数据的大小，以字节为单位
返回值：HISEE_OK:读取指定文件的镜像数据成功；非HISEE_OK:失败
前置条件：当前encos分区已经保存了cos_flash镜像数据，cos_flash镜像数据不超过max_read_size字节
后置条件： 无
*************************************************************/
int filesys_read_img_from_file(const char *filename, char *buffer, size_t *file_size, size_t max_read_size)
{
	int ret = HISEE_OK;
	mm_segment_t old_fs;
	struct kstat m_stat;

	if (NULL == buffer || NULL == filename || NULL == file_size) {
		return HISEE_INVALID_PARAMS;
	}
	old_fs = get_fs();/*lint !e501*/
	set_fs(KERNEL_DS);/*lint !e501*/
	if(0 != sys_access(filename, 0)) {
		pr_err("hisee:%s(): %s is not exist.\n", __func__, filename);
		set_fs(old_fs);
		return HISEE_MULTICOS_COS_FLASH_FILE_ERROR;
	}

	ret = vfs_stat(filename, &m_stat);
	if (ret) {
		set_fs(old_fs);
		return HISEE_INVALID_PARAMS;
	}
	set_fs(old_fs);

	if (0 == (size_t)(m_stat.size) || (size_t)(m_stat.size) > max_read_size) {
		pr_err("hisee:%s(): file size is more than %u bytes.\n",
				__func__, (unsigned int)max_read_size);
		return HISEE_SUB_FILE_SIZE_CHECK_ERROR;
	}
	*file_size = (size_t)(m_stat.size);

	ret = hisee_read_file((const char *)filename, buffer, 0, (*file_size));
	if (ret < HISEE_OK) {
		pr_err("hisee:%s():hisee_read_file failed, ret=%d\n", __func__, ret);
		return ret;
	}
	return HISEE_OK;
}





static void access_hisee_file_prepare(hisee_image_a_access_type access_type, int *flags, long *file_offset, unsigned long *size)
{
	if ((SW_VERSION_WRITE_TYPE == access_type)
		|| (COS_UPGRADE_RUN_WRITE_TYPE == access_type)
		|| (MISC_VERSION_WRITE_TYPE == access_type)
		|| (COS_UPGRADE_INFO_WRITE_TYPE == access_type))
		*flags = O_WRONLY;
	else
		*flags = O_RDONLY;

	if ((SW_VERSION_WRITE_TYPE == access_type)
		|| (SW_VERSION_READ_TYPE == access_type)) {
		*file_offset = HISEE_IMG_PARTITION_SIZE - SIZE_1K;
		*size = sizeof(cosimage_version_info);
	} else if ((COS_UPGRADE_RUN_WRITE_TYPE == access_type)
		|| (COS_UPGRADE_RUN_READ_TYPE == access_type)){
		*file_offset = (long)((HISEE_IMG_PARTITION_SIZE - SIZE_1K) + HISEE_COS_VERSION_STORE_SIZE);
		*size = sizeof(unsigned int);
	} else if ((MISC_VERSION_READ_TYPE == access_type)
					|| (MISC_VERSION_WRITE_TYPE == access_type)){
		*file_offset = (long)((HISEE_IMG_PARTITION_SIZE - SIZE_1K) + HISEE_COS_VERSION_STORE_SIZE
						+ HISEE_UPGRADE_STORE_SIZE);
		*size = sizeof(cosimage_version_info);
	} else {/*for COS_UPGRADE_INFO_READ_TYPE and COS_UPGRADE_INFO_WRITE_TYPE*/
		*file_offset = (long)((HISEE_IMG_PARTITION_SIZE - SIZE_1K) + HISEE_COS_VERSION_STORE_SIZE
													+ HISEE_UPGRADE_STORE_SIZE
													+ HISEE_MISC_VERSION_STORE_SIZE);
		*size = sizeof(multicos_upgrade_info);
	}

	return;
}


int hisee_get_partition_path(char full_path[MAX_PATH_NAME_LEN])
{
	int ret;

	if (1 == g_hisee_partition_byname_find) {
		ret = flash_find_ptn(HISEE_IMAGE_PARTITION_NAME, full_path);
		if (0 != ret) {
			pr_err("%s():flash_find_ptn fail\n", __func__);
			ret = HISEE_OPEN_FILE_ERROR;
		} else {
			ret = HISEE_OK;
		}
	} else {
		flash_find_hisee_ptn(HISEE_IMAGE_A_PARTION_NAME, full_path);
		ret = HISEE_OK;
	}
	return ret;
}


int access_hisee_image_partition(char *data_buf, hisee_image_a_access_type access_type)
{
	int fd;
	ssize_t cnt;
	mm_segment_t old_fs;
	char fullpath[MAX_PATH_NAME_LEN] = {0};
	long file_offset;
	unsigned long size;
	int ret;
	int flags;

	if (NULL == data_buf) {
		set_errno_and_return(HISEE_INVALID_PARAMS);
	}

	ret = hisee_get_partition_path(fullpath);
	if (HISEE_OK != ret) {
		set_errno_and_return(HISEE_INVALID_PARAMS);
	}

	old_fs = get_fs();/*lint !e501*/
	set_fs(KERNEL_DS);/*lint !e501*/

	access_hisee_file_prepare(access_type, &flags, &file_offset, &size);

	fd = (int)sys_open(fullpath, flags, HISEE_FILESYS_DEFAULT_MODE);
	if (fd < 0) {
		pr_err("%s():open %s failed %d\n", __func__, fullpath, fd);
		ret = HISEE_OPEN_FILE_ERROR;
		set_fs(old_fs);
		set_errno_and_return(ret);
	}

	ret = (int)sys_lseek((unsigned int)fd, file_offset, SEEK_SET);
	if (ret < 0) {
		pr_err("%s(): sys_lseek failed,ret=%d.\n", __func__, ret);
		ret = HISEE_LSEEK_FILE_ERROR;
		sys_close((unsigned int)fd);
		set_fs(old_fs);
		set_errno_and_return(ret);
	}
	ret = HISEE_OK;
	if (HISEE_IS_WRITE_ACCESS(access_type)) {
		cnt = sys_write((unsigned int)fd, (char __user *)data_buf, size);
		ret = sys_fsync((unsigned int)fd);
		if (ret < 0) {
			pr_err("%s():fail to sync %s.\n", __func__, fullpath);
			ret = HISEE_ENCOS_SYNC_FILE_ERROR;
		}
	} else {
		cnt = sys_read((unsigned int)fd, (char __user *)data_buf, size);
	}

	if (cnt < (ssize_t)(size)) {
		pr_err("%s(): access %s failed, return [%ld]\n", __func__, fullpath, cnt);
		ret = HISEE_ACCESS_FILE_ERROR;
	}

	sys_close((unsigned int)fd);
	set_fs(old_fs);
	set_errno_and_return(ret);
}
