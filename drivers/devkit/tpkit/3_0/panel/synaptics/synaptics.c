#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include "synaptics.h"

#include <linux/regulator/consumer.h>
#include <huawei_platform/log/log_jank.h>
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

#define SYNAPTICS_CHIP_INFO  "synaptics-"
#define SYNAPTICS_VENDER_NAME  "synaptics"
#define EASY_WAKEUP_FASTRATE 0x011D
#define F54_ANALOG_CMD0  0x016F
#define PALM_REG_BIT 0x01
#define GRIP_DATA_NUM 60
#define MAX_FINGER_NUM 10

#define LOCUS_DATA_NUM 24
#define LOCUS_DATA_LENS 4
#define LETTER_LOCUS_NUM 6
#define LINEAR2_LOCUS_NUM 4
#define LINEAR_LOCUS_NUM 2

/*#define GESTURE_FROM_APP(_x) (_x)*/
/*#define F51_CUSTOM_DATA24_OFFSET 24*/
#define F11_2D_DATA38_OFFSET 54
#define F11_2D_DATA39_OFFSET 55
#define F12_2D_CTRL_LEN 14
#define F12_RX_NUM_OFFSET 12
#define F12_TX_NUM_OFFSET 13

/*Gesture register(F11_2D_data38) value*/
#define DOUBLE_CLICK_WAKEUP  (1<<0)
#define LINEAR_SLIDE_DETECTED  (1<<1)
#define CIRCLE_SLIDE_DETECTED  (1<<3)
#define SPECIFIC_LETTER_DETECTED  (1<<6)
#define DOUBLE_CLICK_S3320_NO 0x03
#define LINEAR_SLIDE_S3320_NO 0x07
#define SPECIFIC_LETTER_S3320_NO 0x0b

/*Linear esture register(F51_custom_data0) value*/
#define LINEAR_SLIDE_LEFT_TO_RIGHT 0X01
#define LINEAR_SLIDE_RIGHT_TO_LEFT 0X02
#define LINEAR_SLIDE_TOP_TO_BOTTOM 0X04
#define LINEAR_SLIDE_BOTTOM_TO_TOP 0X08
#define LINEAR_SLIDE_TOP_TO_BOTTOM2 0X80

/*Letter gesture register(F11_2D_DATA39 (04)/01) value*/
#define SPECIFIC_LETTER_c 0x63
#define SPECIFIC_LETTER_e 0x65
#define SPECIFIC_LETTER_m 0x6D
#define SPECIFIC_LETTER_w 0x77

/*F51 no standard account offset ,so this register must be modified following firmware++++++*/
/*#define PEN_SWITCH_OFFSET  4*/
#define S3350_HOLSTER_SWITCH_OFFSET    10
#define S3320_HOLSTER_SWITCH_OFFSET    3
#define S3350_F51_TOUCHPLUS_OFFSET         0x0E
#define S3320_F51_TOUCHPLUS_OFFSET         0x1B
#define TOUCHPLUS_DOWNUP_BIT04         0x10
#define PRODUCT_ID_FW_LEN 5
#define PROJECT_ID_FW_LEN 9
#define S3320_HOLSTER_FORCE_GLOVE_OFFSET 0x19
#define S3320_HOLSTER_WINDOW_REG_BYTES 8
#define S3320_F51_CUSTOM_CTRL19_OFFSET 15
#define S3320_F51_CUSTOM_CTRL15_OFFSET 11
#define S3320_F54_CMD_BASE_ADDR 0x015C

#define SYNAPTIC_DEFAULT_I2C_ADDR 0x70
#define MULTI_PROTOCAL_1 1
#define MULTI_PROTOCAL_2 2
#define SIZE_OF_QUERY8 3

#ifndef SYNA_UPP
#define SYNA_UPP
#endif
#define USE_F12_DATA_15
#define COMM2_MESSAGE_MARKER 0xa5

/* This workaround comes from official driver project of synaptics.
   See https://github.com/SynapticsHostSoftware/synaptics_dsx_public. */
#ifdef USE_F12_DATA_15
#define F12_DATA_15_WORKAROUND
#endif
static bool f51found = false;
static u8 f51_roi_switch = 0;
static u8 pre_finger_status = 0;
static u8 roi_data_staled = 0;
static struct completion roi_data_done;
static unsigned char roi_data[ROI_DATA_READ_LENGTH] = { 0 };
static unsigned char diff_data_buf[DIFF_DATA_MAX_LEN] = { 0 };

#define EACH_FINGER_DIFF_DATA_LEN    12
#define MAX_SUPPORT_FINGER                5
static unsigned char current_finger_num = 0;


#define IS_APP_ENABLE_GESTURE(x)  ((u32)(1<<x))
static struct mutex wrong_touch_lock;
static DEFINE_MUTEX(ts_power_gpio_sem);
static unsigned char config_id_string[CHIP_INFO_LENGTH] = { 0 };
#ifdef SYNA_UPP
extern int fwu_read_f34_queries(void);
#endif
extern int hwlog_to_jank(int tag, int prio, const char* fmt, ...);
extern int ts_oemdata_type_check_legal(u8 type, u8 len);
extern int fb_esd_recover_disable(int value);
#if defined (CONFIG_TEE_TUI)
extern struct ts_tui_data tee_tui_data;
#endif
extern int synap_debug_data_test(struct ts_diff_data_info *info);
extern int synap_fw_s3718_configid(struct synaptics_rmi4_data *rmi4_data,
				 u8 *buf,
				 size_t buf_size);
#ifdef SYNA_UPP
extern short synap_get_oem_data_info( void );
extern int synap_get_oem_data(unsigned char *oem_data, unsigned short leng);
extern int synap_set_oem_data(unsigned char *oem_data, unsigned short leng);
#endif

static int synaptics_chip_detect (struct ts_kit_platform_data *data);
static int synaptics_wrong_touch(void);
static int synaptics_init_chip(void);
static int synaptics_get_brightness_info(void);

static int synaptics_irq_top_half(struct ts_cmd_node *cmd);
static int synaptics_irq_bottom_half(struct ts_cmd_node *in_cmd,
				     struct ts_cmd_node *out_cmd);
static int synaptics_fw_update_boot(char *file_name);
static int synaptics_fw_update_sd(void);
static int synaptics_oem_info_switch(struct ts_oem_info_param *info);
static int synaptics_chip_get_info(struct ts_chip_info_param *info);
static int synaptics_set_info_flag(struct ts_kit_platform_data *info);
static int synaptics_before_suspend(void);
static int synaptics_suspend(void);
static int synaptics_resume(void);
static int synaptics_after_resume(void *feature_info);
static int synaptics_wakeup_gesture_enable_switch(struct
						  ts_wakeup_gesture_enable_info
						  *info);
static void synaptics_shutdown(void);
static int synaptics_input_config(struct input_dev *input_dev);
static int synaptics_reset_device(void);
static int synaptics_rmi4_reset_device(struct synaptics_rmi4_data *rmi4_data);
static int synaptics_rmi4_status_resume(struct synaptics_rmi4_data *rmi4_data);
static int synaptics_rmi4_status_save(struct synaptics_rmi4_data *rmi4_data);
static void synaptics_rmi4_empty_fn_list(struct synaptics_rmi4_data *rmi4_data);
static void synaptics_rmi4_f1a_kfree(struct synaptics_rmi4_fn *fhandler);
static int synaptics_get_debug_data(struct ts_diff_data_info *info,
				    struct ts_cmd_node *out_cmd);
static int synaptics_get_rawdata(struct ts_rawdata_info *info,
				 struct ts_cmd_node *out_cmd);
static int synaptics_get_calibration_data(struct ts_calibration_data_info *info,
		struct ts_cmd_node *out_cmd);
static int synaptics_get_calibration_info(struct ts_calibration_info_param *info,
		struct ts_cmd_node *out_cmd);
static int synaptics_palm_switch(struct ts_palm_info *info);
static int synaptics_glove_switch(struct ts_glove_info *info);
static int synaptics_get_glove_switch(u8 *glove_switch);
static int synaptics_set_glove_switch(u8 glove_switch);
static int synaptics_charger_switch(struct ts_charger_info *info);
static int synaptics_holster_switch(struct ts_holster_info *info);
static int synaptics_roi_switch(struct ts_roi_info *info);
static unsigned char *synaptics_roi_rawdata(void);
static int synaptics_chip_get_capacitance_test_type(struct ts_test_type_info
						    *info);
static int synaptics_calibrate(void);
static int synaptics_calibrate_wakeup_gesture(void);
static int synaptics_regs_operate(struct ts_regs_info *info);
static void synaptics_rmi4_report_touch(struct synaptics_rmi4_data *rmi4_data,
					struct synaptics_rmi4_fn *fhandler,
					struct ts_fingers *info);
static int synaptics_rmi4_device_status_check(struct synaptics_rmi4_f01_device_status status);
void synaptics_set_screenoff_status_reg(void);

static int g_report_err_log_count = 0;

/* The following is a stub function. For hisilicon platform, it will be redefined in sensorhub module.
For qualcomm platform, it has not been implemented. Thus the stub function can avoid compilation errors.*/

__attribute__((weak)) void preread_fingersense_data(void)
{
    return;
}
#define GLOVE_SWITCH_ADDR 0x0400

#define GESTURE_ENABLE_BIT01 0x02

#define REPORT_2D_W

#define RPT_TYPE (1 << 0)
#define RPT_X_LSB (1 << 1)
#define RPT_X_MSB (1 << 2)
#define RPT_Y_LSB (1 << 3)
#define RPT_Y_MSB (1 << 4)
#define RPT_Z (1 << 5)
#define RPT_WX (1 << 6)
#define RPT_WY (1 << 7)
#define RPT_DEFAULT (RPT_TYPE | RPT_X_LSB | RPT_X_MSB | RPT_Y_LSB | RPT_Y_MSB)

#define MAX_ABS_MT_TOUCH_MAJOR 15

#define F01_STD_QUERY_LEN 21
#define F01_BUID_ID_OFFSET 18
#define F11_STD_QUERY_LEN 9
#define F11_STD_CTRL_LEN 10
#define F11_STD_DATA_LEN 12
/*dts*/
#define SYNAPTCS_IRQ_GPIO "attn_gpio"
#define SYNAPTCS_RST_GPIO "reset_gpio"
#define SYNAPTCS_SLAVE_ADDR "slave_address"
#define SYNAPTICS_VDDIO_GPIO_CTRL "vddio_ctrl_gpio"
#define SYNAPTICS_VCI_GPIO_CTRL "vci_ctrl_gpio"
#define SYNAPTCS_IRQ_CFG "irq_config"
#define SYNAPTICS_ALGO_ID "algo_id"
#define SYNAPTICS_VDD	 "synaptics-vdd"
#define SYNAPTICS_VBUS	 "synaptics-io"
#define SYNAPTICS_IC_TYPES	 "ic_type"
#define SYNAPTICS_WD_CHECK	"need_wd_check_status"
#define SYNAPTICS_X_MAX	 "x_max"
#define SYNAPTICS_Y_MAX	 "y_max"
#define SYNAPTICS_X_MAX_MT	 "x_max_mt"
#define SYNAPTICS_Y_MAX_MT	 "y_max_mt"
#define SYNAPTICS_UNIT_CAP_TEST_INTERFACE "unite_cap_test_interface"
#define SYNAPTICS_REPORT_RATE_TEST "report_rate_test"
#define SYNAPTICS_VCI_GPIO_TYPE	 "vci_gpio_type"
#define SYNAPTICS_VCI_REGULATOR_TYPE	 "vci_regulator_type"
#define SYNAPTICS_VDDIO_GPIO_TYPE	 "vddio_gpio_type"
#define SYNAPTICS_PROJECTID_LEN	 "projectid_len"
#define SYNAPTICS_VDDIO_REGULATOR_TYPE	 "vddio_regulator_type"
#define SYNAPTICS_COVER_FORCE_GLOVE     "force_glove_in_smart_cover"
#define SYNAPTICS_TEST_TYPE	 "tp_test_type"
#define SYNAPTICS_HOSTLER_SWITCH_ADDR "holster_switch_addr"
#define SYNAPTICS_HOSTLER_SWITCH_BIT "holster_switch_bit"
#define SYNAPTICS_GLOVE_SWITCH_ADDR "glove_switch_addr"
#define SYNAPTICS_GLOVE_SWITCH_BIT "glove_switch_bit"

#define SYNAPTICS_VCI_LDO_VALUE "vci_value"
#define SYNAPTICS_VDDIO_LDO_VALUE "vddio_value"
#define SYNAPTICS_NEED_SET_VDDIO_VALUE "need_set_vddio_value"

#define SYNAPTICS_FW_UPDATE_LOGIC "fw_update_logic"
#define SYNAPTICS_SELF_CAP_TEST	 "self_cap_test"
#define SYNAPTICS_CHECK_BULCKED  "check_bulcked"

#define TEST_ENHANCE_RAW_DATA_CAPACITANCE "test_enhance_raw_data_capacitance"
#define SHOULD_CHECK_TP_CALIBRATION_INFO "should_check_tp_calibration_info"
#define CSVFILE_TRX_DELTA_TEST "huawei,trx_delta_test_support"
#define TD43XX_EE_SHORT_TEST "huawei,td43xx_ee_short_test"
#define NO_TEST_CHANGE_RATE "huawei,no_change_report_rate"
#define THRESHOLD_TDDI_EE_SHORT_PARTONE "huawei,threshold_tddi_ee_short_partone"
#define THRESHOLD_TDDI_EE_SHORT_PARTTWO "huawei,threshold_tddi_ee_short_parttwo"
#define CSVFILE_USE_PRODUCT_SYSTEM_TYPE "huawei,csvfile_use_product_system"
#define TRX_SHORT_CIRCUIT "huawei,short_circuit_array"
#define SUPPORT_SHORT_TEST "huawei,support_s3320_short_test"
#define SUPPORT_EXT_TREX_SHORT_TEST "huawei,support_ext_trex_short_test"
#define EXT_TREX_SHORT_TEST_WAIT_REPORT_DELAY_FLAG "huawei,ext_trex_short_test_wait_report_delay_flag"
#define SUPPORT_FORCEKEY_CAP_VALUE_TEST "huawei,support_forcekey_cap_value_test"
#define SUPPORT_2DBARCODE_INFO "huawei,support_2dbarcode_info"

#define NOT_DELAY_ACT "delay_for_fw_update"
#define CRC_ERR_DO_RESET "crc_err_do_reset"

#define SYNAPTICS_MAX_REGDATA_NUM 32

#define S3718_IC_NAME	 "S3718"
#define S3718_IC_NAME_SIZE	 5

#define SYNA_VENDOR_NAME "synaptics"

enum TP_register_type {
	SYNA_REG_CTRL = 0,
	SYNA_REG_DATA = 1,
	SYNA_REG_QUERY = 2,
};

enum device_status {
	STATUS_NO_ERROR = 0x00,
	STATUS_RESET_OCCURRED = 0x01,
	STATUS_INVALID_CONFIG = 0x02,
	STATUS_DEVICE_FAILURE = 0x03,
	STATUS_CONFIG_CRC_FAILURE = 0x04,
	STATUS_FIRMWARE_CRC_FAILURE = 0x05,
	STATUS_CRC_IN_PROGRESS = 0x06,
	STATUS_GUEST_CRC_FAILURE = 0x07,
};

struct synaptics_work_reg_status {
	unsigned char fhandler_ctrl_base;
	unsigned char offset;
	unsigned char length;
};

static struct synaptics_work_reg_status dsm_dump_register_map[] = {
	/* read tp status whether in easy_wake up mode or power_off mode */
	[0] = {
	       .fhandler_ctrl_base = 0,
	       .offset = 0,
	       .length = 4,
	       },
	[1] = {
	       .fhandler_ctrl_base = 0,
	       .offset = 0,
	       .length = 1,
	       },
};

struct synaptics_rmi4_f54_query {
	union {
		struct {
			/* query 0 */
			unsigned char num_of_rx_electrodes;
			/* query 1 */
			unsigned char num_of_tx_electrodes;
			/* query 2 */
			unsigned char f54_query2_b0__1:2;
			unsigned char has_baseline:1;
			unsigned char has_image8:1;
			unsigned char f54_query2_b4__5:2;
			unsigned char has_image16:1;
			unsigned char f54_query2_b7:1;
			/* queries 3.0 and 3.1 */
			unsigned short clock_rate;
			/* query 4 */
			unsigned char touch_controller_family;
			/* query 5 */
			unsigned char has_pixel_touch_threshold_adjustment:1;
			unsigned char f54_query5_b1__7:7;
			/* query 6 */
			unsigned char has_sensor_assignment:1;
			unsigned char has_interference_metric:1;
			unsigned char has_sense_frequency_control:1;
			unsigned char has_firmware_noise_mitigation:1;
			unsigned char has_ctrl11:1;
			unsigned char has_two_byte_report_rate:1;
			unsigned char has_one_byte_report_rate:1;
			unsigned char has_relaxation_control:1;
		} __packed;
		unsigned char data[8];
	};
};

struct synaptics_rmi4_f1a_query {
	union {
		struct {
			unsigned char max_button_count:3;
			unsigned char reserved:5;
			unsigned char has_general_control:1;
			unsigned char has_interrupt_enable:1;
			unsigned char has_multibutton_select:1;
			unsigned char has_tx_rx_map:1;
			unsigned char has_perbutton_threshold:1;
			unsigned char has_release_threshold:1;
			unsigned char has_strongestbtn_hysteresis:1;
			unsigned char has_filter_strength:1;
		} __packed;
		unsigned char data[2];
	};
};

struct synaptics_rmi4_f1a_control_0 {
	union {
		struct {
			unsigned char multibutton_report:2;
			unsigned char filter_mode:2;
			unsigned char reserved:4;
		} __packed;
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f1a_control {
	struct synaptics_rmi4_f1a_control_0 general_control;
	unsigned char *button_int_enable;
	unsigned char *multi_button;
	struct synaptics_rmi4_f1a_control_3_4 *electrode_map;
	unsigned char *button_threshold;
	unsigned char button_release_threshold;
	unsigned char strongest_button_hysteresis;
	unsigned char filter_strength;
};

struct synaptics_rmi4_f1a_handle {
	int button_bitmask_size;
	unsigned char button_count;
	unsigned char valid_button_count;
	unsigned char *button_data_buffer;
	unsigned char *button_map;
	struct synaptics_rmi4_f1a_query button_query;
	struct synaptics_rmi4_f1a_control button_control;
};
struct synaptics_rmi4_data *rmi4_data;


/* when add/remove support TP module need to modify here */


//static struct touch_settings *synaptics_sett_param_regs =
//    &synaptics_sett_param_regs_map[10];
static struct touch_settings *synaptics_sett_param_regs;
static int synaptics_interrupt_num;

static void synaptics_gpio_reset(void);
static void synaptics_power_on(void);
static void synaptics_power_on_gpio_set(void);
static void synaptics_power_off(void);
static void synaptics_power_off_gpio_set(void);
static bool synaptics_rmi4_crc_in_progress(struct synaptics_rmi4_data
					   *rmi4_data,
					   struct
					   synaptics_rmi4_f01_device_status
					   *status);
static int synaptics_rmi4_set_page(struct synaptics_rmi4_data *rmi4_data,
				   unsigned int address);
static struct synaptics_rmi4_fn *synaptics_rmi4_alloc_fh(struct
							 synaptics_rmi4_fn_desc
							 *rmi_fd,
							 int page_number);
static int synaptics_rmi4_query_device_info(struct synaptics_rmi4_data
					    *rmi4_data);
static int synaptics_rmi4_query_device(struct synaptics_rmi4_data *rmi4_data);
static int synaptics_rmi4_f11_init(struct synaptics_rmi4_data *rmi4_data,
				   struct synaptics_rmi4_fn *fhandler,
				   struct synaptics_rmi4_fn_desc *fd,
				   unsigned int intr_count);
static int synaptics_rmi4_f12_init(struct synaptics_rmi4_data *rmi4_data,
				   struct synaptics_rmi4_fn *fhandler,
				   struct synaptics_rmi4_fn_desc *fd,
				   unsigned int intr_count);
static int synaptics_rmi4_f1a_init(struct synaptics_rmi4_data *rmi4_data,
				   struct synaptics_rmi4_fn *fhandler,
				   struct synaptics_rmi4_fn_desc *fd,
				   unsigned int intr_count);
static int synaptics_rmi4_f51_init(struct synaptics_rmi4_data *rmi4_data,
				   struct synaptics_rmi4_fn *fhandler,
				   struct synaptics_rmi4_fn_desc *fd,
				   unsigned int intr_count, unsigned char page);
static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
				   unsigned short addr, unsigned char *data,
				   unsigned short length);
static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
				    unsigned short addr, unsigned char *data,
				    unsigned short length);
static int synaptics_rmi4_sensor_report(struct synaptics_rmi4_data *rmi4_data,
					struct ts_fingers *info);
/* dts */
static int synaptics_pinctrl_get_init(void)
{
	int ret = 0;

	rmi4_data->pctrl = devm_pinctrl_get(&rmi4_data->synaptics_dev->dev);
	if (IS_ERR(rmi4_data->pctrl)) {
		TS_LOG_ERR("failed to devm pinctrl get\n");
		ret = -EINVAL;
		return ret;
	}

	rmi4_data->pins_default =
	    pinctrl_lookup_state(rmi4_data->pctrl, "default");
	if (IS_ERR(rmi4_data->pins_default)) {
		TS_LOG_ERR("failed to pinctrl lookup state default\n");
		ret = -EINVAL;
		goto err_pinctrl_put;
	}

	rmi4_data->pins_idle = pinctrl_lookup_state(rmi4_data->pctrl, "idle");
	if (IS_ERR(rmi4_data->pins_idle)) {
		TS_LOG_ERR("failed to pinctrl lookup state idle\n");
		ret = -EINVAL;
		goto err_pinctrl_put;
	}
	return 0;

err_pinctrl_put:
	devm_pinctrl_put(rmi4_data->pctrl);
	return ret;
}

static int synaptics_pinctrl_select_normal(void)
{
	int retval = NO_ERR;

	retval =
	    pinctrl_select_state(rmi4_data->pctrl, rmi4_data->pins_default);
	if (retval < 0) {
		TS_LOG_ERR("set iomux normal error, %d\n", retval);
	}
	return retval;
}

static int synaptics_pinctrl_select_lowpower(void)
{
	int retval = NO_ERR;
	retval = pinctrl_select_state(rmi4_data->pctrl, rmi4_data->pins_idle);
	if (retval < 0) {
		TS_LOG_ERR("set iomux lowpower error, %d\n", retval);
	}
	return retval;
}

#define GLOVE_SIGNAL
#ifdef GLOVE_SIGNAL
static int rmi_f11_read_finger_state(struct ts_fingers *info)
{
	int i = 0;
	int retval = 0;
	u8 finger_state = 0;
	u16 f51_custom_CTRL03 = 0x0015;

	retval = synaptics_rmi4_i2c_read(rmi4_data, f51_custom_CTRL03,
					 (u8 *) &finger_state,
					 sizeof(finger_state));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read f51_custom_CTRL03, code: %d.\n",
			   retval);
		return retval;
	}

	for (i = 0; i < FILTER_GLOVE_NUMBER; i++) {
		info->fingers[i].status = (finger_state >> (2 * i)) & MASK_2BIT;
		if (info->fingers[i].status == 2) {
			info->fingers[i].status = TP_GLOVE;
		}
	}
	return 0;
}
#endif

static void synaptics_ghost_detect(int value){
	if (GHOST_OPERATE_TOO_FAST & value){
		TS_LOG_INFO("%s operate too fast\n", __func__);
	}else if (GHOST_OPERATE_IN_XY_AXIS & value){
		TS_LOG_INFO("%s operate in same xy asix\n", __func__);
	}else if (GHOST_OPERATE_IN_SAME_POSITION & value){
		TS_LOG_INFO("%s operate in same position\n", __func__);
	}
	//DMD report

	if(rmi4_data->synaptics_chip_data->enable_ghost_dmd_report == 0) {
#if defined (CONFIG_HUAWEI_DSM)
		ts_dmd_report(DSM_TP_GHOST_TOUCH_ERROR_NO, "noise record number: %d.noise_record_num is %d.frequency_selection is %d\n",
			value,rmi4_data->synaptics_chip_data->noise_record_num,
			rmi4_data->synaptics_chip_data->frequency_selection_reg);
#endif
		rmi4_data->synaptics_chip_data->enable_ghost_dmd_report++;
	}

	rmi4_data->synaptics_chip_data->noise_record_num = 0;
}

static bool synaptics_read_crc_value(struct synaptics_rmi4_f01_device_status *status, unsigned short ic_status_reg)
{
	int ret = NO_ERR;

	ret = synaptics_rmi4_i2c_read(rmi4_data, ic_status_reg, status->data, sizeof(status->data));
	if (ret < 0) {
		TS_LOG_ERR("read ic_status_reg = %d status register failed\n", ic_status_reg);
		return false;
	}

	return true;
}

static void synaptics_check_crc_status(unsigned short ic_status_reg)
{
	struct synaptics_rmi4_f01_device_status status;
	bool crc_check_res = false;

	crc_check_res = synaptics_read_crc_value(&status, ic_status_reg);
	if (!crc_check_res) {
		TS_LOG_INFO("can not read out crc\n");
		return;
	}

	if (synaptics_rmi4_device_status_check(status)) {
		TS_LOG_ERR("device status is error, status value = 0x%02x\n", status.status_code);
#if defined (CONFIG_HUAWEI_DSM)
		ts_dmd_report(DSM_TP_DEV_STATUS_ERROR_NO, "[synaptics]device status for 20004 is:%d\n", status.status_code);
#endif
	}
	return;
}

static int synaptics_chip_check_status(void){
	int retval = NO_ERR;
	unsigned char value = 0;
	static unsigned char noise_state_reg = 0;
	static unsigned char frequency_selection_reg = 0;

	TS_LOG_DEBUG("%s +\n", __func__);

	if (rmi4_data->synaptics_chip_data->noise_state_reg){
		retval = rmi4_data->i2c_read(rmi4_data, rmi4_data->synaptics_chip_data->noise_state_reg,
							&value, sizeof(unsigned char));
		if (retval < 0){
			atomic_set(&rmi4_data->synaptics_chip_data->ts_platform_data->ts_esd_state, TS_ESD_HAPPENDED);
			TS_LOG_ERR("%s read error\n", __func__);
			return 0;
		}

		if(noise_state_reg != value) {
			noise_state_reg = value;
			TS_LOG_INFO("%s noise state reg change:%d\n", __func__,noise_state_reg);
		}

		switch(noise_state_reg){
			/* noise nomal */
			case NOISE_VALUE_0:
			case NOISE_VALUE_1:
				TS_LOG_DEBUG("%s noise state normal\n", __func__);
				break;
			/* noise abnormal */
			case NOISE_VALUE_2:
				TS_LOG_INFO("%s noise state abnormal\n", __func__);
				if (rmi4_data->synaptics_chip_data->noise_record_num < NOISE_RECORD_NUM_MAX)
					rmi4_data->synaptics_chip_data->noise_record_num++;
				break;
			default:
				TS_LOG_INFO("%s noise state unknown, abnormal:%d\n", __func__, value);
				break;
		}
	}

	if(rmi4_data->synaptics_chip_data->frequency_selection_reg){
		retval = rmi4_data->i2c_read(rmi4_data, rmi4_data->synaptics_chip_data->frequency_selection_reg,
							&value, sizeof(unsigned char));
		if (retval < 0) {
			TS_LOG_INFO("%s read error\n", __func__);
			return 0;
		}
		if(frequency_selection_reg != value) {
			frequency_selection_reg = value;
			TS_LOG_INFO("frequency_selection reg value change: %d\n", __func__,frequency_selection_reg);
		}
	}

	if (rmi4_data->synaptics_chip_data->ic_status_reg) {
		/*check crc status*/
		synaptics_check_crc_status(rmi4_data->synaptics_chip_data->ic_status_reg);
	}

	TS_LOG_DEBUG("noise_state_reg : %d frequency_selection_reg : %d\n",noise_state_reg,frequency_selection_reg);
	return 0;
}

static void synaptics_doze_enable(unsigned char oper, unsigned char param) {
	int error = 0;
	unsigned char value = 0;

	if ((TS_SWITCH_TYPE_DOZE !=
			(rmi4_data->synaptics_chip_data->touch_switch_flag & TS_SWITCH_TYPE_DOZE))){
		TS_LOG_ERR("doze mode is not supported\n");
		return;
	}

	switch (oper) {
		case TS_SWITCH_DOZE_ENABLE:
			/* 0x000C Value"0"->On; Value"1"->Off */
			value = 0;
			error = synaptics_rmi4_i2c_read(rmi4_data,
						rmi4_data->synaptics_chip_data->touch_switch_reg,
						&value,
						(unsigned short)sizeof(unsigned char));
			if (error < 0) {
				TS_LOG_ERR("read form  addr(%02x) error\n",
					rmi4_data->synaptics_chip_data->touch_switch_reg);
				break;
			}else{
				TS_LOG_INFO("read doze enable addr:%02x=%u\n",
					rmi4_data->synaptics_chip_data->touch_switch_reg, value);
			}

			if (value & (1<<2)){
				value &= ~(1<<2);
				TS_LOG_INFO("set doze on:%u\n", value);

				if (param > 0){
					/* 0x0010 Value"1"(Dex)->1ms; Max->25500(Dex) */
					error = synaptics_rmi4_i2c_write(rmi4_data,
								rmi4_data->synaptics_chip_data->touch_switch_hold_off_reg,
								&param,
								(unsigned short)sizeof(unsigned char));
					if (error < 0) {
						TS_LOG_ERR("write doze hold off time addr:%02x=%u error\n",
							rmi4_data->synaptics_chip_data->touch_switch_hold_off_reg, param);
						break;
					}
				}

				error = synaptics_rmi4_i2c_write(rmi4_data,
							rmi4_data->synaptics_chip_data->touch_switch_reg,
							&value,
							(unsigned short)sizeof(unsigned char));
				if (error < 0) {
					TS_LOG_ERR("write doze enable addr:%u=%u error\n",
						rmi4_data->synaptics_chip_data->touch_switch_reg, value);
					break;
				}
			}else{
				TS_LOG_INFO("doze already on\n");
			}
			break;
		case TS_SWITCH_DOZE_DISABLE:
			if ((TS_SWITCH_TYPE_DOZE !=
				(rmi4_data->synaptics_chip_data->touch_switch_flag & TS_SWITCH_TYPE_DOZE))){
				TS_LOG_ERR("doze mode not supprot\n");
				break;
			}
			/* Value"0"->On; Value"1"->Off */
			value = 0;
			error = synaptics_rmi4_i2c_read(rmi4_data,
						rmi4_data->synaptics_chip_data->touch_switch_reg,
						&value,
						(unsigned short)sizeof(unsigned char));
			if (error < 0) {
				TS_LOG_ERR("read form  addr(%02x) error\n",
					rmi4_data->synaptics_chip_data->touch_switch_reg);
				break;
			}else{
				TS_LOG_INFO("read doze enable addr:%02x=%u\n",
					rmi4_data->synaptics_chip_data->touch_switch_reg, value);
			}

			if (!(value & (1<<2))){
				value |= (1<<2);
				TS_LOG_INFO("set doze off:%u\n", value);

				error = synaptics_rmi4_i2c_write(rmi4_data,
							rmi4_data->synaptics_chip_data->touch_switch_reg,
							&value,
							(unsigned short)sizeof(unsigned char));
				if (error < 0) {
					TS_LOG_ERR("write doze enable addr:%u=%u error\n",
						rmi4_data->synaptics_chip_data->touch_switch_reg, value);
					break;
				}
			}else{
				TS_LOG_INFO("doze already off\n");
			}
			break;
		default:
			TS_LOG_ERR("%s, oper unknown:%d, invalid\n", __func__, oper);
			break;
	}
}

