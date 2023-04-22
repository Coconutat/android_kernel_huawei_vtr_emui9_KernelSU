#include "ilitek_common.h"
#include "ilitek_dts.h"
#include "ilitek_protocol.h"
#include "ilitek_config.h"
#include "ilitek_report.h"
#include "ilitek_mp_test.h"

static struct ilitek_protocol *g_ilitek_pro = NULL;

static u32 calc_package_length_5_x(pro_fw_modes mode);
static int parse_package_5_x(u8 package_id);
static int mode_control_5_x(pro_fw_modes mode, u8 *data);

struct ilitek_version pro_versions[ILITEK_VER_NUMS] = {
    PROTOCOL_VERSION(5, 0, 0),
    PROTOCOL_VERSION(5, 1, 0),
    PROTOCOL_VERSION(5, 2, 0),
    PROTOCOL_VERSION(5, 3, 0),
    PROTOCOL_VERSION(5, 4, 0),
};

struct protocol_control pro_5_0_ctrls[ILITEK_CTRL_NUMS] = {
    PROTOCOL_CONTROL("sense_ctrl", 0x1, 0x0, NA, NA, 2),
    PROTOCOL_CONTROL("sleep_ctrl", 0x2, 0x0, NA, NA, 2),
    PROTOCOL_CONTROL("glove_ctrl", 0x6, 0x0, NA, NA, 2),
    PROTOCOL_CONTROL("stylus_ctrl", 0x7, 0x0, NA, NA, 2),
    PROTOCOL_CONTROL("scan_mode_ctrl", 0x8, 0x0, NA, NA, 2),
    PROTOCOL_CONTROL("lpwg_ctrl", 0xA, 0x0, NA, NA, 2),
    PROTOCOL_CONTROL("gesture_ctrl", 0xB, 0x3F, NA, NA, 2),
    PROTOCOL_CONTROL("phone_cover_ctrl", 0xC, 0x0, NA, NA, 2),
    PROTOCOL_CONTROL("finger_sense_ctrl", 0xF, 0x0, NA, NA, 2),
    PROTOCOL_CONTROL("proximity_ctrl", 0x0, NA, NA, NA, 0), /* protocol 5.0 not support */
    PROTOCOL_CONTROL("plug_ctrl", 0x0, NA, NA, NA, 0),      /* protocol 5.0 not support */
    PROTOCOL_CONTROL("phone_cover_window_ctrl", 0xD, NA, NA, NA, 8),
    PROTOCOL_CONTROL("roi_ctrl", 0x0, 0x0, NA, NA, 0), /* protocol 5.0 not support */
};

struct protocol_control pro_5_x_ctrls[ILITEK_CTRL_NUMS] = {
    PROTOCOL_CONTROL("sense_ctrl", 0x1, 0x1, 0x0, NA, 3),
    PROTOCOL_CONTROL("sleep_ctrl", 0x1, 0x2, 0x0, NA, 3),
    PROTOCOL_CONTROL("glove_ctrl", 0x1, 0x6, 0x0, NA, 3),
    PROTOCOL_CONTROL("stylus_ctrl", 0x1, 0x7, 0x0, NA, 3),
    PROTOCOL_CONTROL("scan_mode_ctrl", 0x1, 0x8, 0x0, NA, 3),
    PROTOCOL_CONTROL("lpwg_ctrl", 0x1, 0xA, 0x0, NA, 3),
    PROTOCOL_CONTROL("gesture_ctrl", 0x1, 0xB, 0x3F, NA, 3),
    PROTOCOL_CONTROL("phone_cover_ctrl", 0x1, 0xC, 0x0, NA, 3),
    PROTOCOL_CONTROL("finger_sense_ctrl", 0x1, 0xF, 0x0, NA, 3),
    PROTOCOL_CONTROL("proximity_ctrl", 0x1, 0x10, 0x0, NA, 3),
    PROTOCOL_CONTROL("plug_ctrl", 0x1, 0x11, 0x0, NA, 3),
    PROTOCOL_CONTROL("phone_cover_window_ctrl", 0xD, NA, NA, NA, 8),
    PROTOCOL_CONTROL("roi_ctrl", 0x1, 0xF, 0x0, NA, 3),
};

