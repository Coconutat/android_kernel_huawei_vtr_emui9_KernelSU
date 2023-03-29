/*
 * Huawei Touchscreen Driver
 *
 * Copyright (C) 2013 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/sched/rt.h>
#include <linux/fb.h>
#include <linux/workqueue.h>
#include <huawei_ts_kit.h>
#include <huawei_ts_kit_platform.h>
#include <huawei_ts_kit_api.h>
#include <huawei_ts_kit_misc.h>
#include "trace-events-touch.h"
#include "huawei_ts_kit_algo.h"
#include <tpkit_platform_adapter.h>

#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
extern struct dsm_client *ts_dclient;
#endif
extern unsigned int  is_all_finger_up;

extern struct ts_kit_platform_data g_ts_kit_platform_data;
atomic_t g_ts_kit_data_report_over = ATOMIC_INIT(1);
/*lint -save -e* */
void ts_proc_bottom_half(struct ts_cmd_node *in_cmd,
			 struct ts_cmd_node *out_cmd)
{
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	TS_LOG_DEBUG("bottom half called\n");

	trace_touch(TOUCH_TRACE_IRQ_BOTTOM, TOUCH_TRACE_FUNC_IN, NULL);
	atomic_set(&g_ts_kit_data_report_over, 0);
	//related event need process, use out cmd to notify
	if (dev->ops->chip_irq_bottom_half) {
		dev->ops->chip_irq_bottom_half(in_cmd, out_cmd);
	}
	trace_touch(TOUCH_TRACE_IRQ_BOTTOM, TOUCH_TRACE_FUNC_OUT, NULL);
}

void ts_work_after_input(void)
{
	struct ts_kit_device_data *chipdata = g_ts_kit_platform_data.chip_data;
	trace_touch(TOUCH_TRACE_GET_ROI_OR_DIFF_DATA, TOUCH_TRACE_FUNC_IN, "read_diff");
	if (chipdata->ops->chip_work_after_input)
		chipdata->ops->chip_work_after_input();
	trace_touch(TOUCH_TRACE_GET_ROI_OR_DIFF_DATA, TOUCH_TRACE_FUNC_OUT, "read_diff");
}
void dump_fingers_info_debug(struct ts_finger *fingers)
{
	int i = 0;

	if (!g_ts_kit_log_cfg)
		return;

	for (; i < TS_MAX_FINGER; i++) {
		if (fingers[i].status)
			TS_LOG_INFO
			    ("%s:id=%d status:0x%x pressure=%d or=%d maj=%d min=%d wx=%d wy=%d ewx=%d ewy=%d xer=%d yer=%d\n",
			     __func__, i, fingers[i].status, fingers[i].pressure,
			     fingers[i].orientation, fingers[i].major,
			     fingers[i].minor, fingers[i].wx, fingers[i].wy,
			     fingers[i].ewx, fingers[i].ewy, fingers[i].xer,
			     fingers[i].yer);
	}
}

static void ts_finger_events_reformat(struct ts_fingers *fingers)
{
	int index;
	dump_fingers_info_debug((struct ts_finger *)fingers);
	is_all_finger_up = true;
	for (index = 0; index < TS_MAX_FINGER; index++) {
		if (fingers->cur_finger_number == 0 ||
		    fingers->fingers[index].status == TP_NONE_FINGER) {
			fingers->fingers[index].status |= TS_FINGER_RELEASE;
		} else {
			fingers->fingers[index].status |= TS_FINGER_PRESS;
			is_all_finger_up = false;
		}
	}
}

void ts_algo_calibrate(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int id;
	int algo_size = g_ts_kit_platform_data.chip_data->algo_size;
	u32 algo_order = in_cmd->cmd_param.pub_params.algo_param.algo_order;
	struct ts_fingers *in_finger = &in_cmd->cmd_param.pub_params.algo_param.info;
	struct ts_fingers *out_finger = &out_cmd->cmd_param.pub_params.algo_param.info;
	struct ts_algo_func *algo;

	if (!algo_size) {
		TS_LOG_INFO("no algo handler, direct report\n");
		goto out;
	}
	TS_LOG_DEBUG("%s: inalgo_order = %d\n", __func__, algo_order);
	/*
	 * If 1.aft enabled, hal algo works or 2.dont need algo t1 to filter
	 * glove events which algo_order configed 0, should skip driver algo
	 * and reformat events to add PRESS/RELEASE to finger->status
	 */

	if (g_ts_kit_platform_data.aft_param.aft_enable_flag || !algo_order) {
		ts_finger_events_reformat((struct ts_fingers *)(in_finger->fingers));
		goto out;
	}

	TS_LOG_DEBUG("algo algo_order: %d, algo_size :%d\n", algo_order, algo_size);

	for (id = 0; id < algo_size; id++) {
		if (algo_order & BIT_MASK(id)) {
			TS_LOG_DEBUG("algo id:%d is setted\n", id);
			list_for_each_entry(algo, &g_ts_kit_platform_data.chip_data->algo_head, node) {
				if (algo->algo_index == id)	//found the right algo func
				{
					TS_LOG_DEBUG("algo :%s called\n", algo->algo_name);
					algo->chip_algo_func(g_ts_kit_platform_data.chip_data, in_finger, out_finger);
					memcpy(in_finger, out_finger, sizeof(struct ts_fingers));
					break;
				}
			}
		}
	}
 out:
	memcpy(out_finger, in_finger, sizeof(struct ts_fingers));
	out_cmd->command = TS_REPORT_INPUT;
	TS_LOG_DEBUG("%s:skip aft\n", __func__);

	/*
	 * copy finger info to aft daemon
	 * if error hanppened, stil goto driver touch report process
	 * else do not report direct and set out_cmd as INVAILD
	 */
	if (!copy_fingers_to_aft_daemon(&g_ts_kit_platform_data, out_finger)) {
		out_cmd->command = TS_INVAILD_CMD;
	}

	return;
}

void ts_check_touch_window(struct ts_fingers *finger)
{
	int id = 0;
	int flag = 0;
	int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
	int window_enable = 0;

	if (!finger) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return;
	}
	window_enable = g_ts_kit_platform_data.feature_info.window_info.window_enable;
	x0 = g_ts_kit_platform_data.feature_info.window_info.top_left_x0;
	y0 = g_ts_kit_platform_data.feature_info.window_info.top_left_y0;
	x1 = g_ts_kit_platform_data.feature_info.window_info.bottom_right_x1;
	y1 = g_ts_kit_platform_data.feature_info.window_info.bottom_right_y1;

	if (0 == window_enable) {
		TS_LOG_DEBUG("no need to part report\n");
		return;
	}

	if (finger->fingers[0].status != TS_FINGER_RELEASE) {
		for (id = 0; id < TS_MAX_FINGER; id++) {
			if (finger->fingers[id].status != 0) {
				if ((finger->fingers[id].x >= x0)
				    && (finger->fingers[id].x <= x1)
				    && (finger->fingers[id].y >= y0)
				    && (finger->fingers[id].y <= y1)) {
					flag = 1;
				} else {
					finger->fingers[id].status = 0;
				}
			}
		}
		if (!flag) {
			finger->fingers[0].status = TS_FINGER_RELEASE;
		}
	}
}

void ts_film_touchplus(struct ts_fingers *finger, int finger_num,
		       struct input_dev *input_dev)
{
	static int pre_special_button_key = TS_TOUCHPLUS_INVALID;
	int key_max = TS_TOUCHPLUS_KEY2;
	int key_min = TS_TOUCHPLUS_KEY3;
	unsigned char ts_state = 0;

