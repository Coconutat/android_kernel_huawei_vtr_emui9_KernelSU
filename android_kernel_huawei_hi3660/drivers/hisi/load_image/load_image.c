/*
 * load_image.c
 *
 * Hisilicon image load driver .
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/hisi/hisi_rproc.h>
#include <linux/hisi/hifidrvinterface.h>
#include <linux/hisi/kirin_partition.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <hisi_partition.h>
#include <partition.h>
#include <teek_client_api.h>
#include <teek_client_id.h>
#include "load_image.h"

#define DEVICE_PATH  "/dev/block/bootdevice/by-name/"
/* 1M Bytes */
#define SECBOOT_BUFLEN  		  (0x100000)
/* use for store vrl or imgdata */
static u8 SECBOOT_BUFFER[SECBOOT_BUFLEN];

typedef enum SVC_SECBOOT_IMG_TYPE SECBOOT_IMG_TYPE;
typedef enum SVC_SECBOOT_CMD_ID SECBOOT_CMD_TYPE;

static struct mutex load_image_lock;
static struct mutex copy_secimg_lock;


/*
 * Function name:TEEK_init.
 * Discription:Init the TEEC and get the context
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ context: context.
 * return value:
 *      @ TEEC_SUCCESS-->success, others-->failed.
 */
static s32 TEEK_init(TEEC_Session *session, TEEC_Context *context)
{
	TEEC_Result result;
	TEEC_UUID svc_uuid = TEE_SERVICE_SECBOOT;
	TEEC_Operation operation = {0};
	char package_name[] = "sec_boot";
	u32 root_id = 0;
	s32 ret = SEC_OK;

	result = TEEK_InitializeContext(
			 NULL,
			 context);

	if (result != TEEC_SUCCESS) {
		sec_print_err("TEEK_InitializeContext failed, result=0x%x!\n", result);
		ret = SEC_ERROR;
		goto error;
	}

	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
				       TEEC_NONE,
				       TEEC_NONE,
				       TEEC_MEMREF_TEMP_INPUT,
				       TEEC_MEMREF_TEMP_INPUT);
	operation.params[2].tmpref.buffer = (void *)(&root_id);
	operation.params[2].tmpref.size = sizeof(root_id);
	operation.params[3].tmpref.buffer = (void *)(package_name);
	operation.params[3].tmpref.size = strlen(package_name) + 1;
	result = TEEK_OpenSession(
			 context,
			 session,
			 &svc_uuid,
			 TEEC_LOGIN_IDENTIFY,
			 NULL,
			 &operation,
			 NULL);

	if (result != TEEC_SUCCESS) {
		sec_print_err("TEEK_OpenSession failed, result=0x%x!\n", result);
		ret = SEC_ERROR;
		TEEK_FinalizeContext(context);
	}

error:

	return ret;
}


/*
 * Function name:reset_soc_image.
 * Discription:reset the core in sec_OS, like modem and hifi core  .
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ image: the core to reset.
 * return value:
 *      @ TEEC_SUCCESS-->success, others-->failed.
 */
s32 reset_soc_image(TEEC_Session *session,
		    SECBOOT_IMG_TYPE  image)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 origin = 0;
	s32 ret = SEC_OK;

	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
				       TEEC_VALUE_INPUT,
				       TEEC_NONE,
				       TEEC_NONE,
				       TEEC_NONE);

	operation.params[0].value.a = image; /*MODEM/HIFI*/
	result = TEEK_InvokeCommand(
			 session,
			 SECBOOT_CMD_ID_RESET_IMAGE ,
			 &operation,
			 &origin);
	if (result != TEEC_SUCCESS) {
		sec_print_err("reset  failed, result is 0x%x!\n", result);
		ret = SEC_ERROR;
	}

	return ret;
}

/*
 * Function name:start_soc_image.
 * Discription:start the image verification, if success, unreset the soc
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ image: the image to verification and unreset
 *      @ run_addr: the image entry address
 * return value:
 *      @ TEEC_SUCCESS-->success, others-->failed.
 */
