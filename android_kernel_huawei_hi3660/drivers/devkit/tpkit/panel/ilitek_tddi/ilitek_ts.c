#include "ilitek_common.h"
#include "ilitek_dts.h"
#include "ilitek_protocol.h"
#include "ilitek_config.h"
#include "ilitek_report.h"
#include "ilitek_test.h"
#include "ilitek_upgrade.h"

/* Debug level */
u32 ilitek_debug_level = DEBUG_NONE;

struct ilitek_ts_data *g_ilitek_ts = NULL;

static int ilitek_pinctrl_init(void)
{
    int ret = 0;

    g_ilitek_ts->pctrl = devm_pinctrl_get(&g_ilitek_ts->pdev->dev);
    if (IS_ERR_OR_NULL(g_ilitek_ts->pctrl)) {
        ilitek_err("get pinctrl failed\n");
        return -EINVAL;
    }

    g_ilitek_ts->pins_active = pinctrl_lookup_state(g_ilitek_ts->pctrl,
        ILITEK_DTS_PIN_ACTIVE);
    if (IS_ERR_OR_NULL(g_ilitek_ts->pins_active)) {
        ilitek_err("lookup active state failed\n");
        ret = -EINVAL;
        goto err_pinctrl_put;
    }

    g_ilitek_ts->pins_idle = pinctrl_lookup_state(g_ilitek_ts->pctrl,
        ILITEK_DTS_PIN_IDLE);
    if (IS_ERR_OR_NULL(g_ilitek_ts->pins_idle)) {
        ilitek_err("lookup idle state failed\n");
        ret = -EINVAL;
        goto err_pinctrl_put;
    }

    g_ilitek_ts->pins_default = pinctrl_lookup_state(g_ilitek_ts->pctrl,
        ILITEK_DTS_PIN_DEFAULT);
    if (IS_ERR_OR_NULL(g_ilitek_ts->pins_default)) {
        ilitek_err("lookup default state failed\n");
        ret = -EINVAL;
        goto err_pinctrl_put;
    }

    ilitek_info("pinctrl init succeeded\n");

    return 0;

err_pinctrl_put:
    if (g_ilitek_ts->pctrl) {
        devm_pinctrl_put(g_ilitek_ts->pctrl);
        g_ilitek_ts->pctrl = NULL;
    }
    return ret;
}

static int ilitek_pinctrl_release(void)
{
    if (g_ilitek_ts->pctrl) {
        devm_pinctrl_put(g_ilitek_ts->pctrl);
        g_ilitek_ts->pctrl = NULL;
        g_ilitek_ts->pins_default = NULL;
        g_ilitek_ts->pins_active = NULL;
        g_ilitek_ts->pins_idle = NULL;
    }

    ilitek_info("pinctrl release succeeded\n");

    return 0;
}

static int ilitek_pinctrl_select_default(void)
{
    int ret = 0;

    if (IS_ERR_OR_NULL(g_ilitek_ts->pctrl) ||
        IS_ERR_OR_NULL(g_ilitek_ts->pins_default)) {
        ilitek_err("pctrl or pins_default is NULL\n");
        return -EINVAL;
    }

    ret = pinctrl_select_state(g_ilitek_ts->pctrl, g_ilitek_ts->pins_default);
    if (ret) {
        ilitek_err("pinctrl select default failed\n");
        return -EINVAL;
    }

    ilitek_info("pinctrl select default succeeded\n");

    return 0;
}

static int ilitek_pinctrl_select_active(void)
{
    int ret = 0;

    if (IS_ERR_OR_NULL(g_ilitek_ts->pctrl) ||
        IS_ERR_OR_NULL(g_ilitek_ts->pins_active)) {
        ilitek_err("pctrl or pins_active is NULL\n");
        return -EINVAL;
    }

    ret = pinctrl_select_state(g_ilitek_ts->pctrl, g_ilitek_ts->pins_active);
    if (ret) {
        ilitek_err("pinctrl select active failed\n");
        return -EINVAL;
    }

    ilitek_info("pinctrl select active succeeded\n");

    return 0;
}

static int ilitek_pinctrl_select_idle(void)
{
    int ret = 0;

    if (IS_ERR_OR_NULL(g_ilitek_ts->pctrl) ||
        IS_ERR_OR_NULL(g_ilitek_ts->pins_idle)) {
        ilitek_err("pctrl or pins_idle is NULL\n");
        return -EINVAL;
    }

    ret = pinctrl_select_state(g_ilitek_ts->pctrl, g_ilitek_ts->pins_idle);
    if (ret) {
        ilitek_err("pinctrl select idle failed\n");
        return -EINVAL;
    }

    ilitek_info("pinctrl select idle succeeded\n");

    return 0;
}