	TS_LOG_DEBUG("ts_film_touchplus called\n");

	/*discard touchplus report in gesture wakeup mode */
	ts_state = atomic_read(&g_ts_kit_platform_data.state);
	if ((TS_SLEEP == ts_state) || (TS_WORK_IN_SLEEP == ts_state)) {
		return;
	}

	/*touchplus(LingXiYiZhi) report ,  The difference between ABS_report and touchpls key_report
	 *when ABS_report is running, touchpls key will not report
	 *when touchpls key is not in range of touchpls keys, will not report key
	 */
	if ((finger_num != 0) || (finger->special_button_key > key_max)
	    || (finger->special_button_key < key_min)) {
		if (finger->special_button_flag != 0) {
			input_report_key(input_dev, finger->special_button_key,
					 0);
			input_sync(input_dev);
		}
		return;
	}

	/*touchplus(LingXiYiZhi) report ,  store touchpls key data(finger->special_button_key)
	 *when special_button_flag report touchpls key DOWN , store current touchpls key
	 *till the key report UP, then other keys will not report
	 */
	if (finger->special_button_flag == 1) {
		input_report_key(input_dev, finger->special_button_key,
				 finger->special_button_flag);
		input_sync(input_dev);
	} else if ((finger->special_button_flag == 0)
		   && (pre_special_button_key == finger->special_button_key)) {
		input_report_key(input_dev, finger->special_button_key,
				 finger->special_button_flag);
		input_sync(input_dev);
	} else if ((finger->special_button_flag == 0)
		   && (pre_special_button_key != finger->special_button_key)) {
		input_report_key(input_dev, pre_special_button_key, 0);
		input_sync(input_dev);
	}
	pre_special_button_key = finger->special_button_key;

	return;
}

static void ts_report_pen_event(struct input_dev *input, struct ts_tool tool,
				int pressure, int tool_type, int tool_value)
{
	input_report_abs(input, ABS_X, tool.x);
	input_report_abs(input, ABS_Y, tool.y);
	input_report_abs(input, ABS_PRESSURE, pressure);

	//check if the pen support tilt event
	if ((tool.tilt_x != 0) || (tool.tilt_y != 0)) {
		input_report_abs(input, ABS_TILT_X, tool.tilt_x);
		input_report_abs(input, ABS_TILT_Y, tool.tilt_y);
	}

	if ((tool.tool_type != WACOM_RUBBER_TO_PEN)
	    && (tool.tool_type != WACOM_PEN_TO_RUBBER)) {
		input_report_key(input, BTN_TOUCH, tool.tip_status);
	}
	input_report_key(input, tool_type, tool_value);
	input_sync(input);
}

void ts_report_pen(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	struct ts_pens *pens = NULL;
	struct input_dev *input = g_ts_kit_platform_data.pen_dev;
	struct anti_false_touch_param *local_param = NULL;
	int finger_num = 0;
	int id = 0;
	int key_value = 0;
	int ret = 0;

	if (!in_cmd || !out_cmd) {
		TS_LOG_ERR("parameters is null!\n", __func__);
		return;
	}
	trace_touch(TOUCH_TRACE_PEN_REPORT, TOUCH_TRACE_FUNC_IN, "report pen");
	pens = &in_cmd->cmd_param.pub_params.report_pen_info;
	if (!pens) {
		TS_LOG_ERR("pens is null!\n", __func__);
		return;
	}
	//report pen basic single button
	for (id = 0; id < TS_MAX_PEN_BUTTON; id++) {
		if (pens->buttons[id].status == 0) {
			continue;
		} else if (pens->buttons[id].status == TS_PEN_BUTTON_PRESS) {
			key_value = 1;
		} else {
			key_value = 0;
		}

		if (pens->buttons[id].key != 0) {
			TS_LOG_ERR("id is %d, key is %d, value is %d\n", id,
				   pens->buttons[id].key, key_value);
			input_report_key(input, pens->buttons[id].key,
					 key_value);
		}
	}

	/*
	 * report tool change   hover -> leave -> in
	 * when hover or leave, the pressure must be 0;
	 * when hover, tool value will report 1, means inrange;
	 * when leave, tool value will report 0, means outrange.
	 */
	if (pens->tool.tool_type == WACOM_RUBBER_TO_PEN) {
		//rubber hover
		ts_report_pen_event(input, pens->tool, 0, BTN_TOOL_RUBBER, 1);
		//rubber leave
		ts_report_pen_event(input, pens->tool, 0, BTN_TOOL_RUBBER, 0);
		//pen in
		pens->tool.tool_type = BTN_TOOL_PEN;
	} else if (pens->tool.tool_type == WACOM_PEN_TO_RUBBER) {
		//pen hover
		ts_report_pen_event(input, pens->tool, 0, BTN_TOOL_PEN, 1);
		//pen leave
		ts_report_pen_event(input, pens->tool, 0, BTN_TOOL_PEN, 0);
		//rubber in
		pens->tool.tool_type = BTN_TOOL_RUBBER;
	}
	//pen or rubber report point
	ts_report_pen_event(input, pens->tool, pens->tool.pressure,
			    pens->tool.tool_type,
			    pens->tool.pen_inrange_status);
#if defined (CONFIG_HUAWEI_DSM)
	if (g_ts_kit_platform_data.chip_data->ts_platform_data->feature_info.pen_info.supported_pen_alg)
	{
		ret = ts_pen_open_confirm(pens);
		if(0 != ret){
			ts_dmd_report(DSM_TP_IN_VNOISE_ERROR_NO, "DSM_TP_IN_VNOISE_ERROR_NO:report %d\n", TS_STYLUS_WAKEUP_SCREEN_ON);
			input_report_key(input, TS_STYLUS_WAKEUP_SCREEN_ON, 1);
			input_sync(input);
			input_report_key(input, TS_STYLUS_WAKEUP_SCREEN_ON, 0);
			input_sync(input);
			TS_LOG_INFO("%s: report TS_STYLUS_WAKEUP_SCREEN_ON .\n",__func__);
		}
	}
#endif
	trace_touch(TOUCH_TRACE_PEN_REPORT, TOUCH_TRACE_FUNC_OUT, "report pen");
}

int ts_count_fingers(struct ts_fingers *fingers)
{
	int count = 0;
	int id = 0;
	TS_LOG_DEBUG("%s:dump finger:\n",__func__);
	dump_fingers_info_debug((struct ts_finger *)fingers);
	for (; id < TS_MAX_FINGER; id++) {
		if (fingers->fingers[id].status & TS_FINGER_PRESS)
			count = id + 1;
	}

	return count;
}
void ts_palm_report(struct ts_cmd_node* in_cmd, struct ts_cmd_node* out_cmd){
	unsigned int key = 0;
	struct input_dev *input_dev = g_ts_kit_platform_data.input_dev;

	if(!input_dev || !in_cmd){
		TS_LOG_ERR("The command node or input device is not exist!\n");
		return;
	}
	key = in_cmd->cmd_param.pub_params.ts_key;
	TS_LOG_INFO("%s is called, palm_button_key is %d\n", __func__, key);
	input_report_key(input_dev, key, 1); /* 1 report key_event DOWN  */
	input_sync(input_dev);
	input_report_key(input_dev, key, 0); /* 0 report key_event UP */
	input_sync (input_dev);
	atomic_set(&g_ts_kit_data_report_over, 1);
}

