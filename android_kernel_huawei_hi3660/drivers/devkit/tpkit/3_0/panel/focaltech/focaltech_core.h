

#ifndef __FOCALTECH_CORE_H__
#define __FOCALTECH_CORE_H__

#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/ctype.h>
#include "../../huawei_ts_kit.h"

extern struct focal_platform_data *g_focal_pdata ;
extern  struct ts_kit_device_data *g_focal_dev_data ;

#define LCD_PANEL_INFO_MAX_LEN  128
#define LCD_PANEL_TYPE_DEVICE_NODE_NAME     "huawei,lcd_panel_type"

#define FTS_MAX_TOUCH_POINTS		10
#define FTS_MODEL_NORMAL		0x00
#define FTS_MODEL_BOOTLOADER		0x01
#define FTS_MODEL_FIRST_START		0x02
#define FTS_PROJECT_ID_LEN		11
#define FTS_PANEL_ID_START_BIT		6
#define FTS_VENDOR_NAME_LEN		12
#define FTS_TP_COLOR_LEN		3
#define TP_COLOR_SIZE   		15
#define FTS_REPORT_BTN_TOUCH
#define FTS_ROI_PACKAGE_NUM     0x9C
#define FTS_ROI_BUFF0_ADDR     0x7E
#define FTS_DETECT_I2C_RETRY_TIMES			1
#define FTS_DETECT_SPI_RETRY_TIMES			3
#define FTS_COMMON_COMMAND_LENGTH			1
#define FTS_COMMON_COMMAND_VALUE			1
#define FTS_FW_MANUAL_UPDATE_FILE_NAME	"ts/touch_screen_firmware.img"

#define FTS_VDDIO_GPIO_NAME		"ts_vddio_gpio"
#define FTS_VCI_GPIO_NAME		"ts_vci_gpio"

#if defined (CONFIG_HUAWEI_DEVKIT_MTK_3_0)
#define PINCTRL_STATE_RESET_HIGH	"state_rst_output1"
#define PINCTRL_STATE_RESET_LOW	"state_rst_output0"
#define PINCTRL_STATE_INT_HIGH	"state_eint_output1"
#define PINCTRL_STATE_INT_LOW	"state_eint_output0"
#define PINCTRL_STATE_AS_INT	"state_eint_as_int"
#endif

#define FTS_REG_WORKMODE		0x00
#define FTS_REG_ESD_SATURATE		0xED
#define FTS_REG_FLOW_WORK_CNT		0x91
#define FTS_REG_WORKMODE_FACTORY_VALUE		0x40
#define FTS_REG_WORKMODE_WORK_VALUE		0x00
#define FTS_REG_WORKMODE_FACTORY_OFFSET	0x70
#define FTS_REG_SPECIAL_VALUEl		 0xEF
#define FTS_REG_DOZE_EN				0x86
#define FTS_REG_DOZE_HOLDOFF_TIME	0x87
#define FTS_DOZE_ENABLE				1
#define FTS_DOZE_DISABLE			0
#define FTS_FT86XX_HIGH                              0x86
#define FTS_PALM_IRON				0x55

#define GESTURE_DOUBLECLICK                     0x24
#define FTS_REG_GESTURE_EN      			(0xD0)
#define FTS_REG_GESTURE_OUTPUT_ADDRESS_D1      (0xD1)
#define FTS_REG_GESTURE_OUTPUT_ADDRESS_D2      (0xD2)
#define FTS_REG_GESTURE_OUTPUT_ADDRESS_D3      (0xD3)
#define FTS_REG_GESTURE_OUTPUT_ADDRESS_D5      (0xD5)
#define FTS_REG_GESTURE_OUTPUT_ADDRESS_D6      (0xD6)
#define FTS_REG_GESTURE_OUTPUT_ADDRESS_D7      (0xD7)
#define FTS_REG_GESTURE_OUTPUT_ADDRESS_D8      (0xD8)
#define FTS_REG_GESTURE_OPEN_FF      (0xFF)
#define FTS_REG_GESTURE_READ_OFFSET      (255)
#define FTS_GESTRUE_POINTS_HEADER           (8)
#define FTS_GESTRUE_POINTS                  (255)

