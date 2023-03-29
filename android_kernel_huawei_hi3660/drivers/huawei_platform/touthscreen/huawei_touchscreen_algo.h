#ifndef __HUAWEI_TOUCHSCREEN_ALGO_H_
#define __HUAWEI_TOUCHSCREEN_ALGO_H_
#include "huawei_touchscreen_chips.h"

#define TP_FINGER				1
#define TP_STYLUS				2
#define TP_GLOVE				6
#define FILTER_GLOVE_NUMBER	4

#define PEN_MOV_LENGTH      120	/*move length (pixels)*/
#define FINGER_REL_TIME     300	/*the time pen checked after finger released shouldn't less than this value(ms)*/

enum TP_state_machine {
	INIT_STATE = 0,
	ZERO_STATE = 1,		/*switch finger to glove*/
	FINGER_STATE = 2,	/*finger state*/
	GLOVE_STATE = 3		/*glove state*/
};

#define AFT_EDGE 1
#define NOT_AFT_EDGE 0

//static char first_touch_tag = 0;
#define FINGER_PRESS_TIME_MIN	20 * 1000 /* mil second */
#define FINGER_PRESS_TIMES_IN_MIN_TIME	5
#define GHOST_MIL_SECOND_TIME 1000000
#define GHOST_OPER_DISTANCE 200
#define GHOST_OPERATE_TOO_FAST (1 << 0)
#define GHOST_OPERATE_TWO_POINT_OPER_TOO_FAST (1 << 1)
#define GHOST_DETECT_SUPPORT "ghost_detect_support"
#define GHOST_LOG_TAG "[ghost]"
#define GHOST_REASON_OPERATE_TOO_FAST "press/release too fast"
#define GHOST_REASON_TWO_POINT_OPER_TOO_FAST "operate between 2 points too fast"

int ts_register_algo_func(struct ts_device_data *chip_data);
void ts_algo_det_ght_init(void);
#endif
