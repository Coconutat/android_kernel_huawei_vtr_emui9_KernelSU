/*
 * Synaptics TCM touchscreen driver
 *
 * Copyright (C) 2017-2018 Synaptics Incorporated. All rights reserved.
 *
 * Copyright (C) 2017-2018 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION DOES
 * NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS'
 * TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S.
 * DOLLARS.
 */

#include <linux/gpio.h>
#include <linux/crc32.h>
#include <linux/firmware.h>
#include "synaptics_tcm_core.h"

#define FW_IMAGE_NAME "ts/hdl_firmware.img"
#define BOOT_CONFIG_ID "BOOT_CONFIG"
#define F35_APP_CODE_ID "F35_APP_CODE"
#define APP_CONFIG_ID "APP_CONFIG"
#define DISP_CONFIG_ID "DISPLAY"
#define IMAGE_FILE_MAGIC_VALUE 0x4818472b
#define FLASH_AREA_MAGIC_VALUE 0x7c05e516
#define F35_CTRL3_OFFSET 18
#define F35_CTRL7_OFFSET 22
#define F35_WRITE_FW_TO_PMEM_COMMAND 4
#define DOWNLOAD_RETRY_COUNT 10
#define READ_OUT_IDENTIFY_MS 20
#define ZEROFLASH_DOWNLOAD_DELAYUS 2000
extern struct ts_kit_platform_data g_ts_kit_platform_data;

enum SPI_COM_MODE {
	INTERRUPT_MODE = 0,
	POLLING_MODE,
	DMA_MODE,
};

enum f35_error_code {
	SUCCESS = 0,
	UNKNOWN_FLASH_PRESENT,
	MAGIC_NUMBER_NOT_PRESENT,
	INVALID_BLOCK_NUMBER,
	BLOCK_NOT_ERASED,
	NO_FLASH_PRESENT,
	CHECKSUM_FAILURE,
	WRITE_FAILURE,
	INVALID_COMMAND,
	IN_DEBUG_MODE,
	INVALID_HEADER,
	REQUESTING_FIRMWARE,
	INVALID_CONFIGURATION,
	DISABLE_BLOCK_PROTECT_FAILURE,
};

enum config_download {
	HDL_INVALID = 0,
	HDL_TOUCH_CONFIG_TO_PMEM,
	HDL_DISPLAY_CONFIG_TO_PMEM,
	HDL_DISPLAY_CONFIG_TO_RAM,
};

struct area_descriptor {
	unsigned char magic_value[4];
	unsigned char id_string[16];
	unsigned char flags[4];
	unsigned char flash_addr_words[4];
	unsigned char length[4];
	unsigned char checksum[4];
};

struct block_data {
	const unsigned char *data;
	unsigned int size;
	unsigned int flash_addr;
};

struct image_info {
	unsigned int packrat_number;
	struct block_data boot_config;
	struct block_data app_firmware;
	struct block_data app_config;
	struct block_data disp_config;
};

struct image_header {
	unsigned char magic_value[4];
	unsigned char num_of_areas[4];
};

struct rmi_f35_query {
	unsigned char version:4;
	unsigned char has_debug_mode:1;
	unsigned char has_data5:1;
	unsigned char has_query1:1;
	unsigned char has_query2:1;
	unsigned char chunk_size;
	unsigned char has_ctrl7:1;
	unsigned char has_host_download:1;
	unsigned char has_spi_master:1;
	unsigned char advanced_recovery_mode:1;
	unsigned char reserved:4;
} __packed;

struct rmi_f35_data {
	unsigned char error_code:5;
	unsigned char recovery_mode_forced:1;
	unsigned char nvm_programmed:1;
	unsigned char in_recovery:1;
} __packed;

struct rmi_pdt_entry {
	unsigned char query_base_addr;
	unsigned char command_base_addr;
	unsigned char control_base_addr;
	unsigned char data_base_addr;
	unsigned char intr_src_count:3;
	unsigned char reserved_1:2;
	unsigned char fn_version:2;
	unsigned char reserved_2:1;
	unsigned char fn_number;
} __packed;