int ilitek_chip_reset(void)
{
    int reset_gpio = g_ilitek_ts->ts_dev_data->ts_platform_data->reset_gpio;

    gpio_direction_output(reset_gpio, 1);
    mdelay(g_ilitek_ts->cfg->chip_info->delay_time_high);

    gpio_set_value(reset_gpio, 0);
    mdelay(g_ilitek_ts->cfg->chip_info->delay_time_low);

    gpio_set_value(reset_gpio, 1);
    mdelay(g_ilitek_ts->cfg->chip_info->delay_time_edge);

    ilitek_info("chip hw reset succeeded\n");

    return 0;
}

static int ilitek_i2c_communicate_check(void)
{
    int i = 0;
    int ret = 0;
    u8 data = 0;
    struct ts_kit_platform_data *p_data = NULL;

    /* set i2c-client slave addr */
    p_data = g_ilitek_ts->ts_dev_data->ts_platform_data;
    p_data->client->addr = g_ilitek_ts->ts_dev_data->slave_addr;

    for (i = 0; i < ILITEK_I2C_RETRYS; i++) {
        ret = ilitek_i2c_read(&data, sizeof(data));
        if (ret) {
            ilitek_err("i2c read failed\n");
            msleep(50);
        } else {
            ilitek_info("chip i2c check succeeded\n");
            return 0;
        }
    }

    return ret;
}

static int ilitek_chip_detect(struct ts_kit_platform_data *p_data)
{
    int ret = 0;

    if (IS_ERR_OR_NULL(p_data) ||
        IS_ERR_OR_NULL(p_data->ts_dev)) {
        ilitek_err("platform_data or ts_dev invaild\n");
        ret = -EINVAL;
        goto exit;
    }

    g_ilitek_ts->ts_dev_data->ts_platform_data = p_data;
    g_ilitek_ts->pdev = p_data->ts_dev;
    g_ilitek_ts->pdev->dev.of_node = g_ilitek_ts->ts_dev_data->cnode;

    g_ilitek_ts->dev = &(p_data->client->dev);
    g_ilitek_ts->ts_dev_data->is_i2c_one_byte = true;
    g_ilitek_ts->ts_dev_data->is_new_oem_structure = false;
    g_ilitek_ts->ts_dev_data->is_parade_solution = false;

    ret = ilitek_prase_ic_config_dts(g_ilitek_ts->pdev->dev.of_node);
    if (ret) {
        ilitek_err("parse ic config in dts failed\n");
        goto err_free;
    }

    ret = ilitek_pinctrl_init();
    if (ret) {
        ilitek_err("pinctrl init failed\n");
        goto err_free;
    }

    ret = ilitek_pinctrl_select_active();
    if (ret) {
        ilitek_err("set pincntrl active failed\n");
         goto err_pinctrl_put;
    }

    ret = ilitek_protocol_init(ILITEK_VER_5_4_0);
    if (ret) {
        ilitek_err("protocol init failed\n");
         goto err_pinctrl_put;
    }

    ret = ilitek_config_init(g_ilitek_ts->chip_id);
    if (ret) {
        ilitek_err("config init failed\n");
         goto err_protocol_free;
    }

    ret = ilitek_chip_reset();
    if (ret) {
        ilitek_err("chip hw reset failed\n");
         goto err_protocol_free;
    }

    ret = ilitek_i2c_communicate_check();
    if (ret) {
        ilitek_err("find ilitek device failed\n");
        goto err_config_free;
    } else {
        strncpy(g_ilitek_ts->ts_dev_data->chip_name, ILITEK_CHIP_NAME, MAX_STR_LEN);
    }

    ilitek_info("driver version : %s\n", DRIVER_VERSION);
    ilitek_info("chip detect succeeded\n");

    return 0;

err_config_free:
    ilitek_config_exit();
err_protocol_free:
    ilitek_protocol_exit();
err_pinctrl_put:
    ilitek_pinctrl_release();
err_free:
    if(g_ilitek_ts->ts_dev_data) {
        kfree(g_ilitek_ts->ts_dev_data);
        g_ilitek_ts->ts_dev_data = NULL;
    }
    if (g_ilitek_ts) {
        kfree(g_ilitek_ts);
        g_ilitek_ts = NULL;
    }
exit:
    ilitek_err("chip detect failed\n");
    return ret;
}