s32 verify_soc_image(TEEC_Session *session,
		     SECBOOT_IMG_TYPE  image,
		     u64 run_addr, SECBOOT_CMD_TYPE type)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 origin = 0;
	s32 ret = SEC_OK;

	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
				       TEEC_VALUE_INPUT,
				       TEEC_VALUE_INPUT,
				       TEEC_NONE,
				       TEEC_NONE);

	operation.params[0].value.a = image;
	operation.params[0].value.b = 0;/*SECBOOT_LOCKSTATE , not used currently*/
	operation.params[1].value.a = (u32)run_addr;
	operation.params[1].value.b = run_addr >> 32;
	result = TEEK_InvokeCommand(
			 session,
			 type,
			 &operation,
			 &origin);
	if (result != TEEC_SUCCESS) {
		sec_print_err("start  failed, result is 0x%x!\n", result);
		ret = SEC_ERROR;
	}
	return ret;
}

/*
 * Function name:copy_img_from_os.
 * Discription:copy img data from secure os to run addr, if success, unreset the soc
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ image: the image to run and unreset
 *      @ run_addr: the image entry address
 * return value:
 *      @ TEEC_SUCCESS-->success, others-->failed.
 */

static s32 copy_img_from_os(TEEC_Session *session,
		     SECBOOT_IMG_TYPE  image, u64 run_addr)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 origin = 0;
	s32 ret = SEC_OK;

	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
				       TEEC_VALUE_INPUT,
				       TEEC_VALUE_INPUT,
				       TEEC_NONE,
				       TEEC_NONE);

	operation.params[0].value.a = image;
	operation.params[1].value.a = (u32)run_addr;
	operation.params[1].value.b = run_addr >> 32;

	result = TEEK_InvokeCommand(
			 session,
			 SECBOOT_CMD_ID_COPY_IMG_TYPE,
			 &operation,
			 &origin);

	if (result != TEEC_SUCCESS) {
		sec_print_err("start  failed, result is 0x%x!\n", result);
		ret = SEC_ERROR;
	}
	return ret;
}

/*
 * Function name:trans_vrl_to_os.
 * Discription:transfer vrl data to sec_OS
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ image: the data of the image to transfer.
 *      @ buf: the buf in  kernel to transfer
 *      @ size: the size to transfer.
 * return value:
 *      @ TEEC_SUCCESS-->success, others--> failed.
 */
static s32 trans_vrl_to_os(TEEC_Session *session,
			   SECBOOT_IMG_TYPE  image,
			   void *buf,
			   const unsigned int size)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 origin = 0;
	s32 ret = SEC_OK;

	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
				       TEEC_VALUE_INPUT,
				       TEEC_MEMREF_TEMP_INPUT,
				       TEEC_NONE,
				       TEEC_NONE);

	operation.params[0].value.a = image;
	operation.params[1].tmpref.buffer = (void *)buf;
	operation.params[1].tmpref.size = size;

	result = TEEK_InvokeCommand(
			 session,
			 SECBOOT_CMD_ID_COPY_VRL_TYPE,
			 &operation,
			 &origin);
	if (result != TEEC_SUCCESS) {
		sec_print_err("invoke failed, result is 0x%x!\n", result);
		ret = SEC_ERROR;
	}

	return ret;
}

/*
 * Function name:trans_data_to_os.
 * Discription:transfer image data to sec_OS
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ image: the data of the image to transfer.
 *      @ run_addr: the image entry address.
 *      @ buf: the buf in  kernel to transfer
 *      @ offset: the offset to run_addr.
 *      @ size: the size to transfer.
 * return value:
 *      @ TEEC_SUCCESS-->success, others--> failed.
 */
static s32 trans_data_to_os(TEEC_Session *session,
			    SECBOOT_IMG_TYPE  image,
			    u64 run_addr,
			    void *buf,
			    const unsigned int offset,
			    const unsigned int size)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 origin = 0;
	s32 ret = SEC_OK;

	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
				       TEEC_VALUE_INPUT,
				       TEEC_VALUE_INPUT,
				       TEEC_VALUE_INPUT,
				       TEEC_VALUE_INPUT);

	operation.params[0].value.a = image;
	operation.params[0].value.b = (u32)run_addr;
	operation.params[1].value.a = run_addr >> 32;
	operation.params[1].value.b = offset;
	operation.params[2].value.a = (u32)virt_to_phys(buf);
	operation.params[2].value.b = virt_to_phys(buf) >> 32;
	operation.params[3].value.a = size;

	result = TEEK_InvokeCommand(
			 session,
			 SECBOOT_CMD_ID_COPY_DATA_TYPE,
			 &operation,
			 &origin);
	if (result != TEEC_SUCCESS) {
		sec_print_err("invoke failed, result is 0x%x!\n", result);
		ret = SEC_ERROR;
	}

	return ret;
}

