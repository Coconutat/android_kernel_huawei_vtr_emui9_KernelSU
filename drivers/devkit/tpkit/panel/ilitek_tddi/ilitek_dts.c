#include "ilitek_common.h"
#include "ilitek_dts.h"
#include "../../tpkit_platform_adapter.h"


/*  must config dts propertys to chip detect */
int ilitek_prase_ic_config_dts(struct device_node *np)
{
    int ret = 0;
    struct ts_kit_device_data* ts_dev_data = g_ilitek_ts->ts_dev_data;

    ret = of_property_read_u32(np, ILITEK_DTS_SLAVE_ADDR, &ts_dev_data->slave_addr);
    if (ret) {
        ilitek_err("get slave addr failed\n");
        return -ENODATA;
    }
    ilitek_info("slvae addr 0x%x\n", ts_dev_data->slave_addr);

    ret = of_property_read_u32(np, ILITEK_DTS_CHIP_ID, &g_ilitek_ts->chip_id);
    if (ret) {
        ilitek_err("get chip id failed\n");
        return -ENODATA;
    }
    ilitek_info("chip id 0x%x\n", g_ilitek_ts->chip_id);

    return 0;
}

static int ilitek_parse_report_config_dts(struct device_node *np)
{
    int ret = 0;

    ret = of_property_read_u32(np, ILITEK_DTS_X_MAX, &g_ilitek_ts->x_max);
    if (ret) {
        ilitek_info("get x_max failed, use default: %d\n",
            ILITEK_TOUCSCEEN_X_MAX);
        g_ilitek_ts->x_max = ILITEK_TOUCSCEEN_X_MAX;
    }

    ret = of_property_read_u32(np, ILITEK_DTS_Y_MAX, &g_ilitek_ts->y_max);
    if (ret) {
        ilitek_info("get y_max failed, use default: %d\n",
            ILITEK_TOUCSCEEN_Y_MAX);
        g_ilitek_ts->y_max = ILITEK_TOUCSCEEN_Y_MAX;
    }

    ret = of_property_read_u32(np, ILITEK_DTS_X_MIN, &g_ilitek_ts->x_min);
    if (ret) {
        ilitek_info("get x_min failed, use default: %d\n",
            ILITEK_TOUCSCEEN_X_MIN);
        g_ilitek_ts->x_min = ILITEK_TOUCSCEEN_X_MIN;
    }

    ret = of_property_read_u32(np, ILITEK_DTS_Y_MIN, &g_ilitek_ts->y_min);
    if (ret) {
        ilitek_info("get y_min failed, use default: %d\n",
            ILITEK_TOUCSCEEN_Y_MIN);
        g_ilitek_ts->y_min = ILITEK_TOUCSCEEN_Y_MIN;
    }

    ret = of_property_read_u32(np, ILITEK_DTS_FINGER_NUMS, &g_ilitek_ts->finger_nums);
    if (ret) {
        ilitek_info("get finger_nums failed, use default: %d\n",
            ILITEK_TOUCSCEEN_FINGERS);
        g_ilitek_ts->finger_nums = ILITEK_TOUCSCEEN_FINGERS;
    }

    ilitek_info("x = [%d, %d], y = [%d, %d], finger_nums = %d\n",
        g_ilitek_ts->x_min,
        g_ilitek_ts->x_max,
        g_ilitek_ts->y_min,
        g_ilitek_ts->y_max,
        g_ilitek_ts->finger_nums);

    return 0;
}

int ilitek_parse_ic_special_dts(struct device_node *np)
{
    int ret = 0;
    const char *str_value = NULL;
    struct device_node *self = NULL;
    struct ts_kit_device_data* ts_dev_data = g_ilitek_ts->ts_dev_data;

    self = of_find_node_by_name(np, g_ilitek_ts->project_id);
    if (IS_ERR_OR_NULL(self)) {
        ilitek_err("find %s node failed\n", g_ilitek_ts->project_id);
        return -ENODATA;
    }

    ret = of_property_read_string(self, ILITEK_DTS_PRODUCER, &str_value);
    if (ret) {
        ilitek_info("get module_name failed, use default: %s\n",
            ILITEK_DTS_DEF_PRODUCER);
        str_value = ILITEK_DTS_DEF_PRODUCER;
    }
    strncpy(ts_dev_data->module_name, str_value, MAX_STR_LEN);
    ilitek_info("module_name = %s\n", ts_dev_data->module_name);

    return 0;
}

