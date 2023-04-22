#include "ilitek_common.h"
#include "ilitek_dts.h"
#include "ilitek_protocol.h"
#include "ilitek_config.h"
#include "ilitek_report.h"
#if defined(CONFIG_HUAWEI_DEVKIT_QCOM)
#include <linux/i2c/i2c-msm-v2.h>
#endif

static struct ilitek_report *g_ilitek_rpt = NULL;

static int ilitek_report_touch_data(struct ts_fingers *p_info)
{
    int i = 0;

    memset(p_info, 0x00, sizeof(struct ts_fingers));

    for (i = 0; i < ILITEK_TOUCSCEEN_FINGERS; i++) {
        if (g_ilitek_rpt->fingers[i].id == ILITEK_TOUCSCEEN_FINGERS) {
            continue;
        }

        p_info->fingers[i].x = g_ilitek_rpt->fingers[i].x;
        p_info->fingers[i].y = g_ilitek_rpt->fingers[i].y;

        p_info->fingers[i].major = 1;
        p_info->fingers[i].minor = 1;
        p_info->fingers[i].pressure = g_ilitek_rpt->fingers[i].pressure;
        p_info->fingers[i].status = TP_FINGER;

        ilitek_debug(DEBUG_FINGER_REPORT, "finger down, id = %d, pos[%d, %d]\n",
            i,
            g_ilitek_rpt->fingers[i].x,
            g_ilitek_rpt->fingers[i].y);
    }

    p_info->cur_finger_number = g_ilitek_rpt->cur_finger_num;

    return 0;
}

static int ilitek_report_release_data(struct ts_fingers *p_info)
{
    ilitek_debug(DEBUG_FINGER_REPORT, "finger released\n");

    p_info->cur_finger_number = 0;

    return 0;
}

static int ilitek_read_roi_data(void)
{
    int i = 0;
    u16 raw_data = 0;
    int ret = 0;
    struct ts_kit_device_data *ts_dev_data = g_ilitek_ts->ts_dev_data;

    ilitek_debug(DEBUG_ROI, "roi_switch = %d, roi_support =%d\n",
        ts_dev_data->ts_platform_data->feature_info.roi_info.roi_switch,
        ts_dev_data->ts_platform_data->feature_info.roi_info.roi_supported);

    ilitek_debug(DEBUG_ROI, "last_fingers = %d, cur_fingers =%d\n",
        g_ilitek_rpt->last_finger_num,
        g_ilitek_rpt->cur_finger_num);

    if(ts_dev_data->ts_platform_data->feature_info.roi_info.roi_switch
        && ts_dev_data->ts_platform_data->feature_info.roi_info.roi_supported){
        if (g_ilitek_rpt->last_finger_num != g_ilitek_rpt->cur_finger_num &&
            g_ilitek_rpt->cur_finger_num <= ILITEK_ROI_FINGERS ) {
            ret = ilitek_config_roi_ctrl(CMD_ROI_DATA);
            if (ret){
                ilitek_err("i2c write data failed, ret = %d\n", ret);
                return ret;
            }

            ret = ilitek_i2c_read(g_ilitek_rpt->roi_data, ROI_DATA_READ_LENGTH);
            if (ret){
                ilitek_err("i2c read data failed, ret = %d\n", ret);
                return ret;
            }

            ilitek_debug(DEBUG_ROI, "index = %d, fixed = %d, peak_row = %d, peak_colum = %d\n",
                g_ilitek_rpt->roi_data[0],
                g_ilitek_rpt->roi_data[1],
                g_ilitek_rpt->roi_data[2],
                g_ilitek_rpt->roi_data[3]);
            if (DEBUG_ROI & ilitek_debug_level) {
                printk("=================== roi data in bytes ===================\n");
                for (i = 4; i < ROI_DATA_READ_LENGTH; i++) {
                    printk(KERN_CONT "%3d ", g_ilitek_rpt->roi_data[i]);
                    if ((i - 3) % 14 == 0) { /* 14 * 14  rawdata bytes */
                        printk("\n");
                    }
                }
            }
        }
    }

    return 0;
}