#define FTS_PACKAGE_SIZE		128
#define FTS_PACKAGE_SIZE_SPI	((4*1024) - 4)

#define FTS_READ_REG_LEN                  (1)
#define FTS_GESTURE_EANBLE                  (1)
#define FTS_SLEEP_TIME_100                  (100)
#define FTS_SLEEP_TIME_220                  (220)
#define FTS_SLEEP_TIME_265                  (265)
#define FTS_SELF_CTRL_POWER			1
#define FTS_VCI_LDO_POWER			1
#define FTS_VDDIO_LDO_POWER			1
#define FTS_POWER_DOWN				1
#define RESET_ENABLE_DELAY			240
#define FT5X46_RESET_KEEP_LOW_TIME		1500
#define FT5X46_RESET_KEEP_LOW_TIME_BEFORE_POWERON		500
#define FTS_EDGE_TOUCH_DATA_LEN			50
#define FTS_FP_EDGE_TOUCH_DATA_LEN		70
#define ABS_MT_TOUCH_MINOR_MAX			255
#define ABS_MT_TOUCH_MAJOR_MAX			255
#define MAX_COMMAND_LENGTH			16
#define CHAR_MIN			-128
#define CHAR_MAX			127
#define FTS_FW_STATE_ERROR		0xEF
#define FTS_FW_DOWNLOAD_MODE         0x02
#define FOCAL_RESET_DELAY_TIME			10
#define FOCAL_AFTER_WRITE_55_DELAY_TIME 	8

enum focal_ic_type {
	FOCAL_FT8716 = 0,
	FOCAL_FT8607,
	FOCAL_FT5X46,
	FOCAL_FT8719,
	FOCAL_FT8201,
	FOCAL_FT8006U,
};

enum roi_data_status {
	ROI_DATA_NOT_NEET_READ = 0,
	ROI_DATA_READY,
	ROI_DATA_NOT_READY,
};

#if defined (CONFIG_HUAWEI_DSM)
enum FW_uptate_state
{
    focal_enter_pram_model_fail = 0,
    focal_check_firmware_size_in_pram_model_fail,
    focal_auto_config_clock_fail,
    focal_enter_pram_update_model_from_pram_fail,
    focal_erasure_app_area_fail,
    focal_write_app_data_fail,
    focal_read_check_sum_fail,
    TS_UPDATE_STATE_UNDEFINE = 255,
};
#endif
struct ts_event {
	u16 position_x[FTS_MAX_TOUCH_POINTS];
	u16 position_y[FTS_MAX_TOUCH_POINTS];

	u16 pressure[FTS_MAX_TOUCH_POINTS];
	u16 area[FTS_MAX_TOUCH_POINTS];
	u16 ewx[FTS_MAX_TOUCH_POINTS];
	u16 ewy[FTS_MAX_TOUCH_POINTS];
	u16 xer[FTS_MAX_TOUCH_POINTS];
	u16 yer[FTS_MAX_TOUCH_POINTS];
	u16 wx[FTS_MAX_TOUCH_POINTS];
	u16 wy[FTS_MAX_TOUCH_POINTS];

	/* touch event: 0 -- down; 1-- up; 2 -- contact */
	u8 touch_event[FTS_MAX_TOUCH_POINTS];
	/* touch ID */
	u8 finger_id[FTS_MAX_TOUCH_POINTS];
	u8 touch_point;
	u8 touch_point_num;
	u8 palm_iron_num;
	int touchs;
};

