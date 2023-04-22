#include "ilitek_common.h"
#include "ilitek_dts.h"
#include "ilitek_protocol.h"
#include "ilitek_config.h"
#include "ilitek_flash.h"
#include "ilitek_mp_test.h"

/* the list of support chip */
/*
 * Different ICs may require different delay time for the reset.
 * They may also depend on what your platform need to.
 */
static struct ilitek_chip_info g_chip_list[ILITEK_IC_NUMS] = {
    CONFIG_CHIP_INFO(ILITEK_ILI7807, ILI7807_WDT_ADDR, ILI7807_PID_ADDR, ILI7807_ICE_MODE_ADDR,
        ILI7807_SLAVE_ADDR, 10, 5, 200),
    CONFIG_CHIP_INFO(ILITEK_ILI9881, ILI9881_WDT_ADDR, ILI9881_PID_ADDR, ILI9881_ICE_MODE_ADDR,
        ILI9881_SLAVE_ADDR, 10, 5, 100),
};

static struct ilitek_config * g_ilitek_cfg = NULL;

void ilitek_config_disable_report_irq(void)
{
    ilitek_debug(DEBUG_CONFIG, "disable report irq data\n");

    mutex_lock(&g_ilitek_ts->ilitek_report_irq);
    if (g_ilitek_ts->isEnableFR) {
        g_ilitek_ts->isEnableFR = false;
        ilitek_debug(DEBUG_CONFIG, "report irq status: %d\n", g_ilitek_ts->isEnableFR);
    } else {
        ilitek_debug(DEBUG_CONFIG, "report already disabled\n");
    }
    mutex_unlock(&g_ilitek_ts->ilitek_report_irq);
}

void ilitek_config_enable_report_irq(void)
{
    ilitek_debug(DEBUG_CONFIG, "enable report irq data\n");

    mutex_lock(&g_ilitek_ts->ilitek_report_irq);
    if (g_ilitek_ts->isEnableFR) {
        ilitek_debug(DEBUG_CONFIG, "report already enabled\n");
    } else {
        g_ilitek_ts->isEnableFR = true;
        ilitek_debug(DEBUG_CONFIG, "report irq status: %d\n", g_ilitek_ts->isEnableFR);
    }
    mutex_unlock(&g_ilitek_ts->ilitek_report_irq);
}

int ilitek_config_check_int_status(bool high)
{
    u32 debug_pccont = 0;
    int timer = 1000;
    int irq_gpio = g_ilitek_ts->ts_dev_data->ts_platform_data->irq_gpio;

    /* From FW request, timeout should at least be 5 sec */
    while (timer) {
        if(high) {
            if (gpio_get_value(irq_gpio)) {
                ilitek_info("check busy is free\n");
                return 0;
            }
        } else {
            if (!gpio_get_value(irq_gpio)) {
                ilitek_info("check busy is free\n");
                return 0;
            }
        }

        mdelay(5);
        timer--;
    }

    /* debug: dump reg in timeout */
    ilitek_config_ice_mode_enable();
    debug_pccont = ilitek_config_ice_mode_read(ILITEK_DEBUG_REG_PC_CONT);
    ilitek_info("read reg[0x%x], data = 0x%x\n", ILITEK_DEBUG_REG_PC_CONT, debug_pccont);
    ilitek_config_ice_mode_disable();

    ilitek_info("check busy timeout !!\n");

    return -EIO;
}

int ilitek_i2c_read(u8 *buf, u16 len)
{
    int ret = 0;
    struct ts_kit_device_data *dev_data = g_ilitek_ts->ts_dev_data;

    if (!dev_data->ts_platform_data->bops->bus_read) {
        ilitek_err("ts kit i2c read is null\n");
        return -ENODEV;
    }

    ret = dev_data->ts_platform_data->bops->bus_read(NULL, 0, buf, len);

    return ret;
}

int ilitek_i2c_write(u8 *buf, u16 len)
{
    int ret = 0;
    u8 check_sum = 0;
    u8 *tmp_buf = NULL;
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;
    struct ts_kit_device_data *dev_data = g_ilitek_ts->ts_dev_data;

    if (!dev_data->ts_platform_data->bops->bus_write) {
        ilitek_err("ts kit i2c write is null\n");
        return -ENODEV;
    }

    /*
     * NOTE: If TP driver is doing MP test and commanding 0xF1 to FW, we add a checksum
     * to the last index and plus 1 with size.
     */
    if (p_pro->ver->data[0] >= 5 && p_pro->ver->data[1] >= 4) {
        if (!g_ilitek_cfg->ice_mode_enable && buf[0] == 0xF1 && core_mp->run) {
            check_sum = calc_checksum(buf, len);
            tmp_buf = kzalloc(len + 1, GFP_KERNEL);
            if (IS_ERR_OR_NULL(tmp_buf)) {
                ilitek_err("alloc tmp_buf failed\n");
                return -ENOMEM;
            }
            memcpy(tmp_buf, buf, len);
            tmp_buf[len] = check_sum;
            ret = dev_data->ts_platform_data->bops->bus_write(tmp_buf, len + 1);
            kfree(tmp_buf);
            return ret;
        }
    }

    ret = dev_data->ts_platform_data->bops->bus_write(buf, len);

    return ret;
}

int ilitek_i2c_write_read(u8 *cmd, u8 *buf, u16 len)
{
    int ret = 0;

    ret = ilitek_i2c_write(cmd, 2);
    if (ret) {
        ilitek_err("i2c write data failed, %d\n", ret);
        return ret;
    }

    mdelay(1);

    ret = ilitek_i2c_write(&cmd[1], 1);
    if (ret) {
        ilitek_err("i2c write data failed, %d\n", ret);
        return ret;
    }

    mdelay(1);

    ret = ilitek_i2c_read(buf, len);
    if (ret) {
        ilitek_err("i2c read data failed, %d\n", ret);
        return ret;
    }

    return 0;
}