struct protocol_ic_info pro_5_x_infos[ILITEK_INFO_NUMS] = {
    PROTOCOL_IC_INFO("pro_info", ilitek_parse_protocol_verison, P5_X_GET_PROTOCOL_VERSION, 4),
    PROTOCOL_IC_INFO("fw_info",  ilitek_parse_firmware_version, P5_X_GET_FIRMWARE_VERSION, 4),
    PROTOCOL_IC_INFO("core_info", ilitek_parse_core_version, P5_X_GET_CORE_VERSION, 5),
    PROTOCOL_IC_INFO("tp_info",  ilitek_parse_tp_info, P5_X_GET_TP_INFORMATION, 14),
    PROTOCOL_IC_INFO("key_info", ilitek_parse_key_info, P5_X_GET_KEY_INFORMATION, 30),
};

struct protocol_firmware_mode pro_5_x_modes[ILITEK_MODE_NUMS] = {
    PROTOCOL_FIRMWARE_MODE("demo_mode", P5_X_FIRMWARE_DEMO_MODE, P5_X_DEMO_PACKET_ID, P5_X_DEMO_MODE_PACKET_LENGTH),
    PROTOCOL_FIRMWARE_MODE("test_mode", P5_X_FIRMWARE_TEST_MODE, P5_X_TEST_PACKET_ID, P5_X_TEST_MODE_PACKET_LENGTH),
    PROTOCOL_FIRMWARE_MODE("debug_mode", P5_X_FIRMWARE_DEBUG_MODE, P5_X_DEBUG_PACKET_ID, P5_X_DEBUG_MODE_PACKET_LENGTH),
    PROTOCOL_FIRMWARE_MODE("i2c_uart_mode", P5_X_FIRMWARE_I2CUART_MODE, P5_X_I2CUART_PACKET_ID, 0),
    PROTOCOL_FIRMWARE_MODE("gesture_mode", P5_X_FIRMWARE_GESTURE_MODE, P5_X_GESTURE_PACKET_ID, 0),
};

struct protocol_funcs pro_5_x_funcs =
    PROTOCOL_REPORT_PACKAGE(calc_package_length_5_x, parse_package_5_x, mode_control_5_x);

static u32 calc_package_length_5_x(pro_fw_modes mode)
{
    u16 x_cn = 0, y_cn = 0, st_cn = 0, sr_cn = 0;
    u16 self_key = 2;
    u32 len = 0;
    struct ilitek_config *p_cfg = g_ilitek_ts->cfg;

    if (p_cfg->tp_info.nXChannelNum == 0 ||
        p_cfg->tp_info.nYChannelNum == 0 ||
        p_cfg->tp_info.self_tx_channel_num == 0 ||
        p_cfg->tp_info.self_rx_channel_num == 0) {
        switch(mode){
        case ILITEK_DEMO_MODE:
        case ILITEK_TEST_MODE:
        case ILITEK_DEBUG_MODE:
            len = g_ilitek_pro->modes[mode].len;
            break;
        default:
            len = 0;
            ilitek_err("unknow firmware mode : %d\n", mode);
            break;
        }
    } else {
        x_cn = p_cfg->tp_info.nXChannelNum;
        y_cn = p_cfg->tp_info.nYChannelNum;
        st_cn = p_cfg->tp_info.self_tx_channel_num;
        sr_cn = p_cfg->tp_info.self_rx_channel_num;

        switch(mode){
        case ILITEK_DEMO_MODE:
            len = g_ilitek_pro->modes[mode].len;
            break;
        case ILITEK_TEST_MODE:
            len = (2 * x_cn * y_cn) + (st_cn * 2) + (sr_cn * 2) +
                2 * self_key + 1;
            len += 1;
            break;
        case ILITEK_DEBUG_MODE:
            len = (2 * x_cn * y_cn) + (st_cn * 2) + (sr_cn * 2) +
                2 * self_key + (8 * 2) + 1;
            len += 35;
            break;
        default:
            len = 0;
            ilitek_err("unknow firmware mode : %d\n", mode);
            break;
        }
    }

    ilitek_debug(DEBUG_FINGER_REPORT, "package len = %d\n", len);

    return len;
}

