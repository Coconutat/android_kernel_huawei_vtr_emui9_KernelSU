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

#include <linux/of_gpio.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/input/mt.h>

#include "synaptics_tcm_core.h"

#include "../../huawei_ts_kit_algo.h"
#include "../../tpkit_platform_adapter.h"
#include "../../huawei_ts_kit_api.h"

#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif
#if defined (CONFIG_TEE_TUI)
#include "tui.h"
#endif
#include "../../huawei_ts_kit.h"

#define ABNORMAL_STATE_LEN 9

static int syna_tcm_hdl_get_state(void);

#define SYNAPTICS_CHIP_INFO  "synaptics"
#define RESET_ON_RESUME
#define RESET_ON_RESUME_DELAY_MS 20
#define PREDICTIVE_READING
#define MIN_READ_LENGTH 9
#define KEEP_DRIVER_ON_ERROR
#define FORCE_RUN_APPLICATION_FIRMWARE
#define NOTIFIER_PRIORITY 2
#define NOTIFIER_TIMEOUT_MS 500
#define RESPONSE_TIMEOUT_MS 3000
#define APP_STATUS_POLL_TIMEOUT_MS 1000
#define APP_STATUS_POLL_MS 100
#define ENABLE_IRQ_DELAY_MS 20
#define FALL_BACK_ON_POLLING
#define POLLING_DELAY_MS 5
#define RUN_WATCHDOG true
#define WATCHDOG_TRIGGER_COUNT 2
#define WATCHDOG_DELAY_MS 1000
#define MODE_SWITCH_DELAY_MS 100
#define READ_RETRY_US_MIN 5000
#define READ_RETRY_US_MAX 10000
#define WRITE_DELAY_US_MIN 500
#define WRITE_DELAY_US_MAX 1000
#define HOST_DOWNLOAD_WAIT_MS 100
#define HOST_DOWNLOAD_TIMEOUT_MS 1000
#define DYNAMIC_CONFIG_SYSFS_DIR_NAME "dynamic_config"
#define PRODUCT_ID_FW_LEN 5
#define PROJECT_ID_FW_LEN 9
#define SYNAPTICS_TEST_TYPE "tp_test_type"
#define FW_IMAGE_NAME_SD "ts/S3706.img"
#define SYNA_VENDOR_NAME "synaptics_tcm"

#define SPI_MAX_SPEED_READ		10000000
#define SPI_MAX_SPEED_WRITE		10000000
#define SPI_MAX_SPEED_RMI_READ	100000
#define SPI_MAX_SPEED_RMI_WRITE	10000000

#define I2C_RETRY_DELAY_TIME_MS	20

#define GPIO_OUTPUT_HIGH	1
#define GPIO_OUTPUT_LOW		0

#define CHARGE_REPORT_DONE		  	0
#define CHARGE_REPORT_NOT_REPORT	1

static int syna_report_priority[BIT_MAX] = {0,3,8,7,12,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static int syna_report_priority_limite[BIT_MAX] = {50,20,20,20,20,50,50,50,20,20,20,20,50,50,50,50};
struct tp_status_and_count tp_status_dmd_bit_status[BIT_MAX];
static struct dmd_report_charger_status syna_dmd_charge_info;

static void syna_tcm_report_dmd_state(int dmd_bit);

struct syna_tcm_hcd *tcm_hcd = NULL;
struct syna_tcm_board_data *bdata = NULL;
extern struct ts_kit_platform_data g_ts_kit_platform_data;
static int touch_init_status = true;

static unsigned char *buf = NULL;
static unsigned int buf_size = 0;
static unsigned char buffer[FIXED_READ_LENGTH] = {0};
static u8 pre_finger_status = 0;
extern struct ts_kit_platform_data g_ts_kit_platform_data;
DECLARE_COMPLETION(response_complete);

#if defined (CONFIG_TEE_TUI)
extern struct ts_tui_data tee_tui_data;
#endif
extern int tp_i2c_get_hwlock(void);
extern void tp_i2c_release_hwlock(void);

static int syna_tcm_comm_check(void);
static int syna_tcm_mmi_test(struct ts_rawdata_info *info,
				 struct ts_cmd_node *out_cmd);
//static int syna_tcm_chip_get_info(struct ts_chip_info_param *info);
static int syna_tcm_fw_update_boot(char *file_name);
static int syna_tcm_fw_update_sd(void);
static int syna_tcm_before_suspend(void);
static int syna_tcm_suspend(void);
static int syna_tcm_resume(void);
static int syna_tcm_after_resume(void *feature_info);
static int syna_tcm_set_info_flag(struct ts_kit_platform_data *info);
static int syna_tcm_irq_bottom_half(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd);
static int syna_tcm_irq_top_half(struct ts_cmd_node *cmd);
static int syna_tcm_input_config(struct input_dev * input_dev);
static int syna_tcm_parse_dts(struct device_node *np, struct ts_kit_device_data *chip_data);
//static int syna_tcm_get_brightness_info(void);
static int syna_tcm_init_chip(void);
static int syna_tcm_chip_detect(struct ts_kit_platform_data* data);
static int synaptics_tcm_chip_get_capacitance_test_type(struct ts_test_type_info *info);
static int syna_tcm_roi_switch(struct ts_roi_info *info);
static unsigned char *syna_tcm_roi_rawdata(void);
static int syna_tcm_charger_switch(struct ts_charger_info *info);
static int syna_tcm_glove_switch(struct ts_glove_info *info);
static int syna_tcm_status_resume(void);
extern int hostprocessing_get_project_id(char *out);
static int synaptics_tcm_chip_get_info(struct ts_chip_info_param *info);
static void synaptics_tcm_chip_touch_switch(void);

struct ts_device_ops ts_kit_syna_tcm_ops = {
	.chip_detect = syna_tcm_chip_detect,
	.chip_init = syna_tcm_init_chip,
//	.chip_get_brightness_info = syna_tcm_get_brightness_info,
	.chip_parse_config = syna_tcm_parse_dts,
	.chip_input_config = syna_tcm_input_config,
	.chip_irq_top_half = syna_tcm_irq_top_half,
	.chip_irq_bottom_half = syna_tcm_irq_bottom_half,
	.chip_fw_update_boot = syna_tcm_fw_update_boot,  // host download on boot
	.chip_fw_update_sd = syna_tcm_fw_update_sd,   // host download by hand
//	.oem_info_switch = synaptics_oem_info_switch,
	.chip_get_info = synaptics_tcm_chip_get_info,
	.chip_get_capacitance_test_type =
		synaptics_tcm_chip_get_capacitance_test_type,
	.chip_set_info_flag = syna_tcm_set_info_flag,
	.chip_before_suspend = syna_tcm_before_suspend,
	.chip_suspend = syna_tcm_suspend,
	.chip_resume = syna_tcm_resume,
	.chip_after_resume = syna_tcm_after_resume,
//	.chip_wakeup_gesture_enable_switch =
//		synaptics_wakeup_gesture_enable_switch,
	.chip_get_rawdata = syna_tcm_mmi_test,
//	.chip_get_calibration_data = synaptics_get_calibration_data,
//	.chip_get_calibration_info = synaptics_get_calibration_info,
//	.chip_get_debug_data = synaptics_get_debug_data,
	.chip_glove_switch = syna_tcm_glove_switch,
//	.chip_shutdown = synaptics_shutdown,
	.chip_charger_switch = syna_tcm_charger_switch,
//	.chip_holster_switch = synaptics_holster_switch,
	.chip_roi_switch = syna_tcm_roi_switch,
	.chip_roi_rawdata = syna_tcm_roi_rawdata,
//	.chip_palm_switch = synaptics_palm_switch,
//	.chip_regs_operate = synaptics_regs_operate,
//	.chip_calibrate = synaptics_calibrate,
//	.chip_calibrate_wakeup_gesture = synaptics_calibrate_wakeup_gesture,
//	.chip_reset = synaptics_reset_device,
//#ifdef HUAWEI_TOUCHSCREEN_TEST
//	.chip_test = test_dbg_cmd_test,
//#endif
//	.chip_wrong_touch = synaptics_wrong_touch,
//	.chip_work_after_input = synaptics_work_after_input_kit,
//	.chip_ghost_detect = synaptics_ghost_detect,
//	.chip_check_status = synaptics_chip_check_status,
	.chip_touch_switch = synaptics_tcm_chip_touch_switch,
};

static void syna_tcm_report_dmd_state_report(void)
{
	int i = 0;
	uint16_t buf[2] = {0};
	unsigned long abnormal_status;
	int report_dmd_count = 0;
	int count = 0;
	static unsigned int report_index = 0;

	buf[0] = (uint16_t)tcm_hcd->ab_device_status.data[0];
	buf[1] = (uint16_t)tcm_hcd->ab_device_status.data[1];
	abnormal_status = (unsigned long)((buf[1] << 8) | buf[0]);

	TS_LOG_INFO("%s, input value is %x  buf[0] = %d buf[1] = %d",__func__, abnormal_status, buf[0], buf[1]);

	for(i=0; i < BIT_MAX; i++) {
		if(0xFF != syna_report_priority[i] && BIT15_RESERVED >= syna_report_priority[i]) {// check same value
			count++;
			if(test_bit(syna_report_priority[i], (unsigned long*)&abnormal_status) && syna_report_priority_limite[i]) {
				if(BIT6_CHARGER_NOISE_HOP == syna_report_priority[i] && CHARGE_REPORT_DONE != syna_dmd_charge_info.charge_CHARGER_NOISE_HOP) {
					syna_dmd_charge_info.charge_CHARGER_NOISE_HOP = CHARGE_REPORT_DONE;
					tp_status_dmd_bit_status[i].bit_count++;
					syna_report_priority_limite[i]--;
				} else if (BIT7_CHARGER_NOISE_EX== syna_report_priority[i] && CHARGE_REPORT_DONE != syna_dmd_charge_info.charge_CHARGER_NOISE_EX) {
					syna_dmd_charge_info.charge_CHARGER_NOISE_EX = CHARGE_REPORT_DONE;
					tp_status_dmd_bit_status[i].bit_count++;
					syna_report_priority_limite[i]--;
				} else if(BIT6_CHARGER_NOISE_HOP != syna_report_priority[i] && BIT7_CHARGER_NOISE_EX != syna_report_priority[i]) {
					tp_status_dmd_bit_status[i].bit_count++;
					syna_report_priority_limite[i]--;
				}
			}
			TS_LOG_INFO("tp_status_dmd_bit_status  after [%d] = report_priority = %d status %d count = %d , limite %d", i, syna_report_priority[i],
				tp_status_dmd_bit_status[i].bit_status, tp_status_dmd_bit_status[i].bit_count , syna_report_priority_limite[i]);
		} else {
			break;
		}
	}
	for(i = 0; i < count; i++) {
		if(tp_status_dmd_bit_status[report_index].bit_count) {
			tp_status_dmd_bit_status[report_index].bit_count--;
			syna_tcm_report_dmd_state(syna_report_priority[report_index]);
			if(count - 1 > report_index) {
				report_index++;
			} else {
				report_index = 0;
			}
			return;
		} else if (count - 1 > report_index){
			report_index++;
		} else  if (count - 1 == report_index){
			report_index = 0;
		}
	}
}
static void syna_tcm_report_dmd_state(int dmd_bit)
{
	TS_LOG_INFO("%s, input bit is %d",__func__, dmd_bit);
#if defined (CONFIG_HUAWEI_DSM)
	switch(dmd_bit) {
		case BIT0_GND_CONNECTION:
			ts_dmd_report(DSM_TP_GND_CONNECTION_ABNORMAL, "TP_GND_CONNECTION_ABNORMAL, ");
			break;
		case BIT1_TX_SNS_CH:
			ts_dmd_report(DSM_TP_TX_SNS_ABNORMAL, "TP_TX_SNS_ABNORMAL, ");
			break;
		case BIT2_RX_SNS_CH:
			ts_dmd_report(DSM_TP_RX_SNS_ABNORMAL, "TP_RX_SNS_ABNORMAL, ");
			break;
		case BIT3_PIXEL_SNS:
			ts_dmd_report(DSM_TP_PIXEL_SNS_ABNORMAL, "TP_PIXEL_SNS_ABNORMAL, ");
			break;
		case BIT4_DISPLAY_NOISE:
			ts_dmd_report(DSM_TP_DISPLAY_LARGE_NOISE, "TP_DISPLAY_LARGE_NOISE, ");
			break;
		case BIT5_CHARGER_NOISE:
			TS_LOG_INFO("report charge noise\n");
			break;
		case BIT6_CHARGER_NOISE_HOP:
			ts_dmd_report(DSM_TP_CHARGER_NOISE_HOP, "TP_CHARGER_NOISE_HOP, ");
			break;
		case BIT7_CHARGER_NOISE_EX:
			ts_dmd_report(DSM_TP_CHARGER_NOISE_EX, "TP_CHARGER_NOISE_EX, ");
			break;
		case BIT8_SELF_CAP_NOISE:
			ts_dmd_report(DSM_TP_SELF_CAP_LARGE_NOISE, "TP_SELF_CAP_LARGE_NOISE, ");
			break;
		case BIT9_MUTUAL_CAP_NOISE:
			ts_dmd_report(DSM_TP_MUTUAL_CAP_LARGE_NOISE, "TP_MUTUAL_CAP_LARGE_NOISE, ");
			break;
		case BIT10_HIGH_TEMP:
			ts_dmd_report(DSM_TP_HIGH_TEMP_MODE, "TP_HIGH_TEMP_MODE, ");
			break;
		case BIT11_LOW_TEMP:
			ts_dmd_report(DSM_TP_LOW_TEMP_MODE, "TP_LOW_TEMP_MODE, ");
			break;
		case BIT12_LARGE_BENDING:
			ts_dmd_report(DSM_TP_LARGE_BENDING, "TP_LARGE_BENDING, ");
			break;
		case BIT13_RESERVED:
		case BIT14_RESERVED:
		case BIT15_RESERVED:
			break;
		default:
			TS_LOG_ERR("error code %d", dmd_bit);
			break;
	}
#endif
}
static int syna_tcm_hdl_get_state(void)
{
	int retval = NO_ERR;
	unsigned char *temp_buf;
	int retry = 20;

	tcm_hcd->ab_device_status.data[0] = 0xff;
	tcm_hcd->ab_device_status.data[1] = 0xff;

	temp_buf = kzalloc(ABNORMAL_STATE_LEN, GFP_KERNEL);
	if (!temp_buf) {
		TS_LOG_ERR(
				"Failed to allocate memory for temp_buf\n");
		return -ENOMEM;
	}

	retval = syna_tcm_write_hdl_message(tcm_hcd,
			CMD_GET_TP_ABNORMAL_STATE,
			NULL,
			0,
			&tcm_hcd->config.buf,
			&tcm_hcd->config.buf_size,
			&tcm_hcd->config.data_length,
			NULL,
			0);

	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to write command %s\n",
				STR(CMD_GET_TP_ABNORMAL_STATE));
		goto exit;
	}

	if (tcm_hcd->in_before_suspend) {
		while (retry) {
			mdelay(1);
			retval = syna_tcm_read(tcm_hcd,
					temp_buf,
					ABNORMAL_STATE_LEN);
			if (temp_buf[1] != STATUS_OK)
				TS_LOG_INFO("temp_buf: 0x%02x 0x%02x 0x%02x 0x%02x ", temp_buf[0],temp_buf[1],temp_buf[2],temp_buf[3]);
			else
				break;
			retry --;
		}

		TS_LOG_INFO("%s temp_buf = %x,%x,%x,%x,%x,%x.\n", __func__, temp_buf[0], temp_buf[1], temp_buf[2], temp_buf[3], temp_buf[4], temp_buf[5]);

		if (temp_buf[1] != STATUS_OK) {
			tcm_hcd->ab_device_status.data[0] = 0x00;
			tcm_hcd->ab_device_status.data[1] = 0x00;
			// abnormal state read failed
			// abnormal state includes two bytes
			// temp[4] is 0x0b as default
			// temp[5] is 0x0a as default

			goto exit;
		} else {
			tcm_hcd->ab_device_status.data[0] = temp_buf[4];
			tcm_hcd->ab_device_status.data[1] = temp_buf[5];
			// abnormal state read failed
			// abnormal state includes two bytes
			// temp[4] is 0x0b as default
			// temp[5] is 0x0a as default
		}
	}
exit:
	kfree(temp_buf);
	return retval;
}

static int synaptics_tcm_get_project_id(char *project_id)
{
	int retval = NO_ERR;
	int projectid_lenth = 0;
	struct lcd_kit_ops *tp_ops = lcd_kit_get_ops();

	if (!tcm_hcd || !tcm_hcd->syna_tcm_chip_data)
		return -EINVAL;

	if (tcm_hcd->syna_tcm_chip_data->projectid_len) {
		projectid_lenth = tcm_hcd->syna_tcm_chip_data->projectid_len;
	} else {
		projectid_lenth = PROJECT_ID_FW_LEN;
	}
	/* 3_0 */
	if(tp_ops && tp_ops->get_project_id) {
		retval = tp_ops->get_project_id(project_id);
	}else{
		retval = -EINVAL;
		TS_LOG_ERR("%s:get lcd_kit_get_ops fail\n", __func__);
	}
	TS_LOG_ERR("Read projectid from lcd is %s\n", project_id);

	if (bdata->default_project_id) {
		if (strncmp(project_id, bdata->default_project_id, PRODUCT_ID_FW_LEN-1)) {
			TS_LOG_INFO("%s: Get project id from lcd error, use default: %s\n",
					__func__, bdata->default_project_id);
			memcpy(project_id, bdata->default_project_id, projectid_lenth);
			return 0;
		}
	}
	project_id[PROJECT_ID_FW_LEN] = 0;

	return retval;
}