void ts_report_input(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	struct ts_fingers *fingers = NULL;
	struct input_dev *input_dev = g_ts_kit_platform_data.input_dev;
	struct anti_false_touch_param *local_param = NULL;
	int finger_count = 0;
	int id = 0;

	trace_touch(TOUCH_TRACE_INPUT, TOUCH_TRACE_FUNC_IN, "without aft");
	if (!input_dev || !in_cmd) {
		TS_LOG_ERR("The command node or input device is not exist!\n");
		return;
	}
	fingers = &in_cmd->cmd_param.pub_params.algo_param.info;
	finger_count = ts_count_fingers(fingers);

	if (g_ts_kit_platform_data.chip_data) {
		local_param = &(g_ts_kit_platform_data.
		      chip_data->anti_false_touch_param_data);
	} else {
		local_param = NULL;
	}
	TS_LOG_DEBUG("ts_report_input\n");
	ts_check_touch_window(fingers);

	for (id = 0; id < finger_count; id++) {
		if (fingers->fingers[id].status & TS_FINGER_PRESS) {
			TS_LOG_DEBUG
			    ("down: id is %d, fingers->fingers[id].pressure = %d, \n",id, fingers->fingers[id].pressure );
			input_report_abs(input_dev, ABS_MT_PRESSURE,
					 fingers->fingers[id].pressure);
			input_report_abs(input_dev, ABS_MT_POSITION_X,
					 fingers->fingers[id].x);
			input_report_abs(input_dev, ABS_MT_POSITION_Y,
					 fingers->fingers[id].y);
			input_report_abs(input_dev, ABS_MT_TRACKING_ID, id);
			if (g_ts_kit_platform_data.fp_tp_enable) {
				input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR,
						fingers->fingers[id].major);
				input_report_abs(input_dev, ABS_MT_TOUCH_MINOR,
						fingers->fingers[id].minor);
			}
			input_mt_sync(input_dev);
		} else if (fingers->fingers[id].status & TS_FINGER_RELEASE) {
			TS_LOG_DEBUG("up: id is %d, status = %d\n", id,fingers->fingers[id].status);
			input_mt_sync(input_dev);
		}
	}

	input_report_key(input_dev, BTN_TOUCH, finger_count);
	input_sync(input_dev);

	ts_film_touchplus(fingers, finger_count, input_dev);
	if (((g_ts_kit_platform_data.chip_data->easy_wakeup_info.sleep_mode == TS_GESTURE_MODE)
		|| (g_ts_kit_platform_data.chip_data->easy_wakeup_info.palm_cover_flag == true))
		&& (g_ts_kit_platform_data.feature_info.holster_info.holster_switch == 0)) {
		input_report_key(input_dev, fingers->gesture_wakeup_value, 1);
		input_sync(input_dev);
		input_report_key(input_dev, fingers->gesture_wakeup_value, 0);
		input_sync(input_dev);
	}
	TS_LOG_DEBUG("ts_report_input done, finger_count = %d\n", finger_count);

	ts_work_after_input();	/* do some delayed works */

	atomic_set(&g_ts_kit_data_report_over, 1);
	trace_touch(TOUCH_TRACE_INPUT, TOUCH_TRACE_FUNC_OUT, "without aft");
	return;
}

static void send_up_msg_in_resume(void)
{
	struct input_dev *input_dev = g_ts_kit_platform_data.input_dev;

	input_report_key(input_dev, BTN_TOUCH, 0);
	input_mt_sync(input_dev);
	input_sync(input_dev);
	TS_LOG_DEBUG("send_up_msg_in_resume\n");
	return;
}

int ts_power_off_control_mode(int irq_id,
		     struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	enum lcd_kit_ts_pm_type pm_type = in_cmd->cmd_param.pub_params.pm_type;

	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	switch (pm_type) {
		case TS_BEFORE_SUSPEND:	/*do something before suspend */
			TS_LOG_DEBUG("[performance]%s:befor suspend in\n", __func__);
			atomic_set(&g_ts_kit_platform_data.power_state,
				   TS_GOTO_SLEEP);
			if (!(g_ts_kit_platform_data.chip_data->is_parade_solution == 1
			     && g_ts_kit_platform_data.chip_data->sleep_in_mode == 0)) {
				ts_stop_wd_timer(&g_ts_kit_platform_data);
			}
			if (dev->ops->chip_before_suspend) {
				error = dev->ops->chip_before_suspend();
			}
			TS_LOG_DEBUG("[performance]%s:befor suspend done\n", __func__);
			break;
		case TS_SUSPEND_DEVICE:	/*device power off or sleep */
			TS_LOG_DEBUG("[performance]%s:suspend in\n", __func__);
			atomic_set(&g_ts_kit_platform_data.state, TS_SLEEP);
			if (g_ts_kit_platform_data.aft_param.aft_enable_flag) {
				TS_LOG_INFO("ts_kit aft suspend\n");
			atomic_set(&g_ts_kit_platform_data.aft_in_slow_status, 0);
			atomic_set(&g_ts_kit_platform_data.last_input_fingers_status, 0);
			atomic_set(&g_ts_kit_platform_data.last_aft_filtered_fingers, 0);
				kobject_uevent(&g_ts_kit_platform_data.input_dev->dev.kobj, KOBJ_OFFLINE);
			}
			if (dev->ops->chip_suspend) {
				error = dev->ops->chip_suspend();
			}
			atomic_set(&g_ts_kit_platform_data.power_state,
				   TS_SLEEP);
			TS_LOG_DEBUG("[performance]%s:suspend done\n", __func__);
			break;
		case TS_IC_SHUT_DOWN:
			TS_LOG_DEBUG("[performance]%s:shutdown in\n", __func__);
			atomic_set(&g_ts_kit_platform_data.power_state,
				   TS_UNINIT);
			if (!(g_ts_kit_platform_data.chip_data->is_parade_solution == true))
				disable_irq(irq_id);
			if (dev->ops->chip_shutdown) {
				dev->ops->chip_shutdown();
			}
			TS_LOG_DEBUG("[performance]%s:shutdown done\n", __func__);
			break;
		case TS_RESUME_DEVICE:	/*device power on or wakeup */
			TS_LOG_DEBUG("[performance]%s:resume in\n", __func__);
			atomic_set(&g_ts_kit_platform_data.power_state,
				   TS_GOTO_WORK);
			if (dev->ops->chip_resume) {
				error = dev->ops->chip_resume();
				if (error) {
#if defined (CONFIG_HUAWEI_DSM)
					ts_dmd_report(DSM_TP_WAKEUP_ERROR_NO,
						      "try to client record 926004020 for resume error \n");
#endif
				}
			}
			TS_LOG_DEBUG("[performance]%s:resume done\n", __func__);
			break;
		case TS_AFTER_RESUME:	/*do something after resume */
			TS_LOG_DEBUG("[performance]%s:after resume in\n", __func__);
			if (dev->ops->chip_after_resume) {
				error = dev->ops->chip_after_resume((void *)
							    &g_ts_kit_platform_data.feature_info);
				if (error) {
#if defined (CONFIG_HUAWEI_DSM)
					ts_dmd_report(DSM_TP_WAKEUP_ERROR_NO,
						      "try to client record 926004020 for after resume error \n");
#endif
				}
			}
			TS_LOG_INFO("%s: chip_name is %s ,module_name is %s ,fw version is: %s .\n",
			     __func__,
			     g_ts_kit_platform_data.chip_data->chip_name,
			     g_ts_kit_platform_data.chip_data->module_name,
			     g_ts_kit_platform_data.chip_data->version_name);
			send_up_msg_in_resume();
			if (g_ts_kit_platform_data.aft_param.aft_enable_flag) {
				TS_LOG_INFO("ts_kit aft resume\n");
				kobject_uevent
				    (&g_ts_kit_platform_data.input_dev->dev.
				     kobj, KOBJ_ONLINE);
			}
			atomic_set(&g_ts_kit_platform_data.state, TS_WORK);
			if (!(g_ts_kit_platform_data.chip_data->is_parade_solution == 1
			     && g_ts_kit_platform_data.chip_data->sleep_in_mode == 0)) {
				enable_irq(irq_id);
				ts_start_wd_timer(&g_ts_kit_platform_data);
			}
			atomic_set(&g_ts_kit_platform_data.power_state,
				   TS_WORK);
			TS_LOG_DEBUG("[performance]%s:after resume done\n", __func__);
			break;
		default:
			TS_LOG_ERR("pm_type = %d\n", pm_type);
			error = -EINVAL;
			break;
	}
	return error;
}