static int ilitek_report_gesture(struct ts_fingers *p_info)
{
    u32 reprot_gesture_key_value = TS_GESTURE_INVALID;
    struct ts_kit_device_data *ts_dev_data = g_ilitek_ts->ts_dev_data;
    struct ts_easy_wakeup_info *gesture_report_info = &ts_dev_data->easy_wakeup_info;

    if (IS_ERR_OR_NULL(p_info) || IS_ERR_OR_NULL(gesture_report_info)){
        ilitek_err("ts_fingers or gesture_report_info is null\n");
        return -EINVAL;
    }

    /*if the easy_wakeup_flag is false,status not in gesture;switch_value is false,gesture is no supported*/
    if ((false == ts_dev_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value) ||
        (false == gesture_report_info->easy_wakeup_flag)){
        ilitek_debug(DEBUG_GESTURE, "gesture switch all closed\n");
        return 0;
    }

    ilitek_info("ilitek gesture code = 0x%2x\n", g_ilitek_rpt->package_data[1]);

    switch (g_ilitek_rpt->package_data[1]) {
    case ILITEK_GESTURE_DOUBLECLICK:
        if (IS_APP_ENABLE_GESTURE(GESTURE_DOUBLE_CLICK) &
            gesture_report_info->easy_wakeup_gesture) {
            reprot_gesture_key_value = TS_DOUBLE_CLICK;
            ilitek_info("@@@TS_DOUBLE_CLICK detected!@@@\n");
        }
        break;
    case ILITEK_GESTURE_UP:
        if (IS_APP_ENABLE_GESTURE(GESTURE_SLIDE_B2T) &
            gesture_report_info->easy_wakeup_gesture) {
            reprot_gesture_key_value = TS_SLIDE_B2T;
            ilitek_info("@@@TS_SLIDE_B2T detected!@@@\n");
        }
        break;
    case ILITEK_GESTURE_DOWN:
        if (IS_APP_ENABLE_GESTURE(GESTURE_SLIDE_T2B) &
            gesture_report_info->easy_wakeup_gesture) {
            reprot_gesture_key_value = TS_SLIDE_T2B;
            ilitek_info("@@@TS_SLIDE_T2B detected!@@@\n");
        }
        break;
    case ILITEK_GESTURE_LEFT:
        if (IS_APP_ENABLE_GESTURE(GESTURE_SLIDE_L2R) &
            gesture_report_info->easy_wakeup_gesture) {
            reprot_gesture_key_value = TS_SLIDE_L2R;
            ilitek_info("@@@TS_SLIDE_L2R detected!@@@\n");
        }
        break;
    case ILITEK_GESTURE_RIGHT:
        if (IS_APP_ENABLE_GESTURE(GESTURE_SLIDE_R2L) &
            gesture_report_info->easy_wakeup_gesture) {
            reprot_gesture_key_value = TS_SLIDE_R2L;
            ilitek_info("@@@TS_SLIDE_R2L detected!@@@\n");
        }
        break;
    case ILITEK_GESTURE_M:
        if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_m) &
            gesture_report_info->easy_wakeup_gesture) {
            reprot_gesture_key_value = TS_LETTER_m;
            ilitek_info("@@@TS_LETTER_m detected!@@@\n");
        }
        break;
    case ILITEK_GESTURE_W:
        if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_w) &
            gesture_report_info->easy_wakeup_gesture) {
            reprot_gesture_key_value = TS_LETTER_w;
            ilitek_info("@@@TS_LETTER_w detected!@@@\n");
        }
        break;
    case ILITEK_GESTURE_C:
        if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_c) &
            gesture_report_info->easy_wakeup_gesture) {
            reprot_gesture_key_value = TS_LETTER_c;
            ilitek_info("@@@TS_LETTER_c detected!@@@\n");
        }
        break;
    case ILITEK_GESTURE_E:
        if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_e) &
            gesture_report_info->easy_wakeup_gesture) {
            reprot_gesture_key_value = TS_LETTER_e;
            ilitek_info("@@@TS_LETTER_e detected!@@@\n");
        }
        break;
    case ILITEK_GESTURE_O:
        if (IS_APP_ENABLE_GESTURE(GESTURE_CIRCLE_SLIDE) &
            gesture_report_info->easy_wakeup_gesture) {
            reprot_gesture_key_value = TS_CIRCLE_SLIDE;
            ilitek_info("@@@TS_CIRCLE_SLIDE detected!@@@\n");
        }
        break;
    case ILITEK_GESTURE_V:
    case ILITEK_GESTURE_S:
    case ILITEK_GESTURE_Z:
        reprot_gesture_key_value = TS_GESTURE_INVALID;
        ilitek_info("ts kit don't support gesture type\n");
        break;
    default:
        reprot_gesture_key_value = TS_GESTURE_INVALID;
        ilitek_err("invalid gesture type\n");
        break;
    }

    ilitek_info("ts kit key value = %d\n", reprot_gesture_key_value);

    if (reprot_gesture_key_value != TS_GESTURE_INVALID) {
        /*increase wake_lock time to avoid system suspend.*/
        wake_lock_timeout(&ts_dev_data->ts_platform_data->ts_wake_lock, 5 * HZ);

        mutex_lock(&g_ilitek_ts->wrong_touch_lock);
        if (true == ts_dev_data->easy_wakeup_info.off_motion_on) {
            ts_dev_data->easy_wakeup_info.off_motion_on = false;
            p_info->gesture_wakeup_value = reprot_gesture_key_value;
            ilitek_info("ready to report event = %d\n", reprot_gesture_key_value);
        }
        mutex_unlock(&g_ilitek_ts->wrong_touch_lock);
    } else {
        ilitek_err("reprot_gesture_key_value = TS_GESTURE_INVALID\n");
        return -EINVAL;
    }

    return 0;
}

