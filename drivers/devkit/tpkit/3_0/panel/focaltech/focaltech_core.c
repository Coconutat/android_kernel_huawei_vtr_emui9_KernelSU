

#include "focaltech_flash.h"
#include "focaltech_core.h"
#include "focaltech_test.h"
#include "focaltech_dts.h"
#include "focaltech_test.h"
#include "focaltech_apk_node.h"
#include <linux/printk.h>
#include <linux/time.h>

#include "../../huawei_ts_kit.h"
#include <huawei_platform/log/log_jank.h>
#if defined(CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif
#if defined (CONFIG_TEE_TUI)
#include "tui.h"
#endif

/*Gesture register(0xd0) value*/
#define DOUBLE_CLICK_WAKEUP  		(0x24)
#define SPECIFIC_LETTER_W  			(0x31)
#define SPECIFIC_LETTER_M			(0x32)
#define SPECIFIC_LETTER_E			(0x33)
#define SPECIFIC_LETTER_C 			(0x34)
#define LETTER_LOCUS_NUM 6
#define LINEAR_LOCUS_NUM 2
#define IS_APP_ENABLE_GESTURE(x)  ((u32)(1<<x))

#define FTS_MAX_POINT_ID		0x0F
#define FTS_POINT_DATA_SIZE		6
#define FTS_EDGE_POINT_DATA_SIZE	5
#define FTS_FP_EDGE_POINT_DATA_SIZE	7
#define FTS_RESUME_MAX_TIMES      10
#define FTS_ESD_MAX_TIMES      3
#define FTS_CHECK_FLOW_COUNT	5
#define TOUCH_DATA_START_ADDR		0x00
#define TOUCH_DATA_START_ADDR_SPI   0x71

#define ADDR_X_H_POS			(3 - TOUCH_DATA_START_ADDR)
#define ADDR_X_L_POS			(4 - TOUCH_DATA_START_ADDR)
#define ADDR_Y_H_POS			(5 - TOUCH_DATA_START_ADDR)
#define ADDR_Y_L_POS			(6 - TOUCH_DATA_START_ADDR)
#define ADDR_EVENT_POS			(3 - TOUCH_DATA_START_ADDR)
#define ADDR_FINGER_ID_POS		(5 - TOUCH_DATA_START_ADDR)
#define ADDR_POINT_NUM			(2 - TOUCH_DATA_START_ADDR)
#define ADDR_PALM_NUM           (1 - TOUCH_DATA_START_ADDR)
#define ADDR_XY_POS			(7 - TOUCH_DATA_START_ADDR)
#define ADDR_MISC			(8 - TOUCH_DATA_START_ADDR)
#define ADDR_EDGE_EWX			1
#define ADDR_EDGE_EWY			3
#define ADDR_EDGE_YER			2
#define ADDR_EDGE_XER			4
#define ADDR_EDGE_WX			5
#define ADDR_EDGE_WY			6
#define FTS_TOUCH_DATA_LEN		\
	(3 - TOUCH_DATA_START_ADDR + FTS_POINT_DATA_SIZE * FTS_MAX_TOUCH_POINTS)
#define FTS_REPORT_DATA_ALL_LEN \
    (FTS_TOUCH_DATA_LEN + 2 + FTS_FP_EDGE_TOUCH_DATA_LEN + 2 + ROI_DATA_READ_LENGTH)
#define FTS_EDGE_DATA_OFF   0x40
#define FTS_ROI_DATA_OFF    0x88

#define PRINT_BULK_MAX_WIDTH 64
#define PRINT_BULK_MIN_WIDTH 8
#define PRINT_BULK_BUF_LEN (PRINT_BULK_MAX_WIDTH * 4)
#define PRINT_DIFF_BUF_LEN (PRINT_BULK_MAX_WIDTH * 8)
#define PRINT_BULK_MAX_LEN 4096
#define FTS_DIFF_DATA_SIZE 1296
#define FTS_DIFF_DATA_WIDTH 36
#define FTS_DIFF_DATA_HEIGHT 18
#define FTS_DBG_DATA_SIZE 32
#define ADDR_DBG_DATA (0xEE + 1)
#define ADDR_DIFF_DATA (0x10E + 1)
#define ADDR_FINGER_DOWN (0x86 + 1)
#define ADDR_FINGER_ALLUP ADDR_POINT_NUM
#define FTS_GET_DBG_ON  1
#define FTS_GET_DBG_OFF 0
#define FTS_ALL_FIGURE_UP 0
#define FTS_FIGURE_DOWN 1
#define focal_print_bulk(buf, len, width) _focal_print_bulk(__func__, buf, len, width)

#define U8_STR_LEN			5
#ifndef NULL
#define NULL 0
#endif

#if defined (CONFIG_TEE_TUI)
extern struct ts_tui_data tee_tui_data;
#endif

struct focal_platform_data *g_focal_pdata = NULL;
struct ts_kit_device_data *g_focal_dev_data = NULL;
struct fts_esdcheck_st fts_esdcheck_data;
static struct mutex wrong_touch_lock;
extern struct ts_kit_platform_data g_ts_kit_platform_data;
extern int ts_kit_get_pt_station_status(int *status);
extern u8 g_ts_kit_log_cfg;
unsigned char focal_roi_data[ROI_DATA_READ_LENGTH] = { 0 };//roi data buff
static int focal_wrong_touch(void);
static int focal_init_chip(void);
static int focal_fw_update_sd(void);
static int focal_before_suspend(void);
static int focal_suspend(void);
static int focal_resume(void);
static void focal_shutdown(void);
static int focal_reset_device(void);
static u8 *focal_roi_rawdata(void);
static int focal_calibrate(void);
static int focal_calibrate_wakeup_gesture(void);

#if defined(HUAWEI_CHARGER_FB)
static int focal_charger_switch(struct ts_charger_info *info);
#endif

static int focal_input_config(struct input_dev *input_dev);
static int focal_palm_switch(struct ts_palm_info *info);
static int focal_glove_switch(struct ts_glove_info *info);
static int focal_get_glove_switch(u8 *glove_switch);
static int focal_set_glove_switch(u8 glove_switch);
static int focal_holster_switch(struct ts_holster_info *info);
static int focal_roi_switch(struct ts_roi_info *info);
static int focal_regs_operate(struct ts_regs_info *info);
static int focal_chip_detect(struct ts_kit_platform_data *data);
static int focal_irq_top_half(struct ts_cmd_node *cmd);
static int focal_fw_update_boot(char *file_name);
static int focal_fw_update_boot_resume(void);
static int focal_fw_update_boot_recovery(void);
static int focal_chip_get_info(struct ts_chip_info_param *info);
static int focal_set_info_flag(struct ts_kit_platform_data *info);
static int focal_after_resume(void *feature_info);
static int focal_wakeup_gesture_enable_switch(
	struct ts_wakeup_gesture_enable_info *info);
static int focal_irq_bottom_half(struct ts_cmd_node *in_cmd,
	struct ts_cmd_node *out_cmd);
static int focal_set_glove_switch(u8 glove_switch);
static int focal_set_holster_switch(u8 holster_switch);
static int focal_set_roi_switch(u8 roi_switch);
static int focal_status_resume(void);
static int focal_get_brightness_info(void);
static int focal_esdcheck_func(void);
static int focal_power_on(void);
static void focal_power_off(void);
static void focal_chip_touch_switch(void);
static int focal_get_raw_data_all(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd);
static int rawdata_proc_focal_printf(struct seq_file *m, struct ts_rawdata_info *info,
					int range_size, int row_size);
static void focal_pinctrl_select_normal(void);
static void focal_pinctrl_select_suspend(void);

static void focal_irq_output(int level);
static int focal_reset_output(int level);
static int focal_cs_output(int level);



extern void focal_param_kree(void);

struct ts_device_ops ts_focal_ops = {
	.chip_detect = focal_chip_detect,
	.chip_init = focal_init_chip,
	.chip_get_brightness_info = focal_get_brightness_info,
	.chip_input_config = focal_input_config,
	.chip_irq_top_half = focal_irq_top_half,
	.chip_irq_bottom_half = focal_irq_bottom_half,
	.chip_fw_update_boot = focal_fw_update_boot,
	.chip_fw_update_sd = focal_fw_update_sd,
	.chip_get_info = focal_chip_get_info,
	.chip_get_capacitance_test_type = focal_chip_get_capacitance_test_type,
	.chip_set_info_flag = focal_set_info_flag,
	.chip_before_suspend = focal_before_suspend,
	.chip_suspend = focal_suspend,
	.chip_resume = focal_resume,
	.chip_after_resume = focal_after_resume,
	.chip_wakeup_gesture_enable_switch = focal_wakeup_gesture_enable_switch,
	.chip_get_rawdata = focal_get_raw_data_all,
	.chip_get_debug_data = focal_get_debug_data,
	.chip_glove_switch = focal_glove_switch,
	.chip_shutdown = focal_shutdown,
	.chip_holster_switch = focal_holster_switch,
	.chip_roi_switch = focal_roi_switch,
	.chip_roi_rawdata = focal_roi_rawdata,
	.chip_palm_switch = focal_palm_switch,
	.chip_regs_operate = focal_regs_operate,
	.chip_calibrate = focal_calibrate,
	.chip_calibrate_wakeup_gesture = focal_calibrate_wakeup_gesture,
	.chip_reset = focal_reset_device,
	.chip_check_status = focal_esdcheck_func,
#if defined(HUAWEI_CHARGER_FB)
	.chip_charger_switch = focal_charger_switch,
#endif
#ifdef HUAWEI_TOUCHSCREEN_TEST
	.chip_test = test_dbg_cmd_test,
#endif
	.chip_wrong_touch = focal_wrong_touch,
	.chip_touch_switch = focal_chip_touch_switch,
	.chip_special_rawdata_proc_printf = rawdata_proc_focal_printf,
};

static int rawdata_proc_focal_printf(struct seq_file *m, struct ts_rawdata_info *info,
					int range_size, int row_size)
{
	int index = 0;
	int index1 = 0;
	int indexsum = 0;
	if((0 == range_size) || (0 == row_size) || (!info)) {
		TS_LOG_ERR("%s  range_size OR row_size is 0 OR info is NULL\n", __func__);
		return -EINVAL;
	}
	/*row_size is 48,range_size is 30,all 7 test case*/
	for (index = 0; row_size * index + 2 < (2+row_size*range_size * FTS_8201_MAX_CAP_TEST_NUM); index++) {
		/*index =0,0~29 row data is rawdata,rawdata array printf tag*/
		if (0 == index) {
			seq_printf(m, "rawdata begin\n");	/*print the title */
		}
		for (index1 = 0; index1 < row_size; index1++) {
			indexsum = 2 + row_size * index + index1;
			seq_printf(m, "%d,", info->buff[indexsum]);	/*print oneline */
		}
		/*index1 = 0;*/
		seq_printf(m, "\n ");
		/*index =29,29~58 row data is shortdata,shortdata array printf tag*/
		if ((range_size - 1) == index) {
			seq_printf(m, "rawdata end\n");
			seq_printf(m, "shortdata begin\n");
		/*index =59,59~87 row data is cbdata,cbdata array printf tag*/
		} else if ((range_size*2 - 1) == index){
			seq_printf(m, "shortdata end\n");
			seq_printf(m, "cbdata begin\n");
		/*index =89,89~128 row data is cb uniformity x data,cb uniformity x data array printf tag*/
		} else if ((range_size*3 - 1) == index){
			seq_printf(m, "cbdata end\n");
			seq_printf(m, "cb uniformity x arry begin\n");
		/*index =129,129~158 row data is cb uniformity y data,cb uniformity y data array printf tag*/
		} else if ((range_size*4 - 1) == index){
			seq_printf(m, "cb uniformity x arry end\n");
			seq_printf(m, "cb uniformity y arry begin\n");
		/*index =159,159~188 row data is noise data,noise data array printf tag*/
		} else if ((range_size*5 - 1) == index){
			seq_printf(m, "cb uniformity y arry end\n");
			seq_printf(m, "noise begin\n");
		/*index =189,189~218 row data is CB increase data,cb increase data array printf tag*/
		} else if ((range_size*6 - 1) == index){
			seq_printf(m, "noise end\n");
			seq_printf(m, "cb increase data begin\n");
		}
	}
	/*index =219,cb increase array printf tag*/
	seq_printf(m, "cb increase data  end\n");
	return NO_ERR;
}

static int focal_get_raw_data_all(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd)
{
	int retval = NO_ERR;
	TS_LOG_INFO("%s enter\n", __func__);
	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		return -ENOMEM;
	}
	if (!out_cmd) {
		TS_LOG_ERR("%s: out_cmd is Null\n", __func__);
		return -ENOMEM;
	}
	if (FOCAL_FT8201 == g_focal_dev_data->ic_type)
		retval = focal_8201_get_raw_data(info, out_cmd);
	else
		retval = focal_get_raw_data(info, out_cmd);
	if (retval < 0)
		TS_LOG_ERR("failed to get rawdata\n");

	return retval;
}

inline struct ts_kit_device_data *focal_get_device_data(void)
{
	return g_focal_dev_data;
}

inline struct focal_platform_data *focal_get_platform_data(void)
{
	return g_focal_pdata;
}

static void focal_print_u8_array(u8 *data, u32 size)
{
	int i = 0;
	u32 str_len = 0;
	char *str_buf = NULL;
	char *str_print = NULL;

	/* every data item use 5 byte */
	str_len = U8_STR_LEN * size;

	str_buf = kzalloc(str_len + 1, GFP_KERNEL);
	if (!str_buf)
		return;

	str_print = str_buf;
	for (i = 0; i < size; i++) {

		snprintf(str_print, str_len, "0x%02X ", data[i]);
		str_print += U8_STR_LEN;
		str_len -= U8_STR_LEN;
	}

	TS_LOG_INFO("%s\n", str_buf);

	kfree(str_buf);
	str_buf = NULL;
	str_print = NULL;
}

char *focal_strncat(char *dest, char *src, size_t dest_size)
{
	size_t dest_len = 0;
	char *start_index = NULL;

	dest_len = strnlen(dest, dest_size);
	start_index = dest + dest_len;

	return strncat(&dest[dest_len], src, (dest_size > dest_len ? (dest_size - dest_len - 1) : 0));
}

char *focal_strncatint(char *dest, int src, char *format, size_t dest_size)
{
	char src_str[16] = {0};

	snprintf(src_str, 16, format, src);

	return focal_strncat(dest, src_str, dest_size);
}


int focal_read(u8 *addrs, u16 addr_size, u8 *values, u16 values_size)
{
	int ret = 0;
	struct ts_bus_info *bops = NULL;

	bops = g_focal_dev_data->ts_platform_data->bops;
	if (TS_BUS_I2C == bops->btype) {
		ret = bops->bus_read(addrs, addr_size, values, values_size);/*i2c*/
	} else {
		ret = fts_read(addrs, addr_size, values, values_size);/* spi */
	}
	if (ret) {
		TS_LOG_ERR("%s:fail, addrs:", __func__);
		focal_print_u8_array(addrs, addr_size);
	}

	return ret;
}

int focal_read_default(u8 *values, u16 values_size)
{
	return focal_read(NULL, 0, values, values_size);
}

int focal_read_reg(u8 addr, u8 *val)
{
	return focal_read(&addr, 1, val, 1);
}