static int parse_package_5_x(u8 package_id)
{
    int i = 0;
    u8 *data = NULL;
    u32 len = 0;
    u8 check_sum =0;
    u32 x_tmp = 0, y_tmp = 0;
    struct ilitek_report *p_rpt = g_ilitek_ts->rpt;

    if (IS_ERR_OR_NULL(p_rpt) ||
        IS_ERR_OR_NULL(p_rpt->package_data) ||
        p_rpt->package_len < ILITEK_PACKAGE_HEAD_LEN) {
        ilitek_err("package data wrong\n");
        return -EINVAL;
    }

    data = p_rpt->package_data;
    len = p_rpt->package_len;

    for (i = 0; i < ILITEK_PACKAGE_HEAD_LEN; i++) {
        ilitek_debug(DEBUG_FINGER_REPORT, "data[%d] = %x\n", i, data[i]);
    }

    check_sum = calc_checksum(data, len - 1);
    ilitek_debug(DEBUG_FINGER_REPORT, "data = %x;  check_sum = %x\n", data[len - 1], check_sum);

    if (data[len - 1] != check_sum) {
        ilitek_err("package checksum wrong, check_usm = %d, data[len - 1] = %d\n",
            check_sum, data[len - 1]);
        return -EINVAL;
    }

    for (i = 0; i < ILITEK_TOUCSCEEN_FINGERS; i++) {
        p_rpt->fingers[i].x = 0;
        p_rpt->fingers[i].y = 0;
        p_rpt->fingers[i].id = ILITEK_TOUCSCEEN_FINGERS;
        p_rpt->fingers[i].pressure = 0;
    }

    if (package_id == g_ilitek_pro->modes[ILITEK_DEMO_MODE].package_id) {
        ilitek_debug(DEBUG_FINGER_REPORT, " **** parsing demo packets : 0x%x ****\n", package_id);

        for (i = 0; i < ILITEK_TOUCSCEEN_FINGERS; i++) {
            if ((data[(4 * i) + 1] == 0xFF) &&
                (data[(4 * i) + 2] && 0xFF) &&
                (data[(4 * i) + 3] == 0xFF)) {
                continue;
            }

            x_tmp = (((data[(4 * i) + 1] & 0xF0) << 4) | (data[(4 * i) + 2]));
            y_tmp = (((data[(4 * i) + 1] & 0x0F) << 8) | (data[(4 * i) + 3]));

            if (g_ilitek_ts->use_ic_res) {
                p_rpt->fingers[i].x = x_tmp;
                p_rpt->fingers[i].y = y_tmp;
                p_rpt->fingers[i].id = i;
            } else {
                p_rpt->fingers[i].x = x_tmp * g_ilitek_ts->x_max / TPD_WIDTH;
                p_rpt->fingers[i].y = y_tmp * g_ilitek_ts->y_max / TPD_HEIGHT;
                p_rpt->fingers[i].id = i;
            }

            if (g_ilitek_ts->support_pressure) {
                p_rpt->fingers[i].pressure = data[(4 * i) + 4];
            } else {
                p_rpt->fingers[i].pressure = 1;
            }

            ilitek_debug(DEBUG_FINGER_REPORT, "[x, y] = [%d, %d]\n", x_tmp, y_tmp);
            ilitek_debug(DEBUG_FINGER_REPORT, "point[%d] = [%d, %d] = %d\n",
                p_rpt->fingers[i].id,
                p_rpt->fingers[i].x,
                p_rpt->fingers[i].y,
                p_rpt->fingers[i].pressure);

            p_rpt->cur_finger_num++;
        }
    }else if (package_id == g_ilitek_pro->modes[ILITEK_DEBUG_MODE].package_id) {
        ilitek_debug(DEBUG_FINGER_REPORT, " **** parsing debug packets : 0x%x ****\n", package_id);
        ilitek_debug(DEBUG_FINGER_REPORT, "length = %d\n", (data[1] << 8 | data[2]));

        for (i = 0; i < ILITEK_TOUCSCEEN_FINGERS; i++) {
            if ((data[(3 * i) + 5] == 0xFF) &&
                (data[(3 * i) + 6] && 0xFF) &&
                (data[(3 * i) + 7] == 0xFF)) {
                continue;
            }

            x_tmp = (((data[(3 * i) + 5] & 0xF0) << 4) | (data[(3 * i) + 6]));
            y_tmp = (((data[(3 * i) + 5] & 0x0F) << 8) | (data[(3 * i) + 7]));

            if (g_ilitek_ts->use_ic_res) {
                p_rpt->fingers[i].x = x_tmp;
                p_rpt->fingers[i].y = y_tmp;
                p_rpt->fingers[i].id = i;
            } else {
                p_rpt->fingers[i].x = x_tmp * g_ilitek_ts->x_max / TPD_WIDTH;
                p_rpt->fingers[i].y = y_tmp * g_ilitek_ts->y_max / TPD_HEIGHT;
                p_rpt->fingers[i].id = i;
            }

            if (g_ilitek_ts->support_pressure) {
                p_rpt->fingers[i].pressure = data[(4 * i) + 4];
            } else {
                p_rpt->fingers[i].pressure = 1;
            }

            ilitek_debug(DEBUG_FINGER_REPORT, "[x, y] = [%d, %d]\n", x_tmp, y_tmp);
            ilitek_debug(DEBUG_FINGER_REPORT, "point[%d] = [%d, %d] = %d\n",
                p_rpt->fingers[i].id,
                p_rpt->fingers[i].x,
                p_rpt->fingers[i].y,
                p_rpt->fingers[i].pressure);

            p_rpt->cur_finger_num++;
        }
    }else {
        if (package_id != 0) {
            /* ignore the pid with 0x0 after enable irq at once */
            ilitek_err(" **** unknown package id : 0x%x ****\n", package_id);
            return -EINVAL;
        }
    }

    return 0;
}