/**
 *  Receive data when fw mode stays at i2cuart mode.
 *
 *  the first is to receive N bytes depending on the mode that firmware stays
 *  before going in this function, and it would check with i2c buffer if it
 *  remains the rest of data.
 */
static int i2cuart_recv_packet(void)
{
    int ret = 0;
    int type = 0;
    int need_read_len = 0, one_data_bytes = 0, actual_len = 0;

    if (!g_ilitek_ts->isEnableNetlink && !g_ilitek_ts->debug_node_open) {
        ilitek_debug(DEBUG_FINGER_REPORT, "debug node don't open, no need to recv\n");
        return 0;
    }

    g_ilitek_ts->i2cuart_len = 0;

    type = g_ilitek_rpt->package_data[3] & 0x0F;
    actual_len = g_ilitek_rpt->package_len - 5;

    ilitek_debug(DEBUG_FINGER_REPORT, "pid = %x, data[3] = %x, type = %x, actual_len = %d\n",
        g_ilitek_rpt->package_data[0],
        g_ilitek_rpt->package_data[3],
        type,
        actual_len);

    need_read_len = g_ilitek_rpt->package_data[1] * g_ilitek_rpt->package_data[2];

    if (type == 0 || type == 1 || type == 6) {
        one_data_bytes = 1;
    } else if (type == 2 || type == 3) {
        one_data_bytes = 2;
    } else if (type == 4 || type == 5) {
        one_data_bytes = 4;
    }

    ilitek_debug(DEBUG_FINGER_REPORT, "need_read_len = %d  one_data_bytes = %d\n",
        need_read_len,
        one_data_bytes);

    need_read_len = need_read_len * one_data_bytes + 1;

    if (need_read_len <= actual_len) {
        ilitek_debug(DEBUG_FINGER_REPORT, "no i2c uart data to read\n");
        return -EINVAL;
    }

    g_ilitek_ts->i2cuart_data = kzalloc(need_read_len - actual_len, GFP_KERNEL);
    if (IS_ERR_OR_NULL(g_ilitek_ts->i2cuart_data)) {
        ilitek_err("alloc i2cuart_buf failed\n");
        return -ENOMEM;
    }

    ret = ilitek_i2c_read(g_ilitek_ts->i2cuart_data, need_read_len - actual_len);
    if (ret) {
        ilitek_err("i2c read data failed, ret = %d\n", ret);
        g_ilitek_ts->i2cuart_len = 0;
        return -EIO;
    }

    g_ilitek_ts->i2cuart_len = need_read_len - actual_len;

    return 0;
}

