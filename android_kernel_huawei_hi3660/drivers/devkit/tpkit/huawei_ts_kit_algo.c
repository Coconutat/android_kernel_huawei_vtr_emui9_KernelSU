#include "huawei_ts_kit_algo.h"
#include <linux/ktime.h>

#define EDGE_Y_MID		1380
#define EDGE_Y_MAX		1919
#define EDGE_X_MAX		1079

#define STOP_LIMITED	150

#define Y_MOVE_S		20
#define Y_MOVE_M		40
#define Y_TEMP_W		40
#define Y_START_W		60

#define CENTER_X		500
#define OFFSET_DIS		150
#define RES_ADDR		0
#define RES_ERT			1
#define RES_ERF			2
#define RES_CRF			3
#define RES_ERT_THR		2
#define SET_TO_NEGATIVE_NUM       (-1)
#define SENCOND_TO_MILLISECOND      1000
#define SENCOND_TO_NANOSECOND       1000000

#define TEMP_EDGE           20
#define START_EDGE          30
#define START_STUDY_EDGE    40

static int stop_left = 0;
static int stop_right = 0;
static int temp_left = 0;
static int temp_right = 0;
static int start_left = 0;
static int start_right = 0;
static int start_study_left = 0;
static int start_study_right = 0;

static int temp_up = 0;
static int temp_down = 0;
static int start_up = 0;
static int start_down = 0;
/*lint -save -e* */
extern struct ts_kit_platform_data g_ts_kit_platform_data;
static struct timespec curr_time[FILTER_GLOVE_NUMBER] = {{0,0}};

static struct timespec pre_finger_time[FILTER_GLOVE_NUMBER] = {{0,0}};

static int touch_pos_x[FILTER_GLOVE_NUMBER] = {-1, -1, -1, -1};
static int touch_pos_y[FILTER_GLOVE_NUMBER] = {-1, -1, -1, -1};
static enum TP_state_machine  touch_state = INIT_STATE;

static u16 must_report_flag = 0;
static u16 pre_must_flag = 0;
static u16 stop_report_flag = 0;
static u16 temp_report_flag = 0;
static int finger_stop_cnt[TS_MAX_FINGER] = {0};
static int finger_stop_y[TS_MAX_FINGER] = {0};

static int left_res_point[4] = {0};		//0 point addr /1 error times /2 error flag /3 correct flag
static int right_res_point[4] = {0};
/*static int ts_algo_filter_anti_false_touch_edge(int x,int finger_id,  struct anti_false_touch_param *param){
	int drv_limit_x_left = 0, drv_limit_x_right = 0;
	if (!param || !param->feature_all){
		return NOT_AFT_EDGE;
	}

		drv_limit_x_left = param->drv_stop_width;
		drv_limit_x_right = param->lcd_width - drv_limit_x_left;
		
	       if (0 == (param->edge_status & (1<<finger_id))) {
			if (x < drv_limit_x_left ||x > drv_limit_x_right) {
				TS_LOG_DEBUG("%s edge, x:%d\n", __func__, x);
				return AFT_EDGE;
	       }
		else {
			param->edge_status |= (1<<finger_id);
		}
       }
	return NOT_AFT_EDGE;
}
*/
struct ghost_finger_touch{
	struct timeval finger_press_tv[TS_MAX_FINGER];
	struct timeval finger_release_tv;
	int finger_num_flag;
	int finger_event;
	int x[TS_MAX_FINGER];
	int y[TS_MAX_FINGER];
};
static struct ghost_finger_touch finger_touch;
static struct ghost_finger_touch pre_finger_touch;

static int ghost_num_record_per_second[TS_MAX_FINGER] = {0};
static unsigned long ghost_time_record[TS_MAX_FINGER] = {0};
static int ghost_num_record_x[TS_MAX_FINGER] = {0};
static int ghost_num_record_y[TS_MAX_FINGER] = {0};

static int delta_x_0 = 0;
static int delta_y_0 = 0;
static int delta_x_1 = 0;
static int delta_y_1 = 0;
static int target_x = 0;
static int target_y = 0;
static int ghost_algo3_num = 0;

void ts_kit_algo_det_ght_init(void){
	TS_LOG_INFO("%s:%s init ghost resource\n",
		__func__, GHOST_LOG_TAG);
	memset(&finger_touch, 0, sizeof(struct ghost_finger_touch));
	memset(&pre_finger_touch, 0, sizeof(struct ghost_finger_touch));
}
EXPORT_SYMBOL(ts_kit_algo_det_ght_init);
/* The last event is press, and current event is release,
 * we consider this as a down/up operate, record the release time.
 **/
static void ts_algo_det_ght_finger_release(void){
	struct timeval tv;

	do_gettimeofday(&tv);
	if (TS_FINGER_PRESS == finger_touch.finger_event){
		TS_LOG_DEBUG("%s:%s finger release\n", __func__, GHOST_LOG_TAG);
		finger_touch.finger_event = TS_FINGER_RELEASE;
		memcpy(&(finger_touch.finger_release_tv), &tv, sizeof(struct timeval));
	}
	return ;
}