static void synaptics_game_mode_enable(unsigned char oper) {
	int error = 0;
	unsigned char value = 0;

	if (TS_SWITCH_TYPE_GAME!=
			(rmi4_data->synaptics_chip_data->touch_switch_flag & TS_SWITCH_TYPE_GAME)){
		TS_LOG_ERR("game mode is not supported\n");
		return;
	}

	switch (oper) {
		case TS_SWITCH_GAME_ENABLE:
			/* 0x000C Value"0"->On; Value"1"->Off */
			value = 0;
			error = synaptics_rmi4_i2c_read(rmi4_data,
						rmi4_data->synaptics_chip_data->touch_game_reg,
						&value,
						(unsigned short)sizeof(unsigned char));
			if (error < 0) {
				TS_LOG_ERR("read form  addr(%02x) error\n",
					rmi4_data->synaptics_chip_data->touch_game_reg);
				break;
			}else{
				TS_LOG_INFO("read game enable addr:%02x=%u\n",
					rmi4_data->synaptics_chip_data->touch_game_reg, value);
			}

			value |= rmi4_data->synaptics_chip_data->game_control_bit;
			TS_LOG_INFO("set game on:%u\n", value);

			error = synaptics_rmi4_i2c_write(rmi4_data,
						rmi4_data->synaptics_chip_data->touch_game_reg,
						&value,
						(unsigned short)sizeof(unsigned char));
			if (error < 0) {
				TS_LOG_ERR("write doze game addr:%u=%u error\n",
					rmi4_data->synaptics_chip_data->touch_game_reg, value);
				break;
			}

			break;
		case TS_SWITCH_GAME_DISABLE:
			if (TS_SWITCH_TYPE_GAME!=
				(rmi4_data->synaptics_chip_data->touch_switch_flag & TS_SWITCH_TYPE_GAME)){
				TS_LOG_ERR("touch switch not supprot\n");
				break;
			}
			/* Value"0"->On; Value"1"->Off */
			value = 0;
			error = synaptics_rmi4_i2c_read(rmi4_data,
						rmi4_data->synaptics_chip_data->touch_game_reg,
						&value,
						(unsigned short)sizeof(unsigned char));
			if (error < 0) {
				TS_LOG_ERR("read form  addr(%02x) error\n",
					rmi4_data->synaptics_chip_data->touch_game_reg);
				break;
			}else{
				TS_LOG_INFO("read game enable addr:%02x=%u\n",
					rmi4_data->synaptics_chip_data->touch_game_reg, value);
			}

			value &= ~(rmi4_data->synaptics_chip_data->game_control_bit);
			TS_LOG_INFO("set game off:%u\n", value);

			error = synaptics_rmi4_i2c_write(rmi4_data,
						rmi4_data->synaptics_chip_data->touch_game_reg,
						&value,
						(unsigned short)sizeof(unsigned char));
			if (error < 0) {
				TS_LOG_ERR("write game enable addr:%u=%u error\n",
					rmi4_data->synaptics_chip_data->touch_game_reg, value);
				break;
			}
			break;
		default:
			TS_LOG_ERR("%s, oper unknown:%d, invalid\n", __func__, oper);
			break;
	}
}

static void synaptics_scene_switch(unsigned char scene, unsigned char oper)
{
	int error = 0;
	unsigned char value = 0;

	if (TS_SWITCH_TYPE_SCENE !=
			(rmi4_data->synaptics_chip_data->touch_switch_flag & TS_SWITCH_TYPE_SCENE)) {
		TS_LOG_ERR("%s, scene switch is not suppored by this chip\n",__func__);
		return;
	}

	switch (oper) {
		case TS_SWITCH_SCENE_ENTER:
			TS_LOG_INFO("%s: enter scene %d\n", __func__, scene);
			value = scene;
			error = synaptics_rmi4_i2c_write(rmi4_data,
						rmi4_data->synaptics_chip_data->touch_scene_reg,
						&value,
						(unsigned short)sizeof(unsigned char));
			if (error < 0) {
				TS_LOG_ERR("write scene switch addr:%x=%u error\n",
					rmi4_data->synaptics_chip_data->touch_scene_reg, scene);
			}
			break;
		case TS_SWITCH_SCENE_EXIT:
			TS_LOG_INFO("%s: enter default scene\n", __func__);
			value = 0;
			error = synaptics_rmi4_i2c_write(rmi4_data,
						rmi4_data->synaptics_chip_data->touch_scene_reg,
						&value,
						(unsigned short)sizeof(unsigned char));
			if (error < 0) {
				TS_LOG_ERR("write scene switch addr:%x=%u error\n",
					rmi4_data->synaptics_chip_data->touch_scene_reg, scene);
			}
			break;
		default:
			TS_LOG_ERR("%s: oper unknown:%d, invalid\n", __func__, oper);
			break;
	}
}

static void synaptics_chip_touch_switch(void){
	unsigned long get_value = 0;
	char *ptr_begin = NULL, *ptr_end = NULL;
	char in_data[MAX_STR_LEN] = {0};
	int len = 0;
	unsigned char stype = 0, soper = 0, param = 0;
	int error = 0;

	TS_LOG_INFO("%s +\n", __func__);

	if (NULL == rmi4_data->synaptics_chip_data){
		TS_LOG_ERR("error chip data\n");
		goto out;
	}

	/* SWITCH_OPER,ENABLE_DISABLE,PARAM */
	memcpy(in_data, rmi4_data->synaptics_chip_data->touch_switch_info, MAX_STR_LEN -1);
	TS_LOG_INFO("in_data:%s\n", in_data);

	/* get switch type */
	ptr_begin = in_data;
	ptr_end = strstr(ptr_begin, ",");
	if (!ptr_end){
		TS_LOG_ERR("%s get stype fail\n", __func__);
		goto out;
	}
	len = ptr_end - ptr_begin;
	if (len <= 0 || len > 3){
		TS_LOG_ERR("%s stype len error\n", __func__);
		goto out;
	}
	*ptr_end = 0;
	error = strict_strtoul(ptr_begin, 0, &get_value);
	if (error) {
		TS_LOG_ERR("strict_strtoul return invaild :%d\n", error);
		goto out;
	}else{
		stype = (unsigned char)get_value;
		TS_LOG_INFO("%s get stype:%u\n", __func__, stype);
	}

	/* get switch operate */
	ptr_begin = ptr_end + 1;
	ptr_end = strstr(ptr_begin, ",");
	if (!ptr_end){
		TS_LOG_ERR("%s get soper fail\n", __func__);
		goto out;
	}
	len = ptr_end - ptr_begin;
	if (len <= 0 || len > 3){
		TS_LOG_ERR("%s soper len error\n", __func__);
		goto out;
	}
	*ptr_end = 0;
	error = strict_strtoul(ptr_begin, 0, &get_value);
	if (error) {
		TS_LOG_ERR("strict_strtoul return invaild :%d\n", error);
		goto out;
	}else{
		soper = (unsigned char)get_value;
		TS_LOG_INFO("%s get soper:%u\n", __func__, soper);
	}

	/* get param */
	ptr_begin = ptr_end + 1;
	error = strict_strtoul(ptr_begin, 0, &get_value);
	if (error) {
		TS_LOG_ERR("strict_strtoul return invaild :%d\n", error);
		goto out;
	}else{
		param = (unsigned char)get_value;
		TS_LOG_INFO("%s get param:%u\n", __func__, param);
	}

	switch (stype){
		case TS_SWITCH_TYPE_DOZE:
			synaptics_doze_enable(soper, param);
			break;
		case TS_SWITCH_SCENE_3:
		case TS_SWITCH_SCENE_4:
			synaptics_scene_switch(stype, soper);
			break;
		case TS_SWITCH_SCENE_5:
			synaptics_game_mode_enable(soper);
			break;
		default:
			TS_LOG_INFO("touch switch type %d not supported.", stype);
			break;
	}

	TS_LOG_INFO("%s -\n", __func__);
out:
	return ;
}

void read_diff_data(void)
{
	int retval = 0;
	unsigned short diff_data_addr = rmi4_data->synaptics_chip_data->diff_data_control_addr;
	unsigned short diff_data_len =  rmi4_data->synaptics_chip_data->diff_data_len;
	unsigned short  curren_len =0;
	unsigned char  diff_data_tmp[DIFF_DATA_MAX_LEN]={0};
	unsigned char temp_finger = (current_finger_num > MAX_SUPPORT_FINGER ? MAX_SUPPORT_FINGER : current_finger_num);

	TS_LOG_DEBUG("%s: [DIFF_DATA]in.\n",__func__ );
	curren_len = temp_finger * EACH_FINGER_DIFF_DATA_LEN;

	memset(diff_data_tmp, 0xFF, DIFF_DATA_MAX_LEN);//set invalid data to 0xFF  for deamon
	if((diff_data_len <= 0)||(diff_data_len > DIFF_DATA_MAX_LEN)
		||(curren_len <= 0) ||(curren_len > DIFF_DATA_MAX_LEN))
		return ;

	retval = synaptics_rmi4_i2c_read(rmi4_data, diff_data_addr,
				    diff_data_tmp,
				    curren_len);
	if (retval < 0) {
		TS_LOG_ERR("[DIFF_DATA] F12 Failed to read diff data, retval= %d \n",  retval);
	}
	else {
		memcpy(rmi4_data->synaptics_chip_data->diff_data,
					diff_data_tmp, diff_data_len);
	}
	TS_LOG_DEBUG("%s: [DIFF_DATA] out.\n",__func__ );
	return ;
}

void synaptics_work_after_input_kit(void)
{
	int retval = 0;
	unsigned short roi_data_addr = 0;
	unsigned char roi_tmp[ROI_DATA_READ_LENGTH]={0};

	if (f51_roi_switch)  {
		if (roi_data_staled == 0)  {  /* roi_data is up to date now. */
			goto out;
		}

		/* We are about to refresh roi_data. To avoid stale output, use a completion to block possible readers. */
		reinit_completion(&roi_data_done);

		/* Request sensorhub to report fingersense data. */
		preread_fingersense_data();

		roi_data_addr =
		    rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.roi_info.roi_data_addr;
		TS_LOG_DEBUG("roi_data_addr=0x%04x\n", roi_data_addr);

		retval =
		    synaptics_rmi4_i2c_read(rmi4_data, roi_data_addr,
					    roi_tmp,
					    ROI_DATA_READ_LENGTH);
		if (retval < 0) {
			TS_LOG_ERR
			    ("F12 Failed to read roi data, retval= %d \n",
			     retval);
		}
		else {
			/* roi_data pointer will be exported to app by function synaptics_roi_rawdata, so there's a race
			   condition while app reads roi_data and our driver refreshes it at the same time. We use local
			   variable(roi_tmp) to decrease the conflicting time window. Thus app will get consistent data,
			   except when we are executing the following memcpy. */
			memcpy(roi_data, roi_tmp, sizeof(roi_tmp));
		}

		roi_data_staled = 0;
		complete_all(&roi_data_done);  /* If anyone has been blocked by us, wake it up. */
	}
	return;
out:
	if( rmi4_data->synaptics_chip_data->diff_data_report_supported){
		read_diff_data();
	}
	return;
}
static int synaptics_boot_detection(void)
{
	unsigned short detection_addr = 0;
	int retval = NO_ERR;
	u8 value = 0;
	int tmp_num_tx = 0;
	if(0 == rmi4_data->synaptics_chip_data->boot_detection_flag){
		TS_LOG_INFO("boot detection not support\n");
		return -EINVAL;
	}
	TS_LOG_INFO("boot detection support\n");
	tmp_num_tx = rmi4_data->num_of_tx;
	detection_addr = (unsigned short)rmi4_data->synaptics_chip_data->boot_detection_addr;
	retval = synaptics_rmi4_i2c_read(rmi4_data, detection_addr, &value, sizeof(value));
	if (retval < 0) {
		TS_LOG_ERR("read boot detection failed  %d\n", retval);
		return -EINVAL;
	}
#if defined (CONFIG_HUAWEI_DSM)
	if((value + tmp_num_tx) > rmi4_data->synaptics_chip_data->boot_detection_threshold) {
		TS_LOG_ERR("Super threshold read addr value[%d] + tmp_num_tx[%d] > detectionflag = %d",value, tmp_num_tx,
					rmi4_data->synaptics_chip_data->boot_detection_threshold);
		ts_dmd_report(DSM_TP_BOOT_DETECTION, "read boot detection addr =0x%02x\n", value);
	}
#endif
	return NO_ERR;
}

static void synaptics_chip_send_sensibility(int cur_value)
{

	unsigned short sensi_addr = 0;
	int pre_value = 0;
	int retval = NO_ERR;
	TS_LOG_DEBUG("%s:value = %d.\n",__func__ ,cur_value);
	if (true != rmi4_data->sensitivity_adjust_support){
		TS_LOG_INFO("sensitivity adjust not support\n");
		return;
	}
	sensi_addr = (unsigned short)rmi4_data->sensitivity_adjust_reg;

	retval = synaptics_rmi4_i2c_read(rmi4_data, sensi_addr, (unsigned char *)&pre_value, 1);
	if (retval < 0){
		TS_LOG_ERR("read sensitivity failed\n");
		return;
	}
	if( pre_value != cur_value){
		TS_LOG_INFO("set sensitivity_reg:0x%02x to %d \n",sensi_addr, cur_value);
		retval = synaptics_rmi4_i2c_write(rmi4_data, sensi_addr, (unsigned char *)&cur_value, 1);
		if (retval < 0){
			TS_LOG_ERR("write sensitivity failed\n");
			return;
		}
#if defined (CONFIG_HUAWEI_DSM)
		ts_dmd_report(DSM_TP_SENSIBILITY_CHANGE, "DSM_TP_SENSIBILITY_CHANGE(%d)->(%d)\n", pre_value,cur_value);
#endif
	}
	return ;
}


struct ts_device_ops ts_kit_synaptics_ops = {
	.chip_detect = synaptics_chip_detect,
	.chip_init = synaptics_init_chip,
	.chip_get_brightness_info = synaptics_get_brightness_info,
	.chip_input_config = synaptics_input_config,
	.chip_irq_top_half = synaptics_irq_top_half,
	.chip_irq_bottom_half = synaptics_irq_bottom_half,
	.chip_fw_update_boot = synaptics_fw_update_boot,
	.chip_fw_update_sd = synaptics_fw_update_sd,
	.oem_info_switch = synaptics_oem_info_switch,
	.chip_get_info = synaptics_chip_get_info,
	.chip_get_capacitance_test_type =
	    synaptics_chip_get_capacitance_test_type,
	.chip_set_info_flag = synaptics_set_info_flag,
	.chip_before_suspend = synaptics_before_suspend,
	.chip_suspend = synaptics_suspend,
	.chip_resume = synaptics_resume,
	.chip_after_resume = synaptics_after_resume,
	.chip_wakeup_gesture_enable_switch =
	    synaptics_wakeup_gesture_enable_switch,
	.chip_get_rawdata = synaptics_get_rawdata,
	.chip_get_calibration_data = synaptics_get_calibration_data,
	.chip_get_calibration_info = synaptics_get_calibration_info,
	.chip_get_debug_data = synaptics_get_debug_data,
	.chip_glove_switch = synaptics_glove_switch,
	.chip_shutdown = synaptics_shutdown,
	.chip_charger_switch = synaptics_charger_switch,
	.chip_holster_switch = synaptics_holster_switch,
	.chip_roi_switch = synaptics_roi_switch,
	.chip_roi_rawdata = synaptics_roi_rawdata,
	.chip_palm_switch = synaptics_palm_switch,
	.chip_regs_operate = synaptics_regs_operate,
	.chip_calibrate = synaptics_calibrate,
	.chip_calibrate_wakeup_gesture = synaptics_calibrate_wakeup_gesture,
	.chip_reset = synaptics_reset_device,
#ifdef HUAWEI_TOUCHSCREEN_TEST
	.chip_test = test_dbg_cmd_test,
#endif
	.chip_wrong_touch = synaptics_wrong_touch,
	.chip_work_after_input = synaptics_work_after_input_kit,
	.chip_ghost_detect = synaptics_ghost_detect,
	.chip_check_status = synaptics_chip_check_status,
	.chip_touch_switch = synaptics_chip_touch_switch,
	.chip_set_sensibility_cfg = synaptics_chip_send_sensibility,
	.chip_boot_detection = synaptics_boot_detection,
};

static int synaptics_get_rawdata(struct ts_rawdata_info *info,
				 struct ts_cmd_node *out_cmd)
{
	int retval = 0;
	if (rmi4_data->synaptics_chip_data->unite_cap_test_interface) {
		TS_LOG_INFO("++++ get rawdata in\n");
		retval =
		    synap_rmi4_f54_init(rmi4_data,
					    synaptics_sett_param_regs->
					    module_name);
		if (retval < 0) {
			TS_LOG_ERR("Failed to init f54\n");
			return retval;
		}

		retval = synap_get_cap_data(info);
		if (retval < 0) {
			TS_LOG_ERR("Failed to get rawdata\n");
			return retval;
		}
		return NO_ERR;
	} else {
		return -EINVAL;
	}
}

static int synaptics_get_calibration_data(struct ts_calibration_data_info *info, struct ts_cmd_node *out_cmd)
{
	int retval = 0;

	TS_LOG_INFO("%s called\n", __FUNCTION__);

	retval = synap_rmi4_f54_init(rmi4_data,synaptics_sett_param_regs->module_name);
	if (retval < 0) {
		TS_LOG_ERR("Failed to init f54\n");
		return retval;
	}

	retval = synap_get_calib_data(info);
	if (retval < 0) {
		TS_LOG_ERR("Failed to get calibration data\n");
		return retval;
	}

	return NO_ERR;
}

static int synaptics_get_calibration_info(struct ts_calibration_info_param *info, struct ts_cmd_node *out_cmd)
{
	int retval = 0;

	TS_LOG_INFO("%s called\n", __FUNCTION__);

	retval = synap_rmi4_f54_init(rmi4_data,synaptics_sett_param_regs->module_name);
	if (retval < 0) {
		TS_LOG_ERR("Failed to init f54\n");
		return retval;
	}

	retval = synap_get_calib_info(info);
	if (retval < 0) {
		TS_LOG_ERR("Failed to get calibration info\n");
		return retval;
	}

	return NO_ERR;
}

static int synaptics_get_debug_data(struct ts_diff_data_info *info,
				    struct ts_cmd_node *out_cmd)
{
	int retval = 0;

	TS_LOG_INFO("synaptics get diff data begin\n");

	retval =
	    synap_rmi4_f54_init(rmi4_data,
				    synaptics_sett_param_regs->module_name);
	if (retval < 0) {
		TS_LOG_ERR("Failed to init f54\n");
		return retval;
	}

	retval = synap_debug_data_test(info);
	if (retval < 0) {
		TS_LOG_ERR("Failed to get diff data\n");
		return retval;
	}

	TS_LOG_INFO("synaptics get diff data end\n");
	return NO_ERR;
}

static int synaptics_set_info_flag(struct ts_kit_platform_data *info)
{
	rmi4_data->synaptics_chip_data->ts_platform_data->get_info_flag = info->get_info_flag;
	return NO_ERR;
}

static u8 tp_result_info[TS_CHIP_TYPE_MAX_SIZE] = {0};
static u8 tp_type_cmd[TS_CHIP_TYPE_MAX_SIZE] = {0};
static int synaptics_reconstruct_barcode(struct ts_oem_info_param *info)
{
	 int retval = NO_ERR;
	 int offset1 = TS_NV_STRUCTURE_BAR_CODE_OFFSET1;
	 int offset2 = TS_NV_STRUCTURE_BAR_CODE_OFFSET2;
	 u8 type = 0;
	 u8 len = 0;

	 TS_LOG_INFO("%s enter\n", __func__);

	type = info->buff[offset1*16 + 0];
	len = info->buff[offset1*16 + 1];
	if ((type == 0x00 && len == 0x00) ||(type == 0xFF && len == 0xFF)) {
		 memcpy(&(info->buff[offset1*16]), tp_type_cmd, tp_type_cmd[1]*16);
		 TS_LOG_INFO("Will write the data to info_buff, offset is %s", offset1);
		 return retval;
	 }

	type = info->buff[offset2*16 + 0];
	len = info->buff[offset2*16 + 1];
	if ((type == 0x00 && len == 0x00) ||(type == 0xFF && len == 0xFF)) {
		 memcpy(&(info->buff[offset2*16]), tp_type_cmd, tp_type_cmd[1]*16);
		 TS_LOG_INFO("Will write the data to info_buff, offset is %s", offset2);
		 return retval;
	 }

	TS_LOG_INFO("%s barcode data is full, could not write into the data\n", __func__);
	tp_result_info[0] = TS_CHIP_NO_FLASH_ERROR;
	retval = -EINVAL;
	 return retval;
 }

static int synaptics_reconstruct_brightness(struct ts_oem_info_param *info)
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
		 TS_LOG_INFO("Will write the data to info_buff, offset is %s", offset1);
		 return retval;
	}

	type = info->buff[offset2*16 + 0];
	len = info->buff[offset2*16 + 1];
	if ((type == 0x00 && len == 0x00) ||(type == 0xFF && len == 0xFF)) {
		 memcpy(&(info->buff[offset2*16]), tp_type_cmd, tp_type_cmd[1]*16);
		 TS_LOG_INFO("Will write the data to info_buff, offset is %s", offset2);
		 return retval;
	 }

	 TS_LOG_INFO("%s brightness data is full, could not write into the data\n", __func__);
	 tp_result_info[0] = TS_CHIP_NO_FLASH_ERROR;
	 retval = -EINVAL;
	 return retval;
}

static int synaptics_reconstruct_whitepoint(struct ts_oem_info_param *info)
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
		TS_LOG_INFO("Will write the data to info_buff, offset is %s", offset1);
		return retval;
	}

	type = info->buff[offset2*16 + 0];
	len = info->buff[offset2*16 + 1];
	if ((type == 0x00 && len == 0x00) ||(type == 0xFF && len == 0xFF)) {
		memcpy(&(info->buff[offset2*16]), tp_type_cmd, tp_type_cmd[1]*16);
		TS_LOG_INFO("Will write the data to info_buff, offset is %s", offset2);
		return retval;
	}

	TS_LOG_INFO("%s white point is full, could not write into the data\n", __func__);
	tp_result_info[0] = TS_CHIP_NO_FLASH_ERROR;
	retval = -EINVAL;
	return retval;
 }

static int synaptics_reconstruct_brightness_whitepoint(struct ts_oem_info_param *info)
{
	 int retval = NO_ERR;
	 TS_LOG_INFO("%s No Flash defined in NV structure\n", __func__);
	 tp_result_info[0] = TS_CHIP_NO_FLASH_ERROR;
	 return retval;
}

static int synaptics_reconstruct_repair_recode(struct ts_oem_info_param *info)
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
			TS_LOG_INFO("Will write the data to info_buff, offset is %s", offset);
			break;
		} else if( offset == TS_NV_STRUCTURE_REPAIR_OFFSET5 ) {
			TS_LOG_INFO("%s repaire recode is full, could not write into the data\n", __func__);
			tp_result_info[0] = TS_CHIP_NO_FLASH_ERROR;
			retval = -EINVAL;
		}
	}
	return retval;
}

static int synaptics_reconstruct_NVstructure(struct ts_oem_info_param *info)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s called\n", __func__);
	TS_LOG_INFO("%s  info->data[0]:%2x\n", __func__, info->data[0]);

	TS_LOG_INFO("%s  itp_type_cmd[0]:%2x\n", __func__, tp_type_cmd[0]);
	switch (tp_type_cmd[0]) {
	case TS_NV_STRUCTURE_BAR_CODE:
		retval = synaptics_reconstruct_barcode(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, parade_reconstruct_barcode faild\n",
				   __func__);
		}
		break;
	case TS_NV_STRUCTURE_BRIGHTNESS:
		retval = synaptics_reconstruct_brightness(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, parade_reconstruct_barcode faild\n",
				   __func__);
		}
		break;
	case TS_NV_STRUCTURE_WHITE_POINT:
		retval = synaptics_reconstruct_whitepoint(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, parade_reconstruct_barcode faild\n",
				   __func__);
		}
		break;
	case TS_NV_STRUCTURE_BRI_WHITE:
		retval = synaptics_reconstruct_brightness_whitepoint(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, parade_reconstruct_barcode faild\n",
				   __func__);
		}
		break;
	case TS_NV_STRUCTURE_REPAIR:
		retval = synaptics_reconstruct_repair_recode(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, parade_reconstruct_barcode faild\n",
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

static int synaptics_get_NVstructure_cur_index(struct ts_oem_info_param *info, u8 type)
{
	int index = 0;
	int latest_index = 0;

	TS_LOG_INFO("%s find type:%u\n", __func__, type);
	for ((TS_NV_STRUCTURE_REPAIR == type)? (index = TS_NV_STRUCTURE_REPAIR_OFFSET1) : (index = 1);
		index < (TS_NV_STRUCTURE_REPAIR_OFFSET5+1); ++index) {
		u8 tmp_type = info->buff[index*16];
		u8 tmp_len = info->buff[index*16+1];
		if (tmp_type == type && ts_oemdata_type_check_legal(tmp_type, tmp_len)) {
			latest_index = index;
			TS_LOG_INFO("%s find latest index:%d\n", __func__, latest_index);
		}
	}

	return latest_index;
}

static int synaptics_get_NVstructure_index(struct ts_oem_info_param *info, u8 type)
{
	int index = 0;
	int latest_index = 0;
	int count = 0;

	for ((TS_NV_STRUCTURE_REPAIR == type)? (index = TS_NV_STRUCTURE_REPAIR_OFFSET1) : (index = 1);
		index < (TS_NV_STRUCTURE_REPAIR_OFFSET5+1); ++index) {
		u8 tmp_type = info->buff[index*16];
		u8 tmp_len = info->buff[index*16+1];
		if (tmp_type == type && ts_oemdata_type_check_legal(tmp_type, tmp_len)) {
			latest_index = index;
			count += 1;
		}
	}

	if(type == TS_NV_STRUCTURE_REPAIR) {
		info->length = count;
		if(info->length) {
			latest_index = TS_NV_STRUCTURE_REPAIR_OFFSET1;
		}
	}else{
		info->length = info->buff[latest_index*16+1];
	}

	return latest_index;
}


static int synaptics_set_oem_info(struct ts_oem_info_param *info)
{
	u8 type_reserved = TS_CHIP_TYPE_RESERVED;
	u8 len_reserved = TS_CHIP_TYPE_LEN_RESERVED;
	int store_type_count = 0;
	short flash_size = 0;
	int used_size = 16;
	int error = NO_ERR;
	int index = 0;
	u8 type = 0;
	u8 len = 0;
	int i = 0;

	TS_LOG_INFO("%s called\n", __func__);
	flash_size = synap_get_oem_data_info();
	if(flash_size < 0) {
		TS_LOG_ERR("%s: Could not get TPIC flash size,fail line=%d\n", __func__,
			   __LINE__);
		error = -EINVAL;
		goto out;
	}

	type = info->data[0];
	len  = info->data[1];
	used_size += len * 16;
	memset(tp_result_info, 0x0, TS_CHIP_TYPE_MAX_SIZE);
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
		memcpy(tp_type_cmd, info->data, MIN(sizeof(tp_type_cmd)-1, len*16));
	} else {
		error = EINVAL;
		tp_result_info[0] = TS_CHIP_WRITE_ERROR;
		TS_LOG_INFO("%s: invalid test cmd\n", __func__);
		return error;
	}

	error = synap_get_oem_data(info->buff, flash_size);
	if (error < 0) {
		TS_LOG_ERR("%s: get oem data failed,fail line=%d\n", __func__,
			   __LINE__);
		tp_result_info[0] = TS_CHIP_READ_ERROR;
		goto out;
	}

	TS_LOG_INFO("%s: Read data from TPIC flash is below\n", __func__);
	for(i = 0; i< flash_size/16; ++i ){
		TS_LOG_INFO("%s: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n", __func__,
			info->buff[0+i*16],info->buff[1+i*16],info->buff[2+i*16],info->buff[3+i*16],
			info->buff[4+i*16],info->buff[5+i*16],info->buff[6+i*16],info->buff[7+i*16],
			info->buff[8+i*16],info->buff[9+i*16],info->buff[10+i*16],info->buff[11+i*16],
			info->buff[12+i*16],info->buff[13+i*16],info->buff[14+i*16],info->buff[15+i*16]
		);
	}

	if( rmi4_data->synaptics_chip_data->is_new_oem_structure) {
		TS_LOG_INFO("%s: use new oem structure\n", __func__);
		if (NO_ERR != synaptics_reconstruct_NVstructure(info)){
			TS_LOG_ERR("%s: synaptics_reconstruct_NVstructure fail\n", __func__ );
			goto out;
		}
	}else{
		// Adding type data into the buffer
		for (index = 1; index < flash_size/16;) {
			u8 tmp_type = info->buff[index*16];
			u8 tmp_len = info->buff[index*16+1];
			if (tmp_len < len_reserved) {
				used_size += tmp_len * 16;
			}

			if (tmp_type == type) {
				store_type_count += 1;
			}

			if (store_type_count >= TS_CHIP_WRITE_MAX_TYPE_COUNT) {
					TS_LOG_ERR("%s: Write TPIC Type:0x%2x are larger than %d times\n", __func__, type, TS_CHIP_WRITE_MAX_TYPE_COUNT );
					tp_result_info[0] = TS_CHIP_MAX_COUNT_ERROR;
					goto out;
			}

			if (used_size > flash_size) {
				TS_LOG_ERR("%s: Used flash size is larger than TPIC Flash size\n", __func__ );
				tp_result_info[0] = TS_CHIP_NO_FLASH_ERROR;
				goto out;
			}

			if ((tmp_type == 0x00 && tmp_len == 0x00) ||(tmp_type == 0xFF && tmp_len == 0xFF)) {
				memcpy(&(info->buff[index*16]), tp_type_cmd, len*16);
				break;
			}
			index += tmp_len;
		}
	}

	TS_LOG_INFO("%s: Add write type data into buff below\n", __func__);
	for(i = 0; i< flash_size/16; ++i ){
		TS_LOG_INFO("%s: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n", __func__,
			info->buff[0+i*16],info->buff[1+i*16],info->buff[2+i*16],info->buff[3+i*16],
			info->buff[4+i*16],info->buff[5+i*16],info->buff[6+i*16],info->buff[7+i*16],
			info->buff[8+i*16],info->buff[9+i*16],info->buff[10+i*16],info->buff[11+i*16],
			info->buff[12+i*16],info->buff[13+i*16],info->buff[14+i*16],info->buff[15+i*16]
		);
	}

	//Write buffer into TPIC flash
	error = synap_set_oem_data(info->buff, flash_size);
	if (error < 0) {
		TS_LOG_ERR("%s: get oem data failed\n", __func__);
		tp_result_info[0] = TS_CHIP_WRITE_ERROR;
		goto out;
	}

	//check the write data.
	memset(info->buff, 0, TS_CHIP_BUFF_MAX_SIZE);
	error = synap_get_oem_data(info->buff, flash_size);
	if (error < 0) {
		TS_LOG_ERR("%s: get oem data failed,fail line=%d\n", __func__,
			   __LINE__);
		tp_result_info[0] = TS_CHIP_READ_ERROR;
		goto out;
	}
	if (rmi4_data->synaptics_chip_data->is_new_oem_structure){
		int latest_index = 0;
		latest_index = synaptics_get_NVstructure_cur_index(info, type);
		if (!latest_index){
			TS_LOG_ERR("%s: set oem data find current line fail line=%d\n", __func__,
				   __LINE__);
			tp_result_info[0] = TS_CHIP_WRITE_ERROR;
			goto out;
		}
		used_size =  latest_index * 16;

		TS_LOG_INFO("%s: CHECK:buff from TPIC\n", __func__);
		for(i = latest_index; i<(latest_index + len); ++i ){
			TS_LOG_INFO("%s: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n", __func__,
				info->buff[0+i*16],info->buff[1+i*16],info->buff[2+i*16],info->buff[3+i*16],
				info->buff[4+i*16],info->buff[5+i*16],info->buff[6+i*16],info->buff[7+i*16],
				info->buff[8+i*16],info->buff[9+i*16],info->buff[10+i*16],info->buff[11+i*16],
				info->buff[12+i*16],info->buff[13+i*16],info->buff[14+i*16],info->buff[15+i*16]
			);
		}
		TS_LOG_INFO("%s: CHECK:tp_type_cmd from PC\n", __func__);
		for(i = 0; i< len; ++i ){
			TS_LOG_INFO("%s: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n", __func__,
				tp_type_cmd[0+i*16],tp_type_cmd[1+i*16],tp_type_cmd[2+i*16],tp_type_cmd[3+i*16],
				tp_type_cmd[4+i*16],tp_type_cmd[5+i*16],tp_type_cmd[6+i*16],tp_type_cmd[7+i*16],
				tp_type_cmd[8+i*16],tp_type_cmd[9+i*16],tp_type_cmd[10+i*16],tp_type_cmd[11+i*16],
				tp_type_cmd[12+i*16],tp_type_cmd[13+i*16],tp_type_cmd[14+i*16],tp_type_cmd[15+i*16]
			);
		}
	}else{
		used_size -= len * 16;
	}

	error = strncmp(&(info->buff[used_size]), tp_type_cmd, len*16);
	if( error ) {
		tp_result_info[0] = TS_CHIP_WRITE_ERROR;
		TS_LOG_ERR("%s: Write type data has some error\n", __func__);
	}

out:
	memset(tp_type_cmd, 0x0, TS_CHIP_TYPE_MAX_SIZE);
	TS_LOG_INFO("%s End\n", __func__);
	return error;

}