int ts_power_off_no_config(int irq_id,
		     struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	enum lcd_kit_ts_pm_type pm_type = in_cmd->cmd_param.pub_params.pm_type;

	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	switch (pm_type) {
		case TS_BEFORE_SUSPEND:
			if (dev->ops->chip_before_suspend) {
				TS_LOG_ERR("TS_BEFORE_SUSPEND\n");
				error = dev->ops->chip_before_suspend();
			}
			break;
		case TS_SUSPEND_DEVICE:
			if (dev->ops->chip_suspend) {
				TS_LOG_ERR("TS_SUSPEND_DEVICE\n");
				error = dev->ops->chip_suspend();
			}
			break;
		case TS_RESUME_DEVICE:
			if (dev->ops->chip_resume) {
				TS_LOG_ERR("TS_RESUME_DEVICE\n");
				error = dev->ops->chip_resume();
			}
			break;
		case TS_AFTER_RESUME:
			if (dev->ops->chip_after_resume) {
				TS_LOG_ERR("TS_AFTER_RESUME\n");
				error = dev->ops->chip_after_resume((void *)&g_ts_kit_platform_data.feature_info);
			}
			break;
		default:
			TS_LOG_ERR("pm_type = %d\n", pm_type);
			error = -EINVAL;
			break;
	}
	return error;
}

int ts_power_off_easy_wakeup_mode(int irq_id,
		     struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	enum lcd_kit_ts_pm_type pm_type = in_cmd->cmd_param.pub_params.pm_type;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	switch (pm_type) {
		case TS_BEFORE_SUSPEND:	/*do something before suspend */
			if (!(g_ts_kit_platform_data.chip_data->is_parade_solution == true)) {
				ts_stop_wd_timer(&g_ts_kit_platform_data);
				disable_irq(irq_id);
			}
			if (dev->ops->chip_before_suspend) {
				error = dev->ops->chip_before_suspend();
			}
			break;
		case TS_SUSPEND_DEVICE:	/*switch to easy-wakeup mode, and enable interrupts */
			atomic_set(&g_ts_kit_platform_data.state, TS_WORK_IN_SLEEP);
			if (dev->ops->chip_suspend) {
				error = dev->ops->chip_suspend();
			}
			if (!(g_ts_kit_platform_data.chip_data->is_parade_solution == true))
				enable_irq(irq_id);
			if(!g_ts_kit_platform_data.chip_data->send_stylus_gesture_switch){
				out_cmd->command = TS_WAKEUP_GESTURE_ENABLE;
				out_cmd->cmd_param.prv_params =
				    (void *)&g_ts_kit_platform_data. feature_info.wakeup_gesture_enable_info;
			}
			break;
		case TS_IC_SHUT_DOWN:
			if (!(g_ts_kit_platform_data.chip_data->is_parade_solution == true))
				disable_irq(irq_id);
			if (dev->ops->chip_shutdown) {
				dev->ops->chip_shutdown();
			}
			break;
		case TS_RESUME_DEVICE:	/*exit easy-wakeup mode and restore sth */
			if (!(g_ts_kit_platform_data.chip_data->is_parade_solution == true))
				disable_irq(irq_id);
			if (dev->ops->chip_resume) {
				error = dev->ops->chip_resume();
			}
			break;
		case TS_AFTER_RESUME:	/*do nothing */
			if (dev->ops->chip_after_resume) {
				error = dev->ops->chip_after_resume((void *)&g_ts_kit_platform_data.feature_info);
			}
			TS_LOG_INFO("%s: chip_name is %s ,module_name is %s ,fw version is: %s .\n",
			     __func__,
			     g_ts_kit_platform_data.chip_data->chip_name,
			     g_ts_kit_platform_data.chip_data->module_name,
			     g_ts_kit_platform_data.chip_data->version_name);
			send_up_msg_in_resume();
			atomic_set(&g_ts_kit_platform_data.state, TS_WORK);
			if (!(g_ts_kit_platform_data.chip_data->is_parade_solution == true)) {
				enable_irq(irq_id);
				ts_start_wd_timer(&g_ts_kit_platform_data);
			}
			break;
		default:
			TS_LOG_ERR("pm_type = %d\n", pm_type);
			error = -EINVAL;
			break;
	}
	return error;
}

int ts_power_control(int irq_id,
		     struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;

	if (g_ts_kit_platform_data.chip_data->easy_wakeup_info.sleep_mode ==
	    TS_POWER_OFF_MODE) {
		if(!(g_ts_kit_platform_data.udfp_enable_flag&&g_ts_kit_platform_data.chip_data->suspend_no_config)){
			error = ts_power_off_control_mode(irq_id, in_cmd, out_cmd);
		}else{
			error = ts_power_off_no_config(irq_id, in_cmd, out_cmd);
		}
	} else if (g_ts_kit_platform_data.chip_data->easy_wakeup_info.sleep_mode == TS_GESTURE_MODE) {
			error = ts_power_off_easy_wakeup_mode(irq_id, in_cmd, out_cmd);
	} else {
		TS_LOG_ERR("no such mode!\n");
		error = -EINVAL;
	}
	return error;
}

int ts_touch_window(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	struct ts_window_info *info = NULL;
	if (!in_cmd) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	info = (struct ts_window_info *)in_cmd->cmd_param.prv_params;
	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	g_ts_kit_platform_data.feature_info.window_info.window_enable = info->window_enable;
	g_ts_kit_platform_data.feature_info.window_info.top_left_x0 = info->top_left_x0;
	g_ts_kit_platform_data.feature_info.window_info.top_left_y0 = info->top_left_y0;
	g_ts_kit_platform_data.feature_info.window_info.bottom_right_x1 = info->bottom_right_x1;
	g_ts_kit_platform_data.feature_info.window_info.bottom_right_y1 = info->bottom_right_y1;

	info->status = TS_ACTION_SUCCESS;

	return NO_ERR;
}

int ts_fw_update_boot(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	char *fw_name = NULL;
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	if (!in_cmd) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	fw_name = in_cmd->cmd_param.pub_params.firmware_info.fw_name;
	if (dev->ops->chip_fw_update_boot) {
		error = dev->ops->chip_fw_update_boot(fw_name);
		if (error) {
#if defined (CONFIG_HUAWEI_DSM)
			ts_dmd_report(DSM_TP_FWUPDATE_ERROR_NO,
				      "fw update result: failed.updata_status is:%d\n",
				      g_ts_kit_platform_data.
				      chip_data->ts_platform_data->
				      dsm_info.constraints_UPDATE_status);
#endif
		}
	}

	TS_LOG_INFO("process firmware update boot, return value:%d\n", error);
	return error;
}