struct rmi_addr {
	unsigned short query_base;
	unsigned short command_base;
	unsigned short control_base;
	unsigned short data_base;
};

struct firmware_status {
	unsigned short invalid_static_config:1;
	unsigned short need_disp_config:1;
	unsigned short need_app_config:1;
	unsigned short reserved:13;
} __packed;

struct zeroflash_hcd {
	bool has_hdl;
	bool f35_ready;
	const unsigned char *image;
	unsigned char *buf;
	const struct firmware *fw_entry;
	struct rmi_addr f35_addr;
	struct image_info image_info;
	struct firmware_status fw_status;
	struct syna_tcm_buffer out;
	struct syna_tcm_buffer resp;
	struct syna_tcm_hcd *tcm_hcd;
};

DECLARE_COMPLETION(zeroflash_remove_complete);

static struct zeroflash_hcd *zeroflash_hcd = NULL;

/**
 * syna_tcm_write_message() - write message to device and receive response
 *
 * @tcm_hcd: handle of core module
 * @command: command to send to device
 * @payload: payload of command
 * @length: length of payload in bytes
 * @resp_buf: buffer for storing command response
 * @resp_buf_size: size of response buffer in bytes
 * @resp_length: length of command response in bytes
 * @response_code: status code returned in command response
 * @polling_delay_ms: delay time after sending command before resuming polling
 *
 * If resp_buf is NULL, raw write mode is used and syna_tcm_raw_write() is
 * called. Otherwise, a command and its payload, if any, are sent to the device
 * and the response to the command generated by the device is read in.
 */

int syna_tcm_write_hdl_message(struct syna_tcm_hcd *tcm_hcd,
		unsigned char command, unsigned char *payload,
		unsigned int length, unsigned char **resp_buf,
		unsigned int *resp_buf_size, unsigned int *resp_length,
		unsigned char *response_code, unsigned int polling_delay_ms)
{
	int retval = NO_ERR;
	unsigned int idx = 0;
	unsigned int chunks = 0;
	unsigned int chunk_space = 0;
	unsigned int xfer_length = 0;
	unsigned int remaining_length = 0;

	if (response_code != NULL)
		*response_code = STATUS_INVALID;

	atomic_set(&tcm_hcd->command_status, CMD_BUSY);
	tcm_hcd->command = command;

	LOCK_BUFFER(tcm_hcd->resp);
	tcm_hcd->resp.buf = *resp_buf;
	tcm_hcd->resp.buf_size = *resp_buf_size;
	tcm_hcd->resp.data_length = 0;
	UNLOCK_BUFFER(tcm_hcd->resp);

	// adding two length bytes as part of payload
	remaining_length = length + 2;

	//available chunk space for payload = total chunk size minus command
	//byte
	if (tcm_hcd->wr_chunk_size == 0)
		chunk_space = remaining_length;
	else
		chunk_space = tcm_hcd->wr_chunk_size - 1;

	chunks = ceil_div(remaining_length, chunk_space);
	chunks = chunks == 0 ? 1 : chunks;

	TS_LOG_DEBUG("Command = 0x%02x\n", command);

	LOCK_BUFFER(tcm_hcd->out);
	for (idx = 0; idx < chunks; idx++) {
		if (remaining_length > chunk_space)
			xfer_length = chunk_space;
		else
			xfer_length = remaining_length;

		retval = syna_tcm_alloc_mem(tcm_hcd,
				&tcm_hcd->out,
				xfer_length + 1);
		if (retval < 0) {
			TS_LOG_ERR("Failed to allocate memory for tcm_hcd->out.buf\n");
			UNLOCK_BUFFER(tcm_hcd->out);
		//	mutex_unlock(&tcm_hcd->rw_ctrl_mutex);
			goto exit;
		}

		if (idx == 0) {
			tcm_hcd->out.buf[0] = command;
			tcm_hcd->out.buf[1] = (unsigned char)length;
			tcm_hcd->out.buf[2] = (unsigned char)(length >> 8);

			if (xfer_length > 2) {
				retval = secure_memcpy(&tcm_hcd->out.buf[3],
						tcm_hcd->out.buf_size - 3,
						payload,
						remaining_length - 2,
						xfer_length - 2);
				if (retval < 0) {
					TS_LOG_ERR("Failed to copy payload\n");
					UNLOCK_BUFFER(tcm_hcd->out);
					//mutex_unlock(&tcm_hcd->rw_ctrl_mutex);
					goto exit;
				}
			}
		} else {
			tcm_hcd->out.buf[0] = CMD_CONTINUE_WRITE;

			retval = secure_memcpy(&tcm_hcd->out.buf[1],
					tcm_hcd->out.buf_size - 1,
					&payload[idx * chunk_space - 2],
					remaining_length,
					xfer_length);
			if (retval < 0) {
				TS_LOG_ERR("Failed to copy payload\n");
				UNLOCK_BUFFER(tcm_hcd->out);
			//	mutex_unlock(&tcm_hcd->rw_ctrl_mutex);
				goto exit;
			}
		}

		retval = syna_tcm_write(tcm_hcd,
				tcm_hcd->out.buf,
				xfer_length + 1);
		if (retval < 0) {
			TS_LOG_ERR("Failed to write to device\n");
			UNLOCK_BUFFER(tcm_hcd->out);
			//(&tcm_hcd->rw_ctrl_mutex);
			goto exit;
		}

		remaining_length -= xfer_length;

		if (chunks > 1)
			usleep_range(500, 1000);
	}
	UNLOCK_BUFFER(tcm_hcd->out);

exit:
	tcm_hcd->command = CMD_NONE;
	atomic_set(&tcm_hcd->command_status, CMD_IDLE);

	return retval;
}