static int ilitek_chip_init(void)
{
    int ret = 0;
    pro_ic_infos index = 0;

    g_ilitek_ts->ts_dev_data->is_direct_proc_cmd = false;
    g_ilitek_ts->ts_dev_data->easy_wakeup_info.sleep_mode = TS_POWER_OFF_MODE;
    g_ilitek_ts->ts_dev_data->easy_wakeup_info.easy_wakeup_gesture = false;
    g_ilitek_ts->ts_dev_data->easy_wakeup_info.easy_wakeup_flag = false;
    g_ilitek_ts->ts_dev_data->easy_wakeup_info.palm_cover_flag = false;
    g_ilitek_ts->ts_dev_data->easy_wakeup_info.palm_cover_control = false;
    g_ilitek_ts->ts_dev_data->easy_wakeup_info.off_motion_on = false;
    g_ilitek_ts->ts_dev_data->ts_platform_data->feature_info.holster_info.holster_switch = false;

    /* ts_kit wait for rawdata test, 6s timeout */
    g_ilitek_ts->ts_dev_data->rawdata_get_timeout = 6;

    /* record ini file open status */
    g_ilitek_ts->open_threshold_status = false;

    mutex_init(&g_ilitek_ts->wrong_touch_lock);

    /* init members for debug */
    g_ilitek_ts->isEnableFR = true;
    g_ilitek_ts->isEnableNetlink = false;
    g_ilitek_ts->debug_data_frame = 0;
    g_ilitek_ts->debug_node_open = false;
    mutex_init(&g_ilitek_ts->ilitek_report_irq);
    mutex_init(&g_ilitek_ts->ilitek_debug_mutex);
    mutex_init(&g_ilitek_ts->ilitek_debug_read_mutex);
    init_waitqueue_head(&(g_ilitek_ts->inq));

    ret = ilitek_parse_dts(g_ilitek_ts->pdev->dev.of_node);
    if (ret) {
        ilitek_err("parse config in dts failed\n");
    }

    ret = ilitek_config_get_project_id();
    if (ret) {
        ilitek_err("read project id failed\n");
    } else {
        ret = ilitek_parse_ic_special_dts(g_ilitek_ts->pdev->dev.of_node);
        if (ret) {
            ilitek_err("parse ic special config in dts failed\n");
        }
    }

    ret = ilitek_config_get_chip_id(g_ilitek_ts->chip_id);
    if (ret) {
        ilitek_err("match chip id failed\n");
        goto err_out;
    }

    ret = ilitek_report_init();
    if (ret) {
        ilitek_err("finger report init failed\n");
        goto err_out;
    }

    ret = ilitek_upgrade_init();
    if (ret) {
        ilitek_err("upgrade init failed\n");
        goto err_out;
    }

    for (index = 0; index < ILITEK_INFO_NUMS; index++) {
        ret = ilitek_config_read_ic_info(index);
        if (ret) {
            ilitek_err("read %s failed\n", g_ilitek_ts->pro->infos[index].name);
        }
    }

    /*
     * to make sure our ic running well before the work,
     * pulling reset pin as low/high once after read ic info.
     */
    ilitek_chip_reset();

    ret = ilitek_proc_init();
    if (ret) {
        ilitek_info("debug node create failed\n");
    }

    ilitek_info("chip init succeeded\n");

    return 0;

//don't free data,firmware upgrade will use this parameter
err_out:
    ilitek_info("chip init failed\n");
    return ret;
}

static int ilitek_get_brightness_info(void)
{
    return 0;
}

