/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/firmware.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include "synaptics.h"
#include "../../huawei_ts_kit.h"
#define SYNAPTICS_FW_S3718_MANUAL_UPDATE_FILE_NAME	"ts/touch_screen_firmware.img"
#define FORCE_UPDATE false
#define DO_LOCKDOWN false

#define FLASH_STATUS_00    0x00
#define FLASH_STATUS_08    0x08

#define MAX_IMAGE_NAME_LEN 256
#define MAX_FIRMWARE_ID_LEN 10
#define RMI_PRODUCT_ID_LENGTH    10

#define IMAGE_HEADER_VERSION_05 0x05
#define IMAGE_HEADER_VERSION_06 0x06
#define IMAGE_HEADER_VERSION_10 0x10
#define PRODUCT_INFO_SIZE 2
#define PRODUCT_ID_SIZE 10
#define BUILD_ID_SIZE 3

#define IMAGE_AREA_OFFSET 0x100
#define LOCKDOWN_SIZE 0x50

#define V5V6_BOOTLOADER_ID_OFFSET 0

#define V5_PROPERTIES_OFFSET 2
#define V5_BLOCK_SIZE_OFFSET 3
#define V5_BLOCK_COUNT_OFFSET 5
#define V5_BLOCK_NUMBER_OFFSET 0
#define V5_BLOCK_DATA_OFFSET 2

#define V6_PROPERTIES_OFFSET 1
#define V6_BLOCK_SIZE_OFFSET 2
#define V6_BLOCK_COUNT_OFFSET 3
#define V6_PROPERTIES_2_OFFSET 4
#define V6_GUEST_CODE_BLOCK_COUNT_OFFSET 5
#define V6_BLOCK_NUMBER_OFFSET 0
#define V6_BLOCK_DATA_OFFSET 1
#define V6_FLASH_COMMAND_OFFSET 2
#define V6_FLASH_STATUS_OFFSET 3
#define V7_FLASH_STATUS_OFFSET 0
#define V7_PARTITION_ID_OFFSET 1
#define V7_BLOCK_NUMBER_OFFSET 2
#define V7_TRANSFER_LENGTH_OFFSET 3
#define V7_COMMAND_OFFSET 4
#define V7_PAYLOAD_OFFSET 5

#define V7_PARTITION_SUPPORT_BYTES 4
#define FIRMWARE_LEN (300*1024)

#define F35_ERROR_CODE_OFFSET 0
#define F35_CHUNK_NUM_LSB_OFFSET 0
#define F35_CHUNK_NUM_MSB_OFFSET 1
#define F35_CHUNK_DATA_OFFSET 2
#define F35_CHUNK_COMMAND_OFFSET 18

#define F35_CHUNK_SIZE 16
#define F35_ERASE_ALL_WAIT_MS 3000
#define F35_RESET_WAIT_MS 250

#define SLEEP_MODE_NORMAL (0x00)
#define SLEEP_MODE_SENSOR_SLEEP (0x01)
#define SLEEP_MODE_RESERVED0 (0x02)
#define SLEEP_MODE_RESERVED1 (0x03)

#define ENABLE_WAIT_MS (1 * 1000)
#define WRITE_WAIT_MS (3 * 1000)
#define ERASE_WAIT_MS (5 * 1000)
#define WRITE_BL_WAIT_MS 150

#define MIN_SLEEP_TIME_US 50
#define MAX_SLEEP_TIME_US 100

#define INT_DISABLE_WAIT_MS 20
#define ENTER_FLASH_PROG_WAIT_MS 20
#define WAIT_FW_REFLASH_MS 25

#define PAYLOAD_LENGTH_MAX 256

#define SYNA_UPP

static int fwu_do_reflash(void);

static int fwu_recovery_check_status(void);
#ifdef SYNA_UPP
static int fwu_read_f34_queries(void);
short synap_get_oem_data_info( void );
int synap_get_oem_data(unsigned char *oem_data, unsigned short leng);
int synap_set_oem_data(unsigned char *oem_data, unsigned short leng);
#endif
static int set_lockdown_data(unsigned char *lockdown_data, unsigned short leng);
extern struct synaptics_rmi4_data *rmi4_data;
extern void synap_parse_chip_specific_dts(struct ts_kit_device_data *chip_data);

enum f34_version {
	F34_V0 = 0,
	F34_V1,
	F34_V2,
};

enum bl_version {
	BL_V5 = 5,
	BL_V6 = 6,
	BL_V7 = 7,
	BL_V8 = 8,
};

enum data_offset_bit {
	DATA_OFFET_4BIT = 4,
	DATA_OFFET_8BIT = 8,
};

enum flash_area {
	NONE = 0,
	UI_FIRMWARE,
	UI_CONFIG,
};

enum update_mode {
	NORMAL = 1,
	FORCE = 2,
	LOCKDOWN = 8,
};

enum config_area {
	UI_CONFIG_AREA = 0,
	PM_CONFIG_AREA,
	BL_CONFIG_AREA,
	DP_CONFIG_AREA,
	FLASH_CONFIG_AREA,
	FORCE_CONFIG_AREA,
	LCM_DATA_AREA,
	OEM_DATA_AREA,
	UPP_AREA,
};

enum v7_status {
	SUCCESS = 0x00,
	DEVICE_NOT_IN_BOOTLOADER_MODE,
	INVALID_PARTITION,
	INVALID_COMMAND,
	INVALID_BLOCK_OFFSET,
	INVALID_TRANSFER,
	NOT_ERASED,
	FLASH_PROGRAMMING_KEY_INCORRECT,
	BAD_PARTITION_TABLE,
	CHECKSUM_FAILED,
	FLASH_HARDWARE_FAILURE = 0x1f,
};

enum v7_partition_id {
	BOOTLOADER_PARTITION = 0x01,
	DEVICE_CONFIG_PARTITION,
	FLASH_CONFIG_PARTITION,
	MANUFACTURING_BLOCK_PARTITION,
	GUEST_SERIALIZATION_PARTITION,
	GLOBAL_PARAMETERS_PARTITION,
	CORE_CODE_PARTITION,
	CORE_CONFIG_PARTITION,
	GUEST_CODE_PARTITION,
	DISPLAY_CONFIG_PARTITION,
	EXTERNAL_TOUCH_AFE_CONFIG_PARTITION,
	UTILITY_PARAMETER_PARTITION,
};

enum v7_flash_command {
	CMD_V7_IDLE = 0x00,
	CMD_V7_ENTER_BL,
	CMD_V7_READ,
	CMD_V7_WRITE,
	CMD_V7_ERASE,
	CMD_V7_ERASE_AP,
	CMD_V7_SENSOR_ID,
};

enum v5v6_flash_command {
	CMD_V5V6_IDLE = 0x0,
	CMD_V5V6_WRITE_FW = 0x2,
	CMD_V5V6_ERASE_ALL = 0x3,
	CMD_V5V6_WRITE_LOCKDOWN = 0x4,
	CMD_V5V6_READ_CONFIG = 0x5,
	CMD_V5V6_WRITE_CONFIG = 0x6,
	CMD_V5V6_ERASE_UI_CONFIG = 0x7,
	CMD_V5V6_ERASE_BL_CONFIG = 0x9,
	CMD_V5V6_ERASE_DISP_CONFIG = 0xa,
	CMD_V5V6_ERASE_GUEST_CODE = 0xb,
	CMD_V5V6_WRITE_GUEST_CODE = 0xc,
	CMD_V5V6_ENABLE_FLASH_PROG = 0xf,
	CMD_V5V6_ERASE_FORCE_CONFIG = 0x11,
	CMD_V5V6_READ_FORCE_CONFIG = 0x12,
	CMD_V5V6_WRITE_FORCE_CONFIG = 0x13,
	CMD_V5V6_ERASE_LOCKDOWN_DATA = 0x1A,
	CMD_V5V6_READ_LOCKDOWN_DATA = 0x1B,
	CMD_V5V6_WRITE_LOCKDOWN_DATA = 0x1C,
	CMD_V5V6_ERASE_LCM_DATA = 0x1D,
	CMD_V5V6_ERASE_OEM_DATA = 0x1E,
};

enum flash_command {
	CMD_IDLE = 0,
	CMD_WRITE_FW,
	CMD_WRITE_CONFIG,
	CMD_WRITE_LOCKDOWN,
	CMD_WRITE_GUEST_CODE,
	CMD_WRITE_BOOTLOADER,
	CMD_WRITE_UTILITY_PARAM,
#ifdef SYNA_UPP
	CMD_READ_UTILITY_PARAM,
#endif
	CMD_READ_CONFIG,
	CMD_ERASE_ALL,
	CMD_ERASE_UI_FIRMWARE,
	CMD_ERASE_UI_CONFIG,
	CMD_ERASE_BL_CONFIG,
	CMD_ERASE_DISP_CONFIG,
	CMD_ERASE_FLASH_CONFIG,
	CMD_ERASE_GUEST_CODE,
	CMD_ERASE_BOOTLOADER,
	CMD_ERASE_UTILITY_PARAMETER,
	CMD_ENABLE_FLASH_PROG,
	CMD_ERASE_FORCE_CONFIG,
	CMD_READ_FORCE_CONFIG,
	CMD_WRITE_FORCE_CONFIG,
	CMD_ERASE_LOCKDOWN_DATA,
	CMD_READ_LOCKDOWN_DATA,
	CMD_WRITE_LOCKDOWN_DATA,
	CMD_ERASE_LCM_DATA,
	CMD_READ_LCM_DATA,
	CMD_WRITE_LCM_DATA,
	CMD_ERASE_OEM_DATA,
	CMD_READ_OEM_DATA,
	CMD_WRITE_OEM_DATA,
};

enum f35_flash_command {
	CMD_F35_IDLE = 0x0,
	CMD_F35_RESERVED = 0x1,
	CMD_F35_WRITE_CHUNK = 0x2,
	CMD_F35_ERASE_ALL = 0x3,
	CMD_F35_RESET = 0x10,
};

enum container_id {
	TOP_LEVEL_CONTAINER = 0,
	UI_CONTAINER,
	UI_CONFIG_CONTAINER,
	BL_CONTAINER,
	BL_IMAGE_CONTAINER,
	BL_CONFIG_CONTAINER,
	BL_LOCKDOWN_INFO_CONTAINER,
	PERMANENT_CONFIG_CONTAINER,
	GUEST_CODE_CONTAINER,
	BL_PROTOCOL_DESCRIPTOR_CONTAINER,
	UI_PROTOCOL_DESCRIPTOR_CONTAINER,
	RMI_SELF_DISCOVERY_CONTAINER,
	RMI_PAGE_CONTENT_CONTAINER,
	GENERAL_INFORMATION_CONTAINER,
	DEVICE_CONFIG_CONTAINER,
	FLASH_CONFIG_CONTAINER,
	GUEST_SERIALIZATION_CONTAINER,
	GLOBAL_PARAMETERS_CONTAINER,
	CORE_CODE_CONTAINER,
	CORE_CONFIG_CONTAINER,
	DISPLAY_CONFIG_CONTAINER,
	EXTERNAL_TOUCH_AFE_CONFIG_CONTAINER,
	UTILITY_CONTAINER,
	UTILITY_PARAMETER_CONTAINER,
};

enum utility_parameter_id {
	UNUSED = 0,
	FORCE_PARAMETER,
	ANTI_BENDING_PARAMETER,
};

struct pdt_properties {
	union {
		struct {
			unsigned char reserved_1:6;
			unsigned char has_bsr:1;
			unsigned char reserved_2:1;
		} __packed;
		unsigned char data[1];
	};
};

struct partition_table {
	unsigned char partition_id:5;
	unsigned char byte_0_reserved:3;
	unsigned char byte_1_reserved;
	unsigned char partition_length_7_0;
	unsigned char partition_length_15_8;
	unsigned char start_physical_address_7_0;
	unsigned char start_physical_address_15_8;
	unsigned char partition_properties_7_0;
	unsigned char partition_properties_15_8;
} __packed;

struct f01_device_control {
	union {
		struct {
			unsigned char sleep_mode:2;
			unsigned char nosleep:1;
			unsigned char reserved:2;
			unsigned char charger_connected:1;
			unsigned char report_rate:1;
			unsigned char configured:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f34_v7_query_0 {
	union {
		struct {
			unsigned char subpacket_1_size:3;
			unsigned char has_config_id:1;
			unsigned char f34_query0_b4:1;
			unsigned char has_thqa:1;
			unsigned char f34_query0_b6__7:2;
		} __packed;
		unsigned char data[1];
	};
};

struct f34_v7_query_1_7 {
	union {
		struct {
			/* query 1 */
			unsigned char bl_minor_revision;
			unsigned char bl_major_revision;

			/* query 2 */
			unsigned char bl_fw_id_7_0;
			unsigned char bl_fw_id_15_8;
			unsigned char bl_fw_id_23_16;
			unsigned char bl_fw_id_31_24;

			/* query 3 */
			unsigned char minimum_write_size;
			unsigned char block_size_7_0;
			unsigned char block_size_15_8;
			unsigned char flash_page_size_7_0;
			unsigned char flash_page_size_15_8;

			/* query 4 */
			unsigned char adjustable_partition_area_size_7_0;
			unsigned char adjustable_partition_area_size_15_8;

			/* query 5 */
			unsigned char flash_config_length_7_0;
			unsigned char flash_config_length_15_8;

			/* query 6 */
			unsigned char payload_length_7_0;
			unsigned char payload_length_15_8;

			/* query 7 */
			unsigned char f34_query7_b0:1;
			unsigned char has_bootloader:1;
			unsigned char has_device_config:1;
			unsigned char has_flash_config:1;
			unsigned char has_manufacturing_block:1;
			unsigned char has_guest_serialization:1;
			unsigned char has_global_parameters:1;
			unsigned char has_core_code:1;
			unsigned char has_core_config:1;
			unsigned char has_guest_code:1;
			unsigned char has_display_config:1;
			unsigned char f34_query7_b11__15:5;
			unsigned char f34_query7_b16__23;
			unsigned char f34_query7_b24__31;
		} __packed;
		unsigned char data[21];
	};
};

struct f34_v7_data0 {
	union {
		struct {
			unsigned char operation_status:5;
			unsigned char device_cfg_status:2;
			unsigned char bl_mode:1;
		} __packed;
		unsigned char data[1];
	};
};
struct f34_v7_data_1_5 {
	union {
		struct {
			unsigned char partition_id:5;
			unsigned char f34_data1_b5__7:3;
			unsigned char block_offset_7_0;
			unsigned char block_offset_15_8;
			unsigned char transfer_length_7_0;
			unsigned char transfer_length_15_8;
			unsigned char command;
			unsigned char payload_0;
			unsigned char payload_1;
		} __packed;
		unsigned char data[8];
	};
};

struct f34_v5v6_flash_properties {
	union {
		struct {
			unsigned char reg_map:1;
			unsigned char unlocked:1;
			unsigned char has_config_id:1;
			unsigned char has_pm_config:1;
			unsigned char has_bl_config:1;
			unsigned char has_disp_config:1;
			unsigned char has_ctrl1:1;
			unsigned char has_query4:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f34_v5v6_flash_properties_2 {
	union {
		struct {
			unsigned char has_guest_code:1;
			unsigned char reserved_1:1;
			unsigned char has_gesture_config:1;
			unsigned char has_force_config:1;
			unsigned char has_lockdown_data:1;
			unsigned char has_lcm_data:1;
			unsigned char has_oem_data:1;
			unsigned char reserved_2:1;
		} __packed;
		unsigned char data[1];
	};
};

struct register_offset {
	unsigned char properties;
	unsigned char properties_2;
	unsigned char block_size;
	unsigned char block_count;
	unsigned char gc_block_count;
	unsigned char flash_status;
	unsigned char partition_id;
	unsigned char block_number;
	unsigned char transfer_length;
	unsigned char flash_cmd;
	unsigned char payload;
};

struct block_count {
	unsigned short ui_firmware;
	unsigned short ui_config;
	unsigned short dp_config;
	unsigned short pm_config;
	unsigned short fl_config;
	unsigned short bl_image;
	unsigned short bl_config;
	unsigned short utility_param;
	unsigned short lockdown;
	unsigned short guest_code;
	unsigned short force_config;
	unsigned short lockdown_data;
	unsigned short lcm_data;
	unsigned short oem_data;
};

struct physical_address {
	unsigned short ui_firmware;
	unsigned short ui_config;
	unsigned short dp_config;
	unsigned short guest_code;
};

struct container_descriptor {
	unsigned char content_checksum[4];
	unsigned char container_id[2];
	unsigned char minor_version;
	unsigned char major_version;
	unsigned char reserved_08;
	unsigned char reserved_09;
	unsigned char reserved_0a;
	unsigned char reserved_0b;
	unsigned char container_option_flags[4];
	unsigned char content_options_length[4];
	unsigned char content_options_address[4];
	unsigned char content_length[4];
	unsigned char content_address[4];
};

struct image_header_10 {
	unsigned char checksum[4];
	unsigned char reserved_04;
	unsigned char reserved_05;
	unsigned char minor_header_version;
	unsigned char major_header_version;
	unsigned char reserved_08;
	unsigned char reserved_09;
	unsigned char reserved_0a;
	unsigned char reserved_0b;
	unsigned char top_level_container_start_addr[4];
};

struct image_header_05_06 {
	/* 0x00 - 0x0f */
	unsigned char checksum[4];
	unsigned char reserved_04;
	unsigned char reserved_05;
	unsigned char options_firmware_id:1;
	unsigned char options_bootloader:1;
	unsigned char options_guest_code:1;
	unsigned char options_tddi:1;
	unsigned char options_reserved:4;
	unsigned char header_version;
	unsigned char firmware_size[4];
	unsigned char config_size[4];
	/* 0x10 - 0x1f */
	unsigned char product_id[PRODUCT_ID_SIZE];
	unsigned char package_id[2];
	unsigned char package_id_revision[2];
	unsigned char product_info[PRODUCT_INFO_SIZE];
	/* 0x20 - 0x2f */
	unsigned char bootloader_addr[4];
	unsigned char bootloader_size[4];
	unsigned char ui_addr[4];
	unsigned char ui_size[4];
	/* 0x30 - 0x3f */
	unsigned char ds_id[16];
	/* 0x40 - 0x4f */
	union {
		struct {
			unsigned char cstmr_product_id[PRODUCT_ID_SIZE];
			unsigned char reserved_4a_4f[6];
		};
		struct {
			unsigned char dsp_cfg_addr[4];
			unsigned char dsp_cfg_size[4];
			unsigned char reserved_48_4f[8];
		};
	};
	/* 0x50 - 0x53 */
	unsigned char firmware_id[4];
};

struct block_data {
	unsigned int size;
	const unsigned char *data;
};

struct image_metadata {
	bool contains_firmware_id;
	bool contains_bootloader;
	bool contains_disp_config;
	bool contains_guest_code;
	bool contains_flash_config;
	bool contains_utility_param;
	unsigned int firmware_id;
	unsigned int checksum;
	unsigned int bootloader_size;
	unsigned int disp_config_offset;
	unsigned char bl_version;
	unsigned char product_id[PRODUCT_ID_SIZE + 1];
	unsigned char cstmr_product_id[PRODUCT_ID_SIZE + 1];
	struct block_data bootloader;
	struct block_data utility;
	struct block_data ui_firmware;
	struct block_data ui_config;
	struct block_data dp_config;
	struct block_data pm_config;
	struct block_data fl_config;
	struct block_data bl_image;
	struct block_data bl_config;
	struct block_data force_param;
	struct block_data anti_bending_param;
	struct block_data guest_code;
	struct block_data lockdown;
	struct block_count blkcount;
	struct physical_address phyaddr;
};

struct synaptics_rmi4_fwu_handle {
	enum bl_version bl_version;
	bool initialized;
	bool in_bl_mode;
	bool in_ub_mode;
	bool force_update;
	bool do_lockdown;
	bool has_guest_code;
	bool has_force_config;
	bool has_lockdown_data;
	bool has_lcm_data;
	bool has_oem_data;
	bool has_utility_param;
	bool new_partition_table;
	bool incompatible_partition_tables;
	bool write_bootloader;

