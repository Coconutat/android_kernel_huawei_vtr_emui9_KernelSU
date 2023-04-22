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

#include <linux/crc32.h>
#include <linux/firmware.h>
#include "synaptics_tcm_core.h"

//#define STARTUP_REFLASH

#define FORCE_REFLASH false

#define DISPLAY_REFLASH false

#define ENABLE_SYSFS_INTERFACE true

#define SYSFS_DIR_NAME "reflash"

#define CUSTOM_DIR_NAME "custom"

#if 1
#define FW_IMAGE_NAME "ts/S3706.img"
#else
#define FW_IMAGE_NAME "PR2756748-s3706_vivo_i2c.img"
#endif

#define BOOT_CONFIG_ID "BOOT_CONFIG"

#define APP_CODE_ID "APP_CODE"

#define APP_CONFIG_ID "APP_CONFIG"

#define DISP_CONFIG_ID "DISPLAY"

#define FB_READY_COUNT 2

#define FB_READY_WAIT_MS 100

#define FB_READY_TIMEOUT_S 30

#define IMAGE_FILE_MAGIC_VALUE 0x4818472b

#define FLASH_AREA_MAGIC_VALUE 0x7c05e516

#define BOOT_CONFIG_SIZE 8

#define BOOT_CONFIG_SLOTS 16

#define IMAGE_BUF_SIZE (512 * 1024)

#define ERASE_FLASH_DELAY_MS 5000

#define WRITE_FLASH_DELAY_MS 30

#define REFLASH (1 << 0)

#define FORCE_UPDATE (1 << 1)

#define APP_CFG_UPDATE (1 << 2)

#define DISP_CFG_UPDATE (1 << 3)

#define BOOT_CFG_UPDATE (1 << 4)

#define BOOT_CFG_LOCKDOWN (1 << 5)

static unsigned char flash_read_data[160] = {0};
static unsigned char boot_config_data[128] = {0};

#define reflash_write(p_name) \
static int reflash_write_##p_name(void) \
{ \
	int retval; \
	unsigned int size; \
	unsigned int flash_addr; \
	const unsigned char *data; \
\
	data = reflash_hcd->image_info.p_name.data; \
	size = reflash_hcd->image_info.p_name.size; \
	flash_addr = reflash_hcd->image_info.p_name.flash_addr; \
\
	retval = reflash_write_flash(flash_addr, data, size); \
	if (retval < 0) { \
		TS_LOG_ERR( \
				"Failed to write to flash\n"); \
		return retval; \
	} \
\
	return 0; \
}
static int reflash_raw_write(struct syna_tcm_hcd *tcm_hcd,
		unsigned char command, unsigned char *data, unsigned int length);
#define reflash_erase(p_name) \
static int reflash_erase_##p_name(void) \
{ \
	int retval; \
	unsigned int size; \
	unsigned int flash_addr; \
	unsigned int page_start; \
	unsigned int page_count; \
\
	flash_addr = reflash_hcd->image_info.p_name.flash_addr; \
\
	page_start = flash_addr / reflash_hcd->page_size; \
\
	size = reflash_hcd->image_info.p_name.size; \
	page_count = ceil_div(size, reflash_hcd->page_size); \
\
	TS_LOG_ERR( \
			"Page start = %d\n", \
			page_start); \
\
	TS_LOG_ERR( \
			"Page count = %d\n", \
			page_count); \
\
	retval = reflash_erase_flash(page_start, page_count); \
	if (retval < 0) { \
		TS_LOG_ERR( \
				"Failed to erase flash pages\n"); \
		return retval; \
	} \
\
	return 0; \
}