int focal_write(u8 *values, u16 values_size)
{
	int ret = 0;
	struct ts_bus_info *bops = NULL;

	bops = g_focal_dev_data->ts_platform_data->bops;
	if (TS_BUS_I2C == bops->btype) {
		ret = bops->bus_write(values, values_size);/* i2c */
	} else {
		ret = fts_write(values, values_size);/* spi */
	}
	if (ret) {
		TS_LOG_ERR("%s:fail, addrs:", __func__);
		focal_print_u8_array(values, values_size);
	}

	return ret;
}

int focal_write_default(u8 value)
{
	return focal_write(&value, 1);
}


int focal_write_reg(u8 addr, u8 value)
{
	u8 buf[2] = {0};

	buf[0] = addr;
	buf[1] = value;

	return focal_write(buf, sizeof(buf));
}
#ifdef CONFIG_HUAWEI_DEVKIT_MTK_3_0
static void focal_irq_output(int level)
{
	int error = 0;
	TS_LOG_INFO("%s:  level = %d\n", __func__, level);
	if (level)
		error = pinctrl_select_state(g_focal_pdata->pctrl, g_focal_pdata->pinctrl_state_int_high);
	else
		error = pinctrl_select_state(g_focal_pdata->pctrl, g_focal_pdata->pinctrl_state_int_low);

	if (error < 0)
			TS_LOG_ERR("%s:Set irq pin state error:%d\n", __func__, error);
}

static int focal_reset_output(int level)
{
	int error = 0;
	int reset_gpio = 0;

	TS_LOG_INFO("%s:  level = %d\n", __func__, level);
#ifdef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	if (level)
		error = pinctrl_select_state(g_focal_pdata->pctrl, g_focal_pdata->pinctrl_state_reset_high);
	else
		error = pinctrl_select_state(g_focal_pdata->pctrl, g_focal_pdata->pinctrl_state_reset_low);
#else
	reset_gpio = g_focal_dev_data->ts_platform_data->reset_gpio;
	if (level)
		error = gpio_direction_output(reset_gpio, 1);
	else
		error = gpio_direction_output(reset_gpio, 0);
#endif
	if (error < 0)
			TS_LOG_ERR("%s:Set reset pin state error:%d\n", __func__, error);
	return error;
}

static int focal_cs_output(int level)
{
	int error = 0;
	int cs_gpio = 0;

	TS_LOG_INFO("%s:  level = %d\n", __func__, level);
#ifndef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	cs_gpio = g_ts_kit_platform_data.cs_gpio;
	if (level)
		error = gpio_direction_output(cs_gpio, 1);
	else
		error = gpio_direction_output(cs_gpio, 0);
#endif
	if (error < 0)
			TS_LOG_ERR("%s:Set reset pin state error:%d\n", __func__, error);
	return error;
}
#endif
static int focal_gpio_reset(void)
{
	int ret = 0;

	ret = focal_reset_output(1);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 1 fail, ret=%d\n",
				   __func__, ret);
		return ret;
	}
	msleep(1);

	ret = focal_reset_output(0);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 0 fail, ret=%d\n",
				   __func__, ret);
		return ret;
	}
	msleep(1);

	ret = focal_reset_output(1);
	if (ret) {
		TS_LOG_ERR("%s:gpio direction output to 1 fail, ret=%d\n",
				   __func__, ret);
		return ret;
	}
	return 0;
}

int focal_hardware_reset(int model)
{
	int ret = 0;
	int reset_enable_delay = 0;

	ret = focal_gpio_reset();
	if(ret){
		TS_LOG_ERR("%s:gpio_reset fail, ret=%d\n",
				   __func__, ret);
		return ret;
	}

	switch (model) {
	case FTS_MODEL_BOOTLOADER:
		reset_enable_delay = 6;
		break;
	case FTS_MODEL_FIRST_START:
		reset_enable_delay = 240;
		break;
	case FTS_MODEL_NORMAL:
		reset_enable_delay = 120;
		break;
	default:
		reset_enable_delay = 240;
	}

	TS_LOG_INFO("%s:reset enable delay:%d\n", __func__, reset_enable_delay);
	if(FOCAL_FT8006U == g_focal_dev_data->ic_type){
		/*ft8006 need accurate delay(6-26ms),the mdelay() is more accurate than msleep()*/
		mdelay(reset_enable_delay);
	}else{
		msleep(reset_enable_delay);
	}

	if(FOCAL_FT5X46 == g_focal_dev_data->ic_type){
	 	/*fae ask add this delay, this 5x46 need 100ms delay*/
		msleep(FTS_SLEEP_TIME_100);
	}

	return 0;
}

int focal_ft5x46_chip_reset(void)
{
	int ret = 0;
	int reset_gpio = 0;
	int reset_enable_delay = 0;
	reset_gpio = g_focal_dev_data->ts_platform_data->reset_gpio;

	udelay(FT5X46_RESET_KEEP_LOW_TIME);
	ret = focal_reset_output(1);
	if(ret){
		TS_LOG_ERR("%s:gpio_reset pull up fail, ret=%d\n",
				   __func__, ret);
		return ret;
	}

	TS_LOG_INFO("%s:reset enable delay\n", __func__);
	msleep(RESET_ENABLE_DELAY);

	return 0;
}

/*
 * description: after finishe control reset gpio, write 0x55 to 0xfc in 6~25ms,
 *			  the ic will enter bootloader(rom boot) model
 *
 * param - focal_pdata : truct focal_platform_data
 *
 * return 0 if success, other wize return error code
 */
int focal_hardware_reset_to_rom_update_model(void)
{
	int i = 0;
	int ret = 0;
	u32 delay_time = 0;

	delay_time = 6;
	for (i = 0; i < FTS_RETRY_TIMES; i++) {

		if (i != 0)
			delay_time += 2;

		ret = focal_hardware_reset(FTS_MODEL_BOOTLOADER);
		if (ret < 0) {
			TS_LOG_ERR("%s:%s, ret=%d\n", __func__,
				"hardware reset to bootloader fail", ret);

			continue;
		}

		ret = focal_write_default(FTS_UPGRADE_55);
		if (ret < 0) {
			TS_LOG_ERR("%s:write command 0x55 fail, ret=%d\n",
				__func__, ret);
			continue;
		}

		mdelay(FOCAL_AFTER_WRITE_55_DELAY_TIME);
		ret = focal_read_chip_id_(&g_focal_pdata->chip_id);
		TS_LOG_INFO("%s:ret = %d , g_focal_pdata->chip_id=%x\n",__func__, ret ,g_focal_pdata->chip_id);
		if (ret|| (g_focal_pdata->chip_id == 0) ){
			TS_LOG_INFO("%s:chip id read fail, retry=%d\n",
				__func__, i);

			delay_time += 2;
			continue;
		} else {
			return 0;
		}
	}

	return -EINVAL;
}

int focal_hardware_reset_to_normal_model(void)
{
	return focal_hardware_reset(FTS_MODEL_NORMAL);
}

/*
 * description: after finishe control reset gpio, write 0x55 to 0xfc in 6~25ms,
 *			  the ic will enter bootloader(rom boot) model
 *
 * @param: focal_pdata - truct focal_platform_data
 *
 * return 0 if success, other wize return error code
 */
int focal_hardware_reset_to_bootloader(void)
{
	int i = 0;
	int ret = 0;
	u32 delay_time = 0;
	u32 chip_id = 0;

	delay_time = 6;
	for (i = 0; i < FTS_RETRY_TIMES; i++) {

		if (i != 0)
			delay_time += 2;

		ret = focal_hardware_reset(FTS_MODEL_BOOTLOADER);
		if (ret < 0) {
			TS_LOG_ERR("%s:%s, ret=%d\n", __func__,
				"hardware reset to bootloader fail", ret);

			continue;
		}

		ret = focal_write_default(FTS_UPGRADE_55);
		if (ret < 0) {
			TS_LOG_ERR("%s:write command 0x55 fail, ret=%d\n",
				__func__, ret);
			continue;
		}

		ret = focal_read_chip_id_(&chip_id);
		if (ret || chip_id == 0) {
			TS_LOG_INFO("%s:chip id read fail, retry=%d\n",
				__func__, i);

			delay_time += 2;
			continue;
		} else {
			return 0;
		}
	}

	return -EINVAL;
}

/************************************************************************
* Name: focal_change_i2c_hid2std
* Brief:  HID to I2C
* Input: i2c info
* Output: no
* Return: fail =0
***********************************************************************/
static int focal_change_i2c_hid2std(void)
{

	u8 cmd[5] = {0};
	int ret = 0;

	cmd[0] = FTS_CHANGE_I2C_HID2STD_ADDR1;
	cmd[1] = FTS_CHANGE_I2C_HID2STD_ADDR2;
	cmd[2] = FTS_CHANGE_I2C_HID2STD_ADDR3;
	ret = focal_write( cmd, 3);
	if(ret){
		TS_LOG_ERR("%s: write fail!ret=%d\n", __func__, ret);
	}
	msleep(10);
	cmd[0] = cmd[1] = cmd[2] = 0;
	ret = focal_read(cmd, 0, cmd, 3);
	if(ret){
		TS_LOG_ERR("%s: read fail!ret=%d\n", __func__, ret);
	}
	if ((FTS_CHANGE_I2C_HID2STD_ADDR1 == cmd[0])
		&& (FTS_CHANGE_I2C_HID2STD_ADDR2 == cmd[1])
		&& (FTS_CHANGE_I2C_HID2STD_STATUS== cmd[2]))
	{
		TS_LOG_INFO("%s:change to stdi2c successful!!\n", __func__);
		ret = 0;
	}
	else
	{
		TS_LOG_ERR("%s:change to stdi2c error!!\n", __func__);
		ret = -EINVAL;
	}

	return ret;

}

int focal_software_reset_to_bootloader(struct focal_platform_data *focal_pdata)
{
	int i = 0;
	int ret = 0;
	u32 chip_id = 0;
	u8  cmd[2] = {0};

	focal_change_i2c_hid2std();

	for (i = 0; i < FTS_RETRY_TIMES; i++) {
		ret = focal_enter_rom_update_model_by_software(focal_pdata);
		if (ret < 0)
		{
			TS_LOG_ERR("%s: focal_enter_rom_update_model_by_software fail!\n", __func__);
			continue;
		}

		ret = focal_change_i2c_hid2std();
		if (ret < 0)
		{
			TS_LOG_ERR("%s: focal_change_i2c_hid2std fail!\n", __func__);
			continue;
		}

		msleep(10);
		cmd[0] = FTS_UPGRADE_55;
		cmd[1] = FTS_UPGRADE_AA;
		ret = focal_write(cmd, 1);
		if (ret < 0)
		{
			TS_LOG_ERR("%s: failed writing  0x55 and 0xaa!\n", __func__);
			continue;
		}

		ret = focal_read_chip_id_(&g_focal_pdata->chip_id);
		if (ret || g_focal_pdata->chip_id == 0) {
			TS_LOG_INFO("%s:chip id read fail, retry=%d\n",
				__func__, i);
			continue;
		} else {
			TS_LOG_INFO("%s:chip id read success:%x\n",
			__func__, chip_id);
			return 0;
		}
	}

	return -EINVAL;
}

int focal_strtolow(char *src_str, size_t size)
{
	char *str = NULL;

	if (NULL == src_str)
		return -EINVAL;

	str = src_str;
	while (*str != '\0' && size > 0) {
		if (*str >= 'A' && *str <= 'Z')
			*str += ('a' - 'A');

		str++;
		size--;
	}

	return 0;
}

static int focal_diff_flag = FTS_GET_DBG_ON;
static int _focal_print_bulk(const char * head, u8 * buf, u32 len, u8 width)
{
	int i = 0;
	int n = 0;
	u8 buf_line[PRINT_BULK_BUF_LEN + 1] = {0};

	if((0 == len)||(len > PRINT_BULK_MAX_LEN)||(NULL == head)||(NULL == buf)) {
		return -EIO;
	}

	if((width > PRINT_BULK_MAX_WIDTH)||(width < PRINT_BULK_MIN_WIDTH)) {
		width = PRINT_BULK_MIN_WIDTH;
	}

	for( i = 0;(i < len)&&(n < PRINT_BULK_BUF_LEN); i++) {
		if(((i % width) == 0) && (n > 0)) {
			TS_LOG_INFO("%s:%s\n", head, buf_line);
			n = 0;
		}
		n += snprintf(&buf_line[n], PRINT_BULK_BUF_LEN - n, "%02x ", buf[i]);
	}
	if(n > 0) {
		TS_LOG_INFO("%s:%s\n", head, buf_line);
	}
	return 0;
}

static int focal_print_diff(u8 *buf, u32 len)
{
	int i = 0;
	int j = 0;
	int n = 0;
	short diff[FTS_DIFF_DATA_HEIGHT*FTS_DIFF_DATA_WIDTH] = { 0 };
	u8 buf_line[PRINT_DIFF_BUF_LEN + 1] = {0};

	if(NULL == buf) {
		return -EIO;
	}

	for (i = 0; i < len; i = i + 2) {
		diff[i >> 1] = ((short)buf[i] << 8) + buf[i + 1];
	}

	for (i = 0; i < FTS_DIFF_DATA_HEIGHT; i++) {
		for (j = 0;(j < FTS_DIFF_DATA_WIDTH)&&(n < PRINT_DIFF_BUF_LEN); j++) {
			n += snprintf(&buf_line[n], PRINT_DIFF_BUF_LEN - n, "%d,", (int)diff[i*FTS_DIFF_DATA_WIDTH+j]);
		}
		TS_LOG_INFO("%s:%s\n", __func__, buf_line);
		n = 0;
	}
	return 0;
}

static int focal_print_dbg(u8 *buf)
{
	if(NULL == buf) {
		return -EIO;
	}
	TS_LOG_INFO("%s:--- print dbg status reg data --- \n", __func__);
	focal_print_bulk(&buf[ADDR_DBG_DATA], FTS_DBG_DATA_SIZE, FTS_DIFF_DATA_WIDTH);
	TS_LOG_INFO("%s:--- print dbg frame diff org data --- \n", __func__);
	focal_print_bulk(&buf[ADDR_DIFF_DATA], FTS_DIFF_DATA_SIZE, FTS_DIFF_DATA_WIDTH);
	focal_print_diff(&buf[ADDR_DIFF_DATA], FTS_DIFF_DATA_SIZE);
	return NO_ERR;
}