u32 ilitek_config_ice_mode_read(u32 addr)
{
    int ret = 0;
    u32 data = 0;
    u8 buf[ILITEK_ICE_MODE_CMD_LEN] = { 0 };

    buf[0] = 0x25;
    buf[1] = (char)((addr & 0x000000FF) >> 0);
    buf[2] = (char)((addr & 0x0000FF00) >> 8);
    buf[3] = (char)((addr & 0x00FF0000) >> 16);

    ret = ilitek_i2c_write(buf, ILITEK_ICE_MODE_CMD_LEN);
    if (ret < 0) {
        ilitek_err("i2c write data failed, ret = %d\n", ret);
        return ret;
    }

    mdelay(10);

    ret = ilitek_i2c_read(buf, ILITEK_ICE_MODE_CMD_LEN);
    if (ret < 0) {
        ilitek_err("i2c read data failed, ret = %d\n", ret);
        return ret;
    }

    data = (buf[0] + buf[1] * 256 + buf[2] * 256 * 256 + buf[3] * 256 * 256 * 256);

    return data;
}

/*
 * Write commands into firmware in ICE Mode.
 *
 */
int ilitek_config_ice_mode_write(u32 addr, u32 data, u32 len)
{
    int i = 0;
    int ret = 0;
    u8 buf[ILITEK_ICE_MODE_W_MAX_LEN] = { 0 };

    buf[0] = 0x25;
    buf[1] = (char)((addr & 0x000000FF) >> 0);
    buf[2] = (char)((addr & 0x0000FF00) >> 8);
    buf[3] = (char)((addr & 0x00FF0000) >> 16);

    for (i = 0; i < len; i++) {
        buf[i + ILITEK_ICE_MODE_CMD_LEN] = (char)(data >> (8 * i));
    }

    ret = ilitek_i2c_write(buf, len + ILITEK_ICE_MODE_CMD_LEN);
    if (ret) {
        ilitek_err("i2c write data failed, ret = %d\n", ret);
        return ret;
    }

    return 0;
}

/*
 * Read & Write one byte in ICE Mode.
 */
u32 ilitek_config_read_write_onebyte(u32 addr)
{
    int ret = 0;
    u32 data = 0;
    u8 buf[ILITEK_ICE_MODE_W_MAX_LEN] = { 0 };

    buf[0] = 0x25;
    buf[1] = (char)((addr & 0x000000FF) >> 0);
    buf[2] = (char)((addr & 0x0000FF00) >> 8);
    buf[3] = (char)((addr & 0x00FF0000) >> 16);

    ret = ilitek_i2c_write(buf, ILITEK_ICE_MODE_CMD_LEN);
    if (ret){
        ilitek_err("i2c write data failed, ret = %d\n", ret);
        return ret;
    }

    mdelay(1);

    ret = ilitek_i2c_read(buf, 1);
    if (ret){
        ilitek_err("i2c read data failed, ret = %d\n", ret);
        return ret;
    }

    data = buf[0];

    return data;
}

int ilitek_config_ice_mode_disable(void)
{
    int ret = 0;
    u8 buf[ILITEK_ICE_MODE_CMD_LEN] = { 0 };
    u32 addr = g_ilitek_cfg->chip_info->ice_mode_addr;

    buf[0] = 0x1b;
    buf[1] = (char)((addr & 0x000000FF) >> 0);
    buf[2] = (char)((addr & 0x0000FF00) >> 8);
    buf[3] = (char)((addr & 0x00FF0000) >> 16);

    ret = ilitek_i2c_write(buf, ILITEK_ICE_MODE_CMD_LEN);
    if (ret) {
        ilitek_err("i2c write data failed, ret = %d\n", ret);
        return ret;
    }

    g_ilitek_cfg->ice_mode_enable = false;

    ilitek_info("ice mode disabled\n");

    return 0;
}

int ilitek_config_ice_mode_enable(void)
{
    int ret = 0;
    u8 buf[ILITEK_ICE_MODE_CMD_LEN] = { 0 };
    u32 addr = g_ilitek_cfg->chip_info->ice_mode_addr;

    buf[0] = 0x25;
    buf[1] = (char)((addr & 0x000000FF) >> 0);
    buf[2] = (char)((addr & 0x0000FF00) >> 8);
    buf[3] = (char)((addr & 0x00FF0000) >> 16);

    ret = ilitek_i2c_write(buf, ILITEK_ICE_MODE_CMD_LEN);
    if (ret) {
        ilitek_err("i2c write data failed, ret = %d\n", ret);
        return ret;
    }

    g_ilitek_cfg->ice_mode_enable = true;

    ilitek_info("ice mode enabled\n");

    return 0;
}

static int ilitek_config_common_ctrl(pro_ctrls index, u8 cmd)
{
    int ret = 0;
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;
    struct protocol_control *p_ctrl= &p_pro->ctrls[index];

    if (index >= ILITEK_CTRL_NUMS) {
        ilitek_err("don't support this control\n");
        return -EINVAL;
    }

    /* last element is used to control this func */
    if (index != ILITEK_PHONE_COVER_WINDOW_CTRL) {
        p_ctrl->cmd[p_ctrl->len - 1] = cmd;
    }

    ret = ilitek_i2c_write(p_ctrl->cmd, p_ctrl->len);
    if (ret) {
        ilitek_err("i2c write data failed, ret = %d\n", ret);
        return ret;
    }

    ilitek_debug(DEBUG_CONFIG, "func: %s, cmd = %d\n", p_ctrl->name, cmd);

    return 0;
}

int ilitek_config_sense_ctrl(cmd_types cmd)
{
    ilitek_info("sense %s\n", cmd ? "start" : "stop");

    return ilitek_config_common_ctrl(ILITEK_SENSE_CTRL, cmd);
}