static int zeroflash_check_uboot(void)
{
	int retval = NO_ERR;
	unsigned char fn_number = 0;
	struct rmi_f35_query query;
	struct rmi_pdt_entry p_entry;
	struct syna_tcm_hcd *tcm_hcd = zeroflash_hcd->tcm_hcd;

	retval = syna_tcm_rmi_read(tcm_hcd,
			PDT_END_ADDR,
			&fn_number,
			sizeof(fn_number));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read RMI function number\n");
		return retval;
	}

	if (fn_number != UBL_FN_NUMBER) {
		TS_LOG_ERR("Failed to find F$35\n");
		return -ENODEV;
	}

	if (zeroflash_hcd->f35_ready)
		return 0;

	retval = syna_tcm_rmi_read(tcm_hcd,
			PDT_START_ADDR,
			(unsigned char *)&p_entry,
			sizeof(p_entry));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read PDT entry\n");
		return retval;
	}

	zeroflash_hcd->f35_addr.query_base = p_entry.query_base_addr;
	zeroflash_hcd->f35_addr.command_base = p_entry.command_base_addr;
	zeroflash_hcd->f35_addr.control_base = p_entry.control_base_addr;
	zeroflash_hcd->f35_addr.data_base = p_entry.data_base_addr;

	retval = syna_tcm_rmi_read(tcm_hcd,
			zeroflash_hcd->f35_addr.query_base,
			(unsigned char *)&query,
			sizeof(query));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read F$35 query\n");
		return retval;
	}

	zeroflash_hcd->f35_ready = true;

	if (query.has_query2 && query.has_ctrl7 && query.has_host_download) {
		zeroflash_hcd->has_hdl = true;
	} else {
		TS_LOG_ERR("Host download not supported\n");
		zeroflash_hcd->has_hdl = false;
		return -ENODEV;
	}

	return 0;
}