static int synaptics_tcm_chip_get_info(struct ts_chip_info_param *info)
{
	int retval = 0;
	char buf_proj_id[CHIP_INFO_LENGTH] = {0};
	char buf_fw_ver[CHIP_INFO_LENGTH] = {0};
	int projectid_lenth = 0;
	int len = 0;

	if (!tcm_hcd)
		return -EINVAL;

	if (tcm_hcd->syna_tcm_chip_data->projectid_len) {
		projectid_lenth = tcm_hcd->syna_tcm_chip_data->projectid_len;
	} else {
		projectid_lenth = PROJECT_ID_FW_LEN;
	}
	memset(info->ic_vendor, 0, sizeof(info->ic_vendor));
	memset(info->mod_vendor, 0, sizeof(info->mod_vendor));
	memset(info->fw_vendor, 0, sizeof(info->fw_vendor));

	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		retval = synaptics_tcm_get_project_id(buf_proj_id);
		if (retval) {
			TS_LOG_ERR("%s get project id error.\n", __func__);
			return -EINVAL;
		}
	} else {
		len = MIN(sizeof(buf_proj_id) - 1, strlen(tcm_hcd->tcm_mod_info.project_id_string));
		memcpy(buf_proj_id, tcm_hcd->tcm_mod_info.project_id_string, len);
	}

	snprintf(buf_fw_ver, CHIP_INFO_LENGTH-1, "PR%d", tcm_hcd->packrat_number);
	TS_LOG_INFO("buf_fw_ver = %s", buf_fw_ver);

	if(!tcm_hcd->syna_tcm_chip_data->ts_platform_data->hide_plain_id)  {
		len = MIN(sizeof(info->ic_vendor) - 1, sizeof(SYNAPTICS_CHIP_INFO));
		memcpy(info->ic_vendor, SYNAPTICS_CHIP_INFO, len);
	} else {
		len = MIN(sizeof(info->ic_vendor) - 1, strlen(buf_proj_id));
		memcpy(info->ic_vendor, buf_proj_id, len);
	}

	strncpy(info->mod_vendor, buf_proj_id, CHIP_INFO_LENGTH-1);
	strncpy(info->fw_vendor, buf_fw_ver, CHIP_INFO_LENGTH-1);

	return 0;
}

static int synaptics_tcm_chip_get_capacitance_test_type(struct ts_test_type_info
		*info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR("synaptics_chip_get_capacitance_test_type: info is Null\n");
		return -ENOMEM;
	}
	switch (info->op_action) {
		case TS_ACTION_READ:
			memcpy(info->tp_test_type,
					tcm_hcd->syna_tcm_chip_data->tp_test_type,
					TS_CAP_TEST_TYPE_LEN);
			TS_LOG_INFO("read_chip_get_test_type=%s, \n",
					info->tp_test_type);
			break;
		case TS_ACTION_WRITE:
			break;
		default:
			TS_LOG_ERR("invalid status: %s", info->tp_test_type);
			retval = -EINVAL;
			break;
	}
	return retval;
}

#define XFER_ATTEMPTS 10
#define TEMP_I2C_ADDR 0x2c

static int syna_tcm_i2c_alloc_mem(struct syna_tcm_hcd *tcm_hcd,
		unsigned int size)
{
	//static unsigned int buf_size;
	//struct i2c_client *i2c = tcm_hcd->syna_tcm_chip_data->ts_platform_data->client;

	if (size > buf_size) {
		if (buf_size)
			kfree(buf);
		buf = kmalloc(size, GFP_KERNEL);
		if (!buf) {
			TS_LOG_ERR(
					"Failed to allocate memory for buf\n");
			buf_size = 0;
			return -ENOMEM;
		}
		buf_size = size;
	}

	return 0;
}

static int syna_tcm_i2c_rmi_read(struct syna_tcm_hcd *tcm_hcd,
		unsigned short addr, unsigned char *data, unsigned int length)
{
	int retval = NO_ERR;
	unsigned char address = 0;
	unsigned int attempt = 0;
	struct i2c_msg msg[2] = {0};
	struct i2c_client *i2c = tcm_hcd->syna_tcm_chip_data->ts_platform_data->client;
	//const struct syna_tcm_board_data *bdata = tcm_hcd->hw_if->bdata;

	if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag) {
		retval = tp_i2c_get_hwlock();
		if (retval) {
			TS_LOG_ERR("i2c get hardware mutex failure\n");
			return -EAGAIN;
		}
	}

	address = (unsigned char)addr;

	//msg[0].addr = bdata->ubl_i2c_addr;
	msg[0].addr =TEMP_I2C_ADDR;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &address;

	msg[1].addr = TEMP_I2C_ADDR;
	msg[1].flags = I2C_M_RD;
	msg[1].len = length;
	msg[1].buf = data;

	for (attempt = 0; attempt < XFER_ATTEMPTS; attempt++) {
		if (i2c_transfer(i2c->adapter, msg, 2) == 2) {
			retval = length;
			goto exit;
		}
		TS_LOG_ERR(
				"Transfer attempt %d failed\n",
				attempt + 1);

		if (attempt + 1 == XFER_ATTEMPTS) {
			retval = -EIO;
			goto exit;
		}

		msleep(I2C_RETRY_DELAY_TIME_MS);
	}

exit:
	if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag) {
		tp_i2c_release_hwlock();
	}
	return retval;
}

static int syna_tcm_i2c_rmi_write(struct syna_tcm_hcd *tcm_hcd,
		unsigned short addr, unsigned char *data, unsigned int length)
{
	int retval;
	unsigned int attempt;
	unsigned int byte_count;
	struct i2c_msg msg;
	struct i2c_client *i2c = tcm_hcd->syna_tcm_chip_data->ts_platform_data->client;
	//const struct syna_tcm_board_data *bdata = tcm_hcd->hw_if->bdata;

	if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag) {
		retval = tp_i2c_get_hwlock();
		if (retval) {
			TS_LOG_ERR("i2c get hardware mutex failure\n");
			return -EAGAIN;
		}
	}

	byte_count = length + 1;

	retval = syna_tcm_i2c_alloc_mem(tcm_hcd, byte_count);
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to allocate memory\n");
		goto exit;
	}

	buf[0] = (unsigned char)addr;
	retval = secure_memcpy(&buf[1],
			length,
			data,
			length,
			length);
	if (retval < 0) {
		TS_LOG_ERR(
				"Failed to copy write data\n");
		goto exit;
	}

	//msg.addr = bdata->ubl_i2c_addr;
	msg.addr = TEMP_I2C_ADDR;
	msg.flags = 0;
	msg.len = byte_count;
	msg.buf = buf;

	for (attempt = 0; attempt < XFER_ATTEMPTS; attempt++) {
		if (i2c_transfer(i2c->adapter, &msg, 1) == 1) {
			retval = length;
			goto exit;
		}
		TS_LOG_ERR(
				"Transfer attempt %d failed\n",
				attempt + 1);

		if (attempt + 1 == XFER_ATTEMPTS) {
			retval = -EIO;
			goto exit;
		}

		msleep(I2C_RETRY_DELAY_TIME_MS);
	}

exit:
	if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag) {
		tp_i2c_release_hwlock();
	}

	return retval;
}

static int syna_tcm_i2c_read_data(struct syna_tcm_hcd *tcm_hcd, unsigned char *data,
		unsigned int length)
{
	int retval = NO_ERR;
	unsigned int attempt = 0;
	struct i2c_msg msg = {0};
	struct i2c_client *i2c = tcm_hcd->syna_tcm_chip_data->ts_platform_data->client;

	if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag) {
		retval = tp_i2c_get_hwlock();
		if (retval) {
			TS_LOG_ERR("i2c get hardware mutex failure\n");
			return -EAGAIN;
		}
	}

	msg.addr = i2c->addr;
	msg.flags = I2C_M_RD;
	msg.len = length;
	msg.buf = data;

	for (attempt = 0; attempt < XFER_ATTEMPTS; attempt++) {
		if (i2c_transfer(i2c->adapter, &msg, 1) == 1) {
			retval = length;
			goto exit;
		}
		TS_LOG_ERR(
				"Transfer attempt %d failed\n",
				attempt + 1);

		if (attempt + 1 == XFER_ATTEMPTS) {
			retval = -EIO;
			goto exit;
		}

		msleep(I2C_RETRY_DELAY_TIME_MS);
	}

exit:
	if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag) {
		tp_i2c_release_hwlock();
	}
	return retval;
}

static int syna_tcm_i2c_write(struct syna_tcm_hcd *tcm_hcd, unsigned char *data,
		unsigned int length)
{
	int retval = NO_ERR;
	unsigned int attempt = 0;
	struct i2c_msg msg = {0};
	struct i2c_client *i2c = tcm_hcd->syna_tcm_chip_data->ts_platform_data->client;

	if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag) {
		retval = tp_i2c_get_hwlock();
		if (retval) {
			TS_LOG_ERR("i2c get hardware mutex failure\n");
			return -EAGAIN;
		}
	}

	msg.addr = i2c->addr;
	msg.flags = 0;
	msg.len = length;
	msg.buf = data;

	TS_LOG_ERR("write i2c addr: %x\n",msg.addr);
	for (attempt = 0; attempt < XFER_ATTEMPTS; attempt++) {
		if (i2c_transfer(i2c->adapter, &msg, 1) == 1) {
			retval = length;
			goto exit;
		}
		TS_LOG_ERR(
				"Transfer attempt %d failed\n",
				attempt + 1);

		if (attempt + 1 == XFER_ATTEMPTS) {
			retval = -EIO;
			goto exit;
		}

		msleep(I2C_RETRY_DELAY_TIME_MS);
	}

exit:
	if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag) {
		tp_i2c_release_hwlock();
	}
	return retval;
}

static int syna_tcm_spi_alloc_mem(struct syna_tcm_hcd *tcm_hcd,
		unsigned int size)
{
	if (size > buf_size) {
		if (buf_size)
			kfree(buf);
		buf = kmalloc(size, GFP_KERNEL);
		if (!buf) {
			TS_LOG_ERR("Failed to allocate memory for buf\n");
			buf_size = 0;
			return -ENOMEM;
		}
		buf_size = size;
	}

	return 0;
}

static int syna_tcm_spi_rmi_read_stransfer(struct syna_tcm_hcd *tcm_hcd,
		unsigned short addr, unsigned char *data, unsigned int length)
{
	int retval = NO_ERR;
	struct spi_device *spi = tcm_hcd->syna_tcm_chip_data->ts_platform_data->spi;
	struct spi_transfer xfer[] = {
		{
			.tx_buf = &buf[0],
			.delay_usecs = bdata->ubl_byte_delay_us,
			.len = 2,
			.cs_change = 0,
			.bits_per_word = 8,
		},
		{
			.tx_buf = &buf[2],
			.rx_buf = data,
			.len = length,
		},
	};

	memset(&buf[2], 0xff, length);
	buf[0] = (unsigned char)(addr >> 8) | 0x80;
	buf[1] = (unsigned char)addr;

	spi->max_speed_hz = SPI_MAX_SPEED_RMI_READ;
	retval = spi_setup(spi);
	if (retval) {
		TS_LOG_ERR("%s spi_setup failed, retval = %d.\n", __func__, retval);
		return retval;
	}

	retval = spi_sync_transfer(spi, xfer, ARRAY_SIZE(xfer));
	if (retval == 0) {
		retval = length;
	} else {
		TS_LOG_ERR("Failed to complete SPI transfer, error = %d\n", retval);
	}

	return retval;
}

static int syna_tcm_spi_rmi_read(struct syna_tcm_hcd *tcm_hcd,
		unsigned short addr, unsigned char *data, unsigned int length)
{
	int retval = NO_ERR;

#if defined (CONFIG_TEE_TUI)
	if (tcm_hcd->syna_tcm_chip_data->report_tui_enable) {
		return NO_ERR;
	}
#endif

	retval = syna_tcm_spi_alloc_mem(tcm_hcd, length + 2);
	if (retval < 0) {
		TS_LOG_ERR("Failed to allocate memory\n");
		goto exit;
	}

	retval = syna_tcm_spi_rmi_read_stransfer(tcm_hcd, addr, data, length);
	if (retval < 0) {
		TS_LOG_ERR("Failed to spi rmi read transfer\n");
		goto exit;
	}

exit:
	return retval;
}

static int syna_tcm_spi_rmi_write_transfer(struct syna_tcm_hcd *tcm_hcd,
		unsigned short addr, unsigned char *data, unsigned int length)
{
	int retval = NO_ERR;
	struct spi_device *spi = tcm_hcd->syna_tcm_chip_data->ts_platform_data->spi;
	struct spi_transfer xfer[] = {
		{
			.tx_buf = buf,
			.len = length,
			.cs_change = 0,
			.delay_usecs = bdata->ubl_byte_delay_us,
		},
	};

	buf[0] = (unsigned char)(addr >> 8) & ~0x80;
	buf[1] = (unsigned char)addr;
	retval = secure_memcpy(&buf[2],
			buf_size - 2,
			data,
			length - 2,
			length - 2);
	if (retval < 0) {
		TS_LOG_ERR("Failed to copy write data\n");
		goto exit;
	}

	if(tcm_hcd->use_dma_download_firmware) {
		spi->max_speed_hz = tcm_hcd->spi_comnunicate_frequency;
		spi->controller_data = &g_ts_kit_platform_data.spidev0_chip_info;
	}else{
		spi->max_speed_hz = SPI_MAX_SPEED_RMI_WRITE;
	}
	retval = spi_setup(spi);
	if (retval) {
		TS_LOG_ERR("%s spi setup failed, retval = %d.\n", __func__, retval);
		return retval;
	}

	retval = spi_sync_transfer(spi, xfer, ARRAY_SIZE(xfer));
	if (retval == 0) {
		retval = length;
	} else {
		TS_LOG_ERR("Failed to complete SPI transfer, error = %d\n", retval);
	}

exit:
	return retval;
}

static int syna_tcm_spi_rmi_write(struct syna_tcm_hcd *tcm_hcd,
		unsigned short addr, unsigned char *data, unsigned int length)
{
	int retval = NO_ERR;

#if defined (CONFIG_TEE_TUI)
	if (tcm_hcd->syna_tcm_chip_data->report_tui_enable) {
		return NO_ERR;
	}
#endif

	retval = syna_tcm_spi_alloc_mem(tcm_hcd, (length + 2));
	if (retval < 0) {
		TS_LOG_ERR("Failed to allocate memory\n");
		goto exit;
	}
	retval = syna_tcm_spi_rmi_write_transfer(tcm_hcd, addr, data, (length + 2));
	if (retval < 0) {
		TS_LOG_ERR("Failed to spi rmi write transfer\n");
		goto exit;
	}
exit:
	return retval;
}

static int syna_tcm_spi_read_transfer(struct syna_tcm_hcd *tcm_hcd, unsigned char *data,
		unsigned int length)
{
	int retval = NO_ERR;
	struct spi_device *spi = tcm_hcd->syna_tcm_chip_data->ts_platform_data->spi;
	struct spi_transfer xfer[] = {
		{
			.tx_buf = buf,
			.rx_buf = data,
			.delay_usecs = bdata->byte_delay_us,
			.cs_change = 0,
			.len = length,
		},
	};

	memset(buf, 0xff, length);

	if(tcm_hcd->use_dma_download_firmware) {
		spi->max_speed_hz = tcm_hcd->spi_comnunicate_frequency;
		spi->controller_data = &g_ts_kit_platform_data.spidev0_chip_info;
	}else{
		spi->max_speed_hz = SPI_MAX_SPEED_READ;
	}
	retval = spi_setup(spi);
	if (retval) {
		TS_LOG_ERR("%s spi setup failed, retval = %d.\n", __func__, retval);
		return retval;
	}

	retval = spi_sync_transfer(spi, xfer, ARRAY_SIZE(xfer));
	if (retval == 0) {
		retval = length;
	} else {
		TS_LOG_ERR("Failed to complete SPI transfer, error = %d\n", retval);
	}

	return retval;
}

static int syna_tcm_spi_read(struct syna_tcm_hcd *tcm_hcd, unsigned char *data,
		unsigned int length)
{
	int retval = NO_ERR;

#if defined (CONFIG_TEE_TUI)
	if (tcm_hcd->syna_tcm_chip_data->report_tui_enable) {
		return NO_ERR;
	}
#endif

	retval = syna_tcm_spi_alloc_mem(tcm_hcd, length);
	if (retval < 0) {
		TS_LOG_ERR("Failed to allocate memory\n");
		goto exit;
	}

	retval = syna_tcm_spi_read_transfer(tcm_hcd, data, length);
	if (retval < 0) {
		TS_LOG_ERR("Failed to spi read transfer\n");
		goto exit;
	}

exit:
	return retval;
}

static int syna_tcm_spi_write_transfer(struct syna_tcm_hcd *tcm_hcd, unsigned char *data,
		unsigned int length)
{
	int retval = NO_ERR;
	struct spi_device *spi = tcm_hcd->syna_tcm_chip_data->ts_platform_data->spi;
	struct spi_transfer xfer[] = {
			{
				.tx_buf = data,
				.delay_usecs = bdata->byte_delay_us,
				.cs_change = 0,
				.len = length,
			},
	};

	if(tcm_hcd->use_dma_download_firmware) {
		spi->max_speed_hz = tcm_hcd->spi_comnunicate_frequency;
		spi->controller_data = &g_ts_kit_platform_data.spidev0_chip_info;
	}else{
		spi->max_speed_hz = SPI_MAX_SPEED_WRITE;
	}
	retval = spi_setup(spi);
	if (retval) {
		TS_LOG_ERR("%s spi setup failed, retval = %d.\n", __func__, retval);
		return retval;
	}

	retval = spi_sync_transfer(spi, xfer, ARRAY_SIZE(xfer));
	if (retval == 0) {
		retval = length;
	} else {
		TS_LOG_ERR("Failed to complete SPI transfer, error = %d\n", retval);
	}

	return retval;
}

