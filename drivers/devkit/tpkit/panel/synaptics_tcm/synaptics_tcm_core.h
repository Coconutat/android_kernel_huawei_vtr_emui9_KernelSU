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

#ifndef _SYNAPTICS_TCM_CORE_H_
#define _SYNAPTICS_TCM_CORE_H_

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/ctype.h>
#include "synaptics_tcm.h"
#include "../../huawei_ts_kit.h"

#ifdef CONFIG_FB
#include <linux/fb.h>
#include <linux/notifier.h>
#endif

#define SYNAPTICS_TCM_ID_PRODUCT (1 << 0)
#define SYNAPTICS_TCM_ID_VERSION 0x0009
#define SYNAPTICS_TCM_ID_SUBVERSION 1
#define SPI_DEFLAUT_SPEED		10000000

#define PLATFORM_DRIVER_NAME "synaptics_tcm"
#define TOUCH_INPUT_NAME "synaptics_tcm_touch"
#define TOUCH_INPUT_PHYS_PATH "synaptics_tcm/touch_input"
#define SYNAPTICS_VENDER_NAME  "synaptics_tcm"

//#define WAKEUP_GESTURE

#define RD_CHUNK_SIZE 0 /* read length limit in bytes, 0 = unlimited */
#define WR_CHUNK_SIZE 0 /* write length limit in bytes, 0 = unlimited */

#define GLOVE_SWITCH_ON 1
#define GLOVE_SWITCH_OFF 0

#define CHARGER_SWITCH_ON 1
#define CHARGER_SWITCH_OFF 0

#define GPIO_OUTPUT_HIGH	1
#define GPIO_OUTPUT_LOW	0

#define FIXED_READ_LENGTH 256
#define MESSAGE_HEADER_SIZE 4
#define MESSAGE_MARKER 0xa5
#define MESSAGE_PADDING 0x5a

#define PDT_START_ADDR 0x00e9
#define PDT_END_ADDR 0x00ee
#define UBL_FN_NUMBER 0x35

#define INIT_BUFFER(buffer, is_clone) \
	mutex_init(&buffer.buf_mutex); \
	buffer.clone = is_clone

#define LOCK_BUFFER(buffer) \
	mutex_lock(&buffer.buf_mutex)

#define UNLOCK_BUFFER(buffer) \
	mutex_unlock(&buffer.buf_mutex)

#define RELEASE_BUFFER(buffer) \
	do { \
		if (buffer.clone == false) { \
			kfree(buffer.buf); \
			buffer.buf_size = 0; \
			buffer.data_length = 0; \
		} \
	} while (0)

#define MAX(a, b) \
	({__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a > _b ? _a : _b; })

#define MIN(a, b) \
	({__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a < _b ? _a : _b; })

#define STR(x) #x

#define CONCAT(a, b) a##b

#define SHOW_PROTOTYPE(m_name, a_name) \
static ssize_t CONCAT(m_name##_sysfs, _##a_name##_show)(struct device *dev, \
		struct device_attribute *attr, char *buf); \