static int zeroflash_parse_fw_image(void)
{
	unsigned int idx = 0;
	unsigned int addr = 0;
	unsigned int offset = 0;
	unsigned int length = 0;
	unsigned int checksum = 0;
	unsigned int flash_addr = 0;
	unsigned int magic_value = 0;
	unsigned int num_of_areas = 0;
	struct image_header *header = NULL;
	struct image_info *image_info = NULL;
	struct area_descriptor *descriptor = NULL;
	const unsigned char *image = NULL;
	const unsigned char *content = NULL;

	image = zeroflash_hcd->image;
	image_info = &zeroflash_hcd->image_info;
	header = (struct image_header *)image;

	magic_value = le4_to_uint(header->magic_value);
	if (magic_value != IMAGE_FILE_MAGIC_VALUE) {
		TS_LOG_ERR("Invalid image file magic value\n");
		return -EINVAL;
	}

	memset(image_info, 0x00, sizeof(*image_info));

	offset = sizeof(*header);
	num_of_areas = le4_to_uint(header->num_of_areas);

	for (idx = 0; idx < num_of_areas; idx++) {
		addr = le4_to_uint(image + offset);
		descriptor = (struct area_descriptor *)(image + addr);
		offset += 4;

		magic_value = le4_to_uint(descriptor->magic_value);
		if (magic_value != FLASH_AREA_MAGIC_VALUE)
			continue;

		length = le4_to_uint(descriptor->length);
		content = (unsigned char *)descriptor + sizeof(*descriptor);
		flash_addr = le4_to_uint(descriptor->flash_addr_words) * 2;
		checksum = le4_to_uint(descriptor->checksum);

		if (0 == strncmp((char *)descriptor->id_string,
				BOOT_CONFIG_ID,
				strlen(BOOT_CONFIG_ID))) {
			if (checksum != (crc32(~0, content, length) ^ ~0)) {
				TS_LOG_ERR("Boot config checksum error\n");
				return -EINVAL;
			}
			image_info->boot_config.size = length;
			image_info->boot_config.data = content;
			image_info->boot_config.flash_addr = flash_addr;
			TS_LOG_INFO("Boot config size = %d\n",
					length);
			TS_LOG_INFO("Boot config flash address = 0x%08x\n",
					flash_addr);
		} else if (0 == strncmp((char *)descriptor->id_string,
				F35_APP_CODE_ID,
				strlen(F35_APP_CODE_ID))) {
			if (checksum != (crc32(~0, content, length) ^ ~0)) {
				TS_LOG_ERR("Application firmware checksum error\n");
				return -EINVAL;
			}
			image_info->app_firmware.size = length;
			image_info->app_firmware.data = content;
			image_info->app_firmware.flash_addr = flash_addr;
			TS_LOG_INFO("Application firmware size = %d\n",
					length);
			TS_LOG_INFO("Application firmware flash address = 0x%08x\n",
					flash_addr);
		} else if (0 == strncmp((char *)descriptor->id_string,
				APP_CONFIG_ID,
				strlen(APP_CONFIG_ID))) {
			if (checksum != (crc32(~0, content, length) ^ ~0)) {
				TS_LOG_ERR("Application config checksum error\n");
				return -EINVAL;
			}
			image_info->app_config.size = length;
			image_info->app_config.data = content;
			image_info->app_config.flash_addr = flash_addr;
			image_info->packrat_number = le4_to_uint(&content[14]);
			TS_LOG_INFO("Application config size = %d  image_info->packrat_number = %d\n",
					length,image_info->packrat_number);
			TS_LOG_INFO("Application config flash address = 0x%08x\n",
					flash_addr);
		} else if (0 == strncmp((char *)descriptor->id_string,
				DISP_CONFIG_ID,
				strlen(DISP_CONFIG_ID))) {
			if (checksum != (crc32(~0, content, length) ^ ~0)) {
				TS_LOG_ERR("Display config checksum error\n");
				return -EINVAL;
			}
			image_info->disp_config.size = length;
			image_info->disp_config.data = content;
			image_info->disp_config.flash_addr = flash_addr;
			TS_LOG_INFO("Display config size = %d\n",
					length);
			TS_LOG_INFO("Display config flash address = 0x%08x\n",
					flash_addr);
		}
	}

	return 0;
}