int ilitek_config_sleep_ctrl(cmd_types cmd)
{
    switch(cmd) {
    case CMD_DISABLE:
        ilitek_info("sleep in\n");
        break;
    case CMD_ENABLE:
        ilitek_info("sleep out\n");
        break;
    case CMD_PT_MODE:
        ilitek_info("sleep to pt mode\n");
        break;
    default:
        ilitek_err("sleep unknown cmd: %d\n", cmd);
        return -EINVAL;
    }

    return ilitek_config_common_ctrl(ILITEK_SLEEP_CTRL, cmd);
}

int ilitek_config_glove_ctrl(cmd_types cmd)
{
    switch(cmd) {
    case CMD_DISABLE:
        ilitek_info("glove disbale\n");
        break;
    case CMD_ENABLE:
        ilitek_info("glove enable\n");
        break;
    case CMD_SEAMLESS:
        ilitek_info("glove seamless\n");
        break;
    case CMD_STATUS:
        ilitek_info("glove read status\n");
        break;
    default:
        ilitek_err("glove unknown cmd: %d\n", cmd);
        return -EINVAL;
    }

    return ilitek_config_common_ctrl(ILITEK_GLOVE_CTRL, cmd);
}

u8 ilitek_config_get_glove_status(void)
{
    int ret = 0;
    u8 glove_status = 0;

    ret = ilitek_config_glove_ctrl(CMD_STATUS);
    if (ret) {
        ilitek_err("config to read glove status failed\n");
        return ret;
    }

    ret = ilitek_i2c_read(&glove_status, 1);
    if (ret) {
        ilitek_err("i2c read data failed\n");
        return ret;
    }

    ilitek_info("glove %s\n", glove_status ? "enable" : "disable");

    return glove_status;
}

int ilitek_config_stylus_ctrl(cmd_types cmd)
{
    switch(cmd) {
    case CMD_DISABLE:
        ilitek_info("stylus disbale\n");
        break;
    case CMD_ENABLE:
        ilitek_info("stylus enable\n");
        break;
    case CMD_SEAMLESS:
        ilitek_info("stylus seamless\n");
        break;
    default:
        ilitek_err("stylus unknown cmd: %d\n", cmd);
        return -EINVAL;
    }

    return ilitek_config_common_ctrl(ILITEK_STYLUS_CTRL, cmd);
}

int ilitek_config_tp_scan_mode_ctrl(cmd_types cmd)
{
    ilitek_info("tp scan %s mode\n", cmd ? "AB" : "B");

    return ilitek_config_common_ctrl(ILITEK_SCAN_MODE_CTRL, cmd);
}

int ilitek_config_lpwg_ctrl(cmd_types cmd)
{
    ilitek_info("lpwg %s\n", cmd ? "enable" : "disable");

    return ilitek_config_common_ctrl(ILITEK_LPWG_CTRL, cmd);
}

int ilitek_config_gesture_ctrl(u8 func)
{
    u8 max_byte = 0x3F;
    u8 min_byte = 0x20;

    if (func > max_byte || func < min_byte) {
        ilitek_err("gesture ctrl invaild, 0x%x\n", func);
        return -EINVAL;
    }

    ilitek_info("gesture function = 0x%x\n", func);

    return ilitek_config_common_ctrl(ILITEK_GESTURE_CTRL, func);
}

int ilitek_config_phone_cover_ctrl(cmd_types cmd)
{
    ilitek_info("phone cover %s\n", cmd ? "enable" : "disable");

    return ilitek_config_common_ctrl(ILITEK_PHONE_COVER_CTRL, cmd);
}

int ilitek_config_finger_sense_ctrl(cmd_types cmd)
{
    ilitek_info("finger sense %s\n", cmd ? "enable" : "disable");

    return ilitek_config_common_ctrl(ILITEK_FINGER_SENSE_CTRL, cmd);
}

int ilitek_config_proximity_ctrl(cmd_types cmd)
{
    ilitek_info("proximity %s\n", cmd ? "enable" : "disable");

    return ilitek_config_common_ctrl(ILITEK_PROXIMITY_CTRL, cmd);
}

int ilitek_config_plug_ctrl(cmd_types cmd)
{
    ilitek_info("plug %s\n", cmd ? "in" : "out");
    return ilitek_config_common_ctrl(ILITEK_PLUG_CTRL, cmd);
}

int ilitek_config_set_phone_cover(u8 *pattern)
{
    int i = 0;
    u32 len = 0;
    u8 *phone_cover_window = NULL;
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;

    len = p_pro->ctrls[ILITEK_PHONE_COVER_WINDOW_CTRL].len;
    phone_cover_window = p_pro->ctrls[ILITEK_PHONE_COVER_WINDOW_CTRL].cmd;

    if (IS_ERR_OR_NULL(pattern)) {
        ilitek_err("pattern invaild\n");
        return -EINVAL;
    }

    for (i = 0; i < len; i++) {
        phone_cover_window[i + 1] = pattern[i];
    }

    ilitek_info("window: cmd = 0x%x\n", phone_cover_window[0]);
    ilitek_info("window: ul_x_l = 0x%x, ul_x_h = 0x%x\n",
        phone_cover_window[1],
        phone_cover_window[2]);
    ilitek_info("window: ul_y_l = 0x%x, ul_y_l = 0x%x\n",
        phone_cover_window[3],
        phone_cover_window[4]);
    ilitek_info("window: br_x_l = 0x%x, br_x_l = 0x%x\n",
        phone_cover_window[5],
        phone_cover_window[6]);
    ilitek_info("window: br_y_l = 0x%x, br_y_l = 0x%x\n",
        phone_cover_window[7],
        phone_cover_window[8]);

    return ilitek_config_common_ctrl(ILITEK_PHONE_COVER_WINDOW_CTRL, 0);
}