static int focal_read_touch_data(struct ts_event *event_data)
{
	int i = 0;
	int ret = -1;
	u32 offset = 0;
	u32 fts_edge_data_len = 0;
	u32 fts_edge_each_point_data_len = 0;

	u8 buf[FTS_REPORT_DATA_ALL_LEN + 1 + FTS_DBG_DATA_SIZE + FTS_DIFF_DATA_SIZE] = { 0 };
	u8 buf_edge[FTS_FP_EDGE_TOUCH_DATA_LEN] = { 0 };

	if (TS_BUS_SPI == g_focal_dev_data->ts_platform_data->bops->btype) {
		buf[0] = TOUCH_DATA_START_ADDR_SPI;
		fts_edge_each_point_data_len = FTS_FP_EDGE_POINT_DATA_SIZE;
		ret = focal_read(NULL, 0, buf + 1, FTS_REPORT_DATA_ALL_LEN);

		if (ret < 0) {
			TS_LOG_ERR("%s:read touchdata failed, ret=%d.\n",
				__func__, ret);
			return ret;
		}
		if ((buf[2] == FTS_FW_STATE_ERROR) && (buf[3] == FTS_FW_STATE_ERROR) && (buf[4] == FTS_FW_STATE_ERROR)) {
			if(true == g_focal_pdata->fw_is_running){
				ret = focal_fw_update_boot_recovery();
				if (!ret) {
					TS_LOG_INFO("%s:boot recovery pass\n", __func__);
				}
			}
			return -EIO;
		}

		/* check if need recovery fw */
		if (g_focal_pdata->enable_edge_touch) {
			memcpy(buf_edge, &buf[FTS_EDGE_DATA_OFF + 1], FTS_FP_EDGE_TOUCH_DATA_LEN);
		}
		if(g_focal_dev_data->ts_platform_data->feature_info.roi_info.roi_switch
			&& g_focal_dev_data->ts_platform_data->feature_info.roi_info.roi_supported) {
			if(ROI_DATA_READY == buf[FTS_ROI_DATA_OFF - 1]) {
				memcpy(focal_roi_data, &buf[FTS_ROI_DATA_OFF + 1], ROI_DATA_READ_LENGTH);
			} else if(ROI_DATA_NOT_NEET_READ== buf[FTS_ROI_DATA_OFF - 1]) {
				TS_LOG_DEBUG("%s: roi data not neet read . \n",__func__);
			} else {
				TS_LOG_ERR("%s: roi data not ready. \n",__func__);
			}
		}
	} else {
		buf[0] = TOUCH_DATA_START_ADDR;
		ret = focal_read(buf, 1, buf, FTS_TOUCH_DATA_LEN);
		if (ret < 0) {
			TS_LOG_ERR("%s:read touchdata failed, ret=%d.\n",
				__func__, ret);
			return ret;
		}

		if (g_focal_pdata->aft_wxy_enable) {
			fts_edge_data_len = FTS_FP_EDGE_TOUCH_DATA_LEN;
			fts_edge_each_point_data_len = FTS_FP_EDGE_POINT_DATA_SIZE;
		} else {
			fts_edge_data_len = FTS_EDGE_TOUCH_DATA_LEN;
			fts_edge_each_point_data_len = FTS_EDGE_POINT_DATA_SIZE;
		}
		if (g_focal_pdata->enable_edge_touch) {
			buf_edge[0] = g_focal_pdata->edge_data_addr;
			ret = focal_read(buf_edge, 1, buf_edge, fts_edge_data_len);
			if (ret < 0) {
				TS_LOG_ERR("%s  failed.\n", __func__);
				return ret;
			}
		}
	}
	memset(event_data, 0, sizeof(struct ts_event));
	event_data->touch_point = 0;
	event_data->touch_point_num = buf[ADDR_POINT_NUM] & 0x0F;

	for (i = 0; i < FTS_MAX_TOUCH_POINTS; i++) {
		offset = FTS_POINT_DATA_SIZE * i;

		event_data->finger_id[i]
			= (buf[ADDR_FINGER_ID_POS + offset]) >> 4;

		if (event_data->finger_id[i] >= FTS_MAX_POINT_ID)
			break;
		else
			event_data->touch_point++;

		event_data->position_x[i] =
			(s16)(buf[ADDR_X_H_POS + offset] & 0x0F) << 8
			| (s16)(buf[ADDR_X_L_POS + offset]);

		event_data->position_y[i] =
			(s16)(buf[ADDR_Y_H_POS + offset] & 0x0F) << 8
			| (s16)(buf[ADDR_Y_L_POS + offset]);

		event_data->touch_event[i] = buf[ADDR_EVENT_POS + offset] >> 6;
		event_data->pressure[i] = (buf[ADDR_XY_POS + offset]);
		event_data->area[i] = (buf[ADDR_MISC + offset]) >> 4;
		if (g_focal_pdata->enable_edge_touch) {
			event_data->ewx[i] = buf_edge[event_data->finger_id[i]*fts_edge_each_point_data_len+ADDR_EDGE_EWX];
			event_data->ewy[i] = buf_edge[event_data->finger_id[i]*fts_edge_each_point_data_len+ADDR_EDGE_EWY];
			event_data->yer[i] = buf_edge[event_data->finger_id[i]*fts_edge_each_point_data_len+ADDR_EDGE_YER];
			event_data->xer[i] = buf_edge[event_data->finger_id[i]*fts_edge_each_point_data_len+ADDR_EDGE_XER];
			if (g_focal_pdata->aft_wxy_enable) {
				event_data->wx[i] = buf_edge[event_data->finger_id[i]*fts_edge_each_point_data_len+ADDR_EDGE_WX];
				event_data->wy[i] = buf_edge[event_data->finger_id[i]*fts_edge_each_point_data_len+ADDR_EDGE_WY];
			}
			TS_LOG_DEBUG("%s: edge touch data: ewx=%d, exy=%d, xer=%d, yer=%d, wx = %d, wy = %d\n",
				__func__,
				event_data->ewx[i],
				event_data->ewy[i],
				event_data->xer[i],
				event_data->yer[i],
				event_data->wx[i],
				event_data->wy[i]);
		}

		/* check event data */
		if (event_data->touch_point_num == 0) {
			if (event_data->touch_event[i] == 0
				|| event_data->touch_event[i] == 2) {
				TS_LOG_ERR("%s:abnormal touch data from fw.\n",
					__func__);
				return -EINVAL;
			}
		}

	}

	return 0;
}

static int i2c_communicate_check(struct ts_kit_platform_data *focal_pdata)
{
	int i = 0;
	int ret = NO_ERR;
	u8 cmd = 0;
	u8 reg_val = 0;

	focal_pdata->client->addr = g_focal_dev_data->slave_addr;
	cmd = FTS_REG_CHIP_ID_H;
	for (i = 0; i < FTS_DETECT_I2C_RETRY_TIMES; i++) {

		ret = focal_read(&cmd, 1, &reg_val, 1);
		if (ret < 0) {
			TS_LOG_ERR("%s:chip id read fail, ret=%d, i=%d\n",
				__func__, ret, i);
			msleep(50);
		} else {
			TS_LOG_INFO("%s:chip id read success, chip id:0x%X\n",
				__func__, reg_val);
			return NO_ERR;
		}
	}

	return ret;
}

static int spi_communicate_check(struct ts_kit_platform_data *focal_pdata)
{
	int i = 0;
	int ret = NO_ERR;
	u8 cmd = 0;
	u8 reg_val = 0;

	mutex_init(&g_focal_pdata->spilock);
	cmd = FTS_REG_BOOT_CHIP_ID;
	for (i = 0; i < FTS_DETECT_SPI_RETRY_TIMES; i++) {
		ret = focal_read(&cmd, FTS_COMMON_COMMAND_LENGTH, &reg_val, FTS_COMMON_COMMAND_VALUE);
		if ((reg_val != 0x00) && (reg_val != 0xFF) && (reg_val != 0xEF)) {/*0x00 0xFF:spi bus error.0xEF:ic error*/
			TS_LOG_INFO("%s:chip id read success, chip id:0x%X\n",
				__func__, reg_val);
			return NO_ERR;
		} else {
			TS_LOG_ERR("%s:chip id read fail, ret=%d, i=%d, reg_val=%d\n",
				__func__, ret, i, reg_val);
			msleep(50);
		}
	}

	TS_LOG_ERR("%s spi communicate timeout", __func__);
	return -EIO;
}

static int communicate_check(struct ts_kit_platform_data *focal_pdata)
{
	if (TS_BUS_I2C == focal_pdata->bops->btype)
		return i2c_communicate_check(focal_pdata);
	else
		return spi_communicate_check(focal_pdata);
}

static void focal_gesture_recovery(void)
{
	int retval = 0;

	/*write reg d1-d2  0xff for open gesture function*/
	retval = focal_write_reg( FTS_REG_GESTURE_OUTPUT_ADDRESS_D1, FTS_REG_GESTURE_OPEN_FF);
	if(retval < 0){
		TS_LOG_ERR("%s, %d: have error\n", __func__, __LINE__);
	}

	retval = focal_write_reg( FTS_REG_GESTURE_OUTPUT_ADDRESS_D2, FTS_REG_GESTURE_OPEN_FF);
	if(retval < 0){
		TS_LOG_ERR("%s, %d: have error\n", __func__, __LINE__);
	}

	retval = focal_write_reg( FTS_REG_GESTURE_OUTPUT_ADDRESS_D5, FTS_REG_GESTURE_OPEN_FF);
	if(retval < 0){
		TS_LOG_ERR("%s, %d: have error\n", __func__, __LINE__);
	}

	retval = focal_write_reg( FTS_REG_GESTURE_OUTPUT_ADDRESS_D6, FTS_REG_GESTURE_OPEN_FF);
	if(retval < 0){
		TS_LOG_ERR("%s, %d: have error\n", __func__, __LINE__);
	}

	retval = focal_write_reg( FTS_REG_GESTURE_OUTPUT_ADDRESS_D7, FTS_REG_GESTURE_OPEN_FF);
	if(retval < 0){
		TS_LOG_ERR("%s, %d: have error\n", __func__, __LINE__);
	}

	retval = focal_write_reg( FTS_REG_GESTURE_OUTPUT_ADDRESS_D8, FTS_REG_GESTURE_OPEN_FF);
	if(retval < 0){
		TS_LOG_ERR("%s, %d: have error\n", __func__, __LINE__);
	}

	//write 1 for enable gesture_en
	retval = focal_write_reg( FTS_REG_GESTURE_EN, 1);
	if(retval < 0){
		TS_LOG_ERR("%s, %d: have error\n", __func__, __LINE__);
	}
	return;
}

/*****************************************************************************
*  Name: focal_get_chip_id
*  Brief: Read Chip Id 3 times
*  Input:
*  Output:
*  Return:  1 - Read Chip Id 3 times failed
*               0 - Read Chip Id pass
*****************************************************************************/
static int focal_esdcheck_chip_id(void)
{
	int err = 0;
	int i = 0;
	u8 reg_value = 0;
	u8 cmd = 0;
	u8 chipid_high =(g_focal_pdata->chip_id >>8 )&0xff;

	for (i = 0; i < FTS_ESD_MAX_TIMES; i++){
		cmd = FTS_REG_CHIP_ID_H;
		err = focal_read(&cmd, 1, &reg_value, 1);
		if ( err < 0 ){
			TS_LOG_ERR("%s:[ESD] Read Reg 0xA3 failed ret = %d \n", __func__, err);
			continue;
		}

		if ( (reg_value == chipid_high) || (reg_value == FTS_REG_SPECIAL_VALUEl) ){ /* Upgrade sometimes can't detect */
			TS_LOG_DEBUG("%s:chip id read success, chip id:0x%X, i=%d\n",__func__, reg_value,i);
			break;
		}
	}

	if(FTS_ESD_MAX_TIMES == i){
		TS_LOG_ERR("%s:chip id read fail, reg_value=%d, i=%d, chipid_high=%d, \n",__func__, reg_value, i, chipid_high);
		return -EINVAL;
	}

	return 0;
}

/*****************************************************************************
*  Name: focal_esdcheck_tp_reset
*  Brief: esd check algorithm
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int focal_esdcheck_tp_reset( void )
{
	int ret = 0;

	ret = focal_hardware_reset(FTS_MODEL_NORMAL);
	if(ret){
		TS_LOG_ERR("%s:[ESD] focal hardware reset fail ret = %d \n", __func__, ret);
#if defined (CONFIG_HUAWEI_DSM)
		ts_dmd_report(DSM_TP_ABNORMAL_DONT_AFFECT_USE_NO, "try to client record DSM_TP_ABNORMAL_DONT_AFFECT_USE_NO(%d): focal ESD hardware reset.\n", DSM_TP_ABNORMAL_DONT_AFFECT_USE_NO);
#endif
		return ret;
	}

	ret = focal_esdcheck_chip_id();
	if (ret < 0){
		TS_LOG_ERR("%s:[ESD] check chip_id error = %d \n", __func__, ret);
#if defined (CONFIG_HUAWEI_DSM)
		ts_dmd_report(DSM_TP_ABNORMAL_DONT_AFFECT_USE_NO, "try to client record DSM_TP_ABNORMAL_DONT_AFFECT_USE_NO(%d): focal esdcheck chip id.\n", DSM_TP_ABNORMAL_DONT_AFFECT_USE_NO);
#endif
		return -EINVAL;
	}

	ret = focal_status_resume();
	if(ret < 0){
		TS_LOG_ERR("%s: failed to resume status\n",__func__, ret);
		return -EINVAL;
	}

	fts_esdcheck_data.flow_work_hold_cnt = 0;

	if((g_focal_dev_data->easy_wakeup_info.sleep_mode == TS_GESTURE_MODE)
		&&(fts_esdcheck_data.suspend == true)){
		focal_gesture_recovery();
    	}

	return 0;
}


/*****************************************************************************
*  Name: focal_get_flow_count
*  Brief: Read flow count(0x91)
*  Input:
*  Output:
*  Return:  1 - Reg 0x91(flow count) abnormal: hold a value for 5 times
*               0 - Reg 0x91(flow count) normal
*****************************************************************************/
static int focal_esdcheck_flow_count(void)
{
	int err = 0;
	u8 reg_value = 0;
	u8 cmd = 0;

	cmd = FTS_REG_FLOW_WORK_CNT;
	err = focal_read(&cmd, 1, &reg_value, 1);
	if (err < 0){
		TS_LOG_ERR("%s:[ESD]: Read Reg 0x91 failed ret = %d \n", __func__, err);
		err = focal_esdcheck_chip_id();
		if(err < 0){
			TS_LOG_ERR("%s:[ESD]: read chip id failed ret = %d \n", __func__, err);
			return -EINVAL;
		}
	}else{
		if ( reg_value == fts_esdcheck_data.flow_work_cnt_last ){
			fts_esdcheck_data.flow_work_hold_cnt++;
		}else{
			fts_esdcheck_data.flow_work_hold_cnt = 0;
			fts_esdcheck_data.flow_work_cnt_last = reg_value;
		}
	}

	/* if read flow work cnt 5 times and the value are all the same, then need hardware_reset */
	if (fts_esdcheck_data.flow_work_hold_cnt >= FTS_CHECK_FLOW_COUNT){
		TS_LOG_INFO("%s:[ESD]: Flow Work Cnt(reg0x91) keep a value for 5 times, need execute TP reset \n", __func__);
		return -EINVAL;
	}

	TS_LOG_DEBUG("%s:[ESD]: check flow count no need reset ret = %d \n", __func__, err);
	return 0;
}