int ts_fw_update_sd(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	TS_LOG_INFO("process firmware update sd\n");
	if (!g_ts_kit_platform_data.chip_data->is_parade_solution) {
		ts_stop_wd_timer(&g_ts_kit_platform_data);
	}
	if (dev->ops->chip_fw_update_sd) {
		error = dev->ops->chip_fw_update_sd();
	}
	if (!g_ts_kit_platform_data.chip_data->is_parade_solution) {
		ts_start_wd_timer(&g_ts_kit_platform_data);
	}

	return error;
}

void ts_start_wd_timer(struct ts_kit_platform_data *cd)
{
	if (!cd || !cd->chip_data) {
		TS_LOG_ERR("%s :the pointer is null\n", __func__);
		return;
	}

	if (!cd->chip_data->need_wd_check_status) {
		TS_LOG_DEBUG("%s :no need to check the status by watch dog\n",
			     __func__);
		return;
	}

	if (!TS_WATCHDOG_TIMEOUT) {
		return;
	}

	TS_LOG_DEBUG("start wd\n");
	if (cd->chip_data->check_status_watchdog_timeout) {
		mod_timer(&cd->watchdog_timer, jiffies +
			  msecs_to_jiffies(cd->chip_data->check_status_watchdog_timeout));
	} else {
		mod_timer(&cd->watchdog_timer, jiffies + msecs_to_jiffies(TS_WATCHDOG_TIMEOUT));
	}

	return;
}

EXPORT_SYMBOL(ts_start_wd_timer);

void ts_stop_wd_timer(struct ts_kit_platform_data *cd)
{

	if (!cd || !cd->chip_data) {
		TS_LOG_ERR("%s :the pointer is null\n", __func__);
		return;
	}

	if (!cd->chip_data->need_wd_check_status) {
		TS_LOG_DEBUG("%s :no need to check the status by watch dog\n",
			     __func__);
		return;
	}

	if (!TS_WATCHDOG_TIMEOUT) {
		return;
	}

	TS_LOG_DEBUG("stop wd\n");
	del_timer(&cd->watchdog_timer);
	cancel_work_sync(&cd->watchdog_work);
	del_timer(&cd->watchdog_timer);
	return;
}

EXPORT_SYMBOL(ts_stop_wd_timer);

bool ts_cmd_need_process(struct ts_cmd_node * cmd)
{
	bool is_need_process = true;
	struct ts_cmd_sync *sync = cmd->sync;
	enum lcd_kit_ts_pm_type pm_type = cmd->cmd_param.pub_params.pm_type;

	if (unlikely((atomic_read(&g_ts_kit_platform_data.state) == TS_SLEEP)
		     || (atomic_read(&g_ts_kit_platform_data.state) ==
			 TS_WORK_IN_SLEEP))) {
		if (atomic_read(&g_ts_kit_platform_data.state) == TS_SLEEP) {
			switch (cmd->command) {
			case TS_POWER_CONTROL:
				if ((pm_type != TS_RESUME_DEVICE)
				    && (pm_type != TS_AFTER_RESUME)) {
					is_need_process = false;
				}
				break;
			case TS_TOUCH_WINDOW:
				is_need_process = true;
				break;
			case TS_INT_PROCESS:
			case TS_INT_ERR_OCCUR:
				//enable_irq(g_ts_data.irq_id);
				if (g_ts_kit_platform_data.
				    chip_data->is_parade_solution) {
					if (g_ts_kit_platform_data.chip_data->isbootupdate_finish == false || g_ts_kit_platform_data.chip_data->sleep_in_mode == 0)	//deep sleep mode need process irq when suspend/resume
						is_need_process = true;
					else
						is_need_process = false;
				} else {
					is_need_process = false;
				}
				break;
			case TS_GET_CHIP_INFO:
				is_need_process = true;
				break;
			default:
				is_need_process = false;
				break;
			}
		} else {
			switch (cmd->command) {
			case TS_POWER_CONTROL:
				if ((pm_type != TS_RESUME_DEVICE)
				    && (pm_type != TS_AFTER_RESUME)) {
					is_need_process = false;
				}
				break;
			case TS_TOUCH_WINDOW:
				is_need_process = true;
				break;
			case TS_OEM_INFO_SWITCH:
				is_need_process = true;
				break;
			case TS_GET_CHIP_INFO:
				is_need_process = true;
				break;
			case TS_TOUCH_SWITCH:
				is_need_process = false;
				break;
			case TS_READ_RAW_DATA:
				is_need_process = false;
				break;
			default:
				is_need_process = true;
				break;
			}
		}
	}

	if (!is_need_process && sync) {
		if (atomic_read(&sync->timeout_flag) == TS_TIMEOUT) {
			kfree(sync);
		} else {
			complete(&sync->done);
		}
	}

	return is_need_process;
}

int ts_kit_power_notify_callback(struct notifier_block *self,
				 unsigned long notify_pm_type, void *data)
{
	int *data_in = data;
	unsigned int timeout;

	if (!data) {
		TS_LOG_ERR("%s: data in null ptr\n", __func__);
		return 0;
	}

	timeout = *data_in;
	TS_LOG_INFO("%s called,pm_type=%d, timeout=%d\n", __func__,
		    notify_pm_type, timeout);
	return ts_kit_power_control_notify(notify_pm_type, timeout);
}

int ts_kit_power_control_notify(enum lcd_kit_ts_pm_type pm_type, int timeout)
{
	int error = 0;
	struct ts_cmd_node cmd;
	TS_LOG_INFO("ts_kit_power_control_notify called,pm_type is %d",
		    pm_type);
	if (TS_UNINIT == atomic_read(&g_ts_kit_platform_data.state)) {
		TS_LOG_INFO("ts is not init");
		return -EINVAL;
	}

#if defined (CONFIG_TEE_TUI)
	if (g_ts_kit_platform_data.chip_data->report_tui_enable
	    && (TS_BEFORE_SUSPEND == pm_type)) {
		g_ts_kit_platform_data.chip_data->tui_set_flag |= 0x1;
		TS_LOG_INFO("TUI is working, later do before suspend\n");
		return NO_ERR;
	}

	if (g_ts_kit_platform_data.chip_data->report_tui_enable
	    && (TS_SUSPEND_DEVICE == pm_type)) {
		g_ts_kit_platform_data.chip_data->tui_set_flag |= 0x2;
		TS_LOG_INFO("TUI is working, later do suspend\n");
		return NO_ERR;
	}
#endif

    if ((g_ts_kit_platform_data.chip_data->easy_wakeup_info.sleep_mode == TS_POWER_OFF_MODE)
		&& (TS_BEFORE_SUSPEND == pm_type)) {
                if(!(g_ts_kit_platform_data.chip_data->is_parade_solution == 1
			&& g_ts_kit_platform_data.chip_data->sleep_in_mode == 0)) {
				disable_irq(g_ts_kit_platform_data.irq_id);
				TS_LOG_INFO("%s :device will go to sleep ,disable irq first !\n", __func__);
                }
    }
	cmd.command = TS_POWER_CONTROL;
	cmd.cmd_param.pub_params.pm_type = pm_type;
	error = ts_kit_put_one_cmd(&cmd, timeout);
	if (error) {
		TS_LOG_ERR("ts_kit_power_control_notify, put cmd error :%d\n", error);
		error = -EBUSY;
	}
	if (TS_AFTER_RESUME == pm_type) {
		if(g_ts_kit_platform_data.chip_data->ts_platform_data->feature_info.pen_info.supported_pen_alg) {
			ts_pen_open_confirm_init();
		}
		TS_LOG_INFO("ts_resume_send_roi_cmd\n");
		if ((g_ts_kit_platform_data.chip_data->is_direct_proc_cmd == 0)&&(g_ts_kit_platform_data.chip_data->suspend_no_config == 0))
			ts_send_roi_cmd(TS_ACTION_WRITE, NO_SYNC_TIMEOUT);	/*force to write the roi function */
		if (error) {
			TS_LOG_ERR("ts_resume_send_roi_cmd failed\n");
		}
	}
	return error;
}