int bsp_read_bin(const char *partion_name, unsigned int offset,
		 unsigned int length, char *buffer)
{
	int ret = SEC_ERROR;
	char *pathname;
	unsigned long pathlen;
	struct file *fp;

	if ((NULL == partion_name) || (NULL == buffer)) {
		sec_print_err("partion_name(%pK) or buffer(%pK) is null!\n", partion_name, buffer);
		return ret;
	}

	/*get resource*/
	pathlen = sizeof(DEVICE_PATH) + strnlen(partion_name,
						(unsigned long)PART_NAMELEN);
	pathname = kzalloc(pathlen, GFP_KERNEL);
	if (!pathname) {
		sec_print_err("pathname kzalloc failed!\n");
		return ret;
	}

	ret = flash_find_ptn((const char *)partion_name, pathname);
	if (0 != ret) {
		sec_print_err("partion_name(%s) not in ptable, ret=0x%x!\n", partion_name, ret);
		ret = SEC_ERROR;
		goto free_pname;
	}

	fp = filp_open(pathname, O_RDONLY, 0400);
	if (IS_ERR(fp)) {
		sec_print_err("filp_open(%s) failed!\n", pathname);
		ret = SEC_ERROR;
		goto free_pname;
	}

	ret = kernel_read(fp, offset, buffer, length);
	if (ret != length) {
		sec_print_err("read ops failed, ret=0x%x, len=0x%x!\n", ret, length);
		ret = SEC_ERROR;
		goto close_file;
	}
	ret = SEC_OK;

close_file:
	filp_close(fp, NULL);

free_pname:
	if (NULL != pathname) {
		kfree(pathname);
		pathname = NULL;
	}

	return ret;

}

/*
 * Function name:load_data_to_os.
 * Discription:cut the  image data to 1M per block, and trans them to  sec_OS.
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 *      @ image: the data of the image to transfer.
 *      @ part_name: image's partition name.
 *      @ run_addr: the image entry address.
 *      @ offset: the offset to run_addr.
 *      @ read_size: total  size of the image.
 * return value:
 *      @ SEC_OK-->success, others--> failed.
 */
static s32 load_data_to_os(TEEC_Session *session,
			   SECBOOT_IMG_TYPE  image,
			   char *part_name,
			   u64 run_addr,
			   u32 offset,
			   u32 read_size)
{
	u32 read_bytes;
	u32 remaining_bytes;
	u32 timers;
	u32 i;
	s32 ret = SEC_ERROR;

	/* make size aligned with 64 bytes, kernel ALIGN is align_up default */
	read_size = ALIGN(read_size, 64U);

	/*split the size to be read to each 1M bytes.*/
	timers = DIV_ROUND_UP(read_size, SECBOOT_BUFLEN);

	remaining_bytes = read_size;

	mutex_lock(&load_image_lock);
	/* call flash_read each time to read to memDst. */
	for (i = 0; i < timers; i++) {
		if (remaining_bytes >= SECBOOT_BUFLEN)
			read_bytes = SECBOOT_BUFLEN;
		else
			read_bytes = remaining_bytes;

		if (bsp_read_bin(part_name, offset + i * SECBOOT_BUFLEN, read_bytes,
				 (void *)SECBOOT_BUFFER)) {
			sec_print_err("flash_read is failed!\n");
			mutex_unlock(&load_image_lock);
			return SEC_ERROR;
		}

		ret = trans_data_to_os(session, image, run_addr, (void *)(SECBOOT_BUFFER),
				       (i * SECBOOT_BUFLEN), read_bytes);
		if (SEC_ERROR == ret) {
			sec_print_err("image trans to os is failed, ret=0x%x!\n", ret);
			mutex_unlock(&load_image_lock);
			return SEC_ERROR;
		}

		remaining_bytes -= read_bytes;
	}
	mutex_unlock(&load_image_lock);

	if (0 != remaining_bytes) {
		sec_print_err("remaining_bytes=0x%x!\n", remaining_bytes);
		return SEC_ERROR;
	}

	return SEC_OK;
}

/*
 * Function name:load_vrl_from_partition_to_os.
 * Discription:load modem  image's VRL  data to sec_OS
 * Parameters:
 *      @ session: the bridge from unsec world to sec world.
 * return value:
 *      @ SEC_OK--> success, others--> failed.
 */