/*****************************************************************************
*  Name: focal_esdcheck_algorithm
*  Brief: esd check algorithm
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int focal_esdcheck_algorithm(void)
{
	int ret = 0;
	u8 reg_value = 0;
	u8 cmd = 0;
	int hardware_reset = 0;

	TS_LOG_DEBUG("%s: Enter!\n", __func__);

	/* 1. esdcheck is interrupt, then return */
	if (true == fts_esdcheck_data.intr){
		TS_LOG_INFO("%s:[ESD]: In interrupt state, not check esd, return immediately! \n", __func__);
		return 0;
	}

	/* 2. check power state, if suspend, no need check esd */
	if (true == fts_esdcheck_data.suspend){
		TS_LOG_INFO("%s:[ESD]: In suspend, not check esd, return immediately!\n", __func__);
		/* because in suspend state, adb can be used, when upgrade FW, will active ESD check(active = 1)
		*  But in suspend, then will don't queue_delayed_work, when resume, don't check ESD again
		*/
		fts_esdcheck_data.active = 0;
		return 0;
	}

	/* 3. In boot upgrade mode , can't check esd */
	if (true == fts_esdcheck_data.boot_upgrade){
		TS_LOG_INFO("%s:[ESD]: In boot upgrade state, not check esd, return immediately! \n", __func__);
		return 0;
	}

	/* 4. In factory mode, can't check esd */
	cmd = FTS_REG_WORKMODE;
	ret= focal_read(&cmd, 1, &reg_value, 1);
	if ( ret < 0 ){
		TS_LOG_ERR("%s : focal read FTS_REG_WORKMODE error\n", __func__);
	}
	else if ( (reg_value & FTS_REG_WORKMODE_FACTORY_OFFSET) ==  FTS_REG_WORKMODE_FACTORY_VALUE){
		TS_LOG_INFO("%s: [ESD]: In factory mode, not check esd, return immediately!!\n", __func__);
		return 0;
	}

	/* 5. Get Chip ID */
	hardware_reset = focal_esdcheck_chip_id();

	/* 6. get Flow work cnt: 0x91 If no change for 5 times, then ESD and reset */
	if (0 == hardware_reset){
		TS_LOG_DEBUG("%s : check flow count begin hardware_reset = %d\n", __func__, hardware_reset);
		hardware_reset = focal_esdcheck_flow_count();
	}

	/* 7. If need hardware reset, then handle it here */
	TS_LOG_DEBUG("%s : esd reset begin hardware_reset = %d \n", __func__, hardware_reset);
	if (hardware_reset < 0){
		ret = focal_esdcheck_tp_reset();
		if (ret < 0){
			TS_LOG_ERR("%s : esd reset failed\n", __func__);
			return ret;
		}
	}

	TS_LOG_DEBUG("%s : esd reset end\n", __func__);
	return 0;
}

static int focal_esdcheck_func(void)
{
	return focal_esdcheck_algorithm();
}

int focal_esdcheck_set_upgrade_flag(u8 boot_upgrade)
{
	fts_esdcheck_data.boot_upgrade = (bool)boot_upgrade;
	return 0;
}

static int focal_wrong_touch(void)
{
	int rc = NO_ERR;
	mutex_lock(&wrong_touch_lock);
	g_focal_dev_data->easy_wakeup_info.off_motion_on  = true;
	mutex_unlock(&wrong_touch_lock);
	TS_LOG_INFO("done\n");
	return rc;
}

static int focal_irq_top_half(struct ts_cmd_node *cmd)
{
	cmd->command = TS_INT_PROCESS;
	return NO_ERR;
}

static int focal_read_roidata(void)
{
	u8 buf[1] = { 0 };
	int ret = -1;
	buf[0] = FTS_ROI_BUFF0_ADDR;
	TS_LOG_DEBUG("%s: Enter!\n", __func__);
	ret = focal_read( buf, 1, focal_roi_data,ROI_DATA_READ_LENGTH);
	if (ret < 0) {
		TS_LOG_ERR("%s  failed.\n", __func__);
		return ret;
	}
	TS_LOG_DEBUG("%s: success ,%d \n", __func__,ret);
	return ret;
}

static int focal_set_finger_number(struct ts_fingers *info,
	struct ts_event *touch_data)
{
	/*
	 * why we need this function ?
	 *         when ts kit found info->cur_finger_number is 0,
	 *     ts kit will report BTN_TOUCH UP, otherwize,
	 *     report BTN_TOUCH DOWN.
	 *         the recovery mode will use BTN_TOUCH UP
	 *     to judge if the finger have release, so we need this event.
	 *
	 * what risk at here ?
	 *         we set cur_finger_number to 0, so the ts kit will not report
	 *     this poit, ts kit will report a BTN_TOUCH UP instead of this
	 *     point. when close FTS_REPORT_BTN_TOUCH, the last two data package
	 *     in input log is like the following two packages, we can open
	 *     FTS_REPORT_BTN_TOUCH, if only the package two is report, we can't
	 *     open FTS_REPORT_BTN_TOUCH.
	 *
	 *     [package 1]
	 *         EV_ABS       ABS_MT_PRESSURE      00000009
	 *         EV_ABS       ABS_MT_POSITION_X    00000434
	 *         EV_ABS       ABS_MT_POSITION_Y    0000075e
	 *         EV_ABS       ABS_MT_TRACKING_ID   00000000
	 *         EV_SYN       SYN_MT_REPORT        00000000
	 *         EV_SYN       SYN_REPORT           00000000
	 *
	 *     [package 2]
	 *         EV_ABS       ABS_MT_PRESSURE      00000000
	 *         EV_ABS       ABS_MT_POSITION_X    00000434
	 *         EV_ABS       ABS_MT_POSITION_Y    0000075e
	 *         EV_ABS       ABS_MT_TRACKING_ID   00000000
	 *         EV_SYN       SYN_MT_REPORT        00000000
	 *         EV_SYN       SYN_REPORT           00000000
	 *
	 * notice: the different between package 1 and package 2 is:
	 *         the value of [package 1]->ABS_MT_POSITION_X is the same as
	 *     [package 2]->ABS_MT_POSITION_X, and so do ABS_MT_POSITION_Y,
	 *     but the ABS_MT_PRESSURE of [package 1] is not 0, and the
	 *     ABS_MT_PRESSURE of [package 2] is 0
	 */

	int i = 0;
	int index = -1;
	u8 check_result = 0;
	struct focal_platform_data *pdata = NULL;
	struct ts_event *last_data = NULL;

	pdata = focal_get_platform_data();
	last_data = &pdata->touch_data;

	/* if this point is the release point, set finger number to 0 */
#ifdef FTS_REPORT_BTN_TOUCH
	if (touch_data->pressure[0] == 0) {
		info->cur_finger_number = 0;

		TS_LOG_DEBUG("%s:%s=%d, %s=%d, %s=%d\n",
				__func__,
				"current finger id", touch_data->finger_id[0],
				"last count", last_data->touch_point,
				"current cound", touch_data->touch_point);

		for (i = 0; i < last_data->touch_point; i++) {
			TS_LOG_DEBUG("%s:last finger id:%d\n",
				__func__, last_data->finger_id[i]);
			if (last_data->finger_id[i] == touch_data->finger_id[0]) {
				index = i;
				break;
			}
		}

		if (index < 0) {
			TS_LOG_ERR("%s:touch data error, %s\n",
				__func__, "please close FTS_REPORT_BTN_TOUCH");
			return -EIO;
		}

		if (last_data->position_x[index] == touch_data->position_x[0]
		&& last_data->position_y[index] == touch_data->position_y[0]) {
			check_result = true;
		}

		if (!check_result) {
			TS_LOG_ERR("%s:not support BTN_TOUCH report, %s\n",
				__func__, "please close FTS_REPORT_BTN_TOUCH");
		}
	} else {
		info->cur_finger_number = touch_data->touch_point;
	}
#else
	info->cur_finger_number = touch_data->touch_point;
#endif

	memcpy(last_data, touch_data, sizeof(struct ts_event));

	return 0;
}

static int focal_easy_wakeup_gesture_report_coordinate(unsigned int
		reprot_gesture_point_num, struct ts_fingers *info, unsigned char *buf)
{
	int retval = 0;
	int x = 0;
	int y = 0;
	unsigned int i = 0;
	int x_pos_0 = 0;
	int x_pos_1 = 0;
	int y_pos_0 = 0;
	int y_pos_1 = 0;
	if (reprot_gesture_point_num != 0) {
		for (i = 0; i < reprot_gesture_point_num; i++) {
			//x_pos: 2,3byte, y_pos:4,5byte
			x_pos_0 = 2 + (4 * i);
			x_pos_1 = 3 + (4 * i);
			y_pos_0 = 4 + (4 * i);
			y_pos_1 = 5 + (4 * i);
			x = (((s16)buf[x_pos_0]) & 0x0F) << 8 | (((s16) buf[x_pos_1])& 0xFF);
			y = (((s16)buf[y_pos_0]) & 0x0F) << 8 | (((s16) buf[y_pos_1])& 0xFF);
			TS_LOG_INFO("%s: Gesture Repot Point %d \n", __func__, i);
			g_focal_dev_data->easy_wakeup_info.easywake_position[i] = (x << 16) | y;
			TS_LOG_INFO("easywake_position[%d] = 0x%04x\n", i, g_focal_dev_data->easy_wakeup_info.easywake_position[i]);
		}
	}
	return retval;
}

static int focal_check_key_gesture_report(struct ts_fingers *info,
	struct ts_easy_wakeup_info *gesture_report_info,
	unsigned char *get_gesture_wakeup_data)
{
	int retval = 0;
	unsigned int reprot_gesture_key_value = 0;
	unsigned int reprot_gesture_point_num = 0;
	TS_LOG_DEBUG("get_gesture_wakeup_data is %d \n", get_gesture_wakeup_data[0]);

	switch (get_gesture_wakeup_data[0]) {
		case DOUBLE_CLICK_WAKEUP:
			if (IS_APP_ENABLE_GESTURE(GESTURE_DOUBLE_CLICK) &
			    gesture_report_info->easy_wakeup_gesture) {
				TS_LOG_ERR("@@@DOUBLE_CLICK_WAKEUP detected!@@@\n");
				reprot_gesture_key_value = TS_DOUBLE_CLICK;
				LOG_JANK_D(JLID_TP_GESTURE_KEY, "JL_TP_GESTURE_KEY");
				reprot_gesture_point_num = LINEAR_LOCUS_NUM;
			}
			break;
		case SPECIFIC_LETTER_C:
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_c) &
			    gesture_report_info->easy_wakeup_gesture) {
				TS_LOG_INFO("@@@SPECIFIC_LETTER_c detected!@@@\n");
				reprot_gesture_key_value = TS_LETTER_c;
				reprot_gesture_point_num = LETTER_LOCUS_NUM;
			}
			break;
		case SPECIFIC_LETTER_E:
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_e) &
			    gesture_report_info->easy_wakeup_gesture) {
				TS_LOG_INFO("@@@SPECIFIC_LETTER_e detected!@@@\n");
				reprot_gesture_key_value = TS_LETTER_e;
				reprot_gesture_point_num = LETTER_LOCUS_NUM;
			}
			break;
		case SPECIFIC_LETTER_M:
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_m) &
			    gesture_report_info->easy_wakeup_gesture) {
				TS_LOG_INFO("@@@SPECIFIC_LETTER_m detected!@@@\n");
				reprot_gesture_key_value = TS_LETTER_m;
				reprot_gesture_point_num = LETTER_LOCUS_NUM;
			}
			break;
		case SPECIFIC_LETTER_W:
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_w) &
			    gesture_report_info->easy_wakeup_gesture) {
				TS_LOG_INFO("@@@SPECIFIC_LETTER_w detected!@@@\n");
				reprot_gesture_key_value = TS_LETTER_w;
				reprot_gesture_point_num = LETTER_LOCUS_NUM;
			}
			break;
		default:
			TS_LOG_INFO("@@@unknow gesture detected!\n");
			return NO_ERR;
	}

	if (0 != reprot_gesture_key_value) {
		/*increase wake_lock time to avoid system suspend.*/
		wake_lock_timeout(&g_focal_dev_data->ts_platform_data->ts_wake_lock, 5 * HZ);
		mutex_lock(&wrong_touch_lock);
		if (true == g_focal_dev_data->easy_wakeup_info.off_motion_on) {
			g_focal_dev_data->easy_wakeup_info.off_motion_on = false;
			retval = focal_easy_wakeup_gesture_report_coordinate(
								  reprot_gesture_point_num,
								  info,
								  get_gesture_wakeup_data);
			if (retval < 0) {
				mutex_unlock(&wrong_touch_lock);
				TS_LOG_INFO
				    ("%s: report line_coordinate error!retval = %d\n",
				     __func__, retval);
				return retval;
			}
			info->gesture_wakeup_value = reprot_gesture_key_value;
			TS_LOG_INFO("%s: info->gesture_wakeup_value = %d\n", __func__, info->gesture_wakeup_value);
		}
		mutex_unlock(&wrong_touch_lock);
	}
	return NO_ERR;
}

static int focal_read_gestrue_data(struct ts_fingers *info,
	struct ts_easy_wakeup_info *gesture_report_info)
{
	unsigned char buf[FTS_GESTRUE_POINTS * 3] = { 0 };
	int ret = 0;
	int gesture_id = 0;
	short pointnum = 0;
	int len = 0;

	buf[0] = FTS_REG_GESTURE_OUTPUT_ADDRESS_D3;
	pointnum = 0;
	ret = focal_read(buf, 1, buf, FTS_GESTRUE_POINTS_HEADER);
	if (ret < 0){
		TS_LOG_ERR( "%s read touchdata failed.\n", __func__);
		return ret;
	}

	gesture_id = buf[0];
	pointnum = (short)(buf[1]) & 0xff;
	//every pointnum is 4 byte length, and have 8 byte for default length. when read length is bigger than 255, need read twice
	len = pointnum * 4 + 8;

	if(gesture_id != DOUBLE_CLICK_WAKEUP){
		buf[0] = FTS_REG_GESTURE_OUTPUT_ADDRESS_D3;
		if(len < FTS_REG_GESTURE_READ_OFFSET){
			ret = focal_read(buf, 1, buf, len);
			if (ret < 0){
				TS_LOG_ERR("%s, %d: have error\n", __func__, __LINE__);
				return RESULT_ERR;
			}
		}else{
			ret = focal_read(buf, 1, buf, FTS_REG_GESTURE_READ_OFFSET);
			if (ret < 0){
				TS_LOG_ERR("%s, %d: read touchdata failed have error\n", __func__, __LINE__);
				return RESULT_ERR;
			}
			len = (len - FTS_REG_GESTURE_READ_OFFSET) > (sizeof(buf) - FTS_REG_GESTURE_READ_OFFSET) ?
				(sizeof(buf) - FTS_REG_GESTURE_READ_OFFSET):(len - FTS_REG_GESTURE_READ_OFFSET);
			ret = focal_read(buf, 0, buf+FTS_REG_GESTURE_READ_OFFSET, len);
			if (ret < 0){
				TS_LOG_ERR("%s, %d: read touchdata failed have error\n", __func__, __LINE__);
				return RESULT_ERR;
			}
		}
	}

	ret = focal_check_key_gesture_report(info, gesture_report_info, buf);
	if (ret < 0){
		TS_LOG_ERR("%s, %d:  have error\n", __func__, __LINE__);
		return RESULT_ERR;
	}

	TS_LOG_INFO("gesture_id = 0x%x \n",gesture_id);
	return NO_ERR;
}

static int focal_check_gesture(struct ts_fingers *info)
{
	int retval = 0;
	unsigned char state = 0;
	unsigned char state_reg = 0;
	u8 buf[FTS_TOUCH_DATA_LEN] = {0};

	struct ts_easy_wakeup_info *gesture_report_info = &g_focal_dev_data->easy_wakeup_info;
	if (g_focal_dev_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value == 0 ||
		false == gesture_report_info->easy_wakeup_flag){
		return true;
	}

	buf[0] = TOUCH_DATA_START_ADDR;
	retval = focal_read(buf, FTS_READ_REG_LEN, buf, FTS_TOUCH_DATA_LEN);
	if (retval < 0) {
		TS_LOG_ERR("%s:read touchdata failed, ret=%d.\n",__func__, retval);
		return RESULT_ERR;
	}

	/*get gesture wake up value, read status register 0xd0 */
	state_reg = FTS_REG_GESTURE_EN;
	retval = focal_read_reg(state_reg,&state);
	if (retval < 0) {
		TS_LOG_ERR("%s read state fail \n",__func__);
		return RESULT_ERR;
	}

	if(state == FTS_GESTURE_EANBLE){
		retval = focal_read_gestrue_data(info, gesture_report_info);
		if(retval != NO_ERR){
			TS_LOG_ERR("%s, %d: have error\n", __func__, __LINE__);
			return RESULT_ERR;
		}
	}else{
		TS_LOG_ERR("%s read state:%d \n",__func__, state);
		return RESULT_ERR;
	}

	return NO_ERR;
}