int zeroflash_get_fw_image(char *file_name)
{
	int retval = NO_ERR;
	char fw_name[MAX_STR_LEN * 4] = {0};
	struct syna_tcm_hcd *tcm_hcd = NULL;
	int projectid_lenth = 0;

	if (!zeroflash_hcd || !zeroflash_hcd->tcm_hcd)
		return -EINVAL;

	if (zeroflash_hcd->fw_entry != NULL)
		return 0;

	tcm_hcd = zeroflash_hcd->tcm_hcd;
	
	snprintf(fw_name, (MAX_STR_LEN * 4), "ts/%s%s.img", file_name,
		tcm_hcd->tcm_mod_info.project_id_string);
	TS_LOG_INFO("%s file_name name is :%s\n", __func__, fw_name);

	retval = request_firmware(&zeroflash_hcd->fw_entry,
			fw_name, tcm_hcd->pdev->dev.parent);
	if (retval < 0) {
		TS_LOG_ERR("Failed to request %s\n", fw_name);
		return retval;
	}

	TS_LOG_INFO("Firmware image size = %d\n",
			(unsigned int)zeroflash_hcd->fw_entry->size);

	zeroflash_hcd->image = zeroflash_hcd->fw_entry->data;

	retval = zeroflash_parse_fw_image();
	if (retval < 0) {
		TS_LOG_ERR("Failed to parse firmware image\n");
		release_firmware(zeroflash_hcd->fw_entry);
		zeroflash_hcd->fw_entry = NULL;
		zeroflash_hcd->image = NULL;
		return retval;
	}

	return 0;
}

static int zeroflash_download_disp_config(void)
{
	int retval = NO_ERR;
	unsigned char response_code = 0;
	struct image_info *image_info = NULL;
	struct syna_tcm_hcd *tcm_hcd = zeroflash_hcd->tcm_hcd;
	static unsigned int retry_count = 0;

	TS_LOG_INFO("Downloading display config\n");

	image_info = &zeroflash_hcd->image_info;
	if (image_info->disp_config.size == 0) {
		TS_LOG_ERR("No display config in image file\n");
		return -EINVAL;
	}

	LOCK_BUFFER(zeroflash_hcd->out);
	retval = syna_tcm_alloc_mem(tcm_hcd,
			&zeroflash_hcd->out,
			image_info->disp_config.size + 2);
	if (retval < 0) {
		TS_LOG_ERR("Failed to allocate memory for zeroflash_hcd->out.buf\n");
		goto unlock_out;
	}

	zeroflash_hcd->out.buf[0] = 1;
	zeroflash_hcd->out.buf[1] = HDL_DISPLAY_CONFIG_TO_PMEM; // 4320 ramless, write to PMEM //HDL_DISPLAY_CONFIG_TO_RAM;  HDL_DISPLAY_CONFIG_TO_PMEM
	retval = secure_memcpy(&zeroflash_hcd->out.buf[2],
			zeroflash_hcd->out.buf_size - 2,
			image_info->disp_config.data,
			image_info->disp_config.size,
			image_info->disp_config.size);
	if (retval < 0) {
		TS_LOG_ERR("Failed to copy display config data\n");
		goto unlock_out;
	}

	zeroflash_hcd->out.data_length = image_info->disp_config.size + 2;

	LOCK_BUFFER(zeroflash_hcd->resp);
	tcm_hcd->wr_chunk_size = 256;
	retval = syna_tcm_write_hdl_message(tcm_hcd,
			CMD_DOWNLOAD_CONFIG,
			zeroflash_hcd->out.buf,
			zeroflash_hcd->out.data_length,
			&zeroflash_hcd->resp.buf,
			&zeroflash_hcd->resp.buf_size,
			&zeroflash_hcd->resp.data_length,
			&response_code,
			2);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n",
				STR(CMD_DOWNLOAD_CONFIG));
		if (response_code != STATUS_ERROR)
			goto unlock_resp;
		retry_count++;
		if (retry_count > DOWNLOAD_RETRY_COUNT)
			goto unlock_resp;
	} else {
		retry_count = 0;
	}

	TS_LOG_INFO("Display config downloaded\n");

	retval = 0;