#define reflash_update(p_name) \
static int reflash_update_##p_name(bool reset) \
{ \
	int retval; \
\
	retval = reflash_set_up_flash_access(); \
	if (retval < 0) { \
		TS_LOG_ERR( \
				"Failed to set up flash access\n"); \
		return retval; \
	} \
\
\
	retval = reflash_check_##p_name(); \
	if (retval < 0) { \
		TS_LOG_ERR( \
				"Failed "#p_name" partition check\n"); \
		reset = true; \
		goto reset; \
	} \
\
	retval = reflash_erase_##p_name(); \
	if (retval < 0) { \
		TS_LOG_ERR( \
				"Failed to erase "#p_name" partition\n"); \
		reset = true; \
		goto reset; \
	} \
\
	TS_LOG_ERR( \
			"Partition erased ("#p_name")\n"); \
\
	retval = reflash_write_##p_name(); \
	if (retval < 0) { \
		TS_LOG_ERR( \
				"Failed to write "#p_name" partition\n"); \
		reset = true; \
		goto reset; \
	} \
\
	TS_LOG_ERR( \
			"Partition written ("#p_name")\n"); \
\
	retval = 0; \
\
reset: \
	if (!reset) \
		goto exit; \
\
	reflash_after_fw_update_do_reset(); \
\
exit: \
\
	return retval; \
}

#define reflash_show_data() \
{ \
	LOCK_BUFFER(reflash_hcd->read); \
\
	readlen = MIN(count, reflash_hcd->read.data_length - pos); \
\
	retval = secure_memcpy(buf, \
			count, \
			&reflash_hcd->read.buf[pos], \
			reflash_hcd->read.buf_size - pos, \
			readlen); \
	if (retval < 0) { \
		TS_LOG_ERR( \
			"Failed to copy read data\n"); \
	} else { \
		retval = readlen; \
	} \
\
	UNLOCK_BUFFER(reflash_hcd->read); \
}

enum update_area {
	NONE = 0,
	FIRMWARE_CONFIG,
	CONFIG_ONLY,
};

struct app_config_header {
	unsigned short magic_value[4];
	unsigned char checksum[4];
	unsigned char length[2];
	unsigned char build_id[4];
	unsigned char customer_config_id[16];
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
	struct block_data boot_config;
	struct block_data app_firmware;
	struct block_data app_config;
	struct block_data disp_config;
};

struct image_header {
	unsigned char magic_value[4];
	unsigned char num_of_areas[4];
};

struct boot_config {
	union {
		unsigned char i2c_address;
		struct {
			unsigned char cpha:1;
			unsigned char cpol:1;
			unsigned char word0_b2__7:6;
		} __packed;
	};
	unsigned char attn_polarity:1;
	unsigned char attn_drive:2;
	unsigned char attn_pullup:1;
	unsigned char word0_b12__14:3;
	unsigned char used:1;
	unsigned short customer_part_id;
	unsigned short boot_timeout;
	unsigned short continue_on_reset:1;
	unsigned short word3_b1__15:15;
} __packed;

struct reflash_hcd {
	bool force_update;
	bool disp_cfg_update;
	const unsigned char *image;
	unsigned char *image_buf;
	unsigned int image_size;
	unsigned int page_size;
	unsigned int write_block_size;
	unsigned int max_write_payload_size;
	const struct firmware *fw_entry;
	struct mutex reflash_mutex;
	struct kobject *sysfs_dir;
	struct kobject *custom_dir;
	struct work_struct work;
	struct workqueue_struct *workqueue;
	struct image_info image_info;
	struct syna_tcm_buffer out;
	struct syna_tcm_buffer resp;
	struct syna_tcm_buffer read;
	struct syna_tcm_hcd *tcm_hcd;
};

static struct reflash_hcd *reflash_hcd;

static int reflash_get_fw_image(char *fw_name);

static int reflash_update_app_config(bool reset);

static int reflash_update_disp_config(bool reset);
int reflash_after_fw_update_do_reset(void);
int reflash_do_reflash(char *fw_name);

unsigned char tmp_response[100] = {0};
static int reflash_set_up_flash_access(void)
{
	int retval = 0;
	unsigned int temp;
	struct syna_tcm_hcd *tcm_hcd = reflash_hcd->tcm_hcd;
	unsigned int max_write_size;

	retval = reflash_raw_write(tcm_hcd,	CMD_IDENTIFY,NULL, 0);
	if (retval < 0) {
		TS_LOG_ERR(
					"in set up reflash 1\n");
		return -EINVAL;
	}
	msleep(50);
	retval = syna_tcm_read(tcm_hcd, tmp_response, sizeof(tmp_response));

	TS_LOG_ERR("response[0] %x, %x, %x, %x, %x, %x, %x\n", tmp_response[0], tmp_response[1], tmp_response[2], tmp_response[3], tmp_response[4], tmp_response[5], tmp_response[6]);
	if (retval < 0 || (tmp_response[1] != STATUS_OK && tmp_response[1] != REPORT_IDENTIFY)) {
		TS_LOG_ERR(
					"in set up reflash 1\n");
		return -EINVAL;
	}

	retval = secure_memcpy((unsigned char *)&tcm_hcd->id_info,
			sizeof(tcm_hcd->id_info),
			&tmp_response[4],
			sizeof(tmp_response),
			MIN(sizeof(tcm_hcd->id_info), sizeof(tmp_response)));
	if (retval < 0) {
	TS_LOG_ERR(
				"Failed to copy identification info\n");
		return -EINVAL;
	}

	tcm_hcd->packrat_number = le4_to_uint(tcm_hcd->id_info.build_id);

	max_write_size = le2_to_uint(tcm_hcd->id_info.max_write_size);
	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		tcm_hcd->wr_chunk_size = MIN(max_write_size, WR_CHUNK_SIZE_SPI);
	}else{
		tcm_hcd->wr_chunk_size = MIN(max_write_size, WR_CHUNK_SIZE_I2C);
	}
	TS_LOG_ERR("max_write_size:%d\n",  max_write_size);

	if (tcm_hcd->wr_chunk_size == 0)
		tcm_hcd->wr_chunk_size = max_write_size;

	if (tcm_hcd->id_info.mode == MODE_APPLICATION) {
		TS_LOG_ERR(
				"need to enter mode bootloader.....\n");
		retval = reflash_raw_write(tcm_hcd, CMD_RUN_BOOTLOADER_FIRMWARE, NULL, 0);
		if (retval < 0) {
			TS_LOG_ERR(
					"Failed to enter bootloader mode 1\n");
			return retval;
		}
		msleep(50);
		retval = syna_tcm_read(tcm_hcd, tmp_response, sizeof(tmp_response));
		if (retval < 0) {
			TS_LOG_ERR(
						"in set up reflash 2\n");
			return -EINVAL;
		}

	TS_LOG_ERR("response[0] %x, %x, %x, %x, %x, %x, %x\n", tmp_response[0], tmp_response[1], tmp_response[2], tmp_response[3], tmp_response[4], tmp_response[5], tmp_response[6]);
		if (tmp_response[1] == REPORT_IDENTIFY) {
				TS_LOG_ERR(
						"receive identify report\n");
			retval = secure_memcpy((unsigned char *)&tcm_hcd->id_info,
				sizeof(tcm_hcd->id_info),
				&tmp_response[4],
				sizeof(tmp_response),
				MIN(sizeof(tcm_hcd->id_info), sizeof(tmp_response)));
			TS_LOG_ERR("mode :%d\n", tcm_hcd->id_info.mode);
		}
	}

	retval = reflash_raw_write(tcm_hcd,
			CMD_GET_BOOT_INFO,
			NULL,
			0);
	if (retval < 0) {
	TS_LOG_ERR(
				"Failed to write command %s\n",
				STR(CMD_GET_BOOT_INFO));
		return -EINVAL;
	}
	msleep(10);
	retval = syna_tcm_read(tcm_hcd, tmp_response, sizeof(tmp_response));
	if (retval < 0) {
		TS_LOG_ERR(
					"in set up reflash 3\n");
		return -EINVAL;
	}
	if (tmp_response[1] == STATUS_OK) {

		retval = secure_memcpy((unsigned char *)&tcm_hcd->boot_info,
				sizeof(tcm_hcd->boot_info),
				&tmp_response[4],
				sizeof(tmp_response),
				MIN(sizeof(tcm_hcd->boot_info), sizeof(tmp_response)));
		if (retval < 0) {
		TS_LOG_ERR(
					"Failed to copy boot info\n");
			return -EINVAL;
		}
	} else {
		TS_LOG_ERR( "set up reflash 4\n");
		return -EINVAL;
	}

	temp = tcm_hcd->boot_info.write_block_size_words;
	reflash_hcd->write_block_size = temp * 2;

	temp = le2_to_uint(tcm_hcd->boot_info.erase_page_size_words);
	reflash_hcd->page_size = temp * 2;

	temp = le2_to_uint(tcm_hcd->boot_info.max_write_payload_size);
	reflash_hcd->max_write_payload_size = temp;

	TS_LOG_ERR(
			"Write block size = %d\n",
			reflash_hcd->write_block_size);

	TS_LOG_ERR(
			"Page size = %d\n",
			reflash_hcd->page_size);

	TS_LOG_ERR(
			"Max write payload size = %d\n",
			reflash_hcd->max_write_payload_size);

	if (reflash_hcd->write_block_size > (tcm_hcd->wr_chunk_size - 5)) {
		TS_LOG_ERR(
				"Write block size greater than available chunk space\n");
		return -EINVAL;
	}
	return 0;
}

static int reflash_parse_fw_image(void)
{
	unsigned int idx;
	unsigned int addr;
	unsigned int offset;
	unsigned int length;
	unsigned int checksum;
	unsigned int flash_addr;
	unsigned int magic_value;
	unsigned int num_of_areas;
	struct image_header *header;
	struct image_info *image_info;
	struct area_descriptor *descriptor;
	const unsigned char *image;
	const unsigned char *content;

	image = reflash_hcd->image;
	image_info = &reflash_hcd->image_info;
	header = (struct image_header *)image;

	magic_value = le4_to_uint(header->magic_value);
	if (magic_value != IMAGE_FILE_MAGIC_VALUE) {
		TS_LOG_ERR(
				"Invalid image file magic value\n");
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
				TS_LOG_ERR(
						"Boot config checksum error\n");
				return -EINVAL;
			}
			image_info->boot_config.size = length;
			image_info->boot_config.data = content;
			image_info->boot_config.flash_addr = flash_addr;
			TS_LOG_ERR(
					"Boot config size = %d\n",
					length);
			TS_LOG_ERR(
					"Boot config flash address = 0x%08x\n",
					flash_addr);
		} else if (0 == strncmp((char *)descriptor->id_string,
				APP_CODE_ID,
				strlen(APP_CODE_ID))) {
			if (checksum != (crc32(~0, content, length) ^ ~0)) {
				TS_LOG_ERR(
						"Application firmware checksum error\n");
				return -EINVAL;
			}
			image_info->app_firmware.size = length;
			image_info->app_firmware.data = content;
			image_info->app_firmware.flash_addr = flash_addr;
			TS_LOG_ERR(
					"Application firmware size = %d\n",
					length);
			TS_LOG_ERR(
					"Application firmware flash address = 0x%08x\n",
					flash_addr);
		} else if (0 == strncmp((char *)descriptor->id_string,
				APP_CONFIG_ID,
				strlen(APP_CONFIG_ID))) {
			if (checksum != (crc32(~0, content, length) ^ ~0)) {
				TS_LOG_ERR(
						"Application config checksum error\n");
				return -EINVAL;
			}
			image_info->app_config.size = length;
			image_info->app_config.data = content;
			image_info->app_config.flash_addr = flash_addr;
			TS_LOG_ERR(
					"Application config size = %d\n",
					length);
			TS_LOG_ERR(
					"Application config flash address = 0x%08x\n",
					flash_addr);
		} else if (0 == strncmp((char *)descriptor->id_string,
				DISP_CONFIG_ID,
				strlen(DISP_CONFIG_ID))) {
			if (checksum != (crc32(~0, content, length) ^ ~0)) {
				TS_LOG_ERR(
						"Display config checksum error\n");
				return -EINVAL;
			}
			image_info->disp_config.size = length;
			image_info->disp_config.data = content;
			image_info->disp_config.flash_addr = flash_addr;
			TS_LOG_ERR(
					"Display config size = %d\n",
					length);
			TS_LOG_ERR(
					"Display config flash address = 0x%08x\n",
					flash_addr);
		}
	}

	return 0;
}

static int reflash_get_fw_image(char *fw_name)
{
	int retval;
	struct syna_tcm_hcd *tcm_hcd = reflash_hcd->tcm_hcd;

	if (fw_name != NULL) {
		TS_LOG_INFO("reflash_get_fw_image call %s\n",fw_name);
		if (reflash_hcd->image == NULL) {
			retval = request_firmware(&reflash_hcd->fw_entry, fw_name,
					tcm_hcd->pdev->dev.parent);
			if (retval < 0) {
				TS_LOG_ERR(
						"Failed to request %s\n",
						fw_name);
				return retval;
			}

			TS_LOG_ERR(
					"success to request %s Firmware image size = %d\n", fw_name,
					(unsigned int)reflash_hcd->fw_entry->size);

			reflash_hcd->image = reflash_hcd->fw_entry->data;
			reflash_hcd->image_size = reflash_hcd->fw_entry->size;
		}
	} else {
		TS_LOG_ERR("fw_name is null\n");
		return -EINVAL;;
	}
	retval = reflash_parse_fw_image();
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to parse firmware image\n");
		return retval;
	}

	return 0;
}

static enum update_area reflash_compare_id_info(void)
{
	enum update_area update_area;
	unsigned int idx;
	unsigned int image_fw_id;
	unsigned int device_fw_id;
	unsigned char *image_config_id;
	unsigned char *device_config_id;
	struct app_config_header *header;
	struct syna_tcm_hcd *tcm_hcd = reflash_hcd->tcm_hcd;
	const unsigned char *app_config_data;

	update_area = NONE;

	if (reflash_hcd->image_info.app_config.size < sizeof(*header)) {
		TS_LOG_ERR(
				"Invalid application config in image file\n");
		goto exit;
	}

	app_config_data = reflash_hcd->image_info.app_config.data;
	header = (struct app_config_header *)app_config_data;

	if (reflash_hcd->force_update) {
		update_area = FIRMWARE_CONFIG;
		goto exit;
	}

	if (tcm_hcd->id_info.mode != MODE_APPLICATION) {
		update_area = FIRMWARE_CONFIG;
		goto exit;
	}

	image_fw_id = le4_to_uint(header->build_id);
	device_fw_id = tcm_hcd->packrat_number;

	TS_LOG_ERR("Image firmware ID : new = %d ,old = %d\n", image_fw_id, device_fw_id);
	if (image_fw_id != device_fw_id) {
		TS_LOG_ERR("Image firmware ID is not same\n");
		update_area = FIRMWARE_CONFIG;
		goto exit;
	}

	image_config_id = header->customer_config_id;
	device_config_id = tcm_hcd->app_info.customer_config_id;

	for (idx = 0; idx < 16; idx++) {
		if (image_config_id[idx] > device_config_id[idx]) {
			TS_LOG_ERR(
					"Image config ID newer than device config ID\n");
			update_area = CONFIG_ONLY;
			goto exit;
		} else if (image_config_id[idx] < device_config_id[idx]) {
			TS_LOG_ERR(
					"Image config ID older than device config ID\n");
			update_area = NONE;
			goto exit;
		}
	}

	update_area = NONE;

exit:
	if (update_area == NONE) {
		TS_LOG_ERR(
				"No need to do reflash\n");
	} else {
		TS_LOG_ERR(
				"Updating %s\n",
				update_area == FIRMWARE_CONFIG ?
				"firmware and config" :
				"config only");
	}

	return update_area;
}

static int reflash_check_app_config(void)
{
	unsigned int temp;
	unsigned int image_addr;
	unsigned int image_size;
	unsigned int device_addr;
	unsigned int device_size;
	struct syna_tcm_hcd *tcm_hcd = reflash_hcd->tcm_hcd;

	if (reflash_hcd->image_info.app_config.size == 0) {
		TS_LOG_ERR(
				"No application config in image file\n");
		return -EINVAL;
	}

	image_addr = reflash_hcd->image_info.app_config.flash_addr;
	image_size = reflash_hcd->image_info.app_config.size;

	temp = le2_to_uint(tcm_hcd->app_info.app_config_start_write_block);
	device_addr = temp * reflash_hcd->write_block_size;
	device_size = le2_to_uint(tcm_hcd->app_info.app_config_size);

	if (device_addr == 0 && device_size == 0)
		return 0;

	if (image_addr != device_addr) {
		TS_LOG_ERR(
				"Flash address mismatch\n");
		return -EINVAL;
	}


	return 0;
}

static int reflash_check_disp_config(void)
{
	unsigned int temp;
	unsigned int image_addr;
	unsigned int image_size;
	unsigned int device_addr;
	unsigned int device_size;
	struct syna_tcm_hcd *tcm_hcd = reflash_hcd->tcm_hcd;

	if (reflash_hcd->image_info.disp_config.size == 0) {
		TS_LOG_ERR(
				"No display config in image file\n");
		return -EINVAL;
	}

	image_addr = reflash_hcd->image_info.disp_config.flash_addr;
	image_size = reflash_hcd->image_info.disp_config.size;

	temp = le4_to_uint(tcm_hcd->boot_info.display_config_start_block);
	device_addr = temp * reflash_hcd->write_block_size;

	temp = le2_to_uint(tcm_hcd->boot_info.display_config_length_blocks);
	device_size = temp * reflash_hcd->write_block_size;

	if (image_addr != device_addr) {
		TS_LOG_ERR(
				"Flash address mismatch\n");
		return -EINVAL;
	}

	if (image_size != device_size) {
		TS_LOG_ERR(
				"Config size mismatch\n");
		return -EINVAL;
	}

	return 0;
}

static int reflash_check_app_firmware(void)
{
	//struct syna_tcm_hcd *tcm_hcd = reflash_hcd->tcm_hcd;

	if (reflash_hcd->image_info.app_firmware.size == 0) {
		TS_LOG_ERR(
				"No application firmware in image file\n");
		return -EINVAL;
	}

	return 0;
}

static int reflash_write_flash(unsigned int address, const unsigned char *data,
		unsigned int datalen)
{
	int retval;
	unsigned int offset;
	unsigned int w_length;
	unsigned int xfer_length;
	unsigned int remaining_length;
	unsigned int flash_address;
	unsigned int block_address;
	struct syna_tcm_hcd *tcm_hcd = reflash_hcd->tcm_hcd;
	unsigned char response[10] = {0};

	w_length = tcm_hcd->wr_chunk_size - 5;

	w_length = w_length - (w_length % reflash_hcd->write_block_size);

	w_length = MIN(w_length, reflash_hcd->max_write_payload_size);

	offset = 0;

	remaining_length = datalen;

	LOCK_BUFFER(reflash_hcd->out);
	LOCK_BUFFER(reflash_hcd->resp);

	while (remaining_length) {
		if (remaining_length > w_length)
			xfer_length = w_length;
		else
			xfer_length = remaining_length;

		retval = syna_tcm_alloc_mem(tcm_hcd,
				&reflash_hcd->out,
				xfer_length + 2);
		if (retval < 0) {
			TS_LOG_ERR(
					"Failed to allocate memory for reflash_hcd->out.buf\n");
			UNLOCK_BUFFER(reflash_hcd->resp);
			UNLOCK_BUFFER(reflash_hcd->out);
			return retval;
		}
		TS_LOG_DEBUG("xfer_length:%d, remaining_length:%d, w_length:%d, wr_chunk_size:%d\n", xfer_length, remaining_length, w_length, tcm_hcd->wr_chunk_size);
		flash_address = address + offset;
		block_address = flash_address / reflash_hcd->write_block_size;
		reflash_hcd->out.buf[0] = (unsigned char)block_address;
		reflash_hcd->out.buf[1] = (unsigned char)(block_address >> 8);

		retval = secure_memcpy(&reflash_hcd->out.buf[2],
				reflash_hcd->out.buf_size - 2,
				&data[offset],
				datalen - offset,
				xfer_length);
		if (retval < 0) {
			TS_LOG_ERR(
					"Failed to copy write data\n");
			UNLOCK_BUFFER(reflash_hcd->resp);
			UNLOCK_BUFFER(reflash_hcd->out);
			return retval;
		}

		retval = reflash_raw_write(tcm_hcd,
				CMD_WRITE_FLASH,
				reflash_hcd->out.buf,
				xfer_length + 2);

		if (retval < 0) {
			TS_LOG_ERR(
					"Failed to write command %s\n",
					STR(CMD_WRITE_FLASH));
			TS_LOG_ERR(
					"Flash address = 0x%08x\n",
					flash_address);
			TS_LOG_ERR(
					"Data length = %d\n",
					xfer_length);
			UNLOCK_BUFFER(reflash_hcd->resp);
			UNLOCK_BUFFER(reflash_hcd->out);
			return retval;
		}
		msleep(30);
		retval = syna_tcm_read(tcm_hcd,
				response,
				sizeof(response));
		if (retval < 0 || (response[1] != STATUS_OK && response[1] != STATUS_IDLE)) {
			TS_LOG_ERR(
					"Failed to get write response\n");

			TS_LOG_ERR("response[0] %x, %x, %x, %x, %x\n", response[0], response[1], response[2], response[3], response[4]);
			UNLOCK_BUFFER(reflash_hcd->resp);
			UNLOCK_BUFFER(reflash_hcd->out);
			return -EINVAL;
		}

		offset += xfer_length;
		remaining_length -= xfer_length;
	}

	UNLOCK_BUFFER(reflash_hcd->resp);
	UNLOCK_BUFFER(reflash_hcd->out);

	return 0;
}

reflash_write(app_config)

reflash_write(disp_config)

reflash_write(app_firmware)

static int reflash_raw_write(struct syna_tcm_hcd *tcm_hcd,
		unsigned char command, unsigned char *payload, unsigned int length)
{
	int retval = NO_ERR;
	unsigned int idx = 0;
	unsigned int chunks = 0;
	unsigned int chunk_space = 0;
	unsigned int xfer_length = 0;
	unsigned int remaining_length = 0;

	atomic_set(&tcm_hcd->command_status, CMD_BUSY);
	tcm_hcd->command = command;

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

	TS_LOG_DEBUG("%s: Command = 0x%02x\n", __func__, command);

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
				goto exit;
			}
		}

		retval = syna_tcm_write(tcm_hcd,
				tcm_hcd->out.buf,
				xfer_length + 1);
		if (retval < 0) {
			TS_LOG_ERR("Failed to write to device\n");
			UNLOCK_BUFFER(tcm_hcd->out);
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

static int reflash_erase_flash(unsigned int page_start, unsigned int page_count)
{
	int retval;
	unsigned char out_buf[2];
	struct syna_tcm_hcd *tcm_hcd = reflash_hcd->tcm_hcd;

	unsigned char response[10] = {0};
	out_buf[0] = (unsigned char)page_start;
	out_buf[1] = (unsigned char)page_count;

	LOCK_BUFFER(reflash_hcd->resp);

	TS_LOG_ERR("start to write erase command\n");
	retval = reflash_raw_write(tcm_hcd,
			CMD_ERASE_FLASH,
			out_buf,
			sizeof(out_buf));

	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to write command %s\n",
				STR(CMD_ERASE_FLASH));
		UNLOCK_BUFFER(reflash_hcd->resp);
		return retval;
	}
	if (page_start == 2) {
		msleep(4000);
	} else {
		msleep(200);
	}

	retval = syna_tcm_read(tcm_hcd,
			response,
			sizeof(response));
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to read erase response\n");
		UNLOCK_BUFFER(reflash_hcd->resp);
		return retval;
	}
	UNLOCK_BUFFER(reflash_hcd->resp);
	if (response[1] != STATUS_OK && response[1] != STATUS_IDLE) {
		TS_LOG_ERR("response[0] %x, %x, %x, %x, %x\n", response[0], response[1], response[2], response[3], response[4]);
		return -EINVAL;
	}
	return 0;
}