static int synaptics_get_oem_info(struct ts_oem_info_param *info)
{
	u8 type_reserved = TS_CHIP_TYPE_RESERVED;
	u8 type = tp_type_cmd[0];
	short flash_size = 0;
	int error = NO_ERR;
	int index =0;
	int latest_index = 0;
	int i = 0;
	int infolength = 0;

	TS_LOG_INFO("%s called\n", __func__);
	memset(info->data, 0x0, TS_CHIP_TYPE_MAX_SIZE);
	flash_size = synap_get_oem_data_info();
	if(flash_size < 0) {
		TS_LOG_ERR("%s: Could not get TPIC flash size,fail line=%d\n", __func__,
			   __LINE__);
		error = -EINVAL;
		goto out;
	}
	if(0 == rmi4_data->synaptics_chip_data->support_2dbarcode_info) {
		//return the result info if type is 0x0
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
	}
	if(rmi4_data->synaptics_chip_data->support_2dbarcode_info){
		flash_size = SYNAPTICS_RMI4_BARCODE_INFO_SIZE;
		error = synap_get_oem_data(info->buff, SYNAPTICS_RMI4_BARCODE_INFO_LEN);
	}
	else
		error = synap_get_oem_data(info->buff, flash_size);
	if (error < 0) {
		TS_LOG_ERR("%s: memory not enough,fail line=%d\n", __func__,
			   __LINE__);
		error = EINVAL;
		goto out;
	}

	TS_LOG_INFO("%s:Get buff data below\n", __func__);
	for(i = 0; i< flash_size/16; ++i ){
		TS_LOG_INFO("%s: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n", __func__,
			info->buff[0+i*16],info->buff[1+i*16],info->buff[2+i*16],info->buff[3+i*16],
			info->buff[4+i*16],info->buff[5+i*16],info->buff[6+i*16],info->buff[7+i*16],
			info->buff[8+i*16],info->buff[9+i*16],info->buff[10+i*16],info->buff[11+i*16],
			info->buff[12+i*16],info->buff[13+i*16],info->buff[14+i*16],info->buff[15+i*16]
		);
		TS_LOG_DEBUG("%s: %c %c %c %c %c %c %c %c %c %c %c %c %c %c %c %c \n", __func__,
			info->buff[0+i*16],info->buff[1+i*16],info->buff[2+i*16],info->buff[3+i*16],
			info->buff[4+i*16],info->buff[5+i*16],info->buff[6+i*16],info->buff[7+i*16],
			info->buff[8+i*16],info->buff[9+i*16],info->buff[10+i*16],info->buff[11+i*16],
			info->buff[12+i*16],info->buff[13+i*16],info->buff[14+i*16],info->buff[15+i*16]
		);
	}
	if(rmi4_data->synaptics_chip_data->is_new_oem_structure) {
		TS_LOG_INFO("%s: use new oem structure\n", __func__);
		//scan each NV struct length
		latest_index = synaptics_get_NVstructure_index(info, type);
		TS_LOG_INFO("%s get type:0x%2x  index = %d\n", __func__, type, latest_index);

		if (latest_index) {
			TS_LOG_INFO("%s type data find. len = %d\n", __func__, info->length);
			infolength = min(TS_CHIP_TYPE_MAX_SIZE,info->length*16+1);
			memcpy(info->data, &(info->buff[latest_index*16]), infolength-1);
		} else {
			info->data[0] = 0x1;
			TS_LOG_INFO("%s No type data find. info->data[0] = %2x\n", __func__, info->data[0]);
		}
	}else if(rmi4_data->synaptics_chip_data->support_2dbarcode_info){
		TS_LOG_INFO("[%s] ->2d bar code to info->data \n",__func__);
		memcpy(info->data, info->buff, SYNAPTICS_RMI4_BARCODE_INFO_SIZE);
	}else{
		//scan each NV struct length
		for (index = 1; index < flash_size/16;) {
			u8 tmp_type = info->buff[index*16];
			u8 tmp_len = info->buff[index*16+1];
			if (tmp_type == type) {
				latest_index = index;
			} else if ((tmp_type == 0 && tmp_len == 0)||(tmp_type == 0xFF && tmp_len == 0xFF) ) {
				break;
			}
			index += tmp_len;
		}

		if (latest_index) {
			TS_LOG_INFO("%s type data find. len = %d\n", __func__, info->buff[latest_index*16+1]*16);
			infolength = min(TS_CHIP_TYPE_MAX_SIZE,info->buff[latest_index*16+1]*16+1);
			memcpy(info->data, &(info->buff[latest_index*16]), infolength-1);
		} else {
			info->data[0] = 0x1;
			TS_LOG_INFO("%s No type data find. info->data[0] = %2x\n", __func__, info->data[0]);
		}
	}
out:
	TS_LOG_INFO("%s End\n", __func__);
	return error;
}

static int synaptics_oem_info_switch(struct ts_oem_info_param *info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR("synaptics_oem_info_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		retval = synaptics_set_oem_info(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, synaptics_oem_info_switch faild\n",
				   __func__);
		}
		break;
	case TS_ACTION_READ:
		retval = synaptics_get_oem_info(info);
		if (retval != 0) {
			TS_LOG_ERR("%s, synaptics_get_oem_info faild\n",
				   __func__);
		}
		break;
	default:
		TS_LOG_INFO("invalid oem info switch(%d) action: %d\n",
				info->data_switch, info->op_action);
		retval = -EINVAL;
		break;
	}
	return retval;
}
static int synaptics_chip_get_info(struct ts_chip_info_param *info)
{
	int retval = 0;
	u8 buf[CHIP_INFO_LENGTH] = { 0 };
	unsigned char string_id_buf[CHIP_INFO_LENGTH * 2] = { 0 };
	int projectid_lenth = 0;

	if (rmi4_data->synaptics_chip_data->projectid_len) {
		projectid_lenth = rmi4_data->synaptics_chip_data->projectid_len;
	} else {
		projectid_lenth = PROJECT_ID_FW_LEN;
	}

	memset(buf, 0, sizeof(buf));
	TS_LOG_INFO("rmi4_data->synaptics_chip_data->ts_platform_data->get_info_flag=%d\n",
				rmi4_data->synaptics_chip_data->ts_platform_data->get_info_flag);
	if (unlikely
	    ((atomic_read(&rmi4_data->synaptics_chip_data->ts_platform_data->state) == TS_SLEEP)
	     || (atomic_read(&rmi4_data->synaptics_chip_data->ts_platform_data->state) == TS_WORK_IN_SLEEP))) {

		strncpy(buf, config_id_string, strlen(config_id_string));
	} else {
		if (!rmi4_data->synaptics_chip_data->ts_platform_data->get_info_flag) {
			if (SYNAPTICS_S3718 != rmi4_data->synaptics_chip_data->ic_type
				&& SYNAPTICS_TD4322 != rmi4_data->synaptics_chip_data->ic_type
				&& SYNAPTICS_TD4310 != rmi4_data->synaptics_chip_data->ic_type
				&& SYNAPTICS_S3706 != rmi4_data->synaptics_chip_data->ic_type){
				retval =
				    synap_fw_configid(rmi4_data, buf,
							  sizeof(buf));
				if (retval < 0)
					TS_LOG_ERR("failed to get configid\n");
					retval = synap_fw_data_init(rmi4_data);
					if (retval < 0) {
						TS_LOG_ERR("synap_fw_data_init error.\n");
					}
					synap_fw_data_release();
			} else {
				retval =
				    synap_fw_s3718_configid(rmi4_data, buf,
								sizeof(buf));
				if (retval < 0)
					TS_LOG_ERR("failed to get configid\n");
					retval = synap_fw_data_s3718_init(rmi4_data);
					if (retval < 0) {
						TS_LOG_ERR("synap_fw_data_s3718_init error.\n");
					}
					synap_fw_data_s3718_release();
					synaptics_rmi4_status_resume(rmi4_data);
			}
		}
	}

	if(NULL != synaptics_sett_param_regs->module_name)
	{
		memcpy(&info->mod_vendor, synaptics_sett_param_regs->module_name,
			    MIN(sizeof(info->mod_vendor), strlen(synaptics_sett_param_regs->module_name)+1));
	}
	memcpy(&info->fw_vendor, buf, strlen(buf));

	if (rmi4_data->synaptics_chip_data->ts_platform_data->hide_plain_id) {
		memcpy(&string_id_buf, rmi4_data->rmi4_mod_info.project_id_string,
		       strlen(rmi4_data->rmi4_mod_info.project_id_string));
		memcpy(&info->ic_vendor, string_id_buf, projectid_lenth);
	} else {
		memcpy(&string_id_buf, SYNAPTICS_CHIP_INFO,
		       strlen(SYNAPTICS_CHIP_INFO));
		strncat(string_id_buf, rmi4_data->rmi4_mod_info.project_id_string,
				projectid_lenth);
		memcpy(&info->ic_vendor, string_id_buf, strlen(string_id_buf));
	}

	return NO_ERR;
}

int create_and_get_u16_array(u16 *array, struct device_node *dev_node, const char *name)
{
	const __be32 *values = NULL;
	int len = 0;
	int sz = 0;
	int i = 0;

	if(NULL == array || NULL == dev_node || NULL == name) {
		TS_LOG_ERR("%s: chip_data or dev_node or name is NULL\n", __func__);
		goto fail;
	}

	values = of_get_property(dev_node, name, &len);
	if (NULL == values) {
		TS_LOG_ERR("%s: of_get_property get values is NULL\n", __func__);
		goto fail;
	}

	sz = len / (int)sizeof(u32);
	if(sz != TP_3320_SHORT_ARRAY_NUM) {
		TS_LOG_ERR("%s: size not match TP_3320_SHORT_ARRAY_NUM, err\n", __func__);
		goto fail;
	}

	for (i = 0; i < sz; i++) {
		array[i] = (u16)be32_to_cpup(values++);
	}

	return NO_ERR;

fail:
	return -ENODEV;
}

static void synap_parse_chip_hybrid_specific_dts(struct device_node *device)
{
	int retval = 0;

	if(NULL == device) {
		TS_LOG_ERR("%s: chip_data or device is NULL\n", __func__);
		return;
	}

	retval = of_property_read_u32(device, NOT_DELAY_ACT, &rmi4_data->delay_for_fw_update);
	if (retval) {
		TS_LOG_INFO("%s: delay_for_fw_update has NOT been set\n", __func__);
	}
	TS_LOG_INFO("%s: delay_for_fw_update = %d\n", __func__, rmi4_data->delay_for_fw_update);

	retval = of_property_read_u32(device, SUPPORT_SHORT_TEST, (u32 *)&rmi4_data->support_s3320_short_test);
	if (retval) {
		TS_LOG_ERR("%s: support_s3320_short_test has NOT been set\n", __func__);
		rmi4_data->support_s3320_short_test = false;
		return;
	}
	TS_LOG_INFO("%s: support_s3320_short_test = %d\n", __func__, rmi4_data->support_s3320_short_test);

	retval = create_and_get_u16_array(rmi4_data->trx_short_circuit_array, device, TRX_SHORT_CIRCUIT);
	if(retval < 0) {
		TS_LOG_ERR("%s: create_and_get_u16_array err\n", __func__);
	}
	return;
}

static void synap_parse_crc_err_reset(struct device_node *device)
{
	int retval = 0;

	if( NULL == device) {
		TS_LOG_ERR("%s: chip_data  is NULL\n", __func__);
		return;
	}

	retval = of_property_read_u32(device, CRC_ERR_DO_RESET, &rmi4_data->support_crc_err_do_reset);
	if(retval < 0) {
		TS_LOG_ERR("%s: get crc_err_do_reset err, set default 0\n", __func__);
		rmi4_data->support_crc_err_do_reset = 0;
	}
	TS_LOG_INFO("%s: set crc_err_do_reset: %d\n", __func__, rmi4_data->support_crc_err_do_reset);

	return;
}

/*  query the configure from dts and store in prv_data */
void  synap_parse_chip_specific_dts(struct ts_kit_device_data *chip_data)
{

	struct device_node *device = NULL;
	unsigned char string_id_buf[CHIP_INFO_LENGTH * 2] = { 0 };
	int retval = 0;
	int synaptics_rawdata_count = 0;
	char *producer = NULL;
	int projectid_lenth = 0;
	char *adv_width = NULL;

	if(NULL == chip_data) {
		TS_LOG_ERR("%s: chip_data is NULL, err!\n",__func__);
		return;
	}
	if (rmi4_data->synaptics_chip_data->projectid_len) {
		projectid_lenth = rmi4_data->synaptics_chip_data->projectid_len;
	} else {
		projectid_lenth = PROJECT_ID_FW_LEN;
	}

	rmi4_data->module_name = NULL;

	memcpy(&string_id_buf, SYNAPTICS_CHIP_INFO,
	       strlen(SYNAPTICS_CHIP_INFO));

	TS_LOG_INFO("project_id_string = %s\n",
		    rmi4_data->rmi4_mod_info.project_id_string);
	TS_LOG_INFO("product_id_string = %s, projectid_lenth=%d\n",
		    rmi4_data->rmi4_mod_info.product_id_string, projectid_lenth);
	strncat(string_id_buf, rmi4_data->rmi4_mod_info.project_id_string,
		projectid_lenth);

	TS_LOG_INFO("try to get chip specific dts: %s\n", string_id_buf);

	device = of_find_compatible_node(NULL, NULL, string_id_buf);
	if (!device) {
		TS_LOG_INFO("No chip specific dts: %s, need to parse\n",
			    string_id_buf);
		return;
	}
	ts_parse_panel_specific_config(device, chip_data);
	rmi4_data->sensor_max_x = chip_data->x_max - 1;
	rmi4_data->sensor_max_y = chip_data->y_max - 1;
	rmi4_data->sensor_max_x_mt = chip_data->x_max - 1;
	rmi4_data->sensor_max_y_mt = chip_data->y_max - 1;

	retval = of_property_read_string(device, "producer", (const char **)&producer);
	if (NULL != producer) {
		TS_LOG_INFO("producer = %s\n", producer);
		rmi4_data->module_name = producer;
	}
	else{
		TS_LOG_ERR("fail parse TP producer \n");
	}

	retval = of_property_read_u32(device, SUPPORT_EXT_TREX_SHORT_TEST, &rmi4_data->support_ext_trex_short_test);
	if (retval) {
		TS_LOG_ERR("%s: support_ext_trex_short_test has NOT been set\n", __func__);
		rmi4_data->support_ext_trex_short_test = 0;
	}
	TS_LOG_INFO("%s: support_ext_trex_short_test = %d\n", __func__, rmi4_data->support_ext_trex_short_test);

	retval = of_property_read_u32(device, "screenoff_status_reg", &rmi4_data->screenoff_status_reg);
	if (retval) {
		TS_LOG_ERR("%s: screenoff_status_reg has NOT been set\n", __func__);
		rmi4_data->screenoff_status_reg = 0;
	}
	TS_LOG_INFO("%s: screenoff_status_reg = %d\n", __func__, rmi4_data->screenoff_status_reg);

	retval = of_property_read_u32(device, "screenoff_status_switch", &rmi4_data->screenoff_status_switch);
	if (retval) {
		TS_LOG_ERR("%s: screenoff_status_switch has NOT been set\n", __func__);
		rmi4_data->screenoff_status_switch = 0;
	}
	TS_LOG_INFO("%s: screenoff_status_switch = %d\n", __func__, rmi4_data->screenoff_status_switch);

	retval = of_property_read_u32(device, "screenoff_status_support", &rmi4_data->screenoff_status_support);
	if (retval) {
		TS_LOG_ERR("%s: screenoff_status_support has NOT been set\n", __func__);
		rmi4_data->screenoff_status_support = 0;
	}
	TS_LOG_INFO("%s: screenoff_status_support = %d\n", __func__, rmi4_data->screenoff_status_support);

	retval = of_property_read_u32(device, "sensitivity_adjust_support", &rmi4_data->sensitivity_adjust_support);
	if (retval) {
		TS_LOG_ERR("%s: sensitivity_adjust_support has NOT been set\n", __func__);
		rmi4_data->sensitivity_adjust_support = 0;
	}
	TS_LOG_INFO("%s: sensitivity_adjust_support = %d\n", __func__, rmi4_data->sensitivity_adjust_support);

	retval = of_property_read_u32(device, "sensitivity_adjust_reg", &rmi4_data->sensitivity_adjust_reg);
	if (retval) {
		TS_LOG_ERR("%s: sensitivity_adjust_reg has NOT been set\n", __func__);
		rmi4_data->sensitivity_adjust_reg = 0;
	}
	TS_LOG_INFO("%s: sensitivity_adjust_reg = %d\n", __func__, rmi4_data->sensitivity_adjust_reg);


	retval = of_property_read_u32(device, EXT_TREX_SHORT_TEST_WAIT_REPORT_DELAY_FLAG, \
							&rmi4_data->ext_trex_short_wait_report_delay_flag);
	if (retval) {
		TS_LOG_ERR("%s: ext_trex_short_wait_report_delay_flag has NOT been set\n", __func__);
		rmi4_data->ext_trex_short_wait_report_delay_flag = 0;
	}
	TS_LOG_INFO("%s: ext_trex_short_wait_report_delay_flag = %d\n", __func__, \
				rmi4_data->ext_trex_short_wait_report_delay_flag);

	retval = of_property_read_u32(device, TEST_ENHANCE_RAW_DATA_CAPACITANCE, &rmi4_data->test_enhance_raw_data_capacitance);
	if (retval) {
		TS_LOG_INFO("get device test_enhance_raw_data_capacitance null, use default\n");
		rmi4_data->test_enhance_raw_data_capacitance = 0;
		retval = 0;
	} else{
		synaptics_rawdata_count = rmi4_data->num_of_rx * rmi4_data->num_of_tx;
		TS_LOG_INFO("synaptics_rawdata_count= %d\n", synaptics_rawdata_count);
		rmi4_data->upper = (int *)kzalloc(synaptics_rawdata_count * sizeof(int), GFP_KERNEL);
		rmi4_data->lower = (int *)kzalloc(synaptics_rawdata_count * sizeof(int), GFP_KERNEL);
		if(!rmi4_data->upper || !rmi4_data->lower) {
			TS_LOG_ERR("Memory was out\n");
			if(rmi4_data->upper) {
				kfree(rmi4_data->upper);
				rmi4_data->upper = NULL;
			}
			if(rmi4_data->lower) {
				kfree(rmi4_data->lower);
				rmi4_data->lower = NULL;
			}
			rmi4_data->test_enhance_raw_data_capacitance = 0;
		} else {
			retval = of_property_read_u32_array(device, "huawei,enhance_rawdata_upperlimit", rmi4_data->upper, synaptics_rawdata_count);
			if(retval) {
				TS_LOG_ERR("get device test_enhance_raw_data error\n");
				memset(rmi4_data->upper, 1, synaptics_rawdata_count*sizeof(int));
				retval = 0;
			}
			retval = of_property_read_u32_array(device, "huawei,enhance_rawdata_lowerlimit", rmi4_data->lower, synaptics_rawdata_count);
			if(retval){
				TS_LOG_ERR("get device test_enhance_raw_data error\n");
				memset(rmi4_data->lower, 0, synaptics_rawdata_count*sizeof(int));
				retval = 0;
			}
		}
	}
	/* syna_wx_wy */
	retval = of_property_read_string(device, "adv_width", (const char **)&adv_width);
	if (retval || !adv_width) {
		TS_LOG_INFO("get device adv_width not exit,use default value\n");
		snprintf(rmi4_data->adv_width, 4, "FFF");
	}else{
		snprintf(rmi4_data->adv_width, 4, "%s", adv_width);
	}
	TS_LOG_INFO("adv_width[0]=%c, adv_width[1]=%c, adv_width[2]=%c\n",
		rmi4_data->adv_width[0], rmi4_data->adv_width[1], rmi4_data->adv_width[2]);

	TS_LOG_INFO("test_enhance_raw_data_capacitance=%d\n", rmi4_data->test_enhance_raw_data_capacitance);

	synap_parse_chip_hybrid_specific_dts(device);
	synap_parse_crc_err_reset(device);
	return;
}


static int synaptics_parse_hover_config(struct device_node *device)
{
	int retval = NO_ERR;
	int value = 0;
	retval = of_property_read_u32(device,"need_disable_hover", &value);
	if (retval) {
		rmi4_data->need_disable_hover = 0;
		TS_LOG_INFO("no need control hover.");
		goto out ;
	}else{
		rmi4_data->need_disable_hover = value;
	}
	retval = of_property_read_u32(device, "hover_switch_addr", &value);
	if (retval) {
		rmi4_data->hover_switch_addr = 0;
		rmi4_data->need_disable_hover = 0;
		goto out ;
	} else {
		rmi4_data->hover_switch_addr = value;
	}
	retval = of_property_read_u32(device, "hover_switch_bit", &value);
	if (retval) {
		rmi4_data->hover_switch_bit= 0;
		rmi4_data->need_disable_hover = 0;
		goto out ;
	}else{
		rmi4_data->hover_switch_bit = value;
	}

out:
	TS_LOG_INFO("need_disable_hover =%d,hover_switch_addr = 0x%d,hover_switch_bit =%d.\n",
		rmi4_data->need_disable_hover ,rmi4_data->hover_switch_addr, rmi4_data->hover_switch_bit);
	return NO_ERR;
}


/*  query the configure from dts and store in prv_data */
static int synaptics_private_config_parse(struct device_node *device,
					struct synaptics_rmi4_data *rmi4_data)
{
	int retval = NO_ERR;
	int value = 0;

	synaptics_parse_hover_config(device);

	retval = of_property_read_u32(device, SYNA_3718_FW_UPDATA_FLAG, &value);
	if (retval) {
		rmi4_data->synaptics3718_fw_updata_flag = 0;
		TS_LOG_ERR("Not define synaptics3718_fw_updata_flag in Dts, use fault value\n");
	}else{
		rmi4_data->synaptics3718_fw_updata_flag = value;
		TS_LOG_INFO("get device synaptics3718_fw_updata_flag =%d,\n",rmi4_data->synaptics3718_fw_updata_flag );
	}

	retval = of_property_read_u32(device, SYNA_3718_TP_PRESSURE_FLAG, &value);
	if (retval) {
		rmi4_data->synaptics3718_Tp_Pressure_flag= 0;
		TS_LOG_ERR("Not define synaptics3718_Tp_Pressure_flag in Dts, use fault value\n");
	}else{
		rmi4_data->synaptics3718_Tp_Pressure_flag = value;
		TS_LOG_INFO("get device synaptics3718_Tp_Pressure_flag =%d,\n",rmi4_data->synaptics3718_Tp_Pressure_flag );
	}
	retval = of_property_read_u32(device, "bootloader_update_enable",
				 &rmi4_data->bootloader_update_enable);
	if (retval) {
		rmi4_data->bootloader_update_enable = 0;
		TS_LOG_INFO("get device bootloader_update_enable fail\n");
	}

	retval = of_property_read_u32(device, "is_multi_protocal", &rmi4_data->is_multi_protocal);
	if (!retval) {
		TS_LOG_INFO("get chip is_multi_protocal = %d\n", rmi4_data->is_multi_protocal);
	} else {
		rmi4_data->is_multi_protocal = 0;
		TS_LOG_INFO("can not get is_multi_protocal, use default:%d\n", rmi4_data->is_multi_protocal);
	}

	retval = of_property_read_u32(device, "delay_for_fw_update",
				 &rmi4_data->delay_for_fw_update);
	if (retval) {
		rmi4_data->delay_for_fw_update = 0;
		TS_LOG_INFO("device delay_for_fw_update not found.\n");
	}

	retval = of_property_read_u32(device, "byte_to_byte_cmp_config_id",
				 &rmi4_data->byte_to_byte_cmp_config_id);
	if (retval) {
		rmi4_data->byte_to_byte_cmp_config_id = 0;
		TS_LOG_INFO("device byte_to_byte_cmp_config_id not found.\n");
	}
	retval = of_property_read_u32(device, "use_ub_supported",
				 &rmi4_data->use_ub_supported);
	if (retval) {
		rmi4_data->use_ub_supported = 0;
		TS_LOG_INFO("device use_ub_supported not exit,use default value.\n");
	}

	retval = of_property_read_u32(device, "delay_for_erase_fw",
				 &rmi4_data->delay_for_erase_fw);
	if (retval) {
		rmi4_data->delay_for_erase_fw = 800;
		TS_LOG_INFO("device delay_for_erase_fw not found,use default value.\n");
	}

	retval = of_property_read_u32(device, "distinguish_ic_type",
				 &rmi4_data->distinguish_ic_type);
	if (retval) {
		rmi4_data->distinguish_ic_type = 0;
		TS_LOG_INFO("device distinguish_ic_type not found,use default value.\n");
	}

	return 0;
}

static void synaptics_power_on_gpio_set(void)
{
	synaptics_pinctrl_select_normal();
	gpio_direction_input(rmi4_data->synaptics_chip_data->ts_platform_data->irq_gpio);
	if (SYNAPTICS_S3207 != rmi4_data->synaptics_chip_data->ic_type
		&& rmi4_data->synaptics_chip_data->ts_platform_data->reset_gpio)
		gpio_direction_output(rmi4_data->synaptics_chip_data->ts_platform_data->reset_gpio, 1);
}


static void synaptics_power_off_gpio_set(void)
{
	TS_LOG_INFO("suspend RST out L\n");
	if ((SYNAPTICS_S3718 == rmi4_data->synaptics_chip_data->ic_type
		|| SYNAPTICS_TD4322 == rmi4_data->synaptics_chip_data->ic_type
		|| SYNAPTICS_TD4310== rmi4_data->synaptics_chip_data->ic_type
		|| SYNAPTICS_S3706== rmi4_data->synaptics_chip_data->ic_type)
		&& rmi4_data->synaptics_chip_data->ts_platform_data->reset_gpio)
		gpio_direction_output(rmi4_data->synaptics_chip_data->ts_platform_data->reset_gpio, 0);
	synaptics_pinctrl_select_lowpower();
	if (SYNAPTICS_S3718 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_TD4322 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_TD4310 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_S3706  != rmi4_data->synaptics_chip_data->ic_type
		&& rmi4_data->synaptics_chip_data->ts_platform_data->reset_gpio)
		gpio_direction_input(rmi4_data->synaptics_chip_data->ts_platform_data->reset_gpio);
	mdelay(1);
}


static void synaptics_gpio_reset(void)
{
	TS_LOG_DEBUG("synaptics_gpio_reset\n");
	if (!rmi4_data->synaptics_chip_data->ts_platform_data->reset_gpio) {
		TS_LOG_DEBUG("reset_gpio is null, not supported reset\n");
		return;
	}
	gpio_direction_output(rmi4_data->synaptics_chip_data->ts_platform_data->reset_gpio, 1);
	mdelay(1);
	gpio_direction_output(rmi4_data->synaptics_chip_data->ts_platform_data->reset_gpio, 0);
	udelay(300);
	gpio_direction_output(rmi4_data->synaptics_chip_data->ts_platform_data->reset_gpio, 1);
	mdelay(1);
}

static int syna_tcm_i2c_read(unsigned char *data,unsigned int length)
{
	int retval = 0;
	unsigned int attempt = 0;
	struct i2c_msg msg = {0};
	struct i2c_client *i2c = rmi4_data->synaptics_chip_data->ts_platform_data->client;

	msg.addr = i2c->addr;
	msg.flags = I2C_M_RD;
	msg.len = length;
	msg.buf = data;

	for (attempt = 0; attempt < I2C_RW_TRIES; attempt++) {
		if (i2c_transfer(i2c->adapter, &msg, 1) == 1) {
			retval = length;
			goto exit;
		}
		TS_LOG_ERR(
				"Transfer attempt %d failed\n",
				attempt + 1);

		if (attempt + 1 == I2C_RW_TRIES) {
			retval = -EIO;
			goto exit;
		}

		msleep(20);
	}

exit:
	return retval;
}

static int syna_tcm_comm_check(void)
{
	int retval = NO_ERR;
	unsigned char marker = 0;

	retval = syna_tcm_i2c_read(&marker,1);
	TS_LOG_INFO("check marker:%d\n", marker);
	if (retval < 0 || marker != COMM2_MESSAGE_MARKER) {
		TS_LOG_INFO("not find comm2 protocol, try rmi4\n");
		return RESULT_ERR;
	}
	return NO_ERR;
}

static int i2c_communicate_check(void)
{
	int retval = NO_ERR;
	int i = 0;
	u8 pdt_entry_addr = PDT_START;
	struct synaptics_rmi4_fn_desc rmi_fd;
	memset(&rmi_fd, 0, sizeof(rmi_fd));

	if(rmi4_data->distinguish_ic_type) {
		if (!(syna_tcm_comm_check())) {
			return RESULT_ERR;
		}
	}


	for (i = 0; i < I2C_RW_TRIES; i++) {
		retval = synaptics_rmi4_i2c_read(rmi4_data, pdt_entry_addr,
						 (unsigned char *)&rmi_fd,
						 sizeof(rmi_fd));
		if (retval < 0) {
			TS_LOG_ERR
			    ("Failed to read register map, i = %d, retval = %d\n",
			     i, retval);
			msleep(50);
		} else {
			TS_LOG_INFO("i2c communicate check success\n");
			return NO_ERR;
		}
	}

	return retval;
}
static int synaptics_power_init(void)
{
	ts_kit_power_supply_get(TS_KIT_IOVDD);
	ts_kit_power_supply_get(TS_KIT_VCC);
	return 0;
}

static int synaptics_power_release(void)
{
	ts_kit_power_supply_put(TS_KIT_IOVDD);
	ts_kit_power_supply_put(TS_KIT_VCC);
	return 0;
}

static void synaptics_power_on(void)
{
	TS_LOG_INFO("synaptics_power_on called\n");
	ts_kit_power_supply_ctrl(TS_KIT_VCC, TS_KIT_POWER_ON, 5);
	ts_kit_power_supply_ctrl(TS_KIT_IOVDD, TS_KIT_POWER_ON, 5);
	synaptics_power_on_gpio_set();
}
static void synaptics_power_off(void)
{
	synaptics_power_off_gpio_set();
	ts_kit_power_supply_ctrl(TS_KIT_VCC, TS_KIT_POWER_OFF, 12);
	ts_kit_power_supply_ctrl(TS_KIT_IOVDD, TS_KIT_POWER_OFF, 30);
}