	bool write_project_id;

	unsigned int data_pos;
	const struct firmware *fw_entry_sd;
	const struct firmware *fw_entry_boot;
	unsigned char *ext_data_source;
	unsigned char *read_config_buf;
	unsigned char intr_mask;
	unsigned char command;
	unsigned char bootloader_id[2];
	unsigned char flash_status;
	unsigned char partitions;
	unsigned short block_size;
	unsigned short config_size;
	unsigned short config_area;
	unsigned short config_block_count;
	unsigned short flash_config_length;
	unsigned short payload_length;
	unsigned short partition_table_bytes;
	unsigned short read_config_buf_size;
	const unsigned char *config_data;
	const unsigned char *image;
	unsigned char *image_name;
	unsigned int image_size;
	struct image_metadata img;
	struct register_offset off;
	struct block_count blkcount;
	struct physical_address phyaddr;
	struct f34_v5v6_flash_properties flash_properties;
	struct synaptics_rmi4_fn_desc f01_fd;
	struct synaptics_rmi4_fn_desc f34_fd;
	struct synaptics_rmi4_fn_desc f35_fd;
	struct synaptics_rmi4_data *rmi4_data;
	struct workqueue_struct *fwu_workqueue;
	struct delayed_work fwu_work;
	struct synaptics_rmi4_exp_fn_ptr *fn_ptr;
};
struct HexValue{
	char m_cHexChar;
	unsigned char m_byValue;
};


static struct synaptics_rmi4_fwu_handle *fwu;

DECLARE_COMPLETION(synptics_fwu_remove_complete);

static void calculate_checksum(unsigned short *data, unsigned long len,
			       unsigned long *result)
{
	unsigned long temp;
	unsigned long sum1 = 0xffff;
	unsigned long sum2 = 0xffff;

	*result = 0xffffffff;

	while (len--) {
		temp = *data;
		sum1 += temp;
		sum2 += sum1;
		sum1 = (sum1 & 0xffff) + (sum1 >> 16);
		sum2 = (sum2 & 0xffff) + (sum2 >> 16);
		data++;
	}

	*result = sum2 << 16 | sum1;

	return;
}

static void convert_to_little_endian(unsigned char *dest, unsigned long src)
{
	dest[0] = (unsigned char)(src & 0xff);
	dest[1] = (unsigned char)((src >> 8) & 0xff);
	dest[2] = (unsigned char)((src >> 16) & 0xff);
	dest[3] = (unsigned char)((src >> 24) & 0xff);

	return;
}

static inline int secure_memcpy(unsigned char *dest, unsigned int dest_size,
				const unsigned char *src, unsigned int src_size,
				unsigned int count)
{
	if (dest == NULL || src == NULL)
		return -EINVAL;

	if (count > dest_size || count > src_size)
		return -EINVAL;

	memcpy((void *)dest, (const void *)src, count);

	return 0;
}

static unsigned int le_to_uint(const unsigned char *ptr)
{
	return (unsigned int)ptr[0] +
	    (unsigned int)ptr[1] * 0x100 +
	    (unsigned int)ptr[2] * 0x10000 + (unsigned int)ptr[3] * 0x1000000;
}

static unsigned int be_to_uint(const unsigned char *ptr)
{
	return (unsigned int)ptr[3] +
	    (unsigned int)ptr[2] * 0x100 +
	    (unsigned int)ptr[1] * 0x10000 + (unsigned int)ptr[0] * 0x1000000;
}

static int fwu_allocate_read_config_buf(unsigned int count)
{
	if (count > fwu->read_config_buf_size) {
		kfree(fwu->read_config_buf);
		fwu->read_config_buf = kzalloc(count, GFP_KERNEL);
		if (!fwu->read_config_buf) {
			TS_LOG_ERR("%s: Failed to alloc mem for fwu->read_config_buf\n", __func__);
			fwu->read_config_buf_size = 0;
			return -ENOMEM;
		}
		fwu->read_config_buf_size = count;
	}

	return 0;
}

static void fwu_compare_partition_tables(void)
{
	if (fwu->phyaddr.ui_firmware != fwu->img.phyaddr.ui_firmware) {
		fwu->new_partition_table = true;
		return;
	}

	if (fwu->phyaddr.ui_config != fwu->img.phyaddr.ui_config) {
		fwu->new_partition_table = true;
		return;
	}

	if (fwu->flash_properties.has_disp_config) {
		if (fwu->phyaddr.dp_config != fwu->img.phyaddr.dp_config) {
			fwu->new_partition_table = true;
			return;
		}
	}

	if (fwu->flash_properties.has_disp_config) {
		if (fwu->phyaddr.dp_config != fwu->img.phyaddr.dp_config) {
			fwu->new_partition_table = true;
			return;
		}
	}

	if (fwu->has_guest_code) {
		if (fwu->phyaddr.guest_code != fwu->img.phyaddr.guest_code) {
			fwu->new_partition_table = true;
			return;
		}
	}

	fwu->new_partition_table = false;

	return;
}

static int synaptics_get_f34_addr(struct synaptics_rmi4_data *rmi4_data,
				  unsigned char *f34_address)
{
	int retval;
	unsigned short addr;
	struct synaptics_rmi4_fn_desc rmi_fd;

	for (addr = PDT_START; addr > PDT_END; addr -= PDT_ENTRY_SIZE) {
		retval =
		    rmi4_data->i2c_read(rmi4_data, addr,
					(unsigned char *)&rmi_fd,
					sizeof(rmi_fd));
		if (retval < 0) {
			TS_LOG_ERR("Failed to read map register\n");
			return retval;
		}

		if (SYNAPTICS_RMI4_F34 == rmi_fd.fn_number) {
			TS_LOG_INFO("Found F%02x\n", rmi_fd.fn_number);
			*f34_address = rmi_fd.ctrl_base_addr;
			return NO_ERR;
		}
	}
	TS_LOG_ERR("Failed to get f34 addr\n");
	return -EINVAL;
}

int synap_fw_s3718_configid(struct synaptics_rmi4_data *rmi4_data, u8 *buf,
				size_t buf_size)
{
	int rc = 0;
	unsigned char config_id[4];
	unsigned char f34_ctrl_base_addr = 0;

	TS_LOG_INFO("synap_fw_s3718_configid called\n");

	rc = synaptics_get_f34_addr(rmi4_data, &f34_ctrl_base_addr);
	if (rc < 0) {
		TS_LOG_ERR("failed to scan pdt\n");
	}

	rc = rmi4_data->i2c_read(rmi4_data, f34_ctrl_base_addr, config_id,
				 sizeof(config_id));
	if (rc < 0) {
		TS_LOG_ERR("Could not read configid\n");
		return rc;
	}

	TS_LOG_INFO("config ID 0x%02X, 0x%02X, 0x%02X, 0x%02X,addr = 0x%02x\n",
		    config_id[0], config_id[1], config_id[2], config_id[3],
		    f34_ctrl_base_addr);

	snprintf(buf, buf_size, "%02x %02x %02x %02x\n",
		 config_id[0], config_id[1], config_id[2], config_id[3]);

	TS_LOG_INFO("buf = %s\n", buf);
	return 0;
}

static void fwu_parse_partition_table(const unsigned char *partition_table,
				      struct block_count *blkcount,
				      struct physical_address *phyaddr)
{
	unsigned char ii;
	unsigned char index;
	unsigned char offset;
	unsigned short partition_length;
	unsigned short physical_address;
	struct partition_table *ptable;

	for (ii = 0; ii < fwu->partitions; ii++) {
		index = ii * 8 + 2;
		ptable = (struct partition_table *)&partition_table[index];
		partition_length =
		    ptable->partition_length_15_8 << 8 | ptable->
		    partition_length_7_0;
		physical_address =
		    ptable->start_physical_address_15_8 << 8 | ptable->
		    start_physical_address_7_0;
		TS_LOG_DEBUG("%s: Partition entry %d:\n", __func__, ii);
		for (offset = 0; offset < 8; offset++) {
			TS_LOG_DEBUG("%s: 0x%02x\n", __func__,
				     partition_table[index + offset]);
		}
		switch (ptable->partition_id) {
		case CORE_CODE_PARTITION:
			blkcount->ui_firmware = partition_length;
			phyaddr->ui_firmware = physical_address;
			TS_LOG_DEBUG("%s: Core code block count: %d\n",
				     __func__, blkcount->ui_firmware);
			break;
		case CORE_CONFIG_PARTITION:
			blkcount->ui_config = partition_length;
			phyaddr->ui_config = physical_address;
			TS_LOG_DEBUG("%s: Core config block count: %d\n",
				     __func__, blkcount->ui_config);
			break;
		case BOOTLOADER_PARTITION:
			blkcount->bl_image = partition_length;
			TS_LOG_DEBUG("%s: Bootloader block count: %d\n",
				     __func__, blkcount->bl_image);
			break;
		case UTILITY_PARAMETER_PARTITION:
			blkcount->utility_param = partition_length;
			TS_LOG_DEBUG("%s: Utility parameter block count: %d\n",
				     __func__, blkcount->utility_param);
			break;
		case DISPLAY_CONFIG_PARTITION:
			blkcount->dp_config = partition_length;
			phyaddr->dp_config = physical_address;
			TS_LOG_DEBUG("%s: Display config block count: %d\n",
				     __func__, blkcount->dp_config);
			break;
		case FLASH_CONFIG_PARTITION:
			blkcount->fl_config = partition_length;
			TS_LOG_DEBUG("%s: Flash config block count: %d\n",
				     __func__, blkcount->fl_config);
			break;
		case GUEST_CODE_PARTITION:
			blkcount->guest_code = partition_length;
			phyaddr->guest_code = physical_address;
			TS_LOG_DEBUG("%s: Guest code block count: %d\n",
				     __func__, blkcount->guest_code);
			break;
		case GUEST_SERIALIZATION_PARTITION:
			blkcount->pm_config = partition_length;
			TS_LOG_DEBUG
			    ("%s: Guest serialization block count: %d\n",
			     __func__, blkcount->pm_config);
			break;
		case GLOBAL_PARAMETERS_PARTITION:
			blkcount->bl_config = partition_length;
			TS_LOG_DEBUG("%s: Global parameters block count: %d\n",
				     __func__, blkcount->bl_config);
			break;
		case DEVICE_CONFIG_PARTITION:
			blkcount->lockdown = partition_length;
			TS_LOG_DEBUG("%s: Device config block count: %d\n",
				     __func__, blkcount->lockdown);
			break;
		};
	}

	return;
}

static void fwu_parse_image_header_10_utility(const unsigned char *image)
{
	unsigned char ii;
	unsigned char num_of_containers;
	unsigned int addr;
	unsigned int container_id;
	unsigned int length;
	const unsigned char *content;
	struct container_descriptor *descriptor;

	num_of_containers = fwu->img.utility.size / 4;

	if (num_of_containers)
		fwu->img.contains_utility_param = true;

	for (ii = 0; ii < num_of_containers; ii++) {
		addr = le_to_uint(fwu->img.utility.data + (ii * 4));
		descriptor = (struct container_descriptor *)(image + addr);
		container_id = descriptor->container_id[0] |
		    descriptor->container_id[1] << 8;
		content = image + le_to_uint(descriptor->content_address);
		length = le_to_uint(descriptor->content_length);
		switch (container_id) {
		case UTILITY_PARAMETER_CONTAINER:
			switch (content[0]) {
			case FORCE_PARAMETER:
				fwu->img.force_param.data = content;
				fwu->img.force_param.size = length;
				break;
			case ANTI_BENDING_PARAMETER:
				fwu->img.anti_bending_param.data = content;
				fwu->img.anti_bending_param.size = length;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		};
	}

	return;
}

static void fwu_parse_image_header_10_bl_container(const unsigned char *image)
{
	unsigned char ii;
	unsigned char num_of_containers;
	unsigned int addr;
	unsigned int container_id;
	unsigned int length;
	const unsigned char *content;
	struct container_descriptor *descriptor;

	num_of_containers = (fwu->img.bootloader.size - 4) / 4;

	for (ii = 1; ii <= num_of_containers; ii++) {
		addr = le_to_uint(fwu->img.bootloader.data + (ii * 4));
		descriptor = (struct container_descriptor *)(image + addr);
		container_id =
		    descriptor->container_id[0] | descriptor->
		    container_id[1] << 8;
		content = image + le_to_uint(descriptor->content_address);
		length = le_to_uint(descriptor->content_length);
		switch (container_id) {
		case BL_IMAGE_CONTAINER:
			fwu->img.bl_image.data = content;
			fwu->img.bl_image.size = length;
			break;
		case BL_CONFIG_CONTAINER:
		case GLOBAL_PARAMETERS_CONTAINER:
			fwu->img.bl_config.data = content;
			fwu->img.bl_config.size = length;
			break;
		case BL_LOCKDOWN_INFO_CONTAINER:
		case DEVICE_CONFIG_CONTAINER:
			fwu->img.lockdown.data = content;
			fwu->img.lockdown.size = length;
			break;
		default:
			break;
		};
	}

	return;
}
/* Check offset + size <= bound.  1 if in bounds, 0 otherwise. */
static bool in_bounds(unsigned long offset, unsigned long size, unsigned long bound)
{
	if (offset > bound || size > bound) {
		TS_LOG_ERR("%s: %lu or %lu > %lu\n", __func__, offset, size, bound);
		return false;
	}
	if (offset > (bound - size)) {
		TS_LOG_ERR("%s: %lu > %lu - %lu\n", __func__, offset, size, bound);
		return false;
	}
	return true;
}
static int fwu_parse_image_header_10(void)
{
	unsigned char ii = 0;
	unsigned int num_of_containers = 0;
	unsigned int addr = 0;
	unsigned int offset = 0;
	unsigned int container_id = 0;
	unsigned int length = 0;
	const unsigned char *image = NULL;
	const unsigned char *content = NULL;
	struct container_descriptor *descriptor = NULL;
	struct image_header_10 *header = NULL;

	if(!fwu || !fwu->image || (fwu->image_size < sizeof(*header))){
		TS_LOG_ERR("fwu is NULL or provided update too small !\n");
		return -EINVAL;
	}
	image = fwu->image;
	header = (struct image_header_10 *)image;

	fwu->img.checksum = le_to_uint(header->checksum);

	/* address of top level container */
	offset = le_to_uint(header->top_level_container_start_addr);
	descriptor = (struct container_descriptor *)(image + offset);

	/* address of top level container content */
	offset = le_to_uint(descriptor->content_address);
	num_of_containers = le_to_uint(descriptor->content_length) / DATA_OFFET_4BIT;

	for (ii = 0; ii < num_of_containers; ii++) {
		addr = le_to_uint(image + offset);
		offset += DATA_OFFET_4BIT;
		descriptor = (struct container_descriptor *)(image + addr);
		container_id =
			descriptor->container_id[0] | descriptor->
			container_id[1] << DATA_OFFET_8BIT;
		content = image + le_to_uint(descriptor->content_address);
		length = le_to_uint(descriptor->content_length);
		if(length){
			if (!in_bounds(offset - DATA_OFFET_4BIT, length, fwu->image_size) &&
				!in_bounds(le_to_uint(descriptor->content_address),
					length, fwu->image_size)) {
				TS_LOG_ERR("fw img_size out of bounds\n");
				return -EINVAL;
			}
		}
		switch (container_id) {
		case UI_CONTAINER:
		case CORE_CODE_CONTAINER:
			fwu->img.ui_firmware.data = content;
			fwu->img.ui_firmware.size = length;
			break;
		case UI_CONFIG_CONTAINER:
		case CORE_CONFIG_CONTAINER:
			fwu->img.ui_config.data = content;
			fwu->img.ui_config.size = length;
			break;
		case BL_CONTAINER:
			fwu->img.bl_version = *content;
			fwu->img.bootloader.data = content;
			fwu->img.bootloader.size = length;
			fwu_parse_image_header_10_bl_container(image);
			break;
		case UTILITY_CONTAINER:
			fwu->img.utility.data = content;
			fwu->img.utility.size = length;
			fwu_parse_image_header_10_utility(image);
			break;
		case GUEST_CODE_CONTAINER:
			fwu->img.contains_guest_code = true;
			fwu->img.guest_code.data = content;
			fwu->img.guest_code.size = length;
			break;
		case DISPLAY_CONFIG_CONTAINER:
			fwu->img.contains_disp_config = true;
			fwu->img.dp_config.data = content;
			fwu->img.dp_config.size = length;
			break;
		case FLASH_CONFIG_CONTAINER:
			fwu->img.contains_flash_config = true;
			fwu->img.fl_config.data = content;
			fwu->img.fl_config.size = length;
			break;
		case GENERAL_INFORMATION_CONTAINER:
			fwu->img.contains_firmware_id = true;
			fwu->img.firmware_id = le_to_uint(content + DATA_OFFET_4BIT);
			break;
		default:
			break;
		}
	}

	return 0;
}

static int fwu_parse_image_header_05_06(void)
{
	int retval = 0;
	const unsigned char *image = NULL;
	struct image_header_05_06 *header = NULL;

	if(!fwu || !fwu->image || (fwu->image_size < sizeof(*header))){
		TS_LOG_ERR("fwu is NULL or provided update too small !\n");
		return -EINVAL;
	}

	image = fwu->image;
	header = (struct image_header_05_06 *)image;
	if((fwu->image_size < IMAGE_AREA_OFFSET) ||
		(fwu->image_size < sizeof(*header)) ||
		(fwu->image_size < le_to_uint(header->bootloader_size)) ||
		(fwu->image_size < le_to_uint(header->firmware_size)) ||
		(fwu->image_size < le_to_uint(header->config_size)) ||
		(fwu->image_size < le_to_uint(header->dsp_cfg_size)) ||
		(fwu->image_size < LOCKDOWN_SIZE)){
			TS_LOG_ERR("provided update too small !\n");
			return -EINVAL;
	}
	fwu->img.checksum = le_to_uint(header->checksum);

	if(header->header_version == IMAGE_HEADER_VERSION_05)
		fwu->img.bl_version = header->header_version + 1;
	else
		fwu->img.bl_version = header->header_version;

	fwu->img.contains_bootloader = header->options_bootloader;
	if (fwu->img.contains_bootloader)
		fwu->img.bootloader_size = le_to_uint(header->bootloader_size);

	fwu->img.ui_firmware.size = le_to_uint(header->firmware_size);
	if (fwu->img.ui_firmware.size) {
		fwu->img.ui_firmware.data = image + IMAGE_AREA_OFFSET;
		if (fwu->img.contains_bootloader)
			fwu->img.ui_firmware.data += fwu->img.bootloader_size;
	}

	if ((fwu->img.bl_version == BL_V6) && header->options_tddi)
		fwu->img.ui_firmware.data = image + IMAGE_AREA_OFFSET;

	fwu->img.ui_config.size = le_to_uint(header->config_size);
	if (fwu->img.ui_config.size) {
		fwu->img.ui_config.data =
		    fwu->img.ui_firmware.data + fwu->img.ui_firmware.size;
	}

	if(fwu->img.contains_bootloader || header->options_tddi )
		fwu->img.contains_disp_config = true;
	else
		fwu->img.contains_disp_config = false;

	if (fwu->img.contains_disp_config) {
		fwu->img.disp_config_offset = le_to_uint(header->dsp_cfg_addr);
		fwu->img.dp_config.size = le_to_uint(header->dsp_cfg_size);
		fwu->img.dp_config.data = image + fwu->img.disp_config_offset;
	} else {
		retval =
		    secure_memcpy(fwu->img.cstmr_product_id,
				  sizeof(fwu->img.cstmr_product_id),
				  header->cstmr_product_id,
				  sizeof(header->cstmr_product_id),
				  PRODUCT_ID_SIZE);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to copy custom product ID string\n",__func__);
		}
		fwu->img.cstmr_product_id[PRODUCT_ID_SIZE] = 0;
	}

	fwu->img.contains_firmware_id = header->options_firmware_id;
	if (fwu->img.contains_firmware_id)
		fwu->img.firmware_id = le_to_uint(header->firmware_id);

	retval = secure_memcpy(fwu->img.product_id,
			       sizeof(fwu->img.product_id),
			       header->product_id,
			       sizeof(header->product_id), PRODUCT_ID_SIZE);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to copy product ID string\n", __func__);
	}
	fwu->img.product_id[PRODUCT_ID_SIZE] = 0;