\
static struct device_attribute dev_attr_##a_name = \
		__ATTR(a_name, S_IRUGO, \
		CONCAT(m_name##_sysfs, _##a_name##_show), \
		syna_tcm_store_error);

#define STORE_PROTOTYPE(m_name, a_name) \
static ssize_t CONCAT(m_name##_sysfs, _##a_name##_store)(struct device *dev, \
		struct device_attribute *attr, const char *buf, size_t count); \
\
static struct device_attribute dev_attr_##a_name = \
		__ATTR(a_name, (S_IWUSR | S_IWGRP), \
		syna_tcm_show_error, \
		CONCAT(m_name##_sysfs, _##a_name##_store));

#define SHOW_STORE_PROTOTYPE(m_name, a_name) \
static ssize_t CONCAT(m_name##_sysfs, _##a_name##_show)(struct device *dev, \
		struct device_attribute *attr, char *buf); \
\
static ssize_t CONCAT(m_name##_sysfs, _##a_name##_store)(struct device *dev, \
		struct device_attribute *attr, const char *buf, size_t count); \
\
static struct device_attribute dev_attr_##a_name = \
		__ATTR(a_name, (S_IRUGO | S_IWUSR | S_IWGRP), \
		CONCAT(m_name##_sysfs, _##a_name##_show), \
		CONCAT(m_name##_sysfs, _##a_name##_store));

#define ATTRIFY(a_name) (&dev_attr_##a_name)

#define SYNAPTICS_RMI4_DATE_CODE_SIZE 3
#define SYNAPTICS_RMI4_PRODUCT_INFO_SIZE 2
#define SYNAPTICS_RMI4_PRODUCT_ID_SIZE 10
#define SYNAPTICS_RMI4_PROJECT_ID_SIZE SYNAPTICS_RMI4_PRODUCT_ID_SIZE
#define SYNAPTICS_RMI4_BUILD_ID_SIZE 3
#define SYNAPTICS_RMI4_IC_NAME_SIZE 2
#define SYNAPTICS_RMI4_CAP_DATA_SIZE 1024
#define SYNAPTICS_BUILD_ID_NO 3
#define SYNAPTICS_IC_NAME_NO 2

enum module_type {
	TCM_TOUCH = 0,
	TCM_DEVICE = 1,
	TCM_TESTING = 2,
	TCM_REFLASH = 3,
	TCM_RECOVERY = 4,
	TCM_ZEROFLASH = 5,
	TCM_DIAGNOSTICS = 6,
	TCM_LAST,
};

enum boot_mode {
	MODE_APPLICATION = 0x01,
	MODE_HOST_DOWNLOAD = 0x02,
	MODE_BOOTLOADER = 0x0b,
	MODE_TDDI_BOOTLOADER = 0x0c,
};

enum boot_status {
	BOOT_STATUS_OK = 0x00,
	BOOT_STATUS_BOOTING = 0x01,
	BOOT_STATUS_APP_BAD_DISPLAY_CRC = 0xfc,
	BOOT_STATUS_BAD_DISPLAY_CONFIG = 0xfd,
	BOOT_STATUS_BAD_APP_FIRMWARE = 0xfe,
	BOOT_STATUS_WARM_BOOT = 0xff,
};
enum report_status {
	NOT_NEED_REPORT = 0,
	NEED_REPORT = 1,
};
enum app_status {
	APP_STATUS_OK = 0x00,
	APP_STATUS_BOOTING = 0x01,
	APP_STATUS_UPDATING = 0x02,
	APP_STATUS_BAD_APP_CONFIG = 0xff,
};

enum firmware_mode {
	FW_MODE_BOOTLOADER = 0,
	FW_MODE_APPLICATION = 1,
};

enum dynamic_config_id {
	DC_UNKNOWN = 0x00,
	DC_NO_DOZE,
	DC_DISABLE_NOISE_MITIGATION,
	DC_INHIBIT_FREQUENCY_SHIFT,
	DC_REQUESTED_FREQUENCY,
	DC_DISABLE_HSYNC,
	DC_REZERO_ON_EXIT_DEEP_SLEEP,
	DC_CHARGER_CONNECTED,
	DC_NO_BASELINE_RELAXATION,
	DC_IN_WAKEUP_GESTURE_MODE,
	DC_STIMULUS_FINGERS,
	DC_GRIP_SUPPRESSION_ENABLED,
	DC_ENABLE_THICK_GLOVE,
	DC_ENABLE_GLOVE,
	DC_ENABLE_ROI = 0xCA,
	DC_SCENE_SWITCH = 0xCB,
	DC_ENABLE_HOLSTER = 0xCC,
};

enum command {
	CMD_NONE = 0x00,
	CMD_CONTINUE_WRITE = 0x01,
	CMD_IDENTIFY = 0x02,
	CMD_RESET = 0x04,
	CMD_ENABLE_REPORT = 0x05,
	CMD_DISABLE_REPORT = 0x06,
	CMD_GET_BOOT_INFO = 0x10,
	CMD_ERASE_FLASH = 0x11,
	CMD_WRITE_FLASH = 0x12,
	CMD_READ_FLASH = 0x13,
	CMD_RUN_APPLICATION_FIRMWARE = 0x14,
	CMD_SPI_MASTER_WRITE_THEN_READ = 0x15,
	CMD_REBOOT_TO_ROM_BOOTLOADER = 0x16,
	CMD_RUN_BOOTLOADER_FIRMWARE = 0x1f,
	CMD_GET_APPLICATION_INFO = 0x20,
	CMD_GET_STATIC_CONFIG = 0x21,
	CMD_SET_STATIC_CONFIG = 0x22,
	CMD_GET_DYNAMIC_CONFIG = 0x23,
	CMD_SET_DYNAMIC_CONFIG = 0x24,
	CMD_GET_TOUCH_REPORT_CONFIG = 0x25,
	CMD_SET_TOUCH_REPORT_CONFIG = 0x26,
	CMD_REZERO = 0x27,
	CMD_COMMIT_CONFIG = 0x28,
	CMD_DESCRIBE_DYNAMIC_CONFIG = 0x29,
	CMD_PRODUCTION_TEST = 0x2a,
	CMD_SET_CONFIG_ID = 0x2b,
	CMD_ENTER_DEEP_SLEEP = 0x2c,
	CMD_EXIT_DEEP_SLEEP = 0x2d,
	CMD_GET_TOUCH_INFO = 0x2e,
	CMD_GET_DATA_LOCATION = 0x2f,
	CMD_DOWNLOAD_CONFIG = 0x30,
	CMD_GET_TP_ABNORMAL_STATE = 0xc5,
};

struct syna_tcm_abnormal_info {
	union {
		struct {
			unsigned char gnd_connection:1;
			unsigned char tx_sns_ch:1;
			unsigned char rx_sns_ch:1;
			unsigned char pixel_sns:1;
			unsigned char display_noise:1;
			unsigned char charge_noise:1;
			unsigned char charge_noise_hop:1;
			unsigned char charge_noise_ex:1;
			unsigned char self_cap_noise:1;
			unsigned char mutual_cap_noise:1;
			unsigned char high_temp:1;
			unsigned char low_temp:1;
			unsigned char large_bending:1;
			unsigned char reserved:3;
		}__packed;
		unsigned char data[2];
	};
};

enum {
	BIT0_GND_CONNECTION = 0,
	BIT1_TX_SNS_CH,
	BIT2_RX_SNS_CH,
	BIT3_PIXEL_SNS,
	BIT4_DISPLAY_NOISE,
	BIT5_CHARGER_NOISE,
	BIT6_CHARGER_NOISE_HOP,
	BIT7_CHARGER_NOISE_EX,
	BIT8_SELF_CAP_NOISE,
	BIT9_MUTUAL_CAP_NOISE,
	BIT10_HIGH_TEMP,
	BIT11_LOW_TEMP,
	BIT12_LARGE_BENDING,
	BIT13_RESERVED,
	BIT14_RESERVED,
	BIT15_RESERVED,
	BIT_MAX,
};
struct dmd_report_charger_status{
	int charge_CHARGER_NOISE_HOP;
	int charge_CHARGER_NOISE_EX;
};

struct tp_status_and_count{
	int bit_status;
	unsigned int bit_count;
};

enum status_code {
	STATUS_IDLE = 0x00,
	STATUS_OK = 0x01,
	STATUS_BUSY = 0x02,
	STATUS_CONTINUED_READ = 0x03,
	STATUS_NOT_EXECUTED_IN_DEEP_SLEEP = 0x0b,
	STATUS_RECEIVE_BUFFER_OVERFLOW = 0x0c,
	STATUS_PREVIOUS_COMMAND_PENDING = 0x0d,
	STATUS_NOT_IMPLEMENTED = 0x0e,
	STATUS_ERROR = 0x0f,
	STATUS_INVALID = 0xff,
};

enum report_type {
	REPORT_IDENTIFY = 0x10,
	REPORT_TOUCH = 0x11,
	REPORT_DELTA = 0x12,
	REPORT_RAW = 0x13,
	REPORT_STATUS = 0x1b,
	REPORT_PRINTF = 0x82,
	REPORT_HDL = 0xfe,
};

enum command_status {
	CMD_IDLE = 0,
	CMD_BUSY = 1,
	CMD_ERROR = -1,
};

enum flash_area {
	BOOTLOADER = 0,
	BOOT_CONFIG,
	APP_FIRMWARE,
	APP_CONFIG,
	DISP_CONFIG,
	CUSTOM_OTP,
	CUSTOM_LCM,
	CUSTOM_OEM,
	PPDT,
};

enum flash_data {
	LCM_DATA = 1,
	OEM_DATA,
	PPDT_DATA,
};

enum helper_task {
	HELP_NONE = 0,
	HELP_RUN_APPLICATION_FIRMWARE,
	HELP_SEND_RESET_NOTIFICATION,
};

enum syna_tcm_ic_type {
	SYNA_TCM_TD4320 =0,
};

struct syna_tcm_helper {
	atomic_t task;
	struct work_struct work;
	struct workqueue_struct *workqueue;
};

struct syna_tcm_watchdog {
	bool run;
	unsigned char count;
	struct delayed_work work;
	struct workqueue_struct *workqueue;
};

struct syna_tcm_buffer {
	bool clone;
	unsigned char *buf;
	unsigned int buf_size;
	unsigned int data_length;
	struct mutex buf_mutex;
};

struct syna_tcm_report {
	unsigned char id;
	struct syna_tcm_buffer buffer;
};

struct syna_tcm_identification {
	unsigned char version;
	unsigned char mode;
	unsigned char part_number[16];
	unsigned char build_id[4];
	unsigned char max_write_size[2];
};

struct syna_tcm_boot_info {
	unsigned char version;
	unsigned char status;
	unsigned char asic_id[2];
	unsigned char write_block_size_words;
	unsigned char erase_page_size_words[2];
	unsigned char max_write_payload_size[2];
	unsigned char last_reset_reason;
	unsigned char pc_at_time_of_last_reset[2];
	unsigned char boot_config_start_block[2];
	unsigned char boot_config_size_blocks[2];
	unsigned char display_config_start_block[4];
	unsigned char display_config_length_blocks[2];
	unsigned char backup_display_config_start_block[4];
	unsigned char backup_display_config_length_blocks[2];
	unsigned char custom_otp_start_block[2];
	unsigned char custom_otp_length_blocks[2];
};

struct syna_tcm_app_info {
	unsigned char version[2];
	unsigned char status[2];
	unsigned char static_config_size[2];
	unsigned char dynamic_config_size[2];
	unsigned char app_config_start_write_block[2];
	unsigned char app_config_size[2];
	unsigned char max_touch_report_config_size[2];
	unsigned char max_touch_report_payload_size[2];
	unsigned char customer_config_id[16];
	unsigned char max_x[2];
	unsigned char max_y[2];
	unsigned char max_objects[2];
	unsigned char num_of_buttons[2];
	unsigned char num_of_image_rows[2];
	unsigned char num_of_image_cols[2];
	unsigned char has_hybrid_data[2];
};

struct syna_tcm_touch_info {
	unsigned char image_2d_scale_factor[4];
	unsigned char image_0d_scale_factor[4];
	unsigned char hybrid_x_scale_factor[4];
	unsigned char hybrid_y_scale_factor[4];
};

struct syna_tcm_message_header {
	unsigned char marker;
	unsigned char code;
	unsigned char length[2];
};

/*
 * struct synaptics_rmi4_device_info - device information
 * @version_major: rmi protocol major version number
 * @version_minor: rmi protocol minor version number
 * @manufacturer_id: manufacturer id
 * @product_props: product properties information
 * @product_info: product info array
 * @date_code: device manufacture date
 * @tester_id: tester id array
 * @serial_number: device serial number
 * @product_id_string: device product id
 * @support_fn_list: linked list for function handlers
 */
struct syna_tcm_device_info {
	unsigned int version_major;
	unsigned int version_minor;
	unsigned char manufacturer_id;
	unsigned char product_props;
	unsigned char product_info[SYNAPTICS_RMI4_PRODUCT_INFO_SIZE];
	unsigned char date_code[SYNAPTICS_RMI4_DATE_CODE_SIZE];
	unsigned short tester_id;
	unsigned short serial_number;
	unsigned char product_id_string[SYNAPTICS_RMI4_PRODUCT_ID_SIZE + 1];
	unsigned char project_id_string[SYNAPTICS_RMI4_PROJECT_ID_SIZE + 1];
	unsigned char build_id[SYNAPTICS_RMI4_BUILD_ID_SIZE];
	unsigned char ic_name[SYNAPTICS_RMI4_IC_NAME_SIZE];
	unsigned char config_id[4];
	unsigned char synaptics_build_id[SYNAPTICS_BUILD_ID_NO];
	unsigned char synaptics_ic_name[SYNAPTICS_IC_NAME_NO];
	unsigned char device_config_id[CHIP_INFO_LENGTH];
	unsigned char image_config_id[CHIP_INFO_LENGTH];
	struct list_head support_fn_list;
};

struct syna_tcm_hcd {
	struct ts_kit_device_data *syna_tcm_chip_data;
	struct syna_tcm_device_info tcm_mod_info;
	struct regulator *syna_tcm_tp_vci;
	struct regulator *syna_tcm_tp_vddio;
	struct pinctrl *pctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_idle;
	pid_t isr_pid;
	atomic_t command_status;
	atomic_t host_downloading;
	wait_queue_head_t hdl_wq;
	int irq;
	bool init_okay;
	bool do_polling;
	bool in_suspend;
	bool irq_enabled;
	bool host_download_mode;
	u16 roi_enable_status;
	u16 holster_enable_status;
	unsigned char *syna_tcm_roi_data;
	unsigned char fb_ready;
	unsigned char command;
	unsigned char async_report_id;
	unsigned char status_report_code;
	unsigned char response_code;
	unsigned int read_length;
	unsigned int payload_length;
	unsigned int packrat_number;
	unsigned int rd_chunk_size;
	unsigned int wr_chunk_size;
	unsigned int app_status;
	unsigned int tp_status_report_support;
	unsigned int device_status;
	bool checking_abnormal_state;
	bool in_before_suspend;
	bool device_status_check;
	bool in_suspend_charge;
	struct syna_tcm_abnormal_info ab_device_status;
	struct platform_device *pdev;
	struct regulator *pwr_reg;
	struct regulator *bus_reg;
	struct kobject *sysfs_dir;
	struct kobject *dynamnic_config_sysfs_dir;
	struct mutex extif_mutex;
	struct delayed_work polling_work;
	struct workqueue_struct *polling_workqueue;
	struct task_struct *notifier_thread;
#ifdef CONFIG_FB
	struct notifier_block fb_notifier;
#endif
	struct syna_tcm_buffer in;
	struct syna_tcm_buffer out;
	struct syna_tcm_buffer resp;
	struct syna_tcm_buffer temp;
	struct syna_tcm_buffer config;
	struct syna_tcm_report report;
	struct syna_tcm_app_info app_info;
	struct syna_tcm_boot_info boot_info;
	struct syna_tcm_touch_info touch_info;
	struct syna_tcm_identification id_info;
	struct syna_tcm_helper helper;
	struct syna_tcm_watchdog watchdog;
	struct syna_tcm_board_data *bdata;
	const struct syna_tcm_hw_interface *hw_if;
	char fw_name[MAX_STR_LEN * 4];
	unsigned int aft_wxy_enable;
	unsigned int esd_report_status;
	unsigned int use_esd_report;
	unsigned int use_dma_download_firmware;
	unsigned int downmload_firmware_frequency;
	unsigned int spi_comnunicate_frequency;
	unsigned int resume_retry_download_fw_support;
	unsigned int retry_download_delay_time;
	unsigned int retry_download_retry_times;
	int (*reset)(struct syna_tcm_hcd *tcm_hcd, bool hw, bool update_wd);
	int (*sleep)(struct syna_tcm_hcd *tcm_hcd, bool en);
	int (*identify)(struct syna_tcm_hcd *tcm_hcd, bool id);
	int (*enable_irq)(struct syna_tcm_hcd *tcm_hcd, bool en, bool ns);
	int (*switch_mode)(struct syna_tcm_hcd *tcm_hcd,
			enum firmware_mode mode);
	int (*read_message)(struct syna_tcm_hcd *tcm_hcd,
			unsigned char *in_buf, unsigned int length);
	int (*write_message)(struct syna_tcm_hcd *tcm_hcd,
			unsigned char command, unsigned char *payload,
			unsigned int length, unsigned char **resp_buf,
			unsigned int *resp_buf_size, unsigned int *resp_length,
			unsigned char *response_code,
			unsigned int polling_delay_ms);
	int (*get_dynamic_config)(struct syna_tcm_hcd *tcm_hcd,
			enum dynamic_config_id id, unsigned short *value);
	int (*set_dynamic_config)(struct syna_tcm_hcd *tcm_hcd,
			enum dynamic_config_id id, unsigned short value);
	int (*get_data_location)(struct syna_tcm_hcd *tcm_hcd,
			enum flash_area area, unsigned int *addr,
			unsigned int *length);
	int (*read_flash_data)(enum flash_area area, bool run_app_firmware,
			struct syna_tcm_buffer *output);
	void (*report_touch)(struct ts_fingers *info);
	void (*update_watchdog)(struct syna_tcm_hcd *tcm_hcd, bool en);
	int (*rmi_read)(struct syna_tcm_hcd *tcm_hcd, unsigned short addr,
			unsigned char *data, unsigned int length);
	int (*rmi_write)(struct syna_tcm_hcd *tcm_hcd, unsigned short addr,
			unsigned char *data, unsigned int length);
	int (*read)(struct syna_tcm_hcd *tcm_hcd, unsigned char *data,
			unsigned int length);
	int (*write)(struct syna_tcm_hcd *tcm_hcd, unsigned char *data,
			unsigned int length);
};

struct syna_tcm_module_handler {
	bool insert;
	bool detach;
	struct list_head link;
	struct syna_tcm_module_cb *mod_cb;
};

struct syna_tcm_module_pool {
	bool initialized;
	bool queue_work;
	struct mutex mutex;
	struct list_head list;
	struct work_struct work;
	struct workqueue_struct *workqueue;
	struct syna_tcm_hcd *tcm_hcd;
};

int syna_tcm_bus_init(void);

void syna_tcm_bus_exit(void);

int syna_tcm_add_module(struct syna_tcm_module_cb *mod_cb, bool insert);

static inline int syna_tcm_rmi_read(struct syna_tcm_hcd *tcm_hcd,
		unsigned short addr, unsigned char *data, unsigned int length)
{
	return tcm_hcd->rmi_read(tcm_hcd, addr, data, length);
}

static inline int syna_tcm_rmi_write(struct syna_tcm_hcd *tcm_hcd,
		unsigned short addr, unsigned char *data, unsigned int length)
{
	return tcm_hcd->rmi_write(tcm_hcd, addr, data, length);
}

static inline int syna_tcm_read(struct syna_tcm_hcd *tcm_hcd,
		unsigned char *data, unsigned int length)
{
	return tcm_hcd->read(tcm_hcd, data, length);
}

static inline int syna_tcm_write(struct syna_tcm_hcd *tcm_hcd,
		unsigned char *data, unsigned int length)
{
	return tcm_hcd->write(tcm_hcd, data, length);
}

static inline ssize_t syna_tcm_show_error(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	pr_err("%s: Attribute not readable\n", __func__);

	return -EPERM;
}

static inline ssize_t syna_tcm_store_error(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	pr_err("%s: Attribute not writable\n", __func__);

	return -EPERM;
}

static inline int secure_memcpy(unsigned char *dest, unsigned int dest_size,
		const unsigned char *src, unsigned int src_size,
		unsigned int count)
{
	if (dest == NULL || src == NULL)
		return -EINVAL;

	if (count > dest_size || count > src_size) {
		pr_err("%s: src_size = %d, dest_size = %d, count = %d\n",
				__func__, src_size, dest_size, count);
		return -EINVAL;
	}

	memcpy((void *)dest, (const void *)src, count);

	return 0;
}

static inline int syna_tcm_realloc_mem(struct syna_tcm_hcd *tcm_hcd,
		struct syna_tcm_buffer *buffer, unsigned int size)
{
	int retval;
	unsigned char *temp;

	if (size > buffer->buf_size) {
		temp = buffer->buf;

		buffer->buf = kmalloc(size, GFP_KERNEL);
		if (!(buffer->buf)) {
			dev_err(tcm_hcd->pdev->dev.parent,
					"%s: Failed to allocate memory\n",
					__func__);
			kfree(temp);
			buffer->buf_size = 0;
			return -ENOMEM;
		}

		retval = secure_memcpy(buffer->buf,
				size,
				temp,
				buffer->buf_size,
				buffer->buf_size);
		if (retval < 0) {
			dev_err(tcm_hcd->pdev->dev.parent,
					"%s: Failed to copy data\n",
					__func__);
			kfree(temp);
			kfree(buffer->buf);
			buffer->buf_size = 0;
			return retval;
		}

		kfree(temp);
		buffer->buf_size = size;
	}

	return 0;
}

static inline int syna_tcm_alloc_mem(struct syna_tcm_hcd *tcm_hcd,
		struct syna_tcm_buffer *buffer, unsigned int size)
{
	if (size > buffer->buf_size) {
		kfree(buffer->buf);
		buffer->buf = kmalloc(size, GFP_KERNEL);
		if (!(buffer->buf)) {
			dev_err(tcm_hcd->pdev->dev.parent,
					"%s: Failed to allocate memory\n",
					__func__);
			dev_err(tcm_hcd->pdev->dev.parent,
					"%s: Allocation size = %d\n",
					__func__, size);
			buffer->buf_size = 0;
			buffer->data_length = 0;
			return -ENOMEM;
		}
		buffer->buf_size = size;
	}

	memset(buffer->buf, 0x00, buffer->buf_size);
	buffer->data_length = 0;

	return 0;
}

static inline unsigned int le2_to_uint(const unsigned char *src)
{
	return (unsigned int)src[0] +
			(unsigned int)src[1] * 0x100;
}

static inline unsigned int le4_to_uint(const unsigned char *src)
{
	return (unsigned int)src[0] +
			(unsigned int)src[1] * 0x100 +
			(unsigned int)src[2] * 0x10000 +
			(unsigned int)src[3] * 0x1000000;
}

static inline unsigned int ceil_div(unsigned int dividend, unsigned divisor)
{
	return (dividend + divisor - 1) / divisor;
}

extern int zeroflash_init(struct syna_tcm_hcd *tcm_hcd);
extern int zeroflash_get_fw_image(char *file_name);
extern int zeroflash_remove(struct syna_tcm_hcd *tcm_hcd);
extern int zeroflash_download(char *file_name,struct syna_tcm_hcd *tcm_hcd);
extern int touch_init(struct syna_tcm_hcd *tcm_hcd);
extern void touch_report(struct ts_fingers *info);
extern int debug_device_init(struct syna_tcm_hcd *tcm_hcd);
extern int syna_tcm_write_hdl_message(struct syna_tcm_hcd *tcm_hcd,
		unsigned char command, unsigned char *payload,
		unsigned int length, unsigned char **resp_buf,
		unsigned int *resp_buf_size, unsigned int *resp_length,
		unsigned char *response_code, unsigned int polling_delay_ms);
extern int syna_tcm_cap_test_init(struct syna_tcm_hcd *tcm_hcd);
extern int syna_tcm_cap_test(struct ts_rawdata_info_new *info,
			struct ts_cmd_node *out_cmd);
extern int syna_tcm_enable_touch(struct syna_tcm_hcd *tcm_hcd, bool en);
#endif