static int mode_control_5_x(pro_fw_modes mode, u8 *data)
{
    int i = 0;
    int ret = 0;
    int checksum = 0;
    int codeLength = 8;
    u8 mp_code[8] = { 0 };
    u8 cmd[4] = { 0 };
    struct ilitek_report *p_rpt = g_ilitek_ts->rpt;

    switch(mode) {
    case ILITEK_I2C_UART_MODE:
        if (IS_ERR_OR_NULL(data)) {
            ilitek_err("params from user space invaild\n");
            ret = -EINVAL;
            goto err_out;
        }

        cmd[0] = g_ilitek_pro->cmd_i2cuart;
        cmd[1] = data[1];
        cmd[2] = data[2];

        ilitek_debug(DEBUG_FINGER_REPORT, "switch to i2cuart mode, cmd = %x, b1 = %x, b2 = %x\n",
            cmd[0], cmd[1], cmd[2]);

        ret = ilitek_i2c_write(cmd, 3);
        if (ret) {
            ilitek_err("i2c write cmd failed");
            goto err_out;
        }

        ilitek_info("switch i2cuart mode succeeded\n");

        break;

    case ILITEK_DEMO_MODE:
    case ILITEK_DEBUG_MODE:
        cmd[0] = g_ilitek_pro->cmd_mode_ctrl;
        cmd[1] = g_ilitek_pro->modes[mode].mode;

        ilitek_debug(DEBUG_FINGER_REPORT, "switch to demo/debug mode, cmd = 0x%x, b1 = 0x%x\n",
            cmd[0], cmd[1]);

        ret = ilitek_i2c_write(cmd, 2);
        if (ret) {
            ilitek_err("i2c write cmd failed\n");
            goto err_out;
        }

        p_rpt->fw_mode = mode;

        ilitek_info("switch demo/debug mode succeeded\n");

        break;

    case ILITEK_TEST_MODE:
        cmd[0] = g_ilitek_pro->cmd_mode_ctrl;
        cmd[1] = g_ilitek_pro->modes[mode].mode;

        ilitek_debug(DEBUG_FINGER_REPORT, "switch to test mode, cmd = 0x%x, b1 = 0x%x\n",
            cmd[0], cmd[1]);

        ret = ilitek_i2c_write(cmd, 2);
        if (ret) {
            ilitek_err("i2c write cmd failed\n");
            goto err_out;
        }

        cmd[0] = 0xFE;

        /* Read MP Test information to ensure if fw supports test mode. */
        ret = ilitek_i2c_write(cmd, 1);
        if (ret) {
            ilitek_err("i2c write cmd failed\n");
            goto err_out;
        }

        mdelay(10);

        ret = ilitek_i2c_read(mp_code, codeLength);
        if (ret) {
            ilitek_err("i2c read mp_code failed\n");
            goto err_out;
        }

        for (i = 0; i < codeLength - 1; i++)
            checksum += mp_code[i];

        if ((-checksum & 0xFF) != mp_code[codeLength - 1]) {
            ilitek_info("checksume error (0x%x), fw doesn't support test mode\n",
                    (-checksum & 0XFF));
            ret = -EINVAL;
            goto err_out;
        }

        /* FW enter to Test Mode */
        ret = core_mp_move_code();
        if (ret) {
            ilitek_err("switch test mode failed\n");
            goto err_out;
        }

        p_rpt->fw_mode = mode;

        ilitek_info("switch test mode succeeded\n");
        break;
    default:
        ret = -EINVAL;
        ilitek_err("unknown firmware mode: %x\n", mode);
        break;
    }

    return 0;

err_out:
    return ret;
}