	fwu->img.lockdown.size = LOCKDOWN_SIZE;
	fwu->img.lockdown.data = image + IMAGE_AREA_OFFSET - LOCKDOWN_SIZE;

	return 0;
}

static int fwu_parse_image_info(void)
{
	struct image_header_10 *header = NULL;

	if(!fwu || !fwu->image){
		TS_LOG_ERR("fwu is NULL !\n");
		return -EINVAL;
	}
	header = (struct image_header_10 *)fwu->image;
	memset(&fwu->img, 0x00, sizeof(fwu->img));

	TS_LOG_INFO("header->major_header_version is %x",
		    header->major_header_version);

	switch (header->major_header_version) {
	case IMAGE_HEADER_VERSION_10:
		if(fwu_parse_image_header_10() < 0){
			TS_LOG_ERR("%s: parse image header error\n",__func__);
			return -EINVAL;
		}
		break;
	case IMAGE_HEADER_VERSION_05:
	case IMAGE_HEADER_VERSION_06:
		if(fwu_parse_image_header_05_06() < 0){
			TS_LOG_ERR("%s: parse image header error\n",__func__);
			return -EINVAL;
		}
		break;
	default:
		TS_LOG_ERR("%s: Unsupported image file format (0x%02x)\n",
			   __func__, header->major_header_version);
		return -EINVAL;
	}

	if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8) {
		if (!fwu->img.contains_flash_config) {
			TS_LOG_ERR
			    ("%s: No flash config found in firmware image\n",__func__);
			return -EINVAL;
		}

		fwu_parse_partition_table(fwu->img.fl_config.data,
					  &fwu->img.blkcount,
					  &fwu->img.phyaddr);
		if (fwu->rmi4_data->synaptics_chip_data->ic_type == SYNAPTICS_S3706) {
			if (fwu->img.blkcount.utility_param)  
			fwu->img.contains_utility_param = true;
			}

		fwu_compare_partition_tables();
	} else {
		fwu->new_partition_table = false;
		if(fwu->rmi4_data->synaptics_chip_data->ic_type == SYNAPTICS_S3706)
			fwu->incompatible_partition_tables = false;
	}
	TS_LOG_INFO("fwu_parse_image_info out\n");
	return 0;
}

static int fwu_read_flash_status(void)
{
	int retval;
	unsigned char status;
	unsigned char command;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	retval = fwu->fn_ptr->read(rmi4_data,
				   fwu->f34_fd.data_base_addr +
				   fwu->off.flash_status, &status,
				   sizeof(status));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read flash status\n");
		return retval;
	}

	TS_LOG_DEBUG("dbg : flash status = 0x%x\n", status);

	fwu->in_bl_mode = status >> 7;

	if (fwu->bl_version == BL_V5)
		fwu->flash_status = (status >> 4) & MASK_3BIT;
	else if (fwu->bl_version == BL_V6)
		fwu->flash_status = status & MASK_3BIT;
	else if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8) 
		fwu->flash_status = status & MASK_5BIT;

	if (rmi4_data->synaptics_chip_data->bootloader_update_enable
	    && fwu->write_bootloader)
			fwu->flash_status = FLASH_STATUS_00;
	if (fwu->flash_status != FLASH_STATUS_00) {
		TS_LOG_INFO("%s: Flash status = %d, command = 0x%02x\n",
			   __func__, fwu->flash_status, fwu->command);
	}

	if (fwu->bl_version == BL_V8) {
		if (fwu->flash_status == FLASH_STATUS_08)
			fwu->flash_status = FLASH_STATUS_00;
	}

	retval = fwu->fn_ptr->read(rmi4_data,
				   fwu->f34_fd.data_base_addr +
				   fwu->off.flash_cmd, &command,
				   sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read flash command\n", __func__);
		return retval;
	}

	if (fwu->bl_version == BL_V5)
		fwu->command = command & MASK_4BIT;
	else if (fwu->bl_version == BL_V6)
		fwu->command = command & MASK_6BIT;
	else if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8)
		fwu->command = command;
	else
	     TS_LOG_ERR("%s: is default.\n", __func__);

	if (rmi4_data->synaptics_chip_data->bootloader_update_enable
	    && fwu->write_bootloader)
		fwu->command = 0x00;

	return 0;
}

static int fwu_wait_for_idle(int timeout_ms, bool poll)
{
	int count = 0;
	int timeout_count = ((timeout_ms * 1000) / MAX_SLEEP_TIME_US) + 1;

	do {
		usleep_range(MIN_SLEEP_TIME_US, MAX_SLEEP_TIME_US);

		count++;
		if (poll || (count == timeout_count))
			fwu_read_flash_status();

		if ((fwu->command == CMD_IDLE) && (fwu->flash_status == FLASH_STATUS_00))
			return 0;
	} while (count < timeout_count);

	TS_LOG_ERR("%s: Timed out waiting for idle status\n", __func__);

	return -ETIMEDOUT;
}

static int fwu_write_f34_v7_command_single_transaction(unsigned char cmd)
{
	int retval;
	unsigned char base;
	struct f34_v7_data_1_5 data_1_5;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	memset(data_1_5.data, 0x00, sizeof(data_1_5.data));

	switch (cmd) {
	case CMD_ERASE_ALL:
		data_1_5.partition_id = CORE_CODE_PARTITION;
		data_1_5.command = CMD_V7_ERASE_AP;
		break;
	case CMD_ERASE_UI_FIRMWARE:
		data_1_5.partition_id = CORE_CODE_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_BL_CONFIG:
		data_1_5.partition_id = GLOBAL_PARAMETERS_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_UI_CONFIG:
		data_1_5.partition_id = CORE_CONFIG_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_DISP_CONFIG:
		data_1_5.partition_id = DISPLAY_CONFIG_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_FLASH_CONFIG:
		data_1_5.partition_id = FLASH_CONFIG_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_GUEST_CODE:
		data_1_5.partition_id = GUEST_CODE_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_BOOTLOADER:
		data_1_5.partition_id = BOOTLOADER_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_UTILITY_PARAMETER:
		data_1_5.partition_id = UTILITY_PARAMETER_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ENABLE_FLASH_PROG:
		data_1_5.partition_id = BOOTLOADER_PARTITION;
		data_1_5.command = CMD_V7_ENTER_BL;
		break;
	};

	data_1_5.payload_0 = fwu->bootloader_id[0];
	data_1_5.payload_1 = fwu->bootloader_id[1];

	retval = fwu->fn_ptr->write(rmi4_data,
				    base + fwu->off.partition_id,
				    data_1_5.data, sizeof(data_1_5.data));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write single transaction command\n",__func__);
		return retval;
	}

	return 0;
}

static int fwu_write_f34_v7_command(unsigned char cmd)
{
	int retval;
	unsigned char base;
	unsigned char command;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	switch (cmd) {
	case CMD_WRITE_FW:
	case CMD_WRITE_CONFIG:
	case CMD_WRITE_GUEST_CODE:
	case CMD_WRITE_BOOTLOADER:
	case CMD_WRITE_UTILITY_PARAM:
		command = CMD_V7_WRITE;
		break;
	case CMD_READ_CONFIG:
#ifdef SYNA_UPP
	case CMD_READ_UTILITY_PARAM:
#endif
		command = CMD_V7_READ;
		break;
	case CMD_ERASE_ALL:
		command = CMD_V7_ERASE_AP;
		break;
	case CMD_ERASE_UI_FIRMWARE:
	case CMD_ERASE_BL_CONFIG:
	case CMD_ERASE_UI_CONFIG:
	case CMD_ERASE_DISP_CONFIG:
	case CMD_ERASE_FLASH_CONFIG:
	case CMD_ERASE_GUEST_CODE:
	case CMD_ERASE_BOOTLOADER:
	case CMD_ERASE_UTILITY_PARAMETER:
		command = CMD_V7_ERASE;
		break;
	case CMD_ENABLE_FLASH_PROG:
		command = CMD_V7_ENTER_BL;
		break;
	default:
		TS_LOG_ERR("%s: Invalid command 0x%02x\n", __func__, cmd);
		return -EINVAL;
	};

	fwu->command = command;

	switch (cmd) {
	case CMD_ERASE_ALL:
	case CMD_ERASE_UI_FIRMWARE:
	case CMD_ERASE_BL_CONFIG:
	case CMD_ERASE_UI_CONFIG:
	case CMD_ERASE_DISP_CONFIG:
	case CMD_ERASE_FLASH_CONFIG:
	case CMD_ERASE_GUEST_CODE:
	case CMD_ERASE_BOOTLOADER:
	case CMD_ERASE_UTILITY_PARAMETER:
	case CMD_ENABLE_FLASH_PROG:
		retval = fwu_write_f34_v7_command_single_transaction(cmd);
		if (retval < 0)
			return retval;
		else
			return 0;
	default:
		break;
	};

	retval = fwu->fn_ptr->write(rmi4_data,
				    base + fwu->off.flash_cmd,
				    &command, sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write flash command\n", __func__);
		return retval;
	}

	return 0;
}

static int fwu_write_f34_v5v6_command(unsigned char cmd)
{
	int retval;
	unsigned char base;
	unsigned char command;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	switch (cmd) {
	case CMD_IDLE:
		command = CMD_V5V6_IDLE;
		break;
	case CMD_WRITE_FW:
		command = CMD_V5V6_WRITE_FW;
		break;
	case CMD_WRITE_CONFIG:
		command = CMD_V5V6_WRITE_CONFIG;
		break;
	case CMD_WRITE_LOCKDOWN:
		command = CMD_V5V6_WRITE_LOCKDOWN;
		break;
	case CMD_WRITE_GUEST_CODE:
		command = CMD_V5V6_WRITE_GUEST_CODE;
		break;
	case CMD_READ_CONFIG:
		command = CMD_V5V6_READ_CONFIG;
		break;
	case CMD_ERASE_ALL:
		command = CMD_V5V6_ERASE_ALL;
		break;
	case CMD_ERASE_UI_CONFIG:
		command = CMD_V5V6_ERASE_UI_CONFIG;
		break;
	case CMD_ERASE_DISP_CONFIG:
		command = CMD_V5V6_ERASE_DISP_CONFIG;
		break;
	case CMD_ERASE_GUEST_CODE:
		command = CMD_V5V6_ERASE_GUEST_CODE;
		break;
	case CMD_ENABLE_FLASH_PROG:
		command = CMD_V5V6_ENABLE_FLASH_PROG;
		break;
	case CMD_ERASE_FORCE_CONFIG:
		command = CMD_V5V6_ERASE_FORCE_CONFIG;
		break;
	case CMD_READ_FORCE_CONFIG:
		command = CMD_V5V6_READ_FORCE_CONFIG;
		break;
	case CMD_WRITE_FORCE_CONFIG:
		command = CMD_V5V6_WRITE_CONFIG;
		break;
	case CMD_ERASE_LOCKDOWN_DATA:
		command = CMD_V5V6_ERASE_LOCKDOWN_DATA;
		break;
	case CMD_READ_LOCKDOWN_DATA:
		command = CMD_V5V6_READ_LOCKDOWN_DATA;
		break;
	case CMD_WRITE_LOCKDOWN_DATA:
		command = CMD_V5V6_WRITE_LOCKDOWN_DATA;
		break;
	case CMD_ERASE_LCM_DATA:
		command = CMD_V5V6_ERASE_LCM_DATA;
		break;
	case CMD_ERASE_OEM_DATA:
		command = CMD_V5V6_ERASE_OEM_DATA;
		break;
	default:
		TS_LOG_ERR("%s: Invalid command 0x%02x\n", __func__, cmd);
		return -EINVAL;
	}

	switch (cmd) {
	case CMD_ERASE_ALL:
	case CMD_ERASE_UI_CONFIG:
	case CMD_ERASE_DISP_CONFIG:
	case CMD_ERASE_GUEST_CODE:
	case CMD_ERASE_FORCE_CONFIG:
	case CMD_ERASE_LOCKDOWN_DATA:
	case CMD_ERASE_LCM_DATA:
	case CMD_ERASE_OEM_DATA:
	case CMD_ENABLE_FLASH_PROG:
		retval =
		    fwu->fn_ptr->write(rmi4_data, base + fwu->off.payload,
				       fwu->bootloader_id,
				       sizeof(fwu->bootloader_id));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to write bootloader ID\n",__func__);
			return retval;
		}
		break;
	default:
		break;
	};

	fwu->command = command;

	retval = fwu->fn_ptr->write(rmi4_data,
				    base + fwu->off.flash_cmd,
				    &command, sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write command 0x%02x\n", __func__,
			   command);
		return retval;
	}

	return 0;
}

static int fwu_write_f34_command(unsigned char cmd)
{
	int retval;

	if (fwu->bl_version == BL_V7|| fwu->bl_version == BL_V8) 
		retval = fwu_write_f34_v7_command(cmd);
	else
		retval = fwu_write_f34_v5v6_command(cmd);

	return retval;
}