static int ilitek_input_config(struct input_dev *input_dev)
{
    u16 x_min = 0, x_max = 0, y_min = 0, y_max = 0, finger_nums = 0;

    g_ilitek_ts->input = input_dev;

    /* set the supported event type for input device */
    set_bit(EV_ABS, input_dev->evbit);
    set_bit(EV_SYN, input_dev->evbit);
    set_bit(EV_KEY, input_dev->evbit);
    set_bit(BTN_TOUCH, input_dev->keybit);
    set_bit(BTN_TOOL_FINGER, input_dev->keybit);
    set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

    /* now only need double click, others need to add*/
    if (g_ilitek_ts->support_gesture) {
        set_bit(TS_DOUBLE_CLICK, input_dev->keybit);
    }

    /* x[0~2047] y[0~2047] */
    if (g_ilitek_ts->use_ic_res) {
        x_min = g_ilitek_ts->cfg->tp_info.nMinX;
        y_min = g_ilitek_ts->cfg->tp_info.nMinY;
        x_max = g_ilitek_ts->cfg->tp_info.nMaxX;
        y_max = g_ilitek_ts->cfg->tp_info.nMaxY;
        finger_nums = g_ilitek_ts->cfg->tp_info.nMaxTouchNum;
    } else {
        x_min = g_ilitek_ts->x_min;
        y_min = g_ilitek_ts->y_min;
        x_max = g_ilitek_ts->x_max - 1;
        y_max = g_ilitek_ts->y_max - 1;
        finger_nums = g_ilitek_ts->finger_nums;
    }

    ilitek_info("input resolution : x = [%d,%d], y = [%d,%d]\n",
        x_min,
        x_max,
        y_min,
        y_max);
    ilitek_info("input touch number : %d\n", finger_nums);

    input_set_abs_params(input_dev, ABS_MT_POSITION_X, x_min, x_max, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_POSITION_Y, y_min, y_max, 0, 0);

    input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);

    input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);

    input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, finger_nums, 0, 0);

    return 0;
}

static int ilitek_irq_top_half(struct ts_cmd_node *cmd)
{
    cmd->command = TS_INT_PROCESS;
    return 0;
}

int ilitek_irq_bottom_half(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
    int ret = 0;
    struct ts_fingers *info = NULL;

    info = &out_cmd->cmd_param.pub_params.algo_param.info;
    out_cmd->command = TS_INPUT_ALGO;
    out_cmd->cmd_param.pub_params.algo_param.algo_order =
            g_ilitek_ts->ts_dev_data->algo_id;

    ret = ilitek_report_data(info);
    if (ret) {
        out_cmd->command = TS_INVAILD_CMD;
    }

    return ret;
}

static int ilitek_chip_get_info(struct ts_chip_info_param *info)
{
    if (IS_ERR_OR_NULL(info)) {
        ilitek_err("info invaild\n");
        return -EINVAL;
    }

    /* [chip name][ic type][project id][module vendor][firmware version] */
    sprintf(info->ic_vendor, "%s%x-%s",
        ILITEK_CHIP_NAME,
        g_ilitek_ts->cfg->chip_info->chip_id,
        g_ilitek_ts->project_id);
    sprintf(info->mod_vendor, "%s", g_ilitek_ts->ts_dev_data->module_name);
    sprintf(info->fw_vendor, "%s", g_ilitek_ts->cfg->fw_ver.str);

    return 0;
}

static int ilitek_chip_get_capacitance_test_type(struct ts_test_type_info *info)
{
    int ret = 0;
    struct ts_kit_device_data *ts_dev_data = g_ilitek_ts->ts_dev_data;

    if (IS_ERR_OR_NULL(ts_dev_data) ||
        IS_ERR_OR_NULL(info)) {
        ilitek_err("ts_dev_data or info invaild\n");
        return -EINVAL;
    }

    switch (info->op_action) {
    case TS_ACTION_READ:
        memcpy(info->tp_test_type, ts_dev_data->tp_test_type, TS_CAP_TEST_TYPE_LEN);
        ilitek_info("test_type = %s\n", info->tp_test_type);
        break;
    case TS_ACTION_WRITE:
        break;
    default:
        ilitek_err("invalid op action: %d\n", info->op_action);
        return -EINVAL;
    }

    return 0;
}

static int ilitek_set_info_flag(struct ts_kit_device_data *info)
{
    return 0;
}

/*sense stop-->check cdc-->display off-->DDI sleep in-->tp sleep*/
static int ilitek_before_suspend(void)
{
    ilitek_info("defore suspend start\n");

    /* sense stop */
    ilitek_config_sense_ctrl(CMD_DISABLE);

    /* check system busy */
    if (ilitek_config_check_cdc_busy(50, 10) < 0) {
        ilitek_err("check busy is timout\n");
    }

    ilitek_info("defore suspend end\n");

    return 0;
}