s32 load_vrl_to_os(TEEC_Session *session,
		   SECBOOT_IMG_TYPE ecoretype,
		   char *partion_name,
		   u32 vrl_type)
{
	s32 ret;
	int vrl_index = 0;
	char image_name[PART_NAMELEN] = {0};

	if (NULL != partion_name) {
		vrl_index = flash_get_ptn_index(partion_name);
		if (vrl_index < 0) {
			sec_print_err("fail to find image ptn!\n");
			return SEC_ERROR;
		}
	} else {
		sec_print_err("image type is error!\n");
		return SEC_ERROR;
	}

	switch (vrl_type) {
	case PrimVRL:
		strncpy(image_name, PART_VRL, (unsigned long)PART_NAMELEN);
		image_name[PART_NAMELEN - 1] = '\0';
		break;
	case BackVRL:
		strncpy(image_name, PART_VRL_BACKUP, (unsigned long)PART_NAMELEN);
		image_name[PART_NAMELEN - 1] = '\0';
		break;
	default:
		sec_print_err("vrl type is error!\n");
		return SEC_ERROR;
	}

	mutex_lock(&load_image_lock);
	/*get image vrl info*/
	ret  = bsp_read_bin((const char *)image_name, (vrl_index * VRL_SIZE), VRL_SIZE,
			    (void *)SECBOOT_BUFFER);
	if (ret < 0) {
		sec_print_err("fail to read the vrl of image, ret=0x%x!\n", ret);
		mutex_unlock(&load_image_lock);
		return SEC_ERROR;
	}

	/* trans the vrl to secure os. */
	ret = trans_vrl_to_os(session, ecoretype, (void *)SECBOOT_BUFFER, VRL_SIZE);
	if (SEC_ERROR == ret) {
		sec_print_err("image vrl trans to secureos is failed, ret=0x%x!\n",
			      ret);
		mutex_unlock(&load_image_lock);
		return SEC_ERROR;
	}
	mutex_unlock(&load_image_lock);

	return SEC_OK;
}

/*
 * Function name:bsp_load_and_verify_image.
 * Discription:load the image to secureOS and the SecureOS verify it.
 * Parameters:
 *      @ void.
 *Called in:
 *      @ ccorereset_task
 * return value:
 *      @ SEC_OK-->success, others-->failed.
 */
int bsp_load_and_verify_image(struct load_image_info *img_info)
{
	TEEC_Session session;
	TEEC_Context context;
	s32 ret = SEC_ERROR;
	u32 vrl_type;


	if (NULL == img_info) {
		sec_print_err("img_info is null!\n");
		return ret;
	}

	ret = TEEK_init(&session, &context);
	if (SEC_OK != ret) {
		sec_print_err("TEEK_InitializeContext failed!\n");
		return ret;
	}

	for (vrl_type = PrimVRL; vrl_type <= BackVRL; vrl_type++) {
		ret = reset_soc_image(&session, (SECBOOT_IMG_TYPE)img_info->ecoretype);
		if (SEC_OK != ret) {
			sec_print_err("reset_soc_image fail!\n");
			goto err_out;
		}

		ret = load_vrl_to_os(&session, (SECBOOT_IMG_TYPE)img_info->ecoretype,
				     img_info->partion_name, vrl_type);
		if (SEC_OK != ret) {
			sec_print_err("load_vrl_to_os fail!\n");
			goto err_out;
		}

		ret = load_data_to_os(&session, (SECBOOT_IMG_TYPE)img_info->ecoretype,
				      img_info->partion_name,
				      img_info->image_addr, 0, img_info->image_size);
		if (SEC_OK != ret) {
			sec_print_err("load %s data to secureos fail, ret=0x%x!\n",
				      img_info->partion_name, ret);
			goto err_out;
		}

		/*end of trans all data, start verify, if success, unreset the SOC*/
		ret = verify_soc_image(&session, (SECBOOT_IMG_TYPE)img_info->ecoretype,
				       img_info->image_addr, SECBOOT_CMD_ID_VERIFY_DATA_TYPE);
		if (SEC_OK != ret) {
			if (PrimVRL == vrl_type) {
				sec_print_err("verify_soc_image primvrl verify failed!\n");
			} else {
				sec_print_err("verify_soc_image prim and back vrl both verify failed!\n");
				goto err_out;
			}
		} else {
			sec_print_info("success!\n");
			break;
		}
	}
err_out:
	TEEK_CloseSession(&session);
	TEEK_FinalizeContext(&context);

	return ret;
}