int ilitek_config_roi_ctrl(cmd_types cmd)
{
    switch(cmd) {
    case CMD_DISABLE:
        ilitek_info("roi disbale\n");
        break;
    case CMD_ENABLE:
        ilitek_info("roi enable\n");
        break;
    case CMD_STATUS:
        ilitek_info("roi read status\n");
        break;
    case CMD_ROI_DATA:
        ilitek_debug(DEBUG_CONFIG, "roi read data\n");
        break;
    default:
        ilitek_err("roi unknown cmd: %d\n", cmd);
        return -EINVAL;
    }

    return ilitek_config_common_ctrl(ILITEK_ROI_CTRL, cmd);
}

u8 ilitek_config_get_roi_status(void)
{
    int ret = 0;
    u8 roi_status = 0;

    ret = ilitek_config_roi_ctrl(CMD_STATUS);
    if (ret) {
        ilitek_err("config to read roi status failed\n");
        return ret;
    }

    ret = ilitek_i2c_read(&roi_status, 1);
    if (ret) {
        ilitek_err("i2c read data failed\n");
        return ret;
    }

    ilitek_info("roi %s\n", roi_status ? "enable" : "disable");

    return roi_status;
}

int ilitek_config_load_gesture_code(void)
{
    int res = 0, i = 0;
    u8 temp[12] = {0};
    u32 gesture_size = 0, gesture_addr = 0;
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;

    temp[0] = 0x01;
    temp[0] = 0x0A;
    temp[1] = 0x07;

    if ((ilitek_i2c_write(temp, 2)) < 0) {
        ilitek_err("write command error\n");
    }
    if ((ilitek_i2c_read(temp, 12)) < 0) {
        ilitek_err("Read command error\n");
    }

    gesture_addr = (temp[6] << 24) + (temp[7] << 16) + (temp[8] << 8) + temp[9];
    gesture_size = (temp[10] << 8) + temp[11];

    printk("gesture_addr = 0x%x, gesture_size = 0x%x\n", gesture_addr, gesture_size);

    for (i = 0; i < 12; i++) {
        printk("0x%x,", temp[i]);
    }

    printk("\n");

    for (i = 0; i < 100; i++) {
        ilitek_info("i = %d\n", i);
        temp[0] = 0x01;
        temp[1] = 0x0A;
        temp[2] = 0x00;
        if ((ilitek_i2c_write(temp, 3)) < 0) {
            ilitek_err("write command error\n");
        }

        temp[0] = p_pro->cmd_read_ctrl;
        temp[1] = p_pro->cmd_cdc_busy;

        mdelay(1);
        ilitek_i2c_write(temp, 2);
        mdelay(1);
        ilitek_i2c_write(&temp[1], 1);
        mdelay(1);
        ilitek_i2c_read(temp, 1);
        if (temp[0] == 0x41 || temp[0] == 0x51) {
            ilitek_info("Check busy is free\n");
            res = 0;
            break;
        }
    }

    if (i == 100 && temp[0] != 0x41) {
        ilitek_info("Check busy is busy\n");
    }
    /* check system busy */
    // if (ilitek_config_check_cdc_busy(50, 10) < 0)
    //     ilitek_err("Check busy is timout !\n");
    temp[0] = 0x01;
    temp[1] = 0x0A;
    temp[2] = 0x03;
    if ((ilitek_i2c_write(temp, 3)) < 0) {
        ilitek_err("write command error\n");
    }

    temp[0] = 0x01;
    temp[1] = 0x0A;
    temp[2] = 0x01;
    if ((ilitek_i2c_write(temp, 3)) < 0) {
        ilitek_err("write command error\n");
    }

    for (i = 0; i < 1000; i++) {
        temp[0] = 0x01;
        temp[1] = 0x0A;
        temp[2] = 0x05;
        if ((ilitek_i2c_write(temp, 3)) < 0) {
            ilitek_err("write command error\n");
        }
        if ((ilitek_i2c_read(temp, 1)) < 0) {
            ilitek_err("Read command error\n");
        }
        if (temp[0] == 0x1) {
            ilitek_info("check fw ready\n");
            break;
        }
    }

    if (i == 1000 && temp[0] != 0x01) {
        ilitek_err("FW is busy, error\n");
    }

    temp[0] = 0x01;
    temp[1] = 0x0A;
    temp[2] = 0x06;
    if ((ilitek_i2c_write(temp, 3)) < 0) {
        ilitek_err("write command error\n");
    }

    return res;
}

/*
 * Doing soft reset on ic.
 *
 * It resets ic's status, moves code and leave ice mode automatically if in
 * that mode.
 */
int ilitek_config_ic_reset(void)
{
    int ret = 0;
    u32 key = 0;

    if (g_ilitek_cfg->chip_info->chip_id == ILITEK_ILI7807) {
        if (g_ilitek_cfg->chip_info->chip_type == ILI7807_TYPE_H) {
            key = 0x00117807;
        } else {
            key = 0x00017807;
        }
    } else if (g_ilitek_cfg->chip_info->chip_id == ILITEK_ILI9881) {
        key = 0x00019881;
    }

    ilitek_debug(DEBUG_CONFIG, "key = 0x%x\n", key);

    if (key == 0) {
        ilitek_err("ic reset key invaild\n");
        return -EINVAL;
    }

    ret = ilitek_config_ice_mode_write(g_ilitek_cfg->chip_info->ic_reset_addr,
            key, ILITEK_ICE_MODE_CMD_LEN);
    if (ret) {
        ilitek_err("ic write data failed\n");
        return -EIO;
    }

    msleep(300);

    ilitek_info("ic soft reset succeeded\n");

    return 0;
}