/* Finger press event happened, if this is the first time of each finger
 * appear press event, we record the finger the press event in finger_num_flag
 * flag, and record the message of the point, mean time the press event happen
 * time.
**/
static void ts_algo_det_ght_finger_press(int index, int x, int y){
	struct timeval tv;

	TS_LOG_DEBUG("%s:%s finger press, index:%d\n", __func__, GHOST_LOG_TAG, index);
	do_gettimeofday(&tv);
	finger_touch.finger_event = TS_FINGER_PRESS;

	if (!(finger_touch.finger_num_flag & (1 << index))){
		TS_LOG_DEBUG("%s:%s record finger_num_flag:%d\n",
			 __func__, GHOST_LOG_TAG, finger_touch.finger_num_flag);
		finger_touch.x[index] = x;
		finger_touch.y[index] = y;
		finger_touch.finger_num_flag |= 1 << index;
		memcpy(&(finger_touch.finger_press_tv[index]), &tv, sizeof(struct timeval));
	}
	return ;
}

/* If we detect user down/up interval time less than FINGER_PRESS_TIME_MIN,
 * we consider this is a Non human being operation.
 * and the user operate FINGER_PRESS_TIMES_IN_MIN_TIME times in one second
 * this operation, we think this should not a human being operate
 **/
static int ts_detect_ghost_algo1(int index) {
	unsigned long delta_time = 0;

	if((index >= GHOST_OPERATE_MAX_FINGER_NUM)||(index < 0)) {
		TS_LOG_ERR("%s:%s Input parameter error\n",
				__func__, GHOST_LOG_TAG);
		return 0;
	}

	delta_time = (finger_touch.finger_release_tv.tv_sec - finger_touch.finger_press_tv[index].tv_sec)*GHOST_MIL_SECOND_TIME
		+ finger_touch.finger_release_tv.tv_usec - finger_touch.finger_press_tv[index].tv_usec;
	if (delta_time > 0 && delta_time < FINGER_PRESS_TIME_MIN_ALGO1){
		TS_LOG_DEBUG("%s:%s suspicious ghost press finger_release_tv.tv_sec:%lu,finger_release_tv.tv_usec:%lu,"
			"finger_press_tv[%d].tv_sec:%lu,finger_press_tv[%d].tv_usec:%lu,delta time:%lu\n",__func__, GHOST_LOG_TAG,
			finger_touch.finger_release_tv.tv_sec, finger_touch.finger_release_tv.tv_usec,
			index, finger_touch.finger_press_tv[index].tv_sec, index, finger_touch.finger_press_tv[index].tv_usec, delta_time);
		if (!ghost_time_record[index]){
			ghost_time_record[index] = finger_touch.finger_release_tv.tv_sec;
			ghost_num_record_per_second[index]++;
		}else if ((ghost_time_record[index] == finger_touch.finger_release_tv.tv_sec)
				||(ghost_time_record[index] == (finger_touch.finger_release_tv.tv_sec - GHOST_OPERATE_ONE_SECOND))){
			ghost_num_record_per_second[index]++;
		}else{
			ghost_num_record_per_second[index] = 0;
			ghost_time_record[index] = 0;
		}
		TS_LOG_DEBUG("%s:%s ghost_num_record_per_second:%d\n",
			__func__, GHOST_LOG_TAG, ghost_num_record_per_second[index]);

		if (ghost_num_record_per_second[index] > FINGER_PRESS_TIMES_IN_MIN_TIME_ALGO1){
			ghost_num_record_per_second[index] = 0;
			ghost_time_record[index] = 0;
			TS_LOG_INFO("%s:%s DETECT:%s\n",
				__func__, GHOST_LOG_TAG, GHOST_REASON_OPERATE_TOO_FAST);
			return GHOST_OPERATE_TOO_FAST;
		}else{
			return 0;
		}
	}
	return 0;
}

/*If there are FINGER_PRESS_TIMES_IN_MIN_TIME_ALGO2 consecutive down/up operations,
 * the distance of x or y axis coordinate between each operation is less than GHOST_OPER_DISTANCE_ALGO2 pixe,
 *we think this should not a human being operate
 **/