static void test_init_5_x(struct protocol_test *p_test)
{
    /* The commands about MP test */
    p_test->cmd_cdc = P5_X_SET_CDC_INIT;
    p_test->cmd_get_cdc = P5_X_GET_CDC_DATA;

    p_test->cdc_len = 15;

    p_test->mutual_dac = 0x1;
    p_test->mutual_bg = 0x2;
    p_test->mutual_signal = 0x3;
    p_test->mutual_no_bk = 0x5;
    p_test->mutual_has_bk = 0x8;
    p_test->mutual_bk_dac = 0x10;

    p_test->self_dac = 0xC;
    p_test->self_bg = 0xF;
    p_test->self_signal = 0xD;
    p_test->self_no_bk = 0xE;
    p_test->self_has_bk = 0xB;
    p_test->self_bk_dac = 0x11;

    p_test->key_dac = 0x14;
    p_test->key_bg = 0x16;
    p_test->key_no_bk = 0x7;
    p_test->key_has_bk = 0x15;
    p_test->key_open = 0x12;
    p_test->key_short = 0x13;

    p_test->st_dac = 0x1A;
    p_test->st_bg = 0x1C;
    p_test->st_no_bk = 0x17;
    p_test->st_has_bk = 0x1B;
    p_test->st_open = 0x18;

    p_test->tx_short = 0x19;

    p_test->rx_short = 0x4;
    p_test->rx_open = 0x6;

    p_test->tx_rx_delta = 0x1E;

    p_test->cm_data = 0x9;
    p_test->cs_data = 0xA;

    p_test->trcrq_pin = 0x20;
    p_test->resx2_pin = 0x21;
    p_test->mutual_integra_time = 0x22;
    p_test->self_integra_time = 0x23;
    p_test->key_integra_time = 0x24;
    p_test->st_integra_time = 0x25;
    p_test->peak_to_peak = 0x1D;

    p_test->get_timing = 0x30;
    p_test->doze_p2p = 0x32;
    p_test->doze_raw = 0x33;
}