static int ilitek_suspend(void)
{
    struct ts_kit_device_data *ts_dev_data = g_ilitek_ts->ts_dev_data;
    struct ts_kit_platform_data* ts_platform_data = ts_dev_data->ts_platform_data;

    ilitek_info("suspend start\n");

    switch (ts_dev_data->easy_wakeup_info.sleep_mode) {
    case TS_POWER_OFF_MODE:
        ilitek_config_ic_suspend();
        ilitek_pinctrl_select_idle();
        break;
    case TS_GESTURE_MODE:
        if (true == ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value) {
            ilitek_config_into_easy_wakeup();
            mutex_lock(&g_ilitek_ts->wrong_touch_lock);
            ts_dev_data->easy_wakeup_info.off_motion_on = true;
            mutex_unlock(&g_ilitek_ts->wrong_touch_lock);
        } else {
            ilitek_config_ic_suspend();
        }
        break;
    default:
        ilitek_config_ic_suspend();
        ilitek_pinctrl_select_idle();
        break;
    }

    ilitek_info("suspend end\n");

    return 0;
}

/* lcdkit do tp and lcd hw reset before tp resume,
 * so don't send cmd(sleep out, sens start) in resume
 * or do soft reset in after_resume
 */
static int ilitek_resume(void)
{
    struct ts_kit_device_data *ts_dev_data = g_ilitek_ts->ts_dev_data;
    struct ts_kit_platform_data* ts_platform_data = ts_dev_data->ts_platform_data;

    ilitek_info("resume start\n");

    switch (ts_dev_data->easy_wakeup_info.sleep_mode) {
    case TS_POWER_OFF_MODE:
        ilitek_pinctrl_select_active();
        //ilitek_config_ic_resume();
        break;
    case TS_GESTURE_MODE:
        if (true == ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value) {
            ilitek_config_outof_easy_wakeup();
        }
        //ilitek_config_ic_resume();
        break;
    default:
        ilitek_pinctrl_select_active();
        //ilitek_config_ic_resume();
        break;
    }

    ilitek_info("resume end\n");

    return 0;
}

/*  do some thing after power on. */
static int ilitek_after_resume(void *feature_info)
{
    ilitek_info("after resume start\n");

    /* Soft reset */
    //ilitek_config_ice_mode_enable();
    //msleep(10);
    //ilitek_config_ic_reset();

    ilitek_info("after resume end\n");

    return 0;
}

static int ilitek_wakeup_gesture_enable_switch(
    struct ts_wakeup_gesture_enable_info *info)
{
    return 0;
}

int ilitek_get_debug_data(struct ts_diff_data_info *info,
    struct ts_cmd_node *out_cmd)
{
    return 0;
}

static int ilitek_get_glove_switch(u8 *glove_switch)
{
    *glove_switch = ilitek_config_get_glove_status();

    ilitek_info("get glove status = %d\n", *glove_switch);

    return 0;
}

static int ilitek_set_glove_switch(bool glove_enable)
{
    int ret = 0;

    ret = ilitek_config_glove_ctrl(glove_enable ? CMD_ENABLE : CMD_DISABLE);
    if (ret) {
        ilitek_err("glove control failed, enable = %d, ret = %d\n", glove_enable, ret);
        return ret;
    }

    ilitek_info("set glove status = %d\n", glove_enable);

    return 0;
}


static int ilitek_glove_switch(struct ts_glove_info *info)
{
    int ret = 0;

    if (IS_ERR_OR_NULL(info)) {
        ilitek_err("info invaild\n");
        return -EINVAL;
    }

    switch (info->op_action) {
    case TS_ACTION_READ:
        ret = ilitek_get_glove_switch(&info->glove_switch);
        if (ret) {
            ilitek_err("%get glove switch failed, ret = %d\n", ret);
            return ret;
        }
        break;
    case TS_ACTION_WRITE:
        ret = ilitek_set_glove_switch(!!info->glove_switch);
        if (ret) {
            ilitek_err("set glove switch failed, ret = %d\n", ret);
            return ret;
        }
        break;
    default:
        ilitek_err("op action invalid : %d\n", info->op_action);
        return -EINVAL;
    }

    return 0;
}

static void ilitek_shutdown(void)
{

}

static int ilitek_holster_switch(struct ts_holster_info  *info)
{
    return 0;
}

#ifdef ROI
static int ilitek_get_roi_switch(u8 *roi_switch)
{
    *roi_switch = ilitek_config_get_roi_status();

    ilitek_info("get roi status = %d\n", *roi_switch);

    return 0;
}