static int synaptics_chip_detect(struct ts_kit_platform_data *data)
{
	int retval = NO_ERR;
	int isbulcked = 0;

	TS_LOG_INFO("synaptics chip detect called\n");
	if ((!data) || (!data->ts_dev) ){
		TS_LOG_ERR("device, ts_kit_platform_data *data or data->ts_dev is NULL \n");
		retval =  -EINVAL;
		goto out;
	}
	rmi4_data->synaptics_chip_data->ts_platform_data = data;
	rmi4_data->synaptics_dev = data->ts_dev;
	rmi4_data->synaptics_dev->dev.of_node = rmi4_data->synaptics_chip_data->cnode;
	rmi4_data->current_page = MASK_8BIT;
	/*setting the default data*/
	rmi4_data->synaptics_chip_data->is_i2c_one_byte = 0;
	rmi4_data->synaptics_chip_data->is_new_oem_structure= 0;
	rmi4_data->synaptics_chip_data->is_parade_solution= 0;

	synaptics_private_config_parse(rmi4_data->synaptics_dev->dev.of_node, rmi4_data);

	if(true == rmi4_data->synaptics_chip_data->check_bulcked){
		if(ts_kit_get_pt_station_status(&isbulcked) || false == isbulcked){
			TS_LOG_ERR("%s, no lcd buckled\n", __func__);
			retval = -EFAULT;
			goto out;
		}
	}

	retval = synaptics_pinctrl_get_init();
	if (retval < 0) {
		TS_LOG_ERR("synaptics_pinctrl_get_init error %d \n",retval);
		goto pinctrl_get_err;
	}

	retval = synaptics_power_init();
	if (retval < 0) {
		TS_LOG_ERR("synaptics_regulator_get error %d \n",retval);
		goto out;
	}

	if(rmi4_data->synaptics_chip_data->vddio_default_on){
		ts_kit_power_supply_ctrl(TS_KIT_IOVDD, TS_KIT_POWER_ON, 0);
		ts_kit_power_supply_ctrl(TS_KIT_IOVDD, TS_KIT_POWER_OFF, 15);
	}

	/*power up the chip */
	synaptics_power_on();

	retval = of_property_read_u32(rmi4_data->synaptics_chip_data->cnode , SYNAPTCS_SLAVE_ADDR,
					(u32 *)&rmi4_data->synaptics_chip_data->ts_platform_data->client->addr);
	if (retval) {
		rmi4_data->synaptics_chip_data->ts_platform_data->client->addr = SYNAPTIC_DEFAULT_I2C_ADDR;
		TS_LOG_INFO("not set %s in dts, use default.\n", SYNAPTCS_SLAVE_ADDR);
	}
	TS_LOG_INFO("synaptics slave_addr= %d.\n", rmi4_data->synaptics_chip_data->ts_platform_data->client->addr);
	/*reset the chip */
	synaptics_gpio_reset();
	TS_LOG_INFO("chip has been reset\n");
	/*for 3350/3320after 100ms, allowed i2c operation, for 3207, time need 250ms */
	msleep(250);

	retval = i2c_communicate_check();
	if (retval < 0) {
		TS_LOG_ERR("not find synaptics device\n");
		//rmi4_data->synaptics_chip_data->ts_platform_data = NULL;
		goto check_err;
	} else {
		TS_LOG_INFO("find synaptics device\n");
		strncpy(rmi4_data->synaptics_chip_data->chip_name, SYNAPTICS_VENDER_NAME,
				(MAX_STR_LEN > strlen(SYNAPTICS_VENDER_NAME)+1) ?
				strlen(SYNAPTICS_VENDER_NAME)+1 :
				(MAX_STR_LEN -1));
	}
	/*lint -esym(527,init_completion)*/
	init_completion(&roi_data_done);
	/*lint +esym(527,init_completion)*/
	TS_LOG_INFO("synaptics chip detect successful\n");
	return NO_ERR;

check_err:
	synaptics_power_off();
	synaptics_power_release();
pinctrl_get_err:
out:
	if(rmi4_data->synaptics_chip_data) {
		kfree(rmi4_data->synaptics_chip_data);
		rmi4_data->synaptics_chip_data = NULL;
	}
	if (rmi4_data) {
		kfree(rmi4_data);
		rmi4_data = NULL;
	}
	TS_LOG_ERR("detect synaptics error\n");
	return retval;
}

/*    init the chip.
*
*     (1) power up;  (2) detect the chip thourgh bus(i2c).
*/
static int synaptics_wrong_touch(void)
{
	int rc = NO_ERR;
	mutex_lock(&wrong_touch_lock);
	rmi4_data->synaptics_chip_data->easy_wakeup_info.off_motion_on = true;
	mutex_unlock(&wrong_touch_lock);
	TS_LOG_INFO("done\n");
	return rc;
}

static int synaptics_init_fwu_info(void)
{
	int retval = 0;

	retval = synap_fw_data_s3718_init(rmi4_data);
	if (retval) {
		TS_LOG_ERR("%s, failed\n", __func__);
	}
	synap_fw_data_s3718_release();
	return retval;
}

static int synaptics_init_chip(void)
{
	int rc = NO_ERR;

    synaptics_sett_param_regs = kzalloc(sizeof(struct touch_settings), GFP_KERNEL);
	if(!synaptics_sett_param_regs)
	{
	     TS_LOG_ERR("Failed to malloc to synaptics_sett_param_regs\n");
	    return -ENOMEM;
	}

	if (rmi4_data->delay_for_fw_update) {
		rmi4_data->reset_delay_ms = 150;
	} else {
		rmi4_data->reset_delay_ms = 100;
	}
#ifdef RED_REMOTE
	rmi4_data->fw_debug = false;
	rmi4_data->i2c_client =  rmi4_data->synaptics_chip_data->ts_platform_data->client;
#endif
	rmi4_data->i2c_read = synaptics_rmi4_i2c_read;
	rmi4_data->i2c_write = synaptics_rmi4_i2c_write;
	rmi4_data->reset_device = synaptics_rmi4_reset_device;
	rmi4_data->status_resume = synaptics_rmi4_status_resume;
	rmi4_data->status_save = synaptics_rmi4_status_save;
	rmi4_data->report_touch = synaptics_rmi4_report_touch;

	rmi4_data->synaptics_chip_data->is_in_cell = true;
	rmi4_data->force_update = false;
	rmi4_data->sensor_max_x = rmi4_data->synaptics_chip_data->x_max - 1;
	rmi4_data->sensor_max_y = rmi4_data->synaptics_chip_data->y_max - 1;
	rmi4_data->sensor_max_x_mt =
	    rmi4_data->synaptics_chip_data->x_max_mt - 1;
	rmi4_data->sensor_max_y_mt =
	    rmi4_data->synaptics_chip_data->y_max_mt - 1;
	rmi4_data->flip_x =
	    rmi4_data->synaptics_chip_data->flip_x;
	rmi4_data->flip_y =
	    rmi4_data->synaptics_chip_data->flip_y;
	rmi4_data->synaptics_chip_data->easy_wakeup_info.sleep_mode =
	    TS_POWER_OFF_MODE;
	rmi4_data->synaptics_chip_data->easy_wakeup_info.easy_wakeup_gesture =
	    false;
	rmi4_data->synaptics_chip_data->easy_wakeup_info.easy_wakeup_flag =
	    false;
	rmi4_data->synaptics_chip_data->easy_wakeup_info.palm_cover_flag =
	    false;
	rmi4_data->synaptics_chip_data->easy_wakeup_info.palm_cover_control =
	    false;
	rmi4_data->synaptics_chip_data->easy_wakeup_info.off_motion_on = false;
	rmi4_data->synaptics_chip_data->diff_data = diff_data_buf;
#if defined (CONFIG_TEE_TUI)
	strncpy(tee_tui_data.device_name, "synaptics", strlen("synaptics"));
	tee_tui_data.device_name[strlen("synaptics")] = '\0';
#endif
	mutex_init(&wrong_touch_lock);

	rc = synaptics_rmi4_query_device(rmi4_data);
	if (rc < 0) {
		TS_LOG_ERR("Failed to synaptics_rmi4_query_device\n");
		return rc;
	}
	rc = synaptics_init_fwu_info();
	if (rc < 0) {
		TS_LOG_ERR("%s,Failed to init fwu info\n", __func__);
		return rc;
	}

	synap_parse_chip_specific_dts(rmi4_data->synaptics_chip_data);
	if (NULL != rmi4_data->module_name) {
		synaptics_sett_param_regs->module_name = rmi4_data->module_name;
		strncpy(rmi4_data->synaptics_chip_data->module_name,  rmi4_data->module_name, MAX_STR_LEN-1);
		TS_LOG_INFO("module name is %s\n",synaptics_sett_param_regs->module_name);
	}
	else{
		TS_LOG_ERR("get IC module name failed ");
	}

	return rc;
}

static int synaptics_get_brightness_info(void)
{
	int error = NO_ERR;
	int bl_max_nit = 0;
	struct ts_oem_info_param *info =NULL;
	TS_LOG_INFO("%s: Enter\n", __func__);

	//setting the read brightness type
	memset(tp_type_cmd, 0x0, TS_CHIP_TYPE_MAX_SIZE);
	tp_type_cmd[0] = TS_CHIP_BRIGHTNESS_TYPE;
	if (TS_UNINIT == atomic_read(&rmi4_data->synaptics_chip_data->ts_platform_data->state)) {
		TS_LOG_INFO("%s:ts module not initialize\n", __func__);
		bl_max_nit = 0;
		return	bl_max_nit;
	}

	info =
	(struct ts_oem_info_param *)
		kzalloc(sizeof(struct ts_oem_info_param), GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("%s: malloc failed\n", __func__);
		error = -ENOMEM;
	goto out;
	}

	synaptics_get_oem_info( info);

	if(info->data[0] == 0x1) {
		TS_LOG_INFO("%s:brightness info is not find in TPIC FLASH\n", __func__);
		bl_max_nit = 0;
	} else {
		TS_LOG_INFO("%s:brightness info find in TPIC FLASH\n", __func__);
		bl_max_nit = info->data[3] << 8 | info->data[2];
	}

out:
	if (NULL != info) {
		kfree(info);
		info = NULL;
	}
	return bl_max_nit;
}



static int synaptics_fw_update_boot(char *file_name)
{
	int retval = NO_ERR;
	bool need_update = false;
	int projectid_lenth = 0;

	if (rmi4_data->synaptics_chip_data->projectid_len) {
		projectid_lenth = rmi4_data->synaptics_chip_data->projectid_len;
	} else {
		projectid_lenth = PROJECT_ID_FW_LEN;
	}

#ifdef RED_REMOTE
	/*used for red remote fucntion */
	synap_fw_debug_dev_init(rmi4_data);
#endif

	TS_LOG_INFO("synaptics_fw_update_boot called\n");

	if (SYNAPTICS_S3718 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_TD4322 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_TD4310 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_S3706 != rmi4_data->synaptics_chip_data->ic_type) {
		retval = synap_fw_data_init(rmi4_data);
		if (retval) {
			TS_LOG_ERR("synaptics_fw_data_init failed\n");
			goto data_release;
		}
		strncat(file_name, rmi4_data->rmi4_mod_info.project_id_string,
			projectid_lenth);
		TS_LOG_INFO("file_name name is :%s\n", file_name);
		msleep(1000);
		retval =
		    synap_get_fw_data_boot(file_name,
					       synaptics_sett_param_regs);
		if (retval) {
			retval = 0;
			TS_LOG_ERR("load fw data from bootimage error\n");
			goto data_release;
		}

		need_update = synap_check_fw_version();

		if (rmi4_data->force_update || need_update) {
			retval = synap_fw_update();
			if (retval) {
				TS_LOG_ERR("failed update fw\n");
				strncpy(config_id_string,
					rmi4_data->rmi4_mod_info.device_config_id,
					strlen(rmi4_data->rmi4_mod_info.device_config_id));
			} else {
				TS_LOG_INFO("successfully update fw\n");
				strncpy(config_id_string,
					rmi4_data->rmi4_mod_info.
					image_config_id,
					strlen(rmi4_data->rmi4_mod_info.image_config_id));
			}
		} else {
			strncpy(config_id_string,
				rmi4_data->rmi4_mod_info.device_config_id,
				strlen(rmi4_data->rmi4_mod_info.
				       device_config_id));
		}
	} else {
		retval = synap_fw_data_s3718_init(rmi4_data);
		if (retval) {
			TS_LOG_ERR("synaptics_fw_data_s3718_init failed\n");
			goto data_release;
		}
		strncat(file_name, rmi4_data->rmi4_mod_info.project_id_string,
			projectid_lenth);
		TS_LOG_INFO("file_name name is :%s\n", file_name);
		msleep(1000);
		retval =
		    synap_get_fw_data_s3718_boot(file_name,
						     synaptics_sett_param_regs);
		if (retval) {
			retval = 0;
			TS_LOG_ERR("load fw data from s3718 bootimage error\n");
			goto data_release;
		}

		need_update = synap_check_fw_s3718_version();

		if (rmi4_data->force_update || need_update) {
			retval = synap_fw_s3718_update();
			if (retval) {
				TS_LOG_ERR("failed update s3718 fw\n");
				strncpy(config_id_string,
					rmi4_data->rmi4_mod_info.device_config_id,
					strlen(rmi4_data->rmi4_mod_info.device_config_id));
			} else {
				TS_LOG_INFO("successfully s3718 update fw\n");
				strncpy(config_id_string,
					rmi4_data->rmi4_mod_info.
					image_config_id,
					strlen(rmi4_data->rmi4_mod_info.image_config_id));
			}
		} else {
			strncpy(config_id_string,
				rmi4_data->rmi4_mod_info.device_config_id,
				strlen(rmi4_data->rmi4_mod_info.device_config_id));
		}
	}

data_release:
	if (SYNAPTICS_S3718 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_TD4322 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_TD4310 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_S3706 != rmi4_data->synaptics_chip_data->ic_type)
		synap_fw_data_release();
	else
		synap_fw_data_s3718_release();
	if (!rmi4_data->synaptics_chip_data->unite_cap_test_interface) {
		if (SYNAPTICS_S3207 == rmi4_data->synaptics_chip_data->ic_type) {
			retval = synap_rmi4_f54_s3207_init(rmi4_data, synaptics_sett_param_regs->module_name);	/* for test report function*/
			if (retval) {
				TS_LOG_ERR("synap_rmi4_f54_init failed\n");
			}
		}
	}
	strncpy(rmi4_data->synaptics_chip_data->version_name,config_id_string,CHIP_INFO_LENGTH);
	fb_esd_recover_disable(0);
	return retval;
}

static int synaptics_fw_update_sd(void)
{
	int retval = NO_ERR;

	TS_LOG_INFO("synaptics_fw_update_sd called\n");
	TS_LOG_INFO("ic_type = %d\n", rmi4_data->synaptics_chip_data->ic_type);
	synaptics_rmi4_status_save(rmi4_data);

	if (SYNAPTICS_S3718 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_TD4322 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_TD4310 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_S3706 != rmi4_data->synaptics_chip_data->ic_type) {
		retval = synap_fw_data_init(rmi4_data);
		if (retval) {
			TS_LOG_ERR("synaptics_fw_data_init failed\n");
			goto data_release;
		}
		msleep(1000);
		retval = synap_get_fw_data_sd();
		if (retval) {
			TS_LOG_ERR("load fw data from bootimage error\n");
			goto data_release;
		}

		/*just check the fw version */
		synap_check_fw_version();

		retval = synap_fw_update();
		if (retval < 0) {
			TS_LOG_ERR("failed update fw\n");
			strncpy(rmi4_data->synaptics_chip_data->version_name,rmi4_data->rmi4_mod_info.device_config_id,CHIP_INFO_LENGTH);

		} else {
			strncpy(rmi4_data->synaptics_chip_data->version_name,rmi4_data->rmi4_mod_info.image_config_id,CHIP_INFO_LENGTH);
			TS_LOG_INFO("success update fw\n");
		}
	} else {
		retval = synap_fw_data_s3718_init(rmi4_data);
		if (retval) {
			TS_LOG_ERR("synaptics_fw_data_s3718_init failed\n");
			goto data_release;
		}
		msleep(1000);
		retval = synap_get_fw_data_s3718_sd();
		if (retval) {
			TS_LOG_ERR("synaptics_get_fw_data_s3718_sd\n");
			goto data_release;
		}

		/*just check the fw version */
		synap_check_fw_s3718_version();

		retval = synap_fw_s3718_update();
		if (retval < 0) {
			strncpy(rmi4_data->synaptics_chip_data->version_name,rmi4_data->rmi4_mod_info.device_config_id,CHIP_INFO_LENGTH);
			TS_LOG_ERR("failed update s3718 fw\n");
		} else {
			strncpy(rmi4_data->synaptics_chip_data->version_name,rmi4_data->rmi4_mod_info.image_config_id,CHIP_INFO_LENGTH);
			TS_LOG_INFO("success update s3718 fw\n");
		}
	}

	synaptics_rmi4_status_resume(rmi4_data);
data_release:
	if (SYNAPTICS_S3718 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_TD4322 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_TD4310 != rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_S3706 != rmi4_data->synaptics_chip_data->ic_type)
		synap_fw_data_release();
	else
		synap_fw_data_s3718_release();
	return retval;
}

static int synaptics_regs_operate(struct ts_regs_info *info)
{
	int retval = NO_ERR;
	u8 *value = info->values;
	u8 bit_value = info->values[0];
	unsigned int bit = (unsigned int)info->bit;
	int i = 0;

	TS_LOG_INFO("addr(%d),op_action(%d),bit(%d),num(%d)\n", info->addr,
		    info->op_action, info->bit, info->num);

	for (i = 0; i < info->num; i++) {
		TS_LOG_INFO("value[%d]=%d\n", i, info->values[i]);
	}
	switch (info->op_action) {
	case TS_ACTION_WRITE:
		if ((1 == info->num) && (8 > bit)) {
			/* only change bit of regiseter */
			retval =
			    synaptics_rmi4_i2c_read(rmi4_data, info->addr,
						    value, info->num);
			if (retval < 0) {
				TS_LOG_ERR("TS_ACTION_READ error, addr(%d)\n",
					   info->addr);
				return -EINVAL;
			}

			if (!bit_value)
				value[0] &= ~(1 << bit);
			else
				value[0] |= (1 << bit);
		}

		retval = synaptics_rmi4_i2c_write(rmi4_data, info->addr, value, info->num);
		if (retval < 0) {
			TS_LOG_ERR("TS_ACTION_WRITE error, addr(%d)\n", info->addr);
			return -EINVAL;
		}
		break;
	case TS_ACTION_READ:
		retval = synaptics_rmi4_i2c_read(rmi4_data, info->addr, value,
					    info->num);
		if (retval < 0) {
			TS_LOG_ERR("TS_ACTION_READ error, addr(%d)\n", info->addr);
			retval = -EINVAL;
			goto out;
		}

		/* read bit of regiseter only */
		if ((1 == info->num) && (8 > bit)) {
			value[0] = (value[0] >> bit) & 0x01;
		}
		break;
	default:
		TS_LOG_ERR("%s, reg operate default invalid action %d\n", __func__, info->op_action);
		return -EINVAL;
	}
out:
	return retval;
}
static int synaptics_set_wakeup_gesture_enable_switch(u8 enable)
{
	int retval = NO_ERR;
	u8 holster_temp_value = 0;
	unsigned short holster_enable_addr = 0;
	unsigned char holster_bit_num = 0;

	holster_enable_addr =
	    rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.holster_info.holster_switch_addr;
	holster_bit_num =
	    rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.holster_info.holster_switch_bit;
	TS_LOG_INFO ("%s, synaptics holster_enable_addr=0x%04x, holster_bit_num=%d\n",
			__func__, holster_enable_addr, holster_bit_num);

	if (!holster_enable_addr) {
		TS_LOG_ERR("%s, holster_enable_addr is null.\n", __func__);
		goto out;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data, holster_enable_addr,
					&holster_temp_value, 1);
	if (retval < 0) {
		TS_LOG_ERR("read holster information failed: %d\n", retval);
		goto out;
	}
	if (!enable) {
		holster_temp_value |= 1 << holster_bit_num;
	} else {
		holster_temp_value &= ~(1 << holster_bit_num);
	}

	TS_LOG_INFO("%s, write TP IC\n", __func__);
	retval =
	    synaptics_rmi4_i2c_write(rmi4_data, holster_enable_addr,
				     &holster_temp_value, 1);
	if (retval < 0) {
		TS_LOG_ERR("switch wakeup gesture enable mode failed: %d\n",
			   retval);
	}
out:
	return retval;
}

static int synaptics_set_charger_switch(u8 charger_switch)
{
	int retval = NO_ERR;
	u8 charger_temp_value = 0;
	unsigned short charger_enable_addr = 0;
	unsigned char charger_bit_num = 0;

	charger_enable_addr =
	    rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.charger_info.charger_switch_addr;
	charger_bit_num =
	    rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.charger_info.charger_switch_bit;
	TS_LOG_INFO
	    ("synaptics charger_enable_addr=0x%04x, charger_bit_num=%d\n",
	     charger_enable_addr, charger_bit_num);

	if (!charger_enable_addr) {
		TS_LOG_ERR
		    ("charger_enable_addr is null, charger feature is not supported.\n");
		goto out;
	}

	retval =
	    synaptics_rmi4_i2c_read(rmi4_data, charger_enable_addr,
				    &charger_temp_value, 1);
	if (retval < 0) {
		TS_LOG_ERR("read charger information failed: %d\n", retval);
		goto out;
	}
	if (charger_switch) {
		charger_temp_value |= 1 << charger_bit_num;
	} else {
		charger_temp_value &= ~(1 << charger_bit_num);
	}

	retval =
	    synaptics_rmi4_i2c_write(rmi4_data, charger_enable_addr,
				     &charger_temp_value, 1);
	if (retval < 0) {
		TS_LOG_ERR("write charger information failed: %d\n", retval);
	}
out:
	TS_LOG_INFO("%s, write TP IC, addr=0x%04x, value=0x%04x\n", __func__,
		    charger_enable_addr, charger_temp_value);
	return retval;
}

static int synaptics_charger_switch(struct ts_charger_info *info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		retval = synaptics_set_charger_switch(info->charger_switch);
		if (retval < 0) {
			TS_LOG_ERR("set charger switch(%d), failed: %d\n",
				   info->charger_switch, retval);
		}
		break;
	default:
		TS_LOG_INFO("%s, invalid cmd\n", __func__);
		retval = -EINVAL;
		break;
	}

	return retval;
}

static int synaptics_set_hover_switch(u8 hover_switch)
{
	int retval = NO_ERR;
	int hover_temp_value = 0;
	unsigned short hover_enable_addr = 0;
	unsigned char hover_bit_num = 0;

	TS_LOG_INFO("%s:hover_switch = %d.\n",__func__, hover_switch);

	hover_enable_addr =  rmi4_data->hover_switch_addr;
	hover_bit_num = rmi4_data->hover_switch_bit;

	if (!hover_enable_addr) {
		TS_LOG_ERR("hover feature is not supported.\n");
		goto out;
	}

	retval =  synaptics_rmi4_i2c_read(rmi4_data, hover_enable_addr,
				    (unsigned char *)&hover_temp_value, 1);
	if (retval < 0) {
		TS_LOG_ERR("read hover information failed: %d\n", retval);
		goto out;
	}
	if (!hover_switch) {
		hover_temp_value |= 1 << hover_bit_num;//set 1 to disable hover scan
	} else {
		hover_temp_value &= ~(1 << hover_bit_num);
	}

	TS_LOG_INFO("hover_enable_addr=0x%04x, hover_bit_num=%d,hover_temp_value=0x%x",
	     hover_enable_addr, hover_bit_num ,hover_temp_value);

	retval = synaptics_rmi4_i2c_write(rmi4_data, hover_enable_addr,
				     (unsigned char *)&hover_temp_value, 1);
	if (retval < 0) {
		TS_LOG_ERR("write hover information failed: %d\n", retval);
	}
out:
	return retval;
}


static int synaptics_set_holster_switch(u8 holster_switch)
{
	int retval = NO_ERR;
	int holster_temp_value = 0;
	unsigned short holster_enable_addr = 0;
	unsigned char holster_bit_num = 0;

	TS_LOG_INFO("synaptics_set_holster_switch called\n");

	holster_enable_addr =
	    rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.holster_info.holster_switch_addr;
	holster_bit_num =
	    rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.holster_info.holster_switch_bit;
	TS_LOG_INFO
	    ("synaptics holster_enable_addr=0x%04x, holster_bit_num=%d\n",
	     holster_enable_addr, holster_bit_num);

	if (!holster_enable_addr) {
		TS_LOG_ERR
		    ("holster_enable_addr is null, hoslter feature is not supported.\n");
		goto out;
	}

	retval =
	    synaptics_rmi4_i2c_read(rmi4_data, holster_enable_addr,
				    (unsigned char *)&holster_temp_value, 1);
	if (retval < 0) {
		TS_LOG_ERR("read holster information failed: %d\n", retval);
		goto out;
	}
	if (holster_switch) {
		holster_temp_value |= 1 << holster_bit_num;
	} else {
		holster_temp_value &= ~(1 << holster_bit_num);
	}

	retval =
	    synaptics_rmi4_i2c_write(rmi4_data, holster_enable_addr,
				     (unsigned char *)&holster_temp_value, 1);
	if (retval < 0) {
		TS_LOG_ERR("write holster information failed: %d\n", retval);
	}
out:
	return retval;
}

static int synaptics_holster_switch(struct ts_holster_info *info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR("synaptics_holster_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		retval = synaptics_set_holster_switch(info->holster_switch);
		if (retval < 0) {
			TS_LOG_ERR("set holster switch(%d), failed: %d\n",
				   info->holster_switch, retval);
		}
		break;
	case TS_ACTION_READ:
		TS_LOG_INFO
		    ("invalid holster switch(%d) action: TS_ACTION_READ\n",
		     info->holster_switch);
		break;
	default:
		TS_LOG_INFO("invalid holster switch(%d) action: %d\n",
			    info->holster_switch, info->op_action);
		retval = -EINVAL;
		break;
	}

	return retval;
}

void synaptics_set_screenoff_status_reg(void)
{
	unsigned short fc_addr = 0;
	u8  temp_fc_switch = 0;
	int retval = NO_ERR;
	if(true != rmi4_data->screenoff_status_support)
		return;
	fc_addr = (unsigned short)rmi4_data->screenoff_status_reg;
	TS_LOG_INFO("fc_addr=0x%04x, \n",fc_addr);

	temp_fc_switch = (u8)rmi4_data->screenoff_status_switch;
	TS_LOG_INFO("screenoff_status_switch=0x%04x, \n",temp_fc_switch);
	retval = synaptics_rmi4_i2c_write(rmi4_data, fc_addr, &temp_fc_switch,1);
	if (retval < 0) {
		TS_LOG_ERR("set fc addr failed: %d, fc_addr=0x%04x\n",retval, fc_addr);
		return;
	}
	return;
}
static int synaptics_set_roi_switch(u8 roi_switch)
{
	int retval = NO_ERR;
	int i = 0;
	unsigned short roi_ctrl_addr = 0;
	u8 roi_control_bit = 0, temp_roi_switch = 0;

	if (!f51found)
		return -ENODEV;

	roi_ctrl_addr = rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.roi_info.roi_control_addr;
	roi_control_bit = rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.roi_info.roi_control_bit;
	TS_LOG_INFO
	    ("roi_ctrl_write_addr=0x%04x, roi_control_bit=%d, roi_switch=%d\n",
	     roi_ctrl_addr, roi_control_bit, roi_switch);

	if (!roi_ctrl_addr) {
		TS_LOG_ERR
		    ("roi_ctrl_addr is null, roi feature is not supported.\n");
		goto out;
	}

	retval =
	    synaptics_rmi4_i2c_read(rmi4_data, roi_ctrl_addr, &temp_roi_switch,
				    1);
	if (retval < 0) {
		TS_LOG_ERR("read roi_control value information failed: %d\n",
			   retval);
		return retval;
	}

	if (roi_switch)
		temp_roi_switch |= (1 << roi_control_bit);
	else
		temp_roi_switch &= (~(1 << roi_control_bit));

	retval =
	    synaptics_rmi4_i2c_write(rmi4_data, roi_ctrl_addr, &temp_roi_switch,
				     1);
	if (retval < 0) {
		TS_LOG_ERR
		    ("set roi switch failed: %d, roi_ctrl_write_addr=0x%04x\n",
		     retval, roi_ctrl_addr);
		return retval;
	}
	f51_roi_switch = temp_roi_switch;
	if (!roi_switch) {
		for (i = 0; i < ROI_DATA_READ_LENGTH; i++) {
			roi_data[i] = 0;
		}
	}
out:
	return retval;
}

static int synaptics_read_roi_switch(void)
{
	int retval = NO_ERR;
	unsigned short roi_ctrl_addr = 0;
	u8 roi_control_bit = 0, roi_switch = 0;
	if (!f51found)
		return -ENODEV;

	roi_ctrl_addr = rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.roi_info.roi_control_addr;
	roi_control_bit = rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.roi_info.roi_control_bit;

	if (!roi_ctrl_addr) {
		TS_LOG_ERR
		    ("roi_ctrl_addr is null, roi feature is not supported.\n");
		goto out;
	}

	retval =
	    synaptics_rmi4_i2c_read(rmi4_data, roi_ctrl_addr, &roi_switch,
				    sizeof(roi_switch));
	if (retval < 0) {
		TS_LOG_ERR("read roi switch failed: %d\n", retval);
		return retval;
	}
	roi_switch &= (1 << roi_control_bit);
	rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.roi_info.roi_switch = roi_switch;
	f51_roi_switch = roi_switch;
out:
	TS_LOG_INFO("roi_ctrl_read_addr=0x%04x, roi_switch=%d\n", roi_ctrl_addr,
		    roi_switch);
	return retval;
}

static int synaptics_roi_switch(struct ts_roi_info *info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR("synaptics_roi_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		retval = synaptics_set_roi_switch(info->roi_switch);
		if (retval < 0) {
			TS_LOG_ERR("%s, synaptics_set_roi_switch faild\n",
				   __func__);
		}
		break;
	case TS_ACTION_READ:
		retval = synaptics_read_roi_switch();
		break;
	default:
		TS_LOG_INFO("invalid roi switch(%d) action: %d\n",
			    info->roi_switch, info->op_action);
		retval = -EINVAL;
		break;
	}
	return retval;
}

static unsigned char *synaptics_roi_rawdata(void)
{
	if (!f51found)
		return NULL;

	if (f51_roi_switch)  {
		if (roi_data_staled)  {  /* roi_data may be refreshing now, wait it for some time(30ms). */
			if (!wait_for_completion_interruptible_timeout(&roi_data_done, msecs_to_jiffies(30))) {
				roi_data_staled = 0;  /* Never wait again if data refreshing gets timeout. */
				memset(roi_data, 0, sizeof(roi_data));
			}
		}
	}

	return (unsigned char *)roi_data;
}

static int synaptics_do_calibrate(unsigned char write_value, int count)
{
	int ret = NO_ERR;
	u8 value = 1;
	int delay_time = 200;
	unsigned short addr = 0;

	count = count / delay_time;

	if (strncmp
	    (rmi4_data->rmi4_mod_info.product_id_string, "S332U",
	     strlen("S332U"))) {
		TS_LOG_INFO("%s, not S332U, no need to calibrate\n", __func__);
		goto out;
	}

	addr =
	    synap_f54_get_calibrate_addr(rmi4_data,
					     synaptics_sett_param_regs->
					     module_name);
	if (!addr) {
		TS_LOG_ERR("%s, get addr error!\n", __func__);
		ret = -ENOMEM;
		goto out;
	}

	ret = synaptics_rmi4_i2c_write(rmi4_data, addr, &write_value, 1);
	if (ret < 0) {
		TS_LOG_ERR("%s synaptics_rmi4_i2c_write",__func__);
	}

	do {
		count--;
		ret = synaptics_rmi4_i2c_read(rmi4_data, addr, &value, sizeof(value));
		if(!ret){
			TS_LOG_ERR("%s synaptics_rmi4_i2c_read fail",__func__);
		}
		TS_LOG_INFO("remain count = %d, value is 0x%04x\n", count,
			    value);
		if (!value) {
			TS_LOG_INFO("%s success\n", __func__);
			goto out;
		}
		msleep(delay_time);
	} while (count > 0);
	ret = -ENOMEM;
	TS_LOG_ERR("%s timeout\n", __func__);

out:
	return ret;

}