int ts_read_debug_data(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd,
		       struct ts_cmd_sync *sync)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_diff_data_info *info = NULL;

	TS_LOG_INFO("read diff data called\n");
	if (!in_cmd || !out_cmd) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	info = (struct ts_diff_data_info *)in_cmd->cmd_param.prv_params;
	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	if (dev->ops->chip_get_debug_data) {
		error = dev->ops->chip_get_debug_data(info, out_cmd);
	}

	if ((sync != NULL) && (atomic_read(&sync->timeout_flag) == TS_TIMEOUT)) {
		TS_LOG_ERR("ts_read_debug_data_timeout\n");
		if (info) {
			kfree(info);
		}
		return error;
	}

	if (!error) {
		TS_LOG_INFO("read diff data success\n");
		info->status = TS_ACTION_SUCCESS;
		info->time_stamp = ktime_get();
		goto out;
	}

	info->status = TS_ACTION_FAILED;
	TS_LOG_INFO("read diff data failed :%d\n", error);

 out:
	return error;
}

int ts_read_rawdata_for_newformat(struct ts_cmd_node *in_cmd,
				  struct ts_cmd_node *out_cmd,
				  struct ts_cmd_sync *sync)
{
	struct ts_rawdata_info_new *info = NULL;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	int error = NO_ERR;

	TS_LOG_INFO("ts read rawdata for new format called\n");
	if (!in_cmd || !out_cmd) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}

	info = (struct ts_rawdata_info_new *)in_cmd->cmd_param.prv_params;
	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	if (!g_ts_kit_platform_data.chip_data->is_parade_solution) {
		ts_stop_wd_timer(&g_ts_kit_platform_data);
	}
	if (dev->ops->chip_get_rawdata) {
		error = dev->ops->chip_get_rawdata((struct ts_rawdata_info *)info, out_cmd);
	}
	if (!g_ts_kit_platform_data.chip_data->is_parade_solution) {
		ts_start_wd_timer(&g_ts_kit_platform_data);
	}
	if (!error) {
		TS_LOG_INFO("read rawdata success\n");
		info->status = TS_ACTION_SUCCESS;
		info->time_stamp = ktime_get();
		goto out;
	}
	info->status = TS_ACTION_FAILED;
	TS_LOG_ERR("read rawdata failed :%d\n", error);
 out:
	return error;
}

int ts_read_rawdata(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd,
		    struct ts_cmd_sync *sync)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_rawdata_info *info = NULL;

	/**********************************************/
	/* Rawdata rectification, if dts configured   */
	/* with a new mark, take a new process        */
	/**********************************************/
	if (g_ts_kit_platform_data.chip_data->rawdata_newformatflag ==
	    TS_RAWDATA_NEWFORMAT) {
		return ts_read_rawdata_for_newformat(in_cmd, out_cmd, sync);
	}

	TS_LOG_INFO("ts read rawdata called\n");
	if (!in_cmd || !out_cmd) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	info = (struct ts_rawdata_info *)in_cmd->cmd_param.prv_params;
	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	if (!g_ts_kit_platform_data.chip_data->is_parade_solution) {
		ts_stop_wd_timer(&g_ts_kit_platform_data);
	}
	if (dev->ops->chip_get_rawdata) {
		error = dev->ops->chip_get_rawdata(info, out_cmd);
	}
	if (!g_ts_kit_platform_data.chip_data->is_parade_solution) {
		ts_start_wd_timer(&g_ts_kit_platform_data);
	}

	if ((sync != NULL) && (atomic_read(&sync->timeout_flag) == TS_TIMEOUT)) {
		TS_LOG_ERR("ts_read_rawdata timeout!\n");
		if (info) {
			if (g_ts_kit_platform_data.chip_data->trx_delta_test_support) {
				if (info->rx_delta_buf) {
					kfree(info->rx_delta_buf);
					info->rx_delta_buf = NULL;
				}
				if (info->tx_delta_buf) {
					kfree(info->tx_delta_buf);
					info->tx_delta_buf = NULL;
				}
			}
			if (g_ts_kit_platform_data.chip_data->td43xx_ee_short_test_support) {
				if (info->td43xx_rt95_part_one) {
					kfree(info->td43xx_rt95_part_one);
					info->td43xx_rt95_part_one = NULL;
				}
				if (info->td43xx_rt95_part_two) {
					kfree(info->td43xx_rt95_part_two);
					info->td43xx_rt95_part_two = NULL;
				}
			}
			vfree(info);
			info = NULL;
		}
		return error;
	}

	if (!error) {
		TS_LOG_INFO("read rawdata success\n");
		info->status = TS_ACTION_SUCCESS;
		info->time_stamp = ktime_get();
		goto out;
	}

	info->status = TS_ACTION_FAILED;
	TS_LOG_ERR("read rawdata failed :%d\n", error);

 out:
	return error;
}

int ts_read_calibration_data(struct ts_cmd_node *in_cmd,
			     struct ts_cmd_node *out_cmd,
			     struct ts_cmd_sync *sync)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_calibration_data_info *info =
	    (struct ts_calibration_data_info *)in_cmd->cmd_param.prv_params;

	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}

	TS_LOG_INFO("%s called\n", __FUNCTION__);

	if (dev->ops->chip_get_calibration_data)
		error = dev->ops->chip_get_calibration_data(info, out_cmd);

	if ((sync != NULL) && (atomic_read(&sync->timeout_flag) == TS_TIMEOUT)) {
		TS_LOG_ERR("ts_read_calibration_data_timeout\n");
		if (info) {
			kfree(info);
		}
		return error;
	}

	if (!error) {
		TS_LOG_INFO("read calibration data success\n");
		info->status = TS_ACTION_SUCCESS;
		info->time_stamp = ktime_get();
		goto out;
	}

	info->status = TS_ACTION_FAILED;
	TS_LOG_ERR("read calibration data failed :%d\n", error);

 out:
	return error;
}

int ts_get_calibration_info(struct ts_cmd_node *in_cmd,
			    struct ts_cmd_node *out_cmd,
			    struct ts_cmd_sync *sync)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_calibration_info_param *info =
	    (struct ts_calibration_info_param *)in_cmd->cmd_param.prv_params;

	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	TS_LOG_INFO("%s called\n", __FUNCTION__);

	if (dev->ops->chip_get_calibration_info)
		error = dev->ops->chip_get_calibration_info(info, out_cmd);

	if ((sync != NULL) && (atomic_read(&sync->timeout_flag) == TS_TIMEOUT)) {
		TS_LOG_ERR("ts_get_calibration_info_timeout\n");
		if (info) {
			kfree(info);
		}
		return error;
	}

	if (error)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;

	return error;
}

int ts_oem_info_switch(struct ts_cmd_node *in_cmd,
		       struct ts_cmd_node *out_cmd, struct ts_cmd_sync *sync)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_oem_info_param *info = (struct ts_oem_info_param *)in_cmd->cmd_param.prv_params;

	TS_LOG_INFO("ts chip data switch called\n");
	if (!info) {
		TS_LOG_ERR("%s, info is NULL, exit\n", __func__);
		return RESULT_ERR;
	}

	if (dev->ops->oem_info_switch)
		error = dev->ops->oem_info_switch(info);

	if ((sync != NULL) && (atomic_read(&sync->timeout_flag) == TS_TIMEOUT)) {
		TS_LOG_ERR("ts_oem_info_switch_timeout\n");
		return error;
	}

	if (error)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;

	return error;
}