reflash_erase(app_config)

reflash_erase(disp_config)

reflash_erase(app_firmware)

reflash_update(app_config)

reflash_update(disp_config)

reflash_update(app_firmware)

int reflash_do_reflash(char *fw_name)
{
	int retval;
	enum update_area update_area;
	struct syna_tcm_hcd *tcm_hcd = reflash_hcd->tcm_hcd;

	retval = reflash_get_fw_image(fw_name);
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to get firmware image\n");
		goto exit;
	}

	TS_LOG_ERR(
			"Start of reflash\n");

	update_area = reflash_compare_id_info();

	switch (update_area) {
	case FIRMWARE_CONFIG:
		retval = reflash_update_app_firmware(false);
		if (retval < 0) {
			TS_LOG_ERR(
					"Failed to reflash application firmware\n");
			goto exit;
		}
		//memset(&tcm_hcd->app_info, 0x00, sizeof(tcm_hcd->app_info));
	case CONFIG_ONLY:
		if (reflash_hcd->disp_cfg_update) {
			retval = reflash_update_disp_config(false);
			if (retval < 0) {
				TS_LOG_ERR(
						"Failed to reflash display config\n");
				goto exit;
			}
		}
		retval = reflash_update_app_config(false);
		if (retval < 0) {
			TS_LOG_ERR(
					"Failed to reflash application config\n");
			goto exit;
		}
		break;
	case NONE:
	default:
		break;
	}

	TS_LOG_ERR(
			"End of reflash\n");

	retval = 0;