static int syna_tcm_spi_write(struct syna_tcm_hcd *tcm_hcd, unsigned char *data,
		unsigned int length)
{
	int retval = NO_ERR;

#if defined (CONFIG_TEE_TUI)
	if (tcm_hcd->syna_tcm_chip_data->report_tui_enable) {
		return NO_ERR;
	}
#endif

	retval = syna_tcm_spi_write_transfer(tcm_hcd, data, length);
	if (retval < 0) {
			TS_LOG_ERR("Failed to spi write transfer\n");
	}

	return retval;
}

/**
 * syna_tcm_dispatch_report() - dispatch report received from device
 *
 * @tcm_hcd: handle of core module
 *
 * The report generated by the device is forwarded to the synchronous inbox of
 * each registered application module for further processing. In addition, the
 * report notifier thread is woken up for asynchronous notification of the
 * report occurrence.
 */

static void syna_tcm_dispatch_report(struct syna_tcm_hcd *tcm_hcd)
{
	LOCK_BUFFER(tcm_hcd->in);
	LOCK_BUFFER(tcm_hcd->report.buffer);

	tcm_hcd->report.buffer.buf = &tcm_hcd->in.buf[MESSAGE_HEADER_SIZE];
	tcm_hcd->report.buffer.buf_size = tcm_hcd->in.buf_size;
	tcm_hcd->report.buffer.buf_size -= MESSAGE_HEADER_SIZE;
	tcm_hcd->report.buffer.data_length = tcm_hcd->payload_length;
	tcm_hcd->report.id = tcm_hcd->status_report_code;

	UNLOCK_BUFFER(tcm_hcd->report.buffer);
	UNLOCK_BUFFER(tcm_hcd->in);

	return;
}

/**
 * syna_tcm_dispatch_response() - dispatch response received from device
 *
 * @tcm_hcd: handle of core module
 *
 * The response to a command is forwarded to the sender of the command.
 */

static void syna_tcm_dispatch_response(struct syna_tcm_hcd *tcm_hcd)
{
	int retval = NO_ERR;

	if (atomic_read(&tcm_hcd->command_status) != CMD_BUSY)
		return;

	tcm_hcd->response_code = tcm_hcd->status_report_code;

	if (tcm_hcd->payload_length == 0) {
		atomic_set(&tcm_hcd->command_status, CMD_IDLE);
		goto exit;
	}

	LOCK_BUFFER(tcm_hcd->resp);
	retval = syna_tcm_alloc_mem(tcm_hcd,
			&tcm_hcd->resp,
			tcm_hcd->payload_length);
	if (retval < 0) {
		TS_LOG_ERR("Failed to allocate memory for tcm_hcd->resp.buf\n");
		UNLOCK_BUFFER(tcm_hcd->resp);
		atomic_set(&tcm_hcd->command_status, CMD_ERROR);
		goto exit;
	}

	LOCK_BUFFER(tcm_hcd->in);
	retval = secure_memcpy(tcm_hcd->resp.buf,
			tcm_hcd->resp.buf_size,
			&tcm_hcd->in.buf[MESSAGE_HEADER_SIZE],
			tcm_hcd->in.buf_size - MESSAGE_HEADER_SIZE,
			tcm_hcd->payload_length);
	if (retval < 0) {
		TS_LOG_ERR("Failed to copy payload\n");
		UNLOCK_BUFFER(tcm_hcd->in);
		UNLOCK_BUFFER(tcm_hcd->resp);
		atomic_set(&tcm_hcd->command_status, CMD_ERROR);
		goto exit;
	}

	tcm_hcd->resp.data_length = tcm_hcd->payload_length;

	UNLOCK_BUFFER(tcm_hcd->in);
	UNLOCK_BUFFER(tcm_hcd->resp);

	atomic_set(&tcm_hcd->command_status, CMD_IDLE);

exit:
	complete(&response_complete);

	return;
}

/**
 * syna_tcm_dispatch_message() - dispatch message received from device
 *
 * @tcm_hcd: handle of core module
 *
 * The information received in the message read in from the device is dispatched
 * to the appropriate destination based on whether the information represents a
 * report or a response to a command.
 */

static void syna_tcm_dispatch_message(struct syna_tcm_hcd *tcm_hcd)
{
	int retval = NO_ERR;
	unsigned char *build_id = NULL;
	unsigned int payload_length = 0;
	unsigned int max_write_size = 0;

	if (tcm_hcd->status_report_code == REPORT_IDENTIFY) {
		payload_length = tcm_hcd->payload_length;

		LOCK_BUFFER(tcm_hcd->in);
		retval = secure_memcpy((unsigned char *)&tcm_hcd->id_info,
				sizeof(tcm_hcd->id_info),
				&tcm_hcd->in.buf[MESSAGE_HEADER_SIZE],
				tcm_hcd->in.buf_size - MESSAGE_HEADER_SIZE,
				MIN(sizeof(tcm_hcd->id_info), payload_length));
		if (retval < 0) {
			TS_LOG_ERR("Failed to copy identification info\n");
			UNLOCK_BUFFER(tcm_hcd->in);
			return;
		}
		UNLOCK_BUFFER(tcm_hcd->in);

		build_id = tcm_hcd->id_info.build_id;
		tcm_hcd->packrat_number = le4_to_uint(build_id);
		max_write_size = le2_to_uint(tcm_hcd->id_info.max_write_size);
		if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
			tcm_hcd->wr_chunk_size = MIN(max_write_size, WR_CHUNK_SIZE_SPI);
		}else{
			tcm_hcd->wr_chunk_size = MIN(max_write_size, WR_CHUNK_SIZE_I2C);
		}
		if (tcm_hcd->wr_chunk_size == 0)
			tcm_hcd->wr_chunk_size = max_write_size;

		TS_LOG_INFO("Received identify report (firmware mode = 0x%02x)\n",
				tcm_hcd->id_info.mode);

		if (atomic_read(&tcm_hcd->command_status) == CMD_BUSY) {
			switch (tcm_hcd->command) {
			case CMD_RESET:
			case CMD_RUN_BOOTLOADER_FIRMWARE:
			case CMD_RUN_APPLICATION_FIRMWARE:
				tcm_hcd->response_code = STATUS_OK;
				atomic_set(&tcm_hcd->command_status, CMD_IDLE);
				complete(&response_complete);
				break;
			default:
				TS_LOG_INFO("Device has been reset\n");
				atomic_set(&tcm_hcd->command_status, CMD_ERROR);
				complete(&response_complete);
				break;
			}
		}

		if (tcm_hcd->id_info.mode == MODE_HOST_DOWNLOAD) {
			tcm_hcd->host_download_mode = true;
			return;
		}

#ifdef FORCE_RUN_APPLICATION_FIRMWARE
		if (tcm_hcd->id_info.mode != MODE_APPLICATION) {
			if (atomic_read(&tcm_hcd->helper.task) == HELP_NONE) {
				atomic_set(&tcm_hcd->helper.task,
						HELP_RUN_APPLICATION_FIRMWARE);
				queue_work(tcm_hcd->helper.workqueue,
						&tcm_hcd->helper.work);
				return;
			}
		}
#endif
	}

	if (tcm_hcd->status_report_code >= REPORT_IDENTIFY)
		syna_tcm_dispatch_report(tcm_hcd);
	else
		syna_tcm_dispatch_response(tcm_hcd);

	return;
}

/**
 * syna_tcm_continued_read() - retrieve entire payload from device
 *
 * @tcm_hcd: handle of core module
 *
 * Read transactions are carried out until the entire payload is retrieved from
 * the device and stored in the handle of the core module.
 */

static int syna_tcm_continued_read(struct syna_tcm_hcd *tcm_hcd)
{
	int retval = NO_ERR;
	unsigned char marker = 0;
	unsigned char code = 0;
	unsigned int idx = 0;
	unsigned int offset = 0;
	unsigned int chunks = 0;
	unsigned int chunk_space = 0;
	unsigned int xfer_length = 0;
	unsigned int total_length = 0;
	unsigned int remaining_length = 0;

	total_length = MESSAGE_HEADER_SIZE + tcm_hcd->payload_length + 1;
	remaining_length = total_length - tcm_hcd->read_length;

	LOCK_BUFFER(tcm_hcd->in);
	retval = syna_tcm_realloc_mem(tcm_hcd,
			&tcm_hcd->in,
			total_length + 1);
	if (retval < 0) {
		TS_LOG_ERR("Failed to reallocate memory for tcm_hcd->in.buf\n");
		UNLOCK_BUFFER(tcm_hcd->in);
		return retval;
	}

	// available chunk space for payload = total chunk size minus header
	// marker byte and header code byte
	if (tcm_hcd->rd_chunk_size == 0)
		chunk_space = remaining_length;
	else
		chunk_space = tcm_hcd->rd_chunk_size - 2;

	chunks = ceil_div(remaining_length, chunk_space);
	chunks = chunks == 0 ? 1 : chunks;
	offset = tcm_hcd->read_length;

	LOCK_BUFFER(tcm_hcd->temp);
	for (idx = 0; idx < chunks; idx++) {
		if (remaining_length > chunk_space)
			xfer_length = chunk_space;
		else
			xfer_length = remaining_length;

		if (xfer_length == 1) {
			tcm_hcd->in.buf[offset] = MESSAGE_PADDING;
			offset += xfer_length;
			remaining_length -= xfer_length;
			continue;
		}

		retval = syna_tcm_alloc_mem(tcm_hcd,
				&tcm_hcd->temp,
				xfer_length + 2);
		if (retval < 0) {
			TS_LOG_ERR("Failed to allocate memory for tcm_hcd->temp.buf\n");
			UNLOCK_BUFFER(tcm_hcd->temp);
			UNLOCK_BUFFER(tcm_hcd->in);
			return retval;
		}

		retval = syna_tcm_read(tcm_hcd,
				tcm_hcd->temp.buf,
				xfer_length + 2);
		if (retval < 0) {
			TS_LOG_ERR("Failed to read from device\n");
			UNLOCK_BUFFER(tcm_hcd->temp);
			UNLOCK_BUFFER(tcm_hcd->in);
			return retval;
		}

		marker = tcm_hcd->temp.buf[0];
		code = tcm_hcd->temp.buf[1];
		if (marker != MESSAGE_MARKER) {
			TS_LOG_ERR("Incorrect header marker (0x%02x)\n", marker);
			UNLOCK_BUFFER(tcm_hcd->temp);
			UNLOCK_BUFFER(tcm_hcd->in);
			return -EIO;
		}

		if (code != STATUS_CONTINUED_READ) {
			TS_LOG_ERR("Incorrect header code (0x%02x)\n", code);
			UNLOCK_BUFFER(tcm_hcd->temp);
			UNLOCK_BUFFER(tcm_hcd->in);
			return -EIO;
		}

		retval = secure_memcpy(&tcm_hcd->in.buf[offset],
				tcm_hcd->in.buf_size - offset,
				&tcm_hcd->temp.buf[2],
				tcm_hcd->temp.buf_size - 2,
				xfer_length);
		if (retval < 0) {
			TS_LOG_ERR("Failed to copy payload\n");
			UNLOCK_BUFFER(tcm_hcd->temp);
			UNLOCK_BUFFER(tcm_hcd->in);
			return retval;
		}

		offset += xfer_length;
		remaining_length -= xfer_length;
	}

	UNLOCK_BUFFER(tcm_hcd->temp);
	UNLOCK_BUFFER(tcm_hcd->in);

	return 0;
}

/**
 * syna_tcm_raw_read() - retrieve specific number of data bytes from device
 *
 * @tcm_hcd: handle of core module
 * @in_buf: buffer for storing data retrieved from device
 * @length: number of bytes to retrieve from device
 *
 * Read transactions are carried out until the specific number of data bytes are
 * retrieved from the device and stored in in_buf.
 */

int syna_tcm_i2c_read(struct syna_tcm_hcd *tcm_hcd,
		unsigned char *in_buf, unsigned int length)
{
	int retval = NO_ERR;
	unsigned char code = 0;
	unsigned int idx = 0;
	unsigned int offset = 0;
	unsigned int chunks = 0;
	unsigned int chunk_space = 0;
	int xfer_length = 0;
	int remaining_length = 0;
	int static log_count = 5;

	if (length < 2) {
		retval = syna_tcm_i2c_read_data(tcm_hcd,
				in_buf,
				length);
		if (retval < 0) {
			TS_LOG_ERR(
					"Failed to read from device\n");
			return retval;
		}
		TS_LOG_ERR(
				"small length information\n");
		return length;
	}

	// minus header marker byte and header code byte
	remaining_length = length - 2;

	// available chunk space for data = total chunk size minus header marker
	// byte and header code byte
	if (tcm_hcd->rd_chunk_size == 0)
		chunk_space = remaining_length;
	else
		chunk_space = tcm_hcd->rd_chunk_size - 2;

	chunks = ceil_div(remaining_length, chunk_space);

	chunks = chunks == 0 ? 1 : chunks;

	offset = 0;

	LOCK_BUFFER(tcm_hcd->temp);

	for (idx = 0; idx < chunks; idx++) {

		if (remaining_length <= 0) {
			break;
		}

		if (remaining_length > chunk_space)
			xfer_length = chunk_space;
		else
			xfer_length = remaining_length;

		if (xfer_length == 1) {
			in_buf[offset] = MESSAGE_PADDING;
			offset += xfer_length;
			remaining_length -= xfer_length;
			continue;
		}

		retval = syna_tcm_alloc_mem(tcm_hcd,
				&tcm_hcd->temp,
				xfer_length + 2);
		if (retval < 0) {
			TS_LOG_ERR(
					"Failed to allocate memory for tcm_hcd->temp.buf\n");
			UNLOCK_BUFFER(tcm_hcd->temp);
			return retval;
		}

		retval = syna_tcm_i2c_read_data(tcm_hcd,
				tcm_hcd->temp.buf,
				xfer_length + 2);
		if (retval < 0) {
			TS_LOG_ERR(
					"Failed to read from device\n");
			UNLOCK_BUFFER(tcm_hcd->temp);
			return retval;
		}

		code = tcm_hcd->temp.buf[1];
		TS_LOG_DEBUG("temp buf:%x, %x, %x, %x, %x, %x\n", tcm_hcd->temp.buf[0], tcm_hcd->temp.buf[1],tcm_hcd->temp.buf[2], tcm_hcd->temp.buf[3],tcm_hcd->temp.buf[4], tcm_hcd->temp.buf[5]);

		if (idx == 0) {
			retval = secure_memcpy(&in_buf[0],
					length,
					&tcm_hcd->temp.buf[0],
					tcm_hcd->temp.buf_size,
					xfer_length + 2);
			//remaiming length cust
			TS_LOG_DEBUG("remaining length is  %d\n", remaining_length);
			if ((length > ((tcm_hcd->temp.buf[3] << 8 | tcm_hcd->temp.buf[2]) + 5)) && (code != STATUS_CONTINUED_READ)) {
				/* 5:header length, 2:minus header marker byte and header code byte */
				remaining_length = (tcm_hcd->temp.buf[3] << 8 | tcm_hcd->temp.buf[2]) + 5 - 2;
			}
			TS_LOG_DEBUG("remaining length cust to %d\n", remaining_length);
		} else {
			if (code != STATUS_CONTINUED_READ) {
				TS_LOG_ERR(
						"Incorrect header code (0x%02x)\n",
						code);
				UNLOCK_BUFFER(tcm_hcd->temp);
				return -EIO;
			}

			retval = secure_memcpy(&in_buf[offset],
					length - offset,
					&tcm_hcd->temp.buf[2],
					tcm_hcd->temp.buf_size - 2,
					xfer_length);
		}
		if (retval < 0) {
			TS_LOG_ERR(
					"Failed to copy data\n");
			UNLOCK_BUFFER(tcm_hcd->temp);
			return retval;
		}

		if (idx == 0)
			offset += (xfer_length + 2);
		else
			offset += xfer_length;

		remaining_length -= xfer_length;
	}

	UNLOCK_BUFFER(tcm_hcd->temp);

	return 0;
}

/**
 * syna_tcm_raw_read() - retrieve specific number of data bytes from device
 *
 * @tcm_hcd: handle of core module
 * @in_buf: buffer for storing data retrieved from device
 * @length: number of bytes to retrieve from device
 *
 * Read transactions are carried out until the specific number of data bytes are
 * retrieved from the device and stored in in_buf.
 */