static int fwu_write_f34_v7_partition_id(unsigned char cmd)
{
	int retval;
	unsigned char base;
	unsigned char partition;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	switch (cmd) {
	case CMD_WRITE_FW:
		partition = CORE_CODE_PARTITION;
		break;
	case CMD_WRITE_CONFIG:
	case CMD_READ_CONFIG:
		if (fwu->config_area == UI_CONFIG_AREA)
			partition = CORE_CONFIG_PARTITION;
		else if (fwu->config_area == DP_CONFIG_AREA)
			partition = DISPLAY_CONFIG_PARTITION;
		else if (fwu->config_area == PM_CONFIG_AREA)
			partition = GUEST_SERIALIZATION_PARTITION;
		else if (fwu->config_area == BL_CONFIG_AREA)
			partition = GLOBAL_PARAMETERS_PARTITION;
		else if (fwu->config_area == FLASH_CONFIG_AREA)
			partition = FLASH_CONFIG_PARTITION;
		else if (fwu->config_area == UPP_AREA)
			partition = UTILITY_PARAMETER_PARTITION;
		else
		    TS_LOG_ERR("%s: is default.\n", __func__);
		break;
	case CMD_WRITE_GUEST_CODE:
		partition = GUEST_CODE_PARTITION;
		break;
	case CMD_WRITE_BOOTLOADER:
		partition = BOOTLOADER_PARTITION;
		break;
	case CMD_WRITE_UTILITY_PARAM:
		partition = UTILITY_PARAMETER_PARTITION;
		break;
#ifdef SYNA_UPP
	case CMD_READ_UTILITY_PARAM:
		partition = UTILITY_PARAMETER_PARTITION;
		TS_LOG_INFO("partition = %d\n", partition);
		break;
#endif
	case CMD_ERASE_ALL:
		partition = CORE_CODE_PARTITION;
		break;
	case CMD_ERASE_BL_CONFIG:
		partition = GLOBAL_PARAMETERS_PARTITION;
		break;
	case CMD_ERASE_UI_CONFIG:
		partition = CORE_CONFIG_PARTITION;
		break;
	case CMD_ERASE_DISP_CONFIG:
		partition = DISPLAY_CONFIG_PARTITION;
		break;
	case CMD_ERASE_FLASH_CONFIG:
		partition = FLASH_CONFIG_PARTITION;
		break;
	case CMD_ERASE_GUEST_CODE:
		partition = GUEST_CODE_PARTITION;
		break;
	case CMD_ERASE_BOOTLOADER:
		partition = BOOTLOADER_PARTITION;
		break;
	case CMD_ENABLE_FLASH_PROG:
		partition = BOOTLOADER_PARTITION;
		break;
	default:
		TS_LOG_ERR("%s: Invalid command 0x%02x\n", __func__, cmd);
		return -EINVAL;
	};

	retval = fwu->fn_ptr->write(rmi4_data,
				    base + fwu->off.partition_id,
				    &partition, sizeof(partition));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write partition ID\n", __func__);
		return retval;
	}

	return 0;
}

static int fwu_write_f34_partition_id(unsigned char cmd)
{
	int retval;

	if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8) 
		retval = fwu_write_f34_v7_partition_id(cmd);
	else
		retval = 0;

	return retval;
}

static int fwu_read_f34_v7_partition_table(void)
{
	int retval;
	unsigned char base;
	unsigned char length[2];
	unsigned short block_number = 0;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	fwu->config_area = FLASH_CONFIG_AREA;

	retval = fwu_write_f34_partition_id(CMD_READ_CONFIG);
	if (retval < 0)
		return retval;

	retval = fwu->fn_ptr->write(rmi4_data,
				    base + fwu->off.block_number,
				    (unsigned char *)&block_number,
				    sizeof(block_number));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write block number\n", __func__);
		return retval;
	}

	length[0] = (unsigned char)(fwu->flash_config_length & MASK_8BIT);
	length[1] = (unsigned char)(fwu->flash_config_length >> 8);

	retval = fwu->fn_ptr->write(rmi4_data,
				    base + fwu->off.transfer_length,
				    length, sizeof(length));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write transfer length\n", __func__);
		return retval;
	}

	retval = fwu_write_f34_command(CMD_READ_CONFIG);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write command\n", __func__);
		return retval;
	}

	retval = fwu_wait_for_idle(WRITE_WAIT_MS, true);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to wait for idle status\n", __func__);
		return retval;
	}

	retval = fwu->fn_ptr->read(rmi4_data,
				   base + fwu->off.payload,
				   fwu->read_config_buf,
				   fwu->partition_table_bytes);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read block data\n", __func__);
		return retval;
	}

	return 0;
}

static int fwu_read_f34_v7_queries(void)
{
	int retval;
	unsigned char ii;
	unsigned char base;
	unsigned char index;
	unsigned char offset;
	unsigned char *ptable;
	struct f34_v7_query_0 query_0;
	struct f34_v7_query_1_7 query_1_7;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.query_base_addr;

	retval = fwu->fn_ptr->read(rmi4_data,
				   base, query_0.data, sizeof(query_0.data));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read query 0\n", __func__);
		return retval;
	}

	offset = query_0.subpacket_1_size + 1;

	retval = fwu->fn_ptr->read(rmi4_data,
				   base + offset,
				   query_1_7.data, sizeof(query_1_7.data));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read queries 1 to 7\n", __func__);
		return retval;
	}

	fwu->bootloader_id[0] = query_1_7.bl_minor_revision;
	fwu->bootloader_id[1] = query_1_7.bl_major_revision;

	if (fwu->bootloader_id[1] == BL_V8){
		fwu->bl_version = BL_V8;
		TS_LOG_INFO("%s: change bootloader version to 8\n", __func__);
	}
	fwu->block_size = query_1_7.block_size_15_8 << 8 |
	    query_1_7.block_size_7_0;

	fwu->flash_config_length = query_1_7.flash_config_length_15_8 << 8 |
	    query_1_7.flash_config_length_7_0;

	fwu->payload_length = query_1_7.payload_length_15_8 << 8 |
	    query_1_7.payload_length_7_0;

	fwu->off.flash_status = V7_FLASH_STATUS_OFFSET;
	fwu->off.partition_id = V7_PARTITION_ID_OFFSET;
	fwu->off.block_number = V7_BLOCK_NUMBER_OFFSET;
	fwu->off.transfer_length = V7_TRANSFER_LENGTH_OFFSET;
	fwu->off.flash_cmd = V7_COMMAND_OFFSET;
	fwu->off.payload = V7_PAYLOAD_OFFSET;

	fwu->flash_properties.has_disp_config = query_1_7.has_display_config;
	fwu->flash_properties.has_pm_config = query_1_7.has_guest_serialization;
	fwu->flash_properties.has_bl_config = query_1_7.has_global_parameters;

	fwu->has_guest_code = query_1_7.has_guest_code;

	index = sizeof(query_1_7.data) - V7_PARTITION_SUPPORT_BYTES;

	fwu->partitions = 0;
	for (offset = 0; offset < V7_PARTITION_SUPPORT_BYTES; offset++) {
		for (ii = 0; ii < 8; ii++) {
			if (query_1_7.data[index + offset] & (1 << ii))
				fwu->partitions++;
		}

		TS_LOG_INFO("%s: Supported partitions: 0x%02x\n", __func__,
			     query_1_7.data[index + offset]);
	}

	fwu->partition_table_bytes = fwu->partitions * 8 + 2;

	kfree(fwu->read_config_buf);
	fwu->read_config_buf = kzalloc(fwu->partition_table_bytes, GFP_KERNEL);
	if (!fwu->read_config_buf) {
		TS_LOG_ERR("%s: Failed to alloc mem for fwu->read_config_buf\n",__func__);
		fwu->read_config_buf_size = 0;
		return -ENOMEM;
	}
	fwu->read_config_buf_size = fwu->partition_table_bytes;
	ptable = fwu->read_config_buf;

	retval = fwu_read_f34_v7_partition_table();
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read partition table\n", __func__);
		return retval;
	}

	fwu_parse_partition_table(ptable, &fwu->blkcount, &fwu->phyaddr);

	if (rmi4_data->synaptics_chip_data->bootloader_update_enable) {
		if (fwu->blkcount.utility_param)
			fwu->has_utility_param = 1;
		else
			fwu->has_utility_param = 0;
	}

	return 0;
}

static int fwu_read_f34_v5v6_queries(void)
{
	int retval;
	unsigned char count;
	unsigned char base;
	unsigned char buf[10];
	struct f34_v5v6_flash_properties_2 properties_2;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.query_base_addr;

	retval = fwu->fn_ptr->read(rmi4_data,
				   base + V5V6_BOOTLOADER_ID_OFFSET,
				   fwu->bootloader_id,
				   sizeof(fwu->bootloader_id));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read bootloader ID\n", __func__);
		return retval;
	}

	if (fwu->bl_version == BL_V5) {
		fwu->off.properties = V5_PROPERTIES_OFFSET;
		fwu->off.block_size = V5_BLOCK_SIZE_OFFSET;
		fwu->off.block_count = V5_BLOCK_COUNT_OFFSET;
		fwu->off.block_number = V5_BLOCK_NUMBER_OFFSET;
		fwu->off.payload = V5_BLOCK_DATA_OFFSET;
	} else if (fwu->bl_version == BL_V6) {
		fwu->off.properties = V6_PROPERTIES_OFFSET;
		fwu->off.properties_2 = V6_PROPERTIES_2_OFFSET;
		fwu->off.block_size = V6_BLOCK_SIZE_OFFSET;
		fwu->off.block_count = V6_BLOCK_COUNT_OFFSET;
		fwu->off.gc_block_count = V6_GUEST_CODE_BLOCK_COUNT_OFFSET;
		fwu->off.block_number = V6_BLOCK_NUMBER_OFFSET;
		fwu->off.payload = V6_BLOCK_DATA_OFFSET;
	}

	retval = fwu->fn_ptr->read(rmi4_data,
				   base + fwu->off.block_size, buf, 2);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read block size info\n", __func__);
		return retval;
	}

	batohs(&fwu->block_size, &(buf[0]));

	if (fwu->bl_version == BL_V5) {
		fwu->off.flash_cmd = fwu->off.payload + fwu->block_size;
		fwu->off.flash_status = fwu->off.flash_cmd;
	} else if (fwu->bl_version == BL_V6) {
		fwu->off.flash_cmd = V6_FLASH_COMMAND_OFFSET;
		fwu->off.flash_status = V6_FLASH_STATUS_OFFSET;
	}

	retval = fwu->fn_ptr->read(rmi4_data,
				   base + fwu->off.properties,
				   fwu->flash_properties.data,
				   sizeof(fwu->flash_properties.data));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read flash properties\n", __func__);
		return retval;
	}

	count = 4;

	if (fwu->flash_properties.has_pm_config)
		count += 2;

	if (fwu->flash_properties.has_bl_config)
		count += 2;

	if (fwu->flash_properties.has_disp_config)
		count += 2;

	retval = fwu->fn_ptr->read(rmi4_data,
				   base + fwu->off.block_count, buf, count);
	if (retval < 0) {
		TS_LOG_ERR("Failed to read block count info\n");
		return retval;
	}

	batohs(&fwu->blkcount.ui_firmware, &(buf[0]));
	batohs(&fwu->blkcount.ui_config, &(buf[2]));

	count = 4;

	if (fwu->flash_properties.has_pm_config) {
		batohs(&fwu->blkcount.pm_config, &(buf[count]));
		count += 2;
	}

	if (fwu->flash_properties.has_bl_config) {
		batohs(&fwu->blkcount.bl_config, &(buf[count]));
		count += 2;
	}

	if (fwu->flash_properties.has_disp_config)
		batohs(&fwu->blkcount.dp_config, &(buf[count]));

	fwu->has_guest_code = false;
	fwu->has_force_config = false;
	fwu->has_lockdown_data = false;
	fwu->has_lcm_data = false;
	fwu->has_oem_data = false;

	if (fwu->flash_properties.has_query4) {
		retval =
		    fwu->fn_ptr->read(rmi4_data, base + fwu->off.properties_2,
				      properties_2.data,
				      sizeof(properties_2.data));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read flash properties 2\n",__func__);
			return retval;
		}

		count = 0;
		if (properties_2.has_guest_code) {
			fwu->has_guest_code = true;
			count +=2;
		}
		if (properties_2.has_force_config){
			fwu->has_force_config = true;
			count +=2;
		}
		if (properties_2.has_lockdown_data){
			fwu->has_lockdown_data = true;
			count += 2;
		}
		if (properties_2.has_lcm_data){
			fwu->has_lcm_data = true;
			count += 2;
		}
		if (properties_2.has_oem_data){
			fwu->has_oem_data = true;
			count += 2;
		}

		count = 0;
		if (properties_2.has_guest_code){
			retval = fwu->fn_ptr->read(rmi4_data,
					base + fwu->off.properties_2 + 1 + count,
					buf,
					2);
			if (retval < 0) {
				TS_LOG_ERR("%s: Failed to read guest code block count\n",__func__);
				return retval;
			}
			batohs(&fwu->blkcount.guest_code, &(buf[0]));
			count++;
			fwu->has_guest_code = true;
		}
		if (properties_2.has_force_config){
			retval = fwu->fn_ptr->read(rmi4_data,
					base + fwu->off.properties_2 + 1 + count,
					buf,
					2);
			if (retval < 0) {
				TS_LOG_ERR("%s: Failed to read guest code block count\n",__func__);
				return retval;
			}
			batohs(&fwu->blkcount.force_config, &(buf[0]));
			count++;
			fwu->has_force_config = true;
		}
		if (properties_2.has_lockdown_data){
			retval = fwu->fn_ptr->read(rmi4_data,
					base + fwu->off.properties_2 + 1 + count,
					buf,
					2);
			if (retval < 0) {
				TS_LOG_ERR("%s: Failed to read guest code block count\n",__func__);
				return retval;
			}

			batohs(&fwu->blkcount.lockdown_data, &(buf[0]));
			count++;
			fwu->has_lockdown_data = true;
		}
		if (properties_2.has_lcm_data){
			retval = fwu->fn_ptr->read(rmi4_data,
					base + fwu->off.properties_2 + 1 + count,
					buf,
					2);
			if (retval < 0) {
				TS_LOG_ERR("%s: Failed to read guest code block count\n",__func__);
				return retval;
			}
			batohs(&fwu->blkcount.lcm_data, &(buf[0]));
			count++;
			fwu->has_lcm_data = true;
		}
		if (properties_2.has_oem_data){
			retval = fwu->fn_ptr->read(rmi4_data,
					base + fwu->off.properties_2 + 1 + count,
					buf,
					2);
			if (retval < 0) {
				TS_LOG_ERR("%s: Failed to read guest code block count\n", __func__);
				return retval;
			}
			batohs(&fwu->blkcount.oem_data, &(buf[0]));
			fwu->has_oem_data = true;
		}
	}

	if (rmi4_data->synaptics_chip_data->bootloader_update_enable) {
		fwu->has_utility_param = false;
	}

	return 0;
}

static int fwu_read_f34_queries(void)
{
	int retval;

	memset(&fwu->blkcount, 0x00, sizeof(fwu->blkcount));
	memset(&fwu->phyaddr, 0x00, sizeof(fwu->phyaddr));

	if (fwu->bl_version == BL_V7)
		retval = fwu_read_f34_v7_queries();
	else
		retval = fwu_read_f34_v5v6_queries();

	return retval;
}

static int fwu_write_f34_v7_blocks(unsigned char *block_ptr,
				   unsigned short block_cnt,
				   unsigned char command)
{
	int retval;
	unsigned char base;
	unsigned char length[2] = {0};
	unsigned short transfer;
	unsigned short max_transfer;
	unsigned short remaining = block_cnt;
	unsigned short block_number = 0;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	retval = fwu_write_f34_partition_id(command);
	if (retval < 0)
		return retval;

	retval = fwu->fn_ptr->write(rmi4_data,
				    base + fwu->off.block_number,
				    (unsigned char *)&block_number,
				    sizeof(block_number));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read flash status\n");
		return retval;
	}

	if (fwu->payload_length > PAYLOAD_LENGTH_MAX)
		max_transfer = PAYLOAD_LENGTH_MAX;
	else
		max_transfer = fwu->payload_length;

	do {
		if (rmi4_data->synaptics_chip_data->bootloader_update_enable
		    && command == CMD_WRITE_BOOTLOADER) {
			if (remaining / fwu->payload_length)
				transfer = fwu->payload_length;
			else
				transfer = remaining;
		} else {
			if (remaining / max_transfer)
				transfer = max_transfer;
			else
				transfer = remaining;
		}

		length[0] = (unsigned char)(transfer & MASK_8BIT);
		length[1] = (unsigned char)(transfer >> 8);

		retval =
		    fwu->fn_ptr->write(rmi4_data,
				       base + fwu->off.transfer_length, length,
				       sizeof(length));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to write transfer length (%d blocks remaining)\n",__func__, remaining);
			return retval;
		}

		retval = fwu_write_f34_command(command);
		if (retval < 0) {
			TS_LOG_ERR
			    ("%s: Failed to write command (%d blocks remaining)\n",__func__, remaining);
			return retval;
		}

		retval =
		    fwu->fn_ptr->write(rmi4_data, base + fwu->off.payload,
				       block_ptr, transfer * fwu->block_size);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to write block data (%d blocks remaining)\n",__func__, remaining);
			return retval;
		}

		if (rmi4_data->synaptics_chip_data->bootloader_update_enable
		    && command == CMD_WRITE_BOOTLOADER) {
			msleep(WRITE_BL_WAIT_MS);
		} else {
			retval = fwu_wait_for_idle(WRITE_WAIT_MS, true);
			if (retval < 0) {
				TS_LOG_ERR("%s: Failed to wait for idle status (%d blocks remaining)\n",__func__, remaining);
				return retval;
			}
		}

		block_ptr += (transfer * fwu->block_size);
		remaining -= transfer;
	} while (remaining);

	return 0;
}

static int fwu_write_f34_v5v6_blocks(unsigned char *block_ptr,
				     unsigned short block_cnt,
				     unsigned char command)
{
	int retval;
	unsigned char base;
	unsigned char block_number[] = { 0, 0 };
	unsigned short blk;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	block_number[1] |= (fwu->config_area << 5);

	retval = fwu->fn_ptr->write(rmi4_data,
				    base + fwu->off.block_number,
				    block_number, sizeof(block_number));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write block number\n", __func__);
		return retval;
	}

	for (blk = 0; blk < block_cnt; blk++) {
		retval =
		    fwu->fn_ptr->write(rmi4_data, base + fwu->off.payload,
				       block_ptr, fwu->block_size);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to write block data (block %d)\n",__func__, blk);
			return retval;
		}

		retval = fwu_write_f34_command(command);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to write command for block %d\n",__func__, blk);
			return retval;
		}

		retval = fwu_wait_for_idle(WRITE_WAIT_MS, true);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to wait for idle status (block %d)\n",__func__, blk);
			return retval;
		}

		block_ptr += fwu->block_size;
	}

	return 0;
}

