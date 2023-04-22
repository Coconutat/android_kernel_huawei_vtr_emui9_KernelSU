#include "ilitek_common.h"
#include "ilitek_dts.h"
#include "ilitek_protocol.h"
#include "ilitek_config.h"
#include "ilitek_mp_test.h"
#include "ilitek_parser.h"
#include "ilitek_test.h"

extern struct mp_test_items tItems[];

static struct ilitek_test *g_ilitek_test = NULL;
static u8 tp_test_failed_reason[ILITEK_TEST_FAILED_REASON_LEN] = {0};

mp_tests g_ilitek_test_items[ILITEK_TEST_ITEMS] = {
    MP_TEST_NOISE_PEAK_TO_PEAK_PANEL,
    MP_TEST_NOISE_PEAK_TO_PEAK_IC,
    MP_TEST_RX_SHORT,
    MP_TEST_OPEN_INTERGRATION_SP,
    MP_TEST_MUTUAL_BK,
    //MP_TEST_MUTUAL_BK_LCM_OFF,
    MP_TEST_MUTUAL_DAC,
    MP_TEST_MUTUAL_NO_BK,
    //MP_TEST_MUTUAL_NO_BK_LCM_OFF,
    //MP_TEST_NOISE_PEAK_TO_PEAK_PANEL_LCM_OFF,
    //MP_TEST_NOISE_PEAK_TO_PEAK_IC_LCM_OFF,
    //MP_TEST_DOZE_RAW_TD_LCM_OFF,
    //MP_TEST_DOZE_P2P_TD_LCM_OFF,
    //MP_TEST_DOZE_RAW,
    //MP_TEST_DOZE_P2P,
};

int ilitek_test_hash(mp_tests key)
{
    int value = 0;

    for (value = 0; value < ILITEK_TEST_ITEMS; value++) {
        if (g_ilitek_test_items[value] == key) {
            return value;
        }
    }

    ilitek_err("invaild test item\n");

    return -EINVAL;
}

int ilitek_test_init (void)
{
    g_ilitek_test = vmalloc(sizeof(*g_ilitek_test));
    if (IS_ERR_OR_NULL(g_ilitek_test)) {
        ilitek_err("alloc ilitek_test_data failed\n");
        return -ENOMEM;
    }

    g_ilitek_ts->test = g_ilitek_test;

    ilitek_info("test init succeeded\n");

    return 0;
}

void ilitek_test_exit(void)
{
    if (g_ilitek_test) {
        vfree(g_ilitek_test);
        g_ilitek_test = NULL;
    }

    g_ilitek_ts->test = NULL;

    ilitek_info("test exit succeeded\n");
}

static int ilitek_test_get_ini_path(void)
{
    char file_name[ILITEK_FILE_NAME_LEN] = {0};
    struct ts_kit_device_data *p_dev_data = g_ilitek_ts->ts_dev_data;

    if (!strnlen(p_dev_data->ts_platform_data->product_name, MAX_STR_LEN - 1)
        || !strnlen(p_dev_data->chip_name, MAX_STR_LEN - 1)
        || !strnlen(g_ilitek_ts->project_id, ILITEK_PROJECT_ID_LEN)
        || !strnlen(p_dev_data->module_name, MAX_STR_LEN - 1)) {
        ilitek_err("csv file name is not detected\n");
        return -EINVAL;
    }

    snprintf(file_name, ILITEK_FILE_NAME_LEN, "%s_%s_%s_%s_raw.ini",
        p_dev_data->ts_platform_data->product_name,
        p_dev_data->chip_name,
        g_ilitek_ts->project_id,
        p_dev_data->module_name);

    snprintf(g_ilitek_ts->ini_path, ILITEK_FILE_PATH_LEN, "%s%s",
        ILITEK_INI_PATH_PERFIX,
        file_name);

    ilitek_info("ini file path = %s\n", g_ilitek_ts->ini_path);

    return 0;
}