/*calirate in sleepin
  1. Write 0x02 to F54_CTRL + 0x2C.
  2. Read value from F54_CTRL + 0x2C until it becomes 0.
*/
static int synaptics_calibrate_wakeup_gesture(void)
{
	int count = 24000;
	unsigned char value = 0x02;
	int ret = NO_ERR;

	ret = synaptics_do_calibrate(value, count);
	if (ret) {
		TS_LOG_ERR("%s error\n", __func__);
	}

	return ret;
}

/*display on*/
/*calibrate in sleepout
  1. Write 0x01 to F54_CTRL + 0x2C.
  2. Read value from F54_CTRL + 0x2C until it becomes 0.*/
static int synaptics_calibrate(void)
{
	int count = 24000;
	unsigned char value = 0x01;
	int ret = NO_ERR;

	ret = synaptics_do_calibrate(value, count);
	if (ret) {
		TS_LOG_ERR("%s error\n", __func__);
	}

	return ret;
}

static int synaptics_get_glove_switch(u8 *glove_switch)
{
	int retval = NO_ERR;
	unsigned short glove_enable_addr = 0;
	unsigned char glove_bit_num = 0;
	u8 value = 0;

	if (!glove_switch) {
		TS_LOG_ERR
		    ("synaptics_get_glove_switch: glove_switch is Null\n");
		return -ENOMEM;
	}

	glove_enable_addr = rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.glove_info.glove_switch_addr;
	glove_bit_num = rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.glove_info.glove_switch_bit;
	TS_LOG_INFO("synaptics glove_enable_addr=%d, glove_bit_num=%d\n",
		    glove_enable_addr, glove_bit_num);

	if (!glove_enable_addr) {
		TS_LOG_ERR
		    ("glove_enable_addr is null, glove feature is not supported.\n");
		goto out;
	}

	retval =
	    synaptics_rmi4_i2c_read(rmi4_data, glove_enable_addr, &value,
				    sizeof(value));
	if (retval < 0) {
		TS_LOG_ERR("read glove switch(%d) err : %d\n", *glove_switch,
			   retval);
		goto out;
	}

	if (!(value & (1 << glove_bit_num))) {
		*glove_switch = GLOVE_SWITCH_OFF;
	} else {
		*glove_switch = GLOVE_SWITCH_ON;
	}

out:
	TS_LOG_DEBUG("synaptics_get_glove_switch done : %d\n", *glove_switch);
	return retval;
}

static int synaptics_set_glove_switch(u8 glove_switch)
{
	int retval = NO_ERR;
	int glove_temp_value = 0;
	unsigned short glove_enable_addr = 0;
	unsigned char glove_bit_num = 0;

	TS_LOG_INFO("synaptics_set_glove_switch called\n");

	glove_enable_addr = rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.glove_info.glove_switch_addr;
	glove_bit_num = rmi4_data->synaptics_chip_data->ts_platform_data->feature_info.glove_info.glove_switch_bit;
	TS_LOG_INFO("synaptics glove_enable_addr=%d, glove_bit_num=%d\n",
		    glove_enable_addr, glove_bit_num);

	if (!glove_enable_addr) {
		TS_LOG_ERR
		    ("glove_enable_addr is null, glove feature is not supported.\n");
		goto out;
	}

	retval =
	    synaptics_rmi4_i2c_read(rmi4_data, glove_enable_addr,
				    (unsigned char *)&glove_temp_value, 1);
	if (retval < 0) {
		TS_LOG_ERR("read glove information failed: %d\n", retval);
		goto out;
	}
	if (glove_switch) {
		glove_temp_value |= 1 << glove_bit_num;
	} else {
		glove_temp_value &= ~(1 << glove_bit_num);
	}

	retval =
	    synaptics_rmi4_i2c_write(rmi4_data, glove_enable_addr,
				     (unsigned char *)&glove_temp_value, 1);
	if (retval < 0) {
		TS_LOG_ERR("write glove information failed: %d\n", retval);
	}
out:
	return retval;
}

static int synaptics_glove_switch(struct ts_glove_info *info)
{
	int retval = NO_ERR;
	u8 buf = 0;

	if (!info) {
		TS_LOG_ERR("synaptics_glove_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}
	switch (info->op_action) {
	case TS_ACTION_READ:
		retval = synaptics_get_glove_switch(&buf);
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
		retval = synaptics_set_glove_switch(buf);
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

static int synaptics_chip_get_capacitance_test_type(struct ts_test_type_info
						    *info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR
		    ("synaptics_chip_get_capacitance_test_type: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}
	switch (info->op_action) {
	case TS_ACTION_READ:
		memcpy(info->tp_test_type,
		       rmi4_data->synaptics_chip_data->tp_test_type,
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

static int synaptics_get_palm_switch(u8 *palm_switch)
{
	int retval = NO_ERR;
	unsigned char palm_enable_addr = 0;
	unsigned char palm_enable_offset = 0;

	if (!palm_switch) {
		TS_LOG_ERR("synaptics_get_palm_switch: palm_switch is Null\n");
		return -ENOMEM;
	}
	if (SYNAPTICS_S3207 == rmi4_data->synaptics_chip_data->ic_type) {
		*palm_switch =
		    rmi4_data->synaptics_chip_data->easy_wakeup_info.
		    palm_cover_flag;
	} else {
		palm_enable_offset =
		    rmi4_data->rmi4_feature.geswakeup_feature.
		    f12_2d_ctrl22_palm;
		palm_enable_addr =
		    rmi4_data->rmi4_feature.f12_ctrl_base_addr +
		    palm_enable_offset;
		TS_LOG_INFO("get palm addr : 0x%02x\n", palm_enable_addr);

		retval =
		    synaptics_rmi4_i2c_read(rmi4_data, palm_enable_addr,
					    palm_switch, sizeof(*palm_switch));
		if (retval < 0) {
			TS_LOG_ERR("read palm switch(%d) err : %d\n",
				   *palm_switch, retval);
			goto out;
		}
	}

	TS_LOG_DEBUG("synaptics_get_palm_switch done : %d\n", *palm_switch);
out:
	return retval;
}

static int synaptics_set_palm_switch(u8 palm_switch)
{
	int retval = NO_ERR;
	unsigned char palm_enable_addr = 0;
	unsigned char palm_enable_offset = 0;
	if (SYNAPTICS_S3207 == rmi4_data->synaptics_chip_data->ic_type) {
		TS_LOG_INFO
		    ("just set palm_contrlo_flag = control for S3207, palm_switch = %d\n",
		     palm_switch);
		rmi4_data->synaptics_chip_data->easy_wakeup_info.
		    palm_cover_flag =
		    rmi4_data->synaptics_chip_data->easy_wakeup_info.
		    palm_cover_control;
	} else {
		palm_enable_offset =
		    rmi4_data->rmi4_feature.geswakeup_feature.
		    f12_2d_ctrl22_palm;
		palm_enable_addr =
		    rmi4_data->rmi4_feature.f12_ctrl_base_addr +
		    palm_enable_offset;
		TS_LOG_INFO
		    ("set palm addr : 0x%02x , value(%d) (1:close) (0:open)\n",
		     palm_enable_addr, palm_switch);

		retval =
		    synaptics_rmi4_i2c_write(rmi4_data, palm_enable_addr,
					     &palm_switch, 1);
		if (retval < 0) {
			TS_LOG_ERR("open palm function failed: %d\n", retval);
		}
	}
	return retval;
}

static int synaptics_palm_switch(struct ts_palm_info *info)
{
	int retval = NO_ERR;
	u8 buf = 0;

	if (!info) {
		TS_LOG_ERR("synaptics_palm_set_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}
	switch (info->op_action) {
	case TS_ACTION_READ:
		if (SYNAPTICS_S3207 == rmi4_data->synaptics_chip_data->ic_type) {
			info->palm_switch =
			    rmi4_data->synaptics_chip_data->easy_wakeup_info.
			    palm_cover_flag;
		} else {
			retval = synaptics_get_palm_switch(&buf);
			if (retval < 0) {
				TS_LOG_ERR
				    ("get palm switch(%d), failed : %d\n",
				     info->palm_switch, retval);
			}
			if ((buf & PALM_REG_BIT) == 1) {	/*palm close status :1 plam invald*/
				info->palm_switch = 0;
			} else if ((buf & PALM_REG_BIT) == 0) {	/*palm open status :0 palm int - sleep*/
				info->palm_switch = 1;
			} else {
				TS_LOG_ERR("wrong state: buf = %d\n", buf);
				retval = -EFAULT;
			}
		}
		break;
	case TS_ACTION_WRITE:
		if (info->palm_switch == 1) {
			buf = 0;	/*open the palm function*/
		} else if (info->palm_switch == 0) {
			buf = 1;	/*close the palm function*/
		} else {
			TS_LOG_ERR("wrong input : %d\n", info->palm_switch);
			buf = 1;
		}
		retval = synaptics_set_palm_switch(buf);
		if (retval < 0) {
			TS_LOG_ERR("set palm switch(%d), failed : %d\n", buf,
				   retval);
		}
		break;
	default:
		TS_LOG_ERR("invalid switch status: %d\n", info->palm_switch);
		retval = -EINVAL;
		break;
	}

	return retval;
}

static void synaptics_shutdown(void)
{
	TS_LOG_INFO("synaptics_shutdown\n");
	synaptics_power_off();
	synaptics_power_release();
	return;
}

/*  do some things before power off.
*/
static int synaptics_before_suspend(void)
{
	int retval = NO_ERR;

	TS_LOG_INFO("before_suspend +\n");
	TS_LOG_INFO("before_suspend -\n");
	return retval;
}

static void synaptics_put_device_into_easy_wakeup(void)
{
	int retval = NO_ERR;
	unsigned char device_ctrl = 0;
	unsigned char device_ctrl_data[4] = { 0 };
	unsigned short f12_ctrl_base = 0;
	unsigned char gusture_ctrl_offset = 0;
	struct ts_easy_wakeup_info *info =
	    &rmi4_data->synaptics_chip_data->easy_wakeup_info;
	TS_LOG_DEBUG
	    ("synaptics_put_device_into_easy_wakeup  info->easy_wakeup_flag =%x \n",
	     info->easy_wakeup_flag);
	/*if the sleep_gesture_flag is ture,it presents that  the tp is at sleep state*/

	if (true == info->easy_wakeup_flag) {
		TS_LOG_INFO
		    ("synaptics_put_device_into_easy_wakeup  info->easy_wakeup_flag =%x \n",
		     info->easy_wakeup_flag);
		return;
	}
	if (SYNAPTICS_S3207 == rmi4_data->synaptics_chip_data->ic_type) {
		retval =
		    synaptics_rmi4_i2c_read(rmi4_data,
					    rmi4_data->rmi4_feature.
					    f11_ctrl_base_addr, &device_ctrl,
					    sizeof(device_ctrl));
		if (retval < 0) {
			TS_LOG_ERR(" Failed to enter sleep mode1\n");
			rmi4_data->sensor_sleep = false;
			return;
		}
		device_ctrl = (device_ctrl & ~MASK_3BIT);	/*get the high five bits,i'dont*/
		device_ctrl = (device_ctrl | 0x04);	/*easy-wakeup bit*/
		retval =
		    synaptics_rmi4_i2c_write(rmi4_data,
					     rmi4_data->rmi4_feature.
					     f11_ctrl_base_addr, &device_ctrl,
					     sizeof(device_ctrl));
		if (retval < 0) {
			TS_LOG_ERR("Failed to enter sleep mode2\n");
			rmi4_data->sensor_sleep = false;
			return;
		} else {
			rmi4_data->sensor_sleep = true;
		}
		/*set fast_rate */
		device_ctrl = 0x07;
		retval =
		    synaptics_rmi4_i2c_write(rmi4_data, EASY_WAKEUP_FASTRATE,
					     &device_ctrl, sizeof(device_ctrl));
		if (retval < 0) {
			TS_LOG_ERR("Failed to set fastrate\n");
			rmi4_data->sensor_sleep = false;
			return;
		} else {
			rmi4_data->sensor_sleep = true;
		}
		TS_LOG_DEBUG
		    (" read from  f11_ctrl_base_addr92  rmi4_data->f11_ctrl_base_addr  = 0x%04x rmi4_data->f11_ctrl_base_addr+92 =0x%04x\n",
		     rmi4_data->rmi4_feature.f11_ctrl_base_addr,
		     rmi4_data->rmi4_feature.f11_ctrl_base_addr + 45);

		retval =
		    synaptics_rmi4_i2c_read(rmi4_data,
					    rmi4_data->rmi4_feature.
					    f11_ctrl_base_addr + 45,
					    &device_ctrl, sizeof(device_ctrl));
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to set wakeup mode3\n",
				   __func__);
			rmi4_data->sensor_sleep = false;
			return;
		} else {
			rmi4_data->sensor_sleep = true;
		}
		TS_LOG_DEBUG
		    (" read from  f11_ctrl_base_addr92  ctrl1 = %0x retval =%0x\n",
		     device_ctrl, retval);

		retval =
		    synaptics_rmi4_i2c_read(rmi4_data, F54_ANALOG_CMD0,
					    &device_ctrl, sizeof(device_ctrl));
		if (retval < 0) {
			TS_LOG_ERR("Failed to set wakeup mode7\n");
			rmi4_data->sensor_sleep = false;
			return;
		} else {
			rmi4_data->sensor_sleep = true;
		}

		device_ctrl = device_ctrl | 0x04;
		TS_LOG_DEBUG
		    (" read from  f11_ctrl_base_addr92  ctrl1 = %0x retval =%0x\n",
		     device_ctrl, retval);
		retval =
		    synaptics_rmi4_i2c_write(rmi4_data, F54_ANALOG_CMD0,
					     &device_ctrl, sizeof(device_ctrl));
		if (retval < 0) {
			TS_LOG_ERR("Failed to set wakeup mode8\n");
			rmi4_data->sensor_sleep = false;
			return;
		} else {
			rmi4_data->sensor_sleep = true;
		}
	} else {
		gusture_ctrl_offset =
		    rmi4_data->rmi4_feature.geswakeup_feature.f12_2d_ctrl20_lpm;
		f12_ctrl_base = rmi4_data->rmi4_feature.f12_ctrl_base_addr;
		/*reg Report Wakeup Gesture Only read first */
		retval =
		    synaptics_rmi4_i2c_read(rmi4_data,
					    f12_ctrl_base + gusture_ctrl_offset,
					    &device_ctrl_data[0],
					    sizeof(device_ctrl_data));
		if (retval < 0) {
			TS_LOG_ERR
			    ("read Wakeup Gesture Only reg F12_2D_CTRL20 fail !\n");
		} else {
			TS_LOG_DEBUG
			    ("read Wakeup Gesture Only reg F12_2D_CTRL20(00)/00 Motion Suppression : CTRL20(00)/00 : 0x%02x  CTRL20(00)/01 : 0x%02x CTRL20(01)/00 : 0x%02x CTRL20(04)/00 : 0x%02x\n",
			     device_ctrl_data[0], device_ctrl_data[1],
			     device_ctrl_data[2], device_ctrl_data[3]);
		}

		/*Report Wakeup Gesture Only set bit 1 */
		device_ctrl_data[2] =
		    device_ctrl_data[2] | GESTURE_ENABLE_BIT01;

		/*Wakeup Gesture Only bit(01) set 1 */
		retval =
		    synaptics_rmi4_i2c_write(rmi4_data,
					     f12_ctrl_base +
					     gusture_ctrl_offset,
					     &device_ctrl_data[0],
					     sizeof(device_ctrl_data));
		if (retval < 0) {
			TS_LOG_ERR
			    ("easy wake up suspend write Wakeup Gesture Only reg fail !\n");
		} else {
			TS_LOG_INFO
			    ("easy wake up suspend write Wakeup Gesture Only reg OK address(0x%02x) valve(0x%02x)\n",
			     f12_ctrl_base + gusture_ctrl_offset,
			     device_ctrl_data[2]);
		}
	}
	info->easy_wakeup_flag = true;
	return;
}

static void synaptics_put_device_outof_easy_wakeup(struct synaptics_rmi4_data
						   *rmi4_data)
{
	int retval = NO_ERR;
	unsigned char device_ctrl = 0;
	unsigned char device_ctrl_data[4] = { 0 };
	unsigned short f12_ctrl_base = 0;
	unsigned char gusture_ctrl_offset = 0;
	struct ts_easy_wakeup_info *info =
	    &rmi4_data->synaptics_chip_data->easy_wakeup_info;

	TS_LOG_DEBUG
	    ("synaptics_put_device_outof_easy_wakeup  info->easy_wakeup_flag =%d\n",
	     info->easy_wakeup_flag);

	if (false == info->easy_wakeup_flag) {
		return;
	}
	if (SYNAPTICS_S3207 == rmi4_data->synaptics_chip_data->ic_type) {
		/*set fastrate*/
		device_ctrl =
		    rmi4_data->synaptics_chip_data->easy_wakeup_info.
		    easy_wakeup_fastrate;
		retval =
		    synaptics_rmi4_i2c_write(rmi4_data, EASY_WAKEUP_FASTRATE,
					     &device_ctrl, sizeof(device_ctrl));
		if (retval < 0) {
			TS_LOG_ERR("Failed to out of  sleep mode1\n");
			rmi4_data->sensor_sleep = true;
			return;
		}
		retval =
		    synaptics_rmi4_i2c_read(rmi4_data,
					    rmi4_data->rmi4_feature.
					    f11_ctrl_base_addr, &device_ctrl,
					    sizeof(device_ctrl));
		if (retval < 0) {
			TS_LOG_ERR("Failed to out of  sleep mode4\n");
			rmi4_data->sensor_sleep = true;
			return;
		}
		device_ctrl = ((device_ctrl & (~MASK_3BIT)) | MASK_1BIT);
		retval =
		    synaptics_rmi4_i2c_write(rmi4_data,
					     rmi4_data->rmi4_feature.
					     f11_ctrl_base_addr, &device_ctrl,
					     sizeof(device_ctrl));
		if (retval < 0) {
			TS_LOG_ERR("Failed to out of  sleep mode5\n");
			rmi4_data->sensor_sleep = true;
			return;
		} else {
			rmi4_data->sensor_sleep = false;
		}
	} else {
		gusture_ctrl_offset =
		    rmi4_data->rmi4_feature.geswakeup_feature.f12_2d_ctrl20_lpm;
		f12_ctrl_base = rmi4_data->rmi4_feature.f12_ctrl_base_addr;
		/*reg Report Wakeup Gesture Only read first */
		retval =
		    synaptics_rmi4_i2c_read(rmi4_data,
					    f12_ctrl_base + gusture_ctrl_offset,
					    &device_ctrl_data[0],
					    sizeof(device_ctrl_data));
		if (retval < 0) {
			TS_LOG_ERR
			    ("read Wakeup Gesture Only reg F12_2D_CTRL20 fail !\n");
		} else {
			TS_LOG_DEBUG
			    ("read Wakeup Gesture Only reg F12_2D_CTRL20(00)/00 Motion Suppression : CTRL20(00)/00 : 0x%02x  CTRL20(00)/01 : 0x%02x CTRL20(01)/00 : 0x%02x CTRL20(04)/00 : 0x%02x\n",
			     device_ctrl_data[0], device_ctrl_data[1],
			     device_ctrl_data[2], device_ctrl_data[3]);
		}

		/*Report Wakeup Gesture Only set bit 0 */
		device_ctrl_data[2] =
		    device_ctrl_data[2] & (~GESTURE_ENABLE_BIT01);

		/*Wakeup Gesture Only bit(01) set 0 */
		retval =
		    synaptics_rmi4_i2c_write(rmi4_data,
					     f12_ctrl_base +
					     gusture_ctrl_offset,
					     &device_ctrl_data[0],
					     sizeof(device_ctrl_data));
		if (retval < 0) {
			TS_LOG_ERR
			    ("easy wake up resume write Wakeup Gesture Only reg fail\n");
		} else {
			TS_LOG_INFO
			    ("easy wake up suspend write Wakeup Gesture Only reg OK address(0x%02x) valve(0x%02x)\n",
			     f12_ctrl_base + gusture_ctrl_offset,
			     device_ctrl_data[2]);
		}
	}

	info->easy_wakeup_flag = false;
	return;
}

/**
 * synaptics_sensor_sleep()
 * This function stops finger data acquisition and puts the sensor to sleep.
 */
static void synaptics_sensor_sleep(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = NO_ERR;
	unsigned char device_ctrl = 0;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 rmi4_data->rmi4_feature.
					 f01_ctrl_base_addr, &device_ctrl,
					 sizeof(device_ctrl));
	if (retval < 0) {
		TS_LOG_ERR("Failed to enter sleep mode\n");
		return;
	}

	if (!strcmp(rmi4_data->synaptics_chip_data->ts_platform_data->product_name, "venus")
		|| !strcmp(rmi4_data->synaptics_chip_data->ts_platform_data->product_name, "victoria")
		||rmi4_data->synaptics_chip_data->ic_type == SYNAPTICS_S3706) {
		TS_LOG_INFO("%s enter sensor_sleep\n", rmi4_data->synaptics_chip_data->ts_platform_data->product_name);
		device_ctrl = (device_ctrl & ~MASK_3BIT);
		device_ctrl = (device_ctrl | NO_SLEEP_OFF | SENSOR_SLEEP_BIT0);
	} else {
		device_ctrl = (device_ctrl & ~MASK_2BIT);
		device_ctrl =
		    (device_ctrl | NORMAL_OPERATION | SENSOR_SLEEP_BIT1);
	}

	retval = synaptics_rmi4_i2c_write(rmi4_data,
					  rmi4_data->rmi4_feature.
					  f01_ctrl_base_addr, &device_ctrl,
					  sizeof(device_ctrl));
	if (retval < 0)
		TS_LOG_ERR("Failed to enter sleep mode\n");

	return;
}

static int synatpics_sleep_mode_in(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = 0;
	unsigned char intr_status = 0;

	TS_LOG_INFO("synatpics_sleep_mode_in\n");
	/* Clear interrupts first */
	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 rmi4_data->rmi4_feature.
					 f01_data_base_addr + 1, &intr_status,
					 rmi4_data->num_of_intr_regs);
	if (retval < 0)
		return retval;

	synaptics_sensor_sleep(rmi4_data);
	return retval;
}

/**
 * synaptics_sleep_mode_out()
 *
  * This function wakes the sensor from sleep.
 */
static void synaptics_sleep_mode_out(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = NO_ERR;
	unsigned char device_ctrl = 0;

	TS_LOG_INFO("synaptics_sleep_mode_out\n");
	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 rmi4_data->rmi4_feature.
					 f01_ctrl_base_addr, &device_ctrl,
					 sizeof(device_ctrl));
	if (retval < 0) {
		TS_LOG_ERR("Failed to wake from sleep mode\n");
		return;
	}

	if (!strcmp(rmi4_data->synaptics_chip_data->ts_platform_data->product_name, "venus")) {
		TS_LOG_INFO("%s  exit sensor_sleep\n", rmi4_data->synaptics_chip_data->ts_platform_data->product_name);
		device_ctrl = (device_ctrl & ~MASK_3BIT);
		device_ctrl = (device_ctrl | NO_SLEEP_ON | NORMAL_OPERATION);
	} else {
		device_ctrl = (device_ctrl & ~MASK_2BIT);
		device_ctrl =
		    (device_ctrl | SENSOR_SLEEP_BIT0 | NORMAL_OPERATION);
	}

	retval = synaptics_rmi4_i2c_write(rmi4_data,
					  rmi4_data->rmi4_feature.
					  f01_ctrl_base_addr, &device_ctrl,
					  sizeof(device_ctrl));
	if (retval < 0)
		TS_LOG_ERR("Failed to wake from sleep mode\n");

	return;
}

static int synaptics_suspend(void)
{
	int retval = NO_ERR;
	int tskit_pt_station_flag = 0;

	TS_LOG_INFO("in last time wake mode synaptics_interrupt_num = %d interrupts tskit_pt_station_flag = %d\n",
	     synaptics_interrupt_num,tskit_pt_station_flag);

	rmi4_data->ud_finger_status = rmi4_data->synaptics_chip_data->ts_platform_data->udfp_enable_flag;
	ts_kit_get_pt_station_status(&tskit_pt_station_flag);
	synaptics_interrupt_num = 0;
	TS_LOG_INFO("suspend +\n");
	switch (rmi4_data->synaptics_chip_data->easy_wakeup_info.sleep_mode) {
	case TS_POWER_OFF_MODE:
		/*for in_cell, tp will power off in suspend. */
		if(!rmi4_data->ud_finger_status){//udfp_enable_flag may be changed when TP have suspended
			if (!tskit_pt_station_flag)
				synaptics_power_off();
			else
				synatpics_sleep_mode_in(rmi4_data);	/*goto sleep mode*/
		} else {
			synaptics_set_screenoff_status_reg();
			TS_LOG_INFO("synaptics_suspend:udfp_enable_flag == 1.\n");
		}
		break;
		/*for gesture wake up mode suspend. */
	case TS_GESTURE_MODE:
		if (true ==
		    rmi4_data->synaptics_chip_data->easy_wakeup_info.
		    palm_cover_flag)
			rmi4_data->synaptics_chip_data->easy_wakeup_info.
			    palm_cover_flag = false;
		synaptics_put_device_into_easy_wakeup();
		mutex_lock(&wrong_touch_lock);
		rmi4_data->synaptics_chip_data->easy_wakeup_info.off_motion_on =
		    true;
		mutex_unlock(&wrong_touch_lock);
		synaptics_set_screenoff_status_reg();
		break;
	default:
		TS_LOG_ERR("no suspend mode\n");
		return -EINVAL;
	}
	TS_LOG_INFO("suspend -\n");
	return retval;
}

/*    do not add time-costly function here.
*/
static int synaptics_resume(void)
{
	int retval = NO_ERR;
	int tskit_pt_station_flag = 0;

	ts_kit_get_pt_station_status(&tskit_pt_station_flag);

	TS_LOG_INFO
	    ("between suspend and resumed there is synaptics_interrupt_num = %d interrupts tskit_pt_station_flag = %d\n",
	     synaptics_interrupt_num,tskit_pt_station_flag);
	synaptics_interrupt_num = 0;
	rmi4_data->synaptics_chip_data->enable_ghost_dmd_report = 0;
	TS_LOG_INFO("resume +\n");
	switch (rmi4_data->synaptics_chip_data->easy_wakeup_info.sleep_mode) {
	case TS_POWER_OFF_MODE:
		/*for in_cell, tp should power on in resume. */
		if(!rmi4_data->ud_finger_status){
			if (!tskit_pt_station_flag)
				synaptics_power_on();
			else
				synaptics_sleep_mode_out(rmi4_data);	/*exit sleep mode*/
		}
		else{
			TS_LOG_INFO("synaptics_suspend:udfp_enable_flag == 1.\n");
		}
		if (SYNAPTICS_TD4322 != rmi4_data->synaptics_chip_data->ic_type
			&&SYNAPTICS_TD4310 != rmi4_data->synaptics_chip_data->ic_type) {
			synaptics_gpio_reset();
		}
		break;
	case TS_GESTURE_MODE:
		synaptics_put_device_outof_easy_wakeup(rmi4_data);
		synaptics_gpio_reset();
		break;
	default:
		TS_LOG_ERR("no resume mode\n");
		return -EINVAL;
	}
	pre_finger_status = 0;
	TS_LOG_INFO("resume -\n");
	return retval;
}

/*  do some things after power on. */
static int synaptics_after_resume(void *feature_info)
{
	int retval = NO_ERR;
	TS_LOG_INFO("after_resume +\n");

	if (SYNAPTICS_TD4322 != rmi4_data->synaptics_chip_data->ic_type
	&&SYNAPTICS_TD4310 != rmi4_data->synaptics_chip_data->ic_type) {
		msleep(150);
	}

	TS_LOG_INFO("synaptics_after_resume increase delay 50ms\n");
	/*empty list and query device again */
	synaptics_rmi4_empty_fn_list(rmi4_data);
	retval = synaptics_rmi4_query_device(rmi4_data);
	if (retval < 0) {
		TS_LOG_ERR("Failed to query device\n");
		return retval;
	}

	synaptics_rmi4_status_resume(rmi4_data);

	TS_LOG_INFO("after_resume -\n");
	return retval;
}

static int synaptics_wakeup_gesture_enable_switch(struct
						  ts_wakeup_gesture_enable_info
						  *info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}

	if (info->op_action == TS_ACTION_WRITE) {
		retval =
		    synaptics_set_wakeup_gesture_enable_switch(info->
							       switch_value);
		TS_LOG_DEBUG("write deep_sleep switch: %d\n",
			     info->switch_value);
		if (retval < 0) {
			TS_LOG_ERR("set deep_sleep switch(%d), failed: %d\n",
				   info->switch_value, retval);
		}
	} else {
		TS_LOG_INFO("invalid deep_sleep switch(%d) action: %d\n",
			    info->switch_value, info->op_action);
		retval = -EINVAL;
	}

	return retval;
}

static int synaptics_rmi4_reset_command(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = NO_ERR;
	int page_number = 0;
	unsigned char command = 0x01;
	unsigned short pdt_entry_addr = 0;
	struct synaptics_rmi4_fn_desc rmi_fd;
	struct synaptics_rmi4_f01_device_status status;
	bool done = false;

	memset(&rmi_fd, 0, sizeof(rmi_fd));

rescan:
	/* Scan the page description tables of the pages to service */
	for (page_number = 0; page_number < PAGES_TO_SERVICE; page_number++) {
		for (pdt_entry_addr = PDT_START; pdt_entry_addr > PDT_END;
		     pdt_entry_addr -= PDT_ENTRY_SIZE) {
			pdt_entry_addr |= (page_number << 8);

			retval = synaptics_rmi4_i2c_read(rmi4_data,
							 pdt_entry_addr,
							 (unsigned char *)
							 &rmi_fd,
							 sizeof(rmi_fd));
			if (retval < 0)
				return retval;

			if (rmi_fd.fn_number == 0)
				break;

			switch (rmi_fd.fn_number) {
			case SYNAPTICS_RMI4_F01:
				rmi4_data->rmi4_feature.f01_cmd_base_addr =
				    rmi_fd.cmd_base_addr;
				rmi4_data->rmi4_feature.f01_data_base_addr =
				    rmi_fd.data_base_addr;
				if (synaptics_rmi4_crc_in_progress
				    (rmi4_data, &status))
					goto rescan;
				done = true;
				break;
			}
		}
		if (done) {
			TS_LOG_DEBUG
			    ("Find F01 in page description table 0x%04x\n",
			     rmi4_data->rmi4_feature.f01_cmd_base_addr);
			break;
		}
	}

	if (!done) {
		TS_LOG_ERR("Cannot find F01 in page description table\n");
		return -EINVAL;;
	}

	retval = synaptics_rmi4_i2c_write(rmi4_data,
					  rmi4_data->rmi4_feature.
					  f01_cmd_base_addr, &command,
					  sizeof(command));
	if (retval < 0) {
		TS_LOG_ERR("Failed to issue reset command, error = %d\n",
			   retval);
		return retval;
	}

	msleep(rmi4_data->reset_delay_ms);
	return retval;
}

static void synaptics_rmi4_empty_fn_list(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fn *fhandler = NULL;
	struct synaptics_rmi4_fn *fhandler_temp = NULL;
	struct synaptics_rmi4_device_info *rmi = NULL;

	rmi = &(rmi4_data->rmi4_mod_info);

	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry_safe(fhandler, fhandler_temp,
					 &rmi->support_fn_list, link) {
			list_del(&fhandler->link);
			if (fhandler->fn_number == SYNAPTICS_RMI4_F1A) {
				synaptics_rmi4_f1a_kfree(fhandler);
			} else {
				if (fhandler->data) {
					kfree(fhandler->data);
					fhandler->data = NULL;
				}
				if (fhandler->extra) {
					kfree(fhandler->extra);
					fhandler->extra = NULL;
				}
				if (fhandler->eratio_data) {
					kfree(fhandler->eratio_data);
					fhandler->eratio_data = NULL;
				}
			}
			kfree(fhandler);
			fhandler = NULL;
		}
	}

	return;
}

static int synaptics_reset_device(void)
{
	return synaptics_rmi4_reset_device(rmi4_data);
}

static int synaptics_rmi4_reset_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = NO_ERR;

	if (!rmi4_data) {
		TS_LOG_ERR("rmi4_data is NULL \n");
		retval = -ENOMEM;
		return retval;
	}

	TS_LOG_DEBUG("synaptics_rmi4_reset_device called\n");

	retval = synaptics_rmi4_reset_command(rmi4_data);
	if (retval < 0) {
		TS_LOG_ERR("Failed to send command reset\n");
		return retval;
	}

	synaptics_rmi4_empty_fn_list(rmi4_data);

	retval = synaptics_rmi4_query_device(rmi4_data);
	if (retval < 0) {
		TS_LOG_ERR("Failed to query device\n");
		return retval;
	}

	TS_LOG_DEBUG("synaptics_rmi4_reset_device end\n");
	return 0;
}

static int synaptics_rmi4_status_save(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = NO_ERR;

	return retval;
}

static int synaptics_rmi4_status_resume(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = 0;
	struct ts_feature_info *info = &rmi4_data->synaptics_chip_data->ts_platform_data->feature_info;

	retval =
	    synaptics_set_holster_switch(info->holster_info.holster_switch);
	if (retval < 0) {
		TS_LOG_ERR("Failed to set holster switch(%d), err: %d\n",
			   info->holster_info.holster_switch, retval);
	}

	if (info->roi_info.roi_supported) {
		retval = synaptics_set_roi_switch(info->roi_info.roi_switch);
		if (retval < 0) {
			TS_LOG_ERR("%s, synaptics_set_roi_switch faild\n",
				   __func__);
		}
	}
#ifdef GLOVE_SIGNAL
	retval = synaptics_set_glove_switch(info->glove_info.glove_switch);
	if (retval < 0) {
		TS_LOG_ERR("Failed to set glove switch(%d), err: %d\n",
			   info->glove_info.glove_switch, retval);
	}
#endif

	if (info->charger_info.charger_supported) {
		retval =
		    synaptics_set_charger_switch(info->charger_info.charger_switch);
		if (retval < 0) {
			TS_LOG_ERR
			    ("Failed to set charger switch(%d), err: %d\n",
			     info->charger_info.charger_switch, retval);
		}
	}
	if(TS_GESTURE_MODE == rmi4_data->synaptics_chip_data->easy_wakeup_info.sleep_mode) {
		retval =
			synaptics_set_palm_switch(!rmi4_data->synaptics_chip_data->
						easy_wakeup_info.palm_cover_control);
		if (retval < 0) {
			TS_LOG_ERR("Failed to set plam switch(%d), err: %d\n",
				   rmi4_data->synaptics_chip_data->easy_wakeup_info.
				   palm_cover_control, retval);
		}
	}
	TS_LOG_INFO("%s: glove switch: %d ,holster switch: %d,roi switch: %d, charger switch: %d\n",__func__,
		info->glove_info.glove_switch,info->holster_info.holster_switch,info->roi_info.roi_switch,info->charger_info.charger_switch);

	/*To ensure that hover is disabled after the phone is unlocked by the fingerprint, it is needed to disable hover when tp resume.
	    fwk will enable and disable hover when fingerprint unlocking is required on  screen on state.
	*/
	if(rmi4_data->need_disable_hover){
		retval = synaptics_set_hover_switch(0);
	}
	return retval;
}


#define FORCE_TOUCH_I2C 0x2C
#define SYN_I2C_RETRY_TIMES 0
#if 0
static int synaptics_rmi4_set_page_f35(struct synaptics_rmi4_data *rmi4_data,unsigned short addr)
{
	return 0;
}
#endif
static int synaptics_rmi4_i2c_read_f35(struct synaptics_rmi4_data *rmi4_data,unsigned short addr, unsigned char *data, unsigned short length)
{

	return 0;

}

static int synaptics_rmi4_i2c_write_f35(struct synaptics_rmi4_data *rmi4_data,unsigned short addr, unsigned char *data, unsigned short length)
{
	return 0;

}

/**
 * synaptics_rmi4_i2c_read()
 *
 * Called by various functions in this driver, and also exported to
 * other expansion Function modules such as rmi_dev.
 *
 * This function reads data of an arbitrary length from the sensor,
 * starting from an assigned register address of the sensor, via I2C
 * with a retry mechanism.
 */
static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
				   unsigned short addr, unsigned char *data,
				   unsigned short length)
{
	int retval = NO_ERR;
	unsigned char reg_addr = addr & MASK_8BIT;

	if (rmi4_data->use_ub_addr){
		retval = synaptics_rmi4_i2c_read_f35(rmi4_data, addr, data, length);
		return retval;
	}

	retval = synaptics_rmi4_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN) {
		if (rmi4_data->use_ub_supported) {
			rmi4_data->use_ub_addr = true;
		}
		TS_LOG_ERR("error, retval != PAGE_SELECT_LEN\n");
		goto exit;
	}

	rmi4_data->use_ub_addr = false;
	if (!rmi4_data->synaptics_chip_data->ts_platform_data->bops->bus_read) {
		TS_LOG_ERR("error, invalid bus_read\n");
		retval = -EIO;
		goto exit;
	}
	retval =
	    rmi4_data->synaptics_chip_data->ts_platform_data->bops->bus_read(&reg_addr, 1, data,
							   length);
	if (retval < 0) {
		TS_LOG_ERR("error, bus read failed, retval  = %d\n", retval);
		goto exit;
	}

exit:
	return retval;
}

 /**
 * synaptics_rmi4_i2c_write()
 *
 * Called by various functions in this driver, and also exported to
 * other expansion Function modules such as rmi_dev.
 *
 * This function writes data of an arbitrary length to the sensor,
 * starting from an assigned register address of the sensor, via I2C with
 * a retry mechanism.
 */