int ilitek_config_set_watch_dog(bool enable)
{
    int timeout = 10, ret = 0;
    u8 off_bit = 0x5A, on_bit = 0xA5;
    u8 value_low = 0x0, value_high = 0x0;
    u32 wdt_addr = g_ilitek_cfg->chip_info->wdt_addr;

    if (wdt_addr <= 0 || g_ilitek_cfg->chip_info->chip_id <= 0) {
        ilitek_err("wdt/chip id is invalid\n");
        return -EINVAL;
    }

    /* Config register and values by IC */
    if (g_ilitek_cfg->chip_info->chip_id == ILITEK_ILI7807) {
        value_low = 0x07;
        value_high = 0x78;
    } else if (g_ilitek_cfg->chip_info->chip_id == ILITEK_ILI9881 ) {
        value_low = 0x81;
        value_high = 0x98;
    } else {
        ilitek_err("unknown chip type (0x%x)\n", g_ilitek_cfg->chip_info->chip_id);
        return -ENODEV;
    }

    if (enable) {
        ilitek_config_ice_mode_write(wdt_addr, 1, 1);
    } else {
        ilitek_config_ice_mode_write(wdt_addr, value_low, 1);
        ilitek_config_ice_mode_write(wdt_addr, value_high, 1);
    }

    while (timeout > 0) {
        ret = ilitek_config_ice_mode_read(0x51018);
        ilitek_debug(DEBUG_CONFIG, "bit = %x\n", ret);

        if (enable) {
            if (CHECK_EQUAL(ret, on_bit) == 0)
                break;
        } else {
            if (CHECK_EQUAL(ret, off_bit) == 0)
                break;
        }

        timeout--;
        mdelay(10);
    }

    if (timeout > 0) {
        if (enable) {
            ilitek_info("wdt turn on succeed\n");
        } else {
            ilitek_config_ice_mode_write(wdt_addr, 0, 1);
            ilitek_info("wdt turn off succeed\n");
        }
    } else {
        ilitek_err("wdt turn on/off timeout !\n");
        return -EINVAL;
    }

    return 0;
}

#define ILITEK_CMD_CDC_DEBUG                   0xF8
int ilitek_config_check_cdc_busy(int retrys, int delay)
{
    int i = 1;
    u8 cmd[2] = { 0 };
    u8 debug_cmd[2] = { 0 };
    u8 busy = 0, debug_data = 0;
    u32 debug_pccont = 0;
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;

    cmd[0] = p_pro->cmd_read_ctrl;
    cmd[1] = p_pro->cmd_cdc_busy;

    debug_cmd[0] = p_pro->cmd_read_ctrl;
    debug_cmd[1] = ILITEK_CMD_CDC_DEBUG;

    ilitek_info("check cdc busy start, retrys = %d, delay = %d\n", retrys, delay);
    do {
        ilitek_i2c_write_read(cmd, &busy, 1);
        ilitek_debug(DEBUG_CONFIG, "cdc busy state = 0x%x\n", busy);

        if (ilitek_debug_level & DEBUG_CONFIG) {
            ilitek_i2c_write_read(debug_cmd, &debug_data, 1);
            ilitek_debug(DEBUG_CONFIG, "cdc f8 data = 0x%x\n", debug_data);
        }

        if (busy == 0x41 || busy == 0x51) {
            ilitek_info("check busy is free\n");
            return 0;
        }
        mdelay(delay);
    } while (i++ <= retrys);

    /* debug: dump reg in timeout */
    ilitek_config_ice_mode_enable();
    debug_pccont = ilitek_config_ice_mode_read(ILITEK_DEBUG_REG_PC_CONT);
    ilitek_info("read reg[0x%x], data = 0x%x\n", ILITEK_DEBUG_REG_PC_CONT, debug_pccont);
    ilitek_config_ice_mode_disable();

    ilitek_info("check busy timeout !!\n");
    return -EIO;
}

/*
 * ic_suspend: Get IC to suspend called from system.
 *
 * The timing when goes to sense stop or how much times the command need to be called
 * is depending on customer's system requirement, which might be different due to
 * the DDI design or other conditions.
 */
void ilitek_config_ic_suspend(void)
{
    /* sense stop */
    //ilitek_config_sense_ctrl(CMD_DISABLE);

    /* check system busy */
    //if (ilitek_config_check_cdc_busy(50, 10) < 0) {
    //    ilitek_err("check busy is timout\n");
    //}
    if (g_tskit_pt_station_flag) {
        ilitek_config_sleep_ctrl(CMD_PT_MODE);
        ilitek_info("send sleep pt cmd in power test mode\n");
    } else {
        /* sleep in */
        ilitek_config_sleep_ctrl(CMD_DISABLE);
    }
}

/*
 * ic_resume: Get IC to resume called from system.
 *
 * The timing when goes to sense start or how much times the command need to be called
 * is depending on customer's system requirement, which might be different due to
 * the DDI design or other conditions.
 */
void ilitek_config_ic_resume(void)
{
    /* sleep out */
    ilitek_config_sleep_ctrl(CMD_ENABLE);

    /* check system busy */
    //if (ilitek_config_check_cdc_busy(50, 10) < 0)
    //    ilitek_err("Check busy is timout !\n");

    /* sense start for TP */
    ilitek_config_sense_ctrl(CMD_ENABLE);
}

int ilitek_config_into_easy_wakeup(void)
{
    struct ts_kit_device_data *ts_dev_data = g_ilitek_ts->ts_dev_data;
    struct ts_easy_wakeup_info *info = &ts_dev_data->easy_wakeup_info;

    ilitek_debug(DEBUG_CONFIG, "easy_wakeup_flag = %d\n",info->easy_wakeup_flag);

    /*if the sleep_gesture_flag is ture,it presents that  the tp is at sleep state*/
    if (ts_dev_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value == false ||
        true == info->easy_wakeup_flag) {
        ilitek_info("easy_wakeup_flag = %d\n", info->easy_wakeup_flag);
        return 0;
    }

    /* sense stop */
    //ilitek_config_sense_ctrl(CMD_DISABLE);

    /* check system busy */
    //if (ilitek_config_check_cdc_busy(50, 10) < 0) {
    //    ilitek_err("check busy is timout\n");
    //}

    ilitek_config_lpwg_ctrl(CMD_ENABLE);

    enable_irq_wake(ts_dev_data->ts_platform_data->irq_id);
    info->easy_wakeup_flag = true;

    return 0;
}