exit:
	if (reflash_hcd->fw_entry) {
		release_firmware(reflash_hcd->fw_entry);
		reflash_hcd->fw_entry = NULL;
		reflash_hcd->image = NULL;
		reflash_hcd->image_size = 0;
	}
	reflash_after_fw_update_do_reset();
	return retval;
}

/*
static int reflash_do_firmware_udpate(struct syna_tcm_hcd *tcm_hcd, unsigned char *fw_name)
{
	int retval = 0;

	pm_stay_awake(&tcm_hcd->pdev->dev);

	mutex_lock(&reflash_hcd->reflash_mutex);

	TS_LOG_ERR("start to do reflash\n");
	retval = reflash_do_reflash();
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to do reflash\n");
	} else {
		TS_LOG_ERR("do reflash success\n");
	}

	mutex_unlock(&reflash_hcd->reflash_mutex);

	pm_relax(&tcm_hcd->pdev->dev);
	return retval;
}
*/

int reflash_after_fw_update_do_reset(void)
{
	int retval = 0;
	struct syna_tcm_hcd *tcm_hcd = reflash_hcd->tcm_hcd;

	retval = reflash_raw_write(tcm_hcd,	CMD_RESET,NULL, 0);
	if (retval < 0) {
		TS_LOG_ERR("in set up reflash 1\n");
		return -EINVAL;
	}
	msleep(60);
	retval = syna_tcm_read(tcm_hcd, tmp_response, sizeof(tmp_response));

	if (tmp_response[1] == REPORT_IDENTIFY) {
		TS_LOG_ERR("receive identify report\n");
		retval = secure_memcpy((unsigned char *)&tcm_hcd->id_info,
				sizeof(tcm_hcd->id_info),
				&tmp_response[4],
				sizeof(tmp_response),
				MIN(sizeof(tcm_hcd->id_info), sizeof(tmp_response)));
		TS_LOG_ERR("mode :%d\n", tcm_hcd->id_info.mode);
	}
	if (tcm_hcd->id_info.mode != MODE_APPLICATION) {
		TS_LOG_ERR("need to enter mode ap.....\n");
		retval = reflash_raw_write(tcm_hcd, CMD_RUN_APPLICATION_FIRMWARE, NULL, 0);
		if (retval < 0) {
			TS_LOG_ERR("Failed to enter bootloader mode 1\n");
			return retval;
		}
		msleep(120);
		retval = syna_tcm_read(tcm_hcd, tmp_response, sizeof(tmp_response));
		if (retval < 0) {
			TS_LOG_ERR("in set up reflash 2\n");
			return -EINVAL;
		}

		TS_LOG_ERR("response[0] %x, %x, %x, %x, %x, %x, %x\n",
				tmp_response[0], tmp_response[1],
				tmp_response[2], tmp_response[3],
				tmp_response[4], tmp_response[5], tmp_response[6]);
		if (tmp_response[1] == REPORT_IDENTIFY) {
			TS_LOG_ERR("receive identify report\n");
			retval = secure_memcpy((unsigned char *)&tcm_hcd->id_info,
					sizeof(tcm_hcd->id_info),
					&tmp_response[4],
					sizeof(tmp_response),
					MIN(sizeof(tcm_hcd->id_info), sizeof(tmp_response)));
			TS_LOG_ERR("mode :%d\n", tcm_hcd->id_info.mode);
		}
	}

	if (TS_BUS_I2C == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		retval = touch_init(tcm_hcd);
		if (retval < 0) {
			TS_LOG_ERR("failed to do touch init after reflash.\n");
		}
	}

	return retval;
}