static int fwu_write_f34_blocks(unsigned char *block_ptr,
				unsigned short block_cnt, unsigned char cmd)
{
	int retval;

	if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8)
		retval = fwu_write_f34_v7_blocks(block_ptr, block_cnt, cmd);
	else
		retval = fwu_write_f34_v5v6_blocks(block_ptr, block_cnt, cmd);

	return retval;
}

static int fwu_read_f34_v7_blocks(unsigned short block_cnt,
				  unsigned char command)
{
	int retval;
	unsigned char base;
	unsigned char length[2];
	unsigned short transfer;
	unsigned short max_transfer;
	unsigned short remaining = block_cnt;
	unsigned short block_number = 0;
	unsigned short index = 0;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	retval = fwu_write_f34_partition_id(command);
	if (retval < 0)
		return retval;

	retval = fwu->fn_ptr->write(rmi4_data,
				    base + fwu->off.block_number,
				    (unsigned char *)&block_number,
				    sizeof(block_number));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write block number\n", __func__);
		return retval;
	}

	if (fwu->payload_length > (PAGE_SIZE / fwu->block_size))
		max_transfer = PAGE_SIZE / fwu->block_size;
	else
		max_transfer = fwu->payload_length;

	do {
		if (remaining / max_transfer)
			transfer = max_transfer;
		else
			transfer = remaining;

		length[0] = (unsigned char)(transfer & MASK_8BIT);
		length[1] = (unsigned char)(transfer >> 8);

		retval =
		    fwu->fn_ptr->write(rmi4_data,
				       base + fwu->off.transfer_length, length,
				       sizeof(length));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to write transfer length (%d blocks remaining)\n",__func__, remaining);
			return retval;
		}

		retval = fwu_write_f34_command(command);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to write command (%d blocks remaining)\n", __func__, remaining);
			return retval;
		}

		retval = fwu_wait_for_idle(WRITE_WAIT_MS, true);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to wait for idle status (%d blocks remaining)\n",__func__, remaining);
			return retval;
		}

		retval = fwu->fn_ptr->read(rmi4_data, base + fwu->off.payload,
				      &fwu->read_config_buf[index],
				      transfer * fwu->block_size);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read block data (%d blocks remaining)\n", __func__, remaining);
			return retval;
		}

		index += (transfer * fwu->block_size);
		remaining -= transfer;
	} while (remaining);

	return 0;
}

static int fwu_read_f34_v5v6_blocks(unsigned short block_cnt,
				    unsigned char command)
{
	int retval;
	unsigned char base;
	unsigned char block_number[] = { 0, 0 };
	unsigned short blk;
	unsigned short index = 0;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	block_number[1] |= (fwu->config_area << 5);

	retval = fwu->fn_ptr->write(rmi4_data,
				    base + fwu->off.block_number,
				    block_number, sizeof(block_number));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write block number\n", __func__);
		return retval;
	}

	retval = fwu_allocate_read_config_buf(6000);
	if (retval) {
		TS_LOG_ERR("%s: allocate_read_config_buf fail\n", __func__);
		return 0;
	}

	for (blk = 0; blk < block_cnt; blk++) {
		retval = fwu_write_f34_command(command);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to write read config command\n",__func__);
			return retval;
		}

		retval = fwu_wait_for_idle(WRITE_WAIT_MS, true);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to wait for idle status\n",__func__);
			return retval;
		}

		retval =
		    fwu->fn_ptr->read(rmi4_data, base + fwu->off.payload,
				      &fwu->read_config_buf[index],
				      fwu->block_size);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read block data (block %d)\n", __func__, blk);
			return retval;
		}

		index += fwu->block_size;
	}

	return 0;
}

static int fwu_read_f34_blocks(unsigned short block_cnt, unsigned char cmd)
{
	int retval;

	if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8)
		retval = fwu_read_f34_v7_blocks(block_cnt, cmd);
	else
		retval = fwu_read_f34_v5v6_blocks(block_cnt, cmd);

	return retval;
}

static int fwu_get_image_firmware_id(unsigned int *fw_id)
{
	int retval;
	unsigned char index = 0;
	char *strptr;
	char *firmware_id;

	if (fwu->img.contains_firmware_id) {
		*fw_id = fwu->img.firmware_id;
	} else {
		if (!fwu->image_name) {
			TS_LOG_ERR("fwu->image_name is NULL, return\n");
			return -EINVAL;
		}
		strptr = strnstr(fwu->image_name, "PR", MAX_IMAGE_NAME_LEN);
		if (!strptr) {
			TS_LOG_ERR("%s: No valid PR number (PRxxxxxxx) "
				   "found in image file name (%s)\n", __func__,
				   fwu->image_name);
			return -EINVAL;
		}

		strptr += 2;
		firmware_id = kzalloc(MAX_FIRMWARE_ID_LEN, GFP_KERNEL);
		if (!firmware_id) {
			TS_LOG_ERR("%s: Failed to alloc mem for firmware_id\n",__func__);
			return -ENOMEM;
		}
		while (strptr[index] >= '0' && strptr[index] <= '9') {
			firmware_id[index] = strptr[index];
			index++;
		}

		retval = sstrtoul(firmware_id, 10, (unsigned long *)fw_id);
		kfree(firmware_id);
		if (retval) {
			TS_LOG_ERR("%s: Failed to obtain image firmware ID\n",__func__);
			return -EINVAL;
		}
	}

	return 0;
}

bool GetHexCharValue(char cHex, unsigned char * byValue)
{
	int i = 0;
	
	if(NULL == byValue)
	{
		return false;
	}
	struct HexValue stHexValue[] = {
		{'0', 0}, {'1', 1}, {'2', 2}, {'3', 3}, {'4', 4},{'5', 5}, {'6', 6}, {'7', 7}, {'8', 8}, {'9', 9},
		{'a', 10}, {'b', 11}, {'c', 12}, {'d', 13}, {'e', 14}, {'f', 15}, 
		{'A', 10}, {'B', 11}, {'C', 12}, {'D', 13}, {'E', 14}, {'F', 15}};

	for ( i = 0; i < (int)(sizeof(stHexValue) / sizeof(struct HexValue)); i++)
	{
		if (stHexValue[i].m_cHexChar == cHex)
		{
			*byValue = stHexValue[i].m_byValue;
			return true;
		}
	}
	return false;
}

static enum flash_area fwu_go_nogo(void)
{
	int retval = 0;
	enum flash_area flash_area = NONE;
	unsigned char config_id[4] = {0};
	unsigned char last_3_num[3] = {0};
	unsigned int device_config_id =0;
	unsigned int image_config_id =0;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;
	unsigned char *tmp = NULL;

	/* syna_wx_wy */
	unsigned char dts_set_buf[3] = {0};
	int ii = 0;
	/* syna_wx_wy end */

	if (fwu->force_update) {
		flash_area = UI_FIRMWARE;
		goto exit;
	}

	/* Update both UI and config if device is in bootloader mode */
	if (fwu->in_bl_mode) {
		flash_area = UI_FIRMWARE;
		goto exit;
	}

	/* Get device config ID */
	retval =
	    fwu->fn_ptr->read(rmi4_data, fwu->f34_fd.ctrl_base_addr, config_id,
			      sizeof(config_id));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read device config ID\n", __func__);
		flash_area = NONE;
		goto exit;
	}
	device_config_id = be_to_uint(config_id);
	TS_LOG_INFO("Device config ID = 0x%02x 0x%02x 0x%02x 0x%02x\n",
		    config_id[0], config_id[1], config_id[2], config_id[3]);
	if (rmi4_data->synaptics_chip_data->ts_platform_data->chip_data->is_multi_protocal){
		last_3_num[0] = 0x0f & config_id[2];
		last_3_num[1] = 0x0f & (config_id[3] >> 4);
		last_3_num[2] = 0x0f & config_id[3];
		TS_LOG_INFO("last_3_num[0] = 0x%02x, last_3_num[1] = 0x%02x, last_3_num[2] = 0x%02x\n",
			last_3_num[0], last_3_num[1], last_3_num[2]);
		/* syna_wx_wy */
		rmi4_data->new_wx_wy = true;
		tmp = rmi4_data->synaptics_chip_data->adv_width;
		for (ii = 0; ii < 3; ii++) {
			if(!GetHexCharValue(tmp[ii], &(dts_set_buf[ii])))
				break;
			TS_LOG_INFO("temp_buf[%d] = 0x%02x, last_3_num[%d] = 0x%02x\n",
				ii, dts_set_buf[ii], ii, last_3_num[ii]);

			if (dts_set_buf[ii] > last_3_num[ii]) {
				TS_LOG_INFO("set old protocol\n");
				rmi4_data->new_wx_wy = false;
				break;
			}
			else if (dts_set_buf[ii] < last_3_num[ii]){
				break;
			}
		}
		/* syna_wx_wy end */
	}
	TS_LOG_INFO("new_wx_wy = %d\n", rmi4_data->new_wx_wy);

	/* Get image config ID */
	image_config_id = be_to_uint(fwu->img.ui_config.data);
	TS_LOG_INFO("Image config ID = 0x%02x 0x%02x 0x%02x 0x%02x\n",
		    fwu->img.ui_config.data[0],
		    fwu->img.ui_config.data[1],
		    fwu->img.ui_config.data[2], fwu->img.ui_config.data[3]);

	snprintf(fwu->rmi4_data->rmi4_mod_info.device_config_id,
		 sizeof(unsigned char) * CHIP_INFO_LENGTH,
		 "%02x %02x %02x %02x ", config_id[0], config_id[1],
		 config_id[2], config_id[3]);
	snprintf(fwu->rmi4_data->rmi4_mod_info.image_config_id,
		 sizeof(unsigned char) * CHIP_INFO_LENGTH,
		 "%02x %02x %02x %02x ", fwu->img.ui_config.data[0],
		 fwu->img.ui_config.data[1], fwu->img.ui_config.data[2],
		 fwu->img.ui_config.data[3]);

	TS_LOG_INFO("Device config ID %d, .img config ID %d\n",
		    device_config_id, image_config_id);

	if (image_config_id != device_config_id) {
		if (rmi4_data->synaptics_chip_data->byte_to_byte_cmp_config_id) {
			if (!((config_id[0] == fwu->img.ui_config.data[0]) &&
				(config_id[1] == fwu->img.ui_config.data[1]) &&
				(config_id[2] == fwu->img.ui_config.data[2]) &&
				(config_id[3] != fwu->img.ui_config.data[3]) &&
                              (fwu->img.ui_config.data[3] != 0))) {
				TS_LOG_ERR("byte to byte cmp failed, config id inavailable\n");
				flash_area = NONE;
				goto exit;
			}
		}
		TS_LOG_INFO("after byte to byte cmp: img configid is different from device configid\n");

		if (FW_UPDATE_TO_HIGH ==
		    fwu->rmi4_data->synaptics_chip_data->fw_update_logic) {
			if (image_config_id < device_config_id) {
				TS_LOG_INFO
				    ("imageConfigID[%d] is little than deviceConfigID[%d], then no need update\n",
				     image_config_id, device_config_id);
				flash_area = NONE;
				goto exit;
			}
		}

		flash_area = UI_FIRMWARE;
		goto exit;
	}

	flash_area = NONE;

exit:
	if (flash_area == NONE)
		TS_LOG_INFO("Nothing needs to be updated\n");
	else
		TS_LOG_INFO("Update %s block\n",
			    flash_area == UI_FIRMWARE ? "UI FW" : "CONFIG");
	return flash_area;
}

static int fwu_scan_pdt(void)
{
	int retval;
	unsigned char ii;
	unsigned char intr_count = 0;
	unsigned char intr_off;
	unsigned char intr_src;
	unsigned short addr;
	bool f01found = false;
	bool f34found = false;
	bool f35found = false;
	struct synaptics_rmi4_fn_desc rmi_fd;
	fwu->in_ub_mode = false;

	TS_LOG_INFO("fwu_scan_pdt called\n");

	for (addr = PDT_START; addr > PDT_END; addr -= PDT_ENTRY_SIZE) {
		retval =
		    fwu->fn_ptr->read(fwu->rmi4_data, addr,
				      (unsigned char *)&rmi_fd, sizeof(rmi_fd));
		if (retval < 0)
			return retval;

		if (rmi_fd.fn_number) {
			TS_LOG_INFO("Found F%02x\n", rmi_fd.fn_number);
			switch (rmi_fd.fn_number) {
			case SYNAPTICS_RMI4_F01:
				f01found = true;
				fwu->f01_fd.query_base_addr =
				    rmi_fd.query_base_addr;
				fwu->f01_fd.ctrl_base_addr =
				    rmi_fd.ctrl_base_addr;
				fwu->f01_fd.data_base_addr =
				    rmi_fd.data_base_addr;
				fwu->f01_fd.cmd_base_addr =
				    rmi_fd.cmd_base_addr;
				break;
			case SYNAPTICS_RMI4_F34:
				f34found = true;
				fwu->f34_fd.query_base_addr =
				    rmi_fd.query_base_addr;
				fwu->f34_fd.ctrl_base_addr =
				    rmi_fd.ctrl_base_addr;
				fwu->f34_fd.data_base_addr =
				    rmi_fd.data_base_addr;
				switch (rmi_fd.fn_version) {
				case F34_V0:
					fwu->bl_version = BL_V5;
					break;
				case F34_V1:
					fwu->bl_version = BL_V6;
					break;
				case F34_V2:
					fwu->bl_version = BL_V7;
					TS_LOG_ERR("%s: bootloader version is 7\n",__func__);
					break;
				default:
					TS_LOG_ERR("%s: Unrecognized F34 version\n",__func__);
					return -EINVAL;
				}
				fwu->intr_mask = 0;
				intr_src = rmi_fd.intr_src_count;
				intr_off = intr_count % 8;
				for (ii = intr_off; ii < (intr_src + intr_off);
				     ii++) {
					fwu->intr_mask |= 1 << ii;
				}
				break;
			case SYNAPTICS_RMI4_F35:
				f35found = true;
				fwu->f35_fd.query_base_addr =
				    rmi_fd.query_base_addr;
				fwu->f35_fd.ctrl_base_addr =
				    rmi_fd.ctrl_base_addr;
				fwu->f35_fd.data_base_addr =
				    rmi_fd.data_base_addr;
				break;
			default:
				TS_LOG_ERR("%s: is   default\n",__func__);
			}
		} else {
			break;
		}
	}

	if (!f01found || !f34found) {
		TS_LOG_ERR("%s: Failed to find both F01 and F34\n", __func__);
		if (!f35found) {
			TS_LOG_ERR("%s: Failed to find F35\n", __func__);
			return -EINVAL;
		} else {
			fwu->in_ub_mode = true;
			TS_LOG_DEBUG("%s: In microbootloader mode\n", __func__);
			fwu_recovery_check_status();
			return 0;
		}
	}

	return 0;
}

static int fwu_enter_flash_prog(void)
{
	int retval;
	struct f01_device_control f01_device_control;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	retval = fwu_read_flash_status();
	if (retval < 0)
		return retval;

	if (fwu->in_bl_mode)
		return 0;

	/*retval = rmi4_data->irq_enable(rmi4_data, false, true);*/
	if (retval < 0)
		return retval;

	msleep(INT_DISABLE_WAIT_MS);

	retval = fwu_write_f34_command(CMD_ENABLE_FLASH_PROG);
	if (retval < 0)
		return retval;
	if (rmi4_data->synaptics_chip_data->delay_for_fw_update)
		msleep(500);
	retval = fwu_wait_for_idle(ENABLE_WAIT_MS, true);
	if (retval < 0)
		return retval;

	if (!fwu->in_bl_mode) {
		TS_LOG_ERR("%s: BL mode not entered\n", __func__);
		return -EINVAL;
	}

	retval = fwu_scan_pdt();
	if (retval < 0)
		return retval;

	if(fwu->rmi4_data->synaptics_chip_data->ic_type == SYNAPTICS_S3706){
		retval = fwu_read_f34_queries(); 
		if (retval < 0)
			return retval;
	}

	retval = fwu->fn_ptr->read(rmi4_data,
				   fwu->f01_fd.ctrl_base_addr,
				   f01_device_control.data,
				   sizeof(f01_device_control.data));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read F01 device control\n", __func__);
		return retval;
	}

	f01_device_control.nosleep = true;
	f01_device_control.sleep_mode = SLEEP_MODE_NORMAL;

	retval = fwu->fn_ptr->write(rmi4_data,
				    fwu->f01_fd.ctrl_base_addr,
				    f01_device_control.data,
				    sizeof(f01_device_control.data));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write F01 device control\n",__func__);
		return retval;
	}

	msleep(ENTER_FLASH_PROG_WAIT_MS);

	return retval;
}