int ilitek_config_outof_easy_wakeup(void)
{
    struct ts_kit_device_data *ts_dev_data = g_ilitek_ts->ts_dev_data;
    struct ts_easy_wakeup_info *info = &ts_dev_data->easy_wakeup_info;

    ilitek_debug(DEBUG_CONFIG, "easy_wakeup_flag = %d\n",info->easy_wakeup_flag);

    info->easy_wakeup_flag = false;

    return 0;
}

void ilitek_parse_protocol_verison(u8 *buf, u16 len)
{
    int i = 0, j = 0;
    int ret = 0;

    /* in protocol v5, ignore the first btye because of a header. */
    for (i = 1,j = 0; i < len; i++,j++) {
        g_ilitek_cfg->pro_ver.data[j] = buf[i];
    }

    sprintf(g_ilitek_cfg->pro_ver.str, "%d.%d.%d",
         g_ilitek_cfg->pro_ver.data[0],
         g_ilitek_cfg->pro_ver.data[1],
         g_ilitek_cfg->pro_ver.data[2]);

    ilitek_info("procotol version = %s\n", g_ilitek_cfg->pro_ver.str);

    ret = ilitek_protocol_update(&g_ilitek_cfg->pro_ver);
    if (ret) {
        ilitek_err("update protocol version failed\n");
    }
}

void ilitek_parse_core_version(u8 *buf, u16 len)
{
    int i = 0, j = 0;

    /* in protocol v5, ignore the first btye because of a header. */
    for (i = 1,j = 0; i < len; i++,j++) {
        g_ilitek_cfg->core_ver.data[j] = buf[i];
    }

    sprintf(g_ilitek_cfg->core_ver.str, "%d.%d.%d.%d",
         g_ilitek_cfg->core_ver.data[0],
         g_ilitek_cfg->core_ver.data[1],
         g_ilitek_cfg->core_ver.data[2],
         g_ilitek_cfg->core_ver.data[3]);

    ilitek_info("core version = %s\n", g_ilitek_cfg->core_ver.str);
}

/*
 * Getting the version of firmware used on the current one.
 *
 */
void ilitek_parse_firmware_version(u8 *buf, u16 len)
{
    int i = 0, j = 0;
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;
    struct ts_kit_device_data *ts_dev_data = g_ilitek_ts->ts_dev_data;

    /* in protocol v5, ignore the first btye because of a header. */
    for (i = 1,j = 0; i < len; i++,j++) {
        g_ilitek_cfg->fw_ver.data[j] = buf[i];
    }

    if (p_pro->ver->data[1] >= 3) {
        sprintf(g_ilitek_cfg->fw_ver.str, "%d.%d.%d.%d",
             g_ilitek_cfg->fw_ver.data[0],
             g_ilitek_cfg->fw_ver.data[1],
             g_ilitek_cfg->fw_ver.data[2],
             g_ilitek_cfg->fw_ver.data[3]);
    } else {
        sprintf(g_ilitek_cfg->fw_ver.str, "%d.%d.%d",
             g_ilitek_cfg->fw_ver.data[0],
             g_ilitek_cfg->fw_ver.data[1],
             g_ilitek_cfg->fw_ver.data[2]);
    }

    snprintf(ts_dev_data->version_name, MAX_STR_LEN - 1, "%s", g_ilitek_cfg->fw_ver.str);

    ilitek_info("firmware version = %s\n", g_ilitek_cfg->fw_ver.str);
}

void ilitek_parse_tp_info(u8 *buf, u16 len)
{
    struct ilitek_tp_info *p_tp_info = &g_ilitek_cfg->tp_info;

    /* in protocol v5, ignore the first btye because of a header. */
    p_tp_info->nMinX = buf[1];
    p_tp_info->nMinY = buf[2];
    p_tp_info->nMaxX = (buf[4] << 8) + buf[3];
    p_tp_info->nMaxY = (buf[6] << 8) + buf[5];
    p_tp_info->nXChannelNum = buf[7];
    p_tp_info->nYChannelNum = buf[8];
    p_tp_info->self_tx_channel_num = buf[11];
    p_tp_info->self_rx_channel_num = buf[12];
    p_tp_info->side_touch_type = buf[13];
    p_tp_info->nMaxTouchNum = buf[9];
    p_tp_info->nKeyCount = buf[10];

    p_tp_info->nMaxKeyButtonNum = 5;

    ilitek_info("minX = %d, minY = %d, maxX = %d, maxY = %d\n",
         p_tp_info->nMinX,
         p_tp_info->nMinY,
         p_tp_info->nMaxX,
         p_tp_info->nMaxY);
    ilitek_info("xchannel = %d, ychannel = %d, self_tx = %d, self_rx = %d\n",
         p_tp_info->nXChannelNum,
         p_tp_info->nYChannelNum,
         p_tp_info->self_tx_channel_num,
         p_tp_info->self_rx_channel_num);
    ilitek_info("side_touch_type = %d, max_touch_num= %d, touch_key_num = %d, max_key_num = %d\n",
         p_tp_info->side_touch_type,
         p_tp_info->nMaxTouchNum,
         p_tp_info->nKeyCount,
         p_tp_info->nMaxKeyButtonNum);

}