static void ilitek_result_init(u8 *result_str)
{
    u8 module_info[ILITEK_TEST_MODULE_LEN] = {0};

    memset(result_str, 0, sizeof(*result_str));

    strncat(result_str, "0F-1F-3F-5F-xF-", strlen("0F-1F-3F-5F-xF-"));
    strncat(result_str, tp_test_failed_reason, ILITEK_TEST_FAILED_REASON_LEN);

    sprintf(module_info, "%s%x-%s",
        ILITEK_CHIP_NAME,
        g_ilitek_ts->cfg->chip_info->chip_id,
        g_ilitek_ts->project_id);

    strncat(result_str, module_info, strlen(module_info));

    /* avoid pull result file error */
    strncat(result_str, ";", strlen(";"));

    ilitek_debug(DEBUG_MP_TEST, "result init is %s\n", result_str);
}

static int ilitek_enter_factory(void)
{
    int ret = 0, retry = ILITEK_SWITCH_TEST_MODE_RETRY;

    /* avoid irq to recv i2c data */
    ilitek_config_disable_report_irq();

    while (retry--) {
        /* Switch to Test mode */
        ret = ilitek_config_mode_ctrl(ILITEK_TEST_MODE, NULL);
        if (ret) {
            ilitek_err("Switch Test Mode failed, retry = %d\n", retry);
        } else {
            return 0;
        }
    };

    return ret;
}

static void ilitek_exit_factory(void)
{
    int ret = 0;

    /* hw reset avoid i2c error */
    ilitek_chip_reset();

    /* Switch to Demo mode */
    ret = ilitek_config_mode_ctrl(ILITEK_DEMO_MODE, NULL);
    if (ret) {
        ilitek_err("switch demo mode failed, still in test mode\n");
    }

    ilitek_config_enable_report_irq();

}

static void ilitek_result_generate(u8 *result_str)
{
    bool result1 = true, result2 = true;
    u8 module_info[ILITEK_TEST_MODULE_LEN] = {0};

    g_ilitek_test->test_result = true;
    memset(result_str, 0, sizeof(result_str));

    /* 0- i2c test */
    strncat(result_str, "0P-", strlen("0P-"));

    /* 1- cap rawdata */
    result1 = ilitek_check_result(MP_TEST_MUTUAL_BK);
    result2 = ilitek_check_result(MP_TEST_MUTUAL_NO_BK);
    if (result1 && result2) {
        strncat(result_str, "1P-", ILITEK_TEST_ITEM_RES_LEN);
    } else {
        if (!result1) {
            ilitek_err("MP_TEST_MUTUAL_BK failed\n");
        }
        if (!result2) {
            ilitek_err("MP_TEST_MUTUAL_NO_BK failed\n");
        }
        g_ilitek_test->test_result = false;
        strncat(result_str, "1F-", ILITEK_TEST_ITEM_RES_LEN);
        strncpy(tp_test_failed_reason, "panel_reason-" , ILITEK_TEST_FAILED_REASON_LEN);
    }

    /* 3- noise test */
    result1 = ilitek_check_result(MP_TEST_NOISE_PEAK_TO_PEAK_PANEL);
    result2 = ilitek_check_result(MP_TEST_NOISE_PEAK_TO_PEAK_IC);
    if (result1 && result2) {
        strncat(result_str, "3P-", ILITEK_TEST_ITEM_RES_LEN);
    } else {
        if (!result1) {
            ilitek_err("MP_TEST_NOISE_PEAK_TO_PEAK_PANEL failed\n");
        }
        if (!result2) {
            ilitek_err("MP_TEST_NOISE_PEAK_TO_PEAK_IC failed\n");
        }
        g_ilitek_test->test_result = false;
        strncat(result_str, "3F-", ILITEK_TEST_ITEM_RES_LEN);
        strncpy(tp_test_failed_reason, "panel_reason-" , ILITEK_TEST_FAILED_REASON_LEN);
    }

    /* 5- open short test */
    result1 = ilitek_check_result(MP_TEST_RX_SHORT);
    result2 = ilitek_check_result(MP_TEST_OPEN_INTERGRATION_SP);
    if (result1 && result2) {
        strncat(result_str, "5P-", ILITEK_TEST_ITEM_RES_LEN);
    } else {
        if (!result1) {
            ilitek_err("MP_TEST_RX_SHORT failed\n");
        }
        if (!result2) {
            ilitek_err("MP_TEST_OPEN_INTERGRATION_SP failed\n");
        }
        g_ilitek_test->test_result = false;
        strncat(result_str, "5F-", ILITEK_TEST_ITEM_RES_LEN);
        strncpy(tp_test_failed_reason, "panel_reason-" , ILITEK_TEST_FAILED_REASON_LEN);
    }

    /* add x- mutual dac test */
    result1 = ilitek_check_result(MP_TEST_MUTUAL_DAC);
    if (result1){
        strncat(result_str, "xP-", ILITEK_TEST_ITEM_RES_LEN);
    } else {
        ilitek_err("MP_TEST_MUTUAL_DAC failed\n");

        g_ilitek_test->test_result = false;
        strncat(result_str, "xF-", ILITEK_TEST_ITEM_RES_LEN);
        strncpy(tp_test_failed_reason, "panel_reason-" , ILITEK_TEST_FAILED_REASON_LEN);
    }

    /* generate failed reason */
    if (!g_ilitek_test->test_result) {
        strncat(result_str, tp_test_failed_reason, ILITEK_TEST_FAILED_REASON_LEN);
    }

    /* generate rawdata test result */
    sprintf(module_info, "%s%x-%s",
        ILITEK_CHIP_NAME,
        g_ilitek_ts->cfg->chip_info->chip_id,
        g_ilitek_ts->project_id);

    strncat(result_str, module_info, strlen(module_info));

    /* avoid pull result file error */
    strncat(result_str, ";", strlen(";"));

    ilitek_debug(DEBUG_MP_TEST, "test result is %s\n", result_str);
}