struct fts_esdcheck_st
{
    u8      active              : 1;    /* 1- esd check active, need check esd 0- no esd check */
    u8      suspend             : 1;
    u8      boot_upgrade          : 1;    /* boot upgrade */
    u8      intr                : 1;    /* 1- Interrupt trigger */
    u8      unused              : 4;
    u8      flow_work_hold_cnt;         /* Flow Work Cnt(reg0x91) keep a same value for x times. >=5 times is ESD, need reset */
    u8      flow_work_cnt_last;         /* Save Flow Work Cnt(reg0x91) value */
    u32     hardware_reset_cnt;
    u32     i2c_nack_cnt;
    u32     i2c_dataerror_cnt;
};

/* platform data that can be configured by extern */
struct focal_platform_data {
	struct platform_device *focal_platform_dev;
	struct focal_delay_time *delay_time;
	struct ts_kit_device_data *focal_device_data;
	unsigned int chip_id;
	struct pinctrl *pctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_idle;
#if defined (CONFIG_HUAWEI_DEVKIT_MTK_3_0)
	struct pinctrl_state *pinctrl_state_reset_high;
	struct pinctrl_state *pinctrl_state_reset_low;
	struct pinctrl_state *pinctrl_state_release;
	struct pinctrl_state *pinctrl_state_int_high;
	struct pinctrl_state *pinctrl_state_int_low;
	struct pinctrl_state *pinctrl_state_as_int;
#endif
	int fw_ver;
	int vendor_id;
	unsigned int pram_projectid_addr;
	char project_id[FTS_PROJECT_ID_LEN];
	char vendor_name[FTS_VENDOR_NAME_LEN + 1];
	struct ts_event touch_data;
	bool support_get_tp_color;/*for tp color */
	struct regulator *vddd;
	struct regulator *vdda;
	int self_ctrl_power;
	int self_ctrl_reset;
	int power_down_ctrl;
	bool open_threshold_status;
	int only_open_once_captest_threshold;
	int projectid_length_control_flag;
	u32 enable_edge_touch;
	u32 edge_data_addr;
	u32 aft_wxy_enable;
	u32 roi_pkg_num_addr;
	int need_distinguish_lcd;
	int hide_plain_lcd_log;
	int fw_only_depend_on_lcd;//0 : fw depend on TP and others ,1 : fw only depend on lcd.
	char lcd_panel_info[LCD_PANEL_INFO_MAX_LEN];
	char lcd_module_name[LCD_PANEL_INFO_MAX_LEN];
	char lcd_hide_module_name[MAX_STR_LEN];
	int fw_is_running;/* confirm fw is running,default 0 */
	char fw_name[MAX_STR_LEN];
	u8 lcd_noise_threshold;
	u8 touch_switch_game_reg;
	u8 touch_switch_scene_reg;
	u8 read_debug_reg_and_differ;
	u32 fts_use_pinctrl;
	struct mutex spilock;
	unsigned int fw_update_duration_check;
};

/* spi interface communication*/
int fts_read(u8 *writebuf, u32 writelen, u8 *readbuf, u32 readlen);
int fts_write(u8 *writebuf, u32 writelen);
int focal_read(u8 *addr, u16 addr_len, u8 *value, u16 values_size);
int focal_read_reg(u8 addr, u8 *val);
int focal_read_default(u8 *values, u16 values_size);
int focal_write(u8 *value, u16 values_size);
int focal_write_reg(u8 addr, u8 val);
int focal_write_default(u8 value);

int focal_hardware_reset_to_normal_model(void);
int focal_hardware_reset_to_bootloader(void);
int focal_hardware_reset_to_rom_update_model(void);

struct ts_kit_device_data *focal_get_device_data(void);
struct focal_platform_data *focal_get_platform_data(void);

char *focal_strncat(char *dest, char *src, size_t dest_size);
char *focal_strncatint(char *dest, int src, char *format, size_t dest_size);
int focal_strtolow(char *src_str, size_t size);
int focal_esdcheck_set_upgrade_flag(u8 boot_upgrade);
extern int focal_8201_get_raw_data(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd);
extern int focal_get_raw_data(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd);

#endif