unlock_resp:
	UNLOCK_BUFFER(zeroflash_hcd->resp);

unlock_out:
	UNLOCK_BUFFER(zeroflash_hcd->out);

	return retval;
}

static int zeroflash_download_app_config(void)
{
	int retval = NO_ERR;
	unsigned char response_code = 0;
	struct image_info *image_info = NULL;
	struct syna_tcm_hcd *tcm_hcd = zeroflash_hcd->tcm_hcd;
	static unsigned int retry_count = 0;

	TS_LOG_INFO("Downloading application config\n");

	image_info = &zeroflash_hcd->image_info;
	if (image_info->app_config.size == 0) {
		TS_LOG_ERR("No application config in image file\n");
		return -EINVAL;
	}

	LOCK_BUFFER(zeroflash_hcd->out);
	retval = syna_tcm_alloc_mem(tcm_hcd,
			&zeroflash_hcd->out,
			image_info->app_config.size + 2);
	if (retval < 0) {
		TS_LOG_ERR("Failed to allocate memory for zeroflash_hcd->out.buf\n");
		goto unlock_out;
	}

	zeroflash_hcd->out.buf[0] = 1;
	zeroflash_hcd->out.buf[1] = HDL_TOUCH_CONFIG_TO_PMEM;

	retval = secure_memcpy(&zeroflash_hcd->out.buf[2],
			zeroflash_hcd->out.buf_size - 2,
			image_info->app_config.data,
			image_info->app_config.size,
			image_info->app_config.size);
	if (retval < 0) {
		TS_LOG_ERR("Failed to copy application config data\n");
		goto unlock_out;
	}

	zeroflash_hcd->out.data_length = image_info->app_config.size + 2;

	LOCK_BUFFER(zeroflash_hcd->resp);
	tcm_hcd->wr_chunk_size = 256;
	retval = syna_tcm_write_hdl_message(tcm_hcd,
			CMD_DOWNLOAD_CONFIG,
			zeroflash_hcd->out.buf,
			zeroflash_hcd->out.data_length,
			&zeroflash_hcd->resp.buf,
			&zeroflash_hcd->resp.buf_size,
			&zeroflash_hcd->resp.data_length,
			&response_code,
			2);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n",
				STR(CMD_DOWNLOAD_CONFIG));
		if (response_code != STATUS_ERROR)
			goto unlock_resp;
		retry_count++;
		if (retry_count > DOWNLOAD_RETRY_COUNT)
			goto unlock_resp;
	} else {
		retry_count = 0;
	}

	TS_LOG_INFO("Application config downloaded\n");

	retval = 0;

unlock_resp:
	UNLOCK_BUFFER(zeroflash_hcd->resp);

unlock_out:
	UNLOCK_BUFFER(zeroflash_hcd->out);

	return retval;
}

static int zeroflash_download_app_fw(void)
{
	int retval = NO_ERR;
	unsigned char command = 0;
	struct image_info *image_info = NULL;
	struct syna_tcm_hcd *tcm_hcd = zeroflash_hcd->tcm_hcd;

	TS_LOG_INFO("Downloading application firmware\n");

	image_info = &zeroflash_hcd->image_info;
	if (image_info->app_firmware.size == 0) {
		TS_LOG_ERR("No application firmware in image file\n");
		return -EINVAL;
	}

	LOCK_BUFFER(zeroflash_hcd->out);
	retval = syna_tcm_alloc_mem(tcm_hcd,
			&zeroflash_hcd->out,
			image_info->app_firmware.size);
	if (retval < 0) {
		TS_LOG_ERR("Failed to allocate memory for zeroflash_hcd->out.buf\n");
		UNLOCK_BUFFER(zeroflash_hcd->out);
		return retval;
	}

	retval = secure_memcpy(zeroflash_hcd->out.buf,
			zeroflash_hcd->out.buf_size,
			image_info->app_firmware.data,
			image_info->app_firmware.size,
			image_info->app_firmware.size);
	if (retval < 0) {
		TS_LOG_ERR("Failed to copy application firmware data\n");
		UNLOCK_BUFFER(zeroflash_hcd->out);
		return retval;
	}

	zeroflash_hcd->out.data_length = image_info->app_firmware.size;

	gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, GPIO_OUTPUT_HIGH);
	mdelay(1);
	gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, GPIO_OUTPUT_LOW);
	udelay(300);
	gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, GPIO_OUTPUT_HIGH);
	mdelay(21);

	command = F35_WRITE_FW_TO_PMEM_COMMAND;
	retval = syna_tcm_rmi_write(tcm_hcd,
			zeroflash_hcd->f35_addr.control_base + F35_CTRL3_OFFSET,
			&command,
			sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("Failed to write F$35 command\n");
		UNLOCK_BUFFER(zeroflash_hcd->out);
		return retval;
	}

	retval = syna_tcm_rmi_write(tcm_hcd,
			zeroflash_hcd->f35_addr.control_base + F35_CTRL7_OFFSET,
			zeroflash_hcd->out.buf,
			zeroflash_hcd->out.data_length);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write application firmware data\n");
		UNLOCK_BUFFER(zeroflash_hcd->out);
		return retval;
	}

	UNLOCK_BUFFER(zeroflash_hcd->out);

	TS_LOG_INFO("Application firmware downloaded\n");
	return retval;
}