/*
 * Function name:bsp_load_sec_img.
 * Discription:load img from teeos to img run addr
 * Parameters:
 *      @ img_info
 *Called in:
 *      @ ccorereset_task
 * return value:
 *      @ SEC_OK-->success, others-->failed.
 */
int bsp_load_sec_img(struct load_image_info *img_info)
{
	TEEC_Session session;
	TEEC_Context context;
	s32 ret = SEC_ERROR;

	mutex_lock(&copy_secimg_lock);

	if (NULL == img_info) {
		sec_print_err("img_info is null!\n");
		mutex_unlock(&copy_secimg_lock);
		return ret;
	}

	ret = TEEK_init(&session, &context);
	if (SEC_OK != ret) {
		sec_print_err("TEEK_InitializeContext failed!\n");
		mutex_unlock(&copy_secimg_lock);
		return ret;
	}

	ret = reset_soc_image(&session, (SECBOOT_IMG_TYPE)img_info->ecoretype);
	if (SEC_OK != ret) {
		sec_print_err("reset_soc_image fail!\n");
		goto err_out;
	}

	/*copy img from tee to img run addr, if success, unreset soc*/
	ret = copy_img_from_os(&session, (SECBOOT_IMG_TYPE)img_info->ecoretype,
					img_info->image_addr);
	if (SEC_OK != ret) {
		sec_print_err("copy_img_from_os fail!\n");
		goto err_out;
	}

err_out:
	TEEK_CloseSession(&session);
	TEEK_FinalizeContext(&context);
	mutex_unlock(&copy_secimg_lock);

	return ret;
}

static int __init load_image_init(void)
{
	mutex_init(&load_image_lock);
	mutex_init(&copy_secimg_lock);
	return SEC_OK;
}

/*
 * Function name:bsp_reset_core_notify.
 * Description:notify the remote processor MODEM is going to reset.
 * Parameters:
 *      @ ecoretype: the core to be notified.
 *      @ cmdtype: to send to remote processor.
 *      @ timeout_ms: max time to wait, ms.
 *      @ retval: the ack's value get from the remote processor.
 *Called in:
 *      @ modem is going to reset. <reset_balong.c>
 *Return value:
 *      @ BSP_RESET_NOTIFY_REPLY_OK-->the remote processor give response in time.
 *      @ BSP_RESET_NOTIFY_SEND_FAILED-->the parameter is wrong or other rproc_sync_send's self error.
 *      @ BSP_RESET_NOTIFY_TIMEOUT-->after wait timeout_ms's time, the remote processor give no response.
*/
int bsp_reset_core_notify(BSP_CORE_TYPE_E ecoretype, unsigned int cmdtype,
			  unsigned int timeout_ms, unsigned int *retval)
{
	int retry;
	int ret = BSP_RESET_NOTIFY_REPLY_OK;
	rproc_id_t rproc_id;
	rproc_msg_t tx_buffer[2];
	rproc_msg_t ack_buffer[2];

	if (BSP_HIFI == ecoretype) {
		tx_buffer[0] = 32 << 8;/*the INT_SRC_NUM to hifi*/
		rproc_id = HISI_RPROC_HIFI_MBX29;
	} else {
		sec_print_err("wrong ecoretype\n");
		return BSP_RESET_NOTIFY_SEND_FAILED;
	}

	/*300ms's timeout is fixed in RPROC_SYNC_SEND,MAILBOX_MANUACK_TIMEOUT is jiffies value */
	retry = msecs_to_jiffies(timeout_ms) / MAILBOX_MANUACK_TIMEOUT + 1;
	tx_buffer[1] = cmdtype;

	do {
		ret = RPROC_SYNC_SEND(rproc_id, tx_buffer, 2, ack_buffer, 2);
		if (0 == ret) {
			/*the send is reponsed by the remote process, break out*/
			*retval = ack_buffer[1];
			break;
		} else if (-ETIMEOUT == ret) {
			/*the timeout will print out, below message to tell it's normal*/
			retry--;
			ret = BSP_RESET_NOTIFY_TIMEOUT;
			sec_print_err("the remote process is getting up, the ipc timeout is normal\n");
			continue;
		} else {
			ret = BSP_RESET_NOTIFY_SEND_FAILED;
			sec_print_err("bad parameter or other error\n");
			break;
		}
	} while (retry);

	return ret;
}

fs_initcall_sync(load_image_init);
MODULE_DESCRIPTION("Hisilicon Load image driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