static int focal_irq_bottom_half(struct ts_cmd_node *in_cmd,
	struct ts_cmd_node *out_cmd)
{
	int x = 0;
	int y = 0;
	int z = 0;
	int wx = 0;
	int wy = 0;
	int ewx = 0;
	int ewy = 0;
	int xer = 0;
	int yer = 0;

	int i = 0;
	int ret = 0;
	int touch_count = 0;
	u8 roi_package_num= 0;
	struct ts_event st_touch_data;
	struct algo_param *algo_p = NULL;
	struct ts_fingers *info = NULL;

	algo_p = &out_cmd->cmd_param.pub_params.algo_param;
	info = &algo_p->info;

	out_cmd->command = TS_INPUT_ALGO;
	algo_p->algo_order = g_focal_dev_data->algo_id;
	TS_LOG_DEBUG("%s:algo_order:%d\n", __func__, algo_p->algo_order);

	if(true == g_focal_dev_data->need_wd_check_status){
		fts_esdcheck_data.intr = true;
	}

	ret = focal_check_gesture(info);
	if (!ret) {
		TS_LOG_DEBUG("focal_gesture_report is called and report gesture\n");
		return ret;
	}

	ret = focal_read_touch_data(&st_touch_data);
	if (ret){
		if(true == g_focal_dev_data->need_wd_check_status){
			fts_esdcheck_data.intr = false;
		}
		return ret;
	}

	for (i = 0; i < st_touch_data.touch_point; i++) {
		x = st_touch_data.position_x[i];
		y = st_touch_data.position_y[i];
		z = st_touch_data.pressure[i];
		if (g_focal_pdata->enable_edge_touch) {
			ewx = st_touch_data.ewx[i];
			ewy = st_touch_data.ewy[i];
			xer = st_touch_data.xer[i];
			yer = st_touch_data.yer[i];
		}
		if (g_focal_pdata->aft_wxy_enable) {
			wx = st_touch_data.wx[i];
			wy = st_touch_data.wy[i];
		} else {
			wx = st_touch_data.area[i];
			wy = st_touch_data.area[i];
		}

		info->fingers[st_touch_data.finger_id[i]].status = TP_FINGER;
		info->fingers[st_touch_data.finger_id[i]].x = x;
		info->fingers[st_touch_data.finger_id[i]].y = y;
		info->fingers[st_touch_data.finger_id[i]].pressure = z;
		if (g_focal_pdata->enable_edge_touch) {
			info->fingers[st_touch_data.finger_id[i]].ewx = ewx;
			info->fingers[st_touch_data.finger_id[i]].ewy = ewy;
			info->fingers[st_touch_data.finger_id[i]].xer = xer;
			info->fingers[st_touch_data.finger_id[i]].yer = yer;
		}
		if (g_focal_pdata->aft_wxy_enable) {
			info->fingers[st_touch_data.finger_id[i]].wx = wx;
			info->fingers[st_touch_data.finger_id[i]].wy = wy;
		} else {
			info->fingers[st_touch_data.finger_id[i]].major = wx;
			info->fingers[st_touch_data.finger_id[i]].minor = wy;
		}

		touch_count++;

		TS_LOG_DEBUG("%s:%d:x = %d; y = %d; wx = %d; wy = %d\n",
			 __func__,
			 st_touch_data.finger_id[i], x, y, wx, wy);
	}
#ifdef ROI
	if (TS_BUS_I2C == g_focal_dev_data->ts_platform_data->bops->btype) {
		if(g_focal_dev_data->ts_platform_data->feature_info.roi_info.roi_switch
			&& g_focal_dev_data->ts_platform_data->feature_info.roi_info.roi_supported){
			if (g_focal_pdata->roi_pkg_num_addr) {
				focal_read_reg((u8)(g_focal_pdata->roi_pkg_num_addr), &roi_package_num);
			} else {
				focal_read_reg(FTS_ROI_PACKAGE_NUM, &roi_package_num);
			}

			if(roi_package_num > 0){
				//focal_roi_data[ROI_DATA_READ_LENGTH] = roi_package_num;
				focal_read_roidata();
			}
		}
	}
#endif
	//info->cur_finger_number = touch_count;
	focal_set_finger_number(info, &st_touch_data);
	TS_LOG_DEBUG("%s:touch_count = %d\n", __func__, touch_count);

	if(true == g_focal_dev_data->need_wd_check_status){
		fts_esdcheck_data.intr = false;
	}

	return NO_ERR;
}
static int focal_get_brightness_info(void)
{
	int error = NO_ERR;
	return error ;
}

static int focal_fw_update_boot(char *file_name)
{
	return focal_firmware_auto_update(g_focal_pdata, file_name);
}

static int focal_fw_update_boot_resume(void)
{
	int tskit_pt_station_flag = 0;

	ts_kit_get_pt_station_status(&tskit_pt_station_flag);
	if (!tskit_pt_station_flag)
		return focal_fw_update_boot(g_focal_pdata->fw_name);

	return 0;
}

static int focal_fw_update_boot_recovery(void)
{
	int ret = 0;
	u8 boot_state = 0;

	TS_LOG_INFO("%s:check if boot recovery\n", __func__);

	ret = focal_read_reg(FTS_REG_VALUE_0XD0, &boot_state);
	if (ret < 0) {
		TS_LOG_ERR("%s:read boot state failed, ret=%d.\n",
			__func__, ret);
		return ret;
	}

	if (boot_state != FTS_FW_DOWNLOAD_MODE) {
		TS_LOG_INFO("%s:not in download mode,exit\n", __func__);
		return -EIO;
	}

	TS_LOG_INFO("%s:abnormal situation,need download fw", __func__);

	ret = focal_fw_update_boot_resume();
	if (ret < 0) {
		TS_LOG_ERR("%s:focal_fw_update_boot_resume fail", __func__);
		return ret;
	}

	ret = focal_status_resume();
	if (ret < 0) {
		TS_LOG_ERR("status resume  err: %d\n",  ret);
		return ret;
	}

	return ret;
}

static int focal_fw_update_sd(void)
{
	int ret =0 ;
	ret =  focal_firmware_manual_update(g_focal_pdata,
		FTS_FW_MANUAL_UPDATE_FILE_NAME);
	if (ret < 0) {
		TS_LOG_ERR("Failed to update fw sd  err: %d\n",  ret);
		return ret;
	}
	ret = focal_status_resume();
	if (ret < 0) {
		TS_LOG_ERR("status resume  err: %d\n",  ret);
	}
	return ret ;
}

static int focal_chip_get_info(struct ts_chip_info_param *info)
{
	size_t ic_vendor_size = 0;
	size_t fw_vendor_size = 0;

	struct focal_platform_data *focal_pdata = NULL;

	focal_pdata = focal_get_platform_data();
	if(!focal_pdata || !info || !focal_pdata->focal_device_data ||
			!focal_pdata->focal_device_data->ts_platform_data) {
		TS_LOG_ERR("%s: focal_pdata or info NULL\n", __func__);
		return -EINVAL;
	}

	ic_vendor_size = CHIP_INFO_LENGTH * 2;
	if(!focal_pdata->focal_device_data->ts_platform_data->hide_plain_id) {
		strncpy(info->ic_vendor, FTS_CHIP_NAME, ic_vendor_size - 1);
		focal_strncat(info->ic_vendor, "-", ic_vendor_size);
		focal_strncat(info->ic_vendor, focal_pdata->project_id, ic_vendor_size);
	} else {
		strncpy(info->ic_vendor, focal_pdata->project_id,
				(ic_vendor_size - 1) < sizeof(focal_pdata->project_id) ?
				(ic_vendor_size - 1) :
				sizeof(focal_pdata->project_id));
	}

	strncpy(info->mod_vendor, focal_pdata->vendor_name, CHIP_INFO_LENGTH);

	fw_vendor_size = CHIP_INFO_LENGTH * 2;
	snprintf(info->fw_vendor, fw_vendor_size, "%d", focal_pdata->fw_ver);

	return NO_ERR;
}

static int focal_set_info_flag(struct ts_kit_platform_data *info)
{
	return NO_ERR;
}

static int focal_before_suspend(void)
{
	return NO_ERR;
}


static void focal_sleep_mode_in(void)
{
	int retval = 0;
	retval = focal_write_reg(FTS_REG_SLEEP, 0x03);  //make ic to be sleep
	if(retval < 0){
		TS_LOG_ERR("%s, %d: have error\n", __func__, __LINE__);
	}
	msleep(20);
	return;
}


static void focal_put_device_into_easy_wakeup(void)
{
	int retval = 0;
	unsigned char write_addr, write_data;

	struct ts_easy_wakeup_info *info = &g_focal_dev_data->easy_wakeup_info;
	TS_LOG_DEBUG("focal_put_device_into_easy_wakeup  info->easy_wakeup_flag =%x \n",
	     			info->easy_wakeup_flag);
	/*if the sleep_gesture_flag is ture,it presents that  the tp is at sleep state*/

	if (g_focal_dev_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value == false ||
		true == info->easy_wakeup_flag) {
		TS_LOG_INFO
		    ("focal_put_device_into_easy_wakeup  info->easy_wakeup_flag =%x \n",
		     info->easy_wakeup_flag);
		return;
	}

	/*Wakeup Gesture reg (d0) set enable 1 */
	write_addr = FTS_REG_GESTURE_EN;
	write_data = 0x01;
	retval = focal_write_reg(write_addr, write_data);
	if(retval){
		goto write_reg_exit;
	}

	//reg 0x d1-d8 is gesture id, write 0xff for open this function
	write_addr = FTS_REG_GESTURE_OUTPUT_ADDRESS_D1;
	write_data = FTS_REG_GESTURE_OPEN_FF;
	retval = focal_write_reg(write_addr, write_data);
	if(retval){
		goto write_reg_exit;
	}

	write_addr = FTS_REG_GESTURE_OUTPUT_ADDRESS_D2;
	retval = focal_write_reg(write_addr, write_data);
	if(retval){
		goto write_reg_exit;
	}
	write_addr = FTS_REG_GESTURE_OUTPUT_ADDRESS_D5;
	retval = focal_write_reg(write_addr, write_data);
	if(retval)
		goto write_reg_exit;

	write_addr = FTS_REG_GESTURE_OUTPUT_ADDRESS_D6;
	retval = focal_write_reg(write_addr, write_data);
	if(retval)
		goto write_reg_exit;

	write_addr = FTS_REG_GESTURE_OUTPUT_ADDRESS_D7;
	retval = focal_write_reg(write_addr, write_data);
	if(retval)
		goto write_reg_exit;

	write_addr = FTS_REG_GESTURE_OUTPUT_ADDRESS_D8;
	retval = focal_write_reg(write_addr, write_data);
	if(retval)
		goto write_reg_exit;

write_reg_exit:
	if (retval < 0) {
		TS_LOG_ERR("easy wake up suspend write Wakeup Gesture Only reg fail addr %0d!\n",write_addr);
	} else {
		TS_LOG_INFO("easy wake up suspend write Wakeup Gesture Only reg OK address(0x%02x) valve(0x%02x)\n",
		     write_addr, write_data);
	}

	info->easy_wakeup_flag = true;
	return;
}

static int focal_suspend(void)
{
	int tskit_pt_station_flag = 0;
	int ret = NO_ERR;
	int error = 0;

	ret = ts_kit_get_pt_station_status(&tskit_pt_station_flag);
	if(ret){
		tskit_pt_station_flag = 0;
		TS_LOG_INFO("get pt_station_status faile, user default [0]");
	}
	TS_LOG_INFO("tskit_pt_station_flag = %d\n", tskit_pt_station_flag);
	if(true == g_focal_dev_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value){
		switch(g_focal_dev_data->easy_wakeup_info.sleep_mode){
		case TS_POWER_OFF_MODE:
			if (FTS_POWER_DOWN == g_focal_pdata->self_ctrl_power && !tskit_pt_station_flag) {
				error = focal_reset_output(0);
				if(error){
					TS_LOG_ERR("%s:gpio direction output fail, ret=%d\n",__func__, error);
				}
				udelay(FT5X46_RESET_KEEP_LOW_TIME);
				focal_power_off();
			} else {
				/*goto sleep mode*/
				TS_LOG_INFO("%s: enter sleep_mode\n", __func__);
				focal_sleep_mode_in();
			}
			focal_pinctrl_select_suspend();
			break;
		case TS_GESTURE_MODE:
			enable_irq_wake(g_focal_dev_data->ts_platform_data->irq_id);
			focal_put_device_into_easy_wakeup();
			mutex_lock(&wrong_touch_lock);
			g_focal_dev_data->easy_wakeup_info.off_motion_on = true;
			mutex_unlock(&wrong_touch_lock);
			break;
		default:
			break;
		}
	}else{
		if (FTS_POWER_DOWN == g_focal_pdata->self_ctrl_power && !tskit_pt_station_flag) {
			error = focal_reset_output(0);
			if(error){
				TS_LOG_ERR("%s:gpio direction output fail, ret=%d\n",__func__, error);
			}
			udelay(FT5X46_RESET_KEEP_LOW_TIME);
			focal_power_off();
		}else if(FTS_POWER_DOWN == g_focal_pdata->self_ctrl_reset && !tskit_pt_station_flag){
			error = focal_reset_output(0);
			if(error){
				TS_LOG_ERR("%s:gpio direction output fail, ret=%d\n",__func__, error);
			}
			if (TS_BUS_SPI == g_focal_dev_data->ts_platform_data->bops->btype) {
				focal_cs_output(0);
			}
			TS_LOG_INFO("reset  low\n");
		}else {
			/**ft8201 don't self ctrl power,lcd suspend turn off vddio 1.8v,so don't need write 0x03 to oxA5,
			   but need set reset gpio low**/
			if (FOCAL_FT8201 != g_focal_dev_data->ic_type) {
				error = focal_write_reg(FTS_REG_SLEEP, 0x03);
				if(error){
					TS_LOG_ERR("%s: failed to set seelp model!ret=%d\n",__func__, error);
				}
			}else{
				/*ft8201 is incell ic,lcd sequence just need set reset gpio low*/
				error = focal_reset_output(0);
				if(error){
					TS_LOG_ERR("%s:gpio direction output fail, ret=%d\n",__func__, error);
				}
			}
		}
		focal_pinctrl_select_suspend();
	}
	if((FOCAL_FT8719 == g_focal_dev_data->ic_type) && (!tskit_pt_station_flag)){
		TS_LOG_INFO("%s: set fw_is_running to false \n", __func__);
		g_focal_pdata->fw_is_running = false;
	}
	TS_LOG_INFO("Suspend end");

	return NO_ERR;
}

static int focal_sleep_mode_out(void)
{
	int retval = 0;

	retval = focal_hardware_reset_to_normal_model();
	if(retval != NO_ERR){
		TS_LOG_ERR("%s: have error\n", __func__);
	}

	return NO_ERR;
}

