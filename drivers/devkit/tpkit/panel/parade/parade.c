#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/limits.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <asm/unaligned.h>
#include <linux/firmware.h>
#include <linux/ctype.h>
#include <linux/atomic.h>

#include <linux/of_gpio.h>
#include "parade.h"
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif
#if defined (CONFIG_TEE_TUI)
#include "tui.h"
#endif
#include "../../huawei_ts_kit_api.h"
#include "../../../lcdkit/lcdkit1.0/include/lcdkit_panel.h"

extern volatile int g_ts_kit_lcd_brightness_info;
extern struct ts_kit_platform_data g_ts_kit_platform_data;
extern unsigned int panel_name_flag;
#if defined (CONFIG_TEE_TUI)
extern struct ts_tui_data tee_tui_data;
#endif
#define PARADE_POWER_ON_ROI_SWITCH 0

#define TP_COLOR_SIZE   15
#define TP_COLOR_READ_SIZE 5
#define PARADE_OEMINFO_LEN_PER_LINE 16
#define PARADE_USE_CMCP_FAILURE_EXPLANATION 0
#define STATUS_SUCCESS	0
#define STATUS_FAIL	-1
#define PIP_CMD_MAX_LENGTH ((1 << 16) - 1)
#define ABS(x)			(((x) < 0) ? -(x) : (x))
#define PutStringToReasonBuf(x) memcpy(tp_test_failed_reason, x,(strlen(x) + 1))
#define AddReasonToBuf(x)	\
do { \
	if (PARADE_USE_CMCP_FAILURE_EXPLANATION)       \
		strncat(tp_test_failed_reason, x,strlen(x));	\
} while (0)
#define AddtoMMIResult(x) strncat(buf_parademmitest_result, x,strlen(x))
#define PARADE_VENDER_NAME  "parade"
/*parade internal declarations start*/
static int parade_loader_probe(struct parade_core_data *cd);
static int parade_device_access_probe(struct parade_core_data *cd);
static int parade_startup(struct parade_core_data *cd, bool reset);
static int parade_hid_send_output_and_wait_(struct parade_core_data *cd, struct parade_hid_output *hid_output);
static int parade_core_suspend(void);
static int parade_core_resume(void);

static u16 _parade_compute_crc(u8 *buf, u32 size);
static int parade_oem_read_nv_data(struct parade_oem_data *parade_oem_data, u8 buf[], int offset, int length);
static int parade_exec_panel_scan_cmd_(struct device *dev);
static int parade_ret_scan_data_cmd_(struct device *dev, u16 read_offset,
		u16 read_count, u8 data_id, u8 *response, u8 *config,
		u16 *actual_read_len, u8 *return_buf);
static int parade_core_poweroff_device_(struct parade_core_data *cd);
static int parade_put_device_into_deep_sleep_(struct parade_core_data *cd);
static int parade_hid_cmd_set_power_(struct parade_core_data *cd,
				      u8 power_state);

/*parade internal declarations end*/
static int parade_regulator_get(void);
static void parade_regulator_put(void);
static int parade_set_voltage(void);
static int parade_gpio_request(void);
static void parade_gpio_free(void);
static int parade_power_on(void);
static int parade_power_off(void);
static int parade_read_input(struct parade_core_data *cd);

static int parade_chip_detect(struct ts_kit_platform_data *platform_data);
static int parade_init_chip(void);
static int parade_parse_dts(struct device_node *device, struct ts_kit_device_data *chip_data);
static int parade_input_config(struct input_dev *input_dev);
static int parade_irq_bottom_half(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd);
static int parade_irq_top_half(struct ts_cmd_node *cmd);
static int parade_register_algo(struct ts_kit_device_data *chip_data);
static int parade_reset_chip(void);
static void parade_hw_reset(void);
static int parade_fw_update_boot(char *file_name);
static int parade_get_rawdata(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd);
static int parade_oem_info_switch(struct ts_oem_info_param *info);
static int parade_chip_get_info(struct ts_chip_info_param *info);
static int parade_get_capacitance_test_type(struct ts_test_type_info *info);
static unsigned char * parade_roi_rawdata (void);
static int parade_roi_switch (struct ts_roi_info *info);
static int parade_calibrate(void);
static int parade_before_suspend(void);
static int parade_after_resume(void *feature_info);
static void parade_shutdown(void);
static int parade_glove_switch(struct ts_glove_info *info);
static int parade_holster_switch(struct ts_holster_info *info);
static int parade_get_calibration_data(struct ts_calibration_data_info *info, struct ts_cmd_node *out_cmd);
static int parade_hid_output_suspend_scanning_(struct parade_core_data *cd);
static int parade_hid_output_resume_scanning_(struct parade_core_data *cd);
static void parade_power_off_gpio_set(void);
static void parade_power_on_gpio_set(void);
static int _parade_fw_update_from_sdcard(u8 *file_path,
					 bool update_flag);
static int parade_file_open_firmware(u8 *file_path, u8 *databuf,
				      int *file_size);
static int parade_fw_update_sd(void);
static int parade_hid_output_initialize_baselines_(u8 test_id,u8 *status);
static int parade_get_mode(struct parade_core_data *cd, struct parade_hid_desc *desc);
static int parade_get_brightness_info(void);
static int parade_need_fw_update(void);
static int parade_get_diff_data(void);
static int move_knuckle_data(struct parade_core_data *cd, struct parade_sysinfo *si);
static int parade_check_cmd_status(void);
static void parade_finish_cmd(void);
static void parade_status_resume_work_fn(struct work_struct *work);
static int parade_set_glove_switch(u8 glove_switch);
static int parade_set_holster_switch(u8 in_switch);
static void parade_chip_touch_switch(void);
/*tp color function*/
static int parade_hid_output_read_color_info_(struct parade_core_data *cd,
		u8 *color_info);
static int parade_get_color_info(struct parade_core_data *cd);
extern u8 cypress_ts_kit_color[TP_COLOR_SIZE];

/* esd function */
static int  parade_watchdog_work_check(void);
static int parade_esd_reset_function(void);
extern void ts_start_wd_timer(struct ts_kit_platform_data* cd);
extern void ts_stop_wd_timer(struct ts_kit_platform_data* cd);
extern char *trans_lcd_panel_name_to_tskit(void);

static unsigned int get_panel_name_flag_adapter(void);
static unsigned int get_panel_name_flag(void);
static int parade_create_project_id(void);
static int parade_pinctrl_select_normal(void);
static int parade_pinctrl_select_lowpower(void);
static int parade_i2c_read(u8 *reg_addr, u16 reg_len, u8 *buf, u16 len);
static int parade_i2c_write(u8* buf, u16 length);
static int parade_chip_wrong_touch(void);

static int parade_wakeup_gesture_switch(struct ts_wakeup_gesture_enable_info* info);
static void parade_deep_sleep_work_fn(struct work_struct *work);
DECLARE_WORK(parade_deep_sleep_work,parade_deep_sleep_work_fn);
static int sleep_resume_flag;

static int already_get_flag = false;

#define POWER_DEEP_SLEEP	1
#define POWER_WAKEUP	0
#define PARADE_NO_NEED_DO_AFTER_RESUME	1

static atomic_t parade_reset_pin_status;
extern bool isbulcked;

enum FW_UPDATA_FLAG
{
	NO_UPDATA = 0,
    NEED_UPDATA = 1,
};
static struct parade_core_data *tskit_parade_data;
static struct delayed_work parade_status_resume_work;
static volatile bool parade_is_power_on = true;
static struct delayed_work parade_easy_wakeup_work;

static DECLARE_DELAYED_WORK(parade_status_resume_work, parade_status_resume_work_fn);
static void parade_enter_easy_wakeup(struct work_struct *work);
static DECLARE_DELAYED_WORK(parade_easy_wakeup_work, parade_enter_easy_wakeup);
#ifdef PARADE_ROI_ENABLE
static unsigned char roi_data[PARADE_ROI_DATA_READ_LENGTH] = { 0 };
#endif
static char* parade_global_pchar_for_doze;
static u32 parade_csv_line_count = 1;

#define PARADE_MMI_RESULT_BUF_LEN 80
#define PARADE_MMI_TEST_FAILED_REASON_LEN 50
#define PARADE_DELAY_20MS	20

static char tp_test_failed_reason[PARADE_MMI_TEST_FAILED_REASON_LEN] = { "-unknown" };
static char buf_parademmitest_result[PARADE_MMI_RESULT_BUF_LEN] = { 0 };	/*store mmi test result*/
struct ts_device_ops ts_kit_parade_ops = {
	.chip_parse_config =  parade_parse_dts,
	.chip_detect = parade_chip_detect,
	.chip_init =  parade_init_chip,
	.chip_register_algo = parade_register_algo,
	.chip_input_config = parade_input_config,
	.chip_irq_top_half =  parade_irq_top_half,
	.chip_irq_bottom_half =  parade_irq_bottom_half,
	.chip_suspend = parade_core_suspend,
	.chip_resume = parade_core_resume,
	.chip_hw_reset= parade_hw_reset,
	.chip_fw_update_boot = parade_fw_update_boot,
	.oem_info_switch = parade_oem_info_switch,
	.chip_get_info = parade_chip_get_info,
	.chip_get_brightness_info = parade_get_brightness_info,
	.chip_get_rawdata = parade_get_rawdata,
	.chip_get_capacitance_test_type = parade_get_capacitance_test_type,
	/*below are added by tony*/
#ifdef PARADE_ROI_ENABLE
	.chip_roi_rawdata = parade_roi_rawdata,
	.chip_roi_switch = parade_roi_switch,
#endif
	.chip_calibrate = parade_calibrate,
	.chip_before_suspend = parade_before_suspend,
	.chip_after_resume = parade_after_resume,
	.chip_shutdown = parade_shutdown,
	.chip_get_calibration_data = parade_get_calibration_data,
	.chip_fw_update_sd = parade_fw_update_sd,
	.chip_glove_switch = parade_glove_switch,
	.chip_holster_switch = parade_holster_switch,
#ifdef PARADE_DOZE_ENABLE
	.chip_touch_switch = parade_chip_touch_switch,
#endif
	.chip_check_status = parade_watchdog_work_check,
	.chip_wakeup_gesture_enable_switch = parade_wakeup_gesture_switch,
	.chip_wrong_touch = parade_chip_wrong_touch,
 };

static const u16 crc_table[16] = {
	 0x0000, 0x1021, 0x2042, 0x3063,
	 0x4084, 0x50a5, 0x60c6, 0x70e7,
	 0x8108, 0x9129, 0xa14a, 0xb16b,
	 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
 };

static const u8 cyttsp5_security_key[] = {
	0xA5, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0x5A
};
#define HID_OUTPUT_APP_COMMAND(command) \
	.cmd_type = HID_OUTPUT_CMD_APP, \
	.command_code = command

#define HID_OUTPUT_BL_COMMAND(command) \
	.cmd_type = HID_OUTPUT_CMD_BL, \
	.command_code = command

struct parade_hid_field {
	 int report_count;
	 int report_size;
	 int size; /* report_count * report_size */
	 int offset;
	 int data_type;
	 int logical_min;
	 int logical_max;
	 /* Usage Page (Hi 16 bit) + Usage (Lo 16 bit) */
	 u32 usage_page;
	 u32 collection_usage_pages[CY_HID_MAX_COLLECTIONS];
	 struct parade_hid_report *report;
	 bool record_field;
 };

struct parade_hid_report {
	 u8 id;
	 u8 type;
	 int size;
	 struct parade_hid_field *fields[CY_HID_MAX_FIELDS];
	 int num_fields;
	 int record_field_index;
	 int header_size;
	 int record_size;
	 u32 usage_page;
 };

struct atten_node {
	 struct list_head node;
	 char *id;
	 struct device *dev;

	 int (*func)(struct device *);
	 int mode;
 };

struct param_node {
	 struct list_head node;
	 u8 id;
	 u32 value;
	 u8 size;
 };

struct module_node {
	 struct list_head node;
	 struct parade_module *module;
	 void *data;
 };

struct parade_hid_cmd {
	 u8 opcode;
	 u8 report_type;
	 union {
		 u8 report_id;
		 u8 power_state;
	 };
	 u8 has_data_register;
	 size_t write_length;
	 u8 *write_buf;
	 u8 *read_buf;
	 u8 wait_interrupt;
	 u8 reset_cmd;
	 u16 timeout_ms;
 };

#ifdef TTHE_TUNER_SUPPORT
static int tthe_print(struct parade_core_data *cd, u8 *buf, int buf_len,
		const u8 *data_name)
{
	int len = strlen(data_name);
	int i, n;
	u8 *p;
	int remain;

	mutex_lock(&cd->tthe_lock);
	if (!cd->tthe_buf)
		goto exit;

	if (cd->tthe_buf_len + (len + buf_len) > PARADE_MAX_PRBUF_SIZE)
		goto exit;

	if (len + buf_len == 0)
		goto exit;

	remain = PARADE_MAX_PRBUF_SIZE - cd->tthe_buf_len;
	if (remain < len)
		len = remain;

	p = cd->tthe_buf + cd->tthe_buf_len;
	memcpy(p, data_name, len);
	cd->tthe_buf_len += len;
	p += len;
	remain -= len;

	*p = 0;
	for (i = 0; i < buf_len; i++) {
		n = scnprintf(p, remain, "%02X ", buf[i]);
		if (!n)
			break;
		p += n;
		remain -= n;
		cd->tthe_buf_len += n;
	}

	n = scnprintf(p, remain, "\n");
	if (!n)
		cd->tthe_buf[cd->tthe_buf_len] = 0;
	cd->tthe_buf_len += n;
	wake_up(&cd->wait_q);
exit:
	mutex_unlock(&cd->tthe_lock);
	return 0;
}
#endif

static unsigned int get_panel_name_flag(void)
{
	return panel_name_flag;
}

/**
* get_panel_name_flag_adapter *
* Format : //new agassi lcd:panel_flag = true; *
* Format : //old agassi lcd:panel_flag = false; *
*/
static unsigned int get_panel_name_flag_adapter(void)
{
	unsigned int panel_flag = 0;
	panel_flag = get_panel_name_flag();
	return panel_flag;
}

static void cyttsp5_pr_buf(u8 *dptr, int size,
		const char *data_name)
{
	//struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	u8 *pr_buf = tskit_parade_data->pr_buf;
	int i, k;
	const char fmt[] = "%02X ";
	int max;

	TS_LOG_DEBUG("%s enter\n", __func__);

	if (!size)
		return;

	max = (CY_MAX_PRBUF_SIZE - 1) - sizeof(CY_PR_TRUNCATED);

	pr_buf[0] = 0;
	for (i = k = 0; i < size && k < max; i++, k += 3)
		scnprintf(pr_buf + k, CY_MAX_PRBUF_SIZE, fmt, dptr[i]);

	if (size)
		TS_LOG_DEBUG("%s:  %s[0..%d]=%s%s\n", __func__, data_name,
			size - 1, pr_buf, size <= max ? "" : CY_PR_TRUNCATED);
	else
		TS_LOG_DEBUG("%s:  %s[]\n", __func__, data_name);
}

static int parade_get_capacitance_test_type(struct ts_test_type_info *info)
{
	TS_LOG_INFO("%s enter\n", __func__);
	if (!info){
		TS_LOG_ERR("%s\n", __func__);
		return -1;
	}

	if(0 !=strlen(tskit_parade_data->parade_chip_data->tp_test_type)){
		strncpy(info->tp_test_type,tskit_parade_data->parade_chip_data->tp_test_type, TS_CAP_TEST_TYPE_LEN - 1);
	}else{
		strncpy(info->tp_test_type, "Normalize_type:judge_different_reslut", TS_CAP_TEST_TYPE_LEN - 1);
	}
	TS_LOG_INFO("%s:info->tp_test_type :%s.\n", __func__,info->tp_test_type);
	return NO_ERR;
}

static int parade_check_oem_parameter(struct parade_oem_data *parade_oem_data, int offset,int length)
{
	if(parade_oem_data->oem_info_ready == false) {
		TS_LOG_ERR("%s oem info is not ready\n", __func__);
		return -1;
	}
	if(offset < 0 || length < 0) {
		TS_LOG_ERR("%s parameter should be positive\n", __func__);
		return -1;
	}
	if(offset % PARADE_OEM_FLASH_ONE_ROW_SIZE != 0) {
		TS_LOG_ERR("%s offset should align with 16 bytes\n", __func__);
		return -1;
	}
	if(length % PARADE_OEM_FLASH_ONE_ROW_SIZE != 0) {
		TS_LOG_ERR("%s length should align with 16 bytes\n", __func__);
		return -1;
	}
	if(length != parade_oem_data->flash_size) {
		TS_LOG_ERR("%s read lenght larget than flash size\n", __func__);
		return -1;
	}
	if(offset + length > parade_oem_data->flash_size) {
		TS_LOG_ERR("%s read area exceed permitted flash\n", __func__);
		return -1;
	}
	return 0;
}

static int parade_oem_suspend(struct parade_oem_data *parade_oem_data)
{
         return parade_hid_output_suspend_scanning_(tskit_parade_data);
}

static int parade_oem_resume(struct parade_oem_data *parade_oem_data)
{
         return parade_hid_output_resume_scanning_(tskit_parade_data);
}

static int parade_oem_hid_send_output_and_wait(struct parade_oem_data *parade_oem_data, struct parade_hid_output *hid_output)
{
        return  parade_hid_send_output_and_wait_(tskit_parade_data, hid_output);
}

/*
*
* this function will write length of data to specific offset
* need first call suspend before use this funciton
*/
static int parade_oem_write_nv_any_data_(struct parade_oem_data *parade_oem_data, u8 buf[], int offset, int length)
{
	u8 *response = NULL;
	u8 write_buf[512];
	int cmd_offset = 0;
	u16 crc;
	int rc;
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_WRITE_NV_DATA),
		.write_length = length + 5 + 8 + 2,
		.write_buf = write_buf,
	};

	write_buf[cmd_offset++] = LOW_BYTE(offset);
	write_buf[cmd_offset++] = HI_BYTE(offset);
	write_buf[cmd_offset++] = LOW_BYTE(length) ;
	write_buf[cmd_offset++] = HI_BYTE(length);
	write_buf[cmd_offset++] = 0x01;

	memcpy(&write_buf[cmd_offset], buf, length);

	cmd_offset += length;
	memcpy(&write_buf[cmd_offset], cyttsp5_security_key, 8);
	 cmd_offset += 8;

	crc = _parade_compute_crc(buf, length);
	write_buf[cmd_offset++] = LOW_BYTE(crc);
	write_buf[cmd_offset++] = HI_BYTE(crc);

	rc = parade_oem_data->ops.suspend(parade_oem_data);
	if(rc != 0){
	 	TS_LOG_ERR("%s suspend scan error\n", __func__);
               goto exit;
	}
	rc = parade_oem_data->ops.hid_send_output_and_wait(parade_oem_data, &hid_output);
	if(rc != 0){
		 	TS_LOG_ERR("%s send write nv data error\n", __func__);
                   goto exit;
	}
	response = (u8 *)(&(tskit_parade_data->input_buf[0]));
	rc = response[5];
	if(rc)
	          TS_LOG_ERR("%s write 16 bytes at offset=%d failed\n", __func__, offset);
	else
	          TS_LOG_INFO("%s write 16 bytes at offset=%d successful\n",  __func__, offset);
exit:
	rc = parade_oem_data->ops.resume(parade_oem_data);
	if(rc != 0){
		 	TS_LOG_ERR("%s resume scan error\n", __func__);
	}
	return rc;
}

/*
* each block is 128 bytes, the correct value for block_num is 0~3
* this function will write 128 bytes to the specific block
* buf is the head of the 128 bytes array
* need first call suspend before use this funciton
*/
static int parade_oem_write_nv_one_block_(struct parade_oem_data *parade_oem_data, u8 buf[], int block_num)
{
	  int rc;
	  rc = parade_oem_write_nv_any_data_(parade_oem_data, buf, PARADE_OEM_FLASH_ROW_SIZE_MAX * block_num , PARADE_OEM_FLASH_ROW_SIZE_MAX);
      return rc;
}

/*
* each row is 16 bytes
* this function will write 16 bytes to the specific offset
* buf is the head of the 16 bytes array
* need first call suspend before use this funciton
*/
static int parade_oem_write_nv_one_row_(struct parade_oem_data *parade_oem_data, u8 buf[], int row_num)
{
	  int rc;
	  rc = parade_oem_write_nv_any_data_(parade_oem_data, buf, PARADE_OEM_FLASH_ONE_ROW_SIZE * row_num , PARADE_OEM_FLASH_ONE_ROW_SIZE);
         return rc;
}

static int parade_oem_write_nv_data(struct parade_oem_data *parade_oem_data, u8 buf[], int offset,int length)
{
	u8 flash_size= 0;
	u8 row_setting = 0;
	u8 write_buf[1024];
	int cmd_offset = 0;
	u16 crc = 0;
	int row_index = 0;
	int column_index = 0;
	int i = 0;
	u8 *response = NULL;
	int start_row = 0;
	int end_row = 0;
	u8 uLastRowChanged = 0;
	u8 uThisRowChanged = 0;
	int offset_start = 0;
	int offset_end = 0;
	int rc = 0;

	parade_oem_data->stored_flash_data = NULL;
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_WRITE_NV_DATA),
		.write_length = length + 5 + 8 + 2,
		.write_buf = write_buf,
	};
       TS_LOG_INFO("%s enter\n", __func__);
       if(parade_check_oem_parameter(parade_oem_data, offset, length)) {
                 TS_LOG_ERR("%s parameter invalid\n", __func__);
                 rc = -1;
				 goto exit;
       }

       parade_oem_data->stored_flash_data = (u8 *)kzalloc(parade_oem_data->flash_size, GFP_KERNEL);
       if(parade_oem_data->stored_flash_data == NULL) {
                 TS_LOG_ERR("%s memory allocate failed\n", __func__);
                 rc = -1;
				 goto exit;
       }
       rc = parade_oem_read_nv_data(parade_oem_data, parade_oem_data->stored_flash_data, 0, parade_oem_data->flash_size);
	   if(rc != 0) {
         TS_LOG_ERR("%s read nv data failed\n", __func__);
         rc = -1;
		 goto exit;
       }
	   start_row = offset / PARADE_OEM_FLASH_ONE_ROW_SIZE;
       end_row = (offset + length/2) / PARADE_OEM_FLASH_ONE_ROW_SIZE;



		/*check whether block 2 has changed data*/
		for(i = PARADE_OEM_FLASH_ROW_SIZE_MAX *2; i < PARADE_OEM_FLASH_ROW_SIZE_MAX *3; i++)
			if(parade_oem_data->stored_flash_data[i] != buf[i]) {
				rc = parade_oem_write_nv_one_block_(parade_oem_data, buf, 2);
				if(rc != 0) {
			         TS_LOG_ERR("%s write nv data failed, with block 2\n", __func__);
			         rc = -1;
					 goto exit;
			    }
				break;
			}
	/*check whether block 3 has changed data*/
		for(i = PARADE_OEM_FLASH_ROW_SIZE_MAX *3; i < PARADE_OEM_FLASH_ROW_SIZE_MAX *4; i++)
			if(parade_oem_data->stored_flash_data[i] != buf[i]) {
				rc = parade_oem_write_nv_one_block_(parade_oem_data, buf, 3);
				if(rc != 0) {
			         TS_LOG_ERR("%s write nv data failed, with block 3\n", __func__);
			         rc = -1;
					 goto exit;
			    }
				break;
			}



	uLastRowChanged = 0;
	/*check whether data changed in block 0 and 1*/
	for(row_index = start_row ; row_index < end_row ; row_index++) {
		TS_LOG_INFO("%s check row=%d\n", __func__, row_index);
		uThisRowChanged = 0;
		if(buf[(row_index - start_row) * PARADE_OEM_FLASH_ONE_ROW_SIZE] !=
                      parade_oem_data->stored_flash_data[row_index * PARADE_OEM_FLASH_ONE_ROW_SIZE]) {
			uThisRowChanged = 1;
			}
		TS_LOG_INFO("%s row=%d, data changed\n", __func__, row_index);
		if((uLastRowChanged == 0) && (uThisRowChanged == 1))
		{
			offset_start = (row_index -start_row)* PARADE_OEM_FLASH_ONE_ROW_SIZE;
		}
		else if ((uLastRowChanged == 1) && (uThisRowChanged == 0))
		{
			offset_end = (row_index -start_row)*PARADE_OEM_FLASH_ONE_ROW_SIZE;
			TS_LOG_INFO("%s overwrite offset from=%d to = %d \n", __func__, offset_start, offset_end);
			rc = parade_oem_write_nv_any_data_(parade_oem_data, &buf[offset_start], offset_start, offset_end - offset_start);
			if(rc != 0) {
				TS_LOG_ERR("%s write nv data failed, with block row index:%d\n", __func__, row_index);
				rc = -1;
				goto exit;
			}
		}
		if((uThisRowChanged == 1) && (row_index == end_row - 1))
		{
			offset_end = (end_row -start_row)*PARADE_OEM_FLASH_ONE_ROW_SIZE;
			TS_LOG_INFO("%s last row overwrite offset from=%d to = %d \n", __func__, offset_start, offset_end);
			rc = parade_oem_write_nv_any_data_(parade_oem_data, &buf[offset_start], offset_start, offset_end - offset_start);
			if(rc != 0) {
				TS_LOG_ERR("%s write nv data failed, with block row index:%d\n", __func__, row_index);
				rc = -1;
				goto exit;
			}
		}
		uLastRowChanged = uThisRowChanged;
	}

	  /*read flash again to make sure that the stored flash data and written data are consistent*/
         rc = parade_oem_read_nv_data(parade_oem_data, parade_oem_data->stored_flash_data, 0, parade_oem_data->flash_size);
	  	if(rc != 0) {
	         TS_LOG_ERR("%s read nv data failed, at line: %d\n", __func__, __LINE__);
	         rc = -1;
			 goto exit;
		}
        for(row_index = start_row ; row_index < end_row ; row_index++)
			for(column_index = 0; column_index < PARADE_OEM_FLASH_ONE_ROW_SIZE; column_index++)
               if(buf[(row_index - start_row) * PARADE_OEM_FLASH_ONE_ROW_SIZE + column_index] !=
                                 parade_oem_data->stored_flash_data[row_index * PARADE_OEM_FLASH_ONE_ROW_SIZE + column_index]) {
                        TS_LOG_ERR("%s error:read data and write data not consistent\n", __func__);
                        rc = -1;
			 			goto exit;
           }
	  TS_LOG_INFO("%s successful\n", __func__);
exit:
	  return rc;
}
static int parade_oem_read_nv_data(struct parade_oem_data *parade_oem_data, u8 buf[], int offset, int length)
{
         u8 status = 0;
         u8 flash_size;
         u8 *response = NULL;
         u8 row_setting;
         u8 cmd_buf[512];
         u8 actual_read_len = 0;
		 int rc = 0;
         struct parade_hid_output hid_output = {
			HID_OUTPUT_APP_COMMAND(HID_OUTPUT_READ_NV_DATA),
			.write_length = 5,
			.write_buf = cmd_buf,
		};
		TS_LOG_INFO("%s enter\n", __func__);

         if(parade_check_oem_parameter(parade_oem_data, offset, length)) {
              TS_LOG_ERR("%s parameter invalid\n", __func__);
                   goto exit;
         }

         rc = parade_oem_data->ops.suspend(parade_oem_data);
		 if(rc != 0){
		 	TS_LOG_ERR("%s suspend scan error\n", __func__);
                   goto exit;
		 }
         response = (u8 *)(&(tskit_parade_data->input_buf[0]));

         while(1) {
                   if(length == 0) {
                            TS_LOG_INFO("%s read finished!\n", __func__);
                            goto exit;
                   }
                   cmd_buf[0] = offset & 0xff;
                   cmd_buf[1] = (offset & 0xff00)>>8;
                   if(length > PARADE_OEM_FLASH_ROW_SIZE_MAX) {
                            cmd_buf[2] = LOW_BYTE(PARADE_OEM_FLASH_ROW_SIZE_MAX);
                            cmd_buf[3] = HI_BYTE(PARADE_OEM_FLASH_ROW_SIZE_MAX);
                   }else {
                            cmd_buf[2] = LOW_BYTE(length);
                            cmd_buf[3] = HI_BYTE(length);
                   }
                   /*Enumerated Block ID is always 1 here*/
                   cmd_buf[4] = 1;
                   rc = parade_oem_data->ops.hid_send_output_and_wait(parade_oem_data, &hid_output);
				   if(rc != 0){
					 	TS_LOG_ERR("%s hid output error,rc=%d,at line: %d\n", __func__, rc, __LINE__);
				               goto exit;
				   }
				   status = response[5];
                   if(status) {
                            TS_LOG_ERR("%s read failed, offset=%d", __func__, offset);
                            goto exit;
                   }
                   actual_read_len = response[7] + 256 * response[8];
                   memcpy(buf, &response[10], actual_read_len);
                   if(actual_read_len % PARADE_OEM_FLASH_ONE_ROW_SIZE != 0) {
                            TS_LOG_ERR("%s actual read length not aligned with 16 bytes", __func__);
                            status = -1;
                            goto exit;
                   }
                   offset += actual_read_len;
                   buf += actual_read_len;
                   length -= actual_read_len;
                   TS_LOG_INFO("%s actual_read_len=%d, remain_length=%d\n", __func__, actual_read_len, length);
         }
exit:
         rc = parade_oem_data->ops.resume(parade_oem_data);
		 if(rc != 0){
		 	TS_LOG_ERR("%s resume scan error at line %d\n", __func__,__LINE__);
		 }
         return rc;
}

static int parade_get_nv_info(struct parade_oem_data *parade_oem_data){

		 struct parade_hid_output hid_output = {
        		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_GET_NV_INFO),
		 };

         u8 status = 0;
         u8 flash_size;
         u8 *response = NULL;
         u8 row_setting;
		 int rc = 0;
         struct parade_nv_info *info = &parade_oem_data->info;
         TS_LOG_INFO("%s enter\n", __func__);

         rc = parade_oem_data->ops.suspend(parade_oem_data);
		 if(rc != 0){
		 	TS_LOG_ERR("%s OEM Suspend Scan Error\n", __func__);
			goto exit;
		 }
         rc = parade_oem_data->ops.hid_send_output_and_wait(parade_oem_data, &hid_output);
		 if(rc != 0){
		 	TS_LOG_ERR("%s OEM HID output error\n", __func__);
			goto exit;
		 }
         response = (u8 *)(&(tskit_parade_data->input_buf[0]));

         info->flash_size = response[6] + 256 * response[7];
         parade_oem_data->flash_size = info->flash_size;
         TS_LOG_INFO("%s flash_size=%d\n", __func__, info->flash_size);
         row_setting = response[8];
         info->row0_set = (row_setting & (3<<0)) >> 0;
         info->row1_set = (row_setting & (3<<2)) >> 2;
         info->row2_set = (row_setting & (3<<4)) >> 4;
         info->row3_set = (row_setting & (3<<6)) >> 6;
         TS_LOG_INFO("%s row0_set=%d\n", __func__, info->row0_set);
         TS_LOG_INFO("%s row1_set=%d\n", __func__, info->row1_set);
         TS_LOG_INFO("%s row2_set=%d\n", __func__, info->row2_set);
         TS_LOG_INFO("%s row3_set=%d\n", __func__, info->row3_set);

         status = response[5];
         if(status == 0)
                   parade_oem_data->oem_info_ready = true;
         else {
                   parade_oem_data->oem_info_ready = false;
                   TS_LOG_ERR("%s oem infomation get failed\n", __func__);
         }
exit:
         rc = parade_oem_data->ops.resume(parade_oem_data);
		 if(rc != 0){
		 	TS_LOG_ERR("%s Resume Scan Error\n", __func__);
		 }
         return rc;
}

static int parade_get_oem_flash_size(struct parade_oem_data *parade_oem_data)
{
         if(parade_oem_data->oem_info_ready == false) {
                   TS_LOG_ERR("%s oem info is not ready\n", __func__);
                   return -1;
         }
         return parade_oem_data->flash_size;
}

static int parade_oem_init(struct parade_oem_data *parade_oem_data)
{
         if(parade_oem_data == NULL) {
                   TS_LOG_ERR("%s error: parade_oem_data is null\n", __func__);
                   return -1;
         }
         parade_oem_data->ops.suspend = parade_oem_suspend;
         parade_oem_data->ops.resume = parade_oem_resume;
         parade_oem_data->ops.write_nv_data = parade_oem_write_nv_data;
         parade_oem_data->ops.read_nv_data = parade_oem_suspend;
         parade_oem_data->ops.get_nv_info = parade_get_nv_info;
         parade_oem_data->ops.hid_send_output_and_wait = parade_oem_hid_send_output_and_wait;

         parade_oem_data->oem_info_ready = false;
         return 0;
}

static unsigned short parade_get_oem_size(void)
{
	int rc;
	struct parade_oem_data *parade_oem_data = &(tskit_parade_data->parade_oem_data);
	rc = parade_oem_init(parade_oem_data);
	if(rc != 0) {
		TS_LOG_ERR("%s: parade_oem_init fail line=%d\n", __func__, __LINE__);
		return -1;
	}
	rc = parade_get_nv_info(parade_oem_data);
	if( rc != 0 ) {
		TS_LOG_ERR("%s: parade_get_nv_info fail line=%d\n", __func__, __LINE__);
		return -1;
	}
	return  parade_get_oem_flash_size(parade_oem_data);
}

static int parade_get_oem_data(unsigned char *oem_data, unsigned short leng)
{
	int retval = NO_ERR;
	struct parade_oem_data *parade_oem_data = &(tskit_parade_data->parade_oem_data);
	retval = parade_oem_read_nv_data(parade_oem_data, oem_data, 0, leng);
	return retval;
}

static int parade_set_oem_data(unsigned char *oem_data, unsigned short leng)
{
	 int retval = NO_ERR;
	 struct parade_oem_data *parade_oem_data = &(tskit_parade_data->parade_oem_data);
	 retval = parade_oem_write_nv_data(parade_oem_data, oem_data, 0, leng);
	 return retval;
}

static u8 tp_result_info[TS_CHIP_TYPE_MAX_SIZE] = {0};
static u8 tp_type_cmd[TS_CHIP_TYPE_MAX_SIZE] = {0};

static void parade_print_NVstructure(struct ts_oem_info_param *info)
{
	int flash_size = parade_get_oem_size();
	int i = 0;

	if(flash_size <= 0 || flash_size >= (TS_CHIP_BUFF_MAX_SIZE - PARADE_OEMINFO_LEN_PER_LINE + 1)) {
		TS_LOG_ERR("%s: Could not get TPIC flash size,fail line=%d\n", __func__,
				__LINE__);
		return;
	}

	for(i = 0; i< flash_size/PARADE_OEMINFO_LEN_PER_LINE; ++i ){
		TS_LOG_INFO("%s: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n", __func__,
			 info->buff[0+i*PARADE_OEMINFO_LEN_PER_LINE],info->buff[1+i*PARADE_OEMINFO_LEN_PER_LINE],info->buff[2+i*PARADE_OEMINFO_LEN_PER_LINE],
			 info->buff[3+i*PARADE_OEMINFO_LEN_PER_LINE], info->buff[4+i*PARADE_OEMINFO_LEN_PER_LINE],info->buff[5+i*PARADE_OEMINFO_LEN_PER_LINE],
			 info->buff[6+i*PARADE_OEMINFO_LEN_PER_LINE],info->buff[7+i*PARADE_OEMINFO_LEN_PER_LINE],info->buff[8+i*PARADE_OEMINFO_LEN_PER_LINE],
			 info->buff[9+i*PARADE_OEMINFO_LEN_PER_LINE],info->buff[10+i*PARADE_OEMINFO_LEN_PER_LINE],info->buff[11+i*PARADE_OEMINFO_LEN_PER_LINE],
			 info->buff[12+i*PARADE_OEMINFO_LEN_PER_LINE],info->buff[13+i*PARADE_OEMINFO_LEN_PER_LINE],info->buff[14+i*PARADE_OEMINFO_LEN_PER_LINE],
			 info->buff[15+i*PARADE_OEMINFO_LEN_PER_LINE]
		 );
	}
}

static int parade_reconstruct_barcode(struct ts_oem_info_param *info)
{
	 int retval = NO_ERR;
	 int offset1 = TS_NV_STRUCTURE_BAR_CODE_OFFSET1;
	 int offset2 = TS_NV_STRUCTURE_BAR_CODE_OFFSET1;
	 u8 type = 0;
	 u8 len = 0;

	 TS_LOG_INFO("%s enter\n", __func__);

	type = info->buff[offset1*16 + 0];
	len = info->buff[offset1*16 + 1];
	if ((type == 0x00 && len == 0x00) ||(type == 0xFF && len == 0xFF)) {
		 memcpy(&(info->buff[offset1*16]), tp_type_cmd, tp_type_cmd[1]*16);
		 TS_LOG_INFO("%s Will write the data to info_buff, offset is %d \n", __func__, offset1);
		 return retval;
	 }

	type = info->buff[offset2*16 + 0];
	len = info->buff[offset2*16 + 1];
	if ((type == 0x00 && len == 0x00) ||(type == 0xFF && len == 0xFF)) {
		 memcpy(&(info->buff[offset2*16]), tp_type_cmd, tp_type_cmd[1]*16);
		 TS_LOG_INFO("%s Will write the data to info_buff, offset is %d\n", __func__, offset2);
		 return retval;
	 }

	TS_LOG_INFO("%s barcode data is full, could not write into the data\n", __func__);
	tp_result_info[0] = TS_CHIP_NO_FLASH_ERROR;
	retval = -EINVAL;
	 return retval;
 }

static int parade_reconstruct_brightness(struct ts_oem_info_param *info)
{
	int retval = NO_ERR;
	int offset1 = TS_NV_STRUCTURE_BRIGHTNESS_OFFSET1;
	int offset2 = TS_NV_STRUCTURE_BRIGHTNESS_OFFSET2;
	u8 type = 0;
	u8 len = 0;

	TS_LOG_INFO("%s enter\n", __func__);

	type = info->buff[offset1*16 + 0];
	len = info->buff[offset1*16 + 1];
	if ((type == 0x00 && len == 0x00) ||(type == 0xFF && len == 0xFF)) {
		 memcpy(&(info->buff[offset1*16]), tp_type_cmd, tp_type_cmd[1]*16);
		 TS_LOG_INFO("%s Will write the data to info_buff, offset is %d\n", __func__, offset1);
		 return retval;
	}

	type = info->buff[offset2*16 + 0];
	len = info->buff[offset2*16 + 1];
	if ((type == 0x00 && len == 0x00) ||(type == 0xFF && len == 0xFF)) {
		 memcpy(&(info->buff[offset2*16]), tp_type_cmd, tp_type_cmd[1]*16);
		 TS_LOG_INFO("%s Will write the data to info_buff, offset is %d\n", __func__, offset2);
		 return retval;
	 }

	 TS_LOG_INFO("%s brightness data is full, could not write into the data\n", __func__);
	 tp_result_info[0] = TS_CHIP_NO_FLASH_ERROR;
	 retval = -EINVAL;
	 return retval;
}

static int parade_reconstruct_whitepoint(struct ts_oem_info_param *info)
{
	int retval = NO_ERR;
	int offset1 = TS_NV_STRUCTURE_WHITE_POINT_OFFSET1;
	int offset2 = TS_NV_STRUCTURE_WHITE_POINT_OFFSET2;
	u8 type = 0;
	u8 len = 0;

	TS_LOG_INFO("%s enter\n", __func__);

	type = info->buff[offset1*16 + 0];
	len = info->buff[offset1*16 + 1];
	if ((type == 0x00 && len == 0x00) ||(type == 0xFF && len == 0xFF)) {
		memcpy(&(info->buff[offset1*16]), tp_type_cmd, tp_type_cmd[1]*16);
		TS_LOG_INFO("%s Will write the data to info_buff, offset is %d\n", __func__, offset1);
		return retval;
	}

	type = info->buff[offset2*16 + 0];
	len = info->buff[offset2*16 + 1];
	if ((type == 0x00 && len == 0x00) ||(type == 0xFF && len == 0xFF)) {
		memcpy(&(info->buff[offset2*16]), tp_type_cmd, tp_type_cmd[1]*16);
		TS_LOG_INFO("%s Will write the data to info_buff, offset is %d\n", __func__, offset2);
		return retval;
	}

	TS_LOG_INFO("%s white point is full, could not write into the data\n", __func__);
	tp_result_info[0] = TS_CHIP_NO_FLASH_ERROR;
	retval = -EINVAL;
	return retval;
 }

static int parade_reconstruct_brightness_whitepoint(struct ts_oem_info_param *info)
{
	 int retval = NO_ERR;
	 TS_LOG_INFO("%s No Flash defined in NV structure\n", __func__);
	 tp_result_info[0] = TS_CHIP_NO_FLASH_ERROR;
	 return retval;
}

static int parade_reconstruct_repair_recode(struct ts_oem_info_param *info)
{
	int retval = NO_ERR;
	int offset = TS_NV_STRUCTURE_REPAIR_OFFSET1;
	u8 type = 0;
	u8 len = 0;

	TS_LOG_INFO("%s enter\n", __func__);

	for(;offset <= TS_NV_STRUCTURE_REPAIR_OFFSET5; ++offset) {
		type = info->buff[offset*16 + 0];
		len = info->buff[offset*16 + 1];

		if ((type == 0x00 && len == 0x00) ||(type == 0xFF && len == 0xFF)) {
			memcpy(&(info->buff[offset*16]), tp_type_cmd, tp_type_cmd[1]*16);
			TS_LOG_INFO("%s Will write the data to info_buff, offset is %d\n", __func__, offset);
			break;
		} else if( offset == TS_NV_STRUCTURE_REPAIR_OFFSET5 ) {
			TS_LOG_INFO("%s repaire recode is full, could not write into the data\n", __func__);
			tp_result_info[0] = TS_CHIP_NO_FLASH_ERROR;
			retval = -EINVAL;
		}
	}
	return retval;
}

static int parade_reconstruct_NVstructure(struct ts_oem_info_param *info)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s called\n", __func__);
	TS_LOG_INFO("%s  info->data[0]:%2x\n", __func__, info->data[0]);

	TS_LOG_INFO("%s  itp_type_cmd[0]:%2x\n", __func__, tp_type_cmd[0]);
	switch (tp_type_cmd[0]) {
	case TS_NV_STRUCTURE_BAR_CODE:
		retval = parade_reconstruct_barcode(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, parade_reconstruct_barcode faild\n",
				   __func__);
		}
		break;
	case TS_NV_STRUCTURE_BRIGHTNESS:
		retval = parade_reconstruct_brightness(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, parade_reconstruct_brightness faild\n",
				   __func__);
		}
		break;
	case TS_NV_STRUCTURE_WHITE_POINT:
		retval = parade_reconstruct_whitepoint(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, parade_reconstruct_whitepoint faild\n",
				   __func__);
		}
		break;
	case TS_NV_STRUCTURE_BRI_WHITE:
		retval = parade_reconstruct_brightness_whitepoint(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, parade_reconstruct_brightness_whitepoint faild\n",
				   __func__);
		}
		break;
	case TS_NV_STRUCTURE_REPAIR:
		retval = parade_reconstruct_repair_recode(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, parade_reconstruct_repair_recode faild\n",
				   __func__);
		}
		break;
	default:
		TS_LOG_INFO("invalid NV structure type=%d\n",
				info->data[0]);
		retval = -EINVAL;
		break;
	}

	TS_LOG_INFO("%s end", __func__);
	return retval;
 }

static int parade_get_NVstructure_index(struct ts_oem_info_param *info, u8 type)
{
	int index = 0;
	int latest_index = 0;
	int count = 0;
	int flash_size = parade_get_oem_size();

	if(flash_size <= 0) {
		TS_LOG_ERR("%s: Could not get TPIC flash size,fail line=%d\n", __func__,
				__LINE__);
		return -1;
	}

	for (index = 1; index < (TS_NV_STRUCTURE_REPAIR_OFFSET5+1); ++index) {
		u8 tmp_type = info->buff[index*16];
		u8 tmp_len = info->buff[index*16+1];
		if (tmp_type == type && ts_oemdata_type_check_legal(tmp_type, tmp_len)) {
			latest_index = index;
			count += 1;
		}
	}

	if(type == 0x05) {
		info->length = count;
		if(info->length) {
			latest_index = TS_NV_STRUCTURE_REPAIR_OFFSET1;
		}
	}else{
		info->length = info->buff[latest_index*16+1];
	}

	return latest_index;
}

static int parade_set_oem_info(struct ts_oem_info_param *info)
{
	u8 type_reserved = TS_CHIP_TYPE_RESERVED;
	u8 len_reserved = TS_CHIP_TYPE_LEN_RESERVED;
	int flash_size = 0;
	int used_size = 16;
	int error = NO_ERR;
	u8 type = 0;
	u8 len = 0;
	memset(tp_result_info, 0x0, TS_CHIP_TYPE_MAX_SIZE);

	TS_LOG_INFO("%s called\n", __func__);
	flash_size = parade_get_oem_size();
	if(flash_size <= 0) {
		TS_LOG_ERR("%s: Could not get TPIC flash size,fail line=%d\n", __func__,
				__LINE__);
		tp_result_info[0] = TS_CHIP_WRITE_ERROR;
		error = EINVAL;
		goto out;
	}
	type = info->data[0];
	len  = info->data[1];
	used_size += len * 16;

	//check type and len below
	TS_LOG_ERR("%s write Type=0x%2x , type data len=%d\n", __func__, type, len);
	if (type == 0x0 || type > type_reserved ) {
		TS_LOG_ERR("%s write Type=0x%2x is RESERVED\n", __func__, type);
		tp_result_info[0] = TS_CHIP_WRITE_ERROR;
		error = EINVAL;
		goto out;
	}

	if ( len > len_reserved ) {
		TS_LOG_ERR("%s TPIC write RESERVED NV STRUCT len\n", __func__);
		tp_result_info[0] = TS_CHIP_WRITE_ERROR;
		error = EINVAL;
		goto out;
	}
	 //just store the data in tp_type_cmd buff
	if (len == 0x0) {
		tp_type_cmd[0] = info->data[0];
		TS_LOG_INFO("%s Just store type:%2x and then finished\n", __func__, info->data[0]);
		return error;
	}

	if (strlen(info->data) <= TS_CHIP_TYPE_MAX_SIZE) {
		memset(tp_type_cmd, 0x0, TS_CHIP_TYPE_MAX_SIZE);
		memcpy(tp_type_cmd, info->data, len*16);
	} else {
		error = EINVAL;
		TS_LOG_INFO("%s: invalid test cmd\n", __func__);
		return error;
	}

	error = parade_get_oem_data(info->buff, flash_size);
	if (error < 0) {
		TS_LOG_ERR("%s: get oem data failed,fail line=%d\n", __func__,
				__LINE__);
		tp_result_info[0] = TS_CHIP_READ_ERROR;
		goto out;
	}

	TS_LOG_INFO("%s: Read data from TPIC flash is below\n", __func__);
	parade_print_NVstructure(info);

	// Adding type data into the buffer
	error = parade_reconstruct_NVstructure(info);
	if (error < 0) {
		TS_LOG_ERR("%s: reconstruct NVstructure failed\n", __func__);
		tp_result_info[0] = TS_CHIP_READ_ERROR;
		goto out;
	}

	TS_LOG_INFO("%s: Add write type data into buff below\n", __func__);
	parade_print_NVstructure(info);

	//Write buffer into TPIC flash
	error = parade_set_oem_data(info->buff, flash_size);
	if (error < 0) {
		TS_LOG_ERR("%s: set oem data failed\n", __func__);
		tp_result_info[0] = TS_CHIP_WRITE_ERROR;
		goto out;
	}

out:
	memset(tp_type_cmd, 0x0, TS_CHIP_TYPE_MAX_SIZE);
	TS_LOG_INFO("%s End\n", __func__);
	return error;

}

static int parade_get_oem_info(struct ts_oem_info_param *info)
{
	u8 type_reserved = TS_CHIP_TYPE_RESERVED;
	u8 type = tp_type_cmd[0];
	unsigned short flash_size = 0;
	int error = NO_ERR;
	int latest_index = 0;

	TS_LOG_INFO("%s called\n", __func__);
	memset(info->data, 0x0, TS_CHIP_TYPE_MAX_SIZE);

	flash_size = parade_get_oem_size();
	if(flash_size <= 0) {
		TS_LOG_ERR("%s: Could not get TPIC flash size,rc=%d\n", __func__,
				flash_size);
		error = EINVAL;
		goto out;
	}
	TS_LOG_INFO("%s:read oem info flash_size =%d.\n", __func__, info->data[0]);

	if(type == 0x0) {
		memcpy(info->data, tp_result_info, TS_CHIP_TYPE_MAX_SIZE);
		TS_LOG_INFO("%s:Reurn the write result=%2x to sys node.\n", __func__, info->data[0]);
		goto out;
	}

	//check type
	TS_LOG_INFO("%s: store type=%2x\n", __func__, type);
	if (type > type_reserved) {
		TS_LOG_ERR("%s Read Type=0x%2x is RESERVED\n", __func__, type);
		error = EINVAL;
		goto out;
	}

	error = parade_get_oem_data(info->buff, flash_size);
	if (error < 0) {
		TS_LOG_ERR("%s: memory not enough,fail line=%d\n", __func__,
				__LINE__);
		error = EINVAL;
		goto out;
	}

	TS_LOG_INFO("%s:Get buff data below\n", __func__);
	parade_print_NVstructure(info);

	//scan each NV struct length
	latest_index = parade_get_NVstructure_index(info, type);
	TS_LOG_INFO("%s get type:0x%2x  index = %d\n", __func__, type, latest_index);

	if (latest_index > 0) {
		TS_LOG_INFO("%s type data find. len = %d\n", __func__, info->length);
		if( info->length*PARADE_OEMINFO_LEN_PER_LINE >= TS_CHIP_TYPE_MAX_SIZE
				|| (info->length + latest_index )* PARADE_OEMINFO_LEN_PER_LINE > TS_CHIP_BUFF_MAX_SIZE){
			TS_LOG_ERR("%s invalid index[ %d ].\n", __func__, latest_index);
			error = EINVAL;
			goto out;
		}
		memcpy(info->data, &(info->buff[latest_index*16]), info->length*16 );
	} else {
		info->data[0] = 0x1;
		TS_LOG_INFO("%s No type data find. info->data[0] = %2x\n", __func__, info->data[0]);
	}

out:
	TS_LOG_INFO("%s End\n", __func__);
	return error;
}



static int parade_combine2byte(u8 *pbuf)
{
	int iReturn = pbuf[1];
	iReturn = iReturn*256 + pbuf[0];
	if(iReturn > 32767)
		iReturn -= 65536;
	return iReturn;
}

static int parade_get_scan_data(enum scan_data_type_list scan_data_type)
{
	struct parade_device_access_data *dad = &tskit_parade_data->dad;
	struct parade_sysinfo *si = &tskit_parade_data->sysinfo;
	struct device *dev = dad->dev;
	struct parade_core_data *cd = tskit_parade_data;
	struct cmcp_data *cmcp_info = cd->dad.cmcp_info;
	u8 config;
	u16 actual_read_len;
	u16 length = 0;
	u8 element_size = 0;
	u8 *buf_offset;
	int elem;
	int elem_offset = 0;
	int rc;
	int i;
	const int max_read_len = 100;

	TS_LOG_INFO("%s enter\n", __func__);
	//No suspend, the caller should maintain the suspend scan

	mutex_lock(&dad->debugfs_lock);

	if(si == NULL) {
		TS_LOG_ERR("%s si is null\n", __func__);
		rc = -1;
		goto release_mutex;
	}
	buf_offset = dad->ic_buf;

	/* Start scan */
	rc = parade_exec_panel_scan_cmd_(dev);
	if (rc < 0) {
		TS_LOG_ERR("%s parade_exec_panel_scan_cmd_ failed\n", __func__);
		goto release_mutex;
	}

	/*start get mutual raw data*/
	dad->heatmap.data_type = scan_data_type;
	elem = si->sensing_conf_data.electrodes_x * si->sensing_conf_data.electrodes_y;

	if (elem > max_read_len) {
		rc = parade_ret_scan_data_cmd_(dev, elem_offset,
		max_read_len, dad->heatmap.data_type, NULL,
		&config, &actual_read_len, buf_offset);
	} else {
		rc = parade_ret_scan_data_cmd_(dev, elem_offset, elem,
			dad->heatmap.data_type, NULL, &config,
			&actual_read_len, buf_offset);
	}

	if (rc < 0) {
		TS_LOG_ERR("%s parade_ret_scan_data_cmd_ failed\n", __func__);
		goto release_mutex;
	}

	element_size = config & CY_CMD_RET_PANEL_ELMNT_SZ_MASK;
	length += actual_read_len * element_size;
	buf_offset = dad->ic_buf + length;

	elem -= actual_read_len;
	elem_offset = actual_read_len;
	while (elem > 0) {
		if (elem > max_read_len) {
			rc = parade_ret_scan_data_cmd_(dev, elem_offset,
			max_read_len, dad->heatmap.data_type, NULL, &config,
			&actual_read_len, buf_offset);
		} else{
			rc = parade_ret_scan_data_cmd_(dev, elem_offset, elem,
				dad->heatmap.data_type, NULL, &config,
				&actual_read_len, buf_offset);
		}

		if (rc < 0) {
			TS_LOG_ERR("%s parade_ret_scan_data_cmd_ failed\n", __func__);
			goto release_mutex;
		}

		if (!actual_read_len)
			break;

		length += actual_read_len * element_size;
		buf_offset = dad->ic_buf + length;
		elem -= actual_read_len;
		elem_offset += actual_read_len;
	}
release_mutex:
	mutex_unlock(&dad->debugfs_lock);
	return 0;
 }

static int validate_diff_result(int diff_threshold)
{
	struct parade_core_data *cd = tskit_parade_data;
	struct cmcp_data *cmcp_info = cd->dad.cmcp_info;
	struct result *result = cd->dad.result;
	int32_t tx_num = cmcp_info->tx_num;
	int32_t rx_num = cmcp_info->rx_num;
	uint32_t sensor_num = tx_num * rx_num;
	int32_t *diff_sensor_data = cmcp_info->diff_sensor_data;
	int i;

	TS_LOG_INFO("%s: start, diff_threshold=%d\n", __func__, diff_threshold);
	result->sensor_diff_data = diff_sensor_data;
	result->diff_test_pass = true;
	for(i = 0; i < sensor_num; i++)
	{
		if(ABS(diff_sensor_data[i]) > diff_threshold)
		{
			result->diff_test_pass = false;
			break;
		}
	}
	return 0;
}

int parade_hid_output_report_rate_switch_(struct cyttsp5_core_data *cd)
{
	if(NULL == cd) {
		TS_LOG_ERR("%s cd NULL! err!\n", __func__);
		return -CY_REPORT_RATE_ERR;
	}

	struct parade_hid_output m_hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_REPORT_RATE_SWITCH),
		.timeout_ms = CY_REPORT_RATE_DELAY,
	};
	TS_LOG_INFO("%s HID_OUTPUT_REPORT_RATE_SWITCH is 0x%02x", __func__, HID_OUTPUT_REPORT_RATE_SWITCH);

	return parade_hid_send_output_and_wait_(cd, &m_hid_output);
}

void parade_change_report_rate(void)
{
	int rc = 0;
	struct parade_core_data *cd = tskit_parade_data;
	struct result *result = NULL;
	u8 response_trcrq = 0;
	u8 response_trcts = 0;

	if(NULL == cd) {
		TS_LOG_ERR("%s cd NULL err!\n", __func__);
		return;
	}

	result = cd->dad.result;
	if(NULL == result) {
		TS_LOG_ERR("%s result NULL err!\n", __func__);
		return;
	}

	rc = parade_hid_output_suspend_scanning_(cd);
	if (rc) {
		TS_LOG_ERR("%s cyttsp5_hid_output_suspend_scanning_ err!\n", __func__);
		goto change_report_rate_err;
	}

	TS_LOG_INFO("parade_change_report_rate start\n");
	rc = parade_hid_output_report_rate_switch_(cd);
	if(!rc) {
		response_trcrq = cd->response_buf[CY_REPORT_RATE_TRCRQ];
		response_trcts = cd->response_buf[CY_REPORT_RATE_TRCTS];
	} else {
		TS_LOG_ERR("%s parade_hid_output_report_rate_switch_ back err!", __func__);
		goto change_report_rate_err;
	}

	rc = parade_hid_output_resume_scanning_(cd);
	if (rc) {
		TS_LOG_ERR("%s parade_hid_output_resume_scanning_ err!\n", __func__);
		goto change_report_rate_err;
	} else {
		if(CY_CORRECT_REPORT_RATE == response_trcrq && CY_CORRECT_REPORT_RATE == response_trcts) {
			result->change_report_rate_pass = CY_REPORT_RATE_TRUE;
			TS_LOG_INFO("%s parade_change_report_rate pass!\n", __func__);
		} else {
			TS_LOG_ERR("%s parade_change_report_rate fail!\n", __func__);
			goto change_report_rate_err;
		}
	}

	return;

change_report_rate_err:
	result->change_report_rate_pass = CY_REPORT_RATE_FALSE;
	return;
}

static void validate_diff_self_result(int rx_diff_threshold, int tx_diff_threshold)
{
	struct parade_core_data *cd = NULL;
	struct cmcp_data *cmcp_info = NULL;
	struct result *result = NULL;
	int32_t rx_num = 0;
	int32_t tx_num = 0;
	int i = 0;

	cd = tskit_parade_data;
	if(NULL == cd) {
		TS_LOG_ERR("%s:parade_core_data's NULL\n", __func__);
		return;
	}

	cmcp_info = cd->dad.cmcp_info;
	if(NULL == cmcp_info) {
		TS_LOG_ERR("%s:cmcp_info's NULL\n", __func__);
		return;
	}
	rx_num = cmcp_info->rx_num;
	tx_num = cmcp_info->tx_num;

	result = cd->dad.result;
	if(NULL == result) {
		TS_LOG_ERR("%s:result's NULL\n", __func__);
		return;
	}

	TS_LOG_INFO("%s: start, rx_diff_threshold=%d, tx_diff_threshold=%d\n", __func__, rx_diff_threshold, tx_diff_threshold);
	result->diff_self_rx_test_pass = true;
	result->diff_self_tx_test_pass = true;

	for(i = 0; i < rx_num; i++)
	{
		if(ABS(cmcp_info->diff_self_data[i]) > rx_diff_threshold)
		{
			result->diff_self_rx_test_pass= false;
			TS_LOG_ERR("%s: Rx self noise data-%d=%d breached threshold\n", __func__,
				(i + CY_SEE_IT_NEXT_ONE), cmcp_info->diff_self_data[i]);
			return;
		}
	}

	for(i = rx_num; i < (rx_num + tx_num); i++)
	{
		if(ABS(cmcp_info->diff_self_data[i]) > tx_diff_threshold)
		{
			result->diff_self_tx_test_pass= false;
			TS_LOG_ERR("%s: Tx self noise data-%d=%d breached threshold\n", __func__,
				(i + CY_SEE_IT_NEXT_ONE - rx_num), cmcp_info->diff_self_data[i]);
			return;
		}
	}

	TS_LOG_INFO("%s: self noise test all pass\n", __func__);
}

static int parade_get_diff_data(void)
{
	struct parade_device_access_data *dad = &tskit_parade_data->dad;
	struct parade_sysinfo *si = &tskit_parade_data->sysinfo;
	struct device *dev = dad->dev;
	struct parade_core_data *cd = tskit_parade_data;
	struct cmcp_data *cmcp_info = cd->dad.cmcp_info;
	u8 config;
	u16 actual_read_len;
	u16 length = 0;
	u8 element_size = 0;
	u8 *buf_offset;
	int elem;
	int elem_offset = 0;
	int rc;
	int i;
	const int max_read_len = 100;
	int iLoop = 2;
	int iAveTimes = 2;
	TS_LOG_INFO("%s enter\n", __func__);
	//No suspend, the caller should maintain the suspend scan

	mutex_lock(&dad->debugfs_lock);

	if(si == NULL) {
		TS_LOG_ERR("%s si is null\n", __func__);
		rc = -1;
		goto release_mutex;
	}

	while(iAveTimes>0){
		iAveTimes--;
		/* Start scan */
		rc = parade_exec_panel_scan_cmd_(dev);
		if (rc < 0) {
			TS_LOG_ERR("%s parade_exec_panel_scan_cmd_ failed\n", __func__);
			goto release_mutex;
		}
		iLoop = 2;
		while(iLoop>0){
			iLoop--;
			buf_offset = dad->ic_buf;
			element_size = 0;
			elem_offset = 0;
			length = 0;
			config = 0;
			actual_read_len = 0;
			if(iLoop == 1){
				/*start get mutual raw data*/
				dad->heatmap.data_type = CY_MUT_DIFF;
				elem = si->sensing_conf_data.electrodes_x * si->sensing_conf_data.electrodes_y;
			} else {
				dad->heatmap.data_type = CY_SELF_DIFF;
				elem = si->sensing_conf_data.electrodes_x + si->sensing_conf_data.electrodes_y;
			}

			if (elem > max_read_len) {
				rc = parade_ret_scan_data_cmd_(dev, elem_offset,
				max_read_len, dad->heatmap.data_type, NULL,
				&config, &actual_read_len, buf_offset);
			} else {
				rc = parade_ret_scan_data_cmd_(dev, elem_offset, elem,
					dad->heatmap.data_type, NULL, &config,
					&actual_read_len, buf_offset);
			}

			if (rc < 0) {
				TS_LOG_ERR("%s parade_ret_scan_data_cmd_ failed\n", __func__);
				goto release_mutex;
			}

			element_size = config & CY_CMD_RET_PANEL_ELMNT_SZ_MASK;
			length += actual_read_len * element_size;
			buf_offset = dad->ic_buf + length;

			elem -= actual_read_len;
			elem_offset = actual_read_len;
			while (elem > 0) {
				if (elem > max_read_len) {
					rc = parade_ret_scan_data_cmd_(dev, elem_offset,
					max_read_len, dad->heatmap.data_type, NULL, &config,
					&actual_read_len, buf_offset);
				} else{
					rc = parade_ret_scan_data_cmd_(dev, elem_offset, elem,
						dad->heatmap.data_type, NULL, &config,
						&actual_read_len, buf_offset);
				}

				if (rc < 0) {
					TS_LOG_ERR("%s parade_ret_scan_data_cmd_ failed\n", __func__);
					goto release_mutex;
				}

				if (!actual_read_len)
					break;

				length += actual_read_len * element_size;
				buf_offset = dad->ic_buf + length;
				elem -= actual_read_len;
				elem_offset += actual_read_len;
			}
			if(iAveTimes == 1){
				if(iLoop == 1){
					for(i = 0; i < length / 2 ; i++) {
							cmcp_info->diff_sensor_data[i] = parade_combine2byte(&dad->ic_buf[i * 2]);
					}
				} else if(iLoop == 0){
					for(i = 0; i < length / 2 ; i++) {
							cmcp_info->diff_self_data[i] = parade_combine2byte(&dad->ic_buf[i * 2]);
					}
				}
			} else if(iAveTimes==0){
				if(iLoop == 1){
					for(i = 0; i < length / 2 ; i++) {
							cmcp_info->diff_sensor_data[i] += parade_combine2byte(&dad->ic_buf[i * 2]);
							cmcp_info->diff_sensor_data[i] /=2;
					}
				} else if(iLoop == 0){
					for(i = 0; i < length / 2 ; i++) {
							cmcp_info->diff_self_data[i] += parade_combine2byte(&dad->ic_buf[i * 2]);
							cmcp_info->diff_self_data[i] /=2;
					}
				}
			}
			msleep(120);//Sleep 120ms for make sure no mc scan is trigger by timeout
		}
	}
release_mutex:
	mutex_unlock(&dad->debugfs_lock);
	return 0;
 }

static int parade_oem_info_switch(struct ts_oem_info_param *info)
{
	int retval = NO_ERR;
	int t = 0;
	int timeout_ms = PARADE_WORK_WAIT_TIMEOUT;
	TS_LOG_INFO("%s: info[0]=0x%2x, info[1]=0x%2x\n", __func__,info->data[0], info->data[1]);

	if(g_ts_kit_platform_data.chip_data->get_brightness_info_flag == 0)
        {
		TS_LOG_INFO("%s:get_brightness_info_flag is 0, will return\n", __func__);
		return retval; //some ic (cs448) no this function. may print error when boot.
        }

	retval = parade_check_cmd_status();
	if(retval)
		return retval;

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		retval = parade_set_oem_info(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, parade_set_oem_info faild\n",
					__func__);
		}
		break;
	case TS_ACTION_READ:
		retval = parade_get_oem_info(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, parade_get_oem_info faild\n",
					__func__);
		}
		break;
	default:
		TS_LOG_INFO("invalid oem info switch(%d) action: %d\n",
				info->data_switch, info->op_action);
		retval = -EINVAL;
		break;
	}
	parade_finish_cmd();
	return retval;
 }

static int parade_get_project_id(struct ts_oem_info_param *info)
{
	int rc = NO_ERR;
	TS_LOG_INFO("%s: Enter\n", __func__);
	if(info == NULL){
		rc = -EINVAL;
		TS_LOG_ERR("%s: Error\n", __func__);
		goto exit;
	}
	if(info->buff[0] != TS_CHIP_PROJECTID_TYPE) /*product id is locates at first 10 bytes*/{
		memcpy(tskit_parade_data->project_id,info->buff,TS_CHIP_PROJECTID_LENGTH);
	} else {
		memcpy(tskit_parade_data->project_id,info->buff + 2,TS_CHIP_PROJECTID_LENGTH); /*Add offset*/
	}
	tskit_parade_data->project_id[TS_CHIP_PROJECTID_LENGTH] = 0;
	TS_LOG_INFO("%s, project id in the TPIC Flash is %s\n",__func__,
		tskit_parade_data->project_id);
exit:
	return rc;
}

static int parade_get_brightness_info(void)
{
	int bl_max_nit = 0;
	int rc = NO_ERR;
	struct ts_oem_info_param *info =NULL;
	TS_LOG_INFO("%s: Enter\n", __func__);
	if((tskit_parade_data->need_provide_projectID_for_sensor) && (tskit_parade_data->create_project_id_flag == 1)){//To generate projectiD for shutdown charging
		rc = parade_create_project_id();
		if (rc < 0) {
			TS_LOG_ERR("%s: create project id failed!\n", __func__);
		}
	}

	if(g_ts_kit_platform_data.chip_data->get_brightness_info_flag == 0)
		return rc; //some ic (cs448) no this function.
	rc = parade_check_cmd_status();
	if(rc)
		return rc;

	//setting the read brightness type
	memset(tp_type_cmd, 0x0, TS_CHIP_TYPE_MAX_SIZE);
	tp_type_cmd[0] = TS_CHIP_BRIGHTNESS_TYPE;

	if (TS_UNINIT == atomic_read(&tskit_parade_data->parade_chip_data->ts_platform_data->state)) {
		TS_LOG_INFO("%s:ts module not initialize\n", __func__);
		bl_max_nit = 0;
		goto out;
	}

	info =
	(struct ts_oem_info_param *)
		kzalloc(sizeof(struct ts_oem_info_param), GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("%s: malloc failed\n", __func__);
		info = NULL;
		rc = -ENOMEM;
		goto out;
	}

	if(tskit_parade_data->mode == CY_MODE_BOOTLOADER){
		TS_LOG_ERR("%s:In bootloader mode, such function does not support\n", __func__);
		rc = -EINVAL;
		goto out;
	}

	rc = parade_get_oem_info(info);
	if(rc != 0) {
		TS_LOG_ERR("%s:brightness info is not find in TPIC FLASH\n", __func__);
		rc = -EIO;
		goto out;
	}
	TS_LOG_INFO("%s:info->data[0]=%d,info->data[1]=%d\n", __func__,info->data[0],info->data[1]);

	if(info->data[0] == 0x1) {
		TS_LOG_INFO("%s:brightness info is not find in TPIC FLASH\n", __func__);
		bl_max_nit = 0;
	} else {
		TS_LOG_INFO("%s:brightness info find in TPIC FLASH\n", __func__);
		bl_max_nit = info->data[3] << 8 | info->data[2];
	}
	rc = parade_get_project_id(info);
out:
	if (info) {
		kfree(info);
		info = NULL;
    }
	parade_finish_cmd();
	if(rc != NO_ERR)
		return rc;
	else
		return bl_max_nit;

}

static void parade_reset_pin_low(void)
{
	TS_LOG_INFO("%s:enter\n", __func__);
	atomic_set(&parade_reset_pin_status,0);
	gpio_set_value(tskit_parade_data->parade_chip_data->ts_platform_data->reset_gpio, 0);
}

static void parade_reset_pin_high(void)
{
	TS_LOG_INFO("%s:enter\n", __func__);
	gpio_set_value(tskit_parade_data->parade_chip_data->ts_platform_data->reset_gpio, 1);
	atomic_set(&parade_reset_pin_status,1);
}

static void parade_hw_reset(void)
{
	TS_LOG_INFO("%s:reset gpio is %d\n", __func__,tskit_parade_data->parade_chip_data->ts_platform_data->reset_gpio);
	parade_reset_pin_high();
	msleep(2);
	parade_reset_pin_low();
	msleep(2);
	parade_reset_pin_high();
	msleep(CY_HID_RESUME_STARTUP_TIMEOUT);
	return NO_ERR;
}

static inline void debug_print_i2c_transfer(bool action, u8 addr, u8 *buf, int size)
{
	int index = 0;

	if(action)
		TS_LOG_DEBUG("[parade debug] i2c action write addr:0x%x, size:%d\n", addr, size);
	else
		TS_LOG_DEBUG("[parade debug] i2c action read addr:0x%x, size:%d\n", addr, size);

	for(;index < size; index++)
		TS_LOG_DEBUG("[parade debug] buf value :0x%x\n", *(buf+index));
}

static int parade_i2c_read(u8 *reg_addr, u16 reg_len, u8 *buf, u16 len)
{
	int rc = 0;

	if(parade_is_power_on){
		rc = tskit_parade_data->parade_chip_data->ts_platform_data->bops->bus_read(reg_addr, reg_len, buf, len);
		if (rc < 0)
			 TS_LOG_ERR("%s,fail read	rc=%d\n", __func__, rc);
		debug_print_i2c_transfer(false, *reg_addr, buf, len);
	}else{
		TS_LOG_ERR("%s,i2c ops is not allow at power off\n",__func__);
		rc = -EINVAL;
	}
	return rc;
 }
static int parade_i2c_write(u8* buf, u16 length)
{
	int rc = 0;

	if(parade_is_power_on){
		rc = tskit_parade_data->parade_chip_data->ts_platform_data->bops->bus_write(buf,length);
		if (rc < 0)
			 TS_LOG_ERR("%s,fail write	rc=%d\n", __func__, rc);
		debug_print_i2c_transfer(true, 0, buf, length);
	}else{
		TS_LOG_ERR("%s,i2c ops is not allow at power off\n",__func__);
		rc = -EINVAL;
	}
	return rc;

}

static int i2c_communicate_check(void)
{
	int rc = NO_ERR;
	int i;
	u8 reg_addr = 0x00;
	u8 mode = 0;

	for (i = 0; i < I2C_RW_TRIES; i ++) {
		rc = parade_i2c_read(&reg_addr, sizeof(reg_addr), &mode, sizeof(mode));
		if (rc < 0) {
			TS_LOG_ERR("%s, Failed to read register 0x00, i = %d, rc = %d\n", __func__, i, rc);
			msleep(50);
		} else {
			TS_LOG_INFO("%s, i2c communicate check success\n", __func__);
			rc = NO_ERR;
			return rc;
		}
	}

	return rc;
}


static int parade_check_and_deassert_int(struct parade_core_data *cd)
{
	u8 reg_addr = PAR_REG_BASE;
	u16 size = 0;
	u8 buf[200] = {0};
	u8 *p = NULL;
	u8 retry = 3;
	int rc = 0;
	do {
		rc = parade_i2c_read(&reg_addr, sizeof(reg_addr), buf, 2);
		if (rc < 0)
			return rc;
		size = get_unaligned_le16(&buf[0]);
		TS_LOG_INFO("%s size=%d\n", __func__, size);

		if (size == 2 || size == 0 || size >= CY_PIP_1P7_EMPTY_BUF)
			return 0;

		p = kzalloc(size, GFP_KERNEL);
		if (!p) {
			TS_LOG_ERR("%s kzalloc failed\n", __func__);
			return -ENOMEM;
		}

		rc = parade_i2c_read(&reg_addr, sizeof(reg_addr), p, size);
		kfree(p);
		if (rc < 0)
			return rc;
		msleep(10);//check every 10ms
	} while (retry--);

	return rc;
}
static int parade_get_hid_descriptor_(struct parade_core_data *cd,
		struct parade_hid_desc *desc)
{
	int rc;
	u8 cmd[2];
	int size;
	TS_LOG_INFO("%s enter\n", __func__);
	int t;

	/* Read HID descriptor length and version */
	mutex_lock(&cd->system_lock);
	cd->hid_cmd_state = 1;
	mutex_unlock(&cd->system_lock);
	/* Set HID descriptor register */
	memcpy(cmd, &cd->core_pdata->hid_desc_register,
		sizeof(cd->hid_core.hid_desc_register));

	rc = parade_i2c_write(cmd, 2);
	if (rc) {
		TS_LOG_DEBUG("%s: failed to get HID descriptor length and version, rc=%d\n",
			__func__, rc);
		goto error;
	}

	t = wait_event_timeout(cd->wait_q, (cd->hid_cmd_state == 0),
			       msecs_to_jiffies
			       (CY_HID_GET_HID_DESCRIPTOR_TIMEOUT));
	if (IS_TMO(t)) {
		TS_LOG_ERR("%s: HID get descriptor timed out\n", __func__);
		rc = -ETIME;
		goto error;
	}

	memcpy((u8 *)desc, cd->response_buf, sizeof(struct parade_hid_desc));
	/* Check HID descriptor length and version */
	TS_LOG_DEBUG("%s: HID len:%X HID ver:%X\n", __func__,
		le16_to_cpu(desc->hid_desc_len),
		le16_to_cpu(desc->bcd_version));

	if (le16_to_cpu(desc->hid_desc_len) != sizeof(*desc) ||
		le16_to_cpu(desc->bcd_version) != CY_HID_VERSION) {
		TS_LOG_DEBUG("%s: Unsupported HID version\n", __func__);
		return -ENODEV;
	}

	goto exit;

error:
	mutex_lock(&cd->system_lock);
	cd->hid_cmd_state = 0;
	mutex_unlock(&cd->system_lock);
exit:
	return rc;
}

static int parade_reset(struct parade_core_data *cd)
{
	/* reset hardware */
	TS_LOG_DEBUG("%s: reset hw...\n", __func__);
	parade_hw_reset();
	return 0;
}

static int parade_reset_and_wait(struct parade_core_data *cd)
{
	int rc;
	int t;

	TS_LOG_INFO("%s Enter",__func__);
	mutex_lock(&cd->system_lock);
	cd->hid_reset_cmd_state = 1;
	mutex_unlock(&cd->system_lock);

	rc = parade_reset(cd);
	if (rc < 0)
		goto error;

	/*lint -save -e* */
	t = wait_event_timeout(cd->wait_q, (cd->hid_reset_cmd_state == 0),
				   msecs_to_jiffies(cd->cy_hid_reset_timeout));
	/*lint -restore*/

	if (IS_TMO(t)) {
		TS_LOG_ERR("%s: reset timed out\n", __func__);
		rc = -ETIME;
		goto error;
	}

	goto exit;

error:
	mutex_lock(&cd->system_lock);
	cd->hid_reset_cmd_state = 0;
	mutex_unlock(&cd->system_lock);
exit:
	return rc;

}


/* Must be called with cd->hid_report_lock acquired */
static void parade_free_hid_reports_(struct parade_core_data *cd)
{
	struct parade_hid_report *report;
	int i, j;

	for (i = 0; i < cd->num_hid_reports; i++) {
		report = cd->hid_reports[i];
		for (j = 0; j < report->num_fields; j++)
			kfree(report->fields[j]);
		kfree(report);
		cd->hid_reports[i] = NULL;
	}
	cd->num_hid_reports = 0;
}
static inline int get_hid_item_data(u8 *data, int item_size)
{
	if (item_size == 1)
		return (int)*data;
	else if (item_size == 2)
		return (int)get_unaligned_le16(data);
	else if (item_size == 4)
		return (int)get_unaligned_le32(data);
	return 0;
}

/* Must be called with cd->hid_report_lock acquired */
static struct parade_hid_report *parade_get_hid_report_(
		struct parade_core_data *cd, u8 report_type, u8 report_id,
		bool create)
{
	struct parade_hid_report *report = NULL;
	int i;

	/* Look for created reports */
	for (i = 0; i < cd->num_hid_reports; i++) {
		if (cd->hid_reports[i]->type == report_type
				&& cd->hid_reports[i]->id == report_id) {
			return cd->hid_reports[i];
		}
	}

	/* Create a new report */
	if (create && cd->num_hid_reports < CY_HID_MAX_REPORTS) {
		report = kzalloc(sizeof(struct parade_hid_report),
				GFP_KERNEL);
		if (!report)
			return NULL;

		report->type = report_type;
		report->id = report_id;
		cd->hid_reports[cd->num_hid_reports++] = report;
	}

	return report;
}
/* Must be called with cd->hid_report_lock acquired */
static struct parade_hid_field *parade_create_hid_field_(
		struct parade_hid_report *report)
{
	struct parade_hid_field *field;

	if (!report)
		return NULL;

	if (report->num_fields == CY_HID_MAX_FIELDS)
		return NULL;

	field = kzalloc(sizeof(struct parade_hid_field), GFP_KERNEL);
	if (!field)
		return NULL;

	field->report = report;

	report->fields[report->num_fields++] = field;

	return field;
}

static int parse_report_descriptor(struct parade_core_data *cd,
		u8 *report_desc, size_t len)
{
	struct parade_hid_report *report;
	struct parade_hid_field *field;
	u8 *buf = report_desc;
	u8 *end = buf + len;
	int rc = 0;
	int offset = 0;
	int i;
	u8 report_type;
	u32 up_usage;
	/* Global items */
	u8 report_id = 0;
	u16 usage_page = 0;
	int report_count = 0;
	int report_size = 0;
	int logical_min = 0;
	int logical_max = 0;
	/* Local items */
	u16 usage = 0;
	/* Main items - Collection stack */
	u32 collection_usages[CY_HID_MAX_NESTED_COLLECTIONS] = {0};
	u8 collection_types[CY_HID_MAX_NESTED_COLLECTIONS] = {0};
	/* First collection for header, second for report */
	int logical_collection_count = 0;
	int collection_nest = 0;

	TS_LOG_DEBUG("%s: Report descriptor length: %u\n",
		__func__, (u32)len);

	mutex_lock(&cd->hid_report_lock);
	parade_free_hid_reports_(cd);

	while (buf < end) {
		int item_type;
		int item_size;
		int item_tag;
		u8 *data;

		/* Get Item */
		item_size = HID_GET_ITEM_SIZE(buf[0]);
		if (item_size == 3)
			item_size = 4;
		item_type = HID_GET_ITEM_TYPE(buf[0]);
		item_tag = HID_GET_ITEM_TAG(buf[0]);

		data = ++buf;
		buf += item_size;

		/* Process current item */
		switch (item_type) {
		case HID_ITEM_TYPE_GLOBAL:
			switch (item_tag) {
			case HID_GLOBAL_ITEM_TAG_REPORT_ID:
				if (item_size != 1) {
					rc = -EINVAL;
					goto exit;
				}
				report_id = get_hid_item_data(data, item_size);
				offset = 0;
				logical_collection_count = 0;
				break;
			case HID_GLOBAL_ITEM_TAG_USAGE_PAGE:
				if (item_size == 0 || item_size == 4) {
					rc = -EINVAL;
					goto exit;
				}
				usage_page = (u16)get_hid_item_data(data,
						item_size);
				break;
			case HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM:
				if (item_size == 0) {
					rc = -EINVAL;
					goto exit;
				}
				logical_min = get_hid_item_data(data,
						item_size);
				break;
			case HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM:
				if (item_size == 0) {
					rc = -EINVAL;
					goto exit;
				}
				logical_max = get_hid_item_data(data,
						item_size);
				break;
			case HID_GLOBAL_ITEM_TAG_REPORT_COUNT:
				if (item_size == 0) {
					rc = -EINVAL;
					goto exit;
				}
				report_count = get_hid_item_data(data,
						item_size);
				break;
			case HID_GLOBAL_ITEM_TAG_REPORT_SIZE:
				if (item_size == 0) {
					rc = -EINVAL;
					goto exit;
				}
				report_size = get_hid_item_data(data,
						item_size);
				break;
			default:
				TS_LOG_INFO("%s: Unrecognized Global tag %d\n",
					__func__, item_tag);
			}
			break;
		case HID_ITEM_TYPE_LOCAL:
			switch (item_tag) {
			case HID_LOCAL_ITEM_TAG_USAGE:
				if (item_size == 0 || item_size == 4) {
					rc = -EINVAL;
					goto exit;
				}
				usage = (u16)get_hid_item_data(data,
						item_size);
				break;
			case HID_LOCAL_ITEM_TAG_USAGE_MINIMUM:
				if (item_size == 0) {
					rc = -EINVAL;
					goto exit;
				}
				/* usage_min = */
				get_hid_item_data(data, item_size);
				break;
			case HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM:
				if (item_size == 0) {
					rc = -EINVAL;
					goto exit;
				}
				/* usage_max = */
				get_hid_item_data(data, item_size);
				break;
			default:
				TS_LOG_INFO("%s: Unrecognized Local tag %d\n",
					__func__, item_tag);
			}
			break;
		case HID_ITEM_TYPE_MAIN:
			switch (item_tag) {
			case HID_MAIN_ITEM_TAG_BEGIN_COLLECTION:
				if (item_size != 1) {
					rc = -EINVAL;
					goto exit;
				}
				if (CY_HID_MAX_NESTED_COLLECTIONS ==
						collection_nest) {
					rc = -EINVAL;
					goto exit;
				}

				up_usage = usage_page << 16 | usage;

				/* Update collection stack */
				collection_usages[collection_nest] = up_usage;
				collection_types[collection_nest] =
					get_hid_item_data(data, item_size);

				if (collection_types[collection_nest] ==
						HID_COLLECTION_LOGICAL)
					logical_collection_count++;

				collection_nest++;
				break;
			case HID_MAIN_ITEM_TAG_END_COLLECTION:
				if (item_size != 0) {
					rc = -EINVAL;
					goto exit;
				}
				if (--collection_nest < 0) {
					rc = -EINVAL;
					goto exit;
				}
				break;
			case HID_MAIN_ITEM_TAG_INPUT:
				report_type = HID_INPUT_REPORT;
				goto continue_main_item;
			case HID_MAIN_ITEM_TAG_OUTPUT:
				report_type = HID_OUTPUT_REPORT;
				goto continue_main_item;
			case HID_MAIN_ITEM_TAG_FEATURE:
				report_type = HID_FEATURE_REPORT;
continue_main_item:
				if (item_size != 1) {
					rc = -EINVAL;
					goto exit;
				}

				up_usage = usage_page << 16 | usage;

				/* Get or create report */
				report = parade_get_hid_report_(cd,
						report_type, report_id, true);
				if (!report) {
					rc = -ENOMEM;
					goto exit;
				}
				if (!report->usage_page && collection_nest > 0)
					report->usage_page =
						collection_usages
							[collection_nest - 1];

				/* Create field */
				field = parade_create_hid_field_(report);
				if (!field) {
					rc = -ENOMEM;
					goto exit;
				}

				field->report_count = report_count;
				field->report_size = report_size;
				field->size = report_count * report_size;
				field->offset = offset;
				field->data_type =
					get_hid_item_data(data, item_size);
				field->logical_min = logical_min;
				field->logical_max = logical_max;
				field->usage_page = up_usage;

				for (i = 0; i < collection_nest; i++) {
					field->collection_usage_pages
							[collection_types[i]] =
						collection_usages[i];
				}

				/* Update report's header or record size */
				if (logical_collection_count == 1) {
					report->header_size += field->size;
				} else if (logical_collection_count == 2) {
					field->record_field = true;
					field->offset -= report->header_size;
					/* Set record field index */
					if (report->record_field_index == 0)
						report->record_field_index =
							report->num_fields - 1;
					report->record_size += field->size;
				}

				report->size += field->size;

				offset += field->size;
				break;
			default:
				TS_LOG_INFO("%s: Unrecognized Main tag %d\n",
					__func__, item_tag);
			}

			/* Reset all local items */
			usage = 0;
			break;
		}
	}

	if (buf != end) {
		TS_LOG_DEBUG("%s: Report descriptor length invalid\n",
			__func__);
		rc = -EINVAL;
		goto exit;
	}

	if (collection_nest) {
		TS_LOG_DEBUG("%s: Unbalanced collection items (%d)\n",
			__func__, collection_nest);
		rc = -EINVAL;
		goto exit;
	}

exit:
	if (rc)
		parade_free_hid_reports_(cd);
	mutex_unlock(&cd->hid_report_lock);
	return rc;
}
static struct parade_hid_field *find_report_desc_field(
		struct parade_core_data *cd, u32 usage_page,
		u32 collection_usage_page)
{
	struct parade_hid_report *report = NULL;
	struct parade_hid_field *field = NULL;
	int i;
	int j;
	u32 field_cup;
	u32 field_up;

	for (i = 0; i < cd->num_hid_reports; i++) {
		report = cd->hid_reports[i];
		for (j = 0; j < report->num_fields; j++) {
			field = report->fields[j];
			field_cup = field->collection_usage_pages
				[HID_COLLECTION_LOGICAL];
			field_up = field->usage_page;
			if (field_cup == collection_usage_page
					&& field_up == usage_page) {
				return field;
			}
		}
	}

	return NULL;
}
static int fill_tch_abs(struct parade_tch_abs_params *tch_abs,
		struct parade_hid_field *field)
{
	tch_abs->ofs = field->offset / 8;
	tch_abs->size = field->report_size / 8;
	if (field->report_size % 8)
		tch_abs->size += 1;
	tch_abs->min = 0;
	tch_abs->max = 1 << field->report_size;
	tch_abs->bofs = field->offset - (tch_abs->ofs << 3);

	return 0;
}

static struct parade_hid_report *find_report_desc(struct parade_core_data *cd,
		u32 usage_page)
{
	struct parade_hid_report *report = NULL;
	int i;

	for (i = 0; i < cd->num_hid_reports; i++) {
		if (cd->hid_reports[i]->usage_page == usage_page) {
			report = cd->hid_reports[i];
			break;
		}
	}

	return report;
}

static int setup_report_descriptor(struct parade_core_data *cd)
{
	struct parade_sysinfo *si = &cd->sysinfo;
	struct parade_hid_report *report;
	struct parade_hid_field *field;
	int i;
	u32 tch_collection_usage_page = HID_CY_TCH_COL_USAGE_PG;
	u32 btn_collection_usage_page = HID_CY_BTN_COL_USAGE_PG;

	for (i = CY_TCH_X; i < CY_TCH_NUM_ABS; i++) {
		field = find_report_desc_field(cd,
				parade_tch_abs_field_map[i],
				tch_collection_usage_page);
		if (field) {
			TS_LOG_DEBUG(
				" Field %p: rep_cnt:%d rep_sz:%d off:%d data:%02X min:%d max:%d usage_page:%08X\n",
				field, field->report_count, field->report_size,
				field->offset, field->data_type,
				field->logical_min, field->logical_max,
				field->usage_page);
			fill_tch_abs(&si->tch_abs[i], field);
			si->tch_abs[i].report = 1;
			TS_LOG_DEBUG("%s: ofs:%u size:%u min:%u max:%u bofs:%u report:%d",
				parade_tch_abs_string[i],
				(u32)si->tch_abs[i].ofs,
				(u32)si->tch_abs[i].size,
				(u32)si->tch_abs[i].min,
				(u32)si->tch_abs[i].max,
				(u32)si->tch_abs[i].bofs,
				si->tch_abs[i].report);

		} else {
			si->tch_abs[i].report = 0;
		}
	}
	for (i = CY_TCH_TIME; i < CY_TCH_NUM_HDR; i++) {
		field = find_report_desc_field(cd,
				parade_tch_hdr_field_map[i],
				tch_collection_usage_page);
		if (field) {
			TS_LOG_DEBUG(
				" Field %p: rep_cnt:%d rep_sz:%d off:%d data:%02X min:%d max:%d usage_page:%08X\n",
				field, field->report_count, field->report_size,
				field->offset, field->data_type,
				field->logical_min, field->logical_max,
				field->usage_page);
			fill_tch_abs(&si->tch_hdr[i], field);
			si->tch_hdr[i].report = 1;
			TS_LOG_DEBUG( "%s: ofs:%u size:%u min:%u max:%u bofs:%u report:%d",
				parade_tch_hdr_string[i],
				(u32)si->tch_hdr[i].ofs,
				(u32)si->tch_hdr[i].size,
				(u32)si->tch_hdr[i].min,
				(u32)si->tch_hdr[i].max,
				(u32)si->tch_hdr[i].bofs,
				si->tch_hdr[i].report);

		} else {
			si->tch_hdr[i].report = 0;
		}
	}

	report = find_report_desc(cd, tch_collection_usage_page);
	if (report) {
		si->desc.tch_report_id = report->id;
		si->desc.tch_record_size = report->record_size / 8;
		si->desc.tch_header_size = (report->header_size / 8) + 3;
	} else {
		si->desc.tch_report_id = HID_TOUCH_REPORT_ID;
		si->desc.tch_record_size = TOUCH_REPORT_SIZE;
		si->desc.tch_header_size = TOUCH_INPUT_HEADER_SIZE;
	}

	report = find_report_desc(cd, btn_collection_usage_page);
	if (report)
		si->desc.btn_report_id = report->id;
	else
		si->desc.btn_report_id = HID_BTN_REPORT_ID;

	for (i = 0; i < cd->num_hid_reports; i++) {
		struct parade_hid_report *report = cd->hid_reports[i];

		switch (report->id) {
		case HID_WAKEUP_REPORT_ID:
			cd->features.easywake = 1;
			break;
		case HID_NOISE_METRIC_REPORT_ID:
			cd->features.noise_metric = 1;
			break;
		case HID_TRACKING_HEATMAP_REPOR_ID:
			cd->features.tracking_heatmap = 1;
			break;
		case HID_SENSOR_DATA_REPORT_ID:
			cd->features.sensor_data = 1;
			break;
		default:
			break;
		}
	}

	TS_LOG_DEBUG( "Features: easywake:%d noise_metric:%d tracking_heatmap:%d sensor_data:%d\n",
			cd->features.easywake, cd->features.noise_metric,
			cd->features.tracking_heatmap,
			cd->features.sensor_data);

	return 0;
}

static int parade_get_report_descriptor_(struct parade_core_data *cd)
{
	u8 cmd[2];
	int rc;
	int t;
	int size;

	/* Read report descriptor length and version */
	mutex_lock(&cd->system_lock);
	cd->hid_cmd_state = 1;
	mutex_unlock(&cd->system_lock);

	/* Set report descriptor register */
	memcpy(cmd, &cd->hid_desc.report_desc_register,
		sizeof(cd->hid_desc.report_desc_register));
	TS_LOG_DEBUG("%s report_desc_register :%d\n", __func__, cd->hid_desc.report_desc_register);
	rc = parade_i2c_write(cmd, 2);
	if (rc) {
		TS_LOG_DEBUG("%s: failed to get HID descriptor length and version, rc=%d\n",
			__func__, rc);
		goto error;
	}
	t = wait_event_timeout(cd->wait_q, (cd->hid_cmd_state == 0),
			       msecs_to_jiffies
			       (CY_HID_GET_REPORT_DESCRIPTOR_TIMEOUT));
	if (IS_TMO(t)) {
		TS_LOG_ERR("%s: HID get descriptor timed out\n", __func__);
		rc = -ETIME;
		goto error;
	}

	cyttsp5_pr_buf(cd->response_buf,
		       cd->hid_core.hid_report_desc_len, "Report Desc");

	if(cd->need_check_report_descriptor_flag) {
		TS_LOG_INFO("%s, check_repord_descriptor report id\n", __func__);
		if (cd->response_buf[HID_OUTPUT_RESPONSE_REPORT_OFFSET] !=
							   CY_HID_APP_REPORT_DESCRIPTOR_PACKET_ID)
		{
			TS_LOG_ERR("%s: Get Report descriptor, wrong  report_id:%X\n", __func__,
					cd->response_buf[HID_OUTPUT_RESPONSE_REPORT_OFFSET]);
			rc = -EPROTO;
			goto error;
		}
	}

	rc = parse_report_descriptor(cd, cd->response_buf + 3,
		get_unaligned_le16(&cd->response_buf[0]) - 3);
	if (rc) {
		TS_LOG_DEBUG("%s: Error parsing report descriptor r=%d\n",
			__func__, rc);
	}

	TS_LOG_DEBUG("%s: %d reports found in descriptor\n", __func__,
		cd->num_hid_reports);

	for (t = 0; t < cd->num_hid_reports; t++) {
		struct parade_hid_report *report = cd->hid_reports[t];
		int j;

		TS_LOG_DEBUG(
			"Report %d: type:%d id:%02X size:%d fields:%d rec_fld_index:%d hdr_sz:%d rec_sz:%d usage_page:%08X\n",
			t, report->type, report->id,
			report->size, report->num_fields,
			report->record_field_index, report->header_size,
			report->record_size, report->usage_page);

		for (j = 0; j < report->num_fields; j++) {
			struct parade_hid_field *field = report->fields[j];

			TS_LOG_DEBUG(
				" Field %d: rep_cnt:%d rep_sz:%d off:%d data:%02X min:%d max:%d usage_page:%08X\n",
				j, field->report_count, field->report_size,
				field->offset, field->data_type,
				field->logical_min, field->logical_max,
				field->usage_page);

			TS_LOG_DEBUG("  Collections Phys:%08X App:%08X Log:%08X\n",
				field->collection_usage_pages
					[HID_COLLECTION_PHYSICAL],
				field->collection_usage_pages
					[HID_COLLECTION_APPLICATION],
				field->collection_usage_pages
					[HID_COLLECTION_LOGICAL]);
		}
	}

	rc = setup_report_descriptor(cd);

	/* Free it for now */
	parade_free_hid_reports_(cd);

	TS_LOG_DEBUG("%s: %d reports found in descriptor\n", __func__,
		cd->num_hid_reports);

	goto exit;

error:
	mutex_lock(&cd->system_lock);
	cd->hid_cmd_state = 0;
	mutex_unlock(&cd->system_lock);
exit:
	return rc;
}

static u16 _parade_compute_crc(u8 *buf, u32 size)
{
	u16 remainder = 0xFFFF;
	u16 xor_mask = 0x0000;
	u32 index;
	u32 byte_value;
	u32 table_index;
	u32 crc_bit_width = sizeof(u16) * 8;

	/* Divide the message by polynomial, via the table. */
	for (index = 0; index < size; index++) {
		byte_value = buf[index];
		table_index = ((byte_value >> 4) & 0x0F)
			^ (remainder >> (crc_bit_width - 4));
		remainder = crc_table[table_index] ^ (remainder << 4);
		table_index = (byte_value & 0x0F)
			^ (remainder >> (crc_bit_width - 4));
		remainder = crc_table[table_index] ^ (remainder << 4);
	}

	/* Perform the final remainder CRC. */
	return remainder ^ xor_mask;
}

static int parade_hid_send_output_(struct parade_core_data *cd,
		struct parade_hid_output *hid_output)
{
	int rc;
	u8 *cmd;
	u16 length;
	u8 report_id;
	u8 cmd_offset = 0;
	u16 crc;
	u8 cmd_allocated = 0;

	switch (hid_output->cmd_type) {
	case HID_OUTPUT_CMD_APP:
		report_id = HID_APP_OUTPUT_REPORT_ID;
		length = 5;
		break;
	case HID_OUTPUT_CMD_BL:
		report_id = HID_BL_OUTPUT_REPORT_ID;
		length = 11 /* 5 + SOP + LEN(2) + CRC(2) + EOP */;
		break;
	default:
		return -EINVAL;
	}
	length += hid_output->write_length;

	if (length + 2 > PARADE_PREALLOCATED_CMD_BUFFER) {
		cmd = kzalloc(length + 2, GFP_KERNEL);
		if (!cmd)
			return -ENOMEM;
		cmd_allocated = 1;
	} else {
		cmd = cd->cmd_buf;
	}

	/* Set Output register */
	memcpy(&cmd[cmd_offset], &cd->hid_desc.output_register,
			sizeof(cd->hid_desc.output_register));
	cmd_offset += sizeof(cd->hid_desc.output_register);

	cmd[cmd_offset++] = LOW_BYTE(length);
	cmd[cmd_offset++] = HI_BYTE(length);
	cmd[cmd_offset++] = report_id;
	cmd[cmd_offset++] = 0x0; /* reserved */
	if (hid_output->cmd_type == HID_OUTPUT_CMD_BL)
		cmd[cmd_offset++] = HID_OUTPUT_BL_SOP;
	cmd[cmd_offset++] = hid_output->command_code;

	/* Set Data Length for bootloader */
	if (hid_output->cmd_type == HID_OUTPUT_CMD_BL) {
		cmd[cmd_offset++] = LOW_BYTE(hid_output->write_length);
		cmd[cmd_offset++] = HI_BYTE(hid_output->write_length);
	}
	/* Set Data */
	if (hid_output->write_length && hid_output->write_buf) {
		memcpy(&cmd[cmd_offset], hid_output->write_buf,
				hid_output->write_length);
		cmd_offset += hid_output->write_length;
	}

	if (hid_output->cmd_type == HID_OUTPUT_CMD_BL) {
		crc = _parade_compute_crc(&cmd[6],
				hid_output->write_length + 4);
		cmd[cmd_offset++] = LOW_BYTE(crc);
		cmd[cmd_offset++] = HI_BYTE(crc);
		cmd[cmd_offset++] = HID_OUTPUT_BL_EOP;
	}

	cyttsp5_pr_buf(cmd, length + 2, "command");
	rc = parade_i2c_write(cmd, length + 2);
	if (rc)
		TS_LOG_DEBUG("%s: Fail parade_adap_transfer\n", __func__);

	if (cmd_allocated)
		kfree(cmd);
	return rc;
}

static int parade_hid_send_output_user_(struct parade_core_data *cd,
		struct parade_hid_output *hid_output)
{
	int rc;

	if (!hid_output->length || !hid_output->write_buf)
		return -EINVAL;

	rc = parade_i2c_write(hid_output->write_buf, hid_output->length);
	if (rc)
		TS_LOG_ERR("%s: Fail parade_adap_transfer\n", __func__);

	return rc;
}

static int parade_hid_send_output_user_and_wait_(struct parade_core_data *cd,
		struct parade_hid_output *hid_output)
{
	int rc;
	int t;

	mutex_lock(&cd->system_lock);
	cd->hid_cmd_state = HID_OUTPUT_USER_CMD + 1;
	mutex_unlock(&cd->system_lock);

	rc = parade_hid_send_output_user_(cd, hid_output);
	if (rc)
		goto error;

	t = wait_event_timeout(cd->wait_q, (cd->hid_cmd_state == 0),
			msecs_to_jiffies(CY_HID_OUTPUT_USER_TIMEOUT));
	if (IS_TMO(t)) {
#if defined (CONFIG_HUAWEI_DSM)
		ts_dmd_report(DSM_TP_IC_TIMEOUT_ERROR_NO, "parade output cmd execution timed out.\n");
#endif
		TS_LOG_ERR("%s: HID output cmd execution timed out\n",
			__func__);
		rc = -ETIME;
		goto error;
	}
	goto exit;

error:
	mutex_lock(&cd->system_lock);
	cd->hid_cmd_state = 0;
	mutex_unlock(&cd->system_lock);

exit:
	return rc;
}

static int parade_hid_output_user_cmd_(struct parade_core_data *cd,
		u16 read_len, u8 *read_buf, u16 write_len, u8 *write_buf,
		u16 *actual_read_len)
{
	int rc;
	u16 size;
#ifdef TTHE_TUNER_SUPPORT
	int command_code = 0;
	int len;
#endif
	struct parade_hid_output hid_output = {
		.length = write_len,
		.write_buf = write_buf,
	};

	rc = parade_hid_send_output_user_and_wait_(cd, &hid_output);
	if (rc)
		return rc;

	size = get_unaligned_le16(&cd->response_buf[0]);
	if (size == 0)
		size = 2;

	if (size > read_len) {
		*actual_read_len = 0;
		return -EINVAL;
	}

	memcpy(read_buf, cd->response_buf, size);
	*actual_read_len = size;

#ifdef TTHE_TUNER_SUPPORT
	/* print up to cmd code */
	len = HID_OUTPUT_CMD_OFFSET + 1;
	if (write_len < len)
		len = write_len;
	else
		command_code = write_buf[HID_OUTPUT_CMD_OFFSET]
			& HID_OUTPUT_CMD_MASK;

	/* Do not print for EXEC_PANEL_SCAN & RETRIEVE_PANEL_SCAN commands */
	if (command_code != HID_OUTPUT_EXEC_PANEL_SCAN
			&& command_code != HID_OUTPUT_RETRIEVE_PANEL_SCAN)
		tthe_print(cd, write_buf, len, "CMD=");
#endif

	return 0;
}

static ssize_t parade_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct parade_device_access_data *dad = &tskit_parade_data->dad;
	u8 val;

	mutex_lock(&dad->sysfs_lock);
	val = dad->status;
	mutex_unlock(&dad->sysfs_lock);

	return scnprintf(buf, PARADE_MAX_PRBUF_SIZE, "%d\n", val);
}

static DEVICE_ATTR(status, S_IRUSR, parade_status_show, NULL);

static ssize_t parade_response_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct parade_device_access_data *dad = &tskit_parade_data->dad;
	int i;
	ssize_t num_read;
	int index;

	mutex_lock(&dad->sysfs_lock);
	index = scnprintf(buf, PARADE_MAX_PRBUF_SIZE,
			"Status %d\n", dad->status);
	if (!dad->status)
		goto error;

	num_read = dad->response_length;

	for (i = 0; i < num_read; i++)
		index += scnprintf(buf + index, PARADE_MAX_PRBUF_SIZE - index,
				"0x%02X\n", dad->response_buf[i]);

	index += scnprintf(buf + index, PARADE_MAX_PRBUF_SIZE - index,
			"(%zd bytes)\n", num_read);

error:
	mutex_unlock(&dad->sysfs_lock);
	return index;
}

static DEVICE_ATTR(response, S_IRUSR, parade_response_show, NULL);

static int parade_ic_parse_input(struct device *dev, const char *buf,
		size_t buf_size, u8 *ic_buf, size_t ic_buf_size)
{
	const char *pbuf = buf;
	unsigned long value;
	char scan_buf[PARADE_INPUT_ELEM_SZ];
	u32 i = 0;
	u32 j;
	int last = 0;
	int ret;

	TS_LOG_DEBUG("%s: pbuf=%p buf=%p size=%zu %s=%zu buf=%s\n", __func__,
			pbuf, buf, buf_size, "scan buf size",
			PARADE_INPUT_ELEM_SZ, buf);

	while (pbuf <= (buf + buf_size)) {
		if (i >= PARADE_MAX_CONFIG_BYTES) {
			TS_LOG_ERR("%s: %s size=%d max=%d\n", __func__,
					"Max cmd size exceeded", i,
					PARADE_MAX_CONFIG_BYTES);
			return -EINVAL;
		}
		if (i >= ic_buf_size) {
			TS_LOG_ERR("%s: %s size=%d buf_size=%zu\n", __func__,
					"Buffer size exceeded", i, ic_buf_size);
			return -EINVAL;
		}
		while (((*pbuf == ' ') || (*pbuf == ','))
				&& (pbuf < (buf + buf_size))) {
			last = *pbuf;
			pbuf++;
		}

		if (pbuf >= (buf + buf_size))
			break;

		memset(scan_buf, 0, PARADE_INPUT_ELEM_SZ);
		if ((last == ',') && (*pbuf == ',')) {
			TS_LOG_ERR("%s: %s \",,\" not allowed.\n", __func__,
					"Invalid data format.");
			return -EINVAL;
		}
		for (j = 0; j < (PARADE_INPUT_ELEM_SZ - 1)
				&& (pbuf < (buf + buf_size))
				&& (*pbuf != ' ')
				&& (*pbuf != ','); j++) {
			last = *pbuf;
			scan_buf[j] = *pbuf++;
		}

		ret = kstrtoul(scan_buf, 16, &value);
		if (ret < 0) {
			TS_LOG_ERR("%s: %s '%s' %s%s i=%d r=%d\n", __func__,
					"Invalid data format. ", scan_buf,
					"Use \"0xHH,...,0xHH\"", " instead.",
					i, ret);
			return ret;
		}

		ic_buf[i] = value;
		i++;
	}

	return i;
}

static ssize_t parade_command_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct parade_core_data *cd = tskit_parade_data;
	struct parade_device_access_data *dad = &tskit_parade_data->dad;
	ssize_t length;
	int rc;

	mutex_lock(&dad->sysfs_lock);
	dad->status = 0;
	dad->response_length = 0;
	length = parade_ic_parse_input(dev, buf, size, dad->ic_buf,
			PARADE_MAX_PRBUF_SIZE);
	if (length <= 0) {
		TS_LOG_ERR("%s: %s Group Data store\n", __func__,
				"Malformed input for");
		goto exit;
	}

	/* write ic_buf to log */
	//cyttsp5_pr_buf(dev, dad->ic_buf, length, "ic_buf");

	//pm_runtime_get_sync(dev);
	rc = parade_hid_output_user_cmd_(cd, PARADE_MAX_PRBUF_SIZE,
			dad->response_buf, length, dad->ic_buf,
			&dad->response_length);
	//pm_runtime_put(dev);
	if (rc) {
		dad->response_length = 0;
		TS_LOG_ERR("%s: Failed to store command\n", __func__);
	} else {
		dad->status = 1;
	}

exit:
	mutex_unlock(&dad->sysfs_lock);
	TS_LOG_INFO("%s: return size=%zu\n", __func__, size);
	return size;
}

static DEVICE_ATTR(command, S_IWUSR, NULL, parade_command_store);

static ssize_t parade_hw_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct parade_core_data *cd = tskit_parade_data;
	int rc;
	rc = parade_startup(cd, true);
	if (rc < 0)
		TS_LOG_ERR("%s: HW reset failed r=%d\n",
			__func__, rc);
	return size;
}

static DEVICE_ATTR(hw_reset, S_IWUSR, NULL, parade_hw_reset_store);

#define PARADE_BOOTLOADER_COMMAND_RESPONSE_LOW_CRC 3
#define PARADE_HID_OUTPUT_VALIDATE_BL_RESPONSE_COUNT 7
static int parade_hid_output_validate_bl_response(
		struct parade_core_data *cd,
		struct parade_hid_output *hid_output)
{
	u16 size = 0;
	u16 crc = 0;
	u8 status = 0;

	size = get_unaligned_le16(&cd->response_buf[0]);

	if (hid_output->reset_expected && !size)
		return 0;

	if(size >= CY_MAX_INPUT || size <= PARADE_HID_OUTPUT_VALIDATE_BL_RESPONSE_COUNT ){
		TS_LOG_ERR("%s: HID output response, wrong size\n",__func__);
		return -EPROTO;
	}
	if (cd->response_buf[HID_OUTPUT_RESPONSE_REPORT_OFFSET]
			!= HID_BL_RESPONSE_REPORT_ID) {
		TS_LOG_ERR("%s: HID output response, wrong report_id\n",
			__func__);
		return -EPROTO;
	}

	if (cd->response_buf[4] != HID_OUTPUT_BL_SOP) {
		TS_LOG_ERR("%s: HID output response, wrong SOP\n",
			__func__);
		return -EPROTO;
	}

	if (cd->response_buf[size - 1] != HID_OUTPUT_BL_EOP) {
		TS_LOG_ERR("%s: HID output response, wrong EOP\n",
			__func__);
		return -EPROTO;
	}

	crc = _parade_compute_crc(&cd->response_buf[4], size - PARADE_HID_OUTPUT_VALIDATE_BL_RESPONSE_COUNT);
	if (cd->response_buf[size - PARADE_BOOTLOADER_COMMAND_RESPONSE_LOW_CRC] != LOW_BYTE(crc)
			|| cd->response_buf[size - 2] != HI_BYTE(crc)) {
		TS_LOG_ERR("%s: HID output response, wrong CRC 0x%X\n",
			__func__, crc);
		return -EPROTO;
	}

	status = cd->response_buf[5];
	if (status) {
		TS_LOG_ERR("%s: HID output response, ERROR:%d\n",
			__func__, status);
		return -EPROTO;
	}

	return 0;
}

static int parade_hid_output_validate_app_response(
		struct parade_core_data *cd,
		struct parade_hid_output *hid_output)
{
	int command_code;
	u16 size;

	size = get_unaligned_le16(&cd->response_buf[0]);

	if (hid_output->reset_expected && !size)
		return 0;

	if (cd->response_buf[HID_OUTPUT_RESPONSE_REPORT_OFFSET]
			!= HID_APP_RESPONSE_REPORT_ID) {
		TS_LOG_ERR("%s: HID output response, wrong report_id:%d\n",
			__func__,cd->response_buf[HID_OUTPUT_RESPONSE_REPORT_OFFSET]);
		return -EPROTO;
	}

	command_code = cd->response_buf[HID_OUTPUT_RESPONSE_CMD_OFFSET]
		& HID_OUTPUT_RESPONSE_CMD_MASK;
	if (command_code != hid_output->command_code) {
		TS_LOG_ERR(
			"%s: HID output response, wrong command_code:%X\n",
			__func__, command_code);
		return -EPROTO;
	}

	return 0;
}

static int parade_hid_output_validate_response(struct parade_core_data *cd,
		struct parade_hid_output *hid_output)
{
	if (hid_output->cmd_type == HID_OUTPUT_CMD_BL)
		return parade_hid_output_validate_bl_response(cd, hid_output);

	return parade_hid_output_validate_app_response(cd, hid_output);

}

static int parade_add_parameter(struct parade_core_data *cd,
		u8 param_id, u32 param_value, u8 param_size)
{
	struct param_node *param, *param_new;

	/* Check if parameter exists */
	spin_lock(&cd->spinlock);
	list_for_each_entry(param, &cd->param_list, node) {
		if (param->id == param_id) {
			/* Update parameter */
			param->value = param_value;
			TS_LOG_DEBUG("%s: Update parameter id:%d value:%d size:%d\n",
				 __func__, param_id, param_value, param_size);
			goto exit_unlock;
		}
	}
	spin_unlock(&cd->spinlock);

	param_new = kzalloc(sizeof(*param_new), GFP_KERNEL);
	if (!param_new)
		return -ENOMEM;

	param_new->id = param_id;
	param_new->value = param_value;
	param_new->size = param_size;

	TS_LOG_DEBUG("%s: Add parameter id:%d value:%d size:%d\n",
		__func__, param_id, param_value, param_size);

	spin_lock(&cd->spinlock);
	list_add(&param_new->node, &cd->param_list);
exit_unlock:
	spin_unlock(&cd->spinlock);

	return 0;
}

static void parade_check_set_parameter(struct parade_core_data *cd,
		struct parade_hid_output *hid_output, bool raw)
{
	u8 *param_buf;
	u32 param_value = 0;
	u8 param_size;
	u8 param_id;
	int i = 0;

	if (!(cd->core_pdata->flags & CY_CORE_FLAG_RESTORE_PARAMETERS))
		return;

	/* Check command input for Set Parameter command */
	if (raw && hid_output->length >= 10 && hid_output->length <= 13
			&& !memcmp(&hid_output->write_buf[0],
					&cd->hid_desc.output_register,
					sizeof(cd->hid_desc.output_register))
			&& hid_output->write_buf[4] ==
					HID_APP_OUTPUT_REPORT_ID
			&& hid_output->write_buf[6] ==
					HID_OUTPUT_SET_PARAM)
		param_buf = &hid_output->write_buf[7];
	else if (!raw && hid_output->cmd_type == HID_OUTPUT_CMD_APP
			&& hid_output->command_code == HID_OUTPUT_SET_PARAM
			&& hid_output->write_length >= 3
			&& hid_output->write_length <= 6)
		param_buf = &hid_output->write_buf[0];
	else
		return;

	/* Get parameter ID, size and value */
	param_id = param_buf[0];
	param_size = param_buf[1];
	if (param_size > 4) {
		TS_LOG_DEBUG("%s: Invalid parameter size\n", __func__);
		return;
	}

	param_buf = &param_buf[2];
	while (i < param_size)
		param_value += *(param_buf++) << (8 * i++);

	/* Check command response for Set Parameter command */
	if (cd->response_buf[2] != HID_APP_RESPONSE_REPORT_ID
			|| (cd->response_buf[4] & HID_OUTPUT_CMD_MASK) !=
				HID_OUTPUT_SET_PARAM
			|| cd->response_buf[5] != param_id
			|| cd->response_buf[6] != param_size) {
		TS_LOG_DEBUG("%s: Set Parameter command not successful\n",
			__func__);
		return;
	}
	parade_add_parameter(cd, param_id, param_value, param_size);
}

static void parade_check_command(struct parade_core_data *cd,
		struct parade_hid_output *hid_output, bool raw)
{
	parade_check_set_parameter(cd, hid_output, raw);
}

static int parade_hid_send_output_and_wait_(struct parade_core_data *cd, struct parade_hid_output *hid_output)
{
	int rc = 0;
	int t;
	int size;
	u16 timeout_ms;
	int retry = 3;

	mutex_lock(&cd->system_lock);
	cd->hid_cmd_state = hid_output->command_code + 1;
	mutex_unlock(&cd->system_lock);

	if (hid_output->timeout_ms)
		timeout_ms = hid_output->timeout_ms;
	else
		timeout_ms = tskit_parade_data->cy_hid_output_timeout;

send_cmd:
	rc = parade_hid_send_output_(cd, hid_output);
	if (rc){
		TS_LOG_ERR("%s parade_hid_send_output_ failed, rc=%d\n", __func__,rc);
		goto error;
	}
	if(cd->force_wait){
		msleep(PARADE_CMD_WAITTIME_LONG);
		if(hid_output->command_code == HID_OUTPUT_GET_SYSINFO)
		parade_read_input(cd);
	}else {
		t = wait_event_timeout(cd->wait_q, (cd->hid_cmd_state == 0),
				msecs_to_jiffies(timeout_ms));
		if (IS_TMO(t)) {
			TS_LOG_ERR("%s: HID output cmd execution timed out\n",
				__func__);
			rc = -ETIME;
			goto error;
		}
	}

	if (!hid_output->novalidate){
		rc = parade_hid_output_validate_response(cd, hid_output);
		if(rc){
			TS_LOG_ERR("%s: validate response error, code:%d\n",
				__func__,rc);
			if(retry--){
				msleep(PARADE_DELAY_20MS);
				goto send_cmd;
			}
		}
	}

	size = get_unaligned_le16(&cd->response_buf[0]);
	cyttsp5_pr_buf(cd->response_buf, size, "return_buf");

	goto exit;

error:
	mutex_lock(&cd->system_lock);
	cd->hid_cmd_state = 0;
	mutex_unlock(&cd->system_lock);
exit:
	return rc;
}

static void parade_si_get_cydata(struct parade_core_data *cd)
{
	struct parade_cydata *cydata = &cd->sysinfo.cydata;
	struct parade_cydata_dev *cydata_dev =
		(struct parade_cydata_dev *)
		&cd->response_buf[HID_SYSINFO_CYDATA_OFFSET];

	cydata->pip_ver_major = cydata_dev->pip_ver_major;
	cydata->pip_ver_minor = cydata_dev->pip_ver_minor;
	cydata->bl_ver_major = cydata_dev->bl_ver_major;
	cydata->bl_ver_minor = cydata_dev->bl_ver_minor;
	cydata->fw_ver_major = cydata_dev->fw_ver_major;
	cydata->fw_ver_minor = cydata_dev->fw_ver_minor;

	cydata->fw_pid = get_unaligned_le16(&cydata_dev->fw_pid);
	cydata->fw_ver_conf = get_unaligned_le16(&cydata_dev->fw_ver_conf);
	cydata->post_code = get_unaligned_le16(&cydata_dev->post_code);
	cydata->revctrl = get_unaligned_le32(&cydata_dev->revctrl);
	cydata->jtag_id_l = get_unaligned_le16(&cydata_dev->jtag_si_id_l);
	cydata->jtag_id_h = get_unaligned_le16(&cydata_dev->jtag_si_id_h);

	memcpy(cydata->mfg_id, cydata_dev->mfg_id, CY_NUM_MFGID);

	memset(tskit_parade_data->parade_chip_data->version_name, 0, MAX_STR_LEN);
	snprintf(tskit_parade_data->parade_chip_data->version_name, MAX_STR_LEN -1, "0x%04X",
					cydata->fw_ver_conf);

}

static void parade_si_get_sensing_conf_data(struct parade_core_data *cd)
{
	struct parade_sensing_conf_data *scd = &cd->sysinfo.sensing_conf_data;
	struct parade_sensing_conf_data_dev *scd_dev =
		(struct parade_sensing_conf_data_dev *)
		&cd->response_buf[HID_SYSINFO_SENSING_OFFSET];

	scd->electrodes_x = scd_dev->electrodes_x;
	scd->electrodes_y = scd_dev->electrodes_y;
	scd->origin_x = scd_dev->origin_x;
	scd->origin_y = scd_dev->origin_y;

	/* PIP 1.4 (001-82649 *Q) add X_IS_TX bit in X_ORG */
	if (scd->origin_x & 0x02) {
		scd->tx_num = scd->electrodes_x;
		scd->rx_num = scd->electrodes_y;
	} else {
		scd->tx_num = scd->electrodes_y;
		scd->rx_num = scd->electrodes_x;
	}

	scd->panel_id = scd_dev->panel_id;
	tskit_parade_data->panel_id = scd->panel_id;
	scd->btn = scd_dev->btn;
	scd->scan_mode = scd_dev->scan_mode;
	scd->max_tch = scd_dev->max_num_of_tch_per_refresh_cycle;

	scd->res_x = get_unaligned_le16(&scd_dev->res_x);
	scd->res_y = get_unaligned_le16(&scd_dev->res_y);
	scd->max_z = get_unaligned_le16(&scd_dev->max_z);
	scd->len_x = get_unaligned_le16(&scd_dev->len_x);
	scd->len_y = get_unaligned_le16(&scd_dev->len_y);


}

static int parade_si_setup(struct parade_core_data *cd)
{
	struct parade_sysinfo *si = &cd->sysinfo;
	unsigned int max_tch = si->sensing_conf_data.max_tch;

	if (!si->xy_data)
		si->xy_data = kzalloc(max_tch * si->desc.tch_record_size,
				GFP_KERNEL);
	if (!si->xy_data)
		return -ENOMEM;

	if (!si->xy_mode)
		si->xy_mode = kzalloc(si->desc.tch_header_size, GFP_KERNEL);
	if (!si->xy_mode) {
		kfree(si->xy_data);
		return -ENOMEM;
	}

	return 0;
}

static int parade_si_get_btn_data(struct parade_core_data *cd)
{
	struct parade_sysinfo *si = &cd->sysinfo;
	int num_btns = 0;
	int num_defined_keys;
	u16 *key_table;
	int btn;
	int i;
	int rc = 0;
	unsigned int btns = cd->response_buf[HID_SYSINFO_BTN_OFFSET]
		& HID_SYSINFO_BTN_MASK;
	size_t btn_keys_size;

	TS_LOG_DEBUG("%s: get btn data\n", __func__);

	for (i = 0; i < HID_SYSINFO_MAX_BTN; i++) {
		if (btns & (1 << i))
			num_btns++;
	}
	si->num_btns = num_btns;

	if (num_btns) {
		btn_keys_size = num_btns * sizeof(struct parade_btn);
		if (!si->btn)
			si->btn = kzalloc(btn_keys_size, GFP_KERNEL);
		if (!si->btn)
			return -ENOMEM;

		if (cd->core_pdata->sett[CY_IC_GRPNUM_BTN_KEYS] == NULL)
			num_defined_keys = 0;
		else if (cd->core_pdata->sett[CY_IC_GRPNUM_BTN_KEYS]->data == NULL)
			num_defined_keys = 0;
		else
			num_defined_keys = cd->core_pdata->sett
				[CY_IC_GRPNUM_BTN_KEYS]->size;

		for (btn = 0; btn < num_btns && btn < num_defined_keys; btn++) {
			key_table = (u16 *)cd->core_pdata->sett
				[CY_IC_GRPNUM_BTN_KEYS]->data;
			si->btn[btn].key_code = key_table[btn];
			si->btn[btn].enabled = true;
		}
		for (; btn < num_btns; btn++) {
			si->btn[btn].key_code = KEY_RESERVED;
			si->btn[btn].enabled = true;
		}

		return rc;
	}

	kfree(si->btn);
	si->btn = NULL;
	return rc;
}

static void parade_si_put_log_data(struct parade_core_data *cd)
{
	struct parade_sysinfo *si = &cd->sysinfo;
	struct parade_cydata *cydata = &si->cydata;
	struct parade_sensing_conf_data *scd = &si->sensing_conf_data;
	int i;

	TS_LOG_DEBUG("%s: pip_ver_major =0x%02X (%d)\n", __func__,
		cydata->pip_ver_major, cydata->pip_ver_major);
	TS_LOG_DEBUG("%s: pip_ver_minor =0x%02X (%d)\n", __func__,
		cydata->pip_ver_minor, cydata->pip_ver_minor);
	TS_LOG_DEBUG("%s: fw_pid =0x%04X (%d)\n", __func__,
		cydata->fw_pid, cydata->fw_pid);
	TS_LOG_DEBUG("%s: fw_ver_major =0x%02X (%d)\n", __func__,
		cydata->fw_ver_major, cydata->fw_ver_major);
	TS_LOG_DEBUG("%s: fw_ver_minor =0x%02X (%d)\n", __func__,
		cydata->fw_ver_minor, cydata->fw_ver_minor);
	TS_LOG_DEBUG("%s: revctrl =0x%08X (%d)\n", __func__,
		cydata->revctrl, cydata->revctrl);
	TS_LOG_DEBUG("%s: fw_ver_conf =0x%04X (%d)\n", __func__,
		cydata->fw_ver_conf, cydata->fw_ver_conf);
	TS_LOG_DEBUG("%s: bl_ver_major =0x%02X (%d)\n", __func__,
		cydata->bl_ver_major, cydata->bl_ver_major);
	TS_LOG_DEBUG("%s: bl_ver_minor =0x%02X (%d)\n", __func__,
		cydata->bl_ver_minor, cydata->bl_ver_minor);
	TS_LOG_DEBUG("%s: jtag_id_h =0x%04X (%d)\n", __func__,
		cydata->jtag_id_h, cydata->jtag_id_h);
	TS_LOG_DEBUG("%s: jtag_id_l =0x%04X (%d)\n", __func__,
		cydata->jtag_id_l, cydata->jtag_id_l);
	for (i = 0; i < CY_NUM_MFGID; i++)
		TS_LOG_DEBUG("%s: mfg_id[%d] =0x%02X (%d)\n", __func__, i,
			cydata->mfg_id[i], cydata->mfg_id[i]);
	TS_LOG_DEBUG("%s: post_code =0x%04X (%d)\n", __func__,
		cydata->post_code, cydata->post_code);

	TS_LOG_DEBUG("%s: electrodes_x =0x%02X (%d)\n", __func__,
		scd->electrodes_x, scd->electrodes_x);
	TS_LOG_DEBUG("%s: electrodes_y =0x%02X (%d)\n", __func__,
		scd->electrodes_y, scd->electrodes_y);
	TS_LOG_DEBUG("%s: len_x =0x%04X (%d)\n", __func__,
		scd->len_x, scd->len_x);
	TS_LOG_DEBUG("%s: len_y =0x%04X (%d)\n", __func__,
		scd->len_y, scd->len_y);
	TS_LOG_DEBUG("%s: res_x =0x%04X (%d)\n", __func__,
		scd->res_x, scd->res_x);
	TS_LOG_DEBUG("%s: res_y =0x%04X (%d)\n", __func__,
		scd->res_y, scd->res_y);
	TS_LOG_DEBUG("%s: max_z =0x%04X (%d)\n", __func__,
		scd->max_z, scd->max_z);
	TS_LOG_DEBUG("%s: origin_x =0x%02X (%d)\n", __func__,
		scd->origin_x, scd->origin_x);
	TS_LOG_DEBUG("%s: origin_y =0x%02X (%d)\n", __func__,
		scd->origin_y, scd->origin_y);
	TS_LOG_DEBUG("%s: panel_id =0x%02X (%d)\n", __func__,
		scd->panel_id, scd->panel_id);
	TS_LOG_DEBUG("%s: btn =0x%02X (%d)\n", __func__,
		scd->btn, scd->btn);
	TS_LOG_DEBUG("%s: scan_mode =0x%02X (%d)\n", __func__,
		scd->scan_mode, scd->scan_mode);
	TS_LOG_DEBUG("%s: max_num_of_tch_per_refresh_cycle =0x%02X (%d)\n",
		__func__, scd->max_tch, scd->max_tch);

	TS_LOG_DEBUG("%s: xy_mode =%p\n", __func__,
		si->xy_mode);
	TS_LOG_DEBUG("%s: xy_data =%p\n", __func__,
		si->xy_data);
}

static int parade_get_sysinfo_regs(struct parade_core_data *cd)
{
	struct parade_sysinfo *si = &cd->sysinfo;
	int rc;
	TS_LOG_INFO("%s parade_si_get_btn_data", __func__);
	rc = parade_si_get_btn_data(cd);
	if (rc < 0)
		return rc;
	TS_LOG_INFO("%s parade_si_get_cydata", __func__);
	parade_si_get_cydata(cd);

	TS_LOG_INFO("%s parade_si_get_sensing_conf_data", __func__);
	parade_si_get_sensing_conf_data(cd);

	TS_LOG_INFO("%s parade_si_setup", __func__);
	parade_si_setup(cd);

	TS_LOG_INFO("%s parade_si_put_log_data", __func__);
	parade_si_put_log_data(cd);

	si->ready = true;
	return rc;
}

static void parade_free_si_ptrs(struct parade_core_data *cd)
{
	struct parade_sysinfo *si = &cd->sysinfo;

	kfree(si->btn);
	kfree(si->xy_mode);
	kfree(si->xy_data);
}
static int parade_before_suspend(void)
{
	int rc = 0;
	struct parade_core_data *cd = tskit_parade_data;
	int sleep_in_mode = cd->sleep_in_mode;
	TS_LOG_INFO("%s Enter", __func__);
	if(CY_SHOW_BEFORE_SUSPEND == tskit_parade_data->show_before_suspend_flag) {
		tskit_parade_data->avoid_roi_switch_flag = CY_AVOID_ROI_SWITCH;
	}
	if(g_ts_kit_platform_data.chip_data->isbootupdate_finish==false
		&& CY_CMD_SLEEP_MODE != sleep_in_mode) {
		enable_irq(tskit_parade_data->parade_chip_data->ts_platform_data->irq_id);
	}
	if((1 == sleep_in_mode) && (cd->parade_chip_data->easy_wakeup_info.sleep_mode == TS_POWER_OFF_MODE)) { //Power down
		parade_is_power_on = false;
	}
	return rc;
}

static int parade_core_poweroff_device_(struct parade_core_data *cd)
{
	int rc = 0;
	TS_LOG_ERR("%s: Not Implemented Yet\n", __func__);
	if (rc < 0)
		TS_LOG_ERR("%s: HW Power down fails r=%d\n", __func__, rc);
	return rc;
}

static int parade_core_poweron_device_(struct parade_core_data *cd)
{
	int rc = 0;
	TS_LOG_ERR("%s: Not Implemented Yet\n", __func__);
	return rc;

}

static int parade_hid_cmd_set_power_(struct parade_core_data *cd,
				      u8 power_state)
{
	int rc;
	u8 cmd_set_power[] = {0x05,0x00,0x00,0x08};
	struct parade_hid_output hid_cmd_set_power;
	cmd_set_power[2] = power_state;
	hid_cmd_set_power.length = 4;
	hid_cmd_set_power.write_buf = cmd_set_power;
	hid_cmd_set_power.timeout_ms = CY_HID_SET_POWER_TIMEOUT;
	rc = parade_hid_send_output_user_and_wait_(cd,&hid_cmd_set_power);
	if (rc) {
		TS_LOG_ERR("%s: Failed to set power to state:%d\n",
			   __func__, power_state);
		return rc;
	}

	/* validate */
	if ((cd->response_buf[2] != HID_RESPONSE_REPORT_ID)
	    || ((cd->response_buf[3] & 0x3) != power_state)
	    || ((cd->response_buf[4] & 0xF) != HID_CMD_SET_POWER))
		rc = -EINVAL;

	return rc;
}

static int parade_put_device_into_deep_sleep_(struct parade_core_data *cd)
{
	int rc;

	rc = parade_hid_cmd_set_power_(cd, HID_POWER_SLEEP);
	if (rc)
		rc = -EBUSY;
	return rc;
}
static int parade_core_wake_device_from_deep_sleep_(struct parade_core_data *cd)
{
	int rc = -EAGAIN;
	int retry_times = 0;

	for (retry_times = 0; retry_times < CY_WAKEUP_RETRY_TIMES;
	     retry_times++) {
		rc = parade_hid_cmd_set_power_(cd, HID_POWER_ON);
		if (!rc) {
			TS_LOG_INFO("%s %d:set power to wake up success\n",
				    __func__, __LINE__);
			break;
		}
		rc = -EAGAIN;
	}
	/* Prevent failure on sequential wake/sleep requests from OS */
	if(rc < 0) {
#if defined (CONFIG_HUAWEI_DSM)
		ts_dmd_report(DSM_TP_WAKEUP_ERROR_NO, "wakeup parade ts failed.\nfailed status is %d.\n", TS_WAKEUP_FAIL);
#endif
	}
	msleep(20);

	return rc;
}
static int parade_wakeup_gesture_switch(struct ts_wakeup_gesture_enable_info* info)
{
	TS_LOG_INFO("%s:done\n", __func__);
	return NO_ERR;
}

static int parade_chip_wrong_touch(void)
{
	int rc = NO_ERR;
	mutex_lock(&tskit_parade_data->system_lock);
	tskit_parade_data->parade_chip_data->easy_wakeup_info.off_motion_on = true;
	mutex_unlock(&tskit_parade_data->system_lock);
	TS_LOG_INFO("%s:done\n", __func__);
	return rc;
}

#define PARADE_ENTER_EASY_WAKEUP_WRITE_LENS 1
static void parade_enter_easy_wakeup(struct work_struct *work)
{
	struct parade_core_data* cd = tskit_parade_data;
	int rc;
	u8 write_buf[PARADE_ENTER_EASY_WAKEUP_WRITE_LENS] = { 0x00 };
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_ENTER_EASYWAKE_STATE),
		.write_length = PARADE_ENTER_EASY_WAKEUP_WRITE_LENS,
		.write_buf = write_buf,
	};

	ts_stop_wd_timer(&g_ts_kit_platform_data);
	
	rc = parade_check_cmd_status();
	if(rc)
		return;

	if(false == g_ts_kit_platform_data.chip_data->isbootupdate_finish) {
		TS_LOG_INFO("%s MMI/fwupgrade is running, no need easywakeup.\n",__func__);
		parade_finish_cmd();
		return;
	}

	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	if (rc || cd->response_buf[CY_APP_RETURN_RESPONSE_STATUS_BIT] == 0){
		TS_LOG_ERR("%s: Get in easy wakeup mode fail, rc = %d, status = %u",
			__func__, rc, cd->response_buf[CY_APP_RETURN_RESPONSE_STATUS_BIT]);
		parade_finish_cmd();
		return;
	}

	if (device_may_wakeup(cd->dev)) {
		rc = enable_irq_wake(g_ts_kit_platform_data.irq_id);
		if (!rc) {
			TS_LOG_INFO("%s: Device may wakeup\n", __func__);
		} else {
			TS_LOG_ERR("%s: enable irq wake fail, rc = %d\n",
				   __func__, rc);
		}
	} else {
		TS_LOG_INFO("%s: Device may not wakeup\n", __func__);
	}

	parade_finish_cmd();
}

static void parade_deep_sleep_work_fn(struct work_struct *work)
{
	struct parade_core_data* cd = tskit_parade_data;
	int rc = NO_ERR;

	if(false == g_ts_kit_platform_data.chip_data->isbootupdate_finish) {
		TS_LOG_INFO("%s MMI/fwupgrade is running, no need suspend/resume.\n",__func__);
		return;
	}
	if(sleep_resume_flag == POWER_DEEP_SLEEP) {
		ts_stop_wd_timer(&g_ts_kit_platform_data);
		rc = parade_check_cmd_status();
		if(rc)
			return;
		parade_put_device_into_deep_sleep_(cd);
		parade_finish_cmd();
		disable_irq(g_ts_kit_platform_data.irq_id);
	}
	else {
		if(cd->parade_chip_data->easy_wakeup_info.sleep_mode != TS_GESTURE_MODE)
			enable_irq(g_ts_kit_platform_data.irq_id);
		rc = parade_check_cmd_status();
		if(rc)
			return;
		parade_core_wake_device_from_deep_sleep_(cd);
		parade_finish_cmd();
		ts_start_wd_timer(&g_ts_kit_platform_data);
		if(CY_SHOW_BEFORE_SUSPEND == tskit_parade_data->show_before_suspend_flag) {
			tskit_parade_data->avoid_roi_switch_flag = 0;
		}
	}

}

static int parade_core_suspend(void)
{
	int rc = 0;
	struct parade_core_data* cd = tskit_parade_data;
	int sleep_in_mode = cd->sleep_in_mode;

	TS_LOG_INFO("%s enter\n", __func__);
	switch (cd->parade_chip_data->easy_wakeup_info.sleep_mode) {
		case TS_POWER_OFF_MODE:
			if(0 == sleep_in_mode){ //Deep Sleep
				TS_LOG_INFO("%s: put device into sleep\n", __func__);
				sleep_resume_flag = POWER_DEEP_SLEEP;
				schedule_work(&parade_deep_sleep_work);
			} else if(1 == sleep_in_mode){ //Power Down
				TS_LOG_INFO("%s: power off device, current control by LCD\n", __func__);
				if(g_ts_kit_platform_data.chip_data->isbootupdate_finish) {
					/*Disable IRQ to avoid the glitch changes of LCD*/
					disable_irq(tskit_parade_data->parade_chip_data->ts_platform_data->irq_id);
				}
			}
			break;
		case TS_GESTURE_MODE:/*gesture easy_wake_up mode. */
			if(0 == cd->core_pdata->wakeup_gesture_enable){
				TS_LOG_ERR("%s: disable easy_wakeup, enter deep sleep!!\n", __func__);
				sleep_resume_flag = POWER_DEEP_SLEEP;
				schedule_work(&parade_deep_sleep_work);
			}else{
				TS_LOG_INFO("%s: enter easy wakeup mode.\n", __func__);
				mutex_lock(&cd->system_lock);
				tskit_parade_data->parade_chip_data->easy_wakeup_info.off_motion_on = true;
				mutex_unlock(&cd->system_lock);
				schedule_work(&parade_easy_wakeup_work);
			}
			break;
		default:
			TS_LOG_ERR("%s error sleep in mode\n", __func__);
			return -EINVAL;
	}
	if((0 != sleep_in_mode) && (g_ts_kit_platform_data.chip_data->isbootupdate_finish) && (cd->parade_chip_data->easy_wakeup_info.sleep_mode == TS_POWER_OFF_MODE)) {
		parade_power_off_gpio_set();
		parade_pinctrl_select_lowpower();
	}
	return rc;
}

static int parade_core_resume(void)
{
	int rc = 0;
	struct parade_core_data* cd = tskit_parade_data;
	int sleep_in_mode = cd->sleep_in_mode;
	TS_LOG_INFO("%s enter\n", __func__);
	parade_is_power_on = true;
	if((0 != sleep_in_mode) && (cd->parade_chip_data->easy_wakeup_info.sleep_mode == TS_POWER_OFF_MODE)) {
		parade_power_on_gpio_set();
		parade_pinctrl_select_normal();
	}
	switch (cd->parade_chip_data->easy_wakeup_info.sleep_mode) {
		case TS_POWER_OFF_MODE:
			if(0 == sleep_in_mode){ //Deep Sleep
				TS_LOG_INFO("%s: wake up device by command\n", __func__);
				sleep_resume_flag = POWER_WAKEUP;
				schedule_work(&parade_deep_sleep_work);
			}else if(1 == sleep_in_mode){ //Power Down
				TS_LOG_INFO("%s: power on device by LCD, start i2c seq\n", __func__);
				parade_hw_reset();
				rc = parade_check_and_deassert_int(cd);
				msleep(PARADE_DELAY_20MS);
				enable_irq(tskit_parade_data->parade_chip_data->ts_platform_data->irq_id);
			}
			break;
		case TS_GESTURE_MODE:/*gesture easy_wake_up mode. */
			TS_LOG_INFO("%s: exit easy wakeup mode.\n", __func__);
			sleep_resume_flag = POWER_WAKEUP;
			schedule_work(&parade_deep_sleep_work);
			break;
		default:
			TS_LOG_ERR("%s error sleep in mode\n", __func__);
			return -EINVAL;
	}
	return rc;

}
/*  do some things after power on. */
static int parade_after_resume(void *feature_info)
{
	int retval = NO_ERR;
	struct parade_sysinfo *si = &tskit_parade_data->sysinfo;
	TS_LOG_INFO("%s Enter\n", __func__);
	if(PARADE_NO_NEED_DO_AFTER_RESUME == tskit_parade_data->no_need_after_resume_flag) {
		TS_LOG_INFO("%s No need doing parade_after_resume\n", __func__);
		return retval;
	}
	if(si->ready){
		tskit_parade_data->is_need_check_mode_before_status_resume = false;
		schedule_delayed_work(&parade_status_resume_work,msecs_to_jiffies(PARADE_AFTER_RESUME_TIMEOUT));
	}
	else
		TS_LOG_ERR("%s sysinfo is not ready, may not success start up",__func__);
	return retval;
}

static void parade_status_resume_work_fn(struct work_struct *work)
{
	int retval = NO_ERR;
	int mode = CY_MODE_UNKNOWN;
	struct parade_core_data *cd = tskit_parade_data;
	struct ts_feature_info *info = &tskit_parade_data->parade_chip_data->ts_platform_data->feature_info;
	TS_LOG_INFO("%s Enter\n", __func__);
	mutex_lock(&tskit_parade_data->parade_chip_data->device_call_lock);
	if(tskit_parade_data->is_need_check_mode_before_status_resume == true){
		TS_LOG_INFO("%s: Reading hid descriptor\n", __func__);
		retval = parade_get_hid_descriptor_(cd, &cd->hid_desc);
		if (retval < 0) {
			TS_LOG_INFO("%s: Error on getting HID descriptor r=%d\n",
				__func__, retval);
			goto out;
		}
		/*Start to get mode*/
		TS_LOG_INFO("%s: start to get mode.\n", __func__);
		mode = parade_get_mode(cd, &cd->hid_desc);
		if(mode == CY_MODE_BOOTLOADER){
			TS_LOG_INFO("%s: bootloader mode, no work to do\n", __func__);
			goto out;
		}
	}
#ifdef PARADE_ROI_ENABLE
	if (info->roi_info.roi_supported) {
		struct ts_roi_info roi_info;
		roi_info.op_action = TS_ACTION_WRITE;
		roi_info.roi_switch = info->roi_info.roi_switch;
		retval = parade_roi_switch(&roi_info);
		if(retval != NO_ERR){
			TS_LOG_ERR("%s Set ROI Swith Fail, status: %d\n", __func__ , retval);
		}
		TS_LOG_INFO("%s Set roi switch to %d [OK]\n", __func__ , info->roi_info.roi_switch);
	}
#endif
	/*Glove Mode*/
	if(info->glove_info.glove_supported) {
		retval = parade_set_glove_switch(info->glove_info.glove_switch);
		if(retval != NO_ERR){
			TS_LOG_ERR("%s Set glove switch Fail, status: %d\n", __func__ , retval);
		}
		TS_LOG_INFO("%s Set glove switch to %d [OK]\n", __func__ , info->glove_info.glove_switch);
	}
	/*Holster Switch*/
	if(info->holster_info.holster_supported) {
		retval = parade_set_holster_switch(info->holster_info.holster_switch);
		if(retval != NO_ERR){
			TS_LOG_ERR("%s Set holster switch Fail, status: %d\n", __func__ , retval);
		}
		TS_LOG_INFO("%s Set holster switch to %d [OK]\n", __func__ , info->holster_info.holster_switch);
	}
out:
	mutex_unlock(&tskit_parade_data->parade_chip_data->device_call_lock);
	return retval;
}

static void parade_shutdown(void)
{
	TS_LOG_INFO("%s parade_shutdown call power off\n",__func__);
	parade_power_off();
	parade_gpio_free();
	return;
}

/*
* this function is only suitable for those
* parameter IDs that only returns one byte
*/
static int parade_get_ram_parameter(struct parade_core_data *cd, u8 id, int* parameter_out,int timeout)
{
	int rc = NO_ERR;
	int write_length = 1;
	int rcsize = 0;
	u8 param[1];
	u8 ret;
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_GET_PARAM),
		.write_length = write_length,
		.write_buf = param,
		.timeout_ms=timeout,
	};
	param[0] = id;
	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	if(rc){
		TS_LOG_ERR("%s HID_OUTPUT_GET_PARAM command failed\n", __func__);
		return rc;
	}
	rcsize = cd->response_buf[6];
	switch(rcsize){
		case 1:
			*parameter_out = cd->response_buf[7];
			break;
		case 2:
			*parameter_out = cd->response_buf[7] + ((int)(cd->response_buf[8]))<<8;
			break;
		case 4:
			*parameter_out = cd->response_buf[7] + ((int)(cd->response_buf[8]))<<8
				+ ((int)(cd->response_buf[9]))<<16 + ((int)(cd->response_buf[10]))<<24;
			break;
		default:
			TS_LOG_ERR("%s No such expected return\n", __func__);
			rc = -EINVAL;
			break;
	}
	TS_LOG_INFO("%s parameter out = %d\n", __func__, *parameter_out);
	return rc;
}

/*
* this function is only suitable for those
* parameter IDs that needs one byte data
*/
static int parade_set_ram_parameter(struct parade_core_data *cd, u8 id, u8 data, int timeout)
{
	int rc = NO_ERR;
	u8 write_buf[3];
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_SET_PARAM),
		.write_buf = write_buf,
		.write_length = 3,
		.timeout_ms = timeout,
	};
	//ONLY SUPPORT SINGLE PARAMETER(1 BYTE) ONLY
	write_buf[0] = id;
	write_buf[1] = 1;//length
	write_buf[2] = data;

	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	if (rc)
		TS_LOG_ERR("%s HID_OUTPUT_SET_PARAM command failed\n", __func__);

	return rc;
}

static int parade_set_power_on_flag(struct parade_core_data *cd)
{
	int rc = NO_ERR;

	TS_LOG_INFO("%s Enter\n",__func__);
	rc = parade_set_ram_parameter(cd,0x21,0x01,CY_HID_OUTPUT_SET_GET_POWER_ON_FLAG);
	if (rc)
		TS_LOG_ERR("%s set power on flag command failed\n", __func__);

	TS_LOG_INFO("%s Exit\n",__func__);
	return rc;
}
static int parade_get_power_on_flag(struct parade_core_data *cd)
{
	int rc = NO_ERR;
	int param_out = 0;

	TS_LOG_INFO("%s Enter\n",__func__);
	rc = parade_get_ram_parameter(cd,0x21,&param_out,CY_HID_OUTPUT_SET_GET_POWER_ON_FLAG);
	if (rc)
		TS_LOG_ERR("%s get power on flag command failed\n", __func__);
	else
		TS_LOG_INFO("%s get power on flag value = %d\n",__func__,param_out);

	return rc;
}

static int parade_set_glove_switch(u8 glove_switch)
{
	int rc = NO_ERR;
	u8 data;

	if(glove_switch > 0)
		data= FINGER_GLOVE_MODE;
	else
		data = FINGER_ONLY_MODE;
	rc = parade_set_ram_parameter(tskit_parade_data, TOUCH_MODE_RAM_ID , data,CY_HID_OUTPUT_SET_RAM_NORMAL_PARAM);
	return rc;
}

static int parade_get_glove_switch(u8 *glove_switch)
{
	int rc = NO_ERR;
	int touch_mode_enable = 0;
	if (!glove_switch) {
		TS_LOG_ERR("parade_get_glove_switch: glove_switch is Null\n");
		return -ENOMEM;
	}
	rc = parade_get_ram_parameter(tskit_parade_data, TOUCH_MODE_RAM_ID, &touch_mode_enable,CY_HID_OUTPUT_SET_RAM_NORMAL_PARAM);
	if(rc != NO_ERR){
		TS_LOG_ERR("%s, Fail to get glove switch,rc = %d\n", __func__,rc);
		goto out;
	}
	TS_LOG_INFO("touch_mode_enable = %d\n", touch_mode_enable);

	if (touch_mode_enable == FINGER_GLOVE_MODE) {
		*glove_switch = GLOVE_SWITCH_ON;
	} else {
		*glove_switch = GLOVE_SWITCH_OFF;
	}
	TS_LOG_DEBUG("parade_get_glove_switch done : %d\n", *glove_switch);
out:
	return rc;
}

static int parade_glove_switch(struct ts_glove_info *info)
{
	int retval = NO_ERR;
	u8 buf = 0;

	if(info->glove_supported == 0) //not support
		return retval;

	retval = parade_check_cmd_status();
	if(retval)
		return retval;

	switch (info->op_action) {
	case TS_ACTION_READ:
		retval = parade_get_glove_switch(&buf);
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
		retval = parade_set_glove_switch(buf);
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

	if (retval)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;
	parade_finish_cmd();
	return retval;
}

static int parade_set_holster_switch(u8 in_switch)
{
	int rc = NO_ERR;
	u8 data;

	if(in_switch > 0)
		data= HOLSTER_ON;
	else
		data = HOLSTER_OFF;
	rc = parade_set_ram_parameter(tskit_parade_data, HOLSTER_RAME_ID , data,CY_HID_OUTPUT_SET_RAM_NORMAL_PARAM);
	return rc;
}

static int parade_get_holster_switch(u8 *out_switch)
{
	int rc = NO_ERR;
	int hoster_mode = 0;
	if (!out_switch) {
		TS_LOG_ERR("parade_get_holster_switch: holster is Null\n");
		return -ENOMEM;
	}
	rc = parade_get_ram_parameter(tskit_parade_data, HOLSTER_RAME_ID, &hoster_mode,CY_HID_OUTPUT_SET_RAM_NORMAL_PARAM);
	if(rc != NO_ERR){
		TS_LOG_ERR("%s, Fail to get holster switch,rc = %d\n", __func__,rc);
		goto out;
	}
	*out_switch = hoster_mode;
	TS_LOG_INFO("parade_get_holster_switch done : %d\n", *out_switch);
out:
	return rc;
}

static int parade_holster_switch(struct ts_holster_info *info)
{
	int retval = NO_ERR;
	u8 uTmp = 0;

	if(info->holster_supported == 0) // not support
		return retval;

	retval = parade_check_cmd_status();
	if(retval)
		return retval;

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		retval = parade_set_holster_switch(info->holster_switch);
		if (retval < 0) {
			TS_LOG_ERR("%s, set holster switch(%d), failed: %d\n",
				   __func__,info->holster_switch, retval);
		}
		break;
	case TS_ACTION_READ:
		retval = parade_get_holster_switch(&uTmp);
		if (retval < 0){
			TS_LOG_ERR("%s, get holster switch failed: %d\n",
				   __func__,retval);
		}
		info->holster_switch = uTmp;
		TS_LOG_INFO("%s, get holster switch: %d\n", __func__, info->holster_switch);
		break;
	default:
		TS_LOG_INFO("invalid holster switch(%d) action: %d\n",
			    info->holster_switch, info->op_action);
		retval = -EINVAL;
		break;
	}

	if (retval)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;
	parade_finish_cmd();
	return retval;
}

/*
* this function is not completed, need to first understand Huawei's
* output format. According to 001-88195 Table 6-58, what kind of values should be returned back?
00h - IDAC and RX Attenuator Calibration Data (center frequency)

04h / 05h / 07h / 09h - IDAC and RX Attenuator
Calibration Data for alternative frequencies
1 / 2 / 3 / 5 respectively. AFH must be enabled

10h - Mutual Gain Correction
11h - Self Gain Correction
12h - Attenuator Trim
13h - Pin index table
*/
#define PARADE_RETRIEVE_DATA_STRUCTURE_RESPONSE_DATA 10
static int parade_get_calibration_data(struct ts_calibration_data_info *info, struct ts_cmd_node *out_cmd)
{
	int rc = NO_ERR;
	u8 write_buf[5];
	u8 data_id = 0;
	u16 read_offset = 0;
	u16 read_length = 100;
	u8 status = 0;
	u16 actual_read_len = 0;

	rc = parade_check_cmd_status();

	if(rc)
		return rc;

	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_GET_DATA_STRUCTURE),
		.write_length = 5,
		.write_buf = write_buf,
	};
	TS_LOG_INFO("%s called\n", __func__);
 	write_buf[0] = LOW_BYTE(read_offset);
	write_buf[1] = HI_BYTE(read_offset);
	write_buf[2] = LOW_BYTE(read_length);
	write_buf[3] = HI_BYTE(read_length);
	write_buf[4] = data_id;

	/*do suspend before getting IDAC*/
	rc = parade_hid_output_suspend_scanning_(tskit_parade_data);
	if(rc) {
		TS_LOG_ERR("%s parade_suspend failed\n", __func__);
		goto exit;
	}

	rc = parade_hid_send_output_and_wait_(tskit_parade_data, &hid_output);
	if(rc) {
		TS_LOG_ERR("%s parade_hid_send_output_and_wait_  for Mutual Cap failed\n", __func__);
		goto exit;
	}
	status = tskit_parade_data->response_buf[5];
	if(status) {
		TS_LOG_ERR("%s parade_hid_send_output_and_wait_ failed\n", __func__);
		goto exit;
	}

	actual_read_len = tskit_parade_data->response_buf[7] + 256 * tskit_parade_data->response_buf[8];
	if((actual_read_len + PARADE_RETRIEVE_DATA_STRUCTURE_RESPONSE_DATA >= CY_MAX_INPUT) || actual_read_len >= CALIBRATION_DATA_SIZE){
		TS_LOG_ERR("%s invalid index[ %d ].\n", __func__, actual_read_len);
		goto exit;
	}
	memcpy(info->data, &tskit_parade_data->response_buf[10], actual_read_len);
	info->used_size = actual_read_len;
	info->tx_num = tskit_parade_data->sysinfo.sensing_conf_data.electrodes_x;
	info->rx_num = tskit_parade_data->sysinfo.sensing_conf_data.electrodes_y;
	TS_LOG_INFO("info->used_size = %d\n", info->used_size);
exit:
	rc = parade_hid_output_resume_scanning_(tskit_parade_data);
	if(rc)
		TS_LOG_ERR("%s parade scan resume fail\n", __func__);

	if (rc)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;
	parade_finish_cmd();
	return rc;
}

static unsigned char * parade_roi_rawdata (void)
{
	TS_LOG_INFO("%s enter", __func__);
#ifdef PARADE_ROI_ENABLE
	cyttsp5_pr_buf(roi_data, PARADE_ROI_DATA_READ_LENGTH, "roi_data");
	cyttsp5_pr_buf(tskit_parade_data->knuckle_data, PARADE_ROI_DATA_SEND_LENGTH, "knucle_data");

	return (unsigned char *)roi_data;
#else
	return NULL;
#endif
}

static int parade_start_knucle_mode(struct parade_core_data *cd)
{
	int rc = NO_ERR;
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_START_KNUCKLE_MODE),
	};
	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	return rc;
}

static int parade_stop_knuckle_mode(struct parade_core_data *cd)
{
	int rc = NO_ERR;
	/*
	* according 001-88195, HID_OUTPUT_STOP_SENSOR_DATA_MODE
	* is the command that can stop heatmap tracking mode
	*/
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_STOP_SENSOR_DATA_MODE),
	};
	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	return rc;
}

static int parade_set_roi_switch(u8 roi_switch)
{
	int rc = NO_ERR;
	TS_LOG_INFO("%s enter, roi_switch = %d", __func__, roi_switch);
#ifdef PARADE_ROI_ENABLE
	rc = parade_hid_output_suspend_scanning_(tskit_parade_data);
	if(rc){
		TS_LOG_ERR("%s suspend failed\n", __func__);
		goto exit;
	}
	if(roi_switch == 0) {
		/*in case of turn off the heatmap tracking mode*/
		rc = parade_stop_knuckle_mode(tskit_parade_data);
		if(!rc)
			tskit_parade_data->roi_switch = 0;
		else
			TS_LOG_ERR("%s stop roi report mode failed\n", __func__);
		TS_LOG_INFO("%s turn off roi report mode,rc=%d\n", __func__, rc);
	} else {
		/*in case of turn on the heatmap tracking mode*/
		rc = parade_start_knucle_mode(tskit_parade_data);
		if(!rc)
			tskit_parade_data->roi_switch = 1;
		else
			TS_LOG_ERR("%s start roi report mode failed\n", __func__);
		TS_LOG_INFO("%s turn on roi report mode, rc=%d\n", __func__, rc);
	}
exit:
	TS_LOG_INFO("%s parade_resume Scan\n", __func__);
	rc = parade_hid_output_resume_scanning_(tskit_parade_data);
	if(rc)
		TS_LOG_ERR("%s resume failed\n", __func__);
#endif
	return rc;
}

static int parade_read_roi_switch(void) {
	TS_LOG_INFO("%s enter", __func__);
#ifdef PARADE_ROI_ENABLE
	tskit_parade_data->parade_chip_data->ts_platform_data->feature_info.roi_info.roi_switch = tskit_parade_data->roi_switch;
#endif
	return NO_ERR;
}


static int parade_roi_switch (struct ts_roi_info *info)
{
	int rc = NO_ERR;
	int t = 0;
	int timeout_ms = PARADE_WORK_WAIT_TIMEOUT;

	TS_LOG_INFO("%s enter", __func__);
	if(CY_SHOW_BEFORE_SUSPEND == tskit_parade_data->show_before_suspend_flag) {
		if(CY_AVOID_ROI_SWITCH == tskit_parade_data->avoid_roi_switch_flag) {
			TS_LOG_ERR("%s: avoid_roi_switch_flag is 1, return\n", __func__);
			return -EINVAL;
		}
	}

#ifdef PARADE_ROI_ENABLE
	rc = parade_check_cmd_status();
	if(rc)
		return rc;
	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		rc = -ENOMEM;
		goto out;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		rc = parade_set_roi_switch(info->roi_switch);
		if (rc < 0) {
			TS_LOG_ERR("%s parade_set_roi_switch faild\n",
				   __func__);
		}
		break;
	case TS_ACTION_READ:
		rc = parade_read_roi_switch();
		break;
	default:
		TS_LOG_INFO("invalid roi switch(%d) action: %d\n",
			    info->roi_switch, info->op_action);
		rc = -EINVAL;
		break;
	}
out:
	parade_finish_cmd();
#endif
	return rc;
}

static void parade_chip_touch_switch(void)
{
	unsigned char value = 0;
	char *ptr_begin = NULL, *ptr_end = NULL;
	char in_data[MAX_STR_LEN] = {0};
	int len = 0;
	int error = 0;
	unsigned char stype = 0, soper = 0, param = 0, sdatatowrite = 0;
	char *cur;
	char *token;
	int rc;
	TS_LOG_INFO("%s enter\n", __func__);
	rc = parade_check_cmd_status();
	if(rc)
		return;
	if ((NULL == tskit_parade_data) ||
		(NULL == tskit_parade_data->parade_chip_data)){
		TS_LOG_ERR("%s, error chip data\n",__func__);
		goto out;
	}
#ifdef PARADE_DOZE_ENABLE
	if (TS_SWITCH_TYPE_DOZE !=	(tskit_parade_data->parade_chip_data->touch_switch_flag & TS_SWITCH_TYPE_DOZE)){
		TS_LOG_ERR("%s, doze mode does not suppored by this chip\n",__func__);
		goto out;
	}
#endif
	/* SWITCH_OPER,ENABLE_DISABLE,PARAM */
	/* get switch operate */
#ifdef PARADE_DOZE_ENABLE
	memcpy(in_data, tskit_parade_data->parade_chip_data->touch_switch_info, MAX_STR_LEN);
#else
	memcpy(in_data, parade_global_pchar_for_doze, MAX_STR_LEN);
#endif

	TS_LOG_INFO("%s, in_data:%s\n",__func__, in_data);

	/* get stype */
	cur = (char*)in_data;
	token = strsep(&cur, ",");
	if(token==NULL){
		TS_LOG_ERR("%s stype get error\n",__func__);
		goto out;
	}
	stype = (unsigned char)simple_strtol(token, NULL, 0);
	TS_LOG_INFO("%s get stype=%d\n",__func__,stype);
	/* get switch operate */
	token = strsep(&cur, ",");
	if(token==NULL){
		TS_LOG_ERR("%s soper get error\n",__func__);
		goto out;
	}
	soper = (unsigned char)simple_strtol(token, NULL, 0);
	TS_LOG_INFO("%s get soper=%d\n",__func__,soper);
	/* get param */
	token = strsep(&cur, ",");
	if(token==NULL){
		TS_LOG_ERR("%s param get error\n",__func__);
		goto out;
	}
	param = (unsigned char)simple_strtol(token, NULL, 0);
	TS_LOG_INFO("%s get param=%d\n",__func__,param);

	if (TS_SWITCH_TYPE_DOZE != (stype & TS_SWITCH_TYPE_DOZE)){
		TS_LOG_ERR("%s stype not  TS_SWITCH_TYPE_DOZE:%d, invalid\n",__func__, stype);
		goto out;
	}
	/* Value"0"->On; Value"1"->Off */
	switch (soper){
		case TS_SWITCH_DOZE_ENABLE:
			sdatatowrite = param;
			break;
		case TS_SWITCH_DOZE_DISABLE:
			sdatatowrite = 0;/*Disable operation*/
			break;
		default:
			TS_LOG_ERR("%s soper unknown:%d, invalid\n", __func__ ,soper);
			goto out;
	}
	rc = parade_set_ram_parameter(tskit_parade_data,TOUCH_SWITCH_RAM_ID,sdatatowrite,CY_HID_OUTPUT_SET_RAM_NORMAL_PARAM);
	if(rc){
		TS_LOG_ERR("%s fail to set doze mode to value %d\n", __func__ ,sdatatowrite);
		goto out;
	}
	TS_LOG_INFO("%s set doze mode to value %d [OK]\n",__func__,sdatatowrite);
out:
	parade_finish_cmd();
	return;
}

static int parade_calibrate_child(void)
{
	int rc = NO_ERR;
	int write_length = 1;
	u8 write_buf[1];
	u8 status;

	int t = 0;
	int timeout_ms = PARADE_WORK_WAIT_TIMEOUT;

	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_CALIBRATE_IDACS),
		.write_length = write_length,
		.write_buf = write_buf,
		.timeout_ms = CY_HID_OUTPUT_CALIBRATE_IDAC_TIMEOUT,
	};
	TS_LOG_INFO("%s enter\n", __func__);
	/*do suspend before calibration*/
	rc = parade_hid_output_suspend_scanning_(tskit_parade_data);
	if(rc) {
		TS_LOG_ERR("%s parade_suspend failed\n", __func__);
		goto exit;
	}

	/*calibrate mutual cap*/
	write_buf[0] = 0;
	rc = parade_hid_send_output_and_wait_(tskit_parade_data, &hid_output);
	if(rc) {
		TS_LOG_ERR("%s parade_hid_send_output_and_wait_  for Mutual Cap failed\n", __func__);
		goto exit;
	}
	status = tskit_parade_data->response_buf[5];
	if(status) {
		TS_LOG_ERR("%s calibrate Mutual Cap failed\n", __func__);
		rc = -EINVAL;
		goto exit;
	}
	/*calibrate self cap*/
	write_buf[0] = 2;
	rc = parade_hid_send_output_and_wait_(tskit_parade_data, &hid_output);
	if(rc) {
		TS_LOG_ERR("%s parade_hid_send_output_and_wait_ for Self Cap failed\n", __func__);
		goto exit;
	}
	status = tskit_parade_data->response_buf[5];
	if(status) {
		TS_LOG_ERR("%s calibrate Sef Cap failed\n", __func__);
		rc = -EINVAL;
		goto exit;
	}
exit:
	rc = parade_hid_output_resume_scanning_(tskit_parade_data);
	if(rc)
		TS_LOG_ERR("%s parade_resume failed\n", __func__);
	return rc;
}

/*
 * Sense Mode:
 * 0: Calibrate IDAC for mutual-cap
 * 1: Calibrate IDAC for buttons
 * 2: Calibrate IDAC for self-cap
 * 3: Calibrate IDAC for mutual without calibrating hop frequencies
 * 4: Calibrate IDAC only, no hop calibration, no attenuator change
 * */
static int parade_calibrate(void)
{
	int rc = NO_ERR;

	rc = parade_check_cmd_status();
	if(rc)
		return rc;

	rc = parade_calibrate_child();

	parade_finish_cmd();

	return rc;
}

static int parade_hid_output_get_sysinfo_(struct parade_core_data *cd)
{
	int rc;
	int size;
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_GET_SYSINFO),
		.timeout_ms = CY_HID_OUTPUT_GET_SYSINFO_TIMEOUT,
	};
	TS_LOG_INFO("%s Enter\n", __func__);
	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	if (rc) {
		if(-EPROTO == rc)/*Patch: Wait for the first chip initialization, just in case the chip has no info*/
		{
			TS_LOG_ERR("%s failed, may be first init, wait 10s\n", __func__);
			msleep(CY_HID_OUTPUT_GET_SYSINFO_TIMEOUT_LONGGEST);
			rc = parade_hid_send_output_and_wait_(cd, &hid_output);
			if(rc){
				TS_LOG_ERR("%s failed, with add 10s delay retry, rc:%d\n", __func__,rc);
				return rc;
			}
		}
		else
			return rc;
	}
	TS_LOG_INFO("%s parade_get_sysinfo_regs start\n", __func__);
	rc = parade_get_sysinfo_regs(cd);
	if (rc)
		parade_free_si_ptrs(cd);
	TS_LOG_INFO("%s parade_get_sysinfo_regs end,rc =%d\n", __func__, rc);
	return rc;
}

static int parade_hid_output_read_conf_block_(struct parade_core_data *cd,
		u16 row_number, u16 length, u8 ebid, u8 *read_buf, u16 *crc)
{
	int read_ebid;
	int read_length;
	int status;
	int rc;
	int write_length = 5;
	u8 write_buf[5];
	u8 cmd_offset = 0;
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_READ_CONF_BLOCK),
		.write_length = write_length,
		.write_buf = write_buf,
	};

	write_buf[cmd_offset++] = LOW_BYTE(row_number);
	write_buf[cmd_offset++] = HI_BYTE(row_number);
	write_buf[cmd_offset++] = LOW_BYTE(length);
	write_buf[cmd_offset++] = HI_BYTE(length);
	write_buf[cmd_offset++] = ebid;

	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	if (rc)
		return rc;

	status = cd->response_buf[5];
	if (status)
		return -EINVAL;

	read_ebid = cd->response_buf[6];
	if ((read_ebid != ebid) || (cd->response_buf[9] != 0))
		return -EPROTO;

	read_length = get_unaligned_le16(&cd->response_buf[7]);
	if (length < read_length)
		length = read_length;

	memcpy(read_buf, &cd->response_buf[10], length);
	*crc = get_unaligned_le16(&cd->response_buf[read_length + 10]);

	return 0;
}

static int parade_hid_output_suspend_scanning_(struct parade_core_data *cd)
{

	TS_LOG_INFO("%s Enter\n", __func__);
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_SUSPEND_SCANNING),
	};

	return parade_hid_send_output_and_wait_(cd, &hid_output);
}

static int parade_hid_output_read_conf_ver_(struct parade_core_data *cd,
		u16 *config_ver)
{
	int rc;
	u8 read_buf[CY_TTCONFIG_VERSION_OFFSET + CY_TTCONFIG_VERSION_SIZE];
	u16 crc;

	rc = parade_hid_output_read_conf_block_(cd, CY_TTCONFIG_VERSION_ROW,
			CY_TTCONFIG_VERSION_OFFSET + CY_TTCONFIG_VERSION_SIZE,
			CY_TCH_PARM_EBID, read_buf, &crc);
	if (rc)
		return rc;

	*config_ver = get_unaligned_le16(
				&read_buf[CY_TTCONFIG_VERSION_OFFSET]);

	return 0;
}

static int parade_hid_output_resume_scanning_(struct parade_core_data *cd)
{
	TS_LOG_INFO("%s Enter\n",__func__);
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_RESUME_SCANNING),
	};

	return parade_hid_send_output_and_wait_(cd, &hid_output);
}

static int parade_get_config_ver_(struct parade_core_data *cd)
{
	struct parade_sysinfo *si = &cd->sysinfo;
	int rc;
	int ret = 0;
	u16 config_ver = 0;

	rc = parade_hid_output_suspend_scanning_(cd);
	if (rc)
		goto error;

	rc = parade_hid_output_read_conf_ver_(cd, &config_ver);
	if (rc)
		goto exit;

	si->cydata.fw_ver_conf = config_ver;

exit:
	ret = parade_hid_output_resume_scanning_(cd);
	if (ret) {
		TS_LOG_ERR("%s: resume_scanning failed\n", __func__);
	}
error:
	TS_LOG_DEBUG("%s: CONFIG_VER:%04X\n", __func__, config_ver);
	return rc;
}

/*added tp_color*/
static int parade_hid_output_read_color_info_(struct parade_core_data *cd,
		u8 *color_info)
{
	int rc = NO_ERR;
	u8 read_buf[TP_COLOR_READ_SIZE] = {0};
	u16 crc = 0;

	if (cd == NULL ) {
		pr_err("%s, cd is error\n", __func__);
		return -EINVAL;
	}

	rc = parade_hid_output_read_conf_block_(cd, CY_TPCOLOR_ROW,
		CY_TPCOLOR_READ_SIZE,CY_MDATA_EBID, read_buf, &crc);
	if (rc)
		return rc;

	*color_info = read_buf[TP_COLOR_READ_SIZE - 1];

	return NO_ERR;
}
static int parade_get_color_info(struct parade_core_data *cd)
{
	int rc = NO_ERR;
	int ret = 0;
	u8 color_info = 0;

	if (cd == NULL ) {
		pr_err("%s, cd is error\n", __func__);
		return -EINVAL;
	}

	rc = parade_hid_output_suspend_scanning_(cd);
	if (rc)
		goto error;

	rc = parade_hid_output_read_color_info_(cd, &color_info);
	if (rc) {
		TS_LOG_ERR("%s: get color info failed!\n", __func__);
		goto exit;
	}
	 cypress_ts_kit_color[0]= color_info;
exit:
	ret = parade_hid_output_resume_scanning_(cd);
	if (ret) {
		TS_LOG_ERR("%s: resume_scanning failed\n", __func__);
	}
error:
	TS_LOG_INFO("%s: color_info:%04X\n", __func__, color_info);
	return rc;
}

static ssize_t parade_doze_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t size)
{
	struct parade_core_data *cd = tskit_parade_data;
	TS_LOG_INFO("%s : enter, data: %s \n", __func__, buf);
#ifdef PARADE_DOZE_ENABLE
	if (size >= MAX_STR_LEN) {
		memcpy(tskit_parade_data->parade_chip_data->touch_switch_info, buf, MAX_STR_LEN-1);
		TS_LOG_ERR("%s: input string is large than %d bit\n", __func__,MAX_STR_LEN);
	}
	else {
		memcpy(tskit_parade_data->parade_chip_data->touch_switch_info, buf, size);
	}
#else
	parade_global_pchar_for_doze = buf;
#endif
	parade_chip_touch_switch();
	return size;
}
static DEVICE_ATTR(doze, S_IWUSR, NULL, parade_doze_store);

static ssize_t parade_ic_ver_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct parade_core_data *cd = tskit_parade_data;
	struct parade_cydata *cydata = &cd->sysinfo.cydata;
	int rc = parade_hid_output_get_sysinfo_(cd);
	if(rc < 0){
		return sprintf(buf,
		"%s:Get Run Time Sysinfo failed, Error Code: %d",__func__,rc);
	}
	else{
        return sprintf(buf,
		"%s: 0x%02X\n"
		"%s: 0x%02X\n"
		"%s: 0x%08X\n"
		"%s: 0x%04X\n"
		"%s: 0x%02X\n"
		"%s: 0x%02X\n"
		"%s: 0x%02X\n"
		"%s: 0x%02X\n",
		"Firmware Major Version", cydata->fw_ver_major,
		"Firmware Minor Version", cydata->fw_ver_minor,
		"Revision Control Number", cydata->revctrl,
		"Firmware Configuration Version", cydata->fw_ver_conf,
		"Bootloader Major Version", cydata->bl_ver_major,
		"Bootloader Minor Version", cydata->bl_ver_minor,
		"Protocol Major Version", cydata->pip_ver_major,
		"Protocol Minor Version", cydata->pip_ver_minor);
    }

}

static DEVICE_ATTR(ic_ver, S_IRUSR, parade_ic_ver_show, NULL);

static ssize_t parade_drv_ver_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
        return sprintf(buf,
		"%s: %d.%d\n"
		"%s: %s\n",
		"Driver Version", PARADE_DRV_VER_MAJOR,PARADE_DRV_VER_MINOR,
		"Last Change Date", PARADE_DRV_LAST_CHANGE_DATE);
}

static DEVICE_ATTR(drv_ver, S_IRUSR, parade_drv_ver_show, NULL);

static ssize_t parade_drv_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct parade_core_data *cd = tskit_parade_data;
	unsigned long value;
	int rc;

	rc = kstrtoul(buf, 10, &value);
	if (rc < 0) {
		TS_LOG_ERR("%s: Invalid value\n", __func__);
		goto exit;
	}

	switch (value) {
#ifdef TTHE_TUNER_SUPPORT
	case CY_TTHE_TUNER_EXIT:
		cd->tthe_exit = 1;
		wake_up(&cd->wait_q);
		kfree(cd->tthe_buf);
		cd->tthe_buf = NULL;
		cd->tthe_exit = 0;
		break;
	case CY_TTHE_BUF_CLEAN:
		if (cd->tthe_buf)
			memset(cd->tthe_buf, 0, PARADE_MAX_PRBUF_SIZE);
		else
			TS_LOG_INFO("%s : tthe_buf not existed\n", __func__);
		break;
	case CY_DBG_HID_STOP_WD:
		TS_LOG_INFO( "%s: stop watchdog (cd=%p)\n", __func__, cd);
		ts_stop_wd_timer(&g_ts_kit_platform_data);
		break;
	case CY_DBG_HID_START_WD:
		TS_LOG_INFO( "%s: start watchdog (cd=%p)\n", __func__, cd);
		ts_start_wd_timer(&g_ts_kit_platform_data);
		break;
#endif
	default:
		TS_LOG_ERR("%s: Invalid value\n", __func__);
	}

exit:
	return size;
}

static DEVICE_ATTR(drv_debug, S_IWUSR, NULL, parade_drv_debug_store);

#ifdef TTHE_TUNER_SUPPORT
static int tthe_debugfs_open(struct inode *inode, struct file *filp)
{
	struct parade_core_data *cd = inode->i_private;
	mutex_lock(&cd->tthe_lock);
	filp->private_data = inode->i_private;

	if (cd->tthe_buf){
		mutex_unlock(&cd->tthe_lock);
		return -EBUSY;
	}

	cd->tthe_buf = kzalloc(PARADE_MAX_PRBUF_SIZE, GFP_KERNEL);
	if (!cd->tthe_buf){
		mutex_unlock(&cd->tthe_lock);
		return -ENOMEM;
	}
	mutex_unlock(&cd->tthe_lock);
	return 0;
}

static int tthe_debugfs_close(struct inode *inode, struct file *filp)
{
	struct parade_core_data *cd = filp->private_data;
	mutex_lock(&cd->tthe_lock);
	filp->private_data = NULL;

	kfree(cd->tthe_buf);
	cd->tthe_buf = NULL;
	mutex_unlock(&cd->tthe_lock);
	return 0;
}

static ssize_t tthe_debugfs_read(struct file *filp, char __user *buf,
		size_t count, loff_t *ppos)
{
	struct parade_core_data *cd = filp->private_data;
	int size;
	int ret = 0;

	wait_event_interruptible(cd->wait_q,
			cd->tthe_buf_len != 0 || cd->tthe_exit);
	mutex_lock(&cd->tthe_lock);
	if (cd->tthe_exit) {
		mutex_unlock(&cd->tthe_lock);
		return ret;
	}
	if (count > cd->tthe_buf_len)
		size = cd->tthe_buf_len;
	else
		size = count;
	if (!size) {
		mutex_unlock(&cd->tthe_lock);
		return ret;
	}
	if (cd->tthe_buf_len >= count){
		mutex_unlock(&cd->tthe_lock);
		return -EFAULT;
	}
	ret = copy_to_user(buf, cd->tthe_buf, cd->tthe_buf_len);
	if (ret == size){
		mutex_unlock(&cd->tthe_lock);
		return -EFAULT;}
	size -= ret;
	cd->tthe_buf_len -= size;
	*ppos += size;
	mutex_unlock(&cd->tthe_lock);
	return size;
}

static const struct file_operations tthe_debugfs_fops = {
	.open = tthe_debugfs_open,
	.release = tthe_debugfs_close,
	.read = tthe_debugfs_read,
};
#endif

static int parade_exec_panel_scan_cmd_(struct device *dev)
{
	struct parade_core_data *cd = tskit_parade_data;
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_EXEC_PANEL_SCAN),
	};

	return parade_hid_send_output_and_wait_(cd, &hid_output);
}

static int cyttsp5_hid_output_retrieve_panel_scan_(
		struct parade_core_data *cd, u16 read_offset, u16 read_count,
		u8 data_id, u8 *response, u8 *config, u16 *actual_read_len,
		u8 *read_buf)
{
	int status;
	u8 read_data_id;
	int rc;
	int write_length = 5;
	u8 write_buf[5];
	u8 cmd_offset = 0;
	u8 data_elem_size;
	int size;
	int data_size;
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_RETRIEVE_PANEL_SCAN),
		.write_length = write_length,
		.write_buf = write_buf,
	};

	write_buf[cmd_offset++] = LOW_BYTE(read_offset);
	write_buf[cmd_offset++] = HI_BYTE(read_offset);
	write_buf[cmd_offset++] = LOW_BYTE(read_count);
	write_buf[cmd_offset++] = HI_BYTE(read_count);
	write_buf[cmd_offset++] = data_id;

	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	if (rc)
		return rc;

	status = cd->response_buf[5];
	if (status)
		return -EINVAL;

	read_data_id = cd->response_buf[6];
	if (read_data_id != data_id)
		return -EPROTO;

	size = get_unaligned_le16(&cd->response_buf[0]);
	*actual_read_len = get_unaligned_le16(&cd->response_buf[7]);
	*config = cd->response_buf[9];

	data_elem_size = *config & 0x07;
	data_size = *actual_read_len * data_elem_size;

	if (read_buf)
		memcpy(read_buf, &cd->response_buf[10], data_size);
	if (response)
		memcpy(response, cd->response_buf, size);
	return rc;
}

static int parade_ret_scan_data_cmd_(struct device *dev, u16 read_offset,
		u16 read_count, u8 data_id, u8 *response, u8 *config,
		u16 *actual_read_len, u8 *return_buf)
{
	int status;
	u8 read_data_id;
	int rc;
	int write_length = 5;
	u8 write_buf[5];
	u8 cmd_offset = 0;
	u8 data_elem_size;
	int size;
	int data_size;
	struct parade_core_data *cd = tskit_parade_data;

	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_RETRIEVE_PANEL_SCAN),
		.write_length = write_length,
		.write_buf = write_buf,
	};

	write_buf[cmd_offset++] = LOW_BYTE(read_offset);
	write_buf[cmd_offset++] = HI_BYTE(read_offset);
	write_buf[cmd_offset++] = LOW_BYTE(read_count);
	write_buf[cmd_offset++] = HI_BYTE(read_count);
	write_buf[cmd_offset++] = data_id;

	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	if (rc)
		return rc;

	status = cd->response_buf[5];
	if (status)
		return -EINVAL;

	read_data_id = cd->response_buf[6];
	if (read_data_id != data_id)
		return -EPROTO;

	size = get_unaligned_le16(&cd->response_buf[0]);
	*actual_read_len = get_unaligned_le16(&cd->response_buf[7]);
	*config = cd->response_buf[9];

	data_elem_size = *config & 0x07;
	data_size = *actual_read_len * data_elem_size;

	if (return_buf)
		memcpy(return_buf, &cd->response_buf[10], data_size);
	if (response)
		memcpy(response, cd->response_buf, size);

	return rc;
}

#ifdef TTHE_TUNER_SUPPORT
static ssize_t tthe_get_panel_data_debugfs_read(struct file *filp,
		char __user *buf, size_t count, loff_t *ppos)
{
	struct parade_device_access_data *dad = filp->private_data;
	struct device *dev;
	u8 config;
	u16 actual_read_len;
	u16 length = 0;
	u8 element_size = 0;
	u8 *buf_offset;
	u8 *buf_out;
	int elem;
	int elem_offset = 0;
	int print_idx = 0;
	int rc;

	int i;

	mutex_lock(&dad->debugfs_lock);
	dev = dad->dev;
	buf_out = dad->tthe_get_panel_data_buf;
	if (!buf_out)
		goto release_mutex;

	if (dad->heatmap.scan_start) {
		/* To fix CDT206291: avoid multiple scans when
		return data is larger than 4096 bytes in one cycle */
		dad->heatmap.scan_start = 0;

		/* Start scan */
		rc = parade_exec_panel_scan_cmd_(dev);
		if (rc < 0)
			goto release_mutex;
	}

	elem = dad->heatmap.num_element;

#if defined(CY_ENABLE_MAX_ELEN)
	if (elem > CY_MAX_ELEN) {
		rc = parade_ret_scan_data_cmd_(dev, elem_offset,
		CY_MAX_ELEN, dad->heatmap.data_type, dad->ic_buf,
		&config, &actual_read_len, NULL);
	} else{
		rc = parade_ret_scan_data_cmd_(dev, elem_offset, elem,
			dad->heatmap.data_type, dad->ic_buf, &config,
			&actual_read_len, NULL);
	}
#else
	rc = parade_ret_scan_data_cmd_(dev, elem_offset, elem,
			dad->heatmap.data_type, dad->ic_buf, &config,
			&actual_read_len, NULL);
#endif
	if (rc < 0)
		goto release_mutex;

	length = get_unaligned_le16(&dad->ic_buf[0]);
	buf_offset = dad->ic_buf + length;

	element_size = config & CY_CMD_RET_PANEL_ELMNT_SZ_MASK;

	elem -= actual_read_len;
	elem_offset = actual_read_len;
	while (elem > 0) {
#ifdef CY_ENABLE_MAX_ELEN
		if (elem > CY_MAX_ELEN) {
			rc = parade_ret_scan_data_cmd_(dev, elem_offset,
			CY_MAX_ELEN, dad->heatmap.data_type, NULL, &config,
			&actual_read_len, buf_offset);
		} else{
			rc = parade_ret_scan_data_cmd_(dev, elem_offset, elem,
				dad->heatmap.data_type, NULL, &config,
				&actual_read_len, buf_offset);
		}
#else

		rc = parade_ret_scan_data_cmd_(dev, elem_offset, elem,
				dad->heatmap.data_type, NULL, &config,
				&actual_read_len, buf_offset);
#endif
		if (rc < 0)
			goto release_mutex;

		if (!actual_read_len)
			break;

		length += actual_read_len * element_size;
		buf_offset = dad->ic_buf + length;
		elem -= actual_read_len;
		elem_offset += actual_read_len;
	}

	/* Reconstruct cmd header */
	put_unaligned_le16(length, &dad->ic_buf[0]);
	put_unaligned_le16(elem_offset, &dad->ic_buf[7]);

	if (rc < 0)
		goto release_mutex;

	print_idx += scnprintf(buf_out, TTHE_TUNER_MAX_BUF, "CY_DATA:");
	for (i = 0; i < length; i++)
		print_idx += scnprintf(buf_out + print_idx,
				TTHE_TUNER_MAX_BUF - print_idx,
				"%02X ", dad->ic_buf[i]);
	print_idx += scnprintf(buf_out + print_idx,
			TTHE_TUNER_MAX_BUF - print_idx,
			":(%d bytes)\n", length);
	rc = simple_read_from_buffer(buf, count, ppos, buf_out, print_idx);
	print_idx = rc;

release_mutex:
	mutex_unlock(&dad->debugfs_lock);
	return print_idx;
}

static ssize_t tthe_get_panel_data_debugfs_write(struct file *filp,
		const char __user *buf, size_t count, loff_t *ppos)
{
	struct parade_device_access_data *dad = filp->private_data;
	struct device *dev = dad->dev;
	ssize_t length;
	int max_read;
	u8 *buf_in = dad->tthe_get_panel_data_buf;
	int ret;

	TS_LOG_INFO("%s enter\n", __func__);
	if (TTHE_TUNER_MAX_BUF - *ppos < count) {
		TS_LOG_ERR("%s:buf_in full\n", __func__);
		return count;
	}
	mutex_lock(&dad->debugfs_lock);
	ret = copy_from_user(buf_in + (*ppos), buf, count);
	if (ret)
		goto exit;
	buf_in[count] = 0;

	length = parade_ic_parse_input(dev, buf_in, count, dad->ic_buf,
			PARADE_MAX_PRBUF_SIZE);
	if (length <= 0) {
		TS_LOG_ERR("%s: %s Group Data store\n", __func__,
				"Malformed input for");
		goto exit;
	}

	/* update parameter value */
	dad->heatmap.num_element = get_unaligned_le16(&dad->ic_buf[3]);
	dad->heatmap.data_type = dad->ic_buf[5];

	if (dad->ic_buf[6] > 0)
		dad->heatmap.scan_start = true;
	else
		dad->heatmap.scan_start = false;

	/* elem can not be bigger then buffer size */
	max_read = CY_CMD_RET_PANEL_HDR;
	max_read += dad->heatmap.num_element * CY_CMD_RET_PANEL_ELMNT_SZ_MAX;

	if (max_read >= PARADE_MAX_PRBUF_SIZE) {
		dad->heatmap.num_element =
			(PARADE_MAX_PRBUF_SIZE - CY_CMD_RET_PANEL_HDR)
			/ CY_CMD_RET_PANEL_ELMNT_SZ_MAX;
		TS_LOG_ERR("%s: Will get %d element\n", __func__,
				dad->heatmap.num_element);
	}

exit:
	mutex_unlock(&dad->debugfs_lock);
	TS_LOG_INFO("%s: return count=%zu\n", __func__, count);
	return count;
}

static int tthe_get_panel_data_debugfs_open(struct inode *inode,
		struct file *filp)
{
	struct parade_device_access_data *dad = inode->i_private;

	mutex_lock(&dad->debugfs_lock);

	if (dad->tthe_get_panel_data_is_open) {
		mutex_unlock(&dad->debugfs_lock);
		return -EBUSY;
	}

	filp->private_data = inode->i_private;

	dad->tthe_get_panel_data_is_open = 1;
	mutex_unlock(&dad->debugfs_lock);
	return 0;
}

static int tthe_get_panel_data_debugfs_close(struct inode *inode,
		struct file *filp)
{
	struct parade_device_access_data *dad = filp->private_data;

	mutex_lock(&dad->debugfs_lock);
	filp->private_data = NULL;
	dad->tthe_get_panel_data_is_open = 0;
	mutex_unlock(&dad->debugfs_lock);

	return 0;
}

static const struct file_operations tthe_get_panel_data_fops = {
	.open = tthe_get_panel_data_debugfs_open,
	.release = tthe_get_panel_data_debugfs_close,
	.read = tthe_get_panel_data_debugfs_read,
	.write = tthe_get_panel_data_debugfs_write,
};
#endif

static int parade_dad_setup_sysfs(struct device *dev, struct parade_device_access_data *dad)
{
	int rc = 0;

#ifdef TTHE_TUNER_SUPPORT
	dad->tthe_get_panel_data_debugfs = debugfs_create_file(
			PARADE_TTHE_TUNER_GET_PANEL_DATA_FILE_NAME,
			0644, NULL, dad, &tthe_get_panel_data_fops);
	if (IS_ERR_OR_NULL(dad->tthe_get_panel_data_debugfs)) {
		TS_LOG_ERR("%s: Error, could not create get_panel_data\n",
				__func__);
		dad->tthe_get_panel_data_debugfs = NULL;
	}
#endif

	return rc;

}

static int parade_get_mode(struct parade_core_data *cd,
		struct parade_hid_desc *desc)
{
	if (CY_HID_APP_REPORT_ID == desc->packet_id)
		return CY_MODE_OPERATIONAL;
	else if (CY_HID_BL_REPORT_ID == desc->packet_id)
		return CY_MODE_BOOTLOADER;

	return CY_MODE_UNKNOWN;
}
static int parade_hid_output_bl_launch_app_(struct parade_core_data *cd)
{
	struct parade_hid_output hid_output = {
		HID_OUTPUT_BL_COMMAND(HID_OUTPUT_BL_LAUNCH_APP),
		.reset_expected = 1,
	};
	return parade_hid_send_output_and_wait_(cd, &hid_output);
}

static int parade_hid_output_null_(struct cyttsp5_core_data *cd)
{
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_NULL),
	};

	return parade_hid_send_output_and_wait_(cd, &hid_output);
}
static int  parade_watchdog_work_check(void)
{
	struct parade_core_data *cd = tskit_parade_data;
	int rc = NO_ERR;

	rc = parade_check_cmd_status();
	if(rc)
		return rc;
	rc = parade_hid_output_null_(cd);

	if (rc) {
		TS_LOG_ERR(
			"%s: failed to access device in watchdog timer r=%d\n",
			__func__, rc);
		parade_esd_reset_function();
	}
	parade_finish_cmd();
	return rc;
}
static int parade_esd_reset_function(void)
{
	struct parade_core_data *cd =  tskit_parade_data;
	int rc = NO_ERR;

	TS_LOG_INFO( "%s: enter\n",__func__);
	/* power on-off */
	parade_power_off();
	mdelay(1000);
	parade_power_on();
	mdelay(200);

	rc = parade_startup(cd, true);
	if (rc < 0) {
		TS_LOG_ERR( "%s: Fail queued startup r=%d\n",
			__func__, rc);
#if defined (CONFIG_HUAWEI_DSM)
		ts_dmd_report(DSM_TP_ESD_ERROR_NO, "parade ESD reset fail.\n");
#endif
	}
	return rc;
}

static int parade_startup(struct parade_core_data *cd, bool is_need_reset)
{
	int retry = CY_CORE_STARTUP_RETRY_COUNT;
	int rc = NO_ERR;
	bool detected = false;
	TS_LOG_INFO("%s: call startup begin\n", __func__);

#ifdef TTHE_TUNER_SUPPORT
		tthe_print(cd, NULL, 0, "enter startup");
#endif
reset:
	if (retry != CY_CORE_STARTUP_RETRY_COUNT)
		TS_LOG_DEBUG("%s: Retry %d\n", __func__,
			CY_CORE_STARTUP_RETRY_COUNT - retry);
	if(is_need_reset)
	{
		//enable_irq(g_ts_data.irq_id);
		//msleep(20);
	}
	else{
		rc = parade_check_and_deassert_int(cd);
	}
	TS_LOG_INFO("%s: start to check i2c.\n", __func__);
	if (is_need_reset || retry != CY_CORE_STARTUP_RETRY_COUNT) {
			/* reset hardware */
			TS_LOG_INFO("%s: start to rest hw & wait.\n", __func__);
			rc = parade_reset_and_wait(cd);
			if (rc < 0) {
				TS_LOG_ERR("%s: Error on h/w reset r=%d\n", __func__,
					   rc);
				RETRY_OR_EXIT(retry--, reset, exit);
			}

	}
	TS_LOG_INFO("%s: Reading hid descriptor\n", __func__);
	rc = parade_get_hid_descriptor_(cd, &cd->hid_desc);
	if (rc < 0) {
		TS_LOG_INFO("%s: Error on getting HID descriptor r=%d\n",
			__func__, rc);
		RETRY_OR_EXIT(retry--, reset, exit);
	}
	/*Start to get mode*/
	TS_LOG_INFO("%s: start to get mode.\n", __func__);
	cd->mode = parade_get_mode(cd, &cd->hid_desc);

	detected = true;

	/* if still bootloader mode after 200ms*/
	if (cd->mode == CY_MODE_BOOTLOADER) {
		TS_LOG_INFO("%s: Bootloader Mode: start to launch_app.\n", __func__);
		rc = parade_hid_output_bl_launch_app_(cd);
		if (rc < 0) {
			TS_LOG_ERR("%s: Error on launch app r=%d,FW May be Corrupted\n", __func__, rc);
			rc = NO_ERR; /*Exit with no error, wait for fw upgrade boot to cover*/
			goto exit;
		}

	}
	else if(cd->mode == CY_MODE_UNKNOWN){
		RETRY_OR_EXIT(retry--, reset, exit);
	}
	TS_LOG_INFO("%s: Application Mode.\n", __func__);
	/* Read descriptor lengths */
	cd->hid_core.hid_report_desc_len =
		le16_to_cpu(cd->hid_desc.report_desc_len);
	cd->hid_core.hid_max_input_len =
		le16_to_cpu(cd->hid_desc.max_input_len);
	cd->hid_core.hid_max_output_len =
		le16_to_cpu(cd->hid_desc.max_output_len);

	TS_LOG_INFO("%s: Reading report descriptor\n", __func__);
	rc = parade_get_report_descriptor_(cd);
	if (rc < 0) {
		TS_LOG_ERR("%s: Error on getting report descriptor r=%d\n",
			__func__, rc);
		RETRY_OR_EXIT(retry--, reset, exit);
	}
	TS_LOG_INFO("%s: Reading system information\n", __func__);
	rc = parade_hid_output_get_sysinfo_(cd);
	if (rc) {
		TS_LOG_ERR("%s: Error on getting sysinfo r=%d\n",
			__func__, rc);
		RETRY_OR_EXIT(retry--, reset, exit);
	}
	if(tskit_parade_data->sleep_in_mode != 0) { //deep sleep mode
		/*Set Power On Flag*/
		rc = parade_set_power_on_flag(cd);
		if (rc) {
			TS_LOG_ERR("%s: set power on flag fail r=%d\n",
				__func__, rc);
			rc = NO_ERR;
		}
		/*Get Power On Flag After*/
		rc = parade_get_power_on_flag(cd);
		if (rc) {
			TS_LOG_ERR("%s: get power on flag fail rc=%d\n",
				__func__, rc);
			rc = NO_ERR;
		}
	}
	TS_LOG_INFO("parade Protocol Version: %d.%d\n",
			cd->sysinfo.cydata.pip_ver_major,
			cd->sysinfo.cydata.pip_ver_minor);
	if (!rc)
		cd->startup_retry_count = 0;

exit:
	TS_LOG_INFO("%s: call startup end\n", __func__);
#ifdef TTHE_TUNER_SUPPORT
		tthe_print(cd, NULL, 0, "exit startup");
#endif
	return rc;
}

static void parade_parse_specific_dts(struct ts_kit_device_data *chip_data)
{
       TS_LOG_INFO("+\n");
       ts_kit_anti_false_touch_param_achieve(chip_data);
       TS_LOG_INFO("-\n");

       return ;
}

static int parade_init_chip(void)
{
	int rc = NO_ERR;
	struct parade_core_data *cd = tskit_parade_data;

#if defined (CONFIG_TEE_TUI)
	strncpy(tee_tui_data.device_name, "parade", strlen("parade"));
	tee_tui_data.device_name[strlen("parade")] = '\0';
#endif
	cd->force_wait = false;
	rc = parade_startup(cd, true);
	if(rc){
		TS_LOG_ERR("%s: startup failed\n", __func__);
		//return rc; don't  return error ,wait fw update to recovery.
	}
	if(tskit_parade_data->support_get_tp_color == true){
		parade_get_color_info(cd);
	}
	parade_device_access_probe(cd);
	parade_parse_specific_dts(tskit_parade_data->parade_chip_data);
	/*provide panel_id for sensor*/
	g_ts_kit_platform_data.panel_id = tskit_parade_data->panel_id;
	TS_LOG_INFO("%s: call parade_init_chip end\n", __func__);
	return NO_ERR;
}

static s16 *create_and_get_u16_array(struct device_node *dev_node,
		const char *name, int *size)
{
	const __be32 *values;
	s16 *val_array;
	int len;
	int sz;
	int rc;
	int i;

	values = of_get_property(dev_node, name, &len);
	if (values == NULL)
		return NULL;

	sz = len / sizeof(u32);
	TS_LOG_INFO("%s: %s size:%d\n", __func__, name, sz);

	val_array = kcalloc(sz, sizeof(s16), GFP_KERNEL);
	if (!val_array) {
		rc = -ENOMEM;
		goto fail;
	}

	for (i = 0; i < sz; i++)
		val_array[i] = (s16)be32_to_cpup(values++);

	*size = sz;

	return val_array;

fail:
	return ERR_PTR(rc);
}

static struct touch_settings *create_and_get_touch_setting(
		struct device_node *core_node, const char *name)
{
	 struct touch_settings *setting;
	 char *tag_name;
	 u32 tag_value;
	 u16 *data;
	 int size;
	 int rc;

	 data = create_and_get_u16_array(core_node, name, &size);
	 if (IS_ERR_OR_NULL(data))
		 return (void *)data;

	 TS_LOG_INFO("%s: Touch setting:'%s' size:%d\n", __func__, name, size);

	 setting = kzalloc(sizeof(*setting), GFP_KERNEL);
	 if (!setting) {
		 rc = -ENOMEM;
		 goto fail_free_data;
	 }

	 setting->data = (u8 *)data;
	 setting->size = size;

	 tag_name = kzalloc(MAX_NAME_LENGTH, GFP_KERNEL);
	 if (!tag_name) {
		 rc = -ENOMEM;
		 goto fail_free_setting;
	 }

	 snprintf(tag_name, MAX_NAME_LENGTH, "%s-tag", name);

	 rc = of_property_read_u32(core_node, tag_name, &tag_value);
	 if (!rc)
		 setting->tag = tag_value;

	 kfree(tag_name);

	 return setting;

 fail_free_setting:
	 kfree(setting);
 fail_free_data:
	 kfree(data);

	 return ERR_PTR(rc);
 }

static int parade_get_dts_value(struct device_node *core_node,char *name)
{
	int value;
	int rc;
	rc = of_property_read_u32(core_node, name, &value);
	if (rc) {
		TS_LOG_DEBUG("%s,[%s] read fail, rc = %d.\n",__func__, name, rc);
		return -1;
	}
	return value;
}

static void parade_pinctrl_release(void)
{
	if (tskit_parade_data->pctrl) {
		devm_pinctrl_put(tskit_parade_data->pctrl);
	}
}

static int parade_pinctrl_select_normal(void)
{
	int retval = -1;

	if (tskit_parade_data->pctrl == NULL || tskit_parade_data->pins_default == NULL) {
		TS_LOG_ERR("%s: pctrl or pins_default is NULL.\n", __func__);
		return retval;
	}
	retval = pinctrl_select_state(tskit_parade_data->pctrl, tskit_parade_data->pins_default);
	if (retval < 0) {
		TS_LOG_ERR("%s: set pinctrl normal error.\n", __func__);
	}
	return retval;
}
static int parade_pinctrl_select_lowpower(void)
{
	int retval = -1;

	if (tskit_parade_data->pctrl == NULL || tskit_parade_data->pins_idle== NULL) {
		TS_LOG_ERR("%s: pctrl or pins_idle is NULL.\n", __func__);
		return retval;
	}
	retval = pinctrl_select_state(tskit_parade_data->pctrl, tskit_parade_data->pins_idle);
	if (retval < 0) {
		TS_LOG_ERR("%s: set pinctrl pins_idle error.\n", __func__);
	}
	return retval;
}

static int parade_pinctrl_get_init(void)
{
	int ret = 0;

	tskit_parade_data->pctrl = devm_pinctrl_get(&tskit_parade_data->parade_dev->dev);
	if (IS_ERR(tskit_parade_data->pctrl)) {
		TS_LOG_ERR("%s failed to devm pinctrl get\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	tskit_parade_data->pins_default =
	    pinctrl_lookup_state(tskit_parade_data->pctrl, "default");
	if (IS_ERR(tskit_parade_data->pins_default)) {
		TS_LOG_ERR("%s fail to pinctrl lookup state default\n", __func__);
		ret = -EINVAL;
		goto err_pinctrl_put;
	}

	tskit_parade_data->pins_idle = pinctrl_lookup_state(tskit_parade_data->pctrl, "idle");
	if (IS_ERR(tskit_parade_data->pins_idle)) {
		TS_LOG_ERR("%s failed to pinctrl lookup state idle\n", __func__);
		ret = -EINVAL;
		goto err_pinctrl_put;
	}
	return 0;

err_pinctrl_put:
	devm_pinctrl_put(tskit_parade_data->pctrl);
	return ret;
}
static int parade_get_lcd_panel_info(void)
{
	struct device_node *dev_node = NULL;
	char *lcd_type = NULL;
	dev_node = of_find_compatible_node(NULL, NULL, LCD_PANEL_TYPE_DEVICE_NODE_NAME);
	if (!dev_node) {
		TS_LOG_ERR("%s: NOT found device node[%s]!\n", __func__, LCD_PANEL_TYPE_DEVICE_NODE_NAME);
		return -EINVAL;
	}
	lcd_type = (char*)of_get_property(dev_node, "lcd_panel_type", NULL);
	if(!lcd_type){
		TS_LOG_ERR("%s: Get lcd panel type faile!\n", __func__);
		return -EINVAL ;
	}

	strncpy(tskit_parade_data->lcd_panel_info, lcd_type, LCD_PANEL_INFO_MAX_LEN-1);
	TS_LOG_INFO("lcd_panel_info = %s.\n", tskit_parade_data->lcd_panel_info);
	return 0;
}

static int parade_get_lcd_panel_info_from_lcdkit(void)
{
	char *lcd_type = NULL;

	lcd_type = trans_lcd_panel_name_to_tskit();
	if(!lcd_type){
		TS_LOG_ERR("%s: Get lcd panel type from lcdkit fail!\n", __func__);
		return -EINVAL ;
	}
	strncpy(tskit_parade_data->lcd_panel_info, lcd_type, LCD_PANEL_INFO_MAX_LEN-1);
	TS_LOG_INFO("lcd_panel_info = %s.\n", tskit_parade_data->lcd_panel_info);
	return 0;
}

static void get_current_distinguishability(void)
{
	char *cur =NULL;
	int ret ;
	tskit_parade_data->current_distinguishability = false;//false:default distinguishability

	if(tskit_parade_data->lcd_panel_name_from_lcdkit){
		ret = parade_get_lcd_panel_info_from_lcdkit();
		if(ret){
			TS_LOG_ERR("%s: get lcd panel info from lcdkit fail!\n ",__func__);
			return;
		}
	}else{
		ret = parade_get_lcd_panel_info();
		if(ret){
			TS_LOG_ERR("%s: get lcd panel info faile!\n ",__func__);
			return;
		}
	}
	cur = strstr(tskit_parade_data->lcd_panel_info, tskit_parade_data->sign_of_second_distinguishability);
	if(!cur){
		TS_LOG_INFO("%s:LCD distinguishability is default \n ",__func__);
		return;
	}
	tskit_parade_data->current_distinguishability = true;//true :second distinguishability
	return;
}
static int  get_lcd_module_name(void)
{
	char temp[LCD_PANEL_INFO_MAX_LEN] = {0};
	int i = 0;
	strncpy(temp, tskit_parade_data->lcd_panel_info, LCD_PANEL_INFO_MAX_LEN-1);
	for(i=0;i<(LCD_PANEL_INFO_MAX_LEN -1) && (i < (MAX_STR_LEN -1));i++)
	{
		if(temp[i] == '_')
		{
			break;
		}
		tskit_parade_data->lcd_module_name[i] = tolower(temp[i]);
	}
	TS_LOG_INFO("lcd_module_name = %s.\n", tskit_parade_data->lcd_module_name);
	return 0;
}
int parade_irq_stat(struct ts_kit_device_data *pdata)
{
	return gpio_get_value(pdata->ts_platform_data->irq_gpio);
}
/* get support cp_delta cap test mode value */
static void parade_get_cp_delta_test_support(struct device_node *core_node)
{
	int value;

	value = parade_get_dts_value(core_node, "cp_delta_cap_test_supported");
	if(value < 0){
		value = 1; //default support
	}
	tskit_parade_data->cp_delta_test_supported = value;
}
void parse_report_rate_check_support(struct device_node *core_node)
{
	int value = 0;

	if(NULL == tskit_parade_data) {
		TS_LOG_ERR("%s tskit_parade_data NULL\n", __func__);
		return;
	}
	if(NULL == core_node) {
		tskit_parade_data->report_rate_check_supported = CY_REPORT_RATE_UNSUP;
		TS_LOG_ERR("%s value or core_node NULL\n", __func__);
		return;
	}
	/*read parade report_rate_check_supported*/
	value = parade_get_dts_value(core_node, "parade,report_rate_support");
	if(value <= CY_REPORT_RATE_UNSUP){
		tskit_parade_data->report_rate_check_supported = CY_REPORT_RATE_UNSUP;
	} else {
		tskit_parade_data->report_rate_check_supported = CY_REPORT_RATE_SUP;
	}
	TS_LOG_INFO("%s, tskit_parade_data->report_rate_check_supported = %d.\n", __func__, value);
}

void parse_self_noise_support(struct device_node *core_node)
{
	int value = 0;

	if(NULL == tskit_parade_data) {
		TS_LOG_ERR("%s tskit_parade_data's NULL\n", __func__);
		return;
	}

	if(NULL == core_node) {
		tskit_parade_data->self_noise_supported = CY_SELF_NOISE_UNSUP;
		TS_LOG_ERR("%s value or core_node's NULL\n", __func__);
		return;
	}

	/* read parade self_noise_support */
	value = parade_get_dts_value(core_node, "parade,self_noise_support");
	if(value <= CY_SELF_NOISE_UNSUP){
		tskit_parade_data->self_noise_supported = CY_SELF_NOISE_UNSUP;
	} else {
		tskit_parade_data->self_noise_supported = CY_SELF_NOISE_SUP;
	}
	TS_LOG_INFO("%s, tskit_parade_data->self_noise_supported = %d.\n", __func__, value);
}

void parse_check_bulcked(struct device_node *core_node, struct ts_kit_device_data *chip_data)
{
	int value = 0;

	if(NULL == core_node || NULL == chip_data) {
		TS_LOG_ERR("%s, core_node or chip_data is NULL\n", __func__);
		return;
	}

	value = parade_get_dts_value(core_node, "parade,check_bulcked");
	if(value <= CY_CHECK_BULCKED_FALSE){
		chip_data->check_bulcked = CY_CHECK_BULCKED_FALSE;
	} else {
		chip_data->check_bulcked = CY_CHECK_BULCKED_TRUE;
	}
	TS_LOG_INFO("%s, is_parade_solution value=%d\n", __func__, value);
}

void parse_check_irq_state(struct device_node *core_node)
{
	int value = 0;

	if(NULL == core_node || NULL == tskit_parade_data) {
		TS_LOG_ERR("%s, core_node or tskit_parade_data is NULL\n", __func__);
		return;
	}
	if(NULL == tskit_parade_data->core_pdata) {
		TS_LOG_ERR("%s, tskit_parade_data->core_pdata is NULL\n", __func__);
		return;
	}

	value = parade_get_dts_value(core_node, "parade,check_irq_state");
	if(value <= CY_CHECK_IRQ_STATE_FALSE){
		tskit_parade_data->core_pdata->check_irq_state= CY_CHECK_IRQ_STATE_FALSE;
	} else {
		tskit_parade_data->core_pdata->check_irq_state= CY_CHECK_IRQ_STATE_TRUE;
	}
	TS_LOG_INFO("%s, check_irq_state value=%d\n", __func__, value);
}


static void  parade_parse_lcd_parameter(struct device_node *device)
{

	int value = 0;
	struct device_node *core_node = device;
	/*read parade,need_distinguish_lcd*/

	value = parade_get_dts_value(core_node, "parade,need_distinguish_lcd");
	if(value <=  0){
		tskit_parade_data->need_distinguish_lcd = false;
	} else {
		tskit_parade_data->need_distinguish_lcd = true;
	}
	TS_LOG_INFO("%s, need_distinguish_lcd =%d\n", __func__,value);

	value = parade_get_dts_value(core_node, "parade,fw_only_depend_on_lcd");
	if(value <=  0){
		tskit_parade_data->fw_only_depend_on_lcd= false;
	} else {
		tskit_parade_data->fw_only_depend_on_lcd = true;
	}
	TS_LOG_INFO("%s, need_get_lcd_module =%d\n", __func__,value);

	return;
}

void parade_parse_after_resume(struct device_node *core_node)
{
	if(NULL == core_node || NULL == tskit_parade_data) {
		TS_LOG_ERR("%s: param null, err\n");
		return;
	}

	tskit_parade_data->no_need_after_resume_flag = parade_get_dts_value(core_node, "parade,no_need_doing_after_resume");
	if(tskit_parade_data->no_need_after_resume_flag < 0) {
		tskit_parade_data->no_need_after_resume_flag = 0;
	}
	TS_LOG_INFO("%s, no_need_after_resume_flag = %d\n", __func__, tskit_parade_data->no_need_after_resume_flag);
}

void parade_parse_calibrate(struct device_node *core_node)
{
	if(NULL == core_node || NULL == tskit_parade_data) {
		TS_LOG_ERR("%s: param null, err\n");
		return;
	}

	tskit_parade_data->calibrate_fw_update_and_cmcp = parade_get_dts_value(core_node, "parade,calibrate_fw_update_and_cmcp");
	if(tskit_parade_data->calibrate_fw_update_and_cmcp < 0) {
		tskit_parade_data->calibrate_fw_update_and_cmcp = 0;
	}
	TS_LOG_INFO("%s, calibrate_fw_update_and_cmcp = %d\n", __func__, tskit_parade_data->calibrate_fw_update_and_cmcp);
}

void parse_before_suspend_flag(struct device_node *core_node)
{
	if(NULL == core_node || NULL == tskit_parade_data) {
		TS_LOG_ERR("%s: param null, err\n");
		return;
	}

	tskit_parade_data->show_before_suspend_flag = parade_get_dts_value(core_node, "parade,show_before_suspend_flag");
	if(tskit_parade_data->show_before_suspend_flag < 0) {
		tskit_parade_data->show_before_suspend_flag = 0;
	}
	TS_LOG_INFO("%s, show_before_suspend_flag = %d\n", __func__, tskit_parade_data->show_before_suspend_flag);
}

void parse_check_img_vers(struct device_node *core_node, struct ts_kit_device_data *chip_data)
{
	int value = 0;

	if(NULL == core_node || NULL == chip_data) {
		TS_LOG_ERR("%s, core_node or chip_data is NULL\n", __func__);
		return;
	}

	value = parade_get_dts_value(core_node, "check_fw_right_flag");
	if (value < 0) {
		TS_LOG_INFO("%s, get device check_fw_right_flag failed\n ", __func__);
		chip_data->check_fw_right_flag = 0;
	}else{
		chip_data->check_fw_right_flag = value;
	}

	TS_LOG_INFO("%s, check_fw_right_flag = %d \n", __func__, chip_data->check_fw_right_flag);
}

void parse_need_set_rst_after_iovcc_flag(struct device_node *core_node)
{
	int value = 0;

	if(NULL == core_node || NULL == tskit_parade_data) {
		TS_LOG_ERR("%s, core_node or chip_data is NULL\n", __func__);
		return;
	}

	value = parade_get_dts_value(core_node, "need_set_rst_after_iovcc_flag");
	if (value < 0) {
		TS_LOG_INFO("%s, get device need_set_rst_after_iovcc_flag failed\n ", __func__);
		tskit_parade_data->need_set_rst_after_iovcc_flag = false;
	}else{
		tskit_parade_data->need_set_rst_after_iovcc_flag = true;
	}

	TS_LOG_INFO("%s, need_set_rst_after_iovcc_flag = %d \n", __func__, tskit_parade_data->need_set_rst_after_iovcc_flag);
}

static int hw_get_dts_value(struct device_node *core_node, char *name)
{
	if(NULL == core_node || NULL == name) {
		TS_LOG_INFO("%s: param NULL\n", __func__);
		return -EINVAL;
	}

	int retval = NO_ERR;

	retval = parade_get_dts_value(core_node , name);
	if (retval < 0) {
		TS_LOG_INFO
		    ("%s, get device %s failed\n",__func__, name);
		retval = 0; //default
	}

	return retval;
}

void provide_projectID_for_sensor(struct device_node *core_node, struct ts_kit_device_data *chip_data)
{
	int value = 0;

	if(NULL == core_node || NULL == chip_data) {
		TS_LOG_ERR("%s, core_node or chip_data is NULL\n", __func__);
		return;
	}

	/*  need provoid projectid for sensor or not  flag */
	value = parade_get_dts_value(core_node , "need_provide_projectID_for_sensor");
	if (value < 0) {
		TS_LOG_INFO("%s, get device need_provide_projectID_for_sensor failed\n",__func__);
		value = 0; //default
	}
	tskit_parade_data->need_provide_projectID_for_sensor= value;
	TS_LOG_INFO("%s, need_provide_projectID_for_sensor = %d \n", __func__, tskit_parade_data->need_provide_projectID_for_sensor);
}

void parse_need_check_report_descriptor_flag(struct device_node *core_node, struct ts_kit_device_data *chip_data)
{
	 int value = 0;

	 if(NULL == core_node || NULL == chip_data || !tskit_parade_data) {
		 TS_LOG_ERR("%s, core_node or chip_data is NULL\n", __func__);
		 return;
	 }

	 /*  need check report descriptor  flag */
	 value = parade_get_dts_value(core_node , "need_check_report_descriptor_flag");
	 if (value < 0) {
		 TS_LOG_INFO("%s, get device need_check_report_descriptor_flag failed\n",__func__);
		 value = 0; //default
	 }
	 tskit_parade_data->need_check_report_descriptor_flag = value;
	 TS_LOG_INFO("%s, need_check_report_descriptor_flag = %d \n", __func__, tskit_parade_data->need_check_report_descriptor_flag);
 }

void parade_parse_fw_need_depend_on_lcd(struct device_node *core_node, struct ts_kit_device_data *chip_data)
{
	int value = 0;

	if(NULL == core_node || NULL == chip_data || !tskit_parade_data) {
		TS_LOG_ERR("%s, core_node or chip_data is NULL\n", __func__);
		return;
	}

	/*  fw need depend on lcd module */
	value = parade_get_dts_value(core_node , "fw_need_depend_on_lcd");
	if (value < 0) {
		TS_LOG_INFO("%s, get device fw_need_depend_on_lcd failed\n",__func__);
		value = 0; //default
	}
	tskit_parade_data->fw_need_depend_on_lcd = value;
	TS_LOG_INFO("%s, fw_need_depend_on_lcd = %d \n", __func__, tskit_parade_data->fw_need_depend_on_lcd);
}

void parade_parse_lcd_panel_name_from_lcdkit(struct device_node *core_node, struct ts_kit_device_data *chip_data)
{
	int value = 0;

	if(NULL == core_node || NULL == chip_data || !tskit_parade_data) {
		TS_LOG_ERR("%s, core_node or chip_data is NULL\n", __func__);
		return;
	}

	/*lcd_panel_name_from_lcdkit or not*/
	value = parade_get_dts_value(core_node, "parade,lcd_panel_name_from_lcdkit");
	if(value < 0){
		value = 0; //default support
	}
	tskit_parade_data->lcd_panel_name_from_lcdkit = value;
	TS_LOG_INFO("%s lcd_panel_name_from_lcdkit = %d\n", __func__, tskit_parade_data->lcd_panel_name_from_lcdkit);

}

void parade_need_delay_after_power_off(struct device_node *core_node, struct ts_kit_device_data *chip_data)
{
    int value = 0;

    if(NULL == core_node || NULL == chip_data || !tskit_parade_data) {
        TS_LOG_ERR("%s, core_node or chip_data is NULL\n", __func__);
        return;
    }

    /* check if need delay after power off*/
    value = parade_get_dts_value(core_node , "parade,need_delay_after_power_off");
    if (value < 0) {
        TS_LOG_INFO("%s, get device need_delay_after_power_off failed\n",__func__);
        value = 0; //default
    }
    tskit_parade_data->need_delay_after_power_off = value;
    TS_LOG_INFO("%s, need_delay_after_power_off = %d \n", __func__, tskit_parade_data->need_delay_after_power_off);
}

 int parade_parse_dts(struct device_node *device, struct ts_kit_device_data *chip_data)
 {
	if(NULL == device || NULL == chip_data) {
		TS_LOG_INFO("%s: param NULL\n", __func__);
		return -EINVAL;
	}
	int retval = NO_ERR;
	int value = 0;
	int rc = NO_ERR;
	struct parade_core_platform_data *core_pdata = NULL;
	struct parade_mt_platform_data *mt_pdata = NULL;
	struct parade_loader_platform_data *loader_pdata = NULL;
	struct device_node *core_node = device;
	TS_LOG_INFO("%s, parade parse config called\n", __func__);
	//memset(tskit_parade_data, 0, sizeof(parade_core_data));
	core_pdata = kzalloc(sizeof(*core_pdata), GFP_KERNEL);
	if (!core_pdata) {
		rc = -ENOMEM;
		goto fail_free;
	}
	mt_pdata = kzalloc(sizeof(*mt_pdata), GFP_KERNEL);
	if (!mt_pdata) {
		rc = -ENOMEM;
		goto fail_free;
	}
	loader_pdata = kzalloc(sizeof(*loader_pdata), GFP_KERNEL);
	if (!loader_pdata) {
		rc = -ENOMEM;
		goto fail_free;
	}

	tskit_parade_data->core_pdata = core_pdata;
	tskit_parade_data->mt_pdata = mt_pdata;
	tskit_parade_data->loader_pdata = loader_pdata;
	tskit_parade_data->core_pdata->irq_stat = parade_irq_stat;

	tskit_parade_data->mt_pdata->flags = 0;
	tskit_parade_data->md.si = &(tskit_parade_data->sysinfo);
	tskit_parade_data->vddd = NULL;
	tskit_parade_data->vdda = NULL;

	TS_LOG_DEBUG("%s: parameter init begin\n", __func__);

	mutex_init(&tskit_parade_data->system_lock);
	mutex_init(&tskit_parade_data->hid_report_lock);
	spin_lock_init(&tskit_parade_data->spinlock);
	INIT_LIST_HEAD(&tskit_parade_data->param_list);
	TS_LOG_DEBUG("%s: parameter init end\n", __func__);

	/* Initialize wait queue */
	init_waitqueue_head(&tskit_parade_data->wait_q);
	init_waitqueue_head(&tskit_parade_data->wait_q_bootup);

	mutex_init(&tskit_parade_data->dad.sysfs_lock);
	mutex_init(&chip_data->device_call_lock);

	tskit_parade_data->parade_chip_data->ts_platform_data->client->addr = parade_get_dts_value(core_node , "slave_address");
	TS_LOG_INFO("%s,client->addr = %d.\n", __func__, tskit_parade_data->parade_chip_data->ts_platform_data->client->addr);
	/*hid_desc_register*/
	core_pdata->hid_desc_register = parade_get_dts_value(core_node, "parade,hid_desc_register");
	if(core_pdata->hid_desc_register < 0)
		goto fail_free;
	TS_LOG_INFO("%s,core_pdata->hid_desc_register = %d.\n", __func__, core_pdata->hid_desc_register);

	/*easy_wakeup_supported_gestures*/
	value = parade_get_dts_value(core_node, "parade,easy_wakeup_supported_gestures");
	if(value < 0)
		value = 0;
	core_pdata->wakeup_gesture_enable = value;
	TS_LOG_INFO("%s,core_pdata->wakeup_gesture_enable = %d.\n", __func__, core_pdata->wakeup_gesture_enable);
	tskit_parade_data->parade_chip_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value = core_pdata->wakeup_gesture_enable;

	/*provide panel_id supported for sensor*/
	value = parade_get_dts_value(core_node, "provide_panel_id_suppot");
	if (value < 0)
		value = 0;
	chip_data->provide_panel_id_support = value;
	TS_LOG_INFO("%s,provide_panel_id_suppot = %d.\n", __func__, chip_data->provide_panel_id_support);

	/*get roi_supported value*/
#ifdef PARADE_ROI_ENABLE
	tskit_parade_data->parade_chip_data->ts_platform_data->feature_info.roi_info.roi_supported = parade_get_dts_value(core_node, "roi_supported");
	TS_LOG_INFO("%s roi_supported = %d\n", __func__, tskit_parade_data->parade_chip_data->ts_platform_data->feature_info.roi_info.roi_supported);
#else
	tskit_parade_data->parade_chip_data->ts_platform_data->feature_info.roi_info.roi_supported = 0;
#endif
	/* get holster mode value */
	value = 0;
	value = parade_get_dts_value(core_node, "holster_mode_supported");
	if(value < 0){
		value = 1; //default support
	}
	tskit_parade_data->parade_chip_data->ts_platform_data->feature_info.holster_info.holster_supported = value;
	TS_LOG_INFO("%s holster_supported = %d\n", __func__, tskit_parade_data->parade_chip_data->ts_platform_data->feature_info.holster_info.holster_supported);

	parade_get_cp_delta_test_support(core_node);
	TS_LOG_INFO("%s,cp_delta_test_supported = %d.\n", __func__, tskit_parade_data->cp_delta_test_supported);

	/* get glove mode value */
	value = 0;
	value = parade_get_dts_value(core_node, "glove_mode_supported");
	if(value < 0){
		value = 1; //default support
	}
	tskit_parade_data->parade_chip_data->ts_platform_data->feature_info.glove_info.glove_supported = value;
	TS_LOG_INFO("%s glove_supported = %d\n", __func__, tskit_parade_data->parade_chip_data->ts_platform_data->feature_info.glove_info.glove_supported);

	/*get should_check_tp_calibration_info value*/
	tskit_parade_data->parade_chip_data->should_check_tp_calibration_info =0; //TODO: TEST ONLY//.= parade_get_dts_value(core_node, "should_check_tp_calibration_info");
	TS_LOG_INFO("%s should_check_tp_calibration_info = %d\n", __func__, tskit_parade_data->parade_chip_data->should_check_tp_calibration_info);

	/*read core_flag*/
	core_pdata->flags = parade_get_dts_value(core_node, "parade,core_flags");
	if(core_pdata->flags < 0)
		goto fail_free;
	TS_LOG_INFO("%s,core_pdata->flags = %d.\n", __func__, core_pdata->flags);

	/*read mt_flag*/
	mt_pdata->flags = parade_get_dts_value(core_node, "parade,mt_flags");
	if(mt_pdata->flags < 0)
		goto fail_free;
	TS_LOG_INFO("%s,mt_pdata->flags = %d.\n", __func__, mt_pdata->flags);

	/*read irq_config*/
	core_pdata->irq_config = parade_get_dts_value(core_node, "parade,irq_config");
	if(core_pdata->irq_config < 0)
		goto fail_free;
	TS_LOG_INFO("%s,core_pdata->irq_config = %d.\n", __func__, core_pdata->irq_config);

	/*read abs_max_t*/
	tskit_parade_data->md.t_max = parade_get_dts_value(core_node, "parade,abs_max_t");
	if(tskit_parade_data->md.t_max < 0)
		goto fail_free;
	TS_LOG_DEBUG("%s,tskit_parade_data->md.t_max = %d.\n", __func__, tskit_parade_data->md.t_max);

	/*read abs_min_t*/
	tskit_parade_data->md.t_min = parade_get_dts_value(core_node, "parade,abs_min_t");
	if(tskit_parade_data->md.t_min < 0)
		goto fail_free;
	TS_LOG_DEBUG("%s,tskit_parade_data->md.t_min = %d.\n", __func__, tskit_parade_data->md.t_min);

	/*read abs_min_or*/
	tskit_parade_data->md.or_min = parade_get_dts_value(core_node, "parade,abs_min_or");
	if(tskit_parade_data->md.or_min < 0)
		goto fail_free;
	TS_LOG_DEBUG("%s,tskit_parade_data->md.abs_min_or = %d.\n", __func__, tskit_parade_data->md.or_min);

	/*read abs_max_or*/
	tskit_parade_data->md.or_max = parade_get_dts_value(core_node, "parade,abs_max_or");
	if(tskit_parade_data->md.or_max < 0)
		goto fail_free;
	TS_LOG_DEBUG("%s,tskit_parade_data->md.or_max = %d.\n", __func__, tskit_parade_data->md.or_max);

	/*read p_min*/
	tskit_parade_data->md.p_min = parade_get_dts_value(core_node, "parade,abs_min_p");
	if(tskit_parade_data->md.p_min < 0)
		goto fail_free;
	TS_LOG_DEBUG("%s,tskit_parade_data->md.p_min = %d.\n", __func__, tskit_parade_data->md.p_min);

	/*read p_max*/
	tskit_parade_data->md.p_max = parade_get_dts_value(core_node, "parade,abs_max_p");
	if(tskit_parade_data->md.p_max < 0)
		goto fail_free;
	TS_LOG_DEBUG("%s,tskit_parade_data->md.p_max = %d.\n", __func__, tskit_parade_data->md.p_max);

	/*read abs_min_mj*/
	tskit_parade_data->md.major_min = parade_get_dts_value(core_node, "parade,abs_min_mj");
	if(tskit_parade_data->md.major_min < 0)
		goto fail_free;
	TS_LOG_DEBUG("%s,tskit_parade_data->md.major_min = %d.\n", __func__, tskit_parade_data->md.major_min);

	/*read abs_max_mj*/
	tskit_parade_data->md.major_max = parade_get_dts_value(core_node, "parade,abs_max_mj");
	if(tskit_parade_data->md.major_max < 0)
		goto fail_free;
	TS_LOG_DEBUG("%s,tskit_parade_data->md.major_max = %d.\n", __func__, tskit_parade_data->md.major_max);

	/*read abs_min_mn*/
	tskit_parade_data->md.minor_min= parade_get_dts_value(core_node, "parade,abs_min_mn");
	if(tskit_parade_data->md.minor_min < 0)
		goto fail_free;
	TS_LOG_DEBUG("%s,tskit_parade_data->md.minor_min = %d.\n", __func__, tskit_parade_data->md.minor_min);

	/*read abs_max_mn*/
	tskit_parade_data->md.minor_max = parade_get_dts_value(core_node, "parade,abs_max_mn");
	if(tskit_parade_data->md.minor_max < 0)
		goto fail_free;
	TS_LOG_DEBUG("%s,tskit_parade_data->md.minor_max = %d.\n", __func__, tskit_parade_data->md.minor_max);

	/*read abs_min_x*/
	tskit_parade_data->md.x_min = parade_get_dts_value(core_node, "parade,abs_min_x");
	if(tskit_parade_data->md.x_min < 0)
		goto fail_free;
	TS_LOG_DEBUG("%s,tskit_parade_data->md.x_min = %d.\n", __func__, tskit_parade_data->md.x_min);

	/*read abs_max_x*/
	tskit_parade_data->md.x_max = parade_get_dts_value(core_node, "parade,abs_max_x");
	if(tskit_parade_data->md.x_max < 0)
		goto fail_free;

	/*read abs_min_y*/
	tskit_parade_data->md.y_min = parade_get_dts_value(core_node, "parade,abs_min_y");
	if(tskit_parade_data->md.y_min < 0)
		goto fail_free;
	TS_LOG_DEBUG("%s,tskit_parade_data->md.y_min = %d.\n", __func__, tskit_parade_data->md.y_min);

	/*read abs_max_y*/
	tskit_parade_data->md.y_max = parade_get_dts_value(core_node, "parade,abs_max_y");
	if(tskit_parade_data->md.y_max < 0)
		goto fail_free;

	parade_parse_lcd_panel_name_from_lcdkit(core_node, chip_data);

	/* need_surport_second_distinguishability or not*/
	value = parade_get_dts_value(core_node, "parade,need_surport_second_distinguishability");
	if(value == SUPPORT_DIFF_DISTINGUISHABILITY){
		rc = of_property_read_string(core_node, "parade,sign_of_second_distinguishability", (const char**)&(tskit_parade_data->sign_of_second_distinguishability));
		if (rc){
			TS_LOG_ERR("%s, parade,sign_of_second_distinguishability read failed:%d\n",__func__ , rc);
			goto fail_free;
		}
		TS_LOG_INFO("%s,sign_of_second_distinguishability: %s. \n",__func__, tskit_parade_data->sign_of_second_distinguishability);
		get_current_distinguishability();
		if(tskit_parade_data->current_distinguishability){
			tskit_parade_data->md.x_max = parade_get_dts_value(core_node, "parade,abs_max_x_of_second_distinguishability");
			if(tskit_parade_data->md.x_max < 0){
				TS_LOG_ERR("%s, tskit_parade_data->md.x_max is illegal\n",__func__);
				goto fail_free;
			}
			tskit_parade_data->md.y_max = parade_get_dts_value(core_node, "parade,abs_max_y_of_second_distinguishability");
			if(tskit_parade_data->md.y_max < 0){
				TS_LOG_ERR("%s, tskit_parade_data->md.y_max is illegal\n",__func__);
				goto fail_free;
			}
		}
	}
	TS_LOG_DEBUG("%s,tskit_parade_data->md.x_max = %d.tskit_parade_data->md.y_max = %d.\n", __func__, tskit_parade_data->md.x_max, tskit_parade_data->md.y_max);

	parse_report_rate_check_support(core_node);
	parse_self_noise_support(core_node);
	parse_check_bulcked(core_node, chip_data);
	parse_check_irq_state(core_node);
	parse_before_suspend_flag(core_node);
	parse_need_set_rst_after_iovcc_flag(core_node);
	parade_parse_fw_need_depend_on_lcd(core_node, chip_data);
	parade_need_delay_after_power_off(core_node, chip_data);

	/*read is parade solution*/
	value = 0;
	value = parade_get_dts_value(core_node, "parade,is_parade_solution");
	if(value < 0){
		goto fail_free;
	} else if(value == 0){
		chip_data->is_parade_solution = false;
	} else {
		chip_data->is_parade_solution = true;
	}
	TS_LOG_INFO("%s, is_parade_solution value=%d\n", __func__,value);

	/*read algo_id*/
	value = parade_get_dts_value(core_node, "algo_id");
	if(value < 0){
		TS_LOG_INFO("%s, algo_id read fail, keep intact\n", __func__);
	} else {
		chip_data->algo_id = value;
	}
	TS_LOG_INFO("%s, algo_id value=%d\n", __func__,value);

	/*read direct_proc_cmd_thread*/
	value = 0;
	value = parade_get_dts_value(core_node, "parade,direct_proc_cmd");
	if(value < 0){
		goto fail_free;
	} else if(value == 0){
		chip_data->is_direct_proc_cmd = false;
	} else {
		chip_data->is_direct_proc_cmd = true;
	}
	TS_LOG_INFO("%s, direct_proc_cmd force set to value=%d\n", __func__,value);

	/*read parade,need_upgrade_again*/
	value = 0;
	value = parade_get_dts_value(core_node, "parade,need_upgrade_again");
	if(value <=  0){
		tskit_parade_data->need_upgrade_again = false;
	} else {
		tskit_parade_data->need_upgrade_again = true;
	}
	TS_LOG_INFO("%s, need_upgrade_again =%d\n", __func__,value);

	 parade_parse_lcd_parameter(core_node);

	/*for cm test,some waiting is not enough*/
	value = 0;
	value = parade_get_dts_value(core_node, "parade,need_wait_after_cm_test");
	if(value <=  0){
		tskit_parade_data->need_wait_after_cm_test = false;
	} else {
		tskit_parade_data->need_wait_after_cm_test = true;
	}
	TS_LOG_INFO("%s, need_wait_after_cm_test = %d\n", __func__,value);

	/*read parade,cy_hid_output_timeout*/
	value = 0;
	value = parade_get_dts_value(core_node, "parade,cy_hid_output_timeout");
	if(value < 0){
		tskit_parade_data->cy_hid_output_timeout = CY_HID_OUTPUT_TIMEOUT;
	} else {
		tskit_parade_data->cy_hid_output_timeout = value;
	}
	TS_LOG_INFO("%s, cy_hid_output_timeout =%d\n", __func__,value);

	/*read cy_hid_reset_timeout*/
	value = 0;
	value = parade_get_dts_value(core_node, "cy_hid_reset_timeout");
	if(value < 0){
		tskit_parade_data->cy_hid_reset_timeout = CY_HID_RESET_TIMEOUT;
	} else {
		tskit_parade_data->cy_hid_reset_timeout = value;
	}
	TS_LOG_INFO("%s, cy_hid_reset_timeout =%d\n", __func__,value);

	/*read i2c_one_byte*/
	value = 0;
	value = parade_get_dts_value(core_node, "parade,i2c_one_byte");
	if(value < 0){
		goto fail_free;
	} else if(value == 0){
		chip_data->is_i2c_one_byte = false;
	} else {
		chip_data->is_i2c_one_byte = true;
	}
	TS_LOG_INFO("%s, is_i2c_one_byte value=%d\n", __func__,value);

	/*read use_new_oem_structure*/
	value = 0;
	value = parade_get_dts_value(core_node, "parade,use_new_oem_structure");
	if(value < 0){
		goto fail_free;
	} else if(value == 0){
		chip_data->is_new_oem_structure = false;
	} else {
		chip_data->is_new_oem_structure = true;
	}
	TS_LOG_INFO("%s, se_new_oem_structure value=%d\n", __func__,value);

	/*no need doing something after resume*/
	parade_parse_after_resume(core_node);

	/*parade_parse_calibrate_for_gff*/
	parade_parse_calibrate(core_node);

	/*read sleep in mode*/
	tskit_parade_data->sleep_in_mode = parade_get_dts_value(core_node, "parade,sleep_in_mode");
	if(tskit_parade_data->sleep_in_mode < 0){
		goto fail_free;
	}
	g_ts_kit_platform_data.chip_data->sleep_in_mode = tskit_parade_data->sleep_in_mode;
	TS_LOG_INFO("%s, chip sleep in mode %d \n", __func__, tskit_parade_data->sleep_in_mode);
	/*power_self_ctrl*/
	tskit_parade_data->self_ctrl_power = parade_get_dts_value(core_node, "power_self_ctrl");
	if (tskit_parade_data->self_ctrl_power < 0) {
		goto fail_free;
	}
	TS_LOG_INFO("%s, power self ctrl = %d \n", __func__, tskit_parade_data->self_ctrl_power);
	/* need wd check status */
	value = parade_get_dts_value(core_node, "need_wd_check_status");
	if(value < 0){
		value = 0; //defalut
	}
	g_ts_kit_platform_data.chip_data->need_wd_check_status	 = value;
	TS_LOG_INFO("%s, need_wd_check_status %d \n", __func__, g_ts_kit_platform_data.chip_data->need_wd_check_status);

	/*vci_ldo_value*/
	chip_data->regulator_ctr.vci_value = parade_get_dts_value(core_node, "vci_ldo_value");
	if (chip_data->regulator_ctr.vci_value < 0) {
		TS_LOG_ERR("get vci_ldo_value failed\n");
	}
	TS_LOG_INFO("%s, regulator_ctr.vci_value = %d \n", __func__, chip_data->regulator_ctr.vci_value);

	/*vddio_ldo_need_set*/
	chip_data->regulator_ctr.need_set_vddio_value = parade_get_dts_value(core_node, "vddio_ldo_need_set");
	if (chip_data->regulator_ctr.need_set_vddio_value < 0) {
		goto fail_free;
	}
	TS_LOG_INFO("%s, regulator_ctr.need_set_vddio_value = %d \n", __func__, chip_data->regulator_ctr.need_set_vddio_value);
	if(chip_data->regulator_ctr.need_set_vddio_value){
		/*vddio_ldo_value*/
		chip_data->regulator_ctr.vddio_value = parade_get_dts_value(core_node, "vddio_ldo_value");
		if (chip_data->regulator_ctr.vddio_value < 0) {
			goto fail_free;
		}
		TS_LOG_INFO("%s, regulator_ctr.vddio_value = %d \n", __func__, chip_data->regulator_ctr.vddio_value);
	}
	/*vci_power_type*/
	chip_data->vci_regulator_type = parade_get_dts_value(core_node, "vci_power_type");
	if (chip_data->vci_regulator_type < 0) {
		goto fail_free;
	}
	TS_LOG_INFO("%s, regulator_ctr.vci_regulator_type = %d \n", __func__, chip_data->vci_regulator_type);
#ifdef PARADE_DOZE_ENABLE
	/*touch switch flag*/
	value = parade_get_dts_value(core_node, "touch_switch_flag");
	if (value < 0) {
		goto fail_free;
	}
	chip_data->touch_switch_flag |= value;
	TS_LOG_INFO("%s, touch_switch_flag = %d \n", __func__, chip_data->touch_switch_flag);
#endif
	/* get tp color flag */
	value = parade_get_dts_value(core_node, "support_get_tp_color");
	if (value < 0) {
		TS_LOG_DEBUG("%s, get device support_get_tp_color failed, will use default value: 0 \n ", __func__);
		value = 0; //default 0: no need know tp color
	}
	tskit_parade_data->support_get_tp_color = value;
	TS_LOG_INFO("%s, support_get_tp_color = %d \n", __func__, tskit_parade_data->support_get_tp_color);

	/*get brightness info flag*/
	value = parade_get_dts_value(core_node, "get_brightness_info_flag");
	if (value < 0) {
		TS_LOG_DEBUG("%s, get device get_brightness_info_flag failed, will use default value: 1 \n ", __func__);
		value = 1;
	}
	chip_data->get_brightness_info_flag |= value;
	TS_LOG_INFO("%s, get_brightness_info_flag = %d \n", __func__, chip_data->get_brightness_info_flag);

	parse_check_img_vers(core_node,chip_data);

	if(tskit_parade_data->self_ctrl_power == 1) { // power by tp will need get power or gpio type
		/*vddio power type*/
		retval = of_property_read_u32(device, "vddio_power_type",
					 &chip_data->vddio_regulator_type);
		if (retval) {
			TS_LOG_ERR
			    ("%s,get device vddio_power_type failed\n",__func__);
		}
		/*vddio gpio type*/
		retval = of_property_read_u32(device, "vddio_gpio_type",
					 &chip_data->vddio_gpio_type);
		if (retval) {
			TS_LOG_ERR
			    ("%s,get device vddio_gpio_type failed\n",__func__);
		}
	}
	/* pinctrl set */
	retval = parade_get_dts_value(core_node , "parade,pinctrl_set");
	if (retval < 0) {
		TS_LOG_DEBUG
		    ("%s, get device pinctrl_set failed\n",__func__);
		retval = 0; //default
	}
	tskit_parade_data->pinctrl_set = retval;
	TS_LOG_INFO("%s, pinctrl_set = %d \n", __func__, tskit_parade_data->pinctrl_set);
	/* get module name flag */
	retval = parade_get_dts_value(core_node , "get_module_name_flag");
	if (retval < 0) {
		TS_LOG_DEBUG
		    ("%s, get device get_module_name_flag failed\n",__func__);
		retval = 0; //default
	}
	tskit_parade_data->get_module_name_flag = retval;
	TS_LOG_DEBUG("%s, get_module_name_flag = %d \n", __func__, tskit_parade_data->get_module_name_flag);
	/* create project id flag */
	retval = parade_get_dts_value(core_node , "create_project_id_flag");
	if (retval < 0) {
		TS_LOG_DEBUG
		    ("%s, get device create_project_id_flag failed\n",__func__);
		retval = 0; //default
	}
	tskit_parade_data->create_project_id_flag = retval;
	TS_LOG_DEBUG("%s, create_project_id_flag = %d \n", __func__, tskit_parade_data->create_project_id_flag);

	provide_projectID_for_sensor(core_node,chip_data);
	parse_need_check_report_descriptor_flag(core_node,chip_data);

	/* TX/RX delta lattice flag */
	retval = hw_get_dts_value(core_node , "cm_delta_lattice_flag");
	tskit_parade_data->cm_delta_lattice_flag = retval;
	TS_LOG_INFO("%s, cm_delta_lattice_flag = %d \n", __func__, tskit_parade_data->cm_delta_lattice_flag);

	/*0 is power supplied by gpio, 1 is power supplied by ldo */
	if (0 == chip_data->vci_regulator_type) {
		chip_data->vci_gpio_ctrl = of_get_named_gpio(device, "vci_gpio_enable", 0);
		if (!gpio_is_valid(chip_data->vci_gpio_ctrl)) {
			TS_LOG_ERR
			    ("%s, Fail to get VCI control GPIO\n",__func__);
			goto fail_free;
		}
	}
	if (0 == chip_data->vddio_gpio_type) {
		chip_data->vddio_gpio_ctrl =
		    of_get_named_gpio(device, "vddio_gpio_enable", 0);
		if (!gpio_is_valid(chip_data->vddio_gpio_ctrl)) {
			TS_LOG_ERR
			    ("%s, Fail to get VDDIO control GPIO\n",__func__);
			goto fail_free;
		}
	}
	/*project id*/
	char *tmp_buff = NULL;
	rc = of_property_read_string(core_node, "project_id", (const char**)&tmp_buff);
    if (rc)
    {
       TS_LOG_ERR("%s, project_id read failed:%d\n",__func__ , rc);
       goto fail_free;
    }
	if(tmp_buff == NULL)
		goto fail_free;
	strncpy(tskit_parade_data->project_id,tmp_buff,MAX_STR_LEN-1);
	TS_LOG_INFO("%s, project_id %s\n",__func__ , tskit_parade_data->project_id);

	/*tp_test_type*/
	rc = of_property_read_string(core_node, "tp_test_type", (const char**)&tmp_buff);
	if (!rc) {
		strncpy(tskit_parade_data->parade_chip_data->tp_test_type, tmp_buff, TS_CAP_TEST_TYPE_LEN-1);
		TS_LOG_INFO("%s:tp_test_type =%s\n",__func__, tskit_parade_data->parade_chip_data->tp_test_type);
	}

	/*chip name*/
	rc = of_property_read_string(core_node, "chip_name", (const char**)&tmp_buff);
    if (rc)
    {
       TS_LOG_ERR("%s, chip_name read failed:%d\n",__func__ , rc);
       goto fail_free;
    }
	if(tmp_buff == NULL)
		goto fail_free;
	strncpy(tskit_parade_data->chip_name,tmp_buff,MAX_STR_LEN-1);
	strncpy(chip_data->chip_name,tmp_buff,MAX_STR_LEN-1);
	TS_LOG_INFO("%s, chip_name %s\n",__func__ , tskit_parade_data->chip_name);
	/*module vendor*/
	rc = of_property_read_string(core_node, "module_vendor", (const char**)&tmp_buff);
    if (rc)
    {
       TS_LOG_ERR("%s, module_vendor read failed:%d\n",__func__ , rc);
       goto fail_free;
    }
	if(tmp_buff == NULL)
		goto fail_free;
	strncpy(tskit_parade_data->module_vendor,tmp_buff,MAX_STR_LEN-1);
	TS_LOG_INFO("%s, module_vendor %s\n",__func__ , tskit_parade_data->module_vendor);
	tskit_parade_data->parade_chip_data->irq_config = core_pdata->irq_config;
	parade_pinctrl_get_init();
	return NO_ERR;

fail_free:
	TS_LOG_ERR("%s, DTS Parse Error, Chip Init Fail\n",__func__);
	if(core_pdata)
		kfree(core_pdata);
	if(mt_pdata)
		kfree(mt_pdata);
	if(loader_pdata)
		kfree(loader_pdata);
	return rc;
 }

static int parade_irq_top_half(struct ts_cmd_node *cmd)
{
	 cmd->command = TS_INT_PROCESS;
	 return NO_ERR;
}
/*lint -save -e* */
static int parade_read_input(struct parade_core_data *cd)
{
	u8 reg_addr = PAR_REG_BASE;
	int rc = 0;
	u32 size = 0;

	rc = parade_i2c_read(&reg_addr, sizeof(reg_addr), cd->input_buf, 2);
if (rc < 0) {
		TS_LOG_ERR("%s, fail read  rc=%d\n", __func__, rc);
		return rc;
	}

	size = get_unaligned_le16(&cd->input_buf[0]);
	if (!size)
		return 0;
	if (size == 2 || size > CY_MAX_INPUT){
		/* Before PIP 1.7, empty buffer is 0x0002;
		From PIP 1.7, empty buffer is 0xFFXX */
		TS_LOG_ERR("%s: input_buf is empty, inval size=%d\n",__func__,size);
		return -EINVAL;
	}
	rc = parade_i2c_read(&reg_addr, sizeof(reg_addr), cd->input_buf, size);
	if (rc < 0) {
		TS_LOG_ERR("%s, fail read  rc=%d\n", __func__, rc);
		return rc;
	}

	if(cd->force_wait) {
		memcpy(cd->response_buf, cd->input_buf, size);
		cyttsp5_pr_buf(cd->response_buf, size, "response_buf");
	}

    return rc;
}
/*lint -restore*/
static void parade_get_touch_axis(struct parade_mt_data *md,
    int *axis, int size, int max, u8 *data, int bofs)
{
    int nbyte;
    int next;

    for (nbyte = 0, *axis = 0, next = 0; nbyte < size; nbyte++) {
        *axis = *axis + ((data[next] >> bofs) << (nbyte * 8));
        next++;
    }

    *axis &= max - 1;
}

static void parade_get_touch_hdr(struct parade_mt_data *md,
	struct parade_touch *touch, u8 *xy_mode)
{

	struct parade_sysinfo *si = md->si;
	enum parade_tch_hdr hdr;

	for (hdr = CY_TCH_TIME; hdr < CY_TCH_NUM_HDR; hdr++) {
		if (!si->tch_hdr[hdr].report)
			continue;
		parade_get_touch_axis(md, &touch->hdr[hdr],
			si->tch_hdr[hdr].size,
			si->tch_hdr[hdr].max,
			xy_mode + si->tch_hdr[hdr].ofs,
			si->tch_hdr[hdr].bofs);
	}
	if(touch->hdr[CY_TCH_NOISE] > 1){
		TS_LOG_INFO("%s: time=%X tch_num=%d lo=%d noise=%d counter=%d\n",
		__func__,
		touch->hdr[CY_TCH_TIME],
		touch->hdr[CY_TCH_NUM],
		touch->hdr[CY_TCH_LO],
		touch->hdr[CY_TCH_NOISE],
		touch->hdr[CY_TCH_COUNTER]);
	} else {
		TS_LOG_DEBUG("%s: time=%X tch_num=%d lo=%d noise=%d counter=%d\n",
		__func__,
		touch->hdr[CY_TCH_TIME],
		touch->hdr[CY_TCH_NUM],
		touch->hdr[CY_TCH_LO],
		touch->hdr[CY_TCH_NOISE],
		touch->hdr[CY_TCH_COUNTER]);
	}
}

static void parade_get_touch_record(struct parade_mt_data *md,
	struct parade_touch *touch, u8 *xy_data)
{
	struct parade_sysinfo *si = md->si;
	enum parade_tch_abs abs;

	for (abs = CY_TCH_X; abs < CY_TCH_NUM_ABS; abs++) {
		if (!si->tch_abs[abs].report)
			continue;
		parade_get_touch_axis(md, &touch->abs[abs],
			si->tch_abs[abs].size,
			si->tch_abs[abs].max,
			xy_data + si->tch_abs[abs].ofs,
			si->tch_abs[abs].bofs);
		TS_LOG_DEBUG("%s: get %s=%04X(%d)\n", __func__,
			parade_tch_abs_string[abs],
			touch->abs[abs], touch->abs[abs]);
	}
}

static void parade_mt_process_touch(struct parade_mt_data *md,
	struct parade_touch *touch)
{
	struct parade_sysinfo *si = md->si;
	int tmp;
	bool flipped;

	/* Orientation is signed */
	touch->abs[CY_TCH_OR] = (int8_t)touch->abs[CY_TCH_OR];

	if (tskit_parade_data->mt_pdata->flags & CY_MT_FLAG_FLIP) {
		tmp = touch->abs[CY_TCH_X];
		touch->abs[CY_TCH_X] = touch->abs[CY_TCH_Y];
		touch->abs[CY_TCH_Y] = tmp;
		if (touch->abs[CY_TCH_OR] > 0)
			touch->abs[CY_TCH_OR] =
				md->or_max - touch->abs[CY_TCH_OR];
		else
			touch->abs[CY_TCH_OR] =
				md->or_min - touch->abs[CY_TCH_OR];
		flipped = true;
	} else
		flipped = false;

	if (tskit_parade_data->mt_pdata->flags & CY_MT_FLAG_INV_X) {
		if (flipped)
			touch->abs[CY_TCH_X] = si->sensing_conf_data.res_y -
				touch->abs[CY_TCH_X] - 1;
		else
			touch->abs[CY_TCH_X] = si->sensing_conf_data.res_x -
				touch->abs[CY_TCH_X] - 1;
		touch->abs[CY_TCH_OR] *= -1;
	}
	if (tskit_parade_data->mt_pdata->flags & CY_MT_FLAG_INV_Y) {
		if (flipped)
			touch->abs[CY_TCH_Y] = si->sensing_conf_data.res_x -
				touch->abs[CY_TCH_Y] - 1;
		else
			touch->abs[CY_TCH_Y] = si->sensing_conf_data.res_y -
				touch->abs[CY_TCH_Y] - 1;
		touch->abs[CY_TCH_OR] *= -1;
	}
}

static void parade_get_mt_touches(struct parade_mt_data *md,
		struct parade_touch *tch, int num_cur_tch, struct ts_fingers *info)
{
	struct parade_sysinfo *si = md->si;
	int i, t = 0;
	u8 *tch_addr;
	u8 touch_id;
	memset(tch->abs, 0, sizeof(tch->abs));
	info->cur_finger_number = num_cur_tch;
	for (i = 0; i < num_cur_tch; i++) {
		tch_addr = si->xy_data + (i * si->desc.tch_record_size);
		parade_get_touch_record(md, tch, tch_addr);

		/* Validate track_id */
		t = tch->abs[CY_TCH_T];
		if (t < md->t_min || t > md->t_max) {
			TS_LOG_ERR("%s: tch=%d -> bad trk_id=%d max_id=%d\n",
				__func__, i, t, md->t_max);
			continue;
		}
		if(t>=TS_MAX_FINGER){
			TS_LOG_ERR("%s: max finger id support is 10\n",
				__func__);
			continue;
		}

		/* Process touch */
		parade_mt_process_touch(md, tch);
		touch_id = tch->abs[CY_TCH_T];
		info->fingers[touch_id].event = tch->abs[CY_TCH_E];
		switch(tch->abs[CY_TCH_E]){ //PATCH: Huawei does not use our event, they use x=0 or y=0 as this id lift off event
			case CY_EV_LIFTOFF:
				info->fingers[touch_id].x = 0;
				info->fingers[touch_id].y = 0;
				break;
			case CY_EV_TOUCHDOWN:
			case CY_EV_NO_EVENT:
			case CY_EV_MOVE:
				info->fingers[touch_id].x = tch->abs[CY_TCH_X];
				info->fingers[touch_id].y = tch->abs[CY_TCH_Y];
				break;
			default:
				TS_LOG_ERR("%s: finger status: %d\n",
				__func__,tch->abs[CY_TCH_E]);
		}

		info->fingers[touch_id].pressure = tch->abs[CY_TCH_P];
	    switch(tch->abs[CY_TCH_O]) {/*Detect the object type*/
           case CY_OBJ_STANDARD_FINGER:
                   info->fingers[touch_id].status = TP_FINGER;
                  break;
           case CY_OBJ_GLOVE:
                   info->fingers[touch_id].status = TP_GLOVE;
                   break;
           case CY_OBJ_STYLUS:
                   info->fingers[touch_id].status = TP_STYLUS;
                   break;
	   }

		info->fingers[touch_id].ewx = tch->abs[CY_TCH_MAJ];
		info->fingers[touch_id].ewy = tch->abs[CY_TCH_MIN];
		info->fingers[touch_id].orientation = tch->abs[CY_TCH_OR];

	}

	md->num_prv_rec = num_cur_tch;
}

/* read xy_data for all current touches */
static int parade_xy_worker(struct parade_core_data *cd, struct ts_fingers *info, struct ts_cmd_node *out_cmd)
{
	//struct device *dev = cd->dev;
	struct parade_mt_data *md = &cd->md;
	struct parade_sysinfo *si = md->si;
	int max_tch = si->sensing_conf_data.max_tch;
	struct parade_touch tch;
	u8 num_cur_tch;
	int rc = 0;
	memset(&tch, 0, sizeof(struct parade_touch));
	parade_get_touch_hdr(md, &tch, si->xy_mode + 3);

	num_cur_tch = tch.hdr[CY_TCH_NUM];
	if (num_cur_tch > max_tch) {
		TS_LOG_ERR("%s: Num touch err detected (n=%d)\n",
			__func__, num_cur_tch);
		num_cur_tch = max_tch;
	}

	if (tch.hdr[CY_TCH_LO]) {
		TS_LOG_INFO("%s: Large area detected\n", __func__);
		if (cd->mt_pdata->flags & CY_MT_FLAG_NO_TOUCH_ON_LO)
			num_cur_tch = 0;
	}

	if (num_cur_tch == 0 && md->num_prv_rec == 0)
		goto parade_xy_worker_exit;

	/* extract xy_data for all currently reported touches */
	TS_LOG_DEBUG("%s: extract data num_cur_tch=%d\n", __func__,
		num_cur_tch);
	/*2016 12 23 Changes:
	  1. Fix the report protocol does not report all finger liftoff event
	  2. This will cause ZhouJin's filter illegal glove does not take effect
	  */
	parade_get_mt_touches(md, &tch, num_cur_tch, info);
	out_cmd->command = TS_INPUT_ALGO;
	out_cmd->cmd_param.pub_params.algo_param.algo_order = TS_ALGO_FUNC_0;

	rc = 0;

parade_xy_worker_exit:
	return rc;
}

static void parade_core_get_touch_axis(struct parade_core_data *cd,
	int *axis, int size, int max, u8 *data, int bofs)
{
	int nbyte;
	int next;

	for (nbyte = 0, *axis = 0, next = 0; nbyte < size; nbyte++) {
		*axis = *axis + ((data[next] >> bofs) << (nbyte * 8));
		next++;
	}

	*axis &= max - 1;
}

static int move_tracking_hetmap_data(struct parade_core_data *cd,
	struct parade_sysinfo *si)
{
#ifdef TTHE_TUNER_SUPPORT
	int size = get_unaligned_le16(&cd->input_buf[0]);

	if (size)
		tthe_print(cd, cd->input_buf, size, "THM=");
#endif
	memcpy(si->xy_mode, cd->input_buf, SENSOR_HEADER_SIZE);
	return 0;
}

static int move_sensor_data(struct parade_core_data *cd,
	struct parade_sysinfo *si)
{
#ifdef TTHE_TUNER_SUPPORT
	int size = get_unaligned_le16(&cd->input_buf[0]);

	if (size)
		tthe_print(cd, cd->input_buf, size, "sensor_monitor=");
#endif
	memcpy(si->xy_mode, cd->input_buf, SENSOR_HEADER_SIZE);
	return 0;
}

static int move_knuckle_data(struct parade_core_data *cd,
	struct parade_sysinfo *si)
{
#ifdef PARADE_ROI_ENABLE
	if(tskit_parade_data->roi_switch != 0) {
		TS_LOG_DEBUG("%s: HID_KNUCKLE_MODE_REPOR_ID Report\n", __func__);
		roi_data[0] = cd->input_buf[2];
		roi_data[1] = cd->input_buf[3];
		roi_data[2] = cd->input_buf[4];
		roi_data[3] = cd->input_buf[5];
		memcpy(&(roi_data[4]), &cd->input_buf[6], PARADE_ROI_DATA_SEND_LENGTH);
	}
#endif
	return 0;
}

static int move_button_data(struct parade_core_data *cd,
	struct parade_sysinfo *si)
{
#ifdef TTHE_TUNER_SUPPORT
	int size = get_unaligned_le16(&cd->input_buf[0]);

	if (size)
		tthe_print(cd, cd->input_buf, size, "OpModeData=");
#endif
	memcpy(si->xy_mode, cd->input_buf, BTN_INPUT_HEADER_SIZE);
	memcpy(si->xy_data, &cd->input_buf[BTN_INPUT_HEADER_SIZE],
			BTN_REPORT_SIZE);
	return 0;
}
static int move_touch_data(struct parade_core_data *cd,
    struct parade_sysinfo *si)
{
    int max_tch = si->sensing_conf_data.max_tch;
    int num_cur_tch;
    int length;
    struct parade_tch_abs_params *tch = &si->tch_hdr[CY_TCH_NUM];

#ifdef TTHE_TUNER_SUPPORT
		int size = get_unaligned_le16(&cd->input_buf[0]);

		if (size)
			tthe_print(cd, cd->input_buf, size, "OpModeData=");
#endif

    memcpy(si->xy_mode, cd->input_buf, si->desc.tch_header_size);
    parade_core_get_touch_axis(cd, &num_cur_tch, tch->size,
            tch->max, si->xy_mode + 3 + tch->ofs, tch->bofs);
    if (unlikely(num_cur_tch > max_tch))
        num_cur_tch = max_tch;

    length = num_cur_tch * si->desc.tch_record_size;

    memcpy(si->xy_data, &cd->input_buf[si->desc.tch_header_size], length);

    return 0;
}

static void parade_pr_buf_op_mode(struct parade_core_data *cd, u8 *pr_buf,
		struct parade_sysinfo *si, u8 cur_touch)
{
	const char fmt[] = "%02X ";
	int max = (CY_MAX_PRBUF_SIZE - 1) - sizeof(CY_PR_TRUNCATED);
	u8 report_id = si->xy_mode[2];
	int header_size = 0;
	int report_size = 0;
	int total_size = 0;
	int i, k;

	if (report_id == si->desc.tch_report_id) {
		header_size = si->desc.tch_header_size;
		report_size = cur_touch * si->desc.tch_record_size;
	} else if (report_id == si->desc.btn_report_id) {
		header_size = BTN_INPUT_HEADER_SIZE;
		report_size = BTN_REPORT_SIZE;
	}
	total_size = header_size + report_size;

	pr_buf[0] = 0;
	for (i = k = 0; i < header_size && i < max; i++, k += 3)
		scnprintf(pr_buf + k, CY_MAX_PRBUF_SIZE, fmt, si->xy_mode[i]);

	for (i = 0; i < report_size && i < max; i++, k += 3)
		scnprintf(pr_buf + k, CY_MAX_PRBUF_SIZE, fmt, si->xy_data[i]);

	pr_info("%s=%s%s\n", "cyttsp5_OpModeData", pr_buf,
			total_size <= max ? "" : CY_PR_TRUNCATED);
}


static int parade_debug_xy_worker(struct parade_core_data *cd)
{
	struct parade_sysinfo *si = &cd->sysinfo;
	u8 report_reg = si->xy_mode[TOUCH_COUNT_BYTE_OFFSET];
	u8 num_cur_tch = GET_NUM_TOUCHES(report_reg);
	/* print data for TTHE */
	parade_pr_buf_op_mode(cd, cd->pr_buf, si, num_cur_tch);
	return 0;
}

static int parade_set_gesture_key(struct ts_fingers *info, struct ts_cmd_node *out_cmd)
{
	if(true == tskit_parade_data->parade_chip_data->easy_wakeup_info.off_motion_on){
		tskit_parade_data->parade_chip_data->easy_wakeup_info.off_motion_on = false;
	}
	
	info->gesture_wakeup_value = TS_DOUBLE_CLICK;
	out_cmd->command = TS_INPUT_ALGO;
	out_cmd->cmd_param.pub_params.algo_param.algo_order =TS_ALGO_FUNC_0;
	TS_LOG_DEBUG("%s: ts_double_click evevnt report\n", __func__);

	return NO_ERR;
}
#define CY_WAKEUP_DOUBLE_TAB_BUF_SIZE 12
static void parade_double_tap_event(struct parade_core_data *cd, struct ts_fingers *info, struct ts_cmd_node *out_cmd)
{
	int i;
	int event_id = cd->input_buf[3];
	/*
	* wakeup event report
	* 0 ~ 1 : report size
	*  2  : report id
	*  3  : event id
	* 4 ~ 5 : x0
	* 6 ~ 7 : y0
	* 8 ~ 9 : x1
	* 10 ~ 11 : y1
	*/

	/*get double_tap dirctrion*/
	for (i = 0; i < CY_DOUBULE_TAB_POINT_NUM; i++) {
		cd->double_tab_info->postion[i].x = -1;
		cd->double_tab_info->postion[i].y = -1;
	}
	/*double click position*/
	for (i = 0; i < CY_DOUBULE_TAB_POINT_NUM; i++) {
		cd->double_tab_info->postion[i].x =
			get_unaligned_le16(&cd->input_buf[4 + i * 4]);
		cd->double_tab_info->postion[i].y =
			get_unaligned_le16(&cd->input_buf[6 + i * 4]);
	}
	TS_LOG_DEBUG("%s: event_id=%d\n", __func__, event_id);
	memset(g_ts_kit_platform_data.chip_data->easy_wakeup_info.easywake_position, 
			0, MAX_POSITON_NUMS);
	
	/*double click easy wakeup area*/
	for (i = 0; i < CY_DOUBULE_TAB_POINT_NUM; i++) {
		g_ts_kit_platform_data.chip_data->easy_wakeup_info.easywake_position[i] = 
				cd->double_tab_info->postion[i].x << 16 |cd->double_tab_info->postion[i].y;
	}

	mutex_lock(&cd->system_lock);
	parade_set_gesture_key(info, out_cmd);
	mutex_unlock(&cd->system_lock);

	return;
}

static void parade_wakeup_host(struct parade_core_data *cd, struct ts_fingers *info, struct ts_cmd_node *out_cmd)
{
	int rc = 0;
	u8 event_id = cd->input_buf[3];
	int size = get_unaligned_le16(&cd->input_buf[0]);
	/*
	* wakeup event report
	* 0 ~ 1 : report size
	*  2  : report id
	*  3  : event id
	* 4 ~ 5 : x0
	* 6 ~ 7 : y0
	* 8 ~ 9 : x1
	* 10 ~ 11 : y1
	*/

	if(false == cd->core_pdata->wakeup_gesture_enable){
		TS_LOG_ERR("%s: easy_wakeup mode is disable!!\n", __func__);
		return;
	}
	if(false == cd->parade_chip_data->easy_wakeup_info.off_motion_on){
		TS_LOG_INFO("%s: easy_wakeup no need report!!\n", __func__);
		return;
	}	
	/* Validate report */
	if (cd->input_buf[2] != HID_WAKEUP_REPORT_ID){
		TS_LOG_ERR("%s: invalid report !\n", __func__);
		goto invalid_report;
	}

	TS_LOG_DEBUG("%s: event_id[0x%x] report !\n", __func__,event_id);
	switch(event_id){
		case CY_CORE_EWG_TAP_TAP :
			TS_LOG_DEBUG("%s: double_tab_event\n", __func__);
			if (size == CY_WAKEUP_DOUBLE_TAB_BUF_SIZE &&
					cd->parade_chip_data->easy_wakeup_info.easy_wakeup_gesture & CY_DOUBLE_CLICK_WAKEUP){
				parade_double_tap_event(cd, info, out_cmd);
				return;
			}else{
				TS_LOG_ERR("%s: invalid size or not support double_tab event!\n", __func__);
			}
			break;
		default :
			TS_LOG_INFO("%s: Warning : no support wakeup_event class !!\n", __func__);
			break;
	}
	
invalid_report:
	parade_core_suspend();
	return;
}

static int parse_touch_input(struct parade_core_data *cd, int size, struct ts_fingers *info, struct ts_cmd_node *out_cmd)
{
    struct parade_sysinfo *si = &cd->sysinfo;
    int report_id = cd->input_buf[2];
    int rc = -EINVAL;
	int i,j;
	out_cmd->command = TS_INVAILD_CMD;
    if (!si->ready) {
        TS_LOG_ERR(
            "%s: Need system information to parse touches\n",
            __func__);
        return 0;
    }
    if (!si->xy_mode || !si->xy_data)
        return rc;

    if (report_id == si->desc.tch_report_id){
        rc = move_touch_data(cd, si);
		TS_LOG_DEBUG("%s: move_touch_data. rc =%d\n", __func__, rc);
		if(rc) return rc;
		rc = parade_xy_worker(cd,info,out_cmd);
		if (rc){
			TS_LOG_ERR("%s: xy_worker error rc = %d\n", __func__, rc);
			return rc;
		}
	if(g_ts_kit_log_cfg)
			parade_debug_xy_worker(cd);

    }
	else if (report_id == si->desc.btn_report_id){
		rc = move_button_data(cd, si);
	}
	else if (report_id == HID_SENSOR_DATA_REPORT_ID)
		rc = move_sensor_data(cd, si);
	else if (report_id == HID_TRACKING_HEATMAP_REPOR_ID)
		rc = move_tracking_hetmap_data(cd, si);
#ifdef PARADE_ROI_ENABLE
	else if (report_id == HID_KNUCKLE_MODE_REPOR_ID){
		rc = move_knuckle_data(cd,si);
	}
#endif
    return rc;
}
static int parse_command_input(struct parade_core_data *cd, int size)
{
	TS_LOG_DEBUG("%s:INT Trigger\n", __func__);

	memcpy(cd->response_buf, cd->input_buf, size);

	mutex_lock(&cd->system_lock);
	cd->hid_cmd_state = 0;
	mutex_unlock(&cd->system_lock);
	wake_up(&cd->wait_q);
	return 0;
}

static int parade_parse_input(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd, struct parade_core_data *cd)
{
    int report_id;
    int is_command = 0;
    int size;
    struct ts_fingers *info = &out_cmd->cmd_param.pub_params.algo_param.info;

	/* report event info
	* 0 ~ 1 : report size
	*  2  : report id
	* ...
	*/
    size = get_unaligned_le16(&cd->input_buf[0]);
    /* check reset */
    if (size == 0) {
        TS_LOG_INFO("%s: Reset complete\n", __func__);
        memcpy(cd->response_buf, cd->input_buf, 2);
        mutex_lock(&cd->system_lock);
        if (!cd->hid_reset_cmd_state && !cd->hid_cmd_state) {
			if(tskit_parade_data->sysinfo.ready){
				tskit_parade_data->is_need_check_mode_before_status_resume = true;
				schedule_delayed_work(&parade_status_resume_work,msecs_to_jiffies(PARADE_AFTER_RESUME_TIMEOUT));//Resume status afte device reset
			}
			mutex_unlock(&cd->system_lock);
            TS_LOG_INFO("%s: Device Initiated Reset\n",
                    __func__);

            return 0;
        }
        cd->hid_reset_cmd_state = 0;
        if (cd->hid_cmd_state == HID_OUTPUT_START_BOOTLOADER + 1
                || cd->hid_cmd_state ==
                    HID_OUTPUT_BL_LAUNCH_APP + 1
                || cd->hid_cmd_state ==
                    HID_OUTPUT_USER_CMD + 1)
            cd->hid_cmd_state = 0;
        mutex_unlock(&cd->system_lock);
        return 0;
    } else if (size == 2){
        return 0;
    } else if (size > CY_MAX_INPUT){
	TS_LOG_ERR("%s: Data size error!size = %d\n", __func__, size);
	cyttsp5_pr_buf(&cd->input_buf[0], CY_MAX_INPUT, "Empty Buffer");
	return 0;
    }
    report_id = cd->input_buf[2];
	TS_LOG_DEBUG("%s: report_id:%X\n", __func__, report_id);
	TS_LOG_DEBUG("%s: wakeup_gesture_enable:%u\n", __func__, cd->core_pdata->wakeup_gesture_enable);
	TS_LOG_DEBUG("%s: sleep_mode:%d\n", __func__, cd->parade_chip_data->easy_wakeup_info.sleep_mode);
	TS_LOG_DEBUG("%s: parade_is_power_on:%d\n", __func__, parade_is_power_on);
	TS_LOG_DEBUG("%s: ts_kit_gesture_func:%d\n", __func__, ts_kit_gesture_func);
	TS_LOG_DEBUG("%s: easy_wakeup_gesture:%d\n", __func__, cd->parade_chip_data->easy_wakeup_info.easy_wakeup_gesture);
	/* Check wake-up report */
	if (report_id == HID_WAKEUP_REPORT_ID 
			&& cd->parade_chip_data->easy_wakeup_info.sleep_mode == TS_GESTURE_MODE
			&& cd->parade_chip_data->easy_wakeup_info.easy_wakeup_gesture != 0
			&& parade_is_power_on
			&& ts_kit_gesture_func) {
		parade_wakeup_host(cd, info, out_cmd);
		return NO_ERR;
	}

#ifdef PARADE_ROI_ENABLE
	if (report_id != cd->sysinfo.desc.tch_report_id
            && report_id != cd->sysinfo.desc.btn_report_id
            && report_id != HID_SENSOR_DATA_REPORT_ID
            && report_id != HID_TRACKING_HEATMAP_REPOR_ID
            && report_id != HID_KNUCKLE_MODE_REPOR_ID)
        is_command = 1;
#else
    if (report_id != cd->sysinfo.desc.tch_report_id
            && report_id != cd->sysinfo.desc.btn_report_id
            && report_id != HID_SENSOR_DATA_REPORT_ID
            && report_id != HID_TRACKING_HEATMAP_REPOR_ID)
        is_command = 1;
#endif

    if (unlikely(is_command)) {
        parse_command_input(cd, size);
        return 0;
    }

    parse_touch_input(cd, size, info,out_cmd);
    return 0;
}

static  bool IS_INT_Glitch(struct parade_core_data *cd)
{
	/* bl_launch app creates a glitch in IRQ line */
	if (cd->hid_cmd_state == HID_OUTPUT_BL_LAUNCH_APP + 1 && cd->core_pdata->irq_stat) {
		/*
		the width for the INT glitch is about 4us,the normal INT width
		of response will last more than 200us, so use 20us delay for
		distinguish the glitch the normal INT is enough.
		*/
		udelay(20);
		if (cd->core_pdata->irq_stat(cd->parade_chip_data) != CY_IRQ_ASSERTED_VALUE){
			return true;
		}
	}
	return false;
}

#define WAIT_I2C_BUS_RESUME_TIME 300
#define USLEEP_RANGE_MIN 1000
#define USLEEP_RANGE_MAX 1200

static int parade_irq_bottom_half(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	struct parade_core_data *cd = tskit_parade_data;
	int rc;

	if(!strncmp(g_ts_kit_platform_data.product_name,"ares",sizeof("ares"))) {
		struct device *parent = cd->dev->parent->parent;
		int try = 0;
		if((g_ts_kit_platform_data.chip_data->easy_wakeup_info.sleep_mode == TS_GESTURE_MODE) &&
		(atomic_read(&g_ts_kit_platform_data.state) == TS_WORK_IN_SLEEP)){
			while ((try < WAIT_I2C_BUS_RESUME_TIME) && (parent->power.is_noirq_suspended == true)) {
				usleep_range(USLEEP_RANGE_MIN, USLEEP_RANGE_MAX);
				try ++;
			}
			if (parent->power.is_noirq_suspended == true)
				TS_LOG_ERR("%s: Failed to wait system resume after %d ms\n",
					__func__, try);
			else if (try > 0)
				TS_LOG_INFO("%s: Success to wait system resume after %d ms\n",
					__func__, try);
		}
	}

	if(cd->core_pdata->check_irq_state)
		if (IS_INT_Glitch(cd))
			return NO_ERR;
	out_cmd->command = TS_INVAILD_CMD;
	if(atomic_read(&parade_reset_pin_status) != 0){ /*Do not read i2c during reset is low stage*/
		rc = parade_read_input(cd);
		if (!rc)
			parade_parse_input(in_cmd, out_cmd,cd);
	}
	return NO_ERR;
}

static int parade_reset_chip(void)
{
	int rc = 0;
	parade_hw_reset();
	return rc;
}

static int parade_algo_t1(struct ts_kit_device_data *dev_data, struct ts_fingers *in_info,
		    struct ts_fingers *out_info)
{
	int index;
	int id;
	TS_LOG_INFO("%s Enter",__func__);
#if 1
	for (index = 0, id = 0; index < TS_MAX_FINGER; index++, id++) {
		if (in_info->cur_finger_number == 0) {
			out_info->fingers[0].status = TS_FINGER_RELEASE;
			if (id >= 1)
				out_info->fingers[id].status = 0;
		} else {
			if (in_info->fingers[index].x != 0
			    || in_info->fingers[index].y != 0) {
				if (CY_EV_TOUCHDOWN ==
				    in_info->fingers[index].event
				    || CY_EV_MOVE ==
				    in_info->fingers[index].event
				    || CY_EV_NO_EVENT ==
				    in_info->fingers[index].event) {
					out_info->fingers[id].x =
					    in_info->fingers[index].x;
					out_info->fingers[id].y =
					    in_info->fingers[index].y;
					out_info->fingers[id].pressure =
					    in_info->fingers[index].pressure;
					out_info->fingers[id].status =
					    TS_FINGER_PRESS;
				} else if (CY_EV_LIFTOFF ==
					   in_info->fingers[index].event) {
					out_info->fingers[id].status =
					    TS_FINGER_RELEASE;
				}
			} else
				out_info->fingers[id].status = 0;
		}
	}
#else
	memcpy(out_info,in_info,sizeof(in_info));
	for(index = 0; index < TS_MAX_FINGER; index++)/*Out Finger Index*/{
		if(in_info->fingers[index].event != 0xFF)/*Valid Event*/{
			if (CY_EV_TOUCHDOWN == in_info->fingers[index].event){
				out_info->fingers[index].status = TS_FINGER_PRESS;
			} else if (CY_EV_MOVE == in_info->fingers[index].event){
				out_info->fingers[index].status = TS_FINGER_MOVE;
			} else if (CY_EV_NO_EVENT == in_info->fingers[index].event){
				out_info->fingers[index].status = TS_FINGER_MOVE;
			} else if (CY_EV_NO_EVENT == in_info->fingers[index].event){
				out_info->fingers[index].status = TS_FINGER_RELEASE;
			}
		} else {
			out_info->fingers[index].status = TS_FINGER_RELEASE;
		}
	}
#endif
	return NO_ERR;
}

static struct ts_algo_func parade_algo_f1 = {
	.algo_name = "parade_algo_t1",
	.chip_algo_func = parade_algo_t1,
};

static int parade_register_algo(struct ts_kit_device_data *chip_data)
{
	int rc = 0;
	TS_LOG_INFO("%s: parade_reg_algo called\n", __func__);

	rc = register_ts_algo_func(chip_data, &parade_algo_f1);	/*put algo_f1 into list contained in chip_data, named algo_t1*/
	if (rc) {
		TS_LOG_ERR("%s: register ts algo fail\n", __func__);
	}

out:
	return rc;
}
static int parade_input_config(struct input_dev *input_dev)
{
	TS_LOG_INFO("%s: parade_input_config called\n", __func__);
	tskit_parade_data->input = input_dev;
	set_bit(TS_DOUBLE_CLICK, input_dev->keybit);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, tskit_parade_data->md.x_min,
			     tskit_parade_data->md.x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, tskit_parade_data->md.y_min,
			     tskit_parade_data->md.y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, tskit_parade_data->md.p_min,
			     tskit_parade_data->md.p_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, tskit_parade_data->md.t_min,
			     tskit_parade_data->md.t_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, tskit_parade_data->md.major_min, tskit_parade_data->md.major_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR, tskit_parade_data->md.minor_min, tskit_parade_data->md.minor_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_ORIENTATION, tskit_parade_data->md.minor_min, tskit_parade_data->md.minor_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 100, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MINOR, 0, 100, 0, 0);

	return NO_ERR;
}


////////////////////////////////parade_loader/////////////////////////////
struct cyttsp5_dev_id {
	u32 silicon_id;
	u8 rev_id;
	u32 bl_ver;
};
#define CY_DATA_ROW_SIZE				128

struct cyttsp5_hex_image {
	u8 array_id;
	u16 row_num;
	u16 row_size;
	u8 row_data[CY_DATA_ROW_SIZE];
} __packed;
#define CY_DATA_MAX_ROW_SIZE				256


static int cyttsp5_ldr_enter_(struct cyttsp5_dev_id *dev_id)
{
	int rc;
	struct parade_hid_output hid_output = {
			HID_OUTPUT_APP_COMMAND(HID_OUTPUT_START_BOOTLOADER),
			.timeout_ms = CY_HID_OUTPUT_START_BOOTLOADER_TIMEOUT,
			.reset_expected = 1,
		};

	rc = parade_hid_send_output_and_wait_(tskit_parade_data, &hid_output);
	if (rc < 0)
		return rc;
	TS_LOG_INFO("%s successful, rc = %d\n", __func__, rc);
	return 0;
}
static u8 *cyttsp5_get_row_(struct parade_core_data *cd, u8 *row_buf,
		u8 *image_buf, int size)
{
	memcpy(row_buf, image_buf, size);
	return image_buf + size;
}
#define CY_ARRAY_ID_OFFSET				0
#define CY_ROW_NUM_OFFSET				1
#define CY_ROW_SIZE_OFFSET				3
#define CY_ROW_DATA_OFFSET				5

static int cyttsp5_ldr_parse_row_(struct parade_core_data *cd, u8 *row_buf,
	struct cyttsp5_hex_image *row_image)
{
	int rc = 0;

	row_image->array_id = row_buf[CY_ARRAY_ID_OFFSET];
	row_image->row_num = get_unaligned_be16(&row_buf[CY_ROW_NUM_OFFSET]);
	row_image->row_size = get_unaligned_be16(&row_buf[CY_ROW_SIZE_OFFSET]);

	if (row_image->row_size > ARRAY_SIZE(row_image->row_data)) {
		TS_LOG_INFO("%s: row data buffer overflow\n", __func__);
		rc = -EOVERFLOW;
		goto cyttsp5_ldr_parse_row_exit;
	}

	memcpy(row_image->row_data, &row_buf[CY_ROW_DATA_OFFSET],
	       row_image->row_size);
cyttsp5_ldr_parse_row_exit:
	return rc;
}

static int parade_initiate_bl_(struct parade_core_data *cd,
		u16 key_size, u8 *key_buf, u16 row_size, u8 *metadata_row_buf)
{
	u16 write_length = key_size + row_size;
	u8 *write_buf;
	int rc;
	TS_LOG_INFO("%s enter, row_size=%d\n", __func__, row_size);
	struct parade_hid_output hid_output = {
		HID_OUTPUT_BL_COMMAND(HID_OUTPUT_BL_INITIATE_BL),
		.write_length = write_length,
		.timeout_ms = CY_HID_OUTPUT_BL_INITIATE_BL_TIMEOUT,
	};

	if(!metadata_row_buf){
		TS_LOG_INFO("%s metadata_row_buf is NULL\n", __func__);
		return -1;
	}

	write_buf = kzalloc(write_length, GFP_KERNEL);
	if (!write_buf)
		return -ENOMEM;

	hid_output.write_buf = write_buf;

	if (key_size)
		memcpy(write_buf, key_buf, key_size);

	if (row_size)
		memcpy(&write_buf[key_size], metadata_row_buf, row_size);

	rc =  parade_hid_send_output_and_wait_(cd, &hid_output);

	kfree(write_buf);
	return rc;
}

static int cyttsp5_ldr_init_(struct parade_core_data *cd,
		struct cyttsp5_hex_image *row_image)
{
		int rc = NO_ERR;
		TS_LOG_INFO("%s, enter\n", __func__);
		rc = parade_initiate_bl_(cd, 8,(u8 *)cyttsp5_security_key, row_image->row_size,row_image->row_data);
		return rc;
}
static int parade_bl_program_and_verify_(
		struct parade_core_data *cd, u16 data_len, u8 *data_buf)
{
	struct parade_hid_output hid_output = {
		HID_OUTPUT_BL_COMMAND(HID_OUTPUT_BL_PROGRAM_AND_VERIFY),
		.write_length = data_len,
		.write_buf = data_buf,
		.timeout_ms = CY_HID_OUTPUT_BL_PROGRAM_AND_VERIFY_TIMEOUT,
	};

	return parade_hid_send_output_and_wait_(cd, &hid_output);
}

static int cyttsp5_ldr_prog_row_(struct parade_core_data *cd,
				 struct cyttsp5_hex_image *row_image)
{
	u16 length = row_image->row_size + 3;
	u8 data[3 + row_image->row_size];
	u8 offset = 0;

	data[offset++] = row_image->array_id;
	data[offset++] = LOW_BYTE(row_image->row_num);
	data[offset++] = HI_BYTE(row_image->row_num);
	memcpy(data + 3, row_image->row_data, row_image->row_size);
	return parade_bl_program_and_verify_(cd,length, data);
}

static int cyttsp5_ldr_exit_(struct parade_core_data *cd)
{
		struct parade_hid_output hid_output = {
		HID_OUTPUT_BL_COMMAND(HID_OUTPUT_BL_LAUNCH_APP),
		.reset_expected = 1,
	};

	return parade_hid_send_output_and_wait_(cd, &hid_output);
}

static int cyttsp5_ldr_verify_chksum_(struct parade_core_data *cd)
{
	u8 result;
	int rc;

	struct parade_hid_output hid_output = {
		HID_OUTPUT_BL_COMMAND(HID_OUTPUT_BL_VERIFY_APP_INTEGRITY),
	};

	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	if (rc)
		return -EINVAL;

	result = cd->response_buf[8];

	/* fail */
	if (result == 0)
		return -EINVAL;

	return 0;
}

static int parade_load_app_(struct device *dev, const u8 *fw, int fw_size)
{
	struct cyttsp5_dev_id *dev_id;
	struct cyttsp5_hex_image *row_image;
	u8 *row_buf;
	size_t image_rec_size;
	size_t row_buf_size = CY_DATA_MAX_ROW_SIZE;
	int row_count = 0;
	u8 *p;
	u8 *last_row;
	int rc;
	int rc_tmp;

	//enable_irq(tskit_parade_data->parade_chip_data->ts_platform_data->irq_id);

	image_rec_size = sizeof(struct cyttsp5_hex_image);
	if (fw_size % image_rec_size != 0) {
		TS_LOG_ERR("%s: Firmware image is misaligned\n", __func__);
		rc = -EINVAL;
		goto _parade_load_app_error;
	}

	TS_LOG_INFO("%s: start load app\n", __func__);

	row_buf = kzalloc(row_buf_size, GFP_KERNEL);
	row_image = kzalloc(sizeof(struct cyttsp5_hex_image), GFP_KERNEL);
	dev_id = kzalloc(sizeof(struct cyttsp5_dev_id), GFP_KERNEL);
	if (!row_buf || !row_image || !dev_id) {
		rc = -ENOMEM;
		goto _parade_load_app_exit;
	}
	if(tskit_parade_data->mode == CY_MODE_OPERATIONAL){
		TS_LOG_INFO("%s: Send BL Loader Enter\n", __func__);
		rc = cyttsp5_ldr_enter_(dev_id);
		if (rc) {
			TS_LOG_INFO("%s: Error cannot start Loader (ret=%d)\n",
				__func__, rc);
			goto _parade_load_app_exit;
		}
		TS_LOG_INFO("%s: Chip is in bootloader now\n", __func__);
		tskit_parade_data->mode = CY_MODE_BOOTLOADER;/*2016.12.22: Bug Fix, if we do not do this, the retry will not take effect*/
	}
	else{
		TS_LOG_INFO("%s: In Bootloader mode, no need to enter\n", __func__);
	}
	/* get last row */
	last_row = (u8 *)fw + fw_size - image_rec_size;
	cyttsp5_get_row_(tskit_parade_data, row_buf, last_row, image_rec_size);
	cyttsp5_ldr_parse_row_(tskit_parade_data, row_buf, row_image);

	/*will check every row size if overflow*/
	p = (u8 *)fw;
	while (p < last_row) {
		/* Get row */
		row_count += 1;
		memset(row_buf, 0, row_buf_size);
		p = cyttsp5_get_row_(tskit_parade_data, row_buf, p, image_rec_size);
		/* Parse row */
		row_image->row_size = get_unaligned_be16(&row_buf[CY_ROW_SIZE_OFFSET]);
		if (row_image->row_size > ARRAY_SIZE(row_image->row_data)) {
			TS_LOG_ERR("%s: check row data buffer overflow,not upgrade,read row=%d\n", __func__,row_count);
			rc = -EOVERFLOW;
			goto _parade_load_app_exit;
		}
	}

	TS_LOG_INFO("%s: read all row=%d\n", __func__, row_count);

	/* initialise bootloader */
	rc = cyttsp5_ldr_init_(tskit_parade_data, row_image);
	if (rc) {
		TS_LOG_ERR("%s: Error cannot init Loader (ret=%d)\n",
			__func__, rc);
		goto _parade_load_app_exit;
	}
	TS_LOG_INFO("%s: Send BL Loader Blocks\n", __func__);

	row_count = 0;
	p = (u8 *)fw;
	while (p < last_row) {
		/* Get row */
		row_count += 1;
		TS_LOG_DEBUG("%s: read row=%d\n", __func__, row_count);
		memset(row_buf, 0, row_buf_size);
		p = cyttsp5_get_row_(tskit_parade_data, row_buf, p, image_rec_size);
		/* Parse row */
		TS_LOG_DEBUG("%s: p=%p buf=%p buf[0]=%02X\n", __func__, p, row_buf, row_buf[0]);
		rc = cyttsp5_ldr_parse_row_(tskit_parade_data, row_buf, row_image);
		TS_LOG_DEBUG("%s: array_id=%02X row_num=%04X(%d) row_size=%04X(%d)\n",
			__func__, row_image->array_id, row_image->row_num, row_image->row_num,
			row_image->row_size, row_image->row_size);
		if (rc) {
			TS_LOG_ERR("%s: Parse Row Error (a=%d r=%d ret=%d\n",__func__, row_image->array_id,
				row_image->row_num, rc);
			goto _parade_load_app_exit;
		} else {
			TS_LOG_DEBUG("%s: Parse Row (a=%d r=%d ret=%d\n", __func__, row_image->array_id,
				row_image->row_num, rc);
		}

		/* program row */
		rc = cyttsp5_ldr_prog_row_(tskit_parade_data, row_image);
		if (rc) {
			TS_LOG_ERR("%s: Program Row Error (array=%d row=%d ret=%d)\n",
				__func__, row_image->array_id,
				row_image->row_num, rc);
			goto _parade_load_app_exit;
		}

		TS_LOG_DEBUG("%s: array=%d row_cnt=%d row_num=%04X\n",
			__func__, row_image->array_id, row_count,
			row_image->row_num);

	}
	//parade_hw_reset();

	/* exit loader */
	TS_LOG_INFO("%s: Send BL Loader Terminate\n", __func__);

	rc = cyttsp5_ldr_exit_(tskit_parade_data);
	if (rc) {
		TS_LOG_ERR("%s: Error on exit Loader (ret=%d)\n",
			__func__, rc);

		/* verify app checksum */
		rc_tmp = cyttsp5_ldr_verify_chksum_(tskit_parade_data);
		if (rc_tmp)
			TS_LOG_INFO("%s: ldr_verify_chksum fail r=%d\n",
				__func__, rc_tmp);
		else
			TS_LOG_INFO("%s: APP Checksum Verified\n", __func__);
	}


_parade_load_app_exit:
	if(row_buf){
		kfree(row_buf);
		row_buf = NULL;
	}
	if(row_image){
		kfree(row_image);
		row_image = NULL;
	}
	if(dev_id){
		kfree(dev_id);
		dev_id = NULL;
	}
_parade_load_app_error:
	return rc;

}


static int cyttsp5_upgrade_firmware(struct device *dev, const u8 *fw_img,
		int fw_size)
{
	int retry = 5;
	int rc;
	struct parade_core_data *cd = tskit_parade_data;

	while (retry--) {
		rc = parade_load_app_(dev, fw_img, fw_size);
		if (rc < 0)
			TS_LOG_INFO("%s: Firmware update failed rc=%d, retry:%d\n",
				__func__, rc, retry);
		else
			break;
		if(retry>0){
			TS_LOG_INFO("%s: Re-Get Mode Before Retry\n", __func__);
			rc = parade_get_hid_descriptor_(cd, &cd->hid_desc);
			if (rc < 0) {
				TS_LOG_ERR("%s: Error on getting HID descriptor r=%d\n",
					__func__, rc);
			} else {
				cd->mode = parade_get_mode(cd, &cd->hid_desc);
				TS_LOG_INFO("%s: chip current mode %d\n", __func__,cd->mode);
			}
		}
		msleep(20);
	}
	if (rc < 0) {
		TS_LOG_INFO("%s: Firmware update failed with error code %d\n",
			__func__, rc);
		return -1;
	}

	TS_LOG_INFO("%s successful\n", __func__);
exit:

	return rc;
}

static int parade_fw_update(const struct firmware *fw)
{
	struct device *dev = tskit_parade_data->dev;
	struct parade_loader_data *ld = &tskit_parade_data->ld;
	u8 header_size = 0;
	int rc = 0;
	TS_LOG_INFO("%s enter", __func__);

	if (!fw){
		rc = -1;
#if defined (CONFIG_HUAWEI_DSM)
		tskit_parade_data->parade_chip_data->ts_platform_data->dsm_info.constraints_UPDATE_status = FWU_REQUEST_FW_FAIL;
#endif
		goto cyttsp5_firmware_cont_exit;
	}
	TS_LOG_INFO("%s fw exit", __func__);
	if (!fw->data || !fw->size) {
		TS_LOG_INFO( "%s: No firmware received\n", __func__);
#if defined (CONFIG_HUAWEI_DSM)
		tskit_parade_data->parade_chip_data->ts_platform_data->dsm_info.constraints_UPDATE_status = FWU_FW_CONT_ERROR;
#endif
		rc = -1;
		goto cyttsp5_firmware_cont_release_exit;
	}
	TS_LOG_INFO("%s fw_size normal", __func__);

	header_size = fw->data[0];
	TS_LOG_INFO("%s header_size = %d", __func__, header_size);
	if (header_size >= (fw->size + 1)) {
		TS_LOG_INFO( "%s: Firmware format is invalid\n", __func__);
#if defined (CONFIG_HUAWEI_DSM)
		tskit_parade_data->parade_chip_data->ts_platform_data->dsm_info.constraints_UPDATE_status = FWU_FW_CONT_ERROR;
#endif
		rc = -1;
		goto cyttsp5_firmware_cont_release_exit;
	}
	TS_LOG_INFO("%s before cyttsp5_upgrade_firmware", __func__);

	rc = cyttsp5_upgrade_firmware(dev, &(fw->data[header_size + 1]),
		fw->size - (header_size + 1));

	TS_LOG_INFO("%s after cyttsp5_upgrade_firmware,rc:%d", __func__,rc);

cyttsp5_firmware_cont_release_exit:
	release_firmware(fw);

cyttsp5_firmware_cont_exit:
	ld->is_manual_upgrade_enabled = 0;
	return rc;
}

static void parade_fw_update_cont(const struct firmware *fw, void *context)
{
	struct device *dev = tskit_parade_data->dev;
	struct parade_loader_data *ld = &tskit_parade_data->ld;
	u8 header_size = 0;
	TS_LOG_INFO("%s enter", __func__);

	if (!fw)
		goto cyttsp5_firmware_cont_exit;
	TS_LOG_INFO("%s fw exit", __func__);
	if (!fw->data || !fw->size) {
		TS_LOG_INFO( "%s: No firmware received\n", __func__);
		goto cyttsp5_firmware_cont_release_exit;
	}
	TS_LOG_INFO("%s fw_size normal", __func__);

	header_size = fw->data[0];
	TS_LOG_INFO("%s header_size = %d", __func__, header_size);
	if (header_size >= (fw->size + 1)) {
		TS_LOG_INFO( "%s: Firmware format is invalid\n", __func__);
		goto cyttsp5_firmware_cont_release_exit;
	}
	TS_LOG_INFO("%s before cyttsp5_upgrade_firmware", __func__);

	cyttsp5_upgrade_firmware(dev, &(fw->data[header_size + 1]),
		fw->size - (header_size + 1));

	TS_LOG_INFO("%s after cyttsp5_upgrade_firmware", __func__);

cyttsp5_firmware_cont_release_exit:
	release_firmware(fw);

cyttsp5_firmware_cont_exit:
	ld->is_manual_upgrade_enabled = 0;
}


static const struct firmware *fw_entry;

static int upgrade_firmware_from_class(struct device *dev)
{
	int retval;

	retval = request_firmware(&fw_entry, "ts/parade_tt.bin", dev);
	if (retval < 0) {
		TS_LOG_ERR("%s: Fail request firmware\n", __func__);
		return -1;
	}
	if (fw_entry == NULL) {
		TS_LOG_ERR("%s: fw_entry == NULL\n", __func__);
		return -2;
	}
	if (fw_entry->data == NULL || fw_entry->size == 0) {
		TS_LOG_ERR("%s: No firmware received\n", __func__);
		return -3;
	}
	TS_LOG_INFO("%s: fw_entry->size = %zu\n", __func__, fw_entry->size);

	TS_LOG_INFO("%s: Enabling firmware class loader\n", __func__);

	return parade_fw_update(fw_entry);

	return 0;
}

#define PROJECT_ID_PRODUCT_NAME_LEN 4
static int parade_create_project_id(void)
{
	int retval = NO_ERR;
	int chip_id = 0;
	int i = 0;
	char tmp_buff[PROJECT_ID_PRODUCT_NAME_LEN + 1 ] = {0};

	//AAAABBCCC: AAAA(product_name), BB: chip_ic_id, CCC: module_id
	if(tskit_parade_data->sysinfo.ready && already_get_flag == false) {
		/* get chip id */
		if(!strcmp("cs448", tskit_parade_data->chip_name)) {
			chip_id = CHIP_CS448_ID;
		}else if(!strcmp("cytt21403", tskit_parade_data->chip_name)){
			chip_id = CHIP_CYTT21403_ID;
		}else if(!strcmp("tt41701", tskit_parade_data->chip_name)){
			chip_id = CHIP_TT41701_ID;
		}else {
			retval =  -EINVAL;
			TS_LOG_ERR("%s: get chip id failed!\n", __func__);
			goto out;
		}
		for (i = 0; g_ts_kit_platform_data.product_name[i] && i < PROJECT_ID_PRODUCT_NAME_LEN; i++) //exchange name to upper
		{ tmp_buff[i] = toupper(g_ts_kit_platform_data.product_name[i]); }
		/* create project id */
		snprintf(tskit_parade_data->project_id, MAX_STR_LEN, "%s%02d%02d0",
				tmp_buff, chip_id,  tskit_parade_data->panel_id);

		/* get module name */
		switch (tskit_parade_data->panel_id) {
			case MODULE_OFILM_ID:
				strncpy(tskit_parade_data->module_vendor, "ofilm", MAX_STR_LEN);
				break;
			case MODULE_TOPTOUCH_ID:
				strncpy(tskit_parade_data->module_vendor, "toptouch", MAX_STR_LEN);
				break;
			case MODULE_LENSONE_ID:
				strncpy(tskit_parade_data->module_vendor, "lensone", MAX_STR_LEN);
				break;
			case MODULE_JUNDA_ID:
				strncpy(tskit_parade_data->module_vendor, "junda", MAX_STR_LEN);
				break;
			case MODULE_MUTTO_ID:
				strncpy(tskit_parade_data->module_vendor, "mutto", MAX_STR_LEN);
				break;
			case MODULE_EELY_ID:
				strncpy(tskit_parade_data->module_vendor, "eely", MAX_STR_LEN);
				break;
			case MODULE_TRULY_ID:
				strncpy(tskit_parade_data->module_vendor, "truly", MAX_STR_LEN);
				break;
			default:
				retval = -EINVAL;
				break;
		}
		if(retval < 0) {
			TS_LOG_ERR("%s: get module name failed!\n", __func__);
			goto out;
		}

		memset(tskit_parade_data->parade_chip_data->module_name, 0, MAX_STR_LEN);
		snprintf(tskit_parade_data->parade_chip_data->module_name, MAX_STR_LEN -1, "%s",
					tskit_parade_data->module_vendor);
		already_get_flag = true;
		if(tskit_parade_data->need_provide_projectID_for_sensor){
			memcpy(tskit_parade_data->parade_chip_data->project_id, tskit_parade_data->project_id,MAX_STR_LEN -1);//proviod TP's project id for other module.
		}
		TS_LOG_INFO("%s: create project id successful: %s!\n", __func__, tskit_parade_data->project_id);
	}
out:
	return retval;
}
static void parade_get_fw_name(char *file_name)
{
	int ret =0;
	tskit_parade_data->fw_and_config_name = file_name; //Point to the host structure
	if(tskit_parade_data->fw_only_depend_on_lcd){ //TP fw only depend on LCD module
		 ret = get_lcd_module_name();
		if(!ret){
			strncat(file_name, tskit_parade_data->lcd_module_name, strlen(tskit_parade_data->lcd_module_name));
		}
	}
	else{
		strncat(file_name, tskit_parade_data->project_id, MAX_STR_LEN);
		strncat(file_name, "_", 1);
		strncat(file_name, tskit_parade_data->module_vendor, MAX_STR_LEN);
	}
	if(tskit_parade_data->fw_need_depend_on_lcd && !tskit_parade_data->is_firmware_broken){
		ret = parade_get_lcd_panel_info();
		if(ret){
			TS_LOG_ERR("%s: get lcd panel info faile!\n ",__func__);
		}
		ret = get_lcd_module_name();
		if(!ret){
			strncat(file_name, "_", 1);
			strncat(file_name, tskit_parade_data->lcd_module_name, strlen(tskit_parade_data->lcd_module_name));
		}
	}
	if(tskit_parade_data->need_distinguish_lcd){
		if (get_panel_name_flag_adapter()){
			strncat(file_name, "_new", strlen("_new"));
			TS_LOG_INFO("%s, firmware name: %s\n",__func__,file_name);
			return;
		}
		if(tskit_parade_data->current_distinguishability){
			strncat(file_name,tskit_parade_data->sign_of_second_distinguishability, strlen(tskit_parade_data->sign_of_second_distinguishability));
			TS_LOG_INFO("%s, firmware name: %s\n",__func__,file_name);
			return;
		}
	}
	TS_LOG_INFO("%s, firmware name: %s\n",__func__,file_name);
	return;
}
static int parade_fw_update_boot(char *file_name)
{
	int rc = NO_ERR;
	int t = 0;
	u8 header_size = 0;
	int timeout_ms = PARADE_WORK_WAIT_TIMEOUT;
	tskit_parade_data->firmware_broken_update_flag = false;
	tskit_parade_data->is_firmware_restore = false;
	if(tskit_parade_data->create_project_id_flag == 1){
		rc = parade_create_project_id();
		if (rc < 0) {
			TS_LOG_ERR("%s: create project id failed!\n", __func__);
			return rc;
		}
	}
	if(tskit_parade_data->need_upgrade_again)
		strncpy(tskit_parade_data->temporary_firmware_name, file_name, MAX_STR_LEN);
upgrade_firmware_again:
	if(tskit_parade_data->need_upgrade_again && tskit_parade_data->is_firmware_broken){
		parade_get_fw_name(tskit_parade_data->temporary_firmware_name);
		tskit_parade_data->is_firmware_restore = true;
		tskit_parade_data->is_firmware_broken = false;
	}else{
		parade_get_fw_name(file_name);
	}
	if(file_name == NULL){
		return -EINVAL;
	}
	rc = parade_check_cmd_status();
	if(rc)
		return rc;
	g_ts_kit_platform_data.chip_data->isbootupdate_finish = false;
	if(tskit_parade_data->need_upgrade_again &&  tskit_parade_data->is_firmware_restore){
		tskit_parade_data->firmware_broken_update_flag = true;
		TS_LOG_INFO("%s, firmware is restore,no need parade_loader_probe\n",__func__);
	}
	else
		parade_loader_probe(tskit_parade_data);

	if (tskit_parade_data->process_state == CY_STATE_INIT) {
		rc = parade_need_fw_update();
		if (rc == 0) {
			tskit_parade_data->process_state = CY_STATE_NORMAL;
			goto exit;
		} else if(rc > 0) {
			tskit_parade_data->process_state = CY_STATE_FW_UPDATE;
		} else {
			goto exit;
		}
	}

	if (fw_entry == NULL) {
		TS_LOG_ERR("%s: fw_entry == NULL\n", __func__);
#if defined (CONFIG_HUAWEI_DSM)
		tskit_parade_data->parade_chip_data->ts_platform_data->dsm_info.constraints_UPDATE_status = FWU_FW_CONT_ERROR;
#endif
		rc = -EINVAL;
		goto entry_null;
	}
	if (fw_entry->data == NULL || fw_entry->size == 0) {
		TS_LOG_ERR("%s: No firmware received\n", __func__);
#if defined (CONFIG_HUAWEI_DSM)
		tskit_parade_data->parade_chip_data->ts_platform_data->dsm_info.constraints_UPDATE_status = FWU_FW_CONT_ERROR;
#endif
		rc = -EINVAL;
		goto firmware_release;
	}
	header_size = fw_entry->data[0];
	if (header_size >= (fw_entry->size + 1)) {
		TS_LOG_ERR("%s: Firmware format is invalid\n", __func__);
#if defined (CONFIG_HUAWEI_DSM)
		tskit_parade_data->parade_chip_data->ts_platform_data->dsm_info.constraints_UPDATE_status = FWU_FW_CONT_ERROR;
#endif
		rc = -EINVAL;
		goto firmware_release;
	}
	rc = parade_fw_update(fw_entry);
	if(rc == 0){
		TS_LOG_INFO("%s: FW upgrade done, Restart up after fw upprade\n",
					__func__);
		rc = parade_startup(tskit_parade_data,false);
		if(rc){
			TS_LOG_ERR("%s: Startup Error after FW upgrade\n",
					__func__);
#if defined (CONFIG_HUAWEI_DSM)
			tskit_parade_data->parade_chip_data->ts_platform_data->dsm_info.constraints_UPDATE_status = FWU_START_UP_FAIL;
#endif
		} else {
			TS_LOG_INFO("%s: Start up successfully after FW upgrade\n",
					__func__);

			if(tskit_parade_data->calibrate_fw_update_and_cmcp) {
				rc = parade_calibrate_child();
				if(!rc) {
					TS_LOG_INFO("%s calibrate end\n", __func__);
				}
			}
		}
	}
	else
		TS_LOG_ERR("%s: Fail to FW upgrade\n",
					__func__);
	if(tskit_parade_data->create_project_id_flag == 1){
		rc = parade_create_project_id();
		if (rc < 0) {
			TS_LOG_ERR("%s: create project id failed!\n", __func__);
		}
	}
	if(tskit_parade_data->need_upgrade_again && tskit_parade_data->is_firmware_broken){
		if(!tskit_parade_data->firmware_broken_update_flag){
			tskit_parade_data->process_state = CY_STATE_INIT;
			parade_finish_cmd();
			goto upgrade_firmware_again;
		}
	}
exit:
	parade_finish_cmd();
	g_ts_kit_platform_data.chip_data->isbootupdate_finish = true;
	wake_up(&tskit_parade_data->wait_q_bootup);
	return rc;

firmware_release:
	TS_LOG_INFO("%s,release_firmware\n", __func__);
	if (fw_entry != NULL) {
		release_firmware(fw_entry);
		fw_entry = NULL;
	}
entry_null:
	tskit_parade_data->process_state = CY_STATE_NORMAL;
	parade_finish_cmd();
	return rc;
}

static int parade_check_firmware_version(u32 fw_ver_new, u32 fw_revctrl_new)
{
	u32 fw_ver_img;
	u32 fw_revctrl_img;
	tskit_parade_data->is_firmware_broken = false;
	if(!tskit_parade_data->sysinfo.ready){
		TS_LOG_INFO("%s: sysinfo is not ready, fw may be corrupted, FW Upgrade\n",
				__func__);
		TS_LOG_INFO("%s: Build in FW Version:0x%04X 0x%04X\n", __func__,
			fw_revctrl_new, fw_ver_new);
#if defined (CONFIG_HUAWEI_DSM)
		ts_dmd_report(DSM_TP_FW_CRC_ERROR_NO, "parade ts CRC error.\n");
#endif

		if(tskit_parade_data->need_upgrade_again){
			tskit_parade_data->is_firmware_broken = true;
			TS_LOG_INFO("%s: Firmware is broken ,is_firmware_broken is %d\n",__func__,tskit_parade_data->is_firmware_broken);
		}
		return 1;

	}
	fw_ver_img = tskit_parade_data->sysinfo.cydata.fw_ver_major << 8;
	fw_ver_img += tskit_parade_data->sysinfo.cydata.fw_ver_minor;
	TS_LOG_INFO("%s: img vers:0x%04X new vers:0x%04X\n", __func__,
			fw_ver_img, fw_ver_new);

	if (fw_ver_new != fw_ver_img) {
		if(!tskit_parade_data->parade_chip_data->check_fw_right_flag){
			TS_LOG_INFO("%s: built in fw does not match ic firmware, will upgrade\n",
					__func__);
			return 1;
		}else{
			if(tskit_parade_data->parade_chip_data->check_fw_right_flag != fw_ver_new){
				TS_LOG_ERR("%s: check_fw_right_flag is not valid,will not upgrade\n",
					__func__);
#if defined (CONFIG_HUAWEI_DSM)
				if (!dsm_client_ocuppy(ts_dclient)) {
					dsm_client_record(ts_dclient, "parade ts check_fw_right_flag error\n");
					dsm_client_notify(ts_dclient, DSM_TP_FW_CRC_ERROR_NO);
				}
#endif
				return 0;
			}
		}
	}

	fw_revctrl_img = tskit_parade_data->sysinfo.cydata.revctrl;

	TS_LOG_INFO("%s: img revctrl:0x%04X new revctrl:0x%04X\n",
			__func__, fw_revctrl_img, fw_revctrl_new);

	if (fw_revctrl_new != fw_revctrl_img) {
		TS_LOG_INFO("%s: built in fw does not match ic firmware, will upgrade\n",
                                 __func__);
		return 1;
	}
	TS_LOG_INFO("%s: built in fw  match ic firmware, will not upgrade\n",
        		 __func__);

	return 0;
}


static int parade_check_firmware_version_builtin(const struct firmware *fw)
{
	u32 fw_ver_new;
	u32 fw_revctrl_new;
	int upgrade;

	if (fw == NULL) {
		TS_LOG_ERR("%s: NULL firmware received, update FW\n", __func__);
		return true;
	}
	if (fw->data == NULL || fw->size == 0) {
		TS_LOG_ERR("%s: No firmware received, update FW\n", __func__);
		return true;
	}

	TS_LOG_INFO("%s: Found firmware\n", __func__);

	fw_ver_new = get_unaligned_be16(fw->data + 3);
	/* 4 middle bytes are not used */
	fw_revctrl_new = get_unaligned_be32(fw->data + 9);

	upgrade = parade_check_firmware_version(fw_ver_new, fw_revctrl_new);

	if (upgrade > 0)
		return 1;

	return 0;
}

static int parade_need_fw_update(void)
{
	int rc = NO_ERR;
	bool fw_update = false;
	struct device *dev = tskit_parade_data->dev;
	char  tmp_filename[4*MAX_STR_LEN] = {0};
	TS_LOG_INFO("%s enter\n", __func__);

	snprintf(tmp_filename, sizeof(tmp_filename), "ts/%s.bin",tskit_parade_data->fw_and_config_name);
	TS_LOG_INFO("%s request fw %s start\n", __func__,tmp_filename );
	rc = request_firmware(&fw_entry, tmp_filename , dev);
	if (rc < 0) {
		TS_LOG_ERR("%s: Fail request firmware\n", __func__);
#ifdef CONFIG_HUAWEI_DSM
		tskit_parade_data->parade_chip_data->ts_platform_data->dsm_info.constraints_UPDATE_status = FWU_REQUEST_FW_FAIL;
#endif/*CONFIG_HUAWEI_DSM*/
		return NO_UPDATA;
	}
	if (tskit_parade_data->force_fw_update == true) {
		TS_LOG_INFO("%s: force to update fw\n", __func__);
		return NEED_UPDATA;
	}

	fw_update = parade_check_firmware_version_builtin(fw_entry);

	if (fw_update == true) {
		TS_LOG_INFO("%s: need to update fw\n", __func__);
		rc = NEED_UPDATA;
	} else {
		TS_LOG_INFO("%s: not need to update fw\n", __func__);
		rc = NO_UPDATA;
		if (fw_entry != NULL) {
			release_firmware(fw_entry);
			fw_entry = NULL;
		}
	}

	return rc;
}

static ssize_t cyttsp5_manual_upgrade_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t rsize;
	int rc = 0;
	rc = _parade_fw_update_from_sdcard(PARADE_FW_FROM_SD_NAME,1);
	if (rc < 0) {
		TS_LOG_ERR("%s: fw update from sdcard failed.\n", __func__);
		rsize = snprintf(buf, 1024,
		"fw update from sdcard failed,rc=%d\n",rc);
	}
	else{
		TS_LOG_INFO("%s: fw update from sdcard is successful.\n", __func__);
		rsize = snprintf(buf, 1024,
		"fw update from sdcard is successful,rc=%d\n",rc);
		rc = parade_startup(tskit_parade_data,false);
		if(rc)
			TS_LOG_ERR("%s: Startup Error after FW upgrade\n",
					__func__);
		else
			TS_LOG_INFO("%s: Start up successfully after FW upgrade\n",
					__func__);
	}
	return rsize;
}

static ssize_t cyttsp5_manual_upgrade_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct parade_loader_data *ld = &tskit_parade_data->ld;
	int rc;
	TS_LOG_INFO("%s: enter\n", __func__);

	if (ld->is_manual_upgrade_enabled)
		return -EBUSY;

	ld->is_manual_upgrade_enabled = 1;

	rc = upgrade_firmware_from_class(dev);

	if (rc < 0)
		ld->is_manual_upgrade_enabled = 0;

	return size;
}

static DEVICE_ATTR(manual_upgrade, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IWGRP | S_IRGRP, cyttsp5_manual_upgrade_show, cyttsp5_manual_upgrade_store);



static struct attribute *parade_attributes[] = {
	&dev_attr_manual_upgrade.attr,
	NULL
};

static const struct attribute_group parade_attr_group = {
	.attrs = parade_attributes,
};


static int parade_loader_probe(struct parade_core_data *cd)
{
	struct parade_loader_data *ld = &tskit_parade_data->ld;

	int rc;
	rc = device_create_file(cd->dev, &dev_attr_manual_upgrade);
	if (rc) {
		TS_LOG_ERR("%s: Error, could not create forced_upgrade\n",
				__func__);
		goto error_create_manual_upgrade;
	}

	ld->loader_pdata = cd->loader_pdata;
	ld->dev = cd->dev;

	TS_LOG_INFO("%s: Successful probe\n", __func__);
	return 0;

error_create_manual_upgrade:
	device_remove_file(cd->dev, &dev_attr_manual_upgrade);
	return rc;
}

static int parade_hid_output_set_param_(struct parade_core_data *cd,
		u8 param_id, u32 value, u8 size)
{
	u8 write_buf[6];
	u8 *ptr = &write_buf[2];
	int rc;
	int i;
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_SET_PARAM),
		.write_buf = write_buf,
	};

	write_buf[0] = param_id;
	write_buf[1] = size;
	for (i = 0; i < size; i++) {
		ptr[i] = value & 0xFF;
		value = value >> 8;
	}

	hid_output.write_length = 2 + size;

	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	if (rc)
		return rc;

	if (param_id != cd->response_buf[5] || size != cd->response_buf[6])
		return -EPROTO;

	return 0;
}

static int parade_hid_output_calibrate_idacs_(struct parade_core_data *cd,
		u8 mode, u8 *status)
{
	int rc;
	int write_length = 1;
	u8 write_buf[1];
	u8 cmd_offset = 0;
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_CALIBRATE_IDACS),
		.write_length = write_length,
		.write_buf = write_buf,
		.timeout_ms = CY_HID_OUTPUT_CALIBRATE_IDAC_TIMEOUT,
	};

	write_buf[cmd_offset++] = mode;
	rc =  parade_hid_send_output_and_wait_(cd, &hid_output);
	if (rc)
		return rc;

	*status = cd->response_buf[5];
	if (*status)
		return -EINVAL;

	return 0;
}

static int parade_hid_output_run_selftest_(
		struct parade_core_data *cd, u8 test_id,
		u8 write_idacs_to_flash, u8 *status, u8 *summary_result,
		u8 *results_available)
{
	int rc;
	u8 write_buf[2];
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_RUN_SELF_TEST),
		.write_length = 1,
		.write_buf = write_buf,
		.timeout_ms = CY_HID_OUTPUT_RUN_SELF_TEST_TIMEOUT,
	};

	write_buf[0] = test_id;
	//write_buf[1] = write_idacs_to_flash;

	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	if (rc)
		return rc;

	if (status)
		*status = cd->response_buf[5];
	if (summary_result)
		*summary_result = cd->response_buf[6];
	if (results_available)
		*results_available = cd->response_buf[7];

	return rc;
}

static int parade_hid_output_get_selftest_result_(
		struct parade_core_data *cd, u16 read_offset, u16 read_length,
		u8 test_id, u8 *status, u16 *actual_read_len, u8 *data)
{
	int rc;
	u16 total_read_len = 0;
	u16 read_len;
	u16 off_buf = 0;
	u8 write_buf[5];
	u8 read_test_id;
	bool repeat;
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_GET_SELF_TEST_RESULT),
		.write_length = 5,
		.write_buf = write_buf,
	};

	/*
	 * Do not repeat reading for Auto Shorts test
	 * when PIP version < 1.3
	 */
	repeat = IS_PIP_VER_GE(&cd->sysinfo, 1, 3)
			|| test_id != CY_ST_ID_AUTOSHORTS;

again:
	write_buf[0] = LOW_BYTE(read_offset);
	write_buf[1] = HI_BYTE(read_offset);
	write_buf[2] = LOW_BYTE(read_length);
	write_buf[3] = HI_BYTE(read_length);
	write_buf[4] = test_id;

	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	if (rc)
		return rc;

	if (cd->response_buf[5] != CY_CMD_STATUS_SUCCESS)
		goto set_status;

	read_test_id = cd->response_buf[6];
	if (read_test_id != test_id)
		return -EPROTO;

	read_len = get_unaligned_le16(&cd->response_buf[7]);
	if (read_len && data) {
		memcpy(&data[off_buf], &cd->response_buf[10], read_len);

		total_read_len += read_len;

		if (repeat && read_len < read_length) {
			read_offset += read_len;
			off_buf += read_len;
			read_length -= read_len;
			goto again;
		}
	}

	if (actual_read_len)
		*actual_read_len = total_read_len;
set_status:
	if (status)
		*status = cd->response_buf[5];

	return rc;
}



static ssize_t parade_run_and_get_selftest_result_noprint(struct parade_core_data *cd,
		char *buf, size_t buf_len, u8 test_id, u16 read_length,
		bool get_result_on_pass)
{
	int status = STATUS_FAIL;
	u8 cmd_status = 0;
	u8 summary_result = 0;
	u16 act_length = 0;
	int length = 0;
	int rc;

	mutex_lock(&cd->dad.sysfs_lock);

	rc = parade_hid_output_suspend_scanning_(cd);
	if (rc < 0) {
		TS_LOG_ERR("%s: Error on suspend scan r=%d\n",
				__func__, rc);
		goto exit;
	}

	rc = parade_hid_output_run_selftest_(cd, test_id, 0,
			&cmd_status, &summary_result, NULL);
	if (rc < 0) {
		TS_LOG_ERR("%s: Error on run self test for test_id:%d r=%d\n",
				__func__, test_id, rc);
		goto resume_scan;
	}

	/* Form response buffer */
	cd->dad.ic_buf[0] = cmd_status;
	cd->dad.ic_buf[1] = summary_result;

	length = 2;

	/* Get data if command status is success */
	if (cmd_status != CY_CMD_STATUS_SUCCESS)
		goto status_success;

	/* Get data unless test result is pass */
	if (summary_result == CY_ST_RESULT_PASS && !get_result_on_pass)
		goto status_success;

	rc = parade_hid_output_get_selftest_result_(cd, 0, read_length,
			test_id, &cmd_status, &act_length, &cd->dad.ic_buf[6]);
	if (rc < 0) {
		TS_LOG_ERR("%s: Error on get self test result r=%d\n",
				__func__, rc);
		goto resume_scan;
	}

	cd->dad.ic_buf[2] = cmd_status;
	cd->dad.ic_buf[3] = test_id;
	cd->dad.ic_buf[4] = LOW_BYTE(act_length);
	cd->dad.ic_buf[5] = HI_BYTE(act_length);

	length = 6 + act_length;

status_success:
	status = STATUS_SUCCESS;

resume_scan:
	rc = parade_hid_output_resume_scanning_(cd);
	if (rc) {
		TS_LOG_ERR("%s: resume_scanning failed\n", __func__);
	}

exit:
	mutex_unlock(&cd->dad.sysfs_lock);

	return status;
}

/* Get basic information, like tx, rx, button number */
static void cmcp_get_basic_info(struct test_case_field *field_array, struct configuration *config)
{
#define CMCP_DEBUG 0
	u32 tx_num = 0;
#if CMCP_DEBUG
	u32 index = 0;
#endif

	config->is_valid_or_not = 1; /* Set to valid by default */
	config->cm_enabled = 0;
	config->cp_enabled = 0;

	if (field_array[CM_TEST_INPUTS].exist_or_not)
		config->cm_enabled = 1;
	if (field_array[CP_TEST_INPUTS].exist_or_not)
		config->cp_enabled = 1;

	/* Get basic information only when CM and CP are enabled */
	if (config->cm_enabled && config->cp_enabled) {
		TS_LOG_DEBUG("%s: Find CM and CP thresholds\n", __func__);

		config->rx_num =
			field_array[PER_ELEMENT_MIN_MAX_TABLE_SENSOR].line_num;
		tx_num =
		(field_array[PER_ELEMENT_MIN_MAX_TABLE_SENSOR].data_num >> 1)
			/field_array[PER_ELEMENT_MIN_MAX_TABLE_SENSOR].line_num;
		config->tx_num = tx_num;

		config->cm_min_max_table_sensor_size =
			field_array[PER_ELEMENT_MIN_MAX_TABLE_SENSOR].data_num;
		config->cp_min_max_table_rx_size =
			field_array[PER_ELEMENT_MIN_MAX_RX].data_num;
		config->cp_min_max_table_tx_size =
			field_array[PER_ELEMENT_MIN_MAX_TX].data_num;
		config->cm_max_table_gradient_cols_percent_size =
			field_array[CM_GRADIENT_CHECK_COL].data_num;
		config->cm_max_table_gradient_rows_percent_size =
			field_array[CM_GRADIENT_CHECK_ROW].data_num;

#if CMCP_DEBUG
		TS_LOG_DEBUG("%d\n", config->cm_excluding_col_edge);
		TS_LOG_DEBUG("%d\n", config->cm_excluding_row_edge);
		for (index = 0;
		index < config->cm_max_table_gradient_cols_percent_size;
		index++)
			TS_LOG_DEBUG("%d\n",
			config->cm_max_table_gradient_cols_percent[index]);
		for (index = 0;
		index < config->cm_max_table_gradient_rows_percent_size;
		index++)
			TS_LOG_DEBUG("%d\n",
			config->cm_max_table_gradient_rows_percent[index]);
		TS_LOG_DEBUG("%d\n", config->cm_range_limit_row);
		TS_LOG_DEBUG("%d\n", config->cm_range_limit_col);
		TS_LOG_DEBUG("%d\n", config->cm_min_limit_cal);
		TS_LOG_DEBUG("%d\n", config->cm_max_limit_cal);
		TS_LOG_DEBUG("%d\n", config->cm_max_delta_sensor_percent);
		TS_LOG_DEBUG("%d\n", config->cm_max_delta_button_percent);

		for (index = 0;
			index < config->cm_min_max_table_sensor_size; index++)
			TS_LOG_DEBUG("%d\n",
			config->cm_min_max_table_sensor[index]);
		TS_LOG_DEBUG("%d\n",
		config->cp_max_delta_sensor_rx_percent);
		TS_LOG_DEBUG("%d\n",
		config->cp_max_delta_sensor_tx_percent);

		for (index = 0;
		index < config->cp_min_max_table_rx_size; index++)
			TS_LOG_DEBUG("%d\n",
			config->cp_min_max_table_rx[index]);
		for (index = 0;
		index < config->cp_min_max_table_tx_size; index++)
			TS_LOG_DEBUG("%d\n",
			config->cp_min_max_table_tx[index]);
#endif
		/* Invalid mutual data length */
		if ((field_array[PER_ELEMENT_MIN_MAX_TABLE_SENSOR].data_num >>
		1) % field_array[PER_ELEMENT_MIN_MAX_TABLE_SENSOR].line_num) {
			config->is_valid_or_not = 0;
			TS_LOG_DEBUG("Invalid mutual data length\n");
		}
	} else {
		if (!config->cm_enabled)
			TS_LOG_DEBUG("%s: Miss CM thresholds or CM data format is wrong!\n",
				__func__);

		if (!config->cp_enabled)
			TS_LOG_DEBUG("%s: Miss CP thresholds or CP data format is wrong!\n",
				__func__);

		config->rx_num = 0;
		config->tx_num = 0;
		config->is_valid_or_not = 0;
	}

	TS_LOG_DEBUG("%s:\n"
		"Input file is %s!\n"
		"CM test: %s\n"
		"CP test: %s\n"
		"rx_num is %d\n"
		"tx_num is %d\n",
		__func__,
		config->is_valid_or_not == 1 ? "VALID" : "!!! INVALID !!!",
		config->cm_enabled == 1 ? "Found" : "Not found",
		config->cp_enabled == 1 ? "Found" : "Not found",
		config->rx_num,
		config->tx_num);

}

static void cmcp_test_case_field_init(struct test_case_field *test_field_array,
	struct configuration *configs)
{
	if(NULL == test_field_array || NULL == configs) {
		TS_LOG_ERR("%s: param NULL\n", __func__);
		return;
	}

	u32 CmRangeLimitType = TEST_CASE_TYPE_ONE;
	u32 *CmRangeLimitRowBuf = &configs->cm_range_limit_row;
	u32 *CmRangeLimitColBuf = &configs->cm_range_limit_col;

	if(FLAG_EXIST == tskit_parade_data->cm_delta_lattice_flag){
		CmRangeLimitType = TEST_CASE_TYPE_MUL_LINES;
		CmRangeLimitRowBuf = &configs->cm_range_limit_row_lattice[0];
		CmRangeLimitColBuf = &configs->cm_range_limit_col_lattice[0];
	}

	struct test_case_field test_case_field_array[MAX_CASE_NUM] = {
		{"CM TEST INPUTS", 14, TEST_CASE_TYPE_NO,
				NULL, 0, 0, 0},
		{"CM_EXCLUDING_COL_EDGE", 21, TEST_CASE_TYPE_ONE,
				&configs->cm_excluding_col_edge, 0, 0, 0},
		{"CM_EXCLUDING_ROW_EDGE", 21, TEST_CASE_TYPE_ONE,
				&configs->cm_excluding_row_edge, 0, 0, 0},
		{"CM_GRADIENT_CHECK_COL", 21, TEST_CASE_TYPE_MUL,
				&configs->cm_max_table_gradient_cols_percent[0],
				0, 0, 0},
		{"CM_GRADIENT_CHECK_ROW", 21, TEST_CASE_TYPE_MUL,
				&configs->cm_max_table_gradient_rows_percent[0],
				0, 0, 0},
		{"DIFF_NOISE_THRESHOLD", 20, TEST_CASE_TYPE_ONE,
				&configs->diff_noise_threshold, 0, 0, 0},
		{"DIFF_SELF_RX_NOISE_THRESHOLD", CY_RAW_RX_NUM, TEST_CASE_TYPE_ONE,
				&configs->diff_self_rx_noise_threshold, CY_CASE_EXIST, CY_DATA_NUM, CY_LINE_NUM},
		{"DIFF_SELF_TX_NOISE_THRESHOLD", CY_RAW_TX_NUM, TEST_CASE_TYPE_ONE,
				&configs->diff_self_tx_noise_threshold, CY_CASE_EXIST, CY_DATA_NUM, CY_LINE_NUM},
		{"CM_RANGE_LIMIT_ROW", 18, CmRangeLimitType,
				CmRangeLimitRowBuf, 0, 0, 0},
		{"CM_RANGE_LIMIT_COL", 18, CmRangeLimitType,
				CmRangeLimitColBuf, 0, 0, 0},
		{"CM_MIN_LIMIT_CAL", 16, TEST_CASE_TYPE_ONE,
				&configs->cm_min_limit_cal, 0, 0, 0},
		{"CM_MAX_LIMIT_CAL", 16, TEST_CASE_TYPE_ONE,
				&configs->cm_max_limit_cal, 0, 0, 0},
		{"CM_MAX_DELTA_SENSOR_PERCENT", 27, TEST_CASE_TYPE_ONE,
				&configs->cm_max_delta_sensor_percent, 0, 0, 0},
		{"CM_MAX_DELTA_BUTTON_PERCENT", 27, TEST_CASE_TYPE_ONE,
				&configs->cm_max_delta_button_percent, 0, 0, 0},
		{"PER_ELEMENT_MIN_MAX_TABLE_BUTTON", 32, TEST_CASE_TYPE_MUL,
				&configs->cm_min_max_table_button[0], 0, 0, 0},
		{"PER_ELEMENT_MIN_MAX_TABLE_SENSOR", 32,
				TEST_CASE_TYPE_MUL_LINES,
				&configs->cm_min_max_table_sensor[0], 0, 0, 0},
		{"CP TEST INPUTS", 14, TEST_CASE_TYPE_NO,
				NULL, 0, 0, 0},
		{"CP_PER_ELEMENT_MIN_MAX_BUTTON", 29, TEST_CASE_TYPE_MUL,
				&configs->cp_min_max_table_button[0], 0, 0, 0},
		{"CP_MAX_DELTA_SENSOR_RX_PERCENT", 30, TEST_CASE_TYPE_ONE,
				&configs->cp_max_delta_sensor_rx_percent,
				0, 0, 0},
		{"CP_MAX_DELTA_SENSOR_TX_PERCENT", 30, TEST_CASE_TYPE_ONE,
				&configs->cp_max_delta_sensor_tx_percent,
				0, 0, 0},
		{"CP_MAX_DELTA_BUTTON_PERCENT", 27, TEST_CASE_TYPE_ONE,
				&configs->cp_max_delta_button_percent, 0, 0, 0},
		{"MIN_BUTTON", 10, TEST_CASE_TYPE_ONE,
				&configs->min_button, 0, 0, 0},
		{"MAX_BUTTON", 10, TEST_CASE_TYPE_ONE,
				&configs->max_button, 0, 0, 0},
		{"PER_ELEMENT_MIN_MAX_RX", 22, TEST_CASE_TYPE_MUL,
				&configs->cp_min_max_table_rx[0], 0, 0, 0},
		{"PER_ELEMENT_MIN_MAX_TX", 22, TEST_CASE_TYPE_MUL,
				&configs->cp_min_max_table_tx[0], 0, 0, 0},
	};

	memcpy(test_field_array, test_case_field_array,
		sizeof(struct test_case_field) * MAX_CASE_NUM);
}

/* Return the buffer offset of new test case */
static u32 cmcp_return_offset_of_new_case(const char *bufPtr, u32 first_time, const char* pFileEnd)
{
	static u32 offset, first_search;

	if (first_time == 0) {
		first_search = 0;
		offset = 0;
	}

	if (first_search != 0) {
		for (;;) {
			/* Search ASCII_LF */
			while (bufPtr < pFileEnd){
				if(*bufPtr++ != ASCII_LF){
					offset++;
				}
				else
					break;
			}
			if(bufPtr >= pFileEnd)
				break;
			offset++;
			/* Single line: end loop
			 * Multiple lines: continue loop */

			if (*bufPtr != ASCII_COMMA)
				break;
		}
	} else
		first_search = 1;

	return offset;
}


/* Get test case information from cmcp threshold file */
static u32 cmcp_get_case_info_from_threshold_file(const char *buf,
		struct test_case_search *search_array, u32 file_size)
{
	u32 case_num = 0, buffer_offset = 0, name_count = 0, first_search = 0;
	char *pFileEnd = buf + file_size;
	TS_LOG_INFO("%s: Search cmcp threshold file, file start p=%p,end p=%p\n", __func__,buf,pFileEnd);

	/* Get all the test cases */
	for (case_num = 0; case_num < MAX_CASE_NUM; case_num++) {
		buffer_offset =
			cmcp_return_offset_of_new_case(&buf[buffer_offset],
			first_search,pFileEnd);
		first_search = 1;

		if (buf[buffer_offset] == 0)
			break;

		for (name_count = 0; name_count < NAME_SIZE_MAX; name_count++) {
			/* File end */
			if (buf[buffer_offset + name_count] == ASCII_COMMA)
				break;

			search_array[case_num].name[name_count] =
					buf[buffer_offset + name_count];
		}

		/* Exit when buffer offset is larger than file size */
		if (buffer_offset >= file_size)
			break;

		search_array[case_num].name_size = name_count;
		search_array[case_num].offset = buffer_offset;
		/*
		dev_vdbg(dev,
	"Find case %d: Name is %s; Name size is %d; Case offset is %d\n",
			case_num,
			search_array[case_num].name,
			search_array[case_num].name_size,
			search_array[case_num].offset);
			*/
	}

	return case_num;
}

/* Compose one value based on data of each bit */
static int cmcp_compose_data(char *buf, u32 count)
{
	u32 base_array[] = {1, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9};
	int value = 0;
	u32 index = 0;

	for (index = 0; index < count; index++)
		value += buf[index] * base_array[count - 1 - index];
	return value;
}



static int cmcp_return_one_value(const char *buf, u32 *offset, u32 *line_num)
{
	int value = -1;
	char tmp_buffer[10];
	u32 count = 0;
	u32 tmp_offset = *offset;

	/* Bypass extra commas */
	while (buf[tmp_offset] == ASCII_COMMA
			&& buf[tmp_offset + 1] == ASCII_COMMA){
			tmp_offset++;
	}

	/* Windows and Linux difference at the end of one line */
	if (buf[tmp_offset] == ASCII_COMMA
			&& buf[tmp_offset + 1] == ASCII_CR
			&& buf[tmp_offset + 2] == ASCII_LF)
		tmp_offset += 2;
	else if (buf[tmp_offset] == ASCII_COMMA
			&& buf[tmp_offset + 1] == ASCII_LF)
		tmp_offset += 1;

	/* New line for multiple lines */
	if (buf[tmp_offset] == ASCII_LF && buf[tmp_offset + 1] == ASCII_COMMA) {
		tmp_offset++;
		parade_csv_line_count ++;
		/*dev_vdbg(dev, "\n");*/
	}

	/* Beginning */
	if (buf[tmp_offset] == ASCII_COMMA) {
		tmp_offset++;
		for (;;) {
			if ((buf[tmp_offset] >= ASCII_ZERO)
			&& (buf[tmp_offset] <= ASCII_NINE)) {
				tmp_buffer[count++] =
				buf[tmp_offset] - ASCII_ZERO;
				tmp_offset++;
			} else {
				if (count != 0) {
					value = cmcp_compose_data(tmp_buffer,
					count);
					/*dev_vdbg(dev, ",%d", value);*/
				} else {
					/* 0 indicates no data available */
					value = -1;
				}
				break;
			}
		}
	} else {
		/* Multiple line: line count */
		*line_num = parade_csv_line_count ;
		/* Reset for next case */
		parade_csv_line_count  = 1;
	}

	*offset = tmp_offset;

	return value;
}


/* Get configuration information */
static void cmcp_get_configuration_info(	const char *buf, struct test_case_search *search_array,
		u32 case_count, struct test_case_field *field_array,
		struct configuration *config)
{
	u32 count = 0, sub_count = 0;
	u32 exist_or_not = 0;
	u32 value_offset = 0;
	int retval = 0;
	u32 data_num = 0;
	u32 line_num = 1;

	TS_LOG_DEBUG("%s: Fill configuration struct per cmcp threshold file\n",
		__func__);

	/* Search cases */
	for (count = 0; count < MAX_CASE_NUM; count++) {
		exist_or_not = 0;
		for (sub_count = 0; sub_count < case_count; sub_count++) {
			if (!strncmp(field_array[count].name,
					search_array[sub_count].name,
					field_array[count].name_size)) {
				exist_or_not = 1;
				break;
			}
		}

		field_array[count].exist_or_not = exist_or_not;

		/* Clear data number */
		data_num = 0;

		if (exist_or_not == 1) {
			switch (field_array[count].type) {
			case TEST_CASE_TYPE_NO:
				field_array[count].data_num = 0;
				field_array[count].line_num = 1;
				break;
			case TEST_CASE_TYPE_ONE:
				value_offset = search_array[sub_count].offset
					+ search_array[sub_count].name_size;
				*field_array[count].bufptr =
						cmcp_return_one_value(buf,
						&value_offset, &line_num);
				field_array[count].data_num = 1;
				field_array[count].line_num = 1;
				break;
			case TEST_CASE_TYPE_MUL:
			case TEST_CASE_TYPE_MUL_LINES:
				line_num = 1;
				value_offset = search_array[sub_count].offset
					+ search_array[sub_count].name_size;
				for (;;) {
					retval = cmcp_return_one_value(buf, &value_offset, &line_num);
					if (retval >= 0) {
						*field_array[count].bufptr++ =
						retval;
						data_num++;
					} else
						break;
				}

				field_array[count].data_num = data_num;
				field_array[count].line_num = line_num;
				break;
			default:
				break;
			}
			TS_LOG_DEBUG("%s: %s: Data number is %d, line number is %d\n",
			__func__,
			field_array[count].name,
			field_array[count].data_num,
			field_array[count].line_num);
		} else
			TS_LOG_DEBUG("%s: !!! %s doesn't exist\n",
				__func__, field_array[count].name);
	}
}


/* Compose one value based on data of each bit */

static ssize_t parade_parse_cmcp_threshold_file_common(
	const char *buf, u32 file_size)
{
	struct parade_device_access_data *dad = &tskit_parade_data->dad;

	ssize_t rc = 0;
	u32 case_count = 0;

	TS_LOG_DEBUG("%s: Start parsing cmcp threshold file. File size is %d\n",
		__func__, file_size);
	parade_csv_line_count = 1;
	cmcp_test_case_field_init(dad->test_field_array, dad->configs);

	/* Get all the cases from .csv file */
	case_count = cmcp_get_case_info_from_threshold_file(buf, dad->test_search_array, file_size);
	if(case_count == 0)	{
		rc = -1;
		goto exit;
	}
	/* Search cases */
	cmcp_get_configuration_info(buf,
		dad->test_search_array, case_count, dad->test_field_array,
		dad->configs);

	/* Get basic information */
	cmcp_get_basic_info(dad->test_field_array, dad->configs);

exit:
	parade_csv_line_count = 1;
	return rc;
}

static int parade_cmcp_parse_threshold_file(void)
{
	int retval = 0;

	struct parade_core_data *cd = tskit_parade_data;

	char  tmp_filename[4*MAX_STR_LEN] = {0};
	TS_LOG_INFO("%s enter\n", __func__);

	if(tskit_parade_data->fw_need_depend_on_lcd) {//limit depen on lcd module name
		snprintf(tmp_filename, sizeof(tmp_filename) -1, "ts/%s_%s_%s_%s_%s_limits.csv",g_ts_kit_platform_data.product_name,
			PARADE_VENDER_NAME,tskit_parade_data->project_id,tskit_parade_data->module_vendor, tskit_parade_data->lcd_module_name);
	} else {
		snprintf(tmp_filename, sizeof(tmp_filename) -1, "ts/%s_%s_%s_%s_limits.csv",g_ts_kit_platform_data.product_name,
			PARADE_VENDER_NAME,tskit_parade_data->project_id,tskit_parade_data->module_vendor);
	}
	TS_LOG_INFO("%s request threshold %s start\n", __func__,tmp_filename );

	retval = request_firmware(&fw_entry, tmp_filename, cd->dev);
	if (retval < 0) {
		TS_LOG_ERR("%s: Fail request firmware\n", __func__);
		goto exit;
	}
	if (fw_entry == NULL) {
		TS_LOG_ERR("%s: fw_entry == NULL\n", __func__);
		retval = -1;
		goto exit;
	}
	if (fw_entry->data == NULL || fw_entry->size == 0) {
		TS_LOG_ERR("%s: No firmware received\n", __func__);
		retval = -2;
		goto exit;
	}
	TS_LOG_INFO("%s: fw_entry->size = %zu\n", __func__, fw_entry->size);

	TS_LOG_DEBUG("%s: Found cmcp threshold file.\n", __func__);

	retval = parade_parse_cmcp_threshold_file_common(&fw_entry->data[0], fw_entry->size);
	if (retval < 0) {
		TS_LOG_ERR("%s: Parse Cmcp file\n", __func__);
		retval = -3;
		goto exit;
	}
exit:
	release_firmware(fw_entry);
	return retval;

}

static int validate_cp_test_results(struct parade_core_data *cd,
	struct configuration *configuration, struct cmcp_data *cmcp_info,
	struct result *result, bool *pass, int test_item)
{
	int i = 0;
	uint32_t configuration_rx_num;
	uint32_t configuration_tx_num;
	int32_t *cp_sensor_tx_data = cmcp_info->cp_tx_data_panel;
	int32_t *cp_sensor_rx_data = cmcp_info->cp_rx_data_panel;

	result->cp_test_pass = true;
	configuration_rx_num = cmcp_info->rx_num; //Fix bug just incase the threshold file parse fail
	configuration_tx_num = cmcp_info->tx_num;

	TS_LOG_INFO("%s start\n", __func__);

	//if ((test_item & CP_PANEL) == CP_PANEL) {
	if (1) {
		int32_t cp_sensor_tx_delta;
		int32_t cp_sensor_rx_delta;

		/* Check Sensor Cp delta for range limit */
		result->cp_sensor_delta_pass = true;
		if(cd->cp_delta_test_supported){
			TS_LOG_INFO("cp_sensor_delta test start\n");
			/*check cp_sensor_tx_delta */
			for (i = 0; i < configuration_tx_num; i++) {
				cp_sensor_tx_delta =
				ABS((cmcp_info->cp_tx_cal_data_panel[i]-
				cmcp_info->cp_tx_data_panel[i]) * 100 /
				cmcp_info->cp_tx_data_panel[i]);

				if (cp_sensor_tx_delta >
				configuration->cp_max_delta_sensor_tx_percent) {
					TS_LOG_ERR("%s: Cp_sensor_tx_delta:%d (%d)\n",
					"Cp sensor delta range limit test",
					cp_sensor_tx_delta,
				configuration->cp_max_delta_sensor_tx_percent);
					result->cp_sensor_delta_pass = false;
				}
			}

			/*check cp_sensor_rx_delta */
			for (i = 0; i < configuration_rx_num; i++) {
				cp_sensor_rx_delta =
				ABS((cmcp_info->cp_rx_cal_data_panel[i] -
				cmcp_info->cp_rx_data_panel[i]) * 100 /
				cmcp_info->cp_rx_data_panel[i]);
				if (cp_sensor_rx_delta >
				configuration->cp_max_delta_sensor_rx_percent) {
					TS_LOG_ERR("%s: Cp_sensor_rx_delta:%d(%d)\n",
					"Cp sensor delta range limit test",
					cp_sensor_rx_delta,
				configuration->cp_max_delta_sensor_rx_percent);
					result->cp_sensor_delta_pass = false;
				}
			}
		}
		/* Check sensor Cp rx for min/max values */
		result->cp_rx_validation_pass = true;
		for (i = 0; i < configuration_rx_num; i++) {
			int32_t cp_rx_min =
				configuration->cp_min_max_table_rx[i * 2];
			int32_t cp_rx_max =
				configuration->cp_min_max_table_rx[i * 2 + 1];
			if ((cp_sensor_rx_data[i] <= cp_rx_min)
				|| (cp_sensor_rx_data[i] >= cp_rx_max)) {
				TS_LOG_ERR("%s: Cp Rx[%d]:%d (%d,%d)\n",
				"Cp Rx min/max test",
				i,
				(int)cp_sensor_rx_data[i],
				cp_rx_min, cp_rx_max);
				result->cp_rx_validation_pass = false;
			}
		}

		/* Check sensor Cp tx for min/max values */
		result->cp_tx_validation_pass = true;
		for (i = 0; i < configuration_tx_num; i++) {
			int32_t cp_tx_min =
				configuration->cp_min_max_table_tx[i * 2];
			int32_t cp_tx_max =
				configuration->cp_min_max_table_tx[i * 2 + 1];
			if ((cp_sensor_tx_data[i] < cp_tx_min)
				|| (cp_sensor_tx_data[i] > cp_tx_max)) {
				TS_LOG_ERR("%s: Cp Tx[%d]:%d(%d,%d)\n",
				"Cp Tx min/max test",
				i,
				cp_sensor_tx_data[i],
				cp_tx_min, cp_tx_max);
				result->cp_tx_validation_pass = false;
			}
		}

		result->cp_test_pass = result->cp_test_pass
				&& result->cp_sensor_delta_pass
				&& result->cp_rx_validation_pass
				&& result->cp_tx_validation_pass;
	}


	if (pass)
		*pass = result->cp_test_pass;

	return 0;
}

#define LIMIT_INDEX(x)	((tskit_parade_data->cm_delta_lattice_flag)?(x-1):(x))

#define LIMIT_ROW(i,j)	((tskit_parade_data->cm_delta_lattice_flag)?(configuration->cm_range_limit_row_lattice[(j-1)*tx_num+i]):(configuration->cm_range_limit_row))

#define LIMIT_COL(i,j)	((tskit_parade_data->cm_delta_lattice_flag)?(configuration->cm_range_limit_col_lattice[j*(tx_num-1)+i-1]):(configuration->cm_range_limit_col))

static int validate_cm_test_results(struct parade_core_data *cd,
	struct configuration *configuration, struct cmcp_data *cmcp_info,
	struct result *result, bool *pass, int test_item)
{
	int32_t tx_num = cmcp_info->tx_num;
	int32_t rx_num = cmcp_info->rx_num;
	uint32_t sensor_num = tx_num * rx_num;
	int32_t *cm_sensor_data = cmcp_info->cm_data_panel;
	int32_t cm_sensor_calibration;
	struct gd_sensor *gd_sensor_col = cmcp_info->gd_sensor_col;
	struct gd_sensor *gd_sensor_row = cmcp_info->gd_sensor_row;
	int32_t *cm_sensor_column_delta = cmcp_info->cm_sensor_column_delta;
	int32_t *cm_sensor_row_delta = cmcp_info->cm_sensor_row_delta;
	int ret = 0;
	int i, j;

	TS_LOG_INFO("%s: start\n", __func__);

	if(test_item){
		TS_LOG_DEBUG("%s: Check each sensor Cm data for min max value\n ", __func__);

		/* Check each sensor Cm data for min/max values */
		result->cm_sensor_validation_pass = true;

		for (i = 0; i < sensor_num; i++) {
			int row = i % rx_num;
			int col = i / rx_num;
			int32_t cm_sensor_min =
			configuration->cm_min_max_table_sensor[(row*tx_num+col)*2];
			int32_t cm_sensor_max =
			configuration->cm_min_max_table_sensor[(row*tx_num+col)*2+1];
			if ((cm_sensor_data[i] < cm_sensor_min)
			|| (cm_sensor_data[i] > cm_sensor_max)) {
				TS_LOG_ERR("%s: Sensor[%d,%d]:%d (%d,%d)\n",
						"Cm sensor min/max test",
						row, col,
						cm_sensor_data[i],
						cm_sensor_min, cm_sensor_max);
				result->cm_sensor_validation_pass = false;
			}
		}

		/*check cm gradient column data*/
		result->cm_sensor_gd_col_pass = true;
		for (i = 0;
		i < configuration->cm_max_table_gradient_cols_percent_size;
		i++) {
			if ((gd_sensor_col + i)->gradient_val >
			10 * configuration->cm_max_table_gradient_cols_percent[i]){
				TS_LOG_ERR("%s: cm_max_table_gradient_cols_percent[%d]:%d, gradient_val:%d\n",
							__func__, i,
							configuration->cm_max_table_gradient_cols_percent[i],
							(gd_sensor_col + i)->gradient_val);
				result->cm_sensor_gd_col_pass = false;
			}
		}

		/*check cm gradient row data*/
		result->cm_sensor_gd_row_pass = true;
		for (j = 0;
		j < configuration->cm_max_table_gradient_rows_percent_size;
		j++) {
			if ((gd_sensor_row + j)->gradient_val >
			10 * configuration->cm_max_table_gradient_rows_percent[j]) {
				TS_LOG_ERR("%s: cm_max_table_gradient_rows_percent[%d]:%d, gradient_val:%d\n",
						__func__,
						j, configuration->cm_max_table_gradient_rows_percent[j],
						(gd_sensor_row + j)->gradient_val);
				result->cm_sensor_gd_row_pass = false;
			}
		}

		result->cm_sensor_row_delta_pass = true;
		result->cm_sensor_col_delta_pass = true;
		result->cm_sensor_calibration_pass = true;
		result->cm_sensor_delta_pass = true;

		/*
		Check each row Cm data
		with neighbor for difference
		*/
		for (i = 0; i < tx_num; i++) {
			for (j = 1; j < rx_num; j++) {
				int32_t cm_sensor_row_diff =
				ABS(cm_sensor_data[i * rx_num + j] -
				cm_sensor_data[i * rx_num + j - 1]);
				cm_sensor_row_delta[i * LIMIT_INDEX(rx_num) + j - 1] =
				cm_sensor_row_diff;
				if (cm_sensor_row_diff >
					LIMIT_ROW(i,j)) {
					TS_LOG_ERR("%s: Sensor[%d,%d]:%d (%d)\n",
									"Cm sensor row range limit test",
									LIMIT_INDEX(j), i,
									cm_sensor_row_diff,
									LIMIT_ROW(i,j));
					result->cm_sensor_row_delta_pass = false;
				}
			}
		}

		/* Check each column Cm data
		with neighbor for difference
		*/
		for (i = 1; i < tx_num; i++) {
			for (j = 0; j < rx_num; j++) {
				int32_t cm_sensor_col_diff =
				ABS((int)cm_sensor_data[i * rx_num + j] -
				(int)cm_sensor_data[(i - 1) * rx_num + j]);
				cm_sensor_column_delta[(i - 1) * rx_num + j] =
					cm_sensor_col_diff;
				if (cm_sensor_col_diff >
					LIMIT_COL(i,j)) {
					TS_LOG_ERR("%s: Sensor[%d,%d]:%d (%d)\n",
								"Cm sensor column range limit test",
								j, LIMIT_INDEX(i),
								cm_sensor_col_diff,
								LIMIT_COL(i,j));
					result->cm_sensor_col_delta_pass = false;
				}
			}
		}

		/* Check sensor calculated Cm for min/max values */
		cm_sensor_calibration = cmcp_info->cm_cal_data_panel;
		if (cm_sensor_calibration <
			configuration->cm_min_limit_cal
			|| cm_sensor_calibration >
			configuration->cm_max_limit_cal) {
			TS_LOG_ERR("%s: Cm_cal:%d (%d,%d)\n",
				"Cm sensor Cm_cal min/max test",
				cm_sensor_calibration,
				configuration->cm_min_limit_cal,
				configuration->cm_max_limit_cal);
			result->cm_sensor_calibration_pass = false;
		}

		/* Check sensor Cm delta for range limit */
		if (cmcp_info->cm_sensor_delta
			 > 10 * configuration->cm_max_delta_sensor_percent) {
			TS_LOG_ERR("%s: Cm_sensor_delta:%d (%d)\n",
				"Cm sensor delta range limit test",
				cmcp_info->cm_sensor_delta,
				configuration->cm_max_delta_sensor_percent);
			result->cm_sensor_delta_pass = false;
		}

		result->cm_test_pass = result->cm_sensor_gd_col_pass
				&& result->cm_sensor_gd_row_pass
				&& result->cm_sensor_validation_pass
				&& result->cm_sensor_row_delta_pass
				&& result->cm_sensor_col_delta_pass
				&& result->cm_sensor_calibration_pass
				&& result->cm_sensor_delta_pass;
		}
	if (pass)
		*pass = result->cm_test_pass;

	return ret;
}


static void calculate_gradient_row(struct gd_sensor *gd_sensor_row_head,
		 uint16_t row_num, int exclude_row_edge, int exclude_col_edge)
{
	int i = 0;
	uint16_t cm_min_cur = 0;
	uint16_t cm_max_cur = 0;
	uint16_t cm_ave_cur = 0;
	uint16_t cm_ave_next = 0;
	uint16_t cm_ave_prev = 0;
	struct gd_sensor *p = gd_sensor_row_head;

	if (exclude_row_edge) {
		for (i = 0; i < row_num; i++) {
			if (!exclude_col_edge) {
				cm_ave_cur = (p + i)->cm_ave;
				cm_min_cur = (p + i)->cm_min;
				cm_max_cur = (p + i)->cm_max;
				if (i < (row_num-1))
					cm_ave_next = (p + i+1)->cm_ave;
				if (i > 0)
					cm_ave_prev = (p + i-1)->cm_ave;
			} else {
				cm_ave_cur = (p + i)->cm_ave_exclude_edge;
				cm_min_cur = (p + i)->cm_min_exclude_edge;
				cm_max_cur = (p + i)->cm_max_exclude_edge;
				if (i < (row_num-1))
					cm_ave_next =
					(p + i+1)->cm_ave_exclude_edge;
				if (i > 0)
					cm_ave_prev =
					(p + i-1)->cm_ave_exclude_edge;
			}

			if (cm_ave_cur == 0)
				cm_ave_cur = 1;

			/*multiple 1000 to increate accuracy*/
			if ((i == 0) || (i == (row_num-1))) {
				(p + i)->gradient_val =
				(cm_max_cur - cm_min_cur) * 1000 /
				cm_ave_cur;
			} else if (i == 1) {
				(p + i)->gradient_val = (cm_max_cur - cm_min_cur
				+ ABS(cm_ave_cur - cm_ave_next)) * 1000 /
				cm_ave_cur;
			} else {
				(p + i)->gradient_val = (cm_max_cur - cm_min_cur
				+ ABS(cm_ave_cur - cm_ave_prev)) * 1000 /
				cm_ave_cur;
			}
		}
	} else if (!exclude_row_edge) {
		for (i = 0; i < row_num; i++) {
			if (!exclude_col_edge) {
				cm_ave_cur = (p + i)->cm_ave;
				cm_min_cur = (p + i)->cm_min;
				cm_max_cur = (p + i)->cm_max;
				if (i < (row_num-1))
					cm_ave_next = (p + i+1)->cm_ave;
				if (i > 0)
					cm_ave_prev = (p + i-1)->cm_ave;
			} else {
				cm_ave_cur = (p + i)->cm_ave_exclude_edge;
				cm_min_cur = (p + i)->cm_min_exclude_edge;
				cm_max_cur = (p + i)->cm_max_exclude_edge;
				if (i < (row_num-1))
					cm_ave_next =
					(p + i+1)->cm_ave_exclude_edge;
				if (i > 0)
					cm_ave_prev =
					(p + i-1)->cm_ave_exclude_edge;
			}

			if (cm_ave_cur == 0)
				cm_ave_cur = 1;
			/*multiple 1000 to increate accuracy*/
			if (i <= 1)
				(p + i)->gradient_val = (cm_max_cur - cm_min_cur
				+ ABS(cm_ave_cur - cm_ave_next)) * 1000 /
				cm_ave_cur;
			else
				(p + i)->gradient_val = (cm_max_cur - cm_min_cur
				+ ABS(cm_ave_cur - cm_ave_prev)) * 1000 /
				cm_ave_cur;
		}
	}
}

static void calculate_gradient_col(struct gd_sensor *gd_sensor_row_head,
	uint16_t col_num, int exclude_row_edge, int exclude_col_edge)
{
	int i = 0;
	int32_t cm_min_cur = 0;
	int32_t cm_max_cur = 0;
	int32_t cm_ave_cur = 0;
	int32_t cm_ave_next = 0;
	int32_t cm_ave_prev = 0;
	struct gd_sensor *p = gd_sensor_row_head;

	if (!exclude_col_edge) {
		for (i = 0; i < col_num; i++) {
			if (!exclude_row_edge) {
				cm_ave_cur = (p + i)->cm_ave;
				cm_min_cur = (p + i)->cm_min;
				cm_max_cur = (p + i)->cm_max;
				if (i < (col_num-1))
					cm_ave_next = (p + i+1)->cm_ave;
				if (i > 0)
					cm_ave_prev = (p + i-1)->cm_ave;
			} else {
				cm_ave_cur = (p + i)->cm_ave_exclude_edge;
				cm_min_cur = (p + i)->cm_min_exclude_edge;
				cm_max_cur = (p + i)->cm_max_exclude_edge;
				if (i < (col_num-1))
					cm_ave_next =
					(p + i+1)->cm_ave_exclude_edge;
				if (i > 0)
					cm_ave_prev =
					(p + i-1)->cm_ave_exclude_edge;
			}
			if (cm_ave_cur == 0)
				cm_ave_cur = 1;
			/*multiple 1000 to increate accuracy*/
			if (i <= 1)
				(p + i)->gradient_val = (cm_max_cur - cm_min_cur
				+ ABS(cm_ave_cur - cm_ave_next)) * 1000 /
				cm_ave_cur;
			else
				(p + i)->gradient_val = (cm_max_cur - cm_min_cur
				+ ABS(cm_ave_cur - cm_ave_prev)) * 1000 /
				cm_ave_cur;
		}
	} else if (exclude_col_edge) {
		for (i = 0; i < col_num; i++) {
			if (!exclude_row_edge) {
				cm_ave_cur = (p + i)->cm_ave;
				cm_min_cur = (p + i)->cm_min;
				cm_max_cur = (p + i)->cm_max;
				if (i < (col_num-1))
					cm_ave_next = (p + i+1)->cm_ave;
				if (i > 0)
					cm_ave_prev = (p + i-1)->cm_ave;
			} else {
				cm_ave_cur = (p + i)->cm_ave_exclude_edge;
				cm_min_cur = (p + i)->cm_min_exclude_edge;
				cm_max_cur = (p + i)->cm_max_exclude_edge;
				if (i < (col_num-1))
					cm_ave_next =
					(p + i+1)->cm_ave_exclude_edge;
				if (i > 0)
					cm_ave_prev =
					(p + i-1)->cm_ave_exclude_edge;
			}

			if (cm_ave_cur == 0)
				cm_ave_cur = 1;
			/*multiple 1000 to increate accuracy*/
			if ((i == 0) || (i == (col_num - 1)))
				(p + i)->gradient_val =
					 (cm_max_cur - cm_min_cur) * 1000 /
					 cm_ave_cur;
			else if (i == 1)
				(p + i)->gradient_val =
					(cm_max_cur - cm_min_cur +
					ABS(cm_ave_cur - cm_ave_next))
					 * 1000 / cm_ave_cur;
			else
				(p + i)->gradient_val =
					(cm_max_cur - cm_min_cur +
					ABS(cm_ave_cur - cm_ave_prev))
					* 1000 / cm_ave_cur;
			}
	}
}

static void fill_gd_sensor_table(struct gd_sensor *head, int32_t index,
	int32_t cm_max, int32_t cm_min,	int32_t cm_ave,
	int32_t cm_max_exclude_edge, int32_t cm_min_exclude_edge,
	int32_t cm_ave_exclude_edge)
{
	(head + index)->cm_max = cm_max;
	(head + index)->cm_min = cm_min;
	(head + index)->cm_ave = cm_ave;
	(head + index)->cm_ave_exclude_edge = cm_ave_exclude_edge;
	(head + index)->cm_max_exclude_edge = cm_max_exclude_edge;
	(head + index)->cm_min_exclude_edge = cm_min_exclude_edge;
}


static void calculate_gd_info(struct gd_sensor *gd_sensor_col,
	struct gd_sensor *gd_sensor_row, int tx_num, int rx_num,
	int32_t *cm_sensor_data, int cm_excluding_row_edge,
	int cm_excluding_col_edge)
{
	int32_t cm_max;
	int32_t cm_min;
	int32_t cm_ave;
	int32_t cm_max_exclude_edge;
	int32_t cm_min_exclude_edge;
	int32_t cm_ave_exclude_edge;
	int32_t cm_data;
	int i;
	int j;

	/*calculate all the gradient related info for column*/
	for (i = 0; i < tx_num; i++) {
		/*re-initialize for a new col*/
		cm_max = cm_sensor_data[i * rx_num];
		cm_min = cm_max;
		cm_ave = 0;
		cm_max_exclude_edge = cm_sensor_data[i * rx_num + 1];
		cm_min_exclude_edge = cm_max_exclude_edge;
		cm_ave_exclude_edge = 0;

		for (j = 0; j < rx_num; j++) {
			cm_data = cm_sensor_data[i * rx_num + j];
			if (cm_data > cm_max)
				cm_max = cm_data;
			if (cm_data < cm_min)
				cm_min = cm_data;
			cm_ave += cm_data;
			/*calculate exclude edge data*/
			if ((j > 0) && (j < (rx_num-1))) {
				if (cm_data > cm_max_exclude_edge)
					cm_max_exclude_edge = cm_data;
				if (cm_data < cm_min_exclude_edge)
					cm_min_exclude_edge = cm_data;
				cm_ave_exclude_edge += cm_data;
			}
		}
		cm_ave /= rx_num;
		cm_ave_exclude_edge /= (rx_num-2);
		fill_gd_sensor_table(gd_sensor_col, i, cm_max, cm_min, cm_ave,
		cm_max_exclude_edge, cm_min_exclude_edge, cm_ave_exclude_edge);
	}

	calculate_gradient_col(gd_sensor_col, tx_num, cm_excluding_row_edge,
		 cm_excluding_col_edge);

	/*calculate all the gradient related info for row*/
	for (j = 0; j < rx_num; j++) {
		/*re-initialize for a new row*/
		cm_max = cm_sensor_data[j];
		cm_min = cm_max;
		cm_ave = 0;
		cm_max_exclude_edge = cm_sensor_data[rx_num + j];
		cm_min_exclude_edge = cm_max_exclude_edge;
		cm_ave_exclude_edge = 0;
		for (i = 0; i < tx_num; i++) {
			cm_data = cm_sensor_data[i * rx_num + j];
			if (cm_data > cm_max)
				cm_max = cm_data;
			if (cm_data < cm_min)
				cm_min = cm_data;
			cm_ave += cm_data;
			/*calculate exclude edge data*/
			if ((i >  0) && (i < (tx_num-1))) {
				if (cm_data > cm_max_exclude_edge)
					cm_max_exclude_edge = cm_data;
				if (cm_data < cm_min_exclude_edge)
					cm_min_exclude_edge = cm_data;
				cm_ave_exclude_edge += cm_data;
			}
		}
		cm_ave /= tx_num;
		cm_ave_exclude_edge /= (tx_num-2);
		fill_gd_sensor_table(gd_sensor_row, j, cm_max, cm_min, cm_ave,
		cm_max_exclude_edge, cm_min_exclude_edge, cm_ave_exclude_edge);
	}
	calculate_gradient_row(gd_sensor_row, rx_num, cm_excluding_row_edge,
		 cm_excluding_col_edge);
}


static int  parade_get_cmcp_info(struct parade_core_data *cd,
	struct cmcp_data *cmcp_info)
{
	int32_t *cm_data_panel = cmcp_info->cm_data_panel;
	struct gd_sensor *gd_sensor_col = cmcp_info->gd_sensor_col;
	struct gd_sensor *gd_sensor_row = cmcp_info->gd_sensor_row;
	struct result *result = cd->dad.result;
	int32_t cm_ave_data_panel = 0;
	u8 tmp_buf[3];
	int tx_num;
	int rx_num;
	int32_t *cp_tx_data_panel = cmcp_info->cp_tx_data_panel;
	int32_t *cp_rx_data_panel = cmcp_info->cp_rx_data_panel;
	int32_t *cp_tx_cal_data_panel = cmcp_info->cp_tx_cal_data_panel;
	int32_t *cp_rx_cal_data_panel = cmcp_info->cp_rx_cal_data_panel;
	int32_t cp_tx_ave_data_panel = 0;
	int32_t cp_rx_ave_data_panel = 0;
	int rc = 0;
	int i;

	cmcp_info->tx_num = cd->sysinfo.sensing_conf_data.tx_num;
	cmcp_info->rx_num = cd->sysinfo.sensing_conf_data.rx_num;
	tx_num = cmcp_info->tx_num;
	rx_num = cmcp_info->rx_num;

	TS_LOG_INFO("%s tx_num=%d\n", __func__, tx_num);
	TS_LOG_INFO("%s rx_num=%d\n", __func__, rx_num);


	/*short test*/
	result->short_test_pass = true;
	rc = parade_run_and_get_selftest_result_noprint(
		cd, tmp_buf, sizeof(tmp_buf),
		CY_ST_ID_AUTOSHORTS, PIP_CMD_MAX_LENGTH, false);
	if (rc) {
		TS_LOG_ERR("%s: short test not supported\n", __func__);
		goto exit;
	}
	if (cd->dad.ic_buf[1] != 0)
		result->short_test_pass = false;
	/*Get cm_panel data*/
	rc = parade_run_and_get_selftest_result_noprint(
		cd, tmp_buf, sizeof(tmp_buf),
		CY_ST_ID_CM_PANEL, PIP_CMD_MAX_LENGTH, true);
	if (rc) {
		TS_LOG_ERR("%s: Get CM Panel not supported\n", __func__);
		goto exit;
	}
	if (cm_data_panel != NULL) {
		for (i = 0; i < tx_num * rx_num;  i++) {
			cm_data_panel[i] =
			10*(cd->dad.ic_buf[CM_PANEL_DATA_OFFSET+i*2] + 256
			* cd->dad.ic_buf[CM_PANEL_DATA_OFFSET+i*2+1]);
			TS_LOG_DEBUG("cm_data_panel[%d]=%d\n",
			i, cm_data_panel[i]);
			cm_ave_data_panel += cm_data_panel[i];
		}
		cm_ave_data_panel /= (tx_num * rx_num);
		cmcp_info->cm_ave_data_panel = cm_ave_data_panel;
		cmcp_info->cm_cal_data_panel =
		10*(cd->dad.ic_buf[CM_PANEL_DATA_OFFSET+i*2]
		+256 * cd->dad.ic_buf[CM_PANEL_DATA_OFFSET+i*2+1]);
		/*multiple 1000 to increate accuracy*/
		cmcp_info->cm_sensor_delta = ABS((cmcp_info->cm_ave_data_panel -
			cmcp_info->cm_cal_data_panel) * 1000 /
			cmcp_info->cm_ave_data_panel);
	}

	/*calculate gradient panel sensor column/row here*/
	calculate_gd_info(gd_sensor_col, gd_sensor_row, tx_num, rx_num,
		 cm_data_panel, 1, 1);
	TS_LOG_DEBUG("%s: TX gradient:\n", __func__);
	for (i = 0; i < tx_num; i++) {
		TS_LOG_DEBUG("i=%d max=%d,min=%d,ave=%d, gradient=%d\n",
			i, gd_sensor_col[i].cm_max, gd_sensor_col[i].cm_min,
			gd_sensor_col[i].cm_ave, gd_sensor_col[i].gradient_val);
	}
	TS_LOG_DEBUG("%s: RX gradient:\n", __func__);
	for (i = 0; i < rx_num; i++) {
		TS_LOG_DEBUG("i=%d max=%d,min=%d,ave=%d, gradient=%d\n",
			i, gd_sensor_row[i].cm_max, gd_sensor_row[i].cm_min,
			gd_sensor_row[i].cm_ave, gd_sensor_row[i].gradient_val);
	}
	/*after get cm data,there is some initial and fast calibration,
	delay 200ms before the next command*/
	if(cd->need_wait_after_cm_test)
		msleep(200);
	/*Get cp data*/
	rc = parade_run_and_get_selftest_result_noprint(
		cd, tmp_buf, sizeof(tmp_buf),
		CY_ST_ID_CP_PANEL, PIP_CMD_MAX_LENGTH, true);
	if (rc) {
		TS_LOG_ERR( "Get CP Panel not supported");
		goto exit;
	}
	/*Get cp_tx_data_panel*/
	if (cp_tx_data_panel != NULL) {
		for (i = 0; i < tx_num; i++) {
			cp_tx_data_panel[i] =
			10*(cd->dad.ic_buf[CP_PANEL_DATA_OFFSET+i*2]
			+ 256 * cd->dad.ic_buf[CP_PANEL_DATA_OFFSET+i*2+1]);
			TS_LOG_DEBUG("cp_tx_data_panel[%d]=%d\n",
			i, cp_tx_data_panel[i]);
			cp_tx_ave_data_panel += cp_tx_data_panel[i];
		}
		cp_tx_ave_data_panel /= tx_num;
		cmcp_info->cp_tx_ave_data_panel = cp_tx_ave_data_panel;
	}

	/*Get cp_tx_cal_data_panel*/
	if (cp_tx_cal_data_panel != NULL) {
		for (i = 0; i < tx_num; i++) {
			cp_tx_cal_data_panel[i] =
			10*(cd->dad.ic_buf[CP_PANEL_DATA_OFFSET+tx_num*2+i*2]
		+ 256 * cd->dad.ic_buf[CP_PANEL_DATA_OFFSET+tx_num*2+i*2+1]);
			TS_LOG_DEBUG(" cp_tx_cal_data_panel[%d]=%d\n",
				 i, cp_tx_cal_data_panel[i]);
		}
	}

	/*get cp_sensor_tx_delta,using the first sensor cal value for temp */
	/*multiple 1000 to increase accuracy*/
	if(cp_tx_ave_data_panel != 0 && cp_tx_cal_data_panel != NULL) {
		cmcp_info->cp_sensor_tx_delta = ABS((cp_tx_cal_data_panel[0]
			- cp_tx_ave_data_panel) * 1000 / cp_tx_ave_data_panel);
	} else {
		TS_LOG_ERR("%s: cp_tx_ave_data_panel is zero, please check it\n", __func__);
	}

	/*Get cp_rx_data_panel*/
	if (cp_rx_data_panel != NULL) {
		for (i = 0; i < rx_num;  i++) {
			cp_rx_data_panel[i] =
			10*(cd->dad.ic_buf[CP_PANEL_DATA_OFFSET+tx_num*4+i*2] +
			256 * cd->dad.ic_buf[CP_PANEL_DATA_OFFSET+tx_num*4+i*2+1]);
			TS_LOG_DEBUG("cp_rx_data_panel[%d]=%d\n", i, cp_rx_data_panel[i]);
			cp_rx_ave_data_panel += cp_rx_data_panel[i];
		}
		cp_rx_ave_data_panel /= rx_num;
		cmcp_info->cp_rx_ave_data_panel = cp_rx_ave_data_panel;
	}

	/*Get cp_rx_cal_data_panel*/
	if (cp_rx_cal_data_panel != NULL) {
		for (i = 0; i < rx_num; i++) {
			cp_rx_cal_data_panel[i] =
		10 * (cd->dad.ic_buf[CP_PANEL_DATA_OFFSET+tx_num*4+rx_num*2+i*2] +
		256 *
		cd->dad.ic_buf[CP_PANEL_DATA_OFFSET+tx_num*4+rx_num*2+i*2+1]);
			TS_LOG_DEBUG("cp_rx_cal_data_panel[%d]=%d\n",
			i,
			cp_rx_cal_data_panel[i]);
		}
	}

	/*get cp_sensor_rx_delta,using the first sensor cal value for temp */
	/*multiple 1000 to increase accuracy*/
	if(cp_rx_ave_data_panel != 0 && cp_rx_cal_data_panel != NULL) {
		cmcp_info->cp_sensor_rx_delta = ABS((cp_rx_cal_data_panel[0]
			- cp_rx_ave_data_panel) * 1000 / cp_rx_ave_data_panel);
	} else {
		TS_LOG_ERR("%s: cp_rx_ave_data_panel is zero, please check it\n", __func__);
	}

exit:
	return rc;
}

//static void parade_cmcp_test(struct parade_core_data *cd)
static ssize_t parade_cmcp_test_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct parade_core_data *cd = tskit_parade_data;
	struct cmcp_data *cmcp_info = cd->dad.cmcp_info;
	struct result *result = cd->dad.result;
	struct configuration *configuration = cd->dad.configs;
	bool final_pass = true;
	int index = 0;
	int test_item = 1;
	int rc;
	u8 status;
	int self_test_id_supported = 0;

	if ((configuration == NULL) || (cmcp_info == NULL)){
		TS_LOG_ERR("%s: configuration or cmcp_info is NULL\n", __func__);
		goto exit;
	}
	TS_LOG_INFO("%s: Start Cm/Cp test!\n", __func__);

	/*force single tx*/
	rc = parade_hid_output_set_param_(cd, 0x1F, 1, 1);
	if (rc)
		TS_LOG_ERR("%s: force single tx failed\n", __func__);

	/*suspend_scanning */
	rc = parade_hid_output_suspend_scanning_(cd);
	if (rc)
		TS_LOG_ERR("%s: suspend_scanning failed\n", __func__);
	/*do calibration*/

	TS_LOG_DEBUG("%s: do calibration in single tx mode", __func__);
	rc = parade_hid_output_calibrate_idacs_(cd, 0, &status);
	if (rc < 0) {
		TS_LOG_ERR("%s: Error on calibrate idacs for mutual r=%d\n",
				__func__, rc);
	}
	rc = parade_hid_output_calibrate_idacs_(cd, 1, &status);
	if (rc < 0) {
		TS_LOG_ERR("%s: Error on calibrate idacs for buttons r=%d\n",
				__func__, rc);
	}
	rc = parade_hid_output_calibrate_idacs_(cd, 2, &status);
	if (rc < 0) {
		TS_LOG_ERR("%s: Error on calibrate idacs  for self r=%d\n",
				__func__, rc);
	}

	/*resume_scanning */
	rc = parade_hid_output_resume_scanning_(cd);
	if (rc)
		TS_LOG_ERR("%s: resume_scanning failed\n", __func__);

	if(cmcp_info == NULL) {
		TS_LOG_ERR("%s: cmcp info is NULL\n", __func__);
		index = snprintf(buf, CY_MAX_PRBUF_SIZE,
			"error\n");
		goto exit;
	}

	/*get all cmcp data from FW*/
	self_test_id_supported =
		parade_get_cmcp_info(cd, cmcp_info);
	if (self_test_id_supported)
		TS_LOG_ERR("%s: parade_get_cmcp_info failed\n", __func__);

	/*restore to multi tx*/
	rc = parade_hid_output_set_param_(cd, 0x1F, 0, 1);
	if (rc)
		TS_LOG_ERR("%s: restore multi tx failed\n", __func__);

	/*suspend_scanning */
	rc = parade_hid_output_suspend_scanning_(cd);
	if (rc)
		TS_LOG_ERR("%s: suspend_scanning failed\n", __func__);
	/*do calibration*/

	TS_LOG_DEBUG("do calibration in multi tx mode");
	rc = parade_hid_output_calibrate_idacs_(cd, 0, &status);
	if (rc < 0) {
		TS_LOG_ERR("%s: Error on calibrate idacs for mutual r=%d\n",
				__func__, rc);
	}
	rc = parade_hid_output_calibrate_idacs_(cd, 1, &status);
	if (rc < 0) {
		TS_LOG_ERR("%s: Error on calibrate idacs for buttons r=%d\n",
				__func__, rc);
	}
	rc = parade_hid_output_calibrate_idacs_(cd, 2, &status);
	if (rc < 0) {
		TS_LOG_ERR("%s: Error on calibrate idacs  for self r=%d\n",
				__func__, rc);
	}

	/*resume_scanning */
	rc = parade_hid_output_resume_scanning_(cd);
	if (rc)
		TS_LOG_ERR("%s: resume_scanning failed\n", __func__);

	TS_LOG_INFO("%s: Finish Cm/Cp test!\n", __func__);

	rc = parade_cmcp_parse_threshold_file();

	validate_cm_test_results(cd, configuration, cmcp_info,
	result, &final_pass, test_item);
	if(final_pass)
		index += snprintf(buf + index, CY_MAX_PRBUF_SIZE,"cm test pass!\n");
	else
		index += snprintf(buf + index, CY_MAX_PRBUF_SIZE,"cm test fail!\n");
	/*do cp validation*/
	validate_cp_test_results(cd, configuration, cmcp_info,
	result, &final_pass, test_item);
	if(final_pass)
		index += snprintf(buf + index, CY_MAX_PRBUF_SIZE,"cp test pass!\n");
	else
		index += snprintf(buf + index, CY_MAX_PRBUF_SIZE,"cp test fail!\n");
exit:
	return index;
}

static DEVICE_ATTR(cmcp_test, S_IRUSR, parade_cmcp_test_show, NULL);

static int parade_device_access_probe(struct parade_core_data *cd)
{
	struct parade_device_access_data *dad = &cd->dad;
	int rc = 0;
	struct configuration *configurations;
	struct cmcp_data *cmcp_info;
	struct result *result;

	int tx_num = MAX_TX_SENSORS;
	int rx_num = MAX_RX_SENSORS;
	int btn_num = MAX_BUTTONS;

	TS_LOG_INFO("%s: enter\n",__func__);

	dad->dev = cd->dev;
#ifdef TTHE_TUNER_SUPPORT
	mutex_init(&dad->debugfs_lock);
	dad->heatmap.num_element = 200;
#endif

	struct test_case_field *test_case_field_array;
	struct test_case_search *test_case_search_array;

	configurations =
		kzalloc(sizeof(*configurations), GFP_KERNEL);
	if (!configurations) {
		rc = -ENOMEM;
		goto exit;
	}
	dad->configs = configurations;

	cmcp_info = kzalloc(sizeof(*cmcp_info), GFP_KERNEL);
	if (!cmcp_info) {
		rc = -ENOMEM;
		goto creat_cmcp_info_failed;
	}
	dad->cmcp_info = cmcp_info;

	cmcp_info->tx_num = tx_num;
	cmcp_info->rx_num = rx_num;
	cmcp_info->btn_num = btn_num;

	result = kzalloc(sizeof(*result), GFP_KERNEL);
	if (!result) {
		rc = -ENOMEM;
		goto create_result_failed;
	}

	dad->result = result;

	test_case_field_array =
		kzalloc(sizeof(*test_case_field_array) * MAX_CASE_NUM,
		GFP_KERNEL);
	if (!test_case_field_array) {
		rc = -ENOMEM;
		goto test_case_field_array_failed;
	}

	test_case_search_array =
		kzalloc(sizeof(*test_case_search_array) * MAX_CASE_NUM,
		GFP_KERNEL);
	if (!test_case_search_array) {
		rc = -ENOMEM;
		goto test_case_search_array_failed;
	}

	cmcp_info->gd_sensor_col = (struct gd_sensor *)
		 kzalloc(tx_num * sizeof(struct gd_sensor), GFP_KERNEL);
	if (cmcp_info->gd_sensor_col == NULL)
		goto gd_sensor_col_failed;

	cmcp_info->gd_sensor_row = (struct gd_sensor *)
		 kzalloc(rx_num * sizeof(struct gd_sensor), GFP_KERNEL);
	if (cmcp_info->gd_sensor_row == NULL)
		goto gd_sensor_row_failed;

	cmcp_info->cm_data_panel=
		 kzalloc((tx_num * rx_num + 1) * sizeof(int32_t), GFP_KERNEL);
	if (cmcp_info->cm_data_panel == NULL)
		goto cm_data_panel_failed;

	cmcp_info->cp_tx_data_panel =
		kzalloc(tx_num * sizeof(int32_t), GFP_KERNEL);
	if (cmcp_info->cp_tx_data_panel == NULL)
		goto cp_tx_data_panel_failed;

	cmcp_info->cp_tx_cal_data_panel =
		kzalloc(tx_num * sizeof(int32_t), GFP_KERNEL);
	if (cmcp_info->cp_tx_cal_data_panel == NULL)
		goto cp_tx_cal_data_panel_failed;

	cmcp_info->cp_rx_data_panel =
		 kzalloc(rx_num * sizeof(int32_t), GFP_KERNEL);
	if (cmcp_info->cp_rx_data_panel == NULL)
		goto cp_rx_data_panel_failed;

	cmcp_info->cp_rx_cal_data_panel =
		 kzalloc(rx_num * sizeof(int32_t), GFP_KERNEL);
	if (cmcp_info->cp_rx_cal_data_panel == NULL)
		goto cp_rx_cal_data_panel_failed;

	cmcp_info->cm_btn_data = kcalloc(btn_num, sizeof(int32_t), GFP_KERNEL);
	if (cmcp_info->cm_btn_data == NULL)
		goto cm_btn_data_failed;

	cmcp_info->cp_btn_data = kcalloc(btn_num, sizeof(int32_t), GFP_KERNEL);
	if (cmcp_info->cp_btn_data == NULL)
		goto cp_btn_data_failed;

	cmcp_info->cm_sensor_column_delta =
		 kzalloc(rx_num * tx_num * sizeof(int32_t), GFP_KERNEL);
	if (cmcp_info->cm_sensor_column_delta == NULL)
		goto cm_sensor_column_delta_failed;

	cmcp_info->cm_sensor_row_delta =
		 kzalloc(tx_num * rx_num  * sizeof(int32_t), GFP_KERNEL);
	if (cmcp_info->cm_sensor_row_delta == NULL)
		goto cm_sensor_row_delta_failed;

	cmcp_info->diff_sensor_data=
		 kzalloc(tx_num * rx_num  * sizeof(int32_t), GFP_KERNEL);
	if (cmcp_info->diff_sensor_data == NULL)
		goto diff_data_failed;
	cmcp_info->diff_self_data =
		 kzalloc((tx_num + rx_num)  * sizeof(int32_t), GFP_KERNEL);
	if (cmcp_info->diff_self_data == NULL)
		goto diff_self_data_failed;

	dad->test_field_array = test_case_field_array;
	dad->test_search_array = test_case_search_array;

	rc = device_create_file(cd->dev, &dev_attr_status);
	if (rc) {
		TS_LOG_ERR("%s: Error, could not create forced_upgrade\n",
				__func__);
		goto error_create_status;
	}

	rc = device_create_file(cd->dev, &dev_attr_response);
	if (rc) {
		TS_LOG_ERR("%s: Error, could not create forced_upgrade\n",
				__func__);
		goto error_create_response;
	}

	rc = device_create_file(cd->dev, &dev_attr_command);
	if (rc) {
		TS_LOG_ERR("%s: Error, could not create forced_upgrade\n",
				__func__);
		goto error_create_command;
	}

	rc = device_create_file(cd->dev, &dev_attr_cmcp_test);
		if (rc) {
			TS_LOG_ERR("%s: Error, could not create cmcp_test\n",
					__func__);
			goto error_create_cmcp_test;
		}

#ifdef TTHE_TUNER_SUPPORT
	dad->tthe_get_panel_data_debugfs = debugfs_create_file(
			PARADE_TTHE_TUNER_GET_PANEL_DATA_FILE_NAME,
			0644, NULL, dad, &tthe_get_panel_data_fops);
	if (IS_ERR_OR_NULL(dad->tthe_get_panel_data_debugfs)) {
		TS_LOG_ERR("%s: Error, could not create get_panel_data\n",
				__func__);
		dad->tthe_get_panel_data_debugfs = NULL;
		goto error_create_get_panel_data;
	}
#endif

	return 0;
error_create_get_panel_data:
	device_remove_file(cd->dev, &dev_attr_cmcp_test);

error_create_cmcp_test:
	device_remove_file(cd->dev, &dev_attr_command);

error_create_command:
	device_remove_file(cd->dev, &dev_attr_response);

error_create_response:
	device_remove_file(cd->dev, &dev_attr_status);
error_create_status:
	kfree(cmcp_info->cm_sensor_row_delta);
diff_self_data_failed:
	kfree(cmcp_info->diff_self_data);
diff_data_failed:
	kfree(cmcp_info->diff_sensor_data);
cm_sensor_row_delta_failed:
	kfree(cmcp_info->cm_sensor_column_delta);
cm_sensor_column_delta_failed:
	kfree(cmcp_info->cp_btn_data);
cp_btn_data_failed:
	kfree(cmcp_info->cm_btn_data);
cm_btn_data_failed:
	kfree(cmcp_info->cp_rx_cal_data_panel);
cp_rx_cal_data_panel_failed:
	kfree(cmcp_info->cp_rx_data_panel);
cp_rx_data_panel_failed:
	kfree(cmcp_info->cp_tx_cal_data_panel);
cp_tx_cal_data_panel_failed:
	kfree(cmcp_info->cp_tx_data_panel);
cp_tx_data_panel_failed:
	kfree(cmcp_info->cm_data_panel);
cm_data_panel_failed:
	kfree(cmcp_info->gd_sensor_row);
gd_sensor_row_failed:
	kfree(cmcp_info->gd_sensor_col);
gd_sensor_col_failed:
	kfree(test_case_search_array);
test_case_search_array_failed:
	kfree(test_case_field_array);
test_case_field_array_failed:
	kfree(result);
create_result_failed:
	kfree(cmcp_info);
creat_cmcp_info_failed:
	kfree(configurations);
exit:
	TS_LOG_ERR("%s: probe fialed\n", __func__);
	return rc;
}

static int parade_check_cmd_status()
{
	int t = 0;
	int timeout_ms = PARADE_WORK_WAIT_TIMEOUT;
	TS_LOG_DEBUG("%s Enter\n", __func__);
	t = wait_event_timeout(tskit_parade_data->wait_q, (tskit_parade_data->isworkongoing == false), msecs_to_jiffies(timeout_ms));
		if (IS_TMO(t)) {
			TS_LOG_ERR("%s: tmo waiting access work, t = %d\n",
				   __func__, t);
			return -EBUSY;
	}
	tskit_parade_data->isworkongoing = true;
	return NO_ERR;
}

static void parade_finish_cmd()
{
	tskit_parade_data->isworkongoing = false;
	wake_up(&tskit_parade_data->wait_q);
}

static int parade_chip_get_info(struct ts_chip_info_param *info)
{
	int retval = NO_ERR;
	struct parade_core_data *cd = tskit_parade_data;
	struct parade_cydata *cydata = &cd->sysinfo.cydata;
	int t = 0;
	int timeout_ms = PARADE_WORK_WAIT_TIMEOUT;

	TS_LOG_INFO("%s Enter\n", __func__);
	retval = parade_check_cmd_status();

	if(retval)
		return retval;
	if (unlikely((atomic_read(&g_ts_kit_platform_data.state) == TS_SLEEP) || (atomic_read(&g_ts_kit_platform_data.state) == TS_WORK_IN_SLEEP))){
		TS_LOG_INFO("%s In sleep state, use cached value\n", __func__);
	} else {
		retval = parade_hid_output_get_sysinfo_(cd);
		if(retval < 0){
			info->status = TS_ACTION_FAILED;
			goto out;
		}
	}
	info->status = TS_ACTION_SUCCESS;

	sprintf(info->fw_vendor,
		"%02X.%02X.%08X.%04X",
		cydata->fw_ver_major,
		cydata->fw_ver_minor,
		cydata->revctrl,
		cydata->fw_ver_conf);
	snprintf(info->chip_name, sizeof(info->chip_name), cd->chip_name);
	snprintf(info->mod_vendor, sizeof(info->mod_vendor), cd->module_vendor);
	if (g_ts_kit_platform_data.hide_plain_id) {
		snprintf(info->ic_vendor, sizeof(info->ic_vendor), cd->project_id);
	} else {
		snprintf(info->ic_vendor, sizeof(info->ic_vendor), PARADE_VENDER_NAME);
	}

out:
	parade_finish_cmd();
	return retval;
}

static int parade_get_rawdata(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd)
{
	if(NULL == tskit_parade_data) {
		TS_LOG_ERR("%s: tskit_parade_data is NULL, err!\n", __func__);
		return -EINVAL;
	}
	if(NULL == info) {
		TS_LOG_ERR("%s: info is NULL, err!\n", __func__);
		return -EINVAL;
	}
	if(NULL == out_cmd) {
		TS_LOG_ERR("%s: out_cmd is NULL, err!\n", __func__);
		return -EINVAL;
	}

	struct parade_core_data *cd = tskit_parade_data;
	struct cmcp_data *cmcp_info = cd->dad.cmcp_info;
	struct result *result = cd->dad.result;
	struct configuration *configuration = cd->dad.configs;
	bool final_pass = true;
	int index = 0;
	int test_item = 1;
	int result_buf_index = 0;
	int rc = NO_ERR;
	u8 status = 0;
	int self_test_id_supported = 0;
	uint32_t sensor_num = 0;
	int32_t *cm_sensor_data =NULL;
	int t = 0;
	int timeout_ms = PARADE_WORK_WAIT_TIMEOUT;
	u8 reg_addr = PAR_REG_BASE;
	u16 size;
	u8 buf[2];
	TS_LOG_INFO("%s Enter\n", __func__);
	if ((configuration == NULL) || (cmcp_info == NULL)){
		TS_LOG_ERR("%s: configuration or cmcp_info is NULL\n", __func__);
		rc = -EINVAL;
		return rc;
	}
	cm_sensor_data = cmcp_info->cm_data_panel;

	info->status = TS_ACTION_SUCCESS;
	rc = parade_check_cmd_status();
	if(rc){
		info->status = TS_ACTION_FAILED;
		return rc;
	}
	if(false == g_ts_kit_platform_data.chip_data->isbootupdate_finish) {
		TS_LOG_ERR("%s MMI test is running.\n",__func__);
		return -EBUSY;
	}
	g_ts_kit_platform_data.chip_data->isbootupdate_finish = false;
	memset(buf_parademmitest_result, 0, sizeof(buf_parademmitest_result));
	memset(tp_test_failed_reason,0,sizeof(tp_test_failed_reason));
	/*i2c test*/
	rc = parade_i2c_read(&reg_addr, sizeof(reg_addr), buf, 2);
	if (rc < 0){
		result->i2c_test_pass = false;
		TS_LOG_ERR("%s: i2c read failed\n", __func__);
		strncat(buf_parademmitest_result, "0F-",strlen("0F-"));
		PutStringToReasonBuf("[0F:I2CRead]");
		goto exit;
	}
	else
		result->i2c_test_pass = true;
	/*get all diff data*/
	/*3F test */
	rc = parade_hid_output_suspend_scanning_(cd);
	if (rc){
		TS_LOG_ERR("%s: get diff parade_hid_output_suspend_scanning_ failed\n", __func__);
		strncat(buf_parademmitest_result, "3F-",strlen("1F-"));
		PutStringToReasonBuf("[3F:Suspend]");
		goto exit;
	}
	/*get diff*/
	rc = parade_get_diff_data();
	if (rc){
		TS_LOG_ERR("%s: get diff failed\n", __func__);
		strncat(buf_parademmitest_result, "1F-",strlen("1F-"));
		PutStringToReasonBuf("[3F:Noise Get]");
		goto exit;
	}

	/*resume_scanning */
	rc = parade_hid_output_resume_scanning_(cd);
	if (rc){
		TS_LOG_ERR("%s: resume_scanning failed\n", __func__);
		strncat(buf_parademmitest_result, "1F-",strlen("1F-"));
		PutStringToReasonBuf("[3F:Resume]");
		goto exit;
	}

        TS_LOG_INFO("%s: Start Cm/Cp test!\n", __func__);
	/*get all cmcp data from FW*/
	self_test_id_supported =
		parade_get_cmcp_info(cd, cmcp_info);
	if (self_test_id_supported)
	{
		TS_LOG_ERR("%s: Error on parade_gtp_init_work_fnet_cmcp_info failed r=%d\n",
				__func__, self_test_id_supported);
		strncat(buf_parademmitest_result, "1F-2F-3F-4F-5F",strlen("1F-2F-3F-4F-5F"));
		PutStringToReasonBuf("[Get Cm]");
		rc = -EIO;
		goto exit;
	}

	rc = parade_hid_output_suspend_scanning_(cd);
	if (rc){
		TS_LOG_ERR("%s: init baseline parade_hid_output_suspend_scanning_ failed\n", __func__);
		strncat(buf_parademmitest_result, "3F-",strlen("1F-"));
		PutStringToReasonBuf("[baseline-Suspend]");
		goto exit;
	}

	/*Reinitialize baseline, as Cm/Cp will corrupt the baseline*/
	rc = parade_hid_output_initialize_baselines_(CY_IB_SM_MUTCAP | CY_IB_SM_SELFCAP,&status);
	if (rc){
		TS_LOG_ERR("%s: re init baseline fail\n", __func__);
		strncat(buf_parademmitest_result, "1F-2F-3F-4F-5F",strlen("1F-2F-3F-4F-5F"));
		PutStringToReasonBuf("[reinit baseline]");
		goto exit;
	}

	/*resume_scanning */
	rc = parade_hid_output_resume_scanning_(cd);
	if (rc){
		TS_LOG_ERR("%s: resume_scanning failed\n", __func__);
		strncat(buf_parademmitest_result, "1F-2F-3F-4F-5F",strlen("1F-2F-3F-4F-5F"));
		PutStringToReasonBuf("[baseline:Resume]");
		goto exit;
	}

	if(tskit_parade_data->calibrate_fw_update_and_cmcp) {
		rc = parade_calibrate_child();
		if(!rc) {
			TS_LOG_INFO("%s calibrate end\n", __func__);
		}
	}

	TS_LOG_INFO("%s: Finish Cm/Cp test!\n", __func__);

	rc = parade_cmcp_parse_threshold_file();
	if (rc){
		TS_LOG_ERR("%s: read threshold file\n", __func__);
		strncat(buf_parademmitest_result, "1F-",strlen("1F-"));
		PutStringToReasonBuf("[Read Threshold File]");
		goto exit;
	}
	validate_cm_test_results(cd, configuration, cmcp_info,result, &final_pass, test_item);
	validate_cp_test_results(cd, configuration, cmcp_info,result, &final_pass, test_item);
	validate_diff_result(configuration->diff_noise_threshold);//Hardcode here
	if (CY_SELF_NOISE_SUP == cd->self_noise_supported) {
		validate_diff_self_result(configuration->diff_self_rx_noise_threshold, configuration->diff_self_tx_noise_threshold);
	}

	result->change_report_rate_pass = CY_REPORT_RATE_FAIL;
	if(CY_REPORT_RATE_SUP == cd->report_rate_check_supported) {
		parade_change_report_rate();
	}

	if (result->i2c_test_pass){
		strncat(buf_parademmitest_result, "0P-",strlen("0P-"));
	}
	else{
		strncat(buf_parademmitest_result, "0F-",strlen("0F-"));
		AddReasonToBuf("-0F:I2C]");
	}
	if (result->cm_sensor_validation_pass){
		strncat(buf_parademmitest_result, "1P-",strlen("1P-"));
	}
	else{
		strncat(buf_parademmitest_result, "1F-",strlen("1F-"));
		AddReasonToBuf("-1F:CmRange]");
	}
	if(result->cm_sensor_gd_col_pass && result->cm_sensor_gd_row_pass
		&& result->cm_sensor_row_delta_pass && result->cm_sensor_col_delta_pass){
		strncat(buf_parademmitest_result, "2P-",strlen("2P-"));
	}
	else{
		strncat(buf_parademmitest_result, "2F-",strlen("2F-"));
		AddReasonToBuf("-2F:CmGrad");
	}
	if(result->diff_test_pass){
		strncat(buf_parademmitest_result, "3P-",strlen("3P-"));
	}
	else{
		strncat(buf_parademmitest_result, "3F-",strlen("3F-"));
		AddReasonToBuf("-3F:Noise");
	}
	if(CY_REPORT_RATE_SUP == cd->report_rate_check_supported) {
		if(result->change_report_rate_pass){
			strncat(buf_parademmitest_result, CY_REPORT_RATE_PASS, strlen(CY_REPORT_RATE_PASS));
		}
	}
	else{
		strncat(buf_parademmitest_result, CY_REPORT_RATE_PASS, strlen(CY_REPORT_RATE_PASS));
	}
	if(result->short_test_pass){
		strncat(buf_parademmitest_result, "5P-",strlen("5P-"));
	}
	else{
		strncat(buf_parademmitest_result, "5F-",strlen("5F-"));
		AddReasonToBuf("-5F:Short");
	}
	if(result->cp_rx_validation_pass
			&& result->cp_tx_validation_pass){
		strncat(buf_parademmitest_result, "6P-",strlen("6P-"));
	}
	else{
		strncat(buf_parademmitest_result, "6F-",strlen("6F-"));
		AddReasonToBuf("-6F:CpRange");
	}
	if(result->cp_sensor_delta_pass){
		strncat(buf_parademmitest_result, "7P-",strlen("7P-"));
	}
	else{
		strncat(buf_parademmitest_result, "7F-",strlen("7F-"));
		AddReasonToBuf("-7F:CpDelta");
	}
	if (CY_SELF_NOISE_SUP == cd->self_noise_supported) {
		if(result->diff_self_rx_test_pass && result->diff_self_tx_test_pass){
			strncat(buf_parademmitest_result, CY_SELF_NOISE_PASS, strlen(CY_SELF_NOISE_PASS));
		} else {
			strncat(buf_parademmitest_result, CY_SELF_NOISE_FAIL, strlen(CY_SELF_NOISE_FAIL));
			AddReasonToBuf("-9F:Self RX Noise");
		}
	}
	/*copy x y electrodes out for show*/
	info->buff[1] = cmcp_info->tx_num;
	info->buff[0] = cmcp_info->rx_num;
	cm_sensor_data = cmcp_info->cm_data_panel;
	sensor_num = cmcp_info->tx_num*cmcp_info->rx_num;
	result_buf_index = 2;
	/*Cm*/
	for(index=0;index < sensor_num;index++)
	{
		info->buff[result_buf_index++] = cm_sensor_data[index];
	}
	/*mc diff*/
	for(index=0;index < sensor_num;index++)
	{
		info->buff[result_buf_index++] = cmcp_info->diff_sensor_data[index];
	}
	/*sc diff*/
	for(index=0;index < (cmcp_info->rx_num + cmcp_info->tx_num);index++)
	{
		info->buff[result_buf_index++] = cmcp_info->diff_self_data[index];
	}
	/*Cm Gradient RX*/
	for(index=0;index < cmcp_info->rx_num;index++)
	{
		info->buff[result_buf_index++] = cmcp_info->gd_sensor_row[index].gradient_val;
	}
	/*Cm Gradient TX*/
	for(index=0;index < cmcp_info->tx_num;index++)
	{
		info->buff[result_buf_index++] = cmcp_info->gd_sensor_col[index].gradient_val;
	}
	/*Cp RX*/
	for(index=0;index < cmcp_info->rx_num;index++)
	{
		info->buff[result_buf_index++] = cmcp_info->cp_rx_data_panel[index];
	}
	/*Cp TX*/
	for(index=0;index < cmcp_info->tx_num;index++)
	{
		info->buff[result_buf_index++] = cmcp_info->cp_tx_data_panel[index];
	}
	/*Cp Delta RX*/
	for(index=0;index < cmcp_info->rx_num;index++)
	{
		info->buff[result_buf_index++] = ABS((cmcp_info->cp_rx_cal_data_panel[index] -
			cmcp_info->cp_rx_data_panel[index]) * 100 /
			cmcp_info->cp_rx_data_panel[index]);
	}
	/*Cp Delta TX*/
	for(index=0;index < cmcp_info->tx_num;index++)
	{
		info->buff[result_buf_index++] = ABS((cmcp_info->cp_tx_cal_data_panel[index] -
			cmcp_info->cp_tx_data_panel[index]) * 100 /
			cmcp_info->cp_tx_data_panel[index]);
	}
	info->used_size = result_buf_index;
	TS_LOG_INFO("%s finish, used buf = %d\n",__func__,info->used_size);

exit:
	if(strlen(buf_parademmitest_result) != 0)
	{
		if(strlen(tp_test_failed_reason) != 0){
			strncat(buf_parademmitest_result, tp_test_failed_reason,strlen(tp_test_failed_reason));
		}
		AddtoMMIResult(";");
		AddtoMMIResult(PARADE_VENDER_NAME);
		AddtoMMIResult("-");
		AddtoMMIResult(tskit_parade_data->chip_name);
		AddtoMMIResult("-");
		AddtoMMIResult(tskit_parade_data->project_id);
		memcpy(info->result, buf_parademmitest_result, strlen(buf_parademmitest_result));
	}
	if(rc!=NO_ERR)
		info->status = TS_ACTION_FAILED;

	rc = parade_hid_output_resume_scanning_(tskit_parade_data);
	if (rc){
		TS_LOG_ERR("%s: resume_scanning failed\n", __func__);
	}
	g_ts_kit_platform_data.chip_data->isbootupdate_finish = true;
	parade_finish_cmd();
	return rc;

}

static int parade_regulator_get(void)
{
	if(tskit_parade_data->self_ctrl_power == 1){
		TS_LOG_INFO("%s, power control by touch ic\n",__func__);
		if (1 == tskit_parade_data->parade_chip_data->vci_regulator_type) {
			tskit_parade_data->vdda = regulator_get(&tskit_parade_data->parade_dev->dev,"parade_vci");
			if (IS_ERR(tskit_parade_data->vdda)) {
				TS_LOG_ERR("%s, regulator tp vdda not used\n",__func__);
				return -EINVAL;
			}
		}

		if (1 == tskit_parade_data->parade_chip_data->vddio_regulator_type) {
			tskit_parade_data->vddd = regulator_get(&tskit_parade_data->parade_dev->dev,"parade_vddio");
			if (IS_ERR(tskit_parade_data->vddd)) {
				TS_LOG_ERR("%s, regulator tp vddd not used\n",__func__);
				goto ts_vddio_out;
			}
		}
	}
	else{
		TS_LOG_INFO("%s, power control by LCD, nothing to do\n",__func__);
	}
	return 0;

ts_vddio_out:
	if (1 == tskit_parade_data->parade_chip_data->vci_regulator_type)
		regulator_put(tskit_parade_data->vdda);
	return -EINVAL;
}

static void parade_regulator_put(void)
{
	if (1 == tskit_parade_data->parade_chip_data->vci_regulator_type) {
		if (!IS_ERR(tskit_parade_data->vdda)) {
			regulator_put(tskit_parade_data->vdda);
		}
	}
	if (1 == tskit_parade_data->parade_chip_data->vddio_regulator_type) {
		if (!IS_ERR(tskit_parade_data->vddd)) {
			regulator_put(tskit_parade_data->vddd);
		}
	}
}


static int parade_set_voltage(void)
{
	int rc = 0;
	if(tskit_parade_data->self_ctrl_power == 1){
		TS_LOG_INFO("%s, power control by touch ic\n",__func__);
		if(1 == tskit_parade_data->parade_chip_data->vci_regulator_type){
			/*set voltage for vddd and vdda*/
			rc = regulator_set_voltage(tskit_parade_data->vdda, tskit_parade_data->parade_chip_data->regulator_ctr.vci_value, tskit_parade_data->parade_chip_data->regulator_ctr.vci_value);
			if(rc < 0){
				TS_LOG_ERR("%s, failed to set parade vdda to %dV, rc = %d\n", __func__, rc,tskit_parade_data->parade_chip_data->regulator_ctr.vci_value);
				return -1;
			}
		}

		if (1 == tskit_parade_data->parade_chip_data->vddio_regulator_type){ 
			rc = regulator_set_voltage(tskit_parade_data->vddd, tskit_parade_data->parade_chip_data->regulator_ctr.vddio_value, tskit_parade_data->parade_chip_data->regulator_ctr.vddio_value);
			if(rc < 0){
				TS_LOG_ERR("%s, failed to set parade vddd to %dV, rc = %d\n", __func__, rc,tskit_parade_data->parade_chip_data->regulator_ctr.vddio_value);
				return -1;
			}
		}
	}
	else{
		TS_LOG_INFO("%s, power control by LCD, nothing to do\n",__func__);
	}

	return 0;
}

static int parade_gpio_request(void)
{
	int retval = NO_ERR;
	TS_LOG_INFO("%s enter\n", __func__);

	if (0 == tskit_parade_data->parade_chip_data->vci_regulator_type) {
		retval = gpio_request(tskit_parade_data->parade_chip_data->vci_gpio_ctrl, "parade_vci_gpio_ctrl");
		if (retval) {
			TS_LOG_ERR
			    ("SFT:Ok;  ASIC: Real ERR----unable to request vci_gpio_ctrl2:%d\n",
			     tskit_parade_data->parade_chip_data->vci_gpio_ctrl);
			return -1;
		}
	}
	if (0 ==  tskit_parade_data->parade_chip_data->vddio_regulator_type) {
		retval = gpio_request(tskit_parade_data->parade_chip_data->vddio_gpio_ctrl, "parade_vddio_gpio_ctrl");
		if (retval) {
			TS_LOG_ERR
			    ("SFT:Ok;  ASIC: Real ERR----unable to request vddio_gpio_ctrl:%d\n",
			     tskit_parade_data->parade_chip_data->vddio_gpio_ctrl);
			goto ts_vddio_out;
		}
	}
	return 0;
	
ts_vddio_out:
	if (0 == tskit_parade_data->parade_chip_data->vci_regulator_type) 
		gpio_free(tskit_parade_data->parade_chip_data->vci_gpio_ctrl);	
	return -1;
}

static void parade_gpio_free(void)
{
	TS_LOG_INFO("%s enter\n", __func__);
	/*0 is power supplied by gpio, 1 is power supplied by ldo */
	if (0 == tskit_parade_data->parade_chip_data->vci_regulator_type) {
		if(0 <= tskit_parade_data->parade_chip_data->vci_gpio_ctrl)
		gpio_free(tskit_parade_data->parade_chip_data->vci_gpio_ctrl);
	}
	if (0 == tskit_parade_data->parade_chip_data->vddio_regulator_type) {
		if (0 <= tskit_parade_data->parade_chip_data->vddio_gpio_ctrl)
		gpio_free(tskit_parade_data->parade_chip_data->vddio_gpio_ctrl);
	}
}


static int parade_power_on(void)
{
	int rc = 0;
	if(tskit_parade_data->self_ctrl_power == 1){
		TS_LOG_INFO("%s, power control by touch ic\n",__func__);
		/*set voltage for vddd and vdda*/
		if (1 == tskit_parade_data->parade_chip_data->vci_regulator_type) {
			if (!IS_ERR(tskit_parade_data->vdda)) {
				TS_LOG_INFO("vdda enable is called\n");
				rc = regulator_enable(tskit_parade_data->vdda);
				if (rc < 0) {
					TS_LOG_ERR("%s, failed to enable parade vdda, rc = %d\n", __func__, rc);
					return -1;
				}
			}
		}
		else if(0 == tskit_parade_data->parade_chip_data->vci_regulator_type) {
			TS_LOG_INFO("%s vdda switch gpio on\n", __func__);
			gpio_direction_output(tskit_parade_data->parade_chip_data->vci_gpio_ctrl, 1);//TODO: need related with dts
		}

		if (1 == tskit_parade_data->parade_chip_data->vddio_regulator_type) {
			if (!IS_ERR(tskit_parade_data->vddd)) {
				TS_LOG_INFO("vdda enable is called\n");
				rc = regulator_enable(tskit_parade_data->vddd);
				if (rc < 0) {
					TS_LOG_ERR("%s, failed to enable parade vddd, rc = %d\n", __func__, rc);
					goto ts_vddd_out;
				}
			}
		}
		if(0 == tskit_parade_data->parade_chip_data->vddio_gpio_type) {
			TS_LOG_INFO("%s vddd switch gpio on\n", __func__);
			gpio_direction_output(tskit_parade_data->parade_chip_data->vddio_gpio_ctrl, 1);//TODO: need related with dts
		}
	}else{
		TS_LOG_INFO("%s, power control by LCD, nothing to do\n",__func__);
	}
	if(true== tskit_parade_data->need_set_rst_after_iovcc_flag) {
        	mdelay(1);	/*make sure iovcc power on success*/
        	TS_LOG_INFO("%s, set rst & int after iovcc sleep 1ms\n", __func__);
        }
	parade_power_on_gpio_set();
	return 0;

ts_vddd_out:
	if (1 == tskit_parade_data->parade_chip_data->vci_regulator_type) {
		rc = regulator_disable(tskit_parade_data->vdda);
		if (rc < 0) {
			TS_LOG_ERR("%s, failed to disable parade vdda, rc = %d\n", __func__, rc);
		}
	}
	return -1;
}

static int parade_power_off(void)
{
	int rc = 0;
	TS_LOG_INFO("%s enter\n", __func__);
	parade_power_off_gpio_set();

	if(tskit_parade_data->self_ctrl_power == 1){
		TS_LOG_INFO("%s, power control by touch ic\n",__func__);
		/*set voltage for vddd and vdda*/
		if (1 == tskit_parade_data->parade_chip_data->vci_regulator_type) {
			if (!IS_ERR(tskit_parade_data->vdda)) {
				TS_LOG_INFO("vdda disable is called\n");
				rc = regulator_disable(tskit_parade_data->vdda);
				if (rc < 0) {
					TS_LOG_ERR("%s, failed to disable parade vdda, rc = %d\n", __func__, rc);
					return -1;
				}
			}
		}
		else if(0 == tskit_parade_data->parade_chip_data->vci_regulator_type) {
			TS_LOG_INFO("%s vdda switch gpio off\n", __func__);
			gpio_direction_output(tskit_parade_data->parade_chip_data->vci_gpio_ctrl, 0);//TODO: need related with dts
		}

		if (1 == tskit_parade_data->parade_chip_data->vddio_regulator_type) {
			if (!IS_ERR(tskit_parade_data->vddd)) {
				TS_LOG_INFO("vdda disable is called\n");
				rc = regulator_disable(tskit_parade_data->vddd);
				if (rc < 0) {
					TS_LOG_ERR("%s, failed to disable parade vddd, rc = %d\n", __func__, rc);
					return -1;
				}
			}
		}
		if(0 == tskit_parade_data->parade_chip_data->vddio_gpio_type) {
			TS_LOG_INFO("%s vddd switch gpio off\n", __func__);
			gpio_direction_output(tskit_parade_data->parade_chip_data->vddio_gpio_ctrl, 0);//TODO: need related with dts
		}

		if((1 == tskit_parade_data->parade_chip_data->vci_regulator_type) || (1 == tskit_parade_data->parade_chip_data->vddio_regulator_type)){
			if(tskit_parade_data->need_delay_after_power_off){
				msleep(20);//need delay 20ms for ldo power off completely
				TS_LOG_INFO("%s need_delay_after_power_off\n", __func__);
			}
		}
	}
	else{
		TS_LOG_INFO("%s, power control by LCD, nothing to do\n",__func__);
	}
	return 0;
}

static void parade_power_off_gpio_set(void)
{
	int rc = 0;

	TS_LOG_INFO("%s: suspend RST\n", __func__);
	rc = gpio_direction_output(tskit_parade_data->parade_chip_data->ts_platform_data->reset_gpio, 0);
	if (rc) {
		TS_LOG_ERR("%s: gpio_direction_output for reset gpio failed\n", __func__);
	}

	rc = gpio_direction_input(tskit_parade_data->parade_chip_data->ts_platform_data->reset_gpio);
	if (rc) {
		TS_LOG_ERR("%s: gpio_direction_input for reset gpio failed\n", __func__);
	}

	mdelay(1);

}

static void parade_power_on_gpio_set(void)
{
	int rc = 0;

	TS_LOG_INFO("%s: resume gpio\n", __func__);
	rc = gpio_direction_input(tskit_parade_data->parade_chip_data->ts_platform_data->irq_gpio);
	if (rc) {
		TS_LOG_ERR("%s: gpio_direction_input for irq gpio failed\n", __func__);
	}

	rc = gpio_direction_output(tskit_parade_data->parade_chip_data->ts_platform_data->reset_gpio, 1);
	if (rc) {
		TS_LOG_ERR("%s: gpio_direction_output for reset gpio failed\n", __func__);
	}
}

static int parade_file_open_firmware(u8 *file_path, u8 *databuf,
				      int *file_size)
{
    struct file *filp = NULL;
    struct inode *inode = NULL;
    unsigned int file_len = 0;
    mm_segment_t oldfs;
    int retval = 0;

	if (file_path == NULL || databuf == NULL) {
		TS_LOG_ERR("%s: path || buf is NULL.\n", __func__);
		return -EINVAL;
	}

	TS_LOG_INFO("%s: path = %s.\n", __func__, file_path);

	/*open file*/
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	filp = filp_open(file_path, O_RDONLY, S_IRUSR);
	if (IS_ERR(filp)) {
		TS_LOG_ERR("%s: open %s error.\n", __func__, file_path);
		retval = -EIO;
		goto err;
	}

	if (filp->f_op == NULL) {
		TS_LOG_ERR("%s: File Operation Method Error\n", __func__);
		retval = -EINVAL;
		goto exit;
	}

	inode = filp->f_path.dentry->d_inode;
	if (inode == NULL) {
		TS_LOG_ERR("%s: Get inode from filp failed\n", __func__);
		retval = -EINVAL;
		goto exit;
	}
	/*Get file size*/
	file_len = i_size_read(inode->i_mapping->host);
	if (file_len == 0) {
		TS_LOG_ERR("%s: file size error,file_len = %d\n", __func__,
			   file_len);
		retval = -EINVAL;
		goto exit;
	}
	/*read image data to kernel */
	if (file_len > PARADE_FIRMWARE_SIZE) {
		TS_LOG_ERR("%s: file size ( %d ) exception.\n",
			__func__, file_len);
		retval = -EINVAL;
		goto exit;
	} else if (vfs_read(filp, databuf, file_len,
			&filp->f_pos) != file_len) {
		TS_LOG_ERR("%s: file read error.\n", __func__);
		retval = -EINVAL;
		goto exit;
	}

	*file_size = file_len;

exit:
	filp_close(filp, NULL);
err:
	set_fs(oldfs);
	return retval;
}

static int _parade_fw_update_from_sdcard(u8 *file_path,
					 bool update_flag)
{
    int retval;
    u8 *fw_data = NULL;
    int header_size = 0;
    int fw_size = 0;
    u32 fw_ver_new = 0;
    u32 fw_revctrl_new = 0;
	struct device *dev = tskit_parade_data->dev;
	struct parade_core_data* cd = tskit_parade_data;

	TS_LOG_INFO("%s: Reading hid descriptor\n", __func__);
	retval = parade_get_hid_descriptor_(cd, &cd->hid_desc);
	if (retval < 0) {
		TS_LOG_INFO("%s: Error on getting HID descriptor r=%d\n",
			__func__, retval);
		goto exit;
	}
	/*Start to get mode*/
	TS_LOG_INFO("%s: start to get mode.\n", __func__);
	cd->mode = parade_get_mode(cd, &cd->hid_desc); //re-get mode

	TS_LOG_INFO("%s: file_name is %s.\n", __func__, file_path);

	fw_data = kzalloc(PARADE_FIRMWARE_SIZE, GFP_KERNEL);
	if (fw_data == NULL) {
		TS_LOG_ERR("%s: kzalloc error.\n", __func__);
		return -EINVAL;
	}

	retval = parade_file_open_firmware(file_path, fw_data, &fw_size);
	if (retval != 0) {
		TS_LOG_ERR("file_open_firmware error, code = %d\n", retval);
		goto exit;
	}

	header_size = fw_data[0];

	TS_LOG_INFO("%s: header_size = %d,fw_size = %d.\n", __func__,
		    header_size, fw_size);

	if (header_size >= (fw_size + 1)) {
		TS_LOG_ERR("%s: Firmware format is invalid,header_size = %d,fw_size = %d.\n",
		     __func__, header_size, fw_size);
		goto exit;
	}

	fw_ver_new = get_unaligned_be16(&fw_data[3]);
	fw_revctrl_new = get_unaligned_be32(&fw_data[9]);
	TS_LOG_INFO("%s force update fw form sdcard.\n",
			__func__);
	retval = cyttsp5_upgrade_firmware(dev,
			(const u8 *)&fw_data[header_size + 1],
			fw_size - (header_size + 1));

	TS_LOG_INFO("%s is done\n", __func__);
exit:
	kfree(fw_data);
	return retval;
}

static int parade_fw_update_sd(void)
{
	int rc  = NO_ERR;
	int t = 0;
	int timeout_ms = PARADE_WORK_WAIT_TIMEOUT;

	rc = parade_check_cmd_status();
	if(rc)
		return rc;

	if(false == g_ts_kit_platform_data.chip_data->isbootupdate_finish) {
		TS_LOG_ERR("%s MMI test is running.\n",__func__);
		return -EBUSY;
	}
	g_ts_kit_platform_data.chip_data->isbootupdate_finish = false;

	rc = _parade_fw_update_from_sdcard(PARADE_FW_FROM_SD_NAME,1);
	if (rc < 0) {
		TS_LOG_ERR("%s: fw update from sdcard failed.\n", __func__);
	}
	else{
		TS_LOG_INFO("%s: fw update from sdcard is successful.\n", __func__);
		rc = parade_startup(tskit_parade_data,false);
		if(rc)
			TS_LOG_ERR("%s: Startup Error after FW upgrade\n",
					__func__);
		else {
			TS_LOG_INFO("%s: Start up successfully after FW upgrade\n",
					__func__);

			if(tskit_parade_data->calibrate_fw_update_and_cmcp) {
				rc = parade_calibrate_child();
				if(!rc) {
					TS_LOG_INFO("%s calibrate end\n", __func__);
				}
			}
		}
	}
	if(tskit_parade_data->create_project_id_flag == 1){
		rc = parade_create_project_id();
		if (rc < 0) {
			TS_LOG_ERR("%s: create project id failed!\n", __func__);
		}
	}
out:
	g_ts_kit_platform_data.chip_data->isbootupdate_finish = true;
	parade_finish_cmd();
	return rc;
}


static int parade_hid_output_initialize_baselines_(u8 test_id,u8 *status)
{
	int rc = 0;
	int write_length = 1;
	u8 write_buf[1];
	u8 cmd_offset = 0;
	struct parade_core_data* cd = tskit_parade_data;
	struct parade_hid_output hid_output = {
		HID_OUTPUT_APP_COMMAND(HID_OUTPUT_INITIALIZE_BASELINES),
		.write_length = write_length,
		.write_buf = write_buf,
	};
	TS_LOG_INFO("%s, Enter", __func__);
	write_buf[cmd_offset++] = test_id;

	rc = parade_hid_send_output_and_wait_(cd, &hid_output);
	if (rc)
		return rc;

	*status = cd->response_buf[5];
	if (*status)
		return -EINVAL;
	return rc;
}

void ts_kit_check_bootup_upgrade(void)
{
	int max_timeout = 10000;//10s
	int t = 0;
	int istatus = 0;
	if((tskit_parade_data == NULL) ||
		(g_ts_kit_platform_data.chip_data == NULL)){
		TS_LOG_INFO("%s not parade solution, just return\n",__func__);
		return;
	}
	istatus = g_ts_kit_platform_data.chip_data->isbootupdate_finish;
	TS_LOG_INFO("%s enter, with max timeout: %d s, current bootupgrade status: %d\n", __func__,max_timeout/1000,istatus);
	t = wait_event_timeout(tskit_parade_data->wait_q_bootup, (g_ts_kit_platform_data.chip_data->isbootupdate_finish == true), msecs_to_jiffies(max_timeout));
		if (IS_TMO(t)) {
			TS_LOG_ERR("%s: tmo waiting access work, t = %d\n",
				   __func__, t);
			return;
	}
	TS_LOG_INFO("%s wait finish, timeout value %d\n",__func__,t);
}
EXPORT_SYMBOL(ts_kit_check_bootup_upgrade);
/*    init the chip.
 *
 *	   (1) power up;  (2) reset gpio config;  (3) detect the chip thourgh bus(i2c).
 */
static int parade_chip_detect(struct ts_kit_platform_data *platform_data)
{
	struct parade_core_data *cd = tskit_parade_data;
	int rc = NO_ERR;
	TS_LOG_INFO("%s, parade chip detect called\n", __func__);

	tskit_parade_data->parade_dev = platform_data->ts_dev;
	tskit_parade_data->parade_dev->dev.of_node = tskit_parade_data->parade_chip_data->cnode ;
	tskit_parade_data->parade_chip_data->ts_platform_data = platform_data;
	tskit_parade_data->parade_chip_data->is_in_cell = true;
	tskit_parade_data->process_state = CY_STATE_INIT;
	tskit_parade_data->parade_chip_data->is_i2c_one_byte = 0;
	tskit_parade_data->parade_chip_data->is_new_oem_structure= 0;
	tskit_parade_data->dev = &(platform_data->client->dev);
	
	g_ts_kit_platform_data.chip_data->isbootupdate_finish = true;

	atomic_set(&parade_reset_pin_status,0);
	parade_is_power_on = true;

	rc = parade_parse_dts(tskit_parade_data->parade_dev->dev.of_node,tskit_parade_data->parade_chip_data);
	if(0 != rc)
	{
		TS_LOG_ERR("%s, error parade_parse_dts\n", __func__);
		goto parse_dts_failed;
	}

	if(true == tskit_parade_data->parade_chip_data->check_bulcked){
		if(false ==isbulcked){
		    TS_LOG_ERR("%s, no lcd buckled \n", __func__);
		    rc = -EFAULT;
			goto bulcked_err;
		}
	}

	if(cd->core_pdata->wakeup_gesture_enable){
		cd->double_tab_info = (struct parade_double_tap_info*)kzalloc(sizeof(struct parade_double_tap_info), GFP_KERNEL);
		if(!cd->double_tab_info){
			TS_LOG_ERR("%s : Failed to kzalloc for parade_double_tap_info.\n", __func__);
			rc = -ENOMEM;
			goto alloc_failed;
		}

		cd->parade_chip_data->easy_wakeup_info.off_motion_on = false;
	}
	/*in g_ts_device_map, parade's chip_detect is at the back of synpatics, so just add msleep 50ms here*/
	msleep(50);
	printk(KERN_ALERT"parade chip detect called1\n");
	rc = parade_regulator_get();
	if(0 != rc)
	{
		TS_LOG_ERR("%s, error request power vdda vddd\n", __func__);
		goto regulator_err;
	}

	rc = parade_gpio_request();
	if(0 != rc)
	{
		TS_LOG_ERR("%s, error request gpio for vci and vddio\n", __func__);
		goto gpio_err;
	}
	if(tskit_parade_data->pinctrl_set == 1) {
		rc = parade_pinctrl_select_normal();
		if(rc < 0){
			TS_LOG_ERR("%s, parade_pinctrl_select_normal failed\n", __func__);
			goto pinctrl_get_err;
		}
	}
	/*irq and rst direciton set*/
	if(false == tskit_parade_data->need_set_rst_after_iovcc_flag) {
		rc = gpio_direction_output(tskit_parade_data->parade_chip_data->ts_platform_data->reset_gpio, 1);
		if (rc) {
			TS_LOG_ERR("%s: gpio_direction_output for reset gpio failed\n", __func__);
		}
		rc = gpio_direction_input(tskit_parade_data->parade_chip_data->ts_platform_data->irq_gpio);
		if (rc) {
			TS_LOG_ERR("%s: gpio_direction_input for reset gpio failed\n", __func__);
		}
	}else {
		rc = gpio_direction_input(tskit_parade_data->parade_chip_data->ts_platform_data->reset_gpio);
		if (rc) {
			TS_LOG_ERR("%s: gpio_direction_input for reset gpio failed\n", __func__);
		}
		TS_LOG_INFO("%s, set rst & int after iovcc, set reset input when power on\n", __func__);
	}

	rc = parade_set_voltage();
	if (rc < 0) {
		TS_LOG_ERR("%s, failed to set voltage, rc = %d\n", __func__, rc);
		goto pinctrl_select_error;
	}
	rc = parade_power_on();
	if (rc < 0) {
		TS_LOG_ERR("%s, failed to enable power, rc = %d\n", __func__, rc);
		goto pinctrl_select_error;
	}
	msleep(5);
	parade_hw_reset();

	TS_LOG_INFO("%s, one byte flag is %d\n", __func__,tskit_parade_data->parade_chip_data->is_i2c_one_byte);
	rc = i2c_communicate_check();
	if (rc < 0) {
		TS_LOG_ERR("%s, not find parade device\n", __func__);
		goto check_err;
	} else {
		strncpy(tskit_parade_data->parade_chip_data->chip_name,PARADE_VENDER_NAME,strlen(PARADE_VENDER_NAME)+1);
		TS_LOG_INFO("%s, find parade device\n", __func__);
	}
	/*Creat File*/
	TS_LOG_INFO("%s: creat device file\n", __func__);

#ifdef TTHE_TUNER_SUPPORT
	mutex_init(&tskit_parade_data->tthe_lock);
	tskit_parade_data->tthe_debugfs = debugfs_create_file(PARADE_TTHE_TUNER_FILE_NAME,
		0644, NULL, tskit_parade_data, &tthe_debugfs_fops);
#endif

	rc = device_create_file(cd->dev, &dev_attr_hw_reset);
	if (rc) {
		TS_LOG_ERR("%s: Error, could not create hw_reset sysfs\n",
				__func__);
		goto check_err;
	}
	rc = device_create_file(cd->dev, &dev_attr_drv_debug);
	if (rc) {
		TS_LOG_ERR("%s: Error, could not create hw_reset sysfs\n",
				__func__);
		goto check_err;
	}

	rc = device_create_file(cd->dev, &dev_attr_ic_ver);
	if (rc) {
		TS_LOG_ERR("%s: Error, could not create ic_ver sysfs\n",
				__func__);
		goto check_err;
	}
	rc = device_create_file(cd->dev, &dev_attr_doze);
	if (rc) {
		TS_LOG_ERR("%s: Error, could not create dozea sysfs\n",
				__func__);
		goto check_err;
	}
	rc = device_create_file(cd->dev, &dev_attr_drv_ver);
	if (rc) {
		TS_LOG_ERR("%s: Error, could not create drv_ver sysfs\n",
				__func__);
		goto check_err;
	}
	//Set chip to reset state during fully init
	TS_LOG_INFO("%s: put reset to low\n", __func__);
	parade_reset_pin_low();
	msleep(5); //Solution: as following IRQ will be register.  if not sleep, it will has false trigger

	TS_LOG_INFO("%s: done\n", __func__);
	return NO_ERR;

check_err:
	parade_power_off();
pinctrl_select_error:
	if(tskit_parade_data->pinctrl_set == 1)
		parade_pinctrl_select_lowpower();
pinctrl_get_err:
	parade_gpio_free();
gpio_err:
	parade_regulator_put();
regulator_err:
	if(cd->double_tab_info){
		kfree(cd->double_tab_info);
		cd->double_tab_info = NULL;
	}
alloc_failed:
bulcked_err:
parse_dts_failed:
	parade_pinctrl_release();
	if(tskit_parade_data->parade_chip_data) {
		kfree(tskit_parade_data->parade_chip_data);
		tskit_parade_data->parade_chip_data = NULL;
	}

	if (tskit_parade_data) {
		kfree(tskit_parade_data);
		tskit_parade_data = NULL;
	}
	TS_LOG_ERR("parade_chip_detect fail!\n");
	return rc;
}

static int __init parade_module_init(void)
{
    bool found = false;
    struct device_node* child = NULL;
    struct device_node* root = NULL;
    int error = NO_ERR;

    TS_LOG_INFO(" parade_module_init called here\n");
    tskit_parade_data = kzalloc(sizeof(struct parade_core_data), GFP_KERNEL);
    if (!tskit_parade_data) {
    	TS_LOG_ERR("Failed to alloc mem for struct tskit_parade_data\n");
       error =  -ENOMEM;
       return error;
    }
    tskit_parade_data->parade_chip_data = kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
    if (!tskit_parade_data->parade_chip_data ) {
    	TS_LOG_ERR("Failed to alloc mem for struct synaptics_chip_data\n");
       error =  -ENOMEM;
       goto out;
    }
    root = of_find_compatible_node(NULL, NULL, "huawei,ts_kit");
    if (!root)
    {
	TS_LOG_ERR("huawei_ts, find_compatible_node huawei,ts_kit error\n");
        error = -EINVAL;
        goto out;
    }

    for_each_child_of_node(root, child)  //find the chip node
    {
        if (of_device_is_compatible(child, PARADE_VENDER_NAME))
        {
            found = true;
            break;
        }
    }
    if (!found){
		TS_LOG_ERR(" not found chip parade child node!\n");
        error = -EINVAL;
        goto out;
    }

    tskit_parade_data->parade_chip_data->cnode = child;
    tskit_parade_data->parade_chip_data->ops = &ts_kit_parade_ops;

    error = huawei_ts_chip_register(tskit_parade_data->parade_chip_data);
    if(error)
    {
	  TS_LOG_ERR(" parade chip register fail !\n");
	  goto out;
    }
    TS_LOG_INFO("parade chip_register! err=%d\n", error);
    return error;
out:
    if(tskit_parade_data->parade_chip_data) {
		kfree(tskit_parade_data->parade_chip_data);
		tskit_parade_data->parade_chip_data = NULL;/*Fix the DTS parse error cause panic bug*/
	}

    if (tskit_parade_data) {
		kfree(tskit_parade_data);
		tskit_parade_data = NULL;
    }
    return error;
}

static void __exit parade_module_exit(void)
{
    
   TS_LOG_INFO("parade_module_exit called here\n");

    return;
}

late_initcall(parade_module_init);
module_exit(parade_module_exit);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");