static int ilitek_report_debug_data(void)
{
    int ret = 0;
    u8 *debug_data = NULL;

    if (!g_ilitek_ts->isEnableNetlink && !g_ilitek_ts->debug_node_open) {
        ilitek_debug(DEBUG_FINGER_REPORT, "debug node don't open, no need to report\n");
        ret = 0;
        goto free_all_data;
    }

    g_ilitek_ts->debug_len = 0;
    g_ilitek_ts->debug_len += g_ilitek_rpt->package_len;
    g_ilitek_ts->debug_len += g_ilitek_ts->i2cuart_len;

    /* 2048 is referred to the defination by user */
    ilitek_debug(DEBUG_FINGER_REPORT, "package_len = %d, i2cuart_len = %d, debug_len = %d, frame_len = %d\n",
        g_ilitek_rpt->package_len,
        g_ilitek_ts->i2cuart_len,
        g_ilitek_ts->debug_len,
        DEBUG_DATA_LEN);

    if (g_ilitek_ts->debug_len > DEBUG_DATA_LEN) {
        ilitek_err("total length (%d) is too long than user can handle\n",
            g_ilitek_ts->debug_len);
        ret = -EINVAL;
        goto free_all_data;
    }

    g_ilitek_ts->debug_data = kzalloc(g_ilitek_ts->debug_len, GFP_KERNEL);
    if (IS_ERR_OR_NULL(g_ilitek_ts->debug_data)) {
        ilitek_err("alloc debug_data failed\n");
        ret = -ENOMEM;
        goto free_all_data;
    }

    memcpy(g_ilitek_ts->debug_data, g_ilitek_rpt->package_data, g_ilitek_rpt->package_len);

    /* merge uart data if it's at i2cuart mode */
    if (g_ilitek_ts->i2cuart_len) {
        memcpy(g_ilitek_ts->debug_data + g_ilitek_rpt->package_len, g_ilitek_ts->i2cuart_data,
            g_ilitek_ts->i2cuart_len);
    }

    if (g_ilitek_ts->isEnableNetlink) {
        netlink_reply_msg(g_ilitek_ts->debug_data, g_ilitek_ts->debug_len);
    }

    if (g_ilitek_ts->debug_node_open) {
        mutex_lock(&g_ilitek_ts->ilitek_debug_mutex);
        memset(g_ilitek_ts->debug_buf[g_ilitek_ts->debug_data_frame], 0x00,  DEBUG_DATA_LEN);
        memcpy(g_ilitek_ts->debug_buf[g_ilitek_ts->debug_data_frame], g_ilitek_ts->debug_data, g_ilitek_ts->debug_len);
        g_ilitek_ts->debug_data_frame++;
        ilitek_debug(DEBUG_FINGER_REPORT, "debug_data_frame = %d\n", g_ilitek_ts->debug_data_frame);
        if (g_ilitek_ts->debug_data_frame >= DEBUG_DATA_NUM) {
            ilitek_err("debug_data_frame = %d > %d\n", g_ilitek_ts->debug_data_frame, DEBUG_DATA_NUM);
            g_ilitek_ts->debug_data_frame = DEBUG_DATA_NUM - 1;
        }
        mutex_unlock(&g_ilitek_ts->ilitek_debug_mutex);
        wake_up(&g_ilitek_ts->inq);
    }

free_all_data:
    if (g_ilitek_ts->i2cuart_data) {
        kfree(g_ilitek_ts->i2cuart_data);
        g_ilitek_ts->i2cuart_data = NULL;
        g_ilitek_ts->i2cuart_len = 0;
    }
    if (g_ilitek_ts->debug_data) {
        kfree(g_ilitek_ts->debug_data);
        g_ilitek_ts->debug_data = NULL;
        g_ilitek_ts->debug_len = 0;
    }
    return ret;
}

