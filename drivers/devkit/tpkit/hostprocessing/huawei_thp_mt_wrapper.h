/*
 * Huawei Touchscreen Driver
 *
 * Copyright (C) 2017 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */

#ifndef _INPUT_MT_WRAPPER_H_
#define _INPUT_MT_WRAPPER_H_

#include "huawei_thp.h"

#define INPUT_MT_WRAPPER_MAX_FINGERS (10)
#define THP_MT_WRAPPER_MAX_FINGERS (10)
#define THP_MT_WRAPPER_MAX_X (1079)
#define THP_MT_WRAPPER_MAX_Y (1919)
#define THP_MT_WRAPPER_MAX_Z (100)
#define THP_MT_WRAPPER_MAX_MAJOR (1)
#define THP_MT_WRAPPER_MAX_MINOR (1)
#define THP_MT_WRAPPER_MIN_ORIENTATION (-90)
#define THP_MT_WRAPPER_MAX_ORIENTATION (90)
#define THP_MT_WRAPPER_TOOL_TYPE_MAX (1)
#define THP_MT_WRAPPER_TOOL_TYPE_STYLUS (1)

#define THP_INPUT_DEV_COMPATIBLE "huawei,thp_input"

enum input_mt_wrapper_state {
	INPUT_MT_WRAPPER_STATE_DEFAULT,
	INPUT_MT_WRAPPER_STATE_FIRST_TOUCH = 1,
	INPUT_MT_WRAPPER_STATE_LAST_TOUCH = 2,
	INPUT_MT_WRAPPER_STATE_SAME_REPORT,
	INPUT_MT_WRAPPER_STATE_SAME_ZERO_REPORT,
};

#define THP_INPUT_DEVICE_NAME	"input_mt_wrapper"

struct thp_input_dev_config {
	int abs_max_x;
	int abs_max_y;
	int abs_max_z;
	int tracking_id_max;
	int major_max;
	int minor_max;
	int orientation_min;
	int orientation_max;
	int tool_type_max;
};

struct thp_mt_wrapper_data {
	struct input_dev *input_dev;
	struct input_dev *key_input_dev;
	struct thp_input_dev_config input_dev_config;
	wait_queue_head_t wait;
	atomic_t status_updated;
};

struct input_mt_wrapper_touch_data {
	unsigned char down;
	unsigned char valid; /* 0:invalid !=0:valid */
	int x;
	int y;
	int pressure;
	int tracking_id;
	int hand_side;
	int major;
	int minor;
	int orientation;
	unsigned int tool_type;
};

struct thp_mt_wrapper_ioctl_touch_data {
	struct input_mt_wrapper_touch_data touch[INPUT_MT_WRAPPER_MAX_FINGERS];
	enum input_mt_wrapper_state state;
	int t_num;
	int down_num;
};

#define KEY_F26 196

enum input_mt_wrapper_keyevent {
	INPUT_MT_WRAPPER_KEYEVENT_NULL    = 0,
	INPUT_MT_WRAPPER_KEYEVENT_ESD    = 1 << 0,
	INPUT_MT_WRAPPER_KEYEVENT_APPROACH = 1 << 1,
	INPUT_MT_WRAPPER_KEYEVENT_AWAY = 1 << 2,
};

#define PROX_VALUE_LEN          3
#define PROX_EVENT_LEN          12
#define APPROCH_EVENT_VALUE     0
#define AWAY_EVENT_VALUE        1

/* commands */
#define INPUT_MT_WRAPPER_IO_TYPE  (0xB9)
#define INPUT_MT_WRAPPER_IOCTL_CMD_SET_COORDINATES \
	_IOWR(INPUT_MT_WRAPPER_IO_TYPE, 0x01, \
		struct thp_mt_wrapper_ioctl_touch_data)
#define INPUT_MT_WRAPPER_IOCTL_READ_STATUS \
	_IOR(INPUT_MT_WRAPPER_IO_TYPE, 0x02, u32)
#define INPUT_MT_WRAPPER_IOCTL_READ_INPUT_CONFIG \
	_IOR(INPUT_MT_WRAPPER_IO_TYPE,  0x03, struct thp_input_dev_config)
#define INPUT_MT_WRAPPER_IOCTL_READ_SCENE_INFO     \
	_IOR(INPUT_MT_WRAPPER_IO_TYPE,  0x04, struct thp_scene_info)
#define INPUT_MT_WRAPPER_IOCTL_CMD_SET_EVENTS \
	_IOR(INPUT_MT_WRAPPER_IO_TYPE, 0x05, uint32_t)
#define INPUT_MT_WRAPPER_IOCTL_CMD_GET_EVENTS \
	_IOR(INPUT_MT_WRAPPER_IO_TYPE, 0x06, uint32_t)
#define INPUT_MT_WRAPPER_IOCTL_CMD_REPORT_KEYEVENT \
	_IOR(INPUT_MT_WRAPPER_IO_TYPE,	0x07, u32)
#define INPUT_MT_WRAPPER_IOCTL_GET_PROJECT_ID \
	_IOR(INPUT_MT_WRAPPER_IO_TYPE, 0x08, uint32_t)
#define INPUT_MT_WRAPPER_IOCTL_SET_ROI_DATA \
	_IOW(INPUT_MT_WRAPPER_IO_TYPE, 0x09, uint32_t)
#define INPUT_MT_WRAPPER_IOCTL_GET_WINDOW_INFO \
	_IOR(INPUT_MT_WRAPPER_IO_TYPE, 0x0a, struct thp_window_info)

int thp_mt_wrapper_init(void);
void thp_mt_wrapper_exit(void);
int thp_mt_wrapper_wakeup_poll(void);
void thp_clean_fingers(void);

#endif /* _INPUT_MT_WRAPPER_H_ */