static int syna_tcm_raw_read(struct syna_tcm_hcd *tcm_hcd,
		unsigned char *in_buf, unsigned int length)
{
	int retval = NO_ERR;
	unsigned char code = 0;
	unsigned int idx = 0;
	unsigned int offset = 0;
	unsigned int chunks = 0;
	unsigned int chunk_space = 0;
	unsigned int xfer_length = 0;
	unsigned int remaining_length = 0;

	if (TS_BUS_SPI != tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		return syna_tcm_i2c_read(tcm_hcd, in_buf, length);
	}

	if (length < 2) {
		TS_LOG_ERR("Invalid length information\n");
		return -EINVAL;
	}

	// minus header marker byte and header code byte
	remaining_length = length - 2;

	// available chunk space for data = total chunk size minus header marker
	// byte and header code byte
	if (tcm_hcd->rd_chunk_size == 0)
		chunk_space = remaining_length;
	else
		chunk_space = tcm_hcd->rd_chunk_size - 2;

	chunks = ceil_div(remaining_length, chunk_space);
	chunks = chunks == 0 ? 1 : chunks;
	offset = 0;

	LOCK_BUFFER(tcm_hcd->temp);
	for (idx = 0; idx < chunks; idx++) {
		if (remaining_length > chunk_space)
			xfer_length = chunk_space;
		else
			xfer_length = remaining_length;

		if (xfer_length == 1) {
			in_buf[offset] = MESSAGE_PADDING;
			offset += xfer_length;
			remaining_length -= xfer_length;
			continue;
		}

		retval = syna_tcm_alloc_mem(tcm_hcd,
				&tcm_hcd->temp,
				xfer_length + 2);
		if (retval < 0) {
			TS_LOG_ERR("Failed to allocate memory for tcm_hcd->temp.buf\n");
			UNLOCK_BUFFER(tcm_hcd->temp);
			return retval;
		}

		retval = syna_tcm_read(tcm_hcd,
				tcm_hcd->temp.buf,
				xfer_length + 2);
		if (retval < 0) {
			TS_LOG_ERR("Failed to read from device\n");
			UNLOCK_BUFFER(tcm_hcd->temp);
			return retval;
		}

		code = tcm_hcd->temp.buf[1];

		if (idx == 0) {
			retval = secure_memcpy(&in_buf[0],
					length,
					&tcm_hcd->temp.buf[0],
					tcm_hcd->temp.buf_size,
					xfer_length + 2);
		} else {
			if (code != STATUS_CONTINUED_READ) {
				TS_LOG_ERR("Incorrect header code (0x%02x)\n", code);
				UNLOCK_BUFFER(tcm_hcd->temp);
				return -EIO;
			}

			retval = secure_memcpy(&in_buf[offset],
					length - offset,
					&tcm_hcd->temp.buf[2],
					tcm_hcd->temp.buf_size - 2,
					xfer_length);
		}
		if (retval < 0) {
			TS_LOG_ERR("Failed to copy data\n");
			UNLOCK_BUFFER(tcm_hcd->temp);
			return retval;
		}

		if (idx == 0)
			offset += (xfer_length + 2);
		else
			offset += xfer_length;

		remaining_length -= xfer_length;
	}
	UNLOCK_BUFFER(tcm_hcd->temp);

	return 0;
}

/**
 * syna_tcm_raw_write() - write command/data to device without receiving
 * response
 *
 * @tcm_hcd: handle of core module
 * @command: command to send to device
 * @data: data to send to device
 * @length: length of data in bytes
 *
 * A command and its data, if any, are sent to the device.
 */

static int syna_tcm_raw_write(struct syna_tcm_hcd *tcm_hcd,
		unsigned char command, unsigned char *data, unsigned int length)
{
	int retval = NO_ERR;
	unsigned int idx  = 0;
	unsigned int chunks = 0;
	unsigned int chunk_space = 0;
	unsigned int xfer_length = 0;
	unsigned int remaining_length = 0;

	remaining_length = length;

	//available chunk space for data = total chunk size minus command
	//byte
	if (tcm_hcd->wr_chunk_size == 0)
		chunk_space = remaining_length;
	else
		chunk_space = tcm_hcd->wr_chunk_size - 1;

	chunks = ceil_div(remaining_length, chunk_space);
	chunks = chunks == 0 ? 1 : chunks;

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
			return retval;
		}

		if (idx == 0)
			tcm_hcd->out.buf[0] = command;
		else
			tcm_hcd->out.buf[0] = CMD_CONTINUE_WRITE;

		if (xfer_length) {
			retval = secure_memcpy(&tcm_hcd->out.buf[1],
					tcm_hcd->out.buf_size - 1,
					&data[idx * chunk_space],
					remaining_length,
					xfer_length);
			if (retval < 0) {
				TS_LOG_ERR("Failed to copy data\n");
				UNLOCK_BUFFER(tcm_hcd->out);
				return retval;
			}
		}

		retval = syna_tcm_write(tcm_hcd,
				tcm_hcd->out.buf,
				xfer_length + 1);
		if (retval < 0) {
			TS_LOG_ERR("Failed to write to device\n");
			UNLOCK_BUFFER(tcm_hcd->out);
			return retval;
		}

		remaining_length -= xfer_length;
	}
	UNLOCK_BUFFER(tcm_hcd->out);

	return 0;
}

/**
 * syna_tcm_read_message() - read message from device
 *
 * @tcm_hcd: handle of core module
 * @in_buf: buffer for storing data in raw read mode
 * @length: length of data in bytes in raw read mode
 *
 * If in_buf is not NULL, raw read mode is used and syna_tcm_raw_read() is
 * called. Otherwise, a message including its entire payload is retrieved from
 * the device and dispatched to the appropriate destination.
 */

static int syna_tcm_read_message(struct syna_tcm_hcd *tcm_hcd,
		unsigned char *in_buf, unsigned int length)
{
	int retval = NO_ERR;
	bool retry = true;
	unsigned int total_length = 0;
	struct syna_tcm_message_header *header = NULL;

	if (in_buf != NULL) {
		retval = syna_tcm_raw_read(tcm_hcd, in_buf, length);
		goto exit;
	}

retry:
	LOCK_BUFFER(tcm_hcd->in);

	retval = syna_tcm_read(tcm_hcd,
			tcm_hcd->in.buf,
			tcm_hcd->read_length);
	if (retval < 0) {
		TS_LOG_ERR("Failed to read from device\n");
		UNLOCK_BUFFER(tcm_hcd->in);
		if (retry) {
			usleep_range(READ_RETRY_US_MIN, READ_RETRY_US_MAX);
			retry = false;
			goto retry;
		}
		goto exit;
	}

	header = (struct syna_tcm_message_header *)tcm_hcd->in.buf;

	if (header->marker != MESSAGE_MARKER) {
		TS_LOG_ERR("Incorrect header marker (0x%02x)\n", header->marker);
		UNLOCK_BUFFER(tcm_hcd->in);
		retval = -ENXIO;
		if (retry) {
			usleep_range(READ_RETRY_US_MIN, READ_RETRY_US_MAX);
			retry = false;
			goto retry;
		}
		goto exit;
	}

	tcm_hcd->status_report_code = header->code;
	tcm_hcd->payload_length = le2_to_uint(header->length);
	if (tcm_hcd->status_report_code <= STATUS_ERROR ||
			tcm_hcd->status_report_code == STATUS_INVALID) {
		switch (tcm_hcd->status_report_code) {
		case STATUS_OK:
			break;
		case STATUS_CONTINUED_READ:
			TS_LOG_DEBUG("Out-of-sync continued read\n");
		case STATUS_IDLE:
		case STATUS_BUSY:

			tcm_hcd->payload_length = 0;
			UNLOCK_BUFFER(tcm_hcd->in);
			retval = 0;
			goto exit;
		default:
			TS_LOG_ERR("Incorrect header code (0x%02x)\n",
					tcm_hcd->status_report_code);
			if (tcm_hcd->status_report_code == STATUS_INVALID)
				tcm_hcd->payload_length = 0;
		}
	}

	total_length = MESSAGE_HEADER_SIZE + tcm_hcd->payload_length + 1;

#ifdef PREDICTIVE_READING
	if (total_length <= tcm_hcd->read_length) {
		goto check_padding;
	} else if (total_length - 1 == tcm_hcd->read_length) {
		tcm_hcd->in.buf[total_length - 1] = MESSAGE_PADDING;
		goto check_padding;
	}
#else
	if (tcm_hcd->payload_length == 0) {
		tcm_hcd->in.buf[total_length - 1] = MESSAGE_PADDING;
		goto check_padding;
	}
#endif

	UNLOCK_BUFFER(tcm_hcd->in);

	retval = syna_tcm_continued_read(tcm_hcd);
	if (retval < 0) {
		TS_LOG_ERR("Failed to do continued read\n");
		goto exit;
	};

	LOCK_BUFFER(tcm_hcd->in);
	tcm_hcd->in.buf[0] = MESSAGE_MARKER;
	tcm_hcd->in.buf[1] = tcm_hcd->status_report_code;
	tcm_hcd->in.buf[2] = (unsigned char)tcm_hcd->payload_length;
	tcm_hcd->in.buf[3] = (unsigned char)(tcm_hcd->payload_length >> 8);

check_padding:
	if (tcm_hcd->in.buf[total_length - 1] != MESSAGE_PADDING) {
		TS_LOG_ERR("Incorrect message padding byte (0x%02x)\n",
				tcm_hcd->in.buf[total_length - 1]);
		UNLOCK_BUFFER(tcm_hcd->in);
		retval = -EIO;
		goto exit;
	}

	UNLOCK_BUFFER(tcm_hcd->in);

#ifdef PREDICTIVE_READING
	total_length = MAX(total_length, MIN_READ_LENGTH);
	tcm_hcd->read_length = MIN(total_length, tcm_hcd->rd_chunk_size);
	if (tcm_hcd->rd_chunk_size == 0)
		tcm_hcd->read_length = total_length;
#endif

	syna_tcm_dispatch_message(tcm_hcd);

	retval = 0;

exit:
	if (retval < 0) {
		if (atomic_read(&tcm_hcd->command_status) == CMD_BUSY) {
			atomic_set(&tcm_hcd->command_status, CMD_ERROR);
			complete(&response_complete);
		}
	}

	return retval;
}

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

static int syna_tcm_write_message(struct syna_tcm_hcd *tcm_hcd,
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
	unsigned int command_status = 0;

	if (response_code != NULL)
		*response_code = STATUS_INVALID;

	if (!tcm_hcd->do_polling && current->pid == tcm_hcd->isr_pid) {
		TS_LOG_ERR("Invalid execution context\n");
		return -EINVAL;
	}

	if (resp_buf == NULL) {
		retval = syna_tcm_raw_write(tcm_hcd, command, payload, length);
		goto exit;
	}

	if (tcm_hcd->do_polling && polling_delay_ms) {
		cancel_delayed_work_sync(&tcm_hcd->polling_work);
		flush_workqueue(tcm_hcd->polling_workqueue);
	}

	atomic_set(&tcm_hcd->command_status, CMD_BUSY);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	reinit_completion(&response_complete);
#else
	INIT_COMPLETION(response_complete);
#endif

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
			usleep_range(WRITE_DELAY_US_MIN, WRITE_DELAY_US_MAX);
	}
	UNLOCK_BUFFER(tcm_hcd->out);

	if (tcm_hcd->do_polling && polling_delay_ms) {
		queue_delayed_work(tcm_hcd->polling_workqueue,
				&tcm_hcd->polling_work,
				msecs_to_jiffies(polling_delay_ms));
	}

	retval = wait_for_completion_timeout(&response_complete,
			msecs_to_jiffies(RESPONSE_TIMEOUT_MS));
	if (retval == 0) {
		TS_LOG_ERR("Timed out waiting for response (command 0x%02x)\n",
				tcm_hcd->command);
		retval = -EIO;
		goto exit;
	}

	command_status = atomic_read(&tcm_hcd->command_status);
	if (command_status != CMD_IDLE) {
		TS_LOG_ERR("Failed to get valid response (command 0x%02x)\n",
				tcm_hcd->command);
		retval = -EIO;
		goto exit;
	}

	LOCK_BUFFER(tcm_hcd->resp);
	if (tcm_hcd->response_code != STATUS_OK) {
		if (tcm_hcd->resp.data_length) {
			TS_LOG_ERR("Error code = 0x%02x (command 0x%02x)\n",
					tcm_hcd->resp.buf[0], tcm_hcd->command);
		}
		retval = -EIO;
	} else {
		retval = 0;
	}

	*resp_buf = tcm_hcd->resp.buf;
	*resp_buf_size = tcm_hcd->resp.buf_size;
	*resp_length = tcm_hcd->resp.data_length;

	if (response_code != NULL)
		*response_code = tcm_hcd->response_code;

	UNLOCK_BUFFER(tcm_hcd->resp);

exit:
	tcm_hcd->command = CMD_NONE;
	atomic_set(&tcm_hcd->command_status, CMD_IDLE);
	return retval;
}


static int syna_tcm_wait_hdl(struct syna_tcm_hcd *tcm_hcd)
{
	int retval = NO_ERR;

	msleep(HOST_DOWNLOAD_WAIT_MS);

	if (!atomic_read(&tcm_hcd->host_downloading))
		return 0;

	retval = wait_event_interruptible_timeout(tcm_hcd->hdl_wq,
			!atomic_read(&tcm_hcd->host_downloading),
			msecs_to_jiffies(HOST_DOWNLOAD_TIMEOUT_MS));
	if (retval == 0) {
		TS_LOG_ERR("Timed out waiting for completion of host download\n");
		retval = -EIO;
	} else {
		retval = 0;
	}

	return retval;
}

static void syna_tcm_update_watchdog(struct syna_tcm_hcd *tcm_hcd, bool en)
{
	cancel_delayed_work_sync(&tcm_hcd->watchdog.work);
	flush_workqueue(tcm_hcd->watchdog.workqueue);

	if (!tcm_hcd->watchdog.run) {
		tcm_hcd->watchdog.count = 0;
		return;
	}

	if (en) {
		queue_delayed_work(tcm_hcd->watchdog.workqueue,
				&tcm_hcd->watchdog.work,
				msecs_to_jiffies(WATCHDOG_DELAY_MS));
	} else {
		tcm_hcd->watchdog.count = 0;
	}

	return;
}

static int syna_tcm_pinctrl_init(void)
{
	int ret = NO_ERR;

	tcm_hcd->pctrl = devm_pinctrl_get(&tcm_hcd->pdev->dev);
	if (IS_ERR(tcm_hcd->pctrl)) {
		TS_LOG_ERR("%s: devm_pinctrl_get failed.\n", __func__);
		tcm_hcd->pctrl = NULL;
		return -EINVAL;
	}

	tcm_hcd->pins_default = pinctrl_lookup_state(tcm_hcd->pctrl, "default");
	if (IS_ERR(tcm_hcd->pins_default)) {
		ret = PTR_ERR(tcm_hcd->pins_default);
		TS_LOG_ERR("%s: lookup state default, ret: %d\n", __func__, ret);
		goto err_pinctrl_put;
	}

	tcm_hcd->pins_idle = pinctrl_lookup_state(tcm_hcd->pctrl, "idle");
	if (IS_ERR(tcm_hcd->pins_idle)) {
		ret = PTR_ERR(tcm_hcd->pins_idle);
		TS_LOG_ERR("%s: lookup state idle, ret: %d\n", __func__, ret);
		goto err_pinctrl_put;
	}

	return NO_ERR;

err_pinctrl_put:
	devm_pinctrl_put(tcm_hcd->pctrl);
	tcm_hcd->pctrl= NULL;
	tcm_hcd->pins_default= NULL;
	tcm_hcd->pins_idle= NULL;

	return ret;
}


static void syna_tcm_pinctrl_release(void)
{
	if (tcm_hcd->pctrl)
		devm_pinctrl_put(tcm_hcd->pctrl);

	tcm_hcd->pctrl= NULL;
	tcm_hcd->pins_default= NULL;
	tcm_hcd->pins_idle= NULL;
}

static void syna_tcm_pinctrl_select_lowpower(void)
{
	int retval = NO_ERR;

	if (tcm_hcd->pctrl) {
		retval = pinctrl_select_state(tcm_hcd->pctrl, tcm_hcd->pins_idle);
		if (retval < 0) {
			TS_LOG_ERR("%s: set idle pins state error, %d\n", __func__, retval);
		}
	}

	return;
}

static void syna_tcm_pinctrl_select_normal(void)
{
	int retval = NO_ERR;

	if (tcm_hcd->pctrl) {
		retval = pinctrl_select_state(tcm_hcd->pctrl, tcm_hcd->pins_default);
		if (retval < 0) {
			TS_LOG_ERR("%s: set normal pins state error, %d\n", __func__, retval);
		}
	}

	return;
}

static void syna_tcm_power_on_gpio_set(void)
{
	int retval = NO_ERR;

	retval = gpio_direction_input(tcm_hcd->syna_tcm_chip_data->ts_platform_data->irq_gpio);
	if (retval) {
		TS_LOG_ERR("%s: set irq gpio direction input error: %d\n", __func__, retval);
	}

	retval = gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, 1);
	if (retval) {
		TS_LOG_ERR("%s: set reset gpio direction output error: %d\n", __func__, retval);
	}
	return;
}

static void syna_tcm_gpio_reset(void)
{
	TS_LOG_INFO("synaptics_gpio_reset\n");
	if (!tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio) {
		TS_LOG_INFO("reset_gpio is null, not supported reset\n");
		return;
	}

	gpio_direction_input(tcm_hcd->syna_tcm_chip_data->ts_platform_data->irq_gpio);
	TS_LOG_INFO("set gpio int input\n");

	gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, GPIO_OUTPUT_HIGH);
	mdelay(1);
	return;
}

static void syna_tcm_power_off_gpio_set(void)
{
	gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, 0);
	mdelay(1);
	return;
}