static int ilitek_set_roi_switch(bool roi_enable)
{
    int ret = 0;

    ret = ilitek_config_roi_ctrl(roi_enable ? CMD_ENABLE : CMD_DISABLE);
    if (ret) {
        ilitek_err("roi control failed, enable = %d, ret = %d\n", roi_enable, ret);
        return ret;
    }

    ilitek_info("set roi status = %d\n", roi_enable);

    return 0;
}

static int ilitek_roi_switch(struct ts_roi_info *info)
{
    int i = 0;
    int ret = 0;
    struct ilitek_report *p_rpt = g_ilitek_ts->rpt;

    if (IS_ERR_OR_NULL(info)) {
        ilitek_err("info invaild\n");
        return -EINVAL;
    }

    switch (info->op_action) {
    case TS_ACTION_READ:
        ret = ilitek_get_roi_switch(&info->roi_switch);
        if (ret) {
            ilitek_err("get roi switch failed, ret = %d\n", ret);
            return ret;
        }
        break;
    case TS_ACTION_WRITE:
        ret = ilitek_set_roi_switch(!!info->roi_switch);
        if (ret) {
            ilitek_err("set roi switch failed, ret = %d\n", ret);
            return ret;
        }

        if(!info->roi_switch){
            for (i = 0; i < ROI_DATA_READ_LENGTH; i++) {
                p_rpt->roi_data[i] = 0;
            }
        }
        break;
    default:
        ilitek_err("op action invalid : %d\n", info->op_action);
        return -EINVAL;
    }

    return 0;
}
#endif

static unsigned char *ilitek_roi_rawdata(void)
{
    struct ilitek_report *p_rpt = g_ilitek_ts->rpt;

    ilitek_debug(DEBUG_ROI, "return roi data\n");

    return p_rpt->roi_data;
}

static int ilitek_palm_switch(struct ts_palm_info *info)
{
    return 0;
}

static int ilitek_regs_operate(struct ts_regs_info *info)
{
    return 0;
}

static int ilitek_calibrate(void)
{
    return 0;
}

static int ilitek_calibrate_wakeup_gesture(void)
{
    return 0;
}

static int ilitek_esdcheck_func(void)
{
    return 0;
}

#if defined(HUAWEI_CHARGER_FB)
static int ilitek_charger_switch(struct ts_charger_info *info)
{
    return 0;
}
#endif

static int ilitek_wrong_touch(void)
{
    struct ts_kit_device_data *ts_dev_data = g_ilitek_ts->ts_dev_data;

    mutex_lock(&g_ilitek_ts->wrong_touch_lock);
    ts_dev_data->easy_wakeup_info.off_motion_on  = true;
    mutex_unlock(&g_ilitek_ts->wrong_touch_lock);

    ilitek_info("wrong touch switch done\n");

    return 0;
}

static int ilitek_debug_switch(u8 loglevel)
{
    switch(loglevel) {
    case TS_INF_MODE:
        ilitek_debug_level = DEBUG_NONE;
        break;
    case TS_DBG_MODE:
        ilitek_debug_level = DEBUG_ALL;
        break;
    case TS_MT_MODE:
        ilitek_debug_level = DEBUG_MP_TEST;
        break;
    default:
        ilitek_debug_level = DEBUG_NONE;
        break;
    }

    ilitek_info("ilitek_debug_level = 0x%x\n", ilitek_debug_level);

    return 0;
}


