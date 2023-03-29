/*
 * Huawei Touchpanel driver
 *
 * Copyright (C) 2013 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#ifndef __HUAWEI_TS_KIT_PLATFORM_H_
#define __HUAWEI_TS_KIT_PLATFORM_H_

#include <linux/kernel.h>

/* marco define*/
#ifndef strict_strtoul
#define strict_strtoul kstrtoul
#endif

#ifndef strict_strtol
#define strict_strtol kstrtol
#endif

#define RAWDATA_SIZE_LIMIT 2

#define TS_DEV_NAME "huawei,ts_kit"
#define TS_PEN_DEV_NAME "huawei,ts_pen"

#define RAW_DATA_SIZE 8192 * 4
#define TS_WATCHDOG_TIMEOUT		1000

#define AFT_WAITQ_IDLE	 (0)
#define AFT_WAITQ_WAIT	 (1)
#define AFT_WAITQ_WAKEUP (2)
#define AFT_WAITQ_IGNORE (3)

/*diff data report status*/
#define DIFF_DATA_IDLE	                (0)
#define DIFF_DATA_WAIT	                (1)
#define DIFF_DATA_WAKEUP                (2)
#define DIFF_DATA_IGNORE                (3)


//#define ANTI_FALSE_TOUCH_STRING_NUM 27
#define ANTI_FALSE_TOUCH_FEATURE_ALL "feature_all"
//for driver
#define ANTI_FALSE_TOUCH_LCD_WIDTH "lcd_width"
#define ANTI_FALSE_TOUCH_LCD_HEIGHT "lcd_height"
#define ANTI_FALSE_TOUCH_DRV_STOP_WIDTH "drv_stop_width"


#define WACOM_TOOL_TYPE_NONE     0

#define WACOM_PEN_TO_RUBBER     249	//after code 248-Mute
#define WACOM_RUBBER_TO_PEN   250

#define TS_FB_LOOP_COUNTS 100
#define TS_FB_WAIT_TIME 5

#define TS_CHIP_DMD_REPORT_SIZE  1024

#define TS_NV_BAR_CODE_LEN 3
#define TS_NV_BRIGHTNESS_LEN 1
#define TS_NV_WHITE_POINT_LEN 1
#define TS_NV_BRI_WHITE_LEN 1
#define TS_NV_REPAIR_LEN 1

#define I2C_DEFAULT_ADDR 0x70
#define TS_SUSPEND_LEVEL 1

#define I2C_WAIT_TIME 25	//25ms wait period

#define TS_NO_KEY_PRESS  (0)
#define TS_IO_UNDEFINE  (-1)
#define TS_IRQ_CFG_UNDEFINE  (-1)

#define TS_PEN_BUTTON_NONE      0
#define TS_PEN_BUTTON_RELEASE   (1 << 5)
#define TS_PEN_BUTTON_PRESS     (1 << 6)
#define TS_PEN_KEY_NONE         0

#define TS_ALGO_FUNC_0		(1<<0)
#define TS_ALGO_FUNC_1		(1<<1)
#define TS_ALGO_FUNC_2		(1<<2)
#define TS_ALGO_FUNC_3		(1<<3)
#define TS_ALGO_FUNC_4		(1<<4)
#define TS_ALGO_FUNC_5		(1<<5)
#define TS_ALGO_FUNC_6		(1<<6)
#define TS_ALGO_FUNC_7		(1<<7)
#define TS_ALGO_FUNC_ALL	(0xFF)

#define TS_GESTURE_COMMAND 0x7ff
#define TS_GESTURE_INVALID_COMMAND 0xFFFF
#define TS_GESTURE_PALM_BIT 0x0800
#define TS_GET_CALCULATE_NUM 2048
#define TS_GESTURE_INVALID_CONTROL_NO 0xFF

#define PEN_MOV_LENGTH      120	//move length (pixels)
#define FINGER_REL_TIME     300	//the time pen checked after finger released shouldn't less than this value(ms)

#define RAWDATA_TESTTMP_NUM 10
#define TEST_CAPACITANCE_VIA_CSVFILE "huawei,test_capacitance_via_csvfile"

enum ts_irq_config {
	TS_IRQ_LOW_LEVEL,
	TS_IRQ_HIGH_LEVEL,
	TS_IRQ_RAISE_EDGE,
	TS_IRQ_FALL_EDGE,
};


void ts_check_touch_window(struct ts_fingers *finger);
int ts_kit_proc_command_directly(struct ts_cmd_node *cmd);

#if defined (CONFIG_TEE_TUI)
void ts_kit_tui_secos_init(void);
void ts_kit_tui_secos_exit(void);
#endif
void ts_film_touchplus(struct ts_fingers *finger, int finger_num,
		       struct input_dev *input_dev);

int ts_count_fingers(struct ts_fingers *fingers);
#endif