static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
				    unsigned short addr, unsigned char *data,
				    unsigned short length)
{
	int retval = NO_ERR;
	unsigned char reg_addr = addr & MASK_8BIT;
	unsigned char wr_buf[length + 1];
	wr_buf[0] = reg_addr;

	if (rmi4_data->use_ub_addr) {
		retval = synaptics_rmi4_i2c_write_f35(rmi4_data, addr, data, length);
		return retval;
	}
	memcpy(wr_buf+1, data, length);

	retval = synaptics_rmi4_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN) {
		if (rmi4_data->use_ub_supported) {
			rmi4_data->use_ub_addr = true;
		}
		TS_LOG_ERR("retval != PAGE_SELECT_LEN, retval = %d\n", retval);
		goto exit;
	}

	rmi4_data->use_ub_addr = false;
	if (!rmi4_data->synaptics_chip_data->ts_platform_data->bops->bus_write) {
		TS_LOG_ERR("bus_write not exits\n");
		retval = -EIO;
		goto exit;
	}
	retval =
	    rmi4_data->synaptics_chip_data->ts_platform_data->bops->bus_write(wr_buf, length + 1);
	if (retval < 0) {
		TS_LOG_ERR("bus_write failed, retval = %d\n", retval);
		goto exit;
	}

exit:
	return retval;
}

static void synaptics_check_crc_err_reset(struct
							synaptics_rmi4_f01_device_status
							status) {
	int ret = 0;

	if(STATUS_FIRMWARE_CRC_FAILURE == status.status_code) {
		ret = synaptics_rmi4_reset_device(rmi4_data);
		if(ret < 0) {
			TS_LOG_ERR("device reset failed!\n");
		}
	}
}

static int synaptics_rmi4_device_status_check(struct
					      synaptics_rmi4_f01_device_status
					      status)
{
	int ret = 0;

	TS_LOG_DEBUG("status value = 0x%02x\n", status.status_code);

	switch (status.status_code) {
	case STATUS_INVALID_CONFIG:
		TS_LOG_ERR("status value = 0x%02x(STATUS_INVALID_CONFIG)\n",
			   status.status_code);
		ret = -1;
		break;
	case STATUS_DEVICE_FAILURE:
		TS_LOG_ERR("status value = 0x%02x(STATUS_DEVICE_FAILURE)\n",
			   status.status_code);
		ret = -1;
		break;
	case STATUS_CONFIG_CRC_FAILURE:
		TS_LOG_ERR("status value = 0x%02x(STATUS_CONFIG_CRC_FAILURE)\n",
			   status.status_code);
		ret = -1;
		break;
	case STATUS_FIRMWARE_CRC_FAILURE:
		TS_LOG_ERR
		    ("status value = 0x%02x(STATUS_FIRMWARE_CRC_FAILURE)\n",
		     status.status_code);
		ret = -1;
		break;
	case STATUS_CRC_IN_PROGRESS:
		TS_LOG_ERR("status value = 0x%02x(STATUS_CRC_IN_PROGRESS)\n",
			   status.status_code);
		ret = -1;
		break;
	case STATUS_GUEST_CRC_FAILURE:
		TS_LOG_ERR("status value = 0x%02x(STATUS_GUEST_CRC_FAILURE)\n",
			   status.status_code);
		ret = -1;
		break;
	default:
		ret = 0;
		break;
	}
	return ret;
}

 /**
 * synaptics_rmi4_query_device()
 *
 * Called by synaptics_init_chip().
 *
 * This funtion scans the page description table, records the offsets
 * to the register types of Function $01, sets up the function handlers
 * for Function $11 and Function $12, determines the number of interrupt
 * sources from the sensor, adds valid Functions with data inputs to the
 * Function linked list, parses information from the query registers of
 * Function $01, and enables the interrupt sources from the valid Functions
 * with data inputs.
 */
static int synaptics_rmi4_query_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = NO_ERR;
	unsigned char ii = 0;
	unsigned char page_number = 0;
	unsigned char intr_count = 0;
	unsigned char data_sources = 0;
	unsigned short pdt_entry_addr = 0;
	unsigned short intr_addr = 0;
	struct synaptics_rmi4_f01_device_status status;
	struct synaptics_rmi4_fn_desc rmi_fd;
	struct synaptics_rmi4_fn *fhandler = NULL;
	struct synaptics_rmi4_device_info *rmi = NULL;
	struct synaptics_work_reg_status *ptr = &dsm_dump_register_map[1];

	memset(&rmi_fd, 0, sizeof(rmi_fd));
	rmi = &(rmi4_data->rmi4_mod_info);

rescan:
	INIT_LIST_HEAD(&rmi->support_fn_list);
	intr_count = 0;
	data_sources = 0;

	/* Scan the page description tables of the pages to service */
	for (page_number = 0; page_number < PAGES_TO_SERVICE; page_number++) {
		for (pdt_entry_addr = PDT_START; pdt_entry_addr > PDT_END;
		     pdt_entry_addr -= PDT_ENTRY_SIZE) {
			pdt_entry_addr |= (page_number << 8);
			retval = synaptics_rmi4_i2c_read(rmi4_data,
							 pdt_entry_addr,
							 (unsigned char *)
							 &rmi_fd,
							 sizeof(rmi_fd));
			if (retval < 0) {
				TS_LOG_ERR
				    ("read pdt_entry_addr = %d regiseter error happened\n",
				     pdt_entry_addr);
				return retval;
			}
			fhandler = NULL;

			if (rmi_fd.fn_number == 0) {
				TS_LOG_DEBUG("Reached end of PDT\n");
				break;
			}

			TS_LOG_INFO("F%02x found (page %d)\n", rmi_fd.fn_number,
				    page_number);

			switch (rmi_fd.fn_number) {
			case SYNAPTICS_RMI4_F01:
				rmi4_data->rmi4_feature.f01_query_base_addr =
				    rmi_fd.query_base_addr;
				TS_LOG_DEBUG("f01 query base addr = 0x%04x\n",
					     rmi4_data->rmi4_feature.
					     f01_query_base_addr);
				rmi4_data->rmi4_feature.f01_ctrl_base_addr =
				    rmi_fd.ctrl_base_addr;
				rmi4_data->rmi4_feature.f01_data_base_addr =
				    rmi_fd.data_base_addr;
				ptr->fhandler_ctrl_base =
				    rmi4_data->rmi4_feature.f01_data_base_addr;
				TS_LOG_DEBUG("f01 query base addr = 0x%04x\n",
					     rmi4_data->rmi4_feature.
					     f01_data_base_addr);
				rmi4_data->rmi4_feature.f01_cmd_base_addr =
				    rmi_fd.cmd_base_addr;

				if (synaptics_rmi4_crc_in_progress
				    (rmi4_data, &status))
					goto rescan;

				retval =
				    synaptics_rmi4_query_device_info(rmi4_data);
				if (retval < 0) {
					TS_LOG_ERR
					    ("Failed to read device_info \n");
					return retval;
				}
				if (status.flash_prog == 1) {
					TS_LOG_INFO
					    ("In flash prog mode, status = 0x%02x\n",
					     status.status_code);
					rmi4_data->force_update = true;	/*crc error, force update fw to workaround */
#if defined (CONFIG_HUAWEI_DSM)
					ts_dmd_report(DSM_TP_FW_CRC_ERROR_NO, "device status for 20003 is :%d\n", status.status_code);
#endif
				}

				if (synaptics_rmi4_device_status_check(status)) {
					TS_LOG_ERR
					    ("device  status is error ,status value = 0x%02x\n",
					     status.status_code);
#if defined (CONFIG_HUAWEI_DSM)
					ts_dmd_report(DSM_TP_DEV_STATUS_ERROR_NO, "device status for 20004 is :%d\n", status.status_code);
#endif
				}

				if(SUPPORT_CRC_ERR_DO_RESET == rmi4_data->support_crc_err_do_reset) {
					synaptics_check_crc_err_reset(status);
				}
				break;

			case SYNAPTICS_RMI4_F11:
				if (rmi_fd.intr_src_count == 0)
					break;

				fhandler =
				    synaptics_rmi4_alloc_fh(&rmi_fd,
							    page_number);
				if (NULL == fhandler) {
					TS_LOG_ERR("Failed to alloc for F%d\n",
						   rmi_fd.fn_number);
					retval = -ENOMEM;
					return retval;
				}

				retval =
				    synaptics_rmi4_f11_init(rmi4_data, fhandler,
							    &rmi_fd,
							    intr_count);
				if (retval < 0) {
					TS_LOG_ERR
					    ("Failed to init f11 handler , retval = %d\n",
					     retval);
					goto exit;
				}
				break;

			case SYNAPTICS_RMI4_F12:
				if (rmi_fd.intr_src_count == 0)
					break;

				fhandler =
				    synaptics_rmi4_alloc_fh(&rmi_fd,
							    page_number);
				if (NULL == fhandler) {
					TS_LOG_ERR("Failed to alloc for F%d\n",
						   rmi_fd.fn_number);
					retval = -ENOMEM;
					return retval;
				}

				retval =
				    synaptics_rmi4_f12_init(rmi4_data, fhandler,
							    &rmi_fd,
							    intr_count);
				if (retval < 0) {
					TS_LOG_ERR
					    ("Failed to init f12 handler , retval = %d\n",
					     retval);
					goto exit;
				}
				break;

			case SYNAPTICS_RMI4_F1A:
				if (rmi_fd.intr_src_count == 0)
					break;

				fhandler =
				    synaptics_rmi4_alloc_fh(&rmi_fd,
							    page_number);
				if (NULL == fhandler) {
					TS_LOG_ERR("Failed to alloc for F%d\n",
						   rmi_fd.fn_number);
					retval = -ENOMEM;
					return retval;
				}

				retval =
				    synaptics_rmi4_f1a_init(rmi4_data, fhandler,
							    &rmi_fd,
							    intr_count);
				if (retval < 0) {
					TS_LOG_ERR
					    ("Failed to init f1a handler , retval = %d\n",
					     retval);
					goto exit;
				}
				break;
			case SYNAPTICS_RMI4_F51:
				f51found = true;
				TS_LOG_INFO("SYNAPTICS_RMI4_F51\n");
				TS_LOG_INFO("rmi_fd.intr_src_count =0x%04x\n",
					    rmi_fd.intr_src_count);
				fhandler =
				    synaptics_rmi4_alloc_fh(&rmi_fd,
							    page_number);
				if (NULL == fhandler) {
					TS_LOG_ERR("Failed to alloc for F%d\n",
						   rmi_fd.fn_number);
					retval = -ENOMEM;
					return retval;
				}

				retval =
				    synaptics_rmi4_f51_init(rmi4_data, fhandler,
							    &rmi_fd, intr_count,
							    page_number);
				if (retval < 0)
					goto exit;
				break;
			case SYNAPTICS_RMI4_F54:
			case SYNAPTICS_RMI4_F55:
				if (rmi_fd.intr_src_count == 0)
					break;

				fhandler =
				    synaptics_rmi4_alloc_fh(&rmi_fd,
							    page_number);
				if (NULL == fhandler) {
					TS_LOG_ERR("Failed to alloc for F%d\n",
						   rmi_fd.fn_number);
					retval = -ENOMEM;
					return retval;
				}
				break;

			default:
				break;
			}

			/* Accumulate the interrupt count */
			intr_count += (rmi_fd.intr_src_count & MASK_3BIT);
			if (fhandler) {
				list_add_tail(&fhandler->link,
					      &rmi->support_fn_list);
			}
		}
	}

	rmi4_data->num_of_intr_regs = (intr_count + 7) / 8;
	if (rmi4_data->num_of_intr_regs >= MAX_INTR_REGISTERS) {
		rmi4_data->num_of_intr_regs = 0;
		memset(rmi4_data->intr_mask, 0x00, sizeof(rmi4_data->intr_mask));
		return -EINVAL;
	}
	TS_LOG_DEBUG("Number of interrupt registers = %d\n",
		     rmi4_data->num_of_intr_regs);

	memset(rmi4_data->intr_mask, 0x00, sizeof(rmi4_data->intr_mask));

	/*
	 * Map out the interrupt bit masks for the interrupt sources
	 * from the registered function handlers.
	 */
	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link)
		    data_sources += fhandler->num_of_data_sources;
	}
	if (data_sources) {
		if (!list_empty(&rmi->support_fn_list)) {
			list_for_each_entry(fhandler,
					    &rmi->support_fn_list, link) {
				if (fhandler->num_of_data_sources) {
					rmi4_data->intr_mask[fhandler->
							     intr_reg_num] |=
					    fhandler->intr_mask;
				}
			}
		}
	} else {
		TS_LOG_ERR("the num of data sources init fail from IC == %d\n",
			   data_sources);
	}

	/* Enable the interrupt sources */
	for (ii = 0; ii < rmi4_data->num_of_intr_regs; ii++) {
		if (rmi4_data->intr_mask[ii] != 0x00) {
			TS_LOG_DEBUG("Interrupt enable mask %d = 0x%02x\n", ii,
				     rmi4_data->intr_mask[ii]);
			intr_addr =
			    rmi4_data->rmi4_feature.f01_ctrl_base_addr + 1 + ii;
			retval =
			    synaptics_rmi4_i2c_write(rmi4_data, intr_addr,
						     &(rmi4_data->
						       intr_mask[ii]),
						     sizeof(rmi4_data->
							    intr_mask[ii]));
			if (retval < 0) {
				TS_LOG_ERR
				    ("Failed to enable interrupt sources, error:%d",
				     retval);
				return retval;
			}
		}
	}

	return 0;
exit:
	if(fhandler){
		kfree(fhandler);
		fhandler = NULL;
	}
	return retval;
}

  /**
 * synaptics_rmi4_crc_in_progress()
 *
 * Check if crc in progress ever occured
 *
 */
static bool synaptics_rmi4_crc_in_progress(struct synaptics_rmi4_data
					   *rmi4_data,
					   struct
					   synaptics_rmi4_f01_device_status
					   *status)
{
	int retval = NO_ERR;
	int times = 0;
	bool rescan = false;
	if(SYNAPTICS_S3706 != rmi4_data->synaptics_chip_data->ic_type)
	{
		if (rmi4_data->delay_for_fw_update)
			msleep(200);
	}

	while (1) {
		retval = synaptics_rmi4_i2c_read(rmi4_data,
						 rmi4_data->rmi4_feature.
						 f01_data_base_addr,
						 status->data,
						 sizeof(status->data));
		if (retval < 0) {
			TS_LOG_ERR
			    ("read rmi4_data->f01_data_base_addr = %d status register failed\n",
			     rmi4_data->rmi4_feature.f01_data_base_addr);
			return false;
		}
		if (status->status_code == STATUS_CRC_IN_PROGRESS) {
			TS_LOG_ERR
			    ("CRC is in progress..., the statu_scode is %d \n",
			     status->status_code);
			rescan = true;
			msleep(20);
		} else {
			TS_LOG_DEBUG("CRC is success, CRC test times = %d\n",
				     times);
			break;
		}
		if (times++ > 500)
			return false;
	}
	return rescan;
}

/**
 * synaptics_rmi4_set_page()
 *
 * Called by synaptics_rmi4_i2c_read() and synaptics_rmi4_i2c_write().
 *
 * This function writes to the page select register to switch to the
 * assigned page.
 */
static int synaptics_rmi4_set_page(struct synaptics_rmi4_data *rmi4_data,
				   unsigned int address)
{
	int retval = NO_ERR;
	unsigned char buf[PAGE_SELECT_LEN] = { 0 };
	unsigned char page = 0;

	page = ((address >> 8) & MASK_8BIT);
	if (page != rmi4_data->current_page) {
		buf[0] = MASK_8BIT;
		buf[1] = page;
		retval =
		    rmi4_data->synaptics_chip_data->ts_platform_data->bops->bus_write(buf,
								    PAGE_SELECT_LEN);
		if (retval != NO_ERR) {
			TS_LOG_ERR("bus_write failed\n");
		} else {
			rmi4_data->current_page = page;
		}
	} else {
		return PAGE_SELECT_LEN;
	}
	return (retval == NO_ERR) ? PAGE_SELECT_LEN : -EIO;
}

static struct synaptics_rmi4_fn *synaptics_rmi4_alloc_fh(struct
							 synaptics_rmi4_fn_desc
							 *rmi_fd,
							 int page_number)
{
	struct synaptics_rmi4_fn *fhandler = NULL;

	fhandler = kzalloc(sizeof(struct synaptics_rmi4_fn), GFP_KERNEL);
	if (!fhandler) {
		TS_LOG_ERR("Failed to alloc memory for fhandler\n");
		return NULL;
	}

	fhandler->full_addr.data_base =
	    (rmi_fd->data_base_addr | (page_number << 8));
	fhandler->full_addr.ctrl_base =
	    (rmi_fd->ctrl_base_addr | (page_number << 8));
	fhandler->full_addr.cmd_base =
	    (rmi_fd->cmd_base_addr | (page_number << 8));
	fhandler->full_addr.query_base =
	    (rmi_fd->query_base_addr | (page_number << 8));
	fhandler->fn_number = rmi_fd->fn_number;
	TS_LOG_DEBUG
	    ("handler number is %d, it's data_base_addr = %d, ctrl_base_addr = %d, cmd_base_addr = %d,query_base_addr = %d, page_number = %d\n",
	     rmi_fd->fn_number, rmi_fd->data_base_addr, rmi_fd->ctrl_base_addr,
	     rmi_fd->cmd_base_addr, rmi_fd->query_base_addr, page_number);
	return fhandler;
}

 /**
 * synaptics_rmi4_query_device_info()
 *
 * Called by synaptics_rmi4_query_device().
 *
 */
static int synaptics_rmi4_query_device_info(struct synaptics_rmi4_data
					    *rmi4_data)
{
	int retval = 0;
	int count = 11;
	unsigned char f01_query[F01_STD_QUERY_LEN] = { 0 };
	struct synaptics_rmi4_device_info *rmi = &(rmi4_data->rmi4_mod_info);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 rmi4_data->rmi4_feature.
					 f01_query_base_addr, f01_query,
					 sizeof(f01_query));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read f01_query_base_addr %d\n",
			   rmi4_data->rmi4_feature.f01_query_base_addr);
		goto out;
	}
	/* RMI Version 4.0 currently supported */
	rmi->version_major = 4;
	rmi->version_minor = 0;

	rmi->manufacturer_id = f01_query[0];
	rmi->product_props = f01_query[1];
	rmi->product_info[0] = f01_query[2] & MASK_7BIT;
	rmi->product_info[1] = f01_query[3] & MASK_7BIT;
	rmi->date_code[0] = f01_query[4] & MASK_5BIT;
	rmi->date_code[1] = f01_query[5] & MASK_4BIT;
	rmi->date_code[2] = f01_query[6] & MASK_5BIT;
	rmi->tester_id =
	    ((f01_query[7] & MASK_7BIT) << 8) | (f01_query[8] & MASK_7BIT);
	rmi->serial_number =
	    ((f01_query[9] & MASK_7BIT) << 8) | (f01_query[10] & MASK_7BIT);
	memcpy(rmi->product_id_string, &f01_query[11], 10);
	while ((count <= 20) && (f01_query[count] != 0)) {
		count++;
	}
	rmi->synaptics_build_id[0] = f01_query[count - 3];
	rmi->synaptics_build_id[1] = f01_query[count - 2];
	rmi->synaptics_build_id[2] = f01_query[count - 1];
	rmi->synaptics_ic_name[0] = f01_query[count - 5];
	rmi->synaptics_ic_name[1] = f01_query[count - 4];
	TS_LOG_DEBUG("count is %d \n", count);
	TS_LOG_DEBUG("synaptics_build_id is (%c  %c  %c)\n",
		    f01_query[count - 3], f01_query[count - 2],
		    f01_query[count - 1]);
	TS_LOG_DEBUG("product ic string is %s\n",
		     rmi4_data->rmi4_mod_info.product_id_string);
	TS_LOG_DEBUG("synaptics_ic_name 0 is %d,  synaptics_ic_name 1 is %d\n",
		     rmi->synaptics_ic_name[0], rmi->synaptics_ic_name[1]);
	if (rmi->manufacturer_id != 1) {
		TS_LOG_ERR("Non-Synaptics device found, manufacturer ID = %d\n",
			   rmi->manufacturer_id);
	}
	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 rmi4_data->rmi4_feature.
					 f01_query_base_addr +
					 F01_BUID_ID_OFFSET, rmi->build_id,
					 sizeof(rmi->build_id));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read firmware build id (code %d)\n",
			   retval);
	}
	TS_LOG_INFO("FIRMWARE_PR:%d\n",(unsigned int)rmi->build_id[0] +
					(unsigned int)rmi->build_id[1] * 0x100 +
								(unsigned int)rmi->build_id[2] * 0x10000);
out:
	return retval;
}

  /**
 * synaptics_rmi4_f11_init()
 *
 * Called by synaptics_rmi4_query_device().
 *
 * This funtion parses information from the Function 11 registers
 * and determines the number of fingers supported, x and y data ranges,
 * offset to the associated interrupt status register, interrupt bit
 * mask, and gathers finger data acquisition capabilities from the query
 * registers.
 */
static int synaptics_rmi4_f11_init(struct synaptics_rmi4_data *rmi4_data,
				   struct synaptics_rmi4_fn *fhandler,
				   struct synaptics_rmi4_fn_desc *fd,
				   unsigned int intr_count)
{
	int retval = 0;
	unsigned char ii = 0;
	unsigned char intr_offset = 0;
	unsigned char abs_data_size = 0;
	unsigned char abs_data_blk_size = 0;
	unsigned char query[F11_STD_QUERY_LEN] = { 0 };
	unsigned char control[F11_STD_CTRL_LEN] = { 0 };

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 fhandler->full_addr.query_base,
					 query, sizeof(query));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read f11 query base reg\n");
		return retval;
	}
	/* Maximum number of fingers supported */
	if ((query[1] & MASK_3BIT) <= 4)
		fhandler->num_of_data_points = (query[1] & MASK_3BIT) + 1;
	else if ((query[1] & MASK_3BIT) == 5)
		fhandler->num_of_data_points = 10;

	rmi4_data->num_of_fingers = fhandler->num_of_data_points;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 fhandler->full_addr.ctrl_base,
					 control, sizeof(control));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read f11 ctrl base reg\n");
		return retval;
	}
	/* Maximum x and y */
	rmi4_data->sensor_max_x = ((control[6] & MASK_8BIT) << 0) |
	    ((control[7] & MASK_4BIT) << 8);
	rmi4_data->sensor_max_y = ((control[8] & MASK_8BIT) << 0) |
	    ((control[9] & MASK_4BIT) << 8);

	TS_LOG_INFO("Function %02x max x = %d max y = %d\n",
		    fhandler->fn_number, rmi4_data->sensor_max_x,
		    rmi4_data->sensor_max_y);

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num >= MAX_INTR_REGISTERS) {
		fhandler->intr_reg_num = 0;
		fhandler->num_of_data_sources = 0;
		fhandler->intr_mask = 0;
		return -EINVAL;
	}
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
	     ii < ((fd->intr_src_count & MASK_3BIT) + intr_offset); ii++)
		fhandler->intr_mask |= 1 << ii;

	abs_data_size = query[5] & MASK_2BIT;
	abs_data_blk_size = 3 + (2 * (abs_data_size == 0 ? 1 : 0));
	fhandler->size_of_data_register_block = abs_data_blk_size;
	rmi4_data->rmi4_feature.f11_query_base_addr = fd->query_base_addr;
	rmi4_data->rmi4_feature.f11_ctrl_base_addr = fd->ctrl_base_addr;
	rmi4_data->rmi4_feature.f11_data_base_addr = fd->data_base_addr;
	rmi4_data->rmi4_feature.f11_cmd_base_addr = fd->cmd_base_addr;
	return retval;
}

static int synaptics_rmi4_f12_set_enables(struct synaptics_rmi4_data *rmi4_data,
					  unsigned short ctrl28)
{
	int retval = NO_ERR;
	static unsigned short ctrl_28_address = 0;

	if (ctrl28)
		ctrl_28_address = ctrl28;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
					  ctrl_28_address,
					  &rmi4_data->report_enable,
					  sizeof(rmi4_data->report_enable));
	if (retval < 0) {
		TS_LOG_ERR("Failed to set enable f12, error:%d\n", retval);
		return retval;
	}

	return retval;
}

 /**
 * synaptics_rmi4_f12_init()
 *
 * Called by synaptics_rmi4_query_device().
 *
 * This funtion parses information from the Function 12 registers and
 * determines the number of fingers supported, offset to the data1
 * register, x and y data ranges, offset to the associated interrupt
 * status register, interrupt bit mask, and allocates memory resources
 * for finger data acquisition.
 */