void ilitek_parse_key_info(u8 *buf, u16 len)
{
    int i = 0;
    struct ilitek_tp_info *p_tp_info = &g_ilitek_cfg->tp_info;

    if (p_tp_info->nKeyCount) {
        /* NOTE: Firmware not ready yet */
        p_tp_info->nKeyAreaXLength = (buf[0] << 8) + buf[1];
        p_tp_info->nKeyAreaYLength = (buf[2] << 8) + buf[3];

        ilitek_info("key: length of X area = %x\n", p_tp_info->nKeyAreaXLength);
        ilitek_info("key: length of Y area = %x\n", p_tp_info->nKeyAreaYLength);

        for (i = 0; i < p_tp_info->nKeyCount; i++) {
            p_tp_info->vkeys[i].nId = buf[i * 5 + 4];
            p_tp_info->vkeys[i].nX = (buf[i * 5 + 5] << 8) + buf[i * 5 + 6];
            p_tp_info->vkeys[i].nY = (buf[i * 5 + 7] << 8) + buf[i * 5 + 8];
            p_tp_info->vkeys[i].nStatus = 0;

            ilitek_info("key: id = %d, X = %d, Y = %d\n",
                 p_tp_info->vkeys[i].nId,
                 p_tp_info->vkeys[i].nX,
                 p_tp_info->vkeys[i].nY);
        }
    }
}

int ilitek_config_read_ic_info(pro_ic_infos index)
{
    int ret = 0;
    u8 cmd[2] = { 0 };
    u8 *buf = NULL;
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;
    struct protocol_ic_info * p_ic_info = &p_pro->infos[index];

    if (IS_ERR_OR_NULL(p_ic_info) ||
        IS_ERR_OR_NULL(p_ic_info->parse_info) ||
        p_ic_info->len <= 0 ) {
        ilitek_err("get ic info failed\n");
        return -EINVAL;
    }

    buf = kzalloc(p_ic_info->len, GFP_KERNEL);
    if (IS_ERR_OR_NULL(buf)) {
        ilitek_err("alloc buf failed\n");
        return -ENOMEM;
    }

    cmd[0] = p_pro->cmd_read_ctrl;
    cmd[1] = p_ic_info->cmd;

    ret = ilitek_i2c_write_read(cmd, buf, p_ic_info->len);
    if (ret < 0) {
        ilitek_err("i2c write read data failed, %d\n", ret);
        goto err_i2c;
    }

    p_ic_info->parse_info(buf, p_ic_info->len);

err_i2c:
    if (buf) {
        kfree(buf);
    }

    return ret;
}

/*
 * It checks chip id shifting sepcific bits based on chip's requirement.
 *
 * @pid_data: 4 bytes, reading from firmware.
 *
 */
static void ilitek_check_chip_id(u32 pid_data)
{
    int i;
    u32 id = 0;
    u32 type = 0;

    id = pid_data >> 16;
    type = (pid_data & 0x0000FF00) >> 8;

    ilitek_info("id = 0x%x, type = 0x%x\n", id, type);

    if(id == ILITEK_ILI9881) {
        for(i = ILI9881_TYPE_F; i <= ILI9881_TYPE_H; i++) {
            if (i == type) {
                g_ilitek_cfg->chip_info->chip_type = i;
                g_ilitek_cfg->chip_info->ic_reset_addr = 0x040050;
            }
        }
    }

    if(id == ILITEK_ILI7807) {
        for(i = ILI7807_TYPE_F_AA; i <= ILI7807_TYPE_H; i++) {
            if (i == type) {
                g_ilitek_cfg->chip_info->chip_type = i;
                if (i == ILI7807_TYPE_F_AB)
                    g_ilitek_cfg->chip_info->ic_reset_addr = 0x04004C;
                else if (i == ILI7807_TYPE_H)
                    g_ilitek_cfg->chip_info->ic_reset_addr = 0x040050;
            }
        }
    }
}


int ilitek_config_get_project_id(void)
{
    int i = 0;
    int ret = 0;
    u8 buf[ILITEK_PROJECT_ID_LEN] = {0};
    u32 addr = g_ilitek_ts->pro->addr_project_id;

    ret = ilitek_config_ice_mode_enable();
    if (ret) {
        ilitek_err("enter ice mode failed, ret = %d\n", ret);
        return -EIO;
    }

    ilitek_config_ice_mode_write(0x041000, 0x0, 1);   /* CS low */
    ilitek_config_ice_mode_write(0x041004, 0x66aa55, 3);  /* Key */

    ilitek_config_ice_mode_write(0x041008, 0x06, 1);
    ilitek_config_ice_mode_write(0x041000, 0x01, 1);
    ilitek_config_ice_mode_write(0x041000, 0x00, 1);
    ilitek_config_ice_mode_write(0x041004, 0x66aa55, 3);  /* Key */
    ilitek_config_ice_mode_write(0x041008, 0x03, 1);

    ilitek_config_ice_mode_write(0x041008, (addr & 0xFF0000) >> 16, 1);
    ilitek_config_ice_mode_write(0x041008, (addr & 0x00FF00) >> 8, 1);
    ilitek_config_ice_mode_write(0x041008, (addr & 0x0000FF), 1);

    for(i = 0; i < ILITEK_PROJECT_ID_LEN; i++) {
        ilitek_config_ice_mode_write(0x041008, 0xFF, 1);
        buf[i] = ilitek_config_ice_mode_read(0x41010);
        ilitek_debug(DEBUG_CONFIG, "buf[%d] = 0x%x\n", i, buf[i]);
    }

    ilitek_config_ice_mode_write(0x041010, 0x1, 0);   /* CS high */
    ilitek_chip_reset();

    memcpy(g_ilitek_ts->project_id, buf, ILITEK_PROJECT_ID_LEN);

    if (g_ilitek_ts->project_id_length_control) {
        g_ilitek_ts->project_id[ILITEK_PROJECT_ID_LEN - 1] = '\0';
        ilitek_info("change proeject id [%s] succeeded\n", g_ilitek_ts->project_id);
    }

    ilitek_info("read project id = %s\n", g_ilitek_ts->project_id);

    return 0;
}