int ilitek_get_raw_data(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd)
{
    int ret = 0;
    int i = 0, j = 0, offset = 0;
    int x_ch = 0, y_ch = 0, data_nums = 0;
    unsigned long timer_start = 0, timer_end = 0;
    u8 result_str[TS_RAWDATA_RESULT_MAX] = {0};

    timer_start = jiffies;

    ilitek_info("mp test start\n");

    ret = ilitek_test_init();
    if (ret) {
        ilitek_err("alloc test data failed\n");
        strncpy(tp_test_failed_reason, "software_reason-" , ILITEK_TEST_FAILED_REASON_LEN);
        goto err_out;
    }

    /* Init MP structure */
    ret = core_mp_init();
    if (ret) {
        ilitek_err("init mp failed\n");
        strncpy(tp_test_failed_reason, "software_reason-" , ILITEK_TEST_FAILED_REASON_LEN);
        goto err_test_data_free;
    }

    ilitek_debug(DEBUG_MP_TEST, "judge_ic_type is %s\n",
        g_ilitek_ts->ts_dev_data->tp_test_type);

    ilitek_debug(DEBUG_MP_TEST, "only_open_once_captest_threshold is %d\n",
        g_ilitek_ts->only_open_once_captest_threshold);

    ret = ilitek_test_get_ini_path();
    if (ret) {
        ilitek_err("get ini file path failed\n");
        strncpy(tp_test_failed_reason, "software_reason-" , ILITEK_TEST_FAILED_REASON_LEN);
        goto err_mp_test_free;
    }

    if (false == g_ilitek_ts->open_threshold_status) {
        ret = core_parser_path(g_ilitek_ts->ini_path);
        if (ret) {
            ilitek_err("parsing ini file failed\n");
            strncpy(tp_test_failed_reason, "software_reason-" , ILITEK_TEST_FAILED_REASON_LEN);
            goto err_mp_test_free;
        }

        if (g_ilitek_ts->only_open_once_captest_threshold) {
            g_ilitek_ts->open_threshold_status = true;
        }
    }

    ret = ilitek_enter_factory();
    if (ret) {
        ilitek_err("enter factory mode failed, back to normal mode\n");
        strncpy(tp_test_failed_reason, "tp_initial_failed-" , ILITEK_TEST_FAILED_REASON_LEN);
        goto err_test_mode;
    }

    /* Do not chang the sequence of test */
    core_mp_run_test("Noise Peak To Peak(With Panel)", true);
    core_mp_run_test("Noise Peak to Peak(IC Only)", true);
    core_mp_run_test("Short Test -ILI9881", true);
    core_mp_run_test("Open Test(integration)_SP", true);
    core_mp_run_test("Raw Data(Have BK)", true);
    //core_mp_run_test("Raw Data(Have BK) (LCM OFF)", true);
    core_mp_run_test("Calibration Data(DAC)", true);
    core_mp_run_test("Raw Data(No BK)", true);
    //core_mp_run_test("Raw Data(No BK) (LCM OFF)", true);
    //core_mp_run_test("Noise Peak to Peak(With Panel) (LCM OFF)", true);
    //core_mp_run_test("Noise Peak to Peak(IC Only) (LCM OFF)", true);
    //core_mp_run_test("Raw Data_TD (LCM OFF)", true);
    //core_mp_run_test("Peak To Peak_TD (LCM OFF)", true);
    //core_mp_run_test("Doze Raw Data", true);
    //core_mp_run_test("Doze Peak To Peak", true);
    //core_mp_run_test("Pin Test ( INT and RST )", true);

    ilitek_exit_factory();

    /* debug open to generate result csv file in /sdcard */
    if (DEBUG_MP_TEST & ilitek_debug_level) {
        core_mp_show_result();
    }

    /* copy to ts kit info->result to show */
    ilitek_result_generate(result_str);
    strncpy(info->result, result_str, TS_RAWDATA_RESULT_MAX);

    /* Y ==> Rx & X ==> Tx num */
    x_ch = g_ilitek_ts->cfg->tp_info.nXChannelNum;
    y_ch = g_ilitek_ts->cfg->tp_info.nYChannelNum;
    data_nums = y_ch * x_ch;

    info->buff[0] = y_ch;
    info->buff[1] = x_ch;
    for(offset = 2, i = 0; i < ILITEK_TEST_ITEMS; i++) {
        for (j = 0; j < data_nums; j++) {
            info->buff[offset + j] = g_ilitek_test->orignal_data[i][j];
        }
        offset += data_nums;
    }

    core_mp_test_free();
    ilitek_test_exit();

    timer_end = jiffies;
    ilitek_info("mp test result: %s\n", info->result);
    ilitek_info("mp test end, use time: %dms\n", jiffies_to_msecs(timer_end - timer_start));

    return 0;

err_test_mode:
    ilitek_exit_factory();
err_mp_test_free:
    core_mp_test_free();
err_test_data_free:
    ilitek_test_exit();
err_out:
    ilitek_result_init(result_str);
    strncpy(info->result, result_str, strlen(result_str));
    ilitek_info("mp test failed result: %s\n", info->result);
    return ret;
}

int ilitek_rawdata_print(struct seq_file *m, struct ts_rawdata_info *info,
    int range_size, int row_size)
{
    int i = 0, index = 0, offset = 0;
    int x_ch = 0, y_ch = 0, data_nums = 0;
    int row = 0, column = 0;

    x_ch = g_ilitek_ts->cfg->tp_info.nXChannelNum;
    y_ch = g_ilitek_ts->cfg->tp_info.nYChannelNum;
    data_nums = x_ch * y_ch;

    for (offset = 2, i = 0; i < ILITEK_TEST_ITEMS; i++) {
        index = g_ilitek_test_items[i];
        seq_printf(m, "%s\n", tItems[index].desp);
        seq_printf(m, "frame count: %d\n", tItems[index].frame_count);
        for (row = 0; row < y_ch; row++) {
            for (column = 0; column < x_ch; column++) {
                seq_printf(m, "%5d,", info->buff[offset + row * x_ch + column]);
            }
            seq_printf(m, "\n");
        }
        offset += data_nums;
        seq_printf(m, "\n");
    }

    return 0;
}