static int ts_detect_ghost_algo2(int index) {
	int delta_x = 0;
	int delta_y = 0;

	if((index >= GHOST_OPERATE_MAX_FINGER_NUM)||(index < 0)) {
		TS_LOG_ERR("%s:%s Input parameter error\n",
				__func__, GHOST_LOG_TAG);
		return 0;
	}

	delta_x = finger_touch.x[index] - pre_finger_touch.x[index];
	delta_y = finger_touch.y[index] - pre_finger_touch.y[index];
	if ((delta_x < GHOST_OPER_DISTANCE_ALGO2) && (delta_x > SET_TO_NEGATIVE_NUM * GHOST_OPER_DISTANCE_ALGO2)){
		ghost_num_record_x[index] ++;
	}else{
		ghost_num_record_x[index] = 0;
	}
	if((delta_y < GHOST_OPER_DISTANCE_ALGO2) && (delta_y > SET_TO_NEGATIVE_NUM * GHOST_OPER_DISTANCE_ALGO2)){
		ghost_num_record_y[index] ++;
	}else{
		ghost_num_record_y[index] = 0;
	}
	TS_LOG_DEBUG("%s:%s ghost_num_record_x[%d] :%d ghost_num_record_y[%d]:%d\n",
		__func__, GHOST_LOG_TAG, index,ghost_num_record_x[index],index,ghost_num_record_y[index]);

	if((ghost_num_record_x[index] > FINGER_PRESS_TIMES_IN_MIN_TIME_ALGO2)
		||(ghost_num_record_y[index] >  FINGER_PRESS_TIMES_IN_MIN_TIME_ALGO2)){
		TS_LOG_INFO("%s:%s DETECT:%s\n",__func__, GHOST_LOG_TAG, GHOST_OPERATE_IN_XY_AXIS);
		ghost_num_record_x[index] = 0;
		ghost_num_record_y[index] = 0;
		return GHOST_OPERATE_IN_XY_AXIS;
	}
	return 0;
}

/*If we detect two fingers down,the interval time is less than FINGER_PRESS_TIME_MIN_ALGO3,
 *One of the fingers is always in the same position(the distance is less than GHOST_OPER_DISTANCE_ALGO3)
 *and the user operate consecutive FINGER_PRESS_TIMES_IN_MIN_TIME_ALGO3 times,
 *we think this should not a human being operate.
 **/
static int ts_detect_ghost_algo3(int finger_num) {
	unsigned long delta_time = 0;
	int i = 0;
	int j = 0;
	int delta_x = 0;
	int delta_y = 0;

	if ((finger_num == GHOST_OPERATE_TWO_FINGERS)&&(pre_finger_touch.finger_num_flag > GHOST_OPERATE_ONE_FINGERS)){
		if(target_x == 0){
			delta_time = (finger_touch.finger_press_tv[1].tv_sec - finger_touch.finger_press_tv[0].tv_sec)*GHOST_MIL_SECOND_TIME
				+ finger_touch.finger_press_tv[1].tv_sec -finger_touch.finger_press_tv[0].tv_sec;
			if(delta_time < FINGER_PRESS_TIME_MIN_ALGO3) {
				for(i=0;i<GHOST_OPERATE_TWO_FINGERS;i++)
					for(j=0;j<GHOST_OPERATE_TWO_FINGERS;j++){
						delta_x = finger_touch.x[i] - pre_finger_touch.x[j];
						delta_y = finger_touch.y[i] - pre_finger_touch.y[j];
						if ((delta_x < GHOST_OPER_DISTANCE_ALGO3) && (delta_x > SET_TO_NEGATIVE_NUM * GHOST_OPER_DISTANCE_ALGO3)
						 &&(delta_y < GHOST_OPER_DISTANCE_ALGO3) && (delta_y > SET_TO_NEGATIVE_NUM * GHOST_OPER_DISTANCE_ALGO3) ) {
							target_x = finger_touch.x[i];
							target_y = finger_touch.y[i];
							i = GHOST_OPERATE_TWO_FINGERS;
							break;
						}
					}
				}
			}else{
				/*Calculate the distance between the first point and the target point*/
				delta_x_0 = finger_touch.x[0] - target_x;
				delta_y_0 = finger_touch.y[0] - target_y;
				/*Calculate the distance between the second point and the target point*/
				delta_x_1 = finger_touch.x[1] - target_x;
				delta_y_1 = finger_touch.y[1] - target_y;
				delta_time = (finger_touch.finger_press_tv[1].tv_sec - finger_touch.finger_press_tv[0].tv_sec)*GHOST_MIL_SECOND_TIME
						+ finger_touch.finger_press_tv[1].tv_sec -finger_touch.finger_press_tv[0].tv_sec;
				if ((delta_x_0 < GHOST_OPER_DISTANCE_ALGO3) && (delta_x_0 > SET_TO_NEGATIVE_NUM * GHOST_OPER_DISTANCE_ALGO3)&&(delta_time < FINGER_PRESS_TIME_MIN_ALGO3)
					&&(delta_y_0 < GHOST_OPER_DISTANCE_ALGO3) && (delta_y_0 > SET_TO_NEGATIVE_NUM * GHOST_OPER_DISTANCE_ALGO3) ) {
					ghost_algo3_num++;
				}else if((delta_x_1 < GHOST_OPER_DISTANCE_ALGO3) && (delta_x_1 > SET_TO_NEGATIVE_NUM*GHOST_OPER_DISTANCE_ALGO3)&&(delta_time < FINGER_PRESS_TIME_MIN_ALGO3)
					&&(delta_y_1 < GHOST_OPER_DISTANCE_ALGO3) && (delta_y_1 > SET_TO_NEGATIVE_NUM * GHOST_OPER_DISTANCE_ALGO3)) {
					ghost_algo3_num++;
				}else{
					ghost_algo3_num = 0;
					target_x = 0;
					target_y = 0;
				}

				TS_LOG_DEBUG("%s:%s ghost_algo3_num:%d\n",__func__, GHOST_LOG_TAG,ghost_algo3_num);
				if(FINGER_PRESS_TIMES_IN_MIN_TIME_ALGO3 == ghost_algo3_num) {
					TS_LOG_INFO("%s:%s DETECT:%s\n",
						__func__, GHOST_LOG_TAG, GHOST_OPERATE_IN_SAME_POSITION);
					target_x = 0;
					target_y = 0;
					ghost_algo3_num = 0;
					return GHOST_OPERATE_IN_SAME_POSITION;
				}
			}
	}else{
		target_x = 0;
		target_y = 0;
		ghost_algo3_num = 0;
	}
	return 0;
}