int ilitek_protocol_update(struct ilitek_version *p_target_ver)
{
    int index = 0;
    struct ilitek_version *p_ver = NULL;

    if (IS_ERR_OR_NULL(p_target_ver)) {
        ilitek_err("p_target_ver is null\n");
        return -EINVAL;
    }

    if (g_ilitek_pro->ver &&
        g_ilitek_pro->ver->data[0] == p_target_ver->data[0] &&
        g_ilitek_pro->ver->data[1] == p_target_ver->data[1] &&
        g_ilitek_pro->ver->data[2] == p_target_ver->data[2]) {
        ilitek_info("same verison, no need to update\n");
        return 0;
    }

    for (index = 0; index < ILITEK_VER_NUMS; index++) {
        p_ver = &pro_versions[index];
        if (p_target_ver->data[0] == p_ver->data[0] &&
            p_target_ver->data[1] == p_ver->data[1] &&
            p_target_ver->data[2] == p_ver->data[2]) {
            break;
        }
    }

    /* protocol 5_X difference */
    switch(index) {
    case ILITEK_VER_5_0_0:
        g_ilitek_pro->ctrls = pro_5_0_ctrls;
        g_ilitek_pro->infos = pro_5_x_infos;
        g_ilitek_pro->infos[ILITEK_FW_INFO].len = 4;
        g_ilitek_pro->infos[ILITEK_PRO_INFO].len = 3;
        break;
    case ILITEK_VER_5_1_0:
        g_ilitek_pro->ctrls = pro_5_x_ctrls;
        g_ilitek_pro->infos = pro_5_x_infos;
        g_ilitek_pro->infos[ILITEK_FW_INFO].len = 4;
        g_ilitek_pro->infos[ILITEK_PRO_INFO].len = 3;
        break;
    case ILITEK_VER_5_2_0:
        g_ilitek_pro->ctrls = pro_5_x_ctrls;
        g_ilitek_pro->infos = pro_5_x_infos;
        g_ilitek_pro->infos[ILITEK_FW_INFO].len = 4;
        g_ilitek_pro->infos[ILITEK_PRO_INFO].len = 4;
        break;
    case ILITEK_VER_5_3_0:
    case ILITEK_VER_5_4_0:
        g_ilitek_pro->ctrls = pro_5_x_ctrls;
        g_ilitek_pro->infos = pro_5_x_infos;
        g_ilitek_pro->infos[ILITEK_FW_INFO].len = 9;
        g_ilitek_pro->infos[ILITEK_PRO_INFO].len = 4;
        break;
    default:
        ilitek_err("driver don't support version: %s\n", p_target_ver->str);
        return -EINVAL;
    }

    /* protocol 5_X same */
    switch(index) {
    case ILITEK_VER_5_0_0:
    case ILITEK_VER_5_1_0:
    case ILITEK_VER_5_2_0:
    case ILITEK_VER_5_3_0:
    case ILITEK_VER_5_4_0:
        g_ilitek_pro->modes = pro_5_x_modes;
        g_ilitek_pro->funcs = &pro_5_x_funcs;
        g_ilitek_pro->cmd_read_ctrl = P5_X_READ_DATA_CTRL;
        g_ilitek_pro->cmd_mode_ctrl = P5_X_MODE_CONTROL;
        g_ilitek_pro->cmd_i2cuart = P5_X_I2C_UART;
        g_ilitek_pro->cmd_cdc_busy = P5_X_CDC_BUSY_STATE;
        g_ilitek_pro->addr_project_id = P5_X_PROJECT_ID_ADDR;

        test_init_5_x(g_ilitek_pro->test);
        break;
    default:
        ilitek_err("driver don't support version: %s\n", p_target_ver->str);
        return -EINVAL;
    }

    g_ilitek_pro->ver = &pro_versions[index];

    ilitek_info("update protocol: %s succeeded\n", g_ilitek_pro->ver->str);

    return 0;
}

int ilitek_protocol_init(pro_vers index)
{
    int ret = 0;

    if (index >= ILITEK_VER_NUMS) {
        ilitek_err("don't support this version: %s\n", pro_versions[index].str);
        return -EINVAL;
    }

    g_ilitek_pro = kzalloc(sizeof(*g_ilitek_pro), GFP_KERNEL);
    if (IS_ERR_OR_NULL(g_ilitek_pro)) {
        ilitek_err("alloc ilitek_protocol failed\n");
        return -ENOMEM;
    }

    g_ilitek_pro->test = kzalloc(sizeof(*g_ilitek_pro->test), GFP_KERNEL);
    if (IS_ERR_OR_NULL(g_ilitek_pro->test)) {
        ilitek_err("alloc protocol_test failed\n");
        ret = -ENOMEM;
        goto err_free_pro;
    }

    ret = ilitek_protocol_update(&pro_versions[index]);
    if (ret) {
        ilitek_err("update protocol: %s failed\n", g_ilitek_pro->ver->str);
        goto err_free_test;
    }

    g_ilitek_ts->pro = g_ilitek_pro;

    ilitek_info("init protocol: %s succeeded\n", g_ilitek_pro->ver->str);

    return 0;

err_free_test:
    if (g_ilitek_pro->test) {
        kfree(g_ilitek_pro->test);
        g_ilitek_pro->test = NULL;
    }
err_free_pro:
    if (g_ilitek_pro) {
        kfree(g_ilitek_pro);
        g_ilitek_pro = NULL;
    }
    return ret;
}

void ilitek_protocol_exit(void)
{
    if (g_ilitek_pro->test) {
        kfree(g_ilitek_pro->test);
        g_ilitek_pro->test = NULL;
    }

    if (g_ilitek_pro) {
        kfree(g_ilitek_pro);
        g_ilitek_pro = NULL;
    }

    g_ilitek_ts->pro = NULL;

    ilitek_info("protocol exit succeeded\n");
}