static int reflash_read_flash(unsigned int address, unsigned char *data,
		unsigned int datalen)
{
	int retval = NO_ERR;
	unsigned int length_words = 0;
	unsigned int flash_addr_words = 0;
	struct syna_tcm_hcd *tcm_hcd = reflash_hcd->tcm_hcd;

	LOCK_BUFFER(reflash_hcd->out);

	retval = syna_tcm_alloc_mem(tcm_hcd,
			&reflash_hcd->out,
			6);
	if (retval < 0) {
		TS_LOG_ERR("Failed to allocate memory for reflash_hcd->out.buf\n");
		UNLOCK_BUFFER(reflash_hcd->out);
		return retval;
	}

	length_words = datalen / 2;
	flash_addr_words = address / 2;

	reflash_hcd->out.buf[0] = (unsigned char)flash_addr_words;
	reflash_hcd->out.buf[1] = (unsigned char)(flash_addr_words >> 8);
	reflash_hcd->out.buf[2] = (unsigned char)(flash_addr_words >> 16);
	reflash_hcd->out.buf[3] = (unsigned char)(flash_addr_words >> 24);
	reflash_hcd->out.buf[4] = (unsigned char)length_words;
	reflash_hcd->out.buf[5] = (unsigned char)(length_words >> 8);

	retval = reflash_raw_write(tcm_hcd,
			CMD_READ_FLASH,
			reflash_hcd->out.buf,
			6);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n", STR(CMD_READ_FLASH));

		UNLOCK_BUFFER(reflash_hcd->out);
		return retval;
	}

	UNLOCK_BUFFER(reflash_hcd->out);

	msleep(50);
	retval = syna_tcm_read(tcm_hcd, flash_read_data, sizeof(flash_read_data));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read flash_read_data\n");

		return -EIO;
	}

	retval = secure_memcpy(data,
			datalen,
			&flash_read_data[4],
			sizeof(flash_read_data) - 4,
			datalen);
	if (retval < 0) {
		TS_LOG_ERR("Failed to copy read data\n");

		return retval;
	}

	return 0;
}