static void ts_algo_detect_ghost(int finger_cnt){
	int index = 0;
	int ghost_detect_flag = 0;
	static int finger_num = 0;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	if (TS_FINGER_RELEASE != finger_touch.finger_event){
		if(finger_num < finger_cnt){
			finger_num = finger_cnt;
		}
		TS_LOG_DEBUG("%s:%s fingers not release, do not analyse\n", __func__, GHOST_LOG_TAG);
		return ;
	}

	TS_LOG_DEBUG("%s:%s finger_num_flag:%d finger_num:%d\n",
		__func__, GHOST_LOG_TAG, finger_touch.finger_num_flag,finger_num);
	/* If detect some down/up operate, start to analyse */
	if (finger_touch.finger_num_flag){
		for (index = 0; index < TS_MAX_FINGER; index++){
			/* Find the finger which down/up operate happened */
			if (finger_touch.finger_num_flag & (1 << index)){
				ghost_detect_flag |= ts_detect_ghost_algo1(index);
				if (pre_finger_touch.finger_num_flag & (1 << index)){
					ghost_detect_flag |= ts_detect_ghost_algo2(index);
				}
			}
		}
		ghost_detect_flag |= ts_detect_ghost_algo3(finger_num);
		if(ghost_detect_flag) {
			if (dev->ops->chip_ghost_detect){
				dev->ops->chip_ghost_detect(ghost_detect_flag);
			}else{
				TS_LOG_INFO("chip_ghost_detect is null\n");
			}
		}
		finger_num = 0;
		/* Save current finger message as last finger message */
		memcpy(&pre_finger_touch, &finger_touch, sizeof(struct ghost_finger_touch));
		memset(&finger_touch, 0, sizeof(struct ghost_finger_touch));
	}else{
		TS_LOG_DEBUG("touch release, but we do not detect any touch press, what happened?\n");
	}
	TS_LOG_DEBUG("%s -\n", __func__);
	return ;
}

static int filter_illegal_glove(u8 n_finger, struct ts_fingers *in_info)
{
	u8 report_flag = 0;
	long interval_time;
	u8 new_mode;
	int x = in_info->fingers[n_finger].x;
	int y = in_info->fingers[n_finger].y;
	new_mode = in_info->fingers[n_finger].status;

	if (new_mode == TP_FINGER || g_ts_kit_platform_data.feature_info.holster_info.holster_switch) { /*the new interrupt is a finger signal*/
		touch_state = FINGER_STATE;
		report_flag = 1;
	} else if ((new_mode == TP_GLOVE) || (new_mode == TP_STYLUS)) { /*the new interrupt is a glove signal.*/
		switch (touch_state) {
			case INIT_STATE:
				report_flag = 1;
				touch_state = GLOVE_STATE;
				break;

			case FINGER_STATE:
				ktime_get_ts(&curr_time[n_finger]);
				interval_time = (curr_time[n_finger].tv_sec - pre_finger_time[n_finger].tv_sec)*SENCOND_TO_MILLISECOND
					+ (curr_time[n_finger].tv_nsec - pre_finger_time[n_finger].tv_nsec)/SENCOND_TO_NANOSECOND;
				if (interval_time > 0 && interval_time <= FINGER_REL_TIME) {
					ktime_get_ts(&pre_finger_time[n_finger]);
				} else {
					touch_state = ZERO_STATE;
				}
				break;

			case ZERO_STATE:
				if ((touch_pos_x[n_finger] == -1) && (touch_pos_y[n_finger] == -1)) {
					touch_pos_x[n_finger] = x;
					touch_pos_y[n_finger] = y;
				} else {
					if (((touch_pos_x[n_finger] - x)*(touch_pos_x[n_finger] - x)
						+ (touch_pos_y[n_finger] - y)*(touch_pos_y[n_finger] - y))
						>= (PEN_MOV_LENGTH * PEN_MOV_LENGTH)) {
						touch_state = GLOVE_STATE;
					}
				}
				break;

			case GLOVE_STATE:
				report_flag = 1;
				break;

			default:
				TS_LOG_ERR("error: touch_state = %d\n", touch_state);
				break;
		}
	}else {
		TS_LOG_ERR("[%s]:cur_mode=%d\n", __func__, new_mode);
		report_flag = 1;
	}

	return report_flag;
}