int ilitek_parse_dts(struct device_node *np)
{
    int ret = 0;
    const char *str_value = NULL;
    struct ts_kit_device_data* ts_dev_data = g_ilitek_ts->ts_dev_data;

    ret = of_property_read_u32(np, ILITEK_DTS_IC_TYEP, &ts_dev_data->ic_type);
    if (ret < 0) {
        ilitek_info("get ic_type failed, use default: %d\n", TDDI);
        ts_dev_data->ic_type = TDDI;
    }
    g_tskit_ic_type = ts_dev_data->ic_type;
    ilitek_info("ic_type = %d\n", g_tskit_ic_type);

    ret = of_property_read_u32(np, ILITEK_DTS_ALGO_ID, &ts_dev_data->algo_id);
    if (ret < 0) {
        ilitek_info("get algo_id failed, use default: %d\n",
            ILITEK_DTS_DEF_ALGO);
        ts_dev_data->algo_id = ILITEK_DTS_DEF_ALGO;
    }
    ilitek_info("algo_id = %d\n", ts_dev_data->algo_id);

    ret = of_property_read_u32(np, ILITEK_DTS_IS_IN_CELL, &ts_dev_data->is_in_cell);
    if (ret < 0) {
        ilitek_info("get is_in_cell failed, use default: %d\n",
            ILITEK_DTS_DEF_IS_IN_CELL);
        ts_dev_data->is_in_cell = ILITEK_DTS_DEF_IS_IN_CELL;
    }
    ilitek_info("is_in_cell = %d\n", ts_dev_data->is_in_cell);

    ret = of_property_read_u32(np, ILITEK_DTS_IRQ_CFG, &ts_dev_data->irq_config);
    if (ret < 0) {
        ilitek_info("get irq_config failed, use default: %d\n",
            ILITEK_DTS_DEF_IRQ_CONFIG);
        ts_dev_data->irq_config = ILITEK_DTS_DEF_IRQ_CONFIG;
    }
    ilitek_info("irq_config = %d\n", ts_dev_data->irq_config);

    ret = of_property_read_u32(np, ILITEK_DTS_USE_IC_RES, &g_ilitek_ts->use_ic_res);
    if (ret) {
        ilitek_info("get use_ic_res failed, use default: %d\n",
            ILITEK_DTS_DEF_USE_IC_RES);
        g_ilitek_ts->use_ic_res = ILITEK_DTS_DEF_USE_IC_RES;
    }
    ilitek_info("use_ic_res = %d\n", g_ilitek_ts->use_ic_res);

    /* convert ic resolution x[0,2047] y[0,2047] to real resolution, fix inaccurate touch in recovery mode,
     * there is no coordinate conversion in recovery mode.
     */
    if (get_into_recovery_flag_adapter()) {
        g_ilitek_ts->use_ic_res = ILITEK_DTS_DEF_USE_IC_RES;
        ilitek_info("recovery mode driver convert resolution\n");
    }

    ret = of_property_read_u32(np, ILITEK_DTS_ROI, &g_ilitek_ts->support_roi);
    if (ret) {
        ilitek_info("get support_roi failed, use default: %d\n",
            ILITEK_DTS_DEF_SUPPORT_ROI);
        g_ilitek_ts->support_roi = ILITEK_DTS_DEF_SUPPORT_ROI;
    }
    ts_dev_data->ts_platform_data->feature_info.roi_info.roi_supported = g_ilitek_ts->support_roi;
    ilitek_info("support_roi = %d\n", g_ilitek_ts->support_roi);

    ret = of_property_read_u32(np, ILITEK_DTS_GESTURE, &g_ilitek_ts->support_gesture);
    if (ret) {
        ilitek_info("get support_gesture failed, use default: %d\n",
            ILITEK_DTS_DEF_SUPPORT_GESTURE);
        g_ilitek_ts->support_gesture = ILITEK_DTS_DEF_SUPPORT_GESTURE;
    }
    ts_dev_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value = g_ilitek_ts->support_gesture;
    ilitek_info("support_gesture = %d\n", g_ilitek_ts->support_gesture);

    ret = of_property_read_u32(np, ILITEK_DTS_PRESSURE, &g_ilitek_ts->support_pressure);
    if (ret) {
        ilitek_info("get support_pressure failed, use default: %d\n",
            ILITEK_DTS_DEF_SUPPORT_PRESSURE);
        g_ilitek_ts->support_pressure = ILITEK_DTS_DEF_SUPPORT_PRESSURE;
    }
    ilitek_info("support_pressure = %d\n", g_ilitek_ts->support_pressure);

    ret = of_property_read_u32(np, ILITEK_DTS_TP_COLOR,
        &g_ilitek_ts->support_get_tp_color);
    if (ret) {
        ilitek_info("get support_get_tp_color failed, use default: %d\n",
            ILITEK_DTS_DEF_TP_COLOR);
        g_ilitek_ts->support_get_tp_color = ILITEK_DTS_DEF_TP_COLOR;
    }
    ilitek_info("support_get_tp_color = %d\n", g_ilitek_ts->support_get_tp_color);

    /*
     * use_lcdkit_power_notify set 0,fb_notify log is on but not deal with FB_EVENT.
     * TDDI in qcom platform, mdss_dsi_panel_on/off-->ts_kit_power_control_notify.
     * TDDI in hisi platform, lcdkit_on/off-->lcdkit_notifier_call_chain-->ts_kit_power_notify_callback-->
     * ts_kit_power_control_notify.
     */
    ret = of_property_read_u32(np, ILITEK_DTS_LCD_POWER_NOTIFY, &ts_dev_data->use_lcdkit_power_notify);
    if (ret) {
        ts_dev_data->use_lcdkit_power_notify = ILITEK_DTS_DEF_LCD_POWER_NOTIFY;
        ilitek_info("get use_lcdkit_power_notify failed , tddi fb_notify log off and lcdkit power notify\n");
    }
    ilitek_info("use_lcdkit_power_notify = %d\n", ts_dev_data->use_lcdkit_power_notify);

    ret = of_property_read_u32(np, ILITEK_DTS_PROJECT_ID_CTRL,
        &g_ilitek_ts->project_id_length_control);
    if (ret) {
        ilitek_info("get project_id_length_control failed, use default: %d\n",
            ILITEK_DTS_DEF_PROJECT_ID_CTRL);
        g_ilitek_ts->project_id_length_control = ILITEK_DTS_DEF_PROJECT_ID_CTRL;
    }
    ilitek_info("project_id_length_control = %d\n", g_ilitek_ts->project_id_length_control);

    ret = of_property_read_u32(np, ILITEK_DTS_RAW_DATA_PRINT,
        &ts_dev_data->is_ic_rawdata_proc_printf);
    if (ret) {
        ilitek_info("get is_ic_rawdata_proc_printf failed, use default: %d\n",
            ILITEK_DTS_DEF_RAWDATA_PRINT);
        ts_dev_data->is_ic_rawdata_proc_printf = ILITEK_DTS_DEF_RAWDATA_PRINT;
    }
    ilitek_info("is_ic_rawdata_proc_printf = %d\n", ts_dev_data->is_ic_rawdata_proc_printf);

    ret = of_property_read_u32(np, ILITEK_DTS_OPEN_ONCE_THRESHOLD,
        &g_ilitek_ts->only_open_once_captest_threshold);
    if (ret) {
        ilitek_info("get only_open_once_captest_threshold failed, use default: %d\n",
            ILITEK_DTS_DEF_TH_OPEN_ONCE);
        g_ilitek_ts->only_open_once_captest_threshold = ILITEK_DTS_DEF_TH_OPEN_ONCE;
    }
    ilitek_info("only_open_once_captest_threshold = %d\n", g_ilitek_ts->only_open_once_captest_threshold);

    ret = of_property_read_string(np, ILITEK_DTS_TEST_TYPE, &str_value);
    if (ret) {
        ilitek_info("get tp_test_type failed, use default: %s\n",
            ILITEK_DTS_DEF_TEST_TYPE);
        str_value = ILITEK_DTS_DEF_TEST_TYPE;
    }
    strncpy(ts_dev_data->tp_test_type, str_value, TS_CAP_TEST_TYPE_LEN);
    ilitek_info("tp_test_type = %s\n", ts_dev_data->tp_test_type);

    /* avoid read project id failed, tp work abnormally */
    ilitek_parse_report_config_dts(np);

    ilitek_info("parse config in dts succeeded\n");

    return 0;
}