struct ts_device_ops ilitek_ops = {
    .chip_detect = ilitek_chip_detect,
    .chip_init = ilitek_chip_init,
    .chip_get_brightness_info = ilitek_get_brightness_info,
    .chip_input_config = ilitek_input_config,
    .chip_irq_top_half = ilitek_irq_top_half,
    .chip_irq_bottom_half = ilitek_irq_bottom_half,
    .chip_fw_update_boot = ilitek_fw_update_boot,
    .chip_fw_update_sd = ilitek_fw_update_sd,
    .chip_get_info = ilitek_chip_get_info,
    .chip_get_capacitance_test_type = ilitek_chip_get_capacitance_test_type,
    .chip_set_info_flag = ilitek_set_info_flag,
    .chip_before_suspend = ilitek_before_suspend,
    .chip_suspend = ilitek_suspend,
    .chip_resume = ilitek_resume,
    .chip_after_resume = ilitek_after_resume,
    .chip_wakeup_gesture_enable_switch = ilitek_wakeup_gesture_enable_switch,
    .chip_get_rawdata = ilitek_get_raw_data,
    .chip_special_rawdata_proc_printf = ilitek_rawdata_print,
    .chip_get_debug_data = ilitek_get_debug_data,
    .chip_glove_switch = ilitek_glove_switch,
    .chip_shutdown = ilitek_shutdown,
    .chip_holster_switch = ilitek_holster_switch,
#ifdef ROI
    .chip_roi_rawdata = ilitek_roi_rawdata,
    .chip_roi_switch = ilitek_roi_switch,
#endif
    .chip_palm_switch = ilitek_palm_switch,
    .chip_regs_operate = ilitek_regs_operate,
    .chip_calibrate = ilitek_calibrate,
    .chip_calibrate_wakeup_gesture = ilitek_calibrate_wakeup_gesture,
    .chip_reset = ilitek_chip_reset,
    .chip_check_status = ilitek_esdcheck_func,
#if defined(HUAWEI_CHARGER_FB)
    .chip_charger_switch = ilitek_charger_switch,
#endif
#ifdef HUAWEI_TOUCHSCREEN_TEST
    .chip_test = test_dbg_cmd_test,
#endif
    .chip_wrong_touch = ilitek_wrong_touch,
    .chip_debug_switch = ilitek_debug_switch,
};

static int __init ilitek_ts_module_init(void)
{
    int ret = 0;
    bool found = false;
    struct device_node *child = NULL;
    struct device_node *root = NULL;

    g_ilitek_ts = kzalloc(sizeof(*g_ilitek_ts), GFP_KERNEL);
    if (IS_ERR_OR_NULL(g_ilitek_ts)) {
        ilitek_err("alloc ilitek_ts_data failed\n");
        ret = -ENOMEM;
        goto err_out;
    }
    g_ilitek_ts->ts_dev_data = kzalloc(sizeof(*g_ilitek_ts->ts_dev_data), GFP_KERNEL);
    if (IS_ERR_OR_NULL(g_ilitek_ts->ts_dev_data)) {
        ilitek_err("alloc ts_kit_device_data failed\n");
       ret =  -ENOMEM;
       goto err_ts_free;
    }

    root = of_find_compatible_node(NULL, NULL, ILITEK_DTS_HW_TS_NODE);
    if (IS_ERR_OR_NULL(root)) {
        ilitek_err("find_compatible_node failed\n");
        ret = -EINVAL;
        goto err_dev_free;
    }

    for_each_child_of_node(root, child) {
        if (of_device_is_compatible(child, ILITEK_DTS_NODE)) {
            found = true;
            break;
        }
    }

    if (!found) {
        ilitek_err("device tree node not found, name = %s\n", ILITEK_DTS_NODE);
        ret = -EINVAL;
        goto err_dev_free;
    }

    g_ilitek_ts->ts_dev_data->cnode = child;
    g_ilitek_ts->ts_dev_data->ops = &ilitek_ops;
    ret = huawei_ts_chip_register(g_ilitek_ts->ts_dev_data);
    if (ret) {
        ilitek_err("chip register fail, ret = %d\n", ret);
        goto err_dev_free;
    }

    ilitek_info("module init succeeded\n");

    return 0;

err_dev_free:
    if(g_ilitek_ts->ts_dev_data) {
        kfree(g_ilitek_ts->ts_dev_data);
        g_ilitek_ts->ts_dev_data = NULL;
    }
err_ts_free:
    if (g_ilitek_ts) {
        kfree(g_ilitek_ts);
        g_ilitek_ts = NULL;
    }
err_out:
    ilitek_err("module init failed\n");
    return ret;
}

static void __exit ilitek_ts_module_exit(void)
{
    ilitek_proc_remove();
    ilitek_upgrade_exit();
    ilitek_report_exit();
    ilitek_config_exit();
    ilitek_protocol_exit();
    ilitek_pinctrl_select_default();
    ilitek_pinctrl_release();

    if(g_ilitek_ts->ts_dev_data) {
        kfree(g_ilitek_ts->ts_dev_data);
        g_ilitek_ts->ts_dev_data = NULL;
    }

    if (g_ilitek_ts) {
        kfree(g_ilitek_ts);
        g_ilitek_ts = NULL;
    }

    ilitek_info("module exit succeeded\n");
}

module_init(ilitek_ts_module_init);
module_exit(ilitek_ts_module_exit);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");