static int synaptics_rmi4_f12_init(struct synaptics_rmi4_data *rmi4_data,
				   struct synaptics_rmi4_fn *fhandler,
				   struct synaptics_rmi4_fn_desc *fd,
				   unsigned int intr_count)
{
	int retval = NO_ERR;
	unsigned char ii = 0;
	unsigned char intr_offset = 0;
	unsigned char size_of_2d_data = 0;
	unsigned char size_of_query8 = 0;
	unsigned char ctrl_8_offset = 0;
	unsigned char ctrl_23_offset = 0;
	unsigned char ctrl_28_offset = 0;
	unsigned char ctrl_26_offset = 0;
	unsigned char num_of_fingers = 0;
	unsigned char ctrl_20_offset = 0;
	unsigned char ctrl_22_offset = 0;
	unsigned char data_04_offset = 0;
	unsigned char f12_2d_data[F12_2D_CTRL_LEN] = { 0 };
	struct synaptics_rmi4_f12_extra_data *extra_data = NULL;
	struct synaptics_rmi4_f12_eratio_data *eratio_data = NULL;
	struct synaptics_rmi4_f12_query_5 query_5;
	struct synaptics_rmi4_f12_query_8 query_8;
	struct synaptics_rmi4_f12_ctrl_8 ctrl_8;
	struct synaptics_rmi4_f12_ctrl_23 ctrl_23;
	struct synaptics_work_reg_status *ptr = &dsm_dump_register_map[0];

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;
	fhandler->extra = kmalloc(sizeof(*extra_data), GFP_KERNEL);
	if (fhandler->extra == NULL) {
		TS_LOG_ERR("Failed to alloc memory for fhandler->extra\n");
		retval = -ENOMEM;
		return retval;
	}
	fhandler->eratio_data = kzalloc((sizeof(*eratio_data) * F12_FINGERS_TO_SUPPORT), GFP_KERNEL);
	if (fhandler->eratio_data == NULL) {
		TS_LOG_ERR("Failed to alloc memory for fhandler->eratio_data\n");
		retval = -ENOMEM;
		goto eratio_data_error_free_mem;
	}
	TS_LOG_DEBUG
	    ("fhandler->num_of_data_sources = %d, fhandler->fn_number = %d\n",
	     fhandler->num_of_data_sources, fhandler->fn_number);
	extra_data = (struct synaptics_rmi4_f12_extra_data *)fhandler->extra;
	size_of_2d_data = sizeof(struct synaptics_rmi4_f12_finger_data);
	rmi4_data->rmi4_feature.f12_ctrl_base_addr =
	    fhandler->full_addr.ctrl_base;
	rmi4_data->rmi4_feature.f12_data_base_addr =
	    fhandler->full_addr.data_base;
	rmi4_data->rmi4_feature.f12_query_base_addr =
	    fhandler->full_addr.query_base;
	ptr->fhandler_ctrl_base = fd->ctrl_base_addr;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 fhandler->full_addr.query_base + 5,
					 query_5.data, sizeof(query_5.data));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read f12->full_addr.query_base\n");
		goto error_free_mem;
	}

	ctrl_8_offset = query_5.ctrl0_is_present +
	    query_5.ctrl1_is_present +
	    query_5.ctrl2_is_present +
	    query_5.ctrl3_is_present +
	    query_5.ctrl4_is_present +
	    query_5.ctrl5_is_present +
	    query_5.ctrl6_is_present + query_5.ctrl7_is_present;
	TS_LOG_DEBUG("ctrl_8_offset is %d\n", ctrl_8_offset);
	retval =
	    synaptics_rmi4_i2c_read(rmi4_data,
				    rmi4_data->rmi4_feature.f12_ctrl_base_addr +
				    ctrl_8_offset, f12_2d_data,
				    F12_2D_CTRL_LEN);
	if (retval < 0) {
		TS_LOG_ERR("Failed to read f12_ctrl_base\n");
		goto error_free_mem;
	}
	TS_LOG_INFO("rx = %d, tx = %d, f12init\n",
		    f12_2d_data[F12_RX_NUM_OFFSET],
		    f12_2d_data[F12_TX_NUM_OFFSET]);
	rmi4_data->num_of_rx = f12_2d_data[F12_RX_NUM_OFFSET];
	rmi4_data->num_of_tx = f12_2d_data[F12_TX_NUM_OFFSET];

	ctrl_20_offset = ctrl_8_offset +
	    query_5.ctrl8_is_present +
	    query_5.ctrl9_is_present +
	    query_5.ctrl10_is_present +
	    query_5.ctrl11_is_present +
	    query_5.ctrl12_is_present +
	    query_5.ctrl13_is_present +
	    query_5.ctrl14_is_present +
	    query_5.ctrl15_is_present +
	    query_5.ctrl16_is_present +
	    query_5.ctrl17_is_present +
	    query_5.ctrl18_is_present + query_5.ctrl19_is_present;
	rmi4_data->rmi4_feature.geswakeup_feature.f12_2d_ctrl20_lpm =
	    ctrl_20_offset;
	ptr->offset = ctrl_20_offset;

	ctrl_22_offset = ctrl_20_offset +
	    query_5.ctrl20_is_present + query_5.ctrl21_is_present;
	rmi4_data->rmi4_feature.geswakeup_feature.f12_2d_ctrl22_palm =
	    ctrl_22_offset;

	ctrl_23_offset = ctrl_22_offset + query_5.ctrl22_is_present;

	ctrl_26_offset = ctrl_23_offset +
	    query_5.ctrl23_is_present +
	    query_5.ctrl24_is_present + query_5.ctrl25_is_present;
	rmi4_data->rmi4_feature.glove_feature.offset = ctrl_26_offset;

	ctrl_28_offset = ctrl_26_offset +
	    query_5.ctrl26_is_present + query_5.ctrl27_is_present;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 fhandler->full_addr.ctrl_base +
					 ctrl_23_offset, ctrl_23.data,
					 sizeof(ctrl_23.data));
	if (retval < 0) {
		TS_LOG_ERR
		    ("Failed to read f12 ->full_addr.ctrl_base = %d + ctrl_23_offset = %d\n",
		     fhandler->full_addr.ctrl_base, ctrl_23_offset);
		goto error_free_mem;
	}

	/* Maximum number of fingers supported */
	fhandler->num_of_data_points = min(ctrl_23.max_reported_objects,
					   (unsigned char)
					   F12_FINGERS_TO_SUPPORT);

	num_of_fingers = fhandler->num_of_data_points;
	rmi4_data->num_of_fingers = num_of_fingers;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 fhandler->full_addr.query_base + 7,
					 &size_of_query8,
					 sizeof(size_of_query8));
	if (retval < 0) {
		TS_LOG_ERR
		    ("Failed to read f12 ->full_addr.query_base = %d,here is +7\n",
		     fhandler->full_addr.query_base);
		goto error_free_mem;
	}
	if(size_of_query8 > SYNAPTICS_RMI4_F12_QUERY_8_MAX){
		size_of_query8 = SYNAPTICS_RMI4_F12_QUERY_8_MAX;
		TS_LOG_ERR("size_of_query8 out of bounds = %d\n",size_of_query8);
	}
	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 fhandler->full_addr.query_base + 8,
					 query_8.data, size_of_query8);
	if (retval < 0) {
		TS_LOG_ERR
		    ("Failed to read f12 ->full_addr.query_base = %d,here is +8\n",
		     fhandler->full_addr.query_base);
		goto error_free_mem;
	}

	/* Determine the presence of the Data0 register */
	extra_data->data1_offset = query_8.data0_is_present;

	if ((size_of_query8 >= SIZE_OF_QUERY8) && (query_8.data15_is_present)) {
		extra_data->data15_offset = query_8.data0_is_present +
		    query_8.data1_is_present +
		    query_8.data2_is_present +
		    query_8.data3_is_present +
		    query_8.data4_is_present +
		    query_8.data5_is_present +
		    query_8.data6_is_present +
		    query_8.data7_is_present +
		    query_8.data8_is_present +
		    query_8.data9_is_present +
		    query_8.data10_is_present +
		    query_8.data11_is_present +
		    query_8.data12_is_present +
		    query_8.data13_is_present + query_8.data14_is_present;
		if(num_of_fingers > F12_FINGERS_TO_SUPPORT){
			TS_LOG_ERR("num_of_fingers too big  %d\n",num_of_fingers);
			num_of_fingers = F12_FINGERS_TO_SUPPORT;
		}
		extra_data->data15_size = (num_of_fingers + 7) / 8;
	} else {
		extra_data->data15_size = 0;
	}
	if ((size_of_query8 >= SIZE_OF_QUERY8) && (query_8.data36_is_present)) {
			TS_LOG_INFO("data36_is_present\n");
		extra_data->data36_offset = extra_data->data15_offset +
		    query_8.data15_is_present +
		    query_8.data16_is_present +
		    query_8.data17_is_present +
		    query_8.data18_is_present +
		    query_8.data19_is_present +
		    query_8.data20_is_present +
		    query_8.data21_is_present +
		    query_8.data22_is_present +
		    query_8.data23_is_present +
		    query_8.data24_is_present +
		    query_8.data25_is_present +
		    query_8.data26_is_present +
		    query_8.data27_is_present +
		    query_8.data28_is_present +
		    query_8.data29_is_present +
		    query_8.data30_is_present +
		    query_8.data31_is_present +
		    query_8.data32_is_present +
		    query_8.data33_is_present +
		    query_8.data34_is_present + query_8.data35_is_present;
	}

	if(rmi4_data->synaptics3718_Tp_Pressure_flag == 1){
		if ((size_of_query8 >= 5) && (query_8.data29_is_present)) {
			TS_LOG_INFO("f12_data29_is_present\n");
			extra_data->data29_offset = query_8.data0_is_present +
			    query_8.data1_is_present +
			    query_8.data2_is_present +
			    query_8.data3_is_present +
			    query_8.data4_is_present +
			    query_8.data5_is_present +
			    query_8.data6_is_present +
			    query_8.data7_is_present +
			    query_8.data8_is_present +
			    query_8.data9_is_present +
			    query_8.data10_is_present +
			    query_8.data11_is_present +
			    query_8.data12_is_present +
			    query_8.data13_is_present +
			    query_8.data14_is_present +
			    query_8.data15_is_present +
			    query_8.data16_is_present +
			    query_8.data17_is_present +
			    query_8.data18_is_present +
			    query_8.data19_is_present +
			    query_8.data20_is_present +
			    query_8.data21_is_present +
			    query_8.data22_is_present +
			    query_8.data23_is_present +
			    query_8.data24_is_present +
			    query_8.data25_is_present +
			    query_8.data26_is_present +
			    query_8.data27_is_present +
			    query_8.data28_is_present;

			extra_data->data29_size = 2;
		}
		else {
			extra_data->data29_offset = 0;
			extra_data->data29_size = 0;
		}
	}

	data_04_offset = query_8.data0_is_present +
	    query_8.data1_is_present +
	    query_8.data2_is_present + query_8.data3_is_present;
	rmi4_data->rmi4_feature.geswakeup_feature.f12_2d_data04_gesture =
	    data_04_offset;

	rmi4_data->report_enable = RPT_DEFAULT;
#ifdef REPORT_2D_Z
	rmi4_data->report_enable |= RPT_Z;
#endif
#ifdef SYNA_FORCE
	rmi4_data->report_enable |= RPT_Z;
#endif
#ifdef REPORT_2D_W
	rmi4_data->report_enable |= (RPT_WX | RPT_WY);
#endif

	retval = synaptics_rmi4_f12_set_enables(rmi4_data,
						fhandler->full_addr.ctrl_base +
						ctrl_28_offset);
	if (retval < 0) {
		TS_LOG_ERR
		    ("Failed to set enable f12,fhandler->full_addr.ctrl_base =%d, ctrl_28_offset = %d\n",
		     fhandler->full_addr.ctrl_base, ctrl_28_offset);
		goto error_free_mem;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 fhandler->full_addr.ctrl_base +
					 ctrl_8_offset, ctrl_8.data,
					 sizeof(ctrl_8.data));
	if (retval < 0) {
		TS_LOG_ERR
		    ("Failed to set enable f12,fhandler->full_addr.ctrl_base =%d, ctrl_8_offset = %d\n",
		     fhandler->full_addr.ctrl_base, ctrl_8_offset);
		goto error_free_mem;
	}

	/* Maximum x and y */
	rmi4_data->sensor_max_x =
	    ((unsigned short)ctrl_8.max_x_coord_lsb << 0) |
	    ((unsigned short)ctrl_8.max_x_coord_msb << 8);
	rmi4_data->sensor_max_y =
	    ((unsigned short)ctrl_8.max_y_coord_lsb << 0) |
	    ((unsigned short)ctrl_8.max_y_coord_msb << 8);
	TS_LOG_INFO("Function %02x max x = %d max y = %d\n",
		    fhandler->fn_number,
		    rmi4_data->sensor_max_x, rmi4_data->sensor_max_y);

	rmi4_data->num_of_rx = ctrl_8.num_of_rx;
	rmi4_data->num_of_tx = ctrl_8.num_of_tx;
	rmi4_data->max_touch_width = max(rmi4_data->num_of_rx,
					 rmi4_data->num_of_tx);

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num >= MAX_INTR_REGISTERS) {
		fhandler->intr_reg_num = 0;
		fhandler->num_of_data_sources = 0;
		fhandler->intr_mask = 0;
		retval = -EINVAL;
		goto error_free_mem;
	}
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
	     ii < ((fd->intr_src_count & MASK_3BIT) + intr_offset); ii++)
		fhandler->intr_mask |= 1 << ii;

	/* Allocate memory for finger data storage space */
	fhandler->data_size = num_of_fingers * size_of_2d_data;
	fhandler->data = kmalloc(fhandler->data_size, GFP_KERNEL);
	if (fhandler->data == NULL) {
		TS_LOG_ERR("Failed to alloc memory for fhandler->data\n");
		retval = -ENOMEM;
		goto error_free_mem;
	}

	return retval;
error_free_mem:
	if(fhandler->eratio_data != NULL)
	{
		kfree(fhandler->eratio_data);
		fhandler->eratio_data = NULL;
	}
eratio_data_error_free_mem:
	if(fhandler->extra != NULL)
	{
		kfree(fhandler->extra);
		fhandler->extra = NULL;
	}
	return retval;
}

static int synaptics_rmi4_f1a_alloc_mem(struct synaptics_rmi4_data *rmi4_data,
					struct synaptics_rmi4_fn *fhandler)
{
	int retval = NO_ERR;
	struct synaptics_rmi4_f1a_handle *f1a = NULL;

	f1a = kzalloc(sizeof(*f1a), GFP_KERNEL);
	if (!f1a) {
		TS_LOG_ERR("Failed to alloc mem for function handle\n");
		return -ENOMEM;
	}

	fhandler->data = (void *)f1a;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 fhandler->full_addr.query_base,
					 f1a->button_query.data,
					 sizeof(f1a->button_query.data));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read query registers\n");
		return retval;
	}

	f1a->button_count = f1a->button_query.max_button_count + 1;
	f1a->button_bitmask_size = (f1a->button_count + 7) / 8;

	f1a->button_data_buffer = kcalloc(f1a->button_bitmask_size,
					  sizeof(*(f1a->button_data_buffer)),
					  GFP_KERNEL);
	if (!f1a->button_data_buffer) {
		TS_LOG_ERR("Failed to alloc mem for data buffer\n");
		return -ENOMEM;
	}

	f1a->button_map = kcalloc(f1a->button_count,
				  sizeof(*(f1a->button_map)), GFP_KERNEL);
	if (!f1a->button_map) {
		TS_LOG_ERR("Failed to alloc mem for button map\n");
		return -ENOMEM;
	}

	return 0;
}

static int synaptics_rmi4_cap_button_map(struct synaptics_rmi4_data *rmi4_data,
					 struct synaptics_rmi4_fn *fhandler)
{
	return 0;
}

static void synaptics_rmi4_f1a_kfree(struct synaptics_rmi4_fn *fhandler)
{
	struct synaptics_rmi4_f1a_handle *f1a = fhandler->data;

	if (f1a) {
		if(f1a->button_data_buffer){
			kfree(f1a->button_data_buffer);
			f1a->button_data_buffer = NULL;
		}
		if(f1a->button_map){
			kfree(f1a->button_map);
			f1a->button_map = NULL;
		}
		kfree(f1a);
		f1a = NULL;
		fhandler->data = NULL;
	}

	return;
}

static int synaptics_rmi4_f1a_init(struct synaptics_rmi4_data *rmi4_data,
				   struct synaptics_rmi4_fn *fhandler,
				   struct synaptics_rmi4_fn_desc *fd,
				   unsigned int intr_count)
{
	int retval = NO_ERR;
	unsigned char ii = 0;
	unsigned short intr_offset = 0;

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;
	TS_LOG_DEBUG
	    ("fhandler->num_of_data_sources = %d, fhandler->fn_number = %d\n",
	     fhandler->num_of_data_sources, fhandler->fn_number);

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num >= MAX_INTR_REGISTERS) {
		fhandler->intr_reg_num = 0;
		fhandler->num_of_data_sources = 0;
		fhandler->intr_mask = 0;
		return -EINVAL;
	}
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
	     ii < ((fd->intr_src_count & MASK_3BIT) + intr_offset); ii++)
		fhandler->intr_mask |= 1 << ii;

	retval = synaptics_rmi4_f1a_alloc_mem(rmi4_data, fhandler);
	if (retval < 0) {
		TS_LOG_ERR("Failed to alloc memory for f1a, retval = %d\n",
			   retval);
		goto error_exit;
	}

	retval = synaptics_rmi4_cap_button_map(rmi4_data, fhandler);
	if (retval < 0)
		goto error_exit;

	rmi4_data->button_0d_enabled = 1;

	return 0;

error_exit:
	synaptics_rmi4_f1a_kfree(fhandler);

	return retval;
}

static int synaptics_rmi4_f51_init(struct synaptics_rmi4_data *rmi4_data,
				   struct synaptics_rmi4_fn *fhandler,
				   struct synaptics_rmi4_fn_desc *fd,
				   unsigned int intr_count, unsigned char page)
{
	unsigned char ii = 0;
	unsigned char intr_offset = 0;
	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;
	fhandler->data = NULL;
	fhandler->extra = NULL;
	fhandler->eratio_data = NULL;

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num >= MAX_INTR_REGISTERS) {
		fhandler->intr_reg_num = 0;
		fhandler->num_of_data_sources = 0;
		fhandler->intr_mask = 0;
		return -EINVAL;
	}
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
	     ii < ((fd->intr_src_count & MASK_3BIT) + intr_offset); ii++)
		fhandler->intr_mask |= 1 << ii;

	rmi4_data->rmi4_feature.f51_ctrl_base_addr =
	    fd->ctrl_base_addr | (page << 8);
	rmi4_data->rmi4_feature.f51_data_base_addr =
	    fd->data_base_addr | (page << 8);
	rmi4_data->rmi4_feature.f51_query_base_addr =
	    fd->query_base_addr | (page << 8);
	TS_LOG_DEBUG("f51 init , data_base_addr = 0x%04x\n"
		     "ctrl_base_addr = 0x%04x\n" "query_base_addr = 0x%04x\n"
		     "page = %d \n", rmi4_data->rmi4_feature.f51_ctrl_base_addr,
		     rmi4_data->rmi4_feature.f51_data_base_addr,
		     rmi4_data->rmi4_feature.f51_query_base_addr, page);

	return 0;
}

static int synaptics_input_config(struct input_dev *input_dev)
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

#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
#endif

	input_set_abs_params(input_dev, ABS_X,
			     0, rmi4_data->sensor_max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_Y,
			     0, rmi4_data->sensor_max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 15, 0, 0);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0,
			     rmi4_data->sensor_max_x_mt, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0,
			     rmi4_data->sensor_max_y_mt, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 100, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MINOR, 0, 100, 0, 0);

#ifdef REPORT_2D_W
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0,
			     MAX_ABS_MT_TOUCH_MAJOR, 0, 0);
	if(rmi4_data->synaptics_chip_data->fp_tp_report_touch_minor_event)
	{
		input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR, 0,
			     MAX_ABS_MT_TOUCH_MAJOR, 0, 0);
	}
#endif

#ifdef TYPE_B_PROTOCOL
#ifdef KERNEL_ABOVE_3_7
	/* input_mt_init_slots now has a "flags" parameter */
	input_mt_init_slots(input_dev, rmi4_data->num_of_fingers,
			    INPUT_MT_DIRECT);
#else
	input_mt_init_slots(input_dev, rmi4_data->num_of_fingers);
#endif
#endif

#ifdef RED_REMOTE
	/*used for red remote fucntion */
	rmi4_data->input_dev = input_dev;
#endif

	return NO_ERR;
}

static int easy_wakeup_gesture_report_coordinate(struct synaptics_rmi4_data
						 *rmi4_data,
						 unsigned int
						 reprot_gesture_point_num,
						 struct ts_fingers *info)
{
	int retval = 0;
	unsigned char get_custom_data[LOCUS_DATA_NUM * F12_FINGERS_TO_SUPPORT+4] = { 0 };
	int x = 0;
	int y = 0;
	int i = 0;
	unsigned short f51_data_base = 0;

	if (reprot_gesture_point_num != 0) {
		f51_data_base = rmi4_data->rmi4_feature.f51_data_base_addr;
		retval = synaptics_rmi4_i2c_read(rmi4_data,
						 f51_data_base,
						 get_custom_data,
						 (reprot_gesture_point_num *
						  LOCUS_DATA_LENS));
		if (retval < 0) {
			TS_LOG_ERR("%s:f51_data_base read error!retval = %d",
				   __func__, retval);
			return retval;
		}

		for (i = 0; i < reprot_gesture_point_num; i++) {
			/*
			 *F51_CUSTOM_DATA[00]~[23],every 4 bytes save 1 x/y position of the gesture locus;
			 *i.e.Point 1 has 4 bytes from [00] to [03] :
			 * [00] means LSB of x position,[01] means MSB of x position,
			 * [02] means LSB of y position,[03] means MSB of y position,
			 *The most points num is 6,point from 1(lower address) to 6(higher address) means:
			 *1.beginning 2.end 3.top 4.leftmost 5.bottom 6.rightmost
			 */
			x = ((get_custom_data[LOCUS_DATA_LENS * i + 1] << 8) |
			     (get_custom_data[LOCUS_DATA_LENS * i + 0]));
			y = ((get_custom_data[LOCUS_DATA_LENS * i + 3] << 8) |
			     (get_custom_data[LOCUS_DATA_LENS * i + 2]));
			if (SYNAPTICS_S3207 ==
			    rmi4_data->synaptics_chip_data->ic_type) {
				x = x * rmi4_data->synaptics_chip_data->x_max_mt /
				    rmi4_data->sensor_max_x;
				if (rmi4_data->synaptics_chip_data->has_virtualkey) {
					y = y * rmi4_data->synaptics_chip_data->lcd_full /
					    rmi4_data->sensor_max_y;
				} else {
					y = y * rmi4_data->synaptics_chip_data->y_max_mt /
					    rmi4_data->sensor_max_y;
				}
				if ((x > rmi4_data->synaptics_chip_data->x_max_mt)
				    || (x < 0)) {
					TS_LOG_ERR
					    ("x position over the size ,x = %d ",
					     x);
				}
				if ((y > rmi4_data->synaptics_chip_data->lcd_full)
				    || (y < 0)) {
					TS_LOG_ERR
					    ("x position over the size ,y = %d ",
					     y);
				}
			}

			TS_LOG_INFO("%s: Gesture Repot Point %d\n", __func__, i);
			rmi4_data->synaptics_chip_data->easy_wakeup_info.
			    easywake_position[i] = x << 16 | y;
			TS_LOG_INFO("easywake_position[%d] = 0x%04x\n", i,
				    rmi4_data->synaptics_chip_data->
				    easy_wakeup_info.easywake_position[i]);
		}
	}

	return retval;
}

static int synaptics_rmi4_key_gesture_report(struct synaptics_rmi4_data
					     *rmi4_data,
					     struct ts_fingers *info,
					     struct ts_easy_wakeup_info
					     *gesture_report_info,
					     unsigned char
					     *get_gesture_wakeup_data)
{
	int retval = 0;
	unsigned int reprot_gesture_key_value = 0;
	unsigned int reprot_gesture_point_num = 0;
	unsigned int easy_wakeup_gesture =
			(unsigned int)gesture_report_info->easy_wakeup_gesture;

	TS_LOG_INFO("get_gesture_wakeup_data is %d, 1 click 64 letter,\n",
		    get_gesture_wakeup_data[0]);

	switch (get_gesture_wakeup_data[0]) {
	case DOUBLE_CLICK_WAKEUP:
		if (IS_APP_ENABLE_GESTURE(GESTURE_DOUBLE_CLICK) &
			easy_wakeup_gesture) {
			TS_LOG_INFO("@@@DOUBLE_CLICK_WAKEUP detected!@@@\n");
			reprot_gesture_key_value = TS_DOUBLE_CLICK;
			LOG_JANK_D(JLID_TP_GESTURE_KEY, "JL_TP_GESTURE_KEY");
		}
		reprot_gesture_point_num = LINEAR_LOCUS_NUM;
		break;
	case LINEAR_SLIDE_DETECTED:
		TS_LOG_INFO("@@@LINEAR_SLIDE_DETECTED detected!@@@ \n");

		switch (get_gesture_wakeup_data[1]) {
		case LINEAR_SLIDE_LEFT_TO_RIGHT:
			if (IS_APP_ENABLE_GESTURE(GESTURE_SLIDE_L2R) &
				easy_wakeup_gesture) {
				TS_LOG_INFO
				    ("@@@LINEAR_SLIDE_LEFT_TO_RIGHT detected!@@@\n");
				reprot_gesture_key_value = TS_SLIDE_L2R;
				reprot_gesture_point_num = LINEAR_LOCUS_NUM;
			}
			break;
		case LINEAR_SLIDE_RIGHT_TO_LEFT:
			if (IS_APP_ENABLE_GESTURE(GESTURE_SLIDE_R2L) &
			    easy_wakeup_gesture) {
				TS_LOG_INFO
				    ("@@@LINEAR_SLIDE_RIGHT_TO_LEFT detected!@@@\n");
				reprot_gesture_key_value = TS_SLIDE_R2L;
				reprot_gesture_point_num = LINEAR_LOCUS_NUM;
			}
			break;
		case LINEAR_SLIDE_TOP_TO_BOTTOM:
			if (IS_APP_ENABLE_GESTURE(GESTURE_SLIDE_T2B) &
				easy_wakeup_gesture) {
				TS_LOG_INFO
				    ("@@@LINEAR_SLIDE_TOP_TO_BOTTOM detected!@@@\n");
				reprot_gesture_key_value = TS_SLIDE_T2B;
				reprot_gesture_point_num = LINEAR_LOCUS_NUM;
			}
			break;
		case LINEAR_SLIDE_BOTTOM_TO_TOP:
			if (IS_APP_ENABLE_GESTURE(GESTURE_SLIDE_B2T) &
				easy_wakeup_gesture) {
				TS_LOG_INFO
				    ("@@@LINEAR_SLIDE_BOTTOM_TO_TOP detected!@@@\n");
				reprot_gesture_key_value = TS_SLIDE_B2T;
				reprot_gesture_point_num = LINEAR_LOCUS_NUM;
			}
			break;
			/*case LINEAR_SLIDE_TOP_TO_BOTTOM2:
			   if (IS_APP_ENABLE_GESTURE(TS_SLIDE_T2B2) & gesture_report_info->easy_wakeup_gesture) {
			   TS_LOG_INFO("@@@LINEAR_SLIDE_TOP_TO_BOTTOM2 detected!@@@\n");
			   reprot_gesture_key_value = TS_SLIDE_T2B2;
			   reprot_gesture_point_num = LINEAR2_LOCUS_NUM;
			   }
			   break; */
		default:
			TS_LOG_INFO
			    ("@@@unknow LINEAR!f51_custom_data0 = %d@@@\n",
			     get_gesture_wakeup_data[0]);
			return 0;
		}
		break;
	case CIRCLE_SLIDE_DETECTED:
		if (IS_APP_ENABLE_GESTURE(GESTURE_CIRCLE_SLIDE) &
			easy_wakeup_gesture) {
			TS_LOG_INFO("@@@CIRCLE_SLIDE_DETECTED detected!@@@\n");
			reprot_gesture_key_value = TS_CIRCLE_SLIDE;
			reprot_gesture_point_num = LETTER_LOCUS_NUM;
		}
		break;
	case SPECIFIC_LETTER_DETECTED:
		TS_LOG_INFO("@@@SPECIFIC_LETTER_DETECTED detected!@@@\n");
		switch (get_gesture_wakeup_data[2]) {
		case SPECIFIC_LETTER_c:
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_c) &
				easy_wakeup_gesture) {
				TS_LOG_INFO
				    ("@@@SPECIFIC_LETTER_c detected!@@@\n");
				reprot_gesture_key_value = TS_LETTER_c;
				reprot_gesture_point_num = LETTER_LOCUS_NUM;
			}
			break;
		case SPECIFIC_LETTER_e:
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_e) &
				easy_wakeup_gesture) {
				TS_LOG_INFO
				    ("@@@SPECIFIC_LETTER_e detected!@@@\n");
				reprot_gesture_key_value = TS_LETTER_e;
				reprot_gesture_point_num = LETTER_LOCUS_NUM;
			}
			break;
		case SPECIFIC_LETTER_m:
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_m) &
				easy_wakeup_gesture) {
				TS_LOG_INFO
				    ("@@@SPECIFIC_LETTER_m detected!@@@\n");
				reprot_gesture_key_value = TS_LETTER_m;
				reprot_gesture_point_num = LETTER_LOCUS_NUM;
			}
			break;
		case SPECIFIC_LETTER_w:
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_w) &
				easy_wakeup_gesture) {
				TS_LOG_INFO
				    ("@@@SPECIFIC_LETTER_w detected!@@@\n");
				reprot_gesture_key_value = TS_LETTER_w;
				reprot_gesture_point_num = LETTER_LOCUS_NUM;
			}
			break;
		default:
			TS_LOG_INFO
			    ("@@@unknow letter!f11_2d_data39[6] = %d@@@\n",
			     get_gesture_wakeup_data[2]);
			return 0;
		}
		break;
	default:
		TS_LOG_INFO("@@@unknow gesture detected!\n");
		return 0;
	}

	if (0 != reprot_gesture_key_value) {
		/*increase wake_lock time to avoid system suspend.*/
		wake_lock_timeout(&rmi4_data->synaptics_chip_data->ts_platform_data->ts_wake_lock, 5 * HZ);
		mutex_lock(&wrong_touch_lock);
		if (true ==
		    rmi4_data->synaptics_chip_data->easy_wakeup_info.
		    off_motion_on) {
			rmi4_data->synaptics_chip_data->easy_wakeup_info.
			    off_motion_on = false;
			retval =
			    easy_wakeup_gesture_report_coordinate(rmi4_data,
								  reprot_gesture_point_num,
								  info);
			if (retval < 0) {
				mutex_unlock(&wrong_touch_lock);
				TS_LOG_INFO
				    ("%s: report line_coordinate error!retval = %d\n",
				     __func__, retval);
				return retval;
			}
			info->gesture_wakeup_value = reprot_gesture_key_value;
		}
		mutex_unlock(&wrong_touch_lock);
	}
	return NO_ERR;
}

static int synaptics_rmi4_gesture_report(struct synaptics_rmi4_data *rmi4_data,
					 struct synaptics_rmi4_fn *fhandler,
					 struct ts_fingers *info)
{
	int retval = NO_ERR;
	unsigned char device_data = 0;
	unsigned char f51_custom_data0 = 0;
	int wake_up_value_addr_offset = 0;
	unsigned short f12_data_base = 0;
	unsigned char f11_2d_data39[8] = { 0 };
	unsigned char get_gesture_wakeup_data[5] = { 0 };

