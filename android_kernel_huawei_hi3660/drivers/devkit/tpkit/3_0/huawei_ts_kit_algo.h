#ifndef __HUAWEI_TOUCHSCREEN_ALGO_H_
#define __HUAWEI_TOUCHSCREEN_ALGO_H_

#include "huawei_ts_kit.h"
/*
#define TP_FINGER				1
#define TP_STYLUS				2
#define TP_GLOVE				6
#define FILTER_GLOVE_NUMBER	4
*/
#define PEN_MOV_LENGTH      120	/*move length (pixels)*/
#define FINGER_REL_TIME     300	/*the time pen checked after finger released shouldn't less than this value(ms)*/

#define AFT_EDGE 1
#define NOT_AFT_EDGE 0

//static char first_touch_tag = 0;
#define FINGER_PRESS_TIME_MIN_ALGO1    20 * 1000 /* mil second */
#define FINGER_PRESS_TIME_MIN_ALGO3    20 * 1000 /* mil second */
#define FINGER_PRESS_TIMES_IN_MIN_TIME_ALGO1    5
#define FINGER_PRESS_TIMES_IN_MIN_TIME_ALGO2    5
#define FINGER_PRESS_TIMES_IN_MIN_TIME_ALGO3    3
#define GHOST_MIL_SECOND_TIME 1000000
#define GHOST_OPER_DISTANCE_ALGO2 2
#define GHOST_OPER_DISTANCE_ALGO3 2
#define GHOST_OPERATE_TOO_FAST (1 << 0)
#define GHOST_OPERATE_IN_XY_AXIS (1 << 1)
#define GHOST_OPERATE_TWO_POINT_OPER_TOO_FAST (1 << 1)
#define GHOST_OPERATE_IN_SAME_POSITION (1 << 2)
#define GHOST_OPERATE_ONE_FINGERS 1
#define GHOST_OPERATE_TWO_FINGERS 2
#define GHOST_OPERATE_MAX_FINGER_NUM 10
#define GHOST_OPERATE_ONE_SECOND 1
#define GHOST_LOG_TAG "[ghost]"
#define GHOST_REASON_OPERATE_TOO_FAST "press/release too fast"
#define GHOST_REASON_TWO_POINT_OPER_TOO_FAST "operate between 2 points too fast"
int ts_kit_register_algo_func(struct ts_kit_device_data *chip_data);

extern void ts_pen_open_confirm_init(void);
extern int ts_pen_open_confirm(struct ts_pens *pens);
#endif