static int syna_tcm_gpio_request(void)
{
	int retval = NO_ERR;
	TS_LOG_INFO("synaptics_gpio_request\n");

	if ((1 == tcm_hcd->syna_tcm_chip_data->vci_gpio_type)
	    && (1 == tcm_hcd->syna_tcm_chip_data->vddio_gpio_type)) {
		if (tcm_hcd->syna_tcm_chip_data->vci_gpio_ctrl ==
		    tcm_hcd->syna_tcm_chip_data->vddio_gpio_ctrl) {
			retval = gpio_request(tcm_hcd->syna_tcm_chip_data->vci_gpio_ctrl, "ts_vci_gpio");
			if (retval) {
				TS_LOG_INFO("unable to request vci_gpio_ctrl firset:%d\n",
				     tcm_hcd->syna_tcm_chip_data->vci_gpio_ctrl);
				goto ts_vci_out;
			}
		} else {
			retval = gpio_request(tcm_hcd->syna_tcm_chip_data->vci_gpio_ctrl, "ts_vci_gpio");
			if (retval) {
				TS_LOG_INFO("unable to request vci_gpio_ctrl2:%d\n",
				     tcm_hcd->syna_tcm_chip_data->vci_gpio_ctrl);
				goto ts_vci_out;
			}
			retval = gpio_request(tcm_hcd->syna_tcm_chip_data->vddio_gpio_ctrl, "ts_vddio_gpio");
			if (retval) {
				TS_LOG_INFO("unable to request vddio_gpio_ctrl:%d\n",
				     tcm_hcd->syna_tcm_chip_data->vddio_gpio_ctrl);
				goto ts_vddio_out;
			}
		}
	} else {
		if (1 == tcm_hcd->syna_tcm_chip_data->vci_gpio_type) {
			retval = gpio_request(tcm_hcd->syna_tcm_chip_data->vci_gpio_ctrl, "ts_vci_gpio");
			if (retval) {
				TS_LOG_INFO("unable to request vci_gpio_ctrl2:%d\n",
				     tcm_hcd->syna_tcm_chip_data->vci_gpio_ctrl);
				goto ts_vci_out;
			}
		}
		if (1 == tcm_hcd->syna_tcm_chip_data->vddio_gpio_type) {
			retval = gpio_request(tcm_hcd->syna_tcm_chip_data->vddio_gpio_ctrl, "ts_vddio_gpio");
			if (retval) {
				TS_LOG_INFO("unable to request vddio_gpio_ctrl:%d\n",
				     tcm_hcd->syna_tcm_chip_data->vddio_gpio_ctrl);
				goto ts_vddio_out;
			}
		}
	}

	TS_LOG_INFO("reset:%d, irq:%d,\n",
			 tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio,
			 tcm_hcd->syna_tcm_chip_data->ts_platform_data->irq_gpio);

	goto ts_reset_out;

ts_vddio_out:
	gpio_free(tcm_hcd->syna_tcm_chip_data->vci_gpio_ctrl);
ts_vci_out:
	//gpio_free(tcm_hcd->syna_tcm_chip_data->irq_gpio);
	//gpio_free(tcm_hcd->syna_tcm_chip_data->reset_gpio);
ts_reset_out:
	return retval;

}

static int syna_tcm_get_app_info(struct syna_tcm_hcd *tcm_hcd)
{
	int retval = NO_ERR;
	unsigned char *resp_buf = NULL;
	unsigned int resp_buf_size = 0;
	unsigned int resp_length = 0;
	unsigned int timeout = 0;

	timeout = APP_STATUS_POLL_TIMEOUT_MS;
	resp_buf = NULL;
	resp_buf_size = 0;

get_app_info:
	retval = tcm_hcd->write_message(tcm_hcd,
			CMD_GET_APPLICATION_INFO,
			NULL,
			0,
			&resp_buf,
			&resp_buf_size,
			&resp_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n",
				STR(CMD_GET_APPLICATION_INFO));
		goto exit;
	}

	retval = secure_memcpy((unsigned char *)&tcm_hcd->app_info,
			sizeof(tcm_hcd->app_info),
			resp_buf,
			resp_buf_size,
			MIN(sizeof(tcm_hcd->app_info), resp_length));
	if (retval < 0) {
		TS_LOG_ERR("Failed to copy application info\n");
		goto exit;
	}

	tcm_hcd->app_status = le2_to_uint(tcm_hcd->app_info.status);

	if (tcm_hcd->app_status == APP_STATUS_BOOTING ||
			tcm_hcd->app_status == APP_STATUS_UPDATING) {
		if (timeout > 0) {
			msleep(APP_STATUS_POLL_MS);
			timeout -= APP_STATUS_POLL_MS;
			goto get_app_info;
		}
	}

	retval = 0;

exit:
	kfree(resp_buf);
	return retval;
}

static int syna_tcm_get_boot_info(struct syna_tcm_hcd *tcm_hcd)
{
	int retval = NO_ERR;
	unsigned char *resp_buf = NULL;
	unsigned int resp_buf_size = 0;
	unsigned int resp_length = 0;

	resp_buf = NULL;
	resp_buf_size = 0;

	retval = tcm_hcd->write_message(tcm_hcd,
			CMD_GET_BOOT_INFO,
			NULL,
			0,
			&resp_buf,
			&resp_buf_size,
			&resp_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n", STR(CMD_GET_BOOT_INFO));
		goto exit;
	}

	retval = secure_memcpy((unsigned char *)&tcm_hcd->boot_info,
			sizeof(tcm_hcd->boot_info),
			resp_buf,
			resp_buf_size,
			MIN(sizeof(tcm_hcd->boot_info), resp_length));
	if (retval < 0) {
		TS_LOG_ERR("Failed to copy boot info\n");
		goto exit;
	}

	retval = 0;

exit:
	kfree(resp_buf);
	return retval;
}

static int syna_tcm_identify(struct syna_tcm_hcd *tcm_hcd, bool id)
{
	int retval = NO_ERR;
	unsigned char *resp_buf = NULL;
	unsigned int resp_buf_size = 0;
	unsigned int resp_length = 0;
	unsigned int max_write_size = 0;

	resp_buf = NULL;
	resp_buf_size = 0;

	if (!id)
		goto get_info;

	retval = tcm_hcd->write_message(tcm_hcd,
			CMD_IDENTIFY,
			NULL,
			0,
			&resp_buf,
			&resp_buf_size,
			&resp_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n", STR(CMD_IDENTIFY));
		goto exit;
	}

	retval = secure_memcpy((unsigned char *)&tcm_hcd->id_info,
			sizeof(tcm_hcd->id_info),
			resp_buf,
			resp_buf_size,
			MIN(sizeof(tcm_hcd->id_info), resp_length));
	if (retval < 0) {
		TS_LOG_ERR("Failed to copy identification info\n");
		goto exit;
	}

	tcm_hcd->packrat_number = le4_to_uint(tcm_hcd->id_info.build_id);

	max_write_size = le2_to_uint(tcm_hcd->id_info.max_write_size);
	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		tcm_hcd->wr_chunk_size = MIN(max_write_size, WR_CHUNK_SIZE_SPI);
	}else{
		tcm_hcd->wr_chunk_size = MIN(max_write_size, WR_CHUNK_SIZE_I2C);
	}
	if (tcm_hcd->wr_chunk_size == 0)
		tcm_hcd->wr_chunk_size = max_write_size;

get_info:
	switch (tcm_hcd->id_info.mode) {
	case MODE_APPLICATION:
		retval = syna_tcm_get_app_info(tcm_hcd);
		if (retval < 0) {
			TS_LOG_ERR("Failed to get application info\n");
			goto exit;
		}
		break;
	case MODE_BOOTLOADER:
	case MODE_TDDI_BOOTLOADER:
		retval = syna_tcm_get_boot_info(tcm_hcd);
		if (retval < 0) {
			TS_LOG_ERR("Failed to get boot info\n");
			goto exit;
		}
		break;
	default:
		break;
	}

	retval = 0;

exit:

	kfree(resp_buf);

	return retval;
}

static int syna_tcm_run_application_firmware(struct syna_tcm_hcd *tcm_hcd)
{
	int retval = NO_ERR;
	bool retry = true;
	unsigned char *resp_buf = NULL;
	unsigned int resp_buf_size = 0;
	unsigned int resp_length = 0;

	resp_buf = NULL;
	resp_buf_size = 0;

retry:
	retval = tcm_hcd->write_message(tcm_hcd,
			CMD_RUN_APPLICATION_FIRMWARE,
			NULL,
			0,
			&resp_buf,
			&resp_buf_size,
			&resp_length,
			NULL,
			MODE_SWITCH_DELAY_MS);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n",
				STR(CMD_RUN_APPLICATION_FIRMWARE));
		goto exit;
	}

	retval = tcm_hcd->identify(tcm_hcd, false);
	if (retval < 0) {
		TS_LOG_ERR("Failed to do identification\n");
		goto exit;
	}

	if (tcm_hcd->id_info.mode != MODE_APPLICATION) {
		TS_LOG_ERR("Failed to run application firmware (boot status = 0x%02x)\n",
				tcm_hcd->boot_info.status);
		if (retry) {
			retry = false;
			goto retry;
		}
		retval = -EINVAL;
		goto exit;
	} else if (tcm_hcd->app_status != APP_STATUS_OK) {
		TS_LOG_ERR("Application status = 0x%02x\n", tcm_hcd->app_status);
	}

	retval = 0;

exit:
	kfree(resp_buf);
	return retval;
}

static int syna_tcm_run_bootloader_firmware(struct syna_tcm_hcd *tcm_hcd)
{
	int retval = NO_ERR;
	unsigned char *resp_buf = NULL;
	unsigned int resp_buf_size = 0;
	unsigned int resp_length = 0;

	resp_buf = NULL;
	resp_buf_size = 0;

	retval = tcm_hcd->write_message(tcm_hcd,
			CMD_RUN_BOOTLOADER_FIRMWARE,
			NULL,
			0,
			&resp_buf,
			&resp_buf_size,
			&resp_length,
			NULL,
			MODE_SWITCH_DELAY_MS);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n",
				STR(CMD_RUN_BOOTLOADER_FIRMWARE));
		goto exit;
	}

	retval = tcm_hcd->identify(tcm_hcd, false);
	if (retval < 0) {
		TS_LOG_ERR("Failed to do identification\n");
		goto exit;
	}

	if (tcm_hcd->id_info.mode == MODE_APPLICATION) {
		TS_LOG_ERR("Failed to enter bootloader mode\n");
		retval = -EINVAL;
		goto exit;
	}

	retval = 0;

exit:
	kfree(resp_buf);

	return retval;
}

static int syna_tcm_switch_mode(struct syna_tcm_hcd *tcm_hcd,
		enum firmware_mode mode)
{
	int retval = NO_ERR;

	tcm_hcd->update_watchdog(tcm_hcd, false);

	switch (mode) {
	case FW_MODE_BOOTLOADER:
		retval = syna_tcm_run_bootloader_firmware(tcm_hcd);
		if (retval < 0) {
			TS_LOG_ERR("Failed to switch to bootloader mode\n");
			goto exit;
		}
		break;
	case FW_MODE_APPLICATION:
		retval = syna_tcm_run_application_firmware(tcm_hcd);
		if (retval < 0) {
			TS_LOG_ERR("Failed to switch to application mode\n");
			goto exit;
		}
		break;
	default:
		TS_LOG_ERR("Invalid firmware mode\n");
		retval = -EINVAL;
		goto exit;
	}

	retval = 0;

exit:
	tcm_hcd->update_watchdog(tcm_hcd, true);

	return retval;
}

static int syna_tcm_get_dynamic_config(struct syna_tcm_hcd *tcm_hcd,
		enum dynamic_config_id id, unsigned short *value)
{
	int retval = NO_ERR;
	unsigned char out_buf = 0;
	unsigned char resp_buf[10] = {0};
	unsigned int resp_buf_size = 0;
	unsigned int resp_length = 0;
	int retry = 5;

	resp_buf_size = 0;
	out_buf = (unsigned char)id;

	retval = syna_tcm_write_hdl_message(tcm_hcd,
			CMD_GET_DYNAMIC_CONFIG,
			&out_buf,
			sizeof(out_buf),
			(unsigned char **)resp_buf,
			&resp_buf_size,
			&resp_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n", STR(CMD_GET_DYNAMIC_CONFIG));
		goto exit;
	}

	while (retry) {
		msleep(50);
		retval = syna_tcm_read(tcm_hcd,
				resp_buf,
				sizeof(resp_buf));
		if (retval < 0 ||resp_buf[0] != MESSAGE_MARKER) {
			TS_LOG_ERR("Failed to read response %s\n", STR(CMD_SET_DYNAMIC_CONFIG));
			goto exit;
		}

		if (resp_buf[1] != STATUS_OK)
			TS_LOG_INFO("resp_buf: 0x%02x 0x%02x 0x%02x 0x%02x ", resp_buf[0],resp_buf[1],resp_buf[2],resp_buf[3]);
		else
			break;
		retry --;
	}

	*value = resp_buf[4] |resp_buf[5] ;
	retval = 0;

exit:

	return retval;
}

static int syna_tcm_set_dynamic_config_no_wait(struct syna_tcm_hcd *tcm_hcd,
		enum dynamic_config_id id, unsigned short value)
{
	int retval = NO_ERR;
	unsigned char out_buf[3] = {0};
	unsigned char resp_buf[10] = {0};
	unsigned int resp_buf_size = 0;
	unsigned int resp_length = 0;


	resp_buf_size = 0;

	out_buf[0] = (unsigned char)id;
	out_buf[1] = (unsigned char)value;
	out_buf[2] = (unsigned char)(value >> 8);


		retval = syna_tcm_write_hdl_message(tcm_hcd,
				CMD_SET_DYNAMIC_CONFIG,
				out_buf,
				sizeof(out_buf),
				resp_buf,
				&resp_buf_size,
				&resp_length,
				NULL,
				0);
		if (retval < 0) {
			TS_LOG_ERR("Failed to write command %s\n", STR(CMD_SET_DYNAMIC_CONFIG));
			goto exit;
		}

	retval = 0;

exit:
	return retval;
}

static int syna_tcm_set_dynamic_config(struct syna_tcm_hcd *tcm_hcd,
		enum dynamic_config_id id, unsigned short value)
{
	int retval = NO_ERR;
	unsigned char out_buf[3] = {0};
	unsigned char resp_buf[10] = {0};
	unsigned int resp_buf_size = 0;
	unsigned int resp_length = 0;
	int retry = 5;

	resp_buf_size = 0;

	out_buf[0] = (unsigned char)id;
	out_buf[1] = (unsigned char)value;
	out_buf[2] = (unsigned char)(value >> 8);

	while (retry) {
		retval = syna_tcm_write_hdl_message(tcm_hcd,
				CMD_SET_DYNAMIC_CONFIG,
				out_buf,
				sizeof(out_buf),
				(unsigned char **)resp_buf,
				&resp_buf_size,
				&resp_length,
				NULL,
				0);
		if (retval < 0) {
			TS_LOG_ERR("Failed to write command %s\n", STR(CMD_SET_DYNAMIC_CONFIG));
			goto exit;
		}

		msleep(50);
		retval = syna_tcm_read(tcm_hcd,
				resp_buf,
				sizeof(resp_buf));
		if (retval < 0 ||resp_buf[0] != MESSAGE_MARKER) {
				TS_LOG_ERR("Failed to read response %s\n", STR(CMD_SET_DYNAMIC_CONFIG));
				goto exit;
		}
		if (resp_buf[1] != STATUS_OK) {
			TS_LOG_INFO("resp_buf: 0x%02x 0x%02x 0x%02x 0x%02x ", resp_buf[0],resp_buf[1],resp_buf[2],resp_buf[3]);
		}else{
			break;
		}
		retry --;
	}

	retval = 0;

exit:
	return retval;
}

static int syna_tcm_get_data_location(struct syna_tcm_hcd *tcm_hcd,
		enum flash_area area, unsigned int *addr, unsigned int *length)
{
	int retval = NO_ERR;
	unsigned char out_buf = 0;
	unsigned char *resp_buf = NULL;
	unsigned int resp_buf_size = 0;
	unsigned int resp_length = 0;

	switch (area) {
	case CUSTOM_LCM:
		out_buf = LCM_DATA;
		break;
	case CUSTOM_OEM:
		out_buf = OEM_DATA;
		break;
	case PPDT:
		out_buf = PPDT_DATA;
		break;
	default:
		TS_LOG_ERR("Invalid flash area\n");
		return -EINVAL;
	}

	resp_buf = NULL;
	resp_buf_size = 0;

	retval = tcm_hcd->write_message(tcm_hcd,
			CMD_GET_DATA_LOCATION,
			&out_buf,
			sizeof(out_buf),
			&resp_buf,
			&resp_buf_size,
			&resp_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n", STR(CMD_GET_DATA_LOCATION));
		goto exit;
	}

	if (resp_length != 4) {
		TS_LOG_ERR("Invalid data length\n");
		retval = -EINVAL;
		goto exit;
	}

	*addr = le2_to_uint(&resp_buf[0]);
	*length = le2_to_uint(&resp_buf[2]);

	retval = 0;

exit:
	kfree(resp_buf);

	return retval;
}

static int syna_tcm_sleep(struct syna_tcm_hcd *tcm_hcd, bool en)
{
	int retval = NO_ERR;
	unsigned char command = 0;
	unsigned char resp_buf[10] = {0};
	unsigned int resp_buf_size = 0;
	unsigned int resp_length = 0;

	TS_LOG_INFO("%s + %d\n", __func__,en);

	command = en ? CMD_ENTER_DEEP_SLEEP : CMD_EXIT_DEEP_SLEEP;
	resp_buf_size = 0;
	retval = syna_tcm_write_hdl_message(tcm_hcd,
			command,
			NULL,
			0,
			(unsigned char **)resp_buf,
			&resp_buf_size,
			&resp_length,
			NULL,
			0);
	if (retval < 0) {
		TS_LOG_ERR("Failed to write command %s\n",
				en ?
				STR(CMD_ENTER_DEEP_SLEEP) :
				STR(CMD_EXIT_DEEP_SLEEP));
		goto exit;
	}

	retval = 0;

exit:
	return retval;
}