	struct ts_easy_wakeup_info *gesture_report_info =
	    &rmi4_data->synaptics_chip_data->easy_wakeup_info;
	if (false == gesture_report_info->easy_wakeup_flag)
		return NO_ERR;
	if (SYNAPTICS_S3207 == rmi4_data->synaptics_chip_data->ic_type) {
		retval =
		    synaptics_rmi4_i2c_read(rmi4_data,
					    rmi4_data->rmi4_feature.
					    f11_data_base_addr +
					    F11_2D_DATA38_OFFSET, &device_data,
					    sizeof(device_data));
		if (retval < 0) {
			TS_LOG_ERR
			    ("F11_2D_DATA38_OFFSET read error, retval = %d\n",
			     retval);
			return 0;
		}
		TS_LOG_DEBUG("f11_data_base_addr = %d,device_data = %d",
			     rmi4_data->rmi4_feature.f11_data_base_addr,
			     device_data);
		get_gesture_wakeup_data[0] = device_data;
		retval =
		    synaptics_rmi4_i2c_read(rmi4_data,
					    cherry_guesture_pattern_addr,
					    &f51_custom_data0,
					    sizeof(f51_custom_data0));
		if (retval < 0) {
			TS_LOG_ERR("LINEAR_SLIDE_DETECTED error!retval = %d\n",
				   retval);
			return 0;
		}
		TS_LOG_DEBUG("cherry_guesture is linear, f51_custom_data0 = %d",
			     f51_custom_data0);
		get_gesture_wakeup_data[1] = f51_custom_data0;

		retval =
		    synaptics_rmi4_i2c_read(rmi4_data,
					    rmi4_data->rmi4_feature.
					    f11_data_base_addr +
					    F11_2D_DATA39_OFFSET,
					    &f11_2d_data39[0],
					    sizeof(f11_2d_data39));
		if (retval < 0) {
			TS_LOG_ERR("F11_2D_DATA39_OFFSET error!retval = %d\n",
				   retval);
			return 0;
		}
		TS_LOG_DEBUG
		    ("%s:###read### f11_2d_data39[7] = 0x%02x, f11_2d_data39[6] = 0x%02x\n",
		     __func__, f11_2d_data39[7], f11_2d_data39[6]);
		get_gesture_wakeup_data[2] = f11_2d_data39[6];
	} else {
		f12_data_base = rmi4_data->rmi4_feature.f12_data_base_addr;
		wake_up_value_addr_offset =
		    rmi4_data->rmi4_feature.geswakeup_feature.
		    f12_2d_data04_gesture;
		/*get gesture wake up value, read reg f12_2d_data04 */
		retval =
		    synaptics_rmi4_i2c_read(rmi4_data,
					    f12_data_base +
					    wake_up_value_addr_offset,
					    &get_gesture_wakeup_data[0],
					    sizeof(get_gesture_wakeup_data));
		if (retval < 0) {
			TS_LOG_ERR("%s read wake up value failed!\n", __func__);
			return retval;
		}
		/*read diff registers and get diff value, here we unite the value for 3320 and 3207 */
		if (DOUBLE_CLICK_S3320_NO == get_gesture_wakeup_data[0]) {
			get_gesture_wakeup_data[0] = DOUBLE_CLICK_WAKEUP;
		} else if (LINEAR_SLIDE_S3320_NO == get_gesture_wakeup_data[0]) {
			get_gesture_wakeup_data[0] = LINEAR_SLIDE_DETECTED;
		} else if (SPECIFIC_LETTER_S3320_NO ==
			   get_gesture_wakeup_data[0]) {
			get_gesture_wakeup_data[0] = SPECIFIC_LETTER_DETECTED;
		}
	}

	if (get_gesture_wakeup_data[0] != 0) {
		retval =
		    synaptics_rmi4_key_gesture_report(rmi4_data, info,
						      gesture_report_info,
						      get_gesture_wakeup_data);
		TS_LOG_DEBUG("%s:get_gesture_wakeup_data[0] value is %d!\n",
			     __func__, get_gesture_wakeup_data[0]);
		if (retval < 0) {
			TS_LOG_ERR
			    ("Failed to report gesture event!, retval = %d\n",
			     retval);
			return 1;
		}
	}
	return 0;
}

static int synaptics_rmi4_palm_sleep_report(struct synaptics_rmi4_data
					    *rmi4_data,
					    struct synaptics_rmi4_fn *fhandler,
					    struct ts_fingers *info)
{
	unsigned char palm_status = 0;
	unsigned short f12_data_base = 0;
	unsigned char object_type_and_status[80] = { 0 };
	int retval = 0;

	if (!rmi4_data->synaptics_chip_data->easy_wakeup_info.palm_cover_flag) {
		TS_LOG_DEBUG
		    (" If in easy_wake_up mode,no need to ack palm report, rs_flag = %d\n",
		     rmi4_data->synaptics_chip_data->easy_wakeup_info.
		     palm_cover_flag);
		return NO_ERR;
	}
	if (SYNAPTICS_S3207 == rmi4_data->synaptics_chip_data->ic_type) {
		retval =
		    synaptics_rmi4_i2c_read(rmi4_data, cherry_palm_status_addr,
					    &palm_status, sizeof(palm_status));
		if (retval < 0) {
			TS_LOG_ERR("read plam object_type_and_status fail !\n");
		} else {
			TS_LOG_DEBUG
			    ("read plam object_type_and_status OK object_type_and_status = 0x%04x\n",
			     palm_status);
		}

		if (PALM_COVER_SLEEP == (palm_status & 0x02)) {
			TS_LOG_INFO("%s:palm gesture detected!\n", __func__);
			info->gesture_wakeup_value = TS_PALM_COVERED;
			return PALM_COVER_SLEEP;
		} else {
			TS_LOG_DEBUG
			    ("%s:no palm_data!, palm_data read from ic is\n",
			     __func__);
		}

		return PALM_COVER_NOSLEEP;
	} else {
		f12_data_base = rmi4_data->rmi4_feature.f12_data_base_addr;
		retval =
		    synaptics_rmi4_i2c_read(rmi4_data, f12_data_base,
					    &object_type_and_status[0],
					    sizeof(object_type_and_status));
		if (retval < 0) {
			TS_LOG_ERR("read plam object_type_and_status fail !\n");
		} else {
			TS_LOG_DEBUG
			    ("read plam object_type_and_status OK object_type_and_status = 0x%04x\n",
			     object_type_and_status[0]);
		}

		/*get palm data report for sleep---------------------- */

		if (object_type_and_status[0] == PALM_COVER_SLEEP_S3320) {
			TS_LOG_DEBUG("%s:palm gesture detected!\n", __func__);
			info->gesture_wakeup_value = TS_PALM_COVERED;
			return 1;
		} else {
			TS_LOG_DEBUG
			    ("%s:no palm_data!, palm_data read from ic is %d\n",
			     __func__, object_type_and_status[0]);
			return NO_ERR;
		}
	}
	return NO_ERR;
}

 /**
 * synaptics_rmi4_f11_abs_report()
 *
 * Called by synaptics_rmi4_report_touch() when valid Function $11
 * finger data has been detected.
 *
 * This function reads the Function $11 data registers, determines the
 * status of each finger supported by the Function, processes any
 * necessary coordinate manipulation, reports the finger data to
 * the input subsystem, and returns the number of fingers detected.
 */
static void synaptics_rmi4_f11_abs_report(struct synaptics_rmi4_data *rmi4_data,
					  struct synaptics_rmi4_fn *fhandler,
					  struct ts_fingers *info)
{
	int retval = NO_ERR;
	unsigned char touch_count = 0;	/* number of touch points */
	unsigned char reg_index = 0;
	unsigned char finger = 0;
	unsigned char fingers_supported = 0;
	unsigned char num_of_finger_status_regs = 0;
	unsigned char finger_shift = 0;
	unsigned char finger_status = 0;
	unsigned char data_reg_blk_size = 0;
	unsigned char finger_status_reg[3] = { 0 };
	unsigned char data[F11_STD_DATA_LEN] = { 0 };
	unsigned short data_addr = 0;
	unsigned short data_offset = 0;
	int x = 0;
	int y = 0;
	int wx = 0;
	int wy = 0;
	int z = 0;
	/*
	 * The number of finger status registers is determined by the
	 * maximum number of fingers supported - 2 bits per finger. So
	 * the number of finger status registers to read is:
	 * register_count = ceil(max_num_of_fingers / 4)
	 */
	fingers_supported = fhandler->num_of_data_points;
	num_of_finger_status_regs = (fingers_supported + 3) / 4;
	data_addr = fhandler->full_addr.data_base;
	data_reg_blk_size = fhandler->size_of_data_register_block;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 data_addr,
					 finger_status_reg,
					 num_of_finger_status_regs);
	if (retval < 0) {
		TS_LOG_ERR("Failed to read: %d\n", retval);
		return;
	}
#ifdef GLOVE_SIGNAL
	retval = rmi_f11_read_finger_state(info);
	if (retval < 0) {
		TS_LOG_ERR("error: finger_state = %d\n", retval);
		return;
	}
#endif

	for (finger = 0; finger < fingers_supported; finger++) {
		reg_index = finger / 4;
		finger_shift = (finger % 4) * 2;
		finger_status = (finger_status_reg[reg_index] >> finger_shift)
		    & MASK_2BIT;

		/*
		 * Each 2-bit finger status field represents the following:
		 * 00 = finger not present
		 * 01 = finger present and data accurate
		 * 10 = finger present but data may be inaccurate
		 * 11 = reserved
		 */
		if (finger_status) {
			data_offset =
			    data_addr + num_of_finger_status_regs +
			    (finger * data_reg_blk_size);
			retval =
			    synaptics_rmi4_i2c_read(rmi4_data, data_offset,
						    data, data_reg_blk_size);
			if (retval < 0) {
				TS_LOG_ERR("Failed to read data report: %d\n",
					   retval);
				return;
			}

			x = (data[0] << 4) | (data[2] & MASK_4BIT);
			y = (data[1] << 4) | ((data[2] >> 4) & MASK_4BIT);
			wx = (data[3] & MASK_4BIT);
			wy = (data[3] >> 4) & MASK_4BIT;
			z = data[4];

			if (rmi4_data->flip_x)
				x = rmi4_data->sensor_max_x - x;
			if (rmi4_data->flip_y)
				y = rmi4_data->sensor_max_y - y;

			x = x * rmi4_data->synaptics_chip_data->x_max_mt /
			    rmi4_data->sensor_max_x;
			if (0 == rmi4_data->synaptics_chip_data->has_virtualkey) {
				y = y * rmi4_data->synaptics_chip_data->y_max_mt /
				    rmi4_data->sensor_max_y;
			} else {
				y = y * rmi4_data->synaptics_chip_data->lcd_full /
				    rmi4_data->sensor_max_y;
			}

			TS_LOG_DEBUG("Finger %d:status = 0x%02x,wx = %d,wy = %d", finger, finger_status,wx, wy);

			info->fingers[finger].x = x;
			info->fingers[finger].y = y;
			info->fingers[finger].ewx = wx;
			info->fingers[finger].ewy = wy;
			info->fingers[finger].pressure = z;

			touch_count++;
		}
	}
	info->cur_finger_number = touch_count;
	TS_LOG_DEBUG("f11_abs_report, touch_count = %d\n", touch_count);
	return;
}

 /**
 * synaptics_rmi4_f12_abs_report()
 *
 * Called by synaptics_rmi4_report_touch() when valid Function $12
 * finger data has been detected.
 *
 * This function reads the Function $12 data registers, determines the
 * status of each finger supported by the Function, processes any
 * necessary coordinate manipulation, reports the finger data to
 * the input subsystem, and returns the number of fingers detected.
 */
static void synaptics_rmi4_f12_abs_report(struct synaptics_rmi4_data *rmi4_data,
					  struct synaptics_rmi4_fn *fhandler,
					  struct ts_fingers *info)
{
	int retval = 0;
	unsigned char touch_count = 0;	/* number of touch points */
	unsigned char finger =0;
	unsigned char fingers_to_process= 0;
	unsigned char finger_status = 0;
	unsigned char size_of_2d_data = 0;
	unsigned char size_of_query8 = 0;
	unsigned char size_of_eratio_data = 0;
	unsigned short data_addr = 0;
	unsigned short temp_finger_status = 0;
	unsigned char grip_data[GRIP_DATA_NUM] = {0};
	int x = 0;
	int y = 0;
	int wx = 0;
	int wy = 0;
	int ewx = 0;
	int ewy = 0;
	int xer = 0;
	int yer = 0;
	struct synaptics_grip_data *finger_grip_data = NULL;
	int grip_data_flag = 0;
	int sg = 0;
	int temp_wx = 0;
	int temp_wy = 0;
	int temp_sg = 0; //syna_wx_wy
#ifdef USE_F12_DATA_15
	int temp = 0;
#endif
#ifdef F12_DATA_15_WORKAROUND
	static unsigned char fingers_already_present = 0;
#endif
	int z = 1;
#ifdef SYNA_FORCE
	unsigned char force_level = 0;
#endif
	struct synaptics_rmi4_f12_extra_data *extra_data = NULL;
	struct synaptics_rmi4_f12_finger_data *data = NULL;
	struct synaptics_rmi4_f12_finger_data *finger_data = NULL;
	struct synaptics_rmi4_f12_eratio_data *eratio_data = NULL;
	struct synaptics_rmi4_f12_eratio_data *finger_ratio_data = NULL;
	struct synaptics_rmi4_f12_query_8 query_8;
	if((unsigned char *)fhandler == NULL)
	{
		TS_LOG_ERR("fhandler is null return\n");
		return;
	}
	if ( (unsigned char *)fhandler->eratio_data == NULL)
	{
		TS_LOG_ERR("fhandler eratio data is null return\n");
		return;
	}
	current_finger_num = 0;
	fingers_to_process = fhandler->num_of_data_points;
	data_addr = fhandler->full_addr.data_base;
	extra_data = (struct synaptics_rmi4_f12_extra_data *)fhandler->extra;
	size_of_2d_data = sizeof(struct synaptics_rmi4_f12_finger_data);
	size_of_eratio_data = sizeof(struct synaptics_rmi4_f12_eratio_data);

#ifdef USE_F12_DATA_15
	/* Determine the total number of fingers to process */
	if (extra_data->data15_size) {
		retval = synaptics_rmi4_i2c_read(rmi4_data,
						 data_addr +
						 extra_data->data15_offset,
						 extra_data->data15_data,
						 extra_data->data15_size);
		if (retval < 0) {
			TS_LOG_ERR("Failed to read data status: %d\n", retval);
			return;
		}

		/* Start checking from the highest bit */
		temp = extra_data->data15_size - 1;	/* Highest byte */
		finger = (fingers_to_process - 1) % 8;	/* Highest bit */
		do {
			if (extra_data->data15_data[temp] & (1 << finger))
				break;

			if (finger) {
				finger--;
			} else {
				temp--;	/* Move to the next lower byte */
				finger = 7;
			}

			fingers_to_process--;
		} while (fingers_to_process);

		TS_LOG_DEBUG("Number of fingers to process = %d\n",
			    fingers_to_process);
	}

#ifdef F12_DATA_15_WORKAROUND
	fingers_to_process = max(fingers_to_process, fingers_already_present);
#endif
	if (!fingers_to_process) {
		TS_LOG_DEBUG("fingers to process is 0\n");
		return;
	}
#endif
	if(fingers_to_process > F12_FINGERS_TO_SUPPORT){
		TS_LOG_ERR("fingers to process = %d.\n",fingers_to_process);
		fingers_to_process = F12_FINGERS_TO_SUPPORT;
	}
	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 data_addr + extra_data->data1_offset,
					 (unsigned char *)fhandler->data,
					 fingers_to_process * size_of_2d_data);
	if (retval < 0) {
		TS_LOG_ERR("Failed to read: %d, read data offset is %d \n",
			   retval, extra_data->data1_offset);
		return;
	}
	if (rmi4_data->is_multi_protocal == MULTI_PROTOCAL_2)
	{
		retval = synaptics_rmi4_i2c_read(rmi4_data,
							 fhandler->full_addr.query_base + 7,
							 &size_of_query8,
							 sizeof(size_of_query8));
		if (retval < 0) {
			TS_LOG_ERR
				("Failed to read f12 ->full_addr.query_base = %d,here is +7\n",
				 fhandler->full_addr.query_base);
			return;
		}
		if(size_of_query8 > SYNAPTICS_RMI4_F12_QUERY_8_MAX){
			size_of_query8 = SYNAPTICS_RMI4_F12_QUERY_8_MAX;
			TS_LOG_ERR("size_of_query8 too big = %d\n",size_of_query8);
		}
		retval = synaptics_rmi4_i2c_read(rmi4_data,
						 fhandler->full_addr.query_base + 8,
						 query_8.data, size_of_query8);
		if (retval < 0) {
			TS_LOG_ERR
			    ("Failed to read f12 ->full_addr.query_base = %d,here is +8\n",
			     fhandler->full_addr.query_base);
			return;
		}

		if(query_8.data36_is_present)
		{
			retval = synaptics_rmi4_i2c_read(rmi4_data,
							 data_addr + extra_data->data36_offset,
							 (unsigned char *)fhandler->eratio_data,
							 fingers_to_process * size_of_eratio_data);
			if (retval < 0) {
				TS_LOG_ERR("Failed to read eratio data: %d, read data offset is %d \n",
					   retval, extra_data->data1_offset);
				return;
			}
		}
	}
	if (rmi4_data->synaptics_chip_data->support_aft) {
		retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->synaptics_chip_data->aft_data_addr,
			grip_data, GRIP_DATA_NUM);
		if (retval < 0) {
			grip_data_flag = !grip_data_flag;
			TS_LOG_ERR("read grip_data error, ret = %d\n", retval);
		}
	}

	data = (struct synaptics_rmi4_f12_finger_data *)fhandler->data;
	eratio_data = (struct synaptics_rmi4_f12_eratio_data *)fhandler->eratio_data;
	TS_LOG_DEBUG("Number of fingers = %d\n", fingers_to_process);

	for (finger = 0; finger < fingers_to_process; finger++) {
		finger_data = data + finger;
		finger_status = finger_data->object_type_and_status;
		finger_ratio_data = eratio_data + finger;
		if (rmi4_data->synaptics_chip_data->support_aft &&
			finger < MAX_FINGER_NUM &&
			!grip_data_flag) {
			finger_grip_data = (struct synaptics_grip_data *)grip_data + finger;
		}

		/*
		 * Each 2-bit finger status field represents the following:
		 * 00 = finger not present
		 * 01 = finger present and data accurate
		 * 10 = finger present but data may be inaccurate
		 * 11 = reserved
		 */

		if (finger_status) {
 #ifdef F12_DATA_15_WORKAROUND
			fingers_already_present = finger + 1;
#endif
			x = (finger_data->x_msb << 8) | (finger_data->x_lsb);/* Convert 16bit number*/
			y = (finger_data->y_msb << 8) | (finger_data->y_lsb);/* Convert 16bit number*/
#ifdef REPORT_2D_W
			/* syna_wx_wy */

			if (rmi4_data->is_multi_protocal == MULTI_PROTOCAL_1
				&& rmi4_data->new_wx_wy) {
				temp_wx = ((finger_data->wx) & MASK_4BIT);
				temp_wy = ((finger_data->wx >> 4) & MASK_4BIT);/*get hight 4bit number*/
				temp_sg = finger_data->wy;
				wx = temp_wx;
				wy = temp_wy;
				sg = temp_sg;
				TS_LOG_DEBUG("new protocol\n");
			} else if (rmi4_data->is_multi_protocal == MULTI_PROTOCAL_2) {
				TS_LOG_DEBUG("new protocol 2\n");
				ewx = ((finger_data->new_ew) & MASK_4BIT);
				ewy = ((finger_data->new_ew>> 4) & MASK_4BIT);/*get hight 4bit number*/
				wx = ((finger_data->new_w) & MASK_4BIT);
				wy = ((finger_data->new_w >> 4) & MASK_4BIT);/*get hight 4bit number*/
				xer = finger_ratio_data->eratio_y;
				yer = finger_ratio_data->eratio_x;
			} else {
				wx = finger_data->wx;
				wy = finger_data->wy;
				TS_LOG_DEBUG("old protocol\n");
			}
#endif

#ifdef REPORT_2D_Z
			z = finger_data->z;
#endif
#ifdef SYNA_FORCE

			if (rmi4_data->synaptics_chip_data->support_3d_func) {
				TS_LOG_DEBUG("Read force value for press\n");
				retval = synaptics_rmi4_i2c_read(rmi4_data,0x041a, &force_level, 1); /*Press force register addr*/
			if (retval < 0) {
				TS_LOG_ERR("Failed to read data status: %d\n",retval);
				force_level = 1;
			}
			z = force_level;
			if (!force_level)
				z = 1;
	}

#endif

			if (!rmi4_data->flip_x)
				x = rmi4_data->sensor_max_x - x;
			if (!rmi4_data->flip_y)
				y = rmi4_data->sensor_max_y - y;

			if (1 == rmi4_data->synaptics_chip_data->has_virtualkey) {
				x = x * rmi4_data->synaptics_chip_data->x_max_mt /
				    rmi4_data->sensor_max_x;
				y = y * rmi4_data->synaptics_chip_data->lcd_full /
				    rmi4_data->sensor_max_y;
			}

			TS_LOG_DEBUG("Finger %d:status = 0x%02x,wx = %d, wy = %d,sg = %d,z = %d\n",
				finger, finger_status, wx, wy, sg, z);
			info->fingers[finger].status = finger_status;
			info->fingers[finger].x = x;
			info->fingers[finger].y = y;
			info->fingers[finger].sg = sg;
			info->fingers[finger].pressure = z;
			if (rmi4_data->synaptics_chip_data->support_aft && !grip_data_flag) {
				info->fingers[finger].wx = wx;
				info->fingers[finger].wy = wy;
				info->fingers[finger].ewx = finger_grip_data->ewx;
				info->fingers[finger].ewy = finger_grip_data->ewy;
				info->fingers[finger].xer = finger_grip_data->xer;
				info->fingers[finger].yer = finger_grip_data->yer;
				TS_LOG_DEBUG("grip data: ewx = %d, ewy = %d, xer = %d, yer = %d\n",
						finger_grip_data->ewx,
						finger_grip_data->ewy,
						finger_grip_data->xer,
						finger_grip_data->yer);
			} else {
				if (rmi4_data->is_multi_protocal == MULTI_PROTOCAL_2) {
					info->fingers[finger].xer = xer ;
					info->fingers[finger].yer = yer ;
					info->fingers[finger].ewx = ewx;
					info->fingers[finger].ewy = ewy;
					info->fingers[finger].wx = wx;
					info->fingers[finger].wy = wy;
					if(rmi4_data->synaptics_chip_data->fp_tp_report_touch_minor_event)
					{
						info->fingers[finger].major = 1;
						info->fingers[finger].minor = 1;
					}
					TS_LOG_DEBUG("xer = %d|yer = %d|ewx = %d|ewy = %d|wx = %d|wy= %d\n",
										 info->fingers[finger].xer,
										info->fingers[finger].yer,
										info->fingers[finger].ewx,
										info->fingers[finger].ewy ,
										info->fingers[finger].wx ,
										info->fingers[finger].wy );
				}
				else
				{
					info->fingers[finger].ewx = wx;
					info->fingers[finger].ewy = wy;
				}
			}
			touch_count++;
			temp_finger_status |= (1 << finger);
		}
	}
	info->cur_finger_number = touch_count;
	current_finger_num = touch_count;
#ifdef F12_DATA_15_WORKAROUND
	if (touch_count == 0) {
		fingers_already_present = 0;
	}
#endif
	TS_LOG_DEBUG("pre_finger_status=%d, temp_finger_status=%d\n",
		     pre_finger_status, temp_finger_status);
	if (f51_roi_switch) {
		if ((temp_finger_status != pre_finger_status
		     && ((temp_finger_status & pre_finger_status) !=
			 temp_finger_status))) {
			/* roi_data shoud be refreshed. But that takes too long (about 2ms on i2c bus), we delay
			   this work to function synaptics_work_after_input. It will be called after input report. */
			roi_data_staled = 1;
		}
	}
	pre_finger_status = temp_finger_status;

	TS_LOG_DEBUG("f12_abs_report, touch_count = %d\n", touch_count);
	return;
}

static void synaptics_rmi4_f51_report(struct synaptics_rmi4_data *rmi4_data,
				      struct synaptics_rmi4_fn *fhandler,
				      struct ts_fingers *info)
{
	int retval = 0;
	unsigned short ctrl_base_addr = 0;
	unsigned short data_addr = 0;
	unsigned short touchplus_offset = 0;
	unsigned char touchplus_data = 0;
	unsigned char button_number = 0;
	unsigned char button_flag = 0;

	ctrl_base_addr = rmi4_data->rmi4_feature.f51_ctrl_base_addr;
	if (!strncmp
	    (rmi4_data->rmi4_mod_info.product_id_string, "S3320",
	     strlen("S3320")))
		touchplus_offset = S3320_F51_TOUCHPLUS_OFFSET;	/*S3320 touchplus addr : 0x0422 */
	else
		touchplus_offset = S3350_F51_TOUCHPLUS_OFFSET;	/*S3350 touchplus addr : 0x040E */

	data_addr = ctrl_base_addr + touchplus_offset;
	TS_LOG_DEBUG("%s : data_addr = 0x%02x \n", __func__, data_addr);
	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 data_addr,
					 &touchplus_data,
					 sizeof(touchplus_data));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read: %d, read ctrl_base_addr is %d\n",
			   retval, ctrl_base_addr);
		return;
	}

	button_flag = touchplus_data & TOUCHPLUS_DOWNUP_BIT04;
	info->special_button_flag = !!button_flag;
	TS_LOG_DEBUG("%s : special_button_flag = %d\n", __func__,
		     info->special_button_flag);

	button_number = touchplus_data & 0x0F;
	TS_LOG_DEBUG("%s : button_number = %d\n", __func__, button_number);
	switch (button_number) {
	case 0:
		TS_LOG_INFO("get button %d\n", button_number);
		info->special_button_key = TS_TOUCHPLUS_KEY0;	/*KEY_F21 */
		break;
	case 1:
		TS_LOG_INFO("get button %d\n", button_number);
		info->special_button_key = TS_TOUCHPLUS_KEY1;	/*KEY_F22 */
		break;
	case 2:
		TS_LOG_INFO("get button %d\n", button_number);
		info->special_button_key = TS_TOUCHPLUS_KEY2;	/*KEY_F23 */
		break;
	case 3:
		TS_LOG_INFO("get button %d\n", button_number);
		info->special_button_key = TS_TOUCHPLUS_KEY3;	/*KEY_F19 */
		break;
	case 4:
		TS_LOG_INFO("get button %d\n", button_number);
		info->special_button_key = TS_TOUCHPLUS_KEY4;	/*KEY_F20 */
		break;
	default:
		TS_LOG_ERR("the get_button_number is invald : %d\n",
			   button_number);
		return;
	}

	return;
}

 /**
 * synaptics_rmi4_report_touch()
 *
 * Called by synaptics_rmi4_sensor_report().
 *
 * This function calls the appropriate finger data reporting function
 * based on the function handler it receives and returns the number of
 * fingers detected.
 */
static void synaptics_rmi4_report_touch(struct synaptics_rmi4_data *rmi4_data,
					struct synaptics_rmi4_fn *fhandler,
					struct ts_fingers *info)
{
	int retval = 0;
	TS_LOG_DEBUG("Function %02x reporting\n", fhandler->fn_number);
	retval = synaptics_rmi4_gesture_report(rmi4_data, fhandler, info);
	if (true == retval) {
		TS_LOG_DEBUG
		    ("synaptics_rmi4_gesture_report is called and report gesture\n");
		return;
	}
	retval = synaptics_rmi4_palm_sleep_report(rmi4_data, fhandler, info);
	if (true == retval) {
		TS_LOG_DEBUG
		    ("synaptics_rmi4_palm_sleep_report is called and report gesture\n");
		return;
	}
	switch (fhandler->fn_number) {
	case SYNAPTICS_RMI4_F11:
		synaptics_rmi4_f11_abs_report(rmi4_data, fhandler, info);
		break;
	case SYNAPTICS_RMI4_F12:
		synaptics_rmi4_f12_abs_report(rmi4_data, fhandler, info);
		break;
	case SYNAPTICS_RMI4_F51:
		synaptics_rmi4_f51_report(rmi4_data, fhandler, info);
		break;

	case SYNAPTICS_RMI4_F1A:
		/*synaptics_rmi4_f1a_report(rmi4_data, fhandler);*/
		break;

	default:
		break;
	}
	return;
}

 /**
 * synaptics_rmi4_sensor_report()
 *
 * Called by synaptics_rmi4_irq().
 *
 * This function determines the interrupt source(s) from the sensor
 * and calls synaptics_rmi4_report_touch() with the appropriate
 * function handler for each function with valid data inputs.
 */
static int synaptics_rmi4_sensor_report(struct synaptics_rmi4_data *rmi4_data,
					struct ts_fingers *info)
{
	int retval = NO_ERR;
	unsigned char intr[MAX_INTR_REGISTERS] = { 0 };
	struct synaptics_rmi4_fn *fhandler = NULL;
	struct synaptics_rmi4_device_info *rmi = NULL;

	rmi = &(rmi4_data->rmi4_mod_info);

	/*
	 * Get interrupt status information from F01 Data1 register to
	 * determine the source(s) that are flagging the interrupt.
	 */
	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 rmi4_data->rmi4_feature.
					 f01_data_base_addr + 1, intr,
					 rmi4_data->num_of_intr_regs);
	if (retval < 0) {
		g_report_err_log_count++;

		if (g_report_err_log_count > 30) {
			g_report_err_log_count = 30;
			TS_LOG_DEBUG
				("get interrupts status information failed, retval = %d\n",
				 retval);
		} else {
			TS_LOG_ERR
				("get interrupts status information failed, retval = %d\n",
				 retval);
		}

		return retval;
	}

	/*
	 * Traverse the function handler list and service the source(s)
	 * of the interrupt accordingly.
	 */
	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->num_of_data_sources) {
				if (fhandler->
				    intr_mask & intr[fhandler->intr_reg_num]) {
					synaptics_rmi4_report_touch(rmi4_data,
								    fhandler,
								    info);
				}
			}
		}
	}
	return NO_ERR;
}

static int synaptics_irq_top_half(struct ts_cmd_node *cmd)
{
	cmd->command = TS_INT_PROCESS;
	TS_LOG_DEBUG("synaptics irq top half called\n");
	return NO_ERR;
}

static int synaptics_irq_bottom_half(struct ts_cmd_node *in_cmd,
				     struct ts_cmd_node *out_cmd)
{
	int retval = NO_ERR;
	struct ts_fingers *info =
	    &out_cmd->cmd_param.pub_params.algo_param.info;

	synaptics_interrupt_num++;
#ifdef RED_REMOTE
	/*This interrupts is used for redremote, not report */
	if (rmi4_data->fw_debug == true) {
		out_cmd->command = TS_INVAILD_CMD;
		synap_rmidev_sysfs_irq(rmi4_data);
		return NO_ERR;
	}
#endif

	out_cmd->command = TS_INPUT_ALGO;
	out_cmd->cmd_param.pub_params.algo_param.algo_order =
	    rmi4_data->synaptics_chip_data->algo_id;
	TS_LOG_DEBUG("order: %d\n",
		     out_cmd->cmd_param.pub_params.algo_param.algo_order);

	retval = synaptics_rmi4_sensor_report(rmi4_data, info);
	if (retval < 0) {
		if (g_report_err_log_count < 30) {
			TS_LOG_ERR("synaptics_rmi4_sensor_report, error: %d\n", retval);
		} else {
			TS_LOG_DEBUG("synaptics_rmi4_sensor_report, error: %d\n", retval);
		}
		return retval;
	}

	return NO_ERR;
}

static int __init synaptics_ts_module_init(void)
{
    bool found = false;
    struct device_node* child = NULL;
    struct device_node* root = NULL;
    int error = NO_ERR;

    TS_LOG_INFO(" synaptics_ts_module_init called here\n");
    //memset(&synaptics_device_data, 0, sizeof(struct ts_kit_device_data));
    rmi4_data = kzalloc(sizeof(*rmi4_data) * 2, GFP_KERNEL);
    if (!rmi4_data) {
    	TS_LOG_ERR("Failed to alloc mem for struct rmi4_data\n");
       error =  -ENOMEM;
       return error;
    }
    rmi4_data->synaptics_chip_data = kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
    if (!rmi4_data->synaptics_chip_data) {
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
        if (of_device_is_compatible(child, SYNAPTICS_VENDER_NAME))
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
	TS_LOG_ERR(" not found chip synaptics child node  !\n");
        error = -EINVAL;
        goto out;
    }

    rmi4_data->synaptics_chip_data->cnode = child;
    rmi4_data->synaptics_chip_data->ops = &ts_kit_synaptics_ops;
    rmi4_data->synaptics_chip_data->vendor_name = SYNA_VENDOR_NAME;
    error = huawei_ts_chip_register(rmi4_data->synaptics_chip_data);
    if(error)
    {
	  TS_LOG_ERR(" synaptics chip register fail !\n");
	  goto out;
    }
    TS_LOG_INFO("synaptics chip_register! err=%d\n", error);
    return error;
out:
    if(rmi4_data->synaptics_chip_data)
	kfree(rmi4_data->synaptics_chip_data);
    if (rmi4_data)
	kfree(rmi4_data);
	rmi4_data = NULL;
    return error;
}

static void __exit synaptics_ts_module_exit(void)
{

   TS_LOG_INFO("synaptics_ts_module_exit called here\n");

    return;
}

module_init(synaptics_ts_module_init);
module_exit(synaptics_ts_module_exit);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");