int zeroflash_download_app_firmware(char *file_name)
{
	int retval = NO_ERR;
	struct rmi_f35_data data;
	struct syna_tcm_hcd *tcm_hcd = zeroflash_hcd->tcm_hcd;
	static unsigned int retry_count = 0;
	char testbuf[30] = {0};

	retval = zeroflash_check_uboot();
	if (retval < 0) {
		TS_LOG_ERR("Microbootloader support unavailable\n");
		goto exit;
	}

	atomic_set(&tcm_hcd->host_downloading, 1);
	retval = syna_tcm_rmi_read(tcm_hcd,
			zeroflash_hcd->f35_addr.data_base,
			(unsigned char *)&data,
			sizeof(data));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read F$35 data\n");
		goto exit;
	}

	TS_LOG_ERR("Microbootloader data = 0x%02x\n",
				data);

	if (data.error_code != REQUESTING_FIRMWARE) {
		TS_LOG_ERR("Microbootloader error code = 0x%02x\n",
				data.error_code);
		if (data.error_code != CHECKSUM_FAILURE) {
			retval = -EIO;
			goto exit;
		} else {
			retry_count++;
		}
	} else {
		retry_count = 0;
	}

	retval = zeroflash_get_fw_image(file_name);
	if (retval < 0) {
		TS_LOG_ERR("Failed to get firmware image\n");
		goto exit;
	}

	TS_LOG_INFO("Start of firmware download\n");

	retval = zeroflash_download_app_fw();
	if (retval < 0) {
		TS_LOG_ERR("Failed to download application firmware\n");
		goto exit;
	}

	// read out IDENTIFY report
	msleep(READ_OUT_IDENTIFY_MS);
	retval = syna_tcm_read(tcm_hcd,
			testbuf,
			sizeof(testbuf));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read tcm ID report\n");
		goto exit;
	}
	TS_LOG_INFO("testbuf: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
			testbuf[0],testbuf[1],testbuf[2],testbuf[3],testbuf[4]);

	TS_LOG_INFO("End of firmware download\n");

exit:
	if (retval < 0)
		retry_count++;

	return retval;
}

int check_report_status(void)
{
	int retval = NO_ERR;
	unsigned char status_report[10] = {0};
	int retry = 3;

	while(retry) {	
		retval = syna_tcm_read(zeroflash_hcd->tcm_hcd,
				status_report,
				sizeof(status_report));
		if (retval < 0) {
			TS_LOG_ERR("Failed to read status report\n");
			return retval;
		}
		TS_LOG_INFO("status_report = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
				status_report[0], status_report[1], status_report[2],
				status_report[3], status_report[4], status_report[5]);
		if (status_report[1] != REPORT_STATUS && status_report[1] != STATUS_OK) {
			retry --;
		} else {
			break;
		}
	}
	if((!retry) && (status_report[1] != REPORT_STATUS && status_report[1] != STATUS_OK)) {
		TS_LOG_ERR("fw_status read error \n");
		return -ENODEV;
	}
	memcpy(&zeroflash_hcd->fw_status, &status_report[4],
			sizeof(struct firmware_status));

	TS_LOG_INFO("fw_status = 0x%04x\n",
			status_report[4] | status_report[5] << 8);

	return 0;
}