static int syna_tcm_reset(struct syna_tcm_hcd *tcm_hcd, bool hw, bool update_wd)
{
	int retval = NO_ERR;
	unsigned char *resp_buf = NULL;
	unsigned int resp_buf_size = 0;
	unsigned int resp_length = 0;

	resp_buf = NULL;
	resp_buf_size = 0;

	if (update_wd)
		tcm_hcd->update_watchdog(tcm_hcd, false);

	if (hw) {
		if (tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio < 0) {
			TS_LOG_ERR("Hardware reset unavailable\n");
			retval = -EINVAL;
			goto exit;
		}
		gpio_set_value(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, bdata->reset_on_state);
		msleep(bdata->reset_active_ms);
		gpio_set_value(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, !bdata->reset_on_state);
	} else {
		retval = tcm_hcd->write_message(tcm_hcd,
				CMD_RESET,
				NULL,
				0,
				&resp_buf,
				&resp_buf_size,
				&resp_length,
				NULL,
				bdata->reset_delay_ms);
		if (retval < 0 && !tcm_hcd->host_download_mode) {
			TS_LOG_ERR("Failed to write command %s\n", STR(CMD_RESET));
			goto exit;
		}
	}

	if (tcm_hcd->host_download_mode) {
		kfree(resp_buf);
		retval = syna_tcm_wait_hdl(tcm_hcd);
		if (retval < 0) {
			TS_LOG_ERR("Failed to wait for completion of host download\n");
			return retval;
		}
		if (update_wd)
			tcm_hcd->update_watchdog(tcm_hcd, true);
		return 0;
	}
	msleep(bdata->reset_delay_ms);

	retval = tcm_hcd->identify(tcm_hcd, false);
	if (retval < 0) {
		TS_LOG_ERR("Failed to do identification\n");
		goto exit;
	}

	if (tcm_hcd->id_info.mode == MODE_APPLICATION)
		goto dispatch_reset;

	retval = tcm_hcd->write_message(tcm_hcd,
			CMD_RUN_APPLICATION_FIRMWARE,
			NULL,
			0,
			&resp_buf,
			&resp_buf_size,
			&resp_length,
			NULL,
			MODE_SWITCH_DELAY_MS);
	if (retval < 0) {
		TS_LOG_INFO("Failed to write command %s\n",
				STR(CMD_RUN_APPLICATION_FIRMWARE));
	}

	retval = tcm_hcd->identify(tcm_hcd, false);
	if (retval < 0) {
		TS_LOG_ERR("Failed to do identification\n");
		goto exit;
	}

dispatch_reset:
	TS_LOG_INFO("Firmware mode = 0x%02x\n",tcm_hcd->id_info.mode);

	if (tcm_hcd->id_info.mode != MODE_APPLICATION) {
		TS_LOG_INFO("Boot status = 0x%02x\n",
				tcm_hcd->boot_info.status);
	} else if (tcm_hcd->app_status != APP_STATUS_OK) {
		TS_LOG_INFO("Application status = 0x%02x\n",
				tcm_hcd->app_status);
	}

	retval = 0;

exit:
	if (update_wd)
		tcm_hcd->update_watchdog(tcm_hcd, true);
	kfree(resp_buf);

	return retval;
}

static int syna_tcm_comm_check(void)
{
	int retval = NO_ERR;
	unsigned char uboot[6] = {0};
	unsigned char marker = 0;

	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		retval = syna_tcm_rmi_read(tcm_hcd, PDT_START_ADDR, uboot, 6);
		TS_LOG_ERR("detect synaptics uboot[0] = %x.uboot[1] = %xuboot[2] = %x.uboot[3] = %xuboot[4] = %x.uboot[5] = %x\n",
			uboot[0],uboot[1], uboot[2], uboot[3], uboot[4],uboot[5]);

		if (uboot[5] == UBL_FN_NUMBER)
			return 0;
		else {
			TS_LOG_ERR("failed to detect F$35!");
			return -ENODEV;
		}
	}else{
		retval = syna_tcm_read(tcm_hcd,
			&marker,
			1);
		TS_LOG_ERR("check result:%d\n", retval);

		if (retval < 0 || marker != MESSAGE_MARKER) {
			TS_LOG_ERR(
					"Failed to read from device\n");
			return RESULT_ERR;
		}
	}
	return NO_ERR;
}

static int syna_tcm_mmi_test(struct ts_rawdata_info *info,
				 struct ts_cmd_node *out_cmd)
{
	int retval = 0;

	if (!info) {
		TS_LOG_ERR("info is NULL return!\n");
		return -ENODEV;
	}

	TS_LOG_INFO("++++ syna_tcm_mmi_test in\n");
	retval = syna_tcm_cap_test_init(tcm_hcd);
	if (retval < 0) {
		TS_LOG_ERR("Failed to init test_tcm\n");
		return retval;
	}

	retval = syna_tcm_cap_test((struct ts_rawdata_info_new *)info, out_cmd);
	if (retval < 0) {
		TS_LOG_ERR("Failed to syna_tcm_testing\n");
		return retval;
	}

	return NO_ERR;
}

static int syna_tcm_read_one_package(struct ts_fingers *info)
{
	int retval = NO_ERR;
	bool retry = true;
	struct syna_tcm_message_header *header = NULL;
	static int error_log_count = 0;

retry:
	retval = syna_tcm_read(tcm_hcd,
			buffer,
			FIXED_READ_LENGTH);
	if (retval < 0) {
		TS_LOG_ERR("Failed to read from device\n");
		if (retry) {
			usleep_range(READ_RETRY_US_MIN, READ_RETRY_US_MAX);
			retry = false;
			goto retry;
		}
		goto exit;
	}

	header = (struct syna_tcm_message_header *)buffer;
	if (header->marker != MESSAGE_MARKER) {
		if(error_log_count < 5) {
			error_log_count++;
			TS_LOG_ERR("Incorrect header marker (0x%02x),error_log_count=%d\n", header->marker,error_log_count);
		}
		retval = -ENXIO;
		if (retry) {
			usleep_range(READ_RETRY_US_MIN, READ_RETRY_US_MAX);
			retry = false;
			goto retry;
		}
		goto exit;
	}

	tcm_hcd->status_report_code = header->code;
	tcm_hcd->payload_length = le2_to_uint(header->length);

	TS_LOG_DEBUG("Header code = 0x%02x\n",
			tcm_hcd->status_report_code);

	TS_LOG_DEBUG("Payload length = %d\n",
			tcm_hcd->payload_length);

	if (tcm_hcd->status_report_code <= STATUS_ERROR ||
			tcm_hcd->status_report_code == STATUS_INVALID) {
		switch (tcm_hcd->status_report_code) {
		case STATUS_OK:
			break;
		case STATUS_CONTINUED_READ:
			TS_LOG_DEBUG("Out-of-sync continued read\n");
		case STATUS_IDLE:
		case STATUS_BUSY:
			memset(buffer +MESSAGE_HEADER_SIZE, 0x00, (FIXED_READ_LENGTH - MESSAGE_HEADER_SIZE));
			tcm_hcd->payload_length = 0;
			retval = 0;
			goto exit;
		default:
			TS_LOG_ERR("Incorrect header code (0x%02x)\n",
					tcm_hcd->status_report_code);
			if (tcm_hcd->status_report_code == STATUS_INVALID)
				tcm_hcd->payload_length = 0;
		}
	}

	if (tcm_hcd->status_report_code >= REPORT_IDENTIFY) {
		LOCK_BUFFER(tcm_hcd->report.buffer);

		tcm_hcd->report.buffer.buf = &buffer[MESSAGE_HEADER_SIZE];
		tcm_hcd->report.buffer.buf_size = tcm_hcd->payload_length;
		tcm_hcd->report.buffer.data_length = tcm_hcd->payload_length;
		tcm_hcd->report.id = tcm_hcd->status_report_code;

		touch_report(info);
		UNLOCK_BUFFER(tcm_hcd->report.buffer);
	}
	retval = 0;

exit:

	return retval;

}

static int syna_tcm_power_init(void)
{
	ts_kit_power_supply_get(TS_KIT_IOVDD);
	ts_kit_power_supply_get(TS_KIT_VCC);
	return 0;
}

static int syna_tcm_power_release(void)
{
	ts_kit_power_supply_put(TS_KIT_IOVDD);
	ts_kit_power_supply_put(TS_KIT_VCC);
	return 0;
}

static void syna_tcm_power_on(void)
{
	TS_LOG_INFO("syna_tcm_power_on called\n");
	ts_kit_power_supply_ctrl(TS_KIT_VCC, TS_KIT_POWER_ON, 5);
	ts_kit_power_supply_ctrl(TS_KIT_IOVDD, TS_KIT_POWER_ON, 5);

	syna_tcm_power_on_gpio_set();
	return;
}

static void syna_tcm_power_off(void)
{
	syna_tcm_power_off_gpio_set();
	ts_kit_power_supply_ctrl(TS_KIT_VCC, TS_KIT_POWER_OFF, 12);
	ts_kit_power_supply_ctrl(TS_KIT_IOVDD, TS_KIT_POWER_OFF, 30);
	return;
}

static int syna_tcm_suspend(void)
{
	int retval = NO_ERR;
	int tskit_pt_station_flag = 0;
	struct lcd_kit_ops *tp_ops = lcd_kit_get_ops();

	if (tcm_hcd->in_suspend)
			return 0;

	TS_LOG_INFO("%s +\n", __func__);

	if(g_ts_kit_platform_data.udfp_enable_flag) {
		TS_LOG_INFO("%s ud do nothing \n", __func__);
		tcm_hcd->ud_sleep_status = true;
		enable_irq(g_ts_kit_platform_data.irq_id);
		return 0;
	}

	if((tp_ops)&&(tp_ops->get_status_by_type)) {
		retval = tp_ops->get_status_by_type(PT_STATION_TYPE, &tskit_pt_station_flag);
		if(retval < 0) {
			TS_LOG_ERR("%s: get tskit_pt_station_flag fail\n", __func__);
			return retval;
		}
	}

	syna_tcm_pinctrl_select_lowpower();
	if (!tskit_pt_station_flag) {
		if(tcm_hcd->syna_tcm_chip_data->ts_platform_data->cs_gpio) {
			gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->cs_gpio, GPIO_OUTPUT_LOW);
		}
		gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, GPIO_OUTPUT_LOW);
		if (TS_BUS_SPI != tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
			syna_tcm_power_off();
		}
	} else {
		syna_tcm_sleep(tcm_hcd, true);	/*goto sleep mode*/
	}

	tcm_hcd->host_download_mode = false;
	tcm_hcd->in_suspend = true;

	return retval;
}

//   do not add time-costly function here.
static int syna_tcm_resume(void)
{
	int retval = NO_ERR;
	int tskit_pt_station_flag = 0;
	struct lcd_kit_ops *tp_ops = lcd_kit_get_ops();

	if (!tcm_hcd->in_suspend)
			return 0;

	TS_LOG_INFO("%s +\n", __func__);
	if(tcm_hcd->ud_sleep_status == true) {
		TS_LOG_INFO("%s udfp_return \n", __func__);
		return 0;
	}

	if((tp_ops)&&(tp_ops->get_status_by_type)) {
		retval = tp_ops->get_status_by_type(PT_STATION_TYPE, &tskit_pt_station_flag);
		if(retval < 0) {
			TS_LOG_ERR("%s: get tskit_pt_station_flag fail\n", __func__);
			return retval;
		}
	}

	syna_tcm_pinctrl_select_normal();
	if (!tskit_pt_station_flag) {
		if(tcm_hcd->syna_tcm_chip_data->ts_platform_data->cs_gpio) {
			gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->cs_gpio, GPIO_OUTPUT_HIGH);
		}
		if (TS_BUS_SPI != tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
			syna_tcm_power_on();
		}
		gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, GPIO_OUTPUT_HIGH);
	} else {
	/*** do host download***/
//		syna_tcm_sleep(tcm_hcd, false);	/*exit sleep mode*/
//		msleep(20);
	}
	if(TS_BUS_SPI != tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		retval = syna_tcm_enable_touch(tcm_hcd, true);
	}
	pre_finger_status = 0;
	tcm_hcd->in_suspend = false;
	return retval;
}

/*  do some things after power on. */
static int syna_tcm_after_resume(void *feature_info)
{
	int retval = NO_ERR;
	int retry = 3;
	TS_LOG_INFO("%s +\n", __func__);

	if(tcm_hcd->ud_sleep_status == true) {
		TS_LOG_INFO("%s do nothing\n", __func__);
		tcm_hcd->ud_sleep_status = false;
		goto exit;
	}
	if (tcm_hcd->init_okay) {
retry:
		retval = zeroflash_download(tcm_hcd->fw_name, tcm_hcd);
			if (retval) {
				TS_LOG_ERR("failed to download fw\n");
				gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, GPIO_OUTPUT_HIGH);
				mdelay(1);
				gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, GPIO_OUTPUT_LOW);
				udelay(300);
				gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, GPIO_OUTPUT_HIGH);
				mdelay(21);
				if(retry --) {
					TS_LOG_ERR("retry to HDL\n");
					goto retry;
				}
				goto exit;
			} else {
				TS_LOG_DEBUG("downloaded firmware successfully\n");
				tcm_hcd->host_download_mode = true;
			}
	} else if(get_into_recovery_flag_adapter()) {
		syna_tcm_fw_update_boot(tcm_hcd->fw_name);
	}

	syna_tcm_status_resume();
	if(tcm_hcd->tp_status_report_support)
		tcm_hcd->in_suspend_charge = false;
exit:
	return retval;
}

static int syna_tcm_before_suspend(void)
{
	int retval = NO_ERR;
	int tskit_pt_station_flag = 0;
	struct lcd_kit_ops *tp_ops = lcd_kit_get_ops();

	if((tp_ops)&&(tp_ops->get_status_by_type)) {
		retval = tp_ops->get_status_by_type(PT_STATION_TYPE, &tskit_pt_station_flag);
		if(retval < 0) {
			TS_LOG_ERR("%s: get tskit_pt_station_flag fail\n", __func__);
			return retval;
		}
	}
	if(tcm_hcd->tp_status_report_support && !tskit_pt_station_flag) {
		tcm_hcd->in_before_suspend = true;
		syna_tcm_hdl_get_state();
		syna_tcm_report_dmd_state_report();
		tcm_hcd->in_before_suspend = false;
		tcm_hcd->in_suspend_charge = true;
	}
	TS_LOG_INFO("%s +\n", __func__);
	if(g_ts_kit_platform_data.udfp_enable_flag) {
		TS_LOG_INFO("%s + ud function,return\n", __func__);
		return retval;
	}
	return retval;
}

static int syna_tcm_set_info_flag(struct ts_kit_platform_data *info)
{
	tcm_hcd->syna_tcm_chip_data->ts_platform_data->get_info_flag = info->get_info_flag;
	return NO_ERR;
}

static int syna_tcm_irq_top_half(struct ts_cmd_node *cmd)
{
	cmd->command = TS_INT_PROCESS;
	return NO_ERR;
}

static int syna_tcm_get_fw_prefix(void)
{
	char joint_chr = '_';
	char *fwname = tcm_hcd->fw_name;
	struct ts_kit_platform_data *ts_platform_data;
	ts_platform_data = tcm_hcd->syna_tcm_chip_data->ts_platform_data;

	if((strlen(ts_platform_data->product_name) + strlen(ts_platform_data->chip_data->chip_name) + 2*sizeof(char)) > MAX_STR_LEN) {
		TS_LOG_ERR("%s invalid fwname\n", __func__);
		return -EINVAL;
	}

	/*firmware name [product_name][ic_name][module][vendor]*/
	strncat(fwname, ts_platform_data->product_name, MAX_STR_LEN);
	strncat(fwname, &joint_chr, sizeof(char));
	strncat(fwname, ts_platform_data->chip_data->chip_name, MAX_STR_LEN);
	strncat(fwname, &joint_chr, sizeof(char));
	TS_LOG_INFO("%s fw name prefix:%s", __func__, fwname);

	return 0;

}

static int syna_tcm_fw_update_sd(void)
{
	int retval = NO_ERR;
	char *tmp_file_name = FW_IMAGE_NAME_SD;
	TS_LOG_INFO("%s is called\n", __func__);
	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		syna_tcm_status_resume();
	} else {
		syna_tcm_fw_update_boot(tmp_file_name);
	}
	return retval;
}

static int syna_tcm_fw_update_boot(char *file_name)
{
	int retval = NO_ERR;
	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		int projectid_lenth = 0;
		int retry = 3;

		if (tcm_hcd->syna_tcm_chip_data->projectid_len) {
			projectid_lenth = tcm_hcd->syna_tcm_chip_data->projectid_len;
		} else {
			projectid_lenth = PROJECT_ID_FW_LEN;
		}

		TS_LOG_INFO("syna_tcm_fw_update_boot called\n");

		retval = zeroflash_init(tcm_hcd);
		if (retval) {
			TS_LOG_ERR("zeroflash_init failed\n");
			goto data_release;
		}

		retval = zeroflash_get_fw_image(file_name);
		if (retval) {
			retval = 0;
			TS_LOG_ERR("load fw data from bootimage error\n");
			goto data_release;
		}

retry:
	retval = zeroflash_download(file_name, tcm_hcd);
	if (retval) {
		TS_LOG_ERR("failed to download fw retry\n");
		gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, GPIO_OUTPUT_HIGH);
		mdelay(1);
		gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, GPIO_OUTPUT_LOW);
		udelay(300);
		gpio_direction_output(tcm_hcd->syna_tcm_chip_data->ts_platform_data->reset_gpio, GPIO_OUTPUT_HIGH);
		mdelay(21);
		if(retry --) {
			TS_LOG_ERR("retry to HDL\n");
			goto retry;
		}
		goto data_release;
	} else {
		TS_LOG_INFO("downloaded firmware successfully\n");
		retval = touch_init(tcm_hcd);
			if (retval)
				TS_LOG_ERR("failed to touch_init\n");
	}
	tcm_hcd->host_download_mode = true;
	tcm_hcd->init_okay = true;
	syna_tcm_status_resume();
	return retval;