int ilitek_report_data(struct ts_fingers *p_info)
{
    int ret = 0;
    int i2c_retries = I2C_RW_TRIES;
    struct i2c_adapter* adapter = NULL;
#if defined(CONFIG_HUAWEI_DEVKIT_QCOM)
    struct i2c_msm_ctrl *ctrl = NULL;
#endif
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;
    struct ts_kit_device_data *ts_dev_data = g_ilitek_ts->ts_dev_data;
    struct ts_kit_platform_data *ts_platform_data = ts_dev_data->ts_platform_data;
    struct ts_easy_wakeup_info *gesture_report_info = &ts_dev_data->easy_wakeup_info;

    if (!g_ilitek_ts->isEnableFR) {
        return -EINVAL;
    }

    adapter = i2c_get_adapter(ts_platform_data->bops->bus_id);
    if (IS_ERR_OR_NULL(adapter)) {
        ilitek_err("i2c_get_adapter failed\n");
        return -EIO;
    }

#if defined(CONFIG_HUAWEI_DEVKIT_QCOM)
    ctrl = (struct i2c_msm_ctrl *)adapter->dev.driver_data;

    /*if the easy_wakeup_flag is false,status not in gesture;switch_value is false,gesture is no supported*/
    if ((true == ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value) &&
        (true == gesture_report_info->easy_wakeup_flag)){
        do {
            if (ctrl->pwr_state == I2C_MSM_PM_SYS_SUSPENDED) {
                ilitek_info("gesture mode, waiting for i2c bus resume\n");
                msleep(I2C_WAIT_TIME);
            } else { /*I2C_MSM_PM_RT_SUSPENDED or I2C_MSM_PM_RT_ACTIVE*/
                ilitek_info("i2c bus resuming or resumed\n");
                break;
            }
        } while (i2c_retries--);

        if (ctrl->pwr_state == I2C_MSM_PM_SYS_SUSPENDED) {
            ilitek_info("trigger gesture irq in system suspending,i2c bus can't resume, so ignore irq\n");
            return -EINVAL;
        }
    }
#endif

    g_ilitek_ts->debug_len = 0;
    g_ilitek_ts->i2cuart_len = 0;
    g_ilitek_rpt->package_len = 0;
    g_ilitek_rpt->cur_finger_num = 0;

    g_ilitek_rpt->package_len = ilitek_config_calc_package_length(g_ilitek_rpt->fw_mode);
    if (g_ilitek_rpt->package_len == 0) {
        ilitek_err("calc packget length failed\n");
        return -EINVAL;
    }

    g_ilitek_rpt->package_data = kzalloc(g_ilitek_rpt->package_len, GFP_KERNEL);
    if (IS_ERR_OR_NULL(g_ilitek_rpt->package_data)) {
        ilitek_err("alloc report data failed\n");
        return -ENOMEM;
    }

    memset(g_ilitek_rpt->package_data, 0xFF, g_ilitek_rpt->package_len);

    ret = ilitek_i2c_read(g_ilitek_rpt->package_data, g_ilitek_rpt->package_len);
    if (ret) {
        ilitek_err("i2c read data failed, ret = %d\n", ret);
        ret = -EIO;
        goto err_free_data;
    }

    g_ilitek_rpt->package_id = g_ilitek_rpt->package_data[0];
    ilitek_debug(DEBUG_FINGER_REPORT, "package_id = 0x%x\n", g_ilitek_rpt->package_id);

    /* i2cuart recv package */
    /* i2cuart package don't return NO_ERR to ts_kit
     * or ts_kit will report no finger touch event because the cur_finger_num is 0.
     */
    if (g_ilitek_rpt->package_id == p_pro->modes[ILITEK_I2C_UART_MODE].package_id) {
        ilitek_debug(DEBUG_FINGER_REPORT, "i2cuart(0x%x): prepare to receive rest of data\n",
            g_ilitek_rpt->package_id);
        i2cuart_recv_packet();
        ret = -EINVAL;
        goto report_debug_data;
    }

    /* gesture report */
    if (g_ilitek_rpt->package_id == p_pro->modes[ILITEK_GESTURE_MODE].package_id &&
        g_ilitek_ts->support_gesture) {
        ilitek_debug(DEBUG_GESTURE, "pid = 0x%x, code = 0x%x\n",
            g_ilitek_rpt->package_id,
            g_ilitek_rpt->package_data[1]);
        ilitek_report_gesture(p_info);
        ret = 0;
        goto err_free_data;
    }

    ret = ilitek_config_parse_package(g_ilitek_rpt->package_id);
    if (ret) {
        ilitek_err("parse report data failed\n");
        ret = -EINVAL;
        goto err_free_data;
    }

    ilitek_debug(DEBUG_FINGER_REPORT, "current finger num = %d, last_finger_num = %d\n",
        g_ilitek_rpt->cur_finger_num,
        g_ilitek_rpt->last_finger_num);

#ifdef ROI
    ilitek_read_roi_data();
#endif

    if (g_ilitek_rpt->cur_finger_num > 0) {
        ilitek_report_touch_data(p_info);
    } else {
        if (g_ilitek_rpt->last_finger_num > 0) {
            ilitek_report_release_data(p_info);
        }
    }

    g_ilitek_rpt->last_finger_num = g_ilitek_rpt->cur_finger_num;

report_debug_data:
   ilitek_report_debug_data();
err_free_data:
    if(g_ilitek_rpt->package_data) {
        kfree(g_ilitek_rpt->package_data);
        g_ilitek_rpt->package_data = NULL;
        g_ilitek_rpt->package_len = 0;
    }

    return ret;
}

int ilitek_report_init(void)
{
    g_ilitek_rpt = kzalloc(sizeof(*g_ilitek_rpt), GFP_KERNEL);
    if (IS_ERR_OR_NULL(g_ilitek_rpt)) {
        ilitek_err("alloc ilitek report failed\n");
        return -ENOMEM;
    }

    g_ilitek_rpt->fw_mode = ILITEK_DEMO_MODE;
    g_ilitek_rpt->package_id = 0;
    g_ilitek_rpt->package_data = NULL;
    g_ilitek_rpt->package_len = 0;
    g_ilitek_rpt->cur_finger_num = 0;
    g_ilitek_rpt->last_finger_num = 0;

    g_ilitek_ts->rpt = g_ilitek_rpt;

    ilitek_info("report init succeeded\n");

    return 0;
}

void ilitek_report_exit(void)
{
    if (g_ilitek_rpt->package_data) {
        kfree(g_ilitek_rpt->package_data);
        g_ilitek_rpt->package_data = NULL;
    }

    if (g_ilitek_rpt) {
        kfree(g_ilitek_rpt);
        g_ilitek_rpt = NULL;
    }

    g_ilitek_ts->rpt = NULL;

    ilitek_info("free report success\n");
}