int reflash_read_boot_config(unsigned char *data, int data_length)
{
	int retval = 0;
	unsigned int temp;
	unsigned int addr;
	unsigned int length;

	struct syna_tcm_app_info *app_info;
	struct syna_tcm_boot_info *boot_info;
	struct syna_tcm_hcd *tcm_hcd = reflash_hcd->tcm_hcd;

	retval = reflash_set_up_flash_access();
	if (retval < 0) {
		TS_LOG_ERR("Failed to set up flash access\n");
		goto exit;
	}

	app_info = &tcm_hcd->app_info;
	boot_info = &tcm_hcd->boot_info;

	temp = le2_to_uint(boot_info->boot_config_start_block);
	addr = temp * reflash_hcd->write_block_size;
	length = BOOT_CONFIG_SIZE * BOOT_CONFIG_SLOTS;

	retval = reflash_read_flash(addr, data, length);
	if (retval < 0) {
		TS_LOG_ERR("Failed to read_flash\n");
		goto exit;
	}

exit:
	reflash_after_fw_update_do_reset();
	return retval;
}

int reflash_parse_boot_config(unsigned char *boot_config_data, int data_length, unsigned char *out_info, int length)
{
	int i = 0;
	int retval = NO_ERR;
	retval = secure_memcpy(out_info, length, &boot_config_data[8], length, length);
	if(retval < 0) {
		TS_LOG_ERR("%s: Failed to copy force parameter data\n",__func__);
		return retval;
	}
	for (i = 0; i < strlen(out_info) && i < length; i++) {
		out_info[i] = tolower(out_info[i]);
	}

	return 0;
}