void ilitek_read_flash_info(u8 cmd, int len)
{
    int i;
    u16 flash_id = 0, flash_mid = 0;
    u8 buf[4] = { 0 };

    /*
     * This command is used to fix the bug of spi clk for 7807F-AB
     * when operating with its flash.
     */
    if (g_ilitek_cfg->chip_info->chip_id == ILITEK_ILI7807 &&
        g_ilitek_cfg->chip_info->chip_type == ILI7807_TYPE_F_AB) {
        ilitek_config_ice_mode_write(0x4100C, 0x01, 1);
        mdelay(25);
    }

    ilitek_config_ice_mode_write(0x41000, 0x0, 1);    /* CS high */
    ilitek_config_ice_mode_write(0x41004, 0x66aa55, 3);    /* Key */

    ilitek_config_ice_mode_write(0x41008, cmd, 1);
    for (i = 0; i < len; i++) {
        ilitek_config_ice_mode_write(0x041008, 0xFF, 1);
        buf[i] = ilitek_config_ice_mode_read(0x41010);
    }

    ilitek_config_ice_mode_write(0x041000, 0x1, 1);    /* CS high */

    /* look up flash info and init its struct after obtained flash id. */
    flash_mid = buf[0];
    flash_id = buf[1] << 8 | buf[2];
    core_flash_init(flash_mid, flash_id);
}

int ilitek_config_get_chip_id (u32 dts_chip_id)
{
    int i = 0;
    int ret = 0;
    u32 pid_data = 0;
    u32 pid_addr = g_ilitek_cfg->chip_info->pid_addr;

    ret = ilitek_config_ice_mode_enable();
    if (ret < 0) {
        ilitek_err(" enter ice mode failed, ret = %d\n", ret);
        goto err_out;
    }

    mdelay(20);

    pid_data = ilitek_config_ice_mode_read(pid_addr);
    g_ilitek_cfg->chip_info->chip_pid = pid_data;

    ilitek_debug(DEBUG_CONFIG, "pid_data = 0x%x\n", g_ilitek_cfg->chip_info->chip_pid);

    if (pid_data && dts_chip_id == (pid_data >> 16)) {
        ilitek_check_chip_id(pid_data);
    } else {
        ilitek_err("match chip id failed, picket data : 0x%x\n", pid_data);
        ret = -EINVAL;
        goto err_out;
    }

    ilitek_read_flash_info(0x9F, 4);

    ilitek_info("match chip id suceeded\n");

err_out:
    ilitek_config_ic_reset();

    mdelay(150);

    return ret;
}

u32 ilitek_config_calc_package_length(pro_fw_modes mode)
{
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;

    if (IS_ERR_OR_NULL(p_pro) ||
        IS_ERR_OR_NULL(p_pro->funcs) ||
        IS_ERR_OR_NULL(p_pro->funcs->calc_packget_length)) {
        ilitek_err("protocol, funcs or calc_packget_length is NULL\n");
        return 0;
    }

    return p_pro->funcs->calc_packget_length(mode);
}

int ilitek_config_parse_package(u8 package_id)
{
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;

    if (IS_ERR_OR_NULL(p_pro) ||
        IS_ERR_OR_NULL(p_pro->funcs) ||
        IS_ERR_OR_NULL(p_pro->funcs->parse_packget)) {
        ilitek_err("protocol, funcs or parse_packget is NULL\n");
        return -EINVAL;
    }

    return p_pro->funcs->parse_packget(package_id);
}

int ilitek_config_mode_ctrl(pro_fw_modes mode, u8 *data)
{
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;

    if (IS_ERR_OR_NULL(p_pro) ||
        IS_ERR_OR_NULL(p_pro->funcs) ||
        IS_ERR_OR_NULL(p_pro->funcs->mode_control)) {
        ilitek_err("protocol, funcs or mode_control is NULL\n");
        return -EINVAL;
    }

    return p_pro->funcs->mode_control(mode, data);
}

int ilitek_config_init(u32 dts_chip_id)
{
    int index = 0;

    for (index = 0; index < ILITEK_IC_NUMS; index++) {
        if (dts_chip_id == g_chip_list[index].chip_id) {
            ilitek_info("driver ic : ili%x\n", dts_chip_id);
            break;
        }
    }

    if (index == ILITEK_IC_NUMS) {
        ilitek_err("driver don't support ic: ili%x\n", dts_chip_id);
        return -EINVAL;
    }

    g_ilitek_cfg = kzalloc(sizeof(*g_ilitek_cfg), GFP_KERNEL);
    if (IS_ERR_OR_NULL(g_ilitek_cfg)) {
        ilitek_err("alloc ilitek_config failed\n");
        return -ENOMEM;
    }

    g_ilitek_cfg->chip_info = &g_chip_list[index];

    g_ilitek_cfg->ice_mode_enable = false;

    g_ilitek_ts->cfg = g_ilitek_cfg;

    ilitek_debug(DEBUG_CONFIG, "chip_id = 0x%x\n",
        g_ilitek_cfg->chip_info->chip_id);
    ilitek_debug(DEBUG_CONFIG, "ice_mode_addr = 0x%x\n",
        g_ilitek_cfg->chip_info->ice_mode_addr);
    ilitek_debug(DEBUG_CONFIG, "delay high = %d\n",
        g_ilitek_cfg->chip_info->delay_time_high);
    ilitek_debug(DEBUG_CONFIG, "delay low = %d\n",
        g_ilitek_cfg->chip_info->delay_time_low);
    ilitek_debug(DEBUG_CONFIG, "delay edge = %d\n",
        g_ilitek_cfg->chip_info->delay_time_edge);

    ilitek_info("config init succeeded\n");

    return 0;
}

void ilitek_config_exit(void)
{
    if (g_ilitek_cfg) {
        kfree(g_ilitek_cfg);
        g_ilitek_cfg = NULL;
    }

    g_ilitek_ts->cfg = NULL;

    ilitek_info("config exit succeeded\n");
}