static void focal_put_device_outof_easy_wakeup(void)
{
	int retval = 0;
	struct ts_easy_wakeup_info *info = &g_focal_dev_data->easy_wakeup_info;

	TS_LOG_DEBUG("focal_put_device_outof_easy_wakeup  info->easy_wakeup_flag =%d\n", info->easy_wakeup_flag);

	if (false == info->easy_wakeup_flag) {
		return;
	}

	/*Wakeup Gesture Only bit(01) set 0 not enable */
	retval = focal_write_reg(FTS_REG_GESTURE_EN,0x00);
	if (retval < 0) {
		TS_LOG_ERR("easy wake up resume write Wakeup Gesture Only reg fail\n");
		return;
	}else {
		TS_LOG_INFO("easy wake up suspend write Wakeup Gesture Only reg OK address(0x%02x) valve(0x%02x)\n",
					FTS_REG_GESTURE_EN, 0x00);
	}

	info->easy_wakeup_flag = false;
	return;
}

static int focal_resume(void)
{
	int ret = NO_ERR;
	int reset_gpio = 0;
	int tskit_pt_station_flag = 0;

	ret = ts_kit_get_pt_station_status(&tskit_pt_station_flag);
	if(ret){
		tskit_pt_station_flag = 0;
		TS_LOG_INFO("get pt_station_status faile, user default [0]");
	}

	if(FOCAL_FT8719 == g_focal_dev_data->ic_type){
		TS_LOG_INFO("%s, %d: FT8719  pull up\n", __func__, __LINE__);
		ret = focal_reset_output(1);
		if(NO_ERR != ret){
			TS_LOG_ERR("%s, %d: FT8719  pull up have error\n", __func__, __LINE__);
		}
	}

	/*ft8201 lcd need tp to set reset gpio high,so need enter resume logic*/
	if((FOCAL_FT5X46 != g_focal_dev_data->ic_type) && (FOCAL_FT8201 != g_focal_dev_data->ic_type)){
		TS_LOG_INFO("%s: tp ic isn't FT5X46 or FT8201, resume needn't do nothing\n", __func__);
		return ret;
	}

	if(true == g_focal_dev_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value){
		switch (g_focal_pdata->focal_device_data->easy_wakeup_info.sleep_mode) {
		case TS_POWER_OFF_MODE:
			if (FTS_POWER_DOWN== g_focal_pdata->self_ctrl_power && !tskit_pt_station_flag) {
				/*In suspend function reset pull down, Wakeup reset not need pull down*/
				focal_power_on();
				udelay(FT5X46_RESET_KEEP_LOW_TIME);
				ret = focal_reset_output(1);
				if(ret){
					TS_LOG_ERR("%s:gpio direction output fail, ret=%d\n",__func__, ret);
				}
			} else {
				/*exit sleep mode*/
				ret = focal_gpio_reset();
				if(NO_ERR != ret){
					TS_LOG_ERR("%s, %d: have error\n", __func__, __LINE__);
				}
			}
			break;
		case TS_GESTURE_MODE:
			focal_put_device_outof_easy_wakeup();
			ret = focal_gpio_reset();
			if(NO_ERR != ret){
				TS_LOG_DEBUG("%s: have error\n", __func__);
			}
			break;
		default:
			TS_LOG_ERR("no resume mode\n");
			return ret;
		}
	}else {
		if (FTS_POWER_DOWN == g_focal_pdata->self_ctrl_power && !tskit_pt_station_flag) {
			/*In suspend function reset pull down, Wakeup reset not need pull down*/
			focal_power_on();
			udelay(FT5X46_RESET_KEEP_LOW_TIME);
			ret = focal_reset_output(1);
			if(ret){
				TS_LOG_ERR("%s:gpio direction output fail, ret=%d\n",__func__, ret);
			}
		}else{
			/*exit sleep mode*/
			if (FOCAL_FT8201 != g_focal_dev_data->ic_type) {
				ret = focal_gpio_reset();
				if(NO_ERR != ret){
					TS_LOG_ERR("%s, %d: have error\n", __func__, __LINE__);
				}
			} else {
				/*ft8201 is incell ic,lcd sequence just need set reset gpio high*/
				ret = focal_reset_output(1);
				if(ret){
					TS_LOG_ERR("%s:gpio direction output fail, ret=%d\n",__func__, ret);
				}
			}
		}
	}

	return ret;
}

static int focal_status_resume(void)
{
	int retval = 0;
	struct ts_feature_info *info = &g_focal_dev_data->ts_platform_data->feature_info;

       if(info->holster_info.holster_supported){
		retval =focal_set_holster_switch(info->holster_info.holster_switch);
		if (retval < 0) {
			TS_LOG_ERR("Failed to set holster switch(%d), err: %d\n",
				   info->holster_info.holster_switch, retval);
		}
       }

	if (info->roi_info.roi_supported) {
		retval = focal_set_roi_switch(info->roi_info.roi_switch);
		if (retval < 0) {
			TS_LOG_ERR("%s, synaptics_set_roi_switch faild\n",
				   __func__);
		}
	}

      if(info->glove_info.glove_supported){
		retval = focal_set_glove_switch(info->glove_info.glove_switch);
		if (retval < 0) {
			TS_LOG_ERR("Failed to set glove switch(%d), err: %d\n",
				   info->glove_info.glove_switch, retval);
		}
	}
	TS_LOG_INFO(" glove_switch (%d), holster switch(%d), roi_switch(%d) \n",
			   info->glove_info.glove_switch,info->holster_info.holster_switch,info->roi_info.roi_switch);
	return retval;
}

static int focal_after_resume(void *feature_info)
{
	int ret = 0;
	int i =0;
	u8 reg_val=0;
	u8 cmd= FTS_REG_CHIP_ID_H;
	u8 chipid_high =(g_focal_pdata->chip_id >>8 )&0xff;

	if(FOCAL_FT5X46 == g_focal_dev_data->ic_type){
		msleep(FTS_SLEEP_TIME_220);
	}
	/*ft8201 lcd and tp need 300ms for fw load time,lcd set 35ms,tp need 265ms*/
	if(FOCAL_FT8201 == g_focal_dev_data->ic_type){
		msleep(FTS_SLEEP_TIME_265);
	}
	struct timeval time_duration, time_start, time_end;
	unsigned int fw_update_duration_check = g_focal_pdata->fw_update_duration_check;

	if (TS_BUS_SPI == g_focal_dev_data->ts_platform_data->bops->btype) {
		focal_cs_output(GPIO_HIGH);

		do_gettimeofday(&time_start);
		ret = focal_fw_update_boot_resume();
		do_gettimeofday(&time_end);
		time_duration.tv_sec = time_end.tv_sec - time_start.tv_sec;
		time_duration.tv_usec = time_end.tv_usec - time_start.tv_usec;
		if(fw_update_duration_check) {
			if((time_duration.tv_sec * 1000000 + time_duration.tv_usec) > fw_update_duration_check * 1000) {
#if defined (CONFIG_HUAWEI_DSM)
				ts_dmd_report( DSM_TP_FWUPDATE_OVERTIME_ERROR_NO,
						"error.%s:for spi:fw update time too long. duration(%d.%d) = %ld ums\n",
						__func__, time_duration.tv_sec, time_duration.tv_usec,
						time_duration.tv_sec * 1000000 + time_duration.tv_usec);
#endif
			}
		}

		if (ret < 0) {
			TS_LOG_ERR("%s:fw update from resume fail,ret=%d", __func__, ret);
			return ret;
		}
		TS_LOG_INFO("%s:fw update from resume for sip, check_threshold(%ld ums).duration(%d.%d) = %ld ums\n",
				__func__, fw_update_duration_check * 1000,
				time_duration.tv_sec, time_duration.tv_usec,
				time_duration.tv_sec * 1000000 + time_duration.tv_usec);

	}
	for(i=0;i<FTS_RESUME_MAX_TIMES;i++)
	{
		ret = focal_read(&cmd, 1, &reg_val, 1);
		if ((reg_val  == chipid_high) || (reg_val  == FTS_FT86XX_HIGH)) {
			TS_LOG_INFO("%s:chip id read success, chip id:0x%X, i=%d\n",__func__, reg_val,i);
			break;
		}
		else{
			TS_LOG_ERR("%s:chip id read fail, reg_val=%d, chipid_high=%d, i=%d\n",__func__, reg_val, chipid_high, i);
			msleep(15);
		}
	}
	if(i == FTS_RESUME_MAX_TIMES)
	{
		TS_LOG_ERR("%s:chip id read fail, ret=%d, i=%d\n",__func__, reg_val, i);
		return -EINVAL;
	}
	ret = focal_status_resume();
	if(ret < 0)
	{
		TS_LOG_ERR("%s: failed to resume status\n",__func__, ret);
		return -EINVAL;
	}

	if(true == g_focal_dev_data->need_wd_check_status){
		fts_esdcheck_data.suspend = false;
	}

	return ret;
}

static int focal_wakeup_gesture_enable_switch(
	struct ts_wakeup_gesture_enable_info *info)
{

	return NO_ERR;
}

static void focal_shutdown(void)
{

}

static int focal_input_config(struct input_dev *input_dev)
{
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);
	set_bit(TS_PALM_COVERED, input_dev->keybit);
	set_bit(TS_DOUBLE_CLICK, input_dev->keybit);
	set_bit(TS_LETTER_c, input_dev->keybit);
	set_bit(TS_LETTER_e, input_dev->keybit);
	set_bit(TS_LETTER_m, input_dev->keybit);
	set_bit(TS_LETTER_w, input_dev->keybit);

#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
#endif

	input_set_abs_params(input_dev, ABS_X,
		0, (g_focal_dev_data->x_max - 1), 0, 0);
	input_set_abs_params(input_dev, ABS_Y,
		0, (g_focal_dev_data->y_max - 1), 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 10, 0, 0);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0,
		(g_focal_dev_data->x_max - 1), 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0,
		(g_focal_dev_data->y_max - 1), 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, ABS_MT_TOUCH_MAJOR_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR, 0, ABS_MT_TOUCH_MINOR_MAX, 0, 0);

#ifdef TYPE_B_PROTOCOL
#ifdef KERNEL_ABOVE_3_7
	/* input_mt_init_slots now has a "flags" parameter */
	input_mt_init_slots(input_dev, 10, INPUT_MT_DIRECT);
#else
	input_mt_init_slots(input_dev, 10);
#endif
#endif

	return NO_ERR;
}

static int focal_reset_device(void)
{
	return focal_hardware_reset_to_normal_model();
}

static int focal_palm_switch(struct ts_palm_info *info)
{
	return NO_ERR;
}