int reflash_init(struct syna_tcm_hcd *tcm_hcd)
{
	int retval = 0;
	//int idx;

	reflash_hcd = kzalloc(sizeof(*reflash_hcd), GFP_KERNEL);
	if (!reflash_hcd) {
		TS_LOG_ERR(
				"Failed to allocate memory for reflash_hcd\n");
		return -ENOMEM;
	}

	reflash_hcd->image_buf = kzalloc(IMAGE_BUF_SIZE, GFP_KERNEL);
	if (!reflash_hcd->image_buf) {
		TS_LOG_ERR(
				"Failed to allocate memory for reflash_hcd->image_buf\n");
		//goto err_allocate_memory;
	}

	reflash_hcd->tcm_hcd = tcm_hcd;
	reflash_hcd->force_update = FORCE_REFLASH;
	reflash_hcd->disp_cfg_update = DISPLAY_REFLASH;
//	reflash_hcd->tcm_hcd->update_firmware = reflash_do_firmware_udpate;

	INIT_BUFFER(reflash_hcd->out, false);
	INIT_BUFFER(reflash_hcd->resp, false);
	INIT_BUFFER(reflash_hcd->read, false);

//	tcm_hcd->read_flash_data = reflash_read_data;

	if (TS_BUS_I2C == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		memset(tcm_hcd->tcm_mod_info.project_id_string, 0,
				sizeof(tcm_hcd->tcm_mod_info.project_id_string));
		reflash_read_boot_config(boot_config_data, sizeof(boot_config_data));
		reflash_parse_boot_config(boot_config_data,
				sizeof(boot_config_data),
				tcm_hcd->tcm_mod_info.project_id_string,
				sizeof(tcm_hcd->tcm_mod_info.project_id_string) - 1);
	}

	return retval;

}