data_release:
	zeroflash_remove(tcm_hcd);
	} else {
		char *tmp_file_name = FW_IMAGE_NAME_SD;

		TS_LOG_ERR("syna_tcm_fw_update_boot %s\n", file_name);

		retval = reflash_do_reflash(tmp_file_name);
		if (retval < 0) {
			TS_LOG_ERR("Failed to do reflash\n");
		} else {
			TS_LOG_ERR("do reflash success\n");
			if (false == touch_init_status) {
				retval = touch_init(tcm_hcd);
				if (retval < 0) {
					TS_LOG_ERR("%s touch_init error.\n", __func__);
				}
			}
		}

	}
	return retval;
}

static int syna_tcm_irq_bottom_half(struct ts_cmd_node *in_cmd,
					struct ts_cmd_node *out_cmd)
{
	int retval = NO_ERR;
	struct ts_fingers *info =
		&out_cmd->cmd_param.pub_params.algo_param.info;

	out_cmd->command = TS_INPUT_ALGO;
	out_cmd->cmd_param.pub_params.algo_param.algo_order =
		tcm_hcd->syna_tcm_chip_data->algo_id;
	TS_LOG_DEBUG("order: %d\n",
			out_cmd->cmd_param.pub_params.algo_param.algo_order);
	tcm_hcd->esd_report_status = NOT_NEED_REPORT;
	retval = syna_tcm_read_one_package(info);
	if (retval < 0) {
		TS_LOG_ERR("Failed to syna_tcm_read_one_package, try to read F$35\n");
	}
	/* 'use_esd_report' setup in DTS; 'esd_report_status' setup in irq function */
	if(NEED_REPORT == tcm_hcd->esd_report_status && tcm_hcd->use_esd_report) {
		TS_LOG_INFO("%s, plam_key_report.\n", __func__);
		out_cmd->command = TS_PALM_KEY;
		out_cmd->cmd_param.pub_params.ts_key = TS_KEY_IRON;
	}

	return retval;
}

static int syna_tcm_input_config(struct input_dev *input_dev)
{
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);
	set_bit(TS_DOUBLE_CLICK, input_dev->keybit);
	set_bit(TS_SLIDE_L2R, input_dev->keybit);
	set_bit(TS_SLIDE_R2L, input_dev->keybit);
	set_bit(TS_SLIDE_T2B, input_dev->keybit);
	set_bit(TS_SLIDE_B2T, input_dev->keybit);
	set_bit(TS_CIRCLE_SLIDE, input_dev->keybit);
	set_bit(TS_LETTER_c, input_dev->keybit);
	set_bit(TS_LETTER_e, input_dev->keybit);
	set_bit(TS_LETTER_m, input_dev->keybit);
	set_bit(TS_LETTER_w, input_dev->keybit);
	set_bit(TS_PALM_COVERED, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY0, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY1, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY2, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY3, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY4, input_dev->keybit);
	set_bit(TS_KEY_IRON, input_dev->keybit);/* report ESD EVENT */

#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
#endif

	input_set_abs_params(input_dev, ABS_X,
				0, bdata->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_Y,
				0, bdata->y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 15, 0, 0);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0,
				bdata->x_max_mt, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0,
				bdata->y_max_mt, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
#ifdef ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 100, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MINOR, 0, 100, 0, 0);
#else
	input_set_abs_params(input_dev, ABS_MT_DISTANCE, 0, 100, 0, 0);
#endif
#ifdef REPORT_2D_W
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0,
				MAX_ABS_MT_TOUCH_MAJOR, 0, 0);
#endif

#ifdef TYPE_B_PROTOCOL
#ifdef KERNEL_ABOVE_3_7
	// input_mt_init_slots now has a "flags" parameter
	input_mt_init_slots(input_dev, bdata->max_finger_objects,
				INPUT_MT_DIRECT);
#else
	input_mt_init_slots(input_dev, bdata->max_finger_objects);
#endif
#endif

	return NO_ERR;
}

static void syna_tcm_parse_basic_dts(struct device_node *np, struct ts_kit_device_data *chip_data)
{
	int retval = NO_ERR;
	u32 value = 0;
	struct property *prop = NULL;
	const char *name = NULL;

	retval = of_property_read_u32(np, "synaptics,irq-on-state", &value);
	if (retval < 0)
		bdata->irq_on_state = 0;
	else
		bdata->irq_on_state = value;

	retval = of_property_read_string(np, "synaptics,pwr-reg-name", &name);
	if (retval < 0)
		bdata->pwr_reg_name = NULL;
	else
		bdata->pwr_reg_name = name;

	retval = of_property_read_string(np, "synaptics,bus-reg-name", &name);
	if (retval < 0)
		bdata->bus_reg_name = NULL;
	else
		bdata->bus_reg_name = name;

	prop = of_find_property(np, "synaptics,x-flip", NULL);
	bdata->x_flip = prop > 0 ? true : false;

	prop = of_find_property(np, "synaptics,y-flip", NULL);
	bdata->y_flip = prop > 0 ? true : false;

	prop = of_find_property(np, "synaptics,swap-axes", NULL);
	bdata->swap_axes = prop > 0 ? true : false;

	prop = of_find_property(np, "synaptics,byte-delay-us", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,byte-delay-us",
				&value);
		if (retval < 0) {
			TS_LOG_ERR("Unable to read synaptics,byte-delay-us property\n");
			return;
		} else {
			bdata->byte_delay_us = value;
		}
	} else {
		bdata->byte_delay_us = 0;
	}

	retval = of_property_read_u32(np, "x_max", &bdata->x_max);
	if (retval) {
		TS_LOG_ERR("get device x_max failed\n");
		bdata->x_max =0;
	}
	retval = of_property_read_u32(np, "y_max", &bdata->y_max);
	if (retval) {
		TS_LOG_ERR("get device y_max failed\n");
		bdata->y_max =0;
	}

	retval = of_property_read_u32(np, "x_max_mt", &bdata->x_max_mt);
	if (retval) {
		TS_LOG_ERR("get device x_max failed\n");
		bdata->x_max_mt =0;
	}
	retval = of_property_read_u32(np, "y_max_mt", &bdata->y_max_mt);
	if (retval) {
		TS_LOG_ERR("get device y_max failed\n");
		bdata->y_max_mt =0;
	}

	retval = of_property_read_u32(np, "max_finger_objects", &bdata->max_finger_objects);
	if (retval) {
		TS_LOG_ERR("get device y_max failed\n");
		bdata->max_finger_objects = 0;
	}

	retval = of_property_read_string(np, "default_project_id", &bdata->default_project_id);
	if (retval < 0) {
		bdata->default_project_id = NULL;
	}
	return;
}

static void syna_tcm_parse_power_dts(struct device_node *np, struct ts_kit_device_data *chip_data)
{
	int retval = NO_ERR;
	u32 value = 0;
	struct property *prop = NULL;

	prop = of_find_property(np, "synaptics,power-gpio", NULL);
	if (prop && prop->length) {
		bdata->power_gpio = of_get_named_gpio_flags(np,
				"synaptics,power-gpio", 0, NULL);
	} else {
		bdata->power_gpio = -1;
	}

	prop = of_find_property(np, "synaptics,power-on-state", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,power-on-state",
				&value);
		if (retval < 0) {
			TS_LOG_ERR("Failed to read synaptics,power-on-state property\n");
			return;
		} else {
			bdata->power_on_state = value;
		}
	} else {
		bdata->power_on_state = 0;
	}


	prop = of_find_property(np, "synaptics,power-delay-ms", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,power-delay-ms",
				&value);
		if (retval < 0) {
			TS_LOG_ERR("Failed to read synaptics,power-delay-ms property\n");
			return;
		} else {
			bdata->power_delay_ms = value;
		}
	} else {
		bdata->power_delay_ms = 0;
	}

	prop = of_find_property(np, "synaptics,reset-on-state", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,reset-on-state",
				&value);
		if (retval < 0) {
			TS_LOG_ERR("Failed to read synaptics,reset-on-state property\n");
			return;
		} else {
			bdata->reset_on_state = value;
		}
	} else {
		bdata->reset_on_state = 0;
	}

	prop = of_find_property(np, "synaptics,reset-active-ms", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,reset-active-ms",
				&value);
		if (retval < 0) {
			TS_LOG_ERR("Failed to read synaptics,reset-active-ms property\n");
			return;
		} else {
			bdata->reset_active_ms = value;
		}
	} else {
		bdata->reset_active_ms = 0;
	}

	prop = of_find_property(np, "synaptics,reset-delay-ms", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,reset-delay-ms",
				&value);
		if (retval < 0) {
			TS_LOG_ERR("Unable to read synaptics,reset-delay-ms property\n");
			return;
		} else {
			bdata->reset_delay_ms = value;
		}
	} else {
		bdata->reset_delay_ms = 0;
	}



	prop = of_find_property(np, "synaptics,block-delay-us", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,block-delay-us",
				&value);
		if (retval < 0) {
			TS_LOG_ERR("Unable to read synaptics,block-delay-us property\n");
			return;
		} else {
			bdata->block_delay_us = value;
		}
	} else {
		bdata->block_delay_us = 0;
	}

	prop = of_find_property(np, "synaptics,spi-mode", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,spi-mode",
				&value);
		if (retval < 0) {
			TS_LOG_ERR("Unable to read synaptics,spi-mode property\n");
			return;
		} else {
			bdata->spi_mode = value;
		}
	} else {
		bdata->spi_mode = 0;
	}

	prop = of_find_property(np, "synaptics,ubl-max-freq", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,ubl-max-freq",
				&value);
		if (retval < 0) {
			TS_LOG_ERR("Unable to read synaptics,ubl-max-freq property\n");
			return;
		} else {
			bdata->ubl_max_freq = value;
		}
	} else {
		bdata->ubl_max_freq = 0;
	}

	prop = of_find_property(np, "synaptics,ubl-byte-delay-us", NULL);
	if (prop && prop->length) {
		retval = of_property_read_u32(np, "synaptics,ubl-byte-delay-us",
				&value);
		if (retval < 0) {
			TS_LOG_ERR("Unable to read synaptics,ubl-byte-delay-us property\n");
			return;
		} else {
			bdata->ubl_byte_delay_us = value;
		}
	} else {
		bdata->ubl_byte_delay_us = 0;
	}
	return;
}

static void syna_tcm_parse_feature_dts(struct device_node *np, struct ts_kit_device_data *chip_data)
{
	int retval = NO_ERR;

	if (NULL == tcm_hcd) {
		TS_LOG_ERR("%s Invalid tcm_hcd data.\n", __func__);
		return;
	}

	retval = of_property_read_u32(np, "tp_status_report_support", &tcm_hcd->tp_status_report_support);
	if(retval) {
		tcm_hcd->tp_status_report_support = 0;
	}
	TS_LOG_INFO("tp_status_report_support is %d", tcm_hcd->tp_status_report_support);
	if(tcm_hcd->tp_status_report_support) {
		retval = of_property_read_u32_array(np, "report_priority", &syna_report_priority[0], BIT_MAX);
		if(retval) {
			TS_LOG_INFO("device get syna_report_priority failed, use default value.\n");
		} else {
			int i = 0;
			for(i =0; i < BIT_MAX; i++)
			{
				TS_LOG_INFO("syna_report_priority [%d] = %d.\n", i, syna_report_priority[i]);
			}
			TS_LOG_INFO("device get syna_report_priority success.\n");
		}
		retval = of_property_read_u32_array(np, "report_priority_limite", &syna_report_priority_limite[0], BIT_MAX);
		if(retval) {
			TS_LOG_INFO("device get syna_report_priority_limite failed, use default value.\n");
		} else {
			int i = 0;
			for(i =0; i < BIT_MAX; i++)
			{
				TS_LOG_INFO("syna_report_priority_limite [%d] = %d.\n", i, syna_report_priority_limite[i]);
			}
			TS_LOG_INFO("device get syna_report_priority_limite success.\n");
		}
	}
	retval = of_property_read_u32(np, "aft_wxy_enable", &tcm_hcd->aft_wxy_enable);
	if(retval) {
		tcm_hcd->aft_wxy_enable = 0;
	}
	TS_LOG_INFO("use dts(retval = %d) for aft_wxy_enable = %d\n", retval, tcm_hcd->aft_wxy_enable);

	retval = of_property_read_u32(np, "use_esd_report", &tcm_hcd->use_esd_report);
	if(retval) {
		tcm_hcd->use_esd_report = 0;
	}
	TS_LOG_INFO("use dts(retval = %d) for use_esd_report = %d\n", retval, tcm_hcd->use_esd_report);


	retval = of_property_read_u32(np, "use_dma_download_firmware", &tcm_hcd->use_dma_download_firmware);
	if(retval) {
		tcm_hcd->use_dma_download_firmware = 0;
	}

	if(tcm_hcd->use_dma_download_firmware) {
		retval = of_property_read_u32(np, "downmload_firmware_frequency", &tcm_hcd->downmload_firmware_frequency);
		if(retval) {
			tcm_hcd->downmload_firmware_frequency = SPI_DEFLAUT_SPEED;
		}
	}
	TS_LOG_INFO("use_dma_download_firmware = %d downmload_firmware_frequency = %d\n",
		tcm_hcd->use_dma_download_firmware,tcm_hcd->downmload_firmware_frequency);
}


static int syna_tcm_parse_dts(struct device_node *np, struct ts_kit_device_data *chip_data)
{
	syna_tcm_parse_basic_dts(np,chip_data);

	syna_tcm_parse_power_dts(np,chip_data);

	syna_tcm_parse_feature_dts(np,chip_data);

	return 0;
}

static int syna_tcm_init_chip(void)
{
	int retval = NO_ERR;
	char buf_proj_id[CHIP_INFO_LENGTH] = {0};
	int projectid_lenth = 0;

	if (!tcm_hcd)
		return -EINVAL;

	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		if (tcm_hcd->syna_tcm_chip_data->projectid_len) {
			projectid_lenth = tcm_hcd->syna_tcm_chip_data->projectid_len;
		} else {
			projectid_lenth = PROJECT_ID_FW_LEN;
		}

		strncpy(tcm_hcd->syna_tcm_chip_data->chip_name,SYNAPTICS_VENDER_NAME,
				MIN(MAX_STR_LEN, strlen(SYNAPTICS_VENDER_NAME)+1));

		retval = synaptics_tcm_get_project_id(buf_proj_id);
		if (retval) {
			TS_LOG_ERR("%s get project id error.\n", __func__);
			return -EINVAL;
		}
		strncpy(tcm_hcd->syna_tcm_chip_data->module_name,buf_proj_id,projectid_lenth);
		memcpy(tcm_hcd->tcm_mod_info.project_id_string, buf_proj_id, projectid_lenth);
		syna_tcm_get_fw_prefix();
	}else{
		retval = touch_init(tcm_hcd);
		if (retval) {
			touch_init_status = false;
		}
		reflash_init(tcm_hcd);
	}
	#if defined (CONFIG_TEE_TUI)
		strncpy(tee_tui_data.device_name, "syna_tcm", strlen("syna_tcm"));
		tee_tui_data.device_name[strlen("syna_tcm")] = '\0';
	#endif
	retval = debug_device_init(tcm_hcd);
	return retval;
}