static int focal_glove_switch(struct ts_glove_info *info)
{
	int ret = 0;

	if (!info) {
		TS_LOG_ERR("%s:info is null\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	switch (info->op_action) {
	case TS_ACTION_READ:
		ret = focal_get_glove_switch(&info->glove_switch);
		if (ret) {
			TS_LOG_ERR("%s:get glove switch fail,ret=%d\n",
				__func__, ret);
			return ret;
		} else {
			TS_LOG_INFO("%s:glove switch=%d\n",
				__func__, info->glove_switch);
		}

		break;
	case TS_ACTION_WRITE:
		TS_LOG_INFO("%s:glove switch=%d\n",
			__func__, info->glove_switch);
		ret = focal_set_glove_switch(!!info->glove_switch);
		if (ret) {
			TS_LOG_ERR("%s:set glove switch fail, ret=%d\n",
				__func__, ret);
			return ret;
		}

		break;
	default:
		TS_LOG_ERR("%s:invalid op action:%d\n",
			__func__, info->op_action);
		return -EINVAL;
	}

	return 0;
}

static struct ts_glove_info *focal_get_glove_info(
	struct ts_kit_device_data *dev_data)
{
	return &dev_data->ts_platform_data->feature_info.glove_info;
}

static int focal_get_glove_switch(u8 *glove_switch)
{
	int ret = 0;

	u8 cmd = 0;
	u8 glove_value = 0;
	u8 glove_enable_addr;

	struct ts_glove_info *glove_info = NULL;

	glove_info = focal_get_glove_info(g_focal_dev_data);
	glove_enable_addr = glove_info->glove_switch_addr;

	TS_LOG_INFO("%s:glove_enable_addr=%d\n", __func__, glove_enable_addr);
	if (!glove_enable_addr) {
		TS_LOG_ERR("%s:glove addr is 0, glove feature not support\n",
			__func__);
		return -ENOTSUPP;
	}

	cmd = glove_enable_addr;

	ret = focal_read_reg(cmd, &glove_value);
	if (ret) {
		TS_LOG_ERR("%s:read glove switch fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	*glove_switch = glove_value;

	TS_LOG_INFO("%s:glove value=%d\n", __func__, *glove_switch);
	return ret;
}

static int focal_set_glove_switch(u8 glove_switch)
{
	int ret = 0;

	u8 cmd[2] = {0};
	u8 glove_value = 0;
	u8 glove_enable_addr = 0;


	struct ts_glove_info *glove_info = NULL;

	glove_info = focal_get_glove_info(g_focal_dev_data);
	glove_enable_addr = glove_info->glove_switch_addr;

	TS_LOG_INFO("%s:glove_enable_addr=%d\n", __func__, glove_enable_addr);
	if (!glove_enable_addr) {
		TS_LOG_ERR("%s:glove_enable_addr is 0, not support glove.\n",
			__func__);

		return -ENOTSUPP;
	}

	if (glove_switch)
		glove_value = 1;
	else
		glove_value = 0;

	cmd[0] = glove_enable_addr;
	cmd[1] = glove_value;

	ret = focal_write(cmd, 2);
	if (ret) {
		TS_LOG_ERR("%s:write glove switch fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	return 0;
}

#if defined(HUAWEI_CHARGER_FB)
static int focal_charger_switch(struct ts_charger_info *info)
{
	return NO_ERR;
}
#endif

static struct ts_holster_info *focal_get_holster_info(
	struct ts_kit_device_data *dev_data)
{
	return &dev_data->ts_platform_data->feature_info.holster_info;
}

static int focal_get_holster_switch(u8 *holster_switch)
{
	int ret = 0;

	u8 cmd = 0;
	u8 holster_value = 0;
	u8 holster_switch_addr;

	struct ts_holster_info *holster_info = NULL;

	holster_info = focal_get_holster_info(g_focal_dev_data);
	holster_switch_addr = holster_info->holster_switch_addr;

	TS_LOG_INFO("%s:holster_switch_addr=%d\n", __func__, holster_switch_addr);
	if (!holster_switch_addr) {
		TS_LOG_ERR("%s:holster_switch_addr is 0, not support holster.\n",
			__func__);

		return -ENOTSUPP;
	}

	cmd = holster_switch_addr;

	ret = focal_read_reg(cmd, &holster_value);
	if (ret) {
		TS_LOG_ERR("%s:read holsterswitch fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	*holster_switch = holster_value;

	TS_LOG_INFO("%s: holster_value=%d\n", __func__, *holster_switch);
	return ret;
}


static int focal_set_holster_switch(u8 holster_switch)
{
	int ret = 0;

	u8 cmd[2] = {0};
	u8 holster_value = 0;
	u8 holster_switch_addr = 0;


	struct ts_holster_info *holster_info = NULL;

	holster_info = focal_get_holster_info(g_focal_dev_data);
	holster_switch_addr = holster_info->holster_switch_addr;

	TS_LOG_INFO("%s:holster_switch_addr=%d\n", __func__, holster_switch_addr);
	if (!holster_switch_addr) {
		TS_LOG_ERR("%s:holster_switch_addr is 0, not support holster.\n",
			__func__);

		return -ENOTSUPP;
	}

	if (holster_switch)
		holster_value = 1;
	else
		holster_value = 0;

	cmd[0] = holster_switch_addr;
	cmd[1] = holster_value;

	ret = focal_write(cmd, 2);
	if (ret) {
		TS_LOG_ERR("%s:write holster switch fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	return 0;
}
static int focal_holster_switch(struct ts_holster_info  *info)
{
	int ret = 0;

	if (!info) {
		TS_LOG_ERR("%s:info is null\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	switch (info->op_action) {
	case TS_ACTION_READ:
		ret = focal_get_holster_switch(&info->holster_switch);
		if (ret) {
			TS_LOG_ERR("%s:get glove switch fail,ret=%d\n",
				__func__, ret);
			return ret;
		} else {
			TS_LOG_INFO("%s:glove switch=%d\n",
				__func__, info->holster_switch);
		}

		break;
	case TS_ACTION_WRITE:
		TS_LOG_INFO("%s:glove switch=%d\n",
			__func__, info->holster_switch);
		ret = focal_set_holster_switch(!!info->holster_switch);
		if (ret) {
			TS_LOG_ERR("%s:set glove switch fail, ret=%d\n",
				__func__, ret);
			return ret;
		}

		break;
	default:
		TS_LOG_ERR("%s:invalid op action:%d\n",
			__func__, info->op_action);
		return -EINVAL;
	}

	return 0;
}

static struct ts_roi_info *focal_get_roi_info(
	struct ts_kit_device_data *dev_data)
{
	return &dev_data->ts_platform_data->feature_info.roi_info;
}
static int focal_set_roi_switch(u8 roi_switch)
{
	int ret = 0;

	u8 cmd[2] = {0};
	u8 roi_value = 0;
	u8 roi_switch_addr =0;
	struct ts_roi_info *roi_info = NULL;
#ifdef ROI
	roi_info = focal_get_roi_info(g_focal_dev_data);
	roi_switch_addr = roi_info->roi_control_addr;

	TS_LOG_INFO("%s:roi_switch_addr=%d\n", __func__, roi_switch_addr);
	if (!roi_switch_addr) {
		TS_LOG_ERR("%s:roi_switch_addr is 0, not support roi.\n",
			__func__);

		return -ENOTSUPP;
	}

	if (roi_switch)
		roi_value = 1;
	else
		roi_value = 0;

	cmd[0] = roi_switch_addr;
	cmd[1] = roi_value;

	ret = focal_write(cmd, 2);
	if (ret) {
		TS_LOG_ERR("%s:write holster switch fail, ret=%d\n",
			__func__, ret);
		return ret;
	}
#endif
	return 0;
}

static int focal_get_roi_switch(u8 *roi_switch)
{
	int ret = 0;

	u8 cmd = 0;
	u8 roi_value =0;
	u8 roi_switch_addr =0;

	struct ts_roi_info *roi_info = NULL;
#ifdef ROI
	roi_info = focal_get_roi_info(g_focal_dev_data);
	roi_switch_addr = roi_info->roi_control_addr;

	TS_LOG_INFO("%s:roi_switch_addr=%d\n", __func__, roi_switch_addr);
	if (!roi_switch_addr) {
		TS_LOG_ERR("%s:roi_switch_addr is 0, not support roi.\n",
			__func__);

		return -ENOTSUPP;
	}

	cmd = roi_switch_addr;

	ret = focal_read_reg(cmd, &roi_value);
	if (ret) {
		TS_LOG_ERR("%s:read roi  switch fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	*roi_switch = roi_value;

	TS_LOG_INFO("%s: roi_value=%d\n", __func__, *roi_switch);
#endif
	return ret;
}
static int focal_roi_switch(struct ts_roi_info *info)
{
	int ret = 0;
	int i=0;
#ifdef ROI
	TS_LOG_INFO("%s: Enter!\n", __func__);
	if (!info) {
		TS_LOG_ERR("%s:info is null\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	switch (info->op_action) {
	case TS_ACTION_READ:
		ret = focal_get_roi_switch(&info->roi_switch);
		if (ret) {
			TS_LOG_ERR("%s:get roi switch fail,ret=%d\n",
				__func__, ret);
			return ret;
		} else {
			TS_LOG_INFO("%s:roi switch=%d\n",
				__func__, info->roi_switch);
		}

		break;
	case TS_ACTION_WRITE:
		TS_LOG_INFO("%s:roi switch=%d\n",
			__func__, info->roi_switch);
		ret = focal_set_roi_switch(!!info->roi_switch);
		if (ret) {
			TS_LOG_ERR("%s:set roi switch fail, ret=%d\n",
				__func__, ret);
			return ret;
		}
		if(!info->roi_switch){
			for (i = 0; i < ROI_DATA_READ_LENGTH; i++) {
				focal_roi_data[i] = 0;
			}
		}

		break;
	default:
		TS_LOG_ERR("%s:invalid op action:%d\n",
			__func__, info->op_action);
		return -EINVAL;
	}
#endif
	return 0;
}

static unsigned char *focal_roi_rawdata(void)
{
#ifdef ROI
	return (unsigned char *)focal_roi_data;
#else
	return NULL;
#endif
}

#define FTS_DOZE_MAX_INPUT_SEPARATE_NUM 2
static void focal_set_doze_mode(unsigned int soper, unsigned char param)
{
	struct ts_kit_device_data *focal_dev_data = g_focal_dev_data;
	int error = 0;

	if (NULL == focal_dev_data){
		TS_LOG_ERR("%s, error chip data\n",__func__);
		goto out;
	}
	if (TS_SWITCH_TYPE_DOZE != (focal_dev_data->touch_switch_flag & TS_SWITCH_TYPE_DOZE)){
		TS_LOG_ERR("%s, doze mode does not suppored by this chip\n",__func__);
		goto out;
	}

	switch (soper){
		case TS_SWITCH_DOZE_ENABLE:
			TS_LOG_INFO("%s:enter doze_mode[param:%d]\n", __func__, param);
			error = focal_write_reg(FTS_REG_DOZE_EN, FTS_DOZE_ENABLE);
			if(error){
				TS_LOG_ERR("%s: Failed enable doze_mode: error%d\n", __func__, error);
				goto out;
			}
			error = focal_write_reg(FTS_REG_DOZE_HOLDOFF_TIME, param);
			if(error){
				TS_LOG_ERR("%s: Failed set holdoff time: error%d\n", __func__, error);
			}
			break;
		case TS_SWITCH_DOZE_DISABLE:
			TS_LOG_INFO("%s:exit doze_mode\n", __func__);
			error = focal_write_reg(FTS_REG_DOZE_EN, FTS_DOZE_DISABLE);
			if(error){
				TS_LOG_ERR("%s: Failed disable doze_mode : error%d\n", __func__, error);
			}
			break;
		default:
			TS_LOG_ERR("%s: soper unknown:%d, invalid\n", __func__, soper);
		break;
	}
out:
	return;
}

static void focal_set_game_mode(unsigned int soper)
{
	int error = 0;
	struct ts_kit_device_data *focal_dev_data = g_focal_dev_data;
	struct focal_platform_data *focal_pdata = g_focal_pdata;

	if ((NULL == focal_pdata) || (NULL == focal_dev_data)){
		TS_LOG_ERR("%s, error chip data\n",__func__);
		goto out;
	}
	if (TS_SWITCH_TYPE_GAME != (focal_dev_data->touch_switch_flag & TS_SWITCH_TYPE_GAME)){
		TS_LOG_ERR("%s, game mode does not suppored by this chip\n",__func__);
		goto out;
	}

	switch (soper){
		case TS_SWITCH_GAME_ENABLE:
			TS_LOG_INFO("%s: enter game_mode\n", __func__);
			error = focal_write_reg(focal_pdata->touch_switch_game_reg, 1);
			if(error){
				TS_LOG_ERR("%s: Switch to game mode Failed: error%d\n", __func__, error);
			}
			break;
		case TS_SWITCH_GAME_DISABLE:
			TS_LOG_INFO("%s: exit game_mode\n", __func__);
			error = focal_write_reg(focal_pdata->touch_switch_game_reg, 0);
			if (error) {
				TS_LOG_ERR("%s: Switch to normal mode Failed: error%d\n", __func__, error);
			}
			break;
		default:
			TS_LOG_ERR("%s: soper unknown:%d, invalid\n", __func__, soper);
			break;
	}
out:
	return;
}

static void focal_chip_touch_switch(void)
{
	char in_data[MAX_STR_LEN] = {0};
	unsigned int stype = 0, soper = 0, time = 0;
	int error = 0;
	unsigned int i = 0, cnt = 0;
	u8 param =0;
	struct ts_kit_device_data *focal_dev_data = g_focal_dev_data;

	TS_LOG_INFO("%s enter\n", __func__);

	if (NULL == focal_dev_data){
		TS_LOG_ERR("%s, error chip data\n",__func__);
		goto out;
	}

	/* SWITCH_OPER,ENABLE_DISABLE,PARAM */
	memcpy(in_data, focal_dev_data->touch_switch_info, MAX_STR_LEN -1);
	TS_LOG_INFO("%s, in_data:%s\n",__func__, in_data);
	for(i = 0; i < strlen(in_data) && (in_data[i] != '\n'); i++){
		if(in_data[i] == ','){
			cnt++;
		}else if(!isdigit(in_data[i])){
			TS_LOG_ERR("%s: input format error!!\n", __func__);
			goto out;
		}
	}
	if(cnt != FTS_DOZE_MAX_INPUT_SEPARATE_NUM){
		TS_LOG_ERR("%s: input format error[separation_cnt=%d]!!\n", __func__, cnt);
		goto out;
	}

	error = sscanf(in_data, "%u,%u,%u", &stype, &soper, &time);
	if(error <= 0){
		TS_LOG_ERR("%s: sscanf error\n", __func__);
		goto out;
	}
	TS_LOG_DEBUG("stype=%u,soper=%u,param=%u\n", stype, soper, time);

	/**
	 * enter DOZE again after a period of time. the min unit of the time is 100ms,
	 * for example, we set 30, the final time is 30 * 100 ms = 3s
	 * The min unit to enter doze_mode is second for focaltech
	**/
	param = (u8)time/10;

	switch (stype) {
		case TS_SWITCH_TYPE_DOZE:
			focal_set_doze_mode(soper, param);
			break;
		case TS_SWITCH_SCENE_3:
		case TS_SWITCH_SCENE_4:
			TS_LOG_INFO("%s : does not suppored\n", __func__);
			break;
		case TS_SWITCH_SCENE_5:
			focal_set_game_mode(soper);
			break;
		default:
			TS_LOG_ERR("%s: stype unknown:%u, invalid\n", __func__, stype);
			break;
	}

out:
	return;
}

static int focal_calibrate(void)
{
	return NO_ERR;
}

static int focal_calibrate_wakeup_gesture(void)
{
	return NO_ERR;
}

static int focal_regs_operate(struct ts_regs_info *info)
{
	int ret = NO_ERR;
	u8 buf[TS_MAX_REG_VALUE_NUM + 1] = { 0 };
	u8 *reg_value = g_focal_dev_data->ts_platform_data->chip_data->reg_values;

	if(info->num > TS_MAX_REG_VALUE_NUM){
		TS_LOG_ERR("%s invalid register num info->num = %d\n", __func__,info->num);
		return -EINVAL;
	}

	if (TS_ACTION_WRITE == info->op_action) {
		TS_LOG_INFO("ts reg write");
		if((info->addr < CHAR_MIN)||(info->addr > CHAR_MAX)) {
			TS_LOG_ERR("%s, invalid parameters info->addr = %d\n", __func__, info->addr);
			return -EINVAL;
		}
		buf[0] = (u8)info->addr;
		memcpy(&buf[1], info->values, info->num);
		ret = focal_write(buf, info->num + 1);
	} else {
		TS_LOG_INFO("ts reg read");
		ret = focal_read((u8 *)&info->addr, 1, reg_value, info->num);
		TS_LOG_INFO("ts reg read:%x", reg_value[0]);
	}

	return ret;
}

static int focal_param_init(struct focal_platform_data *focal_pdata)
{
	int ret = 0;
	u8 fw_ver = 0;
	u8 vendor_id = 0;

	/* init project id and fw_ver and chip id */
	ret = focal_read_project_id(focal_pdata,
		focal_pdata->project_id, FTS_PROJECT_ID_LEN - 1);
	if (ret) {
		TS_LOG_ERR("%s:read project id fail, ret=%d,hope update fw to recovery!\n", __func__, ret);
//		memset(focal_pdata->project_id,0,FTS_PROJECT_ID_LEN);
		return ret;
	}
	ret = focal_get_vendor_name_from_dts(focal_pdata->project_id,
		focal_pdata->vendor_name, FTS_VENDOR_NAME_LEN);
	if (ret) {
		TS_LOG_ERR("%s:read vendor name fail, ret=%d\n", __func__, ret);
		return ret;
	}
	strncpy(g_focal_dev_data->module_name,focal_pdata->vendor_name,
	(FTS_VENDOR_NAME_LEN < (MAX_STR_LEN-1))? FTS_VENDOR_NAME_LEN : (MAX_STR_LEN-1));

	ret = focal_read_vendor_id(focal_pdata, &vendor_id);
	if (ret) {
		TS_LOG_ERR("%s:read vendor id fail, ret=%d\n", __func__, ret);
		return ret;
	} else {
		focal_pdata->vendor_id = vendor_id;
	}

	ret = focal_get_ic_firmware_version(&fw_ver);
	if (ret) {
		TS_LOG_ERR("%s:read firmware version fail, ret=%d\n",
			__func__, ret);
		return ret;
	} else {
		focal_pdata->fw_ver = fw_ver;
		snprintf(g_focal_dev_data->version_name,MAX_STR_LEN-1,"%d",g_focal_pdata->fw_ver);
	}

	return 0;
}

static int focal_pinctrl_init(void)
{
	int error = 0;

	g_focal_pdata->pctrl= devm_pinctrl_get(&g_focal_dev_data->ts_platform_data->ts_dev->dev);
	if (IS_ERR(g_focal_pdata->pctrl)) {
		TS_LOG_ERR("failed to devm pinctrl get\n");
		return -EINVAL;
	}

#ifndef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	g_focal_pdata->pins_default =
	    pinctrl_lookup_state(g_focal_pdata->pctrl, "default");
	if (IS_ERR(g_focal_pdata->pins_default)) {
		TS_LOG_ERR("failed to pinctrl lookup state default\n");
		error = -EINVAL;
		goto err_pinctrl_put;
	}

	g_focal_pdata->pins_idle = pinctrl_lookup_state(g_focal_pdata->pctrl, "idle");
	if (IS_ERR(g_focal_pdata->pins_idle)) {
		TS_LOG_ERR("failed to pinctrl lookup state idle\n");
		error = -EINVAL;
		goto err_pinctrl_put;
	}
#else
     g_focal_pdata->pinctrl_state_reset_high= pinctrl_lookup_state(g_focal_pdata->pctrl, PINCTRL_STATE_RESET_HIGH);
    if (IS_ERR_OR_NULL(g_focal_pdata->pinctrl_state_reset_high))
    {
		TS_LOG_ERR("Can not lookup %s pinstate \n",PINCTRL_STATE_RESET_HIGH);
		error = -EINVAL;
		goto err_pinctrl_put;
    }
    g_focal_pdata->pinctrl_state_reset_low= pinctrl_lookup_state(g_focal_pdata->pctrl, PINCTRL_STATE_RESET_LOW);
    if (IS_ERR_OR_NULL(g_focal_pdata->pinctrl_state_reset_low))
    {
		TS_LOG_ERR("-Can not lookup %s pinstate \n",PINCTRL_STATE_RESET_LOW);
		error = -EINVAL;
		goto err_pinctrl_put;
    }
    g_focal_pdata->pinctrl_state_as_int= pinctrl_lookup_state(g_focal_pdata->pctrl, PINCTRL_STATE_AS_INT);
    if (IS_ERR_OR_NULL(g_focal_pdata->pinctrl_state_as_int))
    {
		TS_LOG_ERR("-Can not lookup %s pinstate \n",PINCTRL_STATE_AS_INT);
		error = -EINVAL;
		goto err_pinctrl_put;
    }
    g_focal_pdata->pinctrl_state_int_high= pinctrl_lookup_state(g_focal_pdata->pctrl,PINCTRL_STATE_INT_HIGH);
    if (IS_ERR_OR_NULL(g_focal_pdata->pinctrl_state_int_high))
    {
		TS_LOG_ERR("-Can not lookup %s pinstate \n",PINCTRL_STATE_INT_HIGH);
		error = -EINVAL;
		goto err_pinctrl_put;
    }
    g_focal_pdata->pinctrl_state_int_low= pinctrl_lookup_state(g_focal_pdata->pctrl,PINCTRL_STATE_INT_LOW);
    if (IS_ERR_OR_NULL(g_focal_pdata->pinctrl_state_int_low))
    {
		TS_LOG_ERR("-Can not lookup %s pinstate \n", PINCTRL_STATE_INT_LOW);
		error = -EINVAL;
		goto err_pinctrl_put;
    }

	error = pinctrl_select_state(g_focal_pdata->pctrl, g_focal_pdata->pinctrl_state_as_int);
	if (error < 0){
		TS_LOG_ERR("set gpio as int faild \n");
		error = -EINVAL;
		goto err_pinctrl_put;
	}
#endif
	return 0;
err_pinctrl_put:
	devm_pinctrl_put(g_focal_pdata->pctrl);
	return error;
}

static void focal_pinctrl_release(void)
{
	TS_LOG_INFO("%s:called\n", __func__);
	if (g_focal_pdata->pctrl) {
		devm_pinctrl_put(g_focal_pdata->pctrl);
	}
	g_focal_pdata->pctrl = NULL;
	g_focal_pdata->pins_default = NULL;
	g_focal_pdata->pins_idle = NULL;
#if defined (CONFIG_HUAWEI_DEVKIT_MTK_3_0)
	g_focal_pdata->pinctrl_state_reset_high = NULL;
	g_focal_pdata->pinctrl_state_int_high = NULL;
	g_focal_pdata->pinctrl_state_int_low = NULL;
	g_focal_pdata->pinctrl_state_reset_low = NULL;
	g_focal_pdata->pinctrl_state_as_int = NULL;
#endif
	TS_LOG_INFO("%s:called end\n", __func__);
}

/**
 * focal_pinctrl_select_normal - set normal pin state
 */
static void focal_pinctrl_select_normal(void)
{
#ifndef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	int ret = 0;
	struct focal_platform_data *pdata = g_focal_pdata;

	if (!pdata->fts_use_pinctrl) {
		TS_LOG_INFO("%s, pinctrl is not in use.\n", __func__);
		return;
	}

	if (pdata->pctrl && pdata->pins_default) {
		ret = pinctrl_select_state(pdata->pctrl , pdata->pins_default);
		if (ret < 0)
			TS_LOG_ERR("%s:Set normal pin state error:%d\n", __func__, ret);
	}
#endif
	return;
}

/**
 * focal_pinctrl_select_suspend - set suspend pin state
 */
static void focal_pinctrl_select_suspend(void)
{
#ifndef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	int ret = 0;
	struct focal_platform_data *pdata = g_focal_pdata;

	if (!pdata->fts_use_pinctrl) {
		TS_LOG_INFO("%s, pinctrl is not in use.\n");
		return;
	}

	if (pdata->pctrl && pdata->pins_idle){
		ret = pinctrl_select_state(pdata->pctrl, pdata->pins_idle);
		if (ret < 0)
			TS_LOG_ERR("%s:Set suspend pin state error:%d\n", __func__, ret);
	}
#endif
	return;
}

static int focal_get_fwname(struct focal_platform_data *focal_pdata){
	char joint_chr = '_';
	char *fwname = focal_pdata->fw_name;
	struct ts_kit_platform_data *ts_platform_data;
	ts_platform_data = g_focal_dev_data->ts_platform_data;

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

static int focal_init_chip(void)
{
	int ret = NO_ERR;

	struct focal_platform_data *focal_pdata = g_focal_pdata;
	struct focal_delay_time *delay_time = NULL;
	struct ts_kit_device_data *focal_dev_data = NULL;
	struct ts_kit_platform_data *ts_platform_data = NULL;

	g_focal_pdata->focal_device_data = g_focal_dev_data;
	focal_dev_data = g_focal_dev_data;
	ts_platform_data = focal_dev_data->ts_platform_data;

#if defined (CONFIG_TEE_TUI)
	if (TS_BUS_I2C == ts_platform_data->bops->btype) {
		strncpy(tee_tui_data.device_name, "fts", strlen("fts"));
		tee_tui_data.device_name[strlen("fts")] = '\0';
	} else {
		strncpy(tee_tui_data.device_name, "focal_spi", strlen("focal_spi"));
		tee_tui_data.device_name[strlen("focal_spi")] = '\0';
	}
#endif

	delay_time = kzalloc(sizeof(struct focal_delay_time), GFP_KERNEL);
	if (!delay_time) {
		TS_LOG_ERR("%s:allocate memory for delay_time fail\n",
			__func__);
		ret = -ENOMEM;
		goto free_focal_pdata;
	}

	memset((u8 *)&fts_esdcheck_data, 0, sizeof(struct fts_esdcheck_st));

	focal_pdata->delay_time = delay_time;

	ret = focal_parse_dts(focal_dev_data->cnode, focal_pdata);
	if (ret) {
		TS_LOG_ERR("%s:parse dts fail, ret=%d\n", __func__, ret);
		goto free_delay_time;
	}

	focal_pdata->focal_platform_dev = ts_platform_data->ts_dev;
	//focal_dev_data->is_in_cell = true;
  	focal_pdata->fw_is_running = false;
	mutex_init(&wrong_touch_lock);

	ret = focal_apk_node_init();
	if (ret < 0) {
		TS_LOG_ERR("%s:apk_node_init error, ret=%d\n", __func__, ret);
		goto free_delay_time;
	}

	ret = focal_param_init(focal_pdata);
	if (ret) {
		TS_LOG_ERR("%s:init param fail, ret=%d\n", __func__, ret);
		goto apk_node_exit;
	}
	/*provide panel_id for sensor*/
	g_focal_dev_data->ts_platform_data->panel_id =
		(focal_pdata->project_id[FTS_PANEL_ID_START_BIT] - '0') * 10 +
		focal_pdata->project_id[FTS_PANEL_ID_START_BIT + 1] - '0';
	TS_LOG_INFO("%s: panel_id=%d\n", __func__, g_focal_dev_data->ts_platform_data->panel_id);

	if (TS_BUS_SPI == g_focal_pdata->focal_device_data->ts_platform_data->bops->btype) {
		ret = focal_get_fwname(focal_pdata);
		if(ret) {
			TS_LOG_ERR("%s:focal get fwname fail\n",__func__);
			goto free_focal_pdata;
		}
	}

	TS_LOG_INFO("%s:init chip success.\n", __func__);
	return NO_ERR;

apk_node_exit:
	//focal_apk_node_exit();

free_delay_time:
	//kfree(focal_pdata->delay_time);
free_focal_pdata:
	//don't free focal_pdata,firmware upgrade will use this parameter
	TS_LOG_ERR("%s:focal init chip error.\n", __func__);

	return ret;
}

static int focal_power_parameter_config(void)
{
	TS_LOG_INFO("%s called\n" , __func__);
	return ts_kit_power_supply_get(TS_KIT_VCC);
}
static int focal_power_release(void)
{
	TS_LOG_INFO("%s called\n" , __func__);
	ts_kit_power_supply_put(TS_KIT_VCC);
	return 0;
}

static int focal_power_on(void)
{

	return ts_kit_power_supply_ctrl(TS_KIT_VCC, TS_KIT_POWER_ON, 5);
}
static void focal_power_off(void)
{
	ts_kit_power_supply_ctrl(TS_KIT_VCC, TS_KIT_POWER_OFF, 5);
}

static int focal_power_control(void)
{
	int ret = 0;

	if (FTS_SELF_CTRL_POWER == g_focal_pdata->self_ctrl_power) {
		TS_LOG_INFO("%s, power control by touch ic\n",__func__);
		ret = focal_power_parameter_config();
		if (ret) {
			TS_LOG_ERR("%s: power config failed\n", __func__);
			goto exit;
		}

		/*power on*/
		ret = focal_power_on();
		if(ret){
			TS_LOG_ERR("%s: power on failed\n", __func__);
			goto exit_regulator_put;
		}
	} else {
		TS_LOG_INFO("%s, power control by LCD, nothing to do\n",__func__);
	}

	return 0;

exit_regulator_put:
	focal_power_release();
exit:
	return ret;
}

static int focal_chip_detect(struct ts_kit_platform_data *pdata)
{
	int ret = NO_ERR;

	g_focal_dev_data->ts_platform_data = pdata;

	if (!pdata){
		TS_LOG_ERR("%s device, ts_kit_platform_data *data is NULL \n", __func__);
		ret = -ENOMEM;
		goto exit;
	}else if(!pdata->ts_dev){
		TS_LOG_ERR("%s device, ts_kit_platform_data data->ts_dev is NULL \n", __func__);
		ret = -ENOMEM;
		goto exit;
	}
	g_focal_dev_data->is_i2c_one_byte = 0;
	g_focal_dev_data->is_new_oem_structure= 0;
	g_focal_dev_data->is_parade_solution= 0;
	g_focal_dev_data->ts_platform_data->ts_dev = pdata->ts_dev;
	g_focal_dev_data->ts_platform_data->ts_dev->dev.of_node = g_focal_dev_data->cnode;
	g_focal_pdata->open_threshold_status = true;

	ret = focal_prase_ic_config_dts(g_focal_dev_data->cnode, g_focal_dev_data);
	if (ret < 0) {
		TS_LOG_ERR("%s:parse ic config dts fail, ret=%d\n",
			__func__, ret);
		goto exit;
	}

	ret = focal_pinctrl_init();
	if (ret < 0) {
		TS_LOG_ERR("%s: focal_pinctrl_init error\n", __func__);
		goto exit_power_off;
	}

	if(FOCAL_FT5X46 == g_focal_dev_data->ic_type){
		ret = focal_reset_output(0);
		if(ret){
			TS_LOG_ERR("%s:gpio direction output to fail, ret=%d\n",__func__, ret);
		}
		udelay(FT5X46_RESET_KEEP_LOW_TIME_BEFORE_POWERON);
	}

	ret = focal_power_control();
	if (ret < 0) {
		TS_LOG_ERR("%s: fts power control failed\n", __func__);
		goto exit;
	}

	if(FOCAL_FT5X46 == g_focal_dev_data->ic_type){
		ret = focal_ft5x46_chip_reset();
		if (ret) {
			TS_LOG_ERR("%s:reset pull up failed, ret=%d\n", __func__, ret);
			goto pinctrl_get_err;
		}
	} else {
		if(FOCAL_FT8201 != g_focal_dev_data->ic_type){
			ret = focal_hardware_reset(FTS_MODEL_FIRST_START);
			if (ret) {
				TS_LOG_ERR("%s:hardware reset fail, ret=%d\n", __func__, ret);
				goto pinctrl_get_err;
			}
		}
	}

	ret = communicate_check(pdata);
	if (ret < 0) {
		TS_LOG_ERR("%s:not find focal device, ret=%d\n", __func__, ret);
		goto pinctrl_get_err;
	} else {
		TS_LOG_INFO("%s:find focal device\n", __func__);

		strncpy(g_focal_dev_data->chip_name, FTS_CHIP_NAME, MAX_STR_LEN);
	}

	TS_LOG_INFO("%s:focal chip detect success\n", __func__);

	return 0;

pinctrl_get_err:
	focal_pinctrl_release();
exit_power_off:
	focal_power_off();
	focal_pinctrl_release();
exit:
	if(g_focal_dev_data) {
		kfree(g_focal_dev_data);
		g_focal_dev_data = NULL;
	}

	if (g_focal_pdata) {
		kfree(g_focal_pdata);
		g_focal_pdata = NULL;
	}

	TS_LOG_INFO("%s:focal chip detect fail\n", __func__);
	return ret;
}

static int __init focal_core_module_init(void)
{
	int ret = NO_ERR;
	bool found = false;
	struct device_node *child = NULL;
	struct device_node *root = NULL;

	TS_LOG_INFO("%s: called\n", __func__);

	g_focal_pdata = kzalloc(sizeof(struct focal_platform_data), GFP_KERNEL);
	if (NULL == g_focal_pdata) {
		TS_LOG_ERR("%s:alloc mem for device data fail\n", __func__);
		ret = -ENOMEM;
		goto error_exit;
	}

	g_focal_dev_data =
		kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
	if (NULL == g_focal_dev_data) {
		TS_LOG_ERR("%s:alloc mem for device data fail\n", __func__);
		ret = -ENOMEM;
		goto error_exit;
	}

	root = of_find_compatible_node(NULL, NULL, HUAWEI_TS_KIT);
	if (!root) {
		TS_LOG_ERR("%s:find_compatible_node error\n", __func__);
		ret = -EINVAL;
		goto error_exit;
	}

	for_each_child_of_node(root, child) {
		if (of_device_is_compatible(child, FTS_CHIP_NAME)) {
			found = true;
			break;
		}
	}

	if (!found) {
		TS_LOG_ERR("%s:device tree node not found, name=%s\n",
			__func__, FTS_CHIP_NAME);
		ret = -EINVAL;
		goto error_exit;
	}

	g_focal_dev_data->cnode = child;
	g_focal_dev_data->ops = &ts_focal_ops;
	ret = huawei_ts_chip_register(g_focal_dev_data);
	if (ret) {
		TS_LOG_ERR("%s:chip register fail, ret=%d\n", __func__, ret);
		goto error_exit;
	}

	TS_LOG_INFO("%s:success\n", __func__);
	return 0;

error_exit:

	if(g_focal_dev_data) {
		kfree(g_focal_dev_data);
		g_focal_dev_data = NULL;
	}

	if (g_focal_pdata) {
		kfree(g_focal_pdata);
		g_focal_pdata = NULL;
	}

	TS_LOG_INFO("%s:fail\n", __func__);
	return ret;
}

static void __exit focal_ts_module_exit(void)
{
	kfree(g_focal_pdata);
	g_focal_pdata = NULL;

	kfree(g_focal_dev_data);
	g_focal_dev_data = NULL;

	focal_param_kree();

	return;
}

late_initcall(focal_core_module_init);
module_exit(focal_ts_module_exit);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");