int ts_kit_algo_t2(struct ts_kit_device_data *dev_data, struct ts_fingers *in_info, struct ts_fingers *out_info)
{
	int index = 0;
	int id = 0;
	struct anti_false_touch_param *local_param = NULL;

	if(!in_info || !out_info){
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	if (NULL == dev_data){
		TS_LOG_DEBUG("%s anti false touch get chip data NULL\n", __func__);
		local_param = NULL;
	}else{
		local_param = &(dev_data->anti_false_touch_param_data);
	}

	memset(out_info, 0, sizeof(struct ts_fingers));

	for (index = 0, id = 0; index < TS_MAX_FINGER; index++, id++) {
		if (in_info->cur_finger_number == 0) {
			out_info->fingers[0].status = TS_FINGER_RELEASE;
			if (local_param && local_param->feature_all){
				local_param->edge_status = 0;
			}
			if (id >= 1)
				out_info->fingers[id].status = 0;
			if (dev_data && dev_data->ghost_detect_support){
				ts_algo_det_ght_finger_release();
			}
		} else {
			if ((in_info->fingers[index].x != 0) ||(in_info->fingers[index].y != 0)) {
				out_info->fingers[id].x = in_info->fingers[index].x;
				out_info->fingers[id].y = in_info->fingers[index].y;
				out_info->fingers[id].pressure = in_info->fingers[index].pressure;
				out_info->fingers[id].major = in_info->fingers[index].major;
				out_info->fingers[id].minor = in_info->fingers[index].minor;
				out_info->fingers[id].wx = in_info->fingers[index].wx;
				out_info->fingers[id].wy = in_info->fingers[index].wy;
				out_info->fingers[id].ewx = in_info->fingers[index].ewx;
				out_info->fingers[id].ewy = in_info->fingers[index].ewy;
				out_info->fingers[id].xer = in_info->fingers[index].xer;
				out_info->fingers[id].yer = in_info->fingers[index].yer;
				out_info->fingers[id].orientation = in_info->fingers[index].orientation;
				out_info->fingers[id].status = TS_FINGER_PRESS;

				if (dev_data && dev_data->ghost_detect_support){
					ts_algo_det_ght_finger_press(index, out_info->fingers[id].x, out_info->fingers[id].y);
				}
			} else{
				out_info->fingers[id].status = 0;
				if (local_param && local_param->feature_all){
					local_param->edge_status &= ~(1 << id);
				}
			}
		}
	}

	if (dev_data && dev_data->ghost_detect_support){
		ts_algo_detect_ghost(in_info->cur_finger_number);
	}

	out_info->cur_finger_number = in_info->cur_finger_number;
	out_info->gesture_wakeup_value = in_info->gesture_wakeup_value;
	out_info->special_button_key = in_info->special_button_key;
	out_info->special_button_flag = in_info->special_button_flag;

	return NO_ERR;
}


int ts_kit_algo_t1(struct ts_kit_device_data *dev_data, struct ts_fingers *in_info, struct ts_fingers *out_info)
{
	int index = 0;
	int id = 0;
	int finger_cnt = 0;
	struct anti_false_touch_param *local_param = NULL;

	if(!in_info || !out_info){
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	if (NULL == dev_data){
		TS_LOG_ERR("%s anti false touch get chip data NULL\n", __func__);
		local_param = NULL;
	}else{
		local_param = &(dev_data->anti_false_touch_param_data);
	}

	memset(out_info, 0, sizeof(struct ts_fingers));

	for (index = 0, id = 0; index < TS_MAX_FINGER; index++, id++) {
		if (in_info->cur_finger_number == 0) {
			if (index < FILTER_GLOVE_NUMBER) {
				touch_pos_x[index] = -1;
				touch_pos_y[index] = -1;
				if (touch_state == FINGER_STATE) {	/*this is a finger release */
					ktime_get_ts(&pre_finger_time[index]);
				}
			}
			out_info->fingers[0].status = TS_FINGER_RELEASE;
			if (local_param && local_param->feature_all){
				local_param->edge_status = 0;
			}
			if (id >= 1)
				out_info->fingers[id].status = 0;
			if (dev_data && dev_data->ghost_detect_support){
				ts_algo_det_ght_finger_release();
			}
		} else {
			if ((in_info->fingers[index].x != 0) ||(in_info->fingers[index].y != 0)) {
				if (index < FILTER_GLOVE_NUMBER) {
					if (filter_illegal_glove(index, in_info) == 0) {
						out_info->fingers[id].status = 0;
					} else {
						finger_cnt++;
						out_info->fingers[id].x = in_info->fingers[index].x;
						out_info->fingers[id].y = in_info->fingers[index].y;
						out_info->fingers[id].pressure = in_info->fingers[index].pressure;
						out_info->fingers[id].major = in_info->fingers[index].major;
						out_info->fingers[id].minor = in_info->fingers[index].minor;
						out_info->fingers[id].wx = in_info->fingers[index].wx;
						out_info->fingers[id].wy = in_info->fingers[index].wy;
						out_info->fingers[id].ewx = in_info->fingers[index].ewx;
						out_info->fingers[id].ewy = in_info->fingers[index].ewy;
						out_info->fingers[id].xer = in_info->fingers[index].xer;
						out_info->fingers[id].yer = in_info->fingers[index].yer;
						out_info->fingers[id].orientation = in_info->fingers[index].orientation;
						out_info->fingers[id].status = TS_FINGER_PRESS;

						if (dev_data && dev_data->ghost_detect_support){
							ts_algo_det_ght_finger_press(index, out_info->fingers[id].x, out_info->fingers[id].y);
						}
					}
				} else {
					finger_cnt++;
					out_info->fingers[id].x = in_info->fingers[index].x;
					out_info->fingers[id].y = in_info->fingers[index].y;
					out_info->fingers[id].pressure = in_info->fingers[index].pressure;
					out_info->fingers[id].major = in_info->fingers[index].major;
					out_info->fingers[id].minor = in_info->fingers[index].minor;
					out_info->fingers[id].wx = in_info->fingers[index].wx;
					out_info->fingers[id].wy = in_info->fingers[index].wy;
					out_info->fingers[id].ewx = in_info->fingers[index].ewx;
					out_info->fingers[id].ewy = in_info->fingers[index].ewy;
					out_info->fingers[id].xer = in_info->fingers[index].xer;
					out_info->fingers[id].yer = in_info->fingers[index].yer;
					out_info->fingers[id].orientation = in_info->fingers[index].orientation;
					out_info->fingers[id].status = TS_FINGER_PRESS;

					if (dev_data && dev_data->ghost_detect_support){
						ts_algo_det_ght_finger_press(index, out_info->fingers[id].x, out_info->fingers[id].y);
					}
				}
			} else{
				out_info->fingers[id].status = 0;
				if (local_param && local_param->feature_all){
					local_param->edge_status &= ~(1 << id);
				}
			}
		}
	}

	if (dev_data && dev_data->ghost_detect_support){
		ts_algo_detect_ghost(finger_cnt);
	}
	out_info->cur_finger_number = finger_cnt;
	out_info->gesture_wakeup_value = in_info->gesture_wakeup_value;
	out_info->special_button_key = in_info->special_button_key;
	out_info->special_button_flag = in_info->special_button_flag;

	return NO_ERR;
}

static int stop_to_start(int x, int y, int start_y, int cnt, int *point)
{
	int temp_value;
	temp_value = y - start_y;

	if (!((y > point[RES_ADDR] - OFFSET_DIS) && (y < point[RES_ADDR] + OFFSET_DIS)))
	{
		if (temp_value > Y_MOVE_S)
		{
			return 1;
		}
		if (temp_value < 0)
		{
			temp_value = 0 - temp_value;
			if (temp_value > Y_MOVE_M)
			{
				return 2;
			}
		}
		if ((x > start_left) && (x < start_right) && (y > start_up && y < start_down))
		{
			return 3;
		}
	}
	else
	{
		if ((x > (start_study_left + cnt)) && (x < (start_study_right - cnt)) && (y > start_up && y < start_down))
		{
			return 4;
		}
	}
	return 0;
}

static int start_stop_area(int x, int y, int *point)
{
	if (x < stop_left || x > stop_right)
	{
		return 1;
	}
	return 0;
}

static int update_restrain_area(int y, int *point)
{
	if (y > EDGE_Y_MID)													//restrain area at the bottom screen
	{
		if (0 == point[RES_ADDR])										//restrain area not exist, set it up
		{
			point[RES_CRF] = 1;											//correct flag
			point[RES_ADDR] = y;
		}
		else if ((y > point[RES_ADDR] - OFFSET_DIS) && (y < point[RES_ADDR] + OFFSET_DIS))
		{																//adust the area
			point[RES_CRF] = 1;											//correct flag
			point[RES_ADDR] = (point[RES_ADDR]*3 + y) >> 2;

			TS_LOG_DEBUG("restrain point updated: %d\n", point[RES_ADDR]);
		}
		else															//not in the current restrain area
		{
			point[RES_ERF] = 1;											//error flag
		}
	}
	return 0;
}

int ts_kit_algo_t3(struct ts_kit_device_data *dev_data, struct ts_fingers *in_info, struct ts_fingers *out_info)
{
	int index = 0;
	int temp_x = 0, temp_y = 0, temp_val = 0;
	int *temp_point = NULL;

	if(!in_info || !out_info){
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	if (0 == g_ts_kit_platform_data.edge_wideth)
	{	
		return NO_ERR;
	}
	if (g_ts_kit_platform_data.edge_wideth != stop_left)
	{
		stop_left = g_ts_kit_platform_data.edge_wideth;
		stop_right = EDGE_X_MAX - stop_left;
		temp_left = stop_left + TEMP_EDGE;
		temp_right = stop_right - TEMP_EDGE;

		start_left = stop_left + START_EDGE;
		start_right = stop_right - START_EDGE;

		temp_up = Y_TEMP_W;
		temp_down = EDGE_Y_MAX - Y_TEMP_W;
		start_up = Y_START_W;
		start_down = EDGE_Y_MAX - Y_START_W;

		start_study_left = stop_left + START_STUDY_EDGE;
		start_study_right = stop_right - START_STUDY_EDGE;
	}

	if (in_info->cur_finger_number == 0)
	{
		TS_LOG_DEBUG("no finger, only a release issue\n");

		if ((left_res_point[RES_CRF] == 0) && (left_res_point[RES_ERF] != 0))
		{
			if (left_res_point[RES_ERT] < RES_ERT_THR)
			{
				left_res_point[RES_ERT]++;
				if (left_res_point[RES_ERT] >= RES_ERT_THR)
				{
					left_res_point[RES_ADDR] = 0;
					left_res_point[RES_ERT] = 0;
				}
			}
		}
		if ((right_res_point[RES_CRF] == 0) && (right_res_point[RES_ERF] != 0))
		{
			if (right_res_point[RES_ERT] < RES_ERT_THR)
			{
				right_res_point[RES_ERT]++;
				if (right_res_point[RES_ERT] >= RES_ERT_THR)
				{
					right_res_point[RES_ADDR] = 0;
					right_res_point[RES_ERT] = 0;
				}
			}
		}
		left_res_point[RES_CRF] = 0;
		left_res_point[RES_ERF] = 0;
		right_res_point[RES_CRF] = 0;
		right_res_point[RES_ERF] = 0;

		must_report_flag = 0;
		stop_report_flag = 0;
		temp_report_flag = 0;
		for (index = 0; index < TS_MAX_FINGER; index++)
		{
			finger_stop_cnt[index] = 0;
			finger_stop_y[index] = 0;
		}
	}
	else
	{
		for (index = 0; index < TS_MAX_FINGER; index++)
		{
			if (in_info->fingers[index].status != 0)
			{
				temp_x = in_info->fingers[index].x;
				temp_y = in_info->fingers[index].y;
				if (must_report_flag & (1 << index))
				{
					TS_LOG_DEBUG("finger index: %d, is reporting \n", index);

					out_info->fingers[index].x = temp_x;
					out_info->fingers[index].y = temp_y;
					out_info->fingers[index].pressure = in_info->fingers[index].pressure;
					out_info->fingers[index].status = TS_FINGER_PRESS;
				}
				else
				{
					temp_point = left_res_point;
					if (temp_x > CENTER_X)
						temp_point = right_res_point;

					if (stop_report_flag & (1 << index))
					{
						temp_val = stop_to_start(temp_x, temp_y, finger_stop_y[index], finger_stop_cnt[index], temp_point);
						TS_LOG_DEBUG("stop_to_start ret_value: %d, \n", temp_val);
						if (temp_val)
						{
							TS_LOG_DEBUG("stopped finger index: %d, coordinate is legal1, can report again \n", index);

							must_report_flag |= (1 << index);
							stop_report_flag &= ~(1 << index);

							out_info->fingers[index].x = temp_x;
							out_info->fingers[index].y = temp_y;
							out_info->fingers[index].pressure = in_info->fingers[index].pressure;
							out_info->fingers[index].status = TS_FINGER_PRESS;
						}
						else
						{
							out_info->fingers[index].status = 0;
							if (finger_stop_cnt[index] < STOP_LIMITED)
								finger_stop_cnt[index]++;
							TS_LOG_DEBUG("finger index: %d, keep stopped, stop_cnt: %d \n", index, finger_stop_cnt[index]);

							update_restrain_area(temp_y, temp_point);
						}
					}
					else
					{
						if (temp_x < temp_left || temp_x > temp_right 
							|| temp_y < temp_up || temp_y > temp_down)							//this area maybe need to restrain
						{
							TS_LOG_DEBUG("finger index: %d, stop judge\n", index);

							out_info->fingers[index].status = 0;								//don't report first (important)

							if (!(temp_report_flag & (1 << index)))								//has been report first
							{
								if (start_stop_area(temp_x, temp_y, temp_point))
								{																//current point in direct restrain area
									TS_LOG_DEBUG("finger need stopped \n");

									stop_report_flag |= 1 << index;
									finger_stop_cnt[index] = 1;
									finger_stop_y[index] = temp_y;

									update_restrain_area(temp_y, temp_point);
								}
								else
								{
									temp_report_flag |= 1 << index;
								}
							}
						}
						else
						{
							TS_LOG_DEBUG("finger index: %d, start OK, directly report \n", index);

							if ((!(temp_report_flag & (1 << index))) ||							//not in edge area forever
								(((temp_x > start_left ) && (temp_x < start_right))
								&& (temp_y > start_up && temp_y < start_down)))					//not in edge area now
							{
									must_report_flag |= 1 << index;
									temp_report_flag &= ~(1 << index);
							}

							out_info->fingers[index].x = in_info->fingers[index].x;
							out_info->fingers[index].y = in_info->fingers[index].y;
							out_info->fingers[index].pressure = in_info->fingers[index].pressure;
							out_info->fingers[index].status = TS_FINGER_PRESS;
						}
					}
				}
			}
			else {
				out_info->fingers[index].status = 0;

				must_report_flag &= ~(1 << index);
				stop_report_flag &= ~(1 << index);
				temp_report_flag &= ~(1 << index);
				finger_stop_cnt[index] = 0;
				finger_stop_y[index] = 0;
			}
		}

		TS_LOG_DEBUG("1_must_report_flag=%d, stop=%d, temp=%d, L_RES_ADDR=%d, L_RES_ERT=%d, R_RES_ADDR=%d, R_RES_ERT=%d\n",
			must_report_flag, stop_report_flag, temp_report_flag, left_res_point[RES_ADDR], left_res_point[RES_ERT],
			right_res_point[RES_ADDR], right_res_point[RES_ERT]);

		if (temp_report_flag)
		{
			if (must_report_flag)
			{
				for (index = 0; index < TS_MAX_FINGER; index++)
				{
					//if ((temp_report_flag & (1 << index)) && (!(must_report_flag & (1 << index))))
					if (temp_report_flag & (1 << index))
					{
						if (!(stop_report_flag & (1 << index)))
						{
							finger_stop_cnt[index] = 1;
							finger_stop_y[index] = in_info->fingers[index].y;

							out_info->fingers[index].status = 0;
						}
					}
				}
				stop_report_flag |= temp_report_flag;
				temp_report_flag = 0;

				if (0 == pre_must_flag)
				{
					out_info->fingers[0].status = TS_FINGER_RELEASE;
				}
			}
			else
			{
				for (index = 0; index < TS_MAX_FINGER; index++)
				{
					//if ((temp_report_flag & (1 << index)) && (!(stop_report_flag & (1 << index))))
					if (temp_report_flag & (1 << index))										//temp_report_flag bit won't the same with stop_report_flag bit
					{
						out_info->fingers[index].x = in_info->fingers[index].x;
						out_info->fingers[index].y = in_info->fingers[index].y;
						out_info->fingers[index].pressure = in_info->fingers[index].pressure;
						out_info->fingers[index].status = TS_FINGER_PRESS;
					}
				}
			}
		}

		TS_LOG_DEBUG("2_must_report_flag=%d, stop=%d, temp=%d, L_RES_ADDR=%d, L_RES_ERT=%d, R_RES_ADDR=%d, R_RES_ERT=%d\n",
			must_report_flag, stop_report_flag, temp_report_flag, left_res_point[RES_ADDR], left_res_point[RES_ERT],
			right_res_point[RES_ADDR], right_res_point[RES_ERT]);

		if ((must_report_flag | temp_report_flag)  == 0)
			out_info->fingers[0].status = TS_FINGER_RELEASE;
	}
	pre_must_flag = must_report_flag;
	return NO_ERR;
}


struct ts_algo_func ts_kit_algo_f1=
{
	.algo_name = "ts_kit_algo_f1",
	.chip_algo_func = ts_kit_algo_t1,
};

struct ts_algo_func ts_kit_algo_f2 =
{
	.algo_name = "ts_kit_algo_f2",
	.chip_algo_func = ts_kit_algo_t2,
};

struct ts_algo_func ts_kit_algo_f3 =
{
	.algo_name = "ts_kit_algo_f3",
	.chip_algo_func = ts_kit_algo_t3,
};

 int register_ts_algo_func(struct ts_kit_device_data *chip_data, struct ts_algo_func *fn)
{
	int error = -EIO;

	if (!chip_data ||!fn)
		goto out;

	fn->algo_index = chip_data->algo_size;
	list_add_tail( &fn->node, &chip_data->algo_head);
	chip_data->algo_size++;
	smp_mb();
	error = NO_ERR;

out:
	return error;
}
EXPORT_SYMBOL(register_ts_algo_func);
 
int ts_kit_register_algo_func(struct ts_kit_device_data *chip_data)
{
	int retval = 0;

	retval = register_ts_algo_func(chip_data, &ts_kit_algo_f1);	//put algo_f1 into list contained in chip_data, named algo_t1
	if (retval < 0) {
		TS_LOG_ERR("alog 1 failed, retval = %d\n", retval);
		return retval;
	}

	retval = register_ts_algo_func(chip_data, &ts_kit_algo_f2);	//put algo_f2 into list contained in chip_data, named algo_t2
	if (retval < 0) {
		TS_LOG_ERR("alog 2 failed, retval = %d\n", retval);
		return retval;
	}

	retval = register_ts_algo_func(chip_data, &ts_kit_algo_f3);	//put algo_f3 into list contained in chip_data, named algo_t3
	if (retval < 0) {
		TS_LOG_ERR("alog 3 failed, retval = %d\n", retval);
		return retval;
	}

	return retval;
}
/*lint -restore*/