static int fwu_check_ui_firmware_size(void)
{
	unsigned short block_count;

	block_count = fwu->img.ui_firmware.size / fwu->block_size;

	if (block_count != fwu->blkcount.ui_firmware) {
		TS_LOG_ERR("%s: UI firmware size mismatch\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_check_ui_configuration_size(void)
{
	unsigned short block_count;

	block_count = fwu->img.ui_config.size / fwu->block_size;

	if (block_count != fwu->blkcount.ui_config) {
		TS_LOG_ERR("%s: UI configuration size mismatch\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_check_dp_configuration_size(void)
{
	unsigned short block_count;

	block_count = fwu->img.dp_config.size / fwu->block_size;

	if (block_count != fwu->blkcount.dp_config) {
		TS_LOG_ERR("%s: Display configuration size mismatch\n",__func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_check_bl_configuration_size(void)
{
	unsigned short block_count;

	block_count = fwu->img.bl_config.size / fwu->block_size;

	if (block_count != fwu->blkcount.bl_config) {
		TS_LOG_ERR("%s: Bootloader configuration size mismatch\n",__func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_check_guest_code_size(void)
{
	unsigned short block_count;

	block_count = fwu->img.guest_code.size / fwu->block_size;
	if (block_count != fwu->blkcount.guest_code) {
		TS_LOG_ERR("%s: Guest code size mismatch\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_write_firmware(void)
{
	unsigned short firmware_block_count;

	firmware_block_count = fwu->img.ui_firmware.size / fwu->block_size;

	return fwu_write_f34_blocks((unsigned char *)fwu->img.ui_firmware.data,
				    firmware_block_count, CMD_WRITE_FW);
}

static int fwu_erase_configuration(void)
{
	int retval;

	switch (fwu->config_area) {
	case UI_CONFIG_AREA:
		retval = fwu_write_f34_command(CMD_ERASE_UI_CONFIG);
		if (retval < 0)
			return retval;
		break;
	case DP_CONFIG_AREA:
		retval = fwu_write_f34_command(CMD_ERASE_DISP_CONFIG);
		if (retval < 0)
			return retval;
		break;
	case BL_CONFIG_AREA:
		retval = fwu_write_f34_command(CMD_ERASE_BL_CONFIG);
		if (retval < 0)
			return retval;
		break;
	case FLASH_CONFIG_AREA:
		retval = fwu_write_f34_command(CMD_ERASE_FLASH_CONFIG);
		if (retval < 0)
			return retval;
		break;
	}

	retval = fwu_wait_for_idle(ERASE_WAIT_MS, true);
	if (retval < 0)
		return retval;
	return retval;
}

static int fwu_erase_bootloader(void)
{
	int retval;

	retval = fwu_write_f34_command(CMD_ERASE_BOOTLOADER);
	if (retval < 0)
		return retval;
	retval = fwu_wait_for_idle(ERASE_WAIT_MS, false);
	if (retval < 0)
		return retval;
	return 0;
}

static int fwu_erase_utility_parameter(void)
{
	int retval;

	retval = fwu_write_f34_command(CMD_ERASE_UTILITY_PARAMETER);
	if (retval < 0)
		return retval;
	retval = fwu_wait_for_idle(ERASE_WAIT_MS, false);
	if (retval < 0)
		return retval;
	return 0;
}

static int fwu_erase_guest_code(void)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	retval = fwu_write_f34_command(CMD_ERASE_GUEST_CODE);
	if (retval < 0)
		return retval;
	retval = fwu_wait_for_idle(ERASE_WAIT_MS, true);
	if (retval < 0)
		return retval;
	return 0;
}

static int fwu_erase_lockdown_data(void)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	retval = fwu_write_f34_command(CMD_ERASE_LOCKDOWN_DATA);
	if (retval < 0)
		return retval;
	msleep(100);
	retval = fwu_wait_for_idle(ERASE_WAIT_MS, true);
	if (retval < 0)
		return retval;
	return 0;
}


static int fwu_erase_all(void)
{
	int retval;

	if (fwu->bl_version == BL_V7) {
		retval = fwu_write_f34_command(CMD_ERASE_UI_FIRMWARE);
		if (retval < 0)
			return retval;

		TS_LOG_DEBUG("%s: Erase command written\n", __func__);

		msleep(1000);
		retval = fwu_wait_for_idle(ERASE_WAIT_MS, true);
		if (retval < 0)
			return retval;

		TS_LOG_DEBUG("%s: Idle status detected\n", __func__);

		fwu->config_area = UI_CONFIG_AREA;
		retval = fwu_erase_configuration();
		if (rmi4_data->synaptics_chip_data->delay_for_fw_update)
			msleep(500);
		if (retval < 0)
			return retval;
	} else {
		retval = fwu_write_f34_command(CMD_ERASE_ALL);
		if (retval < 0)
			return retval;

		TS_LOG_DEBUG("%s: Erase all command written\n", __func__);
		if (rmi4_data->synaptics_chip_data->delay_for_fw_update)
			msleep(rmi4_data->synaptics_chip_data->delay_for_erase_fw);
		if(fwu->rmi4_data->synaptics_chip_data->ic_type == SYNAPTICS_S3706){
			retval = fwu_wait_for_idle(ERASE_WAIT_MS, false);
			if (retval < 0)
				return retval;
		}
		else{
			retval = fwu_wait_for_idle(ERASE_WAIT_MS, true);
			if (retval < 0)
				return retval;
		}

		TS_LOG_DEBUG("%s: Idle status detected\n", __func__);
	}

	if (fwu->flash_properties.has_disp_config) {
		fwu->config_area = DP_CONFIG_AREA;
		retval = fwu_erase_configuration();
		if (retval < 0)
			return retval;
	}

	if ((fwu->new_partition_table && fwu->has_guest_code)||
		(rmi4_data->synaptics_chip_data->synaptics3718_fw_updata_flag && fwu->has_guest_code)) {
		retval = fwu_erase_guest_code();
		if (retval < 0)
			return retval;
	}

	return 0;
}

static int fwu_write_bootloader(void)
{
	int retval;
	unsigned short bootloader_block_count;

	bootloader_block_count = fwu->img.bl_image.size / fwu->block_size;

	fwu->write_bootloader = true;
	retval = fwu_write_f34_blocks((unsigned char *)fwu->img.bl_image.data,
				      bootloader_block_count,
				      CMD_WRITE_BOOTLOADER);
	fwu->write_bootloader = false;

	return retval;
}
#ifdef SYNA_UPP
static int fwu_read_utility_parameter(void)
{
	int retval;
	//unsigned char checksum_array[4];
	unsigned short utility_param_size;
	//unsigned long checksum;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;
	int i;

	utility_param_size = fwu->blkcount.utility_param * fwu->block_size;
	retval = fwu_allocate_read_config_buf(utility_param_size);
	if (retval < 0)
		return retval;
	memset(fwu->read_config_buf, 0x00, utility_param_size);

	fwu->read_config_buf_size = utility_param_size;
	TS_LOG_INFO("utility_param_size = %d\n", utility_param_size);
	retval = fwu_read_f34_blocks(fwu->blkcount.utility_param, CMD_READ_UTILITY_PARAM);
	if (retval < 0)
		return retval;
	return 0;
}
#endif

static int fwu_write_utility_parameter(void)
{
	int retval;
	unsigned char checksum_array[4];
	unsigned short utility_param_size;
	unsigned long checksum;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	utility_param_size = fwu->blkcount.utility_param * fwu->block_size;
	retval = fwu_allocate_read_config_buf(utility_param_size);
	if (retval < 0)
		return retval;
	memset(fwu->read_config_buf, 0x00, utility_param_size);

	if (fwu->img.force_param.size) {
		retval = secure_memcpy(fwu->read_config_buf,
				       utility_param_size - 4,
				       fwu->img.force_param.data,
				       fwu->img.force_param.size,
				       fwu->img.force_param.size);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to copy force parameter data\n",__func__);
			return retval;
		}
	}

	calculate_checksum((unsigned short *)fwu->read_config_buf,
			   ((utility_param_size - 4) / 2), &checksum);

	convert_to_little_endian(checksum_array, checksum);

	fwu->read_config_buf[utility_param_size - 4] = checksum_array[0];
	fwu->read_config_buf[utility_param_size - 3] = checksum_array[1];
	fwu->read_config_buf[utility_param_size - 2] = checksum_array[2];
	fwu->read_config_buf[utility_param_size - 1] = checksum_array[3];

	retval = fwu_write_f34_blocks((unsigned char *)fwu->read_config_buf,
				      fwu->blkcount.utility_param,
				      CMD_WRITE_UTILITY_PARAM);
	if (retval < 0)
		return retval;

	return 0;
}

static int fwu_write_configuration(void)
{
	return fwu_write_f34_blocks((unsigned char *)fwu->config_data,
				    fwu->config_block_count, CMD_WRITE_CONFIG);
}

static int fwu_write_ui_configuration(void)
{
	fwu->config_area = UI_CONFIG_AREA;
	fwu->config_data = fwu->img.ui_config.data;
	fwu->config_size = fwu->img.ui_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	return fwu_write_configuration();
}

static int fwu_write_dp_configuration(void)
{
	fwu->config_area = DP_CONFIG_AREA;
	fwu->config_data = fwu->img.dp_config.data;
	fwu->config_size = fwu->img.dp_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	return fwu_write_configuration();
}

static int fwu_write_lockdown_data(void)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	retval = fwu_write_f34_blocks(fwu->read_config_buf,
			fwu->blkcount.lockdown_data, CMD_WRITE_LOCKDOWN_DATA);
	if (retval < 0)
		return retval;
	rmi4_data->reset_device(rmi4_data);
	return 0;
}

static int fwu_write_flash_configuration(void)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	fwu->config_area = FLASH_CONFIG_AREA;
	fwu->config_data = fwu->img.fl_config.data;
	fwu->config_size = fwu->img.fl_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	if (fwu->config_block_count != fwu->blkcount.fl_config) {
		TS_LOG_ERR("%s: Flash configuration size mismatch\n", __func__);
		return -EINVAL;
	}

	retval = fwu_write_f34_command(CMD_ERASE_FLASH_CONFIG);
	if (retval < 0)
		return retval;
	retval = fwu_wait_for_idle(ERASE_WAIT_MS, true);
	if (retval < 0)
		return retval;
	retval = fwu_write_configuration();
	if (retval < 0)
		return retval;

	rmi4_data->reset_device(rmi4_data);

	return 0;
}

static int fwu_write_guest_code(void)
{
	int retval;
	unsigned short guest_code_block_count;

	guest_code_block_count = fwu->img.guest_code.size / fwu->block_size;

	retval = fwu_write_f34_blocks((unsigned char *)fwu->img.guest_code.data,
				      guest_code_block_count,
				      CMD_WRITE_GUEST_CODE);
	if (retval < 0)
		return retval;

	return 0;
}

static int fwu_write_lockdown(void)
{
	unsigned short lockdown_block_count;

	lockdown_block_count = fwu->img.lockdown.size / fwu->block_size;

	return fwu_write_f34_blocks((unsigned char *)fwu->img.lockdown.data,
				    lockdown_block_count, CMD_WRITE_LOCKDOWN);
}

static int fwu_write_partition_table(void)
{
	int retval;
	unsigned short block_count;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	block_count = fwu->blkcount.bl_config;
	fwu->config_area = BL_CONFIG_AREA;
	fwu->config_size = fwu->block_size * block_count;
	kfree(fwu->read_config_buf);
	fwu->read_config_buf = kzalloc(fwu->config_size, GFP_KERNEL);
	if (!fwu->read_config_buf) {
		TS_LOG_ERR("%s: Failed to alloc mem for fwu->read_config_buf\n",__func__);
		fwu->read_config_buf_size = 0;
		return -ENOMEM;
	}
	fwu->read_config_buf_size = fwu->config_size;

	retval = fwu_read_f34_blocks(block_count, CMD_READ_CONFIG);
	if (retval < 0)
		return retval;

	retval = fwu_erase_configuration();
	if (retval < 0)
		return retval;

	retval = fwu_write_flash_configuration();
	if (retval < 0)
		return retval;

	fwu->config_area = BL_CONFIG_AREA;
	fwu->config_data = fwu->read_config_buf;
	fwu->config_size = fwu->img.bl_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	retval = fwu_write_configuration();
	if (retval < 0)
		return retval;

	return 0;
}

static int fwu_write_partition_table_v8(void)
{
	int retval = 0;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	fwu->config_area = FLASH_CONFIG_AREA;
	fwu->config_data = fwu->img.fl_config.data;
	fwu->config_size = fwu->img.fl_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	if (fwu->config_block_count != fwu->blkcount.fl_config) {
		TS_LOG_ERR("%s: Flash configuration size mismatch\n",__func__);
		return -EINVAL;
	}

	retval = fwu_write_configuration();
	if (retval < 0)
		return retval;

	rmi4_data->reset_device(rmi4_data);

	return 0;
}

static int fwu_write_partition_table_v7(void)
{
	int retval = 0;
	unsigned short block_count = 0;

	block_count = fwu->blkcount.bl_config;
	fwu->config_area = BL_CONFIG_AREA;
	fwu->config_size = fwu->block_size * block_count;

	retval = fwu_allocate_read_config_buf(fwu->config_size);
	if (retval < 0)
		return retval;

	retval = fwu_read_f34_blocks(block_count, CMD_READ_CONFIG);
	if (retval < 0)
		return retval;

	retval = fwu_erase_configuration();
	if (retval < 0)
		return retval;

	retval = fwu_write_flash_configuration();
	if (retval < 0)
		return retval;

	fwu->config_area = BL_CONFIG_AREA;
	fwu->config_data = fwu->read_config_buf;
	fwu->config_size = fwu->img.bl_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	retval = fwu_write_configuration();
	if (retval < 0)
		return retval;

	return 0;
}
static int fwu_write_bl_area_v7(void)
{
	int retval;
	bool has_utility_param;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	has_utility_param = fwu->has_utility_param;
	if (fwu->has_utility_param) {
		retval = fwu_erase_utility_parameter();
		if (retval < 0)
			return retval;
	}

	fwu->config_area = BL_CONFIG_AREA;
	retval = fwu_erase_configuration();
	if (retval < 0)
		return retval;

	fwu->config_area = FLASH_CONFIG_AREA;
	retval = fwu_erase_configuration();
	if (retval < 0)
		return retval;

	retval = fwu_erase_bootloader();
	if (retval < 0)
		return retval;

	retval = fwu_write_bootloader();
	if (retval < 0)
		return retval;
	TS_LOG_INFO("%s: success to write bootloader\n", __func__);
	msleep(F35_RESET_WAIT_MS);

	if(fwu->rmi4_data->synaptics_chip_data->ic_type != SYNAPTICS_S3706)
		fwu_scan_pdt();

	rmi4_data->reset_device(rmi4_data);
	retval = fwu_read_f34_queries();
	if (retval < 0)
		return retval;

	fwu->config_area = FLASH_CONFIG_AREA;
	fwu->config_data = fwu->img.fl_config.data;
	fwu->config_size = fwu->img.fl_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;
	retval = fwu_write_configuration();
	if (retval < 0)
		return retval;
	TS_LOG_INFO("%s: success to write flash config\n", __func__);
	rmi4_data->reset_device(rmi4_data);
	retval = fwu_read_f34_queries();
	if (retval < 0)
		return retval;

	fwu->config_area = BL_CONFIG_AREA;
	fwu->config_data = fwu->img.bl_config.data;
	fwu->config_size = fwu->img.bl_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;
	retval = fwu_write_configuration();
	if (retval < 0)
		return retval;
	if (fwu->img.contains_utility_param && !has_utility_param) {
		retval = fwu_write_utility_parameter();
		if (retval < 0)
			return retval;
		TS_LOG_INFO("%s: success to write utility param\n", __func__);
	}
	return 0;
}

static int fwu_do_reflash(void)
{
	int retval;
	bool do_bl_update = false;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	if (!fwu->new_partition_table) {
		retval = fwu_check_ui_firmware_size();
		if (retval < 0)
			return retval;

		retval = fwu_check_ui_configuration_size();
		if (retval < 0)
			return retval;

		if (fwu->flash_properties.has_disp_config
		    && fwu->img.contains_disp_config) {
			retval = fwu_check_dp_configuration_size();
			if (retval < 0)
				return retval;
		}

		if (fwu->has_guest_code && fwu->img.contains_guest_code) {
			retval = fwu_check_guest_code_size();
			if (retval < 0)
				return retval;
		}
	}
	else if (fwu->bl_version != BL_V8){ //Actually,this function only use for BL_V7
		retval = fwu_check_bl_configuration_size();
		if (retval < 0)
			return retval;
	}
	else{
		TS_LOG_ERR("%s: is default\n", __func__);
	}

	retval = fwu_erase_all();
	if (retval < 0)
		return retval;

	if (rmi4_data->synaptics_chip_data->bootloader_update_enable) {
		if (!fwu->has_utility_param && fwu->img.contains_utility_param) {
			if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8) 
				do_bl_update = true;
		}

		if (fwu->has_utility_param && !fwu->img.contains_utility_param) {
			if (fwu->bl_version == BL_V7 || fwu->bl_version ) 
				do_bl_update = true;
		}
	}
	if( fwu->rmi4_data->synaptics_chip_data->ic_type == SYNAPTICS_S3706){
		if (!do_bl_update && fwu->incompatible_partition_tables) {
			TS_LOG_ERR("%s: Incompatible partition tables\n", __func__);
			return -EINVAL;
		} else if (!do_bl_update && fwu->new_partition_table) {
			if (!fwu->force_update) {
				TS_LOG_ERR("%s: Partition table mismatch\n", __func__);
				return -EINVAL;
			}
		}
	}

	if (do_bl_update) {
		retval = fwu_write_bl_area_v7();
		if (retval < 0)
			return retval;
		TS_LOG_INFO("%s: Bootloader area programmed\n", __func__);
	} else if ((fwu->bl_version != BL_V8) && (fwu->new_partition_table)) { 
		retval = fwu_write_partition_table();
		if (retval < 0)
			return retval;
		TS_LOG_INFO("%s: Partition table programmed\n", __func__);
	} else if (fwu->bl_version == BL_V8) {
		retval = fwu_write_partition_table_v8();
		if (retval < 0)
			return retval;
		TS_LOG_INFO("%s: Partition table programmed\n", __func__);
	}
	else{
		TS_LOG_ERR("%s: is default\n", __func__);
	}

	if (fwu->flash_properties.has_disp_config &&
	    fwu->img.contains_disp_config) {
		retval = fwu_write_dp_configuration();
		if (retval < 0)
			return retval;
		TS_LOG_INFO("%s: Display configuration programmed\n", __func__);
	}

	if (rmi4_data->synaptics_chip_data->synaptics3718_fw_updata_flag
		&& (fwu->has_guest_code && fwu->img.contains_guest_code)) {

		retval = fwu_write_guest_code();
		if (retval < 0)
			return retval;
		TS_LOG_INFO("%s: Guest code programmed\n", __func__);
	}

	fwu->config_area = UI_CONFIG_AREA;
	retval = fwu_write_ui_configuration();
	if (retval < 0)
		return retval;
	TS_LOG_INFO("%s: Configuration programmed\n", __func__);

	retval = fwu_write_firmware();
	if (retval < 0)
		return retval;
	TS_LOG_INFO("%s: Firmware programmed\n", __func__);

	if (fwu->new_partition_table){
		if (fwu->has_guest_code && fwu->img.contains_guest_code) {
			retval = fwu_write_guest_code();
			if (retval < 0)
				return retval;
			TS_LOG_INFO("%s: Guest code programmed\n", __func__);
		}
	}

	return retval;
}

static int fwu_do_read_config(void)
{

	int retval;
	unsigned short block_count;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	switch (fwu->config_area) {
	case UI_CONFIG_AREA:
		block_count = fwu->blkcount.ui_config;
		break;
	case DP_CONFIG_AREA:
		if (!fwu->flash_properties.has_disp_config) {
			TS_LOG_ERR("%s: Display configuration not supported\n",__func__);
			return -EINVAL;
		}
		block_count = fwu->blkcount.dp_config;
		break;
	case PM_CONFIG_AREA:
		if (!fwu->flash_properties.has_pm_config) {
			TS_LOG_ERR
			    ("%s: Permanent configuration not supported\n",
			     __func__);
			return -EINVAL;
		}
		block_count = fwu->blkcount.pm_config;
		break;
	case BL_CONFIG_AREA:
		if (!fwu->flash_properties.has_bl_config) {
			TS_LOG_ERR("%s: Bootloader configuration not supported\n",__func__);
			return -EINVAL;
		}
		block_count = fwu->blkcount.bl_config;
		break;
	default:
		TS_LOG_ERR("%s: Invalid config area\n", __func__);
		return -EINVAL;
	}

	if (block_count == 0) {
		TS_LOG_ERR("%s: Invalid block count\n", __func__);
		return -EINVAL;
	}
	/*mutex_lock(&rmi4_data->rmi4_exp_init_mutex);*/

if (fwu->bl_version == BL_V5 || fwu->bl_version == BL_V6) {
	retval = fwu_enter_flash_prog();
	if (retval < 0)
		goto exit;
}
	fwu->config_size = fwu->block_size * block_count;
	kfree(fwu->read_config_buf);
	fwu->read_config_buf = kzalloc(fwu->config_size, GFP_KERNEL);
	if (!fwu->read_config_buf) {
		TS_LOG_ERR("%s: Failed to alloc mem for fwu->read_config_buf\n",__func__);
		fwu->read_config_buf_size = 0;
		retval = -ENOMEM;
		goto exit;
	}
	fwu->read_config_buf_size = fwu->config_size;

	retval = fwu_read_f34_blocks(block_count, CMD_READ_CONFIG);

exit:
if (fwu->bl_version == BL_V5 || fwu->bl_version == BL_V6) {
	rmi4_data->reset_device(rmi4_data);

/*      mutex_unlock(&rmi4_data->rmi4_exp_init_mutex);*/

	retval = fwu_scan_pdt();

}
	return retval;
}

static int fwu_do_read_config_workaround(void)
{
	int retval;
	int retry = 0;
	int retry_total = 5;

	TS_LOG_INFO("%s: do workaround here + \n", __func__);

	for (retry = 0; retry < retry_total; retry++) {
		retval = fwu_write_f34_v7_command_single_transaction(CMD_ENABLE_FLASH_PROG);
		if (retval < 0) {
			TS_LOG_ERR("%s: fail to send entering bl mode, retry = %d\n", __func__, retry + 1);
			msleep(5);
		}
		else {
			break;
		}
	}
	if ((retry == retry_total) && (retval < 0)) {
		TS_LOG_ERR("%s error: timeout to send a command for entering bl mode\n", __func__);
		goto exit;
	}

	fwu->config_area = UI_CONFIG_AREA;
	fwu_erase_configuration();

	if (fwu->has_guest_code) {
		fwu_erase_guest_code();
	}

	fwu_read_f34_queries();

	retval = fwu_do_read_config();
	if (retval < 0) {
		TS_LOG_ERR("%s: fail to read config again\n", __func__);
	}


exit:
	return retval;
}

static int fwu_do_read_lockdown_data(void)
{
	int retval;
	unsigned short block_count;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	block_count = fwu->blkcount.lockdown_data;

	fwu->config_size = fwu->block_size * block_count;
	if(fwu->read_config_buf)
		kfree(fwu->read_config_buf);

	fwu->read_config_buf = kzalloc(fwu->config_size, GFP_KERNEL);
	TS_LOG_DEBUG("fwu->config_size = %d \n", fwu->config_size);
	if (!fwu->read_config_buf) {
		TS_LOG_ERR("%s: Failed to alloc mem for fwu->read_config_buf\n",__func__);
		fwu->read_config_buf_size = 0;
		retval = -ENOMEM;
		goto exit;
	}
	fwu->read_config_buf_size = fwu->config_size;
	retval = fwu_read_f34_blocks(block_count, CMD_READ_LOCKDOWN_DATA);
	rmi4_data->reset_device(rmi4_data);
exit:
	return retval;

}

static int fwu_start_reflash(void)
{
	int retval = 0;

	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	/*rmi4_data->stay_awake = true;*/
	retval = fwu_enter_flash_prog();
	if (retval < 0) {
		rmi4_data->reset_device(rmi4_data);
		return retval;
	}

	retval = fwu_do_reflash();
	msleep(WAIT_FW_REFLASH_MS);
	rmi4_data->reset_device(rmi4_data);

	retval = fwu_read_flash_status();

	if (retval < 0)
		TS_LOG_ERR("fwu_read_flash_status read error\n");
	else
		TS_LOG_INFO("fwu->in_bl_mode is %s\n",
			    fwu->in_bl_mode ? "in bl mode" : "normal mode");
	/*rmi4_data->stay_awake = false;*/

	return retval;
}

static int fwu_recovery_check_status(void)
{
	int retval;
	unsigned char base;
	unsigned char status;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f35_fd.data_base_addr;

	retval = fwu->fn_ptr->read(rmi4_data,
				   base + F35_ERROR_CODE_OFFSET, &status, 1);
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to read status\n", __func__);
		return retval;
	}

	status = status & MASK_5BIT;

	if (status != 0x00) {
		TS_LOG_ERR("%s: Recovery mode status = %d\n", __func__, status);
		return -EINVAL;
	}

	return 0;
}

static int fwu_recovery_erase_all(void)
{
	int retval;
	unsigned char base;
	unsigned char command = CMD_F35_ERASE_ALL;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f35_fd.ctrl_base_addr;

	retval = fwu->fn_ptr->write(rmi4_data,
				    base + F35_CHUNK_COMMAND_OFFSET,
				    &command, sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("fwu_recovery_erase_all\n");
#if defined (CONFIG_HUAWEI_DSM)
		rmi4_data->synaptics_chip_data->ts_platform_data->dsm_info.constraints_UPDATE_status =
		    fwu_enter_flash_prog_fail;
#endif
		return retval;
	}

	msleep(F35_ERASE_ALL_WAIT_MS);

	retval = fwu_recovery_check_status();
	if (retval < 0)
		return retval;

	return 0;
}

static int fwu_recovery_write_chunk(void)
{
	int retval;
	unsigned char base;
	unsigned char chunk_number[] = { 0, 0 };
	unsigned char chunk_spare;
	unsigned char chunk_size;
	unsigned char buf[F35_CHUNK_SIZE + 1];
	unsigned short chunk;
	unsigned short chunk_total;
	unsigned short bytes_written = 0;
	unsigned char *chunk_ptr = (unsigned char *)fwu->image;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f35_fd.ctrl_base_addr;

	retval = fwu->fn_ptr->write(rmi4_data,
				    base + F35_CHUNK_NUM_LSB_OFFSET,
				    chunk_number, sizeof(chunk_number));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write chunk number\n", __func__);
		return retval;
	}

	buf[sizeof(buf) - 1] = CMD_F35_WRITE_CHUNK;

	chunk_total = fwu->image_size / F35_CHUNK_SIZE;
	chunk_spare = fwu->image_size % F35_CHUNK_SIZE;
	if (chunk_spare)
		chunk_total++;

	for (chunk = 0; chunk < chunk_total; chunk++) {
		if (chunk_spare && chunk == chunk_total - 1)
			chunk_size = chunk_spare;
		else
			chunk_size = F35_CHUNK_SIZE;

		memset(buf, 0x00, F35_CHUNK_SIZE);
		retval = secure_memcpy(buf, sizeof(buf), chunk_ptr,
				fwu->image_size - bytes_written, chunk_size);
		if (retval) {
			TS_LOG_ERR("%s: Failed to copy data \n", __func__);
			return retval;
		}

		retval =
		    fwu->fn_ptr->write(rmi4_data, base + F35_CHUNK_DATA_OFFSET,
				       buf, sizeof(buf));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to write chunk data (chunk %d)\n",__func__, chunk);
			return retval;
		}
		chunk_ptr += chunk_size;
		bytes_written += chunk_size;
	}

	retval = fwu_recovery_check_status();
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write chunk data\n", __func__);
		return retval;
	}

	return 0;
}

static int fwu_recovery_reset(void)
{
	int retval;
	unsigned char base;
	unsigned char command = CMD_F35_RESET;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f35_fd.ctrl_base_addr;

	retval = fwu->fn_ptr->write(rmi4_data,
				    base + F35_CHUNK_COMMAND_OFFSET,
				    &command, sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to issue reset command\n", __func__);
		return retval;
	}

	msleep(F35_RESET_WAIT_MS);

	return 0;
}

static int fwu_start_recovery(void)
{
	int retval = 0;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	if (rmi4_data->sensor_sleep) {
		TS_LOG_ERR("%s: Sensor sleeping\n", __func__);
		return -ENODEV;
	}
	/*rmi4_data->stay_awake = true;*/

	/*mutex_lock(&rmi4_data->rmi4_exp_init_mutex);*/

	pr_notice("%s: Start of recovery process\n", __func__);

	/*retval = rmi4_data->irq_enable(rmi4_data, false, false);*/
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to disable interrupt\n", __func__);
		goto exit;
	}

	retval = fwu_recovery_erase_all();
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to do erase all in recovery mode\n",__func__);
		goto exit;
	}

	pr_notice("%s: External flash erased\n", __func__);

	retval = fwu_recovery_write_chunk();
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to write chunk data in recovery mode\n",__func__);
		goto exit;
	}

	pr_notice("%s: Chunk data programmed\n", __func__);

	retval = fwu_recovery_reset();
	if (retval < 0) {
		TS_LOG_ERR("%s: Failed to reset device in recovery mode\n",__func__);
		goto exit;
	}

	pr_notice("%s: Recovery mode reset issued\n", __func__);
	rmi4_data->use_ub_addr = false;
	rmi4_data->reset_device(rmi4_data);

	retval = 0;

exit:
	pr_notice("%s: End of recovery process\n", __func__);

	return retval;
}

int synap_fw_s3718_update(void)
{
	int retval;

	if (!fwu)
		return -ENODEV;

	if (!fwu->initialized)
		return -ENODEV;

	if (fwu->in_ub_mode){
		retval = fwu_start_recovery();
		if (retval < 0){
			TS_LOG_ERR("Failed to do recovery\n");
			return -ENODEV;
		}
	}else{
		retval = fwu_start_reflash();
		if (retval < 0){
			TS_LOG_ERR("Failed to update firmware\n");
			fwu->image = NULL;
			return -ENODEV;
		}
	}

	fwu->image = NULL;

	return retval;
}

EXPORT_SYMBOL(synap_fw_update);

#ifdef DO_STARTUP_FW_UPDATE
static void fwu_startup_fw_update_work(struct work_struct *work)
{
	static unsigned char do_once = 1;
#ifdef WAIT_FOR_FB_READY
	unsigned int timeout;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;
#endif

	if (!do_once)
		return;
	do_once = 0;

#ifdef WAIT_FOR_FB_READY
	timeout = FB_READY_TIMEOUT_S * 1000 / FB_READY_WAIT_MS + 1;

	while (!rmi4_data->fb_ready) {
		msleep(FB_READY_WAIT_MS);
		timeout--;
		if (timeout == 0) {
			TS_LOG_ERR("%s: Timed out waiting for FB ready\n",__func__);
			return;
		}
	}
#endif

	synaptics_fw_updater(NULL);

	return;
}
#endif

static int synaptics_read_lockdown_data(void)
{
	int retval = -EINVAL;
	int index = 0;
	unsigned short block_count;
	unsigned char test[10];

	if (fwu->bl_version != BL_V6) {
		TS_LOG_ERR("%s: Not support lockdown data in bl v.%d\n", __func__, fwu->bl_version);
		goto out;
	}
	else if (!fwu->has_lockdown_data){
		TS_LOG_ERR("%s: Not support lockdown data in this firmware\n", __func__);
		goto out;
	}

	retval = fwu_do_read_lockdown_data();
	if (retval < 0){
		TS_LOG_ERR( "%s: Failed to read lockdown data\n",__func__);
		goto out;
	}
out:
	return retval;
}

#ifdef SYNA_UPP

static int fwu_erase_oem_data(void)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	retval = fwu_write_f34_command(CMD_ERASE_OEM_DATA);
	if (retval < 0)
		return retval;

	TS_LOG_DEBUG("%s: Erase command written\n",__func__);

	msleep(100);

	retval = fwu_wait_for_idle(ERASE_WAIT_MS, true);
	if (retval < 0)
		return retval;

	TS_LOG_DEBUG("%s: Idle status detected\n",__func__);

	return 0;
}

static int synaptics_read_oem_data(void)
{
	int retval = -EINVAL;
	int index = 0;
	unsigned short block_count;

	if (fwu->bl_version != BL_V6) {
		TS_LOG_ERR("%s: Not support oem data in bl v.%d\n", __func__, fwu->bl_version);
		goto out;
	}
	else if (!fwu->has_oem_data){
		TS_LOG_ERR("%s: Not support oem data in this firmware\n", __func__);
		goto out;
	}

	block_count = fwu->blkcount.oem_data;
	fwu->config_area = OEM_DATA_AREA;
	fwu->config_size = fwu->block_size * block_count;
	if (fwu->read_config_buf)
		kfree(fwu->read_config_buf);
	fwu->read_config_buf = kzalloc(fwu->config_size, GFP_KERNEL);
	if (!fwu->read_config_buf) {
		TS_LOG_ERR("%s: Failed to alloc mem for fwu->read_config_buf\n",__func__);
		fwu->read_config_buf_size = 0;
		return -ENOMEM;
	}
	fwu->read_config_buf_size = fwu->config_size;

	retval = fwu_read_f34_blocks(block_count, CMD_READ_CONFIG);
out:
	return retval;
}

short synap_get_oem_data_info(void)
{
	int retval=0;
	unsigned short oem_data_max_size = 0;
	//struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	retval = synap_fw_data_s3718_init(rmi4_data);
	if (retval) {
		TS_LOG_ERR("%s: synap_fw_data_s3718_init  Failed \n", __func__);
		return -EIO;
	}
	if (!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "S3331",5)) {
		oem_data_max_size = fwu->blkcount.utility_param * fwu->block_size/16;
	}

	else if (!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "S3330",5)) {
		oem_data_max_size = fwu->block_size * fwu->blkcount.pm_config;
	}
	else if ((!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "MAHA261611",6))
		|| (!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "MAHA211010",6))
		|| (!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "VCTO21160",6))) {
		oem_data_max_size = fwu->blkcount.oem_data * fwu->block_size;
	}

	return oem_data_max_size;
}