int ts_get_chip_info(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_chip_info_param *info = (struct ts_chip_info_param *)in_cmd->cmd_param.prv_params;

	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	TS_LOG_INFO("get chip info called\n");
	if (!g_ts_kit_platform_data.chip_data->is_parade_solution) {
		ts_stop_wd_timer(&g_ts_kit_platform_data);
	}
	if (dev->ops->chip_get_info) {
		error = dev->ops->chip_get_info(info);
	}
	if (!g_ts_kit_platform_data.chip_data->is_parade_solution) {
		ts_start_wd_timer(&g_ts_kit_platform_data);
	}

	if (error) {
		info->status = TS_ACTION_FAILED;
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	return error;
}

int ts_set_info_flag(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_kit_platform_data *info = NULL;

	TS_LOG_INFO("ts_set_info_flag called\n");
	if (!in_cmd) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	info = (struct ts_kit_platform_data *)in_cmd->cmd_param.prv_params;
	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	if (dev->ops->chip_set_info_flag) {
		error = dev->ops->chip_set_info_flag(info);
	}
	return error;
}

int ts_calibrate(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd,
		 struct ts_cmd_sync *sync)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_calibrate_info *info =
	    (struct ts_calibrate_info *)in_cmd->cmd_param.prv_params;

	TS_LOG_DEBUG("process firmware calibrate\n");
	if (!info) {
		TS_LOG_ERR("%s, info is NULL, exit\n", __func__);
		return RESULT_ERR;
	}

	if (dev->ops->chip_calibrate) {
		error = dev->ops->chip_calibrate();
	}

	if ((sync != NULL) && (atomic_read(&sync->timeout_flag) == TS_TIMEOUT)) {
		TS_LOG_ERR("ts_calibrate_timeout\n");
		return error;
	}

	if (error) {
		info->status = TS_ACTION_FAILED;
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	return error;
}

int ts_calibrate_wakeup_gesture(struct ts_cmd_node *in_cmd,
				struct ts_cmd_node *out_cmd,
				struct ts_cmd_sync *sync)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_calibrate_info *info = (struct ts_calibrate_info *)in_cmd->cmd_param.prv_params;

	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	TS_LOG_DEBUG("process firmware calibrate\n");

	if (dev->ops->chip_calibrate_wakeup_gesture) {
		error = dev->ops->chip_calibrate_wakeup_gesture();
	}

	if ((sync != NULL) && (atomic_read(&sync->timeout_flag) == TS_TIMEOUT)) {
		TS_LOG_ERR("ts_calibrate_wakeup_gesture_timeout\n");
		return error;
	}

	if (error) {
		info->status = TS_ACTION_FAILED;
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	return error;
}

int ts_dsm_debug(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_dsm_debug_info *info = (struct ts_dsm_debug_info *)in_cmd->cmd_param.prv_params;

	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	TS_LOG_INFO("ts dsm debug is called\n");

	if (dev->ops->chip_dsm_debug) {
		error = dev->ops->chip_dsm_debug();
	}

	if (error) {
		info->status = TS_ACTION_FAILED;
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	return error;
}

int ts_glove_switch(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_glove_info *info = NULL;

	if (!in_cmd) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	info = (struct ts_glove_info *)in_cmd->cmd_param.prv_params;
	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	TS_LOG_DEBUG("glove action :%d, value:%d", info->op_action,
		     info->glove_switch);

	if (dev->ops->chip_glove_switch) {
		error = dev->ops->chip_glove_switch(info);
	}

	if (error) {
		info->status = TS_ACTION_FAILED;
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	TS_LOG_DEBUG("glove switch process result: %d\n", error);

	return error;
}

int ts_get_capacitance_test_type(struct ts_cmd_node *in_cmd,
				 struct ts_cmd_node *out_cmd,
				 struct ts_cmd_sync *sync)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_test_type_info *info = NULL;

	TS_LOG_INFO("get_mmi_test_mode called\n");
	if (!in_cmd) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	info = (struct ts_test_type_info *)in_cmd->cmd_param.prv_params;
	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	if (dev->ops->chip_get_capacitance_test_type) {
		error = dev->ops->chip_get_capacitance_test_type(info);
	}

	if ((sync != NULL) && (atomic_read(&sync->timeout_flag) == TS_TIMEOUT)) {
		TS_LOG_ERR("ts_get_capacitance_test_type_timeout\n");
		if (info) {
			kfree(info);
		}
		return error;
	}

	if (error) {
		info->status = TS_ACTION_FAILED;
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	return error;
}

int ts_chip_detect(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;

	TS_LOG_INFO("ts_chip_detect called\n");
	if (!in_cmd) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	if (atomic_read(&g_ts_kit_platform_data.register_flag) ==
	    TS_REGISTER_DONE) {
		TS_LOG_INFO("%s : ts register done. ignore\n", __func__);
		return error;
	}
	g_ts_kit_platform_data.chip_data =
	    in_cmd->cmd_param.pub_params.chip_data;
	if (g_ts_kit_platform_data.chip_data->ops->chip_detect) {
		error = g_ts_kit_platform_data.chip_data->ops->chip_detect(&g_ts_kit_platform_data);
	}
	if (!error) {
		atomic_set(&g_ts_kit_platform_data.register_flag, TS_REGISTER_DONE);
		atomic_set(&g_ts_kit_platform_data.state, TS_WORK);
		wake_up_process(g_ts_kit_platform_data.ts_init_task);
#if defined(CONFIG_HUAWEI_DEV_SELFCHECK) || defined(CONFIG_HUAWEI_HW_DEV_DCT)
		/* detect current device successful, set the flag as present */
		set_tp_dev_flag();
#endif
	} else {
		g_ts_kit_platform_data.chip_data = NULL;
		//atomic_set(&g_ts_kit_platform_data.state, TS_UNINIT);
	}
	return error;
}

int ts_palm_switch(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd,
		   struct ts_cmd_sync *sync)
{
	int error = -EIO;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_palm_info *info = (struct ts_palm_info *)in_cmd->cmd_param.prv_params;

	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}

	TS_LOG_DEBUG("palm action :%d, value:%d", info->op_action, info->palm_switch);

	if (dev->ops->chip_palm_switch) {
		error = dev->ops->chip_palm_switch(info);
	}

	if ((sync != NULL) && (atomic_read(&sync->timeout_flag) == TS_TIMEOUT)) {
		TS_LOG_ERR("ts_palm_switch_timeout\n");
		if (info) {
			kfree(info);
		}
		return error;
	}

	if (error) {
		info->status = TS_ACTION_FAILED;
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	TS_LOG_DEBUG("palm switch process result: %d\n", error);

	return error;
}

int ts_hand_detect(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_hand_info *info =
	    (struct ts_hand_info *)in_cmd->cmd_param.prv_params;

	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}

	if (dev->ops->chip_hand_detect) {
		error = dev->ops->chip_hand_detect(info);
	}

	if (error) {
		info->status = TS_ACTION_FAILED;
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	return error;
}

int ts_force_reset(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	TS_LOG_INFO("ts force reset called\n");

	if (dev->ops->chip_reset) {
		error = dev->ops->chip_reset();
	}

	if (error) {
		out_cmd->command = TS_ERR_OCCUR;
		goto out;
	}

 out:
	return error;
}

int ts_int_err_process(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	if (dev->ops->chip_reset) {
		error = dev->ops->chip_reset();
	}

	if (error)		//error nest occurred, we define nest level
	{
		out_cmd->command = TS_ERR_OCCUR;
		goto out;
	}

 out:
	return error;
}

int ts_err_process(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	static int error_count = 0;
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	TS_LOG_INFO("error process\n");

	if (dev->ops->chip_reset) {
		error = dev->ops->chip_reset();
	}
	if (error)		//error nest occurred, we define nest level
	{
		out_cmd->command = TS_ERR_OCCUR;
		goto out;
	}

	error_count = 0;
 out:
	return error;
}

int ts_check_status(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	if (!out_cmd) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	if (dev->ops->chip_check_status) {
		error = dev->ops->chip_check_status();
	}
	if (error) {
		out_cmd->command = TS_ERR_OCCUR;
	}

	ts_start_wd_timer(&g_ts_kit_platform_data);
	return error;
}

int ts_wakeup_gesture_enable_switch(struct ts_cmd_node *in_cmd,
				    struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_wakeup_gesture_enable_info *info =
	    (struct ts_wakeup_gesture_enable_info *)in_cmd->cmd_param.prv_params;

	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}

	TS_LOG_INFO("%s: write value: %d", __func__, info->switch_value);
	if(dev->send_stylus_gesture_switch) {
		if (dev->ops->chip_wakeup_gesture_enable_switch) {
			error = dev->ops->chip_wakeup_gesture_enable_switch(info);
		}
	} else {
		if (atomic_read(&g_ts_kit_platform_data.state) == TS_WORK_IN_SLEEP
		    && dev->ops->chip_wakeup_gesture_enable_switch) {
			error = dev->ops->chip_wakeup_gesture_enable_switch(info);
		}
	}

	info->op_action = TS_ACTION_UNDEF;
	if (error) {
		info->status = TS_ACTION_FAILED;
		TS_LOG_ERR("%s, process error: %d\n", __func__, error);
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	TS_LOG_DEBUG("%s, process result: %d\n", __func__, error);

	return error;
}

int ts_holster_switch(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_holster_info *info = NULL;

	if (!in_cmd) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	info = (struct ts_holster_info *)in_cmd->cmd_param.prv_params;
	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	TS_LOG_DEBUG("Holster action :%d, value:%d", info->op_action,
		     info->holster_switch);

	if (dev->ops->chip_holster_switch) {
		error = dev->ops->chip_holster_switch(info);
	}

	if (error) {
		info->status = TS_ACTION_FAILED;
		TS_LOG_ERR("holster switch process error: %d\n", error);
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	TS_LOG_DEBUG("holster switch process result: %d\n", error);

	return error;
}

int ts_roi_switch(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_roi_info *info = NULL;

	if (!in_cmd) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	info = (struct ts_roi_info *)in_cmd->cmd_param.prv_params;
	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	if (dev->ops->chip_roi_switch) {
		error = dev->ops->chip_roi_switch(info);
	}

	if (error) {
		info->status = TS_ACTION_FAILED;
		TS_LOG_ERR("roi switch process error: %d\n", error);
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	TS_LOG_INFO("roi action :%d, value:%d, process result: %d\n",
		    info->op_action, info->roi_switch, error);

	return error;
}

int ts_chip_regs_operate(struct ts_cmd_node *in_cmd,
			 struct ts_cmd_node *out_cmd, struct ts_cmd_sync *sync)
{
	int error = -EIO;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_regs_info *info = NULL;

	if (!in_cmd) {
		TS_LOG_ERR("%s : find a null pointer\n", __func__);
		return -EINVAL;
	}
	info = (struct ts_regs_info *)in_cmd->cmd_param.prv_params;
	if (!info) {
		TS_LOG_ERR("%s, find a null pointer\n", __func__);
		return -EINVAL;
	}
	if (dev->ops->chip_regs_operate) {
		error = dev->ops->chip_regs_operate(info);
	}

	if ((sync != NULL) && (atomic_read(&sync->timeout_flag) == TS_TIMEOUT)) {
		TS_LOG_ERR("ts_chip_regs_operate_timeout\n");
		if (info) {
			kfree(info);
		}
		return error;
	}

	if (error < 0) {
		info->status = TS_ACTION_FAILED;
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	return error;
}

int ts_test_cmd(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	if (dev->ops->chip_test) {
		error = dev->ops->chip_test(in_cmd, out_cmd);
	}

	if (error) {
		out_cmd->command = TS_ERR_OCCUR;
		goto out;
	}

 out:
	return error;
}

int ts_send_holster_cmd(void)
{
	int error = NO_ERR;
	struct ts_cmd_node cmd;

	TS_LOG_DEBUG("set holster\n");
	memset(&cmd, 0, sizeof(struct ts_cmd_node));
	cmd.command = TS_HOLSTER_SWITCH;
	cmd.cmd_param.prv_params =
	    (void *)&g_ts_kit_platform_data.feature_info.holster_info;
	if (g_ts_kit_platform_data.chip_data->is_direct_proc_cmd) {
		error = ts_kit_proc_command_directly(&cmd);
	} else {
		error = ts_kit_put_one_cmd(&cmd, SHORT_SYNC_TIMEOUT);
	}
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}
	if (g_ts_kit_platform_data.feature_info.holster_info.status !=
	    TS_ACTION_SUCCESS) {
		TS_LOG_ERR("action failed\n");
		error = -EIO;
		goto out;
	}

 out:
	return error;
}

void ts_kit_charger_switch(struct ts_cmd_node *in_cmd,
			   struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	struct ts_charger_info *info =
	    (struct ts_charger_info *)in_cmd->cmd_param.prv_params;

	if (!info) {
		TS_LOG_ERR("%s, info is NULL, exit\n", __func__);
		return;
	}

	TS_LOG_DEBUG("%s, action :%d, value:%d\n", __func__, info->op_action,
		     info->charger_switch);

	if (dev->ops->chip_charger_switch)
		error = dev->ops->chip_charger_switch(info);

	if (error) {
		info->status = TS_ACTION_FAILED;
		TS_LOG_ERR("%s process result: %d\n", __func__, error);
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	TS_LOG_DEBUG("%s process result: %d\n", __func__, error);

	return;
}


int ts_oemdata_type_check_legal(u8 type, u8 len)
{
	int ret = 0;

	switch (type) {
	case TS_NV_STRUCTURE_PROID:
		ret = 1;
		break;
	case TS_NV_STRUCTURE_BAR_CODE:
		if (TS_NV_BAR_CODE_LEN == len)
			ret = 1;
		break;
	case TS_NV_STRUCTURE_BRIGHTNESS:
		if (TS_NV_BRIGHTNESS_LEN == len)
			ret = 1;
		break;
	case TS_NV_STRUCTURE_WHITE_POINT:
		if (TS_NV_WHITE_POINT_LEN == len)
			ret = 1;
		break;
	case TS_NV_STRUCTURE_BRI_WHITE:
		if (TS_NV_BRI_WHITE_LEN == len)
			ret = 1;
		break;
	case TS_NV_STRUCTURE_REPAIR:
		if (TS_NV_REPAIR_LEN == len)
			ret = 1;
		break;
	case TS_NV_STRUCTURE_RESERVED:
	default:
		break;
	}
	TS_LOG_INFO("%s: type:%u, len:%u, ret:%d\n", __func__, type, len, ret);
	return ret;
}

EXPORT_SYMBOL(ts_oemdata_type_check_legal);
/*lint -restore*/