static int zeroflash_download_config(void)
{
	int retval = NO_ERR;

	TS_LOG_INFO("Start of config download\n");

	if (zeroflash_hcd->fw_status.need_disp_config) {
		retval = zeroflash_download_disp_config();
		if (retval < 0) {
			TS_LOG_ERR("Failed to download display config\n");
			return retval;
		}
		TS_LOG_INFO("End of display config download\n");
		return retval;
	}

	if (zeroflash_hcd->fw_status.need_app_config) {
		retval = zeroflash_download_app_config();
		if (retval < 0) {
			TS_LOG_ERR("Failed to download application config\n");
			return retval;
		}
		TS_LOG_INFO("End of application config download\n");
		return retval;
	}

	return retval;
}

zeroflash_download(char *file_name,struct syna_tcm_hcd *tcm_hcd)
{
	int retval = NO_ERR;
	char retry = 5;

	if(tcm_hcd->use_dma_download_firmware) {
		g_ts_kit_platform_data.spidev0_chip_info.com_mode = DMA_MODE;
		tcm_hcd->spi_comnunicate_frequency = tcm_hcd->downmload_firmware_frequency;
	}
	retval = zeroflash_download_app_firmware(file_name);
	if (retval < 0)
		goto exit;

	udelay(ZEROFLASH_DOWNLOAD_DELAYUS);
	while (retry) {
		udelay(ZEROFLASH_DOWNLOAD_DELAYUS);
		retval = check_report_status();
		if (retval < 0) {
			TS_LOG_ERR("Failed to read report\n");
			goto exit;
		}

		if (!zeroflash_hcd->fw_status.need_app_config & !zeroflash_hcd->fw_status.need_disp_config) {
			TS_LOG_INFO("All download has been done\n");
			retval = 0;
			goto exit;
		}

		retval = zeroflash_download_config();
		if (retval < 0) {
			TS_LOG_ERR("Failed to download config\n");
			//goto exit;
		}
		retry--;
	}
	udelay(ZEROFLASH_DOWNLOAD_DELAYUS);

exit:
	if(tcm_hcd->use_dma_download_firmware) {
		tcm_hcd->spi_comnunicate_frequency = SPI_DEFLAUT_SPEED;
		g_ts_kit_platform_data.spidev0_chip_info.com_mode = POLLING_MODE;
	}
	return retval;
}

int zeroflash_init(struct syna_tcm_hcd *tcm_hcd)
{
	zeroflash_hcd = kzalloc(sizeof(struct zeroflash_hcd), GFP_KERNEL);
	if (!zeroflash_hcd) {
		TS_LOG_ERR("Failed to allocate memory for zeroflash_hcd\n");
		return -ENOMEM;
	}
	zeroflash_hcd->tcm_hcd = tcm_hcd;

	INIT_BUFFER(zeroflash_hcd->out, false);
	INIT_BUFFER(zeroflash_hcd->resp, false);

	return 0;
}

int zeroflash_remove(struct syna_tcm_hcd *tcm_hcd)
{
	if (!zeroflash_hcd)
		goto exit;

	if (zeroflash_hcd->fw_entry)
		release_firmware(zeroflash_hcd->fw_entry);

	RELEASE_BUFFER(zeroflash_hcd->resp);
	RELEASE_BUFFER(zeroflash_hcd->out);

	kfree(zeroflash_hcd);
	zeroflash_hcd = NULL;

exit:
	complete(&zeroflash_remove_complete);
	return 0;
}

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics TCM Zeroflash Module");
MODULE_LICENSE("GPL v2");