int synap_get_oem_data(unsigned char *oem_data, unsigned short leng)
{
	int retval = -EINVAL;
	unsigned char checksum_array[4];
	unsigned short utility_param_size;
	unsigned long checksum;
	unsigned short block_count;
	unsigned short config_area;

	//struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	retval = synap_fw_data_s3718_init(rmi4_data);
	if (retval) {
		TS_LOG_ERR("%s: synap_fw_data_s3718_init  Failed \n", __func__);
		return -EIO;
	}

	if (!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "S3331",5)) {
		utility_param_size = fwu->blkcount.utility_param * fwu->block_size;
		retval = fwu_read_utility_parameter();
		if (retval < 0)
			return retval;

		calculate_checksum((unsigned short *)fwu->read_config_buf,
				((utility_param_size - 4) / 2), &checksum);
		convert_to_little_endian(checksum_array, checksum);
		if (fwu->read_config_buf[utility_param_size - 4] != checksum_array[0] ||
					fwu->read_config_buf[utility_param_size - 3] != checksum_array[1] ||
					fwu->read_config_buf[utility_param_size - 2] != checksum_array[2] ||
					fwu->read_config_buf[utility_param_size - 1] != checksum_array[3])
			return -EINVAL;

		memcpy(oem_data, fwu->read_config_buf, leng);
	}

	if (!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "S3330",5)) {
		retval = synap_fw_data_s3718_init(rmi4_data);
		if (retval) {
			TS_LOG_ERR("%s: synap_fw_data_s3718_init  Failed \n", __func__);
			return -EIO;
		}
		fwu->config_area = PM_CONFIG_AREA;
		block_count = fwu->blkcount.pm_config;
		if (block_count == 0) {
			dev_err(rmi4_data->pdev->dev.parent,"%s: Invalid block count\n",__func__);
			return -EINVAL;
		}

		if (fwu->bl_version == BL_V5 || fwu->bl_version == BL_V6) {
			config_area = fwu->config_area;
			retval = fwu_enter_flash_prog();
			fwu->config_area = config_area;
			if (retval < 0)
				goto exit;
		}

		fwu->config_size = fwu->block_size * block_count;

		retval = fwu_allocate_read_config_buf(fwu->config_size);
		if (retval < 0)
			goto exit;

		memset(fwu->read_config_buf, 0x00, fwu->config_size);

		retval = fwu_read_f34_blocks(block_count, CMD_READ_CONFIG);
		if (retval < 0)
			goto exit;

		memcpy(oem_data, fwu->read_config_buf, fwu->config_size);    // fwu->config_size//leng

	exit:
		if (fwu->bl_version == BL_V5 || fwu->bl_version == BL_V6)
			rmi4_data->reset_device(fwu->rmi4_data);

	}
	else if ((!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "MAHA261611",6))
		|| (!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "MAHA211010",6))
		|| (!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "VCTO21160",6))) {
		retval = synaptics_read_oem_data();
		if (retval < 0)
			return retval;
		memcpy(oem_data, fwu->read_config_buf, leng);
	}
	synap_fw_data_s3718_release();
	return retval;
}