static int syna_tcm_chip_detect(struct ts_kit_platform_data* data)
{
	int retval = NO_ERR;
	u32 tmp_spi_mode = SPI_MODE_0;

	TS_LOG_INFO(" syna_tcm_chip_detect called !\n");
	tcm_hcd->syna_tcm_chip_data->ts_platform_data = data;
	tcm_hcd->pdev = data->ts_dev;
	tcm_hcd->pdev->dev.of_node = tcm_hcd->syna_tcm_chip_data->cnode;
	tcm_hcd->reset = syna_tcm_reset;
	tcm_hcd->sleep = syna_tcm_sleep;
	tcm_hcd->identify = syna_tcm_identify;
	tcm_hcd->switch_mode = syna_tcm_switch_mode;
	tcm_hcd->read_message = syna_tcm_read_message;
	tcm_hcd->write_message = syna_tcm_write_message;
	tcm_hcd->get_dynamic_config = syna_tcm_get_dynamic_config;
	tcm_hcd->set_dynamic_config = syna_tcm_set_dynamic_config;
	tcm_hcd->get_data_location = syna_tcm_get_data_location;

	if (TS_BUS_SPI != tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		tcm_hcd->read = syna_tcm_i2c_read;
		tcm_hcd->write = syna_tcm_i2c_write;
		tcm_hcd->rmi_read = syna_tcm_i2c_rmi_read;
		tcm_hcd->rmi_write = syna_tcm_i2c_rmi_write;
		tcm_hcd->rd_chunk_size = RD_CHUNK_SIZE_I2C;
		tcm_hcd->wr_chunk_size = WR_CHUNK_SIZE_I2C;
	}else{
		tcm_hcd->read = syna_tcm_spi_read;
		tcm_hcd->write = syna_tcm_spi_write;
		tcm_hcd->rmi_read = syna_tcm_spi_rmi_read;
		tcm_hcd->rmi_write = syna_tcm_spi_rmi_write;
		tcm_hcd->rd_chunk_size = RD_CHUNK_SIZE_SPI;
		tcm_hcd->wr_chunk_size = WR_CHUNK_SIZE_SPI;
	}

#ifdef PREDICTIVE_READING
	tcm_hcd->read_length = MIN_READ_LENGTH;
#else
	tcm_hcd->read_length = MESSAGE_HEADER_SIZE;
#endif

	tcm_hcd->watchdog.run = RUN_WATCHDOG;
	tcm_hcd->update_watchdog = syna_tcm_update_watchdog;

	INIT_BUFFER(tcm_hcd->in, false);
	INIT_BUFFER(tcm_hcd->out, false);
	INIT_BUFFER(tcm_hcd->resp, true);
	INIT_BUFFER(tcm_hcd->temp, false);
	INIT_BUFFER(tcm_hcd->config, false);
	INIT_BUFFER(tcm_hcd->report.buffer, true);

	LOCK_BUFFER(tcm_hcd->in);
	retval = syna_tcm_alloc_mem(tcm_hcd,
			&tcm_hcd->in,
			tcm_hcd->read_length + 1);
	if (retval < 0) {
		TS_LOG_ERR("Failed to allocate memory for tcm_hcd->in.buf\n");
		UNLOCK_BUFFER(tcm_hcd->in);
		goto err_alloc_mem;
	}
	UNLOCK_BUFFER(tcm_hcd->in);

	atomic_set(&tcm_hcd->command_status, CMD_IDLE);
	atomic_set(&tcm_hcd->helper.task, HELP_NONE);
	device_init_wakeup(&data->ts_dev->dev, 1);
	init_waitqueue_head(&tcm_hcd->hdl_wq);

	syna_tcm_parse_dts(tcm_hcd->pdev->dev.of_node, tcm_hcd->syna_tcm_chip_data);

	retval = syna_tcm_power_init();
	if (retval < 0) {
		TS_LOG_ERR("syna_tcm_get_regulator error %d \n",retval);
		goto err_alloc_mem;
	}

	retval = syna_tcm_gpio_request();
	if (retval < 0) {
		TS_LOG_ERR("synaptics_gpio_request error %d \n",retval);
		goto gpio_err;
	}

	retval = syna_tcm_pinctrl_init();
	if (retval < 0) {
		TS_LOG_ERR("%s: syna_tcm_pinctrl_init error %d \n", __func__, retval);
	}

	syna_tcm_power_on();
	/*reset the chip */
	syna_tcm_gpio_reset();
	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		tmp_spi_mode = data->spi->mode;
		retval = ts_change_spi_mode(data->spi, bdata->spi_mode);
		if (retval) {
			goto check_err;
		}
	}
	retval = syna_tcm_comm_check();
	if (retval < 0) {
		TS_LOG_ERR("Failed to syna_tcm_comm_check\n");
		goto check_err;
	}

	tcm_hcd->syna_tcm_chip_data->vendor_name = SYNA_VENDOR_NAME;
	return 0;

check_err:
	syna_tcm_power_off();
	syna_tcm_power_release();
	syna_tcm_pinctrl_release();
gpio_err:
	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype) {
		data->spi->max_speed_hz = g_ts_kit_platform_data.spi_max_frequency;
		ts_change_spi_mode(data->spi, tmp_spi_mode);
	}
err_alloc_mem:
	RELEASE_BUFFER(tcm_hcd->report.buffer);
	RELEASE_BUFFER(tcm_hcd->config);
	RELEASE_BUFFER(tcm_hcd->temp);
	RELEASE_BUFFER(tcm_hcd->resp);
	RELEASE_BUFFER(tcm_hcd->out);
	RELEASE_BUFFER(tcm_hcd->in);

	if(tcm_hcd->syna_tcm_chip_data) {
		kfree(tcm_hcd->syna_tcm_chip_data);
		tcm_hcd->syna_tcm_chip_data = NULL;
	}
	if (tcm_hcd) {
		kfree(tcm_hcd);
		tcm_hcd = NULL;
	}
	TS_LOG_ERR("detect synaptics error\n");

	return retval;
}

static int syna_tcm_roi_switch(struct ts_roi_info *info)
{
	int retval = NO_ERR;
	u16 buf = 0;

	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype
			&& tcm_hcd->host_download_mode == false) {
		TS_LOG_INFO("Bypass set roi command\n");
		return 0;
	}

	if (!info) {
		TS_LOG_ERR("synaptics_roi_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}

	if(tcm_hcd->ud_sleep_status == true) {
		TS_LOG_INFO("ud enable in sleep mode return\n");
		return retval;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		if(0==info->roi_switch) {
			buf = 0;
		} else {
			buf = 1;
		}
		retval = syna_tcm_set_dynamic_config(tcm_hcd,
				info->roi_control_addr, buf);
		if (retval < 0) {
			TS_LOG_ERR("%s, synaptics_set_roi_switch faild\n",
				   __func__);
		}else{
			tcm_hcd->roi_enable_status = buf;
		}

		if (0 == tcm_hcd->roi_enable_status) {
			memset(tcm_hcd->syna_tcm_roi_data, 0, sizeof(tcm_hcd->syna_tcm_roi_data));
		}
		break;
	case TS_ACTION_READ:
		info->roi_switch = tcm_hcd->roi_enable_status;
		break;
	default:
		TS_LOG_INFO("invalid roi switch(%d) action: %d\n",
			    info->roi_switch, info->op_action);
		retval = -EINVAL;
		break;
	}

	TS_LOG_INFO("syna_tcm_roi_switch end info->roi_switch = %d\n",info->roi_switch);
	return retval;
}

static unsigned char *syna_tcm_roi_rawdata(void)
{
	return tcm_hcd->syna_tcm_roi_data ;
}

static int syna_tcm_charger_switch(struct ts_charger_info *info)
{
	int retval = NO_ERR;
	u16 buf = 0;

	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype
			&& tcm_hcd->host_download_mode == false) {
		TS_LOG_INFO("Bypass set charger command\n");
		return retval;
	}

	if (!info) {
		TS_LOG_ERR("synaptics_charger_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}

	switch (info->op_action) {
	case TS_ACTION_READ:
		retval = syna_tcm_get_dynamic_config(tcm_hcd,
				DC_CHARGER_CONNECTED, &buf);
		if (retval < 0) {
			TS_LOG_ERR("get charger switch(%d), failed : %d",
				   info->charger_switch, retval);
			break;
		}
		info->charger_switch = buf;
		TS_LOG_INFO("read_charger_switch=%d, 1:on 0:off\n",
			    info->charger_switch);
		break;
	case TS_ACTION_WRITE:
		buf = info->charger_switch;
		TS_LOG_INFO("write_charger_switch=%d\n", info->charger_switch);
		if ((CHARGER_SWITCH_ON != info->charger_switch)
		    && (CHARGER_SWITCH_OFF != info->charger_switch)) {
			TS_LOG_ERR("write wrong state: buf = %d\n", buf);
			retval = -EFAULT;
			break;
		}
		retval = syna_tcm_set_dynamic_config(tcm_hcd,
				DC_CHARGER_CONNECTED, buf);
		if (retval < 0) {
			TS_LOG_ERR("set charger switch(%d), failed : %d", buf,
				   retval);
		}
		if(tcm_hcd->tp_status_report_support && CHARGER_SWITCH_ON == info->charger_switch && !tcm_hcd->in_suspend_charge) {
			TS_LOG_INFO("change charge to no");
			syna_dmd_charge_info.charge_CHARGER_NOISE_HOP = CHARGE_REPORT_NOT_REPORT;
			syna_dmd_charge_info.charge_CHARGER_NOISE_EX= CHARGE_REPORT_NOT_REPORT;
		}
		break;
	default:
		TS_LOG_ERR("invalid switch status: %d", info->charger_switch);
		retval = -EINVAL;
		break;
	}

	return retval;
}

static int syna_tcm_glove_switch(struct ts_glove_info *info)
{
	int retval = NO_ERR;
	u16 buf = 0;

	if (TS_BUS_SPI == tcm_hcd->syna_tcm_chip_data->ts_platform_data->bops->btype
			&& tcm_hcd->host_download_mode == false) {
		TS_LOG_INFO("Bypass set glove command\n");
		return 0;
	}

	if (!info) {
		TS_LOG_ERR("synaptics_glove_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}

	if (!info->glove_supported) {
		TS_LOG_INFO("%s: not support glove\n", __func__);
		return NO_ERR;
	}

	switch (info->op_action) {
	case TS_ACTION_READ:
		retval = syna_tcm_get_dynamic_config(tcm_hcd,
				info->glove_switch_addr, &buf);
		if (retval < 0) {
			TS_LOG_ERR("get glove switch(%d), failed : %d",
				   info->glove_switch, retval);
			break;
		}
		info->glove_switch = buf;
		TS_LOG_INFO("read_glove_switch=%d, 1:on 0:off\n",
			    info->glove_switch);
		break;
	case TS_ACTION_WRITE:
		buf = info->glove_switch;
		TS_LOG_INFO("write_glove_switch=%d\n", info->glove_switch);
		if ((GLOVE_SWITCH_ON != info->glove_switch)
		    && (GLOVE_SWITCH_OFF != info->glove_switch)) {
			TS_LOG_ERR("write wrong state: buf = %d\n", buf);
			retval = -EFAULT;
			break;
		}
		retval = syna_tcm_set_dynamic_config(tcm_hcd,
				info->glove_switch_addr, buf);
		if (retval < 0) {
			TS_LOG_ERR("set glove switch(%d), failed : %d", buf,
				   retval);
		}
		break;
	default:
		TS_LOG_ERR("invalid switch status: %d", info->glove_switch);
		retval = -EINVAL;
		break;
	}
	return retval;

}

#define SYNA_SWITCH_MAX_INPUT_SEPARATE_NUM 2
static void synaptics_tcm_chip_touch_switch(void)
{
	int retval = 0;
	char in_data[MAX_STR_LEN] = {0};
	unsigned int stype = 0, soper = 0, para = 0;
	int error = 0;
	unsigned int i = 0, cnt = 0;
	u16 buf = 0;
	struct ts_kit_device_data *syna_tcm_dev_data = NULL;

	TS_LOG_INFO("%s enter\n", __func__);

	if(tcm_hcd->ud_sleep_status == true) {
		TS_LOG_INFO("ud enable when chip touch switch return\n");
		return retval;
	}

	if (NULL == tcm_hcd || NULL == tcm_hcd->syna_tcm_chip_data){
		TS_LOG_ERR("%s, error chip data or hcd data\n",__func__);
		goto out;
	}

	syna_tcm_dev_data = tcm_hcd->syna_tcm_chip_data;

	/* SWITCH_OPER,ENABLE_DISABLE,PARAM */
	memcpy(in_data, syna_tcm_dev_data->touch_switch_info, MAX_STR_LEN -1);
	TS_LOG_INFO("%s, in_data:%s\n",__func__, in_data);
	for(i = 0; i < strlen(in_data) && (in_data[i] != '\n'); i++){
		if(in_data[i] == ','){
			cnt++;
		}else if(!isdigit(in_data[i])){
			TS_LOG_ERR("%s: input format error!!\n", __func__);
			goto out;
		}
	}
	if(cnt != SYNA_SWITCH_MAX_INPUT_SEPARATE_NUM){
		TS_LOG_ERR("%s: input format error[separation_cnt=%d]!!\n", __func__, cnt);
		goto out;
	}

	error = sscanf(in_data, "%u,%u,%u", &stype, &soper, &para);
	if(error <= 0){
		TS_LOG_ERR("%s: sscanf error\n", __func__);
		goto out;
	}
	TS_LOG_DEBUG("stype=%u,soper=%u,param=%u\n", stype, soper, para);

	/**
	 * enter DOZE again after a period of time. the min unit of the time is 100ms,
	 * for example, we set 30, the final time is 30 * 100 ms = 3s
	 * The min unit to enter doze_mode is second for focaltech
	**/
	if (stype >= TS_SWITCH_SCENE_3 && stype <= TS_SWITCH_SCENE_20) {
		if (TS_SWITCH_TYPE_SCENE != (syna_tcm_dev_data->touch_switch_flag & TS_SWITCH_TYPE_SCENE)) {
			TS_LOG_ERR("%s, scene switch does not suppored by this chip\n",__func__);
			goto out;
		}

		switch (soper) {
			case TS_SWITCH_SCENE_ENTER:
				buf = stype;
				retval = syna_tcm_set_dynamic_config(tcm_hcd,
						syna_tcm_dev_data->touch_scene_reg, buf);
				if (retval < 0) {
					TS_LOG_ERR("%s, synaptics_set_roi_switch faild\n",
							__func__);
				}
				TS_LOG_INFO("%s: enter scene %d\n", __func__, stype);
				break;
			case TS_SWITCH_SCENE_EXIT:
				buf = 0;
				retval = syna_tcm_set_dynamic_config(tcm_hcd,
						syna_tcm_dev_data->touch_scene_reg, buf);
				if (retval < 0) {
					TS_LOG_ERR("%s, synaptics_set_roi_switch faild\n",
							__func__);
				}
				TS_LOG_INFO("%s: enter default scene\n", __func__);
				break;
			default:
				TS_LOG_ERR("%s: soper unknown:%d, invalid\n", __func__, soper);
				break;
		}
	}

out:
	return;
}
static int syna_tcm_status_resume(void)
{
	int retval = 0;
	struct ts_feature_info *info = &tcm_hcd->syna_tcm_chip_data->ts_platform_data->feature_info;
	struct ts_roi_info roi_info = info->roi_info;
	struct ts_glove_info glove_info = info->glove_info;
	struct ts_charger_info charger_info = info->charger_info;

	if (roi_info.roi_supported) {
		roi_info.op_action = TS_ACTION_WRITE;
		retval = syna_tcm_roi_switch(&roi_info);
		if (retval < 0) {
			TS_LOG_ERR("%s, synaptics_set_roi_switch faild\n", __func__);
		}
	}

	if(glove_info.glove_supported){
		glove_info.op_action = TS_ACTION_WRITE;
		retval = syna_tcm_glove_switch(&glove_info);
		if (retval < 0) {
			TS_LOG_ERR("Failed to set glove switch(%d), err: %d\n",
					info->glove_info.glove_switch, retval);
		}
	}

	if (charger_info.charger_supported) {
		charger_info.op_action = TS_ACTION_WRITE;
		retval = syna_tcm_charger_switch(&charger_info);
		if (retval < 0) {
			TS_LOG_ERR("Failed to set charger switch(%d), err: %d\n",
					info->charger_info.charger_switch, retval);
		}
	}
	TS_LOG_INFO(" glove_switch (%d), roi_switch(%d), charger_switch(%d)\n",
				info->glove_info.glove_switch,info->roi_info.roi_switch, info->charger_info.charger_switch);
	return retval;
}

static int __init syna_tcm_module_init(void)
{
	int retval = NO_ERR;
	bool found = false;
	struct device_node* child = NULL;
	struct device_node* root = NULL;

	TS_LOG_INFO(" syna_tcm_ts_module_init called here\n");

	root = of_find_compatible_node(NULL, NULL, "huawei,ts_kit");
	if (!root) {
		TS_LOG_ERR("huawei_ts, find_compatible_node huawei,ts_kit error\n");
		return -EINVAL;
	}

	for_each_child_of_node(root, child)
	{
		if (of_device_is_compatible(child, SYNAPTICS_VENDER_NAME)) {
			TS_LOG_INFO("found is true\n");
			found = true;
			break;
		}
	}

	if (!found) {
		TS_LOG_ERR(" not found chip synaptics_tcm child node  !\n");
		return -EINVAL;
	}

	tcm_hcd = kzalloc(sizeof(struct syna_tcm_hcd), GFP_KERNEL);
	if (!tcm_hcd) {
		TS_LOG_ERR("Failed to allocate memory for tcm_hcd\n");
		return -ENOMEM;
	}

	tcm_hcd->syna_tcm_chip_data = kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
	if (!tcm_hcd->syna_tcm_chip_data) {
		TS_LOG_ERR("Failed to allocate memory for tcm_hcd\n");
		retval = -ENOMEM;
		goto err_free_hcd;
	}

	tcm_hcd->syna_tcm_chip_data->cnode = child;
	tcm_hcd->syna_tcm_chip_data->ops = &ts_kit_syna_tcm_ops;

	bdata = kzalloc(sizeof(struct syna_tcm_board_data), GFP_KERNEL);
	if (!bdata) {
		TS_LOG_ERR("Failed to allocate memory for board data\n");
		retval = -ENOMEM;
		goto err_free_chip_data;
	}

	tcm_hcd->bdata = bdata;
	tcm_hcd->read = syna_tcm_spi_read;
	tcm_hcd->write = syna_tcm_spi_write;
	tcm_hcd->rmi_read = syna_tcm_spi_rmi_read;
	tcm_hcd->rmi_write = syna_tcm_spi_rmi_write;
	tcm_hcd->host_download_mode = false;

	retval = huawei_ts_chip_register(tcm_hcd->syna_tcm_chip_data);
	if(retval)
	{
		TS_LOG_ERR(" synaptics chip register fail !\n");
		goto err_free_bdata;
	}

	return retval;

err_free_bdata:
	if (bdata) {
		kfree(bdata);
		bdata = NULL;
	}
err_free_chip_data:
	if (tcm_hcd->syna_tcm_chip_data) {
		kfree(tcm_hcd->syna_tcm_chip_data);
		tcm_hcd->syna_tcm_chip_data = NULL;
	}
err_free_hcd:
	if (tcm_hcd) {
		kfree(tcm_hcd);
		tcm_hcd = NULL;
	}
	return retval;
}

static void __exit syna_tcm_module_exit(void)
{
	return;
}

module_init(syna_tcm_module_init);
module_exit(syna_tcm_module_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics TCM Touch Driver");
MODULE_LICENSE("GPL v2");
