 #include "huawei_ts_kit_algo.h"
#include <linux/ktime.h>

#define SENCOND_TO_MILLISECOND      1000
#define SENCOND_TO_NANOSECOND       1000000

/*lint -save -e* */
extern struct ts_kit_platform_data g_ts_kit_platform_data;
static struct timespec curr_time[FILTER_GLOVE_NUMBER] = {{0,0}};

static struct timespec pre_finger_time[FILTER_GLOVE_NUMBER] = {{0,0}};

static int touch_pos_x[FILTER_GLOVE_NUMBER] = {-1, -1, -1, -1};
static int touch_pos_y[FILTER_GLOVE_NUMBER] = {-1, -1, -1, -1};
static enum TP_state_machine  touch_state = INIT_STATE;

/*for pen*/
#define FHD_X_MAX			1080
#define PEN_MOV_PIXEL		100		//100 pixel for FHD display
#define PEN_MOV_INTERVAL	40

#define PRESS_TIME_MIN		40
#define MOVE_TIME_MIN		30
#define REL_TIME_MIN		80
#define ERROR_DELAY			500

#define PEN_REL_STATE		0
#define PEN_HOVER_STATE		1
#define PEN_PRESS_STATE		2
#define ABNORMAL_VAL		-1

static int press_state = PEN_REL_STATE;
static int start_check_flag = 1;
static int pen_pos_x = -1;
static int pen_pos_y = -1;
static int pre_x = -1;
static int pre_y = -1;
static struct timespec pre_pen_down_time = {0,0};
static struct timespec pre_pen_up_time = {0,0};


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
		} else {
			if ((in_info->fingers[index].x != 0) || (in_info->fingers[index].y != 0)) {
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
				}
			} else{
				out_info->fingers[id].status = 0;
			}
		}
	}

	out_info->cur_finger_number = finger_cnt;
	out_info->gesture_wakeup_value = in_info->gesture_wakeup_value;
	out_info->special_button_key = in_info->special_button_key;
	out_info->special_button_flag = in_info->special_button_flag;

	return NO_ERR;
}

struct ts_algo_func ts_kit_algo_f1 =
{
	.algo_name = "ts_kit_algo_f1",
	.chip_algo_func = ts_kit_algo_t1,
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

	return retval;
}
/*lint -restore*/


/*============================================================================================================================================*/
                                                        /*pen_switch_open_confirm*/
/*============================================================================================================================================*/
int ts_pen_open_confirm(struct ts_pens *pens)
{
	int x = 0;
	int y = 0;
	int leng_thr = 0;
	int mov_len = 0;
	int open_pen_flag = 0;
	long interval_time = 0;
	long interval_thr = 0;
	struct timespec cur_time = {0, 0};

	if (STYLUS_WAKEUP_LOW_FREQENCY == g_ts_kit_platform_data.feature_info.wakeup_gesture_enable_info.switch_value) {
		ktime_get_ts(&cur_time);

		if (TS_PEN_OUT_RANGE == pens->tool.pen_inrange_status) {		//pen hover leave
			press_state = PEN_REL_STATE;
		}
		else if (0 == pens->tool.pressure) {		//pen press release
			if (press_state != PEN_HOVER_STATE) {
				if (PEN_PRESS_STATE == press_state) {
					ktime_get_ts(&pre_pen_up_time);
				}
				press_state = PEN_HOVER_STATE;
				interval_time = (cur_time.tv_sec - pre_pen_down_time.tv_sec) * SENCOND_TO_MILLISECOND +
						(cur_time.tv_nsec - pre_pen_down_time.tv_nsec) / SENCOND_TO_NANOSECOND;
				if (interval_time > 0 && interval_time <= PRESS_TIME_MIN) {
					start_check_flag = 0;
				}
			}
		} else {		//pen press
			if (press_state != PEN_PRESS_STATE) {
				press_state = PEN_PRESS_STATE;
				ktime_get_ts(&pre_pen_down_time);

				interval_thr = REL_TIME_MIN;
				if (0 == start_check_flag) {
					interval_thr = ERROR_DELAY;
				}

				interval_time = (cur_time.tv_sec - pre_pen_up_time.tv_sec) * SENCOND_TO_MILLISECOND +
					(cur_time.tv_nsec - pre_pen_up_time.tv_nsec) / SENCOND_TO_NANOSECOND;
				if (interval_time > 0 && interval_time <= interval_thr) {
					start_check_flag = 0;
				} else {
					start_check_flag = 1;
					pen_pos_x = ABNORMAL_VAL;
					pen_pos_y = ABNORMAL_VAL;
				}
			}

			if (start_check_flag) {		//can check
				x = pens->tool.x;
				y = pens->tool.y;
				if ((ABNORMAL_VAL == pen_pos_x) && (ABNORMAL_VAL == pen_pos_y)) {
					pen_pos_x = x;
					pen_pos_y = y;
				} else {
					leng_thr = g_ts_kit_platform_data.chip_data->x_max * PEN_MOV_INTERVAL / FHD_X_MAX;           //about 3mm length
					leng_thr *= leng_thr;
					mov_len = (pre_x - x) * (pre_x - x) + (pre_y - y) * (pre_y - y);
					if (mov_len > leng_thr){
						start_check_flag = 0;
						TS_LOG_INFO("ts_pen_open_confirm_called, move_too_fast, THR=%d, length=%d\n", leng_thr, mov_len);
					} else {
						leng_thr = g_ts_kit_platform_data.chip_data->x_max * PEN_MOV_PIXEL / FHD_X_MAX;         //about 7mm length
						leng_thr *= leng_thr;
						mov_len = (pen_pos_x - x) * (pen_pos_x - x) + (pen_pos_y - y) * (pen_pos_y - y);
						if (mov_len >= leng_thr){
							TS_LOG_INFO("ts_pen_open_confirm_called, THR=%d, length=%d\n", leng_thr, mov_len);

							interval_time = (cur_time.tv_sec - pre_pen_down_time.tv_sec) * SENCOND_TO_MILLISECOND +
								(cur_time.tv_nsec - pre_pen_down_time.tv_nsec) / SENCOND_TO_NANOSECOND;
							if (interval_time > 0 && interval_time <= MOVE_TIME_MIN){
								start_check_flag = 0;
								TS_LOG_INFO("ts_pen_open_confirm_called, movetime_too_short, THR=%d, interval=%ld\n", MOVE_TIME_MIN, interval_time);
							} else {
								start_check_flag = 0;
								open_pen_flag = 1;                                                              //need open pen switch
								TS_LOG_ERR("ts_pen_open_confirm_called, ret_flag=%d\n", open_pen_flag);
							}
						}
					}
				}
				pre_x = x;
				pre_y = y;
			}
		}
	} else {
		TS_LOG_INFO("ts_pen_open_confirm_called, don't_need_process\n");
	}
	TS_LOG_INFO("ts_pen_open_confirm_called, start_check=%d, ret_flag=%d, z=%d\n", start_check_flag, open_pen_flag, pens->tool.pressure);
	return open_pen_flag;
}

void ts_pen_open_confirm_init(void)
{
	press_state = PEN_REL_STATE;
	start_check_flag = 1;
	pen_pos_x = ABNORMAL_VAL;
	pen_pos_y = ABNORMAL_VAL;

	ktime_get_ts(&pre_pen_down_time);
	ktime_get_ts(&pre_pen_up_time);
	TS_LOG_INFO("ts_pen_open_confirm_init_called\n");
}