int synap_set_oem_data(unsigned char *oem_data, unsigned short leng)
{
	int retval = -EINVAL;
	unsigned char checksum_array[4];
	unsigned short utility_param_size;
	unsigned long checksum;
	unsigned short block_count;
	unsigned short config_area;

	//struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	retval = synap_fw_data_s3718_init(rmi4_data);
	if (retval) {
		TS_LOG_ERR("%s: synap_fw_data_s3718_init  Failed \n", __func__);
		return -EIO;
	}

	if (!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "S3331",5)) {
		/* reserve */
		/*
		retval = fwu_read_utility_parameter();
		if (retval < 0)
			goto exit;
		*/

		retval = fwu_enter_flash_prog();
		if (retval < 0)
			goto exit;
		retval = fwu_erase_utility_parameter();
		if (retval < 0)
			goto exit;

		utility_param_size = fwu->blkcount.utility_param * fwu->block_size;
		retval = fwu_allocate_read_config_buf(utility_param_size);
		if (retval < 0)
			goto exit;

		memset(fwu->read_config_buf, 0x00, utility_param_size);
		/* hard code header */
		fwu->read_config_buf[0] = 0x05;
		fwu->read_config_buf[1] = 0x00;
		fwu->read_config_buf[2] = 0x86;
		fwu->read_config_buf[3] = 0x00;
		memcpy(fwu->read_config_buf, oem_data, leng);

		calculate_checksum((unsigned short *)fwu->read_config_buf,
				   ((utility_param_size - 4) / 2), &checksum);

		convert_to_little_endian(checksum_array, checksum);

		fwu->read_config_buf[utility_param_size - 4] = checksum_array[0];
		fwu->read_config_buf[utility_param_size - 3] = checksum_array[1];
		fwu->read_config_buf[utility_param_size - 2] = checksum_array[2];
		fwu->read_config_buf[utility_param_size - 1] = checksum_array[3];

		retval = fwu_write_f34_blocks((unsigned char *)fwu->read_config_buf,
						  fwu->blkcount.utility_param,
						  CMD_WRITE_UTILITY_PARAM);
		if (retval < 0)
			goto exit;
	}
	if (!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "S3330",5)) {
		fwu->config_area = PM_CONFIG_AREA;
		config_area = fwu->config_area;
		retval = fwu_enter_flash_prog();
		if (retval < 0)
			goto exit;

		fwu->config_area = config_area;

		block_count = fwu->blkcount.pm_config;
		if (block_count == 0) {
			TS_LOG_ERR("%s: Invalid block count\n", __func__);
			return -EINVAL;
		}

		if (fwu->bl_version == BL_V5 || fwu->bl_version == BL_V6) {
			config_area = fwu->config_area;
			retval = fwu_enter_flash_prog();
			fwu->config_area = config_area;
			if (retval < 0)
				goto exit;
		}

		fwu->config_size = fwu->block_size * block_count;

		retval = fwu_allocate_read_config_buf(fwu->config_size);
		if (retval < 0)
			goto exit;

		memset(fwu->read_config_buf, 0x00, fwu->config_size);

		retval = fwu_read_f34_blocks(block_count, CMD_READ_CONFIG);
		if (retval < 0)
			goto exit;

		block_count = fwu->blkcount.pm_config;
		if (block_count == 0) {
			TS_LOG_ERR( "%s: Invalid block count\n",__func__);
			return -EINVAL;
		}

		fwu->config_size = fwu->block_size * block_count;
		retval = fwu_allocate_read_config_buf(fwu->config_size);
		if (retval < 0) {
			goto exit;
		}

		//memset(fwu->read_config_buf, 0x00, fwu->config_size);  // avoid flash_status error
		memcpy(fwu->read_config_buf, oem_data, fwu->config_size);

		retval = fwu_write_f34_blocks((unsigned char *)fwu->read_config_buf,
				block_count, CMD_WRITE_CONFIG);
		if (retval < 0) {
			synap_fw_data_s3718_release();
			goto exit;
		}
	}
	else if ((!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "MAHA261611",6))
		|| (!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "MAHA211010",6))
		|| (!strncmp(rmi4_data->rmi4_mod_info.product_id_string, "VCTO21160",6))) {

		retval = fwu_enter_flash_prog();
		if (retval < 0)
			goto exit;
		retval = fwu_erase_oem_data();
		if (retval < 0)
			goto exit;
		retval = fwu_allocate_read_config_buf(fwu->blkcount.oem_data * fwu->block_size);
		if (retval < 0)
			goto exit;
		if(fwu->blkcount.oem_data * fwu->block_size > USHRT_MAX){
			TS_LOG_ERR( "%s: invalid fwu->read_config_buf_size\n",__func__);
			goto exit;
		}
		memset(fwu->read_config_buf, 0x00, fwu->blkcount.oem_data * fwu->block_size);
		memcpy(fwu->read_config_buf, oem_data, leng);
		/* need to modify here: parsing logic */
		fwu->config_area = OEM_DATA_AREA;
		retval = fwu_write_f34_blocks(fwu->read_config_buf,
				fwu->blkcount.oem_data, CMD_WRITE_CONFIG);
		if (retval < 0)
			goto exit;
	}

exit:
	//synap_fw_data_s3718_release();  //avoid panic
	fwu->rmi4_data->reset_device(fwu->rmi4_data);
	return retval;
}
#endif


static int get_lockdown_data(unsigned char *lockdown_data, unsigned short leng)
{
	int retval = -EINVAL;
	retval = synaptics_read_lockdown_data();
	if (retval < 0)
		return retval;
	memcpy(lockdown_data, fwu->read_config_buf + 4, leng);
	return retval;
}

int set_lockdown_data(unsigned char *lockdown_data, unsigned short leng)
{
	int retval = -EINVAL;
	retval = fwu_enter_flash_prog();
	if (retval < 0)
		goto exit;
	retval = fwu_erase_lockdown_data();
	if (retval < 0)
		goto exit;
	retval = fwu_allocate_read_config_buf(fwu->blkcount.lockdown_data * fwu->block_size);
	if (retval < 0)
		goto exit;
	memset(fwu->read_config_buf, 0x00, fwu->blkcount.lockdown_data * fwu->block_size);
	memcpy(fwu->read_config_buf + 4, lockdown_data, leng);
	retval = fwu_write_lockdown_data();
	if (retval < 0)
		goto exit;
exit:
	return retval;
}

static int synaptics_get_project_id(unsigned char *projectid, int plen){
	if (!projectid || !fwu || !fwu->rmi4_data){
		TS_LOG_ERR("%s error!", __func__);
		return -1;
	}
	unsigned char *project_id =
	    fwu->rmi4_data->rmi4_mod_info.project_id_string;
	snprintf(projectid, plen, "%s", project_id);
	return 0;
}

bool synaptics_tddi_new_seq(void){
    bool ret = false;
    unsigned char projectid[SYNAPTICS_RMI4_PROJECT_ID_SIZE+1] = { 0 };

    memset(projectid, 0, sizeof(projectid));
    if (synaptics_get_project_id(projectid, sizeof(projectid))){
        TS_LOG_ERR("get project error");
        return ret;
    }else{
        TS_LOG_INFO("projectid:%s\n", projectid);
    }

    if (!strcmp(projectid, "VCTO21161")){
        ret = true;
    }else{
        ret = false;
    }
    return ret;
}

static int synaptics_read_project_id(void)
{
	int retval = -EINVAL;
	int index = 0;
	unsigned char *project_id =
	    fwu->rmi4_data->rmi4_mod_info.project_id_string;

	TS_LOG_INFO("%s: fwu->bl_version = %d\n", __func__, fwu->bl_version);
	TS_LOG_INFO("%s: ic_type = %d\n", __func__, fwu->rmi4_data->synaptics_chip_data->ic_type);

	if (fwu->bl_version != BL_V7 && fwu->bl_version != BL_V8) {
		if ((fwu->rmi4_data->synaptics_chip_data->ic_type != SYNAPTICS_TD4322)
			&&(fwu->rmi4_data->synaptics_chip_data->ic_type != SYNAPTICS_TD4310)){
			memcpy(project_id,
				fwu->rmi4_data->rmi4_mod_info.product_id_string,
				SYNAPTICS_RMI4_PROJECT_ID_SIZE);
			project_id[SYNAPTICS_RMI4_PROJECT_ID_SIZE] = 0;
			retval = 0;
			goto out;
		} else {
			fwu_read_f34_queries();
			retval = get_lockdown_data(project_id, SYNAPTICS_RMI4_PROJECT_ID_SIZE);
			if (retval < 0) {
				TS_LOG_INFO("get_lockdown_data failed \n");
				memcpy(project_id,
					fwu->rmi4_data->rmi4_mod_info.product_id_string,
					SYNAPTICS_RMI4_PROJECT_ID_SIZE);
				project_id[SYNAPTICS_RMI4_PROJECT_ID_SIZE] = 0;
				if (!strncmp(fwu->rmi4_data->rmi4_mod_info.product_id_string, "MAHA261601", 10)) {
					memcpy(fwu->rmi4_data->rmi4_mod_info.project_id_string, "MAHA261611", SYNAPTICS_RMI4_PROJECT_ID_SIZE);  //MAHA_SHARP_TD4722
				}
				else if(!strncmp(fwu->rmi4_data->rmi4_mod_info.product_id_string, "MAHA211600", 10)){
					memcpy(fwu->rmi4_data->rmi4_mod_info.project_id_string, "MAHA211610", SYNAPTICS_RMI4_PROJECT_ID_SIZE);  //MAHA_SHARP_TD4322
				}
				else if(!strncmp(fwu->rmi4_data->rmi4_mod_info.product_id_string, "MAHA261001", 10)){
					memcpy(fwu->rmi4_data->rmi4_mod_info.project_id_string, "MAHA261011", SYNAPTICS_RMI4_PROJECT_ID_SIZE);  //MAHA_LGD_TD4722
				}
				else if(!strncmp(fwu->rmi4_data->rmi4_mod_info.product_id_string, "MAHA211000", 10)){
					memcpy(fwu->rmi4_data->rmi4_mod_info.project_id_string, "MAHA211010", SYNAPTICS_RMI4_PROJECT_ID_SIZE);  //MAHA_LGD_TD4322
				}
				else
				TS_LOG_ERR("%s:unknow project!!!\n", __func__);

				retval = 0;
				fwu->write_project_id = true;
				goto out;
			} else {
			TS_LOG_ERR("get_lockdown_data successfully\n", __func__);
			goto out;
			}
		}
	}

	fwu->config_area = PM_CONFIG_AREA;

	retval = fwu_do_read_config();
	if (retval < 0) {
		TS_LOG_ERR("%s: Try to read config again\n", __func__);

		retval = fwu_do_read_config_workaround();
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read config\n", __func__);
			goto out;
		}
	}

	for (index = 0;
	     index < SYNAPTICS_RMI4_PROJECT_ID_SIZE && index < fwu->config_size;
	     index++) {
		if ((' ' == fwu->read_config_buf[index])
		    || ('\0' == fwu->read_config_buf[index])) {
			break;
		}
		project_id[index] = fwu->read_config_buf[index];
	}
	project_id[index] = 0;
	if (!project_id[0] && 0 == fwu->read_config_buf[0] && 1 == fwu->read_config_buf[1]){
		TS_LOG_INFO("%s: read project id fail from oemdata, try read from oemdata again\n", __func__);
		for (index = 2;
		     index < SYNAPTICS_RMI4_PROJECT_ID_SIZE+2 && index < fwu->config_size;
		     index++) {
			if ((' ' == fwu->read_config_buf[index])
			    || ('\0' == fwu->read_config_buf[index])) {
				break;
			}
			project_id[index-2] = fwu->read_config_buf[index];
		}
		project_id[index-2] = 0;
	}

out:
	return retval;
}

int synap_fw_data_s3718_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct pdt_properties pdt_props;

	if (fwu) {
		TS_LOG_INFO("%s: Handle already exists\n", __func__);
		return 0;
	}

	fwu = kzalloc(sizeof(*fwu), GFP_KERNEL);
	if (!fwu) {
		TS_LOG_ERR("%s: Failed to alloc mem for fwu\n", __func__);
		retval = -ENOMEM;
		goto exit;
	}

	fwu->fn_ptr = kzalloc(sizeof(*(fwu->fn_ptr)), GFP_KERNEL);
	if (!fwu->fn_ptr) {
		TS_LOG_ERR("%s: Failed to alloc mem for fn_ptr\n", __func__);
		retval = -ENOMEM;
		goto exit_free_fwu;
	}

	fwu->image_name = kzalloc(MAX_IMAGE_NAME_LEN, GFP_KERNEL);
	if (!fwu->image_name) {
		TS_LOG_ERR("%s: Failed to alloc mem for image name\n",__func__);
		retval = -ENOMEM;
		goto exit_free_fn_ptr;
	}

	fwu->rmi4_data = rmi4_data;
	fwu->fn_ptr->read = rmi4_data->i2c_read;
	fwu->fn_ptr->write = rmi4_data->i2c_write;

	retval = fwu->fn_ptr->read(rmi4_data,
				   PDT_PROPS,
				   pdt_props.data, sizeof(pdt_props.data));
	if (retval < 0) {
		TS_LOG_DEBUG("%s: Failed to read PDT properties, assuming 0x00\n",__func__);
	} else if (pdt_props.has_bsr) {
		TS_LOG_ERR("%s: Reflash for LTS not currently supported\n",__func__);
		retval = -ENODEV;
		goto exit_free_mem;
	}

		retval = fwu_scan_pdt();
		if (retval < 0)
			goto exit_free_mem;

		if (!fwu->in_ub_mode) {
			retval = fwu_read_f34_queries();
			if (retval < 0)
				goto exit_free_mem;
		}

	retval = synaptics_read_project_id();
	if (retval < 0) {
		TS_LOG_ERR("Failed to read project id\n");
		goto exit_free_mem;
	}

	fwu->force_update = FORCE_UPDATE;
	fwu->do_lockdown = DO_LOCKDOWN;
	fwu->initialized = true;

	return 0;

exit_free_mem:
	kfree(fwu->image_name);

exit_free_fn_ptr:
	kfree(fwu->fn_ptr);

exit_free_fwu:
	kfree(fwu);
	fwu = NULL;

exit:
	return retval;
}
/*    get the firmware data form boot.img
 */
int synap_get_fw_data_s3718_boot(char *file_name,
				     struct touch_settings
				     *synaptics_sett_param_regs)
{
	if((!file_name)||(!synaptics_sett_param_regs)||(!synaptics_sett_param_regs->module_name)){
		TS_LOG_ERR("filename empty or param regs NULL\n");
		return  -EIO;
	}

	int retval;
	size_t file_name_size  =strlen(file_name) + strlen(synaptics_sett_param_regs->module_name);
	char firmware_name[RMI_PRODUCT_ID_LENGTH + file_name_size + 1];
	struct device *dev = &fwu->rmi4_data->synaptics_dev->dev;

	snprintf(firmware_name, sizeof(firmware_name), "ts/%s_%s.img",
		 file_name, synaptics_sett_param_regs->module_name);
	firmware_name[RMI_PRODUCT_ID_LENGTH + file_name_size] = 0;

	TS_LOG_INFO("file_name size = %ld, Requesting firmware image %s\n",
		    file_name_size, firmware_name);

	if (fwu->in_ub_mode) {
		retval = request_firmware(&fwu->fw_entry_boot, "ts/synaptics.hex", dev);
		if (retval != 0) {
			TS_LOG_ERR("synaptics.hex not available\n");
			return retval;
		}
	} else {
		retval = request_firmware(&fwu->fw_entry_boot, firmware_name, dev);
		if (retval != 0) {
			TS_LOG_ERR("Firmware image %s not available\n", firmware_name);
			return retval;
		}
	}

	if (fwu->fw_entry_boot == NULL) {
		TS_LOG_ERR("fw is null\n");
		return -EINVAL;
	}

	fwu->image = fwu->fw_entry_boot->data;
	fwu->image_size = fwu->fw_entry_boot->size;

	TS_LOG_INFO("Firmware image size = %ld\n", fwu->fw_entry_boot->size);
	return NO_ERR;
}

/*    get the firmware data form file in sd card.
 */
int synap_get_fw_data_s3718_sd(void)
{
	int retval;

	struct device *dev = &fwu->rmi4_data->synaptics_dev->dev;
	retval = request_firmware(&fwu->fw_entry_sd, SYNAPTICS_FW_S3718_MANUAL_UPDATE_FILE_NAME, dev);
	if (retval != 0) {
		TS_LOG_ERR("Firmware image not available\n");
		return retval;
	}

	if (fwu->fw_entry_sd== NULL) {
		TS_LOG_ERR("fw is null\n");
		return -EINVAL;
	}

	fwu->image = fwu->fw_entry_sd->data;
	fwu->image_size = fwu->fw_entry_sd->size;
	TS_LOG_INFO("Firmware image size = %ld\n", fwu->fw_entry_sd->size);
	return NO_ERR;
}

bool synap_check_fw_s3718_version(void)
{
	enum flash_area flash_area;
	int retval = 0;

	// F35
	if (fwu->in_ub_mode)
		return 1;

	retval = fwu_parse_image_info();
	if (retval < 0) {
		TS_LOG_ERR("fwu_parse_image_info error\n");
		return false;
	}
	TS_LOG_INFO("fwu->bl_version = 0x%2x, fwu->img.bl_version = 0x%2x\n",
		fwu->bl_version, fwu->img.bl_version);

	if (fwu->bl_version != fwu->img.bl_version) {
		TS_LOG_ERR("%s: Bootloader version mismatch\n", __func__);
		return false;
	}

	if (!fwu->force_update && fwu->new_partition_table) {
		TS_LOG_INFO("%s: Partition table mismatch\n", __func__);
	}

	retval = fwu_read_flash_status();
	if (retval < 0)
		return false;

	if (fwu->in_bl_mode) {
		TS_LOG_INFO("%s: Device in bootloader mode\n", __func__);
	}

	flash_area = fwu_go_nogo();
	if (flash_area == UI_FIRMWARE) {
		TS_LOG_INFO("update fw\n");
		return true;
	} else {
		TS_LOG_INFO("no need to update fw\n");
		return false;
	}
}

void synap_fw_data_s3718_release(void)
{
	TS_LOG_INFO("s3718 release fw resource\n");
	//Do not need to release hw data during the test
	return 0;
}
